// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_spec.c 1745 2025-04-10 09:36:34Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: p_spec.c,v $
// Revision 1.45  2003/07/23 17:26:36  darkwolf95
// SetLineTexture function for Fraggle Script
//
// Revision 1.44  2003/06/14 04:29:43  ssntails
// Added linedef #s for vertical wind currents. 294 is up, 295 is down.
//
// Revision 1.43  2003/06/11 03:02:12  ssntails
// Vertical wind currents and friction capability on 3d floors
//
// Revision 1.42  2003/05/04 07:34:38  sburke
// Removed LONG swap of scroll_t->affectee, as it broke single-player mode.
//
// Revision 1.41  2003/05/04 02:39:34  sburke
// Need to use LONG(s->affectee) for big-endian machines.
//
// Revision 1.40  2002/09/26 22:54:15  ssntails
// Fixed bugs with new current/wind handling
//
// Revision 1.39  2002/09/25 18:25:21  ssntails
// Better wind/current support for 3d floors
//
// Revision 1.38  2002/09/25 16:38:35  ssntails
// Alpha support for trans 3d floors in software
//
// Revision 1.37  2002/07/26 15:21:36  hurdler
//
// Revision 1.36  2002/07/18 20:47:41  ssntails
// Fix for vertical currents
//
// Revision 1.35  2002/06/30 13:57:30  ssntails
// Added vertical water currents.
//
// Revision 1.34  2002/06/23 17:14:31  ssntails
// Fixed bug with animated flats
//
// Revision 1.33  2002/06/14 02:43:43  ssntails
// Instant-lower and instant-raise capability for sectors added.
//
// Revision 1.32  2002/01/12 02:21:36  stroggonmeth
// Revision 1.31  2001/08/19 20:41:04  hurdler
// Revision 1.30  2001/08/13 22:53:40  stroggonmeth
//
// Revision 1.29  2001/08/11 15:18:02  hurdler
// Add sector colormap in hw mode (first attempt)
//
// Revision 1.28  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.27  2001/06/16 08:07:55  bpereira
// Revision 1.26  2001/05/27 13:42:48  bpereira
//
// Revision 1.25  2001/05/03 18:23:30  crashrl
// corrected wrong comment concerning teamstartsec
//
// Revision 1.24  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.23  2001/03/21 18:24:38  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.22  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.21  2001/02/24 13:35:20  bpereira
//
// Revision 1.20  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.19  2000/11/21 21:13:17  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.18  2000/11/11 13:59:45  bpereira
// Revision 1.17  2000/11/04 16:23:43  bpereira
//
// Revision 1.16  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.15  2000/10/21 08:43:30  bpereira
//
// Revision 1.14  2000/10/07 20:36:13  crashrl
// Added deathmatch team-start-sectors via sector/line-tag and linedef-type 1000-1031
//
// Revision 1.13  2000/05/23 15:22:34  stroggonmeth
// Not much. A graphic bug fixed.
//
// Revision 1.12  2000/05/03 23:51:00  stroggonmeth
// Revision 1.11  2000/04/16 18:38:07  bpereira
// Revision 1.10  2000/04/15 22:12:57  stroggonmeth
//
// Revision 1.9  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.8  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.7  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.6  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.5  2000/04/06 20:54:28  hurdler
// Mostly remove warnings under windows
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Implements special effects:
//      Texture animation, height or lighting changes
//       according to adjacent sectors, respective
//       utility functions, etc.
//      Line Tag handling. Line and Sector triggers.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "p_setup.h"
  // levelflats for flat animation
#include "p_inter.h"
  // P_SetMessage
#include "g_game.h"
#include "r_data.h"
#include "m_random.h"
#include "m_swap.h"
  // LE_SWAP32

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
// SoM: 3/10/2000  Two extra includes.
#include "dstrings.h"
#include "r_main.h"

#include "t_script.h"
//#include "r_sky.h" // Portals

#include "hardware/hw3sound.h"

// Feature enables.

// [WDJ]
byte  friction_model = FR_legacy;

CV_PossibleValue_t zerotags_cons_t[]={
   {0,"Allow"},
   {1,"Boom"},
   {0,NULL}};
consvar_t cv_zerotags = {"zerotag","1", CV_NETVAR | CV_SAVE, zerotags_cons_t};
  // !comp[comp_zerotags]




//SoM: 3/7/2000
static void P_SpawnScrollers(void);

static void P_SpawnFriction(sector_t *);
static void P_SpawnPushers(void);
static void Add_Pusher(int type, fixed_t x_mag, fixed_t y_mag, mobj_t* source, int affectee); //SoM: 3/9/2000
static void P_FindAnimatedFlat (int i);


typedef struct {
    int  base_texnum;   // starting texture
    int  end_texnum;    // ending texture
} anim_texture_t;

typedef struct {
    // flat_ref is kept in animation sequence, so do not need animseq   
    levelflat_t *  flat_ref;  // the animated flat
} anim_flatlist_t;

typedef struct {
    lumpnum_t  base_flat_lumpnum;  // starting flat
    lumpnum_t  end_flat_lumpnum;   // ending flat
    anim_flatlist_t *  flat_list;  // array[numpics] entries (owned)
} anim_flat_t;

// 
//
// Animating textures and planes
// Interpic animation is in wi_stuff.
//
// [WDJ] 5/18/2010 Eliminate -1 value in boolean field.
typedef struct
{
    byte        istexture; // false= flat, true= texture
    byte        numpics;
    unsigned int  speed;	// in tics
    union {
      anim_texture_t  at;
      // [WDJ] A lumpnum_t might not fit into an int,
      // and might be encoded otherwise.
      anim_flat_t     af;
    };
} animtex_t;


//
//      source animation definition
//
// Used to read ANIMATED lump from wad, sizes must be correct.
// Also used for hardcoded tables.
#pragma pack(1) //Hurdler: 04/04/2000: I think pragma is more portable
typedef struct
{
    byte        istexture;      // 0= flat, 1= texture, 0xFF= end
    char        endname[9];
    char        startname[9];
    int32_t     speed;
} animdef_t; 
#pragma pack()



//SoM: 3/7/2000: New structure without limits.
static animtex_t*   anims = NULL;  // animations  (malloc, owned)
static animtex_t*   lastanim;  // next empty slot in anims
static size_t    num_anims_alloc;  // size of anims allocation

//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in the WAD file.
//
animdef_t    harddefs[] =
{
    // DOOM II flat animations.
    {false,     "NUKAGE3",      "NUKAGE1",      8},
    {false,     "FWATER4",      "FWATER1",      8},
    {false,     "SWATER4",      "SWATER1",      8},
    {false,     "LAVA4",        "LAVA1",        8},
    {false,     "BLOOD3",       "BLOOD1",       8},

    {false,     "RROCK08",      "RROCK05",      8},
    {false,     "SLIME04",      "SLIME01",      8},
    {false,     "SLIME08",      "SLIME05",      8},
    {false,     "SLIME12",      "SLIME09",      8},

    // animated textures
    {true,      "BLODGR4",      "BLODGR1",      8},
    {true,      "SLADRIP3",     "SLADRIP1",     8},

    {true,      "BLODRIP4",     "BLODRIP1",     8},
    {true,      "FIREWALL",     "FIREWALA",     8},
    {true,      "GSTFONT3",     "GSTFONT1",     8},
    {true,      "FIRELAVA",     "FIRELAV3",     8},
    {true,      "FIREMAG3",     "FIREMAG1",     8},
    {true,      "FIREBLU2",     "FIREBLU1",     8},
    {true,      "ROCKRED3",     "ROCKRED1",     8},

    {true,      "BFALL4",       "BFALL1",       8},
    {true,      "SFALL4",       "SFALL1",       8},
    {true,      "WFALL4",       "WFALL1",       8},
    {true,      "DBRAIN4",      "DBRAIN1",      8},

    // heretic 
    {false,     "FLTWAWA3",     "FLTWAWA1",     8}, // Water
    {false,     "FLTSLUD3",     "FLTSLUD1",     8}, // Sludge
    {false,     "FLTTELE4",     "FLTTELE1",     6}, // Teleport
    {false,     "FLTFLWW3",     "FLTFLWW1",     9}, // River - West
    {false,     "FLTLAVA4",     "FLTLAVA1",     8}, // Lava
    {false,     "FLATHUH4",     "FLATHUH1",     8}, // Super Lava
    {true,      "LAVAFL3",      "LAVAFL1",      6}, // Texture: Lavaflow
    {true,      "WATRWAL3",     "WATRWAL1",     4}, // Texture: Waterfall

    {0xFF}
};


//
//      Animating line specials
//

void P_Release_PicAnims(void)
{
    if( anims == NULL )  return;

    animtex_t * ap;
    for (ap = anims ; ap < lastanim ; ap++)
    {
        if( ! ap->istexture )
        {
            // Release flat list
            if( ap->af.flat_list )
                free( ap->af.flat_list );
        }
    }

    free( anims );
    anims = NULL;
}

//
// Init animated textures
// - now called at level loading P_SetupLevel()
//

static animdef_t * animdefs = NULL;

//SoM: 3/7/2000: Use new boom method of reading lump from wad file.
// Called by P_SetupLevel, before loading the level structures.
// [WDJ] 5/18/2010 Eliminate -1 value in boolean field, use lastanim as loop test.
void P_Init_PicAnims (void)
{
  //  Init animation
  int         i;

  P_Release_PicAnims();  // in case were not released before
   
  if( VALID_LUMP( W_CheckNumForName("ANIMATED") ) )
  {
    animdefs = (animdef_t *)W_CacheLumpName("ANIMATED", PU_IN_USE);
    // [WDJ] From wad, Do endian conversion on speed
// __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#ifdef __BIG_ENDIAN__
    // [WDJ] Endian conversion, only when BIG_ENDIAN, when from wad,
    // and not when cache hit.
    if( lump_read )
    {
       for (i = 0; animdefs[i].istexture != 0xFF; i++)
       {
          animdefs[i].speed = LE_SWAP32(animdefs[i].speed);
       }
    }
#endif
  }
  else
  {
    animdefs = harddefs;
    // [WDJ] Compiler source, do NOT endian convert speed
  }

  num_anims_alloc = 0;
  for (i = 0; animdefs[i].istexture != 0xFF; i++, num_anims_alloc++)	// count
     ;
  anims = (animtex_t *)malloc(sizeof(animtex_t) * num_anims_alloc);
  if( anims == NULL ) {
     I_Error( "Anims: memory allocation failure" );
  }

  lastanim = anims;
  for (i = 0; animdefs[i].istexture != 0xFF; i++)
  {
    // Anim defs that do not apply to this level are ignored.
    // Thus anims can be shorter than the num_anims_alloc.
    if (animdefs[i].istexture)
    {
      // different episode ?
      if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
          continue;

      lastanim->at.end_texnum = R_TextureNumForName (animdefs[i].endname);
      lastanim->at.base_texnum = R_TextureNumForName (animdefs[i].startname);
      lastanim->numpics = lastanim->at.end_texnum - lastanim->at.base_texnum + 1;
    }
    else
    {
      // FlatNumForName does not simply return NO_LUMP, so must check first.
      if( ! VALID_LUMP( W_CheckNumForName(animdefs[i].startname) ) )
          continue;

      if( ! VALID_LUMP( W_CheckNumForName(animdefs[i].endname) ) )
          continue;

      lastanim->af.end_flat_lumpnum = R_FlatNumForName (animdefs[i].endname);
      lastanim->af.base_flat_lumpnum = R_FlatNumForName (animdefs[i].startname);
      lastanim->numpics = LUMPNUM(lastanim->af.end_flat_lumpnum) - LUMPNUM(lastanim->af.base_flat_lumpnum) + 1;
      lastanim->af.flat_list = NULL;
    }


    lastanim->istexture = ( animdefs[i].istexture != 0 ); // char to boolean

    if (lastanim->numpics < 2)
    {
        I_SoftError ("P_InitPicAnims: animation %i, bad cycle from %s to %s",
            i, animdefs[i].startname, animdefs[i].endname);
        continue;
    }

    lastanim->speed = animdefs[i].speed * NEWTICRATERATIO;
    lastanim++;
  }

  if(animdefs != harddefs)
    Z_ChangeTag (animdefs, PU_LEVEL);  // are referenced
}

//  Check for flats in levelflats, that are part
//  of a flat anim sequence, if so, set them up for animation
//
// Called by P_SetupLevel->P_Setup_LevelFlatAnims, at end of LoadSectors.
//SoM: 3/16/2000: Changed parameter from pointer to "anims" entry number
static
void P_FindAnimatedFlat (int animnum)
{
    levelflat_t *  foundflats;
    animtex_t *    animp;
    anim_flatlist_t * flat_list;
    int    i, start_ln, end_ln, fln, wadnum;
    byte   found = 0;

    animp = & anims[animnum];
    lumpnum_t start_flat_lumpnum = animp->af.base_flat_lumpnum;
    lumpnum_t end_flat_lumpnum   = animp->af.end_flat_lumpnum;

    if( ! VALID_LUMP(start_flat_lumpnum) )
        return;

    if( ! VALID_LUMP(end_flat_lumpnum) )
        return;

    // [WDJ] No more tricky math that depends on lumpnum representation.
    start_ln = LUMPNUM(start_flat_lumpnum);
    end_ln = LUMPNUM(end_flat_lumpnum);
    wadnum = WADFILENUM(start_flat_lumpnum);
    if( WADFILENUM(end_flat_lumpnum) != wadnum )
    {
       I_SoftError ("AnimatedFlat start %s not in same wad as end %s\n",
                animdefs[animnum].startname, animdefs[animnum].endname);
       animp->af.base_flat_lumpnum = NO_LUMP;
       return;
    }

    if( animp->numpics > 250 )  // too many, something is wrong
        return;

    // Allocate the animation flat list.
    flat_list = animp->af.flat_list;
    if( flat_list == NULL )
    {
        unsigned int list_alloc_size = sizeof(anim_flatlist_t) * animp->numpics;
        flat_list = (anim_flatlist_t *) malloc( list_alloc_size );
        animp->af.flat_list = flat_list;
        if( flat_list == NULL )
            return; // allocation error
        memset( flat_list, 0, list_alloc_size );
    }

    //
    // Search through the levelflats, find if this anim flat sequence is used.
    //
    foundflats = levelflats;
    for (i = 0; i<numlevelflats; i++, foundflats++)
    {
        // is that levelflat from the flat anim sequence ?
        if( WADFILENUM(foundflats->lumpnum) != wadnum )  // rare
            continue;

        fln = LUMPNUM(foundflats->lumpnum);
        if( fln >= start_ln && fln <= end_ln )
        {
            // Found a usage.  Let the found code below, fill in all flats.
            found = 1;
            break;
        }
    }
   
    // Make sure that all flats of an animation are present.
    if( found )
    {
        // P_AddLevelFlat may move the levelflats.
        // After that happens, this will need to update bad links,
        // so do not test the links,
        // just replace all links to the levelflats.
        lumpinfo_t * anim_lumpinfo = wadfiles[wadnum]->lumpinfo;

        if (devparm)
        {
            GenPrintf(EMSG_dev,
                "animflat: numpics=%d speed=%d\n", animp->numpics, animp->speed);
        }

        int animseq;
        for( animseq=0; animseq<animp->numpics; animseq++ )
        {
            // [WDJ] Get levelflat_id.
            // If this flat is not in the level flats, it will be added.
            int flat_lumpnum = start_ln + animseq;  // index in the anim seq
            lumpinfo_t * lump_p = & anim_lumpinfo[flat_lumpnum];
            unsigned int levelflat_id = P_AddLevelFlat( lump_p->name );
            // Found it, or created it.  Does not return an error.
            // This may have moved levelflats, that is dealt with by P_Setup_LevelFlatAnims.

            // The flat_list is in animseq order.	   
            flat_list[animseq].flat_ref = & levelflats[ levelflat_id ];

            if (devparm)
            {
                GenPrintf(EMSG_dev,
                   "  [%2d]  %#03d name:%.8s\n",
                   animseq, levelflat_id, levelflats[ levelflat_id ].name );
            }
        }
    }
}


