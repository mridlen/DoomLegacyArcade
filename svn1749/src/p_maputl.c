// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_maputl.c 1676 2024-03-03 04:42:10Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2011 by DooM Legacy Team.
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
// $Log: p_maputl.c,v $
// Revision 1.16  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.15  2003/06/11 00:28:50  ssntails
// Big Blockmap Support (128kb+ ?)
//
// Revision 1.14  2002/06/30 21:37:48  hurdler
// Ready for 1.32 beta 5 release
//
// Revision 1.13  2001/08/28 20:19:55  hurdler
//
// Revision 1.12  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.11  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.10  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.9  2000/11/02 19:49:35  bpereira
//
// Revision 1.8  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.7  2000/08/31 14:30:55  bpereira
// Revision 1.6  2000/08/11 19:10:13  metzgermeister
// Revision 1.5  2000/04/15 22:12:57  stroggonmeth
// Revision 1.4  2000/04/08 17:29:25  stroggonmeth
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
//      Movement/collision utility functions,
//      as used by function in p_map.c.
//      BLOCKMAP Iterator functions,
//      and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "r_main.h"
#include "p_maputl.h"
#include "doomstat.h"
  // deathmatch

//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t  P_AproxDistance ( fixed_t dx, fixed_t dy )
{
    dx = abs(dx);
    dy = abs(dy);
    if (dx < dy)
        return dx+dy-(dx>>1);
    return dx+dy-(dy>>1);
}


//
// P_PointOnLineSide
// Returns 0 = front side, 1 = back side.
int  P_PointOnLineSide ( fixed_t x, fixed_t y, const line_t* line )
{
    if (!line->dx)
    {
        if (x <= line->v1->x)
            return line->dy > 0;

        return line->dy < 0;
    }
    if (!line->dy)
    {
        if (y <= line->v1->y)
            return line->dx < 0;

        return line->dx > 0;
    }

    register fixed_t  dx = (x - line->v1->x);
    register fixed_t  dy = (y - line->v1->y);
    return ( FixedMul( dy , line->dx>>FRACBITS ) >= FixedMul( line->dy>>FRACBITS , dx ) );
        // 0 = front side, 1 = back side
}



//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
int  P_BoxOnLineSide ( const fixed_t* tmbox, const line_t* ld )
{
    register int  p1;

    // [WDJ] Eliminate some branches, and XOR, in favor of speed.
    switch (ld->slopetype)
    {
      case ST_HORIZONTAL:
        p1 = tmbox[BOXTOP] > ld->v1->y;
        if( p1 == ( tmbox[BOXBOTTOM] > ld->v1->y ) )
        {
            return( p1 ^ (ld->dx < 0) );
        }
        break;

      case ST_VERTICAL:
        p1 = tmbox[BOXRIGHT] < ld->v1->x;
        if( p1 == ( tmbox[BOXLEFT] < ld->v1->x ) )
        {
            return( p1 ^ (ld->dy < 0) );
        }
        break;

      case ST_POSITIVE:
        p1 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXTOP], ld);
        if( p1 == P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld) )
            return p1;
        break;

      case ST_NEGATIVE:
        p1 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
        if( p1 == P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld) )
            return p1;
        break;

      default :
#ifdef PARANOIA	
        I_Error("P_BoxOnLineSide: unknown slopetype %d\n",ld->slopetype);
#endif
        break;
    }

    return -1;
}


//
// P_PointOnDivlineSide
// Returns 0 or 1.
int  P_PointOnDivlineSide ( fixed_t x, fixed_t y, const divline_t* line )
{
    if (!line->dx)
    {
        if (x <= line->x)
            return line->dy > 0;

        return line->dy < 0;
    }
    if (!line->dy)
    {
        if (y <= line->y)
            return line->dx < 0;

        return line->dx > 0;
    }

    register fixed_t dx = (x - line->x);
    register fixed_t dy = (y - line->y);

    // try to quickly decide by looking at sign bits
    if ( (line->dy ^ line->dx ^ dx ^ dy)&0x80000000 )
    {
        if ( (line->dy ^ dx) & 0x80000000 )
            return 1;           // (left is negative)
        return 0;
    }

    return( FixedMul ( dy>>8 , line->dx>>8 ) >= FixedMul ( line->dy>>8, dx>>8 ) );
        // 0 = front side, 1 = back side
}



