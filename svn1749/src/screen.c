// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: screen.c 1569 2021-01-28 09:23:45Z wesleyjohnson $
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
// $Log: screen.c,v $
// Revision 1.14  2004/05/16 20:34:47  hurdler
//
// Revision 1.13  2002/11/12 00:06:05  ssntails
// Support for translated translucent columns in software mode.
//
// Revision 1.12  2001/08/20 20:40:39  metzgermeister
// Revision 1.11  2001/05/16 21:21:14  bpereira
//
// Revision 1.10  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.9  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.8  2000/11/02 19:49:37  bpereira
// Revision 1.7  2000/08/31 14:30:56  bpereira
// Revision 1.6  2000/08/11 19:10:13  metzgermeister
// Revision 1.5  2000/08/03 17:57:42  bpereira
//
// Revision 1.4  2000/04/22 20:27:35  metzgermeister
// support for immediate fullscreen switching
//
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      handles multiple resolutions, 8bpp/16bpp(highcolor) modes
//
//-----------------------------------------------------------------------------

// [WDJ] If you need to use a debugger then disable fullscreeen
//#define DEBUG_WINDOWED


#include "doomincl.h"
#include "screen.h"
#include "console.h"
#include "am_map.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "z_zone.h"
//#include "d_main.h"
#include "hardware/hw_main.h"





// --------------------------------------------
// assembly or c drawer routines for 8bpp/16bpp
// --------------------------------------------
void (*skycolfunc) (void);       //new sky column drawer draw posts >128 high
void (*colfunc) (void);          // standard column upto 128 high posts

#ifdef HORIZONTALDRAW
//Fab 17-06-98
void (*hcolfunc) (void);         // horizontal column drawer optimisation
#endif

void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);      // standard fuzzy effect column drawer
void (*skincolfunc) (void);      // skin translation
void (*transcolfunc) (void);     // translucent column drawer
void (*shadecolfunc) (void);     // smokie test..
void (*fogcolfunc) (void);       // fog effects
#ifdef ENABLE_DRAW_ALPHA
void (*alpha_colfunc) (void);       // fog effects
#endif
void (*spanfunc) (void);         // span drawer, use a 64x64 tile
void (*basespanfunc) (void);     // default span func for color mode
void (*fogspanfunc) (void);      // Legacy Fog sheet
void (*transspanfunc) (void);    // translucent span drawer

// Tails 11-11-2002
void (*skintranscolfunc) (void); // skin translation translucent

// ------------------
// global video state
// ------------------
viddef_t  vid;
// video mode change needed, set by menu ( 0 = NOP )
modenum_t setmodeneeded = {MODE_NOP,0};

const char * modetype_string[ MODE_other + 1 ] =
{
    "",
    "window",
    "fullscreen",
    "Voodoo",
    "either",
    "other",
};


// use original Doom fuzzy effect instead of translucency?
void CV_Fuzzymode_OnChange();
//CV_PossibleValue_t fuzzymode_cons_t[]={{0,"Translucent"}, {1,"Fuzzy"}, {0,NULL}};
consvar_t   cv_fuzzymode = {"fuzzymode", "Off", CV_SAVE | CV_CALL, CV_OnOff, CV_Fuzzymode_OnChange};


CV_PossibleValue_t scr_depth_cons_t[]={{8,"8 bits"}, {15,"15 bits"}, {16,"16 bits"}, {24,"24 bits"}, {32,"32 bits"}, {0,NULL}};

//added:03-02-98: default screen mode, as loaded/saved in config
consvar_t   cv_scr_width  = {"scr_width",  "320", CV_VALUE|CV_SAVE, CV_uint16};
consvar_t   cv_scr_height = {"scr_height", "200", CV_VALUE|CV_SAVE, CV_uint16};
consvar_t   cv_scr_depth =  {"scr_depth",  "8 bits",   CV_SAVE, scr_depth_cons_t};
consvar_t   cv_fullscreen = {"fullscreen",  "Yes",CV_SAVE | CV_CALL, CV_YesNo, SCR_ChangeFullscreen};

// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

byte*  scr_borderflat; // flat used to fill the reduced view borders
                       // set at ST_Init ()

#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 ) || defined( ENABLE_DRAW24 ) || defined( ENABLE_DRAW32 )
#define ENABLE_DRAWEXT

#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
// hicolor masks  15 bit / 16 bit
uint16_t mask_01111 = 0, mask_01110 = 0, mask_11110 = 0, mask_11100 = 0, mask_11000 = 0;
uint16_t mask_r = 0, mask_g = 0, mask_b = 0, mask_rb = 0;
byte     shift_r, shift_g, shift_b;
#endif
#endif


// =========================================================================

//added:27-01-98: tell asm code the new rowbytes value.
void ASMCALL ASM_PatchRowBytes(int rowbytes);


//  Short and Tall sky drawer, for the current color mode
void (*skydrawerfunc[2]) (void);


#ifdef PARANOID       
void R_DrawDummy( void )
{
   printf( "R_DrawDummy\n" );
}

void SCR_Set_dummy_draw( void )
{
  colfunc = basecolfunc = R_DrawDummy;
  skincolfunc = transcolfunc = shadecolfunc = R_DrawDummy;
  fogcolfunc = R_DrawDummy;
#ifdef ENABLE_DRAW_ALPHA       
  alpha_colfunc = R_DrawDummy;
#endif

  spanfunc = basespanfunc = R_DrawDummy;
  fogspanfunc = transspanfunc = R_DrawDummy;
  skintranscolfunc = R_DrawDummy;

  skydrawerfunc[0] = R_DrawDummy;
  skydrawerfunc[1] = R_DrawDummy;
}
#endif


