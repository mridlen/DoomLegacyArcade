// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_defs.h 1728 2025-02-15 10:16:38Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2015 by DooM Legacy Team.
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
// $Log: r_defs.h,v $
// Revision 1.35  2003/05/04 04:14:08  sburke
// Prevent misaligned access on Solaris/Sparc.
//
// Revision 1.34  2002/07/20 03:23:20  mrousseau
// Added 'side' to seg_t
//
// Revision 1.33  2002/01/12 02:21:36  stroggonmeth
// Revision 1.32  2001/08/19 20:41:04  hurdler
// Revision 1.31  2001/08/13 22:53:40  stroggonmeth
//
// Revision 1.30  2001/08/12 17:57:15  hurdler
// Beter support of sector coloured lighting in hw mode
//
// Revision 1.29  2001/08/11 15:18:02  hurdler
// Add sector colormap in hw mode (first attempt)
//
// Revision 1.28  2001/08/09 21:35:17  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.27  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.26  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.25  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.24  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.23  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.22  2001/03/30 17:12:51  bpereira
// Revision 1.21  2001/03/21 18:24:39  stroggonmeth
//
// Revision 1.20  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.19  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.18  2001/02/28 17:50:55  bpereira
//
// Revision 1.17  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.16  2000/11/21 21:13:17  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.15  2000/11/09 17:56:20  stroggonmeth
//
// Revision 1.14  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.13  2000/10/07 20:36:13  crashrl
// Added deathmatch team-start-sectors via sector/line-tag and linedef-type 1000-1031
//
// Revision 1.12  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.11  2000/07/01 09:23:49  bpereira
// Revision 1.10  2000/04/18 17:39:39  stroggonmeth
// Revision 1.9  2000/04/18 12:55:39  hurdler
// Revision 1.7  2000/04/15 22:12:58  stroggonmeth
//
// Revision 1.6  2000/04/12 16:01:59  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.5  2000/04/11 19:07:25  stroggonmeth
//
// Revision 1.4  2000/04/06 20:47:08  hurdler
// add Boris' changes for coronas in doom3.wad
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
//      Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------


#ifndef R_DEFS_H
#define R_DEFS_H

#include "doomtype.h"
#include "m_fixed.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "screen.h"
  // MAXVIDWIDTH, MAXVIDHEIGHT


// Max index (or -1). Used in line_t::sidenum and maplinedef_t::sidenum.
#ifdef DEEPSEA_EXTENDED_NODES
// Define as stdint uint16_t.
// Marks it as an unsigned value, so it does not sign extend, and compares are unsigned.
#define NULL_INDEX   UINT16_C(0xFFFF)
#else
// The plain unsigned test, for uint16_t.  Has been compatible with all 32 bit compilers involved.
#define NULL_INDEX   0xFFFF
#endif

// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
// OR bits for silhouette
typedef enum {
   SIL_BOTTOM  = 0x01,
   SIL_TOP     = 0x02
} Silhouette_b;


// SoM: Moved this here...
// This could be wider for >8 bit display.
// Indeed, true color support is possible
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef byte    lighttable_t;  // light map table
   // can be an array of map tables [256], or just one
// index a lighttable by mult by sizeof lighttable ( *256  =>  <<8 )
#define LIGHTTABLE(t)   ((t)<<8)

// right shift to convert 0..255 to 0..(NUM_RGBA_LEVELS-1)
//#define NUM_RGBA_LEVELS  4
//#define LIGHT_TO_RGBA_SHIFT  6
//#define NUM_RGBA_LEVELS  8
//#define LIGHT_TO_RGBA_SHIFT  5
#define NUM_RGBA_LEVELS  16
#define LIGHT_TO_RGBA_SHIFT  4
//#define NUM_RGBA_LEVELS  32
//#define LIGHT_TO_RGBA_SHIFT  3


// SoM: ExtraColormap type. Use for extra_colormaps from now on.
typedef struct
{
  RGBA_t          maskcolor;  // 32 bit bright color
  RGBA_t          fadecolor;  // 32 bit dark color
//  [WDJ] maskalpha is in maskcolor.alpha
//  double          maskalpha;  // 0.0 .. 1.0
  uint16_t        fadestart, fadeend;
  int             fog;

  //Hurdler: rgba is used in hw mode for coloured sector lighting
  // [WDJ] Separate rgba for light levels [0]=darkest, [NUM-1]=brightest
  RGBA_t          rgba[NUM_RGBA_LEVELS]; // similar to maskcolor in sw mode
     // alpha=0..255, 0=black/white tint, 255=saturated color
     // r,g,b are the saturated color, 0..255

  lighttable_t*   colormap; // colormap tables [32][256]
} extracolormap_t;


