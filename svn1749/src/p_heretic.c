// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_heretic.c 1721 2025-01-28 15:15:00Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// $Log: p_heretic.c,v $
// Revision 1.7  2001/07/16 22:35:41  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.6  2001/05/27 13:42:47  bpereira
// Revision 1.5  2001/03/30 17:12:50  bpereira
// Revision 1.4  2001/02/24 13:35:20  bpereira
// Revision 1.3  2001/02/10 13:20:55  hurdler
// update license
//
//
//
// DESCRIPTION:
//    Heretic sound and other specifics.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
#include "dstrings.h"

//---------------------------------------------------------------------------
//
// PROC A_ContMobjSound
//
//---------------------------------------------------------------------------

void A_ContMobjSound(mobj_t *actor)
{
    switch(actor->type)
    {
     case MT_KNIGHTAXE:
        S_StartObjSound(actor, sfx_kgtatk);
        break;
     case MT_MUMMYFX1:
        S_StartObjSound(actor, sfx_mumhed);
        break;
     default:
        break;
    }
}

//----------------------------------------------------------------------------
//
// FUNC P_FaceMobj
//
// Returns 1 if 'source' needs to turn clockwise, or 0 if 'source' needs
// to turn counter clockwise.  'delta' is set to the amount 'source'
// needs to turn.
//
//----------------------------------------------------------------------------
int P_FaceMobj(mobj_t *source, mobj_t *target, angle_t *delta)
{
    angle_t angle1, angle2, diff;

    angle1 = source->angle;
    angle2 = R_PointToAngle2(source->x, source->y, target->x, target->y);
    if(angle2 > angle1)
    {
        diff = angle2 - angle1;
        if(diff > ANG180)
        {
            *delta = ANGLE_MAX - diff;
            return 0;
        }
        else
        {
            *delta = diff;
            return 1;
        }
    }
    else
    {
        diff = angle1 - angle2;
        if(diff > ANG180)
        {
            *delta = ANGLE_MAX - diff;
            return 1;
        }
        else
        {
            *delta = diff;
            return 0;
        }
    }
}

//----------------------------------------------------------------------------
//
// FUNC P_SeekerMissile
//
// The missile tracer field must be mobj_t *target.  Returns true if
// target was tracked, false if not.
//
//----------------------------------------------------------------------------

#ifdef MBF21
// MBF21
//  seek_target: Heretic actor->tracer, MBF21 allows others.
//  seek_center:  
boolean P_SeekerMissile_MBF21( mobj_t * actor, mobj_t ** seek_target, angle_t thresh, angle_t turnMax, boolean seek_center )
{
    int dir;
    int dist;
    angle_t delta;
    mobj_t * target = *seek_target;

    if(target == NULL)
    {
        return false;
    }

    if(!(target->flags&MF_SHOOTABLE))
    { // Target died
        *seek_target = 0;
        return false;
    }

    dir = P_FaceMobj(actor, target, &delta);
    if(delta > thresh)
    {
        delta >>= 1;
        if(delta > turnMax)
        {
            delta = turnMax;
        }
    }
    if(dir)
    { // Turn clockwise
        actor->angle += delta;
    }
    else
    { // Turn counter clockwise
        actor->angle -= delta;
    }
    int angf = ANGLE_TO_FINE(actor->angle);
#ifdef MONSTER_VARY
    actor->momx = FixedMul(actor->speed, finecosine[angf]);
    actor->momy = FixedMul(actor->speed, finesine[angf]);
#else
    actor->momx = FixedMul(actor->info->speed, finecosine[angf]);
    actor->momy = FixedMul(actor->info->speed, finesine[angf]);
#endif
    if( actor->z+actor->height < target->z
        || target->z+target->height < actor->z
        || seek_center )  // MBF21
    { // Need to seek vertically
        dist = P_AproxDistance(target->x-actor->x, target->y-actor->y);
#ifdef MONSTER_VARY
        dist = dist/actor->speed;
#else
        dist = dist/actor->info->speed;
#endif
        if(dist < 1)
          dist = 1;
        fixed_t height_adj = 0;
        // Heretic original: height_adj = 0;
        // DoomLegacy uses the Hexen adj for Heretic too.
        if( EN_heretic_hexen )
          height_adj = (target->height>>1) - (actor->height>>1);
        else if (seek_center)
          height_adj = target->height/2;

        actor->momz = (target->z - actor->z + height_adj) / dist;
    }
    return true;
}

