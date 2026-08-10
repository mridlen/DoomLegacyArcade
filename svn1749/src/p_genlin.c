// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_genlin.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
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
//
// DESCRIPTION:
//  Generalized linedef type handlers
//  Floors, Ceilings, Doors, Locked Doors, Lifts, Stairs, Crushers
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "r_data.h"
#include "g_game.h"
#include "s_sound.h"
#include "z_zone.h"
#include "m_random.h"

/*
  SoM: 3/9/2000: Copied this entire file from Boom sources to Legacy sources.
  This file contains all routines for Generalized linedef types.
*/

//
// EV_DoGenFloor()
//
// Handle generalized floor types
//
// Passed the line activating the generalized floor function
// Returns true if a thinker is created
//
int EV_DoGenFloor ( line_t* line )
{
  int                   secnum;
  int                   rtn = 0;
  boolean               manual;
  sector_t*             sec;
  floormove_t*          mfloor;
  unsigned              value = (unsigned)line->special - GenFloorBase;

  // parse the bit fields in the line's special type

  int Crsh = (value & FloorCrush) >> FloorCrushShift;
  int ChgT = (value & FloorChange) >> FloorChangeShift;
  int Targ = (value & FloorTarget) >> FloorTargetShift;
  int Dirn = (value & FloorDirection) >> FloorDirectionShift;
  int ChgM = (value & FloorModel) >> FloorModelShift;
  int Sped = (value & FloorSpeed) >> FloorSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;  // sector index
    manual = true;  // force exit from loop
    goto manual_floor;  // jump into loop
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

manual_floor:                
    // Do not start another function if floor already moving
    if (P_SectorActive( S_floor_special, sec))
    {
      if (manual)  goto done;
      continue;
    }

    // new floor thinker
    rtn = 1;
    mfloor = Z_Malloc (sizeof(floormove_t), PU_LEVSPEC, 0);
    P_AddThinker (&mfloor->thinker);
    sec->floordata = mfloor;
    mfloor->thinker.function = TFI_MoveFloor;
    mfloor->crush = Crsh;
    mfloor->direction = Dirn? 1 : -1;
    mfloor->sector = sec;
    mfloor->new_floorpic = sec->floorpic;
    mfloor->new_floortype = sec->floortype;  // no change
    mfloor->new_sec_special = sec->special;
    mfloor->old_sec_special = sec->oldspecial;
    mfloor->type = FT_genFloor;

    // set the speed of motion
    switch (Sped)
    {
      case SPEED_Slow:
        mfloor->speed = FLOORSPEED;
        break;
      case SPEED_Normal:
        mfloor->speed = FLOORSPEED*2;
        break;
      case SPEED_Fast:
        mfloor->speed = FLOORSPEED*4;
        break;
      case SPEED_Turbo:
        mfloor->speed = FLOORSPEED*8;
        break;
      default:
        break;
    }

    // set the destination height
    switch(Targ)
    {
      case FTAR_FtoHnF:
        mfloor->floordestheight = P_FindHighestFloorSurrounding(sec);
        break;
      case FTAR_FtoLnF:
        mfloor->floordestheight = P_FindLowestFloorSurrounding(sec);
        break;
      case FTAR_FtoNnF:
        mfloor->floordestheight = Dirn?
          P_FindNextHighestFloor(sec,sec->floorheight) :
          P_FindNextLowestFloor(sec,sec->floorheight);
        break;
      case FTAR_FtoLnC:
        mfloor->floordestheight = P_FindLowestCeilingSurrounding(sec);
        break;
      case FTAR_FtoC:
        mfloor->floordestheight = sec->ceilingheight;
        break;
      case FTAR_FbyST:
        mfloor->floordestheight = (mfloor->sector->floorheight>>FRACBITS) +
          mfloor->direction * (P_FindShortestTextureAround(secnum)>>FRACBITS);
        if (mfloor->floordestheight>32000)
          mfloor->floordestheight=32000;
        if (mfloor->floordestheight<-32000)
          mfloor->floordestheight=-32000;
        mfloor->floordestheight<<=FRACBITS;
        break;
      case FTAR_Fby24:
        mfloor->floordestheight = mfloor->sector->floorheight +
          mfloor->direction * 24*FRACUNIT;
        break;
      case FTAR_Fby32:
        mfloor->floordestheight = mfloor->sector->floorheight +
          mfloor->direction * 32*FRACUNIT;
        break;
      default:
        break;
    }

    // set texture/type change properties
    if (ChgT)   // if a texture change is indicated
    {
      if (ChgM) // if a numeric model change
      {
        sector_t *sec;

        sec = (Targ==FTAR_FtoLnC || Targ==FTAR_FtoC)?
          P_FindModelCeilingSector(mfloor->floordestheight,secnum) :
          P_FindModelFloorSector(mfloor->floordestheight,secnum);
        if (sec)
        {
          mfloor->new_floorpic = sec->floorpic;
          mfloor->new_floortype = sec->floortype;
          switch(ChgT)
          {
            case FCH_FChgZero:  // zero type
              mfloor->new_sec_special = 0;
              mfloor->old_sec_special = 0;
              mfloor->type = FT_genFloorChg0;
              break;
            case FCH_FChgTyp:   // copy type
              mfloor->new_sec_special = sec->special;
              mfloor->old_sec_special = sec->oldspecial;
              mfloor->type = FT_genFloorChgT;
              break;
            case FCH_FChgTxt:   // leave type be
              mfloor->type = FT_genFloorChg;
              break;
            default:
              break;
          }
        }
      }
      else if( line->frontsector )  // except fragglescript with no frontsector
      {
        // trigger frontsector model change
        mfloor->new_floorpic = line->frontsector->floorpic;
        mfloor->new_floortype = line->frontsector->floortype;
        switch (ChgT)
        {
          case FCH_FChgZero:    // zero type
            mfloor->new_sec_special = 0;
            mfloor->old_sec_special = 0;
            mfloor->type = FT_genFloorChg0;
            break;
          case FCH_FChgTyp:     // copy type
            mfloor->new_sec_special = line->frontsector->special;
            mfloor->old_sec_special = line->frontsector->oldspecial;
            mfloor->type = FT_genFloorChgT;
            break;
          case FCH_FChgTxt:     // leave type be
            mfloor->type = FT_genFloorChg;
          default:
            break;
        }
      }
    }
    if (manual)  goto done;
  }
