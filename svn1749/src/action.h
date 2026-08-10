// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: action.h 538 2009-09-23 23:24:07Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2011 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

#ifndef ACTION_H
#define ACTION_H
// Actions for info.c and BEX pointer
// [WDJ] decl with empty param list means any params.
// Doesn't work with g++, needs actionf_p1
void A_Light0();
void A_WeaponReady();
void A_Lower();
void A_Raise();
void A_Punch();
void A_ReFire();
void A_FirePistol();
void A_Light1();
void A_FireShotgun();
void A_Light2();
void A_FireShotgun2();
void A_CheckReload();
void A_OpenShotgun2();
void A_LoadShotgun2();
void A_CloseShotgun2();
void A_FireCGun();
void A_GunFlash();
void A_FireMissile();
void A_Saw();
void A_FirePlasma();
void A_BFGsound();
void A_FireBFG();
void A_BFGSpray();
void A_Explode();
void A_Pain();
void A_PlayerScream();
void A_Fall();
void A_XScream();
void A_Look();
void A_Chase();
void A_FaceTarget();
void A_PosAttack();
void A_Scream();
void A_SPosAttack();
void A_VileChase();
void A_VileStart();
void A_VileTarget();
void A_VileAttack();
void A_StartFire();
void A_Fire();
void A_FireCrackle();
void A_Tracer();
void A_SkelWhoosh();
void A_SkelFist();
void A_SkelMissile();
void A_FatRaise();
void A_FatAttack1();
void A_FatAttack2();
void A_FatAttack3();
void A_BossDeath();
void A_CPosAttack();
void A_CPosRefire();
void A_TroopAttack();
void A_SargAttack();
void A_HeadAttack();
void A_BruisAttack();
void A_SkullAttack();
void A_Metal();
void A_SpidRefire();
void A_BabyMetal();
void A_BspiAttack();
void A_Hoof();
void A_CyberAttack();
void A_PainAttack();
void A_PainDie();
void A_KeenDie();
void A_BrainPain();
void A_BrainScream();
void A_BrainDie();
void A_BrainAwake();
void A_BrainSpit();
void A_SpawnSound();
void A_SpawnFly();
void A_BrainExplode();

// heretic stuff

