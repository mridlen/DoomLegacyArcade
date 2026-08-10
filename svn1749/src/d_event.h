// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_event.h 1711 2025-01-17 04:17:06Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2010 by DooM Legacy Team.
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
// $Log: d_event.h,v $
// Revision 1.3  2001/03/03 06:17:33  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION: 
//      event handling.
//    
//-----------------------------------------------------------------------------

#ifndef D_EVENT_H
#define D_EVENT_H

#include "doomtype.h"

// Input event types.
typedef enum
{
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_mouse2,
    ev_textchar, // SDL2
} evtype_t;

// Event structure.
typedef struct
{
    evtype_t    type;
    uint32_t    data1;          // keys / mouse/joystick buttons
    int         data2;          // mouse x move, ASCII char
    int         data3;          // mouse y move
} event_t;


//
// GLOBAL VARIABLES
//
#define MAXEVENTS               64

extern  event_t         events[MAXEVENTS];
extern  int             eventhead;
extern  int             eventtail;

// current modifier key status
extern byte shiftdown;
extern byte altdown;

#endif
