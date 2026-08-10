// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_floor.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2010 by DooM Legacy Team.
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
// $Log: p_floor.c,v $
// Revision 1.13  2002/06/14 02:43:43  ssntails
// Instant-lower and instant-raise capability for sectors added.
//
// Revision 1.12  2001/03/30 17:12:50  bpereira
// no message
//
// Revision 1.11  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.10  2000/11/02 17:50:07  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.9  2000/10/21 08:43:30  bpereira
// Revision 1.8  2000/09/28 20:57:16  bpereira
// Revision 1.7  2000/07/01 09:23:49  bpereira
//
// Revision 1.6  2000/05/23 15:22:34  stroggonmeth
// Not much. A graphic bug fixed.
//
// Revision 1.5  2000/04/16 18:38:07  bpereira
// Revision 1.4  2000/04/08 17:29:24  stroggonmeth
//
// Revision 1.3  2000/04/04 00:32:46  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "doomstat.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"



// ==========================================================================
//                              FLOORS
// ==========================================================================

// local enables of p_floor, TNT MAP30 fix
byte EN_boom_stairbuild_fix = 1;

void DemoAdapt_p_floor( void )
{
    // [WDJ] 8/29/2011  Boom fixed the stair building bug, where some
    // steps would get double height.
    // TNT MAP30 relies upon the bug to get the height right on the stairs
    // at the red key card, and the stairs in the final room, where the
    // player must fire from the 2nd step from the top to hit the Boss.
    EN_boom_stairbuild_fix = EN_boom && (gamedesc_id != GDESC_tnt);
    // [WDJ] DSDA has a mbf compatibility level that blocks this fix.
    // MBF did not have the stair build fix.
}


// [WDJ] Other doom code does not have 3D floors and does not have floor
// floor stopping dependent upon numattached.  To keep compatible
// behavior, the floor movement must always stop and recheck.
// I have found no reason for the && numattached test.  It may have been
// thought that only 3D floors could obstruct in those directions, but it is
// also possible because of dest in wrong direction (insta-move).  Doom2 Map5.
#define COMPAT_FLOOR_STOP  1

