// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_bsp.h 1585 2021-09-26 05:31:57Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2012 by DooM Legacy Team.
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
// $Log: r_bsp.h,v $
// Revision 1.8  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.7  2000/11/03 02:37:36  stroggonmeth
//
// Revision 1.6  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/08/31 14:30:56  bpereira
//
// Revision 1.4  2000/08/10 14:58:07  ydario
// OS/2 port
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
//      Refresh module, BSP traversal and handling.
//
//-----------------------------------------------------------------------------


#ifndef R_BSP_H
#define R_BSP_H

#include "doomtype.h"
#include "r_defs.h"
  // seg_t, side_t, line_t, sector_t, drawseg_t

#ifdef __GNUG__
#pragma interface
#endif


extern seg_t*           curline;
extern side_t*          sidedef;
extern line_t*          linedef;
extern sector_t*        frontsector;
extern sector_t*        backsector;

extern boolean          skymap;
extern byte             doorclosed;

// faB: drawsegs are now allocated on the fly ... see r_segs.c
// extern drawseg_t*       drawsegs;
//SoM: 3/26/2000: Use boom code.
extern drawseg_t*       drawsegs;
extern uint16_t         maxdrawsegs;
extern drawseg_t*       ds_p;
extern drawseg_t*       firstnewseg;

extern lighttable_t**   hscalelight;
extern lighttable_t**   vscalelight;
extern lighttable_t**   dscalelight;


typedef void (*drawfunc_t) (int start, int stop);


// BSP
void R_Clear_ClipSegs (void);
void R_Setup_ClipSegs (void);
void R_Clear_DrawSegs (void);

void R_RenderBSPNode ( bsp_child_t bspnum );

sector_t* R_FakeFlat(sector_t *sec, sector_t *tempsec, boolean back,
             /*OUT*/ lightlev_t *floorlightlevel,
                     lightlev_t *ceilinglightlevel );

// Find light under planeheight, plain version
ff_light_t *  R_GetPlaneLight(sector_t* sector, fixed_t planeheight);
// Find light under planeheight, slight difference according to viewz
ff_light_t *  R_GetPlaneLight_viewz(sector_t* sector, fixed_t planeheight);

void   R_Prep3DFloors(sector_t* sector);
#endif
