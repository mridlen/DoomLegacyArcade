// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: st_lib.h 1235 2016-05-24 17:33:58Z wesleyjohnson $
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
// $Log: st_lib.h,v $
// Revision 1.3  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      The status bar widget code.
//
//-----------------------------------------------------------------------------


#ifndef STLIB_H
#define STLIB_H

// We are referring to patches.
#include "r_defs.h"
#include "w_wad.h"

//
// Background and foreground screen numbers
//
#define BG 1
#define FG 0

// STlib global enables.
// These are removed to globals to make the repetitive update calls
// as clean as possible, thus smaller and faster.
// These enables only need to be changed infrequently.

// Enable to erase from copy BG.  Clear for overlay and hardware draw.
extern boolean  stlib_enable_erase;

// Enable to refresh from BG.  Clear to do additive updates.
extern boolean  stlib_force_refresh;
  // This replaces the refresh parameter which appeared on every call,
  // and only infrequently was set.

//
// Typedefs of widgets
//
// [WDJ]  Removed on flag from all widgets.
// It was only tested immediately with a return from the draw function.
// The caller can do the same test with much less overhead,
// and much clearer code, without all the indirection and ptrs.

// Widget commands
typedef enum {
    STLIB_REFRESH = 2,
    STLIB_FLASH,
    STLIB_FLASH_CLEAR
} stlib_command_e;

// Number widget

typedef struct
{
    // flash
    byte     command;   //  stlib_command_e

    // upper right-hand corner
    //  of the number (right-justified)
    uint16_t   x, y;

    // max # of digits in number
    byte       width;

    // last number value  (-1000 .. +1000)
    int32_t    prev_num;
   
    // pointer to current value
    int32_t *  num;

    // list of patches for 0-9, minus at [10], percent at [11]
    patch_t**  patches;

    // unused
    // user data
//    int data;

} st_number_t;



// Multiple Icon widget
typedef struct
{
    // flash
    byte     command;   //  stlib_command_e

     // center-justified location of icons
    uint16_t   x, y;

    // last icon number index  (0 .. +1000)
    int16_t    prev_icon_index;

    // pointer to current icon index
    int *      icon_index;

    // list of icons
    patch_t**  patches;

    // unused
    // user data
//    int                 data;

} st_multicon_t;




// Binary Icon widget

typedef struct
{
    // flash
    byte     command;   //  stlib_command_e

    // center-justified location of icon
    uint16_t   x, y;

    boolean    prev_val;

    // pointer to current icon status
    boolean *  boolval;

    patch_t *  patch;      // icon
 
    // unused
//    int                 data;   // user data

} st_binicon_t;


//
// Widget creation, access, and update routines
//


// Number widget routines
void STlib_initNum ( st_number_t* ni, int x, int y,
  patch_t** patch_list, int* num, int width );

void STlib_updateNum ( st_number_t* ni );

// Draw a number as a percentage.
// Percent needs to be in the number font at [11].
void STlib_updatePercent ( st_number_t*  per );


// Multiple Icson widget routines
void STlib_initMultIcon ( st_multicon_t* mi, int x, int y,
  patch_t** patch_list, int* icon_index );


void STlib_updateMultIcon ( st_multicon_t* mi );

// Binary Icon widget routines

void STlib_initBinIcon ( st_binicon_t* bi, int x, int y,
  patch_t* patch, boolean* val );

void STlib_updateBinIcon ( st_binicon_t* bi );

#endif
