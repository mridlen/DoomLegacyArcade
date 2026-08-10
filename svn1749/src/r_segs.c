// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_segs.c 1691 2024-11-27 06:36:07Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: r_segs.c,v $
// Revision 1.32  2003/05/04 04:15:09  sburke
// Wrap patch->width, patch->height references in SHORT for big-endian machines.
//
// Revision 1.31  2002/09/25 16:38:35  ssntails
// Alpha support for trans 3d floors in software
//
// Revision 1.30  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.29  2001/08/29 18:58:57  hurdler
//
// Revision 1.28  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.27  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.26  2001/05/27 13:42:48  bpereira
//
// Revision 1.25  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.24  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.23  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.22  2001/02/24 13:35:21  bpereira
// Revision 1.21  2000/11/26 01:02:27  hurdler
// Revision 1.20  2000/11/25 18:41:21  stroggonmeth
//
// Revision 1.19  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.18  2000/11/14 16:23:16  hurdler
// Revision 1.17  2000/11/09 17:56:20  stroggonmeth
// Revision 1.16  2000/11/03 03:27:17  stroggonmeth
// Revision 1.15  2000/11/02 19:49:36  bpereira
//
// Revision 1.14  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.13  2000/09/28 20:57:17  bpereira
// Revision 1.12  2000/04/30 10:30:10  bpereira
// Revision 1.11  2000/04/18 17:39:40  stroggonmeth
// Revision 1.10  2000/04/16 18:38:07  bpereira
// Revision 1.9  2000/04/15 22:12:58  stroggonmeth
//
// Revision 1.8  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.7  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.6  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.5  2000/04/05 15:47:47  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------

#include <stddef.h>

#include "doomincl.h"
#include "r_local.h"
#include "r_sky.h"

#include "r_splats.h"           //faB: testing

#include "w_wad.h"
#include "z_zone.h"
#include "d_netcmd.h"
#include "p_local.h" //Camera...
#include "console.h" //Con_clipviewtop



// #define DEBUG_STOREWALL   

// Light added for wall orientation (in 0..255 scale)
#define ORIENT_LIGHT   16

// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
static boolean         segtextured;
static boolean         markfloor; // False if the back side is the same plane.
static boolean         markceiling;

static boolean         maskedtexture;
// maskedtexture can use transparent patches
// Only single-sided linedefs use midtexture, 2-sided sets maskedtexture instead.

// toptexture, bottomtexture, midtexture will call draw routines that do not
// look for the post structure of patches.  They assume a full column of pixels,
// without transparent areas, such as TM_picture.
// They can use TM_combine or TM_patch only where there is a single full
// height post per column.
// Violation of this (by the wad) will give tutti-frutti colors.

// texture num, 0=no-texture, otherwise is a valid texture index
static int             toptexture;
static int             bottomtexture;
static int             midtexture;  // single-sided only
static texture_render_t * top_texren = NULL;
static texture_render_t * mid_texren = NULL;
static texture_render_t * bottom_texren = NULL;


static int             numthicksides;
//static short*          thicksidecol;


angle_t         rw_normalangle;
// angle to line origin
int             rw_angle1;
fixed_t         rw_distance;

//
// regular wall
//
static int             rw_x;
static int             rw_stopx;
static angle_t         rw_centerangle;
static fixed_t         rw_offset;
static fixed_t         rw_offset2; // for splats

static fixed_t         rw_scale;
static fixed_t         rw_scalestep;
static fixed_t         rw_midtexturemid;
static fixed_t         rw_toptexturemid;
static fixed_t         rw_bottomtexturemid;

#ifndef TEXTURE_LOCK
static int             rw_texture_num;  // to restore texture cache
#endif

// [WDJ] 2/22/2010 actually is fixed_t in all usage
static fixed_t         worldtop;	// front sector
static fixed_t         worldbottom;
static fixed_t         worldbacktop;	// back sector, only used on two sided lines
static fixed_t         worldbackbottom;

// RenderSegLoop global parameters
static fixed_t         pixhigh;
static fixed_t         pixlow;
static fixed_t         pixhighstep;
static fixed_t         pixlowstep;

static fixed_t         topfrac;
static fixed_t         topstep;

static fixed_t         bottomfrac;
static fixed_t         bottomstep;

lighttable_t**  walllights;  // array[] of colormap selected by lightlevel

int16_t  *  maskedtexturecol;


// Define  colfunc_2s_t
typedef  void (* colfunc_2s_t) (byte *);

#ifdef RANGECHECK
static byte  rangecheck_id;
static const char *  rangecheck_draw_name[] = { "top", "mid", "bottom" };
#endif


// [WDJ] For the 1-sided wall drawer, which only wants a monolithic column of pixels.
// This is only used in three places.
// The previous code, with a +3 offset in the column offsets (for all textures), was not maintainable.
// This is a specialized usage.  The burden of an additional lookup and add should be here
// and not upon the other hacked-up operations that were needed.
// It also allows this to be inline, which saves on the call overhead.

// [WDJ] Setup wall drawer, outside of column loops.
// Can afford to create cached textures here, and other overhead.
static inline
texture_render_t *  R_WallTexture_setup( int texture_num )
{
    texture_render_t * texren;

    if( textures[ texture_num ] == NULL )
         return NULL;

    texren = & texture_render[ texture_num ];
   
    // The texture must be setup for monolithic column draw.
    // Cannot use masked draw because it does not tile on walls.
    if( texren->cache == NULL
        || ! (texren->detect & TD_1s_ready) )
    {
        if( texren->detect & TD_masked )
        {
            // Some masked draw is using the patch, so cannot convert this texren to a picture.
            // Switch to one of the extra texren to make a picture format.
            texren = R_Get_extra_texren( texture_num, texren, TM_picture, TD_1s_ready );
            if( texren->cache
                && (texren->detect & TD_1s_ready) )
                goto done;  // found existing
        }

        // Texture cache may have been freed by other operations using Z_Malloc.
        // This drawer has fixed requirements, and other drawers will not tile.
        // Puts ptr in texture_render cache.
        R_GenerateTexture2( texture_num, texren, TM_picture_column );
        texren->detect |= TD_wall;  // protect
    }

done:   
#ifdef TEXTURE_LOCK
    Z_ChangeTag( texren->data, PU_IN_USE );
#endif
    return texren;  // OK to draw
}


