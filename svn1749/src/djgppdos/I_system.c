// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: I_system.c 1650 2023-10-25 02:25:41Z wesleyjohnson $
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
// $Log: I_system.c,v $
// Revision 1.5  2000/10/21 08:43:32  bpereira
// Revision 1.4  2000/10/02 18:25:46  bpereira
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//     Misc. stuff
//     Startup & Shutdown routines for music,sound,timer,keyboard,...
//     Signal handler to trap errors and exit cleanly.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
  // stdlib, stdio, strings, defines

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <io.h>
#include <stdarg.h>
#include <sys/time.h>

#if defined( DJGPP ) || defined( __DJGPP__ )
 #include <dpmi.h>
 #include <go32.h>
 #include <pc.h>
 #include <dos.h>
 #include <crt0.h>
 #include <sys/segments.h>
 #include <sys/nearptr.h>

 #include <keys.h>
#endif

#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"
#include "i_system.h"
#include "d_net.h"
#include "g_game.h"

#include "d_main.h"

#include "m_argv.h"

#include "w_wad.h"
#include "z_zone.h"
#include "g_input.h"

#include "console.h"

#ifdef __GNUG__
 #pragma implementation "i_system.h"
#endif

#include "i_joy.h"


//### let's try with Allegro ###
#define  alleg_mouse_unused
//#define  alleg_timer_unused
#define  alleg_keyboard_unused
//#define  alleg_joystick_unused
#define  alleg_gfx_driver_unused
#define  alleg_palette_unused
#define  alleg_graphics_unused
#define  alleg_vidmem_unused
#define  alleg_flic_unused
#define  alleg_sound_unused
#define  alleg_file_unused
#define  alleg_datafile_unused
#define  alleg_math_unused
#define  alleg_gui_unused
#include <allegro.h>
//### end of Allegro include ###



// Do not execute cleanup code more than once. See Shutdown_xxx() routines.
byte keyboard_started=false;
byte sound_started=false;
byte timer_started=false;

/* Mouse stuff */
byte mouse_detected=false;

volatile tic_t ticcount;   //returned by I_GetTime(), updated by timer interrupt


#ifdef LOGMESSAGES
#define  LOGFILENAME   "log.txt"
FILE *logstream = NULL;

void  shutdown_logmessage( const char * who, const char * msg )
{
    if( logstream )
    {
        fprintf( logstream, "%s: %s\n", who, msg );
        fclose( logstream );
        logstream = NULL;
    }
}
#endif



void I_Tactile ( int   on,   int   off,   int   total )
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
//  Allocates the base zone memory,
//  this function returns a valid pointer and size,
//  else it should interrupt the program immediately.
//
//added:11-02-98: now checks if mem could be allocated, this is still
//    prehistoric... there's a lot to do here: memory locking, detection
//    of win95 etc...
//
boolean   win95;
boolean   lockmem;

static void I_DetectWin95 (void)
{
    __dpmi_regs     r;

    r.x.ax = 0x160a;        // Get Windows Version
    __dpmi_int(0x2f, &r);

    if(r.x.ax || r.h.bh < 4)    // Not windows or earlier than Win95
    {
        win95 = false;
    }
    else
    {
        CONS_Printf ("Windows 95 detected\n");
        win95 = true;
    }
}

uint64_t I_GetFreeMem(uint64_t *total)
{
    _go32_dpmi_meminfo     info;

    _go32_dpmi_get_free_memory_information(&info);
    if( total )
        *total = info.total_physical_pages<<12; // <<12 for convert page to byte
    return info.available_physical_pages<<12;
}


/*==========================================================================*/
// I_GetTime ()
/*==========================================================================*/
tic_t inline I_GetTime (void)
{
    return ticcount;
}

// Joystick support is enabled in doomdef.h
#ifdef JOYSTICK_SUPPORT
byte joystick_detected = false;
JoyType_t   Joystick;
#endif

//
// I_Init
//


#ifdef JOYSTICK_SUPPORT
void I_WaitJoyButton (void)
{
     CON_Drawer ();
     I_FinishUpdate ();        // page flip or blit buffer

     do {
         poll_joystick();
     } while (!(joy_b1 || joy_b2));

     while (joy_b1 || joy_b2)
         poll_joystick();
}
#endif


