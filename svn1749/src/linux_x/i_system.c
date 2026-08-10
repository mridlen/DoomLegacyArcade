// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_system.c 1649 2023-10-25 02:11:02Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2015 by DooM Legacy Team.
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
// $Log: i_system.c,v $
// Revision 1.18  2002/07/26 15:23:01  hurdler
// near RC release
//
// Revision 1.17  2002/01/03 19:20:07  bock
// Add FreeBSD code to I_GetFreeMem.
// Modified Files:
//     makefile linux_x/i_system.c sdl/i_system.c
//
// Revision 1.16  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.15  2001/03/12 21:03:10  metzgermeister
//   * new symbols for rendererlib added in SDL
//   * console printout fixed for Linux&SDL
//   * Crash fixed in Linux SW renderer initialization
//
// Revision 1.14  2000/10/21 08:43:32  bpereira
// no message
//
// Revision 1.13  2000/10/16 07:53:57  metzgermeister
// fixed I_GetFreeMem
//
// Revision 1.12  2000/10/09 16:22:42  metzgermeister
// implemented GetFreeMem
//
// Revision 1.11  2000/10/02 18:25:47  bpereira
// no message
//
// Revision 1.10  2000/09/10 10:50:21  metzgermeister
// make it work again
//
// Revision 1.9  2000/08/11 19:11:07  metzgermeister
// *** empty log message ***
//
// Revision 1.8  2000/04/25 19:49:46  metzgermeister
// support for automatic wad search
//
// Revision 1.7  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.6  2000/04/12 19:31:37  metzgermeister
// added use_mouse to menu
//
// Revision 1.5  2000/04/07 23:12:38  metzgermeister
// fixed some minor bugs
//
// Revision 1.4  2000/03/22 18:52:56  metzgermeister
// added I_ShutdownCD to I_Quit
//
// Revision 1.3  2000/03/06 15:19:58  hurdler
// Add Bell Kin's changes
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "doomincl.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>
  // dirname

#include <stdarg.h>
#include <sys/time.h>
// statfs()
#ifndef FREEBSD
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
/*For meminfo*/
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef LJOYSTICK // linux joystick 1.x
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/joystick.h>
#endif

#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"
#include "g_input.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"
#include "i_joy.h"
#include "m_argv.h"

// MOUSE2_NIX dependent upon DoomLegacy headers.
#ifdef MOUSE2_NIX
#include <termios.h>
#include <sys/ioctl.h>
#endif

  
//#define DEBUG_MOUSE2
//#define DEBUG_MOUSE2_STIMULUS



extern void D_PostEvent(event_t*);

extern event_t         events[MAXEVENTS];
extern int             eventhead;
extern int             eventtail;

#if 0
// Locations for searching the legacy.dat
#define DEFAULTWADLOCATION1 "/usr/local/games/legacy"
#define DEFAULTWADLOCATION2 "/usr/games/legacy"
#define DEFAULTSEARCHPATH1 "/usr/local"
#define DEFAULTSEARCHPATH2 "/usr/games"
#define WADKEYWORD "legacy.dat"

// holds wad path
static char returnWadPath[256];
#endif

#ifdef JOYSTICK_SUPPORT
#ifdef LJOYSTICK

#define MAX_JOYAXES 3
typedef struct {
  int fd = -1;
  int numaxes = 0;
  uint16_t axis[MAX_JOYAXES];
} joystick_t;

#define MAX_JOYSTICK 4
joystick_t  joystk[ MAX_JOYSTICK ];

int joystick_started = 0;

#define NUM_JOYSTICKDEV 4
const char * joystick_dev[NUM_JOYSTICKDEV] =
{
"/dev/input/js0",
"/dev/input/js1",
"/dev/input/js2",
"/dev/input/js3",
};

#endif
#endif

#ifdef JOYSTICK_SUPPORT
//JoyType_t Joystick;
// must exist, even when LJOYSTICK off
int num_joysticks = 0;
#endif

#ifdef MOUSE2_NIX
int mouse2_fd = -1;
byte mouse2_started = 0;
#endif
#ifdef DEBUG_MOUSE2_STIMULUS
// trigger mouse2 stimulus
static byte  mouse2_stim_trigger = 0;
#endif

// dummy 19990119 by Kin
byte keyboard_started = 0;
// current modifier key status
byte shiftdown = false;
byte altdown = false;

void I_StartupKeyboard (void) {}
void I_StartupTimer (void) {}

void I_OutputMsg (char *fmt, ...) 
{
    va_list     argptr;

    va_start (argptr,fmt);
    vfprintf (stderr,fmt,argptr);
    va_end (argptr);
}

