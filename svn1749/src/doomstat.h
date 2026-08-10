// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: doomstat.h 1674 2024-03-03 04:41:20Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2010 by DooM Legacy Team.
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
// $Log: doomstat.h,v $
// Revision 1.14  2003/07/23 17:20:37  darkwolf95
// Initial Chex Quest 1 Support
//
// Revision 1.13  2002/12/13 22:34:27  ssntails
// MP3/OGG support!
//
// Revision 1.12  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.11  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.10  2001/04/01 17:35:06  bpereira
// Revision 1.9  2001/02/24 13:35:19  bpereira
//
// Revision 1.8  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.7  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.6  2000/10/21 08:43:28  bpereira
// Revision 1.5  2000/08/31 14:30:55  bpereira
// Revision 1.4  2000/08/10 19:58:04  bpereira
//
// Revision 1.3  2000/08/10 14:53:10  ydario
// OS/2 port
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------

#ifndef D_STATE_H
#define D_STATE_H

#include "doomdef.h"
  // CLIENTPREDICTION, LOGMESSAGES, DEBUGFILE

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"

// We need the player data structure as well.
#include "d_player.h"
#include "d_clisrv.h"

#ifdef ENABLE_UMAPINFO
// [MB] 2023-01-22: Support for UMAPINFO added
#include "umapinfo.h"
#endif

// Game mode handling - identify IWAD version,
//  handle IWAD dependend animations etc.
// [WDJ] modifed names to be self readable
typedef enum
{
    doom_shareware,    // DOOM 1 shareware, E1, M9
    doom_registered,   // DOOM 1 registered, E3, M27
    doom2_commercial,  // DOOM 2 retail, E1 M34
    // FreeDoom is DOOM 2, can play as commercial or indetermined
    // DOOM 2 german edition not handled
    ultdoom_retail,    // DOOM 1 retail, E4, M36
    heretic,
    hexen,
    strife,
    indetermined, // Well, no IWAD found.
    chexquest1	  // DarkWolf95:July 14, 2003: Chex Quest Support

} gamemode_e;

// Set from gamemode.
extern byte  EN_heretic_hexen;  // common features
extern byte  EN_heretic;
extern byte  EN_hexen;
extern byte  EN_strife;
extern byte  EN_doom_etc;  // doom, boom, mbf, common behavior  (not heretic, hexen, strife)
// Set by gamemode, but may be enabled by demos too.
extern byte  EN_boom;  // Boom features (boom demo compatibility=0)
extern byte  EN_mbf;   // MBF (Marines Best Friend) enable (similar prboom mbf_features)
#ifdef MBF21
extern byte  EN_mbf21; // MBF21 extension, as defined in DSDA-Doom.
#endif
extern byte  EV_legacy; // DoomLegacy version, 0 when some other demo.


#if 0
// [WDJ] 8/26/2011 Replaced by gamedesc_id, GDESC_
// Mission packs - might be useful for TC stuff?
typedef enum
{
    doom,         // DOOM 1
    doom2,        // DOOM 2
    pack_tnt,     // TNT mission pack
    pack_plut,    // Plutonia pack
    mission_none

} gamemission_t;

extern gamemission_t   gamemission;
#endif



// [WDJ] Structure some of the scattered game differences.

enum gameflags_e {
   GD_idwad       = 0x01, // one of the commercial/shareware wads by id or Raven
   GD_iwad_pref   = 0x02, // load the iwad after legacy.wad to give it preference
   GD_unsupported = 0x08, // unsupported game type
};

// Id of specific iwad, no longer tied to table index
typedef enum {
    GDESC_freedoom,
    GDESC_freedm,
    GDESC_doom2,
    GDESC_freedoom_ultimate,
    GDESC_ultimate,
    GDESC_doom,
    GDESC_doom_shareware,
    GDESC_plutonia,
    GDESC_tnt,
    GDESC_blasphemer,
    GDESC_heretic,
    GDESC_heretic_shareware,
    GDESC_hexen,
    GDESC_hexen_demo,
    GDESC_strife,
    GDESC_strife_shareware,
    GDESC_chex1,
    GDESC_ultimate_mode,
    GDESC_doom_mode,
    GDESC_heretic_mode,
    GDESC_hexen_mode,
    GDESC_other, // other iwad entry, and table search limit
} game_desc_e;


typedef struct
{
    const char * gname;	       // game name, used in savegame
    const char * startup_title; // startup page
    const char * idstr;	       // used for directory and command line
    const char * iwad_filename[3]; // possible filenames
                               // doom, doom2, heretic, heretic1, hexen, etc.
    const char * support_wad;   // another wad to support the game
    const char * keylump[2];   // required lump names
    byte	require_lump;  // lumps that must appear (bit set)
    byte	reject_lump;   // lumps that must not appear (bit set)
    uint16_t	gameflags;     // assorted flags from gameflags_e
    game_desc_e gamedesc_id;   // independent of table index, safer
    gamemode_e	gamemode;
} game_desc_t;

#ifdef LAUNCHER
// game desc for Launcher
game_desc_t *  D_GameDesc( int i );
#endif