//
// P_MakeDivline
//
void P_MakeDivline ( const line_t* li, divline_t* dl )
{
    dl->x = li->v1->x;
    dl->y = li->v1->y;
    dl->dx = li->dx;
    dl->dy = li->dy;
}



//
// P_InterceptVector
// Returns the fractional intercept point along the first divline.
// This is only called by the addthings and addlines traversers.
fixed_t  P_InterceptVector ( const divline_t* v2, const divline_t* v1 )
{
    register fixed_t
    den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

    if (den == 0)
        return 0;
    //  I_Error ("P_InterceptVector: parallel");

    register fixed_t    
    num = FixedMul ( (v1->x - v2->x)>>8 ,v1->dy )
        + FixedMul ( (v2->y - v1->y)>>8, v1->dx );

    return  FixedDiv (num , den);
}

#if 0
// UNUSED
// Debug float version
fixed_t  P_InterceptVector_float_debug ( const divline_t* v2, const divline_t* v1 )
{
    float       frac,num,den;
    float       v1x,v1y,v1dx,v1dy;
    float       v2x,v2y,v2dx,v2dy;

    v1x = (float)v1->x/FRACUNIT;
    v1y = (float)v1->y/FRACUNIT;
    v1dx = (float)v1->dx/FRACUNIT;
    v1dy = (float)v1->dy/FRACUNIT;
    v2x = (float)v2->x/FRACUNIT;
    v2y = (float)v2->y/FRACUNIT;
    v2dx = (float)v2->dx/FRACUNIT;
    v2dy = (float)v2->dy/FRACUNIT;

    den = v1dy*v2dx - v1dx*v2dy;

    if (den == 0)
        return 0;       // parallel

    num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
    frac = num / den;

    return frac*FRACUNIT;
}
#endif

#if 0
// UNUSED
// Float version.
fixed_t  P_InterceptVector_float ( const divline_t * v2, const divline_t * v1 )
{
    float frac,num,den;
    float v1dxf = (float) v1->dx;
    float v1dyf = (float) v1->dy;

    // Both num and den are scaled *(FRACUNIT*FRACUNIT),
    // but this will cancel out in the frac.
    den = (v1dyf * v2->dx) - (v1dxf * v2->dy);

    if (den == 0)
        return 0;       // parallel

    num = ((v1->x - v2->x) * v1dyf) + ((v2->y - v1->y) * v1dxf);
    frac = num / den;

    return (fixed_t)(frac * FRACUNIT);
}
#endif

#if 0
// PrBoom 64 bit version, adapted.
fixed_t  P_InterceptVector_64( const divline_t * v2, const divline_t * v1 )
{
#if 0
  if (compatibility_level < prboom_4_compatibility)
    return P_InterceptVector2(v2, v1);
#endif

    /* cph - This was introduced at prboom_4_compatibility - no precision/overflow problems */
    int64_t den = (int64_t)v1->dy * v2->dx - (int64_t)v1->dx * v2->dy;
    den >>= 16;
    if (!den)
      return 0;

    return (fixed_t)(((int64_t)(v1->x - v2->x) * v1->dy - (int64_t)(v1->y - v2->y) * v1->dx) / den);
}
#endif


//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//
fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t lowfloor;


