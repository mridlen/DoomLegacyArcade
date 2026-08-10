// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hw_main.c 1670 2024-03-03 04:39:25Z wesleyjohnson $
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
// $Log: hw_main.c,v $
// Revision 1.115  2004/05/16 19:11:44  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.114  2003/08/12 12:29:59  hurdler
// better translucent hud
//
// Revision 1.113  2003/07/13 13:16:15  hurdler
//
// Revision 1.112  2003/07/10 20:51:43  bock
// Small types fix
//
// Revision 1.111  2003/06/10 21:48:05  ssntails
// Variable flat size support (32x32 to 2048x2048)
//
// Revision 1.110  2002/05/19 19:37:27  hurdler
// adding some math functions to FraggleScript
//
// Revision 1.109  2002/01/12 02:26:47  stroggonmeth
//
// Revision 1.108  2002/01/05 01:08:51  hurdler
// fix a crashing bug with phobx in splitwall (is it ok ?)
//
// Revision 1.107  2001/12/31 13:47:46  hurdler
// Add setcorona FS command and prepare the code for beta 4
//
// Revision 1.106  2001/12/28 13:27:54  hurdler
// Manage translucent walls
//
// Revision 1.105  2001/12/27 22:50:26  hurdler
// fix a colormap bug, add scrolling floor/ceiling in hw mode
//
// Revision 1.104  2001/12/26 17:24:47  hurdler
// Update Linux version
//
// Revision 1.103  2001/12/26 15:56:11  hurdler
// Manage transparent wall a little better
//
// Revision 1.102  2001/12/15 18:41:36  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.101  2001/09/03 21:06:52  hurdler
//
// Revision 1.100  2001/08/27 19:59:35  hurdler
// Fix colormap in heretic + opengl, fixedcolormap and NEWCORONA
//
// Revision 1.99  2001/08/26 15:27:29  bpereira
// added fov for glide and fixed newcoronas code
//
// Revision 1.98  2001/08/19 20:41:04  hurdler
//
// Revision 1.97  2001/08/19 15:40:07  bpereira
// added Treansform (and lighting) to glide
//
// Revision 1.96  2001/08/14 00:36:26  hurdler
// Revision 1.95  2001/08/13 22:53:54  stroggonmeth
// Revision 1.94  2001/08/13 17:23:17  hurdler
//
// Revision 1.93  2001/08/13 17:01:40  hurdler
// Fix some coloured sector issue
//
// Revision 1.92  2001/08/13 16:27:45  hurdler
// Added translucency to linedef 300 and colormap to 3d-floors
//
// Revision 1.91  2001/08/12 22:08:40  hurdler
// Add alpha value for 3d water
//
// Revision 1.90  2001/08/12 19:05:56  hurdler
// fix a small bug with 3d water
//
// Revision 1.89  2001/08/12 17:57:16  hurdler
// Beter support of sector coloured lighting in hw mode
//
// Revision 1.88  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.87  2001/08/11 16:43:07  hurdler
// Add sector colormap in hw mode (3rd attempt)
//
// Revision 1.86  2001/08/11 15:36:56  hurdler
// Add sector colormap in hw mode (2nd attempt)
//
// Revision 1.85  2001/08/11 15:18:02  hurdler
// Add sector colormap in hw mode (first attempt)
//
// Revision 1.84  2001/08/11 00:51:59  hurdler
// Sort 3D water for better alpha blending
//
// Revision 1.83  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.82  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.81  2001/08/07 00:44:05  hurdler
// MD2 implementation is getting better but still need lots of work
//
// Revision 1.80  2001/08/06 23:55:02  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.79  2001/08/06 14:13:46  hurdler
// Crappy MD2 implementation (still need lots of work)
//
// Revision 1.78  2001/08/03 15:16:11  hurdler
// Fix small bugs (win2k timer + status bar)
//
// Revision 1.77  2001/06/16 08:07:55  bpereira
// Revision 1.76  2001/05/27 13:42:48  bpereira
// Revision 1.75  2001/05/16 21:21:15  bpereira
//
// Revision 1.74  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.73  2001/05/07 20:27:16  stroggonmeth
//
// Revision 1.72  2001/05/03 21:52:23  hurdler
// fix bis
//
// Revision 1.71  2001/05/03 20:05:56  hurdler
// small opengl fix
//
// Revision 1.70  2001/05/01 20:38:34  hurdler
// some fix/hack for the beta release
//
// Revision 1.69  2001/04/30 21:00:10  stroggonmeth
// Another HWR fix
//
// Revision 1.68  2001/04/30 17:19:25  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.67  2001/04/27 13:32:14  bpereira
//
// Revision 1.66  2001/04/18 23:22:00  hurdler
// Until SoM fix it more properly than me ;)
//
// Revision 1.65  2001/04/18 22:53:55  hurdler
// Until SoM fix it more properly than me ;)
//
// Revision 1.64  2001/04/16 21:40:06  hurdler
// Fix misaligned midtexture problem (to verify!)
//
// Revision 1.63  2001/04/16 15:16:45  hurdler
// Revision 1.62  2001/04/09 23:26:05  hurdler
// Revision 1.61  2001/04/09 20:24:28  metzgermeister
// Revision 1.60  2001/04/09 14:18:21  hurdler
// Revision 1.59  2001/04/08 10:15:54  bpereira
//
// Revision 1.58  2001/04/03 23:15:39  hurdler
// FIXME: this code adds basic 3D-floors support in hw mode
//
// Revision 1.57  2001/03/25 18:11:24  metzgermeister
//   * SDL sound bug with swapped stereo channels fixed
//   * separate hw_trick.c now for HW_correctSWTrick(.)
//
// Revision 1.56  2001/03/20 21:26:21  hurdler
// use PI instead of M_PI as it  is defined in hw_defs.h (M_PI undefined under win32 by default)
//
// Revision 1.55  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.54  2001/03/19 18:15:59  hurdler
// fix a fog problem
//
// Revision 1.53  2001/03/13 22:14:21  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.52  2001/02/24 13:35:22  bpereira
// Revision 1.51  2001/02/10 12:27:14  bpereira
//
// Revision 1.50  2001/01/31 17:15:09  hurdler
// Add cv_scalestatusbar in hardware mode
//
// Revision 1.49  2001/01/25 18:56:27  bpereira
// Revision 1.48  2000/11/18 15:51:25  bpereira
//
// Revision 1.47  2000/11/12 21:04:28  hurdler
// Fix a bug with validcount and boom code (dyn.light looked ugly with boom water)
//
// Revision 1.46  2000/11/11 13:59:47  bpereira
// Revision 1.45  2000/11/02 22:14:02  bpereira
// Revision 1.44  2000/11/02 21:54:26  bpereira
// Revision 1.43  2000/11/02 19:49:39  bpereira
//
// Revision 1.42  2000/10/22 14:16:41  hurdler
// Prepare code for TANDL
//
// Revision 1.41  2000/10/21 08:43:32  bpereira
// Revision 1.40  2000/10/04 16:21:57  hurdler
// Revision 1.39  2000/10/02 18:25:46  bpereira
// Revision 1.38  2000/10/01 15:18:38  hurdler
// Revision 1.37  2000/10/01 10:18:23  bpereira
//
// Revision 1.36  2000/10/01 09:10:19  hurdler
// Put the md2 code in #ifdef TANDL
//
// Revision 1.35  2000/10/01 01:22:35  hurdler
// remove polysky, enable PF_Environment for sprites
//
// Revision 1.34  2000/09/28 20:57:20  bpereira
// Revision 1.33  2000/09/21 16:45:11  bpereira
// Revision 1.32  2000/08/31 14:30:57  bpereira
//
// Revision 1.31  2000/08/21 21:13:58  metzgermeister
// crash on polys>256verts fixed
//
// Revision 1.30  2000/08/11 19:11:57  metzgermeister
// Revision 1.29  2000/08/03 17:57:42  bpereira
// Revision 1.28  2000/07/01 09:23:50  bpereira
//
// Revision 1.27  2000/06/08 19:40:34  hurdler
// my changes before splitting (can be reverted in development branch)
//
// Revision 1.26  2000/05/30 18:03:22  kegetys
// Wall, floor and ceiling lighting is now done by changing only the RGB, not the alpha
//
// Revision 1.25  2000/05/10 17:45:35  kegetys
// Revision 1.24  2000/05/05 18:00:05  bpereira
// Revision 1.23  2000/04/30 10:30:10  bpereira
//
// Revision 1.22  2000/04/27 23:41:16  hurdler
// better splitscreen support in OpenGL mode
//
// Revision 1.21  2000/04/27 17:48:47  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.20  2000/04/24 20:24:38  bpereira
//
// Revision 1.19  2000/04/24 15:46:34  hurdler
// Support colormap for text
//
// Revision 1.18  2000/04/23 16:19:52  bpereira
//
// Revision 1.17  2000/04/23 12:50:32  hurdler
// support filter mode in OpenGL
//
// Revision 1.16  2000/04/22 21:08:23  hurdler
//
// Revision 1.15  2000/04/22 16:09:14  hurdler
// support skin color in hardware mode
//
// Revision 1.14  2000/04/18 16:07:16  hurdler
// better support of decals
//
// Revision 1.13  2000/04/18 12:52:21  hurdler
//
// Revision 1.11  2000/04/14 16:34:26  hurdler
// some nice changes for coronas
//
// Revision 1.10  2000/04/12 16:03:51  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.9  2000/04/09 01:59:06  hurdler
//
// Revision 1.8  2000/04/08 11:28:46  hurdler
// added boom water support
//
// Revision 1.7  2000/03/29 19:39:49  bpereira
//
// Revision 1.6  2000/03/08 17:02:05  hurdler
// fix the joiningame problem under Linux
//
// Revision 1.5  2000/03/07 14:22:48  hurdler
//
// Revision 1.4  2000/03/06 16:52:06  hurdler
// hack for OpenGL / Open Entry problem
//
// Revision 1.3  2000/03/05 17:10:56  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      hardware renderer, using the standard HardwareRender driver DLL for Doom Legacy
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "doomincl.h"

#include "hw_glob.h"
#include "hw_light.h"

#include "doomstat.h"
#include "i_video.h"
  // added by Hurdler for rendermode == render_glide
#include "v_video.h"
#include "p_local.h"
#include "p_setup.h"
#include "r_local.h"
#include "d_clisrv.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"
#include "t_func.h"
#include "st_stuff.h"


// #define DEBUG_FIT_RATIO  1

//#define SPRITE_NEAR_CLIP_DIST     4.0f
#define SPRITE_NEAR_CLIP_DIST     1.01f
#define R_FAKEFLOORS


// BP: test of draw sky by polygon like in software with visplane ...
// [WDJ] Looks bad because R_SKY2 is not a plane, it is distant wall texture.
//#define POLYSKY

// BP: test change fov when looking up/down but bsp projection messup :(
//#define NO_MLOOK_EXTENDS_FOV

#ifdef CORONA_CHOICE
// mirror corona choice, with auto modifications
byte  corona_draw_choice;
#endif

//#define DEBUG_TRIG
#ifdef DEBUG_TRIG
static uint32_t  trigger_line = 0xFFFFFFFF;
static uint32_t  trigger_sector = 0xFFFFFFFF;
static uint32_t  trigger_draw_subsector = 0xFFFFFFFF;
static uint32_t  trigger_draw_count = 0;
#endif

// Sky upper and lower halfs.
typedef enum { DSB_none, DSB_upper, DSB_lower, DSB_all }  DSB_e;


// ==========================================================================
// the hardware driver object
// ==========================================================================
struct hwdriver_s hwdriver;

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

static void HWR_AddSprites(sector_t* sec, lightlev_t lightlevel);
static void HWR_ProjectSprite(mobj_t * thing);
static void HWR_Add3DWater(int picnum, poly_subsector_t * xsub, fixed_t fixedheight, lightlev_t lightlevel, int alpha);
static void HWR_Render3DWater();
static void HWR_RenderSorted( void );


// ==========================================================================
//                                          3D ENGINE COMMANDS & CONSOLE VARS
// ==========================================================================

CV_PossibleValue_t grmlook_extends_fov_cons_t[] = { {0, "Off"}, {1, "On"}, {2, "Full"}, {0, NULL} };
CV_PossibleValue_t grgamma_cons_t[] = { {1, "MIN"}, {255, "MAX"}, {0, NULL} };
CV_PossibleValue_t grfov_cons_t[] = { {0, "MIN"}, {179, "MAX"}, {0, NULL} };
CV_PossibleValue_t grfiltermode_cons_t[] = {
{HWD_SET_TEXTUREFILTER_POINTSAMPLED, "Nearest"},
{HWD_SET_TEXTUREFILTER_BILINEAR, "Bilinear"},
{HWD_SET_TEXTUREFILTER_TRILINEAR, "Trilinear"},
{HWD_SET_TEXTUREFILTER_MIXED1, "Linear_Nearest"},
{HWD_SET_TEXTUREFILTER_MIXED2, "Nearest_Linear"},
{0, NULL}
};

// BP: change directly the palette to see the change
void CV_Gammaxxx_ONChange(void)
{
    V_SetPalette(0);
}

void CV_filtermode_ONChange(void);
void CV_FogDensity_ONChange(void);
//static void CV_grFogColor_OnChange (void);
static void CV_grFov_OnChange(void);
//static void CV_grMonsterDL_OnChange (void);
static void CV_grPolygonSmooth_OnChange(void);

consvar_t cv_grrounddown = { "gr_rounddown", "Off", 0, CV_OnOff };
consvar_t cv_grmlook_extends_fov = { "gr_mlook", "Full", CV_SAVE, grmlook_extends_fov_cons_t };
consvar_t cv_grfov = { "gr_fov", "90", CV_SAVE | CV_CALL, grfov_cons_t, CV_grFov_OnChange };
//consvar_t cv_grsky = { "gr_sky", "On", 0, CV_OnOff };
consvar_t cv_grfog = { "gr_fog", "On", CV_SAVE, CV_OnOff };
consvar_t cv_grfogcolor = { "gr_fogcolor", "000000", CV_SAVE, NULL };
consvar_t cv_grfogdensity = { "gr_fogdensity", "100", CV_SAVE | CV_CALL | CV_NOINIT, CV_Unsigned, CV_FogDensity_ONChange };
consvar_t cv_grgammared = { "gr_gammared", "127", CV_SAVE | CV_CALL, grgamma_cons_t, CV_Gammaxxx_ONChange };
consvar_t cv_grgammagreen = { "gr_gammagreen", "127", CV_SAVE | CV_CALL, grgamma_cons_t, CV_Gammaxxx_ONChange };
consvar_t cv_grgammablue = { "gr_gammablue", "127", CV_SAVE | CV_CALL, grgamma_cons_t, CV_Gammaxxx_ONChange };
consvar_t cv_grfiltermode = { "gr_filtermode", "Bilinear", CV_SAVE | CV_CALL, grfiltermode_cons_t, CV_filtermode_ONChange };
consvar_t cv_grzbuffer = { "gr_zbuffer", "On", 0, CV_OnOff };
consvar_t cv_grcorrecttricks = { "gr_correcttricks", "On", 0, CV_OnOff };

consvar_t cv_grsolvetjoin = { "gr_solvetjoin", "On", 0, CV_OnOff };
consvar_t cv_grpolytile = { "gr_polytile", "On", 0, CV_OnOff };

CV_PossibleValue_t grpolyshape_cons_t[] = {
  {0, "Subsector"},
  {1, "Fat"},
  {2, "Trim"},
  {3, "NotConvex"},
  {0, NULL}
};
consvar_t cv_grpolyshape = { "gr_polygon_shape", "Trim", CV_SAVE, grpolyshape_cons_t };

// console variables in development
consvar_t cv_grpolygonsmooth = { "gr_polygonsmooth", "Off", CV_CALL, CV_OnOff, CV_grPolygonSmooth_OnChange };
consvar_t cv_grmd2 = { "gr_md2", "Off", 0, CV_OnOff };

#ifdef TRANSWALL_CHOICE
// three choices for debugging
CV_PossibleValue_t grtranswall_cons_t[] = { {2, "Sorted"}, {3, "SortX3"}, {0, NULL} };
consvar_t cv_grtranswall = { "gr_transwall", "Sorted", 0, grtranswall_cons_t };
#endif

// faB : needs fix : walls are incorrectly clipped one column less
consvar_t cv_grclipwalls = { "gr_clipwalls", "Off", 0, CV_OnOff };

//development variables for diverse uses
consvar_t cv_gralpha = { "gr_alpha", "160", 0, CV_Unsigned };
consvar_t cv_grbeta = { "gr_beta", "0", 0, CV_Unsigned };
consvar_t cv_grgamma = { "gr_gamma", "0", 0, CV_Unsigned };

void CV_FogDensity_ONChange(void)
{
    if( HWD.pfnSetSpecialState )
        HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
}

void CV_filtermode_ONChange(void)
{
    if( HWD.pfnSetSpecialState )
        HWD.pfnSetSpecialState(HWD_SET_TEXTUREFILTERMODE, cv_grfiltermode.value);
}

// ==========================================================================
//                                                               VIEW GLOBALS
// ==========================================================================
// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW      ANG90

angle_t gr_clipangle, gr_clipangle_x_2;

// The gr_viewangle_to_x[viewangle + FINE_ANG90] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int gr_viewangle_to_x[FINE_ANG180];

// The gr_x_to_viewangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t gr_x_to_viewangle[MAXVIDWIDTH + 1];

// ==========================================================================
//                                                                    GLOBALS
// ==========================================================================

// base values set at SetViewSize
float gr_basecentery;
float gr_baseviewwindowy;
float gr_basewindowcentery;

float gr_viewwidth;             // viewport clipping boundaries (screen coords)
float gr_viewheight;
float gr_centerx;
float gr_centery;
float gr_viewwindowx;           // top left corner of view window
float gr_viewwindowy;
float gr_windowcenterx;         // center of view window, for projection
float gr_windowcentery;

static float gr_pspritescale_x;
static float gr_pspritescale_y;

seg_t * gr_curline;
side_t * gr_sidedef;
line_t * gr_linedef;
sector_t * gr_frontsector;
sector_t * gr_backsector;
//RGBA_float_t  gr_cursectorlight;      // colour & intensity of current sector's lighting

// --------------------------------------------------------------------------
//                                              STUFF FOR THE PROJECTION CODE
// --------------------------------------------------------------------------

FTransform_t atransform;
// duplicates of the main code, set after R_SetupFrame() passed them into sharedstruct,
// copied here for local use
static angle_t dup_viewangle;

static float gr_viewx;
static float gr_viewy;
static float gr_viewz;
static float gr_viewsin;
static float gr_viewcos;

//04/01/00: Maybe not necessary with the new T&L code (need to be checked!)
static float gr_viewludsin;     //look up down kik test
static float gr_viewludcos;

static float gr_fovlud;

// ==========================================================================
//                                    Transforms
// ==========================================================================

// The only terms needed, the other terms are 0.
// Don't try to make this a matrix, this is much easier to understand and maintain.
static float world_trans_x_to_x, world_trans_y_to_x,
  world_trans_x_to_y, world_trans_y_to_y, world_trans_z_to_y,
  world_trans_x_to_z, world_trans_y_to_z, world_trans_z_to_z;
static float sprite_trans_x_to_x, sprite_trans_y_to_y, sprite_trans_z_to_y,
  sprite_trans_z_to_z, sprite_trans_y_to_z;

void HWR_set_view_transform( void )
{
    // Combined transforms for position, direction, look up/down, and scaling
    // translation is separate and done first
    // In order:
    //   rotation around vertical y axis
    //   look up/down
    //   scale y before frustum so that frustum can be scaled to screen height
#ifdef FIT_RATIO    
    float fov_scale_x = gr_fovlud * atransform.scalex; // (float)vid.fit_width / (float)vid.width;
    float fov_scale_y = gr_fovlud * atransform.scaley; // ORIGINAL_ASPECT * ((float) vid.fit_height) / (float) vid.height;
    world_trans_x_to_x = gr_viewsin * fov_scale_x;
    world_trans_y_to_x = -gr_viewcos * fov_scale_x;
    world_trans_x_to_y = gr_viewcos * gr_viewludcos * fov_scale_y;
    world_trans_y_to_y = gr_viewsin * gr_viewludcos * fov_scale_y;
    world_trans_z_to_y = gr_viewludsin * fov_scale_y;
    world_trans_x_to_z = gr_viewcos * gr_viewludsin;
    world_trans_y_to_z = gr_viewsin * gr_viewludsin;
    world_trans_z_to_z = -gr_viewludcos;

    // look up/down and scaling
    sprite_trans_x_to_x = fov_scale_x;
    sprite_trans_y_to_y = gr_viewludsin * fov_scale_y;
    sprite_trans_z_to_y = gr_viewludcos * fov_scale_y;
    sprite_trans_z_to_z = gr_viewludsin;
    sprite_trans_y_to_z = -gr_viewludcos;
#else   
    world_trans_x_to_x = gr_viewsin * gr_fovlud;
    world_trans_y_to_x = -gr_viewcos * gr_fovlud;
    world_trans_x_to_y = gr_viewcos * gr_viewludcos * ORIGINAL_ASPECT * gr_fovlud;
    world_trans_y_to_y = gr_viewsin * gr_viewludcos * ORIGINAL_ASPECT * gr_fovlud;
    world_trans_z_to_y = gr_viewludsin * ORIGINAL_ASPECT * gr_fovlud;
    world_trans_x_to_z = gr_viewcos * gr_viewludsin;
    world_trans_y_to_z = gr_viewsin * gr_viewludsin;
    world_trans_z_to_z = -gr_viewludcos;

    // look up/down and scaling
    sprite_trans_x_to_x = gr_fovlud;
    sprite_trans_y_to_y = gr_viewludsin * ORIGINAL_ASPECT * gr_fovlud;
    sprite_trans_z_to_y = gr_viewludcos * ORIGINAL_ASPECT * gr_fovlud;
    sprite_trans_z_to_z = gr_viewludsin;
    sprite_trans_y_to_z = -gr_viewludcos;
#endif   
}

#if 0
// unused because of confused wall drawing
// wx,wy,wz in world coord, to screen vxtx3d_t
void HWR_transform_world_FOut(float wx, float wy, float wz, vxtx3d_t * fovp)
{
    // Combined transforms for position, direction, look up/down, and scaling.
    // translation
    // x world, to x screen
    // vert z world, to vert y screen
    // y world, to screen depth
    float tr_x = wx - gr_viewx;
    float tr_y = wy - gr_viewy;
    float tr_z = wz - gr_viewz;
    fovp->x = (tr_x * world_trans_x_to_x)
       + (tr_y * world_trans_y_to_x);
    fovp->y = (tr_x * world_trans_x_to_y )
       + (tr_y * world_trans_y_to_y )
       + (tr_z * world_trans_z_to_y );
    fovp->z = (tr_x * world_trans_x_to_z )
       + (tr_y * world_trans_y_to_z )
       + (tr_z * world_trans_z_to_z );
}
#endif