//
// Move a plane (floor or ceiling) and check for crushing
//
//SoM: I had to copy the entire function from Boom because it was causing errors.
// Move a floor or ceiling, update all affected structures.
result_e T_MovePlane ( sector_t*     sector,
                       fixed_t       speed,
                       fixed_t       dest,
                       boolean       crush, // enables crushing damage
                       int           floorOrCeiling,
                       int           direction )
{
  boolean       flag;
  fixed_t       lastpos;    // when hit something, must return to lastpos
  fixed_t       destheight; //jff 02/04/98 used to keep floors/ceilings
                            // from moving thru each other
  fixed_t       newheight;


  switch(floorOrCeiling)
  {
    case 0:
      // Moving a floor
      lastpos = sector->floorheight;
      if(direction < 0)
      {
          // Move floor down
          newheight = sector->floorheight - speed;
          //SoM: 3/20/2000: Make splash when platform floor hits water
          if((sector->model == SM_Legacy_water) && EN_boom)
          {
            if((newheight < sectors[sector->modelsec].floorheight )
               && (sector->floorheight > sectors[sector->modelsec].floorheight))
              S_StartSecSound(sector, sfx_gloop);
          }
          // Moving a floor down
          if (newheight < dest)
          { // reached dest, or start was below dest
            sector->floorheight = dest;  // final position
            flag = P_CheckSector(sector,crush);
#ifdef COMPAT_FLOOR_STOP
            if (flag == true)  // hit something
#else
            if (flag == true && sector->numattached)
#endif
            {
              sector->floorheight =lastpos;
              P_CheckSector(sector,crush);
            }
            return MP_pastdest;
          }
          else
          { // floor moving down
            sector->floorheight = newheight;  // intermediate position
            flag = P_CheckSector(sector,crush);
            // PrBoom: EN_boom_floor = !comp[comp_floors]
            // Vanilla compatibility, or 3D floor.
            if( flag == true
                && (!EN_boom_floor || sector->numattached) )
            {
              // Diff here between Boom and original Doom.
              // This code not in Boom, added back by prboom.
              // It stops floors from moving when objects stuck in ceiling.
              // May be necessary because of 3D floors.
              sector->floorheight = lastpos;
              P_CheckSector(sector, crush);
              return MP_crushed;
            }
          }
      }
      else
      {
          // Move floor up
          newheight = sector->floorheight + speed;
          // keep floor from moving thru ceilings
          //SoM: 3/20/2000: Make splash when platform floor hits water
          if((sector->model == SM_Legacy_water) && EN_boom)
          {
            if((newheight > sectors[sector->modelsec].floorheight)
               && (sector->floorheight < sectors[sector->modelsec].floorheight))
              S_StartSecSound(sector, sfx_gloop);
          }

          destheight = dest;  // Vanilla and usual case
          if( EN_boom_floor && (dest > sector->ceilingheight) )
              destheight = sector->ceilingheight;  // limit at ceiling

          if (newheight > destheight)
          { // reached dest, or start was above dest
            sector->floorheight = destheight;  // final position
            flag = P_CheckSector(sector,crush);
            if (flag == true)
            {
              sector->floorheight = lastpos;
              P_CheckSector(sector,crush);
            }
            return MP_pastdest;
          }
          else
          { // floor moving up
            // crushing is possible
            sector->floorheight = newheight;  // intermediate position
            flag = P_CheckSector(sector,crush);
            if (flag == true)
            {
              if(!EN_boom_floor)
              {
                if (crush == true)
                  return MP_crushed;
              }
              sector->floorheight = lastpos;
              P_CheckSector(sector,crush);
              return MP_crushed;
            }
          }
      }
      break;
                                                                        
    case 1:
      // moving a ceiling
      lastpos = sector->ceilingheight;
      if(direction < 0)
      {
          // Move ceiling down
          newheight = sector->ceilingheight - speed;
          if((sector->model == SM_Legacy_water) && EN_boom)
          {
            // make sound when ceiling hits water
            if((newheight < sectors[sector->modelsec].floorheight)
               && (sector->ceilingheight > sectors[sector->modelsec].floorheight))
              S_StartSecSound(sector, sfx_gloop);
          }

          // moving a ceiling down
          // keep ceiling from moving thru floors
          destheight = dest;
          if( EN_boom_floor && (dest < sector->floorheight) )
              destheight = sector->floorheight;

          if (newheight < destheight)
          { // reached dest, or start was below dest
            sector->ceilingheight = destheight;  // final position
            flag = P_CheckSector(sector,crush);
            if (flag == true)
            {
              sector->ceilingheight = lastpos;
              P_CheckSector(sector,crush);
            }
            return MP_pastdest;
          }
          else
          { // ceiling moving down
            // crushing is possible
            sector->ceilingheight = newheight;  // intermediate position
            flag = P_CheckSector(sector,crush);
            if (flag == true)
            {
              if (crush == true)
                return MP_crushed;
              sector->ceilingheight = lastpos;
              P_CheckSector(sector,crush);
              return MP_crushed;
            }
          }
      }
      else
      {
          // Move ceiling up
          newheight = sector->ceilingheight + speed;
          if((sector->model == SM_Legacy_water) && EN_boom)
          {
            // make sound when ceiling hits water
            if((newheight > sectors[sector->modelsec].floorheight)
               && (sector->ceilingheight < sectors[sector->modelsec].floorheight))
              S_StartSecSound(sector, sfx_gloop);
          }
          // moving a ceiling up
          if (newheight > dest)
          { // reached dest, or start was above dest
            sector->ceilingheight = dest;  // final position
#if 0
            // alternative plan
            // wad specific intercepts
            if ( lastpos > dest ) // insta-move, started wrong side of dest
            {
                if( gamedesc_id == GDESC_doom2 )  return MP_pastdest; // ignore
            }
#endif
            flag = P_CheckSector(sector,crush);
#ifdef COMPAT_FLOOR_STOP
            // [WDJ] Doom2 Map05 will close secret rooms tagged with 9, unless
            // the ceiling is stopped.  Cannot be conditional on 3D floor.
            if (flag == true)
#else
            // [WDJ] BUG: Doom2 Map05, this closes secret rooms tagged with 9,
            // because the ceiling is not stopped.
            if (flag == true && sector->numattached)
# if 0
            // alternative plan
            if (flag == true
                &&( sector->numattached
                    || lastpos > dest ) // insta-move, started wrong side of dest
                )
# endif
#endif
            {
              // Boom stops.
              sector->ceilingheight = lastpos;
              P_CheckSector(sector,crush);
            }
            return MP_pastdest;
          }
          else
          { // ceiling moving up
            sector->ceilingheight = newheight;  // intermediate position
            flag = P_CheckSector(sector,crush);
            if (flag == true && sector->numattached) // 3D floor only
            {
              // This code not in Boom.
              // It stops ceiling from moving when objects stuck in floor.
              // Doom2 Map06, spider demon will not be crushed because
              // is stuck in ceiling on first try.
              // May be necessary because of 3D floors.
              // Crush on 3D floor may cause moving floor to get stuck.
              sector->ceilingheight = lastpos;
              P_CheckSector(sector,crush);
              return MP_crushed;
            }
          }
      }
      break;
    }
    return MP_ok;
}


