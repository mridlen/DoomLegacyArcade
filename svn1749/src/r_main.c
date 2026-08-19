// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_main.c 1745 2025-04-10 09:36:34Z wesleyjohnson $
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
// $Log: r_main.c,v $
// Revision 1.29  2004/09/12 19:40:08  darkwolf95
// additional chex quest 1 support
//
// Revision 1.28  2004/04/18 12:40:15  hurdler
// Jive's request for saving screenshots
//
// Revision 1.27  2003/08/12 12:29:42  hurdler
// better translucent hud
//
// Revision 1.26  2003/08/11 13:50:01  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.25  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.24  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.23  2001/05/16 21:21:14  bpereira
// Revision 1.22  2001/03/30 17:12:51  bpereira
// Revision 1.21  2001/03/21 18:24:39  stroggonmeth
//
// Revision 1.20  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.19  2001/02/10 12:27:14  bpereira
//
// Revision 1.18  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.17  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.16  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.15  2000/09/28 20:57:17  bpereira
// Revision 1.14  2000/09/21 16:45:06  bpereira
// Revision 1.13  2000/08/31 14:30:56  bpereira
// Revision 1.12  2000/07/01 09:23:49  bpereira
// Revision 1.11  2000/04/30 10:30:10  bpereira
// Revision 1.10  2000/04/24 20:24:38  bpereira
// Revision 1.9  2000/04/18 17:39:39  stroggonmeth
// Revision 1.8  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.7  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.6  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.5  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.4  2000/03/06 15:15:54  hurdler
//
// Revision 1.3  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Rendering main loop and setup functions,
//       utility functions (BSP, geometry, trigonometry).
//      See tables.c, too.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h"   //faB(21jan):testing
#include "r_sky.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "p_local.h"
#include "t_func.h"
#include "am_map.h"
#include "d_main.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif


// #define DEBUG_FIT_RATIO

//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
long long mycount;
long long mytotal = 0;
//unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


// Fineangles in the SCREENWIDTH wide window.  FINE_ANG90=2048.
#define FINE_FIELDOFVIEW    2048



int                     viewangleoffset;

// increment every time a check is made
int                     validcount = 1;

// center of perspective projection, in screen coordinates, 0=top
int                     centerx;
int                     centery;
int                     centerypsp;     //added:06-02-98:cf R_DrawPSprite

// center of perspective projection, in screen coordinates << FRACBITS
fixed_t                 centerxfrac;
fixed_t                 centeryfrac;

fixed_t                 projection_x;
//added:02-02-98:fixing the aspect ratio stuff...
fixed_t                 projection_y;

// just for profiling purposes
int                     framecount;

int                     sscount;
int                     linecount;
int                     loopcount;

// fog render gobals
uint16_t   fog_col_length;  // post_t column or picture height
uint16_t   fog_tic;    // 0..0xFFF, tic per fog change
byte	   fog_bltic;  // 0..31, blend/blur between tics
tic_t      fog_nexttic;
uint16_t   fog_wave1 = 0x200;   // 0..0x3FF, random small scale changes
uint16_t   fog_wave2 = 0x200;   // 0..0x3FF, random slower
byte       fog_index;  // 0.. column or texture height, for slow fog effect
byte	   fog_init = 0;   // set 1 at fog linedef to clear previous fog blur

// current viewer
// Set by R_SetupFrame

// Normally NULL, which allows normal colormap.
// Set to one lighttable_t entry of colormap table.
// pain=>REDCOLORMAP, invulnerability=>INVERSECOLORMAP, goggles=>colormap[1]
// Set from current viewer
lighttable_t*           fixedcolormap;

mobj_t *   viewmobj;

sector_t * viewer_sector;
int      viewer_modelsec;
boolean  viewer_has_model;
boolean  viewer_underwater;  // only set when viewer_has_model
boolean  viewer_at_water;    // viewer straddles the water plane
boolean  viewer_overceiling; // only set when viewer_has_model
boolean  viewer_at_ceiling;  // viewer straddles the ceiling plane

// Boom colormap, and global viewer coloring
lighttable_t*           view_colormap;  // full lightlevel range colormaps
extracolormap_t *       view_extracolormap;
ffloor_t *  view_fogfloor;  // viewer is in a FF_FOG floor
sector_t *  view_fogmodel;  // viewer is in a FF_FOG floor

fixed_t                 viewx;
fixed_t                 viewy;
fixed_t                 viewz;  // world coord

angle_t                 viewangle;
angle_t                 aimingangle;

fixed_t                 viewcos;
fixed_t                 viewsin;

player_t*               viewplayer;

// END current viewer

// 0 = high, 1 = low
int                     detailshift;

//
// precalculated math tables
//
angle_t                 clipangle, clipangle_x_2;

// The viewangle_to_x[viewangle + FINE_ANG90] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int                     viewangle_to_x[FINE_ANG180];

// The x_to_viewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t                 x_to_viewangle[MAXVIDWIDTH+1];


lighttable_t*           scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*           scalelightfixed[MAXLIGHTSCALE];
lighttable_t*           zlight[LIGHTLEVELS][MAXLIGHTZ];

//SoM: 3/30/2000: Hack to support extra boom colormaps.
int                     num_extra_colormaps = 0;
extracolormap_t         extra_colormaps[MAXCOLORMAPS];

// bumped light from gun blasts
lightlev_t  extralight;	     // extralight seen by most draws
lightlev_t  extralight_fog;  // partial extralight used by FF_FOG
lightlev_t  extralight_cm;   // partial extralight used by colormap->fog

consvar_t cv_chasecam       = {"chasecam","0",0,CV_OnOff};

consvar_t cv_psprites       = {"playersprites","1",0,CV_OnOff};
consvar_t cv_perspcorr      = {"perspectivecrunch","0",0,CV_OnOff};
consvar_t cv_tiltview       = {"tiltview","0",0,CV_OnOff};

CV_PossibleValue_t viewsize_cons_t[]={{3,"MIN"},{12,"MAX"},{0,NULL}};
CV_PossibleValue_t detaillevel_cons_t[]={{0,"High"},{1,"Low"},{0,NULL}};

consvar_t cv_viewsize       = {"viewsize","10",CV_SAVE|CV_CALL,viewsize_cons_t,R_SetViewSize};      //3-12
consvar_t cv_detaillevel    = {"detaillevel","0",CV_SAVE|CV_CALL,detaillevel_cons_t,R_SetViewSize}; // UNUSED
consvar_t cv_scalestatusbar = {"scalestatusbar","0",CV_SAVE|CV_CALL,CV_YesNo,R_SetViewSize};

#ifdef FIT_RATIO
CV_PossibleValue_t viewfit_cons_t[]={{0,"AUTO"},{1,"Stretch"},{2,"Fit width"},{3,"Fit height"},{0,NULL}};
consvar_t cv_viewfit = {"viewfit","0", CV_SAVE|CV_CALL, viewfit_cons_t, R_SetViewSize};
#endif