#if 0
// unused
void HWR_transform_sprite_FOut(float cx, float cy, float cz, vxtx3d_t * fovp)
{
    // Combined transforms for look up/down, and scaling
   fovp->y = (cy * sprite_trans_y_to_y) + (cz * sprite_trans_z_to_y);
   fovp->z = (cy * sprite_trans_y_to_z) + (cz * sprite_trans_z_to_z);
   fovp->x = (cx * sprite_trans_x_to_x);
}
#endif


// ==========================================================================
//                                    LIGHT stuffs
// ==========================================================================

static byte lightlevel_to_lum_table[256];

// added to Doom's sector lightlevel to make things a bit brighter (sprites/walls/planes)
byte LightLevelToLum(lightlev_t l)
{
    if (fixedcolormap)
        return 255;

    // [WDJ] Map light is int16, for unknown reasons.
    // Some maps are using 256, or higher.
    if( l > 255 )  l = 255;
    if( l < 0 )  l = 0;

    l = lightlevel_to_lum_table[l];
    l += extralight;	// from guns
    if (l > 255)
        l = 255;
    return l;
}

// Need to select extra
byte LightLevelToLum_extra(lightlev_t l, lightlev_t extra)
{
    if (fixedcolormap)
        return 255;

    // [WDJ] Map light is int16, for unknown reasons.
    // Some maps are using 256, or higher.
    if( l > 255 )  l = 255;
    if( l < 0 )  l = 0;

    l = lightlevel_to_lum_table[l];
    l += extra;	// from guns, etc..
    if (l > 255)
        l = 255;
    return l;
}


static void Init_LumLut()
{
    int i, k;
    for (i = 0; i < 256; i++)
    {
        // this polygon is the solution of equ : f(0)=0, f(1)=1 f(.5)=.5, f'(0)=0, f'(1)=0), f'(.5)=K
//#define K   2
// [WDJ] Match software renderer brightness,
// with single rgba use 1.25, with rgba[] use 1.9 to 2.1
#define K   1.95f
#define A  (-24+16*K)
#define B  ( 60-40*K)
#define C  (32*K-50)
#define D  (-8*K+15)
        float x = (float) i / 255;
        float xx, xxx;
        xx = x * x;
        xxx = x * xx;
        k = 255 * (A * xx * xxx + B * xx * xx + C * xxx + D * xx);

        lightlevel_to_lum_table[i] = min(255, k);
    }
}


// [WDJ] Handle the extra_colormap, transforming into GL surface info.
// Smaller when inline, but easier to alter in this function.
// This function is only going to get more complicated.
static
void Extracolormap_to_Surf( /*IN*/ extracolormap_t * extracmap, byte lightlum,
                            /*OUT*/ FSurfaceInfo_t * surfp )
{
    RGBA_t temp;
    unsigned int light = lightlum; // prevent sign extension
   
    if( view_extracolormap )
    {
        // Boom global colormap override
        extracmap = view_extracolormap;
    }

    // [WDJ] the rgba color can change according to light level
    // This accomodates the fade in Create_Colormap, and Colormap_Analyze.
    temp.rgba = extracmap->rgba[ light >> LIGHT_TO_RGBA_SHIFT ].rgba;
    // prevent sign extension
    // alpha -> 1..256, so can >> 8
    unsigned int alpha = ((unsigned int)temp.s.alpha) + 1;
    surfp->FlatColor.s.red = ((256 - alpha) * light + alpha * temp.s.red) >> 8;
    surfp->FlatColor.s.blue = ((256 - alpha) * light + alpha * temp.s.blue) >> 8;
    surfp->FlatColor.s.green = ((256 - alpha) * light + alpha * temp.s.green) >> 8;
    surfp->FlatColor.s.alpha = 0xff;
}

// ==========================================================================
//                                   FLOOR/CEILING GENERATION FROM SUBSECTORS
// ==========================================================================


//what is the maximum number of verts around a convex floor/ceiling polygon?
// Note: gothic2 map02 has a 304 vertex poly!!!!
#define MAXPLANEVERTICES   512
static vxtx3d_t planeVerts[MAXPLANEVERTICES];

// Indexed by flat size_index.
static float  flat_flatsize_tab[ 8 ] =
{
    0.0, // 0
    32.0f, // 32x32 flat
    64.0f, // 64x64 flat
    128.0f, // 128x128 flat
    256.0f, // 256x256 flat
    512.0f, // 512x512 flat
    1024.0f, // 1024x1024 flat
    2048.0f, // 2048x2048 flat
};


// Indexed by flat size_index.
static uint32_t  flat_flatmask_tab[ 8 ] =
{
   ~ 0, // 0
   ~( 32 - 1 ), // 32x32 flat
   ~( 64 - 1 ), // 64x64 flat
   ~( 128 - 1 ), // 128x128 flat
   ~( 256 - 1 ), // 256x256 flat
   ~( 512 - 1 ), // 512x512 flat
   ~( 1024 - 1 ), // 1024x1024 flat
   ~( 2048 - 1 ), // 2048x2048 flat
};



// -----------------+
// HWR_RenderPlane  : Render a floor or ceiling convex polygon
// -----------------+
// Preceded by HWR_GetFlat( levelflats[picnum].lumpnum );
// Parameter static global
//   planeVerts : polygon verts
// Parameters:
//   lightlevel : SoM: 3D floors light level
//   picnum : index to levelflats
// Called from HWR_Subsector, HWR_Render3DWater
static
void HWR_RenderPlane(poly_subsector_t * xsub, fixed_t fixedheight,
                     FBITFIELD PolyFlags, extracolormap_t* planecolormap,
                     lightlev_t lightlevel, int picnum)
{
    polyvertex_t * pv;
    vxtx3d_t * v3d;
    int nrPlaneVerts;           //verts original define of convex flat polygon
    float height;               //constant y for all points on the convex flat polygon
    float flatxref, flatyref;
    double flatsize, flatscrollinc;
    int32_t flatmask;  // cannot be uint as the result will be unsigned
    int i;
    byte lightlum;  // 0..255

    FSurfaceInfo_t Surf;

    // no convex poly were generated for this subsector
    if (!xsub->planepoly)
        return;

    height = FIXED_TO_FLOAT( fixedheight );

    pv = xsub->planepoly->pts;
    nrPlaneVerts = xsub->planepoly->numpts;

    if (nrPlaneVerts < 3)       //not even a triangle ?
        return;

    if (nrPlaneVerts > MAXPLANEVERTICES)
    {
        // Too many verts for planeVerts
        I_SoftError("HWR_RenderPlane: polygon size %d exceeds max value of %d vertices\n", nrPlaneVerts, MAXPLANEVERTICES);
        nrPlaneVerts = MAXPLANEVERTICES;  // cut off polygon side
    }

    int sizeindex = levelflats[picnum].size_index;
    flatsize = flat_flatsize_tab[sizeindex];
    flatmask = flat_flatmask_tab[sizeindex];

    //reference point for flat texture coord for each vertex around the polygon
    flatxref = (((int32_t) pv->x) & flatmask) / flatsize;
    flatyref = (((int32_t) pv->y) & flatmask) / flatsize;

    // transform
    flatscrollinc = (FIXED_TO_FLOAT_MULT / flatsize);
    v3d = planeVerts;  // static global
    for (i = 0; i < nrPlaneVerts; i++, v3d++, pv++)
    {
        v3d->y = height;
        v3d->x = pv->x;
        v3d->z = pv->y;
        v3d->sow = (pv->x / flatsize) - flatxref;
        v3d->tow = flatyref - (pv->y / flatsize);
        // Hurdler: add scrolling texture on floor/ceiling
        if (gr_frontsector)
        {
            if (fixedheight < viewz)
            {  // it's a floor
                v3d->sow += gr_frontsector->floor_xoffs * flatscrollinc;
                v3d->tow += gr_frontsector->floor_yoffs * flatscrollinc;
            }
            else
            {
                v3d->sow += gr_frontsector->ceiling_xoffs * flatscrollinc;
                v3d->tow += gr_frontsector->ceiling_yoffs * flatscrollinc;
            }
        }
    }

    // only useful for flat coloured triangles
    //Surf.FlatColor = 0xff804020;

    //  use different light tables
    //  for horizontal / vertical / diagonal
    //  note: try to get the same visual feel as the original
    lightlum = LightLevelToLum(lightlevel);        // SoM: Don't take from the frontsector
    Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = lightlum;

    //Hurdler: colormap test
    if ( !fixedcolormap )
    {
        if ( view_extracolormap )
        {
            Extracolormap_to_Surf( /*IN*/ view_extracolormap, lightlum,
                                   /*OUT*/ & Surf );
        }
        else if ( gr_frontsector )
        {
            sector_t *sector = gr_frontsector;

            if (gr_frontsector->ffloors)
            {
                ffloor_t * caster =
                  R_GetPlaneLight(gr_frontsector, fixedheight)->caster;
                sector = caster ? &sectors[caster->model_secnum] : gr_frontsector;
            }
            if (sector && sector->extra_colormap && planecolormap == NULL)
                planecolormap = sector->extra_colormap;
            if (planecolormap)
            {
                Extracolormap_to_Surf( /*IN*/ planecolormap, lightlum,
                                       /*OUT*/ & Surf );
            }
        }
    }

    if ((PolyFlags & PF_Translucent) && !fixedcolormap)
    {
        // get alpha from HWR_Render3DWater
        // PolyFlags = PF_Translucent | (planeinfo[i].alpha << 24);
        Surf.FlatColor.s.alpha = PolyFlags >> 24;
        HWD.pfnDrawPolygon(&Surf, planeVerts, nrPlaneVerts,
                           PF_Translucent | PF_Modulated | PF_Occlude | PF_Clip);
    }
    else
    {
        Surf.FlatColor.s.alpha = 0xff;
        HWD.pfnDrawPolygon(&Surf, planeVerts, nrPlaneVerts,
                           PolyFlags | PF_Masked | PF_Modulated | PF_Clip);
    }

    //12/11/99: Hurdler: add here code for dynamic lighting on planes
    HWR_PlaneLighting(planeVerts, nrPlaneVerts);

    //experimental code: shadow of the player: should be done only on floor
    //HWR_DynamicShadowing(planeVerts, nrPlaneVerts, plVerts, plyr);
}

#ifdef POLYSKY
// [WDJ] When I got this working, the low sky plane cut off the tops of trees.
// It appears:
// The software render draws sky planes, but without object clipping.
// In sky sectors it draws upper textures as sky.

// this don't draw anything it only update the z-buffer so there isn't problem with
// wall/things upper that sky (map12)
// Parameter static global
//   skypoly : the polygon that sky is seen through
//   polyheight : height of polygon
//   planeVerts : polygon verts
// Called from HWR_Subsector
static
void HWR_RenderSkyPlane(poly_subsector_t * skypoly, fixed_t polyheight)
//                              FBITFIELD         PolyFlags )
{
    polyvertex_t * pv;
    float height;   // constant y for all points on the convex flat polygon
    vxtx3d_t * v3d;
    int nrPlaneVerts;  // verts original define of convex flat polygon
    int i;
    angle_t angle;
    float f, skysow03, skysow12, skytow01, skytow23;
    float xsize, ysize;
    float dx, dy, dz;

    // no convex poly were generated for this subsector
    if (!skypoly->planepoly)
        return;

    height = FIXED_TO_FLOAT( polyheight );
    dz = fabs( height - gr_viewz );

    pv = skypoly->planepoly->pts;
    nrPlaneVerts = skypoly->planepoly->numpts;

    if (nrPlaneVerts < 3)       //not even a triangle ?
        return;
   
    HWR_GetTexture (skytexture, 0);
    xsize = 128;
    ysize = 256;

    angle = ((dup_viewangle + gr_x_to_viewangle[0]) % ANG90);

    // left
    skysow03 = 1.0f + ((float) angle) / (ANG90 - 1);
    // right
    skysow12 = ((float) angle) / (ANG90 - 1);

    f = 40 + 200 * FIXED_TO_FLOAT(
        finetangent[(FINE_ANG90 - ((int) aimingangle >> (ANGLETOFINESHIFT + 1))) & FINEMASK] );
        // finetangent_ANG( -(aimingangle/2) )
#if 1
    if (f < 0)
        f = 0;
    if (f > 240 - 127)
        f = 240 - 127;
#endif
    // up
    skytow23 = f / 127.0f;
    // down
    skytow01 = (f + 127) / 127.0f;   //suppose 256x128 sky...
    
    // Sky x,y,z are all -4.0 to 4.0, but here scaled much larger.
    // transform
    v3d = planeVerts;  // static global
    for (i = 0; i < nrPlaneVerts; i++, v3d++, pv++)
    {
        v3d->y = height;
        v3d->x = pv->x;
        v3d->z = pv->y;
//        v3d->sow = (pv->x / xsize);
//        v3d->tow = - (pv->y / xsize);
     // Still not right.
     // Sky turns with viewer, so clouds are horizontal from all sides.
     // This keeps cloud orientation fixed with respect to polygon,
     // which is more correct for a floor sky hole, but does not match the
     // vanilla doom.
        dx = (pv->x - gr_viewx);
        dy = (pv->y - gr_viewy);
        v3d->sow = skysow03 + ((skysow12 - skysow03) / 2.0f * (dx/dz + 1.0));
        v3d->tow = skytow01 + ((skytow23 - skytow01) / 2.0f * (dy/dz + 1.0));
    }

    HWD.pfnDrawPolygon(NULL, planeVerts, nrPlaneVerts, 0);
#ifdef BLEND_FIELD
//    HWD.pfnDrawPolygon(NULL, planeVerts, nrPlaneVerts,
//                       PF_InvisibleB | PF_Occlude | PF_Clip);
#else
//    HWD.pfnDrawPolygon(NULL, planeVerts, nrPlaneVerts,
//                       PF_InvisibleColor | PF_Occlude | PF_Masked | PF_Clip);
#endif
}
#endif //polysky


/*
   vxtx order is :
          3--2
          | /|
          |/ |
          0--1
*/
#ifdef WALLSPLATS
// Called from HWR_ProjectWall, HWR_RenderWall
void HWR_DrawSegsSplats( FSurfaceInfo_t * pSurfin )
{
    wallsplat_t * splat;
    MipPatch_t * gpatch;
    int blendmode = PF_Translucent;
    FSurfaceInfo_t pSurf2;
    fixed_t tmy;
    // seg bbox
    fixed_t segbbox[4];
    vxtx3d_t vxtx[4];

    M_ClearBox(segbbox);
    // make box from fixed_t vertex
#if 1
    M_AddToBox(segbbox, gr_curline->v1->x, gr_curline->v1->y );
    M_AddToBox(segbbox, gr_curline->v2->x, gr_curline->v2->y );
#else
    // convert polyvertex_t back to fixed_t
    M_AddToBox(segbbox, gr_curline->pv1->x / FIXED_TO_FLOAT_MULT,
               gr_curline->pv1->y / FIXED_TO_FLOAT_MULT);
    M_AddToBox(segbbox, gr_curline->pv2->x / FIXED_TO_FLOAT_MULT,
               gr_curline->pv2->y / FIXED_TO_FLOAT_MULT);
#endif

    // splat are drawn by line but this func is called for eatch segs of a line
    /*
       BP: DONT WORK BECAUSE Z-buffer !!!!
       FIXME : the splat must be stored by segs !
       if( gr_curline->linedef->splatdrawn == validcount )
       return;
       gr_curline->linedef->splatdrawn = validcount;
     */

    splat = (wallsplat_t *) gr_curline->linedef->splats;
    for (; splat; splat = splat->next)
    {
        // For each splat
        //BP: don't draw splat extern to this seg
        //    this is quick fix best is explain in logboris.txt at 12-4-2000
        if (!M_PointInBox(segbbox, splat->v1.x, splat->v1.y)
            && !M_PointInBox(segbbox, splat->v2.x, splat->v2.y))
            continue;

        gpatch = W_CachePatchNum(splat->patch, PU_CACHE);
        HWR_GetPatch(gpatch);

        // Consider unrolling the loop and merge with the first assigns.
        vxtx[0].x = vxtx[3].x = FIXED_TO_FLOAT( splat->v1.x );
        vxtx[0].z = vxtx[3].z = FIXED_TO_FLOAT( splat->v1.y );
        vxtx[2].x = vxtx[1].x = FIXED_TO_FLOAT( splat->v2.x );
        vxtx[2].z = vxtx[1].z = FIXED_TO_FLOAT( splat->v2.y );

        tmy = splat->top;
        if (splat->yoffset)
            tmy += *splat->yoffset;

        vxtx[2].y = vxtx[3].y = FIXED_TO_FLOAT( tmy ) + (gpatch->height >> 1);
        vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( tmy ) - (gpatch->height >> 1);

        vxtx[3].sow = vxtx[3].tow = vxtx[2].sow = vxtx[0].tow = 0.0f;
        vxtx[1].sow = vxtx[1].tow = vxtx[2].tow = vxtx[0].sow = 1.0f;

        memcpy(&pSurf2, pSurfin, sizeof(FSurfaceInfo_t));
        switch (splat->flags & SPLATDRAWMODE_MASK)
        {
            case SPLATDRAWMODE_OPAQUE:
                pSurf2.FlatColor.s.alpha = 0xff;
                blendmode = PF_Translucent;
                break;
            case SPLATDRAWMODE_TRANS:
                pSurf2.FlatColor.s.alpha = 128;
                blendmode = PF_Translucent;
                break;
            case SPLATDRAWMODE_SHADE:
                pSurf2.FlatColor.s.alpha = 0xff;
                blendmode = PF_Subtractive;
                break;
        }

        HWD.pfnDrawPolygon(&pSurf2, vxtx, 4,
                           blendmode | PF_Modulated | PF_Clip | PF_Decal);
    }
}
#endif

#ifdef R_FAKEFLOORS
// [WDJ] Render a fog sheet
void HWR_RenderFog( ffloor_t* fff, sector_t * intosec, int foglight,
                  float dist )
{
    line_t * fogline = fff->master;
    side_t * fogside = & sides[ fogline->sidenum[0] ];
    sector_t * modelsec = fogside->sector;

    FSurfaceInfo_t Surf;
    vxtx3d_t fVert[4];

    int      texnum;
    int      blend;
    fixed_t  wclip_top, wclip_bottom;

    float leftoffset = 0.0, rightoffset = FOG_WIDTH;
    float tz, tdz, tyhigh, tylow, tranzy, tranzz;
    fixed_t h, l;               // 3D sides and 2s middle textures

    // texture num are either 0=no-texture, or valid
    texnum = texturetranslation[fogside->midtexture];
    if( texnum == 0 )  goto nofog;  // no texture to display
   
    tdz = 0.0;
    tz = dist; // dist to fog
    if( dist < 0.1 )
    {
        // random fog scale
        tz = (fog_wave2>>6) + 6;  // 22 .. 6, fogsheet dist
        tdz = ((int)fog_wave1>>7) - 4;  // ( 4 .. -4 )
        tdz *= sprite_trans_z_to_z;  // fogsheet sway
    }

    wclip_top = intosec->ceilingheight;
    wclip_bottom = intosec->floorheight;
    h = modelsec->ceilingheight;
    l = modelsec->floorheight;
    if (h < wclip_bottom || l > wclip_top)
        goto nofog;
    if (h > wclip_top)
        h = wclip_top;
    if (l < wclip_bottom)
        l = wclip_bottom;
    //  3--2   vertices ordering
    //  |  |
    //  0--1
    // Combined transforms for look up/down and scaling
    // assume viewcos = 1, viewsin = 0
    tyhigh = FIXED_TO_FLOAT( h ) - gr_viewz;
    tylow  = FIXED_TO_FLOAT( l ) - gr_viewz;
    fVert[0].x = fVert[3].x = -gr_centerx;
    fVert[1].x = fVert[2].x = gr_centerx;
    tranzy = tz * sprite_trans_z_to_y;
    fVert[0].y = fVert[1].y = (tylow * sprite_trans_y_to_y) + tranzy;
    fVert[2].y = fVert[3].y = (tyhigh * sprite_trans_y_to_y) + tranzy;
    tranzz = tz * sprite_trans_z_to_z;
    fVert[0].z = fVert[1].z = (tylow * sprite_trans_y_to_z) + tranzz;
    fVert[2].z = fVert[3].z = (tyhigh * sprite_trans_y_to_z) + tranzz;
    fVert[0].z += tdz;  // sway
    fVert[3].z += tdz;
    fVert[1].z -= tdz;
    fVert[2].z -= tdz;

    Surf.polyflags = 0;
    Surf.texflags = 0;

    //  light
    Surf.FlatColor.s.alpha = 0xff;
    if (fixedcolormap)
    {
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
    }
    else
    {
        byte lightlum = LightLevelToLum_extra(modelsec->lightlevel, foglight);

        // store Surface->FlatColor to modulate wall texture
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = lightlum;

        if (modelsec->extra_colormap || view_extracolormap)
        {
           Extracolormap_to_Surf( /*IN*/ modelsec->extra_colormap, lightlum,
                                  /*OUT*/ & Surf );
        }
    }

    if (fff->flags & FF_FOG)
    {
        // Legacy Fog (or foggy water) in tagged sector
        // add in player movement to fog
        int playermov = (viewmobj->x + viewmobj->y + (viewmobj->angle>>6)) >> (FRACBITS+6);
        leftoffset = (fog_tic - playermov) % FOG_WIDTH;
        rightoffset += leftoffset;
        dr_alpha = fweff[fff->fw_effect].fsh_alpha;  // dr_alpha 0..255
        Surf.FlatColor.s.alpha = (float)dr_alpha * (251.0f/256.0f) + 5.0f;
//        Surf.FlatColor.s.alpha = (float)dr_alpha * (246.0f/256.0f) + 10.0f;
//        Surf.FlatColor.s.alpha = (float)dr_alpha * (241.0f/256.0f) + 15.0f;
//        Surf.FlatColor.s.alpha = (float)dr_alpha * (226.0f/256.0f) + 20.0f;
        Surf.texflags = TF_Fogsheet;
        blend = PF_Fog;
    }
    else
        goto nofog;

    // miptex is for the base texture, fog texture has fixed size
    // MipTexture_t * miptex = HWR_GetTexture(texnum, Surf.texflags);
    HWR_GetTexture(texnum, Surf.texflags);

    fVert[3].tow = fVert[2].tow = fog_bltic * (1.0f / (FOG_HEIGHT * 128.0f * 4.0f));
    fVert[0].tow = fVert[1].tow = fVert[3].tow + (1.0f / (FOG_HEIGHT * 128.0f * 4.0f));
    fVert[0].sow = fVert[3].sow = leftoffset * (1.0f/FOG_WIDTH);
    fVert[2].sow = fVert[1].sow = rightoffset * (1.0f/FOG_WIDTH);

    HWD.pfnDrawPolygon(&Surf, fVert, 4,
                       blend | PF_Modulated | PF_NoDepthTest);

nofog:
    return;
}
#endif