// [WDJ] Draw code only works when patch has one post monolithic columns, or picture.
// This causes visual artifacts for some wads with strange patches, that work for other ports.
// Forcing the texture into a picture results in turning transparent areas into black,
// so that must be kept separate from texren used by masked draw.
// This functions handles the gory details.
static
void  R_Draw_WallColumn( texture_render_t * texren, int colnum )
{
    // Does not know if TM_picture or TM_patch.
    byte * data = texren->cache;
   
#ifdef TEXTURE_LOCK
#else
    if( ! data )
    {
        // This must be here because cache can be freed by other operations.
        // To prevent must lock individual texture cache on every draw.
        // Messy update, but probably never executed.
        // The detect used to select texren would not have changed
        // during SegLoop, so do not try to change texren.
        R_GenerateTexture2( rw_texture_num, texren, TM_picture_column );
    }
#endif

    if( texren->width_tile_mask )
        colnum &= texren->width_tile_mask;  // set by load textures
    else
    {
        // Odd width texture, cannot just mask.
        // Sometime gets colnum = -1 or = width, even without tiling.
        // Test LostCiv, Map 20, crates.
        colnum = ( colnum < 0 )?
            texren->width - (((-colnum - 1) % texren->width) + 1)
          : colnum % texren->width;
    }
    data += texren->columnofs[colnum];  // column data

    // Draw as monolithic column, ignore posts.
    dc_source = data + texren->pixel_data_offset;

#ifdef RANGECHECK_DRAW_LIMITS
    // Temporary check code.
    // Due to better clipping, this extra clip should no longer be needed.
    if( dc_yl < 0 )
    {
        printf( "RenderSeg %s: dc_yl  %i < 0\n", rangecheck_draw_name[rangecheck_id], dc_yl );
        dc_yl = 0;
    }
    if ( dc_yh >= rdraw_viewheight )
    {
        printf( "RenderSeg %s: dc_yh  %i > rdraw_viewheight\n", rangecheck_draw_name[rangecheck_id], dc_yh );
        dc_yh = rdraw_viewheight - 1;
    }
#endif

#ifdef CLIP2_LIMIT
    //[WDJ] phobiata.wad has many views that need clipping
    if( dc_yl < 0 )   dc_yl = 0;
    if( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
#endif

#ifdef HORIZONTALDRAW
    hcolfunc ();
#else
    colfunc ();
#endif
}


// R_GetPatchColumn
// Masked draw, full patches, 2sided.
static inline
byte* R_GetPatchColumn ( texture_render_t * texren, int colnum )
{
    // Get raw column ptr, one cache array structure.
    byte * data = texren->cache;

#ifdef TEXTURE_LOCK
#else
    if( !data )
    {
        // This must be here because cache can be freed by other operations.
        // To prevent must lock individual texture cache on every draw.
        data = R_GenerateTexture( texren, TM_masked );
    }
#endif

    if( texren->width_tile_mask )
    {
        // width is power of 2
        colnum &= texren->width_tile_mask; // set by load textures
    }
    else
    {
        // [WDJ] Similar to PrBoom, will handle negative colnum, and odd widths.
        // PrBoom: while( colnum < 0 )   colnum += width;
        // Equiv:  colnum + (width * n)  for some n.
        // Note: (x % width) ===  x - (width * n) for some n, result 0 .. width-1.
        // Given: colnum = -1 .. -width, result width-1 .. 0, which is colnum + width.
        // Consider: -((z-colnum) % width) + z  ==>  colnum + (width * n)
        // Boundary: (0 % width) + z = width-1  ==>  z = width-1
        int width = texren->width;
        colnum = ( colnum < 0 )?
              width - ( (-1 - colnum) % width ) - 1
            : colnum % width ;
    }
    return data + texren->columnofs[colnum];
}





// ==========================================================================
// Allocate draw_ffside, draw_ffplane.
// These are variable sized, so cannot use free list.  Create from memory pool.
// Keep ptr aligned.
// Should be greater than  (MAXFFLOORS + 10)
#define  DRAWMEM_BLOCK_NUM_PTR  512
#if DRAWMEM_BLOCK_NUM_PTR < (MAXFFLOORS * 4)
# error "DRAWMEM_BLOCK_NUM_PTR  too small"
#endif

typedef struct drawmem_block_s  drawmem_block_t;
typedef struct drawmem_block_s {
   drawmem_block_t *  next;
   unsigned int free_units;
   void * mem[ DRAWMEM_BLOCK_NUM_PTR ];
} drawmem_block_t;

static drawmem_block_t *  drawmem_pool = NULL;

#define DRAWMEM_ALIGN_MASK  (sizeof(void*) - 1)

// Allocated is always a multiple of sizeof(void*).
// Maximum req_units is  MAXFFLOORS + 3   (approx 43)
static
void *  alloc_drawmem( int req_units )
{
    // Search allocated blocks for one with enough memory.
    // Search the latest block first.
    drawmem_block_t *  blkp = drawmem_pool;
    while( blkp )
    {
        if( blkp->free_units >= req_units )
           goto found;

        blkp = blkp->next;
    }

    // Create another pool
    blkp = (drawmem_block_t*) malloc( sizeof(drawmem_block_t) );
    if( blkp == NULL )
        I_Error( "Drawmem memory pool cannot allocate.\n" );

    // Link into memory pool.
    blkp->free_units = DRAWMEM_BLOCK_NUM_PTR;
    blkp->next = drawmem_pool;
    drawmem_pool = blkp;

found:
   {
      // Allocate the memory from blkp.
      void *  memptr = & blkp->mem[ DRAWMEM_BLOCK_NUM_PTR - blkp->free_units ];
      blkp->free_units -= req_units;
      return memptr;
   }
}


static
void R_Clear_drawmem( void )
{
    drawmem_block_t *  blkp = drawmem_pool;
    while( blkp )
    {
        blkp->free_units = DRAWMEM_BLOCK_NUM_PTR;
        blkp = blkp->next;
    }
}

static
draw_ffplane_t *  create_draw_ffplane( int num_req )
{
    static const int aug_num_req = (sizeof(draw_ffplane_t) + DRAWMEM_ALIGN_MASK) & ~DRAWMEM_ALIGN_MASK;
    draw_ffplane_t * dffp = (draw_ffplane_t *) alloc_drawmem( aug_num_req + num_req );
    dffp->numffloorplanes = num_req;
    return dffp;
}

static
draw_ffside_t *  create_draw_ffside( int num_req )
{
    static const int aug_num_req = (sizeof(draw_ffside_t) + DRAWMEM_ALIGN_MASK) & ~DRAWMEM_ALIGN_MASK;
    draw_ffside_t * dffs = (draw_ffside_t *) alloc_drawmem( aug_num_req + num_req );
    dffs->numthicksides = num_req;
    return dffs;
}


// ==========================================================================
// Backscale memory for sprites vrs floors.
// BSR: BACKSCALE_REF
// This saves cache space, especially as most of the backscale arrays in the drawsegs were unused.
// On average, there is 1/2 block left over allocation,
// but there is 1/2 vid.width wastage on each block (for odd size vidwidths).
// Starts at 64 firstsegs, and increases only when there are more firstsegs.
#define  BSR_ALLOC  (MAXVIDWIDTH*64)

typedef struct  bsr_mem_s {
    struct bsr_mem_s *  next;
    fixed_t  mem_block[ BSR_ALLOC ];
} bsr_mem_t;

bsr_mem_t *  bsr_mem_head = NULL;  // All bsr_mem
bsr_mem_t *  bsr_mem_freelist = NULL;  // Tail of bsr_mem_head list that has not been used.
// Allocation available in current freelist block.
fixed_t * bsr_free_array = NULL;
int       bsr_free_count = 0;

// Get ptr to array of backscale, covering the range.
static
fixed_t * R_get_backscale_ref( int num )
{
    fixed_t * bsr;

    // Allocate from bsr_mem_freelist.
    if( num > bsr_free_count )
    {
        // Try next block
        if( bsr_mem_freelist && bsr_mem_freelist->next )
        {
            // Already have memory block link.
            bsr_mem_freelist = bsr_mem_freelist->next;
        }
        else
        {
            // Allocate some more bsr memory.
            bsr_mem_t * bm = (bsr_mem_t*) malloc( sizeof(bsr_mem_t) );
            if( bm == NULL )  goto memory_fail;
            bm->next = NULL;
            // link
            if( bsr_mem_head == NULL )
            {
                bsr_mem_head = bm;  // very first allocation block
            }
            else if( bsr_mem_freelist )
            {
                bsr_mem_freelist->next = bm;
            }
            bsr_mem_freelist = bm;
        }
        // Use the new bsr memory block.
        bsr_free_array = & bsr_mem_freelist->mem_block[0];
        bsr_free_count = BSR_ALLOC;
    }
    // Allocate num, from the bsr_free_array.
    bsr = bsr_free_array;
    bsr_free_count -= num;
    bsr_free_array += num;
    return  bsr;

memory_fail:
    // Fail so that player can still save game.
    I_SoftError( "Backscale out of memory\n" );
    return  bsr_mem_head->mem_block;  // reuse
}

void R_Clear_backscale_ref( void )
{
    // If there are unused allocations, then release one.
    if( bsr_mem_freelist )
    {
        // But never release first one, as that was being used.
        bsr_mem_t * lmf = bsr_mem_freelist->next;  // first unused
        if( lmf )
        {
            // Free one unused bsr memory allocation.
            bsr_mem_freelist->next = lmf->next;
            free( lmf );
        }
    }

    // Keep rest of allocations, just put them on free list.
    // Put freelist at head of all allocations.
    bsr_mem_freelist = bsr_mem_head;
    if( bsr_mem_freelist )
    {
        bsr_free_count = BSR_ALLOC;
        bsr_free_array = & bsr_mem_freelist->mem_block[0];
    }
    else
    {
        // Have empty freelist
        bsr_free_count = 0;
        bsr_free_array = NULL;
    }
}

// ==========================================================================
// DrawSegs
//faB:  very ugly realloc() of drawsegs at run-time, I upped it to 512
//      instead of 256.. and someone managed to send me a level with 896 drawsegs!
//      So too bad here's a limit removal …-la-Boom
//Hurdler: with Legacy 1.43, drawseg_t is 6780 bytes and thus if having 512 segs, it will take 3.3 Mb of memory
//         default is 128 segs, so it means nearly 1Mb allocated
// Drawsegs set by R_StoreWallRange, used by R_Create_DrawNodes
// [WDJ] DrawSeg no longer includes backscale array, so got smaller.
uint16_t     max_drawsegs;    // number allocated
drawseg_t  * drawsegs = NULL;  // allocated drawsegs
drawseg_t  * ds_p = NULL;    // last drawseg used (tail)
// drawseg_t  * firstnewseg = NULL;  // unused

extern drawseg_t * first_subsec_seg;


static void  R_Clear_pool16( void );

//
// R_Clear_DrawSegs
//
// Called by R_RenderPlayerView
void R_Clear_DrawSegs (void)
{
    ds_p = drawsegs;
    R_Clear_backscale_ref();
    R_Clear_drawmem();
    
    R_Clear_pool16();
}

static
void expand_drawsegs( void )
{
    // drawsegs is NULL on first execution
    // Realloc larger drawseg memory, and adjust old drawseg ptrs
    // Gcc 12.3 compiler warning, that cannot accept testing array ptr
    // after realloc, because it might be freed.
    // It is a bogus warning because the realloc is never for 0 size,
    // and it does not dereference the freed memory anyway.
//    drawseg_t * old_drawsegs = drawsegs;
    ptrdiff_t  drawsegs_diff = (void*)ds_p - (void*)drawsegs;  // avoid using drawsegs after realloc
    unsigned newmax = max_drawsegs ? max_drawsegs*2 : 128;
    drawseg_t * new_drawsegs = realloc(drawsegs, newmax*sizeof(*drawsegs));
    if( new_drawsegs == NULL )
    {
        I_Error( "Failed realloc for drawsegs\n" );
    }
    drawsegs = new_drawsegs;
    max_drawsegs = newmax;
    // Adjust ptrs by adding the difference in drawseg area position
    // [WDJ] Avoid divide and mult by sizeof(drawsegs) by using void* difference
    // If NULL, then point to drawsegs after first alloc.
//    ptrdiff_t  drawsegs_diff = (void*)new_drawsegs - (void*)old_drawsegs;
    drawsegs_diff += (void*)new_drawsegs - (void*)ds_p;
    if( drawsegs_diff == 0 )
        return;
    ds_p = (drawseg_t*)((void*)ds_p + drawsegs_diff);
//    firstnewseg = (drawseg_t*)((void*)firstnewseg + drawsegs_diff);
    if (first_subsec_seg)  // if NULL then keep it NULL
        first_subsec_seg = (drawseg_t*)((void*)first_subsec_seg + drawsegs_diff);
}



// ==========================================================================
// R_Splats Wall Splats Drawer
// ==========================================================================

#ifdef WALLSPLATS
#define BORIS_FIX
#ifdef BORIS_FIX
static short last_ceilingclip[MAXVIDWIDTH];
static short last_floorclip[MAXVIDWIDTH];
#endif

// Called by R_DrawWallSplats
static void R_DrawSplatColumn (column_t* column)
{
    fixed_t     top_post_sc, bottom_post_sc;  // fixed_t screen coord.

    // dc_x is limited to 0..rdraw_viewwidth by caller x1,x2
#ifdef RANGECHECK_DRAW_LIMITS
    if ( (unsigned) dc_x >= rdraw_viewwidth )
    {
        I_SoftError ("R_DrawSplatColumn: dc_x= %i\n", dc_x);
        return;
    }
#endif

#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif
    
    // over all column posts for this column
    for ( ; column->topdelta != 0xff ; )
    {
        // calculate unclipped screen coordinates
        //  for post
        top_post_sc = dm_top_patch + dm_yscale*column->topdelta;
        bottom_post_sc = top_post_sc + dm_yscale*column->length;

        // fixed_t to int screen coord.
        dc_yl = (top_post_sc+FRACUNIT-1)>>FRACBITS;
        dc_yh = (bottom_post_sc-1)>>FRACBITS;

#ifdef BORIS_FIX
        if (dc_yh > last_floorclip[dc_x])
            dc_yh =  last_floorclip[dc_x];
        if (dc_yl < last_ceilingclip[dc_x])
            dc_yl =  last_ceilingclip[dc_x];
#else
        if (dc_yh > dm_floorclip[dc_x])
            dc_yh = dm_floorclip[dc_x];
        if (dc_yl < dm_ceilingclip[dc_x])
            dc_yl = dm_ceilingclip[dc_x];
#endif

#ifdef RANGECHECK_DRAW_LIMITS
    // Temporary check code.
    // Due to better clipping, this extra clip should no longer be needed.
    if( dc_yl < 0 )
    {
        printf( "DrawSplat dc_yl  %i < 0\n", dc_yl );
        dc_yl = 0;
    }
    if ( dc_yh >= rdraw_viewheight )
    {
        printf( "DrawSplat dc_yh  %i >= rdraw_viewheight\n", dc_yh );
        dc_yh = rdraw_viewheight - 1;
    }
#endif

#ifdef CLIP2_LIMIT
        //[WDJ] phobiata.wad has many views that need clipping
        if ( dc_yl < 0 ) dc_yl = 0;
        if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
#endif
        if (dc_yl <= dc_yh && dc_yh >= 0 )
        {
            dc_source = (byte *)column + 3;
            dc_texturemid = dm_texturemid - (column->topdelta<<FRACBITS);
            
            //debug_Printf("l %d h %d %d\n",dc_yl,dc_yh, column->length);
            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            colfunc ();
        }
        column = (column_t *)(  (byte *)column + column->length + 4);
    }
}


// Draw splats for a lineseg.
// Caller sets frontsector.
static void R_DrawWallSplats (void)
{
    wallsplat_t*    splat;
//    seg_t*      seg;
    angle_t     angle1, angle2;
    int         angf;
    int         x1, x2;
    column_t*   col;
    patch_t*    patch;
    lighttable_t  * ro_colormap = NULL;  // override colormap
    fixed_t     texturecolumn;

    splat = (wallsplat_t*) linedef->splats;

#ifdef PARANOIA
    if (!splat)
        I_Error ("R_DrawWallSplats: splat is NULL");
#endif

//    seg = ds_p->curline;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif
    // [WDJ] Fixed determinations, taken out of draw loop.
    // Overrides of colormap, with same usage.
    if( fixedcolormap )
        ro_colormap = fixedcolormap;
    else if( view_colormap )
        ro_colormap = view_colormap;
    else if( frontsector->extra_colormap )  // over the whole line
        ro_colormap = frontsector->extra_colormap->colormap;

    // draw all splats from the line that touches the range of the seg
    for ( ; splat ; splat=splat->next)
    {
        angle1 = R_PointToAngle (splat->v1.x, splat->v1.y);
        angle2 = R_PointToAngle (splat->v2.x, splat->v2.y);
#if 0
        if (angle1>clipangle)
            angle1=clipangle;
        if (angle2>clipangle)
            angle2=clipangle;
        if ((int)angle1<-(int)clipangle)
            angle1=-clipangle;
        if ((int)angle2<-(int)clipangle)
            angle2=-clipangle;
        int angf1 = ANGLE_TO_FINE(angle1 - viewangle + ANG90);
        int angf2 = ANGLE_TO_FINE(angle2 - viewangle + ANG90);
#else
        int angf1 = ANGLE_TO_FINE(angle1 - viewangle + ANG90);
        int angf2 = ANGLE_TO_FINE(angle2 - viewangle + ANG90);
        // BP: out of the viewangle_to_x lut, TODO clip it to the screen
        if( angle1 > FINE_ANG180 || angle2 > FINE_ANG180)
            continue;
#endif
        // viewangle_to_x table is limited to (0..rdraw_viewwidth)
        x1 = viewangle_to_x[angf1];
        x2 = viewangle_to_x[angf2];

        if (x1 >= x2)
            continue;                         // smaller than a pixel

        // splat is not in this seg range
        if (x2 < ds_p->x1 || x1 > ds_p->x2)
            continue;

        if (x1 < ds_p->x1)
            x1 = ds_p->x1;
        if (x2 > ds_p->x2)
            x2 = ds_p->x2;
        if( x2<=x1 )
            continue;

        // calculate incremental stepping values for texture edges
        rw_scalestep = ds_p->scalestep;
        dm_yscale = ds_p->scale1 + (x1 - ds_p->x1)*rw_scalestep;
        dm_floorclip = floorclip;
        dm_ceilingclip = ceilingclip;

        patch = W_CachePatchNum (splat->patch, PU_CACHE); // endian fix

        // clip splat range to seg range left
        /*if (x1 < ds_p->x1)
        {
            dm_yscale += (rw_scalestep * (ds_p->x1 - x1));
            x1 = ds_p->x1;
        }*/
        // clip splat range to seg range right


        // SoM: This is set already. THIS IS WHAT WAS CAUSING PROBLEMS WITH
        // BOOM WATER!
        // frontsector = ds_p->curline->frontsector;

        // [WDJ] FIXME: ?? top of texture, plus 1/2 height ?, relative to viewer
        // So, either splat->top is not top, or something else weird.
        // This is world coord.
        dm_texturemid = splat->top + (patch->height<<(FRACBITS-1)) - viewz;
        if( splat->yoffset )
            dm_texturemid += *splat->yoffset;
        // R_DrawSplatColumn uses dm_texturemid, and generates dc_texturemid

        // top of splat, screen coord.
        dm_top_patch = centeryfrac - FixedMul(dm_texturemid, dm_yscale);

        // set drawing mode for single sided textures
        switch (splat->flags & SPLATDRAWMODE_MASK)
        {
            case SPLATDRAWMODE_OPAQUE:
                colfunc = basecolfunc;  // R_DrawColumn_x
                break;
            case SPLATDRAWMODE_TRANS:
                if( cv_translucency.value == 0 )
                    colfunc = basecolfunc;  // R_DrawColumn_x
                else
                {
                    dr_alpha = 128;  // use normal dc_translucent_index
                    dc_translucent_index = TRANSLU_med;
                    dc_translucentmap = & translucenttables[ TRANSLU_TABLE_med ];
                    colfunc = transcolfunc;  // R_DrawTranslucentColumn_x
                }
    
                break;
            case SPLATDRAWMODE_SHADE:
                colfunc = shadecolfunc;  // R_DrawShadeColumn_x
                break;
        }

        dc_texheight = 0;

        // draw the columns
        // x1,x2 are already limited to 0..rdraw_viewwidth
        for (dc_x = x1 ; dc_x <= x2 ; dc_x++, dm_yscale += rw_scalestep)
        {
            if( !fixedcolormap )
            {
                // distance effect on light, yscale is smaller at distance.
                unsigned  dlit = dm_yscale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                   dlit = MAXLIGHTSCALE-1;

                dc_colormap = walllights[dlit];
                if( ro_colormap )
                {
                    // reverse indexing, and change to extra_colormap
                    int lightindex = dc_colormap - reg_colormaps;
                    dc_colormap = & ro_colormap[ lightindex ];
                }
            }

            dm_top_patch = centeryfrac - FixedMul(dm_texturemid, dm_yscale);
            dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

            // find column of patch, from perspective
            angf = ANGLE_TO_FINE(rw_centerangle + x_to_viewangle[dc_x]);
            texturecolumn = rw_offset2 - splat->offset - FixedMul(finetangent[angf],rw_distance);

            //texturecolumn &= 7;
            //DEBUG

            // FIXME !
//            CONS_Printf ("%.2f width %d, %d[x], %.1f[off]-%.1f[soff]-tg(%d)=%.1f*%.1f[d] = %.1f\n", 
//                         FIXED_TO_FLOAT(texturecolumn), patch->width,
//                         dc_x,FIXED_TO_FLOAT(rw_offset2),FIXED_TO_FLOAT(splat->offset),angf,FIXED_TO_FLOAT(finetangent[angf]),FIXED_TO_FLOAT(rw_distance),FIXED_TO_FLOAT(FixedMul(finetangent[angf],rw_distance)));
            texturecolumn >>= FRACBITS;
            if (texturecolumn < 0 || texturecolumn >= patch->width) 
                continue;

            // draw the texture
            col = (column_t *) ((byte *)patch + patch->columnofs[texturecolumn]);
            R_DrawSplatColumn (col);

        }

    }// next splat

    colfunc = basecolfunc;
}

#endif //WALLSPLATS



// ==========================================================================
// Lightlist and Openings
// [WDJ] separate functions for expand of lists, with error handling

void  expand_lightlist( void )
{
    struct r_lightlist_s *  newlist = 
        realloc(dc_lightlist, sizeof(r_lightlist_t) * dc_numlights);

    if( newlist )
    {
        dc_lightlist = newlist;
        dc_maxlights = dc_numlights;
    }
    else
    {
        // non-fatal protection, allow savegame
        // realloc fail does not disturb existing allocation
        dc_numlights = dc_maxlights;
        I_SoftError( "Expand lightlist realloc failed.\n" );
    }
}


#if 1
// [WDJ] Clipping and opening pool of int16_t
// There is no need for a contiguous memory pool.
// Eliminate all moving of allocated memory, eliminate searching for ptrs.
// This allows uses of pool16 from outside the drawsegs.

// All pool16 memory allocations are of the same size.
#if MAXVIDWIDTH < 1030
# define POOL16_ALLOC  (MAXVIDWIDTH * 64)
#else
# define POOL16_ALLOC  (MAXVIDWIDTH * 128)
#endif

typedef struct pool16_s pool16_t;
typedef struct pool16_s
{
    pool16_t *  next;
    int16_t  data[0];  // variable size array
} pool16_t;

static pool16_t * pool16_head = NULL; // all pool16_t
static pool16_t * pool16_use = NULL;  // current pool16_t
static int16_t  * pool16_data = NULL;
static int16_t  * pool16_end = NULL;  // last data + 1


static
void  pool16_setup_use( pool16_t * p )
{
    pool16_use = p;
    pool16_data = & p->data[0];
    pool16_end =  & p->data[ POOL16_ALLOC ];  // all pool16 are same size
}

static
void  R_Clear_pool16( void )
{
    pool16_data = NULL;
    pool16_end =  NULL;
    pool16_use = pool16_head;
    if( pool16_head )
    {
        pool16_setup_use( pool16_head );
    }
}

//  size : number of int16_t in the array
int16_t *  get_pool16_array( int size )
{
    // in units of int16_t
    if( ! pool16_data
        || ((pool16_data + size) > pool16_end) )
    {
        if( pool16_use )
        {
            // Try next unused pool16.
            pool16_use = pool16_use->next;
        }

        if( ! pool16_use )
        {
            // Allocate another pool16.
            pool16_use = malloc( sizeof(pool16_t) + (sizeof(int16_t) * POOL16_ALLOC) );
            if( pool16_use == NULL )
                I_Error( "Pool16 allocation failed\n" );

            // Link it at the tail, because the unused are at the tail.
            pool16_use->next = NULL;
            if( pool16_head )
            {
                pool16_t * tail = pool16_head;
                while( tail->next )   tail = tail->next;
                tail->next = pool16_use;
            }
            else
            {
                pool16_head = pool16_use;
            }
        }

        pool16_setup_use( pool16_use );
    }

    // Allocate from pool16_use.
    int16_t * p = pool16_data;
    pool16_data += size;  // units of int16_t
    return p;
}


// To preallocate for drawsegs.  This pool16 does not adjust drawsegs.
//  need : number of int16 needed
static inline
void pool16_need( int need )
{
}


#else
// Old expand_openings method.  Deprecated.
//SoM: 3/23/2000: Use boom opening limit removal
static size_t  maxopenings = 0;
static int16_t * openings = NULL;
static int16_t * lastopening = NULL;

static
void  R_Clear_pool16( void )
{
    lastopening = openings;
}

static
void  expand_openings( size_t  need )
{
    size_t lastindex = lastopening - openings;
    drawseg_t *ds;  //needed for fix from *cough* zdoom *cough*
    uintptr_t  adjustdiff;

    if( maxopenings < 1024 )
        maxopenings = 16384;
    while (need > maxopenings)
        maxopenings *= 2;
    int16_t * newopenings = realloc(openings, maxopenings * sizeof(*openings));
    if( newopenings == NULL )
    {
        I_Error( "Failed realloc for openings\n" );
    }
    adjustdiff = (void*)newopenings - (void*)openings; // byte difference in locations
   
    // borrowed fix from *cough* zdoom *cough*
    // [RH] We also need to adjust the openings pointers that
    //    were already stored in drawsegs.
    for (ds = drawsegs; ds < ds_p; ds++)
    {
#define ADJUST(p) if (ds->p + ds->x1 >= openings && ds->p + ds->x1 <= lastopening)\
                        ds->p = ((void*) ds->p) + adjustdiff;
        ADJUST (maskedtexturecol);
        ADJUST (spr_topclip);
        ADJUST (spr_bottomclip);
        if( ds->draw_ffside )
          ADJUST (draw_ffside->thicksidecol);
    }
#undef ADJUST
    openings = newopenings;
    lastopening = & openings[ lastindex ];
}

int16_t *  get_pool16_array( int size )
{
    // in units of int16_t
    if( (lastopening + size) > (openings + maxopenings) )
        expand_openings( size * 4 );

    int16_t * p = lastopening;
    lastopening += size;  // units of int16_t
    return p;
}

// To preallocate for drawsegs, because adjust cannot cope with a partially filled drawseg.
//  need : number of int16 needed
static inline
void pool16_need( int need )
{
    //SoM: Code to remove limits on openings.
    size_t lastindex = lastopening - openings;
    size_t needindex = need + lastindex;
    if (needindex > maxopenings)  expand_openings( needindex );
} 

#endif




// ==========================================================================
// R_RenderMaskedSegRange
// ==========================================================================

// If we have a multi-patch texture on a 2sided wall (rare) then we draw
//  it using R_DrawColumn, else we draw it using R_DrawMaskedColumn.
//  This way we don't have to store or process extra post_t info with each column
//  for multi-patch textures. They are not normally needed as multi-patch
//  textures don't have holes in it. At least not for now.
static int  column2s_length;     // column->length : for multi-patch on 2sided wall = texture->height

// The colfunc_2s function for TM_picture
static
void R_Render_PictureColumn ( byte * column_data )
{
    fixed_t  top_post_sc, bottom_post_sc; // patch on screen, fixed_t screen coords.

    if ( (unsigned) dc_x >= rdraw_viewwidth )   return;
   
    top_post_sc = dm_top_patch; // + dm_yscale*column->topdelta;  topdelta is 0 for the wall
    bottom_post_sc = top_post_sc + dm_yscale * column2s_length;

    // set y bounds to patch bounds, unless there is window
    dc_yl = (dm_top_patch+FRACUNIT-1)>>FRACBITS;
    dc_yh = (bottom_post_sc-1)>>FRACBITS;

    if(dm_windowtop != FIXED_MAX && dm_windowbottom != FIXED_MAX)
    {
      dc_yl = ((dm_windowtop + FRACUNIT) >> FRACBITS);
      dc_yh = (dm_windowbottom - 1) >> FRACBITS;
    }

    {
      if(dc_yh > dm_floorclip[dc_x])
          dc_yh =  dm_floorclip[dc_x];
      if(dc_yl < dm_ceilingclip[dc_x])
          dc_yl =  dm_ceilingclip[dc_x];
    }

    // [WDJ] Draws only within borders
    if (dc_yl >= rdraw_viewheight || dc_yh < 0)
      return;

#ifdef RANGECHECK_DRAW_LIMITS
    // Temporary check code.
    // Due to better clipping, this extra clip should no longer be needed.
    if( dc_yl < 0 )
    {
        printf( "DrawMulti dc_yl  %i < 0\n", dc_yl );
        dc_yl = 0;
    }
    if ( dc_yh >= rdraw_viewheight )
    {
        printf( "DrawMulti dc_yh  %i > rdraw_viewheight\n", dc_yh );
        dc_yh = rdraw_viewheight - 1;
    }
#endif
#ifdef CLIP2_LIMIT
    //[WDJ] phobiata.wad has many views that need clipping
    if ( dc_yl < 0 )   dc_yl = 0;
    if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
#endif
    if (dc_yl <= dc_yh)
    {
        // TM_picture format, do not have to adjust.
        dc_source = column_data;
        colfunc ();
    }
}



// indexed by texture_model_e
colfunc_2s_t  colfunc_2s_masked_table[] =
{
    NULL,  // TM_none
    R_DrawMaskedColumn,  // TM_patch, render the usual 2sided single-patch packed texture
    R_Render_PictureColumn,  // TM_picture, render multipatch with no holes (no post_t info)
    R_DrawMaskedColumn,  // TM_combine_patch, render combined as 2sided single-patch packed texture
      // The rest have no drawers
    NULL,  // TM_multi_patch
};


// [WDJ] Setup masked draw, outside of column loops.
// Can afford to create cached textures here, and other overhead.
static inline
texture_render_t *  R_MaskedDraw_setup( int texture_num )
{
    texture_render_t * texren;

    if( textures[ texture_num ] == NULL )
         return NULL;

    texren = & texture_render[ texture_num ];
   
    // The texture must be setup for monolithic column draw.
    // Cannot use masked draw because it does not tile on walls.
    if( texren->cache == NULL
        || ! (texren->detect & TD_2s_ready) )
    {
        if( texren->detect & TD_wall )
        {
            // Some wall draw is using the patch, so cannot convert this texren to a patch.
            // Switch to one of the extra texren to make a patch format.
            texren = R_Get_extra_texren( texture_num, texren, TM_masked, TD_2s_ready );
            if( texren->cache
                && (texren->detect & TD_2s_ready) )
                goto done;  // found existing
        }

        // Texture cache may have been freed by other operations using Z_Malloc.
        // Generate masked format, with holes, for 2sided draw.
        // Puts ptr in texture_render cache.
        R_GenerateTexture2( texture_num, texren, TM_masked );
        texren->detect |= TD_masked;  // protect
    }

done:   
#ifdef TEXTURE_LOCK
    Z_ChangeTag( texren->data, PU_IN_USE );
#endif
    
    return texren;  // OK to draw
}


// Render with fog, translucent, and transparent, over range x1..x2
// Called from R_Draw_Masked.
void R_RenderMaskedSegRange( drawseg_t* ds, int x1, int x2 )
{
    lightlev_t      vlight;  // visible light 0..255
    lightlev_t      orient_light = 0;  // wall orientation effect
    int             texnum;
    int             i;
    fixed_t	    windowclip_top, windowclip_bottom;
    fixed_t         lightheight, texheightz;
    fixed_t         realbot;
    // Setup lightlist for all 3dfloors, then use it over all x.
    r_lightlist_t * rlight;  // dc_lightlist
    ff_light_t    * ff_light;  // ffloor lightlist item
    lighttable_t  * ro_colormap = NULL;  // override colormap
    texture_render_t * texren;

    // TM_picture does not use column_t.
    // Each colfunc_2s for each format will have to convert type.
    byte *  col_data;
    void (*colfunc_2s) (byte*);

    line_t* ldef;   //faB

    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;

    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    // midtexture, 0=no-texture, otherwise valid
    texnum = texturetranslation[curline->sidedef->midtexture];
#if 1
    texren = R_MaskedDraw_setup( texnum );
#else
    texren = & texture_render[texnum];
    texren->detect |= TD_masked;  // to protect patch
#endif

    dm_windowbottom = dm_windowtop = dm_bottom_patch = FIXED_MAX; // default no clip
    windowclip_top = windowclip_bottom = FIXED_MAX;

    // Select the default, or special effect column drawing functions,
    // which are called by the colfunc_2s functions.

    //faB: hack translucent linedef types (201-205 for translucenttables 1-5)
    //SoM: 201-205 are taken... So I'm switching to 284 - 288
    ldef = curline->linedef;
    if (ldef->special>=284 && ldef->special<=288)  // Legacy translucents
    {
        dc_translucent_index = ldef->special-284+1;
        dc_translucentmap = & translucenttables[ ((ldef->special-284)<<FF_TRANSSHIFT) ];
        dr_alpha = 128; // use normal dc_translucent, all tables are < TRANSLU_REV_ALPHA
        colfunc = transcolfunc;
    }
    else
    if (ldef->special==260 || ldef->translu_eff )  // Boom make translucent
    {
        // Boom 260, make translucent, direct and by tags
        dc_translucent_index = TRANSLU_med; // 50/50
        dc_translucentmap = & translucenttables[TRANSLU_TABLE_INDEX(TRANSLU_med)]; // get transtable 50/50
        dr_alpha = 128;
        if( ldef->translu_eff >= TRANSLU_ext )
        {
            // Boom transparency map
            translucent_map_t * tm = & translu_store[ ldef->translu_eff - TRANSLU_ext ];
#if 0
            if( vid.drawmode == DRAW8PAL )
            {
                // dc_translucentmap only works on DRAW8PAL
                dc_translucentmap = W_CacheLumpNum( tm->translu_lump_num, PU_CACHE );
            }
#else
            // dc_translucentmap required for DRAW8PAL,
            // but is used indirectly in other draw modes, for some TRANSLU.
            dc_translucentmap = W_CacheLumpNum( tm->translu_lump_num, PU_CACHE );
#endif
            // for other draws
            dc_translucent_index = tm->substitute_std_translucent;
        }
        colfunc = transcolfunc;
    }
    else
    if (ldef->special==283)	// Legacy Fog sheet
    {
        // Display fog sheet (128 high) as transparent middle texture.
        // Only where there is a middle texture (in place of it).
        colfunc = fogcolfunc; // R_DrawFogColumn_8 16 ..
        fog_col_length = (texren->texture_model == TM_picture)? textures[texnum]->height : 2;
        fog_index = fog_tic % fog_col_length;  // pixel selection
        fog_init = 1;
        dr_alpha = 64;  // default
        // [WDJ] clip at ceiling and floor, unlike other transparent texture
        // world coord, relative to viewer
        windowclip_top = frontsector->ceilingheight - viewz;
        windowclip_bottom = frontsector->floorheight - viewz;
    }
    else
        colfunc = basecolfunc;

    // Select the 2s draw functions, they are called later.
    //faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
    //     are not stored per-column with post info anymore in Doom Legacy
    // [WDJ] multi-patch transparent texture restored
    if( texren->cache == NULL
        || ! (texren->detect & TD_2s_ready) )
    {
        R_GenerateTexture( texren, TM_masked ); // first time
    }

    colfunc_2s = colfunc_2s_masked_table[ texren->texture_model ];

    texheightz = textureheight[texnum];
    dc_texheight = texheightz >> FRACBITS;
    // For TM_picture, textures[texnum]->height
    column2s_length = dc_texheight;
    
    rw_scalestep = ds->scalestep;
    dm_yscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif

    // Setup lighting based on the presence/lack-of 3D floors.
    dc_numlights = frontsector->numlights;
    if( dc_numlights )
    {
      if(dc_numlights >= dc_maxlights)   expand_lightlist();

      // setup lightlist
      // highest light to lowest light, [0] is sector light at top
      for(i = 0; i < dc_numlights; i++)
      {
        // setup a lightlist entry
        ff_light = &frontsector->lightlist[i];
        rlight = &dc_lightlist[i];  // create in this list slot

        // fake floor light heights in screen coord.
        rlight->height = (centeryfrac) - FixedMul((ff_light->height - viewz), dm_yscale);
        rlight->heightstep = -FixedMul (rw_scalestep, (ff_light->height - viewz));
        rlight->lightlevel = *ff_light->lightlevel;
        rlight->extra_colormap = ff_light->extra_colormap;
        rlight->flags = ff_light->flags;

#if 0
        // [WDJ] When NOSHADE, the light is not used.
        // Questionable if really worth it, for the few times it could skip the light setup.
        if( ff_light->flags & FF_NOSHADE )
           continue; // next 3dfloor light
#endif

        if(rlight->flags & FF_FOG)
          vlight = rlight->lightlevel + extralight_fog;
        else if(rlight->extra_colormap && rlight->extra_colormap->fog)
          vlight = rlight->lightlevel + extralight_cm;
        else if(colfunc == transcolfunc)
          vlight = 255 + orient_light;
        else
          vlight = rlight->lightlevel + extralight + orient_light;

        rlight->vlightmap =
            (vlight < 0) ? scalelight[0]
          : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
          : scalelight[vlight>>LIGHTSEGSHIFT];

        // [WDJ] Fixed determinations, taken out of draw loop.
        // Overrides of colormap, with same usage.
        if( fixedcolormap )
            rlight->rcolormap = fixedcolormap;
        else if( view_extracolormap )
            rlight->extra_colormap = view_extracolormap;
      }  // for
    }
    else
    {
      // frontsector->numlights == 0
      if(colfunc == fogcolfunc) // Legacy Fog sheet
        vlight = frontsector->lightlevel + extralight_fog;
      else if(frontsector->extra_colormap && frontsector->extra_colormap->fog)
        vlight = frontsector->lightlevel + extralight_cm;
      else if(colfunc == transcolfunc)  // Translucent 
        vlight = 255 + orient_light;
      else
        vlight = frontsector->lightlevel + extralight + orient_light;

      walllights =
          (vlight < 0) ? scalelight[0]
        : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
        : scalelight[vlight>>LIGHTSEGSHIFT];
       
      // [WDJ] Fixed determinations, taken out of loop.
      // Overrides of colormap, with same usage.
      if( fixedcolormap )
        ro_colormap = fixedcolormap;
      else if( view_colormap )
        ro_colormap = view_colormap;
      else if( frontsector->extra_colormap )
        ro_colormap = frontsector->extra_colormap->colormap;
    }

    maskedtexturecol = ds->maskedtexturecol;

    dm_floorclip = ds->spr_bottomclip;
    dm_ceilingclip = ds->spr_topclip;

    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
        // highest floor
        dm_texturemid =
         (frontsector->floorheight > backsector->floorheight) ?
            frontsector->floorheight : backsector->floorheight;
        // top of texture, relative to viewer
        dm_texturemid = dm_texturemid + texheightz - viewz;
    }
    else
    {
        // lowest ceiling
        dm_texturemid =
         (frontsector->ceilingheight < backsector->ceilingheight) ?
           frontsector->ceilingheight : backsector->ceilingheight;
        // top of texture, relative to viewer
        dm_texturemid = dm_texturemid - viewz;
    }
    // top of texture, relative to viewer, with rowoffset, world coord.
    dm_texturemid += curline->sidedef->rowoffset;
    dc_texturemid = dm_texturemid; // for using R_Render_PictureColumn
     // R_DrawMaskedColumn uses dm_texturemid to make dc_texturemid

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;

    // draw the columns
    // [WDJ] x1,x2 are limited to 0..rdraw_viewwidth to protect [dc_x] access.
#ifdef RANGECHECK_DRAW_LIMITS
    if( x1 < 0 || x2 >= rdraw_viewwidth )
    {
        I_SoftError( "R_RenderMaskedSegRange: %i  %i\n", x1, x2);
        return;
    }
#endif
    if( x1 < 0 )  x1 = 0;
    if( x2 >= rdraw_viewwidth )  x2 = rdraw_viewwidth-1;
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
        int colnum = maskedtexturecol[ dc_x ];
        if( colnum != SHRT_MAX )
        {
          // if not masked
          // calculate 3Dfloor lighting
          if(dc_numlights)
          {
            // Where there are 3dfloors ...
            dm_bottom_patch = FIXED_MAX;
            // top/bottom of texture, relative to viewer, screen coord.
            dm_top_patch = dm_windowtop = (centeryfrac - FixedMul(dm_texturemid, dm_yscale));
            realbot = dm_windowbottom = FixedMul(texheightz, dm_yscale) + dm_top_patch;
            dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

            col_data = R_GetPatchColumn(texren, colnum);

            // top floor colormap, or fixedcolormap
            dc_colormap = dc_lightlist[0].rcolormap;

            // for each 3Dfloor light
            // highest light to lowest light, [0] is sector light at top
            for(i = 0; i < dc_numlights; i++)
            {
              rlight = &dc_lightlist[i];

              if((rlight->flags & FF_NOSHADE))
                continue; // next 3dfloor light

              if( !fixedcolormap )
              {
                 // distance effect on light, yscale is smaller at distance.
                 unsigned dlit = dm_yscale>>LIGHTSCALESHIFT;
                 if (dlit >=  MAXLIGHTSCALE )
                     dlit = MAXLIGHTSCALE-1;

                 // light table for the distance
                 rlight->rcolormap = rlight->vlightmap[dlit];
                 if( rlight->extra_colormap )
                 {
                     // reverse indexing, and change to extra_colormap
                     int lightindex = rlight->rcolormap - reg_colormaps;
                     rlight->rcolormap = & rlight->extra_colormap->colormap[ lightindex ];
                 }
              }

              rlight->height += rlight->heightstep;

              lightheight = rlight->height;
              if(lightheight <= dm_windowtop)
              {
                // above view window, just get the colormap
                dc_colormap = rlight->rcolormap;
                continue;  // next 3dfloor light
              }

              // actual drawing using col and colfunc, between 3Dfloors
              dm_windowbottom = lightheight;
              if(dm_windowbottom >= realbot)
              {
                // past bottom of view window
                dm_windowbottom = realbot;
                colfunc_2s( col_data );

                // Finish dc_lightlist height adjustments.
                // highest light to lowest light, [0] is sector light at top
                for(i++ ; i < dc_numlights; i++)
                {
                  rlight = &dc_lightlist[i];
                  rlight->height += rlight->heightstep;
                }
                goto next_x;
              }  // if( dm_windowbottom >= realbot )

              colfunc_2s( col_data );

              // for next draw, downward from this light height
              dm_windowtop = dm_windowbottom + 1;
              dc_colormap = rlight->rcolormap;
            } // for( dc_numlights )
            // draw down to sector floor
            dm_windowbottom = realbot;
            if(dm_windowtop < dm_windowbottom)
              colfunc_2s( col_data );

          next_x:
            dm_yscale += rw_scalestep;
            continue;  // next x
          }  // if( dc_numlights )


          // Where there are no 3Dfloors ...
          // calculate lighting for distance using dm_yscale
          if (!fixedcolormap)
          {
              // distance effect on light, yscale is smaller at distance.
              unsigned dlit = dm_yscale>>LIGHTSCALESHIFT;
              if (dlit >=  MAXLIGHTSCALE )
                 dlit = MAXLIGHTSCALE-1;

              // light table for the distance
              dc_colormap = walllights[dlit];
              if( ro_colormap )
              {
                 // reverse indexing, and change to extra_colormap
                 int lightindex = dc_colormap - reg_colormaps;
                 dc_colormap = & ro_colormap[ lightindex ];
              }
          } // fixedcolormap

          if( windowclip_top != FIXED_MAX )
          {
            // fog sheet clipping to ceiling and floor
            dm_windowtop = centeryfrac - FixedMul(windowclip_top, dm_yscale);
            dm_windowbottom = centeryfrac - FixedMul(windowclip_bottom, dm_yscale);
          }

          // top of texture, screen coord.
          dm_top_patch = centeryfrac - FixedMul(dm_texturemid, dm_yscale);
          dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

          // draw texture, as clipped
          col_data = R_GetPatchColumn(texren, colnum);
          colfunc_2s( col_data );

        } // if (maskedtexturecol[dc_x] != SHRT_MAX)
        dm_yscale += rw_scalestep;
    } // for( dx_x = x1..x2 )
    colfunc = basecolfunc;
}





