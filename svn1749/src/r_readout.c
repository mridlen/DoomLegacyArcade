// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 Doom Legacy Arcade.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
//
// [Arcade] Busy Computer Panels: the LED readouts on computer textures (the
// green grids on COMPUTE2 and the like) change now and then, cell by cell,
// so the consoles look busy.  docs/arcade/derp-glow.md.
//
// The idea and the readout finder are DERP's (derp/screen_glass.h, GPLv3).
// DERP does it in a shader; here the texels themselves are rewritten, which
// works in both renderers:
//
//   software  the texture's composited copy (texren->cache), which a wall
//             column reads as one solid run from columnofs[x] +
//             pixel_data_offset whatever the storage model
//   OpenGL    the cache's RGBA copy (HWR_Texture_Texels), then the driver
//             drops the uploaded one so its next use uploads it again
//
// A cell shows, now and then, the original pixels of another cell in its row
// (DERP's rule), so the panel keeps its share of lit cells and only the
// pattern moves.  Which, and when, is a hash of the texture, the cell and the
// level clock: no random numbers, nothing the game can see, so demos and
// netgames cannot notice.  Turned off, every texel is put back.

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "r_data.h"
#include "r_state.h"
#include "r_main.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_swap.h"
#include "i_video.h"
#include "v_video.h"
#include "r_readout.h"
#ifdef HWRENDER
#include "hardware/hw_glow.h"
#endif

// DERP's file is kept verbatim; it writes "if (a) b; if (c) d;" on a line.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include "derp/screen_glass.h"
#pragma GCC diagnostic pop

consvar_t cv_readouts = { "busy_readouts", "Off", CV_SAVE, CV_OnOff };

void R_Readout_Register( void )
{
    CV_RegisterVar( &cv_readouts );
}

// What a texel follows.
enum {
    RO_READOUT = 0,   // a cell of one of DERP's LED readouts (outside screens)
    RO_CELL,          // an indicator square on a screen: blinks
    RO_METER          // a segment slot of a level meter on a screen
};

// One animated texel: where it is, and what decides what it shows.
typedef struct {
    uint16_t  x, y;
    byte      kind;       // RO_*
    byte      fx, fy;     // readout: offset from its top-left texel; meter: fx = slot
    byte      rw;         // readout width
    byte      pairx;      // readout cells two texels wide
    byte      dx, dy;     // meter: offset within the slot
    uint16_t  elem;       // cell or meter: which element
} ro_texel_t;

// An indicator square or a level meter found on a screen (ro_find_elements).
typedef struct {
    byte      kind;          // RO_CELL or RO_METER
    byte      nslots;        // meter: slots, numbered from the lit end
    uint32_t  seed;
    uint16_t  litx, lity;    // meter: a lit segment's top-left texel
    uint16_t  unlitx, unlity;  // meter: an unlit one's
    byte      dimunlit;      // meter: no unlit segment painted; dim the lit one
} ro_elem_t;

typedef struct {
    int          texnum;
    int          w, h;
    byte       * index;     // the original texels, w*h palette indices
    int          ntexels;
    ro_texel_t * texels;
    int          nelems;
    ro_elem_t  * elems;
    byte       * shown;     // what each readout texel shows now
    // OpenGL: the block geometry the last write used, and whether a change
    // is waiting to be uploaded.
    int          gl_bw, gl_bh;
} ro_tex_t;

static ro_tex_t * ro_tex = NULL;
static int        ro_num = 0;
static side_t   * ro_sides = NULL;     // the level they were found for
static uint32_t   ro_numsides = 0;
static boolean    ro_applied = false;  // texels may differ from the art

// How long a cell holds what it shows before it may change, in tics.
#define RO_HOLD_MIN   35
#define RO_HOLD_SPAN  105
// Cells only change on this many tics' boundaries, and nothing is done in
// between: hundreds of cells on their own clocks otherwise changed some
// texture nearly every frame, and in OpenGL every change is an upload with
// its mipmaps.
#define RO_QUANTUM    5
static tic_t      ro_last_step = (tic_t) -1;

