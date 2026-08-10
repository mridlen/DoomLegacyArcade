// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video_ggi.c 1570 2021-01-28 09:24:16Z wesleyjohnson $
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
// $Log: i_video_ggi.c,v $
// Revision 1.7  2004/05/16 19:11:53  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.6  2001/04/27 13:32:14  bpereira
// no message
//
// Revision 1.5  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.4  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.3  2000/03/06 15:19:58  hurdler
// Add Bell Kin's changes
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Inspired from l_video.ggi.c by Colin Phipps (cph@lxdoom.linuxgames.com)
//      GGI target code
//
//-----------------------------------------------------------------------------


// checked mouse 19990314 by Kin
#define CHECKEDMOUSE


#include "doomincl.h"

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <ggi/ggi.h>

#ifdef USEASM
// added 19990125 by Kin
// [WDJ] vid_copy.s exists, but do not have vid_copy.h
// #include "vid_copy.h"
#endif

#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "m_menu.h"
#include "d_main.h"
#include "s_sound.h"
// added for 1.27 19990220 by Kin
#include "g_input.h"
#include "st_stuff.h"
#include "g_game.h"

boolean showkey=0; // force to no 19990118 by Kin

#ifdef EXPAND_BUFFER_ENABLE
// incomplete and unused
boolean expand_buffer = false;
//static byte* out_buffer;
#endif

static ggi_visual_t  g_screen;
static const ggi_pixelformat* pixelformat;

#ifdef MULTIPLY_ENABLE
// incomplete and unused
static int multiply = 1;
#endif

// Mask of events that we are interested in
static const ggi_event_mask ev_mask = \
emKeyPress | emKeyRelease | emPtrRelative | emPtrButtonPress | \
emPtrButtonRelease;

#if 0
// Unused vars to preserve in config file
int leds_always_off;
int use_vsync;
#endif

////////////////////////////////////////////////////////////////
// Input handling utility functions

// Null keyboard translation to satisfy m_misc.c
int I_DoomCode2ScanCode(int c)
{
  return c;
}

int I_ScanCode2DoomCode(int c)
{
  return c;
}

//
// I_GIITranslateKey
//
// Translate keys from LibGII
// Adapted from Linux Heretic v0.9.2

int I_GIITranslateKey(const ggi_key_event* key, event_t * ev)
{
  int label = key->label, sym = key->sym;
  int rc=0;

  ev->data2 = 0;  // ASCII character
  switch(label) {
  case GIIK_CtrlL:  case GIIK_CtrlR:  rc=KEY_LCTRL;      break;
  case GIIK_ShiftL: case GIIK_ShiftR: rc=KEY_LSHIFT;     break;
  case GIIK_MetaL:  case GIIK_MetaR:  rc=KEY_LALT;       break;
  case GIIK_AltL:   case GIIK_AltR:   rc=KEY_RALT;       break;
  case GIIUC_BackSpace:   rc = KEY_BACKSPACE;     break;
  case GIIUC_Escape:      rc = KEY_ESCAPE;        break;
  case GIIK_Delete:       rc = KEY_DELETE;        break;
  case GIIK_Insert:       rc = KEY_INS;        break;
  case GIIK_PageUp:       rc = KEY_PGUP;        break;
  case GIIK_PageDown:     rc = KEY_PGDN;      break;
  case GIIK_Home:         rc = KEY_HOME;          break;
  case GIIK_End:  rc = KEY_END;           break;
  case GIIUC_Tab:         rc = KEY_TAB;     break;
  case GIIK_Up:           rc = KEY_UPARROW; break;
  case GIIK_Down:         rc = KEY_DOWNARROW;       break;
  case GIIK_Left:         rc = KEY_LEFTARROW;       break;
  case GIIK_Right:        rc = KEY_RIGHTARROW;      break;
  case GIIK_Enter:        rc = KEY_ENTER;           break;
  case GIIK_F1: rc = KEY_F1;            break;
  case GIIK_F2: rc = KEY_F2;            break;
  case GIIK_F3: rc = KEY_F3;            break;
  case GIIK_F4: rc = KEY_F4;            break;
  case GIIK_F5: rc = KEY_F5;            break;
  case GIIK_F6: rc = KEY_F6;            break;
  case GIIK_F7: rc = KEY_F7;            break;
  case GIIK_F8: rc = KEY_F8;            break;
  case GIIK_F9: rc = KEY_F9;            break;
  case GIIK_F10:        rc = KEY_F10;           break;
  case GIIK_F11:        rc = KEY_F11;           break;
  case GIIK_F12:        rc = KEY_F12;           break;
  case GIIK_Pause:rc = KEY_PAUSE;               break;
  case GIIK_PSlash:rc = KEY_KPADSLASH;          break;
  case GIIK_P0:rc = KEY_KEYPAD0;break;
  case GIIK_P1:rc = KEY_KEYPAD1;break;
  case GIIK_P2:rc = KEY_KEYPAD2;break;
  case GIIK_P3:rc = KEY_KEYPAD3;break;
  case GIIK_P4:rc = KEY_KEYPAD4;break;
  case GIIK_P5:rc = KEY_KEYPAD5;break;
  case GIIK_P6:rc = KEY_KEYPAD6;break;
  case GIIK_P7:rc = KEY_KEYPAD7;break;
  case GIIK_P8:rc = KEY_KEYPAD8;break;
  case GIIK_P9:rc = KEY_KEYPAD9;break;
  case GIIK_PPlus:rc = KEY_PLUSPAD;break;
  case GIIK_PMinus:rc = KEY_MINUSPAD;break;
  /* Decimal???? why not Dot??? */
  case GIIK_PDecimal:rc = KEY_KPADDEL;break;
  case GIIK_CapsLock:rc = KEY_CAPSLOCK;break;
  case GIIK_ScrollLock:rc = KEY_SCROLLLOCK;break;
  case GIIK_NumLock:rc = KEY_NUMLOCK;break;
  default:
          if ((label > '0' && label < '9') ||
              label == '.' ||
              label == ',') {
              /* Must use label here, or it won't work when shift
                 is down */
            rc = label;
          } else if (sym < 256) {
              /* ASCII key - we want those */
            ev->data2 = sym; // ASCII character
            rc = sym;
              /* We want lowercase */
            if (rc >='A' && rc <='Z') rc -= ('A' - 'a');
            switch (sym) {
              /* Some special cases */
            case '+': rc = '=';  break;
//            case '-': rc = '-';
            default:  break;
            }
          }
  }
  ev->data1 = rc;  // keycode
  return rc;
}