consvar_t cv_corr_clip_width = {"corrclipwidth", "0", 0, CV_OnOff};

CV_PossibleValue_t grtranslucenthud_cons_t[] = { {1, "MIN"}, {255, "MAX"}, {0, NULL} };
consvar_t cv_grtranslucenthud = {"gr_translucenthud","255",CV_SAVE|CV_CALL,grtranslucenthud_cons_t,R_SetViewSize}; //Hurdler: support translucent HUD

CV_PossibleValue_t screenshot_type_cons_t[]={{0,"Compact"},{1,"Full"},
#ifdef SMIF_PC_DOS
   {8,"PCX"},
#endif
   {0,NULL}};
consvar_t cv_screenshot_type = { "screenshottype", "", CV_SAVE, NULL };
consvar_t cv_screenshot_dir = { "screenshotdir", "", CV_SAVE, NULL };

// Boom Colormap Selection
CV_PossibleValue_t boom_colormap_cons_t[]={
   {0,"Sector visible"},  // Sector colormap rules
   {1,"Boom visible"},  // Boom colormap, visible
   {2,"Boom normal" },  // Boom colormap rules
   {3,"Boom detect"},    // Detect Boom features
   {0,NULL} };
void BoomColormap_detect(void);
consvar_t cv_boom_colormap   = {"boomcolormap", "0", CV_SAVE|CV_CALL, boom_colormap_cons_t, BoomColormap_detect};

byte EN_boom_colormap = 1;  // compatibility, user preference
void BoomColormap_detect(void)
{
    switch( cv_boom_colormap.EV )
    {
     default:
     case 0:  // sector colormap
       EN_boom_colormap = 0;  // Legacy sector colormap
       break;
     case 1:  // Boom colormap
       EN_boom_colormap = 1;  // visible colormap
       break;
     case 2:  // Boom visible colormap
       EN_boom_colormap = 2;  // full boom compatibility
       break;
     case 3:  // Boom detect
       EN_boom_colormap = (all_detect & BDTC_boom) ? ( (all_detect & BDTC_legacy) ? 1 : 2 ) : 0;
       break;
    }
}

CV_PossibleValue_t invul_skymap_cons_t[]={
   {0,"Vanilla"},  // Vanilla no colormap change
   {1,"Boom"},     // Boom colormap change
   {0,NULL} };
consvar_t cv_invul_skymap  = {"invul_skymap", "1", CV_SAVE, invul_skymap_cons_t};

// matches fogwater_effect_e
CV_PossibleValue_t  fogwater_effect_cons_t[]={
   {FW_colormap,"Colormap" },
   {FW_clear,"Clear"},
   {FW_cast,"Cast"},
   {FW_fogfluid,"Fog Fluid"},
   {FW_inside,"Inside"},
   {FW_foglite,"Fog Lite"},
   {FW_fogdust,"Fog Dust"},
   {FW_fogsheet,"Fog Sheet"},
   {0,NULL} };
consvar_t cv_water_effect = {"watereffect", "Clear", CV_SAVE|CV_CALL, fogwater_effect_cons_t, R_FW_config_update};
consvar_t cv_fog_effect = {"fogeffect", "Fog Dust", CV_SAVE|CV_CALL, fogwater_effect_cons_t, R_FW_config_update};

// added 16-6-98:splitscreen

void SplitScreen_OnChange(void);

consvar_t cv_splitscreen = {"splitscreen","0",CV_CALL ,CV_OnOff,SplitScreen_OnChange};

void SplitScreen_OnChange(void)
{
    // recompute screen size
    R_ExecuteSetViewSize();

    // change the menu
    M_Player2_MenuEnable( cv_splitscreen.EV );

    if( !demoplayback)
    {
        CL_Splitscreen_Player_Manager();

        if(server && !netgame)
            multiplayer = (D_NumLocalPlayers() > 1);  // [Arcade] not just the split
    }
    else
    {
        // multiplayer demo ??
        int i;
        // [WDJ] Do not break splitscreen setups when a demo runs
        if( displayplayer2_ptr )  return;  // already have a player2
       
        // [WDJ] This code does not guarantee that the demo will get
        // a player2, thus all code using player2 cannot assume that
        // displayplayer2_ptr is valid, even when cv_splitscreen.

        // find any player to be player2
        for( i=0; i<MAXPLAYERS;i++)
        {
            if( playeringame[i] && i!=consoleplayer )
            {
                displayplayer2 = i;
                displayplayer2_ptr = &players[displayplayer2];
                break;
            }
        }
    }
}

//
// R_PointOnSide
// Called by Traverse BSP (sub) tree, check point against partition plane.
// Returns side 0 (front) or 1 (back).
int R_PointOnSide ( fixed_t x, fixed_t y, node_t* node )
{
    fixed_t     dx, dy;
    fixed_t     left, right;

    if (node->dx == 0)
    {
        return ((x <= node->x)? (node->dy > 0) : (node->dy < 0))? 1 : 0;
    }
    if (node->dy == 0)
    {
        return ((y <= node->y)? (node->dx < 0) : (node->dx > 0))? 1 : 0;
    }

    dx = (x - node->x);
    dy = (y - node->y);

    // Decide quickly by looking at sign bits.
    // [WDJ] instead of &0x80000000, use direct sign tests which optimize
    // better and are not type size dependent.
    if ( (node->dy ^ node->dx ^ dx ^ dy) < 0 )
    {
        // (left is negative) returns back side
        return ((node->dy ^ dx) < 0 )? 1 : 0;  // return side
    }

    left = FixedMul ( node->dy>>FRACBITS , dx );
    right = FixedMul ( dy , node->dx>>FRACBITS );

    // (right < left) is front side (0), otherwise back side (1).
    return (right < left) ? 0 : 1;  // return side
}


// Returns side 0 (front) or 1 (back).
int R_PointOnSegSide ( fixed_t x, fixed_t y, seg_t* line )
{
    fixed_t     dx, dy;
    fixed_t     left, right;

    fixed_t lx = line->v1->x;
    fixed_t ly = line->v1->y;

    fixed_t ldx = line->v2->x - lx;
    fixed_t ldy = line->v2->y - ly;

    if (ldx == 0)
    {
        return ((x <= lx)? (ldy > 0) : (ldy < 0))? 1 : 0;
    }
    if (ldy == 0)
    {
        return ((y <= ly)? (ldx < 0) : (ldx > 0))? 1 : 0;
    }

    dx = (x - lx);
    dy = (y - ly);

    // Decide quickly by looking at sign bits.
    // [WDJ] instead of &0x80000000, use direct sign tests which optimize
    // better and are not type size dependent.
    if ( (ldy ^ ldx ^ dx ^ dy) < 0 )
    {
        // (left is negative) returns back side
        return ((ldy ^ dx) < 0 )? 1 : 0;  // return side
    }

    left = FixedMul ( ldy>>FRACBITS , dx );
    right = FixedMul ( dy , ldx>>FRACBITS );
    // (right < left) is front side (0), otherwise back side (1).
    return (right < left) ? 0 : 1;  // return side
}


