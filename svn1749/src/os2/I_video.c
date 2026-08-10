// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_video.c 1656 2023-12-08 14:54:47Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 2000-2016 by DooM Legacy Team.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log: I_video.c,v $
// Revision 1.8  2004/05/16 19:11:53  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.7  2001/04/16 22:59:25  ydario
// Revision 1.6  2001/03/03 19:29:44  ydario
// Revision 1.5  2000/11/02 19:49:40  bpereira
//
// Revision 1.4  2000/08/16 16:31:25  ydario
// Give more timeslice to other threads
//
// Revision 1.3  2000/08/10 11:07:51  ydario
// Revision 1.2  2000/08/10 09:19:31  ydario
// Revision 1.1  2000/08/09 12:15:09  ydario
// OS/2 specific platform code
//
//
// DESCRIPTION:
//      DOOM graphics stuff for OS2.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: I_video.c 1656 2023-12-08 14:54:47Z wesleyjohnson $";

#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <signal.h>

#include "I_os2.h"

//#include "mgraph.h"

#include "doomincl.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
  // vid_mode_table etc.
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"



#ifdef FIT_RATIO
#define MAXWINMODES (11)
#else
#define MAXWINMODES (8)
#endif
#define MAX_LEN_VIDMODENAME  16
static char vidModeName[MAXWINMODES][MAX_LEN_VIDMODENAME];
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
   {1600, 1200},
#endif
};

static byte  mode_bitpp; // bitpp of mode tables.


//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
      // blit directly if BlitThread is not running.
      // Blit the image using DiveBlit
    if (!pmData->fDataInProcess) {
        DiveBlitImage( pmData->hDive, pmData->ulImage, DIVE_BUFFER_SCREEN);
    }
    DosSleep(0);

/*
      // Use secondary blitting thread

      // blitted previous image?
    if (pmData->fBlitReady == TRUE)
      return; // no, try again

      // data is ready for blitting
    memcpy( pmData->pbBuffer2, pmData->pbBuffer, vid.direct_size);
    pmData->fBlitReady = TRUE;
*/
}


//
// This is meant to be called only by CONS_Printf() while game startup
//
// printf to loading screen
void I_LoadingScreen ( const char * msg )
{
    HPS    hps;
    RECTL  rect;

    if ( msg ) {

        hps = WinGetPS( pmData->hwndClient);
        WinQueryWindowRect( pmData->hwndClient, &rect);
        WinFillRect(hps, &rect, CLR_WHITE);
        WinDrawText( hps, strlen( msg), msg, &rect,
                     0, 0,
                     DT_WORDBREAK | DT_TOP | DT_LEFT | DT_TEXTATTRS);
    }
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, vid.display, vid.screen_size);
}

//
// I_SetPalette
//
void I_SetPalette (RGBA_t* palette)
{
   int   i, r, g, b;
   long  colors[ 256];

      // set the X colormap entries
   for (i=0 ; i<256 ; i++,palette++) {
      r = palette->s.red;
      g = palette->s.green;
      b = palette->s.blue;
      colors[i] = (r<<16) + (g<<8) + b; //(PC_RESERVED * 16777216) +
   }
      // set dive palette
   DiveSetSourcePalette( pmData->hDive, 0,
                         pmData->ulNumColors,
                         (PBYTE) colors);
}

//
//  Close the screen, restore previous video mode.
//
void I_ShutdownGraphics(void)
{
   if( graphics_state <= VGS_shutdown )
       return;

   graphics_state = VGS_shutdown;  // to catch some repeats due to errors

   printf( "I_ShutdownGraphics\n");

   ShutdownDIVE( pmData);

   graphics_state = VGS_off;
}

//
//  Initialize video mode, setup dynamic screen size variables,
//  and allocate screens.
//
void I_StartupGraphics(void)
{
    modenum_t  initialmode = {MODE_window,0};  // the initial mode
    // pre-init by V_Init_VideoControl

    graphics_state = VGS_startup;

    if (M_CheckParm( "-mgl"))
    {
#if 0
        if (!MGL_init("..\\..\\..\\", NULL))
                MGL_fatalError("MGL init failed");
        MGL_enableAllDrivers();
            //if ((mglMode = MGL_findMode(SCREENWIDTH, SCREENHEIGHT, 8)) == -1)
            //  MGL_fatalError("Graphics mode not found");
#endif
    } else {
        InitDIVE( pmData);
    }

    // This is an old driver that only handles 8 bit palette mode.
    native_drawmode = DRM_8pal;
    native_bitpp = 8;
    native_bytepp = 1;

    //added:03-01-98: register exit code for graphics
    I_AddExitFunc(I_ShutdownGraphics);
    graphics_state = VGS_active;

    // Has fixed vidmode list
    // set the default video mode
    if( VID_SetMode(0) < 0 )  goto abort_error;
   
    if( verbose )
        GenPrintf(EMSG_ver, "StartupGraphics completed\n" );
    return;

abort_error:
    // cannot return without a display screen
    I_Error("StartupGraphics Abort\n");
}

