// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_think.h 1719 2025-01-25 05:52:31Z wesleyjohnson $
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
// $Log: d_think.h,v $
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//  Mobj, and some mapthings, as thinkers.
//  MapObj data. Map Objects or mobjs are actors, entities,
//  thinker, take-your-pick... anything that moves, acts, or
//  suffers state changes of more or less violent nature.
//
//-----------------------------------------------------------------------------

#ifndef D_THINK_H
#define D_THINK_H


#ifdef __GNUG__
#pragma interface
#endif


//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//
typedef  void (*actionf_v)();
typedef  void (*actionf_p1)( void* );
typedef  void (*actionf_p2)( void*, void* );

#if 0
typedef union
{
  actionf_v     acv;
  actionf_p1    acp1;
  actionf_p2    acp2;

} actionf_t;
#endif 

typedef uint16_t   action_fi_t;  // action_e




// Historically, "think_t" is yet another
//  function pointer to a routine to handle
//  an actor.
// [WDJ] Convoluted, and unsafe, requiring casting to avoid the compiler noticing.
// Use a decent enum instead, and it will be debuggable.
// Savegame has a similar but independent enumeration.
typedef enum {
  TFI_NULL,
  TFI_MobjNullThinker, // (mobj_t * mobj), but does nothing
  TFI_MobjThinker,  // (mobj_t * mobj)
  TFI_MoveCeiling,  // (ceiling_t* ceiling)
  TFI_VerticalDoor, // (vldoor_t * door)
  TFI_MoveFloor,    // (floormove_t* mfloor)
  TFI_PlatRaise,    // (plat_t* plat)
  TFI_LightFlash,   // (lightflash_t* flash)
  TFI_StrobeFlash,  // (strobe_t*  flash)
  TFI_Glow,         // (glow_t* gp)
  TFI_FireFlicker,  // (fireflicker_t* flick)
  TFI_LightFade,    // (lightfader_t * lf)
  TFI_MoveElevator, // (elevator_t* elevator)
  TFI_Scroll,       // (scroll_t *s)
  TFI_Friction,     // (friction_t *f)
  TFI_Pusher,       // (pusher_t *p)
// Heretic    
  TFI_BlasterMobjThinker,  // (mobj_t *mobj) // savegame ??

  TFI_RemoveThinker, // (mobj_t * mobj)
  TFI_END,
} TFI_func_e;



// Doubly linked list of actors.
typedef struct thinker_s  thinker_t;
typedef struct thinker_s
{
    thinker_t  * prev, * next;
#ifdef DEBUG_WINDOWED
    TFI_func_e   function;  // easier to read debugging
#else
    // Because getting the compiler to save space is painful.
    byte   function;   // TFI_func_e
#endif

    // killough 8/29/98: maintain thinkers in several equivalence classes,
    // according to various criteria, so as to allow quicker searches.
    thinker_t  * cnext, * cprev; // linked list thinkers in same class

#ifdef REFERENCE_COUNTING
    // killough 11/98: count of how many other objects reference
    // this one using pointers. Used for garbage collection.
    uint16_t  references;
#endif
} thinker_t;



#endif
