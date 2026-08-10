// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hw_draw.c 1656 2023-12-08 14:54:47Z wesleyjohnson $
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
// $Log: hw_draw.c,v $
// Revision 1.27  2004/07/27 08:19:38  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.26  2004/04/18 20:40:31  hurdler
// go 1.42
//
// Revision 1.25  2004/04/18 12:40:15  hurdler
// Jive's request for saving screenshots
//
// Revision 1.24  2003/10/15 14:09:24  darkwolf95
// Fixed screenshots filename bug
//
// Revision 1.23  2003/08/11 15:12:20  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.22  2003/06/11 04:44:11  ssntails
// High-res patch drawer added.
//
// Revision 1.21  2003/06/10 21:48:06  ssntails
// Variable flat size support (32x32 to 2048x2048)
//
// Revision 1.20  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.19  2001/05/16 21:21:15  bpereira
// Revision 1.18  2001/04/01 17:35:07  bpereira
// Revision 1.17  2001/02/28 17:50:56  bpereira
// Revision 1.16  2001/02/24 13:35:22  bpereira
//
// Revision 1.15  2001/01/31 17:15:09  hurdler
// Add cv_scalestatusbar in hardware mode
//
// Revision 1.14  2001/01/25 18:56:27  bpereira
// Revision 1.13  2000/11/02 19:49:39  bpereira
// Revision 1.12  2000/10/04 16:21:57  hurdler
//
// Revision 1.11  2000/09/14 10:42:47  hurdler
// Fix compiling problem under win32
//
// Revision 1.10  2000/09/10 10:48:13  metzgermeister
// Revision 1.9  2000/08/31 14:30:57  bpereira
// Revision 1.8  2000/08/11 19:11:57  metzgermeister
//
// Revision 1.7  2000/04/27 17:48:47  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.6  2000/04/24 15:22:47  hurdler
// Support colormap for text
//
// Revision 1.5  2000/04/23 00:30:47  hurdler
// fix a small bug in skin color
//
// Revision 1.4  2000/04/22 21:08:23  hurdler
//
// Revision 1.3  2000/04/14 16:34:26  hurdler
// some nice changes for coronas
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      miscellaneous drawing (mainly 2d)
//
//-----------------------------------------------------------------------------

#include "doomincl.h"

#include "hw_glob.h"
#include "hw_drv.h"

#include "m_misc.h"
  //FIL_WriteFile()
#include "m_swap.h"
#include "r_draw.h"
  //viewborderlump
#include "r_main.h"
#include "w_wad.h"
#include "z_zone.h"
#include "v_video.h"
#include "p_setup.h"
  // P_flatsize_to_index
  

#include <unistd.h>
#include <fcntl.h>
#include "i_video.h"
  // for rendermode != render_glide



#define BLENDMODE PF_Translucent

//
// -----------------+
// HWR_DrawPatch    : Draw a 'tile' graphic
// Notes            :
//                  : text(console+score) + menus + status bar
// -----------------+
// Does not obey V_CENTERSCREEN, it is stretched to fill screen instead.
//  x, y : vid coordinates, or drawinfo coordinates (see option)
//  option: OR of flags
//          V_TRANSLUCENTPATCH
//          V_DRAWINFO = use drawinfo
//          V_NOSCALE = vid coordinates
void HWR_DrawPatch (MipPatch_t* gpatch, int x, int y, uint32_t option)
{
    vxtx3d_t      v[4];
    float stx, sty, pdupx, pdupy;

    if( option & V_DRAWINFO )
    {
      // V_SetupDraw now has fdupx, fdupy derived from V_SCALEPATCH.
      pdupx = drawinfo.fdupx * 2.0f;
      pdupy = drawinfo.fdupy * 2.0f;

      // V_SetupDraw now has fdupx0, fdupy0 derived from V_SCALESTART.
      stx = x * drawinfo.fdupx0 * 2.0f;
      sty = y * drawinfo.fdupy0 * 2.0f;
    }
    else
    {
      // V_NOSCALE, vid coordinates
      pdupx = 2.0f;
      pdupy = 2.0f;

      stx = x * 2.0f;
      sty = y * 2.0f;
    }

    // make patch ready in hardware cache
    HWR_GetPatch (gpatch);

//  3--2
//  | /|
//  |/ |
//  0--1
    v[0].x = v[3].x = (stx - (gpatch->leftoffset*pdupx))/vid.width - 1;
    v[2].x = v[1].x = (stx + ((gpatch->width - gpatch->leftoffset)*pdupx))/vid.width - 1;
    v[0].y = v[1].y = 1-(sty - (gpatch->topoffset*pdupy))/vid.height;
    v[2].y = v[3].y = 1-(sty + ((gpatch->height - gpatch->topoffset)*pdupy))/vid.height;

    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    v[0].sow = v[3].sow = 0.0f;
    v[2].sow = v[1].sow = gpatch->max_s;
    v[0].tow = v[1].tow = 0.0f;
    v[2].tow = v[3].tow = gpatch->max_t;

    // clip it since it is used for bunny scroll in doom I
    if (option & V_TRANSLUCENTPATCH)
    {
        FSurfaceInfo_t Surf;
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
        Surf.FlatColor.s.alpha = cv_grtranslucenthud.value;

        HWD.pfnDrawPolygon( &Surf, v, 4,
            BLENDMODE | PF_Modulated | PF_Clip | PF_NoZClip | PF_NoDepthTest);
    }
    else
    {
        HWD.pfnDrawPolygon( NULL, v, 4,
            BLENDMODE | PF_Clip | PF_NoZClip | PF_NoDepthTest);
    }
}

