// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_fab.c 1686 2024-06-20 09:45:56Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: p_fab.c,v $
// Revision 1.5  2000/10/21 08:43:30  bpereira
// Revision 1.4  2000/07/01 09:23:49  bpereira
// Revision 1.3  2000/04/24 20:24:38  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      some new action routines, separated from the original doom
//      sources, so that you can include it or remove it easy.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "g_game.h"
#include "p_local.h"
#include "r_state.h"
#include "p_fab.h"
#include "m_random.h"
#include "dehacked.h"
  // thing_flags_valid_deh
#include "info.h"
  // MT_xxx
#include "r_data.h"
  // TRANSLU_xxx


#ifdef DOORDELAY_CONTROL
// [WDJ] 1/15/2009 support control of door and event delay
// see p_doors.c
extern  int  delay_ticks_per_sec;

CV_PossibleValue_t doordelay_cons_t[]={{0,"0.9x Fast"}, {1,"Normal"}, {2,"1.2x Slow"}, {3,"1.6x Slow"}, {4,"2x Slow"}, {5,"3x Slow"}, {0,NULL}};
int  doordelay_table[6] = { 32, 35, 42, 56, 70, 105 };  // 35 is normal

void DoorDelay_OnChange( void )
{
# ifdef MAPTHING_ADJUST_MASTER
   // [WDJ] Disable door delay with rest of map adjust options ??.
   delay_ticks_per_sec = doordelay_table[ (cv_mapthing_adjust_master.EV == 0)? cv_doordelay.value : 1 ];
# else
   delay_ticks_per_sec = doordelay_table[ cv_doordelay.value ];
# endif
}

consvar_t cv_doordelay = {"doordelay","1",CV_NETVAR|CV_CALL|CV_SAVE,doordelay_cons_t,DoorDelay_OnChange};

#endif



// [WDJ] 2/3/2011  Insta-death by voodoo doll, subverted.
CV_PossibleValue_t instadeath_cons_t[]={{0,"Die"}, {1,"Damage"}, {2,"Zap"}, {0,NULL}};
consvar_t cv_instadeath = {"instadeath", "0", CV_SAVE|CV_NETVAR, instadeath_cons_t, NULL};

voodoo_mode_e  voodoo_mode;

// [WDJ] 2/7/2011  Voodoo doll behavior
void VoodooMode_OnChange( void )
{
   // config file saves cv_voodoo_mode but changes to voodoo_mode do not get saved.
   voodoo_mode = (voodoo_mode_e) cv_voodoo_mode.EV;
}

CV_PossibleValue_t voodoo_mode_cons_t[]={{0,"Vanilla"}, {1,"Multispawn"}, {2,"Target"}, {3, "Auto"}, {0,NULL}};
consvar_t cv_voodoo_mode = {"voodoo_mode", "3", CV_CALL|CV_SAVE|CV_NETVAR, voodoo_mode_cons_t, VoodooMode_OnChange};


// Translucency

void Translucency_OnChange(void);

// Auto exists mostly as the best conversion of previous Off/On.
CV_PossibleValue_t translucency_cons_t[]={{0,"Off"}, {1,"Auto"}, {2,"Boom"}, {3, "Legacy"}, {4, "All"}, {0,NULL}};
consvar_t cv_translucency  = {"translucency" ,"1",CV_CALL|CV_SAVE, translucency_cons_t, Translucency_OnChange};


