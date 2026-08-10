// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_video.c 1650 2023-10-25 02:25:41Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: I_video.c,v $
// Revision 1.5  2001/04/27 13:32:14  bpereira
// Revision 1.4  2000/11/02 19:49:38  bpereira
// Revision 1.3  2000/08/31 14:30:57  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      hardware and software level, screen and video i/o, refresh,
//      setup ... a big mess. Got to clean that up!
//
//-----------------------------------------------------------------------------

// Win95 or Emulation that may not have working clrscr.
#define ENABLE_CLRSCR	


#include "doomincl.h"
  // stdlib, strings, stdio, defines

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <sys/socket.h>

#include <netinet/in.h>
//#include <errnos.h>
#include <signal.h>

#include <go32.h>
#include <pc.h>
#include <dpmi.h>
#include <dos.h>
#include <sys/nearptr.h>

#include <allegro/gfx.h>

#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "i_video.h"
  // cv_fullscreen etc..

// djgpp
#include "Vid_vesa.h"




// This has the modelist in Vid_vesa.
static byte  mode_bitpp;  // bitpp of modelist

// Old method that is no longer used.
// Do not enable this, it ought to go away.
// #define BLAST_VESA

#ifdef BLAST_VESA
// unused
//dosstuff -newly added
// These are not set in the code, and will not work.
static byte * dascreen;
static byte  gfx_use_vesa1 = 0;
#endif

#define SCREENDEPTH   1     // bytes per pixel, do NOT change.

//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?
}

//
// I_OsPolling
//
void I_OsPolling()
{
    I_GetEvent();
    //i dont think i have to do anything else here
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}




//profile stuff ---------------------------------------------------------
//added:16-01-98:wanted to profile the VID_BlitLinearScreen() asm code.
//#define TIMING      //uncomment this to enable profiling
#ifdef TIMING
#include "p5prof.h"
static   long long mycount;
static   long long mytotal = 0;
static   unsigned long  nombre = TICRATE*10;
//static   char runtest[10][80];
#endif
//profile stuff ---------------------------------------------------------


//
// I_FinishUpdate
//
void I_BlitScreenVesa1(void);   //see later

void I_FinishUpdate (void)
{
#ifdef BLAST_VESA
   //blast it to the screen
   // this code sucks
   //memcpy(dascreen,screens[0],screenwidth*screenheight);
#endif

   //added:03-01-98: I tried to vsync here, but it slows down
   //  the game when the view becomes complicated, it looses ticks
   if( cv_vidwait.value )
   {
#if 0
       // Poll the CRTC status for VertRefresh
//       do {
//       } while (inportb(0x3DA) & 8);  // while VRI
       do {
       } while (!(inportb(0x3DA) & 8));  // while not VRI
#else
       vsync(); // allegro wait for vsync
#endif
   }


//added:16-01-98:profile screen blit.
#ifdef TIMING
    ProfZeroTimer();
#endif

#ifdef BLAST_VESA
    //added:08-01-98: support vesa1 bank change, without Allegro's BITMAP screen.
    if( gfx_use_vesa1 )
    {
        I_Error("Banked screen update not finished for dynamic res\n");
        //I_BlitScreenVesa1();    //blast virtual to physical screen.
    }
    else
#endif
    {
        // Standard Blit used by all ports.
        VID_BlitLinearScreen ( vid.buffer, vid.direct,
                               vid.widthbytes, vid.height,  // copy area
                               vid.ybytes, vid.direct_rowbytes );  // scanline inc
    }

#ifdef TIMING
    RDMSR(0x10,&mycount);
    mytotal += mycount;   //64bit add

    if(nombre--==0)
       I_Error("ScreenBlit CPU Spy reports: 0x%d %d\n", *((int*)&mytotal+1),
                                             (int)mytotal );
#endif

}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, vid.display, vid.screen_size);
}


void I_SetPalette (RGBA_t* palette)
{
    int i;

    outportb(0x3c8,0);
#if ( defined(DEBUG_WINDOWED) && defined(WIN32) )
    // Palette disable during debug, otherwise black text on black background
    for (i=17;i<256;i++,palette++)
#else
    for (i=0;i<256;i++,palette++)
#endif
    {
        outportb(0x3c9,palette->s.red>>2);
        outportb(0x3c9,palette->s.green>>2);
        outportb(0x3c9,palette->s.blue>>2);
    }
}

