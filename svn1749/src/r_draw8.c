// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_draw8.c 1691 2024-11-27 06:36:07Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Portions Copyright (C) 2000-2016 by DooM Legacy Team.
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
// $Log: r_draw8.c,v $
// Revision 1.25  2003/07/13 18:04:57  hurdler
// Revision 1.24  2003/07/13 13:16:15  hurdler
//
// Revision 1.23  2003/06/10 23:36:09  ssntails
// Variable flat support (32x32 to 2048x2048)
//
// Revision 1.22  2002/11/12 00:06:05  ssntails
// Support for translated translucent columns in software mode.
//
// Revision 1.21  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.20  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.19  2001/04/02 18:54:32  bpereira
// Revision 1.18  2001/04/01 17:35:07  bpereira
//
// Revision 1.17  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.16  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.15  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.14  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.13  2000/11/06 20:52:16  bpereira
//
// Revision 1.12  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.11  2000/09/28 20:57:17  bpereira
// Revision 1.10  2000/04/30 10:30:10  bpereira
// Revision 1.9  2000/04/24 20:24:38  bpereira
// Revision 1.8  2000/04/18 17:39:39  stroggonmeth
// Revision 1.7  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.6  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.5  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
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
//      8bpp span/column drawer functions
//
//  NOTE: no includes because this is included as part of r_draw.c
//
//-----------------------------------------------------------------------------

// [WDJ] Window limit checks have been added to all the callers of colfunc().

// [WDJ] Combine the Boom enhancements into our draw functions,
// so modifications do not have to be made in two functions.
#define USEBOOMFUNC


// ==========================================================================
// COLUMNS
// ==========================================================================

//  A column is a vertical slice/span of a wall texture that uses
//  a has a constant z depth from top to bottom.
//

#ifndef USEASM
void R_DrawColumn_8(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;

    count = dc_yh - dc_yl + 1;
    if (count <= 0)
        return;  // Zero length, column does not exceed a pixel.

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.
#ifdef USEBOOMFUNC
    // Do two pixels per loop when possible.
    {
        register const byte *source = dc_source;
        register const lighttable_t *colormap = dc_colormap;
        register int heightmask = dc_texheight - 1;
        if (dc_texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) < 0);
            else
                while (frac >= heightmask)
                    frac -= heightmask;

            do
            {
                // Re-map color indices from wall texture column
                //  using a lighting/special effects LUT.
                // heightmask is the Tutti-Frutti fix -- killough

                *dest = colormap[source[frac >> FRACBITS]];
                dest += vid.ybytes;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
            }
            while (--count);
        }
        else
        {
            while ((count -= 2) >= 0)   // texture height is a power of 2 -- killough
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += vid.ybytes;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += vid.ybytes;
                frac += fracstep;
            }
            if (count & 1)
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
        }
    }
#else
    // Simple loop.  Fixed tile 128.
    do
    {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT.
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]];

        dest += vid.ybytes;
        frac += fracstep;

    }
    while (--count);
#endif
}
#endif


#ifndef USEASM
void R_DrawSkyColumn_8(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;

#ifdef USEBOOMFUNC
    count = dc_yh - dc_yl + 1;
    if (count <= 0)
        return;  // Zero length, column does not exceed a pixel.
    // while(--count)
#else   
    count = dc_yh - dc_yl;
    if (count < 0)
        return;  // Zero length, column does not exceed a pixel.
    // while(count--)
#endif

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawSkyColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.

    // [WDJ] FIXME: heightmask=255 is wrong, height is 128 or 240.  Looking up, this could segfault.
#ifdef USEBOOMFUNC
    // Do two pixels per loop when possible.
    {
        register const byte *source = dc_source;
        register const lighttable_t *colormap = dc_colormap;
        register int heightmask = 255;
        if (dc_texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) < 0);
            else
                while (frac >= heightmask)
                    frac -= heightmask;

            do
            {
                // Re-map color indices from wall texture column
                //  using a lighting/special effects LUT.
                // heightmask is the Tutti-Frutti fix -- killough

                *dest = colormap[source[frac >> FRACBITS]];
                dest += vid.ybytes;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
            }
            while (--count);
        }
        else
        {
            while ((count -= 2) >= 0)   // texture height is a power of 2 -- killough
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += vid.ybytes;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += vid.ybytes;
                frac += fracstep;
            }
            if (count & 1)
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
        }
    }
