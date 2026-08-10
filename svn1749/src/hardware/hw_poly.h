// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_glob.h 1035 2013-08-14 00:38:40Z wesleyjohnson $
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

#ifndef HW_POLY_H
#define HW_POLY_H

#include "m_fixed.h"

// -----------
// structures
// -----------

// a vertex of a Doom 'plane' polygon
typedef struct polyvertex_s {
    float   x, y;
} polyvertex_t;

// a convex 'plane' polygon, clockwise order
typedef struct {
    int          numpts;
    polyvertex_t pts[0];
} poly_t;

// holds extra info for 3D render, for each subsector in subsectors[]
typedef struct {
    poly_t*     planepoly;  // the generated convex polygon
} poly_subsector_t;


typedef struct
{ 
    poly_subsector_t    *xsub;
    fixed_t             fixedheight;
    int                 lightlevel;
//    int                 lumpnum;
    uint16_t            picnum;  // index to levelflats
    int                 alpha;
} planeinfo_t;

#endif
