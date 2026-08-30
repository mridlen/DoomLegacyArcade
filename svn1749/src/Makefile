#############################################################################
#
# $Id: Makefile 1641 2022-11-21 00:00:00Z wesleyjohnson $
# GNU Make makefile for Doom Legacy Program
#
# Copyright (C) 1998-2022 by Doom Legacy Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# NOTICE: There is a CMAKE file included that can handle the default
# compilation for SDL.  You can try that first, instead of this more
# complicated Make that has all the OS, other SMIF, and more options.
#  
#
# On first execution of this Makefile, it will create a make_options file
# and the build dirs.
#
# IMPORTANT: Edit the make_options file, choose OS and SMIF, and
# put ALL your other options in there too.
# This is the easiest way to use this Makefile, with the least mistakes.
# Otherwise you must include the OS, and maybe the SMIF,
# with almost EVERY make command.
#   >> make clean OS=DOS
#
# The make_options file can be edited to select OS, SMIF, and options.
# OR you can submit OS, SMIF, and options on the make command line every time.
# OR you can edit make_options, but override it in special cases with
# make command line options (like DEBUG=1 or HAVE_MIXER=1).
#
# If you already have a make_options file before first compilation, then
# you may also have to create build directories.
# The OS is set in the make_options file, so it is not needed on the command.
#   >> make dirs
#
# An example with options on the make command:
#   >> make OS=OS2 SMIF=SDL HAVE_MIXER=1
#
# The preferred system media interface is SDL.
# To compile the generic Linux SDL version (with sound):
#    >> make OS=LINUX HAVE_MIXER=1
#
# Other target options:
# To compile the Linux X11 version (OS=LINUX will be assumed)
#   >> make SMIF=LINUX_X11
# To compile the FreeBSD X11 version (OS=FREEBSD will be assumed)
#   >> make SMIF=FREEBSD_X11
# To compile the OS/2 native version (OS=OS2 will be assumed)
#   >> make SMIF=OS2_NATIVE
#
# To compile with assembly code for some draw functions, specify USEASM
#   >> make OS=LINUX SMIF=SDL USEASM=1
#   but the assembly code is not often updated with the latest improvements.
#
# To generate a debugging executable, specify DEBUG.
#   >> make OS=LINUX SMIF=SDL DEBUG=1
#
# Command line options have precedence over the make_options file.
# To have values in the make_options file override those on the make
# command line, use override in the make_options file:
#   override SMIF=SDL
#
# To use another make_options file, invoke make with MAKE_OPTIONS.
#   >> make MAKE_OPTIONS=my_other_options
#
# This uses GNU Make special commands.  It may work with other make programs,
# and has work-arounds in some places.  It has been tested with mingw32-make.
# If you have problems, please use GNU Make, or gmake.
#
# If you have a workaround for another make, then submit it as a bug fix.
# This makefile must work with Linux, BSD, Win, OS2, DOS, Mac, so Linux specific
# operations must be guarded and limited.  Generic solutions are preferred.
#
# To update dependency then "make depend", otherwise it is automatic.
#   >> make depend OS=LINUX
#
# commands:
#  all  :  compile executable (default)
#  dirs : create dep, objs, bin directories
#  clean : delete objs and bin
#  distclean : delete objs, bin, and make_options
#  depend : update dependencies
#  install: install binaries to INSTALL_DIR
#  uninstall: install binaries from INSTALL_DIR
# maintenance commands (generally not usable to the end user):
#  legacywad : legacy.wad missing message
#  updatewad : update existing legacy.wad
# developers commands:
#  disasm  : disasm of the exe
#  objdump_draw : dump draw functions to assembly
#  dll  : generate dll for X11
#############################################################################

# Debug enables, save on reinventing this everytime.
#DEBUG=1
#PROFILEMODE=1

# Compile settings.
# These MUST be setup by the user before compiling, or the wrong code
# will be generated.
# Do a "make clean" after any Makefile or make_options changes.
# Edit these into the make_options file (without the #),
# or as a parameter on the Make command line, SMIF=LINUX_X11.

# Select one of these operating systems.
# Linux is the default for SDL, that also applies to most unix-like OS.
#   OS=LINUX
#   OS=FREEBSD
# Windows operating systems.  Also see CC_SELECT.
# WIN32 is a generic 32bit Windows.
#   OS=WIN32
# WIN7 is a generic for Win7/Win8.
# Win7/Win8 have caused problems, and any solutions found will be enabled
# by this OS selection.
# This does  NOT YET  make a version customized to Win7/8.
#   OS=WIN7
# Some older Windows, for completeness.
#   OS=WIN98
# An OS2 port is present, but has not been tried in ages.
#   OS=OS2
# A DOS port, (DJGPPDOS).  Needs old libraries.
#   OS=DOS
# MacIntosh operating system.  Also see Mac options.
# Mac/SDL is currently under re-development.
# The MACOS port is old, and has not been tried in ages.
#   OS=MAC

# SMIF - System Media Interface
# Select one of the following.
# The SDL library. SDL is the default.
#   SMIF=SDL
# The X11 window system native calls.  See some X11 options later.
#   SMIF=LINUX_X11
#   SMIF=FREEBSD_X11
# Windows Direct Draw native calls.  See some Direct Draw options later.
#   SMIF=WIN32_NATIVE
#   SMIF=OS2_NATIVE
#   SMIF=DJGPPDOS_NATIVE

# Select a Compiler.  GCC is the default compiler.
# Enable one of these to use an alternative compiler.
#   CC_SELECT=GCC
# The Makefile will use some alternative compiling switches known to be needed
# for these compilers.
#   CC_SELECT=MINGW
#   CC_SELECT=WATCOM
#   CC_SELECT=CLANG
# Enable CC_ENVIRONMENT to allow an environment CC to specify the compiler.
# This does work as expected if it is not gcc command line equivalent.
#   CC_ENVIRONMENT=1
# Use CC_EXPLICIT_CMD to explicitly specify a compiler command.
# This will be used to set CC.
#   CC_EXPLICIT_CMD=clang

# SDL Mixer, to get music.
#   HAVE_MIXER=1

