// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: g_game.c 1745 2025-04-10 09:36:34Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: g_game.c,v $
// Revision 1.50  2004/09/12 19:40:05  darkwolf95
// additional chex quest 1 support
//
// Revision 1.49  2004/07/27 08:19:35  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.48  2003/11/22 00:49:33  darkwolf95
//
// Revision 1.47  2003/11/22 00:22:09  darkwolf95
// get rid of FS hud pics on level exit and new game, also added exl's fix for clearing hub variables on new game
//
// Revision 1.46  2003/03/22 22:35:59  hurdler
//
// Revision 1.45  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.44  2002/08/24 22:42:02  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.43  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.42  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.41  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.40  2001/08/20 18:34:18  bpereira
// glide ligthing and map30 bug
//
// Revision 1.39  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.38  2001/08/02 19:15:59  bpereira
// fix player reset in secret level of doom2
//
// Revision 1.37  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.36  2001/05/16 21:21:14  bpereira
// Revision 1.35  2001/05/03 21:22:25  hurdler
//
// Revision 1.34  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.33  2001/04/01 17:35:06  bpereira
// Revision 1.32  2001/03/03 06:17:33  bpereira
// Revision 1.31  2001/02/24 13:35:19  bpereira
// Revision 1.30  2001/02/10 12:27:13  bpereira
//
// Revision 1.29  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.28  2000/11/26 20:36:14  hurdler
// Adding autorun2
//
// Revision 1.27  2000/11/11 13:59:45  bpereira
// Revision 1.26  2000/11/06 20:52:15  bpereira
// Revision 1.25  2000/11/04 16:23:42  bpereira
// Revision 1.24  2000/11/02 19:49:35  bpereira
//
// Revision 1.23  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.22  2000/10/21 08:43:28  bpereira
// Revision 1.21  2000/10/09 14:03:31  crashrl
// Revision 1.20  2000/10/08 13:30:00  bpereira
//
// Revision 1.19  2000/10/07 20:36:13  crashrl
// Added deathmatch team-start-sectors via sector/line-tag and linedef-type 1000-1031
//
// Revision 1.18  2000/10/01 10:18:17  bpereira
// Revision 1.17  2000/09/28 20:57:14  bpereira
// Revision 1.16  2000/08/31 14:30:55  bpereira
// Revision 1.15  2000/08/10 14:08:48  hurdler
// Revision 1.14  2000/04/30 10:30:10  bpereira
// Revision 1.13  2000/04/23 16:19:52  bpereira
//
// Revision 1.12  2000/04/19 10:56:51  hurdler
// commited for exe release and tag only
//
// Revision 1.11  2000/04/16 18:38:07  bpereira
//
// Revision 1.10  2000/04/11 19:07:23  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.9  2000/04/07 23:11:17  metzgermeister
// added mouse move
//
// Revision 1.8  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.7  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.6  2000/03/29 19:39:48  bpereira
//
// Revision 1.5  2000/03/23 22:54:00  metzgermeister
// added support for HOME/.legacy under Linux
//
// Revision 1.4  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.3  2000/02/27 00:42:10  hurdler
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      game loop functions, events handling
//
//-----------------------------------------------------------------------------


// [WDJ] To show the demo version on the console
#define SHOW_DEMOVERSION   
#define DEBUG_DEMO
// Stick with one demo version because of other ports, and have separate
// fields record DoomLegacy specific version and enables.
// This only changes the demo header, not the content.
// Writes demoversion 143 and up, and can read any DoomLegacy demo.
// Older DoomLegacy demos are demo versions 111..143.


#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "console.h"
#include "dstrings.h"

#include "d_main.h"
#include "d_net.h"
#include "d_netcmd.h"
#include "f_finale.h"
#include "p_setup.h"
#include "p_saveg.h"

#include "i_system.h"

#include "wi_stuff.h"
#include "am_map.h"
#include "m_random.h"
#include "p_local.h"
#include "p_tick.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_things.h"

#include "s_sound.h"

#include "g_game.h"
#include "g_input.h"
#include "hs_stuff.h"

//added:16-01-98:quick hack test of rocket trails
#include "p_fab.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_argv.h"

#include "hu_stuff.h"

#include "st_stuff.h"

#include "keys.h"
#include "i_joy.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_video.h"
#include "p_inter.h"
#include "p_info.h"
#include "byteptr.h"
#include "t_script.h"

#include "b_game.h"	//added by AC for acbot




static void G_ReadDemoTiccmd (ticcmd_t* cmd,int playernum);
static void G_WriteDemoTiccmd (ticcmd_t* cmd,int playernum);
static void G_DoWorldDone (void);


// demoversion the 'dynamic' version number, this should be == game VERSION
// when playing back demos, 'demoversion' receives the version number of the
// demo. At each change to the game play, demoversion is compared to
// the game version, if it's older, the changes are not done, and the older
// code is used for compatibility.
//
byte            demoversion;  // engine behavior version
uint16_t        demoversion_rev;  // demoversion and revision

// Determined by menu selection, or demo.
skill_e         gameskill;
byte            gameepisode;  // current game episode number  1..4
byte            gamemap;      // current game map number 1..31
char            game_map_filename[MAX_WADPATH];      // an external wad filename

#ifdef ENABLE_UMAPINFO
// Root for umapinfo map data.
mapentry_t *    game_umapinfo;  // [MB] 2023-01-22: Support for UMAPINFO added
#endif



// Determined by gamemode and wad.
gamemode_e  gamemode = indetermined;   // Game Mode - identify IWAD as shareware, retail etc.

// [WDJ] Enables for fast (test for zero) feature tests in the engine.
// Byte is efficient and fast to test for 0/1, int is not.
// EN_xxx are enables, value = 0/1.
// EV_xxx are enum,    when value = 0..255, use a byte for efficiency.

// These are set from gamemode.  Still use gamemode in the main setup.
// Set by gamemode, but may be enabled by demos too.
byte  EN_doom_etc;  // doom, boom, mbf, common behavior  (not heretic, hexen, strife)
byte  EN_boom;  // Boom features (boom demo compatibility=0)
byte  EN_mbf;   // MBF (Marines Best Friend) enable (similar prboom mbf_features)
#ifdef MBF21
byte  EN_mbf21; // MBF21 extension, as defined in DSDA-Doom.
#endif
byte  EV_legacy; // DoomLegacy version, 0 when some other demo.

// Raven: Heretic, Hexen, and Strife may be Raven, but code reader
// should not need to know that.  Keep names explicit for easy code reading.
byte  EN_heretic_hexen;  // common features
byte  EN_heretic;
byte  EN_hexen;
byte  EN_strife;

// Secondary features.
// [WDJ] Prevent demo from altering user game settings.
// When a cv_ value range is less than 256, test the EV field (ON/OFF/ENUM).
// Demo settings may change the cv_xxx.EV fields or EN_xxx variables.
// The cv user settings will be restored after the demo by
// CV_Restore_User_Settings (setting .EV from .value).
// Derive EN_ enables for special code logic.  Need to be set in DemoAdapt
// so they are set properly for games and demos.
// Boom
byte  EN_variable_friction;  // Boom demo flag, Heretic, and Legacy.
byte  EN_pushers;
byte  EN_skull_bounce_fix;  // !comp[comp_soul]
byte  EN_skull_bounce_floor; // PrBoom has this enabled by comp level.
byte  EN_boom_physics; // !comp[comp_model]
byte  EN_blazing_double_sound; // comp[comp_blazing]
byte  EN_vile_revive_bug; // comp[comp_vile]
byte  EN_sleeping_sarg_bug;  // fixed PrBoom 4, no comp
byte  EN_doorlight; // !comp[comp_doorlight]
byte  EN_invul_god; // !comp[comp_god]
byte  EN_boom_floor; // !comp[comp_floors]
byte  EN_doom_movestep_bug; // comp[comp_moveblock]
// MBF  (1998-2000)
byte  EN_mbf_pursuit;   // !comp[comp_pursuit]
byte  EN_mbf_telefrag;  // !comp[comp_telefrag]
fixed_t EV_mbf_distfriend;
#ifdef MBF21
// MBF21
byte  EN_ledgeblock;  // comp[comp_ledgeblock]
byte  EN_friendlyspawn; // comp[comp_friendlyspawn]
byte  EN_voodooscroller;  // comp[comp_voodooscroller]
byte  EN_reserved_line_flag;  // comp[comp_reservedlineflag]
byte  EN_mbf21_action;
#endif
// Heretic, Hexen
byte  EN_inventory;

#ifdef DOGS
byte  extra_dog_count = 0;
static   uint16_t  extra_dog_respawn = 0;  // save on extra tests
#define  EXTRA_DOG_RESPAWN_TIME   (5 * TICRATE)
#endif
#ifdef ENABLE_COME_HERE
byte   come_here = 0;  // timer
mobj_t * come_here_player = NULL;  // who said come-here
#endif

// Demo playback enables
static char * playdemo_name = NULL;  // malloc
static byte  EN_demotic_109;  // old demo tic format
static byte  EN_boom_longtics;  // 16 bit boom angle in demo tic



// [WDJ] PrBoom compatibility enum, to read Boom demo.
enum {
  comp_telefrag,
  comp_dropoff,
  comp_vile,
  comp_pain,
  comp_skull,
  comp_blazing,
  comp_doorlight,
  comp_model,
  comp_god,
  comp_falloff,
  comp_floors,
  comp_skymap,
  comp_pursuit,
  comp_doorstuck,
  comp_staylift,
  comp_zombie,
  comp_stairs,
  comp_infcheat,
  comp_zerotags,
  comp_moveblock,
  comp_respawn,
  comp_sound,
  comp_666,
  comp_soul,
  comp_maskedanim,

  // [WDJ] Info from DSDA-Doom (e6y)
  comp_ouchface,
  comp_maxhealth,
  comp_translucency,

#ifdef MBF21
  // [WDJ] Info from DSDA-Doom
  comp_ledgeblock,
  comp_friendlyspawn,
  comp_voodooscroller,
  comp_reservedlineflag,
#endif

  COMP_NUM,
  COMP_TOTAL=32
};


// -------------------------------------------
// Boom and MBF compatibility flags
//   cv or EN : DoomLegacy handling of the demo flags.
// demo_compatibility  === (compatibility_level < boom_compatibility_compatibility)
//   EN_boom =  (compatibility_level >= boom_compatibility_compatibility)
// compatibility === (compatibility_level <= boom_compatibility_compatibility)
// mbf_features === (compatibility_level >= mbf_compatibility)
//   EN_mbf = (compatibility_level >= mbf_compatibility)
// compatibility flag in demo - turn off Boom features
//   EN_boom = ! compatibility // but is demoversion specific
// comp_telefrag - monsters used to telefrag only on MAP30,
//   now they do it for spawners only.
//   EN_telefrag = ! comp[comp_telefrag] // demo support only
// comp_dropoff - MBF encourages things to drop off of overhangs
//   cv_mbf_dropoff = ! comp[comp_dropoff]
// comp_vile - original Doom archville bugs like ghosts
//   EN_vile_revive_bug = comp[comp_vile]
// comp_pain - original Doom limits Pain Elementals from spawning too many skulls
//   EN_skull_limit = comp[comp_pain]
// comp_skull - original Doom let skulls be spit through walls by Pain Elementals
//   EN_old_pain_spawn = comp[comp_skull]
// comp_blazing - original Doom duplicated blazing door sound
//   EN_blazing_double_sound = comp[comp_blazing]
// comp_doorlight - MBF made door lighting changes more gradual
//   EN_doorlight = ! comp[comp_doorlight]
// comp_model - improvements to the game physics
//   EN_boom_physics = ! comp[comp_model]
// comp_god - fixes to God mode
// comp_falloff - MBF encourages things to drop off of overhangs
//   cv_mbf_falloff = ! comp[comp_falloff]
// comp_floors - fixes for moving floors bugs
// comp_skymap - original Doom invul skymap colormap
// comp_pursuit - MBF AI change, limited pursuit?
//   cv_mbf_pursuit = ! comp[comp_pursuit]
// comp_doorstuck - monsters stuck in doors fix
//   cv_mbf_doorstuck = ! comp[comp_doorstuck]
// comp_staylift - MBF AI change, monsters try to stay on lifts
//   cv_mbf_staylift = ! comp[comp_staylift]
// comp_zombie - prevent dead players triggering stuff
// comp_stairs - see p_floor.c
// comp_infcheat - FIXME
// comp_zerotags - allow zero tags in wads
// comp_moveblock - enables keygrab and mancubi shots going thru walls
//   EN_doom_movestep_bug = comp[comp_moveblock]
// comp_respawn - objects which aren't on the map at game start respawn at (0,0)
//   cph - this is the inverse of comp_respawnfix from eternity.
// comp_sound - see s_sound.c
// comp_666 - enables tag 666 in non-ExM8 levels
// comp_soul - enables lost souls bouncing (see P_ZMovement)
//   EN_skull_bounce_fix = ! comp[comp_soul] // normally on
// comp_maskedanim - 2s mid textures don't animate
// comp_ouchface - Fixed permanently, not optional, does not affect play.
// comp_maxhealth, - limits DEH setting MAXHEALTH, not implemented
// comp_translucency, - not implemented

// [WDJ] MBF21, Info from DSDA-Doom
// comp_ledgeblock - ledges block ground enemy, default ON
// comp_friendlyspawn - A_Spawn inherits friend, default ON
// comp_voodooscroller - voodoo on slow scroller move too slowly
// comp_reservedlineflag - line flag 0x8000 clears extended flags, default ON


#if 0
  static const struct {
    complevel_t fix; // level at which fix/change was introduced
    complevel_t opt; // level at which fix/change was made optional
  } levels[] = {
    // comp_doorstuck - monsters stuck in doors fix
    { boom_202_compatibility, mbf_compatibility },

    // comp_vile - original Doom archville bugs like ghosts
    // comp_pain - original Doom limits Pain Elementals from spawning too many skulls
    // comp_skull - original Doom let skulls be spit through walls by Pain Elementals
    // comp_blazing - original Doom duplicated blazing door sound
    // comp_doorlight - MBF made door lighting changes more gradual
    // comp_model - improvements to the game physics
    // comp_god - fixes to God mode
    // comp_skymap
    // comp_zerotags - allow zero tags in wads */
    { boom_compatibility (==boom_201_compatibility), mbf_compatibility },

    // comp_floors - fixes for moving floors bugs
    // comp_stairs - see p_floor.c
    { boom_compatibility_compatibility (==boom_200_compatibility), mbf_compatibility },
     
    // comp_telefrag - monsters used to telefrag only on MAP30, now they do it for spawners only
    // comp_dropoff - MBF encourages things to drop off of overhangs
    // comp_falloff - MBF encourages things to drop off of overhangs
    // comp_pursuit - MBF AI change, limited pursuit?
    // comp_staylift - MBF AI change, monsters try to stay on lifts
    // comp_infcheat - FIXME
    { mbf_compatibility, mbf_compatibility },
     
    // comp_zombie - prevent dead players triggering stuff
    { lxdoom_1_compatibility, mbf_compatibility },
    // comp_moveblock - enables keygrab and mancubi shots going thru walls
    { lxdoom_1_compatibility, prboom_2_compatibility },
    // comp_respawn - objects which aren't on the map at game start respawn at (0,0)
    { prboom_2_compatibility, prboom_2_compatibility },
    // comp_sound - see s_sound.c
    { boom_compatibility_compatibility (==boom_200_compatibility), prboom_3_compatibility },
    // comp_666 - enables tag 666 in non-ExM8 levels
    { ultdoom_compatibility, prboom_4_compatibility },
    // comp_soul - enables lost souls bouncing (see P_ZMovement)
    { prboom_4_compatibility, prboom_4_compatibility },
    // comp_maskedanim - 2s mid textures don't animate
    { doom_1666_compatibility, prboom_4_compatibility },
#ifdef MBF21
    // [WDJ] From DSDA-Doom
    // comp_ledgeblock - ground monsters are blocked by ledges
    { boom_compatibility, mbf21_compatibility },
    // comp_friendlyspawn - A_Spawn new mobj inherits friendliness
    { prboom_1_compatibility, mbf21_compatibility },
    // comp_voodooscroller - Voodoo dolls on slow scrollers move too slowly
    { mbf21_compatibility, mbf21_compatibility },
    // comp_reservedlineflag - ML_RESERVED clears extended flags
    { mbf21_compatibility, mbf21_compatibility }
#endif
  };
#endif


// Engine state
gamestate_e     gamestate = GS_NULL;
gameaction_e    gameaction;
byte            paused;                 // multiple pause bits

boolean         netgame;                // only true if packets are broadcast
boolean         multiplayer;

// players and bots
byte            max_num_players = 32;      // dependent upon demo
byte            num_game_players = 0;      // number of actual players, from playeringame, incl bots
byte            playeringame[MAXPLAYERS];  // player active
byte            player_state[MAXPLAYERS];  // from where, and pending player
player_t        players[MAXPLAYERS];

// [WDJ] Whenever assign to these must update the _ptr too.
// They are not changed anywhere as often as players[] appears in IF stmts.
int             consoleplayer;          // player taking events and displaying
int             displayplayer;          // view being displayed
int             displayplayer2 = -1;    // for splitscreen, -1 when not in use
int             statusbarplayer;        // player who's statusbar is displayed
                                        // (for spying with F12)

// [WDJ] Simplify every test against a player ptr, and splitscreen
player_t *      consoleplayer_ptr = &players[0];
player_t *      displayplayer_ptr = &players[0];
player_t *      displayplayer2_ptr = NULL;  // NULL when not in use

tic_t           gametic;
tic_t           levelstarttic;          // gametic at level start
tic_t           last_input_tic;         // gametic of last player input event

// [Arcade] The episode's last level owes a finale once its intermission is
// done.  Vanilla skips the intermission entirely on those maps and jumps
// straight to the finale, which cost the cabinet both the summary page and
// the HS_LevelExit call that scores the run -- a time on E?M8 could not be
// recorded at all.  Doom II already does it the other way round for MAP30:
// intermission first, finale started afterwards from G_NextLevel.  Setting
// this in G_Start_Intermission makes the Doom 1 / Heretic / Chex maps behave
// the same, and lets wi_stuff skip the "Entering ..." screen, for which
// there is no next location.  Decided in one place rather than recomputed,
// so the UMAPINFO override above it stays authoritative.
boolean         finale_after_intermission = false;
// [WDJ] Derived from PrBoom basetic.
// A tic that always starts at 0, and only runs while the demo runs.
tic_t           game_comp_tic;  // gametic - basetic


// [WDJ] Keep rarely used state information separate so cache usage is cleaner.

// Support
boolean         precache = true;        // if true, load all graphics at start
boolean         gameplay_msg = false;   // enable game play message control
boolean         modifiedgame;           // Set if homebrew PWAD stuff has been added.
language_t      language = english;     // Language.

// Intermission state
int             totalkills, totalitems, totalsecret;
wb_start_t      wminfo;                 // parms for world map / intermission

// Demo state
#define DEMONAME_LEN  32
char            demoname[DEMONAME_LEN+5];
boolean         demorecording;
// [Arcade] This recording is a scratch buffer to be snapshotted, not a demo
// to keep; do not write it out when recording stops.
boolean         demo_scratch = false;
boolean         demoplayback;
byte*           demobuffer;
byte*           demo_p;
byte*           demoend;
boolean         singledemo;             // quit after playing a demo from cmdline

// Timing demo state
boolean         timingdemo;             // if true, exit with report on completion
boolean         nodrawers;              // for comparative timing purposes
boolean         noblit;                 // for comparative timing purposes
tic_t           demostarttime;          // for comparative timing purposes



void ShowMessage_OnChange(void);
void AllowTurbo_OnChange(void);

CV_PossibleValue_t showmessages_cons_t[]={{0,"Off"},{1,"Minimal"},{2,"Play"},{3,"Verbose"},{4,"Debug"},{5,"Dev"},{0,NULL}};
CV_PossibleValue_t pickupflash_cons_t[]   ={{0,"Off"},{1,"Status"},{2,"Half"},{3,"Vanilla"},{0,NULL}};

// [0]=main player [1]=splitscreen player
// [Arcade] One per local player; entries 3 and 4 are the extra panels.
consvar_t cv_autorun[MAXSPLITSCREENPLAYERS] = {
  {"autorun"     ,"0",CV_SAVE,CV_OnOff},
  {"autorun2"    ,"0",CV_SAVE,CV_OnOff},
  {"autorun3"    ,"0",CV_SAVE,CV_OnOff},
  {"autorun4"    ,"0",CV_SAVE,CV_OnOff}
};
consvar_t cv_alwaysfreelook[MAXSPLITSCREENPLAYERS] = {
  {"alwaysmlook" ,"0",CV_SAVE,CV_OnOff},
  {"alwaysmlook2","0",CV_SAVE,CV_OnOff},
  {"alwaysmlook3","0",CV_SAVE,CV_OnOff},
  {"alwaysmlook4","0",CV_SAVE,CV_OnOff}
};
consvar_t cv_mouse_move[MAXSPLITSCREENPLAYERS] = {
  {"mousemove"   ,"1",CV_SAVE,CV_OnOff},
  {"mousemove2"  ,"1",CV_SAVE,CV_OnOff},
  {"mousemove3"  ,"1",CV_SAVE,CV_OnOff},
  {"mousemove4"  ,"1",CV_SAVE,CV_OnOff}
};

