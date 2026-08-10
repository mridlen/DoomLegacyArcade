#############################################################################
#
# $Id: Makefile
# GNU Make makefile for Doom Legacy, tools makefile
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
# This Makefile requires exports from the Main Makefile and
# CANNOT be executed on it own.
#
#############################################################################

# MAIN_BUILD_DIR is export from Main Makefile
ifndef MAIN_BUILD_DIR
  $(error MAIN_BUILD_DIR not defined)
endif

# Subdirectories for binaries and build intermediates
BIN = $(MAIN_BUILD_DIR)/bin
O  = $(MAIN_BUILD_DIR)/objs

ifdef DOSFILE
  RM:=del
  EXE:=.exe
  # DOS/OS2/WIN32 file differences
  BIN_WIN:=$(subst /,\,$(BIN))
  O_WIN:=$(subst /,\,$(O))
else
  RM:=rm
  EXE:=
  SHELL:=/bin/sh
endif

# CC is export from Main Makefile
ifndef CC
  CC:= gcc
  CXX:= g++
endif

CXXFLAGS:= -O4 -mtune=native -Wall
ifeq ($(OS), WIN98)
  CFLAGS:= -O3 -Wall
else
ifeq ($(OS), WIN32)
  CFLAGS:= -O4 -Wall
else
  CFLAGS:= -O6 -mpentium -Wall
endif
endif

LDFLAGS:=
ifeq ($(OS), MAC)
  # MAC linker does not accept these flags.
else
  LDFLAGS+=-Xlinker --warn-common
endif

.PHONY : default tools asm
.PHONY : h2d dckconv dckconv2 dckcomp
.PHONY : convert wadtool dircomp2 fixdep

default: dircomp2

tools: dircomp2 convert wadtool

dircomp2 : $(BIN)/dircomp2$(EXE)
$(BIN)/dircomp2$(EXE):	 dircomp2.c
	$(CC) $(CFLAGS) -s dircomp2.c -o $(BIN)/dircomp2$(EXE)

asm:
	$(CC) $(CFLAGS) -g dircomp2.c -o $(BIN)tmp$(EXE)
	objdump  -dS $(BIN)tmp$(EXE) --debugging --no-show-raw-insn > $(BIN)/dircomp2.s
	$(RM) $(BIN)tmp$(EXE)


h2d : $(BIN)/h2d$(EXE)
$(BIN)/h2d$(EXE):	 h2d.c
	$(CC) $(CFLAGS) -s h2d.c -o $(BIN)/h2d$(EXE)

dckconv : $(BIN)/dckconv$(EXE)
$(BIN)/dckconv$(EXE):    dckconv.c
	$(CC) $(CFLAGS) -s dckconv.c -o $(BIN)/dckconv$(EXE)

dckconv2 : $(BIN)/dckconv2$(EXE)
$(BIN)/dckconv2$(EXE):   dckconv2.c
	$(CC) $(CFLAGS) -s dckconv2.c -o $(BIN)/dckconv2$(EXE)

dckcomp : $(BIN)/dckcomp$(EXE)
$(BIN)/dckcomp$(EXE):    dckcomp.c
	$(CC) $(CFLAGS) -s dckcomp.c -o $(BIN)/dckcomp$(EXE)

convert : $(BIN)/convert$(EXE)
$(BIN)/convert$(EXE):    convert.c
	$(CC) $(CFLAGS) -s convert.c -o $(BIN)/convert$(EXE)

fixdep : $(BIN)/fixdep$(EXE)
$(BIN)/fixdep$(EXE):    $(O)/fixdep.o
	$(CC) $(LDFLAGS) $^ -o $(BIN)/fixdep$(EXE)

$(O)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

wadtool : $(BIN)/wadtool$(EXE)
$(BIN)/wadtool$(EXE): $(O)/wadtool.o
	@echo Linking...
	$(CXX) $(LDFLAGS) $^ -o $@