void A_FreeTargMobj ();
void A_RestoreSpecialThing1 ();
void A_RestoreSpecialThing2 ();
void A_HideThing ();
void A_UnHideThing ();
void A_RestoreArtifact ();
void A_HScream ();
void A_PodPain ();
void A_RemovePod ();
void A_MakePod ();
void A_InitKeyGizmo ();
void A_VolcanoSet ();
void A_VolcanoBlast ();
void A_BeastPuff ();
void A_VolcBallImpact ();
void A_SpawnTeleGlitter ();
void A_SpawnTeleGlitter2 ();
void A_AccTeleGlitter ();
void A_Light0 ();
void A_WeaponReady ();
void A_Lower ();
void A_Raise ();
void A_StaffAttackPL1 ();
void A_ReFire ();
void A_StaffAttackPL2 ();
void A_BeakReady ();
void A_BeakRaise ();
void A_BeakAttackPL1 ();
void A_BeakAttackPL2 ();
void A_GauntletAttack ();
void A_FireBlasterPL1 ();
void A_FireBlasterPL2 ();
void A_SpawnRippers ();
void A_FireMacePL1 ();
void A_FireMacePL2 ();
void A_MacePL1Check ();
void A_MaceBallImpact ();
void A_MaceBallImpact2 ();
void A_DeathBallImpact ();
void A_FireSkullRodPL1 ();
void A_FireSkullRodPL2 ();
void A_SkullRodPL2Seek ();
void A_AddPlayerRain ();
void A_HideInCeiling ();
void A_SkullRodStorm ();
void A_RainImpact ();
void A_FireGoldWandPL1 ();
void A_FireGoldWandPL2 ();
void A_FirePhoenixPL1 ();
void A_InitPhoenixPL2 ();
void A_FirePhoenixPL2 ();
void A_ShutdownPhoenixPL2 ();
void A_PhoenixPuff ();
void A_FlameEnd ();
void A_FloatPuff ();
void A_FireCrossbowPL1 ();
void A_FireCrossbowPL2 ();
void A_BoltSpark ();
void A_NoBlocking ();
void A_AddPlayerCorpse ();
void A_SkullPop ();
void A_FlameSnd ();
void A_CheckBurnGone ();
void A_CheckSkullFloor ();
void A_CheckSkullDone ();
void A_Feathers ();
void A_ChicLook ();
void A_ChicChase ();
void A_ChicPain ();
void A_ChicAttack ();
void A_MummyAttack ();
void A_MummyAttack2 ();
void A_MummySoul ();
void A_ContMobjSound ();
void A_MummyFX1Seek ();
void A_BeastAttack ();
void A_SnakeAttack ();
void A_SnakeAttack2 ();
void A_HHeadAttack ();
void A_HBossDeath ();
void A_HeadIceImpact ();
void A_HeadFireGrow ();
void A_WhirlwindSeek ();
void A_ClinkAttack ();
void A_WizAtk1 ();
void A_WizAtk2 ();
void A_WizAtk3 ();
void A_GhostOff ();
void A_ImpMeAttack ();
void A_ImpMsAttack ();
void A_ImpMsAttack2 ();
void A_ImpDeath ();
void A_ImpXDeath1 ();
void A_ImpXDeath2 ();
void A_ImpExplode ();
void A_KnightAttack ();
void A_DripBlood ();
void A_Sor1Chase ();
void A_Sor1Pain ();
void A_Srcr1Attack ();
void A_SorZap ();
void A_SorcererRise ();
void A_SorRise ();
void A_SorSightSnd ();
void A_Srcr2Decide ();
void A_Srcr2Attack ();
void A_Sor2DthInit ();
void A_SorDSph ();
void A_Sor2DthLoop ();
void A_SorDExp ();
void A_SorDBon ();
void A_BlueSpark ();
void A_GenWizard ();
void A_MinotaurAtk1 ();
void A_MinotaurDecide ();
void A_MinotaurAtk2 ();
void A_MinotaurAtk3 ();
void A_MinotaurCharge ();
void A_MntrFloorFire ();
void A_ESound ();
void A_StartFS();		// Exl: Toxicfluff's FS actions
void A_StartWeaponFS(); //

// [WDJ] MBF functions
void A_Die_MBF();
void A_Detonate_MBF();
void A_Mushroom_MBF();
void A_Spawn_MBF();  // A_Spawn for MBF
void A_Turn_MBF();   // A_Turn for MBF
void A_Face_MBF();   // A_Face for MBF
void A_Scratch_MBF();
void A_PlaySound_MBF();
void A_RandomJump_MBF();
void A_LineEffect_MBF();
void A_KeepChasing_MBF();  // EternityEngine

// [WDJ] MBF21 functions (as in DSDA-Doom)
// These have parameters, but they are not mentioned.
void A_WeaponProjectile_MBF21();
void A_WeaponBulletAttack_MBF21();
void A_WeaponMeleeAttack_MBF21();
void A_WeaponSound_MBF21();
void A_WeaponAlert_MBF21();
void A_WeaponJump_MBF21();
void A_ConsumeAmmo_MBF21();
void A_RefireTo_MBF21();
void A_GunFlashTo_MBF21();
void A_SpawnObject_MBF21();
void A_MonsterProjectile_MBF21();
void A_MonsterBulletAttack_MBF21();
void A_MonsterMeleeAttack_MBF21();
void A_NoiseAlert_MBF21();
void A_HealChase_MBF21();
void A_SeekTracer_MBF21();
void A_ClearTracer_MBF21();
void A_FindTracer_MBF21();
void A_RadiusDamage_MBF21();
void A_CheckAmmo_MBF21();
void A_JumpIfHealthBelow_MBF21();
void A_JumpIfTargetInSight_MBF21();
void A_JumpIfTargetCloser_MBF21();
void A_JumpIfTracerInSight_MBF21();
void A_JumpIfTracerCloser_MBF21();
void A_JumpIfFlagsSet_MBF21();
void A_AddFlags_MBF21();
void A_RemoveFlags_MBF21();

// fab
void A_SmokeTrailer ();

#endif
