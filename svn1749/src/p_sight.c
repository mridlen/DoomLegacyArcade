// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_sight.c 1735 2025-03-14 08:14:39Z wesleyjohnson $
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
// $Log: p_sight.c,v $
// Revision 1.6  2003/03/22 22:35:58  hurdler
//
// Revision 1.5  2002/09/27 16:40:09  tonyd
// First commit of acbot
//
// Revision 1.4  2001/06/10 21:16:01  bpereira
//
// Revision 1.3  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      LineOfSight/Visibility checks, uses REJECT Lookup Table.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "doomstat.h"
#include "p_local.h"
#include "r_state.h"
#include "r_main.h"

//
// P_CheckSight
//
// Check Sight global return vars, also used by P_AimLineAttack
fixed_t         see_topslope;
fixed_t         see_bottomslope;   // slopes to top and bottom of target

// Check Sight internal global vars
static divline_t       cs_trace;        // from t1 to t2
static subsector_t *   cs_t2_subsector; // location of t2
static fixed_t         cs_t2x, cs_t2y;
static fixed_t         cs_startz;       // eye z of looker

static int             cs_sightcounts[2];  // ??? debugging


//
// P_DivlineSide
// Returns side 0 (front), 1 (back), or 2 (on).
//
static int P_DivlineSide( fixed_t x, fixed_t y, divline_t* node )
{
    if (!node->dx)
    {
        if (x == node->x)
            return 2;

        if (x <= node->x)
            return node->dy > 0;

        return node->dy < 0;
    }

    if (!node->dy)
    {
        // [WDJ] Fix "Sleeping Sargeant" bug from DoomWiki.       
        if( (EN_sleeping_sarg_bug ? x : y) == node->y )
            return 2;

        if (y <= node->y)
            return node->dx < 0;

        return node->dx > 0;
    }

    fixed_t left =  (node->dy>>FRACBITS) * ((x - node->x)>>FRACBITS);
    fixed_t right = ((y - node->y)>>FRACBITS) * (node->dx>>FRACBITS);

    if (right < left)
        return 0;       // front side

    if (left == right)
        return 2;
    return 1;           // back side
}


//
// P_InterceptVector2
// Returns the fractional intercept point along the first divline.
//
static fixed_t P_InterceptVector2( divline_t* v2, divline_t* v1 )
{
    fixed_t     num;
    fixed_t     den;

    den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

    if (den == 0)
        return 0;  // parallel

    num = FixedMul ( (v1->x - v2->x)>>8 ,v1->dy) +
        FixedMul ( (v2->y - v1->y)>>8 , v1->dx);

    return  FixedDiv (num , den);  // frac
}


static fixed_t  prev_frac;  // sight trace frac from last linedef crossing

// Clip the sight line with the solid 3dfloors.
// At the position along the cs_trace sight line, at frac.
static void intercept_ffloor( subsector_t * ssec, fixed_t frac )
{
    fixed_t  t_slope, b_slope;
    fixed_t  clip_frac_t, clip_frac_b;
    fixed_t  th, bh;
    ffloor_t * ff;  // fake floor

    // This was not present for any previous version.
    if( EV_legacy < 145 )  return;

    // This ignores blocking floors that are not solid, and only maintains
    // one sight gap through the 3d floors.
    // Lesser sight gaps are treated as blocked.
    // It will clip the top and bottom sight lines.
    for( ff = ssec->sector->ffloors; ff; ff = ff->next)
    {
        if( !(ff->flags & FF_SOLID) )  continue;

        th = *ff->topheight;
        bh = *ff->bottomheight;

        if( cs_startz > th )
        {
           // look down on floor
           clip_frac_t = frac;
           clip_frac_b = prev_frac;
        }
        else if ( cs_startz < bh )
        {
           // look up at floor
           clip_frac_t = prev_frac;
           clip_frac_b = frac;
        }
        else
        {
           // edge of floor at eye height
           clip_frac_t = prev_frac;
           clip_frac_b = prev_frac;
        }
        // Quick rejects.
        if( FixedMul( see_bottomslope, clip_frac_t ) > (th - cs_startz) )
           continue;  // bottom sight is above highest part of floor
        if( FixedMul( see_topslope, clip_frac_b ) < (bh - cs_startz) )
           continue;  // top sight is below lowest part of floor
       
        // Compare the occlusion slopes to the sight line slopes.
        t_slope = FixedDiv (th - cs_startz , clip_frac_t);
        b_slope = FixedDiv (bh - cs_startz , clip_frac_b);

        // Assume t_slope > b_slope.
        // Assume see_topslope > see_bottomslope.
        // Assume b_slope < see_topslope and t_slope > see_bottomslope.
        if( b_slope <= see_bottomslope )
        {
            if( t_slope >= see_topslope )
            {
                // This occurs most often, so make it first test.
                see_bottomslope = FIXED_MAX; // total occlusion
                see_topslope = FIXED_MIN;
                break;
            }
            // (t_slope < see_topslope)
            see_bottomslope = t_slope;  // th clips bottom sight
        }
        else
        {
            // (b_slope > see_bottomslope).
            // If this is simplified there will be overflow math errors in
            // (see_topslope - t_slope) when near the viewer (t_slope -> FIXED_MAX).
            if( t_slope >= see_topslope )
            {
                see_topslope = b_slope;  // bh clips top sight
            }
            else
            {
                // The 3dfloor is in the middle of the sight lines.
                // Trying to clip both top and bottom will loose everything.
                // Keep only the largest gap in the sight lines.
                // Block the 3dfloor and lose the other sight gap.
                if( (see_topslope - t_slope) > (b_slope - see_bottomslope) )
                {
                    see_bottomslope = t_slope;  // th clips bottom sight
                }
                else if( b_slope < see_topslope )
                {
                    see_topslope = b_slope;  // bh clips top sight
                }
            }
        }
        if (see_topslope <= see_bottomslope)  break;  // cannot see
    }
}


