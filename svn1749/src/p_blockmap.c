//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1999 id Software, Chi Hoang, Lee Killough, Jim Flynn,
//                   Rand Phares, Ty Halderman
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2017 Fabian Greffrath
// Copyright(C) 2020 by DooM Legacy Team
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
// DESCRIPTION:
// [crispy] Create Blockmap
// [MB] 2020-05-13: Description of blockmap lump format:
//      https://doomwiki.org/wiki/Blockmap
//

#include "doomincl.h"    // [MB] 2020-05-12: Added for I_Error()

#ifdef GENERATE_BLOCKMAP

#include "p_local.h"
#include "r_state.h"
#include "z_zone.h"
#include "command.h"

//#include "i_system.h"  // [MB] 2020-05-12: I_Realloc() is now here

#include <stdint.h>      // [MB] 2020-05-13: Added for C99 integer data types
#include <stdlib.h>
#include <string.h>      // [MB] 2020-05-12: Added for memset()



#if 0
// Not needed
// I_Realloc
/*
 * [MB] 2020-05-12: Ported from Crispy Doom 5.8.0 (src/i_system.c)
 * - Reject zero new size (would be implementation defined behaviour)
 * - Use (unsigned long) for I_Error() (size is not a pointer)
 */
static void *I_Realloc(void *ptr, size_t size)
{
    void *new_ptr = NULL;

    if (0 == size)
        I_Error("I_Realloc: Failed on zero new size");
    else
    {
        new_ptr = realloc(ptr, size);
        if (NULL == new_ptr)
            I_Error("I_Realloc: Failed on reallocation of %lu bytes",
                    (unsigned long) size);
    }

    return new_ptr;
}
#endif


// [crispy] taken from mbfsrc/P_SETUP.C:547-707, slightly adapted
// [WDJ] More adapted.

// blocklist structure
typedef struct
{
   uint32_t * list;
   int num_lines;
   int num_alloc;
} bmap_t;

/*
 *  [MB] 2020-05-12: Ported from Crispy Doom 5.8.0 (src/doom/p_blockmap.c)
 *  - Change indentation to 4 SPs (matching DooM Legacy style)
 *  - Replace blockmap with blockmapindex (int32_t* => uint32_t*)
 *    Global pointer to header of the blockmap lump
 *  - Replace blockmaplump with blockmaphead (int32_t* => uint32_t*)
 *    Global pointer to beginning of the part containing the offsets
 *  - Added typecasts for unsigned target types
 */