// [WDJ] Generate the tan slope, suitable for tantoangle[] index.
// This is more accuate than the vanilla version,
// but would cause sync loss on demos if used everywhere.
int SlopeDiv_64 (fixed_t num, fixed_t den)
{
    int64_t ans;

    if (den < 64)
        return SLOPERANGE;  // max

    ans = (((int64_t)num) << 11) / den;
    return (ans <= SLOPERANGE) ? ans : SLOPERANGE;  // max
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
// the coordinates are flipped until they are in the first octant of
// the coordinate system, then the y (<=x) is scaled and divided by x
// to get a tangent (slope) value which is looked up in the
// tantoangle[] table.

// Point (x2,y2) to point (x1,y1) angle.
angle_t R_PointToAngle2 ( fixed_t  x2, fixed_t  y2,
                          fixed_t  x1, fixed_t  y1)
{
    // [WDJ] This is inaccurate. Angles can be in error by 0x10000000,
    // and not monotonic (ordering errors).
    // Has 5 bits correct when compared to atan2().
    angle_t ra = 0;
    x1 -= x2;  // diff
    y1 -= y2;

    if ( (x1 == 0) && (y1 == 0) )
        return 0;

    if (x1>= 0)
    {   // x >=0
        if (y1>= 0)
        {   // y>= 0
            ra = (x1>y1)? 
                // octant 0, ra = 0..ANG45
                tantoangle[ SlopeDiv(y1,x1)]
             :
                // octant 1, ra = ANG45..ANG90
                ANG90-1 - tantoangle[ SlopeDiv(x1,y1)] ;
        }
        else
        {   // y<0
            y1 = -y1;
            ra = (x1>y1)?
                // octant 8, ra = ANG315..0 due to angle wrap
                -tantoangle[ SlopeDiv(y1,x1)]
             :
                // octant 7, ra = AN270..ANG315
                ANG270 + tantoangle[ SlopeDiv(x1,y1)] ;
        }
    }
    else
    {   // x<0
        x1 = -x1;
        if (y1>= 0)
        {   // y>= 0
            ra = (x1>y1)?
                // octant 3, ra = ANG135..ANG180
                ANG180-1 - tantoangle[ SlopeDiv(y1,x1)]
             :
                // octant 2, ra = ANG90..ANG135
                ANG90 + tantoangle[ SlopeDiv(x1,y1)] ;
        }
        else
        {   // y<0
            y1 = -y1;
            ra = (x1>y1)?
                // octant 4, ra = AN180..ANG225
                ANG180 + tantoangle[ SlopeDiv(y1,x1)]
             :
                 // octant 5, ra = ANG225..ANG270
                ANG270-1 - tantoangle[ SlopeDiv(x1,y1)] ;
        }
    }
    return ra;
}


// Point of view (viewx,viewy) to point (x1,y1) angle.
angle_t R_PointToAngle ( fixed_t x, fixed_t y)
{
    // Has 13 bits correct when compared to atan2(), which is much
    // better than the 5 correct bits of the vanilla function.
    // Uses the more accurate SlopeDiv_64.
    angle_t vpa = 0;

    x -= viewx;  // diff from viewpoint
    y -= viewy;

    if ( (x == 0) && (y == 0) )
        return 0;
   
    // [WDJ] Fix from PrBoom (e6y).
    // For large x or y, resort to the slower but accurate lib function.
    if (  x > FIXED_MAX/4 || x < -FIXED_MAX/4
          || y > FIXED_MAX/4 || y < -FIXED_MAX/4 )
    {
        // PrBoom used a 1 point cache, but that is too small.
        return (int) (atan2(y,x) * ANG180 / M_PI);
    }

    if (x>= 0)
    {   // x >=0
        if (y>= 0)
        {   // y>= 0
            vpa = (x>y)? 
                // octant 0, vpa = 0..ANG45
                tantoangle[ SlopeDiv_64(y,x)]
             :
                // octant 1, vpa = ANG45..ANG90
                ANG90-1 - tantoangle[ SlopeDiv_64(x,y)] ;
        }
        else
        {   // y<0
            y = -y;
            vpa = (x>y)?
                // octant 8, vpa = ANG315..0 due to angle wrap
                -tantoangle[ SlopeDiv_64(y,x)]
             :
                // octant 7, vpa = AN270..ANG315
                ANG270 + tantoangle[ SlopeDiv_64(x,y)] ;
        }
    }
    else
    {   // x<0
        x = -x;
        if (y>= 0)
        {   // y>= 0
            vpa = (x>y)?
                // octant 3, vpa = ANG135..ANG180
                ANG180-1 - tantoangle[ SlopeDiv_64(y,x)]
             :
                // octant 2, vpa = ANG90..ANG135
                ANG90 + tantoangle[ SlopeDiv_64(x,y)] ;
        }
        else
        {   // y<0
            y = -y;
            vpa = (x>y)?
                // octant 4, vpa = AN180..ANG225
                ANG180 + tantoangle[ SlopeDiv_64(y,x)]
             :
                 // octant 5, vpa = ANG225..ANG270
                ANG270-1 - tantoangle[ SlopeDiv_64(x,y)] ;
        }
    }
    return vpa;
}


fixed_t R_PointToDist2 ( fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1)
{
    angle_t     angle;
    fixed_t     dist;

    fixed_t dx = abs(x1 - x2);
    fixed_t dy = abs(y1 - y2);

    if (dy>dx)
    {
        // swap dx and dy
        register fixed_t dx2 = dx;
        dx = dy;
        dy = dx2;
    }
    if(dy==0)
       return dx;

    angle = tantoangle[ FixedDiv(dy,dx)>>DBITS ] + ANG90;

    // use as cosine
    dist = FixedDiv (dx, sine_ANG(angle));
    return dist;
}


//SoM: 3/27/2000: Little extra utility. Works in the same way as
//R_PointToAngle2
fixed_t R_PointToDist ( fixed_t x, fixed_t y )
{
  return R_PointToDist2(viewx, viewy, x, y);
}


void R_Init_PointToAngle (void)
{
    // UNUSED - now getting from tables.c
#if 0
    int i;
    long        t;
    float       f;
//
// slope (tangent) to angle lookup
//
    for (i=0 ; i<=SLOPERANGE ; i++)
    {
        f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
        t = 0xffffffff*f;
        tantoangle[i] = t;
    }
#endif
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
//added:02-02-98:note: THIS IS USED ONLY FOR WALLS!
// Called from R_StoreWallRange.
fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
    fixed_t             scale;
    fixed_t             num;
    fixed_t             den;

    angle_t anglea = ANG90 + (visangle - viewangle);
    angle_t angleb = ANG90 + (visangle - rw_normalangle);

    // both sines are always positive
    fixed_t sinea = sine_ANG(anglea);
    fixed_t sineb = sine_ANG(angleb);
    //added:02-02-98:now uses projection_y instead of projection for
    //               correct aspect ratio!
    num = FixedMul(projection_y, sineb)<<detailshift;
    den = FixedMul(rw_distance, sinea);

    // [WDJ] Normal path has no branches, faster.
    if( den <= num>>16 )  goto max_scale;
    scale = FixedDiv (num, den);

    if( scale > 64*FRACUNIT )  goto max_scale;
    if( scale < 256 )  goto min_scale;
ret:   
    return scale;

max_scale:
    scale = 64*FRACUNIT;
    goto ret;
     
min_scale:
    scale = 256;
    goto ret;
}



#if 0
void R_Init_Tables (void)
{
    // UNUSED: now getting from tables.c
    int         i;
    float       a;
    float       fv;
    int         t;

    // viewangle tangent table
    for (i=0 ; i<FINE_ANG180 ; i++)
    {
        a = (i-FINE_ANG90+0.5)*PI*2/FINEANGLES;
        fv = FRACUNIT*tan (a);
        t = fv;
        finetangent[i] = t;
    }

    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
        // OPTIMIZE: mirror...
        a = (i+0.5)*PI*2/FINEANGLES;
        t = FRACUNIT*sin (a);
        finesine[i] = t;
    }
}
#endif

