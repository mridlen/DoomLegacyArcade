Title: DoomLegacy install for Linux, SDL.
Date: 2024-11-26

The latest Doom Legacy is 1.48.16

** Download files
You need two downloads.
The download names will vary according to version selected.

"DoomLegacy_1.48.16_Linux_i686_SDL1" contains the doomlegacy binary
 (compiled for x686 processors and SDL 1).
"DoomLegacy_1.48.16_common" contains legacy.wad and the docs.

The first is one of the precompiled binaries for those with Linux systems.
These have selected the default set of options.
Otherwise you will need to compile a binary for your operating system
(See the README_Compiling file in the docs).

Using SDL is recommended, but there are some other compile options
(like X11 native).

** What you need to run DoomLegacy.

1. DoomLegacy binary
2. Run-time libraries:  libdl
   For SDL1 binary, the Run-time libraries:  SDL 1.2, SDL_mixer 1.2  (see notes)
   For SDL2 binary, the Run-time libraries:  SDL 2.0, SDL_mixer 2.0
3. Optional libraries:  libzip, zlib
4. legacy.wad 1.48  (can also use legacy.wad 1.45, but it will complain)
5. Some IWAD (like Doom, Plutonia, TNT, Heretic)

Doom Legacy SDL version requires the SDL run-time and SDL-mixer run-time
libraries.  If you have a newer version of SDL libraries, then use them.
On Linux, SDL is usually installed to the system.

SDL1: You will need the SDL mixer version that the binary was compiled with,
or better, as SDL added features and we had to use them.
To play MP3 music, you must have a SDL1 mixer version with the MP3 option enabled.
Linux SDL1: SDL Mixer 1.2.10 or better (uses MIX_INIT).
Win 32 SDL1: SDL Mixer 1.2.8 or better (does not have MIX_INIT).
Linux SDL2: SDL Mixer 2.0 (usually has MP3 enabled, and includes FluidSynth)

The library libdl is a standard Linux library for accessing dynamic libraries.
It contains dlopen, which is used to detect and load libzip.

The library libzip is used to read zip archives.  It will be detected,
and loaded.

The library zlib is used to read compressed extended node lumps,
which some wads may use.

You will need a copy of legacy.wad version 1.48.
Some definitions for using corona in fragglescript were added.
The previous version legacy.wad 1.45 will also work.
Using -nocheckwadversion will stop the complaining about it being 1.45.

Please get the latest common and update your copy of the docs, which
also gets you the latest legacy.wad.
Most everything from installation, running, and options are already explained there.

** Play setup

Simple setup:
Create a directory for DoomLegacy and move your binary there.
Do not play DoomLegacy from your HOME directory.  DoomLegacy will refuse to
search the HOME directory for wads for reasons of security and speed.

This directory should have:
  Doom Legacy 1.48 binary
  legacy.wad
  dogs.wad  (optional)
A doomwad directory:
  One or more IWAD such as (Doom, Final Doom, UltDoom, Plutonia, TNT, Heretic,
  FreeDoom, etc.)

Doom Legacy will create a hidden directory in your home directory to keep your
config and game save files.  This home directory is ".doomlegacy", or
something similar as details vary by operating system.
If you compile your own Doom Legacy, then this directory name
is set by DEFAULTDIR1 and DEFAULTDIR2 of the doomdef.h file.
The alternative home directory is used if a user specific one cannot be made
(DEFHOME).

When you use a drawmode, and if that drawmode config file does not
exist, then the settings from the main config file will be used instead.
If the drawmode config file exists, then settings from that drawmode config file
will be displayed, and in force, while that drawmode is selected.

To create a drawmode config file, to hold video settings for that
drawmode, follow these steps.
1. Go to video options menu.
2. Press F3 (changes to drawmode config file editing)
3. Go to drawmode menu.
4. Select the drawmode.
5. Press 'C' to create an initial drawmode config file.
6. Goto video menu.
7. If there is a setting you want in the drawmode config file,
   go to it and press INSERT.
8. Set your default video mode.  Remember to press 'D' to set default.
9. Repeat for other drawmodes.
10. Press F1 to close config editing.

Doom Legacy will search for wads in predefined directories before it looks in
the current run directory.
To find legacy.wad, it also searches LEGACYWADDIR.
For the game IWAD and other wad files, it searches several directories.
These are defined by DEFWADS01 to DEFWADS20 in the doomdef.h file.
You can keep shared wads in one of the system-wide directories, and
personal wads in one of the user relative directories, and Doom Legacy
will find wads in those directories without having to specify the directory.
Doom Legacy will also search sub-directories, so you can organize your wads.


** Predefined Directories

In the DEFWADS strings, the ~ is the user home directory, as
implemented by Doom Legacy (Windows and Dos too).

Some of the defines used by the DoomLegacy binaries.

How deep it searches subdirectories.
GAME_SEARCH_DEPTH   4
IWAD_SEARCH_DEPTH   5

Linux, FreeBSD, and Unix:
The binary could also be installed in "/usr/local/bin".
DEFHOME    "legacyhome"
DEFAULTDIR1 ".doomlegacy"
DEFAULTDIR2 ".legacy"
LEGACYWADDIR  "/usr/local/share/games/doomlegacy"
DEFWADS01  "~/games/doomlegacy/wads"
DEFWADS02  "~/games/doomwads"
DEFWADS03  "~/games/doom"
DEFWADS04  "/usr/local/share/games/doomlegacy/wads"
DEFWADS05  "/usr/local/share/games/doomwads"
DEFWADS06  "/usr/local/share/games/doom"
DEFWADS07  "/usr/local/games/doomlegacy/wads"
DEFWADS08  "/usr/local/games/doomwads"
DEFWADS09  "/usr/share/games/doom"
DEFWADS10  "/usr/share/games/doomlegacy/wads"
DEFWADS11  "/usr/share/games/doomwads"
DEFWADS12  "/usr/games/doomlegacy/wads"
DEFWADS13  "/usr/games/doomwads"
DEFWADS16  "~/games/doomlegacy"
DEFWADS17  "/usr/local/share/games/doomlegacy"
DEFWADS18  "/usr/local/games/doomlegacy"
DEFWADS19  "/usr/share/games/doomlegacy"
DEFWADS20  "/usr/games/doomlegacy"


** Other than SDL
There are some options to compile a version of Doom Legacy for other display
systems.

Linux X11-windows native (tested, have binaries)
  - requires X11 (such as X11R6), the usual Linux window system that is
    included with every Linux package (only tiny Linux systems running
    standalone would be without this).

FreeBSD X11-windows native (tested by at least one user)
  - similar to Linux X11 but has some slight library differences.

NETBSD (tested by at least one user)
  - a few library differences

Linux GGI (old and not tested lately)
  - requires GGI libraries

Unixware, and Openserver5 versions (untested lately, usability is unknown)
  - has different music servers

Windows Direct-X native (may or may not work depending upon your header files)
  - requires Direct-X 7 (at least).
  - with or without FMOD

Mac SDL (code exists, is not working, needs a tester).

Macos native (old and not tested lately).

OS2 native (old and not tested lately).

DOS native (old, but was tested and updated recently).
  - requires Allegro
  - requires dos compiler


** Compiling from source

See the README_Compiling file.

