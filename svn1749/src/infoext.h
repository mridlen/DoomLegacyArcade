// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: info.h 1606 2024-12-01 03:00:00Z wesleyjohnson $
//
// Copyright (C) 2023-2025 by DooM Legacy Team.
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
// $Log: info.h,v $
// DESCRIPTION:
//      Extensions and patches to the tables.
//      Alterations due to game selected.
//
//-----------------------------------------------------------------------------

#ifndef INFOEXT_H
#define INFOEXT_H

#include "doomdef.h"
  // MBF21, Hexen
#include "doomtype.h"
  // sfxid_t, stdint
#include "doomstat.h"
  // gamemode
// Needed for action function pointer handling.
#include "d_think.h"
  // action
#include "m_fixed.h"
  // fixed_t
#include "info.h"
  // state_t
#include "r_defs.h"
  // SPLT_
#include "dstrings.h"
  // text



// ---
// [WDJ] State ext information.
// Just a few states have this information.

// [WDJ] CPP compiled for x11, would not accept
// #define  parm1   args[0]
// #define  parm2   args[1]
// -- as if parm1, parm2 were already declared, so have changed to use parm_args.


#ifdef MBF21
typedef enum {
    FRF_SKILL5_TICS_LSB = 0x0001,  // Save the tics LBS
    FRF_SKILL5_MOD_APPLIED = 0x0002,  // SKILL5 mod applied
    FRF_SKILL5_FAST = 0x0004,  // MBF21 SKILL5FAST from DEH
} frame_flags_e;
#endif

extern uint32_t player_default_flags1;
extern uint32_t player_default_flags2;
extern uint32_t mon_default_flags1;
extern uint32_t mon_default_flags2;

typedef struct
{
    // These are used to position player weapon.
    int32_t    parm1, parm2;  // MBF misc1, misc2 parameters
#ifdef MBF21
    // MBF21 uses these for A_xxx functions
# define MAX_NUM_STATE_ARGS   8
    int32_t    parm_args[MAX_NUM_STATE_ARGS];  // MBF21 parameters
    byte       args_set;  // arg set bits, for defaulting
    uint16_t   frflags;  // FRF_xx flags
#endif
    byte       parm_fixed; // fix parm
} state_ext_t;


extern state_ext_t * state_ext;  // dynamic
extern state_ext_t   empty_state_ext;  // all 0

// Lookup ext_state.
state_ext_t *  P_state_ext( state_t * state );
// Give it an ext_state.
state_ext_t *  P_create_state_ext( state_t * state );

#ifdef MBF21
// Only used for frame modify flags.  There are no other frame flags at this time.
// get FRF_xx flags
# define  P_get_frame_flags( state )   (P_state_ext(state)->frflags)

//   clear_frame_flags : clear these flags first
//   set_frame_flags : set these flags
void  P_set_frame_flags( state_t * state, uint16_t clear_frame_flags, uint16_t set_frame_flags );
#endif

//---
// [WDJ] MBF21 groups, for use in mobjinfo[].xxx_group

#ifdef MBF21
// MBF group definitions
// In DoomLegacy, these are limited to byte.

// For use in mobjinfo[].infighting_group
typedef enum {
  IG_DEFAULT,
  IG_DEH_GROUP
} infighting_group_t;

// For use in mobjinfo[].projectile_group
typedef enum {
  PG_DEFAULT,
  PG_BARON,
  PG_DEH_GROUP,
  PG_NO_IMMUNITY = 255,
} projectile_group_t;

// For use in mobjinfo[].splash_group
typedef enum {
  SG_DEFAULT,
  SG_DEH_GROUP
} splash_group_t;

// NOALTSPEED
// Fields are init to 0, so unassigned must be 0.
// Store altspeed as +1.
#endif



// ---
// [WDJ] Thing ext information.
// Just a few things have this information.

#ifdef MBF21
# ifndef ENABLE_THING_EXT
#   define ENABLE_THING_EXT
# endif
#endif
//#ifdef DEHEXTRA
//# ifndef ENABLE_THING_EXT
//#   define ENABLE_THING_EXT
//# endif
//#endif

#ifdef ENABLE_THING_EXT
// [WDJ] To support alternative speed, and other mods.
// This only allocates if DEH invokes it.
// There are three mobjs which are init with alt speed.
typedef enum {
  MAI_alt_speed_deh = 0x0002,
} ma_flags_e;

typedef struct {
    uint16_t   mon_type;   // mobjtype_t
    uint16_t   ma_flags;   // monster alt flags,  mon_flags_e
    fixed_t    speed[2];   // normal, fast (MBF21 altspeed)
} deh_alt_info_t;

deh_alt_info_t *   get_alt_info( uint16_t mon_type );
deh_alt_info_t *   create_alt_info( uint16_t mon_type );
void  set_alt_speed( uint16_t mon_type, fixed_t norm_speed, fixed_t alt_speed );
void  apply_alt_info_speed( byte speed_select );
#endif

// To pass table to name_prefix_detect()
enum { COND_none, COND_numeric, COND_alpha };
typedef struct {
    const char * prefix;
    byte         condition;  // COND_xxx
    uint16_t     detflag;  // upon detect
} prefix_detect_t;

// From infoext.c
extern const prefix_detect_t prefix_detect_table[];
extern const byte num_prefix_detect_table;
extern uint16_t deh_detect;


void Init_info(void);
void init_tables_deh(void);
void P_PatchInfoTables( void );
// Things that are not reversible yet.
void init_tables_commit( void );

#ifdef ENABLE_DEH_REMAP
byte  is_predef_sprite( uint16_t sprite_id );
uint16_t  remap_sprite( uint16_t sprite_id, const char * name );
byte  is_predef_state( uint16_t state_id );
uint16_t  remap_state( uint16_t sprite_id );
// Get sprite index as in namelist.
// Return 0xFFFF when done.
uint16_t  get_remap_sprite( uint16_t index );

// From sounds.c
sfxid_t  remap_sound( uint16_t snd_index );

void zero_nomap_states( void );
#endif

const char * sprite_name( uint16_t  spr_index );

void zero_states( uint16_t s1, uint16_t s2 );

#ifdef MBF21
void  dsda_zero_state( uint16_t stnum );
#endif



#define STATE_FUNC_CHECK
#ifdef STATE_FUNC_CHECK
byte action_acp( action_fi_t ai );
byte lookup_state_acp( uint16_t state_index );
byte lookup_sprite_acp( uint16_t sprite_index );
#endif


#endif

