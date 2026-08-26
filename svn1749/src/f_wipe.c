// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: f_wipe.c 1201 2015-12-26 19:21:12Z wesleyjohnson $
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
// $Log: f_wipe.c,v $
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"

#include "r_data.h"
   // TRANSLU_TABLE
#include "r_draw.h"
   // translucenttables
#include "z_zone.h"
#include "m_random.h"
#include "f_wipe.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------

// when zero, stop the wipe
static boolean  go = 0;

static byte*    wipe_scr_start;
static byte*    wipe_scr_end;
static byte*    wipe_scr;


#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 ) || defined( ENABLE_DRAW24 ) || defined( ENABLE_DRAW32 )
#define ENABLE_DRAWEXT
static uint16_t  mask1 = 0, mask2 = 0;
#endif
static int fadecnt;

// [Arcade] Geometry of the three buffers the wipe works on.
//
// The wipe used to read vid.* directly, which only describes the software
// renderer's framebuffer.  Under OpenGL those fields say nothing useful --
// vid.screen_size is not even set -- so the wipe now carries its own
// geometry, filled from vid.* for a software wipe and from the 24 bit RGB
// capture format for a hardware one.  Everything below works off these.
static int  wipe_width, wipe_height;
static int  wipe_bytepp;       // bytes per pixel
static int  wipe_ybytes;       // bytes per row, including any padding
static int  wipe_widthbytes;   // bytes of actual picture per row
static int  wipe_screen_size;  // bytes of the whole buffer
static int  wipe_dupy;         // vertical scale, keeps melt speed the same
                               // at any resolution
static boolean  wipe_wide_pixel;  // pixel is more than one byte

#ifdef HWRENDER
// True while wiping under a hardware renderer.
static boolean  wipe_hardware = false;
// Hardware wipes cannot borrow screens[]: those are the software renderer's
// buffers and are not sized for this.  Three whole screens of 24 bit RGB,
// held only for the duration of one wipe.
static byte *   hw_wipe_buf[3] = { NULL, NULL, NULL };

static
void wipe_hw_free( void )
{
    int i;
    for( i=0; i<3; i++ )
    {
        if( hw_wipe_buf[i] )
        {
            free( hw_wipe_buf[i] );
            hw_wipe_buf[i] = NULL;
        }
    }
}
#endif


// Set the buffer geometry for the wipe that is about to start.
static
void wipe_set_geometry( void )
{
    wipe_width  = vid.width;
    wipe_height = vid.height;

    // The melt steps in 320x200 units, so scale it to keep the same apparent
    // speed at any resolution.  vid.dupy is not guaranteed meaningful under
    // a hardware renderer, hence the fallback.
    wipe_dupy = vid.dupy;
    if( wipe_dupy < 1 )
        wipe_dupy = vid.height / 200;
    if( wipe_dupy < 1 )
        wipe_dupy = 1;

#ifdef HWRENDER
    if( wipe_hardware )
    {
        // 24 bit RGB, tightly packed, top down.
        wipe_bytepp = 3;
        wipe_widthbytes = wipe_ybytes = wipe_width * 3;
        wipe_screen_size = wipe_ybytes * wipe_height;
        wipe_wide_pixel = true;
        return;
    }
#endif

    wipe_bytepp = vid.bytepp;
    wipe_ybytes = vid.ybytes;
    wipe_widthbytes = vid.widthbytes;
    wipe_screen_size = vid.screen_size;
#ifdef ENABLE_DRAWEXT
    wipe_wide_pixel = ( vid.drawmode != DRAW8PAL );
#else
    wipe_wide_pixel = false;
#endif
}


// Can a wipe run at all in the current render mode?
// Called by D_Display before it bothers capturing anything.
boolean wipe_Available( void )
{
#ifdef HWRENDER
    if( rendermode != render_soft )
        return HWR_Wipe_Supported();
#endif
    return true;
}

static
void wipe_initColorXForm ( void )
{
    // vid : from video setup

//    memcpy(wipe_scr, wipe_scr_start, width*height*vid.bytepp);
    // copy wipe_scr_start to wipe_scr
    VID_BlitLinearScreen( wipe_scr_start, wipe_scr,
			  wipe_widthbytes, wipe_height,
			  wipe_ybytes, wipe_ybytes );
#ifdef HWRENDER
    if( wipe_hardware )
    {
        fadecnt = 0;
        return;
    }
#endif
#ifdef ENABLE_DRAWEXT
    switch( vid.drawmode )
    {
     case DRAW15:
        mask1 = 0x7C1F;
        break;
     case DRAW16:
        mask1 = 0xF81F;
        break;
     default:
        mask1 = 0x00FF;
        break;
    }
    mask2 = ~mask1;
    fadecnt = 4;
#endif
}