int I_GetKey (void) 
{
    // Warning: I_GetKey empties the event queue till next keypress
    event_t*    ev;
    int rc=0;
    
    // return the first keypress from the event queue
    for ( ; eventtail != eventhead ;  )
    {
        ev = &events[eventtail];
        if(ev->type == ev_keydown)
        {
            rc = ev->data1;
        }

        eventtail++;
        eventtail = eventtail & (MAXEVENTS-1);
    }
    
    return rc; 
}

#ifdef JOYSTICK_SUPPORT
#ifdef LJOYSTICK

#if 0
// [WDJ] Old joystick code, scale is now handled in g_game.c
int joy_scale = 1;
void I_JoyScale(void)
{
  joy_scale = (cv_joyscale.value==0)?1:cv_joyscale.value;
}
#endif

void I_GetJoyEvent(void)
{
  struct js_event jdata;
  static event_t event = {0,0,0,0};
  static int buttons = 0;
  int i;

  if(!joystick_started)
      return;

  for(i=0; i<num_joysticks; i++)  // all joysticks found
  {
   while(read( joystk[i].fd, &jdata, sizeof(jdata)) != -1)
   {
    switch(jdata.type) {
    case JS_EVENT_AXIS:
#if 1
      // [WDJ] Polled joystick
      // Save for later polling
      if( jdata.number < joystk[i].numaxes ) {
          joystk[i].axis[jdata.number] = jdata.value >> 5;
      }
#else       
      // [WDJ] Old event driven joystick position, it now is polled
      event.type = ev_joystick;
      event.data1 = 0;
      switch(jdata.number) {
      case 0:
        event.data2 = ((jdata.value >> 5)/joy_scale)*joy_scale;
        D_PostEvent(&event);
        break;
      case 1:
        event.data3 = ((jdata.value >> 5)/joy_scale)*joy_scale;
        D_PostEvent(&event);
      default:
        break;
      }
      break;
#endif       
    case JS_EVENT_BUTTON:
      if(jdata.number<JOYBUTTONS) {
        if(jdata.value) {
          if(!((buttons >> jdata.number)&1)) {
            buttons |= 1 << jdata.number;
            event.type = ev_keydown;
            event.data1 = KEY_JOY0BUT0 + (i*JOYBUTTONS) + jdata.number;
            D_PostEvent(&event);
          }
        } else {
          if((buttons>>jdata.number)&1) {
            buttons ^= 1 << jdata.number;
            event.type = ev_keyup;
            event.data1 = KEY_JOY0BUT0 + (i*JOYBUTTONS) + jdata.number;
            D_PostEvent(&event);
          }
        }
      }
      break;
    }
   }  // while
  } // for
}

static void I_ShutdownJoystick(void)
{
  int i;
  for( i=0; i<num_joysticks; i++ )
  {
      if(joystk[i].fd != -1)
         close(joystk[i].fd);
      joystk[i].fd = -1;
  }
  num_joysticks = 0;
  joystick_started = 0;
}

// open the next joystick
static boolean joy_open(char * fname)
{
#define JOYNAME_LEN 511
  char * joyname[JOYNAME_LEN+1];
  int joyaxes;
  int joyfd = open(fname, O_RDONLY|O_NONBLOCK);
   
  if(joyfd==-1) {
    CONS_Printf("Error opening %s!\n",fname);
    return 0;
  }
  
  // Get number of axes
  ioctl(joyfd,JSIOCGAXES,&joyaxes);
  if(joyaxes<2) {
    CONS_Printf("Not enought axes?\n");
    close(joyfd);
    return 0;
  }

  // Record the joystick
  if( num_joysticks >= MAX_JOYSTICK )  return 0;
  joystk[num_joysticks].fd = joyfd;
  joystk[num_joysticks].numaxes = joyaxes;
  CONS_Printf(" Properties of joystick %d:\n", num_joysticks);
  ioctl(joyfd,JSIOCGNAME(JOYNAME_LEN), joyname)
  CONS_Printf("    %s.\n", joyname );
  CONS_Printf("    %d axes.\n", joyaxes);
//  CONS_Printf("    %d buttons.\n", );
//  CONS_Printf("    %d hats.\n", );
//  CONS_Printf("    %d trackballs.\n", );
  num_joysticks++;
  joystick_started = 1;
  return 1;
}
   
