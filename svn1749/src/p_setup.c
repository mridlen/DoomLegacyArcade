// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_setup.c 1745 2025-04-10 09:36:34Z wesleyjohnson $
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
// $Log: p_setup.c,v $
// Revision 1.49  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.48  2003/06/11 03:38:09  ssntails
// THING Z definable in levels by using upper 9 bits
//
// Revision 1.47  2003/06/11 00:28:49  ssntails
// Big Blockmap Support (128kb+ ?)
//
// Revision 1.46  2003/05/04 02:37:47  sburke
// READSHORT now does byte-swapping on big-endian machines.
//
// Revision 1.45  2002/10/30 23:50:03  bock
//
// Revision 1.44  2002/09/27 16:40:09  tonyd
// First commit of acbot
//
// Revision 1.43  2002/07/24 19:03:07  ssntails
// Added support for things to retain spawned Z position.
//
// Revision 1.42  2002/07/20 03:24:45  mrousseau
// Copy 'side' from SEGS structure to seg_t's copy
//
// Revision 1.41  2002/01/12 12:41:05  hurdler
// Revision 1.40  2002/01/12 02:21:36  stroggonmeth
// Revision 1.39  2001/08/19 20:41:03  hurdler
//
// Revision 1.38  2001/08/13 16:27:44  hurdler
// Added translucency to linedef 300 and colormap to 3d-floors
//
// Revision 1.37  2001/08/12 22:08:40  hurdler
// Add alpha value for 3d water
//
// Revision 1.36  2001/08/12 17:57:15  hurdler
// Beter support of sector coloured lighting in hw mode
//
// Revision 1.35  2001/08/11 15:18:02  hurdler
// Add sector colormap in hw mode (first attempt)
//
// Revision 1.34  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.33  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.32  2001/07/28 16:18:37  bpereira
// Revision 1.31  2001/06/16 08:07:55  bpereira
// Revision 1.30  2001/05/27 13:42:48  bpereira
//
// Revision 1.29  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.28  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.27  2001/03/30 17:12:51  bpereira
//
// Revision 1.26  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.25  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.24  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.23  2000/11/04 16:23:43  bpereira
// Revision 1.22  2000/11/03 03:27:17  stroggonmeth
// Revision 1.21  2000/11/02 19:49:36  bpereira
//
// Revision 1.20  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.19  2000/10/02 18:25:45  bpereira
// Revision 1.18  2000/08/31 14:30:56  bpereira
//
// Revision 1.17  2000/08/11 21:37:17  hurdler
// fix win32 compilation problem
//
// Revision 1.16  2000/08/11 19:10:13  metzgermeister
//
// Revision 1.15  2000/05/23 15:22:34  stroggonmeth
// Not much. A graphic bug fixed.
//
// Revision 1.14  2000/05/03 23:51:00  stroggonmeth
//
// Revision 1.13  2000/04/19 15:21:02  hurdler
// add SDL midi support
//
// Revision 1.12  2000/04/18 12:55:39  hurdler
// Revision 1.11  2000/04/16 18:38:07  bpereira
// Revision 1.10  2000/04/15 22:12:57  stroggonmeth
//
// Revision 1.9  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.8  2000/04/12 16:01:59  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.7  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.6  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.5  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Do all the WAD I/O, get map description,
//             set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "p_setup.h"
#ifdef DEEPSEA_EXTENDED_NODES
#include "p_extnodes.h"  // [MB] 2020-04-21: For import of extended nodes
#endif
#include "p_spec.h"
#include "p_info.h"
#include "g_game.h"

#include "d_main.h"
#include "byteptr.h"

#include "i_sound.h"
  // I_PlayCD()..
#include "i_system.h"
  // I_Sleep

#include "r_data.h"
#include "r_things.h"
#include "r_sky.h"

#include "s_sound.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"
#include "t_array.h"
#include "t_func.h"
#include "t_script.h"

#include "hu_stuff.h"
#include "console.h"


#ifdef HWRENDER
#include "i_video.h"
  // rendermode
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#endif

#include "b_game.h"
  // added by AC for acbot

#ifdef ENABLE_UMAPINFO
#include "umapinfo.h"
#endif

//#define TRACE_BLOCKMAPHEAD
//#define DUMP_BLOCKMAP   "dmdump"


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
boolean         newlevel = false;
boolean         doom1level = false;    // doom 1 level running under doom 2

uint32_t        numvertexes;
vertex_t*       vertexes;

uint32_t        numsegs;
seg_t*          segs;

// [WDJ] numsectors limited to 0xFFFE, or else reject calc will overflow.
uint32_t        numsectors = 0;
sector_t*       sectors = NULL;

uint32_t        numsubsectors;
subsector_t*    subsectors;

uint32_t        numnodes;
node_t*         nodes;

uint32_t        numlines;
line_t*         lines;

uint32_t        numsides;
side_t*         sides;

uint16_t        nummapthings;
mapthing_t*     mapthings;

/*
typedef struct mapdata_s {
    int             numvertexes;
    vertex_t*       vertexes;
    int             numsegs;
    seg_t*          segs;
    int             numsectors;
    sector_t*       sectors;
    int             numsubsectors;
    subsector_t*    subsectors;
    int             numnodes;
    node_t*         nodes;
    int             numlines;
    line_t*         lines;
    int             numsides;
    side_t*         sides;
} mapdata_t;
*/


// BLOCKMAP
// Created from axis aligned bounding box of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection by spatial subdivision in 2D.
//
// Blockmap size.
unsigned int    bmapwidth;
unsigned int    bmapheight;     // size in mapblocks

uint32_t *      blockmapindex;       // for large maps, wad is 16bit
// offsets in blockmap are from here
uint32_t *      blockmaphead; // Big blockmap, SSNTails

// origin of block map
// The bottom left corner of the most SW block.
fixed_t         bmaporgx;
fixed_t         bmaporgy;
// for thing chains
mobj_t   **     blocklinks;


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed line-of-sight calculation.
// Without special effect, this could be used as a PVS lookup as well.
byte     *      rejectmatrix = NULL;
uint32_t        num_rejectmatrix = 0;


// Maintain single and multi player starting spots.
mapthing_t  *   deathmatchstarts[MAX_DM_STARTS];
byte            numdmstarts;  // MAX_DM_STARTS is < 255
mapthing_t  *   playerstarts[MAXPLAYERS];



// [MB] 2020-04-21: Used the woof code from p_extnodes.c instead.
// mapformat_t P_CheckMapFormat (int lumpnum)
#if 0
// [WDJ] Checks from PrBoom.
// Unfinished.

// figgi 08/21/00 -- constants and globals for glBsp support
#define gNd2  0x32644E67
#define gNd3  0x33644E67
#define gNd4  0x34644E67
#define gNd5  0x35644E67
#define ZNOD  0x444F4E5A
#define ZGLN  0x4E4C475A
#define GL_VERT_OFFSET  4



static void P_GetNodesVersion( lumpnum_t lumpnum, lumpnum_t gl_lumpnum )
{
  const void * data;

  data = W_CacheLumpNum(gl_lumpnum+ML_GL_VERTS);
  if ( (gl_lumpnum > lumpnum) && (forceOldBsp == false) && (compatibility_level >= prboom_2_compatibility) ) {
    // Check for gNd2, gNd3, gNd4, gNd5
    if( !strcasecmp( data, "gNd", 3) {
      nodesVersion = gNd4;
      I_SoftError("GL Nodes not supported\n");
    }
  } else {
    data = W_CacheLumpNum(lumpnum + ML_NODES);
    if( !strcasecmp(data, "ZNOD", 4) )
      I_SoftError("ZDoom nodes not supported");

    data = W_CacheLumpNum(lumpnum + ML_SSECTORS);
    if( !strcasecmp(data, "ZGLN", 4) )
      I_SoftError("ZDoom GL nodes not supported");
  }
}
#endif



//
// P_LoadVertexes
//
static
void P_LoadVertexes (lumpnum_t lumpnum)
{
    byte*               data;
    mapvertex_t*        ml;
    vertex_t*           li;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = W_LumpLength(lumpnum) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc (numvertexes*sizeof(vertex_t), PU_LEVEL, NULL);

    // Load data into cache.
    data = W_CacheLumpNum( lumpnum, PU_STATIC );  // vertex lump temp
    // [WDJ] Do endian as read from vertex lump temp

    ml = (mapvertex_t *)data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    uint32_t i;
    for (i=0 ; i<numvertexes ; i++, li++, ml++)
    {
        // signed
        li->x = LE_SWAP16(ml->x)<<FRACBITS;
        li->y = LE_SWAP16(ml->y)<<FRACBITS;
    }

    // Free buffer memory.
    Z_Free (data);
}


//
// Computes the line length in frac units, the glide render needs this
//
float P_SegLength (seg_t* seg)
{
    double      dx,dy;

    // make a vector (start at origin)
    dx = FIXED_TO_FLOAT(seg->v2->x - seg->v1->x);
    dy = FIXED_TO_FLOAT(seg->v2->y - seg->v1->y);

    return sqrt(dx*dx+dy*dy)*FRACUNIT;
}


//
// P_LoadSegs
//
static
void P_LoadSegs ( lumpnum_t lumpnum )
{
    byte*               data;
    mapseg_t*           ml;
    seg_t*              li;
    line_t*             ldef;
    uint16_t   side, linedef;
    uint16_t   vn1, vn2;

    // Load segments, from wad, as generated by nodebuilder.
    numsegs = W_LumpLength(lumpnum) / sizeof(mapseg_t);
    segs = Z_Malloc (numsegs*sizeof(seg_t), PU_LEVEL, NULL);
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = W_CacheLumpNum (lumpnum, PU_STATIC);  // segs lump temp
    // [WDJ] Do endian as read from segs lump temp

    if( !data || (numsegs < 1))
    {
        I_SoftError( "Bad segs data\n" );
        return;
    }
   
    ml = (mapseg_t *)data;
    li = segs;
    uint32_t i;
    for (i=0 ; i<numsegs ; i++, li++, ml++)
    {
        // [WDJ] Detect buggy wad, bad vertex number
        // signed
        vn1 = (uint16_t)LE_SWAP16(ml->v1);
        vn2 = (uint16_t)LE_SWAP16(ml->v2);
        if( vn1 > numvertexes || vn2 > numvertexes )
        {
            I_SoftError("Seg vertex bad %d,%d\n", vn1,vn2 );
            // zero both out together, make seg safer (otherwise will cross another line)
            vn1 = vn2 = 0;
        }
        li->v1 = &vertexes[vn1];
        li->v2 = &vertexes[vn2];

#ifdef HWRENDER // not win32 only 19990829 by Kin
        // [WDJ] Initialize irregardless
//        if (rendermode != render_soft)
        {
            // used for the hardware render
            li->pv1 = li->pv2 = NULL;
            li->length = P_SegLength (li);
            //Hurdler: 04/12/2000: for now, only used in hardware mode
            li->lightmaps = NULL; // list of static lightmap for this seg
        }
#endif

        li->angle = ((uint16_t)( LE_SWAP16(ml->angle) ))<<16;
        li->offset = (LE_SWAP16(ml->offset))<<16;
        linedef = (uint16_t)( LE_SWAP16(ml->linedef) );
        // [WDJ] Detect buggy wad, bad linedef number
        if( linedef > numlines ) {
            I_SoftError( "P_LoadSegs, linedef #%i, > numlines %i\n", linedef, numlines );
            linedef = 0; // default
        }
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = (uint16_t)( LE_SWAP16(ml->side) );
        if( side != 0 && side != 1 )
        {
            // [WDJ] buggy wad
            I_SoftError( "P_LoadSegs, bad side index\n");
            side = 0;  // assume was using wrong side
        }
        // side1 required to have sidenum != NULL_INDEX
        if( ldef->sidenum[side] == NULL_INDEX )
        {
            // [WDJ] buggy wad
            I_SoftError( "P_LoadSegs, using missing sidedef\n");
            side = 0;  // assume was using wrong side
        }
        li->side = side;
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;
        if( ldef-> flags & ML_TWOSIDED && (ldef->sidenum[side^1] != NULL_INDEX) )
            li->backsector = sides[ldef->sidenum[side^1]].sector;
        else
            li->backsector = NULL;

        li->numlights = 0;
        li->rlights = NULL;
    }

    Z_Free (data);
}


//
// P_LoadSubsectors
//
// Called by P_SetupLevel, after LoadLinedefs
static
void P_LoadSubsectors( lumpnum_t lumpnum )
{
    byte*               data;
    mapsubsector_t*     ms;
    subsector_t*        ss;

    // Load subsectors, from wad, as generated by nodebuilder.
    numsubsectors = W_LumpLength(lumpnum) / sizeof(mapsubsector_t);
    uint32_t sizesubsec = numsubsectors*sizeof(subsector_t);
    subsectors = Z_Malloc ( sizesubsec, PU_LEVEL, NULL);
    data = W_CacheLumpNum(lumpnum, PU_STATIC);  // subsectors lump temp
    // [WDJ] Do endian as read from subsectors temp lump

    if( !data || (numsubsectors < 1))
    {
        I_SoftError( "Bad subsector data\n" );
        return;
    }

    ms = (mapsubsector_t *)data;
    memset (subsectors, 0, sizesubsec);

    ss = subsectors;
    uint32_t i;
    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
        ss->numlines = (uint16_t)( LE_SWAP16(ms->numsegs) );
        ss->firstline = (uint16_t)( LE_SWAP16(ms->firstseg) );  // unsigned
        // cannot check if valid, segs not loaded yet
    }

    Z_Free (data);
}