#if 0
//[WDJ] 2012-02-06 DrawSmallPatch found to be unused

// Draws a patch 2x as small SSNTails 06-10-2003
void HWR_DrawSmallPatch (MipPatch_t* gpatch, int x, int y, uint32_t option, byte *colormap)
{
    vxtx3d_t      v[4];

    float stx, sty, pdupx, pdupy;

    // make patch ready in hardware cache
    HWR_GetMappedPatch (gpatch, colormap);

    // V_SetupDraw now has fdupx, fdupy derived from V_SCALEPATCH.
    pdupx = drawinfo.fdupx * 2.0f;
    pdupy = drawinfo.fdupy * 2.0f;

    // V_SetupDraw now has fdupx0, fdupy0 derived from V_SCALESTART.
    stx = x * drawinfo.fdupx0 * 2.0f;
    sty = y * drawinfo.fdupy0 * 2.0f;

    v[0].x = v[3].x = (stx - gpatch->leftoffset*pdupx)/vid.width - 1;
    v[2].x = v[1].x = (stx + (gpatch->width - gpatch->leftoffset)*pdupx)/vid.width - 1;
    v[0].y = v[1].y = 1-(sty - gpatch->topoffset*pdupy)/vid.height;
    v[2].y = v[3].y = 1-(sty + (gpatch->height - gpatch->topoffset)*pdupy)/vid.height;

    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    v[0].sow = v[3].sow = 0.0f;
    v[2].sow = v[1].sow = gpatch->max_s;
    v[0].tow = v[1].tow = 0.0f;
    v[2].tow = v[3].tow = gpatch->max_t;

    // clip it since it is used for bunny scroll in doom I
    HWD.pfnDrawPolygon( NULL, v, 4,
        BLENDMODE | PF_Clip | PF_NoZClip | PF_NoDepthTest);
}
#endif

//
// HWR_DrawMappedPatch(): Like HWR_DrawPatch but with translated color
//
// Does not obey V_CENTERSCREEN, it is stretched to fill screen instead.
//  x, y : drawinfo coordinates only
//  option: OR of flags
//          V_TRANSLUCENTPATCH
void HWR_DrawMappedPatch (MipPatch_t* gpatch, int x, int y, uint32_t option, byte * colormap)
{
    vxtx3d_t      v[4];
    float stx, sty, pdupx, pdupy;

    // make patch ready in hardware cache
    HWR_GetMappedPatch (gpatch, colormap);

    // V_SetupDraw now has fdupx, fdupy derived from V_SCALEPATCH.
    pdupx = drawinfo.fdupx * 2.0f;
    pdupy = drawinfo.fdupy * 2.0f;

    // V_SetupDraw now has fdupx0, fdupy0 derived from V_SCALESTART.
    stx = x * drawinfo.fdupx0 * 2.0f;
    sty = y * drawinfo.fdupy0 * 2.0f;
    
    v[0].x = v[3].x = (stx - gpatch->leftoffset*pdupx)/vid.width - 1;
    v[2].x = v[1].x = (stx + (gpatch->width - gpatch->leftoffset)*pdupx)/vid.width - 1;
    v[0].y = v[1].y = 1-(sty - gpatch->topoffset*pdupy)/vid.height;
    v[2].y = v[3].y = 1-(sty + (gpatch->height - gpatch->topoffset)*pdupy)/vid.height;

    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    v[0].sow = v[3].sow = 0.0f;
    v[2].sow = v[1].sow = gpatch->max_s;
    v[0].tow = v[1].tow = 0.0f;
    v[2].tow = v[3].tow = gpatch->max_t;

    // clip it since it is used for bunny scroll in doom I
    if (option & V_TRANSLUCENTPATCH)
    {
        FSurfaceInfo_t Surf;
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
        Surf.FlatColor.s.alpha = cv_grtranslucenthud.value;
        HWD.pfnDrawPolygon( &Surf, v, 4,
            BLENDMODE | PF_Modulated | PF_Clip | PF_NoZClip | PF_NoDepthTest);
    }
    else
    {
        HWD.pfnDrawPolygon( NULL, v, 4,
            BLENDMODE | PF_Clip | PF_NoZClip | PF_NoDepthTest);
    }
}