#else
    // Simple loop.  Fixed tile 256.
    do
    {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT.
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 255]];

        dest += vid.ybytes;
        frac += fracstep;

    }
    while (count--);
#endif
}
#endif

//  The standard Doom 'fuzzy' (blur, shadow) effect
//  originally used for spectres and when picking up the blur sphere
//
//#ifndef USEASM // NOT IN ASSEMBLER, TO DO.. IF WORTH IT
void R_DrawFuzzColumn_8(void)
{
    register int count;
    register byte *dest;
    // Does not use the source pixels.

    // Adjust borders. Low...
    if (dc_yl <= 0)
        dc_yl = 1;

    // .. and high.
    if (dc_yh >= rdraw_viewheight - 1)
        dc_yh = rdraw_viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;  // Zero length, column does not exceed a pixel.
    // while(count--)

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawFuzzColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // Does not work with blocky mode.
    dest = ylookup[dc_yl] + columnofs[dc_x];

    do
    {
        // Lookup framebuffer, and retrieve a pixel that is either one column
        //  left or right of the current one.
        // Add index from colormap to index.
        // Remap existing dest, modify position, dim through LIGHTTABLE[6].
        *dest = reg_colormaps[LIGHTTABLE(6) + dest[fuzzoffset[fuzzpos]]];

        // Clamp table lookup index.
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest += vid.ybytes;
    }
    while (count--);
}

//#endif

#ifndef USEASM
// used in tiltview, but never called for now, but who know...
void R_DrawSpanNoWrap(void)
{
}
#endif

#ifndef USEASM
void R_DrawShadeColumn_8(void)
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
        I_SoftError("R_DrawShadeColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
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
        *dest = reg_colormaps[ LIGHTTABLE(dc_source[frac >> FRACBITS]) + (*dest) ];

        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
}
#endif

#ifdef ENABLE_DRAW_ALPHA
# ifndef ENABLE_DRAW8_USING_12
// switch at TRANSLU_REV_ALPHA
byte  dr8_src_shift[16]  = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0 };
byte  dr8_dest_shift[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8 };
# endif

