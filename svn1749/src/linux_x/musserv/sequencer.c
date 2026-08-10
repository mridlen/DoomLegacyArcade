// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: sequencer.c 1660 2023-12-09 23:55:48Z wesleyjohnson $
//
// Copyright (C) 1995-1997 Michael Heasley (mheasley@hmc.edu)
//   GNU General Public License Version 2
// Portions Copyright (C) 1996-2016 by DooM Legacy Team.
//   GNU General Public License Version 2
//   Heavily modified for use with Doom Legacy.
// Date: 2021-3
//   Interface functions modified for DoomLegacy use.
//   Added ALSA support.
//   Added FluidSynth device.

#include "../lx_ctrl.h"
  // MUS_DEVICE_OPTION
  // DEV_xx


// #define DEBUG_MUSIC_INIT

// #define DEBUG_MIDI_CONTROL

// #define DEBUG_MIDI_DATA

// Use the ALSA RAWMIDI interface instead of the library calls
//#define ALSA_RAWMIDI

// sdl/i_sound uses 89
// PrBoom defaults to 70, but passes 64
#define TICKS_PER_QUARTER  89

// Some synth, like TiMidity, do not implement all-off effectively.
// Take drastic measures to kill notes that drone on.
//#define ALL_OFF_FIX 1

#ifdef DEV_TIMIDITY
// [WDJ] To make TiMidity dump midi commands to console 1,
// add switch "--verbose=3" to where it is started.
// Look in /etc/rc.d/rc.local.
#endif


#if defined( DEV_AWE32_SYNTH ) || defined( DEV_FM_SYNTH )
#define  SYNTH_DEVICES
#endif


extern const char * music_dev_name[];

// Search orders for pref device option.
// Now that this is changable from the DoomLegacy menu,
// and no longer does search when a specific device is specified.
// Ext midi is only a port, even when nothing is there, so put it last.
// Indexed by mus_dev_e
static const char * search_order[] =
{
  "",    // MDT_NULL, never used
  "TLFAE",      // MDT_SEARCH1
  "AFLTghjkE",  // MDT_SEARCH2, to be customized
  "TLE",  // MDT_MIDI  (any MIDI)
  "T",    // MDT_TIMIDITY
  "L",    // MDT_FLUIDSYNTH
  "E",    // MDT_EXT_MIDI
  "AFL",  // MDT_SYNTH  (any SYNTH)
  "F",    // MDT_FM_SYNTH
  "A",    // MDT_AWE32_SYNTH
  "g",    // MDT_DEV6
  "h",    // MDT_DEV7
  "j",    // MDT_DEV8
  "k"     // MDT_DEV9
};


// A strncpy that gcc10 does not complain about.
// This truncates the src when the dest buffer is full.
// Will always write the terminating 0.
// Does not pad the dest, like strncpy does.
//  destsize: 1..  must have room for term 0
void dl_strncpy( char * dest, const char * src, int destsize )
{
    char * ep = dest + destsize - 1;
    while( (dest < ep) && (* src) )
    {
        * (dest++) = * (src++);
    }
    * dest = 0;
}