#ifdef BLAST_VESA
// Not used.
//added 29-12-1997
/*==========================================================================*/
// I_BlastScreen : copy the virtual screen buffer to the physical screen mem
//                 using bank switching if needed.
/*==========================================================================*/
void I_BlitScreenVesa1(void)
{
#define VIDBANKSIZE     (1<<16)
#define VIDBANKSIZEMASK (VIDBANKSIZE-1)   // defines ahoy!

  __dpmi_regs r;
  unsigned char *p_src;
  long     i;
  long     virtualsize;

   // virtual screen buffer size
   virtualsize = vid.direct_rowbytes * vid.height * SCREENDEPTH;

   p_src  = screens[0];

   for(i=0; virtualsize > 0; i++ )
   {
      r.x.ax = 0x4f05;
      r.x.bx = 0x0;
      r.x.cx = 0x0;
      r.x.dx = i;
      __dpmi_int(0x10,&r);      //set bank

      memcpy((byte *)dascreen,p_src,(virtualsize < VIDBANKSIZE) ? virtualsize : VIDBANKSIZE );

      p_src += VIDBANKSIZE;
      virtualsize -= VIDBANKSIZE;
   }

}
#endif


//added:08-01-98: now we use Allegro's set_gfx_mode, but we want to
//                restore the exact text mode that was before.
static short  myOldVideoMode;

void I_SaveOldVideoMode(void)
{
  __dpmi_regs r;
    r.x.ax = 0x4f03;                 // Return current video mode
    __dpmi_int(0x10,&r);
    if( r.x.ax != 0x4f )
        myOldVideoMode = -1;
    else
        myOldVideoMode = r.x.bx;
}


//
//  Close the screen, restore previous video mode.
//
void I_ShutdownGraphics (void)
{
    __dpmi_regs r;

    if( graphics_state <= VGS_shutdown )
        return;

    graphics_state = VGS_shutdown;  // to catch some repeats due to errors

    // free the last video mode screen buffers
    if (vid.buffer)
        free (vid.buffer);

    /* Restore old video mode */
    if(myOldVideoMode!=-1)
    {
       /* Restore old video mode */
       r.x.ax = 0x4f02;                 // Set Super VGA video mode
       r.x.bx = myOldVideoMode;
       __dpmi_int(0x10,&r);

#ifdef ENABLE_CLRSCR	
       // Boris: my s3 don't do a cls because "win95" :<
       clrscr();
#endif
    }
    else  // no vesa put the normal video mode
    {
       r.x.ax = 0x03;
       __dpmi_int(0x10,&r);
    }

    graphics_state = VGS_off;
}