//=========
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//  like some DOOM-alikes ("wt", "WebView") did.
//
typedef struct
{
    fixed_t     x, y;
} vertex_t;



// Each sector has an xyz_t in its center for sound origin purposes.
// [WDJ] This replaces degenmobj_t, which was prone to breakage.
// I suppose this does not handle sound from
//  moving objects (doppler), because
//  position is prolly just buffered, not updated.

//SoM: 3/23/2000: Store fake planes in a resizable array instead of just by
//heightsec field. Allows for multiple fake planes.
typedef enum
{
  FF_EXISTS            = 0x1,    //MAKE SURE IT'S VALID
  FF_SOLID             = 0x2,    //It clips things
  FF_OUTER_SIDES       = 0x4,    //Render the outside view of sides
  FF_OUTER_PLANES      = 0x8,    //Render the outside view of floor/ceiling
  FF_SLAB_SHADOW       = 0x10,   //Make two lightlist entries to contain light
  FF_NOSHADE           = 0x20,   //No light effect
  // Cut instead of Occlude
  FF_CUTSOLIDS         = 0x40,   //Must cut hidden solid pixels
  FF_CUTEXTRA          = 0x80,   //Must cut hidden translucent pixels
  FF_CUTSPRITES        = 0x100,  //Must cut sprites, Final Step in 3D water
  FF_EXTRA             = 0x200,  //Translucent, water, and fog,
                                 //It gets cut by FF_CUTEXTRAS
  FF_FLUID             = 0x400,  //Fluid surface
  FF_TRANSLUCENT       = 0x800,  //Translucent (see through)
  FF_FOG               = 0x1000, //Fog area
  FF_INNER_PLANES      = 0x2000, //Render the inside view of planes (FOG/WATER)
  FF_INNER_SIDES       = 0x4000, //Render the inside view of sides (FOG/WATER)
  FF_JOIN_SIDES        = 0x8000, //Render the join side between similar sectors
  FF_FOGFACE           = 0x10000,//Render a fogsheet in face
  FF_SWIMMABLE         = 0x20000,//Player can swim
//  FF_unused          = 0x40000,//
//  FF_unused          = 0x80000,//
} ffloortype_e;


typedef struct ffloor_s  ffloor_t;
typedef struct sector_s  sector_t;
typedef struct line_s    line_t;

// created by P_AddFakeFloor
typedef struct ffloor_s
{
  // references to model sector, to pass through changes immediately
  fixed_t        * topheight;  // model sector ceiling
  short          * toppic;
  lightlev_t     * toplightlevel;
  fixed_t        * topxoffs;
  fixed_t        * topyoffs;

  fixed_t        * bottomheight;  // model sector floor
  short          * bottompic;
  //lightlev_t   * bottomlightlevel;
  fixed_t        * bottomxoffs;
  fixed_t        * bottomyoffs;

  int              model_secnum; // model sector num used in linedef
  ffloortype_e     flags;  // draw and property flags set by special linedef
 
  line_t         * master; // the special linedef generating this floor

  sector_t       * taggedtarget; // tagged sector that is affected

  // double linked list of ffloor_t in sector
  ffloor_t       * next;
  ffloor_t       * prev;

  uint16_t         fw_effect;		// index to fweff, 0 is unused
                                        // FF_FOG, FF_TRANSLUCENT, alpha
  uint8_t          alpha;
  uint8_t          lastlight;		// light index, FF_SLAB_SHADOW
} ffloor_t;


// SoM: This struct holds information for shadows casted by 3D floors.
// This information is contained inside the sector_t and is used as the base
// information for casted shadows.
// The item for a fake-floor light list.
typedef struct lightlist_s {
  fixed_t                 height;
  uint32_t                flags;
  lightlev_t *            lightlevel;
  extracolormap_t*        extra_colormap;
  ffloor_t*               caster;  // ffloor that is source of light or shadow
} ff_light_t;


// SoM: This struct is used for rendering walls with shadows casted on them...
typedef struct r_lightlist_s {
  fixed_t                 height;
  fixed_t                 heightstep;
  fixed_t                 botheight;
  fixed_t                 botheightstep;
  lightlev_t              lightlevel;
//  lightlev_t              vlight;  // visible light 0..255
  lighttable_t**          vlightmap;  // scalelights
  extracolormap_t*        extra_colormap; // colormap tables
  lighttable_t*           rcolormap;  // rendering colormap
  uint32_t                flags;
} r_lightlist_t;


typedef enum {
   FLOOR_SOLID,
   FLOOR_WATER,  
   FLOOR_LAVA,   
   FLOOR_SLUDGE, 
   FLOOR_ICE,
} floortype_e;

//=========
// ----- for special tricks with HW renderer -----

