// Emacs style mode select   -*- C++ -*-f
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
// $Log: info.c,v $
// DESCRIPTION:
//      Extensions and patches to the tables.
//      Alterations due to game selected.
//
//-----------------------------------------------------------------------------

#include "infoext.h"
#include "info.h"
#include "doomincl.h"
  // doomdef
  // range_t
#include "p_local.h"
#include "d_items.h"
#include "p_mobj.h"
#include "m_fixed.h"
#include "sounds.h"
#include "action.h"
#include "m_argv.h"

void   S_InitRuntimeSounds (void);
void   S_InitRuntimeMusic (void);



#ifdef STATE_FUNC_CHECK

#ifdef DEBUG_ACTION

#define NUM_action_acp2  (sizeof(action_acp2_table)/sizeof(action_p2))

// Not used anymore, as action enum does this better.
// Return 0=none, 1=acp1, 2=acp2
byte lookup_action_acp( actionf_p2 * ap )
{
    if( ap == NULL || ap->acp2 == NULL )
      return 0;

    int i;
    for( i=0; i< NUM_action_acp2; i++ )
        if( action_acp2_table[i] == ap->acp2 )
        return 2;
    }
    return 1;
}
#endif

// Return 0=none, 1=acp1, 2=acp2
byte action_acp( action_fi_t ai )
{
    if( (ai >= AI_acp1_start) && (ai < AI_acp1_end) )
      return 1;

    if( (ai >= AP_acp2_start) && (ai < AP_acp2_end) )
      return 2;
    
    return 0;
}


// States that are hardcoded in code as acp2.
const range_t  state_acp2_table[] =
{
    {S_LIGHTDONE, S_BFGFLASH2},  // Doom
    {S_DUMMY2, S_DUMMY2},  // Doom FS
// Heretic    
    {S_HLIGHTDONE, S_GAUNTLETATK2_7},
    {S_BLASTERREADY, S_BLASTERATK2_6},
    {S_MACEREADY, S_MACEATK2_4},
    {S_HORNRODREADY, S_HORNRODATK2_9},
    {S_GOLDWANDREADY, S_GOLDWANDATK2_4},
    {S_PHOENIXREADY, S_PHOENIXATK2_4},
    {S_CRBOW1, S_CRBOWATK2_8},
};
#define NUM_state_acp2_table  (sizeof(state_acp2_table)/sizeof(range_t))

// Return 0=none, 1=acp1, 2=acp2
byte lookup_state_acp( uint16_t state_index )
{
    if( state_index == 0 || state_index > S_LEGACY_MAX )
      return 0;

    int i;
    int num = NUM_state_acp2_table;  // Heretic
    if( !EN_heretic )
    {
        num = 2;

        if( (state_index >= S_HERETIC_START) && (state_index <= S_HERETIC_MAX) )
            return 0;  // Doom in Heretic range, is unknown.
    }

    for( i=0; i< num; i++ )
    {
        const range_t * sr = & state_acp2_table[i];
        if( state_index >= sr->first && state_index <= sr->last )
            return 2;
    }
    return 1;  // could be either
}



// Sprites that are known to be acp2 (player weapons)
const range_t  sprite_acp2_table[] =
{
    // Doom
    // SHTG, PUNG, PISG, SHTG, SHTF, SHT2, CHGG, CHGF, MISG, MISF, SAWG, PLSG, PLSF, BFGG, BFGF
    {SPR_SHTG, SPR_BFGF},
    // Heretic
    // STFF, BEAK, GAUN, BLSR, MACE, HROD, GWND, PHNX, CRBW,
    {SPR_STFF, SPR_STFF},
    {SPR_BEAK, SPR_BEAK},
    {SPR_GAUN, SPR_GAUN},
    {SPR_BLSR, SPR_BLSR},
    {SPR_MACE, SPR_MACE},
    {SPR_HROD, SPR_HROD},
    {SPR_GWND, SPR_GWND},
    {SPR_PHNX, SPR_PHNX},
    {SPR_CRBW, SPR_CRBW},
};
#define NUM_sprite_acp2_table  (sizeof(sprite_acp2_table)/sizeof(range_t))


// Return 0=none, 1=acp1, 2=acp2
byte lookup_sprite_acp( uint16_t sprite_index )
{
    if( sprite_index > SPR_LEGACY_MAX )
      return 0;  // do not know
    
    int i;
    int num = NUM_sprite_acp2_table; // Heretic
    if( !EN_heretic )
    {
        num = 1;

        if( (sprite_index >= SPR_HERETIC_START) && (sprite_index <= SPR_HERETIC_MAX) )
            return 0;  // Doom in Heretic range, is unknown.
    }

    for( i=0; i< num; i++ )
    {
        const range_t * sr = & sprite_acp2_table[i];
        if( sprite_index >= sr->first && sprite_index <= sr->last )
            return 2;
    }
    return 1;
}

#endif


//---
// [WDJ] State ext storage.
state_ext_t *  state_ext = NULL;
static uint16_t  num_state_ext_alloc = 0;
static uint16_t  num_state_ext_used = 0;

// All unused state_ext are set to 0.
state_ext_t  empty_state_ext;  // init in P_clear_state_ext

state_ext_t *  P_state_ext( state_t * state )
{
    uint16_t seid = state->state_ext_id;
    if( (seid == 0) || (seid >= num_state_ext_used) )
        return & empty_state_ext;

    // state_ext_id=1 is at state_ext[1].
    return  &state_ext[ seid ];
}

