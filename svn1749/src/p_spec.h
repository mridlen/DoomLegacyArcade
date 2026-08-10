// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_spec.h 1736 2025-03-14 08:15:57Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2012 by DooM Legacy Team.
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
// $Log: p_spec.h,v $
// Revision 1.16  2003/07/23 17:26:36  darkwolf95
// SetLineTexture function for Fraggle Script
//
// Revision 1.15  2003/06/11 03:02:12  ssntails
// Vertical wind currents and friction capability on 3d floors
//
// Revision 1.14  2002/06/30 13:57:30  ssntails
// Added vertical water currents.
//
// Revision 1.13  2002/06/14 02:43:43  ssntails
// Instant-lower and instant-raise capability for sectors added.
//
// Revision 1.12  2001/03/21 18:24:38  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.11  2001/02/24 13:35:20  bpereira
// Revision 1.10  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.9  2000/11/11 13:59:46  bpereira
//
// Revision 1.8  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.7  2000/10/21 08:43:30  bpereira
// Revision 1.6  2000/04/16 18:38:07  bpereira
//
// Revision 1.5  2000/04/07 18:48:56  hurdler
// AnyKey doesn't seem to compile under Linux, now renamed to AnyKey_
//
// Revision 1.4  2000/04/06 20:54:28  hurdler
// Mostly remove warnings under windows
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
//
//-----------------------------------------------------------------------------

#ifndef P_SPEC_H
#define P_SPEC_H

#include "doomtype.h"
#include "p_mobj.h"
  // mobj_t
#include "r_defs.h"
  // line_t, sector_t
#include "d_player.h"
  // player_t

//      Define values for map objects
#define MO_TELEPORTMAN          14

#define SAVE_VERSION_144


extern consvar_t  cv_zerotags;

// at game start
void    P_Release_PicAnims(void);
void    P_Init_PicAnims (void);

// at map load (sectors)
void    P_Setup_LevelFlatAnims (void);

// at map load
void    P_SpawnSpecials (void);

// Called to update fogwater special flags after changing config
void    P_Config_FW_Specials (void);

// every tic
void    P_UpdateSpecials (void);

// when needed
boolean P_UseSpecialLine ( mobj_t* thing, line_t* line, int side );

void    P_ShootSpecialLine ( mobj_t* thing, line_t* line );

void    P_CrossSpecialLine ( line_t* line, int side, mobj_t* thing );

void    P_PlayerInSpecialSector (player_t* player);

int     twoSided ( int sector, int line );

sector_t*  getSector ( int currentSector, int line, int side );

side_t*  getSide ( int currentSector, int line, int side );

fixed_t  P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t  P_FindHighestFloorSurrounding(sector_t* sec);

fixed_t  P_FindNextHighestFloor ( sector_t* sec, int currentheight );

//SoM: 3/6/2000
fixed_t  P_FindNextLowestFloor ( sector_t* sec, int currentheight );

fixed_t  P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t  P_FindHighestCeilingSurrounding(sector_t* sec);

int     P_FindSectorFromLineTag ( line_t* line, int start );

int     P_FindSectorFromTag ( uint16_t tag, int start );

//DarkWolf95:July 23, 2003: Needed for SF_SetLineTexture
int  P_FindLineFromTag( uint16_t tag, int start );

int  P_FindLineFromLineTag(const line_t *line, int start); //SoM: 3/16/2000

lightlev_t  P_FindMinSurroundingLight ( sector_t* sector, lightlev_t max );

sector_t*  getNextSector ( line_t* line, sector_t* sec );

//SoM: 3/6/2000
sector_t*  P_FindModelFloorSector ( fixed_t floordestheight, int secnum );

//SoM: 3/15/2000
fixed_t   P_FindNextHighestCeiling(sector_t *sec, int currentheight);
fixed_t   P_FindNextLowestCeiling(sector_t *sec, int currentheight);
fixed_t   P_FindShortestUpperAround(int secnum);
fixed_t   P_FindShortestTextureAround(int secnum);
sector_t* P_FindModelCeilingSector(fixed_t ceildestheight,int secnum);
boolean   P_CanUnlockGenDoor( line_t* line, player_t* player);
int  P_CheckTag(line_t * line);