// Return blockmaphead = NULL on fail.
void  P_create_blockmap(void)
{
    fixed_t minx = INT_MAX, miny = INT_MAX;
    fixed_t maxx = INT_MIN, maxy = INT_MIN;

    // Find limits of map
    uint32_t i1;
    for( i1=0 ; i1<numvertexes ; i1++ )
    {
        vertex_t * vi = & vertexes[i1];

        register int vix = vi->x >> FRACBITS;
        if(vix < minx)       minx = vix;
        else if(vix > maxx)  maxx = vix;

        register int viy = vi->y >> FRACBITS;
        if(viy < miny)       miny = viy;
        else if(viy > maxy)  maxy = viy;
    }

    // [crispy] doombsp/DRAWING.M:175-178
    minx -= 8; miny -= 8;
    maxx += 8; maxy += 8;

    // Save blockmap parameters
    bmaporgx = minx << FRACBITS;
    bmaporgy = miny << FRACBITS;
    bmapwidth  = ((maxx-minx) >> MAPBTOFRAC) + 1;
    bmapheight = ((maxy-miny) >> MAPBTOFRAC) + 1;

    blockmaphead = NULL;

    // Compute blockmap, which is stored as a 2d array of variable-sized lists
    //
    // Pseudocode:
    //
    // For each linedef:
    //
    //   Map the starting and ending vertices to blocks.
    //
    //   Starting in the starting vertex's block, do:
    //
    //     Add linedef to current block's list, dynamically resizing it.
    //
    //     If current block is the same as the ending vertex's block,
    //     exit loop.
    //
    //     Move to an adjacent block by moving towards the ending block in
    //     either the x or y direction, to the block which contains the
    //     linedef.
    {
        uint32_t  blktot = bmapwidth * bmapheight;        // size of blockmap
        bmap_t * bmap = calloc(sizeof *bmap, blktot);     // array of blocklists
        // calloc inits the bmap memory to 0.

        uint32_t ln;
        for( ln=0; ln < numlines; ln++ )
        {
            line_t * lp = & lines[ln];
            int x, y, adx, ady;
            int dx, dy, diff, blkn, blkend;

            // starting blockmap coordinates
            x = (lp->v1->x >> FRACBITS) - minx;
            fixed_t xbf = x >> MAPBTOFRAC;
            y = (lp->v1->y >> FRACBITS) - miny;
            fixed_t ybf = y >> MAPBTOFRAC;

            // x-y deltas
            adx = lp->dx >> FRACBITS;
            dx = adx < 0 ? -1 : 1;
            ady = lp->dy >> FRACBITS;
            dy = ady < 0 ? -1 : 1;

            // difference in preferring to move across y (>0) instead of x (<0)
            diff = (adx == 0)?  1
                 : (ady == 0)? -1
             :  ( (xbf << MAPBTOFRAC)
                    + ((dx > 0)? MAPBLOCKUNITS-1 : 0) - x )
                      * (ady = abs(ady)) * dx
              - ( (ybf << MAPBTOFRAC)
                    + ((dy > 0)? MAPBLOCKUNITS-1 : 0) - y )
                      * (adx = abs(adx)) * dy;

            // starting block, and pointer to its blocklist structure
            blkn = (ybf * bmapwidth) + xbf;

            // ending block
            blkend = (((lp->v2->y >> FRACBITS) - miny) >> MAPBTOFRAC) *
                bmapwidth +
                    (((lp->v2->x >> FRACBITS) - minx) >> MAPBTOFRAC);

            // delta for pointer when moving across y
            dy *= bmapwidth;

            // deltas for diff inside the loop
            adx <<= MAPBTOFRAC;
            ady <<= MAPBTOFRAC;

            // Now we simply iterate block-by-block until we reach the end block
            while (((uint32_t) blkn) < blktot)    // failsafe -- should ALWAYS be true
            {
                bmap_t * bm = & bmap[blkn];

                // Increase size of allocated list if necessary
                if( bm->num_lines >= bm->num_alloc )
                {
                    size_t req_num = bm->num_alloc + 16;
                    uint32_t * nbp = realloc( bm->list, req_num * sizeof(uint32_t) );
                    if( nbp == NULL )
                       goto memory_fail;  // graceful recovery

                    bm->list = nbp;
                    bm->num_alloc = req_num;
                }

                // Add linedef to end of list
                bm->list[ bm->num_lines++ ] = ln;

                // If we have reached the last block, exit
                if(blkn == blkend)
                    break;

                // Move in either the x or y direction to the next block
                if (diff < 0)
                {
                    diff += ady;
                    blkn += dx;
                }
                else
                {
                    diff -= adx;
                    blkn += dy;
                }
            }
        }

        // Compute the total size of the blockmap.
        //
        // Compression of empty blocks is performed by reserving two offset
        // words at blktot and blktot+1.
        //
        // 4 words, unused if this routine is called, are reserved at the start.
        {
            // we need at least 1 word per block, plus reserved's
            uint32_t count = blktot + 6;

            uint32_t i;
            for (i = 0; i < blktot; i++)
            {
                if(bmap[i].num_lines)
                {
                    // 1 header word + 1 trailer word + blocklist
                    count += bmap[i].num_lines + 2;
                }
            }

            // Allocate blockmap lump with computed count
            blockmaphead = Z_Malloc(sizeof(*blockmaphead) * count, PU_LEVEL, 0);
        }

        // Compress the blockmap.
        {
            bmap_t * bp = bmap;     // Start of uncompressed blockmap
            uint32_t ndx0 = blktot + 4;   // Advance index to start of linedef lists
            uint32_t ndx = ndx0;

            // empty blockmap at ndx0
            blockmaphead[ndx++] = 0;  // Store an empty blockmap list at start
            blockmaphead[ndx++] = (uint32_t)INT32_C(-1); // (For compression)

            uint32_t i;
            for (i = 4; i < ndx0; i++, bp++)
            {	     
                if( bp->list ) // Non-empty blocklist
                {
                    // Store index & header
                    blockmaphead[i] = ndx;
                    blockmaphead[ndx++] = 0;
                    int j;		   
                    for( j = bp->num_lines - 1; j >= 0; j-- )
                    {
                        blockmaphead[ndx++] = (uint32_t) bp->list[j];
                    }
                    // Store trailer
                    blockmaphead[ndx++] = (uint32_t)INT32_C(-1);

                    free(bp->list);  // Free linedef list
                    bp->list = NULL;
                }
                else
                {
                    // Empty blocklist: point to reserved empty blocklist
                    blockmaphead[i] = ndx0;
                }
            }
        }

memory_fail:
        // [MB] 2020-05-13: Moved outside of last nested block to make it more
        //                  obvious that the free() is always executed
#if 1
	{
          uint32_t i;
          for( i=0; i < blktot; i++ )
          {
            bmap_t * bp = & bmap[i];
            // Free the linedef list
            free(bp->list);
            bp->list = NULL;
          }
	}
#endif
        free(bmap);  // Free uncompressed blockmap
    }

    if( blockmaphead )
    {
#if 1
        // [MB] 2020-05-13: Populate blockmap lump header
        // Currently DooM Legacy 1.48 does not use this header. Maybe useful for
        // debugging.
        blockmaphead[0] = bmaporgx>>FRACBITS;  // x coordinate of grid origin
        blockmaphead[1] = bmaporgy>>FRACBITS;  // y coordinate of grid origin
        blockmaphead[2] = bmapwidth;           // Number of columns
        blockmaphead[3] = bmapheight;          // Number of rows
#endif
       
        blockmapindex = & blockmaphead[4];

        // [crispy] copied over from P_LoadBlockMap()
        // [MB] 2020-05-13: Modified to match "clear out mobj chains" of DooM Legacy
        int count = sizeof(*blocklinks) * bmapwidth * bmapheight;

        blocklinks = Z_Malloc(count, PU_LEVEL, 0);
        memset(blocklinks, 0, count);
    }
}

#endif  // GENERATE_BLOCKMAP
