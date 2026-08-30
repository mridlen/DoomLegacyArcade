// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_pspr.c 1745 2025-04-10 09:36:34Z wesleyjohnson $
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
// $Log: p_pspr.c,v $
// Revision 1.11  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.10  2001/08/02 19:15:59  bpereira
// fix player reset in secret level of doom2
//
// Revision 1.9  2001/06/10 21:16:01  bpereira
// Revision 1.8  2001/05/27 13:42:48  bpereira
//
// Revision 1.7  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.6  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.5  2000/10/21 08:43:30  bpereira
// Revision 1.4  2000/10/01 10:18:18  bpereira
// Revision 1.3  2000/08/31 14:30:56  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Weapon sprite animation, weapon objects.
//      Action functions for weapons.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_pspr.h"
#include "d_event.h"
#include "p_local.h"
#include "hs_stuff.h"   // [Arcade] HS_Player_Fired_Weapon
#include "p_inter.h"
#include "s_sound.h"
#include "g_game.h"
#include "g_input.h"
#include "r_main.h"
#include "m_random.h"
#include "p_inter.h"
#include "infoext.h"
#ifdef MBF21
#include "p_tick.h"
#endif

#include "hardware/hw3sound.h"

#define LOWERSPEED              FRACUNIT*6
#define RAISESPEED              FRACUNIT*6

#define WEAPONBOTTOM            128*FRACUNIT
#define WEAPONTOP               32*FRACUNIT

#define FLAME_THROWER_TICS      (10*TICRATE)
#define MAGIC_JUNK              1234
#define MAX_MACE_SPOTS          8

#ifdef MBF21
// [WDJ] MBF21, to support MBF21 changes.

//
// P_SetPsprite
//
//  ps_position : PS_weapon or PS_flash
static
void P_SetPsprite_psp ( player_t*   player,
                        pspdef_t*   psp,
                        statenum_t  stnum )
{
    state_t*    state;

    do
    {
        if (!stnum)
        {
            // object removed itself
            psp->state = NULL;
            break;
        }
#ifdef PARANOIA
        if(stnum >= NUMSTATES_EXT)
        {
            I_SoftError("P_SetPsprite : state %d unknown\n",stnum);
            return;
        }
#endif
        state = &states[stnum];
        psp->state = state;
        psp->tics = state->tics;        // could be 0

        // MBF
        state_ext_t * sep =  P_state_ext( state );
#ifdef HEXEN
        if( EN_hexen )
        {
            // coordinate set, individually conditional on non-zero
            if( sep->parm1 )  // misc1
              psp->sx = sep->parm1 << FRACBITS;
            if( sep->parm2 )  // misc2
              psp->sy = sep->parm2 << FRACBITS;
        }
	else
#endif
        if( sep->parm1 )  // misc1
        {
            // coordinate set
            psp->sx = sep->parm1 << FRACBITS;  // misc1
            psp->sy = sep->parm2 << FRACBITS;  // misc2
        }

        // Call action routine.
        // Modified handling.
        // [WDJ] No more action ptrs, use table lookup.
        if( state->action )
        {
#ifdef PARANOIA	    
            if( (state->action < AP_acp2_start) || (state->action >= AP_acp2_end) )
            {
                I_SoftError( "State[%i] calls bad player weapon action function: %i\n", stnum, state->action );
                break;
            }
#endif
            action_acp2_table[ state->action - AP_acp2_start ](player, psp);  // (player_t*, pspdef_t*)
            if (!psp->state)
                break;
        }

        stnum = psp->state->nextstate;

    } while (!psp->tics);
    // an initial state of 0 could cycle through
}

// The normal calls to  P_SetPsprite.
void P_SetPsprite ( player_t*     player,
                    int           ps_position,
                    statenum_t    stnum )
{
    pspdef_t*  psp = &player->psprites[ps_position];
    P_SetPsprite_psp ( player, psp, stnum );
}


#else

//
// P_SetPsprite
//
//  ps_position : PS_weapon or PS_flash
void P_SetPsprite ( player_t*     player,
                    int           ps_position,
                    statenum_t    stnum )
{
    pspdef_t*   psp;
    state_t*    state;

    psp = &player->psprites[ps_position];

    do
    {
        if (!stnum)
        {
            // object removed itself
            psp->state = NULL;
            break;
        }
#ifdef PARANOIA
        if(stnum >= NUMSTATES_EXT)
        {
            I_SoftError("P_SetPsprite : state %d unknown\n",stnum);
            return;
        }
#endif
        state = &states[stnum];
        psp->state = state;
        psp->tics = state->tics;        // could be 0

        // MBF
        state_ext_t * sep =  P_state_ext( state );
#ifdef HEXEN
        if( EN_hexen )
        {
            // coordinate set, individually conditional on non-zero
            if( sep->parm1 )  // misc1
              psp->sx = sep->parm1 << FRACBITS;  // misc1
            if( sep->parm2 )  // misc2
              psp->sy = sep->parm2 << FRACBITS;  // misc2
        }
	else
#endif
        if( sep->parm1 )  // parm1
        {
            // coordinate set
            psp->sx = sep->parm1 << FRACBITS;  // misc1
            psp->sy = sep->parm2 << FRACBITS;  // misc2
        }

        // Call action routine.
        // Modified handling.
        // [WDJ] No more action ptrs, use table lookup.
        if( state->action )
        {
#ifdef PARANOIA	    
            if( (state->action < AP_acp2_start) || (state->action >= AP_acp2_end) )
            {
                I_SoftError( "State[%i] calls bad player weapon action function: %i\n", stnum, state->action );
                break;
            }
#endif
            action_acp2_table[ state->action - AP_acp2_start ](player, psp);  // (player_t*, pspdef_t*)
            if (!psp->state)
                break;
        }

        stnum = psp->state->nextstate;

    } while (!psp->tics);
    // an initial state of 0 could cycle through
}
#endif



