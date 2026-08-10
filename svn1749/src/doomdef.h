// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: doomdef.h 1748 2025-04-25 08:05:15Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2024 by DooM Legacy Team.
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
//      Defines to control options in the code, tune, and select debugging.
//
//-----------------------------------------------------------------------------

#ifndef DOOMDEF_H
#define DOOMDEF_H

// Version number: major.minor.revision
#define DL_VER_MAJ  1
#define DL_VER_MIN  48
#define DL_VER_REV  18

// Have some experimental code that is not to be enabled for release code.
// Set to current version for testing.  Otherwise keep it disabled.

// Heretic upgrades defered to version 1.49
// #define HERETIC_TODO_VERSION  149


// Versioning
#ifndef SVN_REV
#define SVN_REV "1749"
#endif

// =========================================================================
// Compile settings, configuration, tuning, and options

#ifdef DEBUG_WINDOWED
// Uncheck this to compile debugging code
#define RANGECHECK
#define RANGECHECK_DRAW_LIMITS
// Do some extra tests, that never happens but maybe.
#define PARANOIA
// Write message in log.txt (win32 and Linux only for the moment)
#define LOGMESSAGES
// Show debug messages.
#define DEBUG_MESSAGES_ON
#else
// Normal compile
// Uncheck this to compile debugging code
//#define RANGECHECK
// Do some extra tests, that never happens but maybe.
//#define PARANOIA
// write message in log.txt (win32 and Linux only for the moment)
//#define LOGMESSAGES
// Default to show debug messages.
//#define DEBUG_MESSAGES_ON
#endif

#define LOGLINELEN  80


// [WDJ] Machine speed limitations.
// Leave undefined for netplay, or make sure all machines have same setting.
//#define MACHINE_MHZ  1500

// some tests, enable or disable it
//#define HORIZONTALDRAW        // abandoned : too slow
//#define TILTVIEW              // not finished
//#define PERSPCORRECT          // not finished
#define SPLITSCREEN
//#define CLIENTPREDICTION2     // differant methode
#define NEWLIGHT                // compute lighting with bsp (in construction)
#define FRAGGLESCRIPT           // SoM: Activate FraggleScript

// For Boom demo compatibility, spawns friction thinkers
#define FRICTIONTHINKER

// [WDJ] Voodoo doll 4/30/2009
// A voodoo doll is an accident of having multiple start points for a player.
// It has been used in levels as a token to trip linedefs and create
// sequenced actions, and thus are required to play some wads, like FreeDoom.
// Voodoo doll code is selectable, now a standard feature, 12/15/2015.

// [WDJ] Gives a menu item that allows adjusting the time a door waits open.
// A few of the timed doors in doom2 are near impossible to get thru in time,
// and I have to use cheats to get past that part of the game.
// This is for us old people don't have super-twitch fingers anymore, or don't
// want to repeat from save game 20 times to get past these bad spots.
#define DOORDELAY_CONTROL
  // See p_fab.c, giving it NETVAR status causes saved games to crash program.

// [WDJ] 6/22/2009  Generate gamma table using two settings,
// and a selected function.
// Gamma funcs are selectable, now a standard feature, 12/15/2015.

// [WDJ] 3/25/2010  Savegame slots 0..99
#define SAVEGAME99
#define SAVEGAMEDIR

// [WDJ] 8/26/2011  recover DEH string memory
// Otherwise will just abandon replaced DEH/BEX strings.
// Enable if you are short on memory, or just like clean execution.
// Disable if it gives you trouble.
#define DEH_RECOVER_STRINGS

#if defined PCDOS && ! defined DEH_RECOVER_STRINGS
#define DEH_RECOVER_STRINGS
#endif

// [WDJ] 9/5/2011
// Enable to allow BEX to change SAVEGAMENAME
// This is a security risk, trojan wads could use it to corrupt arbitrary files.
//#define BEX_SAVEGAMENAME

// [WDJ] 9/2/2011  French language controls
// Put french strings inline (from d_french.h)
// #define FRENCH_INLINE

