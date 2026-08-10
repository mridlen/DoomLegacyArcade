// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: screen.h 1656 2023-12-08 14:54:47Z wesleyjohnson $
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
// $Log: screen.h,v $
// Revision 1.14  2004/05/16 20:34:47  hurdler
//
// Revision 1.13  2004/05/16 19:11:44  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.12  2003/05/30 22:44:08  hurdler
// add checkcvar function to FS
//
// Revision 1.11  2003/05/04 04:16:25  sburke
// Allow 1280x1024 for full-screen video on Solaris.
//
// Revision 1.10  2002/11/12 00:06:05  ssntails
// Support for translated translucent columns in software mode.
//
// Revision 1.9  2001/05/16 21:21:14  bpereira
//
// Revision 1.8  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.7  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.6  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.5  2000/11/02 19:49:37  bpereira
// Revision 1.4  2000/08/31 14:30:56  bpereira
//
// Revision 1.3  2000/04/22 20:27:35  metzgermeister
// support for immediate fullscreen switching
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------


#ifndef SCREEN_H
#define SCREEN_H

#include "doomdef.h"
  // HORIZONTALDRAW
#include "doomtype.h"
#include "command.h"
  // consvar_t
#include "i_video.h"
  // modenum_t


// Size of statusbar.
#define ST_HEIGHT    32
#define ST_WIDTH     320
// Heretic, status bar height at bottom of screen.
#define H_STBAR_HEIGHT   42


extern int stbar_height;   // status bar, may be scaled

//added:20-01-98: used now as a maximum video mode size for extra vesa modes.

// we try to re-allocate a minimum of buffers for stability of the memory,
// so all the small-enough tables based on screen size, are allocated once
// and for all at the maximum size.
#define MAXVIDWIDTH    1600
#define MAXVIDHEIGHT   1200

#define BASEVIDWIDTH    320   //NEVER CHANGE THIS! this is the original
#define BASEVIDHEIGHT   200  // resolution of the graphics.


// WDJ 2012-2-6, provide structure to complete the draw capability
// optimized for test speed, array indexing
typedef enum {
  DRAW8PAL, DRAW15, DRAW16, DRAW24, DRAW32, DRAWGL
} sw_drawmode_e;


// WDJ 2012-2-6, Provide structure and isolation of driver problems.
// Mac has padded video buffers, some drivers can provide direct access and
// some cannot, some have 15, 24, or 32 bpp drawing.
// [WDJ] do not need union, minimal wastage, do not make access difficult
// global video state
typedef struct viddef_s
{
    // If hardware allows direct access then display could be video memory,
    // but usually is in buffer allocation.  Decided by driver.
    // Horz. scanlines could be padded (Mac).
    // Screens have same size as display, and same padding.
    byte      * display;         // buffer that gets displayed, screen[0]
    byte      * screen1;         // allocated memory screen[1]

 // Display and screen drawing with correct padding.
 // Sometimes is not the same as direct_size.
 // Use these, they are corrected for padded scanlines and multibyte pixels
    unsigned int screen_size;    // screens and display
 // [WDJ] These are unsigned values, kept in signed vars.
 // When unsigned vars are 64 bit multiplied with signed offsets,
 // it gives large unsigned results, which leads to segfault.
 // Keep these as signed to prevent sign errors when they are multiplied
 // by signed parameters.
    int   widthbytes;     // width * bytepp, to save multiplies
    int   ybytes;         // addr line = & display[ y * ybytes ]

 // basic video mode attributes
 // [WDJ] signed width, height for easy math, (draw errors if unsigned)
    int         width;           // PIXELS per scanline
    int         height;
    int         bytepp;          // BYTES per pixel: 1=256color, 2, 4
    modenum_t   modenum;         // vidmode num, same as setmodeneeded
    byte        drawmode;        // drawing mode, optimized for tables and switch stmts, drawmode_e
    byte        bitpp;		 // BITS per pixel: 8, 15, 16, 24, 32
    byte        numpages;        // always 1, PAGE FLIPPING TODO!!!
//    byte        windowed;        // windowed or fullscreen mode ?
    byte        fullscreen;      // windowed or fullscreen mode ?
    byte        recalc;          // if true, recalc vid-based stuff
    byte        draw_ready;      // true after vid and buffers are setup
 // special uses
#ifdef FIT_RATIO 
    uint16_t    fit_width, fit_height;  // scale draw to fit these
#endif
    int         dupx,dupy;       // scale 1,2,3 value for menus & overlays
    float       fdupx,fdupy;     // same as dupx,dupy but exact value when aspect ratio isn't 320/200
    float       fx_center,fy_center; // half width height
    float       fx_scale2,fy_scale2; // 2.0 / width, 2.0 / height
    int         centerofs;       // centering for the scaled menu gfx

    //int         baseratio;       // SoM: Used to get the correct value for lighting walls //Hurdler: not used anymore
 // PRIVATE TO DRIVER
    byte *       buffer;         // allocated invisible buffers
    byte *       direct;         // linear frame buffer, or vga base mem.
    unsigned int direct_rowbytes; // bytes per scanline of the VIDEO mode
    unsigned int direct_size;    // correct size for copy (up to 6400x4800x64)
 // END PRIVATE
} viddef_t;



