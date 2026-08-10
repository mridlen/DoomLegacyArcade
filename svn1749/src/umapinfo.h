// -----------------------------------------------------------------------------
//
// Copyright(C) 2023 by DooM Legacy Team
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
// DESCRIPTION:
//      Support for additional map information in UMAPINFO format.
//
// -----------------------------------------------------------------------------

#ifndef UMAPINFO_H
#define UMAPINFO_H

#include "doomtype.h"

// Only set when debugging.
#define DEBUG_UMAPINFO  1

#ifdef DEBUG_UMAPINFO
extern byte  EN_umi_debug_out;  // some remote debug print
#endif

// Entry for episode menu
typedef struct emenu_s emenu_t;
struct emenu_s
{
    emenu_t    *next;

    const char * patch;  // Used only if valid for all menu entries
    const char * name;   // Episode name (used with HUD font without patch)
    const char * key;    // Keyboard key
};


// Usable only for monsters that call A_BossDeath
typedef struct bossaction_s bossaction_t;
struct bossaction_s
{
    bossaction_t * next;

    // Types as used in DoomLegacy p_spec.h, p_mobj.h
    uint16_t      thing;    // Thing type (index for table of specification)
    short         special;  // Line special type
    uint16_t      tag;      // Sector tag
};


// Condense rare flags, use less cache.
typedef enum {
// [WDJ] endgame are easier to test as general flags.  Using Tristate was complicated and tests were convoluted.
    // NEVER: both enabled & disabled
    UMA_endgame_enabled  = 0x01,   // explicit enable
    UMA_endgame_disabled = 0x02,   // explicit disable
    UMA_endgame_field = 0x3,       // unchanged = 0, (not enabled, not disabled)

    UMA_endbunny = 0x04,           // End game after level, show bunny
    UMA_endcast = 0x08,            // End game after level, show cast call
    UMA_nointermission = 0x10,     // Skip the 'level finished' screen
    UMA_bossactions_clear = 0x20,  // Clear all default boss actions
    // 0x40
    // 0x80
} mapentry_flag_e;

typedef struct mapentry_s mapentry_t;
struct mapentry_s
{
    mapentry_t   * next;

    const char   * author;
    const char   * label;              // NULL: default, Empty: clear
    const char   * levelname;
    const char   * intertext;          // NULL: default, Empty: clear
    const char   * intertextsecret;    // NULL: default, Empty: clear
    const char   * interbackdrop;
    const char   * intermusic;
    const char   * nextmap;
    const char   * nextsecret;
    const char   * music;
    const char   * skytexture;
    const char   * levelpic;
    const char   * exitpic;
    const char   * enterpic;
    const char   * endpic;
    bossaction_t * bossactions;        // Linked list
// episode and map are limited to byte elsewhere, and in parser.   
    byte           episode, map;
    byte           flags;    // from mapentry_flag_e
    unsigned int   partime;
};


typedef struct
{
    mapentry_t * map_list;     // Linked list, owned (ZMalloc)

    emenu_t    * emenu_list;   // Linked list, owned (ZMalloc)
    boolean      emenu_clear;  // Clear all default episode menu entries
} umapinfo_t;


// Root of umapinfo data.
extern umapinfo_t umapinfo;


void UMI_Load_umapinfo_lump(lumpnum_t lumpnum);


void UMI_Destroy_umapinfo(void);


boolean UMI_Parse_mapname(const char * mapname, byte * episode, byte * map);


mapentry_t * UMI_Lookup_umapinfo(byte episode, byte map);


void UMI_Load_LevelInfo(void);


const char* UMI_Get_MusicLumpName(const char* name);


#endif  // UMAPINFO_H
