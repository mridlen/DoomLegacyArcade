// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_light.h 1481 2019-12-13 05:16:17Z wesleyjohnson $
//
// Copyright (C) 1998-2015 by DooM Legacy Team.
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
// $Log: hw_light.h,v $
// Revision 1.16  2001/08/27 19:59:35  hurdler
// Fix colormap in heretic + opengl, fixedcolormap and NEWCORONA
//
// Revision 1.15  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.14  2001/05/01 20:38:34  hurdler
// some fix/hack for the beta release
//
// Revision 1.13  2001/04/28 15:18:46  hurdler
// newcoronas defined again
//
// Revision 1.12  2001/04/16 21:41:39  hurdler
// do not define NEWCORONA by default
//
// Revision 1.11  2001/02/24 13:35:22  bpereira
// Revision 1.10  2001/01/25 18:56:27  bpereira
// Revision 1.9  2000/11/18 15:51:25  bpereira
// Revision 1.8  2000/08/31 14:30:57  bpereira
// Revision 1.7  2000/08/03 17:57:42  bpereira
// Revision 1.6  2000/04/16 18:38:07  bpereira
// Revision 1.5  2000/04/14 16:34:26  hurdler
// some nice changes for coronas
//
// Revision 1.4  2000/04/12 16:03:51  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.3  2000/03/29 19:39:49  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Dynamic lighting & coronas add on by Hurdler 
//
//-----------------------------------------------------------------------------

#ifndef HW_LIGHTS_H
#define HW_LIGHTS_H

//#define DO_MIRROR

#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_defs.h"


#ifdef DYLT_CORONAS
void HWR_DL_Draw_Coronas( void );
void HWR_DL_AddLightSprite(gr_vissprite_t *spr, MipPatch_t *mpatch);
#endif
#ifdef SPDR_CORONAS
void HWR_DoCoronasLighting(vxtx3d_t *outVerts, gr_vissprite_t *spr);
#endif


void HWR_Init_Light( void );
void HWR_DynamicShadowing(vxtx3d_t *clVerts, int nrClipVerts, player_t *p);
void HWR_PlaneLighting(vxtx3d_t *clVerts, int nrClipVerts);
void HWR_WallLighting(vxtx3d_t *wlVerts);
void HWR_Reset_Lights(void);
void HWR_Set_Lights(byte viewnumber);

#define DL_MAX_LIGHT    256  // maximum number of light (extra light are ignored)

// Per player
typedef struct {
    unsigned int   nb;  // number of dynamic lights
    spr_light_t * p_lspr[DL_MAX_LIGHT];
    v3d_t      position[DL_MAX_LIGHT]; // actually maximum DL_MAX_LIGHT lights
    mobj_t   * mo[DL_MAX_LIGHT];
} dynlights_t;

#endif
