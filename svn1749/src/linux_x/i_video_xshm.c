// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video_xshm.c 1660 2023-12-09 23:55:48Z wesleyjohnson $
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
// $Log: i_video_xshm.c,v $
// Revision 1.32  2004/05/16 19:11:53  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.31  2004/05/13 11:09:38  andyp
// Removed extern int errno references for Linux
//
// Revision 1.30  2002/07/01 19:59:59  metzgermeister
// *** empty log message ***
//
// Revision 1.29  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
//
// Revision 1.28  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.27  2001/04/27 13:32:14  bpereira
// no message
//
// Revision 1.26  2001/04/14 14:17:52  metzgermeister
// clean up
//
// Revision 1.25  2001/03/30 17:12:52  bpereira
// no message
//
// Revision 1.24  2001/03/12 21:03:10  metzgermeister
//   * new symbols for rendererlib added in SDL
//   * console printout fixed for Linux&SDL
//   * Crash fixed in Linux SW renderer initialization
//
// Revision 1.23  2001/02/24 13:35:22  bpereira
// no message
//
// Revision 1.22  2001/02/19 23:55:26  hurdler
// Update to match win32 source code
//
// Revision 1.21  2001/02/13 20:37:27  metzgermeister
// *** empty log message ***
//
// Revision 1.20  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.19  2000/11/04 16:23:45  bpereira
// no message
//
// Revision 1.18  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.17  2000/10/08 13:30:02  bpereira
// no message
//
// Revision 1.16  2000/09/10 10:51:02  metzgermeister
// added vid_wait
//
// Revision 1.15  2000/08/21 21:14:31  metzgermeister
// Voodoo3/Banshee fix
//
// Revision 1.14  2000/08/11 19:11:07  metzgermeister
// *** empty log message ***
//
// Revision 1.12  2000/05/13 19:53:54  metzgermeister
// voodoo fullscreen & remote display
//
// Revision 1.11  2000/05/07 08:29:03  metzgermeister
// added windowed voodoo mode
//
// Revision 1.10  2000/04/22 20:27:35  metzgermeister
// support for immediate fullscreen switching
//
// Revision 1.9  2000/04/18 23:47:00  metzgermeister
// bugfixes
//
// Revision 1.8  2000/04/16 20:10:51  metzgermeister
// screensaver handling
//
// Revision 1.7  2000/04/12 19:32:50  metzgermeister
// new Voodoo and fullscreen support
//
// Revision 1.6  2000/04/07 23:10:15  metzgermeister
// fullscreen support under X in Linux
//
// Revision 1.5  2000/03/23 22:40:49  metzgermeister
// added support for automatic 3dfx fullscreen
//
// Revision 1.4  2000/03/07 03:30:49  hurdler
// fix linux compilation
//
// Revision 1.3  2000/03/06 15:19:58  hurdler
// Add Bell Kin's changes
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
  // stdlib, stdio, defines

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/XShm.h>
#include <X11/extensions/xf86vmode.h>

#ifdef WITH_DGA
#include <X11/extensions/xf86dga.h>
#endif

#define USE_XKB

#ifdef USE_XKB
// XKeycodeToKeysym is deprecated, refered to use XkbKeycodeToKeysym
#include <X11/XKBlib.h>
#endif

// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the X headers.
//#if defined(LINUX)
int XShmGetEventBase( Display* dpy );
//#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dlfcn.h> // added 19990831 by Kin
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#if defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
#include "strcmp.h"
#endif

// unknown flag DONTDEFINEBOOL, it is not in any DoomLegacy code
#define DONTDEFINEBOOL
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
  // vid_mode_table
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "d_main.h"
#include "s_sound.h"
// added for 1.27 19990220 by Kin
#include "g_input.h"
#include "st_stuff.h"
#include "g_game.h"
#include "i_video.h"
#include "screen.h"
  // DRAW8PAL
#include "hardware/hw_main.h"
#include "hardware/hw_drv.h"
#include "hardware/hw_glob.h"
#include "console.h"
#include "command.h"
#include "d_clisrv.h"
  // for dedicated
#include "r_data.h"
  // R_Init_color8_translate, color8



// Voodoo card has video switch, produces fullscreen 3d graphics,
// and we cannot use window mode with it.
static boolean haveVoodoo = false;  // have Voodoo card in hardware mode
static boolean showkey=false; // force to no 19990118 by Kin

static Display*        X_display=NULL;
static Window          X_mainWindow=0;
static Colormap        X_cmap;
static XVisualInfo     X_visualinfo;
static Visual*         X_visual;
static GC              X_gc;
static XEvent          X_event;
static int             X_screen;
static XSizeHints      X_size;
static XWMHints        X_wm;
static XClassHint      X_class;
static Atom            X_wm_delwin;
static XImage*         image = NULL;

static Window   dummy;
static int      dont_care;
static unsigned int      dont_care_ui;

static boolean  localDisplay = true;

// MIT SHared Memory extension.
static boolean  doShm;

static XShmSegmentInfo  X_shminfo;
static int      X_shmeventtype;

// Mouse handling.
static boolean  Mousegrabbed = false;

// X visual mode
static byte     x_drawmode=DRAW8PAL;
static int      x_bitpp=8;
static int      x_bytepp=1;
static uint16_t* x_colormap2 = 0;
static unsigned char* x_colormap3 = 0;
static unsigned long* x_colormap4 = 0;
static unsigned long x_red_mask = 0;
static unsigned long x_green_mask = 0;
static unsigned long x_blue_mask = 0;
static unsigned char x_red_offset = 0;
static unsigned char x_green_offset = 0;
static unsigned char x_blue_offset = 0;

// X11 video modes from which to choose from.

// maximum number of windowed modes for X11 (see windowedModes[][])
#ifdef FIT_RATIO
#define MAXWINMODES (11)
#else
#define MAXWINMODES (8)
#endif
static int windowedModes[MAXWINMODES+1][2] = {
   // hidden from display
   {INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT},  // initial mode
   // public  1..
   {1600, 1200},
#ifdef FIT_RATIO
   {1600, 1024},
#endif   
   {1280, 1024},
#ifdef FIT_RATIO
   {1280, 800},
#endif   
   {1024,  768},
#ifdef FIT_RATIO
   {1024, 600},
#endif   
   { 800,  600},
   { 640,  480},
   { 512,  384},
   { 400,  300},
   { 320,  200}
};

#define NUM_VOODOOMODES (3)
// These are modes for 3dfx voodoo graphics (loopthrough) cards
// 1..
static int voodooModes[NUM_VOODOOMODES][2] = {
    {800, 600},
    {640, 480},
    {512, 384}
};

static boolean vidmode_ext; // Are videmode extensions available?
static boolean vidmode_active = false;
// List of vidmodes from XF86. vidmap will index into this list.
// Alloc by XF86, free by caller.
static XF86VidModeModeInfo ** fullvidmodes = NULL;
static int num_fullvidmodes = 0;  // num in vidmodes

// [WDJ] Submitted by pld-linux patch lib.  Handle larger number of vidmodes without crash.
#define MAX_NUM_VIDMODES (100)
#define MAX_LEN_VIDMODENAME    32
// [WDJ] Submitted by pld-linux patch lib.  Handle larger number of vidmodes without crash.
// allow MAX_NUM_VIDMODES different modes
static char vidModeName[MAX_NUM_VIDMODES][MAX_LEN_VIDMODENAME+1];
static int num_vidmodes = 0;      // num in vidmap, limited to MAX_NUM_VIDMODES
static int vidmap[MAX_NUM_VIDMODES];

// sort vidmodes into vidmap
// excluding those too large for our limits
static void vidmodes_to_vidmap( void )
{
    int i, j;
   
    // initialize mapping
    // exclude modes which are too large (to prevent X-Server problems)
    num_vidmodes = 0;
    for(i=0; i<num_fullvidmodes; i++)
    {
        XF86VidModeModeInfo * vm = fullvidmodes[i];
        if(vm->hdisplay <= MAXVIDWIDTH
           && vm->vdisplay <= MAXVIDHEIGHT)
        {
            // only init vidmap when mode sizes are within our limits
            // Insert sort, largest first.
            unsigned int vs1 = vm->hdisplay * vm->vdisplay;
            for(j=0; j<num_vidmodes; j++)
            {
                XF86VidModeModeInfo * vm2 = fullvidmodes[ vidmap[j] ];
                unsigned int vs2 = vm2->hdisplay * vm2->vdisplay;
                // Largest first in the vidmap.
                if( vs1 > vs2 )
                {
                    memmove( &vidmap[j+1], &vidmap[j], (num_vidmodes - j) * sizeof(int) );
                    break;
                }
            }
            vidmap[j] = i;
            num_vidmodes++;

            // limit num of modes at usage of index
            if( num_vidmodes >= MAX_NUM_VIDMODES )
                break;
        }
    }
}

// Set hardware vidmodes and vidmap, from X_display, X_screen
static void determine_VidModes(void)
{
    if(vidmode_ext)
    {
        // Only get this once, to preserve the orig vidmode.
        if( fullvidmodes == NULL )
        {
            // get fullscreen modes to vidmodes
            XF86VidModeGetAllModeLines(X_display, X_screen, &num_fullvidmodes, &fullvidmodes);
              // vidmodes is allocated, and should be freed by caller.
              // The first element is the current mode.
        }

        vidmodes_to_vidmap();
    }
    else
    {
        num_vidmodes = 0;
    }
    return;
}