#ifdef FRENCH
#define FRENCH_INLINE
#endif

// [WDJ] 9/2/2011  BEX language controls
// Load language BEX file
//#define BEX_LANGUAGE
// Automatic loading of lang.bex file.
//#define BEX_LANG_AUTO_LOAD


// [WDJ] 2/6/2012 Drawing enables
// To save code size, can turn off some drawing bpp that you cannot use.
#define ENABLE_DRAW15
#define ENABLE_DRAW16
#ifndef SMIF_PC_DOS
# define ENABLE_DRAW24
# define ENABLE_DRAW32
#endif

#define ENABLE_DRAW_ALPHA

// Used for palette draw fade operations
#define ENABLE_DRAW8_USING_12

// [WDJ] 6/5/2012 Boom global colormap
// Boom global colormap is selectable, now a standard feature, 12/15/2015.


// If IPX network code is to be included
// This may be overridden for some ports.
#define USE_IPX

// Voodoo video card support (old, from 1990's)
// Only supports Voodoo 1 and Voodoo 2 card.
// Glide is the XOrg X11 driver for Voodoo cards,
// but the driver is also available as a dll for the win32 port.
//#define USE_VOODOO_GLIDE

// Set the initial window size (width).
// Expected sizes are 320x200, 640x480, 800x600, 1024x768.
#define INITIAL_WINDOW_WIDTH   800
#define INITIAL_WINDOW_HEIGHT  600

// Enable alternative fit to view width and height.
#define FIT_RATIO

// [WDJ] Built-in Launcher
#define LAUNCHER
// especially for Window 7,8
#if defined( WIN32 ) && !defined( LAUNCHER )
#define LAUNCHER
#endif

// [WDJ] Multiple config file, main and drawmode, with cvar tracking.
// This enables editing them in the menus.
#define CONFIG_MENU_PAGE

// [WDJ] 7/6/2017  MBF DOGS
#define DOGS

// [WDJ] 8/31/2017  16 rotation sprites option
#define ROT16

// If surround sound is desired
#define SURROUND_SOUND

#if defined(SMIF_SDL) || defined(SMIF_WIN_NATIVE)
// Enable MP3 music
#define MUSIC_MP3

// Enable OGG music
#define MUSIC_OGG

#if defined( MUSIC_MP3 ) || defined( MUSIC_OGG )   
// Select the music lump, provides access to o_xxx music lumps.
#define MUSIC_SOURCE_CONTROL

// If music source has selected MP3 or OGG,
// and such are not found or cannot be played, then be silent.
// Otherwise will play the default MUS or MIDI, like in AUTO.
#define MUSIC_SELECT_ALT_IS_SILENCE
#endif
#endif

// Ability to disable joystick.
#define JOYSTICK_SUPPORT

// Coronas in dynlights
#define DYLT_CORONAS
// Coronas drawn with sprite draw
#define SPDR_CORONAS

#if ( defined(DYLT_CORONAS) && defined(SPDR_CORONAS) )
#define CORONA_CHOICE
#endif

// XBOX controller
#define XBOX_CONTROLLER

// Second mouse
#define MOUSE2

// Allow players to wait for game start.
#define WAIT_GAME_START_INTERMISSION

#ifdef LINUX
// For now Linux only because it requires libzip.
#ifdef HAVE_LIBZIP
// Read zip wads.
// This requires  HAVE_LIBZIP  be set in make_options to get linking.
#define ZIPWAD
#endif
#endif

// DeePsea tall patches.
// Allows patches that exceed 254 height.
// Not used by many wads, but not much extra code either.  Relatively safe.
#define DEEPSEA_TALL_PATCH

// Extended Nodes
// Reads wads that use extended wads format, such as from DeePBSP.
#define DEEPSEA_EXTENDED_NODES

// Generate blockmap option and controls.
#define GENERATE_BLOCKMAP

// Generate options to vary map monsters and objects from the normal attributes.
#define MAPTHING_ADJUST

