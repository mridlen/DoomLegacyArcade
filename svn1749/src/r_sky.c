// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_sky.c 1656 2023-12-08 14:54:47Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: r_sky.c,v $
// Revision 1.8  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.7  2001/04/02 18:54:32  bpereira
//
// Revision 1.6  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.5  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.4  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.3  2000/09/21 16:45:08  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Sky rendering. The DOOM sky is a texture map like any
//      wall, wrapping around. A 1024 columns equal 360 degrees.
//      The default sky map is 256 columns and repeats 4 times
//      on a 320 screen?
//  
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "r_local.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_maputl.h"
  // P_PointOnLineSide
#include "m_swap.h"
#include "r_sky.h"

#include "m_random.h"
#include "v_video.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif


// SoM: I know I should be moving portals out of r_sky.c and as soon
// as I have time and a I will... But for now, they are mostly used
// for sky boxes anyway so they have a mostly appropriate home here.

void sky_gen_OnChange( void );

CV_PossibleValue_t skygen_cons_t[] = {
    {0, "auto"}, {1,"subst"}, {10, "bg_stars" }, {11,"extend_stars"}, {12,"extend_bg"}, {13,"extend_fill"},
    {248,"vanilla"}, {250,"stretch"}, {0,NULL} };
consvar_t cv_sky_gen = {"skygen", "auto", CV_SAVE|CV_CALL, skygen_cons_t, sky_gen_OnChange };

void sky_gen_OnChange( void )
{
  if( sky_texture )
  {
    R_Setup_SkyDraw();
    R_Set_Sky_Scale();
  }
}

//
// sky mapping
//
int     sky_texture = 0;

// sky drawing
int     sky_flatnum;   // to detect sectors with sky
byte *  sky_pict = NULL; // ZMalloc, PU_STATIC
  // TM_picture format, columofs, column image
int     sky_texturemid;
fixed_t sky_scale;
uint32_t sky_widthmask = 0;
int     sky_height=128;
int     sky_yh_max_oc, sky_yl_min_oc;  // sky limits
byte    sky_240=0;  // 0=std 128 sky, 1=240 high sky

byte    skytop_flat[SKY_FLAT_WIDTH][SKY_FLAT_HEIGHT];  // above sky
byte    ground_flat[SKY_FLAT_WIDTH][SKY_FLAT_HEIGHT];  // below sky


// Sample the sky, setup the skytop_flat or ground_flat.
void  set_sky_flat( byte * sflat, byte * srcp, byte sample_y, int sample_width, int sample_height )
{
#if 0
    // A pattern just looks like vertical wall.
    // Sample the sky
    int i;
    for( i=0; i<(SKY_FLAT_WIDTH*SKY_FLAT_HEIGHT); i++ )
    {
        int rx = E_Random() % sample_width;
        int ry = sample_y + (E_Random() % sample_height );
	sflat[i] = srcp[ (rx * sky_height) + ry ];
    }
#endif
#if 1
    // Make a solid average color.
    int r = 0, g = 0, b = 0;
    int i;
    for( i=0; i<32; i++ )
    {
        int rx = E_Random() % sample_width;
        int ry = sample_y + (E_Random() % sample_height );
	byte c1 = srcp[ (rx * sky_height) + ry ];
        RGBA_t p = pLocalPalette[c1];
        r += p.s.red;
        g += p.s.green;
        b += p.s.blue;
    }
    byte solid_color = NearestColor( r/32, g/32, b/32 );
    memset( sflat, solid_color, (SKY_FLAT_WIDTH*SKY_FLAT_HEIGHT) );
#endif
#if 0
    // Pick the most used color.
    int hist[256];
    int max_hist = 0;
    int i;
    byte solid_color;
    memset( hist, 0, 256 );
    for( i=0; i<256; i++ )
    {
        int rx = E_Random() % sample_width;
        int ry = sample_y + (E_Random() % sample_height );
	byte c1 = srcp[ (rx * sky_height) + ry ];
        hist[c1]++;
        if( hist[c1] > max_hist )
        {
	    max_hist = hist[c1];
	    solid_color = c1;
	}
    }
    memset( sflat, solid_color, (SKY_FLAT_WIDTH*SKY_FLAT_HEIGHT) );
#endif
}