// [WDJ] This has been heavily modified.
// Have added ALSA capability and other options.
/*************************************************************************
 *  sequencer.c
 *
 *  Copyright (C) 1995-1997 Michael Heasley (mheasley@hmc.edu)
 *
 *  Portions Copyright (c) 1997 Takashi Iwai (iwai@dragon.mm.t.u-tokyo.ac.jp)
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
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// synth_info
#include "musserver.h"
#include "musseq.h"
  // stdint.h


#ifdef DEV_OSS
#ifdef linux
#  include <sys/soundcard.h>
#  ifdef DEV_AWE32_SYNTH
#    include <linux/awe_voice.h>
#  endif
#elif defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
#  include <sys/soundcard.h>
#  ifdef DEV_AWE32_SYNTH
#    include <sys/awe_voice.h>
#  endif
#elif defined(__FreeBSD__)
#  include <machine/soundcard.h>
#  ifdef DEV_AWE32_SYNTH
#    include <awe_voice.h>
#  endif
#endif
#endif

#ifdef DEV_ALSA
#include <alsa/asoundlib.h>
  // loads all alsa headers
#endif



// ============= Sequencer Interface

uint32_t  midi_tick = 0;
uint32_t  seq_error_count = 0;

//  channel :  0..15
void (*note_on)(int note, int channel, int volume);
//  channel :  0..15
void (*note_off)(int note, int channel, int volume);
void (*pitch_bend)(int channel, int value);
void (*control_change)(int controller, int channel, int value);
void (*patch_change)(int patch, int channel);
void (*midi_wait)( uint32_t tick_time );
int  (*get_queue_avail)(void);
byte (*device_playing)(void);



// ============= Device Port state

static byte use_dev = SD_NULL;    // sound_dev_e
static byte use_mdt = MDT_NULL;   // mus_dev_e

static byte found_mdt = MDT_NULL;  // less code to check if found any
static byte num_midi_ports = 0;
#ifdef SYNTH_DEVICES
static byte num_synth_ports = 0;
#endif

static unsigned int queue_size = 0;

static int chanvol[16];
static int volscale = 100;


// =============  Device Info

typedef struct dev_info {
    byte  mdt_type;   // MDT_  , 0 if not found
    byte  port_num;
#ifdef DEV_ALSA
    snd_seq_addr_t  port_addr;  // ALSA
#endif
#ifdef DEV_FM_SYNTH
    int   num_voices;  // 0 for midi
#endif
    char  name[30];
} dev_info_t;

static dev_info_t *  seq_info = NULL;  // selected dev_info


// Found devices, dev_info index.
static struct {
#ifdef DEV_TIMIDITY
  dev_info_t  timidity;
#endif
#ifdef DEV_FLUIDSYNTH
  dev_info_t  fluidsynth;
#endif
#ifdef DEV_EXTMIDI
  dev_info_t  ext_midi;
#endif
#ifdef DEV_FM_SYNTH   
  dev_info_t  fm_synth;
#endif
#ifdef DEV_AWE32_SYNTH
  dev_info_t  awe_synth;
#endif
} dev_info;


// ============= Common definitions

// Midi Channel Mode Messages
enum {
   CMM_RESET_ALL_CONTROLLERS = 0x79,
   CMM_LOCAL_CONTROL = 0x7A,
   CMM_ALL_NOTES_OFF = 0x7B,
   CMM_OMNI_OFF = 0x7C,
   CMM_OMNI_ON = 0x7D,
   CMM_MONO_ON = 0x7E,  // Poly off
   CMM_POLY_ON = 0x7F   // Mono off
};

#ifdef DEBUG_MIDI_CONTROL
typedef struct {
  byte  controller;
  const char * text;
} cs_entry_t;

static const cs_entry_t  midi_control_string_table[] = {
   {0x00, "BANK" },
   {0x07, "MAIN_VOLUME" },
   {0x08, "BALANCE" },
   {0x0a, "PAN" },
   {0x78, "ALL_SOUNDS_OFF" },
   {0x79, "RESET_CONTROLLERS" },
   {0x7b, "ALL_NOTES_OFF" },
};

#define CS_BUFF_LEN  128
static
char cs_buff[CS_BUFF_LEN];

static
const char * midi_control_string( int controller )
{
  const cs_entry_t * cep;
  const cs_entry_t * cs_end = & midi_control_string_table[ sizeof(midi_control_string_table)/sizeof(cs_entry_t) ];
  for( cep = & midi_control_string_table[0]; cep< cs_end; cep++ )
  {
    if( cep->controller == controller )
        return cep->text;
  }
  // Not found in table
  snprintf( cs_buff, CS_BUFF_LEN, "%x", controller );
  cs_buff[CS_BUFF_LEN-1] = 0;
  return cs_buff;
}
#endif


#ifdef DEFAULT_AWE32_SYNTH
#  define DEFAULT_DEV   MDT_AWE32_SYNTH
#elif defined(DEFAULT_FM_SYNTH)
#  define DEFAULT_DEV   MDT_FM_SYNTH
#else
#  define DEFAULT_DEV   MDT_MIDI
#endif

// Arbitrary dev type value.  The Midi dev types all seem to be 0.
#define DEV_TYPE_TIMIDITY    1001
#define DEV_TYPE_FLUIDSYNTH  1002

//#define SEQ_MIDIOUT(a,b) test(a,b)

#ifdef  SYNTH_DEVICES

// from soundcard.h
// struct synth_info  sinfo[MAX_SYNTH_INFO];
// known fields:
//   char name[]
//   int device : 0..n, index to devices
//   int synth_type
//      SYNTH_TYPE_FM, SYNTH_TYPE_SAMPLE, SYNTH_TYPE_MIDI
//   int synth_subtype
//      FM_TYPE_ADLIB
//      FM_TYPE_OPL3
//      MIDI_TYPE_MPU401
//      SAMPLE_TYPE_BASIC
//      SAMPLE_TYPE_GUS
//      SAMPLE_TYPE_WAVEFRONT
//   int nr_voices
//   int instr_bank_size
//   uint capabilities : bit flags
//      SYNTH_CAP_PERCMODE, SYNTH_CAP_OPL3, SYNTH_CAP_INPUT

#endif

// from soundcard.h
// struct midi_info
// known fields:
//   char name[]
//   int device : 0..n, index to devices
//   uint capabilities   (almost always 0, except for MPU401)
//      MIDI_CAP_MPU401
//   int dev_type        (always 0)



// =============  Functions

static void update_all_channel_volume( void );
static void all_channel_all_notes_off( void );
static void clear_MDT( void );
static dev_info_t * detect_MDT( const char * client_name, const char * info_string, int port_num, int * dev_type_wb );
static void setup_midi_MDT( byte sd_dev, dev_info_t * devinfo );
static int  determine_setup_MDT( byte sd_dev, byte pref_dev );


// ============= MIDI Buffer

#ifdef ALSA_RAWMIDI

#define MIDI_BUFF_SIZE  128
static  byte  midi_buf[ MIDI_BUFF_SIZE ];
int midi_buf_cnt = 0;

static inline void midiout( byte m )
{
    midi_buf[ midi_buf_cnt++ ] = m;
}

static inline byte * get_midiout( byte num )
{
    byte * bp = & midi_buf[ midi_buf_cnt ];
    memset( bp, 0, num );
    midi_buf_cnt += num;
    return  bp;
}

// Each device driver has a function
// to write the midi_buf to the device.

#endif


// ============= ALL OFF


#ifdef ALL_OFF_FIX
// Some midi synth do not implement all-off, and will leave
// notes running (drones) when the stream stops.
// This tracks what notes and channels were used, and stops them.

// The number of notes and channels actually used in a song is limited.
byte channel_used[16];
byte note_used[0x7F];

static
void clear_used(void)
{
    memset( channel_used, 0, sizeof(channel_used) );
    memset( note_used, 0, sizeof(note_used) );
}
#endif

void all_off_midi(void)
{
  if (use_mdt == MDT_MIDI)
  {
#ifdef ALL_OFF_FIX
    unsigned int note;
    unsigned int ch;
    for(ch = 0; ch < 16; ch++)  // channels
    {
      if( ! channel_used[ch] ) continue;
      for( note=0; note <= 0x7F; note++ )
      {
        if( note_used[note] )
        {
          note_off( note, ch, 0 );	   
        }
      }
    }
    clear_used();
#else
    unsigned int ch;
    for (ch = 0; ch < 16; ch++)  // channels
    {
        // All notes off
        // OSS:  CMM_ALL_NOTES_OFF       0x7b
        // ALSA: MIDI_CTL_ALL_NOTES_OFF  0x7b
        control_change( CMM_ALL_NOTES_OFF, ch, 0 );
    }
#endif
  }
}


// ============= Dummy

#ifdef MUS_DEVICE_OPTION
static
void note3_DUMMY(int note, int channel, int volume)
{
}

static
void pi2_DUMMY(int channel, int value)
{
}

static
void midi_wait_DUMMY( uint32_t tick_time )
{
    midi_tick = tick_time;
}

static
int get_queue_avail_DUMMY( void )
{
    return 128;
}

static
byte  device_playing_DUMMY( void )
{
    return 0;
}

static
void setup_DUMMY( void )
{
    note_on = note3_DUMMY;
    note_off = note3_DUMMY;
    control_change = note3_DUMMY; // 3 int
    pitch_bend = pi2_DUMMY; // 2 int
    patch_change = pi2_DUMMY; // 2 int
    midi_wait = midi_wait_DUMMY;
    get_queue_avail = get_queue_avail_DUMMY;
    device_playing = device_playing_DUMMY;
}
#endif

// ============= OSS SECTION

#ifdef DEV_OSS
// OSS support
// Needs the /dev/sequencer OSS device to exist.
static int seqfd = -1;  // /dev/sequencer
// Selected device
static int seq_dev = -1;  // sound port, and index into sinfo or minfo

// Some operations from soundcard.h cause messages about violating strict aliasing.
// This is due to an operation  *(short *)&_seqbuf[_seqbufptr+6] = (w14).
// There is no good way to stop this, as long as -wstrict-aliasing is set.

// OSS sequencer interface.
SEQ_USE_EXTBUF();
SEQ_DEFINEBUF(2048);

// This function must be provided by the program, for the sequencer interface.
// This function is as specified in /usr/include/linux/soundcard.h
// to be used by the SEQ_ macros.
// This will sleep() if the sequencer buffer is full.
void seqbuf_dump(void)
{
  // Write the output buffer to the sequencer port.
  if (_seqbufptr)
  {
    if (write(seqfd, _seqbuf, _seqbufptr) == -1)
    {
       perror("write /dev/sequencer");
       seq_error_count++;
    }
  }
  _seqbufptr = 0;
}

// Called by: playmus
// Return > 0 only when can output at least 4 events.
static
int get_queue_avail_OSS( void )
{
  int i_queue_free;
  // Update the queue status.
  ioctl(seqfd, SNDCTL_SEQ_GETOUTCOUNT, &i_queue_free );
  return 256 - (queue_size - i_queue_free);
}

static
byte  device_playing_OSS( void )
{
  int i_queue_free;
  // Update the queue status.
  ioctl(seqfd, SNDCTL_SEQ_GETOUTCOUNT, &i_queue_free );
  return i_queue_free < queue_size;
}


// ============= OSS SYNTH SECTION

#if 0
// Indexed by synth_type
static const char * synth_type_txt[] = {
   "FM", //  SYNTH_TYPE_FM
   "SAMPLE", // SYNTH_TYPE_SAMPLE
   "MIDI",  // SYNTH_TYPE_MIDI
};
#endif

#ifdef DEV_AWE32_SYNTH

void setup_awe( void )
{
  use_dev = SD_OSS;
  use_mdt = MDT_AWE32_SYNTH;
  seq_info = & dev_info.awe_synth;
  seq_dev = seq_info->port_num;

  if (verbose)
    printf("Use OSS AWE synth device %d (%s)\n", seq_dev+1, seq_info->name);
}

#endif


#ifdef DEV_FM_SYNTH   
static int mixfd = -1;  // /dev/mixer
static int synth_patches[16];
// Defined by wad structs in musseq.h.
opl_instr_t  fm_instruments[175];

static byte fm_note12 = 0;

// Track notes playing, and which voice was used.
typedef struct {
    int32_t    note;
    int32_t    channel;
} synth_voice_t;

static int  num_voices;
static synth_voice_t * voices;


static
void fm_load_sbi(void)
{
  // [WDJ] This is not saved, only need one of these.
  // Defined by OSS in soundcard.h.
  struct sbi_instrument fm_sbi;

  int x;

  // Copy wad structures to OSS FM SYNTH structure.
  // Write to FM SYNTH.
  for (x = 0; x < 175; x++)
  {
    fm_sbi.key = FM_PATCH;
    fm_sbi.device = seq_dev;
    fm_sbi.channel = x;
    fm_sbi.operators[0] = fm_instruments[x].patchdata[0];
    fm_sbi.operators[1] = fm_instruments[x].patchdata[7];
    fm_sbi.operators[2] = fm_instruments[x].patchdata[4] + fm_instruments[x].patchdata[5];
    fm_sbi.operators[3] = fm_instruments[x].patchdata[11] + fm_instruments[x].patchdata[12];
    fm_sbi.operators[4] = fm_instruments[x].patchdata[1];
    fm_sbi.operators[5] = fm_instruments[x].patchdata[8];
    fm_sbi.operators[6] = fm_instruments[x].patchdata[2];
    fm_sbi.operators[7] = fm_instruments[x].patchdata[9];
    fm_sbi.operators[8] = fm_instruments[x].patchdata[3];
    fm_sbi.operators[9] = fm_instruments[x].patchdata[10];
    fm_sbi.operators[10] = fm_instruments[x].patchdata[6];
    fm_sbi.operators[11] = fm_instruments[x].patchdata[16];
    fm_sbi.operators[12] = fm_instruments[x].patchdata[23];
    fm_sbi.operators[13] = fm_instruments[x].patchdata[20] + fm_instruments[x].patchdata[21];
    fm_sbi.operators[14] = fm_instruments[x].patchdata[27] + fm_instruments[x].patchdata[28];
    fm_sbi.operators[15] = fm_instruments[x].patchdata[17];
    fm_sbi.operators[16] = fm_instruments[x].patchdata[24];
    fm_sbi.operators[17] = fm_instruments[x].patchdata[18];
    fm_sbi.operators[18] = fm_instruments[x].patchdata[25];
    fm_sbi.operators[19] = fm_instruments[x].patchdata[19];
    fm_sbi.operators[20] = fm_instruments[x].patchdata[26];
    fm_sbi.operators[21] = fm_instruments[x].patchdata[22];
    SEQ_WRPATCH(&fm_sbi, sizeof(fm_sbi));
  }
}

void setup_fm( void )
{
  char * fail_msg = NULL;
  FILE * sndstat;
  int x;

  use_dev = SD_OSS;
  use_mdt = MDT_FM_SYNTH;
  seq_info = & dev_info.fm_synth;
  seq_dev = seq_info->port_num;
  fm_note12 = 0;

  // Linux no longer has /dev/sndstat
  sndstat = fopen("/dev/sndstat", "r");
  if( sndstat )
  {
      char sndver[100];
      char * snddate = NULL;

      fgets(sndver, 100, sndstat);
      fclose(sndstat);

      if( verbose > 1 )
          printf( "musserv: sndver=%s\n", sndver );

      // [WDJ] Cannot fix this code properly because do not have the specific
      // hardware they were detecting, and they did not leave comments.
      // It does not exist on Linux 2.4 or Linux 2.6.
      // Previous code was mostly extraneous.
      snddate = strchr( sndver, '-' );
      if( snddate && ( strncmp( snddate+1, "950728", 6 ) == 0) )
         fm_note12 = 1;
  }

  num_voices = seq_info->num_voices;
  voices = malloc( num_voices * sizeof(synth_voice_t));
  for (x = 0; x < num_voices; x++)
  {
    voices[x].note = -1;
    voices[x].channel = -1;
  }

  for (x = 0; x < 16; x++)
    synth_patches[x] = -1;

  mixfd = open("/dev/mixer", O_WRONLY, 0);
  if( mixfd < 0 )
  {
    printf( "musserv: /dev/mixer: %s\n", strerror(errno) );
    fail_msg = "Failed to open mixer";
    goto fail_exit;
  }

  if (verbose)
    printf("Use OSS FM synth device %d (%s)\n", seq_dev+1, seq_info->name);
  return;

fail_exit:
  cleanup_exit(2, fail_msg );
  return;
}
#endif



#ifdef SYNTH_DEVICES
//  mdt_type : mus_dev_e
// Return port_num, -1 when fail.
static
int find_synth_OSS( int mdt_type, int dev_type, int port_num, byte en_print )
{
  // [WDJ] This is the only place that really uses this info.
  struct synth_info  sinfo;
  dev_info_t * dip;
  int num_synth = 0;
  int ior;
  int s = 0;
  byte info_mdt_type;

  // Get the number of synth devices
  if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &num_synth) == -1)
    return -1;

  if( num_synth > MAX_SYNTH_INFO )
    num_synth = MAX_SYNTH_INFO;
  
  num_synth_ports = num_synth;

  if( mdt_type != MDT_QUERY && port_num >= 0 && port_num < num_synth)
  {
    // Identify synth device by port_num.     
    sinfo.device = port_num;
    ior = ioctl(seqfd, SNDCTL_SYNTH_INFO, &sinfo );
    if( ior == 0 )
       return port_num;
  }
   
  for (s = 0; s < num_synth; s++)
  {
      sinfo.device = s;
      ior = ioctl(seqfd, SNDCTL_SYNTH_INFO, &sinfo);
      if( ior < 0 )  continue;

      mdt_type = MDT_NULL;
#ifdef DEV_AWE32_SYNTH
      if (sinfo.synth_type == SYNTH_TYPE_SAMPLE)
      {
          if (sinfo.synth_subtype == SAMPLE_TYPE_AWE32)
          {
              info_mdt_type = MDT_AWE32_SYNTH;
              dip = & dev_info.awe_synth;
          }
      }
      else
#endif
      {
#ifdef DEV_FM_SYNTH
          info_mdt_type = MDT_FM_SYNTH;
          dip = & dev_info.fm_synth;
#endif
      }

      if( info_mdt_type )  // recognized
      {
          if( dip->mdt_type == 0 )   // only first found
          {
              found_mdt = info_mdt_type;
              dip->mdt_type = info_mdt_type;
              dip->port_num = s;
#ifdef DEV_FM_SYNTH
              dip->num_voices = sinfo.nr_voices;
#endif
              dl_strncpy( dip->name, sinfo.name, sizeof(dip->name) );
          }
     
          if ((sinfo.synth_type == dev_type) && (info_mdt_type == mdt_type))
              return s;  // found
      }

      if( en_print )
      {
          printf("  Port %2i: Synth device of type %d (%s)\n",
              s, sinfo.synth_type, sinfo.name);
      }
  }
  return -1;  // not found
}

#endif


// ============= OSS MIDI SECTION


// [WDJ] Notes on ioctrl:
// SNDCTL_SEQ_TESTMIDI: tests midi device for life.
//   Returns EFAULT when user not allowed.
//   Returns ENXIO when midi_dev is invalid.
//   Returns open err, when fails to be opened.
//   Returns 0 when passes. 
// SNDCTL_SEQ_SYNC: attempts to sync queue
//   Returns EINTR if fails to empty queue.
//   This only flushes the music to the device or TiMidity,
//   which has its own queue.
//   Often hangs the musserver.
// SNDCTL_SEQ_GETOUTCOUNT:
//   Put (MAX_QUEUE - qlen) to value.
// SNDCTL_SEQ_RESET:  reset the sequencer
//   Midi: sends all-notes-off to all channels of each midi that was written.
// SEQ_WAIT: Starts a sound timer, returns 1.  Does not sleep or wait.
// SEQ_MIDIOUT: uses command MIDIPUTC to put bytes to a buffer.
//   seqbuf_dump flushes the buffer to the sequencer device.
//   During seqbuf_dump, if there is not enough space in queue, the
//   write will sleep.
// SEQ_MIDIPUTC: Will start sound timer when buffer is full, and returns 2.
//   This is not the device queue.

//  mdt_type : mus_dev_e
// Return port_num, -1 when fail.
// Called by: seq_setup
int find_midi_OSS(int mdt_type, int dev_type, int port_num, byte en_print )
{
  // [WDJ] This is the only place that really uses this info.
  struct midi_info   minfo;

  int m, num_midi;
  int ior;

  // Get the number of midi devices.
  if (ioctl(seqfd, SNDCTL_SEQ_NRMIDIS, &num_midi) != 0)
    return 0;
   
  num_midi_ports = num_midi;
   
  if( mdt_type != MDT_QUERY && port_num >= 0 && port_num < num_midi )
  {
    // Get info on a specific port.
    minfo.device = port_num;
    ior = ioctl(seqfd, SNDCTL_MIDI_INFO, &minfo);
    if( ior == 0 )
       return port_num;
  }
  
  for (m = 0; m < num_midi; m++)
  {
    // Set device, then ioctl fills in name, capabilities, dev_type.
    minfo.device = m;
    ior = ioctl(seqfd, SNDCTL_MIDI_INFO, &minfo);
    if( ior < 0 )  continue;

    detect_MDT( minfo.name, minfo.name, m, &minfo.dev_type );

    if( en_print )
    {
        printf("  Port %2i: Midi device of type %d (%s)\n",
            m, minfo.dev_type, minfo.name);
    }
     
    if( dev_type >= 0 )
    {
      if(minfo.dev_type == dev_type)
         return m;
    }
  }
  return -1;
}


// ============= OSS COMMON SECTION

// ---- MIDI

// [WDJ]
// SEQ_MIDIOUT: Puts the 4 byte midi command in the buffer.
// SEQ_START_NOTE: sends MIDI_NOTEON using 8 byte command
// SEQ_STOP_NOTE: sends MIDI_NOTEOFF using 8 byte command

#warning "OSS SEQ_xxx macros cause ** dereferencing type-punned pointer ** messages."
// SEQ_PITCHBEND, SEQ_BENDER_RANGE  are obsolete and incorrectly implemented.
#ifdef SEQ_BENDER_RANGE
# undef SEQ_BENDER_RANGE
static inline void SEQ_BENDER_RANGE( int dev, unsigned int voice, int value )
{
    // 14 bit value
    SEQ_CONTROL( dev, voice, CTRL_PITCH_BENDER_RANGE, value );
}
#endif

// SEQ_MAIN_VOLUME is obsolete and incorrectly implemented.
#ifdef SEQ_MAIN_VOLUME
# undef SEQ_MAIN_VOLUME
static inline void SEQ_MAIN_VOLUME( int dev, unsigned int voice, int value )
{
    // 14 bit value
    SEQ_CONTROL( dev, voice, CTL_MAIN_VOLUME, value );
}
#endif


void pause_midi_OSS(void)
{
  // Pause as much as can be paused.
  // Stop the notes.

#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    AWE_SET_CHANNEL_MODE(seq_dev, 1);
    AWE_NOTEOFF_ALL(seq_dev);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    all_channel_all_notes_off();
  }
#ifdef DEV_FM_SYNTH   
  else
  {
    unsigned int channel;
    for (channel = 0; channel < num_voices; channel++)
    {
        SEQ_STOP_NOTE(seq_dev, channel, voices[channel].note, 64);
        voices[channel].note = -1;
        voices[channel].channel = -1;
    }
  }
#endif
  SEQ_DUMPBUF();
}

void reset_midi_OSS(void)
{
#ifdef DEBUG_MIDI_CONTROL   
  printf("musserv: reset_midi_OSS\n" );
#endif
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    unsigned int channel;

    if( seq_dev < 0 )
        return;

    AWE_SET_CHANNEL_MODE(seq_dev, 1);
    AWE_NOTEOFF_ALL(seq_dev);
    for (channel = 0; channel < 16; channel++)
    {
      // SEQ_BENDER_RANGE is obsolete and incorrectly implemented, replaced by inline func.
      SEQ_BENDER_RANGE(seq_dev, channel, 200);
      SEQ_BENDER(seq_dev, channel, 0);
    }
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    if( seqfd < 0 )
        return;

    // SNDCTL_SEQ_SYNC hangs the musserver
//    ioctl(seqfd, SNDCTL_SEQ_SYNC);
    // All notes off on used channels.
    // Being implemented at the driver, it has the most immediate effect.
    ioctl(seqfd, SNDCTL_SEQ_RESET);
    usleep( 500 );  // long enough for synth to react

#if 0
    // Optional additional all notes off.
    // It does not seem to affect anything much.
    pause_midi();
    // It takes a while for buffer to get to all notes off, and then
    // there is an off decay.
    // If other commands follow too closely, they retrigger the note.
    usleep(20000);
#endif

#ifdef ALL_OFF_FIX
    all_off_midi();  // for synth that drone on
    // Touching the controls too soon seems to retrigger drone
    usleep(20000);
#endif

    unsigned int channel;
    for (channel = 0; channel < 16; channel++)
    {
      /* reset pitch bender */