// consvar_t cv_fov = {"fov","2048", CV_CALL | CV_NOINIT, NULL, R_ExecuteSetViewSize};

//
// R_Init_TextureMapping
//
// Called by R_ExecuteSetViewSize
void R_Init_TextureMapping (void)
{
    int  i;
    int  x;
    int  t;
    fixed_t  focallength;

    // Use tangent table to generate viewangle_to_x:
    //  viewangle_to_x will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv (projection_x,
                            finetangent[(FINE_FIELDOFVIEW/2) + FINE_ANG90] );
//                          finetangent[ cv_fov.value + FINE_ANG90] );

    for (i=0 ; i<FINE_ANG180 ; i++)
    {
        if (finetangent[i] > FRACUNIT*2)
            t = -1;
        else if (finetangent[i] < -FRACUNIT*2)
            t = rdraw_viewwidth+1;
        else
        {
            t = FixedMul (finetangent[i], focallength);
            t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

            if (t < -1)
                t = -1;
            else if (t>rdraw_viewwidth+1)
                t = rdraw_viewwidth+1;
        }
        viewangle_to_x[i] = t;
    }

    // Scan viewangle_to_x[] to generate x_to_viewangle[]:
    //  x_to_viewangle will give the smallest view angle
    //  that maps to x.
    for (x=0;x<=rdraw_viewwidth;x++)
    {
        i = 0;
        while (viewangle_to_x[i]>x)
            i++;
        x_to_viewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

    // Take out the fencepost cases from viewangle_to_x.
    for (i=0 ; i<FINE_ANG180 ; i++)
    {
        t = FixedMul (finetangent[i], focallength);
        t = centerx - t;

        if (viewangle_to_x[i] == -1)
            viewangle_to_x[i] = 0;
        else if (viewangle_to_x[i] == rdraw_viewwidth+1)
            viewangle_to_x[i]  = rdraw_viewwidth;
    }

    clipangle = x_to_viewangle[0];
    clipangle_x_2 = clipangle + clipangle;
}



//
// R_Init_LightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP         2

static
void R_Init_LightTables (void)
{
    int         i;
    int         j;
    int         level;
    int         startmap;
    int         scale;

    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTZ ; j++)
        {
            //added:02-02-98:use BASEVIDWIDTH, vid.width is not set already,
            // and it seems it needs to be calculated only once.
            scale = FixedDiv ((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
            scale >>= LIGHTSCALESHIFT;
            level = startmap - scale/DISTMAP;

            if (level < 0)
                level = 0;

            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;

            zlight[i][j] = & reg_colormaps[ LIGHTTABLE( level ) ];
        }
    }
}


// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
boolean         setsizeneeded = false;

// Called by R_Init, SCR_Recalc
// Called by cv_viewsize, cv_detaillevel, cv_scalestatusbar, cv_grtranslucenthud
void R_SetViewSize (void)
{
    setsizeneeded = true;
}


//
// R_ExecuteSetViewSize
//


