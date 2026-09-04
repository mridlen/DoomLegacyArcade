// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_menu.h 1422 2019-01-29 08:05:39Z wesleyjohnson $
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
// $Log: m_menu.h,v $
// Revision 1.4  2000/10/08 13:30:01  bpereira
//
// Revision 1.3  2000/04/07 23:11:17  metzgermeister
// added mouse move
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Menu widget stuff, episode selection and such.
//    
//-----------------------------------------------------------------------------

#ifndef M_MENU_H
#define M_MENU_H

#include "doomtype.h"
#include "d_event.h"
  // event_t
#include "command.h"


//
// MENUS
//
// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
boolean M_Responder (event_t *ev);


// Called by main loop,
// only used for menu (skull cursor) animation.
void M_Ticker (void);

// Called by main loop,
// draws the menus directly into the screen buffer.
void M_Drawer (void);

// Called by D_DoomMain,
// loads the config file.
void M_Init (void);
// configures according to gamemode
void M_Configure (void);

// [Arcade] Restart the program; does not return.
//   game_idstr : -game short name, or NULL to keep the current game
//   keep_packs : re-add the loaded level packs with -file, else drop them
//   want_devmode : whether the new session gets -devmode; pass the current
//                  devmode to leave the session's mode alone
void M_Restart_Program( const char * game_idstr, boolean keep_packs, boolean want_devmode );
// [Arcade] Operator hotkey (the gc_devmode control): restarts into or out of
// -devmode.  Attract screen only; a refused press is left for the other
// responders.  Call it after M_Responder and CON_Responder, so that the menu
// can still capture the key when the operator is re-assigning it.
boolean  M_Devmode_Hotkey( event_t * ev );
// [Arcade] True once a level pack has been loaded, after which the attract
// screen demos play against the wrong maps.
boolean  M_LevelPack_Loaded( void );
// [Arcade] Name of the loaded level pack, or NULL. Only one loads at a time.
const char * M_LevelPack_LoadedName( void );

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.
void M_StartControlPanel (void);

// [Arcade] Single Level mode finished its one map; return to its menu.
void  M_SingleLevel_Finished(void);

// Close all open menus.  callexitmenufunc runs the menu's quitroutine.
void  M_Clear_Menus(boolean callexitmenufunc);


// Draws a box with a texture inside as background for messages
void M_DrawTextBox (int x, int y, int width, int lines);
// show or hide the setup for player 2 (called at splitscreen change)
void M_Player2_MenuEnable( boolean player2_enable );

// the function to show a message box typing with the string inside
// string must be static (not in the stack)
// routine is a function taking a int in parameter
typedef enum 
{
    MM_NOTHING = 0,     // is just displayed until the user do someting
    MM_YESNO,           // routine is called with only 'y' or 'n' in param
    MM_EVENTHANDLER     // the same of above but without 'y' or 'n' restriction
                        // and routine is void routine(event_t *) (ex: set control)
} menumessagetype_t;

void M_StartMessage ( const char*       string,
                      void*             routine,
                      menumessagetype_t itemtype );

// M_StartMessage with NULL routine and MM_NOTHING
void M_SimpleMessage ( const char*       string );

// Called by linux_x/i_video_xshm.c
void M_QuitResponse(int ch);

void M_Register_Menu_Controls( void );

// [Arcade] How many sets of controls this cabinet has, 1..4.  Read by
// d_clisrv.c to decide how many players join on this machine.  See
// D_NumLocalPlayers(), which clamps it to MAXSPLITSCREENPLAYERS.
extern consvar_t cv_localplayers;
// [Arcade] Two players side by side instead of stacked.  Read by d_clisrv.c
// (D_View_Grid), which is where every placement decision comes from.
extern consvar_t cv_splitvertical;
extern consvar_t cv_jointime;       // [Arcade] join screen countdown, seconds
extern consvar_t cv_initialstimeout;// [Arcade] initials entry timeout, seconds

// [Arcade] The player's chosen fast monsters / monster respawn, as edited on
// the Game Options page.  Split from the engine cvars of the same names,
// which the game-start command line and sk_nightmare both overwrite; see the
// comment beside their definitions in m_menu.c.  G_DeferedInitNew seeds the
// engine pair from these at every menu game start, and hs_ranked_rules[]
// checks *these* so that Nightmare's own forcing does not void a run.
extern consvar_t cv_fastmonsters_menu;
extern consvar_t cv_respawnmonsters_menu;

// [Arcade] True while the initials entry page is up.  The idle timeout asks,
// so a player part way through entering their initials is not closed out
// from under them; the page runs its own (operator set) countdown instead.
boolean  M_Initials_Active( void );

#ifdef LAUNCHER
void M_LaunchMenu( void );
#endif

#endif
