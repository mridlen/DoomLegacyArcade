// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_data.c 1719 2025-01-25 05:52:31Z wesleyjohnson $
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
// $Log: r_data.c,v $
// Revision 1.31  2003/05/04 04:12:54  sburke
// Replace memcpy with memmove, to prevent a misaligned access fault on sparc.
//
// Revision 1.30  2002/12/13 19:22:12  ssntails
// Fix for the Z_CheckHeap and random crashes! (I hope!)
//
// Revision 1.29  2002/01/12 12:41:05  hurdler
// Revision 1.28  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.27  2001/12/27 22:50:25  hurdler
// fix a colormap bug, add scrolling floor/ceiling in hw mode
//
// Revision 1.26  2001/08/13 22:53:40  stroggonmeth
// Revision 1.25  2001/03/21 18:24:39  stroggonmeth
// Revision 1.24  2001/03/19 18:52:01  hurdler
//
// Revision 1.23  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.22  2000/11/04 16:23:43  bpereira
//
// Revision 1.21  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.20  2000/10/04 16:19:23  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.19  2000/09/28 20:57:17  bpereira
//
// Revision 1.18  2000/08/11 12:25:23  hurdler
// latest changes for v1.30
//
// Revision 1.17  2000/07/01 09:23:49  bpereira
// Revision 1.16  2000/05/03 23:51:01  stroggonmeth
// Revision 1.15  2000/04/23 16:19:52  bpereira
// Revision 1.14  2000/04/18 17:39:39  stroggonmeth
//
// Revision 1.13  2000/04/18 12:54:58  hurdler
// software mode bug fixed
//
// Revision 1.12  2000/04/16 18:38:07  bpereira
// Revision 1.11  2000/04/15 22:12:58  stroggonmeth
//
// Revision 1.10  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.9  2000/04/08 17:45:11  hurdler
// fix some boom stuffs
//
// Revision 1.8  2000/04/08 17:29:25  stroggonmeth
// Revision 1.7  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.6  2000/04/07 01:39:53  stroggonmeth
// Fixed crashing bug in Linux.
// Made W_ColormapNumForName search in the other direction to find newer colormaps.
//
// Revision 1.5  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.4  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
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
//      Preparation of data for rendering,
//      generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // thinker
#include "p_setup.h" //levelflats
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "r_data.h"
#include "w_wad.h"
  // numwadfiles
#include "z_zone.h"
#include "v_video.h" //pLocalPalette
#include "m_swap.h"


// [WDJ] debug flat
//#define DEBUG_FLAT

// [WDJ] Generate texture controls

// [WDJ] For y clipping to be technically correct, the pixels in the source
// post must be skipped. To maintain compatibility with the original doom
// engines, which had this bug, other engines do not skip the post pixels either.
// Enabling corrected_clipping will make some textures slightly different
// than displayed in other engines.
// TEKWALL1 will have two boxes in the upper left corner with this off and one
// box with it enabled.  The differences in other textures are less noticable.
// This only affects software rendering, hardware rendering is correct.
// see: cv_corr_clip_width


//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

int             firstflat, lastflat, numflats;
int             firstpatch, lastpatch, numpatches;


// textures
int             numtextures=0;      // total number of textures found,
// size of following tables

// Array [ num_textures ], owned, Z_Malloc
texture_t**     textures = NULL;

// [WDJ] To reduce the repeated indexing using texture id num, better locality of reference.
// To allow creating a texture that was not loaded from textures[].
// Holds all the software render information.
// Array [ num_textures ], owned, Z_Malloc
texture_render_t * texture_render = NULL;

// Array [ num_textures ], owned, Z_Malloc
fixed_t*        textureheight;      // needed for texture pegging


#if 0
int       *flattranslation;             // for global animation
#endif
// Array [ num_textures+1 ], owned, Z_Malloc
int *  texturetranslation;

// needed for pre rendering
spritelump_t *  spritelumps = NULL;  // array of sprite lump values, endian swapped
int             num_spritelump = 0;  // already allocated
int             num_free_spritelump = 0;  // free for allocation

// colormap lightmaps from wad COLORMAP lump
lighttable_t *  reg_colormaps;


//faB: for debugging/info purpose
int             flatmemory;
int             spritememory;
int             texturememory;	// all textures


//faB: highcolor stuff
// [WDJ] 2012-02-06 shared for DRAW15, DRAW16, DRAW24, DRAW32
union color8_u   color8;  // remap color index to rgb value
#ifdef ENABLE_DRAW8_USING_12
byte  color12_to_8[ 0x1000 ];
#endif


//  src_type : TM_row_image (pic_t), TM_column_image (patch, picture)
//  src_data : source data
//  bytepp : source pixel size in bytes
//  sel_offset  : offset into pixel, 0..3
//  blank_value : pixel value that is blank space, >255 = no blank pixel value
//  out_type :  TM_patch, TM_picture, TM_column_image
//  out_flags : in created patch
//    CPO_blank_trim : trim blank columns
//  out_header : temp patch header for width and offset
byte * R_Create_Patch( unsigned int width, unsigned int height,
             /*SRC*/   byte src_type, byte * src_data, byte bytepp, byte sel_offset, uint16_t blank_value,
             /*DEST*/  byte out_type, byte out_flags, patch_t * out_header )
{
    byte  postbuf[ 1024 ];
    unsigned int  head_empty_columns = 0;  // left
    unsigned int  tail_empty_columns = 0;  // right
    unsigned int  mid_empty_columns = 0;
    unsigned int  count_good_columns = 0;
    unsigned int  colofs_size, head_size, wb_blocksize;
    signed int    leftoffset = 0;
    unsigned int  col, length, dest_used, max_length;
    unsigned int  row_inc, col_inc;
    byte          empty_column = 0;
    byte          out_patch_header, out_columnofs, out_post;
    byte       *  wb_dest;  // output, ZMalloc
    uint32_t   *  colofs0 = NULL;
    post_t     *  destpost = NULL;
    byte       *  dest_term = NULL;
    byte       *  destp;
    byte       *  pb;  // in postbuf
    byte       *  src0, * src_end, * src ;

    // Source
    if( src_type == TM_column_image )
    {
        row_inc = bytepp;  // row to row, along col
        col_inc = bytepp * height;  // col to col
    }
    else  // TM_row_image
    {
        col_inc = bytepp;  // col to col, along row
        row_inc = bytepp * width;  // row to row
    }
   
    out_patch_header = out_columnofs = out_post = 0;
    switch( out_type )
    {
     case TM_patch:
        out_patch_header = out_columnofs = out_post = 1;
        break;
     case TM_picture:
        out_columnofs = 1;
        break;
     case TM_column_image:
     default:
	break;
    }
   
    // Dest   
    head_size = 0;
    colofs_size = 0;
    dest_used = 0;
    if( out_patch_header )
    {
        head_size += 8;
    }
    if( out_columnofs )
    {
        colofs_size = width * sizeof( uint32_t );  // width * 4
        head_size += colofs_size;
    }
    wb_blocksize = head_size + (width * height);  // guess
    max_length = 1024;  // buffer length
    if( out_post )
    {
        wb_blocksize += 4;
        max_length = 255;
    }

    wb_dest = (byte*) Z_Malloc( wb_blocksize, PU_IN_USE, NULL );

    if( out_patch_header )
    {
        patch_t * wb_patch = (patch_t*) wb_dest;
	wb_patch->width = width;
        wb_patch->height = height;
        wb_patch->leftoffset = 0;
        wb_patch->topoffset = 0;
        colofs0 = & wb_patch->columnofs[0];
    }
    else if( out_columnofs )
    {
        colofs0 = (uint32_t*) wb_dest;  // no header
    }
   
    destp = wb_dest + head_size;  // posting area
    for( col=0; col<width; col++ )
    {
        destpost = NULL;
        empty_column = 1;  // flag column with no-posts value
        if( colofs0 )
        {
            colofs0[col] = 0;  // header entry for this column
        }
        src = src0 = & src_data[ col * col_inc ];
        src_end = src0 + (row_inc * (height-1)) + 1;  // past last byte of column
        while( src < src_end )
        {
            // traverse column
            for( ; src < src_end ; src += row_inc )
            {
                if( src[sel_offset] != blank_value )
                    goto start_post; // find first non blank pixel
            }
            // only blanks found
            if( empty_column )  // if no posts, then will need to fix later
            {
                if( count_good_columns )
                    tail_empty_columns++;
                else
                    head_empty_columns++;
            }
            break;

        start_post:
            // Must be at least one pixel, because blank span found a non-blank.
            // Enter into header
            if( empty_column )
            {
	        if( colofs0 )
                {
                    colofs0[col] = destp - wb_dest; // offset within patch
                }
                // maintain empty column count
                count_good_columns ++;
                mid_empty_columns += tail_empty_columns;
                tail_empty_columns = 0;
	        empty_column = 0;
            }

            dest_used = destp - wb_dest;
            if( out_post )
            {
                unsigned int topdelta = (src - src0) / row_inc;
                if( topdelta > 254 )  // topdelta would be too large
                    break;  // skip rest of height

                destpost = (post_t*) destp;  // posting area
                destpost->topdelta = topdelta;
                dest_used += 5;  // post + 2 pad + term
            }

            // traverse source column
            length = 0;	    
	    pb = postbuf;  // must use a buffer to convert row indexing to column indexing
            for( ; src < src_end ; src += row_inc )
            {
                if(src[sel_offset] == blank_value)  break;  // find first blank pixel
                if( length >= max_length )  break;  // max post length
                length++;
	        *(pb++) = src[sel_offset];  // into buffer
            }
            // src must point to next, not included in this post

            dest_used += length;
            if( dest_used + 8 > wb_blocksize )
            {
                // Will not fit in the allocation
                // Copy to new allocation.
                unsigned int  wb2_len = dest_used + 1024;  // allocation increment
                byte * wb2 = (byte*) Z_Malloc( wb2_len, PU_IN_USE, NULL );
                intptr_t  adjustdiff = (void*) wb2 - (void*) wb_dest;  // byte difference in locations

                // Move data to wb2
                memcpy( wb2, wb_dest, wb_blocksize );

                // Release old allocation
                Z_Free( wb_dest );
                wb_dest = wb2;
                wb_blocksize = wb2_len;

                // Move ptrs to wb2
                destp += adjustdiff;
	        if( colofs0 )  colofs0 += adjustdiff;
	        if( destpost )  destpost += adjustdiff;
            }

            // Form postbuf into a column post.
            if( destpost )
            {
                destpost->length = length;
                destp[2] = 0;	// pad 0
                destp += 3;  // post pixel data
            }

	    // Copy must be after allocation size check.
            memcpy( destp, postbuf, length );
            destp += length;

            if( destpost )
            {
                *(destp++) = 0; // pad 0
                // Keep ptr to term, for empty columns.
		dest_term = destp;
                *dest_term = 0xFF; // term, may get overwritten by next post
                // next source colpost, adv by (length + 2 byte header + 2 extra bytes)
                // next post overwrites previous term
            }
        }
        // start new column
        if( destpost )
        {
            destp ++;  // skip 0xFF column termination
            dest_used++;
        }
    }

    if( colofs0 && dest_term && mid_empty_columns )
    {
        // Must fix colofs that are still 0.
        // Point column at last 0xFF written.
        unsigned int term_offset = dest_term - wb_dest;  // offset of last 0xFF
        for( col=0; col<width; col++ )
        {
            if( colofs0[col] == 0 )
                colofs0[col] = term_offset;
        }
    }

    if( (out_flags & CPO_blank_trim) && ((head_empty_columns + tail_empty_columns) > 0) )
    {
        unsigned int  new_head_size = 0;
        // trim off the empty columns
        unsigned int  trim_width = head_empty_columns + tail_empty_columns;
        width = width - trim_width;

        if( out_patch_header )
            new_head_size += 8;

        if( colofs0 )  // out_patch_header or out_columnofs
        {
            unsigned int  new_colofs_size = width * sizeof(uint32_t);  // width * 4
            unsigned int  adjustcol = colofs_size - new_colofs_size;  // byte difference in colorofs locations
            new_head_size += new_colofs_size;

            if( head_empty_columns )
            {
                // remove head_empty_columns of colofs table
                memmove( colofs0, & colofs0[head_empty_columns], new_colofs_size );
            }

            // Move ptrs to new positions
            for( col=0; col<width; col++ )  // new width
            {
                colofs0[col] -= adjustcol;
            }

            // move data due to smaller columnofs
            memmove( wb_dest + new_head_size, wb_dest + head_size, dest_used - head_size );
            // colofs_size = new_colofs_size;  // unnecessary
        }

        dest_used = dest_used - head_size + new_head_size;

        leftoffset -= head_empty_columns;
        if( out_patch_header )
	{
            // update
            patch_t * wb_patch = (patch_t*)  wb_dest;
            wb_patch->leftoffset = leftoffset;
            wb_patch->width = width;
        }
    }

    // Resize if necessary
    if( dest_used + 32 < wb_blocksize )
    {
        // excessive allocation
        // Copy to new allocation.
        byte * wb2 = (byte*) Z_Malloc( dest_used, PU_IN_USE, NULL );
        memcpy( wb2, wb_dest, dest_used );
        // Release old allocation
        Z_Free( wb_dest );
        wb_dest = wb2;
    }

    if( out_header )
    {
        out_header->height = height;
        out_header->width = width;
        out_header->leftoffset = leftoffset;
        out_header->topoffset = 0;
    }
    return wb_dest;
}


//  Fixed, solid, image.
//  column_oriented : source data orientation, 0 = row x column (image), 1 = column x row (pic_t)
//  data : source data of width x height (in rows)
byte * R_Create_Image( unsigned int width, unsigned int height, byte column_oriented, byte * data )
{
    byte  postbuf[ 256 ];
    unsigned int  wb_blocksize = (width * height);
    unsigned int  col, length;
    unsigned int  row_inc, col_inc;
    byte       *  pb;
    byte       *  src ;
    byte       *  dest;

    byte * wb_image = Z_Malloc( wb_blocksize, PU_IN_USE, NULL );
   
    if( column_oriented )
    {
        row_inc = 1;  // row to row, along col
        col_inc = height;  // col to col
    }
    else
    {
        col_inc = 1;  // col to col, along row
        row_inc = width;  // row to row
    }
   
    dest = wb_image;  // posting area
    for( col=0; col<width; col++ )
    {
        src = & data[ col * col_inc ];

        // Copy column
        pb = postbuf;  // put into postbuf
        for( ;  ; src += row_inc )
        {
            if( pb >= & postbuf[height] )  break;  // max post length
            *(pb++) = *src;
        }
        length = pb - postbuf;  // must be at least 1

        // Form postbuf into a column post.
        memcpy( dest, postbuf, length );
        if( length < height )
        {
            memset( &dest[length], 0, height - length ) ;
        }

        dest += height;
    }

    return wb_image;
}


#if 0
// indexed by pic_selection_e
byte pic_pixel_offset_table[ 3 ] = { 0, 0, 1 };

// indexed by pic_mode_t
// byte per pixel
byte pic_bytepp_table[ 5 ] = { 1, 1, 2, 3, 4 };

typedef enum {
    PM_PALETTE,
    PM_INTENSITY,
    PM_ALPHA,
} patch_mode_e;

byte pic_operation_table[5][3] =
{
  // 255 cannot be done.
  { // Have PALETTE
    // Want PM_PALETTE, PM_INTENSITY, PM_ALPHA
    0, 40, 40
  },
  { // Have INTENSITY
    // Want PM_PALETTE, PM_INTENSITY, PM_ALPHA
    255, 0, 0
  },
  { // Have INTENSITY_ALPHA
    // Want PM_PALETTE, PM_INTENSITY, PM_ALPHA
    255, 0, 1
  },
  { // Have RGBA24
    // Want PM_PALETTE, PM_INTENSITY, PM_ALPHA
    49, 48, 255
  },
  { // Have RGBA32
    // Want PM_PALETTE, PM_INTENSITY, PM_ALPHA
    49, 48, 3
  }
}