void R_DrawAlphaColumn_8(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;
    register unsigned int  alpha;
#ifdef ENABLE_DRAW8_USING_12
    register unsigned int  alpha_r;
#endif

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

    // Looks familiar.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

    // Colormap the dr_color at the source.
   
#ifdef ENABLE_DRAW8_USING_12
  switch( dr_alpha_mode )
  {
   case 0: // Alpha blend
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = ((dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) + 0x0080) >> 8;  // alpha 0..255
        if( alpha > 14 )
        {
            unsigned int alpha_r = 255 - alpha;

            // Calculate transparency using RGBA
            register RGBA_t p = pLocalPalette[*dest];
            register unsigned int p12 = // 12 bit r,g,b color
              (((((unsigned int)p.s.blue * alpha_r) + ((unsigned int)dr_color.s.blue * alpha)   + 0x0080) >> (8+4-0)) & 0x000F)
            | (((((unsigned int)p.s.green * alpha_r) + ((unsigned int)dr_color.s.green * alpha) + 0x0080) >> (8+4-4)) & 0x00F0)
            | (((((unsigned int)p.s.red * alpha_r) + ((unsigned int)dr_color.s.red * alpha)     + 0x0080) >> (8+4-8)) & 0x0F00);
           *dest = color12_to_8[p12];  // lookup palette
        }
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
        alpha = ((dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) + 0x0080) >> 8;  // alpha 0..255
        if( alpha > 14 )
        {
            // Calculate transparency using RGBA
            register RGBA_t p = pLocalPalette[*dest];
            register unsigned int p12 = // 12 bit r,g,b color
              (((((unsigned int)p.s.blue * alpha_r) + ((unsigned int)dr_color.s.blue * alpha)   + 0x0080) >> (8+4-0)) & 0x000F)
            | (((((unsigned int)p.s.green * alpha_r) + ((unsigned int)dr_color.s.green * alpha) + 0x0080) >> (8+4-4)) & 0x00F0)
            | (((((unsigned int)p.s.red * alpha_r) + ((unsigned int)dr_color.s.red * alpha)     + 0x0080) >> (8+4-8)) & 0x0F00);
           *dest = color12_to_8[p12];  // lookup palette
        }
        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
    break;

   case 2: // Additive alpha
    do
    {
        // [WDJ] The source is a patch of alpha values, 0..255.
        alpha = ((dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) + 0x0080) >> 8;  // alpha 0..255
        if( alpha > 14 )
        {
            // Calculate transparency using RGBA
            register RGBA_t p = pLocalPalette[*dest];
            unsigned int r,g,b;
            // (0xFF * 0xFF * 0xFF) = 0xFD02FF
            b = p.s.blue;
            b += ((((unsigned int)dr_color.s.blue  * alpha * (255 - b)) + 0x8000) >> 16);
            g = p.s.green;
            g += ((((unsigned int)dr_color.s.green * alpha * (255 - g)) + 0x8000) >> 16);
            r = p.s.red;
            r += ((((unsigned int)dr_color.s.red   * alpha * (255 - r)) + 0x8000) >> 16);

            register unsigned int p12 = // 12 bit r,g,b color
              ((b & 0xF0) >> 4) | (g & 0xF0) | ((r & 0xF0) << 4);
           *dest = color12_to_8[p12];  // lookup palette
        }
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
        alpha = ((dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha) + 0x0080) >> 8;  // alpha 0..255
        if( alpha > 14 )
        {
            // Calculate transparency using RGBA
            register RGBA_t p = pLocalPalette[*dest];
            unsigned int r,g,b;
            // (0xFF * 0xFF * 0xFF) = 0xFD02FF
            b = (p.s.blue * alpha_r)>>8;
            b += ((((unsigned int)dr_color.s.blue  * alpha * (255 - b)) + 0x8000) >> 16);
            g = (p.s.green * alpha_r)>>8;
            g += ((((unsigned int)dr_color.s.green * alpha * (255 - g)) + 0x8000) >> 16);
            r = (p.s.red * alpha_r)>>8;
            r += ((((unsigned int)dr_color.s.red   * alpha * (255 - r)) + 0x8000) >> 16);

            register unsigned int p12 = // 12 bit r,g,b color
              ((b & 0xF0) >> 4) | (g & 0xF0) | ((r & 0xF0) << 4);
           *dest = color12_to_8[p12];  // lookup palette
        }
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
        if( alpha > 14 )
        {
            // Calculate transparency using RGBA
            register RGBA_t p = pLocalPalette[*dest];
            unsigned int r,g,b;
            b = ((((unsigned int)dr_color.s.blue  * alpha ) + 0x0080) >> 8) + p.s.blue;
            if(b > 255)  b = 255;
            g = ((((unsigned int)dr_color.s.green * alpha ) + 0x0080) >> 8) + p.s.green;
            if(g > 255)  g = 255;
            r = ((((unsigned int)dr_color.s.red   * alpha ) + 0x0080) >> 8) + p.s.red;
            if(r > 255)  r = 255;

            register unsigned int p12 = // 12 bit r,g,b color
              ((b & 0xF0) >> 4) | (g & 0xF0) | ((r & 0xF0) << 4);
           *dest = color12_to_8[p12];  // lookup palette
        }
        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
    break;
  }
#else
    // Without ENABLE_DRAW8_USING_12 there is only a coarse table lookup.
    do
    {
        // Apply the dr_color, using the patch as alpha.
        // Colormap the dr_color at the source.
        alpha = ((dc_source[frac >> FRACBITS] * (unsigned int)dr_alpha)) >> 12;  // alpha 0..16

        // Select a transparent table to do alpha
        dc_translucentmap = & translucenttables[ translucent_alpha_table[ alpha ] ];
//        *dest = reg_colormaps[ dc_translucentmap[ (dr_color8 << dr8_src_shift[alpha]) + ((*dest) << dr8_src_shift[alpha]) ]];
        *dest = dc_translucentmap[ (dr_color8 << dr8_src_shift[alpha]) + ((*dest) << dr8_src_shift[alpha]) ];

        dest += vid.ybytes;
        frac += fracstep;
    }
    while (count--);
#endif

}
#endif

//
// I've made an asm routine for the transparency, because it slows down
// a lot in 640x480 with big sprites (bfg on all screen, or transparent
// walls on fullscreen)
//
// [WDJ] asm does not have latest changes
#ifndef USEASM
void R_DrawTranslucentColumn_8(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;

    // [WDJ] This check has been added to all the callers of colfunc().
    // check out coords for src*
//    if ((dc_yl < 0) || (dc_x >= vid.width))
//        return;

#ifdef USEBOOMFUNC
    count = dc_yh - dc_yl + 1;
    if (count <= 0)
        return;  // Zero length, column does not exceed a pixel.
#else
    count = dc_yh - dc_yl;
    if (count < 0)
        return;  // Zero length, column does not exceed a pixel.
#endif

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawTranslucentColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif
    // FIXME. As above.
    //src  = ylookup[dc_yl] + columnofs[dc_x+2];

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows? 
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling, which is the only mapping to be done.
    fracstep = dc_iscale;
#ifdef MONSTER_VARY
    frac = dc_texturemid + (dc_yl - dc_y0) * fracstep;
#else
    frac = dc_texturemid + (dc_yl - centery) * fracstep;
#endif

#ifdef USEBOOMFUNC
    // Do two pixels per loop when possible.
    {
        register const byte *source = dc_source;
        register int heightmask = dc_texheight - 1;
        if (dc_texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) < 0);
            else
                while (frac >= heightmask)
                    frac -= heightmask;

            if( dr_alpha < TRANSLU_REV_ALPHA )
            {
              do
              {
                // Re-map color indices from wall texture column
                //  using a lighting/special effects LUT.
                // heightmask is the Tutti-Frutti fix -- killough

                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS] << 8) + (*dest) ]];
                dest += vid.ybytes;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
              }
              while (--count);
            }
            else
            {
              do
              {
                // alpha >= TRANSLU_REV_ALPHA, reversed translucent table usage
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS]) + ((*dest)<<8) ]];
                dest += vid.ybytes;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
              }
              while (--count);
            }
        }
        else
        {
          if( dr_alpha < TRANSLU_REV_ALPHA )
          {
            while ((count -= 2) >= 0)   // texture height is a power of 2 -- killough
            {
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS] << 8) + (*dest) ]];
                dest += vid.ybytes;
                frac += fracstep;
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS] << 8) + (*dest) ]];
                dest += vid.ybytes;
                frac += fracstep;
            }
            if (count & 1)
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS] << 8) + (*dest) ]];
          }
          else
          {
            while ((count -= 2) >= 0)   // texture height is a power of 2 -- killough
            {
                // alpha >= TRANSLU_REV_ALPHA, reversed translucent table usage
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS]) + ((*dest)<<8) ]];
                dest += vid.ybytes;
                frac += fracstep;
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS]) + ((*dest)<<8) ]];
                dest += vid.ybytes;
                frac += fracstep;
            }
            if (count & 1)
                *dest = dc_colormap[ dc_translucentmap[ (source[frac >> FRACBITS]) + ((*dest)<<8) ]];
          }
        }
    }
