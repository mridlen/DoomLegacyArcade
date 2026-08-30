// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_inter.c 1740 2025-03-23 04:28:03Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: p_inter.c,v $
// Revision 1.30  2004/09/12 19:40:07  darkwolf95
// additional chex quest 1 support
//
// Revision 1.29  2002/11/30 18:39:58  judgecutor
// * Fix CR+LF problem
// * Fix FFW bug (player spawining istead of weapon)
//
// Revision 1.28  2002/09/27 16:40:09  tonyd
// First commit of acbot
//
// Revision 1.27  2002/08/24 22:42:03  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.26  2002/07/26 15:21:36  hurdler
//
// Revision 1.25  2002/07/23 15:07:11  mysterial
// Messages to second player appear on his half of the screen
//
// Revision 1.24  2002/01/21 23:14:28  judgecutor
// Frag's Weapon Falling fixes
//
// Revision 1.23  2001/12/26 22:46:01  hurdler
// Revision 1.22  2001/12/26 22:42:52  hurdler
// Revision 1.18  2001/06/10 21:16:01  bpereira
// Revision 1.17  2001/05/27 13:42:47  bpereira
// Revision 1.16  2001/05/16 21:21:14  bpereira
//
// Revision 1.15  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.14  2001/04/19 05:51:47  metzgermeister
// fixed 10 shells instead of 4 - bug
//
// Revision 1.13  2001/03/30 17:12:50  bpereira
// Revision 1.12  2001/02/24 13:35:20  bpereira
//
// Revision 1.11  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.10  2000/11/02 17:50:07  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.9  2000/10/02 18:25:45  bpereira
// Revision 1.8  2000/10/01 10:18:17  bpereira
// Revision 1.7  2000/09/28 20:57:16  bpereira
// Revision 1.6  2000/08/31 14:30:55  bpereira
// Revision 1.5  2000/04/16 18:38:07  bpereira
//
// Revision 1.4  2000/04/04 00:32:46  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.3  2000/02/27 00:42:10  hurdler
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // class-list
#include "p_inter.h"
#include "g_game.h"
#include "g_input.h"
  // cv_allowrocketjump
#include "i_system.h"
  //I_Tactile currently has no effect
#include "am_map.h"
#include "dstrings.h"
#include "m_random.h"
#include "s_sound.h"
#include "r_main.h"
#include "st_stuff.h"
#include "infoext.h"
#include "hs_stuff.h"   // [Arcade] HS_Player_Died
#include "au_stuff.h"   // [Arcade] AU_Player_Death

#define BONUSADD        6
#define PICKUP_FLASH_TICS     10


#ifdef MAPTHING_ADJUST
// [WDJ] Experimental, details may change.
// [WDJ] Controls for varying the pickup amounts, and monster toughness.
// Can use it to make a overly difficult map playable,
// or to make a map tougher when you are feeling cocky.

# ifdef MAPTHING_ADJUST_MASTER
// DO NOT USE
// [WDJ] 2022, map adjust master disable
// This only controlled 4 or 5 things, so is not worth it.  Added too much complexity too.
// Only saved because it may be base for something that has farther reaching control, that would be more useful.
// Can disable other cv vars easier by altering their EV values in OnChange, instead of putting more inline tests.
// Save the EV values in demo and savegame.  Do not save the master disable in demos, unless it has other effects.

CV_PossibleValue_t mapthing_adjust_master_cons_t[]={
   {0,"individual"},  // default
      // because some controls existed long before this map adjust control,
      // and they MUST default to enabled.
   {1,"disabled"},
   // future ideas
//   {2,"vanilla"},
//   {3,"doom1"},
//   {4,"doom2"},
//   {5,"MBF"},
//   {9,"Heretic"},
   {0,NULL}};

void  CV_mapthing_adjust_master_OnChange( void );
void  M_mapthing_adjust_master_menu_setup( void );
void  DoorDelay_OnChange( void );

consvar_t cv_mapthing_adjust_master =
  {"mapthingadjustmaster","0", CV_NETVAR | CV_SAVE | CV_CALL, mapthing_adjust_master_cons_t, CV_mapthing_adjust_master_OnChange };

void  CV_mapthing_adjust_master_OnChange( void )
{
    M_mapthing_adjust_master_menu_setup();
    DoorDelay_OnChange();
}
# endif


CV_PossibleValue_t variation_values_cons_t[]={
   {0,"normal"},
   // delta in percent
   {1,"-40"},
   {2,"-20"},
   {3,"+20"},
   {4,"+40"},
   {5,"+80"},
   {6,"+100"},
   {7,"+140"},
   {8,"+180"},
   {9,"+220"},
   // random increase percent
   {10,"rand+10"},
   {11,"rand+20"},
   // random increase/decrease percent
   {12,"rand10"},   // +/- 10 percent
   {13,"rand20"},
   {0,NULL}};

consvar_t cv_monster_health =
  {"monsterhealth","0", CV_NETVAR | CV_SAVE, variation_values_cons_t };
consvar_t cv_health_pickup =
  {"healthpickup","0", CV_NETVAR | CV_SAVE, variation_values_cons_t };
consvar_t cv_armor_pickup =
  {"armorpickup","0", CV_NETVAR | CV_SAVE, variation_values_cons_t };
consvar_t cv_ammo_pickup =
  {"ammopickup","0", CV_NETVAR | CV_SAVE, variation_values_cons_t };


int8_t  mapthing_adjust_table[] = 
{
  0,   // normal
  -40 / 5, // -40
  -20 / 5, // -20
  20 / 5,  // +20
  40 / 5,  // +40
  80 / 5,  // +80
  100 / 5, // +100
  140 / 5, // +140
  180 / 5, // +180
  220 / 5, // +220
  10 / 5,  // rand+10
  20 / 5,  // rand+20
  20 / 5,  // rand10
  40 / 5,  // rand20
};

//  ev : a mapthing_adjust cons EV
//  value : the current value of the attribute
// Return the new value.
int  mapthing_adjust( byte ev, int value )
{
# ifdef MAPTHING_ADJUST_MASTER
    if( ( cv_mapthing_adjust.EV == 0 ) && ( ev > 0 ) )
# else
    if( ev > 0 )
# endif
    {
        // Vary the attribute value.
        int delta = (value * mapthing_adjust_table[ ev ]) / (100/5); // * 5 / 100
        if( ev >= 10 )
        {
            // Random variations
            byte r = PP_Random(pL_mapvariation);
            int  ri = ( ev >= 12 )? (int)((int8_t)r) : (int)(r);
            delta = (delta * ri) >> 8;
        }
        return value + delta;
    }

    return value;
}

#endif

#ifdef MBF21
static byte EN_remove_dead_thinker;

void DemoAdapt_p_inter( void )
{
    EN_remove_dead_thinker = EN_mbf21;
//      || ( EN_mbf && !prboom_comp[remove_dead_thinker]
}
#endif

// -------------

// a weapon is found with two clip loads,
// a big item has five clip loads
// clip, shell, cell, missile
uint16_t   maxammo[NUMAMMO] = {200, 50, 300, 50};
uint16_t   clipammo[NUMAMMO] = {10, 4, 20, 1};

// [Arcade] Default On: killing the player holding the rocket launcher should
// leave it for whoever did it, which is how modern arena shooters behave and
// what makes a cabinet deathmatch flow.  Only fires when a *player* dies
// (target_player), so single play is unaffected in practice -- a death there
// ends the run and reloads the level.
consvar_t cv_fragsweaponfalling
  = {"fragsweaponfalling"   ,"1", CV_NETVAR, CV_YesNo};

// added 4-2-98 (Boris) for dehacked patch
// (i don't like that but do you see another solution ?)
int     MAXHEALTH= 100;

//--------------------------------------------------------------------------
//
// PROC P_SetMessage
//
//--------------------------------------------------------------------------

// [WDJ] Replaces the Heretic ultimatemsg flag for IDKFA and IDDQD.
// That made the IDKFA and IDDQD message block most other messages,
// until the next player tic.
// But Legacy message timing is different and separate.
// A user control limits the messages seen based on the message level,
// (Off, minimal, normal, verbose, debug).
// This also uses the new message level as priority for competing messages
// that are issued in the same tic.

// Player Message for cheats and object interactions.
// Subject to message level control.
// This does not attempt to duplicate Heretic message blocking identically,
// as it has finer control.
// msglevel:
//   0..9    debug
//   10..19  verbose
//   20..29  normal play
//   30..39  minimal msg play
//   40..49  major
//   50..59  god mode messages
//   60..64  mandatory (even when messages are off)
void P_SetMessage(player_t *player, char *message, byte msglevel)
{
    // This actually optimizes cheaper than a table.
    switch( cv_showmessages.EV )
    {
     case 5: // dev level
     case 4: // debug level
        break;
     case 3: // verbose level
        if( msglevel < 10 )  return;
        break;
     case 2: // play level
        if( msglevel < 20 )  return;
        break;
     case 1: // minimal msg play level
        if( msglevel < 30 )  return;
        break;
     default: // off
        // Does not block mandatory messages.
        if( msglevel < 60 )  return;
        break;
    }

    // Check competing messages in same tic.
    // Per player, to support splitplayer.
    if( player->msglevel > msglevel )
        return;

    player->msglevel = msglevel;
    player->message = message;
    //player->messageTics = MESSAGETICS;
    //BorderTopRefresh = true;
}

//
// GET STUFF
//

// added by Boris : preferred weapons order
void VerifFavoritWeapon (player_t *player)
{
    int newweapon;

    if (player->pendingweapon != wp_nochange)
        return;

    newweapon = FindBestWeapon(player);

    if (newweapon != player->readyweapon)
        player->pendingweapon = newweapon;
}

int FindBestWeapon(player_t *player)
{
    int actualprior, actualweapon = 0, i;

    actualprior = -1;

    for (i = 0; i < NUMWEAPONS; i++)
    {
        // skip super shotgun for non-Doom2
        if (gamemode!=doom2_commercial && i==wp_supershotgun)
            continue;
        // skip plasma-bfg in shareware
        if (gamemode==doom_shareware && (i==wp_plasma || i==wp_bfg))
            continue;

        if (player->weaponowned[i] &&
            actualprior < player->favoritweapon[i] &&
            player->ammo[player->weaponinfo[i].ammo] >= player->weaponinfo[i].ammo_per_shot)
        {
            actualweapon = i;
            actualprior = player->favoritweapon[i];
         }
    }
    
    return actualweapon;
}

static const weapontype_t GetAmmoChange[] =
{
        wp_goldwand,
        wp_crossbow,
        wp_blaster,
        wp_skullrod,
        wp_phoenixrod,
        wp_mace
};


#ifdef MBF21
// [WDJ] MBF21, derived from DSDA-Doom
static
boolean P_GiveAmmo_autoswitch( player_t* player, ammotype_t ammo_type, uint16_t ammo_count_old )
{
    int plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo = & player->weaponinfo[ plwpn ];
    if( (plwpninfo->flags & WPF_AUTOSWITCH_FROM)  // current weapon allows autoswitch away
        && (plwpninfo->ammo != ammo_type) )  // picked up ammo that is not for this weapon
    {
        // Check weapons, high to low order, higher index than current, that can switch to.
        int i;
        for ( i = NUMWEAPONS-1; i > plwpn; --i )
        {
            if( player->weaponowned[i] )  // player has weapon
            {
                weaponinfo_t * wpninfo = & player->weaponinfo[i];
                if( !(wpninfo->flags & WPF_NO_AUTOSWITCH_TO)  // weapon allows autoswitch to
                    && (wpninfo->ammo == ammo_type)  // uses the ammo picked up
                    && (ammo_count_old < wpninfo->ammo_per_shot)  // did not have enough ammo before
                    && (wpninfo->ammo_per_shot <= player->ammo[ammo_type])  // now has enough ammo
                  )
                {
                    player->pendingweapon = i;
                    break;
                }
            }
        }
    }

    return true;
}
#endif


//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

