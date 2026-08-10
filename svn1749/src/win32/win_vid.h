// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_vid.h 1076 2013-12-31 02:44:03Z wesleyjohnson $
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
// $Log: win_vid.h,v $
// Revision 1.5  2000/09/28 20:57:23  bpereira
// no message
//
// Revision 1.4  2000/09/01 19:34:38  bpereira
// no message
//
// Revision 1.3  2000/03/29 20:14:21  hurdler
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      
//
//-----------------------------------------------------------------------------


#ifndef WIN_VID_H
#define WIN_VID_H

#include "../doomtype.h"
#include "../command.h"
#include "../screen.h"

// these are utilised among different display drivers (glide/ddraw/dosvesa/..)
extern int     numvidmodes;     //total number of DirectDraw display modes
extern vmode_t *pvidmodes;      //start of videomodes list.


// setup a video mode, this is to be called from the menu
int  VID_SetMode (modenum_t modenum);

BOOL    VID_FreeAndAllocVidbuffer (viddef_t *lvid);

// uses memcpy
void VID_BlitLinearScreen (byte *srcptr, byte *destptr,
                           int width, int height,
                           int srcrowbytes, int destrowbytes);

#endif // WIN_VID_H