// Set vidmode_ext
static void check_vidmode_extension(void)
{
   int MajorVersion, MinorVersion;

   MajorVersion = MinorVersion = 0;

   // disable extensions for non-local displays
   if(!localDisplay){
       vidmode_ext = false;
       return;
   }

   if (!XF86VidModeQueryVersion(X_display, &MajorVersion, &MinorVersion)) {
      vidmode_ext = false;
   } else {
      CONS_Printf("Using XFree86-VidModeExtension Version %d.%d\n", MajorVersion, MinorVersion);
      vidmode_ext = true;
   }
   return;
}


// Set X_screen, X_visualinfo, X_visual, x_drawmode
static void findVisual(void)
{
   // classes of interest are PseudoColor (dynamic colormap), TrueColor (static colormap)
   X_screen = DefaultScreen(X_display); // screen number, usually 0
   if (XMatchVisualInfo(X_display, X_screen, 8, PseudoColor, &X_visualinfo))
      { x_drawmode = DRAW8PAL; }
   else if
      (XMatchVisualInfo(X_display, X_screen, 15, TrueColor, &X_visualinfo))
      { x_drawmode = DRAW15; }
   else if
      (XMatchVisualInfo(X_display, X_screen, 16, TrueColor, &X_visualinfo))
      { x_drawmode = DRAW16; }
   else if
      (XMatchVisualInfo(X_display, X_screen, 24, TrueColor, &X_visualinfo))
      { x_drawmode = DRAW24; }
   else if
      (XMatchVisualInfo(X_display, X_screen, 32, TrueColor, &X_visualinfo))
      { x_drawmode = DRAW32; }
   else
      I_Error("no supported visual found\n");
   X_visual = X_visualinfo.visual;

   return;
}

// Set color offset and masks from X_visual and x_drawmode
static void determineColorMask(void)
{
   x_red_mask = X_visual->red_mask;
   x_green_mask = X_visual->green_mask;
   x_blue_mask = X_visual->blue_mask;

   if (x_drawmode==DRAW24 || x_drawmode==DRAW32) {
      switch (x_red_mask) {
#ifdef BIGEND
      case 0x000000ff: x_red_offset = 3; break;
      case 0x0000ff00: x_red_offset = 2; break;
      case 0x00ff0000: x_red_offset = 1; break;
      case 0xff000000: x_red_offset = 0; break;
#else
      case 0x000000ff: x_red_offset = 0; break;
      case 0x0000ff00: x_red_offset = 1; break;
      case 0x00ff0000: x_red_offset = 2; break;
      case 0xff000000: x_red_offset = 3; break;
#endif
      }
      switch (x_green_mask) {
#ifdef BIGEND
      case 0x000000ff: x_green_offset = 3; break;
      case 0x0000ff00: x_green_offset = 2; break;
      case 0x00ff0000: x_green_offset = 1; break;
      case 0xff000000: x_green_offset = 0; break;
#else
      case 0x000000ff: x_green_offset = 0; break;
      case 0x0000ff00: x_green_offset = 1; break;
      case 0x00ff0000: x_green_offset = 2; break;
      case 0xff000000: x_green_offset = 3; break;
#endif
      }
      switch (x_blue_mask) {
#ifdef BIGEND
      case 0x000000ff: x_blue_offset = 3; break;
      case 0x0000ff00: x_blue_offset = 2; break;
      case 0x00ff0000: x_blue_offset = 1; break;
      case 0xff000000: x_blue_offset = 0; break;
#else
      case 0x000000ff: x_blue_offset = 0; break;
      case 0x0000ff00: x_blue_offset = 1; break;
      case 0x00ff0000: x_blue_offset = 2; break;
      case 0xff000000: x_blue_offset = 3; break;
#endif
      }
   }
   if (x_drawmode==DRAW15 || x_drawmode==DRAW16) {
      // for 16bpp, x_*_offset specifies the number of bits to shift
      unsigned long mask;

      mask = x_red_mask;
      x_red_offset = 0;
      while (!(mask&1)) {
         x_red_offset++;
         mask >>= 1;
      }
      x_red_mask = 8;
      while (mask&1) {
         x_red_mask--;
         mask >>= 1;
      }

      mask = x_green_mask;
      x_green_offset = 0;
      while (!(mask&1)) {
         x_green_offset++;
         mask >>= 1;
      }
      x_green_mask = 8;
      while (mask&1) {
         x_green_mask--;
         mask >>= 1;
      }

      mask = x_blue_mask;
      x_blue_offset = 0;
      while (!(mask&1)) {
         x_blue_offset++;
         mask >>= 1;
      }
      x_blue_mask = 8;
      while (mask&1) {
         x_blue_mask--;
         mask >>= 1;
      }
   }
   return;
}

// Sets x_bitpp and x_bytepp from X_visualinfo.depth
static void determineBPP(void)
{
   int pixmap_count;
   XPixmapFormatValues* X_pixmapformats;  // temp

   X_pixmapformats = XListPixmapFormats(X_display, &pixmap_count);

   // valid depth are 1, 4, 8, 15, 16, 24, 32
   if (X_pixmapformats) {
      int i;
      x_bitpp=0;
      // search pixmapformats for the depth
      for (i=0; i<pixmap_count; i++) {
         if (X_pixmapformats[i].depth == X_visualinfo.depth) {
            x_bitpp = X_pixmapformats[i].bits_per_pixel;
            break;
         }
      }
      if (x_bitpp==0)
         I_Error("Could not determine bits_per_pixel\n");

      XFree(X_pixmapformats);
   } else
      I_Error("Could not get list of pixmap formats\n");

   x_bytepp = (x_bitpp+7)/8;
   if( verbose )
      GenPrintf(EMSG_ver, "Video depth %i, x_bitpp %i, x_bytepp %i\n", X_visualinfo.depth, x_bitpp, x_bytepp );
   return;
}


int X_error_handler( Display * d, XErrorEvent * ev )
{
#define ERRBUF_SIZE    1024
    char errbuf[ERRBUF_SIZE+1];
    XGetErrorText( d, ev->error_code, errbuf, ERRBUF_SIZE );
    I_SoftError( "%s\n", errbuf );
    return 0;
}


// Set X_display
static char * initDisplay(void)
{
    int pnum;
    char *displayname, *d, displaycopy[256];

    // check for command-line display name
    if((pnum = M_CheckParm("-display")) && (pnum < myargc-1))
        displayname = myargv[pnum+1];
    else
        displayname = NULL;

    // open the display
    X_display = XOpenDisplay(displayname);

    if (!X_display)
    {
        if (displayname)
            I_Error("Could not open display [%s]\n", displayname);
        else
            I_Error("Could not open display (DISPLAY=[%s])\n", getenv("DISPLAY"));
    }

    if(!displayname)
        displayname = (char *) getenv("DISPLAY");

    // check for local display
    if(displayname) {
        dl_strncpy(displaycopy, displayname, 256);
        d = displaycopy;
        while (*d && (*d != ':')) d++;
        if (*d) *d = 0;
        if (!(!strcasecmp(displaycopy, "unix") || !*displaycopy))
            localDisplay = false;
    }

    return displayname;
}

// Set doShm
static void checkForShm( void )
{
    if(rendermode==render_soft) {
        // check for the MITSHM extension
        doShm = XShmQueryExtension(X_display);

        // even if it's available, make sure it's a local connection
        if(doShm && localDisplay) {
            doShm = true;
            GenPrintf(EMSG_info, "Using MITSHM extension\n");
        }
        else
            doShm = false;
    } else //if(rendermode==render_soft)
        doShm = false;

    return;
}

// Create X_cmap and x_colormap2/3/4, from x_drawnmode, x_bytepp
static void createColorMap()
{
   if (x_drawmode == DRAW8PAL)
   {
      X_cmap = XCreateColormap(X_display, RootWindow(X_display, X_screen),
                               X_visual, AllocAll);
   }
   else if (x_bytepp==2)
   {
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
      // [WDJ] Cannot work unless table is init too
      vid.drawmode = (x_bitpp==15)? DRAW15 : DRAW16;
      vid.bitpp = x_bitpp;
      R_Init_color8_translate ( 0 );
      vid.bitpp = 8; // all draw is 8 bpp
      x_colormap2 = &color8.to16[0]; // cheat...19990119 by Kin
#else
      x_colormap2 = malloc(2*256);
#endif
   }
   else if (x_bytepp==3)
      x_colormap3 = malloc(3*256);
   else if (x_bytepp==4)
      x_colormap4 = malloc(4*256);
   return;
}

#ifdef ALT_KEYMAPPING
int con_keymap = 0;

