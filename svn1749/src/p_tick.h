// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_tick.h 1669 2024-03-03 04:38:49Z wesleyjohnson $
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
// $Log: p_tick.h,v $
// Revision 1.3  2000/10/21 08:43:31  bpereira
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      
//
//-----------------------------------------------------------------------------


#ifndef P_TICK_H
#define P_TICK_H

#include "doomtype.h"
  // tic_t
#include "d_think.h"

#ifdef __GNUG__
#pragma interface
#endif

extern tic_t leveltime;



// Called by C_Ticker,
// can call G_PlayerExited.
// Carries out all thinking of monsters and players.
void P_Ticker (void);


// [WDJ] From PrBoom, MBF, EternityEngine, adapted.
// killough 8/29/98: class-lists of thinkers, for more efficient searches
// cph 2002/01/13: for consistency with the main thinker list, keep objects
// pending deletion on a class-list too.
typedef enum {
// The only lists that are actually searched.
  TH_friends,  // live friends
  TH_enemies,  // live enemies
  NUMTHCLASS,
// Conceptual lists, not actually kept.
  TH_misc,
  TH_delete,
// Actions without list.
  TH_all,    // search all
  TH_unknown,  // must classify
  TH_none,
} TH_class_e;


// both the head and tail of the thinker list
extern  thinker_t  thinkercap;
extern  thinker_t  thinkerclasscap[];



void P_Init_Thinkers (void);
void P_AddThinker (thinker_t* thinker);
void P_RemoveThinker (thinker_t* thinker);  // Remove the thinker.
void T_RemoveThinker (thinker_t* thinker);  // Thinker removal action

void P_UpdateClassThink(thinker_t *thinker, int tclass );
// Move in class-list.
//  first: 0=last, 1=first
void P_MoveClassThink(thinker_t *thinker, byte first);
// Move range cap to th, to be last in class-list.
//  cap: is a class-list.
//  thnext: becomes new first in class-list.
void P_MoveClasslistRangeLast( thinker_t * cap, thinker_t * thnext );

#ifdef REFERENCE_COUNTING
// Set the target, with reference counting.
void P_SetReference(mobj_t * rm_mo, mobj_t * add_mo);
# define SET_TARGET_REF( ptrref, newtarget )   { P_SetReference((ptrref), (newtarget)); (ptrref) = (newtarget); }
#else
# define P_SetReference( r, a )
# define SET_TARGET_REF( ptrref, newtarget )   { (ptrref) = (newtarget); }
#endif

#endif