// Called from D_DoomMain, to start Full Graphics.
// Called from D_DoomLoop, when setmodeneeded or drawmode_recalc.
// Called from D_Display, when setmodeneeded or drawmode_recalc.
//   change_flag : 0=initial setup, 1=change existing setup
void SCR_SetMode( byte change_flag )
{
    int ret_value = 0;
    byte old_HWR_patchstore = HWR_patchstore;

    if(dedicated)
        goto done;

    // Possible change of rendermode
    if( change_flag && drawmode_recalc && set_drawmode )
    {
        // Switch the drawmode, this may change the rendermode, setmodeneeded.
        ret_value = V_switch_drawmode( set_drawmode, change_flag );
        if( setmodeneeded.modetype == MODE_NOP )
        {
            if( ! ret_value )  goto done;
            setmodeneeded = vid.modenum;
        }
    }

    // triggered by V_switch_drawmode
    if( rendermode_recalc )
    {
        if( change_flag )
        {
            // Tear down of structures dependent upon rendermode
            // Release using old HWR_patchstore setting.
            SB_Heretic_Release_Graphics();
            ST_Release_Graphics();
            ST_Release_FaceGraphics();
            WI_Release_Data();
            R_Release_Corona();

#ifdef HWRENDER
            if( HWR_patchstore )
            {
                HWR_Shutdown_Render();
            }
#endif

            Z_FreeTags( PU_PURGELEVEL, PU_UNLOCK_CACHE);
        }

#ifdef HWRENDER
        // Safe to switch to the new rendermode patch storage..
        HWR_patchstore = (rendermode > render_soft);
#endif

        I_Rendermode_setup();
    }

    // VID_SetMode will clear vid.draw_ready if it has print messages.
#ifdef DEBUG_WINDOWED
    {
      // Disable fullscreen so can switch to debugger at breakpoints.
      cv_fullscreen.EV = 0;
      if( drawmode_recalc )
      {
          ret_value = I_RequestFullGraphics( false );
      }
      CONS_Printf("Debug Graphics Window...\n");
      modenum_t mode800 = VID_GetModeForSize(800,600, MODE_window);  // debug window
      VID_SetMode(mode800);
      vid.modenum = setmodeneeded; // fix the display
    }
#else
    // video system interface, sets vid.recalc
    if( drawmode_recalc )
    {
        // Graphics drawmode setup, get video modes
        // param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
//        cv_fullscreen.EV = cv_fullscreen.value && allow_fullscreen;
        ret_value = I_RequestFullGraphics( cv_fullscreen.EV );
        if( ret_value < 0 )
            GenPrintf(EMSG_error, "Change Graphics failed: err=%i, fullscreen=%i\n", ret_value, cv_fullscreen.EV );
        if( ret_value < 0 && cv_fullscreen.EV )
        {
            cv_fullscreen.EV = 0;
            ret_value = I_RequestFullGraphics( 0 );  // window mode
        }
    }
    else
    {
        // Select a video mode, within the drawmode and mode list.
        // Can switch fullscreen.
        ret_value = VID_SetMode(setmodeneeded);
            // sets vid.width, vid.height
    }
#endif
   
    // The screens must be setup before any error or verbose messages are
    // printed, or else it will segfault on the bad screen ptr.
    // Redundant call, for some video drivers,
    // but for other cases the video drivers have not setup the screens.
    // Set the screen[x] ptrs on the new vidbuffers.
    // Sets vid.draw_ready to indicate ready to draw messages.
    V_Setup_VideoDraw();

    // No longer can use display, must fail
    if( rendermode == render_soft && vid.display == NULL )
        I_Error( "VID_SetMode failed to provide any display\n" );

    //
    //  setup the right draw routines for either 8bpp or 16bpp
    //
    //debug_Printf ("SCR_SetMode : vid.bitpp is %d\n", vid.bitpp);
    // set the appropriate drawer for the sky (tall or short)
    // vid.bitpp is already protected by V_Setup_VideoDraw
    switch( vid.bitpp )
    {
     case 8:
        vid.drawmode = DRAW8PAL;
        colfunc = basecolfunc = R_DrawColumn_8;
#ifdef HORIZONTALDRAW
        hcolfunc = R_DrawHColumn_8;
#endif
        skincolfunc = R_DrawTranslatedColumn_8;  // skin translation
        transcolfunc = R_DrawTranslucentColumn_8;
        shadecolfunc = R_DrawShadeColumn_8;  //R_DrawColumn_8;
        fogcolfunc = R_DrawFogColumn_8;
#ifdef ENABLE_DRAW_ALPHA
        alpha_colfunc = R_DrawAlphaColumn_8;
#endif

        spanfunc = basespanfunc = R_DrawSpan_8;
        fogspanfunc = R_DrawFogSpan_8;
        transspanfunc = R_DrawTranslucentSpan_8;

        // SSNTails 11-11-2002
        skintranscolfunc = R_DrawTranslatedTranslucentColumn_8;

        // FIXME: quick fix
        skydrawerfunc[0] = R_DrawColumn_8;      //old skies
        skydrawerfunc[1] = R_DrawSkyColumn_8;   //tall sky
        break;
#ifdef ENABLE_DRAW15
     case 15:
        vid.drawmode = DRAW15;
        mask_01111 = 0x3DEF;  // 0 01111 01111 01111 mask out the upper bit of R,G,B
        mask_01110 = 0x39CE;  // 0 01110 01110 01110 mask out the upper and lowest bit of R,G,B
        mask_11110 = 0x7BDE;  // 0 11110 11110 11110 mask out the lowest bit of R,G,B
        mask_11100 = 0x739C;  // 0 11100 11100 11100 mask out the lowest bits of R,G,B
        mask_11000 = 0x6318;  // 0 11000 11000 11000 mask out the lowest bits of R,G,B
        mask_r = 0x7C00;  // 0 11111 00000 00000 mask of R
        mask_g = 0x03E0;  // 0 00000 11111 00000 mask of G
        mask_b = 0x001F;  // 0 00000 00000 11111 mask of B
        mask_rb = 0x7C1F;  // 0 11111 00000 11111 mask of R and B
        shift_r = 10;
        shift_g = 5;
        shift_b = 0;
        goto highcolor_common;
#endif
#ifdef ENABLE_DRAW16
     case 16:
        vid.drawmode = DRAW16;
#if 0
        // by definition, but extra bit in green gives whites a green cast
        mask_01111 = 0x7BEF;  // 01111 011111 01111 mask out the upper bit of R,G,B
        mask_01110 = 0x73CE;  // 01110 011110 01110 mask out the upper bit of R,G,B
        mask_11110 = 0xF7DE;  // 11110 111110 11110 mask out the lowest bit of R,G,B
        mask_11110 = 0xE79C;  // 11100 111100 11100 mask out the lowest bits of R,G,B
        mask_11100 = 0xC718;  // 11000 111000 11000 mask out the lowest bits of R,G,B
        mask_r = 0xF800;  // 11111 000000 00000 mask of R
        mask_g = 0x07E0;  // 00000 111111 00000 mask of G (green cast due to 1 more bit)
        mask_b = 0x001F;  // 00000 000000 11111 mask of B
        mask_rb = 0xF81F;  // 11111 000000 11111 mask of R and B
#else
        // equal number of bits, 5 bits to each color
        mask_01111 = 0x7BCF;  // 01111 011110 01111 mask out the upper bit of R,G,B
        mask_01110 = 0x738E;  // 01110 011100 01110 mask out the upper bit of R,G,B
        mask_11110 = 0xF79E;  // 11110 111100 11110 mask out the lowest bit of R,G,B
        mask_11100 = 0xE71C;  // 11100 111000 11100 mask out the lowest bits of R,G,B
        mask_11000 = 0xC618;  // 11000 110000 11000 mask out the lowest bits of R,G,B
        mask_r = 0xF800;  // 11111 000000 00000 mask of R
        mask_g = 0x07C0;  // 00000 111110 00000 mask of G (5 bit each)
        mask_b = 0x001F;  // 00000 000000 11111 mask of B
        mask_rb = 0xF81F;  // 11111 000000 11111 mask of R and B
#endif
        shift_r = 11;
        shift_g = 5;
        shift_b = 0;
        goto highcolor_common;
#endif

#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
     highcolor_common:
        CONS_Printf ("using highcolor mode\n");

        colfunc = basecolfunc = R_DrawColumn_16;
        skincolfunc = R_DrawTranslatedColumn_16;
        transcolfunc = R_DrawTranslucentColumn_16;
        shadecolfunc = R_DrawShadeColumn_16;
        fogcolfunc = R_DrawFogColumn_16;
#ifdef ENABLE_DRAW_ALPHA       
        alpha_colfunc = R_DrawAlphaColumn_16;
#endif

        spanfunc = basespanfunc = R_DrawSpan_16;
        fogspanfunc = R_DrawFogSpan_16;
        transspanfunc = R_DrawTranslucentSpan_16;

        skintranscolfunc = R_DrawTranslatedTranslucentColumn_16;

        // FIXME: quick fix to think more..
        skydrawerfunc[0] = R_DrawColumn_16;
        skydrawerfunc[1] = R_DrawSkyColumn_16;
        break;
#endif
#ifdef ENABLE_DRAW24
     case 24:
        vid.drawmode = DRAW24;
        colfunc = basecolfunc = R_DrawColumn_24;
        skincolfunc = R_DrawTranslatedColumn_24;
        transcolfunc = R_DrawTranslucentColumn_24;
        shadecolfunc = R_DrawShadeColumn_24;
        fogcolfunc = R_DrawFogColumn_24;
#ifdef ENABLE_DRAW_ALPHA       
        alpha_colfunc = R_DrawAlphaColumn_24;
#endif

        spanfunc = basespanfunc = R_DrawSpan_24;
        fogspanfunc = R_DrawFogSpan_24;
        transspanfunc = R_DrawTranslucentSpan_24;

        skintranscolfunc = R_DrawTranslatedTranslucentColumn_24;

        // FIXME: quick fix to think more..
        skydrawerfunc[0] = R_DrawColumn_24;
        skydrawerfunc[1] = R_DrawSkyColumn_24;
        break;
#endif
#ifdef ENABLE_DRAW32
     case 32:
        vid.drawmode = DRAW32;
        colfunc = basecolfunc = R_DrawColumn_32;
        skincolfunc = R_DrawTranslatedColumn_32;
        transcolfunc = R_DrawTranslucentColumn_32;
        shadecolfunc = R_DrawShadeColumn_32;
        fogcolfunc = R_DrawFogColumn_32;
#ifdef ENABLE_DRAW_ALPHA       
        alpha_colfunc = R_DrawAlphaColumn_32;
#endif

        spanfunc = basespanfunc = R_DrawSpan_32;
        fogspanfunc = R_DrawFogSpan_32;
        transspanfunc = R_DrawTranslucentSpan_32;

        skintranscolfunc = R_DrawTranslatedTranslucentColumn_32;

        // FIXME: quick fix to think more..
        skydrawerfunc[0] = R_DrawColumn_32;
        skydrawerfunc[1] = R_DrawSkyColumn_32;
        break;
#endif
     default:
        goto bpp_err;
    }

    V_SetPalette (0);

    // set fuzzcolfunc
    CV_Fuzzymode_OnChange();

    if( rendermode_recalc )
    {
#ifdef HWRENDER
        byte new_HWR_patchstore = HWR_patchstore;
        // must release with the proper patchstore setting
        HWR_patchstore = old_HWR_patchstore;
#endif

        // release (if already loaded)
        HU_Release_Graphics();  // need this until last second, for fonts
       
        // Setup patch load, again.
#ifdef HWRENDER
        HWR_patchstore = new_HWR_patchstore;
#endif
        HU_Load_Graphics();  // Load the console fonts.
        ST_Load_Graphics();  // Doom and Heretic
        WI_Load_Data();
#ifdef HWRENDER
        if( rendermode != render_soft )
        {
            HWR_Preload_Graphics();
        }
#endif
        R_Load_Corona();
        R_Setup_SkyDraw();
     }

 done:   
     setmodeneeded.modetype = MODE_NOP;  // NULL
     return;

 bpp_err:
     I_Error ("SetMode: cannot draw %i bits per pixel\n", vid.bitpp);
}