static uint32_t ro_hash( uint32_t a, uint32_t b, uint32_t c, uint32_t d )
{
    uint32_t h = 2166136261u;
    h = (h ^ a) * 16777619u;
    h = (h ^ b) * 16777619u;
    h = (h ^ c) * 16777619u;
    h = (h ^ d) * 16777619u;
    h ^= h >> 15;  h *= 0x2c1b3c6d;  h ^= h >> 12;
    return h;
}

// An indicator's off look: its own colour, darker, from the light tables.
#define RO_DIM_LEVEL  22   // of the 32 light levels; 0 is full bright

static byte ro_dim( byte idx )
{
    return reg_colormaps ? reg_colormaps[ LIGHTTABLE( RO_DIM_LEVEL ) + idx ] : idx;
}

// A screen indicator is on about three quarters of the time, each holding
// 1 to 5 seconds before it may change.
static boolean ro_cell_on( ro_elem_t * e, tic_t now )
{
    uint32_t hold = 35 + e->seed % 140;
    uint32_t step = ( now + (e->seed >> 8) % hold ) / hold;
    return ( ro_hash( e->seed, step, 0xce11, 0 ) % 8 ) < 6;
}

// A level meter's level, 0..nslots, a new one every few tenths of a second,
// leaning toward the middle so it bounces rather than flickers end to end.
static int ro_meter_level( ro_elem_t * e, tic_t now )
{
    uint32_t step = now / 10;
    uint32_t a = ro_hash( e->seed, step, 0x3e7e, 1 ) % (e->nslots + 1);
    uint32_t b = ro_hash( e->seed, step, 0x3e7e, 2 ) % (e->nslots + 1);
    return (int)( (a + b + 1) / 2 );
}

static byte ro_texel_now( ro_tex_t * r, ro_texel_t * t, tic_t now );

// The palette index any animated texel should show at time `now`.
static byte ro_want( ro_tex_t * r, ro_texel_t * t, tic_t now )
{
    ro_elem_t * e;
    switch( t->kind )
    {
     case RO_CELL:
        e = &r->elems[ t->elem ];
        return ro_cell_on( e, now )? r->index[ t->y * r->w + t->x ]
                                   : ro_dim( r->index[ t->y * r->w + t->x ] );
     case RO_METER:
        e = &r->elems[ t->elem ];
        if( t->fx < ro_meter_level( e, now ) )
            return r->index[ (e->lity + t->dy) * r->w + e->litx + t->dx ];
        if( e->dimunlit )
            return ro_dim( r->index[ (e->lity + t->dy) * r->w + e->litx + t->dx ] );
        return r->index[ (e->unlity + t->dy) * r->w + e->unlitx + t->dx ];
     default:
        return ro_texel_now( r, t, now );
    }
}

// The palette index readout texel t should show at time `now`.
static byte ro_texel_now( ro_tex_t * r, ro_texel_t * t, tic_t now )
{
    int cw = t->pairx ? 2 : 1;
    int x0 = t->x - t->fx, y0 = t->y - t->fy;
    int col = t->fx / cw, ncols = (t->rw + cw - 1) / cw;
    uint32_t cell = ro_hash( r->texnum, x0 | (y0 << 8), t->fy, col );
    uint32_t hold = RO_HOLD_MIN + cell % RO_HOLD_SPAN;
    uint32_t step = ( now + (cell >> 8) % hold ) / hold;
    uint32_t pick = ro_hash( cell, step, 0x5eed, 0 );
    int src = col, sx;
    // Most of the time a cell shows another cell's light; sometimes its own.
    if( (pick & 3) && ncols > 1 )
        src = (pick >> 2) % ncols;
    sx = x0 + src * cw + (t->fx - col * cw);
    if( sx >= x0 + t->rw )   // the last, part cell
        sx = t->x;
    return r->index[ t->y * r->w + sx ];
}

