// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_argv.h 1730 2025-03-06 13:17:08Z wesleyjohnson $
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
//
// $Log: m_argv.h,v $
// Revision 1.3  2000/03/29 19:39:48  bpereira
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//    Multiple parameters
//    
//-----------------------------------------------------------------------------

#ifndef M_ARGV_H
#define M_ARGV_H

#include "doomtype.h"

//
// MISC
//
extern  int     myargc;
extern  char**  myargv;

// Returns the position of the given parameter
// in the arg list (0 if not found).
int  M_CheckParm (const char* check);


// push all parameters bigining by a +, ex : +map map01
void M_PushSpecialParameters( void );

// return true if there is available parameters
// use it before M_GetNext 
boolean M_IsNextParm(void);

// return the next parameter after a M_CheckParm
// NULL if not found, use M_IsNextParm to find if there is a parameter
char * M_GetNextParm(void);

// Find a Response File
void M_FindResponseFile (void);

#ifdef LAUNCHER
void M_Remove_Param( int i );
void M_Remove_matching_Param( const char * p1, const char * p2 );
// add a param from Launcher, p2 is optional
void M_Add_Param( const char * p1, const char * p2 );
// add two param from Launcher, or remove them if p2==NULL or empty string
void M_Change_2Param( const char * p1, const char * p2 );
// Clear all param from Add_Param
void M_Clear_Add_Param( void );
#endif

#endif //M_ARGV_H
