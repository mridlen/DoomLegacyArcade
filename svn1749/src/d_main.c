// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_main.c 1746 2025-04-10 09:37:38Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2024 by DooM Legacy Team.
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
// $Log: d_main.c,v $
// Revision 1.66  2005/12/20 14:58:25  darkwolf95
// Monster behavior CVAR - Affects how monsters react when they shoot each other
//
// Revision 1.65  2004/07/27 08:19:34  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.64  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.63  2003/11/21 17:52:05  darkwolf95
// added "Monsters Infight" for Dehacked patches
//
// Revision 1.62  2003/07/23 17:20:37  darkwolf95
// Initial Chex Quest 1 Support
//
// Revision 1.61  2003/07/14 21:22:23  hurdler
// Revision 1.60  2003/07/13 13:16:15  hurdler
//
// Revision 1.59  2003/05/04 02:28:34  sburke
// Fix for big-endian machines.
//
// Revision 1.58  2002/12/13 22:34:27  ssntails
// MP3/OGG support!
//
// Revision 1.57  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.56  2002/09/17 21:20:02  hurdler
// Quick hack for hacx freeze
//
// Revision 1.55  2002/08/24 22:42:02  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.54  2002/07/26 15:21:36  hurdler
//
// Revision 1.53  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
//
// Revision 1.52  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.51  2001/08/12 15:21:03  bpereira
// see my log
//
// Revision 1.50  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.49  2001/05/27 13:42:47  bpereira
// Revision 1.48  2001/05/16 21:21:14  bpereira
//
// Revision 1.47  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.46  2001/04/27 13:32:13  bpereira
//
// Revision 1.45  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.44  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.43  2001/04/02 18:54:32  bpereira
// Revision 1.42  2001/04/01 17:35:06  bpereira
// Revision 1.41  2001/03/30 17:12:49  bpereira
//
// Revision 1.40  2001/03/19 18:25:02  hurdler
// Is there a GOOD reason to check for modified game with shareware version?
//
// Revision 1.39  2001/03/03 19:43:09  ydario
// OS/2 code cleanup
//
// Revision 1.38  2001/02/24 13:35:19  bpereira
// Revision 1.37  2001/02/10 12:27:13  bpereira
//
// Revision 1.36  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.35  2000/11/06 20:52:15  bpereira
// Revision 1.34  2000/11/03 03:27:17  stroggonmeth
// Revision 1.33  2000/11/02 19:49:35  bpereira
//
// Revision 1.32  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.31  2000/10/21 08:43:28  bpereira
// Revision 1.30  2000/10/08 13:29:59  bpereira
// Revision 1.29  2000/10/02 18:25:44  bpereira
// Revision 1.28  2000/10/01 10:18:16  bpereira
// Revision 1.27  2000/09/28 20:57:14  bpereira
// Revision 1.26  2000/08/31 14:30:55  bpereira
//
// Revision 1.25  2000/08/29 15:53:47  hurdler
// Remove master server connect timeout on LAN (not connected to Internet)
//
// Revision 1.24  2000/08/21 21:13:00  metzgermeister
// Implementation of I_GetKey() in Linux
//
// Revision 1.23  2000/08/10 14:50:19  ydario
// OS/2 port
//
// Revision 1.22  2000/05/07 08:27:56  metzgermeister
// Revision 1.21  2000/04/30 10:30:10  bpereira
//
// Revision 1.20  2000/04/25 19:49:46  metzgermeister
// support for automatic wad search
//
// Revision 1.19  2000/04/24 20:24:38  bpereira
// Revision 1.18  2000/04/23 16:19:52  bpereira
//
// Revision 1.17  2000/04/22 20:27:35  metzgermeister
// support for immediate fullscreen switching
//
// Revision 1.16  2000/04/21 20:04:20  hurdler
// fix a problem with my last SDL merge
//
// Revision 1.15  2000/04/19 15:21:02  hurdler
// add SDL midi support
//
// Revision 1.14  2000/04/18 12:55:39  hurdler
// Revision 1.13  2000/04/16 18:38:07  bpereira
//
// Revision 1.12  2000/04/07 23:10:15  metzgermeister
// fullscreen support under X in Linux
//
// Revision 1.11  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.10  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.9  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.8  2000/03/29 19:39:48  bpereira
//
// Revision 1.7  2000/03/28 16:18:41  linuxcub
// Added a command to the Linux sound-server which sets a master volume.
// Added code to the main parts of doomlegacy which uses this command to
// implement volume control for sound effects.
//
// Added code so the (really cool) cd music works for me. The volume didn't
// work for me (with a Teac 532E drive): It always started at max (31) no-
// matter what the setting in the config-file was. The added code "jiggles"
// the volume-control, and now it works for me :-)
// If this code is unacceptable, perhaps another solution is to periodically
// compare the cd_volume.value with an actual value _read_ from the drive.
// Ie. not trusting that calling the ioctl with the correct value actually
// sets the hardware-volume to the requested value. Right now, the ioctl
// is assumed to work perfectly, and the value in cd_volume.value is
// compared periodically with cdvolume.
//
// Updated the spec file, so an updated RPM can easily be built, with
// a minimum of editing. Where can I upload my pre-built (S)RPMS to ?
//
// Erling Jacobsen, linuxcub@email.dk
//
// Revision 1.6  2000/03/23 22:54:00  metzgermeister
// added support for HOME/.legacy under Linux
//
// Revision 1.5  2000/03/06 17:33:36  hurdler
// Revision 1.4  2000/03/05 17:10:56  bpereira
// Revision 1.3  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
//
// DESCRIPTION:
//      DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//      plus functions to determine game mode (shareware, doom_registered),
//      parse command line parameters, configure game parameters (turbo),
//      and call the startup functions.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
  // MAX_WADPATH

#ifdef __WIN32__
#include <direct.h>
#else
#include <unistd.h>     // for access
#define _MAX_PATH   MAX_WADPATH
#endif

// using MAX_WADPATH as buffer limit, so _MAX_PATH must be as long
#if _MAX_PATH < MAX_WADPATH
#undef _MAX_PATH
#define _MAX_PATH   MAX_WADPATH
#endif


#include "doomstat.h"

#include "command.h"
#include "console.h"

#include "am_map.h"
#include "d_net.h"
#include "d_netcmd.h"
#include "dehacked.h"
#include "dstrings.h"

#include "f_wipe.h"
#include "f_finale.h"

#include "g_game.h"
#include "hs_stuff.h"
#include "g_input.h"

#include "hu_stuff.h"

#include "i_sound.h"
#include "i_system.h"
#include "i_video.h"

#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_swap.h"

#include "p_setup.h"
#include "p_fab.h"
#include "p_info.h"
#include "infoext.h"
#include "p_local.h"
#include "p_extnodes.h"

#include "r_main.h"
#include "r_local.h"

#include "s_sound.h"
#include "st_stuff.h"

#include "t_script.h"

#include "v_video.h"

#include "wi_stuff.h"
#include "w_wad.h"

#include "z_zone.h"
#include "d_main.h"
#include "d_netfil.h"
#include "m_cheat.h"
#include "p_chex.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
  // 3D View Rendering
#endif

#include "hardware/hw3sound.h"

#include "b_game.h"
  //added by AC for acbot

#ifdef FRENCH_INLINE
#include "d_french.h"
#endif


// DoomLegacy version defines are in doomdef.h
// DOOMLEGACY_COMPONENT_VERSION, is in doomincl.h


// [WDJ] This took days to get the preprocessor to do it right.  They must be separate define macros.
#define TO_STR(s)  #s
#define VERSION_CAT( vmaj, vmin, vrev )  TO_STR(vmaj) "." TO_STR(vmin) "." TO_STR(vrev)

// Version number for program use.
const int  VERSION  = DL_VER_MAJ * 100 + DL_VER_MIN; // major*100 + minor
const int  REVISION = DL_VER_REV;  // for bugfix releases, should not affect compatibility. has nothing to do with svn revisions.
//static const char VERSIONSTRING[] = "(rev " SVN_REV ")";
//static const char VERSIONSTRING[] = "beta (rev " SVN_REV ")";

const char VERSION_BANNER[] = "Doom Legacy " VERSION_CAT(DL_VER_MAJ,DL_VER_MIN,DL_VER_REV) " (rev " SVN_REV ")";
//char VERSION_BANNER[80];

// [WDJ] change this if legacy.wad is changed
const short cur_wadversion = 148;	// release wadversion
// usually allow one version behind cur_wadversion, for easier testing
// does not have to be full featured
const short min_wadversion = 145;


// [WDJ] Beware that getting semicolon at end of any of these macros will make concat fail.
#ifdef SDL2
#  define VLIB  "SDL2"
#else
# ifdef SMIF_SDL
#  define VLIB  "SDL1"
# else
#  define VLIB
# endif
#endif

#ifdef LINUX
# ifdef FREEBSD
# define DLOS  "FreeBSD"
# else
# ifdef __OPENBSD__
# define DLOS  "OpenBSD"
# else
# ifdef NETBSD
# define DLOS  "NetBSD"
# else
# define DLOS  "Linux"
# endif
# endif
# endif
#else

// MINGW does not define __WINDOWS__, but has WIN32, _WIN32, __WIN32, __WIN32__, WINNT
#if defined(__WINDOWS__) || defined(WIN32)
# ifdef SMIF_WIN_NATIVE
# define DLOS  "Windows"
# define VLIB  "Native"
# else
# ifdef WIN32
# define DLOS  "Win32"
# else
# define DLOS  "Windows"
# endif
# endif
#else

#ifdef __APPLE__
# ifdef __MACH__
# define DLOS  "Mac MACH"
# else
# define DLOS  "Mac"
# endif
#else

#ifdef __OS2__
# define DLOS  "OS2"
#else
#ifdef SMIF_PC_DOS
# define DLOS  "DOS"
#else
# define DLOS  "Unknown OS"
#endif /*DOS*/
#endif /*OS2*/
#endif /*APPLE*/
#endif /*WINDOWS*/
#endif /*LINUX*/

#ifdef HAVE_ZLIB
#define OPTZL  " ZLIB"
#else
#define OPTZL
#endif
#ifdef HAVE_LIBZIP
#define OPTLZ  " LIBZIP"
#else
#define OPTLZ
#endif
#ifdef HAVE_DLOPEN
#define OPTDLO  " DLOPEN"
#else
#define OPTDLO
#endif

#ifdef DEV_ALSA
# if DEV_ALSA == 3
# define OPTSA  " ALSA(dl)"
# else
# define OPTSA  " ALSA"
# endif
#else
#define OPTSA
#endif
#ifdef DEV_OSS
#define OPTSO  " OSS"
#else
#define OPTSO
#endif
#ifdef DEV_ESD
# if DEV_ESD == 3
# define OPTSE  " ESD(dl)"
# else
# define OPTSE  " ESD"
# endif
#else
#define OPTSE
#endif
#ifdef DEV_PULSE
# if DEV_PULSE == 3
# define OPTSP  " PulseAudio(dl)"
# else
# define OPTSP  " PulseAudio"
# endif
#else
#define OPTSP
#endif
#ifdef DEV_TIMIDITY
# if DEV_TIMIDITY == 3
# define OPTST  " TiMidity(dl)"
# else
# define OPTST  " TiMidity"
# endif
#else
#define OPTST
#endif
#ifdef DEV_FLUIDSYNTH
# if DEV_FLUIDSYNTH == 3
# define OPTSF  " FluidSynth(dl)"
# else
# define OPTSF  " FluidSynth"
# endif
#else
#define OPTSF
#endif

#ifndef VLIB
# define VLIB
#endif

const char DL_OPTS_STR[] = "Build opts: "
 DLOS " " VLIB OPTZL OPTLZ OPTDLO OPTSA OPTSO OPTSE OPTSP OPTST OPTSF;



//
//  DEMO LOOP
//
int demosequence;
int pagetic;
static const char * pagename = "TITLEPIC";
static boolean hs_attract_page = false;   // [Arcade] high-score table page active
static int     hs_subpage_tic = 0;        // [Arcade] tics left on the map on screen
static boolean hs_page_after_demo = false;  // [Arcade] show scores after this demo

// [Arcade] Extra splash page (CREDIT2 in legacy.wad), shown once per attract
// cycle straight after the stock CREDIT page.
static boolean credit2_pending = false;
#define CREDIT2_SECS   6

//  PROTOS
static void Help(void);
static void Clear_SoftError(void);

// Null terminated list of files.
char * startupwadfiles[MAX_WADFILES+1];

// command line switches
boolean nomonsters;             // checkparm of -nomonsters

boolean singletics = false;     // timedemo

boolean nomusic;
boolean nosoundfx; // had clash with WATCOM i86.h nosound() function

boolean dedicated = false;  // dedicated server

byte    verbose = 0;
byte    devparm = 0;
    // set by -devparm, plus verbose level.
    // devparm enables development mode, with CONS messages reporting
    // on memory usage and other significant events.
byte    devmode = 0;
    // set by -devmode.
    // devmode unlocks menu entries that are hidden in a locked-down
    // (e.g. arcade cabinet) build, such as Multiplayer.

byte    demo_ctrl;
byte    init_sequence = 0;
byte    fatal_error = 0;

// name buffer sizes including directory and everything
#define FILENAME_SIZE  256

#if defined SMIF_PC_DOS || defined __WIN32__ || defined __OS2__
# define  SLASH  "\\"
#else
# define  SLASH  "/"
#endif

// to make savegamename and directories, in m_menu.c
char *legacyhome = NULL;
int   legacyhome_len;

char *dirlist[] =
  { DEFWADS01,
#ifdef DEFWADS02
    DEFWADS02,
#endif
#ifdef DEFWADS03
    DEFWADS03,
#endif
#ifdef DEFWADS04
    DEFWADS04,
#endif
#ifdef DEFWADS05
    DEFWADS05,
#endif
#ifdef DEFWADS06
    DEFWADS06,
#endif
#ifdef DEFWADS07
    DEFWADS07,
#endif
#ifdef DEFWADS08
    DEFWADS08,
#endif
#ifdef DEFWADS09
    DEFWADS09,
#endif
#ifdef DEFWADS10
    DEFWADS10,
#endif
#ifdef DEFWADS11
    DEFWADS11,
#endif
#ifdef DEFWADS12
    DEFWADS12,
#endif
#ifdef DEFWADS13
    DEFWADS13,
#endif
#ifdef DEFWADS14
    DEFWADS14,
#endif
#ifdef DEFWADS15
    DEFWADS15,
#endif
#ifdef DEFWADS16
    DEFWADS16,
#endif
#ifdef DEFWADS17
    DEFWADS17,
#endif
#ifdef DEFWADS18
    DEFWADS18,
#endif
#ifdef DEFWADS19
    DEFWADS19,
#endif
#ifdef DEFWADS20
    DEFWADS20,
#endif
#ifdef DEFWADS21
    DEFWADS21,
#endif
  };
// Doomwaddir allocation:
// [0] = DOOMWADDIR.  (ref)
// [1] = reserved for dynamic use  (ref)
// [2] = reserved for dynamic use  (ref)
#define DOOMWADDIR_DIRLIST   3
// [3.. (MAX_NUM_DOOMWADDIR - 3)] = DEFWADSxx   (ref or malloc)
// [MAX_NUM_DOOMWADDIR-2] = reserved for dynamic use  (ref)
// [MAX_NUM_DOOMWADDIR-1] = reserved for dynamic use  (ref)
char * doomwaddir[MAX_NUM_DOOMWADDIR];

static byte defdir_stat = 0;  // when defdir valid
static byte defdir_search = 0;  // when defdir search is reasonable
static char * defdir = NULL;  // default dir  (malloc)
static char * progdir = NULL;  // program dir  (malloc)
static char * progdir_wads = NULL;  // program wads directory  (malloc)

#ifdef LAUNCHER
consvar_t cv_home = {"home", "", CV_HIDEN, NULL};
consvar_t cv_doomwaddir = {"doomwaddir", "", CV_HIDEN, NULL};
consvar_t cv_iwad = {"iwad", "", CV_HIDEN, NULL};
#endif


#if defined(__APPLE__) && defined(__MACH__)
// [WDJ] This is for a setup using an .app folder
# ifdef MAC_RESOURCES_DIR
//[segabor]: for Mac specific resources
char * mac_resource_dir = NULL;  // app Resources (malloc), to find legacy.wad, md2.wad
# endif
# ifdef MAC_HOME_DIR
// [WDJ] Mac OS X uses  ~ as home dir, like on Linux, so this is not needed.
char * mac_user_home = NULL;   // for config and savegames (malloc)
# endif
#endif