static void ro_free( void )
{
    int i;
    for( i = 0; i < ro_num; i++ )
    {
        free( ro_tex[i].index );
        free( ro_tex[i].texels );
        free( ro_tex[i].shown );
        free( ro_tex[i].elems );
    }
    free( ro_tex );
    ro_tex = NULL;
    ro_num = 0;
}

// The texture composited into palette indices and coverage pairs, row by
// row, as DERP's finder takes it.
static byte * ro_composite( int texnum )
{
    texture_t * tex = textures[texnum];
    int w = tex->width, h = tex->height, i;
    byte * pairs = calloc( (size_t) w * h * 2, 1 );
    if( pairs == NULL )
        return NULL;
    for( i = 0; i < tex->patchcount; i++ )
    {
        texpatch_t * tp = &tex->patches[i];
        int size = W_LumpLength( tp->lumpnum ), pw, x;
        byte * data;
        if( size < 8 )  continue;
        data = W_CacheLumpNum( tp->lumpnum, PU_STATIC );
        pw = (int16_t) LE_SWAP16( ((uint16_t*)data)[0] );
        if( pw > 0 && 8 + pw * 4 <= size )
        {
            for( x = 0; x < pw; x++ )
            {
                int tx = tp->originx + x, previous = -1;
                uint32_t pos;
                if( tx < 0 || tx >= w )  continue;
                pos = LE_SWAP32( ((uint32_t*)(data + 8))[x] );
                while( pos + 4 <= (uint32_t) size && data[pos] != 255 )
                {
                    int top = data[pos], len = data[pos+1], y;
                    if( pos + 4 + len > (uint32_t) size )  break;
                    if( top <= previous )  top += previous;   // tall patches
                    previous = top;
                    for( y = 0; y < len; y++ )
                    {
                        int ty = tp->originy + top + y;
                        if( ty >= 0 && ty < h )
                        {
                            pairs[ ((size_t) ty * w + tx) * 2 ] = data[pos + 3 + y];
                            pairs[ ((size_t) ty * w + tx) * 2 + 1 ] = 255;
                        }
                    }
                    pos += len + 4;
                }
            }
        }
        Z_ChangeTag( data, PU_CACHE );
    }
    return pairs;
}

// ---- screen elements
//
// DERP's finder looks for readouts only outside screens; COMPSTA1/2 draw
// every light they have inside one (indicator squares, a level meter, status
// dots), so nothing on them moved.  This finds, inside the screens: blobs
// of one hue (doom_readout_hue), 2-8 texels across, three-quarters filled,
// grouped by hue, size and exact pixel shape.  Three or more identical ones
// are indicators -- letters are neither that solid nor repeated that
// exactly.  A group stacked in one column at a regular step, with dimmer
// segments carrying on beyond it, is a level meter instead.

typedef struct {
    int       x0, y0, w, h, count, hue;
    uint64_t  mask;     // the blob's texels within its box, w*h <= 64
    int       group;
} ro_comp_t;

typedef struct {
    ro_texel_t * texels;  int ntexels, cap;
    ro_elem_t  * elems;   int nelems, ecap;
} ro_build_t;

static ro_texel_t * ro_add_texel( ro_build_t * b )
{
    if( b->ntexels == b->cap )
    {
        int cap = b->cap ? b->cap * 2 : 256;
        ro_texel_t * t = realloc( b->texels, cap * sizeof(ro_texel_t) );
        if( t == NULL )  return NULL;
        b->texels = t;
        b->cap = cap;
    }
    memset( &b->texels[ b->ntexels ], 0, sizeof(ro_texel_t) );
    return &b->texels[ b->ntexels++ ];
}

