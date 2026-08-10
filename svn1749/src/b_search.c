// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_search.c 1695 2024-11-27 06:46:02Z wesleyjohnson $
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
// $Log: b_search.c,v $
// Revision 1.3  2002/09/28 06:53:11  tonyd
// fixed CR problem, fixed game options crash
//
// Revision 1.2  2002/09/27 16:40:08  tonyd
// First commit of acbot
//

#include "g_game.h"
#include "r_defs.h"
#include "p_local.h"
#include "m_random.h"
#include "r_main.h"
#include "z_zone.h"
#include "b_search.h"
#include "b_node.h"


LinkedList_t* B_LLCreate()
{
    LinkedList_t* ll = Z_Malloc (sizeof(LinkedList_t), PU_STATIC, NULL);

    ll->currentSize = 0;
    ll->first = ll->last = NULL;

    return ll;
}


void B_LLClear(LinkedList_t* list)
{
#ifdef SHOWBOTPATH
    SearchNode_t * tnp;
    while (!B_LLIsEmpty(list))
    {
        tnp = B_LLRemoveFirstNode(list);  // not empty, so should be one
        P_RemoveMobj(tnp->mo);
        Z_Free(tnp);
    }

#else
    while( !B_LLIsEmpty(list) )
        Z_Free( B_LLRemoveFirstNode(list) );  // not empty, so should be one
#endif
}


void B_LLDelete(LinkedList_t* list)
{
    //free(list);
    Z_Free(list);
}


#if 0
SearchNode_t* B_LLFindNode(SearchNode_t** closed, SearchNode_t* node)
{
    SearchNode_t* current = (*closed);

    if( !current ) // if its empty
        return NULL;

    while ((current != NULL) && (current != node))
        current = current->next;

    return current;
}
#endif


// insert first
void B_LLInsertFirstNode(LinkedList_t* list, SearchNode_t* newLink)
{
    if( newLink != NULL )
    {
        if( list->first == NULL )  // its empty
            list->last = newLink;
        else
            list->first->vprevious = newLink;

        newLink->vprevious = NULL;
        newLink->vnext = list->first;
        list->first = newLink;

        list->currentSize++;
    }
}


boolean B_LLIsEmpty(LinkedList_t* list)
{
    return (list->first == NULL);
}


#if 0
SearchNode_t* B_LLRemoveNode(SearchNode_t** closed, SearchNode_t* node)
{
    SearchNode_t * first = *closed;

    if( !first )	// if its empty
        return NULL;

    SearchNode_t * tnp = first;
    while (tnp != node)
    {
        tnp = tnp->next;
        if( tnp == NULL )
            return NULL;
    }

    if( tnp == first )  // if its actually the first element
        *closed = tnp->next;
    else
        tnp->previous->next = tnp->next;

    if (tnp->next != NULL)	// if its not the last element
        tnp->next->previous = tnp->previous;

    return tnp;
}
#endif


SearchNode_t* B_LLRemoveFirstNode(LinkedList_t* list)
{
    SearchNode_t* oldFirst = list->first;  // to be removed
    if( ! oldFirst )
        return NULL;  // should never happen

    list->first = oldFirst->vnext;  // new first entry
    if( oldFirst->vnext )
        oldFirst->vnext->vprevious = NULL;  // back link
    else  // was also last entry
        list->last = NULL;  // is empty list

    list->currentSize--;
    return oldFirst;
}


SearchNode_t* B_LLRemoveLastNode(LinkedList_t* list)
{
    SearchNode_t* oldLast = list->last;  // to be removed
    if( ! oldLast )
        return NULL;  // should never happen

    list->last = oldLast->vprevious;  // new last entry
    if( oldLast->vprevious )  // [WDJ] was wrong test, fixed 1.48.16
        oldLast->vprevious->vnext = NULL; // forward link
    else // was also first entry
        list->first = NULL;  // is now empty list

    list->currentSize--;
    return oldLast;
}