/*int joy_waitb(int fd, int *xpos,int *ypos,int *hxpos,int *hypos) {
  int i,xps,yps,hxps,hyps;
  struct js_event jdata;
  for(i=0;i<1000;i++) {
    while(read(fd,&jdata,sizeof(jdata))!=-1) {
      switch(jdata.type) {
      case JS_EVENT_AXIS:
        switch(jdata.number) {
        case 0: // x
          xps = jdata.value;
          break;
        case 1: // y
          yps = jdata.value;
          break;
        case 3: // hat x
          hxps = jdata.value;
          break;
        case 4: // hat y
          hyps = jdata.value;
        default:
          break;
        }
        break;
      case JS_EVENT_BUTTON:
        break;
      }
    }
  }
  }*/
#endif
#endif

#ifdef JOYSTICK_SUPPORT
void I_InitJoystick (void)
{
#ifdef LJOYSTICK
  int i;
   
  I_ShutdownJoystick();
  if(!strcmp(cv_usejoystick.string,"0"))
    return;

  joy_open(cv_joyport.string);  // primary port
  for(i=0; i<NUM_JOYSTICKDEV; i++ )  // other js ports
  {
      joy_open( joystick_dev[i] );
  }
  return;
#endif
}

int I_JoystickNumAxes (int joynum)
{
#ifdef LJOYSTICK
    if(joynum < num_joysticks )   return joystk[joynum].joyaxes;
#endif
    return 0;
}

// Polling by G_BuildTiccmd
// Caller has individual scale conversion.   
int I_JoystickGetAxis (int joynum, int axisnum )
{
#ifdef LJOYSTICK
    if(joynum < num_joysticks ) {
       if( axisnum < joystk[joynum].numaxes ) {
          return joystk[joynum].axis[axisnum];
       }
    }
#endif
    return 0;
}
#endif


#ifdef MOUSE2
// always MOUSE2_NIX

#ifdef DEBUG_MOUSE2
static void  dump_packet( const char * msg, byte * packet, int len )
{
    char  bb[128];
    int   bn = 0;
    int   i;

    if( len > 127 )  len=127;
    for( i=0; i<len; i++ )
        bn += snprintf( &bb[bn], 128-bn, " %X", packet[i] );
    bb[127] = 0;
    CONS_Printf( "%s len=%i : %s\n", msg, len, bb );
}
#endif

#ifdef DEBUG_MOUSE2_STIMULUS
const byte PC_stim[] = {
 0x40, 0x04, 0x00, 0x40, 0x10, 0x00, 0x40, 0x08, 0x00, 0x41, 0x00, 0x00,
 0x43, 0x31, 0x00, 0x43, 0x20, 0x00, 0x43, 0x28, 0x00, 0x43, 0x21, 0x00,
 0x40, 0x00, 0x10, 0x40, 0x00, 0x20, 0x40, 0x00, 0x1F, 0x44, 0x00, 0x00,
 0x4C, 0x00, 0x3F, 0x4C, 0x00, 0x32, 0x4C, 0x00, 0x31, 0x4C, 0x00, 0x20,
 0x60, 0x00, 0x00, 0x40, 0x00, 0x00, 0x50, 0x00, 0x00, 0x40, 0x00, 0x00,
};
const byte PC_stim_len = 12;
const byte PC_stim_index[] = { 0, 0, 12, 24, 36, 48, 3, 15, 18, 48, 0 };
#endif

// This table converts PC-Mouse button order (LB, RB)
// to our internal button order (B3, B2, B1).
// Converts  (LB, RB) ==> (RB, CB, LB)
const byte PC_Mouse_to_button[4] = {0,4,1,5};

// This table converts MouseSystems button order (LB, CB, RB)
// to our internal button order (B3, B2, B1).
// Converts  (LB, CB, RB) ==> (RB, CB, LB)
//const byte MouseSystems_to_button[8] = {0,4,2,6,1,5,3,7}; // true high
const byte MouseSystems_to_button[8] = {7,3,5,1,6,2,4,0}; // true low

// Converts  (MB, RB, LB) ==> (RB, CB, LB)
const byte PS2_to_button[8] = {0,1,4,5,2,3,6,7};


#define MOUSE2_BUFFER_SIZE 256