// internal additional info for vesa modes only
typedef struct {
    int         vesamode;         // vesa mode number plus LINEAR_MODE bit
    void        *linearmem;      // linear address of start of frame buffer
} vesa_extra_t;

// a video modes from the video modes list,
typedef struct vmode_s  vmode_t;
typedef struct vmode_s {

    vmode_t    * next;
    char       * name;
    unsigned int width;
    unsigned int height;
    unsigned int rowbytes;          //bytes per scanline
    byte         bytesperpixel;     // 1 for 256c, 2 for highcolor
    byte         modetype;          // from modetype_e
    byte         numpages;
    vesa_extra_t *extradata;       //vesa mode extra data
    int          (*setmode_func)(viddef_t *lvid, struct vmode_s *pcurrentmode);
    int          misc;              //misc for display driver (r_glide.dll etc)
} vmode_t;


// [WDJ] definitions for 15bpp, 16bpp, 24bpp, 32bpp

typedef struct {
#ifdef __BIG_ENDIAN__
   byte r, g, b;
#else
   byte b, g, r;
#endif
} pixel24_t;

typedef struct {
#ifdef __BIG_ENDIAN__
   byte alpha, r, g, b;
#else
   byte b, g, r, alpha;
#endif
} pixel32_t;

typedef union {
   uint32_t      ui32;
   pixel24_t     pix24;  // for speed, does not line up with pix32
   struct {
#ifdef __BIG_ENDIAN__
      byte unused;
      pixel24_t  rgb24;
#else
      pixel24_t  rgb24;  // aligns with pix32, rgb
      byte unused;
#endif
   } s24;
   pixel32_t     pix32;
} pixelunion32_t;

// hicolor masks  15 bit / 16 bit
extern uint16_t mask_01111, mask_01110, mask_11110, mask_11100, mask_11000;
extern uint16_t mask_r, mask_g, mask_b, mask_rb;
extern byte     shift_r, shift_g, shift_b;

// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

extern void     (*skycolfunc) (void);
extern void     (*colfunc) (void);
#ifdef HORIZONTALDRAW
extern void     (*hcolfunc) (void);    //Fab 17-06-98
#endif
extern void     (*basecolfunc) (void);
extern void     (*fuzzcolfunc) (void);  // fuzzy
extern void     (*skincolfunc) (void);  // skin translated
extern void     (*transcolfunc) (void);  // translucent
extern void     (*skintranscolfunc) (void); // SSNTails 11-11-2002
extern void     (*shadecolfunc) (void);
extern void     (*fogcolfunc) (void);
#ifdef ENABLE_DRAW_ALPHA
extern void     (*alpha_colfunc) (void);
#endif
extern void     (*spanfunc) (void);
extern void     (*basespanfunc) (void);
extern void     (*fogspanfunc) (void);
extern void     (*transspanfunc) (void);


// ----------------
// screen variables
// ----------------
extern viddef_t vid;

// mode numbers
extern modenum_t  setmodeneeded;   // mode number to set
// mode index 0 is NULL, when passed does default (INITIAL_WINDOW_WIDTH)

extern byte*  scr_borderflat;  // flat used to fill the view borders

extern consvar_t cv_scr_width;
extern consvar_t cv_scr_height;
extern consvar_t cv_scr_depth;
extern consvar_t cv_fullscreen;

extern consvar_t cv_fuzzymode;

// quick fix for tall/short skies, depending on bytesperpixel
extern void (*skydrawerfunc[2]) (void);

#ifdef PARNOIA
void SCR_Set_dummy_draw(void);
#endif

// Change video mode, only at the start of a refresh.
void SCR_SetMode( byte change_flag );
// Recalc screen size dependent stuff
void SCR_Recalc (void);
// Apply config or command line settings.
void SCR_apply_video_settings( byte calc_setmodeneeded );
// Set the mode number which is saved in the config
void SCR_SetDefaultMode (void);

void SCR_Startup (void);

void SCR_ChangeFullscreen (void);

#endif // SCREEN_H
