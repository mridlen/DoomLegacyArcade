// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: soundsrv.c 1579 2021-05-19 03:42:59Z wesleyjohnson $
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
// $Log: soundsrv.c,v $
// Revision 1.8  2001/05/29 22:18:41  bock
// Small BSD commit: build r_opengl.so, sndserver
//
// Revision 1.7  2000/09/01 19:34:37  bpereira
// no message
//
// Revision 1.6  2000/04/30 19:50:37  metzgermeister
// no message
//
// Revision 1.5  2000/04/28 19:26:10  metzgermeister
// musserver fixed, sndserver amplified accordingly
//
// Revision 1.4  2000/04/22 20:30:00  metzgermeister
// fix amplification by 4
//
// Revision 1.3  2000/03/28 16:18:42  linuxcub
// Added a command to the Linux sound-server which sets a master volume.
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      UNIX soundserver, run as a separate process,
//       started by DOOM program.
//      Originally conceived fopr SGI Irix,
//       mostly used with Linux voxware.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
  // stdio, stdlib, stdarg, string, math
  // sys/types, sys/stat
  // doomdef, doomtype

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
  // timeofday

#include "../lx_ctrl.h"
  // SOUND_DEVICE_OPTION
  // DEV_xx
  // SOUND_DEV1
  // NEED_DLOPEN
  // sound_dev_e

// [WDJ] Removed old duplicate sounds.c sounds.h.  Were not kept up.
#include "../../sounds.h"
  // NUMSFX
#include "../../s_sound.h"
  // SURROUND_SEP

#include "soundsrv.h"

#define SNDSERV
#include "../snd_driver.h"
  // LXD_xx



// ========= DEFINE

// #define DEBUG   1
// #define DEBUG_PIPE

// #define DEBUG_VERBOSE


// ======== Functions for snd_driver