//
// R_RenderThickSideRange
// Renders all the thick sides for 3dfloors.
//  x1, x2 : the x range of the seg, to be rendered
//  ffloor : the fake-floor whose thick side is to be rendered
// Called by R_DrawMasked.
void R_RenderThickSideRange( drawseg_t* ds, int x1, int x2, ffloor_t* ffloor)
{
    lightlev_t      vlight;  // visible light 0..255
    lightlev_t      orient_light = 0;  // wall orientation effect
    int             texnum;
    sector_t        tempsec;
    int             base_fog_alpha;
    int             i, cnt;
    fixed_t         bottombounds = rdraw_viewheight << FRACBITS;
    fixed_t         topbounds = (con_clipviewtop - 2) << FRACBITS;
    fixed_t         offsetvalue = 0;
    fixed_t         lightheight, texheightz;
    r_lightlist_t * rlight; // dc_lightlist
    ff_light_t    * ff_light; // light list item
    lighttable_t  * ro_colormap = NULL;  // override colormap
    extracolormap_t  * ro_extracolormap = NULL;  // override extracolormap
    texture_render_t * texren;

    // TM_picture does not use column_t.
    // Each colfunc_2s for each format will have to convert type.
    byte * col_data;
    void (*colfunc_2s) (byte*);

    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?

    curline = ds->curline;
    backsector = ffloor->taggedtarget;
    frontsector = (curline->frontsector == ffloor->taggedtarget) ?
                   curline->backsector : curline->frontsector;

    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    // midtexture, 0=no-texture, otherwise valid
    texnum = sides[ffloor->master->sidenum[0]].midtexture;
    if( texnum == 0 )
       return;  // no texture to display (when 3Dslab is missing side texture)

    texnum = texturetranslation[texnum];
#if 1
    texren = R_MaskedDraw_setup( texnum );
#else
    texren = & texture_render[texnum];
    texren->detect |= TD_masked;  // to protect patch
#endif

    colfunc = basecolfunc;

    if(ffloor->flags & FF_TRANSLUCENT)
    {
      // Hacked up support for alpha value in software mode SSNTails 09-24-2002
      // [WDJ] 11-2012
      dr_alpha = ffloor->alpha; // translucent alpha 0..255
//      dr_alpha = fweff[ffloor->fw_effect].alpha;
      dc_translucent_index = 0; // force use of dr_alpha by RGB drawers
      dc_translucentmap = & translucenttables[ translucent_alpha_table[dr_alpha >> 4] ];
      colfunc = transcolfunc;
    }
    else if(ffloor->flags & FF_FOG)
    {
      colfunc = fogcolfunc;  // R_DrawFogColumn_8 16 ..
      // from fogsheet
      fog_col_length = (texren->texture_model == TM_picture)? textures[texnum]->height : 2;
      fog_index = fog_tic % fog_col_length;  // pixel selection
      fog_init = 1;
      dr_alpha = fweff[ffloor->fw_effect].fsh_alpha; // dr_alpha 0..255
    }
    base_fog_alpha = dr_alpha;

    // [WDJ] Overrides of colormap.
    if( fixedcolormap )
        ro_colormap = fixedcolormap;
    else if( view_extracolormap )
    {
        ro_extracolormap = view_extracolormap;
    }
    else if(ffloor->flags & FF_FOG)   // Same result if test for colormap, or not.
    {
        // [WDJ] FF_FOG has optional colormap.
        // Use that colormap if it is present, there usually is only one colormap in a situation.
        // If no colormap (ro_extracolormap == NULL), then ff_light->extra_colormap can be used.
        ro_extracolormap = ffloor->master->frontsector->extra_colormap;
    }

    //SoM: Moved these up here so they are available for my lightlist calculations
    rw_scalestep = ds->scalestep;
    dm_yscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;

    dc_numlights = frontsector->numlights;
    if( dc_numlights )
    {
      if(dc_numlights > dc_maxlights)    expand_lightlist();

      cnt = 0; // cnt of rlight created, some ff_light will be skipped
      // highest light to lowest light, [0] is sector light at top
      for(i = 0; i < dc_numlights; i++)
      {
        // Limit list to lights that affect this thickside.
        ff_light = &frontsector->lightlist[i];
        rlight = &dc_lightlist[cnt];	// create in this list slot

        if(ff_light->height < *ffloor->bottomheight)
          continue;  // too low, next ff_light

        if(ff_light->height > *ffloor->topheight)
        {
          // This light is above the ffloor thickside.
          // Ignore it if the next light down is also above the ffloor thickside, when
          // that light will block.
          if(i+1 < dc_numlights
             && frontsector->lightlist[i+1].height > *ffloor->topheight
             && !(frontsector->lightlist[i+1].flags & FF_NOSHADE) )
            continue;  // too high, next ff_light
        }

        lightheight = ff_light->height;// > *ffloor->topheight ? *ffloor->topheight + FRACUNIT : ff_light->height;
        rlight->heightstep = -FixedMul (rw_scalestep, (lightheight - viewz));
        rlight->height = (centeryfrac) - FixedMul((lightheight - viewz), dm_yscale) - rlight->heightstep;
        rlight->flags = ff_light->flags;
        if(ff_light->flags & (FF_CUTSOLIDS|FF_CUTEXTRA))
        {
          lightheight = *ff_light->caster->bottomheight;// > *ffloor->topheight ? *ffloor->topheight + FRACUNIT : *ff_light->caster->bottomheight;
          rlight->botheightstep = -FixedMul (rw_scalestep, (lightheight - viewz));
          rlight->botheight = (centeryfrac) - FixedMul((lightheight - viewz), dm_yscale) - rlight->botheightstep;
        }

        rlight->lightlevel = *ff_light->lightlevel;
        rlight->extra_colormap = ff_light->extra_colormap;

        // Check if the current light affects the colormap/lightlevel
        if( ff_light->flags & FF_NOSHADE )
          continue; // next ff_light

        // Allows FOG on ffloor thickside to override lights.
        // Really is only meant to handle one of these options at a time.
        // Light and colormap precedence should match.
        if(ffloor->flags & FF_FOG)
          vlight = ffloor->master->frontsector->lightlevel + extralight_fog;
        else if(rlight->flags & FF_FOG)
          vlight = rlight->lightlevel + extralight_fog;
        else if(rlight->extra_colormap && rlight->extra_colormap->fog)
          vlight = rlight->lightlevel + extralight_cm;
        else
          vlight = rlight->lightlevel + extralight + orient_light;

        rlight->vlightmap =
             (vlight < 0) ? scalelight[0]
           : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
           : scalelight[vlight>>LIGHTSEGSHIFT];

        // [WDJ] Fixed determinations, taken out of loop.
        // Overrides of colormap, with same usage.
        // colormap precedence:
        //  fixedcolormap, ffloor FF_FOG colormap, rlight->extra_colormap
        if( fixedcolormap )
            rlight->rcolormap = fixedcolormap;
        else if( ro_extracolormap ) // override light extracolormap
            rlight->extra_colormap = ro_extracolormap;

        cnt++;
      }
      dc_numlights = cnt;
    }
    else
    {
      // Render ffloor thickside when frontsector does not have lightlist.
      // This happens for fog effects in otherwise normal sector, and probably others.
      //SoM: Get correct light level!
      if(ffloor->flags & FF_FOG)
        vlight = ffloor->master->frontsector->lightlevel + extralight_fog;
      else if(frontsector->extra_colormap && frontsector->extra_colormap->fog)
        vlight = frontsector->lightlevel + extralight_cm;
      else if(colfunc == transcolfunc)
        vlight = 255 + orient_light;
      else
      {
        sector_t * lightsec = R_FakeFlat(frontsector, &tempsec, false,
                                         /*OUT*/ NULL, NULL );
        vlight = lightsec->lightlevel + extralight + orient_light;
      }

      walllights =
           (vlight < 0) ? scalelight[0]
         : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
         : scalelight[vlight>>LIGHTSEGSHIFT];

      // colormap precedence:
      //  fixedcolormap, ffloor FF_FOG colormap, frontsector->extra_colormap
      if( !ro_extracolormap )
      {
        ro_extracolormap = frontsector->extra_colormap;
      }

      if( ro_extracolormap )
        ro_colormap = ro_extracolormap->colormap;
    }

    maskedtexturecol = ds->draw_ffside->thicksidecol;

    dm_floorclip = ds->spr_bottomclip;
    dm_ceilingclip = ds->spr_topclip;

    dm_texturemid = *ffloor->topheight - viewz;

    offsetvalue = sides[ffloor->master->sidenum[0]].rowoffset;
    if(curline->linedef->flags & ML_DONTPEGBOTTOM)
      offsetvalue -= *ffloor->topheight - *ffloor->bottomheight;

    dm_texturemid += offsetvalue;  // R_DrawMaskedColumn sets dc_texturemid
    dc_texturemid = dm_texturemid; // For column drawers other than R_DrawMaskedColumn

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif


    //faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
    //     are not stored per-column with post info anymore in Doom Legacy
    // [WDJ] multi-patch transparent texture restored
    if( texren->cache == NULL
        || ! (texren->detect & TD_2s_ready) )
    {
        R_GenerateTexture( texren, TM_masked );	// first time
    }

    colfunc_2s = colfunc_2s_masked_table[ texren->texture_model ];

    texheightz = textureheight[texnum];
    dc_texheight = texheightz >> FRACBITS;
    // For TM_picture, textures[texnum]->height
    column2s_length = dc_texheight;

    // [WDJ] x1,x2 are limited to 0..rdraw_viewwidth to protect [dc_x] access.
#ifdef RANGECHECK_DRAW_LIMITS
    if( x1 < 0 || x2 >= rdraw_viewwidth )
    {
        I_SoftError( "R_RenderThickSideRange: %i  %i\n", x1, x2);
        return;
    }
#endif

    if( x1 < 0 )  x1 = 0;
    if( x2 >= rdraw_viewwidth )  x2 = rdraw_viewwidth - 1;
    // draw the columns
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
      if(maskedtexturecol[dc_x] != SHRT_MAX)
      {
        dm_top_patch = dm_windowtop = (centeryfrac - FixedMul((dm_texturemid - offsetvalue), dm_yscale));
        dm_bottom_patch = dm_windowbottom = FixedMul(*ffloor->topheight - *ffloor->bottomheight, dm_yscale) + dm_top_patch;

        // SoM: New code does not rely on r_drawColumnShadowed_8 which
        // will (hopefully) put less strain on the stack.
        if(dc_numlights)
        {
          fixed_t        height;
          fixed_t        bheight = 0;
          byte   solid = 0;  // when light source is treated as a solid
          byte   lighteffect = 0;  // when not NOSHADE, light affects the colormap and lightlevel

          if(dm_windowbottom < topbounds || dm_windowtop > bottombounds)
          {
            // The dm_window is outside the view window (topbounds, bottombounds).
            // Apply dc_lightlist height adjustments. The height at the following x are dependent upon this.
            // highest light to lowest light, [0] is sector light at top
            for(i = 0; i < dc_numlights; i++)
            {
              rlight = &dc_lightlist[i];
              rlight->height += rlight->heightstep;
              if(rlight->flags & (FF_CUTSOLIDS|FF_CUTEXTRA))
                rlight->botheight += rlight->botheightstep;
            }
            goto next_x;	    
          }

          dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

          // draw the texture
          col_data = R_GetPatchColumn(texren, maskedtexturecol[dc_x]);

          // Top level colormap, or fixedcolormap.
          dc_colormap = dc_lightlist[0].rcolormap;

          // Setup dc_lightlist as to the 3dfloor light sources and effects.
          // highest light to lowest light, [0] is sector light at top
          for(i = 0; i < dc_numlights; i++)
          {
            // Check if the current light affects the colormap/lightlevel
            rlight = &dc_lightlist[i];
            lighteffect = !(rlight->flags & FF_NOSHADE);
            if(lighteffect)
            {
              // use rlight->rcolormap only when lighteffect
              if( !fixedcolormap )
              {
                // distance effect on light, yscale is smaller at distance.
                unsigned  dlit = dm_yscale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                  dlit = MAXLIGHTSCALE-1;

                // light table for the distance
                rlight->rcolormap = rlight->vlightmap[dlit];
                if( rlight->extra_colormap )
                {
                  // reverse indexing, and change to extra_colormap
                  int lightindex = rlight->rcolormap - reg_colormaps;
                  rlight->rcolormap = & rlight->extra_colormap->colormap[ lightindex ];
                }
              } // not fixedcolormap
            } // lighteffect

            // Check if the current light can cut the current 3D floor.
            if(rlight->flags & FF_CUTSOLIDS && !(ffloor->flags & FF_EXTRA))
              solid = 1;
            else if(rlight->flags & FF_CUTEXTRA && ffloor->flags & FF_EXTRA)
            {
              if(rlight->flags & FF_EXTRA)
              {
                // The light is from an extra 3D floor... Check the flags so
                // there are no undesired cuts.
                if((rlight->flags & (FF_TRANSLUCENT|FF_FOG)) == (ffloor->flags & (FF_TRANSLUCENT|FF_FOG)))
                {
                   if( ffloor->flags & FF_JOIN_SIDES )
                   {
                      float fm = base_fog_alpha * 0.4;  // JOIN
                      if( view_fogfloor && (dc_iscale < 0x4000))
                      {
                          // fade JOIN fogsheet as player approaches it
                          fm = (fm * dc_iscale) * (1.0/0x4000);
                      }
                      dr_alpha = (int)fm;
                   }
                   else
                      solid = 1;
                }
              }
              else
                solid = 1;
            }

            rlight->height += rlight->heightstep;
            height = rlight->height;

            if(solid)
            {
              rlight->botheight += rlight->botheightstep;
              bheight = rlight->botheight - (FRACUNIT >> 1);
            }

            if(height <= dm_windowtop)
            {
              if(lighteffect)
                dc_colormap = rlight->rcolormap;
              if(solid && dm_windowtop < bheight)
                dm_windowtop = bheight;
              continue;  // next light
            }

            dm_windowbottom = height;
            if(dm_windowbottom >= dm_bottom_patch)
            {
              // bottom of view window
              dm_windowbottom = dm_bottom_patch;
              colfunc_2s( col_data );

              // Finish dc_lightlist height adjustments.
              // highest light to lowest light, [0] is sector light at top
              for(i++ ; i < dc_numlights; i++)
              {
                rlight = &dc_lightlist[i];
                rlight->height += rlight->heightstep;
                if(rlight->flags & (FF_CUTSOLIDS|FF_CUTEXTRA))
                  rlight->botheight += rlight->botheightstep;
              }
              goto next_x;
            }

            colfunc_2s( col_data );  // draw

            // downward light from this light level
            dm_windowtop = solid ? bheight : (dm_windowbottom + 1);
            if(lighteffect)
              dc_colormap = rlight->rcolormap;
          } // for lights
          // bottom floor
          dm_windowbottom = dm_bottom_patch;
          if(dm_windowtop < dm_windowbottom)
            colfunc_2s( col_data );

        next_x:
          dm_yscale += rw_scalestep;
          continue;  // dc_x
        }

        // No ffloor.
        // calculate lighting for distance using dm_yscale
        if (!fixedcolormap)
        {
            // distance effect on light, yscale is smaller at distance.
            unsigned  dlit = dm_yscale>>LIGHTSCALESHIFT;
            if (dlit >=  MAXLIGHTSCALE )
                dlit = MAXLIGHTSCALE-1;

            // light table for the distance
            dc_colormap = walllights[dlit];
            if( ro_colormap )
            {
                // reverse indexing, and change to extra_colormap
                int lightindex = dc_colormap - reg_colormaps;
                dc_colormap = & ro_colormap[ lightindex ];
            }
        }

        dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

        // draw the texture
        col_data = R_GetPatchColumn(texren, maskedtexturecol[dc_x]);
        colfunc_2s( col_data );

        dm_yscale += rw_scalestep;
      }
    }
    colfunc = basecolfunc;
}