#ifdef JOYSTICK_SUPPORT
void I_InitJoystick (void)
{
    //init the joystick
    joystick_detected=0;
    if (cv_usejoystick.value && !M_CheckParm("-nojoy"))
    {
        joy_type = JOY_TYPE_4BUTTON;
        if(initialise_joystick()==0) {

            switch(cv_usejoystick.value) {
               case 1 : joy_type = JOY_TYPE_4BUTTON; break;
               case 2 : joy_type = JOY_TYPE_STANDARD;    break;
               case 3 : joy_type = JOY_TYPE_6BUTTON;     break;
               case 4 : joy_type = JOY_TYPE_WINGEX;      break;
               case 5 : joy_type = JOY_TYPE_FSPRO;       break;
               // new since 1.28 support from allegro 3.11
               case 6 : joy_type = JOY_TYPE_8BUTTON;     break;
               case 7 : joy_type = JOY_TYPE_SIDEWINDER;  break;
               case 8 : joy_type = JOY_TYPE_GAMEPAD_PRO; break;
               case 9 : joy_type = JOY_TYPE_SNESPAD_LPT1;break;
               case 10: joy_type = JOY_TYPE_SNESPAD_LPT2;break;
               case 11: joy_type = JOY_TYPE_SNESPAD_LPT3;break;
               case 12: joy_type = JOY_TYPE_WINGWARRIOR; break;
            }
            // only gamepadstyle joysticks
            Joystick.bGamepadStyle=true;

            CONS_Printf("\2CENTER the joystick and press a button:"); I_WaitJoyButton ();
            initialise_joystick();
            CONS_Printf("\nPush the joystick to the UPPER LEFT corner and press a button\n"); I_WaitJoyButton ();
            calibrate_joystick_tl();
            CONS_Printf("Push the joystick to the LOWER RIGHT corner and press a button\n"); I_WaitJoyButton ();
            calibrate_joystick_br();
            if(joy_type== JOY_TYPE_WINGEX || joy_type == JOY_TYPE_FSPRO)
            {
                CONS_Printf("Put Hat at Center and press a button\n"); I_WaitJoyButton ();
                calibrate_joystick_hat(JOY_HAT_CENTRE);
                CONS_Printf("Put Hat at Up and press a button\n"); I_WaitJoyButton ();
                calibrate_joystick_hat(JOY_HAT_UP);
                CONS_Printf("Put Hat at Down and press a button\n"); I_WaitJoyButton ();
                calibrate_joystick_hat(JOY_HAT_DOWN);
                CONS_Printf("Put Hat at Left and press a button\n"); I_WaitJoyButton ();
                calibrate_joystick_hat(JOY_HAT_LEFT);
                CONS_Printf("Put Hat at Right and press a button\n"); I_WaitJoyButton ();
                calibrate_joystick_hat(JOY_HAT_RIGHT);
            }
            joystick_detected=1;
        }
        else
        {
            CONS_Printf("\2No Joystick detected.\n");
        }
    }
}
#endif




//
// I_Error
//
//added:18-02-98: put an error message (with format) on stderr
void I_OutputMsg (char *error, ...)
{
    va_list     argptr;

    va_start (argptr,error);
    vfprintf (stderr,error,argptr);
    va_end (argptr);

    // dont flush the message!
}


//added 31-12-97 : display error messy after shutdowngfx
void I_Error (const char *error, ...)
{
    va_list     argptr;

    // put message to stderr
    va_start (argptr,error);
    fprintf (stderr, "Error: ");
    vfprintf (stderr,error,argptr);
#ifdef DEBUGFILE
    if (debugfile)
    {
        fprintf (debugfile,"I_Error :");
        vfprintf (debugfile,error,argptr);
    }
#endif

    va_end (argptr);


    D_Quit_Save( QUIT_panic );  // No save, safe shutdown

#ifdef LOGMESSAGES
    shutdown_logmessage( "I_Error()", "shutdown" );
#endif

    fprintf (stderr, "\nPress ENTER");
    fflush( stderr );
    getchar();

    exit(-1);
}


// The system dependent part of I_Quit.
void I_Quit_System (void)
{
#ifdef LOGMESSAGES
    shutdown_logstream( "I_Quit()", "end of logstream" );
#endif
    exit(0);
}


