// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: linux.c 1579 2021-05-19 03:42:59Z wesleyjohnson $
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
// $Log: linux.c,v $
// Revision 1.7  2004/05/13 11:09:38  andyp
// Removed extern int errno references for Linux
//
// Revision 1.6  2004/04/17 12:55:27  hurdler
// now compile with gcc 3.3.3 under Linux
//
// Revision 1.5  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.4  2001/05/29 22:18:41  bock
// Small BSD commit: build r_opengl.so, sndserver
//
// Revision 1.3  2000/04/30 19:56:02  metzgermeister
// remove warning
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      UNIX, soundserver for Linux i386.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <stdio.h>

#if 0
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef LINUX
#ifdef FREEBSD
#include <sys/soundcard.h>
#else
#include <linux/soundcard.h>
#endif
#endif

#if defined(SCOOS5) || defined(SCOUW2)
#include <sys/soundcard.h>
#endif
#endif

#include "doomtype.h"
#include "soundsrv.h"


// For info, debug, dev, verbose messages.
// Print to output set by EOUT_flags.
void GenPrintf (const byte emsg, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


// Internal-Sound Interface.
#include "../snd_driver.h"

// Internal-Sound Interface.
// Import interface functions from snd_driver.c.
// void I_UpdateSound()
// void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
// int I_SoundIsPlaying(int handle)
#include "../snd_driver.c"

#if 0
int     audio_fd;
byte    audio_8bit_flag;

void myioctl( int fd, int command, int* arg )
{   
    static byte  ioctl_err_count = 0;
    static byte  ioctl_err_off = 0;
    int rc;

    if( ioctl_err_off )
    {
        ioctl_err_off--;
        return;
    }

    rc = ioctl(fd, command, arg);
    if (rc < 0)
    {
        fprintf(stderr, "ioctl(dsp,%d,arg) failed\n", command);
        fprintf(stderr, "errno=%d\n", errno);
        // [WDJ] No unnecessary fatal exits, let the player recover.
        if( ioctl_err_count < 254 )
            ioctl_err_count++;
        if( ioctl_err_count > 10 )
            ioctl_err_off = 20;
       
//        exit(-1);
    }
}
#endif

void I_InitSound( int samplerate, int samplesize )
{
#if 0   
    int i;
                
    audio_fd = open("/dev/dsp", O_WRONLY);
    if (audio_fd<0)
    {
      fprintf(stderr, "Could not open /dev/dsp: %s\n", strerror(errno));
        return;
    }
   
    // reset is broken in many sound drivers
    //myioctl(audio_fd, SNDCTL_DSP_RESET, 0);
    ioctl(audio_fd, SNDCTL_DSP_RESET, NULL);  // ignore errors

    
    audio_8bit_flag = 1;  // default
    if (getenv("DOOM_SOUND_SAMPLEBITS") == NULL)
    {
        myioctl(audio_fd, SNDCTL_DSP_GETFMTS, &i);
        if (i&=AFMT_S16_LE)
        {
            audio_8bit_flag = 0;
            myioctl(audio_fd, SNDCTL_DSP_SETFMT, &i);
            i = 11 | (2<<16);                                           
            myioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &i);
            i=1;    
            myioctl(audio_fd, SNDCTL_DSP_STEREO, &i);
            fprintf(stderr, "sndserver: Using 16 bit sound card\n");
        }
    }

    if( audio_8bit_flag )  // default
    {
        i=AFMT_U8;
        myioctl(audio_fd, SNDCTL_DSP_SETFMT, &i);
        i = 10 | (2<<16);                                           
        myioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &i);
        fprintf(stderr, "sndserver: Using 8 bit sound card\n");
    }

    i=11025;
    myioctl(audio_fd, SNDCTL_DSP_SPEED, &i);
#endif
}

void I_SubmitOutputBuffer( void* samples, int samplecount )
{
    if (audio_fd >= 0)
    {
        if (!audio_8bit_flag)
            write(audio_fd, samples, samplecount*4);
        else
            write(audio_fd, samples, samplecount);
    }
}

void I_ShutdownSound(void)
{
    if (audio_fd >= 0)
        close(audio_fd);
}


void I_InitMusic(void)
{
}

void I_ShutdownMusic(void)
{
}