#else
    // Simple loops.  No Tile.
    if( dr_alpha < TRANSLU_REV_ALPHA )
    {
      // Here we do an additional index re-mapping.
      do
      {
        *dest = dc_colormap[ dc_translucentmap[ (dc_source[frac >> FRACBITS] << 8) + (*dest) ]];
        dest += vid.ybytes;
        frac += fracstep;
      }
      while (count--);
    }
    else
    {
      do
      {
        // alpha >= TRANSLU_REV_ALPHA, reversed translucent table usage
        *dest = dc_colormap[ dc_translucentmap[ (dc_source[frac >> FRACBITS]) + ((*dest) << 8) ]];
        dest += vid.ybytes;
        frac += fracstep;
      }
      while (count--);
    }
#endif
}
#endif

// New spiffy function.
// Not only does it colormap a sprite,
// but does translucency as well.
// SSNTails 11-11-2002
// Uber-kudos to Cyan Helkaraxe for
// helping me get the brain juices flowing!
// Called with TRANSLU_hi or TRANSLU_more, or arbitrary by thing TRANSMASK
void R_DrawTranslatedTranslucentColumn_8(void)
{
    register int count;
    register fixed_t frac;
    register fixed_t fracstep;
    register byte *dest;

    count = dc_yh - dc_yl + 1;

    if (count <= 0)     // Zero length, column does not exceed a pixel.
        return;

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

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.

    {
        //register const byte *source = dc_source;
        register int heightmask = dc_texheight - 1;
        if (dc_texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) < 0);
            else
            {
                while (frac >= heightmask)
                    frac -= heightmask;
            }

            if( dr_alpha < TRANSLU_REV_ALPHA )
            {
              do
              {
                // Re-map color indices from wall texture column
                //  using a lighting/special effects LUT.
                // heightmask is the Tutti-Frutti fix -- killough
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]] << 8) + (*dest) ]];
                dest += vid.ybytes;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
              }
              while (--count);
            }
            else
            {
              do
              {
                // alpha >= TRANSLU_REV_ALPHA, reversed translucent table usage
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]]) + ((*dest)<<8) ]];
                dest += vid.ybytes;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
              }
              while (--count);
            }
        }
        else
        {
          if( dr_alpha < TRANSLU_REV_ALPHA )
          {
            while ((count -= 2) >= 0)   // texture height is a power of 2 -- killough
            {
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]] << 8) + (*dest) ]];
                dest += vid.ybytes;
                frac += fracstep;
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]] << 8) + (*dest) ]];
                dest += vid.ybytes;
                frac += fracstep;
            }
            if (count & 1)
            {
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]] << 8) + (*dest) ]];
            }
          }
          else
          {
            while ((count -= 2) >= 0)   // texture height is a power of 2 -- killough
            {
                // alpha >= TRANSLU_REV_ALPHA, reversed translucent table usage
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]]) + ((*dest)<<8) ]];
                dest += vid.ybytes;
                frac += fracstep;
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]]) + ((*dest)<<8) ]];
                dest += vid.ybytes;
                frac += fracstep;
            }
            if (count & 1)
            {
                *dest = dc_colormap[ dc_translucentmap[ (dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]]) + ((*dest)<<8) ]];
            }
          }
        }
    }
}