// Setup variable doomwaddir for owner usage.
void  owner_wad_search_order( void )
{
    // Wad search order.
    defdir_search = 0;
    if( defdir_stat )
    {
        if( ( access( "Desktop", R_OK) == 0 )
          ||( access( "Pictures", R_OK) == 0 )
          ||( access( "Music", R_OK) == 0 ) )
        {
            if( verbose )
                GenPrintf( EMSG_ver, "Desktop, Pictures, or Music dir detected, default dir not searched.\n");
        }
        else
        if( defdir
            &&( !(strcmp( defdir, cv_home.string ) == 0) ) // not home directory
            &&( !(progdir && (strcmp( defdir, progdir ) == 0)) ) // not program directory
            &&( !(progdir_wads && (strcmp( defdir, progdir_wads ) == 0)) ) // not wads directory
          )
        {
            defdir_search = 1;
            // Search current dir near first, for other wad searches.
            doomwaddir[1] = defdir;
        }
    }
    // Search progdir/wads early, for other wad searches.
    doomwaddir[2] = progdir_wads;
    // Search last, for other wad searches.
    doomwaddir[MAX_NUM_DOOMWADDIR-1] = progdir;
}



//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
// referenced from i_system.c for I_GetKey()

event_t events[MAXEVENTS];
int eventhead = 0;
int eventtail = 0;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(const event_t * ev)
{
    events[eventhead] = *ev;
    eventhead = (eventhead + 1) & (MAXEVENTS - 1);
    last_input_tic = gametic;  // [Arcade lockdown] any raw input counts as activity
}

// just for lock this function
#ifdef SMIF_PC_DOS
void D_PostEvent_end(void)
{
};
#endif

// Clear the input events before re-enabling play
static
void D_Clear_Events( void )
{
   eventhead = eventtail = 0;
   mousex = mousey = 0;  // clear accumulated motion
}

//
// D_Process_Events
// Send all the events of the given timestamp down the responder chain
//
void D_Process_Events(void)
{
    event_t *ev;

    while (eventtail != eventhead)
    {
        ev = &events[eventtail];

        if (M_Responder(ev)) // Menu input
          ;   // menu ate the event
        else if (CON_Responder(ev)) // console input
          ;
        else
          G_Responder(ev);

        eventtail++;
        eventtail = eventtail & (MAXEVENTS - 1);
    }
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// There is a wipe each change of the gamestate.
// wipegamestate can be set to GS_FORCEWIPE to force a wipe on the next draw.
gamestate_e wipegamestate = GS_DEMOSCREEN;

CV_PossibleValue_t screenslink_cons_t[] = { {0, "None"}, {wipe_ColorXForm + 1, "Crossfade"}, {wipe_Melt + 1, "Melt"}, {0, NULL} };
consvar_t cv_screenslink = { "screenlink", "2", CV_SAVE, screenslink_cons_t };

// Not called when dedicated.
static
void D_Display(void)
{
    // vid : from video setup
    static boolean menuactivestate = false;
    static boolean draw_refresh = false;
    static gamestate_e oldgamestate = GS_FORCEWIPE; // invalid state
    static int borderdrawcount = 0;

    tic_t nowtime;
    tic_t tics;
    tic_t wipestart;
    int y;
    boolean wipe_done;
    boolean wipe;
    boolean redrawsbar;

    if (nodrawers)
        return; // for comparative timing / profiling

    wipe = false;
    redrawsbar = false;
    wipe_done = false;

    //added:21-01-98: check for change of screen size (video mode)
    if( setmodeneeded.modetype || drawmode_recalc )
    {
        SCR_SetMode( 1 );  // change video mode
        //added:26-01-98: NOTE! setsizeneeded is set by SCR_Recalc()
        SCR_Recalc();
          // setsizeneeded -> redrawsbar
          // con_recalc, stbar_recalc, am_recalc
        drawmode_recalc = false;
    }

    // change the view size if needed
    if (setsizeneeded)
    {
        R_ExecuteSetViewSize();  // set rdraw, view scale, limits, projection
        oldgamestate = GS_FORCEWIPE;  // force background redraw
        redrawsbar = true;
        draw_refresh = true;
    }

    if( rendermode_recalc )
    {
        if( gamestate == GS_LEVEL )
        {
//            R_FillBackScreen();
            R_Setup_Drawmode();
            draw_refresh = true;
            oldgamestate = GS_FORCEWIPE;  // force background redraw
#ifdef HWRENDER
            if( rendermode != render_soft )	       
            {
                // Hardware draw only
                HWR_SetupLevel();
                HWR_Preload_Graphics();
            }
#endif
        }
    }

    // save the current screen if about to wipe
    if (gamestate != wipegamestate && rendermode == render_soft)
    {
        wipe = true;
        wipe_StartScreen();
    }

    // draw buffered stuff to screen
    // BP: Used only by linux GGI version
    I_UpdateNoBlit();

    // do buffered drawing
    switch (gamestate)
    {
        case GS_LEVEL:
            if (!gametic)
                break;

            // On each gametic
            HU_Erase();
            if (automapactive)
                AM_Drawer();

            if (wipe || menuactivestate
#ifdef HWRENDER
                || rendermode != render_soft
#endif
                || vid.recalc)
            {
                redrawsbar = true;
            }
            break;

        case GS_INTERMISSION:
            WI_Drawer();
            HU_Draw_Tip();   // [Arcade] idle-timeout countdown
            break;

        case GS_FINALE:
            F_Drawer();
            HU_Draw_Tip();   // [Arcade] idle-timeout countdown
            break;

        case GS_DEDICATEDSERVER:
        case GS_DEMOSCREEN:
            if( hs_attract_page )      // [Arcade]
                HS_Draw_AttractTable();
            else
                D_PageDrawer(pagename);
            break;

        case GS_WAITINGPLAYERS:
            // [WDJ] Because hardware may double buffer, need to overwrite
            // background. Waiting loop may get alternating backgrounds.
            D_PageDrawer(pagename);  // provide background
            D_WaitPlayer_Drawer();
            break;

        case GS_NULL:
        default:
            break;
    }

    if (gamestate == GS_LEVEL)
    {
        if (oldgamestate != GS_LEVEL)
        {
            // Level map play display initialize
            R_FillBackScreen(); // draw the pattern into the back screen
            // the border needs to be initially drawn
            draw_refresh = true;
        }

        // Level map play display
        // draw the view directly
        if (!automapactive)
        {
            // see if the border needs to be updated to the screen
            if( rdraw_scaledviewwidth != vid.width )
            {
                // the menu may draw over parts out of the view window,
                // which are refreshed only when needed
                if( menuactive || menuactivestate || draw_refresh )
                    borderdrawcount = 3;

                if( borderdrawcount )
                {
                    borderdrawcount--;
                    R_DrawViewBorder();     // erase old menu stuff
                }
            }

            if (displayplayer_ptr->mo)
            {
#ifdef CLIENTPREDICTION2
                displayplayer_ptr->mo->flags2 |= MF2_DONTDRAW;
#endif
#ifdef HWRENDER
                if (rendermode != render_soft)
                    HWR_RenderPlayerView(0, displayplayer_ptr);
                else    //if (rendermode == render_soft)
#endif
                    R_RenderPlayerView(0, displayplayer_ptr);
#ifdef CLIENTPREDICTION2
                displayplayer_ptr->mo->flags2 &= ~MF2_DONTDRAW;
#endif
            }

            // added 16-6-98: render the second screen
            // [Arcade] ... and the third and fourth.  Views past the first
            // are taken from localplayer[], not from displayplayer2_ptr,
            // which only ever named the second of two.  The hardware
            // renderer places each view in its cell of the grid (see
            // HWR_RenderPlayerView); the software renderer still has only
            // the two stacked half-screen tables, so it draws at most two.
            {
                byte vind, num_views = D_NumViews();
                for( vind=1; vind<num_views; vind++ )
                {
                    byte pn = localplayer[vind];
                    player_t * vpl;

                    if( pn >= MAXPLAYERS )  continue;   // panel with no player
                    vpl = &players[pn];
                    if( ! vpl->mo )  continue;

#ifdef CLIENTPREDICTION2
                    vpl->mo->flags2 |= MF2_DONTDRAW;
#endif
#ifdef HWRENDER
                    if (rendermode != render_soft)
                        HWR_RenderPlayerView(vind, vpl);
                    else
#endif
                    if( vind == 1 )
                    {
                        // Alter the draw tables to draw into second player window
                        //faB: Boris hack :P !!
                        view_window_y = vid.height / 2;
                        memcpy(ylookup, ylookup2, rdraw_viewheight * sizeof(ylookup[0]));

                        R_RenderPlayerView(1, vpl);

                        // Restore first player tables
                        view_window_y = 0;
                        memcpy(ylookup, ylookup1, rdraw_viewheight * sizeof(ylookup[0]));
                    }
#ifdef CLIENTPREDICTION2
                    vpl->mo->flags2 &= ~MF2_DONTDRAW;
#endif
                }
            }
        }

        HU_Drawer();

        ST_Drawer(redrawsbar);
       
        draw_refresh = false;
    }
    else
    {
        // not GS_LEVEL
        // change gamma if needed
        if( gamestate != oldgamestate )
            V_SetPalette(0);
    }

    menuactivestate = menuactive;
    oldgamestate = wipegamestate = gamestate;

    // draw pause pic
    if (paused && (!menuactive || netgame))
    {
        patch_t *patch;
        if (automapactive)
            y = 4;
        else
            y = view_window_y + 4;
        patch = W_CachePatchName("M_PAUSE", PU_CACHE);  // endian fix
        // 0
        V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART );
        V_DrawScaledPatch(view_window_x + (BASEVIDWIDTH - patch->width) / 2, y, patch);
    }

    //added:24-01-98:vid size change is now finished if it was on...
    vid.recalc = 0;
    rendermode_recalc = false;

    // Exl: draw a faded background
    if( fs_fadealpha != 0 )
    {
#ifdef HWRENDER
        if( rendermode != render_soft)
        {
            HWR_FadeScreenMenuBack(fs_fadecolor, fs_fadealpha, vid.height);
        }
        else
#endif
        {
            // Fade for software draw.
            V_FadeRect(0, vid.width, vid.height, (0xFF - fs_fadealpha),
                       (fs_fadealpha * 32 / 0x100), fs_fadecolor );
        }
    }

        //FIXME: draw either console or menu, not the two
    CON_Drawer();

    M_Drawer(); // menu is drawn even on top of everything
    NetUpdate();        // send out any new accumulation

//
// normal update
//
    if (!wipe)
    {
        if (cv_netstat.value)
        {
            char s[50];
            Net_GetNetStat();
            sprintf(s, "recv %d b/s", netstat_recv_bps);
            V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT - ST_HEIGHT - 40, V_WHITEMAP, s);
            sprintf(s, "send %d b/s", netstat_send_bps);
            V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT - ST_HEIGHT - 30, V_WHITEMAP, s);
            sprintf(s, "GameMiss %.2f%%", netstat_gamelost_percent);
            V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT - ST_HEIGHT - 20, V_WHITEMAP, s);
            sprintf(s, "SysMiss %.2f%%", netstat_lost_percent);
            V_DrawString(BASEVIDWIDTH - V_StringWidth(s), BASEVIDHEIGHT - ST_HEIGHT - 10, V_WHITEMAP, s);
        }

#ifdef TILTVIEW
        //added:12-02-98: tilt view when marine dies... just for fun
        if (gamestate == GS_LEVEL && cv_tiltview.value
            && displayplayer_ptr->playerstate == PST_DEAD)
        {
            V_DrawTiltView(screens[0]);
        }
        else
#endif
#ifdef PERSPCORRECT
        if (gamestate == GS_LEVEL && cv_perspcorr.value
           && rendermode == render_soft )
        {
            V_DrawPerspView(screens[0], displayplayer_ptr->aiming);
        }
        else
#endif
        {
            //I_BeginProfile();
            // display a graph of ticrate 
            if (cv_ticrate.value )
                V_Draw_ticrate_graph();
            I_FinishUpdate();   // page flip or blit buffer
            //debug_Printf("last frame update took %d\n", I_EndProfile());
        }
        return;
    }

//
// wipe update
//
    if (!cv_screenslink.value)
        return;

    wipe_EndScreen();

    wipestart = I_GetTime() - 1;
    y = wipestart + 2 * TICRATE;        // init a timeout
    do
    {
        do
        {
            nowtime = I_GetTime();
            tics = nowtime - wipestart;
        } while (!tics);
        wipestart = nowtime;
        wipe_done = wipe_ScreenWipe(cv_screenslink.value - 1, tics);
        I_OsPolling();
        I_UpdateNoBlit();
        M_Drawer();     // menu is drawn even on top of wipes
        I_FinishUpdate();       // page flip or blit buffer
    } while (!wipe_done && I_GetTime() < (unsigned) y);

    ST_Invalidate();
}

// =========================================================================
//   D_DoomLoop
// =========================================================================

tic_t rendergametic;  // The last gametic that was rendered.
#ifdef CLIENTPREDICTION2
boolean spirit_update;
#endif

//#define SAVECPU_EXPERIMENTAL

// Called by port main program.
void D_DoomLoop(void)
{
    char acbuf[_MAX_PATH ];
    tic_t oldentertics, entertic, realtics, rendertimeout = -1;

    // [Arcade] Check the config here, not from M_LoadConfig.  At load time
    // the video mode has not been set and several subsystems have not applied
    // their settings yet, so scr_width, drawmode and friends still read as
    // defaults and every one of them was reported as having failed.  By the
    // time the loop starts the values have settled.
    M_Verify_Config( configfile_main );

    if (demorecording)
        G_BeginRecording();

    // [WDJ] DoomLegacy may be installed local or in system directory.
    // Feature Request by Leonardo Montenegro.
    // There may be a local autoexec, and/or a system autoexec.
    // Standard: The local file is preferred, and can chain to the system file when preferable.
    cat_filename( acbuf, legacyhome, "autoexec.cfg");  // local file in doomlegacy home
    if( access( acbuf, R_OK) == 0 )
    {
        // user settings
        GenPrintf( EMSG_ver, "Exec Local autoexec: %s\n", acbuf );
        COM_BufAddText( va( "exec %s\n", acbuf) );
    }
    else if( access( "autoexec.cfg", R_OK) == 0 )  // file with executable
    {
        // file with executable, may be system settings
        GenPrintf( EMSG_ver, "Exec System autoexec\n" );
        COM_BufAddText("exec autoexec.cfg\n");
    }

    // end of loading screen: CONS_Printf() will no more call FinishUpdate()
    con_self_refresh = false;

    oldentertics = I_GetTime();

    // make sure to do a d_display to init mode _before_ load a level
    if( setmodeneeded.modetype || drawmode_recalc )
    {
        // This may also execute accumulated commands.
        SCR_SetMode( 1 );      // change video mode
        SCR_Recalc();
        drawmode_recalc = false;
    }
    if( rendermode_recalc )
    {
        I_Rendermode_setup();
        rendermode_recalc = 0;
    }

    D_Clear_Events();  // clear input events to prevent startup jerks,
                         // motion during screen wipe still gets through

    // Execute accumulated commands.
    COM_BufExecute( CFG_none );
   
    while (1)
    {
        // get real tics
        entertic = I_GetTime();
        realtics = entertic - oldentertics;
        oldentertics = entertic;

#ifdef SAVECPU_EXPERIMENTAL
        if (realtics == 0)
        {
            I_Sleep(10);
            continue;
        }
#endif

        // frame synchronous IO operations
        // UNUSED for the moment (18/12/98)
        I_StartFrame();

#ifdef HW3SOUND
        if( ! dedicated )
        {
            HW3S_BeginFrameUpdate();
        }
#endif

        // process tics (but maybe not if realtic==0)
        TryRunTics(realtics);
#ifdef CLIENTPREDICTION2
        if (singletics || spirit_update)
#else
        if (singletics || gametic > rendergametic)
#endif
        {
            rendergametic = gametic;
            rendertimeout = entertic + TICRATE / 17;

            if( ! dedicated )
            {
                //added:16-01-98:consoleplayer -> displayplayer (hear sounds from viewpoint)
                S_UpdateSounds();   // move positional sounds
                // Update display, next frame, with current state.
                D_Display();
            }
#ifdef CLIENTPREDICTION2
            spirit_update = false;
#endif
        }
        else if (rendertimeout < entertic)      // in case the server hang or netsplit
        {
            if( ! dedicated )
            {
                D_Display();
            }
        }

        if( ! dedicated )
        {
            //Other implementations might need to update the sound here.
#ifndef SNDSERV
            // Handles sound mixing, and synchronous driver.
            I_UpdateSound();
#endif

#ifdef CDMUS
            // check for media change, loop music..
            I_UpdateCD();
#endif

#ifdef HW3SOUND
            HW3S_EndFrameUpdate();
#endif
        }
    }
}