unsigned short I_GIITranslateButtons(const ggi_pbutton_event* ev)
{
  return ev->button;
}

////////////////////////////////////////////////////////////////
// API

//
// I_StartFrame
//
void I_StartFrame (void)
{
  // If we reuse the blitting buffer in the next rendering,
  // make sure it is reusable now
#ifdef EXPAND_BUFFER_ENABLE
  if (!expand_buffer)
#endif
    ggiFlush(g_screen);
}

void I_OsPolling(void) {
  I_GetEvent();
}

#ifdef CHECKEDMOUSE
int btmask = 0;
#endif

#ifdef LJOYSTICK
extern void I_GetJoyEvent();
#endif
#ifdef MOUSE2
extern byte mouse2_started = 0;
static void I_GetMouse2Event(void);
#endif

void I_GetEvent(void)
{
  struct timeval nowait = { 0, 0 };

#ifdef MOUSE2
  if( mouse2_started )
    I_GetMouse2Event();
#endif
#ifdef LJOYSTICK
  I_GetJoyEvent();
#endif

  while (ggiEventPoll(g_screen, ev_mask, &nowait)!=evNothing) {
    // There is a desirable event
    ggi_event ggi_ev;
    event_t doom_ev;
    int m_x = 0, m_y = 0; // Mouse motion
    unsigned short buttons; // Buttons
    int i,j;
    // GII will return modified button "number"

    ggiEventRead(g_screen, &ggi_ev, ev_mask);

    switch(ggi_ev.any.type) {
    case evKeyPress:
      doom_ev.type = ev_keydown;
      if (I_GIITranslateKey(&ggi_ev.key, &doom_ev) >0 )
      {
#ifdef DEBUG_MOUSE2_STIMULUS
        // Intercept some keys and fake the mouse2 input using mouse2_stim_trigger.
        if( (event.data1 >= KEY_KEYPAD1) && (event.data1 <= KEY_KEYPAD9) )
        {
            mouse2_stim_trigger = event.data1 - KEY_KEYPAD0;
            break;
        }
#endif
        D_PostEvent(&doom_ev);
      //GenPrintf( EMSG_debug,"p:%4x\n",doom_ev.data1);
      }
      break;
    case evKeyRelease:
      doom_ev.type = ev_keyup;
      if (I_GIITranslateKey(&ggi_ev.key, &doom_ev) >0 )
      {
        D_PostEvent(&doom_ev);
      //GenPrintf( EMSG_debug,"r:%4x\n",doom_ev.data1);
      }
      break;
    case evPtrRelative:
      m_x += ggi_ev.pmove.x;
      m_y += ggi_ev.pmove.y;
      break;
    case evPtrButtonPress:
      buttons = I_GIITranslateButtons(&ggi_ev.pbutton);
#ifdef CHECKEDMOUSE
      if(btmask&(1<<buttons)) {
         break;
      } else btmask |= (1<<buttons);
#endif
      doom_ev.type = ev_keydown;
      // translate button2 in ggi to MOUSE3 19990225 by Kin
      doom_ev.data1=KEY_MOUSE1+(buttons^(buttons>>1))-1;
      D_PostEvent(&doom_ev);
      break;
    case evPtrButtonRelease:
      buttons = I_GIITranslateButtons(&ggi_ev.pbutton);
#ifdef CHECKEDMOUSE
      if(!(btmask&(1<<buttons))) {
         break;
      } else btmask ^= (1<<buttons);
#endif
      doom_ev.type = ev_keyup;
      doom_ev.data1=KEY_MOUSE1+(buttons^(buttons>>1))-1;
      D_PostEvent(&doom_ev);
      break;
    default:
      //I_Error("I_OsPolling: Bad GGI event type");
    }

    if (m_x||m_y) {
      doom_ev.type = ev_mouse;
      // 4x mouse like xdoom 19990227 by Kin
      doom_ev.data2 = m_x << 2;
      doom_ev.data3 = -(m_y << 2);
      D_PostEvent(&doom_ev);
    }
  }
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
  // Finish up any output
  ggiFlush(g_screen);
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
  // cv_vidwait.value not used, X11 handles its own vsync, no controls

#ifdef EXPAND_BUFFER_ENABLE
  // scales the screen size before blitting it
  // disabled for now 19990221 by Kin
  //  if (expand_buffer)
  //  (*I_ExpandImage)(out_buffer, screens[0]);
#endif

  // Blit it
  ggiPutBox(g_screen, 0, 0, vid.width, vid.height, vid.display);
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
  memcpy(scr, vid.display, vid.screen_size);
}