void B_NodePushSuccessors(PriorityQ_t* open, SearchNode_t* parent_node, SearchNode_t* dest)
{
    int cost, heuristic, f;
    int angle;  // bot angles, botdirtype_t

    SearchNode_t *node;

    for (angle=0; angle<NUMBOTDIRS; angle++)
    {
        if (parent_node->dir[angle])
        {
            node = parent_node->dir[angle];
            //sector_t * sector = R_PointInSubsector(posX2x(node->x), posY2y(node->y))->sector;

            //if (!sector->firsttag/* && !sector->nexttag */&& ((((sector->ceilingheight - sector->floorheight) < FIXINT(56))
            //|| (sector->floorheight - R_PointInSubsector(parent_node->x, parent_node->y)->sector->floorheight) > FIXINT(45)))) // can't fit
            //	continue;

            cost = parent_node->costDir[angle] + parent_node->cost;
            heuristic = P_AproxDistance(dest->x - node->x, dest->y - node->y) * 10000;
            // debug_Printf("got heuristic of %d\n", node->heuristic);
            f = cost + heuristic;

            if (node->visited		// if already been looked at before, and before look was better
                 && (node->f <= f))
                continue;

            if (B_PQFindNode(open, node))
            {
                // this node has already been pushed on the todo list
                if (node->f <= f)
                     continue;

                // the path to get here this way is better then the old one's
                // so use this instead, remove the old
                // debug_Printf("found better path\n");
                B_PQRemoveNode(open, node);
            }

            node->cost = cost;
            node->heuristic = heuristic;
            node->f = f;
            node->pprevious = parent_node;
            B_PQInsertNode(open, node);
            //debug_Printf("pushed node at x:%d, y:%d\n", FIXED_TO_INT(node->x), FIXED_TO_INT(node->y));
        }
    }
}

PriorityQ_t* B_PQCreatePQ(int mx)
{
    PriorityQ_t* pq = Z_Malloc (sizeof(PriorityQ_t), PU_LEVEL, NULL);
    pq->maxSize = mx;
    pq->currentSize = 0;
    pq->heapArray = Z_Malloc (((sizeof(SearchNode_t*))*mx), PU_LEVEL, NULL);

    return pq;
}

void B_PQDelete(PriorityQ_t* pq)
{
#if 0
    int i = 0;
    while (pq->currentSize)
    {
//	B_DeleteNode(B_PQRemoveNode(pq));
        i++;
    }
#endif
    Z_Free(pq->heapArray);
    Z_Free(pq);
}

SearchNode_t* B_PQFindNode(PriorityQ_t* pq, SearchNode_t* node)
{
    int tc = 0;

    if (pq->currentSize == 0)
        return NULL;

    while( pq->heapArray[tc] != node )
    {
        tc++;
        if ((tc >= pq->currentSize)
            || (pq->heapArray[tc] == NULL))
            return NULL;
    }

    return pq->heapArray[tc];
}

void B_PQInsertNode(PriorityQ_t* pq, SearchNode_t* newNode)
{
    pq->heapArray[pq->currentSize] = newNode;
    B_PQTrickleUp(pq, pq->currentSize++);
}

SearchNode_t* B_PQRemoveNode(PriorityQ_t* pq, SearchNode_t* node)
{
    int  i;
    int  tc = 0;
    SearchNode_t*  foundNode;

    if (!pq->currentSize)	// if its empty
        return NULL;

    while( pq->heapArray[tc] != node )
    {
        tc++;
        if (pq->heapArray[tc] == NULL)
            return NULL;
    }

    foundNode = pq->heapArray[tc];

    pq->heapArray[tc] = pq->heapArray[--pq->currentSize];
    for (i = pq->currentSize/2 - 1; i >= 0; i--)
        B_PQTrickleDown(pq, i);

    return foundNode;
}


SearchNode_t* B_PQRemoveFirstNode(PriorityQ_t* pq)
{
    SearchNode_t* root = pq->heapArray[0];
    pq->heapArray[0] = pq->heapArray[--pq->currentSize];
    B_PQTrickleDown(pq, 0);

    return root;
}