// ==========================================================================
//                                        WALL GENERATION FROM SUBSECTOR SEGS
// ==========================================================================

// [WDJ] 6/1/2010 Translucent to GL operations table
typedef struct
{
    uint32_t  PF_op;
    uint32_t  drawflags;
    byte alpha_equiv;
} translucent_lookup_t;

static  translucent_lookup_t  translucent_lookup[] =
{
   {0, 0, 0},  // not translucent
   {PF_Translucent, 0, 0x78 }, // TRANSLU_med
   {PF_Translucent, 0, 0x40 }, // TRANSLU_more
   {PF_Translucent, 0, 0x30 }, // TRANSLU_hi
   {PF_Additive,    0, 0x80 }, // TRANSLU_fire
   {PF_Translucent, TF_Opaquetrans, 0xff, }, // TRANSLU_fx1
   {PF_Translucent, 0, 0xB8 }, // TRANSLU_75
};

// Called from HWR_StoreWallRange, HWR_DrawSprite
// sets pSurf->texflags, polyflags
int HWR_TranstableToAlpha(int transtablenum, FSurfaceInfo_t * pSurf)
{
    int pfop = PF_Translucent;  // default
    if( transtablenum >= TRANSLU_ext )
    {
        // [WDJ] get substitute map from translu_store
        translucent_map_t * tm = & translu_store[ transtablenum - TRANSLU_ext ];
#if 1
        if( tm->substitute_error < 30 && tm->substitute_std_translucent <= TRANSLU_fx1 )
        {
            // use a std translucent tuned operation
            transtablenum = tm->substitute_std_translucent;
        }
        else
        {
            // use the analyzed alpha
            // OpenGL alpha tends to be too strong compared to translucent tables
            pSurf->FlatColor.s.alpha = (((int)tm->alpha) * 220) >> 8;
            if( tm->opaque > 50 )
               pfop = PF_Additive;
            goto done;
        }
#else
        // simple substitution, for testing
        // OpenGL drawing order is an issue for translucents
        // for instance in hell ground wad and BOOMEDIT.WAD
        transtablenum = tm->substitute_std_translucent;
#endif
    }
    if( transtablenum <= TRANSLU_75 )
    {
        translucent_lookup_t *  tlup = & translucent_lookup[ transtablenum ];
        pSurf->FlatColor.s.alpha = tlup->alpha_equiv;
        pfop = tlup->PF_op;
        pSurf->polyflags = pfop;  // PF_ flags
        pSurf->texflags = tlup->drawflags;  // TF_ flags
    }
done:   
    return pfop;
}

// v1,v2 : the start & end vertices along the original wall segment, that may have been
//         clipped so that only a visible portion of the wall seg is drawn.
// floorheight, ceilingheight : depend on wall upper/lower/middle, comes from the sectors.

static void HWR_AddTransparentWall(vxtx3d_t * vxtx, FSurfaceInfo_t * pSurf, int texnum, int blend);

// -----------------+
// HWR_ProjectWall  :
// -----------------+
/*
   vxtx order is :
          3--2
          | /|
          |/ |
          0--1
*/
// Called from HWR_SplitWall, HWR_StoreWallRange
static
void HWR_ProjectWall(vxtx3d_t * vxtx, FSurfaceInfo_t * pSurf, int blendmode)
{
    HWD.pfnDrawPolygon(pSurf, vxtx, 4,
                       blendmode | PF_Modulated | PF_Occlude | PF_Clip);

    if (gr_curline->linedef->splats && cv_splats.value)
        HWR_DrawSegsSplats(pSurf);

    //Hurdler: TODO: do static lighting using gr_curline->lm
    HWR_WallLighting(vxtx);

    //Hurdler: for better dynamic light in dark area, we should draw the light first
    //         and then the wall all that with the right blending func
    //HWD.pfnDrawPolygon( pSurf, vxtx, 4,
    //                    PF_Additive|PF_Modulated|PF_Occlude|PF_Clip);
}

#if 1
#define HWR_RenderWall  HWR_ProjectWall
#else
// Called from HWR_RenderTransparentWalls
static
void HWR_RenderWall(vxtx3d_t * vxtx, FSurfaceInfo_t * pSurf, int blendmode)
{
    HWD.pfnDrawPolygon(pSurf, vxtx, 4,
                       blendmode | PF_Modulated | PF_Occlude | PF_Clip);

    if (gr_curline->linedef->splats && cv_splats.value)
        HWR_DrawSegsSplats(pSurf);

    //Hurdler: TODO: do static lighting using gr_curline->lm
    HWR_WallLighting(vxtx);
}
#endif

// ==========================================================================
//                                                          BSP , CULL, ETC..
// ==========================================================================

// return the frac from the interception of the clipping line
// (in fact a clipping plane that has a constant, so can clip with simple 2d)
// with the wall segment
//
static
float HWR_ClipViewSegment(int x, polyvertex_t * v1, polyvertex_t * v2)
{
    float num;
    float den;
    float v1x, v1y;
    float v1dx, v1dy;
    float v2dx, v2dy;

    angle_t clipangle = gr_x_to_viewangle[x];

    // a segment of a polygon
    v1x = v1->x;
    v1y = v1->y;
    v1dx = (v2->x - v1->x);
    v1dy = (v2->y - v1->y);

    // the clipping line
    clipangle = clipangle + dup_viewangle;      // back to normal angle (non-relative)
    v2dx = FIXED_TO_FLOAT( cosine_ANG(clipangle) );
    v2dy = FIXED_TO_FLOAT( sine_ANG(clipangle) );

    den = v2dy * v1dx - v2dx * v1dy;
    if (den == 0)
        return -1;      // parallel

    // calc the frac along the polygon segment,
    //num = (v2x - v1x)*v2dy + (v1y - v2y)*v2dx;
    //num = -v1x * v2dy + v1y * v2dx;
    num = (gr_viewx - v1x) * v2dy + (v1y - gr_viewy) * v2dx;

    return num / den;
}

//
// HWR_SplitWall
//
//   drawflag : draw flags appropriate for the texture position
static
void HWR_SplitWall(sector_t * sector, vxtx3d_t * vxtx, int texnum,
                   FSurfaceInfo_t * Surfp, uint32_t fflags, uint32_t cutflag, uint32_t drawflag)
{
    // SplitWall will call HWR_ProjectWall multiple times, each call invokes dynamic lighting of its wall.
    // The dynamic lighting overwrites the tow and sow of the vxtx with lighting values, so the texture values must be restored.
    float  sw_tow[2];  // vxtx[0].tow = [1], [2] = [3]  (pegb, pegt)
    float  sw_sow[2];  // vxtx[0].sow = [3], [1] = [2]

    /*
       SoM: split up and light walls according to the
       lightlist. This may also include leaving out parts
       of the wall that can't be seen
     */
    MipTexture_t * miptex;
    float realtop, realbot, top, bot;
    float pegmul;
    float height, bheight = 0;
    int solid, i;
    byte base_alpha = Surfp->FlatColor.s.alpha;
    ff_light_t * ffl_list = sector->lightlist;  // fakefloor lightlist
    ffloor_t * caster;

    sw_tow[0] = vxtx[0].tow;  // pegb
    sw_tow[1] = vxtx[2].tow;  // pegt
    sw_sow[0] = vxtx[0].sow;
    sw_sow[1] = vxtx[1].sow;

    realtop = top = vxtx[2].y;
    realbot = bot = vxtx[0].y;
    pegmul = (sw_tow[0] - sw_tow[1]) / (top - bot);

    for (i = 1; i < sector->numlights; i++)
    {
        // check each fake floor for
        if (top < realbot)
            return;

        Surfp->FlatColor.s.alpha = base_alpha;

        // check each ffloor light for blocking or affecting this wall draw
        caster = ffl_list[i].caster;

        //Hurdler: fix a crashing bug, but is it correct?
        //if (!list[i].caster)
        //    continue;

        if ( caster )
        {
            solid = caster->flags & cutflag;
            if( (fflags & FF_JOIN_SIDES)
                && ( (caster->flags & (FF_TRANSLUCENT|FF_FOG)) == (fflags & (FF_TRANSLUCENT|FF_FOG)) ) )
            {
//	        Surfp->FlatColor.s.alpha = base_alpha * 0.4f;  // JOIN
                Surfp->FlatColor.s.alpha = base_alpha * 0.45f;  // JOIN
                solid = false;
            }
        }
        else
            solid = false;

        height = FIXED_TO_FLOAT( ffl_list[i].height );
        if (solid)
            bheight = FIXED_TO_FLOAT( *caster->bottomheight );

        if (height >= top)
        {
            if (solid && top > bheight)
                top = bheight;
            continue;
        }

        //Found a break;
        bot = height;

        if (bot < realbot)
            bot = realbot;

        // draw a portion of the wall 
        if (!fixedcolormap)
        {
            sector_t * sector = (ffl_list[i - 1].caster)? &sectors[ ffl_list[i - 1].caster->model_secnum] : gr_frontsector;

            byte lightlum = LightLevelToLum(*ffl_list[i - 1].lightlevel);
            // store Surface->FlatColor to modulate wall texture
            Surfp->FlatColor.s.red = Surfp->FlatColor.s.green = Surfp->FlatColor.s.blue = lightlum;

            //Hurdler: colormap test
            if (sector->extra_colormap || view_extracolormap)
            {
                Extracolormap_to_Surf( /*IN*/ sector->extra_colormap, lightlum,
                                       /*OUT*/ Surfp );
            }
        }

        vxtx[3].tow = vxtx[2].tow = sw_tow[1] + ((realtop - top) * pegmul);
        vxtx[0].tow = vxtx[1].tow = sw_tow[1] + ((realtop - bot) * pegmul);
        vxtx[0].sow = vxtx[3].sow = sw_sow[0];  // restore orig texture sow
        vxtx[2].sow = vxtx[1].sow = sw_sow[1];

        // set top/bottom coords
        vxtx[2].y = vxtx[3].y = top;
        vxtx[0].y = vxtx[1].y = bot;

        miptex = HWR_GetTexture(texnum, Surfp->texflags);
        if (miptex->mipmap.tfflags & (TF_TRANSPARENT|TF_Fogsheet)
           ||  ( Surfp->polyflags & (PF_Translucent|PF_Fog)) )
            HWR_AddTransparentWall(vxtx, Surfp, texnum, Surfp->polyflags);
        else
            HWR_ProjectWall(vxtx, Surfp, drawflag);

        top = (solid)? bheight : height;
    }

    bot = realbot;
    if (top <= realbot)
        return;

    // draw portion of wall below all ffloors
    if (!fixedcolormap)
    {
        sector_t * sector = ffl_list[i - 1].caster ? &sectors[ffl_list[i - 1].caster->model_secnum] : gr_frontsector;

        byte lightlum = LightLevelToLum(*ffl_list[i - 1].lightlevel);
        // store Surface->FlatColor to modulate wall texture
        Surfp->FlatColor.s.red = Surfp->FlatColor.s.green = Surfp->FlatColor.s.blue = lightlum;

        if (sector->extra_colormap || view_extracolormap)
        {
            Extracolormap_to_Surf( /*IN*/ sector->extra_colormap, lightlum,
                                   /*OUT*/ Surfp );
        }
    }

    vxtx[3].tow = vxtx[2].tow = sw_tow[1] + ((realtop - top) * pegmul);
    vxtx[0].tow = vxtx[1].tow = sw_tow[1] + ((realtop - bot) * pegmul);
    vxtx[0].sow = vxtx[3].sow = sw_sow[0];  // restore orig texture sow
    vxtx[2].sow = vxtx[1].sow = sw_sow[1];

    // set top/bottom coords
    vxtx[2].y = vxtx[3].y = top;
    vxtx[0].y = vxtx[1].y = bot;

    miptex = HWR_GetTexture(texnum, Surfp->texflags);
    if (miptex->mipmap.tfflags & (TF_TRANSPARENT|TF_Fogsheet)
        ||  ( Surfp->polyflags & (PF_Translucent|PF_Fog)) )
        HWR_AddTransparentWall(vxtx, Surfp, texnum, Surfp->polyflags);
    else
        HWR_ProjectWall(vxtx, Surfp, drawflag);
}