#ifdef PITCHBEND_128
      pitch_bend( channel, 128 );  // normal=128
#else
      pitch_bend( channel, 64 );   // normal=64
#endif
      /* reset volume to 100 */
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
      SEQ_MIDIOUT(seq_dev, CTL_MAIN_VOLUME);
      SEQ_MIDIOUT(seq_dev, volscale);
      chanvol[channel] = 100;
      /* reset pan */
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
      SEQ_MIDIOUT(seq_dev, CTL_PAN);
      SEQ_MIDIOUT(seq_dev, 64);

      SEQ_DUMPBUF();
    }
  }
#ifdef DEV_FM_SYNTH   
  else
  {
    unsigned int channel;

    if( seq_dev < 0 )
        return;

    for (channel = 0; channel < num_voices; channel++)
    {
        SEQ_STOP_NOTE(seq_dev, channel, voices[channel].note, 64);
        SEQ_BENDER_RANGE(seq_dev, channel, 200);
        voices[channel].note = -1;
        voices[channel].channel = -1;
    }
  }
#endif   
  SEQ_DUMPBUF();
  usleep( 300 );  // long enough for synth to react
}

//  channel :  0..15
static
void note_off_OSS(int note, int channel, int volume)
{
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    SEQ_STOP_NOTE(seq_dev, channel, note, volume);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    SEQ_MIDIOUT(seq_dev, MIDI_NOTEOFF | channel);
    SEQ_MIDIOUT(seq_dev, note);
    SEQ_MIDIOUT(seq_dev, volume);  // velocity
      // some controllers use NOTEOFF velocity, some don't
  }
#ifdef DEV_FM_SYNTH   
  else if( use_mdt == MDT_FM_SYNTH )
  {
    int x = 0;
    for (x = 0; x < num_voices; x++)
    {
      if ((voices[x].note == note) && (voices[x].channel == channel))
      {
        voices[x].note = -1;
        voices[x].channel = -1;
        SEQ_STOP_NOTE(seq_dev, x, note, volume);
        break;
      }
    }
  }
#endif
  SEQ_DUMPBUF();
}

//  channel :  0..15
static
void note_on_OSS(int note, int channel, int volume)
{
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    SEQ_START_NOTE(seq_dev, channel, note, volume);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    SEQ_MIDIOUT(seq_dev, MIDI_NOTEON | channel);
    SEQ_MIDIOUT(seq_dev, note);  // General Midi has assigned codes to notes, 0..127
    SEQ_MIDIOUT(seq_dev, volume);  // velocity  0..127
#ifdef ALL_OFF_FIX
    channel_used[channel] = 1;
    note_used[note] = 1;
#endif
  }
#ifdef DEV_FM_SYNTH
  else if( use_mdt == MDT_FM_SYNTH )
  {
    int x = 0;
    // Find an empty voice
    for (x = 0; x < num_voices; x++)
    {
      if ((voices[x].note == -1) && (voices[x].channel == -1))
        break;
    }
    if (x < num_voices)
    {
      voices[x].note = note;
      voices[x].channel = channel;
      if (channel == 9)         /* drum note */
      {
        if (use_mdt == MDT_FM_SYNTH)
        {
          SEQ_SET_PATCH(seq_dev, x, note + 93);
          note = fm_instruments[note + 93].note;
        }
        else
          SEQ_SET_PATCH(seq_dev, x, note + 128);
      }
      else
      {
        SEQ_SET_PATCH(seq_dev, x, synth_patches[channel]);
        if ( fm_note12 )  // [WDJ] have no idea what this fixes
          note = note + 12;
      }
      SEQ_START_NOTE(seq_dev, x, note, volume);
    }
  }
#endif	   
  SEQ_DUMPBUF();
}

#ifdef PITCHBEND_128
//  value : 128 = normal
#else
//  value : 64 = normal
#endif
static
void pitch_bend_OSS(int channel, int value)
{ 
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
#ifdef PITCHBEND_128
    // convert 128 to 0x2000, << 6
    value <<= 6;
#else
    // convert 64 to 0x2000, << 7
    value <<= 7;
#endif
    SEQ_BENDER(seq_dev, channel, value);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    // normal = 0x2000
#if 0
    // Recommended to use SEQ_BENDER (synth), 14 bit value.
    // TEST, may not be for midi use.
# ifdef PITCHBEND_128
    // convert 128 to 0x2000, << 6
    SEQ_BENDER( seq_dev, channel, (value << 6) );
# else
    // convert 64 to 0x2000, << 7
    SEQ_BENDER( seq_dev, channel, (value << 7) );
# endif
#else
    // Using MIDI cmd as in 1.42
    SEQ_MIDIOUT(seq_dev, MIDI_PITCH_BEND | channel);
#if 1
    // from 1.42
    // should be LSB7, MSB7.
    SEQ_MIDIOUT(seq_dev, value >> 7);  // upper 7 bits ?? accidently 0
    SEQ_MIDIOUT(seq_dev, value & 127); // lower 7 bits ?? accidently upper of (value<<8)
#else
#ifdef PITCHBEND_128
    // convert 128 to 0x2000, << 6
    value <<= 6;
#else
    // convert 64 to 0x2000, << 7
    value <<= 7;
#endif
    SEQ_MIDIOUT(seq_dev, (value & 0x7F) ); // lower 7 bits
    SEQ_MIDIOUT(seq_dev, ((value >> 7) & 0x7F)); // upper 7 bits
#endif
#endif     
  }