//  mode : patch_mode_e
patch_t * R_Pic_to_Patch( pic_t * pic, byte patch_mode )
{
    byte picmode = pic->mode;
    byte sel_offset = 0;
    byte blank_value = 0;
    byte operation = pic_operation_table[picmode][patch_mode];
    byte bytepp = pic_bytepp_table[ picmode ];
   
    if( operation == 255 )
        return NULL;

    if( operation <= 4 )
    {
        sel_offset = operation;
    }
    else
    {
        // destructive operation, put into 0 byte
        byte * w;
        byte * w_end = &pic->data[pic->width * pic->height]
        for( w= &pic->data[0]; w < w_end; w += bytepp )
        {
            RGBA_t p;
            switch( picmode )
            {
             case PALETTE:
               p = pLocalPalette[w[0]];
               p.s.alpha = 0;
               break;
             case INTENSITY:
               p.s.red = p.s.green = p.s.blue = w[0];
               p.s.alpha = 0;
               break;
             case INTENSITY_ALPHA:
               p.s.red = p.s.green = p.s.blue = w[0];
               p.s.alpha = w[1];
               break;
             case RGBA24:
               p.s.red   = w[0];
               p.s.green = w[1];
               p.s.blue  = w[2];
               p.s.alpha = 0;
               break;
             case RGBA32:
               p = ((RGBA32_t*)w)[0];
               break;
            }
            switch( patch_mode )
            {
             case PM_PALETTE:
               w[0] = NearestColor(p.s.red, p.s.green, p.s.blue);
               break;
             case PM_INTENSITY:
               w[0] = (p.s.red + p.s.green + p.s.blue) / 3;
               break;
             case PM_ALPHA:
               w[0] = p.s.alpha;
               break;
            }
        }
    }

    return  R_Create_Patch( pic->width, pic->height, 1, & pic->data[0],
                            bytepp, sel_offset, blank_value );
}
#endif


#if 0
// [WDJ] This description applied to previous code. It is kept for historical
// reasons. We may have to restore some of this functionality, but this scheme
// would not work with z_zone freeing cache blocks.
// 
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_t generated.
//


#endif


// [WDJ] Need extra texture_render when need both picture and patch for a texture.
// This will not be used most of the time, but when it is needed, there
// is no way around it.  When that happens there will likely be several textures
// involved, because it is triggered by how the textures are used in the wad.

#ifdef RENDER_TEXTURE_EXTRA_FULL
// Lost Civilization Map04 uses an arch texture as a wall texture, and a masked texture,
// right next to each other.  Only the solid portion of the arch is used on the wall,
// but the drawer cannot predict that, so both 1s and 2s textures must be supported at
// the same time.
// This may become a popular technique as the vertical pillar of the arch can be solid wall.

// The texture_render_t will hold a Z_Malloc, and cannot be moved nor reallocated.
// The array of ptrs can be realloc-ed.
static texture_render_t * *  texture_render_extra = NULL;  // array of ptr
static byte texture_render_extra_alloc = 0;  // number allocated
static byte texture_render_extra_count = 0;  // number used
#endif

// One extra texren, for all uses.  Will thrash when have too many needy textures.
static int               common_texren_texture_num = 0;
static unsigned int      common_texren_usage_count = 0;  // number of times flushed
static texture_render_t  common_texren = { 0, 0 };  // reused

// Cannot return NULL.
texture_render_t *  R_Get_extra_texren( int texture_num, texture_render_t * base_texren, byte req_model, byte req_detect )
{
#ifdef RENDER_TEXTURE_EXTRA_FULL
    texture_render_t *  trep;
    int index = base_texren->extra_index;  // 1..

    // If existing matching texture cache entry.
    if( index && (index <= texture_render_extra_count) && texture_render_extra )
    {
        // There is an existing extra.
	trep = texture_render_extra[ index - 1 ];
	
        if( trep->texture_model == req_model )
	    goto done;

        if( trep->detect & req_detect )
            goto done;

        // do not need more than one extra, for now.
        // Something is wrong, so instead of generating many extra, just use the common.
        goto use_common_texren;  // exceeds what can be saved as index
    }
   
    if( texture_render_extra_count > 253 )
        goto use_common_texren;  // exceeds what can be saved as index
   
    if( texture_render_extra_count >= texture_render_extra_alloc )
    {
        // Alloc the array of ptrs.
        int new_alloc = texture_render_extra_alloc + 8;
        void * new_array = realloc( texture_render_extra, sizeof( texture_render_t* ) * new_alloc );
        if( new_array == NULL )
            goto use_common_texren;

        texture_render_extra = new_array;
	texture_render_extra_alloc = new_alloc;
    }
    
    // Not found, create new.
    trep = (texture_render_t*) malloc( sizeof(texture_render_t) );
    if( trep == NULL )
        goto use_common_texren;
   
    memset( trep, 0, sizeof(texture_render_t) );
    trep->texture_model = req_model;
    trep->width = base_texren->width;

    texture_render_extra[ texture_render_extra_count ] = trep;
    // link to base_texren
    base_texren->extra_index = ++ texture_render_extra_count;  // 1..
    
done:   
    return trep;

#endif

#ifdef RENDER_TEXTURE_EXTRA_FULL
use_common_texren:
#endif
    // Use the common_texren as a fallback.
    // This may thrash if the wad design was sloppy.
    if( (common_texren_texture_num != texture_num)
        || !(common_texren.detect & req_detect) )
    {
        if( common_texren_usage_count < 0xFFFFFFFF )
            common_texren_usage_count ++;
        // Wrong content, so clear it.
        if( common_texren.cache )
            Z_Free( common_texren.cache );

        common_texren.width = base_texren->width;
        common_texren.width_tile_mask = base_texren->width_tile_mask;
        common_texren.detect = base_texren->detect;
    }

    return & common_texren; // must return something
}

static
void  R_Release_all_extra_texren( void )
{
    if( verbose && common_texren_usage_count )
    { 
        GenPrintf( EMSG_ver, "Common Extra texren used: %i\n", common_texren_usage_count );
        common_texren_usage_count = 0;
    }

#ifdef RENDER_TEXTURE_EXTRA_FULL
    if( texture_render_extra == NULL )
        return;
   
    if( verbose )
        GenPrintf( EMSG_ver, "Extra texren used: %i\n", texture_render_extra_count );

    int i;
    for( i=0; i<texture_render_extra_count; i++ )
    {
        texture_render_t * trep = texture_render_extra[i];
        if( trep )
        {
            if( trep->cache )
	        Z_Free( trep->cache );
            free( trep );
        }
    }

    free( texture_render_extra );
    texture_render_extra_alloc = texture_render_extra_count = 0;
    texture_render_extra = NULL;
#endif
}



#if 0
// Some unique Texture setup for installing a patch.
void  R_Set_Texture_Patch( int texnum, patch_t * patch )
{
    uint32_t*  colofs;  // to match size in wad

    if( texturecache[texnum] )
        Z_Free( texturecache[texnum] );

    texturecache[texnum] = (byte*) patch;

    // determine width power of 2
    int j = 1;
    while (j*2 <= patch->width)  j<<=1;
    texturewidthmask[texnum] = j-1;
    textureheight[texnum] = patch->height<<FRACBITS;
   
    colofs = &(patch->columnofs[0]);;
#ifdef COLOFS_PLUS_3
    if( colofs[0] == ((patch->width * sizeof(uint32_t)) + 8) )
    {
        // offset to pixels instead of post header
        // Many callers will use colofs-3 to get back to header, but
        // some drawing functions need pixels.
        int i;
        for (i=0; i<patch->width; i++)
            colofs[i] = colofs[i] + 3;  // adjust colofs from wad
    }
#endif
    texturecolumnofs[texnum] = colofs;
}
#endif


// [WDJ] 2/5/2010
// See LITE96 originx=-1, LITERED originx=-4, SW1WOOD originx=-64
// See TEKWALL1 originy=-27, STEP2 originy=-112, and other such textures in doom wad.
// Patches leftoffset and topoffset are ignored when composing textures.

// The original doom has a clipping bug when originy < 0.
// The patch position is moved instead of clipped, the height is clipped.

//
// R_DrawColumnInCache
// Clip and draw a column from a patch into a cached post.  Dest is in columns.
//

static
void R_DrawColumnInCache ( column_t*     colpost,	// source, list of 0 or more post_t
                           byte*         cache,		// dest
                           int           originy,
                           int           cacheheight )  // limit
{
    int         count;
    int         position;  // dest
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches,
    int   cur_topdelta = -1;
#endif
    byte*       source;
//    byte*       dest;

//    dest = (byte *)cache;// + 3;

    // Assemble a texture from column post data from wad lump.
    // Column is a series of posts (post_t), terminated by 0xFF
    while (colpost->topdelta != 0xff)	// end of posts
    {
        // post has 2 byte header (post_t), 
        // and has extra byte before and after pixel data
        source = (byte *)colpost + 3;	// pixel data after post header
        count = colpost->length;

#ifdef DEEPSEA_TALL_PATCH
        // When the column topdelta is <= the current topdelta,
        // it is a DeePsea tall patch relative topdelta.
        int topdelta = colpost->topdelta;
        if( topdelta <= cur_topdelta )
        {
             // DeePsea relative topdelta
             topdelta += cur_topdelta;
        }
        cur_topdelta = topdelta;
        position = originy + topdelta;  // position in dest
#else
        position = originy + colpost->topdelta;  // position in dest
#endif

        if (position < 0)
        {
            count += position;  // skip pixels off top
            // [WDJ] For this clipping to be technically correct, the pixels
            // in the source must be skipped too.
            // Other engines do not skip the post pixels either, to maintain
            // compatibility with the original doom engines, which had this bug.
            // Enabling this will make some textures slightly different
            // than displayed in other engines.
            // TEKWALL1 will have two boxes in the upper left corner with this
            // off and one box with it enabled.  The differences in other
            // textures are less noticable.
            if( cv_corr_clip_width.EV )
            {
                source -= position; // [WDJ] 1/29/2010 skip pixels in post
            }
            position = 0;
        }

        if (position + count > cacheheight)  // trim off bottom
            count = cacheheight - position;

        // copy column (full or partial) to dest cache at position
        if (count > 0)
            memcpy (cache + position, source, count);

        // next source colpost, adv by (length + 2 byte header + 2 extra bytes)
        colpost = (column_t *)(  (byte *)colpost + colpost->length + 4);
    }
}



//
// R_GenerateTexture
//
//   Allocate space for full size texture, either single patch or 'composite'
//   Build the full textures from patches.
//   The texture caching system is a little more hungry of memory, but has
//   been simplified for the sake of highcolor, dynamic lighting, & speed.
//
//   This is not optimized, but it's supposed to be executed only once
//   per level, when enough memory is available.

// Temp structure element used to combine patches into one dest patch.
typedef struct {
    int   nxt_y, bot_y;		// current post segment in dest coord.
    int   width;
    int   originx, originy;	// add to patch to get texture
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches,
    int   cur_topdelta;
#endif
    post_t *  postptr;		// post within that patch
    patch_t * patch;     	// patch source
    // [WDJ] Generate_Texture can now reuse columns (as done in Requiem compacted patches).
    uint32_t  usedpatchdata;    // to detect reuse, compaction
    // [WDJ] Bad patch detection
    int   patchsize;
} compat_t;

#ifdef DEEPSEA_TALL_PATCH
// [MB] [WDJ] DeePsea tall patch.
// DeepSea allows the patch to exceed 254 height.
// A Doom patch has monotonic ascending topdelta values, 0..254.
// DeePsea tall patches have an optional relative delta detected
// when col->topdelta < cur_topdelta.
// When the column topdelta is less than the current topdelta,
// it is a DeePsea tall patch relative topdelta.
#endif