# Music options
# CD-Music enable/disable, if you don't have the special libraries, etc.
#   CD_MUSIC=0
ifdef CD_MUSIC
  ifeq ("$(CD_MUSIC)","1)
    CDMUS=1
  endif
else
  # default
  CDMUS=1
endif
MUS_OS_LIST= SCOOS5 SCOUW2 SCOUW7
# openserver5
#   MUS_OS=SCOOS5
# unixware2
#   MUS_OS=SCOUW2
# unixware7
#   MUS_OS=SCOUW7
# ESD demon for X11 (esound), not needed for SDL.
#   HAVE_ESD=1
#   HAVE_ESD=3 (detect by dlopen)
# OSS sound for X11
#   HAVE_OSS=1
#   HAVE_OSS=3 (OSS does not have dynamic libraries)
# ALSA sound for X11
#   HAVE_ALSA=1
#   HAVE_ALSA=3 (detect by dlopen)
# Jack sound for X11
#   HAVE_JACK=1
#   HAVE_JACK=3 (detect by dlopen)
# PulseAudio sound for X11
#   HAVE_PULSE=1
#   HAVE_PULSE=3 (detect by dlopen)

# GGI option on X11
#   X11_GGI=1
# Uncomment if you want to use the POLL_POINTER hack in X11
#   POLL_POINTER=-DPOLL_POINTER

# WIN_NATIVE options
# For FMOD sound.  Can compile without it.
# The code that uses FMOD seems incomplete, consider it a work in progress.
# HAVE_FMOD=1
# FMODINC="" alternative directory to find fmod.h and other includes
# FMODLIB="" alternative directory to find fmod libs
# DDINC="" alternative directory to find ddraw.h and other includes
# DDLIB="" alternative directory to find ddraw and other libs

# Select MacIntosh operating system.  It has some unique options.
# Mac compile using Linux SDL includes (MacPorts, Fink)
# MAC_FRAMEWORK uses the native Mac SDL setup.
# MAC_FRAMEWORK=1

# When SDL 1.3 is used, some stuff will break.  I don't know what yet.
# Reported that SDLMain is no longer required for Mac using SDL 1.3.
# SDL_1_3=1

# UMAPINFO directory is present
# UMAPINFO_DIR=1

# Warning flags
WFLAGS=-Wall
#WFLAGS=-Wall -Wwrite-strings

# Optimization level, -O0 to -O5
OPTLEV=-O3
#OPTLEV=-O5

#Enable the x86 asm code (which is not always up to date).
# This is only useful if have an old compiler with bad optimization.
#USEASM=1

# Developers with svn can enable this to have svn revision number in executable.
# Causes compile error message otherwise.
# Until can find test for presence of svn, this is best that can be done.
#SVN_ENABLE=1

# This Makefile may be invoked from within the src directory,
# or invoked from the Main Makefile with SRC=<directory>.
# During program development, it is much easier to make from the
# the working directories.

# The main Makefile will change to the current directory to invoke
# this Makefile.  So $(SRC) is not needed in this Makefile.
# Invoke make with SRC= to compile other development directories.
SRCINC:=-I.
# The source directory.  With the current system, this is always the
# current directory.
SD:=

# Older make, such as Make 3.75 used by FreeDOS, cannot handle else if.
# Invoke make with BUILD= to compile to other build directories.
ifdef MAIN_BUILD_DIR
  # Invoked from Main Makefile
  BUILD_DIR:=$(MAIN_BUILD_DIR)
else
ifndef BUILD
  BUILD_DIR:=../
else
ifeq ("$(BUILD)",".")
  # BUILD== "." is the main directory.
  BUILD_DIR:=../
else
ifeq ("$(BUILD)","")
  # BUILD== "" is the main directory.
  BUILD_DIR:=../
else
  # BUILD in a sub-directory of the parent directory.
  BUILD_DIR:=../$(BUILD)/
endif
endif
endif
endif

ifneq ("$(findstring %%%/, %%%$(subst \,/,$(BUILD)))","")
  # Absolute path
  SERV_BUILD_DIR:=$(BUILD_DIR)
else
  # Relative path is more common, so avoid making it absolute.
  SERV_BUILD_DIR:=../../$(BUILD_DIR)
endif
# export this to musserv, sndserv
export SERV_BUILD_DIR

# Subdirectories for binaries and build intermediates
BIN =$(BUILD_DIR)bin
O  = $(BUILD_DIR)objs
DD = $(BUILD_DIR)dep

# Invoke make with MAKE_OPTIONS= to specify another file or location.
ifndef MAKE_OPTIONS
  MAKE_OPTIONS = $(BUILD_DIR)make_options
endif

# Put user settings in this file, and they will be included with every
# invocation of make.
-include $(MAKE_OPTIONS)

# $(warning OS=$(OS) )
# $(warning SMIF=$(SMIF) )

ifdef DEBUG
 # Allow DEBUG=0 to turn debugging off.
 ifeq ("$(DEBUG)","1")
  ifndef DEBUGFLAGS

# Customize to your debugging environment here.
# Normal debug (-g), MACRO info (-g3), stabs or gdb debug info
#    DEBUGFLAGS=-g3 -DDEBUG_WINDOWED
    DEBUGFLAGS=-ggdb -DDEBUG_WINDOWED
# Debuggers do not cope with optimization well, so DEBUG ignores OPTLEV.
# To debug with optimization, include it here, (-O0, -O1, or $(OPTLEV)).
#     DEBUGFLAGS+=$(OPTLEV)
   WFLAGS+=-Wundef
#  Causes many warning messages due to const strings
#  WFLAGS+= -Wwrite-strings
  endif
# Set DEBUGLIBS to special debugging libraries here.

# to detect syntax that some compilers do not handle
#  This produces many warnings, most of which we can do nothing about.
#  DEBUGFLAGS+=-pedantic

 endif
endif
# End of DEBUG

# standard
#STD= -std=c99
  # not ready yet
  # fails because of alloca, it is not std under c99
  # std=c89, does not support // commments, no inline, no asm

# End of User tunable settings


# MAKECMDGOALS is in GNUMake, MinGW
ifdef MAKECMDGOALS
  ifneq ($(MAKECMDGOALS),distclean)
  ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),dirs)
  ifneq ($(MAKECMDGOALS),depend)
    # include deps for all except clean and dirs op
    INCLUDE_DEPS=1
  endif
  endif
  endif
  endif
  ifeq ($(MAKECMDGOALS),depend)
    FORCE_AUTO_DEP=1
  endif
else
  #default
  INCLUDE_DEPS=1
  FORCE_AUTO_DEP=1
endif


# Values changed by system detection
# file delete command, init value
RM=rm
# random stuff
NASMFORMAT=elf -DLINUX

# Doom Legacy is developed with GCC.  MINGW gets used for Win32.
# The behavior of the code under other compilers is uncertain.
# Define CC_EXPLICIT_CMD in make_options to override this setting.
#  CC_EXPLICT_CMD=my_compiler
# Define CC_ENVIRONMENT to allow an environment CC to specify the compiler.

ifdef CC_EXPLICIT_CMD
  CC_CMD:=$(CC_EXPLICIT_CMD)
else
  # gcc is default
  CC_CMD:=gcc
  ifeq ($(CC_SELECT), MINGW)
    CC_CMD:=gcc
  endif
  ifeq ($(CC_SELECT), mingw)
    CC_CMD:=gcc
  endif
  ifeq ($(CC_SELECT), WATCOM)
    CC_CMD:=WATCOMC
  endif
  ifeq ($(CC_SELECT), watcom)
    CC_CMD:=WATCOMC
  endif
  ifeq ($(CC_SELECT), CLANG)
    CC_CMD:=clang
  endif
  ifeq ($(CC_SELECT), clang)
    CC_CMD:=clang
  endif
endif
ifdef CC_ENVIRONMENT
  # Give an environment CC precedence in specifing the compiler.
  CC ?= $(CC_CMD)
else
  CC:=$(CC_CMD)
endif

SMIF_LIST:= SDL LINUX_X11 FREEBSD_X11 WIN_NATIVE OS2_NATIVE DOS_NATIVE

# Default system media interface
ifdef SMIF
  # specific SMIF
  EXT_SMIF:=$(SMIF)
else
  SMIF:=SDL
endif
export SMIF

# Default OS for some SMIF
ifeq ($(SMIF), SDL)
  ifndef OS
    # Do not default OS on initialize of make_options.
    ifdef MAKE_OPTIONS_PRESENT
      # Default to Linux
      OS:=LINUX
    endif
  endif
else
ifeq ($(SMIF), LINUX_X11)
  ifdef OS
    ifneq ($(OS), LINUX)
      $(error "SMIF=LINUX_X11 requires OS=LINUX")
    endif
  else
    OS=LINUX
  endif
else
ifeq ($(SMIF), FREEBSD_X11)
  ifdef OS
    ifneq ($(OS), FREEBSD)
      $(error "SMIF=FREEBSD_X11 requires OS=FREEBSD")
    endif
  else
    OS=FREEBSD
  endif
else
ifeq ($(SMIF), WIN32_NATIVE)
  ifdef OS
    ifneq ($(OS), WIN32)
    ifneq ($(OS), WIN98)
    ifneq ($(OS), WIN7)
      $(error "SMIF=WIN32_NATIVE requires a Windows OS, such as OS=WIN32")
    endif
    endif
    endif
  else
    OS=WIN32
  endif
else
ifeq ($(SMIF), OS2_NATIVE)
  ifdef OS
    ifneq ($(OS), OS2)
      $(error "SMIF=OS2_NATIVE requires OS=OS2")
    endif
  else
    OS=OS2
  endif
else
ifeq ($(SMIF), DJGPPDOS_NATIVE)
  ifdef OS
    ifneq ($(OS), DOS)
      $(error "SMIF=DJGPPDOS_NATIVE requires OS=DOS")
    endif
  else
    OS=DOS
  endif
else
  $(error  "Unknown SMIF: $(SMIF)" )
endif
endif
endif
endif
endif
endif


OS_LIST:=LINUX FREEBSD NETBSD WIN32 WIN7 WIN98 OS2 DOS MAC

OPTS:=
OPTLIB:=
OPTLD:=
OPTCFLAG:=
SUPPORT_EXE:=
LIB_DLOPEN:=
LIB_LIBZIP:=
LIB_ZLIB:=

ifeq ($(OS), LINUX)
  LINUX=1
  OPTS+=-DLINUX
  #Known Compiler symbols
  # linux  __linux  __linux__
  # AIX defines: _AIX
  # HPux defines: hpux __hpux __hpux__
  # SGI IRIX defines: sgi  __sgi  __sgi__
  # Solaris defines:  SOLARIS
  # PPC defines: __PPC__
  # GCC on PPC defines: _ARCH_PPC  __PPC__
  # PPC 64 bit defines: __PPC64__
  # Big Endian machines (PPC) define: __BIG_ENDIAN__
  # SDL_platform.h for Solaris __SVR4 defines:  __SOLARIS__
  # SDL_platform.h for SGI variations defines:  __IRIX__
  # SDL_platform.h for HPux variations defines: __HPUX__
  # SDL_platform.h for Linux variations defines: __LINUX__
  LIB_DLOPEN:=-ldl
  LIB_LIBZIP:=-lzip
  LIB_ZLIB:=-lz