boolean P_SeekerMissile(mobj_t * actor, angle_t thresh, angle_t turnMax)
{
    return P_SeekerMissile_MBF21( actor, &actor->tracer, thresh, turnMax, false );
}

#else
// Heretic

boolean P_SeekerMissile(mobj_t *actor, angle_t thresh, angle_t turnMax)
{
    int dir;
    int dist;
    angle_t delta;
    mobj_t *target;

    target = actor->tracer;
    if(target == NULL)
    {
        return false;
    }
    if(!(target->flags&MF_SHOOTABLE))
    { // Target died
        actor->tracer = 0;
        return false;
    }
    dir = P_FaceMobj(actor, target, &delta);
    if(delta > thresh)
    {
        delta >>= 1;
        if(delta > turnMax)
        {
            delta = turnMax;
        }
    }
    if(dir)
    { // Turn clockwise
        actor->angle += delta;
    }
    else
    { // Turn counter clockwise
        actor->angle -= delta;
    }
    int angf = ANGLE_TO_FINE(actor->angle);
#ifdef MONSTER_VARY
    actor->momx = FixedMul(actor->speed, finecosine[angf]);
    actor->momy = FixedMul(actor->speed, finesine[angf]);
#else
    actor->momx = FixedMul(actor->info->speed, finecosine[angf]);
    actor->momy = FixedMul(actor->info->speed, finesine[angf]);
#endif
    if( actor->z+actor->height < target->z
       || target->z+target->height < actor->z )
    { // Need to seek vertically
       dist = P_AproxDistance(target->x-actor->x, target->y-actor->y);
#ifdef MONSTER_VARY
       dist = dist/actor->speed;
#else
       dist = dist/actor->info->speed;
#endif
       if(dist < 1)
          dist = 1;
       actor->momz = (target->z+(target->height>>1)
                      -(actor->z+(actor->height>>1)))/dist;
    }
    return true;
}
#endif

//---------------------------------------------------------------------------
//
// FUNC P_SpawnMissileAngle
//
// Returns NULL if the missile exploded immediately, otherwise returns
// a mobj_t pointer to the missile.
//
//---------------------------------------------------------------------------

mobj_t *P_SpawnMissileAngle(mobj_t *source, mobjtype_t type,
        angle_t angle, fixed_t momz)
{
    fixed_t z;
    mobj_t *mo;

    switch(type)
    {
     case MT_MNTRFX1: // Minotaur swing attack missile
        z = source->z+40*FRACUNIT;
        break;
     case MT_MNTRFX2: // Minotaur floor fire missile
        z = ONFLOORZ;
        break;
     case MT_SRCRFX1: // Sorcerer Demon fireball
        z = source->z+48*FRACUNIT;
        break;
     default:
        z = source->z+32*FRACUNIT;
        break;
    }
    if(source->flags2&MF2_FEETARECLIPPED)
    {
        z -= FOOTCLIPSIZE;
    }
    mo = P_SpawnMobj(source->x, source->y, z, type);
    if(mo->info->seesound)
    {
        S_StartObjSound(mo, mo->info->seesound);
    }
    mo->target = source; // Originator
    mo->angle = angle;
    // This one spot does not pull common expressions well, explicit is smaller.
    int angf = ANGLE_TO_FINE( angle );
#ifdef MONSTER_VARY
    fixed_t speed = mo->speed;
#else
    fixed_t speed = mo->info->speed;
#endif
    mo->momx = FixedMul(speed, finecosine[angf]);
    mo->momy = FixedMul(speed, finesine[angf]);
    mo->momz = momz;
    return (P_CheckMissileSpawn(mo) ? mo : NULL);
}


