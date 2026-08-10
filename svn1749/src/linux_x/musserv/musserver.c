// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: musserver.c 1616 2022-01-03 11:58:02Z wesleyjohnson $
//
// Copyright (C) 1995-1997 Michael Heasley (mheasley@hmc.edu)
//   GNU General Public License Version 2
// Portions Copyright (C) 1996-2021 by DooM Legacy Team.
//   GNU General Public License Version 2
//   Heavily modified for use with Doom Legacy.
//   Optimized for use with DoomLegacy.
//   Modified IPC message commands.

/*************************************************************************
 *  musserver.c
 *
 *  Copyright (C) 1995-1997 Michael Heasley (mheasley@hmc.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
  // isdigit
#include "musserver.h"
#include "musseq.h"
  // soundcard.h
  // stdint.h

#if defined(SCOUW2)
#include "usleep.h"
#endif


// Globals
byte verbose = 0;
byte changevol_allowed = 1;
byte parent_check = 1;  // check parent process
byte no_devices_exit = 0;
char parent_proc[32];  // parent process /proc/num

int qid;  // IPC message queue id

music_data_t  music_data;

// Locals
static FILE *infile;

static byte sel_snddev = 99;  // sound_dev_e, MUTE
static byte sel_dvt = 99;     // mus_dev_e, MUTE
static int dev_port_num = -1;
static int dev_type = -1;  // as per the ioctl listing

#define  TIMEOUT_UNIT_MS  200
// Timeout in seconds.
#define  DEFAULT_TIMEOUT_SEC  300
static unsigned int timeout = DEFAULT_TIMEOUT_SEC;


static char help_text[] =
{
"Usage: musserver [options]\n"
"  -l               List detected music devices and exit\n"

#ifdef SOUND_DEVICE_OPTION
"  -s <snd>         Sound Device selection.\n"
"<snd>:"
#ifdef DEV_OSS
"  O=OSS"
#endif
#ifdef DEV_ALSA
"  A=ALSA"
#endif
"\n"
#endif

#ifdef MUS_DEVICE_OPTION
"  -d <dev> <port>  MIDI Device selection.\n"
"<dev>:"
"  M=Any MIDI"
#ifdef DEV_TIMIDITY
"  T=TiMidity"
#endif
#ifdef DEV_FLUIDSYNTH
"  L=FluidSynth"
#endif
#ifdef DEV_EXT_MIDI
"  E=Ext MIDI"
#endif
"\n"
#if defined( DEV_FM_SYNTH ) || defined( DEV_AWE32_SYNTH)
"  S=Any Synth"
#ifdef DEV_FM_SYNTH
"  F=FM Synth"
#endif
#ifdef DEV_AWE32_SYNTH
"  A=Awe32 Synth"
#endif
"\n"
#endif
#else
"  -d <port>  MIDI Device selection.\n"
#endif

"<port>  Optional, defaults to first found.\n"

"  -u <number>      Use device of type <number> where <number> is the type\n"
"                   reported by 'musserver -l'.\n"
"  -V <vol>         Ignore volume change messages from Doom\n"
"  -c               Do not check whether the parent process is alive\n"
"  -t <number>      Timeout for getting IPC message queue (sec).\n"
"  -v -v2 -v3       Verbose. Default level 1.\n"
"  -x               Exit if no devices found.\n"
"  -h               Help: print this message and exit\n"
};

void show_help(void)
{
    printf("musserver version %s\n", MUS_VERSION);
    printf("%s", help_text );
}

//#ifdef MUS_DEVICE_OPTION
// indexed by mus_dev_e
const char * music_dev_name[] = {
   "DEFAULT",  // preset default device
   "SEARCH1",
   "SEARCH2",
// MIDI devices
   "MIDI",
   "TIMIDITY",
   "FLUIDSYNTH",
   "EXT_MIDI",
// SYNTH devices
   "SYNTH"
   "FM_SYNTH",
   "AWE32_SYNTH",
// unused devices
   "DEV6",
   "DEV7",
   "DEV8",
   "DEV9",
// request
   "QUERY"
};
//#endif

#ifdef SOUND_DEVICE_OPTION
// indexed by sound_dev_e, values of snd_opt_cons_t
char snd_ipc_opt_tab[] = {
  'd', // Default
  'a', // Search 1
  'b', // Search 2
  'c', // Search 3
  'O', // OSS
  'E', // ESD
  'A', // ALSA
#if 0
// Do not really need this part of table.
  'P', // PulseAudio
  'J', // JACK
  'g', // Dev6
  'h', // Dev7
  'j', // Dev8
  'k'  // Dev9
#endif
};
#endif

#ifdef MUS_DEVICE_OPTION
// indexed by mus_dev_e, values of musserv_opt_cons_t
char mus_ipc_opt_tab[] = {
  'd', // Default
  'a', // Search 1
  'b', // Search 2
  'M', // Midi
  'T', // TiMidity
  'L', // FluidSynth
  'E', // Ext Midi
  'S', // Synth
  'F', // FM Synth
  'A', // Awe32 Synth
  'g', // Dev6
  'h', // Dev7
  'j', // Dev8
  'k'  // Dev9
};
#endif


// Select preference device.
//  ch : test char
//  tab[] : table of chars
// Return the table index, which will be the SD_ or MDT_ value.
static
byte  find_char_code( char ch, char tab[], byte tabsize )
{
    byte sd;
    for( sd=0; sd < tabsize; sd++ )
        if( tab[sd] == ch )  return sd;
    
    return 0;
}

static
void  parse_option_string( const char * optstr )
{
    const char * p = optstr;

    if( optstr == NULL )  return;

    while( *p != 0 )
    {
        while(*p == ' ') p++;
        if( *(p++) != '-' )  continue;
       
        switch( *(p++) )
        {
         case 'v':
            verbose=1;
            if( *p )
              verbose= *p - '0';
            break;
         case 'V':
            // fixed volume
            while(*p == ' ') p++;
            master_volume_change(atoi(p));
            while( isdigit(*p) ) p++;
            changevol_allowed = 0;  // fixed volume
            break;
         case 's':
            while(*p == ' ') p++;
            if( isalpha( *p ) )
            {
                char dc = *(p++);
#ifdef SOUND_DEVICE_OPTION
                sel_snddev = find_char_code( dc, snd_ipc_opt_tab, sizeof(snd_ipc_opt_tab) );
#else
                sel_snddev = SOUND_DEV1;
#endif
                while(*p == ' ') p++;
            }
            break;
         case 'd':
            while(*p == ' ') p++;
            if( isalpha( *p ) )
            {
                char dc = *(p++);
#ifdef MUS_DEVICE_OPTION
                sel_dvt = find_char_code( dc, mus_ipc_opt_tab, sizeof(mus_ipc_opt_tab) );
#else
                sel_dvt = MUS_DEV1;
#endif
                while(*p == ' ') p++;
            }
            if( isdigit( *p ) )
            {
                dev_port_num = atoi(p);
                while( isdigit(*p) ) p++;
            }
            break;
         default:
            break;
        }
    }
}

// Return optional parameter value on the command line switch.
// Return -1 when none.
//  avstr : string after the switch
//  avp :  & *argv[]  /*INOUT*/
int  command_value( char * const avstr, /*INOUT*/ char ** avp[] )
{
    if( avstr && *avstr )
    {
      // Check for number at next char of switch.
      if( isdigit( *avstr ) )
         return  atoi( avstr );
    }
   
    if( avp && *avp )
    {
      // Check for number in token following the switch.
      char * next_arg = (*avp)[1];
      if( next_arg )
      {
        if( isdigit( *next_arg ) )
        {
          (*avp)++;  // argument used up
          return  atoi( next_arg );
        }
      }
    }
    return -1;  // invalid
}


