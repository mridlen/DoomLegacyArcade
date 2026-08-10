// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: sounds.c 1745 2025-04-10 09:36:34Z wesleyjohnson $
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
// $Log: sounds.c,v $
// Revision 1.10  2001/03/30 17:12:51  bpereira
//
// Revision 1.9  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.8  2001/02/24 13:35:21  bpereira
//
// Revision 1.7  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.6  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.5  2000/11/03 11:48:40  hurdler
// Fix compiling problem under win32 with 3D-Floors and FragglScript (to verify!)
//
// Revision 1.4  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.3  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      music/sound tables, and related sound routines
//
// Note: the tables were originally created by a sound utility at Id,
//       kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------


#include "sounds.h"
#include "doomincl.h"
#include "doomstat.h"
#include "p_local.h"
  // BDTC_xxx
#include "s_sound.h"
#include "r_defs.h"
#include "r_things.h"
#include "z_zone.h"
#include "w_wad.h"

// NOTE: add \0 for stringlen=6, to allow dehacked patching

//
// Information about all the music
//

musicinfo_t S_music[NUMMUSIC] =
{
    { 0 },
    { "e1m1\0\0", 0 },
    { "e1m2\0\0", 0 },
    { "e1m3\0\0", 0 },
    { "e1m4\0\0", 0 },
    { "e1m5\0\0", 0 },
    { "e1m6\0\0", 0 },
    { "e1m7\0\0", 0 },
    { "e1m8\0\0", 0 },
    { "e1m9\0\0", 0 },
    { "e2m1\0\0", 0 },
    { "e2m2\0\0", 0 },
    { "e2m3\0\0", 0 },
    { "e2m4\0\0", 0 },
    { "e2m5\0\0", 0 },
    { "e2m6\0\0", 0 },
    { "e2m7\0\0", 0 },
    { "e2m8\0\0", 0 },
    { "e2m9\0\0", 0 },
    { "e3m1\0\0", 0 },
    { "e3m2\0\0", 0 },
    { "e3m3\0\0", 0 },
    { "e3m4\0\0", 0 },
    { "e3m5\0\0", 0 },
    { "e3m6\0\0", 0 },
    { "e3m7\0\0", 0 },
    { "e3m8\0\0", 0 },
    { "e3m9\0\0", 0 },
    { "inter\0" , 0 },
    { "intro\0" , 0 },
    { "bunny\0" , 0 },
    { "victor"  , 0 },
    { "introa"  , 0 },
    { "runnin"  , 0 },
    { "stalks"  , 0 },
    { "countd"  , 0 },
    { "betwee"  , 0 },
    { "doom\0\0", 0 },
    { "the_da"  , 0 },
    { "shawn\0" , 0 },
    { "ddtblu"  , 0 },
    { "in_cit"  , 0 },
    { "dead\0\0", 0 },
    { "stlks2"  , 0 },
    { "theda2"  , 0 },
    { "doom2\0" , 0 },
    { "ddtbl2"  , 0 },
    { "runni2"  , 0 },
    { "dead2\0" , 0 },
    { "stlks3"  , 0 },
    { "romero"  , 0 },
    { "shawn2"  , 0 },
    { "messag"  , 0 },
    { "count2"  , 0 },
    { "ddtbl3"  , 0 },
    { "ampie\0" , 0 },
    { "theda3"  , 0 },
    { "adrian"  , 0 },
    { "messg2"  , 0 },
    { "romer2"  , 0 },
    { "tense\0" , 0 },
    { "shawn3"  , 0 },
    { "openin"  , 0 },
    { "evil\0\0", 0 },
    { "ultima"  , 0 },
    { "read_m"  , 0 },
    { "dm2ttl"  , 0 },
    { "dm2int"  , 0 },
// heretic stuff
        { "MUS_E1M1", 0 }, // 1-1
        { "MUS_E1M2", 0 },
        { "MUS_E1M3", 0 },
        { "MUS_E1M4", 0 },
        { "MUS_E1M5", 0 },
        { "MUS_E1M6", 0 },
        { "MUS_E1M7", 0 },
        { "MUS_E1M8", 0 },
        { "MUS_E1M9", 0 },

        { "MUS_E2M1", 0 }, // 2-1
        { "MUS_E2M2", 0 },
        { "MUS_E2M3", 0 },
        { "MUS_E2M4", 0 },
        { "MUS_E1M4", 0 },
        { "MUS_E2M6", 0 },
        { "MUS_E2M7", 0 },
        { "MUS_E2M8", 0 },
        { "MUS_E2M9", 0 },

        { "MUS_E1M1", 0 }, // 3-1
        { "MUS_E3M2", 0 },
        { "MUS_E3M3", 0 },
        { "MUS_E1M6", 0 },
        { "MUS_E1M3", 0 },
        { "MUS_E1M2", 0 },
        { "MUS_E1M5", 0 },
        { "MUS_E1M9", 0 },
        { "MUS_E2M6", 0 },

        { "MUS_E1M6", 0 }, // 4-1
        { "MUS_E1M2", 0 },
        { "MUS_E1M3", 0 },
        { "MUS_E1M4", 0 },
        { "MUS_E1M5", 0 },
        { "MUS_E1M1", 0 },
        { "MUS_E1M7", 0 },
        { "MUS_E1M8", 0 },
        { "MUS_E1M9", 0 },

        { "MUS_E2M1", 0 }, // 5-1
        { "MUS_E2M2", 0 },
        { "MUS_E2M3", 0 },
        { "MUS_E2M4", 0 },
        { "MUS_E1M4", 0 },
        { "MUS_E2M6", 0 },
        { "MUS_E2M7", 0 },
        { "MUS_E2M8", 0 },
        { "MUS_E2M9", 0 },

        { "MUS_E3M2", 0 }, // 6-1
        { "MUS_E3M3", 0 }, // 6-2
        { "MUS_E1M6", 0 }, // 6-3

        { "MUS_TITL", 0 },
        { "MUS_CPTD", 0 }
};


