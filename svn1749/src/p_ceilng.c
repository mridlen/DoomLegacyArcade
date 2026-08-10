// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_ceilng.c 1736 2025-03-14 08:15:57Z wesleyjohnson $
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
// $Log: p_ceilng.c,v $
// Revision 1.9  2002/06/14 02:43:43  ssntails
// Instant-lower and instant-raise capability for sectors added.
//
// Revision 1.8  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.7  2000/10/21 08:43:30  bpereira
// Revision 1.6  2000/09/28 20:57:16  bpereira
//
// Revision 1.5  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
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
//      Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"

// ==========================================================================
//                              CEILINGS
// ==========================================================================


// SoM: 3/6/2000: the list of ceilings moving currently, including crushers
ceilinglist_t * activeceilings;
//ceiling_t*      activeceilings[MAXCEILINGS];
int ceilmovesound = sfx_stnmov;

//
// T_MoveCeiling
//

void T_MoveCeiling (ceiling_t* ceiling)
{
    result_e    res;

    switch(ceiling->direction)
    {
      case 0:
        // IN STASIS
        break;
      case 1:
        // UP
        res = T_MovePlane(ceiling->sector,
                          ceiling->speed,
                          ceiling->topheight,
                          false,1,ceiling->direction);

        if (!(leveltime % (8*NEWTICRATERATIO)))
        {
            switch(ceiling->type)
            {
              case CT_silentCrushAndRaise:
              case CT_genSilentCrusher:
                break;
              default:
                S_StartSecSound(ceiling->sector, ceilmovesound);
                // ?
                break;
            }
        }

        if (res == MP_pastdest)
        {
            switch(ceiling->type)
            {

              case CT_raiseToHighest:
              //SoM: 3/6/2000
              case CT_genCeiling:  // Boom, general ceiling move
                P_RemoveActiveCeiling(ceiling);
                break;

              // SoM: 3/6/2000: movers with texture change, change the texture then get removed
              case CT_genCeilingChgT:  // Boom, change sector type
              case CT_genCeilingChg0:  // Boom, zero sector type
                ceiling->sector->special = ceiling->new_sec_special;
                ceiling->sector->oldspecial = ceiling->old_sec_special;
                // change ceilingpic too
              case CT_genCeilingChg:  // Boom, change only texture
                ceiling->sector->ceilingpic = ceiling->new_ceilingpic;
                P_RemoveActiveCeiling(ceiling);
                break;

              case CT_silentCrushAndRaise:
                S_StartSecSound(ceiling->sector, sfx_pstop);
              case CT_fastCrushAndRaise:
              case CT_genCrusher: // SoM: 3/6/2000
              case CT_genSilentCrusher:
              case CT_crushAndRaise:
                ceiling->direction = -1;
                break;

              default:
                break;
            }
        }
        break;

      case -1:
        // DOWN
        res = T_MovePlane(ceiling->sector,
                          ceiling->speed,
                          ceiling->bottomheight,
                          ceiling->crush,1,ceiling->direction);

        if (!(leveltime % (8*NEWTICRATERATIO)))
        {
            switch(ceiling->type)
            {
              case CT_silentCrushAndRaise:
              case CT_genSilentCrusher:
                break;
              default:
                S_StartSecSound(ceiling->sector, ceilmovesound);
            }
        }

        if (res == MP_pastdest)
        {
            switch(ceiling->type) //SoM: 3/6/2000: Use boom code
            {
              case CT_genSilentCrusher:
              case CT_genCrusher:
                if (ceiling->oldspeed<CEILSPEED*3)
                  ceiling->speed = ceiling->oldspeed;
                ceiling->direction = 1;
                break;
    
              // make platform stop at bottom of all crusher strokes
              // except generalized ones, reset speed, start back up
              case CT_silentCrushAndRaise:
                S_StartSecSound(ceiling->sector, sfx_pstop);
              case CT_crushAndRaise: 
                ceiling->speed = CEILSPEED;
              case CT_fastCrushAndRaise:
                ceiling->direction = 1;
                break;
              
              // in the case of ceiling mover/changer, change the texture
              // then remove the active ceiling
              case CT_genCeilingChgT:  // Boom
              case CT_genCeilingChg0:  // Boom
                ceiling->sector->special = ceiling->new_sec_special;
                //jff add to fix bug in special transfers from changes
                ceiling->sector->oldspecial = ceiling->old_sec_special;
              case CT_genCeilingChg:  // Boom
                ceiling->sector->ceilingpic = ceiling->new_ceilingpic;
                P_RemoveActiveCeiling(ceiling);
                break;
    
              // all other case, just remove the active ceiling
              case CT_lowerAndCrush:
              case CT_lowerToFloor:
              case CT_lowerToLowest:
              case CT_lowerToMaxFloor:
              case CT_genCeiling:
                P_RemoveActiveCeiling(ceiling);
                break;
    
              default:
                break;
            }
        }
        else // ( res != MP_pastdest )
        {
            if (res == MP_crushed)
            {
                switch(ceiling->type)
                {
                  //SoM: 2/6/2000
                  // slow down slow crushers on obstacle
                  case CT_genCrusher:  
                  case CT_genSilentCrusher:
                    if (ceiling->oldspeed < CEILSPEED*3)
                      ceiling->speed = CEILSPEED / 8;
                    break;

                  case CT_silentCrushAndRaise:
                  case CT_crushAndRaise:
                  case CT_lowerAndCrush:
                    ceiling->speed = CEILSPEED / 8;
                    break;

                  default:
                    break;
                }
            }
        }
        break;
    }
}