//
// HWR_StoreWallRange
// A portion or all of a wall segment will be drawn, from startfrac to endfrac,
//  where 0 is the start of the segment, 1 the end of the segment
// Anything between means the wall segment has been clipped with solidsegs,
//  reducing wall overdraw to a minimum
//
// GLOBAL IN:
//   gr_curline
//   gr_frontsector
//   gr_backsector
// GLOBAL OUT:
//   gr_linedef
//   gr_sidedef
// Called from HWR_ClipSolidWallSegment, HWR_ClipPassWallSegment
static void HWR_StoreWallRange(float startfrac, float endfrac)
{
    vxtx3d_t vxtx[4];
    v2d_t vs, ve;               // start, end vertices of 2d line (view from above)

    fixed_t worldtop;		// front sector
    fixed_t worldbottom;
    fixed_t worldbacktop = 0;	// back sector, only used on two sided lines
    fixed_t worldbackbottom = 0;
    float   skybottom = 2E10;

    MipTexture_t * miptex = NULL;
    float cliplow, cliphigh;
    int midtexnum;
    int blendmode;
    fixed_t h, l;               // 3D sides and 2s middle textures

    FSurfaceInfo_t Surf;

    if (startfrac > endfrac)
        return;

    gr_sidedef = gr_curline->sidedef;
    gr_linedef = gr_curline->linedef;

    // mark the segment as visible for auto map
    gr_linedef->flags |= ML_MAPPED;

    worldtop = gr_frontsector->ceilingheight;
    worldbottom = gr_frontsector->floorheight;

    vs.x = gr_curline->pv1->x;
    vs.y = gr_curline->pv1->y;
    ve.x = gr_curline->pv2->x;
    ve.y = gr_curline->pv2->y;

    //
    // clip the wall segment to solidsegs
    //

/*  BP : removed since there is no more clipwalls !
    // clip start of segment
    if (startfrac > 0){
        if (startfrac>1)
        {
#ifdef PARANOIA
            CONS_Printf ("startfrac %f\n", startfrac );
#endif
            startfrac = 1;
        }
            vs.x = vs.x + (ve.x - vs.x) * startfrac;
            vs.y = vs.y + (ve.y - vs.y) * startfrac;
        }

    // clip end of segment
    if (endfrac < 1){
        if (endfrac<0)
        {
#ifdef PARANOIA
            CONS_Printf ("  endfrac %f\n", endfrac );
#endif
            endfrac=0;
        }
            ve.x = vs.x + (ve.x - vs.x) * endfrac;
            ve.y = vs.y + (ve.y - vs.y) * endfrac;
        }
*/
    // remember vertices ordering
    //  3--2
    //  | /|
    //  |/ |
    //  0--1
    // make a wall polygon (with 2 triangles), using the floor/ceiling heights,
    // and the 2d map coords of start/end vertices
    vxtx[0].x = vxtx[3].x = vs.x;
    vxtx[0].z = vxtx[3].z = vs.y;
    vxtx[2].x = vxtx[1].x = ve.x;
    vxtx[2].z = vxtx[1].z = ve.y;
//    vxtx[0].w = vxtx[1].w = vxtx[2].w = vxtx[3].w = 1.0f;

    {
        // x offset the texture
        fixed_t texturehpeg = gr_sidedef->textureoffset + gr_curline->offset;

        // clip texture s start/end coords with solidsegs
        if (startfrac > 0.0 && startfrac < 1.0)
            cliplow = texturehpeg + gr_curline->length * startfrac;
        else
            cliplow = texturehpeg;

        if (endfrac > 0.0 && endfrac < 1.0)
            cliphigh = texturehpeg + gr_curline->length * endfrac;
        else
            cliphigh = texturehpeg + gr_curline->length;
    }

    Surf.polyflags = PF_Environment;
    Surf.texflags = 0;

    //  use different light tables
    //  for horizontal / vertical / diagonal
    //  note: try to get the same visual feel as the original
    Surf.FlatColor.s.alpha = 0xff;
    if (fixedcolormap)
    {
        // TODO: better handling of fixedcolormap
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
        // See PrBoom use of
        // glDisable(GL_SHARED_TEXTURE_PALETTE_EXT);
        // glEnable(GL_TEXTURE_GEN_S);
        // glEnable(GL_TEXTURE_GEN_T);
        // glEnable(GL_TEXTURE_GEN_Q);
        // glColor4fv(gl_whitecolor);
    }
    else
    {
#if 1
#define ORIENT_LIGHT   16
        lightlev_t lightv = gr_frontsector->lightlevel;

        // wall orient light
        // Added before conversion to lum to take advantage of limit clip in LightLevelToLum.
        if( vs.y == ve.y )
            lightv -= ORIENT_LIGHT;
        else if( vs.x == ve.x )
            lightv += ORIENT_LIGHT;

        byte lightlum = LightLevelToLum( lightv );
#else
        lightlev_t lightv = gr_frontsector->lightlevel;
        byte lightlum = LightLevelToLum(gr_frontsector->lightlevel);

        // wall orient light
        if ((vs.y == ve.y) && lightlum >= (255 / LIGHTLEVELS))
            lightlum -= (255 / LIGHTLEVELS);
        else if ((vs.x == ve.x) && lightlum < (255 - (255 / LIGHTLEVELS)))
            lightlum += (255 / LIGHTLEVELS);
#endif

        // store Surface->FlatColor to modulate wall texture
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = lightlum;

        //Hurdler: it seems to be better here :)
        if ( view_extracolormap )
        {
            Extracolormap_to_Surf( /*IN*/ view_extracolormap, lightlum,
                                   /*OUT*/ & Surf );
        }
        else if (gr_frontsector)
        {
            sector_t * sector = gr_frontsector;

            //Hurdler: colormap test
            if (sector->ffloors)
            {
                ffloor_t * caster =
                  R_GetPlaneLight(sector, sector->floorheight)->caster;
                sector = caster ? &sectors[caster->model_secnum] : sector;
            }
            if (sector->extra_colormap)
            {
                Extracolormap_to_Surf( /*IN*/ sector->extra_colormap, lightlum,
                                       /*OUT*/ & Surf );
            }
        }
    }

    if (gr_backsector)
    {
        // two sided line
        worldbacktop = gr_backsector->ceilingheight;
        worldbackbottom = gr_backsector->floorheight;

        // hack to allow height changes in outdoor areas
        if (gr_frontsector->ceilingpic == sky_flatnum && gr_backsector->ceilingpic == sky_flatnum)
        {
            worldtop = worldbacktop;
        }

        // check TOP TEXTURE
        // texture num are either 0=no-texture, or valid
        int toptexnum = texturetranslation[gr_sidedef->toptexture];
        if (worldbacktop < worldtop && toptexnum)
        {
            // Top pegging
            // [WDJ] sometimes maybe textureheight[gr_sidedef->toptexture] != textureheight[toptexnum]
//          texturevpegtop = worldbacktop + textureheight[gr_sidedef->toptexture] - worldtop;
            fixed_t texturevpegtop = (gr_linedef->flags & ML_DONTPEGTOP)? 0
                : worldbacktop + textureheight[toptexnum] - worldtop;	     
            texturevpegtop += gr_sidedef->rowoffset;

            miptex = HWR_GetTexture(toptexnum, 0);

            vxtx[3].tow = vxtx[2].tow = texturevpegtop * miptex->scaleY;
            vxtx[0].tow = vxtx[1].tow = (texturevpegtop + worldtop - worldbacktop) * miptex->scaleY;
            vxtx[0].sow = vxtx[3].sow = cliplow * miptex->scaleX;
            vxtx[2].sow = vxtx[1].sow = cliphigh * miptex->scaleX;

            // set top/bottom coords
            vxtx[2].y = vxtx[3].y = skybottom = FIXED_TO_FLOAT( worldtop );
            vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( worldbacktop );

            Surf.polyflags = PF_Environment;
            if (gr_frontsector->numlights)
            {
                Surf.polyflags = PF_Masked | PF_NoAlphaTest;
                HWR_SplitWall(gr_frontsector, vxtx, toptexnum, &Surf,
                              0, FF_CUTSOLIDS, PF_Masked | PF_NoAlphaTest );
            }
            else if (miptex->mipmap.tfflags & TF_TRANSPARENT)
            {
                // Top texture, above opening.
                // Transparent texture drawn single sided.	      
                // Draw with transparent as black.
                HWR_ProjectWall(vxtx, &Surf, PF_Masked | PF_NoAlphaTest);  // ignores alpha, alpha=0 ==> black
//                HWR_AddTransparentWall(vxtx, &Surf, toptexnum, PF_Environment);   // previously
            }
            else
            {
//                HWR_ProjectWall(vxtx, &Surf, PF_Masked);
                HWR_ProjectWall(vxtx, &Surf, PF_Masked | PF_NoAlphaTest);
            }
        }

        // check BOTTOM TEXTURE
        // texture num are either 0=no-texture, or valid
        int bottomtexnum = texturetranslation[gr_sidedef->bottomtexture];
        if (worldbackbottom > worldbottom && bottomtexnum)    //only if VISIBLE!!!
        {
            // Bottom pegging
            fixed_t texturevpegbottom = (gr_linedef->flags & ML_DONTPEGBOTTOM)?
                worldtop - worldbackbottom
              : 0;
            texturevpegbottom += gr_sidedef->rowoffset;

            miptex = HWR_GetTexture(bottomtexnum, 0);

            vxtx[3].tow = vxtx[2].tow = texturevpegbottom * miptex->scaleY;
            vxtx[0].tow = vxtx[1].tow = (texturevpegbottom + worldbackbottom - worldbottom) * miptex->scaleY;
            vxtx[0].sow = vxtx[3].sow = cliplow * miptex->scaleX;
            vxtx[2].sow = vxtx[1].sow = cliphigh * miptex->scaleX;

            // set top/bottom coords
            vxtx[2].y = vxtx[3].y = FIXED_TO_FLOAT( worldbackbottom );
            vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( worldbottom );

            Surf.polyflags = PF_Environment;
            if (gr_frontsector->numlights)
            {
                Surf.polyflags = PF_Masked | PF_NoAlphaTest;
                HWR_SplitWall(gr_frontsector, vxtx, bottomtexnum, &Surf,
                              0, FF_CUTSOLIDS, PF_Masked | PF_NoAlphaTest );
            }
            else if (miptex->mipmap.tfflags & TF_TRANSPARENT)
            {
                // Bottom texture, below opening.
                // Transparent texture drawn single sided.	      
                HWR_ProjectWall(vxtx, &Surf, PF_Masked | PF_NoAlphaTest);  // ignores alpha, alpha=0 ==> black
//                HWR_AddTransparentWall(vxtx, &Surf, bottomtexnum, PF_Environment);  // previously
            }
            else
            {
                HWR_ProjectWall(vxtx, &Surf, PF_Masked | PF_NoAlphaTest);
//                HWR_ProjectWall(vxtx, &Surf, PF_Masked);
            }
        }

        // texture num are either 0=no-texture, or valid
        midtexnum = texturetranslation[gr_sidedef->midtexture];
        if (midtexnum)
        {
            int  clip_disable = (gr_linedef->flags & ML_DONTDRAW);
            fixed_t opentop, openbottom, polytop, polybottom;

            blendmode = PF_Masked;
            if(gr_linedef->translu_eff)  // Boom 260: make translucent
            {
                blendmode = HWR_TranstableToAlpha(gr_linedef->translu_eff, &Surf);
            }
            // set alpha for transparent walls (new boom and legacy linedef types)
            switch (gr_linedef->special)
            {
                case 260:  // Boom make translucent
                      // see test on translu_eff
//                    blendmode = HWR_TranstableToAlpha(gr_linedef->translu_eff, &Surf);
                    break;
                           // Legacy translucent  284 to 288
                case 284:  // Legacy translucent, brighten (greenish)
                    blendmode = HWR_TranstableToAlpha(TRANSLU_med, &Surf);
                    break;
                case 285:  // Legacy translucent, brighten (less greenish)
                    blendmode = HWR_TranstableToAlpha(TRANSLU_more, &Surf);
                    break;
                case 286:  // Legacy translucent, darkens
                    blendmode = HWR_TranstableToAlpha(TRANSLU_hi, &Surf);
                    break;
                case 287:  // Legacy translucent, brightens
                    blendmode = HWR_TranstableToAlpha(TRANSLU_fire, &Surf);
                    break;
                case 288:  // Legacy selective translucent, on selected colors
                    // sets TF_Opaquetrans in texflags
                    blendmode = HWR_TranstableToAlpha(TRANSLU_fx1, &Surf);
                    break;
                case 283:  // Legacy fog sheet
                    blendmode = PF_Fog;
                    Surf.FlatColor.s.alpha = 64;
                    Surf.texflags = TF_Fogsheet;
                    break;
                default:
                    // do not override Boom 260 tagged assign of blendmode
//                    blendmode = PF_Masked;
                    break;
            }

            // TRANSLU_fx1 requires TF_Opaquetrans flag to draw texture
            miptex = HWR_GetTexture(midtexnum, Surf.texflags);

            if (miptex && (miptex->mipmap.tfflags & TF_TRANSPARENT))
            {
                blendmode = PF_Environment;
                clip_disable = 1;  // full height, no (h-l) clipping
            }

#if 1
            {
                // [WDJ] ic2005.wad has three textures that are pegged
                // to moving DeepWater sectors.
                // 1) translucent over DeepWater door, yoffset=+120
                //    tried if( blendmode != PF_Masked )
                // 2) transparent electric arc, under DeepWater ceiling.
                //    tried if(miptex->mipmap.tfflags & TF_TRANSPARENT)
                // 3) small texture grid, with moving DeepWater ceiling and floor
                //    no test
                //
                // These must peg to the actual sectors, ignoring DeepWater
                worldtop = gr_linedef->frontsector->ceilingheight;
                worldbottom = gr_linedef->frontsector->floorheight;
                worldbacktop = gr_linedef->backsector->ceilingheight;
                worldbackbottom = gr_linedef->backsector->floorheight;
            }
#endif

            // SoM: a little note: This code re-arranging will
            // fix the bug in Nimrod map02. opentop and openbottom
            // record the limits the texture can be displayed in.
            // polytop and polybottom, are the ideal (i.e. unclipped)
            // heights of the polygon, and h & l, are the final (clipped)
            // poly coords.

            opentop = (worldtop < worldbacktop) ? worldtop : worldbacktop;
            openbottom = (worldbottom > worldbackbottom) ? worldbottom : worldbackbottom;

            if (gr_linedef->flags & ML_DONTPEGBOTTOM)
            {
                polybottom = openbottom + gr_sidedef->rowoffset;
                polytop = polybottom + textureheight[midtexnum];
            }
            else
            {
                polytop = opentop + gr_sidedef->rowoffset;
                polybottom = polytop - textureheight[midtexnum];
            }
            if ((gr_frontsector->ceilingheight == gr_backsector->ceilingheight)
                || clip_disable )
                h = polytop;
            else
                h = (polytop < opentop) ? polytop : opentop;

            if ((gr_frontsector->floorheight == gr_backsector->floorheight)
                || clip_disable )
                l = polybottom;
            else
                l = (polybottom > openbottom) ? polybottom : openbottom;

            {
                fixed_t texturevpeg;
                // PEGGING
                if (gr_linedef->flags & ML_DONTPEGBOTTOM)
                    texturevpeg = l + textureheight[gr_sidedef->midtexture] - h + polybottom - l;
                else
                    texturevpeg = polytop - h;

                vxtx[3].tow = vxtx[2].tow = texturevpeg * miptex->scaleY;
                vxtx[0].tow = vxtx[1].tow = (h - l + texturevpeg) * miptex->scaleY;
                vxtx[0].sow = vxtx[3].sow = cliplow * miptex->scaleX;
                vxtx[2].sow = vxtx[1].sow = cliphigh * miptex->scaleX;
            }
            // set top/bottom coords
            vxtx[2].y = vxtx[3].y = FIXED_TO_FLOAT( h );
            vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( l );
#if 0
            // [WDJ] Causes transparent signs to block wall behind.	   
            if( skybottom > 1E10)
                skybottom = vxtx[2].y;
#endif

            if (blendmode != PF_Masked)
                HWR_AddTransparentWall(vxtx, &Surf, midtexnum, blendmode);
            else
                HWR_ProjectWall(vxtx, &Surf, blendmode);
        }
    }
    else
    {
        // Single sided line... Deal only with the middletexture (if one exists)
        midtexnum = texturetranslation[gr_sidedef->midtexture];
        if (midtexnum)
        {
            miptex = HWR_GetTexture(midtexnum, 0);

            {
                // PEGGING
                fixed_t texturevpeg = ( (uint16_t) gr_linedef->flags & ML_DONTPEGBOTTOM )?
                    worldbottom + textureheight[gr_sidedef->midtexture] - worldtop + gr_sidedef->rowoffset
                  : gr_sidedef->rowoffset;  // top of texture at top

                vxtx[3].tow = vxtx[2].tow = texturevpeg * miptex->scaleY;
                vxtx[0].tow = vxtx[1].tow = (texturevpeg + worldtop - worldbottom) * miptex->scaleY;
                vxtx[0].sow = vxtx[3].sow = cliplow * miptex->scaleX;
                vxtx[2].sow = vxtx[1].sow = cliphigh * miptex->scaleX;
            }
            // set top/bottom coords
            vxtx[2].y = vxtx[3].y = skybottom = FIXED_TO_FLOAT( worldtop );
            vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( worldbottom );

            // I don't think that solid walls can use translucent linedef types...
            Surf.polyflags = PF_Environment;
            if (gr_frontsector->numlights)
            {
                HWR_SplitWall(gr_frontsector, vxtx, midtexnum, &Surf,
                              0, FF_CUTSOLIDS, PF_Masked | PF_NoAlphaTest);
            }
            else if (miptex->mipmap.tfflags & TF_TRANSPARENT)
            {
                // Mid texture, single sided wall.
                // Transparent texture drawn single sided.	      
                // Draw with transparent as black.
                HWR_ProjectWall(vxtx, &Surf, PF_Masked | PF_NoAlphaTest);  // ignores alpha, alpha=0 ==> black
//                HWR_AddTransparentWall(vxtx, &Surf, midtexnum, PF_Environment);  // previously
            }
            else
            {
                HWR_ProjectWall(vxtx, &Surf, PF_Masked | PF_NoAlphaTest);
//                HWR_ProjectWall(vxtx, &Surf, PF_Masked );
            }
        }
        else
        {
            skybottom = FIXED_TO_FLOAT( worldbottom );
        }
    }

    if( (gr_frontsector->ceilingpic == sky_flatnum)
        && (skybottom < 1E10) )
    {
        // [WDJ] Above upper texture is sky
        vxtx[2].y = vxtx[3].y = skybottom;
        vxtx[0].y = vxtx[1].y = 2E10;

        // [WDJ] Does not have skyflat detection, nor colormap,
        // so no place to apply cv_invul_skymap.EV.
        // See PrBoom use of
        // glDisable(GL_SHARED_TEXTURE_PALETTE_EXT);
        // glEnable(GL_TEXTURE_GEN_S);
        // glEnable(GL_TEXTURE_GEN_T);
        // glEnable(GL_TEXTURE_GEN_Q);
        // glColor4fv(gl_whitecolor);

        // Transparent, to set z buffer to block more distant draws.       
        Surf.polyflags = PF_Environment;
#ifdef BLEND_FIELD
        HWD.pfnDrawPolygon(&Surf, vxtx, 4,
                           PF_InvisibleB | PF_Occlude | PF_Clip );
#else
        HWD.pfnDrawPolygon(&Surf, vxtx, 4,
                           PF_InvisibleColor | PF_Occlude | PF_Masked | PF_Clip );
#endif
    }

    //Hurdler: 3d-floors test
#ifdef R_FAKEFLOORS
    if (gr_frontsector && gr_backsector
        && gr_frontsector->tag != gr_backsector->tag
        && (gr_backsector->ffloors || gr_frontsector->ffloors))
    {
        ffloor_t * fff, * bff;
        fixed_t highcut, lowcut;

        highcut = gr_frontsector->ceilingheight < gr_backsector->ceilingheight ? gr_frontsector->ceilingheight : gr_backsector->ceilingheight;
        lowcut = gr_frontsector->floorheight > gr_backsector->floorheight ? gr_frontsector->floorheight : gr_backsector->floorheight;

        if (gr_backsector->ffloors)
        {
            for (bff = gr_backsector->ffloors; bff; bff = bff->next)
            {
                if (!(bff->flags & FF_OUTER_SIDES) || !(bff->flags & FF_EXISTS))
                    continue;
                // outer sides backsector
                if (*bff->topheight < lowcut || *bff->bottomheight > highcut)
                    continue;

                h = *bff->topheight;
                l = *bff->bottomheight;
                if (h > highcut)
                    h = highcut;
                if (l < lowcut)
                    l = lowcut;
                //Hurdler: HW code starts here
                //FIXME: check if peging is correct
                // set top/bottom coords
                vxtx[2].y = vxtx[3].y = FIXED_TO_FLOAT( h );
                vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( l );

                // protect against missing middle texture
                midtexnum = texturetranslation[sides[bff->master->sidenum[0]].midtexture];
                if( midtexnum == 0 ) continue;  // no texture to display (when 3Dslab is missing side texture)

                miptex = HWR_GetTexture( midtexnum, 0 );

                vxtx[3].tow = vxtx[2].tow = (*bff->topheight - h) * miptex->scaleY;
                vxtx[0].tow = vxtx[1].tow = (h - l + (*bff->topheight - h)) * miptex->scaleY;
                vxtx[0].sow = vxtx[3].sow = cliplow * miptex->scaleX;
                vxtx[2].sow = vxtx[1].sow = cliphigh * miptex->scaleX;

                if (bff->flags & FF_FOG)
                {
                    // FF_FOG, must also be usable with water
                    // Legacy Fog in tagged sector
                    blendmode = PF_Fog;
                    dr_alpha = fweff[bff->fw_effect].fsh_alpha;  // dr_alpha 0..255
                    Surf.FlatColor.s.alpha = (float)dr_alpha * (251.0f/256.0f) + 5.0f;
//		    Surf.FlatColor.s.alpha = (float)dr_alpha * (246.0f/256.0f) + 10.0f;
                    Surf.texflags = TF_Fogsheet;
                }
                else
                {
                    // NOT Legacy 3D fog in tagged sector
                    blendmode = PF_Masked;

                    if (bff->flags & FF_TRANSLUCENT)
                    {
                        blendmode = PF_Translucent;
                        Surf.FlatColor.s.alpha = bff->alpha;
                    }
                    else if (miptex->mipmap.tfflags & TF_TRANSPARENT)
                    {
                        blendmode = PF_Environment;
                    }
                }

                if (gr_frontsector->numlights)
                {
//		        if( midtexnum == 0 ) continue;  // no texture to display (when 3Dslab is missing side texture)
                    Surf.polyflags = blendmode;
                    HWR_SplitWall(gr_frontsector, vxtx, midtexnum, &Surf,
                                  bff->flags,
                                  bff->flags & FF_EXTRA ? FF_CUTEXTRA : FF_CUTSOLIDS, PF_Masked );
                }
                else if (blendmode != PF_Masked)
                {
//                  if( midtexnum == 0 ) continue;  // no texture to display (when 3Dslab is missing side texture)
                    HWR_AddTransparentWall(vxtx, &Surf, midtexnum, blendmode);
                }
                else
                {
                    HWR_ProjectWall(vxtx, &Surf, PF_Masked);
                }
            }
        }
        // [WDJ] must check frontsector floors too, to get translucent sides of water
        if (gr_frontsector->ffloors)
        {
            for (fff = gr_frontsector->ffloors; fff; fff = fff->next)
            {
                if (!(fff->flags & FF_INNER_SIDES) || !(fff->flags & FF_EXISTS))
                    continue;
                // inner sides of frontsector
                if (*fff->topheight < lowcut || *fff->bottomheight > highcut)
                    continue;

                h = *fff->topheight;
                l = *fff->bottomheight;
                if (h > highcut)
                    h = highcut;
                if (l < lowcut)
                    l = lowcut;
                //Hurdler: HW code starts here
                //FIXME: check if peging is correct
                // set top/bottom coords
                vxtx[2].y = vxtx[3].y = FIXED_TO_FLOAT( h );
                vxtx[0].y = vxtx[1].y = FIXED_TO_FLOAT( l );

                // protect against missing middle texture
                midtexnum = texturetranslation[sides[fff->master->sidenum[0]].midtexture];
                if( midtexnum == 0 ) continue;  // no texture to display (when 3Dslab is missing side texture)

                miptex = HWR_GetTexture( midtexnum, 0 );

                vxtx[3].tow = vxtx[2].tow = (*fff->topheight - h) * miptex->scaleY;
                vxtx[0].tow = vxtx[1].tow = (h - l + (*fff->topheight - h)) * miptex->scaleY;
                vxtx[0].sow = vxtx[3].sow = cliplow * miptex->scaleX;
                vxtx[2].sow = vxtx[1].sow = cliphigh * miptex->scaleX;

                if (fff->flags & FF_FOG)
                {
                    // FF_FOG, must also be usable with water
                    // Legacy Fog in tagged sector
                    dr_alpha = fweff[fff->fw_effect].fsh_alpha;  // dr_alpha 0..255
                    // fog alpha needs to be biased, otherwise low alpha become invisible
                    Surf.FlatColor.s.alpha = (float)dr_alpha * (251.0f/256.0f) + 5.0f;
                    Surf.texflags = TF_Fogsheet;
                    blendmode = PF_Fog;
                }
                else
                {
                    blendmode = PF_Masked;

                    if (fff->flags & FF_TRANSLUCENT)
                    {
                        blendmode = PF_Translucent;
                        Surf.FlatColor.s.alpha = fff->alpha;
                    }
                    else if (miptex->mipmap.tfflags & TF_TRANSPARENT)
                    {
                        blendmode = PF_Environment;
                    }
                }

                if (gr_backsector->numlights)
                {
//		        if( midtexnum == 0 ) continue;  // no texture to display (when 3Dslab is missing side texture)
                    Surf.polyflags = blendmode;
                    HWR_SplitWall(gr_backsector, vxtx, midtexnum, &Surf,
                                  fff->flags,
                                  fff->flags & FF_EXTRA ? FF_CUTEXTRA : FF_CUTSOLIDS, PF_Masked );
                }
                else if (blendmode != PF_Masked)
                {
//                  if( midtexnum == 0 ) continue;  // no texture to display (when 3Dslab is missing side texture)
                    HWR_AddTransparentWall(vxtx, &Surf, midtexnum, blendmode);
                }
                else
                {
                    HWR_ProjectWall(vxtx, &Surf, PF_Masked);
                }
            }
        }
    }
#endif
//Hurdler: end of 3d-floors test
}

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef struct
{
    int first;
    int last;
} cliprange_t;

//Hurdler: just like in r_bsp.c
#define MAXSEGS         MAXVIDWIDTH/2+1

// newend is one past the last valid seg
cliprange_t * newend;
cliprange_t gr_solidsegs[MAXSEGS];

static
void print_solid_segs(void)
{
    cliprange_t * start;
    if (!newend || cv_grbeta.value != 2)
        return;
    for (start = gr_solidsegs; start != newend; start++)
        CONS_Printf("%d-%d|", start->first, start->last);
    CONS_Printf("\n\n");
}

//
//
// Called from HWR_AddLine
static void HWR_ClipSolidWallSegment(int first, int last)
{
    cliprange_t * next;
    cliprange_t * start;
    float lowfrac, highfrac;
    boolean clipwalls_fragment = false;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = gr_solidsegs;
    while (start->last < first - 1)
        start++;

    if (first < start->first)
    {
        if (last < start->first - 1)
        {
            // Post is entirely visible (above start),
            //  so insert a new clippost.
//            HWR_StoreWallRange(first, last);
            HWR_StoreWallRange(0.0, 1.0);

            next = newend;  // use empty at newend
            newend++;

            // shuffle up clipposts, start to newend-1
            while (next != start)
            {
                *next = *(next - 1);
                next--;
            }
            // insert at start
            next->first = first;
            next->last = last;
            goto done;
        }

        // There is a fragment above *start.
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
//                HWR_StoreWallRange(first, last);
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
        {
            highfrac = HWR_ClipViewSegment(start->first + 1,
                                           gr_curline->pv1, gr_curline->pv2);
            HWR_StoreWallRange(0.0, highfrac);
        }
        // Now adjust the clip size.
        start->first = first;
    }

    // Bottom contained in start?
    if (last <= start->last)
        goto done;

    next = start;
    while (last >= (next + 1)->first - 1)
    {
        // There is a fragment between two posts.
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
//                HWR_StoreWallRange(first, last);
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
        {
            lowfrac = HWR_ClipViewSegment(next->last - 1,
                                          gr_curline->pv1, gr_curline->pv2);
            highfrac = HWR_ClipViewSegment((next + 1)->first + 1,
                                          gr_curline->pv1, gr_curline->pv2);
            HWR_StoreWallRange(lowfrac, highfrac);
        }
        next++;

        if (last <= next->last)
        {
            // Bottom is contained in next.
            // Adjust the clip size.
            start->last = next->last;
            goto crunch;
        }
    }

    if (first == next->first + 1)       // 1 line texture
    {
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
//                HWR_StoreWallRange(first, last);
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
            HWR_StoreWallRange(0.0, 1.0);
    }
    else
    {
        // There is a fragment after *next.
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
//                HWR_StoreWallRange(first, last);
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
        {
            lowfrac = HWR_ClipViewSegment(next->last - 1,
                                          gr_curline->pv1, gr_curline->pv2);
            HWR_StoreWallRange(lowfrac, 1);
        }
    }

    // Adjust the clip size.
    start->last = last;

    // Remove start+1 to next from the clip list,
    // because start now covers their area.
  crunch:
    if (next == start)
        goto done;  // Post just extended past the bottom of one post.

    while (next++ != newend)
    {
        // Remove a post.
        *++start = *next;
    }

    newend = start;
 
  done:
    if( cv_grbeta.value == 2 )
       print_solid_segs();  // debug
    return;
}

