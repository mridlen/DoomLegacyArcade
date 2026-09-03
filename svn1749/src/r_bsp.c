// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_bsp.c 1736 2025-03-14 08:15:57Z wesleyjohnson $
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
// $Log: r_bsp.c,v $
// Revision 1.24  2004/05/16 20:25:47  hurdler
// change version to 1.43
//
// Revision 1.23  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.22  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.21  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.20  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.19  2001/03/21 18:24:39  stroggonmeth
//
// Revision 1.18  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.17  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.16  2000/11/21 21:13:17  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.15  2000/11/09 17:56:20  stroggonmeth
// Revision 1.14  2000/11/02 19:49:36  bpereira
//
// Revision 1.13  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.12  2000/05/23 15:22:34  stroggonmeth
// Not much. A graphic bug fixed.
//
// Revision 1.11  2000/05/03 23:51:01  stroggonmeth
// Revision 1.10  2000/04/20 21:47:24  stroggonmeth
// Revision 1.9  2000/04/18 17:39:39  stroggonmeth
// Revision 1.8  2000/04/15 22:12:58  stroggonmeth
//
// Revision 1.7  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.6  2000/04/11 19:07:25  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.5  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
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
//      BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "g_game.h"
#include "r_local.h"
#include "r_state.h"

#include "r_splats.h"
#include "p_local.h"
  //SoM: 4/10/2000: camera
#include "z_zone.h"

// Draw
// rendermode == render_soft

seg_t*          curline;
side_t*         sidedef;
line_t*         linedef;
sector_t*       frontsector;
sector_t*       backsector;


//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef struct
{
    int first;  // x screen position
    int last;
} cliprange_t;


//SoM: 3/28/2000: Fix from boom.
#define MAX_SOLIDSEGS         MAXVIDWIDTH/2+1

// new_seg_end is one past the last valid seg
static cliprange_t*    new_seg_end;
static cliprange_t     solidsegs[MAX_SOLIDSEGS];


//
// R_ClipSolidWallSegment
// Does handle solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
//
//   first, last: x range
// Called by R_AddLine
void R_ClipSolidWallSegment( int first, int last )
{
    cliprange_t*        next;
    cliprange_t*        start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    // Note that there are initial boundary entries that are off-screen.
    start = solidsegs;
    while (start->last < first-1)
        start++;

    if (first < start->first)
    {
        if (last < start->first-1)
        {
            // Post is entirely visible (above start),
            //  so insert a new clippost.
            R_StoreWallRange (first, last);

            //SoM: 3/28/2000: NO MORE CRASHING!
            if(new_seg_end > & solidsegs[MAX_SOLIDSEGS-1] )
            {
                // [WDJ] Soft recovery
                I_SoftError("R_ClipSolidWallSegment: Solid Segs overflow!\n");
                // Reuse segs, bad drawing, but keeps from crashing.
                new_seg_end = & solidsegs[MAX_SOLIDSEGS-4];
            }

            // Insert cliprange segment at start.	   
            // Shift cliprange segments up, from start to new_seg_end.
            if( start < new_seg_end )
            {
                memmove( start+1, start,
                         sizeof(cliprange_t) * (new_seg_end - start) );
            }
            new_seg_end++;
            // New cliprange segment, post at start
            start->first = first;
            start->last = last;
            return;
        }

        // There is a fragment above *start.
        R_StoreWallRange (first, start->first - 1);
        // Now adjust the clip size.
        start->first = first;
    }

    // Bottom contained in start?
    if (last <= start->last)
        return;

    next = start;
    while (last >= (next+1)->first-1)
    {
        // There is a fragment between two posts.
        R_StoreWallRange (next->last + 1, (next+1)->first - 1);
        next++;

        if (last <= next->last)
        {
            // Bottom is contained in next.
            // Adjust the clip size.
            start->last = next->last;
            goto crunch;
        }
    }

    // There is a fragment after *next.
    R_StoreWallRange (next->last + 1, last);
    // Adjust the clip size.
    start->last = last;

    // Remove start+1 to next from the clip list,
    // because start now covers their area.
  crunch:
    if (next == start)
    {
        // Post just extended past the bottom of one post.
        return;
    }


    while (next++ != new_seg_end)
    {
        // Remove a post.
        *++start = *next;
    }

    new_seg_end = start+1;

    //SoM: 3/28/2000: NO MORE CRASHING!
    if(new_seg_end > & solidsegs[MAX_SOLIDSEGS] )
    {
        // [WDJ] Soft recovery
        I_SoftError("R_ClipSolidWallSegment: Solid Segs overflow!\n");
        // Reuse segs, bad drawing, but keeps from crashing.
        new_seg_end = & solidsegs[MAX_SOLIDSEGS-4];
    }
}


//
// R_ClipPassWallSegment
// Clips the given range of columns,
//  but does not includes it in the clip list.
// Does handle windows,
//  e.g. LineDefs with upper and lower texture.
//
//   first, last: x range
// Called by R_AddLine
void R_ClipPassWallSegment ( int first, int last )
{
    cliprange_t*        start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = solidsegs;
    while (start->last < first-1)
        start++;

    if (first < start->first)
    {
        if (last < start->first-1)
        {
            // Post is entirely visible (above start).
            R_StoreWallRange (first, last);
            return;
        }

        // There is a fragment above *start.
        R_StoreWallRange (first, start->first - 1);
    }

    // Bottom contained in start?
    if (last <= start->last)
        return;

    while (last >= (start+1)->first-1)
    {
        // There is a fragment between two posts.
        R_StoreWallRange (start->last + 1, (start+1)->first - 1);
        start++;

        if (last <= start->last)
            return;
    }

    // There is a fragment after *next.
    R_StoreWallRange (start->last + 1, last);
}



