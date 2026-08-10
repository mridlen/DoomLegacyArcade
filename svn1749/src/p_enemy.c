// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_enemy.c 1741 2025-03-23 04:28:43Z wesleyjohnson $
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
// $Log: p_enemy.c,v $
// Revision 1.20  2004/07/27 08:19:36  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.19  2002/11/30 18:41:20  judgecutor
//
// Revision 1.18  2002/09/27 16:40:09  tonyd
// First commit of acbot
//
// Revision 1.17  2002/09/17 21:20:03  hurdler
// Quick hack for hacx freeze
//
// Revision 1.16  2002/08/13 01:14:20  ssntails
// A_BossDeath fix (I hope!)
//
// Revision 1.15  2002/07/24 22:37:31  ssntails
// Fix for Keen deaths in custom maps.
//
// Revision 1.14  2001/07/28 16:18:37  bpereira
// Revision 1.13  2001/05/27 13:42:47  bpereira
//
// Revision 1.12  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.11  2001/03/30 17:12:50  bpereira
//
// Revision 1.10  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.9  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.8  2000/10/21 08:43:30  bpereira
// Revision 1.7  2000/10/08 13:30:01  bpereira
// Revision 1.6  2000/10/01 10:18:17  bpereira
// Revision 1.5  2000/04/30 10:30:10  bpereira
//
// Revision 1.4  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
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
//      Enemy thinking, AI.
//      Action Pointer Functions
//      that are associated with states/frames.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "p_inter.h"
  // P_KillMobj
#include "g_game.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
#include "t_script.h"
#include "dehacked.h"
#include "infoext.h"


#include "hardware/hw3sound.h"


void A_Fall(mobj_t* actor);
static void FastMonster_OnChange(void);
void A_FaceTarget(mobj_t* actor);
void CV_monster_OnChange(void);

// enable the solid corpses option : still not finished
consvar_t cv_solidcorpse =
  {"solidcorpse","0", CV_NETVAR | CV_SAVE, CV_OnOff};
consvar_t cv_fastmonsters =
  {"fastmonsters","0", CV_NETVAR | CV_CALL, CV_OnOff, FastMonster_OnChange};
consvar_t cv_predictingmonsters =
  {"predictingmonsters","0", CV_NETVAR | CV_SAVE, CV_OnOff};	//added by AC for predmonsters

#ifdef MONSTER_VARY
#if 0
void  setup_vary( void );
#endif

CV_PossibleValue_t mon_vary_cons_t[]={
   {0,"Off"},
   {1,"All"},
   {2,"Small"},
   {3,"Smaller"},
   {4,"Light"},
   {5,"Lighter"},
   {6,"MedSmall"},
   {7,"Medium"},
   {8,"MedLarge"},
   {9,"Heavier"},
   {10,"Heavy"},
   {11,"Bigger"},
   {12,"Big"},
   {0,NULL}};
#if 0
consvar_t cv_monster_vary =
  {"monster_vary","0", CV_NETVAR | CV_SAVE | CV_CALL, mon_vary_cons_t, setup_vary};
#else
consvar_t cv_monster_vary =
  {"monster_vary","0", CV_NETVAR | CV_SAVE, mon_vary_cons_t};
#endif

CV_PossibleValue_t vary_percent_cons_t[]={
   {1,"10"},
   {2,"20"},
   {3,"30"},
   {4,"40"},
   {5,"50"},
   {6,"60"},
   {7,"70"},
   {8,"80"},
   {9,"90"},
   {0,NULL}};
consvar_t cv_vary_percent =
  {"vary_percent","5", CV_NETVAR | CV_SAVE, vary_percent_cons_t };

CV_PossibleValue_t vary_size_cons_t[]={
   {1,"x1"},
   {2,"x2"},
   {3,"x3"},
   {4,"x4"},
   {5,"x5"},
   {0,NULL}};
consvar_t cv_vary_size =
  {"vary_size","3", CV_NETVAR | CV_SAVE, vary_size_cons_t };
#endif

#ifdef ENABLE_SLOW_REACT
CV_PossibleValue_t slow_react_cons_t[]={ {0,"MIN"}, {15,"MAX"}, {0,NULL} };
consvar_t cv_slow_react =
  {"slow_react","0", CV_NETVAR | CV_SAVE, slow_react_cons_t };
#endif


CV_PossibleValue_t mongravity_cons_t[]={
   {0,"Off"},
   {1,"MBF"},
   {2,"On"},
   {0,NULL}};
consvar_t cv_monstergravity =
  {"monstergravity","2", CV_NETVAR | CV_SAVE, mongravity_cons_t };

// DarkWolf95: Monster Behavior
// Values saved in demo, so do not change existing values.
CV_PossibleValue_t monbehavior_cons_t[]={
   {0,"Normal"},
   {1,"Coop"},
   {8,"No Infight"},
   {2,"Infight"},
   {6,"Full Infight"},
   {3,"Force Coop"},
   {5,"Force No Infight"},
   {4,"Force Infight"},
   {7,"Force Full Infight"},
   {0,NULL}};
consvar_t cv_monbehavior =
  { "monsterbehavior", "0", CV_NETVAR | CV_SAVE | CV_CALL, monbehavior_cons_t, CV_monster_OnChange };

// Doom normal is infight, no coop (see Boom).
// Indexed by monbehavior_cons_t values.
static byte monbehav_to_infight[] =
{
  INFT_infight,      // 0  Normal  (infight)
  INFT_coop,         // 1  Coop default
  INFT_infight,      // 2  Infight default
  INFT_coop | INFT_force,         // 3  Coop forced
  INFT_infight | INFT_force,      // 4  Infight forced
  INFT_none | INFT_force,         // 5  No Infight forced
  INFT_full_infight, // 6  Full Infight default
  INFT_full_infight | INFT_force, // 7  Full Infight forced
  INFT_none,         // 8  No Infight
};

CV_PossibleValue_t monsterfriction_t[] = {
   {0,"None"},
   {1,"MBF"},
   {2,"Momentum"},
   {3,"Heretic"},
   {20,"Normal"},
   {0,NULL} };
consvar_t cv_monsterfriction =
  {"monsterfriction","2", CV_NETVAR | CV_SAVE | CV_CALL, monsterfriction_t, CV_monster_OnChange};

// Monster stuck on door edge
CV_PossibleValue_t doorstuck_t[] = {
   {0,"None"},
   {1,"MBF"},
   {2,"Boom"},
   {0,NULL} };
consvar_t cv_doorstuck =
  {"doorstuck","2", CV_NETVAR | CV_SAVE | CV_CALL, doorstuck_t, CV_monster_OnChange};



typedef enum
{
    DI_EAST,
    DI_NORTHEAST,
    DI_NORTH,
    DI_NORTHWEST,
    DI_WEST,
    DI_SOUTHWEST,
    DI_SOUTH,
    DI_SOUTHEAST,
    DI_NODIR,
    NUMDIRS
} dirtype_t;


//
// P_NewChaseDir related LUT.
//

// [WDJ] Speed comparison of Opposite.
//  Table: 35 sec,  Calculated: 33 sec.
#if 0
static dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};
#define  DI_OPPOSITE( di )     (opposite[di])
#else
// Add 180 degrees, same as  (di+4) & 0x0F
#define  DI_OPPOSITE( di )     ((di < DI_NODIR)? ((di) ^ 4) : DI_NODIR)
#endif

static dirtype_t diags[] =
{
    DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};




typedef struct {
    mobjtype_t mon_type;
    byte       speed[2];  // normal, fast
} monster_missile_info_t;

static const monster_missile_info_t  MonsterMissileInfo[] =
{
    // doom
    { MT_BRUISERSHOT, {15, 20}},
    { MT_HEADSHOT,    {10, 20}},
    { MT_TROOPSHOT,   {10, 20}},
    
    // heretic
    { MT_IMPBALL,     {10, 20}},
    { MT_MUMMYFX1,    { 9, 18}},
    { MT_KNIGHTAXE,   { 9, 18}},
    { MT_REDAXE,      { 9, 18}},
    { MT_BEASTBALL,   {12, 20}},
    { MT_WIZFX1,      {18, 24}},
    { MT_SNAKEPRO_A,  {14, 20}},
    { MT_SNAKEPRO_B,  {14, 20}},
    { MT_HEADFX1,     {13, 20}},
    { MT_HEADFX3,     {10, 18}},
    { MT_MNTRFX1,     {20, 26}},
    { MT_MNTRFX2,     {14, 20}},
    { MT_SRCRFX1,     {20, 28}},
    { MT_SOR2FX1,     {20, 28}},
};
byte  num_mon_missile = sizeof(MonsterMissileInfo) / sizeof(monster_missile_info_t);


static
void FastMonster_OnChange(void)
{
    static byte fast_active = 0;

    // cv_fastmonsters.EV selects fast, and is set for sk_nightmare.
    // if( gameskill == sk_nightmare )
    //  set_fast = 1;
    byte set_fast = cv_fastmonsters.EV;

    int i;
    for(i = 0; i<num_mon_missile; i++)
    {
        mobjinfo[ MonsterMissileInfo[i].mon_type ].speed
            = ((uint32_t)MonsterMissileInfo[i].speed[set_fast]) << FRACBITS;
    }

#ifdef MBF21
    // [WDJ] MBF21, to implement MBF21, from examination of DSDA-Doom.
    // This requires FRF_SKILL5_FAST preset for S_SARG_RUN1 to S_SARG_PAIN2.

    // Change info tables when fast_state is changing.
    // [WDJ] This uses a monster alt info structure.
    if( set_fast != fast_active )
    {
        // Apply deh_alt to mobjinfo.
        apply_alt_info_speed( set_fast );

        // Demo compatibility ??
//	if( ! (EN_boom || EN_mbf) )
//	    set_fast_state = 0;

        for( i = 0; i < NUMSTATES_EXT; i++ )
        {
            // [WDJ] The DSDA-Doom implementation does not restore tics correctly.
            // This implementation restores tics exactly.
            state_t * st = &states[i];
            uint16_t frf = P_get_frame_flags( st );
            if( frf & FRF_SKILL5_FAST )
            {
                if( set_fast && !(frf & FRF_SKILL5_MOD_APPLIED) )
                {
                    // [WDJ] DSDA-Doom demo compatibility bypassed the >=2 safety check,
                    // but all vanilla SARGS states had even tics which were >= 2.
                    // As this is conditional, it must be recorded if it got done or not,
                    // or else will not restore original tics value.
                    // As cannot rely that tics will be even, must record tics LSB bit, so can restore.
                    if( st->tics >= 2 )  // DSDA-Doom implementation, not explicitly in spec
                    {
                        uint16_t tics_lsb = st->tics & FRF_SKILL5_TICS_LSB;  // do not lose the LSB
                        st->tics >>= 1;
                        P_set_frame_flags( st, FRF_SKILL5_TICS_LSB, (tics_lsb | FRF_SKILL5_MOD_APPLIED) ); // set
                    }
                }
                else if( fast_active & (frf & FRF_SKILL5_MOD_APPLIED) )  // if restore old tics
                {
                    // Restore the tics field.
                    st->tics <<= 1;
                    st->tics |= frf & FRF_SKILL5_TICS_LSB;
                    P_set_frame_flags( st, (FRF_SKILL5_MOD_APPLIED | FRF_SKILL5_TICS_LSB), 0 );  // clear
                }
            }
        }

        fast_active = set_fast;
    }
#else

    // SARG_RUN1 to SARG_PAIN2 were all even tics.
    if( cv_fastmonsters.EV && !fast_active )
    {
        for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
            states[i].tics >>= 1;
        fast_active=true;
    }
    else
    if( !cv_fastmonsters.EV && fast_active )
    {
        for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
            states[i].tics <<= 1;
        fast_active=false;
    }
#endif
}


// MBF controls
static void mbf_OnChange( void );