void P_LineOpening ( const line_t* linedef )
{
    sector_t*      front;
    sector_t*      back;
    // tm_thing, tm_* are global var to TryMove and P_CheckPosition,
    // whose sub-functions may call here.

    if (linedef->sidenum[1] == NULL_INDEX)
    {
        // single sided line
        openrange = 0;
        return;
    }

    front = linedef->frontsector;
    back = linedef->backsector;
#ifdef PARANOIA
    if(!front)
        I_Error("lindef without front");
    if(!back)
        I_Error("lindef without back");
#endif

    if (front->ceilingheight < back->ceilingheight)
        opentop = front->ceilingheight;
    else
        opentop = back->ceilingheight;

    // [WDJ] Must find the lowest 3d floor, closest to feet,
    // independently in front and back sectors, so that lowfloor is always
    // from the other sector than openbottom floor.  Otherwise two 3d floor
    // in the same sector could become openbottom floor, and lowfloor.
    fixed_t    frontfloor = front->floorheight;
    fixed_t    backfloor = back->floorheight;

    //SoM: 3/27/2000: Check for fake floors in the sector.
    // Check 3d floors against tm_thing
    if(tm_thing
       && (front->ffloors || back->ffloors) )
    {
        // [WDJ] Old logic, in a complicated way, determined if head was
        // closer to slab top, or feet were closer slab bottom.
        // This is same as midslab being above or below midthing.
        // Test against midthing*2, and save all the divides and abs().
        fixed_t  midthing2 = tm_thing->z + tm_thing->z + tm_thing->height ; // midthing*2
        ffloor_t*  rovflr;

        // Check for frontsector 3dfloors
        if(front->ffloors)
        {
          for(rovflr = front->ffloors; rovflr; rovflr = rovflr->next)
          {
            if(!(rovflr->flags & FF_SOLID)) continue;

            // mid of slab >= mid of thing
            // (bottom + top)/2  >= (th->z + (th->z + th->height))/2
            if( (*rovflr->bottomheight + *rovflr->topheight) >= midthing2 )
            {
                // head is closer
                if(*rovflr->bottomheight < opentop)
                    opentop = *rovflr->bottomheight;
            }
            else
            {
                // feet are closer
                if(*rovflr->topheight > frontfloor)
                    frontfloor = *rovflr->topheight;
            }
          }
        }

        // Check for backsector 3dfloors
        if(back->ffloors)
        {
          for(rovflr = back->ffloors; rovflr; rovflr = rovflr->next)
          {
            if(!(rovflr->flags & FF_SOLID))
              continue;

            // mid of slab >= mid of thing
            // (bottom + top)/2  >= (th->z + (th->z + th->height))/2
            if( (*rovflr->bottomheight + *rovflr->topheight) >= midthing2 )
            {
                // head is closer
                if(*rovflr->bottomheight < opentop)
                    opentop = *rovflr->bottomheight;
            }
            else
            {
                // feet are closer
                if(*rovflr->topheight > backfloor)
                    backfloor = *rovflr->topheight;
            }
          }
        }
    }

    // now frontfloor and backfloor account for 3d floors too
    if (frontfloor > backfloor)
    {
        openbottom = frontfloor;
        lowfloor = backfloor;
    }
    else
    {
        openbottom = backfloor;
        lowfloor = frontfloor;
    }

    openrange = opentop - openbottom;
}




//
// THING POSITION SETTING
//


//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition (mobj_t* thing)
{
    if ( ! (thing->flags & MF_NOSECTOR) )
    {
        // inert things don't need to be in blockmap?
        // unlink from subsector
        if (thing->snext)
            thing->snext->sprev = thing->sprev;

        if (thing->sprev)
            thing->sprev->snext = thing->snext;
        else
            thing->subsector->sector->thinglist = thing->snext;
#ifdef PARANOIA
        // Must match the check in SetThingPosition
        thing->sprev = NULL;
        thing->snext = NULL;
#endif
        //SoM: 4/7/2000
        //
        // Save the sector list pointed to by touching_sectorlist.
        // In P_SetThingPosition, we'll keep any nodes that represent
        // sectors the Thing still touches. We'll add new ones then, and
        // delete any nodes for sectors the Thing has vacated. Then we'll
        // put it back into touching_sectorlist. It's done this way to
        // avoid a lot of deleting/creating for nodes, when most of the
        // time you just get back what you deleted anyway.
        //
        // If this Thing is being removed entirely, then the calling
        // routine will clear out the nodes in sector_list.

        sector_list = thing->touching_sectorlist;
        thing->touching_sectorlist = NULL; //to be restored by P_SetThingPosition
    }

    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
        // inert things don't need to be in blockmap
        // unlink from block map

        // Unlink that is independent of thing x,y.
        // This allows unlinking even when thing x,y have been disturbed,
        // which is much safer.
        // This may be similar to Killough code in MBF, PrBoom and others.
        if( thing->bprev ) { // IF NULL then is not linked, could be error.
            *(thing->bprev) = thing->bnext;  // unlink from prev head or thing.
            if( thing->bnext )
                thing->bnext->bprev = thing->bprev;  // propogate back ptr

            thing->bprev = NULL;  // to be consistent with above test
//            thing->bnext = NULL;  // safer, neater, but unnecessary
        }

#ifdef PARANOIA
        // Can set bnext and bprev to NULL, or not.
        thing->bnext = NULL;
        thing->bprev = NULL;
#endif


#if 0  /* OLD CODE */
// Unlink as in Vanilla Doom, Heretic, and Boom.
        if (thing->bnext)
            thing->bnext->bprev = thing->bprev;

        if (thing->bprev)
            thing->bprev->bnext = thing->bnext;
        else
        {
// [WDJ] If thing x,y were changed, then would get wrong blockmap list,
// and unlinking then would corrupt two blockmap lists.
// Later, thing could get linked to itself. This would lead to
// a tight loop during scan of blockmap list, hung program.

            // Assumes is at head if bprev == NULL
            int blockx = (thing->x - bmaporgx)>>MAPBLOCKSHIFT;
            int blocky = (thing->y - bmaporgy)>>MAPBLOCKSHIFT;

            if (blockx>=0 && blockx < (int)bmapwidth
                && blocky>=0 && blocky < (int)bmapheight)
            {
                blocklinks[blocky*bmapwidth+blockx] = thing->bnext;
            }
        }
#endif /* OLD CODE */	
    }
}


