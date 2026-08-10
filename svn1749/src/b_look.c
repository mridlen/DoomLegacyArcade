// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_look.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
//
// Copyright (C) 2002-2016 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//
// $Log: b_look.c,v $
// Revision 1.5  2003/06/11 04:20:45  ssntails
// Keep stupid bots from trying to get to items on 3d Floors.
//
// Revision 1.4  2003/06/11 04:04:50  ssntails
// Rellik's Bot Code!
//
// Revision 1.3  2002/09/28 06:53:11  tonyd
// fixed CR problem, fixed game options crash
//
// Revision 1.2  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
//-----------------------------------------------------------------------------

#include "b_bot.h"
#include "b_game.h"
#include "b_look.h"
//#include "bot_move.h"
#include "b_node.h"
//#include "bot_ctf.h"

#include "doomincl.h"
#include "g_game.h"
#include "r_defs.h"
#include "p_local.h"
#include "m_random.h"
#include "r_main.h"
#include "z_zone.h"
           
#define MAX_TRAVERSE_DIST 100000000 //10 meters, used within b_func.c

extern int max_soul_health;
extern int max_armor;
extern thinker_t thinkercap;

//Used with Reachable().
static mobj_t	* bot_looker_mobj, * bot_dest_mobj;
static sector_t * bot_last_sector;

static boolean PTR_QuickReachable (intercept_t * in)
{
    fixed_t floorheight, ceilingheight;
    line_t* line;
    mobj_t* thing;
    sector_t* s;

    if (in->isaline)
    {
        line = in->d.line;

        if (!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING))
            return false; //Cannot continue.
       
        //Determine if going to use backsector/frontsector.
        s = (line->backsector == bot_last_sector) ? line->frontsector : line->backsector;
        ceilingheight = s->ceilingheight;
        floorheight = s->floorheight;

        int sector_height_diff = FIXED_TO_INT(floorheight - bot_last_sector->floorheight);  // + is higher wall, - is cliff edge
        if( sector_height_diff < -24 )
            return false;  // cannot get back up

        if( sector_height_diff > 24 )
        {
            if( cv_allowjump.EV )
                return false;  // cannot jump

            if( (sector_height_diff > 45)  // highest jump
                || ( (sector_height_diff > 37)  // jump in water
                     && (bot_last_sector->floortype != FLOOR_WATER)) )
                return false;  // too high to jump
        }

        if( ((ceilingheight == floorheight) && line->special)
            || ((ceilingheight - floorheight) >= bot_looker_mobj->height) )  //Does it fit?
        {
            bot_last_sector = s;
            return true;
        }
        return false;
    }
    else
    {
        thing = in->d.thing;
        // Care about solid things that can block our path.
        // Cannot jump over a solid corpse yet.
        if( (thing != bot_looker_mobj) && (thing != bot_dest_mobj) && (thing->flags & MF_SOLID) )
             return false;
    }

    return true;
}