//
// P_CalcSwing
//
/* BP: UNUSED

fixed_t         swingx;
fixed_t         swingy;

void P_CalcSwing (player_t*     player)
{
    fixed_t     swing;
    int         angf;

    // OPTIMIZE: tablify this.
    // A LUT would allow for different modes,
    //  and add flexibility.

    swing = player->bob;

    // bobbing motion
    angf = (FINEANGLES/70*leveltime)&FINEMASK;
    swingx = FixedMul ( swing, finesine[angf]);

    angf = (FINEANGLES/70*leveltime+FINE_ANG180)&FINEMASK;
    swingy = -FixedMul ( swingx, finesine[angf]);
}
*/


//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void P_BringUpWeapon (player_t* player)
{
    statenum_t  newstate;

    if (player->pendingweapon == wp_nochange)
        player->pendingweapon = player->readyweapon;

    if (player->pendingweapon == wp_chainsaw)
        S_StartAttackSound(player->mo, sfx_sawup);

#ifdef PARANOIA
    if(player->pendingweapon>=NUMWEAPONS)
    {
         I_Error("P_BringUpWeapon : pendingweapon %d\n",player->pendingweapon);
    }
#endif

    newstate = player->weaponinfo[player->pendingweapon].upstate;

    player->pendingweapon = wp_nochange;
    player->psprites[PS_weapon].sy = WEAPONBOTTOM;

    P_SetPsprite (player, PS_weapon, newstate);
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
boolean P_CheckAmmo (player_t* player)
{
    byte  plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo = & player->weaponinfo[plwpn];
    ammotype_t          ammo_type;
    int                 count;

    ammo_type = plwpninfo->ammo;

    // Minimal amount for one shot varies.
    count = plwpninfo->ammo_per_shot;

    // Some do not need ammunition anyway.
    // Return if current ammunition sufficient.
    if (ammo_type == am_noammo || player->ammo[ammo_type] >= count)
        return true;

    // Out of ammo, pick a weapon to change to.
    // Preferences are set here.
     // added by Boris : preferred weapons order
    if( ! (player->GF_flags & GF_original_weapon) )
         VerifFavoritWeapon(player);
    else // eof Boris
    if( EN_heretic )
    {
        do
        {
            if(player->weaponowned[wp_skullrod]
                && player->ammo[am_skullrod] > player->weaponinfo[wp_skullrod].ammo_per_shot)
            {
                player->pendingweapon = wp_skullrod;
            }
            else if(player->weaponowned[wp_blaster]
                && player->ammo[am_blaster] > player->weaponinfo[wp_blaster].ammo_per_shot)
            {
                player->pendingweapon = wp_blaster;
            }
            else if(player->weaponowned[wp_crossbow]
                && player->ammo[am_crossbow] > player->weaponinfo[wp_crossbow].ammo_per_shot)
            {
                player->pendingweapon = wp_crossbow;
            }
            else if(player->weaponowned[wp_mace]
                && player->ammo[am_mace] > player->weaponinfo[wp_mace].ammo_per_shot)
            {
                player->pendingweapon = wp_mace;
            }
            else if(player->ammo[am_goldwand] > player->weaponinfo[wp_goldwand].ammo_per_shot)
            {
                player->pendingweapon = wp_goldwand;
            }
            else if(player->weaponowned[wp_gauntlets])
            {
                player->pendingweapon = wp_gauntlets;
            }
            else if(player->weaponowned[wp_phoenixrod]
                && player->ammo[am_phoenixrod] > player->weaponinfo[wp_phoenixrod].ammo_per_shot)
            {
                player->pendingweapon = wp_phoenixrod;
            }
            else
            {
                player->pendingweapon = wp_staff;
            }
        } while(player->pendingweapon == wp_nochange);
    }
    else
    {
        do
        {
            if (player->weaponowned[wp_plasma]
                && player->ammo[am_cell]>=player->weaponinfo[wp_plasma].ammo_per_shot
                && (gamemode != doom_shareware) )
            {
                player->pendingweapon = wp_plasma;
            }
            else if (player->weaponowned[wp_supershotgun]
                && player->ammo[am_shell]>=player->weaponinfo[wp_supershotgun].ammo_per_shot
                && (gamemode == doom2_commercial) )
            {
                player->pendingweapon = wp_supershotgun;
            }
            else if (player->weaponowned[wp_chaingun]
                && player->ammo[am_clip]>=player->weaponinfo[wp_chaingun].ammo_per_shot)
            {
                player->pendingweapon = wp_chaingun;
            }
            else if (player->weaponowned[wp_shotgun]
                && player->ammo[am_shell]>=player->weaponinfo[wp_shotgun].ammo_per_shot)
            {
                player->pendingweapon = wp_shotgun;
            }
            else if (player->ammo[am_clip]>=player->weaponinfo[wp_pistol].ammo_per_shot)
            {
                player->pendingweapon = wp_pistol;
            }
            else if (player->weaponowned[wp_chainsaw])
            {
                player->pendingweapon = wp_chainsaw;
            }
            else if (player->weaponowned[wp_missile]
                && player->ammo[am_misl]>=player->weaponinfo[wp_missile].ammo_per_shot)
            {
                player->pendingweapon = wp_missile;
            }
            else if (player->weaponowned[wp_bfg]
                && player->ammo[am_cell]>=player->weaponinfo[wp_bfg].ammo_per_shot
                && (gamemode != doom_shareware) )
            {
                player->pendingweapon = wp_bfg;
            }
            else
            {
                // If everything fails.
                player->pendingweapon = wp_fist;
            }
            
        } while (player->pendingweapon == wp_nochange);
    }

    // Now set appropriate weapon overlay.
    P_SetPsprite (player,
                  PS_weapon,
                  player->weaponinfo[player->readyweapon].downstate);

    return false;
}


#ifdef MBF21
// [WDJ] MBF21 Ammo handling, reference DSDA-Doom.

// [WDJ] Comments from DSDA-Doom
// P_SubtractAmmo
// Subtracts ammo, w/compat handling. In mbf21, use
// readyweapon's "ammopershot" field if it's explicitly
// defined in dehacked; otherwise use the amount specified
// by the codepointer instead (Doom behavior)
//
// [XA] NOTE: this function is for handling Doom's native
// attack pointers; of note, the new A_ConsumeAmmo mbf21
// codepointer does NOT call this function, since it doesn't
// have to worry about any compatibility shenanigans.
//

// [WDJ] Because DoomLegacy was already using ammo_per_shot,
// the MBF21 function is modified so that the vanilla ammo_used
// is the ammo_per_shot.

//   ammo_used: vanilla amount of ammo used, if > 0xFFF0 use ammo_per_shot
void MBF21_subtract_ammo( player_t* player, unsigned int ammo_used )
{
    byte  plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo = & player->weaponinfo[plwpn];
    ammotype_t ammo_type = plwpninfo->ammo;
    unsigned int pl_ammo = player->ammo[ammo_type];

#if 0
    // [WDJ] Not in DoomLegacy
    // [XA] hmm... I guess vanilla/boom will go out of bounds then?
    if( player->cheats & CF_INFINITE_AMMO )
      return;
#endif

    if( ammo_used > 0xFFF0 )  // use ammo_per_shot
        ammo_used = plwpninfo->ammo_per_shot;
    
    // Handle MBF(WIF_ENABLEAPS)
    if( EN_mbf21 )
    {
        if( ammo_type == am_noammo )
          return;
    
        if( plwpninfo->eflags & WEF_MBF21_PER_SHOT )
        {
            // override the normal ammo used
            ammo_used = plwpninfo->ammo_per_shot;
        }
    }

    pl_ammo -= ammo_used;

    if( EN_mbf21 && (pl_ammo < 0) )
      pl_ammo = 0;

    player->ammo[ammo_type] = pl_ammo;
}
#endif

//
// P_FireWeapon.
//
void P_FireWeapon (player_t* player)
{
    mobj_t * pmo = player->mo;
    statenum_t  newstate;
    byte  plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo = & player->weaponinfo[plwpn];

    if (!P_CheckAmmo (player))
        return;

    // [Arcade] Tyson allows fist, chainsaw and pistol only.  Placed after the
    // ammo check so a click on an empty weapon -- which fires nothing -- does
    // not void the run.  Carrying and even selecting other weapons is fine;
    // this is only reached when a shot is actually taken.
    if( player == &players[consoleplayer] )
        HS_Player_Fired_Weapon( plwpn );

    if( EN_heretic )
    {
        P_SetMobjState(pmo, S_PLAY_ATK2);
        newstate = player->refire ? plwpninfo->holdatkstate : plwpninfo->atkstate;
        // Play the sound for the initial gauntlet attack
        if( plwpn == wp_gauntlets && !player->refire )
            S_StartObjSound(pmo, sfx_gntuse);
    }
#ifdef HEXEN
    else if( EN_hexen )
    {
        P_SetMobjState(player->mo, pclass[player->pclass].fire_weapon_state);
        // plwpninfo = hexen_weaponinfo[player->readyweapon][player->pclass]

        if( player->pclass == PCLASS_FIGHTER
            && (plwpn == wp_second)
            && player->ammo[MANA_1] > 0)
        {
            // Glowing axe
            newstate = S_HEXEN_FAXEATK_G1;
        }
        else
        {
            newstate = player->refire ? plwpninfo->holdatkstate : plwpninfo->atkstate;
        }
    }
#endif
    else
    {
        P_SetMobjState (pmo, S_PLAY_ATK1);
        newstate = plwpninfo->atkstate;
    }
    P_SetPsprite (player, PS_weapon, newstate);
#ifdef MBF21
    if( ! (plwpninfo->flags & WPF_SILENT ) )
      P_NoiseAlert (pmo, pmo);
# ifdef HEXEN
    if( ! (plwpninfo->flags & WPF_SILENT )
        || EN_hexen )
      P_NoiseAlert (pmo, pmo);
# endif    
#else
    P_NoiseAlert (pmo, pmo);
#endif
}



//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon (player_t* player)
{
    P_SetPsprite (player,
                  PS_weapon,
                  player->weaponinfo[player->readyweapon].downstate);
}



// ACP2 functions, called from player weapon state.

//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady ( player_t*     player,
                     pspdef_t*     psp )
{
    mobj_t * pmo = player->mo;
    byte  plwpn = player->readyweapon;

    if(player->chickenTics)
    { // Change to the chicken beak
        P_ActivateBeak(player);
        return;
    }

    // get out of attack state
    if (pmo->state == &states[S_PLAY_ATK1]
        || pmo->state == &states[S_PLAY_ATK2] )
    {
        P_SetMobjState (pmo, S_PLAY);
    }

    if (plwpn == wp_chainsaw
        && psp->state == &states[S_SAW])
    {
        S_StartAttackSound(pmo, sfx_sawidl);
    }
    // Check for staff PL2 active sound
    if( EN_heretic && (plwpn == wp_staff)
        && (psp->state == &states[S_STAFFREADY2_1])
        && PP_Random(ph_staffready) < 128)  // Heretic
    {
        S_StartAttackSound(pmo, sfx_stfcrk);
    }

    // check for change
    //  if player is dead, put the weapon away
    if (player->pendingweapon != wp_nochange || !player->health)
    {
        // change weapon
        //  (pending weapon should already be validated)
        P_SetPsprite (player, PS_weapon, player->weaponinfo[plwpn].downstate);
        return;
    }

    // check for fire
    if (player->cmd.buttons & BT_ATTACK)
    {
        //  the missile launcher and bfg do not auto fire
#ifdef MBF21
        if( ! (player->GB_flags & GB_attackdown) // autofire repeat
             || !( player->weaponinfo[plwpn].flags & WPF_NO_AUTOFIRE )  // Heretic: phoenixrod, Doom: missile, BFG
# ifdef HEXEN
             || EN_hexen   // ?????, is this needed
# endif
           )
#else
        if( ! (player->GB_flags & GB_attackdown)
             || !( (plwpn == wp_missile)  // Heretic: phoenixrod
                    || (EN_doom_etc && (plwpn == wp_bfg)) )
           )
#endif
        {
            player->GB_flags |= GB_attackdown;
            P_FireWeapon (player);
            return;
        }
    }
    else
        player->GB_flags &= ~(GB_attackdown);

#ifdef CLIENTPREDICTION2
#else
    {
        int  angf;
        // bob the weapon based on movement speed, in a half arc
        angf = (128*leveltime/NEWTICRATERATIO)&FINEMASK;
        psp->sx = FRACUNIT + FixedMul (player->bob, finecosine[angf]);
        angf &= (FINE_ANG180-1);  // mask to limit it to ANG180
        psp->sy = WEAPONTOP + FixedMul (player->bob, finesine[angf]);
    }
#endif
}