//
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t* mfloor)
{
    result_e    res = 0;
    sector_t *  secmov;  // moving sector

    res = T_MovePlane(mfloor->sector,
                      mfloor->speed,
                      mfloor->floordestheight,
                      mfloor->crush, 0, mfloor->direction);

    secmov = mfloor->sector;

    if (!(leveltime % (8*NEWTICRATERATIO)))
        S_StartSecSound(secmov, ceilmovesound);

    // [WDJ] Floortype was set by setup according to floorpic.
    // If the floorpic is changed, then also change the floortype.
    // If the floortype is not changed then water splashes from water
    // textures will still be heard after the texture change.

    if (res == MP_pastdest)
    {
        //mfloor->sector->specialdata = NULL;
        if (mfloor->direction == 1)   // raise
        {
            switch(mfloor->type)
            {
              case FT_donutRaise:  // Doom, Heretic
                secmov->floorpic = mfloor->new_floorpic;
                secmov->floortype = mfloor->new_floortype;
                P_Update_Special_Sector( secmov, mfloor->new_sec_special );
                break;
              //SoM: 3/6/2000: Add support for General types
              case FT_genFloorChgT: // Boom, change sector type
              case FT_genFloorChg0: // Boom, zero sector type
                //SoM: 3/6/2000: this records the old special of the sector
                secmov->oldspecial = mfloor->old_sec_special;
                P_Update_Special_Sector( secmov, mfloor->new_sec_special );
                // change floorpic too
              case FT_genFloorChg: // Boom, change only texture
                secmov->floorpic = mfloor->new_floorpic;
                secmov->floortype = mfloor->new_floortype;
                break;
              default:
                break;
            }
        }
        else if (mfloor->direction == -1)   // lower
        {
            switch(mfloor->type)
            {
              case FT_lowerAndChange:  // Doom, Heretic
                // SoM: 3/6/2000: Store old special type
                secmov->oldspecial = mfloor->old_sec_special;
                secmov->floorpic = mfloor->new_floorpic;
                secmov->floortype = mfloor->new_floortype;
                P_Update_Special_Sector( secmov, mfloor->new_sec_special );
                break;
              case FT_genFloorChgT:  // Boom, change sector type
              case FT_genFloorChg0:  // Boom, zero sector type
                secmov->oldspecial = mfloor->old_sec_special;
                P_Update_Special_Sector( secmov, mfloor->new_sec_special );
                // change floorpic too
              case FT_genFloorChg:  // Boom, change only texture
                secmov->floorpic = mfloor->new_floorpic;
                secmov->floortype = mfloor->new_floortype;
                break;
              default:
                break;
            }
        }

        secmov->floordata = NULL; // Clear up the thinker so others can use it
        P_RemoveThinker(&mfloor->thinker);

        // SoM: This code locks out stair steps while generic, retriggerable generic stairs
        // are building.
      
        if (secmov->stairlock==-2) // if this sector is stairlocked
        {
          sector_t *sec = secmov;
          sec->stairlock=-1;              // thinker done, promote lock to -1

          while (sec->prevsec>=0 && sectors[sec->prevsec].stairlock!=-2)
            sec = &sectors[sec->prevsec]; // search for a non-done thinker
          if (sec->prevsec==-1)           // if all thinkers previous are done
          {
            sec = secmov;          // search forward
            while (sec->nextsec>=0 && sectors[sec->nextsec].stairlock!=-2) 
              sec = &sectors[sec->nextsec];
            if (sec->nextsec==-1)         // if all thinkers ahead are done too
            {
              while (sec->prevsec>=0)    // clear all locks
              {
                sec->stairlock = 0;
                sec = &sectors[sec->prevsec];
              }
              sec->stairlock = 0;
            }
          }
        }

        if ((mfloor->type == FT_buildStair && gamemode == heretic) || 
            gamemode != heretic)
            S_StartSecSound(secmov, sfx_pstop);
    }

}