// =========================================================================
//  Demo
// =========================================================================

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
    if (--pagetic < 0)
    {
        D_AdvanceDemo();
        return;
    }

    // [Arcade] The high-score page is really a stack of one-map pages, and
    // pagetic covers all of them; step to the next map as each elapses.
    // Checked after the pagetic expiry above so the last map is not replaced
    // by a one-tic flash of the first on the way out.
    if( hs_attract_page && (--hs_subpage_tic <= 0) )
    {
        HS_Attract_Advance_Page();
        hs_subpage_tic = TICRATE * HS_PAGE_SECS;
    }
}

//
// D_PageDrawer : draw a patch supposed to fill the screen,
//                fill the borders with a background pattern (a flat)
//                if the patch doesn't fit all the screen.
//
void D_PageDrawer(const char *lumpname)
{
    int x, y;
    byte *src;
    byte *dest;  // within screen buffer

    // [WDJ] Draw patch for all bpp, bytepp, and padded lines.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );

    // software mode which uses generally lower resolutions doesn't look
    // good when the pic is scaled, so it fills space around with a pattern,
    // and the pic is only scaled to integer multiples (x2, x3...)
    if (rendermode == render_soft)
    {
        if ((vid.width > BASEVIDWIDTH) || (vid.height > BASEVIDHEIGHT))
        {
            src = scr_borderflat;
            dest = screens[0];

            for (y = 0; y < vid.height; y++)
            {
                // repeatly draw a 64 pixel wide flat
                dest = screens[0] + (y * vid.ybytes);  // within screen buffer
                for (x = 0; x < vid.width / 64; x++)
                {
                    V_DrawPixels( dest, 0, 64, &src[(y & 63) << 6]);
                    dest += (64 * vid.bytepp);
                }
                if (vid.width & 63)
                {
                    V_DrawPixels( dest, 0, (vid.width & 63), &src[(y & 63) << 6]);
                }
            }
        }
    }
    // big hack for legacy's credits
    if (EN_heretic_hexen && (demosequence != 2))
    {
        V_DrawRawScreen_Num(0, 0, W_GetNumForName(lumpname), 320, 200);
        if (demosequence == 0 && pagetic <= 140)
            V_DrawScaledPatch_Name(4, 160, "ADVISOR" );
    }
    else
    {
        V_DrawScaledPatch_Name(0, 0, lumpname );
    }
}


//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
// Called by D_StartTitle, with init of demosequence
// Called by D_PageTicker, when gamestate == GS_DEMOSCREEN
// Called by G_CheckDemoStatus when timing or playing a demo
void D_AdvanceDemo(void)
{
    // [WDJ] do not start a demo when a menu or console is open
    if( !(demo_ctrl & DEMO_seq_disabled) && ! menuactive
        && ! console_open )
        demo_ctrl = DEMO_seq_advance;    // flag to trigger D_DoAdvanceDemo
}

//
// This cycles through the demo sequences.
// FIXME - version dependent demo numbers?
//
// Called by TryRunTics when demo_ctrl == DEMO_seq_advance
void D_DoAdvanceDemo(void)
{
    const char * demo_name;

    demo_ctrl = 0;  // cancel DEMO_seq_advance
    players[consoleplayer].playerstate = PST_LIVE;      // not reborn
    gameaction = ga_nothing;

    // [Arcade] Show the high score table after each demo rather than once
    // per cycle, so it comes around often.  Interposed here instead of being
    // another demosequence case: the cases are shared between game modes and
    // the last one is reachable only under the retail divisor, so inserting
    // pages into that sequence is what forced it to be renumbered before.
    // Skipped when there is nothing to list, or a fresh cabinet would show
    // an empty page after every demo.
    if( hs_page_after_demo )
    {
        hs_page_after_demo = false;
        if( HS_Have_Records() )
        {
            // Step through every map that has a time, HS_PAGE_SECS each, so
            // the whole table is shown between demos without any one page
            // overflowing.  Length scales with how much has been recorded.
            HS_Attract_Reset_Pages();
            hs_subpage_tic = TICRATE * HS_PAGE_SECS;
            pagetic = (TICRATE * HS_PAGE_SECS * HS_Attract_Page_Count()) - 1;
            hs_attract_page = true;
            gamestate = GS_DEMOSCREEN;
            return;                 // demosequence deliberately not advanced
        }
    }

    // [Arcade] The extra splash page, interposed for the same reason as the
    // high score page above: the demosequence cases are shared between game
    // modes and the last is reachable only under the retail divisor, so a new
    // case would have to renumber them.  Returning without advancing shows
    // this page and then carries on where the sequence left off.
    //
    // Skipped when the lump is absent, so a legacy.wad without CREDIT2 -- an
    // older copy, or another install -- behaves exactly as before.
    if( credit2_pending )
    {
        credit2_pending = false;
        if( VALID_LUMP( W_CheckNumForName("CREDIT2") ) )
        {
            pagetic = TICRATE * CREDIT2_SECS;
            gamestate = GS_DEMOSCREEN;
            pagename = "CREDIT2";
            return;             // demosequence deliberately not advanced
        }
    }

    if (gamemode == ultdoom_retail)
        demosequence = (demosequence + 1) % 7;
    else
        demosequence = (demosequence + 1) % 6;

    hs_attract_page = false;   // [Arcade] cleared for the graphic/demo pages
    HS_Clear_DemoLabel();      // [Arcade] only the record cases below set it

    switch (demosequence)
    {
        case 0:
            pagename = "TITLEPIC";
            switch (gamemode)
            {
                case hexen:
                case heretic:
                    pagetic = 210 + 140;
                    pagename = "TITLE";
                    S_StartMusic(mus_htitl);
                    break;
                case doom2_commercial:
                    pagetic = TICRATE * 11;
                    S_StartMusic(mus_dm2ttl);
                    break;
                default:
                    pagetic = 170;
                    S_StartMusic(mus_intro);
                    break;
            }
            gamestate = GS_DEMOSCREEN;
            break;
        case 1:
            demo_name = HS_NextRecordDemoPath();   // [Arcade]
            if( demo_name == NULL )  demo_name = "demo1";
            goto playdemo;
        case 2:
            pagetic = 200;
            gamestate = GS_DEMOSCREEN;
            pagename = "CREDIT";
            credit2_pending = true;   // [Arcade] extra splash follows this one
            break;
        case 3:
            demo_name = HS_NextRecordDemoPath();   // [Arcade]
            if( demo_name == NULL )  demo_name = "demo2";
            goto playdemo;
        case 4:
            gamestate = GS_DEMOSCREEN;
            if (gamemode == doom2_commercial)
            {
                pagetic = TICRATE * 11;
                pagename = "TITLEPIC";
                S_StartMusic(mus_dm2ttl);
            }
            else if (gamemode == heretic)
            {
                pagetic = 200;
                if( ! VALID_LUMP( W_CheckNumForName("e2m1") ) )
                    pagename = "ORDER";
                else
                    pagename = "CREDIT";
            }
            else
            {
                pagetic = 200;

                if (gamemode == ultdoom_retail)
                    pagename = text[CREDIT_NUM];
                else
                    pagename = text[HELP2_NUM];
            }
            break;
        case 5:
            demo_name = HS_NextRecordDemoPath();   // [Arcade]
            if( demo_name == NULL )  demo_name = "demo3";
            goto playdemo;
            // THE DEFINITIVE DOOM Special Edition demo
        case 6:
            demo_name = "demo4";
            goto playdemo;
    }
    return;

 playdemo:
    G_DeferedPlayDemo( demo_name );
    demo_ctrl = DEMO_seq_playdemo;  // demo started here (not console)
    pagetic = 9999999;
    hs_page_after_demo = true;      // [Arcade] scores follow this demo
    return;
}

// Disable demos
// Called when load game or init new game
void D_DisableDemo(void)
{
    if( demoplayback )
        G_StopDemo();
    // stop DEMO_seq_advance, but preserve DEMO_seq_playdemo so can abort it
    demo_ctrl = (demo_ctrl & DEMO_seq_playdemo) | DEMO_seq_disabled;
}

// =========================================================================

//
// D_StartTitle
//
// Called by D_DoomMain(), when not server and not starting game
// Called by Command_ExitGame_f
// Called by CL_ConnectToServer when cannot join server, or aborting
// Called by Got_KickCmd, when player kicked from game
// Called by Net_Packet_Handler, upon server shutdown, timeout, or refused (NACK)
// Called by G_InitNew, when aborting game because cannot downgrade
// Called by M_Responder and M_Setup_prevMenu, when exiting menu and not playing game
void D_StartTitle(void)
{
    D_End_commandline();

    gameaction = ga_nothing;
    playerdeadview = false;
    displayplayer = consoleplayer = statusbarplayer = 0;
    displayplayer_ptr = consoleplayer_ptr = &players[0]; // [WDJ]
    paused = 0;
    demo_ctrl = 0;  // enable screens and seq demos
    demosequence = -1;
    CON_ToggleOff();
    D_AdvanceDemo();
}

// End commandline game setup.
void D_End_commandline( void )
{
    // Does not affect video settings (CS_EV_PROT)
    if( command_EV_param )
        CV_Restore_User_Settings();  // remove temp settings     
}



// =========================================================================
//   D_DoomMain
// =========================================================================


// Print out the search directories for verbose, and error.
//   emf: EMSG_ver, EMSG_error, EMSG_warn
//   enables: 0x01 legacy.wad order
//            0x02 IWAD order
//            0x0F verbose all
static
void  Print_search_directories( byte emf, byte enables )
{
    int wdi;
    GenPrintf(emf, "Search directories:\n");
    // Extra legacy.wad search, and verbose.
    if( (enables&0x01) && progdir )
        GenPrintf(emf, " progdir: %s\n", progdir );
    // Verbose only. For IWAD or legacy.wad they are in doomwaddir entries.
    if( (enables==0x0F) && progdir_wads )
        GenPrintf(emf, "        : %s\n", progdir_wads );
    if( (enables==0x0F) && defdir && defdir_search )
        GenPrintf(emf, " defdir: %s\n", defdir );
#ifdef LEGACYWADDIR
    GenPrintf(emf, " LEGACYWADDIR: %s\n", LEGACYWADDIR );
#endif
    for( wdi=0; wdi<MAX_NUM_DOOMWADDIR; wdi++ )
    {
        if( doomwaddir[wdi] )
            GenPrintf(emf, " Doomwaddir[%i]: %s\n", wdi, doomwaddir[wdi] );
    }
}



//
// D_AddFile
//
static
void D_AddFile(const char *filename)
{
    int numwadfiles;
    char *newfile;

    if( filename == NULL )
        return;

    // find end of wad files by counting
    for (numwadfiles = 0; startupwadfiles[numwadfiles]; numwadfiles++)
        ;
    if( numwadfiles >= MAX_WADFILES )
        I_Error ( "Too many wadfiles, max=%i.\n", MAX_WADFILES );

    newfile = malloc(strlen(filename) + 1);
    strcpy(newfile, filename);

    startupwadfiles[numwadfiles] = newfile;
}


#ifdef LAUNCHER
static void D_Clear_Files( void )
{
    int i;
    for (i = 0; startupwadfiles[i]; i++)
    {
        free( startupwadfiles[i] );
        startupwadfiles[i] = NULL;
    }
}
#endif


// ==========================================================================
// Identify the Doom version, and IWAD file to use.
// Sets 'gamemode' to determine whether doom_registered/doom2_commercial
// features are available (notable loading PWAD files).
// ==========================================================================

// [WDJ] Title and file names used in GDESC_other table entry, and other uses.
#define DESCNAME_SIZE	 64
char other_iwad_filename[ DESCNAME_SIZE ];
char other_gname[ DESCNAME_SIZE ];
char public_title[] = "Public DOOM";

game_desc_e     gamedesc_id;     // unique game id
game_desc_t     gamedesc;	 // active desc

// [WDJ] List of standard lump names to be checked, that appear in many
// game wads.  The game_desc_table also has a list of names that
// are unique to that game wad.
// This list sets a byte value of which are found.
// Each table entry will have required and reject bit masks.

#define COMMON_LUMP_LIST_SIZE   4
enum lumpname_e {
   LN_MAP01 =0x01, LN_E1M1 =0x02, LN_E2M2 =0x04, LN_TITLE =0x08
};
const char * common_lump_names[ COMMON_LUMP_LIST_SIZE ] = 
{
   "MAP01", "E1M1", "E2M2", "TITLE"
};

// [WDJ] The gname (first) is used to recognize save games, so don't change it.
// Some of the lump check information was obtained from ZDoom docs.
// The startup_title will be centered on the Title page.
// Switch names (idstr) need to be kept to 8 chars, all lowercase, so they
// can be used in file names on all systems.
// This table is the game search order.
// The first entry matching all characteristics will be used !
#define  NUM_GDESC   (GDESC_other+1)	// number of entries in game_desc_table
game_desc_t  game_desc_table[ NUM_GDESC ] =
{
// Free wads should get their own gamemode identity
// GDESC_freedoom: FreeDoom project, DoomII replacement
//  freedoom : prboom-plus, edge
//  freedoom2 : edge, chocolate
   { "FreeDoom", NULL, "freedoom",
        {"freedoom2.wad", "freedoom.wad","fdoom2.wad"}, NULL,
        {"FREEDOOM", NULL}, LN_MAP01, 0,
        0, GDESC_freedoom, doom2_commercial },
// doom2f: french version Doom2, eternity.
//         prboom-plus

// GDESC_freedm: FreeDM project, DoomII deathmatch
//  freedm : edge, chocolate
   { "FreeDM", NULL, "freedm",
        {"freedm.wad","fdoomdm.wad",NULL}, NULL,
        {"FREEDOOM", "FREEDM"}, LN_MAP01, 0,
        0, GDESC_freedm, doom2_commercial },
// GDESC_doom2: doom2wad
   { "Doom2", "DOOM 2: Hell on Earth", "doom2",
        {"doom2.wad",NULL,NULL}, NULL,
        {NULL, NULL}, LN_MAP01, LN_TITLE,
        GD_idwad, GDESC_doom2, doom2_commercial },
// GDESC_freedoom_ultimate: FreeDoom project, Ultimate Doom replacement
//  freedoom1 : edge, chocolate
   { "Ultimate FreeDoom", NULL, "freedu",
        {"freedoom1.wad", "freedu.wad","fdoomu.wad"}, NULL,
        {"FREEDOOM", "E4M1"}, LN_E1M1+LN_E2M2, 0,
        0, GDESC_freedoom_ultimate, ultdoom_retail },
// GDESC_ultimate: Doom1 1995, doomuwad
//                 Doom1 1995 on floppy (doom_se.wad)
   { "Ultimate Doom", "The Ultimate DOOM", "doomu",
        {"doomu.wad","doom_se.wad","doom.wad"}, NULL,
        {"E4M1", NULL}, LN_E1M1+LN_E2M2, LN_TITLE,
        GD_idwad, GDESC_ultimate, ultdoom_retail },
// GDESC_doom: DoomI 1994, doomwad
   { "Doom", "DOOM Registered", "doom",
        {"doom.wad",NULL,NULL}, NULL,
        {"E3M9", NULL}, LN_E1M1+LN_E2M2, LN_TITLE,
        GD_idwad, GDESC_doom, doom_registered },
// GDESC_doom_shareware: DoomI shareware, doom1wad
//  doom1 : edge, chocolate,
   { "Doom shareware", "DOOM Shareware", "doom1",
        {"doom1.wad","doom.wad",NULL}, NULL,
        {NULL, NULL}, LN_E1M1, LN_TITLE,
        GD_idwad, GDESC_doom_shareware, doom_shareware },
// GDESC_plutonia: FinalDoom : Plutonia, DoomII engine
   { "Plutonia", "DOOM 2: Plutonia Experiment", "plutonia",
        {"plutonia.wad",NULL,NULL}, NULL,
        {"CAMO1", NULL}, LN_MAP01, LN_TITLE,
        GD_idwad, GDESC_plutonia, doom2_commercial },
// GDESC_tnt: FinalDoom : Tnt Evilution, DoomII engine
   { "Tnt Evilution", "DOOM 2: TNT - Evilution", "tnt",
        {"tnt.wad",NULL,NULL}, NULL,
        {"REDTNT2", NULL}, LN_MAP01, LN_TITLE,
        GD_idwad, GDESC_tnt, doom2_commercial },
// GDESC_blasphemer: FreeDoom project, DoomII replacement
   { "Blasphemer", NULL, "blasphem",
        {"BLASPHEM.WAD","blasphem.wad","heretic.wad"}, NULL,
        {"BLASPHEM", NULL}, LN_E1M1+LN_TITLE, 0,
        0, GDESC_blasphemer, heretic },
// GDESC_heretic: Heretic
   { "Heretic", NULL, "heretic",
        {"heretic.wad",NULL,NULL}, NULL,
        {NULL, NULL}, LN_E1M1+LN_E2M2+LN_TITLE, 0,
        GD_idwad, GDESC_heretic, heretic },
// GDESC_heretic_shareware: Heretic shareware
//  heretic1 : chocolate
   { "Heretic shareware", NULL, "heretic1",
        {"heretic1.wad","heretic.wad",NULL}, NULL,
        {NULL, NULL}, LN_E1M1+LN_TITLE, LN_E2M2,
        GD_idwad, GDESC_heretic_shareware, heretic },
// GDESC_hexen: Hexen
   { "Hexen", NULL, "hexen",
        {"hexen.wad",NULL,NULL}, NULL,
        {"MAP40", NULL}, LN_MAP01+LN_TITLE, 0,
        GD_idwad|GD_unsupported, GDESC_hexen, hexen },
// GDESC_hexen_demo: Hexen
   { "Hexen Demo", NULL, "hexen1",
        {"hexen1.wad","hexen.wad",NULL}, NULL,
        {NULL, NULL}, LN_MAP01+LN_TITLE, 0,
        GD_idwad|GD_unsupported, GDESC_hexen_demo, hexen },
// GDESC_strife: Strife
   { "Strife", NULL, "strife",
        {"strife.wad",NULL,NULL}, NULL,
        {"ENDSTRF", "MAP20"}, LN_MAP01, 0,
        GD_idwad|GD_unsupported, GDESC_strife, strife },
// GDESC_strife_shareware: Strife shareware
   { "Strife shareware", NULL, "strife0",
        {"strife0.wad","strife.wad",NULL}, NULL,
        {"ENDSTRF", NULL}, 0, LN_MAP01,
        GD_idwad|GD_unsupported, GDESC_strife_shareware, strife },
// GDESC_chex1: Chex Quest
   { "Chex Quest", NULL, "chex1",
        {"chex1.wad","chex.wad",NULL}, NULL,
        {"W94_1", "POSSH0M0"}, LN_E1M1, LN_TITLE,
        GD_iwad_pref, GDESC_chex1, chexquest1 },
// GDESC_ultimate_mode: Ultimate Doom replacement
   { "Ultimate mode", NULL, "ultimode",
        {"doomu.wad","doom.wad",NULL}, NULL,
        { NULL, NULL}, LN_E1M1, 0,
        0, GDESC_ultimate, ultdoom_retail },
// GDESC_doom_mode: DoomI replacement
   { "Doom mode", NULL, "doommode",
        {"doom1.wad","doom.wad",NULL}, NULL,
        { NULL, NULL}, LN_E1M1, 0,
        0, GDESC_doom_mode, doom_registered },
// GDESC_heretic_mode: Heretic replacement
   { "Heretic mode", NULL, "heremode",
        {"heretic.wad",NULL,NULL}, NULL,
        { NULL, NULL}, LN_E1M1, 0,
        0, GDESC_heretic_mode, heretic },
// GDESC_hexen_mode: Hexen replacement
   { "Hexen mode", NULL, "hexemode",
        {"hexen.wad",NULL,NULL}, NULL,
        { NULL, NULL}, LN_MAP01, 0,
        GD_unsupported, GDESC_hexen_mode, hexen },
// GDESC_other: Other iwads, all DoomII features enabled,
// strings are ptrs to buffers
   { other_gname, public_title, "",
        {other_iwad_filename,NULL,NULL}, NULL,
        { NULL, NULL}, LN_MAP01, 0,
        GD_iwad_pref, GDESC_other, doom2_commercial }
};