// client prediction stuff
void A_TicWeapon( player_t* player, pspdef_t* psp )
{
    if( (psp->state->action == AP_WeaponReady) && psp->tics == psp->state->tics )
    {
        int angf;

        // bob the weapon based on movement speed
#ifdef CLIENTPREDICTION2
        angf = (128*localgametic/NEWTICRATERATIO) & FINEMASK;
#else
        angf = (128*leveltime/NEWTICRATERATIO) & FINEMASK;
#endif
        psp->sx = FRACUNIT + FixedMul (player->bob, finecosine[angf]);
        angf &= (FINE_ANG180-1);  // mask to limit it to ANG180
        psp->sy = WEAPONTOP + FixedMul (player->bob, finesine[angf]);
    }
}


//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire( player_t* player,  pspdef_t* psp )
{
    // check for fire
    //  (if a weaponchange is pending, let it go through instead)
    if ( (player->cmd.buttons & BT_ATTACK)
         && player->pendingweapon == wp_nochange
         && player->health)
    {
        player->refire++;
        P_FireWeapon (player);
    }
    else
    {
        player->refire = 0;
        P_CheckAmmo (player);
    }
}


void A_CheckReload( player_t* player, pspdef_t* psp )
{
    P_CheckAmmo (player);
#if 0
    if (player->ammo[am_shell]<2)
        P_SetPsprite (player, PS_weapon, S_DSNR1);
#endif
}



