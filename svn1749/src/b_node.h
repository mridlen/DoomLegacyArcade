// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_node.h 1652 2023-11-14 09:10:51Z wesleyjohnson $
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
// $Log: b_node.h,v $
// Revision 1.3  2002/09/28 06:53:11  tonyd
// fixed CR problem, fixed game options crash
//
// Revision 1.2  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
//-----------------------------------------------------------------------------

#ifndef B_NODE_H
#define B_NODE_H

//#define SHOWBOTPATH	//show the path the bot is taking in game?

#include "doomtype.h"
#include "m_fixed.h"
#include "p_mobj.h"

//#define BOTNODEGRIDSIZE	/*8388608		//128<<16	*/2097152		//32<<16
// BOT NODE GRID SIZE 32<<16
#define BOTNODEGRIDSIZE   0x00200000
#define posX2x(a) (((a) + xOffset)*BOTNODEGRIDSIZE)
#define posY2y(a) (((a) + yOffset)*BOTNODEGRIDSIZE)

typedef enum
{
    BDI_EAST,
    BDI_NORTHEAST,
    BDI_NORTH,
    BDI_NORTHWEST,
    BDI_WEST,
    BDI_SOUTHWEST,
    BDI_SOUTH,
    BDI_SOUTHEAST,
    BDI_TELEPORT,
    NUMBOTDIRS

} botdirtype_t;

typedef struct SearchNode_s  SearchNode_t;
typedef struct SearchNode_s
{
    boolean  visited;

    fixed_t  costDir[NUMBOTDIRS];	//the cost of going from this node in a particular dest

    fixed_t  cost,
             f,
             heuristic,
             x, y;

    SearchNode_t * pprevious,
                 * pnext,
                 * vnext,
                 * vprevious,
                 * dir[NUMBOTDIRS];

#ifdef SHOWBOTPATH
    mobj_t * mo;
#endif
} SearchNode_t;

extern SearchNode_t***    botNodeArray;
extern int  numbotnodes;
extern int  xOffset, yOffset;
extern int  xSize, ySize;

//boolean B_PTRPathTraverse (intercept_t *in);
SearchNode_t* B_FindClosestNode(fixed_t x, fixed_t y);
SearchNode_t* B_GetNodeAt(fixed_t x, fixed_t y);
SearchNode_t* B_GetClosestReachableNode(fixed_t x, fixed_t y);
boolean B_NodeReachable(mobj_t* mo, fixed_t x, fixed_t y, fixed_t destx, fixed_t desty);
void B_Init_Nodes(void);

#endif

