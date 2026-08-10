##########################################################
# $Id: Makefile,v 1.7 2004/04/20 00:36:48 andyp Exp $
#
# GNU Make makefile for Doom Legacy
# Makefile for sndserver
#
# Copyright (C) 1998-2015 by Doom Legacy Team.
#
# $Log: Makefile,v $
# Revision 1.7  2004/04/20 00:36:48  andyp
# Linux compilation fixes and string cleanups
#
# Revision 1.6  2002/12/17 19:56:51  bock
# Add esound support without SDL.
# Sorry about this, this code is disabled by default.
# You should build with HAVE_ESD=1 flag.
#
# Revision 1.5  2001/05/29 22:18:41  bock
# Small BSD commit: build r_opengl.so, sndserver
#
# Revision 1.4  2000/04/30 19:50:37  metzgermeister
#
# Revision 1.3  2000/03/12 22:34:07  linuxcub
# Fixed makefile (leading spaces converted to tabs)
#
# Revision 1.2  2000/02/27 00:42:12  hurdler
# Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
# Initial import into CVS (v1.29 pr3)
#
# This Makefile requires exports from the Main Makefile and
# CANNOT be executed on it own.
#
# commands:
#  all  :  compile executable (default)
#  dirs : create objs directories
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

# OSS sound for X11
#   HAVE_OSS
#   OPTIONAL_OSS
# ALSA sound for X11
#   HAVE_ALSA
#   OPTIONAL_ALSA
# ESD demon for X11 (esound), not needed for SDL.
#   HAVE_ESD=1
#   OPTIONAL_ESD
# Jack sound for X11
#   HAVE_JACK
#   OPTIONAL_JACK
# PulseAudio sound for X11
#   HAVE_PULSE
#   OPTIONAL_PULSE

#CC=gcc
IFLAGS=-I. -I../../

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
else
  CFLAGS=-march=i586
endif
CFLAGS+=-O2 -Wall -DNORMALUNIX -DLINUX

LDFLAGS=
LIBS=-lm
OPTS=
OPTOBJ=
SNDEXE=llsndserv
LIB_DLOPEN:=

# Linux and similar versions only.
ifeq ($(OS), LINUX)
  LINUX=1
  OPTS+=-DLINUX
  LIB_DLOPEN:=-ldl
else
ifeq ($(OS), FREEBSD)
  FREEBSD=1
  OPTS+=-DLINUX -DFREEBSD
  LIB_DLOPEN:=-ldl
else
ifeq ($(OS), NETBSD)
  NETBSD=1
  OPTS+=-DLINUX -DNETBSD
  LIB_DLOPEN:=-ldl
endif
endif
endif

# Differences from Linux

ifeq ($(SMIF), FREEBSD_X11)
 SUBDIR=freebsd
 CFLAGS+=-DFREEBSD
else
 SUBDIR=linux
endif

ifeq ($(MUS_OS), SCOOS5)
CC=gcc
# use this for an older GNU C 2.7.2
#CFLAGS=-O2 -m486 -b elf -Wall -DNORMALUNIX -DSCOOS5
# use this for EGCS 1.0.1
CFLAGS=-O2 -mpentium -Wall -DNORMALUNIX -DSCOOS5
LDFLAGS=
LIBS=-lm
OPTOBJ=wadread.o
SUBDIR=openserver5
SNDEXE=sndserver
endif

ifeq ($(MUS_OS), SCOUW2)
CC=gcc
CFLAGS=-O2 -m486 -Wall -DNORMALUNIX -DSCOUW2
LDFLAGS=
OPTOBJ=wadread.o strcmp.o
SUBDIR=unixware2
SNDEXE=sndserver
endif

ifeq ($(MUS_OS), SCOUW7)
CC=cc
CFLAGS=-O2 -K pentium -DNORMALUNIX -DSCOUW2
LDFLAGS=
OPTOBJ=wadread.o strcmp.o
SUBDIR=unixware7
SNDEXE=sndserver
endif

# Sound Devices

SND_DEVS:=
SND_CFLAGS:=
SND_LIBS:=
SND_LDFLAGS:=