static int ro_add_elem( ro_build_t * b, byte kind, uint32_t seed )
{
    if( b->nelems >= 0xffff )
        return -1;
    if( b->nelems == b->ecap )
    {
        int cap = b->ecap ? b->ecap * 2 : 32;
        ro_elem_t * e = realloc( b->elems, cap * sizeof(ro_elem_t) );
        if( e == NULL )  return -1;
        b->elems = e;
        b->ecap = cap;
    }
    memset( &b->elems[ b->nelems ], 0, sizeof(ro_elem_t) );
    b->elems[ b->nelems ].kind = kind;
    b->elems[ b->nelems ].seed = seed;
    return b->nelems++;
}

static int ro_bright( const byte * palette, byte idx )
{
    const byte * c = palette + idx * 3;
    int hi = c[0] > c[1] ? c[0] : c[1];
    return hi > c[2] ? hi : c[2];
}

// Whether a slot-sized box at x,y lies on a screen and has something
// painted in it (an unlit meter segment is dim, not black).
static boolean ro_slot_ok( int x, int y, int sw, int sh, int w, int h,
                           const byte * pairs, const byte * palette, const byte * glass )
{
    int dx, dy, ink = 0;
    if( x < 0 || y < 0 || x + sw > w || y + sh > h )
        return false;
    for( dy = 0; dy < sh; dy++ )
        for( dx = 0; dx < sw; dx++ )
        {
            size_t i = (size_t)(y + dy) * w + x + dx;
            if( glass[i] == DOOM_GLASS_NONE || ! pairs[ i*2 + 1 ] )
                return false;
            if( ro_bright( palette, pairs[ i*2 ] ) >= 40 )
                ink++;
        }
    return ink * 2 >= sw * sh;
}