consvar_t cv_monster_remember =
  {"mon_remember","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // Boom monsters_remember

consvar_t cv_mbf_dropoff = {"dropoff","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // !comp[comp_dropoff]
consvar_t cv_mbf_falloff = {"falloff","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // !comp[comp_falloff]

consvar_t cv_mbf_monster_avoid_hazard =
  {"mon_avoidhazard","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // MBF monster_avoid_hazards
consvar_t cv_mbf_monster_backing =
  {"mon_backing","0", CV_NETVAR | CV_SAVE, CV_OnOff};
  // MBF monster_backing
consvar_t cv_mbf_pursuit = {"pursuit","0", CV_NETVAR | CV_SAVE, CV_OnOff};
  // !comp[comp_pursuit]
consvar_t cv_mbf_distfriend =
  {"distfriend","128", CV_NETVAR | CV_VALUE | CV_SAVE | CV_CALL, CV_uint16, mbf_OnChange};
  // MBF distfriend (0..999)
consvar_t cv_mbf_staylift = {"staylift","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // !comp[comp_staylift]
consvar_t cv_mbf_help_friend = {"helpfriend","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // MBF help_friends
consvar_t cv_mbf_monkeys = {"monkeys","0", CV_NETVAR | CV_SAVE, CV_OnOff};
  // MBF monkeys (climb steep stairs)
#ifdef DOGS   
CV_PossibleValue_t dogs_cons_t[] = {{0,"MIN"}, {9,"MAX"}, {0,NULL} };
consvar_t cv_mbf_dogs = {"dogs_cnt","0", CV_NETVAR | CV_SAVE, dogs_cons_t};
  // MBF single player (and coop) dogs (0..9)
consvar_t cv_mbf_dog_jumping = {"dogjump","1", CV_NETVAR | CV_SAVE, CV_OnOff};
  // MBF dog_jumping
#endif

static void mbf_OnChange( void )
{
    // Demo sets EV_mbf_distfriend.
    EV_mbf_distfriend = cv_mbf_distfriend.value << FRACBITS;  // to fixed_t
}



// Infight settings translated to INFT_ values.  MBF demo sets infight.
byte  monster_infight; //DarkWolf95:November 21, 2003: Monsters Infight!
byte  monster_infight_deh; // DEH input.

       byte  EN_monster_friction;
static byte  EN_mbf_enemyfactor;
static byte  EN_monster_momentum;
       byte  EN_skull_limit;  // comp[comp_pain], enable pain skull limits
       byte  EN_old_pain_spawn;  // comp[comp_skull], can spit skull through walls
static byte  EN_doorstuck;  // !comp[comp_doorstuck], Boom doorstuck or MBF doorstuck
static byte  EN_mbf_doorstuck;  // MBF doorstuck
       byte  EN_mbf_speed;  // p_spec
static byte  EN_melee_radius_adj;  // ! doom_12_compatibility


// [WDJ] Friction got too complicated, use lookup table.
typedef enum {
  FRE_monster_friction = 0x01,
  FRE_monster_momentum = 0x02,
  FRE_mbf_enemyfactor = 0x04,
} friction_en_e;

static const byte friction_table[] =
{
  0,   // None
  FRE_monster_friction | FRE_mbf_enemyfactor,  // MBF
  FRE_monster_friction | FRE_monster_momentum,  // Momentum
  // Heretic demos have FR_orig friction, with special ice sector handling
  // in P_Thrust (so monsters slip only on ice sector)
  0,   // Heretic
};


// [WDJ] Monster friction, doorstuck, infight
void CV_monster_OnChange(void)
{
    // Set monster friction for Boom, MBF, prboom demo, by cv_monsterfriction.EV = 1.
    if( (demoplayback && (friction_model != FR_legacy))
        || ( cv_monsterfriction.EV > 3 ) )
    {
        EN_mbf_enemyfactor = (friction_model >= FR_mbf) && (friction_model <= FR_prboom);
        // Where monster friction is determined by friction model,
        // demo settings.
        // If cv_monsterfriction == 0x80, then EN_monster_friction has
        // already been set from the Boom demo compatiblity flag.
        if( cv_monsterfriction.EV < 0x80 )
            EN_monster_friction = EN_mbf_enemyfactor;  // MBF, PrBoom default
        EN_monster_momentum = 0;  // 2=momentum
    }
    else
    {
        // Legacy Demo, and User settings.
        // Legacy Demo sets through cv_monsterfriction.
        // default: 2= momentum
        register byte ft = friction_table[cv_monsterfriction.EV];
        EN_monster_friction = (ft & FRE_monster_friction);
        EN_mbf_enemyfactor = (ft & FRE_mbf_enemyfactor);
        EN_monster_momentum = (ft & FRE_monster_momentum);
    }
#ifdef FRICTIONTHINKER
    EN_boom_friction_thinker = (friction_model == FR_boom)
           && EN_variable_friction;
#endif

    // Demo sets through cv_doorstuck.
    EN_mbf_doorstuck = (cv_doorstuck.EV == 1);  // 1=MBF
    EN_doorstuck = (cv_doorstuck.EV > 0 );  // 0=none, 2=Boom

    // Demo sets infight through cv_monbehavior.
    // Monster Infight enables, can be changed during game.
    byte mbif = monbehav_to_infight[ cv_monbehavior.EV ];
    monster_infight = ((mbif & INFT_force) || (monster_infight_deh == INFT_none))?
        mbif // cv_monbehavior has precedence
      : monster_infight_deh;  // from DEH
    monster_infight &= ~INFT_force;  // clean up transient bit
}


// local version control
void DemoAdapt_p_enemy( void )
{
    // Demo sets cv_monsterfriction.EV, and other cv_xxx.EV.
    CV_monster_OnChange();

    if( demoversion < 200 )
    {
        EN_skull_limit = ( demoversion <= 132 );  // doom demos
        EN_old_pain_spawn = ( demoversion < 143 );
    }

    EN_mbf_speed = EN_mbf || (EV_legacy >= 145);  // Legacy 1.45, 1.46
    
#ifdef MBF21
    EN_melee_radius_adj = (demoversion >= 100);
#endif

#if 1
    if( demoplayback && verbose > 1 )
    { 
        GenPrintf(EMSG_ver, "friction_model=%i, EN_monster_friction=%i\n",
                friction_model, EN_monster_friction );
        GenPrintf(EMSG_ver, "EN_mbf_enemyfactor=%i, EN_monster_momentum=%i\n",
                EN_mbf_enemyfactor,  EN_monster_momentum );
        GenPrintf(EMSG_ver, "EN_skull_limit=%i, EN_old_pain_spawn=%i, EN_doorstuck=%i, EN_mbf_doorstuck=%i\n",
                EN_skull_limit, EN_old_pain_spawn, EN_doorstuck, EN_mbf_doorstuck );
    }
#endif
}


//
// ENEMY THINKING
// Enemies are always spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

static mobj_t*         soundtarget;

//  soundblocks : 0..1 number of sound blocking linedefs
static void P_RecursiveSound ( sector_t*  sec, byte soundblocks )
{
    int         i;
    line_t*     check;
    sector_t*   other;

    // wake up all monsters in this sector
    if (sec->validcount == validcount
        && sec->soundtraversed <= soundblocks+1)
    {
        return;         // already flooded
    }

    sec->validcount = validcount;
    sec->soundtraversed = soundblocks+1;
    SET_TARGET_REF( sec->soundtarget, soundtarget );

    for (i=0 ; i<sec->linecount ; i++)
    {
        // for each line of the sector linelist
        check = sec->linelist[i];
        if (! (check->flags & ML_TWOSIDED) )
            continue;  // nothing on other side

        P_LineOpening (check);

        if (openrange <= 0)
            continue;   // closed door

        if ( sides[ check->sidenum[0] ].sector == sec)
            other = sides[ check->sidenum[1] ].sector;
        else
            other = sides[ check->sidenum[0] ].sector;

        if (check->flags & ML_SOUNDBLOCK)
        {
            // Sound blocking linedef
            // If 0, then continue recursion with 1 soundblock.
            // If 1, then this is second, which blocks the sound.
            if (soundblocks == 0)
                P_RecursiveSound (other, 1);
        }
        else
            P_RecursiveSound (other, soundblocks);
    }
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void P_NoiseAlert ( mobj_t* target, mobj_t* emmiter )
{
    soundtarget = target;
    validcount++;
    P_RecursiveSound (emmiter->subsector->sector, 0);
}



//
// P_CheckMeleeRange
//
#ifdef MBF21
// MBF21: P_CheckRange, Added range parameter.
//   req_range: the range that allows a melee attack
// Return true if in melee range.
static
boolean P_CheckMeleeRange_orig( mobj_t* actor, fixed_t req_range )
#else
static boolean P_CheckMeleeRange (mobj_t* actor)
#endif
{
    mobj_t *    pl = actor->target;
    fixed_t     dist;

    if(!pl)
        goto ret_false;

    // [WDJ] prboom, killough, friend monsters do not attack other friend
    if( BOTH_FRIEND(actor, pl) )
        goto ret_false;

    dist = P_AproxDistance (pl->x-actor->x, pl->y-actor->y);

    // [WDJ] FIXME pl->info may be NULL (seen in phobiata.wad)
    if (pl->info == NULL )
        goto ret_false;
#ifdef MBF21
    if ( dist >= req_range )
        goto ret_false;
#else
    if (dist >= (MELEERANGE - FIXINT(20) + pl->info->radius) )
        goto ret_false;
#endif

    //added:19-03-98: check height now, so that damn imps cant attack
    //                you if you stand on a higher ledge.
    // DoomLegacy adopted change.  This is same as in Heretic, Hexen.
    if( (EV_legacy > 111)
         && ((pl->z > actor->z + actor->height)
             || (actor->z > pl->z + pl->height) )
      )
        goto ret_false;

    if (! P_CheckSight (actor, actor->target) )
        goto ret_false;

    return true;

ret_false:
    return false;
}


#ifdef MBF21
//
// P_CheckMeleeRange
//
// MBF21 replacement for P_CheckMeleeRange.
// In one place they could not call this, but have to call the orig.
static boolean P_CheckMeleeRange( mobj_t* actor )
{
    // MBF21 modifications
#ifdef UDMF    
    if( actor->subsector->sector->flags & SECF_NO_ATTACK )
      return false;
#endif

    // Original (MELEERANGE - 20*FRACUNIT + pl->info->radius)
    int range = actor->info->melee_range;  // replaces fixed MELEERANGE

    if( EN_melee_radius_adj )  // not Doom 1.2 compatibility
      range += actor->target->info->radius - FIXINT(20);
    
    return P_CheckMeleeRange_orig( actor, range );  // orig, with range param
}
#endif

// [WDJ] MBF, from MBF, PrBoom, EternityEngine
//
// P_HitFriend()
//
// killough 12/98
// This function tries to prevent shooting at friends.
// Known that (actor->flags & MF_FRIEND), checked by caller.
// Return true if aiming would hit a friend.
static boolean P_HitFriend(mobj_t* actor)
{
    mobj_t * target = actor->target;

    if( target )
    {
        P_AimLineAttack(actor,
           R_PointToAngle2(actor->x, actor->y, target->x, target->y),
           P_AproxDistance(actor->x - target->x, actor->y - target->y),
           0 );
        // Because actor is already known to have MF_FRIEND (tested by caller),
        // replace SAME_FRIEND with simpler test of MF_FRIEND.
        if( lar_linetarget
            && lar_linetarget != target
            && lar_linetarget->flags & MF_FRIEND
// MBF:	    && SAME_FRIEND(lar_linetarget, actor)
          )
            return true;
    }
    return false;   
}


//
// P_CheckMissileRange
//
// Return true if good to fire missile.
static boolean P_CheckMissileRange (mobj_t* actor)
{
    mobj_t * target = actor->target;
    fixed_t  dist;

    if (! P_CheckSight (actor, actor->target) )
        goto ret_false;

    if ( actor->flags & MF_JUSTHIT )
    {
        // the target just hit the enemy,
        // so fight back!
        actor->flags &= ~MF_JUSTHIT;

        if( EN_heretic || (demoversion < 147) )
            goto ret_true;  // Old Legacy, Old Doom

        // [WDJ] MBF, from MBF, PrBoom
        // Boom has two calls of P_Random, which affect demos
        // killough 7/18/98: no friendly fire at corpses
        // killough 11/98: prevent too much infighting among friends
        return
         !(actor->flags & MF_FRIEND)
         ||( (target->health > 0)
             &&( !(target->flags & MF_FRIEND)
                 || (target->player ?
                     // PrBoom, MBF, EternityEngine use P_Random(pr_defect)
                     (MONSTER_INFIGHTING || (PP_Random(pr_defect)>128))
                     : !(target->flags & MF_JUSTHIT) && PP_Random(pr_defect)>128  )
                )
           );
    }

    // [WDJ] MBF, from MBF, PrBoom
    // killough 7/18/98: friendly monsters don't attack other friendly
    // monsters or players (except when attacked, and then only once)
    if( BOTH_FRIEND(actor, target) )
        goto ret_false;

    if (actor->reactiontime)
        goto ret_false;  // do not attack yet

    // OPTIMIZE: get this from a global checksight
    dist = P_AproxDistance ( actor->x - actor->target->x,
                             actor->y - actor->target->y) - FIXINT(64);

    if (!actor->info->meleestate)
        dist -= FIXINT(128);   // no melee attack, so fire more

    dist >>= FRACBITS;

#ifdef MBF21
    // [WDJ] MBF21 as in DSDA-Doom.
    // MT_VILE has MF3_SHORTMRANGE flag set.
    if( actor->flags3 & MF3_SHORTMRANGE )
#else
    if (actor->type == MT_VILE)
#endif
    {
        if (dist > 14*64)
            goto ret_false;  // too far away
    }

#ifdef MBF21
    // [WDJ] MBF21 as in DSDA-Doom.
    // MT_UNDEAD has MF3_LONGMELEE flag set.
    if( actor->flags3 & MF3_LONGMELEE )
    {
        if (dist < 196)
            goto ret_false;  // close for fist attack
    }
    // MT_UNDEAD, MT_CYBORG, MT_SPIDER, MT_SKULL has MF3_RANGEHALF flag set.
    // Heretic MT_IMP has MF3_RANGEHALF flag set.
    if( actor->flags3 & MF3_RANGEHALF )
    {
        dist >>= 1;
    }
#else
    if (actor->type == MT_UNDEAD)
    {
        if (dist < 196)
            goto ret_false;  // close for fist attack
        dist >>= 1;
    }

    if (actor->type == MT_CYBORG
        || actor->type == MT_SPIDER
        || actor->type == MT_SKULL
        || actor->type == MT_IMP) // heretic monster
    {
        dist >>= 1;
    }
#endif

    if (dist > 200)
        dist = 200;

#ifdef MBF21
    // [WDJ] MBF21 as in DSDA-Doom.
    // MT_CYBORG has MF3_HIGHERMPROB flag set.
    if( actor->flags3 & MF3_HIGHERMPROB )
#else
    if( actor->type == MT_CYBORG )
#endif
        if( dist > 160 )
            dist = 160;

    // Same as PrBoom.
    if (PP_Random(pr_missrange) < dist)
        goto ret_false;

    if( actor->flags & MF_FRIEND )  // call is only useful if MBF FRIEND
    {
        if( P_HitFriend(actor) )
            goto ret_false;
    }

ret_true:
    return true;

ret_false:
    return false;
}


// [WDJ] MBF, from MBF, PrBoom, EternityEngine
// P_IsOnLift
//
// killough 9/9/98:
//
// Returns true if the object is on a lift. Used for AI, since it may indicate
// the need for crowded conditions, or that a monster should stay on the lift
// for a while while it goes up or down.

boolean P_IsOnLift( const mobj_t* actor )
{
    const sector_t *sec = actor->subsector->sector;
    line_t line;
    int l;

    // Short-circuit: it's on a lift which is active.
    if( sec->floordata
        && ((thinker_t *) sec->floordata)->function == TFI_PlatRaise )
        return true;

    // Check to see if it is in a sector which can be activated as a lift.
    line.tag = sec->tag;
    if( line.tag == 0 )
        return false;
   
    for( l = -1; (l = P_FindLineFromLineTag(&line, l)) >= 0; )
    {
// [WDJ] This is only MBF. Does not include Heretic or expansions.       
        switch( lines[l].special )
        {
            case  10: case  14: case  15: case  20: case  21: case  22:
            case  47: case  53: case  62: case  66: case  67: case  68:
            case  87: case  88: case  95: case 120: case 121: case 122:
            case 123: case 143: case 162: case 163: case 181: case 182:
            case 144: case 148: case 149: case 211: case 227: case 228:
            case 231: case 232: case 235: case 236:
            return true;
        }
    }
    return false;
}


// [WDJ] MBF, from MBF, PrBoom, EternityEngine
// P_IsUnderDamage
// killough 9/9/98:
// Used for AI.
// Returns nonzero if the object is under damage based on their current position.
// Returns 1 if the damage is moderate (ceiling crusher up),
//        -1 if it is serious (ceiling crusher down).
int P_IsUnderDamage(mobj_t* actor)
{
    const msecnode_t * ts;  // touching sector
    const ceiling_t * cl;   // Crushing ceiling
    int dir = 0;

    for( ts = actor->touching_sectorlist; ts; ts = ts->m_tnext )
    {
        cl = ts->m_sector->ceilingdata;
        if( cl && (cl->thinker.function == TFI_MoveCeiling) )
            dir |= cl->direction;
    }
    return dir;
}


//
// P_MoveActor
// Move in the current direction,
// returns false if the move is blocked.
//
static const fixed_t xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
static const fixed_t yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};

// Called multiple times in one step, by P_TryWalk, while trying to find
// valid path for an actor.
// Called by P_SmartMove, indirectly P_TryWalk, A_Chase.
// Only called for actor things, not players, nor missiles.
//  dropoff : 0, 1, 2 dog jump
// Formerly P_Move.
// Return false when move is blocked.
static boolean P_MoveActor (mobj_t* actor, byte dropoff)
{
    fixed_t  tryx, tryy;
    fixed_t  old_momx, old_momy;
    line_t*  ld;
    boolean  good;
    int      hit_block = 0;
#ifdef MONSTER_VARY
    int      speed = actor->speed;
#else
    int      speed = actor->info->speed;
#endif

#ifdef HEXEN
    // [WDJ] Hexen, adapted from DSDA-Doom, FIXME
    // ?? Description does not match effect.
    if( actor->flags3 & MF3_BLASTED )  // got BLASTED
        return true;
#endif
    
    if (actor->movedir == DI_NODIR)
        return false;

#ifdef PARANOIA
    if ((unsigned)actor->movedir >= 8)
        I_Error ("Weird actor->movedir!");
#endif

    old_momx = actor->momx;
    old_momy = actor->momy;

    // [WDJ] Monsters too, killough 10/98
    // Otherwise they move too easily on mud and ice
    if(EN_monster_friction
       && (actor->z <= actor->floorz)  // must be on floor
       && !(actor->flags2&MF2_FLY)  // not heretic flying monsters
       )
    {
        // MBF, prboom: all into speed (except ice)
        // DoomLegacy: fixed proportion of momentum and speed
        fixed_t  dx = speed * xspeed[actor->movedir];
        fixed_t  dy = speed * yspeed[actor->movedir];   
        // [WDJ] Math: Part of speed goes into momentum, R=0.5.
        // fr = FRICTION_NORM/FRACUNIT = 0.90625
        // movement due to momentum (for i=0..)
        //         = Sum( momf * speed * (fr**i)) = momf * speed / (1-fr)
        // Thus: momf = (1- (FRICTION_NORM/FRACUNIT)) * R
        // For R=0.5, momf = 0.046875 = 3/64
        float  momf = ( EN_mbf_enemyfactor )? 0.0 : 0.046875f;  // default
        float  mdiffm = 1.0f;  // movefactor diff mult (to reduce effect)
       
        P_GetMoveFactor(actor);  // sets got_movefactor, got_friction
        if( got_friction < FRICTION_NORM )
        {   // mud
            if( EN_mbf_enemyfactor )
            {   // MBF, prboom: modify speed
                mdiffm = 0.5f;  // 1/2 by MBF
            }
            else
            {   // DoomLegacy:
                // movefactor is for cmd=25, but speed=8 for player size actor
                mdiffm = 0.64f;  // by experiment, larger is slower
            }
        }
        else if (got_friction > FRICTION_NORM )
        {   // ice
            // Avoid adding momentum into walls
            tryx = actor->x + dx;
            tryy = actor->y + dy;
            // [WDJ] Do not use TryMove to check position, as it has
            // too many side effects and is not reversible.
            // Cannot just reset actor x,y.
            if (P_CheckPosition (actor, tryx, tryy))
            { // success, give it momentum too
                if( EN_mbf_enemyfactor )
                {   // MBF, prboom:
                    speed = 0;  // put it all into momf
                    momf = 0.25f;  // becomes MBF equation
                }
                else
                {   // DoomLegacy:
                    // monsters a little better in slime than humans
                    mdiffm = 0.62f; // by experiment, larger is slower
#if 0
                    // [WDJ] proportional as movefactor decreases
                    // This worked better than MBF, but still showed friction transistion accel and decel.
                    fixed_t pro = INT_TO_FIXED(ORIG_FRICTION_FACTOR - got_movefactor) / (ORIG_FRICTION_FACTOR*69/100);
                    if( pro > FIXINT(1) )  pro = FIXINT(1); // limit to 1
                    fixed_t anti_pro = (FIXINT(1) - pro);
                    momf = FixedMul( momf, pro ); // pro to momentum
                    anti_pro = FixedMul(anti_pro, anti_pro); // (1-pro)**2 to speed
                    got_movefactor = FixedMul( got_movefactor, anti_pro);
#endif
                }
            }
            else
            {
                momf = 0.0f;  // otherwise they get stuck at walls
            }
        }
        // [WDJ] Apply mdiffm to difference between movefactor and normal.
        // movefactor has ORIG_FRICTION_FACTOR
        // movediff = (got_movefactor - ORIG_FRICTION_FACTOR) * mdiffm
        // ratio = (ORIG_FRICTION_FACTOR + movediff) / ORIG_FRICTION_FACTOR
        float mf_ratio = (
          (((float)(got_movefactor - ORIG_FRICTION_FACTOR)) * mdiffm)
            + ((float)ORIG_FRICTION_FACTOR)
          ) / ((float)ORIG_FRICTION_FACTOR);

        // modify speed by movefactor
        speed = (int) ( speed * mf_ratio ); // MBF, prboom: no momentum

        // [WDJ] Trying to use momentum for some sectors and not for others
        // results in stall when entering an icy momentum sector,
        // and speed surge when leaving an icy momentum sector.
        // Use it for all sectors to the same degree (except in demo compatibility).
        if( momf > 0.0 )  // not disabled
        {
            // It is necessary that there be full speed at dead stop to get
            // monsters unstuck from walls and each other.
            // Some wads like TNT have overlapping monster starting positions.
            if( actor->momx || actor->momy )  // if not at standstill
                speed /= 2;  // half of speed goes to momentum
            // apply momentum
            momf *= mf_ratio;
            actor->momx += (int) ( dx * momf );
            actor->momy += (int) ( dy * momf );
        }
        else
        {
            // if momf disabled, then minimum speed
            // Minimum speed with momf causes them to walk off ledges.
            if( speed == 0 )  speed = 1;
        }
    }

    tryx = actor->x + speed * xspeed[actor->movedir];
    tryy = actor->y + speed * yspeed[actor->movedir];

    if( !P_TryMove (actor, tryx, tryy, dropoff) )  // caller dropoff
    {
        // blocked move
        // Monsters will be here multiple times in each step while
        // trying to find sucessful path.
        actor->momx = old_momx;  // cancel any momentum changes for next try
        actor->momy = old_momy;

        // open any specials
        // tmr_floatok, tmr_floorz returned by P_TryMove
        if( (actor->flags & MF_FLOAT) && tmr_floatok )
        {
            // must adjust height
            if (actor->z < tmr_floorz)
                actor->z += FLOATSPEED;
            else
                actor->z -= FLOATSPEED;

            actor->flags |= MF_INFLOAT;
            return true;
        }

        if (!numspechit)
            return false;

        actor->movedir = DI_NODIR;
        good = false;
        while (numspechit--)
        {
            ld = &lines[ spechit[numspechit] ];
            // [WDJ] FIXME: Monsters get stuck in the door track when
            // they see the door activation and nothing else.
            // If the special is not a door that can be opened,
            // return false
            if( P_UseSpecialLine (actor, ld, 0) )
            {
                if( EN_mbf_doorstuck && (ld == tmr_blockingline))
                   hit_block = 1;
                good = true;
            }
        }

        if( good && EN_doorstuck )
        {
            // [WDJ] Calls of P_Random here in Boom, affects Demo sync.
            // A line blocking the monster got activated, a little randomness
            // to get unstuck from door frame.
            if (EN_mbf_doorstuck)
            {
                good = (PP_Random(pr_opendoor) >= 230) ^ (hit_block);  // MBF
            }
            else
            {
                good = PP_Random(pr_trywalk) & 3;  // Boom jff, 25% fail
                // Causes monsters to back out when they should not,
                // and causes secondary stickiness.
            }
        }
        return good;
    }
    else  // TryMove
    {
        // successful move

#if 0
        // Not needed, see DoomLegacy Mud and Ice above.
        // Boom ice, from PrBoom
        if( got_friction > ORIG_FRICTION )
        {
            P_UnsetThingPosition(actor);   // In MBF, but not PrBoom (looks necessary)
            // Let normal momentum carry them across ice.
            actor->x = origx;
            actor->y = origy;
            movefactor *= FIXINT(1) / ORIG_FRICTION_FACTOR / 4;
            actor->momx += FixedMul( dx, movefactor );
            actor->momy += FixedMul( dy, movefactor );
            P_SetThingPosition(actor);   // in MBF, but not PrBoom (look necessary)
        }
#endif

        if( EN_monster_momentum && tmr_dropoffline )
        {
            // [WDJ] last move sensed dropoff
            // Reduce momentum near dropoffs, friction 0x4000 to 0xE000
#define  FRICTION_DROPOFF   0x6000
            actor->momx = FixedMul(actor->momx, FRICTION_DROPOFF);
            actor->momy = FixedMul(actor->momx, FRICTION_DROPOFF);
        }
        actor->flags &= ~MF_INFLOAT;
    }


    if(! (actor->flags & MF_FLOAT) )
    {
        // Monster gravity, or MBF felldown, blocks this vanilla instant fall.
        if( ! ( (cv_monstergravity.EV == 2) 
               || ((cv_monstergravity.EV == 1) && tmr_felldown) ) )
        {
            if(actor->z > actor->floorz)
               P_HitFloor(actor);
            actor->z = actor->floorz;
        }
    }
    return true;
}


// [WDJ] From MBF, PrBoom
// P_SmartMove
// killough 9/12/98: Same as P_Move, except smarter
static boolean P_SmartMove(mobj_t* actor)
{
    mobj_t * target = actor->target;
    byte  on_lift;
    byte  dropoff = 0;  // 0,1,2
    int   under_damage = 0;  // -1,0,1

    // killough 9/12/98: Stay on a lift if target is on one
    on_lift = cv_mbf_staylift.EV
      && target
      && target->health > 0
      && target->subsector->sector->tag == actor->subsector->sector->tag
      && P_IsOnLift(actor);

    if( cv_mbf_monster_avoid_hazard.EV )
        under_damage = P_IsUnderDamage(actor);  // 1, 0, -1

#ifdef DOGS
    // killough 10/98: allow dogs to drop off of taller ledges sometimes.
    // dropoff==1 means always allow it,
    // dropoff==2 means only up to 128 high,
    // and only if the target is immediately on the other side of the line.

    // haleyjd: allow all friends of Helper_MT Type to also jump down

    if( (actor->type == MT_DOGS
            || ((actor->type == helper_MT) && (actor->flags & MF_FRIEND)) )
        && cv_mbf_dog_jumping.EV
        && target
        && SAME_FRIEND(target, actor)
        && P_AproxDistance(actor->x - target->x, actor->y - target->y)
             < FIXINT(144)
        && (PP_Random(pr_dropoff) < 235)  // usage enabled by dog_jumping
      )
        dropoff = 2;
#endif

    if( !P_MoveActor(actor, dropoff) )
        return false;

    // killough 9/9/98: avoid crushing ceilings or other damaging areas
    // P_Random usage enabled by mbf_staylift => on_lift.
    if( on_lift
        && (PP_Random(pr_stayonlift) < 230) // Stay on lift
        && !P_IsOnLift(actor)
      )
        goto avoid_damage;
    
    if( cv_mbf_monster_avoid_hazard.EV
        && (under_damage == 0) )
    {
        // P_Random usage enabled by mbf_monster_avoid_hazard.
        // Get away from damage
        under_damage = P_IsUnderDamage(actor);
        if( under_damage
            && ((under_damage < 0) || (PP_Random(pr_avoidcrush) < 200)) )
            goto avoid_damage;
    }

    return true;

avoid_damage:
    actor->movedir = DI_NODIR;  // avoid the area (most of the time anyway)
    return true;
}


line_t * trywalk_dropoffline;

//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
// Called multiple times in one step, while trying to find valid path.
//
static boolean P_TryWalk (mobj_t* actor)
{
    if( !P_SmartMove(actor) )  // P_MoveActor()
    {
        // record if failing due to dropoff
        if( tmr_dropoffline )
            trywalk_dropoffline = tmr_dropoffline;
        return false;
    }
    actor->movecount = PP_Random(pr_trywalk) & 0x0F;
    return true;
}



// [WDJ] MBF, From MBF, PrBoom, EternityEngine
// killough 11/98:
//
// Monsters try to move away from tall dropoffs.
//
// In Doom, they were never allowed to hang over dropoffs,
// and would remain stuck if involuntarily forced over one.
// This logic, combined with p_map.c (P_TryMove), allows
// monsters to free themselves without making them tend to
// hang over dropoffs.

static fixed_t dropoff_r_deltax, dropoff_r_deltay, dropoff_floorz;

// Always returns true, searches all lines.
static boolean PIT_AvoidDropoff(line_t *line)
{
    if( line->backsector // Ignore one-sided linedefs
        && tm_bbox[BOXRIGHT]  > line->bbox[BOXLEFT]
        && tm_bbox[BOXLEFT]   < line->bbox[BOXRIGHT]
        && tm_bbox[BOXTOP]    > line->bbox[BOXBOTTOM] // Linedef must be contacted
        && tm_bbox[BOXBOTTOM] < line->bbox[BOXTOP]
        && P_BoxOnLineSide(tm_bbox, line) == -1
      )
    {
        // Touching the line.
        fixed_t front = line->frontsector->floorheight;
        fixed_t back  = line->backsector->floorheight;
        angle_t angle;

        // The monster must contact one of the two floors,
        // and the other must be a tall dropoff (more than 24).
        if( back == dropoff_floorz
            && front < dropoff_floorz - FIXINT(24) )
        {
            // front side dropoff
            angle = R_PointToAngle2(0,0,line->dx,line->dy);
        }
        else if( front == dropoff_floorz
             && back < dropoff_floorz - FIXINT(24) )
        {
            // back side dropoff       
            angle = R_PointToAngle2(line->dx,line->dy,0,0);
        }
        else
            return true;  // not a dropoff

        // Move away from dropoff at a standard speed.
        // Multiple contacted linedefs are cumulative
        // (e.g. hanging over corner)
        dropoff_r_deltax -= finesine[angle >> ANGLETOFINESHIFT]*32;
        dropoff_r_deltay += finecosine[angle >> ANGLETOFINESHIFT]*32;
    }
    return true;
}

//
// Driver for above
//
static fixed_t P_AvoidDropoff(mobj_t* actor)
{
    int yh, yl, xh, xl;
    int bx, by;

    tm_bbox[BOXTOP] = actor->y + actor->radius;
    tm_bbox[BOXBOTTOM] = actor->y - actor->radius;
    tm_bbox[BOXRIGHT] = actor->x + actor->radius;
    tm_bbox[BOXLEFT] = actor->x - actor->radius;
    yh = (tm_bbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;
    yl = (tm_bbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    xh = (tm_bbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    xl = (tm_bbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;

    // PIT_AvoidDropoff parameters
    dropoff_floorz = actor->z;  // remember floor height
    dropoff_r_deltax = dropoff_r_deltay = 0;  // outputs

    // check lines

    validcount++;
    for( bx=xl ; bx<=xh ; bx++ )
    {
        for( by=yl ; by<=yh ; by++ )
            P_BlockLinesIterator(bx, by, PIT_AvoidDropoff);  // all contacted lines
    }

    // Non-zero if movement prescribed, caller uses dropoff vars.
    return dropoff_r_deltax | dropoff_r_deltay;
}


static
void P_NewChaseDir_P2 (mobj_t*  actor, fixed_t deltax, fixed_t deltay )
{
    int8_t      xdir, ydir, tdir;  // dirtype_t  0..9
    dirtype_t   olddir = actor->movedir;
    dirtype_t   turnaround = DI_OPPOSITE( olddir );

    trywalk_dropoffline = NULL;  // clear dropoff record

    // Clang wont accept FIXINT(-10), which is really strange.
    xdir = ( deltax > FIXINT(10) )?   DI_EAST
         : ( deltax < -FIXINT(10) )?  DI_WEST
         : DI_NODIR ;

    ydir = ( deltay < -FIXINT(10) )? DI_SOUTH
         : ( deltay > FIXINT(10) )?  DI_NORTH
         : DI_NODIR ;

    // try direct route
    if (   xdir != DI_NODIR
        && ydir != DI_NODIR)
    {
        actor->movedir = diags[((deltay<0)<<1)+(deltax>0)];
        if( actor->movedir != turnaround
            && P_TryWalk(actor) )
            goto accept_move;
    }

    // try other directions
    if( PP_Random(pr_newchase) > 200
        ||  abs(deltay) > abs(deltax))
    {
        tdir=xdir;
        xdir=ydir;
        ydir=tdir;
    }

    if( xdir == turnaround)
        xdir = DI_NODIR;
    else if( xdir != DI_NODIR )
    {
        actor->movedir = xdir;
        if( P_TryWalk(actor) )
        {
            // either moved forward or attacked
            goto accept_move;
        }
    }

    if( ydir == turnaround)
        ydir = DI_NODIR;
    else if( ydir != DI_NODIR )
    {
        actor->movedir = ydir;
        if( P_TryWalk(actor) )
            goto accept_move;
    }

    // there is no direct path to the player, so pick another direction.
    if( olddir != DI_NODIR )
    {
        actor->movedir = olddir;
        if( P_TryWalk(actor) )
            goto accept_move;
    }

    // randomly determine direction of search
    if( PP_Random(pr_newchasedir) & 0x01 )
    {
        for( tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++ )
        {
            if( tdir != turnaround )
            {
                actor->movedir = tdir;
                if( P_TryWalk(actor) )
                    goto accept_move;
            }
        }
    }
    else
    {
        for( tdir = DI_SOUTHEAST; tdir >= DI_EAST; tdir-- )  // signed tdir
        {
            if( tdir != turnaround )
            {
                actor->movedir = tdir;
                if( P_TryWalk(actor) )
                    goto accept_move;
            }
        }
    }

#define PRBOOM2_TURNAROUND
#ifdef PRBOOM2_TURNAROUND
    // As in PrBoom
    // Rearranged to work with monster_momentum test below.
    actor->movedir = turnaround;
    if( turnaround != DI_NODIR )
    {
        if( P_TryWalk(actor) )
            goto accept_move;
            // accept move, movdir != DI_NODIR
       
        actor->movedir = DI_NODIR;
    }
    // assert actor->movedir == DI_NODIR
#else
    // [WDJ] This seems to be functionally the same as above
    // due to failure setting NODIR anyway.
    if( turnaround != DI_NODIR )
    {
        actor->movedir = turnaround;
        if( P_TryWalk(actor) )
            goto accept_move;
    }
#endif

    // [WDJ] to not glide off ledges, unless conveyor
    if( EN_monster_momentum && trywalk_dropoffline )
    {
        // [WDJ] Momentum got actor stuck on edge,
        // but just reversing momentum is too much for conveyor.
        // Move perpendicular to dropoff line to get unstuck.
        fixed_t  dax, day;
        if( actor->subsector->sector == trywalk_dropoffline->frontsector )
        {
            // vector to frontsector
            dax = trywalk_dropoffline->dy;
            day = -trywalk_dropoffline->dx;
        }
        else if( actor->subsector->sector == trywalk_dropoffline->backsector )
        {
            // vector to backsector
            dax = -trywalk_dropoffline->dy;
            day = trywalk_dropoffline->dx;
        }
        else
        {
            actor->momx = actor->momy = 0;
            goto no_move;  // don't move across the dropoff
        }
        // Observe monsters on ledge, how long they stay stuck,
        // and on conveyor, how long they take to fall off.
        // Tuned backpedal speed constant.
#ifdef MONSTER_VARY
        register int  backpedal = (actor->speed * 0x87BB);
#else
        register int  backpedal = (actor->info->speed * 0x87BB);
#endif
//        debug_Printf( "backpedal 0x%X ", backpedal );
        // Vector away from line
        fixed_t  dal = P_AproxDistance(dax,day);
        dal = FixedDiv( dal, backpedal );
        if( dal > 1 )
        {
            dax = FixedDiv( dax, dal );  // shorten vector
            day = FixedDiv( day, dal );
        }

//        debug_Printf( "stuck delta (0x%X,0x%X)\n", dax, day );
        if (P_TryMove (actor, actor->x + dax, actor->y + day, true))  // allow cross dropoff
            goto accept_move;

    no_move:
        // must fall off conveyor end, but not off high ledges
        actor->momx = FixedMul(actor->momx, 0xA000);
        actor->momy = FixedMul(actor->momx, 0xA000);
    }

    actor->movedir = DI_NODIR;  // can not move

accept_move:
    return;
}


// [WDJ] MBF, from MBF, PrBoom, EternityEngine
// Most of the original P_NewChaseDir is now in P_NewChaseDir_P2.
static void P_NewChaseDir(mobj_t* actor)
{
    mobj_t *  target = actor->target;
    fixed_t deltax, deltay;

    if( !target )
    {
        I_SoftError ("P_NewChaseDir: called with no target");
        return;
    }

    deltax = target->x - actor->x;
    deltay = target->y - actor->y;
   
    // killough 8/8/98: sometimes move away from target, keeping distance
    //
    // 1) Stay a certain distance away from a friend, to avoid being in their way
    // 2) Take advantage over an enemy without missiles, by keeping distance
    
    actor->strafecount = 0;  // MBF

    if( EN_mbf )
    {
        // CheckPosition has not been called, so tmr_dropoffz is not valid.
        // MBF has mobj keep dropoffz field.
        if( cv_mbf_dropoff.EV
            && actor->floorz - actor->dropoffz > FIXINT(24)
            && actor->z <= actor->floorz
            && !(actor->flags & (MF_DROPOFF|MF_FLOAT))
            && P_AvoidDropoff(actor) // Move away from dropoff
          )
        {
            P_NewChaseDir_P2(actor, dropoff_r_deltax, dropoff_r_deltay);

            // If moving away from dropoff, set movecount to 1 so that
            // small steps are taken to get monster away from dropoff.

            actor->movecount = 1;
            return;
        }
        else
        {
            fixed_t dist = P_AproxDistance(deltax, deltay);

            // Move away from friends when too close, except
            // in certain situations (e.g. a crowded lift)

            if( BOTH_FRIEND(actor, target)
                && dist < EV_mbf_distfriend
                && !P_IsOnLift(target)
                && !P_IsUnderDamage(actor)
              )
            {
                deltax = -deltax;
                deltay = -deltay;
            }
            else if( target->health > 0
                     && ! SAME_FRIEND(actor, target) )
            {   // Live enemy target
                if( cv_mbf_monster_backing.EV
                    && actor->info->missilestate
                    && actor->type != MT_SKULL
                    && ( (!target->info->missilestate
                          && dist < MELEERANGE*2 )
                        || (target->player
                            && (dist < MELEERANGE*3)
#ifdef MBF21
                    && ( (!target->info->missilestate
                          && dist < target->info->melee_range*2 )
                        || (target->player
                            && (dist < target->info->melee_range*3)
                            // Monster flees when our weapon is fist or chainsaw, (mod by DEH).
                            && ( target->player->weaponinfo[ target->player->readyweapon ].flags & WPF_FLEE_MELEE )
                           )
                       )
#else
                    && ( (!target->info->missilestate
                          && dist < MELEERANGE*2 )
                        || (target->player
                            && (dist < MELEERANGE*3)
                            && (target->player->readyweapon == wp_fist
                                || target->player->readyweapon == wp_chainsaw)
                           )
                       )
#endif
                           )
                       )
                  )
                {   // Back away from melee attacker
                    // P_Random usage enabled by mbf_monster_backing.		   
                    actor->strafecount = PP_Random(pr_enemystrafe) & 0x0F;
                    deltax = -deltax;
                    deltay = -deltay;
                }
            }
        }
    }

    P_NewChaseDir_P2(actor, deltax, deltay);

    // If strafing, set movecount to strafecount so that old Doom
    // logic still works the same, except in the strafing part

    if (actor->strafecount)
        actor->movecount = actor->strafecount;
}


// [WDJ] MBF, From MBF, PrBoom, EternityEngine
//
// P_IsVisible
//
// killough 9/9/98: whether a target is visible to a monster
//

static
boolean P_IsVisible(mobj_t* actor, mobj_t* mo, boolean allaround)
{
    if( !allaround )
    {
        // Not visible if not within 90 degree of facing, and outside MELEE range.
        angle_t an = R_PointToAngle2(actor->x, actor->y, mo->x, mo->y)
                     - actor->angle;

        if( ( an > ANG90 && an < ANG270 )
            && ( P_AproxDistance((mo->x - actor->x), (mo->y - actor->y))
                 > MELEERANGE )
          )
            return false;
        // possibly visible, check sight
    }
    return P_CheckSight(actor, mo);
}


// [WDJ] MBF, From MBF, PrBoom, EternityEngine
//
// PIT_FindTarget
//
// killough 9/5/98
//
// Finds monster targets for other monsters
//

static mobj_t * ft_current_actor;
static int ft_current_allaround;

static
boolean PIT_FindTarget(mobj_t* mo)
{
    mobj_t* actor = ft_current_actor;

    if( SAME_FRIEND(mo, actor)  // Invalid target
        || ! (mo->health > 0)
        || ! (mo->flags & MF_COUNTKILL || mo->type == MT_SKULL)
      )
        return true;  // reject, keep searching

    // If the monster is already engaged in a one-on-one attack
    // with a healthy friend, don't attack around 60% the time
    {
        const mobj_t * t2 = mo->target;  // target of this monster
        if( t2
            && t2->target == mo  // both targeting each other
            && PP_Random(pr_skiptarget) > 100  // stuck with test order
          )
        {
            // Already known that mo is not a friend.
            if( ! SAME_FRIEND(t2, mo)  // is attacking a friend
                && t2->health*2 >= t2->info->spawnhealth ) // friend health ok
                return true;  // find someone else
        }
    }

    if( !P_IsVisible(actor, mo, ft_current_allaround) )
        return true;

    SET_TARGET_REF( actor->lastenemy, actor->target );  // Remember previous target
    SET_TARGET_REF( actor->target, mo );                // Found target

    // Move the selected monster to the end of its class-list,
    // so that it gets searched last next time.
    P_MoveClassThink(&mo->thinker, 0);  // Last

    return false;  // Found target
}


//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Modifies actor.
// Returns true if a player is targeted.
//
static boolean P_LookForPlayers ( mobj_t*       actor,
                                  boolean       allaround )
{
    int  c;
    int  stop, stopc;
    int  lastlook = actor->lastlook;
    player_t*   player;

    if( EN_heretic
        && !multiplayer && players[0].health <= 0 )
    { // Single player game and player is dead, look for monsters
        return(PH_LookForMonsters(actor));  // Heretic version
    }

    // [WDJ] MBF, From MBF, PrBoom, EternityEngine.
    if( actor->flags & MF_FRIEND )
    {   // killough 9/9/98: friendly monsters go about players differently
        int anyone;

#if 0
        // If you want friendly monsters not to awaken unprovoked
        if( !allaround )
            return false;
#endif

        // Go back to a player, no matter whether it's visible or not
        // First time look for visible player, then look for anyone.
        for( anyone=0; anyone<=1; anyone++ )
        {
            for( c=0; c<MAXPLAYERS; c++ )
            {
                if( playeringame[c]
                    && players[c].playerstate == PST_LIVE
                    && (anyone
                        || P_IsVisible(actor, players[c].mo, allaround) )
                  )
                {
                    SET_TARGET_REF( actor->target, players[c].mo );

                    // killough 12/98:
                    // get out of refiring loop, to avoid hitting player accidentally
                    if( actor->info->missilestate )
                    {
                        P_SetMobjState(actor, actor->info->seestate);
                        actor->flags &= ~MF_JUSTHIT;
                    }

                    return true;
                }
            }
        }
        return false;
    }

    // Monster looking for a player to attack!

    // Don't look for a player if ignoring
    if( actor->eflags & MF_IGNOREPLAYER )
        return false;

//        sector = actor->subsector->sector;

    // This is not in PrBoom, EternityEngine, and it uses P_Random !!!
    // BP: first time init, this allow minimum lastlook changes
    if( (lastlook < 0) && (EV_legacy >= 129) )
    {
        // Legacy use of P_Random, enabled by EV_legacy >= 129.
#ifdef MAX_PLAYERS_VARIABLE  // unneeded
        // Boom, MBF demo assumes MAXPLAYERS=4, but Legacy MAXPLAYERS=32.
        lastlook = PP_Random(pL_initlastlook) % max_num_players;
#else
        // Boom, MBF Demo cannot reach here.
        lastlook = PP_Random(pL_initlastlook) % MAXPLAYERS;
#endif
    }

    actor->lastlook = lastlook;
    stop = (lastlook-1)&PLAYERSMASK;
    stopc = ( !EN_mbf
              && EN_boom  // !demo_compatibility
              && cv_monster_remember.EV )? MAXPLAYERS : 2;  // killough 9/9/98

    for ( ; ; lastlook = (lastlook+1)&PLAYERSMASK )
    {
        mobj_t*  pmo;
        actor->lastlook = lastlook;

        // [WDJ] The test for stop is ignored in PrBoom, MBF,
        // if that player is not present (broken test).
        // This test just gets to done_looking sooner,
        // which is not affected by playeringame.
        // But it affects SyncDebug.
        // done looking
        if( (EV_legacy >= 147 )
            && (lastlook == stop) )  goto done_looking;

        if( !playeringame[lastlook] )
            continue;

        if( --stopc < 0 )  goto done_looking;

        // Where PrBoom has this test, ignored when stop player is not present.
        if( lastlook == stop )  goto done_looking;  // broken test as in PrBoom

        player = &players[lastlook];
        if (player->health <= 0)
            continue;           // dead

        pmo = player->mo;  // player mobj
	
        // MBF, PrBoom: IsVisible does same as previous code.
        if( !P_IsVisible( actor, pmo, allaround) )
            continue;           // out of sight

        // Player found.

        if( EN_heretic && (pmo->flags & MF_SHADOW) )
        { // Player is invisible
            if((P_AproxDistance(pmo->x-actor->x, pmo->y-actor->y) > 2*MELEERANGE)
                && ( P_AproxDistance(pmo->momx, pmo->momy) < FIXINT(5)) )
            { // Player is sneaking - can't detect
                goto  none_found;
            }

            if(PP_Random(ph_notseen) < 225)
            { // Player isn't sneaking, but still didn't detect
                goto  none_found;
            }
        }

        // Remember old target node for later
        if (actor->target)
        {
            if(actor->target->type == MT_NODE)
               actor->targetnode = actor->target;
        }

        // New target found
        SET_TARGET_REF( actor->target, pmo );

        // killough 9/9/98: give monsters a threshold towards getting players
        // we don't want it to be too easy for a player with dogs :)
        if( cv_mbf_pursuit.EV )  // !comp[comp_pursuit]
            actor->threshold = 60;

        return true;
    }
    goto  none_found;

done_looking:
    // [WDJ] MBF, From MBF, PrBoom, EternityEngine.
    // e6y
    // Fixed Boom incompatibilities. The following code was missed.
    // There are no more desyncs on Donce's demos on horror.wad (in PrBoom).

    if( !EN_mbf
        && EN_boom  // !demo_compatibility
        && cv_monster_remember.EV )
    {
        // Use last known enemy if no players sighted -- killough 2/15/98:
        if( actor->lastenemy && (actor->lastenemy->health > 0) )
        {
            actor->target = actor->lastenemy;
            actor->lastenemy = NULL;
            return true;
        }
    }

none_found:
    return false;
}


// [WDJ] MBF, From MBF, PrBoom, EternityEngine.
//
// Friendly monsters, by Lee Killough 7/18/98
//
// Friendly monsters go after other monsters first, but also return to owner
// if they cannot find any targets.
// A marine's best friend :)  killough 7/18/98, 9/98

static
boolean P_LookForMonsters(mobj_t* actor, boolean allaround)
{
    int x, y, d;
    thinker_t *cap, *th;

#ifdef DEBUG_COME_HERE
    // DEBUG
    if( ! EN_boom )
        GenPrintf(EMSG_debug, "COME HERE ignored by EN_boom\n" );
    if( ! EN_mbf )
        GenPrintf(EMSG_debug, "COME HERE ignored by EN_mbf\n" );
#endif
    
    if( ! EN_boom )
        return false;

    // Boom: This test is at the end of P_LookForPlayers.
    if( cv_monster_remember.EV   // Boom smartypants = monsters_remember && EN_boom
        && actor->lastenemy
        && actor->lastenemy->health > 0
        && ! BOTH_FRIEND(actor->lastenemy, actor) // not friends
      )
    {
        // Boom
        // Use last known enemy if no players sighted.
        SET_TARGET_REF( actor->target, actor->lastenemy );
        SET_TARGET_REF( actor->lastenemy, NULL );

#ifdef DEBUG_COME_HERE
        if( come_here && (actor->flags & MF_FRIEND) )  // dogs and bots
        {
            GenPrintf(EMSG_debug, "COME HERE ignored by monster remember, come_here=%i\n", come_here );
            if( come_here_player && SAME_FRIEND(actor, come_here_player) )
            {
                GenPrintf(EMSG_debug, "COME HERE ignored by monster remember, come_here_player OK, come_here=%i\n", come_here );
            }
        }
#endif
        return true;
    }

    // Old demos do not support monster-seeking bots
    if( ! EN_mbf )
        return false;

#ifdef ENABLE_COME_HERE
    if( come_here && (actor->flags & MF_FRIEND) )  // dogs and bots
    {
# ifdef DEBUG_COME_HERE
        if( ! come_here_player )
            GenPrintf(EMSG_debug, "COME HERE player missing, come_here=%i\n", come_here );
# endif

        if( ! SAME_FRIEND(actor, come_here_player) )
# ifdef DEBUG_COME_HERE
            GenPrintf(EMSG_debug, "COME HERE player not a friend, come_here=%i\n", come_here );
# endif

        if( come_here_player && SAME_FRIEND(actor, come_here_player) )
        {
# ifdef DEBUG_COME_HERE
            GenPrintf(EMSG_debug, "COME HERE set target FRIEND, come_here=%i\n", come_here );
# endif
            SET_TARGET_REF( actor->target, come_here_player ); // who said come-here
            SET_TARGET_REF( actor->lastenemy, NULL );
            return true;
        }
    }
#endif

    // Search the class-list corresponding to this actor's potential targets
    cap = &thinkerclasscap[actor->flags & MF_FRIEND ? TH_enemies : TH_friends];
    if( cap->cnext == cap )  // When empty list, bail out early
        return false;

    // Search for new enemy
    x = (actor->x - bmaporgx)>>MAPBLOCKSHIFT;
    y = (actor->y - bmaporgy)>>MAPBLOCKSHIFT;

    // Parameters PIT_FindTarget
    ft_current_actor = actor;
    ft_current_allaround = allaround;
   
    // Search first in the immediate vicinity.
    if( !P_BlockThingsIterator(x, y, PIT_FindTarget) )
        return true;

    for (d=1; d<5; d++)
    {
        int i = 1 - d;

        do{
            if( !P_BlockThingsIterator(x+i, y-d, PIT_FindTarget )
                || !P_BlockThingsIterator(x+i, y+d, PIT_FindTarget) )
                return true;
        }while (++i < d);

        do{
            if( !P_BlockThingsIterator(x-d, y+i, PIT_FindTarget )
                || !P_BlockThingsIterator(x+d, y+i, PIT_FindTarget) )
                return true;
        }while (--i + d >= 0);
    }

    {   // Random number of monsters, to prevent patterns from forming
        int n = (PP_Random(pr_friends) & 31) + 15;

        for( th = cap->cnext; th != cap; th = th->cnext )
        {
            if( --n < 0 )
            {
                // Only a subset of the monsters were searched. Move all of
                // the ones which were searched so far, to the end of the list.
                P_MoveClasslistRangeLast( cap, th );
                break;
            }

            if( !PIT_FindTarget((mobj_t*) th) )  // If target sighted
                return true;
        }
    }

    return false;  // No monster found
}


// [WDJ] MBF, From MBF, PrBoom, EternityEngine.
//
// P_LookForTargets
//
// killough 9/5/98: look for targets to go after, depending on kind of monster
//

static
boolean P_LookForTargets(mobj_t* actor, int allaround)
{
    if( actor->flags & MF_FRIEND )
    {
        // MBF only
        return P_LookForMonsters(actor, allaround)  // Boom, MBF
               || P_LookForPlayers (actor, allaround);
    }

    return P_LookForPlayers (actor, allaround)
           || P_LookForMonsters(actor, allaround);  // Boom, MBF
}


// [WDJ] MBF, From MBF, PrBoom, EternityEngine.
//
// P_HelpFriend
//
// killough 9/8/98: Help friends in danger of dying
//

static
boolean P_HelpFriend(mobj_t* actor)
{
    thinker_t *cap, *th;

    // If less than 33% health, self-preservation rules
    if( actor->health*3 < actor->info->spawnhealth )
        return false;

    // Parameters PIT_FindTarget
    ft_current_actor = actor;
    ft_current_allaround = true;

    // Possibly help a friend under 50% health
    cap = &thinkerclasscap[actor->flags & MF_FRIEND ? TH_friends : TH_enemies];

    for (th = cap->cnext; th != cap; th = th->cnext)
    {
        mobj_t * mo = (mobj_t*) th;  // thinker mobj 
        if( mo->health*2 >= mo->info->spawnhealth )
        {
            if( PP_Random(pr_helpfriend) < 180 )
                break;
        }
        else
        {
            if( mo->flags & MF_JUSTHIT
                && mo->target
                && mo->target != actor->target
                && !PIT_FindTarget( mo->target)
              )
            {
                // Ignore any attacking monsters, while searching for friend
                actor->threshold = BASETHRESHOLD;
                return true;
            }
        }
    }

    return false;
}


//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//
void A_Look (mobj_t* actor)
{
    mobj_t*     targ;

    // Is there a node we must follow?
    if( actor->targetnode )
    {
        actor->target = actor->targetnode;
        P_SetMobjState(actor, actor->info->seestate);
        return;
    }

    actor->threshold = 0;       // any shot will wake up

    // [WDJ] From Prboom, MBF, EternityEngine
    // killough 7/18/98:
    // Friendly monsters go after other monsters first, but also return to
    // player, without attacking them, if they cannot find any targets.
    actor->pursuecount = 0;

    if( actor->flags & MF_FRIEND )
    {
#ifdef DEBUG_COME_HERE
        // DEBUG	
        if( come_here && (actor->flags & MF_FRIEND) )  // dogs and bots
        {
            if( come_here_player && SAME_FRIEND(actor, come_here_player) )
            {
                GenPrintf(EMSG_debug, "A_Look, come_here=%i\n", come_here );
            }
        }
#endif

        if( P_LookForTargets(actor, false) )  goto seeyou;
    }

    targ = actor->subsector->sector->soundtarget;
    if( targ && (targ->flags & MF_SHOOTABLE) )
    {
        SET_TARGET_REF( actor->target, targ );       

        if( !(actor->flags & MF_AMBUSH) )   goto seeyou;
        if( P_CheckSight(actor, targ) )     goto seeyou;
    }

#if 0   
    if( !EN_mbf )
    {
        if( P_LookForPlayers (actor, false) )   goto seeyou;
        return;
    }
#endif

    if( !(actor->flags & MF_FRIEND) )
    {
        // Look for Players, then monsters.
        if( P_LookForTargets(actor, false) )  goto seeyou;
    }
    return;

    // go into chase state
  seeyou:
#ifdef ENABLE_SLOW_REACT
    if( cv_slow_react.EV )
    {
        uint16_t info_reactiontime = actor->info->reactiontime;
	// reactiontime is already set from info.
        if( actor->reactiontime <= info_reactiontime )
        {
	    // [WDJ] Set the reactiontime to delay going to chase.
            // This delay counts about 2 per second.
            uint32_t srt = cv_slow_react.EV * 5;  // 5/16 sec each unit, approx.
            // Add to existing reaction time, 5/16 to 20/16 sec per step.
            actor->reactiontime += (srt + ((PP_Random(pL_slowreact) * srt * 3) >> 8)) >> 4;
            return;  // delay
        }

        // There is no other countdown
        if( --actor->reactiontime > info_reactiontime )
            return;  // delay
    }
#endif

    if (actor->info->seesound)
    {
        int  sound;

        switch (actor->info->seesound)
        {
          case sfx_posit1:
          case sfx_posit2:
          case sfx_posit3:
            sound = sfx_posit1+PP_Random(pr_see)%3;
            break;

          case sfx_bgsit1:
          case sfx_bgsit2:
            sound = sfx_bgsit1+PP_Random(pr_see)%2;
            break;

          default:
            sound = actor->info->seesound;
            break;
        }

#ifdef MBF21
        // MT_SPIDER, MT_CYBORG have MF3_FULLVOLSOUNDS set
        if( (actor->flags2 & MF2_BOSS)  // Heretic, MBF21
            || (actor->flags3 & MF3_FULLVOLSOUNDS) )  // MBF21
#else
        if (actor->type==MT_SPIDER
         || actor->type == MT_CYBORG
         || (actor->flags2&MF2_BOSS))
#endif
        {
            // full volume
            S_StartSound(sound);
        }
        else
        {
            S_StartScreamSound(actor, sound);
        }
    }

    P_SetMobjState (actor, actor->info->seestate);
}


// [WDJ] Functions from PrBoom, MBF, EternityEngine.

// killough 10/98:
// Allows monsters to continue movement while attacking
// EternityEngine makes it BEX available.
//
void A_KeepChasing_MBF(mobj_t* actor)
{
#ifdef DEBUG_COME_HERE
    // DEBUG	
    if( come_here && (actor->flags & MF_FRIEND) )  // dogs and bots
    {
        if( come_here_player && SAME_FRIEND(actor, come_here_player) )
        {
            GenPrintf(EMSG_debug, "A_KeepChasing, come_here=%i\n", come_here );
        }
    }
#endif

  if (actor->movecount)
  {
      actor->movecount--;
      if (actor->strafecount)
        actor->strafecount--;
      P_SmartMove(actor);
  }
}

//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase (mobj_t*   actor)
{
    int  delta;

#ifdef ENABLE_COME_HERE
    // DEBUG	
    if( come_here && (actor->flags & MF_FRIEND) )  // dogs and bots
    {
#ifdef DEBUG_COME_HERE
        if( ! come_here_player )
            GenPrintf(EMSG_debug, "COME HERE player missing, come_here=%i\n", come_here );

        if( ! SAME_FRIEND(actor, come_here_player) )
            GenPrintf(EMSG_debug, "COME HERE player not a friend, come_here=%i\n", come_here );
#endif

        if( come_here_player && SAME_FRIEND(actor, come_here_player) )
        {
#ifdef DEBUG_COME_HERE
            GenPrintf(EMSG_debug, "A_Chase, come_here=%i\n", come_here );
#endif
            if( come_here
                && (!actor->target || !actor->target->player || (actor->target->player->mo != come_here_player)) )
            {
#ifdef DEBUG_COME_HERE
                 GenPrintf(EMSG_debug, "A_Chase, LookForMonsters, come_here=%i\n", come_here );
#endif
                 P_LookForMonsters( actor, false );
            }
        }
    }
#endif
    
    if (actor->reactiontime)
    {
        actor->reactiontime--;

        // We are pausing at a node, just look for players
        if ( actor->target && actor->target->type == MT_NODE)
        {
            P_LookForPlayers(actor, false);
            return;
        }
    }


    // modify target threshold
    if( actor->threshold )
    {
        if (EN_doom_etc 
            && (!actor->target
              || actor->target->health <= 0
//              || (actor->target->flags & MF_CORPSE)  // corpse health < 0
            ) )
        {
            actor->threshold = 0;
        }
        else
            actor->threshold--;
    }

    if( EN_heretic && cv_fastmonsters.EV)
    { // Monsters move faster in nightmare mode
        actor->tics -= actor->tics/2;
        if(actor->tics < 3)
            actor->tics = 3;
    }

    // Turn towards movement direction if not there yet.
    // killough 9/7/98: keep facing towards target if strafing or backing out
    if( actor->strafecount )  // MBF
    {
        A_FaceTarget(actor);
    }
    else if( actor->movedir < 8 )
    {
        actor->angle &= (7<<29);
        delta = actor->angle - (actor->movedir << 29);

        if (delta > 0)
            actor->angle -= ANG90/2;
        else if (delta < 0)
            actor->angle += ANG90/2;
    }

// [WDJ] compiler complains, "suggest parenthesis"
#if 0
   // Original code was
        if (!actor->target
        || !(actor->target->flags&MF_SHOOTABLE)
                && actor->target->type != MT_NODE
                && !(actor->eflags & MF_IGNOREPLAYER))
#else
// but, based on other tests, the last two tests were added later.
// [WDJ] I think they meant:
    if ( !( actor->target
            && ( actor->target->flags&MF_SHOOTABLE
                 || actor->target->type == MT_NODE
               ) )
         && !(actor->eflags & MF_IGNOREPLAYER)
       )
#endif       
    {
        // [WDJ] Recursion check from EternityEngine.
        // haleyjd 07/26/04: Detect and prevent infinite recursion if
        // Chase is called from a thing's spawnstate.
        static byte  chase_recursion = 0;
        static byte  chase_recursion_count = 0;  // [WDJ] too many messages

        // If recursion is true at this point, P_SetMobjState sent
        // us back here -- print an error message and return.
        if( chase_recursion )
        {
            if( verbose > 1 && (chase_recursion_count++ == 0) )
                GenPrintf( EMSG_warn, "Chase recursion detected\n");
            return;
        }

        // set the flag true before calling P_SetMobjState
        chase_recursion = 1;
#if 1
        // MBF
        // look for a new target
        if( ! P_LookForTargets(actor,true) )
#else
        // look for a new target
        if( ! P_LookForPlayers(actor,true) )
#endif
        {
            // None found
            // This monster will start waiting again
            P_SetMobjState (actor, actor->info->spawnstate);
        }

        chase_recursion = 0; // Must clear this, in either case.
        return;
    }

    // do not attack twice in a row
    if (actor->flags & MF_JUSTATTACKED)
    {
        actor->flags &= ~MF_JUSTATTACKED;
        // MBF: if( gameskill != nightmare && !fastparm )
        if( !cv_fastmonsters.EV )
            P_NewChaseDir (actor);
        return;
    }

    // check for melee attack
    if (actor->info->meleestate
        && P_CheckMeleeRange (actor)
        && (actor->target && actor->target->type != MT_NODE)
        && !(actor->eflags & MF_IGNOREPLAYER))
    {
        if (actor->info->attacksound)
            S_StartAttackSound(actor, actor->info->attacksound);

        P_SetMobjState (actor, actor->info->meleestate);
       
        //MBF: killough 8/98: remember an attack
        // cph - DEMOSYNC?
        if( EN_mbf
            && !actor->info->missilestate )
            actor->flags |= MF_JUSTHIT;

        return;
    }

    // check for missile attack
    if (actor->info->missilestate
        && (actor->target && actor->target->type != MT_NODE)
        && !(actor->eflags & MF_IGNOREPLAYER))
    {
        if( !cv_fastmonsters.EV && actor->movecount )
        {
            goto nomissile;
        }

        if (!P_CheckMissileRange (actor))
            goto nomissile;

        P_SetMobjState (actor, actor->info->missilestate);
        actor->flags |= MF_JUSTATTACKED;
        return;
    }

    // ?
  nomissile:
    // possibly choose another target
#if 0
    // DoomLegacy 1.46.3
    if (multiplayer
        && !actor->threshold
        && !P_CheckSight (actor, actor->target)
        && (actor->target && actor->target->type != MT_NODE)
        && !(actor->eflags & MF_IGNOREPLAYER))
    {
        if (P_LookForPlayers(actor,true))
            return;     // got a new target

    }
#endif
    if( !actor->threshold )
    {
        // [WDJ] MBF, from MBF, PrBoom, EternityEngine.
        if( ! EN_mbf )
        {
            // Doom, Boom, Legacy
            if( multiplayer
                && !P_CheckSight (actor, actor->target)
                && (actor->target && actor->target->type != MT_NODE)
                && !(actor->eflags & MF_IGNOREPLAYER))
            {
                if( P_LookForPlayers(actor,true) )
                    return;     // got a new target
            }
        }
        // MBF
        // killough 7/18/98, 9/9/98: new monster AI
        else if( cv_mbf_help_friend.EV && P_HelpFriend(actor) )
            return;  // killough 9/8/98: Help friends in need.
        // Look for new targets if current one is bad or is out of view.
        else if( actor->pursuecount )
            actor->pursuecount--;
        else
        {
            // Our pursuit time has expired. We're going to think about
            // changing targets.
            actor->pursuecount = BASETHRESHOLD;

            // Unless (we have a live target, and it's not friendly
            // and we can see it)
            // try to find a new one; return if sucessful.
            if( !actor->target
                || ! (actor->target->health > 0)
                ||( (cv_mbf_pursuit.EV || multiplayer)
                    && ! ( ( ! SAME_FRIEND(actor->target, actor)
                             ||( !(actor->flags & MF_FRIEND)
                                 && MONSTER_INFIGHTING
                               )
                            )
                            && P_CheckSight(actor, actor->target)
                         )
                  )
              )
            {
                if( P_LookForTargets(actor, true) )
                    return;
            }

            // Current target was good, or no new target was found.
            // If monster is a missile-less friend, give up pursuit and
            // return to player, if no attacks have occurred recently.

            if( (actor->flags & MF_FRIEND) && !actor->info->missilestate )
            {
                // if recent action, keep fighting, else return to player.
                if( actor->flags & MF_JUSTHIT)
                    actor->flags &= ~MF_JUSTHIT;
                else if( P_LookForPlayers(actor, true) )
                    return;
            }
        }
    }

    if( actor->strafecount )  // MBF
        actor->strafecount--;

    // Patrolling nodes
    if (actor->target && actor->target->type == MT_NODE)
    {
        // Check if a player is near
        if (P_LookForPlayers(actor, false))
        {
            // We found one, let him know we saw him!
            S_StartScreamSound(actor, actor->info->seesound);
            return;
        }

        // Did we touch a node as target?
        if (R_PointToDist2(actor->x, actor->y, actor->target->x, actor->target->y)
            <= actor->target->info->radius )
        {

            // Execute possible FS script
            if (actor->target->nodescript)
            {
                T_RunScript((actor->target->nodescript - 1), actor);
            }

            // Do we wait here?
            if (actor->target->nodewait)
                actor->reactiontime = actor->target->nodewait;

            // Set next node, if any
            if (actor->target->nextnode)
            {
                // Also remember it, if we will encounter an enemy			   
                actor->target = actor->target->nextnode;
                actor->targetnode = actor->target->nextnode;
            }
            else
            {
                actor->target = NULL;
                actor->targetnode = NULL;
            }

            return;
        }
    }


    // chase towards player
    if (--actor->movecount<0
        || !P_SmartMove(actor) )
    {
        P_NewChaseDir (actor);
    }


    // make active sound
    if (actor->info->activesound
        && PP_Random(pr_see) < 3)
    {
        if(actor->type == MT_WIZARD && PP_Random(ph_wizscream) < 128)
            S_StartScreamSound(actor, actor->info->seesound);
        else if(actor->type == MT_SORCERER2)
            S_StartSound( actor->info->activesound );
#ifdef HEXEN
        else if(actor->type == MT_HEXEN_BISHOP && PP_Random(ph_hexen_bishop) < 128)
            S_StartScreamSound(actor, actor->info->seesound);
        else if(actor->type == MT_HEXEN_PIG)
            S_StartScreamSound(actor, hexen_pig_sound[ PP_Random(ph_hexen_pig) & 0x01 ]);
        else if(EN_hexen && actor->flags2 & MF2_BOSS)
            S_StartSound( actor->info->activesound );
#endif
        else
            S_StartScreamSound(actor, actor->info->activesound);
    }

}


//
// A_FaceTarget
//
void A_FaceTarget (mobj_t* actor)
{
    if (!actor->target)
        return;

    actor->flags &= ~MF_AMBUSH;

    actor->angle = R_PointToAngle2 (actor->x, actor->y,
                                    actor->target->x, actor->target->y );

    if (actor->target->flags & MF_SHADOW)
        actor->angle += PP_SignedRandom(pr_facetarget)<<21;
}


//
// A_PosAttack
//
void A_PosAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    S_StartAttackSound(actor, sfx_pistol);
    int angle = actor->angle;
    int slope = P_AimLineAttack (actor, angle, MISSILERANGE, 0);

    angle += PP_SignedRandom(pr_posattack)<<20;
    int damage = ((PP_Random(pr_posattack)%5)+1)*3;
    P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_SPosAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    S_StartAttackSound(actor, sfx_shotgn);
    int bangle = actor->angle;
    int slope = P_AimLineAttack (actor, bangle, MISSILERANGE, 0);

    int i;
    for (i=0 ; i<3 ; i++)
    {
        int angle  = (PP_SignedRandom(pr_sposattack)<<20)+bangle;
        int damage = ((PP_Random(pr_sposattack)%5)+1)*3;
        P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
    }
}

void A_CPosAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    S_StartAttackSound(actor, sfx_shotgn);
    int bangle = actor->angle;
    int slope = P_AimLineAttack (actor, bangle, MISSILERANGE, 0);

    int angle  = (PP_SignedRandom(pr_cposattack)<<20)+bangle;

    int damage = ((PP_Random(pr_cposattack)%5)+1)*3;
    P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_CPosRefire (mobj_t* actor)
{
    // keep firing unless target got out of sight
    A_FaceTarget (actor);

    // MBF: Simplified, HitFriend only true when friend.
    if( actor->flags & MF_FRIEND )
    {
        // Killough: stop firing if a friend has gotten in the way
        if( P_HitFriend(actor) )
            goto stop_refire;
    }

    mobj_t * target = actor->target;

    if( PP_Random(pr_cposrefire) < 40 )
    {
        // MBF:
        // Killough: Prevent firing on friends continuously.
        if( target && BOTH_FRIEND( actor, target ) )
            goto stop_refire;	

        return;
    }

    if( !target
        || (target->health <= 0)
//      || target->flags & MF_CORPSE  // corpse health < 0
        || ! P_CheckSight( actor, target ) )
        goto stop_refire;

    return;
   
stop_refire:   
    P_SetMobjState( actor, actor->info->seestate );
}


void A_SpidRefire (mobj_t* actor)
{
    // keep firing unless target got out of sight
    A_FaceTarget (actor);

    // MBF: Simplified, HitFriend only true when friend.
    if( actor->flags & MF_FRIEND )
    {
        // Killough: stop firing if a friend has gotten in the way
        if( P_HitFriend(actor) )
            goto stop_refire;
    }

    if( PP_Random(pr_spidrefire) < 10 )
        return;

    mobj_t * target = actor->target;
    if( !target
        || (target->health <= 0)
//      || target->flags & MF_CORPSE  // corpse health < 0
        || BOTH_FRIEND( actor, target )
        || ! P_CheckSight( actor, target ) )
        goto stop_refire;

    return;

stop_refire:   
    P_SetMobjState( actor, actor->info->seestate );
}

void A_BspiAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
        S_StartAttackSound(actor, sfx_claw);
        int damage = (PP_Random(pr_troopattack)%8+1)*3;
        P_DamageMobj (actor->target, actor, actor, damage);
        return;
    }


    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_TROOPSHOT);
}


void A_SargAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
#if 0
    // Doom_12_compatibility, not in DoomLegacy
    if( ! EN_melee_radius_adj )   // doom_12_compatibility
    {
        // Doom 1.2
        int damage = ((PP_Random(pr_sargattack)%10)+1)*4;
        P_LineAttack(actor, actor->angle, MELEERANGE, 0, damage);
    }
    else
#endif
    if (P_CheckMeleeRange (actor))
    {
        int damage = ((PP_Random(pr_sargattack)%10)+1)*4;
        P_DamageMobj (actor->target, actor, actor, damage);
    }
}

// Heretic: A_HHeadAttack(mobj_t* actor)
// Doom
void A_HeadAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    // Doom only, for Heretic see A_HHeadAttack
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
        int damage = (PP_Random(pr_headattack)%6+1)*10;
        P_DamageMobj (actor->target, actor, actor, damage);
        return;
    }

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_HEADSHOT);
}

void A_CyberAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    P_SpawnMissile (actor, actor->target, MT_ROCKET);
}


void A_BruisAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    if (P_CheckMeleeRange (actor))
    {
        S_StartAttackSound(actor, sfx_claw);
        int damage = (PP_Random(pr_bruisattack)%8+1)*10;
        P_DamageMobj (actor->target, actor, actor, damage);
        return;
    }

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile (mobj_t* actor)
{
    mobj_t*     mo;

    if (!actor->target)
        return;

    A_FaceTarget (actor);
    actor->z += FIXINT(16);    // so missile spawns higher
    mo = P_SpawnMissile (actor, actor->target, MT_TRACER);
    actor->z -= FIXINT(16);    // back to normal

    if(mo)
    {
        mo->x += mo->momx;
        mo->y += mo->momy;
        SET_TARGET_REF( mo->tracer, actor->target );
    }
}

angle_t  TRACEANGLE = 0x0c000000;

void A_Tracer (mobj_t* actor)
{
    angle_t     exact;
    fixed_t     dist;
    fixed_t     slope;
    mobj_t    * dest;
    mobj_t    * th;

    // killough 1/18/98: this is why some missiles do not have smoke and some do.
    // Also, internal demos start at random gametics, thus the bug in which
    // revenants cause internal demos to go out of sync.
    //
    // [WDJ] Use a proper tic, demo_comp_tic, that only runs while demo plays,
    // instead of the contrived basetic correction.
    //
    // It would have been better to use leveltime to start with in Doom,
    // but since old demos were recorded using gametic, we must stick with it,
    // and improvise around it (using leveltime causes desync across levels).

    // [WDJ] As NEWTICRATERATIO = 1, this is same as gametic & 3, 
    // PrBoom, MBF: use (gametic - basetic), which is same as demo_comp_tick.
    if( ((EV_legacy && (EV_legacy < 147))? gametic : game_comp_tic) % (4 * NEWTICRATERATIO) )
        return;
   
    // spawn a puff of smoke behind the rocket
    P_SpawnPuff (actor->x, actor->y, actor->z);

    th = P_SpawnMobj ((actor->x - actor->momx), (actor->y - actor->momy),
                      actor->z, MT_SMOKE);

    th->momz = FIXINT(1);
    th->tics -= PP_Random(pr_tracer)&3;
    if (th->tics < 1)
        th->tics = 1;

    // adjust direction
    dest = actor->tracer;

    if (!dest || dest->health <= 0)
        return;

    // change angle
    exact = R_PointToAngle2 (actor->x, actor->y,
                             dest->x, dest->y);

    if (exact != actor->angle)
    {
        if (exact - actor->angle > 0x80000000) // (actor->angle > exact)
        {
            actor->angle -= TRACEANGLE;  // correct towards exact
            if (exact - actor->angle < 0x80000000)  // (actor->angle < exact)
                actor->angle = exact;
        }
        else
        {
            actor->angle += TRACEANGLE;  // correct towards exact
            if (exact - actor->angle > 0x80000000)  // (actor->angle > exact)
                actor->angle = exact;
        }
    }

    int angf = ANGLE_TO_FINE(actor->angle);
#ifdef MONSTER_VARY
    actor->momx = FixedMul (actor->speed, finecosine[angf]);
    actor->momy = FixedMul (actor->speed, finesine[angf]);
#else
    actor->momx = FixedMul (actor->info->speed, finecosine[angf]);
    actor->momy = FixedMul (actor->info->speed, finesine[angf]);
#endif

    // change slope
    dist = P_AproxDistance (dest->x - actor->x,
                            dest->y - actor->y);

#ifdef MONSTER_VARY
    dist = dist / actor->speed;
#else
    dist = dist / actor->info->speed;
#endif

    if (dist < 1)
        dist = 1;
    slope = (dest->z + FIXINT(40) - actor->z) / dist;

    if (slope < actor->momz)
        actor->momz -= FRACUNIT/8;
    else
        actor->momz += FRACUNIT/8;
}


void A_SkelWhoosh (mobj_t*      actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    // judgecutor:
    // CHECK ME!
    S_StartAttackSound(actor, sfx_skeswg);
}

void A_SkelFist (mobj_t*        actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);

    if (P_CheckMeleeRange (actor))
    {
        S_StartAttackSound(actor, sfx_skepch);
        int damage = ((PP_Random(pr_skelfist)%10)+1)*6;
        P_DamageMobj (actor->target, actor, actor, damage);
    }
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//
// PIT_VileCheck Global parameter
static mobj_t * vilecheck_r_corpse;  // OUT
static mobj_t * vilecheck_actor;
#ifdef MBF21
static mobjinfo_t * vilecheck_actor_info;
#endif
static xyz_t    vilecheck_pos;

boolean PIT_VileCheck (mobj_t*  thing)
{
    int         maxdist;
    boolean     check;

    if (!(thing->flags & MF_CORPSE) )
        return true;    // not a monster

    if (thing->tics != -1)
        return true;    // not lying still yet

    if (thing->info->raisestate == S_NULL)
        return true;    // monster doesn't have a raise state

#ifdef MBF21
    maxdist = thing->info->radius + vilecheck_actor_info->radius;
#else
    maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;
#endif

    if ( abs(thing->x - vilecheck_pos.x) > maxdist
         || abs(thing->y - vilecheck_pos.y) > maxdist )
        return true;            // not actually touching

    // [WDJ] Prevent raise of corpse on another 3d floor.
    // Because corpse may have 0 height, use only vile reach.
#ifdef MBF21
    if( thing->z > (vilecheck_pos.z + vilecheck_actor_info->height)
        || thing->z < (vilecheck_pos.z - FIXINT(20)) )  // reasonable reach down
        return true;
#else
    if( thing->z > (vilecheck_pos.z + mobjinfo[MT_VILE].height)
        || thing->z < (vilecheck_pos.z - FIXINT(20)) )  // reasonable reach down
        return true;
#endif

    vilecheck_r_corpse = thing;
    vilecheck_r_corpse->momx = vilecheck_r_corpse->momy = 0;

    if( EN_vile_revive_bug )
    {
        // [WDJ] The original code.  Corpse heights are not this simple.
        // Would touch another monster and get stuck.
        // In PrBoom this is enabled by comp[comp_vile].
        vilecheck_r_corpse->height <<= 2;
        check = P_CheckPosition (vilecheck_r_corpse, vilecheck_r_corpse->x, vilecheck_r_corpse->y);
        vilecheck_r_corpse->height >>= 2;
    }
    else
    {
        // [WDJ] Test with revived sizes from info, to fix monsters stuck together bug.
        // Must test as it would be revived, and then restore after the check
        // (because a collision could be found).
        // From considering the same fix in zdoom and prboom.
        fixed_t corpse_height = vilecheck_r_corpse->height;
        vilecheck_r_corpse->height = vilecheck_r_corpse->info->height; // revived height
        fixed_t corpse_radius = vilecheck_r_corpse->radius;
        vilecheck_r_corpse->radius = vilecheck_r_corpse->info->radius; // revived radius
        int corpse_flags = vilecheck_r_corpse->flags;
        vilecheck_r_corpse->flags |= MF_SOLID; // revived would be SOLID
        check = P_CheckPosition (vilecheck_r_corpse, vilecheck_r_corpse->x, vilecheck_r_corpse->y);
        vilecheck_r_corpse->height = corpse_height;
        vilecheck_r_corpse->radius = corpse_radius;
        vilecheck_r_corpse->flags = corpse_flags;
    }

    return !check;	// stop searching when no collisions found
}


#ifdef MBF21
// [WDJ] MBF21: Separate function for HealCorpse.
// Called by A_VileChase, and MBF21 A_HealChase.
// Return 1 when corpse is raised.
static
byte P_HealCorpse( mobj_t * actor, mobjinfo_t * actor_info, statenum_t healstate, sfxid_t healsound )
{
    int    xl, xh;
    int    yl, yh;

    int    bx, by;

    mobjinfo_t * info;
    mobj_t     * temp;

    if( actor->movedir != DI_NODIR )
    {
        // check for corpses to raise
        // Parameters to PIT_VileCheck.
#ifdef MONSTER_VARY
        vilecheck_pos.x =
            actor->x + actor->speed * xspeed[actor->movedir];
        vilecheck_pos.y =
            actor->y + actor->speed * yspeed[actor->movedir];
#else
        vilecheck_pos.x =
            actor->x + actor->info->speed * xspeed[actor->movedir];
        vilecheck_pos.y =
            actor->y + actor->info->speed * yspeed[actor->movedir];
#endif
        vilecheck_pos.z = actor->z;

        xl = (vilecheck_pos.x - bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
        xh = (vilecheck_pos.x - bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
        yl = (vilecheck_pos.y - bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
        yh = (vilecheck_pos.y - bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;

        vilecheck_actor = actor;
        vilecheck_actor_info = actor_info; // Added for MBF21
        for (bx=xl ; bx<=xh ; bx++)
        {
            for (by=yl ; by<=yh ; by++)
            {
                // Call PIT_VileCheck to check whether object is a corpse
                // that can be raised.
                if( !P_BlockThingsIterator(bx, by, PIT_VileCheck) )
                    goto raise_corpse;
            }
        }
    }
    return 0;


raise_corpse:
    // got one!
    temp = actor->target;
    actor->target = vilecheck_r_corpse;  // do not change reference
    A_FaceTarget (actor);
    actor->target = temp;  // reference is still correct

    P_SetMobjState( actor, healstate ); // MBF21 parameter
    S_StartObjSound( vilecheck_r_corpse, healsound );  // MBF21 parameter
    info = vilecheck_r_corpse->info;

    P_SetMobjState (vilecheck_r_corpse,info->raisestate);
    if( demoversion<129 )
    {
        // original code, with ghost bug
        // does not work when monster has been crushed
        vilecheck_r_corpse->height <<= 2;
    }
    else
    {
        // fix vile revives crushed monster as ghost bug
        vilecheck_r_corpse->height = info->height;
        vilecheck_r_corpse->radius = info->radius;
    }
    vilecheck_r_corpse->flags = info->flags & ~MF_FRIEND;
    vilecheck_r_corpse->health = info->spawnhealth;
    SET_TARGET_REF( vilecheck_r_corpse->target, NULL );

    if( EN_mbf )
    {
        // killough 7/18/98:
        // friendliness is transferred from vile to raised corpse
        vilecheck_r_corpse->flags =
            (info->flags & ~(MF_FRIEND|MF_JUSTHIT)) | (actor->flags & MF_FRIEND);

#if 0
        if( vilecheck_r_corpse->flags & (MF_FRIEND | MF_COUNTKILL) == MF_FRIEND)
            totallive++;
#endif

        SET_TARGET_REF( vilecheck_r_corpse->lastenemy, NULL );

        // killough 8/29/98: add to appropriate thread.
        P_UpdateClassThink( &vilecheck_r_corpse->thinker, TH_unknown );
    }
    return 1;
}
#endif


//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase (mobj_t* actor)
{
// [WDJ] MBF21: This is broken into separate functions.
#ifdef MBF21
    // [WDJ] There are several ways to pass MT_VILE specifics.
    // Actor should be a MT_VILE, unless some other new hacks are being used.
    // Need more info than was done in DSDA-Doom.
    if( ! P_HealCorpse( actor, &mobjinfo[MT_VILE], S_VILE_HEAL1, sfx_slop ) )
    {
        // Return to normal attack.
        A_Chase( actor );
    }
#else
    int    xl, xh;
    int    yl, yh;

    int    bx, by;

    mobjinfo_t * info;
    mobj_t     * temp;

    if (actor->movedir != DI_NODIR)
    {
        // check for corpses to raise
        // Parameters to VileCheck.
#ifdef MONSTER_VARY
        vilecheck_pos.x =
            actor->x + actor->speed * xspeed[actor->movedir];
        vilecheck_pos.y =
            actor->y + actor->speed * yspeed[actor->movedir];
#else
        vilecheck_pos.x =
            actor->x + actor->info->speed * xspeed[actor->movedir];
        vilecheck_pos.y =
            actor->y + actor->info->speed * yspeed[actor->movedir];
#endif
        vilecheck_pos.z = actor->z;

        xl = (vilecheck_pos.x - bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
        xh = (vilecheck_pos.x - bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
        yl = (vilecheck_pos.y - bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
        yh = (vilecheck_pos.y - bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;

        vilecheck_actor = actor;
        for (bx=xl ; bx<=xh ; bx++)
        {
            for (by=yl ; by<=yh ; by++)
            {
                // Call PIT_VileCheck to check whether object is a corpse
                // that can be raised.
                if (!P_BlockThingsIterator(bx,by,PIT_VileCheck))
                    goto raise_corpse;
            }
        }
    }

    // Return to normal attack.
    A_Chase (actor);
    return;

raise_corpse:
    // got one!
    temp = actor->target;
    actor->target = vilecheck_r_corpse;  // do not change reference
    A_FaceTarget (actor);
    actor->target = temp;  // reference is still correct

    P_SetMobjState (actor, S_VILE_HEAL1);
    S_StartObjSound( vilecheck_r_corpse, sfx_slop );
    info = vilecheck_r_corpse->info;

    P_SetMobjState (vilecheck_r_corpse,info->raisestate);
    if( demoversion<129 )
    {
        // original code, with ghost bug
        // does not work when monster has been crushed
        vilecheck_r_corpse->height <<= 2;
    }
    else
    {
        // fix vile revives crushed monster as ghost bug
        vilecheck_r_corpse->height = info->height;
        vilecheck_r_corpse->radius = info->radius;
    }
    vilecheck_r_corpse->flags = info->flags & ~MF_FRIEND;
    vilecheck_r_corpse->health = info->spawnhealth;
    SET_TARGET_REF( vilecheck_r_corpse->target, NULL );

    if( EN_mbf )
    {
        // killough 7/18/98:
        // friendliness is transferred from vile to raised corpse
        vilecheck_r_corpse->flags =
            (info->flags & ~(MF_FRIEND|MF_JUSTHIT)) | (actor->flags & MF_FRIEND);

#if 0
        if( vilecheck_r_corpse->flags & (MF_FRIEND | MF_COUNTKILL) == MF_FRIEND)
            totallive++;
#endif

        SET_TARGET_REF( vilecheck_r_corpse->lastenemy, NULL );

        // killough 8/29/98: add to appropriate thread.
        P_UpdateClassThink( &vilecheck_r_corpse->thinker, TH_unknown );
    }
    return;
#endif
}


//
// A_VileStart
//
void A_VileStart (mobj_t* actor)
{
    S_StartAttackSound(actor, sfx_vilatk);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//
void A_Fire (mobj_t* actor);

void A_StartFire (mobj_t* actor)
{
    S_StartObjSound(actor, sfx_flamst);
    A_Fire(actor);
}

void A_FireCrackle (mobj_t* actor)
{
    S_StartObjSound(actor, sfx_flame);
    A_Fire(actor);
}

void A_Fire (mobj_t* actor)
{
    mobj_t*     dest;

    dest = actor->tracer;
    if (!dest)
        return;

    // don't move it if the vile lost sight
    if (!P_CheckSight (actor->target, dest) )
        return;

    int angf = ANGLE_TO_FINE(dest->angle);

    // MT_FIRE is MF_NOBLOCKMAP, unless a DEH should change it.
    P_UnsetThingPosition (actor);
    actor->x = dest->x + FixedMul( FIXINT(24), finecosine[angf] );
    actor->y = dest->y + FixedMul( FIXINT(24), finesine[angf] );
    actor->z = dest->z;
    P_SetThingPosition (actor);
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget (mobj_t*      actor)
{
    mobj_t*     fog;

    if (!actor->target)
        return;

    A_FaceTarget (actor);

#if 0
    // Original bug
    fog = P_SpawnMobj (actor->target->x,
                       actor->target->x,           // Bp: shoul'nt be y ?
                       actor->target->z, MT_FIRE);
#endif

    // [WDJ] Found by Bp: Fix Vile fog bug, similar to prboom (Killough 12/98).
    fog = P_SpawnMobj (actor->target->x,
                       actor->target->y,
                       actor->target->z, MT_FIRE);

    SET_TARGET_REF( actor->tracer, fog );
    SET_TARGET_REF( fog->target, actor );
    SET_TARGET_REF( fog->tracer, actor->target );

    A_Fire (fog);
}




//
// A_VileAttack
//
void A_VileAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);

    if (!P_CheckSight (actor, actor->target) )
        return;

    S_StartObjSound(actor, sfx_barexp);
    P_DamageMobj (actor->target, actor, actor, 20);
    actor->target->momz = FIXINT(1000) / actor->target->info->mass;

    int angf = ANGLE_TO_FINE(actor->angle);

    mobj_t* fire = actor->tracer;
    if (!fire)
        return;

    // [WDJ] Possible blockmap list corruption, in DEH modified things.
    // MT_FIRE is MF_NOBLOCKMAP, so bug is normally not triggered, unless a DEH should change that.
    // Vanilla Doom: Cannot change thing x,y without calling P_UnsetThingPosition.
    //    Otherwise, unlink in P_UnsetThingPosition could corrupt blockmap lists.
    // MBF, PrBoom: have safe unlink in P_UnsetThingPosition.
    // DoomLegacy: have adopted a safe unlink, like in MBF.
    //    Can now do this safely (SVN 1665).

    // move the fire between the vile and the player
    fire->x = actor->target->x - FixedMul( FIXINT(24), finecosine[angf] );
    fire->y = actor->target->y - FixedMul( FIXINT(24), finesine[angf] );
    P_RadiusAttack (fire, actor, 70 );
}




//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it.
//
#define FATSPREAD       (ANG90/8)

void A_FatRaise (mobj_t* actor)
{
    A_FaceTarget (actor);
    S_StartAttackSound(actor, sfx_manatk);
}


void A_FatAttack1 (mobj_t* actor)
{
    A_FaceTarget (actor);
    // Change direction  to ...
    actor->angle += FATSPREAD;
    P_SpawnMissile (actor, actor->target, MT_FATSHOT);

    mobj_t* mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    if(mo)
    {
        mo->angle += FATSPREAD;
        int angf = ANGLE_TO_FINE(mo->angle);
#ifdef MONSTER_VARY
        mo->momx = FixedMul (mo->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->speed, finesine[angf]);
#else
        mo->momx = FixedMul (mo->info->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->info->speed, finesine[angf]);
#endif
    }
}

void A_FatAttack2 (mobj_t* actor)
{
    A_FaceTarget (actor);

    // Now here choose opposite deviation.
    actor->angle -= FATSPREAD;
    P_SpawnMissile (actor, actor->target, MT_FATSHOT);

    mobj_t* mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    if(mo)
    {
        mo->angle -= FATSPREAD*2;
        int angf = ANGLE_TO_FINE(mo->angle);
#ifdef MONSTER_VARY
        mo->momx = FixedMul (mo->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->speed, finesine[angf]);
#else
        mo->momx = FixedMul (mo->info->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->info->speed, finesine[angf]);
#endif
    }
}

void A_FatAttack3 (mobj_t* actor)
{
    int         angf;

    A_FaceTarget (actor);

    mobj_t* mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    if(mo)
    {
        mo->angle -= FATSPREAD/2;
        angf = ANGLE_TO_FINE(mo->angle);
#ifdef MONSTER_VARY
        mo->momx = FixedMul (mo->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->speed, finesine[angf]);
#else
        mo->momx = FixedMul (mo->info->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->info->speed, finesine[angf]);
#endif
    }
    
    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    if(mo)
    {
        mo->angle += FATSPREAD/2;
        angf = ANGLE_TO_FINE(mo->angle);
#ifdef MONSTER_VARY
        mo->momx = FixedMul (mo->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->speed, finesine[angf]);
#else
        mo->momx = FixedMul (mo->info->speed, finecosine[angf]);
        mo->momy = FixedMul (mo->info->speed, finesine[angf]);
#endif
    }
}


//
// SkullAttack
// Fly at the player like a missile.
//
#define SKULLSPEED              (20*FRACUNIT)

void A_SkullAttack (mobj_t* actor)
{
    mobj_t*             dest;

    if (!actor->target)
        return;
   
    if (actor->target->health <= 0)
    {
       actor->target = NULL;
       return;
    }

    dest = actor->target;
    actor->flags |= MF_SKULLFLY;

    A_FaceTarget (actor);
    S_StartScreamSound(actor, actor->info->attacksound);

    if( cv_predictingmonsters.EV || (actor->eflags & MF_PREDICT) ) //added by AC for predmonsters
    {

                boolean canHit;
                fixed_t	px, py, pz;
                subsector_t * sec;

                int dist = P_AproxDistance (dest->x - actor->x, dest->y - actor->y);
                int est_time = dist/SKULLSPEED;
                int pm_time = P_AproxDistance (dest->x + dest->momx*est_time - actor->x,
                                               dest->y + dest->momy*est_time - actor->y)/SKULLSPEED;

                canHit = 0;
                int t = pm_time + 4;
                do
                {
                        t-=4;
                        if (t < 1)
                                t = 1;
                        px = dest->x + dest->momx*t;
                        py = dest->y + dest->momy*t;
                        pz = dest->z + dest->momz*t;
                        canHit = P_CheckSight2(actor, dest, px, py, pz);
                } while (!canHit && (t > 1));

                sec = R_PointInSubsector(px, py);
                if (!sec)
                        sec = dest->subsector;

                if (pz < sec->sector->floorheight)
                        pz = sec->sector->floorheight;
                else if (pz > sec->sector->ceilingheight)
                        pz = sec->sector->ceilingheight - dest->height;

                angle_t ang = R_PointToAngle2 (actor->x, actor->y, px, py);

                // fuzzy player
                if (dest->flags & MF_SHADOW)
                {
                    int aa = PP_SignedRandom(pr_shadow);
                    ang += ( EN_heretic )? (aa << 21) : (aa << 20);
                }

                actor->angle = ang;
                actor->momx = FixedMul (SKULLSPEED, cosine_ANG(ang));
                actor->momy = FixedMul (SKULLSPEED, sine_ANG(ang));

                actor->momz = (pz+(dest->height>>1) - actor->z) / t;
    }
    else
    {
                angle_t ang = actor->angle;
                actor->momx = FixedMul (SKULLSPEED, cosine_ANG(ang));
                actor->momy = FixedMul (SKULLSPEED, sine_ANG(ang));
                int dist = P_AproxDistance (dest->x - actor->x, dest->y - actor->y);
                dist = dist / SKULLSPEED;

                if (dist < 1)
                        dist = 1;
                actor->momz = (dest->z+(dest->height>>1) - actor->z) / dist;
    }
}


//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void
A_PainShootSkull( mobj_t* actor, angle_t angle )
{
    fixed_t     x, y, z;
    mobj_t*     newmobj;
    int         prestep;


    if( EN_skull_limit )  // Boom comp_pain
    {
    //  --------------- SKULL LIMIT CODE -----------------
//  Original Doom code that limits the number of skulls to 20
    int         count;
    thinker_t*  currentthinker;

    // count total number of skull currently on the level
    count = 0;

    currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        if (   (currentthinker->function == TFI_MobjThinker)
            && ((mobj_t*)currentthinker)->type == MT_SKULL)
            count++;
        currentthinker = currentthinker->next;
    }

    // if there are already 20 skulls on the level,
    // don't spit another one
    if (count > 20)
        goto no_skull;

    }

    // okay, there's place for another one
    prestep =
        FIXINT(4)
        + 3*(actor->info->radius + mobjinfo[MT_SKULL].radius)/2;

    x = actor->x + FixedMul (prestep, cosine_ANG(angle));
    y = actor->y + FixedMul (prestep, sine_ANG(angle));
    z = actor->z + FIXINT(8);

    if( EN_old_pain_spawn )  // Boom comp_skull
    {
       newmobj = P_SpawnMobj (x, y, z, MT_SKULL);
    }
    else
    {
       // Check before spawning if spawn spot is valid, not in a wall,
       // not crossing any lines that monsters could not cross.
       if( P_CheckCrossLine( actor, x, y ) )
           goto no_skull;
       
       newmobj = P_SpawnMobj (x, y, z, MT_SKULL);
       
       // [WDJ] Could not think of better way to check this.
       // So modified from prboom (by phares).
       {
           register sector_t * nmsec = newmobj->subsector->sector;
           // check for above ceiling or below floor
           // skull z may be modified by SpawnMobj, so check newmobj itself
           if( ( (newmobj->z + newmobj->height) > nmsec->ceilingheight )
               || ( newmobj->z < nmsec->floorheight ) )
               goto remove_skull;
       }
    }

    // [WDJ] From PrBoom, MBF, EternityEngine.
    // killough 7/20/98: PEs shoot lost souls with the same friendliness
    newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);
    P_UpdateClassThink(&newmobj->thinker, TH_unknown);

    // Check for movements.
    if (!P_TryMove (newmobj, newmobj->x, newmobj->y, false))
       goto remove_skull;

    if( actor->target && (actor->target->health > 0) )
    {
        SET_TARGET_REF( newmobj->target, actor->target );
    }
    
    A_SkullAttack (newmobj);
    return;

remove_skull:   
    // kill it immediately
#define RMSKULL  2
#if RMSKULL == 0
    // The skull dives to the floor and dies
    P_DamageMobj (newmobj,actor,actor,10000);
#endif
#if RMSKULL == 1      
    // The skull dies less showy, like prboom
    newmobj->health = 0;
    P_KillMobj (newmobj,NULL,actor);  // no death messages
#endif
#if RMSKULL == 2
    // The skull does not appear, like Edge
    P_RemoveMobj (newmobj);
#endif
no_skull:
    return;
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//
void A_PainAttack (mobj_t* actor)
{
    if (!actor->target)
        return;
   
    if (actor->target->health <= 0 )
    {
        SET_TARGET_REF( actor->target, NULL );
        return;
    }

    A_FaceTarget (actor);
    A_PainShootSkull (actor, actor->angle);
}


void A_PainDie (mobj_t* actor)
{
    A_Fall (actor);
    A_PainShootSkull (actor, actor->angle+ANG90);
    A_PainShootSkull (actor, actor->angle+ANG180);
    A_PainShootSkull (actor, actor->angle+ANG270);
}


// Heretic: see A_HScream
// Doom, MBF only
void A_Scream (mobj_t* actor)
{
    int         sound;

    switch (actor->info->deathsound)
    {
      case 0:
        return;

      case sfx_podth1:
      case sfx_podth2:
      case sfx_podth3:
        sound = sfx_podth1 + PP_Random(pr_scream)%3;
        break;

      case sfx_bgdth1:
      case sfx_bgdth2:
        sound = sfx_bgdth1 + PP_Random(pr_scream)%2;
        break;

      default:
        sound = actor->info->deathsound;
        break;
    }

    // Heretic: Make boss death sounds full volume.  (in A_HScream)

    // Check for bosses.
#ifdef MBF21
    // MT_SPIDER, MT_CYBORG have MF3_FULLVOLSOUNDS set
    if( (actor->flags2 & MF2_BOSS)  // Heretic, MBF21
        || (actor->flags3 & MF3_FULLVOLSOUNDS) )  // MBF21
#else
    if (actor->type==MT_SPIDER
        || actor->type == MT_CYBORG)
#endif
    {
        // full volume
        S_StartSound(sound);
    }
    else
    {
        S_StartScreamSound(actor, sound);
    }
}


void A_XScream (mobj_t* actor)
{
    S_StartScreamSound(actor, sfx_slop);
}

void A_Pain (mobj_t* actor)
{
    if (actor->info->painsound)
        S_StartScreamSound(actor, actor->info->painsound);
}


//
//  A dying thing falls to the ground (monster deaths)
//
// Invoked by state during death sequence, after P_KillMobj.
void A_Fall (mobj_t* actor)
{
    // actor is on ground, it can be walked over
    if (!cv_solidcorpse.EV)
        actor->flags &= ~MF_SOLID;  // not solid (vanilla doom)

    if( EV_legacy >= 131 )
    {
        // Before version 131 this is done later in P_KillMobj.
        actor->flags   |= MF_CORPSE|MF_DROPOFF;
        actor->height = actor->info->height>>2;
        actor->radius -= (actor->radius>>4);      //for solid corpses
        // [WDJ] Corpse health must be < 0.
        // Too many health checks all over the place, like BossDeath.
        if( actor->health >= 0 )
            actor->health = -1;
        if( actor->health > -actor->info->spawnhealth )
        {
            // Not gibbed yet.
            // Determine health until gibbed, keep some of the damage.
            actor->health = (actor->health - actor->info->spawnhealth)/2;
        }
    }

    // So change this if corpse objects
    // are meant to be obstacles.
}


//
// A_Explode
//
void A_Explode (mobj_t* actor)
{
    int damage = 128;

#ifdef HEXEN
    boolean can_damage_self = true;  // default
    if( EN_hexen )
    {
        switch(actor->type)
        {
         case MT_HEXEN_MNTRFX2:  // Minotaur floor fire
            damage = 24;
            break;
         case MT_HEXEN_BISHOP:  // Bishop radius death
            damage = 25 + (PP_Random(pr_hexen) & 0x0F);
            break;
         case MT_HEXEN_HAMMER_MISSILE:  // Fighter Hammer
            damage = 128;
            can_damage_self = false;
            break;
         case MT_HEXEN_FSWORD_MISSILE:  // Fighter Runesword
            damage = 64;
            can_damage_self = false;
            break;
         case MT_HEXEN_CIRCLEFLAME:  // Cleric Flame secondary flames
            damage = 20;
            can_damage_self = false;
            break;
         case MT_HEXEN_SORCBALL1:  // Sorcerer balls
         case MT_HEXEN_SORCBALL2:
         case MT_HEXEN_SORCBALL3:
            distance = 255;
            damage = 255;
//	    actor->special_args[0] = 1; // do not bounce   // FIXME
            break;
         case MT_HEXEN_SORCFX1:  // Sorcerer spell 1
            damage = 30;
            break;
         case MT_HEXEN_SORCFX4:  // Sorcerer spell 4
            damage = 20;
            break;
         case MT_HEXEN_TREEDESTRUCTIBLE:
            damage = 10;
            break;
         case MT_HEXEN_DRAGON_FX2:
            damage = 80;
            can_damage_self = false;
            break;
         case MT_HEXEN_MSTAFF_FX:
            damage = 64;
            distance = 192;
            can_damage_self = false;
            break;
         case MT_HEXEN_MSTAFF_FX2:
            damage = 80;
            distance = 192;
            can_damage_self = false;
            break;
         case MT_HEXEN_POISONCLOUD:
            damage = 4;
            distance = 40;
            break;
// ARE THESE REALLY HEXEN  ????
         case MT_HEXEN_ZXMAS_TREE:
         case MT_HEXEN_ZSHRUB2:
            damage = 30;
            distance = 64;
            break;
         default:
            break;
        }
        P_RadiusAttack_MBF21 ( actor, actor->target, damage, distance, can_damage_self );
        if( (actor->type != MT_POISONCLOUD)
            && (actor->z <= (actor->floorz + (distance<<FRACBITS))) )
            P_HitFloor(actor);
        return;
    }
#endif

    switch(actor->type)
    {
    case MT_FIREBOMB: // Time Bombs
        actor->z += FIXINT(32);
        actor->flags &= ~MF_SHADOW;
        break;
    case MT_MNTRFX2: // Minotaur floor fire
        damage = 24;
        break;
    case MT_SOR2FX1: // D'Sparil missile
        damage = 80+(PP_Random(ph_soratkdam)&31);
        break;
    default:
        break;
    }

    P_RadiusAttack ( actor, actor->target, damage );
    P_HitFloor(actor);
}

static
state_t * P_FinalState(statenum_t state)
{
    static char final_state[NUMSTATES_EXT]; //Hurdler: quick temporary hack to fix hacx freeze

    memset(final_state, 0, NUMSTATES_EXT);
    while (states[state].tics!=-1)
    {
        final_state[state] = 1;
        state=states[state].nextstate;
        if (final_state[state])
            return NULL;
    }

    return &states[state];
}

#ifdef ENABLE_UMAPINFO
// [MB] 2023-03-19: Support for UMAPINFO added
// Helper function for A_Bosstype_Death(), code is now used more than once
// Returns false if no player is alive
static
boolean A_Player_Alive(void)
{
    int i;
    for( i = 0 ; i < MAXPLAYERS ; i++ )
    {
        if( playeringame[i] && players[i].health > 0 )
            return true;
    }

    return false;
}

// [MB] 2023-03-19: Support for UMAPINFO added
// Helper function for A_Bosstype_Death(), code is now used more than once
// Parameters are forwarded from A_Bosstype_Death().
// Returns true if all other bosses (of same type as in parameter mo) are dead
static
boolean A_All_Bosses_Dead(mobj_t* mo, int boss_type)
{
    boolean     dead = true;
    thinker_t*  th;
    mobj_t*     mo2;

    for( th = thinkercap.next ; th != &thinkercap ; th = th->next )
    {
        if( th->function != TFI_MobjThinker )
            continue;

        // Fixes MAP07 bug where if last arachnotron is killed while first
        // still in death sequence, then both would trigger this code
        // and the floor would be raised twice (bad).
        mo2 = (mobj_t*)th;
        // Check all boss of the same type
        if ( mo2 != mo
             && mo2->type == boss_type )
        {
            // Check if really dead and finished the death sequence.
            // [WDJ] Corpse has health < 0.
            // If two monsters are killed at the same time, this test may occur
            // while first is corpse and second is not.  But the simple health
            // test may trigger twice because second monster already has
            // health < 0 during the first death test.
            if( mo2->health > 0  // the old test (doom original 1.9)
                || !(mo2->flags & MF_CORPSE)
                || mo2->state != P_FinalState(mo2->info->deathstate) )
            {
                // other boss not dead
                dead = false;
            }
        }
    }

    return dead;
}
#endif

//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
// Triggered by actor state change action, last in death sequence.
// A_Fall usually occurs before this.
// Heretic: see A_HBossDeath() in p_henemy.c
// [WDJ]  Keen death does not have tests for mo->type and thus allows
// Dehacked monsters to trigger Keen death and BossDeath effects.
// Should duplicate that ability in Doom maps.
static
void A_Bosstype_Death (mobj_t* mo, int boss_type)
{
    line_t lineop;  // operation performed when all bosses are dead.

#ifdef ENABLE_UMAPINFO

    // [MB] 2023-03-19: Support for UMAPINFO added
# ifdef DEBUG_UMAPINFO
    if( EN_umi_debug_out )
        GenPrintf(EMSG_debug, "Boss died (Type: %d)\n", boss_type);
# endif
    if( game_umapinfo && game_umapinfo->bossactions )
    {
        bossaction_t* umi_ba = game_umapinfo->bossactions;

        if( ! A_Player_Alive() )
            return;

        if( ! A_All_Bosses_Dead(mo, boss_type) )
            return;

        do
        {
            if( boss_type >= 0 && (unsigned int)boss_type == umi_ba->thing)
            {
# ifdef DEBUG_UMAPINFO
                if( EN_umi_debug_out )
                    GenPrintf(EMSG_debug, "UMAPINFO: Bossaction found (Line: %u, Tag: %u)\n",
                          umi_ba->special, umi_ba->tag);
# endif

                // FIXME: Is this sufficient for the xxxSpecialLine() functions?
                memset(&lineop, 0, sizeof(line_t));
                if (umi_ba->special > (unsigned int)SHRT_MAX)
                    continue;
                lineop.special = (short)umi_ba->special;
                if (umi_ba->tag > (unsigned int)UINT16_MAX)
                    continue;
                lineop.tag = (uint16_t)umi_ba->tag;

                // Try to use the line first, cross it if not successful
                {
                    // Prepare fake player (as modified copy of boss map object)
                    mobj_t fake_player_mo = *mo;

                    fake_player_mo.type   = MT_PLAYER;
                    fake_player_mo.player = &players[0];

#if 0
                    // Does not return false, if there is nothing to use
                    if( ! P_UseSpecialLine(&fake_player_mo, &lineop, 0) )
                    {
# ifdef DEBUG_UMAPINFO
                        if( EN_umi_debug_out )
                            GenPrintf(EMSG_debug, "UMAPINFO: Cross special line\n");
# endif
                        P_CrossSpecialLine(&lineop, 0, &fake_player_mo);
                    }
#else
                    // Workaround (do both unconditionally)
                    (void)P_UseSpecialLine(&fake_player_mo, &lineop, 0);
                    P_CrossSpecialLine(&lineop, 0, &fake_player_mo);
#endif
                }
            }
            umi_ba = umi_ba->next;
        } while( umi_ba );

        return;
    }

    if( game_umapinfo && (game_umapinfo->flags & UMA_bossactions_clear) )
    {
        // Default handling explicitly cleared by UMAPINFO
        // Standard Doom boss-action handling explicitly blocked by UMAPINFO.
        return;
    }

#else
    // Not UMAPINFO
    thinker_t*  th;
    mobj_t*     mo2;
    int         i;
#endif
   
    // [MB] Standard doom handling starts here
    if ( gamemode == doom2_commercial)
    {
        // Doom2 MAP07: When last Mancubus is dead,
        //   execute lowerFloortoLowest(sectors tagged 666).
        // Doom2 MAP07: When last Arachnotron is dead,
        //   execute raisetoTexture(sectors tagged 667).
        // Doom2 MAP32: When last Keen is dead,
        //   execute doorOpen(doors tagged 666).
        //   This is not limited to MAP32 in any implementation.
        //   A_KeenDie calls this function, unlike MBF or other implementations.
#ifdef MBF21
        // MT_FATSO (Macubus) has MF3_MAP07_BOSS1 flag set
        // MT_BABY (Arachnotron) has MF3_MAP07_BOSS2 flag set
        if( ! (
          ((gamemap == 7) && (mo->flags3 & (MF3_MAP07_BOSS1 | MF3_MAP07_BOSS2)))
          || (boss_type == MT_KEEN) ) )  // A_KeenDie calls here.
                goto no_action;
#else
          if((boss_type != MT_FATSO)
            && (boss_type != MT_BABY)
            && (boss_type != MT_KEEN))  // A_KeenDie calls here.
                goto no_action;
#endif
    }
    // [WDJ] Untested
    // This could be done with compatibility switch (comp_666), as in prboom.
    else if( (gamemode == doom_shareware || gamemode == doom_registered)
             && gameepisode < 4 )
    {
        // [WDJ] Revert to behavior before UltimateDoom,
        // to fix "Doomsday of UAC" bug.
        if (gamemap != 8)
            goto no_action;
        // Allow all boss types in each episode, (for PWAD)
        // E1: all, such as Baron and Cyberdemon
        // E2,E3,E4: all except Baron
        // [WDJ] Logic from prboom
        if (gameepisode != 1)
        {
#ifdef MBF21
            // MT_BRUISER (Baron) has MF3_E1M8_BOSS flag set
            if( mo->flags3 & MF3_E1M8_BOSS )
                goto no_action;
#else
            if (boss_type == MT_BRUISER)
                goto no_action;
#endif
        }
    }
    else
    {
        switch(gameepisode)
        {
          case 1:
            // Doom E1M8: When all Baron are dead,
            //   execute lowerFloortoLowest(sectors tagged 666).
            if (gamemap != 8)
                goto no_action;

            // This test was added in UltimateDoom.
            // Some PWAD from before then, such as "Doomsday of UAC" which
            // requires death of last Baron and last Cyberdemon, will fail.
#ifdef MBF21
            // MT_BRUISER (Baron) has MF3_E1M8_BOSS flag set
            if( !(mo->flags3 & MF3_E1M8_BOSS) )
                goto no_action;
#else
            if (boss_type != MT_BRUISER)
                goto no_action;
#endif
            break;

          case 2:
            // Doom E2M8: When last Cyberdemon is dead, level ends.
            if (gamemap != 8)
                goto no_action;

#ifdef MBF21
            // MT_CYBORG has MF3_E2M8_BOSS flag set
            if( !(mo->flags3 & MF3_E2M8_BOSS) )
                goto no_action;
#else
            if (boss_type != MT_CYBORG)
                goto no_action;
#endif
            break;

          case 3:
            // Doom E3M8: When last Spidermastermind is dead, level ends.
            if (gamemap != 8)
                goto no_action;

#ifdef MBF21
            // MT_SPIDER has MF3_E3M8_BOSS flag set
            if( !(mo->flags3 & MF3_E3M8_BOSS) )
                goto no_action;
#else
            if (boss_type != MT_SPIDER)
                goto no_action;
#endif
            break;

          case 4:
            switch(gamemap)
            {
              case 6:
                // Doom E4M6: When last Cyberdemon is dead,
                //   execute blazeOpen(doors tagged 666).
#ifdef MBF21
                // MT_CYBORG has MF3_E4M6_BOSS flag set
                if( !(mo->flags3 & MF3_E4M6_BOSS) )
                    goto no_action;
#else
                if (boss_type != MT_CYBORG)
                    goto no_action;
#endif
                break;

              case 8:
                // Doom E4M8: When last Spidermastermind is dead,
                //   execute lowerFloortoLowest(sectors tagged 666).
#ifdef MBF21
                // MT_SPIDER has MF3_E4M8_BOSS flag set
                if( !(mo->flags3 & MF3_E4M8_BOSS) )
                    goto no_action;
#else
                if (boss_type != MT_SPIDER)
                    goto no_action;
#endif
                break;

              default:
                goto no_action;
            }
            break;

          default:
            if (gamemap != 8)
                goto no_action;
            break;
        }

    }

    // make sure there is a player alive for victory
#ifdef ENABLE_UMAPINFO
    if( ! A_Player_Alive() )
        return; // no one left alive, so do not end game
#else
    for (i=0 ; i<MAXPLAYERS ; i++)
        if (playeringame[i] && players[i].health > 0)
            break;

    if (i==MAXPLAYERS)
        return; // no one left alive, so do not end game
#endif

    // scan the remaining thinkers to see
    // if all bosses are dead
#ifdef ENABLE_UMAPINFO
    if( ! A_All_Bosses_Dead(mo, boss_type) )
        goto no_action;
#else
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        if (th->function != TFI_MobjThinker)
            continue;

        // Fixes MAP07 bug where if last arachnotron is killed while first
        // still in death sequence, then both would trigger this code
        // and the floor would be raised twice (bad).
        mo2 = (mobj_t*)th;
        // Check all boss of the same type
        if ( mo2 != mo
            && mo2->type == boss_type )
        {
            // Check if really dead and finished the death sequence.
            // [WDJ] Corpse has health < 0.
            // If two monsters are killed at the same time, this test may occur
            // while first is corpse and second is not.  But the simple health
            // test may trigger twice because second monster already has
            // health < 0 during the first death test.
            if( mo2->health > 0  // the old test (doom original 1.9)
                || !(mo2->flags & MF_CORPSE)
                || mo2->state != P_FinalState(mo2->info->deathstate) )
            {
                // other boss not dead
                goto no_action;
            }
        }
    }
#endif

    // victory!
    if ( gamemode == doom2_commercial)
    {
        if (boss_type == MT_FATSO)
        {
            if(gamemap == 7)
            {
                // Doom2 MAP07: When last Mancubus is dead, execute lowerFloortoLowest.
                //   execute lowerFloortoLowest(sectors tagged 666).
                lineop.tag = 666;
                EV_DoFloor( &lineop, FT_lowerFloorToLowest);
            }
            goto done;
        }
        else if (boss_type == MT_BABY)
        {
            if(gamemap == 7)
            {
                // Doom2 MAP07: When last Arachnotron is dead,
                //   execute raisetoTexture(sectors tagged 667).
                lineop.tag = 667;
                EV_DoFloor( &lineop, FT_raiseToTexture);
            }
            goto done;
        }
        else if(boss_type == MT_KEEN)
        {
            // Doom2 MAP32: When last Keen is dead,
            //   execute doorOpen(doors tagged 666).
            lineop.tag = 666;
            EV_DoDoor( &lineop, VD_dooropen, VDOORSPEED);
            goto done;
        }
    }
    else
    {
        switch(gameepisode)
        {
          case 1:
            // Doom E1M8: When all Baron are dead, execute lowerFloortoLowest
            //   on all sectors tagged 666.
            lineop.tag = 666;
            EV_DoFloor( &lineop, FT_lowerFloorToLowest);
            goto done;

          case 4:
            switch(gamemap)
            {
              case 6:
                // Doom E4M6: When last Cyberdemon is dead, execute blazeOpen.
                //   on all doors tagged 666.
                lineop.tag = 666;
                EV_DoDoor( &lineop, VD_blazeOpen, 4*VDOORSPEED);
                goto done;

              case 8:
                // Doom E4M8: When last Spidermastermind is dead, execute lowerFloortoLowest.
                //   on all sectors tagged 666.
                lineop.tag = 666;
                EV_DoFloor( &lineop, FT_lowerFloorToLowest);
                goto done;
            }
        }
    }
    if( cv_allowexitlevel.EV )
        G_ExitLevel ();
done:
    return;

no_action:
    return;
}

// Info table call, as in Heretic.
void A_BossDeath (mobj_t* mo)
{
    A_Bosstype_Death( mo, mo->type );
}

//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie (mobj_t* mo)
{
    A_Fall (mo);

    // Doom2 MAP32: When last Keen is dead,
    //   execute doorOpen(doors tagged 666).
    // Some Dehacked mods use Keen death to trigger 666 tagged door.
    // Cannot use mo->type when dehacked may have only changed death frame.
    A_Bosstype_Death(mo, MT_KEEN);
}

void A_Hoof (mobj_t* mo)
{
    S_StartObjSound(mo, sfx_hoof);
    A_Chase (mo);
}

void A_Metal (mobj_t* mo)
{
    S_StartObjSound(mo, sfx_metal);
    A_Chase (mo);
}

void A_BabyMetal (mobj_t* mo)
{
    S_StartObjSound(mo, sfx_bspwlk);
    A_Chase (mo);
}

void A_OpenShotgun2 ( player_t*     player,
                      pspdef_t*     psp )
{
    S_StartAttackSound(player->mo, sfx_dbopn);
}

void A_LoadShotgun2 ( player_t*     player,
                      pspdef_t*     psp )
{
    S_StartAttackSound(player->mo, sfx_dbload);
}

void A_ReFire ( player_t*     player,
                pspdef_t*     psp );

void A_CloseShotgun2 ( player_t*     player,
                       pspdef_t*     psp )
{
    S_StartAttackSound(player->mo, sfx_dbcls);
    A_ReFire(player,psp);
}

// [WDJ] Remove hard limits. Similar to prboom, killough 3/26/98.
// Dynamic allocation.
static mobj_t* * braintargets; // dynamic array of ptrs
static int     braintargets_max = 0; // allocated
static int     numbraintargets;
static int     braintargeton;

// Original was 32 max.
// Non-fatal - limited for useability, otherwise can have too many thinkers.
// This affects compatibility in netplay.
#ifdef MACHINE_MHZ
#define MAX_BRAINTARGETS  (32*MACHINE_MHZ/100)
#endif

// return 1 on success
boolean  expand_braintargets( void )
{
    int needed = braintargets_max += 32;
#ifdef MAX_BRAINTARGETS
    if( needed > MAX_BRAINTARGETS )
    {
        I_SoftError( "Expand braintargets exceeds MAX_BRAINTARGETS %d.\n", MAX_BRAINTARGETS );
        return 0;
    }
#endif
    // realloc to new size, copying contents
    mobj_t ** new_braintargets =
    realloc( braintargets, sizeof(mobj_t*) * needed );
    if( new_braintargets )
    {
        braintargets = new_braintargets;
        braintargets_max = needed;
    }
    else
    {
        // non-fatal protection, allow savegame or continue play
        // realloc fail does not disturb existing allocation
        numbraintargets = braintargets_max;
        I_SoftError( "Expand braintargets realloc failed at $d.\n", needed );
        return 0;  // fail to expand
    }
    return 1;
}

void P_Init_BrainTarget()
{
    thinker_t*  thinker;

    // find all the target spots
    numbraintargets = 0;
    braintargeton = 0;

    thinker = thinkercap.next;
    for (thinker = thinkercap.next ;
         thinker != &thinkercap ;
         thinker = thinker->next)
    {
        if (thinker->function != TFI_MobjThinker)
            continue;   // not a mobj

        register mobj_t* m = (mobj_t*)thinker;

        if (m->type == MT_BOSSTARGET )
        {
            if( numbraintargets >= braintargets_max )
            {
                if( ! expand_braintargets() )  break;
            }
            braintargets[numbraintargets] = m;
            numbraintargets++;
        }
    }
}


void A_BrainAwake (mobj_t* mo)
{
    S_StartSound(sfx_bossit);
}


void A_BrainPain (mobj_t*       mo)
{
    S_StartSound(sfx_bospn);
}


void A_BrainScream (mobj_t*     mo)
{
    int         x;
    int         y;
    int         z;
    mobj_t*     th;

    for (x=mo->x - FIXINT(196) ; x< mo->x + FIXINT(320) ; x+= FIXINT(8) )
    {
        y = mo->y - FIXINT(320);
        z = 128 + PP_Random(pr_brainscream) * FIXINT(2);
        th = P_SpawnMobj (x,y,z, MT_ROCKET);
        th->momz = PP_Random(pr_brainscream)*512;

        P_SetMobjState (th, S_BRAINEXPLODE1);

        th->tics -= PP_Random(pr_brainscream)&7;
        if (th->tics < 1)
            th->tics = 1;
    }

    S_StartSound(sfx_bosdth);
}



void A_BrainExplode (mobj_t* mo)
{
    int         x;
    int         y;
    int         z;
    mobj_t*     th;

    x = (PP_SignedRandom(pr_brainexp)<<11)+mo->x;
    y = mo->y;
    z = 128 + PP_Random(pr_brainexp) * FIXINT(2);
    th = P_SpawnMobj (x,y,z, MT_ROCKET);
    th->momz = PP_Random(pr_brainexp)*512;

    P_SetMobjState (th, S_BRAINEXPLODE1);

    th->tics -= PP_Random(pr_brainexp)&7;
    if (th->tics < 1)
        th->tics = 1;
}


void A_BrainDie (mobj_t*        mo)
{
    if( cv_allowexitlevel.EV )
       G_ExitLevel ();
}

void A_BrainSpit (mobj_t*       mo)
{
    mobj_t*     targ;
    mobj_t*     newmobj;

    static int  easy = 0;

    easy ^= 1;
    if (gameskill <= sk_easy && (!easy))
        return;

    if( numbraintargets>0 ) 
    {
        // shoot a cube at current target
        targ = braintargets[braintargeton];
        braintargeton = (braintargeton+1)%numbraintargets;
        
        // spawn brain missile
        newmobj = P_SpawnMissile (mo, targ, MT_SPAWNSHOT);
        if(newmobj)
        {
            SET_TARGET_REF( newmobj->target, targ );
            newmobj->reactiontime =
                ((targ->y - mo->y)/newmobj->momy) / newmobj->state->tics;
            
            // [WDJ] MBF: From PrBoom, MBF, EternityEngine.
            // killough 7/18/98: brain friendliness is transferred
            newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
            P_UpdateClassThink(&newmobj->thinker, TH_unknown);
        }

        S_StartSound(sfx_bospit);
    }
}



void A_SpawnFly (mobj_t* mo);

// travelling cube sound
void A_SpawnSound (mobj_t* mo)
{
    S_StartObjSound(mo,sfx_boscub);
    A_SpawnFly(mo);
}

void A_SpawnFly (mobj_t* mo)
{
    mobj_t*     newmobj;
    mobj_t*     fog;
    mobj_t*     targ;
    int         r;
    mobjtype_t  type;

    if (--mo->reactiontime)
        return; // still flying

    targ = mo->target;
    if( targ == NULL )
    {
        // Happens if save game with cube flying.
        // targ should be the previous braintarget.
        int bt = ((braintargeton == 0)? numbraintargets : braintargeton) - 1;
        targ = braintargets[bt];
    }

    // First spawn teleport fog.
    fog = P_SpawnMobj (targ->x, targ->y, targ->z, MT_SPAWNFIRE);
    S_StartObjSound(fog, sfx_telept);

    // Randomly select monster to spawn.
    r = PP_Random(pr_spawnfly);

    // Probability distribution (kind of :),
    // decreasing likelihood.
    if ( r<50 )
        type = MT_TROOP;
    else if (r<90)
        type = MT_SERGEANT;
    else if (r<120)
        type = MT_SHADOWS;
    else if (r<130)
        type = MT_PAIN;
    else if (r<160)
        type = MT_HEAD;
    else if (r<162)
        type = MT_VILE;
    else if (r<172)
        type = MT_UNDEAD;
    else if (r<192)
        type = MT_BABY;
    else if (r<222)
        type = MT_FATSO;
    else if (r<246)
        type = MT_KNIGHT;
    else
        type = MT_BRUISER;

    newmobj     = P_SpawnMobj (targ->x, targ->y, targ->z, type);

    // MBF: killough 7/18/98: brain friendliness is transferred
    newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
    P_UpdateClassThink(&newmobj->thinker, TH_unknown);

    if( P_LookForTargets(newmobj, true) )
        P_SetMobjState (newmobj, newmobj->info->seestate);
    // cube monsters have no mapthing (spawnpoint=NULL), do not respawn

    // telefrag anything in this spot
    P_TeleportMove (newmobj, newmobj->x, newmobj->y, true);

    // remove self (i.e., cube).
    P_RemoveMobj (mo);
}



void A_PlayerScream (mobj_t* mo)
{
    // Default death sound.
    int         sound = sfx_pldeth;

    if ( (gamemode == doom2_commercial)
        &&      (mo->health < -50))
    {
        // IF THE PLAYER DIES
        // LESS THAN -50% WITHOUT GIBBING
        sound = sfx_pdiehi;
    }
    S_StartScreamSound(mo, sound);
}


// Exl: More Toxness :)
// Running scripts from states (both mobj and weapon)
void A_StartFS(mobj_t* actor)
{
   // load script number from misc1
   int misc1 = actor->tics;
   actor->tics = 0;
   T_RunScript(misc1, actor);
}

void A_StartWeaponFS(player_t * player, pspdef_t * psp)
{
   int misc1;

   // check all pointers for validacy
   if(player && psp && psp->state)
   {
                misc1 = psp->tics;
                psp->tics = 0;
                T_RunScript(misc1, player->mo);
   }
}


// [WDJ] From MBF, PrBoom, EternityEngine.
// cph - MBF-added codepointer functions.

// killough 11/98: kill an object
void A_Die_MBF(mobj_t*  actor)
{
    P_DamageMobj(actor, NULL, NULL, actor->health);
}

//
// A_Detonate
// killough 8/9/98: same as A_Explode, except that the damage is variable
//
void A_Detonate_MBF(mobj_t*  mo)
{
    P_RadiusAttack(mo, mo->target, mo->info->damage);
}

//
// killough 9/98: a mushroom explosion effect, sorta :)
// Original idea: Linguica
//

// As in Woof
#define MUSHROOM_PARM   1

void A_Mushroom_MBF(mobj_t*  actor)
{
    int i, j, n = actor->info->damage;

#ifdef MUSHROOM_PARM
    state_ext_t * sep = P_state_ext( actor->state );
    // Mushroom parameters are part of code pointer's state
    fixed_t misc1 = sep->parm1 ? sep->parm1 : FRACUNIT*4;
    fixed_t misc2 = sep->parm2 ? sep->parm2 : FRACUNIT/2;
//    misc2 &= 0xFFFF; // ensure that is < 1, and cloud will slow
#endif
    
    A_Explode(actor);  // First make normal explosion

    // Now launch mushroom cloud
    for (i = -n; i <= n; i += 8)
    {
        for (j = -n; j <= n; j += 8)
        {
            mobj_t target = *actor;
            // Aim in many directions from source, up fairly high.
            target.x += i << FRACBITS;
            target.y += j << FRACBITS;
#ifdef MUSHROOM_PARM
            target.z += P_AproxDistance(i,j) * misc1;           // Aim fairly high
#else
            target.z += P_AproxDistance(i,j) << (FRACBITS+2);
#endif
            // Launch fireball
            mobj_t * mo = P_SpawnMissile(actor, &target, MT_FATSHOT);
#ifdef MUSHROOM_PARM
            mo->momx = FixedMul(mo->momx, misc2);
            mo->momy = FixedMul(mo->momy, misc2);               // Slow down a bit
            mo->momz = FixedMul(mo->momz, misc2);
#else
            mo->momx >>= 1;
            mo->momy >>= 1;  // Slow it down a bit
            mo->momz >>= 1;
#endif
            mo->flags &= ~MF_NOGRAVITY;  // Make debris fall under gravity
        }
    }
}

//
// killough 11/98
//
// The following were inspired by Len Pitre
// A small set of highly-sought-after code pointers

// MBF
void A_Spawn_MBF(mobj_t* mo)
{
    // parm1: thing id
    state_ext_t * sep = P_state_ext( mo->state );
    if( sep->parm1 ) // misc1
    {
	// parm1: thing id
	// parm2: height above mobj current position
//        mobj_t* newmobj = 
        P_SpawnMobj(mo->x, mo->y, (sep->parm2 << FRACBITS) + mo->z, sep->parm1 - 1);
// CPhipps - no friendlyness (yet)
//      newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
    }
}

// MBF
void A_Turn_MBF(mobj_t* mo)
{
    // parm1: angle degrees
    state_ext_t * sep = P_state_ext( mo->state );
    mo->angle += (uint32_t)((((uint64_t) sep->parm1) << 32) / 360);
}

void A_Face_MBF(mobj_t* mo)
{
    // parm1: angle degrees
    state_ext_t * sep = P_state_ext( mo->state );
    mo->angle = (uint32_t)((((uint64_t) sep->parm1) << 32) / 360);
}

// MBF
// Melee attack
void A_Scratch_MBF(mobj_t* mo)
{
    if( ! mo->target )
        return;

    // [WDJ] Unraveled from horrible one-liner.
    
    A_FaceTarget(mo);       
    if( ! P_CheckMeleeRange(mo) )
        return;

    state_ext_t * sep = P_state_ext( mo->state );

    // parm2: sound
    sfxid_t sfx2 = sep->parm2;  // misc2
    if( sfx2 )
    {
       S_StartObjSound(mo, sfx2);
    }

    // parm1: damage
    P_DamageMobj(mo->target, mo, mo, sep->parm1); // misc1
}

// MBF
void A_PlaySound_MBF(mobj_t* mo)
{
    state_ext_t * sep = P_state_ext( mo->state );
    if( sep->parm2 )  // misc2: global sound enable
        S_StartSound( sep->parm1 );  // misc1: sound
    else
        S_StartObjSound( mo, sep->parm1 );  // misc1: sound
}

// MBF
void A_RandomJump_MBF(mobj_t* mo)
{
    state_ext_t * sep = P_state_ext( mo->state );
    if( sep == &empty_state_ext )
        return;  // no parm

    // As in EternityEngine, test first, then use Random Number.
    statenum_t si = DEH_frame_to_state( sep->parm1 );  // misc1: frame
    if( si == S_NULL )  return;

    if( PP_Random(pr_randomjump) < sep->parm2 )  // misc2: jump percentage 0..255
        P_SetMobjState(mo, si);
}

// MBF
//
// This allows linedef effects to be activated inside deh frames.
//
void A_LineEffect_MBF(mobj_t* mo)
{
    // [WDJ] From PrBoom, EternityEngine.
    // haleyjd 05/02/04: bug fix:
    // This line can end up being referred to long after this function returns,
    // thus it must be made static or memory corruption is possible.
    static line_t  fake_line;

    player_t fake_player;
    player_t * oldplayer;
    state_ext_t * sep = P_state_ext( mo->state );

#if 0
    // From EternityEngine
    if( mo->eflags & MIF_LINEDONE )  return; // Unless already used up
#endif

    fake_line = *lines;

    fake_line.special = (short)sep->parm1;  // misc1: linedef type
    if( !fake_line.special )
        return;

    fake_line.tag = (short)sep->parm2;  // misc2: sector tag

    // Substitute a fake player to absorb effects.
    // Do this after tests that return.  Do not leave fake player in mo.
    fake_player.health = 100;  // make em alive
    oldplayer = mo->player;
    mo->player = &fake_player;  // fake player

    // Invoke the fake line, first by invoke, then by crossing.
    if( !P_UseSpecialLine(mo, &fake_line, 0) )
        P_CrossSpecialLine(&fake_line, 0, mo);

    mo->player = oldplayer;  // restore player

#if 0
    // From EternityEngine
    // If line special cleared, no more for this thing.
    if( !fake_line.special )
        mo->eflags |= MIF_LINEDONE;
#else
    // As in PrBoom.
    // If line special cleared, no more for this state line type.
    // Only get here when sep->parm1 != 0, so it is not the dummy.
    // fake_line.special was copy of sep->parm1, so update it.
    sep->parm1 = fake_line.special;  // misc1, feedback
#endif
}


#ifdef MBF21
// MBF21 functions
signed_angle_t MBF21_P_Random_hitscan_angle( fixed_t spread );
fixed_t  MBF21_P_Random_hitscan_slope( fixed_t spread );

// These comments are from DSDA-Doom
// Basically just A_Spawn with better behavior and more args.
//   args[0]: Type of actor to spawn
//   args[1]: Angle (degrees, in fixed point), relative to calling actor's angle
//   args[2]: X spawn offset (fixed point), relative to calling actor
//   args[3]: Y spawn offset (fixed point), relative to calling actor
//   args[4]: Z spawn offset (fixed point), relative to calling actor
//   args[5]: X velocity (fixed point)
//   args[6]: Y velocity (fixed point)
//   args[7]: Z velocity (fixed point)
void A_SpawnObject_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action || !actor->state )
        return;

    // No damage if default to 0.
    state_ext_t * sep = P_state_ext( actor->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    int p_type = sep->parm_args[0] - 1;  // parm1: object ID, 1..
    if( p_type < 0 )
        return;

    // parm2: angle degrees
    signed_angle_t angle = (angle_t)(((int64_t)sep->parm_args[1] << 16) / 360);
    // position as fixed_t, relative to actor
    fixed_t p_ofs_x = sep->parm_args[2];
    fixed_t p_ofs_y = sep->parm_args[3];
    fixed_t p_ofs_z = sep->parm_args[4];
    // velocity (mom) as fixed_t
    fixed_t p_vel_x = sep->parm_args[5];
    fixed_t p_vel_y = sep->parm_args[6];
    fixed_t p_vel_z = sep->parm_args[7];

    // calculate position offsets
    angle_t an = actor->angle + angle;
    fixed_t cosan = cosine_ang(an);
    fixed_t sinan = sine_ang(an);
    int dx = FixedMul( p_ofs_x, cosan ) - FixedMul( p_ofs_y, sinan );
    int dy = FixedMul( p_ofs_x, sinan ) + FixedMul( p_ofs_y, cosan );

    // spawn the object
    mobj_t * mo = P_SpawnMobj( actor->x + dx, actor->y + dy, actor->z + p_ofs_z, p_type );
    if (!mo)
      return;

    // set velocity
    mo->angle = an;
    mo->momx = FixedMul( p_vel_x, cosan ) - FixedMul( p_vel_y, sinan );
    mo->momy = FixedMul( p_vel_x, sinan ) + FixedMul( p_vel_y, cosan );
    mo->momz = p_vel_z;

    // For a missile or bouncer, set target,tracer
    if( mo->info->flags & (MF_MISSILE | MF_BOUNCES) )
    {
        // normal for missile
        mobj_t * target = actor;
        mobj_t * tracer = actor->target;

        // if actor is also a missile, copy its target,tracer
        if( actor->info->flags & (MF_MISSILE | MF_BOUNCES) )
        {
            target = actor->target;
            tracer = actor->tracer;
        }

//	P_SetTarget( &mo->target, target );
        SET_TARGET_REF( mo->target, target );  // Remember previous target
//	P_SetTarget( &mo->tracer, tracer );
        SET_TARGET_REF( mo->tracer, tracer );
    }
}

// These comments are from DSDA-Doom
// A parameterized monster projectile attack.
//   args[0]: Type of actor to spawn
//   args[1]: Angle (degrees, in fixed point), relative to calling actor's angle
//   args[2]: Pitch (degrees, in fixed point), relative to calling actor's pitch; approximated
//   args[3]: X/Y spawn offset, relative to calling actor's angle
//   args[4]: Z spawn offset, relative to actor's default projectile fire height
void A_MonsterProjectile_MBF21( mobj_t * actor )
{
    // DSDA forgot to protect actor->state.
    if( !EN_mbf21_action || !actor->state )
        return;

    if( ! actor->target )
      return;

    // Args Required.
    // Does spawn of type args[0], 0=player.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;  // no args

    int p_type = sep->parm_args[0] - 1;
    if( p_type < 0 )
      return;

    // angle degrees
    signed_angle_t  p_angle = (angle_t)(((int64_t)sep->parm_args[1] << FRACBITS) / 360);
    int p_pitch = sep->parm_args[2];
    int p_spawn_ofs_xy = sep->parm_args[3];
    int p_spawn_ofs_z = sep->parm_args[4];
  
    A_FaceTarget(actor);
    mobj_t * mo = P_SpawnMissile( actor, actor->target, p_type );
    if( ! mo )
      return;

    // adjust angle
    mo->angle += p_angle;
#ifdef MONSTER_VARY
    fixed_t speed = mo->speed;
#else
    fixed_t speed = mo->info->speed;
#endif
    mo->momx = FixedMul( speed, cosine_ang(mo->angle) );
    mo->momy = FixedMul( speed, sine_ang(mo->angle) );

    // adjust pitch
    // approximated using Doom finetangent table, same as in autoaim
    mo->momz += FixedMul( speed, MBF21_fixed_deg_to_slope(p_pitch) );

    // adjust position to not be exactly on monster
    angle_t pan = actor->angle - ANG90;
    mo->x += FixedMul( p_spawn_ofs_xy, cosine_ang( pan ) );
    mo->y += FixedMul( p_spawn_ofs_xy, sine_ang( pan ) );
    mo->z += p_spawn_ofs_z;

    // set the tracer, so it can be used to fire seeker missiles at will.
//    P_SetTarget( &mo->tracer, actor->target );
    SET_TARGET_REF( mo->tracer, actor->target );
}

// These comments are from DSDA-Doom
// A parameterized monster bullet attack.
//   args[0]: Horizontal spread (degrees, in fixed point)
//   args[1]: Vertical spread (degrees, in fixed point)
//   args[2]: Number of bullets to fire; if not set, defaults to 1
//   args[3]: Base damage of attack (e.g. for 3d5, customize the 3); if not set, defaults to 3
//   args[4]: Attack damage modulus (e.g. for 3d5, customize the 5); if not set, defaults to 5
void A_MonsterBulletAttack_MBF21( mobj_t * actor )
{
    // DSDA forgot to protect actor->state
    if( !EN_mbf21_action || !actor->state )
        return;

    if( ! actor->target )
      return;

    // No damage from default args.
    state_ext_t * sep = P_state_ext( actor->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    int hspread    = sep->parm_args[0];
    int vspread    = sep->parm_args[1];
    int numbullets = sep->parm_args[2];
    int damagebase = sep->parm_args[3];
    int damagemod  = sep->parm_args[4];

    // [WDJ] Protect against wad errors    
    // Bad Args protection, not in DSDA.
    if( damagebase <= 0 )  damagebase = 1;
    if( damagemod <= 0 )   damagemod = 1;

    A_FaceTarget(actor);
    S_StartMobjSound(actor, actor->info->attacksound);

    int aimslope = P_AimLineAttack( actor, actor->angle, MISSILERANGE, 0 );

    int i;
    for (i = 0; i < numbullets; i++)
    {
        int damage = (PP_Random(pr_mbf21) % damagemod + 1) * damagebase;
        signed_angle_t angle = (signed_angle_t)actor->angle + MBF21_P_Random_hitscan_angle( hspread );
        fixed_t slope = aimslope + MBF21_P_Random_hitscan_slope( vspread );
        P_LineAttack( actor, angle, MISSILERANGE, slope, damage );
    }
}

// These comments are from DSDA-Doom
// A parameterized monster melee attack.
//   args[0]: Base damage of attack (e.g. for 3d8, customize the 3); if not set, defaults to 3
//   args[1]: Attack damage modulus (e.g. for 3d8, customize the 8); if not set, defaults to 8
//   args[2]: Sound to play if attack hits
//   args[3]: Range (fixed point); if not set, defaults to monster's melee range
void A_MonsterMeleeAttack_MBF21( mobj_t * actor )
{
    // DSDA forgot to protect actor->state
    if( !EN_mbf21_action || !actor->state )
        return;

    if( ! actor->target )
      return;

    // Has defaults.
    // No damage from default 0.
    state_ext_t * sep = P_state_ext( actor->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    int damagebase = sep->parm_args[0];
    int damagemod  = sep->parm_args[1];
    int hitsound   = sep->parm_args[2];
    int range      = sep->parm_args[3];

    // [WDJ] Protect against wad errors    
    // Bad Args protection, not in DSDA.
    if( damagebase <= 0 )  damagebase = 1;
    if( damagemod <= 0 )   damagemod = 1;
    
    // Specified range with radius applied.
    if (range == 0)
      range = actor->info->melee_range;
    range += actor->target->info->radius - FIXINT(20);

    A_FaceTarget(actor);
    if( ! P_CheckMeleeRange_orig( actor, range ) )  // orig, with range param
      return;

    if( sep != &empty_state_ext )  // [WDJ] Protect against default making sound.
      S_StartMobjSound(actor, hitsound);

    int damage = (PP_Random(pr_mbf21) % damagemod + 1) * damagebase;
    P_DamageMobj( actor->target, actor, actor, damage );
}

// These comments are from DSDA-Doom
// A parameterized version of A_Explode. Friggin' finally. :P
//   args[0]: Damage (int)
//   args[1]: Radius (also int; no real need for fractional precision here)
void A_RadiusDamage_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action || !actor->state )
        return;

    // No damage from default 0.
    state_ext_t * sep = P_state_ext( actor->state );
//    if( sep == &empty_state_ext )
//        return;

    // damage, distance, can_damage_source
    P_RadiusAttack_MBF21( actor, actor->target, sep->parm_args[0], sep->parm_args[1], true );
}

// These comments are from DSDA-Doom
// Alerts nearby monsters (via sound) to the calling actor's target's presence.
void A_NoiseAlert_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action )
      return;

    if( ! actor->target )
        return;

    P_NoiseAlert( actor->target, actor );
}

// These comments are from DSDA-Doom
// A parameterized version of A_VileChase.
//   args[0]: State to jump to on the calling actor when resurrecting a corpse
//   args[1]: Sound to play when resurrecting a corpse
void A_HealChase_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    // Args Required.
    // Args[0] has new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;  // no args

    int state = sep->parm_args[0];
    int sound = sep->parm_args[1];

    // Like A_VileChase, but for any actor.
    if( !P_HealCorpse( actor, actor->info, state, sound ) )
      A_Chase(actor);
}

// These comments are from DSDA-Doom
// A parameterized seeker missile function.
//   args[0]: direct-homing threshold angle (degrees, in fixed point)
//   args[1]: maximum turn angle (degrees, in fixed point)
void A_SeekTracer_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    // No damage from default 0.
    state_ext_t * sep = P_state_ext( actor->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    // This uses FixedToAngle from EternityEngine
    angle_t threshold = FixedToAngle_EE(sep->parm_args[0]);
    angle_t maxturnangle = FixedToAngle_EE(sep->parm_args[1]);

    P_SeekerMissile_MBF21( actor, &actor->tracer, threshold, maxturnangle, true );
}

// These comments are from DSDA-Doom
// Search for a valid tracer (seek target), if the calling actor doesn't already have one.
//   args[0]: field-of-view to search in (degrees, in fixed point); if zero, will search in all directions
//   args[1]: distance to search (map blocks, i.e. 128 units)
void A_FindTracer_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    // No damage from default 0.
    state_ext_t * sep = P_state_ext( actor->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    // This uses FixedToAngle from EternityEngine
    angle_t fov = FixedToAngle_EE(sep->parm_args[0]);
    int dist = sep->parm_args[1];

//    P_SetTarget(&actor->tracer, P_RoughTargetSearch(actor, fov, dist))  // FIXME
    SET_TARGET_REF( actor->tracer, P_RoughTargetSearch(actor, fov, dist) )  // FIXME
}

// These comments are from DSDA-Doom
// Clear current tracer (seek target).
void A_ClearTracer_MBF21( mobj_t * actor )
{
    if( !EN_mbf21_action )
      return;

    if( !actor )
      return;

//    P_SetTarget(&actor->tracer, NULL);
    SET_TARGET_REF( actor->tracer, NULL );
}

// These comments are from DSDA-Doom
// Jumps to a state if caller's health is below the specified threshold.
//   args[0]: State to jump to
//   args[1]: Health threshold
void A_JumpIfHealthBelow_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    // Args Required.
    // Args[0] is a new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;  // no args

    int state = sep->parm_args[0];
    int health = sep->parm_args[1];

    if( actor->health < health )
      P_SetMobjState( actor, state );
}

// These comments are from DSDA-Doom
// Jumps to a state if caller's target is in line-of-sight.
//   args[0]: State to jump to
//   args[1]: Field-of-view to check (degrees, in fixed point); if zero, will check in all directions
void A_JumpIfTargetInSight_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    if( !actor->target )
      return;

    // Args Required.
    // Args[0] is a new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    // This uses FixedToAngle from EternityEngine
    int state = sep->parm_args[0];
    angle_t fov = FixedToAngle_EE(sep->parm_args[1]);

    // Check FOV first since it's faster
    if( (fov > 0) && !P_Check_FOV(actor, actor->target, fov))  // FIXME
      return;

    if( P_CheckSight(actor, actor->target) )
      P_SetMobjState(actor, state);
}

// These comments are from DSDA-Doom
// Jumps to a state if caller's target is closer than the specified distance.
//   args[0]: State to jump to
//   args[1]: Distance threshold
void A_JumpIfTargetCloser_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    if( !actor->target )
      return;

    // Args Required.
    // Args[0] is a new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    int state = sep->parm_args[0];
    int distance = sep->parm_args[1];

    if( distance > P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y))
      P_SetMobjState(actor, state);
}