#define STATE_EXT_INC  64
// Give it an state_ext.
state_ext_t *  P_create_state_ext( state_t * state )
{
    // [WDJ] Allocated state_ext id start at 1.
    if( state->state_ext_id == 0 )
    {
        // does not already have one
        if( num_state_ext_used >= num_state_ext_alloc )
        {
            // Grow the ext states
            int reqnum = num_state_ext_alloc + STATE_EXT_INC;
            state_ext_t * ns = (state_ext_t*)
                 realloc( state_ext, reqnum * sizeof(state_ext_t) );
            if( ns == NULL )
            {
                I_SoftError( "Realloc of states failed\n" );
                return &state_ext[0];  // reuse unused at [0]
            }
            state_ext = ns;
            memset( &ns[num_state_ext_alloc], 0, STATE_EXT_INC * sizeof(state_ext_t));
            num_state_ext_alloc = reqnum;
        }
        state->state_ext_id = num_state_ext_used++;  // starts at 1
        // this does not use [0]
    }

    return  &state_ext[ state->state_ext_id ];
}

// Clear and init the state_ext.
static
void  P_clear_all_state_ext( void )
{
    state_t * s;

    if( state_ext && (num_state_ext_alloc > 256) )
    {
        free( state_ext );
        state_ext = NULL;
    }

    if( state_ext == NULL )
    {
        num_state_ext_alloc = 64;
        state_ext = (state_ext_t*) malloc( num_state_ext_alloc * sizeof(state_ext_t) );
        memset( state_ext, 0, num_state_ext_alloc * sizeof(state_ext_t));
    }

    num_state_ext_used = 1;  // 0 is always dummy

    // Init all states to dummy.
    for( s = states; s < &states[NUMSTATES_EXT]; s++ )
       s->state_ext_id = 0;

    // Size differs, too hard to init in declaration.
    memset( &empty_state_ext, 0, sizeof(empty_state_ext) );
}


uint32_t player_default_flags1 = MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH;  // standard
uint32_t player_default_flags2 = MF2_WINDTHRUST | MF2_FOOTCLIP | MF2_SLIDE | MF2_PASSMOBJ | MF2_TELESTOMP;  // Heretic adopted
uint32_t mon_default_flags1 = MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL; // standard
uint32_t mon_default_flags2 = 0;  // Heretic adopted

#ifdef MBF21
// Only used for frame modify flags.  There are no other frame flags at this time.
//   clear_frame_flags : clear these flags first
//   set_frame_flags : set these flags
void  P_set_frame_flags( state_t * state, uint16_t clear_frame_flags, uint16_t set_frame_flags )
{
    uint16_t * frp = &(P_create_state_ext( state )->frflags);

    *frp &= ~(clear_frame_flags);
    *frp |= set_frame_flags;  // FRF_xx flags
}
#endif

//---
// [WDJ] Alt thing store.
// Settings in thing that only a few thing actually have.

#ifdef MBF21
// [WDJ] To support alternative speed, and other mods.
// To store info that is used only rarely, and not during game play.
// Kept separate to prevent degradation of the locality of reference in mobjinfo.
static  deh_alt_info_t * deh_alt_info;
static  uint16_t  deh_alt_info_num = 0;
static  uint16_t  deh_alt_info_alloc = 0;
#define deh_alt_info_INC   16

deh_alt_info_t *   get_alt_info( uint16_t mon_type )
{
    int i;
    for( i=0; i<deh_alt_info_num; i++ )
    {
        deh_alt_info_t * dap = & deh_alt_info[i];
        if( dap->mon_type == mon_type )
            return dap;  // found it
    }
    return NULL;
}

// Create a alt_info entry for this mobj.  If it already exists, then return that.
deh_alt_info_t *   create_alt_info( uint16_t mon_type )
{
    // Does it exist already.
    deh_alt_info_t * dap = get_alt_info( mon_type );
    if( dap )
        return dap;
    
    if( deh_alt_info_alloc <= deh_alt_info_num )
    {
        // Increase the allocation
        unsigned int req_alt_num = deh_alt_info_alloc + deh_alt_info_INC;
        deh_alt_info_t * dp = realloc( deh_alt_info, sizeof(deh_alt_info_t) * req_alt_num );
        if( dp == NULL )
            return NULL;

        deh_alt_info = dp;
        deh_alt_info_alloc = req_alt_num;
    }
    
    // Create new entry
    dap = & deh_alt_info[ deh_alt_info_num++ ];
    memset( dap, 0, sizeof(deh_alt_info_t) );
    dap->mon_type = mon_type;
    
    return dap;
}

void  set_alt_speed( uint16_t mon_type, fixed_t norm_speed, fixed_t alt_speed )
{
    deh_alt_info_t * dap = create_alt_info( mon_type ); // lookup existing or create.
    dap->ma_flags |= MAI_alt_speed_deh;  // MBF21 altspeed
    dap->speed[0] = norm_speed;  // normal speed
    dap->speed[1] = alt_speed;  // fast speed
}


//  select : 0=normal, 1=fast
void  apply_alt_info_speed( byte speed_select )
{
    // [WDJ] Change monsters that have alt_speed.
    // The alt_speed was set by dehacked into speed[1].
    // The current speed was saved in speed[0].
    // This is NOT like DSDA did it.
    int i;
    for( i=0; i<deh_alt_info_num; i++ )
    {
        deh_alt_info_t * dap = & deh_alt_info[i];
        if( dap->ma_flags & MAI_alt_speed_deh )
        {
            mobjinfo_t * mip = & mobjinfo[ dap->mon_type ];
            mip->speed = dap->speed[speed_select];
        }
    }
}
#endif


