// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_hpspr.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// $Log: p_hpspr.c,v $
// Revision 1.3  2001/05/27 13:42:47  bpereira
// Revision 1.2  2001/02/24 13:35:20  bpereira
//
//
//
// DESCRIPTION:
//   this file is include by P_pspr.c
//   it contain all heretic player sprite specific
//
//-----------------------------------------------------------------------------


// P_pspr.c
/*
#include "doomincl.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
*/
// Macros

#define LOWERSPEED FRACUNIT*6
#define RAISESPEED FRACUNIT*6
#define WEAPONBOTTOM 128*FRACUNIT
#define WEAPONTOP 32*FRACUNIT
#define MAGIC_JUNK 1234
#define MAX_MACE_SPOTS 8

mobjtype_t PuffType;

static int MaceSpotCount;
static struct
{
    fixed_t x;
    fixed_t y;
} MaceSpots[MAX_MACE_SPOTS];

fixed_t bulletslope;


weaponinfo_t wpnlev1info[NUMWEAPONS] =
{
        { // Staff
                am_noammo,              // ammo
                0,
                S_STAFFUP,              // upstate
                S_STAFFDOWN,            // downstate
                S_STAFFREADY,           // readystate
                S_STAFFATK1_1,          // atkstate
                S_STAFFATK1_1,          // holdatkstate
                S_NULL                  // flashstate
        },
        { // Gold wand
                am_goldwand,            // ammo
                USE_GWND_AMMO_1,
                S_GOLDWANDUP,           // upstate
                S_GOLDWANDDOWN,         // downstate
                S_GOLDWANDREADY,        // readystate
                S_GOLDWANDATK1_1,       // atkstate
                S_GOLDWANDATK1_1,       // holdatkstate
                S_NULL                  // flashstate
        },
        { // Crossbow
                am_crossbow,            // ammo
                USE_CBOW_AMMO_1,
                S_CRBOWUP,              // upstate
                S_CRBOWDOWN,            // downstate
                S_CRBOW1,               // readystate
                S_CRBOWATK1_1,          // atkstate
                S_CRBOWATK1_1,          // holdatkstate
                S_NULL                  // flashstate
        },
        { // Blaster
                am_blaster,             // ammo
                USE_BLSR_AMMO_1,
                S_BLASTERUP,            // upstate
                S_BLASTERDOWN,          // downstate
                S_BLASTERREADY,         // readystate
                S_BLASTERATK1_1,        // atkstate
                S_BLASTERATK1_3,        // holdatkstate
                S_NULL                  // flashstate
        },
        { // Skull rod
                am_skullrod,            // ammo
                USE_SKRD_AMMO_1,
                S_HORNRODUP,            // upstate
                S_HORNRODDOWN,          // downstate
                S_HORNRODREADY,         // readystae
                S_HORNRODATK1_1,        // atkstate
                S_HORNRODATK1_1,        // holdatkstate
                S_NULL                  // flashstate
        },
        { // Phoenix rod
                am_phoenixrod,          // ammo
                USE_PHRD_AMMO_1,
                S_PHOENIXUP,            // upstate
                S_PHOENIXDOWN,          // downstate
                S_PHOENIXREADY,         // readystate
                S_PHOENIXATK1_1,        // atkstate
                S_PHOENIXATK1_1,        // holdatkstate
                S_NULL                  // flashstate
        },
        { // Mace
                am_mace,                // ammo
                USE_MACE_AMMO_1,
                S_MACEUP,               // upstate
                S_MACEDOWN,             // downstate
                S_MACEREADY,            // readystate
                S_MACEATK1_1,           // atkstate
                S_MACEATK1_2,           // holdatkstate
                S_NULL                  // flashstate
        },
        { // Gauntlets
                am_noammo,              // ammo
                0,
                S_GAUNTLETUP,           // upstate
                S_GAUNTLETDOWN,         // downstate
                S_GAUNTLETREADY,        // readystate
                S_GAUNTLETATK1_1,       // atkstate
                S_GAUNTLETATK1_3,       // holdatkstate
                S_NULL                  // flashstate
        },
        { // Beak
                am_noammo,              // ammo
                0,
                S_BEAKUP,               // upstate
                S_BEAKDOWN,             // downstate
                S_BEAKREADY,            // readystate
                S_BEAKATK1_1,           // atkstate
                S_BEAKATK1_1,           // holdatkstate
                S_NULL                  // flashstate
        }
};

