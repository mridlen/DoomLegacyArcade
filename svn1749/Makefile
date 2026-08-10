#############################################################################
#
# $Id: Makefile 1448 2019-07-21 01:30:31Z wesleyjohnson $
# GNU Make makefile for Doom Legacy, Main Makefile
#
# Copyright (C) 1998-2015 by Doom Legacy Team.
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
# On first execution of this Makefile, it will copy a make_options file
# and create the build dirs.
#
# IMPORTANT: Edit the make_options file, choose OS and SMIF, and
# put ALL your other options in there too.
# This is the easiest way to use this Makefile, with the least mistakes.
# Otherwise you must include the OS, and maybe the SMIF,
# with almost EVERY make command.
#   >> make clean OS=DOS
#
# The make_options file should be edited to select OS, SMIF, and options.
# Once make_options is edited to specify OS= and SMIF= (and other options)
# they do not need to be included on the make command line anymore.
# The make_options settings are overridden by make command line options
# (like DEBUG=1 or HAVE_MIXER=1).
#
# Command line options have precedence over the make_options file.
# To have values in the make_options file override those on the make
# command line, use override in the make_options file:
#   override SMIF=SDL
#
# To use another make_options file, invoke make with MAKE_OPTIONS.
#   >> make MAKE_OPTIONS=my_other_options
#
# Specify a build directory with its own make_options file, objs, and bin.
#   >> make BUILD=x11
#   >> make clean BUILD=x11
#
# Invoke make with SRC= to compile other development directories.
#   >> make BUILD=xxx SRC=xxx1b
#
# When a make_options file exists before first compilation, you may have
# to create build directories manually.
# The OS is set in the make_options file, so it is not needed on the command.
#   >> make dirs
#
# The preferred system media interface is SDL.
# To directly compile the generic Linux SDL version (with sound):
#    >> make OS=LINUX SMIF=SDL HAVE_MIXER=1
#
# Each of the initial make_options files will also have other SMIF choices and
# other options for that platform.
# Edit make_options and compile the executable:
#    >> make
#
# To generate a debugging executable, specify DEBUG.
# You can also build debug in a separate build directory.
#   >> make DEBUG=1 BUILD=debug
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
# This makefile also includes a standard install.
# To get directions for install:
#   >> make install
#
# commands:
#  all  :  compile executable (default)
#  dirs : create dep, objs, bin directories
#  clean : delete objs and bin
#  distclean : delete objs, bin, and make_options
#  depend : update dependencies
#  install : install directions
#  install_sys : install to share and system directories
#  install_sharegames : install to share and games directories
#  install_games : install to a games directory
#  install_user : install to user
#  uninstall : uninstall directions
#  uninstall_sys : uninstall from share and system directories
#  uninstall_sharegames : uninstall from share and games directories
#  uninstall_games : install from a games directory
#  uninstall_user : install from user
# maintenance commands (generally not usable to the end user):
#  legacywad : legacy.wad missing message
#  wad_update : update existing legacy.wad
#  wad_extract: extract an existing legacy.wad to WAD directory
#  wad_create : create a legacy.wad from the WAD directory (not supplied)
#  tools : make some odd tools
#  fixdep: program to fix dep files
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
#   OS=NETBSD
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

CC_LIST:=GCC MINGW WATCOM CLANG

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

MUS_OS_LIST= EMPTY SCOOS5 SCOUW2 SCOUW7
# openserver5
#   MUS_OS=SCOOS5
# unixware2
#   MUS_OS=SCOUW2
# unixware7
#   MUS_OS=SCOUW7
# ESD demon for X11 (esound), not needed for SDL.
#   HAVE_ESD=1

# GGI option on X11
# X11_GGI=1
# Uncomment if you want to use the POLL_POINTER hack in X11
# POLL_POINTER=-DPOLL_POINTER

# WIN_NATIVE options
# For FMOD sound.  Can compile without FMOD.
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

# The MSYS make does not understand undefine,
# Using undefine makes it fail with message "*** Missing Separator".
# Recommend use mingw32-make.

# Warning flags
# WFLAGS=-Wall -Wwrite-strings

# Optimization level, -O0 to -O5
# OPTLEV=-O3

# Enable the x86 asm code (which is not always up to date).
# This is only useful if have an old compiler with bad optimization.
# USEASM=1

# Developers with svn can enable this to have svn revision number in executable.
# Causes compile error message otherwise.
# Until can find test for presence of svn, this is best that can be done.
# SVN_ENABLE=1

# Invoke make with SRC= to compile other development directories.
# This is always invoked from outside the src directory.
ifndef SRC
  # Default source directory.
  SRC=src