#ifdef LAUNCHER
// Lookup game by table index
game_desc_t *  D_GameDesc( int i )
{
    if ( i<0 || i > NUM_GDESC-1 )   return NULL;
    return  &game_desc_table[i];
}
#endif


// [Arcade] Is the IWAD for a game present on the wad search paths?
// Used by the Select Game menu so it only offers games that can be started.
//   idstr : the -game short name, as in the game_desc_table ("doom2", "doomu")
// Must not be called before the doomwaddir search paths are set up.
boolean  D_Game_Available( const char * idstr )
{
    char  pathbuf[MAX_WADPATH];
    int   gmi, w;

    if( ! idstr )  return false;

    for( gmi = 0; gmi < NUM_GDESC; gmi++ )
    {
        game_desc_t * gmtp = & game_desc_table[gmi];

        if( ! gmtp->idstr || strcasecmp( gmtp->idstr, idstr ) != 0 )
            continue;

        // Any one of the possible filenames will do.
        for( w = 0; w < 3; w++ )
        {
            if( gmtp->iwad_filename[w] == NULL )  break;
            if( Search_doomwaddir( gmtp->iwad_filename[w], GAME_SEARCH_DEPTH,
                                   /*OUT*/ pathbuf ) != FS_NOTFOUND )
                return true;
        }
        return false;   // matched the game, but found no iwad for it
    }
    return false;   // no such game
}


// Check all lump names in lumpnames list, count is limited to 8
// Return byte has a bit set for each lumpname found.
static
byte  Check_lumps( const char * wadname, const char * lumpnames[], int count )
{
#ifdef ZIPWAD
    byte  zhand;
#else
    FILE * 	wadfile;
#endif
    const char * reason;
    wadinfo_t   header;
    filelump_t  lumpx;
    int         hli, lc, bc;
    byte        result = 0;

    // This routine checks the directory, using the system file cache
    // instead of making an internal lumps directory.
    // Speed is not required here, so no extra speedup checks.

    // List may be fixed length, with NULL entries.
    while( count>0 && (lumpnames[count-1] == NULL))
    {
       count--;    // Reduce count by the number of NULL entries on end.
       // It is easier to consider a NULL as always found.
       result |= 1<<count;
    }
    if( count == 0 )  goto ret_result;  // escape NULL list
   
    // Read the wad file header and get directory
#ifdef ZIPWAD
    // This handles normal files, and zip archive files.
    zhand = WZ_open( wadname );
    if( zhand == 0 )
        goto open_err;

    bc = WZ_read( zhand, sizeof(header), /*OUT*/ (byte*)&header );
    if( bc < sizeof(header) )
        goto read_err;
#else
    wadfile = fopen( wadname, "rb" );
    if( wadfile == NULL )
        goto open_err;

    fread( &header, sizeof(header), 1, wadfile);
#endif

    // check for IWAD or PWAD
    if( strncmp(header.identification+1,"WAD",3) != 0 )
        goto not_a_wad;

    // find directory
    header.numlumps = LE_SWAP32(header.numlumps);
    header.infotableofs = LE_SWAP32(header.infotableofs);

#ifdef ZIPWAD
    bc = WZ_seek( zhand, header.infotableofs ); 
#else
    bc = fseek( wadfile, header.infotableofs, SEEK_SET ); 
#endif
    if( bc < 0 )
        goto read_err;

    // Check the directory as it is read out of the system file cache.
    for( hli=0; hli<header.numlumps; hli++ )
    {
#ifdef ZIPWAD
        bc = WZ_read( zhand, sizeof(lumpx), /*OUT*/ (byte*)&lumpx );
        if( bc < sizeof(lumpx) )
            goto read_err;
#else
        bc = fread( &lumpx, sizeof(lumpx), 1, wadfile );
        if( bc < 1 )
            goto read_err;
#endif

#ifdef DETECT_TITLES
        if( strncasecmp( lumpx.name, "TITLE", 5 ) == 0 )
        {
            printf( "Lump=%8s : ", lumpx.name );
            for( lc=0; lc<count; lc++ )
            {
               int cmp = strncasecmp( lumpx.name, lumpname[lc], 8 );
               printf( "  %c %8s", ( (cmp<0)?'<': (cmp>0)? '>' :'='), lumpname);
            }
            printf( "\n" );
        }
#endif
        for( lc=0; lc<count; lc++ )
        {
            if( strncasecmp( lumpx.name, lumpnames[lc], 8 ) == 0 )
                result |= 1<<lc;  // found it, record it
        }
    }
#ifdef ZIPWAD
    WZ_close( zhand );
#else
    fclose( wadfile );
#endif

ret_result:   
    return result;	// default is not found

    // Should only be called with known WAD files
not_a_wad:
    reason = "Not a WAD file";
    goto err_ret0;
   
read_err:  // read and seek errors
    reason = "Wad file err";
    goto err_ret0;

open_err:
    reason = "Wad file open err";

err_ret0:
    GenPrintf( EMSG_error, "File %s: %s\n", wadname, reason );
#ifdef ZIPWAD
    if( zhand )    WZ_close( zhand );
#else
    if( wadfile )  fclose( wadfile );
#endif
    return 0;
}


static
boolean Check_keylumps ( game_desc_t * gmtp, const char * wadname )
{
    byte lumpbits;
    if( gmtp->require_lump | gmtp->reject_lump )
    {
        lumpbits = Check_lumps( wadname,
                               common_lump_names, COMMON_LUMP_LIST_SIZE );
        if((gmtp->require_lump & lumpbits) != gmtp->require_lump )  goto fail;
        if( gmtp->reject_lump & lumpbits )  goto fail;
    }
    // Check 2 unique lump names, NULLS are treated as found.
    lumpbits = Check_lumps( wadname, &(gmtp->keylump[0]), 2 );
    if( lumpbits != 0x03 )   goto fail;
    return 1;  // lump checks successful
fail:       
    return 0;  // does not match
}


// Checks the possible wad filenames in GDESC_ entry.
// Return true when found and keylumps verified
// Leaves name in pathbuf_p, which must be of MAX_PATH length.
static
boolean  Check_wad_filenames( int gmi, /*OUT*/ char * pathbuf_p )
{
    game_desc_t * gmtp = &game_desc_table[gmi];
    filestatus_e fse;
    int w;
    // check each possible filename listed
    for( w=0; w<3; w++ )
    {
        if( gmtp->iwad_filename[w] == NULL )
            break;

        fse = Search_doomwaddir( gmtp->iwad_filename[w], GAME_SEARCH_DEPTH,
                               /*OUT*/ pathbuf_p );
      
#ifdef ZIPWAD
        if( fse == FS_ZIP )
        {
            // Check file in archive
            WZ_open_archive( pathbuf_p );
            byte fnd = Check_keylumps( gmtp, gmtp->iwad_filename[w] );
            WZ_close_archive();
            if( fnd )
                return true;
        }
        else
#endif
        if( fse == FS_FOUND )
        {
            // File exists.
            if( Check_keylumps( gmtp, pathbuf_p ) )
                return true;
        }
    }
    return false;
}


// May be called again after command restart
static
// [Arcade] The operator's boot game, as a game_desc_table idstr ("doomu",
// "doom2", ...), or empty for none.
char  default_game_idstr[16] = "";

// [Arcade] Read cv_defaultgame's value straight out of config.cfg.
//
// It cannot be read as a cvar: M_LoadConfig does not run until several hundred
// lines after IdentifyVersion has already chosen the IWAD, so by the time the
// cvar holds anything it is far too late to act on it.  This is the same
// ordering trap that forces the menu's game-dependent setup into M_Configure.
//
// A targeted parse of one line is deliberately preferred over moving the
// config load earlier, which would change startup ordering for everything.
static void D_Read_Default_Game( void )
{
    FILE * f;
    char line[256];

    default_game_idstr[0] = '\0';

    if( ! configfile_main )  return;
    f = fopen( configfile_main, "r" );
    if( ! f )  return;   // no config yet; first run

    while( fgets( line, sizeof(line), f ) )
    {
        char * p, * e;

        // The cvar's PossibleValue strings are the idstr names themselves, so
        // the quoted value can be used as-is.  Config writes: defaultgame "doom2"
        if( strncmp( line, "defaultgame", 11 ) != 0 )  continue;

        p = strchr( line, '"' );
        if( ! p )  break;
        e = strchr( ++p, '"' );
        if( ! e )  break;
        *e = '\0';

        if( strcmp( p, "None" ) != 0 )
            dl_strncpy( default_game_idstr, p, sizeof(default_game_idstr) );
        break;
    }
    fclose( f );
}


