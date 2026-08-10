// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hw_cache.c 1611 2022-01-01 09:40:58Z wesleyjohnson $
//
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: hw_cache.c,v $
// Revision 1.39  2003/06/11 04:33:46  ssntails
// 256x256 size limit removal for textures. Possible 'compatibility' cvar commented.
//
// Revision 1.38  2003/06/10 21:48:06  ssntails
// Variable flat size support (32x32 to 2048x2048)
//
// Revision 1.37  2002/07/28 20:29:18  hurdler
// "Fix" holes in the sky
//
// Revision 1.36  2001/12/26 15:56:12  hurdler
// Manage transparent wall a little better
//
// Revision 1.35  2001/03/03 06:17:34  bpereira
// Revision 1.34  2001/02/28 17:50:56  bpereira
// Revision 1.33  2001/02/24 13:35:22  bpereira
// Revision 1.32  2001/01/25 18:56:27  bpereira
// Revision 1.31  2000/11/04 16:23:44  bpereira
// Revision 1.30  2000/11/02 22:16:03  bpereira
// Revision 1.29  2000/11/02 21:54:26  bpereira
// Revision 1.28  2000/11/02 19:49:39  bpereira
// Revision 1.27  2000/10/04 16:21:57  hurdler
// Revision 1.26  2000/09/28 20:57:20  bpereira
// Revision 1.25  2000/08/31 14:30:57  bpereira
// Revision 1.24  2000/08/11 19:11:57  metzgermeister
// Revision 1.23  2000/08/03 17:57:42  bpereira
//
// Revision 1.22  2000/07/13 21:07:47  metzgermeister
// fixed memory leak
//
// Revision 1.21  2000/07/01 09:23:50  bpereira
//
// Revision 1.20  2000/05/30 18:01:53  kegetys
// Added the chromakey flag to sprites
//
// Revision 1.19  2000/05/09 22:08:53  hurdler
// fix large sky problem
//
// Revision 1.18  2000/05/09 20:57:31  hurdler
// use my own code for colormap (next time, join with Boris own code)
// (necessary due to a small bug in Boris' code (not found) which shows strange effects under linux)
//
// Revision 1.17  2000/04/30 10:30:10  bpereira
//
// Revision 1.16  2000/04/27 17:48:47  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.15  2000/04/24 20:24:38  bpereira
//
// Revision 1.14  2000/04/24 17:23:26  hurdler
// Better support of colormap
//
// Revision 1.13  2000/04/24 15:46:34  hurdler
// Support colormap for text
//
// Revision 1.12  2000/04/23 16:19:52  bpereira
//
// Revision 1.11  2000/04/23 00:30:47  hurdler
// fix a small bug in skin color
//
// Revision 1.10  2000/04/22 21:08:23  hurdler
// Revision 1.9  2000/04/22 20:16:30  hurdler
//
// Revision 1.8  2000/04/22 19:12:50  hurdler
// support skin color in hardware mode
//
// Revision 1.7  2000/04/22 16:09:14  hurdler
// support skin color in hardware mode
//
// Revision 1.6  2000/04/18 12:52:21  hurdler
// Revision 1.5  2000/04/16 18:38:07  bpereira
//
// Revision 1.4  2000/04/09 17:18:01  hurdler
// modified coronas' code for 16 bits video mode
//
// Revision 1.3  2000/04/06 20:50:23  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      load and convert graphics to the hardware format
//
//-----------------------------------------------------------------------------

#include "doomincl.h"

#include "hw_glob.h"
#include "hw_drv.h"

#include "doomstat.h"
  //gamemode
#include "i_video.h"
  //rendermode
#include "m_swap.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"
#include "v_video.h"
#include "r_sky.h"

#if 0
// [WDJ] Replaced global cache draw flags with drawflags parameter and TF_ flags
//Hurdler: 25/04/2000: used for new colormap code in hardware mode
//unused
byte *gr_colormap = NULL; // by default it must be NULL ! (because colormap tables are not initialized)
#endif

// --------------------------------------------------------------------------
// Values set after a call to HWR_ResizeBlock()
// --------------------------------------------------------------------------
static  int     blocksize;
static  int     blockwidth;
static  int     blockheight;

extern byte *   translucenttables;  // set of translucent tables 

// Uses format values from Glide
int patchformat   = GR_TEXFMT_AP_88; // use alpha for holes
int textureformat = GR_TEXFMT_P_8; // use chromakey for hole

// [WDJ] a strange palette pixel format with alpha
// to replace an endian swap within inner draw loop
// *((unsigned short*)dest) = LE_SWAP16( (alpha<<8) | texel );
typedef struct {
   byte pixel; // paletted color
   byte alpha;
} pixelalpha_t;

#if 0
typedef union {
   uint16_t     ui16;
   pixelalpha_t s16;
} pixelunion16a_t;
#endif