// now uses screen variables cv_viewsize, cv_detaillevel
//
// Called by D_Display when setsizeneeded
// Called by G_DoLoadGame when setsizeneeded
// Called by SplitScreen_OnChange
// Called by cv_fov (disabled)
void R_ExecuteSetViewSize (void)
{
    fixed_t     cosadj;
    fixed_t     dy;
    int         i;
    int         j;
    int         level;
    int         startmap;

    int         setdetail;

//    int         aspectf_x;  //added:02-02-98:for aspect ratio calc. below...

    setsizeneeded = false;
    // no reduced view in splitscreen mode
    if( (D_NumViews() >= 2) && (cv_viewsize.value < 11) )
        CV_SetValue (&cv_viewsize, 11);

    // added by Hurdler
#ifdef HWRENDER
    if((rendermode != render_soft) && (cv_viewsize.value < 6))
        CV_SetValue (&cv_viewsize, 6);
#endif

    setdetail = cv_detaillevel.value;

    // status bar overlay at viewsize 11
    st_overlay_on = (cv_viewsize.value==11);
    //DarkWolf95: Support for Chex Quest
    if (gamemode == chexquest1)
       st_overlay_on = 0;  //don't allow overlay status bar in Chex Quest

    // clamp detail level (actually ignore it, keep it for later who knows)
    if (setdetail)
    {
        setdetail = 0;
        CONS_Printf ("lower detail mode n.a.\n");
        CV_SetValue (&cv_detaillevel,setdetail);
    }

    stbar_height = (EN_heretic)? H_STBAR_HEIGHT : ST_HEIGHT;

    if( cv_scalestatusbar.EV || (cv_viewsize.value>=11) )
        stbar_height *= (rendermode == render_soft)? vid.dupy : vid.fdupy;

#ifdef FIT_RATIO
    // [WDJ] vid.width, vid.height, are the fullscreen, or window full sizes.
    // For some screen shapes, and controls, the pages will not fill the full screen.
    // rdraw_viewwidth, rdraw_viewheight are the cropped draw area sizes.
    // They specify draw windows for splitscreen, and do not change the draw scaling.
    // Info pages will be drawn with integer scaling, to preserve the appearance, using
    // the various dup variables.
#endif
    
    //added 01-01-98: full screen view, without statusbar
    if (cv_viewsize.value > 10)
    {
        rdraw_scaledviewwidth = vid.width;
        rdraw_viewheight = vid.height;
    }
    else
    {
        //added 01-01-98: always a multiple of eight
        rdraw_scaledviewwidth = (cv_viewsize.value*vid.width/10)&~7;
        //added:05-02-98: make rdraw_viewheight multiple of 2 because sometimes
        //                a line is not refreshed by R_DrawViewBorder()
        rdraw_viewheight = (cv_viewsize.value * (vid.height - stbar_height) / 10)&~1;
    }

    // added 16-6-98:splitscreen
    // [Arcade] Any multi-view layout halves the height; the 2x2 grid halves
    // the width too, but only the hardware renderer can place those columns
    // (r_draw.c has just the two stacked ylookup tables), so the software
    // renderer keeps drawing full width and shows at most two views.
    if( D_NumViews() >= 2 )
        rdraw_viewheight >>= 1;

    detailshift = setdetail;
    rdraw_viewwidth = rdraw_scaledviewwidth>>detailshift;

    centery = rdraw_viewheight/2;
    centerx = rdraw_viewwidth/2;
    centerxfrac = centerx<<FRACBITS;
    centeryfrac = centery<<FRACBITS;

#ifdef FIT_RATIO
    // For split screen, draw window is vid.width, rdraw_viewheight.
#ifdef DEBUG_FIT_RATIO
    GenPrintf(EMSG_debug, "vid(%i,%i)   rdraw(%i,%i)\n", vid.width, vid.height, rdraw_viewwidth, rdraw_viewheight );
#endif
    byte viewfit_ev = cv_viewfit.EV;
    if( viewfit_ev == 0 )  // auto
    {
        // The draw for 320,200 ratio is different, as those pixels are not square.
        // For modern screens with square pixels, the ratio of 800,600 applies.
        int r_width = 600 * vid.width / vid.height;

        if( vid.width == 320 )  goto std_fit;
        if( r_width > 840  )  // wide screen
        {
            viewfit_ev = 2; // fit width
        }
        else if( r_width < 760 )  // narrow screen
        {
            viewfit_ev = 3; // fit height
        }
        else
        {
std_fit:
            // standard screen proportions
            viewfit_ev = 1; // fit both (stretches)
        }
    }
#endif   

#ifdef FIT_RATIO
#ifdef DEBUG_FIT_RATIO
    unsigned int base_ratio = ((BASEVIDWIDTH << 16) / BASEVIDHEIGHT) + 1;  // will be rounded
    GenPrintf(EMSG_debug, " base_ratio %i\n", base_ratio );
#endif
    vid.fit_width = rdraw_viewwidth;
    vid.fit_height = vid.height;   // not rdraw_viewheight because of splitwindow
    switch( viewfit_ev )
    {
     default:
     case 1:  // fit both (stretch)
        break;
     case 2:  // fit width
        vid.fit_height = rdraw_viewwidth * BASEVIDHEIGHT / BASEVIDWIDTH;
#ifdef DEBUG_FIT_RATIO
    GenPrintf(EMSG_debug, "2> fit_height %i\n", vid.fit_height );
#endif
        break;
     case 3:  // fit height
        vid.fit_width = rdraw_viewheight * BASEVIDWIDTH / BASEVIDHEIGHT;
#ifdef DEBUG_FIT_RATIO
    GenPrintf(EMSG_debug, "3> fit_width %i\n", vid.fit_width );
#endif
        break;
    }
#endif   

    //added:01-02-98:aspect ratio is now correct, added an 'projection_y'
    //      since the scale is not always the same between horiz. & vert.
#ifdef FIT_RATIO
    projection_x = (vid.fit_width/2) << FRACBITS;
//    projection_y = (((vid.fit_width/2*vid.fit_height*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.fit_width)<<FRACBITS;
    projection_y = ((vid.fit_height * BASEVIDWIDTH)/(BASEVIDHEIGHT * 2))<<FRACBITS;
    //added:02-02-98:now correct aspect ratio!
    // [WDJ] Replace aspectf_x with projection_y.  When aspect ratio is correct, they have same value.
    // aspectf_x = (((centerx*vid.fit_height*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.fit_width) * FRACUNIT;
#ifdef DEBUG_FIT_RATIO
    GenPrintf(EMSG_debug, " (%i,%i) centerxfrac=%X aspectf_x=%X  projection_x=%X projection_y=%X\n", vid.fit_width, vid.fit_height, centerxfrac, aspectf_x, projection_x, projection_y );
#endif
#else   
    projection_x = centerxfrac; // (rdraw_viewwidth/2) << FRACBITS;
    projection_y = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width)<<FRACBITS;
    //added:02-02-98:now correct aspect ratio!
    aspectf_x = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width) * FRACUNIT;
        // used as  aspectx*FRACUNIT,  which is same as projection_y
#endif

    //
    // no more low detail mode, it used to setup the right drawer routines
    // for either detail mode here
    //
    // if (!detailshift) ... else ...

    R_Init_ViewBuffer (rdraw_scaledviewwidth, rdraw_viewheight);

    R_Init_TextureMapping ();

    // HWR_Init_TextureMapping is called by HWR_SetViewSize

    // psprite scales
    centerypsp = rdraw_viewheight/2;  //added:06-02-98:psprite pos for freelook
    // Doom weapons are too high for high resolutions.
    if( EN_doom_etc )
       centerypsp += (rdraw_viewheight/400);

    // pspriteyscale: aspect ratio for psprites
#ifdef FIT_RATIO_XX
    // [WDJ] DISABLED, This may leave a gap at bottom of screen.
    pspritescale  = (vid.fit_width<<FRACBITS)/BASEVIDWIDTH;
    pspriteiscale = (BASEVIDWIDTH<<FRACBITS)/vid.fit_width;   // x axis scale
    pspriteyscale = (((vid.fit_height*vid.fit_width)/vid.width)<<FRACBITS)/BASEVIDHEIGHT;
#ifdef DEBUG_FIT_RATIO
    GenPrintf(EMSG_debug, "fit %i: scale (%i,%i): %i  %i  %i\n", viewfit_ev, vid.fit_width, vid.fit_height, pspritescale, pspriteiscale, pspriteyscale );
#endif
#else
    // Scale weapon, so it always fits the screen boundaries.
    pspritescale  = (rdraw_viewwidth<<FRACBITS)/BASEVIDWIDTH;
    pspriteiscale = (BASEVIDWIDTH<<FRACBITS)/rdraw_viewwidth;   // x axis scale
    //added:02-02-98:now aspect ratio correct for psprites
    pspriteyscale = (((vid.height*rdraw_viewwidth)/vid.width)<<FRACBITS)/BASEVIDHEIGHT;