// Show the EndText, after the graphics are shutdown.
void I_Show_EndText( uint16_t * text )
{
#if 1
    puttext(1,1,80,25, (byte*)text);
#else
    byte* endoom;
    // [WDJ] Fix errors during I_Error shutdown.
    int enddoom_num = W_CheckNumForName("ENDDOOM");
    if( enddoom_num < 0 )
        return;  // Avoid repeat errors during bad environment shutdown.

    endoom = W_CacheLumpName(enddoom_num,PU_CACHE);
    puttext(1,1,80,25,endoom);
#endif
    gotoxy(1,24);

    fflush(stderr);
}



// sleeps for the given amount of milliseconds
void I_Sleep(unsigned int ms)
{
    usleep( ms * 1000 );  // unistd
}


#ifdef LOADING_DISK_ICON
//  Fab: this is probably to activate the 'loading' disc icon
//       it should set a flag, that I_FinishUpdate uses to know
//       whether it draws a small 'loading' disc icon on the screen or not
//
//  also it should explicitly draw the disc because the screen is
//  possibly not refreshed while loading
//
void I_BeginRead (void)
{
}

//  Fab: see above, end the 'loading' disc icon, set the flag false
//
void I_EndRead (void)
{
}
#endif

byte*   I_AllocLow(int length)
{
    byte*       mem;

    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;

}

#define MOUSE2
/* Secondary Mouse*/
#ifdef MOUSE2
_go32_dpmi_seginfo oldmouseinfo,newmouseinfo;
boolean mouse2_started=0;
USHORT  mouse2port;
byte    mouse2irq;
// mmh = MicrosoftMouseIntHandler
volatile int     mmh_mouse2buttons;
volatile int     mmh_mouse2x, mmh_mouse2y;
// internal use
volatile int     bytenum;
volatile byte    combytes[8];

//
// support a secondary mouse without mouse driver !
//
// take from the PC-GPE by Mark Feldman
static void I_MicrosoftMouseIntHandler()
{
  char   dx,dy;
  byte   inbyte;

  // Get the port byte
  inbyte = inportb(mouse2port);

  // Make sure we are properly "synched"
  if((inbyte & 64)== 64 || bytenum>7)
      bytenum = 0;

  // Store the byte and adjust bytenum
  combytes[bytenum] = inbyte;
  bytenum++;

  // Have we received all 3 bytes?
  if(bytenum==3)
  {
      // Yes, so process them
      dx = ((combytes[0] & 3) << 6) + combytes[1];
      dy = ((combytes[0] & 12) << 4) + combytes[2];
      mmh_mouse2x += dx;
      mmh_mouse2y += dy;
      mmh_mouse2buttons = (combytes[0] & (32+16)) >>4;
  }
  else
  if(bytenum==4) // for logitech 3 buttons
  {
      if(combytes[3] & 32)
          mmh_mouse2buttons |= 4;
      else
          mmh_mouse2buttons &= ~4;
  }

  // Acknowledge the interrupt
  outportb(0x20,0x20);
}
END_OF_FUNCTION(I_MicrosoftMouseIntHandler);

// wait ms milliseconde
void I_Delay(int ms)
{
    tic_t  starttime;

    if(timer_started)
    {
       starttime=I_GetTime()+(TICRATE*ms)/1000;
       while(starttime>=I_GetTime())
          ;
    }
    else
        delay(ms);
}

//
//  Removes the mouse2 handler.
//
void I_ShutdownMouse2()
{
    event_t event;
    int i;

    if( !mouse2_started )
        return;

    outportb(mouse2port+4,0x00);   // shutdown mouse (DTR & RTS = 0)
    I_Delay(1);
    outportb(mouse2port+1,0x00);   // disable COM interuption

    asm("cli");
    _go32_dpmi_set_protected_mode_interrupt_vector(mouse2irq, &oldmouseinfo);
    _go32_dpmi_free_iret_wrapper(&newmouseinfo);
    asm("sti");

    mmh_mouse2x = mmh_mouse2y = mmh_mouse2buttons = 0;
    // emulate the up of all mouse buttons
    for(i=0;i<MOUSEBUTTONS;i++)
    {
        event.type = ev_keyup;
        event.data1 = KEY_MOUSE2+i;
        D_PostEvent(&event);
    }

    mouse2_started=false;
}