//
// Information about all the sfx
//

// Same as 0
#define NOL    sfx_None

sfxinfo_t S_sfx[ NUMSFX_EXT ] =
{
  // Volume is diff (+-), so should be 0, but no entry uses it
  // BEX can set name, priority, and flags (SFX_single).
  // Fields: name, priority, link, link_mod, limit_channels, skinsound, flags

  // Doom sounds.
  // Limits have been added.
  // Boom ignored the priority, except for the one link entry.
  // Doom priority (1..256), 1 is highest priority, 64 is avg
  //               |  link_id
  //               |  |  link_mod
  //               |  |    | limit_channels
  //               |  |    |   |  skinsound
  //               |  |    |   |   |  flags
  { "none\0\0" ,   0, NOL, 0,  0, -1 },    // S_sfx[0] is sfx_None.

  { "pistol"   ,  64, NOL, 0, 99, -1, SFX_saw },
  { "shotgn"   ,  64, NOL, 0, 99, -1 },
  { "sgcock"   ,  64, NOL, 0, 99, -1 },
  { "dshtgn"   ,  64, NOL, 0, 99, -1 },
  { "dbopn\0"  ,  64, NOL, 0, 99, -1 },
  { "dbcls\0"  ,  64, NOL, 0, 99, -1 },
  { "dbload"   ,  64, NOL, 0, 99, -1 },
  { "plasma"   ,  64, NOL, 0, 99, -1 },
  { "bfg\0\0\0",  64, NOL, 0, 99, -1 },
  { "sawup\0"  ,  64, NOL, 0,  4, -1, SFX_saw|SFX_id_fin|SFX_org_kill },
  { "sawidl"   , 118, NOL, 0,  4, -1, SFX_saw|SFX_id_fin|SFX_org_kill },
  { "sawful"   ,  64, NOL, 0,  4, -1, SFX_saw|SFX_id_fin|SFX_org_kill },
  { "sawhit"   ,  64, NOL, 0,  4, -1, SFX_saw|SFX_id_fin|SFX_org_kill },
  { "rlaunc"   ,  64, NOL, 0, 99, -1 },
  { "rxplod"   ,  70, NOL, 0, 99, -1 },
  { "firsht"   ,  70, NOL, 0, 99, -1 },
  { "firxpl"   ,  70, NOL, 0, 99, -1 },
  { "pstart"   , 100, NOL, 0,  4, -1 },
  { "pstop\0"  , 100, NOL, 0,  4, -1 },
  { "doropn"   , 100, NOL, 0,  4, -1, SFX_org_kill },
  { "dorcls"   , 100, NOL, 0,  4, -1, SFX_org_kill },
  { "stnmov"   , 119, NOL, 0,  4, -1, SFX_saw|SFX_id_fin|SFX_org_kill },
  { "swtchn"   ,  78, NOL, 0, 99, -1 },
  { "swtchx"   ,  78, NOL, 0, 99, -1 },
  { "plpain"   ,  96, NOL, 0,  8, SKS_plpain},
  { "dmpain"   ,  96, NOL, 0,  8, -1 },
  { "popain"   ,  96, NOL, 0,  8, -1 },
  { "vipain"   ,  96, NOL, 0,  8, -1 },
  { "mnpain"   ,  96, NOL, 0,  8, -1 },
  { "pepain"   ,  96, NOL, 0,  8, -1 },
  { "slop\0\0" ,  78, NOL, 0,  9, SKS_slop},
  { "itemup"   ,  78, NOL, 0,  2, -1, SFX_single|SFX_player },
  { "wpnup"    ,  78, NOL, 0,  2, -1, SFX_single|SFX_player },
  { "oof\0\0\0",  96, NOL, 0,  2, SKS_oof, SFX_player },
  { "telept"   ,  32, NOL, 0, 99, -1 },
  { "posit1"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "posit2"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "posit3"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "bgsit1"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "bgsit2"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "sgtsit"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "cacsit"   ,  98, NOL, 0, 99, -1, SFX_single},
  { "brssit"   ,  94, NOL, 0, 99, -1, SFX_single},
  { "cybsit"   ,  92, NOL, 0, 99, -1, SFX_single},
  { "spisit"   ,  90, NOL, 0, 99, -1, SFX_single},
  { "bspsit"   ,  90, NOL, 0, 99, -1, SFX_single},
  { "kntsit"   ,  90, NOL, 0, 99, -1, SFX_single},
  { "vilsit"   ,  90, NOL, 0, 99, -1, SFX_single},
  { "mansit"   ,  90, NOL, 0, 99, -1, SFX_single},
  { "pesit\0"  ,  90, NOL, 0, 99, -1, SFX_single},
  { "sklatk"   ,  70, NOL, 0, 99, -1 },
  { "sgtatk"   ,  70, NOL, 0, 99, -1 },
  { "skepch"   ,  70, NOL, 0, 99, -1 },
  { "vilatk"   ,  70, NOL, 0, 99, -1 },
  { "claw\0\0" ,  70, NOL, 0, 99, -1 },
  { "skeswg"   ,  70, NOL, 0, 99, -1 },
  { "pldeth"   ,  32, NOL, 0,  4, SKS_pldeth},
  { "pdiehi"   ,  32, NOL, 0,  4, SKS_pdiehi},
  { "podth1"   ,  70, NOL, 0, 99, -1 },
  { "podth2"   ,  70, NOL, 0, 99, -1 },
  { "podth3"   ,  70, NOL, 0, 99, -1 },
  { "bgdth1"   ,  70, NOL, 0, 99, -1 },
  { "bgdth2"   ,  70, NOL, 0, 99, -1 },
  { "sgtdth"   ,  70, NOL, 0, 99, -1 },
  { "cacdth"   ,  70, NOL, 0, 99, -1 },
  { "skldth"   ,  70, NOL, 0, 99, -1 },
  { "brsdth"   ,  32, NOL, 0, 99, -1 },
  { "cybdth"   ,  32, NOL, 0, 99, -1 },
  { "spidth"   ,  32, NOL, 0, 99, -1 },
  { "bspdth"   ,  32, NOL, 0, 99, -1 },
  { "vildth"   ,  32, NOL, 0, 99, -1 },
  { "kntdth"   ,  32, NOL, 0, 99, -1 },
  { "pedth\0"  ,  32, NOL, 0, 99, -1 },
  { "skedth"   ,  32, NOL, 0, 99, -1 },
  { "posact"   , 120, NOL, 0, 99, -1, SFX_single},
  { "bgact\0"  , 120, NOL, 0, 99, -1, SFX_single},
  { "dmact\0"  , 120, NOL, 0, 99, -1, SFX_single},
  { "bspact"   , 100, NOL, 0, 99, -1, SFX_single},
  { "bspwlk"   , 100, NOL, 0, 99, -1, SFX_single},
  { "vilact"   , 100, NOL, 0, 99, -1, SFX_single},
  { "noway\0"  ,  78, NOL, 0,  4, SKS_noway, SFX_player },
  { "barexp"   ,  60, NOL, 0, 99, -1 },
  { "punch\0"  ,  64, NOL, 0,  4, SKS_punch },
  { "hoof\0\0" ,  70, NOL, 0, 99, -1 },
  { "metal\0"  ,  70, NOL, 0, 99, -1 },
  { "chgun\0"  ,  64, sfx_pistol, 1, 32, -1 },
      // [1] pitch=150, volume +0, normal priority.
      // This is the only link entry in Doom.
  { "tink\0\0" ,  60, NOL, 0, 99, -1 },
  { "bdopn\0"  , 100, NOL, 0, 99, -1 },
  { "bdcls\0"  , 100, NOL, 0, 99, -1 },
  { "itmbk\0"  , 100, NOL, 0, 99, -1 },
  { "flame\0"  ,  32, NOL, 0, 99, -1 },
  { "flamst"   ,  32, NOL, 0, 99, -1 },
  { "getpow"   ,  60, NOL, 0,  4, -1, SFX_player },
  { "bospit"   ,  70, NOL, 0, 99, -1 },
  { "boscub"   ,  70, NOL, 0, 99, -1 },
  { "bossit"   ,  70, NOL, 0, 99, -1 },
  { "bospn\0"  ,  70, NOL, 0, 99, -1 },
  { "bosdth"   ,  70, NOL, 0, 99, -1 },
  { "manatk"   ,  70, NOL, 0, 99, -1 },
  { "mandth"   ,  70, NOL, 0, 99, -1 },
  { "sssit\0"  ,  70, NOL, 0, 99, -1 },
  { "ssdth\0"  ,  70, NOL, 0, 99, -1 },
  { "keenpn"   ,  70, NOL, 0, 99, -1 },
  { "keendt"   ,  70, NOL, 0, 99, -1 },
  { "skeact"   ,  70, NOL, 0, 99, -1 },
  { "skesit"   ,  70, NOL, 0, 99, -1 },
  { "skeatk"   ,  70, NOL, 0, 99, -1 },
  { "radio\0"  ,  60, NOL, 0,  4, SKS_radio },

  // [WDJ] MBF, from MBF, modified for our table.
#ifdef DOGS
  // killough 11/98: dog sounds
  { "dgsit\0"  ,  98, NOL, 0, 99, -1 },
  { "dgatk\0"  ,  70, NOL, 0, 99, -1 },
  { "dgact\0"  , 120, NOL, 0, 99, -1 },
  { "dgdth\0"  ,  70, NOL, 0, 99, -1 },
  { "dgpain"   ,  96, NOL, 0, 99, -1 },
#endif

  //added:22-02-98: sound when the player avatar jumps in air 'hmpf!'
  { "jump\0\0" ,  60, NOL, 0,  4, SKS_jump, SFX_player },
  { "ouch\0\0" ,  64, NOL, 0,  4, SKS_ouch, SFX_player },

  //added:09-08-98:test water sounds
  { "gloop\0"  ,  60, NOL, 0, 99, -1 },
  { "splash"   ,  64, NOL, 0, 99, -1 },
  { "floush"   ,  64, NOL, 0, 99, -1 },

  // Heretic sounds.
  // Heretic highest priority is 255, 1 is lowest, 32 is avg.
  { "gldhit",  32, NOL, 0,  2, -1 },
     // 2 channels
  { "gntful",  32, NOL, 0, 99, -1 },
  { "gnthit",  32, NOL, 0, 99, -1 },
  { "gntpow",  32, NOL, 0, 99, -1 },
//  { "gntact",  32, NOL, 0, 99, -1 },
  { "gntuse",  32, NOL, 0, 99, -1 },
  { "phosht",  32, NOL, 0,  2, -1 },
  { "phohit",  32, NOL, 0, 99, -1 },
  { "-phopow", 32, sfx_hedat1, 0,  1, -1 },
  { "lobsht",  20, NOL, 0,  2, -1 },
  { "lobhit",  20, NOL, 0,  2, -1 },
  { "lobpow",  20, NOL, 0,  2, -1 },
  { "hrnsht",  32, NOL, 0,  2, -1 },
  { "hrnhit",  32, NOL, 0,  2, -1 },
  { "hrnpow",  32, NOL, 0,  2, -1 },
  { "ramphit", 32, NOL, 0,  2, -1 },
  { "ramrain", 10, NOL, 0,  2, -1 },
  { "bowsht",  32, NOL, 0,  2, -1 },
  { "stfhit",  32, NOL, 0,  2, -1 },
  { "stfpow",  32, NOL, 0,  2, -1 },
  { "stfcrk",  32, NOL, 0,  2, -1 },
  { "impsit",  32, NOL, 0,  2, -1 },
  { "impat1",  32, NOL, 0,  2, -1 },
  { "impat2",  32, NOL, 0,  2, -1 },
  { "impdth",  80, NOL, 0,  2, -1 },
  { "-impact", 20, sfx_impsit, 0,  2, -1 },
  { "imppai",  32, NOL, 0,  2, -1 },
  { "mumsit",  32, NOL, 0,  2, -1 },
  { "mumat1",  32, NOL, 0,  2, -1 },
  { "mumat2",  32, NOL, 0,  2, -1 },
  { "mumdth",  80, NOL, 0,  2, -1 },
  { "-mumact", 20, sfx_mumsit, 0,  2, -1 },
  { "mumpai",  32, NOL, 0,  2, -1 },
  { "mumhed",  32, NOL, 0,  2, -1 },
  { "bstsit",  32, NOL, 0,  2, -1 },
  { "bstatk",  32, NOL, 0,  2, -1 },
  { "bstdth",  80, NOL, 0,  2, -1 },
  { "bstact",  20, NOL, 0,  2, -1 },
  { "bstpai",  32, NOL, 0,  2, -1 },
  { "clksit",  32, NOL, 0,  2, -1 },
  { "clkatk",  32, NOL, 0,  2, -1 },
  { "clkdth",  80, NOL, 0,  2, -1 },
  { "clkact",  20, NOL, 0,  2, -1 },
  { "clkpai",  32, NOL, 0,  2, -1 },
  { "snksit",  32, NOL, 0,  2, -1 },
  { "snkatk",  32, NOL, 0,  2, -1 },
  { "snkdth",  80, NOL, 0,  2, -1 },
  { "snkact",  20, NOL, 0,  2, -1 },
  { "snkpai",  32, NOL, 0,  2, -1 },
  { "kgtsit",  32, NOL, 0,  2, -1 },
  { "kgtatk",  32, NOL, 0,  2, -1 },
  { "kgtat2",  32, NOL, 0,  2, -1 },
  { "kgtdth",  80, NOL, 0,  2, -1 },
  { "-kgtact", 20, sfx_kgtsit, 0,  2, -1 },
  { "kgtpai",  32, NOL, 0,  2, -1 },
  { "wizsit",  32, NOL, 0,  2, -1 },
  { "wizatk",  32, NOL, 0,  2, -1 },
  { "wizdth",  80, NOL, 0,  2, -1 },
  { "wizact",  20, NOL, 0,  2, -1 },
  { "wizpai",  32, NOL, 0,  2, -1 },
  { "minsit",  32, NOL, 0,  2, -1 },
  { "minat1",  32, NOL, 0,  2, -1 },
  { "minat2",  32, NOL, 0,  2, -1 },
  { "minat3",  32, NOL, 0,  2, -1 },
  { "mindth",  80, NOL, 0,  2, -1 },
  { "minact",  20, NOL, 0,  2, -1 },
  { "minpai",  32, NOL, 0,  2, -1 },
  { "hedsit",  32, NOL, 0,  2, -1 },
  { "hedat1",  32, NOL, 0,  2, -1 },
  { "hedat2",  32, NOL, 0,  2, -1 },
  { "hedat3",  32, NOL, 0,  2, -1 },
  { "heddth",  80, NOL, 0,  2, -1 },
  { "hedact",  20, NOL, 0,  2, -1 },
  { "hedpai",  32, NOL, 0,  2, -1 },
  { "sorzap",  32, NOL, 0,  2, -1 },
  { "sorrise", 32, NOL, 0,  2, -1 },
  { "sorsit",  200,NOL, 0,  2, -1 },
  { "soratk",  32, NOL, 0,  2, -1 },
  { "soract",  200,NOL, 0,  2, -1 },
  { "sorpai",  200,NOL, 0,  2, -1 },
  { "sordsph", 200,NOL, 0,  2, -1 },
  { "sordexp", 200,NOL, 0,  2, -1 },
  { "sordbon", 200,NOL, 0,  2, -1 },
  { "-sbtsit", 32, sfx_bstsit, 0,  2, -1 },
  { "-sbtatk", 32, sfx_bstatk, 0,  2, -1 },
  { "sbtdth",  80, NOL, 0,  2, -1 },
  { "sbtact",  20, NOL, 0,  2, -1 },
  { "sbtpai",  32, NOL, 0,  2, -1 },
//  { "plroof",  32, NOL, 0,  2, -1 },
  { "plrpai",  32, NOL, 0,  2, -1 },
  { "plrdth",  80, NOL, 0,  2, -1 },
  { "gibdth",  100,NOL, 0,  2, -1 },
  { "plrwdth", 80, NOL, 0,  2, -1 },
  { "plrcdth", 100,NOL, 0,  2, -1 },
  { "itemup",  32, NOL, 0,  2, -1, SFX_player },
  { "wpnup",   32, NOL, 0,  2, -1, SFX_player },
//  { "telept",  50, NOL, 0,  2, -1 },
  { "doropn",  40, NOL, 0,  2, -1 },
  { "dorcls",  40, NOL, 0,  2, -1 },
  { "dormov",  40, NOL, 0,  2, -1 },
  { "artiup",  32, NOL, 0,  2, -1, SFX_player },
//  { "switch",  40, NOL, 0,  2, -1 },
  { "pstart",  40, NOL, 0,  2, -1 },
  { "pstop",   40, NOL, 0,  2, -1 },
  { "stnmov",  40, NOL, 0,  2, -1 },
  { "chicpai", 32, NOL, 0,  2, -1 },
  { "chicatk", 32, NOL, 0,  2, -1 },
  { "chicdth", 40, NOL, 0,  2, -1 },
  { "chicact", 32, NOL, 0,  2, -1 },
  { "chicpk1", 32, NOL, 0,  2, -1 },
  { "chicpk2", 32, NOL, 0,  2, -1 },
  { "chicpk3", 32, NOL, 0,  2, -1 },
  { "keyup"  , 50, NOL, 0,  2, -1, SFX_player },
  { "ripslop", 16, NOL, 0,  2, -1 },
  { "newpod" , 16, NOL, 0, 99, -1 },
  { "podexp" , 40, NOL, 0, 99, -1 },
  { "bounce" , 16, NOL, 0,  2, -1 },
  { "-volsht", 16, sfx_bstatk, 0,  2, -1 },
  { "-volhit", 16, sfx_lobhit, 0,  2, -1 },
  { "burn"   , 10, NOL, 0,  2, -1 },
  { "splash" , 10, NOL, 0,  1, -1 },
  { "gloop"  , 10, NOL, 0,  2, -1 },
//  { "respawn", 10, NOL, 0,  1, -1 },
  { "blssht" , 32, NOL, 0,  2, -1 },
  { "blshit" , 32, NOL, 0,  2, -1 },
//  { "chat"   , 100,NOL, -1,  1, -1 },
  { "artiuse", 32, NOL, 0,  1, -1, SFX_player },
  { "gfrag"  , 100,NOL, 0,  1, -1 },
  { "waterfl", 16, NOL, 0,  2, -1 },

  // Monophonic sounds
  // Low priority

  { "wind"   , 16, NOL, 0,  1, -1 },
  { "amb1"   ,  1, NOL, 0,  1, -1 },
  { "amb2"   ,  1, NOL, 0,  1, -1 },
  { "amb3"   ,  1, NOL, 0,  1, -1 },
  { "amb4"   ,  1, NOL, 0,  1, -1 },
  { "amb5"   ,  1, NOL, 0,  1, -1 },
  { "amb6"   ,  1, NOL, 0,  1, -1 },
  { "amb7"   ,  1, NOL, 0,  1, -1 },
  { "amb8"   ,  1, NOL, 0,  1, -1 },
  { "amb9"   ,  1, NOL, 0,  1, -1 },
  { "amb10"  ,  1, NOL, 0,  1, -1 },
  { "amb11"  ,  1, NOL, 0,  1, -1 },
    
  // Menu sounds
  { "menuud" ,  2, NOL, 0,  1, -1, SFX_single},
  { "menuva" ,  2, NOL, 0,  1, -1, SFX_single},
  { "menuen" ,  2, NOL, 0,  1, -1, SFX_single},
  { "menuop" ,  2, NOL, 0,  1, -1, SFX_single},
  { "menuac" ,  2, NOL, 0,  1, -1, SFX_single},
  // skin sounds free slots to add sounds at run time (Boris HACK!!!)
  // initialized to NULL
// Start of free sounds space.  
  
};