//
//  Draw columns upto 128high but remap the green ramp to other colors
//
//#ifndef USEASM        // STILL NOT IN ASM, TO DO..
void R_DrawTranslatedColumn_8(void)
{
    register int count;
    register byte *dest;
    register fixed_t frac;
    register fixed_t fracstep;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawTranslatedColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
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

    // Here we do an additional index re-mapping.
    do
    {
        // Skin Translation tables are used
        //  to map certain colorramps to other ones,
        //  used with PLAYER sprites.
        // Thus the "green" ramp of the player 0 sprite
        //  is mapped to gray, red, black/indigo.
        *dest = dc_colormap[dc_skintran[dc_source[frac >> FRACBITS]]];

        dest += vid.ybytes;

        frac += fracstep;
    }
    while (count--);
}

//#endif

// ==========================================================================
// SPANS
// ==========================================================================

//  Draws the actual span.
//
//#ifndef USEASM //Hurdler: in tmap.nas the old func is now called R_DrawSpan_8a
// [WDJ] Boom func modified several times
void R_DrawSpan_8(void)
{
    // [WDJ] was ULONG=32bit, use uint_fast32_t which can exceed 32bit in size.
    register uint_fast32_t  xfrac;
    register uint_fast32_t  yfrac;
    register byte *dest;
    register int count;

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
    count = ds_x2 - ds_x1;

     // [WDJ] note:  prboom has while(count)
    do
    {
        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        *dest = ds_colormap[ds_source[((yfrac >> flatfracbits) & (flat_ymask)) | (xfrac >> FRACBITS)]];
        dest++;

        // Next step in u,v.
        xfrac += ds_xstep;
        yfrac += ds_ystep;
        xfrac &= flat_imask;
    }
    while (count--);
}
//#endif USESASM