void I_GetMouse2Event(void)
{
    // The system update of the serial ports is slow and there may be 15 to 30 calls
    // before the next data read is ready.
    // We may get an incomplete mouse packet, the partial packet is kept in m2pkt.
    static int   m2plen = 7;  // mouse2 packet length
    static byte  m2pkt[8];    // mouse2 packet
    static byte  mouse2_ev_buttons = 0;
    static byte  staleness = 0;

    event_t event;

    // System read is expensive, so read to buffer, and process all of it.
    byte m2_buffer[MOUSE2_BUFFER_SIZE];
    int  m2_len;
    int  m2i;
    int  mouse2_rd_x, mouse2_rd_y;
    byte  mouse2_rd_buttons;

#ifdef DEBUG_MOUSE2_STIMULUS
    if( cv_mouse2type.EV == 0 )
    {
        if( staleness < 15 )  goto no_input;
        if( mouse2_stim_trigger == 0 )  goto no_input;
        if( cv_mouse2type.EV == 0 )
        {
            memcpy( m2_buffer, &PC_stim[ PC_stim_index[mouse2_stim_trigger]], PC_stim_len );
            m2_len = PC_stim_len;
        }
        mouse2_stim_trigger = 0;
        goto got_packet;
    }
#endif
   
    // Fill m2_buffer, without knowing start of packet.
    m2_len = read(mouse2_fd, m2_buffer, MOUSE2_BUFFER_SIZE);
    if( m2_len < 1 )
        goto no_input;

#ifdef DEBUG_MOUSE2_STIMULUS
got_packet:
#endif
       
#ifdef DEBUG_MOUSE2
    CONS_Printf("staleness= %i \n", staleness);
    if( m2plen > 0 && m2plen < 5 )
    {
       dump_packet(" leftover", m2pkt, m2plen );
    }
    dump_packet( "mouse buffer", m2_buffer, m2_len );
#endif

    // Defer processing inits, most often will not have any input.
    staleness = 0;
    mouse2_rd_buttons = mouse2_ev_buttons; // to detect changes
    mouse2_rd_x = mouse2_rd_y = 0;

    // Mouse packets can be 3 or 4 bytes.
    // Parse the mouse packets
    for(m2i=0; m2i<m2_len; m2i++)
    {
        byte mb = m2_buffer[m2i];

        // Sync detect
        switch( cv_mouse2type.EV )
        {
          case 0:  // PC Mouse
            if( mb & 0x40 )  // first byte
                m2plen = 0;
            break;

          case 1:  // MouseSystems
            if( (m2plen >= 5) && (mb & 0x80) )  // first byte
                m2plen = 0;
            break;

          case 2:  // PS/2
            if( (m2plen >= 3) && (mb & 0x04) )  // maybe first byte
                m2plen = 0;
            break;

#if 0	     
          default: // no sync detect, count bytes
            if( m2plen >= 3 )
                m2plen = 0;
            break; 
#endif
        }
            
        if(m2plen > 6)  continue;
            
#ifdef DEBUG_MOUSE2
        CONS_Printf( " m2pkt[%i]= %X = m2_buffer[%i]\n", m2plen, mb, m2i);
#endif
        m2pkt[m2plen++] = mb;

        switch( cv_mouse2type.EV )
        {
          case 0:  // PC Mouse
            if(m2plen==3)
            {
#ifdef DEBUG_MOUSE2
                dump_packet( "PC", m2pkt, 3 );
#endif
                // PC mouse format, Microsoft protocol, 2 buttons.
                mouse2_rd_buttons &= ~0x05; // B1, B3
                mouse2_rd_buttons |= PC_Mouse_to_button[ (m2pkt[0] & 0x30) >> 4];  // RB, LB
                int dx = (int8_t)(((m2pkt[0] & 0x03) << 6) | (m2pkt[1] & 0x3F)); // signed
                int dy = (int8_t)(((m2pkt[0] & 0x0C) << 4) | (m2pkt[2] & 0x3F)); // signed
#ifdef DEBUG_MOUSE2
                CONS_Printf( "mouse buttons= %X  dx,dy = ( %i, %i )\n", mouse2_rd_buttons, dx, dy);
#endif
                mouse2_rd_x += (int) dx;
                mouse2_rd_y += (int) dy;
                goto post_event;
            }
            else if(m2plen==4) // fourth byte (logitech mouses)
            {
                 // Logitech extension to Microsoft protocol
                 // This is only sent when CB is pressed.
                mouse2_rd_buttons &= ~0x02;
                mouse2_rd_buttons |= (m2pkt[3] & 0x20) >> 4; // CB => B2
#ifdef DEBUG_MOUSE2
                CONS_Printf( "mouse Logitech buttons %i\n", mouse2_rd_buttons);
#endif
                goto post_event;
            }
            continue;

          case 1:  // MouseSystems
            if(m2plen==5)
            {
#ifdef DEBUG_MOUSE2
                dump_packet( "MS", m2pkt, 5 );
#endif
                // MouseSystems mouse buttons
                mouse2_rd_buttons = MouseSystems_to_button[m2pkt[0] & 0x07];  // true low
                // MouseSystems mouse movement
                int dx = (int8_t)(m2pkt[1]);
                int dy = (int8_t)(m2pkt[2]);
                dx += (int8_t)(m2pkt[3]);
                dy += (int8_t)(m2pkt[4]);
#ifdef DEBUG_MOUSE2
                CONS_Printf( "mouse buttons= %X  dx,dy = ( %i, %i )\n", mouse2_rd_buttons, dx, dy);
#endif
                mouse2_rd_x += dx;
                mouse2_rd_y += dy;
                goto post_event;
            }
            continue;

          case 3:  // PS/2
            if(m2plen==3)
            {
#ifdef DEBUG_MOUSE2
                dump_packet( "PS2", m2pkt, 3 );
#endif
                // PS/2 mouse buttons
                mouse2_rd_buttons = PS2_to_button[m2pkt[0] & 0x07];
                // PS/2 mouse movement
                int dx = (int8_t)((m2pkt[0] & 0x10)<<3);  // signed
                int dy = (int8_t)((m2pkt[0] & 0x20)<<2);  // signed
                dx = (dx<<1) | m2pkt[1];
                dy = (dy<<1) | m2pkt[2];
#ifdef DEBUG_MOUSE2
                CONS_Printf( "mouse buttons= %X  dx,dy = ( %i, %i )\n", mouse2_rd_buttons, dx, dy);
#endif
                mouse2_rd_x += dx;
                mouse2_rd_y += dy;
                goto post_event;
            }
            continue;
        } // switch
        continue;

    post_event:
       {
        // Post mouse2 events
        byte mbk = (mouse2_rd_buttons ^ mouse2_ev_buttons); // changed buttons
        if( mbk )  // button changed, infrequent
        {
            mouse2_ev_buttons = mouse2_rd_buttons;

            int k;
            for(k=0; mbk; k++, mbk>>=1)  // until have processed all changed buttons
            {
                if(mbk & 0x01)  // button changed
                {
                    byte mbm = 1<<k;
                    event.type = (mouse2_rd_buttons & mbm)? ev_keydown : ev_keyup;
                    event.data1= KEY_MOUSE2+k;
                    event.data2= 0;  // must for ev_keydown
                    D_PostEvent(&event);
                }
            }
        }
       }
    }

    // Only post sum of mouse movement, as there is only one movement per tick anyway.
    if( mouse2_rd_x | mouse2_rd_y )  // any movement
    {
        event.type = ev_mouse2;
        event.data1 = 0;
        event.data2 = mouse2_rd_x;
        event.data3 = - mouse2_rd_y;
        D_PostEvent(&event);
    }

#ifdef DEBUG_MOUSE2xx
    if( m2plen > 0 && m2plen < 5 )
    {
       dump_packet("Exit leftover", m2pkt, m2plen );
    }
#endif

    return;

no_input:
    if( staleness < 127 )
    {
        if( ++staleness == 32 )
            m2plen = 7;  // clear leftover
    }

    return;
}