consvar_t cv_mouse_invert     = {"invertmouse" ,"0",CV_SAVE,CV_OnOff};
#ifdef MOUSE2
consvar_t cv_mouse2_invert    = {"invertmouse2","0",CV_SAVE,CV_OnOff};
#endif

CV_PossibleValue_t joy_deadzone_cons_t[]={{0,"MIN"},{20,"INC"},{2000,"MAX"},{0,NULL}};
consvar_t cv_joy_deadzone     = {"joydeadzone" ,"800",CV_SAVE,joy_deadzone_cons_t};

// [Arcade lockdown] Idle-to-title timeout: if the player gives no input for
// this many seconds during gameplay, the game ends and returns to the title
// screen.  0 disables.  Skipped when launched with -devmode or in a netgame.
CV_PossibleValue_t idletimeout_cons_t[]  = { {0,"MIN"}, {3600,"MAX"}, {0,NULL} };
consvar_t cv_idletimeout  = { "idletimeout", "60", CV_SAVE, idletimeout_cons_t };

// Lead time (seconds) before idletimeout fires during which an on-screen
// "Returning to title in Ns..." countdown is shown.
CV_PossibleValue_t idlewarntime_cons_t[] = { {0,"MIN"}, {60,"MAX"}, {0,NULL} };
consvar_t cv_idlewarntime = { "idlewarntime", "15", CV_SAVE, idlewarntime_cons_t };

consvar_t cv_showmessages     = {"showmessages","2",CV_SAVE | CV_CALL | CV_NOINIT,showmessages_cons_t,ShowMessage_OnChange};
consvar_t cv_pickupflash      = {"pickupflash" ,"1",CV_SAVE, pickupflash_cons_t};
consvar_t cv_weapon_recoil    = {"weaponrecoil","0",CV_SAVE | CV_NETVAR, CV_OnOff};  // Boom weapon recoil

consvar_t cv_allowturbo       = {"allowturbo"  ,"0",CV_NETVAR | CV_CALL, CV_YesNo, AllowTurbo_OnChange};
consvar_t cv_allowjump        = {"allowjump"   ,"1",CV_NETVAR,CV_YesNo};
consvar_t cv_allowautoaim     = {"allowautoaim","1",CV_NETVAR,CV_YesNo};
//SoM: 3/28/2000: Working rocket jumping.
consvar_t cv_allowrocketjump  = {"allowrocketjump","0",CV_NETVAR,CV_YesNo};
consvar_t cv_allowmlook       = {"allowmlook"  ,"1",CV_NETVAR,CV_YesNo};
consvar_t cv_allowexitlevel   = {"allowexitlevel", "1", CV_NETVAR, CV_YesNo, NULL };

// oof when hit 2s line (in PrBoom enabled by ! comp_sound)
consvar_t cv_oof_2s = {"oof_2s", "0", CV_SAVE|CV_CALL, CV_OnOff, DemoAdapt_p_map};

#ifdef ENABLE_TIRED_RUN
CV_PossibleValue_t tired_cons_t[]={{0,"Off"},{1,"easy"},{2,"half"},{3,"med"},{4,"heavy"},{0,NULL}};
CV_PossibleValue_t drown_cons_t[]={{0,"Off"},{1,"Legacy"},{2,"Boom"},{0,NULL}};
consvar_t cv_tired_run        = {"tiredrun" ,"1",CV_SAVE | CV_NETVAR, tired_cons_t};
consvar_t cv_drown            = {"drown"    ,"1",CV_SAVE | CV_NETVAR, drown_cons_t};
#endif

#ifdef ENABLE_TELE_CONTROL
CV_PossibleValue_t tele_control_t[]={{0,"Norm"},
  {1,"easy2"},{2,"easy1"},{3,"any"},{4,"hard1"},{5,"hard2"},
  {6,"stun2"},{7,"stun4"},{8,"stun6"},
  {9,"rate2"},{10,"rate4"},{11,"rate6"},
  {12,"scatter1"},{13,"scatter2"},{14,"scatter3"},{15,"scatter4"},
  {16,"dup25"},{17,"dup40"},{18,"dup60"},
  {0,NULL}};
consvar_t cv_tele_control     = {"telecontrol" ,"0",CV_SAVE | CV_NETVAR, tele_control_t};
#endif


#if MAXPLAYERS>32
#error please update "player_name" table using the new value for MAXPLAYERS
#endif
#if MAXPLAYERNAME!=21
#error please update "player_name" table using the new value for MAXPLAYERNAME
#endif
// changed to 2d array 19990220 by Kin
char    player_names[MAXPLAYERS][MAXPLAYERNAME] =
{
    // THESE SHOULD BE AT LEAST MAXPLAYERNAME CHARS
    "Player 1\0a123456789a\0",
    "Player 2\0a123456789a\0",
    "Player 3\0a123456789a\0",
    "Player 4\0a123456789a\0",
    "Player 5\0a123456789a\0",        // added 14-1-98 for support 8 players
    "Player 6\0a123456789a\0",        // added 14-1-98 for support 8 players
    "Player 7\0a123456789a\0",        // added 14-1-98 for support 8 players
    "Player 8\0a123456789a\0",        // added 14-1-98 for support 8 players
    "Player 9\0a123456789a\0",
    "Player 10\0a123456789\0",
    "Player 11\0a123456789\0",
    "Player 12\0a123456789\0",
    "Player 13\0a123456789\0",
    "Player 14\0a123456789\0",
    "Player 15\0a123456789\0",
    "Player 16\0a123456789\0",
    "Player 17\0a123456789\0",
    "Player 18\0a123456789\0",
    "Player 19\0a123456789\0",
    "Player 20\0a123456789\0",
    "Player 21\0a123456789\0",
    "Player 22\0a123456789\0",
    "Player 23\0a123456789\0",
    "Player 24\0a123456789\0",
    "Player 25\0a123456789\0",
    "Player 26\0a123456789\0",
    "Player 27\0a123456789\0",
    "Player 28\0a123456789\0",
    "Player 29\0a123456789\0",
    "Player 30\0a123456789\0",
    "Player 31\0a123456789\0",
    "Player 32\0a123456789\0"
};


// DEATHMATCH
void Deathmatch_OnChange(void);

// Coop 0: multiple players, coop map objects
// Deathmatch 1:  placed weapons respawn immediately
// Deathmatch 2:  items respawn
// Deathmatch 3:  items respawn, placed weapons respawn immediately
// Keep original values for demo and savegame compatibility.
CV_PossibleValue_t deathmatch_cons_t[] = {
  {0x80, "Coop_SP_Map"}, {0x30, "Coop_60"}, {0x20, "Coop_80"}, {0x10, "Coop"}, {0, "Coop_weapons"},
  {4, "DM"}, {1, "DM_weapons"}, {2, "DM_items"}, {3, "DM_both"}, {0, NULL} };
consvar_t cv_deathmatch = { "deathmatch", "0", CV_NETVAR | CV_CALL, deathmatch_cons_t, Deathmatch_OnChange };

// [Arcade] Single Level mode: play one chosen map and return to its menu
// rather than continuing into the next map.  Also selects a separate high
// score table (HS_GameId_Mode), so these runs never mix with campaign ones.
// Cleared by Command_ExitGame_f, the single funnel back to the attract
// screen, so the attract page always shows campaign times.
byte  single_level_mode = 0;

byte  deathmatch;
byte  weapon_persist;         // deathmatch weapon pickup multiple times

// deathmatch (0..3)
byte  deathmatch_to_itemrespawn[4]    = { 0, 0, 1, 1 };

void Deathmatch_OnChange(void)
{
    deathmatch = (cv_deathmatch.EV < 0x0F)?  (cv_deathmatch.EV & 0x07) : 0;
    weapon_persist = 0;

    if( cv_deathmatch.EV < 4 )
    {
        // [WDJ] Server and Client, change other cvar affected by deathmatch, locally.
        // Do not modify cvar here, and expect NETVAR to update them in clients.

        // weapon respawn global enable
        weapon_persist = (cv_deathmatch.EV != 2);  // only the orig modes

        // itemrespawn for deathmatch 2,3
        CV_SetParam( &cv_itemrespawn, deathmatch_to_itemrespawn[ deathmatch & 0x03 ] );
        // itemrespawntime is NETVAR, and will have been distributed at join game.

        // [WDJ] Respawn weapons in the itemrespawn queue, for weapon_persist.
        // Fixed code inconsistency: it did not do this when going to coop mode.
        if( weapon_persist )
            P_RespawnWeapons();
    }

    // give all key to the players
    if( deathmatch )
    {
        int j;
        for (j = 0; j < MAXPLAYERS; j++)
        {
            if (playeringame[j])
                players[j].cards = it_allkeys;
        }
    }
}


// TIMELIMIT, FRAGLIMIT

void TimeLimit_OnChange(void);
consvar_t cv_timelimit = { "timelimit", "0", CV_NETVAR | CV_VALUE | CV_CALL | CV_NOINIT, CV_Unsigned, TimeLimit_OnChange };

uint32_t  timelimit_tics = 0;

void TimeLimit_OnChange(void)
{
    // CV_VALUE, may be too large for EV
    if (cv_timelimit.value)
    {
        GenPrintf(EMSG_hud, "Levels will end after %d minute(s).\n", cv_timelimit.value);
        timelimit_tics = cv_timelimit.value * 60 * TICRATE;
    }
    else
    {
        GenPrintf(EMSG_hud, "Time limit disabled\n");
        timelimit_tics = 0;
    }
}

void FragLimit_OnChange(void);
CV_PossibleValue_t fraglimit_cons_t[] = { {0, "MIN"}, {1000, "MAX"}, {0, NULL} };
consvar_t cv_fraglimit = { "fraglimit", "0", CV_NETVAR | CV_VALUE | CV_CALL | CV_NOINIT, fraglimit_cons_t, FragLimit_OnChange };

void FragLimit_OnChange(void)
{
    int i;

    // CV_VALUE, may be too large for EV
    if (cv_fraglimit.value > 0)
    {
        for (i = 0; i < MAXPLAYERS; i++)
            P_CheckFragLimit(&players[i]);
    }
}


// TEAM STATE

team_info_t*  team_info[MAXTEAMS];  // allocated
byte  num_teams = 0;

// Create the team if it does not exist.
team_info_t*  get_team( int team_num )
{
    if( team_num >= MAXTEAMS )
        return NULL;

    // Create missing teams
    while( team_num >= num_teams )
    {
        team_info[num_teams] = Z_Malloc( sizeof(team_info_t), PU_STATIC, NULL);
        team_info[num_teams]->name = NULL;
        num_teams++;
    }
    return team_info[team_num];
}

// Set the team name.
// Create the team if it does not exist.
void  set_team_name( int team_num, const char * str )
{
    // Create the team if it does not exist.
    // Because of the complexity, for now, will create team at init.
    get_team( team_num );

    if( team_num <= num_teams )
    {
        char * name = team_info[team_num]->name;
        if( name )
            Z_Free( name );
        name = Z_Malloc( strlen(str)+6, PU_STATIC, NULL );
        sprintf(name,"%s team", str);
        team_info[team_num]->name = name;
    }
}

char * get_team_name( int team_num )
{
    if( team_num <= num_teams )
    {
        if( team_info[team_num]->name )
            return team_info[team_num]->name;
    }
    return "Unknown team";
}


CV_PossibleValue_t teamplay_cons_t[] = { {0, "Off"}, {1, "Color"}, {2, "Skin"}, {3, NULL} };

void  TeamPlay_OnChange( void );
consvar_t cv_teamplay = { "teamplay", "0", CV_NETVAR | CV_CALL, teamplay_cons_t, TeamPlay_OnChange };
consvar_t cv_teamdamage = { "teamdamage", "0", CV_NETVAR, CV_OnOff };


void TeamPlay_OnChange(void)
{
    int i;
    // Change the name of the teams

    if(cv_teamplay.EV == 1)
    {
        // color
        for(i=0; i<NUMSKINCOLORS; i++)
            set_team_name( i, Color_Names[i]);
    }
    else
    if(cv_teamplay.EV == 2)
    {
        // skins
        for(i=0; i<numskins; i++)
            set_team_name( i, skins[i]->name);
    }
}


// Support

// Simplified body queue.  The Doom bodyqueue was way complicated.
// A way to have player corpses stay around, but limit how many.
mobj_t*   bodyque[BODYQUESIZE];
int       bodyqueslot;


void*     statcopy;                      // for statistics driver

void ShowMessage_OnChange(void)
{
    if( !cv_showmessages.EV )
        CONS_Printf("%s\n",MSGOFF);
    else
        CONS_Printf("%s: %s\n",MSGON, cv_showmessages.string );
}


//  Build an original game map name from episode and map number,
//  based on the game mode (doom1, doom2...)
//
char* G_BuildMapName (int episode, int map)
{
    static char  mapname[9];    // internal map name (wad resource name)

    if (gamemode==doom2_commercial)
        strcpy (mapname, va("MAP%#02d",map));
    else
    {
        mapname[0] = 'E';
        mapname[1] = '0' + episode;
        mapname[2] = 'M';
        mapname[3] = '0' + map;
        mapname[4] = 0;
    }
    return mapname;
}


typedef struct {
    uint16_t bdtc;
    const char * str;
} BDTC_to_str_t;

static const BDTC_to_str_t  BDTC_to_name[]  = {
  { BDTC_doom, "Doom" },
  { BDTC_boom, "Boom" },
  { BDTC_prboom, "PrBoom" },
  { BDTC_MBF, "MBF" },
  { BDTC_MBF21, "MBF21" },
  { BDTC_heretic, "Heretic" },
  { BDTC_hexen, "Hexen" },
  { BDTC_strife, "Strife" },
  { BDTC_legacy, "DoomLegacy" },
  { BDTC_ee, "EternityEngine" },
  { BDTC_dehextra, "DEHEXTRA" },
  { BDTC_dsda, "DSDA" },
//  { BDTC_acp2, "ACP2" },  // have flag, but not for detect
};
#define NUM_BDTC_to_name  (sizeof(BDTC_to_name)/sizeof(BDTC_to_str_t))

// [WDJ] BDTC_game_detection_e bits
uint16_t all_detect;

void report_detect( const char * who, uint16_t detect )
{
    int i;
    for( i = 0; i < NUM_BDTC_to_name; i ++ )
    {
        const BDTC_to_str_t * rdp = &BDTC_to_name[i];
        if( detect & rdp->bdtc )
            GenPrintf(EMSG_warn, "%s: %s detected\n", who, rdp->str );
    }
    all_detect |= detect;
}


//
//  Clip the console player mouse aiming to the current view,
//  also returns a signed char for the player ticcmd if needed.
//  Used whenever the player view pitch is changed manually
//
//added:22-02-98:
//changed:3-3-98: do a angle limitation now
angle_t G_ClipAimingPitch(angle_t aiming)
{
  int32_t limitangle;

  //note: the current software mode implementation doesn't have true perspective
  if (rendermode == render_soft)
    limitangle = 732<<ANGLETOFINESHIFT;
  else
    limitangle = ANG90 - 1;

  int32_t p = aiming; // into signed to make comparisions simpler

  if (p > limitangle)
    p = limitangle;
  else if (p < -limitangle)
    p = -limitangle;
  
  return p; // back into angle_t (unsigned)
}


//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
// set displayplayer2_ptr to build player 2's ticcmd in splitscreen mode
//
//  [0]=main player, [1]=splitscreen player
angle_t localaiming[MAXSPLITSCREENPLAYERS];
angle_t localangle[MAXSPLITSCREENPLAYERS];

//added:06-02-98: mouseaiming (looking up/down with the mouse or keyboard)
#define KB_LOOKSPEED    (1<<25)
#define MAXPLMOVE		(forwardmove[1])
#define TURBOTHRESHOLD  0x32
#define SLOWTURNTICS    (6*NEWTICRATERATIO)

static fixed_t forwardmove[2] = {25/NEWTICRATERATIO, 50/NEWTICRATERATIO};
static fixed_t sidemove[2]    = {24/NEWTICRATERATIO, 40/NEWTICRATERATIO};
static fixed_t angleturn[3]   = {640, 1280, 320};        // + slow turn


// for change this table change also nextweapon func in g_game and P_PlayerThink
char extraweapons[8]={wp_chainsaw,-1,wp_supershotgun,-1,-1,-1,-1,-1};
byte nextweaponorder[NUMWEAPONS]={wp_fist,wp_chainsaw,wp_pistol,
     wp_shotgun,wp_supershotgun,wp_chaingun,wp_missile,wp_plasma,wp_bfg};

static
byte NextWeapon( player_t * player, int step )
{
    byte   w;
    int    plwpn = player->readyweapon;
    int    i;

    for (i=0;i<NUMWEAPONS;i++)
    {
        if( plwpn == nextweaponorder[i] )
        {
            i = (i+NUMWEAPONS+step)%NUMWEAPONS;
            break;
        }
    }

    for ( ; nextweaponorder[i] != plwpn; i = (i+NUMWEAPONS+step)%NUMWEAPONS )
    {
        w = nextweaponorder[i];
        
        // skip super shotgun for non-Doom2
        if (gamemode!=doom2_commercial && w==wp_supershotgun)
            continue;

        // skip plasma-bfg in shareware
        if (gamemode==doom_shareware && (w==wp_plasma || w==wp_bfg))
            continue;

        if ( player->weaponowned[w]
             && player->ammo[player->weaponinfo[w].ammo] >= player->weaponinfo[w].ammo_per_shot )
        {
            if(w==wp_chainsaw)
                return (BT_CHANGE | BT_EXTRAWEAPON | (wp_fist<<BT_WEAPONSHIFT));
            if(w==wp_supershotgun)
                return (BT_CHANGE | BT_EXTRAWEAPON | (wp_shotgun<<BT_WEAPONSHIFT));
            return (BT_CHANGE | (w<<BT_WEAPONSHIFT));
        }
    }
    return 0;
}

static
byte BestWeapon( player_t * player )
{
    int newweapon = FindBestWeapon(player);

    if (newweapon == player->readyweapon)
        return 0;

    if (newweapon == wp_chainsaw)
        return (BT_CHANGE | BT_EXTRAWEAPON | (wp_fist<<BT_WEAPONSHIFT));

    if (newweapon == wp_supershotgun)
        return (BT_CHANGE | BT_EXTRAWEAPON | (wp_shotgun<<BT_WEAPONSHIFT));

    return (BT_CHANGE | (newweapon<<BT_WEAPONSHIFT));
}

// pind : 0,1 for split player identity
static
boolean G_InventoryResponder(player_t * ply, byte pind,
                             int gc[num_gamecontrols][2], event_t * ev)
{
  // [WDJ] 1/9/2009 Do not get to process any keyup events, unless also saw
  // the keydown event.  Now other Responders intercepting
  // the keydown event work correctly.  Specifically heretic will no longer
  // use up an inventory item when game saving.
  static byte keyup_armed[2] = {0,0};   // player1, player2

  // Do not mess with inventory when menu or console are open.
  if( menuactive || console_open )
    return false;

  if (! EN_inventory)
    return false;

  switch (ev->type)
  {
    case ev_keydown:
      if( ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1] )
      {
                if( ply->st_inventoryTics )
                {
                    ply->inv_ptr--;
                    if( ply->inv_ptr < 0 )
                        ply->inv_ptr = 0;
                    else
                    {
                        ply->st_curpos--;
                        if( ply->st_curpos < 0 )
                            ply->st_curpos = 0;
                    }
                }
                ply->st_inventoryTics = 5*TICRATE;
                goto used_key;
      }
      else if( ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1] )
      {
                if( ply->st_inventoryTics )
                {
                    ply->inv_ptr++;
                    if( ply->inv_ptr >= ply->inventorySlotNum )
                    {
                        ply->inv_ptr--;
                        if( ply->inv_ptr < 0)
                            ply->inv_ptr = 0;
                    }
                    else
                    {
                        ply->st_curpos++;
                        if( ply->st_curpos > 6 )
                            ply->st_curpos = 6;
                    }
                }
                ply->st_inventoryTics = 5*TICRATE;
                goto used_key;
      }
      else if( ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1] ){
                goto arm_key;
      }

      break;

    case ev_keyup:
      if( ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1] )
      {
          if( keyup_armed[pind] )  // [WDJ] Only if the keydown was not intercepted by some other responder
          {
              if( ply->st_inventoryTics )
                 ply->st_inventoryTics = 0;
              else if( ply->inventory[ply->inv_ptr].count>0 )
              {
                  Send_NetXCmd_pind( XD_USEARTIFACT, &ply->inventory[ply->inv_ptr].type, 1, pind);
              }
              goto used_key;
          }
      }
      else if( ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1] ||
               ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1] )
          goto used_key;
      break;

    default:
      break; // shut up compiler
  }

  keyup_armed[pind] = 0;  // blanket unused
  return false;