void R_DrawTranslucentSpan_8(void)
{
    fixed_t xfrac, yfrac;
    fixed_t xstep, ystep;
    byte *dest;
    int count;

#ifdef RANGECHECK_DRAW_LIMITS
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= rdraw_viewwidth || (unsigned) ds_y > rdraw_viewheight)
    {
        I_SoftError("R_DrawTranslucentSpan: %i to %i at %i\n", ds_x1, ds_x2, ds_y);
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

    if( dr_alpha < TRANSLU_REV_ALPHA )
    {
      do
      {
        // Current texture index in u,v.

        // Awesome! 256x256 flats!
//              spot = ((yfrac>>(FRACBITS-8))&(0xff00)) + (xfrac>>(FRACBITS));

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        //      *dest++ = ds_colormap[ds_source[spot]];
//              *dest++ = ds_colormap[*(ds_translucentmap + (ds_source[spot] << 8) + (*dest))];
        *dest = ds_colormap[ ds_translucentmap[ (ds_source[((yfrac >> flatfracbits) & (flat_ymask)) | (xfrac >> FRACBITS)] << 8) + (*dest) ]];
        dest++;	// [WDJ] warning: undetermined order when combined with above

        // Next step in u,v.
        xfrac += xstep;
        yfrac += ystep;
        xfrac &= flat_imask;
      }
      while (--count);
    }
    else   
    {
      do
      {
        // alpha >= TRANSLU_REV_ALPHA, reversed translucent table usage
        *dest = ds_colormap[ ds_translucentmap[ (ds_source[((yfrac >> flatfracbits) & (flat_ymask)) | (xfrac >> FRACBITS)]) + ((*dest)<<8) ]];
        dest++;	// [WDJ] warning: undetermined order when combined with above
        // Next step in u,v.
        xfrac += xstep;
        yfrac += ystep;
        xfrac &= flat_imask;
      }
      while (--count);
    }
    /*
       register unsigned position;
       unsigned step;

       byte *source;
       byte *colormap;
       byte *transmap;
       byte *dest;

       unsigned count;
       unsigned spot; 
       unsigned xtemp;
       unsigned ytemp;

       position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
       step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);

       source = ds_source;
       colormap = ds_colormap;
       transmap = ds_translucentmap;
       dest = ylookup[ds_y] + columnofs[ds_x1];
       count = ds_x2 - ds_x1 + 1; 

       while (count >= 4)
       {
       ytemp = position>>4;
       ytemp = ytemp & 0xff00;
       xtemp = position>>26;
       spot = xtemp | ytemp;
       position += step;
       dest[0] = colormap[*(transmap + (source[spot] << 8) + (dest[0]))];

       ytemp = position>>4;
       ytemp = ytemp & 0xff00;
       xtemp = position>>26;
       spot = xtemp | ytemp;
       position += step;
       dest[1] = colormap[*(transmap + (source[spot] << 8) + (dest[1]))];

       ytemp = position>>4;
       ytemp = ytemp & 0xff00;
       xtemp = position>>26;
       spot = xtemp | ytemp;
       position += step;
       dest[2] = colormap[*(transmap + (source[spot] << 8) + (dest[2]))];

       ytemp = position>>4;
       ytemp = ytemp & 0xff00;
       xtemp = position>>26;
       spot = xtemp | ytemp;
       position += step;
       dest[3] = colormap[*(transmap + (source[spot] << 8) + (dest[3]))];

       dest += 4;
       count -= 4;
       }

       while (count--)
       { 
       ytemp = position>>4;
       ytemp = ytemp & 0xff00;
       xtemp = position>>26;
       spot = xtemp | ytemp;
       position += step;
       *dest++ = colormap[*(transmap + (source[spot] << 8) + (*dest))];
       //count--;
       } 
     */
}

// blend of four translucents to imitate alpha translucent
static byte  fog_tran_table[16][4] = {
 { TRANSLU_hi, TRANSLU_hi, TRANSLU_hi, TRANSLU_hi },  // 10%
 { TRANSLU_hi, TRANSLU_hi, TRANSLU_hi, TRANSLU_hi },  // 10%
 { TRANSLU_more, TRANSLU_hi, TRANSLU_hi, TRANSLU_hi },  // 12.5%
 { TRANSLU_more, TRANSLU_hi, TRANSLU_more, TRANSLU_hi },  // 15%
 { TRANSLU_more, TRANSLU_more, TRANSLU_more, TRANSLU_hi },  // 17.5%
 { TRANSLU_more, TRANSLU_more, TRANSLU_more, TRANSLU_more },  // 20%
//   { TRANSLU_med, TRANSLU_hi, TRANSLU_hi, TRANSLU_hi },  // 20%
 { TRANSLU_med, TRANSLU_hi, TRANSLU_more, TRANSLU_hi },  // 22.5%
 { TRANSLU_med, TRANSLU_hi, TRANSLU_more, TRANSLU_hi },  // 22.5%
 { TRANSLU_med, TRANSLU_hi, TRANSLU_med, TRANSLU_hi },  // 30%
 { TRANSLU_med, TRANSLU_more, TRANSLU_med, TRANSLU_hi },  // 32.5%
 { TRANSLU_med, TRANSLU_more, TRANSLU_med, TRANSLU_more },  // 35%
 { TRANSLU_med, TRANSLU_more, TRANSLU_med, TRANSLU_more },  // 35%
 { TRANSLU_med, TRANSLU_med, TRANSLU_med, TRANSLU_hi },  // 40%
 { TRANSLU_med, TRANSLU_med, TRANSLU_med, TRANSLU_more },  // 42.5%
 { TRANSLU_med, TRANSLU_med, TRANSLU_med, TRANSLU_more },  // 42.5%
 { TRANSLU_med, TRANSLU_med, TRANSLU_med, TRANSLU_med },  // 50%
};