endif
ifeq ("$(SRC)","")
  $(error "SRC directory cannot be blank."
endif
ifeq ("$(SRC)",".")
  $(error "SRC directory cannot be the current directory."
endif

# Invoke make with BUILD= to compile to other build directories.
ifndef BUILD
  BUILD:=.
endif
ifeq ("$(BUILD)",".")
  # BUILD== "." is the main directory.
  BUILD:=
endif
ifeq ("$(BUILD)","")
  # BUILD== "" is the main directory.
    BUILD_DIR:=
    # export this to tools, src
    MAIN_BUILD_DIR:=../
else
  # BUILD in a sub-directory.
    BUILD_DIR:=$(BUILD)/
    # export this to tools, src
    ifneq ("$(findstring %%%/, %%%$(subst \,/,$(BUILD)))","")
      # Absolute path
      MAIN_BUILD_DIR:=$(BUILD_DIR)
    else
      # Relative path is more common, so avoid making it absolute.
      MAIN_BUILD_DIR:=../$(BUILD_DIR)
    endif
endif
export MAIN_BUILD_DIR

# Subdirectories for binaries and build intermediates
# DO NOT export these dirs.
BIN = $(BUILD_DIR)bin
O  = $(BUILD_DIR)objs
DD = $(BUILD_DIR)dep

# Invoke make with MAKE_OPTIONS= to specify another file or location.
ifndef MAKE_OPTIONS
  MAKE_OPTIONS = $(BUILD_DIR)make_options
endif

# Put user settings in this file, and they will be included with every
# invocation of make.
-include $(MAKE_OPTIONS)

ifdef DEBUG
  # Allow DEBUG=0 to turn debugging off.
  ifeq ("$(DEBUG)","1")
    export DEBUG
    ifdef DEBUGFLAGS
      export DEBUGFLAGS
    endif
  endif
endif
# End of DEBUG

# End of User tunable settings


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
    MINGW_MAKE=1
  endif
  ifeq ($(CC_SELECT), WATCOM)
    CC_CMD:=WATCOMC
  endif
  ifeq ($(CC_SELECT), CLANG)
    CC_CMD:=clang
  endif
endif
ifdef CC_ENVIRONMENT
  # Give an environment CC precedence in specifing the compiler.
  CC ?= $(CC_CMD)
else
  CC:=$(CC_CMD)
endif
export CC
ifneq ("$(findstring mingw, $(MAKE))","")
  MINGW_MAKE=1
endif

SMIF_LIST:= SDL LINUX_X11 FREEBSD_X11 WIN_NATIVE OS2_NATIVE DOS_NATIVE

# GNU Make 3.79 of djgpp does not allow else if
# Default system media interface
ifdef SMIF
  # specific SMIF
  EXT_SMIF:=$(SMIF)
else
  SMIF:=SDL
endif

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
    OS:=LINUX
  endif
else
ifeq ($(SMIF), FREEBSD_X11)
  ifdef OS
    ifneq ($(OS), FREEBSD)
      $(error "SMIF=FREEBSD_X11 requires OS=FREEBSD")
    endif
  else
    OS:=FREEBSD
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
    OS:=WIN32
  endif
else
ifeq ($(SMIF), OS2_NATIVE)
  ifdef OS
    ifneq ($(OS), OS2)
      $(error "SMIF=OS2_NATIVE requires OS=OS2")
    endif
  else
    OS:=OS2
  endif
else
ifeq ($(SMIF), DJGPPDOS_NATIVE)
  ifdef OS
    ifneq ($(OS), DOS)
      $(error "SMIF=DJGPPDOS_NATIVE requires OS=DOS")
    endif
  else
    OS:=DOS
  endif
else
  # SMIF is not error in Main Makefile.  It is in the src Makefile.
  # $(error  "Unknown SMIF: $(SMIF)" )
endif
endif
endif
endif
endif
endif
export SMIF


OS_LIST:=LINUX FREEBSD NETBSD WIN32 WIN98 WIN7 OS2 DOS MAC

ifeq ($(OS), LINUX)
  LINUX=1
else
ifeq ($(OS), FREEBSD)
  # Has some linking differences from the rest of Linux-like OS.
  $(info  FreeBSD support is dependent upon user reports.)
  $(info  Please submit bug reports, and bug fixes. )
  FREEBSD=1
else
ifeq ($(OS), NETBSD)
  # Uses statvfs(5).
  $(info  NetBSD support is dependent upon user reports.)
  $(info  Please submit bug reports, and bug fixes. )
  NETBSD=1
else
ifeq ($(OS), WIN32)
  $(info  Generic Windows compile.  May or may-not work on modern Windows.)
  $(info  Please submit bug reports, and bug fixes. )
  WIN32=1
  DOSFILE=1
else
ifeq ($(OS), WIN7)
  $(warning  Does  NOT YET  make a version customized to Win7/8.)
  $(info  Needs a Win7/8 developer to test and debug.)
  $(info  Please submit bug report of success or failure, and bug fixes. )
  WIN32=1
  DOSFILE=1
else
ifeq ($(OS), WIN98)
  # One of the development machines is Win98.
  # If Win98 differences occur, this category can isolate them.
  $(info  Win98 Windows compile.  May or may-not work on more modern Windows.)
  WIN32=1
  DOSFILE=1
else
ifeq ($(OS), OS2)
  $(info  OS2 code support is dependent upon user reports.)
  $(warning  OS2 code has not been verified in a long time. )
  $(info  Please submit bug report of success or failure, and bug fixes. )
  OS2=1
  DOSFILE=1
else
ifeq ($(OS), DOS)
  DOS=1
  DOSFILE=1
else
ifeq ($(OS), MAC)
  $(info  MAC code support is dependent upon user reports.)
  $(warning  MAC port code has not been verified in a long time. )
  $(warning  MAC SDL code has known compile failures. )
  $(info  Needs a MAC developer to test and debug.)
  $(info  Please submit bug report of success or failure, and bug fixes. )
  MAC=1
else
  ifdef MAKE_OPTIONS_PRESENT
    $(error  Unknown OS: $(OS) )
  else
    # Mangles the make_options if have wrong OS.
    $(error  Need OS=LINUX or choice from $(OS_LIST) to initialize make_options properly.)
  endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
export OS


# DOS/OS2/WIN32 file differences
export DOSFILE
ifdef DOSFILE
  # DOS, OS2, Win32
  # MSYS shell interferes with DOS commands.
  # Setting SHELL = command.com DOES NOT WORK.
  # Set SHELL = cmd.exe appears to work for some systems.
  ifndef HAVE_DOSCOMMAND
    ifneq ("$(findstring "MSYS", "$(SHELL)"),"")
      HAVE_MSYS=1
      export HAVE_MSYS
      RM:=rm
    endif
  endif
  ifndef HAVE_MSYS
    HAVE_DOSCOMMAND=1
    DEL:=del
  endif
  BIN_WIN:=$(subst /,\,$(BIN))
  O_WIN:=$(subst /,\,$(O))
  DD_WIN:=$(subst /,\,$(DD))
else
  # Linux, FreeBSD, NETBSD, Mac
  RM:=rm
  SHELL:=/bin/sh
endif


#=======================================================
# Commands
#=======================================================
# Do not have directories as prerequisites because their date is updated
# whenever a file is output, which fouls up the rules.

.PHONY : all clean distclean dirs asm dll

# Prevent compile without make_options file
ifdef MAKE_OPTIONS_PRESENT
all: | $(O)
	$(MAKE) -C $(SRC)  all
endif


# Directly using $(error) prevents the output of the other commands.

.PHONY : init
init: $(MAKE_OPTIONS)
	$(error "Stop to EDIT make_options.")
	@false
	@stop # stop make, by error if necessary. see IRIX make.


# Win32/OS2/DOS sees "/" as a switch, "\" is an escape to make
dirs: $(O)
$(O):
ifdef OS
ifdef HAVE_DOSCOMMAND
	@echo "Making dirs (WIN32/OS2/DOS)"
	mkdir "$(BIN_WIN)"
	mkdir "$(O_WIN)"
	mkdir "$(DD_WIN)"
else
ifdef HAVE_MSYS
	@echo "Making dirs (MSYS)"
	- mkdir  $(BIN)
	- mkdir  $(O)
	- mkdir  $(DD)
else
	@echo "Making dirs (Linux/BSD)"
	mkdir -p $(BIN)
	mkdir -p $(O)
	mkdir -p $(DD)
endif
endif
else
	@echo Could not make dirs, do not know OS.
	@echo If do not have dep, bin, and objs dirs, then make dirs.
endif


# Win32/OS2/DOS sees "/" as a switch, "\" is an escape to make
clean:
	$(MAKE) -C $(SRC)  clean


# As all intermediates are kept outside of dist, there is nothing extra to do
# except to remove make_options
distclean: clean
ifdef HAVE_DOSCOMMAND
	- $(DEL) $(MAKE_OPTIONS)
else
	- $(RM) $(MAKE_OPTIONS)
endif
	$(MAKE) -C $(SRC)  distclean


.PHONY : versionstring
# This may fail because, (a) svn not installed, (b) not a svn directory.
# This compiles d_main a second time, with SVN_REV set.
versionstring:
	$(MAKE) -C $(SRC)  versionstring

#=======================================================
# Maintenance Functions, mostly Linux only


# Make a disasm of the exe
disasm:
	$(MAKE) -C $(SRC)  disasm

# To dump draw functions to assembly
objdump_draw:
	$(MAKE) -C $(SRC)  objdump_draw

# Make dll for X11
dll:
	$(MAKE) -C $(SRC)  dll


#=======================================================
# Tools
ifdef TOOLS
  # Externally specified resources dir
  TOOLS_DIR:=TOOLS
else
  ifeq ("$(SRC)","src")
    TOOLS_DIR:=tools
  else
    # Some other source
    TOOLS_DIR:=$(SRC)/../tools
  endif
endif

tools:
	$(MAKE) -C $(TOOLS_DIR)

wadtool:
	$(MAKE) -C $(TOOLS_DIR)  wadtool

fixdep:
	$(MAKE) -C $(TOOLS_DIR)  fixdep

dircomp2:
	$(MAKE) -C $(TOOLS_DIR)  dircomp2

convert:
	$(MAKE) -C $(TOOLS_DIR)  convert

#=======================================================
.PHONY : legacywad wad_update wad_extract wad_create

# Resources
ifdef RESOURCES
  # Externally specified resources dir
  RES_DIR:=RESOURCES
else
  ifeq ("$(SRC)","src")
    RES_DIR:=resources
  else
    # Some other source
    RES_DIR:=$(SRC)/../resources
  endif
endif

#export DEUTEX DOOMDIR DOOMWAD

# Update the legacy.wad with special lumps from resources dir.
# Some of these lumps cannot be inserted by other tools due to limitations.
wad_update :
	$(MAKE) -C $(RES_DIR)  wad_update  WAD=$(WAD)

# Extract the legacy.wad into the wad directory.
wad_extract :
	$(MAKE) -C $(RES_DIR)  wad_extract WAD=$(WAD)

# Create the legacy.wad from wad directory (not supplied).
wad_create :
	$(MAKE) -C $(RES_DIR)  wad_create


# Missing legacy.wad
legacywad :
	@echo "-- WARNING --"
	@echo "If you are missing the legacy.wad, or have an old legacy.wad,"
	@echo "please download the new version in the legacy_common download."
	@echo " "
	@echo "This is not the way to create a legacy.wad file.  This MAKE can only"
	@echo "alter an existing legacy.wad to conform to the new executable."
	@echo " "
	@echo "You will need an old legacy.wad file, and wadtool."
	@echo "The contents of the resources directory will be inserted."
	@echo "Usage: make wad_update WAD=old_legacy.wad"


#=======================================================
# Dependencies

.PHONY : depend
depend:
	$(MAKE) -C $(SRC) depend


#=======================================================
.PHONY : install install_sys install_games install_user


ifdef DOSFILE
  ifdef HAVE_DOSCOMMAND
    RMFILE:= del
  else
    RMFILE:= rm -v
  endif

  ifdef HAVE_MSYS
    # MSYS defines HOME for itself.
    USERHOME:=$(USERPROFILE)
  else
    USERHOME:=$(HOME)
  endif
endif


# Specify INSTALL_DIR= to override.
ifdef INSTALL_DIR
  INSTALL_SYS_DIR:=$(INSTALL_DIR)
  INSTALL_SHARE_DIR:=$(INSTALL_DIR)
  INSTALL_GAMES_DIR:=$(INSTALL_DIR)
  INSTALL_USER_DIR:=$(INSTALL_DIR)
endif
ifdef DOS
  PREFIX ?= \games
  INSTALL_SYS_DIR ?= $(PREFIX)\legacy
  INSTALL_SHARE_DIR ?= $(PREFIX)\legacy
  INSTALL_GAMES_DIR ?= \games\legacy
  INSTALL_USER_DIR ?= $(USERHOME)\games\legacy
else
ifdef DOSFILE
  # WIN32, OS2
  # The quotes are needed for names with spaces. They show up in the final cmd.
  PREFIX ?= "c:\Program Files"
  INSTALL_SYS_DIR ?= $(PREFIX)\doomlegacy
  INSTALL_SHARE_DIR ?= $(PREFIX)\doomlegacy
  INSTALL_GAMES_DIR ?= \games\doomlegacy
  INSTALL_USER_DIR ?= $(USERHOME)\games\doomlegacy
else
  # Linux, FreeBSD, NETBSD, Mac
  # DO NOT use tilde, use $(HOME).
  PREFIX ?= /usr/local
  INSTALL_SYS_DIR ?= $(PREFIX)/bin
  INSTALL_SHARE_DIR ?= $(PREFIX)/share/games/doomlegacy
  INSTALL_GAMES_DIR ?= $(PREFIX)/games/doomlegacy
  INSTALL_USER_DIR ?= $(HOME)/games/doomlegacy
  GROUP ?= games
  OWNER ?= games
  CHOWN_OPTS:= $(GROUP):$(OWNER)
  INSTALL_SYS_OPTS:= -v
  ifneq ("$(GROUP)","")
    INSTALL_SYS_OPTS+= --group=$(GROUP)
  endif
  ifneq ("$(OWNER)","")
    INSTALL_SYS_OPTS+= --owner=$(OWNER)
  endif
  INSTALL ?= install -C
  RMFILE:= rm -I -v
  export INSTALL
endif
endif
export RMFILE

# Install legacy.wad to:
INSTALL_LEGACYWAD_DIR ?= $(INSTALL_SHARE_DIR)
# from:
LEGACYWAD_INSDIR:=install/

# Find a legacy.wad to install
ifdef LEGACY_WAD
  # Specified legacy.wad.
  ifeq ("$(wildcard $(LEGACY_WAD))","")
    $(error Invalid LEGACY_WAD= $(LEGACY_WAD))
  endif
else
  # Search for legacy.wad
  LEGACY_WAD:=$(wildcard $(BIN)/legacy.wad)
  ifeq ("$(LEGACY_WAD)","")
    LEGACY_WAD:=$(wildcard $(LEGACYWAD_INSDIR)legacy.wad)
    ifeq ("$(LEGACY_WAD)","")
#     MSYS make does not understand undefine, but assign empty does the same.
       LEGACY_WAD=
#      undefine LEGACY_WAD
#      ifeq ("$(wildcard $(INSTALL_LEGACYWAD_DIR)/legacy.wad)","")
#        $(info "No legacy.wad: not found in bin/, nor in $(LEGACYWAD_INSDIR) .")
#      endif
    endif
  endif
endif

# Specify where legacy_common download was unpacked with COMMON_DIR=
ifdef COMMON_DIR
  ifndef LEGACY_WAD
    ifneq ("$(wildcard $(COMMON_DIR)/legacy.wad)","")
      LEGACY_WAD:=$(wildcard $(COMMON_DIR)/legacy.wad)
    endif
  endif
endif


# Install
install :
	@echo Choices for install.
	@echo make install_sys
	@echo _ System wide, bin and share directories, requires system privileges
	@echo _ INSTALL_SYS_DIR= $(INSTALL_SYS_DIR)
	@echo _ INSTALL_SHARE_DIR= $(INSTALL_SHARE_DIR)
	@echo make install_sharegames
	@echo _ Games and share directories, may require system privileges.
	@echo _ INSTALL_GAMES_DIR= $(INSTALL_GAMES_DIR)
	@echo _ INSTALL_SHARE_DIR= $(INSTALL_SHARE_DIR)
	@echo make install_games
	@echo _ Games directory, may require system privileges.
	@echo _ INSTALL_GAMES_DIR= $(INSTALL_GAMES_DIR)
	@echo make install_user
	@echo _ Into a user directory, owned by current user.
	@echo _ INSTALL_USER_DIR= $(INSTALL_USER_DIR).
	@echo Use INSTALL_DIR= to choose another target location.
	@echo The install directory can be moved after install.
	@echo Doom Legacy is not configured as to the install location.
	@echo You will also have to install the common package which has legacy.wad.
	@echo This install will attempt to install legacy.wad, if it can find it.
	@echo You can make the directory install/ and put legacy.wad there.
	@echo Use COMMON_DIR= to look in the unpacked legacy_common for legacy.wad.
	@echo _ COMMON_DIR= $(COMMON_DIR)
	@echo Must install legacy.wad to a directory from the doomdefs.h config.
	@echo _ INSTALL_LEGACYWAD_DIR= $(INSTALL_LEGACYWAD_DIR)
	@echo This may constrain your choice of INSTALL_SHARE_DIR.
	@echo To change the directories searched, you must edit doomdefs.h.
	@echo This install does NOT modify nor configure DoomLegacy.

# Install into system bin and share directories.
install_sys:
	@echo Installing DoomLegacy to System
	@echo Installing docs to $(INSTALL_SHARE_DIR)
ifdef DOSFILE
	@- mkdir $(INSTALL_SHARE_DIR)
  ifdef HAVE_DOSCOMMAND
	xcopy $(subst /,\,docs $(INSTALL_SHARE_DIR)/docs) /S /E /I
  else
	- cp -a -u docs $(INSTALL_SHARE_DIR)/docs
  endif
  ifdef LEGACY_WAD
	@- mkdir $(INSTALL_LEGACYWAD_DIR)
    ifdef HAVE_DOSCOMMAND
	- xcopy $(LEGACY_WAD) $(INSTALL_LEGACYWAD_DIR)
    else
	- cp -a -u $(LEGACY_WAD) $(INSTALL_LEGACYWAD_DIR)
    endif
  endif
	@echo Installing to $(INSTALL_SYS_DIR).
	$(MAKE) -C $(SRC) install INSTALL_DIR=$(INSTALL_SYS_DIR)
else
  ifeq ("$(wildcard $(INSTALL_SHARE_DIR))","")
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_SHARE_DIR)
  endif
	- cp -a -u docs $(INSTALL_SHARE_DIR)
  ifneq ("$(CHOWN_OPTS)","")
	-chown -R $(CHOWN_OPTS) $(INSTALL_SHARE_DIR)/docs
  endif
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_SHARE_DIR)/wads
  ifdef LEGACY_WAD
    ifneq ("$(INSTALL_LEGACYWAD_DIR)","$(INSTALL_SHARE_DIR)")
      ifeq ("$(wildcard $(INSTALL_LEGACYWAD_DIR))","")
	- $(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_LEGACYWAD_DIR)
      endif
    endif
	$(INSTALL) $(INSTALL_SYS_OPTS) -t $(INSTALL_LEGACYWAD_DIR)  $(LEGACY_WAD)
  endif
	@echo Installing to $(INSTALL_SYS_DIR).
  ifeq ("$(wildcard $(INSTALL_SYS_DIR))","")
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_SYS_DIR)
  endif
	$(MAKE) -C $(SRC) install INSTALL_DIR=$(INSTALL_SYS_DIR) INSTALL_OPTS="$(INSTALL_SYS_OPTS)"