static int alt_keyboard_MapTable[256] =
{0, 0, 0, 0, 0, 0, 0, 0, 0,
 XK_Escape, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', XK_BackSpace,
 XK_Tab, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', XK_Return,
 XK_Control_L, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, 96,
 XK_Shift_L, 92, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', XK_Shift_R,
 XK_KP_Multiply,
 XK_Meta_L, 32,
 XK_Caps_Lock,
 XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8, XK_F9, XK_F10,
 XK_Num_Lock, XK_Scroll_Lock, XK_KP_Home, XK_KP_Up, XK_KP_Prior, XK_KP_Subtract, XK_KP_Left, XK_KP_Begin, XK_KP_Right, XK_KP_Add,
 XK_KP_End, XK_KP_Down, XK_KP_Page_Down, XK_KP_Insert, XK_KP_Delete, 0, 0, 0,
 XK_F11, XK_F12, XK_Home, XK_Up, XK_Prior,
 XK_Left, 0, XK_Right, XK_End, XK_Down, XK_Next, XK_Insert, XK_Delete,
 XK_KP_Enter, XK_Multi_key, XK_Pause, XK_Print,
 XK_KP_Divide, XK_Alt_R, 0, XK_Super_L, XK_Super_R, XK_Hyper_R,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

//
//  Translates the key currently in X_event
//
static int xlatekey( KeyCode keycode, byte keydown )
{
    KeySym keysym;
    int rc;

#ifdef ALT_KEYMAPPING
    if( con_keymap ) {
        rc = alt_keyboard_MapTable[keycode];
    }
    else {
        // X keymapping
#ifdef USE_XKB
        keysym = XkbKeycodeToKeysym(X_display, keycode, 0, 0);
#else       
        keysym = XKeycodeToKeysym(X_display, keycode, 0);
#endif
        rc = keysym;
    }
#else
    // X keymapping
#ifdef USE_XKB
    keysym = XkbKeycodeToKeysym(X_display, keycode, 0, 0);
#else       
    keysym = XKeycodeToKeysym(X_display, keycode, 0);
#endif
    rc = keysym;
#endif

    switch(rc)
    {
      case XK_Left  : rc = KEY_LEFTARROW;     break;
      case XK_Right : rc = KEY_RIGHTARROW;    break;
      case XK_Down  : rc = KEY_DOWNARROW;     break;
      case XK_Up    : rc = KEY_UPARROW;       break;

      case XK_Escape:   rc = KEY_ESCAPE;        break;
      case XK_Return:   rc = KEY_ENTER;         break;
      case XK_Tab:      rc = KEY_TAB;           break;
      case XK_F1:       rc = KEY_F1;            break;
      case XK_F2:       rc = KEY_F2;            break;
      case XK_F3:       rc = KEY_F3;            break;
      case XK_F4:       rc = KEY_F4;            break;
      case XK_F5:       rc = KEY_F5;            break;
      case XK_F6:       rc = KEY_F6;            break;
      case XK_F7:       rc = KEY_F7;            break;
      case XK_F8:       rc = KEY_F8;            break;
      case XK_F9:       rc = KEY_F9;            break;
      case XK_F10:      rc = KEY_F10;           break;
      case XK_F11:      rc = KEY_F11;           break;
      case XK_F12:      rc = KEY_F12;           break;

      case XK_Pause:    rc = KEY_PAUSE;         break;

//      case XK_equal:    rc = '=';  break;
//      case XK_minus:    rc = '-';   break;

      // [WDJ] Most important that they have unique values because they
      // are assigned game actions by the user (by keypress sample).
      case XK_Caps_Lock: rc = KEY_CAPSLOCK;  break;
      case XK_Num_Lock: rc = KEY_NUMLOCK;  break;
      case XK_Scroll_Lock: rc = KEY_SCROLLLOCK;  break;
      case XK_Multi_key: rc = KEY_MENU /* KEY_unused1 */;  break;
      case XK_Mode_switch: rc = KEY_MODE;   break;

      case XK_Shift_L:  rc = KEY_LSHIFT;  shiftdown = keydown;  break;
      case XK_Shift_R:  rc = KEY_RSHIFT;  shiftdown = keydown;  break;
      case XK_Control_L: rc = KEY_LCTRL;  break;
      case XK_Control_R: rc = KEY_RCTRL;  break;
      case XK_Alt_L: rc = KEY_LALT;  altdown = keydown;  break;
      case XK_Alt_R: rc = KEY_RALT;  altdown = keydown;  break;
      case XK_Meta_R: rc = KEY_RWIN;  break;
      case XK_Meta_L: rc = KEY_LWIN;

      // I forgot them..... 19990128 by Kin
      case XK_Page_Up: rc = KEY_PGUP; break;
      case XK_Page_Down: rc = KEY_PGDN; break;
      case XK_End: rc = KEY_END; break;
      case XK_Home: rc = KEY_HOME; break;
      case XK_Insert: rc = KEY_INS; break;
      case XK_Delete:   rc = KEY_DELETE;   break;
      // hey, it's not a sparc 19990128 by Kin
      case XK_BackSpace: rc = KEY_BACKSPACE;    break;

      // metzgermeister: keypad layout as in g_input.c
      case XK_KP_0 :
      case XK_KP_Insert    : rc = KEY_KEYPAD0;  break;
      case XK_KP_1 :
      case XK_KP_End       : rc = KEY_KEYPAD1;  break;
      case XK_KP_2 :
      case XK_KP_Down      : rc = KEY_KEYPAD2;  break;
      case XK_KP_3 :
      case XK_KP_Page_Down : rc = KEY_KEYPAD3;  break;
      case XK_KP_4 :
      case XK_KP_Left      : rc = KEY_KEYPAD4;  break;
      case XK_KP_5 :
      case XK_KP_Begin     : rc = KEY_KEYPAD5;  break;
      case XK_KP_6 :
      case XK_KP_Right     : rc = KEY_KEYPAD6;  break;
      case XK_KP_7 :
      case XK_KP_Home      : rc = KEY_KEYPAD7;  break;
      case XK_KP_8 :
      case XK_KP_Up        : rc = KEY_KEYPAD8;  break;
      case XK_KP_9 :
      case XK_KP_Page_Up   : rc = KEY_KEYPAD9;  break;
      case XK_KP_Decimal :
      case XK_KP_Delete    : rc = KEY_KPADPERIOD;  break;
      case XK_KP_Divide    : rc = KEY_KPADSLASH; break;
      case XK_KP_Multiply  : rc = KEY_KPADMULT; break;
      case XK_KP_Subtract  : rc = KEY_MINUSPAD;  break;
      case XK_KP_Add       : rc = KEY_PLUSPAD;  break;
      case XK_KP_Enter     : rc = KEY_ENTER;    break;
      case XK_KP_Equal     : rc = KEY_KPADEQUALS;  break;

      default:
#if XK_space != ' '       
        if (rc >= XK_space && rc <= XK_asciitilde)
            rc = rc - XK_space + ' ';
#endif       
//        GenPrintf(EMSG_debug, "Key: %X -> %X -> %X\n", keycode, (unsigned int)keysym, (unsigned int)rc);
        break;
    }

    if (showkey)
      fprintf(stdout,"Key: %d\n", rc);

    return rc;
}

static char shift27[] =
{ 0x22, // QUOTE
  0,0,0,
  '+', '<', '_', '>', '?',  // '=', comma, '-', '.', '/'
  ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',  // '0' .. '9'
  0, ':',  // ':', ';'
};

int to_ASCII( int kcch, byte shiftdown )
{
    // SDL does this by  -> Unicode -> ASCII
    if( shiftdown )
    {
        if( kcch >= 'a' && kcch <= 'z' )
           kcch = kcch - 'a' + 'A';   // to uppercase
        else if( kcch >= 0x27 && kcch <= 0x3F )
           kcch = shift27[ kcch - 0x27 ];
    }
    return  (kcch <= 0x7F) ? kcch : 0; // ASCII key
}


//
// I_StartFrame
//
void I_StartFrame(void)
{
        /* frame synchronous IO operations not needed for X11 */
}


static int      lastmousex = 0;
static int      lastmousey = 0;
#ifdef POLL_POINTER
static int      newmousex;
static int      newmousey;
// last button state from dos code... 19990110 by Kin
static int      lastbuttons = 0;
#endif
boolean         shmFinished;

#ifdef LJOYSTICK
extern void I_GetJoyEvent();
#endif
#ifdef MOUSE2
extern byte mouse2_started;
#ifdef DEBUG_MOUSE2_STIMULUS
extern byte mouse2_stim_trigger;
#endif
void I_GetMouse2Event(void);
#endif


void I_GetEvent(void)
{
    event_t event;
#ifdef LJOYSTICK
    I_GetJoyEvent();
#endif
#ifdef MOUSE2
    if( mouse2_started )
        I_GetMouse2Event();
#endif

    // put event-grabbing stuff in here
    XNextEvent(X_display, &X_event);
    switch (X_event.type)
    {
      case KeyPress:
        event.type = ev_keydown;
        event.data1 = xlatekey(X_event.xkey.keycode, 1);
        event.data2 = to_ASCII(event.data1, shiftdown);
#ifdef DEBUG_MOUSE2_STIMULUS
        // Intercept some keys and fake the mouse2 input using mouse2_stim_trigger.
        if( (event.data1 >= KEY_KEYPAD1) && (event.data1 <= KEY_KEYPAD9) )
        {
            mouse2_stim_trigger = event.data1 - KEY_KEYPAD0;
	    break;
	}
#endif
        D_PostEvent(&event);
        break;

      case KeyRelease:
        event.type = ev_keyup;
        event.data1 = xlatekey(X_event.xkey.keycode, 0);
        event.data2 = to_ASCII(event.data1, shiftdown);
        D_PostEvent(&event);
        break;

#ifndef POLL_POINTER
      case ButtonPress:
        if (cv_usemouse[0].value) {
/*
            int buttons,i,j=1,k;
            event.type = ev_keydown;
            buttons =
                (X_event.xbutton.state & Button1Mask ? 1 : 0)
                | (X_event.xbutton.state & Button2Mask ? 2 : 0)
                | (X_event.xbutton.state & Button3Mask ? 4 : 0)
                | (X_event.xbutton.button == Button1 ? 1 : 0)
                | (X_event.xbutton.button == Button2 ? 2 : 0)
                | (X_event.xbutton.button == Button3 ? 4 : 0);
            k=(buttons ^ lastbuttons); // only changed bit to 1
            lastbuttons = buttons;
            for(i=0;i<MOUSEBUTTONS;i++,j<<=1)
                if(k & j) {
                    event.data1=KEY_MOUSE1+i;
                    D_PostEvent(&event);
                    break; // right? 19990110 by Kin
                }
*/
           int button = X_event.xbutton.button;
           if(button && button-1 < MOUSEBUTTONS)
           {
                   event.type = ev_keydown;
                   if(button==4)
                   {
                       event.data1 = KEY_MOUSEWHEELUP;
                   }
                   else if(button==5)
                   {
                       event.data1 = KEY_MOUSEWHEELDOWN;
                   }
                   else
                   {
                       event.data1=KEY_MOUSE1+button-1;
                   }
                   D_PostEvent(&event);
           }
        }
        break;

      case ButtonRelease:
        if (cv_usemouse[0].value)
        {
/*
          int buttons,i,j=1,k;
          event.type = ev_keyup;
          buttons =
            (X_event.xbutton.state & Button1Mask ? 1 : 0)
            | (X_event.xbutton.state & Button2Mask ? 2 : 0)
            | (X_event.xbutton.state & Button3Mask ? 4 : 0);
          // suggest parentheses around arithmetic in operand of |
          buttons =
            buttons
            ^ (X_event.xbutton.button == Button1 ? 1 : 0)
            ^ (X_event.xbutton.button == Button2 ? 2 : 0)
            ^ (X_event.xbutton.button == Button3 ? 4 : 0);
          k=(buttons ^ lastbuttons); // only changed bit to 1
          lastbuttons = buttons;
          for(i=0;i<MOUSEBUTTONS;i++,j<<=1)
                if(k & j)
                {
                    event.data1=KEY_MOUSE1+i;
                    D_PostEvent(&event);
                    break; // right? 19990110 by Kin
                }
*/
           int button = X_event.xbutton.button;
           if(button && button-1 < MOUSEBUTTONS)
           {
                   event.type = ev_keyup;
                   if(button==4||button==5)
                   {
                           //ignore
                   }
                   else
                   {
                       event.data1=KEY_MOUSE1+button-1;
                       D_PostEvent(&event);
                   }
           }
        }
        break;

      case MotionNotify:
        if (cv_usemouse[0].value)
        {
#ifdef WITH_DGA
          event.type = ev_mouse;
          event.data1 = 0;
          event.data2 = X_event.xmotion.x_root << 2;
          event.data3 = -X_event.xmotion.y_root << 2;
          D_PostEvent(&event);
#else
          // If the event is from warping the pointer back to middle
          // of the screen then ignore it.
          if ((X_event.xmotion.x == (vid.width>>1)) &&
            (X_event.xmotion.y == (vid.height>>1))) {
            lastmousex = X_event.xmotion.x;
            lastmousey = X_event.xmotion.y;
            break;
          } else {
            event.data2 = (X_event.xmotion.x - lastmousex) << 2;
            lastmousex = X_event.xmotion.x;
            event.data3 = (lastmousey - X_event.xmotion.y) << 2;
            lastmousey = X_event.xmotion.y;
          }
          event.type = ev_mouse;
          event.data1 =
            (X_event.xmotion.state & Button1Mask ? 1 : 0)
            | (X_event.xmotion.state & Button2Mask ? 2 : 0)
            | (X_event.xmotion.state & Button3Mask ? 4 : 0);
          D_PostEvent(&event);
          // Warp the pointer back to the middle of the window
          //  or we cannot move any further if it's at a border.
          if ((X_event.xmotion.x < 1) || (X_event.xmotion.y < 1)
            || (X_event.xmotion.x > vid.width-2)
            || (X_event.xmotion.y > vid.height-2))
          {
                XWarpPointer(X_display,
                          None,
                          X_mainWindow,
                          0, 0,
                          0, 0,
                          (vid.width>>1), (vid.height>>1) );
          }
#endif // #WITH_DGA
        }
        break;
#endif

      case UnmapNotify:
        if (!demoplayback) {
          paused = true;
          S_PauseSound();
        }
        break;

      case MapNotify:
        if (!demoplayback) {
          paused = false;
          S_ResumeSound();
        }
        break;

      case ClientMessage:
        if (X_event.xclient.data.l[0] == X_wm_delwin) {
          M_QuitResponse('y');
        }
        break;

      case Expose:
      case ConfigureNotify:
        break;

      default:
        if (doShm && X_event.type == X_shmeventtype)
            shmFinished = true;
        break;

    }

}

static Cursor createnullcursor(Display* display, Window root)
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
                                 &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

static void disableScreensaver()
{
   int timeout, interval, prefer_blanking, allow_exposures;

   XGetScreenSaver(X_display, &timeout, &interval, &prefer_blanking, &allow_exposures);
   XSetScreenSaver(X_display,        0,  interval,  prefer_blanking,  allow_exposures);
   XGetScreenSaver(X_display, &timeout, &interval, &prefer_blanking, &allow_exposures);

}

static void enableScreensaver()
{
   int timeout, interval, prefer_blanking, allow_exposures;

   XGetScreenSaver(X_display, &timeout, &interval, &prefer_blanking, &allow_exposures);
   XSetScreenSaver(X_display,        -1,  interval,  prefer_blanking,  allow_exposures);
   XGetScreenSaver(X_display, &timeout, &interval, &prefer_blanking, &allow_exposures);

}

static void I_GrabMouse(void)
{
    if(!X_display || cv_usemouse[0].value == 0 || M_CheckParm("-nomouse"))
        return;

    if(!Mousegrabbed) {
#ifdef WITH_DGA
        CONS_Printf("enable DGA mouse\n");
        XF86DGADirectVideo(X_display, X_screen, XF86DGADirectMouse);
#endif
        XGrabPointer(X_display, X_mainWindow, True,
#ifndef POLL_POINTER
                     ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
#else
                     0,
#endif
                     GrabModeAsync, GrabModeAsync,
                     X_mainWindow, None, CurrentTime);
        XGrabKeyboard(X_display, X_mainWindow, True,
                      GrabModeAsync, GrabModeAsync,
                      CurrentTime);
#ifdef POLL_POINTER
        XQueryPointer(X_display, X_mainWindow, &dummy, &dummy,
                      &dont_care, &dont_care,
                      &lastmousex, &lastmousey,
                      &dont_care_ui);
#endif
        disableScreensaver();
    }

    Mousegrabbed = true;
    return;
}

void I_UngrabMouse(void)
{
    if(!X_display)
        return;

    if(Mousegrabbed) {
#ifdef WITH_DGA
        CONS_Printf("disable DGA mouse\n");
        XF86DGADirectVideo(X_display, X_screen, 0);
#endif
        XUngrabKeyboard(X_display, CurrentTime);
        XUngrabPointer(X_display, CurrentTime);
        enableScreensaver();
        XSync(X_display, False);
    }

    Mousegrabbed = false;
    return;
}


// Called on video mode change, usemouse change, mousemotion change,
// and game paused.
//   play_mode : enable mouse containment during play
void I_StartupMouse( boolean play_mode )
{
    if( ( vidmode_active && play_mode )
       || (haveVoodoo && !M_CheckParm("-winvoodoo"))) {
        I_GrabMouse();
    }
    else
    {
        if( cv_usemouse[0].value && play_mode ) {
            I_GrabMouse();
        }
        else {
            I_UngrabMouse();
        }
    }
    return;
}

//
// I_OsPolling
//
void I_OsPolling(void)
{
#ifdef POLL_POINTER
    unsigned int mask;
#endif

    if (!X_display)
        return;

    if (cv_usemouse[0].value) {
#ifdef POLL_POINTER
        XQueryPointer(X_display, X_mainWindow, &dummy, &dummy,
                      &dont_care, &dont_care,
                      &newmousex, &newmousey,
                      &mask);
        { // process mouse 19990130 by Kin
            int buttons,i,j=1,k;
            buttons =
                (mask & Button1Mask ? 1 : 0)
                | (mask & Button2Mask ? 2 : 0)
                | (mask & Button3Mask ? 4 : 0)
                //doesn't work, why?
                | (mask & Button4Mask ? 8 : 0)
                | (mask & Button5Mask ? 16 : 0);
            k=(buttons ^ lastbuttons); // only changed bit to 1
            for(i=0;i<MOUSEBUTTONS;i++,j<<=1)
                if(k & j){
                    if(buttons & j) event.type = ev_keydown;
                    else event.type = ev_keyup;
                    if(i==4) event.data1=KEY_MOUSEWHEELUP;
                    else if(i==5) event.data1=KEY_MOUSEWHEELDOWN;
                    else event.data1=KEY_MOUSE1+i;
                    D_PostEvent(&event);
                    // don't break!!! 19990130 by Kin
                }
            lastbuttons = buttons;
        }

        event.type = ev_mouse;
        //          event.data1 = ((mask & Button1Mask) ? 1 : 0) |
        //            ((mask & Button2Mask) ? 2 : 0) |
        //            ((mask & Button3Mask) ? 4 : 0);
        event.data2 = (newmousex - lastmousex) << 2;
        event.data3 = -((newmousey - lastmousey) << 2);
        D_PostEvent(&event);
        if ((newmousex < 1) || (newmousey < 1)
            || (newmousex > vid.width-2)
            || (newmousey > vid.height-2)) {

            XWarpPointer(X_display,
                         None,
                         X_mainWindow,
                         0, 0,
                         0, 0,
                         vid.width/2, vid.height/2);
            lastmousex = vid.width/2;
            lastmousey = vid.height/2;
        } else {
            lastmousex = newmousex;
            lastmousey = newmousey;
        }
#endif
    }

    while (XPending(X_display))
        I_GetEvent();

    //reset wheel like in win32, I don't understand it but works
    gamekeydown[KEY_MOUSEWHEELUP] = 0;
    gamekeydown[KEY_MOUSEWHEELDOWN] = 0;
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
        /* empty */
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
  if(rendermode==render_soft) {

    // cv_vidwait.value not used, X11 handles its own vsync, no controls

    if( x_bytepp == vid.bytepp ) {
        // no translation
        // draw is same bpp as video
        // bpp = 8, bytepp = 1, multiply = 1 19990125 by Kin
        if( vid.direct_rowbytes == vid.widthbytes )
        {
            // if direct draw, then no copy needed
            if( vid.display != (byte*)image->data ) {
                memcpy(image->data, vid.display, vid.direct_size);
            }
        }
        else
            VID_BlitLinearScreen ( vid.display, (byte*)image->data, vid.widthbytes,
                                   vid.height, vid.ybytes, vid.direct_rowbytes );
    }
    // colormap transformation dependent upon X server color depth
    else if (x_bytepp == 2) { // 15 bpp, 16 bpp
       int x,y;
       int xstart = vid.width-1;
       unsigned char* ilineptr;
       uint16_t* olineptr;
       y = vid.height;
       while (y--) {
          olineptr = (uint16_t *) &(image->data[y*vid.width*x_bytepp]);
          ilineptr = (unsigned char*) (vid.display+(y*vid.width));
          x = xstart;
          do {
             olineptr[x] = x_colormap2[ilineptr[x]];
          } while (x--);
       }
    }
    else if (x_bytepp == 3) {  // 24 bpp
       int x,y;
       int xstart = vid.width-1;
       unsigned char* ilineptr;
       unsigned char* olineptr;
       y = vid.height;
       while (y--) {
#ifdef TILTVIEW
          olineptr = (unsigned char *) &image->data[y*vid.width*3];
#else
          olineptr = (unsigned char *) &image->data[y*vid.direct_rowbytes];
#endif
          ilineptr = (unsigned char*) (vid.display+(y*vid.width));
          x = xstart;
         do {
            memcpy(olineptr+3*x,x_colormap3+3*ilineptr[x],3);
         } while (x--);
       }
    }
    else if (x_bytepp == 4) {  // 32 bpp
       int x,y;
       int xstart = vid.width-1;
       unsigned char* ilineptr;
       uint32_t * olineptr;
       y = vid.height;
       while (y--) {
#ifdef TILTVIEW
          olineptr = (unsigned char *) &image->data[y*vid.width<<2]; // *4
#else
          olineptr = (unsigned int *) &(image->data[y*vid.direct_rowbytes]);
#endif
          ilineptr = (unsigned char*) (vid.display+(y*vid.width));
          x = xstart;
          do {
             olineptr[x] = x_colormap4[ilineptr[x]];
          } while (x--);
       }
    }

    if (doShm)
    {

        if (!XShmPutImage(X_display,
                          X_mainWindow,
                          X_gc,
                          image,
                          0, 0,
                          0, 0,
                          vid.width, vid.height,
                          True ))
            I_Error("XShmPutImage() failed\n");

        // wait for it to finish and processes all input events
        shmFinished = false;
        do
        {
            if (XPending(X_display))
                I_GetEvent();
            else
                usleep( 20 );
        } while (!shmFinished);

    }
    else
    {

        // draw the image
        XPutImage(      X_display,
                        X_mainWindow,
                        X_gc,
                        image,
                        0, 0,
                        0, 0,
                        vid.width, vid.height );

    }
  } else {
       HWD.pfnFinishUpdate(cv_vidwait.value);
  }
}


//
// I_ReadScreen
//
void I_ReadScreen(byte* scr)
{
    memcpy (scr, vid.display, vid.screen_size);
}


//
// Palette stuff.
//
static XColor colors[256];

static void UploadNewPalette(Colormap cmap, RGBA_t *palette)
{

    register int        i;
    register int        c;
    static boolean      firstcall = true;

#ifdef __cplusplus
    if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
    if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
        {
            // initialize the colormap
            if (firstcall)
            {
                firstcall = false;
                for (i=0 ; i<256 ; i++)
                {
                    colors[i].pixel = i;
                    colors[i].flags = DoRed|DoGreen|DoBlue;
                }
            }

            // set the X colormap entries
            for (i=0 ; i<256 ; i++,palette++)
            {
                c = palette->s.red;
                colors[i].red = (c<<8) + c;
                c = palette->s.green;
                colors[i].green = (c<<8) + c;
                c = palette->s.blue;
                colors[i].blue = (c<<8) + c;
            }

            // store the colors to the current colormap
            XStoreColors(X_display, cmap, colors, 256);

        }
}

static void EmulateNewPalette(RGBA_t *palette)
{
    register int        i;

    for (i=0 ; i<256 ; i++,palette++)
    {
        switch(x_bytepp) {
        case 2: x_colormap2[i] =
                   ((palette->s.red>>x_red_mask)<<x_red_offset) |
                   ((palette->s.green>>x_green_mask)<<x_green_offset) |
                   ((palette->s.blue>>x_blue_mask)<<x_blue_offset);
                break;
        case 3: x_colormap3[3*i+x_red_offset]   = palette->s.red;
                x_colormap3[3*i+x_green_offset] = palette->s.green;
                x_colormap3[3*i+x_blue_offset]  = palette->s.blue;
                break;
        case 4: x_colormap4[i] = 0;
               ((unsigned char*)(x_colormap4+i))[x_red_offset] =
                   palette->s.red;
               ((unsigned char*)(x_colormap4+i))[x_green_offset] =
                   palette->s.green;
               ((unsigned char*)(x_colormap4+i))[x_blue_offset] =
                   palette->s.blue;
               break;
        }
    }
}

//
// I_SetPalette
//
void I_SetPalette(RGBA_t* palette)
{
    if( rendermode == render_soft )
    {
        if (x_drawmode == DRAW8PAL)
            UploadNewPalette(X_cmap, palette);
        else
            EmulateNewPalette(palette);
    }
}


//
// This function is probably redundant,
//  if XShmDetach works properly.
// ddt never detached the XShm memory,
//  thus there might have been stale
//  handles accumulating.
//
static void grabsharedmemory(int size)
{
  int                   key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds       shminfo;
  int                   minsize = 320*200;
  int                   id;
  int                   rc;
  int                   pollution=5;

  // try to use what was here before
  do
  {
    id = shmget((key_t) key, minsize, 0777); // just get the id
    if (id != -1)
    {
      rc=shmctl(id, IPC_STAT, &shminfo); // get stats on it
      if (!rc)
      {
        if (shminfo.shm_nattch)
        {
          GenPrintf(EMSG_info, "User %d appears to be running "
                  "DOOM.  Is that wise?\n", shminfo.shm_cpid);
          key++;
        }
        else
        {
          if (getuid() == shminfo.shm_perm.cuid)
          {
            rc = shmctl(id, IPC_RMID, 0);
            if (!rc)
              GenPrintf(EMSG_info,
                      "Was able to kill my old shared memory\n");
            else
              I_Error("Was NOT able to kill my old shared memory\n");

            id = shmget((key_t)key, size, IPC_CREAT|0777);
            if (id==-1)
              I_Error("Could not get shared memory\n");

            rc=shmctl(id, IPC_STAT, &shminfo);

            break;
          }
          if (size >= shminfo.shm_segsz)
          {
            GenPrintf(EMSG_info,
                    "will use %d's stale shared memory\n",
                    shminfo.shm_cpid);
            break;
          }
          else
          {
            GenPrintf(EMSG_warn,
                    "warning: can't use stale "
                    "shared memory belonging to id %d, "
                    "key=0x%x\n",
                    shminfo.shm_cpid, key);
            key++;
          }
        }
      }
      else
      {
        I_Error("could not get stats on key=%d\n", key);
      }
    }
    else
    {
      id = shmget((key_t)key, size, IPC_CREAT|0777);
      if (id==-1)
      {
        GenPrintf(EMSG_error, "errno=%d\n", errno);
        I_Error("Could not get any shared memory\n");
      }
      break;
    }
  } while (--pollution);

  if (!pollution)
  {
    I_Error("Sorry, system too polluted with stale "
            "shared memory segments.\n");
  }

  X_shminfo.shmid = id;

  // attach to the shared memory segment
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);
  if(verbose)
  {
      GenPrintf(EMSG_ver, "shared memory id=%d, addr=0x%x\n", id,
              (int) (image->data));
  }
  return;
}