//
// R_Clear_ClipSegs
//
void R_Clear_ClipSegs (void)
{
    solidsegs[0].first = -0x7fffffff;
    solidsegs[0].last = -1;
    solidsegs[1].first = rdraw_viewwidth;
    solidsegs[1].last = 0x7fffffff;
    new_seg_end = solidsegs+2;
}


//SoM: 3/25/2000
// This function is used to fix the automap bug which
// showed lines behind closed doors simply because the door had a dropoff.
//
// It assumes that Doom has already ruled out a door being closed because
// of front-back closure (e.g. front floor is taller than back ceiling).

//SoM:3/25/2000: indicates doors closed wrt automap bugfix:
byte   doorclosed;  // 0=open
  // used r_segs.c

// Called by R_AddLine, HWR_AddLine
int R_DoorClosed(void)
{
  return (

    // if door is closed because back is shut:
    backsector->ceilingheight <= backsector->floorheight

    // preserve a kind of transparent door/lift special effect:
    && (backsector->ceilingheight >= frontsector->ceilingheight
        || curline->sidedef->toptexture)  // 0=no-texture

    && (backsector->floorheight <= frontsector->floorheight
        || curline->sidedef->bottomtexture) // 0=no-texture

    // properly render skies (consider door "open" if both ceilings are sky):
    && (backsector->ceilingpic != sky_flatnum
        || frontsector->ceilingpic != sky_flatnum)
    );
}

//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
//
// Called by software and hardware draw.

