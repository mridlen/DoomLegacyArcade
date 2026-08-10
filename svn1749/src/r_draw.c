// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_draw.c 1691 2024-11-27 06:36:07Z wesleyjohnson $
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
// $Log: r_draw.c,v $
// Revision 1.14  2003/06/10 23:36:09  ssntails
// Variable flat support (32x32 to 2048x2048)
//
// Revision 1.13  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.12  2001/04/01 17:35:06  bpereira
//
// Revision 1.11  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.10  2001/02/24 13:35:21  bpereira
//
// Revision 1.9  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.8  2000/11/09 17:56:20  stroggonmeth
// Revision 1.7  2000/11/03 03:48:54  stroggonmeth
//
// Revision 1.6  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/07/01 09:23:49  bpereira
//
// Revision 1.4  2000/04/07 18:47:09  hurdler
// There is still a problem with the asm code and boom colormap
// At least, with this little modif, it compiles on my Linux box
//
// Revision 1.3  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      span / column drawer functions, for 8bpp and 16bpp
//
//      All drawing to the view buffer is accomplished in this file.
//      The other refresh files only know about ccordinates,
//      not the architecture of the frame buffer.
//      The frame buffer is a linear one, and we need only the base address.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "doomstat.h"
#include "r_local.h"
#include "st_stuff.h"
  //added:24-01-98:need ST_HEIGHT
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "console.h"
  //Som: Until I get buffering finished
#include "r_draw.h"
#include "r_data.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// ==========================================================================
//                     COMMON DATA FOR 8bpp AND 16bpp
// ==========================================================================

// [WDJ] rdraw_ are "render drawing window" variables, which have the
// dimensions of the window into which the span and column routines draw.
// (view is already used by player, window is also used, rw_ is used)
byte*           viewimage;
int             rdraw_viewwidth;		// was viewwidth
                        // width used by drawing routines
                        // half of pixel width when low res
int             rdraw_scaledviewwidth;		// was scaledrviewwidth
                        // width of view window in pixels
int             rdraw_viewheight;		// was viewheight
                        // height of view window in rows (pixels)
// position of smaller rdraw_view window within vid window
int             view_window_x;
int             view_window_y;

                // pointer to the start of each line of the screen,
byte*           ylookup[MAXVIDHEIGHT];

byte*           ylookup1[MAXVIDHEIGHT]; // for view1 (splitscreen)
byte*           ylookup2[MAXVIDHEIGHT]; // for view2 (splitscreen)

                // x byte offset for columns inside the view_window
                // so the first column starts at (SCRWIDTH-VIEWWIDTH)/2
int             columnofs[MAXVIDWIDTH];

#ifdef HORIZONTALDRAW
//Fab 17-06-98: horizontal column drawer optimisation
byte*           yhlookup[MAXVIDWIDTH];
int             hcolumnofs[MAXVIDHEIGHT];
#endif

byte            dr_alpha;  // translucent and fog alpha, 0..255
#ifdef ENABLE_DRAW_ALPHA
byte            dr_alpha_mode;  // alpha combine modes
byte            dr_alpha_background;  // alpha applied to background
byte            dr_color8;
RGBA_t          dr_color;  // draw alpha
#endif

// =========================================================================
//                      COLUMN DRAWING CODE STUFF
// =========================================================================

lighttable_t*           dc_colormap;
int                     dc_x;
int                     dc_yl;
int                     dc_yh;
#ifdef MONSTER_VARY
int                     dc_y0;  // ref for draw, to allow separate sprite scaling
#endif

fixed_t                 dc_iscale;
fixed_t                 dc_texturemid;

byte*                   dc_source;


// -----------------------
// translucency stuff here
// -----------------------
// The number of translucency tables that are used.
#define NUM_TRANSLUCENTTABLES  5

byte*                   translucenttables;    // translucency tables

// R_DrawTransColumn uses this
byte*                   dc_translucentmap;    // one of the translucency tables
byte                    dc_translucent_index;


// ----------------------
// skin translation stuff
// ----------------------