typedef struct msecnode_s   msecnode_t;
typedef struct linechain_s  linechain_t;

//
// For creating a chain with the lines around a sector
//
typedef struct linechain_s
{
    line_t        * line;
    linechain_t   * next;
} linechain_t;
// ----- end special tricks -----

//=========
// Sectors

#define SECTOR_FLAGS

#ifdef SECTOR_FLAGS
typedef enum {
    SCF_floor_moved   = 0x0001,
    SCF_added         = 0x0002,
    // special tricks
    SCF_pseudo_sector = 0x0010,
    SCF_virtual_floor = 0x0020,
    SCF_virtual_ceiling = 0x0040,
} sector_flags_e;
#endif

// sector model	[WDJ] 11/14/2009
typedef enum{
   SM_normal,		// normal sector
   SM_colormap,		// Legacy colormap generation
   SM_fluid,		// start of fluid sectors
   SM_Boom_deep_water,	// special Boom sector
   SM_Legacy_water,	// special Legacy sector
     // Legacy 3D floors are handled through FFloor list
} sector_model_e;

// The SECTORS record, at runtime.
// Stores things/mobjs.
typedef struct sector_s
{
    fixed_t     floorheight;
    fixed_t     ceilingheight;
    short       floorpic;
    short       ceilingpic;
    lightlev_t  lightlevel;
    uint16_t    special;	 // special type code (highly encoded with fields)
    uint16_t    oldspecial;      //SoM: 3/6/2000: Remember if a sector was secret (for automap)
    uint16_t    tag;
//    int nexttag,firsttag;        //SoM: 3/6/2000: by killough: improves searches for tags.
    int32_t     nexttag;         // linked list of sectors with that tag, improves searches for tags.

    // 0 = untraversed, 1,2 = sndlines -1
    byte        soundtraversed;
    byte        floortype;  // see floortype_e

    // thing that made a sound (or null)
    mobj_t    * soundtarget;

    // mapblock bounding box for height changes
    int         blockbox[4];

    // origin for any sounds played by the sector
    xyz_t       soundorg;

    // if == validcount, already checked
    int         validcount;

    // list of mobjs in sector
    mobj_t*     thinglist;

    //SoM: 3/6/2000: Start boom extra stuff
    // thinker_t for reversable actions
    // make thinkers on floors, ceilings, lighting, independent of one another
    void * floordata;
                     // ZMalloc PU_LEVSPEC, in EV_DoFloor
    void * ceilingdata;
    void * lightingdata;
  
    // lockout machinery for stairbuilding
    int stairlock;   // -2 on first locked -1 after thinker done 0 normally
    int prevsec;     // -1 or number of sector for previous step
    int nextsec;     // -1 or number of next step sector
  
    // floor and ceiling texture offsets
    fixed_t   floor_xoffs,   floor_yoffs;
    fixed_t ceiling_xoffs, ceiling_yoffs;

    // [WDJ] 4/20/2010  modelsec is model sector for special linedefs.
    // It will be valid when model != SM_normal.
    // Testing modelsec for water is invalid, it is also used for colormap.
    // Uses model and modelsec, instead of the PrBoom heightsec.
    int modelsec;    // other sector number, or -1 if no other sector
    sector_model_e  model;  // Boom or Legacy special sector  [WDJ] 11/14/2009
    
    // [WDJ] 3/2011, (killough 8/28/98 Friction as sector property).
    // friction=INT_MAX when unused
    fixed_t  friction;
    int movefactor;
  
    int floorlightsec, ceilinglightsec;
    int teamstartsec;

    int bottommap, midmap, topmap; // dynamic colormaps
        // -1 is invalid, valid 0..
  
    // list of mobjs that are at least partially in the sector
    // thinglist is a subset of touching_thinglist
    msecnode_t       *  touching_thinglist;  // phares 3/14/98  
                                    // nodes are ZMalloc PU_LEVEL, by P_GetSecnode
    //SoM: 3/6/2000: end stuff...

    // list of ptrs to lines that have this sector as a side
    int                 linecount;
    line_t           ** linelist;  // [linecount] size

    //SoM: 2/23/2000: Improved fake floor hack
    ffloor_t *          ffloors;    // 3D floor list
                                    // ZMalloc PU_LEVEL, in P_AddFakeFloor
    int  *              attached;   // list of control sectors (by secnum)
                                    // realloc in P_AddFakeFloor
                                    // [WDJ] 7/2010 deallocate in P_SetupLevel
    int                 numattached;
    ff_light_t *        lightlist;  // array of fake floor lights
                                    // ZMalloc PU_LEVEL, in R_Prep3DFloors
    int                 numlights;
    int                 validsort; //if == validsort already been sorted

    // SoM: 4/3/2000: per-sector colormaps!
    extracolormap_t*    extra_colormap;  // (ref) using colormap for this frame
         // selected from bottommap,midmap,topmap, from special linedefs

    // ----- for special tricks with HW renderer -----
    fixed_t             virtualFloorheight;
    fixed_t             virtualCeilingheight;
    linechain_t *       sectorLines;
    sector_t        **  stackList;

#ifdef SECTOR_FLAGS
    uint16_t            flags;  // SCF_xx, moved, added, pseudoSector, virtual, etc.
#else
    boolean             moved;  // floor was moved
    boolean             added;
    // special tricks
    boolean             pseudoSector;
    boolean             virtualCeiling;
    boolean             virtualFloor;
#endif

#ifdef SOLARIS
    // Until we get Z_MallocAlign sorted out, make this a float
    // so that we don't get alignment problems.
    float               lineoutLength;
#else
    double              lineoutLength;
#endif
    // ----- end special tricks -----
} sector_t;



