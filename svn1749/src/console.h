// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: console.h 1652 2023-11-14 09:10:51Z wesleyjohnson $
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
// $Log: console.h,v $
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Drop down console for entering commands.
//   
//-----------------------------------------------------------------------------

#ifndef CONSOLE_H
#define CONSOLE_H

#include "d_event.h"
  // event_t

#if 0
// unused
// for debugging should be replaced by nothing later.. so debug is inactive
#define LOG(x) CONS_Printf(x)
#endif

void CON_Register (void);
void CON_Init_Setup (void);
void CON_Init_Video (void);

boolean CON_Responder (event_t * ev);

extern boolean console_open;  // console is open, no demos

// set true when screen size has changed, to adapt console
extern boolean con_recalc;

extern boolean con_self_refresh;

// top clip value for view render: do not draw part of view hidden by console
extern int     con_clipviewtop;

// 0 means console if off, or moving out
extern int     con_destlines;

extern int     con_clearlines;  // lines of top of screen to refresh
extern boolean con_hudupdate;   // hud messages have changed, need refresh

extern byte*   whitemap;
extern byte*   greenmap;
extern byte*   graymap;

void CON_Clear_HUD (void);       // clear heads up messages

void CON_Ticker (void);
void CON_Drawer (void);       // full feature
void CON_Draw_Console (void);  // text to console
void CONS_Error (char * msg);       // print out error msg, and wait a key

// force console to move out
void CON_ToggleOff (void);

#endif