/* BP:the original one, work only in hicolor
static
int wipe_doColorXForm ( int width,  int height,  int ticks )

{
    boolean     changed;
    byte*       w;
    byte*       e;
    int         newval;

    changed = false;
    w = wipe_scr;
    e = wipe_scr_end;

    while (w!=wipe_scr+width*height)
    {
        if (*w != *e)
        {
            if (*w > *e)
            {
                newval = *w - ticks;
                if (newval < *e)
                    *w = *e;
                else
                    *w = newval;
                changed = true;
            }
            else if (*w < *e)
            {
                newval = *w + ticks;
                if (newval > *e)
                    *w = *e;
                else
                    *w = newval;
                changed = true;
            }
        }
        w++;
        e++;
    }

    return !changed;

}
*/

#ifdef HWRENDER
// [Arcade] Crossfade for the hardware path.  The screens here are 24 bit RGB,
// where the packed 15/16 bit odd/even field trick used below does not apply,
// so this is a straight per channel interpolation from the start screen to
// the end screen over 16 steps.
static
int wipe_doColorXForm_hw ( int ticks )
{
    byte * w = wipe_scr;
    byte * s = wipe_scr_start;
    byte * e = wipe_scr_end;
    int  n = wipe_screen_size;
    int  fade1, fade2;

    fadecnt += ticks;
    if( fadecnt >= 16 )
    {
        // Land exactly on the end screen, then report done.
        memcpy( w, e, wipe_screen_size );
        return true;
    }

    fade2 = fadecnt;
    fade1 = 16 - fade2;
    // Always interpolate from the original start screen rather than stepping
    // the working screen toward the end, so rounding cannot accumulate.
    while( n-- )
        *(w++) = (byte)(( ((int)(*(s++)) * fade1) + ((int)(*(e++)) * fade2) ) >> 4);

    return false;
}
#endif


// repeated until returns done
static
int wipe_doColorXForm ( int ticks )

{
    // vid : from video setup
    static int  slowdown=0;

#ifdef HWRENDER
    if( wipe_hardware )
        return wipe_doColorXForm_hw( ticks );
#endif
    boolean     changed = false;
    int y;
#ifdef ENABLE_DRAWEXT
    int fade1 = 0, fade2 = 0;
    unsigned int  mask1_shftd = 0, mask2_shftd = 0;
#endif
    
    byte* wend;
    byte* w;
    byte* e;
   
    while(ticks--)
    {
      // [WDJ] Fade for all bpp, bytepp, and padding
#ifdef ENABLE_DRAWEXT
      if( vid.drawmode != DRAW8PAL )
      {
	if( fadecnt++ > 16 ) break;  // DONE, changed = false
	// proportional fade, multiply odd and even fields separately
	// Smallest field is 5 bits, so limit shift to 4, otherwise multiply will bleed into next field.
	fade2 = fadecnt;
	fade1 = 16-fade2;
	changed = true;
	mask1_shftd = mask1 << 4;
	mask2_shftd = mask2 << 4;
      }
      else
#endif
      {
	// slowdown the 4 step palette fade
	if(slowdown++) { slowdown=0;  return false; }
      }

      for( y=0; y<vid.screen_size; y+=vid.ybytes )
      {
	e = wipe_scr_end + y;
	w = wipe_scr + y;
	wend = w + vid.widthbytes;  // end of line

#ifdef ENABLE_DRAWEXT
	if( vid.drawmode != DRAW8PAL )
        {
	  while( w < wend )
	  {
	    register unsigned int w16 = *(uint16_t*)w;
	    register unsigned int e16 = *(uint16_t*)e;
 	    register unsigned int b0 = ((w16&mask1)*fade1) + ((e16&mask1)*fade2);
	    register unsigned int b1 = ((w16&mask2)*fade1) + ((e16&mask2)*fade2);
	    *(uint16_t*)w = ( (b0&mask1_shftd) | (b1&mask2_shftd) ) >> 4;
	    w+=2;
	    e+=2;
	  }
	}
        else
#endif
	{
	  // Traditional for 8bpp
	  while( w < wend )
	  {
            if (*w != *e)
            {
	        register byte newval;
                if((newval=translucenttables[TRANSLU_TABLE_more+(*e<<8)+*w])==*w)
                    if((newval=translucenttables[TRANSLU_TABLE_med+(*e<<8)+*w])==*w)
                        if((newval=translucenttables[TRANSLU_TABLE_more+(*w<<8)+*e])==*w)
                            newval=*e;
                *w=newval;
                changed = true;
            }
	    w++;
	    e++;
	  }
	}
      }
    }
    return !changed;
}

