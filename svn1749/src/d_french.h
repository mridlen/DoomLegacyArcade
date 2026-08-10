// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_french.h 1141 2015-04-03 13:41:01Z wesleyjohnson $
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
//
// DESCRIPTION:
//      Printed strings, french translation.
//      Incomplete.  Needs translation of some strings.
//
//-----------------------------------------------------------------------------

#ifndef D_FRENCH_H
#define D_FRENCH_H

#include "doomdef.h"

#ifdef FRENCH_INLINE
#include "dstrings.h"

// [WDJ] 8/5/2011 This is incomplete as it includes mostly orignal game
// text strings, and not many error strings.

#define COPY_TO_TEXT( src, index1 )  memcpy( &text[index1], src, sizeof(src) )


// Called before game determined, or DEH strings saved
void french_early_text( void )
{
#if 0
// Needs to be translated

//text[CREDIT_NUM] = "CREDIT";
//text[HELP2_NUM] = "HELP2";
//text[VICTORY2_NUM] = "VICTORY2";
//text[ENDPIC_NUM] = "ENDPIC";

text[MODIFIED_NUM] =
    "===========================================================================\n"
    "ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
    "get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
    "        You will not receive technical support for modified games.\n"
    "                      press enter to continue\n"
    "===========================================================================\n"
     ;
text[SHAREWARE_NUM] =
    "===========================================================================\n"
    "                     This program is Free Software!\n"
    "===========================================================================\n"
     ;
text[COMERCIAL_NUM] =
    "===========================================================================\n"
    "                     This program is Free Software!\n"
    "             See the terms of the GNU General Public License\n"
    "===========================================================================\n"
     ;

text[AUSTIN_NUM] = "Austin Virtual Gaming: Levels will end after 20 minutes\n"; // AUSTIN
text[M_LOAD_NUM] = "M_LoadDefaults: Load system defaults.\n";
text[Z_INIT_NUM] = "Z_Init: Init zone memory allocation daemon. \n";
text[W_INIT_NUM] = "W_Init: Init WADfiles.\n";
text[M_INIT_NUM] = "M_Init: Init miscellaneous info.\n";
text[R_INIT_NUM] = "R_Init: Init DOOM refresh daemon - ";
text[P_INIT_NUM] = "\nP_Init: Init Playloop state.\n";
text[I_INIT_NUM] = "I_Init: Setting up machine state.\n";
text[D_CHECKNET_NUM] = "D_Startup_NetGame: Checking network game status.\n";
text[S_SETSOUND_NUM] = "S_Init: Setting up sound.\n";
text[HU_INIT_NUM] = "HU_Init: Setting up heads up display.\n";
text[ST_INIT_NUM] = "ST_Init: Init status bar.\n";
text[STATREG_NUM] = "External statistics registered.\n";
#endif
}

// HUSTR_CHATMACRO1_NUM, HUSTR_TALKTOSELF5_NUM
static char * french_chat_text[] =
{
// HUSTR_CHATMACRO1_NUM .. HUSTR_CHATMACRO0_NUM
 "JE SUIS PRET A LEUR EN FAIRE BAVER!",
 "JE VAIS BIEN.",
 "JE N'AI PAS L'AIR EN FORME!",
 "AU SECOURS!",
 "TU CRAINS!",
 "LA PROCHAINE FOIS, MINABLE...",
 "VIENS ICI!",
 "JE VAIS M'EN OCCUPER.",
 "OUI",
 "NON",

// HUSTR_TALKTOSELF1_NUM ..  HUSTR_TALKTOSELF5
 "VOUS PARLEZ TOUT SEUL ",
 "QUI EST LA?",
 "VOUS VOUS FAITES PEUR ",
 "VOUS COMMENCEZ A DELIRER ",
 "VOUS ETES LARGUE..."
};

#if 0
// Needs to be translated
static char * french_quitmsg_text[] = {
// DOOM1: QUITMSG, QUITMSG1 ... QUITMSG7_NUM
"are you sure you want to\nquit this great game?",
"please don't leave, there's more\ndemons to toast!",
"let's beat it -- this is turning\ninto a bloodbath!",
"i wouldn't leave if i were you.\nyour os is much worse.",
"you're trying to say you like your os\nbetter than me, right?",
"don't leave yet -- there's a\ndemon around that corner!",
"ya know, next time you come in here\ni'm gonna toast ya.",
"go ahead and leave. see if i care.",

// QuitDOOM II messages, QUIT2MSG, QUIT2MSG1 ... QUIT2MSG6_NUM
"you want to quit?\nthen, thou hast lost an eighth!",
"don't go now, there's a \ndimensional shambler waiting\nat the shell prompt!",
"get outta here and go back\nto your boring programs.",
"if i were your boss, i'd \n deathmatch ya in a minute!",
"look, bud. you leave now\nand you forfeit your body count!",
"just leave. when you come\nback, i'll be waiting with a bat.",
"you're lucky i don't smack\nyou for thinking about leaving."
};
#endif