//
// P_LoadSectors
//

// Return the flat size_index.
//   flatsize : the flat lump size
// Called by P_PrecacheLevelFlats at level load time.
// Called by V_DrawVidFlatFill, HWR_DrawFlatFill.
uint16_t P_flatsize_to_index( int flatsize, const char * name )
{
  // Drawing as 64*64 (lumpsize=4096) was the default.
  // Heretic LAVA flats are 4160, an odd size; use 64*64.
  // Heretic F_SKY1 flat is 4, an odd size,
  // but it is a placeholder that is never drawn.
  if( flatsize >= 2048*2048 ) // 2048x2048 lump
      return 7;
  if( flatsize >= 1024*1024 ) // 1024x1024 lump
      return 6;
  if( flatsize >= 512*512 ) // 512x512 lump
      return 5;
  if( flatsize >= 256*256 ) // 256x256 lump
      return 4;
  if( flatsize >= 128*128 ) // 128x128 lump
      return 3;
  if( flatsize >= 64*64 ) // 64x64 lump
      return 2;
  if( flatsize >= 32*32 ) // 32x32 lump
      return 1;
  if( verbose && name )
  {
      char buf[10];
      memcpy( buf, name, 8 ); // no termination on flat name
      buf[8] = 0;
      GenPrintf( EMSG_warn, "Flat size not handled, %s, size=%i\n", buf, flatsize );
  }
  return 0;
}

//
// levelflats
//
// usually fewer than 25 flats per level
#define LEVELFLAT_INC   32

unsigned int            levelflat_max = 0;  // num alloc levelflats
unsigned int            numlevelflats;  // actual in use
levelflat_t*            levelflats;

//SoM: Other files want this info.
int P_PrecacheLevelFlats( void )
{
  int flatmemory = 0;
  int i;
  int lump;

  //SoM: 4/18/2000: New flat code to make use of levelflats.
  for(i = 0; i < numlevelflats; i++)
  {
    lump = levelflats[i].lumpnum;
    levelflats[i].size_index = P_flatsize_to_index( W_LumpLength(lump), NULL );
    if(devparm)
      flatmemory += W_LumpLength(lump);
    R_GetFlat (lump);
  }
  return flatmemory;
}


// help function for P_LoadSectors, find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
//
int P_AddLevelFlat( const char* flatname )
{
    lump_name_t name8;
    levelflat_t * lfp;
    int  lf_index;

    numerical_name( flatname, & name8 );  // fast compares

    if( levelflats )
    {
        // scan through the already found flats
        lfp = & levelflats[0];
        for (lf_index=0; lf_index<numlevelflats; lf_index++)
        {
            // Fast numerical name compare.
            if( *(uint64_t *)lfp->name == name8.namecode )
            {
                goto found_level_flat;  // return levelflat index
            }
            lfp ++;
        }
    }

    // create new flat entry in levelflats
    if (devparm)
        GenPrintf(EMSG_dev, "flat %#03d: %s\n", numlevelflats, name8.s);

    if (numlevelflats>=levelflat_max)
    {
        // grow number of levelflats
        // use Z_Malloc directly because it is usually a small number
        levelflat_max += LEVELFLAT_INC;  // alloc more levelflats
        levelflat_t* new_levelflats =
            Z_Malloc (levelflat_max*sizeof(levelflat_t), PU_LEVEL, NULL);
        // must zero because unanimated are left to defaults
        memset( &new_levelflats[numlevelflats], 0,
                (levelflat_max - numlevelflats)*sizeof(levelflat_t) );

        if( levelflats )
        {
            // Copy existing levelflats to new memory.
            memcpy (new_levelflats, levelflats, numlevelflats*sizeof(levelflat_t));
            Z_Free ( levelflats );
        }
        levelflats = new_levelflats;
    }

    lf_index = numlevelflats++;
    lfp = & levelflats[lf_index];  // array moved

    // store the name
    *(uint64_t *)lfp->name = name8.namecode;

    // store the flat lump number
    lfp->lumpnum = R_FlatNumForName (flatname);
    lfp->size_index = P_flatsize_to_index( W_LumpLength(lfp->lumpnum), flatname );

 found_level_flat:
    return lf_index;    // level flat id
}


// SoM: Do I really need to comment this?
//  num : index in levelflats
char * P_FlatNameForNum(int num)
{
  if(num < 0 || num > numlevelflats)
    I_Error("P_FlatNameForNum: Invalid flatnum\n");

  return Z_Strdup(va("%.8s", levelflats[num].name), PU_STATIC, 0);
}


static
void P_LoadSectors( lumpnum_t lumpnum )
{
    byte*               data;
    mapsector_t*        ms;
    sector_t*           ss;
    int                 i;

    numsectors = W_LumpLength(lumpnum) / sizeof(mapsector_t);
    sectors = Z_Malloc (numsectors*sizeof(sector_t), PU_LEVEL, NULL);
    memset (sectors, 0, numsectors*sizeof(sector_t));
    data = W_CacheLumpNum( lumpnum, PU_STATIC );  // mapsector lump temp
    // [WDJ] Fix endian as transfer from temp to internal.

    if( !data || (numsectors < 1))
    {
        I_SoftError( "Bad sector data\n" );
        return;
    }
   
    // [WDJ] init growing flats array
    numlevelflats = 0;
    levelflat_max = 0;
    levelflats = NULL;

    ms = (mapsector_t *)data;  // ms will be ++
    ss = sectors;
    for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
        // signed
        ss->floorheight = LE_SWAP16(ms->floorheight)<<FRACBITS;
        ss->ceilingheight = LE_SWAP16(ms->ceilingheight)<<FRACBITS;

        //
        //  flats
        //
        if( strncasecmp(ms->floorpic,"FWATER",6)==0 || 
            strncasecmp(ms->floorpic,"FLTWAWA1",8)==0 ||
            strncasecmp(ms->floorpic,"FLTFLWW1",8)==0 )
            ss->floortype = FLOOR_WATER;
        else
        if( strncasecmp(ms->floorpic,"FLTLAVA1",8)==0 ||
            strncasecmp(ms->floorpic,"FLATHUH1",8)==0 )
            ss->floortype = FLOOR_LAVA;
        else
        if( strncasecmp(ms->floorpic,"FLTSLUD1",8)==0 )
            ss->floortype = FLOOR_SLUDGE;
        else
            ss->floortype = FLOOR_SOLID;

        ss->floorpic = P_AddLevelFlat (ms->floorpic);
        ss->ceilingpic = P_AddLevelFlat (ms->ceilingpic);

        ss->lightlevel = (uint16_t)( LE_SWAP16(ms->lightlevel) );
#if 0
        // [WDJ] Clamp the int16_t light field to valid values.
        // Was inadequate, some 256 light levels still got through.
        // There are some lighting tricks with glowing sectors, that
        // would be blocked.
        // Have implemented light clip tests at StoreWall.
        if( ss->lightlevel > 255 )  ss->lightlevel = 255;
        if( ss->lightlevel < 0 )    ss->lightlevel = 0;
#endif

        // all values are unsigned, but special field is signed short
        ss->special = (uint16_t)( LE_SWAP16(ms->special) );
        ss->tag = (uint16_t)( LE_SWAP16(ms->tag) );  // unsigned

        ss->thinglist = NULL;
        ss->touching_thinglist = NULL; //SoM: 4/7/2000

        ss->stairlock = 0;
        ss->nextsec = -1;
        ss->prevsec = -1;

        ss->modelsec = -1; //SoM: 3/17/2000: This causes some real problems
                           // [WDJ] Is now dependent upon model
        ss->model = SM_normal; //SoM: 3/20/2000, [WDJ] 11/14/2009
        ss->friction = ORIG_FRICTION;  // normal friction
        ss->movefactor = ORIG_FRICTION_FACTOR;
        ss->floorlightsec = -1;
        ss->ceilinglightsec = -1;
        ss->ffloors = NULL;
        ss->lightlist = NULL;
        ss->numlights = 0;
        ss->attached = NULL;
        ss->numattached = 0;
        ss->extra_colormap = NULL;
#ifdef SECTOR_FLAGS
        // Init other flags to 0.
        ss->flags = SCF_floor_moved;   // force init of light lists
#else
        ss->moved = true;  // force init of light lists
#endif
        ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
        ss->bottommap = ss->midmap = ss->topmap = -1;
        
        // ----- for special tricks with HW renderer -----
#ifdef SECTOR_FLAGS
        // SCF_pseudo_sector, SCF_virtual_floor, SCF_virtual_ceiling are init cleared.
#else
        ss->pseudoSector = false;
        ss->virtualFloor = false;
        ss->virtualCeiling = false;
#endif
        ss->sectorLines = NULL;
        ss->stackList = NULL;
        ss->lineoutLength = -1.0;
        // ----- end special tricks -----
        
    }

    Z_Free (data);

    // whoa! there is usually no more than 25 different flats used per level!!
    //debug_Printf("Load Sectors: %d flats found\n", numlevelflats);

    // set the sky flat num
    sky_flatnum = P_AddLevelFlat ("F_SKY1");

    // search for animated flats and set up
    P_Setup_LevelFlatAnims ();
}