//---
// [WDJ] MBF21 info patches

#ifdef  MBF21
#define MELEERANGE   (64*FRACUNIT)

static void init_mbf21_info_range( int indx0, int indx_end )
{
    int i;
    for( i=indx0; i<indx_end; i++ )
    {
        mobjinfo_t * mi = & mobjinfo[i];
        mi->melee_range = MELEERANGE;
        mi->infight_group = IG_DEFAULT;
        mi->projectile_group = PG_DEFAULT;
        mi->splash_group = SG_DEFAULT;
        mi->rip_sound = sfx_None;
//#ifdef ENABLE_DEHEXTRA
//        mi->dropped_item = MT_NULL;  // DEHEXTRA
//#endif
#if 0
        mi->altspeed = NO_ALTSPEED;  // this field is in deh_alt_info_t
#endif
#if 0
        // Some other standard
        mi->visibility = VT_DOOM;
#endif
    }
}
#endif

//---
// [WDJ] DEH remapping
#ifdef ENABLE_DEH_REMAP
byte EN_state_remap = 5;  // auto
byte EN_sprite_remap = 5;  // auto
byte EN_sfx_remap = 5;    // auto
#endif
byte EN_state_zero = 2;  // auto

// [WDJ] Sprite remapping
#ifdef ENABLE_DEH_REMAP

// Free sprites, Doom
static
const range_t  sprite_free_table_doom[2] =
{
  { SPR_LEGACY_MAX + 1, NUMSPRITES_EXT - 1 },
  { SPR_HERETIC_START, SPR_AMC2 },  // protect SPR_AMS1 to SPR_AMB2 for odd sprite remapping
};
#define NUM_sprite_free_table_doom (sizeof(sprite_free_table_doom)/sizeof(range_t))

// Free sprites, Heretic
static
const range_t  sprite_free_table_heretic[4] =
{
  { SPR_LEGACY_MAX + 1, NUMSPRITES_EXT - 1 },
  { SPR_DOOM_START, SPR_BLUD-1 },  // SPR_BLUD is used for SPR_BLOD
  { SPR_BLUD+1, SPR_PLAY-1 },      // SPR_PLAY is used for SPR_PLAY
  { SPR_PLAY+1, SPR_DOOM_MAX },
};
#define NUM_sprite_free_table_heretic (sizeof(sprite_free_table_heretic)/sizeof(range_t))


static
const range_t *  sprite_free_table = NULL;
static byte  num_sprite_free_table = 0;
static byte  sprite_free_table_index = 0;

static range_t  free_sprite = {10, 0};  // invalid to trigger setup
static range_t  predef_sprite = {0, SPR_TNT1};
static range_t  predef_sprite_ext = {0, SPR_TNT1};
static range_t  nomap_sprite = {0, 0};

// last predef, for EN_sprite_remap settings
// Indexed by EN_sprite_remap.
static
uint16_t predef_sprite_doom_adapt[] =
{
    SPR_DOOM_MAX,  // 0 = off, does not matter
    SPR_LEGACY_MAX,  // almost everything fixed
    SPR_LEGACY_MAX,  // mostly fixed
    SPR_DOOM_MAX,  // doom stuff plus is fixed
    SPR_DOOM_MAX,  // all but predef doom is remapped
    SPR_DOOM_MAX,  // auto
};
static
range_t nomap_sprite_doom_adapt[] =
{
    {0, 0},  // 0 = off, does not matter
    {SPR_LEGACY_MAX+1, SPR_LEGACY_MAX + 128},  // almost everything fixed
    {SPR_LEGACY_MAX+1, SPR_LEGACY_MAX + 16},  // mostly fixed
    {SPR_DOOM_MAX+1, SPR_DOOM_MAX + 64},  // doom stuff plus is fixed
    {0xFFFF, 0},  // all but predef doom is remapped
    {0xFFFF, 0},  // auto
};
static
uint16_t predef_sprite_heretic_adapt[] =
{
    SPR_DOOM_MAX,  // 0 off, does not matter
    SPR_LEGACY_MAX,  // 1 almost everything fixed
    SPR_HERETIC_MAX,  // mostly fixed
    SPR_HERETIC_MAX,  // heretic stuff plus is fixed
    SPR_HERETIC_MAX,  // all but predef doom is remapped
    SPR_HERETIC_MAX,  // auto
};
static
range_t nomap_sprite_heretic_adapt[] =
{
    {0, 0},  // 0 = off, does not matter
    {0, SPR_LEGACY_MAX + 128},  // almost everything fixed
    {0, SPR_HERETIC_MAX + 128}, // mostly fixed
    {0, SPR_HERETIC_MAX + 64},  // Heretic stuff plus is fixed
    {0xFFFF, 0},  // all but predef Heretic is remapped
    {0xFFFF, 0},  // auto
};


typedef struct {
    char name[5];    // sprite name
    uint16_t wad_spr_id;  // sprite id in wad
    uint16_t spr_index;   // sprite id remapped
} sprite_remap_t;

static sprite_remap_t  * sprite_remap = NULL;
//static uint16_t  sprite_remap_0 = 0;
static uint16_t  sprite_remap_used = 0;
static uint16_t  sprite_remap_alloc = 0;

#define SPRITE_REMAP_INC  128