// SoM: 3/6/2000: Lots'o'copied code here.. Elevators.
//
// T_MoveElevator()
//
// Move an elevator to it's destination (up or down)
// Called once per tick for each moving floor.
//
// Passed an elevator_t structure that contains all pertinent info about the
// move. See P_SPEC.H for fields.
// No return.
//
// SoM: 3/6/2000: The function moves the plane differently based on direction, so if it's 
// traveling really fast, the floor and ceiling won't hit each other and stop the lift.
void T_MoveElevator(elevator_t* elevator)
{
  result_e      res = 0;

  if (elevator->direction<0)      // moving down
  {
    res = T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
    (
      elevator->sector,
      elevator->speed,
      elevator->ceilingdestheight,
      0,
      1,                          // move floor
      elevator->direction
    );
    if (res==MP_ok || res==MP_pastdest) // jff 4/7/98 don't move ceil if blocked
      T_MovePlane
      (
        elevator->sector,
        elevator->speed,
        elevator->floordestheight,
        0,
        0,                        // move ceiling
        elevator->direction
      );
  }
  else // up
  {
    res = T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
    (
      elevator->sector,
      elevator->speed,
      elevator->floordestheight,
      0,
      0,                          // move ceiling
      elevator->direction
    );
    if (res==MP_ok || res==MP_pastdest) // jff 4/7/98 don't move floor if blocked
      T_MovePlane
      (
        elevator->sector,
        elevator->speed,
        elevator->ceilingdestheight,
        0,
        1,                        // move floor
        elevator->direction
      );
  }

  // make floor move sound
  if (!(leveltime % (8*NEWTICRATERATIO)))
    S_StartSecSound(elevator->sector, sfx_stnmov);
    
  if (res == MP_pastdest)            // if destination height acheived
  {
    elevator->sector->floordata = NULL;     //jff 2/22/98
    elevator->sector->ceilingdata = NULL;   //jff 2/22/98
    P_RemoveThinker(&elevator->thinker);    // remove elevator from actives

    // make floor stop sound
    S_StartSecSound(elevator->sector, sfx_pstop);
  }
}




