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
// [Arcade] DERP glow effects, OpenGL only.  docs/arcade/derp-glow.md.
//
// The ideas, the list of glowing artwork (derp/emissive.h) and the monsters'
// eye pixels (derp/sprite_eyes_stock.h) come from DERP, the DOOM Engine
// Remaster Project, which is GPLv3; see derp/NOTICE.  DERP builds them into
// a renderer of its own; here they are extra passes over the fixed-function
// one:
//
//   Glowing Lights   A second copy of a wall or flat drawn on top of it,
//                    holding only the pixels emissive.h says glow, at full
//                    brightness.  How much of it shows follows the sector's
//                    light, so a lamp in a blinking room blinks with it.
//   Glowing Liquids  The same for nukage, slime and lava flats, plus an
//                    additive gradient in the liquid's colour up the foot of
//                    every wall standing in it.
//   Glowing Eyes     A second copy of a living monster's sprite holding only
//                    its eye pixels, unlit, so eyes show in the dark.
//
// Everything here is drawing only.  Nothing reads or writes game state, and
// there is no random number anywhere, so demos and netgames cannot notice.
//
// The masks are built from the same texel blocks the cache uploads, by the
// cache's own compositor (HWR_Glow_Composite_Texture, HWR_MakePatch), so
// they line up texel for texel with what they are drawn over.  They live in
// tables freed only after the driver has forgotten them (HWR_Glow_Free);
// their texel blocks are PU_HWRCACHE like every other upload, and a purged
// one is simply rebuilt.

#include <ctype.h>

#include "doomincl.h"
#include "hw_glow.h"
#include "hw_main.h"
#include "hw_glob.h"
#include "hw_drv.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_swap.h"
#include "r_data.h"
#include "r_main.h"
#include "r_state.h"
#include "p_local.h"
#include "p_setup.h"
#include "v_video.h"

#include "derp/emissive.h"
#include "derp/sprite_eyes_stock.h"

consvar_t cv_grglow_lights  = { "gr_glow_lights",  "Off", CV_SAVE, CV_OnOff };
consvar_t cv_grglow_liquids = { "gr_glow_liquids", "Off", CV_SAVE, CV_OnOff };
consvar_t cv_grglow_eyes    = { "gr_glow_eyes",    "Off", CV_SAVE, CV_OnOff };

extern sector_t * gr_frontsector;   // hw_main.c, the sector being drawn

Mipmap_t * hwr_glow_bound = NULL;
int        hwr_glow_texnum = -1;

void HWR_Glow_Register( void )
{
    CV_RegisterVar( &cv_grglow_lights );
    CV_RegisterVar( &cv_grglow_liquids );
    CV_RegisterVar( &cv_grglow_eyes );
}

typedef enum { GLOW_UNKNOWN = 0, GLOW_NONE, GLOW_HAS } glow_state_e;

typedef struct {
    lumpnum_t  lump;    // key, for the lump tables
    byte       state;   // glow_state_e
    byte       kind;    // doom_emissive_kind
    RGBA_t     color;   // a liquid's glow colour, brightest channel 255
    Mipmap_t   mip;     // the mask
} glow_entry_t;

// Wall textures by texture number; flats and sprites by lump, open addressed.
static glow_entry_t * glow_tex = NULL;
static int            glow_numtex = 0;

#define GLOW_FLAT_SLOTS    1024
#define GLOW_SPRITE_SLOTS  4096
static glow_entry_t * glow_flat = NULL;
static glow_entry_t * glow_sprite = NULL;

// The gradient the liquids throw up the walls.
static Mipmap_t glow_ramp;

// The light swing of each sector with an animated light (glow_sector_scale).
static byte * glow_sec_lo = NULL;
static byte * glow_sec_hi = NULL;
static int    glow_numsec = 0;
static sector_t * glow_secbase = NULL;