else
ifeq ($(OS), FREEBSD)
  # Has some linking differences from the rest of Linux-like OS.
  FREEBSD=1
  OPTS+=-DLINUX -DFREEBSD
  #Known Compiler symbols
  # __FreeBSD__  __FreeBSD_kernel__  __DragonFly__
  # SDL_platform.h for FreeBSD defines: __FREEBSD__
  LIB_DLOPEN:=-ldl
  LIB_LIBZIP:=-lzip
  LIB_ZLIB:=-lz
else
ifeq ($(OS), NETBSD)
  # Uses statvfs(5).
  NETBSD=1
  OPTS+=-DLINUX -DNETBSD
  #Known Compiler symbols
  # __NetBSD__
  # SDL_platform.h for NetBSD defines: __NETBSD__
  LIB_DLOPEN:=-ldl
  LIB_LIBZIP:=-lzip
  LIB_ZLIB:=-lz
else
ifeq ($(OS), WIN32)
  WIN32=1
  DOSFILE=1
  OPTS+=-DWIN32
  # if have windows > Win98, and want large memory reports
  ifdef WIN_LARGE_MEM
    OPTS+=-DWIN_LARGE_MEM
  endif
  #Known Compiler symbols
  # WIN32  _WIN32  __WINDOWS__
  # __WIN32__
  # Microsoft Visual C++ defines:  __MSC__
  # Watcom defines:  __WATCOMC__
  #  doomtype.h fixes Watcom to also define:  WIN32  __WIN32___
  # MinGW defines:   __MINGW32__
  # SDL_platform.h for Windows variations defines:  __WIN32__
else
ifeq ($(OS), WIN7)
  # This does  NOT YET  make a version customized to Win7/8.
  WIN32=1
  DOSFILE=1
  OPTS+=-DWIN32
  # probably want large memory reports
  OPTS+=-DWIN_LARGE_MEM
  #Known Compiler symbols
  # This needs investigation ...
else
ifeq ($(OS), WIN98)
  # One of the development machines is Win98.
  # If Win98 differences occur, this category can isolate them.
  WIN32=1
  DOSFILE=1
  OPTS+=-DWIN32
else
ifeq ($(OS), OS2)
  OS2=1
  DOSFILE=1
  OPTS+=-D__OS2__
  #Known Compiler symbols
  # __OS2__
else
ifeq ($(OS), DOS)
  DOS=1
  DOSFILE=1
  OPTS+=-DDOS
  #Known Compiler symbols
  # PC_DOS
else
ifeq ($(OS), MAC)
  MAC=1
  OPTS+=-DMAC
  #Known Compiler symbols
  # __APPLE__  __APPLE_CC__  __MACH__
  # Apple GNAT, GNU C 4.5 defines __APPLE_CC__ = 1,  __MACH__,  __GNUC__
  # Apple C 4.2 defines  __APPLE_CC__ = 5666, __MACH__, __GNUC__
  # When compiling for big endian arch defines:  __BIG_ENDIAN__
  # SDL_platform.h for MacIntosh defines: __MACOS__
  # SDL_platform.h for Apple defines:     __MACOSX__
else
  $(error  "Unknown OS: $(OS)" )
endif
endif
endif
endif
endif
endif
endif
endif
endif
export DOSFILE

# OPTS goes to the C compiler and assembler.
# OPTS is almost all defines using -D. It selects conditional code.
# CFLAGS only goes to the C compiler.  It gets a copy of OPTS.
# LDFLAGS goes to the linker.  Some linkers cannot handle OPTS.
# LIBS goes to the linker.

ifdef COMP_ENVIRONMENT
  # Things like this will always have an enabling control.
  # Otherwise, something that is useful on one system, will be a surprise on another system.
  # And it is very hard to debug other systems when unknown settings
  # are allowed to influence compiling, without any control or notification.

  # NETBSD: Include the environment CFLAGS in OPTS.
  # (This is what I got from the NETBSD user, and don't know why it
  #  goes in OPTS instead of CFLAGS.)
  OPTS+=$(CFLAGS)
#  ENV_CFLAGS=$(CFLAGS)

  # NETBSD: Has environment settings that should be used?
  # Do not reset LIBS, use the LIBS from the environment.
  # Do not reset LDFLAGS, use the LDFLAGS from the environment.
else
  LIBS:=
  LDFLAGS:=
endif

# all OPTINC need to have -I. because of how includes are written
# however the order of -I may be important, so it is left to each SMIF.
OPTINC:=
INSTALL_SUPPL:=

CFLAGS:=
# Machine architecture.
ifdef ARCH
  # if does not have leading -march, -mcpu, -mtune, or similar.
  ifeq ($(filter -march% -mcpu% -mtune% -m%, $(strip $(ARCH))),)
    # Apply to most general architecture compiler flag.
    CFLAGS:=-march=$(ARCH)
    $(warning Using ARCH as: $(CFLAGS) )
  else
    # Looks like a complete switch, use it as is.
    CFLAGS:=$(ARCH)
  endif
endif

ifdef ENV_CFLAGS
  CFLAGS+=$(ENV_CFLAGS)
endif

SND_DEVS:=
SND_CFLAGS:=
SND_LIBS:=
SND_LDFLAGS:=

# Sound Devices support, mostly for Linux X11.
# If HAVE_xx=3, the sound device library will be detected and loaded dynamically.
# If HAVE_xx=1, the sound device library must be present.
# Use export to pass sound devices to the sound server Makefile.
ifdef HAVE_OSS
  ifeq ($(HAVE_OSS), 3)
    SND_DEVS+=-DDEV_OSS=3
  else
    SND_DEVS+=-DDEV_OSS
  endif
  export HAVE_OSS
endif

ifdef HAVE_ALSA
  ifeq ($(HAVE_ALSA), 3)
    SND_DEVS+=-DDEV_ALSA=3
  else
    SND_DEVS+=-DDEV_ALSA
    SND_LIBS+=-lasound
  endif
  SND_LIBS+=-lm -ldl -lpthread -lrt
  export HAVE_ALSA
endif

ifdef HAVE_ESD
  ifeq ($(HAVE_ESD), 3)
    SND_DEVS+=-DDEV_ESD=3
  else
    SND_DEVS+=-DDEV_ESD
    SND_LIBS+=-lesd
  endif
  export HAVE_ESD
endif

ifdef HAVE_JACK
  ifeq ($(HAVE_JACK), 3)
    SND_DEVS+=-DDEV_JACK=3
  else
    SND_DEVS+=-DDEV_JACK
    SND_LIBS+=-ljack
  endif
  SND_LIBS+=-lpthread
  export HAVE_JACK
endif

ifdef HAVE_PULSE
  ifeq ($(HAVE_ALSA), 3)
    SND_DEVS+=-DDEV_PULSE=3
  else
    SND_DEVS+=-DDEV_PULSE
    SND_LIBS+=-L/usr/lib/pulseaudio -lpulse
  # do not seem to need pulsecommon
#  SND_LIBS+=-L/usr/lib/pulseaudio -lpulse -lpulsecommon-$(HAVE_PULSE_COMMON_VERSION)
#  SND_LIBS+=-lpulse
   SND_LDFLAGS+=-L/usr/lib/pulseaudio
#  SND_LDFLAGS+=-L/usr/lib/pulseaudio -lpulsecommon-$(HAVE_PULSE_COMMON_VERSION)
#  SND_LDFLAGS+=-L/usr/lib/pulseaudio -lpulsecommon
  # modlibexecdir=/usr/lib/pulse-9.0/modules
  endif
  SND_CFLAGS+=-D_REENTRANT
  export HAVE_PULSE
endif

# Music Devices support, mostly for Linux X11.
# MIDI devices do not seem to need libraries.
# Use export to pass devices to the music server Makefile.

ifdef HAVE_TIMIDITY
  ifeq ($(HAVE_TIMIDITY), 3)
    SND_DEVS+=-DDEV_TIMIDITY=3
  else
    SND_DEVS+=-DDEV_TIMIDITY
  endif
  export HAVE_TIMIDITY
endif

ifdef HAVE_FLUIDSYNTH
  ifeq ($(HAVE_FLUIDSYNTH), 3)
    SND_DEVS+=-DDEV_FLUIDSYNTH=3
  else
    SND_DEVS+=-DDEV_FLUIDSYNTH
  endif
  export HAVE_FLUIDSYNTH
endif

ifdef HAVE_FM_SYNTH
  ifeq ($(HAVE_FM_SYNTH), 3)
    SND_DEVS+=-DDEV_FM_SYNTH=3
  else
    SND_DEVS+=-DDEV_FM_SYNTH
  endif
  export HAVE_FM_SYNTH
