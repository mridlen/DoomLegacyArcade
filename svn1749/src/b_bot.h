// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_bot.h 1698 2024-11-27 07:19:15Z wesleyjohnson $
//
// Copyright (C) 2002 by DooM Legacy Team.
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
// $Log: b_bot.h,v $
// Revision 1.3  2002/09/28 06:53:11  tonyd
// fixed CR problem, fixed game options crash
//
// Revision 1.2  2002/09/27 16:40:07  tonyd
// First commit of acbot
//
//-----------------------------------------------------------------------------

#ifndef B_BOT_H
#define B_BOT_H

#include "doomtype.h"
#include "m_fixed.h"
#include "tables.h"
#include "p_mobj.h"
#include "b_search.h"

typedef enum {  // 16 bit
    BF_straferight   = 0x01,
    BF_backup        = 0x02,
    BF_avoid         = 0x04,  // avoid lastMobj
    BF_bump          = 0x08,
//    BF_avoid_dropoff = 0x08,  // uses lastMobj
//    BF_not_reachable = 0x10,  // enemy is not reachable, to avoid fall over cliff
} botflags_e;

typedef enum {
    WH_none, WH_friend, WH_enemy, WH_item, WH_line, WH_avoid,
    WH_unseen = 0x10,
} bot_what_e;

typedef struct
{
    // [WDJ] Ptrs first, for alignment.
    LinkedList_t * path;	//path to the best item on the map
    SearchNode_t * destNode;	//the closest node to where wants to go

    mobj_t * bestSeenItem,	//best item seen
           * bestItem,	//best item on map, not neccessarily seen
           * closestEnemy,
           * closestMissile,
           * closestUnseenEnemy,//goes towards this enemy if have nothing else todo
           * closestUnseenTeammate,//goes towards this teammate if have nothing else todo
           * lastMobj,	//last enemy
           * teammate;

    int		blockedcount,
                avoidtimer,	// if blocked by something, like a barrel, it will reverse, and try to get around it
                strafetimer,
                weaponchangetimer,
                runtimer;

    fixed_t	lastMobjX,	//where last enemy was seen
                lastMobjY;
   
    uint16_t    flags;          // strafe, and other flags
    byte	lastNumWeapons;	//used to check if got a new weapon
    byte        skill;          // skill of this bot
    byte        what;           // bot_what_e, mostly what lastMobj is
    byte        turn_timer;     // short timer
} bot_t;

#endif