//=========
// SideDef.

typedef struct
{
    // add this to the calculated texture column
    fixed_t     textureoffset;

    // add this to the calculated texture top
    fixed_t     rowoffset;

    // Texture indices.
    // We do not maintain names here.
    // 0= no-texture, will never have -1
    short       toptexture;
    short       bottomtexture;
    short       midtexture;

    //SoM: 3/6/2000: This is the special of the linedef this side belongs to.
    short       linedef_special;

    // Sector the SideDef is facing.
    sector_t  * sector;
} side_t;



//
// Move clipping aid for LineDefs.
//
typedef enum
{
    ST_HORIZONTAL,
    ST_VERTICAL,
    ST_POSITIVE,
    ST_NEGATIVE

} slopetype_t;


//=========
// LineDef

typedef struct line_s
{
    // Vertices, from v1 to v2.
    vertex_t*   v1;  // linedef start vertex
    vertex_t*   v2;  // linedef end vertex
       // side1 is right side when looking from v1 to v2  (start to end)

    // Precalculated v2 - v1 for side checking.
    fixed_t     dx;
    fixed_t     dy;

    // Animation related.
    uint16_t	flags;
        // [WDJ] flags should be unsigned, but binary gets larger??
        // test shows that unsigned costs 4 more bytes per (flag & ML_bit)
    short       special;  // special linedef code
    uint16_t    tag;	  // special affects sectors with same tag id

    // Visual appearance: SideDefs.
    uint16_t    sidenum[2]; //  sidenum[1] will be NULL_INDEX if one sided
    // [smite] TODO actually they should be side_t pointers...

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
    fixed_t     bbox[4];

    // To aid move clipping.
    slopetype_t slopetype;

    // Front and back sector.
    // Note: redundant? Can be retrieved from SideDefs.
    sector_t*   frontsector; // sidedef[0] sector (right side, required)
    sector_t*   backsector;  // sidedef[1] sector (left side, optional)

    // if == validcount, already checked
    int         validcount;

    // thinker_t for reversable actions
    void*       specialdata;

    // wallsplat_t list
    void*       splats;
    
    //SoM: 3/6/2000
    int translu_eff;       // translucency effect table, 0 == none 
                           // TRANSLU_med or (TRANSLU_ext + translu_store index)
//    int firsttag,nexttag;  // improves searches for tags.
    int32_t     nexttag;   // linked list of lines with that tag value, improves searches for tags.

//    int ecolormap;         // SoM: Used for 282 linedefs
} line_t;




//=========
// SubSector

//
// A SubSector.
// References a Sector or portion of a sector.  Is a convex polygon.
// When the original sector is not convex, the nodebuilder divides it into
// subsectors until it has convex polygons.
// Basically, this is a list of LineSegs,
//  indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
    sector_t*   sector;   // (ref) part of this sector, from segs->sector of firstline
    // numlines and firstline are from the subsectors lump (nodebuilder)
#ifdef DEEPSEA_EXTENDED_NODES
    // [MB] 2020-04-22: Changed to 32-Bit for extended nodes
    uint32_t  numlines;   // number of segs in this subsector
    uint32_t  firstline;  // index into segs lump (loaded from wad)
#else   
    // [WDJ] some wad may be large enough to overflow signed short.
    unsigned short  numlines;   // number of segs in this subsector
    unsigned short  firstline;  // index into segs lump (loaded from wad)
#endif
    // floorsplat_t list
    void*       splats;
    //Hurdler: added for optimized mlook in hw mode
    int         validcount; 
} subsector_t;