static void ro_find_elements( int texnum, int w, int h, const byte * pairs,
                              const byte * palette, const byte * glass, ro_build_t * b )
{
    size_t n = (size_t) w * h, i;
    byte * hue = calloc( n, 1 ), * seen = calloc( n, 1 );
    int * stack = malloc( n * sizeof(int) );
    ro_comp_t * comps = NULL;
    int ncomps = 0, capc = 0, c, ngroups = 0;

    if( hue == NULL || seen == NULL || stack == NULL )
        goto done;
    for( i = 0; i < n; i++ )
        if( pairs[ i*2 + 1 ] && glass[i] != DOOM_GLASS_NONE )
            hue[i] = (byte) doom_readout_hue( palette + pairs[ i*2 ] * 3 );

    // The blobs.
    for( i = 0; i < n; i++ )
    {
        int top = 0, x0 = w, y0 = h, x1 = -1, y1 = -1, count = 0, bw, bh, k;
        if( ! hue[i] || seen[i] )  continue;
        stack[top++] = (int) i;
        seen[i] = 1;
        while( top )
        {
            int q = stack[--top], x = q % w, y = q / w;
            int nx[4] = { x + 1, x - 1, x, x }, ny[4] = { y, y, y + 1, y - 1 };
            count++;
            if( x < x0 )  x0 = x;
            if( x > x1 )  x1 = x;
            if( y < y0 )  y0 = y;
            if( y > y1 )  y1 = y;
            for( k = 0; k < 4; k++ )
            {
                size_t m;
                if( nx[k] < 0 || ny[k] < 0 || nx[k] >= w || ny[k] >= h )  continue;
                m = (size_t) ny[k] * w + nx[k];
                if( hue[m] == hue[i] && ! seen[m] )  { seen[m] = 1;  stack[top++] = (int) m; }
            }
        }
        bw = x1 - x0 + 1;
        bh = y1 - y0 + 1;
        // Solid: 85% filled.  Indicator squares are 88-100%; the blocky
        // screen font's letters are 80% at most, and a test at 75% marked
        // whole words on COMPSTA1 as indicators.
        if( bw < 2 || bw > 8 || bh < 1 || bh > 8 || count < 4 || count * 20 < bw * bh * 17 )
            continue;
        if( ncomps == capc )
        {
            ro_comp_t * g;
            capc = capc ? capc * 2 : 64;
            g = realloc( comps, capc * sizeof(ro_comp_t) );
            if( g == NULL )  goto done;
            comps = g;
        }
        {
            ro_comp_t * cp = &comps[ ncomps++ ];
            int dx, dy;
            cp->x0 = x0;  cp->y0 = y0;  cp->w = bw;  cp->h = bh;
            cp->count = count;  cp->hue = hue[i];  cp->group = -1;  cp->mask = 0;
            for( dy = 0; dy < bh; dy++ )
                for( dx = 0; dx < bw; dx++ )
                    if( hue[ (size_t)(y0 + dy) * w + x0 + dx ] == hue[i] )
                        cp->mask |= (uint64_t) 1 << (dy * bw + dx);
        }
    }

    // Identical blobs, in groups of three or more.
    for( c = 0; c < ncomps; c++ )
    {
        int d, members = 0;
        if( comps[c].group >= 0 )  continue;
        for( d = c; d < ncomps; d++ )
            if( comps[d].group < 0 && comps[d].hue == comps[c].hue && comps[d].w == comps[c].w
                && comps[d].h == comps[c].h && comps[d].mask == comps[c].mask )
            {
                comps[d].group = ngroups;
                members++;
            }
        if( members < 3 )
        {
            for( d = c; d < ncomps; d++ )
                if( comps[d].group == ngroups )  comps[d].group = -2;   // not a group
            continue;
        }

        {
            // A meter: members in one column at a regular step.  Collect the
            // column of the first member, sorted by y.
            int col[16], ncol = 0, step = 0, regular = 1, k, gid = ngroups;
            int sw = comps[c].w, sh = comps[c].h;
            for( d = c; d < ncomps && ncol < 16; d++ )
                if( comps[d].group == gid && comps[d].x0 == comps[c].x0 )
                    col[ncol++] = d;
            for( k = 1; k < ncol; k++ )   // insertion sort by y
            {
                int v = col[k], j = k - 1;
                while( j >= 0 && comps[ col[j] ].y0 > comps[v].y0 )  { col[j+1] = col[j];  j--; }
                col[j+1] = v;
            }
            if( ncol >= 3 )
            {
                step = comps[ col[1] ].y0 - comps[ col[0] ].y0;
                for( k = 2; k < ncol; k++ )
                    if( comps[ col[k] ].y0 - comps[ col[k-1] ].y0 != step )  regular = 0;
            }
            // Segments at least two texels tall: a column of one-texel
            // lines is a text list, not a meter.
            if( sh >= 2 && ncol >= 3 && regular && step >= sh && step <= sh + 3 )
            {
                // Carry on above and below for dimmer segments.
                int up = 0, down = 0, y;
                for( y = comps[ col[0] ].y0 - step;
                     up < 16 && ro_slot_ok( comps[c].x0, y, sw, sh, w, h, pairs, palette, glass );
                     y -= step )
                    up++;
                for( y = comps[ col[ncol-1] ].y0 + step;
                     down < 16 && ro_slot_ok( comps[c].x0, y, sw, sh, w, h, pairs, palette, glass );
                     y += step )
                    down++;
                // Four or more segments.  With no dim segments painted beyond
                // the lit ones (the column of red squares on PLANET1) the
                // unlit look is the lit one, darker.
                if( ncol + up + down >= 4 && ncol + up + down <= 32 )
                {
                    // Fills from the end the lit segments are at.
                    int fromtop = ( up == 0 );   // lit at the top, dim below
                    int total = ncol + up + down, s2;
                    int firsty = comps[ col[0] ].y0 - up * step;
                    int e = ro_add_elem( b, RO_METER, ro_hash( texnum, comps[c].x0, comps[c].y0, 0x3e7e ) );
                    if( e < 0 )  goto done;
                    b->elems[e].nslots = (byte) total;
                    b->elems[e].litx = comps[c].x0;
                    b->elems[e].lity = fromtop ? comps[ col[0] ].y0 : comps[ col[ncol-1] ].y0;
                    b->elems[e].unlitx = comps[c].x0;
                    b->elems[e].unlity = fromtop ? comps[ col[ncol-1] ].y0 + down * step
                                                 : firsty;
                    b->elems[e].dimunlit = ( up == 0 && down == 0 );
                    for( s2 = 0; s2 < total; s2++ )
                    {
                        int sy = firsty + s2 * step, dx, dy;
                        int slot = fromtop ? s2 : total - 1 - s2;   // 0 at the lit end
                        for( dy = 0; dy < sh; dy++ )
                            for( dx = 0; dx < sw; dx++ )
                            {
                                ro_texel_t * t = ro_add_texel( b );
                                if( t == NULL )  goto done;
                                t->x = comps[c].x0 + dx;
                                t->y = sy + dy;
                                t->kind = RO_METER;
                                t->fx = (byte) slot;
                                t->dx = dx;  t->dy = dy;
                                t->elem = (uint16_t) e;
                            }
                    }
                    for( k = 0; k < ncol; k++ )
                        comps[ col[k] ].group = -3;   // used by the meter
                }
            }
        }

        // The rest of the group blink, each on its own -- if they are
        // squares: within a texel of square and two or more tall.  Bars of a
        // graph and dashes in a text list repeat exactly too.
        for( d = c; d < ncomps; d++ )
        {
            int dx, dy, e;
            if( comps[d].group != ngroups )  continue;
            if( comps[d].h < 2 || comps[d].w - comps[d].h > 1 || comps[d].h - comps[d].w > 1 )
                continue;
            // And completely solid.  The screen font's letters are 3x3
            // blocks with one texel out (8 of 9); the indicator squares are
            // all 9 (COMPSTA1's blue ones).  Only that tells them apart.
            if( comps[d].count != comps[d].w * comps[d].h )
                continue;
            e = ro_add_elem( b, RO_CELL, ro_hash( texnum, comps[d].x0, comps[d].y0, 0xce11 ) );
            if( e < 0 )  goto done;
            for( dy = 0; dy < comps[d].h; dy++ )
                for( dx = 0; dx < comps[d].w; dx++ )
                {
                    ro_texel_t * t;
                    if( ! (comps[d].mask & ((uint64_t) 1 << (dy * comps[d].w + dx))) )  continue;
                    t = ro_add_texel( b );
                    if( t == NULL )  goto done;
                    t->x = comps[d].x0 + dx;
                    t->y = comps[d].y0 + dy;
                    t->kind = RO_CELL;
                    t->elem = (uint16_t) e;
                }
        }
        ngroups++;
    }
done:
    free( hue );
    free( seen );
    free( stack );
    free( comps );
}