//
// HANDLE FLOOR TYPES
//
int EV_DoFloor ( line_t* line, floor_e floortype )
{
    sector_t*           sec;
    floormove_t*        mfloor;
    int                 rtn = 0;
    int                 i;

    int secnum = -1;  // init search FindSector
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
        
        // SoM: 3/6/2000: Boom has multiple thinkers per sector.
        // Don't start a second thinker on the same floor
        if (P_SectorActive( S_floor_special, sec)) //jff 2/23/98
          continue;

        // new floor thinker
        rtn = 1;
        mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);
        P_AddThinker (&mfloor->thinker);
        sec->floordata = mfloor; //SoM: 2/5/2000
        mfloor->sector = sec;  // [WDJ] is same for every action
        mfloor->thinker.function = TFI_MoveFloor;
        mfloor->type = floortype;
        mfloor->crush = false;

        switch(floortype)
        {
          case FT_lowerFloor:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = P_FindHighestFloorSurrounding(sec);
            break;

            //jff 02/03/30 support lowering floor by 24 absolute
          case FT_lowerFloor24:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = mfloor->sector->floorheight + 24 * FRACUNIT;
            break;

            //jff 02/03/30 support lowering floor by 32 absolute (fast)
          case FT_lowerFloor32Turbo:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED*4;
            mfloor->floordestheight = mfloor->sector->floorheight + 32 * FRACUNIT;
            break;

          case FT_lowerFloorToLowest:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = P_FindLowestFloorSurrounding(sec);
            break;

            //jff 02/03/30 support lowering floor to next lowest floor
          case FT_lowerFloorToNearest:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight =
              P_FindNextLowestFloor(sec,mfloor->sector->floorheight);
            break;

          case FT_turboLower:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED * 4;
            mfloor->floordestheight = P_FindHighestFloorSurrounding(sec);
            if (mfloor->floordestheight != sec->floorheight || gamemode == heretic )
                mfloor->floordestheight += 8*FRACUNIT;
            break;

          case FT_raiseFloorCrush:
            mfloor->crush = true;
          case FT_raiseFloor:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = P_FindLowestCeilingSurrounding(sec);
            if (mfloor->floordestheight > sec->ceilingheight)
                mfloor->floordestheight = sec->ceilingheight;
            mfloor->floordestheight -= (8*FRACUNIT)* (floortype == FT_raiseFloorCrush);
            break;

          case FT_raiseFloorTurbo:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED*4;
            mfloor->floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
            break;

          case FT_raiseFloorToNearest:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
            break;

          case FT_raiseFloor24:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = mfloor->sector->floorheight + 24 * FRACUNIT;
            break;

          // SoM: 3/6/2000: support straight raise by 32 (fast)
          case FT_raiseFloor32Turbo:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED*4;
            mfloor->floordestheight = mfloor->sector->floorheight + 32 * FRACUNIT;
            break;

          case FT_raiseFloor512:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = mfloor->sector->floorheight + 512 * FRACUNIT;
            break;

          case FT_raiseFloor24AndChange:
            mfloor->direction = 1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = mfloor->sector->floorheight + 24 * FRACUNIT;
            sec->floorpic = line->frontsector->floorpic;
            sec->floortype = line->frontsector->floortype;
            sec->oldspecial = line->frontsector->oldspecial;
            P_Update_Special_Sector( sec, line->frontsector->special );
            break;

          case FT_raiseToTexture:
          {
              fixed_t   minsize = FIXED_MAX;
              side_t*   side;

              // Prevents height overflow.
              if(EN_boom_physics) minsize = 32000<<FRACBITS; //SoM: 3/6/2000:

              mfloor->direction = 1;
//              mfloor->sector = sec;
              mfloor->speed = FLOORSPEED;
              for (i = 0; i < sec->linecount; i++)
              {
                if (twoSided (secnum, i) )
                {
                  side = getSide(secnum,i,0);
                  // jff 8/14/98 don't scan texture 0, its not real
                  if (side->bottomtexture > 0 ||
                      (!EN_boom_physics && !side->bottomtexture))
                  {
                    if (textureheight[side->bottomtexture] < minsize)
                      minsize = textureheight[side->bottomtexture];
                  }
                  side = getSide(secnum,i,1);
                  // jff 8/14/98 don't scan texture 0, its not real
                  if (side->bottomtexture > 0 ||
                      (!EN_boom_physics && !side->bottomtexture))
                  {
                    if (textureheight[side->bottomtexture] < minsize)
                      minsize = textureheight[side->bottomtexture];
                  }
                }
              }
              if(!EN_boom_physics)
              {
                // Vanilla: may overflow
                mfloor->floordestheight = mfloor->sector->floorheight + minsize;
              }
              else
              {
                // Boom: prevent overflow (jff 3/13/98), modified [WDJ]
                register fixed_t fdh =
                  (mfloor->sector->floorheight>>FRACBITS) + (minsize>>FRACBITS);
                if( fdh > 32000 )  fdh = 32000;
                mfloor->floordestheight = fdh<<FRACBITS;
              }
            break;
          }
          //SoM: 3/6/2000: Boom changed allot of stuff I guess, and this was one of 'em 
          case FT_lowerAndChange:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FLOORSPEED;
            mfloor->floordestheight = P_FindLowestFloorSurrounding(sec);
            mfloor->new_floorpic = sec->floorpic;
            mfloor->new_floortype = sec->floortype;

            // jff 1/24/98 make sure mfloor->new_sec_special gets initialized
            // in case no surrounding sector is at floordestheight
            // --> should not affect compatibility <--
            mfloor->new_sec_special = sec->special; 
            //jff 3/14/98 transfer both old and new special
            mfloor->old_sec_special = sec->oldspecial;
    
            //jff 5/23/98 use model subroutine to unify fixes and handling
            // BP: heretic have change something here
            sec = P_FindModelFloorSector(mfloor->floordestheight,sec-sectors);
            if (sec)
            {
              mfloor->new_floorpic = sec->floorpic;
              mfloor->new_floortype = sec->floortype;
              mfloor->new_sec_special = sec->special;
              //jff 3/14/98 transfer both old and new special
              mfloor->old_sec_special = sec->oldspecial;
            }
            break;
          // Instant Lower SSNTails 06-13-2002
          case FT_instantLower:
            mfloor->direction = -1;
//            mfloor->sector = sec;
            mfloor->speed = FIXED_MAX/2; // Go too fast and you'll cause problems...
            mfloor->floordestheight = P_FindLowestFloorSurrounding(sec);
            break;
          default:
            break;
        }
    }
    return rtn;
}