// ===================================================
// Game Mode - identify IWAD as shareware, retail etc.
// ===================================================
//
extern game_desc_e     gamedesc_id; // unique game id
extern game_desc_t     gamedesc;    // active desc used by most of legacy
extern gamemode_e      gamemode;
extern boolean         have_inventory;   // true with heretic and hexen
extern boolean         raven_heretic_hexen;  // true with heretic and hexen

// Set if homebrew PWAD stuff has been added.
extern  boolean	       modifiedgame;


// =========
// Language.
// =========
//

// Identify language to use, software localization.
typedef enum
{
    english,
    french,
    german,
    lang_unknown

} language_t;

extern  language_t   language;



// =============================
// Selected skill type, map etc.
// =============================

// skill levels
typedef enum
{
    sk_baby,
    sk_easy,
    sk_medium,
    sk_hard,
    sk_nightmare
} skill_e;

// Selected by user.
extern  skill_e         gameskill;	// easy, medium, hard
extern  byte            gameepisode;	// Doom episode, 1..4
extern  byte            gamemap;	// level 1..32

#ifdef ENABLE_UMAPINFO
// [MB] 2023-01-22: Support for UMAPINFO added
// If this pointer is NULL, no additional UMAPINFO data is available
// Otherwise it points to a UMAPINFO map entry structure
extern  mapentry_t *    game_umapinfo;
#endif

// Nightmare mode flag, single player.
// extern  boolean         respawnmonsters;

// Netgame? only true in a netgame
extern  boolean         netgame;
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern  boolean         multiplayer;

// Now an enum to handle altdeath/cooperative better.
extern  consvar_t       cv_deathmatch;  // deathmatch and coop
extern  byte            deathmatch;  // deathmatch only
extern  byte            weapon_persist; // deathmatch weapon pickup multiple times



// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean         nomusic; // defined in d_main.c
extern boolean         nosoundfx; // had clash with WATCOM i86.h nosound() function

// =========================
// Status flags for refresh.
// =========================
//

extern  byte    paused;         // Game Pause, multiple bits

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  boolean statusbaractive;
extern  boolean menuactive;     // Menu overlayed?

extern  boolean nodrawers;
extern  boolean noblit;

extern  int     view_window_x;
extern  int     view_window_y;    // splitscreen window position
extern  int     rdraw_viewheight;		// was viewheight
extern  int     rdraw_viewwidth;		// was viewwidth
extern  int     rdraw_scaledviewwidth;		// was scaledrviewwidth



// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int     viewangleoffset;

// Player taking events, and displaying.
extern  int     consoleplayer;
extern  int     displayplayer;
extern  int     displayplayer2; // for splitscreen

// [WDJ] Simplify every test against a player ptr, and splitscreen
extern  player_t * consoleplayer_ptr;
extern  player_t * displayplayer_ptr;
extern  player_t * displayplayer2_ptr;  // NULL when not in use

//added:16-01-98: player from which the statusbar displays the infos.
extern  int     statusbarplayer;


// ============================================
// Statistics on a given map, for intermission.
// ============================================
//
extern  int     totalkills;
extern  int     totalitems;
extern  int     totalsecret;


// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern  tic_t           gametic;
extern  tic_t           game_comp_tic;
extern  tic_t           leveltime;

#ifdef CLIENTPREDICTION2
extern  tic_t           localgametic;
extern  boolean         spirit_update;
#else
#define localgametic  leveltime
#endif

// Player spawn spots.
extern  mapthing_t *  playerstarts[MAXPLAYERS];

// Intermission stats.
// Parameters for world map / intermission.
extern  wb_start_t      wminfo;


#if 0
// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern  uint16_t        maxammo[NUMAMMO];
#endif


// =====================================
// Internal parameters, used for engine.
// =====================================
//

// The current state of the game
typedef enum
{
    GS_NULL = 0,                // at begin
    GS_LEVEL,                   // we are playing
    GS_INTERMISSION,            // gazing at the intermission screen
    GS_FINALE,                  // game final animation
    GS_DEMOSCREEN,              // looking at a demo
    //legacy
    GS_DEDICATEDSERVER,         // added 27-4-98 : new state for dedicated server
    GS_WAITINGPLAYERS,          // added 3-9-98 : waiting player in net game
    // Non-state, out-of-band flags
    GS_FORCEWIPE                // wipegamestate only
} gamestate_e;

extern  gamestate_e     gamestate;

// gamestate_e is unsigned

// wipegamestate can be set to GS_FORCEWIPE
//  to force a wipe on the next draw
extern  gamestate_e     wipegamestate;

// if true, load all graphics at level load
extern  boolean         precache;

//?
// debug flag to cancel adaptiveness
extern  boolean         singletics;



#define   BODYQUESIZE     32

extern mobj_t*   bodyque[BODYQUESIZE];
extern  int             bodyqueslot;


// =============
// Netgame stuff
// =============


//extern  ticcmd_t        localcmds[BACKUPTICS];

extern  ticcmd_t        netcmds[BACKUPTICS][MAXPLAYERS];
// Collect stats for netstat.
extern int   stat_tic_moved, stat_tic_miss;

#endif //__D_STATE__