#if 0
// Needs to be translated
static char * french_deathmsg_text[] =
{
 "%s suicides\n",  // DEATHMSG_SUICIDE
 "%s was telefragged by %s\n", // DEATHMSG_TELEFRAG
 "%s was beaten to a pulp by %s\n", // DEATHMSG_FIST
 "%s was gunned by %s\n", // DEATHMSG_GUN
 "%s was shot down by %s\n", // DEATHMSG_SHOTGUN
 "%s was machine-gunned by %s\n", // DEATHMSG_MACHGUN
 "%s was caught by %s's rocket\n", // DEATHMSG_ROCKET
 "%s was gibbed by %s's rocket\n", // DEATHMSG_GIBROCKET
 "%s eats %s's toaster\n", // DEATHMSG_PLASMA
 "%s enjoys %s's big fraggin' gun\n", // DEATHMSG_BFGBALL
 "%s was divided up into little pieces by %s's chainsaw\n", // DEATHMSG_CHAINSAW
 "%s ate 2 loads of %s's buckshot\n", // DEATHMSG_SUPSHOTGUN
 "%s was killed by %s\n", // DEATHMSG_PLAYUNKNOW
 "%s dies in hellslime\n", // DEATHMSG_HELLSLIME
 "%s gulped a load of nukage\n", // DEATHMSG_NUKE
 "%s dies in super hellslime/strobe hurt\n", // DEATHMSG_SUPHELLSLIME
 "%s dies in special sector\n", // DEATHMSG_SPECUNKNOW
 "%s was barrel-fragged by %s\n", // DEATHMSG_BARRELFRAG
 "%s dies from a barrel explosion\n", // DEATHMSG_BARREL
 "%s was shot by a possessed\n", // DEATHMSG_POSSESSED
 "%s was shot down by a shotguy\n", // DEATHMSG_SHOTGUY
 "%s was blasted by an Arch-vile\n", // DEATHMSG_VILE
 "%s was exploded by a Mancubus\n", // DEATHMSG_FATSO
 "%s was punctured by a Chainguy\n", // DEATHMSG_CHAINGUY
 "%s was fried by an Imp\n", // DEATHMSG_TROOP
 "%s was eviscerated by a Demon\n", // DEATHMSG_SERGEANT
 "%s was mauled by a Shadow Demon\n", // DEATHMSG_SHADOWS
 "%s was fried by a Caco-demon\n", // DEATHMSG_HEAD
 "%s was slain by a Baron of Hell\n", // DEATHMSG_BRUISER
 "%s was smashed by a Revenant\n", // DEATHMSG_UNDEAD
 "%s was slain by a Hell-Knight\n", // DEATHMSG_KNIGHT
 "%s was killed by a Lost Soul\n", // DEATHMSG_SKULL
 "%s was killed by a The Spider Mastermind\n", // DEATHMSG_SPIDER
 "%s was killed by a Arachnotron\n", // DEATHMSG_BABY
 "%s was crushed by the Cyber-demon\n", // DEATHMSG_CYBORG
 "%s was killed by a Pain Elemental\n", // DEATHMSG_PAIN
 "%s was killed by a WolfSS\n", // DEATHMSG_WOLFSS
 "%s died\n", // DEATHMSG_DEAD
};
#endif

// Ultimate Doom level map names
// HUSTR_E1M1_NUM, HUSTR_E3M9_NUM
static char * french_doom_text[] =
{
// HUSTR_E1M1_NUM .. HUST_E1M9_NUM
 "E1M1: HANGAR",
 "E1M2: USINE NUCLEAIRE ",
 "E1M3: RAFFINERIE DE TOXINES ",
 "E1M4: CENTRE DE CONTROLE ",
 "E1M5: LABORATOIRE PHOBOS ",
 "E1M6: TRAITEMENT CENTRAL ",
 "E1M7: CENTRE INFORMATIQUE ",
 "E1M8: ANOMALIE PHOBOS ",
 "E1M9: BASE MILITAIRE ",

// HUSTR_E2M1_NUM .. HUST_E2M9_NUM
 "E2M1: ANOMALIE DEIMOS ",
 "E2M2: ZONE DE CONFINEMENT ",
 "E2M3: RAFFINERIE",
 "E2M4: LABORATOIRE DEIMOS ",
 "E2M5: CENTRE DE CONTROLE ",
 "E2M6: HALLS DES DAMNES ",
 "E2M7: CUVES DE REPRODUCTION ",
 "E2M8: TOUR DE BABEL ",
 "E2M9: FORTERESSE DU MYSTERE ",

// HUSTR_E3M1_NUM .. HUST_E3M9_NUM
 "E3M1: DONJON DE L'ENFER ",
 "E3M2: BOURBIER DU DESESPOIR ",
 "E3M3: PANDEMONIUM",
 "E3M4: MAISON DE LA DOULEUR ",
 "E3M5: CATHEDRALE PROFANE ",
 "E3M6: MONT EREBUS",
 "E3M7: LIMBES",
 "E3M8: DIS",
 "E3M9: CLAPIERS"
};

// Doom2 level map names
// HUSTR_1_NUM, HUSTR_32_NUM
static char * french_doom2_text[] =
{
// HUSTR_1_NUM .. HUSTR_32_NUM
 "NIVEAU 1: ENTREE ",
 "NIVEAU 2: HALLS SOUTERRAINS ",
 "NIVEAU 3: LE FEU NOURRI ",
 "NIVEAU 4: LE FOYER ",
 "NIVEAU 5: LES EGOUTS ",
 "NIVEAU 6: LE BROYEUR ",
 "NIVEAU 7: L'HERBE DE LA MORT",
 "NIVEAU 8: RUSES ET PIEGES ",
 "NIVEAU 9: LE PUITS ",
 "NIVEAU 10: BASE DE RAVITAILLEMENT ",
 "NIVEAU 11: LE CERCLE DE LA MORT!",

 "NIVEAU 12: L'USINE ",
 "NIVEAU 13: LE CENTRE VILLE",
 "NIVEAU 14: LES ANTRES PROFONDES ",
 "NIVEAU 15: LA ZONE INDUSTRIELLE ",
 "NIVEAU 16: LA BANLIEUE",
 "NIVEAU 17: LES IMMEUBLES",
 "NIVEAU 18: LA COUR ",
 "NIVEAU 19: LA CITADELLE ",
 "NIVEAU 20: JE T'AI EU!",

 "NIVEAU 21: LE NIRVANA",
 "NIVEAU 22: LES CATACOMBES ",
 "NIVEAU 23: LA GRANDE FETE ",
 "NIVEAU 24: LE GOUFFRE ",
 "NIVEAU 25: LES CHUTES DE SANG",
 "NIVEAU 26: LES MINES ABANDONNEES ",
 "NIVEAU 27: CHEZ LES MONSTRES ",
 "NIVEAU 28: LE MONDE DE L'ESPRIT ",
 "NIVEAU 29: LA LIMITE ",
 "NIVEAU 30: L'ICONE DU PECHE ",

 "NIVEAU 31: WOLFENSTEIN",
 "NIVEAU 32: LE MASSACRE",
};