//  texnum : index into textures
//  texture_req : requirement,  TM_none, TM_masked, TM_picture, or TM_picture_column
byte* R_GenerateTexture2 ( int texnum, texture_render_t *  texren, byte  texture_req )
{
    texture_t*          texture; // texture info from wad
    texpatch_t*         texpatch;  // patch info to make texture
    patch_t*            realpatch; // patch lump
    uint32_t*           colofs;  // to match size in wad

    byte     * txcblock; // allocated texture memory
    byte     * texture_end; // end of allocated texture area
    patch_t  * txcpatch; // header of txcblock
    byte     * txcdata;  // posting area
    byte     * destpixels;  // current post pixel in txcdata

    int	txcblocksize;

    // array to hold all patches for combine
# define MAXPATCHNUM 256
    compat_t   compat[MAXPATCHNUM];
    post_t   * srcpost, * destpost;

    unsigned int patchcount;
    unsigned int compostsize;
    byte  detect_post = 0;  // detect patch post incomptible with single column
    byte  detect_hole = 0;
    byte  detect = (texren->detect & TD_masked);
    byte  make_picture = 0;
#ifdef DEEPSEA_TALL_PATCH
    int seg_topdelta;  // current seg_topdelta
#endif
    int patchsize;
    int	colofs_size;
    int x, x1, x2, i, p;
    int postlength;  // length of current post
    int segnxt_y, segbot_y; // may be negative
    int bottom;		// next y in column


    texture = textures[texnum];
    // [WDJ] Do not save GenerateTexture texture_model in the texture.
    // There may be more than one needed, and upon reload the last
    // generated model will appear to be a texture requirement.
    detect |= texture->detect;  // texture hints from texture load

    if( texture_req == TM_none )
        texture_req = texren->texture_model;  // previous generate

    if( texture_req == TM_picture )
        make_picture = 1;

    if( texren->cache )
        Z_Free( texren->cache );

    // Column offset table size as determined by wad specs.
    // Column pixel data starts after the table.
    colofs_size = texture->width * sizeof( uint32_t );  // width * 4
    // to allocate texture column offset lookup

#if 0
    // To debug problems with specific textures
    if( strncmp(texture->name, "TEKWALL5", 8 ) == 0 )
       debug_Printf("GenerateTexture - match\n");
#endif

    // single-patch textures can have holes and may be used on
    // 2sided lines so they need to be kept in 'packed' format
    patchcount = texture->patchcount;

    if( patchcount >= MAXPATCHNUM )
    {
       I_SoftError("R_GenerateTexture: Patch count %i exceeds %i, ignoring rest\n",
                   patchcount, MAXPATCHNUM);
       patchcount = MAXPATCHNUM - 1;
    }

    // [WDJ] Protect every alloc using PU_CACHE from all Z_Malloc that
    // follow it, as that can deallocate the PU_CACHE unexpectedly.
   
    // Texture patch format:
    //   patch header (8 bytes), ignored
    //   array[ texture_width ] of column offset
    //   concatenated column posts, of variable length, terminate 0xFF
    //        ( post header (topdelta,length), pixel data )

    // First examination of the source patches
    compostsize = 0;
    texpatch = texture->patches;
    for (p=0; p<patchcount; p++, texpatch++)
    {
        compat_t * cp = &compat[p];
        cp->postptr = NULL;	// disable until reach starting column
        cp->nxt_y = INT_MAX;	// disable

        // Track patch memory usage to detect reused columns.
        cp->usedpatchdata = 0;
        cp->originx = texpatch->originx;
        cp->originy = texpatch->originy;

        // To avoid hardware render cache.
        realpatch = W_CachePatchNum_Endian(texpatch->lumpnum, PU_IN_USE);  // patch temp
        // Preliminary characteristics of the patch.
        cp->patch = realpatch;
        cp->width = realpatch->width;
        int patch_colofs_size = realpatch->width * sizeof( uint32_t );  // width * 4
        // Need patchsize to detect invalid patches.
        patchsize = W_LumpLength(texpatch->lumpnum);
        cp->patchsize = patchsize;

        // [WDJ] Detect PNG patches.
        if(    ((byte*)realpatch)[0]==137
            && ((byte*)realpatch)[1]=='P'
            && ((byte*)realpatch)[2]=='N'
            && ((byte*)realpatch)[3]=='G' )
        {
            // Found a PNG patch, which will crash the draw8 routine.
            // Must assume could be used for top or bottom of wall,
            // which require the patch run full height (no transparent).
#if 1
            // Enable when want to know which textures are triggering this.
            GenPrintf(EMSG_info,"R_GenerateTexture: Texture %8s has PNG patch.\n", texture->name );
#endif
            goto disable_patch;
        }

        // [WDJ] W_CachePatchNum should only get lumps from PATCH section,
        // but it will return a colormap of the same name.
        // Here are some validity checks, to ensure we are working with a patch.
        // colormap size = 0x2200 to 0x2248
        {
            uint32_t* pat_colofs = (uint32_t*)&(realpatch->columnofs); // to match size in wad
            if( patch_colofs_size + 8 > patchsize )  // column list exceeds patch size
                goto disable_patch;

            // Look at patch columns
            for( i=0; i< realpatch->width; i++ )
            {
                uint32_t cofs = pat_colofs[i];  // column offset
                if( cofs > patchsize )
                    goto disable_patch;  // invalid column offset

                post_t * ppp = (post_t*)( (byte*)realpatch + cofs );
                if( ppp->topdelta || (ppp->length != realpatch->height) )
                    detect_post = TD_odd_post;  // patch is not single column

                // Look for holes
                segbot_y = -1;
#ifdef DEEPSEA_TALL_PATCH
                seg_topdelta = -1;	       
#endif
                while( ppp->topdelta != 0xFF )
                {
                    int topdelta = ppp->topdelta;
#ifdef DEEPSEA_TALL_PATCH
		    if( topdelta <= seg_topdelta )
                    {
                        // DeePsea relative topdelta
                        topdelta += seg_topdelta;
                    }
                    seg_topdelta = topdelta;
#endif
                    if( topdelta > (segbot_y + 1) )    detect_hole = TD_hole;
                    segbot_y = topdelta + ppp->length;
                    ppp = (post_t*)((byte*)ppp + ppp->length + 4);
                }

                // Short texture, such as hanging vines, have holes.
                if( segbot_y < texture->height )    detect_hole = TD_hole;
            }
        }

        // Add posts, without columnofs table and 8 byte patch header.
        compostsize += patchsize - patch_colofs_size - 8;
        continue;

    disable_patch:
        cp->originx = 7999;  // to disable this patch
        cp->patchsize = 0;
        cp->width = 0;
        continue;
    }

    // [WDJ] When rejected PNG patches and bad patches.
    if( compostsize == 0 )  // no valid patches found
    { 
#if 1
        // Enable when want to know which textures are triggering this.
        GenPrintf(EMSG_info,"R_GenerateTexture: Texture %8s has no valid patches, using dummy texture.\n", texture->name );
#endif
        goto make_dummy_texture;
    }

    if( make_picture )
        goto multipatch_combine;

    if( patchcount > 1 )
        goto multipatch_combine;

    if( texture_req == TM_picture_column )
    {
        if( detect_hole )
            goto multipatch_combine;
        if( detect_post )
            goto multipatch_combine;
    }

    if( patchcount==1 )
    {
        // Single patch texture, simplify

        // [WDJ] Detect shifted origin, cannot use the simple copy.
        texpatch = texture->patches;
        if( texpatch->originx != 0 || texpatch->originy != 0 )
        {
            // [WDJ] Cannot copy patch to texture.
            // Fixes tekwall2, tekwall3, tekwall5 in FreeDoom,
            // which use right half of large combined patches.
//	    debug_Printf("GenerateTexture %s: offset forced multipatch\n", texture->name );
            goto multipatch_combine;
        }

        // [WDJ] Only need patch lump for the following memcpy.
        realpatch = compat[0].patch;
        patchsize = compat[0].patchsize;

        // [WDJ] Detect mismatch of patch width, too large.
        if( realpatch->width > texture->width )
        {
            // [WDJ] Texture is a portion of a large patch.
            // To prevent duplicate large copies.
            // Unfortunately this also catches the large RSKY that legacy
            // has in legacy.wad.
            if( strncmp(texture->name, "SKY", 3 ) == 0 )
            {
                // let sky match patch
                texture->height = realpatch->height;
            }
            else
            {
//	        debug_Printf("GenerateTexture %s: width forced multipatch\n", texture->name );
                goto multipatch_combine;
            }
        }
        else

        // [WDJ] Detect mismatch of patch width, too small.
        if( realpatch->width < texture->width )
        {
            // [WDJ] Messy situation. Single patch texture where the patch
            // width is smaller than the texture.  There will be segfaults when
            // texture columns are accessed that are not in the patch.
            // Rebuild the patch to meet draw expectations.
            // This occurs in phobiata and maybe heretic.
            // [WDJ] Too late to change texture size to match patch,
            // the caller can already be violating the patch width.
            // Single-sided textures need to be kept packed to preserve holes.
            // The texture may be rebuilt several times due to cache.
            int patch_colofs_size = realpatch->width * sizeof( uint32_t );  // width * 4
            int ofsdiff = colofs_size - patch_colofs_size;
            // reserve 4 bytes at end for empty post handling
            txcblocksize = patchsize + ofsdiff + 4;
#if 0
            // Enable when want to know which textures are triggering this.
            GenPrintf(EMSG_info,"R_GenerateTexture: single patch width does not match texture width %8s\n",
                   texture->name );
#endif
            txcblock = Z_Malloc (txcblocksize,
                          PU_LEVEL,         // will change tag at end of this function
                          (void**)&texren->cache);

            // patches have 8 byte patch header, part of patch_t
            memcpy (txcblock, realpatch, 8); // header
            memcpy (txcblock + colofs_size + 8, ((byte*)realpatch) + patch_colofs_size + 8, patchsize - patch_colofs_size - 8 ); // posts
            // build new column offset table of texture width
            {
                // Copy patch columnofs table to texture, adjusted for new
                // length of columnofs table.
                uint32_t* pat_colofs = (uint32_t*)&(realpatch->columnofs); // to match size in wad
                colofs = (uint32_t*)&(((patch_t*)txcblock)->columnofs); // new
                for( i=0; i< realpatch->width; i++)
                        colofs[i] = pat_colofs[i] + ofsdiff;
                // put empty post for all columns not in the patch.
                // txcblock has 4 bytes reserved for this.
                int empty_post = txcblocksize - 4;
                txcblock[empty_post] = 0xFF;  // empty post list
                txcblock[empty_post+3] = 0xFF;  // paranoid
                for( ; i< texture->width ; i++ )
                        colofs[i] = empty_post;
            }
            goto single_patch_finish;
        }
       

        {
            // Normal: Most often use patch as it is.
            // texture_render cache gets copy so that PU_CACHE deallocate clears the
            // cache automatically
            txcblock = Z_Malloc (patchsize,
                          PU_IN_USE,  // will change tag at end of this function
                          (void**)&texren->cache);

            memcpy (txcblock, realpatch, patchsize);
            txcblocksize = patchsize;
        }

  single_patch_finish:
        // Textures do not use the realpatch offsets, which may be garbage.
        txcpatch = (patch_t*)txcblock;
        txcpatch->leftoffset = 0;
        txcpatch->topoffset = 0;

        // Interface for texture draw.
        // Use the single patch, single column lookup.
        colofs = (uint32_t*)&(((patch_t*)txcblock)->columnofs);
        // colofs from patch are relative to start of table
        texren->columnofs = colofs;
        texren->pixel_data_offset = 3;  // skip over post header and pad byte
        texren->texture_model = TM_patch;
        //debug_Printf ("R_GenTex SINGLE %.8s size: %d\n",texture->name,patchsize);
        if( (detect_hole | detect_post) == 0 )  detect |= TD_1s_ready;
        detect |= TD_2s_ready;
        goto done;
       
        // [WDJ] Dummy texture generation.
  make_dummy_texture:
        {
            // make a dummy texture
            int head_size = colofs_size + 8;
            txcblocksize = head_size + 4 + texture->height + 4;
            // will change tag at end of this function
            txcblock = Z_Malloc (txcblocksize, PU_IN_USE, (void**)&texren->cache);
            patch_t * txcpatch = (patch_t*) txcblock;
            txcpatch->width = texture->width;
            txcpatch->height = texture->height;
            txcpatch->leftoffset = 0;
            txcpatch->topoffset = 0;
            destpost = (post_t*) ((byte*)txcblock + head_size);  // posting area;
            destpost->topdelta = 0;
            destpost->length = texture->height;
            destpixels = (byte*)destpost + 3;
            destpixels[-1] = 0;	// pad 0
            for( i=0; i<texture->height; i++ )
            {
                destpixels[i] = 8; // mono color
            }
            destpixels[i++] = 0; // pad 0
            destpixels[i] = 0xFF; // term
            // all columns use the same post
            colofs = (uint32_t*)&(txcpatch->columnofs);  // has patch header
            for(i=0 ; i< texture->width ; i++ )
                 colofs[i] = head_size;
        }
        goto single_patch_finish;
    }
    // End of Single-patch texture

 multipatch_combine:
    // TM_combine_patch or TM_picture: Multiple patch textures.
    // Decide TM_ format
    // Combined patches + table + header
    compostsize += colofs_size + 8;	// combined patch size
    txcblocksize = colofs_size + (texture->width * texture->height); // picture format size

    if( texture_req == TM_masked )
        goto combine_format;     
    if( (texture_req == TM_picture_column) && (detect_hole | detect_post) )
        goto picture_format;
    // If cache was flushed then do not change model
    if( texture_req == TM_picture )
        goto picture_format;     
    if( texture_req == TM_combine_patch )
        goto combine_format;
    // new texture, decide on a model
    // if patches would not cover picture, then must have transparent regions
    if( compostsize < txcblocksize )
        goto combine_format;
    if( detect & TD_masked )  // hint
        goto combine_format;
    // If many patches and high overlap, then picture format is best.
    // This will need to be tuned if it makes mistakes.
    if( texture->patchcount >= 4 && compostsize > txcblocksize*4 )
        goto picture_format;
  
 combine_format:
    // TM_combine_patch: Combine multiple patches into one patch.

    // Size the new texture.  It may be too big, but must not be too small.
    // Will usually fit into compostsize because it does so as separate patches.
    // Overlap of normal posts will only reduce the size.
    // Worst case is (width * (height/2) * (1 pixel + 4 byte overhead))
    //   worstsize = colofs_size + 8 + (width * height * 5/2)
    // Usually the size will be much smaller, but it is not predictable.
    // all posts + new columnofs table + patch header
    // Compacted patches, like SW1COMM in Requiem MAP08, usually get expanded,
    // and they expand in the texture when they combine with other patches.

    // [WDJ] Generate_Texture will now realloc a texture when estimate was too small.
    // Combined patches + table + header + 1 byte per empty column
    txcblocksize = compostsize + texture->width;

    txcblock = Z_Malloc (txcblocksize, PU_IN_USE, (void**)&texren->cache);
    txcpatch = (patch_t*) txcblock;
    txcpatch->width = texture->width;
    txcpatch->height = texture->height;
    txcpatch->leftoffset = 0;
    txcpatch->topoffset = 0;
    // column offset lookup table
    colofs = (uint32_t*)&(txcpatch->columnofs);  // has patch header

    txcdata = (byte*)txcblock + colofs_size + 8;  // posting area
    // starts as empty post, with 0xFF a possibility
    destpixels = txcdata;
    texture_end = txcblock + txcblocksize - 2; // do not exceed
    detect_post = 0;  // combine has own detect
    detect_hole = 0;

    // Composite the columns together.
    for (x=0; x<texture->width; x++)
    {
        int nxtpat;	// patch number with next post
        int seglen, offset;
        // [WDJ] Detect column reuse (Requiem compacted patches).
        int livepatchcount = 0;
        int reuse_column = -1;
       
        // offset to post header
        colofs[x] = destpixels - (byte*)txcblock;  // this column
        destpost = (post_t*)destpixels;	// first post in column
        postlength = 0;  // length of current post
        bottom = 0;	 // next y in column
        segnxt_y = INT_MAX - 10;	// init to very large, but less than disabled
        segbot_y = INT_MAX - 10;
#ifdef DEEPSEA_TALL_PATCH
        seg_topdelta = -1;  // current dest topdelta
#endif
       
        // setup the columns, active or inactive
        for (p=0; p<patchcount; p++ )
        {
            compat_t * cp = &compat[p];
            int patch_x = x - cp->originx;	// within patch
            if( patch_x >= 0 && patch_x < cp->width )
            {
                realpatch = cp->patch;
                uint32_t* pat_colofs = (uint32_t*)&(realpatch->columnofs); // to match size in wad

                // [WDJ] Detect bad column offset.
                if( pat_colofs[patch_x] > cp->patchsize )  // detect bad patch
                    goto patch_off;  // post is not within patch memory

                cp->postptr = (post_t*)( (byte*)realpatch + pat_colofs[patch_x] );  // patch column
                if ( cp->postptr->topdelta == 0xFF )
                    goto patch_off;

#ifdef DEEPSEA_TALL_PATCH
                cp->cur_topdelta = -1;
#endif
	       
                // To handle Requiem compacted patches, where column data is reused.
                // Empty columns may be shared too, but they are very small,
                // and it really adds to logic complexity, so only look at non-empty.
                if ( pat_colofs[patch_x] > cp->usedpatchdata )
                    cp->usedpatchdata = pat_colofs[patch_x];  // track normal usage pattern
                else
                {
                    // compaction detected, look for reused column
                    int rpx = (cp->originx < 0) ? -cp->originx : 0;  // x in patch
#ifdef DEBUG_REUSECOL
                    if( DEBUG_REUSECOL )
                       debug_Printf("GenerateTexture: %8s detected reuse of column %i in patch %i\n", texture->name, patch_x, p );
#endif
                    for( ; rpx<patch_x; rpx++ )  // find reused column
                    {
                        if( pat_colofs[rpx] == pat_colofs[patch_x] )  // reused in patch
                        {
                            int rtx = rpx + cp->originx; // x in texture
#if 1
                            // Test for reused column is from single patch
                            int p2;
                            int livepatchcount2 = 0;
#ifdef DEBUG_REUSECOL
                            if( DEBUG_REUSECOL )
                               debug_Printf("GenerateTexture: testing reuse of %i, as %i\n", rpx, patch_x);
#endif
                            // [WDJ] If reused column is reused alone in both cases,
                            // then assume they will be identical in final texture too.
                            for (p2=0; p2<patchcount; p2++ )
                            {
                                compat_t * cp2 = &compat[p2];
                                int p2_x = rtx - cp2->originx; // within patch
                                if( p2_x >= 0 && p2_x < cp2->width )
                                {
                                   livepatchcount2 ++;
                                }
                            }
                            if ( livepatchcount2 == 1 )
                            {
                                reuse_column = rpx + cp->originx; // column in generated texture
                                break;
                            }
#else
                            // Compare with texture column, reuse when identical
                            post_t * rpp = (post_t*)( txcblock + colofs[rtx] );  // texture column
                            post_t * ppp = cp->postptr;
#ifdef DEBUG_REUSECOL
                            if( DEBUG_REUSECOL )
                               debug_Printf("GenerateTexture: testing reuse of %i, as %i\n", rpx, patch_x);
#endif
                            // [WDJ] Comparision is made difficult by the pad bytes,
                            // which we set to 0, and Requiem.wad does not.
                            while (rpp->topdelta != 0xff)  // find column end
                            {
                                int len = rpp->length;
                                if( rpp->topdelta != ppp->topdelta )  goto reject_1;
                                if( rpp->length != ppp->length )  goto reject_1;
                                // skip leading pad, and trailing pad
                                if( memcmp( ((byte*)rpp)+3, ((byte*)ppp)+3, len ) )  goto reject_1;
                                // next post
                                len += 4;  // sizeof post_t + lead pad + trail pad
                                rpp = (post_t *) (((byte *)rpp) + len);
                                ppp = (post_t *) (((byte *)ppp) + len);
                            }
                            if ( ppp->topdelta == 0xff )
                            {
                                // columns match
                                reuse_column = rtx;
                                break;
                            }
                           reject_1:
                            continue;
#endif
                        }
                    }
                }
                livepatchcount++;

#ifdef DEEPSEA_TALL_PATCH
                // When the column topdelta is <= the current topdelta,
                // it is a DeePsea tall patch relative topdelta.
                int topdelta = cp->postptr->topdelta;
                if( topdelta <= cp->cur_topdelta )
                {
                    // DeePsea relative topdelta
                    topdelta += cp->cur_topdelta;
                }
                cp->cur_topdelta = topdelta;
                cp->nxt_y = cp->originy + topdelta;
#else
                cp->nxt_y = cp->originy + cp->postptr->topdelta;
#endif
                cp->bot_y = cp->nxt_y + cp->postptr->length;
            }else{
               patch_off: // empty post
                // clip left and right by turning this patch off
                cp->postptr = NULL;
                cp->nxt_y = INT_MAX;
                cp->bot_y = INT_MAX;
            }
        }  // for all patches

        // [WDJ] Reuse shared column data instead of creating multiple copies.
        if( livepatchcount == 1 )
        {
            if( reuse_column >= 0 )
            {
                 // reuse the column
                 colofs[x] = colofs[reuse_column];
#ifdef DEBUG_REUSECOL
                 // DEBUG: enable to see column reuse
                 if( DEBUG_REUSECOL )
                   debug_Printf("GenerateTexture: reuse %i\n", reuse_column);
#endif
                 continue;  // next x
            }
        }

        // Assemble the patch column data from multiple patches to the composite.
        for(;;) // all posts in column
        {
            // Find next post y in this column.
            // Only need the last patch, as that overwrites every other patch.
            nxtpat = -1;	// patch with next post
            segnxt_y = INT_MAX-64;	// may be negative, must be < INT_MAX
            compat_t * cp = &compat[0];
            for (p=0; p<patchcount; p++, cp++)
            {
                // Skip over any patches that have been passed.
                // Includes those copied, or covered by those copied.
                while( cp->bot_y <= bottom )
                {
                    // this post has been passed, go to next post
                    cp->postptr = (post_t*)( ((byte*)cp->postptr) + cp->postptr->length + 4);
                    if( cp->postptr->topdelta == 0xFF )  // end of post column
                    {
                        // turn this patch off
                        cp->postptr = NULL;
                        cp->nxt_y = INT_MAX;	// beyond texture size
                        cp->bot_y = INT_MAX;
                        break;
                    }

#ifdef DEEPSEA_TALL_PATCH
                    // When the column topdelta is <= the current topdelta,
                    // it is a DeePsea tall patch relative topdelta.
		    int topdelta = cp->postptr->topdelta;
                    if( topdelta <= cp->cur_topdelta )
                    {
                        // DeePsea relative topdelta
                        topdelta += cp->cur_topdelta;
                    }
                    cp->cur_topdelta = topdelta;
                    cp->nxt_y = cp->originy + topdelta;
#else
                    cp->nxt_y = cp->originy + cp->postptr->topdelta;
#endif
                    cp->bot_y = cp->nxt_y + cp->postptr->length;
                }

                if( cp->nxt_y <= segnxt_y )
                {
                    // Found an active post
                    nxtpat = p;	// last draw into this segment
                    // Check for continuing in the middle of a post.
                    segnxt_y = (cp->nxt_y < bottom)?
                       bottom	// continue previous
                       : cp->nxt_y; // start at top of post
                    // Only a later drawn patch can overwrite this post.
                    segbot_y = cp->bot_y;
                }
                else
                {
                    // Limit bottom of this post segment by later drawn posts.
                    if( cp->nxt_y < segbot_y )   segbot_y = cp->nxt_y;
                }
            }
            // Exit test, end of column, which may be empty
            if( segnxt_y >= texture->height )
                break; // terminate_column

            // copy whole/remainder of post, or cut it short
            // assert: segbot_y <= cp->bot_y+1  because it is set in loop
            if( segbot_y > texture->height )   segbot_y = texture->height;

            seglen = segbot_y - segnxt_y;

            if( postlength != 0 )
            {
               // Check if next patch does not append to bottom of current patch
               // Combined with Detect.
               if( segnxt_y > bottom )
               {
                   detect_hole = TD_hole;  // whole texture
                   if( bottom > 0 )   postlength = 0;  // start new post
               }
               if( postlength + seglen > 255 )   // if postlength would be too long
               {
                   detect_post = TD_odd_post;  // whole texture
                   postlength = 0;  // start new post
               }
               if( postlength == 0 )
               {
                   // does not append, start new post after existing post
                   destpost = (post_t*)((byte*)destpost + destpost->length + 4);
               }
            }

            // Only one patch is drawn last in this segment, copy that one
            cp = &compat[nxtpat];
            srcpost = cp->postptr;
            // offset>0 when copy starts part way into this source post
            // NOTE: cp->nxt_y = cp->originy + srcpost->topdelta;
            offset = ( segnxt_y > cp->nxt_y )? (segnxt_y - cp->nxt_y) : 0;
            // consider y clipping problem
            if( cp->nxt_y < 0  &&  !cv_corr_clip_width.EV )
            {
                // Original doom had bug in y clipping, such that it
                // would clip the width but not skip the source pixels.
                // Given that segnxt_y was already correctly clipped.
                offset += cp->nxt_y; // reproduce original doom clipping
            }

            if( postlength == 0 )
            {
                // Start a new post
                if( destpixels + 3 >= texture_end )  goto exceed_alloc_error;

#ifdef DEEPSEA_TALL_PATCH
                if( segnxt_y > 254 )
                {
                    // Can also create a DeePsea patch.
                    // When the column topdelta is <= the current topdelta,
                    // it is a DeePsea tall patch relative topdelta.
                    int ds_topdelta = segnxt_y - seg_topdelta;
                    if( ds_topdelta > seg_topdelta )  goto exceed_topdelta;
                    // DeePsea relative topdelta successful.
                    destpost->topdelta = ds_topdelta;
		    detect_post = TD_odd_post;  // Due to DeePsea
                }
                else
                {
                    // new post header and 0 pad byte
                    destpost->topdelta = segnxt_y;
                }
                seg_topdelta = segnxt_y;
#else
                if( segnxt_y > 254 )   goto exceed_topdelta;
                // new post header and 0 pad byte
                destpost->topdelta = segnxt_y;
#endif
                // append at
                destpixels = (byte*)destpost + 3;
                destpixels[-1] = 0;	// pad 0
            }

            seglen = segbot_y - segnxt_y;
            if( destpixels + seglen + 3 >= texture_end )  goto exceed_alloc_error;

            // append to existing post
            memcpy( destpixels, ((byte*)srcpost + offset + 3), seglen );
            destpixels += seglen;
            postlength += seglen;
            bottom = segbot_y;
            // finish post bookkeeping so can terminate loop easily
            *destpixels = 0;	// pad 0
            if( postlength > 255 )   goto exceed_post_length;
            destpost->length = postlength;
        } // for all posts in column

        // terminate_column
        if( postlength )
            destpixels++;	// skip pad 0
        *destpixels++ = 0xFF;	// mark end of column

	if( bottom < texture->height )
            detect_hole = TD_hole;  // hole at bottom
	  
        // may be empty column so do not reference destpost
        continue; // next x
       
       // [WDJ] Realloc texture memory, when estimate was too small.
 exceed_alloc_error:
       {
        // Re-alloc the texture
        byte* old_txcblock = txcblock;
        int old_txcblocksize = txcblocksize;
        // inc alloc by a proportional amount, plus one column
        txcblocksize += (texture->width - x + 1) * txcblocksize / texture->width;
#if 1
        // enable to print re-alloc events
        if( verbose )
            GenPrintf(EMSG_ver, "R_GenerateTexture: %8s re-alloc from %i to %i\n",
                    texture->name, old_txcblocksize, txcblocksize );
#endif
        txcblock = Z_Malloc (txcblocksize, PU_IN_USE, (void**)&texren->cache);
        memcpy( txcblock, old_txcblock, old_txcblocksize );
        texture_end = txcblock + txcblocksize - 2; // do not exceed
        txcdata = (byte*)txcblock + colofs_size + 8;  // posting area
        destpixels = (byte*)txcblock + colofs[x] - 3;  // this column
        {
            patch_t * txcpatch = (patch_t*) txcblock; // header of txcblock
            colofs = (uint32_t*)&(txcpatch->columnofs);  // has patch header
        }
        x--;  // redo this column
        Z_Free(old_txcblock);  // also nulls texturecache ptr
        texren->cache = txcblock; // replace ptr undone by Z_Free
       }

    } // for x

    if( destpixels >= texture_end )
    {
        I_SoftError("R_GenerateTexture: %8s final exceeds allocation %i, used %i bytes\n",
                     texture->name, txcblocksize, destpixels - txcblock );
    }

    // Interface for texture draw.
    // Texture data is after the offset table, no patch header.
    texren->columnofs = colofs;
    texren->pixel_data_offset = 3;  // skip over post header and pad byte
    texren->texture_model = TM_combine_patch;  // transparent combined

    detect |= TD_2s_ready;
    if( (detect_hole | detect_post) == 0 )  detect |= TD_1s_ready;
#if 0
    // Enable to print memory usage
    I_SoftError("R_GenerateTexture: %8s allocated %i, used %i bytes\n",
            texture->name, txcblocksize, destpixels - txcblock );
#endif
    goto done;
   
 exceed_topdelta:
    I_SoftError("R_GenerateTexture: %8s topdelta= %i exceeds 254, make picture\n",
            texture->name, segnxt_y );
    goto error_redo_as_picture;
   
 exceed_post_length:
    I_SoftError("R_GenerateTexture: %8s post length= %i exceeds 255, make picture\n",
            texture->name, postlength );

 error_redo_as_picture:
    Z_Free( txcblock );
   
 picture_format:
    //
    // multi-patch textures (or 'composite')
    // These are stored in a picture format and use a different drawing routine,
    // which is flagged by TM_picture.
    // Format:
    //   array[ width ] of column offset
    //   array[ width ] of column ( array[ height ] pixels )

    txcblocksize = colofs_size + (texture->width * texture->height);
    //debug_Printf ("R_GenTex MULTI  %.8s size: %d\n",texture->name,txcblocksize);

    // will change PU_ at end of this function
    txcblock = Z_Malloc (txcblocksize, PU_IN_USE, (void**)&texren->cache);
    memset( txcblock + colofs_size, 0, txcblocksize - colofs_size ); // black background
   
    // columns lookup table
    colofs = (uint32_t*)txcblock;  // has no patch header
   
    // [WDJ] column offset must cover entire texture, not just under patches
    // and it does not vary with the patches.
    x1 = colofs_size;  // offset to first column, past colofs array
    for (x=0; x<texture->width; x++)
    {
       // generate column offset lookup
       // colofs[x] = (x * texture->height) + (texture->width*4);
       colofs[x] = x1;
       x1 += texture->height;
    }

    // Composite the columns together.
    // The TM_picture format has data in full height columns.
    texpatch = texture->patches;
    for (i=0; i<texture->patchcount; i++, texpatch++)
    {
        // [WDJ] patch only used in this loop, without any other Z_Malloc
        // [WDJ] Must use common patch read to preserve endian consistency.
        // otherwise it will be in cache without endian changes.
        // To avoid hardware render cache.
        realpatch = W_CachePatchNum_Endian(texpatch->lumpnum, PU_CACHE);  // patch temp
        x1 = texpatch->originx;
        if( x1 > texture->width )
	    continue;  // ignore PNG and other bad patches

        x2 = x1 + realpatch->width;
        if( x2 < 0 )
	    continue;  // ignore PNG and other bad patches

        if (x2 > texture->width)
            x2 = texture->width;

        for( x = (x1<0)? 0 : x1; x<x2 ; x++ )
        {
            // source patch column from wad, to be copied
            column_t* patchcol =
             (column_t *)( (byte *)realpatch + realpatch->columnofs[x-x1] );

            R_DrawColumnInCache (patchcol,		// source
                                 txcblock + colofs[x],	// dest
                                 texpatch->originy,
                                 texture->height);	// limit
        }
    }

    // Interface for texture draw.
    // Texture data is after the offset table, no patch header.
    texren->columnofs = colofs;
    texren->pixel_data_offset = 0;
    texren->texture_model = TM_picture;  // non-transparent picture format
    detect |= TD_1s_ready;
    if( detect_hole == 0 )  detect |= TD_2s_ready;  // no loss

done:
    detect |= detect_hole | detect_post;  // will indicate necessity of extra texren
    texren->detect = detect;  // used by wall/masked draw setup

    // unlock all the patches, no longer needed, but may be in another texture
    for( p=0; p<patchcount; p++ )
    {
        Z_ChangeTag (compat[p].patch, PU_CACHE);
    }
   
    // Now that the texture has been built in column cache,
    //  texture cache is purgable from zone memory.
    Z_ChangeTag (txcblock, PU_PRIV_CACHE);  // priority because of expense
    texturememory += txcblocksize;  // global

    return txcblock;
}


