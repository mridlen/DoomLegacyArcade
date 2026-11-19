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

// One readout texel: where it is, and where in its readout.
typedef struct {
    uint16_t  x, y;
    byte      fx, fy;     // offset from the readout's top-left texel
    byte      rw;         // readout width
    byte      pairx;      // cells two texels wide
} ro_texel_t;

typedef struct {
    int          texnum;
    int          w, h;
    byte       * index;     // the original texels, w*h palette indices
    int          ntexels;
    ro_texel_t * texels;
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

static void ro_find( int texnum, const byte * palette )
{
    texture_t * tex;
    char name[9];
    byte * pairs, * glass, * frame;
    int w, h, i, n, nt;
    ro_tex_t * r;

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
    if( doom_screen_readouts( w, h, pairs, palette, glass, frame ) <= 0 )
        goto done;

    n = w * h;
    nt = 0;
    for( i = 0; i < n; i++ )
        if( frame[ i*4 + 2 ] )   // a readout texel has its readout's width
            nt++;
    if( nt == 0 )
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
    r->texels = malloc( nt * sizeof(ro_texel_t) );
    r->shown = malloc( nt );
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
        r->shown[ r->ntexels ] = r->index[i];   // the art, to start with
        r->ntexels++;
    }
    ro_num++;
done:
    free( pairs );
    free( glass );
    free( frame );
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
        byte want = restore ? r->index[ t->y * r->w + t->x ] : ro_texel_now( r, t, now );
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
                            : ro_texel_now( r, &r->texels[i], now );
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