done:
  return rtn;
}


//
// EV_DoGenCeiling()
//
// Handle generalized ceiling types
//
// Passed the linedef activating the ceiling function
// Returns true if a thinker created
//
int EV_DoGenCeiling ( line_t*  line )
{
  int                   secnum;
  int                   rtn = 0;
  boolean               manual;
  fixed_t               targheight;
  sector_t*             sec;
  ceiling_t*            ceiling;
  unsigned              value = (unsigned)line->special - GenCeilingBase;

  // parse the bit fields in the line's special type

  int Crsh = (value & CeilingCrush) >> CeilingCrushShift;
  int ChgT = (value & CeilingChange) >> CeilingChangeShift;
  int Targ = (value & CeilingTarget) >> CeilingTargetShift;
  int Dirn = (value & CeilingDirection) >> CeilingDirectionShift;
  int ChgM = (value & CeilingModel) >> CeilingModelShift;
  int Sped = (value & CeilingSpeed) >> CeilingSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;
    manual = true;  // force exit from loop
    goto manual_ceiling;  // jump into loop
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

manual_ceiling:                
    // Do not start another function if ceiling already moving
    if (P_SectorActive( S_ceiling_special, sec))
    {
      if (manual)  goto done;
      continue;
    }

    // new ceiling thinker
    rtn = 1;
    ceiling = Z_Malloc (sizeof(ceiling_t), PU_LEVSPEC, 0);
    P_AddThinker (&ceiling->thinker);
    sec->ceilingdata = ceiling;
    ceiling->thinker.function = TFI_MoveCeiling;
    ceiling->crush = Crsh;
    ceiling->direction = Dirn? 1 : -1;
    ceiling->sector = sec;
    ceiling->new_ceilingpic = sec->ceilingpic;
    ceiling->new_sec_special = sec->special;
    ceiling->old_sec_special = sec->oldspecial;
    ceiling->tag = sec->tag;
    ceiling->type = CT_genCeiling;

    // set speed of motion
    switch (Sped)
    {
      case SPEED_Slow:
        ceiling->speed = CEILSPEED;
        break;
      case SPEED_Normal:
        ceiling->speed = CEILSPEED*2;
        break;
      case SPEED_Fast:
        ceiling->speed = CEILSPEED*4;
        break;
      case SPEED_Turbo:
        ceiling->speed = CEILSPEED*8;
        break;
      default:
        break;
    }

    // set destination target height
    targheight = sec->ceilingheight;
    switch(Targ)
    {
      case CTAR_CtoHnC:
        targheight = P_FindHighestCeilingSurrounding(sec);
        break;
      case CTAR_CtoLnC:
        targheight = P_FindLowestCeilingSurrounding(sec);
        break;
      case CTAR_CtoNnC:
        targheight = Dirn?
          P_FindNextHighestCeiling(sec,sec->ceilingheight) :
          P_FindNextLowestCeiling(sec,sec->ceilingheight);
        break;
      case CTAR_CtoHnF:
        targheight = P_FindHighestFloorSurrounding(sec);
        break;
      case CTAR_CtoF:
        targheight = sec->floorheight;
        break;
      case CTAR_CbyST:
        targheight = (ceiling->sector->ceilingheight>>FRACBITS) +
          ceiling->direction * (P_FindShortestUpperAround(secnum)>>FRACBITS);
        if (targheight>32000)
          targheight=32000;
        if (targheight<-32000)
          targheight=-32000;
        targheight<<=FRACBITS;
        break;
      case CTAR_Cby24:
        targheight = ceiling->sector->ceilingheight +
          ceiling->direction * 24*FRACUNIT;
        break;
      case CTAR_Cby32:
        targheight = ceiling->sector->ceilingheight +
          ceiling->direction * 32*FRACUNIT;
        break;
      default:
        break;
    }
    //that doesn't compile under windows
    //Dirn? ceiling->topheight : ceiling->bottomheight = targheight;
    if(Dirn)
      ceiling->topheight = targheight;
    else
      ceiling->bottomheight = targheight;

    // set texture/type change properties
    if (ChgT)     // if a texture change is indicated
    {
      if (ChgM)   // if a numeric model change
      {
        sector_t *sec;

        sec = (Targ==CTAR_CtoHnF || Targ==CTAR_CtoF)?
          P_FindModelFloorSector(targheight,secnum) :
          P_FindModelCeilingSector(targheight,secnum);
        if (sec)
        {
          ceiling->new_ceilingpic = sec->ceilingpic;
          switch (ChgT)
          {
            case CCH_CChgZero:  // type is zeroed
              ceiling->new_sec_special = 0;
              ceiling->old_sec_special = 0;
              ceiling->type = CT_genCeilingChg0;
              break;
            case CCH_CChgTyp:   // type is copied
              ceiling->new_sec_special = sec->special;
              ceiling->old_sec_special = sec->oldspecial;
              ceiling->type = CT_genCeilingChgT;
              break;
            case CCH_CChgTxt:   // type is left alone
              ceiling->type = CT_genCeilingChg;
              break;
            default:
              break;
          }
        }
      }
      else if( line->frontsector )  // except fragglescript with no frontsector
      {
        // trigger frontsector model change
        ceiling->new_ceilingpic = line->frontsector->ceilingpic;
        switch (ChgT)
        {
          case CCH_CChgZero:    // type is zeroed
            ceiling->new_sec_special = 0;
            ceiling->old_sec_special = 0;
            ceiling->type = CT_genCeilingChg0;
            break;
          case CCH_CChgTyp:     // type is copied
            ceiling->new_sec_special = line->frontsector->special;
            ceiling->old_sec_special = line->frontsector->oldspecial;
            ceiling->type = CT_genCeilingChgT;
            break;
          case CCH_CChgTxt:     // type is left alone
            ceiling->type = CT_genCeilingChg;
            break;
          default:
            break;
        }
      }
    }
    P_AddActiveCeiling(ceiling);  // add this ceiling to the active list
    if (manual)  goto done;
  }