// Public
//  texture_req : requirement,  TM_none, TM_masked, TM_picture, or TM_picture_column
byte* R_GenerateTexture ( texture_render_t *  texren, byte  texture_req )
{
    return  R_GenerateTexture2( texren - texture_render, texren, texture_req );
}




//  convert flat to hicolor as they are requested
//
//byte**  flatcache;

byte* R_GetFlat (int  flatlumpnum)
{
   // [WDJ] Checking all callers shows that they might tolerate PU_CACHE,
   // but this is safer, and has less reloading of the flats
    return W_CacheLumpNum (flatlumpnum, PU_LUMP);	// flat image
//    return W_CacheLumpNum (flatlumpnum, PU_CACHE);

/*  // this code work but is useless
    byte*    data;
    short*   wput;
    int      i,j;

    //FIXME: work with run time pwads, flats may be added
    // lumpnum to flatnum in flatcache
    if ((data = flatcache[flatlumpnum-firstflat])!=0)
                return data;

    data = W_CacheLumpNum (flatlumpnum, PU_CACHE);
    i=W_LumpLength(flatlumpnum);

    Z_Malloc (i,PU_STATIC,&flatcache[flatlumpnum-firstflat]);
    memcpy (flatcache[flatlumpnum-firstflat], data, i);

    return flatcache[flatlumpnum-firstflat];
*/

/*  // this code don't work because it don't put a proper user in the z_block
    if ((data = flatcache[flatlumpnum-firstflat])!=0)
       return data;

    data = (byte *) W_CacheLumpNum(flatlumpnum,PU_LEVEL);
    flatcache[flatlumpnum-firstflat] = data;
    return data;

    flatlumpnum -= firstflat;

    if (vid.drawmode==DRAW8PAL)
    {
                flatcache[flatlumpnum] = data;
                return data;
    }

    // allocate and convert to high color

    wput = (short*) Z_Malloc (64*64*2,PU_STATIC,&flatcache[flatlumpnum]);
    //flatcache[flatlumpnum] =(byte*) wput;

    for (i=0; i<64; i++)
       for (j=0; j<64; j++)
                        wput[i*64+j] = ((color8to16[*data++]&0x7bde) + ((i<<9|j<<4)&0x7bde))>>1;

                //Z_ChangeTag (data, PU_CACHE);

                return (byte*) wput;
*/
}

//
// Empty the texture cache (used for load wad at runtime)
//
void R_Flush_Texture_Cache (void)
{
    int i;

    if (numtextures>0)
    {
        for (i=0; i<numtextures; i++)
        {
            // Z_Free also sets the cache ptr to NULL
            if( texture_render[i].cache )
                Z_Free( texture_render[i].cache );
        }
    }

    R_Release_all_extra_texren();
}