//
// P_LoadNodes
//
static
void P_LoadNodes ( lumpnum_t lump )
{
    byte*       data;
    int         j;
    int         k;
    mapnode_t*  mn;
    node_t*     no;

    numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
    nodes = Z_Malloc (numnodes*sizeof(node_t), PU_LEVEL, NULL);
    data = W_CacheLumpNum (lump,PU_STATIC);  // mapnode_t array temp
    // [WDJ] Fix endian as transfer from temp to internal.

    // No nodes and one subsector is a trivial but legal map.
    if( (!data || (numnodes < 1)) && (numsubsectors > 1))
    {
        I_SoftError( "Bad node data\n" );
        return;
    }

    mn = (mapnode_t *)data;
    no = nodes;

    uint32_t ni;
    for (ni=0 ; ni<numnodes ; ni++, no++, mn++)
    {
        no->x = LE_SWAP16(mn->x)<<FRACBITS;
        no->y = LE_SWAP16(mn->y)<<FRACBITS;
        no->dx = LE_SWAP16(mn->dx)<<FRACBITS;
        no->dy = LE_SWAP16(mn->dy)<<FRACBITS;
        for (j=0 ; j<2 ; j++)
        {
            // [WDJ] Unsigned 16 bit children.
            // -1 is not a valid value for children, as it only invokes an I_Error.
            bsp_child_t chld = (uint16_t) ( LE_SWAP16(mn->children[j]) );

#ifdef DEEPSEA_EXTENDED_NODES

            // [MB] 2020-04-21: Changed for extended nodes
            // Must detect first as it conflicts with MAP_NF_SUBSECTOR.
            if( chld == 0xFFFF )  // special value of questionable validity
            {
                GenPrintf( EMSG_warn, "Load Node %u, side %i has -1 value\n", ni, j );
# if 1
                // [WDJ] -1 is not a valid value for children, as it only invokes an I_Error.
                // Report it as a Node error, and change it to a safe value.
                chld = 0 | NF_SUBSECTOR;
# else
#  ifdef DEEPSEA_EXTENDED_SIGNED_NODES
                // [WDJ] DEEPSEA_EXTENDED_SIGNED_NODES are obsolete, and only kept for reference.	
                chld = -1;
#  else
                chld = 0xFFFFFFFF;
#  endif
# endif	       
            }
            else
#endif
            if( chld & MAP_NF_SUBSECTOR )
            {
#ifdef DEEPSEA_EXTENDED_NODES
                // Convert normal MAP_NF_SUBSECTOR, to extended node NF_SUBSECTOR.
                chld &= ~MAP_NF_SUBSECTOR;

#  ifdef DEEPSEA_EXTENDED_SIGNED_NODES
                if( (uint32_t)chld >= numsubsectors )
#  else
                if( chld >= numsubsectors )
#  endif
                {
                    GenPrintf( EMSG_warn, "Load Node %u, side %i has bad child sector %i\n", ni, j, chld );
                    chld = 0;
                }

                chld |= NF_SUBSECTOR;
#else
                if( (chld & ~MAP_NF_SUBSECTOR) >= numsubsectors )
                {
                    GenPrintf( EMSG_warn, "Load Node %u, side %i has bad child sector %i\n", ni, j, chld );
                    chld = 0 | NF_SUBSECTOR;
                }
#endif
            }
            else	   
            if( chld >= numnodes
#  ifdef DEEPSEA_EXTENDED_SIGNED_NODES
                || ( chld < 0 )
#  endif
              )
            {
                GenPrintf( EMSG_warn, "Load Node %i, side %i has bad child node %i\n", ni, j, chld );
                // [WDJ] Invalid children value, as it only invokes an I_Error.
                // Report it as a Node error, and change it to a safe value.
                chld = 0 | NF_SUBSECTOR;
            }

            no->children[j] = chld;

            for (k=0 ; k<4 ; k++)
                no->bbox[j][k] = LE_SWAP16(mn->bbox[j][k])<<FRACBITS;
        }
    }

    Z_Free (data);
}

//
// P_LoadThings
//
void P_LoadThings (int lump)
{
  // Doom mapthing template (internally we use a modified version)
  typedef struct
  {
    int16_t   x, y;   // coordinates
    int16_t  angle;   // orientation
    uint16_t  type;   // DoomEd number
    uint16_t flags;
  } doom_mapthing_t;

    mapthing_t*         mt;
    byte * data;

    data = W_CacheLumpNum (lump,PU_LEVEL);  // temp things lump
    // [WDJ] Do endian as read from temp things lump
    nummapthings     = W_LumpLength(lump) / sizeof(doom_mapthing_t);
    mapthings        = Z_Malloc(nummapthings * sizeof(mapthing_t), PU_LEVEL, NULL);

    if( !data || (nummapthings < 1))
    {
        I_SoftError( "Bad things data\n" );
        return;
    }

    //SoM: Because I put a new member into the mapthing_t for use with
    //fragglescript, the format has changed and things won't load correctly
    //using the old method.

    doom_mapthing_t * dmt = (doom_mapthing_t *)data;
    mt = mapthings;  // allocated map things
    uint16_t i;
    for (i=0 ; i<nummapthings ; i++, mt++, dmt++)
    {
        // Do spawn all other stuff.
        // SoM: Do this first so all the mapthing slots are filled!
        mt->x = LE_SWAP16(dmt->x);
        mt->y = LE_SWAP16(dmt->y);
        mt->angle   = LE_SWAP16(dmt->angle);
        mt->type    = (uint16_t)( LE_SWAP16(dmt->type) ); // DoomEd number
        mt->options = (uint16_t)( LE_SWAP16(dmt->flags) );  // bit flags
        mt->mobj = NULL; //SoM:

        if( gamedesc_id == GDESC_tnt && gamemap == 31)
        {
            // Fix TNT MAP31 bug: yellow keycard is multiplayer only
            // Released a fixed copy of TNT later, but CDROM have this bug.
            if( mt->type == 6 )  // Yellow keycard
               mt->options &= ~MTF_MPSPAWN;  // Remove multiplayer only flag
        }

#if 0
        // PrBoom does legality checking here.
        // May need this for special thing detection and engine setup.
        P_Setup_Mapthing(mt);
#endif

        P_Spawn_Mapthing(mt, 0);
    }

    Z_Free(data);
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
    byte*               data;
    int                 i;
    maplinedef_t*       mld;
    line_t*             ld;
    vertex_t*           v1;
    vertex_t*           v2;

    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
    lines = Z_Malloc (numlines*sizeof(line_t), PU_LEVEL, NULL);
    memset (lines, 0, numlines*sizeof(line_t));
    data = W_CacheLumpNum (lump,PU_STATIC);  // temp linedefs array
    // [WDJ] Fix endian as transfer from lump temp to internal.

    if( !data || (numlines < 1))
    {
        I_SoftError( "Bad linedefs data\n" );
        return;
    }

    mld = (maplinedef_t *)data;
    ld = lines;
    for (i=0 ; i<numlines ; i++, mld++, ld++)
    {
        ld->flags = (uint16_t)( LE_SWAP16(mld->flags) );  // bit flags
        ld->special = (uint16_t)( LE_SWAP16(mld->special) );
        ld->tag = (uint16_t)( LE_SWAP16(mld->tag) );
        v1 = ld->v1 = &vertexes[ (uint16_t) LE_SWAP16(mld->v1) ];
        v2 = ld->v2 = &vertexes[ (uint16_t) LE_SWAP16(mld->v2) ];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;

        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv (ld->dy , ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }

        // Set soundorg in P_GroupLines

        // NULL_INDEX = no sidedef
        ld->sidenum[0] = (uint16_t)( LE_SWAP16(mld->sidenum[0]) );
        ld->sidenum[1] = (uint16_t)( LE_SWAP16(mld->sidenum[1]) );

        // [WDJ] detect common wad errors and make playable, similar to prboom
        if( ld->sidenum[0] == NULL_INDEX )
        {
            // linedef is required to always have valid sidedef1
            I_SoftError( "Linedef %i is missing sidedef1\n", i );
            ld->sidenum[0] = 0;  // arbitrary valid sidedef
        }
        else if ( ld->sidenum[0] >= numsides )
        {
            I_SoftError( "Linedef %i has sidedef1 bad index\n", i );
            ld->sidenum[0] = 0;  // arbitrary valid sidedef
        }
        if( ld->sidenum[1] == NULL_INDEX )
        {
            if( ld->flags & ML_TWOSIDED )
            {
                // two-sided linedef is required to always have valid sidedef2
                I_SoftError( "Linedef %i is missing sidedef2\n", i );
                // fix one or the other
//              ld->sidenum[1] = 0;  // arbitrary valid sidedef
                ld->flags &= ~ML_TWOSIDED;
            }
        }
        else if ( ld->sidenum[1] >= numsides )
        {
            I_SoftError( "Linedef %i has sidedef2 bad index\n", i );
            ld->sidenum[1] = 0;  // arbitrary valid sidedef
        }

       
        // special linedef has special sidedef1
        if (ld->sidenum[0] != NULL_INDEX && ld->special)
          sides[ld->sidenum[0]].linedef_special = ld->special;
    }

    Z_Free (data);
}


