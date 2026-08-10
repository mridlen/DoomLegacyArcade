// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_englsh.h 1112 2014-06-03 21:54:41Z smite-meister $
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
// $Log: d_englsh.h,v $
// Revision 1.6  2001/05/16 21:21:14  bpereira
//
// Revision 1.5  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.4  2000/04/16 18:38:06  bpereira
//
// Revision 1.3  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Printed strings for translation.
//      English language support (default).
//
//-----------------------------------------------------------------------------

#ifndef D_TEXT_ENG_H
#define D_TEXT_ENG_H


extern char *text[];


//
//      Printed strings for translation
//

//
// D_Main.C
//
#define D_DEVSTR          text[D_DEVSTR_NUM]
#define D_CDROM           text[D_CDROM_NUM]

//
//      M_Menu.C
//
#define PRESSKEY          text[PRESSKEY_NUM]
#define PRESSYN           text[PRESSYN_NUM]
#define LOADNET           text[LOADNET_NUM]
#define QLOADNET          text[QLOADNET_NUM]
#define QSAVESPOT         text[QSAVESPOT_NUM]
#define SAVEDEAD          text[SAVEDEAD_NUM]
#define QSPROMPT          text[QSPROMPT_NUM]
#define QLPROMPT          text[QLPROMPT_NUM]
#define NEWGAME           text[NEWGAME_NUM]
#define NIGHTMARE         text[NIGHTMARE_NUM]
#define SWSTRING          text[SWSTRING_NUM]
#define MSGOFF            text[MSGOFF_NUM]
#define MSGON             text[MSGON_NUM]
#define NETEND            text[NETEND_NUM]
#define ENDGAME           text[ENDGAME_NUM]
#define DOSY              text[DOSY_NUM]
#define DETAILHI          text[DETAILHI_NUM]
#define DETAILLO          text[DETAILLO_NUM]
#define GAMMALVL0         text[GAMMALVL0_NUM]
#define GAMMALVL1         text[GAMMALVL1_NUM]
#define GAMMALVL2         text[GAMMALVL2_NUM]
#define GAMMALVL3         text[GAMMALVL3_NUM]
#define GAMMALVL4         text[GAMMALVL4_NUM]
#define EMPTYSTRING       text[EMPTYSTRING_NUM]

//
//      P_inter.C
//
#define GOTARMOR          text[GOTARMOR_NUM]
#define GOTMEGA           text[GOTMEGA_NUM]
#define GOTHTHBONUS       text[GOTHTHBONUS_NUM]
#define GOTARMBONUS       text[GOTARMBONUS_NUM]
#define GOTSTIM           text[GOTSTIM_NUM]
#define GOTMEDINEED       text[GOTMEDINEED_NUM]
#define GOTMEDIKIT        text[GOTMEDIKIT_NUM]
#define GOTSUPER          text[GOTSUPER_NUM]
#define GOTBLUECARD       text[GOTBLUECARD_NUM]
#define GOTYELWCARD       text[GOTYELWCARD_NUM]
#define GOTREDCARD        text[GOTREDCARD_NUM]
#define GOTBLUESKUL       text[GOTBLUESKUL_NUM]
#define GOTYELWSKUL       text[GOTYELWSKUL_NUM]
#define GOTREDSKULL       text[GOTREDSKULL_NUM]
#define GOTINVUL          text[GOTINVUL_NUM]
#define GOTBERSERK        text[GOTBERSERK_NUM]
#define GOTINVIS          text[GOTINVIS_NUM]
#define GOTSUIT           text[GOTSUIT_NUM]
#define GOTMAP            text[GOTMAP_NUM]
#define GOTVISOR          text[GOTVISOR_NUM]
#define GOTMSPHERE        text[GOTMSPHERE_NUM]
#define GOTCLIP           text[GOTCLIP_NUM]
#define GOTCLIPBOX        text[GOTCLIPBOX_NUM]
#define GOTROCKET         text[GOTROCKET_NUM]
#define GOTROCKBOX        text[GOTROCKBOX_NUM]
#define GOTCELL           text[GOTCELL_NUM]
#define GOTCELLBOX        text[GOTCELLBOX_NUM]
#define GOTSHELLS         text[GOTSHELLS_NUM]
#define GOTSHELLBOX       text[GOTSHELLBOX_NUM]
#define GOTBACKPACK       text[GOTBACKPACK_NUM]
#define GOTBFG9000        text[GOTBFG9000_NUM]
#define GOTCHAINGUN       text[GOTCHAINGUN_NUM]
#define GOTCHAINSAW       text[GOTCHAINSAW_NUM]
#define GOTLAUNCHER       text[GOTLAUNCHER_NUM]
#define GOTPLASMA         text[GOTPLASMA_NUM]
#define GOTSHOTGUN        text[GOTSHOTGUN_NUM]
#define GOTSHOTGUN2       text[GOTSHOTGUN2_NUM]

