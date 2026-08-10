// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_items.c 1673 2024-03-03 04:40:47Z wesleyjohnson $
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
// $Log: d_items.c,v $
// Revision 1.3  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION: 
//      holds the weapon info for now...
//
//-----------------------------------------------------------------------------


// We are referring to sprite numbers.
#include "info.h"
#include "d_items.h"


//
// PSPRITE ACTIONS for weapons.
// This struct controls the weapon animations.
//
// Each entry is:
//  ammo/amunition type
//  upstate
//  downstate
//  readystate
//  atkstate, i.e. attack/fire/hit frame
//  flashstate, muzzle flash
//
weaponinfo_t doomweaponinfo[NUMWEAPONS] =
{
    {
        // fist
        am_noammo,
        0,
        S_PUNCHUP,
        S_PUNCHDOWN,
        S_PUNCH,
        S_PUNCH1,
        S_PUNCH1,
        S_NULL,
#ifdef MBF21
        WPF_FLEE_MELEE | WPF_AUTOSWITCH_FROM | WPF_NO_AUTOSWITCH_TO
#endif
    },
    {
        // pistol
        am_clip,
        1,
        S_PISTOLUP,
        S_PISTOLDOWN,
        S_PISTOL,
        S_PISTOL1,
        S_PISTOL1,
        S_PISTOLFLASH,
#ifdef MBF21
        WPF_AUTOSWITCH_FROM
#endif
    },
    {
        // shotgun
        am_shell,
        1,
        S_SGUNUP,
        S_SGUNDOWN,
        S_SGUN,
        S_SGUN1,
        S_SGUN1,
        S_SGUNFLASH1,
#ifdef MBF21
        0
#endif
    },
    {
        // chaingun
        am_clip,
        1,
        S_CHAINUP,
        S_CHAINDOWN,
        S_CHAIN,
        S_CHAIN1,
        S_CHAIN1,
        S_CHAINFLASH1,
#ifdef MBF21
        0
#endif
    },
    {
        // missile launcher
        am_misl,
        1,
        S_MISSILEUP,
        S_MISSILEDOWN,
        S_MISSILE,
        S_MISSILE1,
        S_MISSILE1,
        S_MISSILEFLASH1,
#ifdef MBF21
        WPF_NO_AUTOFIRE
#endif
    },
    {
        // plasma rifle
        am_cell,
        1,
        S_PLASMAUP,
        S_PLASMADOWN,
        S_PLASMA,
        S_PLASMA1,
        S_PLASMA1,
        S_PLASMAFLASH1,
#ifdef MBF21
        0
#endif
    },
    {
        // bfg 9000
        am_cell,
        40,
        S_BFGUP,
        S_BFGDOWN,
        S_BFG,
        S_BFG1,
        S_BFG1,
        S_BFGFLASH1,
#ifdef MBF21
        WPF_NO_AUTOFIRE
#endif
    },
    {
        // chainsaw
        am_noammo,
        0,
        S_SAWUP,
        S_SAWDOWN,
        S_SAW,
        S_SAW1,
        S_SAW1,
        S_NULL,
#ifdef MBF21
        WPF_NO_THRUST | WPF_FLEE_MELEE | WPF_NO_AUTOSWITCH_TO
#endif
    },
    {
        // super shotgun
        am_shell,
        2,
        S_DSGUNUP,
        S_DSGUNDOWN,
        S_DSGUN,
        S_DSGUN1,
        S_DSGUN1,
        S_DSGUNFLASH1,
#ifdef MBF21
        0
#endif
    },
};