//
// Action routine, for the ROCKET thing.
// This one adds trails of smoke to the rocket.
// The action pointer of the S_ROCKET state must point here to take effect.
// This routine is based on the Revenant Fireball Tracer code A_Tracer()
//
void A_SmokeTrailer (mobj_t* actor)
{
    mobj_t*     th;

    if (gametic % (4 * NEWTICRATERATIO))
        return;

    // spawn a puff of smoke behind the rocket
    if( (EV_legacy < 125)  // rocket trails spawnpuff from v1.11 to v1.24
        && (EV_legacy >= 111) ) // skull trails since v1.25
        P_SpawnPuff (actor->x, actor->y, actor->z);

    // add the smoke behind the rocket
    th = P_SpawnMobj (actor->x-actor->momx,
                      actor->y-actor->momy,
                      actor->z, MT_SMOK);

    th->momz = FRACUNIT;

    // Legacy use of P_Random, enabled by EV_legacy.
    // Do not use extra P_Random during Doom and Boom demos.
    // [WDJ] Legacy cannot interact with smoke, but the thought of some obscure
    // interaction in the future makes me continue to use P_Random.
    th->tics -= (EV_legacy ? PP_Random(pL_smoketrail) : N_Random() ) & 3;
    if (th->tics < 1)
        th->tics = 1;
}


// [WDJ] Table of translucent objects
enum { TRF_MT = 0xFFF, TRF_ext = 0x1000, TRF_noflag = 0x8000 };

typedef struct {
    uint16_t     flag_mt;  // MT type to check MF_TRANSLUCENT, + TRF_
    uint16_t     first_state, last_state;  // statenum_t
    byte         translu_sel;   // translucent_e, no Boom ext
} PACKED_ATTR  translucent_state_item_t;
// [WDJ] Table costs 500 more bytes than the inline calls when not packed !!