// [WDJ] player skin translation, skintranstables[NUMSKINCOLORS-1][256]
// Does not translate color 0
byte*                   skintranstables;  // player skin translation tables

// R_DrawTranslatedColumn uses this
byte*                   dc_skintran; // ptr to one skintranstables table


struct r_lightlist_s*   dc_lightlist = NULL;
int                     dc_numlights = 0;
int                     dc_maxlights;

int     dc_texheight;

// =========================================================================
//                      SPAN DRAWING CODE STUFF
// =========================================================================

int                     ds_y;
int                     ds_x1;
int                     ds_x2;

lighttable_t*           ds_colormap;

fixed_t                 ds_xfrac;
fixed_t                 ds_yfrac;
fixed_t                 ds_xstep;
fixed_t                 ds_ystep;

byte*                   ds_source;      // start of a 64*64 tile image
byte*                   ds_translucentmap;    // one of the translucency tables

// Variable flat sizes SSNTails 06-10-2003
unsigned int flatsize;
unsigned int flatbitsz;  // flat bit size, flatsize = 2**flatbitsz
unsigned int flatfracbits; // FRACBITS - flatbitsz
unsigned int flat_ymask;   // index mask, = (flatsize-1)<<flatbitsz
fixed_t      flat_imask;   // index mask, = (flatsize<<FRACBITS) - 1


// ==========================================================================
//                        OLD DOOM FUZZY EFFECT
// ==========================================================================

//
// Spectre/Invisibility.
//
#define FUZZTABLE     50
#define FUZZOFF       (1)

static  int fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

static  int fuzzpos = 0;     // move through the fuzz table


//  fuzzoffsets are dependent upon vid width, for optimising purpose
//  this is called by SCR_Recalc() whenever the screen size changes
//
void R_RecalcFuzzOffsets (void)
{
    int i;
//    int offset = 1; // Doom original
//    int offset = vid.width // as in ver 1.42 (which seems wrong)
    int offset = (((vid.width * 2) / BASEVIDWIDTH) + 1)/2;  // proportional rounded
    offset *= vid.bytepp;
    for (i=0;i<FUZZTABLE;i++)
    {
        fuzzoffset[i] = (fuzzoffset[i] < 0) ? -offset : offset;
    }
}


// =========================================================================
//                   TRANSLATION COLORMAP CODE
// =========================================================================

char *Color_Names[NUMSKINCOLORS]={
   "Green",
   "Gray" ,
   "Brown",
   "Red"  ,
   "light gray" ,
   "light brown",
   "light red"  ,
   "light blue" ,
   "Blue"       ,
   "Yellow"     ,
   "Beige"
};

CV_PossibleValue_t Color_cons_t[]={{0,NULL},{1,NULL},{2,NULL},{3,NULL},
                                   {4,NULL},{5,NULL},{6,NULL},{7,NULL},
                                   {8,NULL},{9,NULL},{10,NULL},{0,NULL}};

// [WDJ] Preparation for these skin tables being read from files.

typedef struct {
  byte  num_using_ramp1;      // number of trans using ramp1
  byte  skin_ramp_colornum1;  // ramp1 start
  byte  skin_ramp_colornum2;  // ramp2 start
} skin_trans_entry_t;
  
typedef struct {
   byte      range_start, range_end; // translate in this range
   skin_trans_entry_t  skin[NUMSKINCOLORS];  // skin translation
} skin_trans_desc_t;

skin_trans_desc_t  doom_skins = 
{
   0x70, 0x7f,       // range of original green
 {
  { 16, 0x60, 0x60}, // gray
  { 16, 0x40, 0x40}, // brown
  { 16, 0x20, 0x20}, // red
  { 16, 0x58, 0x58}, // light gray
  { 16, 0x38, 0x38}, // light brown
  { 16, 0xb0, 0xb0}, // light red
  { 16, 0xc0, 0xc0}, // light blue
  {  9, 0xc7, 0xf0}, // dark blue
  {  8, 0xe0, 0xa0}, // yellow
  { 16, 0x80, 0x80}  // beige
 }
};