extern uint16_t WeaponAmmo_pickup[];
extern uint16_t clipammo[NUMAMMO];
extern byte cheat_mus_seq[];
extern byte cheat_choppers_seq[];
extern byte cheat_god_seq[];
extern byte cheat_ammo_seq[];
extern byte cheat_ammonokey_seq[];
extern byte cheat_noclip_seq[];
extern byte cheat_commercial_noclip_seq[];
extern byte cheat_powerup_seq[7][10];
extern byte cheat_clev_seq[];
extern byte cheat_mypos_seq[];
extern byte cheat_amap_seq[];


// [WDJ] Reversible Heretic/Doom info table installation.
typedef struct {
  uint16_t  doomednum;
  uint16_t  doom_mt;
  uint16_t  heretic_mt;
} doomednum_install_t;

// Order is not important.  Only needs to include those doomednum that conflict.
static const doomednum_install_t  doomednum_install_table[] = {
  {  7, MT_SPIDER, MT_SORCERER1 },  // SORCERER  (monster)
  {  9, MT_SHOTGUY, MT_MINOTAUR },    // MINOTAUR  (monster)
  { 16, MT_CYBORG, MT_AMMACEHEFTY },  // MACE BALL STACK (ammo M mace)
  { 64, MT_VILE, MT_HKNIGHT },       // KNIGHT  (monster undead)
  { 65, MT_CHAINGUY, MT_KNIGHTGHOST },  // KNIGHT GHOST  (monster undead)
  { 66, MT_UNDEAD, MT_IMP },         // IMP (monster flying)
  { 68, MT_BABY, MT_MUMMY },  // MUMMY (monster)
  { 69, MT_KNIGHT, MT_MUMMYGHOST },  // MUMMY GHOST  (monster)
  { 82, MT_SUPERSHOTGUN, MT_HMISC3 },  // QUARTZ FLASK  (health)
  { 83, MT_MEGA, MT_ARTIFLY },  // WINGS  (artifact)
  { 84, MT_WOLFSS, MT_ARTIINVULNERABILITY },  // INVULNERABLE  (artifact invul)
  { 87, MT_BOSSTARGET, MT_HMISC12 },  // VOLCANO SPEWER
  { 2001, MT_SHOTGUN, MT_HMISC15 },  // CROSSBOW  (weapon)
  { 2002, MT_CHAINGUN, MT_WMACE },  // MACE  (weapon)
  { 2003, MT_ROCKETLAUNCH, MT_WPHOENIXROD },  // PHOENIX ROD  (weapon)
  { 2004, MT_PLASMAGUN, MT_WSKULLROD },  // SKULL ROD, HELLSTAFF  (weapon)
  { 2005, MT_SHAINSAW, MT_HMISC13 },  // GAUNTLETS  (weapon)
  { 2035, MT_BARREL, MT_POD },  //  POD  (exploding)
  {  5, MT_MISC4, MT_IMPLEADER },  // IMP LEADER (monster flying)
  { 13, MT_MISC5, MT_AMMACEWIMPY },  // MACE BALL  (ammo S mace)
  {  6, MT_MISC6, MT_HHEAD },  // HEAD  (monster)
  { 39, MT_MISC7, MT_STALACTITESMALL },  // STALACTITE SMALL
  { 38, MT_MISC8, MT_STALAGMITELARGE },  // STALAGMITE LARGE
  { 40, MT_MISC9, MT_STALACTITELARGE },  // STALACTITE LARGE
  { 17, MT_MISC21, MT_SKULLHANG70 },  // HANGING SKULL 70
  {  8, MT_MISC24, MT_HMISC1 },  // BAG OF HOLDING (knapsack)
  { 85, MT_MISC29, MT_ITEMSHIELD1 },  // SILVER SHIELD  (armor)
  { 86, MT_MISC30, MT_ARTITOMEOFPOWER },  // TOME  (artifact)
  { 30, MT_MISC32, MT_ARTIEGG },  // EGG  (artifact)
  { 31, MT_MISC33, MT_ITEMSHIELD2 },  // SHIELD MEGA  (armor)
  { 32, MT_MISC34, MT_ARTISUPERHEAL },  // SUPER HEAL  (artifact health)
  { 33, MT_MISC35, MT_HMISC4 },  // TORCH  (artifact torch)
  { 37, MT_MISC36, MT_STALAGMITESMALL },  // STALAGMITE SMALL
  { 36, MT_MISC37, MT_ARTITELEPORT },  //  (artifact teleport)
  { 41, MT_MISC38, MT_SOUNDWATERFALL },  // SOUND WATERFALL  (ambience sound)
  { 42, MT_MISC39, MT_SOUNDWIND },  // SOUND WIND  (ambience sound)
  { 43, MT_MISC40, MT_PODGENERATOR },  // POD GENERATOR
  { 44, MT_MISC41, MT_HBARREL },  // BARREL
  { 45, MT_MISC42, MT_MUMMYLEADER },  // MUMMY LEADER (monster)
  { 46, MT_MISC43, MT_MUMMYLEADERGHOST },  // MUMMY LEADER GHOST  (monster)
  { 55, MT_MISC44, MT_AMBLSRHEFTY },  // ENERGY ORB LARGE  (ammo M claw)
  { 56, MT_MISC45, 0xFFFF  },  // heretic use it for monster spawn
  { 47, MT_MISC47, MT_HMISC7 },  // PILLAR BROWN
  { 48, MT_MISC48, MT_HMISC8 },  // MOSS 1
  { 34, MT_MISC49, MT_HMISC5 },  // BOMB  (exploding timed)
  { 35, MT_MISC50, MT_HMISC2 },  // MAP   (special)
  { 49, MT_MISC51, MT_HMISC9 },  // MOSS 2
  { 50, MT_MISC52, MT_HMISC10 },  // WALL TORCH  (torch)
  { 51, MT_MISC53, MT_HMISC11 },  // HANGING CORPSE
  { 52, MT_MISC54, MT_TELEGLITGEN2 },  // TELEPORT GLITTER GENERATOR 2
  { 53, MT_MISC55, MT_HMISC14 },  // DRAGON CLAW  (weapon)
  { 22, MT_MISC61, MT_AMPHRDWIMPY },  // FLAME ORB  (ammo S phoenix rod)
  { 15, MT_MISC62, MT_WIZARD },  // WIZARD (monster floating)
  { 18, MT_MISC63, MT_AMCBOWWIMPY },  // ETHEREAL ARROWS (ammo S crossbow)
  { 21, MT_MISC64, MT_AMSKRDHEFTY },  // GREATER RUNES  (ammo M skull rod)
  { 23, MT_MISC65, MT_AMPHRDHEFTY },  // FLAME ORB LARGE  (ammo M phoenix rod)
  { 20, MT_MISC66, MT_AMSKRDWIMPY },  // LESSER RUNES  (ammo S skull rod)
  { 19, MT_MISC67, MT_AMCBOWHEFTY },  // ETHEREAL ARROWS QUIVER  (ammo M crossbow)
  { 10, MT_MISC68, MT_AMGWNDWIMPY },  // WAND CRYSTAL (ammo S staff)
  { 12, MT_MISC69, MT_AMGWNDHEFTY },  // WAND CRYSTAL LARGE (ammo M staff)
  { 28, MT_MISC70, MT_CHANDELIER },  // CHANDELIER
  { 24, MT_MISC71, MT_SKULLHANG60 },  // HANGING SKULL 60
  { 27, MT_MISC72, MT_SERPTORCH },  // SERPENT TORCH (torch)
  { 29, MT_MISC73, MT_SMALLPILLAR },  // PILLAR SMALL
  { 25, MT_MISC74, MT_SKULLHANG45 },  // HANGING SKULL 45
  { 26, MT_MISC75, MT_SKULLHANG35 },  // HANGING SKULL 35
  { 54, MT_MISC76, MT_AMBLSRWIMPY },  //  CLAW ORB  (ammo S claw)
  { 70, MT_MISC77, MT_BEAST },  // BEAST (monster)
  { 73, MT_MISC78, MT_AKYY },  // GREEN KEY  (key)
  { 74, MT_MISC79, MT_TELEGLITGEN },  // TELEPORT GLITTER GENERATOR 1
  { 75, MT_MISC80, MT_ARTIINVISIBILITY },  // INVISIBLE  (artifact invis)
  { 76, MT_MISC81, MT_HMISC6 },  // FIRE BRAZIER (torch)
  { 79, MT_MISC84, MT_BKYY },  // BLUE KEY  (key)
  { 80, MT_MISC85, MT_CKEY },  // YELLOW KEY  (key)
  { 81, MT_MISC86, MT_HMISC0 },  // CRYSTAL VIAL  (health)
  { 0xFFFF, 0xFFFF, 0xFFFF },  // END
};

