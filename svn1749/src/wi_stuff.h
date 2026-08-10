// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: wi_stuff.h 1496 2020-01-05 22:11:50Z wesleyjohnson $
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
// $Log: wi_stuff.h,v $
// Revision 1.3  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Intermission.
//
//-----------------------------------------------------------------------------

#ifndef WI_STUFF_H
#define WI_STUFF_H

#include "doomtype.h"
#include "d_player.h"
  // wb_start_t

//added:05-02-98:
typedef struct {
    int  count;
    int  num;
    int  color;
    char *name;
} fragsort_t;

// Called by main loop, animate the intermission.
void WI_Ticker (void);

// Called by main loop,
// draws the intermission directly into the screen buffer.
void WI_Drawer (void);

// Setup for an intermission screen.
void WI_Start(wb_start_t *  wb_start);

void WI_Load_Data(void);
void WI_Release_Data(void);

// Network timeout for next level
void WI_Init_NoState(void);

boolean teamingame(int teamnum);

// draw rankings
//  colwidth : column width
void WI_Draw_Ranking(const char *title, int x, int y, fragsort_t *fragtable,
                    int scorelines, boolean large, int white, int colwidth);

// For startup wait, and deathmatch wait.
extern int  wait_game_start_timer;  // subject to network sync
void WI_Draw_wait( int net_nodes, int net_players, int wait_players, int wait_tics );


#endif