void HWR_DrawPic(int x, int y, lumpnum_t lumpnum)
{
    vxtx3d_t      v[4];
    MipPatch_t  *   mpatch;

    // make pic ready in hardware cache
    mpatch = HWR_GetPic( lumpnum );

//  3--2
//  | /|
//  |/ |
//  0--1

    v[0].x = v[3].x = (float)x * vid.fx_scale2 - 1;
    v[2].x = v[1].x = (float)(x + mpatch->width*vid.fdupx) * vid.fx_scale2 - 1;
    v[0].y = v[1].y = 1 - (float)y * vid.fy_scale2;
    v[2].y = v[3].y = 1 - (float)(y + mpatch->height*vid.fdupy) * vid.fy_scale2;

    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    v[0].sow = v[3].sow =  0.0f;
    v[2].sow = v[1].sow =  mpatch->max_s;
    v[0].tow = v[1].tow =  0.0f;
    v[2].tow = v[3].tow =  mpatch->max_t;


    //Hurdler: Boris, the same comment as above... but maybe for pics
    // it not a problem since they don't have any transparent pixel
    // if I'm right !?
    // But then, the question is: why not 0 instead of PF_Masked ?
    // or maybe PF_Environment ??? (like what I said above)
    // BP: PF_Environment don't change anything ! and 0 is undefined
    if (cv_grtranslucenthud.value != 255)
    {
        FSurfaceInfo_t Surf;
        Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = 0xff;
        Surf.FlatColor.s.alpha = cv_grtranslucenthud.value;
        HWD.pfnDrawPolygon( &Surf, v, 4,
            BLENDMODE | PF_Modulated | PF_NoDepthTest | PF_Clip | PF_NoZClip);
    }
    else
    {
        HWD.pfnDrawPolygon( NULL, v, 4,
            BLENDMODE | PF_NoDepthTest | PF_Clip | PF_NoZClip);
    }
}

// ==========================================================================
//                                                            V_VIDEO.C STUFF
// ==========================================================================


// Fill Flat size and mask.
// Indexed by flat size_index.
static uint16_t  fill_size_tab[ 8 ] =
{
    0, // 0
    32, // 32x32 flat
    64, // 64x64 flat
    128, // 128x128 flat
    256, // 256x256 flat
    512, // 512x512 flat
    1024, // 1024x1024 flat
    2048, // 2048x2048 flat
};