static translucent_state_item_t  translucent_state_table[] =
{
    //revenant fireball, MT_TRACER, extension effect
    {MT_TRACER|TRF_ext, S_TRACER,    S_TRACER2,   TRANSLU_fire},
    {MT_TRACER|TRF_ext, S_TRACEEXP1, S_TRACEEXP3, TRANSLU_med},

    //rev. fireball. smoke trail, MT_SMOKE
    {MT_SMOKE, S_SMOKE1, S_SMOKE5, TRANSLU_med},
                                           
    //imp fireball, MT_TROOPSHOT
    {MT_TROOPSHOT, S_TBALL1,  S_TBALL2,  TRANSLU_fire},
    {MT_TROOPSHOT, S_TBALLX1, S_TBALLX3, TRANSLU_med}, 
                                           
    //archvile attack, MT_FIRE
    {MT_FIRE, S_FIRE1, S_FIRE30, TRANSLU_fire},
                                           
    //bfg ball, MT_BFG
    {MT_BFG, S_BFGSHOT,  S_BFGSHOT2, TRANSLU_fire},
    {MT_BFG, S_BFGLAND,  S_BFGLAND3, TRANSLU_med},
    {MT_BFG, S_BFGLAND4, S_BFGLAND6, TRANSLU_more},
    {MT_BFG, S_BFGEXP,   0         , TRANSLU_med},
    {MT_BFG, S_BFGEXP2,  S_BFGEXP4 , TRANSLU_more},
                                           
    //plasma bullet, MT_PLASMA
    {MT_PLASMA, S_PLASBALL, S_PLASBALL2, TRANSLU_fire},
    {MT_PLASMA, S_PLASEXP,  S_PLASEXP2,  TRANSLU_med},
    {MT_PLASMA, S_PLASEXP3, S_PLASEXP5,  TRANSLU_more},
                                           
    //bullet puff, MT_PUFF
    {MT_PUFF, S_PUFF1, S_PUFF4, TRANSLU_more},
                                           
    //teleport fog, MT_TFOG
    {MT_TFOG, S_TFOG,  S_TFOG5,  TRANSLU_med},
    {MT_TFOG, S_TFOG6, S_TFOG10, TRANSLU_more},
                                           
    //respawn item fog, MT_IFOG
    {MT_IFOG, S_IFOG, S_IFOG5, TRANSLU_med},
                                           
    //soulsphere, MT_MISC12
    {MT_MISC12, S_SOUL, S_SOUL6, TRANSLU_med},
    //invulnerability, MT_INV
    {MT_INV, S_PINV, S_PINV4, TRANSLU_med},
    //blur artifact, MT_INS
    {MT_INS, S_PINS, S_PINS4, TRANSLU_med},
    //megasphere, MT_MEGA
    {MT_MEGA, S_MEGA, S_MEGA4, TRANSLU_med},
                            
    // MT_MISC42, no flags extension effect
    {TRF_noflag, S_GREENTORCH, S_REDTORCH4, TRANSLU_fx1}, // blue torch
    // MT_MISC45, no flags extension effect
    {TRF_noflag, S_GTORCHSHRT, S_RTORCHSHRT4, TRANSLU_fx1}, // short blue torch

    // flaming barrel !!, MT_MISC77, no flags extension effect
    {TRF_noflag, S_BBAR1, S_BBAR3, TRANSLU_fx1},

    //lost soul, MT_SKULL, extension effect
    {MT_SKULL|TRF_ext, S_SKULL_STND, S_SKULL_DIE6, TRANSLU_fx1},
    //baron shot, MT_BRUISERSHOT
    {MT_BRUISERSHOT, S_BRBALL1,  S_BRBALL2,  TRANSLU_fire},
    {MT_BRUISERSHOT, S_BRBALLX1, S_BRBALLX3, TRANSLU_med},
    //demon spawnfire, MT_SPAWNFIRE
    {MT_SPAWNFIRE, S_SPAWNFIRE1, S_SPAWNFIRE3, TRANSLU_fire},
    {MT_SPAWNFIRE, S_SPAWNFIRE4, S_SPAWNFIRE8, TRANSLU_med},
    //caco fireball, MT_HEADSHOT
    {MT_HEADSHOT, S_RBALL1,  S_RBALL2,  TRANSLU_fire},
    {MT_HEADSHOT, S_RBALLX1, S_RBALLX3, TRANSLU_med},

    //arachno shot, MT_ARACHPLAZ
    {MT_ARACHPLAZ, S_ARACH_PLAZ, S_ARACH_PLAZ2, TRANSLU_fire},
    {MT_ARACHPLAZ, S_ARACH_PLEX, S_ARACH_PLEX2, TRANSLU_med},
    {MT_ARACHPLAZ, S_ARACH_PLEX3,S_ARACH_PLEX4, TRANSLU_more},
    {MT_ARACHPLAZ, S_ARACH_PLEX5,            0, TRANSLU_hi},

    //blood puffs!, MT_BLOOD
    //{TRF_noflag, S_BLOOD1   ,            0, TRANSLU_med},
    //{TRF_noflag, S_BLOOD2   , S_BLOOD3    , TRANSLU_more},

    //eye in symbol, MT_MISC38, no flags extension effect
    {TRF_noflag, S_EVILEYE, S_EVILEYE4, TRANSLU_med},
                                          
    //mancubus fireball, MT_FATSHOT
    {MT_FATSHOT, S_FATSHOT1,  S_FATSHOT2,  TRANSLU_fire},
    {MT_FATSHOT, S_FATSHOTX1, S_FATSHOTX3, TRANSLU_med},

    // rockets explosion, MT_ROCKET, no flags extension effect
    {TRF_noflag, S_EXPLODE1, S_EXPLODE2, TRANSLU_fire},
    {TRF_noflag, S_EXPLODE3,          0, TRANSLU_med},

    //Fab: lava/slime damage smoke test, MT_SMOK, no flags extension effect
    {TRF_noflag, S_SMOK1,   S_SMOK5,   TRANSLU_med},
    {TRF_noflag, S_SPLASH1, S_SPLASH3, TRANSLU_more},

    {TRF_noflag, S_NULL, S_NULL, TRANSLU_more}  // term
};

typedef enum {
   TE_off, TE_boom, TE_ext, TE_all
} translucent_enable_e;

static translucent_enable_e  translucent_enable = TE_all;

