// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_game.h 1727 2025-02-07 05:03:05Z wesleyjohnson $
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
// $Log: g_game.h,v $
// Revision 1.13  2004/07/27 08:19:35  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.12  2003/03/22 22:35:59  hurdler
//
// Revision 1.11  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.10  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.9  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.8  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.7  2000/11/26 20:36:14  hurdler
// Adding autorun2
//
// Revision 1.6  2000/11/02 19:49:35  bpereira
// Revision 1.5  2000/10/21 08:43:29  bpereira
// Revision 1.4  2000/10/01 10:18:17  bpereira
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
//   
// 
//-----------------------------------------------------------------------------


#ifndef G_GAME_H
#define G_GAME_H

#include "doomdef.h"
  // SAVEGAMEDIR, DOGS, MAPTHING_ADJUST
#include "doomtype.h"
  // PACKED_ATTR
#include "doomstat.h"
#include "d_event.h"

// ---- Teams

// [WDJ] cannot write to const team_names
// Created basic team record
typedef struct {
    char * name;  // always allocated string
} PACKED_ATTR  team_info_t;

extern team_info_t*  team_info[MAXTEAMS];
extern byte          num_teams;  // limited to MAXTEAMS (32)

team_info_t*  get_team( int team_num );
void  set_team_name( int team_num, const char * str );
char * get_team_name( int team_num );

extern  player_t  players[MAXPLAYERS];
extern  byte      playeringame[MAXPLAYERS];

typedef enum {
  PS_unused,
// active players
  PS_player,
  PS_player_from_server,  // server told us
  PS_player_from_savegame,  // savegame told us
  PS_bot,
// not a player, yet
  PS_join_wait_game_start,  // join waits until game start
  PS_added,
  PS_added_commit,     // add player issued
} player_state_e;
extern byte       player_state[MAXPLAYERS];

//added:11-02-98: yeah now you can change it!
// changed to 2d array 19990220 by Kin
extern char       player_names[MAXPLAYERS][MAXPLAYERNAME];
extern byte       num_game_players;  // number of actual players
extern byte       max_num_players;   // dependent upon demo



extern  char      game_map_filename[MAX_WADPATH];
extern  boolean   nomonsters;   // checkparm of -nomonsters

extern  boolean   gameplay_msg;  // True during gameplay

// --- Event actions

typedef enum
{
    ga_nothing,
    ga_completed,
    ga_worlddone,
    ga_playdemo,
    //HeXen
/*
    ga_initnew,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_leavemap,
    ga_singlereborn
*/
} gameaction_e;

extern  gameaction_e    gameaction;

// ======================================
// DEMO playback/recording related stuff.
// ======================================

// demoplaying back and demo recording
extern  boolean   demoplayback;
extern  boolean   demorecording;
extern  boolean   timingdemo;       

// Quit after playing a demo from cmdline.
extern  boolean   singledemo;

// gametic at level start
extern  tic_t     levelstarttic;  

extern consvar_t  cv_showmessages;
extern consvar_t  cv_pickupflash;
extern consvar_t  cv_oof_2s;         // Boom 2s line
extern consvar_t  cv_weapon_recoil;  // Boom weapon recoil
extern consvar_t  cv_fastmonsters;
extern consvar_t  cv_predictingmonsters;  //added by AC for predmonsters
#ifdef MAPTHING_ADJUST
# ifdef MAPTHING_ADJUST_MASTER
extern consvar_t  cv_mapthing_adjust_master;
# endif
extern consvar_t  cv_monster_health;
extern consvar_t  cv_health_pickup;
extern consvar_t  cv_armor_pickup;
extern consvar_t  cv_ammo_pickup;
#endif
#ifdef ENABLE_TIRED_RUN
extern consvar_t cv_tired_run;
extern consvar_t cv_drown;
#endif
#ifdef MONSTER_VARY
extern  consvar_t cv_monster_vary;
extern  consvar_t cv_vary_percent;
extern  consvar_t cv_vary_size;
#endif
#ifdef ENABLE_TELE_CONTROL
extern  consvar_t cv_tele_control;
#endif
#ifdef ENABLE_SLOW_REACT
extern  consvar_t cv_slow_react;
#endif