// --------------------------------------------------------------------------
// Fills a box of pixels using a flat texture as a pattern
// --------------------------------------------------------------------------
// Does not obey V_CENTERSCREEN, it is stretched to fill screen instead.
//   x, y, w, h : vid coordinates, (0,0) at upper left
//   scale : 1 .. 4
void HWR_DrawVidFlatFill (int x, int y, int w, int h, int scale, lumpnum_t flat_lumpnum)
{
    vxtx3d_t  v[4];

//  3--2
//  | /|
//  |/ |
//  0--1

    // Vid Coordinates to -1..+1.
    v[0].x = v[3].x = (x - vid.fx_center) * vid.fx_scale2;
    v[2].x = v[1].x = ((x+w) - vid.fx_center) * vid.fx_scale2;
    v[0].y = v[1].y = (vid.fy_center - y) * vid.fy_scale2;
    v[2].y = v[3].y = (vid.fy_center - (y+h)) * vid.fy_scale2;
#if 0
    // From when this was scaled(320,200)
    v[0].x = v[3].x = (x - 160.0f)/160.0f;
    v[2].x = v[1].x = ((x+w) - 160.0f)/160.0f;
    v[0].y = v[1].y = -(y - 100.0f)/100.0f;
    v[2].y = v[3].y = -((y+h) - 100.0f)/100.0f;
#endif

    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    HWR_GetFlat (flat_lumpnum);
   
    // The index may someday be supplied from the flat structure.
    int sizeindex = P_flatsize_to_index( W_LumpLength(flat_lumpnum), NULL );
    int size = fill_size_tab[ sizeindex ];
    // Scale 0..15 ==> (size/2) .. (size/2 * vid.fdupy)
    double sc = ((double)(scale) * vid.fdupy + (15 - scale))/15 * size / 2;
    // sow = horz, fractional position within flat, or repeat
    // tow = vert
    // To match edges of tiles, which is needed for HWR_DrawViewBorder,
    // need to account for (x,y) position.
    // But, Menu appearance matches with software flat fill better when (x,y)=0.
#if 0
    // From previous code...
    // This does not work, edges and tiles swim with size change.
    int imask = (scale < 6)? 0: size - 1;  // test scale to detect tiling
    v[0].sow = v[3].sow = (x & imask)/sc;
    v[0].tow = v[1].tow = (y & imask)/sc;
#else
    // This has stable tiles.
    // Test scale to detect tiling.
    v[0].sow = v[3].sow = (scale < 6)? 0 : x/sc;
    v[0].tow = v[1].tow = (scale < 6)? 0 : y/sc;
#endif
    v[2].sow = v[1].sow = v[0].sow + w/sc;
    v[2].tow = v[3].tow = v[0].tow + h/sc;

    //Hurdler: Boris, the same comment as above... but maybe for pics
    // it not a problem since they don't have any transparent pixel
    // if I'm right !?
    // BTW, I see we put 0 for PFs, and If I'm right, that
    // means we take the previous PFs as default
    // how can we be sure they are ok?
      // maybe PF_Translucent ??
    HWD.pfnDrawPolygon( NULL, v, 4, PF_NoDepthTest);
}