// return number of fullscreen or window modes, for listing
// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };  // first is always 1

    if( num_vidmodes == 0 ) 
        determine_VidModes();

    if(haveVoodoo)
        mrange.last = NUM_VOODOOMODES;
    else
        mrange.last = (modetype == MODE_fullscreen) ? num_vidmodes : MAXWINMODES;
    return mrange;
}


modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    if(haveVoodoo)
    { // voodoo modes
        int mi = modenum.index - 1;
        if(mi >= NUM_VOODOOMODES)   goto fail;

        ms.width = voodooModes[mi][0];
        ms.height = voodooModes[mi][1];
        ms.type = MODE_voodoo;
        ms.mark = "fx";
    }
    else if( modenum.modetype == MODE_fullscreen )
    { // fullscreen modes
        int mi = modenum.index - 1;
        if(mi >= num_vidmodes)   goto fail;

        XF86VidModeModeInfo * vm3 = fullvidmodes[ vidmap[mi] ];
        ms.width = vm3->hdisplay;
        ms.height = vm3->vdisplay;
        ms.type = MODE_fullscreen;
        ms.mark = "";
    }
    else
    { // X11 window modes
        int mi = modenum.index;
        if(mi > MAXWINMODES)    goto fail;

        ms.width = windowedModes[mi][0];
        ms.height = windowedModes[mi][1];
        ms.type = MODE_window;
        ms.mark = "X11";
    }
    return  ms;

 fail:
    ms.type = MODE_NOP;
    ms.width = ms.height = 0;
    ms.mark = NULL;
    return ms;
}


