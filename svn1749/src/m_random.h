// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_random.h 1727 2025-02-07 05:03:05Z wesleyjohnson $
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
// $Log: m_random.h,v $
// Revision 1.4  2001/06/10 21:16:01  bpereira
//
// Revision 1.3  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//    Random
//
//    
//-----------------------------------------------------------------------------

#ifndef M_RANDOM_H
#define M_RANDOM_H

#include "doomtype.h"

//#define DEBUGRANDOM

// For debugging, like in PrBoom
//#define PP_RANDOM_EXPOSED


// P_Random: for gameplay decisions, demo sync.
// Returns a number from 0 to 255,
// from a lookup table.

#ifdef DEBUGRANDOM
#define P_Random() P_RandomFL(__FILE__,__LINE__)
#define P_SignedRandom() P_SignedRandomFL(__FILE__,__LINE__)
byte P_RandomFL (char *fn, int ln);
int P_SignedRandomFL (char *fn, int ln);
#else
// As M_Random, but used only by the play simulation.
byte P_Random (void);
int P_SignedRandom ();
#endif

#ifdef PP_RANDOM_EXPOSED
// killough 2/16/98:
//
// Make every random number generator local to each control-equivalent block.
// Critical for demo sync. Changing the order of this list breaks all previous
// versions' demos. The random number generators are made local to reduce the
// chances of sync problems. In Doom, if a single random number generator call
// was off, it would mess up all random number generators. This reduces the
// chances of it happening by making each RNG local to a control flow block.
//
// Notes to developers: if you want to reduce your demo sync hassles, follow
// this rule: for each call to P_Random you add, add a new class to the enum
// type below for each block of code which calls P_Random. If two calls to
// P_Random are not in "control-equivalent blocks", i.e. there are any cases
// where one is executed, and the other is not, put them in separate classes.
//
// Keep all current entries in this list the same, and in the order
// indicated by the #'s, because they're critical for preserving demo
// sync. Do not remove entries simply because they become unused later.