// Parse the command line.
void  command_line( int ac, char * av[] )
{
    char * avstr;
    int val;

    if( ac < 1 )  return;

    // Gave up on getopt, as it could not handle our optional arguments.
    av++; // skip program name
    for( ; *av ; av++ )
    {
      avstr = *av;
      if( *avstr == '-' )
      {
        char swch = avstr[1];
        avstr+=2;  // skip - and char
        switch ( swch )
        {
         case 'd':
            if( *avstr == 0 )
            {
                if( *av[1] == '-' )  continue;
                // Select letter must be in next token.
                av++;
                avstr = *av;
            }
            if( isalpha( *avstr ) )
            {
                char dc = *(avstr++);
#ifdef MUS_DEVICE_OPTION
                sel_dvt = find_char_code( dc, mus_ipc_opt_tab, sizeof(mus_ipc_opt_tab) );
#else
                sel_dvt = MIDI_DEV1;
#endif
            }
            dev_port_num = command_value( avstr, &av );  // optional port num
            break;
         case 'u':
            dev_type = command_value( avstr, &av );
            break;
         case 'l':
           {
            int num = list_devs();
            if( num == 0 )  exit(1);
            exit(0);
           }
            break;
         case 't':
            val = command_value( avstr, &av ); // optional time
            if( val >= 0 )
              timeout = val;  // seconds
            break;
         case 'V':
            changevol_allowed = 0;  // fixed volume
            val = command_value( avstr, &av );  // optional volume
            if( val >= 0 )
            {
              master_volume_change(val);
            }
            break;
         case 'c':
            parent_check = 0;
            break;
         case 'v':
            verbose = 1;
            val = command_value( avstr, &av );  // optional verbose level
            if( val > 0 )
              verbose = val;
            break;
         case 'h':
            show_help();
            exit(0);
            break;
         case 'x':
            no_devices_exit = 1;
            break;
         case '?': case ':':
            show_help();
            exit(1);
            break;
        }
      }
    }
   
//    printf( "dev_sel= %s  dev_port= %i  dev_type= %i\n", music_dev_name[sel_dvt], dev_port_num, dev_type );

#ifndef DEV_AWE32_SYNTH
    if( sel_dvt == MDT_AWE32_SYNTH )
    {
        printf("musserv: No AWE32 support\n");
        sel_dvt = MDT_NULL;
    }
#endif
}



