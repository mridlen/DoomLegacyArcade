// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_main.h 1656 2023-12-08 14:54:47Z wesleyjohnson $
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
// $Log: hw_main.h,v $
// Revision 1.21  2004/07/27 08:19:38  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.20  2001/12/31 13:47:46  hurdler
// Add setcorona FS command and prepare the code for beta 4
//
// Revision 1.19  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.18  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.17  2001/05/16 21:21:15  bpereira
// Revision 1.16  2001/04/09 23:26:06  hurdler
// Revision 1.15  2001/04/09 14:24:56  hurdler
//
// Revision 1.14  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.13  2001/02/24 13:35:22  bpereira
// Revision 1.12  2001/01/25 18:56:28  bpereira
// Revision 1.11  2000/10/04 16:21:57  hurdler
// Revision 1.10  2000/08/31 14:30:57  bpereira
// Revision 1.9  2000/07/01 09:23:50  bpereira
//
// Revision 1.8  2000/05/09 20:57:31  hurdler
// use my own code for colormap (next time, join with Boris own code)
// (necessary due to a small bug in Boris' code (not found) which shows strange effects under linux)
//
// Revision 1.7  2000/04/30 10:30:10  bpereira
//
// Revision 1.6  2000/04/27 17:48:47  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.5  2000/04/24 15:23:13  hurdler
// Support colormap for text
//
// Revision 1.4  2000/04/22 21:08:23  hurdler
//
// Revision 1.3  2000/04/12 16:03:51  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      3D render mode functions
//
//-----------------------------------------------------------------------------

#ifndef HW_MAIN_H
#define HW_MAIN_H

#include "doomdef.h"
#include "hw_defs.h"
#include "hw_data.h"

#include "am_map.h"
#include "d_player.h"
#include "r_defs.h"
#include "command.h"
  // consvar_t

extern float gr_baseviewwindowy, gr_viewwindowx, gr_viewheight, gr_viewwidth;

// Startup & Shutdown the hardware mode renderer
void HWR_Startup_Render (void);
void HWR_Shutdown_Render (void);
void HWR_SetupLevel(void);
void HWR_Preload_Graphics( void );
void HWR_Release_Graphics( void );

void HWR_Clear_Automap (void);
void HWR_drawAMline (fline_t* fl, int color);
void HWR_FadeScreenMenuBack( uint32_t color_rgba, int alpha, int height );
void HWR_RenderPlayerView (byte viewnumber, player_t* player);
void HWR_DrawViewBorder (int clearlines);
//   x, y, w, h : vid coordinates, relative to center.
//   scale : 0 .. 15
void HWR_DrawVidFlatFill (int x, int y, int w, int h, int scale, lumpnum_t flatlumpnum);
byte *  HWR_Get_Screenshot ( byte * bitpp );
void HWR_SetViewSize( int viewsize );
void HWR_DrawPatch (MipPatch_t* gpatch, int x, int y, uint32_t option);
void HWR_DrawMappedPatch (MipPatch_t* gpatch, int x, int y, uint32_t option, byte *colormap);

void HWR_MakePatch (patch_t* patch, MipPatch_t* grPatch, Mipmap_t *grMipmap,
                    uint32_t drawflags);
// This releases the allocation made with HWR_MakePatch
void HWR_release_Patch ( MipPatch_t* grPatch, Mipmap_t *grMipmap );

void HWR_sky_mipmap(void);

void HWR_Create_PlanePolygons (void);
void HWR_Create_StaticLightmaps (void);
void HWR_Prep_LevelCache (int numtextures);
// Scaled to vid, (0,0) at upper left
//  x, y : scaled screen coord.
//  color : palette index
void HWR_DrawVidFill( int x, int y, int w, int h, int color );
void HWR_DrawPic(int x, int y, lumpnum_t lumpnum);

void HWR_Register_Gr1Commands (void);
void HWR_CorrectSWTricks(void);
void transform_world_to_gr(float wx, float wy, float wz, /*OUT*/ float *gx, float *gy, float *gz );
int HWR_TranstableToAlpha(int transtablenum, FSurfaceInfo_t *pSurf);

extern consvar_t cv_grmlook_extends_fov;
extern consvar_t cv_grdynamiclighting;
extern consvar_t cv_grstaticlighting;
extern consvar_t cv_grcorona_draw;
extern consvar_t cv_grfov;
extern consvar_t cv_grpolygonsmooth;
extern consvar_t cv_grmd2;
extern consvar_t cv_grtranswall;
extern consvar_t cv_grfog;
extern consvar_t cv_grfogcolor;
extern consvar_t cv_grfogdensity;
extern consvar_t cv_grcontrast;
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;
extern consvar_t cv_grfiltermode;
extern consvar_t cv_grcorrecttricks;
extern consvar_t cv_grsolvetjoin;
extern consvar_t cv_grpolytile;
extern consvar_t cv_grpolyshape;

extern byte  EN_HWR_flashpalette;  // Enable flash palette call.

#endif