#endif

    // thing clipping
    for (i=0 ; i<rdraw_viewwidth ; i++)
    {
        clip_screen_top_min[i] = 0;
        clip_screen_bot_max[i] = rdraw_viewheight - 1;
    }

    // setup sky scaling for old/new skies (uses pspriteyscale)
    R_Set_Sky_Scale ();

    // planes
    if( rendermode == render_soft )
    {
        // this is only used for planes rendering in software mode
        j = rdraw_viewheight*4;
        for (i=0 ; i<j ; i++)
        {
            //added:10-02-98:(i-centery) became (i-centery*2) and centery*2=rdraw_viewheight
            dy = ((i-rdraw_viewheight*2)<<FRACBITS)+FRACUNIT/2;
            dy = abs(dy);
            // [WDJ] Replace aspectf_x with projection_y.  When aspect ratio is correct they have same value.
            yslopetab[i] = FixedDiv (projection_y, dy);
//            yslopetab[i] = FixedDiv (aspectf_x, dy);
        }
    }

    for (i=0 ; i<rdraw_viewwidth ; i++)
    {
        cosadj = abs( cosine_ANG( x_to_viewangle[i] ));
        distscale[i] = FixedDiv (FRACUNIT, cosadj);
    }

    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
            level = startmap - j*vid.width/(rdraw_viewwidth<<detailshift)/DISTMAP;

            if (level < 0)
                level = 0;

            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;

            scalelight[i][j] = & reg_colormaps[ LIGHTTABLE( level ) ];
        }
    }

    //faB: continue to do the software setviewsize as long as we use
    //     the reference software view
#ifdef HWRENDER // not win32 only 19990829 by Kin
    if( rendermode != render_soft )
        HWR_SetViewSize (cv_viewsize.value);
#endif

    stbar_recalc = true;
    am_recalc = true;
}


//
// R_Init
//


void R_Init (void)
{
#if defined DEBUG_WINDOWED || defined PARANOIA
    // [WDJ] Some checks that could not be done at compile-time (enum).
    if( MFT_TO_SKINMAP( 1<<MFT_TRANSSHIFT ) != (&skintranstables[ 0 ]) )
        I_Error( "MFT_TO_SKINMAP  broken, check MFT_TRANSSHIFT\n" );
    if( (int)MFT_TRANSSHIFT >= (int)MFO_TRANSSHIFT )
        I_Error( "MFO_TRANSSHIFT - MFT_TRANSSHIFT  broken, check MFT_TRANSHIFT\n" );
#endif

    if(dedicated)
        return;

    //added:24-01-98: screensize independent
    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Load_Data");
    R_Load_Data ();
    R_Load_Corona();

    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Init_PointToAngle");
    R_Init_PointToAngle ();

#if 0
    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Init_Tables");
    R_Init_Tables ();
#endif

    R_Init_ViewBorder ();

    R_SetViewSize ();   // setsizeneeded is set true

    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Init_Planes");
    R_Init_Planes ();

    //added:02-02-98: this is now done by SCR_Recalc() at the first mode set
    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Init_LightTables");
    R_Init_LightTables ();

    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Init_SkyMap");
    R_Init_SkyMap ();

    if(devparm)
        GenPrintf(EMSG_dev, "\nR_Init_TranslationsTables");
    R_Init_TranslationTables ();

    R_Init_draw_masked();

    framecount = 0;
}


//
// R_PointInSubsector
//
subsector_t* R_PointInSubsector ( fixed_t x, fixed_t y )
{
    node_t*     node;
    int         side;
    uint32_t    nodenum;

    // single subsector is a special case
    if (!numnodes)
        return subsectors;

    nodenum = numnodes - 1;  // 0..

    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_IsPointInSubsector, same of above but return 0 if not in subsector
//
subsector_t* R_IsPointInSubsector ( fixed_t x, fixed_t y )
{
    node_t*     node;
    int         side;
    subsector_t* ret;

    // single subsector is a special case
    if (!numnodes)
        return subsectors;

    uint32_t nodenum = numnodes - 1; // 0..

    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
    }

    ret=&subsectors[nodenum & ~NF_SUBSECTOR];
    uint32_t i;
    for(i=0; i<ret->numlines; i++)
    {
        if(R_PointOnSegSide(x,y,&segs[ret->firstline+i]))
            return 0;
    }

    return ret;
}


//
// R_SetupFrame
//

// WARNING : a should be unsigned but to add with 2048, it isn't !
#define AIMINGTODY(a) ((tangent_ANG(a)*160)>>FRACBITS)