// Tall, short, heavy, thin monsters.
#define MONSTER_VARY

// Teleport monster control
#define ENABLE_TELE_CONTROL

// Slow reaction from monsters
#define ENABLE_SLOW_REACT

// Enable Umapinfo code
#define ENABLE_UMAPINFO

// MBF21 Dehacked extensions
#define MBF21

// DEHEXTRA Dehacked extension
#define ENABLE_DEHEXTRA
// [WDJ] As wanted by DSDA, much of it guessing.
// DEHEXTRA states from ??? to 3999 (never defined in DSDA)
// DEHEXTRA sprites from 145 to 249
// DEHEXTRA things from 150 to 249
// DEHEXTRA sounds from 259 to 699.
//#define DEHEXTRA_MAX_STATES   3999
//#define DEHEXTRA_MAX_SPRITES  244
//#define DEHEXTRA_MAX_THINGS   249
//#define DEHEXTRA_MAX_SOUNDS   699
// [WDJ] What will play most any wad we care about.
// Actual limits may be higher due to other uses.
// Extra states are those > 2164, must be greater to get more states.
#define DEHEXTRA_MAX_STATES   2600
#define DEHEXTRA_MAX_SPRITES  244
#define DEHEXTRA_MAX_THINGS   249
#define DEHEXTRA_MAX_SOUNDS   500

// [WDJ] Because the wad does not actually know where
// the free slots are at, some use extravagant indexes.
// This puts those into available free slots, and changes all references.
// Only need this if are going to play large wads, that use DEHEXTRA
// or an excessive number of sprites or states.
// This remaps them to the known free space.
#define ENABLE_DEH_REMAP


// Get tired when run too much. Also can drown.
#define ENABLE_TIRED_RUN

// Command dogs and bots to "come here".
#define ENABLE_COME_HERE


// Player morph canceling invisibility and MF_SHADOW, is inconsistent.
// The Heretic vanilla behavior cancels SHADOW when turned into a chicken.
// #define PLAYER_CHICKEN_KEEPS_SHADOW

// Special handling for devparm is somewhat dated.
// Most users do not ever use it.
// #define DEVPARM_LOADING

// File reload at level start when filename starts with "~"
// A fragile development hack that has been mostly disabled for years.
// ** Broken.
// #define WADFILE_RELOAD

// Loading disk icon.
// Disk accesses are too fast for this to be useful anymore.
// ** Broken
// #define LOADING_DISK_ICON



// =========================================================================

// File where all game vars and settings are saved
#define CONFIGFILENAME  "config.cfg"
#define DRAWMODE_CONFIGFILENAME  "config%s.cfg"

// Name of local directory for config files and savegames
// In userhome
#ifdef LINUX
#define DEFAULTDIR1 ".doomlegacy"
#define DEFAULTDIR2 ".legacy"
#endif
#ifdef __APPLE__
// The MAC place for application data, and where some other ports put theirs.
#define DEFAULTDIR1 "/Library/Application Support/doomlegacy"
// Doomlegacy traditional, where there may be an existing directory.
#define DEFAULTDIR2 ".doomlegacy"
#endif
#ifdef SMIF_PC_DOS
#define DEFAULTDIR1 "dmlegacy"
#define DEFAULTDIR2 "legacy"
#endif
#ifndef DEFAULTDIR1
#define DEFAULTDIR1 "doomlegacy"
#endif
#ifndef DEFAULTDIR2
#define DEFAULTDIR2 "legacy"
#endif

#if defined SMIF_PC_DOS || defined WIN32 || defined OS2
// HOME is ~
// Allowed DEFWADS01..DEFWADS21
#if 1
#define DEFWADS01  "~\\games\\doom"
#define DEFWADS02  "~\\games\\doomwads"
#define DEFWADS03  "~\\games\\doomlegacy\\wads"
#else
#define DEFWADS01  "games\\doom"
#define DEFWADS02  "games\\doomwads"
#define DEFWADS03  "games\\doomlegacy\\wads"
#endif
#define DEFWADS04  "\\doomwads"
#define DEFWADS05  "\\games\\doomwads"
#define DEFWADS06  "\\games\\doom"
//#define DEFWADS07  "\\games\\doom"
//#define DEFWADS08  "\\games\\doom"
#if defined WIN32 || defined OS2
#define DEFWADS10  "\\Program Files\\doomlegacy\\wads"
#endif