//
// EV_DoCeiling
// Move a ceiling up/down and all around!
//
int  EV_DoCeiling ( line_t* line, ceiling_e type )
{
    int         secnum;
    int         rtn;
    sector_t*   sec;
    ceiling_t*  ceiling;

    secnum = -1;
    rtn = 0;

    //  Reactivate in-stasis ceilings...for certain types.
    // This restarts a crusher after it has been stopped
    switch(type)
    {
      case CT_fastCrushAndRaise:
      case CT_silentCrushAndRaise:
      case CT_crushAndRaise:
        rtn = P_ActivateInStasisCeiling(line); //SoM: Return true if the crusher is activated
      default:
        break;
    }

    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];


        if (P_SectorActive( S_ceiling_special, sec))  //SoM: 3/6/2000
            continue;

        // new door thinker
        rtn = 1;
        ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
        P_AddThinker (&ceiling->thinker);
        sec->ceilingdata = ceiling;
        ceiling->thinker.function = TFI_MoveCeiling;
        ceiling->sector = sec;
        ceiling->crush = false;

        switch(type)
        {
          case CT_fastCrushAndRaise:
            ceiling->crush = true;
            ceiling->topheight = sec->ceilingheight;
            ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED * 2;
            break;

          case CT_silentCrushAndRaise:
          case CT_crushAndRaise:
            ceiling->crush = true;
            ceiling->topheight = sec->ceilingheight;
          case CT_lowerAndCrush:
          case CT_lowerToFloor:
            ceiling->bottomheight = sec->floorheight;
            if (type != CT_lowerToFloor)
                ceiling->bottomheight += 8*FRACUNIT;
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED;
            break;

          case CT_raiseToHighest:
            ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
            ceiling->direction = 1;
            ceiling->speed = CEILSPEED;
            break;

          //SoM: 3/6/2000: Added Boom types
          case CT_lowerToLowest:
            ceiling->bottomheight = P_FindLowestCeilingSurrounding(sec);
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED;
            break;

          case CT_lowerToMaxFloor:
            ceiling->bottomheight = P_FindHighestFloorSurrounding(sec);
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED;
            break;

          // Instant-raise SSNTails 06-13-2002
          case CT_instantRaise:
            ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
            ceiling->direction = 1;
            ceiling->speed = FIXED_MAX/2; // Go too fast and you'll cause problems...
            break;

          default:
            break;

        }

        ceiling->tag = sec->tag;
        ceiling->type = type;
        P_AddActiveCeiling(ceiling);
    }
    return rtn;
}


//
// Add an active ceiling
//
//SoM: 3/6/2000: Take advantage of the new Boom method for active ceilings.
void P_AddActiveCeiling(ceiling_t* ceiling)
{
  ceilinglist_t *list = malloc(sizeof *list);
  list->ceiling = ceiling;
  ceiling->list = list;
  if ((list->next = activeceilings))
    list->next->prev = &list->next;
  list->prev = &activeceilings;
  activeceilings = list;
}



//
// Remove a ceiling's thinker
//
// SoM: 3/6/2000 :Use improved Boom code.
void P_RemoveActiveCeiling(ceiling_t* ceiling)
{
  ceilinglist_t *list = ceiling->list;
  ceiling->sector->ceilingdata = NULL;  //jff 2/22/98
  P_RemoveThinker(&ceiling->thinker);
  if ((*list->prev = list->next))
    list->next->prev = list->prev;
  free(list);
}



//
// Restart a ceiling that's in-stasis
//
//SoM: 3/6/2000: Use improved boom code
int P_ActivateInStasisCeiling(line_t * line)
{
  ceilinglist_t *cl;
  int rtn=0;

  for (cl=activeceilings; cl; cl=cl->next)
  {
    ceiling_t *ceiling = cl->ceiling;
    if (ceiling->tag == line->tag && ceiling->direction == 0)
    {
      ceiling->direction = ceiling->olddirection;
      ceiling->thinker.function = TFI_MoveCeiling;
      rtn=1;
    }
  }
  return rtn;
}



//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
//SoM: 3/6/2000: use improved Boom code
int EV_CeilingCrushStop(line_t* line)
{
  int rtn=0;

  ceilinglist_t *cl;
  for (cl=activeceilings; cl; cl=cl->next)
  {
    ceiling_t *ceiling = cl->ceiling;
    if (ceiling->direction != 0 && ceiling->tag == line->tag)
    {
      ceiling->olddirection = ceiling->direction;
      ceiling->direction = 0;
      ceiling->thinker.function = AI_NULL;
      rtn=1;
    }
  }
  return rtn;
}



// SoM: 3/6/2000: Extra, boom only function.
//
// P_Remove_AllActiveCeilings()
//
// Removes all ceilings from the active ceiling list
//
// Passed nothing, returns nothing
//
void P_Remove_AllActiveCeilings(void)
{
  while (activeceilings)
  {  
    ceilinglist_t *next = activeceilings->next;
    free(activeceilings);
    activeceilings = next;
  }
}