weaponinfo_t wpnlev2info[NUMWEAPONS] =
{
        { // Staff
                am_noammo,              // ammo
                0,
                S_STAFFUP2,             // upstate
                S_STAFFDOWN2,           // downstate
                S_STAFFREADY2_1,        // readystate
                S_STAFFATK2_1,          // atkstate
                S_STAFFATK2_1,          // holdatkstate
                S_NULL                  // flashstate
        },
        { // Gold wand
                am_goldwand,            // ammo
                USE_GWND_AMMO_2,
                S_GOLDWANDUP,           // upstate
                S_GOLDWANDDOWN,         // downstate
                S_GOLDWANDREADY,        // readystate
                S_GOLDWANDATK2_1,       // atkstate
                S_GOLDWANDATK2_1,       // holdatkstate
                S_NULL                  // flashstate
        },
        { // Crossbow
                am_crossbow,            // ammo
                USE_CBOW_AMMO_2,
                S_CRBOWUP,              // upstate
                S_CRBOWDOWN,            // downstate
                S_CRBOW1,               // readystate
                S_CRBOWATK2_1,          // atkstate
                S_CRBOWATK2_1,          // holdatkstate
                S_NULL                  // flashstate
        },
        { // Blaster
                am_blaster,             // ammo
                USE_BLSR_AMMO_2,
                S_BLASTERUP,            // upstate
                S_BLASTERDOWN,          // downstate
                S_BLASTERREADY,         // readystate
                S_BLASTERATK2_1,        // atkstate
                S_BLASTERATK2_3,        // holdatkstate
                S_NULL                  // flashstate
        },
        { // Skull rod
                am_skullrod,            // ammo
                USE_SKRD_AMMO_2,
                S_HORNRODUP,            // upstate
                S_HORNRODDOWN,          // downstate
                S_HORNRODREADY,         // readystae
                S_HORNRODATK2_1,        // atkstate
                S_HORNRODATK2_1,        // holdatkstate
                S_NULL                  // flashstate
        },
        { // Phoenix rod
                am_phoenixrod,          // ammo
                USE_PHRD_AMMO_2,
                S_PHOENIXUP,            // upstate
                S_PHOENIXDOWN,          // downstate
                S_PHOENIXREADY,         // readystate
                S_PHOENIXATK2_1,        // atkstate
                S_PHOENIXATK2_2,        // holdatkstate
                S_NULL                  // flashstate
        },
        { // Mace
                am_mace,                // ammo
                USE_MACE_AMMO_2,
                S_MACEUP,               // upstate
                S_MACEDOWN,             // downstate
                S_MACEREADY,            // readystate
                S_MACEATK2_1,           // atkstate
                S_MACEATK2_1,           // holdatkstate
                S_NULL                  // flashstate
        },
        { // Gauntlets
                am_noammo,              // ammo
                0,
                S_GAUNTLETUP2,          // upstate
                S_GAUNTLETDOWN2,        // downstate
                S_GAUNTLETREADY2_1,     // readystate
                S_GAUNTLETATK2_1,       // atkstate
                S_GAUNTLETATK2_3,       // holdatkstate
                S_NULL                  // flashstate
        },
        { // Beak
                am_noammo,              // ammo
                0,
                S_BEAKUP,               // upstate
                S_BEAKDOWN,             // downstate
                S_BEAKREADY,            // readystate
                S_BEAKATK2_1,           // atkstate
                S_BEAKATK2_1,           // holdatkstate
                S_NULL                  // flashstate
        }
};

//---------------------------------------------------------------------------
//
// PROC P_OpenWeapons
//
// Called at level load before things are loaded.
//
//---------------------------------------------------------------------------

void P_OpenWeapons(void)
{
    MaceSpotCount = 0;
}

//---------------------------------------------------------------------------
//
// PROC P_AddMaceSpot
//
//---------------------------------------------------------------------------

void P_AddMaceSpot(mapthing_t *mthing)
{
    if(MaceSpotCount == MAX_MACE_SPOTS)
    {
        I_Error("Too many mace spots.");
    }
    MaceSpots[MaceSpotCount].x = mthing->x<<FRACBITS;
    MaceSpots[MaceSpotCount].y = mthing->y<<FRACBITS;
    MaceSpotCount++;
}

//---------------------------------------------------------------------------
//
// PROC P_RepositionMace
//
// Chooses the next spot to place the mace.
//
//---------------------------------------------------------------------------

void P_RepositionMace(mobj_t *mo)
{
    int spot;
    subsector_t *ss;

    P_UnsetThingPosition(mo);
    spot = PP_Random(ph_macepos2) % MaceSpotCount;
    mo->x = MaceSpots[spot].x;
    mo->y = MaceSpots[spot].y;
    ss = R_PointInSubsector(mo->x, mo->y);
    mo->z = mo->floorz = ss->sector->floorheight;
    mo->ceilingz = ss->sector->ceilingheight;
    P_SetThingPosition(mo);
}

//---------------------------------------------------------------------------
//
// PROC P_CloseWeapons
//
// Called at level load after things are loaded.
//
//---------------------------------------------------------------------------

void P_CloseWeapons(void)
{
    int spot;

    if(!MaceSpotCount)
    { // No maces placed
        return;
    }

    if( (! deathmatch) && (PP_Random(ph_macepos1) < 64) )
    { // Sometimes doesn't show up if not in deathmatch
        return;
    }

    spot = PP_Random(ph_macepos1) % MaceSpotCount;
    P_SpawnMobj(MaceSpots[spot].x, MaceSpots[spot].y, ONFLOORZ, MT_WMACE);
}

//---------------------------------------------------------------------------
//
// PROC P_ActivateBeak
//
//---------------------------------------------------------------------------

void P_ActivateBeak(player_t *player)
{
    player->pendingweapon = wp_nochange;
    player->readyweapon = wp_beak;
    player->psprites[PS_weapon].sy = WEAPONTOP;
    P_SetPsprite(player, PS_weapon, S_BEAKREADY);
}

//---------------------------------------------------------------------------
//
// PROC P_PostChickenWeapon
//
//---------------------------------------------------------------------------

void P_PostChickenWeapon(player_t *player, weapontype_t weapon)
{
    if(weapon == wp_beak)
    { // Should never happen
        weapon = wp_staff;
    }
    player->pendingweapon = wp_nochange;
    player->readyweapon = weapon;
    player->psprites[PS_weapon].sy = WEAPONBOTTOM;
    P_SetPsprite(player, PS_weapon, wpnlev1info[weapon].upstate);
}

//---------------------------------------------------------------------------
//
// PROC P_UpdateBeak
//
//---------------------------------------------------------------------------

void P_UpdateBeak(player_t *player, pspdef_t *psp)
{
    psp->sy = WEAPONTOP+(player->chickenPeck<<(FRACBITS-1));
}

