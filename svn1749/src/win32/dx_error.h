// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: dx_error.h 1035 2013-08-14 00:38:40Z wesleyjohnson $
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
// $Log: dx_error.h,v $
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      transform an unreadable DirectX error code
//      into a meaningful error message.
//
//-----------------------------------------------------------------------------

#ifndef DX_ERROR_H
#define DX_ERROR_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Displays a message box containing the given formatted string.
//void __cdecl DXErrorMessageBox (LPSTR fmt, ... );

// Returns a pointer to a string describing the given DD, D3D or D3DRM error code.
char* DXErrorToString (HRESULT error);

#ifdef __cplusplus
};
#endif
#endif // DX_ERROR_H