void I_SetPalette(RGBA_t* palette)
{
  if (vid.bitpp == 8) {
    ggi_color ggi_pal[256];
    ggi_color* p;
    int i;
    // PseudoColor mode, so set the palette

    for (i=0, p=ggi_pal; i<256; i++) {
      // Translate palette entry to 16 bits per component
      p->r = palette->s.red   << 8;
      p->g = palette->s.green << 8;
      p->b = palette->s.blue  << 8;

      p++; palette++;
    }

    ggiSetPalette(g_screen, 0, 256, ggi_pal);
    //  } else {
    // TrueColor mode, so rewrite conversion table

    //    I_SetPaletteTranslation(palette);
  }
}

void I_ShutdownGraphics(void)
{
  if( graphics_state <= VGS_shutdown )
      return;

  graphics_state = VGS_shutdown;  // to catch some repeats due to errors

  ggiRemoveFlags(g_screen, GGIFLAG_ASYNC);

  if (vid.buffer)
    free(vid.buffer);

  ggiExit();
   
  graphics_state = VGS_off;
}

#define MAX_GGIMODES 19
#define MAX_LEN_VIDMODENAME  10

const ggi_coord temp_res[MAX_GGIMODES]= {
        {320,200},
        {320,240},
        {320,350}, /* EGA text */
        {360,400}, /* VGA text */
        {400,300},
        {480,300},
        {512,384},
        {640,200}, /* EGA */
        {640,350}, /* EGA */
        {640,400},
        {640,480},
        {720,350}, /* MDA text */
        {720,400}, /* VGA text */
        {800,600},
        {1024,768},
        {1152,864},
        {1280,960},
        {1280,1024},
        {1600,1200}
};

int num_vidmodes;  // number of vidmodes
ggi_mode vidmodes[MAX_GGIMODES+1];      // the ggi mode
ggi_coord vidmode_res[MAX_GGIMODES+1];  // the actual resolution
char vidname[MAX_GGIMODES+1][MAX_LEN_VIDMODENAME];

// return number of fullscreen or window modes, for listing
// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };  // first is always 1
    mrange.last = num_vidmodes;
    return mrange;
}


modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    if( mi > num_vidmodes )
        goto fail;

    ms.width = vidmode_res[mi].x;
    ms.height = vidmode_res[mi].y;
    ms.type = MODE_fullscreen;
    ms.mark = "";
    return ms;
   
 fail:
    ms.type = MODE_NOP;
    ms.width = ms.height = 0;
    ms.mark = NULL;
    return ms;
}