extern consvar_t  cv_allowjump;
extern consvar_t  cv_allowrocketjump;
extern consvar_t  cv_allowautoaim;
extern consvar_t  cv_allowmlook;
extern consvar_t  cv_allowturbo ;
extern consvar_t  cv_allowexitlevel;



void Command_Turbo_f (void);

// build an internal map name ExMx MAPxx from episode,map numbers
char* G_BuildMapName (int episode, int map);
void G_BuildTiccmd (ticcmd_t* cmd, int realtics, byte pind);

//added:22-02-98: clip the console player aiming to the view
angle_t G_ClipAimingPitch(angle_t aiming);

// [0]=main player [1]=splitscreen player
extern angle_t localangle[2];
extern angle_t localaiming[2]; // should be a angle_t but signed

extern int extramovefactor;		// Extra speed to move at


// ---- Player Spawn
void G_DoReborn (int playernum);
boolean G_DeathMatchSpawnPlayer (int playernum);
void G_CoopSpawnPlayer (int playernum);
void G_PlayerReborn (int player);

void G_AddPlayer( int playernum );

#ifdef DOGS
extern byte  extra_dog_count;
boolean  G_SpawnExtraDog( mapthing_t * spot );
void  G_KillDog( mobj_t * mo );
#endif
#ifdef ENABLE_COME_HERE
extern byte   come_here;  // timer
extern mobj_t * come_here_player;  // who said come-here
#endif

// ---- Game load

void G_InitNew (skill_e skill, const char* mapname, boolean resetplayer);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (skill_e skill, const char* mapname, boolean StartSplitScreenGame);
void G_DoLoadLevel (boolean resetplayer);

void G_DeferedPlayDemo (const char* demo);

// [WDJ] Set the gamemode, and all EN_ that are dependent upon it.
void G_set_gamemode( byte new_gamemode );
boolean G_Downgrade(int version);
void G_setup_VERSION( void );

#ifdef GAMESKILL_FLAGS
void G_setup_gameskill( void );

typedef enum {
    GSF_FAST_MONSTERS = 0x0001,
    GSF_SPAWN_MULTI   = 0x0002,
    GSF_NO_PAIN       = 0x0004,
    GSF_EASY_BRAIN_BOSS = 0x0008,
    GSF_INSTANT_REACTION = 0x0010,
    GSF_AUTO_USE_HEALTH = 0x0020, // heretic, hexen
} gameskill_flags_e;

extern uint16_t gameskill_flags;
#endif


// --- Save Games

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
#ifdef SAVEGAMEDIR
//void G_Load_Game ( const char * savegamedir, int slot );
#endif
void G_Load_Game (int slot);
void G_DoLoadGame (int slot);

// Called by M_Responder.
void G_Save_Game  (int slot, const char* description);
void G_DoSaveGame(int slot, const char* description);

extern char savegamename[MAX_WADPATH];

void G_Savegame_Name( /*OUT*/ char * namebuf, /*IN*/ int slot );

void CheckSaveGame(size_t size);

// --- Demo
// Only called by startup code.
void G_RecordDemo (const char* name);

void G_BeginRecording (void);

void G_DoPlayDemo (const char *defdemoname);
void G_TimeDemo (const char* name);
void G_DoneLevelLoad(void);
void G_StopDemo(void);
boolean G_CheckDemoStatus (void);

// --- Level Func
void G_ExitLevel (void);
void G_SecretExitLevel (void);
void G_NextLevel (void);
void G_DoCompleted( void );
void G_Start_Intermission( void );

void G_Ticker (void);
boolean G_Responder (event_t*   ev);



// [WDJ] 8/2011 Par times can now be modified.
extern int pars[4][10];
extern int cpars[32];

#endif
