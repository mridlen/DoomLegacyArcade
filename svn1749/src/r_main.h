// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_main.h 1692 2024-11-27 06:37:03Z wesleyjohnson $
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
// $Log: r_main.h,v $
// Revision 1.7  2003/08/11 13:50:01  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.6  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.5  2001/01/25 22:15:44  bpereira
// added heretic support
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
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef R_MAIN_H
#define R_MAIN_H

#include "doomdef.h"
#include "doomtype.h"
#include "d_player.h"
#include "r_data.h"
#include "m_fixed.h"
#include "p_mobj.h"
#include "command.h"
  // consvar_t


//
// POV related.
//
extern fixed_t          viewcos;
extern fixed_t          viewsin;

extern int              viewwidth;
extern int              viewheight;
extern int              view_window_x;
extern int              view_window_y;

extern mobj_t*		viewmobj;

extern angle_t          clipangle, clipangle_x_2;

extern int              centerx;
extern int              centery;

extern int      centerypsp;

extern fixed_t          centerxfrac;
extern fixed_t          centeryfrac;
extern fixed_t          projection_x;
extern fixed_t          projection_y;    //added:02-02-98:aspect ratio test...

extern int              validcount;

extern int              linecount;
extern int              loopcount;

extern int      framecount;

// fog render
extern uint16_t	fog_col_length;
extern uint16_t fog_tic;    // 0..0xFFF
extern byte	fog_bltic;  // 0..32, blur/blend between tics
extern uint16_t fog_wave1;  // 0..0x3FF, random small scale changes
extern uint16_t fog_wave2;  // 0..0x3FF, random slower
extern byte     fog_index;  // 0.. column or texture height
extern byte     fog_index2; // fog_index-1 mod texture height
extern byte	fog_init;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Light is 0..255
#define LIGHT_UNIT     16

// Reduced light scale for light table lookup ( scalelight[][] )
// Now why not 32 levels here?
#define LIGHTLEVELS             16
#define LIGHTSEGSHIFT            4
// Light scaled by distance
#define MAXLIGHTSCALE           48
#define LIGHTSCALESHIFT         12
#define MAXLIGHTZ              128
#define LIGHTZSHIFT             20

extern lighttable_t*    scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t*    scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t*    zlight[LIGHTLEVELS][MAXLIGHTZ];

extern lighttable_t*    fixedcolormap;

extern lightlev_t  extralight;      // extralight seen by most draws
extern lightlev_t  extralight_fog;  // partial extralight used by FF_FOG
extern lightlev_t  extralight_cm;   // partial extralight used by colormap->fog

// [WDJ] viewer setup as used by R_RenderBSPNode, R_FakeFlat, R_ProjectSprite
extern sector_t * viewer_sector;
extern int      viewer_modelsec;
extern boolean  viewer_has_model;
extern boolean  viewer_underwater;  // only set when viewer_has_model
extern boolean  viewer_at_water;    // viewer straddles the water plane
extern boolean  viewer_overceiling; // only set when viewer_has_model
extern boolean  viewer_at_ceiling;  // viewer straddles the ceiling plane

extern ffloor_t *  view_fogfloor;  // viewer is in a FF_FOG floor
extern sector_t *  view_fogmodel;  // viewer is in a FF_FOG floor

// Boom colormap, and global viewer coloring
extern lighttable_t*    view_colormap;  // full lightlevel range colormaps
extern extracolormap_t *  view_extracolormap;

extern byte EN_boom_colormap;  // compatibility, user preference
void BoomColormap_detect( void );

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS            32


// Blocky/low detail mode.
//B remove this?
//  0 = high, 1 = low
extern  int  detailshift;


//
// Utility functions.
int R_PointOnSide ( fixed_t x, fixed_t y, node_t* node );

int R_PointOnSegSide ( fixed_t x, fixed_t y, seg_t* line );

angle_t R_PointToAngle ( fixed_t x, fixed_t y );

angle_t R_PointToAngle2 ( fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);

fixed_t R_PointToDist ( fixed_t x, fixed_t y );

//SoM: 3/27/2000
fixed_t R_PointToDist2 ( fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);

#ifdef MBF21
byte P_Check_FOV( mobj_t* t1, mobj_t* t2, angle_t fov );
#endif

fixed_t R_ScaleFromGlobalAngle (angle_t visangle);

subsector_t* R_PointInSubsector ( fixed_t x, fixed_t y );

subsector_t* R_IsPointInSubsector ( fixed_t x, fixed_t y );

void R_AddPointToBox ( int x, int y, fixed_t* box );



//
// REFRESH - the actual rendering functions.
//

extern consvar_t cv_psprites;
extern consvar_t cv_perspcorr;
extern consvar_t cv_tiltview;
extern consvar_t cv_splitscreen;
extern consvar_t cv_viewsize;
#ifdef FIT_RATIO
extern consvar_t cv_viewfit;
#endif
extern consvar_t cv_detaillevel;
extern consvar_t cv_scalestatusbar;
extern consvar_t cv_grtranslucenthud;
extern consvar_t cv_corr_clip_width;
extern consvar_t cv_boom_colormap;
extern consvar_t cv_invul_skymap;
extern consvar_t cv_water_effect;
extern consvar_t cv_fog_effect;

// Called by startup code.
void R_Init (void);


// just sets setsizeneeded true
extern boolean     setsizeneeded;
void   R_SetViewSize (void);

// do it (sometimes explicitly called)
void   R_ExecuteSetViewSize (void);

//  pind : player index, [0]=main player, [1]=splitscreen player
void  R_SetupFrame( byte pind, player_t* player );

// Called by G_Drawer.
//  pind : player index, [0]=main player, [1]=splitscreen player
void  R_RenderPlayerView( byte pind, player_t *player );

// add commands related to engine, at game startup
void   R_Register_EngineStuff (void);
#endif