#ifdef DEV_FM_SYNTH   
  else if( use_mdt == MDT_FM_SYNTH )
  {
    int x;
#ifdef PITCHBEND_128
    // convert 128 to 0x2000, << 6
    value <<= 6;
#else
    // convert 64 to 0x2000, << 7
    value <<= 7;
#endif
    for (x = 0; x < num_voices; x++)
    {
      if (voices[x].channel == channel)
      {
        // SEQ_BENDER_RANGE is obsolete and incorrectly implemented, replaced by inline func.
        SEQ_BENDER_RANGE(seq_dev, x, 200);
        SEQ_BENDER(seq_dev, x, value);
      }
    }
  }
#endif
  SEQ_DUMPBUF();
}


static
void control_change_OSS(int controller, int channel, int value)
{
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    SEQ_CONTROL(seq_dev, channel, controller, value);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE | channel);
    SEQ_MIDIOUT(seq_dev, controller);
    SEQ_MIDIOUT(seq_dev, value);
  }
#ifdef DEV_FM_SYNTH   
  else
  {
    int x;
    for (x = 0; x < num_voices; x++)
    {
      // SEQ_MAIN_VOLUME is obsolete and incorrectly implemented, replaced by inline func.
      if ((voices[x].channel == channel) && (controller == CTL_MAIN_VOLUME))
          SEQ_MAIN_VOLUME(seq_dev, x, value);
    }
  }
#endif
  SEQ_DUMPBUF();
}


void patch_change_OSS(int patch, int channel)
{
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    SEQ_SET_PATCH(seq_dev, channel, patch);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
    SEQ_MIDIOUT(seq_dev, MIDI_PGM_CHANGE | channel);
    SEQ_MIDIOUT(seq_dev, patch);
  }
#ifdef DEV_FM_SYNTH   
  else if( use_mdt == MDT_FM_SYNTH )
  {
    int x;
    for (x = 0; x < num_voices; x++)
    {
      if (((voices[x].channel == -1) && (voices[x].note == -1)) || (voices[x].channel == channel))
      {
        synth_patches[channel] = patch;
        break;
      }
    }
  }
#endif
  SEQ_DUMPBUF();
}

// Wait until
//  tick_time : wakeup time in ticks
void midi_wait_OSS( uint32_t tick_time )
{
  midi_tick = tick_time;
  ioctl(seqfd, SNDCTL_SEQ_SYNC);  // let queue go empty
  SEQ_WAIT_TIME( tick_time );   // wait, absolute time
  SEQ_DUMPBUF();
}


// This is called to start, or restart, the song ticks at 0.
// action : mmt_e
void midi_timer_OSS(byte action)
{
  switch (action)
    {
    case MMT_START:
      // Reset midi timer to 0.
      SEQ_START_TIMER();
      break;
    case MMT_STOP:
      SEQ_STOP_TIMER();
      break;
    case MMT_CONT:
      SEQ_CONTINUE_TIMER();
      break;
    }
}

void master_volume_change_OSS(int volume)
{
#ifdef DEV_AWE32_SYNTH
  if (use_mdt == MDT_AWE32_SYNTH)
  {
    int x;
    for (x = 0; x < 16; x++)
      SEQ_CONTROL(seq_dev, x, CTL_MAIN_VOLUME, chanvol[x] * volscale / 100);
  }
  else
#endif
  if (use_mdt == MDT_MIDI)
  {
      update_all_channel_volume();
  }
#ifdef DEV_FM_SYNTH   
  else
  {
    volume = volscale;
    volume |= (volume << 8);
    //if (-1 == ioctl(mixfd, SOUND_MIXER_WRITE_SYNTH, &volume))
    //  perror("volume change");
    ioctl(mixfd, SOUND_MIXER_WRITE_SYNTH, &volume);
    ioctl(mixfd, SOUND_MIXER_WRITE_LINE2, &volume);
  }
#endif
  SEQ_DUMPBUF();
}


// return number of devices found
byte list_devs_OSS( void )
{
  int num;

  seqfd = open("/dev/sequencer", O_WRONLY, 0);
  if( seqfd < 0 )
  {
    perror("open /dev/sequencer");
    return 0;
  }

  printf("Devices found OSS:\n");
  find_midi_OSS(MDT_QUERY, -1, -1, 1 );  // enable print
  num = num_midi_ports;
#ifdef SYNTH_DEVICES
  find_synth_OSS(MDT_QUERY, -1, -1, 1 ); // enable print
  num += num_synth_ports;
#endif
   
  close(seqfd);
  seqfd = -2;
  return num;
}


static
void seq_setup_OSS(int pref_dev, int dev_type, int port_num)
{
  int fnd_dev = -1;

#ifdef DEBUG_MIDI_CONTROL  
  if( verbose )
    printf( "pref_dev= %i, dev_type= %i, port_num= %i\n", pref_dev, dev_type, port_num );
#endif

  seqfd = open("/dev/sequencer", O_WRONLY, 0);
  if( seqfd < 0 )
  {
    perror("open /dev/sequencer");
    goto no_devices;
  }

  // Get the queue size;
  ioctl(seqfd, SNDCTL_SEQ_GETOUTCOUNT, &queue_size );
  if( verbose )
    printf( " Sequencer queue size= %i\n", queue_size );

  clear_MDT();

  // Midi dev_type is usually always 0.
  if((pref_dev == MDT_TIMIDITY) && (dev_type < 0))
     dev_type = DEV_TYPE_TIMIDITY;
  if((pref_dev == MDT_FLUIDSYNTH) && (dev_type < 0))
     dev_type = DEV_TYPE_FLUIDSYNTH;
  
  if( (pref_dev != MDT_NULL)
      && ((dev_type >= 0) || (port_num >= 0)) )
  {
#ifdef SYNTH_DEVICES
    if( pref_dev >= MDT_SYNTH )
    {
      fnd_dev = find_synth_OSS( pref_dev, dev_type, port_num, 0 ); // no print
    }
    else
#endif
    {
      // find a specific
      fnd_dev = find_midi_OSS( pref_dev, dev_type, port_num, 0 );  // no print
    }
  }

  if( fnd_dev < 0 )
  {
    if (pref_dev == MDT_NULL)
        pref_dev = DEFAULT_DEV;

#ifdef DEFAULT_TYPE
    if (dev_type == -1)
        dev_type = DEFAULT_TYPE;
#endif
    // find any of the dev_type
    find_midi_OSS( MDT_MIDI, dev_type, -1, 0 );  // no print
#ifdef SYNTH_DEVICES
    find_synth_OSS( MDT_SYNTH, dev_type, -1, 0 ); // no print
#endif
  }
  
  if( verbose )
  {
#ifdef DEV_TIMIDITY
    if( dev_info.timidity.mdt_type ) 
      printf("Timidity port: %i\n", dev_info.timidity.port_num );
#endif
#ifdef DEV_FLUIDSYNTH
    if( dev_info.fluidsynth.mdt_type ) 
      printf("Fluidsynth port: %i\n", dev_info.fluidsynth.port_num );
#endif
#ifdef DEV_EXTMIDI
    if( dev_info.ext_midi.mdt_type ) 
      printf("Ext midi port: %i\n", dev_info.ext_midi.port_num );
#endif
#ifdef DEV_FM_SYNTH
    if( dev_info.fm_synth.mdt_type )
      printf("FM port: %i\n", dev_info.fm_synth.port_num );
#endif
#ifdef DEV_AWE32_SYNTH
    if( dev_info.awe_synth.mdt_type )
      printf("AWE32 port: %i\n", dev_info.awe_synth.port_num );
#endif
  }

  if( found_mdt == MDT_NULL )
    goto no_devices;

  if( determine_setup_MDT( SD_OSS, pref_dev ) < 0 )
    goto no_devices;

  use_dev = SD_OSS;
  // Assign interface functions
  note_on = note_on_OSS;
  note_off = note_off_OSS;
  pitch_bend = pitch_bend_OSS;
  control_change = control_change_OSS;
  patch_change = patch_change_OSS;
  midi_wait = midi_wait_OSS;
  get_queue_avail = get_queue_avail_OSS;
  device_playing = device_playing_OSS;

  if( verbose )
      printf( "OSS active\n" );
   
#ifdef DEBUG_MIDI_CONTROL
  printf("musserv: OSS setup, reset_midi\n" );
#endif
  reset_midi_OSS();
  return;

no_devices:
  seq_dev = -1;
  use_dev = SD_NULL;
  return;
}


static
void seq_shutdown_OSS(void)
{
    if( seqfd >= 0 )
    {
        reset_midi_OSS();
        close(seqfd);
        seqfd = -2;  // closed
    }
#ifdef DEV_FM_SYNTH   
    if(use_mdt == MDT_FM_SYNTH)
    {
        if( mixfd >= 0 )
        {
            close(mixfd);
            mixfd = -2;  // closed
        }
    }
#endif
#ifdef MUS_DEVICE_OPTION
    setup_DUMMY();
#endif
}

// Init, load, setup the selected device
static
void seq_init_setup_OSS(int sel_dvt, int dev_type, int port_num)
{
    seq_setup_OSS(sel_dvt, dev_type, port_num);
    if( use_dev == SD_NULL )  // no devices
        return;

#ifdef DEV_FM_SYNTH   
    if (use_mdt == MDT_FM_SYNTH)
    {
        read_wad_genmidi( & genmidi_lump );
        fm_load_sbi();
    }
#endif

    // According to cph, this makes the device really load the instruments
    // Thanks, Colin!!
    seq_shutdown_OSS();
  
    seq_setup_OSS(sel_dvt, dev_type, port_num);

#ifdef DEV_FM_SYNTH   
    if (use_mdt == MDT_FM_SYNTH)
    {
        read_wad_genmidi( & genmidi_lump );
        fmload();
    }
#endif
}


// OSS SECTION
#endif



// ============= ALSA SECTION

#ifdef DEV_ALSA

static int self_client_id = -1;
static int self_port_id = -1;
static int self_queue_id = -1;

// Can use SND_SEQ_QUEUE_DIRECT as the queue_id.
// Can use snd_seq_ev_set_direct on the ev, as it just sets queue = SND_SEQ_QUEUE_DIRECT.
// Must flush event after output or it will sit in the ALSA library queue, not getting to the kernel.

#ifdef ALSA_RAWMIDI
// RawMIDI interface.
static snd_rawmidi_t * rawmidi_ALSA = NULL;
#else
// snd_seq_xx Library call interface.
static snd_seq_t * seq_handle_ALSA = NULL;
#endif

static snd_seq_event_t ev;

static
void report_alsa_error( const char * msg, int err1 )
{
    switch( err1 )
    {
      case -EAGAIN:
        break;
      case -EBUSY:
//        printf( "ALSA event: -EBUSY\n" );
        seq_error_count ++;
        break;
      case -EPIPE:
//        printf( "ALSA event: -EPIPE\n" );
        seq_error_count ++;
        break;
      default:
//        printf( "ALSA event: err %i\n", err );
        seq_error_count ++;
        break;
    }
    printf( "musserv ALSA %s: %s\n", msg, snd_strerror(err1) );
}