//  sec : the original sector
//  tempsec : OUT modified copy of original sector
//  back : When used for backsector
// Called by R_Subsector, R_AddLine, R_RenderThickSideRange.
// Called by HWR_Subsector, HWR_AddLine.
sector_t* R_FakeFlat(sector_t *sec, sector_t *tempsec, boolean back,
             /*OUT*/ lightlev_t *floor_lightlevel,
                     lightlev_t *ceiling_lightlevel )
{
  int  colormapnum = -1; //SoM: 4/4/2000
  int  floorlightsubst, ceilinglightsubst; // light from another sector

  // first light substitution, may be -1 which defaults to sec->lightlevel
  floorlightsubst = sec->floorlightsec;
  ceilinglightsubst = sec->ceilinglightsec;

  //SoM: 4/4/2000: If the sector has a midmap, it's probably from 280 type
  if(sec->model == SM_colormap && sec->midmap >= 0 )
    colormapnum = sec->midmap;  // explicit colormap

  if (sec->model == SM_Boom_deep_water)	// [WDJ] 11/14/2009
  {
      // SM_Boom_deep_water passes modelsec >= 0
      const sector_t *modsecp = &sectors[sec->modelsec];

      // Replace sector being drawn, with a copy to be hacked
      *tempsec = *sec;
      colormapnum = modsecp->midmap;  // Deep-water colormap, middle-section default

      // Replace floor and ceiling height with other sector's heights.
      if( viewer_underwater )
      {
          // under the model sector floor
          tempsec->floorheight = sec->floorheight;
          // at the model sector floor, can still see above it
          if( ! viewer_at_water )
              tempsec->ceilingheight = modsecp->floorheight-1;
      }
      else
      {
          // view above the model sector floor
          tempsec->floorheight   = modsecp->floorheight;
          tempsec->ceilingheight = modsecp->ceilingheight;
      }
     
      if ((viewer_underwater && !back) || viewz <= modsecp->floorheight)
      {                   // head-below-floor hack
          // view under the model sector floor
          tempsec->floorpic    = modsecp->floorpic;
          tempsec->floor_xoffs = modsecp->floor_xoffs;
          tempsec->floor_yoffs = modsecp->floor_yoffs;

          if (viewer_underwater)
          {
            if (modsecp->ceilingpic == sky_flatnum)
            {
                // Boom ref: F_SKY1 as control sector ceiling gives strange effect.
                // Underwater, only the control sector floor appears
                // and it "envelops" the player.
                tempsec->floorheight   = tempsec->ceilingheight+1;
                if( ! viewer_at_water )
                {
                    tempsec->ceilingpic    = tempsec->floorpic;
                    tempsec->ceiling_xoffs = tempsec->floor_xoffs;
                    tempsec->ceiling_yoffs = tempsec->floor_yoffs;
                }
            }
            else if( ! viewer_at_water )
            {
                tempsec->ceilingpic    = modsecp->ceilingpic;
                tempsec->ceiling_xoffs = modsecp->ceiling_xoffs;
                tempsec->ceiling_yoffs = modsecp->ceiling_yoffs;
            }
            colormapnum = modsecp->bottommap; // Boom colormap, underwater
          }

          tempsec->lightlevel  = modsecp->lightlevel;

          // use model substitute, or model light
          floorlightsubst = (modsecp->floorlightsec >= 0) ?  modsecp->floorlightsec : sec->modelsec;
          ceilinglightsubst = (modsecp->ceilinglightsec >= 0) ? modsecp->ceilinglightsec : sec->modelsec;
      }
      else
      {
        if (viewer_overceiling
            && (sec->ceilingheight > modsecp->ceilingheight))
        {   // Above-ceiling hack
            // view over the model sector ceiling
            tempsec->ceilingheight = modsecp->ceilingheight;
            tempsec->floorheight   = modsecp->ceilingheight + 1;

                // Boom ref: F_SKY1 as control sector floor gives strange effect.
                // Over the ceiling, only the control sector ceiling appears
                // and it "envelops" the player.
            tempsec->floorpic    = tempsec->ceilingpic    = modsecp->ceilingpic;
            tempsec->floor_xoffs = tempsec->ceiling_xoffs = modsecp->ceiling_xoffs;
            tempsec->floor_yoffs = tempsec->ceiling_yoffs = modsecp->ceiling_yoffs;

            colormapnum = modsecp->topmap; // Boom colormap, over ceiling

            if (modsecp->floorpic != sky_flatnum)
            {
                // view over ceiling, model floor/ceiling
                tempsec->ceilingheight = sec->ceilingheight;
                tempsec->floorpic      = modsecp->floorpic;
                tempsec->floor_xoffs   = modsecp->floor_xoffs;
                tempsec->floor_yoffs   = modsecp->floor_yoffs;
            }

            tempsec->lightlevel  = modsecp->lightlevel;

            // use model substitute, or model light
            floorlightsubst = (modsecp->floorlightsec >= 0) ? modsecp->floorlightsec : sec->modelsec;
            ceilinglightsubst = (modsecp->ceilinglightsec >= 0) ? modsecp->ceilinglightsec : sec->modelsec;
        }
        // else normal view
      }
      sec = tempsec;

      if( EN_boom_colormap > 1 )
         colormapnum = -1; // Boom colormaps not visible until walk into sector
  }
  else if (sec->model == SM_Legacy_water) //SoM: 3/20/2000
  {
    // SM_Legacy_water passes modelsec >= 0
    sector_t*    modsecp = &sectors[sec->modelsec];

    *tempsec = *sec;

    if(viewer_underwater)
    {
      // view below model sector floor
      colormapnum = modsecp->bottommap; // Legacy colormap, underwater
      if(sec->floorlightsec >= 0)
      {
        // use substitute light
        floorlightsubst = ceilinglightsubst = sec->floorlightsec;
        tempsec->lightlevel = sectors[sec->floorlightsec].lightlevel;
      }
      if(modsecp->floorheight < tempsec->ceilingheight)
      {
        tempsec->ceilingheight = modsecp->floorheight;
        tempsec->ceilingpic = modsecp->floorpic;
        tempsec->ceiling_xoffs = modsecp->floor_xoffs;
        tempsec->ceiling_yoffs = modsecp->floor_yoffs;
      }
    }
    else if(!viewer_underwater && viewer_overceiling)
    {
      // view over model sector ceiling
      colormapnum = modsecp->topmap; // Legacy colormap, over ceiling
      if(sec->ceilinglightsec >= 0)
      {
        // use substitute light
        floorlightsubst = ceilinglightsubst = sec->ceilinglightsec;
        tempsec->lightlevel = sectors[sec->ceilinglightsec].lightlevel;
      }
      if(modsecp->ceilingheight > tempsec->floorheight)
      {
        tempsec->floorheight = modsecp->ceilingheight;
        tempsec->floorpic = modsecp->ceilingpic;
        tempsec->floor_xoffs = modsecp->ceiling_xoffs;
        tempsec->floor_yoffs = modsecp->ceiling_yoffs;
      }
    }
    else
    {
      colormapnum = modsecp->midmap;  // Legacy colormap, middle section
      //SoM: Use middle normal sector's lightlevels.
      if(modsecp->floorheight > tempsec->floorheight)
      {
        tempsec->floorheight = modsecp->floorheight;
        tempsec->floorpic = modsecp->floorpic;
        tempsec->floor_xoffs = modsecp->floor_xoffs;
        tempsec->floor_yoffs = modsecp->floor_yoffs;
      }
      else
      {
        floorlightsubst = -1; // revert floor to no subst
      }
      if(modsecp->ceilingheight < tempsec->ceilingheight)
      {
        tempsec->ceilingheight = modsecp->ceilingheight;
        tempsec->ceilingpic = modsecp->ceilingpic;
        tempsec->ceiling_xoffs = modsecp->ceiling_xoffs;
        tempsec->ceiling_yoffs = modsecp->ceiling_yoffs;
      }
      else
      {
        ceilinglightsubst = -1; // revert ceiling to no subst
      }
    }
    sec = tempsec;
  }

  // colormap that this sector uses for this frame, from colormapnum.
  if(colormapnum >= 0 && colormapnum < num_extra_colormaps)
    sec->extra_colormap = &extra_colormaps[colormapnum];
  else
    sec->extra_colormap = NULL;

  // [WDJ] return light parameters in one place
  if (floor_lightlevel) {
    *floor_lightlevel = (floorlightsubst >= 0) ?
       sectors[floorlightsubst].lightlevel : sec->lightlevel ;
  }

  if (ceiling_lightlevel) {
    *ceiling_lightlevel = (ceilinglightsubst >= 0) ?
       sectors[ceilinglightsubst].lightlevel : sec->lightlevel ;
  }
   
  return sec;
}