void I_ShutdownMouse2(void)
{
  if(mouse2_fd != -1) close(mouse2_fd);
  mouse2_started = 0;
}

#endif

void I_StartupMouse2 (void)
{
#ifdef DEBUG_MOUSE2
    GenPrintf(EMSG_warn, "I_StartupMouse2\n");
#endif
#ifdef MOUSE2_NIX
    struct termios m2tio;
    int i;

    I_ShutdownMouse2();

    if(cv_usemouse[1].EV == 0)  return;

    #define M2PORT_LEN  24
    char m2port[M2PORT_LEN];
    snprintf( m2port, M2PORT_LEN-2, "/dev/%s", cv_mouse2port.string );
    m2port[M2PORT_LEN-1] = 0;
    mouse2_fd = open( m2port, O_RDONLY|O_NONBLOCK|O_NOCTTY );
    if(mouse2_fd == -1)
    {
        CONS_Printf("Error opening %s\n", m2port);
#ifdef DEBUG_MOUSE2_STIMULUS
        goto mouse2_stimulus;
#else
        return;
#endif
    }

    tcflush(mouse2_fd, TCIOFLUSH);
    m2tio.c_iflag = IGNBRK;
    m2tio.c_oflag = 0;
    m2tio.c_cflag = ( cv_mouse2type.EV == 0 )?
        CREAD|CLOCAL|HUPCL|CSTOPB|B1200 | CS7  // PC mouse
      : CREAD|CLOCAL|HUPCL|CSTOPB|B1200 | CS8; // MS and others
    m2tio.c_lflag = 0;
    m2tio.c_cc[VTIME] = 0;
    m2tio.c_cc[VMIN] = 1;
    tcsetattr(mouse2_fd, TCSANOW, &m2tio);

    // Determine DTR and RTS state from opt string.
    unsigned int  tiocm_0 = 0;
    unsigned int  tiocm_1 = 0;
    int optlen = strlen(cv_mouse2opt.string) - 1;  // need 2 char
    for(i=0; i< optlen; i++)
    {
        char c1 = cv_mouse2opt.string[i];
        char c2 = cv_mouse2opt.string[i+1];
        if( c1=='D' || c1=='d' )
        {
            if(c2 == '-')
	        tiocm_0 |= TIOCM_DTR;  // DTR off
            else
                tiocm_1 |= TIOCM_DTR;  // DTR on
        }
        if( c1=='R' || c1=='r' )
        {
            if(c2 == '-')
                tiocm_0 |= TIOCM_RTS;  // RTS off
            else
                tiocm_1 |= TIOCM_RTS;  // RTS on
        }
    }
    // Set DTR and RTS
    if( tiocm_0 | tiocm_1 )
    {
        unsigned int tiocm_v;
        if(!ioctl(mouse2_fd, TIOCMGET, &tiocm_v)) {
            tiocm_v &= ~tiocm_0;  // DTR,RTS off
            tiocm_v |= tiocm_1;  // DTR,RTS on
            ioctl(mouse2_fd, TIOCMSET, &tiocm_v);
        }
    }
    mouse2_started = 1;
#endif

#ifdef DEBUG_MOUSE2_STIMULUS
mouse2_stimulus:
    if( cv_mouse2type.EV == 0 )
    {
        // Enable mouse2 for fake input.        
        mouse2_started = 1;
        return;
    }
#endif
    return;
}