//
// SPECIAL
//
int EV_DoDonut(line_t* line);


#ifdef SAVE_VERSION_144

// [smite] NOTE: For the code in p_saveg.c to work, all the sector effects
// must have a thinker and a sector_t* as their first data fields.

//
// P_LIGHTS
//
typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;	  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (count ... )
    int         count;  
    int         minlight, maxlight;

} fireflicker_144_t;



typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (count ... )
    int         count;
    int         minlight, maxlight;
    int         maxtime;
    int         mintime;

} lightflash_144_t;



typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (count ... )
    int         count;
    int         minlight, maxlight;
    int         darktime;
    int         brighttime;

} strobe_144_t;




typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (minlight ... )
    int         minlight, maxlight;
    int         direction;    // 1 = up, -1 = down

} glow_144_t;

//SoM: thinker struct for fading lights. ToDo: Add effects for light
// transition
typedef struct
{
  thinker_t thinker;  // must be first for ptr conversion
  sector_t *sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (destlevel ... )
  int destlevel;
  int speed;

} lightfader_144_t;

#endif


// [smite] NOTE: For the code in p_saveg.c to work, all the sector effects
// must have a thinker and a sector_t* as their first data fields.

//
// P_LIGHTS
//
typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;	  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (minlight ... )
    lightlev_t  minlight, maxlight;
    int32_t     count;  

} fireflicker_t;



typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (minlight ... )
    lightlev_t  minlight, maxlight;
    int32_t     mintime, maxtime;
    int32_t     count;

} lightflash_t;



typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (minlight ... )
    lightlev_t  minlight, maxlight;
    int32_t     darktime;
    int32_t     brighttime;
    int32_t     count;

} strobe_t;




typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (minlight ... )
    lightlev_t  minlight, maxlight;
    int8_t      direction;    // 1 = up, -1 = down

} glow_t;

//SoM: thinker struct for fading lights. ToDo: Add effects for light
// transition
typedef struct
{
  thinker_t thinker;  // must be first for ptr conversion
  sector_t *sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (destlight ... )
  lightlev_t  destlight;
  lightlev_t  speed;

} lightfader_t;



#define GLOWSPEED               8
#define STROBEBRIGHT            5
#define FASTDARK                15
#define SLOWDARK                35

void   T_FireFlicker (fireflicker_t* flick);
void   P_SpawnFireFlicker (sector_t* sector);
void   T_LightFlash (lightflash_t* flash);
void   P_SpawnLightFlash (sector_t* sector);
void   T_StrobeFlash (strobe_t* flash);

void   P_SpawnStrobeFlash ( sector_t* sector, int fastOrSlow, int inSync );

int    EV_StartLightStrobing(line_t* line);
int    EV_TurnTagLightsOff(line_t* line);

int    EV_LightTurnOn ( line_t* line, lightlev_t bright );
int    EV_LightTurnOnPartway(line_t *line, fixed_t level);

void   T_Glow(glow_t* g);
void   P_SpawnGlowingLight(sector_t* sector);


void   P_FadeLight( uint16_t tag, lightlev_t destvalue, lightlev_t speed );
void   T_LightFade(lightfader_t * lf);



//
// P_SWITCH
//
#pragma pack(1) //Hurdler: 04/04/2000: I think pragma is more portable
typedef struct
{
    char        name1[9];
    char        name2[9];
    short       episode;
} switchlist_t; 
//} __attribute__ ((packed)) switchlist_t; //SoM: 3/22/2000: Packed to read from memory.
#pragma pack()


// internal
typedef enum
{
    B_top_texture,
    B_middle_texture,
    B_bottom_texture
} bwhere_e;


typedef struct
{
    line_t    * line;
    xyz_t     * soundorg;
    bwhere_e    where;
    int         btexture;
    int         btimer;
} button_t;



 // max # of wall switches in a level