// Default refinfo
sfxinfo_t  sfxinfo_free_ref = { "" ,  60, NOL, 0, 99, SKS_none, 0, -1, NO_LUMP, NULL, 0 };


link_mod_t  link_mods[] =
{
  // Boom has pitch=0 for most entries, we had -1 (which has no meaning)
  {  128,  0 },  // default mods
  {  150,  0 },  // chgun
};


static
void  store_sfx( sfxid_t sfxid, const char *name, sfxinfo_t * refinfo, uint32_t flags )
{
    memcpy( & S_sfx[sfxid], refinfo, sizeof(sfxinfo_t) );

    S_sfx[sfxid].name = (char *) Z_Malloc(7,PU_STATIC,NULL);
    memset( S_sfx[sfxid].name, 0, 7 );
#if defined( __GNUC__ ) && ( __GNUC__ >= 12 )
        // [WDJ] GCC introduced their second guessing of every usage in ver 7.1, and since then
        // every programmer has to circumvent it.
        // With every version of GCC, I have to fix this code again, and it was never broken in the first place.
        // This truncation is intentional as we cannot ever supply an infinite buffer for arbitrary names.
        // Because we support other compilers, which behave better, this cannot just be thrown in the compile flags.
//# pragma GCC diagnostic push
# pragma GCC diagnostic ignored	  "-Wstringop-truncation"
    strncpy(S_sfx[sfxid].name,name,6);
//# pragma GCC diagnostic pop
#else
    strncpy(S_sfx[sfxid].name,name,6);
#endif    
    S_sfx[sfxid].name[6]='\0';
    S_sfx[sfxid].flags= flags;

    // if precache load it here ! todo !
    S_sfx[sfxid].data = NULL;
}