// to display selection name of modes
char * VID_GetModeName( modenum_t modenum )
{
    modestat_t ms = VID_GetMode_Stat( modenum );
    if( ! ms.mark )
        goto fail;

    // form the string
    // vidModeName[MAX_NUM_VIDMODES][MAX_LEN_VIDMODENAME+1];
    snprintf( &vidModeName[modenum.index][0], MAX_LEN_VIDMODENAME, "%s %dx%d",
              ms.mark, ms.width, ms.height );
    vidModeName[modenum.index][MAX_LEN_VIDMODENAME] = 0;
    return &vidModeName[modenum.index][0];

 fail:
    return NULL;
}


// rmodetype is of modetype_e
// Returns MODE_NOP when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype )
{
    modenum_t  modenum = { MODE_NOP, 0 };
    int bestdist = INT_MAX;
    int best, tdist, i;

    if(haveVoodoo)
    {
        best = 1; // standard mode if no other found
        for (i = 0; i < NUM_VOODOOMODES; i++)
        {
            tdist = abs(voodooModes[i][0] - rw) + abs(voodooModes[i][1] - rh);
            // find closest dist
            if( bestdist > tdist )
            {
                bestdist = tdist;
                best = i;
                if( tdist == 0 )  break;   // found exact match
            }
        }
        modenum.index = best + 1;  // 1..
        modenum.modetype = MODE_voodoo;
    }
    else if( rmodetype == MODE_fullscreen )
    {   // scan fullscreen modes
       
        // If vidmode_ext == false, or all w,h are too large
        // then will have num_vidmodes == 0
        if( num_vidmodes == 0 )  goto done;
        best = num_vidmodes-1;  // default is smallest mode

        for (i = 0; i < num_vidmodes; i++)
        {
            XF86VidModeModeInfo * vmm = fullvidmodes[ vidmap[i] ];
            tdist = abs(vmm->hdisplay - rw) + abs(vmm->vdisplay - rh);
            // find closest dist
            if( bestdist > tdist )
            {
                bestdist = tdist;
                best = i;
                if( tdist == 0 )  break;   // found exact match
            }
        }
        modenum.index = best + 1;  // 1..
        modenum.modetype = rmodetype;
    }
    else
    {   // windowed modes, 1..
        best = MAXWINMODES;  // default
        for (i = 1; i <= MAXWINMODES; i++)
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
    }
done:
    return modenum;
}