skin_trans_desc_t  heretic_skins = 
{
   225, 240,	// range of original player color
 {
  { 15, 0, 0}, // gray
  { 15, 67, 67}, // brown
  { 15, 145, 145}, // red
  { 15, 9, 9}, // light gray
  { 15, 74, 74}, // light brown
  { 15, 150, 150}, // light red
  { 15, 192, 192}, // light blue
  { 15, 185, 185}, // dark blue
  { 15, 114, 114}, // yellow
  { 15, 95, 95}  // beige
 }
};

//  Creates the translation tables to map the green color ramp to
//  another ramp (gray, brown, red, ...)
//
//  This is precalculated for drawing the player sprites in the player's
//  chosen color
//
void R_Init_TranslationTables (void)
{
    skin_trans_desc_t  * skindesc = & doom_skins;
    int i;
    // Each translucent table is 256x256, has size 65536 = 0x10000.

    //added:11-01-98: load here the transparency lookup tables 'TINTTAB'
    // NOTE: the TINTTAB resource MUST BE aligned on 64k for the asm optimised
    //       (in other words, translucenttables pointer low word is 0)
    translucenttables = Z_MallocAlign (NUM_TRANSLUCENTTABLES*0x10000, PU_STATIC, 0, 16);

    // load in translucency tables
    if( gamemode == heretic )
    {
        skindesc = & heretic_skins; // skin translation desc
        W_ReadLump( W_GetNumForName("TINTTAB"), translucenttables );
        W_ReadLump( W_GetNumForName("TINTTAB"), translucenttables+0x10000 );
        W_ReadLump( W_GetNumForName("TINTTAB"), translucenttables+0x20000 );
        W_ReadLump( W_GetNumForName("TINTTAB"), translucenttables+0x30000 );
        W_ReadLump( W_GetNumForName("TINTTAB"), translucenttables+0x40000 );
    }
    else
    {
        skindesc = & doom_skins;  // skin translation desc
        W_ReadLump( W_GetNumForName("TRANSMED"), translucenttables );
        W_ReadLump( W_GetNumForName("TRANSMOR"), translucenttables+0x10000 );
        W_ReadLump( W_GetNumForName("TRANSHI"),  translucenttables+0x20000 );
        W_ReadLump( W_GetNumForName("TRANSFIR"), translucenttables+0x30000 );
        W_ReadLump( W_GetNumForName("TRANSFX1"), translucenttables+0x40000 );
    }

    // no translate table for color 0
    skintranstables = Z_MallocAlign (256*(NUMSKINCOLORS-1), PU_STATIC, 0, 8);

    // [WDJ] skin desc based skin translation generation
    int sk;
    for (sk = 1; sk<NUMSKINCOLORS; sk++)
    {
        // sk=0 is original skin, and does not appear in translation tables
        byte * trantab = SKIN_TO_SKINMAP( sk );
        skin_trans_entry_t * skintr = & skindesc->skin[sk-1]; // skins 1..
        for (i=0 ; i<256 ; i++)
        {
            byte newcolor = i; // default is to keep the color the same
            if ( i >= skindesc->range_start && i <= skindesc->range_end )
            {
                int ri = i - skindesc->range_start; // ramp index
                // new color is color_start + ramp index
                newcolor = ( ri < skintr->num_using_ramp1 )?
                    skintr->skin_ramp_colornum1
                    : (skintr->skin_ramp_colornum2 - skintr->num_using_ramp1);
                newcolor += ri;
            }
            trantab[i] = newcolor;
        }
    }
}

// Changes in drawmode
void  R_Setup_Drawmode( void )
{
    R_PrecacheLevel();
    R_rdata_setup_rendermode();
}


// ==========================================================================
//               COMMON DRAWER FOR 8 AND 16 BIT COLOR MODES
// ==========================================================================

// in a perfect world, all routines would be compatible for either mode,
// and optimised enough
//
// in reality, the few routines that can work for either mode, are
// put here