// sprite, use alpha and chroma key for hole
// draw flags in mipmap->tfflags
// Called from HWR_GetTexture->HWR_GenerateTexture
// Called from HWR_MakePatch
static
void HWR_DrawPatchInCache (Mipmap_t* mipmap,
                           int blockwidth, int blockheight, int blocklinebyte,
                           int texture_width, int texture_height,
                           int originx, int originy, //where to draw the patch in the surface block
                           patch_t* sw_patch, int bytepp )
{
    int          x,x1,x2;
    int          col,ncols;
    int          count;
    fixed_t      xfrac, xfracstep;
    fixed_t      yfrac, yfracstep;
    fixed_t      scale_y;

    byte         alpha;
    byte         chromakey_mapped = (mipmap->tfflags & TF_CHROMAKEYED)? 1:0;
    // also can be called before translucenttables are setup
    byte        *fx1trans =   // only one opaque trans so far
        ((mipmap->tfflags & TF_Opaquetrans) && translucenttables)?
          & translucenttables[ TRANSLU_TABLE_fx1 ]
          : NULL;
    byte      *  colormap = mipmap->colormap;
    byte      *  block = mipmap->GR_data;
    column_t  *  patchcol;
    byte      *  source;
    byte      *  dest_col, *  dest;

    x1 = originx;
    x2 = x1 + sw_patch->width;

    x = (x1<0)? 0: x1;

    if (x2 > texture_width)
        x2 = texture_width;

    if( texture_width < 1 )
        return;

    col  = x * blockwidth / texture_width;
    ncols= ((x2-x) * blockwidth) / texture_width;

#if 0
    CONS_Printf("patch %dx%d texture %dx%d block %dx%d\n",
         sw_patch->width, sw_patch->height,
         texture_width, texture_height, blockwidth, blockheight);
    CONS_Printf("      col %d ncols %d x %d\n", col, ncols, x);
#endif

    // source advance
    xfrac = 0;
    if (x1<0)
        xfrac = -x1<<16;

    xfracstep = (texture_width << 16) / blockwidth;
    yfracstep = (texture_height<< 16) / blockheight;
    scale_y = (blockheight << 16) / texture_height;

#ifdef PARANOIA   
    if( bytepp<1 || bytepp > 4 )
        I_Error("HWR_DrawPatchInCache: no drawer defined for this bytepp (%d)\n",bytepp);
#endif

    for( dest_col = block + (col * bytepp); ncols--; dest_col += bytepp, xfrac += xfracstep)
    {
#ifdef DEEPSEA_TALL_PATCH
        // [MB (M. Bauerle), from crispy Doom]  Support for DeePsea tall patches, [WDJ]
        int  cur_topdelta = -1;
#endif
        patchcol = (column_t *)
            ((byte *)sw_patch + sw_patch->columnofs[xfrac>>16]);

        while (patchcol->topdelta != 0xff)
        {
#ifdef DEEPSEA_TALL_PATCH
            // [MB] [WDJ] DeePsea tall patch.
            // DeepSea allows the patch to exceed 254 height.
            // A Doom patch has monotonic ascending topdelta values, 0..254.
            // DeePsea tall patches have an optional relative topdelta.	
            // When the column topdelta is <= the current topdelta,
            // it is a DeePsea tall patch relative topdelta.
            if( patchcol->topdelta <= cur_topdelta )
            {
                cur_topdelta += patchcol->topdelta;
            }
            else
            {
                cur_topdelta = patchcol->topdelta;
            }
#endif

            // The patch is stretched to fill the allocated blockheight.
            // The hardware draws are not affected, as the drawing will use
            // the blockheight as the texture height.
            source = (byte *)patchcol + 3;
            count  = ((patchcol->length * scale_y) + (FRACUNIT/2)) >> 16;
            yfrac = 0;

#ifdef DEEPSEA_TALL_PATCH
            int yi = originy + cur_topdelta;
#else
            int yi = originy + patchcol->topdelta;
#endif
            //yfracstep = (patchcol->length << 16) / count;
            if( yi < 0 )
            {
                // [WDJ] Original doom had a bug in clipping.
                // To reproduce that bug, comment out the next line.
                yfrac = -yi<<16;  // skip pixels in patch (not in original doom)

                count += (((yi * scale_y) + (FRACUNIT/2)) >> 16);
                yi = 0;
            }

#if 0
            // Cuts off the bottom ?
            if( count >= texture_height - yi )
                count = texture_height - yi;
#endif

            unsigned int ypos = ((yi * scale_y) + (FRACUNIT/2)) >> 16;
            dest = dest_col + (ypos * blocklinebyte);

	    // count of dest, must compare to dest index
            if( count > blockheight - ypos )
                count = blockheight - ypos;

            while( count-- > 0 )
            {
                byte texel = source[yfrac>>16];

                // [WDJ] Fixed, this is fx1 not fire
                // Verified that 0x40000 is the fx1 translucent table.
                if( fx1trans && (fx1trans[(texel<<8)] != texel) )
                    alpha = 0x80;
                else
                    alpha = 0xff;

                //Hurdler: not perfect, but better than holes
                // Move pixels conflicting with chromakey to a similar color
                if( chromakey_mapped && texel == HWR_PATCHES_CHROMAKEY_COLORINDEX )
                    texel = HWR_CHROMAKEY_EQUIVALENTCOLORINDEX;
                //Hurdler: 25/04/2000: now support colormap in hardware mode
                else if (colormap)
                    texel = colormap[texel];

                // hope compiler will get this switch out of the loops (dreams...)
                // gcc do it ! but vcc not ! (why don't use cygnus gcc for win32 ?)
                switch (bytepp)
                {
                    case 2 :
                       ((pixelalpha_t*)dest)->pixel = texel;
                       ((pixelalpha_t*)dest)->alpha = alpha;
                       break;
                    case 3 :
                       ((RGBA_t*)dest)->s.red   = V_GetColor(texel).s.red;
                       ((RGBA_t*)dest)->s.green = V_GetColor(texel).s.green;
                       ((RGBA_t*)dest)->s.blue  = V_GetColor(texel).s.blue;
                       break;
                    case 4 :
                       *((RGBA_t*)dest) = V_GetColor(texel);
                       ((RGBA_t*)dest)->s.alpha = alpha;
                       break;
                    default:  // default is 1
                       *dest = texel;
                       break;
                }

                dest += blocklinebyte;
                yfrac += yfracstep;
            }
            patchcol = (column_t *)(  (byte *)patchcol + patchcol->length + 4);
        }
    }
}


// resize the patch to be 3dfx complient
// set : blocksize = blockwidth * blockheight  (no bpp used)
//       blockwidth
//       blockheight
//note :  8bit (1 byte per pixel) palettized format
static void HWR_ResizeBlock ( int orig_width,
                              int orig_height,
                              Mipmap_t * mipmap )
{
#ifdef USE_VOODOO_GLIDE
    //   Build the full textures from patches.
    static const GrLOD_t     gr_lods[9] = {
        GR_LOD_LOG2_256,
            GR_LOD_LOG2_128,
            GR_LOD_LOG2_64,
            GR_LOD_LOG2_32,
            GR_LOD_LOG2_16,
            GR_LOD_LOG2_8,
            GR_LOD_LOG2_4,
            GR_LOD_LOG2_2,
            GR_LOD_LOG2_1
    };
    
    typedef struct {
        GrAspectRatio_t aspect;
        float           max_s;
        float           max_t;
    } booring_aspect_t;
    
    static const booring_aspect_t gr_aspects[8] = {
        {GR_ASPECT_LOG2_1x1, 255, 255},
        {GR_ASPECT_LOG2_2x1, 255, 127},
        {GR_ASPECT_LOG2_4x1, 255,  63},
        {GR_ASPECT_LOG2_8x1, 255,  31},
        
        {GR_ASPECT_LOG2_1x1, 255, 255},
        {GR_ASPECT_LOG2_1x2, 127, 255},
        {GR_ASPECT_LOG2_1x4,  63, 255},
        {GR_ASPECT_LOG2_1x8,  31, 255}
    };


    GrTexInfo *  grInfo = & mipmap->grInfo;

    int     j,k;
    int     max,min;

    // find a power of 2 width/height
    if (cv_grrounddown.value)
    {
        blockwidth = 256;
        while (orig_width < blockwidth)
            blockwidth >>= 1;
        if( blockwidth<1 )
        {
            I_SoftError ("3D GenerateTexture : too small");
            blockwidth = 4;
        }

        blockheight = 256;
        while (orig_height < blockheight)
            blockheight >>= 1;
        if( blockheight<1 )
        {
            I_SoftError ("3D GenerateTexture : too small");
            blockheight = 4;
        }
    }
/*    else if (cv_voodoocompatibility.value)
    {
                if(orig_width > 256 || orig_height > 256)
                {
                        blockwidth = 256;
                        while (orig_width < blockwidth)
                                blockwidth >>= 1;
                        if( blockwidth<1 )
                        {
                            I_SoftError ("3D GenerateTexture : too small");
                            blockwidth = 4;
                        }

                        blockheight = 256;
                        while (orig_height < blockheight)
                                blockheight >>= 1;
                        if( blockheight<1 )
                        {
                            I_SoftError ("3D GenerateTexture : too small");
                            blockheight = 4;
                        }
                }
                else
                {
                        //size up to nearest power of 2
                        blockwidth = 1;
                        while (blockwidth < orig_width)
                                blockwidth <<= 1;
                        // scale down the original graphics to fit in 256
                        if (blockwidth>256)
                                blockwidth = 256;
                                //I_Error ("3D GenerateTexture : too big");

                        //size up to nearest power of 2
                        blockheight = 1;
                        while (blockheight < orig_height)
                                blockheight <<= 1;
                        // scale down the original graphics to fit in 256
                        if (blockheight>256)
                                blockheight = 255;
                                //I_Error ("3D GenerateTexture : too big");
                }
    }*/
    else
    {
        //size up to nearest power of 2
        blockwidth = 1;
        while (blockwidth < orig_width)
            blockwidth <<= 1;
        // scale down the original graphics to fit in 256
        if (blockwidth>2048)
            blockwidth = 2048;
            //I_Error ("3D GenerateTexture : too big");

        //size up to nearest power of 2
        blockheight = 1;
        while (blockheight < orig_height)
            blockheight <<= 1;
        // scale down the original graphics to fit in 256
        if (blockheight>2048)
            blockheight = 2048;
            //I_Error ("3D GenerateTexture : too big");
    }

    // do the boring LOD stuff.. blech!
    if (blockwidth >= blockheight)
    {
        max = blockwidth;
        min = blockheight;
    }else{
        max = blockheight;
        min = blockwidth;
    }

    for (k=256, j=0; k > max; j++)
        k>>=1;
    grInfo->smallLodLog2 = gr_lods[j];
    grInfo->largeLodLog2 = gr_lods[j];

    for (k=max, j=0; k>min && j<4; j++)
        k>>=1;
    // aspect ratio too small for 3Dfx (eg: 8x128 is 1x16 : use 1x8)
    if (j==4)
    {
        j=3;
        //CONS_Printf ("HWR_ResizeBlock : bad aspect ratio %dx%d\n", blockwidth,blockheight);
        if (blockwidth<blockheight)
            blockwidth = max>>3;
        else
            blockheight = max>>3;
    }
    if (blockwidth<blockheight)
        j+=4;
    grInfo->aspectRatioLog2 = gr_aspects[j].aspect;

#else
    // Not Voodoo Glide
#if 1    
    // GL prefers power-of-two textures.
    {
        // width: size up to nearest power of 2
        blockwidth = 1;
        while(blockwidth < orig_width)
            blockwidth <<= 1;
        if(blockwidth>2048)
        {
            // scale down the original graphics to fit
            blockwidth = 2048;
        }

        //height: size up to nearest power of 2
        blockheight = 1;
        while(blockheight < orig_height)
            blockheight <<= 1;
        if(blockheight>2048)
        {
            // scale down the original graphics to fit
            blockheight = 2048;
        }
    }
#else    
    blockwidth = orig_width;
    blockheight = orig_height;
#endif
#endif

    blocksize = blockwidth * blockheight;
}