// Called to start rendering graphic screen according to the request switches.
// Fullscreen modes are possible.
// param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
// Returns FAIL_select, FAIL_end, FAIL_create, of status_return_e, 1 on success;
int I_RequestFullGraphics( byte select_fullscreen )
{
    int ret_value;
    modenum_t  initialmode;
    byte  select_fullscreen_mode = vid_mode_table[select_fullscreen];

    // This is an old driver that only handles 8 bit palette mode.
    if( req_bitpp != native_bitpp )  goto no_modes;
   
    mode_bitpp = native_bitpp;
    // Modes are fixed, so no need to get them.
    vid.width = req_width;
    vid.height = req_height;

    initialmode = VID_GetModeForSize( req_width, req_height, select_fullscreen_mode );
    ret_value = VID_SetMode( initialmode );
    if( ret_value < 0 )
        return ret_value;

    vid.recalc = true;
    graphics_state = VGS_fullactive;

    if( verbose )
        GenPrintf(EMSG_ver, "StartupGraphics completed\n" );
    return ret_value;

no_modes:
    return FAIL_select;
}

// Setup HWR calls according to rendermode.
int I_Rendermode_setup( void )
{
    return 1;
}


// return number of fullscreen or window modes, for listing
// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };  // first is always 1
    mrange.last = MAXWINMODES;
    return mrange;
}

// rmodetype is of modetype_e
// Returns MODE_NOP when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype )
{
    modenum_t  modenum = { MODE_NOP, 0 };
    int bestdist = INT_MAX;
    int best, tdist, i;
    
    // fullscreen and windowed modes, 1..
    best = 2;  // default
    for (i = 0; i <= MAXWINMODES; i++)
    {
        tdist = abs(windowedModes[i][0] - rw) + abs(windowedModes[i][1] - rh);
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

modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    // fullscreen and window modes  1..
    if( modenum.index <= MAXWINMODES )  // inclusive
    {
        ms.width = windowedModes[modenum.index][0];
        ms.height = windowedModes[modenum.index][1]);
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


//added:30-01-98:return the name of a video mode
char * VID_GetModeName( modenum_t modenum )
{
   int mi = modenum.index;
   snprintf( vidModeName[mi], MAX_LEN_VIDMODENAME, "%dx%d",
            windowedModes[mi][0],
            windowedModes[mi][1]);
   vidModeName[mi][MAX_LEN_VIDMODENAME-1] = 0;
   return vidModeName[mi];
}


//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    if( request_drawmode == DRM_explicit_bpp )
    {
        // Fixed modes, 8 bit palette only.
        if( request_bitpp == 8 )  return true;
    }
    else if( request_drawmode == DRM_native )
        return true;

    return false;
}


// ========================================================================
// Sets a video mode
// ========================================================================
// Returns FAIL_end, FAIL_create, of status_return_e, 1 on success;
int VID_SetMode( modenum_t modenum )
{
    boolean set_fullscreen = (modenum.modetype == MODE_fullscreen);
   
   // Only handles 8 bit palette mode.

   vid.draw_ready = 0;  // disable print reaching console

   if (modenum.index > MAXWINMODES) {
       GenPrintf( EMSG_error, "VID_SetMode modenum %i >= MAXWINMODES\n", modenum.index);
       return FAIL_end;
   }
/*
   if (pmData->pbBuffer) { // init code only once
       printf("VID_SetMode already called\n");
       return -1;
   }
*/
   // initialize vidbuffer size for setmode
   vid.width  = windowedModes[modenum.index][0];
   vid.height = windowedModes[modenum.index][1];
   //vid.aspect = pcurrentmode->aspect;
   GenPrintf( EMSG_info, "Setting mode: %dx%d\n", vid.width, vid.height);

   // adjust window size
   pmData->ulWidth = vid.width;
   pmData->ulHeight = vid.height;
   WinPostMsg( pmData->hwndClient, WM_COMMAND, (MPARAM) ID_NEWTEXT, NULL);
   WinPostMsg( pmData->hwndClient, WM_COMMAND, (MPARAM) ID_SNAP, NULL);

   //if (pmData->pbBuffer)
   //    ShutdownDIVE( pmData);
   //pmData->pbBuffer = 0;
   InitDIVEBuffer( pmData);

   pmData->currentImage = 0;
   pmData->fDataInProcess = TRUE;
   vid.buffer = (byte*) pmData->pbBuffer; //;//

   //added:20-01-98: recalc all tables and realloc buffers based on
   //                vid values.
   vid.recalc = 1;
   vid.bytepp = 1;
   vid.bitpp = 8;
   vid.drawmode = DRAW8PAL;
   vid.widthbytes = vid.width;
   vid.ybytes = vid.direct_rowbytes = vid.width;
   vid.screen_size = vid.direct_size = vid.width * vid.height;
   vid.display = vid.buffer;
   vid.screen1 = vid.buffer + vid.screen_size;
   vid.modenum  = modenum;

   return 1;
}