// [WDJ] Render a fog sheet, generated from midtexture, with alpha
void R_RenderFog( ffloor_t* fff, sector_t * intosec, lightlev_t foglight,
                  fixed_t scale )
{
    line_t * fogline = fff->master;
    side_t * fogside = & sides[ fogline->sidenum[0] ];
    sector_t * modelsec = fogside->sector;
    lighttable_t** xwalllights = scalelight[0];  // local selection of light table
    lighttable_t  * ro_colormap = NULL;  // override colormap
    texture_render_t * texren;

    byte *  col_data;
    void (*colfunc_2s) (byte*);

    int      texnum;
    fixed_t  texheightz;
    fixed_t  windowclip_top, windowclip_bottom;
    fixed_t  bot_patch;
//    fixed_t  topheight, heightstep, sec_ceilingheight_viewrel;  // became unused
    fixed_t  x1 = 0, x2 = rdraw_viewwidth - 1;

    // midtexture, 0=no-texture, otherwise valid
    texnum = texturetranslation[fogside->midtexture];
    if( texnum == 0 )  goto nofog;
    texren = & texture_render[texnum];

    dm_windowbottom = dm_windowtop = dm_bottom_patch = FIXED_MAX; // default no clip
    // [WDJ] clip at ceiling and floor
    // world coord, relative to viewer
    windowclip_top = intosec->ceilingheight - viewz;
    windowclip_bottom = intosec->floorheight - viewz;

    if( scale > 10 ) {
        dm_yscale = scale;
        rw_scalestep = 0;
    }
    else
    {
        // random fog scale
        dm_yscale = ((int)fog_wave2 << (FRACBITS-6)) + (14<<FRACBITS);  // 30 .. 14
        rw_scalestep = ((int)fog_wave1 << (FRACBITS-7)) - (4<<FRACBITS);  // ( 4 .. -4 )
        dm_yscale -= rw_scalestep/2;
        rw_scalestep /= rdraw_viewwidth;
    }

    // Select the default, or special effect column drawing functions,
    // which are called by the colfunc_2s functions.
    if (fff->flags & FF_FOG)	// Legacy Fog sheet or Fog/water with FF_FOG
    {
        // Display fog sheet (128 high) as transparent middle texture.
        // Only where there is a middle texture (in place of it).
        colfunc = fogcolfunc; // R_DrawFogColumn_8 16 ..
          // need dc_source, dc_colormap, dc_yh, dc_yl, dc_x
        fog_col_length = (texren->texture_model == TM_picture)? textures[texnum]->height : 2;
        // add in player movement to fog
        int playermov = (viewmobj->x + viewmobj->y + (viewmobj->angle>>6)) >> (FRACBITS+6);
        fog_index = (fog_tic + playermov) % fog_col_length;  // pixel selection
        fog_init = 1;
        dr_alpha = fweff[fff->fw_effect].fsh_alpha;  // dr_alpha 0..255
    }
    else
        goto nofog;

    // Select the 2s draw functions, they are called later.
    //faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
    //     are not stored per-column with post info anymore in Doom Legacy
    // [WDJ] multi-patch transparent texture restored
    // DrawMasked needs: dm_floorclip, dm_ceilingclip, dm_yscale,
    //  dm_top_patch, dm_bottom_patch, dm_windowtop, dm_windowbottom
    if( texren->cache == NULL )
    {
        R_GenerateTexture( texren, TM_masked );	// first time
    }

    colfunc_2s = colfunc_2s_masked_table[ texren->texture_model ];

    texheightz = textureheight[texnum];
    dc_texheight = texheightz >> FRACBITS;
    // For TM_picture, textures[texnum]->height
    column2s_length = dc_texheight;

    // To calc topheight.
    // fake floor light heights in screen coord. , at x=0
//    sec_ceilingheight_viewrel = modelsec->ceilingheight - viewz;  // for fog sector
//    topheight = (centeryfrac) - FixedMul(sec_ceilingheight_viewrel, dm_yscale);
//    heightstep = -FixedMul (rw_scalestep, sec_ceilingheight_viewrel);

    // Setup lighting based on the presence/lack-of 3D floors.
    dc_numlights = 1;

    dm_floorclip = clip_screen_bot_max;  // noclip
    dm_ceilingclip = clip_screen_top_min;  // noclip

    // top of texture, relative to viewer, with rowoffset, world coord.
    dm_texturemid = modelsec->ceilingheight + fogside->rowoffset - viewz;
    dc_texturemid = dm_texturemid; // For column drawers other than R_DrawMaskedColumn
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;

    // [WDJ] Fixed determinations, taken out of loop.
    // Overrides of colormap, with same usage.
    if( fixedcolormap )
        ro_colormap = fixedcolormap;
    else if( view_colormap )
        ro_colormap = view_colormap;
    else if( modelsec->extra_colormap )
        ro_colormap = modelsec->extra_colormap->colormap;;

    if( !fixedcolormap )
    {
        lightlev_t vlight = modelsec->lightlevel + foglight;
        xwalllights =
            (vlight < 0) ? scalelight[0]
          : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
          : scalelight[vlight>>LIGHTSEGSHIFT];
    }

    // draw the columns, for one fog FakeFloor
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
        // top/bottom of texture, relative to viewer, screen coord.
        dm_top_patch = (centeryfrac - FixedMul(dm_texturemid, dm_yscale));
        bot_patch = FixedMul(texheightz, dm_yscale) + dm_top_patch;
        // fog sheet clipping to ceiling and floor
        dm_windowtop = centeryfrac - FixedMul(windowclip_top, dm_yscale);
        dm_windowbottom = centeryfrac - FixedMul(windowclip_bottom, dm_yscale);
        
//        topheight += heightstep;
//        if(topheight > dm_windowtop)
        {
//	    if( dm_windowbottom > topheight )
//	        dm_windowbottom = topheight;
            if(dm_windowbottom >= bot_patch)
                dm_windowbottom = bot_patch;

            if( ! fixedcolormap )
            {
                // distance effect on light, yscale is smaller at distance.
                unsigned dlit = dm_yscale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                    dlit = MAXLIGHTSCALE-1;

                // light table for the distance
                dc_colormap = xwalllights[dlit];
                if( ro_colormap )
                {
                    // reverse indexing, and change to extra_colormap
                    int lightindex = dc_colormap - reg_colormaps;
                    dc_colormap = & ro_colormap[ lightindex ];
                }
            }

            dc_iscale = 0xffffffffu / (unsigned)dm_yscale;
            // draw texture, as clipped
            col_data = R_GetPatchColumn(texren, 0);
            colfunc_2s( col_data );
        }
        dm_yscale += rw_scalestep;
    } // for( dx_x = x1..x2 )
    colfunc = basecolfunc;