// Tables are only ever freed here, after HWR_Free_TextureCache has had the
// driver drop every texture and Z_FreeTags has cleared every GR_data owner,
// both of which write into them.
void HWR_Glow_Free( void )
{
    free( glow_tex );     glow_tex = NULL;  glow_numtex = 0;
    free( glow_flat );    glow_flat = NULL;
    free( glow_sprite );  glow_sprite = NULL;
    free( glow_sec_lo );  glow_sec_lo = NULL;
    free( glow_sec_hi );  glow_sec_hi = NULL;
    glow_numsec = 0;
    glow_secbase = NULL;
    hwr_glow_bound = NULL;
    hwr_glow_texnum = -1;
}

static glow_entry_t * glow_lookup( glow_entry_t ** table, int slots, lumpnum_t lump )
{
    unsigned int h, i;
    if( *table == NULL )
    {
        *table = calloc( slots, sizeof(glow_entry_t) );
        if( *table == NULL )  return NULL;
    }
    h = ((uint32_t)lump * 2654435761u) & (slots - 1);
    for( i = 0; i < (unsigned)slots; i++ )
    {
        glow_entry_t * e = &(*table)[ (h + i) & (slots - 1) ];
        if( e->state == GLOW_UNKNOWN )
        {
            e->lump = lump;
            return e;   // new: the caller fills it
        }
        if( e->lump == lump )
            return e;
    }
    return NULL;  // full: that lump just does not glow
}

static void glow_lump_name( lumpnum_t lump, char name[9] )
{
    int i;
    memcpy( name, wadfiles[ WADFILENUM(lump) ]->lumpinfo[ LUMPNUM(lump) ].name, 8 );
    name[8] = 0;
    for( i = 0; i < 8; i++ )
        name[i] = toupper( (unsigned char) name[i] );
}

// The emissive strength of one texel, premultiplied into it.  Returns the
// strength.
static byte glow_texel( int kind, RGBA_t * px, byte coverage )
{
    unsigned char rgb[3] = { px->s.red, px->s.green, px->s.blue };
    byte e = doom_emissive_pixel( kind, rgb, coverage );
    px->s.red   = (px->s.red   * e) / 255;
    px->s.green = (px->s.green * e) / 255;
    px->s.blue  = (px->s.blue  * e) / 255;
    px->s.alpha = e;
    return e;
}

// ---- walls

static glow_entry_t * glow_wall_mask( int texnum )
{
    glow_entry_t * e;

    if( texnum <= 0 || texnum >= numtextures )
        return NULL;
    if( glow_tex == NULL || glow_numtex != numtextures )
    {
        // Only ever grows in a level: numtextures is fixed once loaded.
        if( glow_tex )  return NULL;
        glow_tex = calloc( numtextures, sizeof(glow_entry_t) );
        if( glow_tex == NULL )  return NULL;
        glow_numtex = numtextures;
    }
    e = &glow_tex[texnum];

    if( e->state == GLOW_UNKNOWN )
    {
        char name[9];
        int i;
        memcpy( name, textures[texnum]->name, 8 );
        name[8] = 0;
        for( i = 0; i < 8; i++ )
            name[i] = toupper( (unsigned char) name[i] );
        e->kind = doom_emissive_kind( name, 0 );
        // The skies are composited by a special case; never a light.
        e->state = ( e->kind == DOOM_EMISSIVE_NONE
                     || strncmp( name, "SKY", 3 ) == 0 )? GLOW_NONE : GLOW_HAS;
        if( e->state == GLOW_NONE )
            return NULL;
    }
    if( e->state != GLOW_HAS )
        return NULL;

    if( ! e->mip.downloaded && ! e->mip.GR_data )
    {
        RGBA_t * px;
        int i, n, lit = 0;
        if( ! HWR_Glow_Composite_Texture( texnum, &e->mip ) )
        {
            e->state = GLOW_NONE;
            return NULL;
        }
        px = (RGBA_t *) e->mip.GR_data;
        n = e->mip.width * e->mip.height;
        for( i = 0; i < n; i++ )
            lit |= glow_texel( e->kind, &px[i], px[i].s.alpha );
        if( ! lit )
        {
            // Named like a light, but nothing in it is bright enough.
            Z_Free( e->mip.GR_data );
            e->state = GLOW_NONE;
            return NULL;
        }
    }
    return e;
}

// ---- flats