//
//  Called by P_LoadSectors
//
// Called at end of P_LoadSectors
void P_Setup_LevelFlatAnims (void)
{
    levelflat_t *  old_levelflats = levelflats;

    // [WDJ] 5/18/2010 Eliminate -1 value in boolean field.
    // List can be shorter than num_anims_alloc because it is missing entries
    // for flats and textures that do not appear in this level.
    int  animlen = lastanim - anims;  // count of entries

    byte  loop_limit = 3;
    while( loop_limit -- )
    {
        // the original game flat anim sequences
        int  i;
        for (i=0 ; i<animlen; i++)
        {
            if (!anims[i].istexture)  // flats
                P_FindAnimatedFlat (i);
        }

        if( levelflats == old_levelflats )
            break;   // Good, we are done
       
        // [WDJ] P_AddLevelFlats moved the levelflats.
        // Must update all the animations again.
        // The second time, it will not be adding any flats.
        old_levelflats = levelflats;
    }
// [WDJ] If fragglescript changes a flat to an animated flat
// that has not been found, it will not be animated.
// The level map needs to have the animated flat in at least one sector.
}



//
// UTILITIES
//


//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the linelist index, and the side (0/1) that you want.
//
side_t*  getSide ( int currentSector, int linelisti, int side )
{
    return &sides[ (sectors[currentSector].linelist[linelisti])->sidenum[side] ];
}


//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the linelist index and the side (0/1) that you want.
//
sector_t*  getSector ( int currentSector, int linelisti, int side )
{
    return sides[ (sectors[currentSector].linelist[linelisti])->sidenum[side] ].sector;
}


//
// twoSided()
// Given the sector number and the linelist index,
//  it will tell you whether the line is two-sided or not.
//
//SoM: 3/7/2000: Use the boom method
int  twoSided ( int sector, int linelisti )
{
  return EN_boom_physics?
    ((sectors[sector].linelist[linelisti])->sidenum[1] != NULL_INDEX)
    :
    ((sectors[sector].linelist[linelisti])->flags & ML_TWOSIDED);  // Doom, Heretic
}




//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
//SoM: 3/7/2000: Use boom method.
sector_t*  getNextSector ( line_t* line,  sector_t* sec )
{
  if(!EN_boom_physics)
  {
    // Doom, Heretic
    // Not needed with Boom backsector test returning NULL.
    if (!(line->flags & ML_TWOSIDED))
      return NULL;
  }

  if (line->frontsector == sec)
  {
    // Boom, do not repeat same sector, see floor->highest_floor.
    if( EN_boom_physics && line->backsector==sec )
      return NULL;
    return line->backsector;  // Doom, Heretic
  }
  return line->frontsector;
}




//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindLowestFloorSurrounding(sector_t* sec)
{
    int                 i;
    line_t*             check;
    sector_t*           other;
    fixed_t             lowfloor = sec->floorheight;

    for (i=0; i < sec->linecount ; i++)
    {
        // for each line in the sector linelist
        check = sec->linelist[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        // find any lower floor
        if (other->floorheight < lowfloor)
            lowfloor = other->floorheight;
    }
    return lowfloor;
}




//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
    int                 i;
    line_t*             check;
    sector_t*           other;
    fixed_t             highfloor = -FIXED_MAX;

#if 0
    // DoomLegacy uses -FIXED_MAX, and assumes there will be an adjacent sector.
    // [WDJ] From Boom, prevent overflow.
    highfloor = EN_boom_physics? -32000*FRACUNIT : -500*FRACUNIT;
#endif

    for (i=0 ;i < sec->linecount ; i++)
    {
        // for each line in sector linelist
        check = sec->linelist[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        // find any higher other floor
        if (other->floorheight > highfloor)
            highfloor = other->floorheight;
    }
    return highfloor;
}



//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
// SoM: 3/7/2000: Use Lee Killough's version insted.
// Rewritten by Lee Killough to avoid fixed array and to be faster
//
fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0; i < sec->linecount ; i++)
  {
      // for each line in sector linelist
    if ((other = getNextSector(sec->linelist[i],sec)) &&
         other->floorheight > currentheight)
    {
      int height = other->floorheight;
      while (++i < sec->linecount)
      {
        // for rest of lines in sector linelist
        if ((other = getNextSector(sec->linelist[i],sec))
            && other->floorheight < height
            && other->floorheight > currentheight)
          height = other->floorheight;
      }
      return height;
    }
  }
  return currentheight;
}


////////////////////////////////////////////////////
// SoM: Start new Boom functions
////////////////////////////////////////////////////

// P_FindNextLowestFloor()
//
// Passed a sector and a floor height, returns the fixed point value
// of the largest floor height in a surrounding sector smaller than
// the floor height passed. If no such height exists the floorheight
// passed is returned.
//
//
fixed_t P_FindNextLowestFloor(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
  {
    // for all lines in sector linelist
    if ((other = getNextSector(sec->linelist[i],sec))
        && other->floorheight < currentheight)
    {
      int height = other->floorheight;
      while (++i < sec->linecount)
      {
        // for rest of lines in sector linelist
        if ((other = getNextSector(sec->linelist[i],sec))
            && other->floorheight > height
            && other->floorheight < currentheight)
        {
          height = other->floorheight;
        }
      }
      return height;
    }
  } // for
  return currentheight;
}


//
// P_FindNextLowestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the largest ceiling height in a surrounding sector smaller than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
//
fixed_t P_FindNextLowestCeiling(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
  {
    // for all lines in sector linelist
    if ((other = getNextSector(sec->linelist[i],sec))
        && other->ceilingheight < currentheight)
    {
      int height = other->ceilingheight;
      while (++i < sec->linecount)
      {
        // for rest of lines in sector linelist
        if ((other = getNextSector(sec->linelist[i],sec))
            && other->ceilingheight > height
            && other->ceilingheight < currentheight)
        {
          height = other->ceilingheight;
        }
      }
      return height;
    }
  }
  return currentheight;
}




//
// P_FindNextHighestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the smallest ceiling height in a surrounding sector larger than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
//
fixed_t P_FindNextHighestCeiling(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
  {
    // for all lines in sector linelist
    if ((other = getNextSector(sec->linelist[i],sec))
        && other->ceilingheight > currentheight)
    {
      int height = other->ceilingheight;
      while (++i < sec->linecount)
      {
        // for rest of lines in sector linelist
        if ((other = getNextSector(sec->linelist[i],sec))
            && other->ceilingheight < height
            && other->ceilingheight > currentheight)
        {
          height = other->ceilingheight;
        }
      }
      return height;
    }
  }
  return currentheight;
}

////////////////////////////
// End New Boom functions
////////////////////////////



//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t
P_FindLowestCeilingSurrounding(sector_t* sec)
{
    int                 i;
    line_t*             check;
    sector_t*           other;
    fixed_t             height = FIXED_MAX;

    // [WDJ] removed extra foundsector test that defeated this limit.
    if(EN_boom_physics)  height = 32000*FRACUNIT; //SoM: 3/7/2000: Remove ovf
                                              
    for (i=0 ;i < sec->linecount ; i++)
    {
        // for all lines in sector linelist
        check = sec->linelist[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        // find any lower ceiling
        if (other->ceilingheight < height )
            height = other->ceilingheight;
    }
    return height;
}


//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindHighestCeilingSurrounding(sector_t* sec)
{
    int         i;
    line_t*     check;
    sector_t*   other;
    fixed_t     height = -FIXED_MAX;

#if 0
    // DoomLegacy uses -FIXED_MAX, and assumes there will be an adjacent sector.
    // [WDJ] From Boom, prevent overflow.
    height = EN_boom_physics? -32000*FRACUNIT : 0;
#endif

    for (i=0 ;i < sec->linecount ; i++)
    {
        // for all lines in sector linelist
        check = sec->linelist[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        // find any line with higher ceiling
        if (other->ceilingheight > height)
            height = other->ceilingheight;
    }
    return height;
}



//SoM: 3/7/2000: UTILS.....
//
// P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
//
fixed_t P_FindShortestTextureAround(int secnum)
{
  fixed_t minsize = FIXED_MAX;
  side_t*     side;
  int i;
  sector_t *sec = &sectors[secnum];

  // Boom (jff): prevent overflow in height calcs.
  if( EN_boom_physics )
    minsize = 32000<<FRACBITS;

  for (i = 0; i < sec->linecount; i++)
  {
    // sector line list [i]
    if (twoSided(secnum, i))
    {
      // two sided line, list index i
      side = getSide(secnum,i,0);
      if (side->bottomtexture > 0)
      {
        if (textureheight[side->bottomtexture] < minsize)
          minsize = textureheight[side->bottomtexture];
      }
      side = getSide(secnum,i,1);
      if (side->bottomtexture > 0)
      {
        if (textureheight[side->bottomtexture] < minsize)
          minsize = textureheight[side->bottomtexture];
      }
    }
  }
  return minsize;
}



//SoM: 3/7/2000: Stuff.... (can you tell I'm getting tired? It's 12:30!)
//
// P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
//
fixed_t P_FindShortestUpperAround(int secnum)
{
  fixed_t minsize = FIXED_MAX;
  side_t*     side;
  int i;
  sector_t *sec = &sectors[secnum];

  // Boom (jff): prevent overflow in height calcs.
  if( EN_boom_physics )
    minsize = 32000<<FRACBITS;

  for (i = 0; i < sec->linecount; i++)
  {
    if (twoSided(secnum, i))
    {
      side = getSide(secnum,i,0);
      if (side->toptexture > 0)
      {
        if (textureheight[side->toptexture] < minsize)
          minsize = textureheight[side->toptexture];
      }
      side = getSide(secnum,i,1);
      if (side->toptexture > 0)
      {
        if (textureheight[side->toptexture] < minsize)
          minsize = textureheight[side->toptexture];
      }
    }
  }
  return minsize;
}




//SoM: 3/7/2000
//
// P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
sector_t * P_FindModelFloorSector(fixed_t floordestheight, int secnum)
{
  int i;
  sector_t *sec=NULL;
  int linecount;

  sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < ((!EN_boom && sec->linecount<linecount)?
                   sec->linecount : linecount); i++)
  {
    if ( twoSided(secnum, i) )
    {
#if 1
      // [WDJ] Improved execution, no division needed.
      sec = getSector(secnum,i,
                     ((getSide(secnum,i,0)->sector == &sectors[secnum])? 1:0 )
                     );
#else
      // Boom original uses unnecessary division.
      if (getSide(secnum,i,0)->sector-sectors == secnum)
          sec = getSector(secnum,i,1);
      else
          sec = getSector(secnum,i,0);
#endif

      if (sec->floorheight == floordestheight)
        return sec;
    }
  }
  return NULL;
}



//SoM: 3/7/2000: Last one...
//
// P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
sector_t *P_FindModelCeilingSector(fixed_t ceildestheight,int secnum)
{
  int i;
  sector_t *sec=NULL;
  int linecount;

  sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < (!EN_boom && sec->linecount<linecount?
                   sec->linecount : linecount); i++)
  {
    if ( twoSided(secnum, i) )
    {
      if (getSide(secnum,i,0)->sector-sectors == secnum)
          sec = getSector(secnum,i,1);
      else
          sec = getSector(secnum,i,0);

      if (sec->ceilingheight == ceildestheight)
        return sec;
    }
  }
  return NULL;
}


// [WDJ] Fixed horrible and buggy tag hash, sometimes only had two bins.
// Make it a fixed size with a fixed mask.
// Prboom fixed this by using a modulo instead of a mask.
#define SECTOR_TAGHASH_SIZE  256
#define LINE_TAGHASH_SIZE  64
#define SECTOR_TAGHASH( tag )   (((unsigned int)(tag)) & (SECTOR_TAGHASH_SIZE-1))
#define LINE_TAGHASH( tag )   (((unsigned int)(tag)) & (LINE_TAGHASH_SIZE-1))
int32_t  sector_taghash_secnum[SECTOR_TAGHASH_SIZE];
int32_t  line_taghash_linenum[LINE_TAGHASH_SIZE];

//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//
//SoM: 3/7/2000: Killough wrote this to improve the process.
//  line : the linedef with the search tag
//  start : -1 first iteration, previous return value otherwise.
// Return sector index.
// Return -1 when not found
int  P_FindSectorFromLineTag ( line_t* line, int start )
{
  start = (start >= 0) ?
    sectors[start].nexttag
    : sector_taghash_secnum[ SECTOR_TAGHASH( line->tag )];
  while (start >= 0 && sectors[start].tag != line->tag)
    start = sectors[start].nexttag;
  return start;
}

//
// P_FindSectorFromTag
// Used by FraggleScript
// Return -1 when not found
int  P_FindSectorFromTag( uint16_t tag, int start )
{
  start = (start >= 0) ?
    sectors[start].nexttag
    : sector_taghash_secnum[ SECTOR_TAGHASH( tag )];
  while (start >= 0 && sectors[start].tag != tag)
    start = sectors[start].nexttag;
  return start;
}

//DarkWolf95:July 23, 2003: Needed for SF_SetLineTexture
// Return -1 when not found
int P_FindLineFromTag( uint16_t tag, int start)
{
  start = (start >= 0) ?
     lines[start].nexttag
     : line_taghash_linenum[ LINE_TAGHASH( tag )];
  while (start >= 0 && lines[start].tag != tag)
    start = lines[start].nexttag;
  return start;
}

//SoM: 3/7/2000: More boom specific stuff...
// killough 4/16/98: Same thing, only for linedefs
//Return -1 when not found
int P_FindLineFromLineTag(const line_t *line, int start)
{
  start = (start >= 0) ?
     lines[start].nexttag
     : line_taghash_linenum[ LINE_TAGHASH( line->tag )];
  while (start >= 0 && lines[start].tag != line->tag)
    start = lines[start].nexttag;
  return start;
}


//SoM: 3/7/2000: Oh joy!
// Hash the sector tags across the sectors and linedefs.
static void P_Init_TagLists(void)
{
  int i;
  for (i=SECTOR_TAGHASH_SIZE-1; i>=0; i-- )
  {
      sector_taghash_secnum[i] = -1;
  }
  for (i=LINE_TAGHASH_SIZE-1; i>=0; i-- )
  {
      line_taghash_linenum[i] = -1;
  }

  // reversed for
  for (i=numsectors-1; i>=0; i-- )
  {
      if( sectors[i].tag == 0 && !cv_zerotags.EV )  continue;
      unsigned int j = SECTOR_TAGHASH( sectors[i].tag );
      sectors[i].nexttag = sector_taghash_secnum[j];
      sector_taghash_secnum[j] = i;
  }

  // reversed for
  for (i=numlines-1; i>=0; i-- )
  {
      if( lines[i].tag == 0 && !cv_zerotags.EV )  continue;
      unsigned int j = LINE_TAGHASH( lines[i].tag );
      lines[i].nexttag = line_taghash_linenum[j];
      line_taghash_linenum[j] = i;
  }
}




//
// Find minimum light from an adjacent sector
//
// max is the default value
lightlev_t  P_FindMinSurroundingLight ( sector_t*  sector, lightlev_t max )
{
    int         i;
    lightlev_t  min;
    line_t*     line;
    sector_t*   check;

    min = max;  // initial value
    for (i=0 ; i < sector->linecount ; i++)
    {
        // for all lines in sector linelist
        line = sector->linelist[i];
        check = getNextSector(line,sector);

        if (!check)
            continue;

        // find any lower light level
        if (check->lightlevel < min)
            min = check->lightlevel;
    }
    return min;
}



