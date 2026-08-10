// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: playmus.c 1712 2025-01-17 04:18:00Z wesleyjohnson $
//
// Copyright (C) 1995-1996 Michael Heasley (mheasley@hmc.edu)
//   GNU General Public License
// Portions Copyright (C) 1996-2021 by DooM Legacy Team.
//   GNU General Public License
//   Heavily modified for use with Doom Legacy.
//   Removed wad search and Doom version dependencies.
//   Is now dependent upon IPC msgs from the Doom program
//   for all wad information, and the music lump id.

/*************************************************************************
 *  playmus.c
 *
 *  Copyright (C) 1995-1996 Michael Heasley (mheasley@hmc.edu)
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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#ifdef linux
#  include <signal.h>
#  include <errno.h>
#elif defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7) || defined(__FreeBSD__)
#  include <sys/signal.h>
#  include <errno.h>
#endif
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include <sys/soundcard.h>
  // MIDI field defines

#include "musserver.h"
#include "musseq.h"


// Throttle playback based on guessing playing time.  Does not work well.
// Due to ability to purge music queues, do not need to worry about
// music playing getting far ahead.
//#define THROTTLE_MIDI


#define TERMINATED 0xFFFFF

music_wad_t  music_lump = { NULL, 0 };
music_wad_t  genmidi_lump = { NULL, 0 };

byte  continuous_looping = 0;
byte  music_paused = 0;
byte  option_pending = 0;  // msg has set the option string

char * option_string = NULL;

// Cannot limit music to doom1 and doom2 names, must play any music lump.

//-- Signal functions

void sigfn_quitmus()
{
  if( music_lump.wad_name )
      free( music_lump.wad_name );
  music_lump.wad_name = NULL;
  music_lump.lumpnum = TERMINATED;
  music_lump.state = PLAY_QUITMUS;
}

void sigfn_do_nothing()
{
  signal(SIGHUP, sigfn_do_nothing);
}

//-- Service the IPC messages

// In milliseconds
#define MSG_CHECK_MS    50
#define TIMEOUT_MS    8000
#define THROTTLE_TIME 2000

#define ERROR_COUNT_LIMIT  50


// Change of device, restart playing the music.
void restart_playing( void )
{
    if( music_lump.state == PLAY_RUNNING )
        music_lump.state = PLAY_RESTART;
}

void get_mesg(byte wait_flag)
{
  // [WDJ] This is called often, so do not malloc the buffer.
  static mus_msg_t  recv;  // size MUS_MSG_MTEXT_LENGTH
  static uint32_t   parent_check_counter = 0;
  static uint32_t   pipe_error_count = 0;
  static char *  wadname = NULL; // malloc

  int result;
  char * fail_msg = "";
  unsigned int msgflags = MSG_NOERROR;
  int repeat = 1;

  if( ! wait_flag )
     msgflags |= IPC_NOWAIT;

  while (repeat)
  {
    repeat = 0;
    result = msgrcv(qid, &recv, MUS_MSG_MTEXT_LENGTH, 0, msgflags );
    if (result > 0)
    {
      if (verbose >= 2)
        printf("musserv ipc: bytes = %d, mtext = %s\n", result, recv.mtext);

      pipe_error_count = 0;
       
      switch (recv.mtext[0])
      {
       case 'v': case 'V':  // Volume
        if (changevol_allowed)
        {
          int vol = atoi(&recv.mtext[2]);
          master_volume_change( vol );
          if (verbose >= 2)
            printf("musserv: volume change = %d\n", vol);
        }
        repeat = 1;
        break;
       case 'o': case 'O':  // Option select
        if( option_string )
          free( option_string );
        option_string = strdup( &recv.mtext[1] );
        option_pending = 1;
        restart_playing();
        break;
       case 'd': case 'D':  // Obsolete
        if (verbose)
          printf("musserv: music name = %s\n", &recv.mtext[1]);
        // no longer search for music names, is only informational
        break;
       case 'w': case 'W':  // Wad name
        // Wad name
        if( wadname )
          free( wadname );
        wadname = strdup( &recv.mtext[1] );  // malloc
        repeat = 1;  // Another msg expected.
        break;
       case 'g': case 'G':  // Play GenMidi lump
        // GenMidi lump number.
        if( genmidi_lump.wad_name )
            free( genmidi_lump.wad_name );
        genmidi_lump.wad_name = wadname;
        genmidi_lump.lumpnum = atoi(&recv.mtext[2]);
        genmidi_lump.state = PLAY_START;
        if (verbose >= 2)
          printf("musserv: genmidi lump number = %d, in %s\n", genmidi_lump.lumpnum,
                 (wadname? wadname:"WAD NAME MISSING") );
        wadname = NULL;
        break;
       case 's': case 'S':  // Play music lump
        // Music lump number.
        if( music_lump.wad_name )
            free( music_lump.wad_name );
        music_lump.wad_name = wadname;
        music_lump.lumpnum = atoi(&recv.mtext[2]);
        music_lump.state = PLAY_START;
        continuous_looping = (toupper(recv.mtext[1]) == 'C');
        if (verbose >= 2)
        {
          printf("musserv: Start %s, music lump number = %d, in %s\n",
                 (continuous_looping?"Cont":""), music_lump.lumpnum,
                 (wadname?wadname:"WAD NAME MISSING") );
        }
        wadname = NULL;
        break;
       case 'p': case 'P':  // Pause
        // Pause and Stop
        music_paused = atoi(&recv.mtext[2]);
        if( music_paused )
        {
            if(music_lump.state == PLAY_RUNNING )
              music_lump.state = PLAY_PAUSE;
        }
        else
        {
            if(music_lump.state == PLAY_PAUSE )
              music_lump.state = PLAY_RUNNING;
        }
        break;
       case 'x': case 'X':  // Stop
        // Stop song
        music_lump.state = PLAY_STOP;
        break;
       case 'i': case 'I':  // Identify parent
        {
          // Watch PPID
          int ppid = atoi(&recv.mtext[2]);
          sprintf(parent_proc, "/proc/%d", ppid);  // length 17
          if( ppid > 1 )
            parent_check = 1;
        }
        break;
       case 'q': case 'Q':  // Quit
        // Proper quit is "QQ"
        if( recv.mtext[1] != 'Q' )  break;  // incomplete, ignore
        // Quit
        music_lump.state = PLAY_QUITMUS;
        if( verbose )
          printf("musserv: Received QUIT\n");
        // Do not remove the msg queue until this program actually exits.
        return;
      }
    }
    else if (result < 0)
    {
      switch (errno)
      {
       case EACCES:  // do not have access permission
        fail_msg="IPC message queue, permission failure";
        goto  fail_exit;
       case ENOMEM:  // do not enough memory
        fail_msg="IPC message queue, not enough memory";
        goto  fail_exit;
       case ENOSPC:  // system limit
        fail_msg="IPC message queue, system limit";
        goto  fail_exit;
       case EFAULT:
        fail_msg="IPC message queue, memory address is inaccessible";
        goto  fail_exit;
#if defined(linux) || defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
       case EIDRM:
        // Queue was removed while process was waiting for a message.
        // Can only get EIDRM if process is sleeping while waiting for message.
        fail_msg="IPC message queue, message queue has been removed";
        goto  fail_exit;
#endif
       case EINTR:
        if (verbose)
          printf("IPC message queue: received an interrupt signal\n");
        break;
       case EINVAL:
        // Queue does not exist, or other errors.
        if( (pipe_error_count ++) < 2000 )
        {
            if( (pipe_error_count & 0xFF) == 1 )   printf("IPC message queue: EINVAL count=%i\n", pipe_error_count );
            usleep( 1000 );
            break;
        }

#if defined(linux) || defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
        fail_msg="IPC message queue, EINVAL invalid message size or queue id";
#elif defined(__FreeBSD__)
        fail_msg="IPC message queue, message queue has been removed or invalid queue id";
#else
        fail_msg="IPC message queue, invalid message queue id";
#endif
        goto  fail_exit;
#ifdef __FreeBSD__
       case E2BIG:
        fail_msg="IPC message queue, E2BIG invalid message size or queue id";
        goto  fail_exit;
#endif
#if defined(linux) || defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
       case ENOMSG:
        // The normal case when there is no message.
        break;
#elif defined(__FreeBSD__)
       case EAGAIN:
        break;
#endif
       default:
//	 fail_msg="IPC message queue, general failure";
         fail_msg=strerror(errno);
         goto  fail_exit;
      }
    }
  }

  // Check on the parent, every few messages.
  if (parent_check && ((parent_check_counter++ & 0x07) == 0x06))
  {
    FILE *tmp;
    /* Check to see if doom is still alive... */
    if ((tmp = fopen(parent_proc, "r")) != NULL)
      fclose(tmp);
    else
    {
      cleanup_exit(1, "parent process appears to be dead");
    }
  }
  return;
  