#define  SKY_WIDTH   1024
#define  SKY_HEIGHT   240
#define  SKY_HORIZON_OFFSET  12

typedef  struct{
   uint32_t  colofs[ SKY_WIDTH ];
} pic_header_t;

static
byte dark_pixel( byte c )
{
    RGBA_t rc = pLocalPalette[ c ];
    return( rc.s.red < 2 && rc.s.green < 2 && rc.s.blue < 2 );
}

static
void  sample_sky( int r1, int dr, int tw, int twm, byte buf[SKY_WIDTH][SKY_HEIGHT], byte background_color, byte stargen )
{
    int c;
    byte pix;
    for( c=0; c < tw; c++ )
    {
        if( buf[c][r1] != background_color )  continue;

        int c3 = (c + E_SignedRandom( 21 )) & twm;
        int r3 = r1 + 1 + ((E_Random() * dr) >> 16);
        pix = buf[c3][r3];
        if( stargen && ! dark_pixel(pix) )
        {
            byte cn = 15;
            while( cn-- )
            {
               // Find a star, surrounded by dark.
               if( ( dark_pixel( buf[(c3-1)&twm][r3] )
                + dark_pixel( buf[(c3+1)&twm][r3] )
                + dark_pixel( buf[(c3)&twm][r3-1] )
                + dark_pixel( buf[(c3)&twm][r3+1] ) ) > 2 )  break;
               c3 += 27;
               pix = buf[c3&twm][r3];
            }
        }
        buf[c][r1] = pix;
    }
}


// r : percentage of op1 to op2
static
float proportion( float op1, float op2, float r )
{
    return  (op1 * r) + (op2 * (1.0f - r));
}

//#define DEBUG_SLOPE
//  pixel : the solid pixel
//  c1, r1 : where
//  edgeinc : -1 left edge of solid, +1 for right edge
//  rinc : +1 for top, -1 for bottom
//  twm : mask for c
//  buf : the draw buffer
//  reg_slope : regular slope, as a guide
//  max_slope : maximum
//  pinch_inc : which way and how strongly to pinch slopes together
//  solid_width : width of the solid area
static
int  edge_by_slope( byte pixel, int c1, int r1, int edgeinc, int rinc, unsigned int twm, byte buf[SKY_WIDTH][SKY_HEIGHT], float reg_slope, float max_slope, float pinch_inc, int solid_width )
{
    float accum_slope = 0.0;
    float slope = 0;
    int   accum_cnt = 0;
    int  c, r4, c4;
    int  cinc = edgeinc; // left has outside to the -1 direction
    byte  inside = 0;  // do outside first
    byte  max_inside_depth = 0;
    byte  dep;

    // Assert pixel == buf[c1][r1]

    // Bidirectional loop
    for( c = c1+cinc;  ; c += cinc )
    {
        c4 = c & twm;
        r4 = r1;  // first pixel
       
        if( inside == 0 )
        {
            // Assert buf[c4][r1] != pixel
            for( dep = 1; dep < 17; dep++ )
            {
                r4 -= rinc;  // previous rows
                if( buf[c4][r4] == pixel )  break;
            }
            if( dep >= 16 )  c = 0x7FFF;  // end of pixel solid
        }
        else
        {
            // inside
            if( buf[c4][r1] != pixel )  break;  // end of pixel solid
            // find depth of last matching pixel
            for( dep = 0; dep < 16; dep++ )
            {
                r4 -= rinc;  // previous rows
                if( buf[c4][r4] != pixel )  break;
            }
            // Need the max_inside_depth for pinch calc,
            // but should avoid including the opposite edge of the solid.
            if( dep > max_inside_depth )  // dep should be increasing
                max_inside_depth = dep;
            else
                dep = 0;  // probably the opposite edge
        }
       
        if( (dep > 0) && (dep < 16) )
        {
            // avg the slopes, weighted by their dep
            accum_slope += (float)(c - c1);  // ((c-c1) / dep) * dep
            accum_cnt += dep;
        }

        if(( c <= c1-16 ) || ( c >= c1+16 ))
        {
            if( inside )  break;
            // switch to the inside, and the other direction
            inside = 1;
            cinc = -cinc;
            c = c1;
        }
    }

    // column per row, slope = 0 is straight up, neg towards neg c.
    if( accum_cnt )
    {
        slope = (accum_slope / accum_cnt);
    }
    else
    {
        // choose a random slope; there is no slope sign
        slope = E_SignedRandom(2048) * (2.0f/2048) * reg_slope;  // 0 to 2*reg_slope
    }

    // Use E_Random because it does not repeat within buf size.
    int   er = E_Random();
    float signed_reg_slope = (slope < 0)? -reg_slope : reg_slope;

    if( fabs(slope) < 0.4f )
    {
        // random slope, near 0 to signed_reg_slope/2
        slope = (er + 0x3000) * (0.45f/0xFFFF) * signed_reg_slope;
    }
    else if( er > 0xCC13 )
    {
        slope = proportion( signed_reg_slope, slope, 0.150f );
    }

    slope += E_SignedRandom(1024) * (1.27f/1024);

    // Pinch as function of depth inside.
    if( solid_width < 1 )  solid_width = 1;
    float pinch_strength = pinch_inc * 0.058f / solid_width;
    slope += pinch_strength * max_inside_depth;  // pinch inwards
     
    if( fabs(slope) > max_slope )
    {
        er = E_Random();

        if( ((edgeinc * slope) > 0) && ( er < 0x2BFF ))
        {
            // Flip slope direction
            slope = -slope * 0.980f;
        }

        // Apply some reg_slope.
        slope = proportion( reg_slope, slope, ((er * (0.8f/0xFFFF)) + 0.1f) );

        if( slope > max_slope )  slope = max_slope - (er * (2.0f/0xFFFF));
        else if( slope < -max_slope )  slope = -max_slope + (er * (2.0f/0xFFFF));
    }

    return  (int)(c1 + slope + 0.5f);
}



