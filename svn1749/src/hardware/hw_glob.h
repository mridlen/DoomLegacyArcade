// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_glob.h 1591 2021-10-11 02:46:18Z wesleyjohnson $
//
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: hw_glob.h,v $
// Revision 1.15  2001/08/12 22:08:40  hurdler
// Add alpha value for 3d water
//
// Revision 1.14  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.13  2001/05/16 21:21:15  bpereira
// Revision 1.12  2000/11/18 15:51:25  bpereira
// Revision 1.11  2000/11/04 16:23:44  bpereira
// Revision 1.10  2000/11/02 19:49:39  bpereira
// Revision 1.9  2000/09/21 16:45:11  bpereira
//
// Revision 1.8  2000/04/27 17:48:47  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.7  2000/04/24 15:46:34  hurdler
// Support colormap for text
//
// Revision 1.6  2000/04/23 16:19:52  bpereira
// Revision 1.5  2000/04/22 21:08:23  hurdler
//
// Revision 1.4  2000/04/22 16:09:14  hurdler
// support skin color in hardware mode
//
// Revision 1.3  2000/03/29 19:39:49  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      globals (shared data & code) for hw_ modules
//
//-----------------------------------------------------------------------------

#ifndef HW_GLOB_H
#define HW_GLOB_H

#include "hw_defs.h"
#include "hw_main.h"
#include "hw_poly.h"

// the original aspect ratio of Doom graphics isn't square
#define ORIGINAL_ASPECT     (320.0f/200.0f)

// Too far and it leaves gaps which show sky.
//#define NEAR_PLANE_DIST     (0.9f)
#define NEAR_CLIP_DIST     (0.05f)


// needed for sprite rendering
// equivalent of the software renderer's vissprites
typedef struct gr_vissprite_s
{
    // Doubly linked list
    struct gr_vissprite_s* prev;
    struct gr_vissprite_s* next;
    float               x1;
    float               x2;
    float               tz;
    float               ty;
    lumpnum_t           patch_lumpnum;
    boolean             flip;
    byte                translucency;       //alpha level 0-255
    byte                sectorlight;        // ...
    mobj_t              *mobj; 
   //Hurdler: 25/04/2000: now support colormap in hardware mode
    byte                *colormap;
} gr_vissprite_t;


// --------
// hw_bsp.c
// --------
// Array of poly_subsector_t,
// Index by bsp subsector num,  0.. num_poly_subsector-1
extern  poly_subsector_t*   poly_subsectors;
extern  unsigned int        num_poly_subsector;

void HWR_Init_PolyPool (void);
void HWR_Free_PolyPool (void);


// --------
// hw_cache.c
// --------
void HWR_Init_TextureCache (void);
void HWR_Free_TextureCache (void);

void HWR_GetFlat (lumpnum_t flatlumpnum);
MipTexture_t * HWR_GetTexture (int tex, uint32_t drawflags);
void HWR_GetPatch (MipPatch_t* gpatch);
void HWR_GetMappedPatch(MipPatch_t* gpatch, byte *colormap);
MipPatch_t * HWR_GetPic( lumpnum_t lumpnum );
void HWR_SetPalette( RGBA_t *palette );

extern byte  EN_HWR_flashpalette;
// Faster palette flashes using tints.
//  palette_num : 0..15
void HWR_SetFlashPalette( byte palette_num );

void HWR_sky_mipmap( void );

// --------
// hw_draw.c
// --------
extern  float   gr_patch_scalex;
extern  float   gr_patch_scaley;

void HWR_Init_Fog (void);
void HWR_Free_Fog (void);
void HWR_FoggingOn (void);

extern  consvar_t cv_grrounddown;   //on/off

extern int patchformat;
extern int textureformat;

// ------------
// misc externs
// ------------

#endif // HW_GLOB_H
