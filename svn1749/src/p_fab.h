// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_fab.h 1686 2024-06-20 09:45:56Z wesleyjohnson $
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
// $Log: p_fab.h,v $
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//    FAB
//
//-----------------------------------------------------------------------------

#ifndef P_FAB_H
#define P_FAB_H

#include "doomtype.h"
#include "p_mobj.h"
#include "command.h"
  // consvar_t

extern consvar_t cv_bloodtime;

void  DemoAdapt_p_fab(void);  // local enables of p_fab

// spawn smoke trails behind rockets and skull head attacks
void A_SmokeTrailer (mobj_t* actor);

void Translucency_OnChange(void);  // update translucent info

// hack the states table to set Doom Legacy's default translucency on sprites
void P_SetTranslucencies (void);

// Add commands for deathmatch rules and style (like more blood) :)
extern consvar_t * fab_cvar_list[];

#endif
