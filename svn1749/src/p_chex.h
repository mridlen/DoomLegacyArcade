// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_chex.h 1368 2017-11-01 01:17:48Z wesleyjohnson $
//
// Copyright (C) 2003-2010 by DooM Legacy Team.
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
// DESCRIPTION:
//   Chex Quest support.
//
//-----------------------------------------------------------------------------

#ifndef P_CHEX_H
#define P_CHEX_H

void Chex1_PatchEngine(void);

// Detect Doom graphics and prevent their display while chexquest mode.
void* Chex_safe_pictures( const char* name, void* lumpptr );

#endif