// return free and total physical memory in the system

#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"

uint64_t I_GetFreeMem(uint64_t *total)
{
#ifdef FREEBSD
    unsigned page_count, free_count, pagesize;
    size_t len = sizeof(unsigned);
    if (sysctlbyname("vm.stats.vm.v_page_count", &page_count, &len, NULL, 0))
      goto guess;
    if (sysctlbyname("vm.stats.vm.v_free_count", &free_count, &len, NULL, 0))
      goto guess;
    if (sysctlbyname("hw.pagesize", &pagesize, &len, NULL, 0))
      goto guess;
    *total = (uint64_t)page_count * pagesize;
    return (uint64_t)free_count * pagesize;
#else
    char buf[1024];    
    char *memTag;
    uint64_t freeKBytes;
    uint64_t totalKBytes;
    int n;
    int meminfo_fd = -1;

    meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
    n = read(meminfo_fd, buf, 1023);
    close(meminfo_fd);
    
    if(n<0)
        goto guess;
    
    buf[n] = '\0';
    if(NULL == (memTag = strstr(buf, MEMTOTAL)))
        goto guess;
        
    memTag += sizeof(MEMTOTAL);
    totalKBytes = atoi(memTag);
    
    if(NULL == (memTag = strstr(buf, MEMFREE)))
        goto guess;
        
    memTag += sizeof(MEMFREE);
    freeKBytes = atoi(memTag);
    
    *total = totalKBytes << 10;
    return freeKBytes << 10;

 guess:
    // make a conservative guess
    *total = (32 << 20) + 0x01;  // guess indicator
    return   0;
#endif
}


void I_Tactile( int   on,
                int   off,
                int   total )
{
  // UNUSED.
  on = off = total = 0;
}

ticcmd_t        emptycmd;
ticcmd_t*       I_BaseTiccmd(void)
{
    return &emptycmd;
}

//
// I_GetTime
// returns time in 1/TICRATE second tics
//
tic_t  I_GetTime (void)
{
    struct timeval      tp;
    struct timezone     tzp;
    int                 newtics;
    static int          oldtics=0;
    static int          basetime=0;
  
again:
    gettimeofday(&tp, &tzp);
    if (!basetime)
        basetime = tp.tv_sec;

    // On systems with RTC drift correction or NTP we need to take
    // care about the system clock running backwards sometimes. Make
    // sure the new tic is later then the last one.
    newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
    if (oldtics && (newtics < oldtics))
    {
        usleep(1);
        goto again;
    }
    oldtics = newtics;
    return newtics;
}