//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void A_Lower ( player_t*     player,
               pspdef_t*     psp )
{
    if(player->chickenTics)
        psp->sy = WEAPONBOTTOM;
    else
        psp->sy += LOWERSPEED;

    // Is already down.
    if (psp->sy < WEAPONBOTTOM )
        return;

    // Player is dead.
    if (player->playerstate == PST_DEAD)
    {
        psp->sy = WEAPONBOTTOM;

        // don't bring weapon back up
        return;
    }

    // The old weapon has been lowered off the screen,
    // so change the weapon and start raising it
    if (!player->health)
    {
        // Player is dead, so keep the weapon off screen.
        P_SetPsprite (player,  PS_weapon, S_NULL);
        return;
    }

    player->readyweapon = player->pendingweapon;

    P_BringUpWeapon (player);
}


//
// A_Raise
//
void A_Raise( player_t*     player,
              pspdef_t*     psp )
{
    psp->sy -= RAISESPEED;

    if (psp->sy > WEAPONTOP )
        return;

    psp->sy = WEAPONTOP;

    // The weapon has been raised all the way,
    //  so change to the ready state.
    P_SetPsprite (player, PS_weapon, 
                  player->weaponinfo[player->readyweapon].readystate);
}



// [WDJ] Boom Weapon recoil, from PrBoom
// phares
// Weapons now recoil, amount depending on the weapon.
//
// The flash of the weapon firing was moved here so the recoil could be
// synched with it, rather than the pressing of the trigger.
// The BFG delay caused this to be necessary.

// phares
// The following array holds the recoil values
static const int recoil_values[] =
{
  10, // wp_fist
  10, // wp_pistol
  30, // wp_shotgun
  10, // wp_chaingun
  100,// wp_missile
  20, // wp_plasma
  100,// wp_bfg
  0,  // wp_chainsaw
  80  // wp_supershotgun
};

//  flash_state_offset : 0..4 or less
static void A_fire_flash_recoil(player_t* player, int flash_state_offset)
{
    P_SetPsprite(player, PS_flash,
        player->weaponinfo[player->readyweapon].flashstate + flash_state_offset );

    // killough 3/27/98: prevent recoil in no-clipping mode
    if( cv_weapon_recoil.EV && !(player->mo->flags & MF_NOCLIP) )
    {
        P_Thrust(player, ANG180+player->mo->angle,
               (2048 * recoil_values[player->readyweapon]) );
    }
}


//
// A_GunFlash
//
void A_GunFlash ( player_t* player, pspdef_t* psp )
{
    P_SetMobjState (player->mo, S_PLAY_ATK2);
    A_fire_flash_recoil( player, 0 );
}



//
// WEAPON ATTACKS
//


//
// A_Punch
//
void A_Punch ( player_t* player, pspdef_t* psp )
{
    mobj_t * pmo = player->mo;
    angle_t     angle;
    int         slope;
    int damage = (PP_Random(pr_punch) % 10 + 1)<<1;

    if (player->powers[pw_strength])
        damage *= 10;

    // WARNING: don't put two P_Random in one line, or else
    // the evaluation order is ambiguous.
    angle = pmo->angle + (PP_SignedRandom(pr_punchangle)<<18);

    // [WDJ] MBF, Make autoaiming prefer enemies.
    slope = P_AimLineAttack (pmo, angle, MELEERANGE, 1);  // MBF friend protect
    if( EN_mbf && !lar_linetarget )
        slope = P_AimLineAttack (pmo, angle, MELEERANGE, 0);
    P_LineAttack (pmo, angle, MELEERANGE, slope, damage);

    // turn to face target
    // lar_linetarget returned by P_LineAttack
    if (lar_linetarget)
    {
        S_StartAttackSound(pmo, sfx_punch);
        pmo->angle = R_PointToAngle2 (pmo->x, pmo->y,
                                      lar_linetarget->x, lar_linetarget->y);
    }
}