#define MAXSWITCHES             50

 // 4 players, 4 buttons each at once, max.
 // added 19-1-98 16->MAXPLAYERS*4
#define MAXBUTTONS           (MAXPLAYERS*4) //16

 // 1 second, in ticks.
#define BUTTONTIME      35

extern button_t buttonlist[MAXBUTTONS];

void P_ChangeSwitchTexture ( line_t* line, int useAgain );

void P_Init_SwitchList(void);

// SoM: 3/4/2000: Misc Boom stuff for thinkers that can share sectors, and some other stuff

// internal
typedef enum
{
  S_floor_special,
  S_ceiling_special,
  S_lighting_special,
} sector_special_e;


//SoM: 3/6/2000
boolean P_SectorActive ( sector_special_e spt, sector_t* sec );

// internal
typedef enum
{
  CH_MODEL_trig_only,
  CH_MODEL_num_only,
} change_e;


//
// P_PLATS
// internal, savegame
typedef enum
{
  PLATS_up,
  PLATS_down,
  PLATS_waiting,
  PLATS_in_stasis
} platstat_e;


// internal, savegame
typedef enum
{
  PLATT_perpetualRaise,
  PLATT_downWaitUpStay,
  PLATT_raiseAndChange,
  PLATT_raiseToNearestAndChange,
  PLATT_blazeDWUS,
  //SoM:3/4/2000: Added boom stuffs
  PLATT_genLift,      //General stuff
  PLATT_genPerpetual, 
  PLATT_toggleUpDn,   //Instant toggle of stuff.
} plattype_e;




typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;	// saved
    struct platlist *list; //SoM: 3/6/2000: Boom's improved code without limits.
       // list is not saved, generated by linking plats
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    plattype_e  type;
    fixed_t     speed;
    fixed_t     low, high;	// floor heights
    boolean     crush;		// enables crushing damage
    uint16_t    tag;
    int         wait;
    int         count;	
    platstat_e  status, oldstatus;  // up, down, in_statis etc.
} plat_t;

#ifdef SAVE_VERSION_144
typedef struct
{
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    plattype_e  type;
    fixed_t     speed;
    fixed_t     low, high;	// floor heights
    boolean     crush;		// enables crushing damage
    int         tag;
    int         wait;
    int         count;	
    platstat_e  status, oldstatus;  // up, down, in_statis etc.
} plat_144_t;
#endif

//SoM: 3/6/2000: Boom's improved code without limits.
typedef struct platlist  platlist_t;
typedef struct platlist {
  plat_t *plat; 
  platlist_t *next, **prev;
} platlist_t;

void   P_Remove_AllActivePlats(void); //SoM: 3/9/2000

#define PLATWAIT                3
#define PLATSPEED               (FRACUNIT/NEWTICRATERATIO)
#define MAXPLATS                30


extern platlist_t  *activeplats;

void    T_PlatRaise(plat_t* plat);

int     EV_DoPlat ( line_t* line, plattype_e type, int amount );

void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
int     EV_StopPlat(line_t* line);
void    P_ActivateInStasis(uint16_t tag);


//
// P_DOORS
// internal, savegame
typedef enum
{
  VD_normalDoor,
  VD_close30ThenOpen,
  VD_doorclose,
  VD_dooropen,
  VD_raiseIn5Mins,
  VD_blazeRaise,
  VD_blazeOpen,
  VD_blazeClose,

  //SoM: 3/4/2000: General door types...
  VD_genRaise,
  VD_genBlazeRaise,
  VD_genOpen,
  VD_genBlazeOpen,
  VD_genClose,
  VD_genBlazeClose,
  VD_genCdO,
  VD_genBlazeCdO,
} vldoor_e;



typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
    //SoM: 3/6/2000: the line that triggered the door.
    line_t *    line;	 // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    vldoor_e    type;
    fixed_t     topheight;
    fixed_t     speed;

    // 1 = up, 0 = waiting at top, -1 = down
    int         direction;

    // tics to wait at the top
    int         topwait;
    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
    int         topcountdown;

    // killough 10/98: sector tag for gradual lighting effects.
    uint16_t     lighttag;
} vldoor_t;


