// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_system.c 1649 2023-10-25 02:11:02Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: I_system.c,v $
// Revision 1.9  2001/04/27 13:32:14  bpereira
//
// Revision 1.8  2001/04/16 22:58:55  ydario
// now error dialog is bound to application window
//
// Revision 1.7  2001/03/31 10:16:42  ydario
// network play ESC fix
//
// Revision 1.6  2001/03/03 19:29:44  ydario
// Revision 1.5  2001/02/24 13:35:22  bpereira
// Revision 1.4  2000/10/16 21:21:11  hurdler
//
// Revision 1.3  2000/08/10 14:59:41  ydario
// OS/2 port
//
// Revision 1.2  2000/08/10 11:07:51  ydario
// Revision 1.1  2000/08/09 12:13:38  ydario
// OS/2 specific platform code
//
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: I_system.c 1649 2023-10-25 02:11:02Z wesleyjohnson $";


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "doomincl.h"

#define INCL_DOSDEVIOCTL
#include "I_os2.h"

#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"
#include "i_joy.h"


#ifdef JOYSTICK_SUPPORT
JoyType_t Joystick;
#endif

byte    mb_used = 12;

// Do not execute cleanup code more than once. See Shutdown_xxx() routines.
byte    keyboard_started=0;
byte    sound_started=0;
boolean timer_started = false;
boolean mouse_enabled = false;
#ifdef JOYSTICK_SUPPORT
boolean joystick_detected;
#endif

#ifdef JOYSTICK_SUPPORT
void I_InitJoystick (void) {}
#endif
// Called on video mode change, usemouse change, mousemotion change,
// and game paused.
//   play_mode : enable mouse containment during play
void I_StartupMouse( boolean play_mode ) {};
void I_StartupMouse2(void) {}
void I_StartupTimer(void) {}
void I_StartupKeyboard (void) {}

//  Return a key that has been pushed, or 0
//  (replace getchar() at game startup)
//
int I_GetKey (void)
{
    event_t   *ev;
        
    if (eventtail != eventhead)
    {
        ev = &events[eventtail];
        eventtail = (++eventtail)&(MAXEVENTS-1);
        if (ev->type == ev_keydown)
            return ev->data1;
        else
            return 0;
    }
    return 0;
}

void I_Tactile ( int on, int off, int total )
{
  // UNUSED.
  on = off = total = 0;
}

ticcmd_t        emptycmd;
ticcmd_t*       I_BaseTiccmd(void)
{
    return &emptycmd;
}


//
// I_GetFreeMem
// return free and total memory in the system
//
ULONG I_GetFreeMem(ULONG *total)
{
   ULONG pr_arena;

   DosQuerySysInfo( QSV_TOTPHYSMEM, QSV_TOTPHYSMEM, 
                    (PVOID) total, sizeof(ULONG));
   DosQuerySysInfo( QSV_MAXPRMEM, QSV_MAXPRMEM, 
                    (PVOID) &pr_arena, sizeof(ULONG));

   return pr_arena;
}


//
// I_GetTime
// returns time in 1/70th second tics
//
ULONG  I_GetTime (void)
{
    struct timeval      tp;
    struct timezone     tzp;
    int                 newtics;
    static int          basetime=0;

    gettimeofday(&tp, &tzp);
    if (!basetime)
        basetime = tp.tv_sec;
    newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;

    return newtics;
}




#if 0
// Replaced by D_Quit_Save, I_Quit_System
//
// I_Quit
//
void I_Quit (void)
{
#ifdef DEBUG
   printf( "I_Quit\n");
#endif

    //added:16-02-98: when recording a demo, should exit using 'q' key,
    //        but sometimes we forget and use 'F10'.. so save here too.
    if (demorecording)
        G_CheckDemoStatus();

    D_Quit_NetGame();

    // shutdown everything that was started !
    I_ShutdownSystem();
#ifdef DEBUG
   printf( "I_Quit: shut down everything\n");
#endif

    // use this for 1.28 19990220 by Kin
    M_SaveConfig (NULL);

/*
      // Send CLOSE to PM
   printf( "ShutdownPMSession: send wm_close\n");
   WinPostMsg( pmData->hwndClient, WM_CLOSE, 0, 0);
*/

#ifdef DEBUG
   printf( "I_Quit: _endthread(0)\n");
#endif
   //_endthread();
   exit(0);
}
#endif

// sleeps for the given amount of milliseconds
void I_Sleep(unsigned int ms)
{
#if 1
    usleep( ms * 1000 );  // unistd
#else
    DosSleep( ms );
#endif
}

#ifdef LOADING_DISK_ICON
void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}
#endif

byte*   I_AllocLow(int length)
{
    byte*       mem;

    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}

