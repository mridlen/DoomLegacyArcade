// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_sys.c 1649 2023-10-25 02:11:02Z wesleyjohnson $
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
// $Log: win_sys.c,v $
// Revision 1.19  2001/08/07 00:54:40  hurdler
// MD2 implementation is getting better but still need lots of work
//
// Revision 1.18  2001/08/03 15:16:13  hurdler
// Fix small bugs (win2k timer + status bar)
//
// Revision 1.17  2001/05/06 09:50:35  hurdler
// fix dx8+ sdk compatibility
//
// Revision 1.16  2001/04/27 13:32:15  bpereira
//
// Revision 1.15  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.14  2001/01/21 04:33:35  judgecutor
//
// Revision 1.13  2001/01/06 22:21:08  judgecutor
// Added NoDirectInput mouse input
//
// Revision 1.12  2000/11/26 00:47:59  hurdler
// Please fix this so it works under ALL version of win32
//
// Revision 1.11  2000/10/23 19:25:50  judgecutor
// Fixed problem with mouse wheel event
//
// Revision 1.10  2000/10/21 08:43:32  bpereira
// Revision 1.9  2000/10/02 18:25:47  bpereira
// Revision 1.8  2000/09/28 20:57:22  bpereira
// Revision 1.7  2000/09/01 19:34:38  bpereira
// Revision 1.6  2000/08/10 19:58:05  bpereira
// Revision 1.5  2000/08/03 17:57:42  bpereira
// Revision 1.4  2000/04/23 16:19:52  bpereira
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      win32 system i/o
//      Startup & Shutdown routines for music,sound,timer,keyboard,...
//      Signal handler to trap errors and exit cleanly.
//
//-----------------------------------------------------------------------------

// Because of WINVER redefine, doomtype.h (via doomincl.h) is before any
// other include that might define WINVER
#include "doomincl.h"

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdarg.h>
#include <direct.h>
// unistd is already used throughout DoomLegacy
#include <unistd.h>
  // usleep
#ifdef MSH_WHEEL
#include <winuser.h>
#endif

#include "i_video.h"
#include "i_sound.h"
#include "i_system.h"
  // graphics_state
#include "i_joy.h"

#include "m_misc.h"
#include "d_net.h"
#include "g_game.h"

#include "d_main.h"

#include "m_argv.h"

#include "w_wad.h"
#include "z_zone.h"
#include "g_input.h"

#include "keys.h"
#include "screen.h"
  // vid

#include "fabdxlib.h"

#include <mmsystem.h>

#include "win_main.h"

#define INITGUID
// Force dinput.h to generate old DX3 headers.
// DIRECTINPUT can use version 0x0300, 0x0500, 0x0700
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

// judgecutor: Wheel support for Win95/WinNT3.51
#include <zmouse.h>
  // WM_MOUSEWHEEL

// judgecutor: Taken from Win98/NT4.0
#ifndef SM_MOUSEWHEELPRESENT
#define SM_MOUSEWHEELPRESENT 75
#endif


// ==================
// DIRECT INPUT STUFF
// ==================
// Drop back to second choice of compatibility, NT
#define REQ_DX0300  0x0300
byte   have_DX0300;    // got DirectInput 0x0300 version instead

static LPDIRECTINPUT           DI_main = NULL;    // DirectInput main object
static LPDIRECTINPUTDEVICE     DID_keyboard = NULL;   // Keyboard
static LPDIRECTINPUTDEVICE     DID_mouse = NULL;   // mice

volatile tic_t ticcount;   //returned by I_GetTime(), updated by timer interrupt


// Do not execute cleanup code more than once. See Shutdown_xxx() routines.
byte    keyboard_started=0;
boolean timer_started = false;
boolean mouse_enabled = false;

static void I_ShutdownKeyboard (void);
#ifdef JOYSTICK_SUPPORT
static void I_ShutdownJoystick (void);
#endif

//
// Force feedback here ? :)
//
void I_Tactile ( int   on,   int   off,   int   total )
{
    // UNUSED.
    on = off = total = 0;
}


//
// Why would this be system specific ?? hmmmm....
//
// BP: yes it is for virtual reality system, next incoming feature :)
ticcmd_t        emptycmd;

ticcmd_t*       I_BaseTiccmd(void)
{
    return &emptycmd;
}


//added:11-02-98: now checks if mem could be allocated, this is still
//    prehistoric... there's a lot to do here: memory locking, detection
//    of win95 etc...
//
boolean   win95;
boolean   winnt;

static void I_DetectWin95 (void)
{
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx (&osvi);

    winnt = (osvi.dwPlatformId==VER_PLATFORM_WIN32_NT);
    // 95 or 98 what the hell
    win95 = true;
}


// return free and total memory in the system
// total is required
uint64_t I_GetFreeMem(uint64_t *total)
{
#if defined(WIN_LARGE_MEM) && defined( _WIN32_WINNT ) && (_WIN32_WINNT >= 0x0500)
    // large memory status, only in newer libraries
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    *total = statex.ullTotalPhys;
    return statex.ullAvailPhys;
#else
    // older memory status
    MEMORYSTATUS info;

    info.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus( &info );
    *total = info.dwTotalPhys;
    return info.dwAvailPhys;
#endif
}


#ifdef ENABLE_PROFILE
// [WDJ] Unused, incomplete, commented out call
// ---------
// I_Profile
// Two little functions to profile our code using the high resolution timer
// ---------
static LARGE_INTEGER    ProfileCount;

void I_BeginProfile (void)
{
    if (!QueryPerformanceCounter (&ProfileCount))
        //can't profile without the high res timer
        I_Error ("I_BeginProfile FAILED");  
}
//faB: we're supposed to use this to measure very small amounts of time,
//     that's why we return a DWORD and not a 64bit value
DWORD I_EndProfile (void)
{
    LARGE_INTEGER   CurrTime;
    DWORD           ret;
    if (!QueryPerformanceCounter (&CurrTime))
        I_Error ("I_EndProfile FAILED");
    if (CurrTime.QuadPart - ProfileCount.QuadPart > (LONGLONG)0xFFFFFFFFUL)
        I_Error ("I_EndProfile OVERFLOW");
    ret = (DWORD)(CurrTime.QuadPart - ProfileCount.QuadPart);
    // we can call I_EndProfile() several time, I_BeginProfile() need be called just once
    ProfileCount = CurrTime;
    
    return ret;
}

#endif

// ---------
// I_GetTime
// Use the High Resolution Timer if available,
// else use the multimedia timer which has 1 millisecond precision on Windowz 95,
// but lower precision on Windowz NT
// ---------
static long    hacktics = 0;       //faB: used locally for keyboard repeat keys
static DWORD   starttickcount = 0; //Hurdler: hack for win2k time bug

