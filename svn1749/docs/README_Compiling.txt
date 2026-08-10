Title: Compiling Doom Legacy 1.48.12
Author: Wesley Johnson
Date: 2022-11-1

Chapter: Compile target

Choose the OS, port, and the graphics library:

  SDL:  (Linux, FreeBSD, Mac, Win)
    uses the SDL 1.2 graphic library, and sound

  SDL2: (Linux, FreeBSD, Mac, Win)
    uses the SDL2 graphic library, and sound

  X11: (Linux)
    uses the X11 graphic library

  win32: (Win)
    uses direct-draw graphic libraries, and sound

  DOS (djgppdos):  (DOS)
    direct access of video buffers.
    Allegro for sound

  macos: (Mac before OSX)
    uses the macos native graphic libraries

  OS2:  (OS2)
    uses the OS2 native graphic libraries

Choose the proper make_options file for your OS, and copy it to
make_options.

Edit make_options to set your OS dependent compile options.

Edit the doomdef.h file to select program compile options.


Chapter: Requirements

In order to compile Doom Legacy 1.48.12 you'll need to have the proper
libraries installed on your system for the selected target and selected options.
Make sure you install the developer packages of the libraries,
which include both the runtime libraries (DLLs) and
the stuff required during compilation (header files and import libraries).

NOTE: Most Linux distributions offer these libraries in the form of
precompiled packages.

SDL:
  Used by compile option SDL (all OS ports).
  Simple DirectMedia Layer. A multiplatform multimedia library.
  Version 1.2.10+
  "http://www.libsdl.org/download-1.2.php"
  Ubuntu: libsdl1.2-dev "http://packages.ubuntu.com/libsdl1.2-dev"

SDL Mixer:
  Used by compile option SDL, but usage can be disabled by HAVE_MIXER=0 in make_options.
  A multichannel mixer library for SDL.
  Version 1.2.7+
  Version 1.2.8+  will use RWOPS to play music.
  Version 1.2.10+ will use Mix_Init to detect devices.
  "http://www.libsdl.org/projects/SDL_mixer/"
  Ubuntu: libsdl-mixer1.2-dev  "http://packages.ubuntu.com/libsdl-mixer1.2-dev"

SDL2:
  Used by compile option SDL2 (all OS ports).
  Simple DirectMedia Layer Version 2. A multiplatform multimedia library.
  Version 2.0.0+

SDL2 mixer:
  Used by compile option SDL2, but usage can be disabled by HAVE_MIXER=0 in make_options.
  A multichannel mixer library for SDL2.
  Version 2.0.0+

OpenGL
  Used by OpenGL hardware render (enabled by HWRENDER) in SDL, SDL2, and X11 ports.
  The standard cross-platform graphics library, usually comes with the OS.
  There may be specific versions for specific video cards.
  OpenGL 1.3+

libzip
  Optional, for zip archive reading.  Linux Only (see Note1).
  This allows loading zipped wads directly.
  Enable zip archive reading by setting ZIPWAD in doomdef.h.
  
  The code will detect if you have libzip 1.2 or later, upon which it will
  use the zip_seek function, and will disable the local WZ_zip_seek function.
  
  When normally optioned (HAVE_LIBZIP=1), the libzip library must be present,
  or else the system will refuse to run the Doom Legacy binary.  This will work
  fine for the user compiling a binary just for themselves.

  When the dynamic loading (HAVE_LIBZIP=3) is enabled, then dlopen will
  be used to detect and load libzip. If libzip is not present then
  DoomLegacy will not be able to read zip archives.

zlib
  Optional, for compressed extended nodes reading. Linux Only (see Note1).
  These are only used by one extended node format, and as it is an option,
  is probably only present in a few very large wads.
  It is selected as a compile-time option using the make_options file.
  
  Set HAVE_ZLIB=1 in make_options file, which will link zlib and
  require it be present.
  This will work fine for the user compiling a binary just for themselves.
  Zlib is used commonly enough that it may already be installed for another program.
  
  Set HAVE_ZLIB=3 if you want dynamic zlib detection and loading using
  dlopen.  When zlib is not present then DoomLegacy will still be able to run,
  but will not be able to uncompress the extended node map of some wads.