char * VID_GetModeName( modenum_t modenum )
{
    int mi = modenum.index - 1;

    if( mi > num_vidmodes )
        return NULL;

    // fullscreen modes  1..
    return vidname[ mi ];
}

modenum_t  VID_GetModeForSize( int w, int h, byte modetype )
{
  modenum_t  modenum = { MODE_NOP, 0 };
  int bestdist = INT_MAX;
  int best, tdist, i;
   
  if( modetype == MODE_fullscreen )
  {
      // fullscreen modes
      // find exact match
      for (i=0; i<num_vidmodes;i++) {
          if(vidmode_res[i].x==w && vidmode_res[i].y==h )  goto found_i;
      }

      // find next larger
      for (i=0; i<num_vidmodes;i++) {
          if(vidmode_res[i].x>=w  &&  vidmode_res[i].y>=h )  goto found_i;
      }
  }
  // not found
  return modenum;

found_i:
  modenum.index = i + 1;
  modenum.modetype = modetype;
  return modenum;
}


int VID_GetModes( byte select_bpp )
{
    int gt_parm =
         (select_bpp==8)? GT_8BIT
       : (select_bpp==15)? GT_15BIT
       : (select_bpp==16)? GT_16BIT
       : (select_bpp==24)? GT_24BIT
       : (select_bpp==32)? GT_32BIT ;

    // check available modes
    num_vidmodes=0;
    for(i=0;i<MAX_GGIMODES;i++) {
      ggi_mode * vmp = &vidmodes[num_vidmodes];
      int rx = temp_res[i];
      int ry = temp_res[i];
      if(!ggiCheckSimpleMode(g_screen, rx,ry, 2, gt_parm, vmp))
      {
        memcpy(&vidmode_res[num_vidmodes],&temp_res[i],sizeof(ggi_coord));
        snprintf(vidname[num_vidmodes], MAX_LEN_VIDMODENAME, "%dx%d",rx,ry);
        vidname[num_vidmodes][MAX_LEN_VIDMODENAME-1] = 0;
        if( verbose )
            GenPrintf( EMSG_info,"mode %s\n",vidname[num_vidmodes]);
        num_vidmodes++;
      } else {
        if( GT_DEPTH(vmp->graphtype) == select_bpp ) {
          //if((vmp->.visible.x>temp_res[i-1].x &&
          //    vmp->visible.x<temp_res[i+1].x &&
          //    vmp->visible.y>=temp_res[i-1].y) ||
          //   (vmp->visible.y>temp_res[i-1].y &&
          //    vmp->visible.y<temp_res[i+1].y &&
          //    vmp->visible.x>=temp_res[i-1].x)) {
          if(vmp->visible.x==rx
             && vmp->visible.y==ry )
          {
            vidmode_res[num_vidmodes].x = vmp->visible.x;
            vidmode_res[num_vidmodes].y = vmp->visible.y;
            sprintf(vidname[num_vidmodes],"%dx%d",
                    vidmode_res[num_vidmodes].x,vidmode_res[num_vidmodes].y);
            if( verbose )
                GenPrintf( EMSG_info,"suggested mode %s\n",vidname[num_vidmodes]);
            num_vidmodes++;
          }
        }
      }
    }
    return 1;
}


//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    int ret_value;
    
    // No hardware draw modes.
    if( request_drawmode >= DRM_opengl )
        return false;
   
    // Seems to be fullscreen only.
    if( ! request_fullscreen )
        return false;

    ret_value = VID_GetModes( request_bitpp );
    if( ret_value < 0 )
        return ret_value;

    return ( num_vidmodes > 0 );
}


// Called once. Init with basic error message screen.
void I_StartupGraphics(void)
{
    // pre-init by V_Init_VideoControl
  vid.draw_ready = 0;  // disable print reaching console
  graphics_state = VGS_startup;

  if (ggiInit())
    I_Error("Failed to initialise GGI\n");

  if (!(g_screen = ggiOpen(NULL))) // Open default visual
    I_Error("Failed to get default visual\n");

  native_drawmode = DRM_native;
  // FIXME: get native from ggi
  native_bitpp = 32;
  native_bytepp = 4;

  // ??? No window mode
  
  if( verbose )
      GenPrintf(EMSG_ver, "StartupGraphics completed\n" );

  vid.recalc = true;
  graphics_state = VGS_active;
  return;
}