static void destroyWindow(void)
{
    I_UngrabMouse();

    if(rendermode==render_soft)
    {
       if( vid.buffer )
       {
           free(vid.buffer);
           vid.buffer = vid.direct = NULL; // I want to have an access violation if something goes wrong
           vid.display = NULL;
           vid.screen1 = NULL;
       }

       if(doShm) {
           if (!XShmDetach(X_display, &X_shminfo))
               I_Error("destroyWindow: XShmDetach() failed\n");

           // Release shared memory.
           shmdt(X_shminfo.shmaddr);
           shmctl(X_shminfo.shmid, IPC_RMID, 0);

           if( image )
           {
               XDestroyImage(image); // is this correct? there is no shm counterpart
               image = NULL;
           }
       }
       else {
           if( image )
           {
               XDestroyImage(image);
               image = NULL;
           }
       }

       if( X_mainWindow )
       {
           XDestroyWindow(X_display, X_mainWindow);
           X_mainWindow=0;
       }
    }
    else {
        ; // actually do nothing here; HookXwin takes care; not very clean, but it's legacy
    }

   return;
}

// Called multiple times
static int createWindow(boolean set_fullscreen, modenum_t modenum)
{
    int                  oktodraw;
    unsigned long        attribmask;
    XSetWindowAttributes attribs;
    XGCValues            xgcvalues;
    XTextProperty        windowName, iconName;
    int                  valuemask;
    char                 *window_name = "Legacy";
    char                 *icon_name = window_name;

    // change to the mode
    if( !set_fullscreen )
    {
        // Turn off existing fullscreen.
        if( vidmode_ext && fullvidmodes && vidmode_active )
        {
            // Complains of BadAccess when try to switch mode of main screen.
            // First mode was current mode, so restore to [0].
            XF86VidModeSwitchToMode(X_display, X_screen, fullvidmodes[0]);
        }
        else if( !vidmode_ext ) // probably not necessary
        {
        }
        vidmode_active = false;
    }
    else
    {
        // Fullscreen
        XF86VidModeSwitchToMode(X_display, X_screen, fullvidmodes[vidmap[modenum.index-1]]);
        vidmode_active = true;
        // Move the viewport to top left
        XF86VidModeSetViewPort(X_display, X_screen, 0, 0);
    }

    if(rendermode==render_soft) {
        // setup attributes for main window
        if (vidmode_active) {
            // [WDJ] Submitted by pld-linux: Do not force CWColormap, it may be a truecolor mode.
            attribmask = CWSaveUnder | CWBackingStore |
                 CWEventMask | CWOverrideRedirect;

            attribs.override_redirect = True;
            attribs.backing_store = NotUseful;
            attribs.save_under = False;
        } else {
            // [WDJ] Submitted by pld-linux: Do not force CWColormap, it may be a truecolor mode.
            attribmask = CWBorderPixel | CWEventMask;
        }

        attribs.event_mask = KeyPressMask | KeyReleaseMask
#ifndef POLL_POINTER
            | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
#endif
            | ExposureMask | StructureNotifyMask;

        // [WDJ] Submitted by pld-linux: Do not force CWColormap, it may be a truecolor mode.
        // Only in DRAW8PAL does X handle the colormap, not in TrueColor where we do.
        if (x_drawmode == DRAW8PAL) {
            attribmask |= CWColormap;
            attribs.colormap = X_cmap;
        }
        attribs.border_pixel = 0;

        // create the main window
        X_mainWindow = XCreateWindow(X_display,
                                 RootWindow(X_display, X_screen),
                                 0, 0, // x, y,
                                 vid.width, vid.height,
                                 0, // borderwidth
                                 X_visualinfo.depth, // depth
                                 InputOutput,
                                 X_visual,
                                 attribmask,
                                 &attribs);

        if(!X_mainWindow)  goto fail;

        // create the GC
        valuemask = GCGraphicsExposures;
        xgcvalues.graphics_exposures = False;
        X_gc = XCreateGC(X_display,
                     X_mainWindow,
                     valuemask,
                     &xgcvalues );
    } else {
        // Hardware renderer
        X_mainWindow = HWD.pfnHookXwin(X_display, vid.width, vid.height, vidmode_active);
        if(X_mainWindow == 0)   goto fail;
    }

    // moved here
    XDefineCursor(X_display, X_mainWindow,
                  createnullcursor( X_display, X_mainWindow ) );

    // set size hints for window manager, so that resizing isn't possible
    X_size.flags = USPosition | PSize | PMinSize | PMaxSize;
    X_size.min_width = vid.width;
    X_size.min_height = vid.height;
    X_size.max_width = vid.width;
    X_size.max_height = vid.height;

    // window and icon name for the window manager
    XStringListToTextProperty(&window_name, 1, &windowName);
    XStringListToTextProperty(&icon_name, 1, &iconName);

    // window manager hints
    X_wm.initial_state = NormalState;
    X_wm.input = True;
    X_wm.flags = StateHint | InputHint;

    // property class, in case we get a configuration file sometime
    X_class.res_name = "legacy";
    X_class.res_class = "Legacy";

    // set the properties
    XSetWMProperties(X_display, X_mainWindow, &windowName, &iconName,
                0 /*argv*/, 0 /*argc*/, &X_size, &X_wm, &X_class);

    // set window manager protocol
    X_wm_delwin = XInternAtom(X_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(X_display, X_mainWindow, &X_wm_delwin, 1);

    // map the window
    XMapWindow(X_display, X_mainWindow);

    if (vidmode_active) {
       XMoveWindow(X_display, X_mainWindow, 0, 0);
       XRaiseWindow(X_display, X_mainWindow);
       XWarpPointer(X_display, None, X_mainWindow, 0, 0, 0, 0, 0, 0);
       XFlush(X_display);
       // Move the viewport to top left
       XF86VidModeSetViewPort(X_display, X_screen, 0, 0);
    }
    XFlush(X_display);

    // wait until it is OK to draw
    oktodraw = 0;
    while (!oktodraw)
    {
        XNextEvent(X_display, &X_event);
        if (X_event.type == Expose
            && !X_event.xexpose.count)
        {
            oktodraw = 1;
        }
    }

    // set the focus to the window
    XSetInputFocus(X_display, X_mainWindow, RevertToPointerRoot, CurrentTime);

    // get the pointer coordinates so that the first pointer move won't
    // be completely wrong
    XQueryPointer(X_display, X_mainWindow, &dummy, &dummy,
                  &dont_care, &dont_care,
                  &lastmousex, &lastmousey,
                  &dont_care_ui);

    vid.widthbytes = vid.width * vid.bytepp;
    vid.ybytes = vid.widthbytes;
    vid.screen_size = vid.ybytes * vid.height;

    if(rendermode==render_soft)
    {
      if (doShm)
      {
        X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;

        // create the image
        image = XShmCreateImage(X_display,
                                X_visual,
                                X_visualinfo.depth,
                                ZPixmap,
                                0,
                                &X_shminfo,
                                vid.width,
                                vid.height );

        grabsharedmemory(image->bytes_per_line * image->height);

        if (!image->data)
        {
            perror("");
            I_Error("InitGraphics: shmat() failed\n");
        }

        // get the X server to attach to it
        if (!XShmAttach(X_display, &X_shminfo))
            I_Error("InitGraphics(): XShmAttach() failed\n");
      }
      else
      {
        image = XCreateImage(X_display,
                             X_visual,
                             X_visualinfo.depth,
                             ZPixmap,
                             0,  // offset
                             (char*)malloc(vid.width * vid.height * x_bytepp),
                             vid.width, vid.height,
                             8*x_bytepp,
                             vid.width*x_bytepp );
      }

      // [WDJ] Draw 8pp and translate to other bpp in FinishUpdate
      // unless -bpp or -native
      vid.buffer = (unsigned char *) malloc (vid.screen_size * NUMSCREENS);
      vid.display = vid.buffer;
      vid.screen1 = vid.buffer + vid.screen_size;
#ifdef TILTVIEW
      // FIXME: TILTVIEW must not access image, it may be different bpp
      // FIXME: external access to direct is not allowed
      // Forces Direct to be buffer
      vid.direct = vid.buffer;
      vid.direct_rowbytes = vid.ybytes;
      vid.direct_size = vid.screen_size;
#else
      // Direct is video
      vid.direct = (byte*)image->data;
      vid.direct_rowbytes = vid.width * x_bytepp;
      vid.direct_size = vid.direct_rowbytes * vid.height;
#if 0
// [WDJ] This used to work with my nVidia, but segfaults after a few writes with my Radeon.
      if( x_bytepp == vid.bytepp && vid.direct_rowbytes == vid.widthbytes )
      {
          // can draw direct into image
          vid.display = vid.direct;
          GenPrintf(EMSG_info, "Draw direct\n");
      }
#endif
#endif
      // Will segfault on the verbose messages if the screen is not setup.
      V_Setup_VideoDraw();

      if( verbose )
      {
          GenPrintf(EMSG_ver, "Drawing %i bpp,  video at % i bpp\n", vid.bitpp, x_bitpp );
      }
      vid.fullscreen = set_fullscreen;

      // added for 1.27 19990220 by Kin
      graphics_state = VGS_active;
    } else {
      graphics_state = ((X_mainWindow==0)? VGS_off:VGS_active);
    }
    return 1;
   
  fail:
    return 0;
}


int VID_SetMode( modenum_t modenum )
{
    boolean set_fullscreen = (modenum.modetype == MODE_fullscreen);

    vid.draw_ready = 0;  // disable print reaching console
   
    if(haveVoodoo) {
        int mi = modenum.index - 1;
        if(mi >= NUM_VOODOOMODES)   goto fail_end;

        destroyWindow();
        set_fullscreen = true;
        vid.width = voodooModes[mi][0];
        vid.height = voodooModes[mi][1];
    }
    else if (set_fullscreen) { // fullscreen
        int mi = modenum.index - 1;
        // If no vidmode_ext, then num_vidmodes == 0
        if(mi >= num_vidmodes)   goto fail_end;

        destroyWindow();
        XF86VidModeModeInfo * vm3 = fullvidmodes[ vidmap[mi] ];
        vid.width = vm3->hdisplay;
        vid.height = vm3->vdisplay;
    }
    else { // X11
        int mi = modenum.index;
        if(mi >= MAXWINMODES)   goto fail_end;

        destroyWindow();
        vid.width = windowedModes[mi][0];
        vid.height = windowedModes[mi][1];
    }

    vid.recalc = 1;
    GenPrintf( EMSG_info, "VID_SetMode(%s,%i) %dx%d\n",
               modetype_string[modenum.modetype], modenum.index, vid.width, vid.height);

    if(!createWindow( set_fullscreen, modenum ))
        return FAIL_create;

    I_StartupMouse( false );

    vid.modenum = modenum;

    return 1;

 fail_end:
    return FAIL_end;
}

// detect Voodoo card
void detect_Voodoo( void )
{
    static boolean isVoodooChecked = false;
    char *renderer_str, *voodoo_detect;
    // I cannot detect the Voodoo earlier, so I try to catch it here
    if(!isVoodooChecked) {
        if(rendermode == render_opengl) {
            renderer_str = HWD.pfnGetRenderer();
            if( verbose )
            {
               GenPrintf(EMSG_ver, "%s\n", renderer_str );
            }
            // FIXME: if Mesa ever decides to change the this spotword, we're deep in the shit
            voodoo_detect = strstr(renderer_str, "Voodoo_Graphics");
            if( ! voodoo_detect )
                voodoo_detect = strstr(renderer_str, "Voodoo");
            if(voodoo_detect != NULL)
            {
                haveVoodoo = true;
                if( verbose )
                {
                    GenPrintf(EMSG_ver, "Voodoo card detected\n" );
                }
            }
        }
        isVoodooChecked = true;
    }
}


//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    int num;

    // Require modelist before rendermode is set.
   
    // Only has native bpp and opengl (using native).
    if( request_drawmode < DRM_opengl )
    {
        if( request_bitpp != native_bitpp )
            return false;
    }

    if(haveVoodoo)
    {
        num = NUM_VOODOOMODES;
    }
    else if( request_fullscreen )
    {
        // once we have the vidmodes, it is not going to change
        if( num_vidmodes == 0 ) 
            determine_VidModes();
        num = num_vidmodes;
    }
    else
        num = MAXWINMODES;

    return (num > 0);
}