nofog:
    return;
}



//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//

// HEIGHTFRAC, a fixed point with 12 fraction bits.
#define HEIGHTBITS              12
#define HEIGHTUNIT              (1<<HEIGHTBITS)
#define HEIGHTFRAC_TO_INT( x )      ((x)>>HEIGHTBITS)
#define FIXED_TO_HEIGHTFRAC( x )    ((x)>>(FRACBITS-HEIGHTBITS))


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
long long mycount;
long long mytotal = 0;
unsigned long   nombre = 100000;
//static   char runtest[10][80];
#endif
//profile stuff ---------------------------------------------------------



// Software Render
// IN: rw_ parameters
// Called by R_StoreWallRange
static
void R_RenderSegLoop (void)
{
    int        orient_light = 0;  // wall orientation effect

    int        angf;
    int        yl, yh;

    fixed_t    texturecolumn = 0;
    int        mid, top, bottom;
    int        i;
    lighttable_t  * ro_colormap = NULL;  // override colormap
    extracolormap_t  * ro_extracolormap = NULL;  // override colormap

    // line orientation light, out of the loop
    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif

    if( dc_numlights )
    {
        r_lightlist_t * rlight;
        lightlev_t  vlight;

        // Setup dc_lightlist as to the 3dfloor light and colormaps.
        // The lightlevel and extra_colormap were copied from ffloor by R_StoreWallRange.
        // NOTE: Due to StoreWallRange, the dc_lightlist entries do NOT correspond to
        // the sector lightlist[i].
        for(i = 0; i < dc_numlights; i++)
        {
            rlight = & dc_lightlist[i];

            // [WDJ] FF_FOG is also set when frontsector lightlist caster has FF_FOG.
            if( rlight->flags & FF_FOG )
                vlight = rlight->lightlevel + extralight_fog;
            else if( rlight->extra_colormap && rlight->extra_colormap->fog)
                vlight = rlight->lightlevel + extralight_cm;
            else
                vlight = rlight->lightlevel + extralight + orient_light;

            rlight->vlightmap =
                  (vlight < 0) ? scalelight[0]
                : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
                : scalelight[vlight>>LIGHTSEGSHIFT];

            // [WDJ] Fixed determinations, taken out of line loop.
            // Colormap overrides, with the same usage.
            if( fixedcolormap )
                rlight->rcolormap = fixedcolormap;
            else if( view_extracolormap )
                rlight->extra_colormap = view_extracolormap;
        }
        // Select draw function that will step through dc_lightlist, and sets dc_colormap.
        colfunc = R_DrawColumnShadowed;  // generic 8 16
    }
    else
    {
        // [WDJ] Fixed determinations, taken out of line loop.
        // Colormap overrides, with the same usage.
        if( fixedcolormap )
            ro_colormap = fixedcolormap;
        else if( view_extracolormap )
            ro_extracolormap = view_extracolormap;
        else  // over the whole line
            ro_extracolormap = frontsector->extra_colormap;

        if( ro_extracolormap )
            ro_colormap = ro_extracolormap->colormap;
    }


    // Loop over width
    for ( ; rw_x < rw_stopx ; rw_x++)
    {
        // mark floor / ceiling areas
        yl = HEIGHTFRAC_TO_INT( (topfrac + HEIGHTUNIT - 1) );  // rounded up

        // no space above wall?
        if (yl < ceilingclip[rw_x])
            yl = ceilingclip[rw_x];

        if (markceiling)
        {
            top = ceilingclip[rw_x];
            bottom = yl-1;

            if (bottom > floorclip[rw_x])
                bottom = floorclip[rw_x];

            // visplane global parameter vsp_ceilingplane
            if (top <= bottom)
            {
#ifdef DYNAMIC_VISPLANE_COVER
                vsp_ceilingplane->cover[rw_x].top = top;
                vsp_ceilingplane->cover[rw_x].bottom = bottom;
#else
                vsp_ceilingplane->top[rw_x] = top;
                vsp_ceilingplane->bottom[rw_x] = bottom;
#endif
            }
        }


        yh = HEIGHTFRAC_TO_INT( bottomfrac );

        if (yh > floorclip[rw_x])
            yh = floorclip[rw_x];

        if (markfloor)
        {
            top = yh+1;
            bottom = floorclip[rw_x];
            if (top < ceilingclip[rw_x])
                top = ceilingclip[rw_x];
            // visplane global parameter vsp_floorplane
            if (top <= bottom && vsp_floorplane)
            {
#ifdef DYNAMIC_VISPLANE_COVER
                vsp_floorplane->cover[rw_x].top = top;
                vsp_floorplane->cover[rw_x].bottom = bottom;
#else
                vsp_floorplane->top[rw_x] = top;
                vsp_floorplane->bottom[rw_x] = bottom;
#endif
            }
        }


        // The ffplane and numffplane are of the subsector processed by BSP.
        if (numffplane)
        {
          // The backscale is the rw_scale from the previous SegLoop call.	   
          // It is the scale at the near edge of the ffloor.
          first_subsec_seg->draw_ffplane->backscale_r[rw_x] = backscale[rw_x];
//	  first_subsec_seg->frontscale[rw_x] = rw_scale;

          // Over all ffloor planes.
          for(i = 0; i < numffplane; i++)
          {
            if(ffplane[i].height < viewz)
            {
              int top_w = HEIGHTFRAC_TO_INT( ffplane[i].front_frac ) + 1;
              // [WDJ] To prevent little black lines, front_clip_bot is +1 from a normal clip.
//              int bottom_w = ffplane[i].front_clip_bot[rw_x] - 1;  // get black lines
              int bottom_w = ffplane[i].front_clip_bot[rw_x];

              if(top_w < ceilingclip[rw_x])
                top_w = ceilingclip[rw_x];

              if (bottom_w > floorclip[rw_x])
                bottom_w = floorclip[rw_x];

              if (top_w <= bottom_w)
              {
#ifdef DYNAMIC_VISPLANE_COVER
                ffplane[i].plane->cover[rw_x].top = top_w;
                ffplane[i].plane->cover[rw_x].bottom = bottom_w;
#else
                ffplane[i].plane->top[rw_x] = top_w;
                ffplane[i].plane->bottom[rw_x] = bottom_w;
#endif
              }
            }
            else if (ffplane[i].height > viewz)
            {
              int top_w = ffplane[i].plane_clip_top[rw_x];
              int bottom_w = HEIGHTFRAC_TO_INT( ffplane[i].front_frac );

              if (top_w < ceilingclip[rw_x])
                top_w = ceilingclip[rw_x];

              if (bottom_w > floorclip[rw_x])
                bottom_w = floorclip[rw_x];

              if (top_w <= bottom_w)
              {
#ifdef DYNAMIC_VISPLANE_COVER
                ffplane[i].plane->cover[rw_x].top = top_w;
                ffplane[i].plane->cover[rw_x].bottom = bottom_w;
#else
                ffplane[i].plane->top[rw_x] = top_w;
                ffplane[i].plane->bottom[rw_x] = bottom_w;
#endif
              }
            }
          }
        } // if numffplane

        //SoM: Calculate offsets for Thick fake floors.
        // calculate texture offset
        angf = ANGLE_TO_FINE(rw_centerangle + x_to_viewangle[rw_x]);
        texturecolumn = rw_offset - FixedMul(finetangent[angf], rw_distance);
        texturecolumn >>= FRACBITS;

        // texturecolumn and lighting are independent of wall tiers
        if (segtextured)
        {
            dc_x = rw_x;
            dc_iscale = 0xffffffffu / (unsigned)rw_scale;

            if( ! fixedcolormap )
            {
                // distance effect on light, rw_scale is smaller at distance.
                unsigned  dlit = rw_scale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                    dlit = MAXLIGHTSCALE-1;

                // light table for the distance
                dc_colormap = walllights[dlit];
                if( ro_colormap )
                {
                    // reverse indexing, and change to extra_colormap
                    int lightindex = dc_colormap - reg_colormaps;
                    dc_colormap = & ro_colormap[ lightindex ];
                }
            }
        }

        if(dc_numlights)
        {
          r_lightlist_t * rlight;

          // Setup dc_lightlist as to the 3dfloor light and colormaps.
          // highest light to lowest light, [0] is sector light at top
          for(i = 0; i < dc_numlights; i++)
          {
            rlight = & dc_lightlist[i];

            if( !fixedcolormap )
            {
              // distance effect on light, rw_scale is smaller at distance.
              unsigned  dlit = rw_scale>>LIGHTSCALESHIFT;
              if (dlit >=  MAXLIGHTSCALE )
                dlit = MAXLIGHTSCALE-1;

              // light table for the distance
              rlight->rcolormap = rlight->vlightmap[dlit];
              if( rlight->extra_colormap )
              {
                // reverse indexing, and change to extra_colormap
                int lightindex = rlight->rcolormap - reg_colormaps;
                rlight->rcolormap = & rlight->extra_colormap->colormap[ lightindex ];
              }
            }
          }
          // The colfunc will step through dc_lightlist, and sets dc_colormap.
        } // if dc_numlights

        backscale[rw_x] = rw_scale;

        // draw the wall tiers
        if (midtexture)
        {
          if( yl < rdraw_viewheight && yh >= 0 && yh >= yl ) // not disabled
          {
            // single sided line
            dc_yl = yl;
            dc_yh = yh;
#ifdef RANGECHECK
            rangecheck_id = 1;  // mid
#endif
#ifndef TEXTURE_LOCK
            rw_texture_num = midtexture;  // to restore texture cache
#endif

            dc_texturemid = rw_midtexturemid;
            dc_texheight = textureheight[midtexture] >> FRACBITS;
            //profile stuff ---------------------------------------------------------
#ifdef TIMING
            ProfZeroTimer();
#endif

            // Does not know if TM_picture or TM_patch	     
            R_Draw_WallColumn(mid_texren, texturecolumn);

#ifdef TIMING
            RDMSR(0x10,&mycount);
            mytotal += mycount;      //64bit add
            
            if(nombre--==0)
                I_Error("R_DrawColumn CPU Spy reports: 0x%d %d\n", *((int*)&mytotal+1),
                (int)mytotal );
#endif
            //profile stuff ---------------------------------------------------------
          }
            // dont draw anything more for this column, since
            // a midtexture blocks the view
            ceilingclip[rw_x] = rdraw_viewheight;  // block all
            floorclip[rw_x] = 0;  // block all
        }
        else
        {
            // two sided line
            if (toptexture)
            {
                // top wall
                mid = HEIGHTFRAC_TO_INT( pixhigh );
                pixhigh += pixhighstep;
                
                if (mid > floorclip[rw_x])
                    mid = floorclip[rw_x];

                if (mid >= yl)
                {
                    if( yl < rdraw_viewheight && mid >= 0 ) // not disabled
                    {
                        dc_yl = yl;
                        dc_yh = mid;

#ifdef RANGECHECK
                        rangecheck_id = 0; // top
#endif
#ifndef TEXTURE_LOCK
                        rw_texture_num = toptexture;  // to restore texture cache
#endif

                        dc_texturemid = rw_toptexturemid;
                        dc_texheight = textureheight[toptexture] >> FRACBITS;
                        // Does not know if TM_picture or TM_patch	     
                        R_Draw_WallColumn(top_texren, texturecolumn);
                    } // if mid >= 0
                    ceilingclip[rw_x] = mid + 1;  // next drawable row
                }
                else
                {
                    // mid < yl
                    ceilingclip[rw_x] = yl;  // undrawn
                }
            }
            else
            {
                // no top wall
                if (markceiling)
                {
                    ceilingclip[rw_x] = yl;  // undrawn
                }
            }

            if (bottomtexture)
            {
                // bottom wall
                mid = HEIGHTFRAC_TO_INT( pixlow + HEIGHTUNIT - 1 );  // rounded up
                pixlow += pixlowstep;

                // no space above wall?
                if (mid < ceilingclip[rw_x])
                    mid = ceilingclip[rw_x];

                if (mid <= yh)
                {
                    if( mid < rdraw_viewheight && yh >= 0 ) // not disabled
                    {
                        dc_yl = mid;
                        dc_yh = yh;

#ifdef RANGECHECK
                        rangecheck_id = 0; // top
#endif
#ifndef TEXTURE_LOCK
                        rw_texture_num = bottomtexture;  // to restore texture cache
#endif

                        dc_texturemid = rw_bottomtexturemid;
                        dc_texheight = textureheight[bottomtexture] >> FRACBITS;

                        // Does not know if TM_picture or TM_patch	     
                        R_Draw_WallColumn(bottom_texren, texturecolumn);
                    } // if mid >= 0
                    floorclip[rw_x] = mid - 1;  // next drawable row
                }
                else
                {
                    floorclip[rw_x] = yh;  // undrawn row
                }
            }
            else
            {
                // no bottom wall
                if (markfloor)
                {
                    floorclip[rw_x] = yh;  // undrawn row
                }
            }
        }

        if (maskedtexture || numthicksides)
        {
          // save texturecol
          //  for backdrawing of masked mid texture
          maskedtexturecol[rw_x] = texturecolumn;
        }

        if(dc_numlights)
        {
          // Apply dc_lightlist height adjustments.
          // highest light to lowest light, [0] is sector light at top
          for(i = 0; i < dc_numlights; i++)
          {
            dc_lightlist[i].height += dc_lightlist[i].heightstep;
            if(dc_lightlist[i].flags & FF_SOLID)
              dc_lightlist[i].botheight += dc_lightlist[i].botheightstep;
          }
        }


        /*if(dc_wallportals)
        {
          wallportal_t* wpr;
          for(wpr = dc_wallportals; wpr; wpr = wpr->next)
          {
            wpr->top += wpr->topstep;
            wpr->bottom += wpr->bottomstep;
          }
        }*/


        for(i = 0; i < MAXFFLOORS; i++)
        {
          if (ffplane[i].valid_mark)
          {
            int y_w = HEIGHTFRAC_TO_INT( ffplane[i].back_frac );

            // [WDJ] Would think this should just be (y_w - 1),
            // but adding 1 prevents little black lines between joined sector plane draws.
//            ffplane[i].front_clip_bot[rw_x] = y_w - 1;  // get little black lines
            ffplane[i].front_clip_bot[rw_x] = y_w;
            ffplane[i].plane_clip_top[rw_x] = y_w + 1;
            ffplane[i].back_frac += ffplane[i].back_step;
          }

          ffplane[i].front_frac += ffplane[i].front_step;
        }

        rw_scale += rw_scalestep;
        topfrac += topstep;
        bottomfrac += bottomstep;
        // [WDJ] Overflow protection.  Overflow and underflow of topfrac and
        // bottomfrac cause off-screen textures to be drawn as large bars.
        // See phobiata.wad map07, which has a floor at -20000.
        // The cause of the overflow many times seems to be the step value.
        if( bottomfrac < topfrac ) {
           // Uncomment to see which map areas cause this overflow.
//	   debug_Printf("Overflow break: bottomfrac(%i) < topfrac(%i)\n", bottomfrac, topfrac );
           break;
        }
    }
}