fail_exit:
  cleanup_exit(2, fail_msg);
  return;
}



#ifdef THROTTLE_MIDI
// get_float_time is relative to this value, to preserve significant digits
time_t start_sec = 0;

void  start_float_time(void)
{
    struct timeval      tp;

    gettimeofday(&tp, NULL);  // seconds since EPOCH
    start_sec = tp.tv_sec;
}

// returns time in seconds
double  get_float_time(void)
{
    struct timeval      tp;

    gettimeofday(&tp, NULL);  // seconds since EPOCH

    return (double)(tp.tv_sec - start_sec) + (((double)tp.tv_usec)/1000000.0);
}

#endif


const byte mus_system_event_to_midi[] =
{
  // MUS code 10
  120,  // 10 All sounds off
  123,  // 11 All notes off, Note-1
  126,  // 12 Mono
  127,  // 13 Poly
  121   // 14 Reset all controllers, Note-1
};
// Note-1: Equipment must respond in order to comply with General Midi Level 1.

void playmus(music_data_t * music_data, byte check_msg)
{
  byte * musp;
  byte event0, event1, event2;  // bytes of the event
  byte eventtype;
  byte channelnum;
  byte lastflag;
  byte notenum;
  unsigned int pitchwheel;
  byte controlnum;
  byte controlval;
  unsigned int ticks;
  byte tdata;
  unsigned int lastvol[16];
  double delaytime;
  double curtime;
#ifdef THROTTLE_MIDI
  double clktime, target_time;
#endif
  unsigned int timeout;

  signal(SIGHUP, sigfn_do_nothing);
  signal(SIGQUIT, sigfn_quitmus);
  signal(SIGINT, sigfn_quitmus);
  signal(SIGTERM, sigfn_quitmus);
  signal(SIGCONT, SIG_IGN);

  musp = music_data->data;

  reset_midi();
  midi_timer(MMT_START);
  curtime = 0.0;
  lastflag = 0;

  music_lump.state = PLAY_RUNNING;

#ifdef THROTTLE_MIDI
  start_float_time();
#endif

// byte mus_to_chan[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 9 };
     
  for(;;)
  {
    // MUS event, variable length.
    // MUS format is nearly same as MIDI, but more compressed.
    // 1 track, 16 channels.
    // Fixed speed, must play back at 140 ticks/sec.
    event0 = *musp++;
    channelnum = event0 & 0x0F;
#if 0
    if (channelnum == 15)
      channelnum = 9;  // drum
    else if (channelnum > 8)
      channelnum++;
#else
    if (channelnum > 8)
      channelnum++;
    if (channelnum == 16)
      channelnum = 9;  // drum
#endif
    eventtype = (event0 >> 4) & 0x07;
    lastflag = event0 & 0x80;
    // last in a series of commands that happen on the same tick

    // Read MUS format, convert to MIDI.
    switch (eventtype)
    {
    case 0:		/* note off */
      event1 = *musp++;
      notenum = event1 & 0x7F;
      note_off(notenum, channelnum, lastvol[channelnum]);
      break;

    case 1:		/* note on */
      event1 = *musp++;
      notenum = event1 & 0x7F;
      if (event1 & 0x80)  // MUS event has velocity
      {
        event2 = *musp++;
        lastvol[channelnum] = event2 & 0x7F;  // notevol
      }
      note_on(notenum, channelnum, lastvol[channelnum]);
      break;

    case 2:		/* pitch wheel */
      event1 = *musp++;   // byte
      // pitch, 8 bit
      // 0=down2, 64=down1, 128=normal, 192=up1, 255=up2
#ifdef PITCHBEND_128
      pitchwheel = event1;
#else
      // From 1.42
      pitchwheel = event1 / 2;  // normal=64
#endif
      pitch_bend(channelnum, pitchwheel);
      break;

    case 4:		/* midi controller change */
      event1 = *musp++;
      controlnum = event1 & 0x7F;
      event2 = *musp++;
      controlval = event2 & 0x7F;
      switch (controlnum)
      {
        case 0:		/* patch change */
          patch_change(controlval, channelnum);
          break;
        case 3:		/* volume */
          // 0=silent, 100=normal, 127=loud
          // to MIDI ctrl 7
          // CTL_MAIN_VOLUME = 0x07  soundcard.h
          volume_change( channelnum, controlval );
          break;
        case 4:		/* pan */
          // 0=left, 64=center, 127=right
          // to MIDI ctrl 10
          // CTL_PAN = 0x0a  soundcard.h
          control_change(CTL_PAN, channelnum, controlval);
          break;
        case 1:  // bank select
          // to MIDI ctrl 0 or 32
          break;
        case 2:  // Modulation pot
          // to MIDI ctrl 1
          break;
        case 5:  // Expression pot
          // to MIDI ctrl 11
          break;
        case 6:  // Reverb depth
          // to MIDI ctrl 91
          break;
        case 7:  // Chorus depth
          // to MIDI ctrl 93
          break;
        case 8:  // Sustain pedal
          // to MIDI ctrl 64
          break;
        case 9:  // Soft pedal
          // to MIDI ctrl 67
          break;
      }
      break;

    case 3:    /* system event */
      event1 = *musp++;
#if 1
      // 10 => 120  All sounds off
      // 11 => 123  All notes off
      // 12 => 126  Mono
      // 13 => 127  Poly
      // 14 => 121  Reset all controllers.

      if( event1 >= 10 && event1 <= 14 )
          control_change(mus_system_event_to_midi[ event1 - 10 ], channelnum, 0 );
#endif
      break;
       
    case 6:	/* end of music data */
        if ( continuous_looping )
        {
          all_off_midi();

          // restart music
          while( device_playing() )
          {
              if (check_msg)
              {
                  get_mesg(MSG_NOWAIT);
                  if (music_lump.state != PLAY_RUNNING)  goto handle_msg;
              }
              usleep(500000);
          }
          musp = music_data->data;
          midi_timer(MMT_START);
          curtime = 0.0;
          lastflag = 0;
        }
        else
        {
          // play once
          // Wait until message changes state.
          get_mesg(MSG_WAIT);
        }
      break;

    default:	/* unknown */
      break;
    }

    if (lastflag)	/* next data portion is time data */
    {
      // get delay time, multiple bytes, most sig first
      byte numbytes = 0;
      tdata = *musp++;
      // Variable length number encoding, 7 bits per byte, 0x80 set on all except last byte.
      ticks = tdata & 0x7F;
      while(tdata & 0x80)
      {
        tdata = *musp++;
        ticks = (ticks<<7) + (tdata & 0x7F);
        if( ++numbytes > 4 )
        {
            printf("musserv: Bad time value\n" );
            goto handle_msg;
        }
      }
      // Wait for delaytime,  while checking for messages.
      // Observed delay 0.7 .. 28.0
      // [WDJ] Spec states 140 ticks/sec for Doom.

      // delay is in 128ths of quarter note, which depends upon tempo.
      // approx.  100th second
      delaytime = (double)ticks / 1.4;
      curtime += delaytime;
      // midi_wait syncs the queue, which may sleep,
      // but it is independent of delaytime.
      midi_wait( (uint32_t) curtime);

      // Midi wait only sends a midi event, and returns immediately;
      // it does NOT stall the caller.
      // This means that the midi events sent will be queued up far ahead
      // of the note playing.
      // If we stop or pause, it will take awhile (the queue size)
      // for the music to actually stop.
      
#ifdef THROTTLE_MIDI
      // Do not let curtime get too far ahead of clktime.
      timeout = 0;
      clktime = get_float_time();  // seconds
      // curtime is approx. 100 ticks/sec
      target_time = (curtime/100.0) - (THROTTLE_TIME/1000.0);
      for( ; ; )
      {
        // constant time increments between message checks
        if( get_float_time() > target_time )  break;
        if (check_msg)
        {
          get_mesg(MSG_NOWAIT);
          if (music_lump.state != PLAY_RUNNING)  goto handle_msg;
        }
        usleep( MSG_CHECK_MS * 1000 );
        if( timeout++ > (TIMEOUT_MS/MSG_CHECK_MS) )  break;  // broken time wait, 8 sec
      }
#endif       
    }

    if (check_msg)
    {
      get_mesg(MSG_NOWAIT);
      if (music_lump.state != PLAY_RUNNING)  goto handle_msg;
    }

    // To limit amount of music already in the queue.
    timeout = 0;
    while( get_queue_avail() < 0 )
    {
      if( seq_error_count > ERROR_COUNT_LIMIT )
      {
        printf("musserv: too many device errors\n" );
        break;
      }
      if( timeout++ > (TIMEOUT_MS/MSG_CHECK_MS) )
      {
        printf("musserv: broken queue wait  queue_avail=%i  device errors=%i\n", get_queue_avail(), seq_error_count );
        break;  // broken queue wait, 4 sec
      }
      usleep( MSG_CHECK_MS * 1000 );
    }
    continue;

handle_msg:
    if( music_lump.state == PLAY_PAUSE )
    {
      if( verbose >= 2 )
        printf( "Music paused\n" );
      pause_midi();
      while( music_lump.state == PLAY_PAUSE )
        get_mesg( MSG_WAIT );
      if (music_lump.state != PLAY_RUNNING)
        break;
      // There is no unpause, unless want to restore all note states.
    }
#if 0     
    if( music_lump.state == PLAY_RESTART )
    {
      reset_midi();
    }
#endif     
    if( music_lump.state != PLAY_RUNNING )
        break;
    if( verbose >= 2 )
      printf( "Music run\n" );
  } // play loop

  if( verbose >= 2 )
    printf( "Music STOP\n" );

  reset_midi();
  return;
}
