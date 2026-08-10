// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: st_lib.c 1562 2020-11-29 11:51:00Z wesleyjohnson $
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
// $Log: st_lib.c,v $
// Revision 1.7  2003/05/04 04:17:17  sburke
// Add SHORT() conversion for big-endian machines.
//
// Revision 1.6  2001/02/24 13:35:21  bpereira
//
// Revision 1.5  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.4  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.3  2000/09/28 20:57:18  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      The status bar widget code.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "st_lib.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"

#include "i_video.h"
  // rendermode

//#define DEBUG_COPYRECT

// [WDJ] all STlib, number, etc. patches are already endian fixed

#define FLASH_COLOR  0x72

// STlib global enables.
// Clear to background, except when overlay or hardware draw.
//faB:current hardware mode always refresh the statusbar
boolean  stlib_enable_erase = false;
// Set when redrawing from some unknown state, clear when additive update.
boolean  stlib_force_refresh = true;



// Initialize number widget.
// If font contains percent at [11] then also can draw percent number.
//  patch_list : font
void STlib_initNum ( st_number_t * ni,
                     int  x, int  y,
                     patch_t ** patch_list,
                     int * num,
                     int   width )
{
    ni->x        = x;
    ni->y        = y;
    ni->width    = width;        // number of digits
    ni->num      = num;
    ni->patches  = patch_list;
    ni->prev_num = NON_NUMBER;
    ni->command  = STLIB_REFRESH;
}


//
// Draw when the number changes from the previous value,
// or by command in status.
//
// Called by ST_drawWidgets, when sbar_on
// Called by STlib_updatePercent
void STlib_updateNum ( st_number_t*  ni )
{
    int    numdigits = ni->width;
    int    num = *ni->num;
      // number to be drawn.  NON_NUMBER is not drawn
    boolean   neg;
    // [WDJ] all ST patches are already endian fixed
    // Hardware or software render.
    patch_t * pf = V_patch( ni->patches[0] );  // patch fields
    int    w = pf->width;
    int    h = pf->height;
    int    x;
   
    // Draw to stbar_fg, screen0 status bar

    // clear the area
    x = ni->x - numdigits*w;

#ifdef DEBUG_COPYRECT
       CONS_Printf("V_CopyRect1: %d %d %d %d %d %d %d %d val: %d\n",
              x, ni->y, BG, w*numdigits, h, x, ni->y, stbar_fg, num);
#endif
    if( ni->command == STLIB_FLASH )
    {
        V_DrawScaledFill( x, ni->y, w*numdigits, h, FLASH_COLOR );
        ni->command = STLIB_REFRESH;
    }
    else
    {
        // Clear to background, except when overlay or hardware draw.
        //faB:current hardware mode always refresh the statusbar
        if(stlib_enable_erase)
        {
           // Software render only.
           V_CopyRect(x, ni->y, BG, w*numdigits, h, x, ni->y, stbar_fg);
	}

        ni->command = 0;
    }

    ni->prev_num = num;

    // if non-number, do not draw it
    if (num == NON_NUMBER)
        return;

    neg = num < 0;
    if (neg)
    {
        if (numdigits == 2 && num < -9)
            num = -9;
        else if (numdigits == 3 && num < -99)
            num = -99;

        num = -num;
    }

    x = ni->x;

    // in the special case of 0, you draw 0
    if (num == 0)
        V_DrawScaledPatch(x - w, ni->y, ni->patches[ 0 ]);

    // draw the new number
    while (num && numdigits--)
    {
        x -= w;
        V_DrawScaledPatch(x, ni->y, ni->patches[ num % 10 ]);
        num /= 10;
    }

    // draw a minus sign if necessary
    if (neg && ni->patches[10])
        V_DrawScaledPatch(x - 8, ni->y, ni->patches[10]);
}



// Draw a number as a percentage.
void STlib_updatePercent ( st_number_t*  per )
{
    // Percent is in the number font at [11]
    if (per->command == STLIB_REFRESH || stlib_force_refresh )
    {
      if( per->patches[11] )
        V_DrawScaledPatch(per->x, per->y, per->patches[11]);
    }

    STlib_updateNum(per);
}



void STlib_initMultIcon ( st_multicon_t *  mi,
                          int x, int y,
                          patch_t **  patch_list,
                          int * icon_index )
{
    mi->x = x;
    mi->y = y;
    mi->icon_index = icon_index;
    mi->patches  = patch_list;
    mi->prev_icon_index = -1;  // detectable invalid
    mi->command = STLIB_REFRESH;
}



void STlib_updateMultIcon ( st_multicon_t*  mi )
{
    int  iconindex = *(mi->icon_index);

    if ((mi->prev_icon_index != iconindex) || mi->command || stlib_force_refresh )
    {
        // Icon display has changed from previous.
        if( mi->command == STLIB_FLASH )
        {
            // Actual flash is drawn elsewhere, over several icons positions
            // at once.
            mi->command = STLIB_FLASH_CLEAR;
        }
        else if(mi->prev_icon_index >= 0 || mi->command || stlib_force_refresh )
        {
            //faB:current hardware mode always refresh the statusbar
            // Copy the background at this screen position, erasing previous Icon.
            if( stlib_enable_erase )
            {
                int erase_index = (mi->prev_icon_index >= 0)? mi->prev_icon_index : 0;
                // Hardware or software render.
                patch_t * pp = V_patch( mi->patches[erase_index] );  // patch fields
                int x = mi->x - pp->leftoffset;
                int y = mi->y - pp->topoffset;
                int w = pp->width;
                int h = pp->height;
#ifdef DEBUG_COPYRECT
       CONS_Printf("V_CopyRect2: %d %d %d %d %d %d %d %d\n",
                            x, y, BG, w, h, x, y, stbar_fg);
#endif
                V_CopyRect(x, y, BG, w, h, x, y, stbar_fg);
            }

            mi->command = 0;
        }
        // Draw icon patch.       
        if(iconindex >= 0 && mi->patches[iconindex])
            V_DrawScaledPatch(mi->x, mi->y, mi->patches[iconindex]);

        mi->prev_icon_index = iconindex;
    }
}



void STlib_initBinIcon ( st_binicon_t *        bi,
                         int                   x,
                         int                   y,
                         patch_t *             patch,
                         boolean *             val )
{
    bi->x        = x;
    bi->y        = y;
    bi->boolval  = val;
    bi->patch    = patch;
    bi->prev_val = 0;
    bi->command  = STLIB_REFRESH;
}



void STlib_updateBinIcon ( st_binicon_t*   bi )
{
    boolean on = *bi->boolval;

    if ((bi->prev_val != on) || bi->command || stlib_force_refresh)
    {
        // Update the icon display.
        // BinIcon does not flash.
        if (on)
            V_DrawScaledPatch(bi->x, bi->y, bi->patch);
        else if (stlib_enable_erase)
        {
            // Software render only.
            //faB:current hardware mode always refresh the statusbar
            // Erase icon by copying background.
            patch_t * pf = bi->patch;  // patch fields
            int x = bi->x - pf->leftoffset;
            int y = bi->y - pf->topoffset;
            int w = pf->width;
            int h = pf->height;

#ifdef DEBUG_COPYRECT
       CONS_Printf("V_CopyRect3: %d %d %d %d %d %d %d %d\n",
                            x, y, BG, w, h, x, y, stbar_fg);
#endif
            V_CopyRect(x, y, BG, w, h, x, y, stbar_fg);
        }

        bi->prev_val = on;
        bi->command = 0;
    }
}
