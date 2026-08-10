// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_chex.c 1738 2025-03-14 08:17:57Z wesleyjohnson $
//
// Copyright (C) 2003-2015 by DooM Legacy Team.
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
//   Chex Quest support.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
#include "m_cheat.h"
#include "dstrings.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_chex.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
  // lspr
#endif

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


void Chex1_PatchEngine(void)
{

        //patch new text
        char* NEW_QUIT1 = "Don't give up now...do\nyou still wish to quit?";
        char* NEW_QUIT2 = "please don't leave we\nneed your help!";

        text[QUITMSG_NUM] = NEW_QUIT1;
        text[QUITMSG1_NUM] = NEW_QUIT2;
        text[QUITMSG2_NUM] = NEW_QUIT2;
        text[QUITMSG3_NUM] = NEW_QUIT2;
        text[QUITMSG4_NUM] = NEW_QUIT2;
        text[QUITMSG5_NUM] = NEW_QUIT2;
        text[QUITMSG6_NUM] = NEW_QUIT2;
        text[QUITMSG7_NUM] = NEW_QUIT2;

        text[QUIT2MSG_NUM] = NEW_QUIT1;
        text[QUIT2MSG1_NUM] = NEW_QUIT2;
        text[QUIT2MSG2_NUM] = NEW_QUIT2;
        text[QUIT2MSG3_NUM] = NEW_QUIT2;
        text[QUIT2MSG4_NUM] = NEW_QUIT2;
        text[QUIT2MSG5_NUM] = NEW_QUIT2;
        text[QUIT2MSG6_NUM] = NEW_QUIT2;

        text[HUSTR_E1M1_NUM] = "E1M1: Landing Zone";
        text[HUSTR_E1M2_NUM] = "E1M2: Storage Facility";
        text[HUSTR_E1M3_NUM] = "E1M3: Experimental Lab";
        text[HUSTR_E1M4_NUM] = "E1M4: Arboretum";
        text[HUSTR_E1M5_NUM] = "E1M5: Caverns of Bazoik";

        text[GOTARMBONUS_NUM] = "picked up slime repellant.";
        text[GOTSTIM_NUM] = "picked up bowl of fruit.";
        text[GOTHTHBONUS_NUM] = "picked up glass of water.";
        text[GOTMEDIKIT_NUM] = "picked up bowl of vegetables.";
        text[GOTMEDINEED_NUM] = "vegetables are REALLY good for you!";
        text[GOTARMOR_NUM] = "Picked up the Chex(R) Armor.";
        text[GOTMEGA_NUM] = "Picked up the Super Chex(R) Armor!";
        text[GOTSUPER_NUM] = "Supercharge Breakfast!";
        text[GOTSUIT_NUM] =	"Slimeproof Suit";
        text[GOTBERSERK_NUM] = "SPOOOON!!!!";
        text[GOTINVIS_NUM] = "Semi-invisibility suit.";
        text[GOTVISOR_NUM] = "Light amplification goggles";

        text[GOTBLUECARD_NUM] = "picked up a blue key.";
        text[GOTYELWCARD_NUM] = "picked up a yellow key.";
        text[GOTREDCARD_NUM] = "picked up a red key.";

        text[GOTCLIP_NUM] = "picked up mini zorch recharge.";
        text[GOTCLIPBOX_NUM] = "Picked up a mini zorch pack.";
        text[GOTROCKET_NUM] = "Picked up a Propulsor recharge.";
        text[GOTROCKBOX_NUM] = "Picked up a 5 pack of propulsor zorch.";
        text[GOTCELL_NUM] = "Picked up a Phasing zorcher recharge.";
        text[GOTCELLBOX_NUM] = "Picked up a phasing zorcher pack.";
        text[GOTSHELLS_NUM] = "Picked up 4 large zorcher recharges.";
        text[GOTSHELLBOX_NUM] = "Picked up a large zorcher pack (20)";
        text[GOTBACKPACK_NUM] = "Picked up a zorchpack.";
        text[GOTBFG9000_NUM] = "Oh, yes.You got the LAZ device!";
        text[GOTCHAINGUN_NUM] = "You got a rapid zorcher!";
        text[GOTCHAINSAW_NUM] = "You got the super bootspork!";
        text[GOTLAUNCHER_NUM] = "You got the zorch propulsor!";
        text[GOTPLASMA_NUM] = "You got the phasing zorcher!";
        text[GOTSHOTGUN_NUM] = "You got the large zorcher!";

        text[STSTR_DQDON_NUM] = "Invincible mode on.";
        text[STSTR_DQDOFF_NUM] = "Invincible mode off.";
        text[STSTR_FAADDED_NUM] = "Zorch Added.";
        text[STSTR_KFAADDED_NUM] = "Zorch and keys added.";
        text[STSTR_CHOPPERS_NUM] = "Eat Chex!";

        text[E1TEXT_NUM] = "mission accomplished.\n\nare you prepared for the next mission?\n\n\n\n\n\n\npress the escape key to continue...\n";
        text[NIGHTMARE_NUM] = "careful. this will be tough.\ndo you wish to continue?\n\npress y or n.";

        text[DEATHMSG_SUICIDE] = "%s needs to be more careful\n";
        text[DEATHMSG_TELEFRAG] = "%s was standing in the wrong spot\n";
        text[DEATHMSG_FIST] = "%s ate from the spoon of %s\n";
        text[DEATHMSG_GUN] = "%s was zorched by %s\n";
        text[DEATHMSG_SHOTGUN] = "%s took a large zorch from %s\n";
        text[DEATHMSG_MACHGUN] = "%s was rapidly zorched by %s\n";
        text[DEATHMSG_ROCKET] = "%s almost dodged %'s propulsor\n";
        text[DEATHMSG_GIBROCKET] = "%s was hit by %'s propulsor\n";
        text[DEATHMSG_PLASMA] = "%s was phased by %s\n";
        text[DEATHMSG_BFGBALL] = "%s couldn't escape %'s LAZ either\n";
        text[DEATHMSG_CHAINSAW] = "%s was fed from %'s bootspork\n";
        text[DEATHMSG_PLAYUNKNOW] = "%s was zorched by %s\n";
        text[DEATHMSG_HELLSLIME] = "%s can't swim in slime\n";
        text[DEATHMSG_NUKE] = "%s can't swim in slime\n";
        text[DEATHMSG_SUPHELLSLIME] = "%s can't swim in slime\n";
        text[DEATHMSG_SPECUNKNOW] = "%s needs to be more careful\n";
        text[DEATHMSG_BARRELFRAG] = "%s was zorched by %s\n";
        text[DEATHMSG_BARREL] = "%s needs to be more careful\n";
        text[DEATHMSG_POSSESSED] = "A felmoid slimed %s\n";
        text[DEATHMSG_SHOTGUY] = "%s was slimed by a bipedicus\n";
        text[DEATHMSG_TROOP] = "%s was slimed by an armored bipedicus\n";
        text[DEATHMSG_SERGEANT] = "%s was slimed by a cycloptis\n";
        text[DEATHMSG_BRUISER] = "%s fell victim to the flembrane\n";
        text[DEATHMSG_DEAD] = "%s was slimed\n";


        //patch monster changes
        mobjinfo[MT_POSSESSED].missilestate = 0;
        mobjinfo[MT_POSSESSED].meleestate = S_POSS_ATK1;

        mobjinfo[MT_SHOTGUY].missilestate = 0;
        mobjinfo[MT_SHOTGUY].meleestate = S_SPOS_ATK1;

        mobjinfo[MT_BRUISER].speed = 0;
        mobjinfo[MT_BRUISER].radius = 48*FRACUNIT; //let's try 48

        //coronas
        // Alter Doom sprite light tables.
//        sprite_light[1].dynamic_color = 0xff000050;
        sprite_light[1].dynamic_color.rgba = RGBA(0x50,0x0,0x0,0xff);
//        sprite_light[2].dynamic_color = 0xff000050;
        sprite_light[2].dynamic_color.rgba = RGBA(0x50,0x0,0x0,0xff);
//        sprite_light[3].dynamic_color = 0xff4040f7;
        sprite_light[3].dynamic_color.rgba = RGBA(0xf7,0x40,0x40,0xff);
//        sprite_light[4].dynamic_color = 0xff4040f7;
        sprite_light[4].dynamic_color.rgba = RGBA(0xf7,0x40,0x40,0xff);
//        sprite_light[5].dynamic_color = 0xff4040f7;
        sprite_light[5].dynamic_color.rgba = RGBA(0xf7,0x40,0x40,0xff);
//        sprite_light[6].dynamic_color = 0xff4040a0;
        sprite_light[6].dynamic_color.rgba = RGBA(0xa0,0x40,0x40,0xff);
        sprite_light[7].splgt_flags = 0;
        sprite_light[8].splgt_flags = 0;
        sprite_light[9].splgt_flags = 0;
        sprite_light[10].splgt_flags = 0;
        sprite_light[11].splgt_flags = 0;
        sprite_light[12].splgt_flags = 0;
        sprite_light[15].light_yoffset = 3.0f;	//light
        sprite_light[16].splgt_flags = 0;
        sprite_light[17].splgt_flags = 0;

#ifdef HWRENDER
        // These changes were executed after HWR_StartupEngine(), so reinit.
    if( rendermode != render_soft )
        HWR_Init_Light();
#endif

        //cheat codes

}

