// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video_mac.c 1656 2023-12-08 14:54:47Z wesleyjohnson $
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
// DESCRIPTION:
//      DOOM graphics stuff for Mac
//
//-----------------------------------------------------------------------------

#include <AGL/agl.h>
#include <AGL/gl.h>
#include <AGL/glu.h>
#include <Carbon/Carbon.h>

#include "doomincl.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "m_menu.h"
#include "d_main.h"
#include "s_sound.h"
#include "g_input.h"
#include "st_stuff.h"
#include "g_game.h"
  // cv_fullscreen, cv_gamma etc.
#include "console.h"
#include "command.h"
#include "z_zone.h"
#include "hw_main.h"
#include "hw_drv.h"
#include "hwsym_mac.h"
  // For dynamic referencing of HW rendering functions
#include "r_opengl.h"


// public
byte  native_bitpp, native_bytepp;



struct modeDescription
{
    uint16_t  w, h;
    int freq;
};

RGBA_t  gamma_correction = {0x7F7F7F7F};


WindowRef mainWindow = NULL;

// all modes, used for window modes or fullscreen modes
#define MAXVIDMODES  33
#define MAX_LEN_VIDMODENAME  32
char   vidModeName[MAXVIDMODES][MAX_LEN_VIDMODENAME];
struct modeDescription modeList[MAXVIDMODES];
static int nummodes = 0;

#ifdef FIT_RATIO
#define MAXWINMODES (11)
#else
#define MAXWINMODES (8)
#endif
// windowed video modes from which to choose from.
static int windowedModes[MAXWINMODES+1][2] = {
   // hidden from display
    {INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT},  // initial mode
   // public  1..
    {MAXVIDWIDTH /*1600*/, MAXVIDHEIGHT/*1200*/},
#ifdef FIT_RATIO
    {1600, 1024},
#endif   
    {1280, 1024},
#ifdef FIT_RATIO
    {1280, 800},
#endif   
    {1024, 768},
#ifdef FIT_RATIO
    {1024, 600},
#endif   
    {800, 600},
    {640, 480},
    {512, 384},
    {400, 300},
    {320, 200}

#if 0
    { 320,  200},
    { 400,  300},
    { 512,  384},
    { 640,  480},
    { 800,  600},
    {1024,  768},
    {1280, 1024},
    {1600, 1200}
#endif
};

void I_UpdateNoBlit(void){}
void I_ReadScreen(byte* scr){}

void OglMacSetPalette(RGBA_t *palette, RGBA_t *gamma)
{
    int i;

    for (i=0; i<256; i++)
    {
        myPaletteData[i].s.red   = MIN((palette->s.red   * gamma->s.red)  /127, 255);
        myPaletteData[i].s.green = MIN((palette->s.green * gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((palette->s.blue  * gamma->s.blue) /127, 255);
        myPaletteData[i].s.alpha = 0xff; // opaque
        palette++;
    }

    Flush();
}

//
// I_SetPalette
//
void I_SetPalette (RGBA_t* palette)
{
    int i;

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((palette->s.red   * cv_grgammared.value)  /127, 255);
        myPaletteData[i].s.green = MIN((palette->s.green * cv_grgammagreen.value)/127, 255);
        myPaletteData[i].s.blue  = MIN((palette->s.blue  * cv_grgammablue.value) /127, 255);
        myPaletteData[i].s.alpha = 0xff; // opaque
        palette++;
    }

    Flush();

    return;
}

//------------------------------
//  VID_Pause
//  Used by macConfigureInput
//  Stops fullscreen mode to allow ISp dialog appear
//  newMode - new mode to switch to
//  returns - current mode (should switch back to this)
void VID_Pause(boolean pause)
{
    static int oldMode = -1;

    /*if (pause)
    {
        oldMode = vid.modenum;
        VID_SetMode (3);
    }
    else if (oldMode>0)
        {
        VID_SetMode(oldMode);
                oldMode = -1;
        }*/
}

// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };  // first is always 1
    mrange.last = nummodes;  // fullscreen and window
    return mrange;
}

//------------------------------

modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    // fullscreen and window modes  1..
    if( modenum.index < nummodes )
    {
        ms.width = modeList[modenum.index].w;
        ms.height = modeList[modenum.index].h;
        ms.type = MODE_either;
        ms.mark = "";
    }
    else
    {
        ms.type = MODE_NOP;
        ms.width = ms.height = 0;
        ms.mark = NULL;
    }
    return ms;
}

// VID_GetModeName
// Used in the video mode menu
char * VID_GetModeName( modenum_t modenum )
{
    snprintf(&vidModeName[modenum.index][0], MAX_LEN_VIDMODENAME, "%ix%i", modeList[modenum.index].w, modeList[modenum.index].h);
    vidModeName[modenum.index][MAX_LEN_VIDMODENAME-1] = 0;

    return &vidModeName[modenum.index][0];
}

// rmodetype is of modetype_e
// Returns MODE_NOP when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype )
{
    modenum_t  modenum = { MODE_NOP, 0 };
    int bestdist = INT_MAX;
    int best, tdist, i;

    best = 5;  // default is mode (640x480)

    if( nummodes == 0 )  goto done;
    for(i=1; i<nummodes; i++)   // skip INITIAL_WINDOW
    {
        tdist = abs(modeList[i]->w - rw) + abs(modeList[i]->h - rh);
        // find closest dist
        if( bestdist > tdist )
        {
            bestdist = tdist;
            best = i;
            if( tdist == 0 )  break;   // found exact match
        }
    }
    modenum.index = best;  // 1..
    modenum.modetype = rmodetype;
done:
    return modenum;
}


static
void VID_PrepareModeList(void)
{
    int i;

    for (i=0;i<MAXWINMODES;i++)
    {
        modeList[i].w = windowedModes[i][0];
        modeList[i].h = windowedModes[i][1];
        modeList[i].freq = 0;
    }

    nummodes = i;
}


//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    if( request_drawmode == DRM_opengl )
    {
        return true;
    }
    return false;
}


void SetDSpMode(int w, int h, boolean enable)
{
    static int lastw, lasth, last_enable = -1;

    if (mainWindow)
    {
        DisposeWindow(mainWindow);
        mainWindow = NULL;
    }

    if (enable)
    {
        HideCursor();
        HideMenuBar();
    }
    else
    {
        ShowCursor();
        ShowMenuBar();
        DrawMenuBar();
    }

    lastw = w;
    lasth = h;
    last_enable = enable;
}


// Returns FAIL_end, FAIL_create, of status_return_e, 1 on success;
int VID_SetMode(modenum_t modenum)
{
    boolean set_fullscreen = (modenum.modetype == MODE_fullscreen);

#if 0
    if( graphics_state == VGS_off )
        req_bitpp = 16;                     // quick hack as config hasn't been parsed
                                            // (don't want to assume 32 bit available)
    if( req_bitpp < 16 )
        req_bitpp = 16;                     // dont want 8-bit (?)
#else
    if ( graphics_state == VGS_off )
        cv_scr_depth.value = 16;            // quick hack as config hasn't been parsed
                                            // (don't want to assume 32 bit available)
    if (cv_scr_depth.value<16)
        CV_Set(&cv_scr_depth,"16");         // dont want 8-bit (?)
#endif

    vid.draw_ready = 0;  // disable print reaching console

    vid.bitpp = 32;
    vid.bytepp = 4;
    vid.width = modeList[modenum.index].w;
    vid.height = modeList[modenum.index].h;
    vid.widthbytes = vid.width * vid.bytepp;

    GenPrintf( EMSG_info, "VID_SetMode(%s,%i) %dx%d\n",
               modetype_string[modenum.modetype], modenum.index, vid.width, vid.height);

    // OpenGL only
    vid.direct_rowbytes = 0;
    vid.direct_size = 0;
    vid.ybytes = 0;
    vid.screen_size = 0;
    vid.display = NULL;
    vid.screen1 = NULL;

    SetDSpMode(vid.width, vid.height, set_fullscreen);

    if( rendermode = render_opengl )
    {
        // OpenGL only
        vid.direct_rowbytes = 0;
        vid.direct_size = 0;
        vid.ybytes = 0;
        vid.screen_size = 0;
        vid.display = NULL;
        vid.screen1 = NULL;
        OglMacSurface(&mainWindow, vid.width, vid.height, set_fullscreen);
    }
    else
    {
        // NOT FINISHED ???
    }

    vid.recalc = true;
    vid.modenum = modenum;
    vid.fullscreen = set_fullscreen;

    return 1;
}