#ifdef SAVE_VERSION_144
typedef struct
{
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    vldoor_e    type;
    fixed_t     topheight;
    fixed_t     speed;
    int         direction;
    int         topwait;
    int         topcountdown;
} vldoor_144_t;
#endif



#define VDOORSPEED              (FRACUNIT*2/NEWTICRATERATIO)
#define VDOORWAIT               150

//SoM: 3/6/2000: boom support
int   EV_VerticalDoor ( line_t* line, mobj_t* thing );

int   EV_DoDoor ( line_t* line, vldoor_e type, fixed_t speed);

void  EV_OpenDoor( uint16_t sectag, int speed, int wait_time );
void  EV_CloseDoor( uint16_t sectag, int speed );

int   EV_DoLockedDoor ( line_t* line, vldoor_e type, mobj_t* thing,
                        fixed_t speed );

void  T_VerticalDoor (vldoor_t* door);
void  P_SpawnDoorCloseIn30 (sector_t* sec);

void  P_SpawnDoorRaiseIn5Mins ( sector_t* sec, int secnum );



#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum
{
  SDS_opening,
  SDS_waiting,
  SDS_closing
} sdstat_e;



typedef enum
{
  SDT_openOnly,
  SDT_closeOnly,
  SDT_openAndClose
} sdtype_e;



// Savegame does not have slidedoor_t saving code yet !!
typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    line_t*     line;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    sdtype_e    type;
    int         frame;
    int         whichDoorIndex;
    int         timer;
    sector_t*   frontsector;
    sector_t*   backsector;
    sdstat_e    status;

} slidedoor_t;



typedef struct
{
    char        frontFrame1[9];
    char        frontFrame2[9];
    char        frontFrame3[9];
    char        frontFrame4[9];
    char        backFrame1[9];
    char        backFrame2[9];
    char        backFrame3[9];
    char        backFrame4[9];

} slidename_t;



typedef struct
{
    int             frontFrames[4];
    int             backFrames[4];

} slideframe_t;



// how many frames of animation
#define SNUMFRAMES              4

#define SDOORWAIT               (35*3)
#define SWAITTICS               4

// how many diff. types of anims
#define MAXSLIDEDOORS   5

void  P_Init_SlidingDoorFrames(void);

void  EV_SlidingDoor ( line_t* line, mobj_t* thing );

#endif  // Sliding doors



//
// P_CEILNG
// internal, savegame
typedef enum
{
  CT_lowerToFloor,
  CT_raiseToHighest,
  //SoM:3/4/2000: Extra boom stuffs that tricked me...
  CT_lowerToLowest,
  CT_lowerToMaxFloor,

  CT_lowerAndCrush,
  CT_crushAndRaise,
  CT_fastCrushAndRaise,
  CT_silentCrushAndRaise,
  CT_instantRaise, // Insantly raises SSNTails 06-13-2002

  //SoM:3/4/2000
  //jff 02/04/98 add types for generalized ceiling mover
  CT_genCeiling,
  CT_genCeilingChg,
  CT_genCeilingChg0,
  CT_genCeilingChgT,

  //jff 02/05/98 add types for generalized ceiling mover
  CT_genCrusher,
  CT_genSilentCrusher,
} ceiling_e;


typedef struct ceilinglist_s   ceilinglist_t;

typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
    // SoM: 3/6/2000: by jff: copied from killough's plats
    ceilinglist_t * list;
       // list is not saved, generated by linking ceilings

 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    ceiling_e   type;
    fixed_t     bottomheight;
    fixed_t     topheight;
    fixed_t     speed, oldspeed; //SoM: 3/6/2000
    boolean     crush;
    // 1 = up, 0 = waiting, -1 = down
    int8_t      direction, olddirection;
   
    // ID
    uint16_t    tag;
   
    //SoM: 3/6/2000: Support ceiling changers
    short       new_sec_special, old_sec_special;  // 0 or sector->special
    short       new_ceilingpic;   // change ceiling flat when done
                                  // Boom and Heretic: called texture
} ceiling_t;

