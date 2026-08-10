#############################################################################
#
# $Id: Makefile
# GNU Make makefile for Doom Legacy, Resources Makefile
#
# Copyright (C) 2015 by Doom Legacy Team.
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
# This Makefile requires exports from the Main Makefile and
# CANNOT be executed on it own.
#
# Required for wad_update and wad_extract:
# WAD=      The existing legacy.wad
#
# Optional, use when needed:
# WADTOOL=  Path to wadtool executable
# DEUTEX=   Path to deutex executable
# DEUTOOL=  Path to deutex executable
# DOOMWAD=  Path to doom.wad, doom2.wad, for deutex
# DOOMDIR=  Path to directory of doom.wad, doom2.wad, etc., for deutex
#
# commands:
#  legacywad  : legacy.wad missing message
#  wad_update : update existing legacy.wad
#  wad_extract: extract an existing legacy.wad to WAD directory
#  wad_create : create a legacy.wad from the WAD directory (not supplied)
#############################################################################

# MAIN_BUILD_DIR is export from Main Makefile
ifndef MAIN_BUILD_DIR
  $(error MAIN_BUILD_DIR not defined)
endif

# Subdirectories for binaries and build intermediates
BIN := $(MAIN_BUILD_DIR)/bin
O := $(MAIN_BUILD_DIR)/objs
ifdef USE_DEUTEX
  # Different directory format for deutex, so don't mix them.
  WD := $(MAIN_BUILD_DIR)/wadd
else
  WD := $(MAIN_BUILD_DIR)/wad
endif

ifdef DOSFILE
  # DOS/OS2/WIN32 file differences
  BIN_WIN:=$(subst /,\,$(BIN))
  O_WIN:=$(subst /,\,$(O))
  WD_WIN:=$(subst /,\,$(WD))
else
  SHELL:=/bin/sh
endif

# MAKECMDGOALS is in GNUMake, MinGW
ifdef MAKECMDGOALS
  ifneq ("$(findstring $(MAKECMDGOALS), wad_update wad_extract)","")
    need_wad=FIRM
  endif
else
  #default
  need_wad=MAYBE
endif

ifdef need_wad
  ifdef WAD
    wild_wad:=$(wildcard $(WAD) ../$(WAD))
    ifneq ("$(wild_wad)","")
      # Do not alter exported variables due to sub-make repeating this.
      wadp:=$(word 1, $(wild_wad))
    else
      $(error "WAD not accessible: ../$(WAD)")
    endif
  else
    ifeq ("$(need_wad)","FIRM")
      $(error "WAD=old_legacy.wad is required")
    endif
  endif
endif


ifdef DOSFILE
  WADTOOL?=wadtool.exe
else
  WADTOOL?=wadtool
endif

# Find the tools, searching several places.
# This may get more complicated.
# Many hours of testing reveals that must have no extra spaces in ifeq,
# they are not being stripped.
ifneq ("$(WADTOOL)","")
  wild_wadtool:=$(wildcard $(WADTOOL) /usr/local/bin/$(WADTOOL))
  ifneq ("$(wild_wadtool)","")
    USE_WADTOOL:=1
  else
    # Relative path
    wild_wadtool:=$(wildcard ../$(WADTOOL) $(BIN)/$(WADTOOL))
    ifneq ("$(wild_wadtool)","")
      WADTOOL:=$(word 1, $(wild_wadtool))
      USE_WADTOOL:=1
    endif
  endif
endif

ifdef DEUTEX
  $(info Deutex can extract from legacy.wad, but cannot create it)
  $(info due to the things.h label, which it cannot handle.)
  $(info Recommend using wadtool instead.)
  # Require DEUTEX to use deutex, disable automatic detections.
  # Kept for experimentation.
  ifneq ("$(findstring deutex, $(DEUTEX)","")
    DEUTOOL:=$(DEUTEX)
  else
    ifdef DOSFILE
      DEUTOOL?=deutex.exe
    else
      DEUTOOL?=deutex
    endif
  endif

  ifneq ("$(DEUTOOL)","")
    wild_deutool:=$(wildcard $(DEUTOOL) /usr/local/bin/$(DEUTOOL))
    ifneq ("$(wild_deutool)","")
      USE_DEUTEX:=1
    else
      # Relative path
      wild_deutool:=$(wildcard ../$(DEUTOOL) $(BIN)/$(DEUTOOL))
      ifneq ("$(wild_deutool)","")
        DEUTOOL:=$(word 1, $(wild_deutool))
        USE_DEUTEX:=1
      endif
    endif
  endif

  ifdef DOOMWAD
    DOOMDIR:=$(dir $(DOOMWAD))
    $(info   Extracted DOOMDIR=$(DOOMDIR)  from DOOMWAD=$(DOOMWAD))
  endif
  ifndef DOOMDIR
    $(error  Deutex requires: DOOMDIR or DOOMWAD defined)
  else
    ifeq ("$(DOOMDIR)","")
      $(info   DOOMDIR=$(DOOMDIR))
      $(error  Deutex requires: DOOMDIR or DOOMWAD not empty)
    endif
  endif

  # Disable wadtool
  undefine USE_WADTOOL
else
  # Disable deutex
  undefine DEUTOOL