// R_Init_ViewBuffer
// Creates lookup tables for getting the framebuffer address
// of a pixel to draw.
//
void R_Init_ViewBuffer ( int width, int height )
{
    // ViewBuffer may be smaller than video or screen buffers
    int  bytesperpixel = vid.bytepp;  // smaller code
    int  i;

    if (bytesperpixel<1 || bytesperpixel>4)
    {
        I_Error ("R_Init_ViewBuffer : Invalid bytesperpixel value %d\n",
                 bytesperpixel);
    }

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.
    view_window_x = (vid.width - width) >> 1;  // centered

    // Column offset for those columns of the view window, but
    // relative to the entire screen
    for (i=0 ; i<width ; i++)
        columnofs[i] = (view_window_x + i) * bytesperpixel;

    // Same with base row offset.
    if (width == vid.width)
        view_window_y = 0;
    else
        view_window_y = (vid.height - stbar_height - height) >> 1;

    // [WDJ] Table fixed for all bpp, bytepp, and padding
    // Precalculate all row offsets for screen[0] buffer.
    for (i=0 ; i<height ; i++)
    {
        ylookup[i] = ylookup1[i] = vid.display + (i+view_window_y)*vid.ybytes;
                     ylookup2[i] = vid.display + (i+(vid.height>>1))*vid.ybytes; // for splitscreen
    }
        

#ifdef HORIZONTALDRAW
    //Fab 17-06-98
    // create similar lookup tables for horizontal column draw optimisation
    // [WDJ] assumes screen buffer is width x height, not padded
    // This is not directly compatible with any screen buffer

    // (the first column is the bottom line)
    for (i=0; i<width; i++)
        yhlookup[i] = screens[2] + ((width-i-1) * bytesperpixel * height);

    for (i=0; i<height; i++)
        hcolumnofs[i] = i * bytesperpixel;
#endif
}


//
// Store the lumpnumber of the viewborder patches.
//
lumpnum_t  viewborderlump[8];

void R_Init_ViewBorder (void)
{
    if( EN_heretic_hexen )
    {   // Heretic, Hexen
        viewborderlump[BRDR_T]  = W_GetNumForName ("bordt");
        viewborderlump[BRDR_B]  = W_GetNumForName ("bordb");
        viewborderlump[BRDR_L]  = W_GetNumForName ("bordl");
        viewborderlump[BRDR_R]  = W_GetNumForName ("bordr");
        viewborderlump[BRDR_TL] = W_GetNumForName ("bordtl");
        viewborderlump[BRDR_BL] = W_GetNumForName ("bordbl");
        viewborderlump[BRDR_TR] = W_GetNumForName ("bordtr");
        viewborderlump[BRDR_BR] = W_GetNumForName ("bordbr");
    }
    else
    {   // Doom
        viewborderlump[BRDR_T]  = W_GetNumForName ("brdr_t");
        viewborderlump[BRDR_B]  = W_GetNumForName ("brdr_b");
        viewborderlump[BRDR_L]  = W_GetNumForName ("brdr_l");
        viewborderlump[BRDR_R]  = W_GetNumForName ("brdr_r");
        viewborderlump[BRDR_TL] = W_GetNumForName ("brdr_tl");
        viewborderlump[BRDR_BL] = W_GetNumForName ("brdr_bl");
        viewborderlump[BRDR_TR] = W_GetNumForName ("brdr_tr");
        viewborderlump[BRDR_BR] = W_GetNumForName ("brdr_br");
    }
}


