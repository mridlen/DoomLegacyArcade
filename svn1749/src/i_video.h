// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_video.h 1721 2025-01-28 15:15:00Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: i_video.h,v $
// Revision 1.6  2001/08/20 20:40:39  metzgermeister
// Revision 1.5  2001/06/10 21:16:01  bpereira
// Revision 1.4  2001/02/24 13:35:20  bpereira
// Revision 1.3  2000/11/02 19:49:35  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System specific video interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef I_VIDEO_H
#define I_VIDEO_H


#include "doomtype.h"
#include "command.h"
  // consvar_t

#ifdef __GNUG__
#pragma interface
#endif

// configure settings, all possibilities
// Index to several tables.
typedef enum {
  DRM_none,
  // software draw modes
  DRM_8pal, DRM_15, DRM_16, DRM_24, DRM_32,
  // internal signaling, command line support
  DRM_explicit_bpp,
  DRM_native,
  // hardware draw modes, DRM_opengl is boundary test
  DRM_opengl,
  DRM_minigl,
  DRM_glide,
  DRM_d3d,
  DRM_END
} vid_drawmode_e;

typedef enum {
    render_init   = 0,
    render_soft   = 1,
    render_opengl = 2, //Hurdler: the same for render_minigl
    render_d3d    = 3,
    render_glide  = 4,
    render_none   = 5  // for dedicated server
} rendermode_e;

extern rendermode_e    rendermode;

// Structure for passing modenums with the necessary context
typedef enum {
    MODE_NOP,
    MODE_window,
    MODE_fullscreen,
    MODE_voodoo,
    MODE_either,  // window or fullscreen
    MODE_other
} modetype_e;

extern const char * modetype_string[ MODE_other + 1 ];

typedef struct {
    byte  modetype;  // from modetype_e
    byte  index;
} modenum_t;

typedef struct {
    int   width, height;
    const char *  mark;  // const string, only NULL when invalid mode
    byte  type;  // modetype_e, MODE_NOP when not found
} modestat_t;


extern byte  allow_fullscreen;  // controlled by i_video
// can window before going to cv_fullscreen
extern byte  mode_fullscreen;   // modetype_e

// Request to video drivers.
extern byte req_drawmode;  // vid_drawmode_e
extern byte req_bitpp;
// Parameter to V_switch_drawmode
extern byte req_alt_bitpp;
extern byte req_command_video_settings;
extern uint16_t req_width, req_height;
// From video drivers.
extern byte native_drawmode;  // vid_drawmode_e
extern byte native_bitpp;
extern byte native_bytepp;

// added for OpenGL gamma correction
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;

// wait for page flipping to end or not
extern consvar_t cv_vidwait;

extern consvar_t cv_fullscreen; // for fullscreen support

// Return true if engine can draw using the bitpp
boolean V_CanDraw( byte bitpp );

// ---------------------------------------------

// Setup HWR calls according to rendermode.
int  I_Rendermode_setup( void );

// Initial windowed graphics
void I_StartupGraphics (void);
// Full game playing graphics, with hardware drawing, and options
// Returns FAIL_select, FAIL_end, FAIL_create, of status_return_e, 1 on success;
int I_RequestFullGraphics( byte select_fullscreen );
// Restore original video mode.
void I_ShutdownGraphics(void);

// Takes full 8 bit values.
void I_SetPalette (RGBA_t* palette);

#ifdef MACOS_DI
// in macos directory
void macConfigureInput(void);

void VID_Pause(int pause);
#endif


// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype );
char  *  VID_GetModeName(modenum_t modenum);
modestat_t  VID_GetMode_Stat(modenum_t modenum);

// rmodetype is of modetype_e
// Returns MODE_NOP when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype );

// To check if can draw the video
//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp );

//  By setting setmodeneeded to a value > 0,
//  the video mode change is delayed until the start of the next refresh
//
//  Set the video mode right now.
//  Returns FAIL_end, FAIL_create, of status_return_e, 1 on success;
int  VID_SetMode( modenum_t modenum );

void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

void I_ReadScreen (byte* scr);

#if defined(SMIF_WIN_NATIVE) || defined(SMIF_OS2_NATIVE) 
// printf to loading screen
void I_LoadingScreen ( const char * msg );
#endif

// debug in color
// unused
void IO_Color( unsigned char color, unsigned char r, unsigned char g, unsigned char b );

#endif