byte   ComIrq[4]={0x0c,0x0b,0x0c,0x0b};
USHORT ComPort[4]={0x3F8,0x2F8,0x3E8,0x2E8};
//
//  Installs the mouse2 handler.
//
void I_StartupMouse2()
{
    int i;
    boolean  found;
    __dpmi_regs r;

    if( mouse2_started )
        I_ShutdownMouse2();

    if(!cv_usemouse[1].value)  // mouse2
        return;

    mmh_mouse2x = mmh_mouse2y = mmh_mouse2buttons = 0;

    mouse2irq =ComIrq[cv_mouse2port.value-1];
    mouse2port=ComPort[cv_mouse2port.value-1];
    CONS_Printf("Using %s (irq %d, port 0x%x)\n",cv_mouse2port.string,mouse2irq-8,mouse2port);
    r.x.ax=0x24;
    __dpmi_int(0x33,&r);
    if(r.h.cl+8==mouse2irq)
    {
        CONS_Printf("Irq conflict with mouse 1\n"
                    "Use mouse2port to change the port\n");
        return;
    }

    // install irq wrapper
    asm("cli");
    _go32_dpmi_get_protected_mode_interrupt_vector(mouse2irq, &oldmouseinfo);
    newmouseinfo.pm_selector=_go32_my_cs();
    newmouseinfo.pm_offset=(int)I_MicrosoftMouseIntHandler;
    _go32_dpmi_allocate_iret_wrapper(&newmouseinfo);
    _go32_dpmi_set_protected_mode_interrupt_vector(mouse2irq, &newmouseinfo);

    LOCK_VARIABLE(bytenum);
    LOCK_VARIABLE(mmh_mouse2x);
    LOCK_VARIABLE(mmh_mouse2y);
    LOCK_VARIABLE(mmh_mouse2buttons);
    LOCK_VARIABLE(mouse2port);
    _go32_dpmi_lock_data(combytes,sizeof(combytes));
    LOCK_FUNCTION(I_MicrosoftMouseIntHandler);
    asm("sti");

    outportb(mouse2port+4,0   );   // shutdown mouse (DTR & RTS = 0)
    I_Delay(1);
    outportb(mouse2port+1,0   );   // disable COM interuption
    I_Delay(1);
    outportb(mouse2port+3,0x80);   // change status of port +0 et +1
    I_Delay(1);                    // for baudrate programmation
    outportb(mouse2port  ,0x60);   // 1200 LSB
    I_Delay(1);
    outportb(mouse2port+1,0   );   // 1200 MSB
    I_Delay(1);
    outportb(mouse2port+3,0x02);   // set port protocol 7N1
    I_Delay(1);
    outportb(mouse2port+1,0x01);   // enable COM interuption
    I_Delay(1);
    outportb(0x21,0x0);

    // wait to be sure the mouse have shutdown
    I_Delay(100);

    outportb(mouse2port+4,0x0b);   // restart mouse
    i=I_GetTime()+TICRATE;
    found=(cv_usemouse[1].value==2);  // found mouse2
    while (I_GetTime()<i || !found)
    {
       if(combytes[0]!='M')
          found=true;
    }

    if(found || (cv_usemouse[1].value==2))
    {
        CONS_Printf("Microsoft compatible Secondary Mouse detected\n");

        //register shutdown mouse2 code.
        I_AddExitFunc(I_ShutdownMouse2);
        mouse2_started = true;
    }
    else
    {
        CONS_Printf("Secondary Mouse not found\n");
        // remove irq wraper
        I_ShutdownMouse2();
    }
}
#endif

//  Initialise the mouse. Does not need to be shutdown.
//
//   play_mode : enable mouse containment during play
void I_StartupMouse ( boolean play_mode )
{
    __dpmi_regs r;

    // mouse detection may be skipped by setting usemouse false
    if(cv_usemouse[0].value == 0)  // mouse1
    {
        mouse_detected=false;
        I_ShutdownMouse2();
        return;
    }

    // The mouse cannot escape the window, so play_mode has no effect.

    //detect mouse presence
    r.x.ax=0;
    __dpmi_int(0x33,&r);

    //added:03-01-98:
    if( r.x.ax == 0 && (cv_usemouse[0].value != 2))
    {
        mouse_detected=false;
        CONS_Printf("\2I_StartupMouse: mouse not present.\n");
    }
    else
    {
        mouse_detected=true;

        //hide cursor
        r.x.ax=2;
        __dpmi_int(0x33,&r);

        //reset mickey count
        r.x.ax=0x0b;
        __dpmi_int(0x33,&r);
    }
}

