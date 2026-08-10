// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_maputl.h 1676 2024-03-03 04:42:10Z wesleyjohnson $
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
// $Log: p_maputl.h,v $
// Revision 1.4  2000/11/02 19:49:36  bpereira
// Revision 1.3  2000/04/08 17:29:25  stroggonmeth
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      map utility functions
//
//-----------------------------------------------------------------------------


#ifndef P_MAPUTL_H
#define P_MAPUTL_H

#include "doomtype.h"
#include "r_defs.h"
#include "m_fixed.h"

//
// P_MAPUTL
//
typedef struct
{
    fixed_t     x;
    fixed_t     y;
    fixed_t     dx;
    fixed_t     dy;

} divline_t;

typedef struct
{
    fixed_t     frac;           // along trace line
    boolean     isaline;
    union {
        mobj_t* thing;
        line_t* line;
    }                   d;
} intercept_t;

/*#define MAXINTERCEPTS   128

extern intercept_t      intercepts[MAXINTERCEPTS];
extern intercept_t*     intercept_p;*/

//SoM: 4/6/2000: Remove limit.
extern int              max_intercepts;
extern intercept_t*     intercepts;
extern intercept_t*     intercept_p;

void P_CheckIntercepts();


typedef boolean (*traverser_t) (intercept_t *in);

boolean P_PathTraverse ( fixed_t       x1,
                         fixed_t       y1,
                         fixed_t       x2,
                         fixed_t       y2,
                         int           flags,
                         traverser_t   trav);

fixed_t P_AproxDistance (fixed_t dx, fixed_t dy);
int     P_PointOnLineSide (fixed_t x, fixed_t y, const line_t* line);
int     P_PointOnDivlineSide (fixed_t x, fixed_t y, const divline_t* line);
void    P_MakeDivline ( const line_t* li, /*OUT*/ divline_t* dl);
fixed_t P_InterceptVector ( const divline_t* v2, const divline_t* v1);
int     P_BoxOnLineSide (const fixed_t* tmbox, const line_t* ld);

extern fixed_t          opentop;
extern fixed_t          openbottom;
extern fixed_t          openrange;
extern fixed_t          lowfloor;

void    P_LineOpening ( const line_t* linedef );

boolean P_BlockLinesIterator (int x, int y, boolean(*func)(line_t*) );
boolean P_BlockThingsIterator (int x, int y, boolean(*func)(mobj_t*) );

#define PT_ADDLINES     1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT     4

extern divline_t        trace;

// call your user function for each line of the blockmap in the bbox defined by the radius
/*boolean P_RadiusLinesCheck (  fixed_t    radius,
                              fixed_t    x,
                              fixed_t    y,
                              boolean   (*func)(line_t*));*/

#ifdef MBF21
//   distance : limit of search, in MAPBLOCKUNITS
mobj_t * P_RoughTargetSearch( mobj_t * mo, angle_t fov, int distance );
#endif

#endif // P_MAPUTL_H