// bytes per pixel, index by GrTextureFormat
static const int format2bpp[16] = {
    0, //0
    0, //1
    1, //2  GR_TEXFMT_ALPHA_8
    1, //3  GR_TEXFMT_INTENSITY_8
    1, //4  GR_TEXFMT_ALPHA_INTENSITY_44
    1, //5  GR_TEXFMT_P_8
    4, //6  GR_RGBA
    0, //7
    0, //8
    0, //9
    2, //10 GR_TEXFMT_RGB_565
    2, //11 GR_TEXFMT_ARGB_1555
    2, //12 GR_TEXFMT_ARGB_4444
    2, //13 GR_TEXFMT_ALPHA_INTENSITY_88
    2, //14 GR_TEXFMT_AP_88
};

static byte * Make_Mip_Block( Mipmap_t * mipmap )
{
    int bytepp = format2bpp[ mipmap->GR_format ];
    byte * block = mipmap->GR_data;  // existing block

    // Reallocate data block if it is wrong size.
    if( (block == NULL)
        || ((mipmap->width * mipmap->height) != blocksize) )
    {
        // Need new allocation
        if( block != NULL )  // free any existing data
            Z_Free( block );

        // set mipmap GR_data
        block = Z_Malloc (blocksize*bytepp, PU_STATIC, &mipmap->GR_data );
    }

    // mipmap updated with data block
    mipmap->width = blockwidth;
    mipmap->height = blockheight;

    switch (bytepp)
    {
        case 1:
           memset(block, HWR_PATCHES_CHROMAKEY_COLORINDEX, blocksize );
           break;
        case 2:
           {
                // fill background with chromakey, alpha=0
                pixelalpha_t alphachr = {HWR_PATCHES_CHROMAKEY_COLORINDEX, 0};
                pixelalpha_t * blka = (pixelalpha_t*) block;
                int i;
                for( i=0; i<blocksize; i++ )
                   blka[i] = alphachr;
           }
           break;
        case 4:
           memset(block,0,blocksize*4);
           break;
    }

    return block;
}

static void release_Mip_Block( Mipmap_t * mipmap )
{
    if( mipmap->GR_data != NULL )
    {
        // free existing data       
        Z_Free( mipmap->GR_data );
        // some Z_Free methods will clear owner, but not all
        mipmap->GR_data = NULL;  // mark as empty
    }
}

//
// Create a composite texture from patches,
// adapt the texture size to a power of 2
// height and width for the hardware texture cache.
// Usually called with mipmap from grtex, there are cases where it may
// use another mipmap.
//
// drawflags: TF_Opaquetrans
// Called from HWR_GetTexture
static void HWR_GenerateTexture (int texnum, MipTexture_t* grtex,
                                 Mipmap_t * mipmap, uint32_t drawflags)
{
    byte*               block;
    texture_t*          texture;
    texpatch_t*         texpatch;
    patch_t*            sw_patch;

    int         i;
    int         bytepp;  // bytes per pixel
    boolean     skyspecial = false; //poor hack for Legacy large skies..

    texture = textures[texnum];

    // hack the Legacy skies.. texture size is 256x128 but patch size is larger..
    if ( texture->name[0] == 'S' &&
         texture->name[1] == 'K' &&
         texture->name[2] == 'Y' &&
         texture->name[4] == 0 )
    {
        skyspecial = true;
        mipmap->tfflags = TF_WRAPXY; // don't use the chromakey for sky
    }
    else
        mipmap->tfflags = TF_CHROMAKEYED | TF_WRAPXY;
    mipmap->tfflags |= drawflags;  // TF_Opaquetrans

    mipmap->GR_format = textureformat;
    bytepp = format2bpp[ textureformat ];

    HWR_ResizeBlock( texture->width, texture->height, mipmap );

    block = Make_Mip_Block( mipmap );  // sets GR_data

    if (skyspecial) //Hurdler: not efficient, but better than holes in the sky (and it's done only at level loading)
    {
        int i, j;
        RGBA_t col = V_GetColor(HWR_CHROMAKEY_EQUIVALENTCOLORINDEX);
        // init sky with col so composite cannot leave any transparent holes,
        // must be 32bit
        RGBA_t * bc = (RGBA_t*) block;
        for (j=0; j<blockheight; j++)
        {
            for (i=0; i<blockwidth; i++)
            {
                *bc++ = col;  // endian tolerant
            }
        }
    }

    // Composite the columns together.
    texpatch = texture->patches;
    for (i=0 ; i<texture->patchcount; i++, texpatch++)
    {
        sw_patch = W_CachePatchNum_Endian( texpatch->lumpnum, PU_CACHE );
        // correct texture size for Legacy's large skies
        if (skyspecial) {
            //CONS_Printf("sky %d, %d\n",texture->width,sw_patch->width);
            //texture->width = sw_patch->width;
            texture->height = sw_patch->height;
        }
        HWR_DrawPatchInCache( mipmap,
                              blockwidth, blockheight, blockwidth*bytepp,
                              texture->width, texture->height,
                              texpatch->originx, texpatch->originy,
                              sw_patch, bytepp );
    }
     //Hurdler: not efficient at all but I don't remember exactly how HWR_DrawPatchInCache works :(
    if (bytepp==4)
    {
        // if any pixel is left unwritten (still init to 0), then TF_TRANSPARENT
        for (i=3; i<blocksize; i+=4)
        {
            if (block[i] == 0)
            {
                mipmap->tfflags |= TF_TRANSPARENT;
                break;
            }
        }
    }

    // make it purgable from zone memory
    // use PU_PURGELEVEL so we can Z_FreeTags all at once
    Z_ChangeTag (block, PU_HWRCACHE);

    // to convert fixed_t ceilingheight and floorheight, and x, to texture
    grtex->scaleX = FIXED_TO_FLOAT_MULT / (float)texture->width;
    grtex->scaleY = FIXED_TO_FLOAT_MULT / (float)texture->height;
}