static
void  sprite_remap_allocate( void )
{
    uint16_t req_num = sprite_remap_alloc + SPRITE_REMAP_INC;
    sprite_remap_t *  srnew = realloc( sprite_remap, req_num * sizeof( sprite_remap_t ) );
    if( srnew == NULL )
        return;

    // Init to 0xFFFF, so that sprite id are invalid.
    memset( &srnew[sprite_remap_used], 0xFF, (req_num - sprite_remap_used) * sizeof(sprite_remap_t) );  // zero the new
    sprite_remap = srnew;
    sprite_remap_alloc = req_num;
}

byte  is_predef_sprite( uint16_t sprite_id )
{
    if( (sprite_id == 0)
        || ((sprite_id >= predef_sprite.first) && (sprite_id <= predef_sprite.last))
        || ((sprite_id >= predef_sprite_ext.first) && (sprite_id <= predef_sprite_ext.last)) )
         return 1;

    return 0;
}

static
uint16_t  get_free_sprite( uint16_t sprite_id )
{
    // If current mapping allocation is used up.
    while( free_sprite.first > free_sprite.last )
    {
        if( sprite_free_table_index >= num_sprite_free_table ) // max entries in table
        {
            GenPrintf(EMSG_warn, "Sprite %i: remapping exceeds available free space: %i\n", sprite_id, free_sprite.last );
            return free_sprite.last;  // reuse last map
        }

        // Next free section of sprites.
        free_sprite = sprite_free_table[sprite_free_table_index++];
        // this might remove entire free space, so must loop
        if( free_sprite.first < predef_sprite.first )
        {
            // Free sprite block is before predef.
            if( free_sprite.last >= predef_sprite.first )
                free_sprite.last = predef_sprite.first - 1;
        }
        else
        {
            // Free state block is after predef.
            if( free_sprite.first <= predef_sprite.last )
                free_sprite.first = predef_sprite.last + 1;
            if( nomap_sprite.last && (free_sprite.first <= nomap_sprite.last) )
                free_sprite.first = nomap_sprite.last + 1;
        }
    }

    return free_sprite.first++ ;
}


#define NAME4_AS_NUM( n )  (*((uint32_t*)(n)))

// Detect sprite for current gamemode.
// Remap other sprites to the free sprite space.
uint16_t  remap_sprite( uint16_t sprite_id, const char * name )
{
    if( is_predef_sprite( sprite_id ) )
        return sprite_id;

    if( sprite_remap )
    {
        uint32_t namenum = (name)? NAME4_AS_NUM(name) : 0;
        int i;
        for( i = 0; i < sprite_remap_used; i++ )
        {
            sprite_remap_t * sr = & sprite_remap[ i ];
            if( (sr->wad_spr_id == sprite_id)
                || ( NAME4_AS_NUM(sr->name) == namenum) )
                return sr->spr_index;
        }
    }

    if( sprite_id == 0xFFFF )
        return 0xFFFF;  // do not create new entry
    
    if( sprite_remap_used >= sprite_remap_alloc )
        sprite_remap_allocate();

    // Fill in new entry.
    sprite_remap_t * sr = & sprite_remap[ sprite_remap_used++ ];
    sr->name[0] = name[0];
    sr->name[1] = name[1];
    sr->name[2] = name[2];
    sr->name[3] = name[3];
    sr->name[4] = 0;
    sr->wad_spr_id = sprite_id;

    if( (sprite_id >= nomap_sprite.first) && (sprite_id <= nomap_sprite.last) )
    {
        // not mapped
        sr->spr_index = sprite_id;
    }
    else
    {
        // get free sprite
        sr->spr_index = get_free_sprite( sprite_id );
    }

    if( verbose )
        GenPrintf(EMSG_info, "Sprite %i %s: mapped to %i\n", sprite_id, name, sr->spr_index );
    return sr->spr_index;
}

// Lookup by index
const char * sprite_name( uint16_t  spr_index )
{
    if( is_predef_sprite( spr_index ) )
       return sprnames[ spr_index ];

    // Lookup the sprite in the remap.
    if( sprite_remap )
    {
        int i;
        for( i = 0; i < sprite_remap_used; i++ )
        {
            sprite_remap_t * sr = & sprite_remap[ i ];
            if( sr->spr_index == spr_index )
                return sr->name;
        }
    }

    return "UNKNOWN";
}

// Get sprite index as in namelist.
// Return 0xFFFF when done.
uint16_t  get_remap_sprite( uint16_t index )
{
    if( index >= sprite_remap_used )
        return 0xFFFF;

    // Unused entries are 0xFFFF
    return sprite_remap[ index ].spr_index;
}

#else

// Default sprite name lookup.
const char * sprite_name( uint16_t  spr_index )
{
    return sprnames[ spr_index ];
}
#endif


// --- States remap

#ifdef ENABLE_DEH_REMAP
static range_t  predef_state = {0, S_TNT1};
static range_t  predef_state_ext = {0, S_TNT1};
static range_t  nomap_state = {0, 0};
#endif


#ifdef MBF21
// [WDJ] This is one the horrible kludges that is forced upon
// anyone implementing MBF21, because wads that claim to be MBF21
// are really dependent upon DSDA kludges.
// This is not in the MBF21 spec.
void  dsda_zero_state( uint16_t stnum )
{
    if( stnum < NUMSTATES_EXT )
    {
        // Do not know what effect these dummy values have upon any wad.
        // The problem shows up as undefined behavior when wads depend upon them.
        state_t *  st = & states[stnum];
        st->sprite = SPR_TNT1;
        st->tics = -1;
        st->nextstate = stnum;  // stay put, instead of jump to state 0
    }
}
#endif