// When cannot find user $(HOME), make a home in the executable dir.
#ifdef SMIF_PC_DOS
#define DEFHOME    "DL_HOME"
#else
#define DEFHOME    "legacyhome"
#endif
//#define LEGACYWADDIR  ""

#else
// Linux, unix, FreeBSD, Mac
// HOME is ~
// Allowed DEFWADS01..DEFWADS21
#define DEFWADS01  "~/games/doomlegacy/wads"
#define DEFWADS02  "~/games/doomwads"
#define DEFWADS03  "~/games/doom"

#define DEFWADS04  "/usr/local/share/games/doomlegacy/wads"
#define DEFWADS05  "/usr/local/share/games/doomwads"
#define DEFWADS06  "/usr/local/share/games/doom"
  // Used by Chocolate doom

#define DEFWADS07  "/usr/local/games/doomlegacy/wads"
#define DEFWADS08  "/usr/local/games/doomwads"
#define DEFWADS09  "/usr/share/games/doom"
  // Used by Chocolate doom

#define DEFWADS10  "/usr/share/games/doomlegacy/wads"
#define DEFWADS11  "/usr/share/games/doomwads"
#define DEFWADS12  "/usr/games/doomlegacy/wads"
#define DEFWADS13  "/usr/games/doomwads"

#define DEFWADS16  "~/games/doomlegacy"
#define DEFWADS17  "/usr/local/share/games/doomlegacy"
#define DEFWADS18  "/usr/local/games/doomlegacy"
#define DEFWADS19  "/usr/share/games/doomlegacy"
#define DEFWADS20  "/usr/games/doomlegacy"

// Linux, When cannot find user $(HOME), make a home in the executable dir.
#define DEFHOME    "legacyhome"

#if defined(__APPLE__) && defined(__MACH__)
// Use defined Mac resources (app folder)
#define MAC_RESOURCES_DIR

#undef DEFWADS16
#define DEFWADS16  "~/Library/Application Support/doomlegacy"

// Legacy wad for Mac
//#define  LEGACYWADDIR  ".app"
#if 1
#define  LEGACYWADDIR  "~/Library/Application Support/doomlegacy"
#else
#define  LEGACYWADDIR  "/usr/local/share/games/doomlegacy"
#endif

#else
// Linux, FreeBSD
#define  LEGACYWADDIR  "/usr/local/share/games/doomlegacy"

#endif

#endif

// How many subdirectories deep to search.
#define  GAME_SEARCH_DEPTH   4
#define  IWAD_SEARCH_DEPTH   5

// =========================================================================

// The maximum number of players, multiplayer/networking.
// NOTE: it needs more than this to increase the number of players...

// Limit MAXPLAYERS (and others) to 250.
// TODO: ... more!!!
#define MAXPLAYERS              32
#define MAXSKINS                128
#define PLAYERSMASK             (MAXPLAYERS-1)
#define MAXPLAYERNAME           21
// Limit MAXTEAMS to 250.
#define MAXTEAMS		32

// Determined by skin color tables
#define NUMSKINCOLORS           11

#define SAVESTRINGSIZE          24

// Used for many file path buffer sizes
#ifdef SMIF_PC_DOS
#define MAX_WADPATH   128
#else
// was too short for network systems
#define MAX_WADPATH   256
#endif

// =========================================================================

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define OLDTICRATE       35
// Set 1 for standard, try 4 for 140 fps :)
#define NEWTICRATERATIO   1
#define TICRATE         (OLDTICRATE*NEWTICRATERATIO)


#endif  // DOOMDEF_H