void P_LoadLineDefs2()
{
  int i;
  line_t* ld = lines;
  for(i = 0; i < numlines; i++, ld++)
  {
      if (ld->sidenum[0] != NULL_INDEX)
        ld->frontsector = sides[ld->sidenum[0]].sector;
      else
        ld->frontsector = 0;

      if (ld->sidenum[1] != NULL_INDEX)
        ld->backsector = sides[ld->sidenum[1]].sector;
      else
        ld->backsector = 0;
      
      // special linedef setup after sidedefs are loaded
      switch( ld->special ) 
      {
       case 260:  // Boom transparency
         // sidedef1 of master linedef has the transparency map
         // Similar effect to Boom, no alternatives
         {
             int eff = ld->translu_eff;  // TRANSLU_med, or TRANSLU_ext + lumpid
             short tag = ld->tag;
//             uint16_t tag = ld->tag;  // change when lines[] changes
             if( tag )
             {
                 // Same tagged linedef get it too (both sidedefs).
                 int li;
                 for( li=numlines-1; li>=0; li-- )
                 {
                     if( lines[li].tag == tag )
                        lines[li].translu_eff = eff;
                     // Cannot use special because Boom allows tagged lines
                     // to have the transparent effect simultaneously with
                     // other linedef effects.
                 }
             }
         }
      }
  }
}

#if 0
// See two part load of sidedefs, with special texture interpretation
//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
    byte*               data;
    mapsidedef_t*       msd;
    side_t*             sd;

    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
    uint32_t sizesides = numsides*sizeof(side_t);
    sides = Z_Malloc ( sizesides, PU_LEVEL, NULL);
    memset (sides, 0, sizesides);
    data = W_CacheLumpNum (lump,PU_STATIC);  // sidedefs temp lump
    // [WDJ] Do endian as read from temp sidedefs lump

    msd = (mapsidedef_t *)data;
    sd = sides;
    uint32_t si;
    for (si=0 ; si<numsides ; si++, msd++, sd++)
    {
        sd->textureoffset = LE_SWAP16(msd->textureoffset)<<FRACBITS;
        sd->rowoffset = LE_SWAP16(msd->rowoffset)<<FRACBITS;
        // 0= no-texture, never -1
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);

        sd->sector = &sectors[ (uint16_t)( LE_SWAP16(msd->sector) )];
    }

    Z_Free (data);
}
#endif

static
void Report_sidedef_bad_sector( int sdnum, int secnum )
{
    int li;

    // When WAD has not been cleaned, some editors leave sidedefs around
    // with sector_num=65535.
    if( (! verbose) && (secnum == 65535) )   return;

    for( li=numlines-1; li>=0; li-- )
    {
        if( lines[li].sidenum[0] == sdnum
            || lines[li].sidenum[1] == sdnum )
        {
            // found parent linedef
            I_SoftError( "SideDef %i has bad sector number %i, used by line %i\n",
                          sdnum, secnum, li );
            return;
        }
    }
    I_SoftError( "SideDef %i has bad sector number %i, UNUSED\n", sdnum, secnum );
}


// Two part load of sidedefs
// [WDJ] Do endian conversion in part2
void P_LoadSideDefs (int lump)
{
  numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
  sides = Z_Malloc(numsides*sizeof(side_t), PU_LEVEL, NULL);
  memset(sides, 0, numsides*sizeof(side_t));
}

// SoM: 3/22/2000: Delay loading texture names until after loaded linedefs.

// Interpret Linedef specials in sidedefs,
// after other supporting data has been loaded.
void P_LoadSideDefs2(int lump)
{
  mapsidedef_t * msdlump = W_CacheLumpNum(lump, PU_IN_USE);  // sidedefs lump
  uint16_t  detect = 0;
  // [WDJ] Do endian as read from temp sidedefs lump
  int  num, eff;

  uint32_t sdnum;
  for (sdnum=0; sdnum<numsides; sdnum++)
  {
      register mapsidedef_t *msd = & msdlump[sdnum]; // map sidedef
      register side_t *sd = & sides[sdnum];
      register sector_t *sec;

      sd->textureoffset = LE_SWAP16(msd->textureoffset)<<FRACBITS;
      sd->rowoffset = LE_SWAP16(msd->rowoffset)<<FRACBITS;

      // Refined to allow special linedef colormap (in texture name) to instead
      // be a wall texture (wall texture name instead of colormap name)
      // using the normal colormap.
      // Check if valid texture first, on failure check if valid colormap,
      // because we have func that can check texture without error.

      uint16_t secnum = (uint16_t)( LE_SWAP16(msd->sector) );
      // [WDJ] Check for buggy wad, like prboom
      if( secnum >= numsectors )
      {
          Report_sidedef_bad_sector( sdnum, secnum );
          secnum = 0; // arbitrary use of sector 0
      }
      sd->sector = sec = &sectors[secnum];

      // original linedef types are 1..141, higher values are extensions
      switch (sd->linedef_special)
      {
        case 242:  // Boom deep water, sidedef1 texture is colormap
          detect |= BDTC_boom;
          goto watermap_linedef;
        case 280:  //SoM: 3/22/2000: Legacy water type.
          // Sets topmap,midmap,bottommap colormaps, in the tagged sectors.
          // Uses the model sector lightlevel underwater, and over ceiling.
          // [WDJ] There is no good reason for HWRENDER to block recording the
          // colormaps if the worse that the hardware renderer does is ignore them.
          detect |= BDTC_legacy;
       watermap_linedef:
          {
            num = R_CheckTextureNumForName(msd->toptexture);
            if(num == -1)  // if not texture
            {
              // must be colormap
              sec->topmap = R_ColormapNumForName(msd->toptexture);
              num = 0;
            }
            sd->toptexture = num; // never set to -1

            num = R_CheckTextureNumForName(msd->midtexture);
            if(num == -1)
            {
              sec->midmap = R_ColormapNumForName(msd->midtexture);
              num = 0;
            }
            sd->midtexture = num; // never set to -1


            num = R_CheckTextureNumForName(msd->bottomtexture);
            if(num == -1)
            {
              sec->bottommap = R_ColormapNumForName(msd->bottomtexture);
              num = 0;
            }
            sd->bottomtexture = num; // never set to -1
          }
          break;   // [WDJ]  no fall through

        case 282:                       //SoM: 4/4/2000: Just colormap transfer
          // Set the colormap of all tagged sectors.

          // SoM: R_Create_Colormap will only create a colormap in software mode...
          // [WDJ] Expanded to hardware mode too.
          {
            if(msd->toptexture[0] == '#' || msd->bottomtexture[0] == '#')
            {
              // generate colormap from sidedef1 texture text strings
              sec->midmap = R_Create_Colormap_str(msd->toptexture, msd->midtexture, msd->bottomtexture);
              sd->toptexture = sd->bottomtexture = 0;
              sec->extra_colormap = &extra_colormaps[sec->midmap];
            }
            else
            {
              // textures never set to -1
              if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
                sd->toptexture = 0;
              else
                sd->toptexture = num;
              if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
                sd->midtexture = 0;
              else
                sd->midtexture = num;
              if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
                sd->bottomtexture = 0;
              else
                sd->bottomtexture = num;
            }
          }
          break;  // [WDJ]  no fall through
                  // case 282, if(render_soft), was falling through,
                  // but as 260 has same tests, the damage was benign


        case 260:  // Boom transparency
          // When tag=0: this sidedef middle texture is made translucent using TRANMAP.
          // When tag!=0: all same tagged linedef have their middle texture
          // made translucent using sidedef1.
          // If this sidedef middle texture is a TRANMAP, size=64K, then it
          // is used for the transluceny.
          // This always affects both sidedefs.
          // never set to -1, 0=no_texture
          // Do not mangle special because Boom allows tagged lines
          // to have the transparent effect simultaneously with
          // other linedef effects.
          detect |= BDTC_boom;
          eff = TRANSLU_med;  // default TRANMAP
          sd->midtexture = 0;  // default, no texture
          // texture name = "TRANMAP" means use TRANSLU_med
          if( strncasecmp("TRANMAP", msd->midtexture, 8) != 0 )
          {
              // From Boom, any lump can be transparency map if it is right size
              lumpnum_t  spec_num = W_CheckNumForName( msd->midtexture );
              if( VALID_LUMP( spec_num ) && W_LumpLength(spec_num) == 65536 )
              {
                  int translu_map_num = R_setup_translu_store( spec_num );
                  eff = TRANSLU_ext + translu_map_num;
                  // LoadLineDefs2 will propagate it to other linedefs
              }
              else
              {
                  // Not lump name or not right size
                  // midtexture is texture
                  num = R_CheckTextureNumForName(msd->midtexture);
                  sd->midtexture = (num == -1) ? 0 : num;
              }
          }
          {
              // Find parent linedef and update, otherwise would have to use
              // some field to pass translu_eff back up to it.
              int li;
              for( li=numlines-1; li>=0; li-- )
              {
                  if( lines[li].sidenum[0] == sdnum )  // found parent linedef
                  {
                      lines[li].translu_eff = eff;
                      break;
                  }
              }
          }

          num = R_CheckTextureNumForName(msd->toptexture);
          sd->toptexture = (num == -1) ? 0 : num;

          num = R_CheckTextureNumForName(msd->bottomtexture);
          sd->bottomtexture = (num == -1) ? 0 : num;
          break;
/*        case 260: // killough 4/11/98: apply translucency to 2s normal texture
          sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
            ( ! VALID_LUMP(sd->special = W_CheckNumForName(msd->midtexture)) )
            ||
            W_LumpLength(sd->special) != 65536 ?
            sd->special=0, R_TextureNumForName(msd->midtexture) :
              (sd->special++, 0) : (sd->special=0);
          sd->toptexture = R_TextureNumForName(msd->toptexture);
          sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
          break;*/ //This code is replaced.. I need to fix this though


       //Hurdler: added for alpha value with translucent 3D-floors/water
        case 300:  // Legacy solid translucent 3D floor in tagged
        case 301:  // Legacy translucent 3D water in tagged
        case 302:  // Legacy 3D fog in tagged
        case 304:  // Legacy opaque fluid (because of inside)
            // 3Dfloor slab uses model sector ceiling and floor, heights and flats.
            // Upper texture encodes the translucent alpha: #nnn  => 0..255
            // Uses model sector colormap and lightlevel
            // Interpret texture name string as decimal alpha and fogwater effect
            detect |= BDTC_legacy;
            sd->toptexture = R_Create_FW_effect( sd->linedef_special,
                                                 msd->toptexture );
            sd->bottomtexture = 0;
            sd->midtexture = R_TextureNumForName(msd->midtexture); // side texture
            break;

        default:                        // normal cases
          // SoM: Lots of people are sick of texture errors. 
          // Hurdler: see r_data.c for my suggestion
          // [WDJ] 0=no-texture, texture not found returns default texture.
          // Textures never set to -1, so these texture num are safe to use as array index.
          sd->midtexture = R_TextureNumForName(msd->midtexture);
          sd->toptexture = R_TextureNumForName(msd->toptexture);
          sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
          break;
      }
  }
  Z_Free (msdlump);

  if( detect )
        report_detect( "Linedefs", detect );
  all_detect |= detect;
}