// These comments are from DSDA-Doom
// Jumps to a state if caller's tracer (seek target) is in line-of-sight.
//   args[0]: State to jump to
//   args[1]: Field-of-view to check (degrees, in fixed point); if zero, will check in all directions
void A_JumpIfTracerInSight_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    if( !actor->target )
      return;

    // Args Required.
    // Args[0] is a new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    // This uses FixedToAngle from EternityEngine
    int state = sep->parm_args[0];
    angle_t fov = FixedToAngle_EE(sep->parm_args[1]);

    // Check FOV first since it's faster
    if( (fov > 0) && !P_Check_FOV(actor, actor->tracer, fov))  // FIXME
      return;

    if( P_CheckSight(actor, actor->tracer) )
      P_SetMobjState(actor, state);
}

// These comments are from DSDA-Doom
// Jumps to a state if caller's tracer (seek target) is closer than the specified distance.
//   args[0]: State to jump to
//   args[1]: Distance threshold (fixed point)
void A_JumpIfTracerCloser_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    if( !actor->tracer )
      return;

    // Args Required.
    // Args[0] is a new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    int state = sep->parm_args[0];
    int distance = sep->parm_args[1];

    if( distance > P_AproxDistance(actor->x - actor->tracer->x, actor->y - actor->tracer->y))
      P_SetMobjState(actor, state);
}