// --------------------------------------------------------------------------
// Fade down the screen so that the menu drawn on top of it looks brighter
// --------------------------------------------------------------------------
//  3--2
//  | /|
//  |/ |
//  0--1
// Relative to vid.height.
//  color_rgba : rgba color
//  alpha : 1 .. 255
void HWR_FadeScreenMenuBack( uint32_t color_rgba, int alpha, int height )
{
    vxtx3d_t  v[4];
    FSurfaceInfo_t Surf;

    if( height <= 0 )   return;

    // setup some neat-o translucency effect

    // [WDJ] Draws undersized on Matrox on Win32, when it works at all.
    // Many fixes tried, none worked.
    // Exact use of example code may be required, and glGenTextures.
    // May require turning off accel polygon draw.

    v[0].x = v[3].x = -1.0f;
    v[2].x = v[1].x =  1.0f;
    v[0].y = v[1].y = -1.0f;
    v[2].y = v[3].y =  1.0f;
    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    v[0].sow = v[3].sow = 0.0f;
    v[2].sow = v[1].sow = 1.0f;
    v[0].tow = v[1].tow = 1.0f;
    v[2].tow = v[3].tow = 0.0f;

    Surf.FlatColor.rgba = color_rgba;
    // [WDJ] Constant alpha is more reliable.
    Surf.FlatColor.s.alpha = alpha;

    if( height < vid.height )
    {
        // [WDJ] bugginess of this calc when height==vid.height plagued win32.
        float hf2 = ((float)height) * vid.fy_scale2;
        v[0].y = v[1].y = 1.0f - hf2;  // -1.0 .. 1.0
        // [WDJ] This would only give partial alpha, or overflow alpha.
        // Too hard to fix right for such a small visual impact.
//        Surf.FlatColor.s.alpha = alpha * hf;    //calum: varies console alpha
    }
    HWD.pfnDrawPolygon( &Surf, v, 4,
        PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}


// ==========================================================================
//                                                             R_DRAW.C STUFF
// ==========================================================================

// BORDER FILL SCALE, 0..15
#define BF_DOOM_SCALE 10
#define BF_RAVEN_SCALE 14

// ------------------
// HWR_DrawViewBorder
// Fill the space around the view window with a Doom flat texture,
// draw the beveled edges.
//   clearlines : how many lines to refresh.  (0=refresh all)
//     Is used to clear the heads up messages, when the view
//     window is reduced, so it doesn't refresh all the view borders.
// ------------------
void HWR_DrawViewBorder (int clearlines)
{
    MipPatch_t * mpatch;
    int  bf_scale = (EN_heretic_hexen)? BF_RAVEN_SCALE : BF_DOOM_SCALE;
    int  vw_x, vw_y;  // view window x, y for border
    int  refresh_y;
    int  v_top, v_side, v_width, v_height;  // vid coord.
    int  step_x, step_y, off_x, off_y;
    int  x, y;

    if (gr_viewwidth == vid.width)
        return;

    // refresh all when 0
    refresh_y = (clearlines == 0)? vid.height : (clearlines * vid.fdupy);

    // calc view window size and position using vid coordinates
    v_width = gr_viewwidth;
    v_height = gr_viewheight;
    v_top = gr_baseviewwindowy;
    v_side = gr_viewwindowx;

    // Flat fill uses vid coordinates to fill to screen edges.
    // top
    HWR_DrawVidFlatFill (0, 0,
                     vid.width, ((v_top<refresh_y) ? v_top : refresh_y),
                     bf_scale, st_borderflat_num);

    if (v_top < refresh_y)
    {
        y = ((refresh_y-v_top < v_height) ? refresh_y-v_top : v_height);
        // left
        HWR_DrawVidFlatFill (0, v_top, v_side, y, bf_scale, st_borderflat_num);
        // right
        HWR_DrawVidFlatFill (v_side + v_width, v_top, v_side, y, bf_scale, st_borderflat_num);
    }

    // bottom
    if (v_top+v_height < refresh_y)
        HWR_DrawVidFlatFill (0, v_top+v_height, vid.width, vid.height,
                         bf_scale, st_borderflat_num);

    //
    // draw the view border edging
    //

    // view window position for border edge
    vw_x = (vid.width - v_width)>>1;
    if (v_width >= vid.width)
        vw_y = 0;
    else
        vw_y = v_top;

    // Edge patch size
    if( EN_heretic )
    {
        step_x = step_y = 16;
        off_x = off_y = 4; // borderoffset
    }
    else
    {
        step_x = step_y = 8;
        off_x = off_y = 8;
    }

#if 1
   
#define EDGE_OPT   V_NOSCALE
    V_SetupDraw( V_NOSCALE ); // the edge patch not scaled, like native draw.

#else

#define EDGE_OPT   V_DRAWINFO
    V_SetupDraw( V_SCALEPATCH ); // the edge patch scaled.
    // Patch size
    step_x *= vid.fdupx;
    step_y *= vid.fdupy;
    off_x *= vid.fdupx;
    off_y *= vid.fdupy;

#endif

    // top edge
    if (vw_y-off_y < refresh_y) {
        mpatch = W_CachePatchNum (viewborderlump[BRDR_T],PU_CACHE);
        for (x=vw_x ; x<(vw_x+v_width); x+=step_x)
            HWR_DrawPatch (mpatch, x, vw_y-off_y, EDGE_OPT);
    }

    // bottom edge
    if (vw_y+v_height < refresh_y) {
        mpatch = W_CachePatchNum (viewborderlump[BRDR_B],PU_CACHE);
        for (x=vw_x ; x<vw_x+v_width; x+=step_x)
            HWR_DrawPatch (mpatch, x, vw_y+v_height, EDGE_OPT);
    }

    if (vw_y < refresh_y)
    {
        int v_bot = (vw_y+v_height < refresh_y)? vw_y+v_height : refresh_y;
        // Does not divide evenly, so the last is drawn aligned.
        // left edge
        mpatch = W_CachePatchNum (viewborderlump[BRDR_L],PU_CACHE);
        for (y=vw_y ; y<(v_bot-step_y); y+=step_y)
            HWR_DrawPatch (mpatch, vw_x-off_x, y, EDGE_OPT);
        HWR_DrawPatch (mpatch, vw_x-off_x, v_bot-step_y, EDGE_OPT);

        // right edge
        mpatch = W_CachePatchNum (viewborderlump[BRDR_R],PU_CACHE);
        for (y=vw_y ; y<(v_bot-step_y); y+=step_y)
            HWR_DrawPatch (mpatch, vw_x+v_width, y, EDGE_OPT);
        HWR_DrawPatch (mpatch, vw_x+v_width, v_bot-step_y, EDGE_OPT);
    }

    // top corners
    if (vw_y-off_y < refresh_y)
    {
        HWR_DrawPatch (W_CachePatchNum (viewborderlump[BRDR_TL],PU_CACHE),
                       vw_x-off_x, vw_y-off_y, EDGE_OPT);

        HWR_DrawPatch (W_CachePatchNum (viewborderlump[BRDR_TR],PU_CACHE),
                       vw_x+v_width, vw_y-off_y, EDGE_OPT);
    }

    // bottom corners
    if (vw_y+v_height < refresh_y)
    {
        HWR_DrawPatch (W_CachePatchNum (viewborderlump[BRDR_BL],PU_CACHE),
                       vw_x-off_x, vw_y+v_height, EDGE_OPT);

        HWR_DrawPatch (W_CachePatchNum (viewborderlump[BRDR_BR],PU_CACHE),
                       vw_x+v_width, vw_y+v_height, EDGE_OPT);
    }
}


// ==========================================================================
//                                                     AM_MAP.C DRAWING STUFF
// ==========================================================================

// Clear the automap part of the screen
void HWR_Clear_Automap( void )
{
    RGBA_float_t fColor = { 0,0,0,1 };

    //FIXTHIS faB - optimize by clearing only colors ?
    //HWD.pfnSetBlend ( PF_NoOcclude );

    // minx,miny,maxx,maxy
    HWD.pfnGClipRect( 0, 0, vid.width , vid.height - stbar_height, NEAR_CLIP_DIST );
    HWD.pfnClearBuffer( true, true, &fColor );
    HWD.pfnGClipRect( 0, 0, vid.width, vid.height, NEAR_CLIP_DIST );
}


// -----------------+
// HWR_drawAMline   : draw a line of the automap (the clipping is already done in automap code)
// -----------------+
//  color : palette index
void HWR_drawAMline( fline_t* fl, int color )
{
    v2d_t  v1, v2;
    RGBA_t    color_rgba;

    color_rgba = V_GetColor( color );

    v1.x = ((float)fl->a.x - vid.fx_center) * vid.fx_scale2;
    v1.y = ((float)fl->a.y - vid.fy_center) * vid.fy_scale2;

    v2.x = ((float)fl->b.x - vid.fx_center) * vid.fx_scale2;
    v2.y = ((float)fl->b.y - vid.fy_center) * vid.fy_scale2;

    HWD.pfnDraw2DLine( &v1, &v2, color_rgba );
}


// -----------------+
// HWR_DrawVidFill     : draw flat coloured rectangle, with no texture
// -----------------+
// Vid range coordinates, (0,0) at upper left
//  x, y : screen coord. vid range.
//  color : palette index
void HWR_DrawVidFill( int x, int y, int w, int h, int color )
{
    vxtx3d_t  v[4];
    FSurfaceInfo_t Surf;

//  3--2
//  | /|
//  |/ |
//  0--1
    v[0].x = v[3].x = (x - vid.fx_center) * vid.fx_scale2;
    v[2].x = v[1].x = ((x+w) - vid.fx_center) * vid.fx_scale2;
    v[0].y = v[1].y = (vid.fy_center - y) * vid.fy_scale2;
    v[2].y = v[3].y = (vid.fy_center - (y+h)) * vid.fy_scale2;

#ifdef _GLIDE_ARGB_
    //Hurdler: do we still use this argb color? if not, we should remove it
    v[0].argb = v[1].argb = v[2].argb = v[3].argb = 0xff00ff00; //;
#endif
    v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

    v[0].sow = v[3].sow = 0.0f;
    v[2].sow = v[1].sow = 1.0f;
    v[0].tow = v[1].tow = 0.0f;
    v[2].tow = v[3].tow = 1.0f;

    Surf.FlatColor = V_GetColor( color );

    HWD.pfnDrawPolygon( &Surf, v, 4,
                        PF_Modulated|PF_NoTexture| PF_NoDepthTest );
}


// --------------------------------------------------------------------------
// screen shot
// --------------------------------------------------------------------------

// Return buffer, malloc.
byte *  HWR_Get_Screenshot ( byte * bitpp )
{
    byte* bufr;

    // vid.screen_size is not set for OpenGL.
    // Sized for 24 bit (OpenGL), will work for 16 bit (Glide) too.
    bufr = malloc( (size_t)vid.width * (size_t)vid.height * 3 );
    if (!bufr)
        return NULL;

    // returns 24 bit or 16 bit 565 RGB data, indicated by bitpp
    HWD.pfnReadRect (0, 0, vid.width, vid.height, /*OUT*/ bufr, bitpp );

    return bufr;
}


