// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_local.h 1745 2025-04-10 09:36:34Z wesleyjohnson $
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
// $Log: p_local.h,v $
// Revision 1.24  2004/07/27 08:19:36  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.23  2003/06/11 00:28:50  ssntails
// Big Blockmap Support (128kb+ ?)
//
// Revision 1.22  2003/03/22 22:35:59  hurdler
// Revision 1.21  2002/09/27 16:40:09  tonyd
// First commit of acbot
//
// Revision 1.20  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.19  2001/07/28 16:18:37  bpereira
//
// Revision 1.18  2001/07/16 22:35:41  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.17  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.16  2001/03/21 18:24:38  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.15  2001/02/10 12:27:14  bpereira
//
// Revision 1.14  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.13  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.12  2000/11/02 19:49:35  bpereira
//
// Revision 1.11  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.10  2000/10/21 08:43:30  bpereira
// Revision 1.9  2000/08/31 14:30:55  bpereira
// Revision 1.8  2000/04/16 18:38:07  bpereira
//
// Revision 1.7  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.6  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.5  2000/04/08 17:29:24  stroggonmeth
//
// Revision 1.4  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.3  2000/04/04 00:32:46  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Play functions, animation, global header.
//
//-----------------------------------------------------------------------------


#ifndef P_LOCAL_H
#define P_LOCAL_H

#include "doomdef.h"
  // DOORDELAY_CONTROL
#include "doomtype.h"
  // tic_t
#include "command.h"
  // consvar_t
#include "d_player.h"
#include "m_fixed.h"
#include "m_bbox.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "p_maputl.h"


#define FLOATSPEED              (FRACUNIT*4)

// added by Boris : for dehacked patches, replaced #define by int
extern int MAXHEALTH;   // 100

#define VIEWHEIGHT               41
#define VIEWHEIGHTS             "41"

// default viewheight is changeable at console
extern consvar_t cv_viewheight; // p_mobj.c

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS   128
#define MAPBLOCKSIZE    (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT   (FRACBITS+7)
#define MAPBMASK        (MAPBLOCKSIZE-1)
#define MAPBTOFRAC      (MAPBLOCKSHIFT-FRACBITS)


// player radius used only in am_map.c
#define PLAYERRADIUS    (16*FRACUNIT)

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS       (32*FRACUNIT)

#define MAXMOVE         (30*FRACUNIT/NEWTICRATERATIO)

//added:26-02-98: max Z move up or down without jumping
//      above this, a heigth difference is considered as a 'dropoff'
#define MAXSTEPMOVE     (24*FRACUNIT)

#define USERANGE        (64*FRACUNIT)
#define MELEERANGE      (64*FRACUNIT)
#define MISSILERANGE    (32*64*FRACUNIT)

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD   100

//#define AIMINGTOSLOPE(aiming)   finetangent[(2048+(aiming>>ANGLETOFINESHIFT)) & FINEMASK]
#define AIMINGTOSLOPE(aiming)   finesine[(aiming>>ANGLETOFINESHIFT) & FINEMASK]

//26-07-98: p_mobj.c
extern  consvar_t cv_gravity;

#ifdef DOORDELAY_CONTROL
// [WDJ] 1/15/2009 support control of door and event delay. see p_doors.c
// init in r_main.c
extern  int  delay_ticks_per_sec;
extern  consvar_t  cv_doordelay;
#else
// standard ticks per second
#define delay_ticks_per_sec  35
#endif

// [WDJ] 2/7/2011 Voodoo doll controls and support
extern consvar_t  cv_instadeath;
extern consvar_t  cv_voodoo_mode;
typedef enum {
   VM_vanilla, VM_multispawn, VM_target, VM_auto
} voodoo_mode_e;
extern voodoo_mode_e  voodoo_mode;
extern player_t *  spechit_player; // last player to trigger switch or linedef


//
// P_PSPR
//
void P_SetupPsprites (player_t* curplayer);
void P_MovePsprites (player_t* curplayer);
void P_DropWeapon (player_t* player);