// CC_ZOMBIE_NUM, CC_HERO_NUM
static char * french_cast_text[] =
{
 "ZOMBIE", // CC_ZOMBIE
 "TYPE AU FUSIL", // CC_SHOTGUN
 "MEC SUPER-ARME", // CC_HEAVY
 "DIABLOTIN", // CC_IMP
 "DEMON", // CC_DEMON
 "AME PERDUE", // CC_LOST
 "CACODEMON", // CC_CACO
 "CHEVALIER DE L'ENFER", // CC_HELL
 "BARON DE L'ENFER", // CC_BARON
 "ARACHNOTRON", // CC_ARACH
 "ELEMENTAIRE DE LA DOULEUR", // CC_PAIN
 "REVENANT", // CC_REVEN
 "MANCUBUS", // CC_MANCU
 "ARCHI-INFAME", // CC_ARCH
 "L'ARAIGNEE CERVEAU", // CC_SPIDER
 "LE CYBERDEMON", // CC_CYBER
 "NOTRE HEROS", // CC_HERO
};


void french_text( void )
{
//
// D_Main.C
//
text[D_DEVSTR_NUM] =    "MODE DEVELOPPEMENT ON.\n";
text[D_CDROM_NUM] =     "VERSION CD-ROM: DEFAULT.CFG DANS C:\\DOOMDATA\n";

//
//      M_Menu.C
//
#ifdef PRESSKEY
#undef PRESSKEY
#undef PRESSYN
#endif
#define PRESSKEY        "APPUYEZ SUR UNE TOUCHE.";
#define PRESSYN         "APPUYEZ SUR Y OU N";

text[QUITMSG_NUM] =     "VOUS VOULEZ VRAIMENT\nQUITTER CE SUPER JEU?";
text[LOADNET_NUM] =     "VOUS NE POUVEZ PAS CHARGER\nUN JEU EN RESEAU!\n\n"PRESSKEY;
text[QLOADNET_NUM] =    "CHARGEMENT RAPIDE INTERDIT EN RESEAU!\n\n"PRESSKEY;
text[QSAVESPOT_NUM] =   "VOUS N'AVEZ PAS CHOISI UN EMPLACEMENT!\n\n"PRESSKEY;
text[SAVEDEAD_NUM] =    "VOUS NE POUVEZ PAS SAUVER SI VOUS NE JOUEZ "
"PAS!\n\n"PRESSKEY;
text[QSPROMPT_NUM] =    "SAUVEGARDE RAPIDE DANS LE FICHIER \n\n'%s'?\n\n"PRESSYN;
text[QLPROMPT_NUM] =    "VOULEZ-VOUS CHARGER LA SAUVEGARDE"
"\n\n'%s'?\n\n"PRESSYN;
text[NEWGAME_NUM] =     "VOUS NE POUVEZ PAS LANCER\n"
"UN NOUVEAU JEU SUR RESEAU.\n\n"PRESSKEY;
text[NIGHTMARE_NUM] =   "VOUS CONFIRMEZ? CE NIVEAU EST\n"
"VRAIMENT IMPITOYABLE!n"PRESSYN;
text[SWSTRING_NUM] =    "CECI EST UNE VERSION SHAREWARE DE DOOM.\n\n"
"VOUS DEVRIEZ COMMANDER LA TRILOGIE COMPLETE.\n\n"PRESSKEY;
text[MSGOFF_NUM] =      "MESSAGES OFF";
text[MSGON_NUM] =       "MESSAGES ON";
text[NETEND_NUM] =      "VOUS NE POUVEZ PAS METTRE FIN A UN JEU SUR "
"RESEAU!\n\n"PRESSKEY;
text[ENDGAME_NUM] =     "VOUS VOULEZ VRAIMENT METTRE FIN AU JEU?\n\n"PRESSYN;

text[DOSY_NUM] =        "(APPUYEZ SUR Y POUR REVENIR AU OS.)";

text[EMPTYSTRING_NUM] = "EMPLACEMENT VIDE";

#if 0
// not used in DoomLegacy
text[DETAILHI_NUM] =    "GRAPHISMES MAXIMUM ";
text[DETAILLO_NUM] =    "GRAPHISMES MINIMUM ";
text[GAMMALVL0_NUM] =   "CORRECTION GAMMA OFF";
text[GAMMALVL1_NUM] =   "CORRECTION GAMMA NIVEAU 1";
text[GAMMALVL2_NUM] =   "CORRECTION GAMMA NIVEAU 2";
text[GAMMALVL3_NUM] =   "CORRECTION GAMMA NIVEAU 3";
text[GAMMALVL4_NUM] =   "CORRECTION GAMMA NIVEAU 4";
#endif

//
//      P_inter.C
//
text[GOTARMOR_NUM] =    "ARMURE RECUPEREE.";
text[GOTMEGA_NUM] =     "MEGA-ARMURE RECUPEREE!";
text[GOTHTHBONUS_NUM] = "BONUS DE SANTE RECUPERE.";
text[GOTARMBONUS_NUM] = "BONUS D'ARMURE RECUPERE.";
text[GOTSTIM_NUM] =     "STIMPACK RECUPERE.";
text[GOTMEDINEED_NUM] = "MEDIKIT RECUPERE. VOUS EN AVEZ VRAIMENT BESOIN!";
text[GOTMEDIKIT_NUM] =  "MEDIKIT RECUPERE.";
text[GOTSUPER_NUM] =    "SUPERCHARGE!";

text[GOTBLUECARD_NUM] = "CARTE MAGNETIQUE BLEUE RECUPEREE.";
text[GOTYELWCARD_NUM] = "CARTE MAGNETIQUE JAUNE RECUPEREE.";
text[GOTREDCARD_NUM] =  "CARTE MAGNETIQUE ROUGE RECUPEREE.";
text[GOTBLUESKUL_NUM] = "CLEF CRANE BLEUE RECUPEREE.";
text[GOTYELWSKUL_NUM] = "CLEF CRANE JAUNE RECUPEREE.";
text[GOTREDSKULL_NUM] = "CLEF CRANE ROUGE RECUPEREE.";

text[GOTINVUL_NUM] =    "INVULNERABILITE!";
text[GOTBERSERK_NUM] =  "BERSERK!";
text[GOTINVIS_NUM] =    "INVISIBILITE PARTIELLE ";
text[GOTSUIT_NUM] =     "COMBINAISON ANTI-RADIATIONS ";
text[GOTMAP_NUM] =      "CARTE INFORMATIQUE ";
text[GOTVISOR_NUM] =    "VISEUR A AMPLIFICATION DE LUMIERE ";
text[GOTMSPHERE_NUM] =  "MEGASPHERE!";

text[GOTCLIP_NUM] =     "CHARGEUR RECUPERE.";
text[GOTCLIPBOX_NUM] =  "BOITE DE BALLES RECUPEREE.";
text[GOTROCKET_NUM] =   "ROQUETTE RECUPEREE.";
text[GOTROCKBOX_NUM] =  "CAISSE DE ROQUETTES RECUPEREE.";
text[GOTCELL_NUM] =     "CELLULE D'ENERGIE RECUPEREE.";
text[GOTCELLBOX_NUM] =  "PACK DE CELLULES D'ENERGIE RECUPERE.";
text[GOTSHELLS_NUM] =   "4 CARTOUCHES RECUPEREES.";
text[GOTSHELLBOX_NUM] = "BOITE DE CARTOUCHES RECUPEREE.";
text[GOTBACKPACK_NUM] = "SAC PLEIN DE MUNITIONS RECUPERE!";

text[GOTBFG9000_NUM] =  "VOUS AVEZ UN BFG9000!  OH, OUI!";
text[GOTCHAINGUN_NUM] = "VOUS AVEZ LA MITRAILLEUSE!";
text[GOTCHAINSAW_NUM] = "UNE TRONCONNEUSE!";
text[GOTLAUNCHER_NUM] = "VOUS AVEZ UN LANCE-ROQUETTES!";
text[GOTPLASMA_NUM] =   "VOUS AVEZ UN FUSIL A PLASMA!";
text[GOTSHOTGUN_NUM] =  "VOUS AVEZ UN FUSIL!";
text[GOTSHOTGUN2_NUM] = "VOUS AVEZ UN SUPER FUSIL!";

//
// P_Doors.C
//
// cannot activate switch
text[PD_BLUEO_NUM] =    "IL VOUS FAUT UNE CLEF BLEUE";
text[PD_REDO_NUM] =     "IL VOUS FAUT UNE CLEF ROUGE";
text[PD_YELLOWO_NUM] =  "IL VOUS FAUT UNE CLEF JAUNE";
// cannot open door
text[PD_BLUEK_NUM] =    "IL VOUS FAUT UNE CLEF BLEUE";
text[PD_REDK_NUM] =     "IL VOUS FAUT UNE CLEF ROUGE";
text[PD_YELLOWK_NUM] =  "IL VOUS FAUT UNE CLEF JAUNE";
// Boom need key card, not skull
text[PD_BLUEC_NUM] =    "You need a blue card to open this door";
text[PD_REDC_NUM] =     "You need a red card to open this door";
text[PD_YELLOWC_NUM] =  "You need a yellow card to open this door";
// Boom need skull key, not key card
text[PD_BLUES_NUM] =    "You need a blue skull to open this door";
text[PD_REDS_NUM] =     "You need a red skull to open this door";
text[PD_YELLOWS_NUM] =  "You need a yellow skull to open this door";
// Boom multiple keys
text[PD_ANY_NUM] =      "Any key will open this door";
text[PD_ALL3_NUM] =     "You need all three keys to open this door";
text[PD_ALL6_NUM] =     "You need all six keys to open this door";

//
//      G_game.C
//
text[GGSAVED_NUM] =     "JEU SAUVEGARDE.";

//
//      HU_stuff.C
//
COPY_TO_TEXT( french_doom_text, HUSTR_E1M1_NUM );
COPY_TO_TEXT( french_doom2_text, HUSTR_1_NUM );
COPY_TO_TEXT( french_chat_text, HUSTR_CHATMACRO1_NUM );
text[HUSTR_MSGU_NUM] =		"[MESSAGE NON ENVOYE]";
text[HUSTR_MESSAGESENT_NUM] =   "[MESSAGE ENVOYE]";

//
//      AM_map.C
//

text[AMSTR_FOLLOWON_NUM] =      "MODE POURSUITE ON";
text[AMSTR_FOLLOWOFF_NUM] =     "MODE POURSUITE OFF";

text[AMSTR_GRIDON_NUM] =        "GRILLE ON";
text[AMSTR_GRIDOFF_NUM] =       "GRILLE OFF";

text[AMSTR_MARKEDSPOT_NUM] =    "REPERE MARQUE ";
text[AMSTR_MARKSCLEARED_NUM] =  "REPERES EFFACES ";

//
//      ST_stuff.C
//

text[STSTR_MUS_NUM] =    "CHANGEMENT DE MUSIQUE ";
text[STSTR_NOMUS_NUM] =  "IMPOSSIBLE SELECTION";
text[STSTR_DQDON_NUM] =  "INVULNERABILITE ON ";
text[STSTR_DQDOFF_NUM] = "INVULNERABILITE OFF";

text[STSTR_KFAADDED_NUM] = "ARMEMENT MAXIMUM! ";
text[STSTR_FAADDED_NUM] =  "ARMES (SAUF CLEFS) AJOUTEES";

text[STSTR_NCON_NUM] =   "BARRIERES ON";
text[STSTR_NCOFF_NUM] =  "BARRIERES OFF";

text[STSTR_BEHOLD_NUM] =  "inVuln, Str, Inviso, Rad, Allmap, or Lite-amp";
text[STSTR_BEHOLDX_NUM] = "AMELIORATION ACTIVEE";

text[STSTR_CHOPPERS_NUM] = "... DOESN'T SUCK - GM";
text[STSTR_CLEV_NUM] =     "CHANGEMENT DE NIVEAU...";

//
//      F_Finale.C
//
text[E1TEXT_NUM] = "APRES AVOIR VAINCU LES GROS MECHANTS\n"
"ET NETTOYE LA BASE LUNAIRE, VOUS AVEZ\n"
"GAGNE, NON? PAS VRAI? OU EST DONC VOTRE\n"
" RECOMPENSE ET VOTRE BILLET DE\n"
"RETOUR? QU'EST-QUE CA VEUT DIRE?CE"
"N'EST PAS LA FIN ESPEREE!\n"
"\n" 
"CA SENT LA VIANDE PUTREFIEE, MAIS\n"
"ON DIRAIT LA BASE DEIMOS. VOUS ETES\n"
"APPAREMMENT BLOQUE AUX PORTES DE L'ENFER.\n"
"LA SEULE ISSUE EST DE L'AUTRE COTE.\n"
"\n"
"POUR VIVRE LA SUITE DE DOOM, JOUEZ\n"
"A 'AUX PORTES DE L'ENFER' ET A\n"
"L'EPISODE SUIVANT, 'L'ENFER'!\n"
;

text[E2TEXT_NUM] = "VOUS AVEZ REUSSI. L'INFAME DEMON\n"
"QUI CONTROLAIT LA BASE LUNAIRE DE\n"
"DEIMOS EST MORT, ET VOUS AVEZ\n"
"TRIOMPHE! MAIS... OU ETES-VOUS?\n"
"VOUS GRIMPEZ JUSQU'AU BORD DE LA\n"
"LUNE ET VOUS DECOUVREZ L'ATROCE\n"
"VERITE.\n" 
"\n"
"DEIMOS EST AU-DESSUS DE L'ENFER!\n"
"VOUS SAVEZ QUE PERSONNE NE S'EN\n"
"EST JAMAIS ECHAPPE, MAIS CES FUMIERS\n"
"VONT REGRETTER DE VOUS AVOIR CONNU!\n"
"VOUS REDESCENDEZ RAPIDEMENT VERS\n"
"LA SURFACE DE L'ENFER.\n"
"\n" 
"VOICI MAINTENANT LE CHAPITRE FINAL DE\n"
"DOOM! -- L'ENFER."
;

text[E3TEXT_NUM] = "LE DEMON ARACHNEEN ET REPUGNANT\n"
"QUI A DIRIGE L'INVASION DES BASES\n"
"LUNAIRES ET SEME LA MORT VIENT DE SE\n"
"FAIRE PULVERISER UNE FOIS POUR TOUTES.\n"
"\n"
"UNE PORTE SECRETE S'OUVRE. VOUS ENTREZ.\n"
"VOUS AVEZ PROUVE QUE VOUS POUVIEZ\n"
"RESISTER AUX HORREURS DE L'ENFER.\n"
"IL SAIT ETRE BEAU JOUEUR, ET LORSQUE\n"
"VOUS SORTEZ, VOUS REVOYEZ LES VERTES\n"
"PRAIRIES DE LA TERRE, VOTRE PLANETE.\n"
"\n"
"VOUS VOUS DEMANDEZ CE QUI S'EST PASSE\n"
"SUR TERRE PENDANT QUE VOUS AVEZ\n"
"COMBATTU LE DEMON. HEUREUSEMENT,\n"
"AUCUN GERME DU MAL N'A FRANCHI\n"
"CETTE PORTE AVEC VOUS..."
;


// after level 6, put this:

text[C1TEXT_NUM] = "VOUS ETES AU PLUS PROFOND DE L'ASTROPORT\n" 
"INFESTE DE MONSTRES, MAIS QUELQUE CHOSE\n"
"NE VA PAS. ILS ONT APPORTE LEUR PROPRE\n"
"REALITE, ET LA TECHNOLOGIE DE L'ASTROPORT\n"
"EST AFFECTEE PAR LEUR PRESENCE.\n"
"\n"
"DEVANT VOUS, VOUS VOYEZ UN POSTE AVANCE\n"
"DE L'ENFER, UNE ZONE FORTIFIEE. SI VOUS\n"
"POUVEZ PASSER, VOUS POURREZ PENETRER AU\n"
"COEUR DE LA BASE HANTEE ET TROUVER \n"
"L'INTERRUPTEUR DE CONTROLE QUI GARDE LA \n"
"POPULATION DE LA TERRE EN OTAGE."
;

// After level 11, put this:

text[C2TEXT_NUM] = "VOUS AVEZ GAGNE! VOTRE VICTOIRE A PERMIS\n"
"A L'HUMANITE D'EVACUER LA TERRE ET \n"
"D'ECHAPPER AU CAUCHEMAR. VOUS ETES \n"
"MAINTENANT LE DERNIER HUMAIN A LA SURFACE \n"
"DE LA PLANETE. VOUS ETES ENTOURE DE \n"
"MUTANTS CANNIBALES, D'EXTRATERRESTRES \n"
"CARNIVORES ET D'ESPRITS DU MAL. VOUS \n"
"ATTENDEZ CALMEMENT LA MORT, HEUREUX \n"
"D'AVOIR PU SAUVER VOTRE RACE.\n"
"MAIS UN MESSAGE VOUS PARVIENT SOUDAIN\n"
"DE L'ESPACE: \"NOS CAPTEURS ONT LOCALISE\n"
"LA SOURCE DE L'INVASION EXTRATERRESTRE.\n"
"SI VOUS Y ALLEZ, VOUS POURREZ PEUT-ETRE\n"
"LES ARRETER. LEUR BASE EST SITUEE AU COEUR\n"
"DE VOTRE VILLE NATALE, PRES DE L'ASTROPORT.\n"
"VOUS VOUS RELEVEZ LENTEMENT ET PENIBLEMENT\n"
"ET VOUS REPARTEZ POUR LE FRONT."
;

// After level 20, put this:

text[C3TEXT_NUM] = "VOUS ETES AU COEUR DE LA CITE CORROMPUE,\n"
"ENTOURE PAR LES CADAVRES DE VOS ENNEMIS.\n"
"VOUS NE VOYEZ PAS COMMENT DETRUIRE LA PORTE\n"
"DES CREATURES DE CE COTE. VOUS SERREZ\n"
"LES DENTS ET PLONGEZ DANS L'OUVERTURE.\n"
"\n"
"IL DOIT Y AVOIR UN MOYEN DE LA FERMER\n"
"DE L'AUTRE COTE. VOUS ACCEPTEZ DE\n"
"TRAVERSER L'ENFER POUR LE FAIRE?"
;

// After level 29, put this:

text[C4TEXT_NUM] = "LE VISAGE HORRIBLE D'UN DEMON D'UNE\n"
"TAILLE INCROYABLE S'EFFONDRE DEVANT\n"
"VOUS LORSQUE VOUS TIREZ UNE SALVE DE\n"
"ROQUETTES DANS SON CERVEAU. LE MONSTRE\n"
"SE RATATINE, SES MEMBRES DECHIQUETES\n"
"SE REPANDANT SUR DES CENTAINES DE\n"
"KILOMETRES A LA SURFACE DE L'ENFER.\n"
"\n"
"VOUS AVEZ REUSSI. L'INVASION N'AURA.\n"
"PAS LIEU. LA TERRE EST SAUVEE. L'ENFER\n"
"EST ANEANTI. EN VOUS DEMANDANT OU IRONT\n"
"MAINTENANT LES DAMNES, VOUS ESSUYEZ\n"
"VOTRE FRONT COUVERT DE SUEUR ET REPARTEZ\n"
"VERS LA TERRE. SA RECONSTRUCTION SERA\n"
"BEAUCOUP PLUS DROLE QUE SA DESTRUCTION.\n"
;
   
// Before level 31, put this:

text[C5TEXT_NUM] = "FELICITATIONS! VOUS AVEZ TROUVE LE\n"
"NIVEAU SECRET! IL SEMBLE AVOIR ETE\n"
"CONSTRUIT PAR LES HUMAINS. VOUS VOUS\n"
"DEMANDEZ QUELS PEUVENT ETRE LES\n"
"HABITANTS DE CE COIN PERDU DE L'ENFER."
;

// Before level 32, put this:

text[C6TEXT_NUM] = "FELICITATIONS! VOUS AVEZ DECOUVERT\n"
"LE NIVEAU SUPER SECRET! VOUS FERIEZ\n"
"MIEUX DE FONCER DANS CELUI-LA!\n"
;

//
// Character cast strings F_FINALE.C
//
COPY_TO_TEXT( french_cast_text, CC_ZOMBIE_NUM );

#if 0
// Need to be translated
// Not in BEX
COPY_TO_TEXT( french_quitmsg_text, QUITMSG_NUM );
COPY_TO_TEXT( french_deathmsg_text, DEATHMSG_SUICIDE );
#endif
}



