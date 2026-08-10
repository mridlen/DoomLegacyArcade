// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_main.h 1044 2013-08-26 20:37:47Z wesleyjohnson $
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
// $Log: win_main.h,v $
// Revision 1.5  2000/08/03 17:57:42  bpereira
//
// Revision 1.4  2000/04/27 18:02:35  hurdler
// changed boolean to int (at least it compiles on my computer)
//
// Revision 1.3  2000/04/23 16:19:52  bpereira
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      
//
//-----------------------------------------------------------------------------

#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "../doomdef.h"
  // LOGMESSAGES

extern  HINSTANCE       main_prog_instance;
extern  HWND            hWnd_main;

extern  int             appActive;

extern  byte  cdaudio_started;
extern  byte  sound_started;
extern  byte  music_started;
extern  byte  keyboard_started;
extern  byte  have_DX0300;
extern  boolean  win95;
extern  boolean  winnt;
//faB: midi channel Volume set is delayed by the MIDI stream callback thread, see win_snd.c
#define WM_MSTREAM_UPDATEVOLUME (WM_USER + 101)

#define MSH_WHEEL
#ifdef MSH_WHEEL
extern unsigned int MSHWheelMessage;
#endif

// defined in win_sys.c
void    I_BeginProfile (void);    //for timing code
DWORD   I_EndProfile (void);

void I_GetLastErrorMsgBox (void);

void I_SaveMemToFile (unsigned char* pData, unsigned long iLength, char* sFileName);

// output formatted string to file using win32 functions (win_dbg.c)
void FPrintf (HANDLE fileHandle, LPCTSTR lpFmt, ...);

#endif