// Setup viewx, viewy, viewz, and other view global variables.
// Called by R_RenderPlayerView.
// Called by HWR_RenderPlayerView.
//  pind : player index, [0]=main player, [1]=splitscreen player
void R_SetupFrame( byte pind, player_t* player )
{
    int  i;
    int  fixedcolormap_num;
    int  dy=0; //added:10-02-98:

    viewplayer = player;
    extralight = player->extralight;
    extralight_fog = extralight >> 1;  // 1/2 for FF_FOG
    extralight_cm = extralight - (extralight>>2);  // 3/4 for colormap->fog

    // Chase camera setting must be maintained even with script camera running
    if( cv_chasecam.EV )
    {
        // with splitplayer, only the first player will get the chase camera
        if( !camera.chase )
             P_ResetCamera(player);  // set chase = player
    }
    else
        camera.chase = NULL;

#ifdef FRAGGLESCRIPT
    // Script camera overrides chase camera
    if (script_camera_on)
    {
        // fragglescript camera as viewer
        viewmobj = script_camera.mo;
#ifdef PARANOIA
        if (!viewmobj)
            I_Error("no mobj for the camera");
#endif
        viewz = viewmobj->z;
        aimingangle=script_camera.aiming;
        viewangle = viewmobj->angle;
        ST_Palette0();  // Doom and Heretic
        fixedcolormap_num = camera.fixedcolormap;
    }
    else
#endif
    if (camera.chase == player)  // this player has chase camera
    {
        // chase camera as viewer
        // use outside cam view
        if (!camera.mo)  // because LoadLevel removes camera
            P_ResetCamera(player);  // reset the camera
        viewmobj = camera.mo;
        viewz = viewmobj->z + (viewmobj->height>>1);
        aimingangle=camera.aiming;
        viewangle = viewmobj->angle;
#if 1
        // Player cam does not see player status palette.
        ST_Palette0();   // Doom and Heretic
#else
        // Player cam sees player status palette.
        // Can now handle splitplayer flashes.
        if( EN_heretic )
            H_PaletteFlash( player );
        else
            ST_doPaletteStuff( player );
#endif
        fixedcolormap_num = camera.fixedcolormap;
    }
    else
    {
        // player as viewer
        // use the player's eyes view
        viewz = player->viewz;
#ifdef CLIENTPREDICTION2
        if( demoplayback || !player->spirit)
        {
            viewmobj = player->mo;
            CONS_Printf("\2No spirit\n");
        }
        else
            viewmobj = player->spirit;
#else
        viewmobj = player->mo;
#endif
        
        // Can now handle splitplayer flashes.
        if( EN_heretic )
            H_PaletteFlash( player );
        else
            ST_doPaletteStuff( player );
        fixedcolormap_num = player->fixedcolormap;

        aimingangle=player->aiming;
        viewangle = viewmobj->angle+viewangleoffset;

        if(!demoplayback && player->playerstate!=PST_DEAD && !cl_drone)
        {
            viewangle = localangle[pind]; // WARNING : camera use this
            aimingangle=localaiming[pind];
        }

#ifdef ENABLE_TIRED_RUN
        if( player->drown > (TICRATE*45) )  // 45 sec, upto 10500
        {
            // Drowning .. View goes dark.
            int16_t drown_darkness = (player->drown - (TICRATE*45)) / (TICRATE*7);  // warning
            if( player->drown > (TICRATE*90))  // 90 sec
                drown_darkness += (player->drown - (TICRATE*90)) / (TICRATE/2); // 0 .. 468
            extralight -= drown_darkness;
            extralight_cm -= drown_darkness;
            extralight_fog -= drown_darkness;
# if 0
            printf( "  EXTRALIGHT=%i\n", extralight );  // 0 .. -468
# endif
        }
#endif
   }

#ifdef PARANOIA
    if (!viewmobj)
         I_Error("R_Setupframe : viewmobj null (player %d)",player-players);
#endif
    viewx = viewmobj->x;
    viewy = viewmobj->y;

    viewsin = sine_ANG(viewangle);
    viewcos = cosine_ANG(viewangle);

    // Before R_RenderBSPNode or R_Draw_Masked or R_ProjectSprite
    // Can be camera.mo, viewplayer->mo, or fragglescript script_camera.mo
    viewer_sector = viewmobj->subsector->sector;
    viewer_modelsec = viewer_sector->modelsec;
    // [WDJ] modelsec used for more than water, do proper test
    // Use of modelsec is protected by model field, do not test for -1.
    viewer_has_model  = viewer_sector->model > SM_fluid;
    viewer_underwater = viewer_has_model && (viewz <= sectors[viewer_modelsec].floorheight);
    viewer_at_water = viewer_has_model && (viewz <= sectors[viewer_modelsec].floorheight + 2)
      && (viewz >= sectors[viewer_modelsec].floorheight - 2);
    viewer_overceiling = viewer_has_model && (viewz >= sectors[viewer_modelsec].ceilingheight);
    viewer_at_ceiling = viewer_has_model && (viewz <= sectors[viewer_modelsec].ceilingheight + 2)
      && (viewz >= sectors[viewer_modelsec].ceilingheight - 2);

    sscount = 0;

    fixedcolormap = NULL;  // default
    view_colormap = NULL;  // default
    view_extracolormap = NULL;
    view_fogmodel = NULL;
    view_fogfloor = NULL;

    // [WDJ] fog flag on colormap colors everything (but not very good fog)
    if( viewer_sector->extra_colormap && viewer_sector->extra_colormap->fog )
    {
        view_extracolormap = viewer_sector->extra_colormap;
    }

    // [WDJ] Because of interactions with extra colormaps, precedence must
    // be determined at the usage.  The Boom 282 colormap overrides all
    // normal colormap and extra colormap, but not fixedcolormap.
    // But it requires the light level calculations of an extra colormap.
    if( EN_boom_colormap )
    {
        // [WDJ] 4/11/2012 restore compatible Boom colormap handling
        if( viewer_sector->model == SM_Boom_deep_water )
        {
            sector_t * modsecp = & sectors[ viewer_modelsec ];
            // -1 or a valid colormap num
            int bcm_num = (viewz < modsecp->floorheight) ?
               modsecp->bottommap
             : (viewz > modsecp->ceilingheight)?
               modsecp->topmap
             : modsecp->midmap;
            // only enable view_colormap when overriding globally
            if(bcm_num >= 0 && bcm_num < num_extra_colormaps)
            {
               view_extracolormap = & extra_colormaps[bcm_num];
            }
        }
    }

    // Find a FF_FOG or fog extracolormap that viewer is within.
    // FF_FOG is a ffloor, not a model sector
    if( viewer_sector->ffloors )
    {
        ffloor_t * ffp;
        for( ffp=viewer_sector->ffloors; ffp; ffp=ffp->next )
        {
            if( ( ffp->flags & FF_FOG )
                && ( ffp->model_secnum >= 0 )
                && ( viewz < *(ffp->topheight) ) && ( viewz > *(ffp->bottomheight) )
                )
            {
                view_fogfloor = ffp;
                view_fogmodel = & sectors[ ffp->model_secnum ];
                // Fog already colors surrounding view by looking through
                // fog sheets, planes, and face sheet.
                // This is an extra effect enabled by colormap fog flag.
                if( view_fogmodel->extra_colormap && view_fogmodel->extra_colormap->fog )
                {
                    // fog colors all walls, floors, and things
                    view_extracolormap = view_fogmodel->extra_colormap;
                }
                else
                {
                    // viewer fake floor without colormap overrides viewer sector colormap
                    view_extracolormap = NULL;
                }
                break;
            }
        }
    }
    if( view_fogfloor )
    {
        // partial extralight taken by fog
        extralight = (extralight * (256 - view_fogfloor->alpha)) >> 8;
    }
    if( view_extracolormap )
        view_colormap = view_extracolormap->colormap;

    if (fixedcolormap_num)
    {
        // the fixedcolormap overrides sector colormaps
        fixedcolormap =
            & reg_colormaps[ LIGHTTABLE( fixedcolormap_num ) ];

        walllights = scalelightfixed;

        // refresh scalelights to fixedcolormap
        for (i=0 ; i<MAXLIGHTSCALE ; i++)
            scalelightfixed[i] = fixedcolormap;
    }

    //added:06-02-98:recalc necessary stuff for mouseaiming
    //               slopes are already calculated for the full
    //               possible view (which is 4*rdraw_viewheight).

    if ( rendermode == render_soft )
    {
        // clip it in the case we are looking a hardware 90° full aiming
        // (lmps, nework and use F12...)
        aimingangle = G_ClipAimingPitch(aimingangle);	// limit aimingangle

        // [WDJ] cleaned up
        dy = cv_splitscreen.EV ? rdraw_viewheight*2 : rdraw_viewheight ;
        dy = ( dy * AIMINGTODY(aimingangle) )/ BASEVIDHEIGHT ;

        yslope = &yslopetab[(3*rdraw_viewheight/2) - dy];
    }
    centery = (rdraw_viewheight/2) + dy;
    centeryfrac = centery<<FRACBITS;

    // rendergametic increases by 1 to 5 each pass
    fog_bltic = (rendergametic>>1) & 0x001F;  // 0..31, one cycle per fog_tic
    fog_tic = (rendergametic>>6) & 0x0FFF;  // tic per 1.78 seconds
    if( (fog_nexttic - rendergametic) > 0x00FF )
    {  // semi-random wave, no overflow, no underflow, slightly erratic
       fog_nexttic = rendergametic + 5;
       fog_wave1 += ((int)validcount&0x007F) - 0x003F;
       if(fog_wave1 > 0x4000) fog_wave1 = 0x2C;  // underflow
       else if(fog_wave1 > 0x3FE) fog_wave1 = 0x3D2;
       fog_wave2 += (((int)(fog_wave1+rendergametic)&0x03FF) - 0x01FF) >> 4;
       if(fog_wave2 > 0x4000) fog_wave2 = 0x2C;  // underflow
       else if(fog_wave2 > 0x3FE) fog_wave2 = 0x3D2;
    }

    framecount++;
    validcount++;
}