#if 0
// Heretic
// Needs to be translated
static char * french_heretic_text[] =
    // heretic strings
    "QUARTZ FLASK", // TXT_ARTIHEALTH
    "WINGS OF WRATH", // TXT_ARTIFLY
    "RING OF INVINCIBILITY", // TXT_ARTIINVULNERABILITY
    "TOME OF POWER", // TXT_ARTITOMEOFPOWER
    "SHADOWSPHERE", // TXT_ARTIINVISIBILITY
    "MORPH OVUM", // TXT_ARTIEGG
    "MYSTIC URN", // TXT_ARTISUPERHEALTH
    "TORCH", // TXT_ARTITORCH
    "TIME BOMB OF THE ANCIENTS", // TXT_ARTIFIREBOMB
    "CHAOS DEVICE", // TXT_ARTITELEPORT

    "WAND CRYSTAL", // TXT_AMMOGOLDWAND1
    "CRYSTAL GEODE", // TXT_AMMOGOLDWAND2
    "MACE SPHERES", // TXT_AMMOMACE1
    "PILE OF MACE SPHERES", // TXT_AMMOMACE2
    "ETHEREAL ARROWS", // TXT_AMMOCROSSBOW1
    "QUIVER OF ETHEREAL ARROWS", // TXT_AMMOCROSSBOW2
    "CLAW ORB", // TXT_AMMOBLASTER1
    "ENERGY ORB", // TXT_AMMOBLASTER2
    "LESSER RUNES", // TXT_AMMOSKULLROD1
    "GREATER RUNES", // TXT_AMMOSKULLROD2
    "FLAME ORB", // TXT_AMMOPHOENIXROD1
    "INFERNO ORB", // TXT_AMMOPHOENIXROD2

    "FIREMACE", // TXT_WPNMACE
    "ETHEREAL CROSSBOW", // TXT_WPNCROSSBOW
    "DRAGON CLAW", // TXT_WPNBLASTER
    "HELLSTAFF", // TXT_WPNSKULLROD
    "PHOENIX ROD", // TXT_WPNPHOENIXROD
    "GAUNTLETS OF THE NECROMANCER", // TXT_WPNGAUNTLETS

    "BAG OF HOLDING", // TXT_ITEMBAGOFHOLDING

    "GOD MODE ON", // TXT_CHEATGODON
    "GOD MODE OFF", // TXT_CHEATGODOFF
    "NO CLIPPING ON", // TXT_CHEATNOCLIPON
    "NO CLIPPING OFF", // TXT_CHEATNOCLIPOFF
    "ALL WEAPONS", // TXT_CHEATWEAPONS
    "FLIGHT ON", // TXT_CHEATFLIGHTON
    "FLIGHT OFF", // TXT_CHEATFLIGHTOFF
    "POWER ON", // TXT_CHEATPOWERON
    "POWER OFF", // TXT_CHEATPOWEROFF
    "FULL HEALTH", // TXT_CHEATHEALTH
    "ALL KEYS", // TXT_CHEATKEYS
    "SOUND DEBUG ON", // TXT_CHEATSOUNDON
    "SOUND DEBUG OFF", // TXT_CHEATSOUNDOFF
    "TICKER ON", // TXT_CHEATTICKERON
    "TICKER OFF", // TXT_CHEATTICKEROFF
    "CHOOSE AN ARTIFACT ( A - J )", // TXT_CHEATARTIFACTS1
    "HOW MANY ( 1 - 9 )", // TXT_CHEATARTIFACTS2
    "YOU GOT IT", // TXT_CHEATARTIFACTS3
    "BAD INPUT", // TXT_CHEATARTIFACTSFAIL
    "LEVEL WARP", // TXT_CHEATWARP
    "SCREENSHOT", // TXT_CHEATSCREENSHOT
    "CHICKEN ON", // TXT_CHEATCHICKENON
    "CHICKEN OFF", // TXT_CHEATCHICKENOFF
    "MASSACRE", // TXT_CHEATMASSACRE
    "TRYING TO CHEAT, EH?  NOW YOU DIE!", // TXT_CHEATIDDQD
    "CHEATER - YOU DON'T DESERVE WEAPONS", // TXT_CHEATIDKFA

    // EPISODE 1 - THE CITY OF THE DAMNED, HERETIC_E1M1 ...
    "E1M1:  THE DOCKS",
    "E1M2:  THE DUNGEONS",
    "E1M3:  THE GATEHOUSE",
    "E1M4:  THE GUARD TOWER",
    "E1M5:  THE CITADEL",
    "E1M6:  THE CATHEDRAL",
    "E1M7:  THE CRYPTS",
    "E1M8:  HELL'S MAW",
    "E1M9:  THE GRAVEYARD",
    // EPISODE 2 - HELL'S MAW
    "E2M1:  THE CRATER",
    "E2M2:  THE LAVA PITS",
    "E2M3:  THE RIVER OF FIRE",
    "E2M4:  THE ICE GROTTO",
    "E2M5:  THE CATACOMBS",
    "E2M6:  THE LABYRINTH",
    "E2M7:  THE GREAT HALL",
    "E2M8:  THE PORTALS OF CHAOS",
    "E2M9:  THE GLACIER",
    // EPISODE 3 - THE DOME OF D'SPARIL
    "E3M1:  THE STOREHOUSE",
    "E3M2:  THE CESSPOOL",
    "E3M3:  THE CONFLUENCE",
    "E3M4:  THE AZURE FORTRESS",
    "E3M5:  THE OPHIDIAN LAIR",
    "E3M6:  THE HALLS OF FEAR",
    "E3M7:  THE CHASM",
    "E3M8:  D'SPARIL'S KEEP",
    "E3M9:  THE AQUIFER",
    // EPISODE 4: THE OSSUARY
    "E4M1:  CATAFALQUE",
    "E4M2:  BLOCKHOUSE",
    "E4M3:  AMBULATORY",
    "E4M4:  SEPULCHER",
    "E4M5:  GREAT STAIR",
    "E4M6:  HALLS OF THE APOSTATE",
    "E4M7:  RAMPARTS OF PERDITION",
    "E4M8:  SHATTERED BRIDGE",
    "E4M9:  MAUSOLEUM",
    // EPISODE 5: THE STAGNANT DEMESNE
    "E5M1:  OCHRE CLIFFS",
    "E5M2:  RAPIDS",
    "E5M3:  QUAY",
    "E5M4:  COURTYARD",
    "E5M5:  HYDRATYR",
    "E5M6:  COLONNADE",
    "E5M7:  FOETID MANSE",
    "E5M8:  FIELD OF JUDGEMENT",
    "E5M9:  SKEIN OF D'SPARIL",

    // HERETIC_E1TEXT
    "with the destruction of the iron\n" "liches and their minions, the last\n" "of the undead are cleared from this\n" "plane of existence.\n\n" "those creatures had to come from\n"
        "somewhere, though, and you have the\n" "sneaky suspicion that the fiery\n" "portal of hell's maw opens onto\n" "their home dimension.\n\n" "to make sure that more undead\n"
        "(or even worse things) don't come\n" "through, you'll have to seal hell's\n" "maw from the other side. of course\n" "this means you may get stuck in a\n" "very unfriendly world, but no one\n"
        "ever said being a Heretic was easy!",

    // HERETIC_E2TEXT
    "the mighty maulotaurs have proved\n" "to be no match for you, and as\n" "their steaming corpses slide to the\n" "ground you feel a sense of grim\n" "satisfaction that they have been\n"
        "destroyed.\n\n" "the gateways which they guarded\n" "have opened, revealing what you\n" "hope is the way home. but as you\n" "step through, mocking laughter\n" "rings in your ears.\n\n"
        "was some other force controlling\n" "the maulotaurs? could there be even\n" "more horrific beings through this\n" "gate? the sweep of a crystal dome\n" "overhead where the sky should be is\n"
        "certainly not a good sign....",

    // HERETIC_E3TEXT
    "the death of d'sparil has loosed\n" "the magical bonds holding his\n" "creatures on this plane, their\n" "dying screams overwhelming his own\n" "cries of agony.\n\n"
        "your oath of vengeance fulfilled,\n" "you enter the portal to your own\n" "world, mere moments before the dome\n" "shatters into a million pieces.\n\n" "but if d'sparil's power is broken\n"
        "forever, why don't you feel safe?\n" "was it that last shout just before\n" "his death, the one that sounded\n" "like a curse? or a summoning? you\n" "can't really be sure, but it might\n"
        "just have been a scream.\n\n" "then again, what about the other\n" "serpent riders?",

    // HERETIC_E4TEXT
    "you thought you would return to your\n" "own world after d'sparil died, but\n" "his final act banished you to his\n" "own plane. here you entered the\n" "shattered remnants of lands\n"
        "conquered by d'sparil. you defeated\n" "the last guardians of these lands,\n" "but now you stand before the gates\n" "to d'sparil's stronghold. until this\n"
        "moment you had no doubts about your\n" "ability to face anything you might\n" "encounter, but beyond this portal\n" "lies the very heart of the evil\n" "which invaded your world. d'sparil\n"
        "might be dead, but the pit where he\n" "was spawned remains. now you must\n" "enter that pit in the hopes of\n" "finding a way out. and somewhere,\n" "in the darkest corner of d'sparil's\n"
        "demesne, his personal bodyguards\n" "await your arrival ...",

    // HERETIC_E5TEXT
    "as the final maulotaur bellows his\n" "death-agony, you realize that you\n" "have never come so close to your own\n" "destruction. not even the fight with\n"
        "d'sparil and his disciples had been\n" "this desperate. grimly you stare at\n" "the gates which open before you,\n" "wondering if they lead home, or if\n" "they open onto some undreamed-of\n"
        "horror. you find yourself wondering\n" "if you have the strength to go on,\n" "if nothing but death and pain await\n" "you. but what else can you do, if\n"
        "the will to fight is gone? can you\n" "force yourself to continue in the\n" "face of such despair? do you have\n" "the courage? you find, in the end,\n" "that it is not within you to\n"
        "surrender without a fight. eyes\n" "wide, you go to meet your fate.",
};
#endif

