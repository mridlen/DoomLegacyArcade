// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_draw16.c 1691 2024-11-27 06:36:07Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: r_draw16.c,v $
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      16bpp (HIGHCOLOR) span/column drawer functions
//
//  NOTE: no includes because this is included as part of r_draw.c
//
// THIS IS THE VERY BEGINNING OF DOOM LEGACY's HICOLOR MODE (or TRUECOLOR)
// Doom Legacy will use HICOLOR textures, rendered through software, and
// may use MMX, or WILL use MMX to get a decent speed.
//
//-----------------------------------------------------------------------------


// ==========================================================================
// COLUMNS
// ==========================================================================

// r_data.c
  // include: color8.to16


//hicolor composite (ENABLE_DRAW15 is 5:5:5), (ENABLE_DRAW16 is 5:6:5)
// [WDJ] (c>>1 & mask_01111) or ((c & mask_11110)>>1) both
// round off the LSB, and prevent it from becoming the MSB of the adjacent color.

//  standard upto 128high posts column drawer
//
void R_DrawColumn_16 (void)
{
    int                 count;
    byte *              dest;  // within screen buffer
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl+1;

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
    // Use columnofs LUT for subwindows?
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl-centery) * fracstep;
#endif

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.

    do
    {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT.
        *(uint16_t*)dest = color8.to16[ dc_colormap[ dc_source[(frac>>FRACBITS)&127] ] ];
        dest += vid.ybytes;
        frac += fracstep;
    } while (--count);
}


//  LAME cutnpaste : same as R_DrawColumn_16 but wraps around 256
//  instead of 128 for the tall sky textures (256x240)
//
void R_DrawSkyColumn_16 (void)
{
    int                 count;
    byte *              dest;
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl+1;

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

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
        *(uint16_t*)dest = color8.to16[ dc_colormap[dc_source[(frac >> FRACBITS) & 255]] ];
        dest += vid.ybytes;
        frac += fracstep;
    } while (--count);
}


//
//
//#ifndef USEASM
void R_DrawFuzzColumn_16 (void)
{
    int                 count;
    byte*               dest;
    // Does not use the source pixels.

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
        // Lookup framebuffer, and retrieve a pixel that is either one column
        //  left or right of the current one, dim it 8%, and write it as dest.
        // 8bpp: *dest = reg_colormaps[6*256+dest[fuzzoffset[fuzzpos]]];
        register const unsigned int  alpha = 235;  // 92%
        register uint32_t dc = *(uint16_t*)(dest + fuzzoffset[fuzzpos]);
        *(uint16_t*)dest=
              ((((dc & mask_r) * alpha) >> 8) & mask_r)
            | ((((dc & mask_g) * alpha) >> 8) & mask_g)
            | ((((dc & mask_b) * alpha) >> 8) & mask_b);

        // Clamp table lookup index.
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest += vid.ybytes;
    } while (count--);
}
//#endif