//
// R_FillBackScreen
// Fills the back screen with a pattern for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void)
{
    patch_t*    patch;
    byte*       src;
    byte*       dest;  // within video buffer
    int         x, y;
    int         step,boff; 
    
    //faB: quickfix, don't cache lumps in both modes
    if( rendermode != render_soft )
        return;

     //added:08-01-98:draw pattern around the status bar too (when hires),
    //                so return only when in full-screen without status bar.
    if ((rdraw_scaledviewwidth == vid.width) && (rdraw_viewheight == vid.height))
        return;

    // draw pattern around the status bar
    src  = scr_borderflat;
    dest = screens[1];  // background buffer

    // [WDJ] Draw for all bpp, bytepp, and padding
    for (y=0 ; y<vid.height ; y++)
    {
        // repeatly draw a 64 pixel wide flat
        dest = screens[1] + (y * vid.ybytes);  // within screen buffer
        for (x=0 ; x<(vid.width/64) ; x++)
        {
//            memcpy (dest, src+((y&63)<<6), 64);
            V_DrawPixels( dest, 0, 64, &src[(y & 63) << 6]);
            dest += (64 * vid.bytepp);
        }

        if (vid.width&63)
        {
//            memcpy (dest, src+((y&63)<<6), vid.width&63);
            V_DrawPixels( dest, 0, 64, &src[(y & 63) << 6]);
        }
    }

    //added:08-01-98:dont draw the borders when viewwidth is full vid.width.
    if (rdraw_scaledviewwidth == vid.width)
       return;

    // view window borders
    if( EN_heretic )
    {
        step = 16;
        boff = 4; // borderoffset
    }
    else
    {
        step = 8;
        boff = 8;
    }

    // Draw to screen1
    // top
    patch = W_CacheLumpNum (viewborderlump[BRDR_T],PU_CACHE);
    for (x=view_window_x; x < (view_window_x + rdraw_scaledviewwidth); x+=step)
        V_DrawPatch (x, view_window_y - boff, 1, patch);
    // bottom
    patch = W_CacheLumpNum (viewborderlump[BRDR_B],PU_CACHE);
    for (x=view_window_x; x < (view_window_x + rdraw_scaledviewwidth); x+=step)
        V_DrawPatch (x, view_window_y + rdraw_viewheight, 1, patch);
    patch = W_CacheLumpNum (viewborderlump[BRDR_L],PU_CACHE);
    // Vertical edge is not an even multiple, so draw last aligned.
    // left
    for (y=view_window_y; y < (view_window_y + rdraw_viewheight - step); y+=step)
        V_DrawPatch (view_window_x - boff, y, 1, patch);
    V_DrawPatch (view_window_x - boff, (view_window_y + rdraw_viewheight - step), 1, patch);
    // right
    patch = W_CacheLumpNum (viewborderlump[BRDR_R],PU_CACHE);
    for (y=view_window_y; y < (view_window_y + rdraw_viewheight - step); y+=step)
        V_DrawPatch (view_window_x + rdraw_scaledviewwidth, y, 1, patch);
    V_DrawPatch (view_window_x + rdraw_scaledviewwidth, (view_window_y + rdraw_viewheight - step), 1, patch);

    // Draw beveled corners.
    V_DrawPatch (view_window_x - boff,
                 view_window_y - boff,
                 1,
                 W_CacheLumpNum (viewborderlump[BRDR_TL],PU_CACHE));

    V_DrawPatch (view_window_x + rdraw_scaledviewwidth,
                 view_window_y - boff,
                 1,
                 W_CacheLumpNum (viewborderlump[BRDR_TR],PU_CACHE));

    V_DrawPatch (view_window_x - boff,
                 view_window_y + rdraw_viewheight,
                 1,
                 W_CacheLumpNum (viewborderlump[BRDR_BL],PU_CACHE));

    V_DrawPatch (view_window_x + rdraw_scaledviewwidth,
                 view_window_y + rdraw_viewheight,
                 1,
                 W_CacheLumpNum (viewborderlump[BRDR_BR],PU_CACHE));
}