endif


# Install into games and share directories.
ifdef DOSFILE
# DOS/Windows does not have a Share and Games install.
install_sharegames: install_games
else
install_sharegames:
	@echo Installing DoomLegacy to Share and Games
	@echo Installing docs to $(INSTALL_SHARE_DIR)
  ifeq ("$(wildcard $(INSTALL_SHARE_DIR))","")
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_SHARE_DIR)
  endif
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_SHARE_DIR)/wads
	- cp -a -u docs $(INSTALL_SHARE_DIR)
  ifneq ("$(CHOWN_OPTS)","")
	-chown -R $(CHOWN_OPTS) $(INSTALL_SHARE_DIR)/docs
  endif
  ifdef LEGACY_WAD
    ifneq ("$(INSTALL_LEGACYWAD_DIR)","$(INSTALL_SHARE_DIR)")
      ifeq ("$(wildcard $(INSTALL_LEGACYWAD_DIR))","")
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_LEGACYWAD_DIR)
      endif
    endif
	- $(INSTALL) $(INSTALL_SYS_OPTS) -t $(INSTALL_LEGACYWAD_DIR)  $(LEGACY_WAD)
  endif
	@echo Installing to $(INSTALL_GAMES_DIR).
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_GAMES_DIR)
	$(MAKE) -C $(SRC) install INSTALL_DIR=$(INSTALL_GAMES_DIR) INSTALL_OPTS="$(INSTALL_SYS_OPTS)"