//SoM: 3/7/2000
//
// P_CanUnlockGenDoor()
//
// Passed a generalized locked door linedef and a player, returns whether
// the player has the keys necessary to unlock that door.
//
// Note: The linedef passed MUST be a generalized locked door type
//       or results are undefined.
//
//
boolean P_CanUnlockGenDoor( line_t* line, player_t* player)
{
  char * msg;
  // does this line special distinguish between skulls and keys?
  int skulliscard = (line->special & LockedNKeys)>>LockedNKeysShift;

  // determine for each case of lock type if player's keys are adequate
  switch((line->special & LockedKey)>>LockedKeyShift)
  {
    case DKY_anykey:
      if
      (
        !(player->cards & it_redcard) &&
        !(player->cards & it_redskull) &&
        !(player->cards & it_bluecard) &&
        !(player->cards & it_blueskull) &&
        !(player->cards & it_yellowcard) &&
        !(player->cards & it_yellowskull)
      )
      {
        msg = PD_ANY;
        goto oof_blocked;	
      }
      break;
    case DKY_R_card:
      if
      (
        !(player->cards & it_redcard) &&
        (!skulliscard || !(player->cards & it_redskull))
      )
      {
        msg = skulliscard? PD_REDK : PD_REDC;
        goto oof_blocked;	
      }
      break;
    case DKY_B_card:
      if
      (
        !(player->cards & it_bluecard) &&
        (!skulliscard || !(player->cards & it_blueskull))
      )
      {
        msg = skulliscard? PD_BLUEK : PD_BLUEC;
        goto oof_blocked;	
      }
      break;
    case DKY_Y_card:
      if
      (
        !(player->cards & it_yellowcard) &&
        (!skulliscard || !(player->cards & it_yellowskull))
      )
      {
        msg = skulliscard? PD_YELLOWK : PD_YELLOWC;
        goto oof_blocked;	
      }
      break;
    case DKY_R_skull:
      if
      (
        !(player->cards & it_redskull) &&
        (!skulliscard || !(player->cards & it_redcard))
      )
      {
        msg = skulliscard? PD_REDK : PD_REDS;
        goto oof_blocked;	
      }
      break;
    case DKY_B_skull:
      if
      (
        !(player->cards & it_blueskull) &&
        (!skulliscard || !(player->cards & it_bluecard))
      )
      {
        msg = skulliscard? PD_BLUEK : PD_BLUES;
        goto oof_blocked;	
      }
      break;
    case DKY_Y_skull:
      if
      (
        !(player->cards & it_yellowskull) &&
        (!skulliscard || !(player->cards & it_yellowcard))
      )
      {
        msg = skulliscard? PD_YELLOWK : PD_YELLOWS;
        goto oof_blocked;	
      }
      break;
    case DKY_allkeys:
      if
      (
        !skulliscard &&
        (
          !(player->cards & it_redcard) ||
          !(player->cards & it_redskull) ||
          !(player->cards & it_bluecard) ||
          !(player->cards & it_blueskull) ||
          !(player->cards & it_yellowcard) ||
          !(player->cards & it_yellowskull)
        )
      )
      {
        msg = PD_ALL6;
        goto oof_blocked;	
      }
      if
      (
        skulliscard &&
        (
          (!(player->cards & it_redcard) &&
            !(player->cards & it_redskull)) ||
          (!(player->cards & it_bluecard) &&
            !(player->cards & it_blueskull)) ||
          (!(player->cards & it_yellowcard) &&
            !(player->cards & it_yellowskull))
          // [WDJ] No compatibility with MBF 3 key bug.
        )
      )
      {
        msg = PD_ALL3;
        goto oof_blocked;	
      }
      break;
  }
  return true;

oof_blocked:
  P_SetMessage( player, msg, 31 );
  S_StartScreamSound(player->mo, sfx_oof);
  return false;
}





//
// P_SectorActive()
//
// Passed a linedef special class (floor, ceiling, lighting) and a sector
// returns whether the sector is already busy with a linedef special of the
// same class.
//
//
boolean P_SectorActive( sector_special_e spt, sector_t *sec)
{
  if (!EN_boom)
  {
    // For old demo compatibility, all linedef special classes
    // are the same.
    return sec->floordata || sec->ceilingdata || sec->lightingdata;
  }
  else
  {
    switch (spt)
    {
      case S_floor_special:
        return sec->floordata != NULL;
      case S_ceiling_special:
        return sec->ceilingdata != NULL;
      case S_lighting_special:
        return sec->lightingdata != NULL;
    }
  }
  return true;
}


//SoM: 3/7/2000
//
// P_CheckTag()
//
// Passed a line, returns true if the tag is non-zero or the line special
// allows no tag without harm. If compatibility, all linedef specials are
// allowed to have zero tag.
//
// Note: Only line specials activated by walkover, pushing, or shooting are
//       checked by this routine.
//
//
int P_CheckTag(line_t *line)
{
  // Only called when EN_boom.

  if( line->tag )
    return 1;

  switch(line->special)
  {
    case 1:                 // Manual door specials
    case 26:
    case 27:
    case 28:
    case 31:
    case 32:
    case 33:
    case 34:
    case 117:
    case 118:

    case 139:               // Lighting specials
    case 170:
    case 79:
    case 35:
    case 138:
    case 171:
    case 81:
    case 13:
    case 192:
    case 169:
    case 80:
    case 12:
    case 194:
    case 173:
    case 157:
    case 104:
    case 193:
    case 172:
    case 156:
    case 17:

    case 195:               // Thing teleporters
    case 174:
    case 97:
    case 39:
    case 126:
    case 125:
    case 210:
    case 209:
    case 208:
    case 207:

    case 11:                // Exits
    case 52:
    case 197:
    case 51:
    case 124:
    case 198:

    case 48:                // Scrolling walls
    case 85:
    // FraggleScript types!
    case 272:   // WR
    case 273:
    case 274:   // W1
    case 275:
    case 276:   // SR
    case 277:   // S1
    case 278:   // GR
    case 279:   // G1
      return 1;   // zero tag allowed

#if 0
    // Heretic cannot call this function.
    // Heretic 105 does not use tag.
    // Boom 105 does not allow zero tag, return 0.
    case 105:
      if( EN_heretic )
        return 1;
#endif

    default:
      break;
  }
  return 0;       // zero tag not allowed
}



//SoM: 3/7/2000: Is/WasSecret.
// [WDJ] Unused
//
// P_IsSecret()
//
// Passed a sector, returns if the sector secret type is still active, i.e.
// secret type is set and the secret has not yet been obtained.
//
boolean P_IsSecret(sector_t *sec)
{
  return (sec->special==9 || (sec->special&SECRET_MASK));
}


//
// P_WasSecret()
// [WDJ] Unused
//
// Passed a sector, returns if the sector secret type is was active, i.e.
// secret type was set and the secret has been obtained already.
//
boolean P_WasSecret(sector_t *sec)
{
  return (sec->oldspecial==9 || (sec->oldspecial&SECRET_MASK));
}