//
// P_USER
//
typedef struct camera_s
{
    player_t *  chase;  // player the camera chases, NULL when off
    mobj_t*     mo;     // the camera object
    angle_t     aiming;
    int         fixedcolormap;

    //SoM: Things used by FS cameras.
    fixed_t     viewheight;
    angle_t     startangle;
} camera_t;

extern camera_t camera;

extern consvar_t cv_cam_dist;  
extern consvar_t cv_cam_height;
extern consvar_t cv_cam_speed;

extern fixed_t jumpgravity;  // variable by fragglescipt


void   P_ResetCamera (player_t* player);
void   P_PlayerThink (player_t* player);
void   P_SetPlayer_color( player_t * player, byte color );

// client prediction
void   CL_ResetSpiritPosition (mobj_t* mobj);
void   P_MoveSpirit (player_t* p,ticcmd_t* cmd, int realtics);

//
// P_MOBJ
//
#define ONFLOORZ        FIXED_MIN
#define ONCEILINGZ      FIXED_MAX

// Time interval for item respawning.
// WARING MUST be a power of 2
#define ITEMQUESIZE     128

extern mapthing_t     *itemrespawnque[ITEMQUESIZE];
extern tic_t          itemrespawntime[ITEMQUESIZE];
extern int              iquehead;
extern int              iquetail;


void P_RespawnSpecials (void);
void P_RespawnWeapons(void);

// Bots
extern  consvar_t cv_bots;
extern  consvar_t cv_bot_skill;
extern  consvar_t cv_bot_speed;
extern  consvar_t cv_bot_skin;
extern  consvar_t cv_bot_respawn_time;
extern  consvar_t cv_bot_random;
extern  consvar_t cv_bot_randseed;
extern  consvar_t cv_bot_gen;
extern  consvar_t cv_bot_grab;

//
// P_ENEMY
//
extern  consvar_t cv_monbehavior;
extern  consvar_t cv_monsterfriction;
extern  consvar_t cv_doorstuck;
extern  consvar_t cv_solidcorpse;
extern  consvar_t cv_monstergravity;
extern  consvar_t cv_monster_remember;

extern  consvar_t cv_mbf_dropoff;
extern  consvar_t cv_mbf_falloff;
extern  consvar_t cv_mbf_pursuit;
extern  consvar_t cv_mbf_staylift;
extern  consvar_t cv_mbf_monster_avoid_hazard;
extern  consvar_t cv_mbf_monster_backing;
extern  consvar_t cv_mbf_help_friend;
extern  consvar_t cv_mbf_distfriend;
extern  consvar_t cv_mbf_monkeys;
#ifdef DOGS   
extern  consvar_t cv_mbf_dogs;
extern  consvar_t cv_mbf_dog_jumping;
#endif
extern  uint16_t helper_MT;  // Substitute helper thing (like DOG).

// when pushing a line 
//#define MAXSPECIALCROSS 16

extern  int *   spechit;                //SoM: 3/15/2000: Limit removal
extern  int     numspechit;

void P_NoiseAlert (mobj_t* target, mobj_t* emmiter);
boolean P_IsOnLift( const mobj_t* actor );
int P_IsUnderDamage(mobj_t* actor);

void P_UnsetThingPosition (mobj_t* thing);
void P_SetThingPosition (mobj_t* thing);

// init braintagets position
void P_Init_BrainTarget();

//
// P_MAP
//

// [Arcade] Monster height model.  1 = vanilla infinitely tall things,
// 0 = Legacy/Heretic over-under passing.  See PIT_CheckThing.
extern  consvar_t cv_tall_monsters;

// [Arcade] The single answer both MF2_PASSMOBJ gates must use:
// PIT_CheckThing (moving into a thing) and P_MobjThinker (resting on one).
boolean  P_Mobj_Pass_Over_Under( mobj_t * mo );

// TryMove, thing map global vars
extern fixed_t  tm_bbox[4];	// box around the thing
extern mobj_t*  tm_thing;	// the thing itself
extern uint32_t tm_flags;	// thing flags of tm_thing
extern fixed_t  tm_x, tm_y;	// thing map position