endif


# Install into a games directory.
install_games:
	@echo Installing DoomLegacy to Games
ifdef DOSFILE
	@echo Installing to $(INSTALL_GAMES_DIR).
	@- mkdir $(INSTALL_GAMES_DIR)
  ifdef HAVE_DOSCOMMAND
	- xcopy $(subst /,\,docs $(INSTALL_GAMES_DIR)/docs) /S /E /I
  else
	- cp -a -u docs $(INSTALL_GAMES_DIR)/docs
  endif
  ifdef LEGACY_WAD
    ifdef HAVE_DOSCOMMAND
	- xcopy $(LEGACY_WAD) $(INSTALL_GAMES_DIR)
    else
	- cp -u $(LEGACY_WAD) $(INSTALL_GAMES_DIR)
    endif
  endif
	$(MAKE) -C $(SRC) install INSTALL_DIR=$(INSTALL_GAMES_DIR)
else
	@echo Installing docs to $(INSTALL_GAMES_DIR)
  ifeq ("$(wildcard $(INSTALL_GAMES_DIR))","")
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_GAMES_DIR)
  endif
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_GAMES_DIR)/wads
	- cp -a -u docs $(INSTALL_GAMES_DIR)
  ifneq ("$(CHOWN_OPTS)","")
	-chown -R $(CHOWN_OPTS) $(INSTALL_GAMES_DIR)/docs
  endif
  ifdef LEGACY_WAD
    ifneq ("$(INSTALL_LEGACYWAD_DIR)","$(INSTALL_GAMES_DIR)")
      ifeq ("$(wildcard $(INSTALL_LEGACYWAD_DIR))","")
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_LEGACYWAD_DIR)
      endif
    endif
	- $(INSTALL) $(INSTALL_SYS_OPTS) -t $(INSTALL_LEGACYWAD_DIR)  $(LEGACY_WAD)
  endif
	@echo Installing to $(INSTALL_GAMES_DIR).
	$(INSTALL) $(INSTALL_SYS_OPTS) -d $(INSTALL_GAMES_DIR)
	$(MAKE) -C $(SRC) install INSTALL_DIR=$(INSTALL_GAMES_DIR) INSTALL_OPTS="$(INSTALL_SYS_OPTS)"