//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
void P_SetThingPosition (mobj_t* thing)
{
    subsector_t*        ss;
    sector_t*           sec;

    // link into subsector
    ss = R_PointInSubsector (thing->x,thing->y);
    thing->subsector = ss;

    if ( ! (thing->flags & MF_NOSECTOR) )
    {
        // invisible things don't go into the sector links
        sec = ss->sector;
#ifdef PARANOIA
        // Requires the NULL assigns in UnsetThingPosition.
        if ( thing->sprev != NULL || thing->snext != NULL )
            I_Error("linking a thing(%d) that is already linked", thing->type);
#endif

        thing->sprev = NULL;
        thing->snext = sec->thinglist;

        if (sec->thinglist)
            sec->thinglist->sprev = thing;

        sec->thinglist = thing;

        //SoM: 4/6/2000
        //
        // If sector_list isn't NULL, it has a collection of sector
        // nodes that were just removed from this Thing.

        // Collect the sectors the object will live in by looking at
        // the existing sector_list and adding new nodes and deleting
        // obsolete ones.

        // When a node is deleted, its sector links (the links starting
        // at sector_t->touching_thinglist) are broken. When a node is
        // added, new sector links are created.

        P_Create_SecNodeList(thing, thing->x, thing->y);
        thing->touching_sectorlist = sector_list; // Attach to Thing's mobj_t
        sector_list = NULL; // clear for next time
    }


    // link into blockmap
    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
        // inert things don't need to be in blockmap
        int blockx = (thing->x - bmaporgx)>>MAPBLOCKSHIFT;
        int blocky = (thing->y - bmaporgy)>>MAPBLOCKSHIFT;

        if (blockx>=0
            && blockx < (int)bmapwidth
            && blocky>=0
            && blocky < (int)bmapheight)
        {
            mobj_t * * headp = &blocklinks[ blocky*bmapwidth + blockx ];  // head ptr

            // [WDJ] Blockmap linked list that can be unlinked independent of x,y.
            // The bprev points directly to the previous link, not the previous thing.
            thing->bprev = headp;  // back ptr to head link
            thing->bnext = *headp;
            if( *headp ) {
                (*headp)->bprev = & thing->bnext;  // back ptr to thing link
            }
            *headp = thing;

#if 0  /* OLD CODE */
// Link as in Vanilla Doom, Heretic, and Boom.
            mobj_t * * link = &blocklinks[blocky*bmapwidth+blockx];

            link = &blocklinks[blocky*bmapwidth+blockx];
            thing->bprev = NULL;
            thing->bnext = *link;
            if (*link)
                (*link)->bprev = thing;

            *link = thing;
#endif  /* OLD CODE */
        }
        else
        {
            // thing is off the map
            // Only useful if unlink checks for NULL.
            thing->bprev = NULL;
            thing->bnext = NULL;
        }
    }
}



//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//