typedef enum {
   BE_none,
   BE_greater,
   BE_less,
   BE_adhere
} blob_edge_e;

typedef struct {
  int c1, c2;  // first and last columns
  byte solid_color;
  byte speck;
  byte importance;
  int16_t  sb_left, sb_right;  // index sbp
  int16_t  sb_enclosure;  // enclosing sky_blob
  byte  has_enclosed;
  byte  left_adj, right_adj;
} sky_blob_t;

static
void init_sky_blob( sky_blob_t * sbp )
{
    sbp->c1 = 0;
    sbp->c2 = 0;
    sbp->speck = 0;
    sbp->left_adj = BE_none;
    sbp->right_adj = BE_none;
    sbp->sb_left = -1;
    sbp->sb_right = -1;
    sbp->has_enclosed = 0;
    sbp->sb_enclosure = -1;
    sbp->importance = 1;
}


// Generate large sky.
// [WDJ] Must do this into a temp texture, or else it recursively modifies the sky with each level.
byte * R_Generate_Sky( int texnum )
{
    texture_t * texture = textures[texnum]; // texture info from wad
    byte buf[SKY_WIDTH][SKY_HEIGHT];  // buf of composite sky, column oriented

#define HIST_DEPTH   8
    unsigned int  sky_hist[256], gnd_hist[256];
    unsigned int  hist_total;
    unsigned int  sky_dark_cnt = 0, gnd_dark_cnt = 0;
    int  th, tw;
    unsigned int twm;  // width mask
    int  sky_top_align, sky_bottom_align, sky_horizon, sl_horizon;
//    int  pixel_cnt;
    int  r, c;
    int  c1, c2, c3, c4, r1, r2, r3;
    byte background_color = 0;
    byte sky_color = 0;
    byte ground_color = 0;
    byte en_background_transparent;
    byte night_stars_upper = 0;
    byte night_stars_lower = 0;

    // Blob building
#define MAX_SKY_BLOB   SKY_WIDTH
    sky_blob_t  sky_blob[ MAX_SKY_BLOB ];
    sky_blob_t * sbp, * sbp2, * sbp3, * sbp4;
    sky_blob_t * last_sbp;
 
    int rinc;
    int row1, row2;
//    int num_rows;
    int speck_per;
   
    float reg_slope, max_slope, per;
    byte c_pixel, solid_pixel, write_pixel;
    byte db;
   

    // This puts ptr in texren->cache
    pic_header_t * sky_pic = (pic_header_t*) R_GenerateTexture2( texnum, & texture_render[texnum], TM_picture );

    th = texture->height; // should be 128
    tw = texture->width;  // 256, 512, or 1024
       // Requiem has SKY1 width 1024.
    if( tw > SKY_WIDTH )   tw = SKY_WIDTH;
   
    // tw must be a power of 2.
    twm = tw-1;

    sky_top_align = SKY_HEIGHT - SKY_HORIZON_OFFSET - th;
    if( sky_top_align < 0 )  sky_top_align = 0;
    sky_bottom_align = sky_top_align + th;
    if( sky_bottom_align > SKY_HEIGHT )  sky_bottom_align = SKY_HEIGHT;
    sky_horizon = sky_bottom_align - SKY_HORIZON_OFFSET;

    // Histogram of the sky top 16 rows.
    memset( sky_hist, 0, 256 * sizeof(int) );
    memset( gnd_hist, 0, 256 * sizeof(int) );

    // Note: The sky is displayed reversed left to right,
    // so increasing c is to the left.
    // Find background and ground color
    int bs = 0, bg = 0;
    for( c=0; c<tw; c++ )
    {
        // R_GenerateTexture gives it TM_picture format.
        byte * p = (byte*)sky_pic + sky_pic->colofs[ c ];
        memcpy( &buf[c][sky_top_align], p, th );  // copy sky to buf

        // Detect night sky upper,lower, and histogram.
        for( r=0; r<HIST_DEPTH; r++ )
        {
            byte pix = buf[c][sky_top_align+r];
            int hc = ++sky_hist[pix];  // count pixel colors
            if( hc > bs )  // largest hist
            {
                bs = hc;
                sky_color = pix;
            }
            if( dark_pixel(pix) )  sky_dark_cnt++;

            pix = buf[c][sky_bottom_align-1-r];
            hc = ++gnd_hist[pix];  // count pixel colors
            if( hc > bg )  // largest hist
            {
                bg = hc;
                ground_color = pix;
            }
            if( dark_pixel(pix) )  gnd_dark_cnt++;
        }
    }
    hist_total = tw * HIST_DEPTH;

    if( sky_dark_cnt > (hist_total * 4 / 7) )
    {
        night_stars_upper = 1;
    }

    if( gnd_dark_cnt > (hist_total * 4 / 7) )
    {
        night_stars_lower = 1;
    }
   
    sl_horizon = sky_top_align / 2;

    if(cv_sky_gen.EV == 10) // bg_stars
    {
        en_background_transparent = 1;
        sl_horizon = sky_horizon;
    }
    else if(cv_sky_gen.EV == 11) // extend_stars
    {
//        en_background_transparent = 1;
        sl_horizon = sky_top_align * 4 / 5;
//        sky_color = ci_black;
    }
    else if(cv_sky_gen.EV == 12)  // extend_bg
    {
        en_background_transparent = 1;
        sl_horizon = sky_top_align / 2;
    }
   
#if 0
    if( night_stars_upper || (cv_sky_gen.EV == 11))
    {
        // This color is detected as transparent.
        sky_color = ci_black;
    }
   
    if( night_stars_lower )
    {
        ground_color = ci_black;
    }
#endif   
   
    for( c1=0; c1 < tw; c1++ )
    {
        // set to solid background color
        memset( &buf[c1][0], sky_color, sky_top_align );
        memset( &buf[c1][sky_bottom_align], ground_color, SKY_HEIGHT - sky_bottom_align );
    }
   
    if( night_stars_upper && night_stars_lower )  goto generate_backgrounds;
   
    // Note: buf is circular.  Do previous row setup first.
    // Upper sky values.
    background_color = sky_color;
    row1 = sky_top_align;
    row2 = 0;
//    num_rows = sky_top_align;
    rinc = -1;

    for(;;)
    {
        // Blob building
        sbp = & sky_blob[0];
        init_sky_blob( sbp );
        solid_pixel = buf[0][row1];
        sbp->c1 = 0;
        sbp->solid_color = solid_pixel;
       
        // Initial scan of row1.
        // Positive c is to the right, but display is reversed.
        for( c1 = 1; c1<tw; c1++ )
        {
            // Find trailing edge of each solid
            c_pixel = buf[c1][row1];
            if( c_pixel == solid_pixel )  continue;

            if( buf[(c1+1)&twm][row1] == solid_pixel )
            {
                sbp->speck++;
                continue;
            }

            // Record blob info.
            sbp->c2 = c1-1;
            sbp->importance = 1;

            // Do not cross background blobs.
            if( solid_pixel == background_color )  goto next_blob;

            // check for blob in blob
            c4 = c1 - (tw/4);
            for( sbp2 = & sbp[-1]; sbp2 >= &sky_blob[0]; sbp2-- )
            {
                if( sbp2->solid_color == background_color )   break;
                if( sbp2->importance < 1 )   break;
                if( sbp2->c1 < c4 )  break; // too large a structure

                // This does produce overlapping enclosures,
                // but those make for an interesting sky feature.
                if( sbp2->solid_color == solid_pixel )
                {
                    int sb2id = sbp2 - &sky_blob[0];
                    // part of larger blob, enclosing a smaller blob
                    sbp2->c2 = c1-1;
                    sbp2->importance++;

                    // linking
                    sbp3 = NULL;  // default, sb2 has no enclosures
                    // traverse interior blobs
                    for( sbp4 = sbp2+1; sbp4 < sbp; sbp4++ )
                    {
                        if( sbp4->sb_enclosure == sb2id )
                        {
                            sbp3 = sbp4;
                        }
                        else if( sbp4->sb_enclosure < 0 )  // an unlinked blob
                        {
                            // enclose it in sbp2
                            sbp4->sb_enclosure = sb2id; // enclosed by sbp2
                            if( sbp3 )
                            {
                                int sb3id = sbp3 - &sky_blob[0];
                                sbp4->sb_left = sb3id;
                            }
                            else
                            {
                                sbp4->sb_left = -1;  // first
                            }
                            sbp2->has_enclosed = 1;
                            sbp3 = sbp4;
                        }
                    } // traverse interior blobs
                    sbp--; // was merged
                    break;
                } // solid_color == solid_pixel
            } // for sbp2, blob in blob

         next_blob:
            // next blob
            sbp++;
            init_sky_blob( sbp );
            solid_pixel = c_pixel;
            sbp->solid_color = c_pixel;
            sbp->c1 = c1;
            int sbid = sbp - &sky_blob[0];
            sbp->sb_left = sbid - 1;
        }
        sbp->c2 = tw-1;
        sbp->importance = 1;

        // Wrap the sky
        if((sbp > &sky_blob[0]) && (sbp->solid_color == sky_blob[0].solid_color) )
        {
            // sky wraps, merge first and last
            sky_blob[0].c1 = sbp->c1 - tw;
            sky_blob[0].sb_left = sbp->sb_left;
            sbp--;
        }
        last_sbp = sbp;

        // fix right associations
        for( sbp = &sky_blob[0];  ; sbp++ )
        {
            if( sbp->sb_left >= 0 )
                sky_blob[ sbp->sb_left ].sb_right = sbp - &sky_blob[0];

            if( sbp >= last_sbp )  break;
        }
       
        // Draw
        for( r1 = row1 + rinc; ; r1+=rinc )
        {
            r3 = r1 - rinc;  // prev row
            if( rinc < 0 )
            { // upper
                if( r1 < row2 )  break;
                per = (float)r1 / sky_horizon;
            }
            else
            { // lower
                if( r1 > row2 )  break;
                per = (float)(SKY_HEIGHT - r1) / (SKY_HEIGHT - sky_horizon);
            }
            // per goes to 0 as nears viewer
            reg_slope = proportion(  6.0f, 1.95f, per );
            max_slope = proportion( 13.0f, 4.00f, per );

            for( sbp = & sky_blob[0]; sbp <= last_sbp; sbp++ )
            {
                if( sbp->importance < 1 )  continue;

                solid_pixel = sbp->solid_color;
                if( (solid_pixel == background_color) &&  en_background_transparent )
                {
                    // background has already been filled in and there is no overwrite needed
                    continue;
                }

                c1 = sbp->c1;
                c2 = sbp->c2;

                if(( c1 > c2 ) && (c1 > (3*SKY_WIDTH/4)) && (c2 < (SKY_WIDTH/4)) )
                    c1 -= SKY_WIDTH;  // wrap

                if( sbp->sb_enclosure >= 0 )
                {
                    sbp2 = & sky_blob[ sbp->sb_enclosure ];

                    int c5 = sbp2->c1;
                    int c6 = sbp2->c2;

                    if( c5 > c2 && (c5 > (3*SKY_WIDTH/4)) )
                    {
                        c5 -= SKY_WIDTH;
                        c6 -= SKY_WIDTH;
                    }
                    else if( c6 < c1 && (c6 < (SKY_WIDTH/4)) )
                    {
                        c5 += SKY_WIDTH;
                        c6 += SKY_WIDTH;
                    }

                    if( c1 <= c5 )
                    {
                        c1 = c5 + 1;
                    }
                    if( c2 >= c6 )
                    {
                        c2 = c6 - 1;
                    }
                }

                // negative pinch when above sl_horizon
                int width = c2 - c1 + 1;
                if( width < 1 )
                {
                    sbp->importance = 0;
                    continue;
                }

                c3 = edge_by_slope( solid_pixel, c1, r1,  1, rinc, twm, buf, reg_slope, max_slope, (r1 - sl_horizon), (c2 - c1 + 1) );
                c4 = edge_by_slope( solid_pixel, c2, r1, -1, rinc, twm, buf, reg_slope, max_slope, (sl_horizon - r1), (c2 - c1 + 1) );

                if(( c3 > c4 ) && (c3 > (3*SKY_WIDTH/4)) && (c4 < (SKY_WIDTH/4)) )
                    c3 -= SKY_WIDTH;  // wrap

                sbp->c1 = c3;
                sbp->c2 = c4;

                if( c3 >= c4 )
                {
                    sbp->importance = 0;
                    continue;
                }

                speck_per = sbp->speck / (c4 - c3);
                if( speck_per > 250 )  speck_per = 250;

                db = (buf[c3&twm][r3] != solid_pixel) ? 2 : 0;

                // stripe write
                for( c = c3; c <= c4; c++ )
                {
                    byte rn = E_Random();

                    write_pixel = solid_pixel;
                    if( (c < c4) && (buf[(c+1)&twm][r3] != solid_pixel) )  db |= 4;

                    if( db )
                    {
                        db >>= 1;
                        if( rn < 97 )
                        {
                            int cs = c + (E_Random() & 0x07) - 3;
                            int rs = r1 - (((E_Random() & 0x07) + 1) * rinc);
                            write_pixel = buf[cs&twm][rs];
                        }
                    }
                    else if( speck_per && (speck_per > rn) )
                    {
                        // with specks
                        int cs = c + (E_Random() & 0x0F) - 7;
                        int rs = row1 - ((E_Random() & 0x0F) * rinc);
                        write_pixel = buf[cs&twm][rs];
                    }
                    buf[c&twm][r1] = write_pixel;
                }
            }
        }
       
        // lower ground
        if( row2 > SKY_HEIGHT - 2 )  break;
        row1 = sky_bottom_align - 1;
        row2 = SKY_HEIGHT - 1;
//        num_rows = row2 - row1;
        rinc = 1;
        background_color = ground_color;
        sl_horizon = 0;
    }




   

generate_backgrounds:
    // Generate background above sky texture.
    // Copy stars has precedence because it matches better.
    if( night_stars_upper || ( cv_sky_gen.EV == 13 ))  // or fill random
    {
        for( r1 = sky_top_align-1; r1 >= 0; r1-- )  // must be row first
        {
            int dr = ((sky_top_align - r1) >> 1) + 2;
            sample_sky( r1, dr, tw, twm, buf, sky_color, night_stars_upper );
        }
    }
    else if((cv_sky_gen.EV == 10) || (cv_sky_gen.EV == 11))  // generate upper stars
    {
        int  rst[SKY_WIDTH];  // r position for this c
        byte ci_star1 = NearestColor( 80, 80, 80 );
        byte ci_star2 = NearestColor( 135, 135, 135 );

        memset( rst, 0, sizeof(rst) );
        r2 = (cv_sky_gen.EV == 10)? sky_bottom_align : sky_top_align;
        for( c1=0; c1 < tw; c1++ )
        {
            for( r1 = rst[c1]; r1 < r2; r1++ )
            {
                if( buf[c1][r1] != sky_color )
                {
                    if((cv_sky_gen.EV == 10) && (r1 > sky_top_align) && ((c1==0) || (r1 > rst[c1-1])))  break;
                    continue;
                }

                rst[c1] = r1;
                if((cv_sky_gen.EV == 10) && (c1>0) && (buf[c1-1][r1] == sky_color))
                {
                    c1--; // background to the left, follow it
                }

                // Generate a new star sky
                c4 = 0x35C + (((r1 ^ c1)>>4) & 0x3f);
                buf[c1][r1] = ((E_Random() & 0x3FF) < c4)? ci_black
                     : (E_RandomPercent( 0.13f ))? ci_star2
                     : (E_RandomPercent( 1.28f ))? ci_star1
                     : ci_grey;
            }
        }
    }
   
    // Generate background below sky texture.
    if( night_stars_lower || ( cv_sky_gen.EV == 13 ))  // or fill random
    {
        for( r1 = sky_bottom_align; r1 < SKY_HEIGHT; r1++ )
        {
            int dr = ((r1 - sky_bottom_align) >> 1) + 2;
            sample_sky( r1, dr, tw, twm, buf, ground_color, night_stars_lower );
        }
    }

    goto make_patch;
   
make_patch:
  {
    // Source is  column oriented, pixels are a byte no offset, no blank pixel.
    // Create a TM_picture, with colofs array, no blank trim.
    byte * sky_pict1 = R_Create_Patch( tw, SKY_HEIGHT,
                /*SRC*/    TM_column_image, &buf[0][0], 1, 0, 299,
                /*DEST*/   TM_picture, 0, NULL );
    return sky_pict1;
  }
}