tic_t I_GetTime (void)
{
    int newtics;

    if (!starttickcount) // high precision timer
    {
        LARGE_INTEGER   currtime;       // use only LowPart if high resolution counter not available
        static LARGE_INTEGER basetime = {{0}};

        // use this if High Resolution timer is found
        static LARGE_INTEGER frequency;
    
        if (!basetime.LowPart)
        {
            if (!QueryPerformanceFrequency (&frequency))
                frequency.QuadPart = 0;
            else
                QueryPerformanceCounter (&basetime);
        }

        if (frequency.LowPart &&
            QueryPerformanceCounter (&currtime))
        {
            newtics = (int)((currtime.QuadPart - basetime.QuadPart) * TICRATE / frequency.QuadPart);
        }
        else
        {
            currtime.LowPart = timeGetTime();
            if (!basetime.LowPart)
                basetime.LowPart = currtime.LowPart;
            newtics = ((currtime.LowPart-basetime.LowPart)/(1000/TICRATE));
        }
    }
    else
    {
        newtics = (GetTickCount() - starttickcount)/(1000/TICRATE);
    }
    hacktics = newtics;    //faB: just a local counter for keyboard repeat key
    return newtics;
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
void I_BeginRead (void) {}


//  Fab: see above, end the 'loading' disc icon, set the flag false
//
void I_EndRead (void) {}
#endif


#if 0
// unused
byte*   I_AllocLow(int length)
{
    byte*       mem;
        
    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}
#endif


// ===========================================================================
//                                                         EVENTS
// ===========================================================================

// ----------
// I_GetEvent
// Post new events for all sorts of user-input
// ----------
static void I_GetKeyboardEvents (void);
static void I_GetMouseEvents (void);
#ifdef JOYSTICK_SUPPORT
static void I_GetJoystickEvents (void);
#endif


void I_GetEvent (void)
{
    I_GetKeyboardEvents ();
    I_GetMouseEvents ();
#ifdef JOYSTICK_SUPPORT
    I_GetJoystickEvents ();
#endif
}


// ----------
// I_OsPolling
// ----------
void I_OsPolling(void)
{
    MSG  msg;

    //faB: we need to dispatch messages to the window
    //     so the window procedure can respond to messages and PostEvent() for keys
    //     during D_DoomMain startup.
    // BP: this on replace the main loop of windows since I_OsPolling is called in the main loop
    do {
        while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (GetMessage( &msg, NULL, 0, 0))
            {
                TranslateMessage(&msg); 
                DispatchMessage(&msg);
            }
            else// winspec : this is quit message
                I_Quit ();
        }
        if(!appActive && !netgame)
            WaitMessage();
    } while(!appActive && !netgame);

    // this is called by the network synchronization,
    // check keys and allow escaping
    I_GetEvent();

    // reset "emalated keys"
    gamekeydown[KEY_MOUSEWHEELUP] = 0;
    gamekeydown[KEY_MOUSEWHEELDOWN] = 0;
}


// ===========================================================================
//                                                         TIMER
// ===========================================================================

//
//  Timer user routine called at ticrate.
//
void winTimer (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
        ticcount++;
}


#ifdef USE_MYTIMER
//added:08-01-98: we don't use allegro_exit() so we have to do it ourselves.
static  UINT    myTimer;
#endif

void I_ShutdownTimer (void)
{
#ifdef USE_MYTIMER
    if (myTimer)
          KillTimer (hWnd_main, myTimer);
#endif
    timer_started = false;
}


//
//  Installs the timer interrupt handler with timer speed as TICRATE.
//
#define TIMER_ID            1
#define TIMER_RATE          (1000/TICRATE)

void I_StartupTimer(void)
{
    ticcount = 0;

#ifdef USE_MYTIMER
    if ( (myTimer =SetTimer (hWnd_main, TIMER_ID, TIMER_RATE, winTimerISR)) == 0)
          I_Error("I_StartupTimer: could not install timer.");
#endif
    
    //I_AddExitFunc(I_ShutdownTimer);
    timer_started = true;

    //Hurdler: added for win2k time bug
    if (M_CheckParm("-gettickcount"))
    {
    	starttickcount = GetTickCount();
        CONS_Printf("Using GetTickCount()\n");
    }
}


// ===========================================================================
//                                        EXIT CODE, ERROR HANDLING
// ===========================================================================


void  shutdown_logstream( const char * who, const char * msg );

#if defined(NDEBUG)
//
//  Used to trap various signals, to make sure things get shut down cleanly.
//
static void signal_handler(int num)
{
    static char msg[] = "none";
    char*       sigmsg;
    char        sigdef[64];
        
    I_ShutdownSystem();

    switch (num)
    {
    case SIGINT:
        sigmsg = "interrupt";
        break;
    case SIGILL:
        sigmsg = "illegal instruction - invalid function image";
        break;
    case SIGFPE:
        sigmsg = "floating point exception";
        break;
    case SIGSEGV:
        sigmsg = "segment violation";
        break;
    case SIGTERM:
        sigmsg = "Software termination signal from kill";
        break;
    case SIGBREAK:
        sigmsg = "Ctrl-Break sequence";
        break;
    case SIGABRT:
        sigmsg = "abnormal termination triggered by abort call";
        break;
    default:
        sprintf(sigdef,"signal number %d", num);
        sigmsg = sigdef;
    }
    
#ifdef LOGMESSAGES
    shutdown_logmessage( "signal_handler()", sigmsg );
#endif    
    
    MessageBox(hWnd_main,va("signal_handler() : %s",sigmsg),"Doom Legacy error",MB_OK|MB_ICONERROR);
        
    signal(num, SIG_DFL);               //default signal action
    raise(num);
}
#endif


void I_MsgBox (char * msg )
{
    MessageBox (hWnd_main, msg, "Doom Legacy", MB_OK);
}

//
// put an error message (with format) on stderr
//
void I_OutputMsg (char *error, ...)
{
    va_list     argptr;
    char        txt[512];
        
    va_start (argptr,error);
    vsprintf (txt,error,argptr);
    va_end (argptr);

    // All messages already have formatting and \n.
//    fprintf (stderr, "Error: %s\n", txt);
    fprintf (stderr, "%s", txt);
    // dont flush the message!

#ifdef LOGMESSAGES_xx
    // Already handled by console
    if (logstream)
        fprintf (logstream, "%s", txt);
#endif
}



// display error messy after shutdowngfx
//
void I_Error (const char *error, ...)
{
    va_list     argptr;
    char        txt[512];

    // put message to stderr
    va_start (argptr,error);
    wvsprintf (txt, error, argptr);
    va_end (argptr);

    CONS_Printf("I_Error(): %s\n",txt);
    //wsprintf (stderr, "I_Error(): %s\n", txt);
        
    D_Quit_Save( QUIT_panic );  // No save, safe shutdown

#ifdef LOGMESSAGES
    shutdown_logmessage( "I_Error()", "shutdown" );
#endif

    MessageBox (hWnd_main, txt, "Doom Legacy Error", MB_OK|MB_ICONERROR);

    //getchar();
    exit (-1);
}


// The final part of I_Quit, system dependent.
void I_Quit_System (void)
{
#ifdef LOGMESSAGES
    shutdown_logstream( "I_Quit()", "end of logstream" );
#endif

    // cause an error to test the exception handler
    //i = *((unsigned long *)0x181596);

    fflush(stderr);
    exit(0);
}



// --------------------------------------------------------------------------
// I_ShowLastError
// Displays a GetLastError() error message in a MessageBox
// --------------------------------------------------------------------------
void I_GetLastErrorMsgBox (void)
{
    LPVOID lpMsgBuf;

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    // Display the string.
    MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

    // put it in console too and log if any 
    CONS_Printf("Error : %s\n",lpMsgBuf);

    // Free the buffer.
    LocalFree( lpMsgBuf );
}


// ===========================================================================
// CLEAN STARTUP & SHUTDOWN HANDLING, JUST CLOSE EVERYTHING YOU OPENED.
// ===========================================================================
//
//
#define MAX_QUIT_FUNCS     16

typedef void (*quitfuncptr)();

