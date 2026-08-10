// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: st_stuff.h 1422 2019-01-29 08:05:39Z wesleyjohnson $
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
// $Log: st_stuff.h,v $
// Revision 1.5  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.4  2000/10/01 10:18:19  bpereira
// Revision 1.3  2000/08/31 14:30:56  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Status bar code.
//      Does the face/direction indicator animatin.
//      Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

#ifndef ST_STUFF_H
#define ST_STUFF_H

#include "doomtype.h"
#include "d_event.h"
#include "d_player.h"

// Vars used by Doom and Heretic status display.
extern player_t *  st_plyr;  // Single Player status display
extern boolean  stbar_on;  // status bar active
extern boolean  st_overlay_on;   // status overlay enable

extern byte     st_card;  // key card state displayed  (Doom, Heretic)

//software mode : position according to resolution, not scaled
//hardware mode : original coords, scaled to current resolution, correct aspect
extern int stbar_fg;  // FG with status bar draw flags
extern int stbar_x, stbar_y;  // status bar position
extern int stbar_scalex, stbar_scaley;  // status bar scaling

// Need this for SCR_Recalc() coz widgets coords change with resolutions.
extern boolean  stbar_recalc;

extern int st_palette;  // Doom and Heretic
// pickupflash shifts
extern byte pickupflash_table[ 4 ];

// update all global position variables (just above)
void ST_CalcPos(void);

//
// STATUS BAR
//

// non-number out-of-band signal for int.  (Doom used 1994)
#define  NON_NUMBER   0x7FFF

// Called by main loop.
boolean ST_Responder (event_t* ev);

// Called by main loop.
void ST_Ticker (void);

// Called by main loop.
void ST_Drawer (boolean refresh);

// Called when the console player is spawned on each level.
void ST_Start (void);

// Called by startup code.
void ST_Init (void);

// Called by G_Responder() when pressing F12 while viewing a demo.
void ST_Change_DemoView (void);

// Add status bar related commands & vars
void ST_Register_Commands (void);

// force redraw
void ST_Invalidate(void);

// Set status palette for player.
void ST_doPaletteStuff( player_t * plyr );
// Set status palette 0 for camera.
void ST_Palette0( void );

// States for status bar code.
typedef enum
{
    AutomapState,
    FirstPersonState

} st_stateenum_t;


// States for the chat code.
typedef enum
{
    StartChatState,
    WaitDestState,
    GetChatState

} st_chatstateenum_t;


boolean ST_Responder(event_t* ev);

// face load/unload graphics, called when skin changes
void ST_Load_FaceGraphics (const char *facestr);
void ST_Release_FaceGraphics (void);

// return if player a is in the same team of the player b
boolean ST_SameTeam(player_t *a,player_t *b);

void ST_Load_Graphics(void);
void ST_Release_Graphics(void);

// get the frags of the player
// only one function for calculation : more simple code
int  ST_PlayerFrags (int playernum);


// Heretic status bar
void SB_Heretic_Load_Graphics(void);
void SB_Heretic_Release_Graphics(void);
void SB_Heretic_Ticker(void);
void SB_Heretic_Drawer( boolean refresh );

// Flash for Heretic status
void H_PaletteFlash( player_t * plyr );

#endif