//
// P_Doors.C
//
#define PD_BLUEO          text[PD_BLUEO_NUM]
#define PD_REDO           text[PD_REDO_NUM]
#define PD_YELLOWO        text[PD_YELLOWO_NUM]
#define PD_BLUEK          text[PD_BLUEK_NUM]
#define PD_REDK           text[PD_REDK_NUM]
#define PD_YELLOWK        text[PD_YELLOWK_NUM]

//SoM: 3/9/2000: Add new key messages.
#define PD_BLUEC          text[PD_BLUEC_NUM]
#define PD_REDC           text[PD_REDC_NUM]
#define PD_YELLOWC        text[PD_YELLOWC_NUM]
#define PD_BLUES          text[PD_BLUES_NUM]
#define PD_REDS           text[PD_REDS_NUM]
#define PD_YELLOWS        text[PD_YELLOWS_NUM]
#define PD_ANY            text[PD_ANY_NUM]
#define PD_ALL3           text[PD_ALL3_NUM]
#define PD_ALL6           text[PD_ALL6_NUM]

//
//      G_game.C
//
#define GGSAVED           text[GGSAVED_NUM]

//
//      HU_stuff.C
//
#define HUSTR_MSGU        text[HUSTR_MSGU_NUM]
#define HUSTR_E1M1        text[HUSTR_E1M1_NUM]
#define HUSTR_E1M2        text[HUSTR_E1M2_NUM]
#define HUSTR_E1M3        text[HUSTR_E1M3_NUM]
#define HUSTR_E1M4        text[HUSTR_E1M4_NUM]
#define HUSTR_E1M5        text[HUSTR_E1M5_NUM]
#define HUSTR_E1M6        text[HUSTR_E1M6_NUM]
#define HUSTR_E1M7        text[HUSTR_E1M7_NUM]
#define HUSTR_E1M8        text[HUSTR_E1M8_NUM]
#define HUSTR_E1M9        text[HUSTR_E1M9_NUM]
#define HUSTR_E2M1        text[HUSTR_E2M1_NUM]
#define HUSTR_E2M2        text[HUSTR_E2M2_NUM]
#define HUSTR_E2M3        text[HUSTR_E2M3_NUM]
#define HUSTR_E2M4        text[HUSTR_E2M4_NUM]
#define HUSTR_E2M5        text[HUSTR_E2M5_NUM]
#define HUSTR_E2M6        text[HUSTR_E2M6_NUM]
#define HUSTR_E2M7        text[HUSTR_E2M7_NUM]
#define HUSTR_E2M8        text[HUSTR_E2M8_NUM]
#define HUSTR_E2M9        text[HUSTR_E2M9_NUM]
#define HUSTR_E3M1        text[HUSTR_E3M1_NUM]
#define HUSTR_E3M2        text[HUSTR_E3M2_NUM]
#define HUSTR_E3M3        text[HUSTR_E3M3_NUM]
#define HUSTR_E3M4        text[HUSTR_E3M4_NUM]
#define HUSTR_E3M5        text[HUSTR_E3M5_NUM]
#define HUSTR_E3M6        text[HUSTR_E3M6_NUM]
#define HUSTR_E3M7        text[HUSTR_E3M7_NUM]
#define HUSTR_E3M8        text[HUSTR_E3M8_NUM]
#define HUSTR_E3M9        text[HUSTR_E3M9_NUM]
#define HUSTR_E4M1        text[HUSTR_E4M1_NUM]
#define HUSTR_E4M2        text[HUSTR_E4M2_NUM]
#define HUSTR_E4M3        text[HUSTR_E4M3_NUM]
#define HUSTR_E4M4        text[HUSTR_E4M4_NUM]
#define HUSTR_E4M5        text[HUSTR_E4M5_NUM]
#define HUSTR_E4M6        text[HUSTR_E4M6_NUM]
#define HUSTR_E4M7        text[HUSTR_E4M7_NUM]
#define HUSTR_E4M8        text[HUSTR_E4M8_NUM]
#define HUSTR_E4M9        text[HUSTR_E4M9_NUM]
#define HUSTR_1           text[HUSTR_1_NUM]
#define HUSTR_2           text[HUSTR_2_NUM]
#define HUSTR_3           text[HUSTR_3_NUM]
#define HUSTR_4           text[HUSTR_4_NUM]
#define HUSTR_5           text[HUSTR_5_NUM]
#define HUSTR_6           text[HUSTR_6_NUM]
#define HUSTR_7           text[HUSTR_7_NUM]
#define HUSTR_8           text[HUSTR_8_NUM]
#define HUSTR_9           text[HUSTR_9 _NUM]
#define HUSTR_10          text[HUSTR_10_NUM]
#define HUSTR_11          text[HUSTR_11_NUM]
#define HUSTR_12          text[HUSTR_12_NUM]
#define HUSTR_13          text[HUSTR_13_NUM]
#define HUSTR_14          text[HUSTR_14_NUM]
#define HUSTR_15          text[HUSTR_15_NUM]
#define HUSTR_16          text[HUSTR_16_NUM]
#define HUSTR_17          text[HUSTR_17_NUM]
#define HUSTR_18          text[HUSTR_18_NUM]
#define HUSTR_19          text[HUSTR_19_NUM]
#define HUSTR_20          text[HUSTR_20_NUM]
#define HUSTR_21          text[HUSTR_21_NUM]
#define HUSTR_22          text[HUSTR_22_NUM]
#define HUSTR_23          text[HUSTR_23_NUM]
#define HUSTR_24          text[HUSTR_24_NUM]
#define HUSTR_25          text[HUSTR_25_NUM]
#define HUSTR_26          text[HUSTR_26_NUM]
#define HUSTR_27          text[HUSTR_27_NUM]
#define HUSTR_28          text[HUSTR_28_NUM]
#define HUSTR_29          text[HUSTR_29_NUM]
#define HUSTR_30          text[HUSTR_30_NUM]
#define HUSTR_31          text[HUSTR_31_NUM]
#define HUSTR_32          text[HUSTR_32_NUM]
#define PHUSTR_1          text[PHUSTR_1_NUM]
#define PHUSTR_2          text[PHUSTR_2_NUM]
#define PHUSTR_3          text[PHUSTR_3_NUM]
#define PHUSTR_4          text[PHUSTR_4_NUM]
#define PHUSTR_5          text[PHUSTR_5_NUM]
#define PHUSTR_6          text[PHUSTR_6_NUM]
#define PHUSTR_7          text[PHUSTR_7_NUM]
#define PHUSTR_8          text[PHUSTR_8_NUM]
#define PHUSTR_9          text[PHUSTR_9_NUM]
#define PHUSTR_10         text[PHUSTR_10_NUM]
#define PHUSTR_11         text[PHUSTR_11_NUM]
#define PHUSTR_12         text[PHUSTR_12_NUM]
#define PHUSTR_13         text[PHUSTR_13_NUM]
#define PHUSTR_14         text[PHUSTR_14_NUM]
#define PHUSTR_15         text[PHUSTR_15_NUM]
#define PHUSTR_16         text[PHUSTR_16_NUM]
#define PHUSTR_17         text[PHUSTR_17_NUM]
#define PHUSTR_18         text[PHUSTR_18_NUM]
#define PHUSTR_19         text[PHUSTR_19_NUM]
#define PHUSTR_20         text[PHUSTR_20_NUM]
#define PHUSTR_21         text[PHUSTR_21_NUM]
#define PHUSTR_22         text[PHUSTR_22_NUM]
#define PHUSTR_23         text[PHUSTR_23_NUM]
#define PHUSTR_24         text[PHUSTR_24_NUM]
#define PHUSTR_25         text[PHUSTR_25_NUM]
#define PHUSTR_26         text[PHUSTR_26_NUM]
#define PHUSTR_27         text[PHUSTR_27_NUM]
#define PHUSTR_28         text[PHUSTR_28_NUM]
#define PHUSTR_29         text[PHUSTR_29_NUM]
#define PHUSTR_30         text[PHUSTR_30_NUM]
#define PHUSTR_31         text[PHUSTR_31_NUM]
#define PHUSTR_32         text[PHUSTR_32_NUM]
#define THUSTR_1          text[THUSTR_1_NUM]
#define THUSTR_2          text[THUSTR_2_NUM]
#define THUSTR_3          text[THUSTR_3_NUM]
#define THUSTR_4          text[THUSTR_4_NUM]
#define THUSTR_5          text[THUSTR_5_NUM]
#define THUSTR_6          text[THUSTR_6_NUM]
#define THUSTR_7          text[THUSTR_7_NUM]
#define THUSTR_8          text[THUSTR_8_NUM]
#define THUSTR_9          text[THUSTR_9_NUM]
#define THUSTR_10         text[THUSTR_10_NUM]
#define THUSTR_11         text[THUSTR_11_NUM]
#define THUSTR_12         text[THUSTR_12_NUM]
#define THUSTR_13         text[THUSTR_13_NUM]
#define THUSTR_14         text[THUSTR_14_NUM]
#define THUSTR_15         text[THUSTR_15_NUM]
#define THUSTR_16         text[THUSTR_16_NUM]
#define THUSTR_17         text[THUSTR_17_NUM]
#define THUSTR_18         text[THUSTR_18_NUM]
#define THUSTR_19         text[THUSTR_19_NUM]
#define THUSTR_20         text[THUSTR_20_NUM]
#define THUSTR_21         text[THUSTR_21_NUM]
#define THUSTR_22         text[THUSTR_22_NUM]
#define THUSTR_23         text[THUSTR_23_NUM]
#define THUSTR_24         text[THUSTR_24_NUM]
#define THUSTR_25         text[THUSTR_25_NUM]
#define THUSTR_26         text[THUSTR_26_NUM]
#define THUSTR_27         text[THUSTR_27_NUM]
#define THUSTR_28         text[THUSTR_28_NUM]
#define THUSTR_29         text[THUSTR_29_NUM]
#define THUSTR_30         text[THUSTR_30_NUM]
#define THUSTR_31         text[THUSTR_31_NUM]
#define THUSTR_32         text[THUSTR_32_NUM]
#define HUSTR_CHATMACRO1  text[HUSTR_CHATMACRO1_NUM]
#define HUSTR_CHATMACRO2  text[HUSTR_CHATMACRO2_NUM]
#define HUSTR_CHATMACRO3  text[HUSTR_CHATMACRO3_NUM]
#define HUSTR_CHATMACRO4  text[HUSTR_CHATMACRO4_NUM]
#define HUSTR_CHATMACRO5  text[HUSTR_CHATMACRO5_NUM]
#define HUSTR_CHATMACRO6  text[HUSTR_CHATMACRO6_NUM]
#define HUSTR_CHATMACRO7  text[HUSTR_CHATMACRO7_NUM]
#define HUSTR_CHATMACRO8  text[HUSTR_CHATMACRO8_NUM]
#define HUSTR_CHATMACRO9  text[HUSTR_CHATMACRO9_NUM]
#define HUSTR_CHATMACRO0  text[HUSTR_CHATMACRO0_NUM]
#define HUSTR_TALKTOSELF1 text[HUSTR_TALKTOSELF1_NUM]
#define HUSTR_TALKTOSELF2 text[HUSTR_TALKTOSELF2_NUM]
#define HUSTR_TALKTOSELF3 text[HUSTR_TALKTOSELF3_NUM]
#define HUSTR_TALKTOSELF4 text[HUSTR_TALKTOSELF4_NUM]
#define HUSTR_TALKTOSELF5 text[HUSTR_TALKTOSELF5_NUM]
#define HUSTR_MESSAGESENT text[HUSTR_MESSAGESENT_NUM]