// Only can remove from sfx slots, the loadable sfx sounds.
void S_RemoveSoundFx (sfxid_t sfxid)
{
    if( sfxid >= sfx_freeslot0
        && sfxid <= sfx_freeslot_last
        && S_sfx[sfxid].name)
    {
        if( verbose > 1 )
            GenPrintf(EMSG_ver, "RemoveSoundFx: %s\n", S_sfx[sfxid].name );

        S_FreeSfx(&S_sfx[sfxid]);
        // If name is const char *, then this fails.
        Z_Free(S_sfx[sfxid].name);
        S_sfx[sfxid].lumpnum = NO_LUMP;
        S_sfx[sfxid].name=NULL;  // free sfx slot to use again
    }
}




#ifdef ENABLE_DEH_REMAP
// [WDJ] The sfx_freeslot0 is another system that has not been replaced yet.
// Those sounds are from skins, and fragglescript, and are dynamically allocated,
// and evictable.  Sounds from DEH cannot be replaced when needed, so they
// cannot be in evictable slots.

// [WDJ] Allocate sfx in blocks, to save remap search time at runtime.
// Most sfx are sequential in wads.
// This can only be used with remap, because it requires lookup.
range_t  sfx_free_table_doom[] =
{
#ifdef ENABLE_DEHEXTRA
    { sfx_dehextra_free_start, sfx_dehextra_free_last },
#endif
    { sfx_heretic_start, sfx_heretic_end },
//    { sfx_freeslot0, sfx_freeslot_last },  // cannot use for remap as are evictable
};
#define NUM_sfx_free_table_doom (sizeof(sfx_free_table_doom)/sizeof(range_t))