//SoM: 3/6/2000: Boom's improved ceiling list.
typedef struct ceilinglist_s {
  ceiling_t *ceiling; 
  ceilinglist_t *next, **prev;
} ceilinglist_t;


void P_Remove_AllActiveCeilings(void); //SoM: 3/9/2000


#define CEILSPEED               (FRACUNIT/NEWTICRATERATIO)
#define CEILWAIT                150
#define MAXCEILINGS             30

extern ceilinglist_t  *activeceilings;  //SoM: 3/6/2000: New improved boom code.

int     EV_DoCeiling ( line_t* line, ceiling_e type );

void    T_MoveCeiling (ceiling_t* ceiling);
void    P_AddActiveCeiling(ceiling_t* ceiling);
void    P_RemoveActiveCeiling(ceiling_t* ceiling);
int     EV_CeilingCrushStop(line_t* line);
int     P_ActivateInStasisCeiling(line_t* line);


//
// P_FLOOR
// internal, savegame
typedef enum
{
    // lower floor to highest surrounding floor
  FT_lowerFloor,

    // lower floor to lowest surrounding floor
  FT_lowerFloorToLowest,

    // lower floor to highest surrounding floor VERY FAST
  FT_turboLower,

    // raise floor to lowest surrounding CEILING
  FT_raiseFloor,

    // raise floor to next highest surrounding floor
  FT_raiseFloorToNearest,

    // lower floor to lowest surrounding floor
  FT_lowerFloorToNearest,

    // lower floor 24
  FT_lowerFloor24,

    // lower floor 32
  FT_lowerFloor32Turbo,

    // raise floor to shortest height texture around it
  FT_raiseToTexture,

    // lower floor to lowest surrounding floor
    //  and change floorpic
  FT_lowerAndChange,

  FT_raiseFloor24,

    //raise floor 32
  FT_raiseFloor32Turbo,

  FT_raiseFloor24AndChange,
  FT_raiseFloorCrush,

     // raise to next highest floor, turbo-speed
  FT_raiseFloorTurbo,
  FT_donutRaise,
  FT_raiseFloor512,
  FT_instantLower, // Instantly lowers SSNTails 06-13-2002

    //SoM: 3/4/2000 Boom copy YEAH YEAH
  FT_genFloor,
  FT_genFloorChg,
  FT_genFloorChg0,
  FT_genFloorChgT,

    //new types for stair builders
  FT_buildStair,
  FT_genBuildStair,

} floor_e;

//SoM:3/4/2000: Anothe boom code copy.
// internal, savegame
typedef enum
{
  ET_elevateUp,
  ET_elevateDown,
  ET_elevateCurrent,
} elevator_e;


// internal, savegame
typedef enum
{
  ST_build8,     // slowly build by 8
  ST_turbo16     // quickly build by 16
} stair_e;



typedef struct
{
    thinker_t   thinker;  // must be first for ptr conversion
    sector_t*   sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
    floor_e     type;
    boolean     crush;
    fixed_t     floordestheight;
    fixed_t     speed;
    int8_t      direction;    // 1 = up, 0 = waiting, -1 = down
    //SoM: 3/6/2000
    short       new_sec_special, old_sec_special;  // from sector special
    short       new_floorpic;    // change floor when done
                                 // Heretic: called texture
    short       new_floortype;   // Boom did not have texture sounds.
} floormove_t;


typedef struct //SoM: 3/6/2000: Elevator struct.
{
  thinker_t thinker;  // must be first for ptr conversion
  sector_t* sector;  // saved
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
  elevator_e type;
  int8_t  direction;    // 1 = up, 0 = waiting, -1 = down
  fixed_t floordestheight;
  fixed_t ceilingdestheight;
  fixed_t speed;
} elevator_t;