static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] = {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


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
//  Removes a function from the list that need to be called by I_SystemShutdown().
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


// ===========================================================================
// DIRECT INPUT HELPER CODE
// ===========================================================================


// Create a DirectInputDevice interface,
// create a DirectInputDevice2 interface if possible
// in context of main input DI_main
static void create_device_intf ( REFGUID         pguid,
                            LPDIRECTINPUTDEVICE* lpDEV,
                            LPDIRECTINPUTDEVICE2* lpDEV2 )
{
    HRESULT hr, hr2;
    LPDIRECTINPUTDEVICE lpdid1;
    LPDIRECTINPUTDEVICE2 lpdid2 = NULL;

    hr = DI_main->lpVtbl->CreateDevice (DI_main, pguid, &lpdid1, NULL);

    if (FAILED (hr))
        I_Error ("Could not create IDirectInput device");

    // get Device2 but only if we are not in DirectInput version 3
    if ( lpDEV2 != NULL )
    {
       if ( !have_DX0300 )
       {
            hr2 = lpdid1->lpVtbl->QueryInterface (lpdid1, &IID_IDirectInputDevice2, (void**)&lpdid2);
            if ( FAILED( hr2 ) )
                CONS_Printf ("\2Could not create IDirectInput device 2");
        }
        *lpDEV2 = lpdid2;     //only if we requested it
    }
    *lpDEV = lpdid1;
}


// ===========================================================================
//                                             DIRECT INPUT MOUSE
// ===========================================================================

//number of data elements in mouse buffer
#define DI_MOUSE_BUFFERSIZE  16

//
//  Initialise the mouse.
//
static void I_ShutdownMouse (void);
// Called on video mode change, usemouse change, mousemotion change,
// and game paused.
//   play_mode : enable mouse containment during play
void I_StartupMouse( boolean play_mode )
{
    // this gets called when cv_usemouse is initted
    // for the win32 version, we want to startup the mouse later
}

#ifdef MOUSE2
HANDLE mouse2_filehandle=0;

static void I_ShutdownMouse2 (void)
{
    if(mouse2_filehandle)
    {
        event_t event;
        int i;

        SetCommMask( mouse2_filehandle, 0 ) ;

        EscapeCommFunction( mouse2_filehandle, CLRDTR ) ;
        EscapeCommFunction( mouse2_filehandle, CLRRTS ) ;

        PurgeComm( mouse2_filehandle, PURGE_TXABORT | PURGE_RXABORT |
                                     PURGE_TXCLEAR | PURGE_RXCLEAR ) ;


        CloseHandle(mouse2_filehandle);

        // emulate the up of all mouse buttons
        for(i=0;i<MOUSEBUTTONS;i++)
        {
            event.type=ev_keyup;
            event.data1=KEY_MOUSE2+i;
            D_PostEvent(&event);
        }

        mouse2_filehandle=0;
    }
}

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

static void I_GetMouse2Event()
{
    // The system update of the serial ports is slow and there may be 15 to 30 calls
    // before the next data read is ready.
    // We may get an incomplete mouse packet, the partial packet is kept in m2pkt.
    static int   m2plen = 7;  // mouse2 packet length
    static byte  m2pkt[8];    // mouse2 packet
    static byte  mouse2_ev_buttons = 0;
    static byte  staleness = 0;

    event_t event;

    int  mouse2_rd_x, mouse2_rd_y;
    byte  mouse2_rd_buttons;
   
    // System read is expensive, so read to buffer, and process all of it.
    byte m2_buffer[MOUSE2_BUFFER_SIZE];
    int  m2_len;
    int  m2i;

  {   
    COMSTAT    ComStat ;
    DWORD      dwErrorFlags;
    DWORD      dwLength;

    ClearCommError( mouse2_filehandle, &dwErrorFlags, &ComStat ) ;
    dwLength = min( MOUSE2_BUFFER_SIZE, ComStat.cbInQue ) ;

    if (dwLength > 0)
    {
       if(!ReadFile( mouse2_filehandle, m2_buffer, dwLength, &dwLength, NULL ))
       {
           CONS_Printf("\2Read Error on secondary mouse port\n");
           return;
       }
    }
    m2_len = dwLength;
  }

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
                mouse2_rd_x += dx;
                mouse2_rd_y += dy;
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

          case 2:  // PS/2
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
#ifdef DEBUG_MOUSE2
                if( k > 5 )
                {
                    CONS_Printf("Mouse2 button in tight-loop\n");
                    break;
                }
#endif
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


// secondary mouse don't use directX, therefore forget all about grabing, acquire, etc...
void I_StartupMouse2 (void)
{
    DCB        dcb ;

    if(mouse2_filehandle)
        I_ShutdownMouse2();

    if(cv_usemouse2.value==0)
        return;

    if(!mouse2_filehandle)
    {
        // COM file handle
        mouse2_filehandle = CreateFile( cv_mouse2port.string, GENERIC_READ | GENERIC_WRITE,
                                       0,                     // exclusive access
                                       NULL,                  // no security attrs
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, 
                                       NULL );
        if( mouse2_filehandle == INVALID_HANDLE_VALUE )
        {
            int e=GetLastError();
            if( e==5 )
                CONS_Printf("\2Can't open %s : Access denied\n"
                            "The port is probably already used by one other device (mouse, modem,...)\n",cv_mouse2port.string);
            else
                CONS_Printf("\2Can't open %s : error %d\n",cv_mouse2port.string,e);
            mouse2_filehandle=0;
            return;
        }
    }

    // getevent when somthing happens
    //SetCommMask( mouse2_filehandle, EV_RXCHAR ) ;
    
    // buffers
    SetupComm( mouse2_filehandle, MOUSE2_BUFFER_SIZE, MOUSE2_BUFFER_SIZE ) ;
    
    // purge buffers
    PurgeComm( mouse2_filehandle, PURGE_TXABORT | PURGE_RXABORT |
                                 PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

    // setup port to 1200 7N1
    dcb.DCBlength = sizeof( DCB ) ;

    GetCommState( mouse2_filehandle, &dcb ) ;

    dcb.BaudRate = CBR_1200;
    dcb.ByteSize = 7;
    dcb.Parity = NOPARITY ;
    dcb.StopBits = ONESTOPBIT ;

    dcb.fDtrControl = DTR_CONTROL_ENABLE ;
    dcb.fRtsControl = RTS_CONTROL_ENABLE ;

    dcb.fBinary = TRUE ;
    dcb.fParity = TRUE ;

    SetCommState( mouse2_filehandle, &dcb ) ;

    I_AddExitFunc (I_ShutdownMouse2);
}
#endif

// judgecutor:
// 
#define         MAX_MOUSE_BTNS  3
static int      center_x, center_y;
static int      old_mparms[3], new_mparms[3] = {0, 0, 1};
static          boolean restore_mouse = FALSE;
static int      old_mouse_state = 0;
extern          boolean nodinput;
#ifdef MSH_WHEEL
// [WDJ] Does not compile and I don't know what it does.
char * MSH_MOUSEWHEEL = "MOUSEWHEEL";
unsigned int    MSHWheelMessage = 0;
#endif

static void I_DoStartupSysMouse()
{
    boolean valid;
    RECT    w_rect;
    
    valid = SystemParametersInfo(SPI_GETMOUSE, 0, old_mparms, 0);
    if (valid)
    {
        new_mparms[2] = old_mparms[2];
        restore_mouse = SystemParametersInfo(SPI_SETMOUSE, 0, new_mparms, 0);
    }

    if (vid.fullscreen)
    {
        w_rect.top = 0;
        w_rect.left = 0;
    }
    else
    {
        w_rect.top = windowPosY;
        w_rect.left = windowPosX;
    }
    
    w_rect.bottom = w_rect.top + vid.height;
    w_rect.right = w_rect.left + vid.width;
    center_x = w_rect.left + (vid.width >> 1);
    center_y = w_rect.top + (vid.height >> 1);
    SetCursor(NULL);    
    SetCursorPos(center_x, center_y);
    SetCapture(hWnd_main);
    ClipCursor(&w_rect);

}

static void I_ShutdownSysMouse()
{
    if (restore_mouse)
        SystemParametersInfo(SPI_SETMOUSE, 0, old_mparms, 0);
    //SetCursor(LoadCursor(NULL, IDC_WAIT));
    ClipCursor(NULL);
    ReleaseCapture();
}

void I_RestartSysMouse()
{
    if (nodinput)
    {
        I_ShutdownSysMouse();
        I_DoStartupSysMouse();
    }
}

void I_GetSysMouseEvents(int mouse_state)
{
    int     i;
    event_t event;
    int     xmickeys = 0, ymickeys = 0;
    POINT   c_pos;


    for (i = 0; i < MAX_MOUSE_BTNS; i++)
    {
        // check if button pressed
        if ((mouse_state & (1 << i)) && !(old_mouse_state & ( 1 << i)))
        {
            event.type = ev_keydown;
            event.data1 = KEY_MOUSE1 + i;
            D_PostEvent(&event);
        }
        // check if button released
        if (!(mouse_state & ( 1 << i)) && (old_mouse_state & (1 << i)))
        {
            event.type = ev_keyup;
            event.data1 = KEY_MOUSE1 + i;
            D_PostEvent(&event);
        }
    }
    old_mouse_state = mouse_state;

    // proceed mouse movements
    GetCursorPos(&c_pos);
    xmickeys = c_pos.x - center_x;
    ymickeys = c_pos.y - center_y;

    if (xmickeys || ymickeys)
    {
        event.type  = ev_mouse;
        event.data1 = 0;                      
        event.data2 = xmickeys;
        event.data3 = -ymickeys;
        D_PostEvent (&event);
        SetCursorPos(center_x, center_y);
    }


}

// This is called just before entering the main game loop,
// when we are going fullscreen and the loading screen has finished.
void I_DoStartupMouse (void)
{
    DIPROPDWORD   dip;
   
    // mouse detection may be skipped by setting usemouse false
    if (cv_usemouse.value == 0 || M_CheckParm("-nomouse")) {
        mouse_enabled = false;
        return;
    }

    if (nodinput)
    {
        CONS_Printf("\tMouse will not use DirectInput.\n");
        // System mouse input will be initiated by VID_SetMode
        //I_DoStartupSysMouse();
        I_AddExitFunc(I_ShutdownMouse);
       
#ifdef MSH_WHEEL
        //if (!GetSystemMetrics(SM_MOUSEWHEELPRESENT))
        MSHWheelMessage = RegisterWindowMessage(MSH_MOUSEWHEEL);
#endif

    } 
    else if (DID_mouse==NULL) // acquire the mouse only once
    {
        create_device_intf (&GUID_SysMouse, &DID_mouse, NULL);

        if (DID_mouse)
        {
            if (FAILED( DID_mouse->lpVtbl->SetDataFormat (DID_mouse, &c_dfDIMouse) ))
                I_Error ("Couldn't set mouse data format");
        
            // create buffer for buffered data
            dip.diph.dwSize       = sizeof(dip);
            dip.diph.dwHeaderSize = sizeof(dip.diph);
            dip.diph.dwObj        = 0;
            dip.diph.dwHow        = DIPH_DEVICE;
            dip.dwData            = DI_MOUSE_BUFFERSIZE;
            if (FAILED( DID_mouse->lpVtbl->SetProperty (DID_mouse, DIPROP_BUFFERSIZE, &dip.diph)))
                I_Error ("Couldn't set mouse buffer size");

            if (FAILED( DID_mouse->lpVtbl->SetCooperativeLevel (DID_mouse, hWnd_main, DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
                I_Error ("Couldn't set mouse coop level");

             //BP: acquire it latter
             //if (FAILED( DID_mouse->lpVtbl->Acquire (DID_mouse) ))
             //    I_Error ("Couldn't acquire mouse");
        }
        else
            I_Error ("Couldn't create mouse input");
    }

    if( DID_mouse ) 
        I_AddExitFunc (I_ShutdownMouse);

    // if re-enabled while running, just set mouse_enabled true again,
    // do not acquire the mouse more than once
    mouse_enabled = true;
}


//
// Shutdown Mouse DirectInput device
//
static void I_ShutdownMouse (void)
{
    int i;
    event_t event;

    CONS_Printf ("I_ShutdownMouse()\n");
        
    if (DID_mouse)
    {
        DID_mouse->lpVtbl->Unacquire (DID_mouse);
        DID_mouse->lpVtbl->Release (DID_mouse);
        DID_mouse = NULL;
    }

    // emulate the up of all mouse buttons
    for(i=0;i<MOUSEBUTTONS;i++)
    {
        event.type=ev_keyup;
        event.data1=KEY_MOUSE1+i;
        D_PostEvent(&event);
    }
    if (nodinput)
        I_ShutdownSysMouse();

    mouse_enabled = false;
}


//
// Get buffered data from the mouse
//
static void I_GetMouseEvents (void)
{
    DIDEVICEOBJECTDATA      rgdod[DI_MOUSE_BUFFERSIZE];
    DWORD                   dwItems;
    DWORD                   d;
    HRESULT                 hr;
    
    //DIMOUSESTATE      diMState;

    event_t         event;
    int                     xmickeys,ymickeys;

#ifdef MOUSE2
    if( mouse2_filehandle )
    {
        I_PollMouse2();

        // post key event for buttons
        if (handlermouse2buttons!=lastbuttons2)
        {
            int i,j=1,k;
            k=(handlermouse2buttons ^ lastbuttons2); // only changed bit to 1
            lastbuttons2=handlermouse2buttons;

            for(i=0;i<MOUSEBUTTONS;i++,j<<=1)
                if(k & j)
                {
                    if(handlermouse2buttons & j)
                        event.type=ev_keydown;
                    else
                        event.type=ev_keyup;
                    event.data1=KEY_MOUSE2+i;
                    D_PostEvent(&event);
                }
        }

        if ((handlermouse2x!=0)||(handlermouse2y!=0))
        {
            event.type=ev_mouse2;
            event.data1=0;
//          event.data1=buttons;    // not needed
            event.data2=handlermouse2x<<1;
            event.data3=-handlermouse2y<<1;
            handlermouse2x=0;
            handlermouse2y=0;

            D_PostEvent(&event);
        }
    }
#endif

    if (!mouse_enabled || nodinput)
        return;

getBufferedData:
    dwItems = DI_MOUSE_BUFFERSIZE;
    hr = DID_mouse->lpVtbl->GetDeviceData (DID_mouse, sizeof(DIDEVICEOBJECTDATA),
                                       rgdod,
                                       &dwItems,
                                       0 );

    // If data stream was interrupted, reacquire the device and try again.
    if (hr==DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED)
    {
        hr = DID_mouse->lpVtbl->Acquire (DID_mouse);
        if (SUCCEEDED(hr))
            goto getBufferedData;
    }

    // We got buffered input, act on it
    if (SUCCEEDED(hr))
    {
        xmickeys = 0;
        ymickeys = 0;

        // dwItems contains number of elements read (could be 0)
        for (d = 0; d < dwItems; d++)
        {
            if (rgdod[d].dwOfs >= DIMOFS_BUTTON0 &&
                rgdod[d].dwOfs <  DIMOFS_BUTTON0+MOUSEBUTTONS)
            {
                if (rgdod[d].dwData & 0x80)             // Button down
                    event.type = ev_keydown;
                else
                    event.type = ev_keyup;          // Button up

                event.data1 = rgdod[d].dwOfs - DIMOFS_BUTTON0 + KEY_MOUSE1;
                D_PostEvent(&event);
            }
            else if (rgdod[d].dwOfs == DIMOFS_X) {
                xmickeys += rgdod[d].dwData;
            }
            else if (rgdod[d].dwOfs == DIMOFS_Y) {
                ymickeys += rgdod[d].dwData;
            }
                         
            else if (rgdod[d].dwOfs == DIMOFS_Z) 
            {
                // z-axes the wheel
                if( (int)rgdod[d].dwData>0 ) 
                    event.data1 = KEY_MOUSEWHEELUP;
                else
                    event.data1 = KEY_MOUSEWHEELDOWN;
                event.type = ev_keydown;
                D_PostEvent(&event);
            }
            
        }

        if (xmickeys || ymickeys)
        {
            event.type  = ev_mouse;
            event.data1 = 0;                      
            event.data2 = xmickeys;
            event.data3 = -ymickeys;
            D_PostEvent (&event);
        }
    }
}


#ifdef JOYSTICK_SUPPORT
// ===========================================================================
//                                                DIRECT INPUT JOYSTICK
// ===========================================================================

// gamepad as buttons (otherwise as additive joystick)
#define GAMEPAD_AS_BUTTONS

// 0..1023
#define JOYAXIS_MAX  1023
// Deadzone 2500 = 25%
#define JOY_DEADZONE 2500

#define MAX_JOYSTICK 2
#define MAX_JOYAXES 3

typedef struct {
  LPDIRECTINPUTDEVICE     joydevp;  // joystick interface
  LPDIRECTINPUTDEVICE2    joypolldevp;  // joystick poll, NULL if no poll
  byte  gamepad;
  byte  numaxes;
  uint16_t  axis[MAX_JOYAXES];
  uint32_t  lastjoybuttons;
} joystick_t;

static joystick_t  joystk[ MAX_JOYSTICK ];

static byte joystick_detect = 0;
int num_joysticks = 0;
//byte joystick_started = 0;
#endif


// ------------------
// SetDIDwordProperty ( HELPER )
// Set a DWORD property on a DirectInputDevice.
// ------------------
static HRESULT SetDIDwordProperty( LPDIRECTINPUTDEVICE pdev,
                                   REFGUID guidProperty,
                                   DWORD dwObject,
                                   DWORD dwHow,
                                   DWORD dwValue)
{
    DIPROPDWORD dipdw;

    dipdw.diph.dwSize       = sizeof(dipdw);
    dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
    dipdw.diph.dwObj        = dwObject;
    dipdw.diph.dwHow        = dwHow;
    dipdw.dwData            = dwValue;

    return pdev->lpVtbl->SetProperty( pdev, guidProperty, &dipdw.diph );
}


#ifdef JOYSTICK_SUPPORT
// ---------------
// DIEnumJoysticks
// There is no such thing as a 'system' joystick, contrary to mouse,
// we must enumerate joysticks.
// The Game input may use one or more.
// ---------------
static BOOL CALLBACK DIEnumJoysticks ( LPCDIDEVICEINSTANCE lpddi,
                                       LPVOID pvRef )
{
    LPDIRECTINPUTDEVICE pdev;  // dev ptr
    DIPROPRANGE         diprg;
    DIDEVCAPS_DX3       caps;
    boolean             needpoll;
    const char *        reason = "";  // error msg reason
    joystick_t *        jsp;  // our joystick info

    if( num_joysticks >= MAX_JOYSTICK )
        return DIENUM_STOP;
   
    jsp = & joystk[num_joysticks];  // where to put all joystick info
    memset( jsp, 0, sizeof( joystick_t ) );

    // print out device name
    CONS_Printf ("%d: %s\n",
        num_joysticks,
        //( GET_DIDEVICE_SUBTYPE(lpddi->dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD ) ? "Gamepad " : "Joystick",
        lpddi->tszProductName ); // , lpddi->tszInstanceName );
    
    if (DI_main->lpVtbl->CreateDevice (DI_main, &lpddi->guidInstance,
                                    &pdev, NULL) != DI_OK)
    {
        // if it failed, then we can't use this joystick for some
        // bizarre reason.  (Maybe the user unplugged it while we
        // were in the middle of enumerating it.)  So continue enumerating
        reason = "CreateDevice FAILED";
        goto reason_cont;
    }


    // get the Device capabilities
    //
    caps.dwSize = sizeof(DIDEVCAPS_DX3);
    if ( FAILED( pdev->lpVtbl->GetCapabilities ( pdev, (DIDEVCAPS*)&caps ) ) )
    {
        reason = "GetCapabilities FAILED";
        goto fail_and_release;
    }
    if ( !(caps.dwFlags & DIDC_ATTACHED) )   // should be, since we enumerate only attached devices
        goto retcont;
    
    needpoll = (( caps.dwFlags & DIDC_POLLEDDATAFORMAT ) != 0);

    if ( caps.dwFlags & DIDC_FORCEFEEDBACK )
        CONS_Printf ("Sorry, force feedback is not yet supported\n");

    jsp->gamepad = ( GET_DIDEVICE_SUBTYPE( caps.dwDevType ) == DIDEVTYPEJOYSTICK_GAMEPAD );

    CONS_Printf ("Capabilities: %d axes, %d buttons, %d POVs, poll %d, Gamepad %d\n",
                 caps.dwAxes, caps.dwButtons, caps.dwPOVs, needpoll, jsp->gamepad);
    

    // Set the data format to "simple joystick" - a predefined data format 
    //
    // A data format specifies which controls on a device we
    // are interested in, and how they should be reported.
    //
    // This tells DirectInput that we will be passing a
    // DIJOYSTATE structure to IDirectInputDevice::GetDeviceState.
    if (pdev->lpVtbl->SetDataFormat (pdev, &c_dfDIJoystick) != DI_OK)
    {
        reason = "SetDataFormat FAILED";
        goto fail_and_release;
    }

    // Set the cooperativity level to let DirectInput know how
    // this device should interact with the system and with other
    // DirectInput applications.
    if (pdev->lpVtbl->SetCooperativeLevel (pdev, hWnd_main,
                        DISCL_EXCLUSIVE | DISCL_FOREGROUND) != DI_OK)
    {
        reason = "SetCooperativeLevel FAILED";
        goto fail_and_release;
    }

    // set the range of the joystick axis
    diprg.diph.dwSize       = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow        = DIPH_BYOFFSET;
    diprg.lMin              = -JOYAXIS_MAX;    // value for extreme left
    diprg.lMax              = +JOYAXIS_MAX;    // value for extreme right

    diprg.diph.dwObj = DIJOFS_X;    // set the x-axis range
    if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
        goto SetPropFail;

    diprg.diph.dwObj = DIJOFS_Y;    // set the y-axis range
    if (FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
        goto SetPropFail;

    // Only has code to handle two axis
    jsp->numaxes = 2;

    diprg.diph.dwObj = DIJOFS_Z;    // set the z-axis range
    if (! FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) ) {
        CONS_Printf ("DIJOFS_Z found, but can only use X,Y axis\n");
//        CONS_Printf ("DIJOFS_Z found\n");
//        jsp->numaxes++;
    }

    diprg.diph.dwObj = DIJOFS_RZ;   // set the rudder range
    if (! FAILED( pdev->lpVtbl->SetProperty( pdev, DIPROP_RANGE, &diprg.diph ) ) )
    {
        CONS_Printf ("DIJOFS_RZ (rudder) not found, but can only use X,Y axis\n");
//        CONS_Printf ("DIJOFS_RZ (rudder) found\n");
//        jsp->numaxes++;
    }

#ifdef GAMEPAD_AS_BUTTONS   
    if ( jsp->gamepad )
       jsp->numaxes = 0;
#endif
    if ( ! jsp->gamepad ) {
        // set X axis dead zone to 25% (to avoid accidental turning)
        if ( FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_X,
                                         DIPH_BYOFFSET, JOY_DEADZONE) ) )
        {
            CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for DEAD ZONE\n");
            //pdev->lpVtbl->Release (pdev);
            //goto retcont;
        }
        if (FAILED( SetDIDwordProperty (pdev, DIPROP_DEADZONE, DIJOFS_Y,
                                        DIPH_BYOFFSET, JOY_DEADZONE) ) )
        {
            CONS_Printf ("DIEnumJoysticks(): couldn't SetProperty for DEAD ZONE\n");
            //pdev->lpVtbl->Release (pdev);
            //goto retcont;
        }
    }

    // query for IDirectInputDevice2 - we need this to poll the joystick 
    if ( have_DX0300 || ! needpoll ) {
        // we won't use the poll
        jsp->joypolldevp = NULL;
    }
    else
    {
        // polling device
        if (FAILED( pdev->lpVtbl->QueryInterface(pdev, &IID_IDirectInputDevice2,
                                                 (LPVOID *)&(jsp->joypolldevp)) ) )
        {
            reason= "QueryInterface FAILED";
	    goto fail_and_release;
        }
    }
    
    // we successfully created an IDirectInputDevice.
    jsp->joydevp = pdev;

    num_joysticks++;
    if( num_joysticks < MAX_JOYSTICK )
        goto retcont;  // do not stop, get all
    return DIENUM_STOP;

SetPropFail:
    reason = "SetProperty FAILED";
fail_and_release:
    pdev->lpVtbl->Release (pdev);
reason_cont:
    CONS_Printf ("DIEnumJoysticks(): %s\n", reason);
retcont:
    return DIENUM_CONTINUE;
}
#endif



#ifdef JOYSTICK_SUPPORT
// --------------
// I_InitJoystick
// --------------

void I_InitJoystick (void)
{
    HRESULT hr;

    // cleanup
//    I_ShutdownJoystick ();
    
    if ( M_CheckParm("-nojoy") ) {
        CONS_Printf ("Joystick disabled\n");
        return;
    }

    // acquire the joystick only once
    if ( joystick_detect == 0 )
    {
        joystick_detect = 1;
        CONS_Printf ("Looking for joystick devices:\n");
        num_joysticks = 0;
        // invoke our Callback function DIEnumJoysticks
        hr = DI_main->lpVtbl->EnumDevices( DI_main, DIDEVTYPE_JOYSTICK, 
                                        DIEnumJoysticks,
                                        0, // no user param
                                        DIEDFL_ATTACHEDONLY );
        if (FAILED(hr)) {
            CONS_Printf ("\nI_InitJoystick(): EnumDevices FAILED\n");
            return;
        }

        if (num_joysticks == 0)
        {
	    CONS_Printf ("none found\n");
	    return;
	}

        I_AddExitFunc (I_ShutdownJoystick);

        // set coop level
//        if ( FAILED( joydevp->lpVtbl->SetCooperativeLevel (joydevp, hWnd_main, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) ))
//            I_Error ("I_InitJoystick: SetCooperativeLevel FAILED");

        // later
        //if ( FAILED( joydevp->lpVtbl->Acquire (joydevp) ))
        //    I_Error ("Couldn't acquire Joystick");

        joystick_detect = 2; // found joysticks
    }
}


static void I_ShutdownJoystick (void)
{
    int i, a;
    joystick_t * jsp;
    event_t event;
   
    // emulate the up of all joystick buttons
    for(i=0; i<(JOYBUTTONS*num_joysticks); i++)
    {
        event.type=ev_keyup;
        event.data1=KEY_JOY0BUT0+i;
        D_PostEvent(&event);
    }

    if ( joystick_detect == 2 )
        CONS_Printf ("I_ShutdownJoystick()\n");
        
    for( i=0; i<num_joysticks; i++ )
    {
        jsp = & joystk[i];

        // reset joystick position
	for( a=0; a<jsp->numaxes; a++ )
	    jsp->axis[a] = 0;

        if (jsp->joydevp)
        {
	    jsp->joydevp->lpVtbl->Unacquire (jsp->joydevp);
	    jsp->joydevp->lpVtbl->Release (jsp->joydevp);
	    jsp->joydevp = NULL;
	}
        if (jsp->joypolldevp)
        {
	    jsp->joypolldevp->lpVtbl->Release(jsp->joypolldevp);
	    jsp->joypolldevp = NULL;
	}
    }
    joystick_detect = 0;
}


// -------------------
// I_GetJoystickEvents
// Get current joystick axis and button states
// -------------------
static void I_Get_A_JoystickEvents ( int joynum )
{
    char *  reason;
    HRESULT     hr;
    DIJOYSTATE  js;  // DirectInput joystick state 
    joystick_t * jsp = &joystk[joynum];
    int         i;
    uint32_t    joybuttons;
    event_t event;

    if (jsp->joydevp==NULL)
        return;

    // if input is lost then try a few times to acquire
    // Do not get stuck here.
    for( i=3; i>0; i-- )
    {
        // poll the joystick to read the current state
        //faB: if the device doesn't require polling, this function returns
        //     almost instantly
        if ( jsp->joypolldevp ) {
            hr = jsp->joypolldevp->lpVtbl->Poll(jsp->joypolldevp);
            if ( hr == DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED ) 
                goto acquire;
            if ( FAILED(hr) )
            {
	        reason = "Poll FAILED";
	        goto fail_msg;
            }
        }

        // get the input's device state, and put the state in dims
        hr = jsp->joydevp->lpVtbl->GetDeviceState( jsp->joydevp, sizeof(DIJOYSTATE), &js );
        if( SUCCEEDED( hr ) )
	    goto read_joystick;

        if ( hr == DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED )
        {
            // DirectInput is telling us that the input stream has
            // been interrupted.  We aren't tracking any state
            // between polls, so we don't have any special reset
            // that needs to be done.  We just re-acquire and
            // try again.
            goto acquire;
        }
        reason = "GetDeviceState FAILED";
        goto fail_msg;

acquire:
        //CONS_Printf ("I_GetJoystickEvents(): Acquire\n");
        if ( FAILED(jsp->joydevp->lpVtbl->Acquire( jsp->joydevp )) ) 
             break;  // try later
    }
    return;  // try again later

read_joystick:
    // post virtual key events for buttons
    //
    joybuttons = 0;

    //faB: look for as much buttons as g_input code supports,
    //     we don't use the others
    for (i = JOYBUTTONS-1; i >= 0; i--)
    {
        joybuttons <<= 1;
        if ( js.rgbButtons[i] )
            joybuttons |= 1;
    }

    if(js.rgdwPOV[0]>=0 && js.rgdwPOV[0]!=0xffff && js.rgdwPOV[0]!=0xffFFffFF)
    {
        if((js.rgdwPOV[0]>=000 && js.rgdwPOV[0]<=4500) || (js.rgdwPOV[0]>31500 && js.rgdwPOV[0]<=36000))
            joybuttons|=1<<10;
        else
        if(js.rgdwPOV[0]>04500 && js.rgdwPOV[0]<=13500)
            joybuttons|=1<<13;
        else
        if(js.rgdwPOV[0]>13500 && js.rgdwPOV[0]<=22500)
            joybuttons|=1<<11;
        else
        if(js.rgdwPOV[0]>22500 && js.rgdwPOV[0]<=31500)
            joybuttons|=1<<12;

    }

    if ( joybuttons != jsp->lastjoybuttons )
    {
        uint32_t   j = 1;
        uint32_t   newbuttons;

        // keep only bits that changed since last time
        newbuttons = joybuttons ^ jsp->lastjoybuttons;    
        jsp->lastjoybuttons = joybuttons;

        for( i=0; i < JOYBUTTONS; i++, j<<=1 )
        {
            if ( newbuttons & j )      // button changed state ?
            {
                if ( joybuttons & j )
                    event.type = ev_keydown;
                else
                    event.type = ev_keyup;
                event.data1 = KEY_JOY0BUT0 + (joynum*JOYBUTTONS) + i;
                D_PostEvent (&event);
            }
        }
    }

    // save joystick axis positions (not an event)

    if ( jsp->gamepad )
    {
#ifdef GAMEPAD_AS_BUTTONS
        // gamepad controls to buttons
	// use the last 4 buttons
	int gpkey = 0;
        if ( js.lX < -(JOYAXIS_MAX/2) )
	    gpkey = JOYBUTTONS-1;
        else if ( js.lX > (JOYAXIS_MAX/2) )
	    gpkey = JOYBUTTONS-2;
        else
	    gpkey = 0;
        if ( gpkey != jsp->axis[0] ) {
	    event.type = ev_keyup;
	    event.data1 = KEY_JOY0BUT0 + (joynum*JOYBUTTONS) + jsp->axis[0];
	    D_PostEvent (&event);
	}
        if ( gpkey ) {
	    event.type = ev_keydown;
	    event.data1 = KEY_JOY0BUT0 + (joynum*JOYBUTTONS) + gpkey;
	    D_PostEvent (&event);
	}
	jsp->axis[0] = gpkey;

        if ( js.lY < -(JOYAXIS_MAX/2) )
	    gpkey = JOYBUTTONS-3;
        else if ( js.lY > (JOYAXIS_MAX/2) )
	    gpkey = JOYBUTTONS-4;
        else
	    gpkey = 0;
        if ( gpkey != jsp->axis[1] ) {
	    event.type = ev_keyup;
	    event.data1 = KEY_JOY0BUT0 + (joynum*JOYBUTTONS) + jsp->axis[1];
	    D_PostEvent (&event);
	}
        if ( gpkey ) {
	    event.type = ev_keydown;
	    event.data1 = KEY_JOY0BUT0 + (joynum*JOYBUTTONS) + gpkey;
	    D_PostEvent (&event);
	}
	jsp->axis[1] = gpkey;
#else
        // gamepad control additive joystick
        if ( js.lX < -(JOYAXIS_MAX/2) )
        {
	    if( jsp->axis[0] > -JOYAXIS_MAX )
                jsp->axis[0]--;
	}
        else if ( js.lX > (JOYAXIS_MAX/2) )
        {
	    if( jsp->axis[0] < JOYAXIS_MAX )
                jsp->axis[0]++;
	}
        if ( js.lY < -(JOYAXIS_MAX/2) )
        {
	    if( jsp->axis[1] > -JOYAXIS_MAX )
                jsp->axis[1]--;
	}
        else if ( js.lY > (JOYAXIS_MAX/2) )
        {
	    if( jsp->axis[1] < JOYAXIS_MAX )
                jsp->axis[1]++;
	}
#endif       
    }
    else
    {
        // analog control style , just send the raw data
        jsp->axis[0] = js.lX;    // x axis
        jsp->axis[1] = js.lY;    // y axis
    }
    return;

fail_msg:
    CONS_Printf ("I_GetJoystickEvents(): %s \n", reason);
    return;
}

static void I_GetJoystickEvents (void)
{
    int jn;
    for( jn=0; jn<num_joysticks; jn++)
	I_Get_A_JoystickEvents ( jn );
}


int I_JoystickNumAxes(int joynum)
{
    return (joynum < num_joysticks)? joystk[joynum].numaxes : 0;
}

int I_JoystickGetAxis(int joynum, int axisnum)
{
    return ((joynum < num_joysticks) && (axisnum < joystk[joynum].numaxes)) ?
           joystk[joynum].axis[axisnum] : 0;
}
#endif



// ===========================================================================
//                                            DIRECT INPUT KEYBOARD
// ===========================================================================

uint16_t  ASCIINames[256] =
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

//  0      1      2      3      4      5      6      7
//  8      9      A      B      C      D      E      F

    0,     0,     0,     0,     0,     0,     0,     0,  //0x80
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,  KEY_ENTER, KEY_RCTRL, 0, 0,
    0,     0,     0,     0,     0,     0,     0,     0,  //0xa0
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0, KEY_KPADPERIOD, 0, KEY_KPADSLASH, 0, 0,
  KEY_RALT,  0,   0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     KEY_HOME,  //0xc0
  KEY_UPARROW, KEY_PGUP, 0, KEY_LEFTARROW,0, KEY_RIGHTARROW, 0, KEY_END,
  KEY_DOWNARROW, KEY_PGDN, KEY_INS, KEY_DELETE, 0, 0, 0, 0,
    0,     0,     0, KEY_LWIN, KEY_RWIN, KEY_MENU, 0, 0,
    0,     0,     0,     0,     0,     0,     0,     0,  //0xe0
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,
};



int pausepressed=0;
byte shiftdown = false;
byte altdown = false;

//  Return a key that has been pushed, or 0
//  (replace getchar() at game startup)
//
int I_GetKey (void)
{
    event_t   *ev;
        
    if (eventtail != eventhead)
    {
        ev = &events[eventtail];
        eventtail++;  // MinGW needs this to be separate stmt
        eventtail = (eventtail)&(MAXEVENTS-1);
        if (ev->type == ev_keydown)
            return ev->data1;
        else
            return 0;
    }
    return 0;
}


// -----------------
// I_StartupKeyboard
// Installs DirectInput keyboard
// -----------------

//number of data elements in keyboard buffer
#define DI_KEYBOARD_BUFFERSIZE     32

void I_StartupKeyboard()
{
    DIPROPDWORD dip;

    //faB: make sure the app window has the focus or
    //     DirectInput acquire keyboard won't work
    if ( hWnd_main!=NULL )
    {
        SetFocus(hWnd_main);
        ShowWindow(hWnd_main, SW_SHOW);
        UpdateWindow(hWnd_main);
    }

    //faB: detect error
    if (DID_keyboard != NULL)
        return;

    create_device_intf (&GUID_SysKeyboard, &DID_keyboard, NULL);

    if ( ! DID_keyboard)
        I_Error ("Couldn't create keyboard input");

    if (FAILED( DID_keyboard->lpVtbl->SetCooperativeLevel (DID_keyboard, hWnd_main, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
        I_Error ("Couldn't set keyboard coop level");

    if (FAILED( DID_keyboard->lpVtbl->SetDataFormat (DID_keyboard, &c_dfDIKeyboard) ))
        I_Error ("Couldn't set keyboard data format");
            
    // create buffer for buffered data
    dip.diph.dwSize       = sizeof(dip);
    dip.diph.dwHeaderSize = sizeof(dip.diph);
    dip.diph.dwObj        = 0;
    dip.diph.dwHow        = DIPH_DEVICE;
    dip.dwData            = DI_KEYBOARD_BUFFERSIZE;
    if (FAILED( DID_keyboard->lpVtbl->SetProperty (DID_keyboard, DIPROP_BUFFERSIZE, &dip.diph)))
        I_Error ("Couldn't set keyboard buffer size");
    
        //faB: it seems to FAIL if the window doesn't have the focus
        //BP: acquire it latter
        //if (FAILED( DID_keyboard->lpVtbl->Acquire (DID_keyboard) ))
        //    I_Error ("Couldn't acquire keyboard\n");

    I_AddExitFunc (I_ShutdownKeyboard);
    hacktics = 0;                       //faB: see definition
    keyboard_started = true;
}


// ------------------
// I_ShutdownKeyboard
// Release DirectInput keyboard.
// ------------------
static void I_ShutdownKeyboard()
{
    if (!keyboard_started)
        return;
        
    CONS_Printf ("I_ShutdownKeyboard()\n");
        
    if ( DID_keyboard )
    {
        DID_keyboard->lpVtbl->Unacquire (DID_keyboard);
        DID_keyboard->lpVtbl->Release (DID_keyboard);
        DID_keyboard = NULL;
    }

    keyboard_started = false;
}


//faB: simply repeat the last pushed key every xx tics,
//     make more user friendly input for Console and game Menus
#define KEY_REPEAT_DELAY    (TICRATE/17)      // TICRATE tics, repeat every 1/3 second

// -------------------
// I_GetKeyboardEvents
// Get buffered data from the keyboard
// -------------------
static void I_GetKeyboardEvents (void)
{
    static  boolean         keyboard_lost = false;
    
    static  long            RepeatKeyTics = 0;
    static  BYTE            RepeatKeyCode;
    
    DIDEVICEOBJECTDATA      rgdod[DI_KEYBOARD_BUFFERSIZE];
    DWORD                   dwItems;
    DWORD                   d;
    HRESULT                 hr;
    int      ch, aqcnt = 0;
    
    boolean  keydown;
    event_t  event;
    
    if (!keyboard_started)
        return;
    
retry_getdata:
    if ( ++aqcnt > 5 )  // too many attempts
        return;
    dwItems = DI_KEYBOARD_BUFFERSIZE;
    hr = DID_keyboard->lpVtbl->GetDeviceData (DID_keyboard, sizeof(DIDEVICEOBJECTDATA),
                                       rgdod, &dwItems, 0 );
    
    // If data stream was interrupted, reacquire the device and try again.
    if (hr == DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED)
    {
        // why it succeeds to acquire just after I don't understand.. so I set the flag BEFORE
        keyboard_lost = true;
        
        hr = DID_keyboard->lpVtbl->Acquire (DID_keyboard);
        if (SUCCEEDED(hr))
            goto retry_getdata;
        return;
    }
    
    // we lost data, get device actual state to recover lost information
    if (hr==DI_BUFFEROVERFLOW)
    {
        //I_Error ("DI buffer overflow (keyboard)");
        //I_RecoverKeyboardState ();
        
        //hr = DID_mouse->lpVtbl->GetDeviceState (DID_mouse, sizeof(keys), &diMouseState);
    }
    
    // We got buffered input, act on it
    if (SUCCEEDED(hr))
    {
        // if we previously lost keyboard data, recover its current state
        if (keyboard_lost)
        {
            //faB: my current hack simply clear the keys so we don't have the last pressed keys
            // still active.. to have to re-trigger it is not much trouble for the user.
            memset (gamekeydown, 0, sizeof(gamekeydown));
            //CONS_Printf ("we lost it, we cleared stuff!\n");
            keyboard_lost = false;
        }
        
        // dwItems contains number of elements read (could be 0)
        for (d = 0; d < dwItems; d++)
        {
            // dwOfs member is DIK_* value
            // dwData member 0x80 bit set press down, clear is release
            
            if (rgdod[d].dwData & 0x80)
	    {
                event.type = ev_keydown;
	        keydown = 1;
	    }
            else
	    {
                event.type = ev_keyup;
	        keydown = 0;
	    }
            
            ch = rgdod[d].dwOfs & 0xFF;
	    event.data2 = ch;
	    event.data1 = ASCIINames[ch];
	    if (event.data1 == KEY_LSHIFT || event.data1 == KEY_RSHIFT )
	    {
	        shiftdown = keydown;
	    }
	    if (event.data1 == KEY_LALT || event.data1 == KEY_RALT )
	    {
	        altdown = keydown;
	    }

            D_PostEvent(&event);
        }

        //
        // Key Repeat
        //
        if (dwItems) {
            // new key events, so stop repeating key
            RepeatKeyCode = 0;
            RepeatKeyTics = hacktics + (KEY_REPEAT_DELAY*2);   //delay is trippled for first repeating key
            if (event.type == ev_keydown)       // use the last event!
                RepeatKeyCode = event.data1;
        }
        else
        {
            // no new keys, repeat last pushed key after some time
            if (RepeatKeyCode &&
                (hacktics - RepeatKeyTics) > KEY_REPEAT_DELAY)
            {
                event.type = ev_keydown;
                event.data1 = RepeatKeyCode;
                D_PostEvent (&event);

                RepeatKeyTics = hacktics;
            }
        }
    }
}


//
// Closes DirectInput
//
static void I_ShutdownDirectInput (void)
{
    if (DI_main!=NULL)
        DI_main->lpVtbl->Release (DI_main);
    DI_main = NULL;
}


//  This stuff should get rid of the exception and page faults when
//  Doom bugs out with an error. Now it should exit cleanly.
//
void I_StartupSystem(void)
{
    HRESULT hr;

    // some 'more globals than globals' things to initialize here ?
    keyboard_started = false;
    sound_started = false;
    timer_started = false;
    cdaudio_started = false;
    have_DX0300 = false;
    
    I_DetectWin95 ();

    // check for OS type and version here ?
#ifdef NDEBUG
    signal(SIGABRT, signal_handler);
    signal(SIGFPE , signal_handler);
    signal(SIGILL , signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT , signal_handler);
#endif

    // create DirectInput - so that I_StartupKeyboard/Mouse can be called later on
    // from D_DoomMain just like DOS version
    hr = DirectInputCreate (main_prog_instance, DIRECTINPUT_VERSION, &DI_main, NULL);

    if (  FAILED( hr ) )
    {
        // failed at DIRECTINPUT_VERSION
        // try opening DirectX3 interface for NT compatibility
        hr = DirectInputCreate (main_prog_instance, REQ_DX0300, &DI_main, NULL);

        if ( FAILED ( hr ) )
        {
            char* sErr;
            switch(hr)
            {
            case DIERR_BETADIRECTINPUTVERSION:
                sErr = "DIERR_BETADIRECTINPUTVERSION";
                break;
            case DIERR_INVALIDPARAM :
                sErr = "DIERR_INVALIDPARAM";
                break;
            case DIERR_OLDDIRECTINPUTVERSION :
                sErr = "DIERR_OLDDIRECTINPUTVERSION";
                break;
            case DIERR_OUTOFMEMORY :
                sErr = "DIERR_OUTOFMEMORY";
                break;
            default:
                sErr = "UNKNOWN";
                break;
            }
            I_Error ("Couldn't create DirectInput (reason: %s)", sErr);
        }
       
        // only use DirectInput3 compatible structures and calls
        have_DX0300 = true;
        CONS_Printf ("\2Using DirectX3 interface\n");
    }
    I_AddExitFunc (I_ShutdownDirectInput);
}


// Shutdown joystick and other interfaces, before I_ShutdownGraphics.
void I_Shutdown_IO(void)
{
#ifdef JOYSTICK_SUPPORT
//    I_ShutdownJoystick();
#endif
}

//  Closes down everything. This includes restoring the initial
//  palette and video mode, and removing whatever mouse, keyboard, and
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


// Init system
void I_SysInit(void)
{
    CONS_Printf("Win32 system ...\n");

    I_StartupSystem();
   
    I_StartupKeyboard();

#ifdef JOYSTICK_SUPPORT
    // Initialize the joystick subsystem.
    I_InitJoystick();
#endif

    // d_main will next call I_StartupGraphics
}
   

// Show the EndText, after the graphics are shutdown.
void I_Show_EndText( uint16_t * text )
{
#ifdef WIN98
    puttext(1,1,80,25, (byte*)text);
    gotoxy(1,24);
    fflush(stderr);
#endif
}


// ---------------
// I_SaveMemToFile
// Save as much as iLength bytes starting at pData, to
// a new file of given name. The file is overwritten if it is present.
// ---------------
void I_SaveMemToFile (byte* pData, unsigned long iLength, char* sFileName)
{
    HANDLE  fileHandle;
    DWORD   bytesWritten;
    fileHandle = CreateFile (sFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        I_SoftError ("SaveMemToFile");
        return;
    }
    WriteFile (fileHandle, pData, iLength, &bytesWritten, NULL);
    CloseHandle (fileHandle);
}


// my god how win32 suck
typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes); 

uint64_t I_GetDiskFreeSpace( void )
{
static MyFunc pfnGetDiskFreeSpaceEx=NULL;
static boolean testwin95 = false;

    uint64_t freespace;

    if(!testwin95)
    {
        HINSTANCE h = LoadLibraryA("kernel32.dll"); 

        if (h) {
             pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h,"GetDiskFreeSpaceExA"); 
             FreeLibrary(h);
        }
        testwin95 = true;
    } 
    if (pfnGetDiskFreeSpaceEx)
    {
        // ULARGE_INTEGER is a complex union
        ULARGE_INTEGER ul_freespace;
        ULARGE_INTEGER ul_usedbytes;
        if (pfnGetDiskFreeSpaceEx(NULL, &ul_freespace, &ul_usedbytes, NULL))
	    freespace = ul_freespace.QuadPart;
	else
            freespace = INT_MAX;
    }
    else
    {
        // DWORD is required by GetDiskFreeSpace
        DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters;
        DWORD TotalNumberOfClusters;
        GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, 
                               &NumberOfFreeClusters, &TotalNumberOfClusters);
        freespace = BytesPerSector * SectorsPerCluster * NumberOfFreeClusters;
    }
    return freespace;
}


char *I_GetUserName(void)
{
static char username[MAXPLAYERNAME];  // return to user
     char  *p;
     int   ret;
     // DWORD is required by GetUserName
     DWORD i=MAXPLAYERNAME;
     ret = GetUserName(username, &i);
     if(!ret)
     {
         if((p=getenv("USER"))==NULL)
             if((p=getenv("user"))==NULL)
                 if((p=getenv("USERNAME"))==NULL)
                     if((p=getenv("username"))==NULL)
                         return NULL;
         strncpy(username,p,MAXPLAYERNAME);
     }
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
    int  len;
    char * dnp;

//    if(  )
    {
        // MS-Docs say Windows XP, but MinGW does not guard it.
        len = GetModuleFileName( NULL, dirbuf, MAX_WADPATH-1 );
        if( len > 1 )
        {
	    dirbuf[len] = 0;  // does not terminate string ??
	    goto got_path;
	}
    }

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
    return mkdir(dirname);
    //TODO: should implement ntright under nt ...
}