//
// R_Load_Textures
// Initializes the texture list with the textures from the world map.
//
// [WDJ] Original Doom bug: conflict between texture[0] and 0=no-texture.
// Their solution was to not use the first texture.
// In Doom1 == AASTINKY, FreeDoom == AASHITTY, Heretic = BADPATCH.
// Because any wad compatible with other doom engines, like prboom,
// will not be using texture[0], there is little incentive to fix this bug.
// It is likely that texture[0] will be a duplicate of some other texture.
#define BUGFIX_TEXTURE0
// 
//
void R_Load_Textures (void)
{
    maptexture_t*       mtexture;
    texture_t*          texture;
    mappatch_t*         mpatch;
    texpatch_t*         texpatch;
    char*               pnames;

    int                 i,j;

    uint32_t	      * maptex, * maptex1, * maptex2;  // 4 bytes, in wad
    uint32_t          * directory;	// in wad

    char                name[9];
    char*               name_p;

    lumpnum_t         * patch_to_num;  // patch name to num lookup

    int                 nummappatches;
    int                 offset;
    int                 maxoff;
    int                 maxoff2;
    int                 numtextures1;
    int                 numtextures2;



    // free previous memory before numtextures change

    if (numtextures>0)
    {
        for (i=0; i<numtextures; i++)
        {
            // Z_Free also sets the cache ptr to NULL
            if (textures[i])
                Z_Free (textures[i]);
            if( texture_render[i].cache )
                Z_Free( texture_render[i].cache );
        }
    }

    // Load the patch names from pnames.lmp.
    name[8] = 0;
    pnames = W_CacheLumpName ("PNAMES", PU_LUMP);  // temp
    // [WDJ] Do endian as use pnames temp
    nummappatches = LE_SWAP32 ( *((uint32_t *)pnames) );  // pnames lump [0..3]
    name_p = pnames+4;  // in lump, after number (uint32_t) is list of patch names

    // create name index to patch index lookup table, will free before return
    patch_to_num = calloc (nummappatches, sizeof(*patch_to_num));
    for (i=0 ; i<nummappatches ; i++)
    {
        strncpy (name,name_p+i*8, 8);
        patch_to_num[i] = W_Check_Namespace( name, LNS_patch );  // NO_LUMP when invalid
    }
    Z_Free (pnames);

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex = maptex1 = W_CacheLumpName ("TEXTURE1", PU_LUMP); // will be freed
    numtextures1 = LE_SWAP32(*maptex);  // number of textures, lump[0..3]
    maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
    directory = maptex+1;  // after number of textures, at lump[4]

    if( VALID_LUMP( W_CheckNumForName ("TEXTURE2") ) )
    {
        maptex2 = W_CacheLumpName ("TEXTURE2", PU_LUMP); // will be freed
        numtextures2 = LE_SWAP32(*maptex2); // number of textures, lump[0..3]
        maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
    }
    else
    {
        maptex2 = NULL;
        numtextures2 = 0;
        maxoff2 = 0;
    }
#ifdef BUGFIX_TEXTURE0
#define FIRST_TEXTURE  1
    // [WDJ] make room for using 0 as no-texture and keeping first texture
    numtextures = numtextures1 + numtextures2 + 1;
#else   
    numtextures = numtextures1 + numtextures2;
#define FIRST_TEXTURE  0
#endif


    // [smite] separate allocations, fewer horrible bugs
    if (textures)
    {
        Z_Free(textures);
        Z_Free(texture_render);
        Z_Free(textureheight);
    }

    textures         = Z_Malloc(numtextures * sizeof(*textures),         PU_STATIC, 0);
    texture_render   = Z_Malloc(numtextures * sizeof(texture_render_t),  PU_STATIC, 0);
    // NULL the cache ptrs, format, and flags.
    memset( texture_render, 0, sizeof(texture_render_t) * numtextures );
    textureheight    = Z_Malloc(numtextures * sizeof(*textureheight),    PU_STATIC, 0);

#ifdef BUGFIX_TEXTURE0
    for (i=0 ; i<numtextures-1 ; i++, directory++)
#else
    for (i=0 ; i<numtextures ; i++, directory++)
#endif
    {
        //only during game startup
        //if (!(i&63))
        //    CONS_Printf (".");

        if (i == numtextures1)
        {
            // Start looking in second texture file.
            maptex = maptex2;
            maxoff = maxoff2;
            directory = maptex+1;  // after number of textures, at lump[4]
        }

        // offset to the current texture in TEXTURESn lump
        offset = LE_SWAP32(*directory);  // next uint32_t

        if (offset > maxoff)
            I_Error ("R_Load_Textures: bad texture directory\n");

        // maptexture describes texture name, size, and
        // used patches in z order from bottom to top
        // Ptr to texture header in lump
        mtexture = (maptexture_t *) ( (byte *)maptex + offset);

        // Texture struct allocation is dependent upon number of patches.
        texture = textures[i] =
            Z_Malloc (sizeof(texture_t)
                      + sizeof(texpatch_t)*((uint16_t)(LE_SWAP16(mtexture->patchcount))-1),
                      PU_STATIC, 0);

        // get texture info from texture lump
        texture->width  = (uint16_t)( LE_SWAP16(mtexture->width) );
        texture->height = (uint16_t)( LE_SWAP16(mtexture->height) );
        texture->patchcount = (uint16_t)( LE_SWAP16(mtexture->patchcount) );
        texture->detect = (mtexture->masked)? TD_masked : 0; // hint

        // Sparc requires memmove, becuz gcc doesn't know mtexture is not aligned.
        // gcc will replace memcpy with two 4-byte read/writes, which will bus error.
        memmove(texture->name, mtexture->name, sizeof(texture->name));	
#if 0
        // [WDJ] DEBUG TRACE, watch where the textures go
        debug_Printf( "Texture[%i] = %8.8s\n", i, mtexture->name);
#endif
        mpatch = &mtexture->patches[0]; // first patch ind in texture lump
        texpatch = &texture->patches[0];

        for (j=0 ; j<texture->patchcount ; j++, mpatch++, texpatch++)
        {
            // get texture patch info from texture lump
            texpatch->originx = LE_SWAP16(mpatch->originx);
            texpatch->originy = LE_SWAP16(mpatch->originy);
            texpatch->lumpnum = patch_to_num[ (uint16_t)( LE_SWAP16(mpatch->patchnum) )];
            if( texpatch->lumpnum == NO_LUMP )
            {
                I_Error ("R_Load_Textures: Missing patch in texture %s\n",
                         texture->name);
            }
        }

        // determine width power of 2
#if 1
        // [WDJ] only need to determine if exact power of 2.
        j = 1;
        while (j < texture->width)
            j<<=1;
#else
        // Largest power of 2 that fits within width.
        j = 1;
        while (j*2 <= texture->width)
            j<<=1;
#endif
        if( j != texture->width )
        {
	    // Odd width
	    texture->detect |= TD_odd_width;
            j = 1;  // make width_tile_mask = 0
	}
        texture_render[i].width_tile_mask = j-1;
        texture_render[i].width = texture->width;
        textureheight[i] = texture->height<<FRACBITS;
    }

    Z_Free (maptex1);
    if (maptex2)
        Z_Free (maptex2);
    free(patch_to_num);

#ifdef BUGFIX_TEXTURE0
    // Move texture[0] to texture[numtextures-1]
    textures[numtextures-1] = textures[0];
    texture_render[numtextures-1] = texture_render[0];
    textureheight[numtextures-1] = textureheight[0];
    // cannot have ptr to texture in two places, will deallocate twice
    textures[0] = NULL;	// force segfault on any access to textures[0]
#endif   

    //added:01-04-98: this takes 90% of texture loading time..
    // Precalculate whatever possible.

    // Create translation table for global animation.
    if (texturetranslation)
        Z_Free (texturetranslation);

    // texturetranslation is 1 larger than texture tables, for some unknown reason
    texturetranslation = Z_Malloc ((numtextures+1)*sizeof(*texturetranslation),
                                   PU_STATIC, 0);

    for (i=0 ; i<numtextures ; i++)
        texturetranslation[i] = i;
}


lumpnum_t  R_CheckNumForNameList(const char *name, lumplist_t* list, int listsize)
{
  int   i;
  lumpnum_t  ln;
  // from last entry to first entry
  for(i = listsize - 1; i > -1; i--)
  {
    ln = W_CheckNumForNamePwad(name, list[i].wadfile, list[i].firstlump);
    if( ! VALID_LUMP(ln) )
      continue;  // not found
    if(LUMPNUM(ln) > (list[i].firstlump + list[i].numlumps) )
      continue;  // not within START END

    return ln;
  }
  return NO_LUMP;
}


// Extra colormaps
lumplist_t * colormaplumps = NULL;  // malloc
int          numcolormaplumps = 0;

// called by R_Load_Colormaps
static void R_Load_ExtraColormaps()
{
    lumpnum_t  start_ln, end_ln;
    int       cfile;
    int  ln1;

    numcolormaplumps = 0;
    colormaplumps = NULL;
    cfile = 0;

    for(; cfile < numwadfiles; cfile ++)
    {
        start_ln = W_CheckNumForNamePwad("C_START", cfile, 0);
        if( ! VALID_LUMP(start_ln) )
            continue;
       
        ln1 = LUMPNUM( start_ln );
        end_ln = W_CheckNumForNamePwad("C_END", cfile, ln1);

        if( ! VALID_LUMP(end_ln) )
        {
            I_SoftError("R_Load_Colormaps: C_START without C_END\n");
            continue;
        }

        if(WADFILENUM(start_ln) != WADFILENUM(end_ln))
        {
            I_SoftError("R_Load_Colormaps: C_START and C_END in different wad files!\n");
            continue;
        }

        colormaplumps = (lumplist_t *)realloc(colormaplumps, sizeof(lumplist_t) * (numcolormaplumps + 1));
        colormaplumps[numcolormaplumps].wadfile = WADFILENUM(start_ln);
        ln1++;
        colormaplumps[numcolormaplumps].firstlump = ln1;
        colormaplumps[numcolormaplumps].numlumps = LUMPNUM(end_ln) - ln1;
        numcolormaplumps++;
    }
}



lumplist_t*  flats;
int          numflatlists;


static
void R_Load_Flats ()
{
  lumpnum_t  start_ln, end_ln;
  int       cfile, ln1, ln2;

  numflatlists = 0;
  flats = NULL;
  cfile = 0;

  for(;cfile < numwadfiles; cfile ++)
  {
#ifdef DEBUG_FLAT
    debug_Printf( "Flats in file %i\n", cfile );
#endif
    start_ln = W_CheckNumForNamePwad("F_START", cfile, 0);
    if( ! VALID_LUMP(start_ln) )
    {
#ifdef DEBUG_FLAT
      debug_Printf( "F_START not found, file %i\n", cfile );
#endif
      start_ln = W_CheckNumForNamePwad("FF_START", cfile, 0);

      if( ! VALID_LUMP(start_ln) )  //If STILL NO_LUMP, search the whole file!
      {
#ifdef DEBUG_FLAT
        debug_Printf( "FF_START not found, file %i\n", cfile );
#endif
        end_ln = NO_LUMP;
        goto save_flat_list;
      }
    }

    // Search for END after START.
    ln1 = LUMPNUM( start_ln );
    end_ln = W_CheckNumForNamePwad("F_END", cfile, ln1);
    if( ! VALID_LUMP(end_ln) )
    {
#ifdef DEBUG_FLAT
      debug_Printf( "F_END not found, file %i\n", cfile );
#endif	 
      end_ln = W_CheckNumForNamePwad("FF_END", cfile, ln1);
#ifdef DEBUG_FLAT
      if( ! VALID_LUMP(end_ln) ) {
         debug_Printf( "FF_END not found, file %i\n", cfile );
      }
#endif
    }

save_flat_list:
    flats = (lumplist_t *)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
    flats[numflatlists].wadfile = cfile;
    if(end_ln == NO_LUMP)
    {
      flats[numflatlists].firstlump = 0;
      flats[numflatlists].numlumps = 0xffff; //Search the entire file!
    }
    else
    {
      // delimiting markers were found
      ln2 = LUMPNUM( end_ln );
      if( ln2 <= ln1 )  // should not be able to happen
        ln2 = 0xffff;  // search entire wad
      flats[numflatlists].firstlump = ln1 + 1;
      flats[numflatlists].numlumps = ln2 - (ln1 + 1);
    }
    numflatlists++;
    continue;
  }

  if(!numflatlists)
    I_Error("R_Load_Flats: No flats found!\n");
}



// [WDJ] was R_GetFlatNumForName, but it does not cache like GetFlat
lumpnum_t  R_FlatNumForName(const char *name)
{
  // [WDJ] No use in saving F_START if are not going to use them.
  // FreeDoom, where a flat and sprite both had same name,
  // would display sprite as a flat, badly.
  // Use F_START and F_END first, to find flats without getting a non-flat,
  // and only if not found then try whole file.
  
  lumpnum_t f_lumpnum = R_CheckNumForNameList(name, flats, numflatlists);

  if( ! VALID_LUMP(f_lumpnum) ) {
     // BP:([WDJ] R_CheckNumForNameList) don't work with gothic2.wad
     // [WDJ] Some wads are reported to use a flat as a patch, but that would
     // have to be handled in the patch display code.
     // If this search finds a sprite, sound, etc., it will display
     // multi-colored confetti.
     f_lumpnum = W_CheckNumForName(name);
  }
  
  if( ! VALID_LUMP(f_lumpnum) ) {
     // [WDJ] When not found, dont quit, use first flat by default.
     I_SoftError("R_FlatNumForName: Could not find flat %.8s\n", name);
     f_lumpnum = WADLUMP( flats[0].wadfile, flats[0].firstlump );  // default to first flat
  }

  return f_lumpnum;
}

// [WDJ] Manage the spritelump_t allocations
// Still use this array so that rot=0 sprite can share one entry for all 8 rotations.

void expand_spritelump_alloc( void )
{
    // [WDJ] Expand array and copy
    num_free_spritelump = 256;
    int request = num_spritelump + num_free_spritelump;
    // new array of spritelumps
    spritelump_t * sl_new = Z_Malloc (request*sizeof(spritelump_t), PU_STATIC, 0);
    if( spritelumps ) // existing
    {
        // move existing data
        memcpy( sl_new, spritelumps, sizeof(spritelump_t)*num_spritelump);
        Z_Free( spritelumps ); // old
    }
    spritelumps = sl_new;
}

// Next free spritelump
int  R_Get_spritelump( void )
{
    if( num_free_spritelump == 0 )
       expand_spritelump_alloc();
    num_free_spritelump--;
    return  num_spritelump ++;
}

//
// R_Init_SpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//

//
//   allocate sprite lookup tables
//
void R_Init_SpriteLumps (void)
{
    // the original Doom used to set numspritelumps from S_END-S_START+1

    // [WDJ] Initial allocation, will be expanded as needed
    expand_spritelump_alloc();
}


// ----------------------------------------
// COLORMAPS
// 
// COLORMAP lump : array[34]  of mapping tables [256].
//   [0]  : brightest, light level = 248..255, maps direct to palette colors
//   [31] : darkest, light level = 0..7,       maps to grey,black
//   [32] : invulnerability map,               maps to whites and grays
//   [33] : black map,			       maps entirely to black
//   
// Specific palette colors:
//  [0] = black 0,0,0
//  [4] = white 255,255,255
//  [176] = red 255,0,0
//  [112] = green 103,223,95
//  [200] = blue 0,0,255

// Boom: Extra colormaps can be included in the wad.  They are the same
// size and format as the Doom colormap.
// WATERMAP is predefined by Boom, but may be overloaded.


// called by R_Load_Data
static
void R_Load_Colormaps (void)
{
    lumpnum_t ln;

    // Load in the standard colormap lightmap tables (defined by lump),
    // now 64k aligned for smokie...
    ln = W_GetNumForName("COLORMAP");
    reg_colormaps = Z_MallocAlign ( W_LumpLength(ln), PU_STATIC, NULL, 16);
    W_ReadLump( ln, reg_colormaps );

    //SoM: 3/30/2000: Init Boom colormaps.
    {
      R_Clear_Colormaps();
      R_Load_ExtraColormaps();
    }
}


static lumpnum_t  extra_colormap_lumpnum[MAXCOLORMAPS];  // lump number

//SoM: Clears out extra colormaps between levels.
// called by P_SetupLevel after ZFree(PU_LEVEL,..)
// called by R_Load_Colormaps
void R_Clear_Colormaps()
{
  int   i;
#if 0   
  if( num_extra_colormaps > 30 )
     GenPrintf(EMSG_warn, "Number of colormaps: %i\n", num_extra_colormaps );
#endif

  // extra_colormaps are always present [MAXCOLORMAPS]
  num_extra_colormaps = 0;
  for(i = 0; i < MAXCOLORMAPS; i++)
  {
    extra_colormap_lumpnum[i] = NO_LUMP;
    // [WDJ] The ZMalloc colormap used to be PU_LEVEL, releasing memory without setting the ptrs NULL.
    // It is now PU_COLORMAP, and it is released only by this function.
    if( extra_colormaps[i].colormap )
    {
        Z_Free( extra_colormaps[i].colormap );
        extra_colormaps[i].colormap = NULL;
    }
  }
//  memset(extra_colormaps, 0, sizeof(extra_colormaps));
}


// [WDJ] Enable to print out results of colormap generate.
//#define VIEW_COLORMAP_GEN

// In order: whiteindex, lightgreyindex, medgreyindex, darkgreyindex, grayblackindex
// redindex, greenindex, blueindex
#define NUM_ANALYZE_COLORS  8
static byte  doom_analyze_index[NUM_ANALYZE_COLORS] = {
    4, 87, 96, 106, 7, 188, 123, 206
};
static byte  heretic_analyze_index[NUM_ANALYZE_COLORS] = {
    35, 26, 18, 10, 253, 148, 213, 192
};