// These comments are from DSDA-Doom
// Jumps to a state if caller has the specified thing flags set.
//   args[0]: State to jump to
//   args[1]: Standard Flags to check
//   args[2]: MBF21 Flags to check MF2_
//   args[7]: MBF21 Flags to check MF3_
void A_JumpIfFlagsSet_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    // Args Required.
    // Args[0] is a new state.
    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    int state = sep->parm_args[0];

    // Jump if all bits within flags_req, flags_req2, and flags_req_mbf21 are set.
    // Easier to invert flags, and test for all req being 0.
    if( ( ((~actor->flags ) & sep->parm_args[1])
        | ((~actor->flags2) & sep->parm_args[2]) // MBF21 flags MF2_
        | ((~actor->flags3) & sep->parm_args[7]) // MBF21 flags MF3_
        ) == 0 )
      P_SetMobjState(actor, state);
}

// These comments are from DSDA-Doom
// Adds the specified thing flags to the caller.
//   args[0]: Standard Flag(s) to add
//   args[1]: MBF21 Flags to add MF2_
//   args[7]: MBF21 Flags to add MF3_
void A_AddFlags_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    actor->flags  |= sep->parm_args[0];
    actor->flags2 |= sep->parm_args[1];  // MBF21_flags, MF2_
    actor->flags3 |= sep->parm_args[7];  // MBF21_flags, MF3_
}