//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void
P_CrossSpecialLine ( line_t * line, int side, mobj_t* thing )
{
    int         ok;
    int         forceuse; //SoM: 4/26/2000: ALLTRIGGER should allow monsters to use generalized types too!

    forceuse = line->flags & ML_ALLTRIGGER && thing->type != MT_BLOOD;

    //  Triggers that other things can activate
    if (EN_doom_etc && !thing->player)
    {
        // Things that should NOT trigger specials...
        switch(thing->type)
        {
          case MT_ROCKET:
          case MT_PLASMA:
          case MT_BFG:
          case MT_TROOPSHOT:
          case MT_HEADSHOT:
          case MT_BRUISERSHOT:
            return;
            break;

          default: break;
        }
    }

    //SoM: 3/7/2000: Check for generalized line types/
    if (EN_boom)
    {
      // pointer to line function is NULL by default, set non-null if
      // line special is walkover generalized linedef type
      int (*linefunc)(line_t *line)=NULL;
  
      // check each range of generalized linedefs
      if ((unsigned)line->special >= (GenFloorBase+0x2000))
      {}  // not Boom generalized
      else if ((unsigned)line->special >= GenFloorBase)
      {
        if (!thing->player)
        {
          if (((line->special & FloorChange) || !(line->special & FloorModel)) && !forceuse)
            return;     // FloorModel is "Allow Monsters" if FloorChange is 0
        }
        if (!line->tag)
          return;
        linefunc = EV_DoGenFloor;
      }
      else if ((unsigned)line->special >= GenCeilingBase)
      {
        if (!thing->player)
        {
          if (((line->special & CeilingChange) || !(line->special & CeilingModel)) && !forceuse)
            return;     // CeilingModel is "Allow Monsters" if CeilingChange is 0
        }
        if (!line->tag)
          return;
        linefunc = EV_DoGenCeiling;
      }
      else if ((unsigned)line->special >= GenDoorBase)
      {
        if (!thing->player)
        {
          if (!(line->special & DoorMonster) && !forceuse)
            return;                    // monsters disallowed from this door
          if (line->flags & ML_SECRET) // they can't open secret doors either
            return;
        }
        if (!line->tag)
          return;
        linefunc = EV_DoGenDoor;
      }
      else if ((unsigned)line->special >= GenLockedBase)
      {
        if (!thing->player)
          return;                     // monsters disallowed from unlocking doors
        if (((line->special&TriggerType)==TRIG_WalkOnce)
            || ((line->special&TriggerType)==TRIG_WalkMany))
        {
          if (!P_CanUnlockGenDoor(line,thing->player))
            return;
        }
        else
          return;
        linefunc = EV_DoGenLockedDoor;
      }
      else if ((unsigned)line->special >= GenLiftBase)
      {
        if (!thing->player)
        {
          if (!(line->special & LiftMonster) && !forceuse)
            return; // monsters disallowed
        }
        if (!line->tag)
          return;
        linefunc = EV_DoGenLift;
      }
      else if ((unsigned)line->special >= GenStairsBase)
      {
        if (!thing->player)
        {
          if (!(line->special & StairMonster) && !forceuse)
            return; // monsters disallowed
        }
        if (!line->tag)
          return;
        linefunc = EV_DoGenStairs;
      }
      else if ((unsigned)line->special >= GenCrusherBase)
      {
        if (!thing->player)
        {
          if (!(line->special & StairMonster) && !forceuse)
            return; // monsters disallowed
        }
        if (!line->tag)
          return;
        linefunc = EV_DoGenCrusher;
      }
  
      if (linefunc) // if it was a valid generalized type
      {
        switch((line->special & TriggerType) >> TriggerTypeShift)
        {
          case TRIG_WalkOnce:
            if (linefunc(line))
              line->special = 0;    // clear special if a walk once type
            return;
          case TRIG_WalkMany:
            linefunc(line);
            return;
          default:                  // if not a walk type, do nothing here
            return;
        }
      }
    } // EN_boom


    if(!thing->player)
    {
        ok = 0;
        if( EN_heretic
            && (line->special == 4 || line->special==39 || line->special == 97) )
        {
            ok = 1;
        }
        else
        {
          switch(line->special)
          {
             // Doom monster triggered special linedefs
           case 39:      // TELEPORT TRIGGER
           case 97:      // TELEPORT RETRIGGER
           case 125:     // TELEPORT MONSTERONLY TRIGGER
           case 126:     // TELEPORT MONSTERONLY RETRIGGER
           case 4:       // RAISE DOOR
           case 10:      // PLAT DOWN-WAIT-UP-STAY TRIGGER
           case 88:      // PLAT DOWN-WAIT-UP-STAY RETRIGGER
             ok = 1;
             break;
             // SoM: 3/4/2000: Add boom compatibility for extra monster usable
             // linedef types.
           case 208:     //SoM: Silent thing teleporters
           case 207:
           case 243:     //Silent line to line teleporter
           case 244:     //Same as above but trigger once.
           case 262:     //Same as 243 but reversed
           case 263:     //Same as 244 but reversed
           case 264:     //Monster only, silent, trigger once, reversed
           case 265:     //Same as 264 but repeatable
           case 266:     //Monster only, silent, trigger once
           case 267:     //Same as 266 bot repeatable
           case 268:     //Monster only, silent, trigger once, set pos to thing
           case 269:     //Monster only, silent, repeatable, set pos to thing
             if(EN_boom)
               ok = 1;
             break;
          }
        } // if else
        //SoM: Anything can trigger this line!
        if(line->flags & ML_ALLTRIGGER)
          ok = 1;

        if (!ok)
            return;
    } // ! player

    if( EN_boom && (cv_zerotags.EV > 0) && !P_CheckTag(line) )
      return;

    // Doom special linedefs
    // Note: could use some const's here.
    switch (line->special)
    {
        // W1 TRIGGERS, mostly
      case 2:
        // Open Door
        if(EV_DoDoor( line, VD_dooropen, VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 3:
        // Close Door
        if(EV_DoDoor( line, VD_doorclose, VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 4:
        // Raise Door
        if(EV_DoDoor( line, VD_normalDoor, VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 5:
        // Raise Floor
        if(EV_DoFloor( line, FT_raiseFloor) )  goto W1clear;
        goto W1fail;

      case 6:
        // Fast Ceiling Crush & Raise
        if(EV_DoCeiling( line, CT_fastCrushAndRaise) )  goto W1clear;
        goto W1fail;

      case 8:
        // Build Stairs
        if(EV_BuildStairs(line, (EN_heretic)? 8*FRACUNIT : ST_build8) )
            goto W1clear;
        goto W1fail;

      case 10:
        // PlatDownWaitUp
        if(EV_DoPlat( line, PLATT_downWaitUpStay, 0) )  goto W1clear;
        goto W1fail;

      case 12:
        // Light Turn On - brightest near
        if(EV_LightTurnOn(line,0) )  goto W1clear;
        goto W1fail;

      case 13:
        // Light Turn On 255
        if(EV_LightTurnOn(line,255) )  goto W1clear;
        goto W1fail;

      case 16:
        // Close Door 30
        if(EV_DoDoor( line, VD_close30ThenOpen, VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 17:
        // Start Light Strobing
        if(EV_StartLightStrobing(line) )  goto W1clear;
        goto W1fail;

      case 19:
        // Lower Floor
        if(EV_DoFloor( line, FT_lowerFloor) )  goto W1clear;
        goto W1fail;

      case 22:
        // Raise floor to nearest height and change texture
        if(EV_DoPlat( line, PLATT_raiseToNearestAndChange, 0) )  goto W1clear;
        goto W1fail;

      case 25:
        // Ceiling Crush and Raise
        if(EV_DoCeiling( line, CT_crushAndRaise) )  goto W1clear;
        goto W1fail;

      case 30:
        // Raise floor to shortest texture height
        //  on either side of lines.
        if(EV_DoFloor( line, FT_raiseToTexture) )  goto W1clear;
        goto W1fail;

      case 35:
        // Lights Very Dark
        if(EV_LightTurnOn(line,35) )  goto W1clear;
        goto W1fail;

      case 36:
        // Lower Floor (TURBO)
        if(EV_DoFloor( line, FT_turboLower) )  goto W1clear;
        goto W1fail;

      case 37:
        // LowerAndChange
        if(EV_DoFloor( line, FT_lowerAndChange) )  goto W1clear;
        goto W1fail;

      case 38:
        // Lower Floor To Lowest
        if(EV_DoFloor( line, FT_lowerFloorToLowest ) )  goto W1clear;
        goto W1fail;

      case 39:
        // TELEPORT!
        if(EV_Teleport( line, side, thing ) )  goto W1clear;
        goto W1fail;

      case 40:
        // RaiseCeilingLowerFloor, but actually is RaiseCeilingToHighest
        if( EN_boom )
        {
            // Highest Neighbor ceiling, slow.
            if(EV_DoCeiling( line, CT_raiseToHighest ) )  goto W1clear;
            // Boom does not execute EV_DoFloor() here because for Boom it
            // would succeed, making this linedef behave different than Doom.
            // Boom can have simultaneous ceiling and floor operations.
        }
        else
        {
            // Doom, Heretic	   
            EV_DoCeiling( line, CT_raiseToHighest );
#if 0
            // The docs do not mention a floor movement, but this code is
            // in prboom and chocolate-doom.
            // Because the ceiling just created a thinker for all the same
            // sectors, this floor operation will always be ignored.
            // Disabled this code to ensure that it stays non-operative.
            EV_DoFloor( line, FT_lowerFloorToLowest );
#endif
            goto W1clear;
        }
        break;

      case 44:
        // Ceiling Crush
        if(EV_DoCeiling( line, CT_lowerAndCrush ) )  goto W1clear;
        goto W1fail;

      case 52:
        // EXIT!
        if( cv_allowexitlevel.EV )
        {
            G_ExitLevel ();
            line->special = 0;  // heretic have right
        }
        break;

      case 53:
        // Perpetual Platform Raise
        if(EV_DoPlat( line, PLATT_perpetualRaise, 0) )  goto W1clear;
        goto W1fail;

      case 54:
        // Platform Stop
        if(EV_StopPlat(line) )  goto W1clear;
        goto W1fail;

      case 56:
        // Raise Floor Crush
        if(EV_DoFloor( line, FT_raiseFloorCrush) )  goto W1clear;
        goto W1fail;

      case 57:
        // Ceiling Crush Stop
        if(EV_CeilingCrushStop(line) )  goto W1clear;
        goto W1fail;

      case 58:
        // Raise Floor 24
        if(EV_DoFloor( line, FT_raiseFloor24) )  goto W1clear;
        goto W1fail;

      case 59:
        // Raise Floor 24 And Change
        if(EV_DoFloor( line, FT_raiseFloor24AndChange) )  goto W1clear;
        goto W1fail;

      case 104:
        // Turn lights off in sector(tag)
        if(EV_TurnTagLightsOff(line) )  goto W1clear;
        goto W1fail;

      case 108:
        // Blazing Door Raise (faster than TURBO!)
        if(EV_DoDoor( line, VD_blazeRaise, 4*VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 109:
        // Blazing Door Open (faster than TURBO!)
        if(EV_DoDoor( line, VD_blazeOpen, 4*VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 100:
        if( EN_heretic )
        {
          EV_DoDoor( line, VD_normalDoor, VDOORSPEED * 3);
        }
        else
        {
          // Build Stairs Turbo 16
          if(EV_BuildStairs( line, ST_turbo16) )  goto W1clear;
          goto W1fail;
        }
        break;

      case 110:
        // Blazing Door Close (faster than TURBO!)
        if(EV_DoDoor( line, VD_blazeClose, 4*VDOORSPEED) )  goto W1clear;
        goto W1fail;

      case 119:
        // Raise floor to nearest surr. floor
        if(EV_DoFloor( line, FT_raiseFloorToNearest) )  goto W1clear;
        goto W1fail;

      case 121:
        // Blazing PlatDownWaitUpStay
        if(EV_DoPlat( line, PLATT_blazeDWUS, 0) )  goto W1clear;
        goto W1fail;

      case 124:
        // Secret EXIT
        if( cv_allowexitlevel.EV )
            G_SecretExitLevel ();
        break;

      case 125:
        // TELEPORT MonsterONLY
        if (!thing->player)
        {
            if(EV_Teleport( line, side, thing ) )  goto W1clear;
            goto W1fail;
        }
        break;

      case 130:
        // Raise Floor Turbo
        if(EV_DoFloor( line, FT_raiseFloorTurbo) )  goto W1clear;
        goto W1fail;

      case 141:
        // Silent Ceiling Crush & Raise
        if(EV_DoCeiling( line, CT_silentCrushAndRaise) )  goto W1clear;
        goto W1fail;

      //SoM: FraggleScript
      case 273: //(1sided)
        if(side) break;

      case 272: //(2sided)
        T_RunScript(line->tag, thing);
        break;

      // once-only triggers
      case 275: //(1sided, W1)
        if(side) break;
        // Fall-through to RunScript (W1)

      case 274: //(2sided, W1)
        T_RunScript(line->tag, thing);
        goto W1clear;  // clear trigger


      // RETRIGGERS.  All from here till end.
      case 72:
        // Ceiling Crush
        EV_DoCeiling( line, CT_lowerAndCrush );
        break;

      case 73:
        // Ceiling Crush and Raise
        EV_DoCeiling( line, CT_crushAndRaise);
        break;

      case 74:
        // Ceiling Crush Stop
        EV_CeilingCrushStop(line);
        break;

      case 75:
        // Close Door
        EV_DoDoor( line, VD_doorclose, VDOORSPEED);
        break;

      case 76:
        // Close Door 30
        EV_DoDoor( line, VD_close30ThenOpen, VDOORSPEED);
        break;

      case 77:
        // Fast Ceiling Crush & Raise
        EV_DoCeiling( line, CT_fastCrushAndRaise);
        break;

      case 79:
        // Lights Very Dark
        EV_LightTurnOn(line,35);
        break;

      case 80:
        // Light Turn On - brightest near
        EV_LightTurnOn(line,0);
        break;

      case 81:
        // Light Turn On 255
        EV_LightTurnOn(line,255);
        break;

      case 82:
        // Lower Floor To Lowest
        EV_DoFloor( line, FT_lowerFloorToLowest );
        break;

      case 83:
        // Lower Floor
        EV_DoFloor( line, FT_lowerFloor);
        break;

      case 84:
        // LowerAndChange
        EV_DoFloor( line, FT_lowerAndChange);
        break;

      case 86:
        // Open Door
        EV_DoDoor( line, VD_dooropen, VDOORSPEED);
        break;

      case 87:
        // Perpetual Platform Raise
        EV_DoPlat( line, PLATT_perpetualRaise, 0);
        break;

      case 88:
        // PlatDownWaitUp
        EV_DoPlat( line, PLATT_downWaitUpStay, 0);
        break;

      case 89:
        // Platform Stop
        EV_StopPlat(line);
        break;

      case 90:
        // Raise Door
        EV_DoDoor( line, VD_normalDoor, VDOORSPEED);
        break;

      case 91:
        // Raise Floor
        EV_DoFloor( line, FT_raiseFloor);
        break;

      case 92:
        // Raise Floor 24
        EV_DoFloor( line, FT_raiseFloor24);
        break;

      case 93:
        // Raise Floor 24 And Change
        EV_DoFloor( line, FT_raiseFloor24AndChange);
        break;

      case 94:
        // Raise Floor Crush
        EV_DoFloor( line, FT_raiseFloorCrush);
        break;

      case 95:
        // Raise floor to nearest height
        // and change texture.
        EV_DoPlat( line, PLATT_raiseToNearestAndChange, 0);
        break;

      case 96:
        // Raise floor to shortest texture height
        // on either side of lines.
        EV_DoFloor( line, FT_raiseToTexture);
        break;

      case 97:
        // TELEPORT!
        EV_Teleport( line, side, thing );
        break;

      case 98:
        // Lower Floor (TURBO)
        EV_DoFloor( line, FT_turboLower);
        break;

      case 105:
        if( EN_heretic )
        {
            if( cv_allowexitlevel.EV )
            {
                G_SecretExitLevel ();
                goto W1clear;
            }
        }
        else
        {
            // Blazing Door Raise (faster than TURBO!)
            EV_DoDoor( line, VD_blazeRaise, 4*VDOORSPEED);
        }
        break;

      case 106:
        if( EN_heretic )
        {
            EV_BuildStairs (line, 16 * FRACUNIT);
            goto W1clear;
        }
        else
        {
            // Doom, Boom.
            // Blazing Door Open (faster than TURBO!)
            EV_DoDoor( line, VD_blazeOpen, 4*VDOORSPEED);
        }
        break;

      case 107:
        if( EN_doom_etc ) // used for a switch !
        {
            // Doom, Boom.
            // Blazing Door Close (faster than TURBO!)
            EV_DoDoor( line, VD_blazeClose, 4*VDOORSPEED);
        }
        break;

      case 120:
        // Blazing PlatDownWaitUpStay.
        EV_DoPlat( line, PLATT_blazeDWUS, 0);
        break;

      case 126:
        // TELEPORT MonsterONLY.
        if (!thing->player)
            EV_Teleport( line, side, thing );
        break;

      case 128:
        // Raise To Nearest Floor
        EV_DoFloor( line, FT_raiseFloorToNearest);
        break;

      case 129:
        // Raise Floor Turbo
        EV_DoFloor( line, FT_raiseFloorTurbo);
        break;

      // SoM:3/4/2000: Extended Boom W* triggers.
      default:
        if(EN_boom)
        {
          // Boom special linedefs
          switch(line->special) {
            //SoM: 3/4/2000:Boom Walk once triggers.
            //SoM: 3/4/2000:Yes this is "copied" code! I just cleaned it up. Did you think I was going to retype all this?!
            case 142:
              // Raise Floor 512
              if (EV_DoFloor( line, FT_raiseFloor512))  goto W1clear;
              break;
  
            case 143:
              // Raise Floor 24 and change
              if (EV_DoPlat( line, PLATT_raiseAndChange, 24))  goto W1clear;
              break;

            case 144:
              // Raise Floor 32 and change
              if (EV_DoPlat( line, PLATT_raiseAndChange, 32))  goto W1clear;
              break;

            case 145:
              // Lower Ceiling to Floor
              if (EV_DoCeiling( line, CT_lowerToFloor ))  goto W1clear;
              break;

            case 146:
              // Lower Pillar, Raise Donut
              if (EV_DoDonut(line))  goto W1clear;
              break;

            case 199:
              // Lower ceiling to lowest surrounding ceiling
              if (EV_DoCeiling( line, CT_lowerToLowest))  goto W1clear;
              break;

            case 200:
              // Lower ceiling to highest surrounding floor
              if (EV_DoCeiling( line, CT_lowerToMaxFloor))  goto W1clear;
              break;

            case 207:
              // W1 silent teleporter (normal kind)
              if (EV_SilentTeleport(line, side, thing))  goto W1clear;
              break;

            case 153: 
              // Texture/Type Change Only (Trig)
              if (EV_DoChange( line, CH_MODEL_trig_only))  goto W1clear;
              break;
  
            case 239: 
              // Texture/Type Change Only (Numeric)
              if (EV_DoChange( line, CH_MODEL_num_only))  goto W1clear;
              break;
 
            case 219:
              // Lower floor to next lower neighbor
              if (EV_DoFloor( line, FT_lowerFloorToNearest))  goto W1clear;
              break;

            case 227:
              // Raise elevator next floor
              if (EV_DoElevator( line, ET_elevateUp))  goto W1clear;
              break;

            case 231:
              // Lower elevator next floor
              if (EV_DoElevator( line, ET_elevateDown))  goto W1clear;
              break;

            case 235:
              // Elevator to current floor
              if (EV_DoElevator( line, ET_elevateCurrent))  goto W1clear;
              break;

            case 243: 
              // W1 silent teleporter (linedef-linedef kind)
              if (EV_SilentLineTeleport(line, side, thing, false))  goto W1clear;
              break;

            case 262: 
              if (EV_SilentLineTeleport(line, side, thing, true))  goto W1clear;
              break;
 
            case 264: 
              if (!thing->player &&
                  EV_SilentLineTeleport(line, side, thing, true))  goto W1clear;
                line->special = 0;
              break;

            case 266: 
              if (!thing->player &&
                  EV_SilentLineTeleport(line, side, thing, false))  goto W1clear;
              break;

            case 268: 
              if (!thing->player &&
                  EV_SilentTeleport(line, side, thing))  goto W1clear;
              break;

            // Extended walk many retriggerable
 
            //Boom added lots of linedefs to fill in the gaps in trigger types

            case 147:
              // Raise Floor 512
              EV_DoFloor( line, FT_raiseFloor512);
              break;

            case 148:
              // Raise Floor 24 and Change
              EV_DoPlat( line, PLATT_raiseAndChange, 24);
              break;

            case 149:
              // Raise Floor 32 and Change
              EV_DoPlat( line, PLATT_raiseAndChange, 32);
              break;

            case 150:
              // Start slow silent crusher
              EV_DoCeiling( line, CT_silentCrushAndRaise);
              break;

            case 151:
              // RaiseCeilingLowerFloor
              EV_DoCeiling( line, CT_raiseToHighest );
              EV_DoFloor( line, FT_lowerFloorToLowest );
              break;

            case 152:
              // Lower Ceiling to Floor
              EV_DoCeiling( line, CT_lowerToFloor );
              break;

            case 256:
              // Build stairs, step 8
              EV_BuildStairs( line, ST_build8);
              break;

            case 257:
              // Build stairs, step 16
              EV_BuildStairs( line, ST_turbo16);
              break;

            case 155:
              // Lower Pillar, Raise Donut
              EV_DoDonut(line);
              break;

            case 156:
              // Start lights strobing
              EV_StartLightStrobing(line);
              break;

            case 157:
              // Lights to dimmest near
              EV_TurnTagLightsOff(line);
              break;

            case 201:
              // Lower ceiling to lowest surrounding ceiling
              EV_DoCeiling( line, CT_lowerToLowest);
              break;

            case 202:
              // Lower ceiling to highest surrounding floor
              EV_DoCeiling( line, CT_lowerToMaxFloor);
              break;

            case 208:
              // WR silent teleporter (normal kind)
              EV_SilentTeleport(line, side, thing);
              break;

            case 212:
              // Toggle floor between C and F instantly
              EV_DoPlat( line, PLATT_toggleUpDn, 0);
              break;

            case 154:
              // Texture/Type Change Only (Trigger)
              EV_DoChange( line, CH_MODEL_trig_only);
              break;

            case 240: 
              // Texture/Type Change Only (Numeric)
              EV_DoChange( line, CH_MODEL_num_only);
              break;

            case 220:
              // Lower floor to next lower neighbor
              EV_DoFloor( line, FT_lowerFloorToNearest);
              break;

            case 228:
              // Raise elevator next floor
              EV_DoElevator( line, ET_elevateUp);
              break;

            case 232:
              // Lower elevator next floor
              EV_DoElevator( line, ET_elevateDown);
              break;

            case 236:
              // Elevator to current floor
              EV_DoElevator( line, ET_elevateCurrent);
              break;

            case 244: 
              // WR silent teleporter (linedef-linedef kind)
              EV_SilentLineTeleport(line, side, thing, false);
              break;

            case 263: 
              //Silent line-line reversed
              EV_SilentLineTeleport(line, side, thing, true);
              break;

            case 265: 
              //Monster-only silent line-line reversed
              if (!thing->player)
                EV_SilentLineTeleport(line, side, thing, true);
              break;

            case 267: 
              //Monster-only silent line-line
              if (!thing->player)
                EV_SilentLineTeleport(line, side, thing, false);
              break;

            case 269: 
              //Monster-only silent
              if (!thing->player)
                EV_SilentTeleport(line, side, thing);
              break;
          }
        }
    }
    return;

  // [WDJ] Easier to manage W1 in one place.
  // Doom, unconditional clear.
  // Heretic, unconditional clear.
  // Boom, conditional on rtn, or demo_compatibility.
W1fail:
    if( EN_heretic )  goto W1clear;  // heretic is unconditional
    if( !EN_boom)  goto W1clear;
    // Boom conditional, do not clear until it works.
    return;

W1clear:
    // (W1) clear line special 
    line->special = 0;
    return;
}



//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void P_ShootSpecialLine ( mobj_t* thing, line_t* line )
{
    int  ok;


    //SoM: 3/7/2000: Another General type check
    if (EN_boom)
    {
      // pointer to line function is NULL by default, set non-null if
      // line special is gun triggered generalized linedef type
      int (*linefunc)(line_t *line)=NULL;

      // check each range of generalized linedefs
      if ((unsigned)line->special >= (GenFloorBase+0x2000))
      {}  // not Boom generalized
      else if ((unsigned)line->special >= GenFloorBase)
      {
        if (!thing->player)
        {
          if ((line->special & FloorChange) || !(line->special & FloorModel))
            return;   // FloorModel is "Allow Monsters" if FloorChange is 0
        }
        if (!line->tag) //jff 2/27/98 all gun generalized types require tag
          return;

        linefunc = EV_DoGenFloor;
      }
      else if ((unsigned)line->special >= GenCeilingBase)
      {
        if (!thing->player)
        {
          if ((line->special & CeilingChange) || !(line->special & CeilingModel))
            return;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
        }
        if (!line->tag) //jff 2/27/98 all gun generalized types require tag
          return;
        linefunc = EV_DoGenCeiling;
      }
      else if ((unsigned)line->special >= GenDoorBase)
      {
        if (!thing->player)
        {
          if (!(line->special & DoorMonster))
            return;   // monsters disallowed from this door
          if (line->flags & ML_SECRET) // they can't open secret doors either
            return;
        }
        if (!line->tag) //jff 3/2/98 all gun generalized types require tag
          return;
        linefunc = EV_DoGenDoor;
      }
      else if ((unsigned)line->special >= GenLockedBase)
      {
        if (!thing->player)
          return;   // monsters disallowed from unlocking doors
        if (((line->special&TriggerType)==TRIG_GunOnce)
            || ((line->special&TriggerType)==TRIG_GunMany))
        { //jff 4/1/98 check for being a gun type before reporting door type
          if (!P_CanUnlockGenDoor(line,thing->player))
            return;
        }
        else
          return;
        if (!line->tag) //jff 2/27/98 all gun generalized types require tag
          return;

        linefunc = EV_DoGenLockedDoor;
      }
      else if ((unsigned)line->special >= GenLiftBase)
      {
        if (!thing->player)
          if (!(line->special & LiftMonster))
            return; // monsters disallowed
        linefunc = EV_DoGenLift;
      }
      else if ((unsigned)line->special >= GenStairsBase)
      {
        if (!thing->player)
          if (!(line->special & StairMonster))
            return; // monsters disallowed
        if (!line->tag) //jff 2/27/98 all gun generalized types require tag
          return;
        linefunc = EV_DoGenStairs;
      }
      else if ((unsigned)line->special >= GenCrusherBase)
      {
        if (!thing->player)
          if (!(line->special & StairMonster))
            return; // monsters disallowed
        if (!line->tag) //jff 2/27/98 all gun generalized types require tag
          return;
        linefunc = EV_DoGenCrusher;
      }

      if (linefunc)
        switch((line->special & TriggerType) >> TriggerTypeShift)
        {
          case TRIG_GunOnce:
            if (linefunc(line))
              P_ChangeSwitchTexture(line,0);
            return;
          case TRIG_GunMany:
            if (linefunc(line))
              P_ChangeSwitchTexture(line,1);
            return;
          default:  // if not a gun type, do nothing here
            return;
        }
    }


    //  Impacts that other things can activate.
    if (!thing->player)
    {
        // Check that monster can shoot line.
        // Only does check, continue on to implement action.
        ok = 0;
        switch(line->special)
        {
          // Doom, Heretic: 46
          case 46:
            // OPEN DOOR IMPACT
            ok = 1;
            break;
        }
        if (!ok)
            return;  // monster not allowed
    }

    if(EN_boom && !P_CheckTag(line))
      return;

    switch(line->special)
    {
      // Doom, Heretic: 24, 46, 47.
      case 24:
        // RAISE FLOOR
        if(EV_DoFloor( line, FT_raiseFloor) || !EN_boom)
          P_ChangeSwitchTexture(line,0);
        break;

      case 46:
        // OPEN DOOR
        if(EV_DoDoor( line, VD_dooropen, VDOORSPEED) || !EN_boom)
          P_ChangeSwitchTexture(line,1);
        break;

      case 47:
        // RAISE FLOOR NEAR AND CHANGE
        if(EV_DoPlat( line, PLATT_raiseToNearestAndChange, 0) || !EN_boom)
          P_ChangeSwitchTexture(line,0);
        break;

      //SoM: FraggleScript
      case 278:  // (GR)
      case 279:  // (G1)
        T_RunScript(line->tag, thing);
        if(line->special == 279) line->special = 0;       // clear if G1
        break;

      default:
        if (EN_boom)
        {
          switch (line->special)
          {
            case 197:
              // Exit to next level
              if( cv_allowexitlevel.EV )
              {
                  P_ChangeSwitchTexture(line,0);
                  G_ExitLevel();
              }
              break;

            case 198:
              // Exit to secret level
              if( cv_allowexitlevel.EV )
              {
                  P_ChangeSwitchTexture(line,0);
                  G_SecretExitLevel();
              }
              break;
              //jff end addition of new gun linedefs
          }
        }
        break;
    }
}


#ifdef MBF21
// Call only for MBF21
// All sector effects are instant kill, cannot be moderated with instantdamage.
static
byte  P_MBF21_PlayerSpecialSector( player_t* player, sector_t* sector )
{
    if( sector->special & MBF21_PLAYER_DEATH_SECTOR )  // Detect Alternate Damage Mode
    {
        // All are MBF21 death sectors.
        unsigned int mbf21_death_code = (sector->special & DAMAGE_MASK) >> DAMAGE_SHIFT;
        switch( mbf21_death_code )
        {
         case 0: // Kills the player, unless they have rad suit, or invulnerability.
            if( ! player->powers[pw_invulnerability]
                && ! player->powers[pw_ironfeet] )
                goto do_damage;
            break;
         case 1: // Kills the player
                goto do_damage;
         case 2: // Kills all players, then exits the map.
         case 3: // Kills all players, then exits the map via secret exit.
            {
                int j;
                for (j = 0; j < MAXPLAYERS; j++)
                {
                    if( playeringame[j] )
                      P_DamageMobj( players[j].mo, NULL, NULL, 10000 );
                }
            }
            if( mbf21_death_code == 2 )
              G_ExitLevel();
            else
              G_SecretExitLevel();
            break;
        }
        goto mbf21_intercept;
    }
    return 0;  // not mbf21

do_damage:
    // damage is always 10000, instant kill
    P_DamageMobj( player->mo, NULL, NULL, 10000 );

mbf21_intercept:
    return 1;  // was MBF21 encoding
}

// Monster and MBF21
byte  P_MBF21_MonsterSpecialSector( mobj_t* mon, sector_t* sector )
{
    if( sector->special & MBF21_MONSTER_DEATH_SECTOR )  // Detect MBF kill grounded monsters
    {
        if( (mon->z == mon->floorz)
            && ((mon->flags & (MF_SHOOTABLE | MF_FLOAT)) == MF_SHOOTABLE)) // shootable, not float
        {
            P_DamageMobj( mon, NULL, NULL, 10000 );
#if 0
            // [WDJ] This test is done in Thinker, the only caller.
            if( mon->thinker.function != TMI_MobjThinker )
              return 2;  // MBF21 killed monster
#endif
        }
        return 1; // MBF21 intercept
    }
    return 0;
}
#endif


//
// P_ProcessSpecialSector
// Function that actually applies the sector special to the player.
void P_PlayerSpecialSector_damage(player_t* player, sector_t* sector, boolean instantdamage)
{
    if(sector->special < 32)
    {
      // Heretic specials are in P_Heretic_PlayerInSpecialSector
      // Has hitten ground.
      switch (sector->special)
      {
        case 5:
          // HELLSLIME DAMAGE
          if (!player->powers[pw_ironfeet])  // rad suit
          {
              if (instantdamage)
              {
                  P_DamageMobj (player->mo, NULL, NULL, 10);

                  // spawn a puff of smoke
                  //debug_Printf ("damage!\n"); //debug
                  if( EV_legacy >= 125 )
                      P_SpawnSmoke (player->mo->x, player->mo->y, player->mo->z);
              }
          }
          break;

        case 7:
          // NUKAGE DAMAGE
          if (!player->powers[pw_ironfeet])  // rad suit
          {
              if (instantdamage)
                  P_DamageMobj (player->mo, NULL, NULL, 5);
          }
          break;

        case 16:
          // SUPER HELLSLIME DAMAGE
          // 10/20 unit damage per 31 ticks.
        case 4:
          // STROBE HURT
          // 10/20 unit damage, with blinking warning light.
          if (!player->powers[pw_ironfeet]  // rad suit is intermittant protection
              || (PP_Random(pr_slimehurt)<5) )  // even with suit, take damage
          {
              if (instantdamage)
                  P_DamageMobj (player->mo, NULL, NULL, 20);
          }
          break;

        case 9:
          // SECRET SECTOR
          sector->special = 0;
          goto found_secret_area;

        case 11:
          // EXIT SUPER DAMAGE! (for E1M8 finale)
          player->cheats &= ~CF_GODMODE;

          if (instantdamage)
              P_DamageMobj (player->mo, NULL, NULL, 20);

          if( (player->health <= 10) && cv_allowexitlevel.EV )
              G_ExitLevel();
          break;

        default:
          //SoM: 3/8/2000: Just ignore.
          //debug_Printf("P_PlayerInSpecialSector: unknown special %i",
          //             sector->special);
          break;
      };
   }
   else //SoM: Extended sector types for secrets and damage
   {
#ifdef MBF21
     if( EN_mbf21 )
     {
         // [WDJ] All MBF21 sector effects are instant kill, cannot be moderated with instantdamage.
         if( P_MBF21_PlayerSpecialSector( player, sector ) )
         {
             goto check_secret_flag;
             // If was MBF21 special, then skip Boom generalized decoding.
         }
     }
#endif
     // Boom generalized sector
     switch ((sector->special&DAMAGE_MASK)>>DAMAGE_SHIFT)
     {
       case 0: // no damage
         break;
       case 1: // 2/5 damage per 31 ticks
         if (!player->powers[pw_ironfeet] && instantdamage)
             P_DamageMobj (player->mo, NULL, NULL, 5);
         break;
       case 2: // 5/10 damage per 31 ticks
         if (!player->powers[pw_ironfeet] && instantdamage)
             P_DamageMobj (player->mo, NULL, NULL, 10);
         break;
       case 3: // 10/20 damage per 31 ticks
         if ((!player->powers[pw_ironfeet] || PP_Random(pr_slimehurt)<5)
             && instantdamage)  // take damage even with suit
         {
             P_DamageMobj (player->mo, NULL, NULL, 20);
         }
         break;
     }

#ifdef MBF21
check_secret_flag:
#endif
     // Always check, not subject to instantdamage.
     if (sector->special&SECRET_MASK)
     {
       sector->special &= ~SECRET_MASK;
       if (sector->special<32)
         sector->special=0;
       goto found_secret_area;
     }
   }
   return;

found_secret_area:
   player->secretcount++;
   //faB: useful only in single & coop.
   if( ! deathmatch )
   {
       if( player == displayplayer_ptr )
           GenPrintf(EMSG_playmsg, "\2You found a secret area!\n");
       else if( player == displayplayer2_ptr )
           GenPrintf(EMSG_playmsg2, "\2You found a secret area!\n");
   }
   return;
}



//
// P_PlayerOnSpecial3DFloor
// Checks to see if a player is standing on or is inside a 3D floor (water)
// and applies any specials..
void P_PlayerOnSpecial3DFloor(player_t* player)
{
  sector_t* sector;
  boolean   instantdamage = false;
  ffloor_t* rover;

  sector = player->mo->subsector->sector;
  if(!sector->ffloors)
    return;

  for(rover = sector->ffloors; rover; rover = rover->next)
  {
    if(!rover->master->frontsector->special)
      continue;

    // Check the 3D floor's type...
    if(rover->flags & FF_SOLID)
    {
      // Player must be on top of the floor to be affected...
      if(player->mo->z != *rover->topheight)
        continue;

      if( (EV_legacy >= 125)
        && (player->mo->eflags & MF_JUSTHITFLOOR)
        && (sector->model < SM_fluid) // not water
        && (leveltime % (2*NEWTICRATERATIO))) //SoM: penalize jumping less.
      {
        // hit solid ground
        instantdamage = true;
      }
      else
        instantdamage = !(leveltime % (32*NEWTICRATERATIO));
    }
    else
    {
      //Water and DEATH FOG!!! heh
      if((player->mo->z > *rover->topheight)
         || ((player->mo->z + player->mo->height) < *rover->bottomheight) )
        continue;
      instantdamage = !(leveltime % (32*NEWTICRATERATIO));
    }

    P_PlayerSpecialSector_damage(player, rover->master->frontsector, instantdamage);
  }
}


//
// P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void P_PlayerInSpecialSector (player_t* player)
{
    sector_t*   sector;
    boolean     instantdamage=false;

    // SoM: Check 3D floors...
    P_PlayerOnSpecial3DFloor(player);

    sector = player->mo->subsector->sector;

    //Fab: keep track of what sector type the player's currently in
    player->specialsector = sector->special;


    if (!player->specialsector)     // nothing special, exit
        return;

    if( EN_heretic )
    {
        P_Heretic_PlayerInSpecialSector(player, sector);
        return;
    }

    // Falling, not all the way down yet?
    //SoM: 3/17/2000: Damage if in slimey water!
    if (sector->model > SM_fluid)  // check for water
    {
        // check if in water
        if(player->mo->z > sectors[sector->modelsec].floorheight)
            return;  // not in the water yet
    }
    else if (player->mo->z != sector->floorheight)
            return;

    //Fab: jumping in lava/slime does instant damage (no jump cheat)
    if( (EV_legacy >= 125)
        && (player->mo->eflags & MF_JUSTHITFLOOR)
        && (sector->model < SM_fluid)  // not in water
        && (leveltime % (2*NEWTICRATERATIO))) //SoM: penalize jumping less.
    {
        // hit solid ground
        instantdamage = true;
    }
    else
        instantdamage = !(leveltime % (32*NEWTICRATERATIO));

    P_PlayerSpecialSector_damage(player, sector, instantdamage);
}




//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//

void P_UpdateSpecials (void)
{
    animtex_t*  anim;
    unsigned int  i;

    //  LEVEL TIMER
    if (timelimit_tics && (timelimit_tics < leveltime))
        G_ExitLevel();

    //  ANIMATE TEXTURES
    for (anim = anims ; anim < lastanim ; anim++)
    {
        unsigned int anim_offset = leveltime / anim->speed;
        if( anim->istexture )
        {
            // Texture animation.
            // Update animation indirection for all the textures in the animation sequence.
            // They can be used in large synchronized animations.
            for (i=anim->at.base_texnum ; i<anim->at.base_texnum+anim->numpics ; i++)
            {
                texturetranslation[i] = anim->at.base_texnum + ( (anim_offset + i)%anim->numpics );
            }
        }
        else
        {
            // Flat animation.
            // The flat_list is kept in animation order, so do not need animseq.	    
            anim_flatlist_t * fl = anim->af.flat_list;  // flat animation list
            if( ! fl )  continue;
            for(i=0; i<anim->numpics; i++, fl++)  // step through the flat_list
            {
                if( fl->flat_ref == NULL )  continue;  // should never happen
                // update the levelflat lump number in levelflats
                fl->flat_ref->lumpnum = ADD_TO_LUMPNUM( anim->af.base_flat_lumpnum,
                    (anim_offset + i) % anim->numpics );
            }
        }
    }

    //  DO BUTTONS
    for (i = 0; i < MAXBUTTONS; i++)
    {
        if (buttonlist[i].btimer)
        {
            buttonlist[i].btimer--;
            if (!buttonlist[i].btimer)
            {
                switch(buttonlist[i].where)
                {
                  case B_top_texture:
                    sides[buttonlist[i].line->sidenum[0]].toptexture =
                        buttonlist[i].btexture;
                    break;

                  case B_middle_texture:
                    sides[buttonlist[i].line->sidenum[0]].midtexture =
                        buttonlist[i].btexture;
                    break;

                  case B_bottom_texture:
                    sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                        buttonlist[i].btexture;
                    break;
                }
                S_StartXYZSound(buttonlist[i].soundorg, sfx_swtchn);
                memset(&buttonlist[i],0,sizeof(button_t));
            }
        }
    }  // for
}


//SoM: 3/8/2000: EV_DoDonut moved to p_floor.c

//SoM: 3/23/2000: Adds a sectors floor and ceiling to a sector's ffloor list


// Link ffloor into sector list of ffloor
static
void P_LinkFFloor(sector_t* sec, ffloor_t* ffloor)
{
  ffloor_t* rover;

  if(!sec->ffloors)
  {
    // head of list
    sec->ffloors = ffloor;
    ffloor->next = NULL;
    ffloor->prev = NULL;
    return;
  }

  // find end of list
  for(rover = sec->ffloors; rover->next; rover = rover->next)
     ;

  // append to end of list
  rover->next = ffloor;
  ffloor->prev = rover;
  ffloor->next = NULL;
}


// Implement Legacy 3D floor
// taggedsec is the affected sector, found by tag
// modsec is the model sector
static
void P_AddFakeFloor(sector_t* taggedsec, sector_t* modsec, line_t* master, uint32_t flags)
{
  ffloor_t*      ffloor;
  int            taggedindex = taggedsec - sectors; // tagged sector index

  // modsec is control sector, model sector

  // Make list of control sectors that affect this sector, and grow it
  {
    int  i;

    // Initial condition numattached==0, is also handled by this code
    // if already attached, then do not need to process again
    for(i = 0; i < modsec->numattached; i++)
    {
      if(modsec->attached[i] == taggedindex)
        return;
    }
    // Init to NULL by P_LoadSectors, realloc will make initial allocation
    // or grow the list
    int * new_attached = realloc(modsec->attached, sizeof(int) * (modsec->numattached + 1));
    // non-fatal handling, just ignore the new floor
    if( new_attached == NULL )  return;
    modsec->attached = new_attached;
    modsec->attached[modsec->numattached] = taggedindex;	// sector index
    modsec->numattached ++;
  }

  //Add the floor
  ffloor = Z_Malloc(sizeof(ffloor_t), PU_LEVEL, NULL);
  ffloor->model_secnum = modsec - sectors; // model sector from linedef
  ffloor->taggedtarget = taggedsec;
  // model sector floor -> ffloor bottom
  ffloor->bottomheight     = &modsec->floorheight;
  ffloor->bottompic        = &modsec->floorpic;
  //ffloor->bottomlightlevel = &modsec->lightlevel;
  ffloor->bottomxoffs      = &modsec->floor_xoffs;
  ffloor->bottomyoffs      = &modsec->floor_yoffs;

  // model sector ceiling -> ffloor top
  ffloor->topheight     = &modsec->ceilingheight;
  ffloor->toppic        = &modsec->ceilingpic;
  ffloor->toplightlevel = &modsec->lightlevel;
  ffloor->topxoffs      = &modsec->ceiling_xoffs;
  ffloor->topyoffs      = &modsec->ceiling_yoffs;

  ffloor->flags = flags;
  ffloor->master = master;
  // [WDJ] do not leave uninit
  ffloor->lastlight = 0;  // caused segfault
  ffloor->fw_effect = 0;
  ffloor->alpha = 0;

  if(flags & (FF_TRANSLUCENT|FF_FOG))
  {
      // get fog and translucent alpha
      ffloor->fw_effect = sides[master->sidenum[0]].toptexture;
      ffloor->alpha = fweff[ffloor->fw_effect].alpha;
  }
  P_LinkFFloor(taggedsec, ffloor);	// append to sector ffloor list
}



//
// SPECIAL SPAWNING
//

// Special linedefs flags used by SpawnSpecials and Config_FW_Specials
// index by (fogwater_flags_e & FWF_index) as in fweff[].flags
static const uint32_t   fw_linedef_flags_table[4] =
{
   0,  // unused
//[1] 301: Legacy translucent 3D water in tagged
  FF_EXISTS|FF_TRANSLUCENT|FF_SWIMMABLE
     |FF_SLAB_SHADOW|FF_EXTRA|FF_CUTEXTRA|FF_CUTSPRITES,
//[2] 302: Legacy 3D fog in tagged
  FF_EXISTS|FF_FOG
     |FF_SLAB_SHADOW|FF_EXTRA|FF_CUTEXTRA|FF_CUTSPRITES,
//[3] 304: Legacy opaque fluid
  FF_EXISTS|FF_SWIMMABLE
     |FF_OUTER_SIDES|FF_OUTER_PLANES|FF_INNER_SIDES|FF_INNER_PLANES
     |FF_SLAB_SHADOW|FF_EXTRA|FF_CUTEXTRA|FF_CUTSPRITES,
//[4] 300: Legacy translucent floor, does not allow set default effect
};

// index by fogwater_effect_e;
static const uint32_t  fogwater_flags_table[FW_num] = {
// FW_colormap: // use colormap fog (which only colors all sectors)
  0,
// FW_clear:    // no fog  (WATER default)
  FF_OUTER_SIDES|FF_OUTER_PLANES|FF_INNER_SIDES|FF_INNER_PLANES,
// FW_cast:     // paint all surfaces with textures
  FF_FLUID
     |FF_OUTER_SIDES|FF_OUTER_PLANES|FF_INNER_SIDES|FF_INNER_PLANES,
// FW_fogfluid: // outside, inside fluid, fogsheet
  FF_FLUID|FF_FOG|FF_FOGFACE
     |FF_OUTER_SIDES|FF_OUTER_PLANES|FF_INNER_SIDES|FF_INNER_PLANES,
// FW_inside: render inside side, plane views (old FOG)
  FF_INNER_SIDES|FF_INNER_PLANES,
// FW_foglite:  // outside side, plane views, low alpha overall fog sheet
  FF_FOG|FF_FOGFACE
     |FF_OUTER_SIDES|FF_OUTER_PLANES,
   //|FF_INNER_PLANES,
// FW_fogdust:  // outside, when in fog apply overall fog sheet (FOG default)
  FF_FOG|FF_FOGFACE
     |FF_OUTER_SIDES|FF_OUTER_PLANES,
   //|FF_INNER_PLANES,
// FW_fogsheet: // outside, overall fog sheet, sector join fog sheet
  FF_FOG|FF_FOGFACE
     |FF_OUTER_SIDES|FF_OUTER_PLANES|FF_INNER_PLANES|FF_JOIN_SIDES,
   //|FF_INNER_SIDES,
};

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//

// Parses command line parameters.

// Called by P_SetupLevel after all Load of sectors, linedef, and sidedef.
// P_LoadSideDefs2 has already interpreted colormap and texture
// for linedef specials.
// Called before playing game or playing demo
void P_SpawnSpecials (void)
{
    sector_t*   sector;
    uint32_t    flags;
    BDTC_game_detection_e  detect = 0;
    // update all special sectors
    fixed_t  water_friction = FRICTION_NORM;
    int  water_movefactor = ORIG_FRICTION_FACTOR;
   
    // [WDJ] 3/2011 Legacy shallow water friction
    if( friction_model == FR_legacy )
    {
        water_friction = FRICTION_NORM * 97/100;  // slog
        water_movefactor = 8*((FRICTION_NORM-48 - 0xDB34)*(0xA))/0x80; // non stick
    }

#if 0
    // [WDJ] At one time this may have been important, but is not used now.
    // Appears in prboom.
    int episode = 1;
    if( VALID_LUMP( W_CheckNumForName("texture2") ) )
        episode = 2;
#endif

    //  Init special SECTORs.
    sector = sectors;
    uint32_t si;
    for (si=0 ; si<numsectors ; si++, sector++)
    {
        sector->friction = FRICTION_NORM;  // defaults
        sector->movefactor = ORIG_FRICTION_FACTOR;
       
        if (!sector->special)
            continue;

        if (sector->special&SECRET_MASK) //SoM: 3/8/2000: count secret flags
          totalsecret++;

        switch (EN_heretic_hexen ? sector->special : sector->special&0x1F)
        {
          case 1:
            // FLICKERING LIGHTS
            P_SpawnLightFlash (sector);
            break;

          case 2:
            // STROBE FAST
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            break;

          case 3:
            // STROBE SLOW
            P_SpawnStrobeFlash(sector,SLOWDARK,0);
            break;

          case 4:
            if( EN_heretic_hexen )
                break; // see P_Heretic_PlayerInSpecialSector, Scroll_EastLavaDamage
            // STROBE FAST/DEATH SLIME
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            sector->special |= 3<<DAMAGE_SHIFT; //SoM: 3/8/2000: put damage bits in
            break;

//	  case 5: // see P_Heretic_PlayerInSpecialSector, Damage_LavaWimpy
// 	  case 7: // see P_Heretic_PlayerInSpecialSector, Damage_Sludge
          case 8:
            // GLOWING LIGHT
            P_SpawnGlowingLight(sector);
            break;

// 	  case 9: // see P_Heretic_PlayerInSpecialSector, Secret_Area
          case 9:
            // SECRET SECTOR
            if(sector->special<32)
              totalsecret++;
            break;

          case 10:
            // DOOR CLOSE IN 30 SECONDS
            P_SpawnDoorCloseIn30 (sector);
            break;

// 	  case 11: // see P_Heretic_PlayerInSpecialSector, Exit_SuperDamage
          case 12:
            // SYNC STROBE SLOW
            P_SpawnStrobeFlash (sector, SLOWDARK, 1);
            break;

          case 13:
            // SYNC STROBE FAST
            P_SpawnStrobeFlash (sector, FASTDARK, 1);
            break;

          case 14:
            // DOOR RAISE IN 5 MINUTES
            P_SpawnDoorRaiseIn5Mins (sector, si);
            break;

          case 15:
            if( EN_heretic )
            {
                sector->friction = FRICTION_LOW;  // ice sector
                sector->movefactor = (ORIG_FRICTION_FACTOR/4);
            }
            break;

//	  case 16: // see P_Heretic_PlayerInSpecialSector, Damage_LavaHefty
          case 17:
            P_SpawnFireFlicker(sector);
            break;

//	  case 25..39: // see P_Heretic_PlayerInSpecialSector, Scroll
//	  case 40..51: // see P_Heretic_PlayerInSpecialSector, Wind
        }
    }

    //SoM: 3/8/2000: Boom level init functions
    P_Remove_AllActiveCeilings();
    P_Remove_AllActivePlats();
    uint32_t bi;
    for (bi = 0; bi < MAXBUTTONS; bi++)
      memset(&buttonlist[bi], 0, sizeof(button_t));

    P_Init_TagLists();   //Create xref tables for tags
    P_SpawnScrollers(); //Add generalized scrollers
    P_SpawnPushers();   //New pusher model using linedefs

    //  Init line EFFECTs
    uint32_t ki;
    for (ki=0; ki < numlines; ki++)
    {
        line_t * effline = & lines[ki]; // effect line
        sector_t * model_secp = NULL; // model sector ptr (control sector)
        int model_secnum = -1; // model sector number for effline
       
        // Not all specials use nor require this, so no error messages
        if( effline->sidenum[0] != NULL_INDEX )
        {
            // get model sector and sector number from side 0
            model_secp = sides[ effline->sidenum[0] ].sector;  // frontsector
            model_secnum = model_secp - sectors; // sector index
        }
       
        int fsecn = -1;  // init search FindSector
        switch(effline->special)
        {
          // [WDJ] Protect here, if model is set then ensure modelsec >= 0
          // model sector == control sector

          // support for drawn heights coming from different sector
          case 242:	// Boom deep water
            // Divides the tagged sectors using the model sector ceiling
            // and floor, heights and flats.
            // Renders within and beyond tagged sectors floor and ceiling,
            // which can make them invisible.
            // Uses top texture, middle texture, bottom texture as colormaps,
            // in the tagged sectors. Otherwise, they can be a normal texture of the sidedef.
            // Uses the model sector lightlevel underwater, and over ceiling.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              sectors[fsecn].modelsec = model_secnum;
              sectors[fsecn].model = SM_Boom_deep_water;
              sectors[fsecn].friction = water_friction;
              sectors[fsecn].movefactor = water_movefactor;
            }
            detect |= BDTC_boom;
            break;

          //SoM: 3/20/2000: support for drawn heights coming from different sector
          case 280:	// Legacy water
            // 3D water uses model sector ceiling and floor, heights and flats.
            // The model sector floor and ceiling are rendered as top of water,
            // and fake ceiling, only within limits of tagged sector.
            // Uses top texture, middle texture, bottom texture as colormaps,
            // in the tagged sectors. Otherwise, they can be a normal texture of the sidedef.
            // Uses the model sector lightlevel underwater, and over ceiling.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              sectors[fsecn].modelsec = model_secnum;
              sectors[fsecn].model = SM_Legacy_water;
              sectors[fsecn].friction = water_friction;
              sectors[fsecn].movefactor = water_movefactor;
            }
            detect |= BDTC_legacy;
            break;

          //SoM: 4/4/2000: HACK! Copy colormaps. Just plain colormaps.
          case 282:	// Legacy generate colormap, use in tagged
            // use the colormap in all tagged sectors
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              sectors[fsecn].midmap = effline->frontsector->midmap;
              sectors[fsecn].model = SM_colormap;
            }
            detect |= BDTC_legacy;
            break;

          case 281:	// Legacy solid 3D floor with shadow, in tagged
            // 3Dfloor slab uses model sector ceiling and floor, heights and flats.
            // The middle texture is the slab sides.
            // Under the 3Dfloor is the light and colormap of the model sector.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              P_AddFakeFloor(&sectors[fsecn], model_secp, effline,
                FF_EXISTS|FF_SOLID|FF_OUTER_SIDES|FF_OUTER_PLANES
                |FF_CUTSOLIDS|FF_CUTEXTRA );
            }
            detect |= BDTC_legacy;
            break;

          case 289:	// Legacy solid 3D floor without shadow, in tagged
            // 3Dfloor slab uses model sector ceiling and floor, heights and flats.
            // The middle texture is the slab sides.
            // Light below the slab is the same as above.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              P_AddFakeFloor(&sectors[fsecn], model_secp, effline,
                FF_EXISTS|FF_SOLID|FF_OUTER_SIDES|FF_OUTER_PLANES
                |FF_NOSHADE|FF_CUTSOLIDS|FF_CUTEXTRA );
            }
            detect |= BDTC_legacy;
            break;

          // TL block
          case 300:	// Legacy solid translucent 3D floor in tagged
            // 3Dfloor slab uses model sector ceiling and floor, heights and flats.
            // The middle texture is the slab sides, displayed translucent.
            // Light below the slab is the same as above.
            // Upper texture encodes the translucent alpha: #nnn  => 0..255
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              P_AddFakeFloor(&sectors[fsecn], model_secp, effline,
                FF_EXISTS|FF_SOLID|FF_TRANSLUCENT|FF_OUTER_SIDES|FF_OUTER_PLANES
                |FF_NOSHADE|FF_EXTRA|FF_CUTEXTRA );
            }
            detect |= BDTC_legacy;
            break;

          // TL water
          case 301:	// Legacy translucent 3D water in tagged
            // The middle texture is the water sides, displayed translucent.
            // Upper texture encodes the translucent alpha: #nnn  => 0..255
            flags = fw_linedef_flags_table[FWF_water];
            goto water_fakefloor;

          // Opaque water
          case 304:	// Legacy opaque fluid
            // The middle texture is the slab sides, forced opaque.
            // Upper texture encodes fog alpha: #nnn  => 0..255
            flags = fw_linedef_flags_table[FWF_opaque_water];

          water_fakefloor:
            // 3D water uses model sector ceiling and floor, heights and flats.
            // Within the water or fog is the lightlevel and colormap of the model sector.
            // Over and Under is the light and colormap of the sector.
            if ( model_secnum < 0 )  goto missing_model;
            model_secp->friction = water_friction;
            model_secp->movefactor = water_movefactor;
            {
                side_t * sd = & sides[ effline->sidenum[0] ];
                fogwater_t * fwp = & fweff[ sd->toptexture ];
                // fog and water flags vary
                flags = fogwater_flags_table[fwp->effect] | flags;
                while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
                {
                    P_AddFakeFloor(&sectors[fsecn], model_secp, effline, flags);
                }
                sd->toptexture = 1;
            }
            detect |= BDTC_legacy;
            break;

          // Fog
          case 302:	// Legacy 3D fog in tagged
            // Fog uses model sector ceiling and floor heights.
            // Within the fog is the lightlevel and colormap of the model sector.
            // Over and Under is the light and colormap of the sector.
            if ( model_secnum < 0 )  goto missing_model;
            {
                side_t * sd = & sides[ effline->sidenum[0] ];
                fogwater_t * fwp = & fweff[ sd->toptexture ];
                // all get FF_FOG, but FW_inside does not get FF_FOGFACE
                flags = fogwater_flags_table[fwp->effect] | fw_linedef_flags_table[FWF_fog];
                while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
                {
                    P_AddFakeFloor(&sectors[fsecn], model_secp, effline, flags);
                }
            }
            detect |= BDTC_legacy;
            break;

          // Light effect
          case 303:	// Legacy 3D ceiling light in tagged
            // Light uses model sector ceiling heights.
            // Below the ceiling is the lightlevel and colormap of the model sector.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              P_AddFakeFloor(&sectors[fsecn], model_secp, effline,
                FF_EXISTS|FF_CUTSPRITES );
            }
            detect |= BDTC_legacy;
            break;

          // Inner light effect
          case 305:	// Legacy inner light, within slab
            // Light uses model sector ceiling and floor heights.
            // Within the light bounds is the lightlevel and colormap of the model sector.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              P_AddFakeFloor(&sectors[fsecn], model_secp, effline,
                FF_EXISTS|FF_SLAB_SHADOW|FF_CUTSPRITES );
            }
            detect |= BDTC_legacy;
            break;

          // Invisible barrier
          case 306:	// Legacy invisible floor
            // 3Dfloor slab uses model sector ceiling and floor heights.
            // No light effects, it is invisible.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
            {
              P_AddFakeFloor(&sectors[fsecn], model_secp, effline,
                FF_EXISTS|FF_SOLID );
            }
            detect |= BDTC_legacy;
            break;

          // Boom independent floor lighting (e.g. lava)
          case 213:
            // Set floor light to light in control sector.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
              sectors[fsecn].floorlightsec = model_secnum;
            detect |= BDTC_boom;
            break;

          // Boom independent ceiling lighting
          case 261:
            // Set ceiling light to light in control sector.
            if ( model_secnum < 0 )  goto missing_model;
            while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
              sectors[fsecn].ceilinglightsec = model_secnum;
            detect |= BDTC_boom;
            break;

          // Instant lower for floor SSNTails 06-13-2002
          case 290:
            EV_DoFloor( effline, FT_instantLower);
            break;

          // Instant raise for ceilings SSNTails 06-13-2002
          case 291:
            EV_DoCeiling( effline, CT_instantRaise);
            break;

          default:
            if(effline->special>=1000 && effline->special<1032)
            {
                while ((fsecn = P_FindSectorFromLineTag(effline,fsecn)) >= 0)
                {
                  sectors[fsecn].teamstartsec = effline->special-999; // only 999 so we know when it is set (it's != 0)
                }
                break;
            }
        }
        continue;
       
    missing_model:
       I_SoftError( "Model sector missing: linedef %d\n", ki ); // line num ki
    } // for

    if( detect )
        report_detect( "SpawnSpecials", detect );
    all_detect |= detect;  // used by BoomColormap_detect

    // [WDJ] Last so Boom friction linedef can override other sector frictions.
    P_SpawnFriction(NULL);  //New friction model using linedefs
    BoomColormap_detect();
}