static
boolean P_GiveAmmo ( player_t* player, ammotype_t ammo_type, uint16_t count )
{
    int plwpn = player->readyweapon;
    weaponinfo_t * plwpninfo;
    uint16_t oldammo, newammo;

    if (ammo_type == am_noammo)
        return false;

    if ( (unsigned)ammo_type > NUMAMMO)
    {
        GenPrintf(EMSG_warn, "\2P_GiveAmmo: bad type %i", ammo_type);
        return false;
    }

    oldammo = player->ammo[ammo_type];
    if ( oldammo == player->maxammo[ammo_type]  )
        return false;

    plwpninfo = & player->weaponinfo[ plwpn ];

/*
    if (num)
        num *= clipammo[ammo_type];
    else
        num = clipammo[ammo_type]/2;
*/
    if (gameskill == sk_baby
        || gameskill == sk_nightmare)
    {
        if( EN_heretic )
            count += count>>1;  // add 50% more
        else
        {
            // give double ammo in trainer mode,
            // you'll need in nightmare
            count <<= 1;  // add 100% more
        }
    }

#ifdef MAPTHING_ADJUST
    if( count > 1 )  // only adjust amount in ammo collections
        count = mapthing_adjust( cv_ammo_pickup.EV, count );
#endif

    newammo = oldammo + count;

    if (newammo > player->maxammo[ammo_type])
        newammo = player->maxammo[ammo_type];

    player->ammo[ammo_type] = newammo;

    if( newammo > oldammo )
    {
        // ammo pickup screen indication
        if( ammo_type == plwpninfo->ammo )
        {
            player->ammo_pickup = PICKUP_FLASH_TICS; // ammo for current weapon
        }
        else
        {
            player->ammo_pickup += PICKUP_FLASH_TICS/2;  // some other ammo
            if( player->ammo_pickup > 35 )
                player->ammo_pickup = 35;
        }
    }

#ifdef MBF21
    if( EN_mbf21 )
        return P_GiveAmmo_autoswitch( player, ammo_type, oldammo );
#endif

    // If non zero ammo,
    // don't change up weapons,
    // player was lower on purpose.
    if (oldammo)
        return true;

    // We were down to zero,
    // so select a new weapon.
    // Preferences are not user selectable.

    // Boris hack for preferred weapons order...
    if( ! (player->GF_flags & GF_original_weapon) )
    {
        if( player->ammo[ plwpninfo->ammo ] < plwpninfo->ammo_per_shot )
          VerifFavoritWeapon(player);
        return true;
    }
    else //eof Boris
    if( EN_heretic )
    {
        if( ( plwpn == wp_staff
              || plwpn == wp_gauntlets) 
           && player->weaponowned[GetAmmoChange[ammo_type]]  )
             player->pendingweapon = GetAmmoChange[ammo_type];
    }
    else
    switch (ammo_type)
    {
      case am_clip:
        if( plwpn == wp_fist )
        {
            if (player->weaponowned[wp_chaingun])
                player->pendingweapon = wp_chaingun;
            else
                player->pendingweapon = wp_pistol;
        }
        break;

      case am_shell:
        if( plwpn == wp_fist
            || plwpn == wp_pistol )
        {
            if (player->weaponowned[wp_shotgun])
                player->pendingweapon = wp_shotgun;
        }
        break;

      case am_cell:
        if( plwpn == wp_fist
            || plwpn == wp_pistol )
        {
            if (player->weaponowned[wp_plasma])
                player->pendingweapon = wp_plasma;
        }
        break;

      case am_misl:
        if( plwpn == wp_fist )
        {
            if (player->weaponowned[wp_missile])
                player->pendingweapon = wp_missile;
        }
      default:
        break;
    }

    return true;
}

// ammo get with the weapon
uint16_t WeaponAmmo_pickup[NUMWEAPONS] =
{
    0,  // staff       fist
   20,  // gold wand   pistol
    8,  // crossbow    shotgun
   20,  // blaster     chaingun
    2,  // skull rod   missile    
   40,  // phoenix rod plasma     
   40,  // mace        bfg        
    0,  // gauntlets   chainsaw   
    8,  // beak        supershotgun
};

// Ammo drop from P_TouchSpecialThing
// [WDJ] This used to use -1 for drop of 0 ammo, which was
// incomprehensible in the code, and required special test to eliminate.
// Changed to detect noammo, separate from ammo=0.
// Dropped ammo is so that can test for it even when drop is 0,
// as otherwise other assumptions will be made, and other ammo will be wrongly applied.
// This eliminates the need for negative, so can use uint16_t for ammo.
// This test is only done in one place, and is not worth other code complications.
// Unfortunately, this is in saved games too, so cannot change it without changing
// the savegame format.
#define TST_NOAMMO    0
#define TST_AMMO_0    0xFFFF
static uint16_t TST_has_ammo_dropped = TST_NOAMMO;

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
static
boolean P_GiveWeapon ( player_t*     player,
                       weapontype_t  weapon,
                       boolean       dropped )
{
    boolean     gaveammo = false;
    boolean     gaveweapon = false;
    int         ammo_count;
    int  plwpn = player->readyweapon;
    byte  weapon_ammo_type = player->weaponinfo[weapon].ammo;

    // [WDJ] Orig: (cv_deathmatch != 2)
    if( multiplayer && weapon_persist && !dropped )
    {
        // Deathmatch 1 and 3, map placed weapons persist.
        // Each player can pick up each weapon type only once.

        // leave placed weapons forever on net games
        if (player->weaponowned[weapon])
            return false;

        player->bonuscount += BONUSADD;
        player->weapon_pickup = PICKUP_FLASH_TICS;
        player->weaponowned[weapon] = true;

        ammo_count = deathmatch?  5*clipammo[weapon_ammo_type]
          : WeaponAmmo_pickup[weapon];  // coop
        P_GiveAmmo( player, weapon_ammo_type, ammo_count );

        // Boris hack preferred weapons order...
        if( (player->GF_flags & GF_original_weapon)
          || player->favoritweapon[weapon] > player->favoritweapon[plwpn] )
            player->pendingweapon = weapon;     // do like Doom2 original

        //added:16-01-98:changed consoleplayer to displayplayer
        //               (hear the sounds from the viewpoint)
        if (player == displayplayer_ptr
            || (cv_splitscreen.EV && player == displayplayer2_ptr))  // NULL when unused
            S_StartSound(sfx_wpnup);

        return false;
    }

    if( weapon_ammo_type != am_noammo )
    {
        // give one clip with a dropped weapon,
        // two clips with a found weapon
        if (dropped)
        {
            // TST_has_ammo_dropped is from PIT_CheckThing checking things for ammo.
            // [WDJ] Changed TST_has_ammo_dropped to uint16_t.
            ammo_count = (TST_has_ammo_dropped == TST_NOAMMO) ? clipammo[ weapon_ammo_type ]
              : ((TST_has_ammo_dropped == TST_AMMO_0)? 0 : TST_has_ammo_dropped);
        }
        else
        {
            ammo_count = WeaponAmmo_pickup[weapon];
        }
        gaveammo = P_GiveAmmo( player, weapon_ammo_type, ammo_count );
    }

    if( ! player->weaponowned[weapon] )
    {
        gaveweapon = true;
        player->weaponowned[weapon] = true;
        player->weapon_pickup = PICKUP_FLASH_TICS;
        if( (player->GF_flags & GF_original_weapon)
          || player->favoritweapon[weapon] > player->favoritweapon[plwpn] )
            player->pendingweapon = weapon;    // Doom2 original stuff
    }

    return (gaveweapon || gaveammo);
}



//
// P_GiveHealth  (P_GiveBody)
// Returns false if the health isn't needed at all
//
boolean P_GiveHealth ( player_t*     player,
                       int           num )
{
    int max;
    
    max = MAXHEALTH;
    if(player->chickenTics)
        max = MAXCHICKENHEALTH;

    if (player->health >= max)
        return false;

#ifdef MAPTHING_ADJUST
    num = mapthing_adjust( cv_health_pickup.EV, num );
#endif
    player->health += num;
    if (player->health > max)
        player->health = max;
    player->mo->health = player->health;

    player->health_pickup = PICKUP_FLASH_TICS;
    return true;
}



//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
static
boolean P_GiveArmor ( player_t*     player,
                      int           armortype )
{
    int         hits;

    hits = armortype*100;
#ifdef MAPTHING_ADJUST
    hits = mapthing_adjust( cv_armor_pickup.EV, hits );
#endif
    if (player->armorpoints >= hits)
        return false;   // don't pick up

    player->armortype = armortype;
    player->armorpoints = hits;

    player->armor_pickup = PICKUP_FLASH_TICS;
    return true;
}



//
// P_GiveCard
//
static
boolean P_GiveCard ( player_t*     player,
                     card_t        card )
{
    if (player->cards & card )
        return false;

    player->cards |= card;
    player->bonuscount = BONUSADD;
    player->key_pickup = PICKUP_FLASH_TICS;
    return true;
}


//
// P_GivePower
//
//  power : powertype index
boolean P_GivePower ( player_t* player, int power )
{
    int plypower = player->powers[power];
    int power_tics = 1; // default

    // Was using inventory to detect heretic and hexen rules,
    // but these rules have nothing to do with inventory.
    if (power == pw_invulnerability)
    {
        // Already have it
        if( EN_heretic_hexen && (plypower > BLINKTHRESHOLD) )
            return false;

#ifdef HEXEN
        if( EN_hexen )
        {
            player->mo->flags3 |= (player->pclass == PCLASS_MAGE)?
                (MF3_REFLECTIVE | MF3_INVULNERABLE);  // Mage
                : MF3_INVULNERABLE; // other
        }
#endif

        power_tics = INVULNTICS;
        goto set_power;
    }
    if(power == pw_weaponlevel2)
    {
        // Already have it
        if( EN_heretic_hexen && (plypower > BLINKTHRESHOLD) )
            return false;

        player->weaponinfo = wpnlev2info;
        power_tics = WPNLEV2TICS;
        goto set_power;
    }
    if (power == pw_invisibility)
    {
        // Already have it
        if( EN_heretic_hexen && (plypower > BLINKTHRESHOLD) )
            return false;

        player->mo->flags |= MF_SHADOW;
        power_tics = INVISTICS;
        goto set_power;
    }
    if(power == pw_flight)
    {
        // Already have it
        if( plypower > BLINKTHRESHOLD )
            return false;

        player->mo->flags2 |= MF2_FLY;
        player->mo->flags |= MF_NOGRAVITY;
        if(player->mo->z <= player->mo->floorz)
        {
            player->flyheight = 10; // thrust the player in the air a bit
        }
        power_tics = FLIGHTTICS;
        goto set_power;
    }
    if (power == pw_infrared)
    {
        // Already have it
        if( plypower > BLINKTHRESHOLD )
            return false;

        power_tics = INFRATICS;
        goto set_power;
    }

    if (power == pw_ironfeet)
    {
        power_tics = IRONTICS;
        goto set_power;
    }

    if (power == pw_strength)
    {
        P_GiveHealth (player, 100);
        power_tics = 1;
        goto set_power;
    }

    if( plypower )
        return false;   // already got it

    power_tics = 1;

set_power:
    player->powers[power] = power_tics;
    return true;
}

// Boris stuff : dehacked patches hack
int max_armor=200;
int green_armor_class=1;
int blue_armor_class=2;
int max_soul_health=200;
int soul_health=100;
int mega_health=200;
// eof Boris

//---------------------------------------------------------------------------
//
// FUNC P_GiveArtifact
//
// Returns true if artifact accepted.
//
//---------------------------------------------------------------------------

boolean P_GiveArtifact(player_t *player, artitype_t arti, mobj_t *mo)
{
    int i;
    
    i = 0;
    while(player->inventory[i].type != arti && i < player->inventorySlotNum)
    {
        i++;
    }
    if(i == player->inventorySlotNum)
    {
        player->inventory[i].count = 1;
        player->inventory[i].type = arti;
        player->inventorySlotNum++;
    }
    else
    {
        if(player->inventory[i].count >= MAXARTECONT)
        { // Player already has 16 of this item
            return(false);
        }
        player->inventory[i].count++;
    }
    if( player->inventory[player->inv_ptr].count == 0 )
        player->inv_ptr = i;

    if(mo && (mo->flags&MF_COUNTITEM))
    {
        player->itemcount++;
    }
    return(true);
}


//---------------------------------------------------------------------------
//
// PROC P_SetDormantArtifact
//
// Removes the MF_SPECIAL flag, and initiates the artifact pickup
// animation.
//
//---------------------------------------------------------------------------

static
void P_SetDormantArtifact(mobj_t *arti)
{
    arti->flags &= ~MF_SPECIAL;
    if( deathmatch
        && (arti->type != MT_ARTIINVULNERABILITY )
        && (arti->type != MT_ARTIINVISIBILITY)   )
    {
        P_SetMobjState(arti, S_DORMANTARTI1);
    }
    else
    { // Don't respawn
        P_SetMobjState(arti, S_DEADARTI1);
    }
    S_StartObjSound(arti, sfx_artiup);
}

//---------------------------------------------------------------------------
//
// PROC A_RestoreArtifact
//
//---------------------------------------------------------------------------

void A_RestoreArtifact(mobj_t *arti)
{
        arti->flags |= MF_SPECIAL;
        P_SetMobjState(arti, arti->info->spawnstate);
        S_StartObjSound(arti, sfx_itmbk);
}

//----------------------------------------------------------------------------
//
// PROC P_HideSpecialThing
//
//----------------------------------------------------------------------------

void P_HideSpecialThing(mobj_t *thing)
{
        thing->flags &= ~MF_SPECIAL;
        thing->flags2 |= MF2_DONTDRAW;
        P_SetMobjState(thing, S_HIDESPECIAL1);
}

//---------------------------------------------------------------------------
//
// PROC A_RestoreSpecialThing1
//
// Make a special thing visible again.
//
//---------------------------------------------------------------------------

void A_RestoreSpecialThing1(mobj_t *thing)
{
        if(thing->type == MT_WMACE)
        { // Do random mace placement
                P_RepositionMace(thing);
        }
        thing->flags2 &= ~MF2_DONTDRAW;
        S_StartObjSound(thing, sfx_itmbk);
}

//---------------------------------------------------------------------------
//
// PROC A_RestoreSpecialThing2
//
//---------------------------------------------------------------------------

void A_RestoreSpecialThing2(mobj_t *thing)
{
        thing->flags |= MF_SPECIAL;
        P_SetMobjState(thing, thing->info->spawnstate);
}


//----------------------------------------------------------------------------
//
// PROC A_HideThing
//
//----------------------------------------------------------------------------

void A_HideThing(mobj_t *actor)
{
        //P_UnsetThingPosition(actor);
        actor->flags2 |= MF2_DONTDRAW;
}

//----------------------------------------------------------------------------
//
// PROC A_UnHideThing
//
//----------------------------------------------------------------------------

void A_UnHideThing(mobj_t *actor)
{
        //P_SetThingPosition(actor);
        actor->flags2 &= ~MF2_DONTDRAW;
}