// SoM: 3/6/2000: Function for changing just the floor texture and type.
//
// EV_DoChange()
//
// Handle pure change types. These change floor texture and sector type
// by trigger or numeric model without moving the floor.
//
// The linedef causing the change and the type of change is passed
// Returns true if any sector changes
//
//
int EV_DoChange ( line_t* line, change_e changetype )
{
  sector_t*             sec;
  sector_t*             secm;
  int                   rtn = 0;

  int secnum = -1; // init search FindSector
  // change all sectors with the same tag as the linedef
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
              
    rtn = 1;

    // handle trigger or numeric change type
    switch(changetype)
    {
      case CH_MODEL_trig_only:
        sec->floorpic = line->frontsector->floorpic;
        sec->floortype = line->frontsector->floortype;
        sec->oldspecial = line->frontsector->oldspecial;
        P_Update_Special_Sector( sec, line->frontsector->special );
        break;
      case CH_MODEL_num_only:
        secm = P_FindModelFloorSector(sec->floorheight,secnum);
        if (secm) // if no model, no change
        {
          sec->floorpic = secm->floorpic;
          sec->floortype = secm->floortype;
          sec->oldspecial = secm->oldspecial;
          P_Update_Special_Sector( sec, secm->special );
        }
        break;
      default:
        break;
    }
  }
  return rtn;
}




//
// BUILD A STAIRCASE!
//