// Called to update fogwater special flags after changing config
void P_Config_FW_Specials (void)
{
    sector_t*   sector = sectors;
    ffloor_t*   ffloor;
    fogwater_t* fwp;

    // all fake floor in all sectors
    uint32_t i;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
        for(ffloor = sector->ffloors; ffloor; )
        {
            if( ffloor->fw_effect )  // that use fogwater effect
            {
                fwp = & fweff[ffloor->fw_effect];
                if((fwp->flags & FWF_default_effect) && (fwp->flags & FWF_index)) // set defaults
                {
                    // fog and water flags vary
                    ffloor->flags =
                     fw_linedef_flags_table[fwp->flags & FWF_index]
                     | fogwater_flags_table[fwp->effect];
                }

                // get fog and translucent alpha
                ffloor->alpha = fweff[ffloor->fw_effect].alpha;
            }
            ffloor = ffloor->next;
        }
    } // for sector
}




/*
  SoM: 3/8/2000: General scrolling functions.
  T_Scroll,
  Add_Scroller,
  Add_WallScroller,
  P_SpawnScrollers
*/
//
// This function, with the help of r_plane.c and r_bsp.c, supports generalized
// scrolling floors and walls, with optional mobj-carrying properties, e.g.
// conveyor belts, rivers, etc. A linedef with a special type affects all
// tagged sectors the same way, by creating scrolling and/or object-carrying
// properties. Multiple linedefs may be used on the same sector and are
// cumulative, although the special case of scrolling a floor and carrying
// things on it, requires only one linedef. The linedef's direction determines
// the scrolling direction, and the linedef's length determines the scrolling
// speed. This was designed so that an edge around the sector could be used to
// control the direction of the sector's scrolling, which is usually what is
// desired.
//
// Process the active scrollers.