//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
//   frontsector : the visible sector of the subsector
// Called by R_Subsector
void R_AddLine (seg_t*  lineseg)
{
    static sector_t     tempsec; //SoM: FakeFlat ceiling/water

    int                 x1, x2;
    angle_t             angle1, angle2;
    angle_t             span;

    curline = lineseg;

    // OPTIMIZE: quickly reject orthogonal back sides.
    // Angles here increase to the left.
    angle1 = R_PointToAngle (lineseg->v1->x, lineseg->v1->y); // left
    angle2 = R_PointToAngle (lineseg->v2->x, lineseg->v2->y); // right

    // Clip to view edges.
    span = angle1 - angle2;  // normally span > 0, (angle1 > angle2)

    // Back side? I.e. backface culling?
    if (span >= ANG180)
        return;

    // Global angle needed by segcalc.
    rw_angle1 = angle1;
    // view relative is left 0x20000000, middle 0, right 0xe0000000
    angle1 -= viewangle;
    angle2 -= viewangle;

    // Ranges (from sample)
    // angle1 = 0x7fd4xxxx to -0x7fdcxxxx  (+179 to -179 degrees)
    // angle2 = 0x7ff3xxxx to -0x7fdcxxxx  (+179 to -179 degrees)
    // span = +1 to +179 degrees
    
    // angle1, angle2 valid range from ANG270 to ANG90, unsigned.
    // Because of angle wrap, must contrive tests away from 0.
    // Similar to PrBoom: Bias the test range to give the maximum overrange for the test,
    // Arrange for all the overrange to be on the side being rejected.
    // Valid -clipangle to +clipangle, overrange +clipangle to 360-clipangle.
    if ((clipangle + angle1) > clipangle_x_2) // (angle1 > clipangle)
    {
        // Totally off the left edge?
        if ((angle1 - clipangle) >= span)  // (angle1 - clipangle) >= (angle1 - angle2)
            return;    // angle2 >= clipangle

        angle1 = clipangle;
    }
    // Valid -clipangle to +clipangle, overrange -clipangle to clipangle-360.
    if ((clipangle - angle2) > clipangle_x_2)  // (angle2 < -clipangle)
    {
        // Totally off the right edge?
        if ((-angle2 - clipangle) >= span)  //  (-angle2 - clipangle) >= (angle1 - angle2)
            return;    // angle1 <= -clipangle

        angle2 = -clipangle;
    }

    // The seg is in the view range, but not necessarily visible.
    // angle1, angle2 range is left 0x20000000, middle 0, right 0xe0000000
    x1 = viewangle_to_x[ ANGLE_TO_FINE(angle1+ANG90) ];  // left
    x2 = viewangle_to_x[ ANGLE_TO_FINE(angle2+ANG90) ];  // right

    // [WDJ] This is where PrBoom fixes gaps in OpenGL, by adding segs.
    // Does not cross a pixel?
    if (x1 >= x2)  //SoM: 3/17/2000: Killough said to change the == to >= for... "robustness"?
        return;

    backsector = lineseg->backsector;

    // Single sided line?
    if (!backsector)
        goto clipsolid;

    backsector = R_FakeFlat(backsector, &tempsec, true, /*OUT*/ NULL, NULL );

    // Closed door.
    if (backsector->ceilingheight <= frontsector->floorheight
        || backsector->floorheight >= frontsector->ceilingheight)
    {
        // Rare, misalignment of openings.
        doorclosed = 0; //SoM: 3/25/2000
        goto clipsolid;
    }

    //SoM: 3/25/2000: Check for automap fix. Store in doorclosed for r_segs.c
    doorclosed = R_DoorClosed();
    if (doorclosed)  goto clipsolid;

    // Window.
    if (backsector->ceilingheight != frontsector->ceilingheight
        || backsector->floorheight != frontsector->floorheight)
        goto clippass;


    // Reject empty lines used for triggers
    //  and special events.
    // Identical floor and ceiling on both sides,
    // identical light levels on both sides,
    // and no middle texture.
    if (backsector->ceilingpic == frontsector->ceilingpic
        && backsector->floorpic == frontsector->floorpic
        && backsector->lightlevel == frontsector->lightlevel
        && curline->sidedef->midtexture == 0

        //SoM: 3/22/2000: Check offsets too!
        && backsector->floor_xoffs == frontsector->floor_xoffs
        && backsector->floor_yoffs == frontsector->floor_yoffs
        && backsector->ceiling_xoffs == frontsector->ceiling_xoffs
        && backsector->ceiling_yoffs == frontsector->ceiling_yoffs

        //SoM: 3/17/2000: consider altered lighting
        && backsector->floorlightsec == frontsector->floorlightsec
        && backsector->ceilinglightsec == frontsector->ceilinglightsec
        //SoM: 4/3/2000: Consider colormaps
        && backsector->extra_colormap == frontsector->extra_colormap
        && ((!frontsector->ffloors && !backsector->ffloors) ||
           (frontsector->tag == backsector->tag)))
    {
        return;
    }


  clippass:
    R_ClipPassWallSegment (x1, x2-1);
    return;

  clipsolid:
    R_ClipSolidWallSegment (x1, x2-1);
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
//   | 0 | 1 | 2
// --+---+---+---
// 0 | 0 | 1 | 2
// 1 | 4 | 5 | 6
// 2 | 8 | 9 | A
int     checkcoord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},       // UNUSED
    {2,0,2,1},
    {0},       // UNUSED
    {3,1,3,0},
    {0},       // UNUSED
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};


