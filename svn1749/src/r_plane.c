// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_plane.c 1691 2024-11-27 06:36:07Z wesleyjohnson $
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
// $Log: r_plane.c,v $
// Revision 1.20  2004/05/16 20:25:46  hurdler
// change version to 1.43
//
// Revision 1.19  2003/06/10 23:36:09  ssntails
// Variable flat support (32x32 to 2048x2048)
//
// Revision 1.18  2002/09/25 16:38:35  ssntails
// Alpha support for trans 3d floors in software
//
// Revision 1.17  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.16  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.15  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.14  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.13  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.12  2000/11/11 13:59:46  bpereira
// Revision 1.11  2000/11/06 20:52:16  bpereira
//
// Revision 1.10  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.9  2000/04/30 10:30:10  bpereira
// Revision 1.8  2000/04/18 17:39:39  stroggonmeth
// Revision 1.7  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.6  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.5  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Here is a core component: drawing the floors and ceilings,
//       while maintaining a per column clipping list only.
//      Moreover, the sky areas have to be determined.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "console.h"
#include "g_game.h"
#include "r_data.h"
#include "r_local.h"
#include "r_state.h"
#include "r_splats.h"   //faB(21jan):testing
#include "r_sky.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_setup.h"    // levelflats

planefunction_t         floorfunc = NULL;
planefunction_t         ceilingfunc = NULL;

//
// opening
//

// [WDJ] visplane base   vispl_
// Here comes the obnoxious "visplane".
/*#define                 MAXVISPLANES 128 //SoM: 3/20/2000
visplane_t*             vispl_head;
visplane_t*             vispl_last;*/

//SoM: 3/23/2000: Use Boom visplane hashing.
#define           VISPL_HASHSIZE      128
// visplane hash array, for fast duplicate check
static visplane_t * vispl_hashtab[VISPL_HASHSIZE];

// free list of visplane_t
// [WDJ] head and tail were reversed from normal linked list meanings
// Insert at tail, take free off head, use next for linking.
static visplane_t * vispl_free_head = NULL;
static visplane_t ** vispl_free_tail = &vispl_free_head;  // addr of head or next ptr
#ifdef DYNAMIC_VISPLANE_COVER
static uint16_t visplane_width = 0;  // vid width of all visplanes
static unsigned int  visplane_cover_size = 0; // sizeof cover array
#endif

// [WDJ] visplane_t global parameters  vsp_
// visplane used for drawing in r_bsp and r_segs
visplane_t*             vsp_floorplane;
visplane_t*             vsp_ceilingplane;

// visplane used by R_MapPlane, set by R_DrawSinglePlane
visplane_t*             vsp_currentplane;

// this use 251 Kb memory (in Legacy 1.43)
// [WDJ] Renamed so they do not confuse with ffloor
// The ffplanes of the subsector processed by BSP.
ff_planemgr_t           ffplane[MAXFFLOORS];
int                     numffplane;

//SoM: 3/23/2000: Boom visplane hashing routine.
#define visplane_hash(picnum,lightlevel,height) \
  ((unsigned)((picnum)*3+(lightlevel)+(height)*7) & (VISPL_HASHSIZE-1))



//
// Clip values are the solid pixel bounding the range.
//  [WDJ] Clip is the highest/lowest drawable row for that column.
//  This allows limit tests to be done without needing +1 and -1.
//  In orignal Doom the clip was outside the drawable area,
//  and in limit tests it needed +1 and -1 to get the drawable row.
//  Init floorclip to SCREENHEIGHT (bottom of screen).
//  Init ceilingclip to 0 (top of screen).
//  There are other limit tests applied that will limit the clip to the window.
int16_t                 floorclip[MAXVIDWIDTH];
int16_t                 ceilingclip[MAXVIDWIDTH];
fixed_t                 backscale[MAXVIDWIDTH];


//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int                     spanstart[MAXVIDHEIGHT];
//int                     spanstop[MAXVIDHEIGHT]; //added:08-02-98: Unused!!

//
// texture mapping
//
lighttable_t**          planezlight;
fixed_t                 planeheight;