void  zero_states( uint16_t s1, uint16_t s2 )
{
    // Some wads depend upon some states being blank.
    int sj;
    byte zdefnxt = 0;

    if( EN_state_zero < 2 )
        zdefnxt = EN_state_zero;
    else
        zdefnxt = ((all_detect & (BDTC_MBF21|BDTC_dehextra|BDTC_dsda)) != 0);

    GenPrintf( EMSG_info, "Zero states default: sprite=%s next=%s\n",
               zdefnxt? "TNT1":"0", zdefnxt? "here":"0" );

    // [WDJ] sleepwalking.wad requires default state to have SPR_TNT1, and nextstate=sj,
    // which is typical of MBF21 wads developed on DSDA.
//    if( s1 >= NUMSTATES_EXT )  s1 = NUMSTATES_EXT - 1;
    if( s2 >= NUMSTATES_EXT )  s2 = NUMSTATES_EXT - 1;
    for( sj = s1; sj <= s2; sj++ )
    {
        state_t *  st = & states[sj];
//        memcpy( st, &states[S_BLANK], sizeof(state_t) );
        st->sprite = (devparm)? 0: SPR_TNT1;
        st->frame = 0;
        st->action = AI_NULL;
        st->nextstate = sj;
#ifdef MBF21
        // And DSDA dependent wads, depend upon this.
        if( zdefnxt )
            dsda_zero_state( sj );
#endif
    }
}

#ifdef ENABLE_DEH_REMAP
void zero_nomap_states( void )
{
#if 1
    // Zero the nomap, always after the predef.
    if( nomap_state.last > nomap_state.first )
        zero_states( nomap_state.first, nomap_state.last );
#else
    // nomap area is now always after predef.
    uint16_t zero_first = nomap_state.first;

    // States
    if( nomap_state.first <= predef_state.first )
    {
        // There is some nomap before the predef (Heretic)
        zero_states( nomap_state.first, predef_state.first - 1 );
        zero_first = predef_state.last + 1;
    }

    // The part of nomap after the predef
    if( nomap_state.last && zero_first < 0xFFF1 )
        zero_states( zero_first, nomap_state.last );
#endif
}
#endif


#ifdef ENABLE_DEH_REMAP
// Free states, Doom
static
const range_t  state_free_table_doom[2] =
{
  // Use the usual free space first.
  { NUMSTATES_DEF, NUMSTATES_EXT - 1 },
  { S_HERETIC_START, S_HERETIC_MAX },
};
#define NUM_state_free_table_doom (sizeof(state_free_table_doom)/sizeof(range_t))

// Free states, Heretic
static
const range_t  state_free_table_heretic[5] =
{
  // Use the usual free space first.
  { NUMSTATES_DEF, NUMSTATES_EXT - 1 },
  { S_DOOM_START, S_BLOOD1-1 },
  { S_BLOOD3+1, S_PLAY-1 },
  { S_PLAY+1, S_BLOODSPLATTER1-1 },
  { S_BLOODSPLATTERX+1, S_TECHLAMP4 },
};
#define NUM_state_free_table_heretic (sizeof(state_free_table_heretic)/sizeof(range_t))

#ifdef PROTECT_STATE
const range_t  state_protect[] = {
  { S_DUMMY, S_LEGACY_MAX },  // level 1
//  { SPR_DOOM_MAX, SPR_LEGACY_MAX },  // level 2   This overlaps free table, cannot handle that yet
};
#endif

static
const range_t *  state_free_table = NULL;
static byte  num_state_free_table = 0;
static byte  state_free_table_index = 0;
static range_t  free_state = {10, 0};  // invalid to trigger setup

// predef for EN_state_remap settings
// first is always S_DOOM_START
// Indexed by EN_state_remap.
static
uint16_t predef_state_doom_adapt[] =
{
    0,  // 0 off, does not matter
    S_LEGACY_MAX,  // almost everything fixed
    S_LEGACY_MAX,  // mostly fixed
    S_DOOM_MAX,  // doom stuff plus is fixed
    S_DOOM_MAX,  // all but predef doom is remapped
    S_DOOM_MAX,  // auto
};
static
range_t nomap_state_doom_adapt[] =
{
    {0, 0},  // 0 off, does not matter
    {S_LEGACY_MAX+1, S_LEGACY_MAX + 128},  // almost everything fixed
    {S_LEGACY_MAX+1, S_LEGACY_MAX + 16},  // mostly fixed
    {S_DOOM_MAX+1, S_DOOM_MAX + 64},    // doom stuff plus is fixed
    {0xFFFF, 0},  // all but predef doom is remapped
    {0xFFFF, 0},  // auto
};
// first is always S_HERETIC_START
static
uint16_t predef_state_heretic_adapt[] =
{
    0,  // 0 off, does not matter
    S_LEGACY_MAX,  // almost everything fixed
    S_LEGACY_MAX,  // mostly fixed
    S_HERETIC_MAX,  // Heretic stuff plus is fixed
    S_HERETIC_MAX,  // all but predef Heretic is remapped
    S_HERETIC_MAX,  // auto
};
static
range_t nomap_state_heretic_adapt[] =
{
    {0, 0},  // 0 off, does not matter
    {S_LEGACY_MAX+1, S_LEGACY_MAX + 128},  // almost everything fixed
    {S_LEGACY_MAX+1, S_LEGACY_MAX + 16},  // mostly fixed
    {S_HERETIC_MAX+1, S_HERETIC_MAX + 64},  // Heretic stuff plus is fixed
    {0xFFFF, 0},  // all but predef Heretic is remapped
    {0xFFFF, 0},  // auto
};