boolean R_CheckBBox (fixed_t*   bspcoord)
{
    int      boxpos;
    fixed_t  x1, y1, x2, y2;
    angle_t  angle1, angle2;
    angle_t  span;
    cliprange_t*        start;
    int      sx1, sx2;

    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (viewx <= bspcoord[BOXLEFT])
        boxpos = 0;
    else if (viewx < bspcoord[BOXRIGHT])
        boxpos = 1;
    else
        boxpos = 2;

    if (viewy >= bspcoord[BOXTOP])
        boxpos |= 0;
    else if (viewy > bspcoord[BOXBOTTOM])
        boxpos |= 1<<2;
    else
        boxpos |= 2<<2;

    if (boxpos == 5)
        return true;

    x1 = bspcoord[checkcoord[boxpos][0]];
    y1 = bspcoord[checkcoord[boxpos][1]];
    x2 = bspcoord[checkcoord[boxpos][2]];
    y2 = bspcoord[checkcoord[boxpos][3]];

    // check clip list for an open space
    // Angles here increase to the left.
    angle1 = R_PointToAngle (x1, y1) - viewangle;  // left
    angle2 = R_PointToAngle (x2, y2) - viewangle;  // right

    span = angle1 - angle2;  // normally span > 0, (angle1 > angle2)

    // Sitting on a line?
    if (span >= ANG180)
        return true;

    // angle1, angle2 may range from ANG270 to ANG90, unsigned.
    // Because of angle wrap, must contrive tests away from 0.
    if ((clipangle + angle1) > clipangle_x_2) // (angle1 > clipangle)
    {
        // Totally off the left edge?
        if ((angle1 - clipangle) >= span)  // (angle1 - clipangle) >= (angle1 - angle2)
            return false;    // angle2 >= clipangle

        angle1 = clipangle;
    }
    if ((clipangle - angle2) > clipangle_x_2)  // (angle2 < -clipangle)
    {
        // Totally off the right edge?
        if ((-angle2 - clipangle) >= span)  //  (-angle2 - clipangle) >= (angle1 - angle2)
            return false;    // angle1 <= -clipangle

        angle2 = -clipangle;
    }


    // Find the first clippost that touches the source post
    //  (adjacent pixels are touching).
    sx1 = viewangle_to_x[ ANGLE_TO_FINE(angle1 + ANG90) ];
    sx2 = viewangle_to_x[ ANGLE_TO_FINE(angle2 + ANG90) ];

    // Does not cross a pixel.
    if (sx1 >= sx2)
        return false;
    sx2--;

    start = solidsegs;
    while (start->last < sx2)
        start++;

    if (sx1 >= start->first
        && sx2 <= start->last)
    {
        // The clippost contains the new span.
        return false;
    }

    return true;
}



//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//

// First seg of subsector. It has the backscale for the plane.
drawseg_t *  first_subsec_seg;

