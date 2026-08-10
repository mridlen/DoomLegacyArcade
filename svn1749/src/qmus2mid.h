// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: qmus2mid.h 1648 2023-02-10 13:23:27Z wesleyjohnson $
//
// Copyright (C) 1995 by Sebastien Bacquet.
// Portions Copyright (C) 1998-2013 by DooM Legacy Team.
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
// $Log: qmus2mid.h,v $
// Revision 1.4  2000/09/10 10:46:15  metzgermeister
// merged with SDL version
//
// Revision 1.3  2000/04/21 08:23:47  emanne
// To have SDL working.
// qmus2mid.h: force include of qmus2mid_sdl.h when needed.
//
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//   MUS format (music in Doom lumps) to MIDI conversion.
//
//
//-----------------------------------------------------------------------------

#ifndef QMUS2MID_H
#define QMUS2MID_H

#include "doomtype.h"

extern byte  EN_create_program;
extern byte  EN_midi_compress;

// Call this function to load all music lumps.
void* S_CacheMusicLump(int lump);


typedef enum {
  QM_success,
  QM_NOTMUSFILE,   // Not a MUS file
  QM_COMUSFILE,    // Can't open MUS file
  QM_COTMPFILE,    // Can't open TMP file
  QM_CWMIDFILE,    // Can't write MID file
  QM_MUSFILECOR,   // MUS file corrupted
  QM_TOOMCHAN,     // Too many channels
  QM_MEMALLOC,     // Memory allocation error
  QM_MIDTOOLARGE,  // If the mid don't fit in the buffer
} QMUS_error_code_e;


/* some (old) compilers mistake the "MUS\x1A" construct (interpreting
   it as "MUSx1A")      */
#define MUSHEADER     "MUS\032"
  // this seems to work

#ifdef __OS2__
// Does not use qmus2mid
#else
// Buffer to Buffer version
// Return QMUS_error_code_e
int qmus2mid (byte  *mus,     // input mus
              int muslength,  // input mus length
              uint16_t division, // ticks per quarter note
              int nocomp,     // no compression, is ignored
              int midbuffersize, // output buffer length
    /*INOUT*/
              byte *mid,  // output buffer in memory
              unsigned long* midilength //faB: return midi file length
             );
#endif

#endif