//
// R_Init_SkyMap called at startup, once.
//
void R_Init_SkyMap (void)
{
    // set at P_LoadSectors
    //sky_flatnum = R_FlatNumForName ( SKYFLATNAME );
}


//  Setup sky draw for old or new skies (new skies = freelook 256x240)
//
//  Call at loadlevel after sky_texture is set
//
//  NOTE: skycolfunc should be set at R_ExecuteSetViewSize ()
//        I dont bother because we don't use low detail no more
//
void R_Setup_SkyDraw (void)
{
    texpatch_t*  texpatch;
    patch_t      wpatch;
    int          count;
    int          max_height;
    int          i;

    if( sky_texture == 0 )  return;

    // parse the patches composing sky texture for the tallest one
    // patches are usually RSKY1,RSKY2... and unique

    // note: the TEXTURES lump doesn't have the taller size of Legacy
    //       skies, but the patches it use will give the right size

    count   = textures[sky_texture]->patchcount;
    texpatch = &textures[sky_texture]->patches[0];
    max_height = 0;
    for (i=0;i<count;i++)
    {
        W_ReadLumpHeader( texpatch->lumpnum, &wpatch, sizeof(patch_t) );
        // [WDJ] Do endian fix as this is read.
        wpatch.height = (uint16_t)( LE_SWAP16(wpatch.height) );
        if( wpatch.height > max_height )
            max_height = wpatch.height;
        texpatch++;
    }

    // Doom:  sky texture height=128, sky patch height=128
    // Heretic: sky texture height=128, sky patch height=200, leftoffset=127, topoffset=195
    // Legacy wad RSKY: sky patch height = 240
   
    if( sky_pict )
    {
        Z_Free( sky_pict );
        sky_pict = NULL;
    }

    texture_render[sky_texture].texture_model = TM_none;  // to not affect R_GenerateTexture

    // the horizon line in a 256x128 sky texture
    sky_texturemid = 100<<FRACBITS;   // Boom
    sky_height = 128;  // vanilla, Boom, default

    if(max_height > 128)
    {
        // Need to convince R_GenerateTexture2 of new height.
        if( max_height > textures[sky_texture]->height )
            textures[sky_texture]->height = max_height;
        sky_height = max_height;
        sky_pict = R_GenerateTexture2( sky_texture, & texture_render[sky_texture], TM_picture );
        sky_240 = 1;
        // horizon line on 256x240 freelook textures of Legacy or heretic
        sky_texturemid = 200<<FRACBITS;
    }
    else
    {
        if( cv_sky_gen.value == 0 )  // Auto
        {
            // If have subst sky, then use that
            // otherwise extend the sky.
            cv_sky_gen.EV = 12;  // extend_bg
        }

        if( cv_sky_gen.EV >= 10 && cv_sky_gen.EV < 99 )
        {
            // Extend sky
            sky_pict = R_Generate_Sky( sky_texture );
            sky_240 = 1;
            sky_height = SKY_HEIGHT;
            sky_texturemid = 200<<FRACBITS;
        }
        else
        {
            // Vanilla or Stretch sky
            sky_pict = R_GenerateTexture2( sky_texture, & texture_render[sky_texture], TM_picture );
            sky_240 = 0;
            if( max_height > 128 )
                sky_height = max_height;
        }
    }
    Z_ChangeTag( sky_pict, PU_STATIC );

    // Sky has dedicated texture.

    // determine width power of 2
    // Sky is 128, 256, 512, or 1024 wide.
    int tw = textures[sky_texture]->width;
    sky_widthmask = tw - 1;
    if( tw & sky_widthmask )
    {
        // The hard way, power of 2
        i = 1;
        while( (i<<1) < tw )  i = i<<1;
        sky_widthmask = i - 1;
    }

#if 0   
    if( cv_sky_gen.EV == 248 )  // vanilla
    {
        // Vanilla always has solid flats.	   
        memset( skytop_flat, *(sky_pict + ((uint32_t*)sky_pict)[0]), SKY_FLAT_WIDTH*SKY_FLAT_HEIGHT );
        memset( ground_flat, *(sky_pict + ((uint32_t*)sky_pict)[0] + sky_height-1), SKY_FLAT_WIDTH*SKY_FLAT_HEIGHT );
    }
    else
#endif
    {
        byte * sip = sky_pict + (((uint32_t*) sky_pict)[0]);  // sky image
        set_sky_flat( &skytop_flat[0][0], sip, 0, tw, 8 );  // sample upper sky
        set_sky_flat( &ground_flat[0][0], sip, sky_height-8, tw, 8 );  // sample lower sky, for ground
    }

    // Get the right drawer.  It was set by screen.c, depending on the
    // current video mode bytes per pixel (quick fix)
    skycolfunc = skydrawerfunc[sky_240];

    R_Set_Sky_Scale ();

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_sky_mipmap();
    }