sound devices
  The Linux X11 version of Doom Legacy has its own sound device selection
  mechanism.  It can select between several sound devices:
  OSS, ESD, ALSA, PulseAudio, and JACK, using the sound menu.
  In the make_options file you must select the sound
  devices that are to be included in the Doom Legacy code.
  
  When the normal option (=1) is selected, the device library must be
  present or else the system will refuse to run the Doom Legacy binary.
  
  When the dynamic loading (=3) option is selected, then dlopen will
  be used to detect and load the sound device library.

  The JACK option is untested, as enabling it got involved.
  OSS does not have a library, it will detect the OSS devices.

music devices
  The Linux X11 version of Doom Legacy has its own music device selection
  mechanism.  It can select between several music devices:
  MIDI, TiMidity, FluidSynth, external MIDI, FM_Synth, and AWE32_Synth,
  using the sound menu.
  In the make_options file you must select the sound
  devices that are to be included in the Doom Legacy code.

  When the normal option (=1) is selected, the device library must be
  present or else the system will refuse to run the Doom Legacy binary.

  When the dynamic loading (=3) option is selected, then dlopen will
  be used to detect and load the music device library.
  
  The SDL FluidSynth option is untested, as I could not get my installation to work.
  The SDL2 FluidSynth option is always selected by SDL2.
  The external MIDI option is untested, as I did not have such hardware.
  The last two Synth options depend on older specific sound cards.
  If you do not have such old sound cards, you should not include them.

dlopen
  Optional, for dynamic library detection and loading.  Linux Only (see Note1).
  Set HAVE_DLOPEN in make_options file.
  This is a standard Linux library.  It would only be missing on a
  stripped down Linux.

Note1:
The Linux Only options (dlopen, zlib, libzip) could be done under
Windows too.
This requires someone with a knowledge of Windows to finish the coding,
such as the library naming, and how to do dynamic library loading, and other details.
This needs to work for WindowsXP, Mingw32, and MSYS too.


Chapter: Programs

You will require the following programs during the build process:

Compiler:
  GCC 3.3+, the Gnu Compiler Collection which, among other things, is
  a free C/C++ compiler. "http://gcc.gnu.org/"
  Linux systems most likely already have it installed.
  
  Has been compiled on Linux with Gnu 5.5.0.
  Has been compiled on Linux with Clang 3.8.0
  
  Windows users can install MinGW, a GCC port.
  Windows users can install MSYS, which provides unix commands, and POSIX utilities for Win32.
  MinGW: "http://www.mingw.org/", "http://www.mingw.org/node/18"

  Has been compiled on Windows XP with Mingw-32 5.0.2, and MSYS 1.0.11.
    Use the command "mingw32-make".
    Using the default "make" command with MSYS invokes something else that does not work.


Download the DoomLegacy source.
  "http://sourceforge.net/projects/doomlegacy/"

You can either get the source package or, for the latest code snapshot.
Checkout the legacy_one/trunk/ directory from the Subversion repository at SourceForge:
Subversion: "http://subversion.apache.org/">Subversion
SourceForge: svn co https://doomlegacy.svn.sourceforge.net/svnroot/doomlegacy/legacy_one/trunk some_local_dir


From now on, your local copy of this directory will be referred to as
TRUNK, although you can name it anything you want.

You can have multiple versions of the 'src' directory, such as d01,
d02, d03, etc..

To compile these, cd to the src directory you wish to compile, and run
'make' from there.  The src Makefile will find the BUILD directory.


Chapter: Make Options

The make_options file controls the make process.

Edit it to select various compiling options.  Spelling of options must
be exactly one of the specified choices.


* Select the make_options_xx for your operating system, and copy it
to make_options.
Linux Example:
>> cp  make_options_nix  make_options

* The "make_options" file must be edited by you.  It is customized to your
operating system, available hardware, and your preferences.

The make_options_xx will contain appropriate selections for your
operating system.  Copying options from a make_options file for a different
operating system probably will not work.

* Lines that start with # are comments.  To turn off an option, put
a # at the beginning of that line. Lines without the # are active
selections.

* MAKE_OPTIONS_PRESENT: this informs 'make' that the make_options file has
been read. Leave it alone.

* OS: the operating system, such as LINUX, FREEBSD.

* SMIF: the port draw library, such as SDL, or X11.

* ARCH: the cpu architecture.
You can build a binary customized to your processor.

The examples work for most processors.
It does not need to be exact, as processors will run binaries compiled for any
older CPU versions of their line.

See your compiler docs for the -march, -mcpu, and -mtune command line
switches specifics. Without this line the compiler will select its
default build target, such a generic32, or i386.

