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
consvar_t cv_grglow_lamplight = { "gr_glow_lamplight", "Off", CV_SAVE, CV_OnOff };
consvar_t cv_grglow_ripples = { "gr_glow_ripples", "Off", CV_SAVE, CV_OnOff };

extern sector_t * gr_frontsector;   // hw_main.c, the sector being drawn
extern seg_t    * gr_curline;       // hw_main.c, the wall being drawn

Mipmap_t * hwr_glow_bound = NULL;
int        hwr_glow_texnum = -1;

void HWR_Glow_Register( void )
{
    CV_RegisterVar( &cv_grglow_lights );
    CV_RegisterVar( &cv_grglow_liquids );
    CV_RegisterVar( &cv_grglow_eyes );
    CV_RegisterVar( &cv_grglow_lamplight );
    CV_RegisterVar( &cv_grglow_ripples );
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
#define GLOW_RIPPLE_SLOTS  256
static glow_entry_t * glow_ripple = NULL;   // Liquid Light Ripples, by flat lump

// The gradient the liquids throw up the walls.
static Mipmap_t glow_ramp;

// The light swing of each sector with an animated light (glow_sector_scale).
static byte * glow_sec_lo = NULL;
static byte * glow_sec_hi = NULL;
static int    glow_numsec = 0;
static sector_t * glow_secbase = NULL;

static void glow_lamps_free( void );  // the lamp light, below

// Tables are only ever freed here, after HWR_Free_TextureCache has had the
// driver drop every texture and Z_FreeTags has cleared every GR_data owner,
// both of which write into them.
void HWR_Glow_Free( void )
{
    free( glow_tex );     glow_tex = NULL;  glow_numtex = 0;
    free( glow_flat );    glow_flat = NULL;
    free( glow_sprite );  glow_sprite = NULL;
    free( glow_ripple );  glow_ripple = NULL;
    free( glow_sec_lo );  glow_sec_lo = NULL;
    free( glow_sec_hi );  glow_sec_hi = NULL;
    glow_numsec = 0;
    glow_secbase = NULL;
    glow_lamps_free();
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

// DERP's list of glowing artwork, plus what the cabinet adds to it: the
// wall switches, whose lit indicators read as panel lights (SW1 is the
// switch in one state, SW2 in the other; both carry lights).  emissive.h is
// kept verbatim, so additions go here.
static int glow_kind( const char * name, int flat )
{
    int kind = doom_emissive_kind( name, flat );
    if( kind == DOOM_EMISSIVE_NONE && ! flat
        && ( strncmp( name, "SW1", 3 ) == 0 || strncmp( name, "SW2", 3 ) == 0 ) )
        kind = DOOM_EMISSIVE_PANEL;
    return kind;
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

static int glow_switch_mask_group( int texnum );   // the lamp light, below
static int glow_rgb_group( int r, int g, int b );

static glow_entry_t * glow_wall_mask( int texnum )
{
    glow_entry_t * e;
    int swgroup;

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
        e->kind = glow_kind( name, 0 );
        // The skies are composited by a special case; never a light.
        e->state = ( e->kind == DOOM_EMISSIVE_NONE
                     || strncmp( name, "SKY", 3 ) == 0 )? GLOW_NONE : GLOW_HAS;
        if( e->state == GLOW_NONE )
            return NULL;
    }
    if( e->state != GLOW_HAS )
        return NULL;
    // A switch that is off does not glow (its light follows it on); one
    // that is on glows only in its lit indicator.
    swgroup = glow_switch_mask_group( texnum );
    if( swgroup < 0 )
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
        {
            if( swgroup > 0
                && glow_rgb_group( px[i].s.red, px[i].s.green, px[i].s.blue ) != swgroup )
            {
                px[i].rgba = 0;
                continue;
            }
            lit |= glow_texel( e->kind, &px[i], px[i].s.alpha );
        }
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
        e->kind = glow_kind( name, 1 );
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
    HWD.pfnDrawPolygon( &Surf, v, n, PF_Additive | PF_Modulated | PF_Decal | PF_Clip | PF_NoTint );
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
    HWD.pfnDrawPolygon( &Surf, v, n, PF_Environment | PF_Modulated | PF_Decal | PF_Clip | PF_NoTint );
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

// The glow from one liquid plane, up from a floor (down = 0) or down from a
// ceiling (down = 1).
static void glow_liquid_band( vxtx3d_t * vxtx, byte lightlum, short pic, fixed_t height,
                              int down )
{
    glow_entry_t * e;
    vxtx3d_t v[4];
    FSurfaceInfo_t Surf;
    float plane;
    int i;

    if( pic < 0 || (unsigned) pic >= numlevelflats )
        return;
    e = glow_flat_entry( levelflats[ pic ].lumpnum );
    if( e == NULL || ! glow_is_liquid( e->kind ) )
        return;

    plane = FIXED_TO_FLOAT( height );
    // Wall quads are 0,1 along the bottom and 2,3 along the top.
    if( ! down && vxtx[0].y > plane + GLOW_WALL_HEIGHT && vxtx[1].y > plane + GLOW_WALL_HEIGHT )
        return;
    if( down && vxtx[2].y < plane - GLOW_WALL_HEIGHT && vxtx[3].y < plane - GLOW_WALL_HEIGHT )
        return;

    for( i = 0; i < 4; i++ )
    {
        float t = ( down ? plane - vxtx[i].y : vxtx[i].y - plane ) / GLOW_WALL_HEIGHT;
        v[i] = vxtx[i];
        v[i].sow = 0.5f;
        // Not clamped here: the texture clamps.  A corner past the plane
        // (E1M8 line 107: a lower wall running 32 units above the nukage
        // ceiling) clamped to 0 stretched the whole fade over the full wall,
        // since the coordinate is interpolated between corners, and the
        // band all but vanished.  The part past the plane is hidden by it.
        v[i].tow = t;
    }

    Surf.FlatColor = e->color;
    // Strongest where the room is dark; a bright wall is already lit.
    Surf.FlatColor.s.alpha = (byte)( GLOW_WALL_ALPHA * (512 - lightlum) / 512 );
    Surf.polyflags = 0;
    Surf.texflags = 0;
    glow_set_ramp();
    HWD.pfnDrawPolygon( &Surf, v, 4, PF_Additive | PF_Modulated | PF_Decal | PF_Clip | PF_NoTint );
}

static void glow_liquid_wall( vxtx3d_t * vxtx, byte lightlum )
{
    sector_t * sec = gr_frontsector;
    if( sec == NULL )
        return;
    glow_liquid_band( vxtx, lightlum, sec->floorpic, sec->floorheight, 0 );
    // Nukage on a ceiling glows down the walls too.
    glow_liquid_band( vxtx, lightlum, sec->ceilingpic, sec->ceilingheight, 1 );
}

// ---- Liquid Light Ripples
//
// The wavy light a pool throws on the walls standing in it (DERP's liquid
// caustics, much simplified).  Per liquid flat lump, a 64x64 pattern made
// from that flat's own brightness ripples -- softened over 3x3 so lone
// bright texels drop out, minus the flat's mean, scaled so its strongest
// streak is full -- faded with height, one texel to the map unit.  Each
// animation frame of the flat is its own lump, so the ripples animate with
// the floor; a slow drift along the wall is added.  Added in the liquid's
// colour, one pass, with the fade baked into the texture since a pass takes
// only one texture.  Water ripples too: it does not glow, but light off it
// moves.

#define RIPPLE_HEIGHT  64.0f   // map units above the waterline the ripples reach
#define RIPPLE_ALPHA   180     // their strength, of 255
#define RIPPLE_DRIFT   0.0025f // along the wall, in pattern widths per tic

static boolean glow_is_water_name( const char * name )
{
    return strncmp( name, "FWATER", 6 ) == 0 || strncmp( name, "SWATER", 6 ) == 0;
}

static glow_entry_t * glow_ripple_entry( lumpnum_t lump )
{
    glow_entry_t * e;

    if( ! VALID_LUMP( lump ) )
        return NULL;
    e = glow_lookup( &glow_ripple, GLOW_RIPPLE_SLOTS, lump );
    if( e == NULL )
        return NULL;
    if( e->state == GLOW_UNKNOWN )
    {
        char name[9];
        int kind;
        glow_lump_name( lump, name );
        kind = glow_kind( name, 1 );
        e->kind = kind;
        e->state = ( glow_is_liquid( kind ) || glow_is_water_name( name ) )? GLOW_HAS : GLOW_NONE;
        if( e->state == GLOW_NONE )
            return NULL;
    }
    if( e->state != GLOW_HAS )
        return NULL;

    if( ! e->mip.downloaded && ! e->mip.GR_data )
    {
        static float bright[64*64], soft[64*64];
        byte * raw;
        uint16_t * data;
        float mean = 0.0f, top = 0.0f;
        uint32_t sum[3] = { 0, 0, 0 };
        int x, y, i;

        if( W_LumpLength( lump ) < 4096 )
        {
            e->state = GLOW_NONE;
            return NULL;
        }
        raw = W_CacheLumpNum( lump, PU_STATIC );
        for( i = 0; i < 4096; i++ )
        {
            RGBA_t c = V_GetColor( raw[i] );
            int hi = c.s.red;
            if( c.s.green > hi )  hi = c.s.green;
            if( c.s.blue > hi )   hi = c.s.blue;
            bright[i] = hi;
            sum[0] += c.s.red;  sum[1] += c.s.green;  sum[2] += c.s.blue;
        }
        Z_ChangeTag( raw, PU_CACHE );
        for( y = 0; y < 64; y++ )
        {
            for( x = 0; x < 64; x++ )
            {
                float t = 0.0f;
                int dx, dy;
                for( dy = -1; dy <= 1; dy++ )
                    for( dx = -1; dx <= 1; dx++ )
                        t += bright[ ((y + dy) & 63) * 64 + ((x + dx) & 63) ];
                soft[ y * 64 + x ] = t / 9.0f;
                mean += t / 9.0f;
            }
        }
        mean /= 4096.0f;
        for( i = 0; i < 4096; i++ )
        {
            soft[i] -= mean;
            if( soft[i] > top )  top = soft[i];
        }
        if( top < 1.0f )
        {
            e->state = GLOW_NONE;   // a flat with no ripples to show
            return NULL;
        }
        data = Z_Malloc( 64 * 64 * sizeof(uint16_t), PU_STATIC, &e->mip.GR_data );
        for( y = 0; y < 64; y++ )
        {
            // Row 0 is the waterline; the light fades out by the top row.
            float f = 1.0f - (float) y / 63.0f;
            f = f * f;
            for( x = 0; x < 64; x++ )
            {
                float v = soft[ y * 64 + x ] / top;
                byte a;
                if( v < 0.0f )  v = 0.0f;
                // The square root lifts the many faint streaks toward the
                // one brightest, or only that one showed.
                a = (byte)( 255.0f * sqrt( v ) * f );
                data[ y * 64 + x ] = (a << 8) | a;
            }
        }
        {
            // The colour: a glowing liquid's own glow, water its average.
            glow_entry_t * g = glow_is_liquid( e->kind )? glow_flat_entry( lump ) : NULL;
            if( g )
                e->color = g->color;
            else
            {
                uint32_t hi = sum[0];
                if( sum[1] > hi )  hi = sum[1];
                if( sum[2] > hi )  hi = sum[2];
                if( hi == 0 )  hi = 1;
                e->color.s.red   = (byte)( (uint64_t) sum[0] * 255 / hi );
                e->color.s.green = (byte)( (uint64_t) sum[1] * 255 / hi );
                e->color.s.blue  = (byte)( (uint64_t) sum[2] * 255 / hi );
                e->color.s.alpha = 255;
            }
        }
        e->mip.GR_format = GR_TEXFMT_ALPHA_INTENSITY_88;
        e->mip.width = e->mip.height = 64;
        e->mip.tfflags = TF_WRAPX;   // repeats along the wall, not up it
        Z_ChangeTag( data, PU_HWRCACHE );
    }
    return e;
}

// The ripples from one liquid plane: up a wall from a liquid floor
// (down = 0), or down it from a liquid ceiling (down = 1), as the liquid's
// glow does (glow_liquid_band).  Returns whether drawn.
static boolean glow_ripple_band( vxtx3d_t * vxtx, short pic, fixed_t height, int down )
{
    glow_entry_t * e;
    FSurfaceInfo_t Surf;
    vxtx3d_t v[4];
    float plane, wx, wz, len, ux, uz, drift;
    int i;

    if( pic < 0 || (unsigned) pic >= numlevelflats )
        return false;
    e = glow_ripple_entry( levelflats[ pic ].lumpnum );
    if( e == NULL )
        return false;
    plane = FIXED_TO_FLOAT( height );
    // Wall quads are 0,1 along the bottom and 2,3 along the top.
    if( ! down && vxtx[0].y > plane + RIPPLE_HEIGHT && vxtx[1].y > plane + RIPPLE_HEIGHT )
        return false;
    if( down && vxtx[2].y < plane - RIPPLE_HEIGHT && vxtx[3].y < plane - RIPPLE_HEIGHT )
        return false;
    wx = vxtx[1].x - vxtx[0].x;
    wz = vxtx[1].z - vxtx[0].z;
    len = sqrt( wx*wx + wz*wz );
    if( len < 1.0f )
        return false;
    ux = wx / len;  uz = wz / len;
    drift = leveltime * RIPPLE_DRIFT;
    for( i = 0; i < 4; i++ )
    {
        // Along the wall in map units from the origin, so the pattern runs
        // on unbroken from one wall piece to the next in line.  Up the
        // texture is away from the liquid, whichever way that is.
        float a = vxtx[i].x * ux + vxtx[i].z * uz;
        float t = ( down ? plane - vxtx[i].y : vxtx[i].y - plane ) / RIPPLE_HEIGHT;
        v[i] = vxtx[i];
        v[i].sow = a / 64.0f + drift;
        // Not clamped here: the texture clamps.  A corner past the plane
        // (E1M8 line 107: a lower wall running 32 units above the nukage
        // ceiling) clamped to 0 stretched the whole fade over the full wall,
        // since the coordinate is interpolated between corners, and the
        // band all but vanished.  The part past the plane is hidden by it.
        v[i].tow = t;
    }
    Surf.FlatColor = e->color;
    Surf.FlatColor.s.alpha = RIPPLE_ALPHA;
    Surf.polyflags = 0;
    Surf.texflags = 0;
    HWD.pfnSetTexture( &e->mip );
    HWD.pfnDrawPolygon( &Surf, v, 4, PF_Additive | PF_Modulated | PF_Decal | PF_Clip | PF_NoTint );
    return true;
}

// The ripples on a wall in a sector with a liquid floor or ceiling (E1M8's
// nukage ceiling had none: only floors were looked at).
static boolean glow_ripple_wall( vxtx3d_t * vxtx )
{
    sector_t * sec = gr_frontsector;
    boolean drew;
    if( sec == NULL )
        return false;
    drew = glow_ripple_band( vxtx, sec->floorpic, sec->floorheight, 0 );
    if( glow_ripple_band( vxtx, sec->ceilingpic, sec->ceilingheight, 1 ) )
        drew = true;
    return drew;
}

// ---- lamp light
//
// Lamps that light the room round them, in their own colour: the part of
// DERP's lighting a fixed-function renderer can do.  At level load every
// wall whose texture emissive.h calls a light (strips, panels, exit signs,
// lit doors) gets a light source every LAMP_SPACING units along it, a hand's
// width out from the wall, and every ceiling with a lamp flat gets them on a
// grid.  Each takes its colour from its own art's glowing pixels, and a panel
// dotted with a few LEDs gives little light (DERP's rule).
//
// The light is a disc projected onto each wall and floor near it, drawn with
// PF_LightMul: dest * (1 + colour).  That tints and brightens what is
// already drawn, keeping its detail, where an additive light (the stock
// dynamic lights) washes a flat colour over it.  The other side of the
// trade: it cannot light what is black, so a pitch-dark room stays dark.
//
// Light reaches the lamp's own sector and the sectors joined to it through
// an opening that is open *now* (a closed door stops it), never further.
// There is no other occlusion, so the reach is kept short.  It follows a
// blinking sector's swing (glow_sector_scale), as the lamps' own glow does.

#define LAMP_SPACING      128.0f   // map units between lights along a wall
#define LAMP_OUT          12.0f    // out from the wall face
#define LAMP_RADIUS_MIN   128.0f   // reach of the dimmest
#define LAMP_RADIUS_MAX   224.0f   // and the brightest
#define LAMP_CEIL_DROP    8.0f     // a ceiling lamp hangs this far down
#define LAMP_STRENGTH     255      // colour scale at full strength, of 255
// The additive half of the light, of 255: what lets a lamp colour a dark or
// grey surface, which multiplying alone can at most double.  The cabinet
// found multiply-only "very subtle".
#define LAMP_ADD          56
// How much harder sprites and the weapon take the light than a wall's
// multiply does (HWR_Glow_Tint), in 1/255: they have no additive pass.
#define LAMP_SPRITE_GAIN  2
#define LAMP_MAX          4096
#define LAMP_CEIL_MAX     24       // lights per lamp-ceilinged sector

typedef struct {
    float      x, y, z;     // map x, map y, height
    float      radius;
    RGBA_t     color;       // already scaled by the lamp's strength
    sector_t * sector;
    byte       ceiling;     // hangs from the ceiling
    // A wall light follows its texture (glow_lamp_follow): a switch thrown
    // from SW1 to SW2 changes the indicator's colour, and a texture swapped
    // for an unlit one puts the light out.
    side_t   * side;        // NULL for a ceiling light
    line_t   * line;        // the wall it is on
    float      share;       // of its strength, among lamps crowding it (glow_lamp_shares)
    byte       part;        // GLOW_PART_*
    short      texnum;      // the texture its colour was taken from
} glow_lamp_t;

enum { GLOW_PART_TOP = 1, GLOW_PART_MID, GLOW_PART_BOTTOM };

typedef struct {
    int        sec;         // the sector it lights
    int        lamp;
    line_t   * via;         // NULL in the lamp's own sector, else the opening
} glow_lamp_ref_t;

static glow_lamp_t     * glow_lamps = NULL;
static int               glow_numlamps = 0;
static glow_lamp_ref_t * glow_lamp_refs = NULL;
static int             * glow_sec_first = NULL;   // [numsectors+1], into refs
static sector_t        * glow_lampbase = NULL;
static int               glow_lamp_numsec = 0;

typedef struct {
    byte    state;     // glow_state_e
    byte    strength;  // 0..255
    RGBA_t  color;     // brightest channel 255
    byte    sw;        // GLOW_SW_*: the switch rule, once worked out
    byte    swgroup;   // a lit switch's indicator colour group, GLOW_GRP_*
} glow_lampcolor_t;

// Switches.  A switch's light is the indicator that lights up when it
// changes state: of its red, green and blue pixels, the group that is
// brighter than in its other state.  So an ordinary switch glows only in the
// state where something lights (on), and a red/green switch -- SW1BRCOM's
// red indicator dims and a green one lights in SW2BRCOM -- glows red one way
// and green the other.  Averaging every lit pixel instead made SW2BRCOM
// orange: its dimmed red still passes DERP's test.
enum { GLOW_SW_UNKNOWN = 0, GLOW_SW_NOT, GLOW_SW_LIT, GLOW_SW_DARK };
static glow_lampcolor_t * glow_texcolor = NULL;   // by texture number

static Mipmap_t glow_disc;

static void glow_lamps_free( void )
{
    free( glow_lamps );      glow_lamps = NULL;  glow_numlamps = 0;
    free( glow_lamp_refs );  glow_lamp_refs = NULL;
    free( glow_sec_first );  glow_sec_first = NULL;
    free( glow_texcolor );   glow_texcolor = NULL;
    glow_lampbase = NULL;
    glow_lamp_numsec = 0;
}

static boolean glow_casts_light( int kind )
{
    return kind == DOOM_EMISSIVE_LAMP || kind == DOOM_EMISSIVE_PANEL
        || kind == DOOM_EMISSIVE_DIFFUSE || kind == DOOM_EMISSIVE_GLASS;
}

typedef struct {
    uint32_t  esum, sum[3];   // DERP's glowing pixels, weighted
    uint32_t  bsum[3];        // bright pixels, the fallback
    int       lit, bright, covered;
} glow_cstat_t;

static void glow_cstat_add( glow_cstat_t * st, int kind, const byte * pairs, int n )
{
    int i;
    for( i = 0; i < n; i++ )
    {
        RGBA_t c;
        unsigned char rgb[3];
        byte e, hi;
        if( ! pairs[ i*2 + 1 ] )  continue;
        st->covered++;
        c = V_GetColor( pairs[ i*2 ] );
        rgb[0] = c.s.red;  rgb[1] = c.s.green;  rgb[2] = c.s.blue;
        e = doom_emissive_pixel( kind, rgb, 255 );
        if( e )
        {
            st->lit++;
            st->esum += e;
            st->sum[0] += rgb[0] * e;  st->sum[1] += rgb[1] * e;  st->sum[2] += rgb[2] * e;
        }
        hi = rgb[0];
        if( rgb[1] > hi )  hi = rgb[1];
        if( rgb[2] > hi )  hi = rgb[2];
        if( hi >= 176 )
        {
            st->bright++;
            st->bsum[0] += rgb[0];  st->bsum[1] += rgb[1];  st->bsum[2] += rgb[2];
        }
    }
}

// The light's colour, brightest channel full, and its strength.  From
// DERP's glowing pixels when there are any; a neutral strip or ceiling lamp
// that DERP's test passes over (it wants a bright *and* a coloured or very
// pale pixel, and TLITE6_4 has none) is taken from its bright pixels.
static boolean glow_cstat_finish( glow_cstat_t * st, int kind, int minstrength,
                                  glow_lampcolor_t * out )
{
    uint32_t r, g, b, hi;
    int frac;
    if( st->covered == 0 )
        return false;
    if( st->esum )
    {
        r = st->sum[0];  g = st->sum[1];  b = st->sum[2];
        frac = st->lit * 255 * 3 / st->covered;   // a few LEDs light a little
    }
    else if( st->bright && kind != DOOM_EMISSIVE_PANEL )
    {
        r = st->bsum[0];  g = st->bsum[1];  b = st->bsum[2];
        frac = st->bright * 255 * 4 / st->covered;
    }
    else
        return false;
    hi = r;
    if( g > hi )  hi = g;
    if( b > hi )  hi = b;
    if( hi == 0 )
        return false;
    // In 64 bits: the sums are of colour * strength over every pixel, up to
    // 4096 * 255 * 255, and times 255 that wraps 32 bits.  It did, on any
    // art with many bright pixels -- the red ceiling light came out brown
    // and LITE3 at 1,1,1 -- while the exit sign, with fewer, was right.
    out->color.s.red   = (byte)( (uint64_t) r * 255 / hi );
    out->color.s.green = (byte)( (uint64_t) g * 255 / hi );
    out->color.s.blue  = (byte)( (uint64_t) b * 255 / hi );
    out->color.s.alpha = 255;
    // DERP's rule that a few LEDs light a little is for panels.  A lamp is
    // a lamp: TLITE6_5, the red ceiling light, is a fixture whose bulb is a
    // tenth of the flat, and by its share of lit pixels it came out at a
    // quarter strength and lit nothing anyone could see.
    if( kind == DOOM_EMISSIVE_LAMP && frac < 220 )  frac = 220;
    if( frac < minstrength )  frac = minstrength;
    if( frac < 48 )   frac = 48;
    if( frac > 255 )  frac = 255;
    if( kind == DOOM_EMISSIVE_DIFFUSE && frac > 128 )  frac = 128;
    out->strength = (byte) frac;
    return true;
}

static byte * glow_decode_sprite( lumpnum_t lump, int * pw, int * ph );

static glow_lampcolor_t * glow_texture_light_raw( int texnum )
{
    glow_lampcolor_t * lc;
    if( texnum <= 0 || texnum >= numtextures )
        return NULL;
    if( glow_texcolor == NULL )
    {
        glow_texcolor = calloc( numtextures, sizeof(glow_lampcolor_t) );
        if( glow_texcolor == NULL )  return NULL;
    }
    lc = &glow_texcolor[texnum];
    if( lc->state == GLOW_UNKNOWN )
    {
        char name[9];
        int i, kind;
        glow_cstat_t st;
        memcpy( name, textures[texnum]->name, 8 );
        name[8] = 0;
        for( i = 0; i < 8; i++ )
            name[i] = toupper( (unsigned char) name[i] );
        kind = glow_kind( name, 0 );
        lc->state = GLOW_NONE;
        if( ! glow_casts_light( kind ) )
            return NULL;
        memset( &st, 0, sizeof(st) );
        for( i = 0; i < textures[texnum]->patchcount; i++ )
        {
            int w, h;
            byte * px = glow_decode_sprite( textures[texnum]->patches[i].lumpnum, &w, &h );
            if( px )
            {
                glow_cstat_add( &st, kind, px, w * h );
                free( px );
            }
        }
        // A switch's indicator is a few pixels, but it is the point of the
        // switch: give it enough light to see (DERP has no switches).
        if( glow_cstat_finish( &st, kind,
                               ( strncmp( name, "SW1", 3 ) == 0 || strncmp( name, "SW2", 3 ) == 0 )? 170 : 0,
                               lc ) )
            lc->state = GLOW_HAS;
    }
    return ( lc->state == GLOW_HAS )? lc : NULL;
}

static boolean glow_flat_light( lumpnum_t lump, glow_lampcolor_t * out )
{
    char name[9];
    int kind, size, n, i;
    byte * raw, * pairs;
    glow_cstat_t st;

    if( ! VALID_LUMP( lump ) )
        return false;
    glow_lump_name( lump, name );
    kind = glow_kind( name, 1 );
    if( ! glow_casts_light( kind ) )
        return false;
    size = W_LumpLength( lump );
    n = ( size >= 4096 )? 4096 : size;
    pairs = malloc( n * 2 );
    if( pairs == NULL )
        return false;
    raw = W_CacheLumpNum( lump, PU_STATIC );
    for( i = 0; i < n; i++ )
    {
        pairs[ i*2 ] = raw[i];
        pairs[ i*2 + 1 ] = 255;
    }
    Z_ChangeTag( raw, PU_CACHE );
    memset( &st, 0, sizeof(st) );
    glow_cstat_add( &st, kind, pairs, n );
    free( pairs );
    return glow_cstat_finish( &st, kind, 0, out );
}

enum { GLOW_GRP_NONE = 0, GLOW_GRP_RED, GLOW_GRP_GREEN, GLOW_GRP_BLUE, GLOW_NUMGRP };

static int glow_rgb_group( int r, int g, int b )
{
    if( r >= 100 && r > 2 * g && r > 2 * b )            return GLOW_GRP_RED;
    if( g >= 100 && g * 10 > r * 14 && g * 10 > b * 12 )  return GLOW_GRP_GREEN;
    if( b >= 100 && b * 10 > r * 14 && b * 10 > g * 12 )  return GLOW_GRP_BLUE;
    return GLOW_GRP_NONE;
}

typedef struct {
    uint32_t  energy[GLOW_NUMGRP];     // sum of brightest channels
    uint32_t  sum[GLOW_NUMGRP][3];
    uint32_t  n[GLOW_NUMGRP];
} glow_swstat_t;

static void glow_switch_stat( int texnum, glow_swstat_t * st )
{
    int i, k;
    memset( st, 0, sizeof(*st) );
    if( texnum <= 0 || texnum >= numtextures )
        return;
    for( i = 0; i < textures[texnum]->patchcount; i++ )
    {
        int w, h;
        byte * px = glow_decode_sprite( textures[texnum]->patches[i].lumpnum, &w, &h );
        if( px == NULL )
            continue;
        for( k = 0; k < w * h; k++ )
        {
            RGBA_t c;
            int g, hi;
            if( ! px[ k*2 + 1 ] )  continue;
            c = V_GetColor( px[ k*2 ] );
            g = glow_rgb_group( c.s.red, c.s.green, c.s.blue );
            if( ! g )  continue;
            hi = c.s.red;
            if( c.s.green > hi )  hi = c.s.green;
            if( c.s.blue > hi )   hi = c.s.blue;
            st->n[g]++;
            st->energy[g] += hi;
            st->sum[g][0] += c.s.red;  st->sum[g][1] += c.s.green;  st->sum[g][2] += c.s.blue;
        }
        free( px );
    }
}

static boolean glow_is_switch_name( const char * name )
{
    return strncasecmp( name, "SW1", 3 ) == 0 || strncasecmp( name, "SW2", 3 ) == 0;
}

// The texture of the switch's other state, or -1.
static int glow_switch_partner( int texnum )
{
    char name[9];
    int t;
    // 0 is no texture (a '-' on a wall), and textures[0] is not one: the
    // attract cycle segfaulted here on E3M1's first frame.
    if( texnum <= 0 || texnum >= numtextures )
        return -1;
    memcpy( name, textures[texnum]->name, 8 );
    name[8] = 0;
    if( ! glow_is_switch_name( name ) )
        return -1;
    name[2] = ( name[2] == '1' )? '2' : '1';
    t = R_CheckTextureNumForName( name );
    return ( t > 0 )? t : -1;
}

// A texture's light, the switch rule applied: NULL for a switch that is off.
static glow_lampcolor_t * glow_texture_light( int texnum )
{
    glow_lampcolor_t * lc = glow_texture_light_raw( texnum );
    if( lc == NULL )
        return NULL;
    if( lc->sw == GLOW_SW_UNKNOWN )
    {
        char name[9];
        int partner = glow_switch_partner( texnum );
        memcpy( name, textures[texnum]->name, 8 );
        name[8] = 0;
        if( ! glow_is_switch_name( name ) || partner < 0 )
            lc->sw = GLOW_SW_NOT;          // a switch with no other state lights as a panel
        else
        {
            glow_swstat_t a, b;
            int g, best = GLOW_GRP_NONE, bestrise = 0;
            glow_switch_stat( texnum, &a );
            glow_switch_stat( partner, &b );
            for( g = 1; g < GLOW_NUMGRP; g++ )
            {
                int rise = (int) a.energy[g] - (int) b.energy[g];
                if( rise > bestrise )  { bestrise = rise;  best = g; }
            }
            // At least a handful of indicator pixels' worth brighter.
            if( best && bestrise >= 600 && a.n[best] )
            {
                uint32_t r = a.sum[best][0], gg = a.sum[best][1], bb = a.sum[best][2], hi;
                hi = r;
                if( gg > hi )  hi = gg;
                if( bb > hi )  hi = bb;
                lc->color.s.red   = (byte)( (uint64_t) r  * 255 / hi );
                lc->color.s.green = (byte)( (uint64_t) gg * 255 / hi );
                lc->color.s.blue  = (byte)( (uint64_t) bb * 255 / hi );
                lc->color.s.alpha = 255;
                if( lc->strength < 200 )  lc->strength = 200;
                lc->swgroup = best;
                lc->sw = GLOW_SW_LIT;
            }
            else
                lc->sw = GLOW_SW_DARK;     // nothing lights in this state: off
        }
    }
    return ( lc->sw == GLOW_SW_DARK )? NULL : lc;
}

// For the glow masks: -1 for a switch in its off state (no glow), the lit
// indicator's colour group for one that is on (only those pixels glow), 0
// for anything else.
static int glow_switch_mask_group( int texnum )
{
    glow_lampcolor_t * lc = glow_texture_light_raw( texnum );
    if( lc == NULL )
        return 0;
    if( glow_texture_light( texnum ) == NULL )
        return -1;
    return ( lc->sw == GLOW_SW_LIT )? lc->swgroup : 0;
}

static glow_lamp_t * glow_new_lamp( void )
{
    if( glow_numlamps >= LAMP_MAX )
        return NULL;
    if( glow_lamps == NULL )
    {
        glow_lamps = malloc( LAMP_MAX * sizeof(glow_lamp_t) );
        if( glow_lamps == NULL )  return NULL;
    }
    glow_lamps[ glow_numlamps ].share = 1.0f;
    return &glow_lamps[ glow_numlamps++ ];
}

static void glow_set_lamp_color( glow_lamp_t * L, glow_lampcolor_t * lc )
{
    int k = LAMP_STRENGTH * lc->strength / 255;
    L->color.s.red   = (byte)( lc->color.s.red   * k / 255 );
    L->color.s.green = (byte)( lc->color.s.green * k / 255 );
    L->color.s.blue  = (byte)( lc->color.s.blue  * k / 255 );
    L->color.s.alpha = 255;
    L->radius = LAMP_RADIUS_MIN + (LAMP_RADIUS_MAX - LAMP_RADIUS_MIN) * lc->strength / 255.0f;
}

// Lights along one lit part of a wall, from bottom to top in height.
static void glow_wall_lamps( line_t * ld, int side, sector_t * sec, side_t * sd, int part,
                             int texnum, fixed_t bottom, fixed_t top )
{
    glow_lampcolor_t * lc;
    float x1, y1, x2, y2, len, nx, ny, z;
    int n, j;

    glow_lampcolor_t * plc = NULL;

    if( top - bottom < 4 * FRACUNIT )
        return;
    lc = glow_texture_light( texnum );
    if( lc == NULL )
    {
        // A switch that is off now still gets its light, dark until it is
        // thrown (glow_lamp_follow), sized by its other state's.
        int partner = glow_switch_partner( texnum );
        plc = ( partner > 0 )? glow_texture_light( partner ) : NULL;
        if( plc == NULL )
            return;
    }
    x1 = FIXED_TO_FLOAT( ld->v1->x );  y1 = FIXED_TO_FLOAT( ld->v1->y );
    x2 = FIXED_TO_FLOAT( ld->v2->x );  y2 = FIXED_TO_FLOAT( ld->v2->y );
    len = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    if( len < 1.0f )
        return;
    // Side 0 is on the right looking from v1 to v2.
    nx = (y2 - y1) / len;
    ny = -(x2 - x1) / len;
    if( side )  { nx = -nx;  ny = -ny; }
    z = FIXED_TO_FLOAT( bottom ) + FIXED_TO_FLOAT( top - bottom ) * 0.5f;
    n = (int)( len / LAMP_SPACING + 0.5f );
    if( n < 1 )  n = 1;
    for( j = 0; j < n; j++ )
    {
        float t = (j + 0.5f) / n;
        glow_lamp_t * L = glow_new_lamp();
        if( L == NULL )  return;
        L->x = x1 + (x2 - x1) * t + nx * LAMP_OUT;
        L->y = y1 + (y2 - y1) * t + ny * LAMP_OUT;
        L->z = z;
        L->sector = sec;
        L->ceiling = 0;
        L->side = sd;
        L->line = ld;
        L->part = part;
        L->texnum = texnum;
        if( lc )
            glow_set_lamp_color( L, lc );
        else
        {
            glow_set_lamp_color( L, plc );
            L->color.rgba = 0;
        }
    }
}

static void glow_ceiling_lamps( sector_t * sec )
{
    glow_lampcolor_t lc;
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f, step, x, y, ceil, height;
    int i, made = 0;

    if( sec->ceilingpic < 0 || (unsigned) sec->ceilingpic >= numlevelflats )
        return;
    if( ! glow_flat_light( levelflats[ sec->ceilingpic ].lumpnum, &lc ) )
        return;
    if( sec->linecount <= 0 )
        return;
    for( i = 0; i < sec->linecount; i++ )
    {
        line_t * ld = sec->linelist[i];
        float vx[2] = { FIXED_TO_FLOAT( ld->v1->x ), FIXED_TO_FLOAT( ld->v2->x ) };
        float vy[2] = { FIXED_TO_FLOAT( ld->v1->y ), FIXED_TO_FLOAT( ld->v2->y ) };
        int k;
        for( k = 0; k < 2; k++ )
        {
            if( vx[k] < minx )  minx = vx[k];
            if( vx[k] > maxx )  maxx = vx[k];
            if( vy[k] < miny )  miny = vy[k];
            if( vy[k] > maxy )  maxy = vy[k];
        }
    }
    ceil = FIXED_TO_FLOAT( sec->ceilingheight );
    height = FIXED_TO_FLOAT( sec->ceilingheight - sec->floorheight );
    // Spread no more than LAMP_CEIL_MAX lights over a big lit ceiling.
    step = LAMP_SPACING;
    while( ((maxx - minx) / step + 1) * ((maxy - miny) / step + 1) > LAMP_CEIL_MAX )
        step *= 1.5f;
    for( y = miny + step * 0.5f; y < maxy && made < LAMP_CEIL_MAX; y += step )
    {
        for( x = minx + step * 0.5f; x < maxx && made < LAMP_CEIL_MAX; x += step )
        {
            glow_lamp_t * L;
            if( R_PointInSubsector( (fixed_t)( x * 65536.0f ), (fixed_t)( y * 65536.0f ) )->sector != sec )
                continue;
            L = glow_new_lamp();
            if( L == NULL )  return;
            L->x = x;  L->y = y;  L->z = ceil - LAMP_CEIL_DROP;
            L->sector = sec;
            L->ceiling = 1;
            L->side = NULL;
            L->line = NULL;
            glow_set_lamp_color( L, &lc );
            // A ceiling lamp has to reach the floor under it, and spread
            // over it.
            if( L->radius < height * 1.5f + 64.0f )
                L->radius = height * 1.5f + 64.0f;
            made++;
        }
    }
    if( made == 0 )
    {
        // Too small for the grid: one in the middle of its box.
        glow_lamp_t * L = glow_new_lamp();
        if( L == NULL )  return;
        L->x = (minx + maxx) * 0.5f;  L->y = (miny + maxy) * 0.5f;
        L->z = ceil - LAMP_CEIL_DROP;
        L->sector = sec;
        L->ceiling = 1;
        L->side = NULL;
        L->line = NULL;
        glow_set_lamp_color( L, &lc );
        if( L->radius < height * 1.5f + 64.0f )
            L->radius = height * 1.5f + 64.0f;
    }
}

// Crowding, per lamp.  Each light is two passes and the multiply stacks as a
// product, so ten panels round one computer wall made it white.  Each lamp
// gets a share of its strength, 1 / (1 + how much the lamps round it
// overlap it), once, at load.  A lone switch keeps all of it; a strip of
// lights or a wall of panels shares about one light's worth.  (It used to
// be done per surface, which cut the light hard at the edge between a wall
// with one lamp reaching it and the floor with several -- a lit rectangle
// beside every switch, and the switch's room unlit.)
static void glow_lamp_shares( void )
{
    int i, j;
    for( i = 0; i < glow_numlamps; i++ )
    {
        glow_lamp_t * A = &glow_lamps[i];
        float crowd = 0.0f;
        for( j = 0; j < glow_numlamps; j++ )
        {
            glow_lamp_t * B = &glow_lamps[j];
            float dx, dy, dz, d2, r2;
            if( j == i )  continue;
            dx = B->x - A->x;
            if( dx > A->radius || dx < -A->radius )  continue;
            dy = B->y - A->y;
            if( dy > A->radius || dy < -A->radius )  continue;
            dz = B->z - A->z;
            d2 = dx*dx + dy*dy + dz*dz;
            r2 = A->radius * A->radius;
            if( d2 < r2 )
                crowd += 1.0f - d2 / r2;
        }
        A->share = 1.0f / (1.0f + crowd);
    }
}

static int glow_ref_cmp( const void * a, const void * b )
{
    const glow_lamp_ref_t * ra = a, * rb = b;
    if( ra->sec != rb->sec )  return ra->sec - rb->sec;
    return ra->lamp - rb->lamp;
}

// Finds every lamp in the level, once per level, on first use.
static boolean glow_build_lamps( void )
{
    uint32_t i;
    int s, nrefs, maxrefs;

    if( glow_lampbase == sectors && glow_lamp_numsec == numsectors )
        return ( glow_sec_first != NULL );
    glow_lamps_free();
    glow_lampbase = sectors;
    glow_lamp_numsec = numsectors;
    if( numsectors <= 0 )
        return false;

    for( i = 0; i < numlines; i++ )
    {
        line_t * ld = &lines[i];
        int side;
        for( side = 0; side < 2; side++ )
        {
            side_t * sd;
            sector_t * sec, * other;
            if( ld->sidenum[side] == NULL_INDEX )  continue;
            sd = &sides[ ld->sidenum[side] ];
            sec = sd->sector;
            other = side ? ld->frontsector : ld->backsector;
            if( sec == NULL )  continue;
            if( other == NULL )
                glow_wall_lamps( ld, side, sec, sd, GLOW_PART_MID, sd->midtexture,
                                 sec->floorheight, sec->ceilingheight );
            else
            {
                if( other->ceilingheight < sec->ceilingheight )
                    glow_wall_lamps( ld, side, sec, sd, GLOW_PART_TOP, sd->toptexture,
                                     other->ceilingheight, sec->ceilingheight );
                if( other->floorheight > sec->floorheight )
                    glow_wall_lamps( ld, side, sec, sd, GLOW_PART_BOTTOM, sd->bottomtexture,
                                     sec->floorheight, other->floorheight );
            }
        }
    }
    for( s = 0; s < numsectors; s++ )
        glow_ceiling_lamps( &sectors[s] );

    glow_lamp_shares();

    // Who each lamp lights: its sector, and those one opening away.
    maxrefs = 0;
    for( s = 0; s < glow_numlamps; s++ )
        maxrefs += 1 + glow_lamps[s].sector->linecount;
    glow_lamp_refs = malloc( (maxrefs + 1) * sizeof(glow_lamp_ref_t) );
    glow_sec_first = calloc( numsectors + 1, sizeof(int) );
    if( glow_lamp_refs == NULL || glow_sec_first == NULL )
    {
        glow_lamps_free();
        glow_lampbase = sectors;
        glow_lamp_numsec = numsectors;
        return false;
    }
    nrefs = 0;
    for( s = 0; s < glow_numlamps; s++ )
    {
        sector_t * home = glow_lamps[s].sector;
        int k, first = nrefs;
        glow_lamp_refs[nrefs].sec = home - sectors;
        glow_lamp_refs[nrefs].lamp = s;
        glow_lamp_refs[nrefs].via = NULL;
        nrefs++;
        for( k = 0; k < home->linecount; k++ )
        {
            line_t * ld = home->linelist[k];
            sector_t * o;
            int q, dup = 0;
            if( ld->frontsector == NULL || ld->backsector == NULL )  continue;
            o = ( ld->frontsector == home )? ld->backsector : ld->frontsector;
            if( o == home )  continue;
            for( q = first; q < nrefs; q++ )
                if( glow_lamp_refs[q].sec == o - sectors )  { dup = 1;  break; }
            if( dup )  continue;
            glow_lamp_refs[nrefs].sec = o - sectors;
            glow_lamp_refs[nrefs].lamp = s;
            glow_lamp_refs[nrefs].via = ld;
            nrefs++;
        }
    }
    qsort( glow_lamp_refs, nrefs, sizeof(glow_lamp_ref_t), glow_ref_cmp );
    for( s = 0; s < nrefs; s++ )
        glow_sec_first[ glow_lamp_refs[s].sec + 1 ]++;
    for( s = 0; s < numsectors; s++ )
        glow_sec_first[ s + 1 ] += glow_sec_first[s];
    GenPrintf( EMSG_info, "DERP lamp light: %d lights\n", glow_numlamps );
    return true;
}

static void glow_set_disc( void )
{
    if( ! glow_disc.downloaded && ! glow_disc.GR_data )
    {
        // Alpha-intensity: a soft disc, falling as the square to the rim.
        uint16_t * data = Z_Malloc( 64 * 64 * sizeof(uint16_t), PU_HWRCACHE, &glow_disc.GR_data );
        int x, y;
        for( y = 0; y < 64; y++ )
        {
            for( x = 0; x < 64; x++ )
            {
                float dx = (x - 31.5f) / 31.5f, dy = (y - 31.5f) / 31.5f;
                // Broad, not a spot: 1 - r^2, so half way out it still has
                // three quarters.  (1 - r)^2 had a quarter there, which read
                // as a small dot of colour under each lamp.
                float f = 1.0f - ( dx*dx + dy*dy );
                byte v;
                if( f < 0.0f )  f = 0.0f;
                v = (byte)( 255.0f * f );
                data[ y * 64 + x ] = (v << 8) | v;
            }
        }
        glow_disc.GR_format = GR_TEXFMT_ALPHA_INTENSITY_88;
        glow_disc.width = glow_disc.height = 64;
        glow_disc.tfflags = 0;   // clamped: outside the disc adds nothing
    }
    HWD.pfnSetTexture( &glow_disc );
}

// A wall light whose texture has changed since the level loaded takes the
// new texture's colour, or goes out if it is not a light.  Cached per
// texture, so this is a compare per light per use.
static void glow_lamp_follow( glow_lamp_t * L )
{
    short now;
    glow_lampcolor_t * lc;
    if( L->side == NULL )
        return;
    now = ( L->part == GLOW_PART_TOP )? L->side->toptexture
        : ( L->part == GLOW_PART_BOTTOM )? L->side->bottomtexture
        : L->side->midtexture;
    if( now == L->texnum )
        return;
    L->texnum = now;
    lc = glow_texture_light( now );
    if( lc )
        glow_set_lamp_color( L, lc );
    else
        L->color.rgba = 0;   // dark until it changes back
}

static boolean glow_opening_open( line_t * ld )
{
    fixed_t top, bot;
    if( ld == NULL )  return true;
    top = ld->frontsector->ceilingheight;
    if( ld->backsector->ceilingheight < top )  top = ld->backsector->ceilingheight;
    bot = ld->frontsector->floorheight;
    if( ld->backsector->floorheight > bot )  bot = ld->backsector->floorheight;
    return top > bot;
}

// How much of a source's strength is left at a distance from it, before the
// disc's own fade across the surface: falling as the square, so a floor a
// room's height under a ceiling light still gets most of it.  (Falling
// linearly, it got a third, which with the strength above was nothing.)
static float glow_falloff( float d, float radius )
{
    float f = d / radius;
    return ( f >= 1.0f )? 0.0f : 1.0f - f * f;
}

// The lights reaching one surface, gathered before any is drawn.
#define GLOW_MAX_HITS  48
typedef struct {
    glow_lamp_t * L;
    float         w;          // falloff * blink, 0..1
    float         s[4], t[4]; // the disc's coords at the quad's corners
} glow_hit_t;

static void glow_draw_lamp( glow_lamp_t * L, vxtx3d_t * v, int n, float falloff, boolean * bound )
{
    FSurfaceInfo_t Surf;
    int k = (int)( falloff * 255.0f );   // blink and crowding already in it
    if( ! (L->color.s.red | L->color.s.green | L->color.s.blue) )
        return;   // a light that has gone out
    if( k <= 0 )
        return;
    if( ! *bound )
    {
        glow_set_disc();
        *bound = true;
    }
    Surf.FlatColor.s.red   = (byte)( L->color.s.red   * k / 255 );
    Surf.FlatColor.s.green = (byte)( L->color.s.green * k / 255 );
    Surf.FlatColor.s.blue  = (byte)( L->color.s.blue  * k / 255 );
    Surf.FlatColor.s.alpha = 255;
    Surf.polyflags = 0;
    Surf.texflags = 0;
#ifdef BLEND_FIELD
    HWD.pfnDrawPolygon( &Surf, v, n, PF_LightMul | PF_Modulated | PF_Decal | PF_Clip | PF_NoTint );
#endif
    // And the additive half: colour * disc (texture) * disc (its alpha) *
    // LAMP_ADD, on top of the multiply.
    Surf.FlatColor.s.alpha = LAMP_ADD;
    HWD.pfnDrawPolygon( &Surf, v, n, PF_Additive | PF_Modulated | PF_Decal | PF_Clip | PF_NoTint );
}

// The lamps' light on a wall quad.  Returns whether anything was drawn.
static boolean glow_lamp_wall( vxtx3d_t * vxtx )
{
    static glow_hit_t hit[GLOW_MAX_HITS];
    sector_t * sec = gr_frontsector;
    int si, r, nhit;
    float wx, wz, len, ux, uz, ybot, ytop;
    boolean bound = false;

    if( sec == NULL || ! glow_build_lamps() )
        return false;
    si = sec - sectors;
    if( si < 0 || si >= glow_lamp_numsec || glow_sec_first[si] == glow_sec_first[si+1] )
        return false;
    wx = vxtx[1].x - vxtx[0].x;
    wz = vxtx[1].z - vxtx[0].z;
    len = sqrt( wx*wx + wz*wz );
    if( len < 1.0f )
        return false;
    ux = wx / len;  uz = wz / len;
    ybot = vxtx[0].y < vxtx[1].y ? vxtx[0].y : vxtx[1].y;
    ytop = vxtx[2].y > vxtx[3].y ? vxtx[2].y : vxtx[3].y;

    nhit = 0;
    for( r = glow_sec_first[si]; r < glow_sec_first[si+1] && nhit < GLOW_MAX_HITS; r++ )
    {
        glow_lamp_t * L = &glow_lamps[ glow_lamp_refs[r].lamp ];
        float rx, rz, dist, rp, a0;
        int i;
        if( ! glow_opening_open( glow_lamp_refs[r].via ) )
            continue;
        glow_lamp_follow( L );
        rx = L->x - vxtx[0].x;
        rz = L->y - vxtx[0].z;
        // Only the face the lamp is in front of (HWR_WallLighting's test).
        if( rx * wz - rz * wx < 0 )
            continue;
        dist = fabs( rx * uz - rz * ux );
        if( dist >= L->radius )
            continue;
        rp = sqrt( L->radius * L->radius - dist * dist );
        // A light close to a wall lights a tight pool on it, not a broad
        // one: light that grazes a wall falls off fast along it.  So a
        // switch has a bright pool round it on its own wall and the pieces
        // either side, and lights the rest of the room normally.
        if( rp > dist * 4.0f + 48.0f )
            rp = dist * 4.0f + 48.0f;
        a0 = rx * ux + rz * uz;
        if( a0 + rp < 0.0f || a0 - rp > len || L->z + rp < ybot || L->z - rp > ytop )
            continue;
        hit[nhit].L = L;
        hit[nhit].w = glow_falloff( dist, L->radius ) * L->share
                      * glow_sector_scale( L->sector ) / 255.0f;
        if( hit[nhit].w <= 0.0f || ! (L->color.s.red | L->color.s.green | L->color.s.blue) )
            continue;
        for( i = 0; i < 4; i++ )
        {
            float a = (vxtx[i].x - vxtx[0].x) * ux + (vxtx[i].z - vxtx[0].z) * uz;
            hit[nhit].s[i] = 0.5f + (a - a0) / (2.0f * rp);
            hit[nhit].t[i] = 0.5f + (L->z - vxtx[i].y) / (2.0f * rp);
        }
        nhit++;
    }
    for( r = 0; r < nhit; r++ )
    {
        vxtx3d_t v[4];
        int i;
        for( i = 0; i < 4; i++ )
        {
            v[i] = vxtx[i];
            v[i].sow = hit[r].s[i];
            v[i].tow = hit[r].t[i];
        }
        glow_draw_lamp( hit[r].L, v, 4, hit[r].w, &bound );
    }
    return bound;
}

// The lamps' light on a floor or ceiling polygon.
static boolean glow_lamp_plane( vxtx3d_t * pv, int n )
{
    static vxtx3d_t v[512];
    static glow_hit_t hit[GLOW_MAX_HITS];
    sector_t * sec = gr_frontsector;
    int si, r, i, nhit;
    float h, minx, maxx, minz, maxz;
    boolean isfloor, bound = false;

    if( sec == NULL || n < 3 || n > 512 || ! glow_build_lamps() )
        return false;
    si = sec - sectors;
    if( si < 0 || si >= glow_lamp_numsec || glow_sec_first[si] == glow_sec_first[si+1] )
        return false;
    h = pv[0].y;
    isfloor = fabs( h - FIXED_TO_FLOAT( sec->floorheight ) )
              <= fabs( h - FIXED_TO_FLOAT( sec->ceilingheight ) );
    minx = maxx = pv[0].x;
    minz = maxz = pv[0].z;
    for( i = 1; i < n; i++ )
    {
        if( pv[i].x < minx )  minx = pv[i].x;
        if( pv[i].x > maxx )  maxx = pv[i].x;
        if( pv[i].z < minz )  minz = pv[i].z;
        if( pv[i].z > maxz )  maxz = pv[i].z;
    }

    nhit = 0;
    for( r = glow_sec_first[si]; r < glow_sec_first[si+1] && nhit < GLOW_MAX_HITS; r++ )
    {
        glow_lamp_t * L = &glow_lamps[ glow_lamp_refs[r].lamp ];
        float dh, rp;
        if( ! glow_opening_open( glow_lamp_refs[r].via ) )
            continue;
        glow_lamp_follow( L );
        if( isfloor ? (L->z <= h) : (L->z >= h || L->ceiling) )
            continue;
        dh = fabs( L->z - h );
        if( dh >= L->radius )
            continue;
        rp = sqrt( L->radius * L->radius - dh * dh );
        if( L->x + rp < minx || L->x - rp > maxx || L->y + rp < minz || L->y - rp > maxz )
            continue;
        hit[nhit].L = L;
        hit[nhit].w = glow_falloff( dh, L->radius ) * L->share
                      * glow_sector_scale( L->sector ) / 255.0f;
        hit[nhit].s[0] = rp;   // the disc's radius on this plane
        if( hit[nhit].w <= 0.0f || ! (L->color.s.red | L->color.s.green | L->color.s.blue) )
            continue;
        nhit++;
    }
    for( r = 0; r < nhit; r++ )
    {
        glow_lamp_t * L = hit[r].L;
        float rp = hit[r].s[0];
        for( i = 0; i < n; i++ )
        {
            v[i] = pv[i];
            v[i].sow = 0.5f + (pv[i].x - L->x) / (2.0f * rp);
            v[i].tow = 0.5f + (pv[i].z - L->y) / (2.0f * rp);
        }
        glow_draw_lamp( L, v, n, hit[r].w, &bound );
    }
    return bound;
}

// Tints a sprite's or the weapon's light colour by the lamps reaching the
// point it stands at, as PF_LightMul does a wall: colour * (1 + lamps).
void HWR_Glow_Tint( RGBA_t * c, fixed_t fx, fixed_t fy, fixed_t fz, sector_t * sec )
{
    int si, r, add[3] = { 0, 0, 0 };
    float x, y, z;

    if( ! cv_grglow_lamplight.EV || fixedcolormap || sec == NULL || ! glow_build_lamps() )
        return;
    si = sec - sectors;
    if( si < 0 || si >= glow_lamp_numsec || glow_sec_first[si] == glow_sec_first[si+1] )
        return;
    x = FIXED_TO_FLOAT( fx );  y = FIXED_TO_FLOAT( fy );  z = FIXED_TO_FLOAT( fz );
    for( r = glow_sec_first[si]; r < glow_sec_first[si+1]; r++ )
    {
        glow_lamp_t * L = &glow_lamps[ glow_lamp_refs[r].lamp ];
        float dx, dy, dz, k;
        if( ! glow_opening_open( glow_lamp_refs[r].via ) )
            continue;
        glow_lamp_follow( L );
        dx = L->x - x;  dy = L->y - y;  dz = L->z - z;
        k = glow_falloff( sqrt( dx*dx + dy*dy + dz*dz ), L->radius ) * L->share
            * glow_sector_scale( L->sector );
        if( k <= 0.0f )
            continue;
        add[0] += (int)( L->color.s.red   * k / 255.0f );
        add[1] += (int)( L->color.s.green * k / 255.0f );
        add[2] += (int)( L->color.s.blue  * k / 255.0f );
    }
    {
        int v, hi = add[0];
        // Crowding: no more than one full light's worth on a sprite.
        if( add[1] > hi )  hi = add[1];
        if( add[2] > hi )  hi = add[2];
        if( hi > 255 )
        {
            add[0] = add[0] * 255 / hi;
            add[1] = add[1] * 255 / hi;
            add[2] = add[2] * 255 / hi;
        }
        v = c->s.red   * (255 + LAMP_SPRITE_GAIN * add[0]) / 255;  c->s.red   = (v > 255)? 255 : v;
        v = c->s.green * (255 + LAMP_SPRITE_GAIN * add[1]) / 255;  c->s.green = (v > 255)? 255 : v;
        v = c->s.blue  * (255 + LAMP_SPRITE_GAIN * add[2]) / 255;  c->s.blue  = (v > 255)? 255 : v;
    }
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

    if( cv_grglow_ripples.EV && glow_ripple_wall( vxtx ) )
        drew = true;

    if( cv_grglow_lamplight.EV && glow_lamp_wall( vxtx ) )
        drew = true;

    if( drew )
        glow_rebind();
}

// The glowing pixels of a flat.  Returns whether anything was drawn.
static boolean glow_plane_mask( vxtx3d_t * v, int numverts, lumpnum_t flatlump )
{
    glow_entry_t * e;
    byte s;

    if( ! cv_grglow_lights.EV && ! cv_grglow_liquids.EV )
        return false;
    e = glow_flat_entry( flatlump );
    if( e == NULL )
        return false;
    if( glow_is_liquid( e->kind ) )
    {
        if( ! cv_grglow_liquids.EV )  return false;
        // A pool glows whatever the room's light.
        glow_draw_additive( &e->mip, v, numverts, GLOW_LIQUID_ALPHA );
        return true;
    }
    if( ! cv_grglow_lights.EV )
        return false;
    s = glow_sector_scale( gr_frontsector );
    if( ! s )
        return false;
    glow_draw_mask( &e->mip, v, numverts, s );
    return true;
}

// Called from HWR_RenderPlane, after the plane itself.
void HWR_Glow_Plane( vxtx3d_t * v, int numverts, lumpnum_t flatlump,
                     FBITFIELD polyflags )
{
    boolean drew;

    if( fixedcolormap || ! glow_blend_opaque( polyflags ) )
        return;
    drew = glow_plane_mask( v, numverts, flatlump );
    if( cv_grglow_lamplight.EV && glow_lamp_plane( v, numverts ) )
        drew = true;
    if( drew )
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