range_t  sfx_free_table_heretic[] =
{
#ifdef ENABLE_DEHEXTRA
    { sfx_dehextra_free_start, sfx_dehextra_free_last },
#endif
    { sfx_doom_start, sfx_doom_end },
//    { sfx_freeslot0, sfx_freeslot_last },  // cannot use for remap as are evictable
};
#define NUM_sfx_free_table_heretic (sizeof(sfx_free_table_heretic)/sizeof(range_t))

static range_t *  sfx_free_table = NULL;
static byte  num_sfx_free_table = 2;
static byte  sfx_free_table_index = 0;
static range_t free_sfx;


static sfxid_t  get_free_sfx_least_useful( void );


static
sfxid_t get_free_sfx_block( byte num_req, sfxid_t wad_sfxid, const char * sfx_name )
{
    // If current mapping allocation is used up.
    if( (free_sfx.first + num_req) > free_sfx.last )
    {
        if( sfx_free_table_index >= num_sfx_free_table ) // max entries in table
        {
            // [WDJ] Get a slot from the sfx_freeslot0 system.
            // These are replaceable and evictable, so multiple sounds will be mapped to the same slot.
            // The number of slots has been exceeded, so this is unavoidable.
            GenPrintf(EMSG_warn, "SFX %i (%s): remapping exceeds available free space: share mapped to %i\n", wad_sfxid, sfx_name, free_sfx.last );
            sfxid_t sfxid = get_free_sfx_least_useful();
            return sfxid;  // reuse last map
        }

        // Next free section of sprites.
        free_sfx = sfx_free_table[sfx_free_table_index++];
    }

    sfxid_t free_sfx_id = free_sfx.first;

    free_sfx.first += num_req;

    return free_sfx_id;
}