endif

ifdef HAVE_AWE32_SYNTH
  ifeq ($(HAVE_AWE32_SYNTH), 3)
    SND_DEVS+=-DDEV_AWE32_SYNTH=3
  else
    SND_DEVS+=-DDEV_AWE32_SYNTH
  endif
  export HAVE_AWE32_SYNTH
endif



#=================================
ifeq ($(SMIF), SDL)
# SDL is the default system media interface. This is what all modern systems should use.
# Requires SDL libs, and optionally SDL_mixer libs.

# The system and SDL interface files are in sdl directory (all i_*.c).
  INTERFACE=sdl

  MAINOBJ:=i_main.o
  CDMUSOBJ:=i_cdmus.o
  SMIFOBJS:=i_system.o i_video.o i_sound.o i_net.o \
    hw_bsp.o hw_draw.o hw_light.o hw_main.o hw_md2.o hw_cache.o hw_trick.o \
    r_opengl.o ogl_sdl.o hwsym_sdl.o \
    filesrch.o dosstr.o endtxt.o

  DEP_HARD:=1
  DEP_OPENGL:=1

# platform
  ifdef WIN32
    # Windows (MinGW)
    EXENAME:=doomlegacy.exe
    LIBS:=-lopengl32 -lglu32 -lwsock32 -lm
  endif
  ifdef MAC
    EXENAME:=doomlegacy
    LDFLAGS+=-framework OpenGL
  endif
  ifndef EXENAME
    # default is Linux, for all unix SDL
    EXENAME:=doomlegacy
    LDFLAGS+=-L/usr/X11R6/lib
    LIBS+=-lGL -lGLU -lm
    # -L/usr/X11R6/lib is needed by Linux 2.4 and others that still have
    # the GLU libraries in an X11 directory.
    # -lm is needed for pow, powf, and other MATH1 functions.
  endif

  # Try to access sdl-config, for all platforms.
  ifdef SDL2
    # SDL 2.x
    OPTS+=-DSDL2
    # SDL2 does not support CD-MUSIC.
    undefine CDMUS
    SDLCONFIG_LIBS:=$(shell sdl2-config --libs)
    ifeq "$(findstring -lSDL, $(SDLCONFIG_LIBS) )" "-lSDL"
      # Got valid output from sdl2-config
      LIBS+=$(SDLCONFIG_LIBS)
      OPTINC+=$(shell sdl2-config --cflags)
    else
      $(warning SDL2: sdl2-config not found )
      # Extracted from Linux sdl-config script
      ifeq ($(CC_SELECT), MINGW)
        ifndef SDL_DIR
          SDL_DIR="C:/SDL"
#          SDL_DIR="C:/mingw"
        endif
        OPTS+=-Dmain=SDL_main
        OPTINC+=-I$(SDL_DIR)/include/SDL2
        LIBS+=-L$(SDL_DIR)/lib
        LIBS+=-lmingw32 -mwindows
        LIBS+=-lSDLmain -lSDL2
      endif
      ifeq ($(CC_SELECT), WATCOM)
        ifndef SDL_DIR
          SDL_DIR="C:/SDL"
#          SDL_DIR="C:/watcom"
        endif
        OPTS+=-Dmain=SDL_main
        OPTINC+=-I$(SDL_DIR)/include/SDL2
#       OPTINC+=-I/watcom/h/SDL2
        LIBS+=-L$(SDL_DIR)/lib
        LIBS+=-lwatcom -mwindows
        LIBS+=-lSDLmain -lSDL2
      endif
      ifndef SDL_DIR
         # Linux, BSD, Mac
#        SDL_DIR="/usr"
         SDL_DIR="/usr/local"
      endif
      ifdef MAC
        # Some Mac SDL downloads do not have sdl2-config.
        # Mac packages can put SDL in /usr or /usr/local, so cover both cases.
        OPTS+=-D_REENTRANT
        OPTINC+=-I$(SDL_DIR)/include/SDL2
        LIBS+=-L$(SDL_DIR)/lib -Wl,-rpath,$(SDL_DIR)/lib -Wl,-enable-new-dtags -lpthread -lSDL2
      endif
      ifneq "$(findstring SDL, $(OPTINC) )" "SDL"
        OPTS+=-D_REENTRANT
        OPTINC+=-I$(SDL_DIR)/include/SDL2
        LIBS+=-L$(SDL_DIR)/lib -Wl,-rpath,$(SDL_DIR)/lib -Wl,-enable-new-dtags -pthread -lSDL2
      endif
    endif
  else
    # SDL 1.2
    SDLCONFIG_LIBS:=$(shell sdl-config --libs)
    ifeq "$(findstring -lSDL, $(SDLCONFIG_LIBS) )" "-lSDL"
      # Got valid output from sdl-config
      LIBS+=$(SDLCONFIG_LIBS)
      OPTINC+=$(shell sdl-config --cflags)
    else
      $(warning SDL: sdl-config not found )
      # Extracted from sdl-config script
      ifeq ($(CC_SELECT), MINGW)
        ifndef SDL_DIR
          SDL_DIR="C:/SDL"
#         SDL_DIR="/mingw"
        endif
        OPTS+=-D_GNU_SOURCE=1 -Dmain=SDL_main
        OPTINC+=-I$(SDL_DIR)/include/SDL
        LIBS+=-L$(SDL_DIR)/lib
#        LIBS+=-L/mingw/lib
#        LIBS+=-L/windows/system32
        LIBS+=-lmingw32 -mwindows
        LIBS+=-lSDLmain -lSDL
      endif
      ifeq ($(CC_SELECT), WATCOM)
        ifndef SDL_DIR
          SDL_DIR="C:/SDL"
#         SDL_DIR="/watcom"
        endif
        OPTS+=-D_GNU_SOURCE=1 -Dmain=SDL_main
        OPTINC+=-I$(SDL_DIR)/include/SDL
#       OPTINC+=-I/watcom/h/SDL
        LIBS+=-L$(SDL_DIR)/lib
#       LIBS+=-L/windows/system32
        LIBS+=-lwatcom -mwindows
        LIBS+=-lSDLmain -lSDL
      endif
      ifndef SDL_DIR
         # Linux, BSD
#        SDL_DIR="/SDL"
#        SDL_DIR="/usr"
         SDL_DIR="/usr/local"
      endif
      ifdef MAC
        # Some Mac SDL downloads do not have sdl-config.
        # Mac packages can put SDL in /usr or /usr/local, so cover both cases.
        OPTS+=-D_GNU_SOURCE=1 -D_THREAD_SAFE
        OPTINC+=-I/usr/include/SDL -I/usr/local/include/SDL
#        LIBS+=-L/usr/lib -Wl,-rpath,/usr/lib -lSDL -lpthread
        LIBS+=-L/usr/local/lib -lSDL -Wl,-framework,Cocoa
      endif
      ifneq "$(findstring SDL, $(OPTINC) )" "SDL"
        OPTS+=-D_GNU_SOURCE=1 -D_REENTRANT
        OPTINC+=-I$(SDL_DIR)/include/SDL
        LIBS+=-L$(SDL_DIR)/lib -Wl,-rpath,$(SDL_DIR)/lib -lSDL -lpthread
      endif
    endif
  endif

  OPTS+=-DHWRENDER -DSMIF_SDL
  OPTINC+=$(SRCINC)

  ifdef HAVE_MIXER
    # Allow HAVE_MIXER=0 to turn it off.
    ifeq ("$(HAVE_MIXER)", "1")
      # SDL has separate SDL_Mixer library.
      OPTS+=-DHAVE_MIXER
      ifdef SDL2
        LIBS+=-lSDL2_mixer
      else
        LIBS+=-lSDL_mixer
      endif
    endif
  endif

  ifdef MAC
    # Objective-C main program is required for SDL 1.2.x
    # DoomLegacy has its own SDLmain source, but others may work,
    # though they would change the default directory according to .app.
    SMIFOBJS+=SDLmain.o
    LIBS+=-lobjc
    # Optional places for include and libraries, unique to Mac.
    # Enable MAC_FRAMEWORK if you really want to debug frameworks (I don't).
    ifdef MAC_FRAMEWORK
      CFLAGS += -DMAC_FRAMEWORK
      FRAMEWORK = -framework Cocoa -framework OpenGL -framework SDL
      # Do not remember which example had -F./osx/Frameworks      
      #      LDFLAGS += -F./osx/Frameworks $(FRAMEWORK) -framework Foundation
      LDFLAGS +=  $(FRAMEWORK) -framework Foundation
    endif
  endif