// The following should NOT be changed unless it seems
// just AWFULLY necessary

#define HUSTR_KEYGREEN  'g'
#define HUSTR_KEYINDIGO 'i'
#define HUSTR_KEYBROWN  'b'
#define HUSTR_KEYRED    'r'

//
//      AM_map.C
//

#define AMSTR_FOLLOWON     text[AMSTR_FOLLOWON_NUM]
#define AMSTR_FOLLOWOFF    text[AMSTR_FOLLOWOFF_NUM]
#define AMSTR_GRIDON       text[AMSTR_GRIDON_NUM]
#define AMSTR_GRIDOFF      text[AMSTR_GRIDOFF_NUM]
#define AMSTR_MARKEDSPOT   text[AMSTR_MARKEDSPOT_NUM]
#define AMSTR_MARKSCLEARED text[AMSTR_MARKSCLEARED_NUM]

//
//      ST_stuff.C
//

#define STSTR_MUS          text[STSTR_MUS_NUM]
#define STSTR_NOMUS        text[STSTR_NOMUS_NUM]
#define STSTR_DQDON        text[STSTR_DQDON_NUM]
#define STSTR_DQDOFF       text[STSTR_DQDOFF_NUM]
#define STSTR_KFAADDED     text[STSTR_KFAADDED_NUM]
#define STSTR_FAADDED      text[STSTR_FAADDED_NUM]
#define STSTR_NCON         text[STSTR_NCON_NUM]
#define STSTR_NCOFF        text[STSTR_NCOFF_NUM]
#define STSTR_BEHOLD       text[STSTR_BEHOLD_NUM]
#define STSTR_BEHOLDX      text[STSTR_BEHOLDX_NUM]
#define STSTR_CHOPPERS     text[STSTR_CHOPPERS_NUM]
#define STSTR_CLEV         text[STSTR_CLEV_NUM]