done:   
  return rtn;
}

//
// EV_DoGenLift()
//
// Handle generalized lift types
//
// Passed the linedef activating the lift
// Returns true if a thinker is created
//
int EV_DoGenLift ( line_t* line )
{
  plat_t*         plat;
  int             secnum;
  int             rtn = 0;
  boolean         manual;
  sector_t*       sec;
  unsigned        value = (unsigned)line->special - GenLiftBase;

  // parse the bit fields in the line's special type

  int Targ = (value & LiftTarget) >> LiftTargetShift;
  int Dely = (value & LiftDelay) >> LiftDelayShift;
  int Sped = (value & LiftSpeed) >> LiftSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  // Activate all <type> plats that are in_stasis

  if (Targ==LTAR_LnF2HnF)
    P_ActivateInStasis(line->tag);
        
  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;
    manual = true;     // force exit from loop
    goto manual_lift;  // jump into loop
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

manual_lift:
    // Do not start another function if floor already moving
    if (P_SectorActive( S_floor_special, sec))
    {
      if (manual)  goto done;
      continue;
    }
      
    // Setup the plat thinker
    rtn = 1;
    plat = Z_Malloc( sizeof(*plat), PU_LEVSPEC, 0);
    P_AddThinker(&plat->thinker);
              
    plat->sector = sec;
    plat->sector->floordata = plat;
    plat->thinker.function = TFI_PlatRaise;
    plat->crush = false;
    plat->tag = line->tag;

    plat->type = PLATT_genLift;
    plat->high = sec->floorheight;
    plat->status = PLATS_down;

    // setup the target destination height
    switch(Targ)
    {
      case LTAR_F2LnF:
        plat->low = P_FindLowestFloorSurrounding(sec);
        if (plat->low > sec->floorheight)
          plat->low = sec->floorheight;
        break;
      case LTAR_F2NnF:
        plat->low = P_FindNextLowestFloor(sec,sec->floorheight);
        break;
      case LTAR_F2LnC:
        plat->low = P_FindLowestCeilingSurrounding(sec);
        if (plat->low > sec->floorheight)
          plat->low = sec->floorheight;
        break;
      case LTAR_LnF2HnF:
        plat->type = PLATT_genPerpetual;
        plat->low = P_FindLowestFloorSurrounding(sec);
        if (plat->low > sec->floorheight)
          plat->low = sec->floorheight;
        plat->high = P_FindHighestFloorSurrounding(sec);
        if (plat->high < sec->floorheight)
          plat->high = sec->floorheight;
        plat->status = PP_Random(pr_genlift)&1;
        break;
      default:
        break;
    }

    // setup the speed of motion
    switch(Sped)
    {
      case SPEED_Slow:
        plat->speed = PLATSPEED * 2;
        break;
      case SPEED_Normal:
        plat->speed = PLATSPEED * 4;
        break;
      case SPEED_Fast:
        plat->speed = PLATSPEED * 8;
        break;
      case SPEED_Turbo:
        plat->speed = PLATSPEED * 16;
        break;
      default:
        break;
    }

    // [WDJ] 1/15/2009 Add DOORDELAY_CONTROL of delay_ticks_per_sec.
    // setup the delay time before the floor returns
    switch(Dely)
    {
      case 0:
        plat->wait = 1 * delay_ticks_per_sec;  // [WDJ]
        break;
      case 1:
        plat->wait = PLATWAIT * delay_ticks_per_sec;  // [WDJ]
        break;
      case 2:
        plat->wait = 5 * delay_ticks_per_sec;  // [WDJ]
        break;
      case 3:
        plat->wait = 10 * delay_ticks_per_sec;  // [WDJ]
        break;
    }

    S_StartSecSound(sec, sfx_pstart);
    P_AddActivePlat(plat); // add this plat to the list of active plats

    if (manual)  goto done;
  }