// Called by R_RenderBSPNode
static
void R_Subsector ( uint32_t num )
{
    static sector_t     tempsec; //SoM: 3/17/2000: Deep water hack

    int                 segcount;
    seg_t*              lineseg;
    subsector_t*        sub;
    sector_t *          ssector;  // sector of the sub
    sector_t *          vsector;  // visible sector, may be ssector, or tempsec
      // [WDJ] Do not use frontsector, as that forces the optimizer to assume that
      // it is being passed to some other function, so it cannot use registers.
    lightlev_t          floor_lightlevel;
    lightlev_t          ceiling_lightlevel;
    extracolormap_t*    floor_colormap;
    extracolormap_t*    ceiling_colormap;
    ff_light_t *        ff_light;  // lightlist index

#ifdef RANGECHECK
    if (num>=numsubsectors)
        I_Error ("R_Subsector: ss %u with numss = %u",
                 num,
                 numsubsectors);
#endif

    //faB: subsectors added at run-time
    if (num>=numsubsectors)
        return;

#ifdef DEBUG_SUBSECTOR
  printf( "BSP: subsec=%i\n", num );
#endif

    sscount++;
    sub = &subsectors[num];
    ssector = sub->sector;  // for working on sector
    segcount = sub->numlines;
    lineseg = &segs[sub->firstline];

    //SoM: 3/17/2000: Deep water/fake ceiling effect.
    vsector = R_FakeFlat(ssector, &tempsec, false,
                             /*OUT*/ &floor_lightlevel, &ceiling_lightlevel );
    // [WDJ] vsector is the visible sector.
    // It may be ssector, or may be a modified copy of ssector (tempsec).

    floor_colormap = ceiling_colormap = vsector->extra_colormap;

    // SoM: Check and prep all 3D floors. Set the sector floor/ceiling light
    // levels and colormaps.
    if(vsector->ffloors)
    {
#ifdef SECTOR_FLAGS
      if( vsector->flags & SCF_floor_moved ) // floor or ceiling moved, must refresh
#else
      if(vsector->moved) // floor or ceiling moved, must refresh
#endif
      {
        vsector->numlights = ssector->numlights = 0;
        R_Prep3DFloors(vsector);  // refresh light lists
        // This assumes that updating ssector with vsector's Prep3DFloors data, is correct.
        // This only works because where it is not correct, it is also not visible.
        // So that multiple sector DeepWater that moved get updated, once.
        ssector->lightlist = vsector->lightlist;
        ssector->numlights = vsector->numlights;
#ifdef SECTOR_FLAGS
        ssector->flags &= ~(SCF_floor_moved); // clear floor moved flags
        vsector->flags &= ~(SCF_floor_moved);
#else
        ssector->moved = vsector->moved = false;  // clear until next move
#endif
      }

      ff_light = R_GetPlaneLight(vsector, vsector->floorheight);
      if(vsector->floorlightsec == -1)
        floor_lightlevel = *ff_light->lightlevel;
      floor_colormap = ff_light->extra_colormap;

      ff_light = R_GetPlaneLight(vsector, vsector->ceilingheight);
      if(vsector->ceilinglightsec == -1)
        ceiling_lightlevel = *ff_light->lightlevel;
      ceiling_colormap = ff_light->extra_colormap;
    }

    ssector->extra_colormap = vsector->extra_colormap;

    if ((vsector->floorheight < viewz)
        || (vsector->model > SM_fluid
            && sectors[vsector->modelsec].ceilingpic == sky_flatnum))
    {
        // visplane global parameter
        vsp_floorplane = R_FindPlane (vsector->floorheight,
                                  vsector->floorpic,
                                  floor_lightlevel,
                                  vsector->floor_xoffs,
                                  vsector->floor_yoffs,
                                  floor_colormap,
                                  NULL);
    }
    else
        vsp_floorplane = NULL;

    if ((vsector->ceilingheight > viewz)
        || (vsector->ceilingpic == sky_flatnum)
        || (vsector->model > SM_fluid
            && sectors[vsector->modelsec].floorpic == sky_flatnum))
    {
        // visplane global parameter
        vsp_ceilingplane = R_FindPlane (vsector->ceilingheight,
                                    vsector->ceilingpic,
                                    ceiling_lightlevel,
                                    vsector->ceiling_xoffs,
                                    vsector->ceiling_yoffs,
                                    ceiling_colormap,
                                    NULL);
    }
    else
        vsp_ceilingplane = NULL;


    // BSP reuses the ffplane array for each sector, to pass info to Render
    numffplane = 0;
    ffplane[numffplane].plane = NULL;
    first_subsec_seg = NULL;

    if(vsector->ffloors)
    {
      ffloor_t*  fff;

      for(fff = vsector->ffloors; fff; fff = fff->next)
      {

        if(!(fff->flags & (FF_OUTER_PLANES|FF_INNER_PLANES))
           || !(fff->flags & FF_EXISTS))
          continue;

        ffplane[numffplane].plane = NULL;

        fixed_t ffbh = *fff->bottomheight;
        if( ffbh <= vsector->ceilingheight
         && ffbh >= vsector->floorheight
#if 1	    
         && ((viewz < ffbh && (fff->flags & FF_OUTER_PLANES))
             || (viewz > ffbh && (fff->flags & FF_INNER_PLANES))) )
#else	   
         // [WDJ] What about (viewz == ffbh) ???
         && ((viewz <= ffbh && (fff->flags & FF_OUTER_PLANES))
             || (viewz >= ffbh && (fff->flags & FF_INNER_PLANES))) )
#endif
        {
          ff_light = R_GetPlaneLight_viewz(vsector, ffbh);
          ffplane[numffplane].plane =
            R_FindPlane(ffbh,
                        *fff->bottompic,
                        *ff_light->lightlevel,
                        *fff->bottomxoffs,
                        *fff->bottomyoffs,
                        ff_light->extra_colormap,
                        fff);

          ffplane[numffplane].height = ffbh;
          ffplane[numffplane].ffloor = fff;
          numffplane++;
          if(numffplane >= MAXFFLOORS)
            break;
        }

        fixed_t ffth = *fff->topheight;
        if( ffth >= vsector->floorheight
         && ffth <= vsector->ceilingheight
#if 1	    
         && ((viewz > ffth && (fff->flags & FF_OUTER_PLANES))
             || (viewz < ffth && (fff->flags & FF_INNER_PLANES))))
#else	   
         // [WDJ] What about (viewz == ffth) ???
         && ((viewz >= ffth && (fff->flags & FF_OUTER_PLANES))
            || (viewz <= ffth && (fff->flags & FF_INNER_PLANES))))
#endif
        {
          ff_light = R_GetPlaneLight_viewz(vsector, ffth);
          ffplane[numffplane].plane =
            R_FindPlane(ffth,
                        *fff->toppic,
                        *ff_light->lightlevel,
                        *fff->topxoffs,
                        *fff->topyoffs,
                        ff_light->extra_colormap,
                        fff);

          ffplane[numffplane].height = ffth;
          ffplane[numffplane].ffloor = fff;
          numffplane++;
          if(numffplane >= MAXFFLOORS)
            break;
        }
      }
    }

    frontsector = vsector;  // parameter of R_AddSprites and R_AddLine.