#=================================
else
ifeq ($(SMIF), LINUX_X11)
  # Requires X11/Xlib, and xshm or ggi libs
  INTERFACE=linux_x

  SUPPORT_EXE:=dll
  
  DEP_HARD:=1
  DEP_X11:=1
  DEP_OPENGL:=1

  #use the x86 asm code
  #USEASM=1

  # Uncomment if you want to use the POLL_POINTER hack
  #POLL_POINTER:=-DPOLL_POINTER
  # Uncomment if you want to use SOUND_RESET, only valid without SNDSERV
  #OPTS+=-DSOUND_RESET

  OPTS+=-DSMIF_X11 -DHWRENDER $(SND_DEVS)

  # objs common to xshm and ggi, video is added later
  MAINOBJ:=i_main.o
  CDMUSOBJ:=i_cdmus.o
  SMIFOBJS:=i_system.o i_sound.o i_net.o \
    hw_bsp.o hw_draw.o hw_light.o hw_main.o hw_md2.o hw_cache.o hw_trick.o \
    r_opengl.o ogl_x11.o \
    searchp.o filesrch.o dosstr.o endtxt.o

  INSTALL_SUPPL:=r_opengl.so

  ifdef SOUND_SERVER
    OPTS+=-DSNDSERV
    SNDSERV:=linux_x/sndserv
    SUPPORT_EXE+=sndserv
  else
    OPTS+=$(SND_CFLAGS)
    OPTLIB+=$(SND_LIBS)
    OPTLD+=$(SND_LDFLAGS)
    SMIFOBJS+=snd_driver.o
  endif

  ifdef MUSIC_SERVER
    OPTS+=-DMUSSERV
    MUSSERV:=linux_x/musserv
    SUPPORT_EXE+=musserv
  else
  endif

  ifdef X11_GGI
    # GGI
    OPTS+=-DSMIF_X11_GGI
    OPTINC:=$(SRCINC)
    LDFLAGS:=$(OPTLD)
    LIBS:=-lggi -lm $(OPTLIB)
    # name of the exefile
    EXENAME:=llggidoom
    SFLAGS:=-g $(OPTS)
    SMIFOBJS+=i_video_ggi.o
  else
    # standard X11
    OPTS+=$(POLL_POINTER)
    OPTINC+=$(SRCINC) -I/usr/X11R6/include
    LDFLAGS:=-L/usr/X11R6/lib $(OPTLD)
    LIBS:=-lGL -lGLU -lXext -lX11 -lm -ldl -lXxf86vm $(OPTLIB)

    #WITH_DGA=1
    ifdef WITH_DGA
      OPTS+=-DWITH_DGA
      LIBS+=-lXxf86dga -lXext
    endif #WITH_DGA

    # name of the exefile
    EXENAME:=llxdoom
    SFLAGS:=-g $(OPTS)
    SMIFOBJS+=i_video_xshm.o
  endif


#=================================
else
ifeq ($(SMIF), FREEBSD_X11)
  # Requires X11/Xlib, and xshm or ggi libs
  INTERFACE=linux_x

#  MUSSERV=linux_x/musserv
  OSVERSION:=$(shell /sbin/sysctl -n kern.osreldate)
  OSMAJOR:=$(shell echo $(OSVERSION) | sed 's/.....$$//')

  SUPPORT_EXE:=dll

  DEP_HARD:=1
  DEP_X11:=1
  DEP_OPENGL:=1

  #use the x86 asm code
  #USEASM:=1

  # Uncomment if you want to use the POLL_POINTER hack
  #POLL_POINTER:=-DPOLL_POINTER
  # Uncomment if you want to use SOUND_RESET, only valid without SNDSERV
  #OPTS+=-DSOUND_RESET

  OPTS+=-DSMIF_X11 -DHWRENDER

  # objs common to xshm and ggi, video is added later
  # Note: free-bsd version of cdmus
  MAINOBJ:=i_main.o
  CDMUSOBJ:=i_cdmus_fbsd.o
  SMIFOBJS:=i_system.o i_sound.o i_net.o \
    hw_bsp.o hw_draw.o hw_light.o hw_main.o hw_md2.o hw_cache.o hw_trick.o \
    searchp.o filesrch.o dosstr.o endtxt.o

  INSTALL_SUPPL:=r_opengl.so

  ifdef SOUND_SERVER
    OPTS+=-DSNDSERV
    SNDSERV=linux_x/sndserv
    SUPPORT_EXE+=sndserv
  else
    OPTS+=$(SND_CFLAGS)
    OPTLIB+=$(SND_LIBS)
    OPTLD+=$(SND_LDFLAGS)
    SMIFOBJS+=snd_driver.o
  endif

  ifdef X11_GGI
    # GGI
    OPTS+=-DSMIF_X11_GGI
    OPTINC+=$(SRCINC) -I/usr/local/include
    LDFLAGS:=$(OPTLD)
    LIBS:=-lggi -lm -lipx $(OPTLIB)
    # name of the exefile
    EXENAME:=llggidoom
    SFLAGS:=-g $(OPTS)
    SMIFOBJS+=i_video_ggi.o
  else
    # standard X11
    OPTS+=$(POLL_POINTER)
    OPTINC+=$(SRCINC) -I/usr/X11R6/include
    LDFLAGS:=-L/usr/X11R6/lib $(OPTLD)
    LIBS:=-lXext -lX11 -lm -lXxf86vm -lipx -lkvm -lpthread $(OPTLIB)
    # name of the exefile
    EXENAME:=llxdoom
    SFLAGS:=-g $(OPTS)
    SMIFOBJS+=i_video_xshm.o
  endif

#=================================
else
ifeq ($(SMIF), OS2_NATIVE)
  #Requires OS2 libs, os2.h, os2me.h, fourcc.h, DIVE libs, and mmpm2 libs
  #Does not have cdmusic support
  DOSFILE=1
  INTERFACE=os2
  NASMFORMAT:=coff

#use the x86 asm code
  NASM:=nasm
#USEASM=1
  PROFILEMODE=1
# options (separate OS2 library and OS2_NATIVE video)
  OPTS+= -DSMIF_OS2_NATIVE
  OPTINC+=-Zmt $(SRCINC) -I$(SD)os2

#  DEBUGLIBS:=
  LIBS:=-g $(INTERFACE)/I_pm.def -lmmpm2 -lsocket

  SFLAGS:=-g $(OPTS)

  MAINOBJ:=main.o
  CDMUSOBJ:=i_cdmus.o
  SMIFOBJS:=i_system.o i_video.o i_sound.o i_net.o i_dart.o i_dive.o I_pm.o \
    printf.o I_pm.res filesrch.o

# name of the exefile
  EXENAME:=legacy.exe


#=================================
else
ifeq ($(SMIF), WIN32_NATIVE)
  # Requires windows.h, windowsx.h, ddraw.h and ddraw libs, and
  # optionally fmod libs
  DOSFILE=1
  INTERFACE=win32
  NASMFORMAT:=coff

  DEP_HARD:=1
  DEP_DD:=1

  # DDINC="" alternative directory to find ddraw.h and other includes
  # DDLIB="" alternative directory to find ddraw and other libs
  ifndef DDINC
#    DDINC=-I\MINGW\MX80\include
  endif
  ifndef DDLIB
#    DDLIB=-L\MINGW\MX80\lib
  endif
  # FMODINC="" alternative directory to find fmod.h and other includes
  # FMODLIB="" alternative directory to find fmod libs
  ifdef HAVE_FMOD
    ifndef FMODINC
      FMODINC=-I\MINGW\fmod\include
    endif
    ifndef FMODLIB
      FMODLIB=-L\MINGW\fmod\lib
    endif
  endif

#use the x86 asm code
  #USEASM=1

# options (separate WIN32 library, and WIN_NATIVE video)
  OPTS+=-DSMIF_WIN_NATIVE
  OPTINC+=-g $(SRCINC)
  ifdef DDINC
    OPTINC+=$(DDINC)
  endif

#  DEBUGLIBS:=
  LIBS:=
  ifdef DDLIB
    LIBS+=$(DDLIB)
  endif
  ifeq ($(CC_SELECT), MINGW)
    LIBS+=-lwinmm -lwsock32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid
  else
    LIBS+=-lwinmm -lsocket -lddraw -ldsound -ldinput
  endif

  ifdef HAVE_FMOD
    OPTS+=-DFMOD_SOUND
    ifdef FMODLIB
      LIBS+=$(FMODLIB) -lfmod
    endif
  endif

  SFLAGS:=

  MAINOBJ:=win_main.o
  SMIFOBJS:=win_sys.o win_vid.o win_snd.o win_net.o win_cd.o \
    filesrch.o fabdxlib.o mid2strm.o dx_error.o

# name of the exefile
  EXENAME:=legacy.exe