#endif   
}


// set the correct scale for the sky at setviewsize
void R_Set_Sky_Scale (void)
{
    // Altered by screensize, splitscreen
    //  rdraw_viewheight, centery
// Boom
//   SCREENHEIGHT=200, SCREENWIDTH=320
//   centery = viewheight/2;
//   pspriteyscale = (((SCREENHEIGHT*viewwidth)/SCREENWIDTH)<<FRACBITS)/200;
//   projectiony = (((SCREENHEIGHT*centerx*320)/200)/SCREENWIDTH)*FRACUNIT;
//   sky  iscale = FRACUNIT*200/viewheight;
// Legacy
//   BASEVIDHEIGHT=200, BASEVIDWIDTH=320
//   centery = rdraw_viewheight/2;
//   pspriteyscale = (((vid.height*rdraw_viewwidth)/vid.width)<<FRACBITS)/BASEVIDHEIGHT;
//   projection_y = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width)<<FRACBITS;
//   sky  dc_iscale = FixedDiv (FRACUNIT, pspriteyscale);
 
    // normal aspect ratio corrected scale
    // sky_scale = (FRACUNIT*200)/rdraw_viewheight;  // Boom
    sky_scale = FixedDiv (FRACUNIT, pspriteyscale);  // Legacy

    if( (cv_sky_gen.EV == 250) && (sky_height < 130) )  // Stretch
    {
        // double the texture vertically, bleeergh!!
        sky_scale >>= 1;
    }

    // Sky draw limits
    // fixed_t  frac = dc_texturemid + (dc_yl - centery) * fracstep;
    // limit frac to (0..sky_height-1) << 16
    // centery varies according to view    
    // (dc_yl - centery) = ((frac - dc_texturemid) / fracstep);
    sky_yl_min_oc = ( 0 - sky_texturemid ) / sky_scale;  // frac = 0
    sky_yh_max_oc = ( ((sky_height-1)<<16) - sky_texturemid ) / sky_scale;  // frac = sky_height-1
}
