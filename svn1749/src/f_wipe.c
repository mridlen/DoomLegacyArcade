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
static int fadecnt;
static uint16_t  mask1 = 0, mask2 = 0;
#endif

static
void wipe_initColorXForm ( void )
{
    // vid : from video setup

//    memcpy(wipe_scr, wipe_scr_start, width*height*vid.bytepp);
    // copy wipe_scr_start to wipe_scr
    VID_BlitLinearScreen( wipe_scr_start, wipe_scr,
			  vid.widthbytes, vid.height,
			  vid.ybytes, vid.ybytes );
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

// repeated until returns done
static
int wipe_doColorXForm ( int ticks )

{
    // vid : from video setup
    static int  slowdown=0;
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
    // vid : from video setup
    int i, my;
    int meltwidth = vid.width/2;  // melt is 2 pixels at a time

    // copy start screen to main screen
//    memcpy(wipe_scr, wipe_scr_start, width*height*scr_bytepp);
    VID_BlitLinearScreen( wipe_scr_start, wipe_scr,
			  vid.widthbytes, vid.height,
			  vid.ybytes, vid.ybytes );

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
        melty[i] = my * vid.dupy;
    }
}


static
int wipe_doMelt ( int ticks )
{
    // vid : from video setup
    boolean  done = true;
#ifdef ENABLE_DRAWEXT
    int  cpycnt = vid.bytepp + vid.bytepp;  // 2 pixels
#endif
    int  meltwidth = vid.width/2;  // melt is 2 pixels at a time
    int  height = vid.height;
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
                dy *= vid.dupy;
                if (melty[i]+dy >= height) dy = height - melty[i];  // bottom
	        int idx = ((i+i)*vid.bytepp);  // x offset only
                s = &wipe_scr_start[idx];
	        idx += (melty[i]*vid.ybytes);  // with melty offset
                d = &wipe_scr[idx];
                e = &wipe_scr_end[idx];
                melty[i] += dy;
#ifdef ENABLE_DRAWEXT
	        if( vid.drawmode != DRAW8PAL )
	        {
		    // copy end screen over newly exposed dy area
		    for (j=dy;j;j--)
		    {
		        memcpy(d, e, cpycnt);  // 2 pixels
		        e += vid.ybytes;
		        d += vid.ybytes;
		    }
		    // redraw start screen columns shifted down by melty[i]
		    for (j=height-melty[i];j;j--)
		    {
		        memcpy(d, s, cpycnt);  // 2 pixels
		        s += vid.ybytes;
		        d += vid.ybytes;
		    }
		}
	        else
#endif
	        {
		    // Simpler, faster for older slow machines
		    // copy end screen over newly exposed dy area
		    for (j=dy;j;j--)
		    {
		        *(uint16_t*)d = *(uint16_t*)e;  // 2 pixels
		        e += vid.ybytes;
		        d += vid.ybytes;
		    }
		    // redraw start screen columns shifted down by melty[i]
		    for (j=height-melty[i];j;j--)
		    {
		        *(uint16_t*)d = *(uint16_t*)s;  // 2 pixels
		        s += vid.ybytes;
		        d += vid.ybytes;
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
}


//  save the 'before' screen of the wipe (the one that melts down)
//
// [WDJ] always full copy
int wipe_StartScreen ( void )
{
    wipe_scr_start = screens[2];
    I_ReadScreen(wipe_scr_start);  // copy vid.display in screen format
    return 0;
}


//  save the 'after' screen of the wipe (the one that show behind the melt)
//
// [WDJ] always full copy
int wipe_EndScreen ( void )
{
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
        wipe_scr = screens[0];
        (*wipes_init[wipeno])();
    }

    // do a piece of wipe-in
#ifdef DIRTY_RECT
    //V_MarkRect(0, 0, width, height);
#endif
    rc = (*wipes_do[wipeno])(ticks);
    //  V_CopyBlock(x, y, width, height, wipe_scr, screens[0]); // DEBUG

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes_exit[wipeno])();
    }

    return !go;

}