#ifdef ALSA_RAWMIDI
void write_buffer_ALSA( void )
{
    // This is arranged so that the normal execution is straight through.
    int cnt = snd_rawmidi_write( rawmidi_ALSA, midi_buf, midi_buf_cnt );
    if( cnt < midi_buf_cnt )
    {
        // May be frequent.
        if( cnt < 0 )
            goto err_handler;

        // Midi full, need to throttle output.
        midi_buf_cnt -= cnt;
        memmove( &midi_buf[0], &midibuf[cnt], cnt );
        goto full_handler;
    }

    midi_buf_cnt = 0;
    return;


    // Isolate the exception cases, to not load them to cache.
err_handler:
    if( cnt == -EAGAIN )
        goto stall; // full

    if( cnt == -EBUSY )
    {
        // busy with some other app
        cnt = 50000; // 50 msec
    }
    else
    {
        printf( "ALSA MIDI: %s\n", snd_strerror(err) );
        cnt = 1000000; // 1 sec
    }
    // Delay for situation to clear.
    seq_error_count ++;
    goto stall;

full_handler:
    // MIDI full, give it time to execute events.
    // Cannot assume that they use much time.
    cnt = 15000; // 15 msec
stall:
    usleep( cnt );
    return;
}

#else
// not RAWMIDI, QUEUED

// Get the tick time of the ALSA queue.
static
uint32_t  ALSA_tick_time( void )
{
    snd_seq_queue_status_t * qsp;
    snd_seq_queue_status_alloca( & qsp );
    int err6 = snd_seq_get_queue_status( seq_handle_ALSA, self_queue_id, qsp );
    if( err6 < 0 )
    {
        report_alsa_error( "get_queue_status", err6 );
        return 0;  // Queue is likely stopped too.
    }
    snd_seq_tick_time_t qtick = snd_seq_queue_status_get_tick_time( qsp );
    return qtick;
}


// Drain and sync the ALSA output.  Wait until it is empty.
static
void  drain_sync_ALSA( int ms_timeout )
{
#ifdef DEBUG_MIDI_DATA   
    uint32_t loop_cnt = 0;
#endif
    int err3;

// ??? FIXME   
    // Calling snd_seq_sync directly can stall indefinitely.
    // We need an escape so can service messages.
    for(;;)
    {
        // Send queue to the kernel pool.
        int err3 = snd_seq_drain_output( seq_handle_ALSA );
        if( err3 >= 0 )
           break;  // Still has something in the queue.

        // Error
        if( err3 != -EAGAIN )
        {
            report_alsa_error( "ALSA sync", err3 );
            ms_timeout >>= 1;
        }
        usleep( 2000 );  // 2 ms
        ms_timeout -= 2;
        if( ms_timeout <= 0 )
        {
#ifdef DEBUG_MIDI_DATA	   
            printf( "musserv: drain_sync_ALSA timeout\n" );
#endif
            return;
        }
#ifdef DEBUG_MIDI_DATA   
        loop_cnt++;
#endif
    }

    // Wait until queue is empty.
    // Can get stuck in snd_seq_sync_output_queue
    err3 = snd_seq_sync_output_queue( seq_handle_ALSA );
    if( err3 < 0 )
        report_alsa_error( "ALSA sync", err3 );

#ifdef DEBUG_MIDI_DATA   
    printf( "musserv: drain_sync_ALSA  synced,  loops=%i\n", loop_cnt );
#endif
}


#if 0
static
void write_direct_ALSA( void )
{
#if 0   
    ev.flags = SND_SEQ_EVENT_LENGTH_FIXED | SND_SEQ_PRIORITY_NORMAL;
    ev.queue = SND_SEQ_EVENT_DIRECT;
    ev.time.tick = 0;  // immediate
#endif

    // Send event directly to sequencer, not using buffer queue.
    // Returns the byte size of events still on the buffer, or an err.
    int err = snd_seq_event_output_direct( seq_handle_ALSA, &ev );
       // Does not work
       // invalid arguement
    if( err < 0 )
    {
        if( err == -EAGAIN )
        {
            printf( "ALSA event: -EAGAIN\n" );
        }
        else
        {
            report_alsa_error( "ALSA event", err );
        }
    }
#if 0   
    ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS | SND_SEQ_EVENT_LENGTH_FIXED | SND_SEQ_PRIORITY_NORMAL;
    ev.queue = self_queue_id;
    ev.time.tick = midi_tick;
#endif
}
#endif

static
void write_event_ALSA( void )
{
    int err;

    // Send event to buffer queue, drain when buffer full.
    // Will block if the output pool is full.
    // Returns the byte size of events still on the buffer, or an err.
    err = snd_seq_event_output( seq_handle_ALSA, &ev );
    if( err < 0 )
    {
        if( err == -EAGAIN )
        {
            printf( "ALSA event: -EAGAIN\n" );

            // Returns immediately after sending all events to the queue.
            // Returns the size of events still remaining on the queue.
            err = snd_seq_drain_output( seq_handle_ALSA );
            if( err < 0 )
            {
                report_alsa_error( "ALSA EAGAIN drain", err );
                return;
            }
            usleep( 2000 );
            err = snd_seq_event_output( seq_handle_ALSA, &ev );
            if( err < 0 )
                report_alsa_error( "ALSA EAGAIN event", err );
            // do not drain it so it indicates to throttle
        }
        else
        {
            report_alsa_error( "ALSA event", err );
        }
        return;
    }

    // Send queue to the kernel pool.
    err = snd_seq_drain_output( seq_handle_ALSA );
    if( err > 0 )
    {
        // Full kernel pool.        
        printf( "musserv ALSA drain: Has %i left\n", err );
    }
    else if( err < 0 )
    {
        if( err == -EAGAIN )
        {
            // kernel queue full
        }
        else
        {
            printf( "err=%i  seq_handle_ALSA=%p  ", err, seq_handle_ALSA );
            report_alsa_error( "ALSA write drain", err );
        }
    }
}

#endif  // RAWMIDI


// Called by: playmus
// Return > 0 only when can output at least 4 events.
static
int get_queue_avail_ALSA( void )
{
    int avail = 0;

#ifdef ALSA_RAWMIDI
    int i_queue_free;
  // Update the queue status.
//  ioctl(seqfd, SNDCTL_SEQ_GETOUTCOUNT, &i_queue_free );


    return avail;
#else
    // QUEUED
    // While throttling, this is the only handler to flush the queues.
    static int  prev_cur_size = 0;
 static int  turn = 0;
    
     turn ++;

    // snd_seq_event_output_pending measures current content of self_queue.
    int cur_size = snd_seq_event_output_pending( seq_handle_ALSA );
    if( cur_size < 0 )  // err
        goto error_handler;

    // Something still in our queue means system buffer is full.
    if( cur_size > 0 )
    {
        // Drain returns size left in queue.
        cur_size = snd_seq_drain_output( seq_handle_ALSA );
        if( cur_size < 0 )
        {
            if( cur_size == -EAGAIN )
                goto full_queue;
            goto error_handler;
        }
    }

    // sizeof event = 28
    // Throttle when our queue is not draining completely to kernel queue.
    avail = 1 - cur_size - prev_cur_size;
    prev_cur_size = cur_size;
    // At end of music, seems to have about 4 seconds of queued music.

    return avail;

full_queue:
    return -100;  // full, cannot drain
   
error_handler:
    seq_error_count ++;
    return -1000;  // error, cannot send

#endif // RAWMIDI
}


#ifdef ALSA_RAWMIDI
#else
static
byte  device_playing_ALSA( void )
{
    uint32_t qtick3 = ALSA_tick_time();
#ifdef DEBUG_MIDI_DATA   
//  printf("%s: tick_time=%i    queue_tick_time=%i\n", (midi_tick>qtick3)? "WAIT": "DONE", midi_tick, qtick3 );
#endif
    return ( midi_tick > qtick3 );
}
#endif



// TiMidity is Standard Midi
// Rawmidi is raw midi without timing commands.

//  channel :  0..15
static
void note_on_ALSA(int note, int channel, int volume)
{
#ifdef ALSA_RAWMIDI
    // MIDI_NOTEON = 0x90      (soundcard.h)
    // MIDI_CMD_NOTEON = 0x90  (asoundef.h)
    midiout( MIDI_CMD_NOTEON | channel );
    midiout( note );  // General Midi has assigned codes to notes, 0..127
    midiout( volume );  // velocity  0..127
    write_buffer_ALSA();
#else    
    // QUEUED
    // SND_SEQ_EVENT_NOTEON == 6
    ev.type = SND_SEQ_EVENT_NOTEON;
    // data = snd_seq_ev_note_t
    ev.data.note.channel = channel;
    ev.data.note.note = note;
    ev.data.note.velocity = volume;
    // blocks when full
    write_event_ALSA();
#endif // RAWMIDI

#ifdef ALL_OFF_FIX
    channel_used[channel] = 1;
    note_used[note] = 1;
#endif
}

//  channel :  0..15
static
void note_off_ALSA(int note, int channel, int volume)
{
#ifdef ALSA_RAWMIDI
    // MIDI_NOTEOFF = 0x80     (soundcard.h)
    // MIDI_CMD_NOTEOFF = 0x80 (asoundef.h)
    midiout( MIDI_NOTEOFF | channel );
    midiout( note );
    midiout( volume );  // velocity
      // some controllers use NOTEOFF velocity, some don't
    write_buffer_ALSA();
#else
    // QUEUED
    // SND_SEQ_EVENT_NOTEON == 7
    ev.type = SND_SEQ_EVENT_NOTEOFF;
    // data = snd_seq_ev_note_t
    ev.data.note.channel = channel;
    ev.data.note.note = note;
    ev.data.note.velocity = volume;
    // blocks when full
    write_event_ALSA();
#endif
}

#ifdef PITCHBEND_128
//  value : 128 = normal
#else
//  value : 64 = normal
#endif
static
void pitch_bend_ALSA(int channel, int value)
{
    // normal = 0x2000
#ifdef ALSA_RAWMIDI
    // MIDI_PITCH_BEND = 0xE0  (soundcard.h)
    // MIDI_CMD_BENDER = 0xE0  (asoundef.h)
    midiout( MIDI_CMD_BENDER | channel );
#ifdef PITCHBEND_128
    // convert 128 to 0x2000, << 7
    midiout( 0 ); // lower 7 bits
    midiout((value & 0x7F)); // upper 7 bits
#else
    // convert 64 to 0x2000, << 8
    midiout( 0 ); // lower 7 bits
    midiout(((((int)value) << 1) & 0x7F)); // upper 7 bits
#endif     

#else
    // QUEUED
    // see snd_seq_ev_set_pitchbend()
    // SND_SEQ_EVENT_PITCHBEND == 13
    ev.type = SND_SEQ_EVENT_PITCHBEND;
    // data = snd_seq_ev_ctrl_t
    ev.data.control.channel = channel;
    // signed 32 bit, normal = 0, range -0x2000..0x1FFF,
#ifdef PITCHBEND_128
    //  value : 128 = normal
    ev.data.control.value = (value - 128) << 8;
#else
    //  value : 64 = normal
    ev.data.control.value = (value - 64) << 8;
#endif
    // blocks when full
    write_event_ALSA();

#endif  // RAWMIDI
}