#ifdef GENERATE_BLOCKMAP
CV_PossibleValue_t blockmap_gen_cons_t[]={{0,"Vanilla"}, {1,"Large"}, {2,"Generate"}, {3, "Auto"}, {0,NULL}};
consvar_t cv_blockmap_gen = {"blockmap_gen", "3", CV_SAVE|CV_NETVAR, blockmap_gen_cons_t, NULL};
#endif

//
// P_LoadBlockMap
//
// Read wad blockmap using int16_t wadblockmaplump[].
// Expand from 16bit wad to internal 32bit blockmap.
void P_LoadBlockMap (int lump)
{
  uint16_t * wadblockmaplump; // blockmap lump temp
  uint32_t firstlist, lastlist;  // blockmap block list bounds
  uint32_t overflow_corr = 0;
  uint32_t bme;
  uint32_t max_bme = 0;  // for detecting overflow wrap
#ifdef TRACE_BLOCKMAPHEAD
  uint32_t head_bmi = 0;
#endif
#ifdef DUMP_BLOCKMAP
  char dbmfn[MAX_WADPATH];  // DUMP_BLOCKMAP file name
  FILE * dbmfp = NULL;
#endif
  int blockmap_errors = 0;
  int count;  // number of 16 bit blockmap entries
  int i;

  blockmaphead = NULL;
  wadblockmaplump = NULL;
   
#ifdef GENERATE_BLOCKMAP
  if( cv_blockmap_gen.EV == 2 )
      goto  generate_blockmap;
#endif

  // [WDJ] when zennode has not been run, this code will corrupt Zone memory.
  // It assumes a minimum size blockmap.
  count = W_LumpLength(lump)/2;  // number of 16 bit blockmap entries
  if( count < 5 )
  {
      I_SoftError( "Missing blockmap, node builder has not been run.\n" );
#ifdef GENERATE_BLOCKMAP
      if( cv_blockmap_gen.EV >= 2 )  // Generate, Auto
          goto  generate_blockmap;
#endif
  }
#ifdef GENERATE_BLOCKMAP
  if( cv_blockmap_gen.EV == 1 )  // Large blockmaps
  {
      if( count > (0xFFFF / 4) )
          goto  generate_blockmap;
  }
#endif

   
  wadblockmaplump = W_CacheLumpNum (lump, PU_LEVEL); // blockmap lump temp
   
  // [WDJ] Do endian as read from blockmap lump temp
  blockmaphead = Z_Malloc(sizeof(*blockmaphead) * count, PU_LEVEL, NULL);

      // killough 3/1/98: Expand wad blockmap into larger internal one,
      // by treating all offsets except -1 as unsigned and zero-extending
      // them. This potentially doubles the size of blockmaps allowed,
      // because Doom originally considered the offsets as always signed.
      // [WDJ] They are unsigned in Unofficial Doom Spec.

  blockmaphead[0] = LE_SWAP16(wadblockmaplump[0]);  // map orgin_x
  blockmaphead[1] = LE_SWAP16(wadblockmaplump[1]);  // map orgin_y
  blockmaphead[2] = (uint16_t)( LE_SWAP16(wadblockmaplump[2]) );  // number columns (x size)
  blockmaphead[3] = (uint16_t)( LE_SWAP16(wadblockmaplump[3]) );  // number rows (y size)

  bmaporgx = blockmaphead[0]<<FRACBITS;
  bmaporgy = blockmaphead[1]<<FRACBITS;
  bmapwidth = blockmaphead[2];
  bmapheight = blockmaphead[3];

  firstlist = 4 + (bmapwidth*bmapheight);
  lastlist = count - 1;

#ifdef DUMP_BLOCKMAP
  snprintf( dbmfn, MAX_WADPATH-1, "%s_M%i", DUMP_BLOCKMAP, gamemap );
  dbmfp = fopen( dbmfn, "w" );
#endif
#ifdef TRACE_BLOCKMAPHEAD
  GenPrintf(EMSG_warn,"Blockmap: width=%i, height=%i, org=(%4X.%4X,%4X.%4X), firstlist=%i, count=%i\n",
            bmapwidth, bmapheight, (bmaporgx>>16), (bmaporgx&0xFFFF), (bmaporgy>>16), (bmaporgy&0xFFFF), firstlist, count );
#endif
#ifdef DUMP_BLOCKMAP
  fprintf( dbmfp, "Map: %i\n", gamemap );
  fprintf( dbmfp, "Blockmap: width=%i, height=%i, org=(%4X.%4X,%4X.%4X), firstlist=%X (%i), lastlist=%X\n",
            bmapwidth, bmapheight, (bmaporgx>>16), (bmaporgx&0xFFFF), (bmaporgy>>16), (bmaporgy&0xFFFF), firstlist, firstlist, lastlist );
  if( lastlist > 0xFFFF )
      fprintf( dbmfp, "Large Blockmap: overflow +%i\n", lastlist >> 16 );
#endif

  if( firstlist >= lastlist || bmapwidth < 1 || bmapwidth > 0xFFF0 || bmapheight < 1 || bmapheight > 0xFFF0 )
      I_Error( "Blockmap corrupt, must run node builder on wad.\n" );

  // read blockmap index array
  for (i=4 ; i<firstlist ; i++)  // for all entries in wad offset index
  {
      // [WDJ] LE_SWAP16 returns a 16 bit signed value on a big-endian machine,
      // so must make it unsigned, otherwise it can get sign extended.
      // Found by Michael Bauerle.
      bme = (uint16_t)( LE_SWAP16(wadblockmaplump[i]) );  // offset
#ifdef DUMP_BLOCKMAP
      fprintf( dbmfp,"#%6X  %6X\n", i, bme );
#endif

      if( bme == firstlist )
      {
          // Use the first list. Zennode compress will put a common empty list as first list.
          goto save_blockmap;
      }

      // upon overflow, the bme will wrap to low values
      if ( (bme < firstlist)  // too small to be valid
           && (bme < 0x2000) && ((max_bme & 0xFFFF) > 0xE000))   // wrapped
      {
          // first or repeated overflow
          uint32_t overflow_corr_n = overflow_corr + 0x00010000;
          if( overflow_corr_n >= lastlist )
          {
              GenPrintf(EMSG_warn,"Blockmap offset[%i]: Overflow 0x%X,  exceeds lastlist=0x%X\n", i, overflow_corr_n, lastlist );
#ifdef DUMP_BLOCKMAP
              fprintf( dbmfp,"Blockmap offset[%i]: Overflow 0x%X,  exceeds lastlist=0x%X\n", i, overflow_corr_n, lastlist );
#endif
#ifdef GENERATE_BLOCKMAP
              if( cv_blockmap_gen.EV > 0 )
                  goto  generate_blockmap;
#endif
          }
          else
          {
              // within lump bounds
              overflow_corr = overflow_corr_n;	      
              GenPrintf(EMSG_warn,"Blockmap offset[%i...]: Overflow correct 0x%X\n", i, overflow_corr );
#ifdef DUMP_BLOCKMAP
              fprintf( dbmfp,"Blockmap offset[%X...]: Overflow correct 0x%X\n", i, overflow_corr );
#endif
#ifdef GENERATE_BLOCKMAP
              if( cv_blockmap_gen.EV == 1 )
                  goto  generate_blockmap;
#endif
          }
      }

      if( overflow_corr == 0 )  // normal blockmap entry
      {
          goto save_blockmap;
      }

      // Blockmap is loaded before linedefs, so cannot verify blockmap by checking linedefs.
//      if( overflow_corr )
      {
          // correct for overflow
          uint32_t bmec = bme + overflow_corr;
          uint32_t bmi = blockmaphead[i-1];
          byte  have_corrected = 0;

#ifdef DUMP_BLOCKMAP
          fprintf( dbmfp, "  Overflow: prev blockmaphead[%X]=%X, max_bme=%X, corrected bmec=%X\n", i-1, bmi, max_bme, bmec );
#endif

          if( bmec <= lastlist )
          {
              uint32_t wbmec = (uint16_t)( LE_SWAP16(wadblockmaplump[bmec]) );
              uint32_t wbm3 = (bmec > 0)? (uint16_t)( LE_SWAP16(wadblockmaplump[bmec-1]) ) : 0xFFFF;
#ifdef DUMP_BLOCKMAP
              fprintf( dbmfp, "  Check list head: wadblockmap[bmec-1]=%X  wadblockmap[bmec]=%X\n", wbm3, wbmec );
#endif
              if( (wbm3 == 0xFFFF) && (wbmec == 0) ) // valid start list
              {
                  // Valid blockmap list.
                  // Note: zennode will compress, scattering the entries.
#ifdef DUMP_BLOCKMAP
                  fprintf( dbmfp, " Valid List: %X\n", bmec );
#endif
                  bme = bmec;  // accept corrected blockmap list head
                  goto save_blockmap;
              }
          }

          // not accepted above
          {
              // Test index in previous overflow_corr groups.
              // This happens when zennode compresses the blockmap by reusing exisiting linedef lists.
              // Assert: overflow_corr < lastlist
              uint32_t bmec2;
              for( bmec2 = bme; bmec2 < overflow_corr; bmec2 += 0x10000 )  // all previous overflow_corr
              {
                  uint32_t wbmec = (uint16_t)( LE_SWAP16(wadblockmaplump[bmec2]) );
                  uint32_t wbm3 = (bmec2 > 0)? (uint16_t)( LE_SWAP16(wadblockmaplump[bmec2-1]) ) : 0xFFFF;
                  if( (wbm3 == 0xFFFF) && (wbmec == 0) ) // valid start list
                  {
                      // Because node builder did not make a new entry, it must be this existing entry.
#ifdef DUMP_BLOCKMAP
                      fprintf( dbmfp, "  Found list entry at: bmec=%X\n", bmec2 );
#endif
                      bme = bmec2;
                      goto save_blockmap;
                  }
              }
          }

          // not accepted above
          GenPrintf( EMSG_warn, "Blockmap offset not a list head: bme=%X bmec=%X\n", bme, bmec );
          GenPrintf( EMSG_warn, "   prev blockmaphead[%i]=%X\n", i-1, blockmaphead[i-1] );
#ifdef DUMP_BLOCKMAP
          fprintf( dbmfp, "Blockmap offset not a list head: bme=%X bmec=%X\n", bme, bmec );
          fprintf( dbmfp, "   prev blockmaphead[%i]=%X\n", i-1, blockmaphead[i-1] );
#endif
#ifdef GENERATE_BLOCKMAP
          if( cv_blockmap_gen.EV > 0 )
             goto  generate_blockmap;
#endif

          // FIXME: Replace this with something that is more likely to find a usable blockmap list.
          // Meantime, this dumps some of the blockmap list, for diagnosis.	  
          {
              // Search for next blockmap list head, desperate last fix.
              // This assumes there is something wrong with our blockmap list decoding.	      
              uint32_t wbm1 = (uint16_t)( LE_SWAP16(wadblockmaplump[bmi]) );  // prev
              uint32_t list_head = 0;
#ifdef TRACE_BLOCKMAPHEAD
              uint32_t headcnt = 0;
#endif
              byte  fatal_error_bme = 0;

              list_head = blockmaphead[i-1];
              for( bmi = blockmaphead[i-1]+1; bmi <= lastlist; bmi++ )
              {
                  uint32_t wbm2 = (uint16_t)( LE_SWAP16(wadblockmaplump[bmi]) );
                  if(wbm2 > (0xFFFF/2))
                  {
                      // Blockmap list entry suspicious large
                      GenPrintf( EMSG_warn, "Blockmap list entry suspicious large: %X\n", wbm2 );
#ifdef DUMP_BLOCKMAP
                      fprintf( dbmfp, "Blockmap list entry suspicious large: %X\n", wbm2 );
#endif
                  }

                  if(wbm1 == 0xFFFF)  // prev was end-of-list
                  {
                      if( wbm2 == 0 )
                      {
                          // Head of list
                          list_head = bmi;

#ifdef TRACE_BLOCKMAPHEAD
                          if( (headcnt < 1) || ((headcnt < 8) && (bmi > head_bmi)) || (((bmi - bme)&0xFF) == 0) )
                          {
                              GenPrintf( EMSG_warn, "Found blockmaphead: wadblockmaplump[%X]   bme_diff=%X\n", bmi, (bmi - bme));
#ifdef DUMP_BLOCKMAP
                              fprintf( dbmfp, "Found blockmaphead: wadblockmaplump[%X]   bme_diff=%X\n", bmi, (bmi - bme));
#endif
                              if( bmi > head_bmi )  head_bmi = bmi;
                          }

                          headcnt++;
#endif

                          if( ! have_corrected )
                          {
                              GenPrintf( EMSG_warn, "Using next blockmaphead: wadblockmaplump[%X]   bme_diff=%X\n", bmi, (bmi - bme));
#ifdef DUMP_BLOCKMAP
                              fprintf( dbmfp, "Using next blockmaphead: wadblockmaplump[%X]   bme_diff=%X\n", bmi, (bmi - bme));
#endif
                              have_corrected = 1;
                              bmec = bmi;
                              break;
                          }
                      }
                      else
                      {
                          // After 0xFFFF must be 0.
                          GenPrintf( EMSG_warn, "Corrupt blockmaphead: wadblockmaplump[%X] = %X\n", bmi, wbm2 );
#ifdef DUMP_BLOCKMAP
                          fprintf( dbmfp, "Corrupt blockmaphead: wadblockmaplump[%X] = %X\n", bmi, wbm2 );
#endif
                          blockmap_errors++;
                          fatal_error_bme = 1;
                      }
                  }
                  else if( wbm2 == 0xFFFF )
                  {
                      // End of blockmap list
                      GenPrintf( EMSG_warn, " list_length=%X\n", (bmi - list_head));
#ifdef DUMP_BLOCKMAP
                      fprintf( dbmfp, " list_length=%X\n", (bmi - list_head));
#endif
                  }
                  else if( wbm1 == 0 )
                  {
                      // first entry of list
                  }

                  wbm1 = wbm2;
              }

#ifdef TRACE_BLOCKMAPHEAD
              if( headcnt > 1 )
              {
                  GenPrintf( EMSG_debug, "Found %i possible blockmaphead.\n", headcnt );
#ifdef DUMP_BLOCKMAP
                  fprintf( dbmfp, "Found %i possible blockmaphead.\n", headcnt );
#endif
              }
#endif

              if( fatal_error_bme || ( ! have_corrected ))
              {
                  goto fake_it;
              }

              bme = bmec;
          }
      }


 save_blockmap:
      if ( bme > lastlist )
      {
#ifdef DUMP_BLOCKMAP
          fprintf( dbmfp, "Blockmap offset[%i]= %X, exceeds bounds.\n", i, bme);
#endif
          I_SoftError("Blockmap offset[%i]= %X, exceeds bounds.\n", i, bme);
          goto fake_it;
      }
      if ( bme < firstlist
           || ((uint16_t)(LE_SWAP16(wadblockmaplump[bme]))) != 0 )  // not start list
      {
#ifdef DUMP_BLOCKMAP
          fprintf( dbmfp, "Bad blockmap offset[%i]= %X.\n", i, bme);
#endif
          I_SoftError("Bad blockmap offset[%i]= %X.\n", i, bme);
          goto fake_it;
      }

  store_it:
      blockmaphead[i] = bme;
      if( bme > max_bme )   max_bme = bme;   // detecting wrap
      continue;

  fake_it:
#ifdef GENERATE_BLOCKMAP
      if( cv_blockmap_gen.EV > 0 )  // other than Vanilla
         goto  generate_blockmap;
#endif
      // put something reasonable, so rest of map can be played.
      // This does not work if i=0, but in that case there is no blockmap at all,
      // and it will error out anyway.
      bme = blockmaphead[i-1];
      goto store_it;
  }
   
  if( blockmap_errors > 32 )
  {
#ifdef GENERATE_BLOCKMAP
      if( cv_blockmap_gen.EV > 0 )  // other than Vanilla
         goto  generate_blockmap;
#endif
      I_SoftError("Too many blockmap errors.\n");
      goto  blockmap_failed;
  }


#ifdef DUMP_BLOCKMAP
  fprintf( dbmfp, "Linedef lists:\n" );
#endif
   
  // read blockmap linedef lists
  for (i=firstlist ; i<count ; i++)  // for all lists in lump
  {
      // killough 3/1/98
      // Unsigned except for -1, which will be read as 0xFFFF.
      // [WDJ] LE_SWAP16 is signed on big-endian machines, so convert to prevent errors.
      uint16_t  bme = (uint16_t)( LE_SWAP16(wadblockmaplump[i]) );
      blockmaphead[i] = (bme == 0xffff)? ((uint32_t) -1) : ((uint32_t) bme);
#ifdef DUMP_BLOCKMAP
      fprintf( dbmfp, "#%6X    %6X\n", i, bme );
#endif
  }
#ifdef DUMP_BLOCKMAP
  fclose(dbmfp);
#endif

  Z_Free(wadblockmaplump);
  goto activate_blockmap;


#ifdef GENERATE_BLOCKMAP
generate_blockmap:
  // Remove remains of loading blockmap.
#ifdef DUMP_BLOCKMAP
  if( dbmfp )
      fclose(dbmfp);
#endif
  // Remove partial blockmap.
  if( blockmaphead )
  {
     Z_Free(blockmaphead);
  }
  if( wadblockmaplump )
  {
     Z_Free(wadblockmaplump);
  }

  // Create a new blockmap.
  GenPrintf( EMSG_warn, "Generate blockmap\n" );
  P_create_blockmap();
  if( ! blockmaphead )
      goto blockmap_failed;

  // success, activate the blockmap
  goto activate_blockmap;
#endif

   
activate_blockmap:
  // Normal exit.
  // Activate position index.
  blockmapindex = & blockmaphead[4];

  // clear out mobj chains
  unsigned int bm_size = sizeof(*blocklinks) * bmapwidth * bmapheight;
  blocklinks = Z_Malloc (bm_size, PU_LEVEL, NULL);
  memset (blocklinks, 0, bm_size);
  return;


blockmap_failed:
  fatal_error = true;
   
#ifdef DUMP_BLOCKMAP
  if( dbmfp )
      fclose(dbmfp);
#endif
  // Remove partial blockmap.
  if( blockmaphead )
  {
     Z_Free(blockmaphead);
     blockmaphead = NULL;
  }
  if( wadblockmaplump )
  {
     Z_Free(wadblockmaplump);
  }
  return;

/* Original
                blockmaplump = W_CacheLumpNum (lump,PU_LEVEL);
                blockmap = blockmaplump+4;
                count = W_LumpLength (lump)/2;

                for (i=0 ; i<count ; i++)
                        blockmaplump[i] = LE_SWAP16(blockmaplump[i]);

                bmaporgx = blockmaplump[0]<<FRACBITS;
                bmaporgy = blockmaplump[1]<<FRACBITS;
                bmapwidth = blockmaplump[2];
                bmapheight = blockmaplump[3];
        }

        // clear out mobj chains
        count = sizeof(*blocklinks)*bmapwidth*bmapheight;
        blocklinks = Z_Malloc (count, PU_LEVEL, NULL);
        memset (blocklinks, 0, count);
 */
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
    line_t**            linebuffer;  // to build line list
    line_t*             li;
    sector_t*           sector;
    subsector_t*        ss;
    seg_t*              seg;
    fixed_t             bbox[4];
    int                 block;
    int                 total;

    // look up sector number for each subsector
    ss = subsectors;
    uint32_t si;
    for (si=0 ; si<numsubsectors ; si++, ss++)
    {
        // [WDJ] Detct buggy wad
        if( ss->firstline > numsegs )
        {
            I_Error( "P_GroupLines: subsector firstline %i > numsegs %u\n",
                         ss->firstline, numsegs );
        }
        seg = &segs[ss->firstline];  // first seg of the subsector
        if( seg->sidedef )
        {
            // normal
            ss->sector = seg->sidedef->sector;
        }
        else
        {
            // [WDJ] prboom can play Europe.wad, but Legacy segfaults, cannot have that.
            // buggy wad, try to recover
            I_SoftError( "P_GroupLines, subsector without sidedef1 sector\n");
            // find one good reference in all the segs, like in prboom does
            uint32_t j;
            for(j=0; j < ss->numlines; j++ )
            {
                if( seg->sidedef )
                {
                    ss->sector = seg->sidedef->sector;
                    GenPrintf(EMSG_error, " found sector in seg #%d\n", j );
                    goto continue_subsectors;
                }
                seg++;  // step through segs from firstline
            }
            // subsector has no sector
            I_SoftError( "P_GroupLines, subsector defaulted to sector[0]\n");
            // [WDJ] arbitrarily use sector[0], because these usually are not visible.
            ss->sector = & sectors[0];
        }
       continue_subsectors:
        continue;
    }

    // count number of lines in each sector
    li = lines;
    total = 0;
    uint32_t k;
    for (k=0 ; k<numlines ; k++, li++) // for each line
    {
        // [WDJ] some wads ( like blowup.wad, and zdoom formats)
        // have missing frontsector, but other ports (like prboom) cope
        if( li->frontsector )
        {
            total++;
            li->frontsector->linecount++;
        }
        else
        {
            I_SoftError( "P_GroupLines, linedef #%d missing frontsector\n", k );
        }

        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

    // build line tables for each sector
    // One allocation for all, each sector using a segment of the list
    linebuffer = Z_Malloc(total*sizeof(line_t *), PU_LEVEL, NULL);
    sector = sectors;  // &sectors[0]
    uint32_t s;
    for (s=0 ; s<numsectors ; s++, sector++)
    {
        // for each sector
        M_ClearBox (bbox);
        sector->linelist = linebuffer;  // the next segment
        li = lines;  // &lines[0]
        uint32_t j;
        for (j=0 ; j<numlines ; j++, li++)
        {
            // for each line
            // check if it has this sector as a side
            if (li->frontsector == sector || li->backsector == sector)
            {
                *linebuffer++ = li; // add it to this segment
                M_AddToBox (bbox, li->v1->x, li->v1->y);
                M_AddToBox (bbox, li->v2->x, li->v2->y);
            }
        }
        // check that added lines in this segment equal the sector linecount
        if (linebuffer - sector->linelist != sector->linecount)
            I_Error ("P_GroupLines: miscounted");

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
        sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

        // adjust bounding box to map blocks
        block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight-1 : block;
        sector->blockbox[BOXTOP]=block;

        block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM]=block;

        block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth-1 : block;
        sector->blockbox[BOXRIGHT]=block;

        block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT]=block;
    }

}