//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
// x,y are blockmap indexes
boolean P_BlockLinesIterator(int x, int y,
                             boolean (*func)(line_t*))
{
    if (x<0 || y<0 ||
        x >= (int)bmapwidth || y >= (int)bmapheight)
    {
        return true;
    }

    int offset = y * bmapwidth+x;
    offset = blockmapindex[offset]; //	offset = blockmap[y*bmapwidth+x];

    const uint32_t *list; // Big blockmap, SSNTails
    for (list = &blockmaphead[offset] ; *list != (uint32_t)-1 ; list++)
    {
        line_t *ld = &lines[*list];

        if (ld->validcount == validcount)
            continue;   // line has already been checked

        ld->validcount = validcount;

        if ( !func(ld) )
            return false;
    }
    return true;        // everything was checked
}


//
// P_BlockThingsIterator
//
boolean P_BlockThingsIterator ( int x, int y,
                                boolean(*func)(mobj_t*) )
{
    mobj_t *  mobj;

    if ( x<0
         || y<0
         || x>= (int)bmapwidth
         || y>= (int)bmapheight)
    {
        return true;
    }

    //added:15-02-98: check interaction (ligne de tir, ...)
    //                avec les objets dans le blocmap
    for (mobj = blocklinks[y*bmapwidth+x] ;
         mobj ;
         mobj = mobj->bnext)
    {
        if (!func( mobj ) )
            return false;
    }
    return true;
}



//
// INTERCEPT ROUTINES
//

//SoM: 4/6/2000: Limit removal
int max_intercepts = 0;   // num intercepts allocated
intercept_t*    intercepts = NULL;	// realloc,
intercept_t*    intercept_p = NULL;

divline_t       trace;
boolean         earlyout;
int             ptflags;



//SoM: 4/6/2000: Remove limit on intercepts.
void P_CheckIntercepts( void )
{
  int count = intercept_p - intercepts;

  if(max_intercepts <= count)
  {
    if(!max_intercepts)
      max_intercepts = 128;
    else
      max_intercepts = max_intercepts * 2;

    intercepts = realloc(intercepts, sizeof(intercept_t) * max_intercepts);
    if( intercepts == NULL )
        I_Error("P_CheckIntercepts: allocation failure of %i intercepts\n", max_intercepts);
       
    intercept_p = intercepts + count;
  }
}

//
// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.
//
boolean  PIT_AddLineIntercepts (line_t* ld)
{
    int                 s1;
    int                 s2;
    fixed_t             frac;
    divline_t           dl;

    // avoid precision problems with two routines
    if ( trace.dx > FRACUNIT*16
         || trace.dy > FRACUNIT*16
         || trace.dx < -FRACUNIT*16
         || trace.dy < -FRACUNIT*16)
    {
        //Hurdler: crash here with phobia when you shoot on the door next the stone bridge
        //stack overflow???
        s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
        s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
    }
    else
    {
        s1 = P_PointOnLineSide (trace.x, trace.y, ld);
        s2 = P_PointOnLineSide (trace.x+trace.dx, trace.y+trace.dy, ld);
    }

    if (s1 == s2)
        return true;    // line isn't crossed

    // hit the line
    P_MakeDivline (ld, &dl);
    frac = P_InterceptVector (&trace, &dl);

    if (frac < 0)
        return true;    // behind source

    // try to early out the check
    if (earlyout
        && frac < FRACUNIT
        && !ld->backsector)
    {
        return false;   // stop checking
    }

    //SoM: 4/6/2000: Limit removal
    P_CheckIntercepts();

    intercept_p->frac = frac;
    intercept_p->isaline = true;
    intercept_p->d.line = ld;
    intercept_p++;

    return true;        // continue
}



//
// PIT_AddThingIntercepts
//
boolean PIT_AddThingIntercepts (mobj_t* thing)
{
    fixed_t             x1;
    fixed_t             y1;
    fixed_t             x2;
    fixed_t             y2;

    int                 s1;
    int                 s2;

    boolean             tracepositive;

    divline_t           dl;

    fixed_t             frac;

    tracepositive = (trace.dx ^ trace.dy)>0;

    // check a corner to corner crossection for hit
    if (tracepositive)
    {
        x1 = thing->x - thing->radius;
        y1 = thing->y + thing->radius;

        x2 = thing->x + thing->radius;
        y2 = thing->y - thing->radius;
    }
    else
    {
        x1 = thing->x - thing->radius;
        y1 = thing->y - thing->radius;

        x2 = thing->x + thing->radius;
        y2 = thing->y + thing->radius;
    }

    s1 = P_PointOnDivlineSide (x1, y1, &trace);
    s2 = P_PointOnDivlineSide (x2, y2, &trace);

    if (s1 == s2)
        return true;            // line isn't crossed

    dl.x = x1;
    dl.y = y1;
    dl.dx = x2-x1;
    dl.dy = y2-y1;

    frac = P_InterceptVector (&trace, &dl);

    if (frac < 0)
        return true;            // behind source

    P_CheckIntercepts();

    intercept_p->frac = frac;
    intercept_p->isaline = false;
    intercept_p->d.thing = thing;
    intercept_p++;

    return true;                // keep going
}