typedef enum {
  pr_skullfly,                // #1
  pr_damage,                  // #2
  pr_crush,                   // #3
  pr_genlift,                 // #4
  pr_killtics,                // #5
  pr_damagemobj,              // #6
  pr_painchance,              // #7
  pr_lights,                  // #8
  pr_explode,                 // #9
  pr_respawn,                 // #10
  pr_lastlook,                // #11
  pr_spawnthing,              // #12
  pr_spawnpuff,               // #13
  pr_spawnblood,              // #14
  pr_missile,                 // #15
  pr_shadow,                  // #16
  pr_plats,                   // #17
  pr_punch,                   // #18
  pr_punchangle,              // #19
  pr_saw,                     // #20
  pr_plasma,                  // #21
  pr_gunshot,                 // #22
  pr_misfire,                 // #23
  pr_shotgun,                 // #24
  pr_bfg,                     // #25
  pr_slimehurt,               // #26
  pr_dmspawn,                 // #27
  pr_missrange,               // #28
  pr_trywalk,                 // #29
  pr_newchase,                // #30
  pr_newchasedir,             // #31
  pr_see,                     // #32
  pr_facetarget,              // #33
  pr_posattack,               // #34
  pr_sposattack,              // #35
  pr_cposattack,              // #36
  pr_spidrefire,              // #37
  pr_troopattack,             // #38
  pr_sargattack,              // #39
  pr_headattack,              // #40
  pr_bruisattack,             // #41
  pr_tracer,                  // #42
  pr_skelfist,                // #43
  pr_scream,                  // #44
  pr_brainscream,             // #45
  pr_cposrefire,              // #46
  pr_brainexp,                // #47
  pr_spawnfly,                // #48
  pr_misc,                    // #49
  pr_all_in_one,              // #50
  /* CPhipps - new entries from MBF, mostly unused for now */
  pr_opendoor,                // #51
  pr_targetsearch,            // #52  (unused in MBF)
  pr_friends,                 // #53
  pr_threshold,               // #54  (unused in MBF)
  pr_skiptarget,              // #55
  pr_enemystrafe,             // #56
  pr_avoidcrush,              // #57
  pr_stayonlift,              // #58
  pr_helpfriend,              // #59
  pr_dropoff,                 // #60
  pr_randomjump,              // #61
  pr_defect,                  // #62  // Start new entries -- add new entries below

  // Legacy use of P_Random, Not in Doom, PrBoom, MBF
  pL_smoketrail,       // SmokeTrailer, EV_Legacy
  pL_smokefeet,        // SpawnSmoke, EV_Legacy >= 125
  pL_splashsound,      // SpawnSplash, EV_Legacy >= 125
  pL_spawnbloodxy,     // SpawnBlood, EV_Legacy >= 128
  pL_bloodsplat,       // SpawnBloodSplat, cv_splats
  pL_bloodtrav,        // SpawnBloodSplat, cv_splats   
  pL_initlastlook,     // LookForPlayers, EV_Legacy >= 129
  pL_PRnd,             // fragglescript
  pL_coopthing,        // Legacy 1.48: coop mode, cv_deathmatch= COOP_60, COOP_80
  pL_mapvariation,     // Legacy 1.48; spawn varied objects and monsters

  // Heretic
  ph_heretic,
  ph_spawnmobjfloat,
  ph_spawnfloatbob,
  ph_teleglitter1,
  ph_teleglitter2,
  ph_blooddrip,
  ph_floorwater,
  ph_floorlava,
  ph_floorsludge,
  ph_volcanotic,   
  ph_volcanobl,
  ph_volcanohit,
  ph_podgoo,
  ph_podgoomom,
  ph_podthrust,
  ph_lookmon,
  ph_notseen,
  ph_ripdam,
  ph_impattack,
  ph_impball,
  ph_impdamage,
  ph_impexpl,
  ph_clinkdam,
  ph_knightaxe,
  ph_beastpuff,
  ph_headatk,
  ph_headdam,
  ph_headwind,
  ph_whirlwind,
  ph_wizscream,
  ph_sormissile,
  ph_sorsparkmom,
  ph_sortele1,
  ph_sortele2,
  ph_sordam,
  ph_soratkdam,
  ph_minocharge,
  ph_minofire,
  ph_minoatk3,
  ph_minoslam,
  ph_chickenmorph,
  ph_chickenthink,
  ph_beakatk1,
  ph_beakatk2,
  ph_chickendam,
  ph_feathers,
  ph_morphlast,
  ph_staffready,
  ph_staffatk1,
  ph_staffatk2,
  ph_goldwandatk1,
  ph_goldwandatk2,
  ph_blaster1,
  ph_blasterthink,
  ph_boltspark,
  ph_skullrodatk1,
  ph_skullrodrain,
  ph_rainimpact,
  ph_raindam,
  ph_phoenix2,
  ph_phoenixdam2,
  ph_gauntlet,
  ph_gauntlet2,
  ph_macepos1,
  ph_macepos2,
  ph_maceatk1,
  ph_bloodyskullmom,
  ph_dropitem,   
  ph_dropmom,
  ph_telearti,

  // Added Legacy uses of P_Random
  pL_scatter,
  pL_tele,
  pL_slowreact,

  // End of new entries
  NUMPRCLASS               // MUST be last item in list
} pr_class_t;

byte  PP_Random( byte pr );
int   PP_SignedRandom( byte pr );
#else

// MBF, PrBoom
// Ignore the pr_class parameter.
# define  PP_Random(pr)  P_Random()
# define  PP_SignedRandom(pr)  P_SignedRandom()
#endif

// New Legacy stuffs, unsynced.
byte N_Random(void);
int  N_SignedRandom(void);

// M_Random: music, st_stuff, wi_stuff
// Returns a number from 0 to 255.
byte M_Random (void);

// separate so to not affect demo playback
byte A_Random (void);  // ambience
byte B_Random (void);  // bots etc..

// Fix randoms for demos.
void M_ClearRandom (void);

byte P_Rand_GetIndex(void);
byte B_Rand_GetIndex(void);

void P_Rand_SetIndex(byte rindex);
void B_Rand_SetIndex(byte rindex);

// [WDJ] Extended random, has long repeat period.
//  returns unsigned 16 bit
int  E_Random(void);
//  returns -range, 0, +range
int  E_SignedRandom( int range );

// True for the percentage of the calls.
#define E_RandomPercent( per )   (E_Random() < ((unsigned int)(per * (0.01f * 0xFFFF))))

uint32_t  E_Rand_Get( uint32_t * rs );
void  E_Rand_Set( uint32_t rn, uint32_t rs );

#endif
