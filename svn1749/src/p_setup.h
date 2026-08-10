// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_setup.h 1736 2025-03-14 08:15:57Z wesleyjohnson $
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
// $Log: p_setup.h,v $
// Revision 1.6  2002/07/26 15:21:36  hurdler
//
// Revision 1.5  2002/07/24 19:03:06  ssntails
// Added support for things to retain spawned Z position.
//
// Revision 1.4  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.3  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Setup a game, startup stuff.
//
//-----------------------------------------------------------------------------

#ifndef P_SETUP_H
#define P_SETUP_H

#include "doomtype.h"
#include "doomdata.h"
  // mapthing_t
#include "doomstat.h"
  // skill_e
#include "r_defs.h"
  // m_fixed.h, subsector_t

// Player spawn spots for deathmatch.
// Limited to 255, see numdmstarts.
#define MAX_DM_STARTS   64
extern  mapthing_t *    deathmatchstarts[MAX_DM_STARTS];
extern  byte            numdmstarts;
//extern  mapthing_t**    deathmatch_p;

// Animate flats using anim structure instead of using levelflat fields.
// MAP used flats lookup table
//
typedef struct
{
    // most often used reference first, keeps it aligned
    lumpnum_t   lumpnum;        // lump number of the flat (mod by animation)

    // for better packing, odd bytes last
    uint16_t    size_index;     // flat size index
    char        name[8];        // resource name from wad
} levelflat_t;

extern unsigned int    numlevelflats;
extern levelflat_t*    levelflats;

uint16_t P_flatsize_to_index( int flatsize, const char * name );
int P_AddLevelFlat( const char* flatname );
int P_PrecacheLevelFlats (void);
char * P_FlatNameForNum(int num);

extern uint16_t        nummapthings;
extern mapthing_t*     mapthings;

// From P_SetupLevel
extern lumpnum_t  level_lumpnum;  // for info and comparative savegame
extern char*  level_mapname;  // to savegame and info

// NOT called by W_Ticker. Fixme.
//  to_episode : change to episode num
//  to_map : change to map number
//  to_skill : change to skill
//  map_wadname : map command, load wad file
boolean P_SetupLevel (int      to_episode,
                      int      to_map,
                      skill_e  to_skill,
                      char*    map_wadname);


typedef struct {
  char *  mapname;  // map in string format
  byte    episode;  // episode number
  byte    map;      // map number
} level_id_t;

// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
//  wadfilename : filename of wad to be loaded 
//  firstmap_out : /*OUT*/  info about the first level map
boolean P_AddWadFile (char* wadfilename, /*OUT*/ level_id_t * firstmap_out );

subsector_t* R_PointInSubsector(fixed_t x, fixed_t y);


extern boolean  newlevel;
extern boolean  doom1level;

// Create a blockmap.
void P_create_blockmap( void );

#endif