//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
boolean P_TraverseIntercepts ( traverser_t   func,
                               fixed_t       maxfrac )
{
    int                 count;
    fixed_t             dist;
    intercept_t*        scan;
    intercept_t*        in;

    count = intercept_p - intercepts;

    in = 0;                     // shut up compiler warning

    while (count--)
    {
        dist = FIXED_MAX;
        for (scan = intercepts ; scan<intercept_p ; scan++)
        {
            if (scan->frac < dist)
            {
                dist = scan->frac;
                in = scan;
            }
        }

        if (dist > maxfrac)
            return true;        // checked everything in range

#if 0  // UNUSED
    {
        // don't check these yet, there may be others inserted
        in = scan = intercepts;
        for ( scan = intercepts ; scan<intercept_p ; scan++)
            if (scan->frac > maxfrac)
                *in++ = *scan;
        intercept_p = in;
        return false;
    }
#endif

        // appelle la fonction en commencant par l' intercept_t le plus
        // proche
        if ( !func (in) )
            return false;       // don't bother going farther

        in->frac = FIXED_MAX;
    }

    return true;                // everything was traversed
}




//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
boolean P_PathTraverse ( fixed_t       x1,
                         fixed_t       y1,
                         fixed_t       x2,
                         fixed_t       y2,
                         int           flags,
                         traverser_t   trav)
{
    fixed_t     xt1;
    fixed_t     yt1;
    fixed_t     xt2;
    fixed_t     yt2;

    fixed_t     xstep;
    fixed_t     ystep;

    fixed_t     partial;

    fixed_t     xintercept;
    fixed_t     yintercept;

    int         mapx;
    int         mapy;

    int         mapxstep;
    int         mapystep;

    int         count;

    earlyout = flags & PT_EARLYOUT;

    validcount++;
    intercept_p = intercepts;

    if ( ((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
        x1 += FRACUNIT; // don't side exactly on a line

    if ( ((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
        y1 += FRACUNIT; // don't side exactly on a line

    trace.x = x1;
    trace.y = y1;
    trace.dx = x2 - x1;
    trace.dy = y2 - y1;

    x1 -= bmaporgx;
    y1 -= bmaporgy;
    xt1 = x1>>MAPBLOCKSHIFT;
    yt1 = y1>>MAPBLOCKSHIFT;

    x2 -= bmaporgx;
    y2 -= bmaporgy;
    xt2 = x2>>MAPBLOCKSHIFT;
    yt2 = y2>>MAPBLOCKSHIFT;

    if (xt2 > xt1)
    {
        mapxstep = 1;
        partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
        ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else if (xt2 < xt1)
    {
        mapxstep = -1;
        partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
        ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else
    {
        mapxstep = 0;
        partial = FRACUNIT;
        ystep = 256*FRACUNIT;
    }

    yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);


    if (yt2 > yt1)
    {
        mapystep = 1;
        partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
        xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else if (yt2 < yt1)
    {
        mapystep = -1;
        partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
        xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else
    {
        mapystep = 0;
        partial = FRACUNIT;
        xstep = 256*FRACUNIT;
    }
    xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);

    // Step through map blocks.
    // Count is present to prevent a round off error
    // from skipping the break.
    mapx = xt1;
    mapy = yt1;

    for (count = 0 ; count < 64 ; count++)
    {
        if (flags & PT_ADDLINES)
        {
            if (!P_BlockLinesIterator (mapx, mapy,PIT_AddLineIntercepts))
                return false;   // early out
        }

        if (flags & PT_ADDTHINGS)
        {
            if (!P_BlockThingsIterator (mapx, mapy,PIT_AddThingIntercepts))
                return false;   // early out
        }

        if (mapx == xt2
            && mapy == yt2)
        {
            break;
        }

        if ( (yintercept >> FRACBITS) == mapy)
        {
            yintercept += ystep;
            mapx += mapxstep;
        }
        else if ( (xintercept >> FRACBITS) == mapx)
        {
            xintercept += xstep;
            mapy += mapystep;
        }

    }
    // go through the sorted list
    return P_TraverseIntercepts ( trav, FRACUNIT );
}


#ifdef HEXEN
// hexen

static
mobj_t * Hexen_RoughBlockCheck( mobj_t * mo, int index )
{
    mobj_t * blklink;
    mobj_t * master;
    angle_t angle;

    for( blklink = blocklinks[index] ; blklink ; blklink = blklink->bnext )
    {
        if( mo->player )  // Minotaur looking around player
        {
            if( (blklink->flags & MF_COUNTKILL)
                || (bblklink->player && (blklink != mo)))
            {
                if( !(blklink->flags & MF_SHOOTABLE) )
                    continue;

                if( blklink->flags3 & MF3_DORMANT )
                    continue;

                if( (blklink->type == MT_HEXEN_MINOTAUR)
                    && (blklink->special1.m == mo))
                    continue;

                if( netgame && !deathmatch && blklink->player )
                    continue;

                if( P_CheckSight(mo, blklink) )
                    return blklink;
            }
        }
        else if( mo->type == MT_HEXEN_MINOTAUR) // looking around minotaur
        {
            master = mo->special1.m;
            if( (blklink->flags & MF_COUNTKILL)
                || (blklink->player && (blklink != master)) )
            {
                if( !(blklink->flags & MF_SHOOTABLE))
                    continue;

                if( blklink->flags3 & MF3_DORMANT )
                    continue;

                if( (blklink->type == MT_HEXEN_MT_MINOTAUR)
                    && (blklink->special1.m == mo->special1.m))
                    continue;

                if( netgame && !deathmatch && blklink->player )
                    continue;

                if (P_CheckSight(mo, blklink))
                    return blklink;
            }
        }
        else if( mo->type == MT_HEXEN_MSTAFF_FX2) // bloodscourge
        {
            if( ((blklink->flags & MF_COUNTKILL)
                 || (blklink->player && blklink != mo->target))
                && !(blklink->flags2 & MF2_DORMANT) )
            {
                if( !(blklink->flags & MF_SHOOTABLE) )
                    continue;

                if( netgame && !deathmatch && blklink->player )
                    continue;

                if( P_CheckSight(mo, blklink) )
                {
                    master = mo->target;
                    angle = R_PointToAngle2( master->x, master->y, blklink->x, blklink->y) - master->angle;
                    angle >>= 24;
                    if( angle > 226 || angle < 30 )
                        return blklink;
                }
            }
        }
        else // spirits
        {
            if( ((blklink->flags & MF_COUNTKILL)
                 || (blklink->player && blklink != mo->target))
                && !(blklink->flags2 & MF2_DORMANT) )
            {
                if( !(blklink->flags & MF_SHOOTABLE) )
                    continue;

                if( netgame && !deathmatch && blklink->player )
                    continue;

                if( blklink == mo->target )
                    continue;

                if( P_CheckSight(mo, blklink) )
                    return blklink;
            }
        }
    }
    return NULL;
}
#endif


#ifdef MBF21
// [WDJ] MBF21, derived from DSDA-Doom, required for MBF21.
// RoughBlockCheck
// [XA] adapted from Hexen -- used by P_RoughTargetSearch

static
mobj_t * RoughBlockCheck( mobj_t * mo, int index, angle_t fov )
{
    mobj_t * blklink;

#ifdef HEXEN    
    if( EN_hexen )
      return  Hexen_RoughBlockCheck( mo, index );
#endif

    for( blklink = blocklinks[index] ; blklink ; blklink = blklink->bnext )
    {
        // skip non-shootable actors
        if( !(blklink->flags & MF_SHOOTABLE) )
          continue;

#ifdef HEXEN
        // skip dormant actors
        if( blklink->flags3 & MF3_DORMANT )
          continue;
#endif

        // skip the projectile owner
        if( blklink == mo->target )
          continue;

        // skip actors on the same team, unless infighting or deathmatching
        if( mo->target
            && SAME_FRIEND( blklink, mo->target )
            && mo->target->target != blklink
            && !(deathmatch && blklink->player && mo->target->player))
        {
            continue;
        }

        // skip actors outside of specified FOV
        if( (fov > 0) && !P_Check_FOV(mo, blklink, fov) )
            continue;

        // find actor in line of sight
        if( P_CheckSight(mo, blklink) )
          return blklink;
    }

    // couldn't find a valid target
    return NULL;
}


// [WDJ] MBF21, another function required to implement MBF21
//
// P_RoughTargetSearch
// Searches though the surrounding mapblocks for monsters/players
// based on Hexen's P_RoughMonsterSearch
//
//   distance : limit of search, in MAPBLOCKUNITS
mobj_t * P_RoughTargetSearch( mobj_t * mo, angle_t fov, int distance )
{
    mobj_t * target;
    int x0, y0;  // mobj location
    int x1, y1;  // traversing nearby blocks, corner1
    int x2, y2;  // corner2
    int blkindex, blkindex1, blkindex2, blkindex3, blkindex4;
    int count;

    x0 = (mo->x - bmaporgx) >> MAPBLOCKSHIFT;
    y0 = (mo->y - bmaporgy) >> MAPBLOCKSHIFT;

    if( (x0 >= 0) && (x0 < bmapwidth) && (y0 >= 0) && (y0 < bmapheight) )
    {
        target = RoughBlockCheck( mo, (y0 * bmapwidth) + x0, fov );
        if( target )
          goto found_target; // found a target right away
    }

    for( count = 1; count <= distance; count++ )
    {
        y1 = y0 - count;
        if( y1 < 0 )
        {
            y1 = 0;
        }
        else if( y1 >= bmapheight )
        {
            y1 = bmapheight - 1;
        }

        x1 = x0 - count;
        if( x1 < 0 )
        {
            x1 = 0;
        }
        else if( x1 >= bmapwidth )
        {
            x1 = bmapwidth - 1;
        }

        x2 = x0 + count;
        if( x2 < 0 )
            continue;

        if( x2 >= bmapwidth )
        {
            x2 = bmapwidth - 1;
        }
        y2 = y0 + count;
        if( y2 < 0 )
            continue;

        if( y2 >= bmapheight )
        {
            y2 = bmapheight - 1;
        }

        blkindex3 = (y2 * bmapwidth) + x1;
        blkindex2 = (y2 * bmapwidth) + x2;
        blkindex1 = (y1 * bmapwidth) + x2;
        blkindex4 = (y1 * bmapwidth) + x1;

        blkindex = blkindex4;
        // Trace the first block section (along the top)
        for(  ; blkindex <= blkindex1; blkindex++ )
        {
            target = RoughBlockCheck(mo, blkindex, fov);
            if( target )
                goto found_target;
        }
        // Trace the second block section (right edge)
        for( blkindex--; blkindex <= blkindex2; blkindex += bmapwidth )
        {
            target = RoughBlockCheck(mo, blkindex, fov);
            if( target )
                goto found_target;
        }
        // Trace the third block section (bottom edge)
        for( blkindex -= bmapwidth; blkindex >= blkindex3; blkindex-- )
        {
            target = RoughBlockCheck(mo, blkindex, fov);
            if( target )
                goto found_target;
        }
        // Trace the final block section (left edge)
        for( blkindex++; blkindex > blkindex4; blkindex -= bmapwidth )
        {
            target = RoughBlockCheck(mo, blkindex, fov);
            if( target )
              goto found_target;
        }
    }
    return NULL;

found_target:
    return target;
}
#endif


// =========================================================================
//                                                        BLOCKMAP ITERATORS
// =========================================================================

// blockmap iterator for all sorts of use
// your routine must return FALSE to exit the loop earlier
// returns FALSE if the loop exited early after a false return
// value from your user function

//abandoned, maybe I'll need it someday..  
/*
boolean P_RadiusLinesCheck (  fixed_t    radius,
                              fixed_t    x,
                              fixed_t    y,
                              boolean   (*func)(line_t*))
{
    int   xl, xh, yl, yh;
    int   bx, by;

    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;
    
    // check lines
    xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator (bx,by,func))
                return false;
    return true;
}
*/