// [WDJ] Generate a foggy texture from base texture as an alternate
static void HWR_GenerateFogTexture (int texnum, Mipmap_t * mipmap,
                                    uint32_t drawflags)
{
    RGBA_t* rgbablock;
    RGBA_t* fb;
    RGBA_t* endpixel;
    RGBA_t* cp;

    int  srcsize, si_line3, si, si_endx;
    int  i, x, y, zc;
    unsigned int  fc_g, fc_r, fc_b;
    RGBA_t fc, fc_avg;
    RGBA_t fogcolor = {.rgba=0x10101010};  // any endian
    fogcolor.s.alpha = 0xFF;
   
    // must have original texture first
    MipTexture_t* base = HWR_GetTexture (texnum, 0);
    RGBA_t * src = (RGBA_t*) base->mipmap.GR_data;
    srcsize = base->mipmap.height * base->mipmap.width;

    mipmap->tfflags = TF_WRAPXY | TF_Fogsheet | drawflags;
    mipmap->GR_format = GR_RGBA;

    HWR_ResizeBlock( FOG_WIDTH, FOG_HEIGHT, mipmap );

    rgbablock = (RGBA_t*) Make_Mip_Block( mipmap );  // sets GR_data
    fb = rgbablock;
    endpixel = rgbablock + ((blockheight * blockwidth) - 1);

    // Emulate column blend of other drawers
    si_line3 = 3 * base->mipmap.height;
    // average 16 pixels
    fc_r = fc_g = fc_b = 0;
    si = 0;
    for( i=0; i<16; i++ )
    {
        // find non-transparent pixel
        for( zc = srcsize; zc > 0; zc-- )
        {
            si += si_line3 + 17;  // 3 lines + 17 pixels
            if( si >= srcsize )  si -= srcsize;
            fc.rgba = src[si].rgba;
            if( fc.s.alpha > 0x20 )  break;
            // if all transparent, will avg random pixel noise
        }
        fc_r += fc.s.red;
        fc_g += fc.s.green;
        fc_b += fc.s.blue;
    }
    fc_avg.s.red = fc_r >> 4; // avg
    fc_avg.s.green = fc_g >> 4;
    fc_avg.s.blue = fc_b >> 4;
    fc_avg.s.alpha = 0xff;
    fogcolor.rgba = fc_avg.rgba;  // init
    // make a fog texture from averaged colors of linedef texture
    for( x=0; x<blockwidth+32; x++ )  // wrap around width to smooth
    {
        if( x > blockwidth )
        {
            // reduce visible artifact by smoothing over x wrap edge
            fb = & rgbablock[ x - blockwidth ];
            fc_r = fb->s.red * 3;
            fc_g = fb->s.green * 3;
            fc_b = fb->s.blue * 3;
        }
        else
        {
            si = x % base->mipmap.width;  // top of column
            // always average three pixels of source texture
            fc_r = fc_g = fc_b = 0;
            for( i=0; i<3; i++ )
            {
                for( zc = 32; ; )
                {
                    fc.rgba = src[si].rgba;
                    if( fc.s.alpha > 0x20 )  break;
                    // skip transparent pixels
                    si += base->mipmap.width;  // down column
                    if( si >= srcsize )  si -= srcsize;  // wrap source
                    if( --zc < 0 )  // too many transparent pixel
                    {
                        fc.rgba = fc_avg.rgba;
                        break;
                    }
                }
                fc_r += fc.s.red;
                fc_g += fc.s.green;
                fc_b += fc.s.blue;
                si += si_line3;
                if( si > srcsize )  si -= srcsize;  // wrap source
            }
        }
        // blend 61 and 3 = 4.6875%
        fogcolor.s.red = ((((uint16_t)fogcolor.s.red)*61) + fc_r) >> 6;
        fogcolor.s.green = ((((uint16_t)fogcolor.s.green)*61) + fc_g) >> 6;
        fogcolor.s.blue = ((((uint16_t)fogcolor.s.blue)*61) + fc_b) >> 6;

        // place fog down entire column
        for( cp=fb; cp<=endpixel; cp+=blockwidth )
        {
            cp->rgba = fogcolor.rgba;
        }
        fb++;
    }
    // copy any masked outline into fog
    for( y=0; y<blockheight; y++ )
    {
       si= (y % base->mipmap.height) * base->mipmap.width;  // wrap
       si_endx = si + base->mipmap.width;
       fb= & rgbablock[y * blockwidth];
       for( x=0; x<blockwidth; x++ )
       {
           // Draw masked only recognizes alpha != 0
           fb->s.alpha = src[si].s.alpha;
           fb++;
           si++;
           if( si >= si_endx )  // wrap source, larger than fog
             si = si_endx - base->mipmap.width;
       }
    }

    // make it purgable from zone memory
    // use PU_PURGELEVEL so we can Z_FreeTags all at once
    Z_ChangeTag (rgbablock, PU_HWRCACHE);
}


// grTex : Hardware texture cache info
//         .data : address of converted patch in heap memory
//                 user for Z_Malloc(), becomes NULL if it is purged from the cache
// drawflags can be TF_Opaquetrans
// Called from HWR_Draw* -> HWR_LoadMappedPatch
// Called from HWR_GetPatch
// Called from W_CachePatchNum, W_CacheMappedPatchNum
void HWR_MakePatch (patch_t* patch, MipPatch_t* grPatch, Mipmap_t *grMipmap,
                    uint32_t drawflags)
{
    byte*   block;
    int     newwidth, newheight;
    int     bytepp;

    // don't do it twice (like a cache)
    if(grMipmap->width==0)
    {
        // save the original patch header so that the MipPatch can be casted
        // into a standard patch_t struct and the existing code can get the
        // orginal patch dimensions and offsets.
        grPatch->width = patch->width;
        grPatch->height = patch->height;
        grPatch->leftoffset = patch->leftoffset;
        grPatch->topoffset = patch->topoffset;

        // find the good 3dfx size (boring spec)
        HWR_ResizeBlock( patch->width, patch->height, grMipmap );

        // setup the texture info
        grMipmap->GR_format = patchformat;

        // no wrap around, no chroma key
        grMipmap->tfflags = drawflags;  // TF_Opaquetrans
    }
    else
    {
        blockwidth = grMipmap->width;
        blockheight = grMipmap->height;
        blocksize = blockwidth * blockheight;
    }

    block = Make_Mip_Block(grMipmap);  // set GR_data

    // if rounddown, rounddown patches as well as textures
    if (cv_grrounddown.value)
    {
        newwidth = blockwidth;
        newheight = blockheight;
    }
/*	else if(cv_voodoocompatibility.value) // Only scales down textures that exceed 256x256.
        {
                // no rounddown, do not size up patches, so they don't look 'scaled'
        newwidth  = min( patch->width , blockwidth );
        newheight = min( patch->height, blockheight);

                if(newwidth > 256 || newheight > 256)
                {
                        newwidth = blockwidth;
                        newheight = blockheight;
                }
        }*/
    else
    {
        // no rounddown, do not size up patches, so they don't look 'scaled'
        newwidth  = min( patch->width , blockwidth );
        newheight = min( patch->height, blockheight);
    }

    bytepp = format2bpp[ grMipmap->GR_format ];
    HWR_DrawPatchInCache( grMipmap,
                          newwidth, newheight, blockwidth*bytepp,
                          patch->width, patch->height,
                          0, 0,
                          patch, bytepp );

    grPatch->max_s = (float)newwidth / (float)blockwidth;
    grPatch->max_t = (float)newheight / (float)blockheight;

    // Now that the texture has been built in cache, it is purgable from zone memory.
    Z_ChangeTag (block, PU_HWRCACHE);
}


// This releases the allocation made with HWR_MakePatch
void HWR_release_Patch ( MipPatch_t* grPatch, Mipmap_t *grMipmap )
{
    release_Mip_Block( grMipmap );
    grMipmap->width = 0;
}



// =================================================
//             CACHING HANDLING
// =================================================

static int  gr_numtextures = 0;
static MipTexture_t*  gr_textures = NULL;  // for ALL Doom textures, malloc

void HWR_Init_TextureCache (void)
{
    gr_numtextures = 0;
    gr_textures = NULL;
}