//
// A_Saw
//
void A_Saw ( player_t* player, pspdef_t* psp )
{
    mobj_t * pmo = player->mo;
    int  slope;
    // Random() must be in separate statements otherwise
    // evaluation order will be ambiguous (lose demo sync).
    int  damage = 2 * (PP_Random(pr_saw)%10+1);  // first random is damage
    // second random adds to angle, third random is subtraction
    angle_t  angle = pmo->angle + (PP_SignedRandom(pr_saw)<<18);

    // use meleerange + 1 so the puff doesn't skip the flash
    // [WDJ] MBF, Make autoaiming prefer enemies.
    slope = P_AimLineAttack (pmo, angle, MELEERANGE+1, EN_mbf);  // MBF friend protect
    if( EN_mbf && !lar_linetarget )
        slope = P_AimLineAttack (pmo, angle, MELEERANGE+1, 0);
    P_LineAttack (pmo, angle, MELEERANGE+1, slope, damage);

    // lar_linetarget returned by P_LineAttack
    if (!lar_linetarget)
    {
        S_StartAttackSound(pmo, sfx_sawful);
        return;
    }
    S_StartAttackSound(pmo, sfx_sawhit);

    // turn to face target
    angle = R_PointToAngle2 (pmo->x, pmo->y,
                             lar_linetarget->x, lar_linetarget->y);
    if (angle - pmo->angle > ANG180)
    {
        if (angle - pmo->angle < -ANG90/20)
            pmo->angle = angle + ANG90/21;
        else
            pmo->angle -= ANG90/20;
    }
    else
    {
        if (angle - pmo->angle > ANG90/20)
            pmo->angle = angle - ANG90/21;
        else
            pmo->angle += ANG90/20;
    }
    pmo->flags |= MF_JUSTATTACKED;
}



//
// A_FireMissile : rocket launcher fires a rocket
//
void A_FireMissile ( player_t*     player,
                     pspdef_t*     psp )
{
#ifdef MBF21
    MBF21_subtract_ammo( player, 0xFFFF );  // use ammo_per_shot
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo] -= player->weaponinfo[player->readyweapon].ammo_per_shot;
#endif
    //added:16-02-98: added player arg3
    P_SpawnPlayerMissile (player->mo, MT_ROCKET);
}


//
// A_FireBFG
//
void A_FireBFG ( player_t*     player,
                 pspdef_t*     psp )
{
#ifdef MBF21
    MBF21_subtract_ammo( player, 0xFFFF );  // use ammo_per_shot
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo] -= player->weaponinfo[player->readyweapon].ammo_per_shot;
#endif
    //added:16-02-98:added player arg3
    P_SpawnPlayerMissile (player->mo, MT_BFG);
}



//
// A_FirePlasma
//
void A_FirePlasma ( player_t*     player,
                    pspdef_t*     psp )
{
#ifdef MBF21
    MBF21_subtract_ammo( player, 0xFFFF );  // use ammo_per_shot
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo] -= player->weaponinfo[player->readyweapon].ammo_per_shot;
#endif

    A_fire_flash_recoil( player, (PP_Random(pr_plasma)&1) );

    //added:16-02-98: added player arg3
    P_SpawnPlayerMissile (player->mo, MT_PLASMA);
}



//
// P_BulletSlope
// Sets a slope so a near miss is at approximately
// the height of the intended target
//
fixed_t         bulletslope;

//added:16-02-98: Fab comments: autoaim for the bullet-type weapons
void P_BulletSlope (mobj_t* mo)
{
    player_t*  ply = mo->player;
    byte    friend_protect;
    angle_t     an;

    //added:18-02-98: if AUTOAIM, try to aim at something
//    if(!ply->autoaim_toggle || !cv_allowautoaim.EV || demoversion<=111)  // [WDJ] breaks demos
    if( !(ply->GF_flags & GF_autoaim) || !cv_allowautoaim.EV )
        goto no_target_found;

    // Try first with friend_protect, then without friend_protect.
    for( friend_protect = EN_mbf; ;  )
    {
        // see which target is to be aimed at
        an = mo->angle;
        bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT, friend_protect);

        // lar_linetarget returned by P_AimLineAttack
        if( lar_linetarget )  break;
        an += 1<<26;
        bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT, friend_protect);
        if( lar_linetarget )  break;
        an -= 2<<26;
        bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT, friend_protect);
        if( lar_linetarget )  break;
        if( ! friend_protect )  goto no_target_found;
        // MBF, retry without the friend protect.
        friend_protect = 0;
    }
    return;

no_target_found:
    // Manual aiming
    if( EV_legacy >= 128 )
        bulletslope = AIMINGTOSLOPE(ply->aiming);
    else
        bulletslope = (ply->aiming<<FRACBITS)/160;
    return;
}


//
// P_GunShot
//
//added:16-02-98: used only for player (pistol,shotgun,chaingun)
//                supershotgun use p_lineattack directly
void P_GunShot ( mobj_t*       mo,
                 boolean       accurate )
{
    angle_t     angle;
    int         damage;

    damage = 5 * (PP_Random(pr_gunshot)%3+1);
    angle = mo->angle;

    if (!accurate)
    {
        // WARNING: don't put two P_Random on one line, ambiguous
        angle += (PP_SignedRandom(pr_misfire)<<18);
    }

    P_LineAttack (mo, angle, MISSILERANGE, bulletslope, damage);
}


//
// A_FirePistol
//
void A_FirePistol ( player_t*     player,
                    pspdef_t*     psp )
{
    mobj_t * pmo = player->mo;

    S_StartAttackSound(pmo, sfx_pistol);

    P_SetMobjState (pmo, S_PLAY_ATK2);
#ifdef MBF21
    MBF21_subtract_ammo( player, 1 );
//    MBF21_subtract_ammo( player, player->weaponinfo[player->readyweapon].ammo_per_shot );  // works too
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo]--;
#endif
    A_fire_flash_recoil( player, 0 );

    P_BulletSlope (pmo);
    P_GunShot (pmo, !player->refire);
}


//
// A_FireShotgun
//
void A_FireShotgun ( player_t*     player,
                     pspdef_t*     psp )
{
    mobj_t * pmo = player->mo;
    int         i;

    S_StartAttackSound(pmo, sfx_shotgn);
    P_SetMobjState (pmo, S_PLAY_ATK2);

#ifdef MBF21
    MBF21_subtract_ammo( player, 1 );
//    MBF21_subtract_ammo( player, player->weaponinfo[player->readyweapon].ammo_per_shot );  // works too
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo]--;
#endif
    A_fire_flash_recoil( player, 0 );

    P_BulletSlope (pmo);
    for (i=0 ; i<7 ; i++)
        P_GunShot (pmo, false);
}



