// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: sounds.h 1737 2025-03-14 08:17:22Z wesleyjohnson $
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
// $Log: sounds.h,v $
// Revision 1.6  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.5  2001/02/24 13:35:21  bpereira
//
// Revision 1.4  2001/01/25 22:15:44  bpereira
// added heretic support
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
//      Created by the sound utility written by Dave Taylor.
//      Kept as a sample, DOOM2  sounds. Frozen.
//
//-----------------------------------------------------------------------------

#ifndef SOUNDS_H
#define SOUNDS_H

#include "doomdef.h"
#include "doomtype.h"
  // sfxid_t, stdint

// 10 customisable sounds for Skins
// [WDJ] Not actually referenced directly.
// Are entered into tables.
// Guess they must be in this order.
typedef enum {
    SKS_plpain,
    SKS_slop,
    SKS_oof,
    SKS_pldeth,
    SKS_pdiehi,
    SKS_noway,
    SKS_punch,
    SKS_radio,
    SKS_jump,
    SKS_ouch,
    NUMSKINSOUNDS,
    SKS_none = 255,  // or any value > NUMSKINSOUNDS
} skinsound_e;


// free sfx for S_AddSoundFx()
// MAXSKINS
#define NUMSFXFREESLOTS    ((32*NUMSKINSOUNDS)+10)
#define NUMMUSICFREESLOTS  64

// [WDJ] Convert singularity, multiplicity, and Saw tests to flags.
// Modified similar to Hexen, and Edge.
// uint16_t
typedef enum {
   SFX_num = 0x03,   // Number allowed mask (Hexen)
   SFX_single = 0x01,  // Sfx has only one source (play only one at a time)
   SFX_two    = 0x02,  // Allows two playing sound copies  (TODO)
 // flags on num, single
   SFX_id_fin = 0x04,  // finish existing sound of same sfxid (Edge)
 // kill sound
   SFX_player = 0x10,  // Player sound, (PrBoom pickup)
   SFX_saw    = 0x20,  // Saw sound
   SFX_org_kill = 0x1000,  // can kill existing sound when same origin
   SFX_boss   = 0x2000,  // Special treatment for bosses (Edge)
} sfxflag_e;

//
// SoundFX struct.
//
typedef struct sfxinfo_struct   sfxinfo_t;

struct sfxinfo_struct
{
// fields loaded by sounds init
    // up to 6-character name
    char * name;
      // Assigned const strings for built-in names.
      // Allocated strings for loaded names and deh.
      // If name is const char *, then cannot remove loadable sounds.

    // Sfx priority, 1 is highest, 64 is avg.
    uint16_t    priority;

    // referenced sound if a link
    uint16_t    link_id;   // index to another sound in S_sfx, 0 = NULL
    byte        link_mod;  // index to param modifiers (only chgun actually)

    byte        limit_channels;  // number of channels allowed

    // sound that can be remapped for a skin, indexes skins[].skinsounds
    // 0 up to (NUMSKINSOUNDS-1), -1 or 255 = not skin specifc
    byte        skinsound;  // there are only 10 of them

    uint16_t    flags;  // sfxflag_e

// end of fields loaded by sounds init

#ifdef CLEANUP   
    // this is checked every second to see if sound
    // can be thrown out (if 0, then decrement, if -1,
    // then throw out, if > 0, then it is in use)
#endif
    int16_t     usefulness;  // lowest is removed

    // lump number of sfx
    lumpnum_t   lumpnum;

    void*       data;      // sound data

    int32_t     length;  // length of sound data
};

typedef struct {
    int16_t     pitch;
    int16_t     mod_volume;
} link_mod_t;



//
// MusicInfo struct.
//
typedef struct
{
    // up to 6-character name
    char * name;
      // Assigned const strings for built-in names.
      // Allocated strings for loaded names and deh.

    // lump number of music
    lumpnum_t   lumpnum;

    // music data
    byte *      data;

    // music handle once registered
    int handle;

} musicinfo_t;




// the complete set of sound effects
extern sfxinfo_t        S_sfx[];
extern link_mod_t       link_mods[];

// the complete set of music
extern musicinfo_t      S_music[];

//
// Identifiers for all music in game.
//