static boolean glow_is_liquid( int kind )
{
    return kind == DOOM_EMISSIVE_SLIME || kind == DOOM_EMISSIVE_LAVA
        || kind == DOOM_EMISSIVE_MAGMA;
}


static glow_entry_t * glow_flat_entry( lumpnum_t lump )
{
    glow_entry_t * e;

    if( ! VALID_LUMP( lump ) )
        return NULL;
    e = glow_lookup( &glow_flat, GLOW_FLAT_SLOTS, lump );
    if( e == NULL )
        return NULL;

    if( e->state == GLOW_UNKNOWN )
    {
        char name[9];
        glow_lump_name( lump, name );
        e->kind = doom_emissive_kind( name, 1 );
        e->state = ( e->kind == DOOM_EMISSIVE_NONE )? GLOW_NONE : GLOW_HAS;
    }
    if( e->state != GLOW_HAS )
        return NULL;

    if( ! e->mip.downloaded && ! e->mip.GR_data )
    {
        // Sized as HWR_CacheFlat sizes it, so the texture coords agree.
        int size = W_LumpLength( lump );
        int side, i, n, lit = 0;
        uint32_t sum[3] = { 0, 0, 0 };
        byte * raw;
        RGBA_t * px;

        for( side = 2048; side > 32 && side * side > size; side >>= 1 ) ;
        if( side * side > size )
        {
            e->state = GLOW_NONE;
            return NULL;
        }
        n = side * side;
        raw = W_CacheLumpNum( lump, PU_STATIC );
        px = Z_Malloc( n * sizeof(RGBA_t), PU_STATIC, &e->mip.GR_data );
        for( i = 0; i < n; i++ )
        {
            byte s;
            RGBA_t c = V_GetColor( raw[i] );
            px[i] = c;
            s = glow_texel( e->kind, &px[i], 255 );
            lit |= s;
            sum[0] += px[i].s.red;  sum[1] += px[i].s.green;  sum[2] += px[i].s.blue;
            if( glow_is_liquid( e->kind ) )
            {
                // A liquid is drawn additively (HWR_Glow_Plane): its bright
                // cells at their own colour, the rest adding nothing.  Drawn
                // at full brightness instead, as a lamp is, nukage hardly
                // changed: its glowing greens are dark texels (DERP's test
                // lights the brightest third of NUKAGE1, at 19/255 on
                // average), and a lit pool already shows them near that.
                if( s )
                {
                    px[i] = c;
                    px[i].s.alpha = 255;
                }
                else
                    px[i].rgba = 0;
            }
        }
        Z_ChangeTag( raw, PU_CACHE );
        if( ! lit )
        {
            Z_Free( e->mip.GR_data );
            e->state = GLOW_NONE;
            return NULL;
        }
        {
            // The colour the pool throws: its glowing pixels' average, raised
            // so its brightest channel is full.
            uint32_t hi = sum[0];
            if( sum[1] > hi )  hi = sum[1];
            if( sum[2] > hi )  hi = sum[2];
            if( hi == 0 )  hi = 1;
            e->color.s.red   = (byte)( sum[0] * 255 / hi );
            e->color.s.green = (byte)( sum[1] * 255 / hi );
            e->color.s.blue  = (byte)( sum[2] * 255 / hi );
            e->color.s.alpha = 255;
        }
        e->mip.GR_format = GR_RGBA;
        e->mip.tfflags = TF_WRAPXY;
        e->mip.width = e->mip.height = side;
        Z_ChangeTag( px, PU_HWRCACHE );
    }
    return e;
}

// How much of a light's glowing pixels shows.  All of it, whatever the
// room's light, as GZDoom's brightmaps do: a lamp in a dark room is the
// point.  (An earlier version faded it with the sector's light, and that
// turned it off in exactly the rooms where it shows: the GL renderer draws a
// light-128 room near full brightness already.)
//
// Except in a sector whose light is animated -- blink, strobe, glow, flicker
// -- where the lamps follow the swing, as DERP's do: off at the bottom of it,
// full at the top.  The swing is learnt by watching: the lowest and highest
// light each such sector has been drawn at this level.  Until it has been
// seen to move, its lamps are on.