// SoM: 3/6/2000: Use the Boom version of this function.
int EV_BuildStairs ( line_t*  line, stair_e type )
{
  int                   height;
  int                   texture;
  int                   do_another;
  int                   rtn = 0;
  int                   secnum, new_secnum, old_secnum;
  int                   i;
  boolean               crushing = false;  // crushing stairs
    
  sector_t*             sec;
  sector_t*             tsec;

  floormove_t*          mfloor;
    
  fixed_t               stairsize;
  fixed_t               speed;

  // [WDJ] crushing, speed, stairsize, are constant for the loop.
  // Set up the speed and stepsize according to the stairs type.
  // [WDJ] PrBoom had compatibility with bug due to uninitialized crush field.
  // DoomLegacy does not emulate this bug.
  switch(type)
  {
    case ST_build8:
      speed = FLOORSPEED/4;
      stairsize = 8*FRACUNIT;
      crushing = false;
      break;
    case ST_turbo16:
      speed = FLOORSPEED*4;
      stairsize = 16*FRACUNIT;
      crushing = true;
      break;
    // used by heretic
    default:
      speed = FLOORSPEED;
      stairsize = type;
      crushing = true;
      break;
  }
  if( ! EN_boom ) // logic of above cases, crushing stairs only in Boom
      crushing = false;
  // [WDJ] init crush even when not EN_boom, same for all of stair.
   
  secnum = -1; // init search FindSector
  // start a stair at each sector tagged the same as the linedef
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
              
    // don't start a stair if the first step's floor is already moving
    if (P_SectorActive( S_floor_special, sec))
      continue;
      
    // create new floor thinker for first step
    rtn = 1;
    mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);
    P_AddThinker (&mfloor->thinker);
    sec->floordata = mfloor;
    mfloor->thinker.function = TFI_MoveFloor;
    mfloor->direction = 1;
    mfloor->sector = sec;
    mfloor->type = FT_buildStair;   //jff 3/31/98 do not leave uninited
    mfloor->new_sec_special = 0;  // init, see DoomWiki staircase bug
    mfloor->new_floorpic = 0;     // init
    mfloor->new_floortype = FLOOR_SOLID;
    mfloor->crush = crushing;  //jff 2/27/98 fix uninitialized crush field
    mfloor->speed = speed;
    height = sec->floorheight + stairsize;
    mfloor->floordestheight = height;
              
    texture = sec->floorpic;
    old_secnum = secnum;           //jff 3/4/98 preserve loop index
      
    // Find next sector to raise
    //   1. Find 2-sided line with same sector side[0] (lowest numbered)
    //   2. Other side is the next sector to raise
    //   3. Unless already moving, or different texture, then stop building
    do
    {
      do_another = 0;
      for (i = 0; i < sec->linecount; i++)
      {
        // for each line of the sector linelist
        if ( !((sec->linelist[i])->flags & ML_TWOSIDED) )
          continue;
                                  
        tsec = (sec->linelist[i])->frontsector;
        new_secnum = tsec-sectors;
          
        if (secnum != new_secnum)
          continue;

        tsec = (sec->linelist[i])->backsector;
        if (!tsec) continue;     //jff 5/7/98 if no backside, continue
        new_secnum = tsec - sectors;

        // if sector's floor is different texture, look for another
        if (tsec->floorpic != texture)
          continue;

        // Boom moves stair incr, but cannot use on TNT MAP30
        if (!EN_boom_stairbuild_fix) // jff 6/19/98 prevent double stepsize
          height += stairsize; // jff 6/28/98 change demo compatibility

        // if sector's floor already moving, look for another
        if (P_SectorActive( S_floor_special, tsec)) //jff 2/22/98
          continue;

        // Boom stairbuild fix
        if (EN_boom_stairbuild_fix) // jff 6/19/98 increase height AFTER continue
          height += stairsize; // jff 6/28/98 change demo compatibility

        sec = tsec;
        secnum = new_secnum;

        // create and initialize a thinker for the next step
        mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);
        P_AddThinker (&mfloor->thinker);

        sec->floordata = mfloor; //jff 2/22/98
        mfloor->thinker.function = TFI_MoveFloor;
        mfloor->direction = 1;
        mfloor->sector = sec;
        mfloor->speed = speed;
        mfloor->floordestheight = height;
        mfloor->type = FT_buildStair; //jff 3/31/98 do not leave uninited
        mfloor->new_sec_special = 0;  // init, see DoomWiki staircase bug
        mfloor->new_floorpic = 0;     // init
        mfloor->crush = crushing;  //jff 2/27/98 fix uninitialized crush field

        do_another = 1;
        break;
      }
    } while(do_another);      // continue until no next step is found
    secnum = old_secnum; //jff 3/4/98 restore loop index
  }
  return rtn;
}