arm_key:
  keyup_armed[pind] = 1;  // ready for keyup event
  return true;
   
used_key:
  keyup_armed[pind] = 0;  // used up
  return true;
}



//  pind : player index, [0]=main player, [1]=splitscreen player
void G_BuildTiccmd(ticcmd_t* cmd, int realtics, byte pind)
{
    ticcmd_t * base = I_BaseTiccmd ();             // empty, or external driver
    memcpy (cmd,base,sizeof(*cmd));

    
    player_t * this_player;
    int (*gcc)[2];

#define G_KEY_DOWN(k) (gamekeydown[gcc[(k)][0]] || gamekeydown[gcc[(k)][1]])
#define G_KEY_PRESSED(k) (G_KEY_DOWN(k) || gamekeytapped[gcc[(k)][0]] || gamekeytapped[gcc[(k)][1]])

    angle_t pitch;

    // [Arcade] Take the player from localplayer[pind], not from a display
    // pointer.  This used to read displayplayer2_ptr, which is about the
    // second *view*: it is NULL whenever the screen is not split, so a panel
    // with a player but no viewport dereferenced NULL here.  Views and local
    // players are separate things once a cabinet has more than two panels.
    if( pind == 0 )
    {
      this_player = consoleplayer_ptr;
    }
    else
    {
      byte pn = localplayer[pind];
      if( pn >= MAXPLAYERS )  goto done;   // this panel has no player
      this_player = &players[pn];
    }
    gcc = gamecontrol_pl[pind];
    pitch = localaiming[pind];

    // Exit now if locked
    if( this_player->GB_flags & GB_locked )
      goto done;

    // a little clumsy, but then the g_input.c became a lot simpler!
    boolean strafe = G_KEY_DOWN(gc_strafe);
    int speed  = G_KEY_DOWN(gc_speed) ^ cv_autorun[pind].EV;

    boolean turnright = G_KEY_DOWN(gc_turnright);
    boolean turnleft  = G_KEY_DOWN(gc_turnleft);
    boolean mouseaiming = G_KEY_DOWN(gc_mouseaiming) ^ cv_alwaysfreelook[pind].EV;

    int forward = 0, side = 0; // these must not wrap around, so we need bigger ranges than chars

    // strafing and yaw
    if (strafe)
    {
        if (turnright)
            side += sidemove[speed];
        if (turnleft)
            side -= sidemove[speed];
    }
    else
    {
      // use two stage accelerative turning
      // on the keyboard and joystick
      static int  turnheld[2];   // for accelerative turning

      if (turnleft || turnright)
        turnheld[pind] += realtics;
      else
        turnheld[pind] = 0;
      
      int tspeed = (turnheld[pind] < SLOWTURNTICS) ? 2 : speed;

      if (turnright)
        cmd->angleturn -= angleturn[tspeed];
      if (turnleft)
        cmd->angleturn += angleturn[tspeed];
    }

    // forwards/backwards, strafing
    if (G_KEY_DOWN(gc_forward))
        forward += forwardmove[speed];
    if (G_KEY_DOWN(gc_backward))
        forward -= forwardmove[speed];
    //added:07-02-98: some people strafe left & right with mouse buttons
    if (G_KEY_DOWN(gc_straferight))
        side += sidemove[speed];
    if (G_KEY_DOWN(gc_strafeleft))
        side -= sidemove[speed];

    //added:07-02-98: fire with any button/key
    if (G_KEY_DOWN(gc_fire))
        cmd->buttons |= BT_ATTACK;

    //added:07-02-98: use with any button/key
    if (G_KEY_DOWN(gc_use))
        cmd->buttons |= BT_USE;

    //added:22-02-98: jump button
    if (cv_allowjump.EV && G_KEY_DOWN(gc_jump))
        cmd->buttons |= BT_JUMP;

#ifdef ENABLE_COME_HERE
    if( G_KEY_DOWN(gc_comehere) )
    {
# ifdef TICCMD_148
        cmd->ticflags |= TC_comehere;
# endif
    }
#endif

    //added:07-02-98: any key / button can trigger a weapon
    // chainsaw overrides
    if (G_KEY_PRESSED(gc_nextweapon))
      cmd->buttons |= NextWeapon(this_player,1);
    else if (G_KEY_PRESSED(gc_prevweapon))
      cmd->buttons |= NextWeapon(this_player,-1);
    else if (G_KEY_PRESSED(gc_bestweapon))
      cmd->buttons |= BestWeapon(this_player);
    else
    {
        int ki;
        for (ki=gc_weapon1; ki<gc_weapon1+NUMWEAPONS-1; ki++)
        {
            if (G_KEY_PRESSED(ki))
            {
                int wix = ki - gc_weapon1;
                cmd->buttons |= BT_CHANGE | BT_EXTRAWEAPON; // extra by default
                cmd->buttons |= wix<<BT_WEAPONSHIFT;
                // already have extraweapon in hand switch to the normal one
                if (this_player->readyweapon == extraweapons[wix])
                    cmd->buttons &= ~BT_EXTRAWEAPON;
                break;
            }
        }
    }


    // pitch
    static byte  keyboard_look[2]; // true if lookup/down using keyboard


    // spring back if not using keyboard neither mouselookin'
    if (!keyboard_look[pind] && !mouseaiming)
        pitch = 0;

    if (G_KEY_DOWN(gc_lookup))
    {
        pitch += KB_LOOKSPEED;
        keyboard_look[pind] = true;
    }
    else
    if (G_KEY_DOWN(gc_lookdown))
    {
        pitch -= KB_LOOKSPEED;
        keyboard_look[pind] = true;
    }
    else
    if (G_KEY_PRESSED(gc_centerview))
    {
        pitch = 0;
        keyboard_look[pind] = false;
    }

    // mice

    // mouse look stuff (mouse look is not the same as mouse aim)
    if (pind == 0)
    {
      if (mouseaiming)
      {
        keyboard_look[pind] = false;

        // looking up/down
        if (cv_mouse_invert.EV)
            pitch -= mousey<<19;
        else
            pitch += mousey<<19;
      }
      else if (cv_mouse_move[0].EV)
        forward += mousey;

      if (strafe)
        side += mousex*2;
      else
        cmd->angleturn -= mousex*8;

      mousex = mousey = 0;
    }
    else
    {
      if (mouseaiming)
      {
        keyboard_look[pind] = false;

        // looking up/down
#ifdef MOUSE2
        if (cv_mouse2_invert.EV)
          pitch -= mouse2y<<19;
        else
#endif	   
          pitch += mouse2y<<19;
      }
      else if (cv_mouse_move[1].EV)
        forward += mouse2y;

      if (strafe)
        side += mouse2x*2;
      else
        cmd->angleturn -= mouse2x*8;

      mouse2x = mouse2y = 0;
    }

#ifdef JOYSTICK_SUPPORT
    // Finally the joysticks.
    int ji;
    for (ji=0; ji < num_joybindings; ji++)
    {
      joybinding_t jb = joybindings[ji];

      if (jb.playnum != pind)
        continue;

      int joyvalue = I_JoystickGetAxis(jb.joynum, jb.axisnum);
      if( joyvalue >= -cv_joy_deadzone.value && joyvalue <= cv_joy_deadzone.value)  joyvalue = 0; // Deadzone
      int value = (int)(jb.scale * joyvalue);

      switch (jb.action)
      {
        case ja_pitch:
          pitch = value << 16;
          break;
        case ja_move:
          if(speed == 1) value *= 2; // Double value if player is running
          forward += value;
          break;
        case ja_turn:
          cmd->angleturn += value;
          break;
        case ja_strafe:
          if(speed == 1) value *= 2; // Double value if player is running
          side += value;
          break;
        default:
          break;
      }
    }
#endif


    // Do not go faster than max. speed
    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;

    //26/02/2000: added by Hurdler: accept no mlook for network games
    if (!cv_allowmlook.EV)
        pitch = 0;

    pitch = G_ClipAimingPitch(pitch); // clip pitch to a reasonable sector
    cmd->aiming = pitch >> 16; // to short

    // Generated cmd are absolute angles
    localangle[pind] += (cmd->angleturn<<16);
    cmd->angleturn = localangle[pind] >> 16;
    localaiming[pind] = pitch;

    if( gamemode == heretic )
    {
        if (G_KEY_DOWN(gc_flydown))
#ifdef TICCMD_148
            cmd->ticflags |= TC_flydown;
        else
            cmd->ticflags &= ~TC_flydown;
#else
            cmd->angleturn |= BT_FLYDOWN;
        else
            cmd->angleturn &= ~BT_FLYDOWN;
#endif
    }

 done:
    memset(gamekeytapped, 0, sizeof(gamekeytapped)); // we're done, reset key-tapping status
#ifdef TICCMD_148
    cmd->ticflags |= TC_received;
#endif
}


static fixed_t  originalforwardmove[2] = {0x19, 0x32};
static fixed_t  originalsidemove[2] = {0x18, 0x28};

void AllowTurbo_OnChange(void)
{
    if(!cv_allowturbo.EV && netgame)
    {
        // like turbo 100
        forwardmove[0] = originalforwardmove[0];
        forwardmove[1] = originalforwardmove[1];
        sidemove[0] = originalsidemove[0];
        sidemove[1] = originalsidemove[1];
    }
}

//  turbo <10-255>
//
void Command_Turbo_f (void)
{
    int     scale = 200;

    if(!cv_allowturbo.EV && netgame)
    {
        CONS_Printf("This server don't allow turbo\n");
        return;
    }

    if (COM_Argc()!=2)
    {
        CONS_Printf("turbo <10-255> : set turbo");
        return;
    }

    scale = atoi (COM_Argv(1));

    if (scale < 10)
        scale = 10;
    if (scale > 255)
        scale = 255;

    CONS_Printf ("turbo scale: %i%%\n",scale);

    forwardmove[0] = originalforwardmove[0]*scale/100;
    forwardmove[1] = originalforwardmove[1]*scale/100;
    sidemove[0] = originalsidemove[0]*scale/100;
    sidemove[1] = originalsidemove[1]*scale/100;
}


//
// G_DoLoadLevel
// All games, except loading savegames.
//
// Called from:  G_InitNew, G_DoReborn, G_DoWorldDone, Command_Restart_f
void G_DoLoadLevel (boolean resetplayer)
{
    int             i;

    levelstarttic = gametic;        // for time calculation
    // [Arcade] Re-arm the idle timer for a new level, so intermission time
    // does not carry over.  NOT during demo playback: the attract cycle
    // starts a new demo every ~30-45s, and each one came through here and
    // reset the timer -- so a menu left open on the attract screen could
    // never accumulate the 60s the timeout wants, and appeared to be broken.
    if( ! demoplayback )
        last_input_tic = gametic;
    // [WDJ] Derived from PrBoom, gametic demosync.
    if( EN_boom && !EN_mbf )
        game_comp_tic = 0;  // Boom demos start at tic 0

    gameplay_msg = false;

    // Reset certain attributes
    // (should be in resetplayer 'if'?)
    fs_fadealpha = 0;
    extramovefactor = 0;
    jumpgravity = (6*FRACUNIT/NEWTICRATERATIO);  // re-init
    consoleplayer_ptr->GB_flags &= ~(GB_locked);

    if (wipegamestate == GS_LEVEL)
        wipegamestate = GS_FORCEWIPE;  // force a wipe

    gamestate = GS_LEVEL;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if( resetplayer || (playeringame[i] && players[i].playerstate == PST_DEAD))
            players[i].playerstate = PST_REBORN;
        memset (players[i].frags,0,sizeof(players[i].frags));
        players[i].addfrags = 0;
    }

#ifdef DOGS   
    extra_dog_count = 0;
#endif

    // game_map_filename is external wad
    if (!P_SetupLevel (gameepisode, gamemap, gameskill, game_map_filename ))
    {
        // fail so reset game stuff
        Command_ExitGame_f();
        return;
    }

    // [WDJ] Some demos specify a console player that does not exist.
    // This happens before demoplayback is set.
    // Have not been able to determine anything better to do.
    if ( consoleplayer_ptr->weaponinfo == NULL )
    {
        G_AddPlayer( consoleplayer );  // to prevent segfaults
        playeringame[consoleplayer] = true;  // needs an mobj
    }

    if( ! demoplayback )  // because demo sets it too
    {
        displayplayer = consoleplayer;          // view the guy you are playing
        displayplayer_ptr = consoleplayer_ptr;
    }

    if(!cv_splitscreen.EV)
    {
        // [WDJ] Changed to a testable off for player 2
        displayplayer2 = -1;
        displayplayer2_ptr = NULL;  // use as test for player2 active
    }

    DemoAdapt_bots();
    if( server )
    {
        B_Regulate_Bots( cv_bots.EV );
    }

    gameaction = ga_nothing;
#ifdef PARANOIA
    Z_CheckHeap (-2);
#endif

    if (camera.chase)
        P_ResetCamera ( displayplayer_ptr );

    // clear cmd building stuff
    memset(gamekeydown, 0, sizeof(gamekeydown));
    memset(gamekeytapped, 0, sizeof(gamekeytapped));
    mousex = mousey = mouse2x = mouse2y = 0;
    I_StartupMouse( true );  // play mode

    // [WDJ] In case demo is from other than player1 (from prboom, killough)
    ST_Start();
    // clear hud messages remains (usually from game startup)
    HU_Clear_FSPics();
    CON_Clear_HUD ();

    gameplay_msg = true;
}

//
// G_Responder
//  Get info needed to make ticcmd_ts for the players.
//
// return true if event is acted upon
// [WDJ] The return of this is ignored, because it is last in the chain.
boolean G_Responder (event_t* ev)
{
    // allow spy mode changes even during the demo
    if (gamestate == GS_LEVEL && ev->type == ev_keydown
        && ev->data1 == KEY_F12
        && ( singledemo || ! deathmatch ) )
    {
        // spy mode
        do
        {
            displayplayer++;
            if (displayplayer == MAXPLAYERS)
                displayplayer = 0;
        } while (!playeringame[displayplayer] && displayplayer != consoleplayer);
        displayplayer_ptr = &players[displayplayer];

        //added:16-01-98:change statusbar also if playingback demo
        if( singledemo )
            ST_Change_DemoView ();

        //added:11-04-98: tell who's the view
        GenPrintf(EMSG_hud, "Viewpoint : %s\n", player_names[displayplayer]);
        goto handled;
    }

    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo &&
        (demoplayback || gamestate == GS_DEMOSCREEN) )
    {
        if (ev->type == ev_keydown)
        {
            M_StartControlPanel ();
            goto handled;
        }
        goto rejected;
    }

    if (gamestate == GS_LEVEL)
    {
#if 0
        if (devparm && ev->type == ev_keydown && ev->data1 == ';')
        {
            // added Boris : test different player colors
            consoleplayer_ptr->skincolor = (consoleplayer_ptr->skincolor+1) % NUMSKINCOLORS;
            consoleplayer_ptr->mo->flags |= (consoleplayer_ptr->skincolor)<<MF_TRANSSHIFT;
            G_DeathMatchSpawnPlayer (0);
            goto handled;
        }
#endif
        if(!multiplayer)
        {
           if( cht_Responder (ev))
              goto handled;
        }
        if (HU_Responder (ev))
            goto handled; // chat ate the event
        if (ST_Responder (ev))
            goto handled; // status window ate it
        if (AM_Responder (ev))
            goto handled; // automap ate it
        if (G_InventoryResponder (consoleplayer_ptr, 0, gamecontrol, ev))
            goto handled;
        if (displayplayer2_ptr
            && G_InventoryResponder (displayplayer2_ptr, 1, gamecontrol2, ev))
            goto handled;
        //added:07-02-98: map the event (key/mouse/joy) to a gamecontrol
    }

    if (gamestate == GS_FINALE)
    {
        if (F_Responder (ev))
            goto handled;  // finale ate the event
    }


    // update keys current state
    G_MapEventsToControls (ev);

    switch (ev->type)
    {
      case ev_keydown:
        if (ev->data1 == KEY_PAUSE  // keyboard
            || ev->data1 == gamecontrol[gc_pause][0] || ev->data1 == gamecontrol[gc_pause][1] )  // joystick
        {
            COM_BufAddText("pause\n");
            goto handled;
        }
        goto handled;

      case ev_keyup:
        goto rejected;   // always let key up events filter down

      case ev_mouse:
        goto handled;  // eat events

      default:
        break;
    }

rejected:
    return false;

handled:
    return true;
}


// [Arcade] Demo desync diagnostic.
// With -synclog, write one line of simulation state per tic while a demo
// is being recorded or played back.  Recording goes to synclog_rec.txt,
// playback to synclog_play.txt; diff them and the first differing line is
// the tic where the simulation diverged.
// Lines are keyed on leveltime (which restarts at 0 on every level load)
// rather than gametic, because a recording session and a playback session
// reach the level at different absolute gametic values.
static FILE * synclog_fp = NULL;
static byte   synclog_mode = 0;   // 0 = unchecked, 1 = off, 2 = recording, 3 = playback

void G_Synclog_Tic( void )
{
    player_t * p = &players[consoleplayer];

    if( synclog_mode == 1 )  return;

    if( synclog_mode == 0 )
    {
        if( ! M_CheckParm("-synclog") )
        {
            synclog_mode = 1;
            return;
        }
        if( demorecording )
            synclog_fp = fopen("synclog_rec.txt", "w");
        else if( demoplayback )
            synclog_fp = fopen("synclog_play.txt", "w");
        if( ! synclog_fp )
        {
            synclog_mode = 1;   // neither, or could not open
            return;
        }
        synclog_mode = demorecording ? 2 : 3;
        fprintf(synclog_fp,
                "# leveltime prnd x y angle momx momy fwd side aturn btn tflags\n");
    }

    if( p->mo )
    {
        ticcmd_t * c = &p->cmd;
        fprintf(synclog_fp, "%u %u %d %d %u %d %d %d %d %d %u %u\n",
                (unsigned int) leveltime, (unsigned int) P_Rand_GetIndex(),
                p->mo->x, p->mo->y, (unsigned int) p->mo->angle,
                p->mo->momx, p->mo->momy,
                (int) c->forwardmove, (int) c->sidemove, (int) c->angleturn,
                (unsigned int) c->buttons,
#ifdef TICCMD_148
                (unsigned int) c->ticflags
#else
                0u
#endif
                );
        fflush(synclog_fp);
    }
}


// [Arcade lockdown] Idle-to-title timeout.
// Called once per tic from G_Ticker for GS_LEVEL, GS_INTERMISSION and
// GS_FINALE.  The latter two matter as much as play does: both wait
// indefinitely for a keypress the cabinet's walk-away player will never
// give, so without this the machine hangs on the tally or the end text.
//
// Not during demo playback: the attract-mode demos generate no real input,
// so the timer would always expire and kick back to the title.
// Local splitscreen sets netgame too (see M_StartServer), so test for it
// explicitly -- otherwise a two player game on the cabinet would never
// return to the attract screen.  Only a real network game, which this build
// cannot reach without -devmode, is exempt.
// in_menu: called for a menu left open on the attract screen, rather than
// for an abandoned game.  See the two differences below.
static void G_Idle_Timeout_Check( boolean in_menu )
{
    static int last_warn_secs_shown = -1;
    tic_t idle_tics, timeout_tics, warn_tics;

    if( devmode || cv_idletimeout.value <= 0 )  return;

    // [Arcade] A demo playing on its own *is* the attract screen doing its
    // job, so playback normally means "not idle".  But an attract demo is
    // still running behind an open menu, so when a menu is up that guard has
    // to be dropped or the menu case could never fire.
    if( demoplayback && ! in_menu )  return;

    if( netgame && ! cv_splitscreen.EV )  return;

    idle_tics    = gametic - last_input_tic;
    timeout_tics = (tic_t)cv_idletimeout.value * TICRATE;
    warn_tics    = (cv_idletimeout.value > cv_idlewarntime.value)
                    ? (tic_t)(cv_idletimeout.value - cv_idlewarntime.value) * TICRATE
                    : 0;

    if( idle_tics >= timeout_tics )
    {
        last_warn_secs_shown = -1;

        // [Arcade] A menu abandoned on the attract screen just needs closing
        // -- the attract cycle is already running behind it.  There is no
        // game to tear down, and calling Command_ExitGame_f here would
        // restart the title needlessly.  single_level_mode is cleared for the
        // same reason it is cleared there: the attract page keys off it.
        if( in_menu )
        {
            M_Clear_Menus( true );
            single_level_mode = 0;
            last_input_tic = gametic;   // do not re-fire on the next tic
            return;
        }

        // A level pack overrides the IWAD maps, so the attract screen's
        // built-in demos would play back against the wrong levels, and the
        // splitscreen state would persist.  Restart for a clean attract
        // screen instead of returning to title.
        if( M_LevelPack_Loaded() )
            M_Restart_Program( NULL, false );   // no return

        Command_ExitGame_f();
    }
    else if( idle_tics >= warn_tics && ! in_menu )
    {
        // Not warned for the menu case: D_Display only calls HU_Drawer for
        // GS_LEVEL, so on the attract screen HU_SetTip would draw nothing.
        int remain_secs = cv_idletimeout.value - (int)(idle_tics / TICRATE);
        if( remain_secs != last_warn_secs_shown )
        {
            char idlemsg[64];
            snprintf(idlemsg, sizeof(idlemsg), "Returning to title in %ds...", remain_secs);
            HU_SetTip(idlemsg, 3*TICRATE);
            last_warn_secs_shown = remain_secs;
        }
    }
    else
    {
        last_warn_secs_shown = -1;
    }
}