// SoM: 3/6/200
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
  sector_t   *  m_sector; // a sector containing this object
  mobj_t     *  m_thing;  // this object
  msecnode_t *  m_tprev;  // prev msecnode_t for this thing
  msecnode_t *  m_tnext;  // next msecnode_t for this thing
  msecnode_t *  m_sprev;  // prev msecnode_t for this sector
  msecnode_t *  m_snext;  // next msecnode_t for this sector
  boolean visited; // killough 4/4/98, 4/7/98: used in search algorithms
} msecnode_t;


//=========
// Corona and Dynamic lights

// Defined constants for altering corona and dynamic lights from Fragglescript.
typedef enum {
    LT_NOLIGHT = 0,

    LT_PLASMA, LT_PLASMAEXP,
    LT_ROCKET, LT_ROCKETEXP,
    LT_BFG, LT_BFGEXP,
    LT_BLUETALL, LT_GREENTALL, LT_REDTALL,
    LT_BLUESMALL, LT_GREENSMALL, LT_REDSMALL,
    LT_TECHLAMP, LT_TECHLAMP2,
    LT_COLUMN,
    LT_CANDLE,
    LT_CANDLEABRE,
    LT_REDBALL, LT_GREENBALL,
    LT_ROCKET2,
  // Heretic
    LT_FX03,
    LT_FX17,
    LT_FX00,
    LT_FX08,
    LT_FX04,
    LT_FX02,
    LT_WTRH,
    LT_SRTC,
    LT_CHDL,
    LT_KFR1,

    NUMLIGHTS
} sprite_light_ind_e;

// [WDJ] Sprite light sources bit defines.
// The _SPR names are defined in legacy.wad for use in fragglescript scripts.
typedef enum {
   // UNDEFINED_SPR = 0
  SPLGT_none     = 0x00, // actually just for testing

// Effect enables
   // CORONA_SPR = 1
  SPLGT_corona   = 0x01, // emit a corona
   // DYNLIGHT_SPR = 2
  SPLGT_dynamic  = 0x02, // emit dynamic lighting

// Type field, can create a light source
  SPLT_type_field = 0xF0,
  SPLT_unk      = 0x00, // phobiata, newmaps default, plain corona
  SPLT_rocket   = 0x10, // flicker
  SPLT_lamp     = 0x20,
  SPLT_fire     = 0x30, // slow flicker, torch
//SPLT_monster  = 0x40,
//SPLT_ammo     = 0x50,
//SPLT_bonus    = 0x60,
  SPLT_light    = 0xC0, // no fade
  SPLT_firefly  = 0xD0, // firefly flicker, un-synch
  SPLT_random   = 0xE0, // random LED, un-synch
  SPLT_pulse    = 0xF0, // slow pulsation, un-synch

// Standard Combinations
   // LIGHT_SPR = 3
  SPLGT_light    = (SPLGT_dynamic|SPLGT_corona),
   // ROCKET_SPR = 0x13
  SPLGT_rocket   = SPLT_rocket | (SPLGT_dynamic|SPLGT_corona),
} sprite_light_flags_e;

  
typedef enum {
//  SPLT_type_field = 0xF0  // working type setting
   SLI_type_set= 0x02,  // the type was set, probably by fragglescript
   SLI_corona_set= 0x04,// the corona was set, only by fragglescript
   SLI_changed = 0x08,  // the data was changed, probably by fragglescript
} sprite_light_impl_flags_e;


// Special sprite lighting. Optimized for Hardware, OpenGL.
typedef struct
{
    uint16_t  splgt_flags;   // sprite_light_e, used in hwr_light.c

    float   light_xoffset;  // unused
    float   light_yoffset;  // y offset to adjust corona's height

    RGBA_t  corona_color;   // color of the light for static lighting
    float   corona_radius;  // radius of the coronas

    RGBA_t  dynamic_color;  // color of the light for dynamic lighting
    float   dynamic_radius; // radius of the light ball
// implementation data, not in tables
    float   dynamic_sqrradius; // radius^2 of the light ball
    uint16_t  impl_flags;   // implementation flags, sprite_light_impl_flags_e
} spr_light_t;

extern spr_light_t  * corona_lsp;
extern float     corona_size;
extern byte      corona_alpha, corona_bright;

spr_light_t *  Sprite_Corona_Light_lsp( int sprnum, state_t * sprstate );
byte  Sprite_Corona_Light_fade( spr_light_t * lsp, float cz, int objid );

typedef struct lightmap_s  lightmap_t;
typedef struct lightmap_s 
{
    float               s[2], t[2];
    spr_light_t       * light;
    lightmap_t        * next;
} lightmap_t;

//=========

#ifdef HWRENDER
# include "hardware/hw_poly.h"
#endif

//=========
// BSP traverse