#ifdef FLOORSPLATS
    if( sub->splats && cv_splats.EV )
        R_AddVisibleFloorSplats (sub);
#endif

    R_AddSprites (ssector, tempsec.lightlevel);

    while (segcount--) // over all lines in the subsector
    {
      R_AddLine (lineseg);
      lineseg++;
    }
}



//
// R_Prep3DFloors
//
// This function creates the lightlists that the given sector uses to light
// floors/ceilings/walls according to the 3D floors.
// Called by R_Subsector whenever a floor has moved
void R_Prep3DFloors(sector_t*  sector)
{
  ffloor_t*      rover;
  ffloor_t*      best;
  fixed_t        bestheight, maxheight;
  int            count, i, mapnum;
  sector_t*      modelsec;
  ff_light_t   * ff_light;

  // count needed lightlist entries
  count = 1;
  for(rover = sector->ffloors; rover; rover = rover->next)
  {
    if((rover->flags & FF_EXISTS)
       && (!(rover->flags & FF_NOSHADE)
           || (rover->flags & (FF_CUTSOLIDS|FF_CUTEXTRA|FF_CUTSPRITES))) )
    {
      count++;
      if(rover->flags & FF_SLAB_SHADOW)  // uses two entries
        count++;
    }
  }

  if(count != sector->numlights)
  {
    // Allocate the fake-floor light list.
    if(sector->lightlist)
      Z_Free(sector->lightlist);
    sector->lightlist = Z_Malloc(sizeof(ff_light_t) * count, PU_LEVEL, 0);
    sector->numlights = count;
  }
  // clear lightlist 
  memset(sector->lightlist, 0, sizeof(ff_light_t) * count);

  // init [0] to sector light
  ff_light = & sector->lightlist[0];
  ff_light->height = sector->ceilingheight + 1;
  ff_light->lightlevel = &sector->lightlevel;
  ff_light->caster = NULL;
  ff_light->extra_colormap = sector->extra_colormap;
  ff_light->flags = 0;

  // Work down from highest light to lowest light.
  // Determine each light in lightlist.
  maxheight = FIXED_MAX;  // down from max, previous light
  for(i = 1; i < count; i++)
  {
    ff_light = & sector->lightlist[i];
    bestheight = -FIXED_MAX;
    best = NULL;
    for(rover = sector->ffloors; rover; rover = rover->next)
    {
      if(!(rover->flags & FF_EXISTS)
         || ((rover->flags & FF_NOSHADE)
             && !(rover->flags & (FF_CUTSOLIDS|FF_CUTEXTRA|FF_CUTSPRITES))) )
        continue;

      // find highest topheight, lower than maxheight
      if(*rover->topheight > bestheight && *rover->topheight < maxheight)
      {
        best = rover;
        bestheight = *rover->topheight;
        continue;
      }
      // FF_SLAB_SHADOW considers bottomheight too, light limited to slab
      if(rover->flags & FF_SLAB_SHADOW
         && *rover->bottomheight > bestheight && *rover->bottomheight < maxheight)
      {
        best = rover;
        bestheight = *rover->bottomheight;
        continue;
      }
    }
    if(!best)  // failure escape
    {
      sector->numlights = i;
      return;
    }

    ff_light->height = maxheight = bestheight;
    ff_light->caster = best;
    ff_light->flags = best->flags;

    // Setup the model sector extra_colormap
    // this could be done elsewhere, once.
    // (P_LoadSideDefs2, P_SpawnSpecials, SF_SectorColormap)
    modelsec = &sectors[best->model_secnum];
    mapnum = modelsec->midmap;
    if(mapnum >= 0 && mapnum < num_extra_colormaps)
      modelsec->extra_colormap = &extra_colormaps[mapnum];
    else
      modelsec->extra_colormap = NULL;

    // best is highest floor less than maxheight
    if(best->flags & FF_NOSHADE)
    {
      // FF_NOSHADE, copy next higher light
      ff_light->lightlevel = sector->lightlist[i-1].lightlevel;
      ff_light->extra_colormap = sector->lightlist[i-1].extra_colormap;
    }
    else
    {
      // usual light
      ff_light->lightlevel = best->toplightlevel;
      ff_light->extra_colormap = modelsec->extra_colormap;
    }

    if(best->flags & FF_SLAB_SHADOW)
    {
      // FF_SLAB_SHADOW, consider bottomheight too.
      // Below this slab, the light is from above this slab.
      if(bestheight == *best->bottomheight)
      {
        // [WDJ] segfault here in Chexquest-newmaps E2M2, best->lastlight wild value
        // Stopped segfault by init to 0.
        // Happens when bottom is found without finding top.
        // Get from lastlight indirect.
        ff_light->lightlevel = sector->lightlist[best->lastlight].lightlevel;
        ff_light->extra_colormap = sector->lightlist[best->lastlight].extra_colormap;
      }
      else
      {
        // Slab light does not show below slab, indirect to light above slab
        best->lastlight = i - 1;
      }
    }
  }
}