done:
  return rtn;
}

//
// EV_DoGenStairs()
//
// Handle generalized stair building
//
// Passed the linedef activating the stairs
// Returns true if a thinker is created
//
int EV_DoGenStairs ( line_t* line )
{
  int                   height;
  int                   texture;
  int                   ok;
  int                   rtn = 0;
  int                   secnum, old_secnum, new_secnum;
  int                   i;
  boolean               manual;
    
  sector_t*             sec;
  sector_t*             tsec;

  floormove_t*          mfloor;
    
  fixed_t               stairsize;
  fixed_t               speed;

  unsigned              value = (unsigned)line->special - GenStairsBase;

  // parse the bit fields in the line's special type

  int Igno = (value & StairIgnore) >> StairIgnoreShift;
  int Dirn = (value & StairDirection) >> StairDirectionShift;
  int Step = (value & StairStep) >> StairStepShift;
  int Sped = (value & StairSpeed) >> StairSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;
    manual = true;
    goto manual_stair;
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

manual_stair:          
    //Do not start another function if floor already moving
    //Add special lockout condition to wait for entire
    //staircase to build before retriggering
    if (P_SectorActive( S_floor_special, sec) || sec->stairlock)
    {
      if (manual)  goto done;
      continue;
    }
      
    // new floor thinker
    rtn = 1;
    mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);
    P_AddThinker (&mfloor->thinker);
    sec->floordata = mfloor;
    mfloor->thinker.function = TFI_MoveFloor;
    mfloor->direction = Dirn? 1 : -1;
    mfloor->sector = sec;

    // setup speed of stair building
    switch(Sped)
    {
      default:
      case SPEED_Slow:
        mfloor->speed = FLOORSPEED/4;
        break;
      case SPEED_Normal:
        mfloor->speed = FLOORSPEED/2;
        break;
      case SPEED_Fast:
        mfloor->speed = FLOORSPEED*2;
        break;
      case SPEED_Turbo:
        mfloor->speed = FLOORSPEED*4;
        break;
    }

    // setup stepsize for stairs
    switch(Step)
    {
      default:
      case 0:
        stairsize = 4*FRACUNIT;
        break;
      case 1:
        stairsize = 8*FRACUNIT;
        break;
      case 2:
        stairsize = 16*FRACUNIT;
        break;
      case 3:
        stairsize = 24*FRACUNIT;
        break;
    }

    speed = mfloor->speed;
    height = sec->floorheight + mfloor->direction * stairsize;
    mfloor->floordestheight = height;
    texture = sec->floorpic;
    mfloor->crush = false;
    mfloor->type = FT_genBuildStair;

    sec->stairlock = -2;
    sec->nextsec = -1;
    sec->prevsec = -1;

    old_secnum = secnum;
    // Find next sector to raise
    // 1.     Find 2-sided line with same sector side[0]
    // 2.     Other side is the next sector to raise
    do
    {
      ok = 0;
      for (i = 0; i < sec->linecount; i++)
      {
        // for each line in sector linelist
        register line_t * slinei = sec->linelist[i];
            // [WDJ] ptr slinei, saves 0 bytes, but is easier to read.
        if ( !(slinei->backsector) )   // ignore line with no backsector
          continue;
                                  
        tsec = slinei->frontsector;
        new_secnum = tsec - sectors; // frontsector sector num
          
        if (secnum != new_secnum)  // ignore line with different frontsector
          continue;

        tsec = slinei->backsector;
        new_secnum = tsec - sectors; // backsector sector num

        if (!Igno && tsec->floorpic != texture)
          continue;

        if (!EN_boom)
          height += mfloor->direction * stairsize;

        if (P_SectorActive( S_floor_special, tsec) || tsec->stairlock)
          continue;
        
        if (EN_boom)
          height += mfloor->direction * stairsize;

        // link the stair chain in both directions
        // lock the stair sector until building complete
        sec->nextsec = new_secnum; // link step to next
        tsec->prevsec = secnum;   // link next back
        tsec->nextsec = -1;       // set next forward link as end
        tsec->stairlock = -2;     // lock the step

        sec = tsec;
        secnum = new_secnum;
        mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);

        P_AddThinker (&mfloor->thinker);

        sec->floordata = mfloor;
        mfloor->thinker.function = TFI_MoveFloor;
        mfloor->direction = Dirn? 1 : -1;
        mfloor->sector = sec;
        mfloor->speed = speed;
        mfloor->floordestheight = height;
        mfloor->crush = false;
        mfloor->type = FT_genBuildStair;

        ok = 1;
        break;
      }
    } while(ok);
    if (manual)  goto done;
    secnum = old_secnum;
  }
  // retriggerable generalized stairs build up or down alternately
  if (rtn)
    line->special ^= StairDirection; // alternate dir on succ activations
