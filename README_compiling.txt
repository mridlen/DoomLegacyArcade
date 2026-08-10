README for Doom Legacy Compiling from source.
2015-3-18
The latest Doom Legacy is 1.45.2.



** Compiling from source

To compile you will need a gcc compatible compiler and the compile packages
for any libraries that are needed.

If you are going to compile for SDL, this means installing the SDL development
package (which is more than the SDL run-time libraries).

Download the source package "DoomLegacy_1.45_source".
Create a working directory (such as DL145) which will contain the bin and obj
directories.
Unpack the source package into the working directory as a subdirectory.


** Installation

After compiling the binaries will be in the bin directory.

To install, move all the files in bin directory to the directory where you
will run Doom Legacy.  This can be a personal directory, or some system games
directory like /usr/local/games, or /usr/local/bin.

Do not try to execute directly from the bin directory.  The Doom Legacy
program is designed to execute from an executable directory.

The legacy.wad file from the common package will be needed.
It can be put in the executable directory, or it can be put in one of
the directories searched for wad files.

See the README_install.txt file for more information on installation.


** The Make method

Run the make file once to create the bin, obj, and dep directories, and a
blank file make_options file.

Edit the make_options file to reflect your operating system and SMIF selection.
The Makefile has a description of the options as comments.
Copy selected lines from the Makefile to make_options, and uncomment them
to make them active.

If you want to modify the Doom Legacy options, edit the doomdef.h file.
Uncommenting the #define line for features that you want included,
or commenting the #define lines for features that you do not want.
This can make the binary smaller, or if you really hate some features that
were added.  Most features can also be turned off at run-time.

Change directory to the source file directory.
If there has been a change in the Makefile options, then always clean the
old dep and obj directories first.
>> make clean

To compile.
>> make

The binary (and possibly some support files) will be generated in the bin
directory.

To install, move all the files in bin directory to the directory where you
will run Doom Legacy.  This can be a personal directory, or some system games
directory like /usr/local/games, or /usr/local/bin.

** The Make command line option

The make options can also be placed on the make command line, or a script
file can be created.
>> make SMIF=SDL HAVE_MIXER=1

The command line has precedence over options gotten from make_options.
This means that modifiers on the command line will work with those from
make_options.
The following will add the DEBUG option to those options selected by
make_options.
>> make DEBUG=1

This is most useful if you are a developer, must debug, or must compile
several versions such as SDL and X11.
When compiling for a single platform it is safest to setup make_options once
and always use that.


** Make options

This is from the Makefile.
Copy the line to make_options and remove the comment character '#' from the line.

# Most can just uncomment an appropriate selection for your system.
# WIN32=1
# WIN98=1
# WINNT=1
# OS2=1
# DOS=1
# CC_MINGW=1
# CC_WATCOM=1
# MAC=1
# Mac compile using Linux SDL includes (MacPorts, Fink)
# MAC_FRAMEWORK uses the native Mac SDL setup.
# MAC_FRAMEWORK=1

# SMIF - System Media Interface (default=SDL)
#SMIF=SDL
#SMIF=LINUX_X11
#SMIF=FREEBSD_X11
#SMIF=OS2_NATIVE
#SMIF=DJGPPDOS_NATIVE

# GGI option on X11
#X11_GGI=1

# SDL Mixer, to get music
#HAVE_MIXER=1

# CD-Music enable/disable, if you don't have the special libraries, etc.
#NOCDMUS=1
#ifndef NOCDMUS
  CDMUS=1
#endif
# Music options
#openserver5
# MUS_OS=SCOOS5
#unixware2
# MUS_OS=SCOUW2
#unixware7
# MUS_OS=SCOUW7
#ESD demon
# HAVE_ESD=1

# WIN_NATIVE enables
# For FMOD sound.  Can compile without it.
# HAVE_FMOD=1
# FMODINC="" alternative directory to find fmod.h and other includes
# FMODLIB="" alternative directory to find fmod libs
# DDINC="" alternative directory to find ddraw.h and other includes
# DDLIB="" alternative directory to find ddraw and other libs

# When SDL 1.3 is used, some stuff will break.  I don't know what yet.
# Reported that SDLMain is no longer required for Mac using SDL 1.3.
# SDL_1_3=1

# Developers with svn can enable this to have svn revision number in executable.
# Causes compile error message otherwise.
# Until can find test for presence of svn, this is best that can be done.
#SVN_ENABLE=1


An example make_options file for a Linux SDL compile.

MAKE_OPTIONS_PRESENT=1
SMIF=SDL
HAVE_MIXER=1
OPTLEV=-04
# DEBUG=1

An example make_options file for a Linux X11 compile.

MAKE_OPTIONS_PRESENT=1
SMIF=LINUX_X11
NOCDMUS=1
OPTLEV=-05
# DEBUG=1

An example make_options file for a Windows SDL compile.

