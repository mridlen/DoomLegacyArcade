// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_things.h 1634 2022-10-30 10:07:56Z wesleyjohnson $
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
// $Log: r_things.h,v $
// Revision 1.11  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.10  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
//
// Revision 1.9  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.8  2001/06/16 08:07:55  bpereira
//
// Revision 1.7  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.6  2000/11/09 17:56:20  stroggonmeth
// Hopefully fixed a few bugs and did a few optimizations.
//
// Revision 1.5  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.4  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/30 10:30:10  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------


#ifndef R_THINGS_H
#define R_THINGS_H

#include "doomtype.h"
#include "r_defs.h"
  // m_fixed.h, sector_t
#include "sounds.h"
#include "command.h"
  // consvar_t
#include "d_player.h"

#define CLIP2_LIMIT

#if defined(RANGECHECK) && defined(CLIP2_LIMIT)
# undef CLIP2_LIMIT
#endif


// MAXVISSPRITES was 128, then 256 (2-2-98)
#define MAXVISSPRITES   16000
 // [WDJ] Remove sprite limits. This is tuning not a hard limit.
extern consvar_t  cv_spritelim;

// Constant arrays used for psprite clipping
//  and initializing clipping.
extern int16_t          clip_screen_top_min[MAXVIDWIDTH];
extern int16_t          clip_screen_bot_max[MAXVIDWIDTH];

// vars for R_DrawMaskedColumn
// clipping array[x], in int screen coord.
extern int16_t       *  dm_floorclip;
extern int16_t       *  dm_ceilingclip;

extern fixed_t          dm_yscale;  // world to fixed_t screen coord
// drawn patch, in fixed_t screen coord
extern fixed_t          dm_top_patch, dm_bottom_patch;
// draw window clipping, in fixed_t screen coord
extern fixed_t          dm_windowtop, dm_windowbottom;
extern fixed_t          dm_texturemid;

extern fixed_t          pspritescale;
extern fixed_t          pspriteiscale;
extern fixed_t          pspriteyscale;  //added:02-02-98:for aspect ratio

extern const int PSpriteSY[];

void R_DrawMaskedColumn ( byte * column_data );

void R_SortVisSprites (void);

//faB: find sprites in wadfile, replace existing, add new ones
//     (only sprites from namelist are added or replaced)
void R_AddSpriteDefs (char** namelist, int wadnum);

//SoM: 6/5/2000: Light sprites correctly!
void R_AddSprites (sector_t* sec, int lightlevel);
void R_AddPSprites (void);
void R_Init_Sprites (char** namelist);
void R_Clear_Sprites (void);
void R_Init_draw_masked();
void R_Draw_Masked (void);

void R_ClipVisSprite ( vissprite_t* vis, int xl, int xh );

void R_DrawPlayerSprites (void);

void R_Load_Corona( void );
void R_Release_Corona( void );



// -----------
// SKINS STUFF
// -----------
#define SKINNAMESIZE 16
#define DEFAULTSKIN  "marine"   // name of the standard doom marine as skin

typedef struct
{
    char        name[SKINNAMESIZE+1];   // short descriptive name of the skin
    spritedef_t spritedef;
    char        faceprefix[4];          // 3 chars+'\0', default is "STF"

    // specific sounds per skin
    sfxid_t     soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table
} skin_t;

extern int       numskins;
extern skin_t *  skins[MAXSKINS+1];
extern consvar_t cv_skin[2];

void    SetPlayerSkin_by_index( player_t * player, int index );
void    SetPlayerSkin(int playernum, const char *skinname);
int     R_SkinAvailable( const char* name );
void    R_AddSkins (int wadnum);


#endif /*__R_THINGS__*/