// SoM: 6/27: Don't restrict maps to MAPxx/ExMx any more!
char * levellumps[] =
{
  "label",        // ML_LABEL,    A separator, name, ExMx or MAPxx
  "THINGS",       // ML_THINGS,   Monsters, items..
  "LINEDEFS",     // ML_LINEDEFS, LineDefs, from editing
  "SIDEDEFS",     // ML_SIDEDEFS, SideDefs, from editing
  "VERTEXES",     // ML_VERTEXES, Vertices, edited and BSP splits generated
  "SEGS",         // ML_SEGS,     LineSegs, from LineDefs split by BSP
  "SSECTORS",     // ML_SSECTORS, SubSectors, list of LineSegs
  "NODES",        // ML_NODES,    BSP nodes
  "SECTORS",      // ML_SECTORS,  Sectors, from editing
  "REJECT",       // ML_REJECT,   LUT, sector-sector visibility
  "BLOCKMAP",     // ML_BLOCKMAP  LUT, motion clipping, walls/grid element
  "BEHAVIOR",     // ML_BEHAVIOR  Hexen
};


#if 0
// Unused
//
// Checks for the normal level header lumps.
//   lump : the header lump
static
boolean  P_CheckHeaderLumps( lumpnum_t lump )
{
  int  i;
  int  filen, lumpn;

  filen = WADFILENUM(lump);
  if( filen > numwadfiles )  goto fail;
  
  int numlump = wadfiles[filen]->numlups;
  lumpinfo_t * lump_p = wadfiles[filen]->lumpinfo;

  lumpn = LUMPNUM(lump);

  for(i=ML_THINGS; i<=ML_BLOCKMAP; i++)
  {
      int li = lumpn + i;
      if( li > numlump )  goto fail;
      if( strncmp( lump_p[li].name, levellumps[i], 8) )  goto fail;
  }
  return true;

fail:
  return false;
}
#endif