endif


# Install in a user location.
install_user:
	@echo Installing DoomLegacy to $(INSTALL_USER_DIR)
ifdef DOSFILE
	@- mkdir $(INSTALL_USER_DIR)
  ifdef HAVE_DOSCOMMAND
	xcopy $(subst /,\,docs $(INSTALL_USER_DIR)/docs) /S /E /I
  else
	cp -a -u docs $(INSTALL_USER_DIR)/docs
  endif
  ifdef LEGACY_WAD
    ifdef HAVE_DOSCOMMAND
	xcopy $(LEGACY_WAD) $(INSTALL_USER_DIR)
    else
	cp -u $(LEGACY_WAD) $(INSTALL_USER_DIR)
    endif
  endif
else
	mkdir -p $(INSTALL_USER_DIR)
	@echo Hard link of docs files \(shared files\) instead of copy.
	@echo Delete of source dir is OK, but user modify of docs files will leak.
	- cp -a --link docs $(INSTALL_USER_DIR)
	mkdir -p $(INSTALL_USER_DIR)/wads
  ifdef LEGACY_WAD
	cp -v -u $(LEGACY_WAD) $(INSTALL_USER_DIR)/wads
  endif
endif
	$(MAKE) -C $(SRC) install INSTALL_DIR=$(abspath $(INSTALL_USER_DIR))