//SoM: 3/6/2000: boom donut function
//
// EV_DoDonut()
//
// Handle donut function: lower pillar, raise surrounding pool, both to height,
// texture and type of the sector surrounding the pool.
//
// Passed the linedef that triggered the donut
// Returns whether a thinker was created
//
int EV_DoDonut(line_t*  line)
{
  sector_t* s1;
  sector_t* s2;
  sector_t* s2model;
  int       secnum;
  int       rtn = 0;
  int       i;

  floormove_t* mfloor;  // new floor

  // do function on all sectors with same tag as linedef
  secnum = -1;  // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    s1 = &sectors[secnum];                // s1 is pillar's sector
              
    // do not start the donut if the pillar is already moving
    if (P_SectorActive( S_floor_special, s1)) //jff 2/22/98
      continue;
                      
    s2 = getNextSector(s1->linelist[0],s1);  // s2 is pool's sector
    if (!s2) continue;                    // note lowest numbered line around
                                          // pillar must be two-sided 

    // do not start the donut if the pool is already moving
    if( EN_boom_floor && P_SectorActive( S_floor_special, s2) ) 
      continue;                           //jff 5/7/98
                      
    // find a two sided line around the pool whose other side isn't the pillar
    for (i = 0; i < s2->linecount; i++)
    {
      // for each line of sector s2 linelist
      // [WDJ] using ptr s = s2->linelist[i] gives larger code (by 32 bytes).
      //jff 3/29/98 use true two-sidedness, not the flag
      // killough 4/5/98: changed demo_compatibility to compatibility
      if(!EN_boom_physics)
      {
        if (!(s2->linelist[i]->flags & ML_TWOSIDED)
            || (s2->linelist[i]->backsector == s1))
          continue;
      }
      else if (!s2->linelist[i]->backsector
               || s2->linelist[i]->backsector == s1)
        continue;

      rtn = 1; //jff 1/26/98 no donut action - no switch change on return

      s2model = s2->linelist[i]->backsector;  // s2model is model sector for changes
        
      //  Spawn rising slime
      mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);
      P_AddThinker (&mfloor->thinker);
      s2->floordata = mfloor; //jff 2/22/98
      mfloor->thinker.function = TFI_MoveFloor;
      mfloor->type = FT_donutRaise;
      mfloor->crush = false;
      mfloor->direction = 1;
      mfloor->sector = s2;
      mfloor->speed = FLOORSPEED / 2;
      mfloor->new_floorpic = s2model->floorpic;
      mfloor->new_sec_special = 0;
      mfloor->floordestheight = s2model->floorheight;
        
      //  Spawn lowering donut-hole pillar
      mfloor = Z_Malloc (sizeof(*mfloor), PU_LEVSPEC, 0);
      P_AddThinker (&mfloor->thinker);
      s1->floordata = mfloor; //jff 2/22/98
      mfloor->thinker.function = TFI_MoveFloor;
      mfloor->type = FT_lowerFloor;
      mfloor->crush = false;
      mfloor->direction = -1;
      mfloor->sector = s1;
      mfloor->speed = FLOORSPEED / 2;
      mfloor->floordestheight = s2model->floorheight;
      break;
    }
  }
  return rtn;
}


// SoM: Boom elevator support.
//
// EV_DoElevator
//
// Handle elevator linedef types
//
// Passed the linedef that triggered the elevator and the elevator action
//
// jff 2/22/98 new type to move floor and ceiling in parallel
//
int EV_DoElevator ( line_t* line, elevator_e elevtype )
{
  sector_t*             sec;
  elevator_t*           elevator;
  int                   rtn = 0;

  // act on all sectors with the same tag as the triggering linedef
  int secnum = -1; // init search FindSector
  while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
              
    // If either floor or ceiling is already activated, skip it
    if (sec->floordata || sec->ceilingdata) //jff 2/22/98
      continue;
      
    // create and initialize new elevator thinker
    rtn = 1;
    elevator = Z_Malloc (sizeof(*elevator), PU_LEVSPEC, 0);
    P_AddThinker (&elevator->thinker);
    sec->floordata = elevator; //jff 2/22/98
    sec->ceilingdata = elevator; //jff 2/22/98
    elevator->thinker.function = TFI_MoveElevator;
    elevator->type = elevtype;

    // set up the fields according to the type of elevator action
    switch(elevtype)
    {
        // elevator down to next floor
      case ET_elevateDown:
        elevator->direction = -1;
        elevator->sector = sec;
        elevator->speed = ELEVATORSPEED;
        elevator->floordestheight =
          P_FindNextLowestFloor(sec,sec->floorheight);
        elevator->ceilingdestheight =
          elevator->floordestheight + sec->ceilingheight - sec->floorheight;
        break;

        // elevator up to next floor
      case ET_elevateUp:
        elevator->direction = 1;
        elevator->sector = sec;
        elevator->speed = ELEVATORSPEED;
        elevator->floordestheight =
          P_FindNextHighestFloor(sec,sec->floorheight);
        elevator->ceilingdestheight =
          elevator->floordestheight + sec->ceilingheight - sec->floorheight;
        break;

        // elevator to floor height of activating switch's front sector
      case ET_elevateCurrent:
        elevator->sector = sec;
        elevator->speed = ELEVATORSPEED;
        elevator->floordestheight = line->frontsector->floorheight;
        elevator->ceilingdestheight =
          elevator->floordestheight + sec->ceilingheight - sec->floorheight;
        elevator->direction =
          elevator->floordestheight>sec->floorheight?  1 : -1;
        break;

      default:
        break;
    }
  }
  return rtn;
}