int GetTextureMemoryUsed(void)
{
    return 0;
}

// NOT FINISHED
void I_FinishUpdate(void)
{
    if(rendermode==render_soft)
    {
        // NOT FINISHED
    }
    else
    {
        HWD.pfnFinishUpdate();
    }
}


// Initialize the graphics system, with a initial window.
void I_StartupGraphics( void )
{
    modenum_t  initialmode = {MODE_window,0};  // the initial mode
    // pre-init by V_Init_VideoControl

    graphics_state = VGS_startup;

    I_StartupMouse( false );

    native_drawmode = DRM_opengl;
    // do not assume 32 bit available
    native_bitpp = 32;
    native_bytepp = 4;

    VID_PrepareModeList();

    if( Set_VidMode( initialmode ) < 0 )   goto abort_error
   
    graphics_state = VGS_active;
    if( verbose )
        GenPrintf(EMSG_ver, "StartupGraphics completed\n" );
    return;

abort_error:
    // cannot return without a display screen
    I_Error("StartupGraphics Abort\n");
}


// Setup HWR calls according to rendermode.
int I_Rendermode_setup( void )
{
    HWD.pfnInit             = hwSym("Init");
    HWD.pfnFinishUpdate     = hwSym("FinishUpdate");
    HWD.pfnDraw2DLine       = hwSym("Draw2DLine");
    HWD.pfnDrawPolygon      = hwSym("DrawPolygon");
    HWD.pfnSetBlend         = hwSym("SetBlend");
    HWD.pfnClearBuffer      = hwSym("ClearBuffer");
    HWD.pfnSetTexture       = hwSym("SetTexture");
    HWD.pfnReadRect         = hwSym("ReadRect");
    HWD.pfnGClipRect        = hwSym("GClipRect");
    HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache");
    HWD.pfnSetSpecialState  = hwSym("SetSpecialState");
    HWD.pfnSetTransform     = hwSym("SetTransform");
    HWD.pfnDrawMD2          = hwSym("DrawMD2");
    HWD.pfnSetPalette       = OglMacSetPalette;
    HWD.pfnGetTextureUsed   = GetTextureMemoryUsed;
    return 1;
}


// Called to start rendering graphic screen according to the request switches.
// Fullscreen modes are possible.
// param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
// Returns FAIL_select, FAIL_end, FAIL_create, of status_return_e, 1 on success;
int I_RequestFullGraphics( byte select_fullscreen )
{
    modenum_t  initialmode;  // the initial mode
    byte  select_bitpp = 32;  // to select modes
    byte  select_fullscreen_mode;
    int  ret_value;

    // Seems to be OpenGL only.
    switch( req_drawmode )
    {
      case DRM_opengl:
#if 0
        select_bitpp = req_bitpp;
#else
        select_bitpp = cv_scr_depth.EV;
#endif
//        select_bitpp = native_bitpp;
        break;
#if 0
      case DRM_native;
        select_bitpp = native_bitpp;
        // NOT FINISHED
        break;
#endif
      default:
        goto no_modes;
    }

    I_Rendermode_setup();

    textureformatGL = GL_RGBA;
//    rendermode = render_opengl;  // rendermode is set by v_switch_drawmode

    if( nummodes == 0 )
        goto no_modes;
   
    vid.width = req_width;
    vid.height = req_height;

    select_fullscreen_mode = vid_mode_table[select_fullscreen];
    initialmode = VID_GetModeForSize( req_width, req_height, select_fullscreen_mode );
    ret_value = VID_SetMode( initialmode );
    if( ret_value < 0 )
        return ret_value;

    graphics_state = VGS_fullactive;

    if( verbose )
        GenPrintf(EMSG_ver, "RequestFullGraphics completed\n" );
    return ret_value;

no_modes:
    return FAIL_select;
}


void I_ShutdownGraphics(void)
{
    if( graphics_state <= VGS_shutdown )
        return;

    graphics_state = VGS_shutdown;  // to catch some repeats due to errors

    CONS_Printf("I_ShutdownGraphics\n");
    OglMacShutdown();
    DisposeWindow(mainWindow);
    ShowCursor();

    graphics_state = VGS_off;
}