//
//  handle LineDefs with upper and lower texture (windows)
//
// Called from HWR_AddLine
static void HWR_ClipPassWallSegment(int first, int last)
{
    cliprange_t * start;
    float lowfrac, highfrac;
    //to allow noclipwalls but still solidseg reject of non-visible walls
    boolean clipwalls_fragment = false;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = gr_solidsegs;
    while (start->last < first - 1)
        start++;

    if (first < start->first)
    {
        if (last < start->first - 1)
        {
            // Post is entirely visible (above start).
            HWR_StoreWallRange(0.0, 1.0);
            return;
        }

        // There is a fragment above *start.
        if (!cv_grclipwalls.value)
        {
            //20/08/99: Changed by Hurdler (taken from faB's code)
            if (!clipwalls_fragment)
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
        {
            highfrac = HWR_ClipViewSegment(min(start->first + 1, start->last),
                                           gr_curline->pv1, gr_curline->pv2);
            HWR_StoreWallRange(0.0, highfrac);
        }
    }

    // Bottom contained in start?
    if (last <= start->last)
        return;

    while (last >= (start + 1)->first - 1)
    {
        // There is a fragment between two posts.
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
        {
            lowfrac = HWR_ClipViewSegment(max(start->last - 1, start->first),
                                          gr_curline->pv1, gr_curline->pv2);
            highfrac = HWR_ClipViewSegment(min((start + 1)->first + 1, (start + 1)->last),
                                          gr_curline->pv1, gr_curline->pv2);
            HWR_StoreWallRange(lowfrac, highfrac);
        }
        start++;

        if (last <= start->last)
            return;
    }

    if (first == start->first + 1)      // 1 line texture
    {
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
            HWR_StoreWallRange(0.0, 1.0);
    }
    else
    {
        // There is a fragment after *next.
        if (!cv_grclipwalls.value)
        {
            if (!clipwalls_fragment)
                HWR_StoreWallRange(0.0, 1.0);
            clipwalls_fragment = true;
        }
        else
        {
            lowfrac = HWR_ClipViewSegment(max(start->last - 1, start->first),
                                          gr_curline->pv1, gr_curline->pv2);
            HWR_StoreWallRange(lowfrac, 1.0);
        }
    }
}

// --------------------------------------------------------------------------
//  HWR_ClipToSolidSegs check if it is hide by wall (solidsegs)
// --------------------------------------------------------------------------
static boolean HWR_ClipToSolidSegs(int first, int last)
{
    cliprange_t * start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = gr_solidsegs;
    while (start->last < first - 1)
        start++;

    if (first < start->first)
        return true;

    // Bottom contained in start?
    if (last <= start->last)
        return false;

    return true;
}

//
// HWR_Clear_ClipSegs
//
static void HWR_Clear_ClipSegs(void)
{
    gr_solidsegs[0].first = -0x7fffffff;
    gr_solidsegs[0].last = -1;
    gr_solidsegs[1].first = vid.width;  //viewwidth;
    gr_solidsegs[1].last = 0x7fffffff;
    newend = gr_solidsegs + 2;
}

// -----------------+
// HWR_AddLine      : Clips the given segment and adds any visible pieces to the line list.
// Notes            : gr_cursectorlight is set to the current subsector -> sector -> light value
//                  : ( it may be mixed with the wall's own flat colour in the future ... )
// -----------------+
// Called from HWR_Subsector.
static void HWR_AddLine(seg_t * lineseg)
{
    int x1, x2;
    angle_t angle1, angle2;
    angle_t span;

    // SoM: Backsector needs to be run through R_FakeFlat
    sector_t tempsec;

    gr_curline = lineseg;

    // OPTIMIZE: quickly reject orthogonal back sides.
#if 1
    // angle calc uses fixed_t math
    // Angles here increase to the left.
    angle1 = R_PointToAngle( lineseg->v1->x, lineseg->v1->y );  // left
    angle2 = R_PointToAngle( lineseg->v2->x, lineseg->v2->y );  // right
#else
    // convert polyvertex_t back to fixed_t
    angle1 = R_PointToAngle(((polyvertex_t *) lineseg->v1)->x * FRACUNIT, ((polyvertex_t *) lineseg->v1)->y * FRACUNIT);
    angle2 = R_PointToAngle(((polyvertex_t *) lineseg->v2)->x * FRACUNIT, ((polyvertex_t *) lineseg->v2)->y * FRACUNIT);
#endif

    // Clip to view edges.
    span = angle1 - angle2;  // normally span > 0, (angle1 > angle2)

    // backface culling : span is < ANG180 if ang1 > ang2 : the seg is facing
    if (span >= ANG180)
        return;

    // Global angle needed by segcalc.
    //rw_angle1 = angle1;
    // view relative is left 0x20000000, middle 0, right 0xe0000000
    angle1 -= dup_viewangle;
    angle2 -= dup_viewangle;

    // angle1, angle2 may range from ANG270 to ANG90, unsigned.
    // Because of angle wrap, must contrive tests away from 0.
    // Trying to use signed tests, like prboom, did not work well.
    if ((gr_clipangle + angle1) > gr_clipangle_x_2) // (angle1 > clipangle)
    {
        // Totally off the left edge?
        if ((angle1 - gr_clipangle) >= span)  // (angle1 - clipangle) >= (angle1 - angle2)
            return;    // angle2 >= clipangle

        angle1 = gr_clipangle;
    }

    if ((gr_clipangle - angle2) > gr_clipangle_x_2)  // (angle2 < -clipangle)
    {
        // Totally off the right edge?
        if ((-angle2 - gr_clipangle) >= span)  //  (-angle2 - clipangle) >= (angle1 - angle2)
            return;    // angle1 <= -clipangle

        angle2 = -gr_clipangle;
    }

#if 0
    {
        float fx1, fx2, fy1, fy2;
        //BP: test with a better projection than viewangle_to_x[R_PointToAngle(angle)]
        // do not enable this at release 4 mul and 2 div
        fx1 = lineseg->pv1->x - gr_viewx;
        fy1 = lineseg->pv1->y - gr_viewy;
        fy2 = (fx1 * gr_viewcos + fy1 * gr_viewsin);
        if (fy2 < 0)
            // the point is back
            fx1 = 0;
        else
            fx1 = gr_windowcenterx + (fx1 * gr_viewsin - fy1 * gr_viewcos) * gr_centerx / fy2;

        fx2 = ((polyvertex_t *) (lineseg->v2))->x - gr_viewx;
        fy2 = ((polyvertex_t *) (lineseg->v2))->y - gr_viewy;
        fy1 = (fx2 * gr_viewcos + fy2 * gr_viewsin);
        if (fy1 < 0)
            // the point is back
            fx2 = vid.width;
        else
            fx2 = gr_windowcenterx + (fx2 * gr_viewsin - fy2 * gr_viewcos) * gr_centerx / fy1;

        x1 = fx1 + 0.5f;
        x2 = fx2 + 0.5f;
    }
#else
    // The seg is in the view range, but not necessarily visible.
    x1 = gr_viewangle_to_x[ ANGLE_TO_FINE(angle1+ANG90) ];  // left
    x2 = gr_viewangle_to_x[ ANGLE_TO_FINE(angle2+ANG90) ];  // right
#endif
    // Does not cross a pixel?
//    if (x1 == x2)
/*    {
        // BP: HERE IS THE MAIN PROBLEM !
        //CONS_Printf("tineline\n");
        return;
    }
*/
    gr_backsector = lineseg->backsector;

    // Single sided line?
    if (!gr_backsector)
        goto clipsolid;

    gr_backsector = R_FakeFlat(gr_backsector, &tempsec, true,
                               /*OUT*/ NULL, NULL );
   
#ifdef DOORCLOSED_FIX
    // [WDJ] Improvement on door closed detection from r_bsp
    doorclosed = 0; //SoM: 3/25/2000
#endif

    // Closed door.
    if (gr_backsector->ceilingheight <= gr_frontsector->floorheight
        || gr_backsector->floorheight >= gr_frontsector->ceilingheight)
        goto clipsolid;

#ifdef DOORCLOSED_FIX
    //SoM: 3/25/2000: Check for automap fix. Store in doorclosed for r_segs.c
    if ((doorclosed = R_DoorClosed()))
      goto clipsolid;
#endif

    // Window.
    if (gr_backsector->ceilingheight != gr_frontsector->ceilingheight
        || gr_backsector->floorheight != gr_frontsector->floorheight)
        goto clippass;

#if 1
    // [WDJ] 4/20/2010 From software renderer, to get improvements
    // Reject empty lines used for triggers and special events.
    // Identical floor and ceiling on both sides,
    // identical light levels on both sides, and no middle texture.
    if (gr_backsector->ceilingpic == gr_frontsector->ceilingpic
        && gr_backsector->floorpic == gr_frontsector->floorpic
        && gr_backsector->lightlevel == gr_frontsector->lightlevel
        && gr_curline->sidedef->midtexture == 0

        //SoM: 3/22/2000: Check offsets too!
        && gr_backsector->floor_xoffs == gr_frontsector->floor_xoffs
        && gr_backsector->floor_yoffs == gr_frontsector->floor_yoffs
        && gr_backsector->ceiling_xoffs == gr_frontsector->ceiling_xoffs
        && gr_backsector->ceiling_yoffs == gr_frontsector->ceiling_yoffs

        //SoM: 3/17/2000: consider altered lighting
        && gr_backsector->floorlightsec == gr_frontsector->floorlightsec
        && gr_backsector->ceilinglightsec == gr_frontsector->ceilinglightsec
        //SoM: 4/3/2000: Consider colormaps
        && gr_backsector->extra_colormap == gr_frontsector->extra_colormap
        && ((!gr_frontsector->ffloors && !gr_backsector->ffloors) ||
           (gr_frontsector->tag == gr_backsector->tag)))
    {
        return;
    }
#else
    // Reject empty lines used for triggers and special events.
    // Identical floor and ceiling on both sides,
    //  identical light levels on both sides,
    //  and no middle texture.
    if (gr_backsector->ceilingpic == gr_frontsector->ceilingpic
        && gr_backsector->floorpic == gr_frontsector->floorpic
        && gr_backsector->lightlevel == gr_frontsector->lightlevel
        && gr_curline->sidedef->midtexture == 0
        && !gr_backsector->ffloors
        && !gr_frontsector->ffloors)
        // SoM: For 3D sides... Boris, would you like to take a
        // crack at rendering 3D sides? You would need to add the
        // above check and add code to HWR_StoreWallRange...
    {
        return;
    }
#endif

  clippass:
    if (x1 == x2)
    {
        x2++;
        x1 -= 2;
    }
    HWR_ClipPassWallSegment(x1, x2 - 1);
    return;

  clipsolid:
    if (x1 == x2)
        goto clippass;
    HWR_ClipSolidWallSegment(x1, x2 - 1);
}

// HWR_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
// modified to use local variables

extern int checkcoord[12][4];   //r_bsp.c

static boolean HWR_CheckBBox(fixed_t * bspcoord)
{
    int boxpos;
    fixed_t x1, y1, x2, y2;
    angle_t angle1, angle2;
    angle_t span;
    int sx1, sx2;

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
        boxpos |= 1 << 2;
    else
        boxpos |= 2 << 2;

    if (boxpos == 5)
        return true;

    x1 = bspcoord[checkcoord[boxpos][0]];
    y1 = bspcoord[checkcoord[boxpos][1]];
    x2 = bspcoord[checkcoord[boxpos][2]];
    y2 = bspcoord[checkcoord[boxpos][3]];

    // check clip list for an open space
    // Angles here increase to the left.
    angle1 = R_PointToAngle(x1, y1) - dup_viewangle;  // left
    angle2 = R_PointToAngle(x2, y2) - dup_viewangle;  // right

    span = angle1 - angle2;  // normally span > 0, (angle1 > angle2)

    // Sitting on a line?
    if (span >= ANG180)
        return true;

    // angle1, angle2 may range from ANG270 to ANG90, unsigned.
    // Because of angle wrap, must contrive tests away from 0.
    if ((gr_clipangle + angle1) > gr_clipangle_x_2) // (angle1 > clipangle)
    {
        // Totally off the left edge?
        if ((angle1 - gr_clipangle) >= span)  // (angle1 - clipangle) >= (angle1 - angle2)
            return false;    // angle2 >= clipangle

        angle1 = gr_clipangle;
    }
    if ((gr_clipangle - angle2) > gr_clipangle_x_2)  // (angle2 < -clipangle)
    {
        // Totally off the right edge?
        if ((-angle2 - gr_clipangle) >= span)  //  (-angle2 - clipangle) >= (angle1 - angle2)
            return false;    // angle1 <= -clipangle

        angle2 = -gr_clipangle;
    }

    // Find the first clippost that touches the source post
    //  (adjacent pixels are touching).
    sx1 = gr_viewangle_to_x[ ANGLE_TO_FINE(angle1 + ANG90) ];
    sx2 = gr_viewangle_to_x[ ANGLE_TO_FINE(angle2 + ANG90) ];

    // Does not cross a pixel.
    if (sx1 == sx2)
        return false;

    return HWR_ClipToSolidSegs(sx1, sx2 - 1);
}


// -----------------+
// HWR_Subsector    : Determine floor/ceiling planes.
//                  : Add sprites of things in sector.
//                  : Draw one or more line segments.
// Notes            : Sets gr_cursectorlight to the light of the parent sector, to modulate wall textures
// -----------------+
#ifdef DCK_WATER_TEST
static lumpnum_t  doomwaterflat;       //set by R_Init_Flats hack
#endif
static byte  need_sky_background;

// Called from HWR_RenderBSPNode
//  num : subsector number
static void HWR_Subsector(int num)
{
    int segcount;
    seg_t * lineseg;
    subsector_t * sub;
    sector_t tempsec;           //SoM: 4/7/2000
    lightlev_t  floorlightlevel;
    lightlev_t  ceilinglightlevel;
    int locFloorHeight, locCeilingHeight;
    extracolormap_t * floorcolormap = NULL, * ceilingcolormap = NULL;

//no risk while developing, enough debugging nights!
#ifdef PARANOIA
    if (num >= num_poly_subsector)
        I_Error("HWR_Subsector: ss %i with numss = %i, extrass = %d", num, numsubsectors, num_poly_subsector);

    /*
       if (num>=numsubsectors)
       I_Error ("HWR_Subsector: ss %i with numss = %i",
       num,
       numsubsectors );
     */
#endif

    if (num < numsubsectors)
    {
        sscount++;
        // subsector
        sub = &subsectors[num];
        // sector
        gr_frontsector = sub->sector;
        // how many linedefs
        segcount = sub->numlines;
        // first line seg
        lineseg = &segs[sub->firstline];
    }
    else
    {
        // there are no segs but only planes
        sub = &subsectors[0];
        gr_frontsector = sub->sector;
        segcount = 0;
        lineseg = NULL;
    }

#ifdef DEBUG_TRIG   
    if( ( num == trigger_draw_subsector )
        || ( trigger_sector < numsectors
              && sub->sector == & sectors[trigger_sector] ) )
    {
        GenPrintf(EMSG_debug, "TRIGGER SECTOR %i SUBSECTOR %i: first line= %i\n", sub->sector - sectors, trigger_draw_subsector, sub->firstline );
    }
#endif
     
    //SoM: 4/7/2000: Test to make Boom water work in Hardware mode.
    gr_frontsector = R_FakeFlat(gr_frontsector, &tempsec, false,
                                /*OUT*/ &floorlightlevel, &ceilinglightlevel );
    //FIXME: Use floorlightlevel and ceilinglightlevel insted of lightlevel.

    // ------------------------------------------------------------------------
    // sector lighting, DISABLED because it's done in HWR_StoreWallRange
    // ------------------------------------------------------------------------
    // TODO : store a RGBA instead of just intensity, allow coloured sector lighting
    //light = (byte)(sub->sector->lightlevel & 0xFF) / 255.0f;
    //gr_cursectorlight.red   = light;
    //gr_cursectorlight.green = light;
    //gr_cursectorlight.blue  = light;
    //gr_cursectorlight.alpha = light;

// ----- for special tricks with HW renderer -----
#ifdef SECTOR_FLAGS
    if( gr_frontsector->flags & SCF_pseudo_sector )
#else
    if (gr_frontsector->pseudoSector)
#endif
    {
        locFloorHeight = gr_frontsector->virtualFloorheight;
        locCeilingHeight = gr_frontsector->virtualCeilingheight;
    }
#ifdef SECTOR_FLAGS
    else if( gr_frontsector->flags & SCF_virtual_floor )
#else
    else if (gr_frontsector->virtualFloor)
#endif
    {
        locFloorHeight = gr_frontsector->virtualFloorheight;
#ifdef SECTOR_FLAGS
        if( gr_frontsector->flags & SCF_virtual_ceiling )
#else
        if (gr_frontsector->virtualCeiling)
#endif
            locCeilingHeight = gr_frontsector->virtualCeilingheight;
        else
            locCeilingHeight = gr_frontsector->ceilingheight;
    }
#ifdef SECTOR_FLAGS
    else if( gr_frontsector->flags & SCF_virtual_ceiling )
#else
    else if (gr_frontsector->virtualCeiling)
#endif
    {
        locCeilingHeight = gr_frontsector->virtualCeilingheight;
        locFloorHeight = gr_frontsector->floorheight;
    }
    else
    {
        locFloorHeight = gr_frontsector->floorheight;
        locCeilingHeight = gr_frontsector->ceilingheight;
    }
// ----- end special tricks -----

    if (gr_frontsector->ffloors)
    {
#ifdef SECTOR_FLAGS
        if( gr_frontsector->flags & SCF_floor_moved )
#else
        if (gr_frontsector->moved)
#endif
        {
            gr_frontsector->numlights = sub->sector->numlights = 0;
            R_Prep3DFloors(gr_frontsector);
            sub->sector->lightlist = gr_frontsector->lightlist;
            sub->sector->numlights = gr_frontsector->numlights;
#ifdef SECTOR_FLAGS
            gr_frontsector->flags &= ~(SCF_floor_moved);
            sub->sector->flags &= ~(SCF_floor_moved);
#else
            sub->sector->moved = gr_frontsector->moved = false;
#endif
        }

        // [WDJ] from r_bsp.c 4/22/2010
        // adapted to local vars, and may still need a little tuning
//      R_GetPlaneLight(gr_frontsector, gr_frontsector->floorheight);
        ff_light_t * ff_light = R_GetPlaneLight(gr_frontsector, locFloorHeight);
        if(gr_frontsector->floorlightsec < 0)
        {
          floorlightlevel = *ff_light->lightlevel;
          floorcolormap = ff_light->extra_colormap;
        }
//      R_GetPlaneLight(gr_frontsector, gr_frontsector->ceilingheight);
        ff_light = R_GetPlaneLight(gr_frontsector, locCeilingHeight);
        if(gr_frontsector->ceilinglightsec < 0)
        {
          ceilinglightlevel = *ff_light->lightlevel;
          ceilingcolormap = ff_light->extra_colormap;
        }
    }
   
    sub->sector->extra_colormap = gr_frontsector->extra_colormap;

    // Render floor.
    // yeah, easy backface cull! :)
    if (locFloorHeight < viewz)
    {
        if (gr_frontsector->floorpic != sky_flatnum)
        {
            if (sub->validcount != validcount)
            {
                HWR_GetFlat(levelflats[gr_frontsector->floorpic].lumpnum);
                HWR_RenderPlane(&poly_subsectors[num], locFloorHeight, PF_Occlude,
                                floorcolormap, floorlightlevel,
                                gr_frontsector->floorpic);
            }
        }
        else
        {
            // Sky as floor.
#ifdef POLYSKY
            HWR_RenderSkyPlane(&poly_subsectors[num], locFloorHeight);
#endif
            need_sky_background |= DSB_lower;
        }
    }

    if (locCeilingHeight > viewz)
    {
        if (gr_frontsector->ceilingpic != sky_flatnum)
        {
            if (sub->validcount != validcount)
            {
                HWR_GetFlat(levelflats[gr_frontsector->ceilingpic].lumpnum);
                HWR_RenderPlane(&poly_subsectors[num], locCeilingHeight, PF_Occlude,
                                ceilingcolormap, ceilinglightlevel,
                                gr_frontsector->ceilingpic);
            }
        }
        else
        {
            // Sky as ceiling.
#ifdef POLYSKY
            HWR_RenderSkyPlane(&poly_subsectors[num], locCeilingHeight);
#endif
            need_sky_background |= DSB_upper;
        }
    }

#ifdef R_FAKEFLOORS
    if (gr_frontsector->ffloors)
    {
        // TODO:fix light, xoffs, yoffs, extracolormap ?
        ffloor_t * fff;

        R_Prep3DFloors(gr_frontsector);
        for (fff = gr_frontsector->ffloors; fff; fff = fff->next)
        {
            if (!(fff->flags & (FF_OUTER_PLANES|FF_INNER_PLANES))
                || !(fff->flags & FF_EXISTS))
                continue;
            if (sub->validcount == validcount)
                continue;

            if (*fff->bottomheight <= gr_frontsector->ceilingheight
                && *fff->bottomheight >= gr_frontsector->floorheight
//                && ((viewz < *fff->bottomheight && (fff->flags & FF_OUTER_PLANES))
                && ((viewz <= *fff->bottomheight && (fff->flags & FF_OUTER_PLANES))
                    || (viewz > *fff->bottomheight && (fff->flags & FF_INNER_PLANES))))
            {
                if (fff->flags & (FF_TRANSLUCENT | FF_FOG))   // SoM: Flags are more efficient
                {
                    ff_light_t * ff_light =
                      R_GetPlaneLight_viewz(gr_frontsector, *fff->bottomheight);
                    HWR_Add3DWater(*fff->bottompic, &poly_subsectors[num],
                                   *fff->bottomheight, *ff_light->lightlevel,
                                   fff->alpha);
                }
                else
                {
                    HWR_GetFlat(levelflats[*fff->bottompic].lumpnum);
                    ff_light_t * ff_light =
                      R_GetPlaneLight_viewz(gr_frontsector, *fff->bottomheight);
                    HWR_RenderPlane(&poly_subsectors[num], *fff->bottomheight,
                                    PF_Occlude, NULL,
                                    *ff_light->lightlevel,
                                    *fff->bottompic);
                }
            }
            if (*fff->topheight >= gr_frontsector->floorheight
                && *fff->topheight <= gr_frontsector->ceilingheight
//                && ((viewz > *fff->topheight && (fff->flags & FF_OUTER_PLANES))
                && ((viewz >= *fff->topheight && (fff->flags & FF_OUTER_PLANES))
                    || (viewz < *fff->topheight && (fff->flags & FF_INNER_PLANES))))
            {
                if (fff->flags & (FF_TRANSLUCENT | FF_FOG))
                {
                    ff_light_t * ff_light =
                      R_GetPlaneLight_viewz(gr_frontsector, *fff->topheight);
                    HWR_Add3DWater(*fff->toppic, &poly_subsectors[num],
                                   *fff->topheight, *ff_light->lightlevel,
                                   fff->alpha);
                }
                else
                {
                    HWR_GetFlat(levelflats[*fff->toppic].lumpnum);
                    ff_light_t * ff_light =
                      R_GetPlaneLight_viewz(gr_frontsector, *fff->topheight);
                    HWR_RenderPlane(&poly_subsectors[num], *fff->topheight,
                                    PF_Occlude, NULL,
                                    *ff_light->lightlevel,
                                    *fff->toppic);
                }
            }

        }
    }
#endif

// Hurder ici se passe les choses intéressantes!
// on vient de tracer le sol et le plafond
// on trace à présent d'abord les sprites et ensuite les murs
// hurdler: faux: on ajoute seulement les sprites, le murs sont tracés d'abord
    if (lineseg != NULL)
    {
        // draw sprites first , coz they are clipped to the solidsegs of
        // subsectors more 'in front'
        HWR_AddSprites(gr_frontsector, tempsec.lightlevel);  // ???
//	HWR_AddSprites(gr_frontsector, gr_frontsector->lightlevel);  // ???

        //Hurdler: at this point validcount must be the same, but is not because
        //         gr_frontsector doesn't point anymore to sub->sector due to
        //         the call gr_frontsector = R_FakeFlat(...)
        //         if it's not done, the sprite is drawn more than once,
        //         what looks really bad with translucency or dynamic light,
        //         without talking about the overdraw of course.
        sub->sector->validcount = validcount;   //TODO: fix that in a better way

#ifdef DEBUG_TRIG       
        if( trigger_line < numlines
            && lineseg->linedef == & lines[ trigger_line ] )
        {
            GenPrintf(EMSG_debug, "TRIGGER LINE %i: side=%i\n", trigger_line, lineseg->side );
        }
#endif

        while (segcount--)
        {
            HWR_AddLine(lineseg);
            lineseg++;
        }
    }

#ifdef DCK_WATER_TEST
// DCK: an obsolete editor for DOS and Win95.
// It only allowed linedefs 0..255, which prevented creating water sectors.
//20/08/99: Changed by Hurdler (taken from faB's code)
    // -------------------- WATER IN DEV. TEST ------------------------
    //dck hack : use abs(tag) for waterheight
    if (gr_frontsector->tag < 0)
    {
        fixed_t wh;
        wh = ((-gr_frontsector->tag) << 16) + (FRACUNIT / 2);
        if (wh > gr_frontsector->floorheight && wh < gr_frontsector->ceilingheight)
        {
            HWR_GetFlat(doomwaterflat_pic.lumpnum);
            HWR_RenderPlane(&poly_subsectors[num], wh, PF_Translucent,
                            NULL, gr_frontsector->lightlevel, doomwaterflat_pic);
        }
    }
    // -------------------- WATER IN DEV. TEST ------------------------
#endif

    sub->validcount = validcount;
}

//
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.

#ifdef coolhack
//t;b;l;r
static fixed_t hackbbox[4];
//BOXTOP,
//BOXBOTTOM,
//BOXLEFT,
//BOXRIGHT
static boolean HWR_CheckHackBBox(fixed_t * bb)
{
    if (bb[BOXTOP] < hackbbox[BOXBOTTOM])       //y up
        return false;
    if (bb[BOXBOTTOM] > hackbbox[BOXTOP])
        return false;
    if (bb[BOXLEFT] > hackbbox[BOXRIGHT])
        return false;
    if (bb[BOXRIGHT] < hackbbox[BOXLEFT])
        return false;
    return true;
}
#endif

// BP: big hack for a test in lighning ref:1249753487AB
int * bsp_bbox;

#if 0
// [WDJ] Pull the degenerate case out of the main BSP loop.
// It can only be invoked at the first call.
// Can get same effect by calling with ( 0 | NF_SUBSECTOR ).
static void HWR_RenderBSPNode_degen(int bspnum)
{
    if (bspnum == -1)
    { 
        // [WDJ] Degenerate map with no nodes.
        //*(gr_drawsubsector_p++) = 0;
        HWR_Subsector(0);
    }
}
#endif

// Recursive walk through BSP tree.
// Called from HWR_RenderPlayerView.
static void HWR_RenderBSPNode(bsp_child_t bspnum)
{
    node_t * bsp;
    unsigned int  side;  // 0, 1

    // [WDJ] From EternityEngine, killough 5/2/98: remove tail recursion
    while( ! (bspnum & NF_SUBSECTOR) )
    {
        // not a subsector, a nodes
        bsp = &nodes[bspnum];

        // Decide which side the view point is on.
        side = R_PointOnSide(viewx, viewy, bsp);

        // BP: big hack for a test in lighning ref:1249753487AB
        bsp_bbox = bsp->bbox[side];

        // Recursively divide front space.
        HWR_RenderBSPNode(bsp->children[side]);

        // Possibly divide back space.
        side = side ^ 0x0001; // XOR bit0, other side
        if (! HWR_CheckBBox(bsp->bbox[side]))
            return;  // Not in back space

        // BP: big hack for a test in lighning ref:1249753487AB
        bsp_bbox = bsp->bbox[side];
        // [WDJ] Convert tail recursion to loop.
        // This does HWR_RenderBSPNode(bsp->children[side]);
        bspnum = bsp->children[side];
    }

    // Found a subsector
    {
        unsigned int  subsecnum = bspnum & (~NF_SUBSECTOR);  // subsector index
        if( subsecnum >= numsubsectors )  goto bad_subsector;
        //*(gr_drawsubsector_p++) = subsecnum;
        // HWR_Subsector->HWR_RenderPlane->HWR_PlaneLighting  which uses bsp_bbox.
        HWR_Subsector(subsecnum);
        return;

bad_subsector:
        // Error situations, should not get here.
#ifdef PARANOIA
        GenPrintf( EMSG_warn, "Bad BSP subsector: %i\n", subsecnum );
#else
        ;  // otherwise compile error
#endif
    }
    return;
}

/*
//
// Clear 'stack' of subsectors to draw
//
static void HWR_Clear_DrawSubsectors (void)
{
    gr_drawsubsector_p = gr_drawsubsectors;
}

//
// Draw subsectors pushed on the drawsubsectors 'stack', back to front
//
static void HWR_RenderSubsectors (void)
{
    while (gr_drawsubsector_p > gr_drawsubsectors)
    {
        HWR_RenderBSPNode (
        lastsubsec->nextsubsec = bspnum & (~NF_SUBSECTOR);
    }
}
*/

// ==========================================================================
//                                                              FROM R_MAIN.C
// ==========================================================================

#ifdef NO_MLOOK_EXTENDS_FOV
static angle_t anglefov = FIELDOFVIEW;
#endif

//BP : exactly the same as R_Init_TextureMapping
// Called from HWR_SetViewSize
static
void HWR_Init_TextureMapping( void )
{
    int i;
    int x;
    int t;
    fixed_t focallength;

    fixed_t grcenterx;
    fixed_t grcenterxfrac;
    int grviewwidth;

#ifdef NO_MLOOK_EXTENDS_FOV
    angle_t clipanglefov;
    static angle_t oldclipanglefov = 0;

    // anglefov initialized to FIELDOFVIEW
    clipanglefov = anglefov + 2 * abs((int) aimingangle);
    if (clipanglefov == oldclipanglefov)  // same as before
        return;
    oldclipanglefov = clipanglefov;
    int fov_angf = ANGLE_TO_FINE( clipanglefov );
    // [WDJ] ANGLE_1 has significant round-off error, but in this usage it does not matter.
    if (fov_angf >= ANGLE_TO_FINE(ANG180 - ANGLE_1))
        fov_angf = ANGLE_TO_FINE(ANG180 - ANGLE_1);

    CONS_Printf("HW_InitTextureMapping() %d %d %d\n",
                fov_angf, ANGLE_TO_FINE(aimingangle), ANGLE_TO_FINE(anglefov) );
#else
#define fov_angf  ANGLE_TO_FINE(FIELDOFVIEW)
#endif

    grviewwidth = vid.width;
    grcenterx = grviewwidth / 2;
    grcenterxfrac = grcenterx << FRACBITS;

    // Use tangent table to generate viewangle_to_x:
    //  viewangle_to_x will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
#ifdef FIT_RATIO
    //  so FIELDOFVIEW angles covers fit_width.
    focallength = FixedDiv( (int)(vid.fit_width/2) << FRACBITS, finetangent[(fov_angf/2) + FINE_ANG90]);
# ifdef DEBUG_FIT_RATIO
    GenPrintf(EMSG_debug, "HWR_TextureMapping: fit_width=%f  grcenterxfrac=%f\n", FIXED_TO_FLOAT((int)(vid.fit_width/2) << FRACBITS), FIXED_TO_FLOAT(grcenterxfrac));
# endif
#else   
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv(grcenterxfrac, finetangent[(fov_angf/2) + FINE_ANG90]);
#endif

    for (i = 0; i < FINE_ANG180; i++)
    {
        if (finetangent[i] > FRACUNIT * 2)
            t = -1;
        else if (finetangent[i] < -FRACUNIT * 2)
            t = grviewwidth + 1;
        else
        {
            t = FixedMul(finetangent[i], focallength);
            t = (grcenterxfrac - t + FRACUNIT - 1) >> FRACBITS;

            if (t < -1)
                t = -1;
            else if (t > grviewwidth + 1)
                t = grviewwidth + 1;
        }
        gr_viewangle_to_x[i] = t;
    }

    // Scan viewangle_to_x[] to generate x_to_viewangle[]:
    //  x_to_viewangle will give the smallest view angle
    //  that maps to x.
    for (x = 0; x <= grviewwidth; x++)
    {
        i = 0;
        while (gr_viewangle_to_x[i] > x)
            i++;
        gr_x_to_viewangle[x] = (i << ANGLETOFINESHIFT) - ANG90;
    }

    // Take out the fencepost cases from viewangle_to_x.
    for (i = 0; i < FINE_ANG180; i++)
    {
        if (gr_viewangle_to_x[i] == -1)
            gr_viewangle_to_x[i] = 0;
        else if (gr_viewangle_to_x[i] == grviewwidth + 1)
            gr_viewangle_to_x[i] = grviewwidth;
    }

    gr_clipangle = gr_x_to_viewangle[0];
    gr_clipangle_x_2 = gr_clipangle + gr_clipangle;
}

// ==========================================================================
// gr_things.c
// ==========================================================================

// sprites are drawn after all wall and planes are rendered, so that
// sprite translucency effects apply on the rendered view (instead of the background sky!!)

gr_vissprite_t gr_vissprites[MAXVISSPRITES];
gr_vissprite_t * gr_vissprite_p;

// --------------------------------------------------------------------------
// HWR_Clear_Sprites
// Called at frame start.
// --------------------------------------------------------------------------
static void HWR_Clear_Sprites(void)
{
    gr_vissprite_p = gr_vissprites;
}

// --------------------------------------------------------------------------
// HWR_NewVisSprite
// --------------------------------------------------------------------------
gr_vissprite_t gr_overflowsprite;

gr_vissprite_t * HWR_NewVisSprite(void)
{
    if (gr_vissprite_p == &gr_vissprites[MAXVISSPRITES])
        return &gr_overflowsprite;

    gr_vissprite_p++;
    return gr_vissprite_p - 1;
}

// -----------------+
// HWR_DrawSprite   : Draw flat sprites
//                  : (monsters, bonuses, weapons, lights, ...)
// Returns          :
// -----------------+
static void HWR_DrawSprite(gr_vissprite_t * spr)
{
    byte lightlum;  // 0..255
    vxtx3d_t vxtx[4];
    MipPatch_t * gpatch;       //sprite patch converted to hardware
    FSurfaceInfo_t Surf;

    // cache sprite graphics
    //12/12/99: Hurdler:
    //          OK, I don't change anything for MD2 support because I want to be
    //          sure to do it the right way. So actually, we keep normal sprite
    //          in memory and we add the md2 model if it exists for that sprite

    // convert sprite differently when fxtranslucent is detected
    Surf.polyflags = 0;
    Surf.texflags = 0;
    if ((spr->mobj->frame & FF_TRANSMASK) == (TRANSLU_fx1<<FF_TRANSSHIFT))
    {
        Surf.texflags = TF_Opaquetrans;
    }
    // get patch and draw to hardware cache
    gpatch = W_CacheMappedPatchNum(spr->patch_lumpnum, Surf.texflags );

    // dynamic lighting
    HWR_DL_AddLightSprite(spr, gpatch);

    // create the sprite billboard
    //
    //  3--2
    //  | /|
    //  |/ |
    //  0--1

    // fastest, use transform terms in optimized shared code
    // Combined transforms for look up/down and scaling
    float topty = spr->ty - gpatch->height;
    vxtx[0].x = vxtx[3].x = (spr->x1 * sprite_trans_x_to_x);
    vxtx[1].x = vxtx[2].x = (spr->x2 * sprite_trans_x_to_x);
    float tranzy = spr->tz * sprite_trans_z_to_y;
    vxtx[0].y = vxtx[1].y = (topty * sprite_trans_y_to_y) + tranzy;
    vxtx[2].y = vxtx[3].y = (spr->ty * sprite_trans_y_to_y) + tranzy;
    float tranzz = spr->tz * sprite_trans_z_to_z;
    vxtx[0].z = vxtx[1].z = (topty * sprite_trans_y_to_z) + tranzz;
    vxtx[2].z = vxtx[3].z = (spr->ty * sprite_trans_y_to_z) + tranzz;

    if (spr->flip)
    {
        vxtx[0].sow = vxtx[3].sow = gpatch->max_s;
        vxtx[2].sow = vxtx[1].sow = 0;
    }
    else
    {
        vxtx[0].sow = vxtx[3].sow = 0;
        vxtx[2].sow = vxtx[1].sow = gpatch->max_s;
    }
    vxtx[3].tow = vxtx[2].tow = 0;
    vxtx[0].tow = vxtx[1].tow = gpatch->max_t;

    // cache the patch in the graphics card memory
    //12/12/99: Hurdler: same comment as above (for md2)
    //Hurdler: 25/04/2000: now support colormap in hardware mode
    HWR_GetMappedPatch(gpatch, spr->colormap);

    // sprite (TODO: coloured-) lighting by modulating the RGB components
    // [WDJ] coloured seems to be done below
    lightlum = spr->sectorlight;
    Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = lightlum;

    //Hurdler: colormap test
    if (!fixedcolormap)
    {
        sector_t * sector = spr->mobj->subsector->sector;

        if (sector->ffloors)
        {
            ffloor_t * caster =
              R_GetPlaneLight(sector, spr->mobj->z)->caster;
            sector = caster ? &sectors[caster->model_secnum] : sector;
        }
        if (sector->extra_colormap || view_extracolormap)
        {
            Extracolormap_to_Surf( /*IN*/ sector->extra_colormap, lightlum,
                                   /*OUT*/ & Surf );
        }
    }

    //TODO: do the test earlier
    if (!cv_grmd2.value || (md2_models[spr->mobj->sprite].scale < 0.0f))
    {
        int blend = 0;
        if (spr->mobj->frame & FF_TRANSMASK)
            blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK) >> FF_TRANSSHIFT, &Surf);
        else if (spr->mobj->frame & FF_SMOKESHADE)
        {
            Surf.FlatColor.s.alpha = 0x80;
            blend = PF_Translucent;
        }
        else if (spr->mobj->flags & MF_SHADOW)
        {
            Surf.FlatColor.s.alpha = 0x40;
            blend = PF_Translucent;
        }
        else
        {
            // BP: i agree that is little better in environment but it don't
            //     work properly under glide nor with fogcolor to ffffff :(
            // Hurdler: PF_Environment would be cool, but we need to fix
            //          the issue with the fog before
            Surf.FlatColor.s.alpha = 0xFF;
            blend = PF_Translucent | PF_Occlude;
        }

        HWD.pfnDrawPolygon(&Surf, vxtx, 4, blend | PF_Modulated | PF_Clip);
    }

    // draw a corona if this sprite contain light(s)
