// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_system.c 1649 2023-10-25 02:11:02Z wesleyjohnson $
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
// Revision 1.1  2001/04/17 22:23:38  calumr
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
// DESCRIPTION:
//   Macos system interface
//
//-----------------------------------------------------------------------------

#include <Carbon/Carbon.h>

#include "doomincl.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"
#include "i_system.h"
#include "d_net.h"
#include "g_game.h"
#include "g_input.h"
#include "i_joy.h"


#ifdef JOYSTICK_SUPPORT
JoyType_t Joystick;
#endif

uint16_t scantokey [128] =
{
  '=',  '9',  '7',  '-',  '8',  '0',  ']',  'o',
  'y',	't',  '1',  '2',  '3',  '4',  '6',  '5',
  'c',	'v',    0,  'b',  'q',  'w',  'e',  'r',
  'a',	's',  'd',  'f',  'h',  'g',  'z',  'x',
  KEY_LSHIFT, KEY_CAPSLOCK, KEY_LALT, KEY_LCTRL, 0, 0, 0, 0,
  KEY_TAB, KEY_SPACE, KEY_CONSOLE, KEY_BACKSPACE, 0, KEY_ESCAPE, 0, KEY_RALT,
  'k',	';', '\\',  ',',  '/',  'n',  'm',  '.',
  'u',	'[',  'i',  'p',  KEY_ENTER,  'l',  'j',  '\'',
  KEY_KEYPAD6, KEY_KEYPAD7, 0, KEY_KEYPAD8, KEY_KEYPAD9, 0, 0, 0,
  0, KEY_EQUALS, KEY_KEYPAD0, KEY_KEYPAD1, KEY_KEYPAD2, KEY_KEYPAD3, KEY_KEYPAD4, KEY_KEYPAD5,
  0, 0, 0, KEY_KPADSLASH, KEY_KPADENTER, 0, KEY_MINUSPAD, 0,
  '`',  KEY_KPADPERIOD, 0,  '*',  0, KEY_PLUSPAD, 0, KEY_NUMLOCK,
  KEY_F2, KEY_PGDN, KEY_F1, KEY_LEFTARROW, KEY_RIGHTARROW, KEY_DOWNARROW, KEY_UPARROW, 0,
  0, 0, KEY_INS, KEY_HOME, KEY_PGUP, KEY_DELETE, KEY_F4, KEY_END,
  0, 0, 0, 0, 0, KEY_F10, 0, KEY_F12,
  KEY_F5, KEY_F6, KEY_F7, KEY_F3, KEY_F8, KEY_F9, 0, KEY_F11,
};

int I_GetKey(void)
{
    KeyMap keymap;
    byte newBit;
    int i,n;

    GetKeys ((unsigned long *) keymap);

    for (i=0;i<4;i++)
    {
        if (keymap[i])
        {
            for (n=0;n<32;n++)
            {
                newBit = ((keymap[i] >> n) & 1);
                if (newBit)
                    return scantokey[i*32+n];
            }
        }
    }

    return 0;
}

//
// I_OutputMsg
// Outputs message to log.txt file in Legacy folder.
//
#include <fcntl.h>
void I_OutputMsg (char *error, ...)
{
#ifdef DEBUG_TO_FILE
    int handle;
    static int wipe = 1;
    va_list     argptr;
    char        txt[1024];

    va_start (argptr,error);
    vsprintf (txt,error,argptr);
    va_end   (argptr);

    printf(txt);	// Woohoo! MacOSX command-line output!

    if (wipe)
    {
        handle = open ("log.txt", O_WRONLY | O_APPEND | O_BINARY | O_TRUNC | O_CREAT, 0666);
        wipe = 0;
    }
    else
        handle = open ("log.txt", O_RDWR | O_APPEND | O_BINARY, 0666);
   
    if (handle == -1)
        return;

    write(handle, txt, strlen(txt));
    close(handle);
#endif
}

//----------------------------------------------
//----------------------------------------------
// cwd (current working directory) does not apply
// to mac apps (unless run from terminal). Make
// cwd = app directory. 

void GetApplicationFSSpec(FSSpec *me)
{
        ProcessInfoRec info;
        ProcessSerialNumber psn = { 0,kCurrentProcess };
        OSErr err;

        GetCurrentProcess(&psn);
        memset((char*)&info,0,sizeof(info));	
        info.processInfoLength = sizeof(info);
        info.processAppSpec = me;
        err = GetProcessInformation(&psn,&info);
}
#include <unistd.h>
#include <fcntl.h>
char *I_GetWadDir(void)
{
        FSSpec app_spec;
        FSSpec app_dir_spec;
        FSRef app_dir;
        UInt32 len = 256;
        char *path;
        OSErr err;

        GetApplicationFSSpec(&app_spec);

        FSMakeFSSpec(app_spec.vRefNum, app_spec.parID, "", &app_dir_spec);
        FSMakeFSSpec(app_dir_spec.vRefNum, app_dir_spec.parID, "", &app_spec);
        FSMakeFSSpec(app_spec.vRefNum, app_spec.parID, "", &app_dir_spec);
        FSMakeFSSpec(app_dir_spec.vRefNum, app_dir_spec.parID, "", &app_spec);

        err = FSpMakeFSRef(&app_spec, &app_dir);

        path = malloc(256);
        err = FSRefMakePath(&app_dir, path, len);

        I_OutputMsg("You double-clicked me!\n");

        chdir(path);

        return path;
}