typedef struct {
   char * orig_name;
   char * replace_name;
   uint64_t  checksum;
} chex_picture_replace_t;

#define NUM_CHEX_REPLACEMENTS  6
static chex_picture_replace_t  chex_repl[ NUM_CHEX_REPLACEMENTS ] =
{
   {"PFUB1", "CREDIT",  0x00B251DC },  // bunny
   {"PFUB2", "CREDIT", 0x007B517E },  // bunny
   {"VICTORY2", "CREDIT", 0x002F5EE5 },  // spikes
   {"ENDPIC", "TITLEPIC", 0x006BFE6B },  // bloody marine
   {"INTERPIC", "M_DOOM", 0x004C2CBE },  // wall
   {"M_EPI1", NULL, 0x000A5FF0 },  // wall
};


// Detect Doom graphics and prevent their display while chexquest mode.
void* Chex_safe_pictures( const char* name, void* lumpptr )
{
    int i;

    if( lumpptr == NULL )
    {
        lumpptr = W_CachePatchName ( name, PU_CACHE);
        if( lumpptr == NULL )  goto done;
    }
   
    for( i=0; i<NUM_CHEX_REPLACEMENTS; i++ )
    { 
       if( strncmp( name, chex_repl[i].orig_name, 8 ) == 0 )
       {
           // found a matching entry name
           uint64_t c_checksum = W_lump_checksum( lumpptr );
           // [WDJ] Uncomment the next line to get the checksums of pictures.
//	   debug_Printf( "Chex_safe_pictures: %8.8s  checksum=%16lX\n", name, (long)c_checksum);
           if( chex_repl[i].checksum == c_checksum )
           {
               // it is the original, replace it
               char * replname = chex_repl[i].replace_name;
               if( replname )
               {
                  lumpptr = W_CachePatchName ( replname, PU_LEVEL );
               }
               else
               {
                  lumpptr = NULL;  // return NULL to indicate detection
               }
           }
           goto done;
       }
    }
    // not found, do nothing
done: // other returns
    return lumpptr;
}