static byte glow_sector_scale( sector_t * sec )
{
    int i, l, lo, hi;

    if( sec == NULL )
        return 255;
    switch( sec->special & 31 )   // Boom keeps the Doom light type in the low bits
    {
     case 1: case 2: case 3: case 4: case 8: case 12: case 13: case 17:
        break;
     default:
        return 255;
    }
    if( glow_secbase != sectors || glow_numsec != numsectors )
    {
        free( glow_sec_lo );
        free( glow_sec_hi );
        glow_numsec = 0;
        glow_secbase = sectors;
        glow_sec_lo = malloc( numsectors );
        glow_sec_hi = malloc( numsectors );
        if( glow_sec_lo == NULL || glow_sec_hi == NULL || numsectors <= 0 )
            return 255;
        memset( glow_sec_lo, 255, numsectors );
        memset( glow_sec_hi, 0, numsectors );
        glow_numsec = numsectors;
    }
    i = sec - sectors;
    if( i < 0 || i >= glow_numsec )
        return 255;
    l = sec->lightlevel;
    if( l < 0 )    l = 0;
    if( l > 255 )  l = 255;
    if( l < glow_sec_lo[i] )  glow_sec_lo[i] = l;
    if( l > glow_sec_hi[i] )  glow_sec_hi[i] = l;
    lo = glow_sec_lo[i];
    hi = glow_sec_hi[i];
    if( hi - lo < 16 )
        return 255;
    return (byte)( (l - lo) * 255 / (hi - lo) );
}

// How strongly a pool adds its own bright cells over itself.
#define GLOW_LIQUID_ALPHA  150

static void glow_draw_additive( Mipmap_t * mip, vxtx3d_t * v, int n, byte alpha )
{
    FSurfaceInfo_t Surf;
    Surf.FlatColor.rgba = 0xffffffff;
    Surf.FlatColor.s.alpha = alpha;
    Surf.polyflags = 0;
    Surf.texflags = 0;
    HWD.pfnSetTexture( mip );
    HWD.pfnDrawPolygon( &Surf, v, n, PF_Additive | PF_Modulated | PF_Decal | PF_Clip );
}

static void glow_draw_mask( Mipmap_t * mip, vxtx3d_t * v, int n, byte scale )
{
    FSurfaceInfo_t Surf;
    // Premultiplied: out = mask * scale + dest * (1 - mask.a * scale).
    Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = scale;
    Surf.FlatColor.s.alpha = scale;
    Surf.polyflags = 0;
    Surf.texflags = 0;
    HWD.pfnSetTexture( mip );
    HWD.pfnDrawPolygon( &Surf, v, n, PF_Environment | PF_Modulated | PF_Decal | PF_Clip );
}

static void glow_rebind( void )
{
    if( hwr_glow_bound )
        HWD.pfnSetTexture( hwr_glow_bound );
}

// ---- the liquid's light up the walls

#define GLOW_RAMP_H      64
#define GLOW_WALL_HEIGHT 96.0f   // map units the glow climbs
#define GLOW_WALL_ALPHA  110     // its strength at the waterline

static void glow_set_ramp( void )
{
    if( ! glow_ramp.downloaded && ! glow_ramp.GR_data )
    {
        // Alpha-intensity: white, fading out as the square of the height.
        uint16_t * data = Z_Malloc( 8 * GLOW_RAMP_H * sizeof(uint16_t), PU_HWRCACHE,
                                    &glow_ramp.GR_data );
        int x, y;
        for( y = 0; y < GLOW_RAMP_H; y++ )
        {
            float f = 1.0f - (float) y / (GLOW_RAMP_H - 1);
            byte a = (byte)( 255.0f * f * f );
            for( x = 0; x < 8; x++ )
                data[ y * 8 + x ] = (a << 8) | 0xff;
        }
        glow_ramp.GR_format = GR_TEXFMT_ALPHA_INTENSITY_88;
        glow_ramp.width = 8;
        glow_ramp.height = GLOW_RAMP_H;
        glow_ramp.tfflags = 0;   // clamped: above the ramp it stays clear
    }
    HWD.pfnSetTexture( &glow_ramp );
}