#ifdef SPDR_CORONAS
#ifdef CORONA_CHOICE
    if( corona_draw_choice == 1 )
#else
    if( cv_corona.EV )
#endif
    {
        HWR_DoCoronasLighting(vxtx, spr);
    }
#endif
}

// --------------------------------------------------------------------------
// Sort vissprites by distance
// --------------------------------------------------------------------------
static gr_vissprite_t gr_vsprsortedhead;

static void HWR_SortVisSprites(void)
{
    int i;
    int count;
    gr_vissprite_t * ds;
    gr_vissprite_t * best = NULL;        //shut up compiler
    gr_vissprite_t unsorted;
    float bestdist;

    count = gr_vissprite_p - gr_vissprites;

    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
        return;

    for (ds = gr_vissprites; ds < gr_vissprite_p; ds++)
    {
        ds->next = ds + 1;
        ds->prev = ds - 1;
    }

    gr_vissprites[0].prev = &unsorted;
    unsorted.next = &gr_vissprites[0];
    (gr_vissprite_p - 1)->next = &unsorted;
    unsorted.prev = gr_vissprite_p - 1;

    // pull the vissprites out by scale
    gr_vsprsortedhead.next = gr_vsprsortedhead.prev = &gr_vsprsortedhead;
    for (i = 0; i < count; i++)
    {
        bestdist = SPRITE_NEAR_CLIP_DIST - 1;
        for (ds = unsorted.next; ds != &unsorted; ds = ds->next)
        {
            if (ds->tz > bestdist)
            {
                bestdist = ds->tz;
                best = ds;
            }
        }
        best->next->prev = best->prev;
        best->prev->next = best->next;
        best->next = &gr_vsprsortedhead;
        best->prev = gr_vsprsortedhead.prev;
        gr_vsprsortedhead.prev->next = best;
        gr_vsprsortedhead.prev = best;
    }
}


// --------------------------------------------------------------------------
//  Draw all MD2
// --------------------------------------------------------------------------
static void HWR_DrawMD2S(void)
{
    if (gr_vissprite_p > gr_vissprites)
    {
        gr_vissprite_t * spr;

        // draw all MD2 back to front
        for (spr = gr_vsprsortedhead.next; spr != &gr_vsprsortedhead; spr = spr->next)
        {
            HWR_DrawMD2(spr);
        }
    }
}

// --------------------------------------------------------------------------
// HWR_AddSprites
// During BSP traversal, this adds sprites by sector.
// --------------------------------------------------------------------------
static byte  sectorlight;  // Lum
// Does not need separate lightlevel as long as it is called with frontsector.
static void HWR_AddSprites(sector_t * sec, lightlev_t sprite_lightlevel)
{
    mobj_t * thing;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;

    // Well, now it will be done.
    sec->validcount = validcount;

    // sprite lighting
    if(!sec->numlights) // when numlights then light in DrawSprite
    {
      lightlev_t lightlevel = sprite_lightlevel;
      if(sec->model < SM_fluid)   lightlevel = sec->lightlevel;
      sectorlight = LightLevelToLum(lightlevel); // add extra light
    }

    // Handle all things in sector.
    for (thing = sec->thinglist; thing; thing = thing->snext)
    {
        if ((thing->flags2 & MF2_DONTDRAW) == 0)
            HWR_ProjectSprite(thing);
    }
}

// --------------------------------------------------------------------------
// HWR_ProjectSprite
//  Generates a vissprite for a thing if it might be visible.
// --------------------------------------------------------------------------
// BP why not use x_to_viewangle/viewangle_to_x like in bsp ?....
static void HWR_ProjectSprite(mobj_t * thing)
{
    gr_vissprite_t * vis;

    float tr_x, tr_y;
    float tx, tz;
    float tx1, tx2;  // edges
    float px1, px2;  // projected

    spritedef_t * sprdef;
    spriteframe_t * sprframe;
    sprite_frot_t * sprfrot;
    spritelump_t * sprlump;
    unsigned int rot, fr;
    angle_t ang;

    // Calculate view of sprite, relative to player, in world dimensions.
    // transform the origin point
    tr_x = FIXED_TO_FLOAT( thing->x ) - gr_viewx;  // relative position
    tr_y = FIXED_TO_FLOAT( thing->y ) - gr_viewy;

    // rotation around vertical axis
    tz = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);  // view depth

    // thing is behind view plane?
    if (tz < SPRITE_NEAR_CLIP_DIST)
        return;

    tx = (tr_x * gr_viewsin) - (tr_y * gr_viewcos);  // view x

    // decide which patch to use for sprite relative to player
    if ((unsigned) thing->sprite >= numsprites)
    {
#ifdef RANGECHECK
        I_SoftError("HWR_ProjectSprite: invalid sprite number %i \n", thing->sprite);
#endif
        return;
    }

    //Fab:02-08-98: 'skin' override spritedef currently used for skin
    if (thing->skin)
        sprdef = &((skin_t *) thing->skin)->spritedef;
    else
        sprdef = &sprites[thing->sprite];

    fr = thing->frame & FF_FRAMEMASK;
    if(fr >= sprdef->numframes)
    {
#ifdef RANGECHECK
        I_SoftError("HWR_ProjectSprite: invalid sprite frame %i : %i for %s\n",
                    thing->sprite, fr, sprnames[thing->sprite]);
#endif
        return;
    }
    sprframe = get_spriteframe( sprdef, fr );

    if( sprframe->rotation_pattern == SRP_1 )
    {
        // use single rotation for all views
        rot = 0;                        //Fab: for vis->patch below
    }
    else
    {
        // choose a different rotation based on player view
        ang = R_PointToAngle(thing->x, thing->y);       // uses viewx,viewy

        if( sprframe->rotation_pattern == SRP_8)
        {
            // 8 direction rotation pattern
            rot = (ang - thing->angle + (unsigned) (ANG45/2) * 9) >> 29;
        }
#ifdef ROT16
        else if( sprframe->rotation_pattern == SRP_16)
        {
            // 16 direction rotation pattern
            rot = (ang - thing->angle + (unsigned) (ANG45/4) * 17) >> 28;
        }
#endif
        else return;

    }

    sprfrot = get_framerotation( sprdef, fr, rot );
   
    //Fab: [WDJ] spritelump_id is the index
    sprlump = &spritelumps[sprfrot->spritelump_id];

    // The sprite patch is in world dimensions, offsets in world dimensions.
    // calculate edges of the shape
    tx1 = tx - FIXED_TO_FLOAT( sprlump->leftoffset );

    // project x
    px1 = gr_windowcenterx + (tx1 * gr_centerx / tz);

    // BP: FOV des sprites, c'est ici que sa ce passe
    // left edge off the right side?
#ifdef NO_MLOOK_EXTENDS_FOV
    if (px1 > gr_viewwidth)
#else
    if ((px1 > gr_viewwidth) && !cv_grmlook_extends_fov.value)
        //if ((px1 > gr_viewwidth) && (cv_grfov.value<=90) /*&& !cv_grmd2.value*/)
#endif
        return;

    tx2 = tx1 + FIXED_TO_FLOAT( sprlump->width );
    px2 = gr_windowcenterx + (tx2 * gr_centerx / tz);

    // BP: FOV des sprites, ici aussi
    // right edge off the left side
#ifdef NO_MLOOK_EXTENDS_FOV
    if (px2 < 0)
#else
    if ((px2 < 0) && !cv_grmlook_extends_fov.value)
        //if ((px2 < 0) && (cv_grfov.value<=90) /*&& !cv_grmd2.value*/)
#endif
        return;

    // sprite completely hidden ?
#ifdef NO_MLOOK_EXTENDS_FOV
    if (!HWR_ClipToSolidSegs((int) px1, (int) px2))
#else
    if ((!HWR_ClipToSolidSegs((int) px1, (int) px2)) && !cv_grmlook_extends_fov.value)
        //if ((!HWR_ClipToSolidSegs((int)px1,(int)px2)) && (cv_grfov.value<=90) /*&& !cv_grmd2.value*/)
#endif
        return;

#define FEET_IN_GROUND_TOPOFFSET_FIX   
#ifdef FEET_IN_GROUND_TOPOFFSET_FIX
    // [WDJ] Feet-in-ground adjustment moved here from r_things.
    // There it changed topoffset at lump load time, but did not adapt to changing drawmode.
    // spritelump is just patch header.
    //BP: we cannot use special trick in hardware mode because feet in ground caused by z-buffer
    // topoffset may be negative, use signed compare
    fixed_t fig_topoffset =
        ( (sprlump->topoffset > 0) && (sprlump->topoffset < sprlump->height) )? // not for psprite
           // perfect is patch.height but sometime it is too high
           min(sprlump->topoffset + (4<<FRACBITS), sprlump->height)
         : sprlump->topoffset;
#endif
   {
   // [WDJ] from r_things.c
   // This fixes the thing lighting in special sectors
    sector_t*		thingsector;	 // [WDJ] 11/14/2009
    int                 thingmodelsec;
    boolean	        thing_has_model;  // has a model, such as water
#ifdef FEET_IN_GROUND_TOPOFFSET_FIX
    fixed_t  gz_top = thing->z + fig_topoffset;  // Adjusted topoffset.
#else
    // Has feet-in-ground problem, due to z-buffer.
    fixed_t  gz_top = thing->z + sprlump->topoffset;
#endif
    thingsector = thing->subsector->sector;	 // [WDJ] 11/14/2009
    if(thingsector->numlights)
    {
      ff_light_t * ff_light = R_GetPlaneLight(thingsector, gz_top);
      lightlev_t lightlevel = *ff_light->lightlevel;
      if(ff_light->caster && (ff_light->caster->flags & FF_FOG))
        sectorlight = LightLevelToLum_extra(lightlevel, extralight_fog); // add extralight
      else
        sectorlight = LightLevelToLum_extra(lightlevel, 0);  // extralight=0
    }

    thingmodelsec = thingsector->modelsec;
    thing_has_model = thingsector->model > SM_fluid; // water

    if (thing_has_model)   // only clip things which are in special sectors
    {
      sector_t * thingmodsecp = & sectors[thingmodelsec];

      // [WDJ] Could use viewer_at_water to force view of objects above and
      // below to be seen simultaneously.
      // Instead have choosen to have objects underwater not be seen until
      // viewer_underwater.
      // When at viewer_at_water, will not see objects above nor below the water.
      // As this has some validity in reality, and does not generate HOM,
      // will live with it.  It is transient, and most players will not notice.
      if (viewer_has_model)
      {
          if( viewer_underwater ?
              (thing->z >= thingmodsecp->floorheight)
              : (gz_top < thingmodsecp->floorheight)
              )
              return;
          if( viewer_overceiling ?
              ((gz_top < thingmodsecp->ceilingheight) && (viewz > thingmodsecp->ceilingheight))
              : (thing->z >= thingmodsecp->ceilingheight)
              )
              return;
      }
    }
   }