//
//      F_Finale.C
//
#define E1TEXT             text[E1TEXT_NUM]
#define E2TEXT             text[E2TEXT_NUM]
#define E3TEXT             text[E3TEXT_NUM]
#define E4TEXT             text[E4TEXT_NUM]

// after level 6] put this:
#define C1TEXT             text[C1TEXT_NUM]

// After level 11] put this:
#define C2TEXT             text[C2TEXT_NUM]

// After level 20] put this:
#define C3TEXT             text[C3TEXT_NUM]

// After level 29] put this:
#define C4TEXT             text[C4TEXT_NUM]

// Before level 31] put this:
#define C5TEXT             text[C5TEXT_NUM]

// Before level 32] put this:

#define C6TEXT             text[C6TEXT_NUM]

#define T1TEXT             text[T1TEXT_NUM]
#define T2TEXT             text[T2TEXT_NUM]
#define T3TEXT             text[T3TEXT_NUM]
#define T4TEXT             text[T4TEXT_NUM]
#define T5TEXT             text[T5TEXT_NUM]
#define T6TEXT             text[T6TEXT_NUM]

//
// Character cast strings F_FINALE.C
//
#define CC_ZOMBIE          text[CC_ZOMBIE_NUM]
#define CC_SHOTGUN         text[CC_SHOTGUN_NUM]
#define CC_HEAVY           text[CC_HEAVY_NUM]
#define CC_IMP             text[CC_IMP_NUM]
#define CC_DEMON           text[CC_DEMON_NUM]
#define CC_LOST            text[CC_LOST_NUM]
#define CC_CACO            text[CC_CACO_NUM]
#define CC_HELL            text[CC_HELL _NUM]
#define CC_BARON           text[CC_BARON_NUM]
#define CC_ARACH           text[CC_ARACH_NUM]
#define CC_PAIN            text[CC_PAIN_NUM]
#define CC_REVEN           text[CC_REVEN_NUM]
#define CC_MANCU           text[CC_MANCU_NUM]
#define CC_ARCH            text[CC_ARCH_NUM]
#define CC_SPIDER          text[CC_SPIDER_NUM]
#define CC_CYBER           text[CC_CYBER_NUM]
#define CC_HERO            text[CC_HERO_NUM]