#endif


// [WDJ] Older, search sfx for empty or one to remove.
// older routine for reusing the least useful sfx
static
sfxid_t get_free_sfx_least_useful( void )
{
    uint16_t least_usefulness = 8000;
    sfxid_t  leastid = sfx_None;
    sfxid_t  sfxid;

    // sfx slots are sfxid_t that are reserved for loadable sfx.
    // Quick search for free slot, which start at sfx_freeslot0.
    for(sfxid=sfx_freeslot0; sfxid <= sfx_freeslot_last; sfxid++)
    {
        // find empty sfx slot
        if(!S_sfx[sfxid].name)
           return sfxid;
    }

    // No free slots, try to remove least useful.
    // This is a separate loop because the SoundPlaying test is a search.
    for(sfxid=sfx_freeslot0; sfxid <= sfx_freeslot_last; sfxid++)
    {
        // degrade all equally
        if ( S_sfx[sfxid].usefulness > -8000 )
           S_sfx[sfxid].usefulness --;
        // find least useful
        // all entries must be valid because empty slot search failed
        if ( S_sfx[sfxid].skinsound < 0  // not a skin sound
             && S_sfx[sfxid].usefulness < least_usefulness)
        {
            if (!S_SoundPlaying(NULL, sfxid))
            {
                least_usefulness = S_sfx[sfxid].usefulness;
                leastid = sfxid;
            }
        }
    }
    if ( leastid != sfx_None )
    {
        // clear the leastid slot
        S_RemoveSoundFx( leastid );
        sfxid = leastid;
        return sfxid;
    }

    GenPrintf(EMSG_warn, "\2No more free sound slots\n");
    return 0;  // not valid
}


#ifdef ENABLE_DEH_REMAP

// FIXME, FRAGGLESCRIPT has types and sounds, etc..

// [WDJ] These do not get put into the sfx table,
// until they are encountered, and where they are mapped into.
// This table is indexed by entries in sound_remap.
sfxinfo_t sfx_extensions[ ] =
{
  // [WDJ] Was in DSDA and woof, and I have no idea of its source
  // or uses.  So OF COURSE "something" depends upon it.
  // Was at [sfx_dgpain+1] in woof, where in legacy there are legacy sounds.
  { "secret" ,  60, NOL, 0,  4 },
  // [WDJ] Sounds from woof, do not know anything else about them.
  // In the woof table, they are related to oof, so.
  // Except that there should only be one SKS_oof, or SKS_noway.
  // { "oof\0\0\0",  96, NOL, 0,  2, SKS_oof, SFX_player },
  { "splash" ,  96, NOL, 0, 2, SKS_none, SFX_player },  // duplicate name, but different
  { "ploosh" ,  96, NOL, 0, 2, SKS_none, SFX_player },
  { "livsiz" ,  96, NOL, 0, 2, SKS_none, SFX_player },
  { "splsml" ,  96, NOL, 0, 2, SKS_none, SFX_player },
  { "plosml" ,  96, NOL, 0, 2, SKS_none, SFX_player },
  { "lavsml" ,  96, NOL, 0, 2, SKS_none, SFX_player },

#ifdef ENABLE_DEHEXTRA
  // DSDA claims these at [500] to [699]
  { "fre000",  127, NOL, 0,  1, -1 },  // for all dsfre
#endif
};


#define  SFX_FREE  0xFFF2
#define  SFX_DEHEXTRA  0xFFF3

typedef struct {
   range_t      wad_sfx_id;
   sfxid_t      remap_sfx_id;
   uint16_t     detect; // BDTC_ bits
   uint16_t     sfx_extension_id;  // only used when SFX_FREE, or SFX_DEHEXTRA
} sfx_pre_remap_t;