// [WDJ] Analyze an extra colormap to derive some GL parameters
void  R_Colormap_Analyze( int mapnum )
{
    extracolormap_t * colormapp = & extra_colormaps[ mapnum ];
    colormapp->fadestart = 0;
    colormapp->fadeend = 33;
//    colormapp->maskalpha = 0x0;  // now in maskcolor
    colormapp->fog = 0;

#ifdef HWRENDER
  // Hardware renderer does not use, nor allocate, the colormap.
  // It uses the rgba field instead.
  if( rendermode == render_soft )
#endif     
  {
    // Software renderer defaults.
    // SoM: Added, we set all params of the colormap to normal because there
    // is no real way to tell how GL should handle a colormap lump anyway..
    colormapp->maskcolor.rgba = 0xffffffff; // white
    colormapp->maskcolor.s.alpha = 0;
    colormapp->fadecolor.rgba = 0x0; // black
    colormapp->rgba[0].rgba = 0xffffffff;
    colormapp->rgba[0].s.alpha = 0xe1;
  }
#ifdef HWRENDER
  else
  {
    // Analyze the Boom colormap for the hardware renderer.
    // The Boom colormap has been loaded already.
    // lighttable_t = byte array
    byte * cm = colormapp->colormap; // Boom colormap
    byte * tstcolor = & doom_analyze_index[0];  // colors to test
    RGBA_t work_rgba;
    int i;

    if( cm == NULL )  // no colormap to analyze
    {
        GenPrintf(EMSG_warn, "R_Colormap_Analyze: map %i, has no colormap\n", mapnum );
        return;
    }

    if( EN_heretic )
       tstcolor = & heretic_analyze_index[0];  // heretic colors to test

    // [WDJ] Analyze the colormap to get some imitative parameters.
    for(i=0; i<NUM_RGBA_LEVELS; i++)
    {
        // rgba[0]=darkest, rgba[NUM_RGBA_LEVELS-1] is the brightest
        // Within a range, analyze a map nearer the middle of the range.
        int mapindex = (((NUM_RGBA_LEVELS-1-i) * 32) + 8) / NUM_RGBA_LEVELS;
        // Estimate an alpha, could set it to max,
        // smaller alpha indicates washed out color.
        // This is completely ad-hoc, and rough.  It purposely favors high alpha.

// Analyze 4
        // the brightest red, green, blue should have the same brightness
        float h4r = 0.0; // alpha = 1
        int cnt =0;
        int try_cnt = 0;
        int dd4, dn4, k1, k2;
        // for all combinations of tstcolor
        for( k1=0; k1<NUM_ANALYZE_COLORS-1; k1++ )
        {
            byte i1 = tstcolor[k1];
            byte cm1 = cm[ LIGHTTABLE(mapindex) + i1 ];
            for( k2=k1+1; k2<NUM_ANALYZE_COLORS; k2++ )
            {
                byte i2 = tstcolor[k2];
                byte cm2 = cm[ LIGHTTABLE(mapindex) + i2 ];
                // for each color
                int krgb;
                for( krgb=0; krgb<3; krgb++ )  // red, green, blue
                {
                    //  (1-h) = (cm[b].r - cm[r].r) / (p[b].r -  p[r].r)
                    switch( krgb )
                    {
                     case 0: // red
                        dd4 = pLocalPalette[i1].s.red -  pLocalPalette[i2].s.red;
                        dn4 = pLocalPalette[cm1].s.red - pLocalPalette[cm2].s.red;
                        break;
                     case 1: // green
                        dd4 = pLocalPalette[i1].s.green -  pLocalPalette[i2].s.green;
                        dn4 = pLocalPalette[cm1].s.green - pLocalPalette[cm2].s.green;
                        break;
                     default: // blue
                        dd4 = pLocalPalette[i1].s.blue -  pLocalPalette[i2].s.blue;
                        dn4 = pLocalPalette[cm1].s.blue - pLocalPalette[cm2].s.blue;
                        break;
                    }
                    if( dn4 && (dd4 > 0.01f || dd4 < -0.01f))
                    {
                        float h3 = (float)dn4 / (float)dd4;
                        if( h3 > 1.0f )   h3 = 1.0;
                        if( h3 < 0.01f )  h3 = 0.01f;
                        h4r += h3;  // total for avg
                        cnt ++;
                    }
                    try_cnt ++;  // for fog
                }
            }
        }
        h4r /= cnt;
        int m4_fog = 0;
        // h = 0.0 is a useless table
        if( h4r > 0.99 ) {
            h4r = 0.99f;
            m4_fog = 1;
        }
        if( h4r < 0.00 ) {
            h4r = 0.00;
            m4_fog = 1;
        }
        if( cnt * 2 < try_cnt )
            m4_fog = 1;
        h4r = h4r * (0.66f / 1.732f);  // correction for testing at 1/3 max brightness
        float h4 = 1.0 - h4r;
        int m4_red, m4_blue, m4_green;
#if 0
        // Generate color tint from changes grey.
        byte greyindex = tstcolor[2];
        byte cm_grey = cm[ LIGHTTABLE(mapindex) + greyindex ];  // grey
        // cr = (cm[w].r - (1-h) * p[w].r) / h ;
        m4_red = ( pLocalPalette[cm_grey].s.red - (h4r* pLocalPalette[greyindex].s.red)) / h4;
        if( m4_red > 255 ) m4_red = 255;
        if( m4_red < 0 ) m4_red = 0;
        // cg = (cm[w].g - (1-h) * p[w].g) / h ;
        m4_green = ( pLocalPalette[cm_grey].s.green - (h4r* pLocalPalette[greyindex].s.green)) / h4;
        if( m4_green > 255 ) m4_green = 255;
        if( m4_green < 0 ) m4_green = 0;
        // cb = (cm[w].b - (1-h) * p[w].b) / h ;
        m4_blue = ( pLocalPalette[cm_grey].s.blue - (h4r* pLocalPalette[greyindex].s.blue)) / h4;
        if( m4_blue > 255 ) m4_blue = 255;
        if( m4_blue < 0 ) m4_blue = 0;
#else
        // Generate color tint from changes white.
        byte whiteindex = tstcolor[0];
        byte cm_white = cm[ LIGHTTABLE(mapindex) + whiteindex ];  // white
        // cr = (cm[w].r - (1-h) * p[w].r) / h ;
        m4_red = ( pLocalPalette[cm_white].s.red - (h4r* pLocalPalette[whiteindex].s.red)) / h4;
        if( m4_red > 255 ) m4_red = 255;
        if( m4_red < 0 ) m4_red = 0;
        // cg = (cm[w].g - (1-h) * p[w].g) / h ;
        m4_green = ( pLocalPalette[cm_white].s.green - (h4r* pLocalPalette[whiteindex].s.green)) / h4;
        if( m4_green > 255 ) m4_green = 255;
        if( m4_green < 0 ) m4_green = 0;
        // cb = (cm[w].b - (1-h) * p[w].b) / h ;
        m4_blue = ( pLocalPalette[cm_white].s.blue - (h4r* pLocalPalette[whiteindex].s.blue)) / h4;
        if( m4_blue > 255 ) m4_blue = 255;
        if( m4_blue < 0 ) m4_blue = 0;
#endif
#ifdef VIEW_COLORMAP_GEN
        // Enable to see results of analysis.
        GenPrintf(EMSG_info,
               "Analyze4: alpha=%i  red=%i  green=%i  blue=%i  fog=%i\n",
               (int)(255.0f*h4), m4_red, m4_green, m4_blue, m4_fog );
#endif      
        // Not great, get some tints wrong, and sometimes too light.
        // Give m4_fog some treatment, to stop compiler messages.
        work_rgba.s.alpha = (int)(h4 * 255.0f) - (m4_fog * 2.0f);
        work_rgba.s.red = m4_red;
        work_rgba.s.green = m4_green;
        work_rgba.s.blue = m4_blue;

        colormapp->rgba[i].rgba = work_rgba.rgba;  // to extra_colormap

#ifdef VIEW_COLORMAP_GEN
        // Enable to view settings of RGBA for hardware renderer.
        GenPrintf(EMSG_info,"RGBA[%i]: %8x   (alpha=%i, R=%i, G=%i, B=%i)\n",
              i, colormapp->rgba[i].rgba,
              (int)work_rgba.s.alpha,
              (int)work_rgba.s.red, (int)work_rgba.s.green, (int)work_rgba.s.blue );
#endif
    }
    // They had plans, but these are still unused in hardware renderer.
    colormapp->maskcolor.rgba = colormapp->rgba[NUM_RGBA_LEVELS-1].rgba;
    colormapp->fadecolor.rgba = colormapp->rgba[0].rgba;
  
  }
#endif
}




// [WDJ] The name parameter has trailing garbage, but the name lookup
// only uses the first 8 chars.
// Return the new colormap id number
int R_ColormapNumForName(const char *name)
{
  lumpnum_t lumpnum;
  int i;

  // Check for existing colormap of same name
  lumpnum = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
  if( ! VALID_LUMP(lumpnum) )
  {
    I_SoftError("R_ColormapNumForName: Cannot find colormap lump %8s\n", name);
    return 0;
  }

  for(i = 0; i < num_extra_colormaps; i++)
  {
    if(lumpnum == extra_colormap_lumpnum[i])
      return i;
  }

  // Add another colormap
  if(num_extra_colormaps == MAXCOLORMAPS)
  {
    I_SoftError("R_ColormapNumForName: Too many colormaps!\n");
    return 0;
  }

  extra_colormap_lumpnum[num_extra_colormaps] = lumpnum;
  extracolormap_t * ecmp = & extra_colormaps[num_extra_colormaps];

  // The extra_colormap structure is static and allocated ptrs in it must be set NULL upon release.
  // Align on 16 bits, like other colormap allocations.
  // ecmp->colormap =
      Z_MallocAlign (W_LumpLength(lumpnum), PU_COLORMAP, (void**)&(ecmp->colormap), 16);
  // read colormap tables [][] of byte, mapping to alternative palette colors
  W_ReadLump( lumpnum, ecmp->colormap );

#ifdef VIEW_COLORMAP_GEN
  GenPrintf(EMSG_info, "\nBoom Colormap: num=%i name= %8.8s\n", num_extra_colormaps, name );
#endif
  R_Colormap_Analyze( num_extra_colormaps );
  
  num_extra_colormaps++;
  return num_extra_colormaps - 1;
}



// SoM:
//
// R_Create_Colormap
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data. 

byte NearestColor(byte r, byte g, byte b);
int  RoundUp(double number);
void R_Create_Colormap_ex( extracolormap_t * extra_colormap_p );

#define HEX_TO_INT(x) (x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA_TO_INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : 0)
#define CHAR_TO_INT(c)  (c >= '0' && c <= '9' ? c - '0' : 0)
#define ABS2(x) ((x) < 0 ? -(x) : (x))

// [WDJ]
// The colorstr is the toptexture name:   '#' R(2) G(2) B(2) A(1)
// The ctrlstr is the midtexture name.    '#' FOG(1) FADE_BEGIN(2)  FADE_END(2)
// The fadestr is the bottomtexture name: '#' R(2) G(2) B(2).
// Translations:
//   R(2), G(2), B(2):   2-digit HEX => 256 color (8bit)
//   A(1): char a-z or A-Z  => 0 to 25 alpha
//   FOG: '0'= not-fog, '1'= fog-effect
//   FADE_BEGIN: 2-digit decimal (0..32) colormap where fade begins
//   FADE_END:   2-digit decimal (1..33) colormap where fade-to color is reached.
// Return the new colormap id number
int R_Create_Colormap_str(char *colorstr, char *ctrlstr, char *fadestr)
{
  extracolormap_t * extra_colormap_p;
  RGBA_t  maskcolor, fadecolor;
  unsigned int  fadestart = 0, fadeend = 33;
  int    c_alpha = 0;
  int    i;
  int    fog = 0;
  int    mapnum = num_extra_colormaps;  // the new colormap id number

  if(colorstr[0] == '#')  // colormap generate string is recognized
  {
    // color value from top texture string
    maskcolor.s.red = ((HEX_TO_INT(colorstr[1])<<4) + HEX_TO_INT(colorstr[2]));
    maskcolor.s.green = ((HEX_TO_INT(colorstr[3])<<4) + HEX_TO_INT(colorstr[4]));
    maskcolor.s.blue = ((HEX_TO_INT(colorstr[5])<<4) + HEX_TO_INT(colorstr[6]));
    if(colorstr[7] >= 'a' && colorstr[7] <= 'z')
      c_alpha = (colorstr[7] - 'a');
    else if(colorstr[7] >= 'A' && colorstr[7] <= 'Z')
      c_alpha = (colorstr[7] - 'A');
    else
      c_alpha = 25;
    maskcolor.s.alpha = (c_alpha * 255) / 25;  // convert from 0..25 to 0..255
  }
  else
  {
    // [WDJ] default for missing upper is not in docs, and was inconsistent
    // Cannot be 0xff after multiply by maskalpha=0.
    maskcolor.s.red = 0xff;
    maskcolor.s.green = 0xff;
    maskcolor.s.blue = 0xff;
    maskcolor.s.alpha = 0;
  }

  if(ctrlstr[0] == '#')
  {
    // SoM: Get parameters like, fadestart, fadeend, and the fogflag...
    fadestart = CHAR_TO_INT(ctrlstr[3]) + (CHAR_TO_INT(ctrlstr[2]) * 10);
    fadeend = CHAR_TO_INT(ctrlstr[5]) + (CHAR_TO_INT(ctrlstr[4]) * 10);
    if(fadestart > 32)
      fadestart = 0;
    if(fadeend > 33 || fadeend < 1)
      fadeend = 33;
    fog = CHAR_TO_INT(ctrlstr[1]) ? 1 : 0;
  }

  fadecolor.rgba = 0;
  if(fadestr[0] == '#')
  {
    fadecolor.s.red = ((HEX_TO_INT(fadestr[1]) * 16) + HEX_TO_INT(fadestr[2]));
    fadecolor.s.green = ((HEX_TO_INT(fadestr[3]) * 16) + HEX_TO_INT(fadestr[4]));
    fadecolor.s.blue = ((HEX_TO_INT(fadestr[5]) * 16) + HEX_TO_INT(fadestr[6]));
  }

  // find any identical existing colormap
  for(i = 0; i < num_extra_colormaps; i++)
  {
    // created colormaps only
    if( VALID_LUMP(extra_colormap_lumpnum[i]) )
      continue;
    if(   maskcolor.rgba == extra_colormaps[i].maskcolor.rgba
       && fadecolor.rgba == extra_colormaps[i].fadecolor.rgba
// [WDJ] maskalpha is now in maskcolor.alpha
//       && maskalpha == extra_colormaps[i].maskalpha
       && fadestart == extra_colormaps[i].fadestart
       && fadeend == extra_colormaps[i].fadeend
       && fog == extra_colormaps[i].fog )
      return i;
  }

#ifdef VIEW_COLORMAP_GEN
  GenPrintf(EMSG_info, "\nGenerate Colormap: num=%i\n", num_extra_colormaps );
  GenPrintf(EMSG_info, " alpha=%2x, color=(%2x,%2x,%2x), fade=(%2x,%2x,%2x), fog=%i\n",
          maskcolor.s.alpha, maskcolor.s.red, maskcolor.s.green, maskcolor.s.blue,
          fadecolor.s.red, fadecolor.s.green, fadecolor.s.blue,
          fog );
#endif

  if(num_extra_colormaps == MAXCOLORMAPS)
  {
    I_SoftError("R_Create_Colormap: Too many colormaps!\n");
    return 0;
  }

  num_extra_colormaps++;

  // Created colormaps do not have lumpnum
  extra_colormap_lumpnum[mapnum] = NO_LUMP;
  extra_colormap_p = &extra_colormaps[mapnum];

  extra_colormap_p->colormap = NULL;
  extra_colormap_p->maskcolor.rgba = maskcolor.rgba;
  extra_colormap_p->fadecolor.rgba = fadecolor.rgba;
//  extra_colormap_p->maskalpha = maskalpha;  // 0.0 .. 1.0  // now in maskcolor
  extra_colormap_p->fadestart = fadestart;
  extra_colormap_p->fadeend = fadeend;
  extra_colormap_p->fog = fog;

  R_Create_Colormap_ex( extra_colormap_p );

#if 0
#ifdef VIEW_COLORMAP_GEN
  if(rendermode != render_soft)
  {
    // [WDJ] DEBUG, TEST AGAINST OLD HDW CODE
    uint32_t rgba_oldhw =
          (HEX_TO_INT(colorstr[1]) << 4) + (HEX_TO_INT(colorstr[2]) << 0) +
          (HEX_TO_INT(colorstr[3]) << 12) + (HEX_TO_INT(colorstr[4]) << 8) +
          (HEX_TO_INT(colorstr[5]) << 20) + (HEX_TO_INT(colorstr[6]) << 16) +
          (ALPHA_TO_INT(colorstr[7]) << 24);
   if( rgba_oldhw != extra_colormap_p->rgba[0].rgba )
      GenPrintf(EMSG_info,"RGBA: old=%X, new=%x\n", rgba_oldhw, extra_colormap_p->rgba[i-1].rgba);
  }
#endif
#endif
   
  return mapnum;
}
   

