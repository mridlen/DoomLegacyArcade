// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_segs.h 1630 2022-10-30 10:03:58Z wesleyjohnson $
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
// $Log: r_segs.h,v $
// Revision 1.5  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.4  2000/11/03 02:37:36  stroggonmeth
// Revision 1.3  2000/11/02 19:49:36  bpereira
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Refresh module, drawing LineSegs from BSP.
//
//-----------------------------------------------------------------------------

#ifndef R_SEGS_H
#define R_SEGS_H

#include "r_defs.h"

#ifdef __GNUG__
#pragma interface
#endif

extern lighttable_t**   walllights;

// Render with transparency, over range x1..x2
void R_RenderMaskedSegRange( drawseg_t* ds, int x1, int x2 );

void R_RenderThickSideRange( drawseg_t* ds, int x1, int x2, ffloor_t* ffloor);

void R_RenderFog( ffloor_t* fff, sector_t * fogsec, lightlev_t foglight,
                  fixed_t scale );

void R_StoreWallRange( int start, int stop );

int16_t *  get_pool16_array( int size );

#endif