# Uninstall
uninstall :
	@echo Choices for uninstall.
	@echo Need to select same install and directories as when installed.
	@echo Can also just delete the installed files and doomlegacy directories.
	@echo make uninstall_sys
	@echo _ System wide, bin and share directories, requires system privileges
	@echo _ INSTALL_SYS_DIR= $(INSTALL_SYS_DIR)
	@echo _ INSTALL_SHARE_DIR= $(INSTALL_SHARE_DIR)
	@echo make uninstall_sharegames
	@echo _ Games and share directories, may require system privileges.
	@echo _ INSTALL_GAMES_DIR= $(INSTALL_GAMES_DIR)
	@echo _ INSTALL_SHARE_DIR= $(INSTALL_SHARE_DIR)
	@echo make uninstall_games
	@echo _ Games directory, may require system privileges.
	@echo _ INSTALL_GAMES_DIR= $(INSTALL_GAMES_DIR)
	@echo make uninstall_user
	@echo _ A user directory, owned by current user.
	@echo _ INSTALL_USER_DIR= $(INSTALL_USER_DIR).
	@echo INSTALL_DIR= to choose another target location.

ifneq ("$(findstring *, $(INSTALL_SYS_DIR))","")
  $(error Wildcard * in INSTALL_SYS_DIR)