// Called from P_SetupLevel->HWR_PrepLevelCache
// Coordinate with malloc in HWR_GetMappedPatch
void HWR_Free_TextureCache (void)
{
    int i, wf;

    if( HWD.pfnClearMipMapCache == NULL )   return;  // cache never set
   
    // free references to the textures
    HWD.pfnClearMipMapCache ();
   
    // Free GR_data before freeing mipmaps that Z_FreeTags will write.
    // Free all hardware-converted graphics cached in the heap.
    // Our goal is only the textures since user of the texture is the texture cache.
    Z_FreeTags (PU_HWRCACHE, PU_HWRCACHE);

    // free all skin after each level: must be done after pfnClearMipMapCache!
    for(wf=0; wf<numwadfiles; wf++)
    {
        for (i=0; i<wadfiles[wf]->numlumps; i++)
        {
            // Free all colormap mipmaps.
            MipPatch_t *grpatch = &(wadfiles[wf]->hwrcache[i]);
            Mipmap_t * grmip = grpatch->mipmap.nextcolormap;
            grpatch->mipmap.nextcolormap = NULL;	   
            while( grmip )
            {
                Mipmap_t * grmip_nxt = grmip->nextcolormap;
                free(grmip);
                grmip = grmip_nxt;	        
            }
        }
    }

    // now the heap don't have any 'user' pointing to our
    // texturecache info, we can free it
    if (gr_textures)
    {
        // destroy all of gr_textures
        for( i=0; i<gr_numtextures; i++ )
        {
            // free alternate texture mipmap used for TF_Opaquetrans
            Mipmap_t * altmip = gr_textures[i].mipmap.nextcolormap;
            while( altmip )
            {
                register Mipmap_t * nxt = altmip->nextcolormap;
                free(altmip);
                altmip = nxt;
            }
        }
        free (gr_textures);
        gr_textures = NULL;
    }
}

// Called from P_SetupLevel->HWR_Preload_Graphics
void HWR_Prep_LevelCache (int numtextures)
{
    // problem: the mipmap cache management hold a list of mipmaps.. but they are
    //           reallocated on each level..
    //sub-optimal, but
    //  1) just need re-download stuff in hardware cache VERY fast
    //  2) sprite/menu stuff mixed with level textures so can't do anything else

    // we must free it since numtextures changed
    HWR_Free_TextureCache ();

    gr_numtextures = numtextures;
    gr_textures = malloc (sizeof(MipTexture_t) * numtextures);
    if (!gr_textures)
        I_Error ("3D can't alloc gr_textures");
    memset (gr_textures, 0, sizeof(MipTexture_t) * numtextures);
}

void HWR_SetPalette( RGBA_t *palette )
{
    if( HWD.pfnSetPalette )
    {
        //Hurdler: 16/10/99: added for OpenGL gamma correction
        RGBA_t  gamma_correction = {0x7F7F7F7F};
    
        gamma_correction.s.red   = cv_grgammared.value;
        gamma_correction.s.green = cv_grgammagreen.value;
        gamma_correction.s.blue  = cv_grgammablue.value;
        HWD.pfnSetPalette( palette, &gamma_correction );
    }

    // hardware driver will flush their own cache if cache is non paletized
    // now flush data texture cache so 32 bit texture are recomputed
    if( patchformat == GR_RGBA || textureformat == GR_RGBA )
        Z_FreeTags (PU_HWRCACHE, PU_HWRCACHE);
}


// Imitate the special object screen tints for each special palette.
// Corresponding to Doom special effect palettes.
// These seem to be the same for Doom and Heretic.
static uint32_t  palette_to_tint[16] =
{
   0x0,  // 00 normal
   0xff373797, // 01 red
   0xff373797, // 02 red
   0xff3030a7, // 03 red
   0xff2727b7, // 04 red
   0xff2020c7, // 05 red
   0xff1717d7, // 06 red
   0xff1010e7, // 07 red
   0xff0707f7, // 08 red
   0xffff6060, // 09 blue
   0xff70a090, // 0A light green
   0xff67b097, // 0B light green
   0xff60c0a0, // 0C light green
   0xff60ff60, // 0D green
   0xffff6060, // 0E blue
   0xffff6060  // 0F blue
};


// Enables flash palette.
byte  EN_HWR_flashpalette = 0;

// Faster palette flashes using tints.
//  palette_num : 0..15
void HWR_SetFlashPalette( byte palette_num )
{
        
    //faB - NOW DO ONLY IN SOFTWARE MODE, LETS FIX THIS FOR GOOD OR NEVER
    //      idea : use a true color gradient from frame to frame, because we
    //             are in true color in HW3D, we can have smoother palette change
    //             than the palettes defined in the wad

    //Hurdler: TODO: see if all heretic palettes are properly managed

    // Could change the HW palette used for paletted textures, but
    // that would require conversion of textures to RGB again.
    // This sets a tint that is averaged with the surface color
    // by the drawing engine.
    HWD.pfnSetSpecialState(HWD_SET_TINT_COLOR, palette_to_tint[palette_num] );
}


// --------------------------------------------------------------------------
// Make sure texture is downloaded and set it as the source
// --------------------------------------------------------------------------
// drawflags: TF_Opaquetrans
// Called from HWR_RenderSkyPlane // commented out
// Called from HWR_DrawSkyBackground
// Called from HWR_SplitWall
// Called from HWR_StoreWallRange
// Called from HWR_RenderTransparentWalls
MipTexture_t* HWR_GetTexture (int tex, uint32_t drawflags)
{
    MipTexture_t * miptex;
    Mipmap_t * mipmap;
#ifdef PARANOIA
    if( tex>=gr_numtextures )
        I_Error(" HWR_GetTexture : tex>=numtextures\n");
#endif
    miptex = &gr_textures[tex];
    mipmap = &(miptex->mipmap);  // mipmap in miptex
    if( miptex->mipmap.GR_data || miptex->mipmap.downloaded )
    {
        uint32_t tstflags = drawflags & (TF_Opaquetrans|TF_Fogsheet);
        // mipmap already in use, find matching flags
        for(; ; mipmap = mipmap->nextcolormap)
        {
            if ((mipmap->tfflags & (TF_Opaquetrans|TF_Fogsheet)) == tstflags)
                goto found_mipmap;
            if( ! mipmap->nextcolormap )  break;
        }
        {
            // no matching mipmap found, make new one as alternate
            Mipmap_t * newmip = malloc(sizeof(Mipmap_t));
            if( newmip == NULL )
               I_Error(" HWR_GetTexture : mipmap alloc failed\n");

            mipmap->nextcolormap = newmip;  // link
            memset(newmip, 0, sizeof(Mipmap_t));

            mipmap = newmip;
        }
    }
    else if (drawflags & TF_Fogsheet)
    {
         // Do not have base texture, base mipmap must go to base texture.
         // These cannot overlap, otherwise wrong mipmap gets used
         HWR_GetTexture (tex, 0);  // get base texture first
         miptex = HWR_GetTexture (tex, drawflags);  // gen fog texture mipmap
         return miptex;
    }

    if( drawflags & TF_Fogsheet )
    {
        // generate mipmap with foggy texture as alternate
        HWR_GenerateFogTexture (tex, mipmap, drawflags);
    }
    else
    {
        // generate mipmap with texture
        HWR_GenerateTexture (tex, miptex, mipmap, drawflags);
    }

found_mipmap:
    HWD.pfnSetTexture (mipmap);
    return miptex;
}