// TryMove, thing map response global vars
// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern byte     tmr_floatok;  // floating thing ok to move
extern byte     tmr_felldown; // MBF, went off deep dropoff
extern fixed_t  tmr_floorz;   // floor and ceiling of new position
extern fixed_t  tmr_ceilingz;
extern fixed_t  tmr_sectorceilingz;      //added:28-02-98: p_spawnmobj
extern mobj_t*  tmr_floorthing;   // standing on another thing
extern line_t*  tmr_ceilingline;  // line that lowers ceiling, for missile sky test
extern line_t*  tmr_blockingline; // stopping line that is solid
extern line_t*  tmr_dropoffline;  // line that is dropoff edge
extern fixed_t  tmr_dropoffz;   // the lowest point contacted (monster check)
extern sector_t* tmr_sector;  // for bots

extern  msecnode_t*     sector_list;

// P_CheckPosition, P_TryMove, P_CheckCrossLine
// use tm_ global vars, and return tmr_ global vars
boolean P_CheckPosition (mobj_t* thing, fixed_t x, fixed_t y);
boolean P_TryMove (mobj_t* thing, fixed_t x, fixed_t y, byte allowdropoff);
boolean P_CheckCrossLine (mobj_t* thing, fixed_t x, fixed_t y);

boolean P_TeleportMove (mobj_t* thing, fixed_t x, fixed_t y, byte stomp);
void    P_SlideMove (mobj_t* mo);

boolean P_CheckSight (mobj_t* t1, mobj_t* t2);
boolean P_CheckSight2 (mobj_t* t1, mobj_t* t2, fixed_t x, fixed_t y, fixed_t z);	//added by AC for predicting
void    P_UseLines (player_t* player);

boolean P_CheckSector(sector_t* sector, boolean crunch);
boolean P_ChangeSector (sector_t* sector, boolean crunch);

void    P_DelSeclist(msecnode_t *);
void    P_Create_SecNodeList(mobj_t*,fixed_t,fixed_t);
void    P_Init_Secnode( void );

extern fixed_t  got_friction;
extern int      got_movefactor;  // return values of P_GetFriction and P_GetMoveFactor
fixed_t P_GetFriction( const mobj_t*  mo );
int     P_GetMoveFactor(mobj_t* mo);

// Line Attack return global vars  lar_*
extern mobj_t*  lar_linetarget;  // who got hit (or NULL)
extern fixed_t  la_attackrange;  // max range of weapon

fixed_t P_AimLineAttack ( mobj_t* t1, angle_t angle, fixed_t distance,
                          byte  mbf_friend_protection );

void P_LineAttack ( mobj_t* t1, angle_t angle, fixed_t distance,
                    fixed_t slope, int damage );

void P_RadiusAttack ( mobj_t* spot, mobj_t* source, int damage );
#ifdef MBF21
void P_RadiusAttack_MBF21 ( mobj_t* spot, mobj_t* source, int damage, int distance, boolean can_damage_source );
#endif

// MBF
void P_ApplyTorque(mobj_t* mo);


//
// P_SETUP
//
extern byte*            rejectmatrix;   // for fast sight rejection
extern uint32_t         num_rejectmatrix;
// Read wad blockmap using int16_t wadblockmaplump[].
// Expand from 16bit wad to internal 32bit blockmap.
extern uint32_t*        blockmaphead;   // offsets in blockmap are from here
extern uint32_t*        blockmapindex;  // Big blockmap, SSNTails
extern unsigned int     bmapwidth;
extern unsigned int     bmapheight;     // in mapblocks
extern fixed_t          bmaporgx;
extern fixed_t          bmaporgy;       // origin of block map
extern mobj_t**         blocklinks;     // for thing chains

#ifdef GENERATE_BLOCKMAP
extern consvar_t cv_blockmap_gen;
#endif

//
// P_INTER
//
extern uint16_t  maxammo[NUMAMMO];
extern uint16_t  clipammo[NUMAMMO];

void P_TouchSpecialThing ( mobj_t* special, mobj_t* toucher );