// sleeps for the given amount of milliseconds
void I_Sleep(unsigned int ms)
{
    usleep( ms * 1000 );
#if 0	   
#ifdef SGI
    sginap(1);                                           
#else
#ifdef SUN
    sleep(0);
#else
    usleep (count * (1000000/70) );                                
#endif
#endif
#endif
}

#ifdef LOADING_DISK_ICON
void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}
#endif

#if 0
// Unused
byte*   I_AllocLow(int length)
{
    byte*       mem;
        
    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}
#endif

//
// I_Error
//
void I_Error (const char *error, ...)
{
    va_list     argptr;

    // Message first.
    va_start (argptr,error);
    fprintf (stderr, "Error: ");
    vfprintf (stderr,error,argptr);
    fprintf (stderr, "\n");
    va_end (argptr);

    fflush( stderr );

    D_Quit_Save( QUIT_panic );  // No save, safe shutdown
    
    exit(-1);
}

// The final part of I_Quit, system dependent.
void I_Quit_System (void)
{
    exit(0);
}

   

#define MAX_QUIT_FUNCS     16
typedef void (*quitfuncptr)();
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
               { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
               };
//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
   int c;

   for (c=0; c<MAX_QUIT_FUNCS; c++) {
      if (!quit_funcs[c]) {
         quit_funcs[c] = func;
         break;
      }
   }
}


#if 0
// Unused
//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
   int c;

   for (c=0; c<MAX_QUIT_FUNCS; c++) {
      if (quit_funcs[c] == func) {
         while (c<MAX_QUIT_FUNCS-1) {
            quit_funcs[c] = quit_funcs[c+1];
            c++;
         }
         quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
         break;
      }
   }
}
#endif

// Shutdown joystick and other interfaces, before I_ShutdownGraphics.
void I_Shutdown_IO(void)
{
#ifdef LJOYSTICK
    I_ShutdownJoystick();
#endif
}
     
//
//  Closes down everything. This includes restoring the initial
//  pallete and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
   int c;

   for (c=MAX_QUIT_FUNCS-1; c>=0; c--)
   {
      if (quit_funcs[c])
         (*quit_funcs[c])();
   }
}

uint64_t I_GetDiskFreeSpace(void)
{
  struct statfs stfs;
  if(statfs(".",&stfs)==-1) {
    return INT_MAX;
  }
  return stfs.f_bavail*stfs.f_bsize;
}

char *I_GetUserName(void)
{
  static char username[MAXPLAYERNAME];

  char  *p;
  if((p=getenv("USER"))==NULL)
    if((p=getenv("user"))==NULL)
      if((p=getenv("USERNAME"))==NULL)
        if((p=getenv("username"))==NULL)
          return NULL;

  strncpy(username, p, MAXPLAYERNAME-1);
  username[MAXPLAYERNAME-1] = 0;

  if( strcmp(username,"")==0 )
    return NULL;

  return username;
}


// Get the directory of this program.
//   dirbuf: a buffer of length MAX_WADPATH, 
// Return true when success, dirbuf contains the directory.
boolean I_Get_Prog_Dir( char * defdir, /*OUT*/ char * dirbuf )
{
    int  len;
    char * dnp;

#ifdef FREEBSD
    len = readlink( "/proc/curproc/file", dirbuf, MAX_WADPATH-1 );
    if( len > 1 )
    {
        dirbuf[len] = 0;  // readlink does not terminate string
        goto got_path;
    }
# if 0
    // Sysctl to get program path.
    {
        // FIXME
        int mib[4];
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;
        sysctl(mib, 4, dirbuf, MAX_WADPATH-1, NULL, 0);
    }
# endif
#elif defined( SOLARIS )
    len = readlink( "/proc/self/path/a.out", dirbuf, MAX_WADPATH-1 );
    if( len > 1 )
    {
        dirbuf[len] = 0;  // readlink does not terminate string
        goto got_path;
    }
#  if 0
    // [WDJ] Am missing a few details
    getexecname();
#  endif
#else
    // Linux
    len = readlink( "/proc/self/exe", dirbuf, MAX_WADPATH-1 );
    if( len > 1 )
    {
        dirbuf[len] = 0;  // readlink does not terminate string
        goto got_path;
    }
#endif

    // The argv[0] method
    char * arg0p = myargv[0];
//    GenPrintf(EMSG_debug, "argv[0]=%s\n", arg0p );
    // Linux, FreeBSD, Mac
    if( arg0p[0] == '/' )
    {
        // argv[0] is an absolute path
        strncpy( dirbuf, arg0p, MAX_WADPATH-1 );
        dirbuf[MAX_WADPATH-1] = 0;
        goto got_path;
    }
    // Linux, FreeBSD, Mac
    else if( strchr( arg0p, '/' ) )
    {
        // argv[0] is relative to current dir
        if( defdir )
        {
            cat_filename( dirbuf, defdir, arg0p );
            goto got_path;
        }
    }
    goto failed;
   
got_path:
    // Get only the directory name
    dnp = dirname( dirbuf );
    if( dnp == NULL )  goto failed;
    if( dnp != dirbuf )
    {
        cat_filename( dirbuf, "", dnp );
    }
    return true;

failed:
    dirbuf[0] = 0;
    return false;
}