//
// A_FireShotgun2 (SuperShotgun)
//
void A_FireShotgun2 ( player_t*     player,
                      pspdef_t*     psp )
{
    mobj_t * pmo = player->mo;
    int         i;
    angle_t     angle;
    int         damage, slope;

    S_StartAttackSound(pmo, sfx_dshtgn);
    P_SetMobjState (pmo, S_PLAY_ATK2);

#ifdef MBF21
    MBF21_subtract_ammo( player, 2 );
//    MBF21_subtract_ammo( player, player->weaponinfo[player->readyweapon].ammo_per_shot );  // works too
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo]-=2;
#endif
    A_fire_flash_recoil( player, 0 );

    P_BulletSlope (pmo);

    for (i=0 ; i<20 ; i++)
    {
        if( demoversion > 114 && demoversion < 144 )
        {
            // Old legacy order, slope, damage, angle
            slope = bulletslope + (PP_SignedRandom(pr_shotgun)<<5);
            damage = 5 * (PP_Random(pr_shotgun)%3+1);
            angle = pmo->angle + (PP_SignedRandom(pr_shotgun) << 19);
        }
        else
        {
            // [WDJ] Boom, P_Random order is damage, angle, slope
            damage = 5 * (PP_Random(pr_shotgun)%3+1);
            angle = pmo->angle + (PP_SignedRandom(pr_shotgun) << 19);
            slope = bulletslope + (PP_SignedRandom(pr_shotgun)<<5);
        }
        P_LineAttack (pmo, angle, MISSILERANGE, slope, damage);
    }
}


//
// A_FireCGun
//
void A_FireCGun ( player_t*     player,
                  pspdef_t*     psp )
{
    mobj_t * pmo = player->mo;

    S_StartAttackSound(pmo, sfx_pistol);

    if (!player->ammo[player->weaponinfo[player->readyweapon].ammo])
        return;

    P_SetMobjState (pmo, S_PLAY_ATK2);
#ifdef MBF21
    MBF21_subtract_ammo( player, 1 );
//    MBF21_subtract_ammo( player, player->weaponinfo[player->readyweapon].ammo_per_shot );  // works too
#else
    player->ammo[player->weaponinfo[player->readyweapon].ammo]--;
#endif
    A_fire_flash_recoil( player, psp->state - &states[S_CHAIN1] );

    P_BulletSlope (pmo);
    P_GunShot (pmo, !player->refire);
}



//
// Flash light when fire gun
//
void A_Light0 (player_t *player, pspdef_t *psp)
{
    player->extralight = 0;
}

void A_Light1 (player_t *player, pspdef_t *psp)
{
    player->extralight = 1*LIGHT_UNIT;
}

void A_Light2 (player_t *player, pspdef_t *psp)
{
    player->extralight = 2*LIGHT_UNIT;
}


//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray (mobj_t* mo)
{
    int     i, j;
    int     damage;
    angle_t an;
    mobj_t  *extrabfg;

    // offset angles from its attack angle
    for (i=0 ; i<40 ; i++)
    {
        an = mo->angle - ANG90/2 + ANG90/40*i;

        // mo->target is the originator (player) of the missile.
        // MBF: Make autoaiming prefer enemies.
        P_AimLineAttack (mo->target, an, 16*64*FRACUNIT, EN_mbf);  // friend protect
        if( EN_mbf && !lar_linetarget )
            P_AimLineAttack (mo->target, an, 16*64*FRACUNIT, 0);

        // lar_linetarget returned by P_AimLineAttack
        if (!lar_linetarget)
            continue;

        extrabfg = P_SpawnMobj (lar_linetarget->x, lar_linetarget->y,
                                lar_linetarget->z + (lar_linetarget->height>>2),
                                MT_EXTRABFG);
        extrabfg->target = mo->target;

        damage = 0;
        for (j=0;j<15;j++)
            damage += (PP_Random(pr_bfg)&7) + 1;

        //BP: use extrabfg as inflictor so we have the good death message
        P_DamageMobj (lar_linetarget, extrabfg, mo->target, damage);
    }
}


//
// A_BFGsound
//
void A_BFGsound ( player_t* player, pspdef_t* psp )
{
    S_StartAttackSound(player->mo, sfx_bfg);
}


#ifdef MBF21

// MBF21 Random functions
// These are also needed in p_enemy.c.

// These comments are from DSDA-Doom
// Outputs a random angle between (-spread, spread), as an int (because it can be negative).
//   spread: Maximum angle (degrees, in fixed point -- not BAM!)
signed_angle_t MBF21_P_Random_hitscan_angle( fixed_t spread )
{
    // FixedToAngle doesn't work for negative numbers,
    // so for convenience take just the absolute value.
    fixed_t spread_abs = (spread < 0) ? -spread : spread;
    int64_t spread_bam = FixedToAngle_EE(spread_abs);
    return (signed_angle_t)((spread_bam * PP_SignedRandom(pr_mbf21)) / 255);
}
 
// These comments are from DSDA-Doom
// Outputs a random angle between (-spread, spread), converted to values used for slope
//   spread: Maximum vertical angle (degrees, in fixed point -- not BAM!)
fixed_t  MBF21_P_Random_hitscan_slope( fixed_t spread )
{
    signed_angle_t s_angle = MBF21_P_Random_hitscan_angle( spread );
//    return  MBF21_fixed_deg_to_slope( s_angle );
    return  MBF21_angle_to_slope( s_angle );
}


