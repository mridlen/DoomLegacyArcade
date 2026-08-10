// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: fabdxlib.h 1044 2013-08-26 20:37:47Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: fabdxlib.h,v $
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      faB's DirectX library v1.0
//
//-----------------------------------------------------------------------------

#ifndef FABDXLIB_H
#define FABDXLIB_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// We can use 0x0300, 0x0500, 0x0600, or 0x0700, or can default
#define DIRECTDRAW_VERSION  0x0300
#include <ddraw.h>

// format of function in app called with width,height
typedef BOOL (*FDX_enum_mode_callback)(int width, int height, int bpp);


// globals
extern IDirectDraw*             DDr;
extern IDirectDrawSurface*      ScreenReal;
extern IDirectDrawSurface*      ScreenVirtual;
extern IDirectDrawPalette*      DDPalette;

extern BOOL                     fdx_fullscreen;
  // main code might need this to know the current
  // fullscreen or windowed state

extern int                      windowPosX;   // current position in windowed mode
extern int                      windowPosY;

extern int                      ScreenWidth;    
extern int                      ScreenHeight;
extern BOOL                     ScreenLocked; // Screen surface is being locked
extern int                      ScreenPitch;  // offset from one line to the next
extern unsigned char*           ScreenPtr;    // memory of the surface


BOOL    FDX_EnumDisplayModes (FDX_enum_mode_callback appFunc);
void    FDX_create_main_instance (void);

BOOL    FDX_InitDDMode (HWND appWin, int width, int height, int bpp, int fullScr);
void    FDX_CloseDirectDraw (void);

void    FDX_ReleaseChtuff (void);

void    FDX_ClearSurface (IDirectDrawSurface* surface, int color);
void    FDX_ScreenFlip (int wait);
void    TextPrint (int x, int y, char* message);

void    FDX_CreateDDPalette (PALETTEENTRY* colorTable);
void    FDX_DestroyDDPalette (void);
void    FDX_SetDDPalette (PALETTEENTRY* pal);

void    FDX_WaitVbl (void);

BOOL    FDX_LockScreen (void);
void    FDX_UnlockScreen (void);


#endif /* FABDXLIB_H */