#=================================
else
ifeq ($(SMIF), DJGPPDOS_NATIVE)
  # Requires allegro libs, bcd libs (cd-music)
  RM:=del
  DOSFILE=1
  INTERFACE=djgppdos
  NASMFORMAT:=coff

#  DEP_HARD:=1

#use the x86 asm code
  #USEASM=1

# options
  OPTS:=-DSMIF_PC_DOS
  OPTINC+=-g $(SRCINC)

#  DEBUGLIBS:=
  LIBS:=-lalleg -lsocket
  ifdef CDMUS
     LIBS+=-lbcd
  endif

  SFLAGS:=

  MAINOBJ:=i_main.o
  CDMUSOBJ:=i_cdmus.o
  SMIFOBJS:=i_system.o i_video.o vid_vesa.o i_sound.o i_net.o filesrch.o

# name of the exefile
  EXENAME:=legacy.exe
endif
endif
endif
endif
endif
endif
#=================================

#  $(info $(PATH) )
# DOS/OS2/WIN32 file differences
ifdef DOSFILE
  ifndef HAVE_DOSCOMMAND
    ifneq ("$(findstring "MSYS", "$(SHELL)"),"")
      $(info HAVE MSYS )
      # When SHELL = command.com, it chokes on @echo and - $(RM)
      # When SHELL = cmd.exe, it works for mingw32-make, but MSYS make
      # does not execute the DOS commands
#     SHELL:=cmd.exe
#     .SHELLFLAGS="/C /Q"
      HAVE_MSYS=1
    endif
  endif
  ifdef HAVE_MSYS
    # MSYS has its own idea of the root directory.
    LIBS:=$(subst -L/,-LC:/,$(LIBS))
    $(info LIBS=$(LIBS) )
  else
    $(info HAVE DOSCOMMAND )
    HAVE_DOSCOMMAND=1
    RM:=del
  endif
  BIN_WIN:=$(subst /,\,$(BIN))
  O_WIN:=$(subst /,\,$(O))
  DD_WIN:=$(subst /,\,$(DD))
else
  SHELL:=/bin/sh
endif
#  $(info HAVE SHELL=$(SHELL) )

OPTOBJS:=

# use assembly routines?
ifdef USEASM
   OPTOBJS+=tmap.o vid_copy.o
   OPTS+=-DUSEASM
endif

# CD-Music
ifdef CDMUS
   OPTOBJS+=$(CDMUSOBJ)
   OPTS+=-DCDMUS
endif

# LIBZIP
# For now, Linux, BSD, NetBSD, FreeBSD, only.
# The protective test is not actually necessary,
# as only these OS should ever define HAVE_DLOPEN, HAVE_LIBZIP.
# Using $(filter) as a in_list test, would be indirect and confusing,
# and might not work in some make, such as MINGW_make.
ifdef LIB_DLOPEN
  ifdef HAVE_DLOPEN
    LIBS+=$(LIB_DLOPEN)
    OPTS+=-DHAVE_DLOPEN
  endif
endif

ifdef LIB_LIBZIP
  ifdef HAVE_LIBZIP
    OPTS+=-DHAVE_LIBZIP=$(HAVE_LIBZIP)
    ifeq ($(HAVE_LIBZIP), 1)
      LIBS+=$(LIB_LIBZIP)
    endif
  endif
endif

# ZLIB
ifdef LIB_ZLIB
  ifdef HAVE_ZLIB
    OPTS+=-DHAVE_ZLIB=$(HAVE_ZLIB)
    ifeq ($(HAVE_ZLIB), 1)
      LIBS+=$(LIB_ZLIB)
    endif
  endif
endif

# UMAPINFO directory
ifdef UMAPINFO_DIR
  ifeq ($(UMAPINFO, 0))
    undefine UMAPINFO_DIR
  endif
else
  # UMAPINFO_DIR by default
  UMAPINFO_DIR=1
endif


# compiler and linker flags
CFLAGS+=$(WFLAGS)

ifdef PROFILEMODE
# build with gprof profiling information
  CFLAGS+=-g -pg -fomit-frame-pointer
else
ifdef DEBUG
# No -fomit-frame-pointer because debugging on some machines depends upon it.
  #CFLAGS+=-g3 -O0
  CFLAGS+=$(DEBUGFLAGS)
  ifeq ($(CC_CMD), clang)
    # Clang does not recognize this flag  
  else
    CFLAGS+=-fno-guess-branch-probability
  endif
  ifdef DEBUGLIBS
    # Search debug libraries first, otherwise normal library
    LIBS:=$(DEBUGLIBS) $(LIBS)
  endif
else
# build a normal optimized version
  #CFLAGS+=-O3
  CFLAGS+=$(OPTLEV) -fomit-frame-pointer
endif
endif

CFLAGS+=-ffast-math -fno-strict-aliasing
# WIN98 linking needs LCFLAGS without -D flags and does not need includes
LCFLAGS:=$(CFLAGS)
CFLAGS+=$(OPTS) $(OPTINC)

ifdef MAC
  # MAC linker does not accept these flags.
else
ifdef NETBSD
  # NETBSD linker does not accept these flags.
else
  # Flags to the linker.
  LDFLAGS+=-Xlinker --warn-common
endif
endif


# objs that are in main directories
MOBJS:=\
screen.o v_video.o \
r_draw.o r_plane.o r_segs.o r_sky.o r_things.o r_splats.o \
r_bsp.o r_data.o r_main.o \
z_zone.o \
p_sight.o p_mobj.o p_enemy.o p_user.o p_inter.o p_pspr.o \
p_lights.o p_ceilng.o p_doors.o p_plats.o p_floor.o p_spec.o \
p_switch.o p_genlin.o p_telept.o p_tick.o p_fab.o p_info.o p_setup.o \
p_blockmap.o p_extnodes.o \
p_map.o p_maputl.o \
p_heretic.o p_hsight.o \
p_chex.o \
umapinfo.o \
sb_bar.o hu_stuff.o \
st_lib.o st_stuff.o \
t_array.o t_func.o t_oper.o t_parse.o t_prepro.o t_script.o t_spec.o t_vari.o \
sounds.o qmus2mid.o s_sound.o s_amb.o mserv.o \
b_game.o b_look.o b_node.o b_search.o \
g_input.o g_game.o \
f_finale.o f_wipe.o \
wi_stuff.o \
hs_stuff.o \
au_stuff.o \
am_map.o \
md5.o \
m_menu.o m_misc.o m_argv.o m_bbox.o m_fixed.o m_swap.o m_cheat.o m_random.o \
console.o command.o \
p_saveg.o \
tables.o info.o infoext.o dstrings.o dehacked.o w_wad.o w_zip.o\
d_netcmd.o d_clisrv.o d_net.o d_netfil.o i_tcp.o \
d_items.o d_main.o

ifdef UMAPINFO_DIR
# UMAPINFO directory
UMIOBJS:= doom_umi_api.o doom_umi_parser.o doom_umi_lexer.o doom_umi_keywords.o
endif

# combine all objs ( main, INTERFACE specific, asm, CDMUS )
OBJS:=$(addprefix $(O)/, $(SMIFOBJS) $(OPTOBJS) $(MOBJS) $(UMIOBJS))
MAINOBJ:=$(addprefix $(O)/, $(MAINOBJ))


#=======================================================
# rules
#=======================================================
# Do not have directories as prerequisites because their date is updated
# whenever a file is output, which fouls up the rules.

.PHONY : all clean distclean dirs asm dll install

# prevent compile without make_options file
ifndef MAKE_OPTIONS_PRESENT
  $(error Create make_options file first, using parent Makefile.)
endif

# ALL, compile and link
all:	 $(BIN)/$(EXENAME)  $(SUPPORT_EXE)  cabinet_home


# [Arcade] Portable install.
# The engine prefers a "legacyhome" directory sitting next to the binary over
# the one in $HOME (d_main.c), so staging the repo's tracked config there makes
# a freshly built tree run with no command line arguments at all.
#
# CABINET_HOME is the tracked source of truth; $(BIN)/legacyhome is the live
# copy the game actually reads and writes.  bin/ is gitignored, so edits an
# operator makes in a -devmode session land in an untracked file -- run
# "make cabinet_save" to bring them back to the tracked copy and see them as a
# git diff.
#
# Recipes are prefixed with "-" and guarded, so a platform without a POSIX
# shell just skips this rather than failing the build.
CABINET_HOME := ../../cabinet/legacyhome

SMOKE_SH := ../../tools/smoke.sh

.PHONY: cabinet_home cabinet_save smoke

# Stage the tracked config beside the binary, without ever clobbering a live
# one.  cp -n leaves an existing config.cfg alone, so a running cabinet does
# not have its settings reset by a rebuild.
cabinet_home:
	-@if [ -d "$(CABINET_HOME)" ]; then \
	  mkdir -p "$(BIN)/legacyhome" && \
	  cp -n "$(CABINET_HOME)/config.cfg" "$(BIN)/legacyhome/" 2>/dev/null; \
	  echo "Portable legacyhome staged at $(BIN)/legacyhome"; \
	fi

# [Arcade] Headless smoke test.  There is no unit test suite; this is the
# closest thing to one -- it runs the built binary under SDL's dummy and
# offscreen drivers and checks startup, level setup, the OpenGL path, a level
# exit and the high-score write.  See tools/smoke.sh and CLAUDE.md.
#
# Runs against a scratch copy, never the live cabinet.  Set DOOMWADDIR if the
# IWADs are not in one of the usual places.
smoke:
	@$(SMOKE_SH) $(SMOKE_ARGS)

# Copy the live config back over the tracked one, for committing.
cabinet_save:
	-@if [ -f "$(BIN)/legacyhome/config.cfg" ]; then \
	  cp "$(BIN)/legacyhome/config.cfg" "$(CABINET_HOME)/config.cfg" && \
	  echo "Saved $(BIN)/legacyhome/config.cfg -> $(CABINET_HOME)/config.cfg"; \
	else \
	  echo "No live config at $(BIN)/legacyhome/config.cfg"; \
	fi


# Directly using $(error) prevents the output of the other commands.


# Win32/OS2/DOS sees "/" as a switch, "\" is an escape to make
clean:
ifdef HAVE_DOSCOMMAND
	@echo "Removing dependencies $(DD_WIN)"
	- $(RM) $(DD_WIN)\*.dep
	@echo "Removing obj  $(O_WIN)"
	- $(RM) $(O_WIN)\*.o
else
	@echo "Removing dependencies $(DD)"
	- $(RM) $(DD)/*
	@echo "Removing obj  $(O)"
	- $(RM) $(O)/*.o
endif
ifeq ($(SMIF), LINUX_X11)
	cd $(SD)$(MUSSERV) && $(MAKE) clean
	cd $(SD)$(SNDSERV) && $(MAKE) clean
#	- $(RM) $(O)/$(MUSSERV)/*.o
#	- $(RM) $(O)/$(SNDSERV)/*.o
else
ifeq ($(SMIF), FREEBSD_X11)
	cd $(SD)$(MUSSERV) && $(MAKE) clean
	cd $(SD)$(SNDSERV) && $(MAKE) clean
#	- $(RM) $(O)/$(MUSSERV)/*.o
#	- $(RM) $(O)/$(SNDSERV)/*.o
endif
endif

distclean:
ifdef HAVE_DOSCOMMAND
	- $(RM) $(DD_WIN)\*.dep
	- $(RM) $(O_WIN)\*.o
	- $(RM) $(O_WIN)\*\*.o
else
	- $(RM) $(DD)/*
	- $(RM) $(O)/*.o
	- $(RM) $(O)/*/*.o
endif


.PHONY : versionstring
# This may fail because, (a) svn not installed, (b) not a svn directory.
# This compiles d_main a second time, with SVN_REV set.
versionstring:
ifdef SVN_ENABLE
	$(CC) -c $(CFLAGS) -DSVN_REV=\"`svn info | grep Revision | sed -e 's/Revision: //'`\" d_main.c -o $(O)/d_main.o
endif

# Maintenance Functions, mostly Linux only


# Make a disasm of the exe
disasm: $(BIN)/$(EXENAME)
	objdump -d $(BIN)/$(EXENAME) --no-show-raw-insn > $(BIN)/doomlegacy.s

# To dump draw functions to assembly
objdump_draw: $(O)/r_draw.o
	objdump -d $(O)/r_draw.o -l --source --no-show-raw-insn > $(BIN)/r_draw.s


# Make dll for X11
dll : $(O)/r_opengl.o $(O)/ogl_x11.o
	$(CC) -O3 $(OPTS) -o $(BIN)/r_opengl.so -shared -nostartfiles \
	  $(O)/r_opengl.o $(O)/ogl_x11.o \
	  -L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lGL -lGLU -lm


# Linux X11 sound and music servers
ifdef MUS_OS
  export MUS_OS
endif

musserv:
ifdef MUSSERV
	cd $(SD)$(MUSSERV) && $(MAKE)
endif

sndserv:
ifdef SNDSERV
	cd $(SD)$(SNDSERV) && $(MAKE)
endif


#=======================================================

# executable rule

$(BIN)/$(EXENAME): $(OBJS) $(MAINOBJ) versionstring
	@echo Linking...
ifdef DOSFILE
# MinGW on WIN98: linking cannot handle $(OPTS), use $(LCFLAGS) instead of $(CFLAGS)
	$(CC) $(LCFLAGS) $(LDFLAGS) -o $(BIN)/$(EXENAME) \
	$(MAINOBJ) $(OBJS) \
	$(LIBS)
else
# Linux/BSD
ifdef CC_EXE_LAST
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(MAINOBJ) \
	$(LIBS) -o $(BIN)/$(EXENAME)
else
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BIN)/$(EXENAME) \
	$(MAINOBJ) $(OBJS) \
	$(LIBS)