endif
ifneq ("$(findstring *, $(INSTALL_SHARE_DIR))","")
  $(error Wildcard * in INSTALL_SHARE_DIR)
endif
ifneq ("$(findstring *, $(INSTALL_USER_DIR))","")
  $(error Wildcard * in INSTALL_USER_DIR)
endif

# Uninstall from a system program directory.
uninstall_sys:
	@echo Uninstalling DoomLegacy from $(INSTALL_SYS_DIR)
	$(MAKE) -C $(SRC) uninstall INSTALL_DIR=$(INSTALL_SYS_DIR)
	@echo Uninstalling DoomLegacy from $(INSTALL_SHARE_DIR).
ifdef HAVE_DOSCOMMAND
	- $(RMFILE) $(subst /,\,$(INSTALL_SHARE_DIR)/legacy.wad )
	$(MAKE) del_docs_dir INSTALL_DIR=$(INSTALL_SHARE_DIR)
else
	- cd $(INSTALL_SHARE_DIR) && $(RMFILE) legacy.wad
	- cd $(INSTALL_SHARE_DIR) && $(RMFILE) -R docs
endif


ifdef DOSFILE
# DOS/Windows does not have a Share and Games install.
uninstall_sharegames: uninstall_games
else
uninstall_sharegames:
	@echo Uninstalling DoomLegacy from $(INSTALL_GAMES_DIR)
	$(MAKE) -C $(SRC) uninstall INSTALL_DIR=$(INSTALL_GAMES_DIR)
ifdef HAVE_DOSCOMMAND
	- $(RMFILE) $(subst /,\,$(INSTALL_GAMES_DIR)/legacy.wad )
	$(MAKE) delete_docs_dir INSTALL_DIR=$(INSTALL_GAMES_DIR)
	- rmdir $(INSTALL_GAMES_DIR)
else
	@echo Uninstalling DoomLegacy from $(INSTALL_SHARE_DIR).
	- cd $(INSTALL_SHARE_DIR) && $(RMFILE) legacy.wad
	- cd $(INSTALL_SHARE_DIR) && $(RMFILE) -R docs
	- rmdir $(INSTALL_SHARE_DIR)/wads
	- rmdir $(INSTALL_SHARE_DIR)
	- rmdir $(INSTALL_GAMES_DIR)
endif
endif


# Uninstall from a games directory.
uninstall_games:
	@echo Uninstalling DoomLegacy from $(INSTALL_GAMES_DIR)
	$(MAKE) -C $(SRC) uninstall INSTALL_DIR=$(INSTALL_GAMES_DIR)
ifdef HAVE_DOSCOMMAND
	- $(RMFILE) $(subst /,\,$(INSTALL_GAMES_DIR)/legacy.wad )
	$(MAKE) delete_docs_dir INSTALL_DIR=$(INSTALL_GAMES_DIR)
	- rmdir $(INSTALL_GAMES_DIR)