typedef struct {
    uint16_t wad_state_id;  // state id in wad
    uint16_t state_index;   // state id remapped
} state_remap_t;

static state_remap_t  * state_remap = NULL;
//static uint16_t  state_remap_0 = 0;
static uint16_t  state_remap_used = 0;
static uint16_t  state_remap_alloc = 0;

#define STATE_REMAP_INC  128

static
void  state_remap_allocate( void )
{
    uint16_t req_num = state_remap_alloc + STATE_REMAP_INC;
    state_remap_t *  srnew = realloc( state_remap, req_num * sizeof( state_remap_t ) );
    if( srnew == NULL )
        return;

    memset( &srnew[state_remap_used], 0, (req_num - state_remap_used) * sizeof(state_remap_t) );  // zero the new
    state_remap = srnew;
    state_remap_alloc = req_num;
}

byte  is_predef_state( uint16_t state_id )
{
    if( (state_id == 0)
        || ((state_id >= predef_state.first) && (state_id <= predef_state.last))
        || ((state_id >= predef_state_ext.first) && (state_id <= predef_state_ext.last)) )
        return 1;
    
    return 0;
}

static
uint16_t  get_free_state( uint16_t state_id )
{
    // If current mapping allocation is used up.
    while( free_state.first > free_state.last )
    {
        if( state_free_table_index >= num_state_free_table ) // max entries in table
        {
            GenPrintf(EMSG_warn, "State %i: remapping exceeds available free space: %i\n", state_id, free_state.last );
            return free_state.last;  // reuse last map
        }

        // Next free section of sprites.
        free_state = state_free_table[state_free_table_index++];
        // this might remove entire free space, so must loop
        if( free_state.first < predef_state.first )
        {
            // Free state block is before predef.
            if( free_state.last >= predef_state.first )
                free_state.last = predef_state.first - 1;
        }
        else
        {
            // Free state block is after predef.
            if( free_state.first <= predef_state.last )
                free_state.first = predef_state.last + 1;
            if( nomap_state.last && (free_state.first <= nomap_state.last) )
                free_state.first = nomap_state.last + 1;
        }

        if( free_state.first <= free_state.last )
            zero_states( free_state.first, free_state.last );
    }
    
    return free_state.first ++;
}

// Detect state for current gamemode.
// Remap other states to the free state space.
uint16_t  remap_state( uint16_t state_id )
{
    if( is_predef_state( state_id ) )
        return state_id;

    if( state_remap )
    {
        // Check if in remap table already.
        int i;
        for( i = 0; i < state_remap_used; i++ )
        {
            state_remap_t * sr = & state_remap[ i ];
            if( sr->wad_state_id == state_id )
                return sr->state_index;
        }
    }

    if( state_id == 0xFFFF )
        return 0xFFFF;  // do not create new entry

    // Enter into the remap table.
    if( state_remap_used >= state_remap_alloc )
        state_remap_allocate();

    // Fill in new entry.
    state_remap_t * sr = & state_remap[ state_remap_used++ ];
    sr->wad_state_id = state_id;
    if( (state_id >= nomap_state.first) && (state_id <= nomap_state.last) )
    {
        // not mapped, already zeroed
        sr->state_index = state_id;
//        zero_states( state_id, state_id );  // zero state fix
    }
    else
    {
        // get free state
        sr->state_index = get_free_state( state_id );
//        zero_states( sr->state_index, sr->state_index );
    }

    if( verbose )
        GenPrintf(EMSG_info, "State %i: mapped to %i\n", state_id, sr->state_index );
    return sr->state_index;
}

#endif // ENABLE_DEH_REMAP


// To pass prefix table to W_name_prefix_detect, which is called right before
// processing dehacked lumps in that file.
// See prefix_detect_t in w_wad.h
const prefix_detect_t  prefix_detect_table[] =
{
    { "SP",    COND_numeric, BDTC_dehextra },  // sprites
    { "DSFRE", COND_numeric, BDTC_dehextra },  // sounds
    { "DSSFX", COND_numeric, BDTC_legacy },
};
const byte num_prefix_detect_table = (sizeof(prefix_detect_table)/sizeof(prefix_detect_t));


