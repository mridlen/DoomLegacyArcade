// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_state.h 1736 2025-03-14 08:15:57Z wesleyjohnson $
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
// $Log: r_state.h,v $
// Revision 1.11  2001/03/21 18:24:56  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.10  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.9  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.8  2000/04/18 17:39:40  stroggonmeth
// Bug fixes and performance tuning.
//
// Revision 1.7  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.6  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.5  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.4  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.3  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#ifndef R_STATE_H
#define R_STATE_H

#include "d_player.h"
  // player_t
//#include "r_data.h"
  // m_fixed.h, r_defs.h
#include "r_defs.h"
  // m_fixed.h


#ifdef __GNUG__
#pragma interface
#endif



//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern fixed_t*         textureheight;

// needed for pre rendering (fracs)
// [WDJ] Made into one record, sprite lump values, endian swapped.
// Patch header stored as fixed_t.
typedef struct {
    fixed_t  width;
    fixed_t  height;
    fixed_t  leftoffset;
    fixed_t  topoffset;
} spritelump_t;

extern spritelump_t*    spritelumps;  // array ptr
// to get one
int  R_Get_spritelump( void );  // get by id number

// colormap lightmaps from wad COLORMAP lump
extern lighttable_t*    reg_colormaps;

//SoM: 3/30/2000: Boom colormaps.
//SoM: 4/7/2000: Had to put a limit on colormaps :(
//#define                 MAXCOLORMAPS 30
// [WDJ]: 5/17/2010 hth2.wad uses 38 colormaps
// Too little memory to save to use dynamic methods.
#define                 MAXCOLORMAPS 64

// defined colormap lightmaps descriptors
extern int              num_extra_colormaps;
extern extracolormap_t  extra_colormaps[MAXCOLORMAPS];

extern int		rdraw_viewwidth;		// was viewwidth
extern int              rdraw_scaledviewwidth;		// was scaledrviewwidth
extern int              rdraw_viewheight;		// was viewheight

extern int              firstflat;
extern int              firstwaterflat; //added:18-02-98:WATER!

// for global animation
#if 0
extern int*             flattranslation;
#endif
extern int*             texturetranslation;


//
// Lookup tables for map data.
//
extern uint16_t         numsprites;
extern spritedef_t*     sprites;

extern uint32_t         numvertexes;
extern vertex_t*        vertexes;

extern uint32_t         numsegs;
extern seg_t*           segs;

extern uint32_t         numsectors;
extern sector_t*        sectors;  // [0..(numsectors-1)]

extern uint32_t         numsubsectors;
extern subsector_t*     subsectors;

extern uint32_t         numnodes;
extern node_t*          nodes;

extern uint32_t         numlines;
extern line_t*          lines;

extern uint32_t         numsides;
extern side_t*          sides;


//
// POV data.
//
extern fixed_t          viewx;
extern fixed_t          viewy;
extern fixed_t          viewz;

// SoM: Portals require that certain functions use a different x and y pos
// than the actual view pos...
extern fixed_t          bspx;
extern fixed_t          bspy;

extern angle_t          viewangle;
extern angle_t          aimingangle;
extern angle_t          bspangle;
extern player_t*        viewplayer;

// ?
extern angle_t          clipangle;

// Table content is -FINE_ANG90 to +FINE_ANG90.
// Lookup as viewangle_to_x[viewangle + FINE_ANG90].
// Maps the visible view angles to screen X coordinates.
extern int              viewangle_to_x[FINE_ANG180];
// The lowest viewangle that maps back to x.
// Maps (0 to screenwidth) to range (clipangle to -clipangle).
extern angle_t          x_to_viewangle[MAXVIDWIDTH+1];
//extern fixed_t                finetangent[FINEANGLES/2];

extern fixed_t          rw_distance;
extern angle_t          rw_normalangle;



// angle to line origin
extern int              rw_angle1;

// Segs count?
extern int              sscount;

#endif