// For info, debug, dev, verbose messages.
// Print to output set by EOUT_flags.
void GenPrintf (const byte emsg, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void I_SoftError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf (stderr, "Warn: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


// ======== GLOBAL for snd_driver

// Set by command
int   mix_sfxvolume;  // local copy  0..31
byte  cv_num_channels = 16;
byte  sound_init = 0;
  // 0 = not ready, shutdown
  // 1 = ready for selection, no device
  // 6 = sound device ready
  // 7 = sound device active

byte  verbose = 0;

// Information for loaded sfx.
server_sfx_t   srv_sfx[NUMSFX];


// ======== Server

// an internal time keeper
static int      mytime = 0;

static struct timeval           last={0,0};
//static struct timeval         now;

static struct timezone          m_timezone;



#if 0
// [WDJ] Unused
void outputushort(int num)
{

    static unsigned char        buff[5] = { 0, 0, 0, 0, '\n' };
    static char*                badbuff = "xxxx\n";

    // outputs a 16-bit # in hex or "xxxx" if -1.
    if (num < 0)
    {
        write(1, badbuff, 5);
    }
    else
    {
        buff[0] = num>>12;
        buff[0] += buff[0] > 9 ? 'a'-10 : '0';
        buff[1] = (num>>8) & 0xf;
        buff[1] += buff[1] > 9 ? 'a'-10 : '0';
        buff[2] = (num>>4) & 0xf;
        buff[2] += buff[2] > 9 ? 'a'-10 : '0';
        buff[3] = num & 0xf;
        buff[3] += buff[3] > 9 ? 'a'-10 : '0';
        write(1, buff, 5);
    }
}
#endif


// Safely reads the pipe to the buffer, ensuring all bytes are read.
// This needs to be used for all pipe reads of more than 1 byte.
//  buf : to the buffer
//  req : the number of bytes to read
static
void  read_pipe( void * buf, int req )
{
    // hey, read on a pipe will not always
    // fill the whole buffer 19990203 by Kin
    int tlen = 0;
    while( tlen < req )
    {
        register int rc = read(0, buf+tlen, req-tlen);
        if( rc < 0 )  return;
        tlen += rc;  // bytes read
    }
}

// Load sfx command ( sfxid, flags, length, data )
static
void SSX_load_sound_data( void )
{
    server_sfx_t * sfxp;
    server_load_sound_t  sls;
    int  data_len;

    read_pipe( &sls, sizeof(sls) );  // sfx id, flags, snd_length
    data_len = sls.snd_len + 8;  // including sound header
   
#ifdef DEBUG
    fprintf(stderr, "SS: load_sound %i, flags=%x, size=%i\n", sls.id, sls.flags, sls.snd_len );
#endif
   
    if( sls.id >= NUMSFX )
        goto gracefully_ignore;
     
    //fprintf(stderr,"%d in...\n",bln);
   
    sfxp = & srv_sfx[sls.id];
    if( sfxp->data )
        free( sfxp->data );

    sfxp->data = malloc(data_len);
    if( sfxp->data == NULL )
    {
        fprintf(stderr, "Soundserver: sfx %i, memory req %i\n", sls.id, data_len );
        goto gracefully_ignore;
    };
    sfxp->flags = sls.flags;
    sfxp->length = data_len;
    read_pipe( sfxp->data, data_len );  // the snd data
    return;

gracefully_ignore:
   {
       byte dum[16];
       while( data_len > 16 )
       {
           read_pipe( dum, 16 );  // the snd data
           data_len -= 16;
       }
       read_pipe( dum, data_len );  // the snd data
   }
}


// Play sound command ( sfxid, vol, pitch, sep, priority, handle )
static
void SSX_play_sound( void )
{
    server_play_sound_t  sps;

    read_pipe( (byte*)&sps, sizeof(sps) );

#ifdef DEBUG_PIPE
    fprintf(stderr, "SS: play_sound %i, vol=%i, pitch=%i, sep=%i\n",
            sps.sfxid, sps.vol, sps.pitch, sps.sep );
#endif
   
    if( sps.sfxid >= NUMSFX )  return;

    //if (verbose)
    //{
    //  commandbuf[9]=0;
    //  fprintf(stderr, "%s\n", commandbuf);
    //}

    // The handle is determined by the main program, as it needs it
    // to stop the sound.
    //   sound_age: vrs priority 0..255
    LXD_StartSound_handle( sps.sfxid, sps.vol, sps.sep, sps.pitch, sps.priority, sps.handle, mytime );
}

// Stop sound command ( handle ).
static
void SSX_stop_sound( void )
{
    uint16_t  handle16;

    read_pipe(&handle16, sizeof(uint16_t));    // Get stop sound handle
    LXD_StopSound( handle16 );
}

// Update sound parameters (handle, vol, sep, pitch)
static
void SSX_update_sound_param( void )
{
   server_update_sound_t  sus;
   read_pipe(&sus, sizeof(sus));
   LXD_UpdateSoundParams( sus.handle, sus.vol, sus.sep, sus.pitch);
}


// Socket interface
static fd_set  fdset;
static fd_set  scratchset;



int main ( int c, char** v )
{
    int  rc;
    int  nrc;

    unsigned char       commandbuf[10];
    struct timeval      zerowait = { 0, 0 };

    memset( srv_sfx, 0, sizeof(srv_sfx) );  // must set ptr fields NULL

#ifdef DEBUG_VERBOSE
    verbose=1;
#endif
   
    gettimeofday(&last, &m_timezone);

    // init any data
    LXD_InitSound();

#ifdef DEBUG_PIPE
    if (verbose)
        fprintf(stderr, "SNDSERV ready\n");
#endif
    
    // parse commands and play sounds until done
    FD_ZERO(&fdset);
    FD_SET(0, &fdset);

    for(;;)
    {
        // Playing sound has priority.  Perform update periodically.
        LXD_UpdateSound();

        // Previous version would handle commands to the exclusion of updating the mixer.
        // That could affect sound playback.

        mytime++;

        scratchset = fdset;
        rc = select(FD_SETSIZE, &scratchset, 0, 0, &zerowait);
        if (rc < 0)  // error such as shutdown
            goto shutdown;

        if (rc > 0)
        {
            //  fprintf(stderr, "select is true\n");
            // got a command
            nrc = read(0, commandbuf, 1);
            if (!nrc)
                goto shutdown;		   

#ifdef DEBUG_PIPE
            if( verbose )
                fprintf(stderr, "cmd: %c 0x%X\n", commandbuf[0], commandbuf[0]);
#endif

            switch( commandbuf[0] )
            {
                // Most often first.
             case 'p':
                // play a new sound effect
                SSX_play_sound();
                break;
             case 'r':
                // update sound parameters
                SSX_update_sound_param();
                break;
             case 's':
                // stop sound
                SSX_stop_sound();
                break;
             case 'v':
                // master volume, num channels
                read_pipe(commandbuf, 2);
                mix_sfxvolume = commandbuf[0];  // local copy  0..31
                cv_num_channels = commandbuf[1];  // from cv_numChannels
                LXD_SetSfxVolume( mix_sfxvolume );  // from master volume  0..31
                break;
#ifdef SOUND_DEVICE_OPTION
             case 'd':
                // Sound device option, select sound output device.
                read_pipe(commandbuf, 1);
                LXD_SetSoundOption( commandbuf[0] );
                break;
#endif
             case 'l':
                SSX_load_sound_data();
                break;
             case 'q':
                goto shutdown;
                 
#ifdef DEBUG_PIPE
             case 'D':  // dump sfx
               {
                int fd;
                read_pipe(commandbuf, 3);
                commandbuf[2] = 0;
                fd = open((char*)commandbuf, O_CREAT|O_WRONLY, 0644);
                commandbuf[0] -= commandbuf[0]>='a' ? 'a'-10 : '0';
                commandbuf[1] -= commandbuf[1]>='a' ? 'a'-10 : '0';
                int sndnum = (commandbuf[0]<<4) + commandbuf[1];
                write(fd, srv_sfx[sndnum].data, srv_sfx[sndnum].length);
                close(fd);
               }
                break;
#endif
             case 0:  // a NOP
                break;	       
             default:
                fprintf(stderr, "Did not recognize command %d\n",commandbuf[0]);
                break;
            } // switch
        }
       
        usleep( 1000 );  // 100 times a sec
    }

shutdown:
    // Finish sounds, then shutdown device.
    LXD_ShutdownSound();

    return 0;
}