//---------------------------------------------------------------------------
//
// PROC A_BeakReady
//
//---------------------------------------------------------------------------

void A_BeakReady(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    if(player->cmd.buttons&BT_ATTACK)
    { // Chicken beak attack
        player->GB_flags |= GB_attackdown;
        P_SetMobjState(pmo, S_CHICPLAY_ATK1);
        if(player->powers[pw_weaponlevel2])
        {
            P_SetPsprite(player, PS_weapon, S_BEAKATK2_1);
        }
        else
        {
            P_SetPsprite(player, PS_weapon, S_BEAKATK1_1);
        }
        P_NoiseAlert(pmo, pmo);
    }
    else
    {
        if(pmo->state == &states[S_CHICPLAY_ATK1])
        { // Take out of attack state
            P_SetMobjState(pmo, S_CHICPLAY);
        }
        player->GB_flags &= ~(GB_attackdown);
    }
}

//---------------------------------------------------------------------------
//
// PROC A_BeakRaise
//
//---------------------------------------------------------------------------

void A_BeakRaise(player_t *player, pspdef_t *psp)
{
    psp->sy = WEAPONTOP;
    P_SetPsprite(player, PS_weapon,
                 wpnlev1info[player->readyweapon].readystate);
}

//****************************************************************************
//
// WEAPON ATTACKS
//
//****************************************************************************

//----------------------------------------------------------------------------
//
// PROC A_BeakAttackPL1
//
//----------------------------------------------------------------------------

void A_BeakAttackPL1(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    int slope;

    damage = 1+(PP_Random(ph_beakatk1)&3);
    angle = pmo->angle;
    slope = P_AimLineAttack(pmo, angle, MELEERANGE, 0);
    // [WDJ] When PuffType is not set will get extraneous player mobj created.
    PuffType = MT_BEAKPUFF;  // param to P_SpawnPuff via LineAttack
    P_LineAttack(pmo, angle, MELEERANGE, slope, damage);
    // lar_linetarget returned by P_LineAttack
    if(lar_linetarget)
    {
        pmo->angle = R_PointToAngle2(pmo->x, pmo->y,
                                     lar_linetarget->x, lar_linetarget->y);
    }
    S_StartObjSound(pmo, sfx_chicpk1+(PP_Random(ph_beakatk1)%3));
    player->chickenPeck = 12;
    psp->tics -= PP_Random(ph_beakatk1)&7;
}

//----------------------------------------------------------------------------
//
// PROC A_BeakAttackPL2
//
//----------------------------------------------------------------------------

void A_BeakAttackPL2(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    int slope;

    damage = HITDICE(4);
    angle = pmo->angle;
    slope = P_AimLineAttack(pmo, angle, MELEERANGE, 0);
    PuffType = MT_BEAKPUFF;  // param to P_SpawnPuff via LineAttack
    P_LineAttack(pmo, angle, MELEERANGE, slope, damage);
    // lar_linetarget returned by P_LineAttack
    if(lar_linetarget)
    {
        pmo->angle = R_PointToAngle2(pmo->x, pmo->y,
                                     lar_linetarget->x, lar_linetarget->y);
    }
    S_StartObjSound(pmo, sfx_chicpk1+(PP_Random(ph_beakatk2)%3));
    player->chickenPeck = 12;
    psp->tics -= PP_Random(ph_beakatk2)&3;
}

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL1
//
//----------------------------------------------------------------------------

