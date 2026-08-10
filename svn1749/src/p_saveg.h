// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_saveg.h 1618 2022-01-24 21:01:46Z wesleyjohnson $
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
// $Log: p_saveg.h,v $
// Revision 1.4  2000/11/11 13:59:45  bpereira
//
// Revision 1.3  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Savegame I/O, archiving, persistence.
//
//-----------------------------------------------------------------------------


#ifndef P_SAVEG_H
#define P_SAVEG_H

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif


// Persistent storage/archiving.
// These are the load / save game routines.

// Allocate malloc an appropriately sized buffer
// Header-only, or data sized buffer (large).
byte *  P_Alloc_savebuffer( boolean large_size );

// Setup savegame file write
int  P_Savegame_Writefile( const char * filename );
// Setup savegame file read
int  P_Savegame_Readfile( const char * filename );
// Close savegame file, and return error indication <0
int  P_Savegame_Closefile( boolean writeflag );
// In case of error
void  P_Savegame_Error_Closefile( void );

// return SAVEBUF_OVERFLOW_LENGTH if overrun the buffer
#define SAVEBUF_OVERFLOW_LENGTH  0xFF000000
size_t  P_Savegame_length( void );

// Write savegame header to savegame buffer.
void   P_Savegame_Write_header( const char * description, byte write_netgame );

typedef struct {
   char * name;	// description
   char * game; // game name
   char * wad;  // wad name containing level
   char * map;  // level MAP01, E1M2, etc.
   char * levtime; // time into this level, as string
   boolean  have_game, have_wad;  // validity checks on read
   char   msg[60];  // currently uses 46 chars max
} savegame_info_t;

// Read savegame header from savegame buffer.
// Returns header info in infop, as pointers into savebuffer.
// Returns 1 when header is correct.
boolean P_Savegame_Read_header( savegame_info_t * infop, byte read_netgame );

void    P_Savegame_Save_game (void);
boolean P_Savegame_Load_game (void);

extern byte*		savebuffer;
extern byte*            save_p; 
extern size_t		savebuffer_size;

#endif