//
// The LineSeg.
//
typedef struct
{
    // v1, v2, side, angle, offset, linedef are from wad segs lump
    vertex_t*   v1;  // start vertex  (derived from vertex index in wad)
    vertex_t*   v2;  // end vertex
       // side1 is right side when looking from v1 to v2  (start to end)

    int         side;
        // 0= seg is on right side of linedef
        // 1= seg is on left side of linedef (seg direction is opposite linedef)

    fixed_t     offset;
        // offset from linedef start or end, to segment vertex v1
        // when side=0, is offset from start of linedef to start of seg
        // when side=1, is offset from end of linedef to start of seg

    angle_t     angle;	// Binary Angle wad angle converted
        // EAST  = 0x00000000
        // NORTH = 0x40000000
        // WEST  = 0x80000000
        // SOUTH = 0xC0000000

    line_t*     linedef;  // (derived from linedef index in wad)
    side_t*     sidedef;  // segment sidedef (derived from linedef and side)

    // Sector references.
    // Could be retrieved from linedef, too.
    // backsector is NULL for one sided lines
    // (dervived from linedef and side)
    sector_t*   frontsector;  // sidedef sector, the segment sector/subsector, required
    sector_t*   backsector;   // side of linedef away from sector, optional

#ifdef HWRENDER   
    polyvertex_t  *pv1, *pv2; // float polygon vertex
    // length of the seg : used by the hardware renderer
    float       length;

    //Hurdler: 04/12/2000: added for static lightmap
    // hardware renderer
    lightmap_t  *lightmaps;
#endif

    // SoM: Why slow things down by calculating lightlists for every
    // thick side.
    int               numlights;
    r_lightlist_t*    rlights;
} seg_t;



#ifdef DEEPSEA_EXTENDED_NODES
    // [MB] 2020-04-22: Changed to 32-Bit for extended nodes
// [WDJ] BSP code has been examined and made unsigned safe.
// Do not want to go back to signed, would rather fix the new code.
// Other DoomLegacy code has already changed to use 0xFFFFFFFF instead of -1.
// #define DEEPSEA_EXTENDED_SIGNED_NODES
# ifdef DEEPSEA_EXTENDED_SIGNED_NODES
    //      Use int to match rest of the code (should be uint32_t)
    typedef int  bsp_child_t;
# else
    // The logic only used one neg value, and all rest are unsigned indexes.
    // This could be uint16_t without changing the code.
    typedef uint32_t  bsp_child_t;
# endif
# define NF_SUBSECTOR    0x80000000
#else
    // Normal BSP child.
    typedef uint16_t  bsp_child_t;
# define NF_SUBSECTOR    0x8000
#endif


//
// BSP node.
//
typedef struct
{
    // Partition line from (x,y) to x+dx,y+dy)
    fixed_t     x, y;
    fixed_t     dx, dy;

    // [Arcade] Partition direction re-aligned to the linedef the partition
    // seg lies on, for the render traversal only (P_Fix_Node_Partitions).
    // Equal to dx,dy unless the NODES lump rounded the direction.
    // x,y,dx,dy stay exactly as the WAD has them, so gameplay is unchanged.
    fixed_t     rdx, rdy;

    // Bounding box for each child.
    fixed_t     bbox[2][4];
        // bbox[0]= right child, all segs of right must be within the box
        // bbox[1]= left child, all segs of left must be within the box

    // If NF_SUBSECTOR is set then rest of it is a subsector index,
    // otherwise it is another node index.
    bsp_child_t  children[2];
        // children[0]= right
        // children[1]= left
} node_t;



//=========
// OTHER TYPES
//


#ifndef MAXFFLOORS
#define MAXFFLOORS    40
#endif

typedef struct visplane_s  visplane_t;
typedef struct vissprite_s  vissprite_t;
typedef struct drawseg_s  drawseg_t;
typedef struct drawsprite_s  drawsprite_t;

//#define DEBUG_DRAWSPRITE

typedef enum {
  DSO_none,
  DSO_before_drawseg, // this drawseg
  DSO_ffloor_area,  // order_id = none
  DSO_after_ffloor,  // order_id = ffloor index
  DSO_before_ffloor,  // order_id = ffloor index
  DSO_after_sprite,  // order_id = ??
  DSO_before_sprite,  // order_id = ??
} drawsprite_order_e;

typedef struct drawsprite_s {
  // because in mingw32, near is modifier of ptrs
  drawsprite_t * nearer, * prev;
  vissprite_t  * sprite;
  int16_t  x1, x2;  // partial sprite draws
  byte  order_type;  // drawsprite_order_e
  byte  order_id;    // which ffloor or sprite
#ifdef DEBUG_DRAWSPRITE
  mobjtype_t  type;   // MT_*  (MT_PLAYER, MT_VILE, MT_BFG, etc.)
#endif
} drawsprite_t;