// Called to start rendering graphic screen according to the request switches.
// Fullscreen modes are possible.
// param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
// Returns FAIL_select, FAIL_end, FAIL_create, of status_return_e, 1 on success;
void I_RequestFullGraphics( byte select_fullscreen )
{
  char * req_errmsg = NULL;
  modenum_t initialmode;
  int i;
  byte  select_bitpp = 0;
  byte  select_bytepp = 1;
  byte  select_fullscreen_mode;
  int ret_value;

  vid.draw_ready = 0;  // disable print reaching console
  graphics_state = VGS_startup;

#ifdef MULTIPLY_ENABLE
  { // Check for screen enlargement
    char str[3] = { '-', 0, 0 };
    int n;

    for (n=1; n<4; n++) {
      str[1] = n + '0';
      if (M_CheckParm(str)) multiply = n;
    }
  }
#endif

  switch(req_drawmode)
  {
   case DRM_native:
     if( V_CanDraw( native_bitpp )) {
         select_bitpp = native_bitpp;
         select_bytepp = native_bytepp;
     }else{
         // Use 8 bit and do the palette lookup.
         if( verbose )
             GenPrintf( EMSG_ver,"Native %i bpp rejected\n", native_bitpp );
         select_bitpp = 8;
         select_bytepp = 1;
     }
     break;
   default:
   case DRM_explicit_bpp:
     select_bitpp = req_bitpp;
     select_bytepp = (select_bpp + 7) >> 3;
     break;
  }
   
  ret_value = VID_GetModes( select_bitpp );
  if( ret_value < 0 )
      return ret_value;

  if( num_vidmodes )  goto found_modes;
     
  if(req_drawmode == DRM_explicit_bpp)
  {
      GenPrintf( EMSG_warn,"No %i bpp modes\n", req_bitpp );
      goto no_modes;
  }

draw_8pal:
  select_bitpp = 8;
  select_bytepp = 1;
  goto found_modes;

found_modes:
  vid.bitpp = select_bpp;
  vid.bytepp = select_bytepp;
  vid.width = req_width;
  vid.height = req_height;
  vid.buffer = NULL;
  vid.display = NULL;
  vid.screen1 = NULL;

  select_fullscreen_mode = vid_mode_table[select_fullscreen];
  initialmode = VID_GetModeForSize( req_width, req_height, select_fullscreen_mode );
  ret_value = VID_SetMode( initialmode );
  if( ret_value < 0 )
      return ret_value;

  // Go asynchronous
  ggiAddFlags(g_screen, GGIFLAG_ASYNC);

  // Mask events
  ggiSetEventMask(g_screen, ev_mask);

  graphics_state = VGS_fullactive;

  if( verbose )
        GenPrintf(EMSG_ver, "StartupGraphics completed\n" );
  return ret_value;

no_modes:
    return FAIL_select;
}



int VID_SetMode(modenum_t modenum)
{
  int mi = modenum_index - 1;
  if( mi >= num_vidmodes || mi < 0 )   goto fail_end;

  vid.draw_ready = 0;  // disable print reaching console
   
  if (ggiSetMode(g_screen,&vidmodes[mi])) {
    I_SoftError("Failed to set mode");
    return  FAIL_create;
  }
   
  // Commit to the new mode
  vid.modenum = modenum;

  vid.width = vidmode_res[mi].x;
  vid.height = vidmode_res[mi].y;
  vid.widthbytes = vid.width * vid.bytepp;
  vid.ybytes = vid.width * vid.bytepp;
  vid.screen_size = vid.ybytes * vid.height;

  if(vid.buffer) free(vid.buffer);
  vid.buffer = (unsigned char *) malloc (vid.screen_size * NUMSCREENS);
  vid.display = vid.buffer;
  vid.screen1 = vid.buffer + vid.screen_size;
  // direct buffer drawing
  vid.direct = vid.buffer;
  vid.direct_rowbytes = vid.width * vid.bytepp;
  vid.direct_size = vid.direct_rowbytes * vid.height;

  vid.fullscreen = 1; // usually, it does not know
  vid.recalc = 1;

#ifdef EXPAND_BUFFER_ENABLE
  //if(expand_buffer) {
  //  if(out_buffer) free(out_buffer);
  //}
  // Allocate enlarement buffer if needed
  //if (expand_buffer)
  //  out_buffer = malloc(vid.screen_size);
  //else
  //out_buffer = (byte*)screens[0];
#endif
  return 1;

fail_end:
  return FAIL_end;
}