#if 0
void  doomednum_check( byte heretic_check )
{
    doomednum_install_t * ii;
    for( ii = & doomednum_install_table[0]; ii->doomednum < 0xFFFF; ii++ )
    {
        uint16_t mt = ( heretic_check )?  ii->heretic_mt : ii->doom_mt;
        if( mt < NUMMOBJTYPES )
          if( mobjinfo[ mt ].doomednum != ii->doomednum )
            printf( "MOBJINFO[%i].doomednum=%i,  TABLE doomednum=%i, doom_mt=%i, heretic_mt=%i\n", mt, mobjinfo[mt].doomednum, ii->doomednum, ii->doom_mt, ii->heretic_mt );
    }
}
#endif

void  doomednum_install( byte set_heretic )
{
    const doomednum_install_t * ii;
    uint16_t  cancel_mt, set_mt;
    for( ii = & doomednum_install_table[0]; ii->doomednum < 0xFFFF; ii++ )
    {
        cancel_mt = ( set_heretic )?  ii->doom_mt : ii->heretic_mt;
        set_mt = ( set_heretic )?  ii->heretic_mt : ii->doom_mt;
        if( cancel_mt < NUMMOBJTYPES )
        {
            if( mobjinfo[ cancel_mt ].doomednum == ii->doomednum )
                mobjinfo[ cancel_mt ].doomednum = -1;
        }
        if( set_mt < NUMMOBJTYPES )
            mobjinfo[ set_mt ].doomednum = ii->doomednum;
    }
}