// heretic
#define TXT_ARTIHEALTH           text[TXT_ARTIHEALTH_NUM]
#define TXT_ARTIFLY              text[TXT_ARTIFLY_NUM]
#define TXT_ARTIINVULNERABILITY  text[TXT_ARTIINVULNERABILITY_NUM]
#define TXT_ARTITOMEOFPOWER      text[TXT_ARTITOMEOFPOWER_NUM]
#define TXT_ARTIINVISIBILITY     text[TXT_ARTIINVISIBILITY_NUM]
#define TXT_ARTIEGG              text[TXT_ARTIEGG_NUM]
#define TXT_ARTISUPERHEALTH      text[TXT_ARTISUPERHEALTH_NUM]
#define TXT_ARTITORCH            text[TXT_ARTITORCH_NUM]
#define TXT_ARTIFIREBOMB         text[TXT_ARTIFIREBOMB_NUM]
#define TXT_ARTITELEPORT         text[TXT_ARTITELEPORT_NUM]

#define TXT_AMMOGOLDWAND1        text[TXT_AMMOGOLDWAND1_NUM]
#define TXT_AMMOGOLDWAND2        text[TXT_AMMOGOLDWAND2_NUM]
#define TXT_AMMOMACE1            text[TXT_AMMOMACE1_NUM]
#define TXT_AMMOMACE2            text[TXT_AMMOMACE2_NUM]
#define TXT_AMMOCROSSBOW1        text[TXT_AMMOCROSSBOW1_NUM]
#define TXT_AMMOCROSSBOW2        text[TXT_AMMOCROSSBOW2_NUM]
#define TXT_AMMOBLASTER1         text[TXT_AMMOBLASTER1_NUM]
#define TXT_AMMOBLASTER2         text[TXT_AMMOBLASTER2_NUM]
#define TXT_AMMOSKULLROD1        text[TXT_AMMOSKULLROD1_NUM]
#define TXT_AMMOSKULLROD2        text[TXT_AMMOSKULLROD2_NUM]
#define TXT_AMMOPHOENIXROD1      text[TXT_AMMOPHOENIXROD1_NUM]
#define TXT_AMMOPHOENIXROD2      text[TXT_AMMOPHOENIXROD2_NUM]