#define ELEVATORSPEED (FRACUNIT*4/NEWTICRATERATIO) //SoM: 3/6/2000
#define FLOORSPEED    (FRACUNIT/NEWTICRATERATIO)

typedef enum
{
  MP_ok,
  MP_crushed,
  MP_pastdest
} result_e;

result_e  T_MovePlane ( sector_t*     sector,
                        fixed_t       speed,
                        fixed_t       dest,
                        boolean       crush,
                        int           floorOrCeiling,
                        int           direction );

int   EV_BuildStairs ( line_t* line, stair_e type );

int   EV_DoFloor ( line_t* line, floor_e floortype );

int   EV_DoChange ( line_t* line, change_e changetype ); //SoM: 3/16/2000

int   EV_DoElevator ( line_t* line, elevator_e elevtype ); //SoM: 3/16/2000

void  T_MoveFloor( floormove_t* floor);

//SoM: New thinker functions.
void  T_MoveElevator(elevator_t* elevator);

//
// P_TELEPT
//
int  EV_Teleport ( line_t* line, int side, mobj_t* thing );

//SoM: 3/15/2000: Boom silent teleport functions
int  EV_SilentTeleport(line_t *line, int side, mobj_t *thing);
int  EV_SilentLineTeleport(line_t *line, int side, mobj_t *thing, boolean reverse);
int  EV_PortalTeleport(line_t*  line, mobj_t* thing, int side);




/* SoM: 3/4/2000: This is a large section of copied code. Sorry if this offends people, but
   I really don't want to read, understand and rewrite all the changes to the source and entire
   team made! Anyway, this is for the generalized linedef types. */

//jff 3/14/98 add bits and shifts for generalized sector types

#define DAMAGE_MASK     0x60
#define DAMAGE_SHIFT    5
#define SECRET_MASK     0x80
#define SECRET_SHIFT    7
#define FRICTION_MASK   0x100
#define FRICTION_SHIFT  8
#define PUSH_MASK       0x200
#define PUSH_SHIFT      9

#ifdef MBF21
// Instant death sector special.
// MBF21, bit 12, alternate damage code in DAMAGE
#define MBF21_PLAYER_DEATH_SECTOR   0x1000
// MBF21, bit 13
#define MBF21_MONSTER_DEATH_SECTOR  0x2000
#endif

//jff 02/04/98 Define masks, shifts, for fields in 
// generalized linedef types

#define GenFloorBase          0x6000
#define GenCeilingBase        0x4000
#define GenDoorBase           0x3c00
#define GenLockedBase         0x3800
#define GenLiftBase           0x3400
#define GenStairsBase         0x3000
#define GenCrusherBase        0x2F80

#define TriggerType           0x0007
#define TriggerTypeShift      0

// define masks and shifts for the floor type fields

#define FloorCrush            0x1000
#define FloorChange           0x0c00
#define FloorTarget           0x0380
#define FloorDirection        0x0040
#define FloorModel            0x0020
#define FloorSpeed            0x0018

#define FloorCrushShift           12
#define FloorChangeShift          10
#define FloorTargetShift           7
#define FloorDirectionShift        6
#define FloorModelShift            5
#define FloorSpeedShift            3
                               
// define masks and shifts for the ceiling type fields

#define CeilingCrush          0x1000
#define CeilingChange         0x0c00
#define CeilingTarget         0x0380
#define CeilingDirection      0x0040
#define CeilingModel          0x0020
#define CeilingSpeed          0x0018

#define CeilingCrushShift         12
#define CeilingChangeShift        10
#define CeilingTargetShift         7
#define CeilingDirectionShift      6
#define CeilingModelShift          5
#define CeilingSpeedShift          3

// define masks and shifts for the lift type fields

#define LiftTarget            0x0300
#define LiftDelay             0x00c0
#define LiftMonster           0x0020
#define LiftSpeed             0x0018

#define LiftTargetShift            8
#define LiftDelayShift             6
#define LiftMonsterShift           5
#define LiftSpeedShift             3

// define masks and shifts for the stairs type fields