// Setup HWR calls according to rendermode.
int I_Rendermode_setup( void )
{
    static byte  HWD_current = 0;
    void *dlptr;

    if( rendermode == render_opengl )
    {
        if( HWD_current == render_opengl )
            return 1;

        // only set MESA_GLX_FX if not set by set user
        if(!getenv("MESA_GLX_FX"))
        {
            if(M_CheckParm("-winvoodoo"))
            {
                // use windowed mode for voodoo cards if requested
                putenv("MESA_GLX_FX=window");
                putenv("SSTV2_VGA_PASS=1");
                putenv("SSTV2_NOSHUTDOWN=1");
            }
            else
            {
                // Tell Mesa GLX to use 3Dfx driver in fullscreen mode.
                putenv("MESA_GLX_FX=fullscreen");
            }

            // Disable 3Dfx Glide splash screen
            putenv("FX_GLIDE_NO_SPLASH=0");
        }

       // try to open library in CWD
       dlptr = dlopen("./r_opengl.so",RTLD_NOW | RTLD_GLOBAL);

       if(!dlptr) {
           // try to open in LIBPATH
           dlptr = dlopen("r_opengl.so",RTLD_NOW | RTLD_GLOBAL);
       }

       if(!dlptr)
       {
           // to get first error messages
           dlopen("./r_opengl.so",RTLD_NOW | RTLD_GLOBAL);
           GenPrintf(EMSG_error, "Error opening r_opengl.so\n%s\n", dlerror());
#if 0	  
           {
               // [WDJ] Troubleshoot why cannot open it
               char * cwd2 = getcwd( NULL, 0 );  // malloc
               GenPrintf(EMSG_error, "CWD: %s \n", cwd2 );
               free( cwd2 );
               char * readperm = (access( "r_opengl.so", R_OK ) == 0 )? "OK": "NOT PERMITTED";
               char * execperm = (access( "r_opengl.so", X_OK ) == 0 )? "OK": "NOT PERMITTED";
               GenPrintf(EMSG_error, "Access r_opengl.so: READ %s, EXECUTE %s\n", readperm, execperm );
           }
#endif	  
           // Fail to software rendering
           rendermode = render_soft;
           return FAIL;
       }
       
       // linkage to dll r_opengl.so
       HWD.pfnInit = dlsym(dlptr,"Init");
       HWD.pfnShutdown = dlsym(dlptr,"Shutdown");
       HWD.pfnHookXwin = dlsym(dlptr,"HookXwin");
       HWD.pfnSetPalette = dlsym(dlptr,"SetPalette");
       HWD.pfnFinishUpdate = dlsym(dlptr,"FinishUpdate");
       HWD.pfnDraw2DLine = dlsym(dlptr,"Draw2DLine");
       HWD.pfnDrawPolygon = dlsym(dlptr,"DrawPolygon");
       //HWD.pfnGetState = dlsym(dlptr,"GetState");
       HWD.pfnSetBlend = dlsym(dlptr,"SetBlend");
       HWD.pfnClearBuffer = dlsym(dlptr,"ClearBuffer");
       HWD.pfnSetTexture = dlsym(dlptr,"SetTexture");
       HWD.pfnReadRect = dlsym(dlptr,"ReadRect");
       HWD.pfnGClipRect = dlsym(dlptr,"GClipRect");
       HWD.pfnClearMipMapCache = dlsym(dlptr,"ClearMipMapCache");
       HWD.pfnSetSpecialState = dlsym(dlptr,"SetSpecialState");
       HWD.pfnGetRenderer = dlsym(dlptr, "GetRenderer");
       //FIXME: check if all this is ok:
       HWD.pfnDrawMD2 = dlsym(dlptr, "DrawMD2");
       HWD.pfnSetTransform = dlsym(dlptr, "SetTransform");
       HWD.pfnGetTextureUsed = dlsym(dlptr, "GetTextureUsed");
       HWD.pfnGetRenderVersion = dlsym(dlptr, "GetRenderVersion");

       // check gl renderer lib
       if (HWD.pfnGetRenderVersion() != DOOMLEGACY_COMPONENT_VERSION)
       {
           I_Error ("Renderer version does not match executable version\n");
       }

       HWD_current = render_opengl;
    }
    return 1;
}