#define TXT_WPNMACE              text[TXT_WPNMACE_NUM]
#define TXT_WPNCROSSBOW          text[TXT_WPNCROSSBOW_NUM]
#define TXT_WPNBLASTER           text[TXT_WPNBLASTER_NUM]
#define TXT_WPNSKULLROD          text[TXT_WPNSKULLROD_NUM]
#define TXT_WPNPHOENIXROD        text[TXT_WPNPHOENIXROD_NUM]
#define TXT_WPNGAUNTLETS         text[TXT_WPNGAUNTLETS_NUM]

#define TXT_ITEMBAGOFHOLDING     text[TXT_ITEMBAGOFHOLDING_NUM]

#define TXT_CHEATGODON           text[TXT_CHEATGODON_NUM]
#define TXT_CHEATGODOFF          text[TXT_CHEATGODOFF_NUM]
#define TXT_CHEATNOCLIPON        text[TXT_CHEATNOCLIPON_NUM]
#define TXT_CHEATNOCLIPOFF       text[TXT_CHEATNOCLIPOFF_NUM]
#define TXT_CHEATWEAPONS         text[TXT_CHEATWEAPONS_NUM]
#define TXT_CHEATFLIGHTON        text[TXT_CHEATFLIGHTON_NUM]
#define TXT_CHEATFLIGHTOFF       text[TXT_CHEATFLIGHTOFF_NUM]
#define TXT_CHEATPOWERON         text[TXT_CHEATPOWERON_NUM]
#define TXT_CHEATPOWEROFF        text[TXT_CHEATPOWEROFF_NUM]
#define TXT_CHEATHEALTH          text[TXT_CHEATHEALTH_NUM]
#define TXT_CHEATKEYS            text[TXT_CHEATKEYS_NUM]
#define TXT_CHEATSOUNDON         text[TXT_CHEATSOUNDON_NUM]
#define TXT_CHEATSOUNDOFF        text[TXT_CHEATSOUNDOFF_NUM]
#define TXT_CHEATTICKERON        text[TXT_CHEATTICKERON_NUM]
#define TXT_CHEATTICKEROFF       text[TXT_CHEATTICKEROFF_NUM]
#define TXT_CHEATARTIFACTS1      text[TXT_CHEATARTIFACTS1_NUM]
#define TXT_CHEATARTIFACTS2      text[TXT_CHEATARTIFACTS2_NUM]
#define TXT_CHEATARTIFACTS3      text[TXT_CHEATARTIFACTS3_NUM]
#define TXT_CHEATARTIFACTSFAIL   text[TXT_CHEATARTIFACTSFAIL_NUM]
#define TXT_CHEATWARP            text[TXT_CHEATWARP_NUM]
#define TXT_CHEATSCREENSHOT      text[TXT_CHEATSCREENSHOT_NUM]
#define TXT_CHEATCHICKENON       text[TXT_CHEATCHICKENON_NUM]
#define TXT_CHEATCHICKENOFF      text[TXT_CHEATCHICKENOFF_NUM]
#define TXT_CHEATMASSACRE        text[TXT_CHEATMASSACRE_NUM]
#define TXT_CHEATIDDQD           text[TXT_CHEATIDDQD_NUM]
#define TXT_CHEATIDKFA           text[TXT_CHEATIDKFA_NUM]


#endif