//
// P_CrossSubsector
// Returns true
//  if cs_trace crosses the given subsector successfully.
//
static boolean P_CrossSubsector (int num)
{
    seg_t  * seg;
    line_t * line;
    subsector_t * sub;
    sector_t * front, * back;
    vertex_t * v1, * v2;
    fixed_t             opentop, openbottom;
    divline_t           divl;
    fixed_t             frac;  // 1.0
    fixed_t             slope;
    int   s1, s2;
    int   count;

#ifdef RANGECHECK
    if (num>=numsubsectors)
        I_Error ("P_CrossSubsector: ss %i with numss = %i",
                 num, numsubsectors);
#endif

    sub = &subsectors[num];
    if( sub == cs_t2_subsector )
    {
        // This is the subsector that t2 is within, so there cannot be
        // any new linedef crossings.
        if( sub->sector->ffloors )
        {
            // Clip for the 3dfloors.
            intercept_ffloor( sub, FRACUNIT );

            if (see_topslope <= see_bottomslope)
               return false;               // stop
        }

        // [WDJ] Fix demo sync problems
        // The return skips some significant sight tests.
        if( EV_legacy >= 145 && EV_legacy < 147 )
            return true;
    }

    // check lines
    count = sub->numlines;
    seg = &segs[sub->firstline];

    for ( ; count ; seg++, count--)
    {
        line = seg->linedef;

        // already checked other side?
        if (line->validcount == validcount)
            continue;

        line->validcount = validcount;
       
        // [WDJ] PrBoom does bounding box check here, but it causes the
        // original Doom demos to lose sync. We also lose sync because
        // of the Sleeping Sargent bug fix in P_DivlineSide.

        // Check the vertex of the line segment against the sight trace.
        v1 = line->v1;
        v2 = line->v2;
        s1 = P_DivlineSide (v1->x, v1->y, &cs_trace);
        s2 = P_DivlineSide (v2->x, v2->y, &cs_trace);

        // line isn't crossed?
        if (s1 == s2)
            continue;  // both vertex of the linedef are on same side of cs_trace

        divl.x = v1->x;
        divl.y = v1->y;
        divl.dx = v2->x - v1->x;
        divl.dy = v2->y - v1->y;
        s1 = P_DivlineSide (cs_trace.x, cs_trace.y, &divl);
        s2 = P_DivlineSide (cs_t2x, cs_t2y, &divl);

        // line isn't crossed?
        if (s1 == s2)
            continue; // both vertex of the trace are on same side of the linedef

        // stop because it is not two sided anyway
        // might do this after updating validcount?
        if ( !(line->flags & ML_TWOSIDED) )
            return false;

        // crosses a two sided line
        front = seg->frontsector;
        back = seg->backsector;

        // no wall to block sight with?
        if (front->floorheight == back->floorheight
            && front->ceilingheight == back->ceilingheight)
            continue;

        // possible occluder
        // because of ceiling height differences
        opentop = min( front->ceilingheight, back->ceilingheight );

        // because of floor height differences
        openbottom = max( front->floorheight, back->floorheight );

        // quick test for totally closed doors
        if (openbottom >= opentop)
            return false;               // stop
       
        // PrBoom: test against minz, maxz, here.

        // PrBoom: InterceptVector2 only for PrBoom5 or PrBoom6 compatibility.
        // The PrBoom P_InterceptVector is 64 bit, except when < PrBoom4.
        //  PrBoom 3 and before: P_InterceptVector2 (32bit)
        //  PrBoom 4: P_InterceptVector (64bit)
        //  PrBoom 5, 6: P_InterceptVector2 (32bit)
        // EternityEngine: only use InterceptVector2
        // DoomLegacy : P_InterceptVector and P_InterceptVector2 are identical.
        // Test the sight lines across this linedef segment.
        // Fraction of the sight trace covered.
#if 0
        frac = ((EV_legacy > 0)?
                P_InterceptVector2(&cs_trace, &divl)
                : ( (demoversion < 212)?  // < prboom 4
                    P_InterceptVector2(&cs_trace, &divl)
                    : (demoversion >= 213 && demoversion <= 214)?  // prboom 5, prboom 6
                    P_InterceptVector2(&cs_trace, &divl)
                    : P_InterceptVector_64(&cs_trace, &divl)
                  )

#else
        frac = P_InterceptVector2 (&cs_trace, &divl);
#endif

        if (front->floorheight != back->floorheight)
        {
            slope = FixedDiv (openbottom - cs_startz , frac);
            if (slope > see_bottomslope)
                see_bottomslope = slope;
        }

        if (front->ceilingheight != back->ceilingheight)
        {
            slope = FixedDiv (opentop - cs_startz , frac);
            if (slope < see_topslope)
                see_topslope = slope;
        }

        if (see_topslope <= see_bottomslope)
            return false;               // stop

        if( sub->sector->ffloors )
        {
            // Clip for the 3dfloors.
            intercept_ffloor( sub, frac );

            if (see_topslope <= see_bottomslope)
               return false;               // stop
        }
        prev_frac = frac;
    }

    // passed the subsector ok
    return true;
}



//
// P_CrossBSPNode
// Returns true
//  if cs_trace crosses the given node successfully.
//
static boolean P_CrossBSPNode (int bspnum)
{
  node_t * bsp;
  int      side;

  // [WDJ] Remove tail recursion, similar to PrBoom.
  while( !(bspnum & NF_SUBSECTOR) )
  {
    bsp = &nodes[bspnum];

    // decide which side the start point is on
    side = P_DivlineSide (cs_trace.x, cs_trace.y, (divline_t *)bsp);
    side &= 0x01;  // 2 ==> 0, an "on" should cross both sides

    // [WDJ] As in PrBoom and EternityEngine.
    // The partition plane is crossed here.
    if( side == P_DivlineSide( cs_t2x, cs_t2y, (divline_t *)bsp) )
    {
        // cross the starting side, using loop
        bspnum = bsp->children[side]; // the line doesn't touch the other side
    }
    else
    {
        // the partition plane is crossed here
        if( !P_CrossBSPNode( bsp->children[side]) )
            return false;   // cross the starting side

        // cross the ending side, using loop
        bspnum = bsp->children[side^1];
    }
  }

  if( bspnum == -1 )
      return P_CrossSubsector (0);
   
  return P_CrossSubsector( bspnum & ~NF_SUBSECTOR );
}


#define FIX_SIGHT_HEIGHT  1
//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
boolean P_CheckSight( mobj_t* t1, mobj_t* t2 )
{
    const sector_t * s1p = t1->subsector->sector;
    const sector_t * s2p = t2->subsector->sector;

    // First check for trivial rejection.

    // Determine subsector entries in REJECT table.
    uint32_t s1 = (s1p - sectors);
    uint32_t s2 = (s2p - sectors);
    if( rejectmatrix && ((s1|s2) & 0xFFFF0000L) == 0 )  // sector indexes are reasonable.
    {
        uint64_t pnum = s1*((uint64_t)numsectors) + s2;
        uint32_t bytenum = pnum>>3;
        // [WDJ] Have added num_rejectmatrix, to protect against segfault.
        // Also handle when no rejectmatrix.
        if( bytenum < num_rejectmatrix )
        {
            uint32_t bitnum = 1 << (pnum&7);
            // Check in REJECT table.
            if (rejectmatrix[bytenum]&bitnum)
            {
                cs_sightcounts[0]++;

                // can't possibly be connected
                goto ret_false;
            }
        }
    }

    // [WDJ] From PrBoom
    // Uses model and modelsec, instead of the PrBoom heightsec.
    // killough 4/19/98: make fake floors and ceilings block monster view
    if( s1p->model > SM_fluid )
    {
        s1p = &sectors[s1p->modelsec];

        // Test: cannot see when t1 above floor, and t2 below floor.
        if( t1->z + t1->height <= s1p->floorheight
            && t2->z >= s1p->floorheight )
            goto ret_false;

        // Test: cannot see when t1 above ceiling, and t2 below ceiling.
        if( t1->z >= s1p->ceilingheight
            && (
#ifdef FIX_SIGHT_HEIGHT
             // [WDJ] entire t1 is below ceiling
             // t2->z + t2->height
             t2->z + ((EV_legacy >= 147)? t2->height : t1->height)
#else
             // PrBoom orig, adds t2 to t1 height.
             t2->z + t1->height 
#endif
                  <= s1p->ceilingheight ) )
            goto ret_false;
    }
  
    if( s2p->model > SM_fluid )
    { 
        s2p = &sectors[s2p->modelsec];

        // Test: cannot see when t2 below floor, and t1 above floor.
        if( t2->z + t2->height <= s2p->floorheight
            && t1->z >= s2p->floorheight )
            goto ret_false;

        // Test: cannot see when t2 above ceiling, and t1 below ceiling.
        if( t2->z >= s2p->ceilingheight
            && (
#ifdef FIX_SIGHT_HEIGHT
               // [WDJ] entire t1 is below ceiling
               // t1->z + t1->height
               t1->z + ((EV_legacy >= 147)? t1->height : t2->height)
#else
               // PrBoom orig, adds t1 to t2 height.
               t1->z + t2->height 
#endif
                  <= s2p->ceilingheight ) )
            goto ret_false;
    }

#if 0
//  BP: it seem that it don't work :( TODO: fix it
    if (EN_heretic )
    {
        //
        // check precisely
        //              
        cs_startz = t1->z + t1->height - (t1->height>>2);
        see_topslope = (t2->z+t2->height) - cs_startz;
        see_bottomslope = (t2->z) - cs_startz;
        
        return P_SightPathTraverse ( t1->x, t1->y, t2->x, t2->y );
    }
#endif

    // [WDJ] MBF, From MBF, PrBoom.
    // killough 11/98: shortcut for melee situations.
    // same subsector? obviously visible
    // cph - compatibility optioned for demo sync, cf HR06-UV.LMP
    if( EN_mbf && (t1->subsector == t2->subsector) )
        goto ret_true;

    // An unobstructed LOS is possible.
    // Now look from eyes of t1 to any part of t2.
    cs_sightcounts[1]++;

    validcount++;

    cs_startz = t1->z + t1->height - (t1->height>>2);  // eyes at 3/4
    // Slope is (height / horz.), where horz. is measured such that the
    // the distance from eyes to target = 1.  Slope here is (height/1).
    see_bottomslope = t2->z - cs_startz;           // feet of target
    see_topslope = see_bottomslope + t2->height;   // head of target

    cs_trace.x = t1->x;
    cs_trace.y = t1->y;
    cs_t2x = t2->x;
    cs_t2y = t2->y;
    cs_trace.dx = t2->x - t1->x;
    cs_trace.dy = t2->y - t1->y;
    cs_t2_subsector = t2->subsector;  // location of t2
   
    // Setup for 3dfloor sight tests
    prev_frac = 0;

    // the head node is the last node output
    return P_CrossBSPNode (numnodes-1);

ret_true:
    return true;
   
ret_false:
    return false;
}

//	added by AC for missle prediction
// Only called by bots, and when cv_predicting_monsters is on.
// P_CheckSight2
// Returns true
//  if a straight line between t1 and t2's predicted location is unobstructed.
// Uses REJECT.
//
boolean P_CheckSight2( mobj_t* t1, mobj_t* t2, fixed_t px, fixed_t py, fixed_t pz )
{
    const sector_t * s1p = t1->subsector->sector;
    const sector_t * s2p = t2->subsector->sector;

    // First check for trivial rejection.

    // Determine subsector entries in REJECT table.
    uint32_t s1 = (s1p - sectors);
    uint32_t s2 = (s2p - sectors);
    if( rejectmatrix && ((s1|s2) & 0xFFFF0000L) == 0 )  // sector indexes are reasonable.
    {
        uint64_t pnum = s1*((uint64_t)numsectors) + s2;
        uint32_t bytenum = pnum>>3;
        // [WDJ] Have added num_rejectmatrix, to protect against segfault.
        // Also handle when no rejectmatrix.
        if( bytenum < num_rejectmatrix )
        {
            uint32_t bitnum = 1 << (pnum&7);
            // Check in REJECT table.
            if (rejectmatrix[bytenum]&bitnum)
            {
                cs_sightcounts[0]++;

                // can't possibly be connected
                goto ret_false;
            }
        }
    }

    // [WDJ] From PrBoom
    // Uses model and modelsec, instead of the PrBoom heightsec.
    // killough 4/19/98: make fake floors and ceilings block monster view
    if( s1p->model > SM_fluid )
    {
        s1p = &sectors[s1p->modelsec];

        // Test: cannot see when t1 above floor, and t2 below floor.
        if( t1->z + t1->height <= s1p->floorheight
            && pz >= s1p->floorheight )
            goto ret_false;

        // Test: cannot see when t1 above ceiling, and t2 below ceiling.
        if( t1->z >= s1p->ceilingheight
            && (
#ifdef FIX_SIGHT_HEIGHT
             // [WDJ] entire t1 is below ceiling
             // t2->z + t2->height
             pz + ((EV_legacy >= 147)? t2->height : t1->height)
#else
             // PrBoom orig, adds t2 to t1 height.
             pz + t1->height 
#endif
                  <= s1p->ceilingheight ) )
            goto ret_false;
    }
  
    if( s2p->model > SM_fluid )
    { 
        s2p = &sectors[s2p->modelsec];

        // Test: cannot see when t2 below floor, and t1 above floor.
        if( pz + t2->height <= s2p->floorheight
            && t1->z >= s2p->floorheight )
            goto ret_false;

        // Test: cannot see when t2 above ceiling, and t1 below ceiling.
        if( pz >= s2p->ceilingheight
            && (
#ifdef FIX_SIGHT_HEIGHT
               // [WDJ] entire t1 is below ceiling
               // t1->z + t1->height
               t1->z + ((EV_legacy >= 147)? t1->height : t2->height)
#else
               // PrBoom orig, adds t1 to t2 height.
               t1->z + t2->height 
#endif
                  <= s2p->ceilingheight ) )
            goto ret_false;
   }

#if 0
//  BP: it seem that it don't work :( TODO: fix it
    if (EN_heretic )
    {
        //
        // check precisely
        //              
        cs_startz = t1->z + t1->height - (t1->height>>2);
        see_topslope = (t2->z+t2->height) - cs_startz;
        see_bottomslope = (t2->z) - cs_startz;
        
        return P_SightPathTraverse ( t1->x, t1->y, t2->x, t2->y );
    }
#endif

    // [WDJ] MBF, From MBF, PrBoom.
    // killough 11/98: shortcut for melee situations.
    // same subsector? obviously visible
    // cph - compatibility optioned for demo sync, cf HR06-UV.LMP
    if( EN_mbf && (t1->subsector == t2->subsector) )
        goto ret_true;     

    // An unobstructed LOS is possible.
    // Now look from eyes of t1 to any part of t2.
    cs_sightcounts[1]++;

    validcount++;

    cs_startz = t1->z + t1->height - (t1->height>>2);
    see_bottomslope = (pz) - cs_startz;           // feet of target
    see_topslope = see_bottomslope + t2->height;   // head of target

    cs_trace.x = t1->x;
    cs_trace.y = t1->y;
    cs_t2x = px;
    cs_t2y = py;
    cs_trace.dx = px - t1->x;
    cs_trace.dy = py - t1->y;

    // Setup for 3dfloor sight tests
    prev_frac = 0;
    
    // the head node is the last node output
    return P_CrossBSPNode (numnodes-1);

ret_true:
    return true;
   
ret_false:
    return false;
}


#ifdef MBF21
// [WDJ] MBF21, required for MBF21, adapted from DSDA-Doom.

// P_CheckFov
// Returns true if t2 is within t1's field of view.
// Not directly related to P_CheckSight, but often
// used in tandem.
//
// Adapted from Eternity, so big thanks to Quasar
//
byte P_Check_FOV( mobj_t* t1, mobj_t* t2, angle_t fov )
{
    angle_t angle = R_PointToAngle2(t1->x, t1->y, t2->x, t2->y);
    angle_t fov2 = fov / 2;
    angle_t minang = t1->angle - fov2;
    angle_t maxang = t1->angle + fov2;

    return((minang > maxang) ?
             (angle >= minang) || (angle <= maxang)
           : (angle >= minang) && (angle <= maxang) );
}
#endif
