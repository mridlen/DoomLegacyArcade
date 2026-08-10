// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_player.h 1699 2024-11-27 07:20:27Z wesleyjohnson $
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
// $Log: d_player.h,v $
// Revision 1.7  2004/07/27 08:19:34  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.6  2003/03/22 22:35:59  hurdler
//
// Revision 1.5  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.4  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.3  2000/08/31 14:30:55  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      player data structures
//
//-----------------------------------------------------------------------------

#ifndef D_PLAYER_H
#define D_PLAYER_H

#include "doomdef.h"
#include "doomtype.h"

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "d_items.h"
#include "p_pspr.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

#include "b_bot.h"	//added by AC for acbot

#ifdef ENABLE_UMAPINFO
// [MB] 2023-01-22: Support for UMAPINFO added
#include "umapinfo.h"
#endif



//
// Player states.
//
typedef enum
{
    // Playing or camping.
    PST_LIVE,
    // Dead on the ground, view follows killer.
    PST_DEAD,
    // Ready to restart/respawn???
    PST_REBORN

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
    // No clipping, walk through barriers.
    CF_NOCLIP           = 0x01,
    // No damage, no health loss.
    CF_GODMODE          = 0x02,
    // Not really a cheat, just a debug aid.
    CF_NOMOMENTUM       = 0x04,

    //added:28-02-98: new cheats
    CF_FLYAROUND        = 0x08,

    //added:28-02-98: NOT REALLY A CHEAT
    // Allow player avatar to walk in-air
    //  if trying to get over a small wall (hack for playability)
    CF_JUMPOVER         = 0x10

} cheat_e;

// [WDJ] Convert boolean to bit flags.
// For savegame Ver 1.49.
typedef enum
{
    GF_backpack         = 0x01,  // have the backpack
    GF_original_weapon  = 0x02,
    GF_autoaim          = 0x04,  // autoaim state
    GF_didsecret        = 0x08,
} player_GF_flags_e;

typedef enum
{
    GB_attackdown       = 0x01,
    GB_usedown          = 0x02,
    GB_jumpdown         = 0x04,
    GB_jumpdown2        = 0x08,  // spirit, (was 2 value in a boolean, corrupted by savegame)
    GB_locked           = 0x80,  // player cannot use game controls (moving, shooting, aiming)
} player_GB_flags_e;

