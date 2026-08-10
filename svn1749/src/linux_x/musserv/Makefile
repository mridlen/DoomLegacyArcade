##########################################################
# GNU Make makefile for Doom Legacy
# Makefile for musserver 1.4
#
# Copyright (C) 1998-2015 by Doom Legacy Team.
#
# This Makefile requires exports from the Main Makefile and
# CANNOT be executed on it own.
#
# commands:
#  all  :  compile executable (default)
#  dirs : create dep, objs, bin directories
#  clean : delete objs and bin
#  install: install binaries to INSTALL_DIR
#  uninstall: install binaries from INSTALL_DIR
##########################################################

# SERV_BUILD_DIR is export from Main or Src Makefile
ifndef SERV_BUILD_DIR
  $(error SERV_BUILD_DIR not defined)
endif

# Invoke make with MAKE_OPTIONS= to specify another file or location.
ifndef MAKE_OPTIONS
  MAKE_OPTIONS = $(SERV_BUILD_DIR)make_options
endif

# Put user settings in this file, and they will be included with every
# invocation of make.
-include $(MAKE_OPTIONS)

#openserver5
# MUS_OS=SCOOS5
#unixware2
# MUS_OS=SCOUW2
#unixware7
# MUS_OS=SCOUW7


SND_DEVS:=

# Music Devices support, mostly for Linux X11.
# MIDI devices do not seem to need libraries.

ifdef HAVE_OSS
  ifeq ($(HAVE_OSS), OPT)
    SND_DEVS+=-DDEV_OSS=3
  else
    SND_DEVS+=-DDEV_OSS
  endif
endif

ifdef HAVE_ALSA
  ifeq ($(HAVE_ALSA), OPT)
    SND_DEVS+=-DDEV_ALSA=3
  else
    SND_DEVS+=-DDEV_ALSA
    SND_LIBS+=-lasound
  endif
  SND_LIBS+=-lm -ldl -lpthread -lrt
endif

ifdef HAVE_TIMIDITY
  ifeq ($(HAVE_TIMIDITY), OPT)
    SND_DEVS+=-DDEV_TIMIDITY=3
  else
    SND_DEVS+=-DDEV_TIMIDITY
  endif
endif

ifdef HAVE_FLUIDSYNTH
  ifeq ($(HAVE_FLUIDSYNTH), OPT)
    SND_DEVS+=-DDEV_FLUIDSYNTH=3
  else
    SND_DEVS+=-DDEV_FLUIDSYNTH
  endif
endif

ifdef HAVE_FM_SYNTH
  ifeq ($(HAVE_FM_SYNTH), OPT)
    SND_DEVS+=-DDEV_FM_SYNTH=3
  else
    SND_DEVS+=-DDEV_FM_SYNTH
  endif
endif

ifdef HAVE_AWE32_SYNTH
  ifeq ($(HAVE_AWE32_SYNTH), OPT)
    SND_DEVS+=-DDEV_AWE32_SYNTH=3
  else
    SND_DEVS+=-DDEV_AWE32_SYNTH
#   SND_LIBS+=
  endif
endif



ifdef DEBUG
DEBUGFLG= -g3
else
DEBUGFLG=
endif
#CC	= gcc

CFLAGS:=
OPT:=

ifdef SND_DEVS
  OPT+=$(SND_DEVS)
endif

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
else
  CFLAGS=-march=i586
endif
CFLAGS+= -Wall -O2

LDFLAGS	=
#LDFLAGS	= -static
OPTOBJ=

INCOPT = -I. -I../../

# Differences from Linux

ifeq ($(SMIF), FREEBSD_X11)
 CFLAGS	= -Wall -O2 -m486 -I/usr/src/sys/i386/isa/sound -DFREEBSD
 SUBDIR=freebsd
else
 SUBDIR=linux
endif

ifeq ($(MUS_OS), SCOOS5)
# The differences from Linux
# use this for an older GNU C 2.7.2
#CFLAGS	= -Wall -O2 -m486 -b elf -DSCOOS5
# use this for EGCS 1.0.1
CFLAGS	= -Wall -O2 -mpentium -DSCOOS5
LDFLAGS	=
OPTOBJ=usleep.o
SUBDIR=openserver5
endif

ifeq ($(MUS_OS), SCOUW2)
CFLAGS	= -Wall -O2 -m486 -DSCOUW2
LDFLAGS	=
OPTOBJ=usleep.o
SUBDIR=unixware2
endif

ifeq ($(MUS_OS), SCOUW7)
CC=cc
CFLAGS	= -O2 -K pentium -DSCOUW7
LDFLAGS	=
OPTOBJ=usleep.o
SUBDIR=unixware7
endif

LDFLAGS+=$(SND_LIBS)

#############################################
# Nothing below this line should be changed #
#############################################

# Export SERV_BUILD_DIR from parent Make.
BIN:=$(SERV_BUILD_DIR)bin
O:=$(SERV_BUILD_DIR)objs/mus_$(SUBDIR)

MUSOBJLIST:=musserver.o readwad.o playmus.o sequencer.o ../m_swap.o $(OPTOBJ)
MUSOBJ:=$(addprefix $(O)/, $(MUSOBJLIST))

all: $(BIN)/musserver

$(BIN)/musserver: $(MUSOBJ)
	${CC} ${DEBUGFLG} ${CFLAGS} ${LDFLAGS} $(MUSOBJ) -o $(BIN)/musserver

clean:
	rm -f $(O)/*

ifeq ("$(wildcard $(O))","")
  # Missing obj dir
  $(MUSOBJ) : | dirs
endif

dirs:
	mkdir -p $(O)

$(O)/%.o: %.c
	${CC} ${DEBUGFLG} ${CFLAGS} ${OPT} ${INCOPT} -c $< -o $@

#=======================================================
# Install binaries

install:
	$(INSTALL) $(INSTALL_OPTS) -t $(INSTALL_DIR)  $(BIN)/musserver

uninstall:
	- cd $(INSTALL_DIR) && $(RMFILE) musserver

##########################################################