// These are usually detected by dehacked sound_id range.
sfx_pre_remap_t  sfx_pre_remap_table_doom[] =
{
    { { sfx_dgpain+1, sfx_dgpain+1 }, SFX_FREE, BDTC_dsda | BDTC_MBF21,  0 },
    { { sfx_dgpain+2, sfx_dgpain+7 }, SFX_FREE, BDTC_dsda | BDTC_MBF21,  1 },
    { { 500, 699 }, SFX_DEHEXTRA, (BDTC_dehextra | BDTC_dsda | BDTC_MBF21),  7 },
      // This becomes names "FRE000" to "FRE199"
};
#define NUM_sfx_pre_remap_table_doom  (sizeof(sfx_pre_remap_table_doom)/sizeof(sfx_pre_remap_t))

sfx_pre_remap_t  sfx_pre_remap_table_heretic[] =
{
    // Normal heretic sound_id remapped to our placement.
    { { sfx_doom_start, sfx_doom_start + (sfx_heretic_end - sfx_heretic_start) }, sfx_heretic_start, BDTC_heretic,  0 },
};
#define NUM_sfx_pre_remap_table_heretic  (sizeof(sfx_pre_remap_table_heretic)/sizeof(sfx_pre_remap_t))

sfx_pre_remap_t *  sfx_pre_remap_table = NULL;
byte  num_sfx_pre_remap_table = 0;


range_t  predef_sfx_doom = { sfx_doom_start, sfx_doom_end };
// last unumapped, for EN_sfx_remap settings
// Indexed by EN_sfx_remap.
uint16_t predef_sfx_doom_adapt[] =
{
    sfx_doom_end,  // 0 off, does not matter
    sfx_freeslot0, // 1 almost everything fixed, but only where there are names
    sfx_menu_end,  // mostly fixed
    sfx_doom_ext_end,  // doom stuff plus is fixed
    sfx_doom_end,  // all but predef doom is remapped
    sfx_doom_end,  // auto
};
// As appears in Heretic.
range_t  predef_sfx_heretic = { 0, 0 };  // got to be remapped

range_t  predef_sfx;


//---
// SFX remap

// from infoext.c
extern byte EN_sfx_remap;

// Remap ranges, to save on search time.
typedef struct {
   range_t      wad_sfx_id;  // range of sfx
   sfxid_t      remap_sfx_id;
} sfx_remap_t;

static sfx_remap_t *  sfx_remap = NULL;
static uint16_t  sfx_remap_used = 0;
static uint16_t  sfx_remap_alloc = 0;


static
void  sfx_remap_allocate( void )
{
    // Increase the allocation
    uint16_t req_num = sfx_remap_used + 128;
    sfx_remap_t *  srnew = realloc( sfx_remap, req_num * sizeof( sfx_remap_t ) );
    if( srnew == NULL )
    {
        I_SoftError( "remap allocate failed\n" );
        return;
    }

    // Have additional remap records.
    memset( &srnew[sfx_remap_used], 0, (req_num - sfx_remap_used) * sizeof(sfx_remap_t) );  // zero the new
    sfx_remap = srnew;
    sfx_remap_alloc = req_num;
}


// This must return the same remapping for the same snd_index.
sfxid_t  remap_sound( uint16_t wad_sfx_index )
{
    if( (wad_sfx_index >= predef_sfx.first) && (wad_sfx_index <= predef_sfx.last) )
    {
        return wad_sfx_index;
    }

    char name_buf[20];
    char * name_p;
    sfx_remap_t * rmrfnd = NULL;  // remap record
    sfxid_t sfxid;
    byte sfx_extension_id = 0;
    byte num_req = 1;
    int i;

    // Search the remap state.
    for( i = 0; i < sfx_remap_used; i++ )
    {
        sfx_remap_t * rmr = & sfx_remap[ i ];
        // within the remap range
        if( (wad_sfx_index >= rmr->wad_sfx_id.first) && (wad_sfx_index <= rmr->wad_sfx_id.last) )
        {
            sfxid = (wad_sfx_index - rmr->wad_sfx_id.first) + rmr->remap_sfx_id;
            if( S_sfx[sfxid].name == NULL )
            {
                rmrfnd = rmr;
                break; // already mapped, but name not filled in
            }

            return sfxid;
        }
    }

    // Search the predef remap table.
    for( i=0; i<num_sfx_pre_remap_table; i++ )
    {
        sfx_pre_remap_t *  prermt = & sfx_pre_remap_table[i];
        if( ! (all_detect & prermt->detect) )
        {
            continue;  // not for this context
        }

        // Find record where wad_sfx_index is within its range.
        if( (wad_sfx_index >= prermt->wad_sfx_id.first) && (wad_sfx_index <= prermt->wad_sfx_id.last) )
        {
            sfx_extension_id = prermt->sfx_extension_id;  // index to reference sfx, to copy

#ifdef ENABLE_DEHEXTRA
            if( prermt->remap_sfx_id == SFX_DEHEXTRA )
            {
                // Dehextra sfx names.
                // The lump lookup adds a DS to the name, or another prefix.
                snprintf( name_buf, 18, "FRE%03i", wad_sfx_index - 500 );
                name_buf[18] = 0;
                name_p = name_buf;
                num_req = 64;  // get larger blocks
                if( verbose )
                    GenPrintf(EMSG_warn, "DEHEXTRA SFX %i: given name %s (DS%s)\n", wad_sfx_index, name_buf, name_buf );
                goto remap_from_free;
            }
#endif
            if( prermt->remap_sfx_id == SFX_FREE )
            {
                sfxinfo_t * refinfo = & sfx_extensions[sfx_extension_id];
                name_p = refinfo->name;
                num_req = 8;  // get a few
                goto remap_from_free;
            }

            // Plain sfx translation.
            sfxid = prermt->remap_sfx_id + (wad_sfx_index - prermt->wad_sfx_id.first);
            goto remap_done;
        }
    }


    // Not found
    // The lump lookup adds a DS to the name, or another prefix.
    snprintf( name_buf, 18, "SFX%03i", wad_sfx_index );
    name_buf[18] = 0;
    name_p = name_buf;
    GenPrintf(EMSG_warn, "\2SFX %i: has no name, given name %s (DS%s)\n", wad_sfx_index, name_buf, name_buf );
    num_req = 16;  // no idea how many there will be
    goto remap_from_free;  // unknown gets a slot too

remap_from_free:
    // [WDJ] Get a block of free sfx.  Record the mapping from this wad_sfx_id.
    
    if( rmrfnd )  // already have sfxid from remap table
        goto remap_within_block;

    if( EN_sfx_remap )
    {
        if( num_req > 1 )
        {
            // Remove overlap from request.
            // If it was within a remap block, that would have been detected already.
            uint16_t r2 = wad_sfx_index + num_req - 1;  // last request index
            for( i = 0; i < sfx_remap_used; i++ )  // search remap table
            {
                uint16_t rmf = sfx_remap[ i ].wad_sfx_id.first;
                if( (wad_sfx_index <= rmf) && (r2 >= rmf) )  // rmf is within the requested
                {
                    r2 = rmf - 1;
                    num_req = r2 - wad_sfx_index + 1;
                    // The first encounter may not be the worst, and they are not in order.
                }
            }
        }

        sfxid = get_free_sfx_block( num_req, wad_sfx_index, name_p );
    }
    else
    {
        sfxid = wad_sfx_index; // no remap
    }

    // Get some sfx remap list.
    if( sfx_remap_used >= sfx_remap_alloc )
        sfx_remap_allocate();

    // Record the mapping range
    sfx_remap_t * sr = & sfx_remap[ sfx_remap_used++ ];
    sr->wad_sfx_id.first = wad_sfx_index;
    sr->wad_sfx_id.last = wad_sfx_index + num_req;
    sr->remap_sfx_id = sfxid;

remap_within_block:
    {
        sfxinfo_t * refinfo = & sfx_extensions[sfx_extension_id];
        store_sfx( sfxid, name_p, refinfo, refinfo->flags );
        if( verbose )
            GenPrintf(EMSG_info, "Sfx %i: mapped to %i\n", wad_sfx_index, sfxid );
    }

remap_done:
    return sfxid;
}