// MBF21 functions on the player
// These comments are from DSDA-Doom
// A parameterized player weapon projectile attack. Does not consume ammo.
//   args[0]: Type of actor to spawn
//   args[1]: Angle (degrees, in fixed point), relative to calling player's angle
//   args[2]: Pitch (degrees, in fixed point), relative to calling player's pitch; approximated
//   args[3]: X/Y spawn offset, relative to calling player's angle
//   args[4]: Z spawn offset, relative to player's default projectile fire height
void A_WeaponProjectile_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    mobj_t * pmo = player->mo;
    state_ext_t * sep = P_state_ext( psp->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    {
        int p_type = sep->parm_args[0] - 1;
        if( p_type < 0 )
          return;

        signed_angle_t  p_angle = (((int64_t)sep->parm_args[1]) << FRACBITS) / 360;
        int p_pitch = sep->parm_args[2];
        int p_spawn_ofs_xy = sep->parm_args[3];
        int p_spawn_ofs_z = sep->parm_args[4];

        mobj_t * mo = P_SpawnPlayerMissile( pmo, p_type );
        if( ! mo )
            return;

        // adjust angle
        mo->angle += p_angle;
        // adjust momentum
#ifdef MONSTER_VARY
        mo->momx = FixedMul( mo->speed, cosine_ang( mo->angle ) );
        mo->momy = FixedMul( mo->speed, sine_ang( mo->angle ) );
#else
        mo->momx = FixedMul( mo->info->speed, cosine_ang( mo->angle ) );
        mo->momy = FixedMul( mo->info->speed, sine_ang( mo->angle ) );
#endif
        // adjust pitch
        // approximated using Doom finetangent table, same as in autoaim
#ifdef MONSTER_VARY
        mo->momz = FixedMul( mo->speed, MBF21_fixed_deg_to_slope(p_pitch) );
#else
        mo->momz = FixedMul( mo->info->speed, MBF21_fixed_deg_to_slope(p_pitch) );
#endif

        // adjust position to not be exactly on player
        angle_t pan = pmo->angle - ANG90;
        mo->x += FixedMul( p_spawn_ofs_xy, cosine_ang( pan ) );
        mo->y += FixedMul( p_spawn_ofs_xy, sine_ang( pan ) );
        mo->z += p_spawn_ofs_z;

        // Set tracer to the player autoaim target,
        // so a seeker missile will prioritize that monster.
//        P_SetTarget( &mo->tracer, lar_linetarget );
        SET_TARGET_REF( mo->tracer, lar_linetarget );
    }
}

// These comments are from DSDA-Doom
// A parameterized player weapon bullet attack. Does not consume ammo.
//   args[0]: Horizontal spread (degrees, in fixed point)
//   args[1]: Vertical spread (degrees, in fixed point)
//   args[2]: Number of bullets to fire; if not set, defaults to 1
//   args[3]: Base damage of attack (e.g. for 5d3, customize the 5); if not set, defaults to 5
//   args[4]: Attack damage modulus (e.g. for 5d3, customize the 3); if not set, defaults to 3
void A_WeaponBulletAttack_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    // No damage if default to 0.
    state_ext_t * sep = P_state_ext( psp->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    int hspread    = sep->parm_args[0];
    int vspread    = sep->parm_args[1];
    int numbullets = sep->parm_args[2];
    int damagebase = sep->parm_args[3];
//    if( damagebase == 0 )   damagebase = 5;
    int damagemod = sep->parm_args[4];
    if( damagemod == 0 )   damagemod = 3;  // otherwise will fault

    mobj_t * pmo = player->mo;
    P_BulletSlope(pmo);

    int i;
    for (i = 0; i < numbullets; i++)
    {
        int damage = (PP_Random(pr_mbf21) % damagemod + 1) * damagebase;
        signed_angle_t angle = (signed_angle_t)pmo->angle + MBF21_P_Random_hitscan_angle( hspread );
        fixed_t slope = bulletslope + MBF21_P_Random_hitscan_slope( vspread );
        P_LineAttack( pmo, angle, MISSILERANGE, slope, damage );
    }
}

// These comments are from DSDA-Doom
// A parameterized player weapon melee attack.
//   args[0]: Base damage of attack (e.g. for 2d10, customize the 2); if not set, defaults to 2
//   args[1]: Attack damage modulus (e.g. for 2d10, customize the 10); if not set, defaults to 10
//   args[2]: Berserk damage multiplier (fixed point); if not set, defaults to 1.0 (no change).
//   args[3]: Sound to play if attack hits
//   args[4]: Range (fixed point); if not set, defaults to player mobj's melee range
void A_WeaponMeleeAttack_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    // No damage if default to 0.
    state_ext_t * sep = P_state_ext( psp->state );
//    if( sep == &empty_state_ext )
//        return;  // no args

    mobj_t * pmo = player->mo;
    
    int damagebase = sep->parm_args[0];
//    if( damagebase == 0 )   damagebase = 2;
    int damagemod = sep->parm_args[1];
    if( damagemod == 0 )   damagemod = 10;  // otherwise will fault
    int bzkmult = sep->parm_args[2];
    int hitsound = sep->parm_args[3];

    int range = sep->parm_args[4];
    if( range == 0 )
      range = pmo->info->melee_range;

    int damage = (PP_Random(pr_mbf21) % damagemod + 1) * damagebase;
    if( bzkmult && player->powers[pw_strength] )
      damage = (damage * bzkmult) >> FRACBITS;

    // slight randomization
    angle_t angle = pmo->angle + (PP_SignedRandom(pr_mbf21)<<18);

    // make autoaim prefer enemies
    int slope = P_AimLineAttack(pmo, angle, range, 1); // friend protection
    if( ! lar_linetarget )
      slope = P_AimLineAttack(pmo, angle, range, 0);

    // attack
    P_LineAttack(pmo, angle, range, slope, damage);

    // missed
    if( ! lar_linetarget)
      return;

    // hit sound
    S_StartMobjSound(pmo, hitsound);

    // turn to face target
    pmo->angle = R_PointToAngle2(pmo->x, pmo->y, lar_linetarget->x, lar_linetarget->y);
#ifdef DSDA_XXX
    // [WDJ] This resets something internal to DSDA-Doom.
    R_SmoothPlaying_Reset(player);
#endif
}

// These comments are from DSDA-Doom
// Plays a sound. Usable from weapons, unlike A_PlaySound
//   args[0]: ID of sound to play
//   args[1]: If 1, play sound at full volume (may be useful in DM?)
void A_WeaponSound_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    // Require args. Do not allow default 0 sound.
    state_ext_t * sep = P_state_ext( psp->state );
    if( sep == &empty_state_ext )
        return;  // no args

    mobj_t * pmo = player->mo;
    S_StartMobjSound( sep->parm_args[1] ? NULL : pmo, sep->parm_args[0] );
}