static void ro_find( int texnum, const byte * palette )
{
    texture_t * tex;
    char name[9];
    byte * pairs, * glass, * frame;
    int w, h, i, n, nt;
    ro_tex_t * r;
    ro_build_t eb;

    memset( &eb, 0, sizeof(eb) );

    if( texnum <= 0 || texnum >= numtextures )
        return;
    for( i = 0; i < ro_num; i++ )
        if( ro_tex[i].texnum == texnum )
            return;
    tex = textures[texnum];
    memcpy( name, tex->name, 8 );
    name[8] = 0;
    for( i = 0; i < 8; i++ )
        name[i] = toupper( (unsigned char) name[i] );
    if( ! doom_screen_texture( name, 0 ) )
        return;
    w = tex->width;
    h = tex->height;
    if( w <= 0 || h <= 0 || w > 1024 || h > 1024 )
        return;

    pairs = ro_composite( texnum );
    glass = malloc( (size_t) w * h );
    frame = calloc( (size_t) w * h * 4, 1 );
    if( pairs == NULL || glass == NULL || frame == NULL )
        goto done;
    doom_screen_glass( w, h, pairs, palette, glass, NULL );
    doom_screen_readouts( w, h, pairs, palette, glass, frame );
    ro_find_elements( texnum, w, h, pairs, palette, glass, &eb );

    n = w * h;
    nt = 0;
    for( i = 0; i < n; i++ )
        if( frame[ i*4 + 2 ] )   // a readout texel has its readout's width
            nt++;
    if( nt + eb.ntexels == 0 )
        goto done;

    {
        ro_tex_t * grown = realloc( ro_tex, (ro_num + 1) * sizeof(ro_tex_t) );
        if( grown == NULL )
            goto done;
        ro_tex = grown;
    }
    r = &ro_tex[ ro_num ];
    memset( r, 0, sizeof(*r) );
    r->texnum = texnum;
    r->w = w;
    r->h = h;
    r->index = malloc( n );
    r->texels = malloc( (nt + eb.ntexels) * sizeof(ro_texel_t) );
    r->shown = malloc( nt + eb.ntexels );
    if( r->index == NULL || r->texels == NULL || r->shown == NULL )
    {
        free( r->index );  free( r->texels );  free( r->shown );
        goto done;
    }
    for( i = 0; i < n; i++ )
        r->index[i] = pairs[ i*2 ];
    r->ntexels = 0;
    for( i = 0; i < n; i++ )
    {
        byte * f = &frame[ i*4 ];
        ro_texel_t * t;
        if( ! f[2] )  continue;
        t = &r->texels[ r->ntexels ];
        t->x = i % w;
        t->y = i / w;
        t->fx = f[0] & 0x7f;           // DOOM_READOUT_LIT is the top bit
        t->fy = f[1];
        t->rw = f[2] & 0x7f;
        t->pairx = ( f[2] & DOOM_READOUT_PAIR )? 1 : 0;
        t->kind = RO_READOUT;
        t->dx = t->dy = 0;
        t->elem = 0;
        r->shown[ r->ntexels ] = r->index[i];   // the art, to start with
        r->ntexels++;
    }
    for( i = 0; i < eb.ntexels; i++ )
    {
        r->texels[ r->ntexels ] = eb.texels[i];
        r->shown[ r->ntexels ] = r->index[ eb.texels[i].y * w + eb.texels[i].x ];
        r->ntexels++;
    }
    r->nelems = eb.nelems;
    r->elems = eb.elems;     // ownership moves to the texture
    eb.elems = NULL;
    ro_num++;
done:
    free( pairs );
    free( glass );
    free( frame );
    free( eb.texels );
    free( eb.elems );
}