// ========================================================================
//                          PLAYER STRUCTURE
// ========================================================================
typedef struct player_s
{
    mobj_t*             mo;
    // added 1-6-98: for movement prediction
#ifdef CLIENTPREDICTION2
    mobj_t*             spirit;
#endif
    playerstate_t       playerstate;
    ticcmd_t            cmd;

    // Determine POV,
    //  including viewpoint bobbing during movement.
    // Focal origin above r.z
    fixed_t             viewz;
    // Base height above floor for viewz.
    fixed_t             viewheight;
    // Bob/squat speed.
    fixed_t             deltaviewheight;
    // bob effect
    fixed_t             bob;
    // bob momentum, adjusted for friction
    fixed_t		bob_momx, bob_momy;

    //added:16-02-98: mouse aiming, where the guy is looking at!
    //                 It is updated with cmd->aiming.
    angle_t             aiming;

    // This is only used between levels,
    // mo->health is used during levels.
    int                 health;
    int                 armorpoints;
    // Armor type is 0-2.
    byte                armortype;

    // Power ups. invinc and invis are tic counters.
    int                 powers[NUMPOWERS];
    byte                cards; // bit field see declaration of card_t
//    boolean             backpack;
    byte                GB_flags; // attackdown, etc.
    uint16_t            GF_flags; // backpack, and other booleans

    // Frags, kills of other players.
    uint16_t            addfrags;   // player have killed a player but is gone
    uint16_t            frags[MAXPLAYERS];
    weapontype_t        readyweapon;

    // Is wp_nochange if not changing.
    weapontype_t        pendingweapon;

    boolean             weaponowned[NUMWEAPONS];
    // These would be uint16_t, but have to be int because of the status bar pointing to them.
    int                 ammo[NUMAMMO];
    int                 maxammo[NUMAMMO];
    // added by Boris : preferred weapons order stuff
    char                favoritweapon[NUMWEAPONS];
//    boolean             originalweaponswitch;
    //added:28-02-98:
//    boolean             autoaim_toggle;

    // True if button down last tic.
//    boolean             attackdown;
//    boolean             usedown;
//    boolean             jumpdown;   //added:19-03-98:dont jump like a monkey!

    // Refired shots are less accurate.
    int                 refire;

     // For intermission stats.
    int                 killcount;
    int                 itemcount;
    int                 secretcount;

    // Hint messages.
    char*               message;
    byte                msglevel;  // priority of the message

    // Bit flags, for cheats and debug.
    // See cheat_e, above.
    uint16_t            cheats;  // cheat_e

    // For screen flashing (red or bright).
    uint16_t            damagecount;
    uint16_t            bonuscount;
    byte                key_pickup, health_pickup, armor_pickup,
                        ammo_pickup, weapon_pickup;

    // Who did damage (NULL for floors/ceilings).
    mobj_t*             attacker;
    int                 specialsector;      //lava/slime/water...

    // So gun flashes light up areas.
    lightlev_t          extralight;

    // Normally 0, which allows normal colormap.
    // pain=>REDCOLORMAP, invulnerability=>INVERSECOLORMAP, goggles=>1
    int                 fixedcolormap;

    // Player skin colorshift,
    //  0-3 for which color to draw player.
    // adding 6-2-98 comment : unused by doom2 1.9 now is used
    int                 skincolor;

    // added 2/8/98
    int                 skin;

    // Overlay view sprites (gun, etc).
    pspdef_t            psprites[NUMPSPRITES];

    // True if secret level has been done.
//    boolean             didsecret;

    // heretic
    int             chickenTics;      // player is a chicken if > 0
    int             chickenPeck;      // chicken peck countdown
    mobj_t *        rain1;         // active rain maker 1
    mobj_t *        rain2;         // active rain maker 2
    int             flamecount;
    int             flyheight;
    inventory_t     inventory[NUMINVENTORYSLOTS];
    int             inventorySlotNum;

    int             inv_ptr;
    int             st_curpos;        // position of inventory scroll
    int             st_inventoryTics; // when >0 show inventory in status bar

    weaponinfo_t *  weaponinfo; // can be changed when use level2 weapons (heretic)

    bot_t*          bot;	//added by AC for acbot

    // True if the player cannot use game controls (moving, shooting, aiming)
//    boolean locked;
#ifdef ENABLE_TIRED_RUN
    uint16_t  tired;  // tired when running
    uint16_t  drown;  // need air
#endif
} player_t;


//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct
{
    boolean     in;     // whether the player is in game

    // Player stats, kills, collected items etc.
    int         skills;
    int         sitems;
    int         ssecret;
    int         stime;
    uint16_t    frags[MAXPLAYERS]; // added 17-1-98 more than 4 players
    int         score;  // current score on entry, modified on return
    // BP: unused for now but don't forget....
    uint16_t    addfrags;
} wb_player_t;

typedef struct
{
    // [WDJ]  epsd and lev are 0 based indexes, inherited from the original.
    // They are problematic, getting +1 and -1 everywhere in the code.
    // To protect their use as array indexes, they should not be signed.
    // Doom2 will have episode=0, which will generate a bad epsd irregardless,
    // but that will be ignored, as it does not get used for indexing.
    uint16_t    epsd;   // episode # (0-2)
#ifdef ENABLE_UMAPINFO
    // UMAPINFO may cross into another episode.
    uint16_t    epsd_next;
#endif

    // previous and next levels, origin 0
    uint16_t    lev_prev;
    uint16_t    lev_next;

    // if true, splash the secret level
    boolean     didsecret;

    int         maxkills;
    int         maxitems;
    int         maxsecret;
    int         maxfrags;

    // the par time
    int         partime;

    // index of this player in game
    int         pnum;

#ifdef ENABLE_UMAPINFO
    // [MB] 2023-01-22: Support for UMAPINFO added
    mapentry_t * umapinfo_done;  // of map just finished
    mapentry_t * umapinfo_next;  // of next map to be played
#endif

    wb_player_t    plyr[MAXPLAYERS];
} wb_start_t;

void A_TicWeapon( player_t* player,  pspdef_t* psp );

#endif