done:   
  return rtn;
}

//
// EV_DoGenCrusher()
//
// Handle generalized crusher types
//
// Passed the linedef activating the crusher
// Returns true if a thinker created
//
int EV_DoGenCrusher ( line_t* line )
{
  boolean               manual;
  int                   rtn = 0;
  int                   secnum;
  sector_t*             sec;
  ceiling_t*            ceiling;
  unsigned              value = (unsigned)line->special - GenCrusherBase;

  // parse the bit fields in the line's special type

  int Slnt = (value & CrusherSilent) >> CrusherSilentShift;
  int Sped = (value & CrusherSpeed) >> CrusherSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  rtn = P_ActivateInStasisCeiling(line);

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;
    manual = true;
    goto manual_crusher;
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

manual_crusher:                
    // Do not start another function if ceiling already moving
    if (P_SectorActive( S_ceiling_special, sec))
    {
      if (manual)  goto done;
      continue;
    }

    // new ceiling thinker
    rtn = 1;
    ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
    P_AddThinker (&ceiling->thinker);
    sec->ceilingdata = ceiling;
    ceiling->thinker.function = TFI_MoveCeiling;
    ceiling->crush = true;
    ceiling->direction = -1;
    ceiling->sector = sec;
    ceiling->new_ceilingpic = sec->ceilingpic;
    ceiling->new_sec_special = sec->special;
    ceiling->tag = sec->tag;
    ceiling->type = Slnt? CT_genSilentCrusher : CT_genCrusher;
    ceiling->topheight = sec->ceilingheight;
    ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);

    // setup ceiling motion speed
    switch (Sped)
    {
      case SPEED_Slow:
        ceiling->speed = CEILSPEED;
        break;
      case SPEED_Normal:
        ceiling->speed = CEILSPEED*2;
        break;
      case SPEED_Fast:
        ceiling->speed = CEILSPEED*4;
        break;
      case SPEED_Turbo:
        ceiling->speed = CEILSPEED*8;
        break;
      default:
        break;
    }
    ceiling->oldspeed=ceiling->speed;

    P_AddActiveCeiling(ceiling);  // add to list of active ceilings
    if (manual)  goto done;
  }
