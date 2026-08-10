// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_henemy.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
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
// $Log: p_henemy.c,v $
// Revision 1.6  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.5  2001/05/27 13:42:47  bpereira
//
// Revision 1.4  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.3  2001/02/24 13:35:20  bpereira
//
//
//
// DESCRIPTION:
//   this file is include by P_enemy.c
//   it contain all heretic enemy specific
//
//-----------------------------------------------------------------------------

// Macros

#define MAX_BOSS_SPOTS 8


// Types

typedef struct
{
    fixed_t x;
    fixed_t y;
    angle_t angle;
} BossSpot_t;

// Private Data

static int BossSpotCount;
static BossSpot_t BossSpots[MAX_BOSS_SPOTS];

// Proto

//----------------------------------------------------------------------------
//
// PROC P_Init_Monsters
//
// Called at level load.
//
//----------------------------------------------------------------------------

void P_Init_Monsters(void)
{
    BossSpotCount = 0;
}

//----------------------------------------------------------------------------
//
// PROC P_AddBossSpot
//
//----------------------------------------------------------------------------

void P_AddBossSpot(fixed_t x, fixed_t y, angle_t angle)
{
    if(BossSpotCount == MAX_BOSS_SPOTS)
    {
        // BP:not a critical problem 
        CONS_Printf("Too many boss spots.");
        return;
    }
    BossSpots[BossSpotCount].x = x;
    BossSpots[BossSpotCount].y = y;
    BossSpots[BossSpotCount].angle = angle;
    BossSpotCount++;
}


//---------------------------------------------------------------------------
//
// FUNC P_LookForMonsters
//
//---------------------------------------------------------------------------

#define MONS_LOOK_RANGE (20*64*FRACUNIT)
#define MONS_LOOK_LIMIT 64

boolean PH_LookForMonsters(mobj_t *actor)
{
    int count;
    mobj_t *mo;
    thinker_t *think;

    if(!P_CheckSight(players[0].mo, actor))
    { // Player can't see monster
        return(false);
    }
    count = 0;
    for(think = thinkercap.next; think != &thinkercap; think = think->next)
    {
        if(think->function != TFI_MobjThinker)
        { // Not a mobj thinker
            continue;
        }
        mo = (mobj_t *)think;
        if(!(mo->flags&MF_COUNTKILL) || (mo == actor) || (mo->health <= 0))
        { // Not a valid monster
            continue;
        }
        if(P_AproxDistance(actor->x-mo->x, actor->y-mo->y)
            > MONS_LOOK_RANGE)
        { // Out of range
            continue;
        }
        if(PP_Random(ph_lookmon) < 16)
        { // Skip
            continue;
        }
        if(count++ > MONS_LOOK_LIMIT)
        { // Stop searching
            return(false);
        }
        if(!P_CheckSight(actor, mo))
        { // Out of sight
            continue;
        }
        // Found a target monster
        actor->target = mo;
        return(true);
    }
    return(false);
}

/*
===============================================================================

                                                ACTION ROUTINES

===============================================================================
*/


//----------------------------------------------------------------------------
//
// PROC A_DripBlood
//
//----------------------------------------------------------------------------

void A_DripBlood(mobj_t *actor)
{
    mobj_t *mo;
    int r,s;

    // evaluation order isn't define in C
    r = PP_SignedRandom(ph_blooddrip);
    s = PP_SignedRandom(ph_blooddrip);
    mo = P_SpawnMobj(actor->x+(r<<11),
                     actor->y+(s<<11), actor->z, MT_BLOOD);
    mo->momx = PP_SignedRandom(ph_blooddrip)<<10;
    mo->momy = PP_SignedRandom(ph_blooddrip)<<10;
    mo->flags2 |= MF2_LOGRAV;
}

//----------------------------------------------------------------------------
//
// PROC A_KnightAttack
//
//----------------------------------------------------------------------------

void A_KnightAttack(mobj_t *actor)
{
    if(!actor->target)
    {
        return;
    }
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(3));
        S_StartObjSound(actor, sfx_kgtat2);
        return;
    }
    // Throw axe
    S_StartObjSound(actor, actor->info->attacksound);
    if(actor->type == MT_KNIGHTGHOST || PP_Random(ph_knightaxe) < 40)
    { // Red axe
        P_SpawnMissile(actor, actor->target, MT_REDAXE);
        return;
    }
    // Green axe
    P_SpawnMissile(actor, actor->target, MT_KNIGHTAXE);
}

//----------------------------------------------------------------------------
//
// PROC A_ImpExplode
//
//----------------------------------------------------------------------------