// Find light under planeheight, plain version
// Return a fake-floor light.
ff_light_t *  R_GetPlaneLight(sector_t* sector, fixed_t planeheight)
{
  ff_light_t * light1 = & sector->lightlist[1];  // first fake-floor

  // [0] is sector light, which is above all
  // lightlist is highest first
  for( ; light1 < & sector->lightlist[sector->numlights]; light1++)
  {
    if( light1->height <= planeheight)  break;
  }

  return light1 - 1;  // return previous light
}


// Find light under planeheight, slight difference according to viewz
// Return a fake-floor light.
ff_light_t *  R_GetPlaneLight_viewz(sector_t* sector, fixed_t  planeheight)
{
  ff_light_t * light1 = & sector->lightlist[1];  // first fake-floor
  ff_light_t * lightend = & sector->lightlist[sector->numlights];  // past last

#if 0
  // faster
  if( viewz < planeheight )
  {
      for( ; light1 < lightend; light1++)
        if(light1.height < planeheight)   goto found;
  }
  else
  {
      for( ; light1 < lightend; light1++)
        if(light1.height <= planeheight)   goto found;
  }
#else
  // smaller
  if( viewz >= planeheight )
     return R_GetPlaneLight( sector, planeheight );
   
  for( ; light1 < lightend; light1++)
        if(light1->height < planeheight)   goto found;
#endif
  // not found
  return light1 - 1;  // last light in list

found:     
  return light1 - 1;  // previous light
}



//
// RenderBSPNode
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
// Called by R_RenderPlayerView

#if 0
// [WDJ] Pull the degenerate case out of the main BSP loop.
// It can only be invoked at the first call.
// Can get same effect by calling
// R_RenderBSPNode( 0 | NF_SUBSECTOR )
void R_RenderBSPNode_degen (int bspnum)
{
    if (bspnum == -1)
    {
        // BP: never happen : bspnum = int, children = unsigned short
        // except first call if numsubsectors=0 ! who care ?
        R_Subsector (0);
    }
}
#endif


#if 0
// Recursive
//  bspnum : children[]
//  Call R_RenderBSPNode_degen for degenerate case.
void R_RenderBSPNode ( bsp_child_t bspnum )
{
    node_t*     bsp;
    int         side;

    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
    {
        R_Subsector (bspnum&(~NF_SUBSECTOR));
        return;
    }

    bsp = &nodes[bspnum];

    // Decide which side the view point is on.
    side = R_PointOnSide_Render (viewx, viewy, bsp);  // [Arcade]

    // Recursively divide front space.
    R_RenderBSPNode (bsp->children[side]);

    // Possibly divide back space.
    if (R_CheckBBox (bsp->bbox[side^1]))
        R_RenderBSPNode (bsp->children[side^1]);
}
#endif

#if 1
// Recursive and Loop, as in PrBoom.
//  bspnum : children[]
//  Call R_RenderBSPNode_degen for degenerate case.
void R_RenderBSPNode ( bsp_child_t bspnum )
{
    node_t*     bsp;
    int         side;

  for(;;)
  {
    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
    {
        R_Subsector (bspnum&(~NF_SUBSECTOR));
        return;
    }

    bsp = &nodes[bspnum];

    // Decide which side the view point is on.
    side = R_PointOnSide_Render (viewx, viewy, bsp);  // [Arcade]

    // Recursively divide front space.
    R_RenderBSPNode (bsp->children[side]);

    // Possibly divide back space.
    if( ! R_CheckBBox (bsp->bbox[side^1]))
        return;

    // Use loop to do back space, instead of calling R_RenderBSPNode recursively.
    bspnum = bsp->children[side^1];
  }
}
#endif


#if 0
//
// RenderBSPNode : DATA RECURSION version, slower :<
//
// Denis.F. 03-April-1998 : I let this here for learning purpose
//                          but this is slower coz PGCC optimises
//                          fairly well the recursive version.
//                          (it was clocked with p5prof)
//
#define MAX_BSPNUM_PUSHED 512

// Stack based descent
//  bspnum : children[]
//  Call R_RenderBSPNode_degen for degenerate case.
void R_RenderBSPNode (bsp_child_t bspnum)
{
    node_t*     bsp;
    int         side;

    node_t      *bspstack[MAX_BSPNUM_PUSHED];
    node_t      **bspnum_p;

    //int         visited=0;

    bspstack[0] = NULL;
    bspstack[1] = NULL;
    bspnum_p = &bspstack[2];

    // Recursively divide front space.
    for (;;)
    {
        // Recursively divide front space.
        while (!(bspnum & NF_SUBSECTOR))
        {
            bsp = &nodes[bspnum];

            // Decide which side the view point is on.
            side = R_PointOnSide_Render (viewx, viewy, bsp);  // [Arcade]

            *bspnum_p++ = bsp;
            *bspnum_p++ = (void*) side;
            bspnum = bsp->children[side];
        }

        // Found a subsector
        R_Subsector (bspnum&(~NF_SUBSECTOR));

        side = (int) *--bspnum_p;
        if ((bsp = *--bspnum_p) == NULL )
        {
            // we're done
            //debug_Printf("Subsectors visited: %d\n", visited);
            return;
        }

        // Possibly divide back space.
        if (R_CheckBBox (bsp->bbox[side^1]))
            // dirty optimisation here!! :) double-pop done because no push!
            bspnum = bsp->children[side^1];
    }

}
#endif