// Analyze param and create the colormap data for the rendermode.
// Called when rendermode change.
void R_Create_Colormap_ex( extracolormap_t * extra_colormap_p )
{
  RGBA_t maskcolor, fadecolor;
  double color_r, color_g, color_b; // color RGB from top-texture
  double cfade_r, cfade_g, cfade_b; // fade-to color from bottom-texture
  double maskalpha;
  unsigned int  fadestart, fadeend, c_alpha;
  int  i;
  unsigned int p;
  
  maskcolor.rgba = extra_colormap_p->maskcolor.rgba;
  fadecolor.rgba = extra_colormap_p->fadecolor.rgba;
  fadestart = extra_colormap_p->fadestart;
  fadeend = extra_colormap_p->fadeend;

  c_alpha = maskcolor.s.alpha;
  maskalpha = (double)maskcolor.s.alpha / (double)255;  // 0.0 .. 1.0
  color_r = (double)maskcolor.s.red;
  color_g = (double)maskcolor.s.green;
  color_b = (double)maskcolor.s.blue;

  cfade_r = (double)fadecolor.s.red;
  cfade_g = (double)fadecolor.s.green;
  cfade_b = (double)fadecolor.s.blue;


#ifdef HWRENDER
  // Hardware renderer does not use, nor allocate, the colormap.
  // It uses the rgba field instead.
  if(rendermode == render_soft)
#endif
  {
    byte * colormap_p;
    double othermask = 1.0 - maskalpha;
    double by_alpha = maskalpha / 255.0;
    double fadedist = fadeend - fadestart;
    double  deltas[256][3], map[256][3];
     
    // The extra_colormap structure is static and allocated ptrs in it must be set NULL upon release.
    // Aligning on 16 bits, NOT 8, keeps it from crashing! SSNTails 12-13-2002
    if( extra_colormap_p->colormap == NULL )
    {
        // extra_colormap_p->colormap = 
        Z_MallocAlign((256 * 34) + 10, PU_COLORMAP, (void**)&(extra_colormap_p->colormap), 16);
    }
    colormap_p = extra_colormap_p->colormap;
     
    // premultiply, this messes up rgba calc so it must be done here
    color_r *= by_alpha;  // to 0.0 .. 1.0 range
    color_g *= by_alpha;
    color_b *= by_alpha;
    for(i = 0; i < 256; i++)
    {
      double r = pLocalPalette[i].s.red;
      double g = pLocalPalette[i].s.green;
      double b = pLocalPalette[i].s.blue;
      double cbrightness = sqrt((r*r) + (g*g) + (b*b));

      map[i][0] = (cbrightness * color_r) + (r * othermask);
      if(map[i][0] > 255.0)
        map[i][0] = 255.0;
      deltas[i][0] = (map[i][0] - cfade_r) / fadedist;

      map[i][1] = (cbrightness * color_g) + (g * othermask);
      if(map[i][1] > 255.0)
        map[i][1] = 255.0;
      deltas[i][1] = (map[i][1] - cfade_g) / fadedist;

      map[i][2] = (cbrightness * color_b) + (b * othermask);
      if(map[i][2] > 255.0)
        map[i][2] = 255.0;
      deltas[i][2] = (map[i][2] - cfade_b) / fadedist;
    }

    for(p = 0; p < 34; p++)
    {
      for(i = 0; i < 256; i++)
      {
        *colormap_p = NearestColor(RoundUp(map[i][0]), RoundUp(map[i][1]), RoundUp(map[i][2]));
        colormap_p++;
  
        if( p < fadestart )
          continue;
  
        if(ABS2(map[i][0] - cfade_r) > ABS2(deltas[i][0]))
          map[i][0] -= deltas[i][0];
        else
          map[i][0] = cfade_r;

        if(ABS2(map[i][1] - cfade_g) > ABS2(deltas[i][1]))
          map[i][1] -= deltas[i][1];
        else
          map[i][1] = cfade_g;

        if(ABS2(map[i][2] - cfade_b) > ABS2(deltas[i][1]))
          map[i][2] -= deltas[i][2];
        else
          map[i][2] = cfade_b;
      }
    }
  }
#ifdef HWRENDER
  else
  {
      if( extra_colormap_p->colormap )
      {
          // Does not use colormap
          Z_Free( extra_colormap_p->colormap );
          extra_colormap_p->colormap = NULL;
      }

      // hardware needs color_r, color_g, color_b, before they get *maskalpha.
      for( i=0; i<NUM_RGBA_LEVELS; i++ )
      {
          // rgba[0]=darkest, rgba[NUM_RGBA_LEVELS-1] is the brightest
          int mapindex = ((NUM_RGBA_LEVELS-1-i) * 34) / NUM_RGBA_LEVELS;
          double colorper, fadeper;
          if( mapindex <= fadestart )
          {
              colorper = 1.0; // rgba = color
              fadeper = 0.0;
          }
          else if( mapindex < fadeend )
          {
              // mapindex != fadeend, fadeend != fadestart
              // proportional ramp from color to fade color
              colorper = ((double)(fadeend-mapindex))/(fadeend-fadestart);
              fadeper = 1.0 - colorper;
          }
          else
          {
              // fadeend and after
              colorper = 0.0;
              fadeper = 1.0; // rgba = fade
          }

          RGBA_t * cp = & extra_colormap_p->rgba[i];
          cp->s.red = (int)( colorper*color_r + fadeper*cfade_r );
          cp->s.green = (int)( colorper*color_g + fadeper*cfade_g );
          cp->s.blue = (int)( colorper*color_b + fadeper*cfade_b );
          cp->s.alpha = c_alpha;
#ifdef VIEW_COLORMAP_GEN
          GenPrintf(EMSG_info,"RGBA[%i]: %x\n", i, extra_colormap_p->rgba[i].rgba);
#endif
      }
  }
#endif	       
}
#undef ALPHA_TO_INT
#undef HEX_TO_INT
#undef ABS2


//Thanks to quake2 source!
//utils3/qdata/images.c
byte NearestColor(byte r, byte g, byte b)
{
  int dr, dg, db;
  int distortion;
  int bestdistortion = 256 * 256 * 4;
  int bestcolor = 0;
  int i;

  for(i = 0; i < 256; i++) {
    dr = r - pLocalPalette[i].s.red;
    dg = g - pLocalPalette[i].s.green;
    db = b - pLocalPalette[i].s.blue;
    distortion = dr*dr + dg*dg + db*db;
    if(distortion < bestdistortion) {

      if(!distortion)
        return i;

      bestdistortion = distortion;
      bestcolor = i;
    }
  }

  return bestcolor;
}


// Rounds off floating numbers and checks for 0 - 255 bounds
int RoundUp(double number)
{
  if(number > 255.0)
    return 255;
  if(number < 0)
    return 0;

  if((int)number <= (int)(number -0.5))
    return (int)number + 1;

  return (int)number;
}




char * R_ColormapNameForNum(int num)
{
  if(num == -1)
    return "NONE";

  if(num < 0 || num > MAXCOLORMAPS)
  {
    I_SoftError("R_ColormapNameForNum: num is invalid!\n");
    return "NONE";
  }

  if( ! VALID_LUMP(extra_colormap_lumpnum[num]) )
    return "INLEVEL";  // created colormap

  return wadfiles[WADFILENUM(extra_colormap_lumpnum[num])]->lumpinfo[LUMPNUM(extra_colormap_lumpnum[num])].name;
}


#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 ) || defined( ENABLE_DRAW24 ) || defined( ENABLE_DRAW32 )
//
//  build a table for quick conversion from 8bpp to 15bpp, 16bpp, 24bpp, 32bpp
//
// covert 8 bit colors
void R_Init_color8_translate ( RGBA_t * palette )
{
    int  i;

    // [WDJ] unconditional now, palette flashes etc.
    {
        for (i=0;i<256;i++)
        {
            // use palette after gamma modified
            register byte r = palette[i].s.red;
            register byte g = palette[i].s.green;
            register byte b = palette[i].s.blue;
            switch( vid.drawmode )
            {
#ifdef ENABLE_DRAW15	       
             case DRAW15: // 15 bpp (5,5,5)
               color8.to16[i] = (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3)));
               break;
#endif
#ifdef ENABLE_DRAW16
             case DRAW16: // 16 bpp (5,6,5)
               color8.to16[i] = (((r >> 3) << 11) | ((g >> 2) << 5) | ((b >> 3)));
               break;
#endif
#ifdef ENABLE_DRAW24
             case DRAW24:
               {
                   // different alignment from DRAW32, for speed
                   pixelunion32_t c32;
                   c32.pix24.r = r;
                   c32.pix24.g = g;
                   c32.pix24.b = b;
                   color8.to32[i] = c32.ui32;
               }
               break;
#endif
#ifdef ENABLE_DRAW32
             case DRAW32:
               {
                   pixelunion32_t c32;
                   c32.pix32.r = r;
                   c32.pix32.g = g;
                   c32.pix32.b = b;
                   c32.pix32.alpha = 0xFF;
                   color8.to32[i] = c32.ui32;
               }
               break;
#endif
             default:
               break;  // should not be calling
            }
        }
    }

}
#endif

#ifdef ENABLE_DRAW8_USING_12
//#define COLOR12_QUALITY_CHECK
// covert 8 bit colors
void R_Init_color12_translate ( RGBA_t * palette )
{
    unsigned int i;

#ifdef COLOR12_QUALITY_CHECK
    printf(" color12 quality check:\n" );
#endif
   
    // Fill table
    for( i=0; i <= 0x0FFF; i++ )
    {
        // 12 bit color is  R,G,B  4 bits each.
        // Must shift 4 bit color up to be 8 bit color;
        register byte r = ((i & 0x0F00) >> 4) + 0x08;
        register byte g = ((i & 0x00F0)     ) + 0x08;
        register byte b = ((i & 0x000F) << 4) + 0x08;
        color12_to_8[i] = NearestColor( r, g, b );

#ifdef COLOR12_QUALITY_CHECK
        unsigned int  b1 = ((i << 4) & 0xF0);
        unsigned int  g1 = ((i ) & 0xF0);
        unsigned int  r1 = ((i >> 4) & 0xF0);
        register RGBA_t p = pLocalPalette[ color12_to_8[i] ];
        unsigned int  er = (abs( b1 - p.s.blue ) + abs( g1 - p.s.green ) + abs( r1 - p.s.red )) * 100 / 255;  // 0 to 300%
        if(   ( abs( b1 - p.s.blue  ) > 0x3F )
           || ( abs( g1 - p.s.green ) > 0x3F )
           || ( abs( r1 - p.s.red   ) > 0x3F )
           || ( er > 99) )
        {
            printf(" color12_translate[%4i]: (%2X,%2X,%2X) ==> (%2X,%2X,%2X)  ERROR= %4i\n", i,  r1,g1,b1, p.s.red,p.s.green,p.s.blue, er );
        }
#endif
    }
}
#endif


typedef struct
{
    byte  alpha;  // 0..255
    byte  opaque, clear;  // 0..100
} translucent_subst_info_t;

// From analyze of std translucent maps (with adjustments)
static  translucent_subst_info_t   translucent_subst[] =
{
    {131, 2, 0}, // TRANSLU_med
    {62, 0, 4}, // TRANSLU_more
    {60, 1, 22}, // TRANSLU_hi
    {140, 11, 0}, // TRANSLU_fire
    {200, 71, 3},  // TRANSLU_fx1
    {182, 0, 0}  // TRANSLU_75
};


//#define VIEW_TRANSLUMAP_GEN
//#define VIEW_TRANSLUMAP_GEN_DETAIL

// [WDJ] Analyze a Boom Translucent Map to derive some GL parameters
void  R_TranslucentMap_Analyze( translucent_map_t * transp, byte * tmap )
{
    // Analyze the Boom translucent map for rgb drawing.
    byte * tstcolor = & doom_analyze_index[0];  // colors to test
    int ti;

    if( EN_heretic )
       tstcolor = & heretic_analyze_index[0];  // heretic colors to test

    float h4 = 0.0; // alpha = 0
    int alpha;
    int tot_cnt = 0;
    int opaque_cnt = 0;
    int clear_cnt = 0;
    int dd4, dn4, k1, k2;
    // tm3 = i1 * (1-alpha) + i2 * alpha
    // alpha = (i1-tm3) / (i1-i2)
    for( k1=0; k1<NUM_ANALYZE_COLORS; k1++ )
    {
        byte i1 = tstcolor[k1];  // background
        for( k2=0; k2<NUM_ANALYZE_COLORS; k2++ )
        {
            if( k1 == k2 ) continue;
            byte i2 = tstcolor[k2];  // translucent
            byte tm3 = tmap[ (i2<<8) + i1 ];
            // for each color
            int krgb;
#ifdef  VIEW_TRANSLUMAP_GEN_DETAIL
GenPrintf(EMSG_info, "bg(%i,%i,%i):fb(%i,%i,%i):>(%i,%i,%i) ",
  pLocalPalette[i1].s.red,pLocalPalette[i1].s.green,pLocalPalette[i1].s.blue,
  pLocalPalette[i2].s.red,pLocalPalette[i2].s.green,pLocalPalette[i2].s.blue,
  pLocalPalette[tm3].s.red,pLocalPalette[tm3].s.green,pLocalPalette[tm3].s.blue
);
#endif	       
            for( krgb=0; krgb<3; krgb++ )  // red, green, blue
            {
                // alpha = (i1-tm3) / (i1-i2)
                switch( krgb )
                {
                 case 0: // red
                   dd4 = (int)pLocalPalette[i1].s.red -  (int)pLocalPalette[i2].s.red;
                   dn4 = (int)pLocalPalette[i1].s.red - (int)pLocalPalette[tm3].s.red;
                   break;
                 case 1: // green
                   dd4 = (int)pLocalPalette[i1].s.green -  (int)pLocalPalette[i2].s.green;
                   dn4 = (int)pLocalPalette[i1].s.green - (int)pLocalPalette[tm3].s.green;
                   break;
                 default: // blue
                   dd4 = (int)pLocalPalette[i1].s.blue -  (int)pLocalPalette[i2].s.blue;
                   dn4 = (int)pLocalPalette[i1].s.blue - (int)pLocalPalette[tm3].s.blue;
                   break;
                }
                // eliminate cases where equation is least accurate
                if( dn4 && (dd4 > 10.0 || dd4 < -10.0))
                {
                    float h3 = (float)dn4 / (float)dd4;
#ifdef VIEW_TRANSLUMAP_GEN_DETAIL
                    GenPrintf(EMSG_info, " %i", (int)(h3*255) );
#endif
                    // eliminate wierd alpha (due to color quantization in making the transmap)
                    if( h3 > 0.0 && h3 < 1.1 )
                    {
                        h4 += h3;  // total for avg
                        tot_cnt ++;
                        if( h3 > 0.9 )   opaque_cnt++;
                        if( h3 < 0.1 )   clear_cnt++;
                    }
                }
#ifdef VIEW_TRANSLUMAP_GEN_DETAIL
                else
                {
                    GenPrintf(EMSG_info, " -" );
                }
#endif	       
            }
#ifdef VIEW_TRANSLUMAP_GEN_DETAIL
            GenPrintf(EMSG_info,"\n");
#endif	       
        }
    }
    alpha = (int) (255.0 * h4 / tot_cnt);
    if( alpha > 255 )  alpha = 255;
    if( alpha < 0 )    alpha = 0;
    transp->alpha = alpha;
    transp->opaque = (opaque_cnt*100)/tot_cnt;
    transp->clear = (clear_cnt*100/tot_cnt);
    transp->substitute_std_translucent = TRANSLU_med;  // default
    transp->substitute_error = 255;
    {
        // find closest standard transparency
        float bestdist = 1E20;
        for(ti=1; ti<TRANSLU_75; ti++)
        {
            translucent_subst_info_t * tinfop = & translucent_subst[ti-1];
            float dista = tinfop->alpha - alpha;  // 0..255
            float distop = tinfop->opaque - transp->opaque;  // 0..100
            float distcl = tinfop->clear - transp->clear;  // 0..100
            float dist = (dista*dista) + (distop*distop) + (distcl*distcl);
            if( dist < bestdist )
            {
                bestdist = dist;
                dist *= (255.0/(40.0*40.0 + 20.0*20.0  + 10.0*10.0));
                if( dist > 255 )  dist = 255;
                transp->substitute_error = dist;
                transp->substitute_std_translucent = ti;
            }
        }
    }
#ifdef VIEW_TRANSLUMAP_GEN
    // Enable to see results of analysis.
    GenPrintf(EMSG_info,
             "Analyze Trans: alpha=%i  opaque=%i%%  clear=%i%%  subst=%i  subst_err=%i\n",
              alpha, transp->opaque, transp->clear,
              transp->substitute_std_translucent, transp->substitute_error);
#endif      
}

#define TRANSLU_STORE_INC  8
translucent_map_t *  translu_store = NULL;
int translu_store_num = 0;
int translu_store_len = 0;

// Return translu_store index
int  R_setup_translu_store( int lump_num )
{
   int ti;
   translucent_map_t * tranlu_map;
   // check if already in store
   for(ti=0; ti< translu_store_num; ti++ )
   {
       if( translu_store[ti].translu_lump_num == lump_num )
          goto done;
   }
   // check for expand store
   if( translu_store_num >= translu_store_len )
   {
       translu_store_len += TRANSLU_STORE_INC;
       translu_store = realloc( translu_store, translu_store_len );
       if( translu_store == NULL )
          I_Error( "Translucent Store: cannot alloc\n" );
   }
   // create new store and fill it in
   ti = translu_store_num++;
   tranlu_map = & translu_store[ti];
   tranlu_map->translu_lump_num = lump_num;
   R_TranslucentMap_Analyze( tranlu_map, W_CacheLumpNum( lump_num, PU_CACHE ) );
   if( verbose>1 )
      GenPrintf(EMSG_ver, "TRANSLU STORE lump=%i, subst=%i\n", lump_num, tranlu_map->substitute_std_translucent );
done:
   return ti;
}