boolean P_DamageMobj ( mobj_t* target, mobj_t* inflictor,
                       mobj_t* source, int damage );

//
// P_SIGHT
//

// slopes to top and bottom of target
extern fixed_t  see_topslope;
extern fixed_t  see_bottomslope;


//
// P_SPEC
//
#include "p_spec.h"


// Secondary features.
extern byte  EN_monster_friction;
extern byte  EN_variable_friction;
#ifdef FRICTIONTHINKER
extern byte  EN_boom_friction_thinker;
#endif
extern byte  EN_skull_bounce_fix;  // PrBoom 2001  !comp[comp_soul]
extern byte  EN_skull_bounce_floor;
extern byte  EN_catch_respawn_0;

// Boom
extern byte  EN_pushers;
extern byte  EN_boom_physics;
extern byte  EN_blazing_double_sound;
extern byte  EN_vile_revive_bug;
extern byte  EN_sleeping_sarg_bug;
extern byte  EN_doorlight;
extern byte  EN_skull_limit;
extern byte  EN_old_pain_spawn;
extern byte  EN_invul_god;
extern byte  EN_boom_floor;
extern byte  EN_doom_movestep_bug;

// MBF
extern fixed_t EV_mbf_distfriend;
extern byte  EN_mbf_speed;
extern byte  EN_mbf_telefrag;

#ifdef MBF21
// MBF21
extern byte  EN_ledgeblock;
extern byte  EN_mbf21_action;
#endif

// Heretic, Hexen
extern byte  EN_inventory;   // Heretic, Hexen

// Use Legacy N_Random, instead of some P_Random.
extern byte  EN_nrandom;


// Infight values are arranged for quick testing.
// Any infighting
#define  MONSTER_INFIGHTING    ((byte)(monster_infight - INFT_infight) < (byte)(INFT_coop - INFT_infight))
// Missile infighting
//   if( monster_infight == (INFT_infight|INFT_full) )
// Other
//   if( monster_infight == INFT_coop )
// Cannot change due to being saved in demos.
typedef enum {
 // Boom values
   INFT_none,     // Boom demo value, Doom default of no infight.
   INFT_infight,  // Boom demo value, Monster to monster fighting.
 // Enhanced values
   INFT_full_infight = INFT_infight+4,  // infight with missiles
   INFT_coop = 16,  // Legacy enhancement, and ZDoom option
   INFT_force = 0x80, // transient flag
   INFT_infight_off = INFT_none & INFT_force,  // DEH non-null off (not important in demos)
} infight_e;
// Values from infight_e and from Boom demo.
extern byte  monster_infight; //DarkWolf95:November 21, 2003: Monsters Infight!
extern byte  monster_infight_deh; // DEH input.

typedef enum {
  FR_orig, FR_boom, FR_mbf, FR_prboom, FR_legacy, FR_heretic, FR_hexen
} friction_model_e;
extern byte  friction_model;  // friction_model_e

// Detection bit
typedef enum {
  BDTC_doom = 0x01,
  BDTC_boom = 0x02,
  BDTC_prboom = 0x04,
  BDTC_MBF = 0x08,
  BDTC_MBF21 = 0x10,
  BDTC_heretic = 0x20,
  BDTC_hexen = 0x40,
  BDTC_strife = 0x80,
  BDTC_legacy = 0x100,  // Doom Legacy extensions
  BDTC_ee = 0x200,   // EternityEngine
  BDTC_dehextra = 0x400,
  BDTC_dsda = 0x800,  // DSDA-isms
  BDTC_acp2 = 0x8000
} BDTC_game_detection_e;
extern uint16_t all_detect;   // BDTC_game_detection_e
extern uint16_t deh_detect;   // BDTC_game_detection_e
void report_detect( const char * who, uint16_t detect );  // BDTC_game_detection_e

void  DemoAdapt_p_user( void );
void  DemoAdapt_p_mobj( void );
void  DemoAdapt_p_enemy( void );
void  DemoAdapt_p_floor( void );
void  DemoAdapt_p_map( void );
void  DemoAdapt_bots( void );
#ifdef MBF21
void  DemoAdapt_p_inter( void );
#endif