// Cleanup and Exit
void cleanup_exit(int status, char * exit_msg)
{

    seq_shutdown();

#if 1
    // Close the message queue.
    msgctl(qid, IPC_RMID, (struct msqid_ds *) NULL);
#else   
    struct msqid_ds *dummy;
    dummy = malloc(sizeof(struct msqid_ds));
    msgctl(qid, IPC_RMID, dummy);
    free(dummy);
#endif

    if( infile )
      fclose(infile);

    if( (status > 1) || (status < -1) || verbose )
    {
      if( exit_msg )
      {
        if( strlen(exit_msg) > 0 )
          printf( "musserv: %s.\n", exit_msg );
        printf( "musserv: exiting.\n" );
      }
    }
    fflush( stdout );
    fflush( stderr );
    usleep( 200 );
    exit(status);
}


int main(int argc, char **argv)
{
    char * fail_msg = "";
    unsigned int musicsize;
    pid_t ppid;
    int timeout_cnt;

    music_data.data = NULL;
    music_data.size = 0;

#if defined(SCOOS5)
    parent_check = 0;
#endif
    command_line( argc, argv );
  
verbose = 4;
    if( verbose )
        printf("musserv: version %s\n", MUS_VERSION);

    ppid = getpid();  // our pid
    if(verbose >= 2) 
        printf("musserv: pid %d\n", ppid);

    if( parent_check )
    {
        ppid = getppid();  // parent pid
        sprintf(parent_proc, "/proc/%d", (int)ppid);  // length 17
        if (verbose >= 2)
            printf("parent pid %d %s\n", ppid, parent_proc);
        if( ppid < 2 )
            parent_check = 0;  // started by init, such as from system() call.
        // Will get correct PPID sent by IPC.
    }

    // The message queue is created after the musserver is started.
    qid = -9;
    for( timeout_cnt = timeout * 1000 / TIMEOUT_UNIT_MS; ; timeout_cnt-- )
    {
        // Even if timeout is 0, test for IPC queue at least once.
        qid = msgget( MUSSERVER_MSG_KEY, 0);
        // Cannot have a printf before checking errno !
        if (qid >= 0 )  break;

        // Error codes are positive from errno.
        switch(errno)
        {
          case ENOENT:  // does not exist yet
            if ((verbose >= 2) && ((timeout_cnt & 0x0F) == 4))
            {
               printf("Waiting for message queue id...\n");
            }
            break;
          case EACCES:  // do not have access permission
            fail_msg="IPC message queue, permission failure";
            goto  fail_exit;
          case ENOMEM:  // do not enough memory
            fail_msg="IPC message queue, not enough memory";
            goto  fail_exit;
          case ENOSPC:  // system limit
            fail_msg="IPC message queue, system limit";
            goto  fail_exit;
          default:
            fail_msg="IPC message queue, general failure";
            goto  fail_exit;
        }
        if( timeout_cnt < 1 )
        {
            fail_msg="Could not connect to IPC";
            goto  fail_exit;
        }
#ifdef DEBUG_WAIT
        printf("musserv: wait for queue, %s\n", strerror(errno) );
#endif
        usleep(TIMEOUT_UNIT_MS * 1000);  // 0.2 sec
    }
    if (verbose >= 2)
        printf("musserv: message queue id= %d\n", qid);

    if (verbose >= 2)
        printf("musserv: Waiting for first message from Doom...\n");

    // The DoomLegacy wad search is very complicated, and game dependent.
    // PWAD may also be involved.
    // Get the wad file name from IPC.

    while(genmidi_lump.state == PLAY_OFF)
       get_mesg(MSG_WAIT);

    if( genmidi_lump.wad_name == NULL )
      goto normal_exit_terminate;
   
    if( option_pending )
    {
        option_pending = 0;
       
        // Parse the option string from doom
        parse_option_string( option_string );
    }

    if((dev_type > 0) || (dev_port_num > 0))
    {
        if( sel_snddev > 50 )  sel_snddev = SD_NULL;
        if( sel_dvt > 50 )  sel_dvt = MDT_NULL;
    }
    if( verbose >= 2 )
       printf( "musserv: select sel_dvt=%s, dev_type=%i, port=%i\n", music_dev_name[sel_dvt], dev_type, dev_port_num );
    // init, load, setup the selected device
    seq_init_setup( sel_snddev, sel_dvt, dev_type, dev_port_num );
   
    // Instrument setup is done.
    
    // Wait for first music
    while(music_lump.state == PLAY_OFF)
       get_mesg(MSG_WAIT);
   
    if( music_lump.wad_name == NULL )
      goto normal_exit_terminate;

    for(;;)
    {
        if( option_pending )
        {
            option_pending = 0;
            parse_option_string( option_string );
            seq_shutdown();
            // load, setup the selected device
            seq_init_setup(sel_snddev, sel_dvt, dev_type, dev_port_num);
        }
       
        if( music_lump.state != PLAY_START )
        {
            get_mesg(MSG_WAIT);
        }

        if( music_lump.state != PLAY_START )  continue;

        if (verbose >= 2)
            printf("musserv: Playing music resource number %d\n", music_lump.lumpnum + 1);
        musicsize = read_wad_music( & music_lump,
             /* OUT */  & music_data );
        if( musicsize )
        {
            // Looping is now set by IPC message.
            playmus( & music_data, 1 );
        }
        switch ( music_lump.state )
        {
          case PLAY_START:
            reset_midi();
            release_music_data( & music_data );
            break;
          case PLAY_STOP:
            release_music_data( & music_data );
            break;
          case PLAY_RESTART:
            music_lump.state = PLAY_START;
            break;
          case PLAY_QUITMUS:
            if (verbose)
                printf("musserv: Terminated\n");
            goto normal_exit_terminate;
          default:
            fail_msg = "unknown error in music playing";
            goto fail_exit;
        }
    }

normal_exit_terminate:
#ifdef DEBUG_EXIT   
    printf( "musserv: normal_exit_terminate\n" );
#endif
    cleanup_exit(0, NULL);
    return 0;

fail_exit:
    cleanup_exit(2, fail_msg);
    return 1;
}
