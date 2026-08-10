// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_inter.h 1393 2018-05-31 04:53:50Z wesleyjohnson $
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
// $Log: p_inter.h,v $
// Revision 1.6  2002/08/24 22:42:03  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.5  2001/11/02 21:39:45  judgecutor
// Added Frag's weapon falling
//
// Revision 1.4  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.3  2000/11/04 16:23:43  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//
//
//-----------------------------------------------------------------------------

#ifndef P_INTER_H
#define P_INTER_H

#include "doomtype.h"
#include "d_player.h"
#include "p_mobj.h"
#include "command.h"
  // consvar_t


#ifdef __GNUG__
#pragma interface
#endif

extern consvar_t cv_fragsweaponfalling;
extern boolean onground;  // player is onground

// Boris hack : preferred weapons order
void VerifFavoritWeapon (player_t *player);

int FindBestWeapon(player_t *player);

//  power : powertype index
boolean P_GivePower ( player_t* player, int power );

//  num : amount of health
boolean P_GiveHealth( player_t* player, int num );

void P_CheckFragLimit(player_t *p);

void P_KillMobj ( mobj_t*       target,
                  mobj_t*       inflictor,
                  mobj_t*       source );


//added:28-02-98: boooring handling of thing(s) on top of thing(s)
/* BUGGY CODE
void P_CheckSupportThings (mobj_t* mobj);
void P_MoveSupportThings (mobj_t* mobj, fixed_t xmove,
                                        fixed_t ymove,
                                        fixed_t zmove);
void P_LinkFloorThing(mobj_t* mobj);
void P_UnlinkFloorThing(mobj_t* mobj);
*/

// Player Message for cheats and object interations.
// Subject to message level control.
// msglevel:
//   0..9    debug
//   10..19  verbose
//   20..29  normal play
//   30..39  minimal msg play
//   40..49  major
//   50..59  god mode messages
//   60..64  mandatory (even when messages are off)
void  P_SetMessage(player_t *player, char *message, byte msglevel);

#endif