#if 0
    // gr vis does not have a heightsec (yet ??)
    // [WDJ] Only pass water models, not colormap model sectors
    vis->modelsec = thing_has_model ? thingmodelsec : -1 ; //SoM: 3/17/2000
#endif
   
    //
    // store information in a vissprite
    //
    vis = HWR_NewVisSprite();
    vis->x1 = tx1;  // left of sprite in view, world coord
    vis->x2 = tx2;  // right of sprite in view, world coord
    vis->tz = tz;   // away depth
    vis->patch_lumpnum = sprfrot->pat_lumpnum;
    vis->flip = sprfrot->flip;
    vis->mobj = thing;

    //Hurdler: 25/04/2000: now support colormap in hardware mode
    if (thing->tflags & MFT_TRANSLATION6)
    {
        vis->colormap = MFT_TO_SKINMAP( thing->tflags ); // skins 1..
    }
    else
    {
        vis->colormap = & reg_colormaps[0];
    }

    // set top/bottom coords
#ifdef FEET_IN_GROUND_TOPOFFSET_FIX
    vis->ty = FIXED_TO_FLOAT( thing->z + fig_topoffset ) - gr_viewz;
#else
    vis->ty = FIXED_TO_FLOAT( thing->z + sprlump->topoffset ) - gr_viewz;
#endif

    //CONS_Printf("------------------\nH: sprite  : %d\nH: frame   : %x\nH: type    : %d\nH: sname   : %s\n\n",
    //            thing->sprite, thing->frame, thing->type, sprnames[thing->sprite]);

    if (thing->state->frame & FF_FULLBRIGHT)
    {
        // TODO: disable also the fog
        vis->sectorlight = 0xff;
    }
    else
    {
        vis->sectorlight = sectorlight;
    }
}

#define BASECENTER_Y           (BASEVIDHEIGHT/2)
// -----------------+
// HWR_DrawPSprite  : Draw 'player sprites' : weapons, etc.
//                  : fSectorLight ranges 0...1
// -----------------+
// Draw parts of the viewplayer weapon
void HWR_DrawPSprite(pspdef_t * psp,  byte lightlum)
{
    spritedef_t * sprdef;
//    spriteframe_t * sprframe;
    sprite_frot_t * sprfrot;
    spritelump_t * sprlump = NULL;

    vxtx3d_t vxtx[4];
    int i, fr;
    float tx, ty;
//    float x1, x2;

    MipPatch_t * gpatch;       //sprite patch converted to hardware

    FSurfaceInfo_t Surf;

    // [WDJ] 11/14/2012 use viewer variables, which will be for viewplayer

    // decide which patch to use
#ifdef RANGECHECK
    if ((unsigned) psp->state->sprite >= numsprites)
    {
        I_SoftError("HWR_ProjectSprite: invalid sprite number %i\n", psp->state->sprite);
        return;       
    }
#endif

    sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ((psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes)
    {
        I_SoftError("HWR_ProjectSprite: invalid sprite frame %i : %i\n", psp->state->sprite, psp->state->frame);
        return;
    }
#endif

    fr = psp->state->frame & FF_FRAMEMASK;
//    sprframe = get_spriteframe( sprdef, fr );

    // use single rotation for all views
    sprfrot = get_framerotation( sprdef, fr, 0 );
   
    sprlump = &spritelumps[sprfrot->spritelump_id];

    // calculate edges of the shape

    tx = FIXED_TO_FLOAT( (psp->sx - ((BASEVIDWIDTH / 2) << FRACBITS)) );
    tx -= FIXED_TO_FLOAT( sprlump->leftoffset );
//    x1 = gr_windowcenterx + (tx * gr_pspritescale_x);

    vxtx[3].x = vxtx[0].x = tx;

    tx += FIXED_TO_FLOAT( sprlump->width );
//    x2 = gr_windowcenterx + (tx * gr_pspritescale_x) - 1;

    vxtx[2].x = vxtx[1].x = tx;

    //  3--2
    //  | /|
    //  |/ |
    //  0--1
    vxtx[0].z = vxtx[1].z = vxtx[2].z = vxtx[3].z = 1;
//    vxtx[0].w = vxtx[1].w = vxtx[2].w = vxtx[3].w = 1;  // unused

    // cache sprite graphics
    gpatch = W_CachePatchNum(sprfrot->pat_lumpnum, PU_CACHE);
    HWR_GetPatch(gpatch);

    // set top/bottom coords
    ty = FIXED_TO_FLOAT( psp->sy - sprlump->topoffset );
    if (cv_splitscreen.value && (cv_grfov.value == 90))
        ty -= 20;       //Hurdler: so it's a bit higher
    if (EN_heretic_hexen)
    {
        if (rdraw_viewheight == vid.height
            || (!cv_scalestatusbar.value && vid.dupy > 1) )
            ty += FIXED_TO_FLOAT( PSpriteSY[viewplayer->readyweapon] );
    }

    vxtx[3].y = vxtx[2].y = (float) BASECENTER_Y - ty;

    ty += gpatch->height;
    vxtx[0].y = vxtx[1].y = (float) BASECENTER_Y - ty;

    if( sprfrot->flip )
    {
        vxtx[0].sow = vxtx[3].sow = gpatch->max_s;
        vxtx[2].sow = vxtx[1].sow = 0.0f;
    }
    else
    {
        vxtx[0].sow = vxtx[3].sow = 0.0f;
        vxtx[2].sow = vxtx[1].sow = gpatch->max_s;
    }
    vxtx[3].tow = vxtx[2].tow = 0.0f;
    vxtx[0].tow = vxtx[1].tow = gpatch->max_t;

    // project clipped vertices, [WDJ] can be done on one set of verts
    for (i = 0; i < 4; i++)
    {
        //Hurdler: sorry, I had to multiply all by 4 for correct splitscreen mode
        vxtx[i].x /= 40.0f;
        vxtx[i].y /= 25.0f;
        vxtx[i].z = 4.0f;
    }

    // clip 2d polygon to view window
    //wClipVerts = ClipToView (vxtx, outVerts, 4 );

    // set transparency and light level

    if (viewmobj->flags & MF_SHADOW)
    {
        if (viewplayer->powers[pw_invisibility] > 4 * TICRATE || viewplayer->powers[pw_invisibility] & 8)
            Surf.FlatColor.s.alpha = 0xff / 3;
        else
            Surf.FlatColor.s.alpha = 2 * 0xff / 3;
    }
    else
        Surf.FlatColor.s.alpha = 0xff;

    if (psp->state->frame & FF_FULLBRIGHT)
    {
        // TODO: remove fog for this sprite !
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
    }
    else
    {
        sector_t * sector = viewer_sector;

        // default opaque mode using alpha 0 for holes
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = lightlum;
        //Hurdler: colormap test

        if (!fixedcolormap)
        {
            if (sector->ffloors)
            {
                ffloor_t * caster = R_GetPlaneLight(sector, viewz)->caster;
                sector = caster ? &sectors[caster->model_secnum] : sector;
            }
            if (sector->extra_colormap || view_extracolormap)
            {
                Extracolormap_to_Surf( /*IN*/ sector->extra_colormap, lightlum,
                                       /*OUT*/ & Surf );
            }
        }
    }

    // invis player doesnt look good with PF_Environment so use PF_Translucent instead
    if (viewplayer->powers[pw_invisibility])
        HWD.pfnDrawPolygon(&Surf, vxtx, 4,
                           PF_Modulated | PF_Translucent | PF_NoDepthTest);
    else
        HWD.pfnDrawPolygon(&Surf, vxtx, 4,
                           PF_Modulated | PF_Environment | PF_NoDepthTest | PF_Occlude);
}

// --------------------------------------------------------------------------
// HWR_DrawPlayerSprites
// --------------------------------------------------------------------------
// Draw the viewplayer weapon
static void HWR_DrawPlayerSprites(void)
{
    int i;
    pspdef_t * psp;
    lightlev_t sll;
    byte lightlum;

    // [WDJ] 11/14/2012 use viewer variables for viewplayer

    if (viewer_sector->numlights)
    {
        ff_light_t * ff_light =
          R_GetPlaneLight(viewer_sector, viewmobj->z + viewmobj->info->height);
        sll = *ff_light->lightlevel;
    }
    else
    {
        // get light level
        sll = viewer_sector->lightlevel;
    }
    lightlum = LightLevelToLum(sll);

    // add all active psprites
    for (i = 0, psp = viewplayer->psprites; i < NUMPSPRITES; i++, psp++)
    {
        if (psp->state)
            HWR_DrawPSprite(psp, lightlum);
    }
}

// ==========================================================================
//
// ==========================================================================
//  upper_lower : DSB_e
void HWR_DrawSkyBackground(player_t * player, byte upper_lower)
{
    vxtx3d_t v[4];
    angle_t angle;

//  3--2
//  | /|
//  |/ |
//  0--1

#ifdef PARANOIA
    if( sky_mipmap.GR_data == NULL )
    {
        printf( "sky_mipmap missing\n" );
        HWR_sky_mipmap();
        if( sky_mipmap.GR_data == NULL )
            return;
    }
#endif


    // ANGLE_MAX = 0xffffffff
    // ANGLE_MAX/4 = 0x3fffffff
    // ANG90 = 0x40000000
    // Sky width 256, covers 1/4 the sky.
    // Sky width 1024, covers all the sky.
#define WRAPANGLE (ANGLE_MAX/4)
    // sky_width is power of 2, so wrapangle is always a mask
    // sky_width = 1024  ==>  wrapangle = 0xFFFFFFFF
    // The shift does overflow, but lower bits still give correct result.
    angle_t wrapangle = (sky_mipmap.width << 22) - 1;
    angle = dup_viewangle + gr_x_to_viewangle[0] - ANG90;

    // OpenGL does not really need us to modulo the angle, but we can.
    angle &= wrapangle; // modulo by the angle of the sky size (it is always a mask)
   
    v[2].sow = v[1].sow = ((float) angle) / wrapangle;  // X high
    v[0].sow = v[3].sow = v[1].sow + (256.0f / sky_mipmap.width) ;  // X low

    // View angle effect on screen.
    // -1.0 when looking straight up.
    // 0 when look at horizon.
    float vpf = FIXED_TO_FLOAT(
        finetangent[(FINE_ANG90 - ((int) aimingangle >> (ANGLETOFINESHIFT + 1))) & FINEMASK] );
        // finetangent_ANG( -(aimingangle/2) )

    // At vpf= 0, top of screen is top of 127 high texture.   
    // With 240 high sky, player can look directly at top of 127 high sky.
    // Expanding to 240 high sky adds 100 pixels to top of sky, and 12 pixels to bottom.
    // The view of the sky texture is 200 pixels high.
    // The tow[] are the texture positions for each corner of the view area, as percentage of texture width, height.
    // The tow[0], tow[1] are at the bottom of the screen, well below the bottom of the displayed texture.
    // If tow[3] > tow[0] the sky will be inverted.

    // The sky should stay stationary against window edges when the head is moved.
    // The previous  40 + ( 200 * vpf ) made the sky slew too fast.
//    float f = 40 + (200 * vpf);
    // This value keeps the sky (mostly) still relative to walls when the head is turned.
    float vpl = vpf * 336.0f; // scale movement to texture line.
    float tte;  // top texture line

    unsigned int sky_height = sky_mipmap.height;
    // Original was 320x200, non-square pixels.
    // Doom2 sky texture is 256w x 128h.
    // TNT sky texture is 1024w x 128h.
    // Heretic and Hexen sky texture are 256w x 200h.
    // Expanded texture for free-look is 256w x 240h.
    // When view is at horizon, draw sky texture [40] as top.
    // When aiming angle > 0x10480000, then sky repeats at top.
    //   horizon = -0.2017
    // When aiming angle > 0x38c00000, then sky repeats at top again.
    //   horizon = -0.83

    // [WDJ] The special values are determined by experiment.
    // Attempts to calculate these using logic have generated distorted skies.

    if( cv_sky_gen.EV == 248 )  // Vanilla
    {
        // Display the sky texture, at normal vertical scale.
        tte = vpl;
       
        if( sky_height > 128 )
           tte += 100.0f;  // view offset for larger skies
       
#if 0
// Reference:
//        v[3].tow = v[2].tow = (336.0f / 128.0f) * vpf;
//        v[0].tow = v[1].tow = v[3].tow + (200.0f / 128.0f);
        sky_tow_top = ( sky_mipmap.height == 128 )?
              (vpl / 128.0f)
            : ((vpl + 100.0f) / sky_mipmap.height);
        sky_tow_bottom = sky_tow_top + (200.0f / sky_mipmap.height);

#endif
    }
    else
    {
        // Generated textures (240 high), and stretch.
        // Even though the sky may be 240, the pixels/angle is the same as sky 128.
        // OpenGL uses percentages, so can pretend it is 240 high and it stretches to fit.
        if( (cv_sky_gen.EV == 250) && (sky_height < 240) )  // stretch
            sky_height = 240;  // Stretch the 128 high texture to cover 240 pixels.

        tte = vpl + 100.0f;
        // top edge at vpf=-0.299
    }

    float top_edge = 4.0f;
    float bottom_edge = -4.0f;
    // bottom texture edge (before trimming top)
    float bte = tte + 200.0f;
   
    if( tte < 0 )  // top texture limit
    {
        top_edge = 4.0f + (tte * 0.040f);   // in display position
        tte = 0.4;  // to prevent texture wrap-around
    }

    if( bte > sky_height )  // bottom texture limit
    {
        bottom_edge = -4.0f + ( (bte - sky_height) * 0.040f);  // in display position
        bte = sky_height - 0.4;  // to prevent texture wrap-around
    }

    int visible = bte - tte;  // sky texture is visible

    if( bte < 0 )  // bottom is above top of sky texture
    {
        bte = 0;
        visible = -1;
    }
       
   
#if 0
    // FIXME: This does not handle free-look.
    // FIXME: This does not handle f hitting the limits.
    switch( upper_lower )
    {
     case DSB_upper: // sky as ceiling
        v[0].y = v[1].y = 8.0f * anglef;
//        v[0].tow = v[1].tow = (f + 64) / 127.0f;
        break;
     case DSB_lower: // sky as floor
        v[2].y = v[3].y = 0.0f;
        v[3].tow = v[2].tow = (f - 64) / 127.0f;
        break;
     default:
        break;
    }
#endif

   
    //Hurdler: the sky is the only texture who need 4.0f instead of 1.0
    //         because it's called just after clearing the screen
    //         and thus, the near clipping plane is set to 3.99

    v[0].x = v[3].x = -4.0f;
    v[1].x = v[2].x = 4.0f;

    v[0].z = v[1].z = v[2].z = v[3].z = 4.0f;


    // Setup in R_Setup_SkyDraw(), sky_mipmap, skytop_mipmap, ground_mipmap.
    if( top_edge < 4.0f )
    {
        // Skytop: Over top of sky texture
        v[2].y = v[3].y = 4.0f;
        v[0].y = v[1].y = top_edge - 0.1;
        v[3].tow = v[2].tow = 0;
        v[0].tow = v[1].tow = 1.0;
        
        HWD.pfnSetTexture( &skytop_mipmap );
        HWD.pfnDrawPolygon(NULL, v, 4, 0);
    }
    if( bottom_edge > -4.0f )
    {
        // Ground: Below bottom of sky texture
        v[2].y = v[3].y = bottom_edge + 0.1;
        v[0].y = v[1].y = -4.0f;
        v[3].tow = v[2].tow = 0;
        v[0].tow = v[1].tow = 1.0;
        
        HWD.pfnSetTexture( &ground_mipmap );
        HWD.pfnDrawPolygon(NULL, v, 4, 0);
    }

    // When normal sky is visible
    if( visible > 0 )
    {
        v[0].y = v[1].y = bottom_edge;
        v[2].y = v[3].y = top_edge;
        v[3].tow = v[2].tow = tte / sky_height;
        v[0].tow = v[1].tow = bte / sky_height;
   
        // drawflags = 0
        HWD.pfnSetTexture( &sky_mipmap );
        HWD.pfnDrawPolygon(NULL, v, 4, 0);
    }
}


// -----------------+
// HWR_Clear_View : clear the viewwindow, with maximum z value
// -----------------+
void HWR_Clear_View(void)
{
    //  3--2
    //  | /|
    //  |/ |
    //  0--1

    //FIXTHIS faB - enable depth mask, disable color mask

    HWD.pfnGClipRect((int) gr_viewwindowx, (int) gr_viewwindowy, (int) (gr_viewwindowx + gr_viewwidth), (int) (gr_viewwindowy + gr_viewheight), 3.99f);
    HWD.pfnClearBuffer(false, true, 0);

    //disable clip window - set to full size
    // rem by Hurdler
    // HWD.pfnGClipRect (0,0,vid.width,vid.height );
}

static byte  viewsv_viewnumber;

// -----------------+
// HWR_SetViewSize  : set projection and scaling values depending on the
//                  : view window size
// -----------------+
void HWR_SetViewSize( int viewsize )
{
    // setup view size

    // clamping viewsize is normally not needed coz it's done in R_ExecuteSetViewSize()
    // BEFORE calling here
    if (viewsize < 3 || viewsize > 12)
        viewsize = 10;
    if (viewsize > 10 || (viewsize == 10 && (!cv_scalestatusbar.value || cv_grtranslucenthud.value < 255)))
    {
        gr_viewwidth = (float) vid.width;
        gr_viewheight = (float) vid.height;
    }
    else
    {
        // always a multiple of eight
        gr_viewwidth = (float) ((viewsize * vid.width / 10) & ~7);
        gr_viewheight = (float) ((viewsize * (vid.height - stbar_height / 2) / 10) & ~1);
    }

    if (cv_splitscreen.value)
        gr_viewheight /= 2;

    gr_centerx = gr_viewwidth / 2;
    gr_basecentery = gr_viewheight / 2; //note: this is (gr_centerx * gr_viewheight / gr_viewwidth)

    gr_viewwindowx = (vid.width - gr_viewwidth) / 2;
    gr_windowcenterx = (float) (vid.width / 2);
    if (gr_viewwidth == vid.width)
    {
        // window top left corner at 0,0
        gr_baseviewwindowy = 0;
        gr_basewindowcentery = gr_viewheight / 2;
    }
    else
    {
        gr_baseviewwindowy = (vid.height - stbar_height - gr_viewheight) / 2;
        gr_basewindowcentery = (float) ((vid.height - stbar_height) / 2);
    }

#ifdef FIT_RATIO
    gr_pspritescale_x = ((float)vid.fit_width) / BASEVIDWIDTH;
    gr_pspritescale_y = ((float)vid.fit_height) / BASEVIDHEIGHT;
#else
    gr_pspritescale_x = gr_viewwidth / BASEVIDWIDTH;
    gr_pspritescale_y = ((vid.height * gr_pspritescale_x * BASEVIDWIDTH) / BASEVIDHEIGHT) / vid.width;
#endif

    HWR_Init_TextureMapping ();
   
    viewsv_viewnumber = 255;  // force init of render
}


//Hurdler: 3D water stuffs
static int numplanes = 0;
static int num_late_walls = 0;  // drawn late, transparent walls


// ==========================================================================
//
// ==========================================================================
// Split player saved settings.
// indexed from viewnumber, when 0,1.
static byte viewsv_need_sky[2];


//  viewnumber : splitscreen 0=upper, 1=lower. Single player is always 0.
//    
//  pind : splitscreen 0=upper, 1=lower, 2=init. Single player is always 0.
void HWR_RenderPlayerView(byte pind, player_t * player)
{
    // viewnumber = pind, because [0] is upper splitscreen
    //static float    distance = BASEVIDWIDTH;

    // Palette moved to R_SetupFrame.

    // Is also forced upon first Render, by init of viewsv_viewnumber.
    if(viewsv_viewnumber != pind)
    {
        if( viewsv_viewnumber < 2 )
        {
            // swap split window settings
            viewsv_need_sky[viewsv_viewnumber] = need_sky_background;
            need_sky_background = viewsv_need_sky[pind];
        }
        else
        {
            // Initial values.
            need_sky_background = DSB_all;
            viewsv_need_sky[0] = viewsv_need_sky[1] = DSB_all;
        }
        viewsv_viewnumber = pind;
        HWR_Set_Lights(pind);
    }
     
    // note: sets viewangle, viewx, viewy, viewz
    R_SetupFrame(pind, player);

    // copy view cam position for local use
    dup_viewangle = viewangle;

    // set window position
    gr_centery = gr_basecentery;
    gr_viewwindowy = gr_baseviewwindowy;
    gr_windowcentery = gr_basewindowcentery;
    if (cv_splitscreen.value && (pind == 1))
    {
        // lower screen
        //gr_centery += (vid.height/2 );
        gr_viewwindowy += (vid.height / 2);
        gr_windowcentery += (vid.height / 2);
    }

    // hmm solidsegs probably useless here
    //R_Clear_DrawSegs ( );
    // useless
    //R_Clear_Planes (player );
    //HWR_Clear_Sprites ( );

    // check for new console commands.
    NetUpdate();

    gr_viewx = FIXED_TO_FLOAT( viewx );
    gr_viewy = FIXED_TO_FLOAT( viewy );
    gr_viewz = FIXED_TO_FLOAT( viewz );
    gr_viewsin = FIXED_TO_FLOAT( viewsin );
    gr_viewcos = FIXED_TO_FLOAT( viewcos );

    // viewludsin( aimingangle ) instead of viewsin( viewangle )
    gr_viewludsin = FIXED_TO_FLOAT(cosine_ANG(aimingangle));
    gr_viewludcos = FIXED_TO_FLOAT(-sine_ANG(aimingangle));

    //04/01/2000: Hurdler: added for T&L
    //                     It should replace all other gr_viewxxx when finished
    atransform.anglex = (float) ANGLE_TO_FINE(aimingangle) * (360.0f / (float) FINEANGLES);
    atransform.angley = (float) ANGLE_TO_FINE(viewangle) * (360.0f / (float) FINEANGLES);
    atransform.x = gr_viewx;    // FIXED_TO_FLOAT( viewx )
    atransform.y = gr_viewy;    // FIXED_TO_FLOAT( viewy )
    atransform.z = gr_viewz;    // FIXED_TO_FLOAT( viewz )
#ifdef FIT_RATIO   
    atransform.scalex = (float)vid.fit_width / (float)vid.width;
    atransform.scaley = ORIGINAL_ASPECT * ((float) vid.fit_height) / (float) vid.height;
#else
    atransform.scalex = 1;
    atransform.scaley = ORIGINAL_ASPECT;
#endif
    atransform.scalez = 1;
    atransform.fovxangle = cv_grfov.value;
    atransform.fovyangle = cv_grfov.value;
    atransform.splitscreen = cv_splitscreen.value;
    gr_fovlud = 1 / tan(cv_grfov.value * PI / 360);

#ifdef NO_MLOOK_EXTENDS_FOV
    // enlarge fov when looking up/down
    HWR_Init_TextureMapping();
#endif

    //------------------------------------------------------------------------
    HWR_Clear_View();

    if (cv_grfog.value)
        HWR_FoggingOn();

    // Needs to be drawn early.  Is drawn with different transform.
    // Translucents are drawn over it.
    // Enable when detected sky sectors in previous frame draw.
    if (need_sky_background)
        HWR_DrawSkyBackground(player, need_sky_background);

    need_sky_background = DSB_none;

    // added by Hurdler for FOV 120
//    if (cv_grfov.value != 90)
//        HWD.pfnSetSpecialState(HWD_SET_FOV, cv_grfov.value);

    //14/11/99: Hurdler: we will add lights while processing sprites
    //it doesn't work with all subsectors (if we use AddSprites to do that).
    //TOO bad, that's why I removed this line (until this is fixed).
//    HWR_Reset_Lights();

    HWR_Clear_Sprites();

    HWR_Clear_ClipSegs();

    //04/01/2000: Hurdler: added for T&L
    //                     Actually it only works on Walls and Planes
    HWD.pfnSetTransform(&atransform);
    // [WDJ] transform upgrade
    HWR_set_view_transform();

    validcount++;

    // [WDJ] Intercept degenerate case, so BSP node is never -1.
    bsp_child_t top_node = ( numnodes > 0 )? numnodes-1
       : ( 0 | NF_SUBSECTOR );  // Degenerate, sector 0

    HWR_RenderBSPNode( top_node );

#ifndef NO_MLOOK_EXTENDS_FOV
    if (cv_grmlook_extends_fov.value && (aimingangle || cv_grfov.value > 90))
    {
        dup_viewangle += ANG90;
        HWR_Clear_ClipSegs();
        HWR_RenderBSPNode( top_node );        //left

        dup_viewangle += ANG90;
        if (cv_grmlook_extends_fov.value == 2 && ((int) aimingangle > ANG45 || (int) aimingangle < -ANG45))
        {
            HWR_Clear_ClipSegs();
            HWR_RenderBSPNode( top_node );    //back
        }

        dup_viewangle += ANG90;
        HWR_Clear_ClipSegs();
        HWR_RenderBSPNode( top_node );        //right

        dup_viewangle += ANG90;
    }
#endif

    // Check for new console commands.
    NetUpdate();

    //14/11/99: Hurdler: moved here because it doesn't work with
    // subsector, see other comments;
    HWR_Reset_Lights();

    // Draw MD2 and sprites
    HWR_SortVisSprites();
    HWR_DrawMD2S();
#ifdef CORONA_CHOICE
    // mirror corona choice, with auto -> sprite draw
    corona_draw_choice = (cv_corona.EV==0)? 0 : ((cv_grcorona_draw.value == 3)?  1 : cv_grcorona_draw.value);
#endif     
    HWR_RenderSorted();
    HWD.pfnSetTransform(NULL);

    // Check for new console commands.
    NetUpdate();

    if( view_fogfloor
        && ( view_fogfloor->flags & FF_FOGFACE  ) )
    {
        int fog_extralight = (player->extralight * view_fogfloor->alpha) >> 7;
        // fog drawn over eyes, using sprite transforms
        if( extralight )
           HWR_RenderFog( view_fogfloor, viewer_sector, fog_extralight, 4.0f );
        HWR_RenderFog( view_fogfloor, viewer_sector, fog_extralight, 0.0 );  // random scale
    }

    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!viewangleoffset && !camera.chase && cv_psprites.value && script_camera_on == false)
        HWR_DrawPlayerSprites();

    //------------------------------------------------------------------------
    // put it off for menus etc
    if (cv_grfog.value)
        HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 0);

    // added by Hurdler for correct splitscreen
    // moved here by hurdler so it works with the new near clipping plane
    HWD.pfnGClipRect(0, 0, vid.width, vid.height, NEAR_CLIP_DIST);
}