// Every readout on the level's walls, once per level.
static void ro_build( void )
{
    uint32_t i;
    byte * palette;
    lumpnum_t pal = W_CheckNumForName( "PLAYPAL" );

    ro_free();
    ro_sides = sides;
    ro_numsides = numsides;
    ro_applied = false;
    if( ! VALID_LUMP( pal ) || W_LumpLength( pal ) < 768 || textures == NULL )
        return;
    palette = W_CacheLumpNum( pal, PU_STATIC );
    for( i = 0; i < numsides; i++ )
    {
        ro_find( sides[i].toptexture, palette );
        ro_find( sides[i].midtexture, palette );
        ro_find( sides[i].bottomtexture, palette );
    }
    Z_ChangeTag( palette, PU_CACHE );
    if( ro_num )
        GenPrintf( EMSG_info, "Busy computer panels: %d textures\n", ro_num );
}

// ---- software

static void ro_apply_soft( ro_tex_t * r, boolean restore, tic_t now )
{
    texture_render_t * texren;
    int i;
    if( texture_render == NULL )
        return;
    texren = &texture_render[ r->texnum ];
    if( texren->cache == NULL || texren->columnofs == NULL || texren->width < r->w )
        return;   // not drawn yet; picked up once it is
    // A column with holes or several posts is not one solid run, and
    // writing into it as one would land on its post headers.  Computer
    // panels are solid; anything that is not is left alone.
    if( texren->detect & (TD_hole | TD_odd_post) )
        return;
    for( i = 0; i < r->ntexels; i++ )
    {
        ro_texel_t * t = &r->texels[i];
        byte want = restore ? r->index[ t->y * r->w + t->x ] : ro_want( r, t, now );
        byte * px = texren->cache + texren->columnofs[ t->x ] + texren->pixel_data_offset + t->y;
        if( *px != want )
            *px = want;
    }
}

