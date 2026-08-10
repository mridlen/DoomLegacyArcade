// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: dehacked.h 1729 2025-03-06 13:15:29Z wesleyjohnson $
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
// $Log: dehacked.h,v $
// Revision 1.5  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/04/16 18:38:07  bpereira
//
// Revision 1.3  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Dehacked and BEX modifiers to the Doom object behavior, and Doom messages.
//
//-----------------------------------------------------------------------------

#ifndef DEHACKED_H
#define DEHACKED_H

#include "doomtype.h"

void DEH_Init(void);  // only once
// Initialize what is reported in Finalize
void DEH_Init_Load_Dehacked( void );
void DEH_Finalize( void );
// permission: 0=game, 1=adv, 2=language
//void DEH_LoadDehackedFile_by_name( char* filename, byte bex_permission );
void DEH_LoadDehackedLump( lumpnum_t lumpnum );
#ifdef BEX_LANGUAGE
void BEX_load_language( char * langname, byte bex_permission );
#endif

// Detection
extern uint16_t  deh_detect;  // BDTC_xxx detection bits

extern boolean  deh_loaded;
extern byte  thing_flags_valid_deh;  // thing flags altered (from DEH), boolean
extern byte  pars_valid_bex;  // have valid PAR values (from BEX), boolean

#endif