#if 0
// [WDJ] Only enable to setup colormap analyze tables
#define ANALYZE_GAMEMAP
void Analyze_gamemap( void )
{
#define NUMTESTCOLOR  8   
    // For new port, analyze the colormap to find white, grey, red, blue, green.
    // This only affects this color analyzer, and is not fatal.
    static const char * name[NUMTESTCOLOR]=
     {"white", "light grey", "med grey", "dark grey", "grayblack", "red", "green", "blue" };
    typedef struct { byte red, green, blue; } test_rgb_t;
    static const test_rgb_t ideal[NUMTESTCOLOR]=
     {{255,255,255}, {192,192,192}, {128,128,128}, {64,64,64}, {10,10,10}, {100,0,0}, {0,100,0}, {0,0,100} };
    float bestdist[NUMTESTCOLOR];
    byte bestcolor[NUMTESTCOLOR];
    int i, k;

    GenPrintf(EMSG_info, "Game colormap analyze:\n" );
    for(k=0; k<8; k++ )   bestdist[k] = 1E20;
    
    for(i=0; i<256; i++)
    {
        int cr = pLocalPalette[i].s.red;
        int cg = pLocalPalette[i].s.green;
        int cb = pLocalPalette[i].s.blue;
        for(k=0; k<NUMTESTCOLOR; k++ )
        {
            float dr = cr - ideal[k].red;
            float dg = cg - ideal[k].green;
            float db = cb - ideal[k].blue;
            float d = dr*dr + dg*dg + db*db;
            if( d < bestdist[k] )
            {
                bestdist[k] = d;
                bestcolor[k] = i;
            }
        }
    }
    for(k=0; k<NUMTESTCOLOR; k++ )
    {
        GenPrintf(EMSG_info, "Analyze Index %s = %i\n", name[k], bestcolor[k] );
    }
}
#endif

#if 0
// [WDJ] Only enable to setup translucent analyze tables
#define ANALYZE_TRANSLUCENT_MAPS
void Analyze_translucent_maps( void )
{
    // translucent maps
    translucent_map_t trans;
    int k;
    GenPrintf(EMSG_info, "Game translucent maps analyze:\n" );
    if( translucenttables == NULL )   return;
    for(k=1; k<=TRANSLU_fx1; k++ )  // Doom2 maps
    { 
        byte * tmap = & translucenttables[ TRANSLU_TABLE_INDEX(k) ];
        if( tmap )
        {
            GenPrintf(EMSG_info, "  Analyze translucent map %i:\n", k );
            R_TranslucentMap_Analyze( &trans, tmap);
        }
    }
}
#endif


// Table of alpha = 0..255 to translucent tables to be used for r_draw8
// For use by DRAW8PAL, Not for draw modes that can use dr_alpha.
// index by alpha >> 4
const unsigned int  translucent_alpha_table[16] =
{
   // hi=10..15%, more=20..25%, med=50%
   TRANSLU_TABLE_hi,   // 0..15    ( 0.. 6%)
   TRANSLU_TABLE_hi,   // 16..31   ( 6..12%)
   TRANSLU_TABLE_hi,   // 32..47   (12..18%)
   TRANSLU_TABLE_more, // 48..63   (18..25%)
   TRANSLU_TABLE_more, // 64..79   (25..31%)
   TRANSLU_TABLE_more, // 80..95   (31..37%)
   TRANSLU_TABLE_med,  // 96..111  (37..43%)
   TRANSLU_TABLE_med,  // 112..127 (44..50%)
   TRANSLU_TABLE_med,  // 128..143 (50..56%)
 // reversed table usage, per TRANSLU_REV_ALPHA
   TRANSLU_TABLE_med,  // 144..159 (56..62%)
   TRANSLU_TABLE_more, // 160..177 (63..69%)
   TRANSLU_TABLE_more, // 176..191 (69..75%)
   TRANSLU_TABLE_more, // 192..207 (75..81%)
   TRANSLU_TABLE_hi,   // 208..223 (81..87%)
   TRANSLU_TABLE_hi,   // 224..239 (88..93%)
   TRANSLU_TABLE_hi,   // 240..255 (94..100%)
};



// Fake floor fog and water effect structure
#define FWE_STORE_INC  16
fogwater_t *  fweff = NULL;  // array
// 0 of the array is not used
static int fweff_num = 0;
static int fweff_len = 0;  // num allocated

int  R_get_fweff( void )
{
   // check for expand store
   if( fweff_num >= fweff_len )
   {
       fweff_len += FWE_STORE_INC;
       fweff = (fogwater_t*) realloc( fweff, fweff_len * sizeof(fogwater_t) );
       if( fweff == NULL )
          I_Error( "Fog Store: cannot alloc\n" );
   }
   memset( &fweff[fweff_num], 0, sizeof( fogwater_t ) );
   return fweff_num++;
}


// update config settings
void R_FW_config_update( void )
{
   int i;
   // If not init yet, then init will call this again
   if( ! fweff )
       return;

   // defaults affected by controls
   fweff[1].effect = (fogwater_effect_e) cv_water_effect.value;
   fweff[2].effect = (fogwater_effect_e) cv_fog_effect.value;
   fweff[3].effect = fweff[1].effect;
   // update all fweff settings that were defaulted
   for( i=5; i<fweff_num; i++ )
   {
       byte fwf = fweff[i].flags;
       fogwater_t * def = & fweff[ fwf & FWF_index ];
       if( fwf & FWF_default_effect )
       {
           fweff[i].effect = def->effect;
       }
       if( fwf & FWF_default_alpha )
       {
           fweff[i].alpha = def->alpha;
           fweff[i].fsh_alpha = def->fsh_alpha;
       }
   }
   // Must also change the affected FakeFloor flags
   P_Config_FW_Specials ();
}

// start of level
void R_Clear_FW_effect( void )
{
    if( ! fweff )  // no memory yet
    { 
        // init
        R_get_fweff();  // [0] unused
        R_get_fweff();  // [1] water
        R_get_fweff();  // [2] fog
        R_get_fweff();  // [3] opaque water
        R_get_fweff();  // [4] solid floor

        fweff[0].effect = FW_clear; // unused
        fweff[0].alpha = 0;
        fweff[0].fsh_alpha = 0;
        fweff[0].flags = 0;

        fweff[1].effect = FW_clear;
        fweff[1].alpha = 128;  // default water alpha
        fweff[1].fsh_alpha = 128;
        fweff[1].flags = FWF_water | FWF_default_alpha | FWF_default_effect;

        fweff[2].effect = FW_fogdust;
        fweff[2].alpha = 110;  // default fog alpha
        fweff[2].fsh_alpha = 110 * 0.90;
        fweff[2].flags = FWF_fog | FWF_default_alpha | FWF_default_effect;

        fweff[3].effect = FW_clear;
        fweff[3].alpha = 190;  // default opaque water
        fweff[3].fsh_alpha = 190;
        fweff[3].flags = FWF_opaque_water | FWF_default_alpha | FWF_default_effect;

        fweff[4].effect = FW_clear;  // does not matter
        fweff[4].alpha = 128;  // default solid translucent floor
        fweff[4].fsh_alpha = 128;
        fweff[4].flags = FWF_solid_floor;  // not settable defaults

        R_FW_config_update();  // config settings
    }
    fweff_num = 5;  // only save the defaults
}


// Create Fog Effect from texture name string
// Syntax: #aaaN
// where aaa is alpha, in decimal, 0..255
// where N is fog_effect, from this table
fogwater_effect_e  fwe_code_table[FW_num] =
{
   FW_clear,     // 'A' no fog  (old WATER default)
   FW_cast,      // 'B' paint all surfaces with textures
   FW_colormap,  // 'C' use colormap fog (which only colors all sectors)
   FW_inside,    // 'D' render inside side, plane views (old FOG)
   FW_foglite,   // 'E' outside side, plane views, low alpha overall fog sheet
   FW_fogdust,   // 'F' outside, when in fog apply overall fog sheet (FOG default)
   FW_fogsheet,  // 'G' outside, overall fog sheet, sector join fog sheet
   FW_fogfluid,  // 'H' outside, inside fluid, fogsheet
};


// return index into fweff
int R_Create_FW_effect( int special_linedef, char * tstr )
{
    int def_index =  // of fweff[]
       ( special_linedef == 304 )? FWF_opaque_water  // opaque water
     : ( special_linedef == 302 )? FWF_fog           // fog
     : ( special_linedef == 301 )? FWF_water         // water
     : ( special_linedef == 300 )? FWF_solid_floor   // solid floor
     : 0;
    int fwe_index;
    byte wflags = 0;
    fogwater_t * fwp;

    if( tstr[0] != '#' )
    {
        return def_index;  // can use a default
    }
    // make a unique entry for this situation
    fwe_index = R_get_fweff();
    fwp = &fweff[fwe_index];
    fwp->effect = fweff[def_index].effect;  // get default settings
    fwp->alpha = fweff[def_index].alpha;
    wflags = fweff[def_index].flags;

    if( tstr[0] == '#')  // decode upper texture string
    {
        // alpha
        fwp->alpha = CHAR_TO_INT(tstr[1])*100 + CHAR_TO_INT(tstr[2])*10 + CHAR_TO_INT(tstr[3]);
        wflags &= ~ FWF_default_alpha;
        // fog type
        char chf = tstr[4];
        if( chf >= 'A' && chf <= ('A'+FW_num) )
        {
            fwp->effect = fwe_code_table[ chf - 'A' ];
            wflags &= ~ FWF_default_effect;
        }
    }
    if( special_linedef == 300 )  // FWF_solid_floor
    {
        fwp->effect = FW_clear;  // no internal fog effect
    }
    switch( fwp->effect )
    {
     case FW_foglite:
        fwp->fsh_alpha = fwp->alpha * 0.60;
        break;
     case FW_fogdust:
        fwp->fsh_alpha = fwp->alpha * 0.90;
        break;
     case FW_fogsheet: 
        fwp->fsh_alpha = fwp->alpha * 0.80;
        break;
     case FW_fogfluid:
     default:
        fwp->fsh_alpha = fwp->alpha;
        break;
    }
    fwp->flags = wflags;
    return fwe_index;
}


// R_Load_Data
// Locates all the lumps that will be used by all views
// Must be called after W_Init.
//
void R_Load_Data (void)
{
    CONS_Printf ("\nLoad Textures...");
    R_Load_Textures ();
    CONS_Printf ("\nLoad Flats...");
    R_Load_Flats ();

    CONS_Printf ("\nInitSprites...\n");
    R_Init_SpriteLumps ();
    R_Init_Sprites (sprnames);

    CONS_Printf ("\nLoad Colormaps...\n");
    R_Load_Colormaps ();
#ifdef ANALYZE_GAMEMAP
    Analyze_gamemap();
#endif
}


//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
// [WDJ] Original Doom bug: conflict between texture[0] and 0=no-texture.
// 
// Parameter name is 8 char without term.
// Return -1 for not found, 0 for no texture
int  R_CheckTextureNumForName (const char *name)
{
    int  i;

    // "NoTexture" marker.
    if (name[0] == '-')
        return 0;

    for (i=FIRST_TEXTURE ; i<numtextures ; i++)
        if (!strncasecmp (textures[i]->name, name, 8) )
            return i;
#ifdef RANGECHECK
    if( i == 0 )
        GenPrintf(EMSG_warn, "Texture %8.8s  is texture[0], imitates no-texture.\n", name);
#endif   
   
    return -1;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//
// Return  0 for no texture "-".
// Return default texture when texture not found (would HOM otherwise).
// Parameter name is 8 char without term.
// Is used for side_t texture fields, which are used for array access
// without further error checks, so never returns -1.
int R_TextureNumForName (const char* name)
{
    int i;

    i = R_CheckTextureNumForName (name);
#if 0
// [WDJ] DEBUG TRACE, to see where textures have ended up and which are accessed.
#  define trace_SIZE 512
   static char debugtrace_RTNFN[ trace_SIZE ];
   if( i<trace_SIZE && debugtrace_RTNFN[i] != 0x55 ) {
      debug_Printf( "Texture %8.8s is texture[%i]\n", name, i);
      debugtrace_RTNFN[i] = 0x55;
   }
#  undef trace_SIZE   
#endif   

    if(i==-1)
    {
        // Exclude # parameters from "not found" message.
        // Prevent warning due to using signed char as index
        if( isalpha((unsigned char) name[0]) )
            I_SoftError("R_TextureNumForName: %.8s not found\n", name);

        i=1;	// default to texture[1]
    }
    return i;
}




//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//

void R_PrecacheLevel (void)
{
    char*  texturepresent;
    char*  spritepresent;

    int i,j,k,n;
    lumpnum_t  lumpnum;

    thinker_t*          th;
    spriteframe_t *     sf;
    sprite_frot_t *     sv;

    //int numgenerated;  //faB:debug

    if (demoplayback)
        return;

    // do not flush the memory, Z_Malloc twice with same user
    // will cause error in Z_CheckHeap(), 19991022 by Kin
    if (rendermode != render_soft)
        return;

    // Precache flats.
    flatmemory = P_PrecacheLevelFlats();

    //
    // Precache textures.
    //
    // no need to precache all software textures in 3D mode
    // (note they are still used with the reference software view)
    texturepresent = calloc(numtextures,sizeof(char));  // temp alloc, zeroed

    for (i=0 ; i<numsides ; i++)
    {
        // for all sides
        // texture num 0=no-texture, otherwise is valid texture
#if 1
        if (sides[i].toptexture)
            texturepresent[sides[i].toptexture] = 1;
        if (sides[i].midtexture)
            texturepresent[sides[i].midtexture] = 1;
        if (sides[i].bottomtexture)
            texturepresent[sides[i].bottomtexture] = 1;
#else 
        //Hurdler: huh, a potential bug here????
        if (sides[i].toptexture < numtextures)
            texturepresent[sides[i].toptexture] = 1;
        if (sides[i].midtexture < numtextures)
            texturepresent[sides[i].midtexture] = 1;
        if (sides[i].bottomtexture < numtextures)
            texturepresent[sides[i].bottomtexture] = 1;
#endif       
    }

#if 0   
    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependent name.
    texturepresent[sky_texture] = 1;
#endif

    //if (devparm)
    //    GenPrintf(EMSG_dev, "Generating textures..\n");

    texturememory = 0;  // global
    for (i=FIRST_TEXTURE ; i<numtextures ; i++)
    {
        if (!texturepresent[i])
            continue;

        //texture = textures[i];
        if( texture_render[i].cache == NULL )
             R_GenerateTexture2 ( i, &texture_render[i], TM_none );
        //numgenerated++;

        // note: pre-caching individual patches that compose textures became
        //       obsolete since we now cache entire composite textures
    }
    //debug_Printf("total mem for %d textures: %d k\n",numgenerated,texturememory>>10);
    free(texturepresent);

    //
    // Precache sprites.
    //
    spritepresent = calloc(numsprites,sizeof(char));  // temp alloc, zeroed

    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        if (th->function == TFI_MobjThinker)
            spritepresent[((mobj_t *)th)->sprite] = 1;
    }

    spritememory = 0;
    for (i=0 ; i<numsprites ; i++)
    {
        if (!spritepresent[i])
            continue;

        for (j=0 ; j<sprites[i].numframes ; j++)
        {
            sf = get_spriteframe( &sprites[i], j );
            n = srp_to_num_rot[ sf->rotation_pattern ];
            for (k=0 ; k<n ; k++)
            {
                sv = get_framerotation( &sprites[i], j, k );
                //Fab: see R_Init_Sprites for more about pat_lumpnum,lumpid
                lumpnum = sv->pat_lumpnum;
                if(devparm)
                   spritememory += W_LumpLength(lumpnum);
                W_CachePatchNum( lumpnum, PU_CACHE );
            }
        }
    }
    free(spritepresent);

    //FIXME: this is no more correct with glide render mode
    if (devparm)
    {
        GenPrintf(EMSG_dev,
                    "Precache level done:\n"
                    "flatmemory:    %ld k\n"
                    "texturememory: %ld k\n"
                    "spritememory:  %ld k\n", flatmemory>>10, texturememory>>10, spritememory>>10 );
    }
#ifdef ANALYZE_TRANSLUCENT_MAPS
    // maps are loaded by now
    Analyze_translucent_maps();
#endif
}


void R_Init_rdata(void)
{
    int i;

    // clear for easier debugging
    memset(extra_colormaps, 0, sizeof(extra_colormaps));

    for(i = 0; i < MAXCOLORMAPS; i++)
    {
        extra_colormap_lumpnum[i] = NO_LUMP;
        extra_colormaps[i].colormap = NULL;
    }
}

// Upon change in rendermode.
void R_rdata_setup_rendermode( void )
{
    int i;
   
    // Colormaps are depedent upon rendermode.
    for( i=0; i<num_extra_colormaps; i++ )
    {
        if( VALID_LUMP( extra_colormap_lumpnum[i] ) )
        {
            // Analyze the lump
            R_Colormap_Analyze( i );
        }
        else
        {
            // Create from parameters
            R_Create_Colormap_ex( & extra_colormaps[i] );
        }
    }
}