// Called once. Init with basic error message screen.
void I_StartupGraphics(void)
{
    modenum_t  initialmode = {MODE_window,0};  // the initial mode
    // pre-init by V_Init_VideoControl
    char  *displayname;
   
    vid.draw_ready = 0;  // disable print reaching console
    graphics_state = VGS_startup;

    // FIXME: catch other signals as well?
    signal(SIGINT, (void (*)(int)) I_Quit);
    signal(SIGTERM, (void (*)(int)) I_Quit); // shutdown gracefully if terminated

    XSetErrorHandler( X_error_handler );

    // subject to -display switch
    displayname = initDisplay();  // Set X_display

    findVisual();  // Set X_screen, X_visualinfo, X_visual, x_drawmode

    determineBPP(); // Sets x_bitpp and x_bytepp from X_visualinfo.depth
    native_drawmode = DRM_native;
    native_bitpp = x_bitpp;
    native_bytepp = x_bytepp;

    check_vidmode_extension();  // Set vidmode_ext
    checkForShm();  // Set doShm

    if( V_CanDraw( x_bitpp ))
    {
        vid.bitpp = x_bitpp;
        vid.bytepp = x_bytepp;
        GenPrintf(EMSG_info, "Video %s: %i bpp (%i bytes)\n", displayname, vid.bitpp, vid.bytepp);
    }
    else if( verbose )
    {
        // Use 8 bit and do the palette translation.
        vid.bitpp = 8;
        vid.bytepp = 1;
        GenPrintf(EMSG_ver, "Video %s: %i bpp rejected\n", displayname, x_bitpp );
    }
    determineColorMask();  // Set color offset and masks
    createColorMap();  // Create X_cmap and x_colormap2/3/4

    // window
    initialmode = VID_GetModeForSize( vid.width, vid.height, MODE_window );
    if( ! createWindow( false, initialmode) )
        goto abort_error;
   
    // startupscreen does not need a grabbed mouse
    I_UngrabMouse();

    if( verbose )
        GenPrintf(EMSG_ver, "StartupGraphics completed\n" );

    vid.modenum = initialmode;
    vid.recalc = true;
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
    modenum_t  initialmode;
    byte  select_fullscreen_mode;
    int  ret_value;

    vid.draw_ready = 0;  // disable print reaching console
    graphics_state = VGS_startup;
    destroyWindow();

    // setup vid 19990110 by Kin

    if( req_drawmode == DRM_opengl ) 
    {
        ret_value = I_Rendermode_setup();  // some functions needed immediately
        if( ret_value < 0 )
            return ret_value;
       
        // requires HWD.pfnGetRenderer
        detect_Voodoo();
    }

    // Set hardware vidmodes and vidmap, from X_display, X_screen

    if( select_fullscreen )
        determine_VidModes();

    findVisual();  // Set X_screen, X_visualinfo, X_visual, x_drawmode

    determineBPP(); // Sets x_bitpp and x_bytepp from X_visualinfo.depth

    checkForShm();  // Set doShm

    switch(req_drawmode)
    {
     case DRM_explicit_bpp:
       if( req_bitpp == 8 )
           goto draw_pal8;

       if( x_bitpp != req_bitpp )
       {
           GenPrintf(EMSG_error, "Not in %i bpp mode\n", req_bitpp );
           goto no_modes;
       }
       vid.bitpp = x_bitpp;
       vid.bytepp = x_bytepp;
       goto accept;

     case DRM_native:
       if( V_CanDraw( native_bitpp ))
       {
           if( verbose )
               GenPrintf(EMSG_info, "Video %i bpp (%i bytes)\n", native_bitpp, native_bytepp);
           goto draw_native;
       }
       else
       {
           GenPrintf( EMSG_info,"Native %i bpp rejected\n", native_bitpp );
           // Use 8 bit and do the palette translation.
           goto draw_pal8;
       }
       break;

     case DRM_opengl:
     default:
       goto draw_native;
    }

draw_pal8:
    // Use 8 bit and do the palette translation.
    vid.bitpp = 8;
    vid.bytepp = 1;
    goto accept;

draw_native:
    vid.bitpp = native_bitpp;
    vid.bytepp = native_bytepp;
    goto accept;

accept:   
    determineColorMask();  // Set color offset and masks
    createColorMap();  // Create X_cmap and x_colormap2/3/4

    if( select_fullscreen
        && ! haveVoodoo
        && num_vidmodes == 0)  // do we have any fullscreen modes at all?
    {
        // switch to windowed
        select_fullscreen = 0;
        GenPrintf(EMSG_info,"No usable video modes,\nSwitching to windowed mode ...\n");
//        goto no_modes;
    }
   
    vid.width = req_width;
    vid.height = req_height;

    select_fullscreen_mode = vid_mode_table[select_fullscreen];
    initialmode = VID_GetModeForSize( req_width, req_height, select_fullscreen_mode );
    if( ! createWindow( select_fullscreen, initialmode ) )
        return FAIL_create;

    // startupscreen does not need a grabbed mouse
    I_UngrabMouse();

    if( verbose )
        GenPrintf(EMSG_ver, "RequestFullGraphics completed\n" );

    vid.modenum = initialmode;
    vid.recalc = true;
    graphics_state = VGS_fullactive;
    return 1;

no_modes:
    return FAIL_select;
}


void I_ShutdownGraphics(void)
{
    if( graphics_state <= VGS_shutdown )
        return;

    graphics_state = VGS_shutdown;  // to catch some repeats due to errors

    // was graphics initialized anyway?
    if (!X_display)
        return;

    destroyWindow();

    // return to normal mode
    if( vidmode_ext && vidmode_active && fullvidmodes )
    {
        // First item of vidmodes was the current mode.       
        XF86VidModeSwitchToMode(X_display, X_screen, fullvidmodes[0]);
        vidmode_active = false;
    }

    if(rendermode != render_soft) {
        HWD.pfnShutdown();
    }

    if( fullvidmodes )
    {
        free( fullvidmodes );
        fullvidmodes = NULL;
    }

    XCloseDisplay(X_display);
    graphics_state = VGS_off;
}