#ifdef HORIZONTALDRAW

#define CACHEW 32      // bytes in cache line
#define CACHELINES 32  // cache lines to use

void R_RotateBuffere (void)
{
    byte    bh,bw;
//    int     modulo;
    byte*   src,*srca,*srcr;
    byte*   dest,*destr;  // within screen buffer
    int     i,dl;


#define modulo 200  //= rdraw_viewheight;

    // [WDJ] yhlookup screen buffer is width x height, not padded
    // This is not directly compatible with any screen buffer
    srcr  = yhlookup[0];
    destr = ylookup[0] + columnofs[0];  // screen[0] buffer

    bh = rdraw_viewwidth / CACHELINES;
    while (bh--)
    {
        srca = srcr;
        dest = destr;

        bw = rdraw_viewheight;
        while (bw--)
        {
             src  = srca++;
             for (i=0;i<CACHELINES/4;i++)  // fill 32 cache lines
             {
                 *dest++ = *src;
                 *dest++ = *(src-modulo);
                 *dest++ = *(src-2*modulo);
                 *dest++ = *(src-3*modulo);
                 src -= 4*modulo;
             }
             dest = (dest - CACHELINES) + vid.ybytes;
        }
        srcr  -= (CACHELINES*rdraw_viewheight);
        destr += CACHELINES;
    }
}
#endif



// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?
// [WDJ] Our video buffer is not locked, only driver can lock direct
// buffer, and only for duration of transfer.


//extern consvar_t cv_grsoftwareview; //r_glide.c
extern void R_DrawFloorSplats (void);   //r_plane.c

// Draw player view, render_soft.
// In splitscreen the ylookup tables and view_window_y are adjusted per player.
// Global variables include:  ylookup[], view_window_y, rdraw_viewheight
//  pind : player index, [0]=main player, [1]=splitscreen player
void R_RenderPlayerView( byte pind, player_t* player )
{
    // rendermode == render_soft
    R_SetupFrame(pind, player);

    // Clear buffers.
    R_Clear_ClipSegs ();
    R_Clear_DrawSegs ();
    R_Clear_Planes (player);     //needs player for waterheight in occupied sector
    //R_Clear_Portals ();
    R_Clear_Sprites ();

#ifdef FLOORSPLATS
    R_Clear_VisibleFloorSplats ();
#endif

    // check for new console commands.
    NetUpdate ();

    // The head node is the last node output.

//profile stuff ---------------------------------------------------------
#ifdef TIMING
    mytotal=0;
    ProfZeroTimer();
#endif

    // [WDJ] Intercept degenerate case, so BSP node is never -1.
    R_RenderBSPNode( ( numnodes > 0 )? numnodes-1
                   : ( 0 | NF_SUBSECTOR ) );  // Degenerate, sector 0

#ifdef TIMING
    RDMSR(0x10,&mycount);
    mytotal += mycount;   //64bit add

    CONS_Printf("RenderBSPNode: 0x%d %d\n", *((int*)&mytotal+1),
                                             (int)mytotal );
#endif
//profile stuff ---------------------------------------------------------

// horizontal column draw optimisation test.. deceiving.
#ifdef HORIZONTALDRAW
//    R_RotateBuffere ();
    dc_source   = yhlookup[0];
    dc_colormap = ylookup[0] + columnofs[0];
    R_RotateBufferasm ();
#endif

    // Check for new console commands.
    NetUpdate ();

    //R_Draw_Portals ();
    R_Draw_Planes ();

    // Check for new console commands.
    NetUpdate ();

#ifdef FLOORSPLATS
    //faB(21jan): testing
    R_DrawVisibleFloorSplats ();
#endif

    // draw mid texture and sprite
    // SoM: And now 3D floors/sides!
    R_Draw_Masked ();

    if( view_fogfloor
        && ( view_fogfloor->flags & FF_FOGFACE  ) )
    {
        lightlev_t fog_extralight = (player->extralight * view_fogfloor->alpha) >> 7;
        R_RenderFog( view_fogfloor, viewer_sector, fog_extralight, 0 );  // random scale
        if( extralight )
           R_RenderFog( view_fogfloor, viewer_sector, fog_extralight, 32*FRACUNIT );
    }

    // If enabled, draw the weapon psprites on top of everything
    // but not on side views, nor on camera views.
    if( cv_psprites.EV && !viewangleoffset && !script_camera_on
        && camera.chase != player )
        R_DrawPlayerSprites ();

    // Check for new console commands.
    NetUpdate ();
    player->mo->flags &= ~MF_NOSECTOR; // don't show self (uninit) clientprediction code
}


// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

static
consvar_t * engine_ded_cvar_list[] =
{
    // Any cv_ with CV_SAVE needs to be registered, even if it is not used.
    // Otherwise there will be error messages when config is loaded.
   &cv_gravity,
   &cv_fog_effect,
   &cv_boom_colormap,
   &cv_invul_skymap,
   &cv_water_effect,
   &cv_sky_gen,

   &cv_screenshot_dir,
   &cv_screenshot_type,

   &cv_viewsize,
#ifdef FIT_RATIO
   &cv_viewfit,
#endif
   &cv_psprites,
   &cv_splitscreen,
//   &cv_fov,
   &cv_spritelim,

    // Default viewheight is changeable,
    // initialized to standard rdraw_viewheight
   &cv_viewheight,
   &cv_scalestatusbar,
   &cv_grtranslucenthud,
   &cv_corr_clip_width,
   NULL
};


static
consvar_t * engine_client_cvar_list[] =
{
   &cv_chasecam,
   &cv_cam_dist,
   &cv_cam_height,
   &cv_cam_speed,

    // unfinished, not for release
#ifdef PERSPCORRECT
   &cv_perspcorr,
#endif

    // unfinished, not for release
#ifdef TILTVIEW
   &cv_tiltview,
#endif
   NULL
};


void R_Register_EngineStuff (void)
{
    CV_RegisterVar_list( engine_ded_cvar_list );

//added by Hurdler
#ifdef HWRENDER
    HWR_Register_Gr1Commands ();
#endif
   
    // Enough for ded. server
    if( dedicated )
        return;

    CV_RegisterVar_list( engine_client_cvar_list );
}