endif
endif


#=======================================================

# obj rules

vpath %.o $(O)

#$(O)/*.o : doomdef.h

$(O)/r_draw.o : $(SD)r_draw.c $(SD)r_draw8.c $(SD)r_draw16.c $(SD)r_draw24.c $(SD)r_draw32.c


# common obj rules

$(O)/%.o: $(SD)%.c
	$(CC) $(CFLAGS) $(SND_DEVS) -c $< -o $@

$(O)/%.o: $(SD)$(INTERFACE)/%.c
	$(CC) $(CFLAGS) $(SND_DEVS) -c $< -o $@

$(O)/%.o: $(SD)hardware/%.c
	$(CC) $(CFLAGS) -c $< -o $@

ifeq ($(SMIF), OS2_NATIVE)
# OS2 rules
$(O)/%.res: $(SD)os2/%.rc
	rc.exe -i $(SD)os2 -r $< $@
endif


ifeq ($(SMIF), SDL)
# SDL
# OpenGL rules
$(O)/r_opengl.o: $(SD)hardware/r_opengl/r_opengl.c $(SD)hardware/r_opengl/r_opengl.h
	$(CC) $(CFLAGS) -c $< -o $@

ifdef MAC
# Mac must have Objective-C main for SDL
$(O)/SDLmain.o : $(SD)sdl/SDLmain.m
	$(CC) -x objective-c $(CFLAGS) $(FRAMEWORK) -c $< -o $@
endif

else
# Not SDL
# OpenGL rules
# LINUX_X11, FREEBSD_X11 at least
$(O)/r_opengl.o: $(SD)hardware/r_opengl/r_opengl.c $(SD)hardware/r_opengl/r_opengl.h
	$(CC) $(CFLAGS) -shared -nostartfiles -c $< -o $@
endif


$(O)/ogl_x11.o:  $(SD)hardware/r_opengl/ogl_x11.c $(SD)hardware/r_opengl/r_opengl.h
	$(CC) $(CFLAGS) -shared -nostartfiles -I/usr/X11R6/include -c $< -o $@ 


ifdef USEASM
# Assembly rules
$(O)/tmap.o: $(SD)tmap.nas
	nasm -o $@ -f $(NASMFORMAT) $<

$(O)/vid_copy.o: $(SD)vid_copy.s
	$(CC) $(OPTS) $(OPTINC) $(SFLAGS) -x assembler-with-cpp -c $< -o $@
endif

ifdef UMAPINFO_DIR
# UMAPINFO rules
$(O)/%.o: $(SD)umapinfo/%.c
	$(CC) $(CFLAGS) -Iumapinfo/ -c $< -o $@
endif


# stop searches for re-building source files

*.c :
	@:

*/*.c :
	@:

hardware/*/*.c :
	@:

hardware/*/*/*.c :
	@:

*.h :
	@:

*/*.h :
	@:

hardware/*/*.h :
	@:

hardware/*/*/*.h :
	@:

# Do not remake makefile

Makefile:
	@:

#=======================================================
#[WDJ] Dependencies, replaces makedep.mak, which was unmaintained.