void IdentifyVersion()
{
    char pathiwad[_MAX_PATH + 16];
    // debug_Printf("MAX_PATH: %i\n", _MAX_PATH);

    boolean  other_names = 0;	// indicates -iwad other names
#ifdef DEVPARM_LOADING
    boolean  devgame = false;   // indicates -devgame <game>
#endif

    int gamedesc_index = NUM_GDESC; // nothing
    int gmi;

    // find legacy.wad, IWADs
    // and... Doom LEGACY !!! :)
    char *legacywad = NULL;

    if( verbose )
    {
        Print_search_directories( EMSG_ver, 0x0F );
    }


#if defined(__APPLE__) && defined(__MACH__) && defined( MAC_RESOURCES_DIR )
    //[segabor]: on Mac OS X legacy.wad is within .app folder
    // for uniformity, use the strdup at found_legacy_wad
    if( mac_resource_dir && ( access( mac_resource_dir, R_OK) == 0 ) )
    {
        cat_filename( pathiwad, mac_resource_dir, "legacy.wad" );
//        cat_filename( pathiwad, mac_resource_dir, "md2.wad" );
        if( access( pathiwad, R_OK) == 0 )    goto found_legacy_wad;
    }
    // check other locations
#endif

    // [WDJ]: find legacy.wad .
    // pathiwad must be MAX_WADPATH to be used by cat_filename.
    // Look in program directory first, because executable may have
    // its own version of legacy.wad.
    if( progdir && ( access( progdir, R_OK) == 0 ) )
    {
        // [WDJ] look for legacy.wad with doomlegacy
        cat_filename(pathiwad, progdir, "legacy.wad");
        if( access( pathiwad, R_OK) == 0 )   goto found_legacy_wad;
    }

#ifdef LEGACYWADDIR
    // [WDJ] Try LEGACYWADDIR as the first wad dir.
    if( access( LEGACYWADDIR , R_OK) == 0 )
    {
        // [WDJ] legacy.wad is in shared directory
        cat_filename(pathiwad, LEGACYWADDIR, "legacy.wad");
        if( access( pathiwad, R_OK) == 0 )   goto found_legacy_wad;
    }
#endif

    // Search wad directories.
    doomwaddir[1] = progdir_wads;
    // Should not be zipped   
    if( Search_doomwaddir( "legacy.wad", 0, /*OUT*/ pathiwad ) == FS_FOUND )
         goto found_legacy_wad;

    I_SoftError( "legacy.wad not found\n" );  // fatal exit
    GenPrintf(EMSG_error, "Looked for legacy.wad in:\n" );
    Print_search_directories( EMSG_error, 0x01 );
    goto fatal_err;
   
   
 found_legacy_wad:
    legacywad = strdup( pathiwad );  // malloc
    doomwaddir[1] = NULL;

    if( verbose )
    {
        GenPrintf(EMSG_ver, "Legacy.wad: %s\n", legacywad );
    }

    owner_wad_search_order();

    /*
       French stuff.
       doom2fwad = malloc(strlen(doomwaddir)+1+10+1);
       sprintf(doom2fwad, "%s/doom2f.wad", doomwaddir);
     */

    // [WDJ] were too many chained ELSE. Figured it out once and used direct goto.

#ifdef DEVPARM_LOADING
    // [WDJ] Old switches -shdev, -regdev, -comdev are now -devgame <game>
    // Earlier did direct test of -devparm, do not overwrite it.
    devgame = M_CheckParm("-devgame");
    // [WDJ] search for one of the listed GDESC_ forcing switches
    boolean have_game_parm = ( devgame || M_CheckParm("-game") );
#else
    boolean have_game_parm = ( M_CheckParm("-game") != 0 );
#endif

    // [Arcade] With no -game switch, boot the game the operator chose
    // (cv_defaultgame, read out of config.cfg by D_Read_Default_Game before
    // this runs -- the config proper is not loaded until long after the IWAD
    // has been picked, so the cvar itself is useless here).
    //
    // Validated *before* entering the block below rather than inside it: the
    // block's game_switch_found label lives inside its own braces, and an
    // unrecognized value there takes a fatal error path.  A default naming a
    // game that has since been uninstalled must not stop the cabinet booting,
    // so anything wrong here just warns and drops through to the normal
    // search.  An explicit -iwad also wins over the default.
    const char * boot_game = NULL;
    if( ! have_game_parm && ! M_CheckParm("-iwad") && default_game_idstr[0] )
    {
        int  bi;
        for( bi=0; bi<GDESC_other; bi++ )
        {
            if( !strcmp(default_game_idstr, game_desc_table[bi].idstr) )  break;
        }
        if( bi >= GDESC_other )
            GenPrintf( EMSG_warn, "Boot game \"%s\" not recognized, using normal search.\n",
                       default_game_idstr );
        else if( ! D_Game_Available( default_game_idstr ) )
            GenPrintf( EMSG_warn, "Boot game \"%s\" is not installed, using normal search.\n",
                       default_game_idstr );
        else
            boot_game = default_game_idstr;
    }

    if ( have_game_parm || boot_game )
    {
        // boot_game is already known good; only the switch can be malformed.
        char *temp = boot_game ? (char*) boot_game : M_GetNextParm();
        if( temp == NULL )
        {
#ifdef DEVPARM_LOADING
            I_SoftError( "Switch  -game <name> or -devgame <name>\n" );
#else
            I_SoftError( "Switch  -game <name>\n" );
#endif
            goto fatal_err;
        }

        for( gmi=0; gmi<GDESC_other; gmi++ )
        {
            // compare to recognized game mode names
            if (!strcmp(temp, game_desc_table[gmi].idstr))
                goto game_switch_found;
        }
        I_SoftError( "Switch  -game %s  not recognized\n", temp );
        goto fatal_err;
       
       game_switch_found:
        // switch forces the GDESC_ selection
        gamedesc_index = gmi;
        gamedesc = game_desc_table[gamedesc_index]; // copy the game descriptor
        if (gamedesc.gameflags & GD_unsupported)  goto unsupported_wad;

#ifdef DEVPARM_LOADING
        // handle the recognized special -devgame switch
        if( devgame )
        {
            devparm = 1 + verbose;
#if 0
            M_Set_configfile_main( DEVDATA CONFIGFILENAME );
            // [WDJ] Old, irrelevant, and it was interfering with new
            // GDESC changes.
            // Better to just use -file so I am disabling it.
            switch( gamedesc_index )
            {
             case GDESC_doom_shareware:
               // instead use:
               //  doomlegacy -devgame doom1 -file data_se/texture1.lmp data_se/pnames.lmp
               D_AddFile(DEVDATA "doom1.wad");
               D_AddFile(DEVMAPS "data_se/texture1.lmp");
               D_AddFile(DEVMAPS "data_se/pnames.lmp");
               goto got_iwad;
             case GDESC_doom:
               // instead use:
               //   doomlegacy -devgame doom1 -file data_se/texture1.lmp data_se/texture2.lmp data_se/pnames.lmp
               D_AddFile(DEVDATA "doom.wad");
               D_AddFile(DEVMAPS "data_se/texture1.lmp");
               D_AddFile(DEVMAPS "data_se/texture2.lmp");
               D_AddFile(DEVMAPS "data_se/pnames.lmp");
               goto got_iwad;
             case GDESC_doom2:
               // instead use:
               //   doomlegacy -devgame doom2 -file cdata/texture1.lmp cdata/pnames.lmp
               D_AddFile(DEVDATA "doom2.wad");
               D_AddFile(DEVMAPS "cdata/texture1.lmp");
               D_AddFile(DEVMAPS "cdata/pnames.lmp");
               goto got_iwad;
            }
#endif
        }
#endif
    }
   

    // Specify the name of the IWAD file to use, so we can have several IWAD's
    // in the same directory, and/or have legacy.exe only once in a different location
    if (M_CheckParm("-iwad"))
    {
        filestatus_e fse = FS_NOTFOUND;

        // BP: big hack for fullpath wad, we should use wadpath instead in d_addfile
        char *s = M_GetNextParm();
        if ( s == NULL )
        {
            I_SoftError("Switch -iwad <filename>.\n");
            goto fatal_err;
        }

        const char * ipath = file_searchpath( s );
        if( ipath )
        {
            // Absolute or relative path, no search.
            cat_filename( pathiwad, ipath, s );
        }
        else
        {
            // Simple filename.
            // Find the IWAD in the doomwaddir.
            fse = Search_doomwaddir( s, IWAD_SEARCH_DEPTH, /*OUT*/ pathiwad );
#ifdef ZIPWAD
            if( fse == FS_ZIP )
            {
                // Found zip archive in doomwaddir.
//                cat_filename( pathiwad, "", s );
            }
            else
#endif
            if( fse != FS_FOUND )
            {
                // Not found in doomwaddir.
                cat_filename( pathiwad, "", s );
            }
        }

#ifdef LAUNCHER
        CV_Set( & cv_iwad, pathiwad );  // for launcher
        cv_iwad.state &= ~CS_MODIFIED;
#endif

        if ( access(pathiwad, R_OK) < 0 )
        {
            I_SoftError("IWAD %s not found\n", s);
            Print_search_directories( EMSG_error, 0x02 );
            goto fatal_err;
        }

        char *filename = FIL_Filename_of( pathiwad );
#ifdef ZIPWAD
        char  filename_wad[MAX_WADPATH];
        if( fse == FS_ZIP )
        {
            // Need archive name for lookup.
            WZ_save_archive_name( filename );
            // Need wad name for GDESC lookup.
            if( WZ_make_name_with_extension( filename, "wad", /*OUT*/ filename_wad )  )
                filename = filename_wad;  // use wad name instead of archive name
        }
#endif

        if ( gamedesc_index == NUM_GDESC ) // check forcing switch
        {
            // No forcing switch
            // [WDJ] search game table for matching iwad name
#ifdef ZIPWAD
            if( fse == FS_ZIP )
            {
                // Check file in archive
                WZ_open_archive( pathiwad );
            }
#endif
            for( gmi=0; gmi<GDESC_other; gmi++ )
            {
                game_desc_t * gmtp = &game_desc_table[gmi];
                int w;
                // check each possible filename listed
                for( w=0; w<3; w++ )
                {
                    if( gmtp->iwad_filename[w] == NULL )
                        break;  // end of list

                    if( strcasecmp(gmtp->iwad_filename[w], filename) == 0 )
                    {
#ifdef ZIPWAD
                        if( fse == FS_ZIP )
                        {
                            // Check file in archive
                            byte fnd = Check_keylumps( gmtp, filename );
                            if( fnd )
                                goto got_gmi_iwad;
                        }
                        else
#endif
                        if( Check_keylumps( gmtp, pathiwad ) )
                            goto got_gmi_iwad;
                    }
                }
            }
            // unknown IWAD is GDESC_other
            gamedesc_index = GDESC_other;
#ifdef ZIPWAD
            if( archive_open )
            {
                WZ_close_archive();
            }
#endif
        }

        other_names = 1;        // preserve other names when forcing switch
        // for save game header
        dl_strncpy( other_iwad_filename, filename, DESCNAME_SIZE );
        // create game name from the wad name, used in save game
        dl_strncpy( other_gname, other_iwad_filename, DESCNAME_SIZE );
               // use the wad name, without the ".wad" as the gname
        {
            char * dp = strchr( other_gname, '.' );
            if( dp )  *dp = 0;
        }
        goto got_iwad;
    }
    // No -iwad switch:
    // [WDJ] Select IWAD by game switch
    if(gamedesc_index < GDESC_other)  // selected by switch, and no -iwad
    {
        // make iwad name by switch
        // use pathiwad to output wad path from Check_wad_filenames
        if( Check_wad_filenames( gamedesc_index, pathiwad ) )
            goto got_iwad;

        I_SoftError("IWAD %s not found in:\n",
                     game_desc_table[gamedesc_index].iwad_filename[0]);
        Print_search_directories( EMSG_error, 0x02 );
        goto fatal_err;
    }
    // No -iwad switch, and no mode select switch:
    // [WDJ] search the table for the first iwad filename found
    for( gmi=0; gmi<GDESC_other; gmi++ )
    {
        // use pathiwad to output wad path from Check_wad_filenames
        if( Check_wad_filenames( gmi, pathiwad ) )
            goto got_gmi_iwad;
    }

    I_SoftError("Main WAD file not found\n"
            "You need doom.wad, doom2.wad, heretic.wad or some other IWAD file\n"
            "from any shareware, commercial or free version of Doom or Heretic!\n"
#if !defined(__WIN32__) && !(defined __DJGPP__)
            "If you have one of those files, be sure its name is lowercase\n"
            "or use the -iwad command line switch.\n"
#endif
            );
    goto fatal_err;

 got_gmi_iwad:
    gamedesc_index = gmi;  // a search loop found it
 got_iwad:
    gamedesc = game_desc_table[gamedesc_index]; // copy the game descriptor

    if( other_names )  // keep names from -iwad
    {
        gamedesc.gname = other_gname;
        gamedesc.iwad_filename[0] = other_iwad_filename;
    }
    gamedesc_id = gamedesc.gamedesc_id;
    G_set_gamemode( gamedesc.gamemode );
    GenPrintf( EMSG_info, "IWAD recognized: %s\n", gamedesc.gname);

    if (gamedesc.gameflags & GD_unsupported)  goto unsupported_wad;

    D_AddFile(pathiwad);
    D_AddFile(legacywad);  // So can replace some graphics with Legacy ones.
    if( gamedesc.gameflags & GD_iwad_pref )
    {
       // Because legacy.wad replaced some things it shouldn't, give the iwad
       // preference from both search directions.
       // Chexquest1: legacy.wad was replacing the green splats, with bloody ones.
       D_AddFile(pathiwad);
    }
    if( gamedesc.support_wad )
       D_AddFile( gamedesc.support_wad );

cleanup_ret:
#ifdef ZIPWAD
    if( archive_open )
    {
        WZ_close_archive();
    }
#endif
    free(legacywad);  // from strdup, free local copy of name
    return;

unsupported_wad:
    I_SoftError("Doom Legacy currently does not support this game.\n");
    goto fatal_err;
   
fatal_err:
    if( legacywad )
    {
        // [WDJ] Load legacywad if possible, because it contains parts of the
        // user interface, and there will misleading errors.
        D_AddFile(legacywad);  // To prevent additional errors.
    }
    fatal_error = 1;
    goto cleanup_ret;
}

/* ======================================================================== */
// Just print the nice red titlebar like the original DOOM2 for DOS.
/* ======================================================================== */
#ifdef SMIF_PC_DOS
void D_Titlebar(const char *title1, const char *title2)
{
    // DOOM LEGACY banner
    clrscr();
    textattr((BLUE << 4) + WHITE);
    clreol();
    cputs(title1);

    // standard doom/doom2 banner
    textattr((RED << 4) + WHITE);
    clreol();
    gotoxy((80 - strlen(title2)) / 2, 2);
    cputs(title2);
    normvideo();
    gotoxy(1, 3);
}
#endif

#define MAX_TITLE_LEN   80
static char legacytitle[MAX_TITLE_LEN+1];  // length of line

//added:11-01-98:
//
//  Center the title string, then add the date and time of compilation.
//
static void D_Make_legacytitle(void)
{
  const char *s = VERSION_BANNER;
  int i;

  memset(legacytitle, ' ', sizeof(legacytitle));

  for (i = (MAX_TITLE_LEN - strlen(s)) / 2; *s; )  // center
    legacytitle[i++] = *s++;

  const char *u = __DATE__;
  for (i = 0; i < 11; i++)
    legacytitle[i + 1] = u[i]; 

  u = __TIME__;
  for (i = 0; i < 8; i++)
    legacytitle[i + 71] = u[i];

  legacytitle[MAX_TITLE_LEN] = '\0';
}


// Check Legacy.wad
void D_CheckWadVersion()
{
    int wadversion = 0;
    char hs[128];
    int wv2, hlen;
    lumpnum_t ver_lumpnum;
/* BP: disabled since this should work fine now...
    // check main iwad using demo1 version 
    ver_lumpnum = W_CheckNumForNameFirst("demo1");
    // well no demo1, this is not a main wad file
    if( ! VALID_LUMP(ver_lumpnum) )
        I_Error("%s is not a Main wad file (IWAD)\n"
                "try with Doom.wad or Doom2.wad\n"
                "\n"
                "Use -nocheckwadversion to remove this check,\n"
                "but this can cause Legacy to hang\n",wadfiles[0]->filename);
    W_ReadLumpHeader (lump,&wadversion,1);
    if( wadversion<109 )
        I_Error("Your %s file is version %d.%d\n"
                "Doom Legacy need version 1.9\n"
                "Upgrade your version to 1.9 using IdSofware patch\n"
                "\n"
                "Use -nocheckwadversion to remove this check,\n"
                "but this can cause Legacy to hang\n",wadfiles[0]->filename,wadversion/100,wadversion%100);
*/
    // check version, of legacy.wad using version lump
    ver_lumpnum = W_CheckNumForName("version");
    if( ! VALID_LUMP(ver_lumpnum) )
    {
        I_SoftError("No legacy.wad file.\n");
        fatal_error = 1;
        return;
    }
    hlen = W_ReadLumpHeader(ver_lumpnum, &hs, 128);
    if (hlen < 128)
    {
        hs[hlen] = '\0';
        if (sscanf(hs, "Doom Legacy WAD V%d.%d", &wv2, &wadversion) == 2)
          wadversion += wv2 * 100;
    }
    if (wadversion != cur_wadversion)
    {
        I_SoftError("Your legacy.wad file is version %d.%d, you need version %d.%d\n"
                "Use the legacy.wad that came in the same archive as this executable.\n"
                "\n"
                "Use -nocheckwadversion to remove this check,\n"
                "but this can cause Legacy to crash.\n",
                (wadversion / 100), (wadversion % 100),
                (int)(cur_wadversion / 100), (int)(cur_wadversion % 100) );
        if( wadversion < min_wadversion )
            fatal_error = 1;
    }
}

//
// D_DoomMain
//
// Called from port main program to processes setup.
// Returns before game starts.
void D_DoomMain()
{
    int p, wdi;
#ifdef DEVPARM_LOADING
    char fbuf[FILENAME_SIZE];
#endif
    char dirbuf[_MAX_PATH ];
    char cfgbuf[_MAX_PATH ];

    int startepisode;
    int startmap;
    boolean autostart;

#ifdef FRENCH_INLINE
    french_early_text();
#endif

    // print version banner just once here, use it anywhere
//    sprintf(VERSION_BANNER, "Doom Legacy %d.%d.%d %s", VERSION/100, VERSION%100, REVISION, VERSIONSTRING);
    demoversion = VERSION;

    D_Make_legacytitle();

    memset( startupwadfiles, 0, sizeof(startupwadfiles) );
   
    CON_Init_Setup();  // vid, zone independent
    EOUT_flags |= EOUT_con;  // all msgs to CON buffer
    use_font1 = 1;  // until PLAYPAL and fonts loaded
    vid.draw_ready = 0;  // disable print reaching console

    //added:18-02-98:keep error messages until the final flush(stderr)
    if (setvbuf(stderr, NULL, _IOFBF, 1000))
        GenPrintf(EMSG_warn,"setvbuf didnt work\n");
    setbuf(stdout, NULL);       // non-buffered output

    // get parameters from a response file (eg: doom3 @parms.txt)
    M_FindResponseFile();

    // some basic commandline options
    if (M_CheckParm("--version"))
    {
      printf("%s\n", legacytitle);
      printf("%s\n", DL_OPTS_STR );
      exit(0);
    }

    if (M_CheckParm("-v"))
    {
      verbose = 1;
    }
    if (M_CheckParm("-v2"))
    {
      verbose = 2;
    }
   
    if (M_CheckParm("--help") || M_CheckParm("-h"))
    {
      printf("%s\n", legacytitle);
      Help();
      exit(0);
    }

    GenPrintf( EMSG_info|EMSG_all, "%s\n", legacytitle);

    // Find or make a default dir that is not root dir
    // get the current directory (possible problem on NT with "." as current dir)
    if (getcwd(dirbuf, _MAX_PATH) != NULL)
    {
        // Need a working default dir, to prevent "" leading to root files.
        if( (strlen(dirbuf) > 4)
            || (strcmp( dirbuf, "." ) == 0) )   // systems that pass "."
        {
            defdir = strdup( dirbuf );
            if( verbose )
                GenPrintf(EMSG_ver, "Current directory: %s\n", defdir);

            if( access( defdir, X_OK ) == 0 )
                defdir_stat = 1;
        }
    }

    // [WDJ] When I_Get_Prog_Dir fails, progdir will be NULL.
    // Protect all uses of progdir and progdir_wads accordingly.
    if( I_Get_Prog_Dir( defdir, /*OUT*/ dirbuf ) )
    {
        // At worst, dirbuf may be an empty string.  OS dependent.
        progdir = strdup( dirbuf );
        if( verbose )
          GenPrintf(EMSG_ver, "Program directory: %s\n", progdir);

        // Set the directories that are relative to the program directory.
        if( access( progdir, X_OK ) == 0 )
        {
            cat_filename(dirbuf, progdir, "wads");
            progdir_wads = strdup(dirbuf);
        }

#ifdef MAC_RESOURCES_DIR
#if defined( __APPLE__ ) && defined( __MACH__ )
#if 1
        {
            // Get Resource path using core foundation.
            CFURLRef app_ref = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR("legacy"), CFSTR("wad"), CFSTR("Resources") );
            CFStringRef mac_path = CFURLCopyFileSystemPath( app_ref, kCFURLPOSIXPathStyle );
            QString res_path = CFStringGetCStringPtr( mac_path, CFStringGetSystemEncoding() );
            if( res_path )
            {
                mac_resource_dir = strdup( res_path );
            }
            // release core foundation
            CFRelease( app_ref );
            CFRelease( mac_path );
        }
#else
        cat_filename( dirbuf, progdir, "../Resources" );
        if( access( dirbuf, X_OK ) == 0 )
        {
            mac_resource_dir = strdup( dirbuf );
        }
#endif
#endif
#endif
    }

    memset( doomwaddir, 0, sizeof(doomwaddir) );
    doomwaddir[0] = getenv("DOOMWADDIR");  // ptr to environment string

    // CDROM overrides doomwaddir (when valid)
    if (M_CheckParm("-cdrom"))
    {
        GenPrintf(EMSG_hud, D_CDROM);
        // [WDJ] Execute DoomLegacy off CDROM ??
        // DoomLegacy already has separate doomwaddir and legacyhome.
        // Legacy is not compatible with other port config and savegames,
        // so do not put such in old doom "c:\\doomdata".
        // Substitute CDROM for doomwaddir, but not legacyhome.
        if( defdir )
            doomwaddir[0] = ""; // wads from cur dir
        defdir_stat = 0; // do not let legacyhome use current dir
    }