static byte fogstir = 0;

void R_DrawFogSpan_8(void)
{
    uint16_t fogcolor = ds_colormap[ ds_source[fog_index] ];  // to allow shift by 8
    byte alpha16;
    int count, count4;
    byte * ftranslucent[4];
    byte *dest;

    alpha16 = dr_alpha >> 3;  // dr_alpha 0..255 => 0..31
    if( alpha16 > 15 )
    {
        if( alpha16 > 31 )  alpha16 = 31;
        alpha16 = 31 - alpha16; // high alpha reverse trans
    }
    fogstir ++;
    for( count = 0; count < 4; count++ )
    {
        byte fogtran = fog_tran_table[ alpha16 ][ (count+fogstir) & 0x03 ];
        dc_translucentmap = & translucenttables[ TRANSLU_TABLE_INDEX(fogtran) ];
        ftranslucent[count] =   // normal table, or rev table
         & dc_translucentmap[ (dr_alpha<128) ? (fogcolor<<8) : fogcolor ];
    }

    count = ds_x2 - ds_x1 + 1;
    dest = ylookup[ds_y] + columnofs[ds_x1];

    count4 = count>>2;
    count -= count4<<2;
    if( dr_alpha < 128 )
    {
        // low alpha, use four translucent tables
        while (count4--)
        {
            *dest = ds_colormap[ ftranslucent[3][ (*dest) ]];
            dest ++;
            *dest = ds_colormap[ ftranslucent[2][ (*dest) ]];
            dest ++;
            *dest = ds_colormap[ ftranslucent[1][ (*dest) ]];
            dest ++;
            *dest = ds_colormap[ ftranslucent[0][ (*dest) ]];
            dest ++;
        }
        while( count-- )
        {
            *dest = ds_colormap[ ftranslucent[count&0x03][ (*dest) ]];
            dest ++;
        }
    }
    else
    {
        // high alpha, reversed use of four translucent tables
        while (count4--)
        {
            *dest = ds_colormap[ ftranslucent[3][ (*dest) << 8 ]];
            dest ++;
            *dest = ds_colormap[ ftranslucent[2][ (*dest) << 8 ]];
            dest ++;
            *dest = ds_colormap[ ftranslucent[1][ (*dest) << 8 ]];
            dest ++;
            *dest = ds_colormap[ ftranslucent[0][ (*dest) << 8 ]];
            dest ++;
        }
        while( count-- )
        {
            *dest = ds_colormap[ ftranslucent[count&0x03][ (*dest) << 8 ]];
            dest ++;
        }
    }
}


