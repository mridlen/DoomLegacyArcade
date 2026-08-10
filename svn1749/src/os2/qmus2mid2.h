// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: qmus2mid2.h
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
// DESCRIPTION:
//   MUS format (music in Doom lumps) to MIDI conversion.
//
//
//-----------------------------------------------------------------------------
#ifndef QMUS2MID2_H
#define QMUS2MID2_H

#include ../qmus2mid.h
  // common qmus2mid declares


// file to file version
// Return QMUS_error_code_e
int qmus2mid_file( const char *mus, const char *mid, int nodisplay,
              int2 division, int BufferSize, int nocomp );

/*
HMMIO    qmus2mid( const char *mus, int muslen, int nodisplay,
             int2 division, int BufferSize, int nocomp);
*/
#endif