// [WDJ] Reversible Heretic/Doom Ammo table installation.
typedef struct {
  uint16_t  max_val;
  byte      clip_val;
} ammo_install_t;

// Index by ammotype_t
static const ammo_install_t  heretic_ammo_table[NUMAMMO] = {
   { 100,  5 }, // am_goldwand, clip used in deathmatch 1 & 3 mul by 5 (P_GiveWeapon)
   {  50,  2 }, // am_crossbow
   { 200,  6 }, // am_blaster
   { 200, 10 }, // am_skullrod
   {  20,  1 }, // am_phoenixrod
   { 150, 10 }, // am_mace
};

// Index by ammotype_t
static const ammo_install_t  doom_ammo_table[NUMAMMO] = {
   { 200, 10 }, // am_clip
   {  50,  4 }, // am_shell
   { 300, 20 }, // am_cell
   {  50,  1 }, // am_misl
   {   0,  0 },
   {   0,  0 },
};

// Index by weapontype_t
static const byte  heretic_weapon_ammo_table[NUMWEAPONS] = {
   0, // wp_staff
  25, // wp_goldwand
  10, // wp_crossbow
  30, // wp_blaster
  50, // wp_skullrod
   2, // wp_phoenixrod
  50, // wp_mace
   0, // wp_gauntlets
   0, // wp_beak
};

// Index by weapontype_t
static const byte  doom_weapon_ammo_table[NUMWEAPONS] = {
   0, // wp_fist
  20, // wp_pistol
   8, // wp_shotgun
  20, // wp_chaingun
   2, // wp_missle
  40, // wp_plasma
  40, // wp_bfg
   0, // wp_chainsaw
   8, // wp_supershotgun
};


// [WDJ] Reversible Heretic/Doom Sfx table installation.
typedef struct {
    sfxid_t  sfxid;  // sfxenum_e
    char *   name;
    byte     priority;
} sfx_install_t;

static const sfx_install_t  heretic_sfx_table[] = {
  { sfx_oof, "plroof", 32 },
  { sfx_swtchn, "switch", 40 },
  { sfx_swtchx, "switch", 40 },
  { sfx_telept, "telept", 50 },
  { sfx_sawup, "gntact", 32 },  // gauntlets
  { sfx_pistol, "keyup", 64 },  // for the menu
  { sfx_tink, "chat", 100 },
  { sfx_itmbk, "respawn", 10 },
  { 0xFFFF, NULL, 0 }
};