// ----------
// I_GetEvent
// Post new events for all sorts of user-input
// ----------
static void I_GetKeyboardEvents (void);
static void I_GetMouseEvents (void);
#ifdef JOYSTICK_SUPPORT
static void I_GetJoystickEvents (void);
#endif

void I_GetEvent (void)
{
   I_GetKeyboardEvents();
   //I_GetMouseEvents();
#ifdef JOYSTICK_SUPPORT
   //I_GetJoystickEvents();
#endif
}


static event_t ev_alt_up = { ev_keyup, KEY_LALT, 0, 0 };
static event_t ev_alt_do = { ev_keydown, KEY_LALT, 0, 0 };
static event_t ev_lshf_up = { ev_keyup, KEY_LSHIFT, 0, 0 };
static event_t ev_lshf_do = { ev_keydown, KEY_LSHIFT, 0, 0 };
static event_t ev_rshf_up = { ev_keyup, KEY_RSHIFT, 0, 0 };
static event_t ev_rshf_do = { ev_keydown, KEY_RSHIFT, 0, 0 };

void I_GetKeyboardEvents(void)
{
   APIRET   rc;
   ULONG    ulReturn;
   static HFILE hkbd;
   SHIFTSTATE shiftstate;
//   event_t  ev;
//      data1 is 16bit keycode
//      data2 is ASCII char
 
   if (hkbd == NULL) {
       DosOpen( (PSZ) "kbd$",                    // open driver
                (HFILE*)&hkbd, &ulReturn, 0, 0, FILE_OPEN,
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE, NULL);
      //printf( "I_GetKeyboardEvents DosOpen %08x\n", hkbd);
   }

   if (!hkbd)
      return;

      // query shift state   
   rc = DosDevIOCtl( hkbd, IOCTL_KEYBOARD, KBD_GETSHIFTSTATE,
                    NULL, NULL, NULL,
                    (void*) &shiftstate, sizeof(SHIFTSTATE), &ulReturn);
   //printf( "I_GetKeyboardEvents IOCTL rc=%d, shift=%08x\n", rc, shiftstate.fsState);

   if (shiftstate.fsState & ALT) {
      if (!pmData->fAltPressed) {
         D_PostEvent(&ev_alt_do);
         pmData->fAltPressed = 1;
      }
   } else {
      if (pmData->fAltPressed) {
         D_PostEvent(&ev_alt_up);
         pmData->fAltPressed = 0;
      }
   }

   if (shiftstate.fsState & LEFTSHIFT) {
      if (!pmData->fShiftPressed&0x02) {
         D_PostEvent(&ev_lshf_do);
         pmData->fShiftPressed |= 0x02;
      }
   } else {
      if (pmData->fShiftPressed) {
         D_PostEvent(&ev_lshf_up);
         pmData->fShiftPressed &= ~0x02;
      }
   }
   if (shiftstate.fsState & RIGHTSHIFT) {
      if (!pmData->fShiftPressed&1) {
         D_PostEvent(&ev_rshf_do);
         pmData->fShiftPressed |= 1;
      }
   } else {
      if (pmData->fShiftPressed&1) {
         D_PostEvent(&ev_rshf_up);
         pmData->fShiftPressed &= ~1;
      }
   }

}

//
// I_OsPolling
//
void I_OsPolling (void)
{
    QMSG   qmsg;

        while (WinPeekMsg( pmData->hab, &qmsg, (HWND) NULL, 0, 0, PM_NOREMOVE))
        {
            if (WinGetMsg( pmData->hab, &qmsg, (HWND) NULL, 0, 0))
                WinDispatchMsg( pmData->hab, &qmsg);
            else  // winspec : this is quit message
                I_Quit ();
        }

    // this is called by the network synchronization,
    // check keys and allow escaping
    I_GetEvent();
}

//
// I_Error
//
#if 0
extern boolean demorecording;
#endif

void I_Error (char *error, ...)
{
    va_list     argptr;

    // Message first.
   char  buffer[ 1024];
   va_start (argptr,error);
   vsprintf( buffer, error, argptr);
   printf( "Error: %s\n", buffer);
   WinMessageBox( HWND_DESKTOP, 
                  pmData->hwndFrame ? pmData->hwndFrame : HWND_DESKTOP,
                  (PSZ) buffer,
                  pmData->title, 
                  0, MB_OK | MB_INFORMATION );
    va_end (argptr);

#if 1
    D_Quit_Save( QUIT_panic );  // No save, safe shutdown
#else
    //added:18-02-98: save one time is enough!
    //if (!errorcount)
    {
        M_SaveConfig (NULL);   //save game config, cvars..
    }
        
    //added:16-02-98: save demo, could be useful for debug
    //                NOTE: demos are normally not saved here.
    if (demorecording)
        G_CheckDemoStatus();

    D_Quit_NetGame();

    I_Sleep( 3000 );  // to see some messages
    // shutdown everything that was started !
    I_ShutdownSystem();
#endif

    exit(-1);
}