//added:10-02-98: yslopetab is what yslope used to be,
//                yslope points somewhere into yslopetab,
//                now (viewheight/2) slopes are calculated above and
//                below the original viewheight for mouselook
//                (this is to calculate yslopes only when really needed)
//                (when mouselookin', yslope is moving into yslopetab)
//                Check R_SetupFrame, R_SetViewSize for more...
fixed_t                 yslopetab[MAXVIDHEIGHT*4];
fixed_t*                yslope = NULL;

fixed_t                 distscale[MAXVIDWIDTH];
fixed_t                 base_scale_x;
fixed_t                 base_scale_y;

fixed_t                 cachedheight[MAXVIDHEIGHT];
fixed_t                 cacheddistance[MAXVIDHEIGHT];
fixed_t                 cachedxstep[MAXVIDHEIGHT];
fixed_t                 cachedystep[MAXVIDHEIGHT];

fixed_t   xoffs, yoffs;

// R_Init_Planes
// Only at game startup.
//
void R_Init_Planes (void)
{
  // Doh!
}


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
         long long mycount;
         long long mytotal = 0;
         unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  base_scale_x
//  base_scale_y
//  viewx
//  viewy
//  xoffs
//  yoffs
//
// BASIC PRIMITIVE
//

// Draw plane span at row y, span=(x1..x2)
// at planeheight, using spanfunc
void R_MapPlane ( int y, int x1, int x2 )
{
    int         angf;
    fixed_t     distance;
    fixed_t     length;
    unsigned    index;

#ifdef RANGECHECK_DRAW_LIMITS
    if (x2 < x1
        || x1<0
        || x2>=rdraw_viewwidth
        || (unsigned)y>rdraw_viewheight)    // [WDJ] ??  y>=rdraw_viewheight
    {
        I_SoftError ("R_MapPlane: %i, %i at %i",x1,x2,y);
        return;
    }
#endif

    if (planeheight != cachedheight[y])
    {
        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
        ds_xstep = cachedxstep[y] = FixedMul (distance,base_scale_x);
        ds_ystep = cachedystep[y] = FixedMul (distance,base_scale_y);
    }
    else
    {
        distance = cacheddistance[y];
        ds_xstep = cachedxstep[y];
        ds_ystep = cachedystep[y];
    }
    length = FixedMul (distance,distscale[x1]);
    angf = ANGLE_TO_FINE(vsp_currentplane->viewangle + x_to_viewangle[x1]);
#if 0
    ds_xfrac = viewx + FixedMul(finecosine[angf], length) + xoffs;
    ds_yfrac = yoffs - viewy - FixedMul(finesine[angf], length);
#else
    // SoM: Wouldn't it be faster just to add viewx and viewy to the plane's
    // x/yoffs anyway?? (Besides, it serves my purpose well for portals!)
    ds_xfrac = FixedMul(finecosine[angf], length) + xoffs;
    ds_yfrac = yoffs - FixedMul(finesine[angf], length);
#endif


    if (fixedcolormap)
        ds_colormap = fixedcolormap; // overriding colormap
    else
    {
        index = distance >> LIGHTZSHIFT;

        if (index >= MAXLIGHTZ )
            index = MAXLIGHTZ-1;

        ds_colormap = planezlight[index];
        if(vsp_currentplane->extra_colormap || view_colormap)
        {
            // reverse indexing, and change to extra_colormap
            int lightindex = ds_colormap - reg_colormaps;
            lighttable_t* cm = view_colormap? view_colormap : vsp_currentplane->extra_colormap->colormap;
            ds_colormap = & cm[ lightindex ];
        }
    }

    ds_y = y;
    ds_x1 = x1;
    ds_x2 = x2;
    // high or low detail

//added:16-01-98:profile hspans drawer.
#ifdef TIMING
  ProfZeroTimer();
#endif
   
  spanfunc ();

#ifdef TIMING
  RDMSR(0x10,&mycount);
  mytotal += mycount;   //64bit add
  if(nombre--==0)
  I_Error("spanfunc() CPU Spy reports: 0x%d %d\n", *((int*)&mytotal+1),
                                        (int)mytotal );
#endif

}