For a modern x86 processor, selecting "ARCH=-march=i686" will compile
a binary for an x686, which will be smaller and faster than an x386
binary.  This binary will run on any x686 compatible processor.

If you have a specific processor, like an Athlon, then specifying
that will have the compiler use specific optimizations for the Athlon.
The resultant binary will run faster on an Athlon.  It may run on
other processors too, but in some cases this is not guaranteed.

* CD_MUSIC: if you don't have the music libraries, then turn off
music by setting CD_MUSIC=0.  The build will not have music.

* USEASM: use assembly code for certain operations.
This used to be slightly faster code, but modern compilers will now beat this.
The assembly is not updated often enough, so it may not be current.
Avoid this unless you like pain.

* There are several install options for DoomLegacy.  Each can be
customized.  See the next section.


Chapter: Install Options

The Makefile has options for installing the program.  This is
traditional for Linux makefiles.  It is espcially useful when
compiling your own binary.

You can use any other kind of installation too, such as copying the
bin files to a "run" directory.  Do not try to run DoomLegacy from the
compile environment, as the run setup is quite different.

You can have several doomlegacy versions in your run directory.
They may have any name you choose.  I suggest making a name that
identifies the variation, such as "doomlegacy_i686_athlon_v3".

You must have system permissions for system install.
An ordinary user can only install_user.

>> make install
  Provides install instructions.

The following are supported in "make_options":

>> make install_sys
  Install doomlegacy to the system.
  INSTALL_SYS_DIR: the system directory where the binary will be installed.
  INSTALL_SHARE_DIR: the system directory where the support files will be installed.
  
>> make install_games
  Install doomlegacy to the system games.
  INSTALL_GAMES_DIR: the system directory where the binary will be installed.
  INSTALL_SHARE_DIR: the system directory where the support files will be installed.

>> make install_user
  Install doomlegacy to a user directory.
  INSTALL_USER_DIR: the user directory where the binary and suport files will be installed.
  INSTALL_LEGACYWAD_DIR: the directory where the support files will be installed.


Chapter: Compiling Legacy

1. Open a shell window, like console. Go to the TRUNK directory.

2. If you want a separate BUILD directory, then create it.
Create bin, dep, objs directories there.
The BUILD directory will need its own make_options file (see step 4).
This allows you to compile SDL in one directory, and X11 in another.
>> make BUILD=x11 dirs

3. The top level TRUNK is the default BUILD directory.
The make_options file there will control the build, when BUILD is not
specified.

4. Select and configure your make_options file.
Copy one of the make_options_xx files to make_options of your BUILD
directory.  Edit your make_options to set your configure options.
>> cp  make_options_nix  x11/make_options

5. Edit compile options in doomdef.h.
These are options within DoomLegacy.
Some features can be disabled to make a smaller binary.
There are also some experimental code options, that are kept disabled.

6. To clean the build:
When you have been copying files, or have modified make_options,
you need to start from a clean build.  This removes old object files.
>> make clean

With a BUILD directory:
>> make BUILD=x11 clean

7. To build the executable:
>> make

To build with bin, obj, dep, in a separate directory (example x11):
>> make BUILD=x11

8. To build a debug version:
This puts the debugging symbols for the debugger in the binary.
The debug version also forces DoomLegacy video to an 800x600 window,
so the user can switch between the debugger window and the DoomLegacy window.
>> make DEBUG=1

With a BUILD directory:
>> make BUILD=x11 DEBUG=1


10. Example: X11 DEBUG
>> make BUILD=x11d clean
>> make BUILD=x11d DEBUG=1 MIXER=1

Debugging with X11 can be difficult.
Keep your game window and debugging window separated.
Be prepared to switch to another console and kill the game.
Print information to a log file rather than try to look at it
with a debugger.

11. Example: MinGW
Must use mingw32-make, not the make command from MSYS.
>> mingw32-make DEBUG=1 clean

12. Read the Makefile for the help at the top of the file.
This will document the latest commands.

13. Example: SDL2

Make a build dir for SDL2
> mkdir sdl2
> mkdir sdl2/bin sdl2/dep sdl2/objs
> copy make_options  sdl2/make_options

Edit sdl2/make_options
  SDL2=1
On windows with mingw32
  If you do not have sdl2-config then tell it whereever SDL2 installed the include and lib.
  SDL_DIR=C:SDL2/

Compile.
make BUILD=sdl2  clean
make BUILD=sdl2

Binary will be in sdl2/bin.

See docs/source.html for more details.

