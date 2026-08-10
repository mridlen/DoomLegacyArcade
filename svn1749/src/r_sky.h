// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_sky.h 1558 2020-11-17 23:36:10Z wesleyjohnson $
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
// $Log: r_sky.h,v $
// Revision 1.4  2001/03/21 18:24:39  stroggonmeth
//
// Revision 1.3  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Sky rendering.
//
//-----------------------------------------------------------------------------

#ifndef R_SKY_H
#define R_SKY_H

#include "m_fixed.h"
#include "r_defs.h"
  // patch_t

#ifdef __GNUG__
#pragma interface
#endif

// SKY, store the number for name.
#define SKYFLATNAME  "F_SKY1"

// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT         22

#define SKY_FLAT_HEIGHT  16
#define SKY_FLAT_WIDTH   64

extern byte skytop_flat[SKY_FLAT_WIDTH][SKY_FLAT_HEIGHT];  // above sky
extern byte ground_flat[SKY_FLAT_WIDTH][SKY_FLAT_HEIGHT];  // below sky

extern byte *  sky_pict;
extern int     sky_texture;
extern int     sky_texturemid;
extern int     sky_height;
extern int     sky_yl_min_oc, sky_yh_max_oc;
extern fixed_t sky_scale;
extern uint32_t sky_widthmask;
extern byte    sky_240;  // 0=std 128 sky, 1=240 high sky
                                  // see SCR_SetMode

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.
extern int     sky_flatnum;

//added:12-02-98: declare the asm routine which draws the sky columns
void R_DrawSkyColumn (void);

// Called once at startup.
void R_Init_SkyMap (void);

// call after skytexture is set to adapt for old/new skies
void R_Setup_SkyDraw (void);

#if 0
void R_StorePortalRange(void);
void R_Init_Portals(void);
void R_Clear_Portals(void);
void R_Draw_Portals(void);
#endif

void R_Set_Sky_Scale (void);

#endif
