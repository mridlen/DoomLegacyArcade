// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_system.h 1257 2016-09-20 17:14:21Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: i_system.h,v $
// Revision 1.8  2001/04/27 13:32:14  bpereira
// Revision 1.7  2001/02/24 13:35:20  bpereira
// Revision 1.6  2000/10/21 08:43:29  bpereira
// Revision 1.5  2000/10/02 18:25:45  bpereira
//
// Revision 1.4  2000/04/25 19:49:46  metzgermeister
// support for automatic wad search
//
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef I_SYSTEM_H
#define I_SYSTEM_H

#include "doomtype.h"
#include "d_ticcmd.h"
  // ticcmd_t
#include "d_event.h"

// [WDJ] To inform and control graphics startup and shutdown.
typedef enum {
   VGS_off,  // Unusable
   // Some querys allowed, but not full graphics.
   VGS_shutdown, // Mostly used to detect shutdown loops due to errors.
   VGS_startup,
   // Usable graphics for most of program.
   VGS_active,  // have minimal graphics
   VGS_fullactive  // have full graphics
} graphics_state_e;
extern byte graphics_state;  // graphics_state_e


// system initialization
void I_SysInit(void);

// return free and total physical memory in the system
uint64_t I_GetFreeMem(uint64_t *total);

// Called by D_DoomLoop,
// returns current time in tics.
tic_t I_GetTime (void);

// replace getchar() once the keyboard has been appropriated
int I_GetKey (void);

void I_GetEvent (void);


//
// Called by D_DoomLoop,
// called before processing any tics in a frame
// (just after displaying a frame).
// Time consuming syncronous operations
// are performed here (joystick reading).
// Can call D_PostEvent.
//
void I_StartFrame (void);


//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_OsPolling (void);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t* I_BaseTiccmd (void);

// sleeps for the given amount of milliseconds
void I_Sleep(unsigned int ms);

// Called by M_Responder when quit is selected, return code 0.
typedef enum {
   QUIT_normal,  // commanded quit
   QUIT_shutdown,  // error quit
   QUIT_panic    // I_Error or worse
} quit_severity_e;
// Quit without error (exit 0), no return, QUIT_normal.
void I_Quit (void);
// The system independent quit and save config.
void D_Quit_Save ( quit_severity_e severity );
// The final part of I_Quit, system dependent.
void I_Quit_System (void);
// Show the EndText, after the graphics are shutdown.
void I_Show_EndText( uint16_t * endtext );

void I_Error (const char *error, ...);

void I_Tactile (int on, int off, int total);

//added:18-02-98: write a message to stderr (use before I_Quit)
//                for when you need to quit with a msg, but need
//                the return code 0 of I_Quit();
void I_OutputMsg (char *error, ...);

#ifdef SMIF_WIN_NATIVE
void I_MsgBox (char * msg );
#endif

/* list of functions to call at program cleanup */
void I_AddExitFunc (void (*func)(void));
void I_RemoveExitFunc (void (*func)(void));

// Setup signal handler, plus stuff for trapping errors and cleanly exit.
// Not called by game, port optional, see I_SysInit
void I_StartupSystem (void);
// Not called by game, port optional, see I_Quit
void I_ShutdownSystem (void);

uint64_t I_GetDiskFreeSpace(void);
char *I_GetUserName(void);
int  I_mkdir(const char *dirname, int unixright);

// Get the directory of this program.
//   defdir: the default directory, if must use argv[0] method (may be NULL)
//   dirbuf: a buffer of length MAX_WADPATH, 
// Return true when success, dirbuf contains the directory.
boolean I_Get_Prog_Dir( char * defdir, /*OUT*/ char * dirbuf );

// Called on video mode change, usemouse change, mousemotion change,
// and game paused.
//   play_mode : enable mouse containment during play
void I_StartupMouse( boolean play_mode );
void I_StartupMouse2(void);
void I_UngrabMouse(void);

// Shutdown joystick and other interfaces, before I_ShutdownGraphics.
void I_Shutdown_IO(void);

#endif