// ---- OpenGL

#ifdef HWRENDER
static void ro_apply_gl( ro_tex_t * r, boolean restore, tic_t now )
{
    int i, bw, bh, bx, by, changed = 0;
    boolean fresh;
    RGBA_t * block;

    for( i = 0; i < r->ntexels; i++ )
    {
        byte want = restore ? r->index[ r->texels[i].y * r->w + r->texels[i].x ]
                            : ro_want( r, &r->texels[i], now );
        if( r->shown[i] != want )
        {
            r->shown[i] = want;
            changed = 1;
        }
    }
    if( ! changed )
        return;
    block = HWR_Texture_Texels( r->texnum, &bw, &bh, &fresh );
    if( block == NULL || bw <= 0 || bh <= 0 )
        return;
    {
        // Every block texel over a readout texel takes what it shows.  The
        // block may be scaled from the texture: block x samples texture
        // column (x * width) / blockwidth, as HWR_DrawPatchInCache does.
        fixed_t xstep = (r->w << 16) / bw, ystep = (r->h << 16) / bh;
        byte * map = calloc( (size_t) r->w * r->h, 1 );
        int16_t * which = malloc( (size_t) r->w * r->h * sizeof(int16_t) );
        if( map == NULL || which == NULL )
        {
            free( map );  free( which );
            return;
        }
        for( i = 0; i < r->ntexels; i++ )
        {
            int k = r->texels[i].y * r->w + r->texels[i].x;
            map[k] = 1;
            which[k] = i;
        }
        for( by = 0; by < bh; by++ )
        {
            int ty = (int)( ((int64_t) by * ystep) >> 16 );
            if( ty >= r->h )  continue;
            for( bx = 0; bx < bw; bx++ )
            {
                int tx = (int)( ((int64_t) bx * xstep) >> 16 ), k;
                if( tx >= r->w )  continue;
                k = ty * r->w + tx;
                if( ! map[k] )  continue;
                {
                    RGBA_t c = V_GetColor( r->shown[ which[k] ] );
                    c.s.alpha = 255;
                    block[ by * bw + bx ] = c;
                }
            }
        }
        free( map );
        free( which );
    }
    HWR_Texture_Reupload( r->texnum );
    (void) fresh;
}
#endif

// Once a frame, before any view is drawn (D_Display).
void R_Readout_Frame( void )
{
    int i;
    boolean on = cv_readouts.EV && gamestate == GS_LEVEL;

    if( ! on && ! ro_applied )
        return;
    if( sides != ro_sides || numsides != ro_numsides )
    {
        if( ! on )
            return;
        ro_build();
        ro_last_step = (tic_t) -1;
    }
    {
        tic_t step = leveltime / RO_QUANTUM;
        // Software also re-checks on other frames, cheaply: a purged and
        // regenerated texture cache holds the art again until rewritten.
        if( on && step == ro_last_step && rendermode != render_soft )
            return;
        ro_last_step = step;
        for( i = 0; i < ro_num; i++ )
        {
#ifdef HWRENDER
            if( rendermode != render_soft )
                ro_apply_gl( &ro_tex[i], ! on, step * RO_QUANTUM );
            else
#endif
                ro_apply_soft( &ro_tex[i], ! on, step * RO_QUANTUM );
        }
    }
    ro_applied = on;
}