//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void)
{
    int         i;
    int         buf;
    ticcmd_t*   cmd;

    // do player reborns if needed
    if( gamestate == GS_LEVEL )
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (playeringame[i])
            {
                if( players[i].playerstate == PST_REBORN )
                    G_DoReborn (i);
                if( players[i].st_inventoryTics )
                    players[i].st_inventoryTics--;
            }
        }
#ifdef DOGS
        // [WDJ] MBF dogs, extension for DoomLegacy.
        if( extra_dog_respawn )  // coop respawn
        {
            extra_dog_respawn--;  // counter
            if( extra_dog_respawn == 0 )
            {
                G_SpawnExtraDog( NULL );
                if( extra_dog_count < cv_mbf_dogs.EV )
                    extra_dog_count = EXTRA_DOG_RESPAWN_TIME;
            }
        }
#endif
    }

    // Do things to change the game state.
    while (gameaction != ga_nothing)
    {
        switch (gameaction)
        {
            case ga_completed:
                G_DoCompleted ();
                G_Start_Intermission();
#ifdef WAIT_GAME_START_INTERMISSION
                // This must be after G_Start_Intermission because it overrides
                // wait_game_start_timer that is set to default for deathmatch.	   
                if( server )
                    SV_Add_game_start_waiting_players( 0 );
#endif
                break;
            case ga_worlddone:
                G_DoWorldDone ();
                break;
            case ga_playdemo:
                G_DoPlayDemo( playdemo_name );
                break;
            case ga_nothing:
                break;
            default:
                // [WDJ] Softer recovery.
                I_SoftError("GAME: gameaction = %d\n", gameaction);
                gameaction = ga_nothing;
                break;
        }
    }

    // [WDJ] From PrBoom, MBF, EternityEngine
    // killough 9/29/98: Skip some commands while pausing during demo playback,
    // or while the menu is active.
    //
    // Do not increment game_comp_tic and skip processing if a demo being played back
    // is paused or if the menu is active while a non-net game is being played,
    // to maintain sync while allowing pauses.
    //
    // P_Ticker() does not stop netgames if a menu is activated, so
    // we do not need to stop if a menu is pulled up during netgames.
    if( (paused & 0x02)
        || (!demoplayback && menuactive && !netgame ) )
    {
       goto main_actions;
    }
       
    game_comp_tic++;  // For revenant tracers and RNG -- we must maintain sync

    buf = gametic%BACKUPTICS;

    // read/write demo and check turbo cheat
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        // BP: i==0 for playback of demos 1.29 now new players is added with xcmd
        if ((playeringame[i] || i==0) && !dedicated)
        {
            cmd = &players[i].cmd;

            // All clients run the bots, identically.
            if (players[i].bot)	//added by AC for acbot
                B_BuildTiccmd(&players[i], &netcmds[buf][i]);

            if (demoplayback)
            {
                G_ReadDemoTiccmd (cmd,i);
            }
            else
            {
                // Copy the network ticcmd (and indirectly the local ticcmd) to the player.
                memcpy (cmd, &netcmds[buf][i], sizeof(ticcmd_t));
            }

            if (demorecording)
                G_WriteDemoTiccmd (cmd,i);

            // check for turbo cheats
            if (cmd->forwardmove > TURBOTHRESHOLD
                && !(gametic % (32*NEWTICRATERATIO)) && ((gametic / (32*NEWTICRATERATIO))&3) == i )
            {
#define TURBO_MSG_LEN  80	       
                static char turbomessage[TURBO_MSG_LEN];
                // [WDJ] Gcc10 thinks that the entire player_name array MAY be written to this string.
                // [MB] Using the return value will silence the warning.
                // Assignment to a dummy variable (even if declared "volatile") is not sufficient.
                if (0 > snprintf (turbomessage, TURBO_MSG_LEN-1, "%s is turbo!", player_names[i]) )
                    turbomessage[TURBO_MSG_LEN-1] = 0;  // good as anything else
                consoleplayer_ptr->message = turbomessage;
            }
        }
    }

main_actions:

    // do main actions
    switch (gamestate)
    {
      case GS_LEVEL:
        //IO_Color(0,255,0,0);
        // Apply player cmds to players, then run thinkers and level animations for this gametic.
        P_Ticker ();             // tic the game
        //IO_Color(0,0,255,0);
        ST_Ticker ();
        AM_Ticker ();
        HU_Ticker ();

        G_Synclog_Tic();   // [Arcade] demo desync diagnostic, -synclog

        // [Arcade] demoplayback && menuactive means an attract demo is running
        // behind an open menu -- gamestate is GS_LEVEL there, not
        // GS_DEMOSCREEN, and the plain check bails on demoplayback, so without
        // this the menu only timed out during the attract screen's *title*
        // pages and appeared not to work at all while a demo was on.
        // A real game with the menu open has demoplayback false and still
        // takes the normal path, which ends the game rather than just closing
        // the menu.
        G_Idle_Timeout_Check( demoplayback && menuactive );   // [Arcade]
        break;

      case GS_INTERMISSION:
        WI_Ticker ();
        // [Arcade] The tally screen waits forever for a keypress once the
        // counters finish (wi_stuff.c, sp_state == 10), so the cabinet must
        // be able to time out here too.
        G_Idle_Timeout_Check( false );
        break;

      case GS_FINALE:
        F_Ticker ();
        // [Arcade] Same for the end-of-episode text (F_Ticker's finalestage
        // 0 needs keypressed under doom2_commercial) and for the cast call,
        // which loops indefinitely.
        G_Idle_Timeout_Check( false );
        break;

      case GS_DEMOSCREEN:
        D_PageTicker ();
        // [Arcade] A menu left open on the attract screen would otherwise sit
        // there for ever -- the timeouts above only cover an abandoned game.
        if( menuactive )
            G_Idle_Timeout_Check( true );
        break;

      case GS_WAITINGPLAYERS:
      case GS_DEDICATEDSERVER:
      case GS_NULL:
      default:
        // do nothing
        gameplay_msg = false;
        break;
    }
}


// Spawn Test
// [WDJ] Use a temporary mobj to test position.
// This can be modified to suit,
// and being static does not leave dangling ptrs into stack space.
// Need flags, flags2, radius, height.
static mobj_t  tstobj;

//  testtype: determines the test mobj size, how much radius is needed
// Return false when an collision with an existing object would occur.
static
boolean  test_spot_unoccupied( mobjtype_t testtype, fixed_t x, fixed_t y, fixed_t z )
{
    // [WDJ] Test spawn spot for any player or blocking object.
    // Needed for spawning off of map spawn spots, and for multiple players.
    // Spawn test position is already set.
    // MF_SOLID is required to test against objects.
    // MF_PICKUP is off to prevent picking up objects.
    // MF2_PASSMOBJ is off, as in Heretic.
    tstobj.x = x;
    tstobj.y = y;
    tstobj.z = z;
    tstobj.player = NULL;  // so cannot take damage during test
    tstobj.type = testtype;
    tstobj.info = &mobjinfo[testtype];
    tstobj.radius = tstobj.info->radius;
    tstobj.height = tstobj.info->height;
    tstobj.flags2 = tstobj.info->flags2 & ~MF2_PASSMOBJ;
    tstobj.flags = MF_SOLID|MF_SHOOTABLE|MF_DROPOFF;
    return  P_CheckPosition( &tstobj, x, y );
}


boolean  G_Player_SpawnSpot( int playernum, mapthing_t* spot );


// Extra dynamic spawn spots
// Kept static because mobj will keep a ptr to it, and NULL is not good either.
mapthing_t extra_coop_spawn;  // roving coop spawn
// Static spawn index, so if a player gets a difficult spawn,
// it does not repeat every spawn.
static int32_t spind = 1;


static
boolean  scatter_spawn( mobjtype_t spawn_type, int playernum, mapthing_t * spot )
{
    int i;

    extra_coop_spawn = * spot;  // copy, will be modified
    tstobj.x = spot->x<<16;  // spot map location
    tstobj.y = spot->y<<16;
   
    for(i=255; i>0; i--)
    {
        spind += 83;  // scatter the pattern with prime 83
        // The low 8 bits of rv will cycle through all patterns in 256 iter.
        // Range (-15..15) * (player_radius + 2)
        extra_coop_spawn.x = spot->x + (((spind & 0x0F) - 8) * 18);
        extra_coop_spawn.y = spot->y + ((((spind >> 4) & 0x0F) - 8) * 18);
        
        // Not allowed to cross any blocking lines, to keep it out of the void.
        if( P_CheckCrossLine( &tstobj, extra_coop_spawn.x<<16, extra_coop_spawn.y<<16 ) )  continue;

        if( spawn_type == MT_PLAYER )
        {
            if (G_Player_SpawnSpot(playernum, &extra_coop_spawn) )
                return true;
        }
#ifdef DOGS
        else
        {
            if (G_SpawnExtraDog(&extra_coop_spawn) )
                return true;
        }
#endif       
    }
    return false;
}



#ifdef DOGS
// [WDJ] Spawn dogs.
// MBF spawns extra dogs at map load, only for single player.
// DoomLegacy also spawns dogs for Coop.

// Do not modify level map spot due to reusing them during coop.
// A player may join and use a spot that was previously used for a dog.
static mapthing_t  extra_dog_spot;

// Spawn extra player dog.
// These are dogs that start as player spots, limited by cv_mbf_dogs.
// Dogs that are level map objects spawn normally, as monsters or friends.
boolean  G_SpawnExtraDog( mapthing_t * spot )
{
    fixed_t       x, y;
    sector_t *    sec;

    // Spawn a dog on one of the unused player starts, up to cv_mbf_dogs.
    // Avoid multiple dogs in case of multiple starts, using playerstart.
    // Extra playerstarts will already be voodoo doll spots.
    if( extra_dog_count >= cv_mbf_dogs.EV )  goto no_more_dogs;

    if( deathmatch )  goto no_more_dogs;
   
    if( spot == NULL )
    {
        // [WDJ] Have run out of start spots.
        // This is a recursive call.
        int i;
        // Try to spawn near one of the other player spots.
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if( playerstarts[i] == NULL )  continue;
            if( scatter_spawn( MT_DOGS, 0, playerstarts[i] )  )
                return true;
        }
        return false;
    }
   
    // Avoid modify of level map things.
    memcpy( &extra_dog_spot, spot, sizeof(mapthing_t) );

    // killough 10/98: force it to be a friend
    // [WDJ] Make sure it is not blocked (MTF_MPSPAWN, MTF_NODM, MTF_NOCOOP).
    extra_dog_spot.options |= MTF_FRIEND | 0x07;
    extra_dog_spot.options &= ~(MTF_MPSPAWN | MTF_NODM | MTF_NOCOOP);

    // haleyjd 9/22/99: deh, bex substitution	       
    int extra_dog_MT = ( helper_MT < ENDDOOM_MT )? helper_MT : MT_DOGS;

    // [WDJ] Test spawn spot for any player or blocking object.
    // Use tstobj, so do not need player mobj.  There may not be one.
    // The spawn spot location.
    x = extra_dog_spot.x << FRACBITS;
    y = extra_dog_spot.y << FRACBITS;
    sec = R_PointInSubsector(x,y)->sector;
    // Needs MT_DOGS to get its radius from info.
    if( ! test_spot_unoccupied( extra_dog_MT, x, y, sec->floorheight ) )
        return false;
   
    // SpawnMapthing keeps a reference to the spawn spot in the mobj.
    // Don't have a valid a doomed number, spawn by MT_ code.
    P_Spawn_Mapthing( &extra_dog_spot, extra_dog_MT );

    extra_dog_count ++;
    return true;

no_more_dogs:
    extra_dog_respawn = 0;
    return false;
}

// To deal with respawning dog issues.
void  G_KillDog( mobj_t * mo )
{
    if( mo->spawnpoint != &extra_dog_spot )  return;

    if( !(mo->flags & MF_FRIEND) )  return;

    if( multiplayer && ( ! deathmatch )  // coop respawn
        && !cv_respawnmonsters.EV  )  // not otherwise respawned
    {
        extra_dog_count --;
        if( extra_dog_respawn == 0 )
            extra_dog_respawn = EXTRA_DOG_RESPAWN_TIME;
    }
}

#endif

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer
// Called at the start.
// Called by the game initialization functions.
//
/* BP:UNUSED !
void G_InitPlayer (int player)
{
    player_t*   p;

    // set up the saved info
    p = &players[player];

    // clear everything else to defaults
    G_PlayerReborn (player);
}
*/


//
// G_PlayerFinishLevel
//  Can when a player completes a level.
//
void G_PlayerFinishLevel (int player)
{
    player_t*  p;
    mobj_t * pmo;
    int        i;

    p = &players[player];
    pmo = p->mo;
    for(i=0; i<p->inventorySlotNum; i++)
    {
        if( p->inventory[i].count>1) 
            p->inventory[i].count = 1;
    }
    if( ! deathmatch )
    {
        for(i = 0; i < MAXARTECONT; i++)
            P_PlayerUseArtifact(p, arti_fly);
    }
    memset (p->powers, 0, sizeof (p->powers));
    if( gamemode == heretic )
        p->weaponinfo = wpnlev1info;    // cancel power weapons
    else
        p->weaponinfo = doomweaponinfo;
    p->cards = 0;
    if( pmo )
        pmo->flags &= ~MF_SHADOW;       // cancel invisibility
    p->extralight = 0;                  // cancel gun flashes
    p->fixedcolormap = 0;               // cancel ir gogles
    p->damagecount = 0;                 // no palette changes
    p->bonuscount = 0;
    p->health_pickup = 0;
    p->armor_pickup = 0;
    p->weapon_pickup = 0;
    p->ammo_pickup = 0;
    p->key_pickup = 0;
    p->aiming = 0;  // reset freelook 

    if(p->chickenTics)
    {
        if( pmo )
            p->readyweapon = pmo->special1; // Restore weapon
        p->chickenTics = 0;
    }
    p->rain1 = NULL;
    p->rain2 = NULL;
}


// added 2-2-98 for hacking with dehacked patch
int initial_health=100; //MAXHEALTH;
int initial_bullets=50;

void VerifFavoritWeapon (player_t *player);

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
// Called by P_SpawnPlayer when PST_REBORN.
void G_PlayerReborn (int player)
{
    player_t*   p;
    int         i;
    uint16_t    frags[MAXPLAYERS];
    uint16_t    addfrags;
    int         killcount;
    int         itemcount;
    int         secretcount;

    //from Boris
    int         skincolor;
    char        favoritweapon[NUMWEAPONS];
    uint16_t    gf; // originalweaponswitch, autoaim
    int         skin;   //Fab: keep same skin
#ifdef CLIENTPREDICTION2
    mobj_t      *spirit;
#endif
    bot_t*      bot;	//added by AC for acbot

    p = &players[player];
    memcpy (frags, p->frags, sizeof(frags));
    addfrags = p->addfrags;
    killcount = p->killcount;
    itemcount = p->itemcount;
    secretcount = p->secretcount;

    //from Boris
    skincolor = p->skincolor;
    memcpy (favoritweapon, p->favoritweapon, NUMWEAPONS);
    gf = p->GF_flags; // originalweaponswitch, autoaim
    skin = p->skin;
#ifdef CLIENTPREDICTION2
    spirit = p->spirit;
#endif
    bot = p->bot;	//added by AC for acbot

    memset (p, 0, sizeof(*p));

    memcpy (p->frags, frags, sizeof(p->frags));
    p->addfrags=addfrags;
    p->killcount = killcount;
    p->itemcount = itemcount;
    p->secretcount = secretcount;

    // save player config truth reborn
    p->skincolor = skincolor;
    p->GF_flags = gf & (GF_autoaim|GF_original_weapon);
    memcpy (p->favoritweapon,favoritweapon,NUMWEAPONS);
    p->skin = skin;
#ifdef CLIENTPREDICTION2
    p->spirit = spirit;
#endif
    p->bot = bot;	//added by AC for acbot

    p->GB_flags = (GB_usedown|GB_attackdown); // don't do anything immediately
    p->playerstate = PST_LIVE;
    p->health = initial_health;
    if( gamemode == heretic )
    {
        // wand start
        p->weaponinfo = wpnlev1info;
        p->readyweapon = p->pendingweapon = wp_goldwand;
        p->weaponowned[wp_staff] = true;
        p->weaponowned[wp_goldwand] = true;
        p->ammo[am_goldwand] = 50;
    }
    else
    {
        // pistol start
        p->weaponinfo = doomweaponinfo;
        p->readyweapon = p->pendingweapon = wp_pistol;
        p->weaponowned[wp_fist] = true;
        p->weaponowned[wp_pistol] = true;
        p->ammo[am_clip] = initial_bullets;
    }

    // Boris stuff
    if(! (gf & GF_original_weapon))
        VerifFavoritWeapon(p);
    //eof Boris

    for (i=0 ; i<NUMAMMO ; i++)
        p->maxammo[i] = maxammo[i];
}


//
// G_Player_SpawnSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot because something is occupying it
// Generate spawn fog.
// Spawn player at the spot.
//
//  playernum : the player to check and spawn
//  spot : the level map spawn spot
// Return true when spawn spot is clear.
boolean  G_Player_SpawnSpot( int playernum, mapthing_t* spot )
{
    fixed_t       x, y;
    subsector_t*  ss;
    sector_t *    ssec;
    mobj_t*       mo;
    player_t *    player = & players[playernum];

    // added 25-4-98 : maybe there is no player start
    if(!spot || spot->type<0)   goto failexit;
    if( ! player )   goto failexit;

    // [WDJ] kill bob momentum or player will keep bobbing at spawn spot
    player->bob_momx = player->bob_momy = 0;
    player->aiming = 0;  // reset freelook 

    // The spawn spot location
    x = spot->x << FRACBITS;
    y = spot->y << FRACBITS;

#if 0
    // First spawn with no checks, is only kept because it is in Doom.
    // This does not work after any player moves an inch,
    // such as joining a network game.
    // Using static tstobj, tests now can be done without player->mo.
    if ((gametic == levelstarttic) && (player->mo == NULL))
    {
        int i;
        // first spawn of level, before corpses
        // Not all player[i].mo are init yet, see P_SetupLevel().
        for (i=0 ; i<playernum ; i++)
        {
            // Check if another player is on this spot.
            // added 15-1-98 check if player is in game (mistake from id)
            if (playeringame[i]
                && players[i].mo->x == x
                && players[i].mo->y == y)
                goto failexit;
        }
        // No fog, and no spawn sound.
        goto silent_spawn;
    }
#endif   

    ss = R_PointInSubsector (x,y);
    ssec = ss->sector;

    // check for respawn in team-sector.
    // Dogs do not know how to play teams.
    if(ssec->teamstartsec)
    {
        if(cv_teamplay.EV == 1)
        {
            // color
            if(player->skincolor!=(ssec->teamstartsec-1)) // -1 because wanted to know when it is set
                goto failexit;
        }
        else
        if(cv_teamplay.EV == 2)
        {
            // skins
            if(player->skin!=(ssec->teamstartsec-1)) // -1 because wanted to know when it is set
                goto failexit;
        }
    }
   
    // [WDJ] Test spawn spot for any player or blocking object.
    // Use tstobj, so do not need player mobj.  There may not be one.
    if( ! test_spot_unoccupied( MT_PLAYER, x, y, ssec->floorheight ) )
        goto failexit;
   
    // Spawn Spot accepted. Start spawn process.

    // If there is no corpse, there is no respawn fog nor sound.
    if( player->mo == NULL )
        goto silent_spawn;

    // Flush an old corpse from queue, if needed.
    if (bodyqueslot >= BODYQUESIZE)
        P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]);
    // Put player mobj in the body queue.
    bodyque[bodyqueslot%BODYQUESIZE] = player->mo;
    bodyqueslot++;

    // spawn a teleport fog
    // [WDJ] Note prboom fix doc (cph 2001/04/05), of Vanilla Doom bug found by Ville Vuorinen.
    // Signed mapthing_t angle would create table lookup using negative index.
    // Current prboom emulates buggy code that accesses outside of finecosine,
    // finecosine[-4096] -> finetangent[2048].
    // This code fixes the bug.
    // Unsigned fine angle worked, but better to use angle_t conversion too,
    // which is done by wad_to_angle returning unsigned angle_t.
    int angf = ANGLE_TO_FINE( wad_to_angle(spot->angle) );

    mo = P_SpawnMobj (x+20*finecosine[angf], y+20*finesine[angf],
                      ssec->floorheight, MT_TFOG);

    //added:16-01-98:consoleplayer -> displayplayer (hear snds from viewpt)
    // removed 9-12-98: why not ????
    if ( displayplayer_ptr->viewz != 1 )
        S_StartObjSound(mo, sfx_telept);  // don't start sound on first frame