//
// R_Clear_Planes
// At begining of frame.
//
//Fab:26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of the view hidden under the console
void R_Clear_Planes (player_t *player)
{
    int  i, p;
    int  angf;


    // opening / clipping determination
    // init to window limits
    for (i=0 ; i<rdraw_viewwidth ; i++)
    {
        backscale[i] = FIXED_MAX;
        // Init to drawable window.
        floorclip[i] = rdraw_viewheight - 1;
        ceilingclip[i] = con_clipviewtop;
        for(p = 0; p < MAXFFLOORS; p++)
        {
          // [WDJ] set to highest and lowest drawable row
          ffplane[p].front_clip_bot[i] = rdraw_viewheight - 1;
          ffplane[p].plane_clip_top[i] = con_clipviewtop;
        }
    }

    numffplane = 0;

    //vispl_last = vispl_head;

    //SoM: 3/23/2000
    // put all visplanes to free list, while clearing vispl_hashtab[] to NULL
    for (i=0; i<VISPL_HASHSIZE; i++)
    {
        *vispl_free_tail = vispl_hashtab[i];
        vispl_hashtab[i] = NULL;
        while( *vispl_free_tail )
            vispl_free_tail = &(*vispl_free_tail)->next;
    }
   
#ifdef DYNAMIC_VISPLANE_COVER
    // Check if the display width has changed.
    if( vid.width != visplane_width )
    {
        // The visplane top and bottom arrays have the wrong sizes.
        while( vispl_free_head )
        {
           visplane_t* vd = vispl_free_head;
           vispl_free_head = vd->next;
           free( vd );  // top and bottom arrays are part of the allocation.
        }
        // New visplane will be of this width.
        visplane_width = vid.width;
        visplane_cover_size = sizeof(vis_cover_t) * vid.width;  // not incl. the pads
        vispl_free_tail = & vispl_free_head;
    }
#endif

    // texture calculation
    memset (cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angf = ANGLE_TO_FINE(viewangle - ANG90);

    // scale will be unit scale at SCREENWIDTH/2 distance
    base_scale_x = FixedDiv (finecosine[angf], projection_x);
    base_scale_y = -FixedDiv (finesine[angf], projection_x);
}


//SoM: 3/23/2000: New function, by Lee Killough
// [WDJ] 7/2010 Mostly rewritten.
static visplane_t*  new_visplane(unsigned hash)
{
  // return the next visplane_t from the free list
  visplane_t*  np = vispl_free_head;
  if (np)
  {
    // unlink from free list
    vispl_free_head = vispl_free_head->next;
    if ( ! vispl_free_head )
      vispl_free_tail = &vispl_free_head;	// empty free list
  }
  else
  {
    // list empty, make a new visplane
#ifdef DYNAMIC_VISPLANE_COVER
    // The visplane has a dynamically sized cover array.
    // The cover array is sized to the vid.width.
    int cover_size = ( vid.width + 1 ) * sizeof(vis_cover_t);  // must include pad2
    np = calloc(1, sizeof(visplane_t) + cover_size );  // 1 visplane_t, zeroed
#else
    np = calloc(1, sizeof(visplane_t));  // 1 visplane_t, zeroed
#endif
  }
  // link into hash, at [hash]
  np->next = vispl_hashtab[hash];
  vispl_hashtab[hash] = np;
  
  // [WDJ] Initialize the top and bottom clip array, with the visplane.
#ifdef DYNAMIC_VISPLANE_COVER
  vis_cover_t * vc;
  vis_cover_t * vc_end = &np->cover[vid.width];
  for( vc = &np->cover[0]; vc < vc_end; vc ++ )
  {
      vc->top = TOP_MAX;
      vc->bottom = 0;
  }
#else
  memset(np->top, 0xff, sizeof np->top);
#endif
  return np;
}



//
// R_FindPlane : cherche un visplane ayant les valeurs identiques:
//               meme hauteur, meme flattexture, meme lightlevel.
//               Sinon en alloue un autre.
//  Search the existing visplanes for one matching :
//    height, flattexture, lightlevel, etc..
//  If none found, return a new visplane with these settings.
//
visplane_t* R_FindPlane( fixed_t height,
                         int     picnum,
                         lightlev_t  lightlevel,
                         fixed_t xoff,
                         fixed_t yoff,
                         extracolormap_t* planecolormap,
                         ffloor_t* ffloor)
{
    visplane_t* check;
    unsigned    hash; //SoM: 3/23/2000

    xoff += viewx; // SoM
    yoff = -viewy + yoff;

    if (picnum == sky_flatnum)
    {
        height = 0;                     // all skys map together
        lightlevel = 0;
    }


    //SoM: 3/23/2000: New visplane algorithm uses hash table -- killough
    hash = visplane_hash(picnum,lightlevel,height);

    for (check=vispl_hashtab[hash]; check; check=check->next)
    {
      if (height == check->height &&
          picnum == check->picnum &&
          lightlevel == check->lightlevel &&
          xoff == check->xoffs &&
          yoff == check->yoffs &&
          planecolormap == check->extra_colormap &&
          !ffloor && !check->ffloor &&
          check->viewz == viewz &&
          check->viewangle == viewangle)
        return check; // found matching
    }

    check = new_visplane(hash);

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = vid.width;
    check->maxx = -1;
    check->xoffs = xoff;
    check->yoffs = yoff;
    check->extra_colormap = planecolormap;
    check->ffloor = ffloor;
    check->viewz = viewz;
    check->viewangle = viewangle;

    return check;
}




//
// R_CheckPlane : return same visplane or alloc a new one if needed
//
visplane_t*  R_CheckPlane( visplane_t*   pl,
                           int           start,
                           int           stop )
{
    int         intrl, intrh;  // intersect of the ranges
    int         unionl, unionh;  // union of the ranges
    int         x;

    // (unionl,intrl) = minmax( pl->minx, start )
    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    // (intrh,unionh) = minmax( pl->maxx, stop )
    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    //added 30-12-97 : 0xff ne vaut plus -1 avec un short...
    // find any x in intersect range where have valid top[]
    for (x=intrl ; x<= intrh ; x++)
#ifdef DYNAMIC_VISPLANE_COVER
        if (pl->cover[x].top != TOP_MAX)
            break;
#else
        if (pl->top[x] != TOP_MAX)
            break;
#endif     

    //SoM: 3/23/2000: Boom code
    if (x > intrh)
    {
        // no valid top[] within intersect range
        // No overlap, can extend visplane to union
        pl->minx = unionl;
        pl->maxx = unionh;
    }
    else
    {
        // overlap conflict, must create new visplane
        // new visplane over range start..stop
        unsigned hash = visplane_hash(pl->picnum, pl->lightlevel, pl->height);
        visplane_t *new_pl = new_visplane(hash);

        new_pl->height = pl->height;
        new_pl->picnum = pl->picnum;
        new_pl->lightlevel = pl->lightlevel;
        new_pl->xoffs = pl->xoffs;           // killough 2/28/98
        new_pl->yoffs = pl->yoffs;
        new_pl->extra_colormap = pl->extra_colormap;
        new_pl->ffloor = pl->ffloor;
        new_pl->viewz = pl->viewz;
        new_pl->viewangle = pl->viewangle;
        pl = new_pl;  // return new visplane
        pl->minx = start;
        pl->maxx = stop;
    }
    return pl;
}


//
// R_ExpandPlane
//
// SoM: This function basically expands the visplane or I_Errors
// The reason for this is that when creating 3D floor planes, there is no
// need to create new ones with R_CheckPlane, because 3D floor planes
// are created by subsector and there is no way a subsector can graphically
// overlap.
// Called from R_StoreWallRange for ffloors.
void R_ExpandPlane(visplane_t*  pl, int start, int stop)
{
#if 1
    // [WDJ] 6/22/2010 simpler code that has same result.
    // Set visplane to union of visplane range and start..stop
    if (start < pl->minx)  pl->minx = start;
    if (stop > pl->maxx)   pl->maxx = stop;
#else
    // [WDJ] 6/22/2010 intrl, intrh vars and FOR loop are irrelevant to result
    // intersect of the ranges
    int         intrl;
    int         intrh;
    // union of the ranges
    int         unionl;
    int         unionh;

    // (unionl,intrl) = minmax( pl->minx, start )
    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    // (intrh,unionh) = minmax( pl->maxx, stop )
    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

#if 0
    // This code is only useful as a check that calls I_Error, because
    // the exact same result is always set after it.

    // Find any x in start..stop range where have valid top[], thus overlaps.
    int x;
    for (x = start ; x <= stop ; x++)
#ifdef DYNAMIC_VISPLANE_COVER
        if (pl->cover[x].top != TOP_MAX)
            break;
#else     
        if (pl->top[x] != TOP_MAX)
            break;
#endif

    //SoM: 3/23/2000: Boom code
    if (x > stop)
      pl->minx = unionl, pl->maxx = unionh;
//    else
//      I_Error("R_ExpandPlane: planes in same subsector overlap?!\nminx: %i, maxx: %i, start: %i, stop: %i\n", pl->minx, pl->maxx, start, stop);
#endif
    pl->minx = unionl, pl->maxx = unionh;
#endif   
}


//
// R_MakeSpans
//
// Draw plane spans at rows (t1..b1), span=(spanstart..x-1)
//    except when disabled by t1>viewheight
// Setup spanstart for next span at rows (t2..b2),
//    except when disabled by t2>viewheight
// at planeheight, using spanfunc
//  t1, b1: top and bottom y at x-1
//  t2, b2: top and bottom y at x
void R_MakeSpans ( int x, int t1, int b1, int t2, int b2 )
{
    // [WDJ] 11/10/2009  Fix crash in 3DHorror wad, sloppy limit checks on
    // spans caused writes to spanstart[] to overwrite yslope array.
    int lim;

    // Draw the spans over (t1..b1), skipping (t2..b2) which will be
    // drawn with (t2..b2) as one span.

    if( b1 >= rdraw_viewheight)
       b1 = rdraw_viewheight-1;
    if( b2 >= rdraw_viewheight)
       b2 = rdraw_viewheight-1;
   
    // Draw the spans over (t1..b1), up to but not including t2
    // If t2>rdraw_viewheight, then not valid and non-overlapping
    lim = b1+1; // not including
    if( t2 < lim )   lim = t2;		// valid and overlapping
    // unnecessary to limit lim to rdraw_viewheight if limit b1
    while (t1 < lim)  //  while (t1 < t2 && t1<=b1)
    {
        R_MapPlane (t1,spanstart[t1],x-1);  // y=t1, x=(spanstart[t1] .. x-1)
        t1++;
    }
   
    // Continue drawing (t1..b1), from b1, down to but not including b2.
    // If t2>rdraw_viewheight (disabled), then previous loop did it all
    // already and completed with t1<b1
    lim = t1-1;  // not including, if t1 invalid then is disabling
    if( b2 > lim )   lim = b2;		// valid and overlapping
    // unnecessary to limit lim to rdraw_viewheight, must limit b1 instead
    while (b1 > lim)  //  while (b1 > b2 && b1>=t1)
    {
       R_MapPlane (b1,spanstart[b1],x-1);	// y=b1, x=(spanstart[b1] .. x-1)
       b1--;
    }
   
    // Init spanstart over next span (t2..b2) that is not within (t1..b1)
    // Within (t1..b1) will use existing spanstart to draw this span
    // combined with other deferred span draws.

    // Init spanstart over (t2..b2) where less than t1.
    lim = b2+1;  // not including
    if( t1 < lim )   lim = t1;		// valid and overlapping
    // unnecessary to limit lim to rdraw_viewheight if limit b2
    // unnecessary to limit t2, as it is set from unsigned
    // loop only if t2<rdraw_viewheight, because b2 is limited
    while (t2 < lim)  // while (t2 < t1 && t2<=b2)
    {
        spanstart[t2] = x;
        t2++;
    }
   
    // Init spanstart over (t2..b2) where greater than b1.
    lim = t2-1;  // not including, if t2 invalid, then is disabling
    if( b1 > lim )   lim = b1;		// valid and overlapping
    if( lim < -1 )   lim = -1;
    while( b2 > lim )  //  while (b2 > b1 && b2>=t2)
    {
        spanstart[b2] = x;
        b2--;
    }
}



byte* R_GetFlat (int  flatnum);

// Draw the visplanes list
void R_Draw_Planes (void)
{
    visplane_t*         pl;
    int                 angle;
    int                 i; //SoM: 3/23/2000

    spanfunc = basespanfunc;
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif

    // over all visplane in hash table, following the linked lists
    for (i=0; i<VISPL_HASHSIZE; i++)
    for (pl=vispl_hashtab[i]; pl; pl=pl->next)
    {
        // sky flat
        if (pl->picnum == sky_flatnum)
        {
//            texture_render_t * sky_texren = & texture_render[ skytexture ];
            int yl_min = sky_yl_min_oc + centery;
            int yh_max = sky_yh_max_oc + centery;
            // combine tests
            if ( yl_min < 0 )   yl_min = 0;
            if ( yh_max >= rdraw_viewheight )   yh_max = rdraw_viewheight - 1;

            //added:12-02-98: use correct aspect ratio scale
            dc_iscale = sky_scale;

// Kik test non-moving sky .. weird
// cy = centery;
// centery = (rdraw_viewheight/2);

            // [WDJ] Invul sky bug fix from PrBoom.
            // Vanilla: Sky is always drawn full bright, i.e. colormaps[0],
            // thus the sky is not affected by INVUL inverse mapping.
            if(cv_invul_skymap.EV && fixedcolormap)
                dc_colormap = fixedcolormap;
            else
                dc_colormap = reg_colormaps;  // [0]

            // Sky has dedicated texture.
            dc_texturemid = sky_texturemid;
            dc_texheight = sky_height;  // passed to R_DrawColumn when 128 high sky
#if 1
            // Draw sky and sky flats in loop
            for (dc_x=pl->minx ; dc_x <= pl->maxx ; dc_x++)
            {
#ifdef DYNAMIC_VISPLANE_COVER
                dc_yl = pl->cover[dc_x].top;
                dc_yh = pl->cover[dc_x].bottom;
#else
                dc_yl = pl->top[dc_x];
                dc_yh = pl->bottom[dc_x];
#endif

                if (dc_yl <= dc_yh && dc_yh >= 0 && dc_yl < rdraw_viewheight )
                {
                    angle = (viewangle + x_to_viewangle[dc_x])>>ANGLETOSKYSHIFT;
                    angle &= sky_widthmask;

                    //[WDJ] phobiata.wad has many views that need clipping
                    if( dc_yl < yl_min )
                    {
                        // Above sky
                        // This part of sky should be drawn as a flat.
                        // Any pattern tiled looks to be vertical.
                        if( dc_yl < 0 )  dc_yl = 0;
                        dc_source = & skytop_flat[ (angle & (SKY_FLAT_WIDTH-1)) ][0];
                        dc_texheight = SKY_FLAT_HEIGHT;
                        skydrawerfunc[0] ();  // tiled
                        dc_texheight = sky_height;  // restore for sky_pict
                        dc_yl = yl_min;
                    }
                    if( dc_yh > yh_max )
                    {
                        // Below sky
                        // This part of sky should be ground drawn as a flat.
                        // Any pattern tiled looks to be vertical.
                        if( dc_yh > rdraw_viewheight-1 )  dc_yh = rdraw_viewheight-1;		       
                        dc_source = & ground_flat[ (angle & (SKY_FLAT_WIDTH-1)) ][0];
                        dc_texheight = SKY_FLAT_HEIGHT;
                        skydrawerfunc[0] ();  // tiled
                        dc_texheight = sky_height;  // restore for sky_pict
                        dc_yh = yh_max;
                    }
                    // sky_pict has colofs array
                    dc_source = & sky_pict[ ((uint32_t*)sky_pict)[ angle ] ];
                    skycolfunc ();
                }
            }
#else
          // old sky draw, for reference
            int x;
            for (x=pl->minx ; x <= pl->maxx ; x++)
            {
#ifdef DYNAMIC_VISPLANE_COVER
                dc_yl = pl->cover[x].top;
                dc_yh = pl->cover[x].bottom;
#else
                dc_yl = pl->top[x];
                dc_yh = pl->bottom[x];
#endif

                if (dc_yl <= dc_yh && dc_yh >= 0 && dc_yl < rdraw_viewheight )
                {
                    //[WDJ] phobiata.wad has many views that need clipping
                    if ( dc_yl < 0 )   dc_yl = 0;
                    if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
                    angle = (viewangle + x_to_viewangle[x])>>ANGLETOSKYSHIFT;
                    dc_x = x;
                    // Sky has dedicated texture.
                    dc_source = ((byte*)sky_patch) + sky_patch->columnofs[ angle & sky_widthmask ];
                    skycolfunc ();
                }
            }
#endif
// centery = cy;
            continue;
        }

        if(pl->ffloor)
          continue;

        R_DrawSinglePlane(pl);
    }
}

// ----
// Flat size_index tables for R_DrawSinglePlane.

// Fixed to int shift in draw.
// Indexed by flat size_index.
static byte      flat_fracbits_tab[ 8 ] =
{
    FRACBITS,  // 0
    FRACBITS - 5,  // 32x32 flat
    FRACBITS - 6,  // 64x64 flat
    FRACBITS - 7,  // 128x128 flat
    FRACBITS - 8,  // 256x256 flat
    FRACBITS - 9,  // 512x512 flat
    FRACBITS - 10,  // 1024x1024 flat
    FRACBITS - 11,  // 2048x2048 flat
};
// Flat Index mask
// Indexed by flat size_index.
static fixed_t  flat_imask_tab[ 8 ] =
{
    0, // 0
    (32<<FRACBITS) - 1, // 32x32 flat
    (64<<FRACBITS) - 1, // 64x64 flat
    (128<<FRACBITS) - 1, // 128x128 flat
    (256<<FRACBITS) - 1, // 256x256 flat
    (512<<FRACBITS) - 1, // 512x512 flat
    (1024<<FRACBITS) - 1, // 1024x1024 flat
    (2048<<FRACBITS) - 1, // 2048x2048 flat
};
// Flat Y address mask
// Indexed by flat size_index.
static fixed_t  flat_ymask_tab[ 8 ] =
{
    0, // 0
    ((32-1)<<5), // 32x32 flat
    ((64-1)<<6), // 64x64 flat
    ((128-1)<<7), // 128x128 flat
    ((256-1)<<8), // 256x256 flat
    ((512-1)<<9), // 512x512 flat
    ((1024-1)<<10), // 1024x1024 flat
    ((2048-1)<<11), // 2048x2048 flat
};


void R_DrawSinglePlane(visplane_t* pl)
{
  int  x;
  int  stop;
  int  angf;
  lightlev_t  addlight = (pl->extra_colormap && pl->extra_colormap->fog) ? extralight_cm : extralight;
  lightlev_t  vlight = pl->lightlevel;  // visible light 0..255

  if (pl->minx > pl->maxx)
    return;

  spanfunc = basespanfunc;
  if(pl->ffloor)
  {
    ffloor_t * ffp = pl->ffloor;
    if(ffp->flags & (FF_TRANSLUCENT|FF_FOG))
    {
      // Hacked up support for alpha value in software mode SSNTails 09-24-2002
      // [WDJ] 11-2012
      dr_alpha = ffp->alpha;
//      dr_alpha = fweff[ffp->fw_effect].alpha;
      ds_translucentmap = & translucenttables[ translucent_alpha_table[dr_alpha >> 4] ];

      if((ffp->flags & FF_FOG) && !(ffp->flags & FF_FLUID))
      {
          spanfunc = fogspanfunc; // R_DrawFogSpan_8 16 ..
          addlight = extralight_fog;
      }
      else
      {
          spanfunc = transspanfunc; // R_DrawTranslucentSpan_8 16 ..
          if( ! (pl->extra_colormap && pl->extra_colormap->fog))
          {
             addlight = 0;
             vlight = 255;
          }
      }
    }
  }
  vlight += addlight;

  if(viewangle != pl->viewangle)
  {
    memset (cachedheight, 0, sizeof(cachedheight));

    angf = ANGLE_TO_FINE(pl->viewangle - ANG90);

    base_scale_x = FixedDiv (finecosine[angf], projection_x);
    base_scale_y = -FixedDiv (finesine[angf], projection_x);
    viewangle = pl->viewangle;
  }

  vsp_currentplane = pl;


  // [WDJ] Flat use is safe from alloc, change to PU_CACHE at function exit.
  ds_source = (byte *) R_GetFlat (levelflats[pl->picnum].lumpnum);

  int sizeindex = levelflats[pl->picnum].size_index;
  flatfracbits = flat_fracbits_tab[sizeindex];
  flat_imask = flat_imask_tab[sizeindex];
  flat_ymask = flat_ymask_tab[sizeindex];

  xoffs = pl->xoffs;
  yoffs = pl->yoffs;
  planeheight = abs(pl->height - pl->viewz);

  planezlight =
      (vlight < 0) ?  zlight[0]
    : (vlight >= 255) ?  zlight[LIGHTLEVELS-1]
    : zlight[vlight>>LIGHTSEGSHIFT];

  //set the MAXIMUM value for unsigned short (but is not MAX for int)
  // mark the columns on either side of the valid area
#ifdef DYNAMIC_VISPLANE_COVER
  pl->cover[ pl->maxx+1 ].top = TOP_MAX;  // disable setup spanstart
  pl->cover[ pl->minx-1 ].top = TOP_MAX;  // disable drawing on first call
#else
  pl->top[pl->maxx+1] = TOP_MAX;  // disable setup spanstart
  pl->top[pl->minx-1] = TOP_MAX;  // disable drawing on first call
//  pl->bottom[pl->maxx+1] = 0;		// prevent interference from random value
//  pl->bottom[pl->minx-1] = 0;		// prevent interference from random value
#endif

  stop = pl->maxx + 1;

  for (x=pl->minx ; x<= stop ; x++)
  {
    R_MakeSpans( x,
#ifdef DYNAMIC_VISPLANE_COVER
                pl->cover[x-1].top, pl->cover[x-1].bottom,  // draw range (except first)
                pl->cover[x].top, pl->cover[x].bottom	// setup spanstart range
#else
                pl->top[x-1], pl->bottom[x-1],	// draw range (except first)
                pl->top[x], pl->bottom[x]	// setup spanstart range
#endif
                );
  }


  Z_ChangeTag (ds_source, PU_CACHE);
}


// Find limits of top[] and bottom[], to highest_top, lowest_bottom
void R_PlaneBounds(visplane_t* plane)
{
  int  i;
  uint16_t  hi, low;

#ifdef DYNAMIC_VISPLANE_COVER
  hi = plane->cover[plane->minx].top;
  low = plane->cover[plane->minx].bottom;
#else
  hi = plane->top[plane->minx];
  low = plane->bottom[plane->minx];
#endif

  for(i = plane->minx + 1; i <= plane->maxx; i++)
  {
    // in screen coord, where 0 is top (hi)
#ifdef DYNAMIC_VISPLANE_COVER
    if( hi > plane->cover[i].top )
      hi = plane->cover[i].top;
    if( low < plane->cover[i].bottom )
      low = plane->cover[i].bottom;
#else
    if(plane->top[i] < hi)
      hi = plane->top[i];
    if(plane->bottom[i] > low)
      low = plane->bottom[i];
#endif
  }
  plane->highest_top = hi;     // highest top
  plane->lowest_bottom = low;  // lowest bottom
}
