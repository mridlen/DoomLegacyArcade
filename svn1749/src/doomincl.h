// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: doomincl.h 835 2011-05-27 00:49:51Z wesleyjohnson $
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
//      Internally used data structures for virtually everything,
//      key definitions, lots of other stuff.
//      Not used in headers.
//
//-----------------------------------------------------------------------------

#ifndef DOOMINCL_H
#define DOOMINCL_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#if defined( __DJGPP__ )
#include <io.h>
#endif

#ifdef SMIF_PC_DOS
#include <conio.h>
#endif

#include "doomdef.h"
#include "doomtype.h"


// commonly used routines - moved here for include convenience

// [WDJ] Message types, subject to routing and output controls.
// Many choices so can be individually configured.
// There are tables indexed by EMSG_cat in console.c.
typedef enum {
   EMSG_cat = 0x0F, // mask category subject to display enables
 // one of the following categories
   EMSG_CONS = 0x00,  // existing unclassified CONS_Printf messages.
   EMSG_playmsg = 0x01,
   EMSG_playmsg2 = 0x02,
   EMSG_console = 0x05,  // console interactive
   EMSG_hud = 0x06,  // interactive messages, network
   EMSG_7,
   EMSG_info = 0x08,
   EMSG_ver = 0x09,  // verbose
   EMSG_debug = 0x0A,
   EMSG_dev = 0x0B,
   EMSG_warn = 0x0C,
   EMSG_errlog = 0x0D,  // stderr and log, but not console
   EMSG_error = 0x0E,
   EMSG_error2 = 0x0F,  // severe error
 // additional flags
   EMSG_now  = 0x40, // immediate update
   EMSG_all = 0x80
} EMSG_e;

// [WDJ] Enables for messages to various outputs
// Many choices so can be individually configured.
typedef enum {
   EOUT_hud = 0x01,   // hud message lines
   EOUT_con = 0x04,   // console
   EOUT_text = 0x10,  // stderr
   EOUT_log  = 0x20,  // log file
   EOUT_all = EOUT_text|EOUT_con|EOUT_log,
} EOUT_e;

extern  byte  EOUT_flags;  // EOUT_e
extern  byte  fatal_error;

// console.h
// Global param: EOUT_flags
void  CONS_Printf (const char * fmt, ...);
// For info, debug, dev, verbose messages
// print to text, console, and logs
//  emsg : EMSG_e
void  GenPrintf (const byte emsg, const char * fmt, ...);
void  GenPrintf_va (const byte emsg, const char * fmt, va_list ap );
// Console interaction printf interface.
void  con_Printf (const char *fmt, ...);
// Debug printf interface.
void  debug_Printf (const char *fmt, ...);

// i_system.h
void  I_Error (const char *error, ...);
void  I_SoftError (const char *errmsg, ...);

// m_misc.h
char * va(const char * format, ...);
char * Z_StrDup (const char * in);

// Network
extern  boolean  dedicated;  // dedicated server

// g_game.h
extern  byte    verbose;   // 1, 2
extern  byte    devparm;   // 1, 2, 3  // development mode (-devparm)

// demo version when playback demo, or the current VERSION
// used to enable/disable selected features for backward compatibility
// (where possible)
extern  byte    demoversion;
extern  uint16_t  demoversion_rev;  // VERREV(demoversion, revision)
#define  VERREV(v,r)   (((int)(v)<<8)+(r))

// version numbering
// [WDJ] For separate libs that cannot access VERSION var.
// So can test that have not mixed components from different releases.
#define DOOMLEGACY_COMPONENT_VERSION   (DL_VER_MAJ * 10000 + DL_VER_MIN * 100 + DL_VER_REV)

extern const int  VERSION;
extern const int  REVISION;
extern const char VERSION_BANNER[];

// =======================
// Log and Debug stuff
// =======================

// File handling stuff.
//#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if(debugfile) fputs(msg,debugfile); }
extern  FILE*           debugfile;
#else
#define DEBFILE(msg) {}
//extern  FILE*           debugfile;
#endif

#ifdef LOGMESSAGES
extern  FILE  *logstream;
#endif


#endif  /* DOOMINCL_H */

