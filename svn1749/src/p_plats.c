// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_plats.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
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
// $Log: p_plats.c,v $
// Revision 1.5  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.4  2000/10/21 08:43:30  bpereira
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
//      Plats (i.e. elevator platforms) code, raising/lowering.
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
#include "m_random.h"

//SoM: 3/7/2000: Use boom's limitless format.
platlist_t      *activeplats;


//
// Move a plat up and down
//
void T_PlatRaise(plat_t* plat)
{
    result_e    res;

    switch(plat->status)
    {
      case  PLATS_up:
        res = T_MovePlane( plat->sector, plat->speed,
                           plat->high, plat->crush, 0, 1);

        if ( EN_heretic && !(leveltime % (32*NEWTICRATERATIO)))
            S_StartSecSound(plat->sector, sfx_stnmov);

        if (plat->type == PLATT_raiseAndChange
            || plat->type == PLATT_raiseToNearestAndChange)
        {
            if (!(leveltime % (8*NEWTICRATERATIO)))
                S_StartSecSound(plat->sector, sfx_stnmov);
        }


        if (res == MP_crushed && (!plat->crush))
        {
            plat->count = plat->wait;
            plat->status = PLATS_down;
            S_StartSecSound(plat->sector, sfx_pstart);
        }
        else
        {
            if (res == MP_pastdest)
            {
                //SoM: 3/7/2000: Moved this little baby over.
                // if not an instant toggle type, wait, make plat stop sound
                if (plat->type!=PLATT_toggleUpDn)
                {
                  plat->count = plat->wait;
                  plat->status = PLATS_waiting;
                  S_StartSecSound(plat->sector, sfx_pstop);
                }
                else // else go into stasis awaiting next toggle activation
                {
                  plat->oldstatus = plat->status;//jff 3/14/98 after action wait  
                  plat->status = PLATS_in_stasis;  //for reactivation of toggle
                }

                if( EN_heretic )
                {
                    // Heretic: Must not remove others, or else can retrigger.
                    switch(plat->type)
                    {
                      case PLATT_downWaitUpStay:
                      case PLATT_raiseAndChange:
                        P_RemoveActivePlat(plat);
                        break;
                      // case PLATT_raiseToNearestAndChange:
                      default:
                        break;
                    }
                    return;
                }

                // Doom and Boom
                switch(plat->type)
                {
                  case PLATT_blazeDWUS:
                  case PLATT_downWaitUpStay:
                  case PLATT_raiseAndChange:
                  case PLATT_raiseToNearestAndChange:
                  case PLATT_genLift:
                    P_RemoveActivePlat(plat); //SoM: 3/7/2000: Much cleaner boom code.
                  default:
                    break;
                }
            }
        }
        break;

      case  PLATS_down:
        res = T_MovePlane( plat->sector, plat->speed, plat->low, false, 0, -1 );

        if (res == MP_pastdest)
        {
            //SoM: 3/7/2000: if not an instant toggle, start waiting, make plat stop sound
            if (plat->type!=PLATT_toggleUpDn) 
            {                           
              plat->count = plat->wait;
              plat->status = PLATS_waiting;
              S_StartSecSound(plat->sector, sfx_pstop);
            }
            else //SoM: 3/7/2000: instant toggles go into stasis awaiting next activation
            {
              plat->oldstatus = plat->status;
              plat->status = PLATS_in_stasis;
            }

            //jff 1/26/98 remove the plat if it bounced so it can be tried again
            //only affects plats that raise and bounce
    
            if (EN_boom)
            {
              switch(plat->type)
              {
                case PLATT_raiseAndChange:
                case PLATT_raiseToNearestAndChange:
                  P_RemoveActivePlat(plat);
                default:
                  break;
              }
            }
        }
        else
        {
            if (EN_heretic && !(leveltime & 31))
                S_StartSecSound(plat->sector, sfx_stnmov);
        }

        break;

      case  PLATS_waiting:
        if (!--plat->count)
        {
            if (plat->sector->floorheight == plat->low)
                plat->status = PLATS_up;
            else
                plat->status = PLATS_down;
            S_StartSecSound(plat->sector, sfx_pstart);
        }
      case  PLATS_in_stasis:
        break;
    }
}