boolean B_Reachable(player_t* p, mobj_t* mo)
{
    bot_looker_mobj = p->mo;
    bot_last_sector = p->mo->subsector->sector;
    bot_dest_mobj = mo;

    // Bots shouldn't try to get stuff that's on a 3dfloor they can't get to. SSNTails 06-10-2003
    if(p->mo->subsector == mo->subsector && p->mo->subsector->sector->ffloors)
    {
      ffloor_t*  rover;

      for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
      {
        if(!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

        if( *rover->topheight <= p->mo->z && mo->z < *rover->topheight )
            return false;

        if( *rover->bottomheight >= p->mo->z + p->mo->height
            && mo->z > *rover->bottomheight)
            return false;
      }
    }

    return P_PathTraverse (p->mo->x, p->mo->y, mo->x, mo->y,
                           PT_ADDLINES|PT_ADDTHINGS, PTR_QuickReachable);
}

//Checks TRUE reachability from
//one actor to another. First mobj (actor) is bot_looker_mobj.
boolean B_ReachablePoint (player_t* p, sector_t* destSector,
                          fixed_t x, fixed_t y)
{
/*  if((destSector->ceilingheight - destSector->floorheight)
        < p->mo->height) //Where target is, bot_looker_mobj can't be.
        return false;
 */

    //if (p->mo->subsector->sector == destSector)
    //	return true;

    bot_looker_mobj = p->mo;
    bot_last_sector = p->mo->subsector->sector;
    bot_dest_mobj = NULL;

    return P_PathTraverse (p->mo->x, p->mo->y, x, y,
                           PT_ADDLINES|PT_ADDTHINGS, PTR_QuickReachable);
}


//
// B_LookForSpecialLine
//
// This function looks through the sector the bot is in,
// and one sector level outwards.
// Very inefficient cause searches of sectors are done multiple times.
// when a sector has many linedefs between a single sector-sector boundary
// Must fix this, perhaps use the visited boolean.
// Maybe should do search through the switches array instead.
//

static
boolean B_Is_Usable_Special_Line( short line_special )
{
  return 
    //edge->special && !(edge->special & ML_REPEAT_SPECIAL)
    //P_CheckTag(edge) && (!specialsector || !specialsector->ceilingdata))
    //!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING)
    //((edge->special & TriggerType) >> TriggerTypeShift) == SwitchOnce)
    //|| ((edge->special & TriggerType) >> TriggerTypeShift) == PushOnce)
    (line_special == 31)   // Door
    // || (edge->special == 1)
    || (line_special == 23)    // SW: Lower floor to lowest
    || (line_special == 102)   // SW: Lower floor to surrounding floor height
    || (line_special == 103)   // SW: Open Door
    || (line_special == 71) ;  // SW: Turbo lower floor
}

boolean B_LookForSpecialLine(player_t* p, fixed_t* x, fixed_t* y)
{
    int  i, j;
    sector_t * in_sector, * sector2;
    line_t * edge;
    msecnode_t * in_sector_node;

    in_sector_node = p->mo->touching_sectorlist;
    while (in_sector_node)
    {
        in_sector = in_sector_node->m_sector;
        for (i = 0; i < in_sector->linecount; i++)
        {
            // for all lines in sector linelist
            edge = in_sector->linelist[i];
            // sector_t * specialsector = (in_sector == edge->frontsector) ? edge->backsector : edge->frontsector;
            if( B_Is_Usable_Special_Line( edge->special ) )  goto ret_edge_center;

            if (edge->sidenum[1] != NULL_INDEX)
            {
                // its a double sided linedef
                sector2 = (edge->frontsector == in_sector) ? edge->backsector : edge->frontsector;

                for (j = 0; j < sector2->linecount; j++)
                {
                    // for all lines in sector linelist
                    edge = sector2->linelist[j];
                    // sector_t * specialsector = (sector == edge->frontsector) ? edge->backsector : edge->frontsector;
                    if( B_Is_Usable_Special_Line( edge->special ) )  goto ret_edge_center;
                }
            }
        }
        in_sector_node = in_sector_node->m_snext;
    }

    return false;

ret_edge_center:
    *x = (edge->v1->x + edge->v2->x)/2;
    *y = (edge->v1->y + edge->v2->y)/2;
    return true;
}

// id : any identifier
// on_time, period_time : tics
// Return periodic value, 0..255
byte regulate( mobj_t * mo, int id, int on_time, int period_time )
{
    // Periodic, individualized for each id and mo.
    int mobjid = (intptr_t) mo;
    int pr = (gametic + id + (mobjid>>1)) % period_time;  // periodic ramp
    
    if( mo->health < 5 ) // more desperate
    {
        pr -= TICRATE;  // add a second
    }
    
    if( pr < on_time )  return 255;
    return 0;
}

typedef enum {
  WB_SHOT = 0x01,  // shotgun
  WB_SSG  = 0x02,  // supershotgun
  WB_CHAIN = 0x04, // chaingun
  WB_ROCKET = 0x08, // rocket launcher
  WB_PLASMA = 0x10, // plasma
  WB_BFG = 0x20    // BFG
} weapon_bits_e;



//
// B_LookForThings
//
//  p : a bot player
void B_LookForThings( player_t* p )
{
    fixed_t  bestItemDistance = 0;
    fixed_t  bestSeenItemDistance = 0;
    fixed_t  closestEnemyDistance = 0;
    fixed_t  closestMissileDistance = 0;
    fixed_t  closestUnseenEnemyDistance = 0;
    fixed_t  closestUnseenTeammateDistance = 0;
    fixed_t  furthestTeammateDistance = 0;
    fixed_t  thingDistance = 0;

    // ItemWeight is usefulness of item, byte 0..10
    byte  bestItemWeight = 0; //used to determine best object to get
    byte  bestSeenItemWeight = 0;
    byte  itemWeight = 0;
    int   enemy_weight = 0;

    mobj_t *  pmo = p->mo;  // player mobj
    mobj_t *  bestSeenItem = NULL;
    mobj_t *  bestItem = NULL;
    mobj_t *  mo;
    bot_t *  pbot = p->bot;  // player bot
    thinker_t*	 currentthinker;
   
    byte item_respawn = cv_itemrespawn.EV || (deathmatch == 2);  // DM_items
    byte item_getable;
   
    byte weapon = 0;
    byte ammo = 0;
    byte out_of_ammo = 0;

    // Make actions of bot friendlier when coop with another player.
    byte grab_index = cv_bot_grab.EV;
    byte friend_weight = 5;
#ifdef ENABLE_COME_HERE
    byte friend_summon = (pmo->flags & MF_FRIEND) && come_here;
#endif

    if( (pmo->flags & MF_FRIEND) && (grab_index <= 2) )
    {
        friend_weight = 100;
        if( grab_index )  grab_index --;
    }

    static byte thresh_soul_health_table[] =  // 0..32
    {
         8, // low
        12, // moderate
        24, // friend
        32, // grabby, previous non-deathmatch, no friends
        32, // selfish, previous behavior for comp
    };
    static byte thresh_armor_table[] =  // 0..32
    {
         8, // low
        10, // moderate
        16, // friend
        32, // grabby, previous non-deathmatch, no friends
        32, // selfish, previous behavior for comp
    };
    static int thresh_power_table[] =  // 0..32
    {
        0, // low
        0, // moderate
        15*TICRATE, // friend
        32*TICRATE, // grabby, previous non-deathmatch, no friends
        100*TICRATE, // selfish, previous behavior for comp
    };

    int thresh_soul_health = (max_soul_health * thresh_soul_health_table[ grab_index ]) >> 5;
    int thresh_armor = (max_armor * thresh_armor_table[ grab_index ]) >> 5;
    int thresh_power = thresh_power_table[ grab_index ];

    static byte health_index_table[5][5] =  // 0..100
    {
        { 8,10,20,30, 40}, // low
        {10,25,40,50, 60}, // moderate
        {15,30,50,65, 80}, // friend
        {40,50,60,80,100}, // grabby, previous non-deathmatch, no friends
        {40,50,60,80,100}, // selfish, previous behavior for comp. Before 1.48.16
    };

    int health_index;  // 0..5
    for( health_index = 0; health_index < 5; health_index ++ )
    {
        // Let the compiler optimize this.
        if( p->health < health_index_table[ cv_bot_grab.EV ][health_index] )   break;
    }

    // Old deathmatch behavior
    if( deathmatch && (cv_bot_grab.EV == 4) )
    {
        friend_weight = 0;
        thresh_soul_health = 200;
        thresh_armor = 200;
        thresh_power = 100*TICRATE;
    }

    pbot->closestEnemy = NULL;
    pbot->closestMissile = NULL;
    pbot->closestUnseenEnemy = NULL;
    pbot->closestUnseenTeammate = NULL;
    pbot->teammate = NULL;
    pbot->bestSeenItem = NULL;
    pbot->bestItem = NULL;

    // For simpler tests
    if( p->weaponowned[wp_shotgun] )  weapon |= WB_SHOT;
    if( p->weaponowned[wp_supershotgun] )  weapon |= WB_SSG;
    if( p->weaponowned[wp_chaingun] )  weapon |= WB_CHAIN;
    if( p->weaponowned[wp_missile] )  weapon |= WB_ROCKET;
    if( p->weaponowned[wp_plasma] )  weapon |= WB_PLASMA;
    if( p->weaponowned[wp_bfg] )  weapon |= WB_BFG;
    if( p->ammo[am_shell] >= 2 )  ammo |= WB_SHOT | WB_SSG;
    if( p->ammo[am_clip] >= 4 )  ammo |= WB_CHAIN;
    if( p->ammo[am_misl] )  ammo |= WB_ROCKET;
    if( p->ammo[am_cell] >= 4 )  ammo |= WB_PLASMA | WB_BFG;
    if((p->readyweapon == wp_fist) || (p->readyweapon == wp_chainsaw))  out_of_ammo = 1;

#ifdef DEBUG_COME_HERE
    if( friend_summon )
    {
       GenPrintf( EMSG_debug, "Friend summon\n" );
    }
#endif

    //search through the list of all thinkers
    for( currentthinker = thinkercap.next; currentthinker != &thinkercap; currentthinker = currentthinker->next )
    {
        if (currentthinker->function == TFI_MobjThinker)
        {
            enemy_weight = 0;
            itemWeight = 0;  // initialize to no weight, best items have greatest weight
            mo = (mobj_t *)currentthinker;
            thingDistance = P_AproxDistance (pmo->x - mo->x, pmo->y - mo->y);

#ifdef DOGS
            if( (mo->type == MT_DOGS || ((mo->type == helper_MT) && (mo->flags & MF_FRIEND)) ) )
            {
                if( SAME_FRIEND( pmo, mo ) )
                    continue;
            }
#endif

            if((mo->flags & MF_COUNTKILL) || (mo->type == MT_SKULL) )
            {
#ifdef ENABLE_COME_HERE
                if( friend_summon )
                    continue;  // don't hunt enemy when called
#endif

                if( SAME_FRIEND( pmo, mo ) )
                {
                    // if not player, but is friend of this player
                    continue;  // ignore it (to fix bots killing dogs )
                }

                // Corpse may be solid, so check health.
                if( mo->health <= 0 )
                    continue;

                enemy_weight = mo->health | 128;  // estimate of importance
            }
            else if( (mo->type == MT_BARREL) || (mo->type == MT_POD) || (mo->flags & MF_TOUCHY) )
            {
#ifdef ENABLE_COME_HERE
                if( friend_summon )
                    continue;  // don't hunt barrel when called
#endif

                // lessen bot fixation with shooting barrels
                if((thingDistance > (80*FRACUNIT)) && !out_of_ammo)
                {
                    if( regulate(pmo, MT_BARREL, 4*TICRATE, 15*TICRATE ) )  // 0..255
                        enemy_weight = 64;  // fire 1/4 of time
                }
            }
            else if (mo->player)
            {
                if( p != mo->player)
                {
                    if( mo->health <= 0 )
                        continue;

                    if( friend_weight
                        && SAME_FRIEND( pmo, mo )
                        && thingDistance < (280*FRACUNIT) )
                    {
                        friend_weight = 200;  // leave items for friend
                    }

#ifdef ENABLE_COME_HERE
                    if( friend_summon && (mo == come_here_player)
                        && SAME_FRIEND( pmo, mo )
                        && thingDistance < FIXINT(1600) )
                    {
                        // Bot goes to the calling player.
                        pbot->teammate = come_here_player;
//                        furthestTeammateDistance = thingDistance + thingDistance;
//                        enemy_weight = 10;
                        pbot->bestSeenItem = NULL;
                        pbot->closestEnemy = NULL;
                        if( (pbot->flags & BF_avoid) && (pbot->what != WH_friend) )
                        {
                            pbot->flags &= ~BF_avoid;
                            pbot->avoidtimer = 0;
                        }
# ifdef DEBUG_COME_HERE			
                        GenPrintf(EMSG_debug, "COME_HERE Bot, come_here=%i\n", come_here );
# endif
                    }
#endif

                    if( deathmatch )
                    {
                        enemy_weight = 250;
                    }
                    else
                    {
                        if (B_Reachable(p, mo))	//i can reach this teammate
                        {
                            // Prefer teammate that is farthest away
                            if ((thingDistance > furthestTeammateDistance)
                                && (!pbot->teammate && !mo->player->bot))
                            {
                                furthestTeammateDistance = thingDistance;
                                pbot->teammate = mo;
                                //debug_Printf("found a teammate\n");
#ifdef BOT_NOT_REACHABLE
                                pbot->flags &= ~BF_not_reachable;
#endif
                            }
                        }
                        else //i can not reach this teammate
                        {
                            // Unseen teammate is actually unreachable.
                            SearchNode_t* tempNode = B_GetNodeAt(mo->x, mo->y);
                            if (tempNode
                                && (!closestUnseenTeammateDistance
                                    || ((thingDistance < closestUnseenTeammateDistance)
                                        && (!pbot->teammate
                                            || (!mo->player->bot
                                                && pbot->teammate->player->bot))))
                                )
                            {
                                closestUnseenTeammateDistance = thingDistance;
                                pbot->closestUnseenTeammate = mo;
                                //debug_Printf("found a teammate\n");
                            }
                        }
                    }
                }
            }
            else if (mo->flags & MF_MISSILE)	//is it a threatening missile
            {
                if (mo->target != pmo) //if its an enemies missile I had better avoid it
                {  // important: a missiles "target" is actually its owner...
                   // see if the missile is heading my way, if the missile will be closer to me, next tick
                   // then its heading at least somewhat towards me, so better dodge it
                    if (P_AproxDistance (pmo->x + pmo->momx - (mo->x + mo->momx), pmo->y + pmo->momy - (mo->y+mo->momy)) < thingDistance)
                    {
                        //if its the closest missile and its reasonably close I should try and avoid it
                        if (thingDistance
                            && (!closestMissileDistance || (thingDistance < closestMissileDistance))
                            && (thingDistance <= FIXINT(300)))
                        {
                            closestMissileDistance = thingDistance;
                            pbot->closestMissile = mo;
                        }
                    }
                    thingDistance = 0;
                }
            }
            else if (((mo->flags & MF_SPECIAL)
                      || (mo->flags & MF_DROPPED))) //most likely a pickup
            {
#ifdef ENABLE_COME_HERE
                if( friend_summon )
                    continue;  // don't hunt items when called
#endif

                item_getable = (mo->flags & MF_DROPPED) || item_respawn;
                if(EN_heretic)
                {
                    switch (mo->type)
                    {
//  HERETIC??? --> ///////// bonuses/powerups ////////////////////////
                     case MT_ARTIINVULNERABILITY:  //invulnerability, always run to get it
                        if( p->powers[pw_invulnerability] < thresh_power )
//                        if( deathmatch || !p->powers[pw_invulnerability])
                            itemWeight = 10;
                        break;
                     case MT_ARTIINVISIBILITY:	//invisability
                        if( p->powers[pw_invisibility] < thresh_power )
//                        if( deathmatch || !p->powers[pw_invisibility])
                            itemWeight = 9;
                        break;
                     case MT_ARTISUPERHEAL:	//soul sphere
                        if( p->health < thresh_soul_health )
//                        if( deathmatch || p->health < max_soul_health)
                            itemWeight = 8;
                        break;
                     case MT_ITEMSHIELD2:	//blue armour, if we have >= maxarmour, its impossible to get
                        if( (p->armorpoints < thresh_armor) && (friend_weight < 60) )
//                        if (p->armorpoints < max_armor)
                            itemWeight = 8;
                        break;
                     case MT_ITEMSHIELD1:	//green armour
                        if (p->armorpoints < (thresh_armor>>1))
//                        if (p->armorpoints < (max_armor/2))
                            itemWeight = 5;
                        break;
                     case SPR_MEDI: case SPR_STIM: //medication
                        if (health_index < 5)
                        {
                            // index by health_index
                            static const byte  stim_weight[5] = { 7, 6, 5, 4, 3 };
//                            static const byte  stim_weight[5] = { 6, 6, 5, 4, 3 };
                            itemWeight = stim_weight[ health_index ];
                        }
                        break;
                     case SPR_BON1:	//health potion
                        if (pmo->health < thresh_soul_health)
//                        if (pmo->health < max_soul_health)
                            itemWeight = 1;
                        break;
                     case SPR_BON2:	//armour bonus
                        if (p->armorpoints < thresh_armor)
//                        if (p->armorpoints < max_armor)
                            itemWeight = 1;
                        break;

        /////////////// weapons ////////////////////////////
                     case SPR_SHOT:
                        if (!p->weaponowned[wp_shotgun])
                        {
                            if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_ROCKET | WB_PLASMA) )
                                itemWeight = 4;
                            else
                                itemWeight = 6;
                        }
                        else if( item_getable
                                 && (p->ammo[am_shell] < p->maxammo[am_shell])
                                 )
                            itemWeight = 3;
                        break;
                     case SPR_MGUN:
                        if (!p->weaponowned[wp_chaingun])
                        {
                            if( weapon & ammo & (WB_SSG | WB_ROCKET | WB_PLASMA) )
                                itemWeight = 5;
                            else
                                itemWeight = 6;
                        }
                        else if( item_getable
                                 && (p->ammo[am_clip] < p->maxammo[am_clip]))
                            itemWeight = 3;
                        break;
                     case SPR_LAUN:
                        if (!p->weaponowned[wp_missile])
                        {
                            if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_PLASMA) )
                                itemWeight = 5;
                            else
                                itemWeight = 7;
                        }
                        else if( item_getable
                                 && (p->ammo[am_misl] < p->maxammo[am_misl]))
                            itemWeight = 3;
                        break;
                     case SPR_PLAS:
                        if (!p->weaponowned[wp_plasma])
                        {
                            if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_ROCKET) )
                                itemWeight = 5;
                            else
                                itemWeight = 7;
                        }
                        else if( item_getable
                                 && (p->ammo[am_cell] < p->maxammo[am_cell]))
                            itemWeight = 3;
                        break;
                     case SPR_BFUG:
                        if (!p->weaponowned[wp_bfg])
                        {
                            if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_ROCKET | WB_PLASMA) )
                                itemWeight = 5;
                            else
                                itemWeight = 7;
                        }
                        else if( item_getable
                                 && (p->ammo[am_cell] < p->maxammo[am_cell]))
                            itemWeight = 3;
                        break;
                     case SPR_SGN2:
                        if (!p->weaponowned[wp_supershotgun])
                        {
                            if( weapon & ammo & (WB_CHAIN | WB_ROCKET | WB_PLASMA) )
                                itemWeight = 5;
                            else
                                itemWeight = 7;
                        }
                        else if( item_getable
                                 && (p->ammo[am_shell] < p->maxammo[am_shell]))
                            itemWeight = 3;
                        break;

        /////////////////////ammo
                     case SPR_CLIP: case SPR_AMMO:
                        if( (p->ammo[am_clip]==0) && out_of_ammo )
                            itemWeight = 6;
                        else if( p->ammo[am_clip] < p->maxammo[am_clip])
                            itemWeight = 3;
                        break;
                     case SPR_SHEL: case SPR_SBOX:
                        if( (weapon & (WB_SHOT | WB_SSG) & ~ammo )  // shotgun without ammo
                            && out_of_ammo )
                            itemWeight = 6;
                        else if(p->ammo[am_shell] < p->maxammo[am_shell])
                            itemWeight = 3;
                        break;
                     case SPR_ROCK: case SPR_BROK:
                        if( (weapon & WB_ROCKET & ~ammo )  // launcher without ammo
                            && out_of_ammo )
                            itemWeight = 6;
                        else if(p->ammo[am_misl] < p->maxammo[am_misl])
                            itemWeight = 3;
                        break;
                     case SPR_CELL: case SPR_CELP:
                        if( (weapon & WB_PLASMA & ~ammo )  // plasma without ammo
                            && out_of_ammo )
                            itemWeight = 6;
                        else if(p->ammo[am_cell] < p->maxammo[am_cell])
                            itemWeight = 3;
                        break;

                     default:
                        itemWeight = 0;	// dont want it

                        if( friend_weight < 10 )
                        {
                            // Items that are only taken if not a friend.
                            switch (mo->type)
                            {
        ///////////////////////keys
                             case SPR_BKEY:
                                if (!(p->cards & it_bluecard))
                                    itemWeight = 5;
                                break;
                             case SPR_BSKU:
                                if (!(p->cards & it_blueskull))
                                    itemWeight = 5;
                                break;
                             case SPR_RKEY:
                                if (!(p->cards & it_redcard))
                                    itemWeight = 5;
                                break;
                             case SPR_RSKU:
                                if (!(p->cards & it_redskull))
                                    itemWeight = 5;
                                break;
                             case SPR_YKEY:
                                if (!(p->cards & it_yellowcard))
                                    itemWeight = 5;
                                break;
                             case SPR_YSKU:
                                if (!(p->cards & it_yellowskull))
                                    itemWeight = 5;
                                break;
                             default:
                                break;
                            }
                        }
                        break;
                    }
                }
                else switch (mo->sprite)
                {
//NON-HERETIC???////////// bonuses/powerups now checks for skill level
                 case SPR_PINV:	//invulnrability always run to get it
                    if( p->powers[pw_invulnerability] < thresh_power )
//                    if( deathmatch || !p->powers[pw_invulnerability])
                    {
                        // index by gameskill
                        static const byte  pinv_weight[5] = {2, 5, 6, 8, 10};
                        itemWeight = pinv_weight[ gameskill ];
                    }
                    break;
                 case SPR_MEGA: //megasphere
                    if( (p->health < thresh_soul_health || p->armorpoints < thresh_armor ) && (friend_weight < 35) )
//                    if( deathmatch
//                        || (p->health < max_soul_health || p->armorpoints < max_armor) )
                    {
                        static const byte  mega_weight[5] = {2, 4, 5, 7, 9};
                        itemWeight = mega_weight[ gameskill ];
                    }
                    break;
                 case SPR_PINS:	//invisibility
                    if( p->powers[pw_invisibility] < thresh_power )
//                    if( deathmatch || !p->powers[pw_invisibility] )
                    {
                        static const byte  pins_weight[5] = {2, 3, 5, 7, 9};
                        itemWeight = pins_weight[ gameskill ];
                    }
                    break;
                 case SPR_SOUL:	//soul sphere
                    if( ( p->health < thresh_soul_health ) && (friend_weight < 40))
//                    if( deathmatch || p->health < max_soul_health )
                    {
                        static const byte  soul_weight[5] = {1, 2, 4, 6, 9};
                        itemWeight = soul_weight[ gameskill ];
                    }
                    break;
                 case SPR_ARM2:	//blue armour, if we have >= maxarmour, its impossible to get
                    if( (p->armorpoints < thresh_armor) && (friend_weight < 60) )
//                    if (p->armorpoints < max_armor)
                    {
                        static const byte arm2_weight[5] = {1, 2, 4, 6, 8};
                        itemWeight = arm2_weight[ gameskill ];
                    }
                    break;
                 case SPR_PSTR:	//berserk pack
                    if (health_index < 5)
                    {
                        // index by gameskill, health test
                        static const byte  pstr_weight[5][5] =
                        {
                            {9, 9, 9, 8, 7},  // sk_baby
                            {9, 9, 8, 7, 6},  // sk_easy
                            {9, 8, 7, 6, 5},  // sk_medium
                            {9, 8, 7, 5, 4},  // sk_hard
                            {7, 6, 5, 4, 3}   // sk_nightmare
                        };
                        itemWeight = pstr_weight[ gameskill ][ health_index ];
                    }
                    else if( (p->powers[pw_strength] < thresh_power) && (friend_weight < 40))
//                    else if (!p->powers[pw_strength])
                        itemWeight = 2;
                    break;

                 case SPR_ARM1:	//green armour
                    if( p->armorpoints < thresh_armor/2 )
//                    if (p->armorpoints < max_armor/2)
                    {
                        static const byte arm1_weight[5] = {1, 2, 3, 4, 5};
                        itemWeight = arm1_weight[ gameskill ];
                    }
                    break;

                 case SPR_MEDI: case SPR_STIM: //medication  MEDIKIT or STIMPACK
                    if (health_index < 5)
                    {
                        // index by gameskill, health test
                        static const byte  medi_weight[5][5] =
                        {
                            {2, 2, 1, 1, 1},  // sk_baby
                            {3, 3, 2, 1, 1},  // sk_easy
                            {4, 4, 3, 2, 1},  // sk_medium
                            {5, 5, 4, 3, 2},  // sk_hard
                            {6, 6, 5, 4, 3}   // sk_nightmare
                        };
                        itemWeight = medi_weight[ gameskill ][ health_index ];
                    }
                    break;

                 case SPR_BON1:	//health potion
                    if (pmo->health < thresh_soul_health)
//                    if (pmo->health < max_soul_health)
                        itemWeight = 1;
                    break;
                 case SPR_BON2:	//armour bonus
                    if (p->armorpoints < thresh_armor)
//                    if (p->armorpoints < max_armor)
                        itemWeight = 1;
                    break;

/////////////// weapons ////////////////////////////				
                 case SPR_SHOT:
                    if (!p->weaponowned[wp_shotgun])
                    {
                        if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_ROCKET | WB_PLASMA) )
                            itemWeight = 4;
                        else
                            itemWeight = 6;
                    }
                    else if( item_getable
                             && (p->ammo[am_shell] < p->maxammo[am_shell]))
                        itemWeight = 3;
                    break;
                 case SPR_MGUN:
                    if (!p->weaponowned[wp_chaingun])
                    {
                        if( weapon & ammo & (WB_SSG | WB_ROCKET | WB_PLASMA) )
                            itemWeight = 5;
                        else
                            itemWeight = 6;
                    }
                    else if( item_getable
                             && (p->ammo[am_clip] < p->maxammo[am_clip]))
                        itemWeight = 3;
                    break;
                 case SPR_LAUN:
                    if (!p->weaponowned[wp_missile])
                    {
                        if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_PLASMA) )
                            itemWeight = 5;
                       else
                            itemWeight = 7;
                    }
                    else if( item_getable
                             && (p->ammo[am_misl] < p->maxammo[am_misl]))
                       itemWeight = 3;
                    break;
                 case SPR_PLAS:
                    if (!p->weaponowned[wp_plasma])
                    {
                        if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_ROCKET) )
                            itemWeight = 5;
                        else
                            itemWeight = 7;
                    }
                    else if( item_getable
                             && (p->ammo[am_cell] < p->maxammo[am_cell]))
                        itemWeight = 3;
                    break;
                 case SPR_BFUG:
                    if (!p->weaponowned[wp_bfg])
                    {
                        if( weapon & ammo & (WB_SSG | WB_CHAIN | WB_ROCKET | WB_PLASMA) )
                            itemWeight = 5;
                        else
                            itemWeight = 7;
                    }
                    else if( item_getable
                             && (p->ammo[am_cell] < p->maxammo[am_cell]))
                        itemWeight = 3;
                    break;
                 case SPR_SGN2:
                    if (!p->weaponowned[wp_supershotgun])
                    {
                        if( weapon & ammo & (WB_CHAIN | WB_ROCKET | WB_PLASMA) )
                            itemWeight = 5;
                        else
                            itemWeight = 7;
                    }
                    else if( item_getable
                             && (p->ammo[am_shell] < p->maxammo[am_shell]))
                        itemWeight = 3;
                    break;