#if 0
//[WDJ] disabled in 143beta_macosx
// was test on MACOS_DI but could exclude or include __MACH__ ??
//[segabor]
#if defined( __APPLE__ ) && ! defined( __MACH__ )
    // cwd is always "/" when app is dbl-clicked
    if (!strcasecmp(doomwaddir, "/"))
    {
        // doomwaddir maybe malloc string, maybe not
        doomwaddir[0] = I_GetWadDir();
    }
#endif
#endif

    EOUT_flags = EOUT_text | EOUT_log | EOUT_con;

    CONS_Printf(text[Z_INIT_NUM]);
    // Cannot Init nor register cv_ vars until after Z_Init and some
    // other systems are init first.
    // -mb cannot be changed by Launcher, use Response File instead
    Z_Init();

    // Init once
    COM_Init(); // command buffer
    // Can now call CV_RegisterVar, and COM_AddCommand

    // may have some command line dependent init, like joystick
    I_SysInit();

    dedicated = M_CheckParm("-dedicated") != 0;

    //---------------------------------------------------- START DISPLAY
    //--- Display Error Messages
    CONS_Printf("StartupGraphics...\n");
    // setup loading screen with dedicated=0 and vid=800,600
    V_Init_VideoControl();  // before I_StartupGraphics

    if( ! dedicated )
    {
        I_StartupGraphics();    // window
        SCR_Startup();
    }

#ifdef PARANOID
    SCR_Set_dummy_draw();
#endif

    if( verbose > 1 )
        CONS_Printf("Init DEH, cht, menu\n");

    Init_info();
    // save Doom, Heretic, Chex strings for DEH
    DEH_Init();  // Init DEH before files and lumps loaded
    cht_Init();	 // init iwad independent cheats info, needed by Responder

    // [Arcade] Must precede M_Init, which applies the menu lockdown
    // according to devmode.
    devmode = M_CheckParm("-devmode");  // -devmode : unlock full menu

    M_Init();    // init menu
    R_Init_rdata();

    if( verbose > 1 )
        CONS_Printf( "Register\n" );

    // Any cv_ with CV_SAVE need to be registered here, before reading the config.
    // Some of these are dependent upon the dedicated command line switch.
    CON_Register();
    D_Register_ClientCommands(); //Hurdler: be sure that this is called before D_Setup_NetGame

    M_Register_Menu_Controls();
    HU_Register_Commands();
    ST_Register_Commands();

    T_Register_Commands();    // fragglescript
    B_Register_Commands();    //added by AC for acbot
    R_Register_EngineStuff();
    S_Register_SoundStuff();

    P_Register_Info_Commands();

#ifdef LAUNCHER
    CV_RegisterVar(&cv_home);
    CV_RegisterVar(&cv_doomwaddir);
    CV_RegisterVar(&cv_iwad);
    CV_Set( &cv_doomwaddir, doomwaddir[0] ? doomwaddir[0] : "" );
    cv_doomwaddir.state &= ~CS_MODIFIED;
#endif

    //Fab:29-04-98: do some dirty chatmacros strings initialisation
    HU_Init_Chatmacros();

    // load default control
    G_Controldefault();
   
#ifdef ZIPWAD
# ifdef OPT_LIBZIP
    WZ_available();  // check for zip lib
# endif
#endif
#ifdef HAVE_ZLIB
# if HAVE_ZLIB == 3
    // Dynamic load zlib.
    ZLIB_available();
# endif
#endif

    // Before this line are initializations that are run only one time.
    //---------------------------------------------------- 
    // After this line is code that deals with configuration,
    // game and wad selection, and finding files and directories.
    // It may retry some actions and may execute functions multiple times.

#ifdef LAUNCHER
    //---------------------------------------------------- LAUNCHER restarts here
restart_command:

    fatal_error = 0;
    all_detect = 0;  // clear previous detect settings
    deh_detect = 0;
    verbose = 0;  // verbose may be changed by launcher
    if (M_CheckParm("-v"))
    {
      verbose = 1;
    }
    if (M_CheckParm("-v2"))
    {
      verbose = 2;
    }

    dedicated = M_CheckParm("-dedicated") != 0;

    if( legacyhome )
       free( legacyhome );  // from previous
#endif  // LAUNCHER

    V_SetupFont( 1, NULL, 0 );  // Startup font size
    EOUT_flags = EOUT_text | EOUT_log | EOUT_con;

    //---------------------------------------------------- FIND FILES
    // -devgame is handled later, by IdentifyVersion
    devparm = M_CheckParm("-devparm");  // -devparm
    if (devparm)
    {
      devparm += verbose;  // levels of devparm
      CONS_Printf(D_DEVSTR);
    }

    if( verbose > 1 )
        CONS_Printf("Find HOME\n");
    // userhome section
    {
        const char * userhome = NULL;
#ifdef LAUNCHER
        byte   userhome_parm = 0;
#endif
        if (M_CheckParm("-home"))
        {
            userhome = M_GetNextParm();
            if( userhome == NULL )
            {
                I_SoftError( "Switch  -home <directory>\n" );
                userhome = "";
                fatal_error = 1;
            }
#ifdef LAUNCHER
            userhome_parm = 1;
#endif
        }
        else
        {
            userhome = getenv("HOME");
            if( userhome && verbose > 1 )
                CONS_Printf("HOME = %s\n", userhome);
#ifdef WIN32
            if( userhome && strstr( userhome, "MSYS" ) )
            {
                // Ignore MSYS HOME, it is not the one wanted.
                if( verbose > 1 )
                    CONS_Printf("Ignore MYS HOME = %s\n", userhome);
                userhome = NULL;
            }
            // Windows XP,
            if( !userhome )
            {
                 userhome = getenv("UserProfile");
                 if( userhome && verbose > 1 )
                     CONS_Printf("UserProfile = %s\n", userhome);
            }
#endif
        }

#if defined(LINUX) || (defined(__APPLE__) && defined(__MACH__))
        if( !userhome )
        {
            // Default userhome, on some systems
            userhome = "~";
        }
#endif
       
        if( !userhome )
        {
            if(verbose)
                GenPrintf(EMSG_ver, "Please set $HOME to your home directory, or use -home switch\n");

#if 0
            // [WDJ] Using current directory just lead to the user losing
            // the config and savegames.

            // Try to use current directory and defaults
            // Make an absolute default directory, not root.
            if( defdir_stat )
            {
                // have working default dir
                // userhome cannot be "", because save games can end up in root directory
                cat_filename( dirbuf, defdir, defhome );
                userhome = strdup(dirbuf);  // malloc
                if(verbose)
                    GenPrintf(EMSG_ver, " Using userhome= %s\n", userhome );
            }
            else
            {
                GenPrintf(EMSG_warn, " No home dir, and no defdir.\n" );
            }
#endif
        }

#ifdef LAUNCHER
        if( (! userhome_parm || init_sequence == 0)
          && userhome )
        {
            // Save the input userhome for the Launcher, unless it came from -home.
            CV_Set( &cv_home, userhome );
            cv_home.state &= ~CS_MODIFIED;
        }
#endif

#if defined(__APPLE__) && defined(__MACH__) && defined( MAC_HOME_DIR )
// [WDJ] Mac OS X uses  ~ as home dir, and a BSD Unix directory structure, so this is not needed.
// This is probably OS 8 or OS 9.
// Code for setting mac_user_home is missing !
        //[segabor] ... ([WDJ] MAC port has vars handy)
//        sprintf(configfile, "%s/DooMLegacy.cfg", mac_user_home);
        cat_filename( cfgbuf, mac_user_home, "DooMLegacy.cfg" );
        M_Set_configfile_main( cfgbuf );
        sprintf(savegamename, "%s/Saved games/Game %%d.doomSaveGame", mac_user_home);
        if ( ! userhome)
            userhome = mac_user_home;
        // legacyhome = strdup( mac_user_home );
        // Needs slash
        legacyhome = (char*) malloc( strlen(userhome) + 3 );
        // example: "/home/user/"
        sprintf(legacyhome, "%s/", userhome);
#else
        // [Arcade] Portable install.  A legacyhome directory sitting next to
        // the binary takes priority over the one in $HOME, so a checked-out
        // tree runs from its own tracked config.cfg with no command line
        // arguments at all.  The *presence of the directory is the switch*:
        // without it nothing below changes, so an existing ~/.doomlegacy
        // install is untouched until one is deliberately created.
        //
        // progdir comes from readlink("/proc/self/exe") on Linux
        // (I_Get_Prog_Dir, sdl/i_system.c), so this follows the executable
        // rather than the working directory -- launching from a menu entry or
        // a service unit finds the same files as launching from a shell.
        //
        // Kept in a local rather than testing legacyhome directly, because
        // D_DoomMain can re-run this block via the launcher's restart path
        // and legacyhome may already hold the previous pass's value.
        char * portable_home = NULL;
        if( progdir )   // NULL when I_Get_Prog_Dir failed
        {
            char dirpath[ MAX_WADPATH ];

            // Trailing slash matters: savegamename concatenates directly onto
            // legacyhome, and cat_filename only separates dir from name, so it
            // will not supply one at the end.
            cat_filename( dirpath, progdir, DEFHOME SLASH );
            if( access(dirpath, R_OK) == 0 )  // portable home found
            {
                portable_home = strdup( dirpath );  // malloc
                GenPrintf(EMSG_ver, "Portable legacyhome= %s\n", portable_home );
            }
        }

        // Find the legacyhome directory
        if( portable_home )
        {
            legacyhome = portable_home;
        }
        else if (userhome)
        {
            // [WDJ] find directory, .doomlegacy, or .legacy
            char dirpath[ MAX_WADPATH ];

            // form directory filename, with slash (for savegamename)
            cat_filename( dirpath, userhome, DEFAULTDIR1 SLASH );
            // if it exists then use it
            if( access(dirpath, R_OK) < 0 )  // not found
            {
                // not there, try 2nd choice
                cat_filename( dirpath, userhome, DEFAULTDIR2 SLASH );
                if( access(dirpath, R_OK) < 0 )  // not found
                {
                    // not there either, then make primary default dir
                    cat_filename( dirpath, userhome, DEFAULTDIR1 SLASH );
                }
            }
            // make subdirectory in userhome
            // example: "/home/user/.doomlegacy/"
            legacyhome = strdup( dirpath );  // malloc
        }
        else
        {
            // Check for an existing DEFHOME in current directory.
            // Only if user has made it.
            if( access(DEFHOME, R_OK) == 0 )  // legacy home found
            {
                legacyhome = strdup( DEFHOME );  // malloc, will be free.
            }
        }

        if( ! legacyhome )
        {
            // Make a default legacy home in the program directory.
            // default absolute path, do not set to ""
            cat_filename( dirbuf, progdir, DEFHOME );
            legacyhome = strdup( dirbuf );  // malloc, will be free.
            if( verbose )
                GenPrintf(EMSG_ver, "Default legacyhome= %s\n", legacyhome );
        }

        // Make the legacyhome directory
        if( access(legacyhome, R_OK) < 0 )  // not found
        {
            if( verbose )
                GenPrintf(EMSG_ver, "MKDIR legacyhome= %s\n", legacyhome );
            I_mkdir( legacyhome, 0700);
        }
        legacyhome_len = strlen(legacyhome);
       
        // [WDJ] configfile must be set whereever legacyhome is on DOS or WIN32
        {
            const char * cfgstr;
            // user specific config file
#ifdef DEVPARM_LOADING
            if( devparm )
            {
                // example: /home/user/.legacy/devdataconfig.cfg
                cfgstr = DEVDATA CONFIGFILENAME;
            }
            else
#endif	     
            {
                // example: /home/user/.legacy/config.cfg
                cfgstr = CONFIGFILENAME;
            }
            cat_filename( cfgbuf, legacyhome, cfgstr );
            M_Set_configfile_main( cfgbuf );
        }

#ifdef SAVEGAMEDIR
        // default savegame file name, example: "/home/user/.legacy/%s/doomsav%i.dsg"
//        sprintf(savegamename, "%s%%s" SLASH "%s", legacyhome, text[NORM_SAVEI_NUM]);
        snprintf(savegamename, MAX_WADPATH-1, "%s%%s" SLASH "%s%s", legacyhome, SAVEGAMENAME, "%d.dsg");
        // so can extract legacyhome from savegamename later
#else    
        // default savegame file name, example: "/home/user/.legacy/doomsav%i.dsg"
//        sprintf(savegamename, "%s%s", legacyhome, text[NORM_SAVEI_NUM]);
        snprintf(savegamename, MAX_WADPATH-1, "%s%s%s", legacyhome, SAVEGAMENAME, "%d.dsg");
#endif
        savegamename[MAX_WADPATH-1] = '\0';
#endif

        // [WDJ] Would have a doomwaddir in the config file too, but LoadConfig
        // is done way too late for that.
        for( wdi=0; wdi<(sizeof(dirlist)/sizeof(char*)); wdi++ )
        {
            char ** dwp = & doomwaddir[wdi+DOOMWADDIR_DIRLIST]; // where it goes in doomwaddir
            if( *dwp && (*dwp != dirlist[wdi]) )
                free( *dwp );  // was malloc
            // Default searches
            if( dirlist[wdi] && dirlist[wdi][0] == '~' )
            {
                // Relative to user home.
                cat_filename( dirbuf, (userhome? userhome : "" ), &dirlist[wdi][2]);
                *dwp = strdup( dirbuf );  // (malloc)
            }
            else
            {
                *dwp = dirlist[wdi];  // (ref)
            }
        }
    }

    HS_Init();   // [Arcade] load persisted high scores, ensure demos/ dir exists

    // [Arcade] Must be after legacyhome/configfile_main are resolved and
    // before IdentifyVersion below, which is what acts on it.
    D_Read_Default_Game();

    // [Arcade] -clearhighscores : wipe the times and record demos at startup,
    // for a cabinet reset without needing the console.
    if( M_CheckParm("-clearhighscores") )
        Command_ClearHighScores_f();

#if 0
    Print_search_directories( EMSG_debug, 0x0F );
#endif

    if( verbose )
    {
        GenPrintf(EMSG_ver, "Config: %s\n", configfile_main );
        GenPrintf(EMSG_ver, "Savegames: %s\n", savegamename );
    }

    // identify the main IWAD file to use
    IdentifyVersion();  // game, iwad
    modifiedgame = false;

    // Title page
    const char *gametitle = gamedesc.startup_title;  // set by IdentifyVersion
    if( gametitle == NULL )   gametitle = gamedesc.gname;
    if( gametitle )
      GenPrintf(EMSG_info, "%s\n", gametitle);

#ifdef DEVPARM_LOADING
    // convenience hack to allow -wart e m to add a wad file
    p = M_CheckParm("-wart");
    if (p)
    {
        // big hack, change to -warp so a later CheckParm does the warp.
        myargv[p][4] = 'p';

        // Map name handling.  Form wad name from map/episode numbers.
#ifdef WADFILE_RELOAD
        // prepend a tilde to the filename so wadfile will be reloadable
#endif
        switch (gamemode)
        {
            case doom_shareware:
            case ultdoom_retail:
            case doom_registered:
             if( (p+2) < myargc )
             {
#ifdef WADFILE_RELOAD
                sprintf(fbuf, "~" DEVMAPS "E%cM%c.wad", myargv[p + 1][0], myargv[p + 2][0]);
#else
                sprintf(fbuf,  DEVMAPS "E%cM%c.wad", myargv[p + 1][0], myargv[p + 2][0]);
#endif
                GenPrintf(EMSG_info, "Warping to Episode %s, Map %s.\n", myargv[p + 1], myargv[p + 2]);
             }
             break;

            case doom2_commercial:
            default:
             if( (p+1) < myargc )
             {
                p = atoi(myargv[p + 1]);
                if (p < 10)
#ifdef WADFILE_RELOAD
                    sprintf(fbuf, "~" DEVMAPS "cdata/map0%i.wad", p);
#else
                    sprintf(fbuf, DEVMAPS "cdata/map0%i.wad", p);
#endif
                else
#ifdef WADFILE_RELOAD
                    sprintf(fbuf, "~" DEVMAPS "cdata/map%i.wad", p);
#else
                    sprintf(fbuf, DEVMAPS "cdata/map%i.wad", p);
#endif
             }
             break;
        }
        D_AddFile(fbuf);
        // continue and execute -warp
    }