typedef struct draw_ffside_s {
  uint16_t      numthicksides; // no max
  int16_t    *  thicksidecol;  // ref
  ffloor_t   *  thicksides[];  // ref
     // variable size array
} draw_ffside_t;

typedef struct draw_ffplane_s {
  byte          numffloorplanes;  // MAXFLOORS = 40
  byte          pad1;             // better align
  // z check for sprite clipping
  fixed_t       nearest_scale;  // the nearest scale of the planes
  fixed_t    *  backscale_r;  // ref to array [0..vid.width]
  visplane_t *  ffloorplanes[];  // ref to plane
     // variable size array
} draw_ffplane_t;


//
// Drawseg for floor and 3D floor thickseg
//
typedef struct drawseg_s
{
    int           x1, x2;  // x1..x2

    fixed_t       scale1, scale2;  // scale x1..x2
    fixed_t       scalestep;
    fixed_t       near_scale;   // nearest scale

    // silhouette is where a drawseg can overlap a sprite
    byte          silhouette;	      // bit flags, Silhouette_b
    fixed_t       sil_top_height;     // do not clip sprites below this
    fixed_t       sil_bottom_height;  // do not clip sprites above this

    // Pointers to lists for sprite clipping,
    //  all three adjusted so [x1] is first value.
    int16_t    *  spr_topclip;     // from opening pool array [x1..x2]
    int16_t    *  spr_bottomclip;  // from opening pool array [x1..x2]
    int16_t    *  maskedtexturecol;  // ref to array [x1..x2]

    seg_t      *  curline;
    drawseg_t  *  sector_drawseg;

    // 3D floors, only use what is needed, often none
    draw_ffplane_t  * draw_ffplane;
    draw_ffside_t   * draw_ffside;
} drawseg_t;


//=========
// Patch formats
// TM_patch

// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures.
// We compose textures from the TEXTURE1 and TEXTURE2 lists
// of patches.
//
//WARNING: this structure is cloned in GlidePatch_t
// Derived from:   pat_hdr_t;
// [WDJ] This is used for reading patches from wad.
typedef struct
{
    // pat_hdr_t
    uint16_t            width;          // bounding box size
    uint16_t            height;
    int16_t             leftoffset;     // draw image at pixels to the left of origin
    int16_t             topoffset;      // draw image at pixels above the origin
    // patch fields
    uint32_t            columnofs[8];   // actually [width]
       // offset of each column from start of patch header
    // This is used as the head of a patch, and columnofs[8] provides
    // access to an array that is usually [64], [128], or [256].
    // This would not work if the [8] was actually enforced.
} patch_t;

// The column data starts at offset read from columnofs[col].
// Patch column is series of posts, terminated by 0xFF.
// The posts are drawn, the space between posts is transparent.

// Posts are runs of non-masked source pixels, drawn at the topdelta.
// Post format: (header post_t), 0, (pixels bytes[length]), 0
typedef struct
{
    // unsigned test reads (topdelta = 0xFF) at column termination (which is not valid post_t)
    byte                topdelta; 	// y offset within patch of this post
    byte                length;         // length data bytes follows
} post_t;

// Doom posts have a 0 pad before and after the post data.
// Example of Doom column data:
//  (header post_t), 0, (pixels bytes[length]), 0,
//  (header post_t), 0, (pixels bytes[length]), 0,
//  (header post_t), 0, (pixels bytes[length]), 0,
//  0xFF

// column_t is a list of 0 or more post_t, (0xFF) terminated
typedef post_t  column_t;

//=========
// Picture format
// TM_picture
// Has columnofs table, no column posts, columns are solid.
// Used to draw walls and sky, solid (no transparency).
// Draws can be tiled.

// header
typedef struct
{
    uint32_t            columnofs[8];   // actually [width]
       // offset of each column from start of picture header
} picture_t;
// followed by TM_column_image

// Example of Doom TM_picture column data:
//  (pixels bytes[length])


//=========
// This is the Doom PIC format (not the Pictor PIC format).
typedef enum {
    PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
    INTENSITY       = 1,  // 1 byte intensity
    INTENSITY_ALPHA = 2,  // 2 byte : alpha then intensity
    RGB24           = 3,  // 24 bit rgb
    RGBA32          = 4,  // 32 bit rgba
} pic_mode_t;

// a pic is an unmasked block of pixels, stored in horizontal way
//
typedef struct
{
    uint16_t  width;
    byte      zero;   // set to 0 allow autodetection of pic_t 
                      // mode instead of patch or raw
    byte      mode;   // see pic_mode_t above
    uint16_t  height;
    uint16_t  reserved1;  // set to 0
    byte      data[0];  // pixels  (rows x columns), TM_row_image
} pic_t;

//=========
// Sprite