//
// R_StoreWallRange, software render.
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
// Called during BSP traversal for every visible linedef segment of a subsector.
void R_StoreWallRange( int   start, int   stop)
{
    fixed_t             hyp;
    fixed_t             sineval;
    angle_t             distangle, offsetangle;
    fixed_t             vtop;
    lightlev_t          vlight;  // visible light 0..255
    lightlev_t          orient_light = 0;  // wall orientation effect
    int                 i, cnt;
    ff_light_t        * ff_light;  // light list item
    r_lightlist_t     * rlight;
    ffloor_t          * bff, * fff;  // backsector fake floor, frontsector fake floor
//    fixed_t             lightheight;  // unused

    // ds_p : next available drawseg
    if (ds_p == &drawsegs[max_drawsegs])
        expand_drawsegs();

    // Transfer wall attributes to next drawseg ( ds_p ).

#ifdef RANGECHECK_DRAW_LIMITS
    if (start >=rdraw_viewwidth || start > stop)
    { 
        I_SoftError ("R_StoreWallRange: Bad x range, %i to %i\n", start , stop);
        return;
    }
#endif

    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    sidedef = curline->sidedef;
    linedef = curline->linedef;

    // mark the segment as visible for auto map
    linedef->flags |= ML_MAPPED;

    // calculate rw_distance for scale calculation
    rw_normalangle = curline->angle + ANG90;
    offsetangle = abs((signed_angle_t)rw_normalangle - (signed_angle_t)rw_angle1);

    if (offsetangle > ANG90)
        offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    sineval = sine_ANG(distangle);
    hyp = R_PointToDist (curline->v1->x, curline->v1->y);
    rw_distance = FixedMul (hyp, sineval);

#ifdef DEBUG_STOREWALL   
    GenPrintf( EMSG_debug, "WALL:  drawseg[%i] (%i..%i)", (ds_p - drawsegs), start, stop );
    if( numffplane )  GenPrintf( EMSG_debug, " numffplane=%i", numffplane );
    GenPrintf( EMSG_debug, "\n" );
#endif

    // Start drawseg
    ds_p->curline = curline;

    // segment ends
    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    rw_stopx = stop+1;
    // [WDJ] draw range is rw_x .. rw_stopx-1
    if( rw_stopx > rdraw_viewwidth )  rw_stopx = rdraw_viewwidth;

    pool16_need( (rw_stopx - start)*4 );

    // calculate scale at both ends and step
    ds_p->scale1 = rw_scale =
        R_ScaleFromGlobalAngle (viewangle + x_to_viewangle[start]);

    if (stop > start)
    {
        ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + x_to_viewangle[stop]);
        ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (stop-start);
    }
    else
    {
#if 0
        // UNUSED: try to fix the stretched line bug
        if (rw_distance < FRACUNIT/2)
        {
            fixed_t         tr_x,tr_y;
            fixed_t         gxt,gyt;
            
            tr_x = curline->v1->x - viewx;
            tr_y = curline->v1->y - viewy;
            
            gxt = FixedMul(tr_x,viewcos);
            gyt = -FixedMul(tr_y,viewsin);
            ds_p->scale1 = FixedDiv(projection, gxt-gyt)<<detailshift;
        }
#endif
        ds_p->scale2 = ds_p->scale1;
        ds_p->scalestep = 0;
    }

    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    worldtop = frontsector->ceilingheight - viewz;
    worldbottom = frontsector->floorheight - viewz;

    midtexture = toptexture = bottomtexture = maskedtexture = 0; // no-texture
    mid_texren = top_texren = bottom_texren = NULL;

    numthicksides = 0;

    ds_p->maskedtexturecol = NULL;
    ds_p->draw_ffplane = NULL;
    ds_p->draw_ffside = NULL;

    for(i = 0; i < MAXFFLOORS; i++)
    {
      ffplane[i].valid_mark = false;
    }

    // The ffplane and numffplane are of the subsector processed by BSP.
    if(numffplane)
    {
      // This should be in BSP
      for(i = 0; i < numffplane; i++)
        ffplane[i].front_pos = ffplane[i].height - viewz;
    }

    if (!backsector)
    {
        // single sided line
        // Single sided: assumes that there MUST be a midtexture on this side.
        // midtexture, 0=no-texture, otherwise valid
        midtexture = texturetranslation[sidedef->midtexture];
        mid_texren = R_WallTexture_setup( midtexture );

        // a single sided line is terminal, so it must mark ends
        markfloor = markceiling = true;

        if (linedef->flags & ML_DONTPEGBOTTOM)
        {
            // tile using original texture size
            vtop = frontsector->floorheight +
                textureheight[sidedef->midtexture];
            // bottom of texture at bottom
            rw_midtexturemid = vtop - viewz;
        }
        else
        {
            // top of texture at top
            rw_midtexturemid = worldtop;
        }
        rw_midtexturemid += sidedef->rowoffset;

        // drawseg blocks sprites
        ds_p->silhouette = SIL_TOP|SIL_BOTTOM; // BOTH
        ds_p->spr_topclip = clip_screen_bot_max;  // clip all
        ds_p->spr_bottomclip = clip_screen_top_min; // clip all
        ds_p->sil_bottom_height = FIXED_MAX;
        ds_p->sil_top_height = FIXED_MIN;
    }
    else
    {
        // two sided line
        ds_p->spr_topclip = ds_p->spr_bottomclip = NULL;
        ds_p->silhouette = 0;

        if (frontsector->floorheight > backsector->floorheight)
        {
            // frontsector floor clips backsector floor and sprites
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->sil_bottom_height = frontsector->floorheight;
        }
        else if (backsector->floorheight > viewz)
        {
            // backsector floor not visible, clip sprites
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->sil_bottom_height = FIXED_MAX;
            // ds_p->spr_bottomclip = clip_screen_top_min;  // clip all
        }

        if (frontsector->ceilingheight < backsector->ceilingheight)
        {
            // frontsector ceiling clips backsector ceiling and sprites
            ds_p->silhouette |= SIL_TOP;
            ds_p->sil_top_height = frontsector->ceilingheight;
        }
        else if (backsector->ceilingheight < viewz)
        {
            // backsector ceiling not visible, clip sprites
            ds_p->silhouette |= SIL_TOP;
            ds_p->sil_top_height = FIXED_MIN;
            // ds_p->spr_topclip = clip_screen_bot_max;  // clip all
        }


#if 0
// Duplicated by fix below.
        if (backsector->ceilingheight <= frontsector->floorheight)
        {
            // backsector below frontsector
            ds_p->spr_bottomclip = clip_screen_top_min;  // clip all
            ds_p->sil_bottom_height = FIXED_MAX;
            ds_p->silhouette |= SIL_BOTTOM;
        }
        
        if (backsector->floorheight >= frontsector->ceilingheight)
        {
            // backsector above frontsector
            ds_p->spr_topclip = clip_screen_bot_max;  // clip all
            ds_p->sil_top_height = FIXED_MIN;
            ds_p->silhouette |= SIL_TOP;
        }
#endif

        //SoM: 3/25/2000: This code fixes an automap bug that didn't check
        // frontsector->ceiling and backsector->floor to see if a door was closed.
        // Without the following code, sprites get displayed behind closed doors.
        if (doorclosed || backsector->ceilingheight<=frontsector->floorheight)
        {
            // backsector below frontsector
            ds_p->spr_bottomclip = clip_screen_top_min;  // clip all
            ds_p->sil_bottom_height = FIXED_MAX;
            ds_p->silhouette |= SIL_BOTTOM;
        }
        if (doorclosed || backsector->floorheight>=frontsector->ceilingheight)
        {                   // killough 1/17/98, 2/8/98
            // backsector above frontsector
            ds_p->spr_topclip = clip_screen_bot_max;  // clip all
            ds_p->sil_top_height = FIXED_MIN;
            ds_p->silhouette |= SIL_TOP;
        }

        worldbacktop = backsector->ceilingheight - viewz;
        worldbackbottom = backsector->floorheight - viewz;

        // hack to allow height changes in outdoor areas
        if (frontsector->ceilingpic == sky_flatnum
            && backsector->ceilingpic == sky_flatnum)
        {
            // SKY to SKY
            // [WDJ] Prevent worldtop < worldbottom, is used as error test
            if( worldbacktop < worldbottom )    worldbacktop = worldbottom;
            worldtop = worldbacktop;  // disable upper texture tests
        }


        if (worldbackbottom != worldbottom
            || backsector->floorpic != frontsector->floorpic
            || backsector->lightlevel != frontsector->lightlevel
            //SoM: 3/22/2000: Check floor x and y offsets.
            || backsector->floor_xoffs != frontsector->floor_xoffs
            || backsector->floor_yoffs != frontsector->floor_yoffs
            //SoM: 3/22/2000: Prevents bleeding.
            || (frontsector->model > SM_fluid)
            || backsector->modelsec != frontsector->modelsec
            || backsector->floorlightsec != frontsector->floorlightsec
            //SoM: 4/3/2000: Check for colormaps
            || frontsector->extra_colormap != backsector->extra_colormap
            || (frontsector->ffloors != backsector->ffloors && frontsector->tag != backsector->tag))
        {
            markfloor = true;  // backsector and frontsector floor are different
        }
        else
        {
            // same plane on both sides
            markfloor = false;
        }


        if (worldbacktop != worldtop
            || backsector->ceilingpic != frontsector->ceilingpic
            || backsector->lightlevel != frontsector->lightlevel
            //SoM: 3/22/2000: Check floor x and y offsets.
            || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
            || backsector->ceiling_yoffs != frontsector->ceiling_yoffs
            //SoM: 3/22/2000: Prevents bleeding.
//            || (frontsector->modelsec != -1 &&
            || (frontsector->model > SM_fluid &&
                frontsector->ceilingpic != sky_flatnum)
            || backsector->modelsec != frontsector->modelsec
            || backsector->floorlightsec != frontsector->floorlightsec
            //SoM: 4/3/2000: Check for colormaps
            || frontsector->extra_colormap != backsector->extra_colormap
            || (frontsector->ffloors != backsector->ffloors && frontsector->tag != backsector->tag))
        {
            markceiling = true;  // backsector and frontsector ceilings are different
        }
        else
        {
            // same plane on both sides
            markceiling = false;
        }

        if (backsector->ceilingheight <= frontsector->floorheight
            || backsector->floorheight >= frontsector->ceilingheight)
        {
            // closed door
            markceiling = markfloor = true;
        }

        // check TOP TEXTURE
        if (worldbacktop < worldtop)
        {
            // top texture, 0=no-texture, otherwise valid
            toptexture = texturetranslation[sidedef->toptexture];
            top_texren = R_WallTexture_setup( toptexture );

            if (linedef->flags & ML_DONTPEGTOP)
            {
                // top of texture at top
                rw_toptexturemid = worldtop;
            }
            else
            {
                // tile using original texture size
                vtop = backsector->ceilingheight
                     + textureheight[sidedef->toptexture];

                // bottom of texture
                rw_toptexturemid = vtop - viewz;
            }
        }

        // check BOTTOM TEXTURE
        if (worldbackbottom > worldbottom)     //seulement si VISIBLE!!!
        {
            // bottom texture, 0=no-texture, otherwise valid
            bottomtexture = texturetranslation[sidedef->bottomtexture];
            bottom_texren = R_WallTexture_setup( bottomtexture );

            if (linedef->flags & ML_DONTPEGBOTTOM )
            {
                // bottom of texture at bottom
                // top of texture at top
                rw_bottomtexturemid = worldtop;
            }
            else    // top of texture at top
                rw_bottomtexturemid = worldbackbottom;
        }

        rw_toptexturemid += sidedef->rowoffset;
        rw_bottomtexturemid += sidedef->rowoffset;

        // allocate space for masked texture tables
        if (frontsector && backsector && frontsector->tag != backsector->tag
            && (backsector->ffloors || frontsector->ffloors))
        {
          fixed_t   lowcut, highcut;
          int thi = 0;  // thicksides index
          ffloor_t  * ff_thicksides[MAXFFLOORS];  // temp ref

          //markceiling = markfloor = true;
          maskedtexture = true;

          // segment 3d floor sides
          maskedtexturecol = get_pool16_array( rw_stopx - rw_x ) - rw_x;

          lowcut = frontsector->floorheight > backsector->floorheight ? frontsector->floorheight : backsector->floorheight;
          highcut = frontsector->ceilingheight < backsector->ceilingheight ? frontsector->ceilingheight : backsector->ceilingheight;

          if(frontsector->ffloors && backsector->ffloors)
          {
            // For all backsector, check all frontsector
            for(bff = backsector->ffloors; bff; bff = bff->next)
            {
              if(!(bff->flags & FF_OUTER_SIDES) || !(bff->flags & FF_EXISTS))
                continue;
              // outside sides of bff
              if(*bff->topheight < lowcut || *bff->bottomheight > highcut)
                continue;

              // look for matching ffloor where we do not render the join side
              for(fff = frontsector->ffloors; fff; fff = fff->next)
              {
                if(!(fff->flags & (FF_OUTER_SIDES|FF_INNER_SIDES))
                   || !(fff->flags & FF_EXISTS)
                   || *fff->topheight < lowcut || *fff->bottomheight > highcut)
                  continue;
                // check against sides of fff

                if(bff->flags & FF_EXTRA)
                {
                  if(!(fff->flags & FF_CUTEXTRA))
                    continue;

                  if(fff->flags & FF_EXTRA
                     && (fff->flags & (FF_TRANSLUCENT|FF_FOG)) != (bff->flags & (FF_TRANSLUCENT|FF_FOG)))
                    continue;
                }
                else
                {
                  if(!(fff->flags & FF_CUTSOLIDS))
                    continue;
                }

                if(*bff->topheight > *fff->topheight
                   || *bff->bottomheight < *fff->bottomheight)
                  continue;

                // check for forced fogsheet
                if(bff->flags & FF_JOIN_SIDES)
                {
                    // and only one forced fogsheet
                    // note: fweff[0] unused, so safe to check it
                    if(( ! (fff->flags & FF_INNER_SIDES) )
                       || (fff->flags & FF_JOIN_SIDES) )
                    {
                        continue;  // bff fogsheet
                    }
                }
                break;  // fff overlaps bff, do not render bff side
              } // for fff
              if(fff)  // found fff that completely overlaps bff
                continue;

#ifdef DEBUG_STOREWALL   
              GenPrintf( EMSG_debug, "w: front/back backsector thickside[%i]\n", thi );
#endif
              ff_thicksides[thi] = bff;  // backsector 3d floor outer side
              thi++;
              if( thi >= MAXFFLOORS )
                 break;
            } // for bff

            // For all frontsector, check all backsector
            for(fff = frontsector->ffloors; fff; fff = fff->next)
            {
              if(!(fff->flags & FF_INNER_SIDES) || !(fff->flags & FF_EXISTS))
                continue;
              // inside sides of fff
              if(*fff->topheight < lowcut || *fff->bottomheight > highcut)
                continue;

              // check for forced fogsheet
              if(fff->flags & FF_JOIN_SIDES)
                 goto render_side_fff;

              // look for matching ffloor where we do not render the join side
              for(bff = backsector->ffloors; bff; bff = bff->next)
              {
                if(!(bff->flags & (FF_OUTER_SIDES|FF_INNER_SIDES))
                   || !(bff->flags & FF_EXISTS)
                   || *bff->topheight < lowcut || *bff->bottomheight > highcut)
                  continue;
                // check against sides of bff

                if(fff->flags & FF_EXTRA)
                {
                  if(!(bff->flags & FF_CUTEXTRA))
                    continue;

                  if(bff->flags & FF_EXTRA
                     && (bff->flags & (FF_TRANSLUCENT|FF_FOG)) != (fff->flags & (FF_TRANSLUCENT|FF_FOG)))
                    continue;
                }
                else
                {
                  if(!(bff->flags & FF_CUTSOLIDS))
                    continue;
                }

                if(*fff->topheight > *bff->topheight
                   || *fff->bottomheight < *bff->bottomheight)
                  continue;

                break;  // bff overlaps fff, do not render fff side
              } // for bff
              if(bff)  // found bff that completely overlaps fff
                continue;

            render_side_fff:
              if( thi >= MAXFFLOORS ) // also protects against exit from bff loop
                 break;

#ifdef DEBUG_STOREWALL   
              GenPrintf( EMSG_debug, "w: front/back frontsector thickside[%i]\n", thi );
#endif
              ff_thicksides[thi] = fff;  // frontsector 3d floor inner side
              thi++;
              if( thi >= MAXFFLOORS )
                 break;
            } // for fff
          }
          else if(backsector->ffloors)
          {
            // For all backsector
            for(bff = backsector->ffloors; bff; bff = bff->next)
            {
              if(!(bff->flags & FF_OUTER_SIDES) || !(bff->flags & FF_EXISTS))
                continue;
              // outer sides of bff
              if(*bff->topheight <= frontsector->floorheight
                 || *bff->bottomheight >= frontsector->ceilingheight)
                continue;

#ifdef DEBUG_STOREWALL   
              GenPrintf( EMSG_debug, "w: backsector thickside[%i]\n", thi );
#endif
              ff_thicksides[thi] = bff; // backsector 3d floor outer side
              thi++;
              if( thi >= MAXFFLOORS )
                 break;
            }
          }
          else if(frontsector->ffloors)
          {
            // For all frontsector
            for(fff = frontsector->ffloors; fff; fff = fff->next)
            {
              if(!(fff->flags & FF_INNER_SIDES) || !(fff->flags & FF_EXISTS))
                continue;
              // inner sides of fff
              if(*fff->topheight <= frontsector->floorheight
                 || *fff->bottomheight >= frontsector->ceilingheight)
                continue;
              if(*fff->topheight <= backsector->floorheight
                 || *fff->bottomheight >= backsector->ceilingheight)
                continue;

#ifdef DEBUG_STOREWALL   
              GenPrintf( EMSG_debug, "w: frontsector thickside[%i]\n", thi );
#endif
              ff_thicksides[thi] = fff;  // frontsector 3d floor inner side
              thi++;
              if( thi >= MAXFFLOORS )
                 break;
            } // for fff
          }

          numthicksides = thi;
          if( thi )
          {
            // Create ffside_t only when there are ffloor and thicksides.
            // Copy temp ff_thicksides to draw_ffside in drawseg.
            draw_ffside_t * dffs = create_draw_ffside( thi );
            ds_p->draw_ffside = dffs;
            dffs->thicksidecol = maskedtexturecol;
            memcpy( &dffs->thicksides[0], &ff_thicksides[0], (sizeof(ffloor_t*) * thi) );
          }
        } // if frontsector && backsector ..

        // midtexture, 0=no-texture, otherwise valid
        if (sidedef->midtexture)
        {
            // masked midtexture
            draw_ffside_t * dffs = ds_p->draw_ffside;  // from above
            if( dffs && dffs->thicksidecol )  // also is maskedtexurecol
            {
                ds_p->maskedtexturecol = dffs->thicksidecol;
            }
            else
            {
                maskedtexturecol = get_pool16_array( rw_stopx - rw_x ) - rw_x;
                ds_p->maskedtexturecol = maskedtexturecol;
            }

            maskedtexture = true;
        }
    }

    // calculate rw_offset (only needed for textured lines)
    segtextured = midtexture || toptexture || bottomtexture || maskedtexture || (numthicksides > 0);

    if (segtextured)
    {
        offsetangle = rw_normalangle-rw_angle1;

        if (offsetangle > ANG180)
            offsetangle = -offsetangle;

        if (offsetangle > ANG90)
            offsetangle = ANG90;

        sineval = sine_ANG(offsetangle);
        rw_offset = FixedMul (hyp, sineval);

        if (rw_normalangle-rw_angle1 < ANG180)
            rw_offset = -rw_offset;

        /// don't use texture offset for splats
        rw_offset2 = rw_offset + curline->offset;
        rw_offset += sidedef->textureoffset + curline->offset;
        rw_centerangle = ANG90 + viewangle - rw_normalangle;

        // calculate light table
        //  use different light tables
        //  for horizontal / vertical / diagonal
        if (!fixedcolormap)
        {
            vlight = frontsector->lightlevel + extralight + orient_light;

            walllights =
                (vlight < 0) ? scalelight[0]
              : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
              : scalelight[vlight>>LIGHTSEGSHIFT];
        }
    }

    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.

    //added:18-02-98: WATER! cacher ici dans certaines conditions?
    //                la surface eau est visible de dessous et dessus...
    if (frontsector->model > SM_fluid)
    {
        if (frontsector->floorheight >= viewz)
        {
            // above view plane
            markfloor = false;
        }

        if (frontsector->ceilingheight <= viewz
            && frontsector->ceilingpic != sky_flatnum)
        {
            // below view plane
            markceiling = false;
        }
    }

    // calculate incremental stepping values for texture edges
    worldtop = FIXED_TO_HEIGHTFRAC(worldtop);
    worldbottom = FIXED_TO_HEIGHTFRAC(worldbottom);

    topstep = -FixedMul (rw_scalestep, worldtop);
    topfrac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul (worldtop, rw_scale);

    bottomstep = -FixedMul (rw_scalestep,worldbottom);
    bottomfrac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul (worldbottom, rw_scale);        

    // [WDJ] Intercept overflow in FixedMul math
    if( bottomfrac < topfrac )
    {
       // enable print to see where this happens
//     debug_Printf("Overflow mult: bottomfrac(%i) < topfrac(%i)\n", bottomfrac, topfrac );
       return;
    }

    dc_numlights = frontsector->numlights;
    if( dc_numlights )  // has ff_lights
    {
      if(dc_numlights >= dc_maxlights)    expand_lightlist();

      cnt = 0; // cnt of rlight created, some ff_light will be skipped
      // Setup dc_lightlist, as to 3dfloor heights, lights, and extra_colormap.
      // highest light to lowest light, [0] is sector light at top
      for(i = 0; i < dc_numlights; i++)
      {
        // [WDJ] This makes the dc_lightlist shorter in the render loop, but the resultant
        // entries dc_lightlist[i] will not correspond to the sector lightlist[i].
        // All references to ff_light must be done here.
        ff_light = &frontsector->lightlist[i];
        rlight = &dc_lightlist[cnt];

        if(i != 0)
        {
          if(ff_light->height < frontsector->floorheight)
            continue;

          if(ff_light->height > frontsector->ceilingheight)
            if(i+1 < dc_numlights && frontsector->lightlist[i+1].height > frontsector->ceilingheight)
              continue;
        }
        rlight->height = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul( FIXED_TO_HEIGHTFRAC(ff_light->height - viewz), rw_scale );
        rlight->heightstep = - FixedMul (rw_scalestep, FIXED_TO_HEIGHTFRAC(ff_light->height - viewz) );
        rlight->flags = ff_light->flags;
        if(ff_light->caster && ff_light->caster->flags & FF_SOLID)
        {
#if 0
          // [WDJ] At some time this became unused.
          lightheight = (*ff_light->caster->bottomheight > frontsector->ceilingheight) ?
              frontsector->ceilingheight + FRACUNIT
             : *ff_light->caster->bottomheight;
#endif
          // in screen coord.
          rlight->botheight = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul( FIXED_TO_HEIGHTFRAC(*ff_light->caster->bottomheight - viewz), rw_scale);
          rlight->botheightstep = - FixedMul (rw_scalestep, FIXED_TO_HEIGHTFRAC(*ff_light->caster->bottomheight - viewz) );
        }

#if 1
// Was in SegLoop, which was an error, because the dc_lightlist entries do NOT correspond to
// the sector lightlist[i] entries.
// From R_Prep3DFloors:
//   ff_light->caster = ff_floor, NULL for top floor
//   ff_light->caster->flags = ff_floor->flags = ff_light->flags = rlight->flags
//   ff_light->height = sector->ceilingheight + 1;
        // [WDJ] Cannot figure out what the height test accomplishes.
        // This seems to be close to the original, whatever it did.
        if( ff_light->caster && ff_light->caster->flags & FF_FOG
            && (ff_light->height != *ff_light->caster->bottomheight))
            rlight->flags |= FF_FOG;
#endif

        rlight->lightlevel = *ff_light->lightlevel;
        rlight->extra_colormap = ff_light->extra_colormap;

        cnt++;
      }
      dc_numlights = cnt;
    }

    if(numffplane)
    {
      for(i = 0; i < numffplane; i++)
      {
        ffplane[i].front_pos = FIXED_TO_HEIGHTFRAC( ffplane[i].front_pos );
        ffplane[i].front_step = FixedMul(-rw_scalestep, ffplane[i].front_pos);
        ffplane[i].front_frac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul(ffplane[i].front_pos, rw_scale);
      }
    }

    if (backsector)
    {
        worldbacktop = FIXED_TO_HEIGHTFRAC( worldbacktop );
        worldbackbottom = FIXED_TO_HEIGHTFRAC( worldbackbottom );
        
        if (worldbacktop < worldtop)
        {
            pixhigh = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul (worldbacktop, rw_scale);
            pixhighstep = -FixedMul (rw_scalestep,worldbacktop);
        }
        
        if (worldbackbottom > worldbottom)
        {
            pixlow = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul (worldbackbottom, rw_scale);
            pixlowstep = -FixedMul (rw_scalestep,worldbackbottom);
        }

        i = 0;

        if(backsector->ffloors)
        {
            ffloor_t * bff; // backsector fake floor
            for(bff = backsector->ffloors; bff; bff = bff->next)
            {
                if(!(bff->flags & (FF_OUTER_PLANES|FF_INNER_PLANES))
                   || !(bff->flags & FF_EXISTS))
                  continue;

                fixed_t bff_bh = *bff->bottomheight;
                if(   bff_bh <= backsector->ceilingheight
                   && bff_bh >= backsector->floorheight
                   && ((viewz < bff_bh && (bff->flags & FF_OUTER_PLANES))
                       || (viewz > bff_bh && (bff->flags & FF_INNER_PLANES))) )
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = FIXED_TO_HEIGHTFRAC(bff_bh - viewz);
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }

                fixed_t bff_th = *bff->topheight;
                if(   bff_th >= backsector->floorheight
                   && bff_th <= backsector->ceilingheight
                   && ((viewz > bff_th && (bff->flags & FF_OUTER_PLANES))
                       || (viewz < bff_th && (bff->flags & FF_INNER_PLANES))) )
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = FIXED_TO_HEIGHTFRAC(bff_th - viewz);
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }
            }