// change drawer function when fuzzymode is changed
void CV_Fuzzymode_OnChange(void)
{
  if( (rendermode != render_soft) && cv_fuzzymode.EV )
      GenPrintf(EMSG_hud, "OpenGL has only translucent shadow.\n" );
  switch(vid.drawmode)
  {
   default:
   case DRAW8PAL:
     fuzzcolfunc = (cv_fuzzymode.EV) ? R_DrawFuzzColumn_8 : R_DrawTranslucentColumn_8;
     break;
#if defined(ENABLE_DRAW15) || defined(ENABLE_DRAW16)
   case DRAW15:
   case DRAW16:
     fuzzcolfunc = (cv_fuzzymode.EV) ? R_DrawFuzzColumn_16 : R_DrawTranslucentColumn_16;
     break;
#endif
#ifdef ENABLE_DRAW24
   case DRAW24:
     fuzzcolfunc = (cv_fuzzymode.EV) ? R_DrawFuzzColumn_24 : R_DrawTranslucentColumn_24;
     break;
#endif
#ifdef ENABLE_DRAW32
   case DRAW32:
     fuzzcolfunc = (cv_fuzzymode.EV) ? R_DrawFuzzColumn_32 : R_DrawTranslucentColumn_32;
     break;
#endif
  }
}