silent_spawn:
    // Spawn the player at the spawn spot.
    P_SpawnPlayer (spot, playernum);
    return true;

failexit:
    return false;
}


//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
// Return true when spawned.
boolean G_DeathMatchSpawnPlayer (int playernum)
{
    int  i,j,n;

    if( !numdmstarts )
    {
        I_SoftError("No deathmatch start in this map!");
        return false;
    }

    if( EV_legacy < 123 )
        n=20;  // Doom, Boom
    else
        n=64;

    // Random select a deathmatch spot.  Try n times for an unoccupied one.
    for (j=0 ; j<n ; j++)
    {
        i = PP_Random(pr_dmspawn) % numdmstarts;
        if( G_Player_SpawnSpot( playernum, deathmatchstarts[i]) )
            return true;
    }

    if(demoversion<113)
    {
        // Doom method of last recourse.
        // no good spot, so the player will probably get stuck
        P_SpawnPlayer (playerstarts[playernum], playernum);
        return true;
    }

    // [WDJ] Spawn at random offsets from the last random deathmatch spawn location.
    // This allows more players than spawn spots.
    if( deathmatchstarts[i] )
    {
        if( scatter_spawn( MT_PLAYER, playernum, deathmatchstarts[i] )  )
            return true;
    }

    return false;
}

// Will always spawn the player somewhere.
void G_CoopSpawnPlayer (int playernum)
{
    mapthing_t * coop_spawn = playerstarts[playernum];
    int i;

    // Check for the COOP player spot unoccupied.
    if( G_Player_SpawnSpot(playernum, coop_spawn) )
        return;

    // Try to spawn at one of the other players spots.
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if( G_Player_SpawnSpot( playernum, playerstarts[i]) )
            return;
    }

    if(demoversion<113)
    {
        // Doom method of last recourse.
        P_SpawnPlayer (coop_spawn, playernum);
        return;
    }
   
    // [WDJ] Spawn at random offsets from the coop_spawn location.
    // This allows more players than spawn spots.
    if( coop_spawn == NULL )
        coop_spawn = playerstarts[0];  // first player start should always exist

    if( coop_spawn )
    {
        if( scatter_spawn( MT_PLAYER, playernum, coop_spawn )  )
            return;
    }

    // Try to use a deathmatch spot.
    // No message about deathmatch starts in coop mode.
    if( numdmstarts && ( ! deathmatch ) )
    {
        if( G_DeathMatchSpawnPlayer( playernum )  )
            return;
    }

    // Probably will spawn within someone.
    P_SpawnPlayer (coop_spawn, playernum);
}

//
// G_DoReborn
//
// Called from: P_SetupLevel, G_Ticker
void G_DoReborn (int playernum)
{
    player_t*  player = &players[playernum];

    // boris comment : this test is like 'single player game'
    //                 all this kind of hiden variable must be removed
    if( (! multiplayer) && (! deathmatch) )
    {
        // [Arcade] In Single Level mode a death ends the run.  The player
        // still watches the corpse as usual -- this is the "use" press that
        // would normally retry the level (P_DeathThink sets PST_REBORN), and
        // it returns to the Single Level menu instead.  The run has already
        // been voided by HS_Player_Died, so reloading would only hand out a
        // silent fresh attempt at the same map.
        //
        // Deferred through gameaction rather than calling
        // M_SingleLevel_Finished() from here: this runs inside G_Ticker's
        // reborn loop, above the point where tearing the level down is safe.
        // G_Ticker dispatches gameaction a few lines later, which is the same
        // route an ordinary level exit already takes to that function.
        //
        // Not during demo playback: a record demo can contain a death, and
        // replaying one must not tear down the game around it.  That covers
        // both a demo watched from this menu and an attract demo playing
        // while the player sits on the Single Level page, where
        // single_level_mode is still set.
        if( single_level_mode && ! demoplayback )
        {
            gameaction = ga_worlddone;
            return;
        }

        // reload the level from scratch
        G_DoLoadLevel (true);
    }
    else
    {
        // respawn at the start

        // first dissasociate the corpse
        if(player->mo)
        {
            player->mo->player = NULL;
            player->mo->flags2 &= ~MF2_DONTDRAW;
        }
        // spawn at random spot if in death match
        if( deathmatch )
        {
            if(G_DeathMatchSpawnPlayer (playernum))
               return;
            // use coop spots too if deathmatch spots occupied
        }

        G_CoopSpawnPlayer (playernum);
    }
}

void G_AddPlayer( int playernum )
{
    player_t *p=&players[playernum];

    p->playerstate = PST_REBORN;
    memset(p->inventory, 0, sizeof( p->inventory ));
    p->inventorySlotNum = 0;
    p->inv_ptr = 0;
    p->st_curpos = 0;
    p->st_inventoryTics = 0;

    if( gamemode == heretic )
        p->weaponinfo = wpnlev1info;
    else
        p->weaponinfo = doomweaponinfo;
}

// [WDJ] Par times can now be modified.
// DOOM Par Times
int pars[4][10] =
{
    {0},
    {0,30,75,120,90,165,180,180,30,165},
    {0,90,90,90,120,90,360,240,30,170},
    {0,90,45,90,150,90,90,165,30,135}
};

// DOOM II Par Times
int cpars[32] =
{
    30,90,120,120,90,150,120,120,270,90,        //  1-10
    210,150,150,150,210,150,420,150,210,150,    // 11-20
    240,150,180,150,150,300,330,420,300,180,    // 21-30
    120,30                                      // 31-32
};


//
// G_DoCompleted
//
boolean         secretexit;

void G_ExitLevel (void)
{
    if( gamestate==GS_LEVEL )
    {
        secretexit = false;
        gameaction = ga_completed;
    }
}

// Here's for the german edition.
void G_SecretExitLevel (void)
{
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ( (gamemode == doom2_commercial)
      && ( ! VALID_LUMP( W_CheckNumForName("map31") ) ))
        secretexit = false;
    else
        secretexit = true;
    gameaction = ga_completed;
}

void G_DoCompleted (void)
{
    int  i;

    gameaction = ga_nothing;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
            G_PlayerFinishLevel (i);        // take away cards and stuff
    }

    if (automapactive)
        AM_Stop ();

    automapactive = false;
}

#ifdef ENABLE_UMAPINFO
// [MB] 2023-01-22: Support for UMAPINFO added
// Returns true if default setup should be skipped
static
boolean G_Do_umapinfo (void)
{
    boolean result = false;

    wminfo.epsd_next   = wminfo.epsd;
    wminfo.lev_next    = wminfo.lev_prev + 1;  // adv to next lev
    wminfo.umapinfo_done = game_umapinfo;  // for next intermission
    wminfo.umapinfo_next = NULL;

    if (game_umapinfo)
    {
        const char * mapname = NULL;

        // Keys nextmap and nextsecret overrides default setup
        if (secretexit && game_umapinfo->nextsecret)
        {
            mapname = game_umapinfo->nextsecret;
            int i;
            for (i = 0; MAXPLAYERS > i; ++i)
                players[i].GF_flags |= GF_didsecret;
        }
        else if (game_umapinfo->nextmap)
            mapname = game_umapinfo->nextmap;

        if (mapname)
        {
            result = true;  // UMAPINFO overrides default setup

            {
                byte epinum = 0;
                byte mapnum = 0;

                UMI_Parse_mapname(mapname, &epinum, &mapnum);
                GenPrintf(EMSG_info, "UMAPINFO: Do Episode %d, Map %d\n", epinum, mapnum);
                // wminfo is 0 based
                wminfo.epsd_next = ((int)epinum) - 1;
                wminfo.lev_next = ((int)mapnum) - 1;
            }

            if (wminfo.epsd_next != wminfo.epsd)
            {
                // Jump to different episode
                int i;
                for (i = 0; MAXPLAYERS > i; ++i)
                    players[i].GF_flags &= ~(GF_didsecret);
            }
        }

        // Explicitly check for endgame too.
        // UMAPINFO may contain nonsense (example from 2022ado.wad map E5M8):
        //     next    = "E5M8"
        //     endgame = true
        // Give endgame priority (and do not skip default setup) in such cases
        if ( result
             && (game_umapinfo->flags & (UMA_endgame_enabled | UMA_endbunny | UMA_endcast) ) )
        {
            result = false;
        }
   }

    return result;
}
#endif


void G_Start_Intermission( void )
{
    int  i;

    finale_after_intermission = false;

#ifdef ENABLE_UMAPINFO   
    // [MB] 2023-01-22: Moved to beginning for G_DoUMapInfo()
    // wminfo is 0 based
    wminfo.epsd     = ((int)gameepisode) - 1;
    wminfo.lev_prev = ((int)gamemap) - 1;

    // [MB] 2023-01-22: Support for UMAPINFO added
    {
        boolean skip = G_Do_umapinfo();

        if (skip)
            goto beyond_std_setup;
    }
#endif

    if (gamemode != doom2_commercial)
    {
        switch(gamemap)
        {
          case 8:
            //BP add comment : no intermission screen
            if( deathmatch )
                wminfo.lev_next = 0;
            else
            {
                // also for heretic
                // [Arcade] Show the intermission first and start the finale
                // from G_NextLevel, the way Doom II already handles MAP30.
                // CL_Reset moves there with it.
                finale_after_intermission = true;
            }
            break; // [WDJ] 4/11/2012  map 8 is not secret level, and prboom and boom do not fall thru here.
          case 9:
            for (i=0 ; i<MAXPLAYERS ; i++)
                players[i].GF_flags |= GF_didsecret;
            break;
        }
    }
    //DarkWolf95: September 11, 2004: More chex stuff
    if (gamemode == chexquest1)
    {
        if( !modifiedgame && gamemap == 5 )  // original chexquest ends at E1M5
        {
            if( deathmatch )
                wminfo.lev_next = 0;
            else
                finale_after_intermission = true;   // [Arcade] as for map 8
        }
    }

    if(!dedicated)
      wminfo.didsecret = (consoleplayer_ptr->GF_flags & GF_didsecret)? 1: 0;

#ifdef ENABLE_UMAPINFO
#else
    // wminfo is 0 based
    wminfo.epsd = ((int)gameepisode) - 1;
    wminfo.lev_prev = ((int)gamemap) -1;
#endif

    // go to next level
    // wminfo.lev_next is 0 biased, unlike gamemap
    wminfo.lev_next = gamemap;

    // overwrite next level in some cases
    if (gamemode == doom2_commercial)
    {
        if (secretexit)
        {
            switch(gamemap)
            {
              case 15 : wminfo.lev_next = 30; break;
              case 31 : wminfo.lev_next = 31; break;
              default : wminfo.lev_next = 15;break;
            }
        }
        else
        {
            switch(gamemap)
            {
              case 31:
              case 32: wminfo.lev_next = 15; break;
              default: wminfo.lev_next = gamemap;
            }
        }
    }
    else
    if (gamemode == heretic)
    {
        static const int afterSecret[5] = { 7, 5, 5, 5, 4 };
        if (secretexit)
            wminfo.lev_next = 8;    // go to secret level
        else if (gamemap == 9)
            wminfo.lev_next = afterSecret[gameepisode-1]-1;
    }
    else
    {
        if (secretexit)
            wminfo.lev_next = 8;    // go to secret level
        else if (gamemap == 9)
        {
            // returning from secret level
            switch (gameepisode)
            {
              case 1 :  wminfo.lev_next = 3; break;
              case 2 :  wminfo.lev_next = 5; break;
              case 3 :  wminfo.lev_next = 6; break;
              case 4 :  wminfo.lev_next = 2; break;
              default : wminfo.lev_next = 0; break;
            }
        }
        else
        {
            if (gamemap == 8)
                wminfo.lev_next = 0; // wrap around in deathmatch
        }
    }

#ifdef ENABLE_UMAPINFO   
beyond_std_setup:

    // [MB] 2023-01-22: Support for UMAPINFO added
    {
        // wminfo is 0 based, umapinfo is 1 based
        int epinum = wminfo.epsd_next + 1;
        int mapnum = wminfo.lev_next + 1;

        if (epinum >= 0 && epinum <= 255u && mapnum > 0 && mapnum <= 255u)
        {
            wminfo.umapinfo_next = UMI_Lookup_umapinfo( (byte) epinum, (byte) mapnum );
        }
    }
#endif

    wminfo.maxkills = totalkills;
    wminfo.maxitems = totalitems;
    wminfo.maxsecret = totalsecret;
    wminfo.maxfrags = 0;
    if( info_partime != -1)
        wminfo.partime = TICRATE*info_partime;
    else if ( gamemode == doom2_commercial )
        wminfo.partime = TICRATE*cpars[gamemap-1];
    else
        wminfo.partime = TICRATE*pars[gameepisode][gamemap];
    wminfo.pnum = consoleplayer;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        wminfo.plyr[i].in = playeringame[i];
        wminfo.plyr[i].skills = players[i].killcount;
        wminfo.plyr[i].sitems = players[i].itemcount;
        wminfo.plyr[i].ssecret = players[i].secretcount;
        wminfo.plyr[i].stime = leveltime;
        memcpy (wminfo.plyr[i].frags, players[i].frags
                , sizeof(wminfo.plyr[i].frags));
        wminfo.plyr[i].addfrags = players[i].addfrags;
    }

    if (statcopy)
        memcpy (statcopy, &wminfo, sizeof(wminfo));

    gamestate = GS_INTERMISSION;

    WI_Start (&wminfo);
}


//
// G_NextLevel (WorldDone)
//
// init next level or go to the final scene
// called by end of intermision screen (wi_stuff)
void G_NextLevel (void)
{
    gameaction = ga_worlddone;
    if (secretexit)
        consoleplayer_ptr->GF_flags |= GF_didsecret;

    // [Arcade] Single Level mode plays one map and returns to its own menu
    // from G_DoWorldDone, so it must never be diverted into a finale: those
    // do not come back on their own (the Doom 2 cast call loops for ever,
    // and the text screens wait for a keypress), and a player who picked one
    // level should not end up watching the credits.  Leaving gameaction as
    // ga_worlddone is all that is needed.  Not during demo playback, where
    // a record demo replays through real level exits.
    if( single_level_mode && ! demoplayback )
    {
        finale_after_intermission = false;
        return;
    }

    // [Arcade] The episode's last level deferred its finale until after the
    // intermission (see G_Start_Intermission).  This is the Doom 1, Heretic
    // and Chex equivalent of the doom2_commercial block below.
    if( finale_after_intermission )
    {
        finale_after_intermission = false;
        CL_Reset();        // end of game, disconnect from server
        gameaction = ga_nothing;
#ifdef ENABLE_UMAPINFO
        F_StartFinale (secretexit);
#else
        F_StartFinale ();
#endif
        return;
    }

#ifdef ENABLE_UMAPINFO   
    // [MB] 2023-04-01: Support for UMAPINFO added
    if( game_umapinfo )
    {
        // [WDJ] Simplified, does the same thing.
        // If endgame_enabled, it is != unchanged.
        // Do not block endbunny or endcast, when 'endgame_disabled'.
        if (game_umapinfo->flags & (UMA_endbunny | UMA_endcast | UMA_endgame_enabled) )
        {
            CL_Reset (); // end of game disconnect from server
            gameaction = ga_nothing;
            F_StartFinale (secretexit);  // [MB] 2023-03-04: Parameter added
        }
    }
#endif

    if( gamemode == doom2_commercial )
    {
        if( deathmatch )
        {
            if( gamemap == 30 )
                wminfo.lev_next = 0; // wrap around in deathmatch
        }
        else
        {
            switch (gamemap)
            {
            case 15:
            case 31:
                if (!secretexit)
                    break;
            case 6:
            case 11:
            case 20:
            case 30:
                if( gamemap == 30 )
                    CL_Reset(); // end of game disconnect from server
                gameaction = ga_nothing;
#ifdef ENABLE_UMAPINFO	       
                F_StartFinale (secretexit);  // [MB] 2023-03-04: Parameter added
#else
                F_StartFinale ();
#endif
                break;
            }
        }
    }
}

void G_DoWorldDone (void)
{
    // [Arcade] Single Level mode plays exactly one map.  Everything below is
    // about choosing and loading the *next* map, so returning here is all
    // that is needed.  Two routes arrive:
    //   - a finished level, where the intermission has been shown and
    //     HS_LevelExit has already scored the run;
    //   - a death, where G_DoReborn redirects the retry here instead of
    //     reloading the map (HS_Player_Died has voided the run).
    //
    // Not applied during demo playback: a record demo replays through real
    // level exits, and cutting it short here would truncate it.
    if( single_level_mode && ! demoplayback )
    {
        M_SingleLevel_Finished();
        return;
    }

    if( demoversion<129 )
    {
        gamemap = wminfo.lev_next+1;
        G_DoLoadLevel (true);
    }
    else
    {
        // not in demo because demo have the mapcommand on it
#ifdef ENABLE_UMAPINFO
        // [MB] 2023-03-26: Replaced gameepisode with wminfo.epsd_next+1
        //                  Crossing episodes is possible with UMAPINFO
 // [WDJ] If gameepisode is not correct, then why not fix it to be correct.  Why leave it with the wrong value?
#endif
        if(server && !demoplayback) 
        {
            if( ! deathmatch )
            {
                // don't reset player between maps
#ifdef ENABLE_UMAPINFO
                COM_BufAddText (va("map \"%s\" -noresetplayers\n",G_BuildMapName(wminfo.epsd_next+1, wminfo.lev_next+1)));
#else
                COM_BufAddText (va("map \"%s\" -noresetplayers\n",G_BuildMapName(gameepisode, wminfo.lev_next+1)));
#endif
            }
            else
            {
                // resetplayer in deathmatch for more equality
#ifdef ENABLE_UMAPINFO
                COM_BufAddText (va("map \"%s\"\n",G_BuildMapName(wminfo.epsd_next+1, wminfo.lev_next+1)));
#else
                COM_BufAddText (va("map \"%s\"\n",G_BuildMapName(gameepisode, wminfo.lev_next+1)));
#endif
            }
        }
    }
    
    gameaction = ga_nothing;
}


// compose menu message from strings
static
void compose_message( const char * str1, const char * str2 )
{
    char msgtemp[128];
    if( str2 == NULL )  str2 = "";
    sprintf( msgtemp, "%s %s\n\nPress ESC\n", str1, str2 );
    M_SimpleMessage ( msgtemp );
}


extern char  savegamedir[SAVESTRINGSIZE];
char  savegamename[MAX_WADPATH];