// This is reversible, within launcher loop.
void init_tables_deh( void )
{
    // [WDJ] Detect special lumps, from W_name_prefix_detect.

#ifdef ENABLE_DEH_REMAP
    EN_sfx_remap = EN_sprite_remap = EN_state_remap = 5;  // auto

    if( gamemode == chexquest1 )
    {
        // No remap for Chexquest, newmaps.
        EN_sfx_remap = EN_sprite_remap = EN_state_remap = 0;
    }


    int p = M_CheckParm("-remap");
    if( p && ((p+1)<myargc))
    {
        EN_state_remap = atoi(myargv[p + 1]);
        EN_sfx_remap = EN_sprite_remap = EN_state_remap;
    }
    p = M_CheckParm("-sprite_remap");
    if( p && ((p+1)<myargc))
    {
        EN_sprite_remap = atoi(myargv[p+1]);
    }
    p = M_CheckParm("-state_remap");
    if( p && ((p+1)<myargc))
    {
        EN_state_remap = atoi(myargv[p+1]);
    }
    p = M_CheckParm("-sfx_remap");
    if( p && ((p+1)<myargc))
    {
        EN_sfx_remap = atoi(myargv[p+1]);
    }
    if( EN_state_remap < 0 )
        EN_state_remap = 0;
    if( EN_state_remap > 5 )
        EN_state_remap = 5;
    if( EN_sprite_remap < 0 )
        EN_sprite_remap = 0;
    if( EN_sprite_remap > 5 )
        EN_sprite_remap = 5;
    if( EN_sfx_remap < 0 )
        EN_sfx_remap = 0;
    if( EN_sfx_remap > 5 )
        EN_sfx_remap = 5;

    p = M_CheckParm("-state_zero");
    if( p && ((p+1)<myargc))
    {
        EN_state_zero = atoi(myargv[p+1]);
    }
    if( EN_state_zero > 2 )
        EN_state_zero = 2;  // auto

    sprite_free_table_index = 0;
    state_free_table_index = 0;

    if( EN_heretic )
    {
        sprite_free_table = & sprite_free_table_heretic[0];
        num_sprite_free_table = NUM_sprite_free_table_heretic;
        predef_sprite.first = SPR_HERETIC_START;
        predef_sprite.last = predef_sprite_heretic_adapt[EN_sprite_remap];
        predef_sprite_ext.first = SPR_DOGS;
        predef_sprite_ext.last = SPR_LEGACY_MAX;
        nomap_sprite = nomap_sprite_heretic_adapt[EN_state_remap];

        state_free_table = & state_free_table_heretic[0];
        num_state_free_table = NUM_state_free_table_heretic;
        predef_state.first = S_HERETIC_START;
        predef_state.last = predef_state_heretic_adapt[EN_state_remap];
        predef_state_ext.first = S_DUMMY;
        predef_state_ext.last = S_LEGACY_MAX;
        nomap_state = nomap_state_heretic_adapt[EN_state_remap];
    }
    else
    {
        sprite_free_table = & sprite_free_table_doom[0];
        num_sprite_free_table = NUM_sprite_free_table_doom;
        predef_sprite.first = SPR_DOOM_START;
        predef_sprite.last = predef_sprite_doom_adapt[EN_sprite_remap];
        predef_sprite_ext.first = SPR_DOGS;
        predef_sprite_ext.last = SPR_LEGACY_MAX;
        nomap_sprite = nomap_sprite_doom_adapt[EN_state_remap];

        state_free_table = & state_free_table_doom[0];
        num_state_free_table = NUM_state_free_table_doom;
        predef_state.first = S_DOOM_START;
        predef_state.last = predef_state_doom_adapt[EN_state_remap];
        predef_state_ext.first = S_DUMMY;
        predef_state_ext.last = S_LEGACY_MAX;
        nomap_state = nomap_state_doom_adapt[EN_state_remap];
    }

    // Leave something for remap and DEHEXTRA.
    if( nomap_sprite.last >= NUMSPRITES_EXT-4 )
        nomap_sprite.last = NUMSPRITES_EXT-4;
    // There are 4 spare states.
    if( nomap_state.last >= NUMSTATES_EXT-4 )
        nomap_state.last = NUMSTATES_EXT-4;
#endif

    // initialize free sfx slots for skin sounds, and dehacked.
    S_InitRuntimeSounds();
    S_InitRuntimeMusic();
}

// Things that are not reversible yet.
void init_tables_commit( void )
{
    zero_nomap_states();
}



//---
// [WDJ] Info and things tables patches.

// From p_heretic.c
void  doomednum_install( byte set_heretic );
void  sfx_ammo_text_install( byte set_heretic );


// Doom text ranges that get modified by Heretic and ChexQuest.
range_t  doom_text_range[] =
{
    {QUITMSG_NUM, QUIT2MSG6_NUM},
    {HUSTR_E1M1_NUM, HUSTR_E1M5_NUM},
    {GOTARMOR_NUM, GOTSUPER_NUM},
    {GOTINVUL_NUM, GOTSUIT_NUM},
    {GOTBLUECARD_NUM, GOTREDCARD_NUM},
    {GOTVISOR_NUM, GOTSHOTGUN2_NUM},
    {STSTR_DQDON_NUM, STSTR_FAADDED_NUM},
    {STSTR_CHOPPERS_NUM, STSTR_CHOPPERS_NUM},
    {E1TEXT_NUM, E4TEXT_NUM},
    {NIGHTMARE_NUM, NIGHTMARE_NUM},
    {DEATHMSG_SUICIDE, DEATHMSG_DEAD},
};
#define NUM_doom_text_range  (sizeof(doom_text_range)/sizeof(range_t))

// Initial tests saved 165 strings.
#define NUM_doom_text_save  170
static char *  doom_text[ NUM_doom_text_save ];


// [WDJ] Save and restore Doom text, so that changes are reversible.
// Because, we cannot trust the user to not change the gamemode serveral
// times when in the launcher.
//   save_flag: 0 = restore, 1 = save
static
void save_restore_doom_text( byte save_flag )
{
    range_t * range = doom_text_range;
    uint16_t st = 0;
    int i;
    for( i=0; i<NUM_doom_text_range; i++ )
    {
        uint16_t tt;
        for( tt=range->first; tt<=range->last; tt++ )
        {
            if( save_flag )
                doom_text[st++] = text[tt];  // save
            else
                text[tt] = doom_text[st++];  // restore

            if( st >= NUM_doom_text_save )
            {
                I_SoftError( "Save doom text area too small, Undo unavailable.\n" );
                return;
            }
        }
    }
    if( verbose > 1 )
        GenPrintf( EMSG_info, "Save Doom text: used %i of %i\n", st, NUM_doom_text_save );
}



void Heretic_PatchEngine(void);
void Chex1_PatchEngine(void);

