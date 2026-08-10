// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_data.h 1647 2023-02-06 09:14:08Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log: r_data.h,v $
// Revision 1.6  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.5  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Refresh module, data I/O, caching, retrieval of graphics
//      by name.
//
//-----------------------------------------------------------------------------


#ifndef R_DATA_H
#define R_DATA_H

#include "doomdef.h"
  // ENABLE_DRAWxx
#include "doomtype.h"
  // RBGA_t
#include "r_defs.h"
#include "r_state.h"
#include "command.h"
  // CV_PossibleValue_t

#ifdef __GNUG__
#pragma interface
#endif


// moved here for r_sky.c (texture_t is used)

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
// Used to read texture patch info from wad, sizes must be correct.
typedef struct
{
    // The patches leftoffset and topoffset are ignored.
    int16_t	originx;	// from top left of texture area
    int16_t	originy;
    uint16_t    patchnum;	// index [0..] of the entry in PNAMES
    int16_t	stepdir;	// 1
    uint16_t    colormap;	// 0
} mappatch_t;



//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
// Used to read texture lump from wad, sizes must be correct.
// UDS is unclear on the exact size of some of these fields.
typedef struct
{
    char                name[8];
    uint32_t		masked;		// [8] must be 4 bytes
                                        // boolean size cannot be trusted
    uint16_t            width;		// [12]
    uint16_t            height;		// [14]
    char                columndirectory[4]; //void **columndirectory; // OBSOLETE 
    uint16_t            patchcount;	// [20]
    mappatch_t		patches[1];	// [22] array
} maptexture_t;

// A single patch from a texture definition,
//  basically a rectangular area within the texture rectangle.
// Used only in internal texture struct. Wad read uses mappatch_t, which has more fields.
// There is clipping code for originx<0 and originy<0, which occur in doom wads.
// The original doom has a clipping bug when originy < 0.
typedef struct
{
    int32_t     originx;
    int32_t     originy;
    lumpnum_t	lumpnum;
} texpatch_t;

// [WDJ] 2/8/2010
typedef enum {
   TM_none,
   TM_patch,	// original single patch texture  (has draw)
   TM_picture,	// drawn into picture buffer  (has draw)
   TM_combine_patch,  // transparent combined multi-patch texture  (has draw)
   TM_multi_patch, // original multi-patch texture
   TM_column_image,    // raw image, (column x row)
   TM_row_image,   // raw image, (row x column)
   TM_masked,   // detect masked flag (hint)
   TM_picture_column,  // force column with no posts (hint)
   TM_invalid	// disabled for some internal reason
} texture_model_e;

typedef enum {
   TD_hole = 0x01, // holes in patch
   TD_odd_post = 0x02, // short posts, or multiple posts
   TD_odd_width = 0x04,  // not power of 2
   TD_message = 0x08,
   TD_wall = 0x10,    // used as wall texture
   TD_masked = 0x20,  // used as masked
   TD_1s_ready = 0x40, // ready to draw as 1s wall
   TD_2s_ready = 0x80, // ready to draw as 2s, masked
} texture_detect_e;



// texture_t describes a rectangular texture, which is composed of
// one or more graphic patches in texpatch_t structures.
// Used internally only.
typedef struct
{
    // Keep name for switch changing, etc.
    char        name[8];
    uint16_t    width;
    uint16_t    height;
    byte        detect;  // texture detects

    // All the patches[patchcount]
    //  are drawn back to front into the cached texture.
    uint16_t    patchcount;
    texpatch_t  patches[1];
} texture_t;


// All loaded and prepared textures from the start of the game,
// Contains info from the TEXTURE1 and TEXTURE2 lumps.
// [WDJ] Original Doom bug: conflict between texture[0] and 0=no-texture.
// Their solution was to not use the first texture.
// See BUGFIX_TEXTURE0 in r_data.c.
//   array[ 0.. numtextures-1 ] of texture_t*,
//   but [0] is unusable because it conflicts with 0=no-texture.

extern int             numtextures;
extern texture_t**     textures;

#define RENDER_TEXTURE_EXTRA_FULL