// Checks a lump and returns whether or not it is a level header lump.
//  ml : a specific lump type or 0 to search all  (0..11)
static
int  P_CheckLumpName( lumpnum_t lump, byte ml )
{
  uint32_t filen;
  uint32_t lumpn;

  filen = WADFILENUM(lump);
  if( filen > numwadfiles )
      goto fail;

  lumpn = LUMPNUM(lump);
  if( lumpn > wadfiles[filen]->numlumps )
      goto fail;

  lumpinfo_t * lump_p = wadfiles[filen]->lumpinfo;
  const char * lump_name = lump_p[lumpn].name;

  byte ml2 = ml;
  if( ml == 0)
  {   // search
      ml = ML_THINGS;
      ml2 = ML_BEHAVIOR;
  }

  for( ; ml<=ml2; ml++)
  {
      if( strncmp( lump_name, levellumps[ml], 8) == 0 )
          return ml;
  }

fail:
  return -1;
}


//
// Setup sky texture to use for the level, actually moved the code
// from G_DoLoadLevel() which had nothing to do there.
//
// - in future, each level may use a different sky.
//
// The sky texture to be used instead of the F_SKY1 dummy.
void P_Setup_LevelSky (void)
{
#ifdef ENABLE_UMAPINFO
    const char * sn = "SKY1";
#else
    char * sn = "SKY1";
#endif
    char   skytexname[12];

    // DOOM determines the sky texture to be used
    // depending on the current episode, and the game version.

    if(*info_skyname)
      sn = info_skyname;
    else
    if (gamemode == doom2_commercial)  // includes doom2, plut, tnt
      // || (gamemode == pack_tnt) he ! is not a mode is a episode !
      //    || ( gamemode == pack_plut )
    {
        if (gamemap < 12)
            sn = "SKY1";
        else
        if (gamemap < 21)
            sn = "SKY2";
        else
            sn = "SKY3";
    }
    else
    if ( (gamemode==ultdoom_retail) ||
         (gamemode==doom_registered) )
    {
        if (gameepisode<1 || gameepisode>4)     // useful??
            gameepisode = 1;

        sprintf (skytexname,"SKY%d",gameepisode);
        sn = & skytexname[0];
    }
    else // who knows?
    if (gamemode==heretic)
    {
        static char *skyLumpNames[5] = {
            "SKY1", "SKY2", "SKY3", "SKY1", "SKY3" };

        if(gameepisode > 0 && gameepisode <= 5)
            sn = skyLumpNames[gameepisode-1];
    }

    sky_texture = R_TextureNumForName ( sn );
    // scale up the old skies, if needed
    R_Setup_SkyDraw ();
}


//
// P_SetupLevel
//
// Load the level from an existing lump or from a external wad !
// Purge all previous PU_LEVEL memory.
lumpnum_t  level_lumpnum = 0;  // for info and comparative savegame
char*  level_mapname = NULL;  // to savegame and info

//  to_episode : change to episode num
//  to_map : change to map number
//  to_skill : change to skill
//  map_wadname : map command, load wad file
// Called from: G_DoLoadLevel, P_UnArchiveMisc() (savegame)
boolean P_SetupLevel (int      to_episode,
                      int      to_map,
                      skill_e  to_skill,
                      char*    map_wadname)      // for wad files
{
    const char  *errstr;
    char  *sl_mapname = NULL;
    int   i;
    mapformat_t mapformat;

    GenPrintf( (verbose? (EMSG_ver|EMSG_now) : (EMSG_console|EMSG_now)),
               "Setup Level\n" );

    //Initialize Boom sector node list.
    P_Init_Secnode();

    // Clear existing level variables and reclaim memory.
    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        players[i].killcount = players[i].secretcount
            = players[i].itemcount = 0;
        players[i].mo = NULL;  // will be freed with PU_LEVEL
#ifdef CLIENTPREDICTION2
        players[i].spirit = NULL;
#endif
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1;

    P_Release_PicAnims();
   
    // [WDJ] 7/2010 Free allocated memory in sectors before PU_LEVEL purge
    for (i=0 ; i<numsectors ; i++)
    {
        sector_t * sp = &sectors[i];
        if( sp )
        {
            if( sp->attached )  // from realloc in P_AddFakeFloor
            {
                free( sp->attached );
                sp->attached = NULL;
            }
        }
    }

    I_Sleep( 100 );  // give menu sound a chance to finish
    // Make sure all sounds are stopped before Z_FreeTags.
    // This will kill the last menu pistol sound too.
    S_Stop_LevelSound();

#if 0 // UNUSED
    if (debugfile)
    {
        Z_FreeTags (PU_LEVEL, 255);  // all purge tags
        Z_FileDumpHeap (debugfile);
    }
#endif

    Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);
   
    // [WDJ] all temp lumps are unlocked, to be freed unless they are accessed first
    Z_ChangeTags_To (PU_LUMP, PU_CACHE);
    Z_ChangeTags_To (PU_IN_USE, PU_CACHE);  // for any missed otherwise

#ifdef WALLSPLATS
    // clear the splats from previous level
    R_Clear_LevelSplats ();
#endif
    P_Clear_Extra_Mapthing();  // remove FS mapthings 

    script_camera_on = false;
    HU_Clear_Tips();

    if (camera.chase)
        camera.mo = NULL;

    // UNUSED W_Profile ();
    
    P_Init_Thinkers ();

    // Loading new level map.

#ifdef WADFILE_RELOAD
    // if working with a devlopment map, reload it
    W_Reload ();
#endif

    // Load the map from existing game resource or external wad file.
    if (map_wadname && map_wadname[0] )
    {
        // External wad file load.
        level_id_t level_id;

        if (!P_AddWadFile (map_wadname, &level_id) ||
            level_id.mapname==NULL)            // no maps were found
        {
            // go back to title screen if no map is loaded
            errstr = "No Maps";
            goto load_reject;
        }

        // From the added wad, returned by P_AddWadFile().
        to_episode = gameepisode = level_id.episode;
        to_map = gamemap = level_id.map;
        sl_mapname = level_id.mapname;  // mapname from P_AddWadFile
    }
    else
    {
        // Existing game map
        sl_mapname = G_BuildMapName(to_episode,to_map);
    }

    // Determine this level map name for savegame and info next level.
    if(level_mapname)   Z_Free(level_mapname);
    level_mapname = Z_Strdup(sl_mapname, PU_STATIC, 0);  // MAP01 or E1M1, etc.
    level_lumpnum = W_GetNumForName(sl_mapname);

    leveltime = 0;

    // textures are needed first
//    R_Load_Textures ();
//    R_Flush_Texture_Cache();

    R_Clear_FW_effect();  // clear and init of fog store
    R_Clear_Colormaps();  // colormap ZMalloc cleared by Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1)

#ifdef FRAGGLESCRIPT
    // load level lump info(level name etc)
    // Dependent upon level_mapname, level_lumpnum.
    P_Load_LevelInfo();
#endif
#ifdef ENABLE_UMAPINFO
    // [MB] 2023-01-22: Support for UMAPINFO added
    UMI_Load_LevelInfo();