static void HWR_CacheFlat (Mipmap_t* grMipmap, int flatlumpnum)
{
    byte *block;
    int size, flatsize;

    // setup the texture info
#ifdef USE_VOODOO_GLIDE
    grMipmap->grInfo.smallLodLog2 = GR_LOD_LOG2_64;
    grMipmap->grInfo.largeLodLog2 = GR_LOD_LOG2_64;
    grMipmap->grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
#endif   
    grMipmap->GR_format = GR_TEXFMT_P_8;
    grMipmap->tfflags = TF_WRAPXY;

    size = W_LumpLength(flatlumpnum);

    switch(size)
    {
                case 4194304: // 2048x2048 lump
                        flatsize = 2048;
                        break;
                case 1048576: // 1024x1024 lump
                        flatsize = 1024;
                        break;
                case 262144:// 512x512 lump
                        flatsize = 512;
                        break;
                case 65536: // 256x256 lump
                        flatsize = 256;
                        break;
                case 16384: // 128x128 lump
                        flatsize = 128;
                        break;
                case 1024: // 32x32 lump
                        flatsize = 32;
                        break;
                default: // 64x64 lump
                        flatsize = 64;
                        break;
    }
    grMipmap->width  = flatsize;
    grMipmap->height = flatsize;

    // the flat raw data needn't be converted with palettized textures
    block = Z_Malloc (W_LumpLength(flatlumpnum),
                      PU_HWRCACHE,
                      &grMipmap->GR_data);

    W_ReadLump (flatlumpnum, block);
}


// Download a Doom 'flat' to the hardware cache and make it ready for use
void HWR_GetFlat (lumpnum_t flatlumpnum)
{
    Mipmap_t * grmip;

    grmip = &(wadfiles[ WADFILENUM(flatlumpnum) ]->hwrcache[ LUMPNUM(flatlumpnum) ].mipmap);

    if (!grmip->downloaded
        && !grmip->GR_data )
            HWR_CacheFlat (grmip, flatlumpnum);

    HWD.pfnSetTexture (grmip);
}

//
// HWR_LoadMappedPatch(): replace the skin color of the sprite in cache
//                          : load it first in doom cache if not already
//
// grmip.tfflags can have TF_Opaquetrans
// Called from HWR_Draw* ->HWR_GetMappedPatch
static void HWR_LoadMappedPatch(Mipmap_t *grmip, MipPatch_t *gpatch)
{
    if( !grmip->downloaded
        && !grmip->GR_data )
    {
        // Load patch to temp, free it afterwards
        patch_t* pp = W_CachePatchNum_Endian(gpatch->patch_lumpnum, PU_IN_USE);
        HWR_MakePatch( pp, gpatch, grmip, grmip->tfflags);

        Z_Free(pp);
    }

    HWD.pfnSetTexture(grmip);
}

// -----------------+
// HWR_GetPatch     : Download a patch to the hardware cache and make it ready for use
// -----------------+
// Called from HWR_Draw* -> HWR_DrawSegsSplats, HWR_DrawPSprite, HWR_DrawPatch
void HWR_GetPatch( MipPatch_t* gpatch )
{
    // is it in hardware cache
    if( !gpatch->mipmap.downloaded
        && !gpatch->mipmap.GR_data )
    {
        // load the software patch, PU_STATIC or the Z_Malloc for hardware patch will
        // flush the software patch before the conversion! oh yeah I suffered
        patch_t * swpatch = W_CachePatchNum_Endian(gpatch->patch_lumpnum, PU_IN_USE);

#ifdef PARANOIA
        if( swpatch == 0
            || swpatch->width != gpatch->width || swpatch->height != gpatch->height
            || swpatch->leftoffset != gpatch->leftoffset || swpatch->topoffset != gpatch->topoffset ) {
            printf("HWR_GetPatch, bad patch: patch_lumpnum = %i\n", gpatch->patch_lumpnum);
        }
#endif

        HWR_MakePatch ( swpatch, gpatch, &gpatch->mipmap, 0);

        // this is inefficient.. but the hardware patch in heap is purgeable so it should
        // not fragment memory, and besides the REAL cache here is the hardware memory
        Z_Free(swpatch);
    }

    HWD.pfnSetTexture( &gpatch->mipmap );
}


// -------------------+
// HWR_GetMappedPatch : Same as HWR_GetPatch for sprite color
// -------------------+
// colormap variations only, drawflags are fixed for whole sprite
// Called from HWR_DrawSmallPatch, HWR_DrawMappedPatch, HWR_DrawSprite, HWR_DrawMD2
void HWR_GetMappedPatch(MipPatch_t* gpatch, byte *colormap)
{
    Mipmap_t   *grmip, *newmip;

    if( (colormap==NULL) || (colormap==reg_colormaps) )
    {
        // Load the default (green) color in doom cache (temporary?) AND hardware cache
        HWR_GetPatch(gpatch);
        return;
    }

    // search for the mipmap
    // skip the first (no colormap translated)
    for(grmip = &gpatch->mipmap ; grmip->nextcolormap ;)
    {
        grmip = grmip->nextcolormap;
        if (grmip->colormap==colormap)
        {
            HWR_LoadMappedPatch( grmip, gpatch );
            return;
        }
    }
    // not found, create it !
    // If we are here, the sprite with the current colormap is not already in hardware memory

    //BP: WARNING : don't free it manually without clearing the cache of hardware renderer
    //              (it have a list of mipmap)
    //    this malloc is cleared in HWR_FreeTextureCache
    //    (...) unfortunately z_malloc fragment alot the memory :( so malloc is better
    newmip = malloc(sizeof(Mipmap_t));
    memset(newmip, 0, sizeof(Mipmap_t));
    grmip->nextcolormap = newmip;

    newmip->colormap   = colormap;
    HWR_LoadMappedPatch( newmip, gpatch );
}

#if 0
// [WDJ] Not needed until a patch needs both with and without TF_Opaquetrans
// May be called with drawflags = TF_Opaquetrans
void HWR_GetMappedPatch(MipPatch_t* gpatch, byte *colormap, uint32_t drawflags)
{
    Mipmap_t   *grmip, *newmip;

    if( !drawflags && ((colormap==NULL) || (colormap==reg_colormaps)) )
    {
        // Load the default (green) color in doom cache (temporary?) AND hardware cache
        HWR_GetPatch(gpatch);
        return;
    }

    // search for the mipmap
    // skip the first (no colormap translated)
    for(grmip = &gpatch->mipmap ; grmip->nextcolormap ;)
    {
        grmip = grmip->nextcolormap;
        if (grmip->colormap==colormap
            && ((grmip->tfflags & TF_Opaquetrans)==drawflags)  )
        {
            HWR_LoadMappedPatch( grmip, gpatch );
            return;
        }
    }
    // not found, create it !
    // If we are here, the sprite with the current colormap is not already in hardware memory

    //BP: WARNING : don't free it manually without clearing the cache of hardware renderer
    //              (it have a list of mipmap)
    //    this malloc is cleared in HWR_FreeTextureCache
    //    (...) unfortunately z_malloc fragment alot the memory :( so malloc is better
    newmip = malloc(sizeof(Mipmap_t));
    grmip->nextcolormap = newmip;
    memset(newmip, 0, sizeof(Mipmap_t));

    newmip->tfflags = drawflags;
    HWR_LoadMappedPatch( newmip, gpatch );
}
#endif

static const int picmode2GR[] = {
    GR_TEXFMT_P_8,                // PALETTE
    0,                            // INTENSITY          (unsuported yet)
    GR_TEXFMT_ALPHA_INTENSITY_88, // INTENSITY_ALPHA    (corona use this)
    0,                            // RGB24              (unsuported yet)
    GR_RGBA,                      // RGBA32             (opengl only)
};