static
void control_change_ALSA(int controller, int channel, int value)
{
#ifdef DEBUG_ALSA_CC
  if( controller != 0x07 )
  {
     printf( "ALSA  %s, channel %i, value %i  midi_tick=%i\n", midi_control_string(controller), channel, value, midi_tick );
  }
#endif
    // Must handle controller = CTL_MAIN_VOLUME, CTL_PAN,
    // From soundcard.h
    // CTL_MAIN_VOLUME = 0x07  
    // CTL_PAN = 0x0a

    // From alsa
    // MIDI_CTL_MSB_MAIN_VOLUME = 0x07
    // MIDI_CTL_MSB_PAN = 0x0a

#ifdef ALSA_RAWMIDI
    // MIDI_CTL_CHANGE  = 0xB0  (soundcard.h)
    // MIDI_CMD_CONTROL = 0xB0  (asoundef.h)
    midiout( MIDI_CMD_CONTROL | channel );
    midiout( controller );
    midiout( value );
    write_buffer_ALSA();

#else
    // QUEUED
    // see snd_seq_ev_set_controller()
    // SND_SEQ_EVENT_CONTROLLER == 10
    ev.type = SND_SEQ_EVENT_CONTROLLER;
    // data = snd_seq_ev_cntrl_t
    ev.data.control.channel = channel;
    // controller
    ev.data.control.param = controller;
    // signed 32 bit
    ev.data.control.value = value;
    // blocks when full
    write_event_ALSA();

#endif // RAWMIDI
}

// unused
void set_tempo_ALSA( void )
{
    // Change tempo by sending an event.
    // from qmus2midi
    // 0x00, 0xff, 0x51, 0x03, 0x09, 0xa3, 0x1a   // usec/quarter_note
    // delta = 0x00
    // cmd = 0xff => meta event
    //   c = 0x51 => tempo
    //   len = 0x03
    //   value = 0x09<<16 | 0xa3<<8 | 0x1a
    ev.type = SND_SEQ_EVENT_TEMPO;
//   ev.port = port;
    ev.time.tick = midi_tick;
    snd_seq_ev_set_dest( &ev, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_TIMER );
    ev.data.queue.queue = self_queue_id;
    ev.data.queue.param.value = 0x09a31a;
    write_event_ALSA();
    // restore
    ev.dest = seq_info->port_addr;
}

#ifdef ALSA_RAWMIDI
#else
// QUEUED
static
void setup_tempo_ALSA( void )
{
    int err;
    snd_seq_queue_tempo_t * q_tempo;  // private type

    // Fixed at 140 ticks/sec.
    // Setup the playback tempo
    snd_seq_queue_tempo_alloca( & q_tempo );

    // Tempo must be setup, or get no sound.
    // microseconds per tick
    // default 500000 is 120 bpm
    snd_seq_queue_tempo_set_tempo( q_tempo, 1000000L * 120 / 140 );
    // ticks per quarter note
    snd_seq_queue_tempo_set_ppq( q_tempo, TICKS_PER_QUARTER );  // see sdl/i_sound.c, qmus2mid

    err = snd_seq_set_queue_tempo( seq_handle_ALSA, self_queue_id, q_tempo );
    if( err < 0 )
        report_alsa_error( "queue tempo", err );
}
#endif  // RAWMIDI

void patch_change_ALSA(int patch, int channel)
{
#ifdef ALSA_RAWMIDI
    // MIDI_PGM_CHANGE = 0xC0      (soundcard.h)
    // MIDI_CMD_PGM_CHANGE = 0xC0  (asoundef.h)
    midiout( MIDI_CMD_PGM_CHANGE | channel );
    midiout( patch );
    write_buffer_ALSA();
#else
    // QUEUED

#ifdef DEBUG_ALSA_PC
    printf( "ALSA patch %i, channel %i\n", patch, channel );
#endif
    // see snd_seq_ev_set_pgmchange
    // SND_SEQ_EVENT_PGMCHANGE = 11
    ev.type = SND_SEQ_EVENT_PGMCHANGE;
    // data = snd_seq_ev_cntrl_t
    ev.data.control.channel = channel;
    ev.data.control.param = 0;  // unused
    // signed 32 bit
    ev.data.control.value = patch; // program number
    // blocks when full
    write_event_ALSA();

#endif // RAWMIDI
}


// Wait until
//  tick_time : wakeup time in ticks
static
void midi_wait_ALSA( uint32_t tick_time )
{
    midi_tick = tick_time;
#ifdef ALSA_RAWMIDI
    snd_rawmidi_drain( rawmidi_ALSA ); // let queue go empty
    SEQ_WAIT_TIME( tick_time );   // wait, absolute time
    write_buffer_ALSA();
#else
#ifdef DEBUG_ALSA_TICK
  uint32_t qtick2 = ALSA_tick_time();
  printf("musserv: tick_time=%i    queue_tick_time=%i\n", midi_tick, qtick2 );
#endif
    ev.time.tick = tick_time;
    snd_seq_drain_output( seq_handle_ALSA );
#endif
}


// This is called to start, or restart, the song ticks at 0.
// action : mmt_e
static
void midi_timer_ALSA(byte action)
{
#ifdef DEBUG_MIDI_DATA
    printf( "musserv: ALSA start timer, midi_tick=%i\n", midi_tick );
#endif
   
#ifdef ALSA_RAWMIDI
    // indexed by mmt_e
    static const byte MMT_to_MIDI_rawcmd[3] = {
       4,  // MMT_START: TMR_START, SEQ_START_TIMER();
       3,  // MMT_STOP:  TMR_STOP, SEQ_STOP_TIMER();
       5,  // MMT_CONTINUE: TMR_CONTINUE, SEQ_CONTINUE_TIMER();
    };

    // Reset midi timer to 0.
# if 0
    midiout( 0x81 ); // EV_TIMING
    midiout( MMT_to_MIDI_rawcmd[action] );
    midiout( 0 );
    midiout( 0 );
    midiout( 0 );
    midiout( 0 );
    midiout( 0 );
    midiout( 0 );
# else   
    byte * mbp = get_midiout( 8 );  // 8 bytes, zeroed
    mbp[0] = 0x81; // EV_TIMING
    mbp[1] = MMT_to_MIDI_rawcmd[action];
# endif
    write_buffer_ALSA();

    // Only MMT_START is used, and then only before play music.
    if( action == MMT_START )
    {
        midi_tick = 0;
    }
   
#else
    // QUEUED
    // Note: midi_tick=0, and queue_tick is still running.

    // Need to ensure that midi queue is empty before changing ticks.
    drain_sync_ALSA( 100 ); // ms

    usleep( 1000 );
   

    // indexed by mmt_e
    static const byte MMT_to_MIDI_cmd[3] = {
       SND_SEQ_EVENT_START,     // MMT_START
       SND_SEQ_EVENT_STOP,      // MMT_STOP
       SND_SEQ_EVENT_CONTINUE,  // MMT_CONTINUE
    };

    // Send START, Reset midi timer to 0.
    // snd_seq_ev_queue_control_t
    // see snd_seq_ev_set_queue_start, snd_seq_ev_set_queue_control
    ev.type = MMT_to_MIDI_cmd[action];
    snd_seq_ev_set_dest( &ev, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_TIMER );
    ev.data.queue.queue = self_queue_id;
    ev.data.queue.param.time.tick = midi_tick;

    ev.flags = (ev.flags & ~SND_SEQ_PRIORITY_MASK) | SND_SEQ_PRIORITY_HIGH;
    write_event_ALSA();

    // Note: queue_tick = 0;

    // restore normal
    ev.flags = (ev.flags & ~SND_SEQ_PRIORITY_MASK) | SND_SEQ_PRIORITY_NORMAL;
    ev.dest = seq_info->port_addr;

    // Only MMT_START is used, and then only before play music.
    if( action == MMT_START )
    {
        midi_tick = 0;
        ev.time.tick = 0;  // immediate
    }

#endif // RAWMIDI

}



static
void reset_midi_ALSA(void)
{
#ifdef ALSA_RAWMIDI

    // SNDCTL_SEQ_SYNC hangs the musserver
    // ioctl(seqfd, SNDCTL_SEQ_SYNC);
    // All notes off on used channels.
    // Being implemented at the driver, it has the most immediate effect.
    // ioctl(seqfd, SNDCTL_SEQ_RESET);
    snd_rawmidi_drop( rawmidi_ALSA );
    all_channel_all_notes_off();

    unsigned int channel;
    for (channel = 0; channel < 16; channel++)
    {
        // reset pitch bender
#ifdef PITCHBEND_128
        pitch_bend( channel, 128 );  // normal=128
#else
        pitch_bend( channel, 64 );   // normal=64
#endif
        // reset volume to 100
        chanvol[channel] = 100;
        control_change(CTL_MAIN_VOLUME, channel, volscale );
        // reset pan
        control_change(CTL_PAN, channel, 64 );
    }
   
#else
    // QUEUED
   
    // Setup the event.
    snd_seq_ev_clear( &ev );

    // see snd_seq_ev_schedule_tick
    //   flags |= SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;
    //   time.tick = tick
    //   queue = self_queue_id
    ev.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS | SND_SEQ_EVENT_LENGTH_FIXED | SND_SEQ_PRIORITY_NORMAL;
    ev.queue = self_queue_id;
    ev.time.tick = midi_tick;

    // port in midi, changed by midi port number meta event, which MUS does not have.
    ev.source.client = self_client_id;
    ev.source.port = self_port_id;
    ev.dest = seq_info->port_addr;

#if 1
    // snd_seq_event_output_pending measures current content of self_queue.
    int cur_size = snd_seq_event_output_pending( seq_handle_ALSA );

    // Something still in our queue means system buffer is full.
    if( cur_size > 0 )
    {
# if 1
#  ifdef DEBUG_ALSA_RESET       
        printf("musserv: reset_midi drop_output\n" );
#  endif
        int err18 = snd_seq_drop_output( seq_handle_ALSA );
        if( err18 < 0 )
            report_alsa_error( "           reset midi,  drop output", err18 );
# endif
    }
#endif

   
# if 1
// Problem ??
    all_channel_all_notes_off();
    unsigned int channel;
    for (channel = 0; channel < 16; channel++)
    {
        // reset pitch bender
#  ifdef PITCHBEND_128
        pitch_bend( channel, 128 );  // normal=128
#  else
        pitch_bend( channel, 64 );   // normal=64
#  endif
        // reset volume to 100
        chanvol[channel] = 100;
        control_change(CTL_MAIN_VOLUME, channel, volscale );
        // reset pan
        control_change(CTL_PAN, channel, 64 );
    }
# endif

# ifdef DEBUG_MIDI_CONTROL
    usleep( 1500 );  // long enough for synth to react
# endif
   
    int err2 = snd_seq_start_queue( seq_handle_ALSA, self_queue_id, NULL );
    // Still requires drain to make the queue play.
    // The queue does not start until START_QUEUE event is drained to the kernel (from aplaymidi).
    if( err2 < 0 )
        report_alsa_error( "reset midi, start_queue", err2 );
   
    ev.time.tick = 0; // start queue sets tick to 0
    midi_tick = 0;

# ifdef DEBUG_MIDI_CONTROL
    printf("musserv: start queue, tick=0\n");
    fflush(stdout);
    usleep( 1500 );  // long enough for synth to react
# endif

#endif // RAWMIDI
}