//  do some initial settings for the game loading screen
//
// Dependent upon vid settings
// May be called more than once
void SCR_Startup (void)
{
    if(dedicated)
        return;

    V_Setup_VideoDraw();

#ifdef USEASM
    ASM_PatchRowBytes(vid.ybytes);
#endif

    // Called too early to get Doom palette from wad.
    V_Setup_Wad_VideoResc();

    V_SetPalette (0);
}


//added:24-01-98:
//
// Recalc settings for game drawing, trigger updates to view size and status bar.
// Called at new frame, if the video mode has changed
// Called from D_DoomLoop upon entry, after SCR_SetMode.
// Called from D_Display upon video mode change, after SCR_SetMode.
void SCR_Recalc (void)
{
    if( ! vid.recalc )
        return;

    if( dedicated )
        return;

    V_Setup_Wad_VideoResc();

    // patch the asm code depending on vid buffer rowbytes
#ifdef USEASM
    ASM_PatchRowBytes(vid.ybytes);
#endif

    // toggle off automap because some screensize-dependent values will
    // be calculated next time the automap is activated.
    if (automapactive)
        AM_Stop();

    // fuzzoffsets for the 'spectre' effect,... this is a quick hack for
    // compatibility, because I don't use it anymore since transparency
    // looks much better.
    R_RecalcFuzzOffsets();

        // r_plane stuff : visplanes, openings, floorclip, ceilingclip, spanstart,
        //                 spanstop, yslope, distscale, cachedheight, cacheddistance,
        //                 cachedxstep, cachedystep
        //              -> allocated at the maximum vidsize, static.

    // r_main : x_to_viewangle, allocated at the maximum size.
    // r_things : clip_screen_top_min, clip_screen_bot_max allocated max. size.

    // scr_viewsize doesn't change, neither detailLevel, but the pixels
    // per screenblock is different now, since we've changed resolution.
    R_SetViewSize ();   //just set setsizeneeded true now ..

    // vid.recalc lasts only for the next refresh...
    con_recalc = true;
//    CON_ToggleOff ();  // make sure con height is right for new screen height

    stbar_recalc = true;
    am_recalc = true;
}