typedef enum
{
  mus_None,
  mus_e1m1,
  mus_e1m2,
  mus_e1m3,
  mus_e1m4,
  mus_e1m5,
  mus_e1m6,
  mus_e1m7,
  mus_e1m8,
  mus_e1m9,
  mus_e2m1,
  mus_e2m2,
  mus_e2m3,
  mus_e2m4,
  mus_e2m5,
  mus_e2m6,
  mus_e2m7,
  mus_e2m8,
  mus_e2m9,
  mus_e3m1,
  mus_e3m2,
  mus_e3m3,
  mus_e3m4,
  mus_e3m5,
  mus_e3m6,
  mus_e3m7,
  mus_e3m8,
  mus_e3m9,
  mus_inter,
  mus_intro,
  mus_bunny,
  mus_victor,
  mus_introa,
  mus_runnin,
  mus_stalks,
  mus_countd,
  mus_betwee,
  mus_doom,
  mus_the_da,
  mus_shawn,
  mus_ddtblu,
  mus_in_cit,
  mus_dead,
  mus_stlks2,
  mus_theda2,
  mus_doom2,
  mus_ddtbl2,
  mus_runni2,
  mus_dead2,
  mus_stlks3,
  mus_romero,
  mus_shawn2,
  mus_messag,
  mus_count2,
  mus_ddtbl3,
  mus_ampie,
  mus_theda3,
  mus_adrian,
  mus_messg2,
  mus_romer2,
  mus_tense,
  mus_shawn3,
  mus_openin,
  mus_evil,
  mus_ultima,
  mus_read_m,
  mus_dm2ttl,
  mus_dm2int,

// heretic stuff
  mus_he1m1,
  mus_he1m2,
  mus_he1m3,
  mus_he1m4,
  mus_he1m5,
  mus_he1m6,
  mus_he1m7,
  mus_he1m8,
  mus_he1m9,
            
  mus_he2m1,
  mus_he2m2,
  mus_he2m3,
  mus_he2m4,
  mus_he2m5,
  mus_he2m6,
  mus_he2m7,
  mus_he2m8,
  mus_he2m9,

  mus_he3m1,
  mus_he3m2,
  mus_he3m3,
  mus_he3m4,
  mus_he3m5,
  mus_he3m6,
  mus_he3m7,
  mus_he3m8,
  mus_he3m9,

  mus_he4m1,
  mus_he4m2,
  mus_he4m3,
  mus_he4m4,
  mus_he4m5,
  mus_he4m6,
  mus_he4m7,
  mus_he4m8,
  mus_he4m9,

  mus_he5m1,
  mus_he5m2,
  mus_he5m3,
  mus_he5m4,
  mus_he5m5,
  mus_he5m6,
  mus_he5m7,
  mus_he5m8,
  mus_he5m9,

  mus_he6m1,
  mus_he6m2,
  mus_he6m3,
            
  mus_htitl,
  mus_hcptd,
    NUMMUSIC_DEF,

  mus_firstfreeslot = NUMMUSIC_DEF,
    // 64 free slots here
  mus_lastfreeslot = mus_firstfreeslot + NUMMUSICFREESLOTS - 1,

    NUMMUSIC
} musicenum_e;


//
// Identifiers for all sfx in game.
//

