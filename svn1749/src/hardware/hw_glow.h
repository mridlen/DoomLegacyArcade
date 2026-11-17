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
// [Arcade] DERP glow effects for the OpenGL renderer: glowing lights and
// screens, glowing liquids and the light they throw up the walls round
// them, and glowing monster eyes.  See docs/arcade/derp-glow.md.
//
//-----------------------------------------------------------------------------

#ifndef HW_GLOW_H
#define HW_GLOW_H

#include "doomtype.h"
#include "command.h"
#include "hw_defs.h"
#include "hw_data.h"

extern consvar_t cv_grglow_lights;
extern consvar_t cv_grglow_liquids;
extern consvar_t cv_grglow_eyes;

// The texture or flat the cache last handed the driver (hw_cache.c), so a
// glow pass can put it back when it is done with its own.
extern Mipmap_t * hwr_glow_bound;
extern int        hwr_glow_texnum;   // the wall texture of that, or -1

void HWR_Glow_Register( void );

// After HWR_Free_TextureCache has cleared the driver and Z_FreeTags.
void HWR_Glow_Free( void );

// Each after the surface's own draw, with its vertices and blend.
void HWR_Glow_Wall( vxtx3d_t * vxtx, FSurfaceInfo_t * pSurf, int blendmode );
void HWR_Glow_Plane( vxtx3d_t * v, int numverts, lumpnum_t flatlump,
                     FBITFIELD polyflags );
void HWR_Glow_Sprite( vxtx3d_t * vxtx, lumpnum_t patch_lumpnum,
                      Mipmap_t * smip, uint32_t mobjflags, int health );

// hw_cache.c: composite a wall texture exactly as HWR_GetTexture does, into
// the caller's mipmap.  False when the texture format is not RGBA.
boolean HWR_Glow_Composite_Texture( int texnum, Mipmap_t * mipmap );

#endif