// These comments are from DSDA-Doom
// Alerts monsters to the player's presence. Handy when combined with WPF_SILENT.
void A_WeaponAlert_MBF21(player_t * player, pspdef_t * psp)
{
    if( !EN_mbf21_action || !psp->state )
        return;

    // Does not use args.

    mobj_t * pmo = player->mo;
    P_NoiseAlert( pmo, pmo );
}

// These comments are from DSDA-Doom
// Jumps to the specified state, with variable random chance.
// Basically the same as A_RandomJump, but for weapons.
//   args[0]: State number
//   args[1]: Chance, out of 255, to make the jump
void A_WeaponJump_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    // Args are required.
    // Arg[0] is a state to jump to.
    state_ext_t * sep = P_state_ext( psp->state );
    if( sep == &empty_state_ext )
        return;  // no args

    if( PP_Random(pr_mbf21) < sep->parm_args[1] )
      P_SetPsprite_psp( player, psp, sep->parm_args[0] );
}

// These comments are from DSDA-Doom
// Subtracts ammo from the player's "inventory". 'Nuff said.
//   args[0]: Amount of ammo to consume. If zero, use the weapon's ammo-per-shot amount.
void A_ConsumeAmmo_MBF21( player_t* player, pspdef_t* psp)
{
    // DSDA allows psp->state == NULL. We cannot do that.
    if( !EN_mbf21_action || !psp->state )
        return;

    // Has defaults for 0 args.
    // sleepwalking.wad calls this with no args, so that must be valid.
    state_ext_t * sep = P_state_ext( psp->state );

    byte  plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo = & player->weaponinfo[plwpn];
    ammotype_t p_type = plwpninfo->ammo;
    if( p_type == am_noammo )
        return;

#if 0
    // [WDJ] DoomLegacy does not have infinite ammo cheat.
    if( player->cheats & CF_INFINITE_AMMO )
        return;
#endif

    int p_ammo_used = sep->parm_args[0];
    // Default, use the weapon's ammo-per-shot amount.
    // Cannot subtract zero ammo using this function.
    if( p_ammo_used == 0 )
      p_ammo_used = plwpninfo->ammo_per_shot;

    // subtract ammo, but don't let it get below zero
    int cur_ammo = player->ammo[p_type];
    cur_ammo -= p_ammo_used; // may be neg
    player->ammo[p_type] = (cur_ammo < 0)? 0 : cur_ammo;
}

// These comments are from DSDA-Doom
// Jumps to a state if the player's ammo is lower than the specified amount.
//   args[0]: State to jump to
//   args[1]: Minimum required ammo to NOT jump. If zero, use the weapon's ammo-per-shot amount.
void A_CheckAmmo_MBF21( player_t* player, pspdef_t* psp )
{
    // DSDA allows psp->state == NULL. We cannot do that.
    if( !EN_mbf21_action || !psp->state )
        return;

    // Args Required.
    // Has defaults for 0 args, but will goto new state.
    state_ext_t * sep = P_state_ext( psp->state );
    if( sep == &empty_state_ext )
        return;  // no args

    byte  plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo = & player->weaponinfo[plwpn];
    ammotype_t p_type = plwpninfo->ammo;
    if( p_type == am_noammo )
      return;

    int p_ammo_req = sep->parm_args[1];
    // Default, use the weapon's ammo-per-shot amount.
    // Cannot check for zero ammo using this function.
    if( p_ammo_req == 0 )
      p_ammo_req = plwpninfo->ammo_per_shot;

    if( player->ammo[p_type] < p_ammo_req )
      P_SetPsprite_psp( player, psp, sep->parm_args[0] ); // to new state
}

// These comments are from DSDA-Doom
// Jumps to a state if the player is holding down the fire button
//   args[0]: State to jump to
//   args[1]: If nonzero, skip the ammo check
void A_RefireTo_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    // Args Required.
    // Will goto new state.
    state_ext_t * sep = P_state_ext( psp->state );
    if( sep == &empty_state_ext )
        return;

    if( (sep->parm_args[1] || P_CheckAmmo(player))
        && (player->cmd.buttons & BT_ATTACK)
        && (player->pendingweapon == wp_nochange && player->health) )
      P_SetPsprite_psp(player, psp, sep->parm_args[0]); // to new state
}

// These comments are from DSDA-Doom
// Sets the weapon flash layer to the specified state.
//   args[0]: State number
//   args[1]: If nonzero, don't change the player actor state
void A_GunFlashTo_MBF21( player_t* player, pspdef_t* psp )
{
    if( !EN_mbf21_action || !psp->state )
        return;

    state_ext_t * sep = P_state_ext( psp->state );
    if( sep == &empty_state_ext )
        return;

    if( ! sep->parm_args[1] )  // disable actor state change
    {
        P_SetMobjState( player->mo, S_PLAY_ATK2 );
    }

    P_SetPsprite( player, PS_flash, sep->parm_args[0] );
}

#endif

//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites (player_t* player)
{
    int i;

    // remove all psprites
    for (i=0 ; i<NUMPSPRITES ; i++)
        player->psprites[i].state = NULL;

    // spawn the gun
    player->pendingweapon = player->readyweapon;
    P_BringUpWeapon (player);
}




//
// P_MovePsprites
// Called every tic by player thinking routine.
//
void P_MovePsprites (player_t* player)
{
    int         i;
    pspdef_t*   psp;

    psp = &player->psprites[0];
    for (i=0 ; i<NUMPSPRITES ; i++, psp++)
    {
        // a null state means not active
        state_t * state = psp->state;
        if( state )
        {
            // drop tic count and possibly change state

            // a -1 tic count never changes
            if (psp->tics != -1)
            {
                psp->tics--;
                if (!psp->tics)
                    P_SetPsprite (player, i, state->nextstate);
            }
        }
    }

    player->psprites[PS_flash].sx = player->psprites[PS_weapon].sx;
    player->psprites[PS_flash].sy = player->psprites[PS_weapon].sy;
}


#include "p_hpspr.c"