// Needs translation
void  french_heretic( void )
{
#if 0
    text[PD_BLUEK_NUM]   = "YOU NEED A BLUE KEY TO OPEN THIS DOOR";
    text[PD_YELLOWK_NUM] = "YOU NEED A YELLOW KEY TO OPEN THIS DOOR";
    text[PD_REDK_NUM]    = "YOU NEED A GREEN KEY TO OPEN THIS DOOR";
    text[GOTARMOR_NUM] = "SILVER SHIELD";
    text[GOTMEGA_NUM ] = "ENCHANTED SHIELD";
    text[GOTSTIM_NUM ] = "CRYSTAL VIAL";
    text[GOTMAP_NUM  ] = "MAP SCROLL";
    text[GOTBLUECARD_NUM] = "BLUE KEY";
    text[GOTYELWCARD_NUM] = "YELLOW KEY";
    text[GOTREDCARD_NUM ] = "GREEN KEY";
    COPY_TO_TEXT( french_heretic_text, TXT_ARTIHEALTH_NUM );
#endif
}


#if 0
// Needs translation
// HUSTR_E1M1_NUM, HUSTR_E1M5_NUM
static char * french_chexquest_levels[] = 
{
// HUSTR_E1M1_NUM .. HUSTR_E1M5_NUM
 "E1M1: Landing Zone",
 "E1M2: Storage Facility",
 "E1M3: Experimental Lab",
 "E1M4: Arboretum",
 "E1M5: Caverns of Bazoik"
};