void T_Scroll(scroll_t *s)
{
  fixed_t  dx = s->dx, dy = s->dy;
  int	   affectee = s->affectee;

  if (s->control != -1)
  {   // compute scroll amounts based on a sector's height changes
      fixed_t height = sectors[s->control].floorheight +
        sectors[s->control].ceilingheight;
      fixed_t delta = height - s->last_height;
      s->last_height = height;
      dx = FixedMul(dx, delta);
      dy = FixedMul(dy, delta);
  }

  if (s->accel)
  {
      s->vdx = dx += s->vdx;
      s->vdy = dy += s->vdy;
  }

  if (!(dx | dy))                   // no-op if both (x,y) offsets 0
    return;

  switch (s->type)
  {
      side_t *side;
      sector_t *sec;
      fixed_t height, waterheight;
      msecnode_t *node;
      mobj_t *thing;

    case SCROLL_side:                   //Scroll wall texture
        side = sides + affectee;
        side->textureoffset += dx;
        side->rowoffset += dy;
        break;

    case SCROLL_floor:                  //Scroll floor texture
        sec = sectors + affectee;
        sec->floor_xoffs += dx;
        sec->floor_yoffs += dy;
        break;

    case SCROLL_ceiling:               //Scroll ceiling texture
        sec = sectors + affectee;
        sec->ceiling_xoffs += dx;
        sec->ceiling_yoffs += dy;
        break;

    case SCROLL_carry:

      sec = sectors + affectee;
      height = sec->floorheight;
      // [WDJ] fix precedence  11/25/2009
      // FIXED_MIN unless has special sector, and sector floor height > height
      // Uses model and modelsec, instead of the PrBoom heightsec.
      waterheight = ((sec->model > SM_fluid)
        && (sectors[sec->modelsec].floorheight > height)) ?
            sectors[sec->modelsec].floorheight : FIXED_MIN ;

      for (node = sec->touching_thinglist; node; node = node->m_snext)
      {
        thing = node->m_thing;
        if( !(thing->flags & MF_NOCLIP)
            && (!(thing->flags & MF_NOGRAVITY || thing->z > height)
                || thing->z < waterheight) )
        {
            // Move objects only if on floor or underwater,
            // non-floating, and clipped.
            thing->momx += dx;
            thing->momy += dy;
        }
      }
      break;

    case SCROLL_carry_ceiling:       // to be added later
      break;
  } // switch
}