static
void wipe_exitColorXForm ( void )
{
}


static int*  melty;  // y indexes for melt


static
void wipe_initMelt ( void )
{
    int i, my;
    int meltwidth = wipe_width/2;  // melt is 2 pixels at a time

    // [Arcade] D_Display gives up on a wipe after 2 seconds, which leaves the
    // previous one un-exited and its melty[] still allocated.  Reclaim it
    // rather than leaking, and never inherit stale column positions.
    if( melty )
        Z_Free( melty );

    // copy start screen to main screen
//    memcpy(wipe_scr, wipe_scr_start, width*height*scr_bytepp);
    VID_BlitLinearScreen( wipe_scr_start, wipe_scr,
			  wipe_widthbytes, wipe_height,
			  wipe_ybytes, wipe_ybytes );

    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    melty = (int *) Z_Malloc(meltwidth*sizeof(int), PU_STATIC, 0);
    my = melty[0] = -(M_Random()%16);  // set neg numbers as delay for a column
    for (i=1;i<meltwidth;i++)
    {
        my += (M_Random()%3) - 1;
        if (my > 0) my = 0;  // start immediately
        else if (my <= -16) my = -15;  // max delay
        // dup to keep normal speed in high res screens
        melty[i] = my * wipe_dupy;
    }
}


static
int wipe_doMelt ( int ticks )
{
    boolean  done = true;
    int  cpycnt = wipe_bytepp + wipe_bytepp;  // 2 pixels
    int  meltwidth = wipe_width/2;  // melt is 2 pixels at a time
    int  height = wipe_height;
    int  i, j;
    int  dy;

    byte *s, *e, *d;

    // [WDJ] Melt for all bpp, bytepp, and padding
    while (ticks--)
    {
        for (i=0;i<meltwidth;i++)
        {
            if (melty[i]<0)  // delay
            {
                melty[i]++; done = false;
            }
            else if (melty[i] < height)  // moving
            {
                dy = (melty[i] < 16) ? melty[i]+1 : 8;
                dy *= wipe_dupy;
                if (melty[i]+dy >= height) dy = height - melty[i];  // bottom
	        int idx = ((i+i)*wipe_bytepp);  // x offset only
                s = &wipe_scr_start[idx];
	        idx += (melty[i]*wipe_ybytes);  // with melty offset
                d = &wipe_scr[idx];
                e = &wipe_scr_end[idx];
                melty[i] += dy;
                // [Arcade] Was a compile time ENABLE_DRAWEXT choice on
                // vid.drawmode.  Both paths are now always built and picked at
                // run time, because a hardware wipe is 3 bytes per pixel
                // whatever the software renderer was configured for.
	        if( wipe_wide_pixel )
	        {
		    // copy end screen over newly exposed dy area
		    for (j=dy;j;j--)
		    {
		        memcpy(d, e, cpycnt);  // 2 pixels
		        e += wipe_ybytes;
		        d += wipe_ybytes;
		    }
		    // redraw start screen columns shifted down by melty[i]
		    for (j=height-melty[i];j;j--)
		    {
		        memcpy(d, s, cpycnt);  // 2 pixels
		        s += wipe_ybytes;
		        d += wipe_ybytes;
		    }
		}
	        else
	        {
		    // Simpler, faster for older slow machines
		    // copy end screen over newly exposed dy area
		    for (j=dy;j;j--)
		    {
		        *(uint16_t*)d = *(uint16_t*)e;  // 2 pixels
		        e += wipe_ybytes;
		        d += wipe_ybytes;
		    }
		    // redraw start screen columns shifted down by melty[i]
		    for (j=height-melty[i];j;j--)
		    {
		        *(uint16_t*)d = *(uint16_t*)s;  // 2 pixels
		        s += wipe_ybytes;
		        d += wipe_ybytes;
		    }
		}
                done = false;
            }
        }
    }

    return done;
}


static
void wipe_exitMelt ( void )
{
    Z_Free(melty);
    melty = NULL;
}