//
// Copy a screen buffer.
//
void R_VideoErase (unsigned ofs, int count)
{
    // LFB copy.
    // This might not be a good idea if memcpy is not optimal,
    //  e.g. byte by byte on a 32bit CPU, as GNU GCC/Linux libc did
    //  at one point.
    memcpy (screens[0]+ofs, screens[1]+ofs, count);
}


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void R_DrawViewBorder (void)
{
    int  top_brdr;  // height of top border
    int  topbytes;
    int  side_brdr;  // side border
    int  ofs;

#ifdef HWRENDER // not win32 only 19990829 by Kin
    if (rendermode != render_soft)
    {
        HWR_DrawViewBorder (0);  // 0 means draw all
        return;
    }
#endif


#ifdef DEBUG
    debug_Printf("RDVB: vidwidth %d vidheight %d rdraw_scaledviewwidth %d rdraw_viewheight %d\n",
             vid.width,vid.height,rdraw_scaledviewwidth,rdraw_viewheight);
#endif

     //added:08-01-98: draw the backtile pattern around the status bar too
    //                 (when statusbar width is shorter than vid.width)
    /*
    if( (vid.width>ST_WIDTH) && (vid.height!=rdraw_viewheight) )
    {
        ofs  = (vid.height - stbar_height) * vid.ybytes;
        side_brdr = (vid.width-ST_WIDTH)>>1;
        R_VideoErase(ofs,side_brdr);

        ofs += (vid.width-side_brdr);
        for (i=1;i<stbar_height;i++)
        {
            R_VideoErase(ofs,side_brdr<<1);  //wraparound right to left border
            ofs += vid.width;
        }
        R_VideoErase(ofs,side_brdr);
    }*/

    if (rdraw_scaledviewwidth == vid.width)
        return;
   
    // rdraw_viewheight is the height of the window within the border
    // draw view border
    top_brdr  = (vid.height - stbar_height - rdraw_viewheight) >>1;
    topbytes = top_brdr * vid.ybytes;
    side_brdr = (vid.width - rdraw_scaledviewwidth) >>1;

    // copy background to display screen
    // [WDJ] cannot wrap around because some video cards pad the video buffer
    // copy top
    R_VideoErase (0, topbytes);

    // copy bottom
    R_VideoErase ((rdraw_viewheight + top_brdr)*vid.ybytes, topbytes);

    //added:05-02-98:simpler using our new VID_Blit routine
    // copy left side
    VID_BlitLinearScreen(screens[1]+topbytes, screens[0]+topbytes,
                         side_brdr * vid.bytepp, rdraw_viewheight, vid.ybytes, vid.ybytes);

    // copy right side
    ofs = topbytes + ((vid.width - side_brdr)*vid.bytepp);
    VID_BlitLinearScreen(screens[1]+ofs, screens[0]+ofs,
                         side_brdr * vid.bytepp, rdraw_viewheight, vid.ybytes, vid.ybytes);

#ifdef DIRTY_RECT
    // useless, old dirty rectangle stuff
    //V_MarkRect (0,0,vid.width, vid.height - stbar_height);
#endif
}

// SoM: This is for 3D floors that cast shadows on walls.
// This function just cuts the column up into sections and calls
// R_DrawColumn
void R_DrawColumnShadowed(void)
{
    int count;
//    int realyh, realyl;
    int realyh;
    int i;
    int height, bheight = 0;
    int solid = 0;

    realyh = dc_yh;
//    realyl = dc_yl;

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
        return;

#ifdef RANGECHECK
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
        // anyway because the lighting of the top should be affected.
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
        basecolfunc();  // R_DrawColumn_x
        if (solid)
            dc_yl = bheight;
        else
            dc_yl = dc_yh + 1;

        dc_colormap = dc_lightlist[i].rcolormap;
    }
    dc_yh = realyh;
    if (dc_yl <= realyh)
        basecolfunc();  // R_DrawColumn_x
}


// ==========================================================================
//                   INCLUDE 8bpp DRAWING CODE HERE
// ==========================================================================

#include "r_draw8.c"


// ==========================================================================
//                   INCLUDE 16bpp DRAWING CODE HERE
// ==========================================================================

#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
#include "r_draw16.c"
#endif


// ==========================================================================
//                   INCLUDE 24bpp DRAWING CODE HERE
// ==========================================================================

#ifdef ENABLE_DRAW24
#include "r_draw24.c"
#endif


// ==========================================================================
//                   INCLUDE 32bpp DRAWING CODE HERE
// ==========================================================================

#ifdef ENABLE_DRAW32
#include "r_draw32.c"
#endif

