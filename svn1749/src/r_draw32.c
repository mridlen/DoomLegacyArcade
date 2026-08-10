// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_draw32.c 902 2012-03-02 06:00:00Z wesleyjohnson $
//
// Copyright (C) 2012-2012 by DooM Legacy Team.
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
// DESCRIPTION:
//      32bpp span/column drawer functions
//
//  NOTE: no includes because this is included as part of r_draw.c
//
//-----------------------------------------------------------------------------


// ==========================================================================
// COLUMNS
// ==========================================================================

// r_data.c
  // include: color8.to32


// 32bpp composite (ENABLE_DRAW24) is 0:8:8:8
// color_8.to32 color is aligned with pix32

//  standard upto 128high posts column drawer
//
void R_DrawColumn_32 (void)
{
    unsigned int heightmask = dc_texheight - 1;
    int     count;
    byte *  dest;  // within screen buffer
    fixed_t frac;
    fixed_t fracstep;
    pixelunion32_t c32;


    count = dc_yh - dc_yl + 1;

    // Zero length, column does not exceed a pixel.
    if (count <= 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0
        || dc_yh >= vid.height)
    {
        I_SoftError ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // Screen buffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling, which is the only mapping to be done.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl-centery) * fracstep;
#endif

    if( dc_texheight & heightmask )
    {
        // Odd size texture, use texheight
        fixed_t texheight = dc_texheight << FRACBITS;
        // From Boom, to fix the odd frac
        if (frac < 0)
            while ((frac += texheight) < 0);
        else
            while (frac >= texheight)  frac -= texheight;
        // Inner loop that does the actual texture mapping.
        do
        {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            c32.ui32 = color8.to32[ dc_colormap[ dc_source[(frac>>FRACBITS)] ] ];
            *(pixel32_t*)dest = c32.pix32;
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (--count);
    }
    else
    {
        // Texture size power of 2, use heightmask
        // Inner loop that does the actual texture mapping.
        do
        {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            c32.ui32 = color8.to32[ dc_colormap[ dc_source[(frac>>FRACBITS)&heightmask] ] ];
            *(pixel32_t*)dest = c32.pix32;
            dest += vid.ybytes;
            frac += fracstep;
        } while (--count);
    }
}


//  Same as R_DrawColumn_32 but wraps around 256
//  instead of 128 for the tall sky textures (256x240)
//
void R_DrawSkyColumn_32 (void)
{
    int     count;
    byte *  dest;  // within screen buffer
    fixed_t frac;
    fixed_t fracstep;
    pixelunion32_t c32;

    count = dc_yh - dc_yl + 1;

    // Zero length, column does not exceed a pixel.
    if (count <= 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0
        || dc_yh >= vid.height)
    {
        I_SoftError ("R_DrawSkyColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
        c32.ui32 = color8.to32[ dc_colormap[ dc_source[(frac>>FRACBITS)&255] ] ];
        *(pixel32_t*)dest = c32.pix32;
        dest += vid.ybytes;
        frac += fracstep;
    } while (--count);
}


void R_DrawFuzzColumn_32 (void)
{
    int     count;
    byte*   dest;

    // Adjust borders. Low...
    if (!dc_yl)
        dc_yl = 1;

    // .. and high.
    if (dc_yh == rdraw_viewheight-1)
        dc_yh = rdraw_viewheight - 2;

    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
        return;


#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0 || dc_yh >= vid.height)
    {
        I_SoftError ("R_DrawFuzzColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // Does not work with blocky mode.
    dest = ylookup[dc_yl] + columnofs[dc_x];

    do
    {
        // Retrieve a pixel that is either one column left or right
        // of the dest, dim it 8%, and write it as dest.
        // 8bpp: *dest = reg_colormaps[6*256+dest[fuzzoffset[fuzzpos]]];
        register pixel32_t * src = (pixel32_t*)(dest + fuzzoffset[fuzzpos]);
        ((pixel32_t*)dest)->b = src->b - (src->b >> 3);
        ((pixel32_t*)dest)->g = src->g - (src->g >> 3);
        ((pixel32_t*)dest)->r = src->r - (src->r >> 3);

        dest += vid.ybytes;
        // Clamp table lookup index.
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
    } while (count--);
}


// To draw FF_SMOKESHADE things, sprite is an alpha channel
void R_DrawShadeColumn_32(void)
{
    fixed_t texheight = dc_texheight << FRACBITS;  // any texture size
    int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;
    unsigned int  alpha;

    // [WDJ] Source check has been added to all the callers of colfunc().

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawShadeColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // FIXME. As above.
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif
    if (texheight > 0)  // hangs when texheight==0
    {
        // From Boom, to fix the odd frac
        if (frac < 0)
            while ((frac += texheight) < 0);
        else
            while (frac >= texheight)  frac -= texheight;
    }

    do
    {
        // 8bpp: *dest = reg_colormaps[ LIGHTTABLE(dc_source[frac >> FRACBITS]) + (*dest) ];
        // [WDJ] The source is a patch of alpha values, 0..255.
        // light level values are 0..NUMCOLORMAPS-1, which is 0..32
//        alpha = dc_source[frac >> FRACBITS] >> 3;  // reduce 0..255 to 0..32
        alpha = dc_source[frac >> FRACBITS];
        register pixel32_t * p32 = (pixel32_t*)dest;
        p32->b = (p32->b * alpha) >> 5;
        p32->g = (p32->g * alpha) >> 5;
        p32->r = (p32->r * alpha) >> 5;

        dest += vid.ybytes;
        frac += fracstep;
        if( frac >= texheight )
            frac -= texheight;
    }
    while (count--);
}


#ifdef ENABLE_DRAW_ALPHA
void R_DrawAlphaColumn_32(void)
{
#if 0
    fixed_t texheight = dc_texheight << FRACBITS;  // any texture size
#endif
    int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;
    unsigned int  alpha, alpha_r;

    // [WDJ] Source check has been added to all the callers of colfunc().

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawAlphaColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    dest = ylookup[dc_yl] + columnofs[dc_x];
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

#if 0
    // [WDJ] Only draws sprite for now.
    if (texheight > 0)  // hangs when texheight==0
    {
        // From Boom, to fix the odd frac
        if (frac < 0)
            while ((frac += texheight) < 0);
        else
            while (frac >= texheight)  frac -= texheight;
    }
#endif

  switch( dr_alpha_mode )
  {
    
   case 0: // Alpha blend
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        alpha_r = 255 - alpha;
        register pixel32_t p32 = *(pixel32_t*)dest;
        p32.b = (((unsigned int)p32.b * alpha_r) + ((unsigned int)dr_color.s.blue * alpha))  >> 8;
        p32.g = (((unsigned int)p32.g * alpha_r) + ((unsigned int)dr_color.s.green * alpha)) >> 8;
        p32.r = (((unsigned int)p32.r * alpha_r) + ((unsigned int)dr_color.s.red * alpha))   >> 8;
        *(pixel32_t*)dest = p32;

        dest += vid.ybytes;
        frac += fracstep;
#if 0
        if( frac >= texheight )
            frac -= texheight;  // wrap texture
#endif
    }
    while (count--);
    break;

   case 1: // Alpha blend with constant background alpha.
    // Caller must set dr_alpha_background at (255 - dr_alpha) or less.
    alpha_r = dr_alpha_background;
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register pixel32_t p32 = *(pixel32_t*)dest;
        p32.b = (((unsigned int)p32.b * alpha_r) + ((unsigned int)dr_color.s.blue * alpha))  >> 8;
        p32.g = (((unsigned int)p32.g * alpha_r) + ((unsigned int)dr_color.s.green * alpha)) >> 8;
        p32.r = (((unsigned int)p32.r * alpha_r) + ((unsigned int)dr_color.s.red * alpha))   >> 8;
        *(pixel32_t*)dest = p32;

        dest += vid.ybytes;
        frac += fracstep;
#if 0
        if( frac >= texheight )
            frac -= texheight;
#endif
    }
    while (count--);
    break;

   case 2: // Additive alpha
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register pixel32_t p32 = *(pixel32_t*)dest;
        p32.b += (((uint32_t)dr_color.s.blue  * alpha * (255 - p32.b))) >> 16;
        p32.g += (((uint32_t)dr_color.s.green * alpha * (255 - p32.g))) >> 16;
        p32.r += (((uint32_t)dr_color.s.red   * alpha * (255 - p32.r))) >> 16;
        *(pixel32_t*)dest = p32;

        dest += vid.ybytes;
        frac += fracstep;
#if 0
        if( frac >= texheight )
            frac -= texheight;
#endif
    }
    while (count--);
    break;

   case 3: // Additive alpha, with background alpha
    alpha_r = dr_alpha_background;
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register pixel32_t p32 = *(pixel32_t*)dest;
        register unsigned int r,g,b;
        b = (p32.b * alpha_r)>>8;
        p32.b = (((uint32_t)dr_color.s.blue  * alpha * (255 - b)) >> 16) + b;
        g = (p32.g * alpha_r)>>8;
        p32.g = (((uint32_t)dr_color.s.green * alpha * (255 - g)) >> 16) + g;
        r = (p32.r * alpha_r)>>8;
        p32.r = (((uint32_t)dr_color.s.red   * alpha * (255 - r)) >> 16) + r;
        *(pixel32_t*)dest = p32;

        dest += vid.ybytes;
        frac += fracstep;
#if 0
        if( frac >= texheight )
            frac -= texheight;
#endif
    }
    while (count--);
    break;

   case 4: // Additive alpha, limit math
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register pixel32_t p32 = *(pixel32_t*)dest;
        register unsigned int r,g,b;
        b = (((unsigned int)dr_color.s.blue * alpha) >> 8) + p32.b;
        p32.b = (b > 255)? 255 : b;
        g = (((unsigned int)dr_color.s.green * alpha) >> 8) + p32.g;
        p32.g = (g > 255)? 255 : g;
        r = (((unsigned int)dr_color.s.red * alpha) >> 8) + p32.r;
        p32.r = (r > 255)? 255 : r;
        *(pixel32_t*)dest = p32;

        dest += vid.ybytes;
        frac += fracstep;
#if 0
        if( frac >= texheight )
            frac -= texheight;
#endif
    }
    while (count--);
    break;
  }

}
#endif


