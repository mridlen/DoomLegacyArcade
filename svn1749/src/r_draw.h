// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_draw.h 1691 2024-11-27 06:36:07Z wesleyjohnson $
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
// $Log: r_draw.h,v $
// Revision 1.9  2003/06/10 23:36:09  ssntails
// Variable flat support (32x32 to 2048x2048)
//
// Revision 1.8  2002/11/12 00:06:05  ssntails
// Support for translated translucent columns in software mode.
//
// Revision 1.7  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.6  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.5  2001/02/24 13:35:21  bpereira
// Revision 1.4  2000/11/09 17:56:20  stroggonmeth
//
// Revision 1.3  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      low level span/column drawer functions.
//
//-----------------------------------------------------------------------------


#ifndef R_DRAW_H
#define R_DRAW_H

#include "doomdef.h"
  // HORIZONTALDRAW
#include "doomtype.h"
#include "r_defs.h"

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------
extern byte*            ylookup[MAXVIDHEIGHT];
extern byte*            ylookup1[MAXVIDHEIGHT];
extern byte*            ylookup2[MAXVIDHEIGHT];
extern int              columnofs[MAXVIDWIDTH];

#ifdef HORIZONTALDRAW
//Fab 17-06-98
extern byte*            yhlookup[MAXVIDWIDTH];
extern int              hcolumnofs[MAXVIDHEIGHT];
#endif

extern byte             dr_alpha;  // translucent and fog alpha, 0..255
#ifdef ENABLE_DRAW_ALPHA
extern byte             dr_alpha_mode;  // alpha combine modes
extern byte             dr_alpha_background;  // alpha applied to background
extern byte             dr_color8;
extern RGBA_t           dr_color;  // draw alpha
#endif


// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

extern lighttable_t*    dc_colormap;
extern int              dc_x;
extern int              dc_yl;
extern int              dc_yh;
#ifdef MONSTER_VARY
extern int              dc_y0;  // ref for draw, to allow separate sprite scaling
#endif
extern fixed_t          dc_iscale;
extern fixed_t          dc_texturemid;

extern byte*            dc_source;      // first pixel in a column

// translucency stuff here
extern byte*            translucenttables;   // translucency tables, should be (*transtables)[5][256][256]
extern byte*            dc_translucentmap;   // ptr to selected table
extern byte             dc_translucent_index;


// Variable flat sizes SSNTails 06-10-2003
extern unsigned int flatsize;
extern unsigned int flatbitsz;  // flat bit size, flatsize = 2**flatbitsz
extern unsigned int flatfracbits; // FRACBITS - flatbitsz
extern unsigned int flat_ymask;   // index mask, = (flatsize-1)<<flatbitsz
extern fixed_t      flat_imask;   // index mask, = (flatsize<<flatbitsz) - 1

// translation stuff here

// [WDJ] player skin translation, skintranstables[NUMSKINCOLORS-1][256]
// Does not translate color 0
// Boom calls these TRANSLATION, but that confuses with translucent.
extern byte*            skintranstables;  // player skin translation tables
extern byte*            dc_skintran;  // ptr to selected skin table

// for skin = 1..(MAXSKINNUM-1), skin=0 does not use translation
#define SKIN_TO_SKINMAP( skin )  (&skintranstables[ ((skin)-1)<<8 ])

// For flags containing MF_TRANSLATION bits, 0=original skin
// [WDJ] MFT_TRANSSHIFT is an enum constant (p_mobj.h) so it cannot be tested in an #ifdef.
// There is a PARANOIA check in r_main.c.
// SVN1363: MFT_TRANSSHIFT==8, there is no need for a shift.
// CASE: MFT_TRANSSHIFT == 8
#define MFT_TO_SKINMAP( flags )  (&skintranstables[ ((flags) & MFT_TRANSLATION6) - 256 ])
// Just in case someone changes MFT_TRANSSHIFT, I include the other two cases here.  You never know.
// CASE: MFT_TRANSSHIFT > 8
// #define MFT_TO_SKINMAP( flags )  (&skintranstables[ (((flags) & MFT_TRANSLATION6) >> (MFT_TRANSSHIFT-8)) - 256 ])
// CASE: MFT_TRANSSHIFT < 8
// #define MFT_TO_SKINMAP( flags )  (&skintranstables[ (((flags) & MFT_TRANSLATION6) << (8-MFT_TRANSSHIFT)) - 256 ])


extern struct r_lightlist_s*      dc_lightlist;
extern int                        dc_numlights;
extern int                        dc_maxlights;

//Fix TUTIFRUTI
extern int      dc_texheight;


// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

extern int              ds_y;
extern int              ds_x1;
extern int              ds_x2;

extern lighttable_t*    ds_colormap;