#if 0
// Not used.
//added:08-01-98:
//  Set VESA1 video mode, coz Allegro set_gfx_mode a larger screenwidth...
//
int set_vesa1_mode( int width, int height )
{
    __dpmi_regs r;

    // setup video mode.
    r.x.ax = 0x4f02;
    if( ( width==320 )&&( height==200 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x13;                             // 320x 200x1 (256 colors)
    else
    if( ( width==320 )&&( height==240 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x154;                            // 320x 240x1 (256 colors)
    else
    if( ( width==320 )&&( height==400 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x155;                            // 320x 400x1 (256 colors)
    else
    if( ( width==640 )&&( height==400 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x100;                            // 640x 400x1 (256 colors)
    else
    if( ( width==640 )&&( height==480 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x101;                            // 640x 480x1 (256 colors)
    else
    if( ( width==800 )&&( height==600 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x103;                            // 800x 600x1 (256 colors)
    else
    if( ( width==1024)&&( height==768 ) && ( SCREENDEPTH==1 ) )
       r.x.bx   = 0x105;                            //1024x 768x1 (256 colors)
    else
       I_Error("I_SetVesa1Mode: video mode not supported.");

    // enter graphics mode.
    __dpmi_int(0x10,&r);

    if( r.x.ax != 0x4f )
       I_Error("I_SetVesa1Mode: init video mode failed !");

    return 0;
}
#endif


//added:08-01-98: now uses Allegro to setup Linear Frame Buffer video modes.
//
//  Initialize video mode, setup dynamic screen size variables,
//  and allocate screens.
//

// Initialize the graphics system, with a initial window.
void I_StartupGraphics( void )
{
    modenum_t initial_mode = {MODE_window, 0};
    // pre-init by V_Init_VideoControl

    graphics_state = VGS_startup;
    // remember the exact screen mode we were...
    I_SaveOldVideoMode();
   
    // Do not know current settings, so use this default.
    native_drawmode = DRM_native;
    native_bitpp = 8;
    native_bytepp = 1;
   
    //added:26-01-98: VID_Init() must be done only once,
    //                use VID_SetMode() to change vid mode while in the game.
    GenPrintf( EMSG_info, "Vid_Init...");
    VID_Init();

#ifdef BLAST_VESA
    //gfx_use_vesa1 = false;
#endif

    //added:03-01-98: register exit code for graphics
    I_AddExitFunc(I_ShutdownGraphics);

    // set the startup window
    VID_InitVGAModes();
    if( VID_SetMode ( initial_mode ) < 0 )
    {
        initial_mode.index = 1;  // 320
        if( VID_SetMode ( initial_mode ) < 0 )  goto abort_error;
    };

    graphics_state = VGS_active;
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
    byte select_bitpp = 0;
    int ret_value;
    modenum_t initial_mode = {MODE_window, 0};
    byte select_fullscreen_mode;

    switch(req_drawmode)
    {
     case DRM_native:
       if( V_CanDraw( native_bitpp )) {
           select_bitpp = native_bitpp;
       }else{
           // Use 8 bit and do the palette lookup.
           GenPrintf(EMSG_ver, "Native %i bpp rejected\n", native_bitpp );
           select_bitpp = 8;
       }
       break;
     case DRM_explicit_bpp:
       select_bitpp = req_bitpp;
       break;
     default:
       goto no_modes;
       break;
    }
   
    ret_value = VID_GetModes( req_drawmode, select_bitpp );
    if( ret_value < 0 )
        return ret_value;

    // DOS did not have windows, but this may run under Windows.
    if( num_full_vidmodes == 0 )
        goto no_modes;
   
    vid.width = req_width;
    vid.height = req_height;

    // set the startup screen
    select_fullscreen_mode = vid_mode_table[select_fullscreen];
    initial_mode = VID_GetModeForSize( req_width, req_height, select_fullscreen_mode );
    ret_value = VID_SetMode ( initial_mode );
    if( ret_value < 0 )
        return ret_value;

    graphics_state = VGS_fullactive;
    return ret_value;  // have video mode

no_modes:
    return FAIL_select;
}

// Setup HWR calls according to rendermode.
int I_Rendermode_setup( void )
{
    return 1;
}


#if 0
// See VID_Query_Modelist in Vid_vesa.c

//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    int ret_value;
    byte  old_loaded_driver = loaded_driver; // must put this back

    // Require modelist before rendermode is set.
   
    // if '-win' is specified on the command line, do not add DirectDraw modes
    req_win = M_CheckParm ("-win");

    if( req_win || ! request_fullscreen )
        return true;  // uses window modes
   
    // Fullscreen modes
    // This may change the video driver.
    ret_value = VID_load_driver( request_drawmode );
    if( ret_value < 0 )
        return ret_value;

    ret_value = VID_GetModes( request_drawmode, request_bitpp );
   
    if( loaded_driver != old_loaded_driver )
    {
        // restore the driver
        VID_load_driver( old_loaded_driver );
    }

    if( ret_value < 0 )
        return false; 
    
    return ( num_full_vidmodes > 0 );
}
#endif


// for debuging
void IO_Color( byte color, byte r, byte g, byte b )
{
  outportb( 0x03c8 , color );              // registre couleur
  outportb( 0x03c9 , (r>>2) & 0x3f );     // R
  outportb( 0x03c9 , (g>>2) & 0x3f );     // G
  outportb( 0x03c9 , (b>>2) & 0x3f );     // B
}