void A_StaffAttackPL1(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    int slope;

    damage = 5+(PP_Random(ph_staffatk1)&15);
    angle = pmo->angle;
    angle += PP_SignedRandom(ph_staffatk1)<<18;
    slope = P_AimLineAttack(pmo, angle, MELEERANGE, 0);
    PuffType = MT_STAFFPUFF;
    P_LineAttack(pmo, angle, MELEERANGE, slope, damage);
    // lar_linetarget returned by P_LineAttack
    if(lar_linetarget)
    {
        //S_StartObjSound(pmo, sfx_stfhit);
        // turn to face target
        pmo->angle = R_PointToAngle2(pmo->x, pmo->y,
                                     lar_linetarget->x, lar_linetarget->y);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL2
//
//----------------------------------------------------------------------------

void A_StaffAttackPL2(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    int slope;

    // P_inter.c:P_DamageMobj() handles target momentums
    damage = 18+(PP_Random(ph_staffatk2)&63);
    angle = pmo->angle;
    angle += PP_SignedRandom(ph_staffatk2)<<18;
    slope = P_AimLineAttack(pmo, angle, MELEERANGE, 0);
    PuffType = MT_STAFFPUFF2;
    P_LineAttack(pmo, angle, MELEERANGE, slope, damage);
    // lar_linetarget returned by P_LineAttack
    if(lar_linetarget)
    {
        //S_StartObjSound(pmo, sfx_stfpow);
        // turn to face target
        pmo->angle = R_PointToAngle2(pmo->x, pmo->y,
                                     lar_linetarget->x, lar_linetarget->y);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireBlasterPL1
//
//----------------------------------------------------------------------------

void A_FireBlasterPL1(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    
    S_StartObjSound(pmo, sfx_gldhit);
    player->ammo[am_blaster] -= USE_BLSR_AMMO_1;
    P_BulletSlope(pmo);
    damage = HITDICE(4);
    angle = pmo->angle;
    if(player->refire)
        angle += PP_SignedRandom(ph_blaster1)<<18;
    PuffType = MT_BLASTERPUFF1;
    P_LineAttack(pmo, angle, MISSILERANGE, bulletslope, damage);
    S_StartObjSound(pmo, sfx_blssht);
}

//----------------------------------------------------------------------------
//
// PROC P_BlasterMobjThinker
//
// Thinker for the ultra-fast blaster PL2 ripper-spawning missile.
//
//----------------------------------------------------------------------------

void P_BlasterMobjThinker(mobj_t *mobj)
{
    int i;
    fixed_t mfx, mfy, mfz;  // move by mom fraction
    fixed_t z;
    boolean changexy;

    // Handle movement
    if(mobj->momx || mobj->momy ||
        (mobj->z != mobj->floorz) || mobj->momz)
    {
        mfx = mobj->momx>>3;
        mfy = mobj->momy>>3;
        mfz = mobj->momz>>3;
        changexy = mfx || mfy;
        for(i = 0; i < 8; i++)
        {
            if(changexy)
            {
                if(!P_TryMove(mobj, mobj->x+mfx, mobj->y+mfy, true))
                { // Blocked move
                    P_ExplodeMissile(mobj);
                    return;
                }
            }
            mobj->z += mfz;
            if(mobj->z <= mobj->floorz)
            { // Hit the floor
                mobj->z = mobj->floorz;
                P_HitFloor(mobj);
                P_ExplodeMissile(mobj);
                return;
            }
            if(mobj->z+mobj->height > mobj->ceilingz)
            { // Hit the ceiling
                mobj->z = mobj->ceilingz-mobj->height;
                P_ExplodeMissile(mobj);
                return;
            }
            if(changexy && (PP_Random(ph_blasterthink) < 64))
            {
                z = mobj->z-8*FRACUNIT;
                if(z < mobj->floorz)
                {
                    z = mobj->floorz;
                }
                P_SpawnMobj(mobj->x, mobj->y, z, MT_BLASTERSMOKE);
            }
        }
    }
    // Advance the state
    if(mobj->tics != -1)
    {
        mobj->tics--;
        while(!mobj->tics)
        {
            if(!P_SetMobjState(mobj, mobj->state->nextstate))
            { // mobj was removed
                return;
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireBlasterPL2
//
//----------------------------------------------------------------------------

void A_FireBlasterPL2(player_t *player, pspdef_t *psp)
{
    mobj_t *mo;

    player->ammo[am_blaster] -= (deathmatch)? USE_BLSR_AMMO_1 : USE_BLSR_AMMO_2;
    mo = P_SpawnPlayerMissile(player->mo, MT_BLASTERFX1);
    if( mo )
      mo->thinker.function = TFI_BlasterMobjThinker;

    S_StartObjSound(player->mo, sfx_blssht);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL1
//
//----------------------------------------------------------------------------

void A_FireGoldWandPL1(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    
    player->ammo[am_goldwand] -= USE_GWND_AMMO_1;
    P_BulletSlope(pmo);
    damage = 7+(PP_Random(ph_goldwandatk1)&7);
    angle = pmo->angle;
    if(player->refire)
        angle += PP_SignedRandom(ph_goldwandatk1)<<18;
    PuffType = MT_GOLDWANDPUFF1;
    P_LineAttack(pmo, angle, MISSILERANGE, bulletslope, damage);
    S_StartObjSound(pmo, sfx_gldhit);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL2
//
//----------------------------------------------------------------------------

void A_FireGoldWandPL2(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    int i;
    angle_t angle;
    int damage;

    player->ammo[am_goldwand] -= (deathmatch)? USE_GWND_AMMO_1 : USE_GWND_AMMO_2;
    PuffType = MT_GOLDWANDPUFF2;
    P_BulletSlope(pmo);
// [WDJ] Disabled since 1.42 and I do not know why.
//      fixed_t momz = FixedMul(mobjinfo[MT_GOLDWANDFX2].speed, bulletslope);
//      P_SpawnMissileAngle(pmo, MT_GOLDWANDFX2, mo->angle-(ANG45/8), momz);
//      P_SpawnMissileAngle(pmo, MT_GOLDWANDFX2, mo->angle+(ANG45/8), momz);
    angle = pmo->angle - (ANG45/8);
    for(i = 0; i < 5; i++)
    {
        damage = 1+(PP_Random(ph_goldwandatk2)&7);
        P_LineAttack(pmo, angle, MISSILERANGE, bulletslope, damage);
        angle += ((ANG45/8)*2)/4;
    }
    S_StartObjSound(pmo, sfx_gldhit);
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1B
//
//----------------------------------------------------------------------------

void A_FireMacePL1B(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    mobj_t *ball;
    angle_t angle;

    if(player->ammo[am_mace] < USE_MACE_AMMO_1)
    {
        return;
    }
    player->ammo[am_mace] -= USE_MACE_AMMO_1;
    ball = P_SpawnMobj(pmo->x, pmo->y, pmo->z+28*FRACUNIT
                       - FOOTCLIPSIZE*((pmo->flags2&MF2_FEETARECLIPPED) != 0), MT_MACEFX2);
    ball->momz = 2*FRACUNIT+((player->aiming)<<(FRACBITS-5));
    ball->z += (player->aiming)<<(FRACBITS-4);
    ball->target = pmo;
    angle = pmo->angle;
    ball->angle = angle;
    // Common expression extraction is poor here, so explicit.
    int angf = ANGLE_TO_FINE( angle );
#ifdef MONSTER_VARY
    fixed_t speed = ball->speed;
#else
    fixed_t speed = ball->info->speed;
#endif
    ball->momx = (pmo->momx>>1) + FixedMul(speed, finecosine[angf]);
    ball->momy = (pmo->momy>>1) + FixedMul(speed, finesine[angf]);
    S_StartObjSound(ball, sfx_lobsht);
    P_CheckMissileSpawn(ball);
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1
//
//----------------------------------------------------------------------------

void A_FireMacePL1(player_t *player, pspdef_t *psp)
{
    mobj_t *ball;

    if(PP_Random(ph_maceatk1) < 28)
    {
        A_FireMacePL1B(player, psp);
        return;
    }
    if(player->ammo[am_mace] < USE_MACE_AMMO_1)
    {
        return;
    }
    player->ammo[am_mace] -= USE_MACE_AMMO_1;
    psp->sx = ((PP_Random(ph_maceatk1)&3)-2)*FRACUNIT;
    psp->sy = WEAPONTOP+(PP_Random(ph_maceatk1)&3)*FRACUNIT;
    ball = P_SPMAngle(player->mo, MT_MACEFX1,
                      player->mo->angle + (((PP_Random(ph_maceatk1)&7)-4)<<24));
    if(ball)
    {
        ball->special1 = 16; // tics till dropoff
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MacePL1Check
//
//----------------------------------------------------------------------------

void A_MacePL1Check(mobj_t *ball)
{
    if(ball->special1 == 0)
    {
        return;
    }
    ball->special1 -= 4;
    if(ball->special1 > 0)
    {
        return;
    }
    ball->special1 = 0;
    ball->flags2 |= MF2_LOGRAV;
    int angf = ANGLE_TO_FINE(ball->angle);
    ball->momx = FixedMul(7*FRACUNIT, finecosine[angf]);
    ball->momy = FixedMul(7*FRACUNIT, finesine[angf]);
    ball->momz -= ball->momz>>1;
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact
//
//----------------------------------------------------------------------------

void A_MaceBallImpact(mobj_t *ball)
{
    if((ball->z <= ball->floorz) && (P_HitFloor(ball) != FLOOR_SOLID))
    { // Landed in some sort of liquid
        P_RemoveMobj(ball);
        return;
    }
    if((ball->health != MAGIC_JUNK) && (ball->z <= ball->floorz)
        && ball->momz)
    { // Bounce
        ball->health = MAGIC_JUNK;
        ball->momz = (ball->momz*192)>>8;
        ball->flags2 &= ~MF2_FLOORBOUNCE;
        P_SetMobjState(ball, ball->info->spawnstate);
        S_StartObjSound(ball, sfx_bounce);
    }
    else
    { // Explode
        ball->flags |= MF_NOGRAVITY;
        ball->flags2 &= ~MF2_LOGRAV;
        S_StartObjSound(ball, sfx_lobhit);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact2
//
//----------------------------------------------------------------------------

void A_MaceBallImpact2(mobj_t *ball)
{
    angle_t angle;
    int     angf;
    fixed_t zm;
    mobj_t *tiny;

    if((ball->z <= ball->floorz) && (P_HitFloor(ball) != FLOOR_SOLID))
    { // Landed in some sort of liquid
        P_RemoveMobj(ball);
        return;
    }
    if((ball->z != ball->floorz) || (ball->momz < 2*FRACUNIT))
    { // Explode
        ball->momx = ball->momy = ball->momz = 0;
        ball->flags |= MF_NOGRAVITY;
        ball->flags2 &= ~(MF2_LOGRAV|MF2_FLOORBOUNCE);
    }
    else
    { // Bounce
        ball->momz = (ball->momz*192)>>8;
        zm = ball->momz - FRACUNIT;  // momz of FX3
        P_SetMobjState(ball, ball->info->spawnstate);

        tiny = P_SpawnMobj(ball->x, ball->y, ball->z, MT_MACEFX3);
        tiny->target = ball->target;
        angle = ball->angle + ANG90;
        tiny->angle = angle;
        angf = ANGLE_TO_FINE(angle);
        tiny->momx = (ball->momx>>1) + FixedMul(zm, finecosine[angf]);
        tiny->momy = (ball->momy>>1) + FixedMul(zm, finesine[angf]);
        tiny->momz = ball->momz;
        P_CheckMissileSpawn(tiny);

        tiny = P_SpawnMobj(ball->x, ball->y, ball->z, MT_MACEFX3);
        tiny->target = ball->target;
        angle = ball->angle - ANG90;
        tiny->angle = angle;
        angf = ANGLE_TO_FINE(angle);
        tiny->momx = (ball->momx>>1) + FixedMul(zm, finecosine[angf]);
        tiny->momy = (ball->momy>>1) + FixedMul(zm, finesine[angf]);
        tiny->momz = ball->momz;
        P_CheckMissileSpawn(tiny);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL2
//
//----------------------------------------------------------------------------

void A_FireMacePL2(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    mobj_t *mo;

    player->ammo[am_mace] -= (deathmatch)? USE_MACE_AMMO_1 : USE_MACE_AMMO_2;
    mo = P_SpawnPlayerMissile(pmo, MT_MACEFX4);
    if(mo)
    {
        mo->momx += pmo->momx;
        mo->momy += pmo->momy;
        mo->momz = 2*FRACUNIT+((player->aiming)<<(FRACBITS-5));
        // lar_linetarget returned by P_AimLineAttack, via P_SPMAngle
        if(lar_linetarget)
        {
            mo->tracer = lar_linetarget;
        }
    }
    S_StartObjSound(pmo, sfx_lobsht);
}

//----------------------------------------------------------------------------
//
// PROC A_DeathBallImpact
//
//----------------------------------------------------------------------------

void A_DeathBallImpact(mobj_t *ball)
{
    int i;
    mobj_t *target;
    angle_t angle = 0;
    boolean newAngle;

    if((ball->z <= ball->floorz) && (P_HitFloor(ball) != FLOOR_SOLID))
    { // Landed in some sort of liquid
        P_RemoveMobj(ball);
        return;
    }
    if((ball->z <= ball->floorz) && ball->momz)
    { // Bounce
        newAngle = false;
        target = ball->tracer;
        if(target)
        {
            if(!(target->flags&MF_SHOOTABLE))
            { // Target died
                ball->tracer = 0;
            }
            else
            { // Seek
                angle = R_PointToAngle2(ball->x, ball->y,
                                        target->x, target->y);
                newAngle = true;
            }
        }
        else
        { // Find new target
            for(i = 0; i < 16; i++)
            {
                P_AimLineAttack(ball, angle, 10*64*FRACUNIT, 0);
                // lar_linetarget returned by P_AimLineAttack
                if(lar_linetarget && ball->target != lar_linetarget)
                {
                    ball->tracer = lar_linetarget;
                    angle = R_PointToAngle2(ball->x, ball->y,
                                            lar_linetarget->x, lar_linetarget->y);
                    newAngle = true;
                    break;
                }
                angle += ANG45/2;
            }
        }
        if(newAngle)
        {
            ball->angle = angle;
            int angf = ANGLE_TO_FINE(angle);
#ifdef MONSTER_VARY
            ball->momx = FixedMul(ball->speed, finecosine[angf]);
            ball->momy = FixedMul(ball->speed, finesine[angf]);
#else
            ball->momx = FixedMul(ball->info->speed, finecosine[angf]);
            ball->momy = FixedMul(ball->info->speed, finesine[angf]);
#endif
        }
        P_SetMobjState(ball, ball->info->spawnstate);
        S_StartObjSound(ball, sfx_pstop);
    }
    else
    { // Explode
        ball->flags |= MF_NOGRAVITY;
        ball->flags2 &= ~MF2_LOGRAV;
        S_StartObjSound(ball, sfx_phohit);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnRippers
//
//----------------------------------------------------------------------------

void A_SpawnRippers(mobj_t *actor)
{
    int i;
    angle_t angle;
    mobj_t *ripper;

    for(i = 0; i < 8; i++)
    {
        ripper = P_SpawnMobj(actor->x, actor->y, actor->z, MT_RIPPER);
        ripper->target = actor->target;
        angle = i*ANG45;
        ripper->angle = angle;
        int angf = ANGLE_TO_FINE(angle);
#ifdef MONSTER_VARY
        ripper->momx = FixedMul(ripper->speed, finecosine[angf]);
        ripper->momy = FixedMul(ripper->speed, finesine[angf]);
#else
        ripper->momx = FixedMul(ripper->info->speed, finecosine[angf]);
        ripper->momy = FixedMul(ripper->info->speed, finesine[angf]);
#endif
        P_CheckMissileSpawn(ripper);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL1
//
//----------------------------------------------------------------------------

void A_FireCrossbowPL1(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;

    player->ammo[am_crossbow] -= USE_CBOW_AMMO_1;
    P_SpawnPlayerMissile(pmo, MT_CRBOWFX1);
    P_SPMAngle(pmo, MT_CRBOWFX3, pmo->angle-(ANG45/10));
    P_SPMAngle(pmo, MT_CRBOWFX3, pmo->angle+(ANG45/10));
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL2
//
//----------------------------------------------------------------------------

void A_FireCrossbowPL2(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;

    player->ammo[am_crossbow] -= (deathmatch)? USE_CBOW_AMMO_1 : USE_CBOW_AMMO_2;
    P_SpawnPlayerMissile(pmo, MT_CRBOWFX2);
    P_SPMAngle(pmo, MT_CRBOWFX2, pmo->angle-(ANG45/10));
    P_SPMAngle(pmo, MT_CRBOWFX2, pmo->angle+(ANG45/10));
    P_SPMAngle(pmo, MT_CRBOWFX3, pmo->angle-(ANG45/5));
    P_SPMAngle(pmo, MT_CRBOWFX3, pmo->angle+(ANG45/5));
}

//----------------------------------------------------------------------------
//
// PROC A_BoltSpark
//
//----------------------------------------------------------------------------

void A_BoltSpark(mobj_t *bolt)
{
    mobj_t *spark;
    
    if(PP_Random(ph_boltspark) > 50)
    {
        spark = P_SpawnMobj(bolt->x, bolt->y, bolt->z, MT_CRBOWFX4);
        spark->x += PP_SignedRandom(ph_boltspark)<<10;
        spark->y += PP_SignedRandom(ph_boltspark)<<10;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL1
//
//----------------------------------------------------------------------------

void A_FireSkullRodPL1(player_t *player, pspdef_t *psp)
{
    mobj_t *mo;

    if(player->ammo[am_skullrod] < USE_SKRD_AMMO_1)
    {
        return;
    }
    player->ammo[am_skullrod] -= USE_SKRD_AMMO_1;
    mo = P_SpawnPlayerMissile(player->mo, MT_HORNRODFX1);
    // Randomize the first frame
    if(mo && PP_Random(ph_skullrodatk1) > 128)
    {
        P_SetMobjState(mo, S_HRODFX1_2);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL2
//
// The special2 field holds the player number that shot the rain missile.
// The tracer field is used for the seeking routines, then as a counter
// for the sound looping.
//
//----------------------------------------------------------------------------

void A_FireSkullRodPL2(player_t *player, pspdef_t *psp)
{
    mobj_t *MissileMobj;

    player->ammo[am_skullrod] -= (deathmatch)? USE_SKRD_AMMO_1 : USE_SKRD_AMMO_2;
    MissileMobj = P_SpawnPlayerMissile(player->mo, MT_HORNRODFX2);
    // Use MissileMobj instead of the return value from
    // P_SpawnPlayerMissile because we need to give info to the mobj
    // even if it exploded immediately.
    if( MissileMobj )
    {
        if(multiplayer)
        { // Multi-player game
            MissileMobj->special2 = player-players;
        }
        else
        { // Always use red missiles in single player games
            MissileMobj->special2 = 2;
        }
        // lar_linetarget returned by P_AimLineAttack, via P_SPMAngle
        if(lar_linetarget)
            MissileMobj->tracer = lar_linetarget;
        
        S_StartObjSound(MissileMobj, sfx_hrnpow);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodPL2Seek
//
//----------------------------------------------------------------------------

void A_SkullRodPL2Seek(mobj_t *actor)
{
    P_SeekerMissile(actor, ANGLE_1*10, ANGLE_1*30);
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerRain
//
//----------------------------------------------------------------------------

void A_AddPlayerRain(mobj_t *actor)
{
    int playerNum;
    player_t *player;

    playerNum = multiplayer ? actor->special2 : 0;
    if(!playeringame[playerNum])
    { // Player left the game
        return;
    }
    player = &players[playerNum];
    if(player->health <= 0)
    { // Player is dead
        return;
    }
    if(player->rain1 && player->rain2)
    { // Terminate an active rain
        if(player->rain1->health < player->rain2->health)
        {
            if(player->rain1->health > 16)
            {
                player->rain1->health = 16;
            }
            player->rain1 = NULL;
        }
        else
        {
            if(player->rain2->health > 16)
            {
                player->rain2->health = 16;
            }
            player->rain2 = NULL;
        }
    }
    // Add rain mobj to list
    if(player->rain1)
    {
        player->rain2 = actor;
    }
    else
    {
        player->rain1 = actor;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodStorm
//
//----------------------------------------------------------------------------

void A_SkullRodStorm(mobj_t *actor)
{
    fixed_t x, y;
    mobj_t *mo;
    int player_num;
    player_t *player;

    if(actor->health-- == 0)
    {
        P_SetMobjState(actor, S_NULL);
        player_num = multiplayer ? actor->special2 : 0;
        if(!playeringame[player_num])
        { // Player left the game
            return;
        }
        player = &players[player_num];
        if(player->health <= 0)
        { // Player is dead
            return;
        }
        if(player->rain1 == actor)
        {
            player->rain1 = NULL;
        }
        else if(player->rain2 == actor)
        {
            player->rain2 = NULL;
        }
        return;
    }

    if(PP_Random(ph_skullrodrain) < 25)
    { // Fudge rain frequency
        return;
    }

    x = actor->x + ((PP_Random(ph_skullrodrain)&127)-64)*FRACUNIT;
    y = actor->y + ((PP_Random(ph_skullrodrain)&127)-64)*FRACUNIT;
    mo = P_SpawnMobj(x, y, ONCEILINGZ, MT_RAINPLR1+actor->special2);
    mo->target = actor->target;
    mo->momx = 1; // Force collision detection
    mo->momz = -mo->info->speed;
    mo->special2 = actor->special2; // Transfer player number
    P_CheckMissileSpawn(mo);
    if(!(actor->special1&31))
    {
        S_StartObjSound(actor, sfx_ramrain);
    }
    actor->special1++;
}

//----------------------------------------------------------------------------
//
// PROC A_RainImpact
//
//----------------------------------------------------------------------------

void A_RainImpact(mobj_t *actor)
{
    if(actor->z > actor->floorz)
    {
        P_SetMobjState(actor, S_RAINAIRXPLR1_1+actor->special2);
    }
    else if(PP_Random(ph_rainimpact) < 40)
    {
        P_HitFloor(actor);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HideInCeiling
//
//----------------------------------------------------------------------------

void A_HideInCeiling(mobj_t *actor)
{
    actor->z = actor->ceilingz+4*FRACUNIT;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL1
//
//----------------------------------------------------------------------------

void A_FirePhoenixPL1(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;

    player->ammo[am_phoenixrod] -= USE_PHRD_AMMO_1;
    P_SpawnPlayerMissile(pmo, MT_PHOENIXFX1);
    //P_SpawnPlayerMissile(player->mo, MT_MNTRFX2);
    int angf = ANGLE_TO_FINE( pmo->angle + ANG180 );
    pmo->momx += FixedMul(4*FRACUNIT, finecosine[angf]);
    pmo->momy += FixedMul(4*FRACUNIT, finesine[angf]);
}

//----------------------------------------------------------------------------
//
// PROC A_PhoenixPuff
//
//----------------------------------------------------------------------------

void A_PhoenixPuff(mobj_t *actor)
{
    int    angf;
    mobj_t *puff;

    P_SeekerMissile(actor, ANGLE_1*5, ANGLE_1*10);

    puff = P_SpawnMobj(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
    angf = ANGLE_TO_FINE( actor->angle + ANG90 );
    puff->momx = FixedMul(FRACUNIT*1.3, finecosine[angf]);
    puff->momy = FixedMul(FRACUNIT*1.3, finesine[angf]);
    puff->momz = 0;

    puff = P_SpawnMobj(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
    angf = ANGLE_TO_FINE( actor->angle - ANG90 );
    puff->momx = FixedMul(FRACUNIT*1.3, finecosine[angf]);
    puff->momy = FixedMul(FRACUNIT*1.3, finesine[angf]);
    puff->momz = 0;
}

//----------------------------------------------------------------------------
//
// PROC A_InitPhoenixPL2
//
//----------------------------------------------------------------------------

void A_InitPhoenixPL2(player_t *player, pspdef_t *psp)
{
    player->flamecount = FLAME_THROWER_TICS;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL2
//
// Flame thrower effect.
//
//----------------------------------------------------------------------------

void A_FirePhoenixPL2(player_t *player, pspdef_t *psp)
{
    mobj_t *mo;
    mobj_t *pmo;
    angle_t angle;
    fixed_t x, y, z;
    fixed_t slope;

    if(--player->flamecount == 0)
    { // Out of flame
        P_SetPsprite(player, PS_weapon, S_PHOENIXATK2_4);
        player->refire = 0;
        return;
    }
    pmo = player->mo;
    angle = pmo->angle;
    x = pmo->x+(PP_SignedRandom(ph_phoenix2)<<9);
    y = pmo->y+(PP_SignedRandom(ph_phoenix2)<<9);
    z = pmo->z+26*FRACUNIT+((player->aiming)<<FRACBITS)/173;
    if(pmo->flags2&MF2_FEETARECLIPPED)
    {
        z -= FOOTCLIPSIZE;
    }
    slope = AIMINGTOSLOPE(player->aiming);

    mo = P_SpawnMobj(x, y, z, MT_PHOENIXFX2);
    mo->target = pmo;
    mo->angle = angle;
    int angf = ANGLE_TO_FINE(angle);
#ifdef MONSTER_VARY
    mo->momx = pmo->momx + FixedMul(mo->speed, finecosine[angf]);
    mo->momy = pmo->momy + FixedMul(mo->speed, finesine[angf]);
#else
    mo->momx = pmo->momx + FixedMul(mo->info->speed, finecosine[angf]);
    mo->momy = pmo->momy + FixedMul(mo->info->speed, finesine[angf]);
#endif
    mo->momz = FixedMul(mo->info->speed, slope);
    if(!player->refire || !(leveltime%38))
    {
        S_StartObjSound(player->mo, sfx_phopow);
    }
    P_CheckMissileSpawn(mo);
}

//----------------------------------------------------------------------------
//
// PROC A_ShutdownPhoenixPL2
//
//----------------------------------------------------------------------------

void A_ShutdownPhoenixPL2(player_t *player, pspdef_t *psp)
{
    player->ammo[am_phoenixrod] -= USE_PHRD_AMMO_2;
}

//----------------------------------------------------------------------------
//
// PROC A_FlameEnd
//
//----------------------------------------------------------------------------

void A_FlameEnd(mobj_t *actor)
{
    actor->momz += 1.5*FRACUNIT;
}

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

void A_FloatPuff(mobj_t *puff)
{
    puff->momz += 1.8*FRACUNIT;
}

//---------------------------------------------------------------------------
//
// PROC A_GauntletAttack
//
//---------------------------------------------------------------------------

void A_GauntletAttack(player_t *player, pspdef_t *psp)
{
    mobj_t * pmo = player->mo;
    angle_t angle;
    int damage;
    int slope;
    int randVal;
    fixed_t dist;

    psp->sx = ((PP_Random(ph_gauntlet)&3)-2)*FRACUNIT;
    psp->sy = WEAPONTOP+(PP_Random(ph_gauntlet)&3)*FRACUNIT;
    angle = pmo->angle;
    if(player->powers[pw_weaponlevel2])
    {
        damage = HITDICE(2);
        dist = 4*MELEERANGE;
        angle += PP_SignedRandom(ph_gauntlet)<<17;
        PuffType = MT_GAUNTLETPUFF2;
    }
    else
    {
        damage = HITDICE(2);
        dist = MELEERANGE+1;
        angle += PP_SignedRandom(ph_gauntlet)<<18;
        PuffType = MT_GAUNTLETPUFF1;
    }
    slope = P_AimLineAttack(pmo, angle, dist, 0);
    P_LineAttack(pmo, angle, dist, slope, damage);
    // lar_linetarget returned by P_LineAttack, P_AimLineAttack
    if(!lar_linetarget)
    {
        if(PP_Random(ph_gauntlet2) > 64)
        {
            player->extralight = (player->extralight)? 0:LIGHT_UNIT;
        }
        S_StartObjSound(pmo, sfx_gntful);
        return;
    }

    randVal = PP_Random(ph_gauntlet);
    if(randVal < 64)
    {
        player->extralight = 0;
    }
    else if(randVal < 160)
    {
        player->extralight = 1*LIGHT_UNIT;
    }
    else
    {
        player->extralight = 2*LIGHT_UNIT;
    }
    if(player->powers[pw_weaponlevel2])
    {
        P_GiveHealth(player, damage>>1);
        S_StartObjSound(pmo, sfx_gntpow);
    }
    else
    {
        S_StartObjSound(pmo, sfx_gnthit);
    }
    // lar_linetarget returned by P_AimLineAttack, P_LineAttack
    // turn to face target
    angle = R_PointToAngle2(pmo->x, pmo->y,
                            lar_linetarget->x, lar_linetarget->y);
    if(angle - pmo->angle > ANG180)
    {
        if(angle - pmo->angle < -ANG90/20)
          pmo->angle = angle + ANG90/21;
        else
          pmo->angle -= ANG90/20;
    }
    else
    {
        if(angle - pmo->angle > ANG90/20)
          pmo->angle = angle - ANG90/21;
        else
          pmo->angle += ANG90/20;
    }
    pmo->flags |= MF_JUSTATTACKED;
}