static void glow_liquid_wall( vxtx3d_t * vxtx, byte lightlum )
{
    sector_t * sec = gr_frontsector;
    glow_entry_t * e;
    vxtx3d_t v[4];
    FSurfaceInfo_t Surf;
    float floor;
    int i;

    if( sec == NULL || sec->floorpic < 0 || (unsigned) sec->floorpic >= numlevelflats )
        return;
    e = glow_flat_entry( levelflats[ sec->floorpic ].lumpnum );
    if( e == NULL || ! glow_is_liquid( e->kind ) )
        return;

    floor = FIXED_TO_FLOAT( sec->floorheight );
    // Wall quads are 0,1 along the bottom and 2,3 along the top.
    if( vxtx[0].y > floor + GLOW_WALL_HEIGHT && vxtx[1].y > floor + GLOW_WALL_HEIGHT )
        return;

    for( i = 0; i < 4; i++ )
    {
        float t = ( vxtx[i].y - floor ) / GLOW_WALL_HEIGHT;
        v[i] = vxtx[i];
        v[i].sow = 0.5f;
        v[i].tow = ( t < 0.0f )? 0.0f : t;
    }

    Surf.FlatColor = e->color;
    // Strongest where the room is dark; a bright wall is already lit.
    Surf.FlatColor.s.alpha = (byte)( GLOW_WALL_ALPHA * (512 - lightlum) / 512 );
    Surf.polyflags = 0;
    Surf.texflags = 0;
    glow_set_ramp();
    HWD.pfnDrawPolygon( &Surf, v, 4, PF_Additive | PF_Modulated | PF_Decal | PF_Clip );
}

// ---- hooks

// An opaque or masked surface: not glass, fog sheets or translucent water.
// The blend is a 4-bit value under BLEND_FIELD, not bits, and PF_Additive
// shares its 0x80 with the PF_NoAlphaTest that every ordinary masked wall
// carries, so the value is what is tested.
static boolean glow_blend_opaque( int flags )
{
#ifdef BLEND_FIELD
    int blend = flags & PF_blend_field;
    return ( blend == PF_Overdraw || blend == PF_Masked );
#else
    return ! ( flags & PF_Blending & ~PF_Masked );
#endif
}

static byte glow_surf_lum( FSurfaceInfo_t * pSurf )
{
    byte l = pSurf->FlatColor.s.red;
    if( pSurf->FlatColor.s.green > l )  l = pSurf->FlatColor.s.green;
    if( pSurf->FlatColor.s.blue > l )   l = pSurf->FlatColor.s.blue;
    return l;
}

// Called from HWR_ProjectWall, after the wall itself.
void HWR_Glow_Wall( vxtx3d_t * vxtx, FSurfaceInfo_t * pSurf, int blendmode )
{
    boolean drew = false;
    byte lum;

    if( fixedcolormap )   // light amplification, invulnerability
        return;
    if( ! glow_blend_opaque( blendmode ) )
        return;
    if( pSurf->texflags & TF_Fogsheet )
        return;

    lum = glow_surf_lum( pSurf );

    if( cv_grglow_lights.EV )
    {
        glow_entry_t * e = glow_wall_mask( hwr_glow_texnum );
        if( e && ! glow_is_liquid( e->kind ) )
        {
            byte s = glow_sector_scale( gr_frontsector );
            if( s )
            {
                glow_draw_mask( &e->mip, vxtx, 4, s );
                drew = true;
            }
        }
        else if( e && cv_grglow_liquids.EV )
        {
            // A falling-slime wall glows whatever the room's light.
            glow_draw_mask( &e->mip, vxtx, 4, 255 );
            drew = true;
        }
    }
    else if( cv_grglow_liquids.EV )
    {
        glow_entry_t * e = glow_wall_mask( hwr_glow_texnum );
        if( e && glow_is_liquid( e->kind ) )
        {
            glow_draw_mask( &e->mip, vxtx, 4, 255 );
            drew = true;
        }
    }

    if( cv_grglow_liquids.EV )
    {
        glow_liquid_wall( vxtx, lum );
        drew = true;
    }

    if( drew )
        glow_rebind();
}