else
	@echo Uninstalling DoomLegacy from $(INSTALL_SHARE_DIR).
	- cd $(INSTALL_GAMES_DIR) && $(RMFILE) legacy.wad
	- cd $(INSTALL_GAMES_DIR) && $(RMFILE) -R docs
	- rmdir $(INSTALL_GAMES_DIR)/wads
	- rmdir $(INSTALL_GAMES_DIR)
endif


# Uninstall from a user location.
uninstall_user:
	@echo Uninstalling DoomLegacy from $(INSTALL_USER_DIR)
	$(MAKE) -C $(SRC) uninstall INSTALL_DIR=$(abspath $(INSTALL_USER_DIR))
ifdef HAVE_DOSCOMMAND
	- $(RMFILE) $(subst /,\, $(INSTALL_USER_DIR)/legacy.wad )
	$(MAKE) del_docs_dir INSTALL_DIR=$(INSTALL_USER_DIR)
	- rmdir $(INSTALL_USER_DIR)/wads
	- rmdir $(INSTALL_USER_DIR)
else
	- cd $(INSTALL_USER_DIR) && $(RMFILE) legacy.wad
	- cd $(INSTALL_USER_DIR) && $(RMFILE) -R docs
	- rmdir $(INSTALL_USER_DIR)/wads
	- rmdir $(INSTALL_USER_DIR)
endif



# Keep INSTALL_DIR from being empty
ifeq ("$(INSTALL_DIR)","")
  INSTALL_DIR =.
endif

delete_docs_dir:
ifdef HAVE_DOSCOMMAND
  ifdef DOS
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/docs/images/demo/*.* )
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/docs/images/*.* )
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/docs/res/*.* )
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/docs/technical/*.* )
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/docs/*.* )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/images/demo )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/images )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/res )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/technical )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs )
  else
  ifdef WIN98
	- $(RMFILE) $(subst /,\,$(INSTALL_DIR)/docs )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/images/demo )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/images )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/res )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs/technical )
	- rmdir $(subst /,\,$(INSTALL_DIR)/docs )
  else
	- rmdir $(subst /,\, $(INSTALL_DIR)/docs )  /S
  endif
  endif
else
	- $(RMFILE) -R $(INSTALL_DIR)/docs
endif


#=======================================================
# Make Options init

# File make_options does not exist on first make.
# Create initial make_options file and dirs.
# (a PHONY always triggers).
$(MAKE_OPTIONS): | $(O)
ifneq ("$(BUILD_DIR)","")
  ifeq ("$(wildcard $(BUILD_DIR))","")
    ifdef DOSFILE
	mkdir "$(BUILD_DIR)"
    else
	mkdir -p $(BUILD_DIR)
    endif
  endif
endif
	@echo -----------------------------------------------------------------------------
	@echo Initializing Doom Legacy make_options file.
	@echo See README_compiling.txt
ifdef DOSFILE
  ifdef DOS
	copy  make_options_dos  $(subst /,\,$(BUILD_DIR)make_options)
  else
  ifeq ($(OS), OS2)
	copy  make_options_os2  $(subst /,\,$(BUILD_DIR)make_options)
  else
	copy  make_options_win  $(subst /,\,$(BUILD_DIR)make_options)
  endif
  endif
else
  ifeq ($(OS), MAC)
	cp -n  make_options_mac  $(BUILD_DIR)make_options
  else
  ifeq ($(OS), FREEBSD)
	cp -n  make_options_nix  $(BUILD_DIR)make_options
  else
  ifeq ($(OS), NETBSD)
	if ! test -e $(BUILD_DIR)make_options; then cp make_options_nix  $(BUILD_DIR)make_options; fi
  else
	cp -n  make_options_nix  $(BUILD_DIR)make_options
  endif
  endif
  endif
endif
# Init message must be here.
# If separate, Make evaluates initial make_options, and errors.
	@echo -----------------------------------------------------------------------------
	@echo Edit the make_options file with OS selection, before running make again,
	@echo or use make command line options every time.
ifeq ($(SMIF), SDL)
	@echo -----------------------------------------------------------------------------
	@echo NOTICE: There is a CMAKE file included that can handle the default
	@echo compilation for SDL.  You can try that first, instead of this more
	@echo complicated Make that has all the OS, other SMIF, and more options.
endif
	@echo -----------------------------------------------------------------------------
	@echo Init Done.  Please Edit $(MAKE_OPTIONS) now.
	@false
	@echo A Choose one: error message is due to the make_options file.
	@stop # stop make, by error if necessary. Problem: IRIX make.
# DO NOT use $(error) here because it also prevents the above from executing.

#=======================================================

# Do not remake makefile

Makefile:
	@:

#############################################################
#
#############################################################