// [WDJ] To reduce the repeated indexing using texture id num, better locality of reference in cache.
// To allow creating a texture that was not loaded from textures[].
// Holds all the software render information.
typedef struct
{
    byte     * cache;   // graphics data generated full-size texture (maybe TM_patch, or TM_picture)
    uint32_t * columnofs;  // column offset lookup table for this texture
    uint16_t   width_tile_mask;  // mask that tiles the texture
    uint16_t   width;
#ifdef DEBUG_WINDOWED   
    texture_model_e  texture_model;	// drawing and storage models
#else
    byte       texture_model;	// drawing and storage models
#endif
    byte       detect;  // texture_detect_e flags
    byte       pixel_data_offset;  // to add to columnofs[]
#ifdef RENDER_TEXTURE_EXTRA_FULL
    byte       extra_index;  // additional texture_render for same texture
#endif
} texture_render_t;

// Array [ num_textures ]
extern texture_render_t * texture_render;

texture_render_t *  R_Get_extra_texren( int texture_num, texture_render_t * base_texren, byte req_model, byte req_detect );


// textureheight is not used in the same locality as the other texture arrays.
// Array [ num_textures ]
extern fixed_t*        textureheight;      // needed for texture pegging
// [WDJ] Future consideration, as a render struct field.
//    fixed_t    heightz;  // world coord. height, for texture pegging

//extern lighttable_t    *colormaps;
extern CV_PossibleValue_t Color_cons_t[];

// Load TEXTURE1/TEXTURE2/PNAMES definitions, create lookup tables
void  R_Load_Textures (void);
void  R_Flush_Texture_Cache (void);

// R_Create_Patch flags
enum {
  CPO_blank_trim = 0x20, // trim blank columns
};

//  src_type : TM_row_image (pic_t), TM_column_image (patch, picture)
//  src_data : source data
//  bytepp : source pixel size in bytes
//  sel_offset  : offset into pixel, 0..3
//  blank_value : pixel value that is blank space, >255 = no blank pixel value
//  out_type :  TM_patch, TM_picture, TM_column_image
//  out_flags : in created patch
//    CPO_blank_trim : trim blank columns
//  out_header : temp patch header for width and offset
byte * R_Create_Patch( unsigned int width, unsigned int height,
             /*SRC*/   byte src_type, byte * src_data, byte bytepp, byte sel_offset, uint16_t blank_value,
             /*DEST*/  byte out_type, byte out_flags, patch_t * out_header );

#if 0
void  R_Set_Texture_Patch( int texnum, patch_t * patch );
#endif

// Generate a texture from texture desc. and patches.
byte* R_GenerateTexture ( texture_render_t *  texren, byte texture_req );
byte* R_GenerateTexture2 ( int texnum, texture_render_t *  texren, byte  texture_req );


byte* R_GetFlat (int  flatnum);

// I/O, setting up the stuff.
void R_Load_Data (void);
void R_PrecacheLevel (void);

void R_Init_rdata(void);
// Upon change in rendermode.
void R_rdata_setup_rendermode( void );

// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
lumpnum_t  R_FlatNumForName (const char *name);


// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
int R_TextureNumForName (const char *name);
int R_CheckTextureNumForName (const char *name);


void R_Clear_Colormaps();
int R_ColormapNumForName(const char *name);

// The colorstr is the toptexture name.
// The ctrlstr is the midtexture name.
// The fadestr is the bottomtexture name.
int R_Create_Colormap_str(char *colorstr, char *ctrlstr, char *fadestr);

// [WDJ] Analyze an extra colormap to derive some GL parameters
void  R_Colormap_Analyze( int mapnum );

char *R_ColormapNameForNum(int num);

// [WDJ] 2012-02-06 shared for DRAW15, DRAW16, DRAW24, DRAW32
// hicolor tables, vid dependent
union color8_u {
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
  uint16_t  to16[256];
#endif
#if defined( ENABLE_DRAW24 ) || defined( ENABLE_DRAW32 )
  uint32_t  to32[256];
#endif
  byte      dummy;  // prevent errors when only 8bpp
};
extern union color8_u  color8;
void R_Init_color8_translate ( RGBA_t * palette );
#ifdef ENABLE_DRAW8_USING_12
extern byte  color12_to_8[ 0x1000 ];
void R_Init_color12_translate( RGBA_t * palette );
#endif