done:
  return rtn;
}

//
// EV_DoGenLockedDoor()
//
// Handle generalized locked door types
//
// Passed the linedef activating the generalized locked door
// Returns true if a thinker created
//
int EV_DoGenLockedDoor ( line_t* line )
{
  boolean manual;
  int   rtn = 0;
  int   secnum;
  sector_t* sec;
  vldoor_t* door;
  unsigned  value = (unsigned)line->special - GenLockedBase;

  // parse the bit fields in the line's special type

  int Kind = (value & LockedKind) >> LockedKindShift;
  int Sped = (value & LockedSpeed) >> LockedSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;
    manual = true;
    goto manual_locked;
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
manual_locked:
    // Do not start another function if ceiling already moving
    if (P_SectorActive( S_ceiling_special, sec))
    {
      if (manual)  goto done;
      continue;
    }
  
    // new door thinker
    rtn = 1;
    door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
    P_AddThinker (&door->thinker);
    sec->ceilingdata = door;

    door->thinker.function = TFI_VerticalDoor;
    door->sector = sec;
    door->topwait = VDOORWAIT;
    door->line = line;
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4*FRACUNIT;
    door->direction = 1;

    // [WDJ] MBF gradual light, From MBF, PrBoom.
    // killough 10/98: implement gradual lighting.
    door->lighttag =
        ( EN_doorlight
          && (line->special&6) == 6
          && line->special > GenLockedBase ) ? line->tag : 0;

#ifdef DEBUG_LIGHTTAG
    if( door->lighttag )
    {
        // Detect fault situation in cyberarena3 map02.
        // Cannot reproduce this, so have to detect the initiation.
        printf( "DoGenLockedDoor: Lighttag = %i, line# = %i\n", door->lighttag, line - lines );
    }
#endif

    // setup speed of door motion
    switch(Sped)
    {
      default:
      case SPEED_Slow:
        door->type = Kind? VD_genOpen : VD_genRaise;
        door->speed = VDOORSPEED;
        break;
      case SPEED_Normal:
        door->type = Kind? VD_genOpen : VD_genRaise;
        door->speed = VDOORSPEED*2;
        break;
      case SPEED_Fast:
        door->type = Kind? VD_genBlazeOpen : VD_genBlazeRaise;
        door->speed = VDOORSPEED*4;
        break;
      case SPEED_Turbo:
        door->type = Kind? VD_genBlazeOpen : VD_genBlazeRaise;
        door->speed = VDOORSPEED*8;

        break;
    }

    // killough 4/15/98: fix generalized door opening sounds
    // (previously they always had the blazing door close sound)
    S_StartSecSound(door->sector,   // killough 4/15/98
                 door->speed >= VDOORSPEED*4 ? sfx_bdopn : sfx_doropn);

    if (manual)  goto done;
  }
done:
  return rtn;
}