// Alter for Doom or Heretic.
extern int ceilmovesound;
extern int doorclosesound;

// Heretic specific
extern int H_ArtifactFlash;

#define TELEFOGHEIGHT  (32*FRACUNIT)
extern mobjtype_t      PuffType;
#define FOOTCLIPSIZE   (10*FRACUNIT)
#define HITDICE(a) ((1+(P_Random()&7))*a)

#define MAXCHICKENHEALTH 30

// Now used by Heretic and Doom powers (ironfeet).
// A power counts down to 0. Below BLINKTHRESHOLD, there is a warning blink.
#define BLINKTHRESHOLD  (4*32)
#define WPNLEV2TICS     (40*TICRATE)
#define FLIGHTTICS      (60*TICRATE)

#define CHICKENTICS     (40*TICRATE)
#define FLOATRANDZ      (INT_MAX - 1)

void P_RepositionMace(mobj_t* mo);
void P_ActivateBeak(player_t* player);
void P_PlayerUseArtifact(player_t* player, artitype_t arti);
void P_DSparilTeleport(mobj_t* actor);
void P_Init_Monsters(void);
boolean PH_LookForMonsters(mobj_t* actor);  // Heretic
int P_GetThingFloorType(mobj_t* thing);
mobj_t* P_CheckOnmobj(mobj_t* thing);
void P_AddMaceSpot(mapthing_t* mthing);
boolean P_SightPathTraverse (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
void P_UpdateBeak(player_t* player, pspdef_t* psp);
boolean P_TestMobjLocation(mobj_t* mobj);
void P_PostChickenWeapon(player_t* player, weapontype_t weapon);
void P_SetPsprite ( player_t*     player,
                    int           position,
                    statenum_t    stnum );
boolean P_UseArtifact(player_t* player, artitype_t arti);
byte    P_Teleport_with_effects(mobj_t* thing, fixed_t x, fixed_t y, angle_t angle);
boolean P_SeekerMissile(mobj_t* actor, angle_t thresh, angle_t turnMax);
#ifdef MBF21
boolean P_SeekerMissile_MBF21( mobj_t* actor, mobj_t ** seek_target, angle_t thresh, angle_t turnMax, boolean seek_center );
#endif
mobj_t* P_SpawnMissileAngle(mobj_t* source, mobjtype_t type,
        angle_t angle, fixed_t momz);
boolean P_SetMobjStateNF(mobj_t* mobj, statenum_t state);
boolean P_CheckMissileSpawn (mobj_t* th);
void P_ThrustMobj(mobj_t* mo, angle_t angle, fixed_t move);
void P_Thrust(player_t* player, angle_t angle, fixed_t move);
void P_ExplodeMissile (mobj_t* mo);
boolean P_GiveArtifact(player_t* player, artitype_t arti, mobj_t* mo);
boolean P_ChickenMorphPlayer(player_t* player);
void P_Massacre(void);
void P_AddBossSpot(fixed_t x, fixed_t y, angle_t angle);

statenum_t  DEH_frame_to_state( int deh_frame );

extern spr_light_t  sprite_light[NUMLIGHTS];
extern byte  sprite_light_ind[NUMSPRITES_DEF];
void  Setup_sprite_light( byte  mons_ball_light );

//extern consvar_t cv_dynamiclight;
//extern consvar_t cv_staticlight;
extern consvar_t cv_corona;
extern consvar_t cv_coronasize;
extern consvar_t cv_corona_draw_mode;
extern consvar_t cv_monball_light;

void  Deathmatch_OnChange( void );

// Sky
extern consvar_t cv_sky_gen;

#ifdef HEXEN
// [WDJ] Hexen public.
extern int tm_floorpic;
extern mobj_t*  hexen_blocking_mobj;

void P_hexen_BounceWall( mobj_t* mo );
boolean P_hexen_use_puzzle_item( player_t* player, int item_type );
void  PIT_hexen_thrust_spike( mobjt_t* actor );
#endif

#endif  // P_LOCAL_H