// Called from HWR_GetPic
static
void HWR_DrawPicInCache (byte* block,
                         int blockwidth, int blockheight, int blocklinebyte,
                         pic_t* pic, int dest_bytepp)
{
    int     i,j;
    int     picbytepp;
    int     srcindex;
    fixed_t posx,posy,stepx,stepy;
    byte    *dest,*src,texel;
   
    stepy = ((int)pic->height<<16)/blockheight;
    stepx = ((int)pic->width<<16)/blockwidth;
    picbytepp = format2bpp[picmode2GR[pic->mode]];
    posy = 0;
    for( j=0 ;j<blockheight;j++)
    {
        posx = 0;
        dest = &block[j*blocklinebyte];
        src = &pic->data[(posy>>16)*pic->width*picbytepp];
        for( i=0 ;i<blockwidth;i++)
        {
            srcindex = (posx+FRACUNIT/2)>>16; // rounded to int
            switch (pic->mode) { // source bpp
                case PALETTE :
                    texel = src[srcindex];
                    switch( dest_bytepp ) { // destination bpp
                        case 1 :
                            *dest++ = texel;
                            break;
                        case 2 :
                            ((pixelalpha_t*)dest)->pixel = texel;
                            ((pixelalpha_t*)dest)->alpha = 0xff;
                            dest +=2;
                            break;
                        case 3 : 
                            ((RGBA_t*)dest)->s.red   = V_GetColor(texel).s.red;
                            ((RGBA_t*)dest)->s.green = V_GetColor(texel).s.green;
                            ((RGBA_t*)dest)->s.blue  = V_GetColor(texel).s.blue;   
                            dest += 3;
                            break;
                        case 4 :
                            *(RGBA_t*)dest = V_GetColor(texel); 
                            dest += 4;
                            break;
                    }
                    break;
                case INTENSITY :
                    *dest++ = src[srcindex];
                    break; 
                case INTENSITY_ALPHA : // assume dest bpp = 2
                    *(uint16_t*)dest = ((uint16_t*)src)[ srcindex ]; 
                    dest+=2;
                    break; 
                case RGB24 :
                    break;  // not supported yet
                case RGBA32 : // assume dest bpp = 4
                // [WDJ] without some note, must assume the inc should be after, not before
//                    dest += 4;
                    *(uint32_t*)dest = ((uint32_t*)src)[ srcindex ]; 
                    dest += 4;
                    break; 
            }
            posx += stepx;
        }
        posy += stepy;
    }
}

// -----------------+
// HWR_GetPic       : Download a Doom pic (raw row encoded with no 'holes')
// Returns          :
// -----------------+
MipPatch_t * HWR_GetPic( lumpnum_t lumpnum )
{
    MipPatch_t *grpatch;

    grpatch = &(wadfiles[ WADFILENUM(lumpnum) ]->hwrcache[ LUMPNUM(lumpnum) ]);

    if(    !grpatch->mipmap.downloaded
        && !grpatch->mipmap.GR_data )
    {
        pic_t *pic;
        byte  *block;
        int   len;
        int   newwidth,newheight;

        if( grpatch->mipmap.tfflags & TF_Her_Raw_Pic )
        {
            // raw pic : so get size from grpatch since it is save in v_drawrawscreen
            // [WDJ] CacheRawAsPic is correct endian
            // Will change to PU_CACHE before end of function
            pic = W_CacheRawAsPic( lumpnum, grpatch->width, grpatch->height, PU_IN_USE );
            len = W_LumpLength(lumpnum);
        }
        else
        {
            // Will change to PU_CACHE before end of function
            pic = W_CachePicNum( lumpnum, PU_IN_USE ); // endian fixed
            grpatch->width = pic->width;
            grpatch->height = pic->height;
            len = W_LumpLength(lumpnum) - sizeof(pic_t); 
        }

        grpatch->leftoffset = 0;
        grpatch->topoffset = 0;

        grpatch->mipmap.GR_format = ( pic->mode == PALETTE )?
            textureformat // can be set by driver
            : picmode2GR[ pic->mode ];

        // find the good 3dfx size (boring spec)
        HWR_ResizeBlock( grpatch->width, grpatch->height, &grpatch->mipmap );

        // allocate block
        block = Make_Mip_Block(&grpatch->mipmap);

        // if rounddown, rounddown patches as well as textures
        if (cv_grrounddown.value)
        {
            newwidth = blockwidth;
            newheight = blockheight;
        }
/*		else if(cv_voodoocompatibility.value) // Only scales down textures that exceed 256x256.
                {
                        // no rounddown, do not size up patches, so they don't look 'scaled'
            newwidth  = min(pic->width,blockwidth);
            newheight = min(pic->height,blockheight);

                        if(newwidth > 256 || newheight > 256)
                        {
                                newwidth = blockwidth;
                                newheight = blockheight;
                        }
                }*/
        else
        {
            // no rounddown, do not size up patches, so they don't look 'scaled'
            newwidth  = min(pic->width,blockwidth);
            newheight = min(pic->height,blockheight);
        }


        if( grpatch->width  == blockwidth
            && grpatch->height == blockheight
            && format2bpp[grpatch->mipmap.GR_format] == format2bpp[picmode2GR[pic->mode]] )
        {
            // no conversion needed
            memcpy( grpatch->mipmap.GR_data, pic->data, len );
        }
        else
        {
            HWR_DrawPicInCache(block, newwidth, newheight, 
                               blockwidth*format2bpp[grpatch->mipmap.GR_format],
                               pic,
                               format2bpp[grpatch->mipmap.GR_format]);
        }

        // Release PU_IN_USE
        Z_ChangeTag(pic, PU_CACHE);
        Z_ChangeTag(block, PU_HWRCACHE);

        grpatch->mipmap.tfflags &= TF_Her_Raw_Pic;
        grpatch->max_s = (float)newwidth  / (float)blockwidth;
        grpatch->max_t = (float)newheight / (float)blockheight;
    }
    HWD.pfnSetTexture( &grpatch->mipmap );
    //CONS_Printf("picloaded at %x as texture %d\n",grpatch->mipmap.GR_data, grpatch->mipmap.downloaded);

    return grpatch;
}


// [WDJ] Do not need these options, yet.
//#define  TM_PICT_TRANSLUCENT
//#define  TM_PICT_CHROMAKEY