#endif

    //SoM: We've loaded the music lump, start the music.
    S_Start_LevelSound();

    //faB: now part of level loading since in future each level may have
    //     its own anim texture sequences, switches etc.
    P_Init_SwitchList ();
    P_Init_PicAnims ();
    P_Init_Lava ();
    P_Setup_LevelSky ();

    // SoM: WOO HOO!
    // SoM: DOH!
    //R_Init_Portals ();

    // [WDJ] Check on Hexen-format maps, idea from PrBoom.
    if( P_CheckLumpName( level_lumpnum+ML_BEHAVIOR, ML_BEHAVIOR ) == ML_BEHAVIOR )
    {
        errstr = "Hexen format not supported";
        goto load_reject;
    }

    // [MB] 2020-04-21: Node format check from woof (p_extnodes.c)
    mapformat = P_CheckMapFormat(level_lumpnum);
    switch (mapformat)
    {
    case MFMT_DOOMBSP:
        GenPrintf(EMSG_info, "Node format: Regular\n" );
        break;
    case MFMT_DEEPBSP:
        GenPrintf(EMSG_info, "Node format: DeeP V4 extended\n" );
        break;
    case MFMT_ZDBSPX:
        GenPrintf(EMSG_info, "Node format: ZDoom extended\n" );
        break;
    case MFMT_ZDBSPZ:
        GenPrintf(EMSG_info, "Node format: ZDoom extended (compressed)\n" );
#ifdef HAVE_ZLIB
        break;
#else
        errstr = "Compressed nodes not supported";
        goto load_reject;
#endif
    default:
        errstr = "Node format not supported";
        goto load_reject;
    }

    // note: most of this ordering is important
    P_LoadVertexes (level_lumpnum+ML_VERTEXES);
    P_LoadSectors  (level_lumpnum+ML_SECTORS);
    P_LoadSideDefs (level_lumpnum+ML_SIDEDEFS);

    P_LoadLineDefs (level_lumpnum+ML_LINEDEFS);
    P_LoadSideDefs2(level_lumpnum+ML_SIDEDEFS);
    P_LoadLineDefs2();

    // Generate blockmap needs vertexes and linedefs.
    // PrBoom has here after, instead of before as in Vanilla.
    P_LoadBlockMap (level_lumpnum+ML_BLOCKMAP);
    if( fatal_error )
    {
        errstr = "Blockmap error";
        goto load_reject;
    }

#ifdef DEEPSEA_EXTENDED_NODES
    // [MB] 2020-04-21: Hook in code imported from woof 1.2.0 (p_extnodes.c)
    if (mapformat == MFMT_ZDBSPX || mapformat == MFMT_ZDBSPZ)
    {
        // Combined nodes, subsectors, and segs.  May be compressed.
        // Modifies vertexes.
        P_LoadNodes_ZDBSP (level_lumpnum+ML_NODES, mapformat == MFMT_ZDBSPZ);
    }
    else if (mapformat == MFMT_DEEPBSP)
    {
        P_LoadSubsectors_DeePBSP (level_lumpnum+ML_SSECTORS);
        P_LoadNodes_DeePBSP (level_lumpnum+ML_NODES);
        P_LoadSegs_DeePBSP (level_lumpnum+ML_SEGS);
    }
    else
#endif
    {
        // [WDJ] Normal node format loaders.        
        P_LoadSubsectors (level_lumpnum+ML_SSECTORS);
        P_LoadNodes (level_lumpnum+ML_NODES);
        P_LoadSegs (level_lumpnum+ML_SEGS);
    }

    // [WDJ] Limits numsectors to 0xFFFE, or else reject calc will overflow.
    // For many wads this will be empty (NULL).
    {
        lumpnum_t rln = level_lumpnum + ML_REJECT;  // reject lump
        rejectmatrix = W_CacheLumpNum ( rln, PU_LEVEL );
        num_rejectmatrix = (uint32_t) W_LumpLength( rln );  // to protect
    }

    P_GroupLines ();

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_SetupLevel();
    }
#endif

    bodyqueslot = 0;

    numdmstarts = 0;
    // added 25-4-98 : reset the players starts
    //SoM: Set pointers to NULL
    for(i=0;i<MAXPLAYERS;i++)
       playerstarts[i] = NULL;

    P_Init_AmbientSound ();
    P_Init_Monsters ();
    P_OpenWeapons ();
    P_LoadThings (level_lumpnum+ML_THINGS);
    P_CloseWeapons ();

    // set up world state
    P_SpawnSpecials ();
    P_Init_BrainTarget();

    //BP: spawnplayers after all structures are inititialized
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            if( deathmatch )
            {
                G_DoReborn(i);
            }
            else if( EV_legacy >= 128 )
            {
                G_CoopSpawnPlayer(i);
            }
        }
#ifdef DOGS       
        else if( extra_dog_count < cv_mbf_dogs.EV )
        {
            G_SpawnExtraDog( playerstarts[i] );
        }
#endif       
    }

    // clear special respawning que
    iquehead = iquetail = 0;

    // build subsector connect matrix
    //  UNUSED P_ConnectSubsectors ();

#ifdef CDMUS
    //Fab:19-07-98:start cd music for this level (note: can be remapped)
    if (gamemode==doom2_commercial)
        I_PlayCD (to_map, true);                // Doom2, 32 maps
    else
        I_PlayCD ((to_episode-1)*9+ to_map, true);  // Doom1, 9maps per episode
#endif

    // preload graphics
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_Preload_Graphics();
    }
#endif

    if (precache)
        R_PrecacheLevel ();


#ifdef FRAGGLESCRIPT
    T_Init_FSArrayList();         // Setup FS array list
    T_PreprocessScripts();        // preprocess FraggleScript scripts
#endif

    script_camera_on = false;

    B_Init_Nodes();  //added by AC for acbot

    //debug_Printf("P_SetupLevel: %d vertexs %d segs %d subsector\n",numvertexes,numsegs,numsubsectors);
    return true;

load_reject:
    // If want error messages to be seen, need a delay, or else the screen will be redrawn.
    GenPrintf( EMSG_hud|EMSG_now, "%s: %s\n", errstr, level_mapname );
    I_Sleep(4000);
    I_SoftError("%s: %s\n", errstr, level_mapname);
    return false;
}


static void  P_process_wadfile( int wadfilenum, /*OUT*/ level_id_t * firstmap_out );

// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
//  wadfilename : filename of wad to be loaded 
//  firstmap_out : /*OUT*/  info about the first level map
// Called by Command_Addfile, CL_Load_ServerFiles.
boolean P_AddWadFile (char* wadfilename, /*OUT*/ level_id_t * firstmap_out )
{
    int  wadfilenum;

    if( firstmap_out )
       firstmap_out->mapname = NULL;

    wadfilenum = W_Load_WadFile( wadfilename );
    if( wadfilenum == -1 )
    {
        GenPrintf(EMSG_warn, "could not load wad file %s\n", wadfilename);
        return false;
    }

#ifdef ZIPWAD
    wadfile_t * wadfile = wadfiles[wadfilenum];
    if( wadfile->classify == FC_zip )
    {
        // A zip file
        int num = wadfile->archive_num_wadfile;
        while( num-- )
        {
            // all wadfile in archive follow it in sequence
            wadfilenum ++;
            P_process_wadfile( wadfilenum, /*OUT*/ firstmap_out );
        }
        return true;
    }
#endif

    P_process_wadfile( wadfilenum, /*OUT*/ firstmap_out );
    return true;
}


//   wadfilenum : from W_Load_WadFile  (eventually need it for making a lumpnum)
//   firstmap_out : see P_SetupLevel
static
void  P_process_wadfile( int wadfilenum, /*OUT*/ level_id_t * firstmap_out )
{
    wadfile_t *  wadfile;
    lumpinfo_t * lumpinfo;
    char*       name;
    int         first_map_replaced, map_num;
    int         i,j;
    int         replace_cnt;

    //
    // search for sound replacements
    //
    wadfile = wadfiles[wadfilenum];
    lumpinfo = wadfile->lumpinfo;
    if( lumpinfo == NULL )
        return;

    replace_cnt = 0;
    for (i=0; i<wadfile->numlumps; i++,lumpinfo++)
    {
        name = lumpinfo->name;
        if (name[0]=='D' && name[1]=='S')
        {
            for (j=1 ; j < NUMSFX_EXT ; j++)
            {
                if ( S_sfx[j].name
                    && !S_sfx[j].link_id
                    && !strncasecmp(S_sfx[j].name,name+2,6) )
                {
                    // the sound will be reloaded when needed,
                    // since sfx->data will be NULL
                    if (devparm)
                        GenPrintf(EMSG_dev, "Sound %.8s replaced\n", name);

                    S_FreeSfx (&S_sfx[j]);

                    replace_cnt++;
                }
            }
        }
    }
    if (!devparm && replace_cnt)
        GenPrintf(EMSG_dev, "%d sounds replaced\n", replace_cnt);

    //
    // search for music replacements
    //
    lumpinfo = wadfile->lumpinfo;
    replace_cnt = 0;
    for (i=0; i<wadfile->numlumps; i++,lumpinfo++)
    {
        name = lumpinfo->name;
        if (name[0]=='D' && name[1]=='_')
        {
            if (devparm)
                GenPrintf(EMSG_dev, "Music %.8s replaced\n", name);
            replace_cnt++;
        }
    }
    if (!devparm && replace_cnt)
        GenPrintf(EMSG_dev, "%d musics replaced\n", replace_cnt);

    //
    // search for sprite replacements
    //
    R_AddSpriteDefs (sprnames, numwadfiles-1);

    // [WDJ] This previously would try to detect texture changes.
    // But any change of patch or texture will invalidate the current
    // cached textures, so the only safe thing to do is rebuild them all.
    // This fixes bad textures on netgames, and after Map command.
    R_Flush_Texture_Cache();  // clear all previous
    R_Load_Textures();       // reload all textures

    //
    // look for skins
    //
    R_AddSkins (wadfilenum);      //faB: wadfile index in wadfiles[]

    //
    // search for maps
    //
    lumpinfo = wadfile->lumpinfo;
    first_map_replaced = INT_MAX;  // invalid
    for (i=0; i<wadfile->numlumps; i++,lumpinfo++)
    {
        name = lumpinfo->name;
        // map_num format:  Episode: 8 bits, map: 8 bits
        //  which allows test for lowest in one operation.
        map_num = 0;  // invalid
        if (gamemode==doom2_commercial)       // Doom2
        {
            if(    name[0]=='M'
                && name[1]=='A'
                && name[2]=='P' )
            {
                map_num = (name[3]-'0')*10 + (name[4]-'0');
                GenPrintf(EMSG_info, "Map %d\n", map_num);
            }
        }
        else
        {
            if (   name[0]=='E'
                && ((unsigned)name[1]-'0')<='9'   // a digit
                && name[2]=='M'
                && ((unsigned)name[3]-'0')<='9'
                && name[4]==0   )
            {
                map_num = ((name[1]-'0')<<8) + (name[3]-'0');
                GenPrintf(EMSG_info, "Episode %d map %d\n", name[1]-'0',
                                                    name[3]-'0');
            }
        }
        // The lowest numbered map is the first map. No map 0.
        if ( (map_num > 0) && (map_num < first_map_replaced) )
        {
            first_map_replaced = map_num;
            if(firstmap_out)  /* OUT */
            {
                firstmap_out->mapname = name;
                firstmap_out->episode = map_num >> 8;
                firstmap_out->map = map_num & 0xFF;
            }
        }
    }

    if ( first_map_replaced >= 0xFFFF )  // invalid
        GenPrintf(EMSG_info, "No maps added\n");

    // reload status bar (warning should have valid player !)
    if( gamestate == GS_LEVEL )
        ST_Start();
}