enum patched_e { PE_none, PE_heretic, PE_chex1 };
static byte patched_by = 0;


// Before load dehacked files.
// patch the info mobjinfo table and state table for new legacy needs
// actualy only time is recomputed for newticrate
void P_PatchInfoTables( void )
{
    // Have EN_heretic, EN_doom, set from G_set_gamemode.
    // But there is no EN_chex, so test gamemode itself.

    // Need to be common, reversible,
    // Each choice has patch tables, one patch engine.

    if (gamemode == heretic)
    {
        // Patch with Heretic info
        save_restore_doom_text( 1 ); // Save doom text first

        Heretic_PatchEngine();
#ifdef FRENCH_INLINE
        french_heretic();
#endif
        patched_by = PE_heretic;
    }
    else if(gamemode == chexquest1)
    {
        // Patch with Chex1 info
        save_restore_doom_text( 1 ); // Save doom text first

        Chex1_PatchEngine();
#ifdef FRENCH_INLINE
        french_chexquest();
#endif
        patched_by = PE_chex1;
    }
    else
    {
        // Restore Doom info
        if( patched_by
            && doom_text[0] )  // saved text is present
        {
            save_restore_doom_text( 0 ); // Restore doom text first
        }

        if( patched_by == PE_heretic )
        {
            // These are things that Heretic or Chex1 altered.
            mobjinfo[MT_TFOG].spawnstate = S_TFOG;
            sprnames[SPR_BLUD] = "BLUD";

            S_music[mus_inter].name = "inter\0";

            sfx_ammo_text_install( 0 ); // install Doom sfx, ammo
   
            // Restore the Doom doomednum.
            doomednum_install( 0 );  // install Doom doomednum
        }

        if( patched_by == PE_chex1 )
        {
            // Undo Chex changes
            //patch monster changes
            mobjinfo[MT_POSSESSED].missilestate = S_POSS_ATK1;
            mobjinfo[MT_POSSESSED].meleestate = 0;

            mobjinfo[MT_SHOTGUY].missilestate = S_SPOS_ATK1;
            mobjinfo[MT_SHOTGUY].meleestate = 0;

            mobjinfo[MT_BRUISER].speed = 8;
            mobjinfo[MT_BRUISER].radius = FIXINT(24);

            //coronas
            sprite_light[1].dynamic_color.rgba = RGBA(0x60,0x77,0xf7,0x20);
            sprite_light[2].dynamic_color.rgba = RGBA(0x60,0x77,0xf7,0x40);
            sprite_light[3].dynamic_color.rgba = RGBA(0xf7,0xf7,0x20,0x40);
            sprite_light[4].dynamic_color.rgba = RGBA(0xf7,0xf7,0x20,0x60);
            sprite_light[5].dynamic_color.rgba = RGBA(0x60,0xf0,0x60,0x80);
            sprite_light[6].dynamic_color.rgba = RGBA(0x60,0xf0,0x60,0x60);
            sprite_light[7].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[8].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[9].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[10].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[11].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[12].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[15].light_yoffset = 19;  //light
            sprite_light[16].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;
            sprite_light[17].splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_fire;

#ifdef HWRENDER
        // These changes were executed after HWR_StartupEngine(), so reinit.
        // Got moved, this is now before
//        HWR_Init_Light();
#endif
        }

        patched_by = PE_none;  // Doom normal
    }
    
    int i;
    for (i=0;i<NUMMOBJTYPES;i++)
    {
        mobjinfo[i].reactiontime *= NEWTICRATERATIO;
        //mobjinfo[i].speed        /= NEWTICRATERATIO;
#ifdef ENABLE_DEHEXTRA
        mobjinfo[i].dropped_item = MT_NULL;
#endif
    }
#if defined DEBUG_WINDOWED || defined PARANOIA
    if( NUMSTATES_EXT >= 0xFFFE )
        I_Error( "NUMSTATES exceeds 16 bits\n" );
#endif
    for (i=0;i<NUMSTATES_EXT;i++)
    {
        states[i].tics *= NEWTICRATERATIO;
    }

#ifdef MBF21
    init_mbf21_info_range( 0, NUMMOBJTYPES );

    // [WDJ] MBF21 moved hardcoded tests into info table.
    // (tm_thing->target->type == MT_KNIGHT  && thing->type == MT_BRUISER)
    // (tm_thing->target->type == MT_BRUISER && thing->type == MT_KNIGHT)
    mobjinfo[MT_BRUISER].projectile_group = PG_BARON;
    mobjinfo[MT_KNIGHT].projectile_group = PG_BARON;
    
    set_alt_speed( MT_BRUISERSHOT, FIXINT(15), FIXINT(20) );
    set_alt_speed( MT_HEADSHOT, FIXINT(10), FIXINT(20) );
    set_alt_speed( MT_TROOPSHOT, FIXINT(10), FIXINT(20) );
#endif

#ifdef ENABLE_DEHEXTRA
    mobjinfo[MT_POSSESSED].dropped_item = MT_CLIP;
    mobjinfo[MT_SHOTGUY].dropped_item = MT_SHOTGUN;
    mobjinfo[MT_CHAINGUY].dropped_item = MT_CHAINGUN;
    mobjinfo[MT_WOLFSS].dropped_item = MT_CLIP;
#endif
}



// Called early, before launcher.
// Command line is set.
// Many vars, like gamemode, can be changed by launcher.
void Init_info(void)
{
    doom_text[0] = NULL;  // flag that save is not present.

    P_clear_all_state_ext();  // init state_ext
}