// May use alpha and chroma key for hole.
// draw flags in mipmap->tfflags
//   tm_format: TM_picture, TM_column_image
// Called from: HWR_sky_mipmap
static
void HWR_draw_TM_cache( byte tm_format, Mipmap_t* mipmap,
                        int blockwidth, int blockheight, int blocklinebyte,
                        int pict_width, int pict_height,
                        int originx, int originy, // where to draw the patch in the surface block
                        byte * sw_pict, int bytepp )
{
    int          x1, x2;
    int          col,ncols;
    int          count, count0;
    fixed_t      xfrac, xfracstep;
    fixed_t      yfrac, yfrac0, yfracstep, ypos, ypos0;
    fixed_t      scale_y;

#ifdef TM_PICT_TRANSLUCENT
    // also can be called before translucenttables are setup
    byte        *fx1trans =   // only one opaque trans so far
        ((mipmap->tfflags & TF_Opaquetrans) && translucenttables)?
          & translucenttables[ TRANSLU_TABLE_fx1 ]
          : NULL;
    byte         alpha;
#else   
    const byte   alpha = 0xff;
#endif
#ifdef TM_PICT_CHROMAKEY
    byte         chromakey_mapped = (mipmap->tfflags & TF_CHROMAKEYED)? 1:0;
#endif
    byte       * colormap = mipmap->colormap;
    byte       * block = mipmap->GR_data;
    byte       * source;  // pixel data, no posts
    byte       * dest;

#ifdef PARANOIA   
    if( bytepp<1 || bytepp > 4 )
    {
        I_SoftError("HWR_draw_TM_picture_cache: bad bytepp %d\n", bytepp);
        return;
    }
#endif

    if( (pict_width < 1) || (pict_height < 1) )
        return;

    // TM_picture, All columns are same height.
    scale_y = (blockheight << 16) / pict_height;  // fixed_t
    count0 = ((pict_height * scale_y) + (FRACUNIT/2)) >> 16;  // int
   
    x1 = originx;
    x2 = x1 + pict_width;
    if( x2 > pict_width )
        x2 = pict_width;

    // source advance
    xfrac = (x1<0)? -x1<<16 : 0;  // skip source pixels
    xfracstep = (pict_width << 16) / blockwidth;
    yfracstep = (pict_height<< 16) / blockheight;
    yfrac0 = 0;
    if( originy < 0 )
    {
        yfrac0 = (-originy)<<16;  // skip source pixels
        count0 += (((originy * scale_y) + (FRACUNIT/2)) >> 16);
        originy = 0;
    }
    ypos0 = ((originy * scale_y) + (FRACUNIT/2)) >> 16;
   
    if( ypos0 + count0 > blockheight )
        count0 = blockheight - ypos0;
   
   
    // dest position
    int x = (x1<0)? 0 : x1;
    col  = x * blockwidth / pict_width;
    ncols= ((x2-x) * blockwidth) / pict_width;

   
#if 0
    CONS_Printf("Draw TM_picture %dx%d to block %dx%d\n",
         pict_width, pict_height, blockwidth, blockheight);
    CONS_Printf("      col %d ncols %d x %d\n", col, ncols, x);
#endif
   
    for( block += col*bytepp; ncols--; block+=bytepp )
    {
        unsigned int srccol = xfrac >> 16;
        source = sw_pict + (
            (tm_format == TM_picture)?
                // TM_picture has columnofs table.
                ((uint32_t*)sw_pict)[srccol]
              : (srccol * pict_height)
                             );

        // TM_picture is columns of pixels.  Each column is pict_height, no offset.
        yfrac = yfrac0;
        ypos = ypos0;
        dest = block + (ypos * blocklinebyte);
        count = count0;
        while( count-- > 0 )
        {
            byte texel = source[yfrac>>16];

#ifdef TM_PICT_TRANSLUCENT

            // [WDJ] Fixed, this is fx1 not fire
            // Verified that 0x40000 is the fx1 translucent table.
            if( fx1trans && (fx1trans[(((unsigned int)texel)<<8)] != texel) )
                alpha = 0x80;
            else
                alpha = 0xff;
#endif

#ifdef TM_PICT_CHROMAKEY
            //Hurdler: not perfect, but better than holes
            // Move pixels conflicting with chromakey to a similar color
            if( chromakey_mapped && texel == HWR_PATCHES_CHROMAKEY_COLORINDEX )
                texel = HWR_CHROMAKEY_EQUIVALENTCOLORINDEX;
            else
#endif
            //Hurdler: 25/04/2000: now support colormap in hardware mode
            if (colormap)
                texel = colormap[texel];

            // hope compiler will get this switch out of the loops (dreams...)
            // gcc do it ! but vcc not ! (why don't use cygnus gcc for win32 ?)
            switch (bytepp) {
             case 2 :
                ((pixelalpha_t*)dest)->pixel = texel;
                ((pixelalpha_t*)dest)->alpha = alpha;
                break;
             case 3 :
                {
                RGBA_t t2 = V_GetColor(texel);
                ((RGBA_t*)dest)->s.red   = t2.s.red;
                ((RGBA_t*)dest)->s.green = t2.s.green;
                ((RGBA_t*)dest)->s.blue  = t2.s.blue;
                break;
                }
             case 4 :
                *((RGBA_t*)dest) = V_GetColor(texel);
                ((RGBA_t*)dest)->s.alpha = alpha;
                break;
             default:  // default is 1
                *dest = texel;
                break;
            }

            dest += blocklinebyte;
            yfrac += yfracstep;
        }
        xfrac += xfracstep;
    }
}


// ---- SKY

Mipmap_t  sky_mipmap;
Mipmap_t  skytop_mipmap;
Mipmap_t  ground_mipmap;
byte      sky_mipmap_state = 0;

void HWR_sky_mipmap( void )
{
    int sky_width = textures[sky_texture]->width;
    byte bytepp;

    // drawflags = 0, no chromakey
    if( sky_mipmap_state == 0 )
    {
        memset( &sky_mipmap, 0, sizeof(Mipmap_t));
        sky_mipmap.tfflags = TF_WRAPXY; // don't use the chromakey for sky

        memset( &skytop_mipmap, 0, sizeof(Mipmap_t));
        skytop_mipmap.tfflags = TF_WRAPXY; // don't use the chromakey for sky
       
        memset( &ground_mipmap, 0, sizeof(Mipmap_t));
        ground_mipmap.tfflags = TF_WRAPXY; // don't use the chromakey for sky
       
        sky_mipmap_state = 1;
    }
    else
    {
        // Need a way to unload mipmap, or refresh it.
        // The only tool available to release a loaded mipmap.
        HWD.pfnClearMipMapCache();  // Release ALL mipmaps.

        // DO NOT TOUCH: sky_mipmap.downloaded
        // causes mip list corruption due to relinking.
    }
   
    sky_mipmap.GR_format = textureformat;
    skytop_mipmap.GR_format = textureformat;
    ground_mipmap.GR_format = textureformat;
    bytepp = format2bpp[ textureformat ];
   
    HWR_ResizeBlock( sky_width, sky_height, &sky_mipmap );

#if 1
    RGBA_t * block = (RGBA_t*) Make_Mip_Block( &sky_mipmap );  // sets GR_data in sky_mipmap

    // [WDJ] Fill sky with default.
    // This prevents holes, cracks, or other defects, from incomplete coverage during
    // the pasting of sky textures into cache block.    
    // This does not have anything to do with chromakey, it just uses that color.
    {
        int i, j;
        RGBA_t col = V_GetColor(HWR_CHROMAKEY_EQUIVALENTCOLORINDEX);
        // init sky with col so composite cannot leave any transparent holes,
        // must be 32bit
        RGBA_t * bc = (RGBA_t*) block;	
        for (j=0; j<blockheight; j++)
        {
            for (i=0; i<blockwidth; i++)
                *bc++ = col;   // endian tolerant
        }
    }
#else   
    Make_Mip_Block( &sky_mipmap );  // sets GR_data in sky_mipmap
#endif
    Z_ChangeTag (block, PU_STATIC);

    // Redraw sky_pict in RGB format.
    HWR_draw_TM_cache( TM_picture, & sky_mipmap,
                       blockwidth, blockheight, (blockwidth * bytepp),
                       sky_width, sky_height,
                       0, 0, // offset
                       sky_pict, bytepp );
   

    HWR_ResizeBlock( SKY_FLAT_WIDTH, SKY_FLAT_HEIGHT, &skytop_mipmap );
    byte * sblk = Make_Mip_Block( &skytop_mipmap );  // sets GR_data in skytop_mipmap
    Z_ChangeTag (sblk, PU_STATIC);
    HWR_draw_TM_cache( TM_column_image, & skytop_mipmap,
                       blockwidth, blockheight, (blockwidth * bytepp),
                       SKY_FLAT_WIDTH, SKY_FLAT_HEIGHT,
                       0, 0, // offset
                       & skytop_flat[0][0], bytepp );

    HWR_ResizeBlock( SKY_FLAT_WIDTH, SKY_FLAT_HEIGHT, &ground_mipmap );
    sblk = Make_Mip_Block( &ground_mipmap );  // sets GR_data in ground_mipmap
    Z_ChangeTag (sblk, PU_STATIC);
    HWR_draw_TM_cache( TM_column_image, & ground_mipmap,
                       blockwidth, blockheight, (blockwidth * bytepp),
                       SKY_FLAT_WIDTH, SKY_FLAT_HEIGHT,
                       0, 0, // offset
                       & ground_flat[0][0], bytepp );
}