//
// Do Platforms
//  "amount" is only used for SOME platforms.
//
int  EV_DoPlat ( line_t* line, plattype_e type, int amount )
{
    plat_t    * plat;
    sector_t  * sec;
    int         secnum;
    int         rtn = 0;

    //  Activate all <type> plats that are PLATS_in_stasis
    switch(type)
    {
      case PLATT_perpetualRaise:
        P_ActivateInStasis(line->tag);
        break;

      case PLATT_toggleUpDn:
        P_ActivateInStasis(line->tag);
        rtn=1;
        break;

      default:
        break;
    }

    secnum = -1;  // init search FindSector
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];

        if (P_SectorActive( S_floor_special, sec)) //SoM: 3/7/2000: 
            continue;

        // Find lowest & highest floors around sector
        rtn = 1;
        plat = Z_Malloc( sizeof(*plat), PU_LEVSPEC, 0);
        P_AddThinker(&plat->thinker);

        plat->type = type;
        plat->sector = sec;
        plat->sector->floordata = plat; //SoM: 3/7/2000
        plat->thinker.function = TFI_PlatRaise;
        plat->crush = false;
        plat->tag = line->tag;

        //jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
        //going down forever -- default low to plat height when triggered
        plat->low = sec->floorheight;

        // [WDJ] 1/15/2009 Add DOORDELAY_CONTROL of delay_ticks_per_sec.
        switch(type)
        {
          case PLATT_raiseToNearestAndChange:
            plat->speed = PLATSPEED/2;
            sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
            plat->high = P_FindNextHighestFloor(sec,sec->floorheight);
            plat->wait = 0;
            plat->status = PLATS_up;
            // NO MORE DAMAGE, IF APPLICABLE
            sec->special = 0;
            sec->oldspecial = 0; //SoM: 3/7/2000: Clear old field.

            S_StartSecSound(sec, sfx_stnmov);
            break;

          case PLATT_raiseAndChange:
            plat->speed = PLATSPEED/2;
            sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
            plat->high = sec->floorheight + amount*FRACUNIT;
            plat->wait = 0;
            plat->status = PLATS_up;

            S_StartSecSound(sec, sfx_stnmov);
            break;

          case PLATT_downWaitUpStay:
            plat->speed = PLATSPEED * 4;
            plat->low = P_FindLowestFloorSurrounding(sec);

            if (plat->low > sec->floorheight)
                plat->low = sec->floorheight;

            plat->high = sec->floorheight;
            plat->wait = PLATWAIT * delay_ticks_per_sec;  // [WDJ]
            plat->status = PLATS_down;
            S_StartSecSound(sec, sfx_pstart);
            break;

          case PLATT_blazeDWUS:
            plat->speed = PLATSPEED * 8;
            plat->low = P_FindLowestFloorSurrounding(sec);

            if (plat->low > sec->floorheight)
                plat->low = sec->floorheight;

            plat->high = sec->floorheight;
            plat->wait = PLATWAIT * delay_ticks_per_sec;
            plat->status = PLATS_down;
            S_StartSecSound(sec, sfx_pstart);
            break;

          case PLATT_perpetualRaise:
            plat->speed = PLATSPEED;
            plat->low = P_FindLowestFloorSurrounding(sec);

            if (plat->low > sec->floorheight)
                plat->low = sec->floorheight;

            plat->high = P_FindHighestFloorSurrounding(sec);

            if (plat->high < sec->floorheight)
                plat->high = sec->floorheight;

            plat->wait = PLATWAIT * delay_ticks_per_sec;  // [WDJ]
            plat->status = PP_Random(pr_plats)&1;

            S_StartSecSound(sec, sfx_pstart);
            break;

          case PLATT_toggleUpDn: //SoM: 3/7/2000: Instant toggle.
            plat->speed = PLATSPEED;
            plat->wait = PLATWAIT * delay_ticks_per_sec;  // [WDJ]
            plat->crush = true;

            // set up toggling between ceiling, floor inclusive
            plat->low = sec->ceilingheight;
            plat->high = sec->floorheight;
            plat->status = PLATS_down;
            break;

          default:
            break;
        }
        P_AddActivePlat(plat);
    }
    return rtn;
}


//SoM: 3/7/2000: Use boom limit removal
void P_ActivateInStasis( uint16_t tag )
{
  platlist_t *pl;
  for (pl=activeplats; pl; pl=pl->next)
  {
    plat_t *plat = pl->plat;
    if (plat->tag == tag && plat->status == PLATS_in_stasis) 
    {
      if (plat->type==PLATT_toggleUpDn)
        plat->status = (plat->oldstatus==PLATS_up)? PLATS_down : PLATS_up;
      else
        plat->status = plat->oldstatus;

      plat->thinker.function = TFI_PlatRaise;
    }
  }
}

//SoM: 3/7/2000: use Boom code instead.
int EV_StopPlat(line_t* line)
{
  platlist_t *pl;
  for (pl=activeplats; pl; pl=pl->next)
  {
    plat_t *plat = pl->plat;
    if ((plat->status != PLATS_in_stasis) && (plat->tag == line->tag))
    {
      plat->oldstatus = plat->status;
      plat->status = PLATS_in_stasis;
      plat->thinker.function = AI_NULL;
    }
  }
  return 1;
}

//SoM: 3/7/2000: No more limits!
void P_AddActivePlat(plat_t* plat)
{
  platlist_t *list = malloc(sizeof *list);
  list->plat = plat;
  plat->list = list;
  if ((list->next = activeplats))
    list->next->prev = &list->next;
  list->prev = &activeplats;
  activeplats = list;
}

//SoM: 3/7/2000: No more limits!
void P_RemoveActivePlat(plat_t* plat)
{
  platlist_t *list = plat->list;
  plat->sector->floordata = NULL; //jff 2/23/98 multiple thinkers
  P_RemoveThinker(&plat->thinker);
  if ((*list->prev = list->next))
    list->next->prev = list->prev;
  free(list);
}


//SoM: 3/7/2000: Removes all active plats.
void P_Remove_AllActivePlats(void)
{
  while (activeplats)
  {  
    platlist_t *next = activeplats->next;
    free(activeplats);
    activeplats = next;
  }
}