//
// Add_Scroller()
//
// Add a generalized scroller to the thinker list.
//
// type: the enumerated type of scrolling: floor, ceiling, floor carrier,
//   wall, floor carrier & scroller
//
// (dx,dy): the direction and speed of the scrolling or its acceleration
//
// control: the sector whose heights control this scroller's effect
//   remotely, or -1 if no control sector
//
// affectee: the index of the affected object (sector or sidedef)
//
// accel: non-zero if this is an accelerative effect
//

static void Add_Scroller(int type, fixed_t dx, fixed_t dy,
                         int control, int affectee, int accel)
{
  scroll_t *s = Z_Malloc(sizeof *s, PU_LEVSPEC, 0);
  s->thinker.function = TFI_Scroll;
  s->type = type;
  s->dx = dx;
  s->dy = dy;
  s->accel = accel;
  s->vdx = s->vdy = 0;
  if ((s->control = control) != -1)
  {
    s->last_height =
      sectors[control].floorheight + sectors[control].ceilingheight;
  }
  s->affectee = affectee;
  P_AddThinker(&s->thinker);
}

// Adds wall scroller. Scroll amount is rotated with respect to wall's
// linedef first, so that scrolling towards the wall in a perpendicular
// direction is translated into vertical motion, while scrolling along
// the wall in a parallel direction is translated into horizontal motion.

static void Add_WallScroller(fixed_t dx, fixed_t dy, const line_t *l,
                             int control, int accel)
{
  fixed_t x = abs(l->dx), y = abs(l->dy), d;
  if (y > x)
  {
    d = x, x = y, y = d;
  }
  d = FixedDiv(x, sine_ANG( tantoangle[FixedDiv(y,x) >> DBITS] + ANG90 ) );
  x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
  y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
  Add_Scroller(SCROLL_side, x, y, control, *l->sidenum, accel);
}

// Amount (dx,dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))

// Initialize the scrollers
static void P_SpawnScrollers(void)
{
  line_t * lnp = lines;  // line ptr

  uint32_t ki;
  for (ki=0; ki<numlines; ki++,lnp++)
  {
      // for all lines l
      fixed_t dx = lnp->dx >> SCROLL_SHIFT;  // direction and speed of scrolling
      fixed_t dy = lnp->dy >> SCROLL_SHIFT;
      int control = -1, accel = 0;         // no control sector or acceleration
      int special = lnp->special;

      // Types 245-249 are same as 250-254 except that the
      // first side's sector's heights cause scrolling when they change, and
      // this linedef controls the direction and speed of the scrolling. The
      // most complicated linedef since donuts, but powerful :)

      if (special >= 245 && special <= 249)         // displacement scrollers
      {
          special += 250-245;
          control = sides[*lnp->sidenum].sector - sectors;
      }
      else
      {
        if (special >= 214 && special <= 218)       // accelerative scrollers
        {
            accel = 1;
            special += 250-214;
            control = sides[*lnp->sidenum].sector - sectors;
        }
      }

      int fsecn = -1;  // init search FindSector
      switch (special)
      {
        case 250:   // scroll effect ceiling
          while ((fsecn = P_FindSectorFromLineTag(lnp,fsecn)) >= 0)
            Add_Scroller(SCROLL_ceiling, -dx, dy, control, fsecn, accel);
          break;

        case 251:   // scroll effect floor
        case 253:   // scroll and carry objects on floor
          while ((fsecn = P_FindSectorFromLineTag(lnp,fsecn)) >= 0)
            Add_Scroller(SCROLL_floor, -dx, dy, control, fsecn, accel);
          if (special != 253)
            break;

        case 252: // carry objects on floor
          dx = FixedMul(dx,CARRYFACTOR);
          dy = FixedMul(dy,CARRYFACTOR);
          while ((fsecn = P_FindSectorFromLineTag(lnp,fsecn)) >= 0)
            Add_Scroller(SCROLL_carry, dx, dy, control, fsecn, accel);
          break;

          // scroll wall according to linedef
          // (same direction and speed as scrolling floors)
        case 254:  // Boom scroll tagged wall, sync with floor/ceiling
         {
          int linenum = -1;
          while ((linenum = P_FindLineFromLineTag(lnp,linenum)) >= 0)
          {
            if ((uint32_t)linenum != ki)
              Add_WallScroller(dx, dy, &lines[linenum], control, accel);
          }
         }
          break;

        case 255:  // Boom scroll wall by sidedef offsets
         {
          int linenum = lines[ki].sidenum[0];
          Add_Scroller(SCROLL_side, -sides[linenum].textureoffset,
                       sides[linenum].rowoffset, -1, linenum, accel);
         }
          break;

        case 48:  // Doom  // scroll first side
          Add_Scroller(SCROLL_side,  FRACUNIT, 0, -1, lines[ki].sidenum[0], accel);
          break;

        case 99: // heretic right scrolling
          if(! EN_heretic)
              break; // doom use it as bluekeydoor
          // Heretic: fall through to implement scroller.
        case 85:  // boom  // jff 1/30/98 2-way scroll
          Add_Scroller(SCROLL_side, -FRACUNIT, 0, -1, lines[ki].sidenum[0], accel);
          break;

#ifdef MBF21
       // [WDJ] MBF21 scrollers, from DSDA-Doom.
       // Like special 255, but apply the scroll to other lines with the same tag
       case 1024: // no control sector, no accel
       case 1025: // control sector
       case 1026: // control sector, accel
          if( lnp->tag == 0 )  // tag == 0 is not allowed
          {
              I_SoftError("Scroller Line %u has tag = 0", ki);
              return;
          }

          if( special > 1024 )  // control sector
            control = sides[*lnp->sidenum].sector - sectors;

          if( special == 1026 )  // accel
            accel = 1;  // not boolean

          {
              byte side = lines[ki].sidenum[0];
              dx = -sides[side].textureoffset / 8;
              dy = sides[side].rowoffset / 8;

              int linenum = -1;
              while ((linenum = P_FindLineFromLineTag(lnp,linenum)) >= 0)
              {
                  if( (uint32_t)linenum != ki )
                    Add_WallScroller(dx, dy, &lines[linenum], control, accel);
              }
          }
          break;
#endif
      }
  }
}



/*
  SoM: 3/8/2000: Friction functions start.
  Add_Friction,
  T_Friction,
  P_SpawnFriction
*/

#ifdef FRICTIONTHINKER
byte  EN_boom_friction_thinker;

// Adds friction thinker.
// [WDJ] Obsolete. Only kept for Boom demo.
static void Add_Friction(int friction, int movefactor, int affectee)
{
    friction_t *f = Z_Malloc(sizeof *f, PU_LEVSPEC, 0);

    f->thinker.function = TFI_Friction;
    f->friction = friction;
    f->movefactor = movefactor;
    f->affectee = affectee;
    P_AddThinker(&f->thinker);
}



//Function to apply friction to all the things in a sector.
// [WDJ] Obsolete. Only kept for Boom demo.
void T_Friction(friction_t *f)
{
    sector_t *sec;
    mobj_t   *thing;
    msecnode_t* node;
    boolean foundfloor = false;

    // [WDJ] This is only enabled by EN_boom_friction_thinker, which requires
    // (friction_model == FR_boom) && EN_variable_friction.
    // But they could switch models during play ...?
#if 1
    if( !EN_boom_friction_thinker )
        return;
#else
    if (!EN_boom || !EN_variable_friction)
        return;
#endif

    sec = sectors + f->affectee;

    // Be sure the special sector type is still turned on. If so, proceed.
    // Else, bail out; the sector type has been changed on us.

    if (!(sec->special & FRICTION_MASK))
    {
        // sector does not have friction, check if any 3d floor has friction
        if(sec->ffloors)
        {
            ffloor_t*  rover;

            for(rover = sec->ffloors; rover; rover = rover->next)
            {
                // Do some small extra checks here to possibly save unneeded work.
                if(!(rover->master->frontsector->special & FRICTION_MASK))
                   continue;
                foundfloor = true;
            }
        }

        if( ! foundfloor) // Not even a 3d floor has the FRICTION_MASK.
           return;
    }

    // Assign the friction value to players on the floor, non-floating,
    // and clipped. Normally the object's friction value is kept at
    // ORIG_FRICTION and this thinker changes it for icy or muddy floors.

    // In Phase II, you can apply friction to Things other than players.

    // When the object is straddling sectors with the same
    // floorheight that have different frictions, use the lowest
    // friction value (muddy has precedence over icy).

    node = sec->touching_thinglist; // things touching this sector
    while (node)
    {
        thing = node->m_thing;
        if (thing->player &&
            !(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) &&
            thing->z == thing->floorz)
        {
            if(foundfloor && thing->z == sec->floorheight); // Skip

            else if ((thing->friction == ORIG_FRICTION) ||     // normal friction?
                (f->friction < thing->friction))
            {
                thing->friction   = f->friction;
                thing->movefactor = f->movefactor;
            }
        }
        node = node->m_snext;
    }
}
#endif

// [WDJ] 3/2011 Adopting the section friction code of killough,
// such as zdoom and prboom use.
// 1. Friction thinker only considered sectors with same tag as friction linedef.
// Section friction code would work with changing tag too.
// 2. Unlike the original concept, this implementation uses the sector friction
// fields in all sectors, making for faster friction testing.  Sector friction
// must be updated anywhere FRICTION_MASK could be changed.
// 3. Implements the edge effect for ice, fully as described, where other
// implementations should have problems due to not considering NORMAL sectors.

// I feel obligated to include the original comment that explains it
// (from prboom).
// 
// killough 8/28/98:
//
// Completely redid code, which did not need thinkers, and which put a heavy
// drag on CPU. Friction is now a property of sectors, NOT objects inside
// them. All objects, not just players, are affected by it, if they touch
// the sector's floor. Code simpler and faster, only calling on friction
// calculations when an object needs friction considered, instead of doing
// friction calculations on every sector during every tic.
//
// Although this -might- ruin Boom demo sync involving friction, it's the only
// way, short of code explosion, to fix the original design bug. Fixing the
// design bug in Boom's original friction code, while maintaining demo sync
// under every conceivable circumstance, would double or triple code size, and
// would require maintenance of buggy legacy code which is only useful for old
// demos. Doom demos, which are more important IMO, are not affected by this
// change.


//Spawn all friction.
// One specific sector, or NULL for all linedef and sectors
// Called before playing game or playing demo
static void P_SpawnFriction( sector_t * sec )
{
    line_t * lnp = lines;
    int fsecn;
    fixed_t friction;   // friction value to be applied during movement
    int movefactor; // applied to each player move to simulate inertia

    uint32_t ki;
    for (ki = 0 ; ki < numlines ; ki++,lnp++)
    {
        if (lnp->special == 223)  // Boom Friction by length linedef
        {
            if( sec && (sec->tag != lnp->tag) ) // specific sector test
                continue;
            // From Boom ref:
            // The length of the linedef controls the friction in the tagged sector.
            // Length < 100 : stickiness, like mud
            // Length > 100 : slippery, like ice
            // Only works on a like tagged sector with the friction enable bit set,
            // which allows the effect to be turned on/off.
            // Demo sync is a known problem (see prboom).

            // line length controls magnitude
            int length = P_AproxDistance(lnp->dx,lnp->dy)>>FRACBITS;
            // [WDJ] ZDoom uses 0xD001, prboom uses 0xD000
            // At length=100, friction should equal ORIG_FRICTION = 0xE800
//            friction = (0x1EB8*length)/0x80 + 0xD000;
            friction = (0x1EB8*length)/0x80 + 0xD001;

            // [WDJ] friction and movefactor limiting appeared in
            // mbf, prboom, and legacy demos.
            // I see no reason to exclude it for older demos, as it only prevents weird effects.
            if(friction > FRACUNIT)
                friction = FRACUNIT;
            if(friction < 0)
                friction = 0;

            // The following check might seem odd. At the time of movement,
            // the move distance is multiplied by 'friction/0x10000', so a
            // higher friction value actually means 'less friction'.

            if (friction > ORIG_FRICTION)       // ice
            {
                if( EN_heretic_hexen )
                {
                  // heretic or hexen
                  // [WDJ] From ZDoom calc of momentum to equal heretic/hexen at friction=0xf900
                  movefactor = ((0x10092 - friction)*(0x40))/0x110 + 0x238;
                }
                else
                  movefactor = ((0x10092 - friction)*(0x70))/0x158; // boom ice
            }
            else
                movefactor = ((friction - 0xDB34)*(0xA))/0x80;  // mud

// [WDJ] To see friction calculations, uncomment this print
// debug_Printf( "Friction Line  length = %d, Friction = %X, Movefactor = %d\n", length, friction, movefactor);

            // killough 8/28/98: prevent odd situations
            if (movefactor < 32)
                movefactor = 32;

            if( sec )
            {
                // update one specific sector
                sec->friction = friction;
                sec->movefactor = movefactor;
#ifdef FRICTIONTHINKER
                // [WDJ] Friction thinkers are obsolete.
                if( EN_boom_friction_thinker )
                {
                    // Thinkers used only for some demos.
                    Add_Friction(friction, movefactor, sec-sectors);
                }
#endif
                continue;
            }

            // set friction in all sectors with the same linedef tag
            fsecn = -1; // init search FindSector
            while ((fsecn = P_FindSectorFromLineTag(lnp,fsecn)) >= 0)
            {
                // Sector friction
                sectors[fsecn].friction = friction;
                sectors[fsecn].movefactor = movefactor;
#ifdef FRICTIONTHINKER
                // [WDJ] Friction thinkers are obsolete.
                if( EN_boom_friction_thinker )
                {
                    // Thinkers used only for some demos.
                    Add_Friction(friction, movefactor, fsecn);
                }
#endif
            }
        }
    }
}