#ifdef DEBUG_STOREWALL   
            if( i > 0 )  GenPrintf( EMSG_debug, "w: backsector ffloors = %i\n", i );
#endif
        } // if(backsector->ffloors)
        else if(frontsector && frontsector->ffloors)
        {
            ffloor_t * fff; // frontsector fake floor
            for(fff = frontsector->ffloors; fff; fff = fff->next)
            {
                if(!(fff->flags & (FF_OUTER_PLANES|FF_INNER_PLANES))
                   || !(fff->flags & FF_EXISTS))
                  continue;

                fixed_t fff_bh = *fff->bottomheight;
                if(   fff_bh <= frontsector->ceilingheight
                   && fff_bh >= frontsector->floorheight
                   && ((viewz < fff_bh && (fff->flags & FF_OUTER_PLANES))
                       || (viewz > fff_bh && (fff->flags & FF_INNER_PLANES))) )
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = FIXED_TO_HEIGHTFRAC(fff_bh - viewz);
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }

                fixed_t fff_th = *fff->topheight;
                if(   fff_th >= frontsector->floorheight
                   && fff_th <= frontsector->ceilingheight
                   && ((viewz > fff_th && (fff->flags & FF_OUTER_PLANES))
                       || (viewz < fff_th && (fff->flags & FF_INNER_PLANES))) )
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = FIXED_TO_HEIGHTFRAC(fff_th - viewz);
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = FIXED_TO_HEIGHTFRAC(centeryfrac) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }
            }