# Sound Devices support.
# If a sound device is HAVE_xx, its library must be present.
# If a sound device is HAVE_xx==3, its library will be detected and loaded dynamically.
# The sndserver also uses snd_driver, and lx_ctrl.h.

ifdef HAVE_OSS
  ifeq ($(HAVE_OSS), 3)
    SND_DEVS+=-DDEV_OSS=3
  else
    SND_DEVS+=-DDEV_OSS
  endif
endif

ifdef HAVE_ALSA
  ifeq ($(HAVE_ALSA), 3)
    SND_DEVS+=-DDEV_ALSA=3
    ifdef ALSA_KLUDGE
# This was required to avoid confusion by the linker, caused by ALSA having
# ALSA 0.9, and ALSA 0.9.0_rc4,
# The wrong symbols are referenced in new programs because the
# linker defaults to the OLD ALSA 0.9, if not given an explicit library.
# Because the pcm_old api does not have the symbols for:
#  snd_pcm_hw_params_sizeof, snd_pcm_sw_params_sizeof
# This has been fixed by explict load of those symbols.
    OPTS+=-DALSA_DYN_KLUDGE
    SND_LIBS+=-lasound
    endif
  else
    SND_DEVS+=-DDEV_ALSA
    SND_LIBS+=-lasound
  endif
  SND_LIBS+=-lm -ldl -lpthread -lrt
endif

ifdef HAVE_ESD
  ifeq ($(HAVE_ESD), 3)
    SND_DEVS+=-DDEV_ESD=3
  else
    SND_DEVS+=-DDEV_ESD
    SND_LIBS+=-lesd
#  SND_CFLAGS	+=	$(shell esd-config --cflags)
#  SND_LIBS	+=	$(shell esd-config --libs)
  endif
endif

ifdef HAVE_JACK
  ifeq ($(HAVE_JACK), 3)
    SND_DEVS+=-DDEV_JACK=3
  else
    SND_DEVS+=-DDEV_JACK
    SND_LIBS+=-ljack
  endif
  SND_LIBS+=-lpthread
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
endif


CFLAGS+=$(SND_CFLAGS) $(SND_DEVS)
LIBS+=$(SND_LIBS)
LDFLAGS+=$(SND_LDFLAGS)

# Handle dlopen
ifdef LIB_DLOPEN
  ifdef HAVE_DLOPEN
    LIBS+=$(LIB_DLOPEN)
    OPTS+=-DHAVE_DLOPEN
  endif
endif



#############################################

# Subdirectories for binaries and build intermediates
BIN:= $(SERV_BUILD_DIR)bin
O:= $(SERV_BUILD_DIR)objs/snd_$(SUBDIR)


#HWOUTOBJ	:=	linux.o
HWOUTOBJ=

SNDOBJLIST:=soundsrv.o snd_driver.o $(HWOUTOBJ) $(OPTOBJ)
SNDOBJ:=$(addprefix $(O)/, $(SNDOBJLIST))


all:  $(BIN)/$(SNDEXE)

clean:
	rm -f $(O)/*

ifeq ("$(wildcard $(O))","")
  # Missing obj dir
  $(SNDOBJ) : | dirs
endif

dirs:
	mkdir -p $(O)

# Target
$(BIN)/$(SNDEXE): $(SNDOBJ)
	@echo Making sound server: $(SNDEXE)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SNDOBJ) -o $(BIN)/$(SNDEXE) $(LIBS)

# Rule
$(O)/%.o: %.c
	$(CC) $(CFLAGS) $(IFLAGS) $(SND_DEVS) $(OPTS) $(OPTLIB) -c $< -o $@

# snd_driver from linux_x
# Must be compiled for SNDSERV
$(O)/snd_driver.o: ../snd_driver.c
	$(CC) $(CFLAGS) $(IFLAGS) $(SND_DEVS) -DSNDSERV $(OPTS) $(OPTLIB) -c $< -o $@


#=======================================================
# Install binaries

install:
	$(INSTALL) $(INSTALL_OPTS) -t $(INSTALL_DIR)  $(BIN)/$(SNDEXE)

uninstall:
	- cd $(INSTALL_DIR) && $(RMFILE) $(SNDEXE)

##########################################################