MAKE_OPTIONS_PRESENT=1
WIN32=1
SMIF=SDL
HAVE_MIXER=1

An example make_options file for a Windows Direct-X compile.

MAKE_OPTIONS_PRESENT=1
WIN32=1
SMIF=WIN32_NATIVE


** The CMake method

To use cmake, cd to the working directory.
>> cmake doomlegacy

This will create a new Makefile at the working directory level.
I believe this only compiles for SDL.

<To be updated by smite-meister>


** Notes on specific OS

Linux:
GCC 3.3.6 and GCC 4.4 have been used.
Can compile for SDL, or X-windows native.
Compiling for X-windows native generates some additional binary support files
for sound, music, and OpenGL.
Doom Legacy is developed on Linux with SDL.

Win32:
Recommend Mingw32 gcc suite.
If you try to compile for Direct-X then get be warned that some header
packages will allow the code to compile, but will crash Doom Legacy upon
the first call to Direct-X because the header package does not describe
the interface correctly.


** Doomdef options

These are in the doomdef.h file.
Some of the Doom Legacy options.  There are more but you probably should not change those.
Remove the comment "//" on a #define to enable an option.  Add a "//" is disable a #define.

// =========================================================================
// Compile settings, configuration, tuning, and options

// Uncheck this to compile debugging code
#define PARANOIA                // test things that should never happen
#define LOGMESSAGES             // write message in log.txt (win32 and Linux only for the moment)
#define SHOW_DEBUG_MESSAGES

// [WDJ] Machine speed limitations.
// Leave undefined for netplay, or make sure all machines have same setting.
//#define MACHINE_MHZ  1500

#define SPLITSCREEN		// Two players on one monitor.
#define ABSOLUTEANGLE           // Vanilla Doom used relative angles in demo files.
#define FRAGGLESCRIPT           // FraggleScript in wad files.

// For Boom demo compatibility, spawns friction thinkers
#define FRICTIONTHINKER

// Select type of bob code
#define BOB_MOM

// [WDJ] Voodoo doll 4/30/2009
// A voodoo doll is an accident of having multiple start points for a player.
// It has been used in levels as a token to trip linedefs and create
// sequenced actions, and thus are required to play some wads, like FreeDoom.
#define VOODOO_DOLL

// [WDJ] Gives a menu item that allows adjusting the time a door waits open.
// A few of the timed doors in doom2 are near impossible to get thru in time,
// and I have to use cheats to get past that part of the game.
// This is for us old people don't have super-twitch fingers anymore, or don't
// want to repeat from save game 20 times to get past these bad spots.
#define DOORDELAY_CONTROL

// [WDJ] 6/22/2009  Generate gamma table using two settings,
// and a selected function.
#define GAMMA_FUNCS

// [WDJ] 3/25/2010  Savegame slots 0..99
#define SAVEGAME99
#define SAVEGAMEDIR

// [WDJ] 8/26/2011  recover DEH string memory
// Otherwise will just abandon replaced DEH/BEX strings.
// Enable if you are short on memory, or just like clean execution.
// Disable if it gives you trouble.
#define DEH_RECOVER_STRINGS

// [WDJ] 9/2/2011  French language controls
// Put french strings inline (from d_french.h)
// #define FRENCH_INLINE

// [WDJ] 9/2/2011  BEX language controls
// Load language BEX file
//#define BEX_LANGUAGE
// Automatic loading of lang.bex file.
//#define BEX_LANG_AUTO_LOAD

// [WDJ] 2/6/2012 Drawing enables
// To save code size, can turn off some drawing bpp that you cannot use.
#define ENABLE_DRAW15
#define ENABLE_DRAW16

// [WDJ] 6/5/2012 Boom global colormap
#define BOOM_GLOBAL_COLORMAP

// If IPX network code is to be included
// This may be overridden for some ports.
#define USE_IPX

// Set the initial window size (width).
// Expected sizes are 320x200, 640x480, 800x600, 1024x768.
#define INITIAL_WINDOW_WIDTH   800
#define INITIAL_WINDOW_HEIGHT  600

// [WDJ] Built-in Launcher
#define LAUNCHER

// Player morph canceling invisibility and MF_SHADOW, is inconsistent.
// The Heretic vanilla behavior cancels SHADOW when turned into a chicken.
// #define PLAYER_CHICKEN_KEEPS_SHADOW

// =========================================================================

// Name of local directory for config files and savegames
// These differ by operating system, Linux is shown for this example listing.
#define DEFAULTDIR1 ".doomlegacy"
#define DEFAULTDIR2 ".legacy"
#define DEFWADS1  "/usr/local/share/games/doomwads"
#define DEFWADS2  "/usr/local/games/doomwads"
#define DEFHOME   "/usr/local/games/legacyhome"
#define LEGACYWADDIR  "/usr/local/share/games/doomlegacy"