extern fixed_t          ds_xfrac;
extern fixed_t          ds_yfrac;
extern fixed_t          ds_xstep;
extern fixed_t          ds_ystep;

extern byte*            ds_source;      // start of a 64*64 tile image
extern byte*            ds_translucentmap; // ptr to one translucent map


// viewborder patches lump numbers
#define BRDR_T      0
#define BRDR_B      1
#define BRDR_L      2
#define BRDR_R      3
#define BRDR_TL     4
#define BRDR_TR     5
#define BRDR_BL     6
#define BRDR_BR     7

extern lumpnum_t  viewborderlump[8];
extern lumpnum_t  st_borderflat_num;  // st_stuff.c

// ------------------------------------------------
// r_draw.c COMMON ROUTINES FOR BOTH 8bpp and 16bpp
// ------------------------------------------------

//added:26-01-98: called by SCR_Recalc() when video mode changes
void    R_RecalcFuzzOffsets (void);
// Initialize color translation tables, for player rendering etc.
void    R_Init_TranslationTables (void);

void    R_Init_ViewBuffer ( int width, int height );

void    R_Init_ViewBorder (void);

// Changes in drawmode
void    R_Setup_Drawmode (void);

void    R_VideoErase ( unsigned int ofs, int count );

// Rendering function.
void    R_FillBackScreen (void);

// If the view size is not full screen, draws a border around it.
void    R_DrawViewBorder (void);

// [WDJ] Generic
void    R_DrawColumnShadowed (void);

// -----------------
// 8bpp DRAWING CODE
// -----------------

#ifdef HORIZONTALDRAW
//Fab 17-06-98
void    R_DrawHColumn_8 (void);
#endif

void    ASMCALL R_DrawColumn_8 (void);
void    ASMCALL R_DrawSkyColumn_8 (void);
void    ASMCALL R_DrawShadeColumn_8 (void);             //smokie test..
void    ASMCALL R_DrawFuzzColumn_8 (void);
void    ASMCALL R_DrawAlphaColumn_8 (void);
void    ASMCALL R_DrawTranslucentColumn_8 (void);
void    ASMCALL R_DrawTranslatedColumn_8 (void);  // skin
void    ASMCALL R_DrawSpan_8 (void);

// SSNTails 11-11-2002
void    R_DrawTranslatedTranslucentColumn_8 (void);  // skin translucent

void    R_DrawTranslucentSpan_8 (void);
void    R_DrawFogSpan_8 (void);
void    R_DrawFogColumn_8 (void); //SoM: Test
//void    R_DrawColumnShadowed_8 (void);
//void    R_DrawPortalColumn_8 (void);

// ------------------
// 16bpp DRAWING CODE
// ------------------

void    ASMCALL R_DrawColumn_16 (void);
void    ASMCALL R_DrawSkyColumn_16 (void);
void    ASMCALL R_DrawFuzzColumn_16 (void);
void    ASMCALL R_DrawTranslucentColumn_16 (void);
void    ASMCALL R_DrawTranslatedColumn_16 (void);  // skin
void    ASMCALL R_DrawSpan_16 (void);

void    R_DrawTranslatedTranslucentColumn_16 (void);  // skin translucent
void    R_DrawShadeColumn_16(void);
void    R_DrawAlphaColumn_16(void);
void    R_DrawTranslucentSpan_16(void);
void    R_DrawFogSpan_16(void);
void    R_DrawFogColumn_16(void);


// ------------------
// 24bpp DRAWING CODE
// ------------------

void    R_DrawColumn_24 (void);
void    R_DrawSkyColumn_24 (void);
void    R_DrawFuzzColumn_24 (void);
void    R_DrawTranslucentColumn_24 (void);
void    R_DrawTranslatedColumn_24 (void);  // skin
void    R_DrawSpan_24 (void);

void    R_DrawTranslatedTranslucentColumn_24 (void);  // skin translucent
void    R_DrawShadeColumn_24(void);
void    R_DrawAlphaColumn_24(void);
void    R_DrawTranslucentSpan_24(void);
void    R_DrawFogSpan_24(void);
void    R_DrawFogColumn_24(void);

// ------------------
// 32bpp DRAWING CODE
// ------------------

void    R_DrawColumn_32 (void);
void    R_DrawSkyColumn_32 (void);
void    R_DrawFuzzColumn_32 (void);
void    R_DrawTranslucentColumn_32 (void);
void    R_DrawTranslatedColumn_32 (void);  // skin
void    R_DrawSpan_32 (void);

void    R_DrawTranslatedTranslucentColumn_32 (void);  // skin translucent
void    R_DrawShadeColumn_32(void);
void    R_DrawAlphaColumn_32(void);
void    R_DrawTranslucentSpan_32(void);
void    R_DrawFogSpan_32(void);
void    R_DrawFogColumn_32(void);

// =========================================================================
#endif  // R_DRAW_H