//   DEATHMSG_SUICIDE, DEATHMSG_DEAD
static char * french_chexquest_deathmsg[] = 
{
//   DEATHMSG_SUICIDE .. DEATHMSG_DEAD
 "%s needs to be more careful\n", // DEATHMSG_SUICIDE
 "%s was standing in the wrong spot\n", // DEATHMSG_TELEFRAG
 "%s ate from the spoon of %s\n", // DEATHMSG_FIST
 "%s was zorched by %s\n", // DEATHMSG_GUN
 "%s took a large zorch from %s\n", // DEATHMSG_SHOTGUN
 "%s was rapidly zorched by %s\n", // DEATHMSG_MACHGUN
 "%s almost dodged %'s propulsor\n", // DEATHMSG_ROCKET
 "%s was hit by %'s propulsor\n", // DEATHMSG_GIBROCKET
 "%s was phased by %s\n", // DEATHMSG_PLASMA
 "%s couldn't escape %'s LAZ either\n", // DEATHMSG_BFGBALL
 "%s was fed from %'s bootspork\n", // DEATHMSG_CHAINSAW
 "%s was zorched by %s\n", // DEATHMSG_PLAYUNKNOW
 "%s can't swim in slime\n", // DEATHMSG_HELLSLIME
 "%s can't swim in slime\n", // DEATHMSG_NUKE
 "%s can't swim in slime\n", // DEATHMSG_SUPHELLSLIME
 "%s needs to be more careful\n", // DEATHMSG_SPECUNKNOW
 "%s was zorched by %s\n", // DEATHMSG_BARRELFRAG
 "%s needs to be more careful\n", // DEATHMSG_BARREL
 "A felmoid slimed %s\n", // DEATHMSG_POSSESSED
 "%s was slimed by a bipedicus\n", // DEATHMSG_SHOTGUY
 "%s was slimed by an armored bipedicus\n", // DEATHMSG_TROOP
 "%s was slimed by a cycloptis\n", // DEATHMSG_SERGEANT
 "%s fell victim to the flembrane\n", // DEATHMSG_BRUISER
 "%s was slimed\n", // DEATHMSG_DEAD
};