//  hack the translucency in the states for a set of standard doom sprites
//
void P_SetTranslucencies (void)
{
    translucent_state_item_t * tip;
    state_t * laststate;
    state_t * state;
    boolean  tr_enable;

    for(tip=&translucent_state_table[0]; tip->first_state != S_NULL; tip++)
    {
        tr_enable = false;  // default
        switch( translucent_enable )
        {
         case TE_off:  // reset translucent
            break;
         case TE_all:  // independent of info MF_TRANSLUCENT
            tr_enable = true;
            break;
         case TE_boom: // ignore DoomLegacy extensions
            if(tip->flag_mt & TRF_ext)  break;  // legacy extension
            // continue to TE_ext to test flag
         case TE_ext:  // all flag, including DoomLegacy extensions
            if(tip->flag_mt == TRF_noflag)  // does not check MF_TRANSLUCENT
            {
                tr_enable = true;
            }
            else
            {
                // check info flag MF_TRANSLUCENT, maybe modified by BEX
                register int mobjindex = tip->flag_mt & TRF_MT;
                if( mobjindex < NUMMOBJTYPES
                    &&  mobjinfo[mobjindex].flags & MF_TRANSLUCENT )
                    tr_enable = true;
            }
            break;
        }
        // change the info state tables for the object
        laststate = &states[tip->last_state];
        for( state = &states[tip->first_state]; state <= laststate; state++ )
        {
            state->frame &= ~FF_TRANSMASK;  // clear previous translucent
            if( tr_enable )
                state->frame |= (tip->translu_sel<<FF_TRANSSHIFT);
        }
    }
}

// Auto -> TE_ext, extensions checked
byte translucency_en_table[] =
{ TE_off, TE_ext, TE_boom, TE_ext, TE_all };

void Translucency_OnChange(void)
{
    // [WDJ] Translucent control
    // TE_boom has fewer transparent items.
    // TE_all (as before) unless DEH has set flags.
    translucent_enable = translucency_en_table[ cv_translucency.value ];
    if( cv_translucency.value != 1 )  // not auto
    {
        if( thing_flags_valid_deh
           && ((translucent_enable == TE_off) || (translucent_enable == TE_all) ) )
            GenPrintf( EMSG_hud, "DEH set translucency overridden by controls.\n" );
    }
    P_SetTranslucencies();
}




// =======================================================================
//                    FUNKY DEATHMATCH COMMANDS
// =======================================================================

void BloodTime_OnChange (void);

CV_PossibleValue_t bloodtime_cons_t[]={{1,"MIN"},{3600,"MAX"},{0,NULL}};
// how much tics to last for the last (third) frame of blood (S_BLOODx)
consvar_t cv_bloodtime = {"bloodtime","20",CV_NETVAR|CV_VALUE|CV_CALL|CV_SAVE,bloodtime_cons_t,BloodTime_OnChange};

// Called when var. 'bloodtime' is changed : set the blood states duration
//
void BloodTime_OnChange (void)
{
    // CV_VALUE
    states[S_BLOOD1].tics = 8;
    states[S_BLOOD2].tics = 8;
    states[S_BLOOD3].tics = (cv_bloodtime.value*TICRATE) - 16;

    CONS_Printf ("blood lasts for %d seconds\n", cv_bloodtime.value);
}

consvar_t * fab_cvar_list[] =
{
  // add commands for deathmatch rules and style (like more blood) :)
  &cv_bloodtime,

  // BP:not realy in deathmatch but is just here
  &cv_translucency,

#ifdef DOORDELAY_CONTROL
  // [WDJ] 1/15/2009 support control of door and event delay
  // see p_doors.c
  &cv_doordelay,
#endif
  NULL
};


void  DemoAdapt_p_fab(void)  // local enables of p_fab
{
    if( ! demoplayback )
    {
        // restore player settings
#ifdef DOORDELAY_CONTROL
        DoorDelay_OnChange();
#endif
        // cv_instadeath not covered here
        VoodooMode_OnChange();
    }
}