/////////////////////ammo
                 case SPR_CLIP: case SPR_AMMO:
                    if( (p->ammo[am_clip] == 0) && out_of_ammo )
                        itemWeight = 6;
                    else if(p->ammo[am_clip] < p->maxammo[am_clip])
                        itemWeight = 3;
                    break;
                 case SPR_SHEL: case SPR_SBOX:
                    if( (weapon & (WB_SHOT | WB_SSG) & ~ammo )  // shotgun without ammo
                        && out_of_ammo )
                        itemWeight = 6;
                    else if(p->ammo[am_shell] < p->maxammo[am_shell])
                        itemWeight = 3;
                    break;
                 case SPR_ROCK: case SPR_BROK:
                    if( (weapon & WB_ROCKET & ~ammo )  // launcher without ammo
                        && out_of_ammo )
                        itemWeight = 6;
                    else if(p->ammo[am_misl] < p->maxammo[am_misl])
                        itemWeight = 3;
                    break;
                 case SPR_CELL: case SPR_CELP:
                    if( (weapon & (WB_PLASMA | WB_BFG) & ~ammo )  // plasma without ammo
                        && out_of_ammo )
                        itemWeight = 6;
                    else if(p->ammo[am_cell] < p->maxammo[am_cell])
                        itemWeight = 3;
                    break;

                 default:
                    itemWeight = 0;	//don't want it

                    if( friend_weight < 10 )
                    {
                        // Items that are only taken if not a friend.
                        switch (mo->type)
                        {
///////////////////////keys
                         case SPR_BKEY:
                            if (!(p->cards & it_bluecard))
                                itemWeight = 5;
                            break;
                         case SPR_BSKU:
                            if (!(p->cards & it_blueskull))
                              itemWeight = 5;
                            break;
                         case SPR_RKEY:
                            if (!(p->cards & it_redcard))
                              itemWeight = 5;
                            break;
                         case SPR_RSKU:
                            if (!(p->cards & it_redskull))
                              itemWeight = 5;
                            break;
                         case SPR_YKEY:
                            if (!(p->cards & it_yellowcard))
                              itemWeight = 5;
                            break;
                         case SPR_YSKU:
                            if (!(p->cards & it_yellowskull))
                              itemWeight = 5;
                            break;
                         default:
                            break;
                        }
                    }

                    break;
                }

                if (P_CheckSight(pmo, mo) && B_Reachable(p, mo))
                {
                    if (((itemWeight > bestSeenItemWeight)
                         || ((itemWeight == bestSeenItemWeight)
                             && (thingDistance < bestSeenItemDistance))))
                    {
#ifdef DEBUG_COME_HERE
                        if( friend_summon )
                            GenPrintf( EMSG_debug, "COME_HERE, Look sets bestSeenItem, #10\n" );
#endif

                        // Select this item.
                        bestSeenItem = mo;
                        bestSeenItemDistance = thingDistance;
                        bestSeenItemWeight = itemWeight;
                    }
                }
                else // this item is not getable atm, may use a search later to find a path to it
                {
                    SearchNode_t* tempNode = B_GetNodeAt(mo->x, mo->y);
                    // if there is a node near the item wanted, and its the best item
                    if (tempNode
                        // && ((P_AproxDistance(posX2x(tempNode->x) - mo->x, posY2y(tempNode->y) - mo->y) < (BOTNODEGRIDSIZE<<1))
                        && ((((itemWeight > bestItemWeight)
                              || ((itemWeight == bestItemWeight)
                                  && (thingDistance < bestItemDistance)))))
                        )
                    {
#ifdef DEBUG_COME_HERE
                        if( friend_summon )
                            GenPrintf( EMSG_debug, "COME_HERE, Look sets bestItem, #11\n" );
#endif
                        bestItem = mo;
                        bestItemDistance = thingDistance;
                        bestItemWeight = itemWeight;
                        //debug_Printf("best item set to x:%d y:%d for type:%d\n", FIXED_TO_INT(mo->x), FIXED_TO_INT(mo->y), mo->type);
                    }

                    //if (!tempNode)
                    //	debug_Printf("could not find a node here x:%d y:%d for type:%d\n", FIXED_TO_INT(mo->x), FIXED_TO_INT(mo->y), mo->type);
                }
            }  // hunt items

            // Reduce constant firing
            if( enemy_weight && (enemy_weight > B_Random()))
            {
#if 0
// FIXME, did not shoot at them
                if( ! B_Reachable(p, mo) ) // cannot reach this enemy, can still shoot at them
                    continue;
#endif

                if (P_CheckSight(pmo, mo))
                {
                    // if I have seen an enemy, if its deathmatch,
                    // players have priority, so closest player targeted
                    // otherwise make closest target the closest monster
                    if (thingDistance
                        && (!closestEnemyDistance || (thingDistance < closestEnemyDistance)
                            || (mo->player && !pbot->closestEnemy->player)))
                    {
#ifdef DEBUG_COME_HERE
                        if( friend_summon )
                            GenPrintf( EMSG_debug, "COME_HERE, Look sets closestEnemy, #12\n" );
#endif
                        closestEnemyDistance = thingDistance;
                        pbot->closestEnemy = mo;
                    }
                }
                else
                {
                    SearchNode_t* tempNode = B_GetNodeAt(mo->x, mo->y);
                    if (tempNode
                        && ((!closestUnseenEnemyDistance || (thingDistance < closestUnseenEnemyDistance)
                             || (mo->player && !pbot->closestUnseenEnemy->player))))
                    {
#ifdef DEBUG_COME_HERE
                        if( friend_summon )
                            GenPrintf( EMSG_debug, "COME_HERE, Look sets closestUnseenEnemy, #13\n" );
#endif
                        closestUnseenEnemyDistance = thingDistance;
                        pbot->closestUnseenEnemy = mo;
                    }
                }

                enemy_weight = 0;
                thingDistance = 0;
            }
        }
    }

    // if a item has a good weight, get it no matter what.
    // Else only if we have no target/enemy get it.
    pbot->bestSeenItem =
     ((bestSeenItemWeight > 5)
      || (bestSeenItemWeight && !pbot->closestEnemy)) ?
         bestSeenItem : NULL;

    pbot->bestItem = (bestItemWeight) ? bestItem : NULL;
}