#endif

    // Add any files specified on the command line with -file <wadfile>
    // to the wad list
    if (M_CheckParm("-file"))
    {
        // the parms after p are wadfile/lump names,
        // until end of parms or another - preceded parm
        modifiedgame = true;    // homebrew levels
        while (M_IsNextParm())
            D_AddFile( M_GetNextParm() );
    }

    // load dehacked file
    p = M_CheckParm("-dehacked");
    if (!p)
        p = M_CheckParm("-deh");        //Fab:02-08-98:like Boom & DosDoom
    if (p != 0)
    {
        while (M_IsNextParm())
            D_AddFile( M_GetNextParm() );
    }

#ifdef FRENCH_INLINE
    french_text();
#endif

#ifdef BEX_LANGUAGE
    if ( M_CheckParm("-lang") )
    {
        // will check for NULL parameter
        BEX_load_language( M_GetNextParm(), 2 );  // language name
    }
#ifdef BEX_LANG_AUTO_LOAD
    else
    {
        BEX_load_language( NULL, 2 );  // default language name
    }
#endif
#endif

    // DEH sensitive settings

#ifdef MBF21
    // DSDA-isms.
    if( M_CheckParm("-dsda") )
        all_detect |= BDTC_dsda;
#endif

    // Initialize tables for DEH modification.
    init_tables_deh();

    // Adapt tables to legacy needs
    // This must be reversible, because it is within launcher.
    P_PatchInfoTables();

    // Anything that inits tables must be done before this line,
    // before dehacked is applied.

    init_tables_commit();  // not yet reversible
    
    // Load wad, including the main wad file.
    // This will read DEH and BEX files.  Need devparm and verbose.
    if( W_Init_MultipleFiles(startupwadfiles) == 0 )
    {
       // Some wad failed to load.
       if( !M_CheckParm( "-noloadfail" ) )
         fatal_error = true;
    }
    
    if ( !M_CheckParm("-nocheckwadversion") )
        D_CheckWadVersion();


    //Hurdler: someone wants to keep those lines?
    //BP: i agree with you why should be registered to play someone wads ?
    //    unfortunately most additional wad have more texture and monsters
    //    that shareware wad do, so there will miss resource :(

    if ( gamedesc.gameflags & GD_idwad )
    {
      // [WDJ] These warnings only apply to id iwad files, and should not
      // appear when only using FreeDoom or third party iwads.

      // Check for -file in shareware
      if (modifiedgame)
      {
        // These are the lumps that will be checked in IWAD,
        // if any one is not present, execution will be aborted.
        char name[23][8] = {
            "e2m1", "e2m2", "e2m3", "e2m4", "e2m5", "e2m6", "e2m7", "e2m8", "e2m9",
            "e3m1", "e3m3", "e3m3", "e3m4", "e3m5", "e3m6", "e3m7", "e3m8", "e3m9",
            "dphoof", "bfgga0", "heada1", "cybra1", "spida1d1"
        };

        if (gamemode == doom_shareware)
            CONS_Printf("\nYou shouldn't use -file with the shareware version. Register!\n");

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version.
        if (gamemode == doom_registered)
        {
            int i;
            for (i = 0; i < 23; i++)
            {
                if( ! VALID_LUMP( W_CheckNumForName(name[i]) ) )
                    CONS_Printf("\nThis is not the registered version.");
            }
        }
      }
   
      // If additonal PWAD files are used, print modified banner
      if (modifiedgame)
          CONS_Printf(text[MODIFIED_NUM]);

      // Check and print which version is executed.
      switch (gamemode)
      {
        case doom_shareware:
        case indetermined:
            CONS_Printf(text[SHAREWARE_NUM]);
            break;
        case doom_registered:
        case ultdoom_retail:
        case doom2_commercial:
            CONS_Printf(text[COMERCIAL_NUM]);
            break;
        default:
            // Ouch.
            break;
      }
    }
   
    EOUT_flags = EOUT_text | EOUT_log | EOUT_con;


#ifdef LAUNCHER
fatal_error_action:
    //---------------------------------------------------- LAUNCHER display
    if ( fatal_error || init_sequence == 1 )
    {
        // [WDJ] Invoke built-in launcher command line
        if ( fatal_error )
        {
            CONS_Printf("Fatal error display: (press ESC to continue).\n");
            con_destlines = BASEVIDHEIGHT;
            do
            {
                CON_Draw_Console();
                I_OsPolling();
                D_Process_Events ();  // menu and console responder
                CON_Ticker ();
                I_UpdateNoBlit();
                I_FinishUpdate();       // page flip or blit buffer
            } while( con_destlines>0 );
        }
        init_sequence = 1;
        M_LaunchMenu();  // changes init_sequence > 1 to exit restart loop

        // restart
        Clear_SoftError();
        D_Clear_Files();
        con_Printf( "Launcher restart:\n" );
        goto restart_command;
    }
    // END OF LAUNCHER
#endif

    //--------------------------------------------------------- LAUNCHED
    // After this line, commit to the initial game and video port selected.
    // Use I_Error.

    if( ! VALID_LUMP( W_CheckNumForName ( "PLAYPAL" ) ) )
    {
        //Hurdler: I'm tired of that question ;)
        I_Error (
        "The main IWAD file is not found, or does not have PLAYPAL lump.\n"
        "The IWAD can be either doom.wad, doom1.wad, doom2.wad, tnt.wad\n"
        "plutonia.wad, heretic.wad, or heretic1.wad from any shareware\n"
        "or commercial version of Doom or Heretic, or some other IWAD!\n"
        "Cannot use legacy.wad, nor a PWAD, for an IWAD.\n" );
    }

    if( fatal_error )
    {
        I_Error ( "Shutdown due to fatal error.\n" );
    }

    //---------------------------------------------------- LOAD CONFIG
   
    // The drawmode and video settings are now part of the config.
    // It needs to be loaded before the full graphics.
   
    // check for an alternative config file
    p = M_CheckParm ("-config");
    if (p && (p+1)<myargc)
    {
        // substitute config file
        dl_strncpy(cfgbuf, myargv[p+1], MAX_WADPATH);
        M_Set_configfile_main( cfgbuf );
        CONS_Printf ("config file: %s\n", configfile_main);
    }
    // This config will load the config drawmode setting.
    M_ClearConfig( CFG_main );  // due to launcher loop
    M_LoadConfig( CFG_main, configfile_main );        // WARNING : this do a "COM_BufExecute()"

    // [Arcade] Force the ranked ruleset: vanilla difficulty settings with
    // Boom/MBF engine behavior left at its defaults.  DoomLegacy defaults
    // several extras ON ("tiredrun" easy, "drown" Legacy), and they are
    // single global CV_NETVARs, so the two-player Options screen changes
    // single player too.  Applied after the config load so a stale config
    // cannot bring them back; -devmode leaves them alone for experimenting.
    // See hs_stuff.c for the table and for what an altered ruleset costs.
    if( ! devmode )
        HS_Apply_Ranked_Ruleset();


    //---------------------------------------------------- READY SCREEN
#ifdef HWRENDER
    // Init the rendermode patch storage.
    HWR_patchstore = 0;
    EN_HWR_flashpalette = 0;  // software and default
#endif

    // we need to check for dedicated before initialization of some subsystems
    dedicated = M_CheckParm("-dedicated") != 0;
    if( dedicated )
    {
        nodrawers = true;
        vid.draw_ready = 0;
        drawmode_recalc = false;
        I_ShutdownGraphics();
        EOUT_flags = EOUT_log;
    }
    else
    {
        //--------------------------------------------------------- GRAPHICS SETTINGS
        set_drawmode = cv_drawmode.EV;  // from config file
        req_bitpp = 0;  // because of launcher looping
        req_alt_bitpp = 0;

        if( M_CheckParm("-highcolor") )
        {
            set_drawmode = DRM_explicit_bpp;  // 15 or 16 bpp
            req_bitpp = 16;
            req_alt_bitpp = 15;
        }
        if( M_CheckParm("-truecolor") )
        {
            set_drawmode = DRM_explicit_bpp;  // 24 or 32 bpp
            req_bitpp = 32;
            req_alt_bitpp = 24;
        }
        if( M_CheckParm("-native") )
        {
            set_drawmode = DRM_native;  // bpp of the default screen
        }
        p = M_CheckParm("-bpp");  // specific bit per pixel color
        if( p )
        {
            // binding, should fail if cannot find a mode
            req_bitpp = atoi(myargv[p + 1]);
            if( ! V_CanDraw( req_bitpp ) )
            {
#ifdef LAUNCHER
              I_SoftError( "-bpp invalid\n");
              fatal_error = 1;
              goto fatal_error_action;
#else
              I_Error( "-bpp invalid\n");
#endif
            }
            set_drawmode = DRM_explicit_bpp;
        }
        req_command_video_settings = req_bitpp & 0x3F;

        // Allow a config file for opengl to overload the config settings.
        // It may be edited to set only what settings should be specific to opengl.
        // May be a problem if opengl cannot really be started.

        if( M_CheckParm("-opengl") )
        {
            set_drawmode = DRM_opengl; // opengl temporary
        }
#ifdef HWRENDER
        else if( M_CheckParm ("-3dfx") || M_CheckParm ("-glide") )
        {
#ifdef USE_VOODOO_GLIDE
            set_drawmode = DRM_glide; // glide temporary
#else
            I_SoftError( "Voodoo Glide support not present.\n");
#endif
        }
#ifdef SMIF_WIN_NATIVE
        else if( M_CheckParm ("-minigl") ) // MiniGL is considered to be opengl
        {
            set_drawmode = DRM_minigl; // opengl temporary
        }
        else if( M_CheckParm ("-d3d") )
        {
            set_drawmode = DRM_d3d; // D3D temporary
        }
#endif
#endif

        M_ClearConfig( CFG_drawmode );  // due to launcher loop
        // Load the config file for this drawmode.       
        // example: /home/user/.legacy/config32.cfg
        M_Set_configfile_drawmode( set_drawmode );
        // Conditional on name defined and file existing.
        // This cannot change the drawmode, but can load screen sizes.
        M_LoadConfig( CFG_drawmode, configfile_drawmode );        // WARNING : this do a "COM_BufExecute()"


        // 0 means not set at the cmd-line
        req_width = 0;
        req_height = 0;

        p = M_CheckParm("-width");
        if (p && (p+1) < myargc)
        {
            req_width = atoi(myargv[p+1]);
            req_command_video_settings |= 0x80;
        }

        p = M_CheckParm("-height");
        if (p && (p+1) < myargc)
        {
            req_height = atoi(myargv[p+1]);
            req_command_video_settings |= 0x40;
        }


        //--------------------------------------------------------- FULL GRAPHICS
        // setup loading screen
        // Still using font1 during this init, up into CON_Init_Video.
        CONS_Printf("RequestFullGraphics...\n");

        // Allow VID_QueryModelist to detect fullscreen capability.
        allow_fullscreen = ! M_CheckParm("-window");
        cv_fullscreen.EV = cv_fullscreen.value && allow_fullscreen;
        cv_fullscreen.state |= CS_EV_PROT;  // protect against restore

        // Initial setup of rendermode
        // Does VID_QueryModelist, which does not change the current modelist in all ports.
        byte validv = V_switch_drawmode( set_drawmode, 0 );  // command line, do not change config files
        if( ! validv )
        {
            GenPrintf( EMSG_error, "FullGraphics: setup drawmode failed, no valid modes.\n" );
            // The set_drawmode, from config or command line, was not possible.
            // The drawmode may not be included in the compile options, or no modes.
            // Recover to native window.
            cv_fullscreen.EV = 0;   // window mode
            req_bitpp = native_bitpp;  // native drawing
            req_command_video_settings |= 0x01;
            set_drawmode = DRM_native;
            validv = V_switch_drawmode( set_drawmode, 0 );  // default, do not change config files
            if( ! validv )
            {
                I_Error( "FullGraphics: setup drawmode failed, cannot use native window.\n" );
            }
        }

        // set user default mode or mode set at cmdline
        // Disable setmodeneeded, do not have correct modelist yet.
        SCR_apply_video_settings( 0 );  // command line settings, or config file settings.
        setmodeneeded.modetype = MODE_NOP;  // initialization is not exactly a mode change.

        // Full graphics.
        // param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
        // If fails for one fullscreen mode, does not mean fails for all fullscreen modes.
        // Calls  I_Rendermode_setup, V_Setup_VideoDraw, HWR_Startup_Render.
        // Calls  V_SetPalette, HWR_SetPalette.
        drawmode_recalc = true;  // bypass setmodeneeded
        rendermode_recalc = true;
        // Gets the new Modelist.
        SCR_SetMode( 0 );

        SCR_Recalc();
        V_Clear_Display();

        drawmode_recalc = false;
        rendermode_recalc = false;

        //--------------------------------------------------------- CONSOLE
        // Need console fonts, colormaps, before can stop using font1.
        CONS_Printf(text[HU_INIT_NUM]);
#ifdef PARANOID
        // Console fonts were loaded by SCR_SetMode.
        if( hu_fonts_loaded < 2 )
        {
            // Do not reload without unloading fonts first.	    
            if( hu_fonts_loaded == 0 )
            {
                GenPrintf( EMSG_warn, "Console fonts not loaded\n" );
                // Load console fonts, setting hu_fonts_loaded.
                HU_Load_Graphics();  // dependent upon dedicated and game
            }
            else
            {
                GenPrintf( EMSG_warn, "Console fonts not complete\n" );
            }
        }
#endif       
        // Load console colormaps, and size console.
        CON_Init_Video();  // dependent upon vid, hu_font

        if( (hu_fonts_loaded == 2) && whitemap )
            use_font1 = 0;  // Now use console fonts, no longer using font1.

        EOUT_flags = EOUT_log | EOUT_con;
    }

#if 0   
    p = M_CheckParm ("-cfgmod");
    if (p && (p+1)<myargc)
    {
        // Add mod config file
        dl_strncpy(cfgbuf, myargv[p+1], MAX_WADPATH);
        // not saveable, so do not need to save name
        CONS_Printf ("add config file: %s\n", cfgbuf);
        // This cannot change the drawmode.
        M_LoadConfig( CFG_other, cfgbuf );        // WARNING : this do a "COM_BufExecute()"
    }
#endif

    //--------------------------------------------------------- GAME SETTINGS
    // get skill / episode / map from parms
    gameskill = sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;

    p = M_CheckParm("-skill");
    if (p && (p+1) < myargc)
    {
        gameskill = myargv[p + 1][0] - '1';
        autostart = true;
    }

    p = M_CheckParm("-episode");
    if (p && (p+1) < myargc)
    {
        startepisode = myargv[p + 1][0] - '0';
        startmap = 1;
        autostart = true;
    }

    p = M_CheckParm("-warp");
    if (p && (p+1) < myargc)
    {
        if (gamemode == doom2_commercial)
            startmap = atoi(myargv[p + 1]);
        else
        {
            startepisode = myargv[p + 1][0] - '0';
            if (((p+2) < myargc) && myargv[p+2][0] >= '0' && myargv[p+2][0] <= '9')
                startmap = myargv[p+2][0] - '0';
            else
                startmap = 1;
        }
        autostart = true;
    }

    // [WDJ] This triggers the first draw to the screen,
    // debug it here instead of waiting for CONS_Printf in BloodTime_OnChange
    CONS_Printf( "Init after ...\n" );

    CONS_Printf(text[W_INIT_NUM]);

    B_Init_Bots();       //added by AC for acbot

    wipegamestate = gamestate;

    //------------------------------------------------ COMMAND LINE PARAMS

#ifdef CDMUS
    // Initialize CD-Audio, no music on a dedicated server
    if (!M_CheckParm("-nocd") && ! dedicated )
      I_InitCD();
#endif

    if (M_CheckParm("-splitscreen"))
        CV_SetParam(&cv_splitscreen, 1);
   
    // Affects only the game started at the command line.
    // CV_NETVAR are sent by string, CV_SAVE are saved by string.
    nomonsters = M_CheckParm("-nomonsters");

    if (M_CheckParm("-respawn"))
      CV_SetParam( &cv_respawnmonsters, 1 );  // NETVAR