#ifdef DEBUG_STOREWALL   
            if( i > 0 )  GenPrintf( EMSG_debug, "w: backsector ffloors = %i\n", i );
#endif
        }  // if backsector->ffloors
    } // if backsector

    // get a new or use the same visplane
    if (markceiling)
    {
      // visplane global parameter vsp_ceilingplane
      if(vsp_ceilingplane) //SoM: 3/29/2000: Check for null ceiling planes
        vsp_ceilingplane = R_CheckPlane (vsp_ceilingplane, rw_x, rw_stopx-1);
      else
        markceiling = 0;
    }

    // get a new or use the same visplane
    if (markfloor)
    {
      // visplane global parameter vsp_floorplane
      if(vsp_floorplane) //SoM: 3/29/2000: Check for null planes
        vsp_floorplane = R_CheckPlane (vsp_floorplane, rw_x, rw_stopx-1);
      else
        markfloor = 0;
    }

    ds_p->sector_drawseg = first_subsec_seg;

    if( numffplane )
    {
      if( first_subsec_seg == NULL )
      {
        first_subsec_seg = ds_p;
#ifdef DEBUG_STOREWALL   
        GenPrintf( EMSG_debug, "  FIRST SEG FFLOOR: first_subsec_seg=%i, num ffloors= %i\n", (first_subsec_seg - drawsegs), numffplane );
#endif
        // Fill-in first_subsec_seg information.
        // Create the draw_ffplane.  Only where there are ffloor.
        draw_ffplane_t *  dffp = create_draw_ffplane( numffplane );
        ds_p->draw_ffplane = dffp;

        // Ref to ffloor
        // These ffloor will be further altered by R_ExpandPlane.
        // The ffplane[].plane also ref to ffloor.
        for(i = 0; i < numffplane; i++)
          dffp->ffloorplanes[i] = ffplane[i].plane = R_CheckPlane(ffplane[i].plane, rw_x, rw_stopx - 1);

        // [WDJ] The only place that calls R_RenderSegLoop, where backscale is filled in.
        // For now, get the entire vid width.
        dffp->backscale_r = R_get_backscale_ref( vid.width );

      }
      else
      {
#ifdef DEBUG_STOREWALL   
        GenPrintf( EMSG_debug, " EXPAND PLANE: first_subsec_seg= %i, [%i..%i]\n", (first_subsec_seg - drawsegs), rw_x, rw_stopx-1 );
#endif
        for(i = 0; i < numffplane; i++)
          R_ExpandPlane(ffplane[i].plane, rw_x, rw_stopx - 1);
      }
    }

    // [WDJ] Intercept overflow in math
    if( bottomfrac < topfrac )
    {
       // Enable to see where this happens.
//       debug_Printf("Overflow in call: bottomfrac(%i) < topfrac(%i)\n", bottomfrac, topfrac );
       return;
    }

#ifdef BORIS_FIX
    if( linedef->splats && cv_splats.EV )
    {
        // SoM: Isn't a bit wasteful to copy the ENTIRE array for every drawseg?
        memcpy(&last_ceilingclip[ds_p->x1], &ceilingclip[ds_p->x1], sizeof(short) * (ds_p->x2 - ds_p->x1 + 1));
        memcpy(&last_floorclip[ds_p->x1], &floorclip[ds_p->x1], sizeof(short) * (ds_p->x2 - ds_p->x1 + 1));
        R_RenderSegLoop ();
        R_DrawWallSplats ();
    }
    else
    {
        R_RenderSegLoop ();
    }
#else
    R_RenderSegLoop ();
#ifdef WALLSPLATS
    if (linedef->splats)
        R_DrawWallSplats ();
#endif
#endif
    colfunc = basecolfunc;


    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
        && !ds_p->spr_topclip)
    {
        int16_t * ma = get_pool16_array( rw_stopx - start );
        memcpy( ma, &ceilingclip[start], (rw_stopx - start) * sizeof(int16_t));
        ds_p->spr_topclip = ma - start;
    }

    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
        && !ds_p->spr_bottomclip)
    {
        int16_t * ma = get_pool16_array( rw_stopx - start );
        memcpy( ma, &floorclip[start], (rw_stopx - start) * sizeof(int16_t));
        ds_p->spr_bottomclip = ma - start;
    }

    if (maskedtexture && !(ds_p->silhouette&SIL_TOP))
    {
        ds_p->silhouette |= SIL_TOP;
        // midtexture, 0=no-texture, otherwise valid
        ds_p->sil_top_height = sidedef->midtexture ? FIXED_MIN: FIXED_MAX;
    }
    if (maskedtexture && !(ds_p->silhouette&SIL_BOTTOM))
    {
        ds_p->silhouette |= SIL_BOTTOM;
        // midtexture, 0=no-texture, otherwise valid
        ds_p->sil_bottom_height = sidedef->midtexture ? FIXED_MAX: FIXED_MIN;
    }
    ds_p++;
}