// Called from HWR_RenderPlane, after the plane itself.
void HWR_Glow_Plane( vxtx3d_t * v, int numverts, lumpnum_t flatlump,
                     FBITFIELD polyflags )
{
    glow_entry_t * e;
    byte s;

    if( fixedcolormap || ! glow_blend_opaque( polyflags ) )
        return;
    if( ! cv_grglow_lights.EV && ! cv_grglow_liquids.EV )
        return;
    e = glow_flat_entry( flatlump );
    if( e == NULL )
        return;
    if( glow_is_liquid( e->kind ) )
    {
        if( ! cv_grglow_liquids.EV )  return;
        // A pool glows whatever the room's light.
        glow_draw_additive( &e->mip, v, numverts, GLOW_LIQUID_ALPHA );
        glow_rebind();
        return;
    }
    else
    {
        if( ! cv_grglow_lights.EV )  return;
        s = glow_sector_scale( gr_frontsector );
        if( ! s )  return;
    }
    glow_draw_mask( &e->mip, v, numverts, s );
    glow_rebind();
}

// ---- eyes

// Decodes a sprite lump as DERP does, to hash it against the stock table:
// palette index and coverage pairs, row by row.
static byte * glow_decode_sprite( lumpnum_t lump, int * pw, int * ph )
{
    int size = W_LumpLength( lump );
    byte * data, * pixels = NULL;
    int w, h, x;

    if( size < 8 )  return NULL;
    data = W_CacheLumpNum( lump, PU_STATIC );
    w = (int16_t) LE_SWAP16( ((uint16_t*)data)[0] );
    h = (int16_t) LE_SWAP16( ((uint16_t*)data)[1] );
    if( w < 1 || h < 1 || w > 1024 || h > 1024 || 8 + w * 4 > size )
        goto done;
    pixels = calloc( (size_t) w * h * 2, 1 );
    if( pixels == NULL )
        goto done;
    for( x = 0; x < w; x++ )
    {
        uint32_t pos = LE_SWAP32( ((uint32_t*)(data + 8))[x] );
        int previous = -1;
        while( pos < (uint32_t) size && data[pos] != 255 )
        {
            int top = data[pos], len, y;
            if( pos + 4 > (uint32_t) size || pos + 4 + data[pos+1] > (uint32_t) size )
            {
                free( pixels );
                pixels = NULL;
                goto done;
            }
            if( top <= previous )  top += previous;   // tall patches
            previous = top;
            len = data[pos+1];
            for( y = 0; y < len; y++ )
            {
                if( top + y >= 0 && top + y < h )
                {
                    size_t p = ((size_t)(top + y) * w + x) * 2;
                    pixels[p] = data[pos + 3 + y];
                    pixels[p+1] = 255;
                }
            }
            pos += len + 4;
        }
    }
    *pw = w;
    *ph = h;
done:
    Z_ChangeTag( data, PU_CACHE );
    return pixels;
}

// Builds a patch holding only the eye pixels, in the sprite's own size and
// offsets, so HWR_MakePatch lays it out exactly as it lays out the sprite.
static patch_t * glow_eye_patch( const doom_stock_eye_entry * te, const byte * pixels,
                                 int w, int h )
{
    // At most one post of one pixel per eye pixel, each 5 bytes, plus the
    // column ends.
    int neyes = 0, x, y;
    const char * p = te->pixels;
    byte * eyes, * out, * cp;
    patch_t * patch;
    size_t bytes;

    eyes = calloc( (size_t) w * h, 1 );
    if( eyes == NULL )  return NULL;
    while( doom_stock_eye_next( &p, &x, &y ) )
    {
        if( x >= 0 && y >= 0 && x < w && y < h && y < 254 && pixels[ ((size_t)y * w + x) * 2 + 1 ] )
        {
            eyes[ (size_t)y * w + x ] = 1;
            neyes++;
        }
    }
    if( neyes == 0 )
    {
        free( eyes );
        return NULL;
    }
    bytes = 8 + (size_t) w * 4 + (size_t) neyes * 5 + w;
    out = calloc( bytes, 1 );
    if( out == NULL )
    {
        free( eyes );
        return NULL;
    }
    patch = (patch_t *) out;
    patch->width = w;
    patch->height = h;
    patch->leftoffset = 0;
    patch->topoffset = 0;
    cp = out + 8 + w * 4;
    for( x = 0; x < w; x++ )
    {
        patch->columnofs[x] = (uint32_t)( cp - out );   // host order, as cached
        for( y = 0; y < h; y++ )
        {
            if( ! eyes[ (size_t)y * w + x ] )  continue;
            *cp++ = (byte) y;   // topdelta
            *cp++ = 1;          // length
            *cp++ = 0;          // pad
            *cp++ = pixels[ ((size_t)y * w + x) * 2 ];
            *cp++ = 0;          // pad
        }
        *cp++ = 255;
    }
    free( eyes );
    return patch;
}