#undef NOL
#endif  // ENABLE_DEH_REMAP


// Add a new sound fx into a free sfx slot.
// Does not have an existing sfxid.
// Return sfx id.
sfxid_t  S_AddSoundFx (const char *name, uint32_t flags)
{
    sfxid_t  sfxid;

#ifdef ENABLE_DEH_REMAP
    // block size 1, no wad_sfx_id so cannot map
    sfxid = get_free_sfx_block( 1, 0, name );
#else
    sfxid = get_free_sfx_least_useful();
    if( sfxid == 0 )
        return 0;
#endif

    store_sfx( sfxid, name, & sfxinfo_free_ref, flags );
    return sfxid;
}



// Prepare free sfx slots to add sfx at run time
void S_InitRuntimeSounds (void)
{
    sfxid_t  i;

    for (i=sfx_freeslot0; i<=sfx_freeslot_last; i++)
        S_sfx[i].name = NULL;

#ifdef ENABLE_DEH_REMAP
    if( EN_heretic )
    {
        predef_sfx = predef_sfx_heretic;
        sfx_pre_remap_table = sfx_pre_remap_table_heretic;
        num_sfx_pre_remap_table = NUM_sfx_pre_remap_table_heretic;
        sfx_free_table = sfx_free_table_heretic;
        num_sfx_free_table = NUM_sfx_free_table_heretic;
    }
    else
    {
        predef_sfx = predef_sfx_doom;
        predef_sfx.last = predef_sfx_doom_adapt[EN_sfx_remap];
        sfx_pre_remap_table = sfx_pre_remap_table_doom;
        num_sfx_pre_remap_table = NUM_sfx_pre_remap_table_doom;
        sfx_free_table = sfx_free_table_doom;
        num_sfx_free_table = NUM_sfx_free_table_doom;
    }
#endif    
}





//
// S_AddMusic
// Adds a single song to the runtime songs.
int S_AddMusic( const char * name, const char * prefix )
{
  int    i;

  lumpnum_t music_ln = S_FindExtMusic( name, prefix );
    
  if( ! VALID_LUMP(music_ln) )
      return mus_None; // cannot add an invalid lump

  // Find free slot, store music info there.
  for(i = mus_firstfreeslot; i < mus_lastfreeslot; i++)
  {
    if(S_music[i].name == NULL)
    {
      S_music[i].name = Z_Strdup(name, PU_STATIC, 0);
      S_music[i].lumpnum = music_ln;
      S_music[i].data = 0;
      return i;
    }
  }

  GenPrintf(EMSG_warn, "All music slots are full!\n");
  return mus_None;  // mus_None, not a free slot
}

// Return 0 (mus_None) when not found
int S_FindMusic( const char * name )
{
  int   i;
  for(i = 1; i < NUMMUSIC; i++)  // skip mus_None
  {
    if(!S_music[i].name)
      continue;
    if(!strcasecmp(name, S_music[i].name)) return i;
  }
  return mus_None; // graceful music failure
}


//  prefix = NULL for standard names
// Return -1 (invalid lump) when not found
int S_FindExtMusic( const char * name, const char * prefix )
{
  char  lumpname[32];
  lumpnum_t music_ln = NO_LUMP;

  memset(lumpname, 0, 30);

  // Find odd names
  if( prefix )
  {
    snprintf(lumpname, 31, "%s%.8s", prefix, name );
  }
  else
  {
    snprintf(lumpname, 31, (EN_heretic? "%.8s": "d_%.6s"), name);
  }
    
  lumpname[31] = 0;
  music_ln = W_Check_Namespace( lumpname, LNS_any );
  if( VALID_LUMP(music_ln) )
      return music_ln;

  return -1; // graceful music failure
}