ULONG I_GetFreeMem(ULONG *total)
{
  // add 0x01 to indicate guess
        *total = MaxBlock() - 4*1024*1024;
        return 0;  // no freemem
}

#if 0
static int quiting=0; /* prevent recursive I_Quit() */
#endif

void I_Tactile(int on,int off,int total )
{
  // Used for force feedback. 
  on = off = total = 0;
}

ticcmd_t        emptycmd;
ticcmd_t* I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return MaxBlock() - 4*1024*1024;	//Fix this too. 
}

byte* I_ZoneBase (int*  size)
{
    return NULL;
}

//
// I_GetTime
// returns time in 1/TICRATE second tics
//

void I_StartupTimer (void)
{}

ULONG  I_GetTime (void)
{
    UnsignedWide ftime;
    static double baseTicks=0;
    static UInt32 hiTicks=0;
    double ticks;

    Microseconds(&ftime);

    ticks = ftime.lo/1000000.0f;

    if (!baseTicks)
    {
        baseTicks = ticks;
        hiTicks = ftime.hi;
    }

    if (hiTicks != ftime.hi)
    {
        baseTicks = 0;
    }

    hiTicks = ftime.hi;

    return (ticks - baseTicks) * TICRATE;
}

#if 0
//
// I_Init
//
void I_Init (void)
{
    MenuBarHandle menuBar;
    
        /*menuBar = GetNewMBar(rMenuBar);
    SetMenuBar(menuBar);	
        DisposeHandle(menuBar);*/
    
    I_StartupSound();  // sfx and music
#if 0   
    quiting = 0;
#endif
}
#endif

#if 0
// Replaced by D_Quit_Save, I_Quit_System
//
// I_Quit
//
void I_Quit (void)
{
    /* prevent recursive I_Quit() */
    if(quiting)
        return;
    quiting = 1;
    if (demorecording)
        G_CheckDemoStatus();
    W_Shutdown();
    D_Quit_NetGame();
    I_ShutdownSound();  // sfx and music
#ifdef CDMUS
    I_ShutdownCD();
#endif
    M_SaveConfig (NULL);
    I_ShutdownGraphics();
    I_ShutdownInput();
    I_OutputMsg("Nice knowing you...\n");
    ExitToShell();
}
#endif

// sleeps for the given amount of milliseconds
void I_Sleep(unsigned int ms)
{
    usleep( ms * 1000 );
}

#ifdef LOADING_DISK_ICON
void I_BeginRead(void)
{
      //can be used to show a "loading..." thing
      //but it would appear for only a fraction of a second.
}

void I_EndRead(void)
{}
#endif


//
// I_Error
//
extern boolean demorecording;

void I_Error (char *error, ...)
{
    va_list     argptr;
    char        txt[1024];

    va_start (argptr,error);
    vsprintf (txt,error,argptr);
    va_end   (argptr);
    
    {
        SInt16 res;
        c2pstr(txt);

        StandardAlert(kAlertStopAlert,"\pError:",(ConstStr255Param)txt,NULL,&res);
    }

#if 1
    D_Quit_Save( QUIT_panic );  // No save, safe shutdown
#else
    // Shutdown. Here might be other errors.
    if (demorecording)
        G_CheckDemoStatus();
    W_Shutdown();
    D_Quit_NetGame();
    I_ShutdownMusic();
    I_ShutdownSound();
    I_Sleep( 3000 );  // to see some messages
    I_ShutdownGraphics();
    I_ShutdownInput();
    
    I_OutputMsg("Error: %s. \n",txt);
    ExitToShell();
#endif
}

// The final part of I_Quit, system dependent.
void I_Quit_System (void)
{
    ExitToShell();
}

// Shutdown joystick and other interfaces, before I_ShutdownGraphics.
void I_Shutdown_IO(void)
{
#ifdef JOYSTICK_SUPPORT
#ifdef LJOYSTICK
    I_ShutdownJoystick();
#endif
#endif
}

void I_ShutdownSystem(void)
{
}


void I_GetDiskFreeSpace(long long *freespace) {
    // 10MB should be enough
    *freespace = 10*1024*1024;
}

char *I_GetUserName(void)
{
    return getenv("USER");
}