static
void seq_shutdown_ALSA(void)
{
#ifdef ALSA_RAWMIDI
    if( rawmidi_ALSA == NULL )
        return;

    snd_rawmidi_close( rawmidi_ALSA );
    rawmidi_ALSA = NULL;
#else
    // QUEUED
    if( seq_handle_ALSA == NULL )
        return;

    // Delete the port
    if( self_port_id >= 0 )
    {
# ifdef DEBUG_MIDI_CONTROL       
        printf( "musserv: ALSA delete_port  %i\n", self_port_id );
# endif
        snd_seq_delete_simple_port( seq_handle_ALSA, self_port_id );
        self_port_id = -1;
    }

    // Close the ALSA sequencer device connection.
    
# ifdef DEBUG_MIDI_CONTROL       
    printf( "musserv: ALSA seq_close %p\n", seq_handle_ALSA );
# endif
    snd_seq_close( seq_handle_ALSA );
    seq_handle_ALSA = NULL;

#endif  // RAWMIDI

    if( verbose )
        printf( "ALSA shutdown\n" );

#ifdef MUS_DEVICE_OPTION
    setup_DUMMY();
#endif

#ifdef DEBUG_MIDI_CONTROL       
    fflush(stdout);
#endif
}

static
int find_devs_ALSA( byte en_list, byte en_print )
{
#ifdef ALSA_RAWMIDI
    char * portname;
#else
    snd_seq_client_info_t  * client_info;
    snd_seq_port_info_t    * port_info;
#endif
    int num_found = 0;
    int err;

#ifdef DEBUG_MIDI_CONTROL
    printf("musserv: Find_devs_ALSA\n");
#endif

#ifdef ALSA_RAWMIDI
    if( en_list )
    {
        // Minimum open, just for getting port info.
        err = snd_rawmidi_open( NULL, & rawmidi_ALSA, portname, SND_RAWMIDI_NOBLOCK );
        if( err < 0 )  goto ret_done;

        printf( "Devices found ALSA:\n" );
    }
    if( ! rawmidi_ALSA )
        goto ret_done;


#else
    // QUEUED

    if( en_list )
    {
        // Minimum open, just for getting port info.
        err = snd_seq_open( &seq_handle_ALSA, "default", SND_SEQ_OPEN_OUTPUT, 0 );
        if( err < 0 )  goto ret_done;
    }
    if( ! seq_handle_ALSA )
        goto ret_done;
    
    printf( "Devices found ALSA:\n" );

    // Allocate the query structures, on the stack.  Do not need to free.
    snd_seq_client_info_alloca( & client_info );
    snd_seq_port_info_alloca( & port_info );

    snd_seq_client_info_set_client( client_info, -1 );

    for(;;)
    {
        // iterate clients
        err = snd_seq_query_next_client( seq_handle_ALSA, client_info );
        if( err < 0 )  break; // done

        // magical incantation as seen in aplaymidi
        unsigned int client_id = snd_seq_client_info_get_client( client_info );
        snd_seq_port_info_set_client( port_info, client_id );
        snd_seq_port_info_set_port( port_info, -1 );

        for(;;)
        {
            snd_seq_addr_t  port_addr;

            // iterate ports
            err	= snd_seq_query_next_port( seq_handle_ALSA, port_info ); 
            if( err < 0 )  break; // done
            
            // type and capability are encoded as flags
            unsigned int flags1 = snd_seq_port_info_get_type( port_info );
            // Require MIDI
            if( (flags1 & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0 )   continue;
            // Require WRITE, SUBS_WRITE
            unsigned int flags2 = snd_seq_port_info_get_capability( port_info );
            if( (flags2 & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
                       != (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE) )  continue;

            port_addr.client = snd_seq_port_info_get_client( port_info );
            port_addr.port = snd_seq_port_info_get_port( port_info );
            const char * client_name = snd_seq_client_info_get_name( client_info );
            const char * port_name = snd_seq_port_info_get_name( port_info );
            // Do not save into midi_info, as that is dependent upon soundcard.h
            dev_info_t * dip = detect_MDT( client_name, port_name, num_found, NULL );
            if( dip )  // MIDI found
            {
                // ALSA info fields
                dip->port_addr = port_addr;
            }

            if( en_print )
            {
                printf( "%-3i:%-3i    %s, %s\n", port_addr.client, port_addr.port, client_name, port_name );
            }
            num_found++;
        }
    }

#endif // RAWMIDI

    if( en_list )
    {
        seq_shutdown_ALSA();
    }

ret_done:
    return num_found;
}


// Init, load, setup the selected device
static
void seq_init_setup_ALSA(int sel_dvt, int dev_type, int port_num)
{
    const char * msg;
#ifdef ALSA_RAWMIDI
    char * portname;
#else
#endif
    int err;

#ifdef DEBUG_MUSIC_INIT
    printf("ALSA init\n");
#endif
#ifdef DEBUG_MIDI_CONTROL   
    printf("musserv: seq_init_setup_ALSA( %i, %i, %i)\n", sel_dvt, dev_type, port_num);
#endif
    clear_MDT();
   
#ifdef ALSA_RAWMIDI
    // Open for write
    // Get exclusive access to the MIDI device.
    // We are NOT using SND_RAWMIDI_APPEND, because that would intermix bits of music
    // with other users MIDI events.
    msg = "open";
    err = snd_rawmidi_open( NULL, & rawmidi_ALSA, portname, SND_RAWMIDI_NOBLOCK );
    if( err < 0 )
    {
        goto handle_msg;
    }

#else
    // QUEUED

    // Open the ALSA sequencer device for writing.
    // Name of the port, such as "default", "hw:0,0"
    // Open for output (write).
    // NONBLOCK makes writes return -EAGAIN instead of blocking.
    msg = "seq_open";
    err = snd_seq_open( &seq_handle_ALSA, "default", SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK );
    if( err < 0 )
       goto handle_err;
   
# ifdef DEBUG_MUSIC_INIT
    printf("ALSA opened, seq_handle_ALSA = %p\n", seq_handle_ALSA);
# endif

    // Our program name.
    err = snd_seq_set_client_name( seq_handle_ALSA, "DoomLegacy" );
    // ignore any err, not important

    self_client_id = snd_seq_client_id( seq_handle_ALSA );

# ifdef DEBUG_MUSIC_INIT
    printf("ALSA client_id = %i\n", self_client_id);
# endif
   
    msg = "simple_port";
    self_port_id = snd_seq_create_simple_port( seq_handle_ALSA, "DoomLegacy_MIDI",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
//        SND_SEQ_PORT_TYPE_MIDI_GENERIC );
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION );
    if( self_port_id < 0 )
        goto handle_err;

# ifdef DEBUG_MUSIC_INIT
    printf("ALSA port_id = %i\n", self_port_id);
# endif

    msg = "create queue";
    self_queue_id = snd_seq_alloc_named_queue( seq_handle_ALSA, "DoomLegacy" );
    if( self_queue_id < 0 )
       goto handle_err;

# ifdef DEBUG_MUSIC_INIT
    printf("ALSA queue_id = %i\n", self_queue_id);
# endif
    if( verbose )
        printf( "Created queue %i, port %i\n", self_queue_id, self_port_id );

    if( ! find_devs_ALSA( 0, verbose ) )
        goto no_devices;

    if( determine_setup_MDT( SD_ALSA, sel_dvt ) < 0 )
        goto no_devices;

# ifdef DEBUG_MUSIC_INIT
    // Default buffer size 16384
    err = snd_seq_get_output_buffer_size( seq_handle_ALSA );
    if( err < 0 )
        printf( "musserv output_buffer_size : %s\n",  snd_strerror(err) );
    else
        printf( "musserv output_buffer_size : %i\n",  err );
# endif

# ifdef DEBUG_MUSIC_INIT
    printf("ALSA connect %i to %i:%i\n", self_port_id, seq_info->port_addr.client, seq_info->port_addr.port);
# endif
    // Force the rawmidi ports to remain open, so ALSA does not reset them after events.
//    err = snd_seq_connect_to( seq_handle_ALSA, 0, port1.client, port1.port );
    err = snd_seq_connect_to( seq_handle_ALSA, self_port_id, seq_info->port_addr.client, seq_info->port_addr.port );
    if( err < 0 )
       goto handle_err;

# ifdef DEBUG_MUSIC_INIT
    printf( "ALSA connected\n");
# endif

    setup_tempo_ALSA();
    
    msg = "start queue";
    err = snd_seq_start_queue( seq_handle_ALSA, self_queue_id, NULL );
    // Still requires drain to make the queue play.
    // The queue does not start until START_QUEUE event is drained to the kernel (from aplaymidi).
    if( err < 0 )
       goto handle_err;
# ifdef DEBUG_MUSIC_INIT
    printf( "ALSA queue started\n");
# endif

# endif  // RAWMIDI

# ifdef DEBUG_MIDI_CONTROL
    usleep( 2000000 ); // DEBUG
# endif
   
    use_dev = SD_ALSA;
    // Assign interface functions
    note_on = note_on_ALSA;
    note_off = note_off_ALSA;
    pitch_bend = pitch_bend_ALSA;
    control_change = control_change_ALSA;
    patch_change = patch_change_ALSA;
    midi_wait = midi_wait_ALSA;
    get_queue_avail = get_queue_avail_ALSA;
    device_playing = device_playing_ALSA;

    if( verbose )
        printf( "ALSA active\n" );

    return;

#ifdef ALSA_RAWMIDI
handle_msg:
    printf( "musserv RawMidi %s: %s\n", msg, snd_strerror(err) );
    return;
#else
handle_err:
    printf( "musserv ALSA MIDI %s: %s\n", msg, snd_strerror(err) );
    return;
#endif
no_devices:
    use_dev = SD_NULL;
    return;
}



// ALSA SECTION
#endif



// ============= Device selection


static
void  clear_MDT( void )
{
    found_mdt = MDT_NULL;
    num_midi_ports = 0;
#ifdef SYNTH_DEVICES   
    num_synth_ports = 0;
#endif
    memset( &dev_info, 0, sizeof(dev_info) );
}

//  client_name: name from info
//  port_info: info string from info
//  port_num : index into minfo
//  dev_type_wb : dev_type writeback ptr
// return mdt_type
static
dev_info_t * detect_MDT( const char * client_name, const char * port_info, int port_num, int * dev_type_wb )
{
    dev_info_t * dip;  // query info
    const char * name = "";
    int dev_type = 0;
    byte mdt_type;

#ifdef DEBUG_MUSIC_INIT
    printf("MIDI %i = %s : %s\n", port_num, client_name, port_info );
#endif

//    if( strstr( name, "aseqdump" ) )

#ifdef DEV_TIMIDITY
    if( strstr( client_name, "TiMidi" ) )
    {
#ifdef DEBUG_MUSIC_INIT
        printf("FOUND Timidity\n");
#endif
        dev_type = DEV_TYPE_TIMIDITY;
        mdt_type = MDT_TIMIDITY;
        dip = & dev_info.timidity;
        name = port_info;
        goto recognized;
    }
#endif

#ifdef DEV_FLUIDSYNTH
    if( strstr( client_name, "FLUID" ) )
    {
#ifdef DEBUG_MUSIC_INIT
        printf("FOUND FluidSynth\n");
#endif
        dev_type = DEV_TYPE_FLUIDSYNTH;
        mdt_type = MDT_FLUIDSYNTH;
        dip = & dev_info.fluidsynth;
        name = client_name;
        goto recognized;
    }
#endif

#ifdef DEV_EXTMIDI
    // test for physical midi port
    if( strstr( port_info, "Midi Through" ) )
    {
//        dev_type = 0;
        mdt_type = MDT_EXT_MIDI;
        dip = & dev_info.ext_midi;
        name = client_name;
        goto recognized;
    }
#endif
    return NULL;

recognized:
    if( dev_type && dev_type_wb )
        *dev_type_wb = dev_type;

    if( dip->mdt_type )
        return NULL;  // ignore additional ports of same type

    // record details of recognized port
    found_mdt = mdt_type;
    dip->mdt_type = mdt_type;
    dip->port_num = port_num;
#ifdef DEV_FM_SYNTH
    dip->num_voices = 0;  // midi
#endif
    dl_strncpy( dip->name, name, sizeof(dip->name));
    return dip;
}


// sd_dev : SD_OSS, SD_ALSA
// mdt : MDT_TIMIDITY, MDT_FLUID, etc.
// dev_info : & dev_info.timidity, etc
static
void  setup_midi_MDT( byte sd_dev, dev_info_t * devinfo )
{
    use_dev = sd_dev;
    use_mdt = MDT_MIDI;  // MDT_EXT_MIDI, MDT_TIMIDITY, MDT_FLUIDSYNTH
    seq_info = devinfo;
    seq_dev = seq_info->port_num;

    const char * sd_str =
#ifdef DEV_ALSA     
       (sd_dev==SD_ALSA)? "ALSA" :
#endif
#ifdef DEV_OSS
       (sd_dev==SD_OSS)? "OSS" :
#endif
       "UNK";

    if (verbose)
        printf("Use %s midi device %d (%s)\n", sd_str, seq_dev+1, seq_info->name);
}


// Implement search, setup the selected MDT.
// pref_dev : search or device selection
static
int  determine_setup_MDT( byte sd_dev, byte pref_dev )
{
    const char * pc;  // pref sequence chars
   
    if( pref_dev < MDT_SEARCH1 || pref_dev > MDT_DEV9 )  // table limits
    {
        pref_dev = MDT_SEARCH1;
    }
    pc = search_order[pref_dev];

    use_mdt = MDT_NULL;
    for( ; ; pc++ )
    {
        switch( *pc )
        {
         case 'T': // Timidity dev
#ifdef DEV_TIMIDITY
            if( dev_info.timidity.mdt_type == 0 )  continue;
            setup_midi_MDT( sd_dev, &dev_info.timidity );
#endif
            break;
         case 'L': // Fluidsynth dev
#ifdef DEV_FLUIDSYNTH
            if( dev_info.fluidsynth.mdt_type == 0 )  continue;
            setup_midi_MDT( sd_dev, &dev_info.fluidsynth );
#endif
            break;
         case 'E': // Ext Midi dev
#ifdef DEV_EXTMIDI
            if( dev_info.ext_midi.mdt_type == 0 )  continue;
            setup_midi_MDT( sd_dev, &dev_info.ext_midi );
#endif
            break;
         case 'F': // FM dev
#ifdef DEV_FM_SYNTH
            if( dev_info.fm_synth.mdt_type == 0 )  continue;
            setup_fm( sd_dev );
#endif
            break;
         case 'A': // Awe32 dev
#ifdef DEV_AWE32_SYNTH
            if( dev_info.awe_synth.mdt_type == 0 )  continue;
            setup_awe( sd_dev );
#endif
            break;
         case 'g':  // new device
         case 'h':  // new device
         case 'j':  // new device
         case 'k':  // new device
            break;
         case 0:  // end of list
            goto err_exit;	   
        }
        if( use_mdt != MDT_NULL )  break;  // success
    }
    return 0;

err_exit:
    return -1;   
}



// ============= Interface


static
void update_all_channel_volume( void )
{
    // Update all channels with new volscale applied.
    int ch;
    for (ch = 0; ch < 16; ch++)
    {
        control_change(CTL_MAIN_VOLUME, ch, chanvol[ch] * volscale / 100 );
    }
}

// volume: 0=silent, 100=normal, 127=loud
void volume_change( int channel, int volume )
{
    chanvol[channel] = volume;
    control_change(CTL_MAIN_VOLUME, channel, volume * volscale / 100 );
}

static int logscale[32] = {
   0,15, 25,33, 40,45, 50,55, 59,62, 65,68, 70,73, 75,77,
   79,81, 83,85, 87,89, 90,92, 93,94, 95,97, 98,99, 100,100
};

void master_volume_change(int volume)
{
    volume = (volume < 0 ? 0 : (volume > 31 ? 31 : volume));
    volscale = logscale[volume];

    switch( use_dev )
    {
#ifdef DEV_OSS
     case SD_OSS:
        master_volume_change_OSS( volume );
        break;
#endif
#ifdef DEV_ALSA
     case SD_ALSA:
        update_all_channel_volume();
        break;
#endif
     default:
        break;
    }
}


// action : mmt_e
void midi_timer(byte action)
{
    // Reset midi timer to 0.
    midi_tick = 0;

    switch( use_dev )
    {
#ifdef DEV_OSS
     case SD_OSS:
        midi_timer_OSS( action );
        break;
#endif
#ifdef DEV_ALSA
     case SD_ALSA:
        midi_timer_ALSA( action );
        break;
#endif
     default:
        break;
    }
}


static
void all_channel_all_notes_off( void )
{
    // Notes off on all channels.
    int ch;
    for (ch = 0; ch < 16; ch++)
    {
        // All notes off
        // OSS:  CMM_ALL_NOTES_OFF       0x7b
        // ALSA: MIDI_CTL_ALL_NOTES_OFF  0x7b
        control_change( CMM_ALL_NOTES_OFF, ch, 0 );
    }
}

void pause_midi(void)
{
#ifdef DEV_OSS
    if( use_dev == SD_OSS )
    {
        pause_midi_OSS();
        return;
    }
#endif

    all_channel_all_notes_off();
}

void reset_midi(void)
{
    seq_error_count = 0;

    switch( use_dev )
    {
#ifdef DEV_OSS
     case SD_OSS:
        reset_midi_OSS();
        break;
#endif
#ifdef DEV_ALSA
     case SD_ALSA:
        reset_midi_ALSA();
        break;
#endif
     default:
        break;
    }
    usleep( 500 );
}

#ifdef SOUND_DEVICE_OPTION
// indexed by sound_dev_e
static byte search_devices_dev[][2] = {
// Entries are limited to devices for which we have a MIDI search.
  { SD_ALSA, SD_OSS  },  // search 0, default
  { SD_OSS,  SD_ALSA },  // search 1
  { SD_ALSA, SD_OSS  },  // search 2
  { SD_ALSA, SD_OSS  },  // search 3
  { SD_OSS,  SD_ALSA },  // SD_OSS
  { SD_ALSA, SD_OSS  },  // SD_ESD
  { SD_ALSA, SD_OSS  },  // SD_ALSA
  { SD_ALSA, SD_OSS  },  // SD_PULSE
  { SD_ALSA, SD_OSS  },  // SD_JACK
  { SD_ALSA, SD_OSS  },  // SD_DEV6
  { SD_ALSA, SD_OSS  },  // SD_DEV7
  { SD_ALSA, SD_OSS  },  // SD_DEV8
  { SD_ALSA, SD_OSS  },  // SD_DEV9
};
#endif

// Init, load, setup the selected device
// Called by main
void seq_init_setup( byte sel_snddev, byte sel_dvt, int dev_type, int port_num)
{
#ifdef SOUND_DEVICE_OPTION
    byte * search_list = NULL; // no search
    int i;

#ifdef DEBUG_MIDI_CONTROL   
    verbose = 4;
    printf( "musserv: seq_init_setup( sel_snddev=%i, sel_dvt=%i, dev_type=%i, port_num=%i)\n", sel_snddev, sel_dvt, dev_type, port_num );
#endif
   
    if( sel_dvt > MDT_QUERY )   // 99
    {
#ifdef DEBUG_MIDI_CONTROL       
        printf( "musserv: seq_init_setup: bad sel_dvt = %i\n", sel_dvt );
#endif
        // MUTE
        seq_shutdown();
        setup_DUMMY();
        return;
    }
   
    // Always does search due to sound devices that do not support MIDI
    // or we don't have MIDI interface for them.
    if( sel_snddev > SD_DEV9 )
        sel_snddev = SD_NULL; // default to search 0

    search_list = & search_devices_dev[ sel_snddev ][0];  // search list
   
    for(i=0; i<2; i++)
    {
        sel_snddev = *(search_list++);  // search next

        if( use_dev && (sel_snddev != use_dev) )
            seq_shutdown();

        switch( sel_snddev )
        {
#ifdef DEV_OSS
         case SD_OSS:
#ifdef DEBUG_MIDI_CONTROL   
            printf( "musserv: seq_init_setup: search OSS  sel_dvt= %i\n", sel_dvt );
#endif
            seq_init_setup_OSS( sel_dvt, dev_type, port_num );
            break;
#endif
#ifdef DEV_ALSA
         case SD_ALSA:
#ifdef DEBUG_MIDI_CONTROL   
            printf( "musserv: seq_init_setup: search ALSA  sel_dvt= %i\n", sel_dvt );
#endif
            seq_init_setup_ALSA( sel_dvt, dev_type, port_num );
            break;
#endif
         default:
#ifdef DEBUG_MIDI_CONTROL   
            printf( "musserv: seq_init_setup: search unknwon sel_snddev= %i  sel_dvt= %i\n", sel_snddev, sel_dvt );
#endif
            break;
        }

        if( use_dev > 0 )
            break;
    }
    // If playing, trigger a restart.
    restart_playing();

#else

    // Single sound device, SOUND_DEV1
    if( use_dev )
        seq_shutdown();

# if SOUND_DEV1 == SD_OSS
    seq_init_setup_OSS( sel_dvt, dev_type, port_num );
# endif
# if SOUND_DEV1 == SD_ALSA
    seq_init_setup_ALSA( sel_dvt, dev_type, port_num );
# endif
#endif

    // may return without success
    if( (use_dev == SD_NULL) && no_devices_exit )
    {
#ifdef DEBUG_MIDI_CONTROL       
        printf( "musserv: seq_init_setup: EXIT no devices\n" );
#endif
        cleanup_exit(1, "no music devices found" );
    }
}

void seq_shutdown(void)
{
    if( use_dev == 0 )
        return;

    reset_midi();
#ifdef DEV_OSS
    seq_shutdown_OSS();
#endif
#ifdef DEV_ALSA
    seq_shutdown_ALSA();
#endif
    use_dev = 0;
}

// Called by command_line
int list_devs( void )
{
    int num = 0;

    clear_MDT();

#ifdef DEV_OSS
    num += list_devs_OSS();
#endif
#ifdef DEV_ALSA
    num += find_devs_ALSA( 1, 1 );
#endif

    return num;
}