void I_GetEvent (void)
{
    __dpmi_regs r;
    event_t event;

#ifdef MOUSE2
    // mouse may be disabled during the game by setting usemouse false
    if (mouse2_started)
    {
        //mouse movement
        static unsigned int last_mouse2buttons = 0;

        // post key event for buttons
        unsigned int dbut2=(mmh_mouse2buttons ^ last_mouse2buttons);  // only those that changed
        if (dbut2)
        {
            last_mouse2buttons = mmh_mouse2buttons;

            unsigned int bm=1; // button bit mask
            int i;  // button index
            for(i=0; i<MOUSEBUTTONS; i++,bm<<=1)  // step through the buttons by bit mask
            {
                if(dbut2 & bm)
                {
                    if(mmh_mouse2buttons & bm)
                       event.type = ev_keydown;
                    else
                       event.type = ev_keyup;
                    event.data1 = KEY_MOUSE2+i;
                    D_PostEvent(&event);
                }
            }
        }

        if ((mmh_mouse2x != 0)||(mmh_mouse2y != 0))
        {
            event.type = ev_mouse2;
            event.data1 = 0;
//          event.data1=buttons;    // not needed
            event.data2 = mmh_mouse2x<<1;
            event.data3 = -mmh_mouse2y<<1;

            D_PostEvent(&event);
            mmh_mouse2x = 0;
            mmh_mouse2y = 0;
        }

    }
#endif
    if(mouse_detected)
    {
        static unsigned int last_buttons=0;
        unsigned int buttons;

        //mouse movement
        int xmickeys,ymickeys;

        r.x.ax=0x0b;           // ask for the movement, not the position
        __dpmi_int(0x33,&r);
        xmickeys=(signed short)r.x.cx;
        ymickeys=(signed short)r.x.dx;
        r.x.ax=0x03;
        __dpmi_int(0x33,&r);
        buttons=r.x.bx;

        // post key event for buttons
        unsigned int dbut=(buttons ^ last_buttons); // only those that changed
        if (dbut)
        {
            last_buttons = buttons;

            unsigned int bm=1;  // button bit mask
            int i;  // button index
            for(i=0; i<MOUSEBUTTONS; i++,bm<<=1)  // step through the buttons by bit mask
            {
                if(dbut & bm)
                {
                    if(buttons & bm)
                       event.type = ev_keydown;
                    else
                       event.type = ev_keyup;
                    event.data1 = KEY_MOUSE1 + i;
                    D_PostEvent(&event);
                }
            }
        }

        if ((xmickeys!=0)||(ymickeys!=0))
        {
          event.type = ev_mouse;
          event.data1 = 0;
//          event.data1=buttons;    // not needed
          event.data2 = xmickeys;
          event.data3 = -ymickeys;

          D_PostEvent(&event);
        }

    }
#ifdef JOYSTICK_SUPPORT
    //joystick
    if (joystick_detected)
    {
        static unsigned int last_joybuttons=0;
        unsigned int joybuttons;

        poll_joystick();
        // I assume that true is 1
        joybuttons=joy_b1+(joy_b2<<1)+(joy_b3<<2)+(joy_b4<<3)
                         +(joy_b5<<4)+(joy_b6<<5)+(joy_b7<<6)+(joy_b8<<7);

        switch(joy_hat) {
          case JOY_HAT_UP   : joybuttons|=1<<10;break;
          case JOY_HAT_DOWN : joybuttons|=1<<11;break;
          case JOY_HAT_LEFT : joybuttons|=1<<12;break;
          case JOY_HAT_RIGHT: joybuttons|=1<<13;break;
        }

        // post key event for buttons
        unsigned int djoy = (joybuttons ^ lastjoybuttons); // only those that changed
        if (djoy)
        {
            last_joybuttons = joybuttons;

            unsigned int jm=1;  // joy button bit mask
            int i;  // joy button index
            for(i=0; i<JOYBUTTONS; i++,jm<<=1)  // step through the joysticks by bit mask
            {
                if(djoy & jm) // test each bit and post the corresponding event
                {
                    if(joybuttons & jm)
                       event.type = ev_keydown;
                    else
                       event.type = ev_keyup;
                    event.data1 = KEY_JOY0BUT0 + i;
                    D_PostEvent(&event);
                }
            }
        }

        event.type = ev_joystick;
        event.data1 = 0;
        event.data2 = 0;
        event.data3 = 0;

        if(joy_left)
           event.data2 = -1;
        if(joy_right)
           event.data2 = 1;
        if(joy_up)
           event.data3 = -1;
        if(joy_down)
           event.data3 = 1;

        D_PostEvent(&event);
    }
#endif
}