void B_PQTrickleDown(PriorityQ_t* pq, int index)
{
    int smallerChild;
    SearchNode_t* top = pq->heapArray[index];        // save root

    while(index < pq->currentSize/2)        // not on bottom row
    {
        int leftChild = 2*index+1;
        int rightChild = leftChild+1;
        // find smaller child
        smallerChild = 
         (rightChild < pq->currentSize
          && pq->heapArray[leftChild]->f < pq->heapArray[rightChild]->f ) ?
              leftChild
            : rightChild;
        // top >= largerChild?
        if( top->f <= pq->heapArray[smallerChild]->f )
            break;
        // shift child up
        pq->heapArray[index] = pq->heapArray[smallerChild];
        index = smallerChild;             // go down
    }  // end while
    pq->heapArray[index] = top;             // root to index
}


void B_PQTrickleUp(PriorityQ_t* pq, int index)
{
    int parent = (index - 1)/2;
    SearchNode_t* bottom = pq->heapArray[index];

    while( (index > 0) && (pq->heapArray[parent]->f > bottom->f) )
    {
        pq->heapArray[index] = pq->heapArray[parent];
        index = parent;
        parent = (parent - 1)/2;
    }

    pq->heapArray[index] = bottom;
}


boolean B_FindNextNode(player_t* p)
{
    boolean found = false;
//    int num_nodes_searched = 0;  // unused
    bot_t * bp = p->bot;

    PriorityQ_t	 * open;
    SearchNode_t * bestNode = NULL;  // if cant reach favourite item try heading towards this closest point
    LinkedList_t * visitedList;

    SearchNode_t* closestnode = B_GetNodeAt(p->mo->x, p->mo->y); // (B_FindClosestNode(p->mo->x, p->mo->y);


    if( closestnode )
     // || P_AproxDistance(p->mo->x - closestnode->x, p->mo->y - closestnode->y) > (BOTNODEGRIDSIZE<<1)) // no nodes can get here
    {
        B_LLClear(bp->path);

        open = B_PQCreatePQ(numbotnodes);
        visitedList = B_LLCreate();

        // debug_Printf("closest node found is x:%d, y:%d\n", FIXED_TO_INT(closestnode->x), FIXED_TO_INT(closestnode->y));
        closestnode->pprevious = NULL;
        closestnode->cost = 0;
        closestnode->f = closestnode->heuristic =
            P_AproxDistance (closestnode->x - bp->destNode->x,
                             closestnode->y - bp->destNode->y) * 10000;

        B_PQInsertNode(open, closestnode);
        // while there are nodes left to check
        while (open->currentSize && !found)
        {
            SearchNode_t * tnp = B_PQRemoveFirstNode(open);  // grab the best node
            //debug_Printf("doing node a node at x:%d, y:%d\n", FIXED_TO_INT(tnp->x), FIXED_TO_INT(tnp->y));
            // if have found a/the node closest to the thing
            if( tnp == bp->destNode )
            {  // I have found the sector where I want to get to
                bestNode = tnp;
                found = true;
            }
            else
            {
                if( !bestNode || (tnp->heuristic < bestNode->heuristic) )
                    bestNode = tnp;

                B_NodePushSuccessors(open, tnp, bp->destNode);

                if( !tnp->visited )
                {
                    // so later can set this back to not visited
                    B_LLInsertFirstNode(visitedList, tnp);
                    tnp->visited = true;
                }
            }
//            num_nodes_searched++;
        }

        // reset all visited nodes to not visited
        while (visitedList->first)
            B_LLRemoveFirstNode(visitedList)->visited = false;

        if (bestNode && (bestNode != closestnode))
        {
            bestNode->pnext = NULL;
            // find the first node taken on way
            while (bestNode->pprevious != NULL)
            {
                SearchNode_t * np = Z_Malloc(sizeof(SearchNode_t),PU_STATIC,0);
                np->x = bestNode->x;
                np->y = bestNode->y;
#ifdef SHOWBOTPATH
                np->mo = P_SpawnMobj(posX2x(np->x), posY2y(np->y), R_PointInSubsector(posX2x(np->x), posY2y(np->y))->sector->floorheight, MT_MISC49);
#endif
                B_LLInsertFirstNode(bp->path, np);
                bestNode = bestNode->pprevious;
            }

            found = true;
        }

        B_PQDelete(open);
        B_LLDelete(visitedList);
    }
    //else
    //	CONS_Printf("Bot is stuck here x:%d y:%d\n", FIXED_TO_INT(p->mo->x), FIXED_TO_INT(p->mo->y));

    return found;
}