typedef enum
{
    sfx_None,
// Doom sounds
    sfx_pistol,
        sfx_doom_start = sfx_pistol,
    sfx_shotgn,
    sfx_sgcock,
    sfx_dshtgn,
    sfx_dbopn,
    sfx_dbcls,
    sfx_dbload,
    sfx_plasma,
    sfx_bfg,
    sfx_sawup,
    sfx_sawidl,
    sfx_sawful,
    sfx_sawhit,
    sfx_rlaunc,
    sfx_rxplod,
    sfx_firsht,
    sfx_firxpl,
    sfx_pstart,  // 18
    sfx_pstop,
    sfx_doropn,
    sfx_dorcls,
    sfx_stnmov,
    sfx_swtchn,
    sfx_swtchx,
    sfx_plpain,
    sfx_dmpain,
    sfx_popain,
    sfx_vipain,
    sfx_mnpain,
    sfx_pepain,
    sfx_slop,
    sfx_itemup,
    sfx_wpnup,
    sfx_oof,  // 34
    sfx_telept,
    sfx_posit1,
    sfx_posit2,
    sfx_posit3,
    sfx_bgsit1,
    sfx_bgsit2,
    sfx_sgtsit,
    sfx_cacsit,
    sfx_brssit,
    sfx_cybsit,
    sfx_spisit,
    sfx_bspsit,
    sfx_kntsit,
    sfx_vilsit,
    sfx_mansit,
    sfx_pesit,
    sfx_sklatk,
    sfx_sgtatk,
    sfx_skepch,
    sfx_vilatk,
    sfx_claw,   // 55
    sfx_skeswg,
    sfx_pldeth,
    sfx_pdiehi,
    sfx_podth1,
    sfx_podth2,
    sfx_podth3,
    sfx_bgdth1,
    sfx_bgdth2,
    sfx_sgtdth,
    sfx_cacdth,
    sfx_skldth,
    sfx_brsdth,
    sfx_cybdth,
    sfx_spidth,
    sfx_bspdth,
    sfx_vildth,
    sfx_kntdth,
    sfx_pedth,
    sfx_skedth,
    sfx_posact,
    sfx_bgact,
    sfx_dmact,
    sfx_bspact,
    sfx_bspwlk,
    sfx_vilact,
    sfx_noway,  // 81
    sfx_barexp,
    sfx_punch,
    sfx_hoof,
    sfx_metal,
    sfx_chgun,
    sfx_tink,
    sfx_bdopn,
    sfx_bdcls,
    sfx_itmbk,
    sfx_flame,
    sfx_flamst,
    sfx_getpow,
    sfx_bospit,
    sfx_boscub,
    sfx_bossit,
    sfx_bospn,
    sfx_bosdth,
    sfx_manatk,
    sfx_mandth,  // 100
    sfx_sssit,
    sfx_ssdth,
    sfx_keenpn,
    sfx_keendt,
    sfx_skeact,
    sfx_skesit,
    sfx_skeatk,
    sfx_radio,
        sfx_doom_end = sfx_radio,  // 108
#ifdef DOGS
    // killough 11/98: dog sounds
    sfx_dgsit,
        sfx_dogs_start = sfx_dgsit,  // 109
    sfx_dgatk,
    sfx_dgact,
    sfx_dgdth,
    sfx_dgpain,
#endif
    // These are sounded by player actions.
    //added:22-02-98: player avatar jumps
    sfx_jump,   // 114
    //added:22-02-98: player hits something hard and says 'ouch!'
    sfx_ouch,
    //test water
    sfx_gloop,
    sfx_splash,
    sfx_floush,
        sfx_doom_ext_end = sfx_floush,  // 118

// heretic stuff
    sfx_gldhit,
        sfx_heretic_start = sfx_gldhit,  // 119
    sfx_gntful,
    sfx_gnthit,
    sfx_gntpow,
//      sfx_gntact,
    sfx_gntuse,
    sfx_phosht,
    sfx_phohit,
    sfx_phopow,
    sfx_lobsht,
    sfx_lobhit,
    sfx_lobpow,
    sfx_hrnsht,
    sfx_hrnhit,
    sfx_hrnpow,
    sfx_ramphit,
    sfx_ramrain,
    sfx_bowsht,  // 135
    sfx_stfhit,
    sfx_stfpow,
    sfx_stfcrk,
    sfx_impsit,
    sfx_impat1,
    sfx_impat2,
    sfx_impdth,
    sfx_impact,
    sfx_imppai,
    sfx_mumsit,
    sfx_mumat1,
    sfx_mumat2,
    sfx_mumdth,
    sfx_mumact,
    sfx_mumpai,  // 150
    sfx_mumhed,
    sfx_bstsit,
    sfx_bstatk,
    sfx_bstdth,
    sfx_bstact,
    sfx_bstpai,
    sfx_clksit,
    sfx_clkatk,
    sfx_clkdth,
    sfx_clkact,
    sfx_clkpai,
    sfx_snksit,
    sfx_snkatk,
    sfx_snkdth,
    sfx_snkact,
    sfx_snkpai,
    sfx_kgtsit,
    sfx_kgtatk,
    sfx_kgtat2,
    sfx_kgtdth,
    sfx_kgtact,
    sfx_kgtpai,
    sfx_wizsit,
    sfx_wizatk,
    sfx_wizdth,
    sfx_wizact,
    sfx_wizpai,
    sfx_minsit,
    sfx_minat1,
    sfx_minat2,  // 180
    sfx_minat3,
    sfx_mindth,
    sfx_minact,
    sfx_minpai,
    sfx_hedsit,
    sfx_hedat1,
    sfx_hedat2,
    sfx_hedat3,
    sfx_heddth,
    sfx_hedact,
    sfx_hedpai,
    sfx_sorzap,
    sfx_sorrise,
    sfx_sorsit,
    sfx_soratk,
    sfx_soract,
    sfx_sorpai,
    sfx_sordsph,
    sfx_sordexp,
    sfx_sordbon,  // 200
    sfx_sbtsit,
    sfx_sbtatk,
    sfx_sbtdth,
    sfx_sbtact,
    sfx_sbtpai,  // 205
//      sfx_plroof,
    sfx_plrpai,
    sfx_plrdth,             // Normal
    sfx_gibdth,             // Extreme
    sfx_plrwdth,    // Wimpy
    sfx_plrcdth,    // Crazy
    sfx_hitemup,
    sfx_hwpnup,  // 212
//      sfx_htelept,
    sfx_hdoropn,
    sfx_hdorcls,
    sfx_dormov,
    sfx_artiup,  // 216
//      sfx_switch,
    sfx_hpstart,
    sfx_hpstop,
    sfx_hstnmov,
    sfx_chicpai,
    sfx_chicatk,
    sfx_chicdth,
    sfx_chicact,
    sfx_chicpk1,
    sfx_chicpk2,
    sfx_chicpk3,
    sfx_keyup,
    sfx_ripslop,
    sfx_newpod,
    sfx_podexp,
    sfx_bounce,
    sfx_volsht,
    sfx_volhit,
    sfx_burn,
    sfx_hsplash,
    sfx_hgloop,  // 236
//      sfx_respawn,
    sfx_blssht,
    sfx_blshit,
//      sfx_chat,
    sfx_artiuse,
    sfx_gfrag,
    sfx_waterfl,

// Monophonic sounds

    sfx_wind,  // 242
    sfx_amb1,
    sfx_amb2,
    sfx_amb3,
    sfx_amb4,
    sfx_amb5,
    sfx_amb6,
    sfx_amb7,
    sfx_amb8,
    sfx_amb9,
    sfx_amb10,
    sfx_amb11,
        sfx_heretic_end = sfx_amb11,  // 253

    sfx_menuud,  // menu up down
        sfx_menu_start = sfx_menuud,  // 254
    sfx_menuva,  // menu value change
    sfx_menuen,  // menu enter
    sfx_menuop,  // menu open
    sfx_menuac,  // menu action
        sfx_menu_end = sfx_menuac,  // 258

        NUMSFX_DEF,

#ifdef ENABLE_DEHEXTRA
    sfx_dehextra_free_start = NUMSFX_DEF,
# if DEHEXTRA_MAX_SOUNDS > 270
    // free slots 259 to  699 (or as set in doomdef.h)
    sfx_dehextra_free_last = DEHEXTRA_MAX_SOUNDS,
# else    
    sfx_dehextra_free_last = NUMSFX_DEF + 20,
# endif
#endif

    // free slots for S_AddSoundFx() at run-time --------------------
    sfx_freeslot0,
    //
    // ... 60 free sounds here ...
    //
    // Use the defined free slots
    sfx_freeslot_last = (sfx_freeslot0+NUMSFXFREESLOTS-1),
    // end of freeslots ---------------------------------------------

        NUMSFX_EXT,
} sfxenum_e;

// typedef uint16_t   sfxid_t;  // defined in doomtype.h


void   S_InitRuntimeSounds (void);
// Add a new sound fx into a free sfx slot.
// Return sfx id.
sfxid_t  S_AddSoundFx (const char *name, uint32_t flags);
// Only can remove from sfx slots, the loadable sfx sounds.
void   S_RemoveSoundFx (sfxid_t sfxid);

#ifdef ENABLE_DEH_REMAP
sfxid_t  remap_sound( uint16_t snd_index );
#endif

// Return 0 (mus_None) when not found
int S_FindMusic( const char * name );
//  prefix = NULL for standard names
// Return -1 (invalid lump) when not found
int S_FindExtMusic( const char * name, const char * prefix );
// Return 0 (mus_None) when not found
int S_AddMusic( const char * name, const char * prefix );
#endif