//
//  Timer user routine called at ticrate.
//
void I_TimerISR (void)
{
   //  IO_PlayerInput();      // old doom did that
   ticcount++;

}
END_OF_FUNCTION(I_TimerISR);


//added:08-01-98: we don't use allegro_exit() so we have to do it ourselves.
void I_ShutdownTimer (void)
{
    if( !timer_started )
        return;
    remove_timer();
}


//
//  Installs the timer interrupt handler with timer speed as TICRATE.
//
void I_StartupTimer(void)
{
   ticcount = 0;

   //lock this from being swapped to disk! BEFORE INSTALLING
   LOCK_VARIABLE(ticcount);
   LOCK_FUNCTION(I_TimerISR);

   if( install_timer() != 0 )
      I_Error("I_StartupTimer: could not install timer.");

   if( install_int_ex( I_TimerISR, BPS_TO_TIMER(TICRATE) ) != 0 )
      //should never happen since we use only one.
      I_Error("I_StartupTimer: no room for callback routine.");

   //added:08-01-98: remove the timer explicitly because we don't use
   //                Allegro 's allegro_exit() shutdown code.
   I_AddExitFunc(I_ShutdownTimer);
   timer_started = true;
}


//added:07-02-98:
//
//
uint16_t ASCIINames[128] =
{
//  0      1      2      3      4      5      6      7
//  8      9      A      B      C      D      E      F
    0,    27,   '1',   '2',   '3',   '4',   '5',   '6',
  '7',   '8',   '9',   '0',   '-',   '=', KEY_BACKSPACE, KEY_TAB,
  'q',   'w',   'e',   'r',   't',   'y',   'u',   'i',
  'o',   'p',   '[',   ']', KEY_ENTER, KEY_LCTRL,  'a',  's',
  'd',   'f',   'g',   'h',   'j',   'k',   'l',   ';',
 '\'',   '`', KEY_LSHIFT,  '\\',  'z',  'x',  'c',  'v',
  'b',   'n',   'm',   ',',   '.',   '/', KEY_RSHIFT,  '*',
  KEY_LALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
  KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KEYPAD7,
  KEY_KEYPAD8, KEY_KEYPAD9, KEY_MINUSPAD, KEY_KEYPAD4, KEY_KEYPAD5, KEY_KEYPAD6, KEY_PLUSPAD, KEY_KEYPAD1,
  KEY_KEYPAD2, KEY_KEYPAD3, KEY_KEYPAD0, KEY_KPADPERIOD, 0, 0, 0,  KEY_F11,
  KEY_F12, 0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
};


volatile int pausepressed=0;
volatile char nextkeyextended;