//
// EV_DoGenDoor()
//
// Handle generalized door types
//
// Passed the linedef activating the generalized door
// Returns true if a thinker created
//
int EV_DoGenDoor ( line_t* line )
{
  boolean   manual;
  int   secnum;
  int   rtn = 0;
  vldoor_t* door;
  sector_t* sec;
  unsigned  value = (unsigned)line->special - GenDoorBase;

  // parse the bit fields in the line's special type

  int Dely = (value & DoorDelay) >> DoorDelayShift;
  int Kind = (value & DoorKind) >> DoorKindShift;
  int Sped = (value & DoorSpeed) >> DoorSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==TRIG_PushOnce || Trig==TRIG_PushMany)
  {
    if (!(sec = line->backsector))  goto done;
    secnum = sec - sectors;
    manual = true;
    goto manual_door;
  }

  // if not manual do all sectors tagged the same as the line
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
manual_door:
    // Do not start another function if ceiling already moving
    if (P_SectorActive( S_ceiling_special, sec))
    {
      if (manual)  goto done;
      continue;
    }
  
    // new door thinker
    rtn = 1;
    door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
    P_AddThinker (&door->thinker);
    sec->ceilingdata = door;

    door->thinker.function = TFI_VerticalDoor;
    door->sector = sec;
    // setup delay for door remaining open/closed
    switch(Dely)
    {
      default:
      case 0:
        door->topwait = 35;
        break;
      case 1:
        door->topwait = VDOORWAIT;
        break;
      case 2:
        door->topwait = 2*VDOORWAIT;
        break;
      case 3:
        door->topwait = 7*VDOORWAIT;
        break;
    }

    // setup speed of door motion
    switch(Sped)
    {
      default:
      case SPEED_Slow:
        door->speed = VDOORSPEED;
        break;
      case SPEED_Normal:
        door->speed = VDOORSPEED*2;
        break;
      case SPEED_Fast:
        door->speed = VDOORSPEED*4;
        break;
      case SPEED_Turbo:
        door->speed = VDOORSPEED*8;
        break;
    }
    door->line = line;

    // [WDJ] MBF gradual light, From MBF, PrBoom.
    // killough 10/98: implement gradual lighting.
    door->lighttag =
        ( EN_doorlight
          && (line->special&6) == 6
          && line->special > GenLockedBase ) ? line->tag : 0;

#ifdef DEBUG_LIGHTTAG
    if( door->lighttag )
    {
        // Detect fault situation in cyberarena3 map02.
        // Cannot reproduce this, so have to detect the initiation.
        printf( "DoGenDoor: Lighttag = %i, line# = %i\n", door->lighttag, line - lines );
    }
#endif

    // set kind of door, whether it opens then close, opens, closes etc.
    // assign target heights accordingly
    switch(Kind)
    {
      case DT_OdCDoor:
        door->direction = 1;
        door->topheight = P_FindLowestCeilingSurrounding(sec);
        door->topheight -= 4*FRACUNIT;
        if (door->topheight != sec->ceilingheight)
          S_StartSecSound(door->sector, sfx_bdopn);
        door->type = (Sped>=SPEED_Fast)? VD_genBlazeRaise : VD_genRaise;
        break;
      case DT_ODoor:
        door->direction = 1;
        door->topheight = P_FindLowestCeilingSurrounding(sec);
        door->topheight -= 4*FRACUNIT;
        if (door->topheight != sec->ceilingheight)
          S_StartSecSound(door->sector, sfx_bdopn);
        door->type = (Sped>=SPEED_Fast)? VD_genBlazeOpen : VD_genOpen;
        break;
      case DT_CdODoor:
        door->topheight = sec->ceilingheight;
        door->direction = -1;
        S_StartSecSound(door->sector, sfx_dorcls);
        door->type = (Sped>=SPEED_Fast)? VD_genBlazeCdO : VD_genCdO;
        break;
      case DT_CDoor:
        door->topheight = P_FindLowestCeilingSurrounding(sec);
        door->topheight -= 4*FRACUNIT;
        door->direction = -1;
        S_StartSecSound(door->sector, sfx_dorcls);
        door->type = (Sped>=SPEED_Fast)? VD_genBlazeClose : VD_genClose;
        break;
      default:
        break;
    }
    if (manual)  goto done;
  }
done:
  return rtn;
}


          