//  save the 'before' screen of the wipe (the one that melts down)
//
// [WDJ] always full copy
int wipe_StartScreen ( void )
{
    // [Arcade] A wipe that hit D_Display's 2 second timeout never finished and
    // left go set, which would make the next one skip its init and run on the
    // previous wipe's freed state.  A new capture always starts a new wipe.
    go = 0;

#ifdef HWRENDER
    wipe_hardware = ( rendermode != render_soft );
    if( wipe_hardware )
    {
        size_t bufsize;
        int i;

        wipe_set_geometry();
        bufsize = (size_t) wipe_screen_size;

        wipe_hw_free();  // defensive: a previous wipe that never ran
        for( i=0; i<3; i++ )
        {
            hw_wipe_buf[i] = malloc( bufsize );
            if( ! hw_wipe_buf[i] )
            {
                wipe_hw_free();
                return 1;  // no wipe, D_Display carries on without one
            }
        }
        wipe_scr_start = hw_wipe_buf[0];
        wipe_scr_end   = hw_wipe_buf[1];

        // [Arcade] Read the FRONT buffer, not the back one.  This runs at the
        // top of D_Display, immediately after the previous frame was swapped
        // to the monitor, and the contents of the back buffer at that moment
        // are undefined -- capturing it gives whatever stale frame the driver
        // left there.  The front buffer is the outgoing frame the melt is
        // supposed to slide away.
        HWR_Wipe_ReadScreen( wipe_scr_start, true );
        return 0;
    }
#endif

    wipe_set_geometry();
    wipe_scr_start = screens[2];
    I_ReadScreen(wipe_scr_start);  // copy vid.display in screen format
    return 0;
}


//  save the 'after' screen of the wipe (the one that show behind the melt)
//
// [WDJ] always full copy
int wipe_EndScreen ( void )
{
#ifdef HWRENDER
    if( wipe_hardware )
    {
        // The incoming frame has just been drawn and not yet swapped, so it
        // is the back buffer -- the normal read.
        HWR_Wipe_ReadScreen( wipe_scr_end, false );
        return 0;
    }
#endif

    // vid : from video setup
    wipe_scr_end = screens[3];
    I_ReadScreen(wipe_scr_end);  // copy vid.display in screen format
    // restore start scr.
//    V_CopyRect(x, y, 2, width, height, x, y, 0);  // screen[2] -> screen[0]
    VID_BlitLinearScreen(wipe_scr_start, screens[0], vid.width, vid.height, vid.ybytes, vid.ybytes);
    return 0;
}


// Wipe function tables, different parameters
static void (*wipes_init[])(void) =
{
    wipe_initColorXForm, // wipeno == wipe_ColorXForm
    wipe_initMelt,       // wipeno == wipe_Melt
};
static int (*wipes_do[])(int) =
{
    wipe_doColorXForm,  // wipeno == wipe_ColorXForm
    wipe_doMelt,        // wipeno == wipe_Melt
};
static void (*wipes_exit[])(void) =
{
    wipe_exitColorXForm, // wipeno == wipe_ColorXForm
    wipe_exitMelt        // wipeno == wipe_Melt
};

// Screen wipe is always full width and height.
// There is no use passing parameters for width, height, x, y,
// as the functions do not have such flexibility, and it would not be used.
int wipe_ScreenWipe( int wipeno, int ticks )
{
    int rc;

#ifdef DIRTY_RECT
    //Fab: obsolete (we don't use dirty-rectangles type of refresh)
    //void V_MarkRect(int, int, int, int);
#endif

    // initial stuff
    if (!go)
    {
        go = 1;
        // wipe_scr = (byte *) Z_Malloc(width*height*vid.bytepp, PU_STATIC, 0); // DEBUG
#ifdef HWRENDER
        // Software composes into the visible framebuffer, which I_FinishUpdate
        // then blits.  Hardware has no such buffer to write, so it composes
        // into its own and pushes the result over below.
        wipe_scr = wipe_hardware ? hw_wipe_buf[2] : screens[0];
#else
        wipe_scr = screens[0];
#endif
        (*wipes_init[wipeno])();
    }

    // do a piece of wipe-in
#ifdef DIRTY_RECT
    //V_MarkRect(0, 0, width, height);
#endif
    rc = (*wipes_do[wipeno])(ticks);
    //  V_CopyBlock(x, y, width, height, wipe_scr, screens[0]); // DEBUG

#ifdef HWRENDER
    if( wipe_hardware )
        HWR_Wipe_DrawScreen( wipe_scr );  // put this frame on the screen
#endif

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes_exit[wipeno])();
#ifdef HWRENDER
        if( wipe_hardware )
        {
            wipe_hw_free();
            wipe_scr = wipe_scr_start = wipe_scr_end = NULL;
        }
#endif
    }

    return !go;

}