// The final part of I_Quit, system dependent.
void I_Quit_System (void)
{
    exit(0);
}


// ===========================================================================================
// CLEAN STARTUP & SHUTDOWN HANDLING, JUST CLOSE EVERYTHING YOU OPENED.
// ===========================================================================================
//
//
#define MAX_QUIT_FUNCS     16

typedef void (*quitfuncptr)();

static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] = {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
    int c;
    
    for (c=0; c<MAX_QUIT_FUNCS; c++) {
        if (!quit_funcs[c]) {
            quit_funcs[c] = func;
            break;
        }
    }
}


#if 0
// Unused
//  Removes a function from the list that need to be called by I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
    int c;
    
    for (c=0; c<MAX_QUIT_FUNCS; c++) {
        if (quit_funcs[c] == func) {
            while (c<MAX_QUIT_FUNCS-1) {
                quit_funcs[c] = quit_funcs[c+1];
                c++;
            }
            quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
            break;
        }
    }
}
#endif


//  This stuff should get rid of the exception and page faults when
//  Doom bugs out with an error. Now it should exit cleanly.
//
void  I_StartupSystem(void)
{

    // some 'more globals than globals' things to initialize here ?
    keyboard_started = false;
    sound_started = false;
    timer_started = false;
    cdaudio_started = false;
        
    // check for OS type and version here ?
#ifdef NDEBUG
    signal(SIGABRT, signal_handler);
    signal(SIGFPE , signal_handler);
    signal(SIGILL , signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT , signal_handler);
#endif

}

// Shutdown joystick and other interfaces, before I_ShutdownGraphics.
void I_Shutdown_IO(void)
{
#ifdef JOYSTICK_SUPPORT
//    I_ShutdownJoystick();
#endif
}

//  Closes down everything. This includes restoring the initial
//  pallete and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
//
void I_ShutdownSystem()
{
    int c;
    
    for (c=MAX_QUIT_FUNCS-1; c>=0; c--)
    {
        if (quit_funcs[c]) {
            printf( "I_ShutdownSystem: calling func#%d\n", c);
            (*quit_funcs[c])();
        }
    }
}


void I_GetDiskFreeSpace(INT64 *freespace)
{
        *freespace = INT_MAX;
}

char *I_GetUserName(void)
{
static char username[MAXPLAYERNAME];
     char * p;
     if((p=getenv("USER"))==NULL)
         if((p=getenv("user"))==NULL)
            if((p=getenv("USERNAME"))==NULL)
               if((p=getenv("username"))==NULL)
                  return NULL;
     strncpy(username,p,MAXPLAYERNAME);

     if( strcmp(username,"")==0 )
         return NULL;
     return username;
}


// Get the directory of this program.
//   defdir: the current directory
//   dirbuf: a buffer of length MAX_WADPATH, 
// Return true when success, dirbuf contains the directory.
boolean I_Get_Prog_Dir( char * defdir, /*OUT*/ char * dirbuf )
{
    char * dnp;
#if 0
    int  len;

//    if(  )
    {
        // FIXME for OS2
        len = GetModuleFileName( NULL, dirbuf, MAX_WADPATH-1 );
        if( len > 1 )
        {
	    dirbuf[len] = 0;  // does not terminate string ??
	    goto got_path;
	}
    }
#endif   

    // The argv[0] method
    char * arg0p = myargv[0];
//    GenPrintf(EMSG_debug, "argv[0]=%s\n", arg0p );
    // Windows, DOS, OS2
    if( arg0p[0] == '\\' )
    {
        // argv[0] is an absolute path
        strncpy( dirbuf, arg0p, MAX_WADPATH-1 );
        dirbuf[MAX_WADPATH-1] = 0;
        goto got_path;
    }
    // Windows, DOS, OS2
    else if( strchr( arg0p, '/' ) || strchr( arg0p, '\\' ) )
    {
        // argv[0] is relative to current dir
        if( defdir )
        {
	    cat_filename( dirbuf, defdir, arg0p );
	    goto got_path;
	}
    }
    goto failed;
   
got_path:
    // Get only the directory name
    dnp = dirname( dirbuf );
    if( dnp == NULL )  goto failed;
    if( dnp != dirbuf )
    {
        cat_filename( dirbuf, "", dnp );
    }
    return true;

failed:
    dirbuf[0] = 0;
    return false;
}


int  I_mkdir(const char *dirname, int unixright)
{
    return mkdir(dirname,unixright);
}