static glow_entry_t * glow_eye_mask( lumpnum_t lump, Mipmap_t * smip )
{
    glow_entry_t * e = glow_lookup( &glow_sprite, GLOW_SPRITE_SLOTS, lump );
    if( e == NULL )
        return NULL;

    if( e->state == GLOW_UNKNOWN )
    {
        // Only the lumps in the stock table can have eyes; decide from the
        // name before decoding anything.
        char name[9];
        size_t k, n = sizeof(doom_stock_eye_table) / sizeof(doom_stock_eye_table[0]);
        glow_lump_name( lump, name );
        e->state = GLOW_NONE;
        for( k = 0; k < n; k++ )
        {
            if( strcmp( doom_stock_eye_table[k].name, name ) == 0 )
            {
                if( doom_stock_eye_table[k].pixels[0] )
                    e->state = GLOW_HAS;   // checked against the art below
                break;
            }
        }
    }
    if( e->state != GLOW_HAS )
        return NULL;

    if( ! e->mip.downloaded && ! e->mip.GR_data )
    {
        int w, h;
        char name[9];
        const doom_stock_eye_entry * te;
        byte * pixels = glow_decode_sprite( lump, &w, &h );
        patch_t * patch;
        MipPatch_t dummy;

        glow_lump_name( lump, name );
        te = pixels ? doom_stock_eyes( name, w, h, pixels ) : NULL;
        patch = te ? glow_eye_patch( te, pixels, w, h ) : NULL;
        free( pixels );
        if( patch == NULL )
        {
            // Replaced artwork (the hash differs), or no eye in view.
            e->state = GLOW_NONE;
            return NULL;
        }
        // The same copy flags as the sprite's, so the same margin.
        memset( &dummy, 0, sizeof(dummy) );
        dummy.patch_lumpnum = lump;
        e->mip.width = 0;
        HWR_MakePatch( patch, &dummy, &e->mip,
                       smip->tfflags & (TF_MarginCopies|TF_Opaquetrans) );
        free( patch );
        // Eyes are only ever drawn over this sprite copy; if the margin came
        // out different the coords would not agree, so do not draw them.
        if( (e->mip.tfflags & TF_SpriteMargin) != (smip->tfflags & TF_SpriteMargin) )
        {
            Z_Free( e->mip.GR_data );
            e->state = GLOW_NONE;
            return NULL;
        }
    }
    return e;
}

// Called from HWR_DrawSprite, after an ordinary opaque sprite is drawn.
void HWR_Glow_Sprite( vxtx3d_t * vxtx, lumpnum_t patch_lumpnum,
                      Mipmap_t * smip, uint32_t mobjflags, int health )
{
    glow_entry_t * e;

    if( ! cv_grglow_eyes.EV || fixedcolormap )
        return;
    if( health <= 0 || (mobjflags & (MF_CORPSE|MF_SHADOW)) || ! (mobjflags & MF_COUNTKILL) )
        return;
    e = glow_eye_mask( patch_lumpnum, smip );
    if( e == NULL )
        return;
    glow_draw_mask( &e->mip, vxtx, 4, 255 );
}
