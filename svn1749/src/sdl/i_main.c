// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_main.c 1562 2020-11-29 11:51:00Z wesleyjohnson $
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
// $Log: i_main.c,v $
// Revision 1.4  2003/06/05 20:36:10  hurdler
// do not write log.txt if no .log directory exists
//
// Revision 1.3  2002/09/10 19:30:27  hurdler
// Add log file under Linux
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
// 
//
// DESCRIPTION:
//      Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
  // stdlib, stdio, string
  // MAC_SDL, LOGMESSAGES

#include "m_argv.h"
#include "d_main.h"

#ifdef MAC_SDL
// [WDJ] SDL 1.2.x Necessary on Mac to setup objective-C stuff.
// It is reported that SDL 1.3 will not require SDL_main.
// Must keep SDL_main until such time, if any, that SDL 1.2.x is not supported,
// probably around 2015.
// This include will rename main as SDL_main, and use a main from SDL.
// Must also compile and link SDLmain.m, which is objective-C program.
# include <SDL.h>
  // This will also get SDL_main.h, SDL_config.h, SDL_platform.h
#endif

#ifdef LOGMESSAGES
#include <stdio.h>
#define  LOGFILENAME   "log.txt"
FILE *logstream = NULL;

void  shutdown_logmessage( const char * who, const char * msg )
{
    if( logstream )
    {
        fprintf( logstream, "%s: %s\n", who, msg );
        fclose( logstream );
        logstream = NULL;
    }
}
#endif

int main(int argc, char **argv)
{ 
    myargc = argc; 
    myargv = argv; 
 
#ifdef MAC_SDL
// __MACOS__ is defined in SDL_platform.h (==macintosh)
# ifdef __MACOS__
    // [WDJ] As stated in SDL_main.h, but not needed for MACOSX
    struct QDGlobals quickdraw_g;
    SDL_InitQuickDraw( & quickdraw_g );
# endif
#endif

#ifdef LOGMESSAGES
    //Hurdler: only write log if we have the permission in the current directory
    logstream = fopen( LOGFILENAME, "w");
    // console and GenPrintf not initialized yet.
    printf( "Log file %s: %s\n", (logstream)? "opened":"open failed", LOGFILENAME );
#endif

    D_DoomMain ();
    D_DoomLoop ();
    return 0;
} 