// cut bit flags
typedef enum
{
  SC_NONE = 0,
  SC_TOP = 1,
  SC_BOTTOM = 2
} spritecut_b;


// A vissprite_t is a thing
//  that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
typedef struct vissprite_s  vissprite_t;
typedef struct vissprite_s
{
    // Doubly linked list.
    vissprite_t       * prev;
    vissprite_t       * next;

    // Screen x range.
    int                 x0;      // x for texture alignment
    int                 x1, x2;  // x1, x2 clipped draw range

    // global bottom / top for silhouette clipping, world coordinates
    fixed_t             gz_bot;
    fixed_t             gz_top;

    fixed_t             scale;  // distance scale

    // texture
    fixed_t             tex_x0;   // texture x at vis x0, fractional
    fixed_t             tex_x_iscale;  // x texture step per pixel
        // negative if flipped

    fixed_t             texturemid;
    lumpnum_t           patch_lumpnum;

    // for color translation and shadow draw,
    //  maxbright frames as well
    lighttable_t*       colormap;

    //Fab:29-04-98: for MF_SHADOW sprites, which translucency table to use
    byte*               translucentmap;
    byte                translucent_index;
   
    byte                dist_priority;  // 0..255, when too many sprites   

    // SoM: 3/6/2000: sector for underwater/fake ceiling support
    int                 modelsec;   // -1 none

    //SoM: 4/3/2000: Global colormaps!
    extracolormap_t*    extra_colormap;
    fixed_t             xscale;
#ifdef MONSTER_VARY
    fixed_t             yscale;  // modified by size
#endif

    sector_t          * sector; // The sector containing the thing.
    mobj_t            * mobj;   // The thing

    // From mobj
    uint32_t            mobj_flags;  // mobjflag_e, MF_, modified
    // for line side calculation
    fixed_t             mobj_x, mobj_y;
    // Physical bottom / top for sorting with 3D floors, world coordinates.
    fixed_t             mobj_top_z, mobj_bot_z;  // modified by cuts
    //The actual height of the thing (for 3D floors)
//    fixed_t             mobj_height;   // unused

    //SoM: Precalculated top and bottom screen coords for the sprite.
    // [WDJ] Only used in r_things.c, as cut, used for clip tests.
    // Do not really need cut, just set y_top, and y_bot properly.
    int                 y_top, y_bot;  // sz_top, sz_bot

    uint8_t             cut;  // spritecut_b

    // To clip sprite for drawseg conflict drawing.
    int16_t          *  clip_top;
    int16_t          *  clip_bot;
} vissprite_t;


typedef enum { SRP_NULL, SRP_1, SRP_8, SRP_16 }  sprite_rotation_pattern_e;

//
// A sprite is a patch_t that is assumed to represent a three dimensional
// object and may have multiple rotations pre-drawn.
// Sprites are patches with a special naming convention so they can be
// recognized by R_Init_Sprites.
// The base name is combined with frame (F) and rotation (x) characters.
// A sprite with only one patch for all views: NNNNF0.
// With rotation it is named NNNNFx or NNNNFxFx, with
//   x = 1 .. 8, indicating the rotation.
// The second frame uses Horizontal flipping (to save space),
//  thus NNNNF2F5 defines a mirrored patch with F2 normal and F5 flipped.
// The sprite and frame specified by a thing_t
//  is range checked at run time.

// [WDJ] Tried to get better packing.
// Having two levels of structure requires two levels of decoding, and
// even more cache wasted due to field alignments.
// This arrangement eliminates most conditional tests.
// It allows storing 1-rotation sprites in a smaller allocation.
// There may be enhancements later, so optimization of this structure
// will be a waste of effort.  Trying to put the rotation_pattern in
// the structure just results in an irregular structure.


// Per frame rotation
// size 8
typedef struct
{
    lumpnum_t   pat_lumpnum;   // lump number of patch
    uint16_t    spritelump_id; // into spritelumps[]
    byte        flip;    // Flip bit (1 = flip)
} sprite_frot_t;

// Per frame, often has multiple rotations.
typedef struct
{
    byte        rotation_pattern;  // sprite_rotation_pattern_e
} spriteframe_t;


//
// A sprite definition:  a number of animation frames.
//
typedef struct
{
    byte                numframes; // MAX_FRAMES are 29
    byte                frame_rot; // array indexing step
    spriteframe_t *     spriteframe;  // array[numframes]
    sprite_frot_t *     framerotation;  // array[numframes * frame_rot]
} spritedef_t;

spriteframe_t *  get_spriteframe( const spritedef_t * spritedef, unsigned int frame_num );
sprite_frot_t *  get_framerotation( const spritedef_t * spritedef,
                                    unsigned int frame_num, byte rotation );

extern const byte srp_to_num_rot[4];

#endif
