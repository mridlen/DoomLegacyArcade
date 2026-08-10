// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hu_stuff.h 1543 2020-08-22 02:36:35Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: hu_stuff.h,v $
// Revision 1.5  2003/11/22 00:22:09  darkwolf95
// get rid of FS hud pics on level exit and new game, also added exl's fix for clearing hub variables on new game
//
// Revision 1.4  2000/11/03 03:27:17  stroggonmeth
//
// Revision 1.3  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:  
//      Head up display
//
//-----------------------------------------------------------------------------

#ifndef HU_STUFF_H
#define HU_STUFF_H

#include "doomtype.h"
#include "d_event.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "r_defs.h"
#include "command.h"
  // consvar_t

//------------------------------------
//           heads up font
//------------------------------------
// the first font characters
#define HU_FONTSTART    '!'
// the last font characters
#define HU_FONTEND      '_'

#define HU_FONTSIZE     (HU_FONTEND - HU_FONTSTART + 1)


extern consvar_t  cv_crosshair[2];
//extern consvar_t  cv_crosshairscale;

extern char*   shiftxform;   // french/english translation shift table

extern char    english_shiftxform[];
extern char    french_shiftxform[];

//------------------------------------
//           chat stuff
//------------------------------------
#define HU_MAXMSGLEN    80

extern patch_t*       hu_font[HU_FONTSIZE];
extern byte  hu_fonts_loaded; // 1=partially loaded, 2=fully loaded

//set true by hu_ when entering a chat message
extern boolean chat_on; 

// P_DeathThink set this true to show scores while dead, in dmatch
extern boolean hu_showscores;
extern boolean playerdeadview;

void HU_Register_Commands( void );

void HU_Load_Graphics( void );
void HU_Release_Graphics( void );

// reset heads up when consoleplayer respawns.
void    HU_Start(void);

//
boolean HU_Responder(event_t* ev);

//
void    HU_Ticker(void);
void    HU_Drawer(void);
void    HU_Erase(void);

// used by console input
char ForeignTranslation(unsigned char ch);

// Initialize the chatmacros.
void HU_Init_Chatmacros (void);

// chatmacro <0-9> "message" console command
void Command_Chatmacro_f (void);

int HU_Create_TeamFragTbl(fragsort_t *fragtab,
                         int dmtotals[],
                         int fragtbl[MAXPLAYERS][MAXPLAYERS]);



void HU_SetTip(char *tip, int displaytics);
void HU_Clear_Tips();

void HU_Draw_FSPics();
void HU_Clear_FSPics();
int  HU_Get_FSPic( lumpnum_t lumpnum, int xpos, int ypos );
int  HU_Delete_FSPic(int handle);
int  HU_Modify_FSPic(int handle, lumpnum_t lumpnum, int xpos, int ypos);

int  HU_FS_Display(int handle, boolean enable_draw);
#endif