int  I_mkdir(const char *dirname, int unixright)
{
    return mkdir(dirname, unixright);
}

#if 0
// check if legacy.dat exists in the given path
static boolean isWadPathOk(char *path)
{
    char wad3path[256];
    
    sprintf(wad3path, "%s/%s", path, WADKEYWORD);
    
    if(access(wad3path, R_OK)) {
        return false; // no access
    }
    
    return true;
}

// search for legacy.dat in the given path
static char *searchWad(char *searchDir) 
{
    int pipeDescr[2];
    pid_t childPid;
    boolean finished;
    
    printf("Searching directory '%s' for '%s' ... please wait\n", searchDir, WADKEYWORD);
    
    if(pipe(pipeDescr) == -1) {
        fprintf(stderr, "Unable to open pipe\n");
        return NULL;
    }
    
    // generate child process
    childPid = fork();
    
    if(childPid == -1) {
        fprintf(stderr, "Unable to fork\n");
        return NULL;
    }
    
    if(childPid == 0) { // here comes the child
        close(pipeDescr[0]);
        
        // set stdout to pipe
        dup2(pipeDescr[1], STDOUT_FILENO);
        
        // execute the find command
        execlp("find", "find", searchDir, "-name", WADKEYWORD, NULL);
        exit(1); // shouldn't be reached
    }
    
    // parent
    close(pipeDescr[1]);
    
    // now we have to wait for the output of 'find'
    finished = false;
    
    while(!finished) {
        char *namePtr;
        int pathLen;
        
        pathLen = read(pipeDescr[0], returnWadPath, 256);
        
        if(pathLen == 0) { // end of "file" reached
            return NULL;
        }
        
        if(pathLen == -1) { // should not happen
            fprintf(stderr, "searchWad: reading in non-blocking mode - please fix me\n");
            return NULL;
        }
        
        namePtr = strstr(returnWadPath, WADKEYWORD); // check if we read something sensible
        if(namePtr--) {
            *namePtr = 0; //terminate string before legacy.dat
            finished = true;
        }
    }
    
    // kill child ... oops
    kill(childPid, SIGKILL);
    
    return returnWadPath;
}

// go through all possible paths and look for legacy.dat
static char *locateWad(void)
{
    char *WadPath;
    char *userhome;
    
    // does DOOMWADDIR exist?
    WadPath = getenv("DOOMWADDIR");
    if(WadPath) {
        if(isWadPathOk(WadPath)) {
            return WadPath;
        }
    }
    
    // examine current dir
    strcpy(returnWadPath, ".");
    if(isWadPathOk(returnWadPath)) {
        return returnWadPath;
    }
    
    // examine default dirs
    strcpy(returnWadPath, DEFAULTWADLOCATION1);
    if(isWadPathOk(returnWadPath)) {
        return returnWadPath;
    }
    strcpy(returnWadPath, DEFAULTWADLOCATION2);
    if(isWadPathOk(returnWadPath)) {
        return returnWadPath;
    }
    
    // find in $HOME
    userhome = getenv("HOME");
    if(userhome) {
        WadPath = searchWad(userhome);
        if(WadPath) {
            return WadPath;
        }
    }
    
    // find in /usr/local
    WadPath = searchWad(DEFAULTSEARCHPATH1);
    if(WadPath) {
        return WadPath;
    }
    // find in /usr/games
    WadPath = searchWad(DEFAULTSEARCHPATH2);
    if(WadPath) {
        return WadPath;
    }
    
    // if nothing was found
    return NULL;
}

void I_LocateWad(void)
{
    char *waddir;

    waddir = locateWad();

    if(waddir) {
        chdir(waddir); // change to the directory where we found legacy.dat
    }

    return;
}
#endif



void I_SysInit(void)
{
  CONS_Printf("Linux X11 system ...\n");

#ifdef JOYSTICK_SUPPORT
  // Initialize the joystick subsystem.
  I_InitJoystick();
#endif
  
  // d_main will next call I_StartupGraphics
}