ifndef FORCE_AUTO_DEP
# This PHONY disables automatic dep file generation when a file changes.
# Define FORCE_AUTO_DEP to have automatic dep file generation.
.PHONY : $(DD)/*.dep
endif

.PHONY : depend
depend:  $(DD)/main1.dep $(DD)/main3.dep $(DD)/main4.dep $(DD)/main5.dep \
 $(DD)/main_d.dep $(DD)/main_m.dep \
 $(DD)/main_p1.dep $(DD)/main_p2.dep $(DD)/main_p3.dep $(DD)/main_ph.dep \
 $(DD)/main_r.dep $(DD)/main_t.dep

$(DD)/main1.dep : $(SD)a*.c $(SD)b*.c $(SD)c*.c
$(DD)/main3.dep : $(SD)q*.c $(SD)s*.c
$(DD)/main4.dep : $(SD)f*.c $(SD)g*.c $(SD)h*.c $(SD)i*.c
$(DD)/main5.dep : $(SD)v*.c $(SD)w*.c $(SD)z*.c
$(DD)/main_d.dep : $(SD)d*.c
$(DD)/main_m.dep : $(SD)m*.c
$(DD)/main_p1.dep : $(SD)p_c*.c $(SD)p_d*.c $(SD)p_e*.c $(SD)p_f*.c
$(DD)/main_p2.dep : $(SD)p_g*.c $(SD)p_i*.c $(SD)p_l*.c $(SD)p_m*.c
$(DD)/main_p3.dep : $(SD)p_p*.c $(SD)p_s*.c $(SD)p_t*.c $(SD)p_u*.c
$(DD)/main_ph.dep : $(SD)p_h*.c
$(DD)/main_r.dep : $(SD)r*.c
$(DD)/main_t.dep : $(SD)t*.c
# none: e*.c j*.c k*.c l*.c n*.c o*.c u*.c x*.c y*.c

ifdef INTERFACE
depend: $(DD)/$(INTERFACE).dep
endif
# $(INTERFACE).dep : $(SD)$(INTERFACE)/*.c
$(DD)/djgppdos.dep : $(SD)djgppdos/*.c
$(DD)/linux_x.dep : $(SD)linux_x/*.c $(SD)linux_x/musserv/*.c $(SD)linux_x/sndserv/*.c
$(DD)/os_2.dep : $(SD)os2/*.c
$(DD)/macos.dep : $(SD)macos/*.c
$(DD)/sdl.dep : $(SD)sdl/*.c
$(DD)/win32.dep : $(SD)win32/*.c

ifdef DEP_HARD
depend: $(DD)/hard1.dep
$(DD)/hard1.dep : $(SD)hardware/*.c
endif

ifdef DEP_OPENGL
depend: $(DD)/hardgl.dep
$(DD)/hardgl.dep : $(SD)hardware/r_opengl/r_opengl.c
endif

ifdef DEP_DD
# Direct-draw dependent dep.
depend: $(DD)/hardwin.dep
$(DD)/hardwin.dep : $(SD)hardware/r_d3d/*.cpp
$(DD)/hardwin.dep : $(SD)hardware/s_ds3d/*.c
$(DD)/hardwin.dep : $(SD)hardware/r_glide/*.c
endif

ifdef DEP_MINIGL
depend: $(DD)/hardmini.dep
$(DD)/hardmini.dep : $(SD)hardware/r_minigl/*.c
endif

ifdef DEP_X11
depend: $(DD)/hardx11.dep
$(DD)/hardx11.dep : $(SD)hardware/r_opengl/ogl_x11.c
$(DD)/hardx11.dep : $(SD)hardware/r_opengl/r_opengl.c
endif

ifdef UMAPINFO_DIR
# UMAPINFO
$(DD)/umapinfo.dep : $(SD)umapinfo/*.c
endif

ifdef DOSFILE
# Fixdep adds the obj directory to the dep entries.
# The dep entries use / in MinGW.

ifdef HAVE_MSYS
  # MSYS make cannot handle DOS directories either.
  FIXDEP:=$(BIN)/fixdep.exe
else
  FIXDEP:=$(BIN_WIN)\fixdep.exe
endif

$(FIXDEP):
	$(MAKE) -C ..  fixdep

# [Arcade] Under MSYS the recipes run in sh, not cmd.exe, and there a backslash
# is an escape character rather than a path separator.  A DOS-style path in a
# recipe therefore reaches the program with its separators deleted:
# "> ..\dep\main1.dep" writes a file in src/ literally named "..depmain1.dep",
# and fixdep is then handed "..\dep\main1.dep" and answers "Dep file does not
# exist".  That reads as a broken fixdep or a missing dep directory; it is
# neither -- the dep file was written, just somewhere nobody looked.  The MSYS
# case gets its own branch using POSIX paths throughout.  (The MINGW branch
# below is half-converted already: it redirects to $(DD)/ but still passes
# $(DD_WIN)\ to fixdep, so it has the same fault.)
%.dep : | $(FIXDEP)
  ifdef HAVE_MSYS
	@echo "Making MSYS dependencies $(@F)"
	$(CC) $(CFLAGS) -MM $^ > $(DD)/$(@F)
	$(FIXDEP)  $(O)/  $(DD)/$(@F)
  else ifeq ($(CC_SELECT), MINGW)
	@echo "Making MINGW dependencies $(@F)"
   ifdef HAVE_DOSCOMMAND
	$(CC) $(CFLAGS) -MM $^ > $(DD_WIN)\$(@F)
   else
	$(CC) $(CFLAGS) -MM $^ > $(DD)/$(@F)
   endif
	$(FIXDEP)  $(O)/  $(DD_WIN)\$(@F)
  else
	@echo "Making WIN dependencies $(@F)"
	$(CC) $(CFLAGS) -MM $^ > $(DD_WIN)\$(@F)
	$(FIXDEP)  $(O_WIN)\  $(DD_WIN)\$(@F)
  endif

else
# Linux uses sed to add the obj directory to the dep entries.

%.dep :
	@echo "Making dependencies $(@F)"
	$(CC) $(CFLAGS) $(SND_DEVS) -MM $^ > $(DD)/$(@F)
	sed -e "s!^[a-zA-Z0-9_]*\.o:!\$$\(O)/&!" $(DD)/$(@F) > $(DD)/sed.dep
	mv $(DD)/sed.dep $(DD)/$(@F)

endif


# Stop update of include files when clean
ifdef MAKE_OPTIONS_PRESENT
ifdef INCLUDE_DEPS
include $(DD)/main1.dep
include $(DD)/main3.dep
include $(DD)/main4.dep
include $(DD)/main5.dep
include $(DD)/main_d.dep
include $(DD)/main_p1.dep
include $(DD)/main_p2.dep
include $(DD)/main_p3.dep
include $(DD)/main_ph.dep
include $(DD)/main_m.dep
include $(DD)/main_r.dep
include $(DD)/main_t.dep
include $(DD)/hard1.dep
ifdef INTERFACE
 include $(DD)/$(INTERFACE).dep
endif
ifdef DEP_OPENGL
 include $(DD)/hardgl.dep
endif
ifdef DEP_DD
 include $(DD)/hardwin.dep
endif
ifdef DEP_MINIGL
 include $(DD)/hardmini.dep
endif
ifdef DEP_X11
 include $(DD)/hardx11.dep
endif
ifdef UMAPINFO_DIR
include $(DD)/umapinfo.dep
endif
endif
endif

#=======================================================
# Install binaries

INSTALL_FILES:=$(addprefix $(BIN)/, $(EXENAME) $(INSTALL_SUPPL))
ifdef DOSFILE
  ifdef HAVE_DOSCOMMAND
    RMFILE ?= del
  else
    RMFILE ?= rm -v
  endif
  INSTALL_FILES_WIN:=$(subst /,\,$(INSTALL_FILES))
else
  RMFILE ?= rm -I -v
endif

INSTALL ?= install
export INSTALL

install:
ifdef HAVE_DOSCOMMAND
	xcopy $(INSTALL_FILES_WIN) $(INSTALL_DIR) /S /E
else
	$(INSTALL) $(INSTALL_OPTS) -t $(INSTALL_DIR)  $(INSTALL_FILES)
endif
ifdef MUSSERV
	cd $(SD)$(MUSSERV) && $(MAKE) install INSTALL_DIR=$(abspath $(INSTALL_DIR))
endif
ifdef SNDSERV
	cd $(SD)$(SNDSERV) && $(MAKE) install INSTALL_DIR=$(abspath $(INSTALL_DIR))
endif

uninstall:
ifdef HAVE_DOSCOMMAND
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/$(EXENAME))
  ifdef INSTALL_SUPPL
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/$(INSTALL_SUPPL))
  endif
else
	- cd $(INSTALL_DIR) && $(RMFILE) $(notdir $(INSTALL_FILES))
endif
ifdef MUSSERV
	cd $(SD)$(MUSSERV) && $(MAKE) uninstall INSTALL_DIR=$(abspath $(INSTALL_DIR))
endif
ifdef SNDSERV
	cd $(SD)$(SNDSERV) && $(MAKE) uninstall INSTALL_DIR=$(abspath $(INSTALL_DIR))
endif


#############################################################
#
#############################################################