static char* french_chex_QUIT1 = "Don't give up now...do\nyou still wish to quit?";
static char* french_chex_QUIT2 = "please don't leave we\nneed your help!";

static char * french_chex_quitmsg[] =
{
// QUITMSG_NUM .. QUIT2MSG6_NUM
 french_chex_QUIT1;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;

 french_chex_QUIT1;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
 french_chex_QUIT2;
};
#endif


void  french_chexquest( void )
{
#if 0
  COPY_TO_TEXT( french_chexquest_levels, HUSTR_E1M1_NUM );
  COPY_TO_TEXT( french_chex_quitmsg, QUITMSG_NUM );
  COPY_TO_TEXT( french_chexquest_deathmsg, DEATHMSG_SUICIDE );

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
#endif
}

#endif // #ifdef FRENCH_INLINE


#if 0
// The following should NOT be changed unless it seems
// just AWFULLY necessary

#define HUSTR_PLRGREEN  "VERT: "
#define HUSTR_PLRINDIGO "INDIGO: "
#define HUSTR_PLRBROWN  "BRUN: "
#define HUSTR_PLRRED    "ROUGE: "

#define HUSTR_KEYGREEN  'g' // french key should be "V"
#define HUSTR_KEYINDIGO 'i' 
#define HUSTR_KEYBROWN  'b' 
#define HUSTR_KEYRED    'r'
#endif

// end of f_french.h
#endif