static void I_KeyboardHandler()
{
    unsigned char ch;
    event_t       event;

    ch=inportb(0x60);

    if(pausepressed>0)
        pausepressed--;
    else
    {
        if(ch==0xE1) // pause key
        {
          event.type=ev_keydown;
          event.data1=KEY_PAUSE;
          D_PostEvent(&event);
          pausepressed=5;
        }
        else
        if(ch==0xE0) // extended key handled at next call
        {
          nextkeyextended=1;
        }
        else
        {
          if((ch&0x80)==0)
            event.type=ev_keydown;
          else
            event.type=ev_keyup;

          ch&=0x7f;

          if(nextkeyextended)
          {
            nextkeyextended=0;

            if(ch==70)  // crtl-break
            {
#if 0
// Really old
                asm ("movb $0x79, %%al
                     call ___djgpp_hw_exception"
                     : : :"%eax","%ebx","%ecx","%edx","%esi","%edi","memory");
#else
// DJGPP version ?
                asm ("movb $0x79, %%al\n"
                     "call ___djgpp_hw_exception"
                     : : :"%eax","%ebx","%ecx","%edx","%esi","%edi","memory");
#endif
            }

            // remap lonely keypad slash
            if (ch==53)
                event.data1 = KEY_KPADSLASH;
            else
            // remap the bill gates keys...
            if (ch>=91 && ch<=93)
                event.data1 = ch + 0x80;    // leftwin, rightwin, menu
            else
            // remap non-keypad extended keys to a value<128, but
            // make them different than the KEYPAD keys.
            if (ch>=71 && ch<=83)
                event.data1 = 0x80 + ch + 30;
            else if (ch==28)
                event.data1 = KEY_ENTER;    // keypad enter -> return key
            else if (ch==29)
                event.data1 = KEY_LCTRL;     // rctrl -> lctrl
            else if (ch==56)
                event.data1 = KEY_LALT;      // ralt -> lalt
            else
                ch = 0;
            if (ch)
                D_PostEvent(&event);
          }
          else
          {
            if (ASCIINames[ch]!=0)
              event.data1=ASCIINames[ch];
            else
              event.data1=ch+0x80;
            D_PostEvent(&event);
          }
        }
    }

    outportb(0x20,0x20);
}
END_OF_FUNCTION(I_KeyboardHandler);

//  Return a key that has been pushed, or 0
//  (replace getchar() at game startup)
//
int I_GetKey (void)
{
    if( keyboard_started )
    {
    event_t   *ev;

    if (eventtail != eventhead)
    {
        ev = &events[eventtail];
        eventtail = (++eventtail)&(MAXEVENTS-1);
        if (ev->type == ev_keydown)
            return ev->data1;
        else
            return 0;
    }
    return 0;
    }

    // keyboard not started use the bios call trouth djgpp
    if(_conio_kbhit())
    {
        int key=getch();
        if(key==0) key=getch()+256;
        return key;
    }
    else
        return 0;

}

/* Keyboard handler stuff */
_go32_dpmi_seginfo oldkeyinfo,newkeyinfo;

//
//  Removes the keyboard handler.
//
void I_ShutdownKeyboard()
{
    if( !keyboard_started )
        return;

    asm("cli");
    _go32_dpmi_set_protected_mode_interrupt_vector(9, &oldkeyinfo);
    _go32_dpmi_free_iret_wrapper(&newkeyinfo);
    asm("sti");

    keyboard_started=false;
}

//
//  Installs the keyboard handler.
//
void I_StartupKeyboard()
{
    if(keyboard_started)
        return;

    nextkeyextended=0;

    asm("cli");
    _go32_dpmi_get_protected_mode_interrupt_vector(9, &oldkeyinfo);
    newkeyinfo.pm_offset=(int)I_KeyboardHandler;
    newkeyinfo.pm_selector=_go32_my_cs();
    _go32_dpmi_allocate_iret_wrapper(&newkeyinfo);
    _go32_dpmi_set_protected_mode_interrupt_vector(9, &newkeyinfo);

    LOCK_VARIABLE(nextkeyextended);
    LOCK_VARIABLE(pausepressed);
    _go32_dpmi_lock_data(ASCIINames,sizeof(ASCIINames));
    LOCK_FUNCTION(I_KeyboardHandler);

    _go32_dpmi_lock_data(events,sizeof(events));
    LOCK_VARIABLE(eventhead);
    LOCK_FUNCTION(D_PostEvent);

    asm("sti");

    //added:08-01-98:register shutdown keyboard code.
    I_AddExitFunc(I_ShutdownKeyboard);
    keyboard_started = true;
}




//added:08-01-98:
//
//  Clean Startup & Shutdown handling, as does Allegro.
//  We need this services for ourselves too, and we don't want to mix
//  with Allegro, because someone might not use Allegro.
//  (all 'exit' was renamed to 'quit')
//
#define MAX_QUIT_FUNCS     16
typedef void (*quitfuncptr)();
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
               { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
               };


//added:08-01-98:
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


//added:08-01-98:
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



 //added:03-01-98:
//
// signal_handler:
//  Used to trap various signals, to make sure things get shut down cleanly.
//
static void exception_handler(int num)
{
static char msg[255];
    sprintf(msg, "%s\r\n"
            "This is an error of Legacy, try to send the following info to programmers\r\n", VERSION_BANNER);

    //D_Quit_NetGame ();  //say 'byebye' to other players when your machine
                        // crashes?... hmm... do they have to die with you???

    I_ShutdownSystem();

    _write(STDERR_FILENO, msg, strlen(msg));

    signal(num, SIG_DFL);
    raise(num);
    // TODO: write it in a log !!
}

static void break_handler(int num)
{
static char msg[] = "Oh no! Back to reality!\r\n";

    //D_Quit_NetGame ();  //say 'byebye' to other players when your machine
                        // crashes?... hmm... do they have to die with you???

    I_ShutdownSystem();

    _write(STDERR_FILENO, msg, sizeof(msg)-1);

    signal(num, SIG_DFL);
    raise(num);
}



//added:08-01-98: now this replaces allegro_init()
//
//  REMEMBER: THIS ROUTINE MUST BE STARTED IN i_main.c BEFORE D_DoomMain()
//
//  This stuff should get rid of the exception and page faults when
//  Doom bugs out with an error. Now it should exit cleanly.
//
void  I_StartupSystem(void)
{
    I_DetectWin95 ();

   // some 'more globals than globals' things to initialize here ?
   graphics_state = VGS_startup;
   keyboard_started = false;
   sound_started = false;
   timer_started = false;
#ifdef CDMUS
   cdaudio_started = false;
#endif

   // check for OS type and version here ?


   signal(SIGABRT, exception_handler);
   signal(SIGFPE , exception_handler);
   signal(SIGILL , exception_handler);
   signal(SIGSEGV, exception_handler);
   signal(SIGINT , break_handler);
   signal(SIGKILL, break_handler);
   signal(SIGQUIT, break_handler);

#ifdef LOGMESSAGES
    logstream = fopen( LOGFILENAME, "w");
#endif
}

// Init system called by d_main
void I_SysInit(void)
{
    CONS_Printf("DOS system ...\n");

    I_StartupSystem();

#ifdef JOYSTICK_SUPPORT
    // Initialize the joystick subsystem.
    I_InitJoystick();
#endif

    // d_main will next call I_StartupGraphics
}


// Shutdown joystick and other interfaces, before I_ShutdownGraphics.
void I_Shutdown_IO(void)
{
#ifdef JOYSTICK_SUPPORT
//    I_ShutdownJoystick();
#endif
}

//added:08-01-98:
//
//  Closes down everything. This includes restoring the initial
//  pallete and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
//
void I_ShutdownSystem()
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
    struct diskfree_t df;
    if(_dos_getdiskfree(0,&df))
    {
        return (uint64_t) df.avail_clusters *
                     (unsigned long)df.bytes_per_sector *
                     (unsigned long)df.sectors_per_cluster;
    }
    else
#if 1      
        return (uint64_t) INT_MAX;  // consistent with SDL port
#else
        return (uint64_t) LONG_MAX; // maybe too large for display
#endif
}

char * I_GetUserName(void)
{
static char username[MAXPLAYERNAME];
    
     char * p = getenv("USER");
     if(p==NULL)
     {
         if((p=getenv("user"))==NULL)
            if((p=getenv("USERNAME"))==NULL)
               if((p=getenv("username"))==NULL)
                  return NULL;
     }
     strncpy(username,p,MAXPLAYERNAME);

     if( strcmp(username,"")==0 )
         return NULL;
     return username;
}


// Get the directory of this program.
//   defdir: the current directory
//   dirbuf: a buffer of length MAX_WADPATH, 
// Return true when success, dirbuf contains the directory.
boolean I_Get_Prog_Dir( char * defdir, /*OUT*/ char * dirbuf )
{
    char * dnp;

    // The argv[0] method
    char * arg0p = myargv[0];
//    GenPrintf(EMSG_debug, "argv[0]=%s\n", arg0p );
    // Windows, DOS, OS2
    if( arg0p[0] == '\\' )
    {
        // argv[0] is an absolute path
        strncpy( dirbuf, arg0p, MAX_WADPATH-1 );
        dirbuf[MAX_WADPATH-1] = 0;
        goto got_path;
    }
    // Windows, DOS, OS2
    else if( strchr( arg0p, '/' ) || strchr( arg0p, '\\' ) )
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
    return mkdir(dirname,unixright);
}