// To undo heretic install.
static const sfx_install_t  doom_sfx_table[] = {
  { sfx_oof, "oof\0\0\0", 96 },
  { sfx_swtchn, "swtchn", 78 },
  { sfx_swtchx, "swtchx", 78 },
  { sfx_telept, "telept", 32 },
  { sfx_sawup, "sawup\0", 64 },  // chainsaw
  { sfx_pistol, "pistol", 64 },  // pistol
  { sfx_tink, "tink\0\0", 60 },
  { sfx_itmbk, "itmbk\0", 100 },
  { 0xFFFF, NULL, 0 }
};


// [WDJ] Reversible Heretic/Doom text table installation.
typedef struct {
    uint16_t  textid;
    char *    text;
} text_install_t;

static const text_install_t  heretic_text_table[] = {
  { PD_BLUEK_NUM,   "YOU NEED A BLUE KEY TO OPEN THIS DOOR" },
  { PD_YELLOWK_NUM, "YOU NEED A YELLOW KEY TO OPEN THIS DOOR" },
  { PD_REDK_NUM,    "YOU NEED A GREEN KEY TO OPEN THIS DOOR" },
  { GOTBLUECARD_NUM, "BLUE KEY" },
  { GOTYELWCARD_NUM, "YELLOW KEY" },
  { GOTREDCARD_NUM,  "GREEN KEY" },
  { GOTARMOR_NUM, "SILVER SHIELD" },
  { GOTMEGA_NUM,  "ENCHANTED SHIELD" },
  { GOTSTIM_NUM,  "CRYSTAL VIAL" },
  { GOTMAP_NUM,  "MAP SCROLL" },
  { 0xFFFF, NULL }
};

static const text_install_t  doom_text_table[] = {
  { PD_BLUEK_NUM,   "You need a blue key to open this door" },
  { PD_YELLOWK_NUM, "You need a yellow key to open this door" },
  { PD_REDK_NUM,    "You need a red key to open this door" },
  { GOTBLUECARD_NUM, "Picked up a blue keycard." },
  { GOTYELWCARD_NUM, "Picked up a yellow keycard." },
  { GOTREDCARD_NUM,  "Picked up a red keycard." },
  { GOTARMOR_NUM, "Picked up the armor." },
  { GOTMEGA_NUM,  "Picked up the MegaArmor!" },
  { GOTSTIM_NUM,  "Picked up a stimpack." },
  { GOTMAP_NUM,  "Computer Area Map" },
  { 0xFFFF, NULL }
};

void  sfx_ammo_text_install( byte set_heretic )
{
    const sfx_install_t * st = doom_sfx_table;
    const ammo_install_t * aat = doom_ammo_table;
    const byte * wat = doom_weapon_ammo_table;
    const text_install_t * tt = doom_text_table;
    int ai;

    // SFX
    if( set_heretic )
    {
        ceilmovesound = sfx_dormov;
        doorclosesound = sfx_doropn;

        st = heretic_sfx_table;
        aat = heretic_ammo_table;
        wat = heretic_weapon_ammo_table;
        tt = heretic_text_table;
    }
    else
    {
        ceilmovesound = sfx_stnmov;
        doorclosesound = sfx_dorcls;
    }

    // SFX
    for( ; st->sfxid < NUMSFX_DEF; st++ )
    {
        S_sfx[st->sfxid].priority = st->priority;
        if( st->name )
            S_sfx[st->sfxid].name = st->name;
    }
   

    // AMMO
    for( ai = 0; ai < NUMAMMO; ai++ )  // index ammotype_t
    {
        maxammo[ ai ]  = aat[ai].max_val;
        clipammo[ ai ] = aat[ai].clip_val;
    }

    for( ai = 0; ai < NUMWEAPONS; ai++ )  // index weapontype_t
    {
        WeaponAmmo_pickup[ai] = wat[ai];
    }

    // TEXT
    for( ; tt->textid < NUMTEXT ; tt++ )
    {
        text[tt->textid] = tt->text;
    }
}