void A_ImpExplode(mobj_t *actor)
{
    mobj_t *mo;

    mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_IMPCHUNK1);
    mo->momx = PP_SignedRandom(ph_impexpl)<<10;
    mo->momy = PP_SignedRandom(ph_impexpl)<<10;
    mo->momz = 9*FRACUNIT;
    mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_IMPCHUNK2);
    mo->momx = PP_SignedRandom(ph_impexpl)<<10;
    mo->momy = PP_SignedRandom(ph_impexpl)<<10;
    mo->momz = 9*FRACUNIT;
    if(actor->special1 == 666)
    { // Extreme death crash
        P_SetMobjState(actor, S_IMP_XCRASH1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BeastPuff
//
//----------------------------------------------------------------------------

void A_BeastPuff(mobj_t *actor)
{
    if( PP_Random(ph_beastpuff) > 64 )
    {
        int r,s,t;
        r = PP_SignedRandom(ph_beastpuff);
        s = PP_SignedRandom(ph_beastpuff);
        t = PP_SignedRandom(ph_beastpuff);
        
        P_SpawnMobj(actor->x+(r<<10),
                    actor->y+(s<<10),
                    actor->z+(t<<10), MT_PUFFY);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMeAttack
//
//----------------------------------------------------------------------------

void A_ImpMeAttack(mobj_t *actor)
{
    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);

    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, 5+(PP_Random(ph_impdamage)&7));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMsAttack
//
//----------------------------------------------------------------------------

void A_ImpMsAttack(mobj_t *actor)
{
    mobj_t *dest;
    int dist;

    if( (!actor->target) || (PP_Random(ph_impattack) > 64) )
    {
        P_SetMobjState(actor, actor->info->seestate);
        return;
    }
    dest = actor->target;
    actor->flags |= MF_SKULLFLY;
    S_StartObjSound(actor, actor->info->attacksound);
    A_FaceTarget(actor);

    int angf = ANGLE_TO_FINE(actor->angle);
    actor->momx = FixedMul(12*FRACUNIT, finecosine[angf]);
    actor->momy = FixedMul(12*FRACUNIT, finesine[angf]);
    dist = P_AproxDistance(dest->x-actor->x, dest->y-actor->y);
    dist = dist/(12*FRACUNIT);
    if(dist < 1)
    {
        dist = 1;
    }
    actor->momz = (dest->z+(dest->height>>1)-actor->z)/dist;
}

//----------------------------------------------------------------------------
//
// PROC A_ImpMsAttack2
//
// Fireball attack of the imp leader.
//
//----------------------------------------------------------------------------

void A_ImpMsAttack2(mobj_t *actor)
{
    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, 5+(PP_Random(ph_impball)&7));
        return;
    }
    P_SpawnMissile(actor, actor->target, MT_IMPBALL);
}

//----------------------------------------------------------------------------
//
// PROC A_ImpDeath
//
//----------------------------------------------------------------------------

void A_ImpDeath(mobj_t *actor)
{
    actor->flags &= ~MF_SOLID;
    actor->flags2 |= MF2_FOOTCLIP;

    actor->flags  |= MF_CORPSE|MF_DROPOFF;
    actor->height >>= 2;
    actor->radius -= (actor->radius>>4);      //for solid corpses

    if(actor->z <= actor->floorz)
    {
        P_SetMobjState(actor, S_IMP_CRASH1);
        actor->flags &= ~MF_CORPSE;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ImpXDeath1
//
//----------------------------------------------------------------------------

void A_ImpXDeath1(mobj_t *actor)
{
    actor->flags &= ~MF_SOLID;
    actor->flags |= MF_NOGRAVITY;
    actor->flags  |= MF_CORPSE|MF_DROPOFF;
    actor->height >>= 2;
    actor->radius -= (actor->radius>>4);      //for solid corpses

    actor->flags2 |= MF2_FOOTCLIP;
    actor->special1 = 666; // Flag the crash routine
}

//----------------------------------------------------------------------------
//
// PROC A_ImpXDeath2
//
//----------------------------------------------------------------------------

void A_ImpXDeath2(mobj_t *actor)
{
    actor->flags &= ~MF_NOGRAVITY;
    if(actor->z <= actor->floorz)
    {
        P_SetMobjState(actor, S_IMP_CRASH1);
        actor->flags &= ~MF_CORPSE;
    }
}

//----------------------------------------------------------------------------
//
// FUNC P_UpdateChicken
//
// Returns true if the chicken morphs.
//
//----------------------------------------------------------------------------

// Monster chickens only.
// [WDJ] Fixed to Morph the same mobj,
// instead of hiding it as a corpse above the ceiling using S_FREETARGMOBJ.
boolean P_UpdateChicken(mobj_t *actor, int tics)
{
    mobjtype_t moType;

    actor->special1 -= tics;  // chicken tics for monsters
    if(actor->special1 > 0)
    {
        return false;
    }
    moType = actor->special2;  // morphs save the previous type in special2
    // morph back to monster moType
    if( ! P_MorphMobj( actor, moType, MM_testsize|MM_telefog, MF_SHADOW ))
    { // Didn't fit, restored fields
        actor->special1 = 5*TICRATE; // Next try in 5 seconds
        actor->special2 = moType;
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
//
// PROC A_ChicAttack
//
//----------------------------------------------------------------------------

void A_ChicAttack(mobj_t *actor)
{
    if(P_UpdateChicken(actor, 18))
    {
        return;
    }
    if(!actor->target)
    {
        return;
    }
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, 1+(PP_Random(ph_chickendam)&1));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_ChicLook
//
//----------------------------------------------------------------------------

void A_ChicLook(mobj_t *actor)
{
    if(P_UpdateChicken(actor, 10))
    {
        return;
    }
    A_Look(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ChicChase
//
//----------------------------------------------------------------------------

void A_ChicChase(mobj_t *actor)
{
    if(P_UpdateChicken(actor, 3))
    {
        return;
    }
    A_Chase(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_ChicPain
//
//----------------------------------------------------------------------------

void A_ChicPain(mobj_t *actor)
{
    if(P_UpdateChicken(actor, 10))
    {
        return;
    }
    S_StartObjSound(actor, actor->info->painsound);
}

//----------------------------------------------------------------------------
//
// PROC A_Feathers
//
//----------------------------------------------------------------------------

void A_Feathers(mobj_t *actor)
{
    int i;
    int count;
    mobj_t *mo;

    if(actor->health > 0)
    { // Pain
        count = PP_Random(ph_feathers) < 32 ? 2 : 1;
    }
    else
    { // Death
        count = 5+(PP_Random(ph_feathers)&3);
    }

    for(i = 0; i < count; i++)
    {
        // create feathers
        mo = P_SpawnMobj(actor->x, actor->y, actor->z+20*FRACUNIT, MT_FEATHER);
        mo->target = actor;
        mo->momx = PP_SignedRandom(ph_feathers)<<8;
        mo->momy = PP_SignedRandom(ph_feathers)<<8;
        mo->momz = FRACUNIT+(PP_Random(ph_feathers)<<9);
        P_SetMobjState(mo, S_FEATHER1+(PP_Random(ph_feathers)&7));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MummyAttack
//
//----------------------------------------------------------------------------

void A_MummyAttack(mobj_t *actor)
{
    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(2));
        S_StartObjSound(actor, sfx_mumat2);
        return;
    }
    S_StartObjSound(actor, sfx_mumat1);
}

//----------------------------------------------------------------------------
//
// PROC A_MummyAttack2
//
// Mummy leader missile attack.
//
//----------------------------------------------------------------------------

void A_MummyAttack2(mobj_t *actor)
{
    mobj_t *mo;

    if(!actor->target)
    {
        return;
    }
    //S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(2));
        return;
    }
    mo = P_SpawnMissile(actor, actor->target, MT_MUMMYFX1);
    //mo = P_SpawnMissile(actor, actor->target, MT_EGGFX);
    if(mo != NULL)
    {
        mo->tracer = actor->target;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MummyFX1Seek
//
//----------------------------------------------------------------------------

void A_MummyFX1Seek(mobj_t *actor)
{
    P_SeekerMissile(actor, ANGLE_1*10, ANGLE_1*20);
}

//----------------------------------------------------------------------------
//
// PROC A_MummySoul
//
//----------------------------------------------------------------------------

void A_MummySoul(mobj_t *mummy)
{
    mobj_t *mo;

    mo = P_SpawnMobj(mummy->x, mummy->y, mummy->z+10*FRACUNIT, MT_MUMMYSOUL);
    mo->momz = FRACUNIT;
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Pain
//
//----------------------------------------------------------------------------

void A_Sor1Pain(mobj_t *actor)
{
    actor->special1 = 20; // Number of steps to walk fast
    A_Pain(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Chase
//
//----------------------------------------------------------------------------

void A_Sor1Chase(mobj_t *actor)
{
    if(actor->special1)
    {
        actor->special1--;
        actor->tics -= 3;
    }
    A_Chase(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr1Attack
//
// Sorcerer demon attack.
//
//----------------------------------------------------------------------------

void A_Srcr1Attack(mobj_t *actor)
{
    mobj_t *mo;
    fixed_t momz;
    angle_t angle;

    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(8));
        return;
    }
    if(actor->health > (actor->info->spawnhealth/3)*2)
    { // Spit one fireball
        P_SpawnMissile(actor, actor->target, MT_SRCRFX1);
    }
    else
    { // Spit three fireballs
        mo = P_SpawnMissile(actor, actor->target, MT_SRCRFX1);
        if(mo)
        {
            momz = mo->momz;
            angle = mo->angle;
            P_SpawnMissileAngle(actor, MT_SRCRFX1, angle-ANGLE_1*3, momz);
            P_SpawnMissileAngle(actor, MT_SRCRFX1, angle+ANGLE_1*3, momz);
        }
        if(actor->health < actor->info->spawnhealth/3)
        { // Maybe attack again
            if(actor->special1)
            { // Just attacked, so don't attack again
                actor->special1 = 0;
            }
            else
            { // Set state to attack again
                actor->special1 = 1;
                P_SetMobjState(actor, S_SRCR1_ATK4);
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SorcererRise
//
//----------------------------------------------------------------------------

void A_SorcererRise(mobj_t *actor)
{
    mobj_t *mo;

    actor->flags &= ~MF_SOLID;
    mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_SORCERER2);
    P_SetMobjState(mo, S_SOR2_RISE1);
    mo->angle = actor->angle;
    mo->target = actor->target;
}

//----------------------------------------------------------------------------
//
// PROC P_DSparilTeleport
//
//----------------------------------------------------------------------------

void P_DSparilTeleport(mobj_t *actor)
{
    int i;
    fixed_t x, y;
    fixed_t prev_x, prev_y, prev_z;
    mobj_t *mo;

    if(!BossSpotCount)
    { // No spots
        return;
    }

    i = PP_Random(ph_sortele1);
    do
    {
        i++;
        x = BossSpots[i%BossSpotCount].x;
        y = BossSpots[i%BossSpotCount].y;
    } while(P_AproxDistance(actor->x-x, actor->y-y) < 128*FRACUNIT);
    prev_x = actor->x;
    prev_y = actor->y;
    prev_z = actor->z;
    if(P_TeleportMove(actor, x, y, true)) // stomp uses MF2_TELESTOMP
    {
        mo = P_SpawnMobj(prev_x, prev_y, prev_z, MT_SOR2TELEFADE);
        S_StartObjSound(mo, sfx_telept);
        P_SetMobjState(actor, S_SOR2_TELE1);
        S_StartObjSound(actor, sfx_telept);
        actor->z = actor->floorz;
        actor->angle = BossSpots[i%BossSpotCount].angle;
        actor->momx = actor->momy = actor->momz = 0;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Decide
//
//----------------------------------------------------------------------------

void A_Srcr2Decide(mobj_t *actor)
{
    static int chance[] =
    {
        192, 120, 120, 120, 64, 64, 32, 16, 0
    };

    if(!BossSpotCount)
    { // No spots
        return;
    }

    if(PP_Random(ph_sortele2) < chance[actor->health/(actor->info->spawnhealth/8)])
    {
        P_DSparilTeleport(actor);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Attack
//
//----------------------------------------------------------------------------

void A_Srcr2Attack(mobj_t *actor)
{
    int chance;

    if(!actor->target)
    {
        return;
    }
    S_StartSound( actor->info->attacksound );
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(20));
        return;
    }
    chance = actor->health < actor->info->spawnhealth/2 ? 96 : 48;
    if(PP_Random(ph_sormissile) < chance)
    { // Wizard spawners
        P_SpawnMissileAngle(actor, MT_SOR2FX2, actor->angle-ANG45, FRACUNIT/2);
        P_SpawnMissileAngle(actor, MT_SOR2FX2, actor->angle+ANG45, FRACUNIT/2);
    }
    else
    { // Blue bolt
        P_SpawnMissile(actor, actor->target, MT_SOR2FX1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BlueSpark
//
//----------------------------------------------------------------------------

void A_BlueSpark(mobj_t *actor)
{
    int i;
    mobj_t *mo;
    
    for(i = 0; i < 2; i++)
    {
        mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_SOR2FXSPARK);
        mo->momx = PP_SignedRandom(ph_sorsparkmom)<<9;
        mo->momy = PP_SignedRandom(ph_sorsparkmom)<<9;
        mo->momz = FRACUNIT+(PP_Random(ph_sorsparkmom)<<8);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_GenWizard
//
//----------------------------------------------------------------------------

void A_GenWizard(mobj_t *actor)
{
    mobj_t *mo;
    mobj_t *fog;

    mo = P_SpawnMobj(actor->x, actor->y,
                     actor->z-mobjinfo[MT_WIZARD].height/2, MT_WIZARD);
    if(P_TestMobjLocation(mo) == false)
    { // Didn't fit
        P_RemoveMobj(mo);
        return;
    }
    actor->momx = actor->momy = actor->momz = 0;
    P_SetMobjState(actor, mobjinfo[actor->type].deathstate);
    actor->flags &= ~MF_MISSILE;
    fog = P_SpawnMobj(actor->x, actor->y, actor->z, MT_TFOG);
    S_StartObjSound(fog, sfx_telept);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthInit
//
//----------------------------------------------------------------------------

void A_Sor2DthInit(mobj_t *actor)
{
    actor->special1 = 7; // Animation loop counter
    P_Massacre(); // Kill monsters early
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthLoop
//
//----------------------------------------------------------------------------

void A_Sor2DthLoop(mobj_t *actor)
{
    if(--actor->special1)
    { // Need to loop
        P_SetMobjState(actor, S_SOR2_DIE4);
    }
}

//----------------------------------------------------------------------------
//
// D'Sparil Sound Routines
//
//----------------------------------------------------------------------------

void A_SorZap(mobj_t *actor) {S_StartSound(sfx_sorzap);}
void A_SorRise(mobj_t *actor) {S_StartSound(sfx_sorrise);}
void A_SorDSph(mobj_t *actor) {S_StartSound(sfx_sordsph);}
void A_SorDExp(mobj_t *actor) {S_StartSound(sfx_sordexp);}
void A_SorDBon(mobj_t *actor) {S_StartSound(sfx_sordbon);}
void A_SorSightSnd(mobj_t *actor) {S_StartSound(sfx_sorsit);}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk1
//
// Melee attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk1(mobj_t *actor)
{
    player_t *player;

    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, sfx_stfpow);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(4));
        if((player = actor->target->player) != NULL)
        { // Squish the player
            player->deltaviewheight = -16*FRACUNIT;
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurDecide
//
// Choose a missile attack.
//
//----------------------------------------------------------------------------

#define MNTR_CHARGE_SPEED (13*FRACUNIT)

void A_MinotaurDecide(mobj_t *actor)
{
    int dist;
    mobj_t *target;

    target = actor->target;
    if(!target)
    {
        return;
    }
    S_StartObjSound(actor, sfx_minsit);
    dist = P_AproxDistance(actor->x-target->x, actor->y-target->y);
    if(target->z+target->height > actor->z
        && target->z+target->height < actor->z+actor->height
        && dist < 8*64*FRACUNIT
        && dist > 1*64*FRACUNIT
        && PP_Random(ph_minocharge) < 150)
    { // Charge attack
      // Don't call the state function right away
        P_SetMobjStateNF(actor, S_MNTR_ATK4_1);
        actor->flags |= MF_SKULLFLY;
        A_FaceTarget(actor);
        int angf = ANGLE_TO_FINE(actor->angle);
        actor->momx = FixedMul(MNTR_CHARGE_SPEED, finecosine[angf]);
        actor->momy = FixedMul(MNTR_CHARGE_SPEED, finesine[angf]);
        actor->special1 = TICRATE/2; // Charge duration
    }
    else if(target->z == target->floorz
            && dist < 9*64*FRACUNIT
            && PP_Random(ph_minofire) < 220)
    { // Floor fire attack
        P_SetMobjState(actor, S_MNTR_ATK3_1);
        actor->special2 = 0;
    }
    else
    { // Swing attack
        A_FaceTarget(actor);
        // Don't need to call P_SetMobjState because the current state
        // falls through to the swing attack
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurCharge
//
//----------------------------------------------------------------------------

void A_MinotaurCharge(mobj_t *actor)
{
    mobj_t *puff;

    if(actor->special1)
    {
        puff = P_SpawnMobj(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
        puff->momz = 2*FRACUNIT;
        actor->special1--;
    }
    else
    {
        actor->flags &= ~MF_SKULLFLY;
        P_SetMobjState(actor, actor->info->seestate);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk2
//
// Swing attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk2(mobj_t *actor)
{
    mobj_t *mo;
    angle_t angle;
    fixed_t momz;

    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, sfx_minat2);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(5));
        return;
    }
    mo = P_SpawnMissile(actor, actor->target, MT_MNTRFX1);
    if(mo)
    {
        S_StartObjSound(mo, sfx_minat2);
        momz = mo->momz;
        angle = mo->angle;
        P_SpawnMissileAngle(actor, MT_MNTRFX1, angle-(ANG45/8), momz);
        P_SpawnMissileAngle(actor, MT_MNTRFX1, angle+(ANG45/8), momz);
        P_SpawnMissileAngle(actor, MT_MNTRFX1, angle-(ANG45/16), momz);
        P_SpawnMissileAngle(actor, MT_MNTRFX1, angle+(ANG45/16), momz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk3
//
// Floor fire attack.
//
//----------------------------------------------------------------------------

void A_MinotaurAtk3(mobj_t *actor)
{
    mobj_t *mo;
    player_t *player;

    if(!actor->target)
    {
        return;
    }
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(5));
        if((player = actor->target->player) != NULL)
        { // Squish the player
            player->deltaviewheight = -16*FRACUNIT;
        }
    }
    else
    {
        mo = P_SpawnMissile(actor, actor->target, MT_MNTRFX2);
        if(mo != NULL)
        {
            S_StartObjSound(mo, sfx_minat1);
        }
    }
    if(PP_Random(ph_minoatk3) < 192 && actor->special2 == 0)
    {
        P_SetMobjState(actor, S_MNTR_ATK3_4);
        actor->special2 = 1;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MntrFloorFire
//
//----------------------------------------------------------------------------

void A_MntrFloorFire(mobj_t *actor)
{
    mobj_t *mo;
    int r,s;
    
    actor->z = actor->floorz;
    r = PP_SignedRandom(ph_minofire);
    s = PP_SignedRandom(ph_minofire);
    mo = P_SpawnMobj(actor->x+(r<<10),
                     actor->y+(s<<10), ONFLOORZ, MT_MNTRFX3);
    mo->target = actor->target;
    mo->momx = 1; // Force block checking
    P_CheckMissileSpawn(mo);
}

//----------------------------------------------------------------------------
//
// PROC A_BeastAttack
//
//----------------------------------------------------------------------------

void A_BeastAttack(mobj_t *actor)
{
    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(3));
        return;
    }
    P_SpawnMissile(actor, actor->target, MT_BEASTBALL);
}

//----------------------------------------------------------------------------
//
// PROC A_HeadAttack
//
//----------------------------------------------------------------------------

void A_HHeadAttack(mobj_t *actor)
{
    static int atkResolve1[] = { 50, 150 };
    static int atkResolve2[] = { 150, 200 };
   
    int i;
    mobj_t *fire;
    mobj_t *baseFire;
    mobj_t *mo;
    mobj_t *target;
    int randAttack;
    int dist;

    // Ice ball             (close 20% : far 60%)
    // Fire column  (close 40% : far 20%)
    // Whirlwind    (close 40% : far 20%)
    // Distance threshold = 8 cells

    target = actor->target;
    if(target == NULL)
    {
        return;
    }
    A_FaceTarget(actor);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(target, actor, actor, HITDICE(6));
        return;
    }
    dist = P_AproxDistance(actor->x-target->x, actor->y-target->y)
           > 8*64*FRACUNIT;
    randAttack = PP_Random(ph_headatk);
    if(randAttack < atkResolve1[dist])
    { // Ice ball
        P_SpawnMissile(actor, target, MT_HEADFX1);
        S_StartObjSound(actor, sfx_hedat2);
    }
    else if(randAttack < atkResolve2[dist])
    { // Fire column
        baseFire = P_SpawnMissile(actor, target, MT_HEADFX3);
        if(baseFire != NULL)
        {
            P_SetMobjState(baseFire, S_HHEADFX3_4); // Don't grow
            for(i = 0; i < 5; i++)
            {
                fire = P_SpawnMobj(baseFire->x, baseFire->y, baseFire->z, MT_HEADFX3);
                if(i == 0)
                {
                    S_StartObjSound(actor, sfx_hedat1);
                }
                fire->target = baseFire->target;
                fire->angle = baseFire->angle;
                fire->momx = baseFire->momx;
                fire->momy = baseFire->momy;
                fire->momz = baseFire->momz;
                //fire->damage = 0;
                fire->health = (i+1)*2;
                P_CheckMissileSpawn(fire);
            }
        }
    }
    else
    { // Whirlwind
        mo = P_SpawnMissile(actor, target, MT_WHIRLWIND);
        if(mo != NULL)
        {
            mo->z -= 32*FRACUNIT;
            mo->tracer = target;
            mo->special2 = 50; // Timer for active sound
            mo->health = 20*TICRATE; // Duration
            S_StartObjSound(actor, sfx_hedat3);
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_WhirlwindSeek
//
//----------------------------------------------------------------------------

void A_WhirlwindSeek(mobj_t *actor)
{
    actor->health -= 3;
    if(actor->health < 0)
    {
        actor->momx = actor->momy = actor->momz = 0;
        P_SetMobjState(actor, mobjinfo[actor->type].deathstate);
        actor->flags &= ~MF_MISSILE;
        return;
    }
    if((actor->special2 -= 3) < 0)
    {
        actor->special2 = 58+(PP_Random(ph_headwind)&31);
        S_StartObjSound(actor, sfx_hedat3);
    }
    if(actor->tracer
       && actor->tracer->flags&MF_SHADOW)
    {
        return;
    }
    P_SeekerMissile(actor, ANGLE_1*10, ANGLE_1*30);
}

//----------------------------------------------------------------------------
//
// PROC A_HeadIceImpact
//
//----------------------------------------------------------------------------

void A_HeadIceImpact(mobj_t *ice)
{
    int i;
    angle_t angle;
    mobj_t *shard;

    for(i = 0; i < 8; i++)
    {
        shard = P_SpawnMobj(ice->x, ice->y, ice->z, MT_HEADFX2);
        shard->target = ice->target;
        angle = i*ANG45;
        shard->angle = angle;
#ifdef MONSTER_VARY
        shard->momx = FixedMul(shard->speed, cosine_ANG(angle));
        shard->momy = FixedMul(shard->speed, sine_ANG(angle));
#else
        shard->momx = FixedMul(shard->info->speed, cosine_ANG(angle));
        shard->momy = FixedMul(shard->info->speed, sine_ANG(angle));
#endif
        shard->momz = -.6*FRACUNIT;
        P_CheckMissileSpawn(shard);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HeadFireGrow
//
//----------------------------------------------------------------------------

void A_HeadFireGrow(mobj_t *fire)
{
    fire->health--;
    fire->z += 9*FRACUNIT;
    if(fire->health == 0)
    {
        //fire->damage = fire->info->damage;
        P_SetMobjState(fire, S_HHEADFX3_4);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SnakeAttack
//
//----------------------------------------------------------------------------

void A_SnakeAttack(mobj_t *actor)
{
    if(!actor->target)
    {
        P_SetMobjState(actor, S_SNAKE_WALK1);
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    A_FaceTarget(actor);
    P_SpawnMissile(actor, actor->target, MT_SNAKEPRO_A);
}

//----------------------------------------------------------------------------
//
// PROC A_SnakeAttack2
//
//----------------------------------------------------------------------------

void A_SnakeAttack2(mobj_t *actor)
{
    if(!actor->target)
    {
        P_SetMobjState(actor, S_SNAKE_WALK1);
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    A_FaceTarget(actor);
    P_SpawnMissile(actor, actor->target, MT_SNAKEPRO_B);
}

//----------------------------------------------------------------------------
//
// PROC A_ClinkAttack
//
//----------------------------------------------------------------------------

void A_ClinkAttack(mobj_t *actor)
{
    int damage;

    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        damage = ((PP_Random(ph_clinkdam)%7)+3);
        P_DamageMobj(actor->target, actor, actor, damage);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_GhostOff
//
//----------------------------------------------------------------------------

void A_GhostOff(mobj_t *actor)
{
    actor->flags &= ~MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk1
//
//----------------------------------------------------------------------------

void A_WizAtk1(mobj_t *actor)
{
    A_FaceTarget(actor);
    actor->flags &= ~MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk2
//
//----------------------------------------------------------------------------

void A_WizAtk2(mobj_t *actor)
{
    A_FaceTarget(actor);
    actor->flags |= MF_SHADOW;
}

//----------------------------------------------------------------------------
//
// PROC A_WizAtk3
//
//----------------------------------------------------------------------------

void A_WizAtk3(mobj_t *actor)
{
    mobj_t *mo;
    angle_t angle;
    fixed_t momz;

    actor->flags &= ~MF_SHADOW;
    if(!actor->target)
    {
        return;
    }
    S_StartObjSound(actor, actor->info->attacksound);
    if(P_CheckMeleeRange(actor))
    {
        P_DamageMobj(actor->target, actor, actor, HITDICE(4));
        return;
    }
    mo = P_SpawnMissile(actor, actor->target, MT_WIZFX1);
    if(mo)
    {
        momz = mo->momz;
        angle = mo->angle;
        P_SpawnMissileAngle(actor, MT_WIZFX1, angle-(ANG45/8), momz);
        P_SpawnMissileAngle(actor, MT_WIZFX1, angle+(ANG45/8), momz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_Scream
//
//----------------------------------------------------------------------------

void A_HScream(mobj_t *actor)
{
    switch(actor->type)
    {
     case MT_CHICPLAYER:
     case MT_SORCERER1:
     case MT_MINOTAUR:
        // Make boss death sounds full volume
        S_StartSound(actor->info->deathsound);
        break;
     case MT_PLAYER:
        // Handle the different player death screams
        if(actor->special1 < 10)
        { // Wimpy death sound
            S_StartObjSound(actor, sfx_plrwdth);
        }
        else if(actor->health > -50)
        { // Normal death sound
            S_StartObjSound(actor, actor->info->deathsound);
        }
        else if(actor->health > -100)
        { // Crazy death sound
            S_StartObjSound(actor, sfx_plrcdth);
        }
        else
        { // Extreme death sound
            S_StartObjSound(actor, sfx_gibdth);
        }
        break;
     default:
        S_StartObjSound(actor, actor->info->deathsound);
        break;
    }
}

//---------------------------------------------------------------------------
//
// PROC P_DropItem
//
//---------------------------------------------------------------------------

void P_DropItem(mobj_t *source, mobjtype_t type, int special, int chance)
{
    mobj_t *mo;

    if( PP_Random(ph_dropitem) > chance )
    {
        return;
    }

    mo = P_SpawnMobj(source->x, source->y, source->z+(source->height>>1), type);
    mo->momx = PP_SignedRandom(ph_dropmom)<<8;
    mo->momy = PP_SignedRandom(ph_dropmom)<<8;
    mo->momz = FRACUNIT*5+(PP_Random(ph_dropmom)<<10);
    mo->flags |= MF_DROPPED;
    mo->health = special;
}

//----------------------------------------------------------------------------
//
// PROC A_NoBlocking
//
//----------------------------------------------------------------------------

void A_NoBlocking(mobj_t *actor)
{
    A_Fall(actor);
    // Check for monsters dropping things
    switch(actor->type)
    {
        case MT_MUMMY:
        case MT_MUMMYLEADER:
        case MT_MUMMYGHOST:
        case MT_MUMMYLEADERGHOST:
            P_DropItem(actor, MT_AMGWNDWIMPY, 3, 84);
            break;
        case MT_KNIGHT:
        case MT_KNIGHTGHOST:
            P_DropItem(actor, MT_AMCBOWWIMPY, 5, 84);
            break;
        case MT_WIZARD:
            P_DropItem(actor, MT_AMBLSRWIMPY, 10, 84);
            P_DropItem(actor, MT_ARTITOMEOFPOWER, 0, 4);
            break;
        case MT_HHEAD:
            P_DropItem(actor, MT_AMBLSRWIMPY, 10, 84);
            P_DropItem(actor, MT_ARTIEGG, 0, 51);
            break;
        case MT_BEAST:
            P_DropItem(actor, MT_AMCBOWWIMPY, 10, 84);
            break;
        case MT_CLINK:
            P_DropItem(actor, MT_AMSKRDWIMPY, 20, 84);
            break;
        case MT_SNAKE:
            P_DropItem(actor, MT_AMPHRDWIMPY, 5, 84);
            break;
        case MT_MINOTAUR:
            P_DropItem(actor, MT_ARTISUPERHEAL, 0, 51);
            P_DropItem(actor, MT_AMPHRDWIMPY, 10, 84);
            break;
        default:
            break;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_PodPain
//
//----------------------------------------------------------------------------

void A_PodPain(mobj_t *actor)
{
    int i;
    int count;
    int chance;
    mobj_t *goo;
    
    chance = PP_Random(ph_podgoo);
    if(chance < 128)
    {
        return;
    }
    count = chance > 240 ? 2 : 1;
    for(i = 0; i < count; i++)
    {
        goo = P_SpawnMobj(actor->x, actor->y,
            actor->z+48*FRACUNIT, MT_PODGOO);
        goo->target = actor;
        goo->momx = PP_SignedRandom(ph_podgoomom)<<9;
        goo->momy = PP_SignedRandom(ph_podgoomom)<<9;
        goo->momz = FRACUNIT/2+(PP_Random(ph_podgoomom)<<9);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_RemovePod
//
//----------------------------------------------------------------------------

void A_RemovePod(mobj_t *actor)
{
    mobj_t *mo;

    if(actor->target)
    {
        mo = actor->target;
        if(mo->special1 > 0)
        {
            mo->special1--;
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MakePod
//
//----------------------------------------------------------------------------

#define MAX_GEN_PODS 16

void A_MakePod(mobj_t *actor)
{
    mobj_t *mo;
    fixed_t x, y;

    if(actor->special1 == MAX_GEN_PODS)
    { // Too many generated pods
        return;
    }

    x = actor->x;
    y = actor->y;
    mo = P_SpawnMobj(x, y, ONFLOORZ, MT_POD);
    if(P_CheckPosition(mo, x, y) == false)
    { // Didn't fit
        P_RemoveMobj(mo);
        return;
    }

    P_SetMobjState(mo, S_POD_GROW1);
    P_ThrustMobj(mo, PP_Random(ph_podthrust)<<24, (fixed_t)(4.5*FRACUNIT));
    S_StartObjSound(mo, sfx_newpod);
    actor->special1++; // Increment generated pod count

    //mo->special2 = (int)actor; // Link the generator to the pod
    mo->target = actor; // [smite] here target means owner/parent, naturally! I'm so happy we got rid of this in Legacy2.
    return;
}

//----------------------------------------------------------------------------
//
// PROC P_Massacre
//
// Kills all monsters.
//
//----------------------------------------------------------------------------

void P_Massacre(void)
{
    mobj_t *mo;
    thinker_t *think;

    for(think = thinkercap.next; think != &thinkercap; think = think->next)
    {
        if(think->function != TFI_MobjThinker)
        { // Not a mobj thinker
            continue;
        }
        mo = (mobj_t *)think;
        if((mo->flags&MF_COUNTKILL) && (mo->health > 0))
        {
            P_DamageMobj(mo, NULL, NULL, 10000);
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC A_BossDeath
//
// Trigger special effects if all bosses are dead.
//
//----------------------------------------------------------------------------

void A_HBossDeath(mobj_t *actor)
{
// Heretic:
// E1M8: When last IronLiche is dead, execute lowerFloor(sectors tagged 666).
// E2M8: When last Maulotar is dead, execute lowerFloor(sectors tagged 666),
//	    and Massacre.
// E3M8: When D'Sparil is dead, execute lowerFloor(sectors tagged 666),
//       and Massacre, (which has already been done by D'Sparil death).
// E4M8: When last IronLiche is dead, execute lowerFloor(sectors tagged 666),
//	    and Massacre.
// E5M8: When last Maulotar is dead, execute lowerFloor(sectors tagged 666),
//	    and Massacre.

    // indexed by game episode number -1
    static const mobjtype_t bossType[6] =
    {
        MT_HHEAD,	// episode 1
        MT_MINOTAUR,
        MT_SORCERER2,
        MT_HHEAD,
        MT_MINOTAUR,	// episode 5
        -1
    };

    mobj_t *mo;
    thinker_t *think;
    line_t dummyLine;

    if(gamemap != 8)
    { // Not a boss level
        return;
    }
    if(actor->type != bossType[gameepisode-1])
    { // Not considered a boss in this episode
        return;
    }
    // Make sure all bosses are dead
    for(think = thinkercap.next; think != &thinkercap; think = think->next)
    {
        if(think->function != TFI_MobjThinker)
        { // Not a mobj thinker
            continue;
        }
        mo = (mobj_t *)think;
        // [WDJ] Corpse health is < 0.
        // If two monsters are killed at the same time, this test may occur
        // while first is corpse and second is not.  But the simple health
        // test may trigger twice because second monster already has
        // health < 0 during the first death test.
//        if((mo != actor) && (mo->type == actor->type) && (mo->health > 0))
        if((mo != actor) && (mo->type == actor->type))
        {
            if( !(mo->flags & MF_CORPSE)
                || (mo->health > 0) )
                return;  // Found a less than dead boss
        }
    }
    if(gameepisode > 1)
    { // Kill any remaining monsters
        P_Massacre();
    }
    // lowerFloor all sectors tagged 666
    dummyLine.tag = 666;
    EV_DoFloor( &dummyLine, FT_lowerFloor);
}

//----------------------------------------------------------------------------
//
// PROC A_ESound
//
//----------------------------------------------------------------------------

void A_ESound(mobj_t *mo)
{
    int sound;

    switch(mo->type)
    {
     case MT_SOUNDWATERFALL:
        sound = sfx_waterfl;
        break;
     case MT_SOUNDWIND:
        sound = sfx_wind;
        break;
     default:
        sound = sfx_None;
        break;
    }
    S_StartObjSound(mo, sound);
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnTeleGlitter
//
//----------------------------------------------------------------------------

void A_SpawnTeleGlitter(mobj_t *actor)
{
    mobj_t *mo;

    int r = PP_Random(ph_teleglitter1);
    int s = PP_Random(ph_teleglitter1);
    mo = P_SpawnMobj(actor->x+((r&31)-16)*FRACUNIT,
                     actor->y+((s&31)-16)*FRACUNIT,
                     actor->subsector->sector->floorheight, MT_TELEGLITTER);
    mo->momz = FRACUNIT/4;
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnTeleGlitter2
//
//----------------------------------------------------------------------------

void A_SpawnTeleGlitter2(mobj_t *actor)
{
    mobj_t *mo;

    int r = PP_Random(ph_teleglitter2);
    int s = PP_Random(ph_teleglitter2);
    mo = P_SpawnMobj(actor->x+((r&31)-16)*FRACUNIT,
                     actor->y+((s&31)-16)*FRACUNIT,
                     actor->subsector->sector->floorheight, MT_TELEGLITTER2);
    mo->momz = FRACUNIT/4;
}

//----------------------------------------------------------------------------
//
// PROC A_AccTeleGlitter
//
//----------------------------------------------------------------------------

void A_AccTeleGlitter(mobj_t *actor)
{
    if(++actor->health > 35)
    {
        actor->momz += actor->momz/2;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_InitKeyGizmo
//
//----------------------------------------------------------------------------

void A_InitKeyGizmo(mobj_t *gizmo)
{
    mobj_t *mo;
    statenum_t state;

    switch(gizmo->type)
    {
     case MT_KEYGIZMOBLUE:
        state = S_KGZ_BLUEFLOAT1;
        break;
     case MT_KEYGIZMOGREEN:
        state = S_KGZ_GREENFLOAT1;
        break;
     case MT_KEYGIZMOYELLOW:
        state = S_KGZ_YELLOWFLOAT1;
        break;
     default:
        state = S_NULL;
        break;
    }
    mo = P_SpawnMobj(gizmo->x, gizmo->y, gizmo->z+60*FRACUNIT, MT_KEYGIZMOFLOAT);
    P_SetMobjState(mo, state);
}

//----------------------------------------------------------------------------
//
// PROC A_VolcanoSet
//
//----------------------------------------------------------------------------

void A_VolcanoSet(mobj_t *volcano)
{
    volcano->tics = 105+(PP_Random(ph_volcanotic)&127);
}

//----------------------------------------------------------------------------
//
// PROC A_VolcanoBlast
//
//----------------------------------------------------------------------------

void A_VolcanoBlast(mobj_t *volcano)
{
    int i, count;
    mobj_t *blast;
    angle_t angle;

    count = 1+(PP_Random(ph_volcanobl)%3);
    for(i = 0; i < count; i++)
    {
        blast = P_SpawnMobj(volcano->x, volcano->y,
                            volcano->z+44*FRACUNIT, MT_VOLCANOBLAST); // MT_VOLCANOBLAST
        blast->target = volcano;
        angle = PP_Random(ph_volcanobl)<<24;
        blast->angle = angle;
        blast->momx = FixedMul(1*FRACUNIT, cosine_ANG(angle));
        blast->momy = FixedMul(1*FRACUNIT, sine_ANG(angle));
        blast->momz = (2.5*FRACUNIT)+(PP_Random(ph_volcanobl)<<10);
        S_StartObjSound(blast, sfx_volsht);
        P_CheckMissileSpawn(blast);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_VolcBallImpact
//
//----------------------------------------------------------------------------

void A_VolcBallImpact(mobj_t *ball)
{
    int i;
    mobj_t *tiny;
    angle_t angle;

    if(ball->z <= ball->floorz)
    {
        ball->flags |= MF_NOGRAVITY;
        ball->flags2 &= ~MF2_LOGRAV;
        ball->z += 28*FRACUNIT;
        //ball->momz = 3*FRACUNIT;
    }
    P_RadiusAttack(ball, ball->target, 25);
    for(i = 0; i < 4; i++)
    {
        tiny = P_SpawnMobj(ball->x, ball->y, ball->z, MT_VOLCANOTBLAST);
        tiny->target = ball;
        angle = i*ANG90;
        tiny->angle = angle;
        tiny->momx = FixedMul(FRACUNIT*.7, cosine_ANG(angle));
        tiny->momy = FixedMul(FRACUNIT*.7, sine_ANG(angle));
        tiny->momz = FRACUNIT+(PP_Random(ph_volcanohit)<<9);
        P_CheckMissileSpawn(tiny);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SkullPop
//
//----------------------------------------------------------------------------

// Player only.
// [WDJ] Fixed to not change the player mobj.
void A_SkullPop(mobj_t *actor)
{
    player_t *player = actor->player;

    actor->flags &= ~MF_SOLID;
#if 1
    // Morph player mobj to bloody skull.
    // Player is bloody skull, closest to original code.
    int health = actor->health;  // usually 0 as this is used in death seq.
    P_MorphMobj( actor, MT_BLOODYSKULL, 0, 0 );
    actor->z += 48*FRACUNIT;
    actor->momx = PP_SignedRandom(ph_bloodyskullmom)<<9;
    actor->momy = PP_SignedRandom(ph_bloodyskullmom)<<9;
    actor->momz = FRACUNIT*2+(PP_Random(ph_bloodyskullmom)<<6);
    actor->health = health;
#else
    // Spawn bloody skull as separate object
    // Allows player to see bloody skull.
    mobj_t *mo;
    mo = P_SpawnMobj(actor->x, actor->y, actor->z+48*FRACUNIT, MT_BLOODYSKULL);
    //mo->target = actor;
    mo->momx = PP_SignedRandom(ph_bloodyskullmom)<<9;
    mo->momy = PP_SignedRandom(ph_bloodyskullmom)<<9;
    mo->momz = FRACUNIT*2+(PP_Random(ph_bloodyskullmom)<<6);
    mo->angle = actor->angle;
    mo->health = actor->health;
#if 0000
    // Attach player mobj to bloody skull
    // [WDJ] No, would leave current player mobj unattached.
    // Legacy2 does not attach skull either.
    actor->player = NULL;
    mo->player = player;
    player->mo = mo;  // change player mobj **
    // This leaves old player mobj unattached as leaking memory.
#endif
#endif
    player->aiming = 0;
    player->damagecount = 32;
}


//----------------------------------------------------------------------------
//
// PROC A_CheckSkullFloor
//
//----------------------------------------------------------------------------

void A_CheckSkullFloor(mobj_t *actor)
{
    if(actor->z <= actor->floorz)
    {
        P_SetMobjState(actor, S_BLOODYSKULLX1);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_CheckSkullDone
//
//----------------------------------------------------------------------------

void A_CheckSkullDone(mobj_t *actor)
{
    if(actor->special2 == 666)
    {
        P_SetMobjState(actor, S_BLOODYSKULLX2);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_CheckBurnGone
//
//----------------------------------------------------------------------------

void A_CheckBurnGone(mobj_t *actor)
{
    if(actor->special2 == 666)
    {
        //P_SetMobjState(actor, S_PLAY_FDTH20);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FreeTargMobj
//
//----------------------------------------------------------------------------

// Hide previous player mobj as corpse above the ceiling.
// [WDJ] Uses of this have been changed to reuse the same mobj.
void A_FreeTargMobj(mobj_t *mo)
{
    mo->momx = mo->momy = mo->momz = 0;
    mo->z = mo->ceilingz+4*FRACUNIT;
    mo->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY|MF_SOLID);
    mo->flags |= MF_CORPSE|MF_DROPOFF|MF_NOGRAVITY;
    mo->flags2 &= ~(MF2_PASSMOBJ|MF2_LOGRAV);
    mo->player = NULL;
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerCorpse
//
//----------------------------------------------------------------------------

void A_AddPlayerCorpse(mobj_t *actor)
{
    // Using the bodyque defined in g_game.c.
    if(bodyqueslot >= BODYQUESIZE)
    { // Too many player corpses - remove an old one
        P_RemoveMobj(bodyque[bodyqueslot%BODYQUESIZE]);
    }
    bodyque[bodyqueslot%BODYQUESIZE] = actor;
    bodyqueslot++;
}

//----------------------------------------------------------------------------
//
// PROC A_FlameSnd
//
//----------------------------------------------------------------------------

void A_FlameSnd(mobj_t *actor)
{
    S_StartObjSound(actor, sfx_hedat1); // Burn sound
}