//      COM_BufAddText("respawnmonsters 1\n");
    if (M_CheckParm("-coopmonsters"))
      CV_SetParam( &cv_monbehavior, 1 );  // NETVAR, SAVE
//      COM_BufAddText("monsterbehavior 1\n");
    if (M_CheckParm("-infight"))
      CV_SetParam( &cv_monbehavior, 2 );  // NETVAR, SAVE
//      COM_BufAddText("monsterbehavior 2\n");
    if (M_CheckParm("-teamplay"))
      CV_SetParam( &cv_teamplay, 1 );  // NETVAR
//        COM_BufAddText("teamplay 1\n");
    if (M_CheckParm("-teamskin"))
      CV_SetParam( &cv_teamplay, 2 );  // NETVAR
//        COM_BufAddText("teamplay 2\n");
    // Setting deathmatch also sets cv_itemrespawn (d_netcmd).
    if (M_CheckParm("-altdeath"))
      CV_SetParam( &cv_deathmatch, 2 );  // NETVAR
//      COM_BufAddText("deathmatch 2\n");
    else if (M_CheckParm("-deathmatch"))
      CV_SetParam( &cv_deathmatch, 1 );  // NETVAR
//        COM_BufAddText("deathmatch 1\n");
    if (M_CheckParm("-fast"))
      CV_SetParam( &cv_fastmonsters, 1 );  // NETVAR
//        COM_BufAddText("fastmonsters 1\n");
    //added by AC
    if (M_CheckParm("-predicting"))
      CV_SetParam( &cv_predictingmonsters, 1 );  // NETVAR
//        COM_BufAddText("predictingmonsters 1\n");

    if (M_CheckParm("-timer"))
    {
        char *s = M_GetNextParm();
        if( s == NULL )
        {
            I_SoftError( "Switch  -timer <seconds>\n" );
        }
        else
        {
            // May be larger than EV, so cannot use CV_SetParam.
            CV_Set( &cv_timelimit, s );  // NETVAR
//            COM_BufAddText(va("timelimit %s\n", s));
        }
    }

    if (M_CheckParm("-avg"))
    {
        // May be larger than EV, so cannot use CV_SetParam.
        CV_SetValue( &cv_timelimit, 20 );  // NETVAR
//        COM_BufAddText("timelimit 20\n");
        CONS_Printf(text[AUSTIN_NUM]);
    }

    // turbo option, is not meant to be saved in config, still
    // supported at cmd-line for compatibility
    if (M_CheckParm("-turbo"))
    {
        if( M_IsNextParm() )
        {
            COM_BufAddText(va("turbo %s\n", M_GetNextParm()));
        }
        else
        {
            I_SoftError( "Switch  -turbo <10-255>\n" );
        }
    }

    // push all "+" parameter at the command buffer
    M_PushSpecialParameters();

    CONS_Printf(text[M_INIT_NUM]);
    M_Configure();

    CONS_Printf(text[R_INIT_NUM]);
    R_Init();

    //
    // setting up sound
    //
    CONS_Printf(text[S_SETSOUND_NUM]);
    nosoundfx = M_CheckParm("-nosound");
    nomusic = M_CheckParm("-nomusic");
    if( dedicated )
    {
        nosoundfx = 1;
        nomusic = 1;
        // allow sound driver to disable self
    }
    // Music init is in I_StartupSound
    I_StartupSound();
    S_Init(cv_soundvolume.value, cv_musicvolume.value);

    CONS_Printf(text[ST_INIT_NUM]);
    ST_Init();

    // SoM: Init FraggleScript
    T_Init_FS();

    // init all NETWORK
    CONS_Printf(text[D_CHECKNET_NUM]);
    if (D_Startup_NetGame())
        autostart = true;

    // check for a driver that wants intermission stats
    p = M_CheckParm("-statcopy");
    if (p && (p+1) < myargc)
    {
        I_SoftError("Sorry but statcopy isn't supported at this time\n");
        /*
           // for statistics driver
           extern  void*   statcopy;

           statcopy = (void*)atoi(myargv[p+1]);
           CONS_Printf (text[STATREG_NUM]);
         */
    }

    // start the apropriate game based on parms
    p = M_CheckParm("-record");
    if (p && (p+1) < myargc)
    {
        G_RecordDemo(myargv[p + 1]);
        autostart = true;
    }

    // demo doesn't need anymore to be added with D_AddFile()
    p = M_CheckParm("-playdemo");
    if( !p && M_CheckParm("-timedemo") )
      p = 2500;  // indicate timedemo
    if (p)
    {
      if( ! M_IsNextParm() )
      {
        I_SoftError( "Switch  -playdemo <name>  or  -timedemo <name> \n" );
      }
      else
      {
        char demo_name[MAX_WADPATH];  // filename
        // add .lmp to identify the EXTERNAL demo file
        // it is NOT possible to play an internal demo using -playdemo,
        // rather push a playdemo command.. to do.

        dl_strncpy(demo_name, M_GetNextParm(), MAX_WADPATH);
        // get spaced filename or directory
        while (M_IsNextParm())
        {
            // [WDJ] Protect against long demo name on command line
            int dn_free = MAX_WADPATH - 2 - strlen(demo_name);
            if( dn_free > 1 )
            {
                strcat(demo_name, " ");
                strncat(demo_name, M_GetNextParm(), dn_free );
                demo_name[MAX_WADPATH-1] = '\0';
            }
        }
        FIL_DefaultExtension(demo_name, ".lmp");

        CONS_Printf("Playing demo %s.\n", demo_name);

        if( p == 2500 )
        {  // timedemo
            G_TimeDemo(demo_name);
        }
        else
        {  // playdemo
            singledemo = true;  // quit after one demo
            G_DeferedPlayDemo(demo_name);
        }

        gamestate = wipegamestate = GS_NULL;

        return;
      }
    }

    p = M_CheckParm("-loadgame");
    if (p && (p+1) < myargc)
    {
        G_Load_Game(atoi(myargv[p + 1]));
    }
    else
    {
        if (dedicated && server)
        {
            pagename = "TITLEPIC";
            gamestate = GS_WAITINGPLAYERS;
        }
        else if (autostart || netgame
                 || M_CheckParm("+connect") || M_CheckParm("-connect"))
        {
            //added:27-02-98: reset the current version number
            G_setup_VERSION();
            gameaction = ga_nothing;
            if (server && !M_CheckParm("+map"))
                COM_BufAddText(va("map \"%s\"\n", G_BuildMapName(startepisode, startmap)));
        }
        else
        {
            // Cancel commandline, restore cv_ var.
            D_StartTitle();     // start up intro loop
        }

    }

    drawmode_recalc = false;
    Clear_SoftError();
    // This leaves commands for the first COM_BufExecute in D_DoomLoop to execute.
}


// Print error and continue game [WDJ] 1/19/2009
#define SOFTERROR_LISTSIZE   8
static const char *  SE_msg[SOFTERROR_LISTSIZE];
static uint32_t      SE_val[SOFTERROR_LISTSIZE]; // we only want to compare
static int  SE_msgcnt = 0;
static int  SE_next_msg_slot = 0;

byte  EOUT_flags = EOUT_text | EOUT_log;  // EOUT_e

static void Clear_SoftError(void)
{
   SE_msgcnt = 0;
}


// Print out error and continue program.  Maintains list of errors and
// does not repeat error messages in recent history.
void I_SoftError (const char *errmsg, ...)
{
    va_list     argptr;
    int         index;
    uint32_t    errval;

    // Message first.
    va_start (argptr,errmsg);
    errval = va_arg( argptr, uint32_t ) ; // sample it as an int, no matter what
    va_end (argptr);
//  debug_Printf("errval=%d\n", errval );   // debug
    for( index = 0; index < SE_msgcnt; index ++ ){
       if( errmsg == SE_msg[index] ){
          if( errval == SE_val[index] ) goto done;	// it is a repeat msg
       }
    }
    // save comparison info
    SE_msg[SE_next_msg_slot] = errmsg;
    SE_val[SE_next_msg_slot] = errval;
    SE_next_msg_slot++;
    if( SE_next_msg_slot > SE_msgcnt )
       SE_msgcnt = SE_next_msg_slot;  // max
    if( SE_next_msg_slot >= SOFTERROR_LISTSIZE )
       SE_next_msg_slot = 0;  // wrap
    // Error, always prints EMSG_text
    fprintf (stderr, "Warn: ");
    va_start (argptr,errmsg);
    GenPrintf_va( EMSG_error, errmsg, argptr );  // handles EOUT_con
    va_end (argptr);

done:   
    fflush( stderr );
}


// The system independent quit and save config.
void D_Quit_Save ( quit_severity_e severity )
{
    // Prevent recursive I_Quit(), mainly due to situation problems.
    static byte quitseq = 0;
    uint16_t *  endtext = NULL;

    // If this gets called twice, it cannot just return.
    // Some shutdown routine called I_Quit again due to an error and we
    // cannot get back to the previous invocation to finish the shutdown.
    if( quitseq == 0 )
    {
        quitseq = 1;
        //added:16-02-98: when recording a demo, should exit using 'q' key,
        //   but sometimes we forget and use 'F10'.. so save here too.
        if (demorecording)
           G_CheckDemoStatus();
    }
    if( quitseq < 2 )
    {
        quitseq = 2;
        D_Quit_NetGame ();
    }
    if( quitseq < 5 )
    {
        quitseq = 5;
        I_ShutdownSound();
    }
#ifdef CDMUS
    if( quitseq < 6 )
    {
        quitseq = 6;
        I_ShutdownCD();
    }
#endif
    if( quitseq < 8 )
    {
        quitseq = 8;
        if( severity == QUIT_normal )
        {
            M_SaveAllConfig();
        }
    }
    if( quitseq < 10 )
    {
        quitseq = 10;
        if( (severity == QUIT_normal)
             && ! M_CheckParm("-noendtext")  // normal spelling, docs
             && ! M_CheckParm("-noendtxt")   // previous versions
             && cv_textout.EV > 0 )
        {
            // [WDJ] Check on errors during I_Error shutdown.
            // Avoid repeat errors during bad environment shutdown.
            lumpnum_t endtxt_num = W_CheckNumForName("ENDOOM");
            if( VALID_LUMP(endtxt_num) )
                endtext = W_CacheLumpNum( endtxt_num, PU_STATIC );
            // If there are any more errors, then do not show the end text.
        }
    }
    if( quitseq < 11 )
    {
        quitseq = 11;
        // Close open wad files.
        // Neccesity for a Mac.  Open files hang devices.
        W_Shutdown();
    }
    if( quitseq < 15 )
    {
        quitseq = 15;
        I_Shutdown_IO();
    }
    if( quitseq < 20 )
    {
        quitseq = 20;
        if( severity != QUIT_normal )
            I_Sleep( 3000 );  // to see some messages
        vid.draw_ready = 0;        
        I_ShutdownGraphics();
        HU_Release_Graphics();
#ifdef HWRENDER
        if( HWR_patchstore )
        {
            HWR_Shutdown_Render();
        }
#endif
    }
    if( quitseq < 22 )
    {
        quitseq = 22;
        I_ShutdownSystem();
    }
    if( quitseq < 29 )
    {
        quitseq = 29;
        if( (severity == QUIT_normal)
          && endtext )
        {
            // Show the ENDOOM text
            printf("\r");
            I_Show_EndText( endtext );
        }
    }
}

// I_Quit exits with (exit 0).
// No return
void I_Quit (void)
{
    D_Quit_Save( QUIT_normal );
#ifdef ENABLE_UMAPINFO
# if 0
    // [WDJ] Optional.  Do not need to tear down structures on program exit.
    // [MB] 2023-03-19: Support for UMAPINFO added
    UMI_DestroyUMapInfo();
# endif
#endif
    I_Quit_System();  // No Return
}


static void Help( void )
{
  char * np = M_GetNextParm();
   
  if( np == NULL )
  {
    printf
       ("Usage: doomlegacy [-opengl] [-iwad xxx.wad] [-file pwad.wad ...]\n"
        "--version   Print Doom Legacy version\n"
        "-h     Help\n"
        "-h g   Help game and wads\n"
        "-h m   Help multiplayer\n"
        "-h c   Help config\n"
        "-h s   Help server\n"
        "-h d   Help demo\n"
        "-h D   Help Devmode\n"
        );
     return;
  }
  switch( np[0] )
  {
   case 'g': // game
     printf
       (
        "-game name      doomu, doom2, tnt, plutonia, freedoom, heretic, chex1, etc.\n"
        "-iwad file      The game wad\n"
        "-file file      Load DEH and PWAD files (one or more)\n"
        "-deh  file      Load DEH files (one or more)\n"
        "-dehthing name  DEH translation: legacy, boom, prboom, ee, mbf, heretic.\n"
        "-remap 4        Level of remap, 0 to 4. 0=off"
        "-sprite_remap 4 Level of sprite remap, 0 to 4."
        "-state_remap 4  Level of state remap, 0 to 4."
        "-sfx_remap 4    Level of sfx remap, 0 to 4."
        "-state_zero 1   Level of state zero, 0 to 2, 0=off."
        "-loadgame num   Load savegame num\n"  
        "-episode 2      Goto episode 2, level 1\n"
        "-skill 3        Skill 1 to 5\n"
        "-warp 13        Goto map13\n"
        "-warp 1 3       Goto episode 1 level 3\n"
        "-nomonsters     No monsters\n"
        "-respawn        Monsters respawn after killed\n"
        "-coopmonsters   Monsters cooperate\n"
        "-infight        Monsters fight each other\n"
        "-fast           Monsters are fast\n"
        "-predicting     Monsters aim better\n"
        "-turbo num      Player speed %%, 10 to 255\n"   
        );
     break;
   case 'm': // multiplayer
     printf
       (
        "-teamplay       Play with teams by color\n"
        "-teamskin       Play with teams using skins\n"
        "-splitscreen    Two players on this screen\n"
        "-deathmatch     Deathmatch, weapons respawn\n"
        "-altdeath       Deathmatch, items respawn\n"
        "-timer num      Timelimit in minutes\n"
        "-avg            Austin 20 min rounds\n"
        );
     break;
   case 'c': // config
     printf
       (
        "-v   -v2        Verbose\n"
        "-home name      Config and savegame directory\n"
        "-config file    Config file\n"
        "-opengl         OpenGL hardware renderer\n"
        "-nosound        No sound effects\n"
#ifdef CDMUS
        "-nocd           No CD music\n"
#endif
        "-nomusic        No music\n"
        "-precachesound  Preload sound effects\n"
        "-mb num         Pre-allocate num MiB of memory\n"
        "-window         No fullscreen\n"
        "-width num      Video mode width\n"
        "-height num     Video mode height\n"
        "-highcolor      Request 15bpp or 16bpp\n"
        "-truecolor      Request 24bpp or 32bpp\n"
        "-native         Video mode in native bpp\n"
        "-bpp num        Video mode in (8,15,16,24,32) bpp\n"
        "-nocheckwadversion   Ignore legacy.wad version\n"
#ifdef BEX_LANGUAGE
        "-lang name      Load BEX language file name.bex\n"
#endif
        );
     break;
   case 's': // server
     printf
       (
        "-server         Start as game server\n"
        "-dedicated      Dedicated server, no player\n"
        "-connect name   Connect to server name\n"
        "-bandwidth bps  Net bandwidth in bytes/sec\n"
        "-packetsize num Net packetsize\n"
        "-nodownload     No download from server\n"
        "-nofiles        Download all from server\n"
        "-clientport x   Use port x for client\n"
        "-udpport x      Use udp port x for server (and client)\n"
#ifdef USE_IPX
        "-ipx            Use IPX\n"
#endif
        "-extratic x     Send redundant player movement\n"
        "-debugfile file Log to debug file\n"
        "-left           Left slaved view\n"
        "-right          Right slaved view\n"
        "-screendeg x    Slaved view at x degrees\n"
        );
     break;
   case 'd': // demo
     printf
       (
        "-record file    Record demo to file\n"
        "-maxdemo num    Limit record demo size, in KiB\n"
        "-playdemo file  Play demo from file\n"
        );
     break;
   case 'D': // devmode
     printf
       (
        "-devparm        Develop mode\n"
        "-devmode        Unlock full menu (e.g. Multiplayer)\n"
        "-clearhighscores  Erase recorded times and record demos\n"
        "-synclog        Log per-tic state while recording/playing a demo\n"
#ifdef DEVPARM_LOADING
        "-devgame gamename  Develop mode, and specify game\n"
        "-wart 3 1       Load file devmaps/E3M1.wad, then warp to it\n"
        "-wart 13        Load file devmaps/cdata/map13.wad, then warp to it\n"
#endif
        "-timedemo file  Timedemo from file\n"
        "-nodraw         Timedemo without draw\n"
        "-noblit         Timedemo without blit\n"
        );
     break;
  }
}