void Heretic_PatchEngine(void)
{
    // we can put such thinks in a dehacked lump, maybe for later
    mobjinfo[MT_TFOG].spawnstate = S_HTFOG1;
    sprnames[SPR_BLUD] = "BLOD";

    S_music[mus_inter].name = "MUS_INTR";

    sfx_ammo_text_install( 1 ); // install heretic sfx, ammo
   
    // conflicting number for doomednum
    // so disable doom mobjs and enable heretic's one
    doomednum_install( 1 );  // install heretic doomednum
}

mobj_t LavaInflictor;

//----------------------------------------------------------------------------
//
// PROC P_Init_Lava
//
//----------------------------------------------------------------------------

void P_Init_Lava(void)
{
    memset(&LavaInflictor, 0, sizeof(mobj_t));
    LavaInflictor.type = MT_PHOENIXFX2;
    LavaInflictor.flags2 = MF2_FIREDAMAGE|MF2_NODMGTHRUST;
}

//----------------------------------------------------------------------------
//
// PROC P_HerePlayerInSpecialSector
//
// Called every tic frame that the player origin is in a special sector.
//
//----------------------------------------------------------------------------

void P_Heretic_PlayerInSpecialSector( player_t * player, sector_t * sector )
{
    static int pushTab[5] = {
        2048*5,
        2048*10,
        2048*25,
        2048*30,
        2048*35
    };
    
    // Player is not touching the floor
    if( player->mo->z != sector->floorheight )
        return;
    
    switch(sector->special)
    {
    case 7: // Damage_Sludge
        if(!(leveltime&31))
        {
            P_DamageMobj(player->mo, NULL, NULL, 4);
        }
        break;
    case 5: // Damage_LavaWimpy
        if(!(leveltime&15))
        {
            P_DamageMobj(player->mo, &LavaInflictor, NULL, 5);
            P_HitFloor(player->mo);
        }
        break;
    case 16: // Damage_LavaHefty
        if(!(leveltime&15))
        {
            P_DamageMobj(player->mo, &LavaInflictor, NULL, 8);
            P_HitFloor(player->mo);
        }
        break;
    case 4: // Scroll_EastLavaDamage
        P_Thrust(player, 0, 2048*28);
        if(!(leveltime&15))
        {
            P_DamageMobj(player->mo, &LavaInflictor, NULL, 5);
            P_HitFloor(player->mo);
        }
        break;
    case 9: // SecretArea
        player->secretcount++;
        sector->special = 0;
        break;
    case 11: // Exit_SuperDamage (DOOM E1M8 finale)
             /*
             player->cheats &= ~CF_GODMODE;
             if(!(leveltime&0x1f))
             {
             P_DamageMobj(player->mo, NULL, NULL, 20);
             }
             if(player->health <= 10)
             {
             G_ExitLevel();
             }
        */
        break;
        
    case 25: case 26: case 27: case 28: case 29: // Scroll_North
        P_Thrust(player, ANG90, pushTab[sector->special-25]);
        break;
    case 20: case 21: case 22: case 23: case 24: // Scroll_East
        P_Thrust(player, 0, pushTab[sector->special-20]);
        break;
    case 30: case 31: case 32: case 33: case 34: // Scroll_South
        P_Thrust(player, ANG270, pushTab[sector->special-30]);
        break;
    case 35: case 36: case 37: case 38: case 39: // Scroll_West
        P_Thrust(player, ANG180, pushTab[sector->special-35]);
        break;
        
    case 40: case 41: case 42: case 43: case 44: case 45:
    case 46: case 47: case 48: case 49: case 50: case 51:
        // Wind specials are handled in (P_mobj):P_XYMovement
        break;
        
    case 15: // Friction_Low
        // Only used in (P_mobj):P_XYMovement and (P_user):P_Thrust
        break;
        
    default:
        I_SoftError( "P_PlayerInSpecialSector: "
                    "unknown special %i\n", sector->special);
    }
}

//---------------------------------------------------------------------------
//
// FUNC P_GetThingFloorType
//
//---------------------------------------------------------------------------
int P_GetThingFloorType(mobj_t *thing)
{
    return thing->subsector->sector->floortype;
}