#define StairIgnore           0x0200
#define StairDirection        0x0100
#define StairStep             0x00c0
#define StairMonster          0x0020
#define StairSpeed            0x0018

#define StairIgnoreShift           9
#define StairDirectionShift        8
#define StairStepShift             6
#define StairMonsterShift          5
#define StairSpeedShift            3

// define masks and shifts for the crusher type fields

#define CrusherSilent         0x0040
#define CrusherMonster        0x0020
#define CrusherSpeed          0x0018

#define CrusherSilentShift         6
#define CrusherMonsterShift        5
#define CrusherSpeedShift          3

// define masks and shifts for the door type fields

#define DoorDelay             0x0300
#define DoorMonster           0x0080
#define DoorKind              0x0060
#define DoorSpeed             0x0018

#define DoorDelayShift             8
#define DoorMonsterShift           7
#define DoorKindShift              5
#define DoorSpeedShift             3

// define masks and shifts for the locked door type fields

#define LockedNKeys           0x0200
#define LockedKey             0x01c0
#define LockedKind            0x0020
#define LockedSpeed           0x0018

#define LockedNKeysShift           9
#define LockedKeyShift             6
#define LockedKindShift            5
#define LockedSpeedShift           3

#ifdef MBF21
byte  P_MBF21_MonsterSpecialSector( mobj_t* mon, sector_t* sector );
#endif

//SoM: 3/9/2000: p_genlin

int  EV_DoGenFloor ( line_t* line );
int  EV_DoGenCeiling ( line_t* line );
int  EV_DoGenLift ( line_t* line );
int  EV_DoGenStairs ( line_t* line );
int  EV_DoGenCrusher ( line_t* line );
int  EV_DoGenDoor ( line_t* line );
int  EV_DoGenLockedDoor ( line_t* line );

// define names for the TriggerType field of the general linedefs
// Boom defined
typedef enum
{
  TRIG_WalkOnce,
  TRIG_WalkMany,
  TRIG_SwitchOnce,
  TRIG_SwitchMany,
  TRIG_GunOnce,
  TRIG_GunMany,
  TRIG_PushOnce,
  TRIG_PushMany,
} triggertype_e;

// define names for the Speed field of the general linedefs
// Boom defined
typedef enum
{
  SPEED_Slow,
  SPEED_Normal,
  SPEED_Fast,
  SPEED_Turbo,
} motionspeed_e;

// define names for the Target field of the general floor
// Boom defined
typedef enum
{
  FTAR_FtoHnF,
  FTAR_FtoLnF,
  FTAR_FtoNnF,
  FTAR_FtoLnC,
  FTAR_FtoC,
  FTAR_FbyST,
  FTAR_Fby24,
  FTAR_Fby32,
} floortarget_e;

// define names for the Changer Type field of the general floor
// Boom defined
typedef enum
{
  FCH_FNoChg,
  FCH_FChgZero,
  FCH_FChgTxt,
  FCH_FChgTyp,
} floorchange_e;

// define names for the Change Model field of the general floor
// define names for the Change Model field of the general ceiling
// Boom defined
typedef enum
{
  MODEL_Trigger,
  MODEL_Numeric,
} floorceil_model_t;

// define names for the Target field of the general ceiling
// Boom defined
typedef enum
{
  CTAR_CtoHnC,
  CTAR_CtoLnC,
  CTAR_CtoNnC,
  CTAR_CtoHnF,
  CTAR_CtoF,
  CTAR_CbyST,
  CTAR_Cby24,
  CTAR_Cby32,
} ceilingtarget_e;

// define names for the Changer Type field of the general ceiling
// Boom defined
typedef enum
{
  CCH_CNoChg,
  CCH_CChgZero,
  CCH_CChgTxt,
  CCH_CChgTyp,
} ceilingchange_e;

// define names for the Target field of the general lift
// Boom defined
typedef enum
{
  LTAR_F2LnF,
  LTAR_F2NnF,
  LTAR_F2LnC,
  LTAR_LnF2HnF,
} lifttarget_e;