// These comments are from DSDA-Doom
// Removes the specified thing flags from the caller.
//   args[0]: Flag(s) to remove
//   args[1]: MBF21 Flags to remove MF2_
//   args[7]: MBF21 Flags to remove MF3_
void A_RemoveFlags_MBF21( mobj_t* actor )
{
    if( !EN_mbf21_action || !actor || !actor->state )
        return;

    state_ext_t * sep = P_state_ext( actor->state );
    if( sep == &empty_state_ext )
        return;

    actor->flags  &= ~(sep->parm_args[0]);
    actor->flags2 &= ~(sep->parm_args[1]);  // MBF21 flags, MF2_
    actor->flags3 &= ~(sep->parm_args[7]);  // MBF21 flags, MF3_
}

// MBF21
#endif


consvar_t * enemy_cvar_list[] =
{
  &cv_solidcorpse,
  &cv_monbehavior,
  &cv_monsterfriction,
  &cv_monster_remember,
  &cv_monstergravity,
#ifdef MONSTER_VARY
  &cv_monster_vary,
  &cv_vary_percent,
  &cv_vary_size,
#endif
#ifdef ENABLE_SLOW_REACT
  &cv_slow_react,
#endif
  &cv_doorstuck,
    // MBF
  &cv_mbf_falloff,
  &cv_mbf_monster_avoid_hazard,
  &cv_mbf_monster_backing,
  &cv_mbf_dropoff,
  &cv_mbf_pursuit,
  &cv_mbf_distfriend,
  &cv_mbf_staylift,
  &cv_mbf_help_friend,
  &cv_mbf_monkeys,
#ifdef DOGS
  &cv_mbf_dogs,
  &cv_mbf_dog_jumping,
#endif
  NULL
};



#include "p_henemy.c"