void R_DrawShadeColumn_16(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;

    // [WDJ] This check has been added to all the callers of colfunc().
    // check out coords for src*
//    if ((dc_yl < 0) || (dc_x >= vid.width))
//        return;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // FIXME. As above.
    //src  = ylookup[dc_yl] + columnofs[dc_x+2];
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

    // Here we do an additional index re-mapping.
    do
    {
        // apply shading/translucent with existing showing through
        // Remap the existing dest color, dimming it through source LIGHTTABLE.
//        *dest = *(reg_colormaps + (dc_source[frac >> FRACBITS] << 8) + (*dest));
//        *dest = reg_colormaps[ LIGHTTABLE(dc_source[frac >> FRACBITS]) + (*dest) ];
        register byte sc = reg_colormaps[ LIGHTTABLE(dc_source[frac >> FRACBITS]) + 4 ]; // white
#if 1
        // 50/50 translucent
        register uint16_t dc = *(uint16_t*)dest;
        register uint16_t nc = color8.to16[sc];
        *(uint16_t*)dest=
            ((nc & mask_11110)>>1)  // 50%
            + ((dc & mask_11110)>>1);  // 50%
#endif
#if 0
        // 25/75 translucent
        register uint16_t dc = (*(uint16_t*)dest & mask_11110)>>1;
        register uint16_t nc = color8.to16[sc];
        *(uint16_t*)dest=
            ((nc & mask_11100)>>2) // 25%
            + (dc + ((dc & mask_11110)>>1)); // 75%
#endif

        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
}


#ifdef ENABLE_DRAW_ALPHA
void R_DrawAlphaColumn_16(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;
    unsigned int  alpha, alpha_r;
    uint32_t dr16_red, dr16_green, dr16_blue;

    // [WDJ] Source check has been added to all the callers of colfunc().

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

    dr16_red =   ((uint16_t)dr_color.s.red    << shift_r) & mask_r;
    dr16_green = ((uint16_t)dr_color.s.green  << shift_g) & mask_g;
    dr16_blue =  ((uint16_t)dr_color.s.blue   << shift_b) & mask_b;

  switch( dr_alpha_mode )
  {
    
   case 0: // Alpha blend
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        alpha_r = 255 - alpha;

        register uint16_t dc = *(uint16_t*)dest;
        *(uint16_t*)dest= (
              (((((dc & mask_r) * alpha_r) + (dr16_red * alpha)  ) >> 8) & mask_r)
            | (((((dc & mask_g) * alpha_r) + (dr16_green * alpha)) >> 8) & mask_g)
            | (((((dc & mask_b) * alpha_r) + (dr16_blue * alpha) ) >> 8) & mask_b) );

        dest += vid.ybytes;
        frac += fracstep;
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
        register uint16_t dc = *(uint16_t*)dest;
        *(uint16_t*)dest= (
              (((((dc & mask_r) * alpha_r) + (dr16_red * alpha)  ) >> 8) & mask_r)
            | (((((dc & mask_g) * alpha_r) + (dr16_green * alpha)) >> 8) & mask_g)
            | (((((dc & mask_b) * alpha_r) + (dr16_blue * alpha) ) >> 8) & mask_b) );

        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
    break;

   case 2: // Additive alpha
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register uint16_t dc = *(uint16_t*)dest;
        register uint32_t r = (dc & mask_r);
        register uint32_t g = (dc & mask_g);
        register uint32_t b = (dc & mask_b);
        r += ((dr_color.s.red   * alpha * (mask_r - r))) >> 16;
        g += ((dr_color.s.green * alpha * (mask_g - g))) >> 16;
        b += ((dr_color.s.blue  * alpha * (mask_b - b))) >> 16;
        *(uint16_t*)dest= (r & mask_r) | (g & mask_g) | (b & mask_b);

        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
    break;

   case 3: // Additive alpha, with background alpha
    alpha_r = dr_alpha_background;
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register uint16_t dc = *(uint16_t*)dest;
        register uint32_t r = (((dc & mask_r) * alpha_r)>>8) & mask_r;
        register uint32_t g = (((dc & mask_g) * alpha_r)>>8) & mask_g;
        register uint32_t b = (((dc & mask_b) * alpha_r)>>8) & mask_b;;
        r += ((dr_color.s.red   * alpha * (mask_r - r))) >> 16;
        g += ((dr_color.s.green * alpha * (mask_g - g))) >> 16;
        b += ((dr_color.s.blue  * alpha * (mask_b - b))) >> 16;
        *(uint16_t*)dest= (r & mask_r) | (g & mask_g) | (b & mask_b);
       
        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
    break;

   case 4: // Additive alpha, limit math
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = (dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) >> 8;
        register uint16_t dc = *(uint16_t*)dest;
        register uint32_t r = ((dr16_red   * alpha ) >> 8) + (dc & mask_r);
        if(r > mask_r)   r = mask_r;
        register uint32_t g = ((dr16_green * alpha ) >> 8) + (dc & mask_g);
        if(g > mask_g)   g = mask_g;
        register uint32_t b = ((dr16_blue  * alpha ) >> 8) + (dc & mask_b);
        if(b > mask_b)   b = mask_b;
        *(uint16_t*)dest= (r & mask_r) | (g & mask_g) | (b & mask_b);

        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
    break;
  }

}
#endif


//
//
//#ifndef USEASM
void R_DrawTranslucentColumn_16 (void)
{
    int                 count;
    byte*               dest;  // within screen buffer
    fixed_t             frac;
    fixed_t             fracstep;

    //byte*               src;

    // check out coords for src*
    if((dc_yl<0)||(dc_x>=vid.width))
      return;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0
        || dc_yh >= vid.height)
    {
        I_SoftError ( "R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // FIXME. As above.
    //src  = ylookup[dc_yl] + columnofs[dc_x+2];
    dest = ylookup[dc_yl] + columnofs[dc_x];


    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl-centery) * fracstep;
#endif

    // Here we do an additional index re-mapping.
        // Remap the existing dest color, dimming it through something?.
        // color is in 5,5,5 format
        // (c>>1 & mask_01110) ==> multiply R,G,B by 0.5 and mask lsb
        // perhaps meant: (c>>1 & mask_01111) ==> multiply R,G,B by 0.5
        // It takes an extra bit to add two values and divide by 2, so
        // need to prevent overflow into the adjacent field.
        // This divides by 2 first, and masks off the least bit.
        // An OR of selected bits, dependent upon a translucent mask, would be more stable.
        // *dest = dc_colormap[ dc_translucentmap[ (dc_source[frac >> FRACBITS] << 8) + (*dest) ]];
    switch( dc_translucent_index )
    {
     case 0:
       {
           unsigned int alpha_d = dr_alpha >> 3; // 5 bit alpha
           unsigned int alpha_r = 31 - alpha_d;
           do
           {
               // alpha translucent
               register uint16_t nc = color8.to16[dc_colormap[dc_source[frac>>FRACBITS]]];
               register uint32_t dc0 = *(uint16_t*)dest & mask_rb;  // for overlapped execution
               register uint32_t dc1 = *(uint16_t*)dest & mask_g;
               dc0 = (((dc0 * alpha_r) + ((nc & mask_rb) * alpha_d)) >> 5) & mask_rb;
               dc1 = (((dc1 * alpha_r) + ((nc & mask_g) * alpha_d)) >> 5) & mask_g;
               *(uint16_t*)dest = dc0 | dc1;
               dest += vid.ybytes;
               frac += fracstep;
           } while (count--);
        }
        break;
     case TRANSLU_more: // 20 80  puffs
        do
        {
            // 25/75 translucent
            register uint16_t dc = (*(uint16_t*)dest & mask_11110)>>1;
//	    register uint16_t nc = color8.to16[dc_source[frac>>FRACBITS]];
            register uint16_t nc = color8.to16[dc_colormap[dc_source[frac>>FRACBITS]]];
            *(uint16_t*)dest=
             ((nc & mask_11100)>>2) // 25%
             + (dc + ((dc & mask_11110)>>1)); // 75%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
     case TRANSLU_hi:   // 10 90  blur effect
        do
        {
            // 15/85 translucent
            register uint16_t dc = *(uint16_t*)dest;
            register uint16_t dc85 = dc - ((dc & mask_11000)>>3);
            register uint16_t nc = color8.to16[dc_colormap[dc_source[frac>>FRACBITS]]];
//	    register uint16_t nc = color8.to16[dc_source[frac>>FRACBITS]];
            *(uint16_t*)dest=
             ((nc & mask_11000)>>3) // 15%
             + dc85; // 85%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
     case TRANSLU_med:  // sprite 50 backg 50
     default:
        do
        {
            // 50/50 translucent
            register uint16_t dc = *(uint16_t*)dest;
//	    register uint16_t nc = color8.to16[dc_source[frac>>FRACBITS]];
            register uint16_t nc = color8.to16[dc_colormap[dc_source[frac>>FRACBITS]]];
            *(uint16_t*)dest=
             ((nc & mask_11110)>>1)  // 50%
             + ((dc & mask_11110)>>1);  // 50%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
     case TRANSLU_fire: // 50 50 but brighter for fireballs, shots..
        do
        {
            // 50/50 translucent
            // *dest = dc_colormap[ dc_translucentmap[ (dc_source[frac >> FRACBITS] << 8) + (*dest) ]];
            register uint16_t dc = *(uint16_t*)dest;
//	    register uint16_t nc = color8.to16[dc_source[frac>>FRACBITS]];
            register uint16_t nc = color8.to16[dc_colormap[dc_translucentmap[dc_source[frac>>FRACBITS]<<8]]];
            *(uint16_t*)dest=
             ((nc & mask_11110)>>1)  // 50%
             + ((dc & mask_11110)>>1);  // 50%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
     case TRANSLU_fx1:  // Linedef 288
        // 50 50 brighter some colors, else opaque for torches
#define FX1_OPTION 1
        do
        {
            // 50/50 translucent with modifications
            register int sb = dc_source[frac>>FRACBITS];
            register uint16_t nc = color8.to16[dc_colormap[sb]];
//	    register uint16_t nc = color8.to16[sb];
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
                register uint16_t dc = *(uint16_t*)dest;
                *(uint16_t*)dest=
                 ((nc & mask_11110)>>1)  // 50%
                 + ((dc & mask_11110)>>1);  // 50%
            }
            else
            {
                *(uint16_t*)dest= nc; // opaque
            }
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
     case TRANSLU_75: // 75 25
        do
        {
            // 75/25 translucent
            register uint16_t dc = *(uint16_t*)dest;
            register uint16_t nc = (color8.to16[dc_source[frac>>FRACBITS]] & mask_11110)>>1;
            *(uint16_t*)dest=
             (nc + ((nc & mask_11110)>>1)) // 75%
             + ((dc & mask_11100)>>2); // 25%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
    }
#if 0
        // original
        // [WDJ] The original is not a balanced translucent, and there are 5 different
        // translucent effects in the 8 bit code.
        *(uint16_t*)dest=
            ( ((color8.to16[dc_source[frac>>FRACBITS]]>>1) & mask_01110)
            + (*(uint16_t*)dest & mask_11110) ) /*>> 1*/ & 0x7fff;
#endif
#if 0
        // 75/25 translucent
        register uint16_t dc = *(uint16_t*)dest;
//	register uint16_t nc = (color8.to16[dc_source[frac>>FRACBITS]] & mask_11110)>>1;
        register uint16_t nc = (color8.to16[dc_colormap[dc_source[frac>>FRACBITS]]] & mask_11110)>>1;
        *(uint16_t*)dest=
            (nc + ((nc & mask_11110)>>1))  // 75%
            + ((dc & mask_11100)>>2); // 25%
#endif
#if 0
        // by 256 entry translucent table (16x16) of uint16_t
        register unsigned int nc = color8.to16[dc_source[frac>>FRACBITS]];
        register unsigned int dc = *(uint16_t*)dest;
        nc <<=3; // must be 32 bit register or will lose high bits
        dc >>=1;
        *(uint16_t*)dest= dc16_translucent[(nc&0xF0)|(dc&0x0F)];
        nc >>=5;
        dc >>=5;
        *(uint16_t*)dest|= dc16_translucent[(nc&0xF0)|(dc&0x0F)];
        nc >>=5;
        dc >>=5;
        *(uint16_t*)dest|= dc16_translucent[(nc&0xF0)|(dc&0x0F)];
#endif
#if 0
        // by 1024 entry translucent table (32x32) of uint16_t
        register unsigned int nc = color8.to16[dc_source[frac>>FRACBITS]];
        register unsigned int dc = *(uint16_t*)dest;
        nc <<=4; // must be 32 bit register or will lose high bits
        *(uint16_t*)dest = dc16_translucent[(nc&0x02E0)|(dc&0x001F)];
        nc >>=5;
        dc >>=5;
        *(uint16_t*)dest |= dc16_translucent[(nc&0x02E0)|(dc&0x001F)];
        nc >>=5;
        dc >>=5;
        *(uint16_t*)dest |= dc16_translucent[(nc&0x02E0)|(dc&0x001F)];
#endif
}
//#endif



// transparent with skin translations
// Although the vissprite has capability for any transparency,
// Called with TRANSLU_hi or TRANSLU_more, or arbitrary by thing TRANSMASK
void R_DrawTranslatedTranslucentColumn_16(void)
{
#if 1
    R_DrawTranslucentColumn_16();
#else
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count < 0)     // Zero length, column does not exceed a pixel.
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

    // Here we do an additional index re-mapping.
    switch( dc_translucent_index )
    {
     case TRANSLU_hi:   // 10 90  blur effect
        do
        {
            // Makes player visible in dark rooms, but so does draw8 version
            // *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]] << 8) + (*dest) ]];

            // 15/85 translucent
            register uint16_t dc = *(uint16_t*)dest;
            register uint16_t dc85 = dc - ((dc & mask_11000)>>3);
//	    register uint16_t nc = color8.to16[dc_skintran[dc_source[frac>>FRACBITS]]];
            register uint16_t nc = color8.to16[dc_colormap[dc_skintran[dc_source[frac>>FRACBITS]]]];
            *(uint16_t*)dest=
             ((nc & mask_11000)>>3) // 15%
             + dc85; // 85%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
     case TRANSLU_med:  // sprite 50 backg 50
     default:
        // Seems to be unused
        do
        {
            // 50/50 translucent
            register uint16_t dc = *(uint16_t*)dest;
//	    register uint16_t nc = color8.to16[dc_skintran[dc_source[frac>>FRACBITS]]];
            register uint16_t nc = color8.to16[dc_colormap[dc_skintran[dc_source[frac>>FRACBITS]]]];
            *(uint16_t*)dest=
             ((nc & mask_11110)>>1)  // 50%
             + ((dc & mask_11110)>>1);  // 50%
            dest += vid.ybytes;
            frac += fracstep;
        } while (count--);
        break;
    }
#endif
}


//
//
//#ifndef USEASM
void R_DrawTranslatedColumn_16 (void)
{
    int                 count;
    byte*               dest;  // within screen buffer
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    if ((unsigned)dc_x >= vid.width
        || dc_yl < 0
        || dc_yh >= vid.height)
    {
        I_SoftError ( "R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
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

    // Here we do an additional index re-mapping.
    do
    {
        *(uint16_t*)dest = color8.to16[ dc_colormap[ dc_skintran[ dc_source[frac>>FRACBITS]]] ];
        dest += vid.ybytes;
        frac += fracstep;
    } while (count--);
}
//#endif



// ==========================================================================
// SPANS
// ==========================================================================

//
//
//#ifndef USEASM
void R_DrawSpan_16 (void)
{
    fixed_t             xfrac;
    fixed_t             yfrac;
    byte *              dest;
    int                 count;

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

    dest = ylookup[ds_y] + columnofs[ds_x1];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
        // Current texture index in u,v.
        xfrac &= flat_imask;
        register int spot = ((yfrac>>flatfracbits)&flat_ymask) + (xfrac>>FRACBITS);
        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        *(uint16_t*)dest = color8.to16[ ds_colormap[ds_source[spot]] ];
        dest += 2;
        // Next step in u,v.
        xfrac += ds_xstep;
        yfrac += ds_ystep;
    } while (count--);
}
//#endif


void R_DrawTranslucentSpan_16(void)
{
    fixed_t xfrac, yfrac;
    fixed_t xstep, ystep;
    unsigned int alpha_d = dr_alpha;
    unsigned int alpha_r, count;
    byte * dest;

    alpha_d >>= 3; // 5 bit alpha
    alpha_r = 31 - alpha_d;

#ifdef RANGECHECK_DRAW_LIMITS
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= rdraw_viewwidth || (unsigned) ds_y > rdraw_viewheight)
    {
        I_SoftError("R_DrawSpan: %i to %i at %i\n", ds_x1, ds_x2, ds_y);
        return;
    }
#endif

    xfrac = ds_xfrac & flat_imask;
    yfrac = ds_yfrac;

    dest = ylookup[ds_y] + columnofs[ds_x1];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1 + 1;

    xstep = ds_xstep;
    ystep = ds_ystep;

    do
    {
        // Current texture index in u,v.
        // Lookup pixel from flat texture tile,
        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        register uint16_t nc = color8.to16[ ds_colormap[ds_source[((yfrac >> flatfracbits) & flat_ymask) | (xfrac >> FRACBITS)]] ];
        // alpha translucent
        register uint32_t dc0 = *(uint16_t*)dest & mask_rb;  // for overlapped execution
        register uint32_t dc1 = *(uint16_t*)dest & mask_g;
        dc0 = (((dc0 * alpha_r) + ((nc & mask_rb) * alpha_d)) >> 5) & mask_rb;
        dc1 = (((dc1 * alpha_r) + ((nc & mask_g) * alpha_d)) >> 5) & mask_g;
        *(uint16_t*)dest = dc0 | dc1;
        dest+=2;

        // Next step in u,v.
        xfrac += xstep;
        yfrac += ystep;
        xfrac &= flat_imask;
    }
    while (--count);
}


void R_DrawFogSpan_16(void)
{
    uint32_t fc = color8.to16[ ds_colormap[ ds_source[fog_index] ]];
    uint32_t fogcolor_rb, fogcolor_g;
    unsigned int alpha_d = dr_alpha >> 3;  // 5 bit alpha
    unsigned int alpha_r = (31 - alpha_d) * 0.84; // cloudy fog
    unsigned int count;
    byte * dest;

    // the working fog color for this span
    fogcolor_rb = ((fc & mask_r) | (fc & mask_b)) * alpha_d;
    fogcolor_g = (fc & mask_g) * alpha_d;

    dest = ylookup[ds_y] + columnofs[ds_x1];
    count = ds_x2 - ds_x1 + 1;

    while (count--)
    {
        // alpha translucent fog
        register uint16_t d16 = *(uint16_t*)dest;
        register uint32_t dc0 = d16 & mask_rb;  // for overlapped execution
        register uint32_t dc1 = d16 & mask_g;
        dc0 = (((dc0 * alpha_r) + fogcolor_rb) >> 5) & mask_rb;
        dc1 = (((dc1 * alpha_r) + fogcolor_g) >> 5) & mask_g;
        *(uint16_t*)dest = dc0 | dc1;
        dest+=2;
    }
}


//SoM: Fog wall.
// Used for Legacy linetype 302, walls of 3D fog in tagged
// Used for Legacy linetype 283, fog sheet
void R_DrawFogColumn_16(void)
{
   // fogcolor blur needs to be kept as 8 bit or else the blur is inadequate
static uint16_t fogcolor_r8 = 0x1000;
static uint16_t fogcolor_g8 = 0x0080;
static uint16_t fogcolor_b8 = 0x0002<<3;
    uint32_t fc, fc2, fc3;  // uint16_t + 3
    uint32_t fogcolor_rb, fogcolor_g;
    unsigned int alpha_d = dr_alpha >> 3; // 5 bit alpha
    unsigned int alpha_r = (31 - alpha_d) * 0.84; // cloudy fog
    int count, fi;
    byte * dest;

    // fog_index 0.. column height
    // always average three pixels of source texture
    fc = color8.to16[ dc_colormap[ dc_source[fog_index] ]];
    fi = fog_index + 3;
    if( fi >= fog_col_length )  fi -= fog_col_length;
    fc2 = color8.to16[ dc_colormap[ dc_source[fi] ]];
    fi += 3;
    if( fi >= fog_col_length )  fi -= fog_col_length;
    fc3 = color8.to16[ dc_colormap[ dc_source[fi] ]];
    // fogcolor has some fraction bits
    register uint32_t fr1 = ((fc & mask_r) + (fc2 & mask_r) + (fc3 & mask_r)) / 3;
    register uint32_t fg1 = ((fc & mask_g) + (fc2 & mask_g) + (fc3 & mask_g)) / 3;
    register uint32_t fb1 = (((fc & mask_b) + (fc2 & mask_b) + (fc3 & mask_b)) << 3 ) / 3;  // fraction << 3
    if( fog_init )
    {
        // init blur
        fogcolor_r8 = fr1;
        fogcolor_g8 = fg1;
        fogcolor_b8 = fb1;
        fog_init = 0;
    }
    else
    {
        // blur
        fogcolor_r8 = ((((unsigned int)fogcolor_r8)*31) + fr1) >> 5;
        fogcolor_g8 = ((((unsigned int)fogcolor_g8)*31) + fg1) >> 5;
        fogcolor_b8 = ((((unsigned int)fogcolor_b8)*31) + fb1) >> 5;
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

    // the working fog color for this column
    // fogcolor_x8 carries fractional colors, so need mask
    fogcolor_rb = ((fogcolor_r8 & mask_r) | ((fogcolor_b8>>3) & mask_b)) * alpha_d;
    fogcolor_g = (fogcolor_g8 & mask_g) * alpha_d;

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    dest = ylookup[dc_yl] + columnofs[dc_x];

    do
    {
        register uint16_t d16 = *(uint16_t*)dest;
        register uint32_t dc0 = d16 & mask_rb;  // for overlapped execution
        register uint32_t dc1 = d16 & mask_g;
        dc0 = (((dc0 * alpha_r) + fogcolor_rb) >> 5) & mask_rb;
        dc1 = (((dc1 * alpha_r) + fogcolor_g) >> 5) & mask_g;
        *(uint16_t*)dest = dc0 | dc1;
        dest += vid.ybytes;
    }
    while (count--);
}