// Apply the config video settings.
void SCR_apply_video_settings( byte calc_setmodeneeded )
{
    // command line settings have precedence
    if( ! (req_command_video_settings & 0x3F) ) // bitpp
    {
        // From config file, or menu.
        req_bitpp = cv_scr_depth.value;
    }
    if( req_command_video_settings & 0xC0 ) // width, height
    {
        if( req_width < 100 )
            req_width = ((int)req_height) * 4 / 3;  // approximate
        if( req_height < 100 )
            req_height = ((int)req_width) * 3 / 4;  // approximate
    }
    else
    {
        // From config file, or menu.
        req_width = cv_scr_width.value;
        req_height = cv_scr_height.value;
    }

    if( verbose )
    {
        CONS_Printf("Request video: %d x %d (%d bits)\n", req_width, req_height, req_bitpp );
    }

    if( calc_setmodeneeded )  // not during init
    {
        // returns MODE_NOP if not found, (no mode change)
        setmodeneeded = VID_GetModeForSize( req_width, req_height, vid_mode_table[cv_fullscreen.EV] );
    }
}


//added:03-02-98: sets the modenum as the new default video mode to be saved
//                in the config file
void SCR_SetDefaultMode (void)
{
    // remember the default screen size
    CV_SetValue (&cv_scr_width,  vid.width);
    CV_SetValue (&cv_scr_height, vid.height);
    CV_SetValue (&cv_scr_depth,  vid.bitpp);
    //    CV_SetValue (&cv_fullscreen, vid.fullscreen); // metzgermeister: unnecessary?
}

// Change fullscreen on/off according to cv_fullscreen
void SCR_ChangeFullscreen (void)
{
    // Can cancel allow_fullscreen, when past I_RequestFullGraphics.
    if( graphics_state >= VGS_fullactive )  // by I_RequestFullGraphics
        allow_fullscreen = true;  // override command line

    // Prevent switching to fullscreen during startup

//    if( graphics_state >= VGS_fullactive )  // by I_RequestFullGraphics
//    if( graphics_state >= VGS_active )  //  by I_StartupGraphics()
    if( graphics_state >= VGS_startup )  // by I_StartupGraphics()
    {
        cv_fullscreen.EV = cv_fullscreen.value && allow_fullscreen;
        SCR_apply_video_settings( 1 );
    }
    else
    {
        cv_fullscreen.EV = 0;
    }
}