//SoM: Fog wall.
void R_DrawFogColumn_8(void)
{
   // fogcolor blur needs to be kept as 8 bit or else the blur is inadequate
static byte  fogcolor_c;
    int count, count4, fi;
    byte * ftranslucent[4];
    byte fc, fc2;
    byte alpha16;
    uint16_t fogcolor;  // to allow shift by 8

    fogstir ++;
    // fog_index 0.. column height
    // always average two pixels of source texture
    fi = fog_index + 3;
    fc = dc_colormap[ dc_source[fog_index] ];
    if( fi >= fog_col_length )  fi -= fog_col_length;
    fc2 = dc_colormap[ dc_source[fi] ];
    fc = translucenttables[ TRANSLU_TABLE_med + (fc2 << 8) + fc ];
    if( fog_init )
    {
        // init blur
        fogcolor_c = fc;
        fog_init = 0;
    }
    else
    {
        // blur
        fogcolor_c = translucenttables[ TRANSLU_TABLE_more + (fc << 8) + fogcolor_c ];
    }
    fogcolor = dc_colormap[ fogcolor_c ];

    alpha16 = dr_alpha >> 3;  // dr_alpha 0..255 => 0..31
    if( alpha16 > 15 )
    {
        if( alpha16 > 31 )  alpha16 = 31;
        alpha16 = 31 - alpha16; // high alpha reverse trans
    }
    for( count = 0; count < 4; count++ )
    {
        byte fogtran = fog_tran_table[ alpha16 ][ (count+fogstir) & 0x03 ];
        dc_translucentmap = & translucenttables[ TRANSLU_TABLE_INDEX(fogtran) ];
        if( dr_alpha < 128 )
           ftranslucent[count] = & dc_translucentmap[ fogcolor << 8 ];
        else
           ftranslucent[count] = & dc_translucentmap[ fogcolor ];  // rev table
    }

    count = dc_yh - dc_yl + 1;

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
    byte * dest = ylookup[dc_yl] + columnofs[dc_x];

    count4 = count>>2;
    count -= count4<<2;
    if( dr_alpha < 128 )
    {
        // low alpha, use four translucent tables
        while (count4--)
        {
            *dest = dc_colormap[ ftranslucent[3][ (*dest) ]];
            dest += vid.ybytes;
            *dest = dc_colormap[ ftranslucent[2][ (*dest) ]];
            dest += vid.ybytes;
            *dest = dc_colormap[ ftranslucent[1][ (*dest) ]];
            dest += vid.ybytes;
            *dest = dc_colormap[ ftranslucent[0][ (*dest) ]];
            dest += vid.ybytes;
        }
        while( count-- )
        {
            *dest = dc_colormap[ ftranslucent[count&0x03][ (*dest) ]];
            dest += vid.ybytes;
        }
    }
    else
    {
        // high alpha, reversed use of four translucent tables
        while (count4--)
        {
            *dest = dc_colormap[ ftranslucent[3][ (*dest) << 8 ]];
            dest += vid.ybytes;
            *dest = dc_colormap[ ftranslucent[2][ (*dest) << 8 ]];
            dest += vid.ybytes;
            *dest = dc_colormap[ ftranslucent[1][ (*dest) << 8 ]];
            dest += vid.ybytes;
            *dest = dc_colormap[ ftranslucent[0][ (*dest) << 8 ]];
            dest += vid.ybytes;
        }
        while( count-- )
        {
            *dest = dc_colormap[ ftranslucent[count&0x03][ (*dest) << 8 ]];
            dest += vid.ybytes;
        }
    }
}

#if 0
// [WDJ] Replaced by generic R_DrawColumnShadowed
// SoM: This is for 3D floors that cast shadows on walls.
// This function just cuts the column up into sections and calls
// R_DrawColumn_8
void R_DrawColumnShadowed_8(void)
{
    int count;
    int realyh, realyl;
    int i;
    int height, bheight = 0;
    int solid = 0;

    realyh = dc_yh;
    realyl = dc_yl;

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
        return;

#ifdef RANGECHECK_DRAW_LIMITS
    // [WDJ] Draw window is actually rdraw_viewwidth and rdraw_viewheight
    if ((unsigned) dc_x >= rdraw_viewwidth || dc_yl < 0 || dc_yh >= rdraw_viewheight)
    {
        I_SoftError("R_DrawShadowedColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
        return;
    }
#endif

    // SoM: This runs through the lightlist from top to bottom and cuts up
    // the column accordingly.
    for (i = 0; i < dc_numlights; i++)
    {
        // If the height of the light is above the column, get the colormap
        // anyway because the lighting of the top should be effected.
        solid = dc_lightlist[i].flags & FF_CUTSOLIDS;

        height = dc_lightlist[i].height >> 12;
        if (solid)
            bheight = dc_lightlist[i].botheight >> 12;
        if (height <= dc_yl)
        {
            dc_colormap = dc_lightlist[i].rcolormap;
            if (solid && dc_yl < bheight)
                dc_yl = bheight;
            continue;
        }
        // Found a break in the column!
        dc_yh = height;

        if (dc_yh > realyh)
            dc_yh = realyh;
        R_DrawColumn_8();
        if (solid)
            dc_yl = bheight;
        else
            dc_yl = dc_yh + 1;

        dc_colormap = dc_lightlist[i].rcolormap;
    }
    dc_yh = realyh;
    if (dc_yl <= realyh)
        R_DrawColumn_8();
}
#endif