// Must be able to handle 99 savegame slots, even when
// not SAVEGAME99, so net game saves are universally accepted.
void G_Savegame_Name( /*OUT*/ char * namebuf, /*IN*/ int slot )
{
#ifdef SAVEGAMEDIR
    sprintf(namebuf, savegamename, savegamedir, slot);
#else
    sprintf(namebuf, savegamename, slot);
#endif
#ifdef SMIF_PC_DOS
    if( slot > 9 )
    {
        // shorten name to 8 char
        int ln = strlen( namebuf );
        memmove( &namebuf[ln-4], &namebuf[ln-3], 4 );
    }
#endif
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
// Called from menu M_LoadSelect from M_Responder,
// and from D_Main code for -loadgame command line switch.
void G_Load_Game (int slot)
{
    // [WDJ] will handle 99 slots
    COM_BufAddText(va("load %d\n",slot));
    // net command call to G_DoLoadGame
}

// Called from network command, sent from G_LoadGame
// Reads the save game file.
void G_DoLoadGame (int slot)
{
    char        savename[255];
    savegame_info_t   sginfo;  // read header info

    G_Savegame_Name( savename, slot );

    if( P_Savegame_Readfile( savename ) < 0 )  goto cannot_read_file;
    // file is open and savebuffer allocated

    if( ! P_Savegame_Read_header( &sginfo, 0 ) )  goto load_header_failed;
    if( ! sginfo.have_game )  goto wrong_game;
    if( ! sginfo.have_wad )  goto wrong_wad;

    D_DisableDemo();  // turn off demos and keeps them off

    //added:27-02-98: reset the game version
    G_setup_VERSION();

    paused        = 0;
    automapactive = false;

    // dearchive all the modifications
    P_Savegame_Load_game(); // read game data in savebuffer, defer error test
    if( P_Savegame_Closefile( 0 ) < 0 )  goto load_failed;
    // savegame buffer deallocated, and file closed

#ifdef BASETIC_DEMOSYNC
# if 0
    // [WDJ] PrBoom savegames indirectly save the gametic.
    // I see no need for a demo fix to affect savegames.
    // killough 11/98: load revenant tracer state
    basetic = gametic - *save_p++;
# endif
#endif

    gameaction = ga_nothing;
    gamestate = GS_LEVEL;

    displayplayer = consoleplayer;
    displayplayer_ptr = consoleplayer_ptr;

    // done
    multiplayer = playeringame[1];
    if(playeringame[1] && !netgame)
        CV_SetValue(&cv_splitscreen,1);

    if (setsizeneeded)
        R_ExecuteSetViewSize ();

    // draw the pattern into the back screen
    R_FillBackScreen ();
    CON_ToggleOff ();
    return;

cannot_read_file:
    CONS_Printf ("Couldn't read file %s", savename);
    goto failed_exit;

load_header_failed:
    compose_message( sginfo.msg, NULL );
    goto failed_exit;

wrong_game:
    compose_message( "savegame requires game:", sginfo.game );
    goto failed_exit;

wrong_wad:
    compose_message( "savegame requires wad:", sginfo.wad );
    goto failed_exit;

load_failed:
    M_SimpleMessage("savegame file corrupted\n\nPress ESC\n" );
    Command_ExitGame_f();
failed_exit:
    P_Savegame_Error_Closefile();  // to dealloate buffer
    // were not playing, but server got started by sending load message
    if( gamestate == GS_WAITINGPLAYERS )
    {
        // [WDJ] fix ALLREADYPLAYING message, so that still not playing
        Command_ExitGame_f();
    }
    return;
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
// Called from menu M_DoSave from M_Responder.
void G_Save_Game ( int   slot, const char* description )
{
    // Solo player has server, net player without server cannot save.
    if (server)
    {
        // [WDJ] will handle 99 slots
        COM_BufAddText(va("save %d \"%s\"\n",slot,description));
        // Net command call to G_DoSaveGame
    }
}

// Called from network command sent from G_SaveGame.
// Writes the save game file.
void G_DoSaveGame (int   savegameslot, const char* savedescription)
{
    char        savename[256];

    gameaction = ga_nothing;

    G_Savegame_Name( savename, savegameslot );

    gameaction = ga_nothing;

    if( P_Savegame_Writefile( savename ) < 0 )  return;
    
    P_Savegame_Write_header( savedescription, 0 );
    P_Savegame_Save_game();  // Write game data to savegame buffer.
   
    if( P_Savegame_Closefile( 1 ) < 0 )  return;

    gameaction = ga_nothing;

    consoleplayer_ptr->message = GGSAVED;

    // draw the pattern into the back screen
    R_FillBackScreen ();
    ST_Drawer( 1 );	// [WDJ] refresh status background without global flags
}


//
// G_InitNew
//  Can be called by the startup code or the menu task,
//  consoleplayer, displayplayer, playeringame[] should be set.
//
// Boris comment : single player start game
// Called by SF_StartSkill, M_ChooseSkill, M_VerifyNightmare
// Called by cht_Responder on clev, CheatWarpFunc
void G_DeferedInitNew (skill_e skill, const char* mapname, boolean StartSplitScreenGame)
{
    paused = 0;
    
    if( demoplayback )
        COM_BufAddText ("stopdemo\n");  // invokes G_CheckDemoStatus

    D_DisableDemo();  // turn off demos and keeps them off

    G_setup_VERSION(); // [WDJ] should be after demo is stopped

    // this leave the actual game if needed
    SV_StartSinglePlayerServer();
    
    // Setup before start of game.
    COM_BufAddText (va("splitscreen %d;deathmatch 0;fastmonsters 0;"
                       "respawnmonsters 0;timelimit 0;fraglimit 0\n",
                       StartSplitScreenGame));

    COM_BufAddText (va("map \"%s\" -skill %d -monsters 1\n",mapname,skill+1));
}

//
// This is the map command interpretation something like Command_Map_f
//
// called at : map cmd execution, doloadgame, doplaydemo
void G_InitNew (skill_e skill, const char* mapname, boolean resetplayer)
{
    //added:27-02-98: disable selected features for compatibility with
    //                older demos, plus reset new features as default
    if(!G_Downgrade (demoversion))
    {
        CONS_Printf("Cannot Downgrade engine\n");
        CL_Reset();
        D_StartTitle();
        return;
    }

    if (paused)
    {
        paused = 0;
        S_ResumeSound ();
    }

    if (skill > sk_nightmare)
        skill = sk_nightmare;

    M_ClearRandom ();

    if( server && skill == sk_nightmare )
    {
        // NETVAR, not saved        
#if 1       
        CV_SetParam(&cv_respawnmonsters,1);
        CV_SetParam(&cv_fastmonsters,1);
#else
        CV_SetValue(&cv_respawnmonsters,1);
        CV_SetValue(&cv_fastmonsters,1);
#endif
    }

    // [Arcade] Re-derive the deathmatch mode settings at every game start.
    // Deathmatch_OnChange is what turns cv_deathmatch into weapon_persist
    // and cv_itemrespawn, but as an OnChange it only runs when the cvar
    // actually *changes* -- CV_Set returns early on an unchanged value
    // (command.c).  Meanwhile HS_Apply_Ranked_Ruleset pins cv_itemrespawn
    // back to 0 on the way out to the attract screen.  So the first DM game
    // after boot had item respawn and every later one at the same setting
    // silently did not.  Calling it here makes the mapping authoritative.
    Deathmatch_OnChange();

    // for internal maps only
    if (FIL_CheckExtension(mapname))
    {
        // external map file
        dl_strncpy(game_map_filename, mapname, MAX_WADPATH);
        // dummy values, to be set by P_SetupLevel.
        gameepisode = 1;
        gamemap = 1;
    }
    else
    {
        // internal game map
        // well this  check is useless because it is done before (d_netcmd.c::command_map_f)
        // but in case of for demos....
        if( ! VALID_LUMP( W_CheckNumForName(mapname) ) )
        {
            CONS_Printf("\2Internal game map '%s' not found\n"
                        "(use .wad extension for external maps)\n",mapname);
            Command_ExitGame_f();
            return;
        }

        game_map_filename[0] = 0;             // means not an external wad file
        if (gamemode==doom2_commercial)       //doom2
        {
            gamemap = atoi(mapname+3);  // get xx out of MAPxx
            gameepisode = 1;
        }
        else
        {
            gamemap = mapname[3]-'0';           // ExMy
            gameepisode = mapname[1]-'0';
        }
    }

    gameskill      = skill;
    playerdeadview = false;
    automapactive  = false;

    G_DoLoadLevel (resetplayer);
}


// Sets defaults according to current master EN_
// Do not call after a Demo has set these settings.
static
void G_gamemode_EN_defaults( void )
{
    // Fixes and buggy.
    // Heretic never fixed this, but PrBoom did.  Default to fixed.
    EN_skull_bounce_fix = 1;  // Off only for old demos, incl Legacy demos.
    EN_skull_bounce_floor = 1;
    EN_catch_respawn_0 = 1;
    // Boom
    EN_pushers = EN_boom;
    EN_doorlight = EN_boom;
    EN_invul_god = EN_boom;
    EN_boom_physics = EN_boom;
    EN_boom_floor = EN_boom | EN_heretic;
    EN_blazing_double_sound = 0;
    EN_vile_revive_bug = 0;
    EN_sleeping_sarg_bug = 0;
    EN_skull_limit = 0;
    EN_old_pain_spawn = 0;
    EN_doom_movestep_bug = 0;
    // MBF
    EN_mbf_telefrag = EN_mbf | EN_heretic;
#ifdef MBF21
    EN_ledgeblock = EN_boom | EN_mbf21;  // default ON
    EN_friendlyspawn = EN_mbf21;  // or PrBoom, default ON
    EN_voodooscroller = EN_mbf21;
    EN_reserved_line_flag = EN_mbf21;  // default ON
    EN_mbf21_action = EN_mbf21;
#endif
}


// [WDJ] Set the gamemode, and all EN_ that are dependent upon it.
// Done here to be near G_Downgrade.
void G_set_gamemode( byte new_gamemode )
{
    gamemode = new_gamemode;
    max_num_players = MAXPLAYERS;  // dependent upon demo
    EV_legacy = VERSION;  // current DoomLegacy version
    // Legacy defaults.
    EN_doom_etc = 1;
    EN_boom = 1;
    EN_mbf = 1;
#ifdef MBF21
    EN_mbf21 = 0; // MBF21 extension, until detected.
#endif
    // Doom and doom-like defaults.
    EN_heretic = EN_hexen = EN_strife = 0;
    EN_heretic_hexen = 0;
    EN_inventory = 0;
    // Hexen and Strife are setup even though not implemented yet,
    // as placeholders and to prevent bad assumptions.
    switch( gamemode )
    {
     case heretic:
      EN_heretic = 1;
      goto not_doom;
     case hexen:
      EN_hexen = 1;
      goto not_doom;
     case strife:
      EN_strife = 1;
      goto not_doom;
     case chexquest1:  // is doom
     default:
      break;
    }
    
    // Doom
#ifdef MBF21
    if( all_detect & BDTC_MBF21 )
    {
        EN_mbf21 = 1;
    }
#endif
    goto finish;

not_doom:
    EN_heretic_hexen = EN_heretic || EN_hexen;
    EN_doom_etc = EN_mbf = EN_boom = 0;
    EN_inventory = 1;

finish:
    G_gamemode_EN_defaults();
    return;
}


// Sets defaults according to current master EN_
// Do not call after a Demo has set these settings.
// Called after getting demoversion, EN_doom, etc, but before setting
// individual demo settings.
static
void G_demo_defaults( void )
{
    // For DoomLegacy demos, generally, after version 1.44 when DoomLegacy got a capability,
    // it got a flag in the demo header.  Default them to off.
    friction_model = FR_orig;
    monster_infight = INFT_infight;  // Default is to infight, DEH can turn it off.
    voodoo_mode = VM_vanilla;
    cv_viewheight.EV = 41; // vanilla viewheight
    cv_solidcorpse.EV = 0;
    // [Arcade] Off for playback whatever the live default is.  Stock IWAD
    // demos contain player deaths -- the Ultimate Doom E4M2 one does -- and
    // were recorded without weapon dropping, so replaying them with it on
    // spawns a weapon that is not in the recording and desyncs from there.
    cv_fragsweaponfalling.EV = 0;
    cv_instadeath.EV = 0;  // Die
    cv_monstergravity.EV = 0;
    cv_monbehavior.EV = 0;  // Vanilla
    cv_monsterfriction.EV = 0; // Vanilla
    EN_skull_bounce_fix = 0;  // Vanilla and DoomLegacy < 1.47
    EN_catch_respawn_0 = 0;

    // Boom
    cv_rndsoundpitch.EV = EN_boom;  // normal in Boom, calls M_Random
    EN_pushers = EN_boom;
    // introduced Boom 2.00 without demo flag
    EN_doom_movestep_bug = EN_doom_etc && ! EN_boom;
    byte boom_200 = EN_boom && (demoversion >= 200);  // 0=Vanilla, 1=Boom
    EN_variable_friction = boom_200;
    EN_boom_floor = boom_200;
    // introduced Boom 2.01 without demo flag
    byte boom_201 = EN_boom && (demoversion >= 201);  // 0=Vanilla, 1=Boom
    EN_boom_physics = boom_201;
    EN_doorlight = boom_201;
    EN_invul_god = boom_201;
    cv_invul_skymap.EV = boom_201;
    cv_zerotags.EV = boom_201;  // 0=Vanilla, 1=Boom
    byte doom_etc_before_201 = EN_doom_etc && ! boom_201;  // fixed Boom 2.01
    EN_blazing_double_sound = doom_etc_before_201;
    EN_skull_limit = doom_etc_before_201;
    EN_old_pain_spawn = doom_etc_before_201;
    EN_vile_revive_bug = doom_etc_before_201;
    // introduced Boom 2.02 without demo flag
    cv_doorstuck.EV = EN_boom && (demoversion >= 202);  // Boom 2.02
    // introduced Boom 2.04
    EN_sleeping_sarg_bug = EN_doom_etc && (demoversion < 204);  // fixed Boom 2.04

    // MBF
    cv_mbf_dropoff.EV = EN_mbf;
    cv_mbf_falloff.EV = EN_mbf;
    cv_mbf_monster_avoid_hazard.EV = EN_mbf;
    cv_mbf_monster_backing.EV = EN_mbf;
    cv_mbf_pursuit.EV = EN_mbf;
    cv_mbf_staylift.EV = EN_mbf;
    cv_mbf_help_friend.EV = EN_mbf;
    cv_mbf_monkeys.EV = EN_mbf;
#ifdef DOGS
    cv_mbf_dogs.EV = EN_mbf;
    cv_mbf_dog_jumping.EV = EN_mbf;
#endif
#ifdef MBF21
    EN_ledgeblock = EN_boom | EN_mbf21;
    EN_friendlyspawn = EN_mbf21;  // or PrBoom
    EN_voodooscroller = EN_mbf21;
    EN_reserved_line_flag = EN_mbf21;
    EN_mbf21_action = EN_mbf21;
#endif
#ifdef MAPTHING_ADJUST
    cv_monster_health.EV = 0;
    cv_health_pickup.EV = 0;
    cv_armor_pickup.EV = 0;
    cv_ammo_pickup.EV = 0;
#endif
#ifdef ENABLE_TIRED_RUN
    cv_tired_run.EV = 0;
    cv_drown.EV = 0;
#endif
#ifdef MONSTER_VARY
    cv_monster_vary.EV = 0;
#endif
#ifdef ENABLE_TELE_CONTROL
    cv_tele_control.EV = 0;
#endif
#ifdef ENABLE_SLOW_REACT
    cv_slow_react.EV = 0;
#endif
#ifdef DOORDELAY_CONTROL
    delay_ticks_per_sec = 35; // default
#endif

    max_num_players = MAXPLAYERS;  // dependent upon demo
}


static
void G_restore_user_settings( void )
{
    // Force some restore to invoke CV_CALL functions.
    cv_monbehavior.EV = 255;  // infight
    cv_voodoo_mode.EV = 255;
#ifdef DOORDELAY_CONTROL
    cv_doordelay.EV = 255;
#endif
#ifdef MAPTHING_ADJUST_MASTER
    cv_mapthing_adjust_master.EV = 255;  // force update
#endif

    cv_mbf_distfriend.EV = ~cv_mbf_distfriend.value;  // forced mismatch

    // Restore all modifed cvar
    CV_Restore_User_Settings();  //  Set EV = value
}



//added:03-02-98:
//
//  'Downgrade' the game engine so that it is compatible with older demo
//   versions. This will probably get harder and harder with each new
//   'feature' that we add to the game. This will stay until it cannot
//   be done a 'clean' way, then we'll have to forget about old demos..
//
// demoversion is usually set before this is called
boolean G_Downgrade(int version)
{
    int i;

    if (verbose > 1)
    {
        GenPrintf(EMSG_info|EMSG_all,"Downgrade to version: %i\n", version);
    }

    if (version<109)
        return false;

    // always true now, might be false in the future, if couldn't
    // go backward and disable all the features...
    demoversion = version;

    if( version<130 )
    {
        mobjinfo[MT_BLOOD].radius = FIXINT(20);
        mobjinfo[MT_BLOOD].height = FIXINT(16);
        mobjinfo[MT_BLOOD].flags  = MF_NOBLOCKMAP;
    }
    else
    {
        mobjinfo[MT_BLOOD].radius = FIXINT(3);
        mobjinfo[MT_BLOOD].height = FIXINT(0);
        mobjinfo[MT_BLOOD].flags  = 0;
    }

    // smoke trails for skull head attack since v1.25
    if (version<125)
    {
        states[S_ROCKET].action = AI_NULL;

        states[S_SKULL_ATK3].action = AI_NULL;
        states[S_SKULL_ATK4].action = AI_NULL;
    }
    else
    {
        //activate rocket trails by default
        states[S_ROCKET].action     = AI_SmokeTrailer;

        // smoke trails behind the skull heads
        states[S_SKULL_ATK3].action = AI_SmokeTrailer;
        states[S_SKULL_ATK4].action = AI_SmokeTrailer;
    }

    if(version <= 109)
    {
        // disable rocket trails
        states[S_ROCKET].action = AI_NULL; //NULL like in Doom2 v1.9

        // Boris : for older demos, initialize the new skincolor value
        //         also disable the new preferred weapons order.
        for(i=0;i<4;i++)
        {
            players[i].skincolor = i % NUMSKINCOLORS;
            players[i].GF_flags |= GF_original_weapon;
        }//eof Boris
    }
   
    if(version <= 111 || version >= 200)
    {
        //added:16-02-98: make sure autoaim is used for older
        //                demos not using mouse aiming
        cv_allowautoaim.EV = 9;  // force autoaim
        for(i=0;i<MAXPLAYERS;i++)
            players[i].GF_flags |= GF_autoaim;
    }

    // PrBoom has this enabled by comp level.
    // It is only off for old Doom demos.
    EN_skull_bounce_floor = EN_skull_bounce_fix
       || ( version > 109 && version < 212 );

    //SoM: 3/17/2000: Demo compatability
    // EN_boom has been loaded from Boom demo compatiblity.
    if(gamemode == heretic)
    {
        EN_boom = 0;  // expected to be OFF
        EN_pushers = 0;
        EN_variable_friction = 1;  // Needed for ICE E2M4
        EN_sleeping_sarg_bug = 0;
    }
    else if(version < 129 || ! EN_boom )
    {
        // Boom demo_compatibility mode  (boom demo version < 200)
        EN_pushers = 0;
        EN_variable_friction = 0;
        EN_sleeping_sarg_bug = 1;
    }
    else if( version < 200 )
    {
        // Legacy
        // Flags loaded by (Boom, MBF, prboom) demos, but not others.
        EN_pushers = 1;	// of Boom 2.02
        EN_variable_friction = 1;  // of Boom 2.02
        EN_sleeping_sarg_bug = EV_legacy < 144;
    }

    if( !demoplayback || friction_model == FR_orig )
    {
        friction_model =
           (gamemode == heretic)? FR_heretic
         : (gamemode == hexen)? FR_hexen
         : (version <= 132)? FR_orig  // older legacy demos, and doom demos
         : (version <= 143)? FR_boom  // old legacy demos
         : (version > 200) ?
           (
              (version <= 202)? FR_boom  // boom 200, 201, 202
            : (version == 203)? FR_mbf
            : FR_prboom  // prboom
           )
         : FR_legacy;  // new model, default
    }

#if 0
    // EN_boom_invul_skymap, was not enabled for DoomLegacy before 1.47,
    // but does not affect demo sync.  Is a matter of preference.
    if( version < 147 )
        cv_invul_skymap.EV = 0;
#endif

#if 0
    // [WDJ]
    // TODO:
    // auto weapon change on pickup
    if( demoplayback )
    { 
        // values that are set by the demo
    }
    else
    {
    }
#endif

    DemoAdapt_p_user();  // local enables of p_user
    DemoAdapt_p_mobj();  // local enables of p_mobj
    DemoAdapt_p_enemy(); // local enables of p_enemy
    DemoAdapt_p_fab();   // local enables of p_fab
    DemoAdapt_p_floor(); // local enables of p_floor, TNT MAP30 fix
    DemoAdapt_p_map();   // local enables of p_map
    DemoAdapt_bots();    // local enables of bots
#ifdef MBF21
    DemoAdapt_p_inter();
#endif
    return true;
}

// Make it easy to setup the VERSION play, without mistakes.
void G_setup_VERSION( void )
{
#ifdef PARANOIA
    if( !EN_mbf )
    {
        GenPrintf( EMSG_warn, "Setup_VERSION: EN_mbf=0, possibly after demo\n" );
        G_set_gamemode( gamemode );  // restore EN set by gamemode
    }
#endif
    // Reset demoversion to normal
    EV_legacy = VERSION;
    G_Downgrade( VERSION );
}



#ifdef GAMESKILL_FLAGS
// Not enabled yet.
// Mostly have simple enables that cover these flags already, or
// the original test is just as simple as testing the flag.
uint16_t gameskill_flags;

// indexed by gameskill
uint16_t doom_gameskill_flags_table[5] =
{
    // [WDJ] Mostly, these are not implemented.  Here for reference.
    // sk_baby
    GSF_EASY_BRAIN_BOSS,
    // sk_easy
    GSF_EASY_BRAIN_BOSS,
    // sk_medium
    0,
    // sk_hard
    0,
    // sk_nightmare
    GSF_FAST_MONSTERS | GSF_INSTANT_REACTION,
};

uint16_t heretic_gameskill_flags_table[5] =
{
    // [WDJ] Mostly, these are not implemented.  Here for reference.
    // sk_baby
    GSF_AUTO_USE_HEALTH,
    // sk_easy
    0,
    // sk_medium
    0,
    // sk_hard
    0,
    // sk_nightmare
    GSF_FAST_MONSTERS | GSF_INSTANT_REACTION,
};


// Setup flags and enables based upon game and gameskill.
void G_setup_gameskill( void )
{
    gameskill_flags = (EN_heretic_hexen)? doom_gameskill_flags_table[ gameskill ]
      : heretic_gameskill_flags_table[ gameskill ];
}
#endif


//
// DEMO RECORDING
//

#define ZT_FWD          0x01
#define ZT_SIDE         0x02
#define ZT_ANGLE        0x04
#define ZT_BUTTONS      0x08
#define ZT_AIMING       0x10
#define ZT_CHAT         0x20    // no more used
#define ZT_EXTRADATA    0x40
#define DEMOMARKER      0x80    // demoend

// SERVER_ID appears in Demo 1.48
#if SERVER_PID < MAXPLAYERS
#  error "SERVER_PID must be > MAXPLAYERS"
#endif

ticcmd_t oldcmd[MAXPLAYERS];

// Only called when demoplayback.
static
void G_ReadDemoTiccmd (ticcmd_t* cmd, int playernum)
{
    if( (*demo_p == DEMOMARKER) || (demo_p > demoend) )
    {
        // end of demo data stream
        G_CheckDemoStatus ();
        return;
    }

    if( EN_demotic_109 )  // vanilla demo tic format
    {
        // Doom, Boom, MBF, prboom demo
        cmd->forwardmove = READCHAR(demo_p);
        cmd->sidemove = READCHAR(demo_p);
        if( EN_boom_longtics )
        {
            cmd->angleturn = READ16(demo_p);
        }
        else
        {
            cmd->angleturn = READBYTE(demo_p)<<8;
        }
        cmd->buttons = READBYTE(demo_p);
        // demo does not have
        cmd->aiming = 0;
    }
    else
    {
        // DoomLegacy advanced demos
        char ziptic=*demo_p++;  // bit flags for ZT_

        if(ziptic & ZT_FWD)
            oldcmd[playernum].forwardmove = READCHAR(demo_p);

        if(ziptic & ZT_SIDE)
            oldcmd[playernum].sidemove = READCHAR(demo_p);

        if(ziptic & ZT_ANGLE)
        {
            if(demoversion<125)
                oldcmd[playernum].angleturn = READBYTE(demo_p)<<8;
            else
                oldcmd[playernum].angleturn = READ16(demo_p);
        }

        if(ziptic & ZT_BUTTONS)
            oldcmd[playernum].buttons = READBYTE(demo_p);

        if(ziptic & ZT_AIMING)
        {
            if(demoversion<128)
                oldcmd[playernum].aiming = READCHAR(demo_p);
            else
                oldcmd[playernum].aiming = READ16(demo_p);
        }

        if(ziptic & ZT_CHAT)
            demo_p++;

        if(ziptic & ZT_EXTRADATA)
            ReadLmpExtraData(&demo_p,playernum);
#if 0
// only used to clear textcmd, which is now done in TryRunTics       
        else
            ReadLmpExtraData(0,playernum);
#endif

        memcpy(cmd,&(oldcmd[playernum]),sizeof(ticcmd_t));
    }
}


static
void G_WriteDemoTiccmd (ticcmd_t* cmd,int playernum)
{
    char ziptic=0;
    byte *ziptic_p;

    ziptic_p=demo_p++;  // the ziptic
                        // write at the end of this function

    if(cmd->forwardmove != oldcmd[playernum].forwardmove)
    {
        *demo_p++ = cmd->forwardmove;
        oldcmd[playernum].forwardmove = cmd->forwardmove;
        ziptic|=ZT_FWD;
    }

    if(cmd->sidemove != oldcmd[playernum].sidemove)
    {
        *demo_p++ = cmd->sidemove;
        oldcmd[playernum].sidemove=cmd->sidemove;
        ziptic|=ZT_SIDE;
    }

    if(cmd->angleturn != oldcmd[playernum].angleturn)
    {
        WRITE16(demo_p,cmd->angleturn);
        oldcmd[playernum].angleturn=cmd->angleturn;
        ziptic|=ZT_ANGLE;
    }

    if(cmd->buttons != oldcmd[playernum].buttons)
    {
        *demo_p++ = cmd->buttons;
        oldcmd[playernum].buttons=cmd->buttons;
        ziptic|=ZT_BUTTONS;
    }

    if(cmd->aiming != oldcmd[playernum].aiming)
    {
        WRITE16(demo_p,cmd->aiming);
        oldcmd[playernum].aiming=cmd->aiming;
        ziptic|=ZT_AIMING;
    }

    if(AddLmpExtradata(&demo_p,playernum))
        ziptic|=ZT_EXTRADATA;

    *ziptic_p=ziptic;
    //added:16-02-98: attention here for the ticcmd size!
    // latest demos with mouse aiming byte in ticcmd
    if (ziptic_p > demoend - (5*MAXPLAYERS))
    {
        G_CheckDemoStatus ();   // no more space
        return;
    }

//  don't work in network the consistency is not copyed in the cmd
//    demo_p = ziptic_p;
//    G_ReadDemoTiccmd (cmd,playernum);         // make SURE it is exactly the same
}



//
// G_RecordDemo
//
// [Arcade] Factored out of G_RecordDemo so the background high-score
// recorder (hs_stuff.c) can (re)start recording once per game, not just
// once per process run -- frees any stale buffer first to avoid a leak.
void G_RecordDemo_maxsize (const char* name, int maxsize)
{
    // demoname is DEMONAME_LEN+5
    dl_strncpy(demoname, name, DEMONAME_LEN);
    strcat (demoname, ".lmp");

    demo_scratch = false;   // -record saves; see G_CheckDemoStatus

    if( demobuffer )
    {
        Z_Free (demobuffer);
        demobuffer = NULL;
    }
    demobuffer = Z_Malloc (maxsize,PU_STATIC,NULL);
    demoend = demobuffer + maxsize;

    demorecording = true;
}

void G_RecordDemo (const char* name)
{
    int             i;
    int             maxsize = 0x20000;

    i = M_CheckParm ("-maxdemo");
    if (i && i<myargc-1)
        maxsize = atoi(myargv[i+1])*1024;

    G_RecordDemo_maxsize(name, maxsize);
}


void G_BeginRecording (void)
{
    int             i;
    int rec_version = VERSION;

#if 0
    // If ever need to record something other than VERSION
    // make sure they agree this time.
    if( rec_version != VERSION )
    {
        G_Downgrade( rec_version );
    }
#endif

    demo_p = demobuffer;

    // DoomLegacy version 1.44, 1.45, and after will all use demo header 144,
    // The actual DoomLegacy version is recorded in its header fields.
    // Do not change this header, except to add new fields in the empty space.

    // write DL format (demo144) header
    *demo_p++ = 144;   // Mark all DoomLegacy demo as version 144.
    *demo_p++ = 'D';   // "DL" for DoomLegacy
    *demo_p++ = 'L';   
    *demo_p++ = 1;     // non-zero format version (demo144_format)
                       // 0 would be an older version with new header.
    *demo_p++ = VERSION;  // version of doomlegacy that recorded it.
    *demo_p++ = rec_version;  // actual DoomLegacy demoversion recorded
    *demo_p++ = REVISION;     // demo subversion, when needed
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    // Save EV value to get correct values when invoked by command line or saved game.
    *demo_p++ = cv_deathmatch.EV;
    *demo_p++ = cv_respawnmonsters.EV;
    *demo_p++ = cv_fastmonsters.EV;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;
    *demo_p++ = cv_timelimit.value;      // just to be compatible with old demo (no more used)
    *demo_p++ = multiplayer;             // 1..31

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if(playeringame[i])
          *demo_p++ = 1;
        else
          *demo_p++ = 0;
    }
   
    byte * demo_p_next = demo_p + 64;
    // more settings that affect playback
    *demo_p++ = cv_solidcorpse.EV;
#ifdef DOORDELAY_CONTROL
    *demo_p++ = delay_ticks_per_sec;  // doordelay, 0 is not default
#else
    *demo_p++ = 0; 	// no doordelay
#endif
    *demo_p++ = 0x40 + voodoo_mode;  // 0 is not default
    *demo_p++ = cv_instadeath.EV;  // voodoo doll instadeath, 0 is default
    *demo_p++ = cv_monsterfriction.EV;
    *demo_p++ = friction_model;
    *demo_p++ = cv_rndsoundpitch.EV;  // uses M_Random
    *demo_p++ = cv_monbehavior.EV;
    *demo_p++ = cv_doorstuck.EV;
    *demo_p++ = cv_monstergravity.EV;
    // 10
    // Boom and MBF derived controls.
    *demo_p++ = cv_monster_remember.EV;
    *demo_p++ = cv_weapon_recoil.EV;
    *demo_p++ = cv_invul_skymap.EV;
    *demo_p++ = cv_zerotags.EV;
    *demo_p++ = cv_mbf_dropoff.EV;
    *demo_p++ = cv_mbf_falloff.EV;
    *demo_p++ = cv_mbf_pursuit.EV;
    *demo_p++ = cv_mbf_monster_avoid_hazard.EV;
    *demo_p++ = cv_mbf_monster_backing.EV;
    *demo_p++ = cv_mbf_staylift.EV;
    *demo_p++ = cv_mbf_help_friend.EV;
    *demo_p++ = (cv_mbf_distfriend.value >> 8);  // MSB
    *demo_p++ = cv_mbf_distfriend.value & 0xFF;  // LSB
    *demo_p++ = cv_mbf_monkeys.EV;
#ifdef DOGS
    *demo_p++ = cv_mbf_dogs.EV;
    *demo_p++ = cv_mbf_dog_jumping.EV;
#else
    *demo_p++ = 0;
    *demo_p++ = 0;
#endif
    // 26
    *demo_p++ = (cv_respawnmonsterstime.value >> 8);  // MSB
    *demo_p++ = cv_respawnmonsterstime.value & 0xFF;  // LSB
    *demo_p++ = (cv_itemrespawntime.value >> 8);  // MSB
    *demo_p++ = cv_itemrespawntime.value & 0xFF;  // LSB
    // 30
    *demo_p++ = (game_comp_tic >> 24); // MSB
    *demo_p++ = (game_comp_tic >> 16);
    *demo_p++ = (game_comp_tic >> 8);
    *demo_p++ = game_comp_tic & 0xFF; // LSB
    // 34
#ifdef MAPTHING_ADJUST
    *demo_p++ = cv_monster_health.EV;
    *demo_p++ = cv_health_pickup.EV;
    *demo_p++ = cv_armor_pickup.EV;
    *demo_p++ = cv_ammo_pickup.EV;
#else
    *demo_p++ = 0;
    *demo_p++ = 0;
    *demo_p++ = 0;
    *demo_p++ = 0;
#endif
    // 38

    // [WDJ fix] Legacy gameplay extras.  G_demo_defaults() disables all of
    // these for playback, but they were never written to the demo, so any
    // demo recorded with one of them enabled desynced -- notably tiredrun,
    // which scales movefactor and so slowly skews the player's momentum.
    // Recorded into the previously zero-filled option area, so demos from
    // older builds read 0 here and keep the old (disabled) behavior.
#ifdef ENABLE_TIRED_RUN
    *demo_p++ = cv_tired_run.EV;
    *demo_p++ = cv_drown.EV;
#else
    *demo_p++ = 0;
    *demo_p++ = 0;
#endif
#ifdef MONSTER_VARY
    *demo_p++ = cv_monster_vary.EV;
#else
    *demo_p++ = 0;
#endif
#ifdef ENABLE_TELE_CONTROL
    *demo_p++ = cv_tele_control.EV;
#else
    *demo_p++ = 0;
#endif
#ifdef ENABLE_SLOW_REACT
    *demo_p++ = cv_slow_react.EV;
#else
    *demo_p++ = 0;
#endif
    *demo_p++ = cv_viewheight.EV;   // 0 means "not recorded", see read side
    // 44

    // empty space
    while( demo_p < demo_p_next )  *demo_p++ = 0;

    *demo_p++ = 0x55;   // Sync mark, start of data
    memset(oldcmd,0,sizeof(oldcmd));
}


// The following are set by DemoAdapt:
//  voodoo_mode,_doordelay;  // see DemoAdapt_p_fab

// The following are init by starting a game (demos cannot occur during game):
// deathmatch, multiplayer, nomonsters, respawnmonsters, fastmonsters
// timelimit.
// Timelimit is NOT saved to config.

// The following are set by G_Downgrade and/or G_DoPlayDemo:
// EN_variable_friction, EN_pushers

static byte      pdss_settings_valid = 0;
static uint16_t  pdss_respawnmonsterstime;
static uint16_t  pdss_itemrespawntime;

void playdemo_save_settings( void )
{
    // Still have a few settings that need save, restore.
    if( pdss_settings_valid == 0 )
    {
        pdss_settings_valid = 1;
        pdss_respawnmonsterstime = cv_respawnmonsterstime.value;
        pdss_itemrespawntime = cv_itemrespawntime.value;
    }
}

void playdemo_restore_settings( void )
{
    if( pdss_settings_valid )
    {
        cv_respawnmonsterstime.value = pdss_respawnmonsterstime;
        cv_itemrespawntime.value = pdss_itemrespawntime;

        // only restore when demo has changed some settings
        G_restore_user_settings();
    }
    pdss_settings_valid = 0;  // so user can change settings between demos
}


//
// G_PlayDemo
//
// Called by D_DoAdvanceDemo to start a demo
// Called by D_DoomMain to play a command line demo
// Called by G_TimeDemo to play and time a demo
void G_DeferedPlayDemo (const char* name)
{
    // [WDJ] All as one string, or else it executes partial string
//    COM_BufAddText(va("playdemo \"%s\"\n", name));

    // Using console command adds extra tics that cause sync problems.
    // Copy the demo name, as some are from stack buffers.
    if( playdemo_name == NULL )
    {
        playdemo_name = (char*) malloc(MAX_WADPATH);
    }
    strncpy( playdemo_name, name, MAX_WADPATH-1);     // parameter
    playdemo_name[MAX_WADPATH-1] = '\0';
   
    gameaction = ga_playdemo;  // play demo after finishing this
}


//
//  Start a demo from a .LMP file or from a wad resource (eg: DEMO1)
//
// Called from SF_PlayDemo, fragglescript plays a demo lump
// Called from Command_Playdemo_f, command play demo file or lump
void G_DoPlayDemo (const char *defdemoname)
{
    skill_e skill;
    lumpnum_t  lmp;
    int   i, episode, map;
    int   demo_size;
    boolean boomdemo = 0;
    byte  demo144_format = 0;
    byte  boom_compatibility_mode = 0;  // Boom 2.00 compatibility flag
    byte  boom_compatibility_level = 0;

    playdemo_save_settings();  // [WDJ] Save user settings.

    // Enables that might be set directly by the demo.
    // Defaults
    EN_boom = 0;
    EN_mbf = 0;
    EV_legacy = 0;
    // Default demo tic read
    EN_demotic_109 = 0;
    EN_boom_longtics = 0;

    // [WDJ] Adapted from PrBoom, keep some old demos in sync.
    game_comp_tic = 0;

//
// load demo file / resource
//

    //it's an internal demo
    dl_strncpy(demoname, defdemoname, DEMONAME_LEN);

    lmp = W_CheckNumForName(defdemoname);
    if( VALID_LUMP( lmp ) )
    {
        // lump
        demobuffer = demo_p = W_CacheLumpNum (lmp, PU_STATIC);
        demo_size = W_LumpLength( lmp );
    }
    else
    {
        // external file
        // [Arcade] Use a full-length local buffer here, not the DEMONAME_LEN
        // (32 char) 'demoname' above -- that truncates long paths like the
        // legacyhome/demos/... record-holder demo files, silently feeding
        // FIL_ReadFile a mangled path.
        char  demopath[MAX_WADPATH];
        dl_strncpy(demopath, defdemoname, MAX_WADPATH);
        FIL_DefaultExtension(demopath,".lmp");
        demo_size = FIL_ReadFile (demopath, &demobuffer);
        demo_p = demobuffer;
    }

    if ( demo_size <= 0 )
    {
        GenPrintf(EMSG_warn, "\2ERROR: couldn't open lump/file '%s'.\n", defdemoname);
        goto no_demo;
    }

    demoend = demo_p + demo_size - 1;

//
// read demo header
//

    gameaction = ga_nothing;
    demoversion = READBYTE(demo_p);
    // header[0]: byte : demo version
    // 101 = Strife 1.01  (unsupported)
    // 104 = Doom 1.4 beta (unsupported)
    // 105 = Doom 1.5 beta (unsupported)
    // 106 = Doom 1.6 beta, 1.666 (unsupported)
    // 107 = Doom2 1.7, 1.7a (unsupported)
    // 108 = Doom 1.8, Doom2 1.8 (unsupported)
    // 109 = Doom 1.9, Doom2 1.9
    // 110 = Doom, published source code
    // 111..143 = Legacy
    // 144 = Legacy DL format
    // 200 = Boom 2.00	(supported badly, no sync)
    // 201 = Boom 2.01  (supported badly, no sync)
    // 202 = Boom 2.02  (supported badly, no sync)
    // 203 = LxDoom or MBF  (supported badly, no sync)
    // 210..214 = prboom (supported badly, no sync)
    // Do not have version: Hexen, Heretic, Doom 1.2 and before
    if( demoversion < 109 )
        goto bad_demo_version;

    if( demoversion < 111 )
    {
        EN_demotic_109 = 1;  // vanilla demo tic format
    }
    else if( demoversion >= 111 && demoversion <= 144 )
    {
        // DoomLegacy Demos
        if( demoversion == 144 )  // DoomLegacy demo DL format number
        {
            if( READBYTE(demo_p) != 'D' )  goto broken_header;
            if( READBYTE(demo_p) != 'L' )  goto broken_header;
            demo144_format = *demo_p++;  // DL format num, (1)
            demo_p++;  // recording legacy version number
            demoversion = READBYTE(demo_p);  // DoomLegacy DL demoversion number
            byte rev = READBYTE(demo_p);
            demoversion_rev = VERREV(demoversion, rev);  // with revision
            // maybe DL header on old demo
            if( demoversion < 111 )  goto broken_header;
            if( demoversion < 143 )  demo144_format = 0;
        }
        EV_legacy = demoversion;  // is a DoomLegacy version
        // Older demos (which could have had a demo144_format header put on them)
        EN_demotic_109 = (demoversion < 112);  // vanilla demo tic format
        EN_boom = (demoversion >= 129);
        // [WDJ] enable of "Marine's Best Friend" feature emulation
        EN_mbf = (demoversion >= 147);
    }
    else if (demoversion >= 200 && demoversion <= 214)
    {
        // Boom, MBF, and prboom headers
        // Used by FreeDoom

        // Read the "Boom" or "MBF" header line
        // Signature starts with 0x1d, end with 0xe6, padded to 6 bytes.
        // Signature Boom:  0x1d 'B' 'o' 'o' 'm'  0xe6
        // Signature MBF:   0x1d 'M' 'B' 'F' 0xe6 0x00
        if( *demo_p == 0x1d )
        {
            // Read signature into header buf and terminate as string.
            byte header[10];
            for ( i=0; i<5; i++ )
            {
                header[i] = demo_p[i+1];
                if( header[i] == 0xe6 )  break;
            }
            header[i] = 0;
            demo_p += 6;  // signature is always 6 bytes

            boom_compatibility_level = demoversion;  // default
            if( (demoversion == 203) && (header[0] == 'B') )
            {
                // LxDoom
                boom_compatibility_mode = 0;
                boom_compatibility_level = 200;  // LxDoom not supported
                EN_boom = 1;
#ifdef DEBUG_DEMO
                debug_Printf( " LxDoom demo\n" );
#endif
            }
            else
            {
                // MBF and prboom header have compatibility flag,
                // which in newer demos will be ignored.
                boom_compatibility_mode = *demo_p++;
                EN_boom = ! boom_compatibility_mode;
#ifdef DEBUG_DEMO
                debug_Printf( " Boom demo\n" );
#endif
            }

            // Complicated decoding for various boom versions.
            if( boom_compatibility_mode )
            { 
                if(demoversion <= 202)
                {
                    boom_compatibility_level = 200;
                }
            }

            EN_demotic_109 = 1;
            if( demoversion == 214 )
            {
                EN_boom_longtics = 1;
            }

            EN_mbf = EN_boom && (header[0] == 'M') && (boom_compatibility_level >= 203);
#ifdef DEBUG_DEMO
            debug_Printf( " demo header: %s.\n", header );
            debug_Printf( " compatibility 0x%x.\n", boom_compatibility_mode );
            debug_Printf( " compatibility_level 0x%x.\n", boom_compatibility_level );
            debug_Printf( " EN_boom %i  EN_mbf %i.\n", EN_boom, EN_mbf );
#endif
            boomdemo = 1;
        }
        else
        {
            goto broken_header;
        }
    }
    else
    {
        goto bad_demo_version;
    }


#ifdef SHOW_DEMOVERSION
    CONS_Printf( "Demo Version %i.\n", (int)demoversion );
#endif
#ifdef DEBUG_DEMO
    debug_Printf( "Demo version %i.\n", (int)demoversion );
#endif

    if (demoversion < VERSION)
        CONS_Printf ("\2Demo is from an older game version\n");

    if( demo_p > demoend )  goto broken_header;

    G_demo_defaults();  // Per EN_boom, EN_mbf

    // header[1]: byte: skill level 0..4
    skill       = *demo_p++;
    // header[2]: byte: Doom episode 1..3, Doom2 and above use 1
    episode     = *demo_p++;
    // header[3]: byte: map level 1..32
    map         = *demo_p++;
#ifdef DEBUG_DEMO
    debug_Printf( " skill %i.\n", (int)skill );
    debug_Printf( " episode %i.\n", (int)episode );
    debug_Printf( " map %i.\n", (int)map );
#endif
    // header[4]: byte: play mode 0..2
    //   0 = single player or coop
    //   1 = deathmatch
    //   2 = alt deathmatch
#ifdef DEBUG_DEMO
    debug_Printf( " play mode/deathmatch %i.\n", (int)demo_p[0] );
#endif
    if (demoversion < 127 || demo144_format || boomdemo)
    {
        // store it, using the console will set it too late
        cv_deathmatch.EV = *demo_p++;
        Deathmatch_OnChange();
    }
    else
        demo_p++;  // old legacy demo, ignore deathmatch

    if( ! boomdemo )
    {
#ifdef DEBUG_DEMO
        debug_Printf( " respawn %i.\n", (int)demo_p[1] );
        debug_Printf( " fast monsters %i.\n", (int)demo_p[2] );
#endif
        // header[5]: byte: respawn boolean
        if (demoversion < 128 || demo144_format)
        {
            // store it, using the console will set it too late
            cv_respawnmonsters.EV = *demo_p++;
        }
        else
            demo_p++;  // legacy demo, ignore respawnmonsters

        // header[6]: byte: fast boolean
        if (demoversion < 128 || demo144_format)
        {
            // store it, using the console will set it too late
            cv_fastmonsters.EV = *demo_p++;
            cv_fastmonsters.func();
        }
        else
            demo_p++;  // legacy demo, ignore fastmonsters

        // header[7]: byte: no monsters present boolean
        nomonsters  = *demo_p++;
#ifdef DEBUG_DEMO
        debug_Printf( " no monsters %i.\n", (int)nomonsters );
#endif
        cv_rndsoundpitch.EV = 0;
    }

    // header[8]: byte: viewing player 0..3, 0=player1
    //added:08-02-98: added displayplayer because the status bar links
    // to the display player when playing back a demo.
    displayplayer = consoleplayer = *demo_p++;
    displayplayer_ptr = consoleplayer_ptr = &players[consoleplayer];  // [WDJ]

#ifdef DEBUG_DEMO
    debug_Printf( " viewing player %i.\n",  (int)displayplayer );
#endif

    //  support old v1.9 demos with ONLY 4 PLAYERS ! Man! what a shame!!!
    if( demoversion==109 )
    {
        // header[9..12]: byte: player[1..4] present boolean
        max_num_players = 4;
    }
    else if( boomdemo )
    {
        cv_rndsoundpitch.EV = EN_boom;  // normal in Boom, call M_Random

        // Boom ReadOptions
        // [WDJ] according to prboom
        // [0] monsters remember
        // [1] variable friction
        // [2] weapon recoil
        // [3] allow pushers
        // [4] ??
        // [5] player bobbing
        // [6] respawn
        // [7] fast monsters
        // [8] no monsters
        cv_monster_remember.EV = demo_p[0];
        EN_variable_friction = demo_p[1];
        cv_weapon_recoil.EV = demo_p[2];
        EN_pushers = demo_p[3];
#ifdef DEBUG_DEMO
        debug_Printf( " respawn %i.\n", (int)demo_p[6] );
        debug_Printf( " fast monsters %i.\n", (int)demo_p[7] );
#endif
        cv_respawnmonsters.EV = demo_p[6];  // respawn monsters, boolean
        cv_fastmonsters.EV = demo_p[7]; // fast monsters, boolean
        cv_fastmonsters.func();
        nomonsters = demo_p[8];  // nomonsters, boolean
#ifdef DEBUG_DEMO
        debug_Printf( " no monsters %i.\n", (int)nomonsters );
#endif
        // [9] demo insurance
        // [10..13] random number seed
        //   When demo insurance, Boom has random number generator per usage,
        //   all initialized from this seed.  DoomLegacy does not have this.
        // Seed is not needed for the standard random number generators.
        if( demo_p[9] )
          debug_Printf( " demo insurance RNG, not implemented.\n" );

        if( demoversion >= 203 ) // MBF and prboom
        {
            // [14] monster infighting
            // [15] dogs
            // [16..17] ??
            // [18..19] distfriend
            // [20] monster backing
            // [21] monster avoid hazards
            // [22] monster friction
            // [23] help friends
            // [24] dog jumping
            // [25] monkeys
            // [26..57] comp vector x32
            // [58] force old BSP
            // monster_infight from demo is 0/1
            // Feature enables 1=ON, Do not notify NET
            cv_monbehavior.EV = demo_p[14]? 2:5; // (infight:off)
#ifdef DOGS
            cv_mbf_dogs.EV = demo_p[15];
#endif
            EV_mbf_distfriend = ((demo_p[18]<<8) + demo_p[19]) << FRACBITS;
            cv_mbf_monster_backing.EV = demo_p[20];
            cv_mbf_monster_avoid_hazard.EV = demo_p[21];
            // Pass EN_monster_friction, and flag cv_monsterfriction
            EN_monster_friction = demo_p[22];
            cv_monsterfriction.EV = 0x80;  // MBF, Vanilla;
            cv_mbf_help_friend.EV = demo_p[23];
#ifdef DOGS
            cv_mbf_dog_jumping.EV = demo_p[24];
#endif
            cv_mbf_monkeys.EV = demo_p[25];
            // comp vector at [26],  1=old demo compatibility
            byte * comp = demo_p + 26;
            EN_mbf_telefrag = ! comp[comp_telefrag];
            cv_mbf_dropoff.EV = ! comp[comp_dropoff];
            EN_vile_revive_bug = comp[comp_vile];  // Vanilla
            EN_skull_limit = comp[comp_pain];
            EN_old_pain_spawn = comp[comp_skull];
            EN_blazing_double_sound = comp[comp_blazing];  // Vanilla
            EN_doorlight = ! comp[comp_doorlight];
            EN_boom_physics = ! comp[comp_model];
            EN_invul_god = ! comp[comp_god];
            cv_mbf_falloff.EV = ! comp[comp_falloff];
            EN_boom_floor = ! comp[comp_floors];
            cv_invul_skymap.EV = ! comp[comp_pursuit];  // 0=Vanilla, 1=Boom
            cv_mbf_pursuit.EV = ! comp[comp_pursuit];
            cv_doorstuck.EV = comp[comp_doorstuck]? 0:2; // Vanilla : MBF
            cv_mbf_staylift.EV = ! comp[comp_staylift];
            EN_catch_respawn_0 = ! comp[comp_respawn];
            EN_skull_bounce_fix = ! comp[comp_soul];
            cv_zerotags.EV = ! comp[comp_zerotags]; // 0=Vanilla, 1=Boom
            EN_doom_movestep_bug = comp[comp_moveblock];  // 1=Vanilla
#ifdef MBF21
            if( demoversion >= 221 )
            {
                EN_ledgeblock = comp[comp_ledgeblock];
                EN_friendlyspawn = comp[comp_friendlyspawn];
                EN_voodooscroller = comp[comp_voodooscroller];
                EN_reserved_line_flag = comp[comp_reservedlineflag];
            }
#endif
        }
        else
        {
            // Boom, not MBF
            cv_doorstuck.EV = EN_boom;  // 1=Boom
        }

        demo_p += (demoversion == 200)? 256 : 64;  // option area size
       

        // byte: player[1..32] present boolean
        // Boom saved room for 32 players even though only supported 4
        max_num_players = (boom_compatibility_level < 200)? 4 : 32;

        if( boom_compatibility_mode )
        {
#ifdef DEBUG_DEMO
            debug_Printf( " Boom demo imitating Doom2\n" );
#endif
            demoversion = 110;  // imitate non-Boom demo
        }
    }
    else
    {
#ifdef DEBUG_DEMO
       debug_Printf( " time limit %i.\n", (int)*demo_p );
#endif
        if(demoversion<128)
        {
           // Here is a byte, but user value may exceed a byte.
           cv_timelimit.value = *demo_p++;
           cv_timelimit.func();
        }
        else
            demo_p++;

        if (demoversion<113)
        {
            // header[9..16]: byte: player[1..8] present boolean
            max_num_players = 8;	    
        }
        else
        {
            // header[17]: byte: multiplayer boolean
            if( demoversion>=131 ) {
                multiplayer = *demo_p++;
#ifdef DEBUG_DEMO
                debug_Printf( " multi-player %i.\n", (int)multiplayer );
#endif
            }

            // header[18..50]: byte: player[1..32] present boolean
            max_num_players = 32;
        }
    }

    if( demo_p > demoend )  goto broken_header;

#if MAXPLAYERS>32
#error Please add support for old lmps
#endif

    // Read players in game.
    memset( playeringame, 0, sizeof(playeringame) );
    for (i=0 ; i<max_num_players ; i++)
    {
        playeringame[i] = *demo_p++;
#ifdef DEBUG_DEMO
        if( playeringame[i] )
             debug_Printf( "   player %i\n", i+1 );
#endif
    }
   
    // FIXME: do a proper test here
    if( demoversion<131 )
        multiplayer = playeringame[1];

    if( demo_p > demoend )  goto broken_header;
   
    // [WDJ]
    if( demo144_format )
    {
        byte * demo_p_next = demo_p + ((demoversion < 147)? 32 : 64);
        // more settings that affect playback
        cv_solidcorpse.EV = *demo_p++;
#ifdef DOORDELAY_CONTROL
        delay_ticks_per_sec = *demo_p++;  // 0 is not default
        if( delay_ticks_per_sec < 20 )  delay_ticks_per_sec = 35;  // default
#else
        demo_p++; 	// no doordelay
#endif
        if( *demo_p >= 0x40 )  // Voodoo doll control
           voodoo_mode = *demo_p++ - 0x40;  // 0 is not default
        else
           voodoo_mode = VM_auto;  // default
        cv_instadeath.EV = *demo_p++;  // voodoo doll instadeath, 0 is default
        cv_monsterfriction.EV = *demo_p++;
        friction_model = *demo_p++;
        cv_rndsoundpitch.EV = *demo_p++;  // uses M_Random
        cv_monbehavior.EV = *demo_p++;
        if( demoversion_rev < VERREV(148,5) )
        {
            // Previous versions implemented full infight.
            if( cv_monbehavior.EV == 2 )  cv_monbehavior.EV = 6;
            else if( cv_monbehavior.EV == 4 )  cv_monbehavior.EV = 7;
        }
        cv_doorstuck.EV = *demo_p++;
        cv_monstergravity.EV = *demo_p++;
        // Boom and MBF derived controls.
        cv_monster_remember.EV = *demo_p++;
        cv_weapon_recoil.EV = *demo_p++;
        cv_invul_skymap.EV = *demo_p++;
        cv_zerotags.EV = *demo_p++;
        cv_mbf_dropoff.EV = *demo_p++;
        cv_mbf_falloff.EV = *demo_p++;
        cv_mbf_pursuit.EV = *demo_p++;
        cv_mbf_monster_avoid_hazard.EV = *demo_p++;
        cv_mbf_monster_backing.EV = *demo_p++;
        cv_mbf_staylift.EV = *demo_p++;
        cv_mbf_help_friend.EV = *demo_p++;
        EV_mbf_distfriend = ((demo_p[0]<<8) + demo_p[1]) << FRACBITS;
        demo_p += 2;
        cv_mbf_monkeys.EV = *demo_p++;
#ifdef DOGS
        cv_mbf_dogs.EV = *demo_p++;
        cv_mbf_dog_jumping.EV = *demo_p++;
#else
        demo_p++;
        demo_p++;
#endif
        cv_respawnmonsterstime.value = (demo_p[0]<<8) + demo_p[1];
        demo_p += 2;
        cv_itemrespawntime.value = (demo_p[0]<<8) + demo_p[1];
        demo_p += 2;
        game_comp_tic = (((((demo_p[0]<<8) + demo_p[1])<<8) + demo_p[2])<<8) + demo_p[3];
        demo_p += 4;

        // 34
#ifdef MAPTHING_ADJUST
        cv_monster_health.EV = *demo_p++;
        cv_health_pickup.EV = *demo_p++;
        cv_armor_pickup.EV = *demo_p++;
        cv_ammo_pickup.EV = *demo_p++;
#else
        demo_p += 4;
#endif
        // 38

        // [WDJ fix] Legacy gameplay extras, see G_BeginRecording.
        // Demos recorded before these were added have 0 here, which matches
        // the G_demo_defaults() values already applied, so they are unaffected.
        if( demoversion >= 148 )
        {
#ifdef ENABLE_TIRED_RUN
            cv_tired_run.EV = *demo_p++;
            cv_drown.EV = *demo_p++;
#else
            demo_p += 2;
#endif
#ifdef MONSTER_VARY
            cv_monster_vary.EV = *demo_p++;
#else
            demo_p++;
#endif
#ifdef ENABLE_TELE_CONTROL
            cv_tele_control.EV = *demo_p++;
#else
            demo_p++;
#endif
#ifdef ENABLE_SLOW_REACT
            cv_slow_react.EV = *demo_p++;
#else
            demo_p++;
#endif
            {
                byte vh = *demo_p++;
                if( vh )  cv_viewheight.EV = vh;  // 0 = not recorded
            }
            // 44
        }

        demo_p = demo_p_next;  // skip rest of settings
        if( *demo_p++ != 0x55 )  goto broken_header;  // Sync mark, start of data
    }

    if( demo_p > demoend )  goto kill_demo;
   
    memset(oldcmd,0,sizeof(oldcmd));

    demoplayback = true;

    // [Arcade] A demo draws one full-screen view whatever the cabinet's panel
    // count says (see D_NumViews), and the viewport sizes are only recomputed
    // on request -- without this the demo kept the 2x2 geometry and played in
    // the top-left quadrant.
    R_SetViewSize();

    // [Arcade] The replay is its own run for the intermission's purposes.
    // Set after demoplayback so HS_LevelExit takes the replay path, and
    // before G_InitNew below, which reaches the first level.
    HS_Demo_Start();

    // don't spend a lot of time in loadlevel
    if(demoversion<127 || boomdemo)
    {
        precache = false;
        G_InitNew (skill, G_BuildMapName(episode, map),true);
        precache = true;
    }
    else
    {
        // wait map command in the demo
//        gamestate = wipegamestate = GS_WAITINGPLAYERS;  // will display waiting counter
//        gamestate = wipegamestate = GS_DEMOSCREEN;  // will advance to next demo
        gamestate = wipegamestate = GS_NULL;
    }

    CON_ToggleOff (); // may be also done at the end of map command
    return;


bad_demo_version:
    GenPrintf(EMSG_warn, "\2ERROR: Incompatible demo (version %d). Legacy supports demo versions 109-%d.\n", demoversion, VERSION);
    goto kill_demo;
   
broken_header:   
#ifdef DEBUG_DEMO
    debug_Printf( " broken demo header\n" );
#endif
   
kill_demo:
    Z_Free (demobuffer);
    playdemo_restore_settings();
    G_set_gamemode( gamemode );  // restore EN set by gamemode
    G_setup_VERSION();
no_demo:
    gameaction = ga_nothing;
    return;
}

//
// G_TimeDemo
//             NOTE: name is a full filename for external demos
//
static byte EV_restore_cv_vidwait = 0;

void G_TimeDemo (const char* name)
{
    nodrawers = M_CheckParm ("-nodraw");
    noblit = M_CheckParm ("-noblit");
    EV_restore_cv_vidwait = cv_vidwait.EV;
    if( cv_vidwait.EV )
        CV_Set( &cv_vidwait, "0");
    timingdemo = true;
    singletics = true;
    framecount = 0;
    demostarttime = I_GetTime ();
    G_DeferedPlayDemo (name);
}


void G_DoneLevelLoad(void)
{
    CONS_Printf("Load Level in %f sec\n",(float)(I_GetTime()-demostarttime)/TICRATE);
    framecount = 0;
    demostarttime = I_GetTime ();
}


// Called after a death or level completion to allow demos to be cleaned up
// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void G_StopDemo(void)
{
    Z_Free (demobuffer);
    demobuffer = NULL;  // [Arcade] was left dangling; G_RecordDemo_maxsize frees it
    demoplayback  = false;
    timingdemo = false;
    singletics = false;

    R_SetViewSize();   // [Arcade] back to the cabinet's own view layout

    playdemo_restore_settings();  // [WDJ] restore user settings
    G_set_gamemode( gamemode );  // restore EN set by gamemode
    G_setup_VERSION();

    gamestate=wipegamestate=GS_NULL;
    SV_StopServer();
//    SV_StartServer();
    SV_ResetServer();
   
    // cleanup
    if( playdemo_name )
    {
        free(playdemo_name);
        playdemo_name = NULL;
    }
}

// [Arcade] Write a copy of the demo recorded so far (from game start
// through the current point) to filename, WITHOUT touching demo_p or
// demobuffer -- the live recording continues uninterrupted after this
// call, so later levels in the same run can still set their own records.
// Called by: HS_LevelExit (hs_stuff.c), when a new best time is set.
boolean G_SnapshotDemo (const char * filename)
{
    int    len;
    byte * tempbuf;
    boolean ok;

    if( ! demorecording )  return false;

    len = demo_p - demobuffer;
    tempbuf = (byte*) malloc(len + 1);
    if( ! tempbuf )  return false;

    memcpy(tempbuf, demobuffer, len);
    tempbuf[len] = DEMOMARKER;
    ok = FIL_WriteFile (filename, tempbuf, len+1);
    free(tempbuf);
    return ok;
}

// Called by G_DeferedInitNew, G_ReadDemoTiccmd, G_WriteDemoTiccmd
// return value is not used by any caller
boolean G_CheckDemoStatus (void)
{
    if (timingdemo)
    {
        int time;
        float f1,f2;
        time = I_GetTime () - demostarttime;
        if(!time) return true;
        G_StopDemo ();
        timingdemo = false;
        f1=time;
        f2=framecount*TICRATE;
        CONS_Printf ("timed %i gametics in %i realtics\n"
                     "%f seconds, %f avg fps\n"
                     ,leveltime,time,f1/TICRATE,f2/f1);
        if( EV_restore_cv_vidwait != cv_vidwait.EV )
            CV_SetValue(&cv_vidwait, EV_restore_cv_vidwait);
        D_AdvanceDemo ();
        return true;
    }

    if (demoplayback)
    {
        if (singledemo)
            I_Quit();  // No return
        G_StopDemo();

        // [Arcade] A record demo watched from the Single Level menu returns
        // there rather than dropping into the attract cycle.
        if( single_level_mode )
        {
            M_SingleLevel_Finished();
            return true;
        }

        D_AdvanceDemo ();
        return true;
    }

    if (demorecording)
    {
        *demo_p++ = DEMOMARKER;

        // [Arcade] The high score recorder runs continuously and is only
        // ever read through G_SnapshotDemo, so its buffer has no consumer
        // when recording stops.  Writing it dropped a stray hs_background.lmp
        // into the working directory on every exit.
        if( ! demo_scratch )
        {
            FIL_WriteFile (demoname, demobuffer, demo_p - demobuffer);
            GenPrintf(EMSG_hud, "\2Demo %s recorded\n", demoname);
        }

        Z_Free (demobuffer);
        demobuffer = NULL;  // [Arcade] was left dangling
        demorecording = false;
        demo_scratch = false;
        return true;
    }

    return false;
}