// ==========================================================================
//                                                                        FOG
// ==========================================================================

static
unsigned int hex_val(const char *str)
{
    unsigned int val = 0;
    int i, d;
    const char * sc;
    char c;

    sc = str;
    for(i = 0; i < 6; i++)
    {
        c = *(sc++);
        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (c >= 'a' && c <= 'f')
            d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            d = c - 'A' + 10;
        else
            break;
        val = (val << 4) | d;
    }
    //CONS_Printf ("col %x\n", val);
    return val;
}

void HWR_FoggingOn(void)
{
    HWD.pfnSetSpecialState(HWD_SET_FOG_COLOR, hex_val(cv_grfogcolor.string));
    HWD.pfnSetSpecialState(HWD_SET_FOG_DENSITY, cv_grfogdensity.value);
    HWD.pfnSetSpecialState(HWD_SET_FOG_MODE, 1);
}

// ==========================================================================
//                                                         3D ENGINE COMMANDS
// ==========================================================================

static void CV_grFov_OnChange(void)
{
    // autoset mlook when FOV > 90
    if ((!cv_grmlook_extends_fov.value) && (cv_grfov.value > 90))
        CV_SetValue(&cv_grmlook_extends_fov, 1);
}

static void CV_grPolygonSmooth_OnChange(void)
{
    if( HWD.pfnSetSpecialState )
        HWD.pfnSetSpecialState(HWD_SET_POLYGON_SMOOTH, cv_grpolygonsmooth.value);
}

/*
static void CV_grFogColor_OnChange (void)
{
    //HWD.pfnSetSpecialState (HWD_SET_FOG_COLOR, hex_val(cv_grfogcolor.string));
}
*/
static void Command_GrStats_f(void)
{
    //debug
    Z_CheckHeap(9875);

    CONS_Printf("Patch info headers : %7d kb\n", Z_TagUsage(PU_HWRPATCHINFO) >> 10);
    CONS_Printf("3D Texture cache   : %7d kb\n", Z_TagUsage(PU_HWRCACHE) >> 10);
    CONS_Printf("Plane polygon      : %7d kb\n", Z_TagUsage(PU_HWRPLANE) >> 10);
}

// **************************************************************************
//                                                            3D ENGINE SETUP
// **************************************************************************

// --------------------------------------------------------------------------
// Register hardware engine commands & consvars
// --------------------------------------------------------------------------
//added by Hurdler: console variable that are saved
void HWR_Register_Gr1Commands(void)
{
    // [WDJ] Any cv_ with CV_SAVE needs to be registered, even if it is not used.
    // Otherwise there will be error messages when config is loaded.
    CV_RegisterVar(&cv_grgammablue);
    CV_RegisterVar(&cv_grgammagreen);
    CV_RegisterVar(&cv_grgammared);
    //CV_RegisterVar (&cv_grcontrast);
    //CV_RegisterVar (&cv_grpolygonsmooth); // moved below
    CV_RegisterVar(&cv_grmd2);
    CV_RegisterVar(&cv_grstaticlighting);
    CV_RegisterVar(&cv_grdynamiclighting);
    CV_RegisterVar(&cv_grcorona_draw);
    CV_RegisterVar(&cv_grfov);
    CV_RegisterVar(&cv_grfogdensity);
    CV_RegisterVar(&cv_grfogcolor);
    CV_RegisterVar(&cv_grfog);
    CV_RegisterVar(&cv_grmlook_extends_fov);
    CV_RegisterVar(&cv_grfiltermode);
    CV_RegisterVar(&cv_grcorrecttricks);
    CV_RegisterVar(&cv_grsolvetjoin);
    CV_RegisterVar(&cv_grpolytile);
    CV_RegisterVar(&cv_grpolyshape);
}

// HWR Engine state and modes.
void HWR_Register_Gr2Commands(void)
{
    CV_RegisterVar(&cv_grpolygonsmooth);

    // engine state variables
    //CV_RegisterVar (&cv_grsky);
    //CV_RegisterVar (&cv_grzbuffer);
    //CV_RegisterVar (&cv_grclipwalls);
    CV_RegisterVar(&cv_grrounddown);

    // engine development mode variables
    // - usage may vary from version to version..
    CV_RegisterVar(&cv_gralpha);
    CV_RegisterVar(&cv_grbeta);
    CV_RegisterVar(&cv_grgamma);
    CV_RegisterVar(&cv_grzbuffer);
#ifdef TRANSWALL_CHOICE
    CV_RegisterVar(&cv_grtranswall);
#endif
    CV_RegisterVar(&cv_grclipwalls);

    // engine commands
    COM_AddCommand("gr_stats", Command_GrStats_f, CC_info);
}

// --------------------------------------------------------------------------
// Setup the hardware renderer
// --------------------------------------------------------------------------
void HWR_Startup_Render(void)
{
    static int startupdone = 0;

    CONS_Printf("HWR_Startup()\n");

    // initialize light lut translation
    Init_LumLut();

    // do this once
    if (!startupdone)
    {
        // add console cmds & vars
        HWR_Register_Gr2Commands();

        HWR_Init_PolyPool();
        HWR_Init_TextureCache();

#ifdef DCK_WATER_TEST
        // for test water translucent surface
        doomwaterflat = W_CheckNumForName("FWATER1");
        if( ! VALID_LUMP(doomwaterflat) )        // if FWATER1 not found (in doom shareware)
            doomwaterflat = W_GetNumForName("WATER0");
#endif

        HWR_Init_MD2();
    }

    HWR_Init_Light();

    if (rendermode == render_opengl)
        textureformat = patchformat = GR_RGBA;

    // The hardware draw modes that can use the flash palette call.
    EN_HWR_flashpalette = (rendermode == render_opengl) || (rendermode == render_d3d);

    startupdone = 1;
}


// Called after setup level, and when change drawmode to HWR draw.
void  HWR_SetupLevel( void )
{
    // Setup structures needed for HWR draw.
    // BP: reset light between levels (we draw preview frame lights on current frame)
    HWR_Reset_Lights();
    // Correct missing sidedefs & deep water trick
    HWR_CorrectSWTricks();
    HWR_Create_PlanePolygons();
}

// Called after setup level, and when change drawmode to HWR draw.
void  HWR_Preload_Graphics( void )
{
    HWR_Prep_LevelCache (numtextures);
    HWR_Create_StaticLightmaps();
}

// --------------------------------------------------------------------------
// Free resources allocated by the hardware renderer
// --------------------------------------------------------------------------
void HWR_Shutdown_Render(void)
{
    CONS_Printf("HWR_Shutdown()\n");
    HWR_Free_PolyPool();
    HWR_Free_TextureCache();
}


// temporary, to supply old call
void transform_world_to_gr(float wx, float wy, float wz, /*OUT*/ float * gx, float * gy, float * gz )
{
    // translation
    // Combined transforms for position, direction, look up/down, and scaling
    float tr_x = wx - gr_viewx;
    float tr_y = wz - gr_viewy;
    float tr_z = wy - gr_viewz;
    *gx = (tr_x * world_trans_x_to_x)
       + (tr_y * world_trans_y_to_x);
    *gy = (tr_x * world_trans_x_to_y )
       + (tr_y * world_trans_y_to_y )
       + (tr_z * world_trans_z_to_y );
    *gz = (tr_x * world_trans_x_to_z )
       + (tr_y * world_trans_y_to_z )
       + (tr_z * world_trans_z_to_z );
}

#if 0
// for reference
void transform_world_to_gr(float * cx, float * cy, float * cz)
{
    float tr_x, tr_y;
    // translation
    tr_x = *cx - gr_viewx;
    tr_y = *cz - gr_viewy;
//   *cy = *cy;

    // rotation around vertical y axis
    *cx = (tr_x * gr_viewsin) - (tr_y * gr_viewcos);
    tr_x = (tr_x * gr_viewcos) + (tr_y * gr_viewsin);

    //look up/down
    tr_y = *cy - gr_viewz;

    *cy = (tr_x * gr_viewludcos) + (tr_y * gr_viewludsin);
    *cz = (tr_x * gr_viewludsin) - (tr_y * gr_viewludcos);

    //scale y before frustum so that frustum can be scaled to screen height
    *cy *= ORIGINAL_ASPECT * gr_fovlud;
    *cx *= gr_fovlud;
}
#endif

//Hurdler: 3D Water stuff
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define PLANEINFO_INC 512
static planeinfo_t * planeinfo = NULL;
static int planeinfo_len = 0;  // num allocated


// Add translucent plane, called for each plane visible
//  picnum : index to levelflats
void HWR_Add3DWater(int picnum, poly_subsector_t * xsub, fixed_t fixedheight, lightlev_t lightlevel, int alpha)
{
    planeinfo_t * pl;

    if (numplanes >= planeinfo_len)
    {
        // expand number of planeinfo
        size_t planeinfo_req = planeinfo_len + PLANEINFO_INC;
        pl = (planeinfo_t *) realloc(planeinfo, planeinfo_req * sizeof(planeinfo_t));
        if( pl == NULL )
        {
            // Missing some planes for a while, but player can finish level.
            I_SoftError( "Planeinfo: cannot alloc %i\n", planeinfo_req );
            return;
        }
        planeinfo = pl;
        planeinfo_len = planeinfo_req;
    }

    // [WDJ] Insert sort is faster than bubble-sort or quicksort, because
    // the tests can be made simpler, takes advantage of already sorted list,
    // and it moves all closer entries at once, and only once.
    planeinfo_t * plnew = & planeinfo[numplanes];
    // insert sort farthest to closest
    fixed_t dist_abs = ABS(viewz - fixedheight);
    fixed_t dist_min = viewz - dist_abs;  // test
    fixed_t dist_max = viewz + dist_abs;  // test
    for( pl = & planeinfo[0]; pl < plnew; pl++ )
    {
        // test for plane closer
        if( pl->fixedheight > dist_min && pl->fixedheight < dist_max )
           break; // entry is closer than new plane
    }
    if( pl < plnew )
    {
        // move all closer entries at once
        memmove( pl+1, pl, (byte*)plnew-(byte*)pl );
        plnew = pl;
    }

    // The new water plane
    plnew->fixedheight = fixedheight;
    plnew->lightlevel = lightlevel;
    plnew->picnum = picnum;
    plnew->xsub = xsub;
    plnew->alpha = alpha;

    numplanes++;
}


void HWR_Render3DWater()
{
    int i;

    //[WDJ] Do insert sort during insert of plane, then fewest operations,
    // one calc of dist, and one memcpy

    gr_frontsector = NULL;      //Hurdler: gr_frontsector is no longer valid
    for (i = 0; i < numplanes; i++)
    {
        // pass alpha to HWR_RenderPlane
        FBITFIELD PolyFlags = PF_Translucent | (planeinfo[i].alpha << 24);

        HWR_GetFlat(levelflats[planeinfo[i].picnum].lumpnum);
        HWR_RenderPlane(planeinfo[i].xsub, planeinfo[i].fixedheight, PolyFlags,
                        NULL, planeinfo[i].lightlevel, planeinfo[i].picnum);
    }
    numplanes = 0;
}

//Hurdler: manage transparent texture a little better
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define LATE_WALLINFO_INC  128

typedef struct
{
    vxtx3d_t vxtx[4];
    FSurfaceInfo_t Surf;
    int texnum;
    int blend;
    int next_nearer;  // -1 end of list
    float dist1, dist2;
} late_wallinfo_t;

static late_wallinfo_t * late_wallinfo = NULL;  // array
static int late_wallinfo_alloc = 0;
static int late_wall_farthest = -1;

// return index of next free
int  expand_late_wallinfo( void )
{
    int lw_size = late_wallinfo_alloc + LATE_WALLINFO_INC;
    late_wallinfo_t * lw_p =
     (late_wallinfo_t *) realloc(late_wallinfo, lw_size * sizeof(late_wallinfo_t));
    if( lw_p )  // normal
    {
        late_wallinfo = lw_p;
        late_wallinfo_alloc = lw_size;
        return num_late_walls;  // next free
    }
    // failure
    if( num_late_walls <= 0 )
       I_Error( "Late wallinfo alloc failed\n" );  // no alloc
    I_SoftError( "Late wallinfo realloc failed, %i\n", lw_size );
    num_late_walls -= 1;
    return late_wall_farthest;  // reuse farthest, but keep running
}

// [WDJ] To sort transparent walls
// vxtx is in world coord with y up
static
void  late_wall_dist( vxtx3d_t * wVs, late_wallinfo_t * lw_p )
{
    float tr_x = wVs[0].x - gr_viewx;
    float tr_y = wVs[0].z - gr_viewy;
    float tr_z = wVs[0].y - gr_viewz;
    lw_p->dist1 = (tr_x * world_trans_x_to_z )
        + (tr_y * world_trans_y_to_z )
        + (tr_z * world_trans_z_to_z );
    tr_x = wVs[2].x - gr_viewx;
    tr_y = wVs[2].z - gr_viewy;
    tr_z = wVs[2].y - gr_viewz;
    lw_p->dist2 = (tr_x * world_trans_x_to_z )
        + (tr_y * world_trans_y_to_z )
        + (tr_z * world_trans_z_to_z );
}

// Called from HWR_SplitWall, HWR_StoreWallRange
// vxtx is in world coord with y up
static
void HWR_AddTransparentWall(vxtx3d_t * vxtx, FSurfaceInfo_t * pSurf, int texnum, int blend)
{
    late_wallinfo_t *  lw_p;
    int lwi, prev_lwi;
    int new_lwi = num_late_walls;
    if( new_lwi >= late_wallinfo_alloc )
    {
        new_lwi = expand_late_wallinfo();  // may have to reuse farthest
    }

    // New late wall record.
    lw_p = & late_wallinfo[new_lwi];
    memcpy( lw_p->vxtx, vxtx, sizeof(lw_p->vxtx));
    memcpy( & lw_p->Surf, pSurf, sizeof(FSurfaceInfo_t));
    lw_p->texnum = texnum;
    lw_p->blend = blend;
    
    // [WDJ] insert sort into the late_wallinfo
    late_wall_dist( vxtx, lw_p );  // set dist1, dist2
    prev_lwi = -1;
    lwi = late_wall_farthest;
    while( lwi >= 0 )  // from farthest to nearest
    {
        register late_wallinfo_t * wp = & late_wallinfo[lwi];
        // they may share a vertex or two
        if( wp->dist1 < lw_p->dist1 )
            break;  // new wall is farther
        else if( wp->dist1 == lw_p->dist1 )
        {
            if( wp->dist2 < lw_p->dist2 )
                break;  // new wall is farther
            else if( wp->dist2 == lw_p->dist2 ) {
                break; // no longer matters, they must overlap
            }
        }
        prev_lwi = lwi;
        lwi = late_wallinfo[lwi].next_nearer;
    }
    lw_p->next_nearer = lwi;
    if( prev_lwi >= 0 )
    {
        // link in before nearer
        late_wallinfo[prev_lwi].next_nearer = new_lwi;
    }
    else
    {
        // new is farthest
        late_wall_farthest = new_lwi;
    }
    num_late_walls++;
}

//#define DEBUG_DRAWSORTED
static
void HWR_RenderSorted( void )
{
#ifdef DEBUG_DRAWSORTED   
    int   neg_dist_cnt = 0;
    int   transform_load_cnt = 0;
#endif
    byte  affine_transform = 255;
    // transparent walls
    int lwi = -1;
    late_wallinfo_t * lw_p = NULL;
    // sprites
    gr_vissprite_t * spr = NULL;
    float spr_dist = -1000.0f;
    float tranwall_dist = -1000.0f;

    // sprites
    if (gr_vissprite_p > gr_vissprites)
    {
        spr = gr_vsprsortedhead.next;
        if( spr )
            spr_dist = spr->tz;
    }
    if (num_late_walls)
    {
        lwi = late_wall_farthest;
        if( lwi >= 0 )
        {
            lw_p = & late_wallinfo[lwi];
            tranwall_dist = lw_p->dist1;
        }
    }


    for(;;)
    {
        if( spr && ( spr_dist > tranwall_dist ))
        {
#ifdef DEBUG_DRAWSORTED   
            if( spr_dist < 0 )  neg_dist_cnt++;
#else
            if( spr_dist < -200.0f )  break;
               // farther back and cannot even contribute light
#endif
            // sprite
            if( affine_transform != 0 )
            {
                HWD.pfnSetTransform(NULL);
                affine_transform = 0;
#ifdef DEBUG_DRAWSORTED   
                transform_load_cnt ++;
#endif
            }
            HWR_DrawSprite(spr);
            // draw vissprites back to front
            spr = spr->next;
            spr_dist = spr->tz;
            if(spr == &gr_vsprsortedhead)
            {
                spr = NULL;   // stop sprites
                spr_dist = -1000.0f;
            }
        }
        else if( lw_p )
        {
#ifdef DEBUG_DRAWSORTED   
            if( tranwall_dist < 0 )  neg_dist_cnt++;
#else
            // [WDJ] this test has been culling left side visible walls
//	    if( tranwall_dist < -200.0f )  break;
            if( tranwall_dist < -2000.0f )  break;
               // farther back and cannot even contribute light
#endif
            // transparent wall
            if( affine_transform != 1 )
            {
                HWD.pfnSetTransform(&atransform);
                affine_transform = 1;
#ifdef DEBUG_DRAWSORTED   
                transform_load_cnt ++;
#endif
            }
            // with TF_Opaquetrans, TF_Fogsheet flag
            HWR_GetTexture(lw_p->texnum, lw_p->Surf.texflags);
            HWR_RenderWall(lw_p->vxtx, &lw_p->Surf, lw_p->blend);
            lwi = lw_p->next_nearer;
            if( lwi >= 0 )
            {
                lw_p = & late_wallinfo[lwi];
                tranwall_dist = lw_p->dist1;
            }
            else
            {
                lw_p = NULL;
                tranwall_dist = -10000.0f;
            }
        }
        else
            break; // done
    }
    num_late_walls = 0;
    late_wall_farthest = -1;

#ifdef DYLT_CORONAS
    HWD.pfnSetTransform(NULL);
#ifdef CORONA_CHOICE
    if( corona_draw_choice == 2 )
#else
    if( cv_corona.EV )
#endif
    {
        //Hurdler: they must be drawn before translucent planes, what about gl fog?
        HWR_DL_Draw_Coronas();
    }
#endif

    if (numplanes)
    {
        HWD.pfnSetTransform(&atransform);
        if (numplanes)
            HWR_Render3DWater();
        HWD.pfnSetTransform(NULL);
    }
#ifdef DEBUG_DRAWSORTED   
    fprintf( stderr, "Neg dist count %i, Transforms %i\n", neg_dist_cnt, transform_load_cnt );
#endif
}