// define names for the door Kind field of the general ceiling
// Boom defined
typedef enum
{
  DT_OdCDoor,
  DT_ODoor,
  DT_CdODoor,
  DT_CDoor,
} doorkind_e;

// define names for the locked door Kind field of the general ceiling
// Boom defined
typedef enum
{
  DKY_anykey,
  DKY_R_card,
  DKY_B_card,
  DKY_Y_card,
  DKY_R_skull,
  DKY_B_skull,
  DKY_Y_skull,
  DKY_allkeys,
} keykind_e;

/* SoM: End generalized linedef code */

// Boom defined
typedef enum
{
  SCROLL_side,
  SCROLL_floor,
  SCROLL_ceiling,
  SCROLL_carry,
  SCROLL_carry_ceiling,
} scrolltype_e;

//SoM: 3/8/2000: Add generalized scroller code
typedef struct {
  // Thinker structure for scrolling
  thinker_t thinker;  // must be first for ptr conversion
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
  scrolltype_e  type;
  int affectee;        // Number of affected sidedef, sector, tag, or whatever
  int control;         // Control sector (-1 if none) used to control scrolling
  int accel;           // Whether it's accelerative
  fixed_t dx, dy;      // (dx,dy) scroll speeds
  fixed_t last_height; // Last known height of control sector
  fixed_t vdx, vdy;    // Accumulated velocity if accelerative
} scroll_t;

void T_Scroll(scroll_t *s);


//SoM: 3/8/2000: added new model of friction for ice/sludge effects

typedef struct {
  // Thinker structure for friction
  thinker_t thinker;  // must be first for ptr conversion
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (affectee ... )
  int affectee;        // Number of affected sector
  int friction;        // friction value (E800 = normal)
  int movefactor;      // inertia factor when adding to momentum
} friction_t;

//SoM: Friction defines.
// original values
#define ORIG_FRICTION          0xE800
#define ORIG_FRICTION_FACTOR   2048
// Heretic
#define FRICTION_NORM          0xe800
#define FRICTION_LOW           0xf900
#define FRICTION_FLY           0xeb00

//SoM: 3/9/2000: Otherwise, the compiler whines!
void T_Friction(friction_t *f);

// Update sector fields after a change of special type.
void P_Update_Special_Sector( sector_t * sec, short new_special );

//SoM: 3/8/2000: Model for Pushers for push/pull effects

// Boom defined
typedef enum
{
  PP_push,
  PP_pull,	// [WDJ] not used, uses p_push for push and pull
  PP_wind,
  PP_current,
  PP_upcurrent, // SSNTails 06-10-2002
  PP_downcurrent, // SSNTails 06-10-2002
  PP_upwind, // SSNTails 06-10-2003 WOAH! EXACTLY ONE YEAR LATER! FREAKY!
  PP_downwind, // SSNTails 06-10-2003
} pushpull_type_e;

typedef struct {
  // Thinker structure for Pusher
  thinker_t thinker;  // must be first for ptr conversion
  mobj_t  * source;      // Point source if point pusher
                        // not saved, derived from affectee
 // State to be saved in save game (p_saveg.c)
 // Savegame saves fields (type ... )
  pushpull_type_e  type;
  int affectee;        // Number of affected sector
  int x_mag, y_mag;    // X Strength
  int magnitude;       // Vector strength for point pusher
  int radius;          // Effective radius for point pusher
  fixed_t  x_src, y_src;    // X,Y of point source if point pusher
} pusher_t;

//SoM: 3/9/2000: Prototype functions for pushers
boolean  PIT_PushThing(mobj_t* thing);
void     T_Pusher(pusher_t *p);
mobj_t*  P_GetPushThing(int s);


//SoM: 3/16/2000
void P_CalcHeight (player_t* player);


// heretic stuff
// heretic.c
void P_Init_Lava(void);
void P_Heretic_PlayerInSpecialSector(player_t * player, sector_t * sector);
// s_amb.c
void P_AmbientSound(void);
void P_AddAmbientSfx(int sequence);
void P_Init_AmbientSound(void);

#endif