//
// P_TouchSpecialThing
//
// called by PIT_CheckThing.
void P_TouchSpecialThing ( mobj_t*       special,
                           mobj_t*       toucher )
{                  
    player_t*   player;
    char *      msg = NULL;
    byte        msglevel = 20;  // normal play for common ammo
    spritenum_e group = 0xFFFF;  // combined handling by spritenum
    boolean     special_dropped;  // special item was dropped
    int         val, i;
    fixed_t     delta;
    int         sound;

    delta = special->z - toucher->z;

    //SoM: 3/27/2000: For some reason, the old code allowed the player to
    //grab items that were out of reach...
    if (delta > toucher->height
        || delta < -special->height)
    {
        // out of reach
        return;
    }

    // Dead thing touching.
    // Can happen with a sliding player corpse.
    if (toucher->health <= 0 || toucher->flags&MF_CORPSE)
        return;

    sound = sfx_itemup;
    player = toucher->player;

    if( player &&
        toucher != player->mo )  // voodoo doll toucher
    {
        if( voodoo_mode >= VM_target )
        {
            // Target last player to trigger a switch or linedef.
            if( spechit_player && spechit_player->mo )
            {
                player = spechit_player;
                toucher = player->mo;
            }
        }
        if( !player || !player->mo )
            return; // player left or voodoo multiplayer spawn
    }

#ifdef PARANOIA
    if( !player )
    {
        I_SoftError("P_TouchSpecialThing: without player\n");
        return;
    }
#endif


    // Frag Weapon Falling support
    // Dropped ammo available for pickup on the special thing.
    // [WDJ] TST_has_ammo_dropped changed for understandable logic, and uint16_t.
    TST_has_ammo_dropped = special->dropped_ammo_count;

    // Avoid multiple conversions to boolean.
    special_dropped = (boolean)(special->flags & MF_DROPPED);

    // Identify by sprite.
    switch (special->sprite)
    {
      case SPR_SHLD: // Item_Shield1
        // armor
      case SPR_ARM1:  // common armor
        val = green_armor_class;
        msg = GOTARMOR;
        msglevel = 28;
        goto get_armor;

      case SPR_SHD2: // Item_Shield2
      case SPR_ARM2:
        val = blue_armor_class;
        msg = GOTMEGA;
        msglevel = 37;

get_armor:
        if (!P_GiveArmor (player, val))
            return;
        break;

        // bonus items
      case SPR_BON1:  // common health inc
        val = player->health + 1;               // can go over 100%
        if (val > 2*MAXHEALTH)
            val = 2*MAXHEALTH;
        if((val > player->health) && (player->health_pickup < 35))
            player->health_pickup += PICKUP_FLASH_TICS/2;
        player->mo->health = player->health = val;
        msg = GOTHTHBONUS;
        msglevel = 22;
        break;

      case SPR_BON2:  // common armor inc
        val = player->armorpoints + 1;          // can go over 100%
        if (val > max_armor)
            val = max_armor;
        if((val > player->armorpoints) && (player->armor_pickup < 35))
            player->armor_pickup += PICKUP_FLASH_TICS/2;
        player->armorpoints = val;       
        if (!player->armortype)
            player->armortype = 1;
        msg = GOTARMBONUS;
        msglevel = 22;
        break;

      case SPR_SOUL:
#ifdef MAPTHING_ADJUST
        // [WDJ] vary bonus
        val = player->health + mapthing_adjust( cv_health_pickup.EV, soul_health );
#else
        val = player->health + soul_health;
#endif
        if (val > max_soul_health)
            val = max_soul_health;
        if((val > player->health) && (player->health_pickup < 35))
            player->health_pickup += PICKUP_FLASH_TICS;
        player->mo->health = player->health = val;
        msg = GOTSUPER;
        msglevel = 38;
        sound = sfx_getpow;
        break;

      case SPR_MEGA:
        if (gamemode != doom2_commercial)
            return;

#ifdef MAPTHING_ADJUST
        {
            // [WDJ] vary bonus
            int mh = mapthing_adjust( cv_health_pickup.EV, mega_health );
            if( mh < mega_health )
            {
                // less than full mega bonus, but mega bonus does not normally add to health
                val = player->health + mh;
                if( val > mega_health )
                    val = mega_health;
            }
            else
            {
                val = mega_health;  // most normal
            }
        }
#else
        val = mega_health;
#endif
        if(player->health_pickup < 35)
            player->health_pickup += PICKUP_FLASH_TICS;
        player->mo->health = player->health = val;
        P_GiveArmor (player,2);
        msg = GOTMSPHERE;
        msglevel = 38;
        sound = sfx_getpow;
        break;

        // cards
        // leave cards for everyone
      case SPR_BKYY: // Key_Blue
      case SPR_BKEY:
        group = SPR_BKEY;        
        if( P_GiveCard (player, it_bluecard) )
        {
            msg = GOTBLUECARD;
            msglevel = 45;
        }
        break;

      case SPR_CKYY: // Key_Yellow
      case SPR_YKEY:
        group = SPR_BKEY;        
        if( P_GiveCard (player, it_yellowcard) )
        {
            msg = GOTYELWCARD;
            msglevel = 45;
        }
        break;

      case SPR_AKYY: // Key_Green
      case SPR_RKEY:
        group = SPR_BKEY;        
        if (P_GiveCard (player, it_redcard))
        {
            msg = GOTREDCARD;
            msglevel = 45;
        }
        break;

      case SPR_BSKU:
        group = SPR_BKEY;        
        if (P_GiveCard (player, it_blueskull))
        {
            msg = GOTBLUESKUL;
            msglevel = 45;
        }
        break;

      case SPR_YSKU:
        group = SPR_BKEY;        
        if (P_GiveCard (player, it_yellowskull))
        {
            msg = GOTYELWSKUL;
            msglevel = 45;
        }
        break;

      case SPR_RSKU:
        group = SPR_BKEY;        
        if (P_GiveCard (player, it_redskull))
        {
            msg = GOTREDSKULL;
            msglevel = 45;
        }
        break;

        // medikits, heals
      case SPR_PTN1: // Item_HealingPotion
      case SPR_STIM:
        if (!P_GiveHealth (player, 10))
            return;
        msg = GOTSTIM;
        break;

      case SPR_MEDI:
        // [WDJ] fix medkit message
        // DoomWiki fix would put messages first, but that would give
        // message even when not using the medkit
        if (!P_GiveHealth (player, 25))  // add 25 to health
            return;
        // if health was used, then give message
        // use fix from prboom, thanks to Quasar
        if (player->health < 50) // old health was < 25, before adding 25
        {
            msg = GOTMEDINEED;
            msglevel = 31;
        }
        else
        {
            msg = GOTMEDIKIT;
            msglevel = 23;
        }
        break;

        // heretic Artifacts :
      case SPR_PTN2: // Arti_HealingPotion
          if(P_GiveArtifact(player, arti_health, special))
          {
              P_SetMessage(player, TXT_ARTIHEALTH, 28);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_SOAR: // Arti_Fly
          if(P_GiveArtifact(player, arti_fly, special))
          {
              P_SetMessage(player, TXT_ARTIFLY, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_INVU: // Arti_Invulnerability
          if(P_GiveArtifact(player, arti_invulnerability, special))
          {
              P_SetMessage(player, TXT_ARTIINVULNERABILITY, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_PWBK: // Arti_TomeOfPower
          if(P_GiveArtifact(player, arti_tomeofpower, special))
          {
              P_SetMessage(player, TXT_ARTITOMEOFPOWER, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_INVS: // Arti_Invisibility
          if(P_GiveArtifact(player, arti_invisibility, special))
          {
              P_SetMessage(player, TXT_ARTIINVISIBILITY, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_EGGC: // Arti_Egg
          if(P_GiveArtifact(player, arti_egg, special))
          {
              P_SetMessage(player, TXT_ARTIEGG, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_SPHL: // Arti_SuperHealth
          if(P_GiveArtifact(player, arti_superhealth, special))
          {
              P_SetMessage(player, TXT_ARTISUPERHEALTH, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_TRCH: // Arti_Torch
          if(P_GiveArtifact(player, arti_torch, special))
          {
              P_SetMessage(player, TXT_ARTITORCH, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_FBMB: // Arti_FireBomb
          if(P_GiveArtifact(player, arti_firebomb, special))
          {
              P_SetMessage(player, TXT_ARTIFIREBOMB, 31);
              P_SetDormantArtifact(special);
          }
          return;
      case SPR_ATLP: // Arti_Teleport
          if(P_GiveArtifact(player, arti_teleport, special))
          {
              P_SetMessage(player, TXT_ARTITELEPORT, 31);
              P_SetDormantArtifact(special);
          }
          return;

        // power ups
      case SPR_PINV:
        if (!P_GivePower (player, pw_invulnerability))
            return;
        msg = GOTINVUL;
        msglevel = 34;
        sound = sfx_getpow;
        break;

      case SPR_PSTR:
        if (!P_GivePower (player, pw_strength))
            return;
        msg = GOTBERSERK;
        msglevel = 34;
        if (player->readyweapon != wp_fist)
            player->pendingweapon = wp_fist;
        sound = sfx_getpow;
        break;

      case SPR_PINS:
        if (!P_GivePower (player, pw_invisibility))
            return;
        msg = GOTINVIS;
        msglevel = 34;
        sound = sfx_getpow;
        break;

      case SPR_SUIT:
        if (!P_GivePower (player, pw_ironfeet))
            return;
        msg = GOTSUIT;
        msglevel = 32;
        sound = sfx_getpow;
        break;

      case SPR_SPMP: // Item_SuperMap
      case SPR_PMAP:
        if (!P_GivePower (player, pw_allmap))
            return;
        msg = GOTMAP;
        msglevel = 31;
        if( EN_doom_etc )
            sound = sfx_getpow;
        break;

      case SPR_PVIS:
        if (!P_GivePower (player, pw_infrared))
            return;
        msg = GOTVISOR;
        msglevel = 32;
        sound = sfx_getpow;
        break;

        // heretic Ammo
      case SPR_AMG1: // Ammo_GoldWandWimpy
        if(!P_GiveAmmo(player, am_goldwand, special->health))
            return;
        msg = TXT_AMMOGOLDWAND1;
        break;
      case SPR_AMG2: // Ammo_GoldWandHefty
        if(!P_GiveAmmo(player, am_goldwand, special->health))
            return;
        msg = TXT_AMMOGOLDWAND2;
        break;
      case SPR_AMM1: // Ammo_MaceWimpy
        if(!P_GiveAmmo(player, am_mace, special->health))
            return;
        msg = TXT_AMMOMACE1;
        break;
      case SPR_AMM2: // Ammo_MaceHefty
        if(!P_GiveAmmo(player, am_mace, special->health))
            return;
        msg = TXT_AMMOMACE2;
        break;
      case SPR_AMC1: // Ammo_CrossbowWimpy
        if(!P_GiveAmmo(player, am_crossbow, special->health))
            return;
        msg = TXT_AMMOCROSSBOW1;
        break;
      case SPR_AMC2: // Ammo_CrossbowHefty
        if(!P_GiveAmmo(player, am_crossbow, special->health))
            return;
        msg = TXT_AMMOCROSSBOW2;
        break;
      case SPR_AMB1: // Ammo_BlasterWimpy
        if(!P_GiveAmmo(player, am_blaster, special->health))
            return;
        msg = TXT_AMMOBLASTER1;
        break;
      case SPR_AMB2: // Ammo_BlasterHefty
        if(!P_GiveAmmo(player, am_blaster, special->health))
            return;
        msg = TXT_AMMOBLASTER2;
        break;
      case SPR_AMS1: // Ammo_SkullRodWimpy
        if(!P_GiveAmmo(player, am_skullrod, special->health))
            return;
        msg = TXT_AMMOSKULLROD1;
        break;
      case SPR_AMS2: // Ammo_SkullRodHefty
        if(!P_GiveAmmo(player, am_skullrod, special->health))
            return;
        msg = TXT_AMMOSKULLROD2;
        break;
      case SPR_AMP1: // Ammo_PhoenixRodWimpy
        if(!P_GiveAmmo(player, am_phoenixrod, special->health))
            return;
        msg = TXT_AMMOPHOENIXROD1;
        break;
      case SPR_AMP2: // Ammo_PhoenixRodHefty
        if(!P_GiveAmmo(player, am_phoenixrod, special->health))
            return;
        msg = TXT_AMMOPHOENIXROD2;
        break;

        // ammo
      case SPR_CLIP:
        if (!P_GiveAmmo (player, am_clip,
               ((special_dropped)? clipammo[am_clip]/2 : clipammo[am_clip]) ))
            return;
        msg = GOTCLIP;
        break;

      case SPR_AMMO:
        if (!P_GiveAmmo (player, am_clip,5*clipammo[am_clip]))
            return;
        msg = GOTCLIPBOX;
        break;

      case SPR_ROCK:
        if (!P_GiveAmmo (player, am_misl,clipammo[am_misl]))
            return;
        msg = GOTROCKET;
        break;

      case SPR_BROK:
        if (!P_GiveAmmo (player, am_misl,5*clipammo[am_misl]))
            return;
        msg = GOTROCKBOX;
        break;

      case SPR_CELL:
        if (!P_GiveAmmo (player, am_cell,clipammo[am_cell]))
            return;
        msg = GOTCELL;
        break;

      case SPR_CELP:
        if (!P_GiveAmmo (player, am_cell,5*clipammo[am_cell]))
            return;
        msg = GOTCELLBOX;
        break;

      case SPR_SHEL:
        if (!P_GiveAmmo (player, am_shell,clipammo[am_shell]))
            return;
        msg = GOTSHELLS;
        break;

      case SPR_SBOX:
        if (!P_GiveAmmo (player, am_shell,5*clipammo[am_shell]))
            return;
        msg = GOTSHELLBOX;
        break;

      case SPR_BPAK:
        if(! (player->GF_flags & GF_backpack) )
        {
            for (i=0 ; i<NUMAMMO ; i++)
                player->maxammo[i] *= 2;
            player->GF_flags |= GF_backpack;
        }
        for (i=0 ; i<NUMAMMO ; i++)
            P_GiveAmmo (player, i, clipammo[i]);
        msg = GOTBACKPACK;
        msglevel = 27;
        break;

      case SPR_BAGH: // Item_BagOfHolding
        if(! (player->GF_flags & GF_backpack) )
        {
            for(i = 0; i < NUMAMMO; i++)
                player->maxammo[i] *= 2;
            player->GF_flags |= GF_backpack;
        }
        P_GiveAmmo(player, am_goldwand, AMMO_GWND_WIMPY);
        P_GiveAmmo(player, am_blaster, AMMO_BLSR_WIMPY);
        P_GiveAmmo(player, am_crossbow, AMMO_CBOW_WIMPY);
        P_GiveAmmo(player, am_skullrod, AMMO_SKRD_WIMPY);
        P_GiveAmmo(player, am_phoenixrod, AMMO_PHRD_WIMPY);
        msg = TXT_ITEMBAGOFHOLDING;
        msglevel = 27;
        break;

        // weapons
      case SPR_BFUG:
        if (!P_GiveWeapon (player, wp_bfg, special_dropped) )
            return;
        msg = GOTBFG9000;
        msglevel = 38;
        sound = sfx_wpnup;
        break;

      case SPR_MGUN:
        if (!P_GiveWeapon (player, wp_chaingun, special_dropped) )
            return;
        msg = GOTCHAINGUN;
        msglevel = 29;
        sound = sfx_wpnup;
        break;

      case SPR_CSAW:
        if (!P_GiveWeapon (player, wp_chainsaw, false) )
            return;
        msg = GOTCHAINSAW;
        msglevel = 21;
        sound = sfx_wpnup;
        break;

      case SPR_LAUN:
        if (!P_GiveWeapon (player, wp_missile, special_dropped) )
            return;
        msg = GOTLAUNCHER;
        msglevel = 32;
        sound = sfx_wpnup;
        break;

      case SPR_PLAS:
        if (!P_GiveWeapon (player, wp_plasma, special_dropped) )
            return;
        msg = GOTPLASMA;
        msglevel = 32;
        sound = sfx_wpnup;
        break;

      case SPR_SHOT:
        if (!P_GiveWeapon (player, wp_shotgun, special_dropped) )
            return;
        msg = GOTSHOTGUN;
        msglevel = 24;
        sound = sfx_wpnup;
        break;

      case SPR_SGN2:
        if (!P_GiveWeapon (player, wp_supershotgun, special_dropped) )
            return;
        msg = GOTSHOTGUN2;
        msglevel = 32;
        sound = sfx_wpnup;
        break;

      // heretic weapons
      case SPR_WMCE: // Weapon_Mace
        if(!P_GiveWeapon(player, wp_mace, special_dropped))
            return;
        msg = TXT_WPNMACE;
        msglevel = 32;
        sound = sfx_wpnup;
        break;
      case SPR_WBOW: // Weapon_Crossbow
        if(!P_GiveWeapon(player, wp_crossbow, special_dropped))
            return;
        msg = TXT_WPNCROSSBOW;
        msglevel = 24;
        sound = sfx_wpnup;
        break;
      case SPR_WBLS: // Weapon_Blaster
        if(!P_GiveWeapon(player, wp_blaster, special_dropped))
            return;
        msg = TXT_WPNBLASTER;
        msglevel = 32;
        sound = sfx_wpnup;
        break;
      case SPR_WSKL: // Weapon_SkullRod
        if(!P_GiveWeapon(player, wp_skullrod, special_dropped))
            return;
        msg = TXT_WPNSKULLROD;
        msglevel = 36;
        sound = sfx_wpnup;
        break;
      case SPR_WPHX: // Weapon_PhoenixRod
        if(!P_GiveWeapon(player, wp_phoenixrod, special_dropped))
            return;
        msg = TXT_WPNPHOENIXROD;
        msglevel = 38;
        sound = sfx_wpnup;
        break;
      case SPR_WGNT: // Weapon_Gauntlets
        if(!P_GiveWeapon(player, wp_gauntlets, false))
            return;
        msg = TXT_WPNGAUNTLETS;
        msglevel = 21;
        sound = sfx_wpnup;
        break;

      default:
        // SoM: New gettable things with FraggleScript!
        //debug_Printf ("\2P_TouchSpecialThing: Unknown gettable thing\n");
        return;
    }

    if( msg )
    {
        P_SetMessage( player, msg, msglevel );
    }
    if( group == SPR_BKEY )  // all keys
    {
        // keycard
        if( EN_heretic )
            sound = sfx_keyup;
        if (multiplayer)  return;  // leave keys in multiplayer
    }
   
    if (special->flags & MF_COUNTITEM)
        player->itemcount++;

    // [WDJ] Do not know when this got changed, or where.
    // Phobiata tests the health of things that got picked up (fragglescript).
    special->health = 0;

    P_RemoveMobj ( special );
    player->bonuscount += BONUSADD;

    //added:16-01-98:consoleplayer -> displayplayer (hear sounds from viewpoint)
    if (player == displayplayer_ptr
        || (cv_splitscreen.EV && player == displayplayer2_ptr))  // NULL when unused
        S_StartSound(sound);
}



#ifdef thatsbuggycode
//
//  Tell each supported thing to check again its position,
//  because the 'base' thing has vanished or diminished,
//  the supported things might fall.
//
//added:28-02-98:
void P_CheckSupportThings (mobj_t* mobj)
{
    fixed_t   supportz = mobj->z + mobj->height;

    while ((mobj = mobj->supportthings))
    {
        // only for things above support thing
        if (mobj->z > supportz)
            mobj->eflags |= MF_CHECKPOS;
    }
}


//
//  If a thing moves and supportthings,
//  move the supported things along.
//
//added:28-02-98:
void P_MoveSupportThings (mobj_t* mobj, fixed_t xmove, fixed_t ymove, fixed_t zmove)
{
    fixed_t   supportz = mobj->z + mobj->height;
    mobj_t    *mo = mobj->supportthings;

    while (mo)
    {
        //added:28-02-98:debug
        if (mo==mobj)
        {
            mobj->supportthings = NULL;
            break;
        }

        // only for things above support thing
        if (mobj->z > supportz)
        {
            mobj->eflags |= MF_CHECKPOS;
            mobj->momx += xmove;
            mobj->momy += ymove;
            mobj->momz += zmove;
        }

        mo = mo->supportthings;
    }
}


//
//  Link a thing to it's 'base' (supporting) thing.
//  When the supporting thing will move or change size,
//  the supported will then be aware.
//
//added:28-02-98:
void P_LinkFloorThing(mobj_t*   mobj)
{
    mobj_t*     mo;
    mobj_t*     nmo;

    // no supporting thing
    if (!(mo = mobj->floorthing))
        return;

    // link mobj 'above' the lower mobjs, so that lower supporting
    // mobjs act upon this mobj
    while ( (nmo = mo->supportthings) &&
            (nmo->z<=mobj->z) )
    {
        // dont link multiple times
        if (nmo==mobj)
            return;

        mo = nmo;
    }
    mo->supportthings = mobj;
    mobj->supportthings = nmo;
}


//
//  Unlink a thing from it's support,
//  when it's 'floorthing' has changed,
//  before linking with the new 'floorthing'.
//
//added:28-02-98:
void P_UnlinkFloorThing(mobj_t*   mobj)
{
    mobj_t*     mo;

    if (!(mo = mobj->floorthing))      // just to be sure (may happen)
       return;

    while (mo->supportthings)
    {
        if (mo->supportthings == mobj)
        {
            mo->supportthings = NULL;
            break;
        }
        mo = mo->supportthings;
    }
}
#endif


#define BUFFSIZE  512
// Death messages relating to the target (dying) player
//
static
void P_DeathMessages ( mobj_t*       target,
                       mobj_t*       inflictor,
                       mobj_t*       source )
{
    char txt[BUFFSIZE+1];
    int     w;
    char    *str;

    if (!target || !target->player)
        return;

    if (target->player->mo != target )  // voodoo doll died
        return;
   
    if (source && source->player)
    {
        if (source->player==target->player)
        {
            str = text[DEATHMSG_SUICIDE];
            snprintf(txt, BUFFSIZE, str, player_names[target->player - players]);
            txt[BUFFSIZE-1] = 0;
            GenPrintf(EMSG_playmsg, txt);
            if( cv_splitscreen.EV )
                GenPrintf(EMSG_playmsg2, txt);
        }
        else
        {
            if (target->health < -9000) // telefrag !
                str = text[DEATHMSG_TELEFRAG];
            else
            {
                w = source->player->readyweapon;
                if( inflictor )
                {
                    switch(inflictor->type) {
                    case MT_ROCKET   : w = wp_missile; break;
                    case MT_PLASMA   : w = wp_plasma;  break;
                    case MT_EXTRABFG :
                    case MT_BFG      : w = wp_bfg;     break;
                    default : break;
                    }
                }

                switch(w)
                {
                case wp_fist:
                    str = text[DEATHMSG_FIST];
                    break;
                case wp_pistol:
                    str = text[DEATHMSG_GUN];
                    break;
                case wp_shotgun:
                    str = text[DEATHMSG_SHOTGUN];
                    break;
                case wp_chaingun:
                    str = text[DEATHMSG_MACHGUN];
                    break;
                case wp_missile:
                    str = text[DEATHMSG_ROCKET];
                    if (target->health < -target->info->spawnhealth &&
                        target->info->xdeathstate)
                        str = text[DEATHMSG_GIBROCKET];
                    break;
                case wp_plasma:
                    str = text[DEATHMSG_PLASMA];
                    break;
                case wp_bfg:
                    str = text[DEATHMSG_BFGBALL];
                    break;
                case wp_chainsaw:
                    str = text[DEATHMSG_CHAINSAW];
                    break;
                case wp_supershotgun:
                    str = text[DEATHMSG_SUPSHOTGUN];
                    break;
                default:
                    str = text[DEATHMSG_PLAYUNKNOW];
                    break;
                }
            }

            snprintf(txt, BUFFSIZE, str,
                     player_names[target->player - players],
                     player_names[source->player - players]);
            txt[BUFFSIZE-1] = 0;
            GenPrintf(EMSG_playmsg, txt);
            if( cv_splitscreen.EV )
                GenPrintf(EMSG_playmsg2, txt);
        }
    }
    else
    {
        if (!source)
        {
            // environment kills
            w = target->player->specialsector;      //see p_spec.c

            if (w==5)
                str = text[DEATHMSG_HELLSLIME];
            else if (w==7)
                str = text[DEATHMSG_NUKE];
            else if (w==16 || w==4)
                str = text[DEATHMSG_SUPHELLSLIME];
            else
                str = text[DEATHMSG_SPECUNKNOW];
        }
        else
        {
            // check for lava,slime,water,crush,fall,monsters..
            if (source->type == MT_BARREL)
            {
                if (source->target->player)
                {
                    GenPrintf(EMSG_playmsg, text[DEATHMSG_BARRELFRAG],
                                player_names[target->player - players],
                                player_names[source->target->player - players]);
                    return;
                }
                else
                    str = text[DEATHMSG_BARREL];
            }
            else
            switch (source->type)
            {
              case MT_POSSESSED: str = text[DEATHMSG_POSSESSED]; break;
              case MT_SHOTGUY:   str = text[DEATHMSG_SHOTGUY];   break;
              case MT_VILE:      str = text[DEATHMSG_VILE];      break;
              case MT_FATSO:     str = text[DEATHMSG_FATSO];     break;
              case MT_CHAINGUY:  str = text[DEATHMSG_CHAINGUY];  break;
              case MT_TROOP:     str = text[DEATHMSG_TROOP];     break;
              case MT_SERGEANT:  str = text[DEATHMSG_SERGEANT];  break;
              case MT_SHADOWS:   str = text[DEATHMSG_SHADOWS];   break;
              case MT_HEAD:      str = text[DEATHMSG_HEAD];      break;
              case MT_BRUISER:   str = text[DEATHMSG_BRUISER];   break;
              case MT_UNDEAD:    str = text[DEATHMSG_UNDEAD];    break;
              case MT_KNIGHT:    str = text[DEATHMSG_KNIGHT];    break;
              case MT_SKULL:     str = text[DEATHMSG_SKULL];     break;
              case MT_SPIDER:    str = text[DEATHMSG_SPIDER];    break;
              case MT_BABY:      str = text[DEATHMSG_BABY];      break;
              case MT_CYBORG:    str = text[DEATHMSG_CYBORG];    break;
              case MT_PAIN:      str = text[DEATHMSG_PAIN];      break;
              case MT_WOLFSS:    str = text[DEATHMSG_WOLFSS];    break;
              default:           str = text[DEATHMSG_DEAD];      break;
            }
        }
        GenPrintf(EMSG_playmsg, str, player_names[target->player - players]);
    }
}

// WARNING : check cv_fraglimit>0 before call this function !
void P_CheckFragLimit(player_t *p)
{
    int fragteam = 0;
    if( cv_teamplay.EV )
    {
        int i;
        for(i=0;i<MAXPLAYERS;i++)
        {
            if(ST_SameTeam(p,&players[i]))
                fragteam += ST_PlayerFrags(i);
        }
    }
    else
    {
        fragteam = ST_PlayerFrags(p - players);
    }
    // CV_VALUE, may be too large for EV
    if( fragteam >= cv_fraglimit.value )
        G_ExitLevel();
}


/************************************************************
 *
 *  Returns ammo count in current weapon
 *
 ************************************************************
 */
// [WDJ] This was the source of a negative ammo count.
// This was solely to allow return of empty clip to trigger a later test
// that differentiates dropped ammo.
// This coding has been changed, to fit into uint16_t.
// This is stored into TST_has_ammo_dropped and dropped_ammo_count.
// Unfortunately, this is in saved games too.
static
uint16_t P_AmmoInWeapon( player_t * player )
{
    ammotype_t  ammo_type = player->weaponinfo[player->readyweapon].ammo;
    // ammo is int in player, which is due to status bar.
    int         ammo_count = player->ammo[ammo_type];

    // empty ammo drop is differentiated from noammo
    return (ammo_type == am_noammo) ? TST_NOAMMO   // no ammo dropped
        : ((ammo_count <= 0)? TST_AMMO_0 : ammo_count);  // ammo dropped
}


#ifdef HEXEN
typedef struct {
    sfxid_t     sfxid;
    statenum_t  to_state;
} hexen_kill_entry_t;

static hexen_kill_entry_t  hexen_fire_play_kill_table[3] = 
{
    {sfx_hexen_player_fighter_burn_death, S_HEXEN_PLAY_F_FDTH1 },  // PCLASS_FIGHTER
    {sfx_hexen_player_cleric_burn_death,  S_HEXEN_PLAY_C_FDTH1 },  // PCLASS_CLERIC
    {sfx_hexen_player_mage_burn_death,    S_HEXEN_PLAY_M_FDTH1 },  // PCLASS_MAGE
};

static statenum_t  hexen_ice_play_kill_table[4] =
{
    S_HEXEN_PLAY_F_PLAY_ICE,  // PCLASS_FIGHTER
    S_HEXEN_PLAY_C_PLAY_ICE,  // PCLASS_CLERIC
    S_HEXEN_PLAY_M_PLAY_ICE,  // PCLASS_MAGE
    S_HEXEN_PLAY_P_PLAY_ICE,  // PCLASS_PIG
};

// Hexen kill complications
static byte hexen_kill_player( m_obj_t* target )
{
    int pclass = target->player_class;

    if( target->flags2 & MF2_FIREDAMAGE )
    {
        if( pclass >= PCLASS_FIGHTER && pclass <= PCLASS_MAGE )
        {
            // FIXME: pclass indexing
            hexen_kill_entry_t * hke = hexen_fire_play_kill_table[ pclass ];
            S_StartMobjSound( target, hke->sfxid );
            P_SetMobjState( target, hke->to_state );
            return 1;
        }
    }

    if( target->flags3 & MF3_ICEDAMAGE )
    {
        target->tflags &= MFT_TRANSLATION6; // turn color off
        target->eflags |= MF_ICECORPSE;  // ice corpse, can be blasted.
        if( pclass >= PCLASS_FIGHTER && pclass <= PCLASS_PIG )
        {
            // FIXME: pclass indexing
            P_SetMobjState( target, hexen_ice_play_kill_table[ pclass ] );
            return 1;
        }
    }

    return 0;
}


// Same table as hexen_fire_play_kill_table
static hexen_kill_entry_t  hexen_fire_mon_kill_table[3] =
{
    {sfx_hexen_player_fighter_burn_death, S_HEXEN_PLAY_F_FDTH1 },  // FIGHTER_BOSS
    {sfx_hexen_player_cleric_burn_death,  S_HEXEN_PLAY_C_FDTH1 },  // CLERIC_BOSS
    {sfx_hexen_player_mage_burn_death,    S_HEXEN_PLAY_M_FDTH1 },  // MAGE_BOSS
};

static statenum_t  hexen_ice_mon_kill_table[4] =
{
    S_HEXEN_PLAY_F_PLAY_ICE,  // PCLASS_FIGHTER
    S_HEXEN_PLAY_C_PLAY_ICE,  // PCLASS_CLERIC
    S_HEXEN_PLAY_M_PLAY_ICE,  // PCLASS_MAGE
    S_HEXEN_PLAY_P_PLAY_ICE,  // PCLASS_PIG
};

typedef struct {
    uint16_t    type;  // monster type
    statenum_t  to_state;
} hexen_kill_pairentry_t;

// Not indexed
static hexen_kill_pairentry_t hexen_ice_mon_kill_table[] =
{
    { MT_HEXEN_BISHOP, S_HEXEN_BISHOP_ICE },
    { MT_HEXEN_CENTAUR, S_HEXEN_CENTAUR_ICE },
    { MT_HEXEN_CENTAURLEADER, S_HEXEN_CENTAUR_ICE },
    { MT_HEXEN_DEMON, S_HEXEN_DEMON_ICE },
    { MT_HEXEN_DEMON2, S_HEXEN_DEMON_ICE },
    { MT_HEXEN_SERPENT, S_HEXEN_SERPENT_ICE },
    { MT_HEXEN_SERPENTLEADER, S_HEXEN_SERPENT_ICE },
    { MT_HEXEN_WRAITH, S_HEXEN_WRAITH_ICE },
    { MT_HEXEN_WRAITHB, S_HEXEN_WRAITH_ICE },
    { MT_HEXEN_ETTIN, S_HEXEN_ETTIN_ICE1 },
    { MT_HEXEN_FIREDEMON, S_HEXEN_FIRED_ICE1 },
    { MT_HEXEN_FIGHTER_BOSS, S_HEXEN_FIGHTER_ICE },
    { MT_HEXEN_CLERIC_BOSS, S_HEXEN_CLERIC_ICE },
    { MT_HEXEN_MAGE_BOSS, S_HEXEN_MAGE_ICE },
    { MT_HEXEN_PIG, S_HEXEN_PIG_ICE },
};
const byte num_hexen_ice_mon_kill = sizeof(hexen_ice_mon_kill_table) / sizeof(hexen_kill_pairentry_t);

static byte hexen_kill_monster( m_obj_t* target )
{
    int tt = target->type;
    sfxid_t     sfxid = 0;
    statenum_t  to_state = 0;

    if( target->flags2 & MF2_FIREDAMAGE )
    {
        if( tt >= MT_HEXEN_FIGHTER_BOSS && tt <= MT_HEXEN_MAGE_BOSS )  // Assuming they are in order
        {
            // FIXME: type indexing
            hexen_kill_entry_t * hke = hexen_fire_mon_kill_table[ tt - MT_HEXEN_FIGHTER_BOSS ];
            sfxid = hke->sfxid;
            to_state = hke->to_state;
            goto apply_sfx;  // fire kill return
        }
        else if ( tt == MT_HEXEN_TREE_DESTRUCTIBLE )
        {
            target->height = 24 * FRACUNIT;
            sfxid = sfx_hexen_tree_explode;
            to_state = S_HEXEN_ZTREEDES_X1;
            goto apply_sfx;  // tree kill return
        }
    }

    if( target->flags3 & MF3_ICEDAMAGE )
    {
        target->eflags |= MF_ICECORPSE;  // ice corpse, can be blasted
        // The monster types are scattered, so pair table.
        int ii;
        for( ii=0; ii<num_hexen_ice_mon_kill; ii++ )
        {
            hexen_kill_pairentry_t * hkpe = hexen_ice_mon_kill_table[ii];
            if( hkpe->type == tt )
            {
                to_state = hkpe->to_state;
                goto apply_state;  // ice kill return
            }
        }
        target->eflags &= ~MF_ICECORPSE;  // Remove it again
    }

    if( tt == MT_HEXEN_MINOTAUR )
    {
        mobj_t * tsm = target->special1.m;  // ???
        if( tsm->health > 0 )
        {
            if( ! ActiveMinotaur( tsm->player ) )  // player is Minotaur ??
            {
                tsm->player->powers[pw_minotaur] = 0;  // end Minotaur power
            }
        }
    }
    else if( tt == MT_HEXEN_TREE_DESTRUCTIBLE )
    {
        target->height = 24*FRACUNIT;
    }

    to_state = target->info->deathstate; // default
    
    if( target->info->xdeathstate )
    {
        // If overkill by more than half, and have an extreme death state.
        if( target->health < -(P_MobjSpawnHealth( target ) >> 1)
            // To fix firedemon staying in fall state.
            || ( tt == MT_HEXEN_FIREDEMON
                 && ( target->z <= target->floorz + (2*FRACUNIT)) ) )
        {
            to_state = target->info->xdeathstate;
        }
    }
    goto apply_state;

apply_sfx:
    if( sfxid )
      S_StartMobjSound( target, sfxid );
apply_state:
    P_SetMobjState( target, to_state );
    return 1;
}

// Called by 
void hexen_target_play_puppybeat( mobj_t* target, byte rand_chance )
{
    if( target->flags & MF_COUNTKILL
        && PP_Random(ph_hexen) < 128
        && ! S_GetSoundPlayingInfo( target, sfx_hexen_puppybeat) )
    {
        if( target->type == MT_HEXEN_CENTAUR
            || target->type == MT_HEXEN_CENTAURLEADER
            || target->type == MT_HEXEN_ETTIN )
        {
            S_StartMobjSound( target, sfx_hexen_puppybeat );
        }
    }
}

#endif


// P_KillMobj
//
//      source is the attacker, (for revenge, frags)
//      target is the 'target' of the attack, target dies...
//      inflictor is the weapon, missile, creature melee, or NULL, (for messages)
//                                          113
void P_KillMobj ( mobj_t*  target,
                  mobj_t*  inflictor,
                  mobj_t*  source )
{
    mobjtype_t  item = 0;
    mobj_t*     mo;
    player_t*   target_player;
    player_t*   source_player;
    uint16_t    drop_ammo_count = 0;

    target_player = target->player; // there must be a target
    source_player = source? source->player : NULL;
    
    // dead target is no more shootable
    if( ! cv_solidcorpse.EV )
        target->flags &= ~MF_SHOOTABLE;

    target->flags &= ~(MF_FLOAT|MF_SKULLFLY);

    if (target->type != MT_SKULL)
        target->flags &= ~MF_NOGRAVITY;

#ifdef DOGS
    // [WDJ] MBF dogs, extension for DoomLegacy.
    if( (target->type == MT_DOGS) || (target->type == helper_MT) )
        G_KillDog( target );     
#endif

    // scream a corpse :)
    if( target->flags & MF_CORPSE )
    {
        // Turn it to gibs.
        P_SetMobjState (target, S_GIBS);

        target->flags &= ~MF_SOLID;
        target->height = 0;
        target->radius<<= 1;
        target->skin = 0;

        //added:22-02-98: lets have a neat 'crunch' sound!
        S_StartObjSound(target, sfx_slop);
        return;
    }

    //added:22-02-98: remember who exploded the barrel, so that the guy who
    //                shot the barrel which killed another guy, gets the frag!
    //                (source is passed from barrel to barrel also!)
    //                (only for multiplayer fun, does not remember monsters)
    if ((target->type == MT_BARREL || target->type == MT_POD)
        && source_player )
    {
        SET_TARGET_REF( target->target, source );
    }

    if( EV_legacy < 131 )  // old Legacy, Boom, MBF
    {
        // Version 131 and after this is done later in A_Fall.
        // (this fix the stepping monster)
        target->flags   |= MF_CORPSE|MF_DROPOFF;
        target->height >>= 2;
        if( EV_legacy >= 112 )
            target->radius -= (target->radius>>4);      //for solid corpses
    }
    // show death messages, only if it concern the console player
    // (be it an attacker or a target)
    if( target_player && (target_player == consoleplayer_ptr) )
        P_DeathMessages (target, inflictor, source);
    else
    if( source_player && (source_player == consoleplayer_ptr) )
        P_DeathMessages (target, inflictor, source);
    else
    if( target_player && target_player->bot )	//added by AC for acbot
       P_DeathMessages (target, inflictor, source);

#ifdef MBF21
    if( EN_remove_dead_thinker )
    {
        // Need in MBF21 to test if monster died.
        P_UpdateClassThink( &target->thinker, TH_unknown );
    }
#endif
    
#ifdef HEXEN
    if( EN_hexen )
    {
        if( target->special )
        {
            if( (target->flags & MF_COUNTKILL) || (target->type == MT_HEXEN_ZBELL) )
            {
                if( target->type == MT_HEXEN_SORCBOSS )
                {
                    byte darg[3] = (0,0,0);
                    P_Start_ACS( target->special, 0, darg, target, NULL, 0 );
                }
                else
                {
                    // DSDA ??
                    // FIXME
                    mapformat.execute_line_special( target->special, target->special_args, NULL, 0,
                                                    map_info.flags & MI_ACTIVATE_OWN_DEATH_SPECIALS ? target : source );
                }
            }
        }
    }
#endif

    // if killed by a player
    if( source_player )
    {
        // count for intermission
        if (target->flags & MF_COUNTKILL)
            source_player->killcount++;

        // count frags if player killed player
        if( target_player )
        {
            source_player->frags[target_player - players]++;
            if( EN_heretic )
            {
                if( source_player == displayplayer_ptr
                   || source_player == displayplayer2_ptr )
                    S_StartSound(sfx_gfrag);

                // Make a super chicken
                if( source_player->chickenTics )
                    P_GivePower(source_player, pw_weaponlevel2);
            }
            // check fraglimit cvar
            if (cv_fraglimit.value)
                P_CheckFragLimit( source_player );
        }
    }
    else if (!multiplayer && (target->flags & MF_COUNTKILL))
    {
        // count all monster deaths,
        // even those caused by other monsters
        players[0].killcount++;
    }

    // if a player avatar dies...
    if( target_player )
    {
        // count environment kills against you (you fragged yourself!)
        if (!source)
            target_player->frags[target_player - players]++;

        if( ! cv_solidcorpse.EV )
            target->flags &= ~MF_SOLID;                     // does not block
        target->flags2 &= ~MF2_FLY;
        target_player->powers[pw_flight] = 0;
        target_player->powers[pw_weaponlevel2] = 0;
        target_player->playerstate = PST_DEAD;
        P_DropWeapon (target_player);                  // put weapon away

        // [Arcade] A death ends scoring for the rest of the run.  Guarded
        // for single player and live play inside HS_Player_Died.
        HS_Player_Died();

        // [Arcade] Audit counts *every* player death, not just the scored
        // single player ones HS_Player_Died cares about: a deathmatch death
        // is still the cabinet being played.
        AU_Player_Death();

        if( target_player == consoleplayer_ptr )
        {
            // don't die in auto map,
            // switch view prior to dying
            if (automapactive)
                AM_Stop ();

            //added:22-02-98: recenter view for next live...
            localaiming[0] = 0;
        }
        if( target_player == displayplayer2_ptr ) // NULL when unused
        {
            // player 2
            //added:22-02-98: recenter view for next live...
            localaiming[1] = 0;
        }
#if 0
        // Heretic player flame death not implemented in DoomLegacy
        // All this does is make the player scream for a while.
        if( EN_heretic && (target->flags2 & MF2_FIREDAMAGE))
        { // Player flame death
            P_SetMobjState(target, S_PLAY_FDTH1);
            //S_StartObjSound(target, sfx_hedat1); // Burn sound
            goto done;
        }
#endif
#ifdef HEXEN
        if( EN_hexen )
        {
            if( hexen_kill_player( target ) )
                goto done;
        }
#endif
    }

#ifdef HEXEN
    if( EN_hexen )
    {
        if( hexen_kill_monster( target ) )
          goto done;
    }
#endif

    if ( target->info->xdeathstate
         && ( target->health < -(
               (EN_heretic)? (target->info->spawnhealth>>1)  // heretic
               : target->info->spawnhealth  // doom
            ) )
       )
    {
        P_SetMobjState (target, target->info->xdeathstate);
    }
    else
        P_SetMobjState (target, target->info->deathstate);

    target->tics -= PP_Random(pr_killtics)&3;

    if (target->tics < 1)
        target->tics = 1;

    // Drop stuff.
    // This determines the kind of object spawned
    // during the death frame of a thing.

    // Frags Weapon Falling support
    if( target_player && cv_fragsweaponfalling.EV )
    {
        // [WDJ] Changed to be rid of negative ammo counts.
        drop_ammo_count = P_AmmoInWeapon(target_player);
        //if ( drop_ammo_count == TST_NOAMMO )
        //    goto done;

        if (EN_heretic)
        {
            switch (target_player->readyweapon)
            {
                case wp_crossbow:
                    item = MT_HMISC15;
                    break;

                case wp_blaster:
                    item = MT_RIPPER;
                    break;

                case wp_skullrod:
                    item = MT_WSKULLROD;
                    break;

                case wp_phoenixrod:
                    item = MT_WPHOENIXROD;
                    break;

                case wp_mace:
                    item = MT_WMACE;
                    break;

                default:
                    //debug_Printf("Unknown weapon %d\n", target->player->readyweapon);
                    goto done;
            }
        }
        else
        {
            switch (target->player->readyweapon)
            {
                case wp_shotgun:
                    item = MT_SHOTGUN;
                    break;

                case wp_supershotgun:
                    item = MT_SUPERSHOTGUN;
                    break;

                case wp_chaingun:
                    item = MT_CHAINGUN;
                    break;

                case wp_missile:
                    item = MT_ROCKETLAUNCH;
                    break;

                case wp_plasma:
                    item = MT_PLASMAGUN;
                    break;

                case wp_bfg:
                    item = MT_BFG9000;
                    break;

                default:
                    //debug_Printf("Unknown weapon %d\n", target->player->readyweapon);
                    goto done;
            }
        }
    }
    else
    {
        //DarkWolf95: Support for Chex Quest
        if(gamemode == chexquest1)  //don't drop monster ammo in chex quest
           goto done;

#ifdef ENABLE_DEHEXTRA
        // MT_WOLFSS, MT_POSSESSED, MT_SHOTGUY, MT_CHAINGUY have initialized dropped_item.
        item = target->info->dropped_item;
        if( (item == 0) || (item == MT_NULL) )  // because init to 0
            goto done;
#else
        switch (target->type)
        {
            case MT_WOLFSS:
            case MT_POSSESSED:
                item = MT_CLIP;
                break;

            case MT_SHOTGUY:
                item = MT_SHOTGUN;
                break;

            case MT_CHAINGUY:
                item = MT_CHAINGUN;
                break;

            default:
                goto done;
        }
#endif
    }

    // SoM: Damnit! Why not use the target's floorz?
    // Doom, Boom, MBF use ONFLOORZ.
    mo = P_SpawnMobj (target->x, target->y,
                      ((EV_legacy < 132) ? ONFLOORZ : target->floorz), item);
    mo->flags |= MF_DROPPED;    // special versions of items

    if( !cv_fragsweaponfalling.EV )
        drop_ammo_count = 0;    // Doom default ammo count

    mo->dropped_ammo_count = drop_ammo_count;

done:
    return;
}


//---------------------------------------------------------------------------
//
// FUNC P_MinotaurSlam
//
//---------------------------------------------------------------------------

static
void P_MinotaurSlam(mobj_t *source, mobj_t *target)
{
    angle_t angle;
    fixed_t thrust;
    
    angle = R_PointToAngle2(source->x, source->y, target->x, target->y);
    thrust = 16*FRACUNIT + (PP_Random(ph_minoslam)<<10);
    target->momx += FixedMul(thrust, cosine_ANG(angle));
    target->momy += FixedMul(thrust, sine_ANG(angle));
    P_DamageMobj(target, NULL, NULL, HITDICE(6));
    if(target->player)
    {
        target->reactiontime = 14 + (PP_Random(ph_minoslam)&7);
    }
}

//---------------------------------------------------------------------------
//
// FUNC P_TouchWhirlwind
//
//---------------------------------------------------------------------------

static
boolean P_TouchWhirlwind(mobj_t *target)
{
    int randVal;
    
    target->angle += PP_SignedRandom(ph_whirlwind)<<20;
    target->momx += PP_SignedRandom(ph_whirlwind)<<10;
    target->momy += PP_SignedRandom(ph_whirlwind)<<10;
    if(leveltime&16 && !(target->flags2&MF2_BOSS))
    {
        randVal = PP_Random(ph_whirlwind);
        if(randVal > 160)
        {
            randVal = 160;
        }
        target->momz += randVal<<10;
        if(target->momz > 12*FRACUNIT)
        {
            target->momz = 12*FRACUNIT;
        }
    }
    if(!(leveltime&7))
    {
        return P_DamageMobj(target, NULL, NULL, 3);
    }
    return false;
}


//---------------------------------------------------------------------------
//
// FUNC P_ChickenMorphPlayer
//
// Returns true if the player gets turned into a chicken.
//
//---------------------------------------------------------------------------

// [WDJ] Fixed to keep the same player mobj.
// Used to change the player mobj, and hide the prev as a corpse above
// the ceiling using S_FREETARGMOBJ.  This could happen in Line attack or Damage.
boolean P_ChickenMorphPlayer(player_t *player)
{
    mobj_t *pmo;
    int oldflags2;
    
    if(player->chickenTics)
    {
        if((player->chickenTics < CHICKENTICS-TICRATE)
            && !player->powers[pw_weaponlevel2])
        { // Make a super chicken
            P_GivePower(player, pw_weaponlevel2);
        }
        return false;
    }
    if(player->powers[pw_invulnerability])
    { // Immune when invulnerable
        return false;
    }
    pmo = player->mo;
    oldflags2 = pmo->flags2;
    P_MorphMobj(pmo, MT_CHICPLAYER, MM_telefog,
#ifdef PLAYER_CHICKEN_KEEPS_SHADOW
                      MF_SHADOW
#else
                      0
#endif
                );
    pmo->special1 = player->readyweapon;  // save for later restore
    pmo->flags2 |= oldflags2&MF2_FLY;  // preserve fly
    // Clear skin so it does not override chicken.
    pmo->skin = NULL;  // Chickens all look alike.
    pmo->tflags &= ~MFT_TRANSLATION6; // no color translation for chicken
    // spawnhealth for chicken is 100, this is 30
    player->health = pmo->health = MAXCHICKENHEALTH;
    player->armorpoints = player->armortype = 0;
#ifndef PLAYER_CHICKEN_KEEPS_SHADOW
    // If keep MF_SHADOW and cancel invisibility, then MF_SHADOW is permananet.
    player->powers[pw_invisibility] = 0;
#endif
    player->powers[pw_weaponlevel2] = 0;
    player->weaponinfo = wpnlev1info;
    player->chickenTics = CHICKENTICS;  // start chicken timer
    P_ActivateBeak(player);
    return true;
}

//---------------------------------------------------------------------------
//
// FUNC P_ChickenMorph
//
//---------------------------------------------------------------------------

// Other actors, not players.
boolean P_ChickenMorph(mobj_t *actor)
{
    mobjtype_t moType;
    
    if(actor->player)
    {
        return false;
    }
    moType = actor->type;
    switch(moType)
    {
        case MT_POD:
        case MT_CHICKEN:
        case MT_HHEAD:
        case MT_MINOTAUR:
        case MT_SORCERER1:
        case MT_SORCERER2:
            return false;
        default:
            break;
    }
    
    // preserve position, angle, target, invisible
    P_MorphMobj(actor, MT_CHICKEN, MM_telefog, MF_SHADOW);
    actor->special1 = CHICKENTICS+PP_Random(ph_chickenmorph);  // monster chickentics
    actor->special2 = moType;  // save type for restore
    return true;
}

//---------------------------------------------------------------------------
//
// FUNC P_AutoUseChaosDevice
//
//---------------------------------------------------------------------------

boolean P_AutoUseChaosDevice(player_t *player)
{
    int i;
    
    for(i = 0; i < player->inventorySlotNum; i++)
    {
        if(player->inventory[i].type == arti_teleport)
        {
            P_PlayerUseArtifact(player, arti_teleport);
            player->health = player->mo->health = (player->health+1)/2;
            return(true);
        }
    }
    return(false);
}

//---------------------------------------------------------------------------
//
// PROC P_AutoUseHealth
//
//---------------------------------------------------------------------------

// From Heretic
void P_AutoUseHealth(player_t *player, int saveHealth)
{
    int i;
    int count;
    int normalCount;
    int superCount;

    // Uses P_PlayerUseArtifact, so do not need to know inventory slot.
    normalCount = superCount = 0;
    for(i = 0; i < player->inventorySlotNum; i++)
    {
        if(player->inventory[i].type == arti_health)
        {
            normalCount = player->inventory[i].count;
        }
        else if(player->inventory[i].type == arti_superhealth)
        {
            superCount = player->inventory[i].count;
        }
    }
    if((gameskill == sk_baby) && (normalCount*25 >= saveHealth))
    { // Use quartz flasks
        count = (saveHealth+24)/25;
        for(i = 0; i < count; i++)
            P_PlayerUseArtifact( player, arti_health);
    }
    else if(superCount*100 >= saveHealth)
    { // Use mystic urns
        count = (saveHealth+99)/100;
        for(i = 0; i < count; i++)
            P_PlayerUseArtifact( player, arti_superhealth);
    }
    else if((gameskill == sk_baby)
        && (superCount*100+normalCount*25 >= saveHealth))
    { // Use mystic urns and quartz flasks
        count = (saveHealth+24)/25;
        for(i = 0; i < count; i++)
            P_PlayerUseArtifact( player, arti_health);

        saveHealth -= count*25;
        count = (saveHealth+99)/100;
        for(i = 0; i < count; i++)
            P_PlayerUseArtifact( player, arti_superhealth);
    }
    player->mo->health = player->health;
}


#ifdef MBF21
// Return: true when not default behaviour, and are in the same infight group
// Called from: P_DamageMobj
static inline
boolean P_infighting_immune( mobj_t* target, mobj_t* source )
{
//    byte ifgrp = mobjinfo[target->type].infight_group;
    byte ifgrp = target->info->infight_group;

    return (ifgrp != IG_DEFAULT) && (ifgrp == source->info->infight_group);
}
#endif


//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
// Return true when damaged, for blood splats and other effects.
// Fixed to not change the player mobj.
boolean P_DamageMobj ( mobj_t*   target,
                       mobj_t*   inflictor,
                       mobj_t*   source,
                       int       damage )
{
    player_t*   target_player;  // always target->player
    player_t*   source_player;
    angle_t     ang;
    int         angf;
    fixed_t     thrust;
    boolean     voodoo_target = false;
    boolean     mbf_justhit = false;  // MBF, delayed MF_JUSTHIT
    boolean     takedamage = true;  // block damage between members of same team

    target_player = target->player;
    source_player = source? source->player : NULL;

    // [Arcade] Pacifist: the player must not damage a monster.
    //
    // Testing the *source* rather than the weapon covers every route in one
    // place -- a hitscan, a melee swing, a rocket's splash, and a barrel chain
    // all arrive here with the player as source, because A_Explode passes the
    // barrel's own target (whoever set it off) on as the source of the blast.
    // So "no shooting barrels that harm monsters" needs no rule of its own.
    // Monster infighting is untouched: the source is then the other monster.
    //
    // MF_COUNTKILL is the monster test.  It excludes barrels, which are
    // shootable but are not monsters -- blowing one up harms nothing by
    // itself, and if it does harm a monster that damage arrives here as its
    // own call with the player still the source.
    //
    // source->player is checked against source itself so a voodoo doll cannot
    // be mistaken for the player, the same guard the voodoo code below uses.
    if( damage > 0 && source_player && (source_player->mo == source)
        && (target_player == NULL) && (target->flags & MF_COUNTKILL) )
    {
        HS_Player_Damaged_Monster();
    }

    // [WDJ] 7/2017 Moved the voodoo intercept of damage to be tested earlier
    // because of the weapons and armor specific player checks that
    // can get applied to the wrong player otherwise.
    // This code can change the target of the damage.
    // If we ever implement the player as a monster, this code needs to be first.
    if( target_player )
    {
        // [WDJ] 2/7/2011 Intercept voodoo damage
        voodoo_target = (target_player->mo != target);
        if( voodoo_target )
        {
            mobj_t * voodoo_thing = target;
            if(voodoo_mode >= VM_target)
            {
                // Multiplayer and single player:
                // try to find someone appropriate, instead of spawn point player.
                // Target source player causing damage
                if( source_player
                    && (source_player->mo == source) )  // is real player
                {
                    // Shooting any voodoo doll, select shooting player
                    target_player = source_player;
                }
                // Target last player to trigger a switch or linedef.
                else if( spechit_player && spechit_player->mo )
                {
                    target_player = spechit_player;
                }
            }

            if(! target_player->mo )  // this player is not present
            {
                if( voodoo_mode < VM_target )
                {
                    // remove this voodoo doll to avoid segfaults
                    P_RemoveMobj( voodoo_thing );
                }
                goto ret_false;
            }

            if( voodoo_mode == VM_vanilla )
            {
                target->health -= damage;  // damage the voodoo too
            }
            else
            {
                if( multiplayer && (damage > target_player->health))
                {
                    // Kill the voodoo, so it cannot kill after respawn.
                    // Voodoo doll in crusher is game fatal otherwise.
                    voodoo_thing->health = 0;
                    voodoo_thing->player = NULL;
                    P_KillMobj ( voodoo_thing, inflictor, source );
                    spechit_player = NULL;  // cancel voodoo damage
                }
                // let player mobj get the damage, no Zombies
                target = target_player->mo;
            }
        }

        if( gameskill == sk_baby )
            damage >>= 1;   // take half damage in trainer mode
    }
   
 
    // killough 8/31/98: allow bouncers to take damage
    if ( !(target->flags & (MF_SHOOTABLE | MF_BOUNCES)) )
        goto ret_false; // shouldn't happen...

    if( target->health <= 0 )
    {
#ifdef HEXEN
        // [WDJ]
        if( EN_hexen )
        {
            if( inflictor && (inflictor->flags3 & MF3_ICEDAMAGE) )
              goto ret_false;

            if( target->eflags & MF_ICECORPSE )  // has been frozen, can be blasted
            {
                target->tics = 1;
                target->momx = target->momy = 0;
            }
        }
#endif

        // [WDJ] Solid Corpse health < 0.
        if( !(target->flags & MF_CORPSE) )
          goto ret_false;
        // Can shoot corpses to turn them into gibs.
    }

#ifdef HEXEN
    if( EN_hexen )
    {
        if( (target->flags3 & MF3_INVULNERABLE) && (damage < 10000) )
        {
            // Hexen Invulnerable
            if( player )
              goto ret_false;

            if( ! inflictor )
              goto ret_false;  // default blocked by INVULNERABLE

            switch( inflictor->type )
            {
                // Inflictors that override INVULNERABLE
             case MT_HEXEN_HOLY_FX:
             case MT_HEXEN_POISONCLOUD:
             case MT_HEXEN_FIREBOMB:
                break;
             default:
                goto ret_false;  // all others blocked by INVULNERABLE
            }
        }

        // Player with GOD, or invulnerability blocks normal damage.
        if( target_player
          && ( (player->cheats & CF_GODMODE) || target_player->powers[pw_invulnerability] )
          && (damage < 10000) )
          goto ret_false;
    }
#endif

    if ( target->flags & MF_SKULLFLY )
    {
        // Minotaur is invulnerable during charge attack
        if(target->type == MT_MINOTAUR)
            goto ret_false;

        target->momx = target->momy = target->momz = 0;
    }

#ifdef HEXEN
    if( target->flags3 && MF3_DORMANT )
    {
        goto ret_false; // dormant monster is invulnerable, and does not wake up.
    }
#endif

    // Special damage types
    if( EN_heretic && inflictor )
    {
        switch(inflictor->type)
        {
            // Heretic
        case MT_EGGFX:
            if( target_player )
            {
                // Fixed to not change the player mobj.
                P_ChickenMorphPlayer( target_player );
            }
            else
            {
                P_ChickenMorph(target);
            }
            goto ret_false; // Always return
        case MT_WHIRLWIND:
            takedamage = P_TouchWhirlwind(target);
            goto ret_damage;
        case MT_MINOTAUR:
            if(inflictor->flags&MF_SKULLFLY)
            { // Slam only when in charge mode
                P_MinotaurSlam(inflictor, target);
                goto ret_true;
            }
            break;
        case MT_MACEFX4: // Death ball
            if((target->flags2&MF2_BOSS) || target->type == MT_HHEAD)
            { // Don't allow cheap boss kills
                break;
            }
            else if( target_player )
            { // Player specific checks
                if( target_player->powers[pw_invulnerability] )
                { // Can't hurt invulnerable players
                    break;
                }
                if( P_AutoUseChaosDevice(target_player) )
                { // Player was saved using chaos device
                    goto ret_false;
                }
            }
            damage = 10000; // Something's gonna die
            break;
        case MT_PHOENIXFX2: // Flame thrower
            if( target_player && PP_Random(ph_phoenixdam2) < 128 )
            { // Freeze player for a bit
                target->reactiontime += 4;
            }
            break;
        case MT_RAINPLR1: // Rain missiles
        case MT_RAINPLR2:
        case MT_RAINPLR3:
        case MT_RAINPLR4:
            if(target->flags2&MF2_BOSS)
            { // Decrease damage for bosses
                damage = (PP_Random(ph_raindam)&7)+1;
            }
            break;
        case MT_HORNRODFX2:
        case MT_PHOENIXFX1:
            if(target->type == MT_SORCERER2 && PP_Random(ph_sordam) < 96)
            { // D'Sparil teleports away
                P_DSparilTeleport(target);
                goto ret_false;
            }
            break;
        case MT_BLASTERFX1:
        case MT_RIPPER:
            if(target->type == MT_HHEAD)
            { // Less damage to Ironlich bosses
                damage = PP_Random(ph_headdam)&1;
                if(!damage)
                    goto ret_false;
            }
            break;
        default:
            break;
        }
    }

#ifdef HEXEN
    if( EN_hexen && inflictor )
    {
        switch(inflictor->type)
        {
            // Heretic
        case MT_HEXEN_EGGFX:
            if( target_player )
            {
                // Fixed to not change the player mobj.
                P_ChickenMorphPlayer(target_player);
            }
            else
            {
                P_ChickenMorph(target);
            }
            goto ret_false; // Always return
        case MT_HEXEN_TELOTHER_FX1:
        case MT_HEXEN_TELOTHER_FX2:
        case MT_HEXEN_TELOTHER_FX3:
        case MT_HEXEN_TELOTHER_FX4:
        case MT_HEXEN_TELOTHER_FX5:
            if( (target->flags & FT_COUNTKILL)
                && (target->type != MT_HEXEN_SERPENT)
                && (target->type != MT_HEXEN_SERPENTLEADER)
                && !(target->flags2 & MF2_BOSS) )
            {
                P_TeleportOther( target );
            }
            goto ret_false;

        case MT_HEXEN_MINOTAUR:
            if( inflictor->flags & MF_SKULLFLY )
            { // Slam only when in charge mode
                P_MinotaurSlam(inflictor, target);
                goto ret_true;
            }
            break;
        case MT_HEXEN_BISHOP_FX:
            // Bishops are just too nasty otherwise
            damage >>= 1;
            break;
        case MT_HEXEN_SHARD_FX1:
            unsigned int mu = inflictor->special2.i;
            if( mu >= 1 && mu <= 3 )
              damage <<= mu;
            break;
         case MT_HEXEN_ICEGUY_FX2:
            damage >>= 1;
            break;
         case MT_HEXEN_C_STAFF_MISSLE:
            // Cleric Serpent staff, does poison damage.
#if 0  // same as POISONDART	    
            if( target_player )
            {
                P_PoisonPlayer( target_player, source, 20 );
                damage >>= 1;
            }
            break;
#endif	    
         case MT_HEXEN_POISONDART:
            // Poison dart, does poison damage.
            if( target_player )
            {
                P_PoisonPlayer( target_player, source, 20 );
                damage >>= 1;
            }
            break;
         case MT_HEXEN_POISONCLOUD:
            // Poison cloud, does poison damage.
            if( target_player )
            {
                if( target_player->poisoncount < 4 )
                {
                    unsigned int pd = (PP_Random(ph_hexen) & 0x0F) + 15;
                    P_PoisonDamage( target_player, source, pd, false ); // no painsound
                    P_PoisonPlayer( target_player, source, 20 );
                    S_StartMobjSound( target, sfx_hexen_player_poisoncough );
                    goto ret_true;
                }
                goto ret_false;
            }
            else if( ! (target->flags & MF_COUNTKILL)
            {
                goto ret_false;  // only damage player and monsters with poisoncloud
            }
            break;
         case MT_HEXEN_F_SWORD_MISSLE:
            if( target_player )
            {
                damage -= damage >> 2;  // reduce player damage by 25%
            }
            break;
         default:
            break;
        }
    }
#endif

    // Some close combat weapons should not
    // inflict thrust and push the victim out of reach,
    // thus kick away unless using the chainsaw.
#ifdef HEXEN
    // !(target->flags & MF_NOCLIP)  // does not appear in HEXEN
    // DSDA-Doom has this test.
    // All Hexen weapons do not thrust, so cannot affect player battles.
#endif
    if (inflictor
        && !(target->flags & MF_NOCLIP)  // unless target is NOCLIP
        && !(inflictor->flags2&MF2_NODMGTHRUST)  // unless inflictor cannot thrust
        && !(source_player  // weapon held by source_player
#ifdef MBF21
             && (
# ifdef HEXEN
                 // Should allow some player weapons to be set to thrust, by using MBF21 flags.
                 // Must take care in Dehacked that Hexen weapon default is WPF_NOTHRUST,
                 // probably by an explicit THRUST keyword.
//                 EN_hexen   // all player weapons in hexen do not thrust
//                 ||
# endif
                 // Any weapon that does not thrust. Chainsaw, (or set by DEH).
                 source_player->weaponinfo[ source_player->readyweapon ].flags & WPF_NO_THRUST
                 )
#else
             && (
# ifdef HEXEN
                 EN_hexen   // all player weapons in hexen do not thrust
                 ||
# endif
                 // not chainsaw
                 source_player->readyweapon == wp_chainsaw
                 )
#endif
            )
       )
    {
        // Impose thrust upon the target from the weapon
        fixed_t  amomx, amomy, amomz=0;//SoM: 3/28/2000

        ang = R_PointToAngle2 ( inflictor->x, inflictor->y,
                                target->x, target->y);

        unsigned int thrust_factor = (EN_heretic)? 150*(FRACUNIT>>3) : 100*(FRACUNIT>>3);
        thrust = thrust_factor * damage / target->info->mass;

        // sometimes a target shot down might fall off a ledge forwards
        if ( damage < 40
             && damage > target->health
             && target->z - inflictor->z > 64*FRACUNIT
             && (PP_Random(pr_damagemobj) & 0x01)
           )
        {
            ang += ANG180;
            thrust *= 4;
        }

        angf = ANGLE_TO_FINE(ang);

#ifdef HEXEN
        // Hexen too ??, wp_staff only
#endif
        if( EN_heretic
            && source_player && (source == inflictor)
            && source_player->powers[pw_weaponlevel2]
            && source_player->readyweapon == wp_staff)
        {
            // Staff power level 2
            target->momx += FixedMul(10*FRACUNIT, finecosine[angf]);
            target->momy += FixedMul(10*FRACUNIT, finesine[angf]);
            if(!(target->flags&MF_NOGRAVITY))
            {
                target->momz += 5*FRACUNIT;
            }
        }
        else
        {
            // all other thrusting weapons
            amomx = FixedMul (thrust, finecosine[angf]);
            amomy = FixedMul (thrust, finesine[angf]);
            target->momx += amomx;
            target->momy += amomy;
            
            // added momz (do it better for missiles explosion)
            if ( source
                 && (EV_legacy >= 124)
                 && ((EV_legacy < 129) || !cv_allowrocketjump.EV))
            {
                // Legacy
                int dist,z;
                
                if(source==target) // rocket in yourself (suicide)
                {
                    viewx=inflictor->x;
                    viewy=inflictor->y;
                    z=inflictor->z;
                }
                else
                {
                    viewx=source->x;
                    viewy=source->y;
                    z=source->z;
                }
                dist=R_PointToDist(target->x,target->y);
                
                viewx=0;
                viewy=z;
                ang = R_PointToAngle(dist,target->z);
                amomz = FixedMul (thrust, sine_ANG(ang));
            }
            else //SoM: 2/28/2000: Added new function.
            if( (EV_legacy >= 129) && cv_allowrocketjump.EV )
            {
                // Legacy rocket jump.
                fixed_t delta1 = abs(inflictor->z - target->z);
                fixed_t delta2 = abs(inflictor->z - (target->z + target->height));
                amomz = (abs(amomx) + abs(amomy))>>1;
                
                if(delta1 >= delta2 && inflictor->momz < 0)
                    amomz = -amomz;
            }

            target->momz += amomz;
#ifdef CLIENTPREDICTION2
            if( target_player && target_player->spirit )
            {
                target_player->spirit->momx += amomx;
                target_player->spirit->momy += amomy;
                target_player->spirit->momz += amomz;
            }
#endif
            // [WDJ] MBF
            // killough 11/98: thrust objects hanging off ledges
            if( target->eflags & MF_FALLING && (target->tipcount >= MAXTIPCOUNT) )
                target->tipcount = 0;
        }
    }
   
    // Solid Corpse specific
    if( target->flags & MF_CORPSE )
    {
        target->health -= damage;
        // [WDJ] Corpse health < 0, so solid corpse test is < 0.
        if( target->health < -target->info->spawnhealth )
            P_KillMobj ( target, inflictor, source );  // to gibs
        // Keep corpse from ticking the P_Random in the pain test.
        goto ret_true;
    }

    // target player specific
    if( target_player )
    {
        // end of game hell hack
        if( EN_doom_etc
            && target->subsector->sector->special == 11
            && damage >= target->health)
        {
            damage = target->health - 1;
        }

#ifdef HEXEN
        if( EN_hexen )
        {
            // Complicated Hexen armor damage.	    
            fixed_t saved_per = pclass[ target_player->pclass ].auto_armor_save
              + target_player->armorpoints[ ARMOR_ARMOR ]
              + target_player->armorpoints[ ARMOR_SHEILD ]
              + target_player->armorpoints[ ARMOR_HELMET ]
              + target_player->armorpoints[ ARMOR_AMULET ];

            if( saved_per )
            {
                if( saved_per > (100*FRACUNIT) )  // limit to 100%
                  saved_per = (100*FRACUNIT);

                fixed_t fixed_damage = damage << FRACBITS;

                int ai;
                for( ai=0; ai < NUM_HEXEN_ARMOR; ai++ )
                {
                    int * tpa = &target_player->armorpoints[i];
                    if( *tpa )
                    {
                        fixed_t damadj =
                          FixedMul( fixed_damage, pclass[ player->pclass ].armor_increment[ai] );
                        *tpa -= FixedDiv( damadj, (300*FRACUNIT) );
                    }

                    if( *tpa < (2*FRACUNIT) )  // armor is gone
                      *tpa = 0;
                }

                fixed_t saved_damage =
                  FixedDiv( FixedMul( fixed_damage, saved_per ), (100*FRACUNIT) );

                if( saved_damage > (saved_per * 2))
                  saved_damage == (saved_per * 2);

                damage -= (saved_damage >> FRACBITS);
            }
            goto hexen_bypass_1;
        }
#endif

        // Below certain threshold,
        // ignore damage in GOD mode, or with INVUL power.
        if( (target_player->cheats&CF_GODMODE) || target_player->powers[pw_invulnerability] )
        {
            // Boom, MBF: killough 3/26/98: make god mode 100%
            // !comp[comp_god]
            if( (target_player->cheats&CF_GODMODE) && EN_invul_god )
                goto ret_false;

            if( damage < 1000 )
                goto ret_false;
        }

        if( target_player->armortype )
        {
            int saved;
            if( target_player->armortype == 1 )
                saved = (EN_heretic)? damage>>1 : damage/3;
            else
                saved = (EN_heretic)? (damage>>1)+(damage>>2) : damage/2;

            if( target_player->armorpoints <= saved )
            {
                // armor is used up
                saved = target_player->armorpoints;
                target_player->armortype = 0;
            }
            target_player->armorpoints -= saved;
            damage -= saved;
        }

#ifdef HEXEN
hexen_bypass_1:
#endif	

        // added team play and teamdamage (view logboris at 13-8-98 to understand)
        // [WDJ] 2/7/2011  Allow damage to player when:
        // olddemo (version < 125) // because they did not have these restrictions
        // OR telefrag        // not subject to friendly fire tests
        // OR no source       // no source interaction, sector damage etc.
        // OR NOT sourceplayer  // monster attack
        // OR (source==target)  // self inflicted damage (missile launcher)
        // OR voodoo_target   // voodoo damage allowed by previous tests
        // OR NOT multiplayer  // single player
        // OR ( coop         // all on same team
        //     AND cv_teamdamage  // team members can hurt each other
        //    )
        // OR ( deathmatch   // teams or individual, not coop
        //     AND ( NOT teamplay
        //               // otherwise teamplay
        //           OR cv_teamdamage   // team members can hurt each other
        //           OR (target.team != source.team)
        //         )
        //    )
        // [WDJ] For readability and understanding, please do not try to reduce
        // these equations, they are not executed very often, and the
        // compiler will reduce them during optimization anyway.
        if( (! source)		   // no source interaction, sector damage etc.
            || (! source_player)   // monster attack
            || voodoo_target	   // allowed voodoo damage
            || (damage>1000)       // telefrag and death-ball
            || (EV_legacy < 125)   // old demoversion bypasses restrictions
            || (source==target)    // self-inflicted
            || (! multiplayer)     // single player
            || ( (!deathmatch) && cv_teamdamage.EV )  // coop
            || ( deathmatch        // deathmatch 1,2,3
                 && ( (!cv_teamplay.EV)    // no teams
                      || cv_teamdamage.EV  // can damage within team
                      || ! ST_SameTeam(source_player, target_player) // diff team
                    )
                 )
            )
        {
            if( damage >= target_player->health
                && ((gameskill == sk_baby) || deathmatch)
                && !target_player->chickenTics)
            { // Try to use some inventory health
                P_AutoUseHealth( target_player, damage - target_player->health + 1 );
            }

            // Update player health here, because they may die before
            // reaching the later player update.
            target_player->health -= damage;   // mirror mobj health here for Dave
            if (target_player->health < 0)
                target_player->health = 0;
            // [WDJ] If target_player->mo is updated here, it prevents player gibs.
            // target = target_player->mo, as set in voodoo logic.

            target_player->damagecount += damage;  // add damage after armor / invuln

            if( target_player->damagecount > 100 )
                target_player->damagecount = 100;  // teleport stomp does 10k points...

            //added:22-02-98: force feedback ??? electro-shock???
            if( target_player == consoleplayer_ptr )
            {
                I_Tactile (40,10,40+min(damage, 100)*2);
#if 0
                // [WDJ] Palette flash was in DSDA-Doom, but we may have moved or removed it.
                // We have a screen flash control, that can flash status bar instead.
                if( EN_heretic_hexen )
//                    SB_PaletteFlash( false );  // DSDA-Doom
                    H_PaletteFlash( target_player );
#endif
            }
        }
        else
        {
            takedamage = false;  // block damage
        }
        target_player->attacker = source;
    }

    if( takedamage )
    {
        target->health -= damage;

        // check for kill
        if (target->health <= 0)
        {
            if( EN_heretic )
            {
                target->special1 = damage;
                if( target_player && !target_player->chickenTics && inflictor )
                { // Check for flame death
                    if((inflictor->flags2&MF2_FIREDAMAGE)
                       || ((inflictor->type == MT_PHOENIXFX1)
                           && (target->health > -50) && (damage > 25)))
                    {
                        target->flags2 |= MF2_FIREDAMAGE;
                    }
                }
            }

#ifdef HEXEN
            else if( EN_hexen )
            {
                if( inflictor )
                {
                    // special fire damage, ice damage
                    // Uses Heretic fire flags
                    if( inflictor->flags2 & MF2_FIREDAMAGE )
                    {
                        if( target_player && ! target_player->morph_tics )
                        {
                            // player burned
                            if( (target->health > -50) && (damage > 25 ) )  // flame death
                              target->flags2 |= MF2_FIREDAMAGE; // indicate flame damage
                        }
                        else
                        {
                            target->flags2 |= MF2_FIREDAMAGE; // indicate flame damage
                        }
                    }
                    else if( inflictor->flags3 & MF3_ICEDAMAGE )
                    {
                        target->flags3 |= MF3_ICEDAMAGE;  // indicate ice damage
                    }
                }

                if( source && source->type == MT_HEXEN_MINOTAUR )
                {
                    // Minotaur kills credited to his master.
                    mobj_t * m_master = source->special.m;
                    // Only if still alive, and not pointing to fighter head.
                    if( m_master->player
                        && ( m_master->player->mo == m_master ) )
                    {
                        source = m_master;  // redirect kill credits
                        source_player = m_master->player;
                    }
                }
                else  // bug fix: fixes MINOTAUR test affecting deadly fourth weapon
                if( source_player
                    && source_player->readyweapon == wp_fourth )
                {
                    // Always extreme death from fourth weapon
                    // [WDJ] This is not right for MINOTAUR redirect above.  FIXME
                    // This should be before redirect.
                    target->health -= 5000;
                }
            }
#endif

            P_KillMobj ( target, inflictor, source );
            goto ret_true;
        }

        // This must be after KillMobj, so target damage can be negative.
        if( target_player )
        {
            if( target_player->mo )
                target_player->mo->health = target_player->health; // keep mobj and player health same
        }

#ifdef MONSTER_VARY
        if( cv_monster_vary.EV )
            speed_varied_by_damage( target, damage );
#endif

        // [WDJ] MBF, From MBF, PrBoom, EternityEngine.
        // killough 9/7/98: keep track of targets so that friends can help friends
        if( EN_mbf )
        {
            // If target is a player, set player's target to source,
            // so that a friend can tell who is hurting a player
            if( target_player )
            {
                SET_TARGET_REF( target->target, source );
            }

            // killough 9/8/98:
            // If target's health is less than 50%, move it to the front of its list.
            // This will slightly increase the chances that enemies will choose to
            // "finish it off", but its main purpose is to alert friends of danger.
            if( target->health*2 < target->info->spawnhealth )
            {
                P_MoveClassThink( &target->thinker, 1 );  // move first
            }
        }

        if( (PP_Random(pr_painchance) < target->info->painchance)
            && !(target->flags&(MF_SKULLFLY|MF_CORPSE)) )
        {
#ifdef HEXEN
            if( EN_hexen
                && inflictor
                && inflictor->type >= MT_HEXEN_LIGHTNING_FLOOR
                && inflictor->type <= MT_HEXEN_LIGHTNING_ZAP )
            {
                if( PP_Random(ph_hexen) < 96 )
                {
                    target->flags |= MF_JUSTHIT; // fight back!
                    P_SetMobjState( target, target->info->painstate );
                }
                else
                {
                    // Electrocute the target
                    target->frame |= FF_FULLBRIGHT;
                    hexen_target_play_puppybeat( target, 128 );
                }
                goto hexen_bypass_2;
            }
#endif

            if( EN_mbf )
                mbf_justhit = true;  // defer setting MF_JUSTHIT to below
            else
                target->flags |= MF_JUSTHIT;    // fight back!

            P_SetMobjState (target, target->info->painstate);

#ifdef HEXEN
            if( EN_hexen
                && inflictor
                && inflictor->type >= MT_HEXEN_POISONCLOUD )
            {
                hexen_target_play_puppybeat( target, 128 );
            }
#endif
        }

#ifdef HEXEN
hexen_bypass_2:
#endif
        target->reactiontime = 0;           // we're awake now...
    }

    if ( source
         && ( source != target  // fixes bug where monster attacks self
#if 0
              || compatibility_level == doom_12_compatibility  // not in DoomLegacy
#endif
              )
#ifdef MBF21
         // MT_VILE has set MF3_DMGIGNORED, MF3_NOTHRESHOLD
         && !(source->flags3 & MF3_DMGIGNORED)  // MT_VILE
         && (!target->threshold || (target->flags3 & MF3_NOTHRESHOLD))  // MT_VILE
         && !( EN_heretic_hexen
               && ( (source->flags2 & MF2_BOSS)
                    || (source->type == MT_WIZARD && target->type == MT_SORCERER2)
              ) )
#else
         // Doom2 specific monsters
         && source->type != MT_VILE
         && (!target->threshold || target->type == MT_VILE)
         && !(source->flags2 & MF2_BOSS)
         && !(source->type == MT_WIZARD && target->type == MT_SORCERER2)
#endif
#ifdef HEXEN
         && !( EN_hexen
               && (   (target->type == MT_HEXEN_BISHOP)   // Could use MF3_DMGIGNORED
                   || (target->type == MT_HEXEN_MINOTAUR)   // Could use MF3_DMGIGNORED
                   || (target->type == MT_HEXEN_CENTAUR && source->type == MT_CENTAURLEADER)
                   || (target->type == MT_HEXEN_CENTAURLEADER && source->type == MT_CENTAUR)
                  )
             )
#endif
         && ( MONSTER_INFIGHTING
              || ! (EN_mbf && SAME_FRIEND(source, target))
            )
#ifdef MBF21
         && ! P_infighting_immune( target, source )  // MBF21 infighting settings
#endif
       )
    {
        // killough 2/15/98: remember last enemy, to prevent sleeping early
        // 2/21/98: Place priority on players
        // killough 9/9/98: cleaned up, made more consistent:
        if( !target->lastenemy
            || target->lastenemy->health <= 0
            ||( EN_mbf ?
                  ( target->target != source
                    && SAME_FRIEND(target, target->lastenemy) )  // MBF
                : ! target->lastenemy->player  // Vanilla
              )
          )
        {
            // remember last enemy - killough
            SET_TARGET_REF( target->lastenemy, target->target );
        }

        // if not intent on another player,
        // chase after this one
        SET_TARGET_REF( target->target, source );       // killough 11/98

        target->threshold = BASETHRESHOLD;
        if( target->state == &states[target->info->spawnstate]
            && target->info->seestate != S_NULL)
            P_SetMobjState (target, target->info->seestate);
    }

    // killough 11/98: Don't attack a friend, unless hit by that friend.
    // cph 2006/04/01 - implicitly this is only if mbf_features
    if( mbf_justhit   // set by MBF to defer MF_JUSTHIT to here
        && ( !target->target
             || target->target == source
             || !BOTH_FRIEND(target, target->target) )
      )
        target->flags |= MF_JUSTHIT;    // fight back!

ret_damage:
    return takedamage;

ret_false:
    return false;

ret_true:
    return true;  // damaged
}