endif


ifdef USE_WADTOOL
  # Using wadtool
  undefine USE_DEUTEX
else
ifdef USE_DEUTEX
  # Using deutex
else
  $(info   Cannot create legacy.wad this way, can only modify it.)
  $(info   Get legacy.wad in the legacy_common download.)
  $(info   Common wad tools are used on legacy.wad, except for adding some special lumps.)
  $(info   Deutex can extract, but cannot add the special lumps.)
  $(info   Recommend: make wadtool)
  $(info   Looks for wadtool in bin, current dir, or can define WADTOOL=wadtool.exe)
  $(error  Requires wadtool (or deutex).)
endif
endif



.PHONY : legacywad wad_update wad_extract wad_create

# Missing legacy.wad
legacywad :
	@echo "If you are missing the legacy.wad, or have an old legacy.wad,"
	@echo "please download the new version in the legacy_common download."
	@echo " "
	@echo "This is not the way to create a legacy.wad file.  This MAKE can only"
	@echo "alter an existing legacy.wad to conform to the new executable."
	@echo " "
	@echo "You will need an old legacy.wad file,"
	@echo "and the legacy.wad contents of the resources directory,"
	@echo "and the wadtool from the tools directory."
	@echo "Usage: make updatewad WAD=old_legacy.wad"


# Update the legacy.wad file, using wadtool.
wad_update : wad_extract
	@echo "Building a new legacy.wad using an old version $(WAD)..."
ifdef DOSFILE
  ifdef USE_WADTOOL	
	copy *.txt *.lmp *.h $(WD_WIN)
  else
  ifdef USE_DEUTEX
	copy FS_things.h $(WD_WIN)\lumps\things.h.lmp
	copy VERSION.txt $(WD_WIN)\lumps\version.lmp
	copy *.lmp $(WD_WIN)\lumps
  endif
  endif
else
  ifdef USE_WADTOOL	
	cp *.txt *.lmp *.h $(WD)
  else
  ifdef USE_DEUTEX
	cp FS_things.h $(WD)/lumps/things.h.lmp
	cp VERSION.txt $(WD)/lumps/version.lmp
	cp ENDOOM.lmp $(WD)/lumps/endoom.lmp
  endif
  endif
endif
	$(MAKE) wad_create


ifdef USE_DEUTEX
  DEU_OPT:= -v3 -overwrite -doom $(DOOMDIR)
  # -lumps -graphics -sounds -patches -sprites
endif

# Extract the legacy.wad file, using deutex.
wad_extract :
ifdef DOSFILE
	@echo Extracting $(WAD) to $(WD_WIN).
	@echo Untested
	@mkdir "$(WD_WIN)"
	@echo Extracting to $(WD_WIN)
  ifdef USE_WADTOOL	
	$(WADTOOL) -x $(wadp) -d $(WD_WIN)
	move $(WD_WIN)\VERSION.lmp $(WD_WIN)\VERSION.txt
	move $(WD_WIN)\THINGS.H.lmp $(WD_WIN)\FS_things.h
  else
  ifdef USE_DEUTEX
	$(DEUTOOL) $(DEU_OPT) -dir $(WD_WIN) -xtract $(wadp)
  endif
  endif
	@echo Extracted legacy.wad is in $(WD_WIN).
else
	@echo "Extracting $(wadp) to $(WD)."
	@mkdir -p $(WD)
	@echo Extracting to $(WD)
  ifdef USE_WADTOOL	
	$(WADTOOL) -d $(WD) -x $(wadp)
	mv $(WD)/VERSION.lmp $(WD)/VERSION.txt
	mv $(WD)/THINGS.H.lmp $(WD)/FS_things.h
  else
  ifdef USE_DEUTEX
	$(DEUTOOL) $(DEU_OPT) -dir $(WD) -xtract $(wadp)
  endif
  endif
	@echo Extracted legacy.wad is in $(WD).
endif


# Create the legacy.wad file, using deutex.
# CANNOT FIX: Deutex fails because wadinfo.txt has a line "THINGS.H",
# the dot is illegal in a lump name.
wad_create :
	@echo "Building a new legacy.wad from wad directory $(WAD)"
ifdef DOSFILE
	@echo Untested
  ifdef USE_WADTOOL
	$(WADTOOL) -d $(WD_WIN) -c $(BIN_WIN)\legacy.wad legacy.wad.inventory
	@echo New legacy.wad is in $(BIN_WIN).
  else
  ifdef USE_DEUTEX
	@echo Deutex will fail here, we already know.
	$(DEUTOOL) -doom $(DOOMDIR) -dir $(WD_WIN) -create $(BIN_WIN)\legacy.wad
	@echo Deutex failed here, we already know.
  endif
  endif
else
  ifdef USE_WADTOOL	
	$(WADTOOL) -d $(WD) -c $(BIN)/legacy.wad legacy.wad.inventory
	@echo New legacy.wad is in $(BIN).
  else
  ifdef USE_DEUTEX
	@echo Deutex will fail here, we already know.
	$(DEUTOOL) -doom $(DOOMDIR) -dir $(WD) -create $(BIN)/legacy.wad
	@echo Deutex failed here, we already know.
  endif
  endif
endif