byte NearestColor(byte r, byte g, byte b);

// translucency tables

// TODO: add another asm routine which use the fg and bg indexes in the
//       inverse order so the 20-80 becomes 80-20 translucency, no need
//       for other tables (thus 1090,2080,5050,8020,9010, and fire special)

typedef enum
{
    // 0 is not translucent
    TRANSLU_med=1,   //sprite 50 backg 50  most shots
    TRANSLU_more=2,  //       20       80  puffs
    TRANSLU_hi=3,    //       10       90  blur effect
    TRANSLU_fire=4,  // 50 50 but brighter for fireballs, shots..
    TRANSLU_fx1=5,   // 50 50 brighter some colors, else opaque for torches
    TRANSLU_75=6,    //       75       25
    TRANSLU_ext=7    // Boom external transparency table
} translucent_e;

// Translucent table is 256x256,
// To overlay a translucent source on an existing dest:
//   *dest = table[source][*dest];
//   *dest = table[ (source<<8) + (*dest) ];

// 0 code does not have a table, so must subtract 1, (or one table size).
// Table size is 0x10000, = 1<<FF_TRANSSHIFT.
#define TRANSLU_TABLE_INDEX( trans )   (((trans)-1)<<FF_TRANSSHIFT)

typedef enum
{
    TRANSLU_TABLE_med =  TRANSLU_TABLE_INDEX(TRANSLU_med),
    TRANSLU_TABLE_more = TRANSLU_TABLE_INDEX(TRANSLU_more),
    TRANSLU_TABLE_hi =   TRANSLU_TABLE_INDEX(TRANSLU_hi),
    TRANSLU_TABLE_fire = TRANSLU_TABLE_INDEX(TRANSLU_fire),
    TRANSLU_TABLE_fx1 =  TRANSLU_TABLE_INDEX(TRANSLU_fx1)
} translucent_table_index_e;

// Table of alpha = 0..255 to translucent tables to be used for DRAW8PAL
// index by alpha >> 4
extern const unsigned int  translucent_alpha_table[16];
// alpha where reversed translucent tables are used
#define TRANSLU_REV_ALPHA    144


typedef struct
{
  uint32_t  translu_lump_num;  // translucent table lump
//  byte *   translu_map;  // translucent tables [256][256]
  // Analysis
  byte  alpha;  // alpha 0..255
  byte  opaque, clear;  // 0..100
  // render aids
  byte  substitute_std_translucent;  // from translucent_e
  byte  substitute_error;  // 0..255, 0 is perfect
  int   PF_op;  // hardware translucent operation
} translucent_map_t;

extern translucent_map_t *  translu_store;  // array

int  R_setup_translu_store( int lump_num );
  

// Fog data structure
typedef enum
{
   FW_colormap,  // use colormap fog (which only colors all sectors)
// water     
   FW_clear,     // no fog  (old WATER default)
   FW_cast,      // paint all surfaces with textures
   FW_fogfluid,  // outside, inside fluid, fogsheet
// fog
   FW_inside,    // render inside side, plane views (old FOG)
   FW_foglite,   // outside side, plane views, low alpha overall fog sheet
   FW_fogdust,   // outside, when in fog apply overall fog sheet (FOG default)
   FW_fogsheet,  // outside, inside, overall fog sheet, sector join fog sheet
   FW_num
} fogwater_effect_e;

enum fogwater_flags_e {
// default index
   FWF_water = 1,
   FWF_fog = 2,
   FWF_opaque_water = 3,
   FWF_solid_floor = 4,
   FWF_index = 0x03,
// flags
   FWF_default_alpha  = 0x40,
   FWF_default_effect = 0x80,
};

// fake floor fog and water effects
typedef struct
{
   fogwater_effect_e  effect;
   byte  alpha;  // alpha 0..255
   byte  fsh_alpha; // fog sheet reduced alpha
   byte  flags;  // FWF_ from fogwater_flags_e
} fogwater_t;

extern fogwater_t *  fweff;  // array
// 0 of the array is not used

// return index into fog_store
int R_Create_FW_effect( int special_linedef, char * tstr );
void R_Clear_FW_effect( void );
void R_FW_config_update( void );  // upon config change

#endif