// Get the directory of this program.
//   defdir: the current directory
//   dirbuf: a buffer of length MAX_WADPATH, 
// Return true when success, dirbuf contains the directory.
boolean I_Get_Prog_Dir( char * defdir, /*OUT*/ char * dirbuf )
{
    char * dnp;

#ifdef __MACH__
    // FIXME
#  if 0
    uint32_t  bufsize = MAX_WADPATH-1;
    // [WDJ] Am missing a few details
    if( _NSGetExecutablePath( dirbuf, & bufsize ) == 0 )   goto got_path;
#  endif
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

void I_LocateWad(void)
{
}


// [WDJ] Copied from sdl/filesrch.c, but not tested.
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "d_netfil.h"

#define MAX_SRCHPATH (MAX_WADPATH * 2)

//
// sys_filesearch:
//
//  filename : the filename to be found
//  wantedmd5sum : NULL for no md5 check
//  completepath : when not NULL, return the full path and name
//      must be a buffer of MAX_WADPATH
//  maxsearchdepth : dir depth, when 0 only search given directory
// return FS_NOTFOUND
//        FS_MD5SUMBAD
//        FS_FOUND

filestatus_e  sys_filesearch( const char * filename, const char * startpath,
                              const byte * wantedmd5sum, int maxsearchdepth,
                              /*OUT*/ char * completepath )
{
    filestatus_e retval = FS_NOTFOUND;
    DIR ** dirhandle_stack;  // (malloc)
    DIR * dirhandle;
    int * index_stack;  // each directory in the searchpath  (malloc)
    int depth=0;
    struct dirent *dent;
    struct stat fstat;
    int cur_index, remspace;  // searchpath
    char searchpath[MAX_SRCHPATH];

    if( maxsearchdepth < 1 )
       maxsearchdepth = 0;
    dirhandle_stack = (DIR**) malloc( (maxsearchdepth+1) * sizeof( DIR*));
    if( dirhandle_stack == NULL )   goto error1_exit;
    index_stack = (int*) malloc( (maxsearchdepth+1) * sizeof(int));
    if( index_stack == NULL )   goto error2_exit;
    
    strncpy( searchpath, startpath, MAX_SRCHPATH-1 );
    searchpath[MAX_SRCHPATH-1] = '\0';
    cur_index = strlen( searchpath) + 1;

    dirhandle = opendir( searchpath);
    if( dirhandle == NULL )  goto error3_exit;

    // Initial stack
    index_stack[0] = cur_index;
    dirhandle_stack[0] = dirhandle;

    if(searchpath[cur_index-2] != '/')
    {
        searchpath[cur_index-1] = '/';
        searchpath[cur_index] = 0;
    }
    else
    {
        cur_index--;
    }

    for(;;)
    {
        searchpath[cur_index]=0;
        dent = readdir( dirhandle );  // next dir entry
        if( !dent)  // done with dir
        {
            closedir( dirhandle );
            // Pop stack to previous directory.
	    if( depth == 0 )  break;  // done
            cur_index = index_stack[--depth];
            dirhandle = dirhandle_stack[depth];
            continue;
        }
        if( dent->d_name[0]=='.' )
        {
            // ignore the "." and ".." entries, we don't want to scan uptree
            if( dent->d_name[1]=='\0' )  continue;
            if( dent->d_name[1]=='.' && dent->d_name[2]=='\0' )  continue;
        }

        // Form file name for stat.
        remspace = (MAX_SRCHPATH - 1) - cur_index;
        strncpy(&searchpath[cur_index], dent->d_name, remspace);

        if( stat(searchpath,&fstat) < 0) // do we want to follow symlinks? if not: change it to lstat
        {
            // was the file (re)moved? can't stat it
            continue;
        }

        if( S_ISDIR(fstat.st_mode) )
        {
            if( depth >= maxsearchdepth )  continue;  // depth limited

            dirhandle = opendir(searchpath);
            if( dirhandle == NULL )
            {
                // can't open it... maybe no read-permissions
                // go back to previous dir
                cur_index = index_stack[depth];
                dirhandle = dirhandle_stack[depth];
                continue;
            }

            // Push new directory to stack.
            cur_index = strlen(searchpath) + 1;
            index_stack[++depth] = cur_index;
            dirhandle_stack[depth] = dirhandle;

            searchpath[cur_index-1]='/';
            searchpath[cur_index]=0;
        }
        else if ( strcasecmp(filename, dent->d_name) == 0 )
        {
            // Found the filename.
            retval = checkfile_md5(searchpath, wantedmd5sum);
            if( retval != FS_FOUND )  continue;

            // FS_FOUND, Return the found file.
	    if(completepath)
            {
                strncpy(completepath, searchpath, MAX_WADPATH-1);
                completepath[MAX_WADPATH-1] = '\0';
            }
            // Close the open directories.
            for(; depth >= 0; closedir(dirhandle_stack[depth--]));
            break;
        }
    }

error3_exit:
    free(index_stack);
error2_exit:
    free(dirhandle_stack);
error1_exit:

    return retval;
}