void R_DrawTranslucentColumn_32 (void)
{
    fixed_t texheight = dc_texheight << FRACBITS;  // any texture size
    int     count;
    byte*   dest;  // within screen buffer
    fixed_t frac;
    fixed_t fracstep;
    pixelunion32_t c32;

    // [WDJ] Source check has been added to all the callers of colfunc().

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0
        || dc_yh >= vid.height)
    {
        I_SoftError ( "R_DrawTranslucentColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl-centery) * fracstep;
#endif
    if (texheight > 0)  // hangs when texheight==0
    {
        // From Boom, to fix the odd frac
        if (frac < 0)
            while ((frac += texheight) < 0);
        else
            while (frac >= texheight)  frac -= texheight;
    }
    if( fracstep < 0 )
       I_Error( "DrawTranslucent: fracstep < 0\n");

    // 8bpp: *dest = dc_colormap[ dc_translucentmap[ (dc_source[frac >> FRACBITS] << 8) + (*dest) ]];
    switch( dc_translucent_index )
    {
     case 0:
        {
            // alpha translucent, by dr_alpha
            unsigned int alpha_d = dr_alpha;
            unsigned int alpha_r = 255 - alpha_d;
            do
            {
                c32.ui32 = color8.to32[dc_colormap[dc_source[frac>>FRACBITS]]];
//               c32.ui32 = color8.to32[dc_source[frac>>FRACBITS]];
                // alpha translucent
                register pixel32_t * p32 = (pixel32_t*)dest;
                p32->b = (((unsigned int)p32->b * alpha_r) + (c32.pix32.b * alpha_d)) >> 8;
                p32->g = (((unsigned int)p32->g * alpha_r) + (c32.pix32.g * alpha_d)) >> 8;
                p32->r = (((unsigned int)p32->r * alpha_r) + (c32.pix32.r * alpha_d)) >> 8;
                dest += vid.ybytes;
                frac += fracstep;
                if( frac >= texheight )
                    frac -= texheight;
            } while (count--);
        }
        break;
     case TRANSLU_more: // 20 80  puffs, Linedef 285
        do
        {
            // 25/75 translucent
            c32.ui32 = color8.to32[dc_colormap[dc_source[frac>>FRACBITS]]];
//	    c32.ui32 = color8.to32[dc_source[frac>>FRACBITS]];
            register uint16_t dc0, dc1, dc2; // for overlapped execution
            register pixel32_t * p32 = (pixel32_t*)dest;
            dc0 = p32->b;
            p32->b = (dc0 + dc0 + dc0 + c32.pix32.b) >> 2;
            dc1 = p32->g;
            p32->g = (dc1 + dc1 + dc1 + c32.pix32.g) >> 2;
            dc2 = p32->r;
            p32->r = (dc2 + dc2 + dc2 + c32.pix32.r) >> 2;
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (count--);
        break;
     case TRANSLU_hi:   // 10 90  blur effect, Linedef 286
        do
        {
            // 15/85 translucent
            c32.ui32 = color8.to32[dc_colormap[dc_source[frac>>FRACBITS]]];
//	    c32.ui32 = color8.to32[dc_source[frac>>FRACBITS]];
            register uint16_t dc0, dc1, dc2; // for overlapped execution
            register pixel32_t * p32 = (pixel32_t*)dest;
            dc0 = p32->b;
            p32->b = ((dc0<<3) - dc0 + c32.pix32.b) >> 3;
            dc1 = p32->g;
            p32->g = ((dc1<<3) - dc1 + c32.pix32.g) >> 3;
            dc2 = p32->r;
            p32->r = ((dc2<<3) - dc2 + c32.pix32.r) >> 3;
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (count--);
        break;
     case TRANSLU_med:  // sprite 50 backg 50, Linedef 260, 284, 300
     default:
        do
        {
            // 50/50 translucent
            c32.ui32 = color8.to32[dc_colormap[dc_source[frac>>FRACBITS]]];
//	    c32.ui32 = color8.to32[dc_source[frac>>FRACBITS]];
            register uint16_t dc0, dc1, dc2; // for overlapped execution
            register pixel32_t * p32 = (pixel32_t*)dest;
            dc0 = p32->b;
            p32->b = (dc0 + c32.pix32.b) >> 1;
            dc1 = p32->g;
            p32->g = (dc1 + c32.pix32.g) >> 1;
            dc2 = p32->r;
            p32->r = (dc2 + c32.pix32.r) >> 1;
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (count--);
        break;
     case TRANSLU_fire: // Linedef 287
        // 50 50 but brighter for fireballs, shots
#define TFIRE_OPTION  1
        do
        {
            // 50/50 translucent with modifications
            register int sb = dc_source[frac>>FRACBITS];
#if TFIRE_OPTION==4
//	    sb = dc_translucentmap[ (sb << 8) + 80 ]; // grays 80..111; // faded
//	    sb = dc_translucentmap[ (sb << 8) + 110 ]; // grays 80..111;
            sb = dc_translucentmap[ (sb << 8) ]; // black
#endif
            c32.ui32 = color8.to32[dc_colormap[ sb ]];
//	    c32.ui32 = color8.to32[ sb ];
#if TFIRE_OPTION==1
            // dark background is enhanced before avg, to give light add effect
            register pixelunion32_t sh32;
            sh32.ui32 = (c32.ui32 >> 1) & 0x70707070;  // tint
//	    sh32.ui32 = (c32.ui32 >> 1) & 0x7F7F7F7F;  // heavy
            register uint16_t dc0, dc1, dc2; // for overlapped execution
            register pixel32_t * p32 = (pixel32_t*)dest;
            dc0 = p32->b;
            p32->b = ((dc0 | sh32.pix32.b) + c32.pix32.b) >> 1;
            dc1 = p32->g;
            p32->g = ((dc1 | sh32.pix32.g) + c32.pix32.g) >> 1;
            dc2 = p32->r;
            p32->r = ((dc2 | sh32.pix32.r) + c32.pix32.r) >> 1;
#endif
#if TFIRE_OPTION==2
            // Bright and nearly opaque
            // additive light (not average)
            register uint16_t dc0, dc1, dc2; // for overlapped execution
            register pixel32_t * p32 = (pixel32_t*)dest;
            dc0 = p32->b;
            p32->b = (dc0 | c32.pix32.b);
            dc1 = p32->g;
            p32->g = (dc1 | c32.pix32.g);
            dc2 = p32->r;
            p32->r = (dc2 | c32.pix32.r);
#endif
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (count--);
        break;
     case TRANSLU_fx1:  // Linedef 288
        // 50 50 brighter some colors, else opaque for torches
        // fire translucent texture
#define FX1_OPTION 1
        do
        {
            // 50/50 translucent with modifications
            register int sb = dc_source[frac>>FRACBITS];
            c32.ui32 = color8.to32[dc_colormap[ sb ]];
//	    c32.ui32 = color8.to32[ sb ];
#if FX1_OPTION==1
            // check translucent map for opaque, sprite vrs white bkg
            register byte twht = dc_translucentmap[ (sb << 8) + 4 ];
            register int  tdiff = sb - twht;  // any change
#else
            // check translucent map for opaque, blk bkg vrs white bkg
            register byte tblk = dc_translucentmap[ (sb << 8) ];
            register byte twht = dc_translucentmap[ (sb << 8) + 4 ];
            register int  tdiff = tblk - twht;
#endif

            if( tdiff > 2 || tdiff < -2 )
            {
                // 50/50
                register uint16_t dc0, dc1, dc2;
                register pixel32_t * p32 = (pixel32_t*)dest;
                dc0 = p32->b;
                p32->b = (dc0 + c32.pix32.b) >> 1;
                dc1 = p32->g;
                p32->g = (dc1 + c32.pix32.g) >> 1;
                dc2 = p32->r;
                p32->r = (dc2 + c32.pix32.r) >> 1;
            }
            else
            {
                *(pixel32_t*)dest = c32.pix32;  // opaque
            }
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (count--);
        break;
     case TRANSLU_75: // 75 25
        do
        {
            // 75/25 translucent
            c32.ui32 = color8.to32[dc_colormap[dc_source[frac>>FRACBITS]]];
            register uint16_t dc0, dc1, dc2; // for overlapped execution
            register pixel32_t * p32 = (pixel32_t*)dest;
            dc0 = c32.pix32.b;
            p32->b = (dc0 + dc0 + dc0 + p32->b) >> 2;
            dc1 = c32.pix32.g;
            p32->g = (dc1 + dc1 + dc1 + p32->g) >> 2;
            dc2 = c32.pix32.r;
            p32->r = (dc2 + dc2 + dc2 + p32->r) >> 2;
            dest += vid.ybytes;
            frac += fracstep;
            if( frac >= texheight )
                frac -= texheight;
        } while (count--);
        break;
    }
}


// transparent with skin translations
// Although the vissprite has capability for any transparency,
// Called with TRANSLU_hi or TRANSLU_more, or arbitrary by thing TRANSMASK
void R_DrawTranslatedTranslucentColumn_32(void)
{
    R_DrawTranslucentColumn_32();
}


// Skin
void R_DrawTranslatedColumn_32 (void)
{
    fixed_t texheight = dc_texheight << FRACBITS;  // any texture size
    int     count;
    byte*   dest;  // within screen buffer
    fixed_t frac;
    fixed_t fracstep;
    pixelunion32_t c32;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0
        || dc_yh >= vid.height)
    {
        I_SoftError ( "R_DrawTranslatedColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl-centery) * fracstep;
#endif
    if (texheight > 0)  // hangs when texheight==0
    {
        // From Boom, to fix the odd frac
        if (frac < 0)
            while ((frac += texheight) < 0);
        else
            while (frac >= texheight)  frac -= texheight;
    }

    // Here we do an additional skin re-mapping.
    do
    {
        c32.ui32 = color8.to32[ dc_colormap[ dc_skintran[ dc_source[frac>>FRACBITS]]] ];
        *(pixel32_t*)dest = c32.pix32;  // opaque
        dest += vid.ybytes;
        frac += fracstep;
        if( frac >= texheight )
            frac -= texheight;
    } while (count--);
}



// ==========================================================================
// SPANS
// ==========================================================================

void R_DrawSpan_32 (void)
{
    fixed_t xfrac, yfrac;
    pixel32_t *  p32;
    int     count;
    pixelunion32_t c32;

#ifdef RANGECHECK_DRAW_LIMITS
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2>=vid.width
        || (unsigned)ds_y>vid.height)
    {
        I_SoftError( "R_DrawSpan: %i to %i at %i", ds_x1,ds_x2,ds_y);
        return;
    }
#endif

    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    p32 = (pixel32_t*)( ylookup[ds_y] + columnofs[ds_x1] );

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
        // Current texture index in u,v.
        xfrac &= flat_imask;
        register int spot = ((yfrac >> flatfracbits) & flat_ymask) | (xfrac >> FRACBITS);
        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        c32.ui32 = color8.to32[ ds_colormap[ds_source[spot]] ];
        *(p32++) = c32.pix32;  // opaque
        // Next step in u,v.
        xfrac += ds_xstep;
        yfrac += ds_ystep;
    } while (count--);
}


void R_DrawTranslucentSpan_32(void)
{
    fixed_t xfrac, yfrac;
    fixed_t xstep, ystep;
    pixel32_t *  p32;
    unsigned int alpha_d = dr_alpha;
    unsigned int alpha_r = 255 - alpha_d;
    int count;
    pixelunion32_t c32;

#ifdef RANGECHECK_DRAW_LIMITS
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= rdraw_viewwidth || (unsigned) ds_y > rdraw_viewheight)
    {
        I_SoftError("R_DrawTranslucentSpan: %i to %i at %i\n", ds_x1, ds_x2, ds_y);
        return;
    }
#endif

    xfrac = ds_xfrac & flat_imask;
    yfrac = ds_yfrac;

    p32 = (pixel32_t*)( ylookup[ds_y] + columnofs[ds_x1] );

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1 + 1;

    xstep = ds_xstep;
    ystep = ds_ystep;

    do
    {
        // Current texture index in u,v.
        // Lookup pixel from flat texture tile,
        register int spot = ((yfrac >> flatfracbits) & flat_ymask) | (xfrac >> FRACBITS);
        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        c32.ui32 = color8.to32[ ds_colormap[ds_source[spot]] ];
        // alpha translucent
        p32->b = (((unsigned int)p32->b * alpha_r) + (c32.pix32.b * alpha_d)) >> 8;
        p32->g = (((unsigned int)p32->g * alpha_r) + (c32.pix32.g * alpha_d)) >> 8;
        p32->r = (((unsigned int)p32->r * alpha_r) + (c32.pix32.r * alpha_d)) >> 8;
        p32 ++;  // *4

        // Next step in u,v.
        xfrac += xstep;
        yfrac += ystep;
        xfrac &= flat_imask;
    }
    while (--count);
}


// Used for Legacy linetype 302, ceiling and floor of 3D fog in tagged
void R_DrawFogSpan_32(void)
{
    unsigned int count;
    unsigned int alpha_d = dr_alpha;
    unsigned int alpha_r = (255 - alpha_d) * 0.84;  // cloudy fog
    pixelunion32_t fogcolor;
    pixel32_t *  p32;

    fogcolor.ui32 = color8.to32[ ds_colormap[ ds_source[fog_index] ]];

    count = ds_x2 - ds_x1 + 1;
    p32 = (pixel32_t*)( ylookup[ds_y] + columnofs[ds_x1] );

    {
        register unsigned int fb = fogcolor.pix32.b * alpha_d;
        register unsigned int fg = fogcolor.pix32.g * alpha_d;
        register unsigned int fr = fogcolor.pix32.r * alpha_d;
        while (count--)
        {
            p32->b = ((p32->b * alpha_r) + fb) >> 8;
            p32->g = ((p32->g * alpha_r) + fg) >> 8;
            p32->r = ((p32->r * alpha_r) + fr) >> 8;
            p32 ++;  // *4
        }
    }
}

//SoM: Fog wall.
// Used for Legacy linetype 302, walls of 3D fog in tagged
// Used for Legacy linetype 283, fog sheet
void R_DrawFogColumn_32(void)
{
static pixelunion32_t fogcolor = {.ui32=0x00101010};
    pixelunion32_t fc, fc2, fc3;
    unsigned int alpha_d = dr_alpha;
    unsigned int alpha_r = (255 - alpha_d) * 0.84;  // cloudy fog
    int fi, count;
    byte * dest;

    // fog_index 0.. column height
    // always average three pixels of source texture
    fc.ui32 = color8.to32[ dc_colormap[ dc_source[fog_index] ]];
    fi = fog_index + 3;
    if( fi >= fog_col_length )  fi = 0;
    fc2.ui32 = color8.to32[ dc_colormap[ dc_source[fi] ]];
    fi += 3;
    if( fi >= fog_col_length )  fi = 0;
    fc3.ui32 = color8.to32[ dc_colormap[ dc_source[fi] ]];
    fc.pix32.r = ((unsigned int)fc.pix32.r + fc2.pix32.r + fc3.pix32.r) /3 ;
    fc.pix32.g = ((unsigned int)fc.pix32.g + fc2.pix32.g + fc3.pix32.g) /3;
    fc.pix32.b = ((unsigned int)fc.pix32.b + fc2.pix32.b + fc3.pix32.b) /3;
    if( fog_init )
    {
        // init blur
        fogcolor.pix32 = fc.pix32;
        fog_init = 0;
    }
    else
    {
        // blur 31 and 1 = 3.125%
        fogcolor.pix32.r = ((((unsigned int)fogcolor.pix32.r)*31) + fc.pix32.r) >> 5;
        fogcolor.pix32.g = ((((unsigned int)fogcolor.pix32.g)*31) + fc.pix32.g) >> 5;
        fogcolor.pix32.b = ((((unsigned int)fogcolor.pix32.b)*31) + fc.pix32.b) >> 5;
    }

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawFogColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    dest = ylookup[dc_yl] + columnofs[dc_x];

    {
        register unsigned int fb = fogcolor.pix32.b * alpha_d;
        register unsigned int fg = fogcolor.pix32.g * alpha_d;
        register unsigned int fr = fogcolor.pix32.r * alpha_d;
        do
        {
            register pixel32_t * p32 = (pixel32_t*)dest;
            p32->b = ((p32->b * alpha_r) + fb) >> 8;
            p32->g = ((p32->g * alpha_r) + fg) >> 8;
            p32->r = ((p32->r * alpha_r) + fr) >> 8;
            dest += vid.ybytes;
        }
        while (count--);
    }
}