// [WDJ] 3/2011 To support using friction field when change of FRICTION_MASK occurs.
// Update sector fields after a change of special type.
void P_Update_Special_Sector( sector_t * sec, short new_special )
{
    sec->special = new_special;

    // Normal sector first, FRICTION_MASK might be off or might not find friction linedef.
    // normal friction default
    sec->friction = FRICTION_NORM;
    sec->movefactor = ORIG_FRICTION_FACTOR;
    if (EN_heretic
        && new_special == 15 )   // Friction_Low
    {
        sec->friction = FRICTION_LOW;
        sec->movefactor = ORIG_FRICTION_FACTOR/4;
    }
    else if( sec->model >= SM_fluid )
    {
        if( friction_model == FR_legacy )
        {  // water_friction
            sec->friction = FRICTION_NORM * 97/100;  // slog
            sec->movefactor = 8*((FRICTION_NORM-48 - 0xDB34)*(0xA))/0x80; // non stick
        }
    }
    // FRICTION_MASK is Boom generalized sector type ( special > 32 )
    // Can override other sector special friction
    // Or, there might not be a friction linedef with the tag number.
    if( new_special & FRICTION_MASK )
    {
        // With FRICTION_MASK, must recalculate the friction.
        // Re-spawn for this sector
        P_SpawnFriction( sec );
    }
}


/*
  SoM: 3/8/2000: Push/Pull/Wind/Current functions.
  Add_Pusher,
  PIT_PushThing,
  T_Pusher,
  P_GetPushThing,
  P_SpawnPushers
*/


#define PUSH_FACTOR 7

// Adds a pusher
static void Add_Pusher(int type, fixed_t x_mag, fixed_t y_mag, mobj_t* source, int affectee)
{
    pusher_t *p = Z_Malloc(sizeof *p, PU_LEVSPEC, 0);

    p->thinker.function = TFI_Pusher;
    p->source = source;
    p->type = type;
    p->x_mag = x_mag>>FRACBITS;
    p->y_mag = y_mag>>FRACBITS;

    // Magnitude is used for something else for vertical currents
    // SSNTails 06-14-2002
    if(type == PP_downcurrent || type == PP_upcurrent || type == PP_upwind || type == PP_downwind)
        p->magnitude = P_AproxDistance(p->x_mag,p->y_mag)<<(FRACBITS-PUSH_FACTOR);
    else
        p->magnitude = P_AproxDistance(p->x_mag,p->y_mag);

    if (source) // point source exist?
    {
        p->radius = (p->magnitude)<<(FRACBITS+1); // where force goes to zero
        p->x_src = p->source->x;
        p->y_src = p->source->y;
    }
    p->affectee = affectee;
    P_AddThinker(&p->thinker);
}


// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.
// Only used for PP_push

pusher_t* tmpusher; // pusher structure for blockmap searches

boolean PIT_PushThing(mobj_t* thing)
{
    if (thing->player &&
        !(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
    {
        angle_t pushangle;
        fixed_t dist;
        int speed;
        fixed_t sx,sy;

        sx = tmpusher->x_src;
        sy = tmpusher->y_src;
        dist = P_AproxDistance(thing->x - sx,thing->y - sy);
        speed = (tmpusher->magnitude -
                 ((dist>>FRACBITS)>>1))<<(FRACBITS-PUSH_FACTOR-1);

        // Square law distance effect by Killough 10/98, from prboom (not in Boom)
        // [WDJ] Modified to float
        if( EN_mbf_speed && (speed > 0) )
        {
            float fdx = thing->x - sx;
            float fdy = thing->y - sy;
            float mag = tmpusher->magnitude;
            speed = (int)( mag * ((float)(1<<23) * (float)(1<<FRACBITS) * (float)(1<<FRACBITS) * 2 )
                    / ( fdx*fdx + fdy*fdy + 1));
        }

        // If speed <= 0, you're outside the effective radius. You also have
        // to be able to see the push/pull source point.

        if ((speed > 0) && (P_CheckSight(thing,tmpusher->source)))
        {
            pushangle = R_PointToAngle2(thing->x,thing->y,sx,sy);
            if (tmpusher->source->type == MT_PUSH)
                pushangle += ANG180;    // away
            thing->momx += FixedMul(speed, cosine_ANG(pushangle));
            thing->momy += FixedMul(speed, sine_ANG(pushangle));
        }
    }
    return true;
}


// T_Pusher looks for all objects that are inside the radius of
// the effect.

void T_Pusher(pusher_t * p)
{
    sector_t *sec;
    mobj_t   *thing;
    msecnode_t* node;
    int xspeed = 0, yspeed = 0;
    int xl,xh,yl,yh,bx,by;
    int radius;
    fixed_t sm_ht = 0;
    boolean water_sector = false;
    boolean inwater = false;
    boolean touching = false;

    if (!EN_pushers)
        return;

    sec = & sectors[ p->affectee ];

    // Be sure the special sector type is still turned on. If so, proceed.
    // Else, bail out; the sector type has been changed on us.

    if( EV_legacy <= 140 )
    {
        if (!(sec->special & PUSH_MASK))
            return;
    }
    else // Now you can have a PUSH_MASK for individual 3d floors. SSNTails 09-25-2002
    {
        if (!(sec->special & PUSH_MASK)) // Main sector doesn't have one, so let's check the rovers.
        {
            boolean foundfloor = false;

            if(sec->ffloors)
            {
                ffloor_t*  rover;

                for(rover = sec->ffloors; rover; rover = rover->next)
                {
                    // Do some small extra checks here to possibly save unneeded work.
                    if(!(rover->master->frontsector->special & PUSH_MASK))
                         continue;
                    foundfloor = true;
                }
            }

            if(! foundfloor) // Not even a 3d floor has the PUSH_MASK.
                return;
        }
    }

    // For constant pushers (wind/current) there are 3 situations:
    //
    // 1) Affected Thing is above the floor.
    //
    //    Apply the full force if wind, no force if current.
    //
    // 2) Affected Thing is on the ground.
    //
    //    Apply half force if wind, full force if current.
    //
    // 3) Affected Thing is below the ground (underwater effect).
    //
    //    Apply no force if wind, full force if current.
    //
    // Apply the effect to clipped players only for now.
    //
    // In Phase II, you can apply these effects to Things other than players.

    if (p->type == PP_push)
    {

        // Seek out all pushable things within the force radius of this
        // point pusher. Crosses sectors, so use blockmap.

        tmpusher = p; // MT_PUSH/MT_PULL point source
        radius = p->radius; // where force goes to zero
        tm_bbox[BOXTOP]    = p->y_src + radius;
        tm_bbox[BOXBOTTOM] = p->y_src - radius;
        tm_bbox[BOXRIGHT]  = p->x_src + radius;
        tm_bbox[BOXLEFT]   = p->x_src - radius;

        xl = (tm_bbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
        xh = (tm_bbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
        yl = (tm_bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
        yh = (tm_bbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
        for (bx=xl ; bx<=xh ; bx++)
        {
            for (by=yl ; by<=yh ; by++)
                P_BlockThingsIterator(bx,by,PIT_PushThing);
        }
        return;
    }

    // constant pushers PP_wind and PP_current

    if( EV_legacy <= 140 )
    {
        // Old Legacy, Boom, MBF
        // Uses model and modelsec, instead of the PrBoom heightsec.
        if (sec->model > SM_fluid) // special water sector
        {
           sm_ht = sectors[sec->modelsec].floorheight;
           water_sector = true;
        }

        // Boom
        node = sec->touching_thinglist; // things touching this sector
        for ( ; node ; node = node->m_snext)
        {
            thing = node->m_thing;
            if (!thing->player || (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
                continue;
            if (p->type == PP_wind)
            {
                if (! water_sector) // NOT special water sector
                {
                    if (thing->z > thing->floorz) // above ground
                    {
                        xspeed = p->x_mag; // full force
                        yspeed = p->y_mag;
                    }
                    else // on ground
                    {
                        xspeed = (p->x_mag)>>1; // half force
                        yspeed = (p->y_mag)>>1;
                    }
                }
                else // special water sector
                {
                    if (thing->z > sm_ht) // above ground
                    {
                        xspeed = p->x_mag; // full force
                        yspeed = p->y_mag;
                    }
                    else if (thing->player->viewz < sm_ht) // underwater
                        xspeed = yspeed = 0; // no force
                    else // wading in water
                    {
                        xspeed = (p->x_mag)>>1; // half force
                        yspeed = (p->y_mag)>>1;
                    }
                }
            }
            else // PP_current
            {
                // Added Z currents SSNTails 06-10-2002
                if (! water_sector) // NOT special water sector
                {
                    if (thing->z > sec->floorheight) // above ground
                        xspeed = yspeed = 0; // no force
                    else // on ground
                    {
                        if(p->type == PP_upcurrent)
                            thing->momz += p->magnitude;
                        else if(p->type == PP_downcurrent)
                            thing->momz -= p->magnitude;
                        else
                        {
                            xspeed = p->x_mag; // full force
                            yspeed = p->y_mag;
                        }
                    }
                }
                else // special water sector
                {
                    if (thing->z > sm_ht) // above ground
                        xspeed = yspeed = 0; // no force
                    else // underwater
                    {
                        if(p->type == PP_upcurrent)
                            thing->momz += p->magnitude;
                        else if(p->type == PP_downcurrent)
                            thing->momz -= p->magnitude;
                        else
                        {
                            xspeed = p->x_mag; // full force
                            yspeed = p->y_mag;
                        }
                    }
                }
            }

            if(p->type != PP_downcurrent && p->type != PP_upcurrent)
            {
                thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
                thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
            }

        }
    }
    else // New support
    {
        // Kudos to P_MobjCheckWater().
        // SSNTails 09-25-2002
        // Excludes Boom_deep_water for some reason.

        // [WDJ] Was very confused, separate the deep water,
        // surface water and lava.
        // Boom_deep_water re-enabled.

        // [WDJ] An old test for Legacy_water was wrapped around everything
        // and prevented detecting FLOOR_WATER and LAVA.

        // Sector tests, independent of nodes
        if (sec->model > SM_fluid)	// Legacy and Boom water
        {
            sm_ht = (sectors[sec->modelsec].floorheight);
            water_sector = true;
        }
        else if (sec->floortype == FLOOR_WATER || sec->floortype == FLOOR_LAVA) // Lava support
        {
            sm_ht = sec->floorheight + (FRACUNIT/4); // water texture
            water_sector = true;
        }

       
        node = sec->touching_thinglist; // things touching this sector
        for ( ; node ; node = node->m_snext)
        {
            thing = node->m_thing;
            if (!thing->player || (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
                continue;

            touching = false;  // [WDJ] reset so each thing is independent
            inwater = false;

            // Find the area that the 'thing' is in
            if(water_sector) // Sector has water
            {
                // Is possible to have touching and inwater at same time
                // with these tests.
                if ((thing->z <= sm_ht) && (thing->z+thing->height > sm_ht))
                  touching = true;	// touching, but part above water

                if (thing->z+(thing->height>>1) <= sm_ht)
                  inwater = true;	// more than half in water
            }

            // Not "else"! Check ALL possibilities!
            // Can skip fake floors if both are already set.
            if(( !(inwater && touching)) // Do fake floors if either is false
                && sec->ffloors)
            {
                // water not decisive, check fake floors
                ffloor_t*  rover;

                for(rover = sec->ffloors; rover; rover = rover->next)
                {
                    if(*rover->topheight < thing->z || *rover->bottomheight > (thing->z + (thing->height >> 1)))
                        continue;

                    if(!(rover->master->frontsector->special & PUSH_MASK))
                        continue;

                    // check for immersed in a fake floor
                    if(thing->z + thing->height > *rover->topheight)
                        touching = true;

                    if(thing->z + (thing->height >> 1) < *rover->topheight)
                        inwater = true;
                }
            }

            if (thing->z == sec->floorheight)
                touching = true;

            if (p->type == PP_wind)
            {
                if (!touching && !inwater) // above ground
                {
                    xspeed = p->x_mag; // full force
                    yspeed = p->y_mag;
                }
                else if (touching) // on ground
                {
                    xspeed = (p->x_mag)>>1; // half force
                    yspeed = (p->y_mag)>>1;
                }
                else if (inwater) // underwater
                    xspeed = yspeed = 0; // no force
                else
                    xspeed = yspeed = 0;
            }
            else if (p->type == PP_upwind)
            {
                if (!touching && !inwater) // above ground
                {
                    thing->momz += p->magnitude;
                }
                else if (touching) // on ground
                {
                    thing->momz += (p->magnitude)>>1;
                }
                else if (inwater) // underwater
                    xspeed = yspeed = 0; // no force
                else
                    xspeed = yspeed = 0;
            }
            else if (p->type == PP_downwind)
            {
                if (!touching && !inwater) // above ground
                {
                    thing->momz -= p->magnitude;
                }
                else if (touching) // on ground
                {
                    thing->momz -= (p->magnitude)>>1;
                }
                else if (inwater) // underwater
                    xspeed = yspeed = 0; // no force
                else  
                    xspeed = yspeed = 0;
            }
            else // PP_current
            {
                // Added Z currents SSNTails 06-10-2002
                if(!touching && !inwater) // Not in water at all
                    xspeed = yspeed = 0; // no force
                else // underwater / touching water
                {
                    if(p->type == PP_upcurrent)
                        thing->momz += p->magnitude;
                    else if(p->type == PP_downcurrent)
                        thing->momz -= p->magnitude;
                    else
                    {
                        xspeed = p->x_mag; // full force
                        yspeed = p->y_mag;
                    }
                }
            }

            if(p->type != PP_downcurrent && p->type != PP_upcurrent
                && p->type != PP_upwind && p->type != PP_downwind)
            {
                thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
                thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
            }
        } // for
    } // if else
}


// Get pusher object.
mobj_t* P_GetPushThing(int s)
{
    mobj_t* thing;
    sector_t* sec;

    sec = & sectors[s];
    thing = sec->thinglist;
    while (thing)
    {
        switch(thing->type)
        {
          case MT_PUSH:
          case MT_PULL:
            return thing;
          default:
            break;
        }
        thing = thing->snext;
    }
    return NULL;
}


// Spawn pushers.
static void P_SpawnPushers(void)
{
    line_t *l = lines;
    int fsecn;
    mobj_t* thing;

    uint32_t ki;
    for (ki = 0 ; ki < numlines ; ki++,l++)
    {
        fsecn = -1; // init search P_FindSector
        switch(l->special)
        {
          case 224: // wind
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
                Add_Pusher( PP_wind, l->dx, l->dy, NULL, fsecn);
            break;
          case 225: // current
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
                Add_Pusher( PP_current, l->dx, l->dy, NULL, fsecn);
            break;
          case 226: // push/pull
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
            {
                thing = P_GetPushThing(fsecn);
                if (thing) // No MT_P* means no effect
                    Add_Pusher( PP_push, l->dx, l->dy, thing, fsecn);
            }
            break;
          case 292: // current up SSNTails 06-10-2002
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
                Add_Pusher( PP_upcurrent, l->dx, l->dy, NULL, fsecn);
            break;
          case 293: // current down SSNTails 06-10-2002
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
                Add_Pusher( PP_downcurrent, l->dx, l->dy, NULL, fsecn);
            break;
          case 294: // wind up SSNTails 06-14-2003
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
                Add_Pusher( PP_upwind, l->dx, l->dy, NULL, fsecn);
            break;
          case 295: // wind down SSNTails 06-14-2003
            while ((fsecn = P_FindSectorFromLineTag(l,fsecn)) >= 0)
                Add_Pusher( PP_downwind, l->dx, l->dy, NULL, fsecn);
            break;
        } // switch
    } // for
}
