// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_splats.h 1626 2022-08-11 05:02:25Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: r_splats.h,v $
// Revision 1.5  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.4  2000/11/02 19:49:37  bpereira
// Revision 1.3  2000/04/30 10:30:10  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      flat sprites & blood splats effects
//
//-----------------------------------------------------------------------------

#ifndef R_SPLATS_H
#define R_SPLATS_H

#include "r_defs.h"

#define WALLSPLATS      // comment this out to compile without splat effects
//#define FLOORSPLATS

#define MAXLEVELSPLATS      1024

// splat flags
#define SPLATDRAWMODE_MASK   0x03       // mask to get drawmode from flags
#define SPLATDRAWMODE_OPAQUE 0x00
#define SPLATDRAWMODE_SHADE  0x01
#define SPLATDRAWMODE_TRANS  0x02
/*
#define SPLATUPPER           0x04
#define SPLATLOWER           0x08
*/
// ==========================================================================
// DEFINITIONS
// ==========================================================================

// WALL SPLATS are patches drawn on top of wall segs
typedef struct wallsplat_s wallsplat_t;
typedef struct wallsplat_s {
    int         patch;      // lump id.
    vertex_t    v1, v2;     // vertices along the linedef
    fixed_t     top;
    fixed_t     offset;     // offset in columns<<FRACBITS from start of linedef to start of splat
    int         flags;
    int       * yoffset;
    //short       xofs, yofs;
    //int         tictime;
    line_t*     line;       // the parent line of the splat seg
    wallsplat_t * next;
} wallsplat_t;

// FLOOR SPLATS are pic_t (raw horizontally stored) drawn on top of the floor or ceiling
typedef struct floorsplat_s floorsplat_t;
typedef struct floorsplat_s {
    int         pic;        // a pic_t lump id
    int         flags;
    vertex_t    verts[4];   // (x,y) as viewed from above on map
    fixed_t     z;          //     z (height) is constant for all the floorsplat
    subsector_t* subsector;       // the parent subsector
    floorsplat_t * next;
    floorsplat_t * nextvis;
} floorsplat_t;



//p_setup.c
extern float P_SegLength (seg_t* seg);

// call at P_SetupLevel()
void R_Clear_LevelSplats (void);

void R_AddWallSplat (line_t* wallline, int sectorside, const char* patchname,
                     fixed_t top, fixed_t wallfrac, int flags);
void R_AddFloorSplat (subsector_t* subsec, char* picname, fixed_t x, fixed_t y, fixed_t z, int flags);

void R_Clear_Visible_FloorSplats (void);
void R_Add_Visible_FloorSplats (subsector_t* subsec);
void R_Draw_Visible_FloorSplats (void);


#endif // R_SPLATS_H
