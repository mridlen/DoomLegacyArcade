// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: snd_driver.c 1403 2018-07-06 09:49:21Z wesleyjohnson $
//
// Copyright (C) 2020 by DooM Legacy Team.
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
//
//
// DESCRIPTION:
//      System sound drivers.
//      Used by Internal-Sound Interface, and by SoundServer.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
  // stdio, stdlib, stdarg, string, math
  // sys/types
  // doomdef, doomtype

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/msg.h>
#include <sys/time.h>

#include <errno.h>

// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>

// See ENABLE_SOUND in doomdef.h.

#include "s_sound.h"
  // NUMSFX, flags

#include "lx_ctrl.h"
  // SOUND_DEVICE_OPTION
  // DEV_xx
  // SOUND_DEV1
  // NEED_DLOPEN
  // sound_dev_e

#include "snd_driver.h"

#ifdef NEED_DLOPEN
# ifdef HAVE_DLOPEN
    // dlopen library
#   include <dlfcn.h>
#else
# warning "Need dlopen libary, dynamic loading disabled."
# endif
#endif

// SCO OS5 and Unixware OSS sound output
#if defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
#endif

//============ DEFINE

// Debugging
//#define DEBUG_INIT_SOUND

//#define DEBUG_SOUND

//#define DEBUG_MIXER

//#define DEBUG_VOLUME

//#define DEBUG_TIME


//============ GLOBAL

// mix_sfxvolume is from main program, server has local copy.
extern  int   mix_sfxvolume;  // 0..31

// sound_init
extern  byte sound_init;
  // 0 = not ready, shutdown
  // 1 = ready for selection, no device
  // 2 = sound server ready
  // 6 = sound device ready
  // 7 = sound device active

// verbose from main program, server has local copy.
extern  byte  verbose;

#ifdef SNDSERV
// Sound Server
#include "sndserv/soundsrv.h"
// Sound driver access to S_sfx will use srv_sfx (simpler copy).
extern  server_sfx_t   srv_sfx[NUMSFX];
#define  S_sfx   srv_sfx

// server linkage
extern  byte  cv_num_channels;  // server has local copy

#else
// Direct driver access
//#include "i_system.h"
//#include "i_sound.h"
//#include "s_sound.h"
//#include "m_argv.h"

// Direct access to num channels.
#define cv_num_channels    (cv_numChannels.EV)

#endif


//============ SETTINGS

// max channels of all sound formats
#ifdef QUAD_AUDIO
// For future development.
# define MIX_CHANNELS    4
#else
# define MIX_CHANNELS    2
#endif

// Number of samples to request.
#define ADJ_SAMPLECOUNT        (SAMPLECOUNT * SAMPLERATE / DOOM_SAMPLERATE)
//#define MIXBUFFER_SAMPLESIZE   ((SAMPLECOUNT * SAMPLERATE / DOOM_SAMPLERATE) * MIX_CHANNELS)
#define MIXBUFFER_BYTESIZE     (( ADJ_SAMPLECOUNT * 4 ) * MIX_CHANNELS * sizeof(int16_t))

#ifdef DEBUG_WINDOWED
sound_dev_e  sound_device = 0;
#else
byte  sound_device = 0;  // sound_dev_e
#endif

typedef enum {
  AM_mono8,
  AM_stereo16,
  AM_quad16,
#ifdef QUAD_AUDIO
  AM_max = AM_quad16
#else
  AM_max = AM_stereo16
#endif
}  audio_mode_e;

// Setup by sound port
static byte  port_volume = 128; // volume adjustment for port  0..255, 128=normal
static byte  audio_mode;  // audio_mode_e, quad, stereo, 8bit mono
static byte  audio_channels;     // 1 or 2
static byte  audio_format_bits;  // 8 bit or 16 bit
static byte  byte_per_sample;  // sample size, all channels
static byte  byte_sample_shift;
#define  BYTES_TO_SAMPLES( x )  ((x)>>(byte_sample_shift))
#define  SAMPLES_TO_BYTES( x )  ((x)<<(byte_sample_shift))

// From SetSfxVolume
static unsigned int  mix_volume7;  // 7 bit mixer volume  0..127

// mix_sfxvolume is global in main program and in server, 0..31.
//extern int mix_sfxvolume;  // i_sound.h


//============ SOUND CHANNELS

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Needed for calling the actual sound output.
#define NUM_MIX_CHANNEL            16
#define CHANNEL_NUM_MASK  (NUM_MIX_CHANNEL-1)
// Defined in i_sound.c and soundsrv.c
// #define cv_num_channels    cv_num_channels

typedef struct {
    // the channel data, current position
    byte *  data_ptr;  // NULL=inactive
    // the channel data end pointer
    byte *  data_end;  // past last sound sample
    // the channel volume lookup, modifed by master volume
    int  * left_vol_tab, * right_vol_tab;
    // the channel step amount
    unsigned int step;
    // 0.16 bit remainder of last step
    unsigned int step_remainder;
    // When the channel starts playing, and there are too many sounds,
    // determine which to kill by oldest and priority.
    unsigned int age_priority;
    // The data sample rate
    unsigned int sample_rate;
    // The channel handle, determined on registration,
    //  might be used to unregister/stop/modify.
    // Lowest bits are the channel num.
    int  handle;
    // SFX id of the playing sound effect.
    // Used to catch duplicates (like chainsaw).
    uint16_t  id;
   
    // normalized 0..255, before master volume
    byte  left_volume, right_volume;
   
#ifdef SURROUND_SOUND
    byte  invert_right;
#endif

} mix_channel_t;

static mix_channel_t   mix_channel[NUM_MIX_CHANNEL];
static byte  active_channels = 0;  // optimization
static byte  want_more;  // request another update call


//============ MIXER

// The global mixing buffer.
// All active internal channels are mixed into the buffer,
// which is submitted to the audio device.
// Stereo is interlaced, a frame (sample) is 2 channels, of int16 values.
// Alternative format is 8 bit sound mono.
#ifdef DEBUG_MIXER
static byte     mixbuffer[MIXBUFFER_BYTESIZE+64];  // easier to work in bytes
# define  MIXBUFFER_CANARY  0xAA
#else
static byte     mixbuffer[MIXBUFFER_BYTESIZE+4];  // easier to work in bytes
#endif
#define MIXBUFFER_END    (&mixbuffer[MIXBUFFER_BYTESIZE])

#ifdef MIXOUT_PROTECT
// Driver sets this to protect the samples at mixer_out,
// such as if DMA directly from mixbuffer.
static byte *   mixer_protect = MIXBUFFER_END;
#endif

// Some device drivers may write a partial sample.
// They will use a byte count of (mixer_mix - mixer_out).
// At empty buffer, mixer_out = mixer_mix.
static byte *   mixer_out = mixbuffer; // current device position in mixbuffer
static byte *   mixer_mix = mixbuffer; // kept one past last mixbuffer position mixed
static unsigned int  mixer_samples = 0;   // total samples in mixer buffer
static unsigned int  limit_samples;  // setting: limit samples mixed to control latency

// QUIET pads the buffer when the sounds end.
// This avoids shutting down device drivers too quickly, when another sound follows.
static unsigned int  quiet_samples = 0;  // current continuous quiet


//============ UPDATE

// Proto
static void LXD_update_sound_dummy( void );

// UpdateSound indirect function
static void (*LXD_update_sound)(void) = LXD_update_sound_dummy;


//============ TABLES

// Pitch to stepping lookup, 16.16 fixed point.
static int steptable[256];

// Volume lookups.
static int vol_lookup[128][256];

// Note: this previously was called by S_Init.
// See soundserver initdata().
// Called by LXD_Init_sound_device
//  audio_mode : global
static
void setup_mixer_tables( void )
{
    // Init internal lookups (raw data, mixing buffer, channels).
    // This function sets up internal lookups used during
    //  the mixing process. 
    int i, j, v;

    double base_step = (double)DOOM_SAMPLERATE / (double)SAMPLERATE;
   
    // This table provides step widths for pitch parameters.
    for (i = -128; i < 128; i++)
        steptable[i+128] = (int) (base_step * pow(2.0, (i / 64.0)) * 65536.0);

    // Generates volume lookup tables
    //  which also turn the unsigned samples
    //  into signed samples.
    for (i = 0; i < 128; i++)
    {
        for (j = 0; j < 256; j++)
        {
#if 1
            // Lower volume, use whole table.
            v = (i * (j - 128) * 256) / 255;
#else
            // Only 64 volume levels got used, otherwise mixing clips.
            v = (i * (j - 128) * 256) / 127;
#endif
            vol_lookup[i][j] = (audio_mode == AM_mono8)?
                 v * 4 // 8bit audio
               : v;    // 16bit audio
        }
    }
}


//============ DEBUG FUNCTIONS

// GenPrintf is in main program and server.
// void GenPrintf(const byte emsg, const char *fmt, va_list ap);

#ifdef DEBUG_TIME
uint32_t  msec_t0;
uint32_t  msec_t1;

static
uint32_t  get_msec( void )
{
    struct timeval      tp;
    struct timezone     tzp;
  
    gettimeofday(&tp, &tzp);
    return  (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

#if 0
static
void  print_msec( const char * msg, uint32_t t0, uint32_t t1 )
{
    // difference between 
    GenPrintf(EMSG_debug, "%s = %i msec\n", msg, t1 - t0 );
}
#endif

static
void  print_msec_ex( const char * msg, uint32_t t0, uint32_t t1, uint32_t t3 )
{
    // difference between 
    uint32_t td = t1 - t0;
    if( td > t3 )
        GenPrintf(EMSG_debug, "%s = %i msec\n", msg, td );
}
#endif

#ifdef DEBUG_MIXER
static int trap_cnt = 0;
void  error_trap( void )
{
    // trap errors
    trap_cnt++;
    if( trap_cnt > 10 )
        LXD_update_sound = LXD_update_sound_dummy;
}
#endif


//============ DYNAMIC LOAD FUNCTIONS

#ifdef NEED_DLOPEN

typedef struct {
   void ** DL_func;
   char * func_name;
} lib_sym_t;

// Load a list of lib functions using dlopen, dlsym.
//   dlp : list of lib functions
//   num : number in list
//   libname : filename of library
//   port_libp : library handle from dlopen
// Return -1 on error from dlopen.
int  load_lib( const char * who, const lib_sym_t * dlp, byte num, const char * libname, /*OUT*/ void ** port_libp )
{
    char * msg = NULL;
    void * libp;
    const lib_sym_t * list_end = &dlp[num];

    dlerror();  // to clear error

    if( verbose )
        GenPrintf( EMSG_debug, "DLOPEN: loading %s \n", who );

    libp = dlopen( libname, RTLD_NOW );
    if( libp == NULL )
    {
        msg = "library not found";	
        goto report_err;
    }

    while( dlp < list_end )
    {
        void * lda = dlsym( libp, dlp->func_name );
        if( lda == NULL )
        {
            msg = "library load failed";
            goto report_err;
        }
        *(dlp->DL_func) = lda;
        dlp++;
    }
   
    if( port_libp )  *port_libp = libp;
    return 0;
    
report_err:
    {
        // Have default message.	
        const char * dlmsg = dlerror();
        if( ! dlmsg )  dlmsg = "";
        GenPrintf( EMSG_error, "DLOPEN %s: %s: %s\n", who, msg, dlmsg );
    }
    return -1;
}
#endif


//============ SOUND FUNCTIONS

static void LXD_update_sound_dummy( void )
{
    mixer_samples = 0;
    want_more = 0;
}


//  mode : AM_ mode
// Return index to the mode tables.
static
byte  select_audio_mode( byte mode )
{
#ifdef QUAD_AUDIO
    if( mode == AM_quad16 )  // 4 channel
    {
        audio_mode = AM_quad16;
        audio_format_bits = 16;
        audio_channels = 4;
        byte_per_sample = 4 * 2;  // (quad, 16 bit) = 8 bytes
        byte_sample_shift = 3; // (quad, 16bit) = 8 bytes
        return 2;
    }
#endif
    if( mode == AM_stereo16 )  // 2 channel
    {
        audio_mode = AM_stereo16;
        audio_format_bits = 16;
        audio_channels = 2;
        byte_per_sample = 2 * 2;  // (stereo, 16 bit) = 4
        byte_sample_shift = 2; // (stereo, 16bit) = 4 bytes
        return 1;
    }
    if( mode == AM_mono8 )  // 8 bit mono
    {
        audio_mode = AM_mono8;
        audio_format_bits = 8;
        audio_channels = 1;
        byte_per_sample = 1;  // 8 bit
        byte_sample_shift = 0; // (mono, 8bit) = 1 byte
        return 0;
    }

    return 0xFF;
}

static
void stop_sound( mix_channel_t * chp )
{
    chp->data_ptr = NULL;  // stop the channel
    active_channels |= 0x80; // force recheck in sfx_mixer
}


// Mixes all active sound channels, for one sample period.
// Retrieves a given number of samples from the raw sound data,
// modifies it according to the current channel parameters.
// Mixes the per channel samples into the mixbuffer,
// clamping it to the allowed volume range,
// and sets up the transfer of the mixbuffer to the sound device.
//  req_samples : the number of samples requested by the sound port
static
void sfx_mixer( unsigned int req_samples )
{
    int rem_buf_s2;
    mix_channel_t * chp;  // mix_channel
    byte *  mixp;  // ptr in mixbuffer
   
    if( active_channels & 0x80 )   // Forced recount of active channels
    {
        active_channels = 0;

        // Check for all quiet here, to keep this out of the mixer loop.
        for( chp = &mix_channel[0]; chp < &mix_channel[cv_num_channels]; chp++ )
        {
            // Check for active channels.
            if( chp->data_ptr )  active_channels++;
        }
    }

#ifdef DEBUG_MIXER
    if(( mixer_out < mixbuffer ) || ( mixer_out > MIXBUFFER_END ))
    {
        GenPrintf(EMSG_debug, "mixer entry: ERROR  mixer_out outside mixbuffer [%i]\n", mixer_out - mixbuffer );
        error_trap();
    }
    if(( mixer_mix < mixbuffer ) || ( mixer_mix > MIXBUFFER_END+1 ))
    {
        GenPrintf(EMSG_debug, "mixer entry: ERROR  mixer_mix outside mixbuffer [%i]\n", mixer_mix - mixbuffer );
        error_trap();
    }
#ifdef MIXOUT_PROTECT
    if( mixbuffer[MIXBUFFER_BYTESIZE] != MIXBUFFER_CANARY )
    {
        GenPrintf(EMSG_debug, "mixer 1: ERROR  mixerbuffer overrun %i %i %i %i\n",
                 mixbuffer[MIXBUFFER_BYTESIZE], mixbuffer[MIXBUFFER_BYTESIZE+1], mixbuffer[MIXBUFFER_BYTESIZE+2], mixbuffer[MIXBUFFER_BYTESIZE+3] );
        error_trap();
    }
#endif
#endif

    // Limit the request.
    if( req_samples > limit_samples )
        req_samples = limit_samples;  // not enough buffer, or exceeds latency limit
   
    // Append to existing mixer buffer content, when possible.
    // Limit to room left in mixbuffer.
    rem_buf_s2 = BYTES_TO_SAMPLES(MIXBUFFER_END - mixer_mix);
    if( req_samples > rem_buf_s2 )
    {
        // Must keep content of mixer buffer contiguous.
        // Cannot move mixer_mix to buffer[0] until mixer_samples are 0, it is used by some drivers.
        if( mixer_samples == 0 )  // check for buffer empty
        {
#ifdef MIXOUT_PROTECT
            // Avoid overwrite of the buffer contents. Overwrite annoys some of the sound drivers.
            if( mixer_protect < MIXBUFFER_END )
            {
                mixer_protect += 8; // ease off protection to avoid deadlock
                int rem_buf_s1 = BYTES_TO_SAMPLES(mixer_protect - mixbuffer);
                if( rem_buf_s1 > rem_buf_s2 )
                {
                    // Mix from start of buffer.
                    if( req_samples > rem_buf_s1 )
                        req_samples = rem_buf_s1;  // not enough buffer
                    mixer_out = mixer_mix = mixbuffer;
                }
                else
                    req_samples = rem_buf_s2;  // not enough buffer
            }
            else
                mixer_out = mixer_mix = mixbuffer;
#else
            // Mix from start of buffer.
            mixer_out = mixer_mix = mixbuffer;
#endif
        }
        else
            req_samples = rem_buf_s2;  // not enough buffer
    }

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT.
   
    mixp = mixer_mix;

    if( active_channels == 0 )
        goto quiet_mix;
   
    quiet_samples = 0;

#ifdef DEBUG_MIXER
    if( req_samples > limit_samples )
    {
        GenPrintf(EMSG_debug, "mixer 4: ERROR  req_samples %i\n", req_samples );
        error_trap();
    }
    if( mixer_samples > limit_samples )
    {
        GenPrintf(EMSG_debug, "mixer 4: ERROR  mixer_samples %i\n", mixer_samples );
        error_trap();
    }
#endif
   
    while( mixer_samples < req_samples )
    {
        // Mix current sound data.
        // Data, from raw sound, for right and left.
        register int dl, dr;
        // Reset left/right value. 
        dl = 0;
        dr = 0;

        // Love thy L2 chache - made this a loop.
        // Now more channels could be set at compile time as well.
        for( chp = &mix_channel[0]; chp < &mix_channel[cv_num_channels]; chp++ )
        {
            // Check channel, if active.
            if (chp->data_ptr)
            {
                // Get the raw data from the channel. 
                register unsigned int sample = * chp->data_ptr;
                // Add left and right part for this channel (sound)
                //  to the current data.
                // Adjust volume accordingly.
                dl += chp->left_vol_tab[sample];
#ifdef SURROUND_SOUND
                if( chp->invert_right )
                  dr -= chp->right_vol_tab[sample];
                else
                  dr += chp->right_vol_tab[sample];
#else
                dr += chp->right_vol_tab[sample];
#endif
                // Increment fixed point index
                chp->step_remainder += chp->step;
                // MSB is next sample
                chp->data_ptr += chp->step_remainder >> 16;
                // Keep the fractional index part
                chp->step_remainder &= 0xFFFF;

                // Check whether we are done.
                if( chp->data_ptr >= chp->data_end )
                {
                    chp->data_ptr = NULL;
                    active_channels --;
                    // still must output the last sound sample
                }
            }
        }

        // Clamp to range. Left hardware channel.
        // Has been char instead of int16.
        // if (dl > 127) *leftout = 127;
        // else if (dl < -128) *leftout = -128;
        // else *leftout = dl;

        if( audio_mode == AM_stereo16 )
        {
            // 16 bit stereo
            // Left and right channel are in global mixbuffer, alternating.
            // left channel
            ((int16_t*)mixp)[0] =
                (dl > 0x7fff)?   0x7fff
              : (dl < -0x8000)? -0x8000
              :	dl;

            // right channel
            ((int16_t*)mixp)[1] =
                (dr > 0x7fff)?   0x7fff
              : (dr < -0x8000)? -0x8000
              :	dr;
            mixp += 4; // 2 * 2
        }
        else
        {
            // 8 bit mono
            if (dl > 0x7fff)
                dl = 0x7fff;
            else if (dl < -0x8000)
                dl = -0x8000;
            // [WDJ] Stero to mono combining, from 1.42, with no explanation.
            uint16_t  sdl = dl ^ 0xfff8000;

            if (dr > 0x7fff)
                dr = 0x7fff;
            else if (dr < -0x8000)
                dr = -0x8000;
            uint16_t  sdr = dr ^ 0xfff8000;

            *(mixp++) = (((sdr + sdl) / 2) >> 8);
        }
        mixer_samples ++;

        if( active_channels == 0 )  // was last sound sample
            goto quiet_mix_append;
    }
    goto active_out; 

quiet_mix:
    // No active channels.
    // The buffer is NOT all 0, required extra code to save some extra clearing of the buffer.
#ifdef DEBUG_MIXER_xx
    GenPrintf(EMSG_debug, "mixer quiet: req_samples=%i, quiet_samples=%i\n", req_samples, quiet_samples );
#endif

quiet_mix_append:
    // Append quiet to the mixer buffer at mixp.
   {
    // Previous tests have ensured that req_samples will fit within buffer at mixp.
    int quiet_s1 = (int)req_samples - (int)mixer_samples;
    int quiet_b2 = MIXBUFFER_END - mixp;
      
    if( quiet_s1 <= 0 )
        goto active_out;

    if( quiet_b2 <= 0 )
        goto active_out;

#if 1
    // Unneeded if req_samples is limited correctly.
    // TODO: change to PARANOIA
    rem_buf_s2 = BYTES_TO_SAMPLES(quiet_b2);
    if( quiet_s1 > rem_buf_s2 )
    {
# ifdef DEBUG_MIXER_xx
        GenPrintf(EMSG_debug, "mixer quiet limit: quiet_s1=%i rem_buf_s2=%i\n", quiet_s1, rem_buf_s2 );
# endif
        quiet_s1 = rem_buf_s2;
    }
#endif

#ifdef DEBUG_MIXER
    if(( mixer_out < mixbuffer ) || ( mixer_out > MIXBUFFER_END ))
    {
        GenPrintf(EMSG_debug, "mixer quiet: ERROR  mixer_out outside mixbuffer [%i]\n", mixer_out - mixbuffer );
        error_trap();
    }
    if(( mixer_mix < mixbuffer ) || ( mixer_mix > MIXBUFFER_END+1 ))
    {
        GenPrintf(EMSG_debug, "mixer quiet: ERROR  mixer_mix outside mixbuffer [%i]\n", mixer_mix - mixbuffer );
        error_trap();
    }
#endif

    mixer_samples += quiet_s1;
    if( quiet_samples < 0x3FFFFFFF )
        quiet_samples += quiet_s1;

    int quiet_b1 = SAMPLES_TO_BYTES(quiet_s1);
    memset( mixp, 0, quiet_b1 );
    mixp += quiet_b1;
   }

active_out:
    // Normal buffer tracking
    mixer_mix = mixp;

#ifdef DEBUG_MIXER
    if(( mixer_out < mixbuffer ) || ( mixer_out > MIXBUFFER_END ))
    {
        GenPrintf(EMSG_debug, "mixer: ERROR  mixer_out outside mixbuffer [%i]\n", mixer_out - mixbuffer );
        error_trap();
    }
    if(( mixer_mix < mixbuffer ) || ( mixer_mix > MIXBUFFER_END+1 ))
    {
        GenPrintf(EMSG_debug, "mixer: ERROR  mixer_mix outside mixbuffer [%i]\n", mixer_mix - mixbuffer );
        error_trap();
    }
#endif
    return;
}



//============ OSS SECTION

#ifdef DEV_OSS   
// OSS sound Interface
#ifdef LINUX
# ifdef FREEBSD
   // FreeBSD
#  include <sys/soundcard.h>
# else
   // Linux
#  include <linux/soundcard.h>
# endif
#endif
// SCO OS5 and Unixware OSS sound output
#if defined(SCOOS5) || defined(SCOUW2) || defined(SCOUW7)
# include <sys/soundcard.h>
#endif

#ifdef DEV_OPT_OSS
// There is no OSS library to be loaded.
// OSS is now handled by ALSA
#endif

#define MYIOCTL_REPORT

#ifdef MYIOCTL_REPORT
// Internal-Sound Interface, IOCTL.
//
// Safe ioctl, convenience.
// For Internal-Sound Interface.
void myioctl(int fd, int command, int *arg)
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
        GenPrintf(EMSG_error, "ioctl(dsp,%d,arg) failed\n", command);
        GenPrintf(EMSG_error, "errno=%d\n", errno);
        // [WDJ] No unnecessary fatal exits, let the player savegame.
        if( ioctl_err_count < 254 )
            ioctl_err_count++;
        if( ioctl_err_count > 10 )
            ioctl_err_off = 20;
       
//        exit(-1);
    }
}
#else
#define  myioctl   ioctl
#endif


// OSS output device.
static int oss_audio_fd = -1;

static void LXD_update_sound_OSS( void )
{
    if( (quiet_samples - mixer_samples) > 4096 )
        goto flush_buffer;

    sfx_mixer( SAMPLECOUNT );

//    unsigned int wr_size = SAMPLES_TO_BYTES(mixer_samples);
    unsigned int wr_size = mixer_mix - mixer_out;  // partial sample safe
    ssize_t cnt = write( oss_audio_fd, mixer_out, wr_size );  // 8 or 16 bit
    if( cnt < 0 )
    {
        want_more = 0;
        return;  // err, not handled
    }

    if( cnt < wr_size )
    {
        // OSS did not take all the submitted samples.
        // Does not happen very often.
        mixer_out += cnt;
        mixer_samples -= BYTES_TO_SAMPLES(cnt);
        want_more = 0;
        return;
    }
    // wrote all of buffer
    want_more = (wr_size < (SAMPLECOUNT*7/10)) && (quiet_samples < 128);
    
flush_buffer:
    mixer_out = mixer_mix;
    mixer_samples = 0;
}

static void LXD_shutdown_OSS( void )
{
    // Clean up and release of the DSP device.
    close(oss_audio_fd);
    oss_audio_fd = -1;
    LXD_update_sound = LXD_update_sound_dummy;
}

// Return errcode on failure, 0 on success.
static byte LXD_init_OSS( void )
{
    const char * msg;
    int i;

#ifdef DEV_OPT_OSS
    // There is no OSS library to be loaded.
    // All commands are written to the device.
#endif

    // Need snd_pcm_oss, root can create it by "cp x.wav /dev/dsp".
    oss_audio_fd = open("/dev/dsp", O_WRONLY);
    if( oss_audio_fd < 0 )
    {
        msg = "open /dev/dsp";
        goto report_msg;
    }

#ifdef DEBUG_INIT_SOUND
    GenPrintf(EMSG_debug, "OSS: handle= %i\n", oss_audio_fd );
#endif

#ifdef SOUND_RESET
    myioctl(oss_audio_fd, SNDCTL_DSP_RESET, 0);
#endif

    msg = "default";
    port_volume = 129;
    audio_mode = AM_mono8;  // default, 8 bit mono
    if (getenv("DOOM_SOUND_SAMPLEBITS") == NULL)
    {
        // MIX_CHANNELS == 2
        myioctl(oss_audio_fd, SNDCTL_DSP_GETFMTS, &i);
#ifdef __BIG_ENDIAN__
        if (i &= AFMT_S16_BE)
#else
        if (i &= AFMT_S16_LE)
#endif
        {
            msg = "16bit stereo";
            select_audio_mode( AM_stereo16 );

#ifdef __BIG_ENDIAN__
            i = AFMT_S16_BE;
#else
            i = AFMT_S16_LE;
#endif
            myioctl(oss_audio_fd, SNDCTL_DSP_SETFMT, &i);
            i = 11 | (2 << 16);
            myioctl(oss_audio_fd, SNDCTL_DSP_SETFRAGMENT, &i);
            i = 1;
            myioctl(oss_audio_fd, SNDCTL_DSP_STEREO, &i);
        }
    }

    if( audio_mode == AM_mono8 )  // default, 8 bit mono
    {
        msg = "8bit mono";
        select_audio_mode( AM_mono8 );  // mono, 8bit

        i = AFMT_U8;
        myioctl(oss_audio_fd, SNDCTL_DSP_SETFMT, &i);
        i = 10 | (2 << 16);
        myioctl(oss_audio_fd, SNDCTL_DSP_SETFRAGMENT, &i);
    }
   
    i = SAMPLERATE;
    myioctl(oss_audio_fd, SNDCTL_DSP_SPEED, &i);

    GenPrintf(EMSG_info, " OSS: configured %s audio device\n", msg );
    LXD_update_sound = LXD_update_sound_OSS;
    sound_init = 6;
    return 0;

report_msg:
    GenPrintf(EMSG_error, "OSS: %s\n", msg);
    perror(NULL);
    return 3;

#if 0
report_err:   
    GenPrintf(EMSG_error, "OSS: %s\n", msg);
    if( oss_audio_fd >= 0 )
        LXD_shutdown_OSS();
    return 4;
#endif
}

#endif // End OSS


//============ ESD SECTION

#ifdef DEV_ESD
// Enlightened Sound Daemon, Sound device interface
#ifdef __BIG_ENDIAN__
#  warning  "ESD is only little-endian"
#endif

#include <esd.h>
#include <sys/types.h>
#include <sys/socket.h>

//#define DEBUG_ESD

// ESD: No problems with start up or underrun sounds.
// There is a bit of latency with the first sound.
// Quieter than ALSA.
// This is best used with sound server.
// Something is blocking and making gameplay stutter
// when used directly.
#define ESD_TRAIL_PAD  2048

#ifdef DEV_OPT_ESD
#define LIBESD_NAME   "libesd.so"
void * esd_lp = NULL;
// ESD functions
static int (*DL_esd_play_stream_fallback)( esd_format_t, int, const char *, const char *);
#define esd_play_stream_fallback  (*DL_esd_play_stream_fallback)

const lib_sym_t  symlist_ESD[] =
{
  { (void*)& DL_esd_play_stream_fallback, "esd_play_stream_fallback" }
};
#endif

static int  esd_audio_fd = -1;

static void LXD_update_sound_ESD( void )
{
#define ESD_chunk_size   128   
    ssize_t cnte;
    int wr_size;

    if( (quiet_samples - mixer_samples) > ESD_TRAIL_PAD )
        goto flush_buffer;

#ifdef SNDSERV   
    sfx_mixer( SAMPLECOUNT );
#else
    // to reduce stuttering
    sfx_mixer( SAMPLECOUNT/2 );
#endif

#ifdef DEBUG_TIME
    msec_t0 = get_msec();
#endif
   
    // Very slow write, 85 to 132 msec to write SAMPLECOUNT.
    // Tic time is only 33 msec, so it bogs down whole game.
//    wr_size = SAMPLES_TO_BYTES(mixer_samples);
    wr_size = mixer_mix - mixer_out;
#if 0
    for(  ; wr_size > ESD_chunk_size; wr_size -= ESD_chunk_size )
    {
        cnte = write( esd_audio_fd, mixer_out, ESD_chunk_size );
        if( cnte < 0 )
            return;  // err, not handled

        mixer_out += cnte;
        if( cnte < ESD_chunk_size )
            goto incomplete_write;

    }
#endif
    cnte = write( esd_audio_fd, mixer_out, wr_size );
    if( cnte < 0 )
        return;  // err, not handled

    mixer_out += cnte;
    if( cnte < wr_size )
        goto incomplete_write;

flush_buffer:   
    mixer_out = mixer_mix;
    mixer_samples = 0;
//    want_more = (wr_size < (SAMPLECOUNT*4/10)) && (quiet_samples < 32);

#ifdef DEBUG_TIME
    msec_t1 = get_msec();
    print_msec_ex( "ESD loop", msec_t0, msec_t1, 80 );
    msec_t0 = msec_t1;
#endif
   
    return;

incomplete_write:
#ifdef DEBUG_TIME
    msec_t1 = get_msec();
    print_msec_ex( "ESD loop incomplete", msec_t0, msec_t1, 80 );
    msec_t0 = msec_t1;
#endif
    mixer_samples = BYTES_TO_SAMPLES( mixer_mix - mixer_out );
    want_more = 0;
#ifdef DEBUG_SOUND   
    printf("ESD: incomplete write  mixer_samples=%i\n", mixer_samples );
#endif
    return;
}

static void LXD_shutdown_ESD( void )
{
#ifdef DEBUG_SOUND   
    printf( "LXD_shutdown_ESD\n" );
#endif
    // Clean up and release of the ESP connection.
    if( esd_audio_fd >= 0 )
        close(esd_audio_fd);
    esd_audio_fd = -1;
    LXD_update_sound = LXD_update_sound_dummy;
}

static byte LXD_init_ESD( void )
{
   //( int   samplerate, int   samplesize )
    const char * msg;
    unsigned int sample_rate = SAMPLERATE;
    int sockopt = 32;
    // ESound only supports 2 channels.
    int mode = ESD_BITS16 | ESD_STEREO | ESD_STREAM | ESD_PLAY;
    int err;

#ifdef DEBUG_SOUND_INIT
    printf( "LXD_init_ESD\n" );
#endif

#ifdef DEV_OPT_ESD
    // Dynamic load ESD library.
    if( esd_lp == NULL )
    {
#ifdef DEBUG_SOUND_INIT
        printf( "LXD_init_ESD: dlopen\n" );
#endif
        if( load_lib( "ESD", symlist_ESD, 1, LIBESD_NAME, /*OUT*/ &esd_lp ) < 0 )
            goto esd_done;
    }
#endif
   
    // Return < 0 when error.
    msg = "ESD open";
    esd_audio_fd = esd_play_stream_fallback( mode, sample_rate, NULL, NULL );
    if( esd_audio_fd < 0 )  goto esd_error;

    msg = "ESD setsockopt";
    err = setsockopt(esd_audio_fd, SOL_SOCKET, SO_SNDBUF, &sockopt, sizeof(sockopt));
    if( err != 0 )  goto esd_error;

    // 16bit stereo
    select_audio_mode( AM_stereo16 );

    GenPrintf(EMSG_info, " ESD: configured %s audio device\n", msg );
    LXD_update_sound = LXD_update_sound_ESD;
    port_volume = 130;
    return 0;

esd_error:
    GenPrintf(EMSG_error, "ESD: %s\n", msg);
    perror(NULL);
    goto esd_done;

esd_done:
    if( esd_audio_fd >= 0 )
    {
        LXD_shutdown_ESD();
        return 4;
    }
    return 3;
}
#endif


//============ ALSA SECTION

#ifdef DEV_ALSA
// ALSA sound Interface
#include <alsa/asoundlib.h>

//#define DEBUG_ALSA
#ifdef DEBUG_ALSA
static byte alsa_tick = 0;
#endif
#ifdef DEBUG_INIT_SOUND
static snd_output_t  *  alsa_output = NULL;
#endif

// Asynchronous callback by ALSA
// It has been reported that Linux systems that use PulseAudio are not
// safe using callback.
//#define ALSA_CALLBACK

// quiet time after sound
#define ALSA_TRAIL_PAD  2048

#ifdef DEV_OPT_ALSA
#define LIBALSA_NAME   "libasound.so"
void * alsa_lp = NULL;
// ALSA functions
static int (*DL_snd_pcm_open)(snd_pcm_t **, const char *, snd_pcm_stream_t, int);
#define snd_pcm_open  (*DL_snd_pcm_open)

static int (*DL_snd_pcm_close)(snd_pcm_t *);
#define snd_pcm_close  (*DL_snd_pcm_close)

static const char * (*DL_snd_pcm_name)(snd_pcm_t *);
#define snd_pcm_name  (*DL_snd_pcm_name)

// #define snd_pcm_hw_params_alloca(ptr)
// #define __snd_alloca

// This gets referenced by snd_pcm_hw_params_alloca. Required for correct linking.
static size_t (*DL_snd_pcm_hw_params_sizeof)();
#define snd_pcm_hw_params_sizeof  (*DL_snd_pcm_hw_params_sizeof)

static int (*DL_snd_pcm_hw_params_any)(snd_pcm_t *, snd_pcm_hw_params_t *);
#define snd_pcm_hw_params_any  (*DL_snd_pcm_hw_params_any)

static int (*DL_snd_pcm_hw_params)(snd_pcm_t *, snd_pcm_hw_params_t *);
#define snd_pcm_hw_params  (*DL_snd_pcm_hw_params)

static int (*DL_snd_pcm_hw_params_test_channels)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int);
#define snd_pcm_hw_params_test_channels  (*DL_snd_pcm_hw_params_test_channels)

static int (*DL_snd_pcm_hw_params_test_format)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
#define snd_pcm_hw_params_test_format  (*DL_snd_pcm_hw_params_test_format)

static int (*DL_snd_pcm_hw_params_set_access)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t);
#define snd_pcm_hw_params_set_access  (*DL_snd_pcm_hw_params_set_access)

static int (*DL_snd_pcm_hw_params_set_format)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
#define snd_pcm_hw_params_set_format  (*DL_snd_pcm_hw_params_set_format)

static int (*DL_snd_pcm_hw_params_set_channels)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int);
#define snd_pcm_hw_params_set_channels  (*DL_snd_pcm_hw_params_set_channels)

static int (*DL_snd_pcm_hw_params_set_rate_near)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
#define snd_pcm_hw_params_set_rate_near  (*DL_snd_pcm_hw_params_set_rate_near)

static int (*DL_snd_pcm_hw_params_set_buffer_size_near)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
#define snd_pcm_hw_params_set_buffer_size_near  (*DL_snd_pcm_hw_params_set_buffer_size_near)

static int (*DL_snd_pcm_hw_params_set_periods)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int, int);
#define snd_pcm_hw_params_set_periods  (*DL_snd_pcm_hw_params_set_periods)

static int (*DL_snd_pcm_hw_params_set_buffer_size)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t);
#define snd_pcm_hw_params_set_buffer_size  (*DL_snd_pcm_hw_params_set_buffer_size)

static int (*DL_snd_pcm_sw_params)(snd_pcm_t *, snd_pcm_sw_params_t *);
#define snd_pcm_sw_params  (*DL_snd_pcm_sw_params)

// This gets referenced twice by ALSA functions. Required for correct linking.
static size_t (*DL_snd_pcm_sw_params_sizeof)();
#define snd_pcm_sw_params_sizeof  (*DL_snd_pcm_sw_params_sizeof)

static int (*DL_snd_pcm_sw_params_current)(snd_pcm_t *, snd_pcm_sw_params_t *);
#define snd_pcm_sw_params_current  (*DL_snd_pcm_sw_params_current)

static int (*DL_snd_pcm_sw_params_set_avail_min)(snd_pcm_t *, snd_pcm_sw_params_t *, snd_pcm_uframes_t);
#define snd_pcm_sw_params_set_avail_min  (*DL_snd_pcm_sw_params_set_avail_min)

static int (*DL_snd_pcm_sw_params_set_start_threshold)(snd_pcm_t *, snd_pcm_sw_params_t *, snd_pcm_uframes_t);
#define snd_pcm_sw_params_set_start_threshold  (*DL_snd_pcm_sw_params_set_start_threshold)

static int (*DL_snd_pcm_start)(snd_pcm_t *);
#define snd_pcm_start  (*DL_snd_pcm_start)

static snd_pcm_state_t (*DL_snd_pcm_state)(snd_pcm_t *);
#define snd_pcm_state  (*DL_snd_pcm_state)

static snd_pcm_sframes_t (*DL_snd_pcm_avail)(snd_pcm_t *);
#define snd_pcm_avail  (*DL_snd_pcm_avail)

static snd_pcm_sframes_t (*DL_snd_pcm_avail_update)(snd_pcm_t *);
#define snd_pcm_avail_update  (*DL_snd_pcm_avail_update)

static int (*DL_snd_pcm_prepare)(snd_pcm_t *);
#define snd_pcm_prepare  (*DL_snd_pcm_prepare)

static int (*DL_snd_pcm_recover)(snd_pcm_t *, int, int);
#define snd_pcm_recover  (*DL_snd_pcm_recover)

static snd_pcm_sframes_t (*DL_snd_pcm_writei)(snd_pcm_t *, const void *, snd_pcm_uframes_t);
#define snd_pcm_writei  (*DL_snd_pcm_writei)

#ifdef ALSA_CALLBACK
static int (*DL_snd_async_add_pcm_handler)(snd_async_handler_t **, snd_pcm_t *, snd_async_callback_t, void *);
#define snd_async_add_pcm_handler  (*DL_snd_async_add_pcm_handler)
#endif

#ifdef DEBUG_ALSA
static const char * (*DL_snd_pcm_state_name)(const snd_pcm_state_t);
#define snd_pcm_state_name  (*DL_snd_pcm_state_name)
#endif

#ifdef DEBUG_INIT_SOUND
static int (*DL_snd_output_stdio_attach)(snd_output_t **, FILE *, int);
#define snd_output_stdio_attach  (*DL_snd_output_stdio_attach)

static int (*DL_snd_pcm_dump)(snd_pcm_t *, snd_output_t *);
#define snd_pcm_dump  (*DL_snd_pcm_dump)
#endif

static const char * (*DL_snd_strerror)(int);
#define snd_strerror  (*DL_snd_strerror)

const lib_sym_t  symlist_ALSA[] =
{
  { (void*)& DL_snd_pcm_open, "snd_pcm_open" },
  { (void*)& DL_snd_pcm_close, "snd_pcm_close" },
  { (void*)& DL_snd_pcm_name, "snd_pcm_name" },
  { (void*)& DL_snd_pcm_hw_params, "snd_pcm_hw_params" },
  { (void*)& DL_snd_pcm_hw_params_sizeof, "snd_pcm_hw_params_sizeof" },
  { (void*)& DL_snd_pcm_hw_params_any, "snd_pcm_hw_params_any" },
  { (void*)& DL_snd_pcm_hw_params_test_channels, "snd_pcm_hw_params_test_channels" },
  { (void*)& DL_snd_pcm_hw_params_test_format, "snd_pcm_hw_params_test_format" },
  { (void*)& DL_snd_pcm_hw_params_set_access, "snd_pcm_hw_params_set_access" },
  { (void*)& DL_snd_pcm_hw_params_set_format, "snd_pcm_hw_params_set_format" },
  { (void*)& DL_snd_pcm_hw_params_set_channels, "snd_pcm_hw_params_set_channels" },
  { (void*)& DL_snd_pcm_hw_params_set_rate_near, "snd_pcm_hw_params_set_rate_near" },
  { (void*)& DL_snd_pcm_hw_params_set_periods, "snd_pcm_hw_params_set_periods" },
  { (void*)& DL_snd_pcm_hw_params_set_buffer_size, "snd_pcm_hw_params_set_buffer_size" },
  { (void*)& DL_snd_pcm_hw_params_set_buffer_size_near, "snd_pcm_hw_params_set_buffer_size_near" },
  { (void*)& DL_snd_pcm_sw_params, "snd_pcm_sw_params" },
  { (void*)& DL_snd_pcm_sw_params_sizeof, "snd_pcm_sw_params_sizeof" },
  { (void*)& DL_snd_pcm_sw_params_current, "snd_pcm_sw_params_current" },
  { (void*)& DL_snd_pcm_sw_params_set_avail_min, "snd_pcm_sw_params_set_avail_min" },
  { (void*)& DL_snd_pcm_sw_params_set_start_threshold, "snd_pcm_sw_params_set_start_threshold" },
  { (void*)& DL_snd_pcm_start, "snd_pcm_start" },
  { (void*)& DL_snd_pcm_state, "snd_pcm_state" },
  { (void*)& DL_snd_pcm_avail, "snd_pcm_avail" },
  { (void*)& DL_snd_pcm_avail_update, "snd_pcm_avail_update" },
  { (void*)& DL_snd_pcm_prepare, "snd_pcm_prepare" },
  { (void*)& DL_snd_pcm_recover, "snd_pcm_recover" },
  { (void*)& DL_snd_pcm_writei, "snd_pcm_writei" },
#ifdef ALSA_CALLBACK
  { (void*)& DL_snd_async_add_pcm_handler, "snd_async_add_pcm_handler" },
#endif
#ifdef DEBUG_ALSA
  { (void*)& DL_snd_pcm_state_name, "snd_pcm_state_name" },
#endif
#ifdef DEBUG_INIT_SOUND
  { (void*)& DL_snd_output_stdio_attach, "snd_output_stdio_attach" },
  { (void*)& DL_snd_pcm_dump, "snd_pcm_dump" },
#endif
  { (void*)& DL_snd_strerror, "snd_strerror" },
};
#endif

snd_pcm_t * alsa_handle = NULL;
snd_pcm_format_t alsa_format;
//snd_pcm_stream_t  alsa_stream = SND_PCM_STREAM_PLAYBACK;


// Notes:
// snd_pcm_drain is blocking, do not use
//        snd_pcm_drain( alsa_handle );



#ifdef ALSA_CALLBACK
// Overly complicated ALSA indirectly set of parameters.
snd_async_handler_t * alsa_pcm_callback = NULL;
extern byte  sound_callback_active;


// The callback from ALSA to get more frames.
// This must be treated as an interrupt routine, do not do anything else.
static void  ALSA_callback( snd_async_handler_t *  alsa_async_reg )
{
    snd_pcm_t * callback_handle;
    snd_pcm_state_t  alsa_state;
    int cnt, err;
    snd_pcm_sframes_t  avail;

#ifdef DEBUG_ALSA
    alsa_tick ++;
#endif

    if( (quiet_samples - mixer_samples) > ALSA_TRAIL_PAD )
        goto flush_buffer;

    // Same as alsa_handle
    callback_handle = snd_async_handler_get_pcm( alsa_async_reg );
#ifdef DEBUG_ALSA
    if( callback_handle != alsa_handle )
        GenPrintf(EMSG_debug, "ALSA handle=%p alsa_handle=%p\n", callback_handle, alsa_handle );
#endif

    if( ! callback_handle )
        return;

    avail = snd_pcm_avail_update( callback_handle );
    if( avail < 0 )
    {
        err = avail; // error code
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA %i: snd_pcm_avail_update err=%s\n", alsa_tick, snd_strerror(err) );
#endif
        // Required to clear the error ??
        err = snd_pcm_prepare( alsa_handle );
        if( err < 0 )
            goto handle_err;

        avail = snd_pcm_avail_update( alsa_handle );
        if( avail < 0 )
            goto handle_err;
    }

#ifdef DEBUG_ALSA
    GenPrintf(EMSG_debug, "ALSA CALLBACK: avail=%i\n", avail );
#endif

    sfx_mixer( avail );

    // ALSA state can be, OPEN, SETUP, PREPARED, RUNNING, XRUN, DRAINING, PAUSED, SUSPENDED, DISCONNECTED
    alsa_state = snd_pcm_state( callback_handle );

    // Check quiet.
    if( (alsa_state == SND_PCM_STATE_RUNNING) && (quiet_samples > ALSA_TRAIL_PAD) && (quiet_samples > mixer_samples) )
    {
        // Have written all non-quiet samples plus trail padding.
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA %i: go quiet\n", alsa_tick);
#endif
        goto flush_buffer;  // let ALSA go idle
    }

    if( (alsa_state == SND_PCM_STATE_XRUN) || (alsa_state == SND_PCM_STATE_SETUP) )
    {
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA: XRUN\n");
#endif
        err = snd_pcm_prepare( callback_handle );
        if( err < 0 )  goto handle_err;
    }


    // Write frames from buffer to PCM device.
    // Returns the number of frames actually written.
    snd_pcm_uframes_t frame_cnt = mixer_samples;
    cnt = snd_pcm_writei( callback_handle, mixer_out, frame_cnt ); // TO ALSA
    // The buffer must be left unchanged for a while.
    // Altering it puts noise in the output.
    if( cnt == -EPIPE )
    {
        // Broken Pipe
        // RECOVER
        err = snd_pcm_recover( alsa_handle, cnt, 0 );  // cnt has previous err
        cnt = snd_pcm_writei( alsa_handle, mixer_out, frame_cnt ); // TO ALSA
    }

    if( cnt < 0 )  goto handle_cnt_err;
    if( cnt < mixer_samples )
    {
        // Does not happen very often.
        mixer_samples -= cnt;
        mixer_out += SAMPLES_TO_BYTES(cnt);
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA full  %i:  write cnt=%i, mixer_samples %i\n", alsa_tick, cnt, mixer_samples );
#endif
        return;
    }

flush_buffer:
    mixer_samples = 0;
    mixer_out = mixer_mix;
    return;

handle_cnt_err:
    err = cnt; // cnt < 0
    goto handle_err;
   
handle_err:
#ifdef DEBUG_ALSA
    alsa_state = snd_pcm_state( alsa_handle );
#endif
    // Return err<0 is error.
    if( err == -EAGAIN )  // full
         return;

    if( err == -EPIPE )
    {
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug," ALSA  %i: ERR  -EPIPE hard underrun  %s\n", alsa_tick, snd_pcm_state_name( alsa_state ) );
#endif
        err = snd_pcm_prepare( callback_handle );
        if( (err < 0) && verbose )
            GenPrintf(EMSG_warn, "ALSA underrun\n" );
        return;
    }
#if 0   
    else if( err == -ESTRPIPE )
    {
        // suspend        
    }
#endif   
    else if( verbose )
        GenPrintf(EMSG_warn, "ALSA: %s\n", snd_strerror(err) );
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug," ALSA  %i: ERR  err=%i, %s  %s\n", alsa_tick, err, snd_strerror(err), snd_pcm_state_name( alsa_state ) );
#endif
}
#endif


static void LXD_update_sound_ALSA( void )
{
#ifdef ALSA_CALLBACK
    // Start the ALSA callback
    snd_pcm_start( alsa_handle );
#else
    // Direct update of ALSA
    int cnt, err;
    snd_pcm_state_t  alsa_state;
    snd_pcm_sframes_t  avail;

    if( ! alsa_handle )
    {
        GenPrintf(EMSG_warn, "ALSA submit sound without handle\n" );
        return;
    }
#ifdef DEBUG_ALSA
    alsa_tick ++;
#endif

#ifdef DEBUG_TIME
msec_t1 = get_msec();
print_msec_ex( "ALSA loop", msec_t0, msec_t1, 80 );
msec_t0 = msec_t1;
#endif

    if( (quiet_samples - mixer_samples) > ALSA_TRAIL_PAD )
        goto flush_buffer;  // let ALSA go idle
     
    // Avoid very small buffer updates.
    // They cause noise in the output.
    // ALSA returns pending errors as neg values.
    // Note: snd_pcm_avail() is slower but more accurate
    // Note: returns number of samples, in multiples of period.
    avail = snd_pcm_avail_update( alsa_handle );
    if( avail < 0 )
    {
        err = avail; // error code
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA %i: snd_pcm_avail_update err=%s\n", alsa_tick, snd_strerror(err) );
#endif
        // Required to clear the error ??
        err = snd_pcm_prepare( alsa_handle );
        if( err < 0 )
            goto handle_err;

        avail = snd_pcm_avail_update( alsa_handle );
        if( avail < 0 )
            goto handle_err;
    }
    if( avail < 128 )
        return;

    sfx_mixer( avail );

    // ALSA state can be, OPEN, SETUP, PREPARED, RUNNING, XRUN, DRAINING, PAUSED, SUSPENDED, DISCONNECTED
    // Normally RUNNING.
    alsa_state = snd_pcm_state( alsa_handle );
   
    // Check quiet.
    if( (alsa_state == SND_PCM_STATE_RUNNING) && (quiet_samples > ALSA_TRAIL_PAD) && (quiet_samples > mixer_samples) )
    {
        // Have written all non-quiet samples plus trail padding.
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA %i: go quiet\n", alsa_tick);
#endif
        goto flush_buffer;  // let ALSA go idle
    }
   
    if( (alsa_state == SND_PCM_STATE_XRUN) || (alsa_state == SND_PCM_STATE_SETUP) )
    {
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA %i: prepare state=%s\n", alsa_tick,  snd_pcm_state_name( alsa_state ) );
#endif
        err = snd_pcm_prepare( alsa_handle );
        if( err < 0 )  goto handle_err;
    }

#ifdef DEBUG_ALSAxx
    alsa_state = snd_pcm_state( alsa_handle );
    if( alsa_state == SND_PCM_STATE_RUNNING || alsa_state == SND_PCM_STATE_PREPARED )
        GenPrintf(EMSG_debug, "ALSA %i: WRITE state=%s avail=%i\n", alsa_tick, snd_pcm_state_name( alsa_state ), (int)avail );
    else
        GenPrintf(EMSG_debug, "ALSA %i: WRITE state=%s\n", alsa_tick, snd_pcm_state_name( alsa_state ) );
#endif

    // Initial write of 1433 samples.
    // Update write of 360 to 900 samples per loop using doom2.
#if 0
    int avail_samples = avail;
    if( avail_samples > mixer_samples )  avail_samples = mixer_samples;
    printf( "A: wr %i\n", avail_samples );
#endif
   
    // Write frames from buffer to PCM device.
    // Returns the number of frames actually written.
    snd_pcm_uframes_t frame_cnt = mixer_samples;
    cnt = snd_pcm_writei( alsa_handle, mixer_out, frame_cnt ); // TO ALSA
    // The buffer must be left unchanged for a while.
    // Altering it puts noise in the output.
    if( cnt == -EPIPE )
    {
        // Broken Pipe
#ifdef DEBUG_ALSA
        // this print makes an audible sound
        alsa_state = snd_pcm_state( alsa_handle );
        GenPrintf(EMSG_debug,"ALSA %i: -EPIPE (underrun), state=%s\n", alsa_tick, snd_pcm_state_name( alsa_state ) );
#endif
        // RECOVER
        err = snd_pcm_recover( alsa_handle, cnt, 0 );  // cnt has previous err
#ifdef DEBUG_ALSA
        if( err < 0 )
            GenPrintf(EMSG_debug,"ALSA %i: snd_pcm_recover failed =%s\n", alsa_tick, snd_strerror(err) );
#endif

        cnt = snd_pcm_writei( alsa_handle, mixer_out, frame_cnt ); // TO ALSA
    }

    if( cnt < 0 )  goto handle_cnt_err;
    if( cnt < mixer_samples )
    {
        // Does not happen very often.
        mixer_samples -= cnt;
        mixer_out += SAMPLES_TO_BYTES(cnt);
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug,"ALSA full  %i:  write cnt=%i, mixer_samples %i\n", alsa_tick, cnt, mixer_samples );
#endif
        return;
    }

flush_buffer:
    mixer_samples = 0;
    mixer_out = mixer_mix;
    return;

handle_cnt_err:
    err = cnt; // cnt < 0
    goto handle_err;
   
handle_err:
#ifdef DEBUG_ALSA
    alsa_state = snd_pcm_state( alsa_handle );
#endif
    // Return err<0 is error.
    if( err == -EAGAIN )  // full
         return;

    if( err == -EPIPE )
    {
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug," ALSA  %i: ERR  -EPIPE hard underrun  %s\n", alsa_tick, snd_pcm_state_name( alsa_state ) );
#endif
        err = snd_pcm_prepare( alsa_handle );
        if( (err < 0) && verbose )
            GenPrintf(EMSG_warn, "ALSA underrun\n" );
        return;
    }
#if 0   
    else if( err == -ESTRPIPE )
    {
        // suspend
    }
#endif   
    else if( verbose )
        GenPrintf(EMSG_warn, "ALSA: %s\n", snd_strerror(err) );
#ifdef DEBUG_ALSA
        GenPrintf(EMSG_debug," ALSA  %i: ERR  err=%i, %s  %s\n", alsa_tick, err, snd_strerror(err), snd_pcm_state_name( alsa_state ) );
#endif

#endif
}


static void LXD_shutdown_ALSA( void )
{
    // Clean up and release of the ALSA connection.
    if( alsa_handle )
    {
        snd_pcm_close( alsa_handle );
        alsa_handle = NULL;
    }

    LXD_update_sound = LXD_update_sound_dummy;
}


#ifdef ALSA_CALLBACK
#if 0
// ALSA shutdown or disconnect.
static void  ALSA_shutdown_callback(   ??  )
{
    LXD_shutdown_ALSA();
    sound_callback_active = 0;
}
#endif
#endif


// The hw_params method

// Number of periods to size ALSA buffer
// ALSA only reports avail in multiples of period.
#define ALSA_period_samples   (SAMPLECOUNT/2)
#define ALSA_num_periods      6


#if 0
// ALSA devices
const char * alsa_device[] = {
    "default",
    "plug:surround40",  // 4 ch
    "plug:surround51"   // 6 ch
};
#endif

snd_pcm_format_t  alsa_format_table[] =
{
  SND_PCM_FORMAT_U8,  // AM_mono8
#ifdef __BIG_ENDIAN__
  SND_PCM_FORMAT_S16_BE,  // AM_stereo16
#else
  SND_PCM_FORMAT_S16_LE,  // AM_stereo16
#endif
// SND_PCM_FORMAT_GSM;  // for testing, should fail
};


// Return errcode on failure, 0 on success.
static byte LXD_init_ALSA( void )
{
    const char * msg = "";
    snd_pcm_hw_params_t * alsa_hw_params = NULL;  // on the stack
    snd_pcm_sw_params_t * alsa_sw_params = NULL;
    int ft;
    unsigned int sample_rate = SAMPLERATE;
#ifdef DEBUG_INIT_SOUND
    unsigned int period_bytes;
#endif
    snd_pcm_uframes_t  alsa_buffer_frames;
    int err = 0;
    byte am;

#ifdef DEV_OPT_ALSA
    // Dynamic load ALSA library.
    if( alsa_lp == NULL )
    {
        if( load_lib( "ALSA", symlist_ALSA, sizeof(symlist_ALSA)/sizeof(lib_sym_t), LIBALSA_NAME, /*OUT*/ &alsa_lp ) < 0 )
            goto report_done;
    }
#endif
   
#ifdef DEBUG_TIME
    msec_t0 = get_msec();
#endif
#ifdef DEBUG_INIT_SOUND
    snd_output_stdio_attach( &alsa_output, stdout, 0);
    if( alsa_handle )
    {
        GenPrintf(EMSG_debug, "ALSA: handle already in use\n" );
        LXD_shutdown_ALSA();
    }
#endif
   
    msg = "open";
    // SND_PCM_NONBLOCK makes calls that would block, return EAGAIN instead.
    // SND_PCM_ASYNC causes SIGIO to be emitted when a period has been completely processed by the soundcard.
    err = snd_pcm_open( &alsa_handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK );
    if(err < 0)
        goto report_err;

#ifdef DEBUG_INIT_SOUND
    GenPrintf(EMSG_debug, "ALSA: handle open\n" );
#endif
   
#if 0
    // Always returns same name as used in open, "default"
    const char * name = snd_pcm_name( alsa_handle );
    if( name )
        GenPrintf(EMSG_debug, "ALSA device: %s\n", name );
    else
        return 99;
#endif

    // HW PARAMS method
    msg = "get_params";
    snd_pcm_hw_params_alloca( &alsa_hw_params );  // on the stack, do not have to free
    err = snd_pcm_hw_params_any( alsa_handle, alsa_hw_params );
    if(err < 0)  goto report_err;  // broken device

#ifdef DEBUG_INIT_SOUND_xxx
    // only prints the device line
    GenPrintf(EMSG_debug, "ALSA: native settings\n" );
    snd_pcm_dump( alsa_handle, alsa_output );
#endif
   
    // To test ALSA
    //  snd_pcm_hw_params_can_<capability>
    //  snd_pcm_hw_params_is_<property>
    //  snd_pcm_hw_params_get_<parameter>
    //  snd_pcm_hw_params_test_<parameter>
   
    // ALSA period (like fragment) is the unit of buffer fill.
    // The period_size is the fragment size, the unit size of fill.
    // There needs to be at least 2 periods in the buffer, to prevent underrun.
    // If period_size is too small, the CPU will have to fill more often.
    // If period_size is too large, the latency will suffer.
    // frame_rate = sample_rate
    // buffer_size = (periods * period_size)
    // latency = (periods * period_size) / (frame_rate * bytes_per_frame)

    msg = "select audio mode";
    err = 0;
    for( am = AM_max; ; am -- )  // choices
    {
        if( am > AM_max )  // end of list  (unsigned wrap)
            goto report_err;

        if( select_audio_mode( am ) > AM_max )  continue;
        alsa_format = alsa_format_table[ am ];

#ifdef DEBUG_INIT_SOUND
        GenPrintf(EMSG_debug, "ALSA: testing format %i, channels=%i\n", am , audio_channels);
#endif
       
        ft = snd_pcm_hw_params_test_channels( alsa_handle, alsa_hw_params, audio_channels );
        if( ft < 0 )  continue;

        // Test preferred format, 0=pass, <0=err.
        ft = snd_pcm_hw_params_test_format( alsa_handle, alsa_hw_params, alsa_format );
        if( ft < 0 )  continue;
       
        break;
    }

    // latency = (period_bytes * periods) / (samplerate * bytes_per_frame)
    //         = SAMPLECOUNT * ALSA_num_periods / SAMPLERATE
#ifdef DEBUG_INIT_SOUND
    period_bytes = ALSA_period_samples * byte_per_sample;  // bytes
#endif
    alsa_buffer_frames = ALSA_period_samples * ALSA_num_periods;  // frames

#ifdef DEBUG_INIT_SOUND
    GenPrintf(EMSG_debug, "ALSA: samplerate=%i, period_bytes=%i, periods=%i, buffer_frames=%i\n",
      sample_rate, period_bytes, ALSA_num_periods, alsa_buffer_frames );
#endif
    limit_samples = ADJ_SAMPLECOUNT * 14/10;  // not related to period_samples
   
    msg = "set_access";
    err = snd_pcm_hw_params_set_access( alsa_handle, alsa_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED );
    if(err < 0)  goto report_err;

    msg = "format";
    err = snd_pcm_hw_params_set_format( alsa_handle, alsa_hw_params, alsa_format );
    if(err < 0)  goto report_err;

    msg = "channels";
    err = snd_pcm_hw_params_set_channels( alsa_handle, alsa_hw_params, audio_channels );
    if(err < 0)  goto report_err;

    msg = "rate";
    err = snd_pcm_hw_params_set_rate_near( alsa_handle, alsa_hw_params, &sample_rate, 0);
    if(err < 0)  goto report_err;

    msg = "fragments";
    // periods = fragments
    err = snd_pcm_hw_params_set_periods( alsa_handle, alsa_hw_params, ALSA_num_periods, 0);
    if(err < 0)  goto report_err;

    msg = "buffer size";
    // buffer size in frames
    err = snd_pcm_hw_params_set_buffer_size( alsa_handle, alsa_hw_params, alsa_buffer_frames);
    if(err < 0)
    {
        err = snd_pcm_hw_params_set_buffer_size_near( alsa_handle, alsa_hw_params, &alsa_buffer_frames);
        if(err < 0)  goto report_err;
    }

    // latency = (period_bytes * periods) / (samplerate * bytes_per_sample)
#ifdef DEBUG_INIT_SOUND
    GenPrintf(EMSG_debug, "ALSA: buffer frames=%i  latency=%i msec\n",
        (int)alsa_buffer_frames, (int)(alsa_buffer_frames * byte_per_sample * 1000)/(sample_rate * byte_per_sample));
#endif

    msg = "set hw param";
    err = snd_pcm_hw_params( alsa_handle, alsa_hw_params );
    if(err < 0)  goto report_err;

    // Software params
    snd_pcm_sw_params_alloca( &alsa_sw_params );
    err = snd_pcm_sw_params_current( alsa_handle, alsa_sw_params );
    if(err < 0)  goto report_err;

    msg = "set sw param";
    err = snd_pcm_sw_params_set_avail_min( alsa_handle, alsa_sw_params, 128 );
    if(err < 0)  goto report_err;

    err = snd_pcm_sw_params_set_start_threshold( alsa_handle, alsa_sw_params, 2 );
    if(err < 0)  goto report_err;
   
    err = snd_pcm_sw_params( alsa_handle, alsa_sw_params );
    if(err < 0)  goto report_err;
   
#ifdef ALSA_CALLBACK
#if 0   
    snd_pcm_sw_params_t * alsa_sw_params = NULL;
    snd_pcm_sw_params_alloca( &alsa_sw_params );
    snd_pcm_sw_params_current( alsa_handle, alsa_sw_params );
    // when does ALSA start playing, in frames
    snd_pcm_sw_params_set_start_threshold( alsa_handle, alsa_sw_params, 8 );
    // do not callback until have avail buffer space, in frames
    snd_pcm_sw_params_set_avail_min( alsa_handle, alsa_sw_params, SAMPLECOUNT );
    // write the sw params
    snd_pcm_sw_params( alsa_handle, alsa_sw_params );
#endif

    snd_async_add_pcm_handler( &alsa_pcm_callback, alsa_handle, ALSA_callback, NULL );
#ifdef DEBUG_INIT_SOUND
    GenPrintf(EMSG_debug, "ALSA: added callback\n" );
#endif
   
    snd_pcm_start( alsa_handle );
    sound_callback_active = 1;
#endif

    GenPrintf(EMSG_info, " configured %d bit audio device\n", audio_format_bits );
#ifdef DEBUG_INIT_SOUND
    snd_pcm_dump( alsa_handle, alsa_output );
#endif


    LXD_update_sound = LXD_update_sound_ALSA;
//    port_volume = 100;  // exceeds max easily
    port_volume = 70;

#ifdef DEBUG_ALSA
    snd_pcm_state_t  alsa_state;
    alsa_state = snd_pcm_state( alsa_handle );
    GenPrintf(EMSG_debug, "ALSA %i INIT: state=%s\n", alsa_tick, snd_pcm_state_name( alsa_state ) );
#endif

    // ALSA inits to state PREPARED.
    return 0;

report_err:
    if( err )
        GenPrintf(EMSG_error, "ALSA %s err: %s\n", msg, snd_strerror(err));
    else
        GenPrintf(EMSG_error, "ALSA %s : Failed\n", msg );
    goto report_done;

report_done:
    if( alsa_handle )
        LXD_shutdown_ALSA();
    return 4;
}

#endif // End ALSA


//============ JACK SECTION

#ifdef DEV_JACK
// Jack sound Interface
#include <jack/jack.h>

#define  JACK_CALLBACK

#define  JACK_TRAIL_PAD  1024

static void LXD_shutdown_JACK( void );

#ifdef DEV_OPT_JACK
#define LIBJACK_NAME   "libjack.so"
void * jack_lp = NULL;


// JACK function ptrs
static jack_client_t * (*DL_jack_client_open)(const char *, jack_options_t, jack_status_t *, ...);
#define jack_client_open  (*DL_jack_client_open)

static jack_port_t * (*DL_jack_port_register) (jack_client_t *, const char *, const char *, unsigned long, unsigned long );
#define jack_port_register  (*DL_jack_port_register)

static const char * (*DL_jack_port_name)(const jack_port_t *) JACK_OPTIONAL_WEAK_EXPORT;
#define jack_port_name  (*DL_jack_port_name)

static void * (*DL_jack_port_get_buffer)(jack_port_t *, jack_nframes_t);
#define jack_port_get_buffer  (*DL_jack_port_get_buffer)

static const char ** (*DL_jack_get_ports) (jack_client_t *, const char *, const char *, unsigned long );
#define jack_get_ports  (*DL_jack_get_ports)

static int (*DL_jack_connect)(jack_client_t *, const char *, const char *);
#define jack_connect  (*DL_jack_connect)

static int (*DL_jack_client_close)(jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;
#define jack_client_close  (*DL_jack_client_close)

#ifdef JACK_CALLBACK
static int (*DL_jack_set_process_callback)(jack_client_t *, JackProcessCallback, void *);
#define jack_set_process_callback  (*DL_jack_set_process_callback)

static void (*DL_jack_on_shutdown)(jack_client_t *, JackShutdownCallback, void *);
#define jack_on_shutdown  (*DL_jack_on_shutdown)

static int (*DL_jack_activate)(jack_client_t *);
#define jack_activate  (*DL_jack_activate)
#endif

const lib_sym_t  symlist_JACK[] =
{
  { (void*)& DL_jack_client_open, "jack_client_open" },
  { (void*)& DL_jack_port_register, "jack_port_register" },
  { (void*)& DL_jack_port_name, "jack_port_name" },
  { (void*)& DL_jack_port_get_buffer, "jack_port_get_buffer" },
  { (void*)& DL_jack_get_ports, "jack_get_ports" },
  { (void*)& DL_jack_connect, "jack_connect" },
  { (void*)& DL_jack_client_close, "jack_client_close" },
#ifdef JACK_CALLBACK
  { (void*)& DL_jack_set_process_callback, "jack_set_process_callback" },
  { (void*)& DL_jack_on_shutdown, "jack_on_shutdown" },
  { (void*)& DL_jack_activate, "jack_activate" },
#endif
};

#endif

static jack_client_t * jack_client = NULL;
static jack_port_t   * our_output_port = NULL;   // register ourselves as an output port
extern byte  sound_callback_active;


#ifdef JACK_CALLBACK
// The callback from JACK to get more frames.
// This must be treated as an interrupt routine, do not do anything else.
//  num_frames: the buffer space available, in frames 
int jack_callback( jack_nframes_t num_frames, void * arg )
{
    jack_default_audio_sample_t *  j_out;
    j_out = jack_port_get_buffer( our_output_port, num_frames );
//    unsigned int seg_size = sizeof(jack_default_audio_sample_t) * num_frames;
   
    sfx_mixer( num_frames );
    if( num_frames > mixer_samples )  num_frames = mixer_samples;

    // with conversion
    while( num_frames -- )
    {
        // stereo, int16_t to float conversion
        jack_default_audio_sample_t s1 = *(int16_t*)mixer_out;
        mixer_out += sizeof(int16_t);
        *(j_out++) = s1;
        jack_default_audio_sample_t s2 = *(int16_t*)mixer_out;
        mixer_out += sizeof(int16_t);
        *(j_out++) = s2;
    }
    mixer_samples -= num_frames;
    return 0;
}

// Jack shutdown or disconnect.
void  jack_shutdown_callback( void * arg )
{
    LXD_shutdown_JACK();
}
#endif


static void LXD_shutdown_JACK( void )
{
    // Clean up and release of the JACK connection.
    if( jack_client )
    {
        jack_client_close( jack_client );
        jack_client = NULL;
    }
     
    LXD_update_sound = LXD_update_sound_dummy;
    sound_callback_active = 0;
}

// Return errcode on failure, 0 on success.
static byte LXD_init_JACK( void )
{
    const char * msg = "";
#ifdef JACK_CALLBACK
    const char ** ports;
#endif
    jack_status_t  jack_status;

    int err = 0;

#ifdef DEV_OPT_JACK
    // Dynamic load JACK library.
    if( jack_lp == NULL )
    {
        if( load_lib( "JACK", symlist_JACK, sizeof(symlist_JACK)/sizeof(lib_sym_t), LIBJACK_NAME, /*OUT*/ &jack_lp ) < 0 )
            goto report_err;  // err = 0
    }
#endif
   
#ifdef DEBUG_INIT_SOUND
    if( jack_client )
    {
        GenPrintf(EMSG_debug, "JACK: client already in use\n" );
        LXD_shutdown_JACK();
    }
#endif
   
    msg = "open";
    jack_client = jack_client_open( "DoomLegacyArcade", JackNullOption, &jack_status, NULL );  // [Arcade]
    if( jack_client == NULL )  goto report_msg;

#ifdef JACK_CALLBACK
    // Setup callback.
    jack_set_process_callback( jack_client, jack_callback, 0);
    jack_on_shutdown( jack_client, jack_shutdown_callback, 0);
    sound_callback_active = 1;
#endif
   
    msg = "get port";
    // Our output is input to Jack.
    // DEFAULT_AUDIO_TYPE is stereo
    our_output_port = jack_port_register( jack_client, "sfx", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
    if( our_output_port == NULL )  goto report_msg;
   
#ifdef JACK_CALLBACK
    // Start callback.
    // Mut activate first, because only an active client can have ports.    
    msg = "activate client";
    err = jack_activate( jack_client );
    if( err )  goto report_err;
   
    // Connect the ports.
    // Get a Jack backend input port (like ALSA).
    msg = "connect ports";
    ports = jack_get_ports( jack_client, NULL, NULL, JackPortIsInput );
//			    JackPortIsPhysical | JackPortIsInput );

    if( ports == NULL )  goto report_msg;

    // Use the first port.
    // The port names should be listed in the menu, and the
    // user should select one.
    err = jack_connect( jack_client, jack_port_name( our_output_port ), ports[0] );
    if( err )  goto report_err;
   
    free( ports );
#endif

    LXD_update_sound = NULL;
    port_volume = 120;
    return 0;

report_msg:
    GenPrintf(EMSG_error, "Jack %s : Failed\n", msg);
    err = 0;

report_err:
    if( jack_status & JackServerFailed )
    {
        GenPrintf(EMSG_error, "Jack: Unable to connect to server\n" );
    }
    else if( err )
    {
        GenPrintf(EMSG_error, "Jack %s err: %i\n", msg, err);
    }
    return 4;
}

#endif // End Jack


//============ PULSE SECTION

#ifdef DEV_PULSE
// PulseAudio sound Interface
// Documentation on PulseAudio is mostly basic.
// see SDL1 source code, Sam Latinga.
// see apulse source code (too complicated).
#include <pulse/mainloop.h>
#include <pulse/stream.h>
#include <pulse/error.h>

// #define DEBUG_PULSE

#ifdef DEBUG_PULSE
static  unsigned int pulse_cnt = 0;  // event sequence numbering for debug
static  byte  pulse_err_cnt = 0;
static void LXD_shutdown_PULSE( void );
#endif

// quiet time after sound
#define PULSE_TRAIL_PAD  4096
//#define PULSE_TRAIL_PAD  512

// Control over channel mapping.
#define CHANNEL_MAP


#ifdef DEV_OPT_PULSE
// Dynamic load pulseaudio library, so it does not have to be present.
#define LIBPULSE_NAME   "libpulse.so.0"
void *  pulse_lp = NULL;

// Pulse library function ptrs.
static pa_mainloop * (*DL_pa_mainloop_new)(void);
#define pa_mainloop_new  (*DL_pa_mainloop_new)

static void (*DL_pa_mainloop_free)(pa_mainloop*);
#define pa_mainloop_free  (*DL_pa_mainloop_free)

static pa_mainloop_api * (*DL_pa_mainloop_get_api)(pa_mainloop*);
#define pa_mainloop_get_api  (*DL_pa_mainloop_get_api)

static int (*DL_pa_mainloop_iterate)(pa_mainloop *, int, int *);
#define pa_mainloop_iterate  (*DL_pa_mainloop_iterate)

static pa_context * (*DL_pa_context_new)(pa_mainloop_api *, const char *);
#define pa_context_new  (*DL_pa_context_new)

static int (*DL_pa_context_connect)(pa_context *, const char *, pa_context_flags_t, const pa_spawn_api *);
#define pa_context_connect  (*DL_pa_context_connect)

static void (*DL_pa_context_disconnect)(pa_context *);
#define pa_context_disconnect  (*DL_pa_context_disconnect)

static void (*DL_pa_context_unref)(pa_context *);
#define pa_context_unref  (*DL_pa_context_unref)

static pa_context_state_t (*DL_pa_context_get_state)(pa_context *);
#define pa_context_get_state  (*DL_pa_context_get_state)

static int (*DL_pa_context_errno)(pa_context *);
#define pa_context_errno  (*DL_pa_context_errno)

#ifdef CHANNEL_MAP
static pa_channel_map* (*DL_pa_channel_map_init_auto)(pa_channel_map *m, unsigned channels, pa_channel_map_def_t def);
#define pa_channel_map_init_auto  (*DL_pa_channel_map_init_auto)
#endif

static pa_stream * (*DL_pa_stream_new)( pa_context *c, const char *, const pa_sample_spec *, const pa_channel_map * );
#define pa_stream_new  (*DL_pa_stream_new)

static int (*DL_pa_stream_connect_playback)( pa_stream *, const char *, const pa_buffer_attr *, pa_stream_flags_t, const pa_cvolume *, pa_stream * );
#define pa_stream_connect_playback  (*DL_pa_stream_connect_playback)

static int (*DL_pa_stream_disconnect)(pa_stream *s);
#define pa_stream_disconnect  (*DL_pa_stream_disconnect)

static void (*DL_pa_stream_unref)(pa_stream *);
#define pa_stream_unref  (*DL_pa_stream_unref)

static pa_stream_state_t (*DL_pa_stream_get_state)(pa_stream *p);
#define pa_stream_get_state  (*DL_pa_stream_get_state)

static size_t (*DL_pa_stream_writable_size)(pa_stream *);
#define pa_stream_writable_size  (*DL_pa_stream_writable_size)

static int (*DL_pa_stream_write)( pa_stream *, const void *, size_t, pa_free_cb_t, int64_t, pa_seek_mode_t );
#define pa_stream_write  (*DL_pa_stream_write)

static pa_operation* (*DL_pa_stream_trigger)(pa_stream *, pa_stream_success_cb_t, void *);
#define pa_stream_trigger  (*DL_pa_stream_trigger)

static const char* (*DL_pa_strerror)(int);
#define pa_strerror  (*DL_pa_strerror)

const lib_sym_t  symlist_PULSE[] =
{
  { (void*)& DL_pa_mainloop_new, "pa_mainloop_new" },
  { (void*)& DL_pa_mainloop_free, "pa_mainloop_free" },
  { (void*)& DL_pa_mainloop_get_api, "pa_mainloop_get_api" },
  { (void*)& DL_pa_mainloop_iterate, "pa_mainloop_iterate" },
  { (void*)& DL_pa_context_new, "pa_context_new" },
  { (void*)& DL_pa_context_connect, "pa_context_connect" },
  { (void*)& DL_pa_context_disconnect, "pa_context_disconnect" },
  { (void*)& DL_pa_context_unref, "pa_context_unref" },
  { (void*)& DL_pa_context_get_state, "pa_context_get_state" },
  { (void*)& DL_pa_context_errno, "pa_context_errno" },
#ifdef CHANNEL_MAP   
  { (void*)& DL_pa_channel_map_init_auto, "pa_channel_map_init_auto" },
#endif
  { (void*)& DL_pa_stream_new, "pa_stream_new" },
  { (void*)& DL_pa_stream_connect_playback, "pa_stream_connect_playback" },
  { (void*)& DL_pa_stream_disconnect, "pa_stream_disconnect" },
  { (void*)& DL_pa_stream_unref, "pa_stream_unref" },
  { (void*)& DL_pa_stream_get_state, "pa_stream_get_state" },
  { (void*)& DL_pa_stream_writable_size, "pa_stream_writable_size" },
  { (void*)& DL_pa_stream_write, "pa_stream_write" },
  { (void*)& DL_pa_stream_trigger, "pa_stream_trigger" },
  { (void*)& DL_pa_strerror, "pa_strerror" },
};

#endif


static  pa_mainloop * pa_ml = NULL;
static  pa_context  * pa_ctxt = NULL;
static  pa_stream   * pa_strm = NULL;



static void LXD_update_sound_PULSE( void )
{
    int i, byte_cnt, err;

    if( ! pa_strm )
    {
        GenPrintf(EMSG_warn, "PULSE: submit sound without handle\n" );
        goto flush_buffer;
    }

#ifdef DEBUG_PULSE
    pulse_cnt ++;  // event sequence number
#endif

    if( (quiet_samples - mixer_samples) > PULSE_TRAIL_PAD )
    {
        sound_init = 6;  // pulse idle
        goto flush_buffer;  // Do nothing until a new sound
    }

    // Iterate first.  Usually 1, sometimes more.
    // PulseAudio will not advance without servicing mainloop_iterate.
    for(i=0; i<8; i++)
    {
        err = pa_mainloop_iterate(pa_ml, 0, NULL);  // no blocking
        if( err == 0 )  break;
        if( err < 0 )
        {
#ifdef DEBUG_PULSE
            printf( "PULSE: iterate err\n" );
#endif
            return;  // iterate not ready
        }
    }
   
    if( pa_context_get_state(pa_ctxt) != PA_CONTEXT_READY )
    {
#ifdef DEBUG_PULSE
        printf( "PULSE: context not ready\n" );
#endif
        goto flush_buffer;  // not ready
    }

    if( pa_stream_get_state(pa_strm) != PA_STREAM_READY )
    {
#ifdef DEBUG_PULSE
        printf( "PULSE: stream not ready\n" );
#endif
        goto flush_buffer;  // not ready
    }

    // Avoid very small buffer updates.
    // This also returns pending errors as neg values.
    size_t  avail = pa_stream_writable_size( pa_strm );  // bytes
    if( avail < 0 )
        return;

    int avail_samples = BYTES_TO_SAMPLES(avail);  // write only whole samples
    if( avail_samples < 128 )
        return;

    sfx_mixer( avail_samples );

    // Write frames from buffer to PCM device.
    // Returns the number of frames actually written.
    if( avail_samples > mixer_samples )
        avail_samples = mixer_samples;
    // Write size on doom2 is around 380 to 480 samples per loop.
    mixer_samples -= avail_samples;
    byte_cnt = SAMPLES_TO_BYTES(avail_samples);
    err = pa_stream_write( pa_strm, mixer_out, byte_cnt, NULL, 0, PA_SEEK_RELATIVE ); // TO PULSE
    if( err )
        goto handle_err;

    mixer_out += byte_cnt;
   
#ifdef DEBUG_PULSE
    printf( "P: wr %i  byte cnt=%i\n", avail_samples, byte_cnt );
    if( mixer_out < mixbuffer )  GenPrintf(EMSG_debug, "PULSE 2  %i: ERROR  mixer_out < mixbuffer\n", pulse_cnt );
    if( mixer_out > MIXBUFFER_END )  GenPrintf(EMSG_debug, "PULSE 2  %i: ERROR  mixer_out > mixbuffer\n", pulse_cnt );
    pulse_err_cnt = 0;
#endif

#if 0
    if( sound_init < 7 )
    {
        pa_stream_trigger( pa_strm, NULL, NULL ); // start playback immediately
        sound_init = 7;
    }
#endif

    // Iterate after write.  Usually 1, sometimes upto 4.
    // This is necessary when not using SNDSERV.
    for(i=0; i<8; i++)
    {
        err = pa_mainloop_iterate(pa_ml, 0, NULL);  // no blocking
        if( err <= 0 )  break;
    }
    return;

flush_buffer:
    mixer_out = mixer_mix;
    mixer_samples = 0;
    return;

handle_err:
#ifdef DEBUG_PULSE
    GenPrintf(EMSG_debug," PULSE  %i: err=%i, %s\n", pulse_cnt, err, pa_strerror(err) );
    if( pulse_err_cnt++ > 5 )  LXD_shutdown_PULSE();
#endif
    return;
}


static void LXD_shutdown_PULSE( void )
{
#ifdef DEBUG_PULSE
    printf( "PulseAudio: Shutdown\n" );
#endif
    // Clean up and release of PulseAudio.
    if( pa_strm )
    {
        pa_stream_disconnect( pa_strm );
        pa_stream_unref( pa_strm );
        pa_strm = NULL;
    }
    if( pa_ctxt )
    {
        pa_context_disconnect( pa_ctxt );
        pa_context_unref( pa_ctxt );
        pa_ctxt = NULL;
    }
    if( pa_ml )
    {
        pa_mainloop_free( pa_ml );
        pa_ml = NULL;
    }
    LXD_update_sound = LXD_update_sound_dummy;
}


byte  pulse_format_table[] =
{
  PA_SAMPLE_U8,     // AM_mono8
#ifdef __BIG_ENDIAN__
  PA_SAMPLE_S16BE,  // AM_stereo16
#else
  PA_SAMPLE_S16LE,  // AM_stereo16
#endif
#ifdef QUAD_AUDIO
#ifdef __BIG_ENDIAN__
  PA_SAMPLE_S16BE,  // AM_stereo16
#else
  PA_SAMPLE_S16LE,  // AM_stereo16
#endif
#endif
// PA_SAMPLE_S24_32BE;  // for testing, should fail
};


// Return errcode on failure, 0 on success.
static byte LXD_init_PULSE( void )
{
    const char * msg = "";
#ifdef DEBUG_PULSE
    const char * errmsg = "";
#endif
    pa_mainloop_api * pa_mlapi = NULL;
    pa_sample_spec paspec;
#ifdef CHANNEL_MAP
    pa_channel_map pachmap;
#endif
    pa_buffer_attr paattr;
    pa_stream_flags_t paflags;
    int pa_state;
    int err = 0;
    int i;
    byte am;

#ifdef DEV_OPT_PULSE
    // Dynamic load pulseaudio library.
    if( pulse_lp == NULL )
    {
        if( load_lib( "PulseAudio", symlist_PULSE, sizeof(symlist_PULSE)/sizeof(lib_sym_t), LIBPULSE_NAME, /*OUT*/ &pulse_lp ) < 0 )
            goto report_done;
    }
#endif
   
    msg = "open mainloop";
    pa_ml = pa_mainloop_new();
    if( pa_ml == NULL )
        goto report_msg;

    pa_mlapi = pa_mainloop_get_api( pa_ml );
    if( pa_mlapi == NULL )
        goto report_msg;
   
    msg = "open context";
    // with caption "DoomLegacy"
    pa_ctxt = pa_context_new( pa_mlapi, "DoomLegacyArcade" );  // [Arcade]
    if( pa_ctxt == NULL )
        goto report_msg;
   
    msg = "connect context";
    // connect to the server
    err = pa_context_connect( pa_ctxt, NULL, PA_CONTEXT_NOFAIL, NULL );
    if( err < 0 )
        goto context_err;
   
    // Wait for context connect.
    msg = "context connect iterate";
    for(i=0; ;i++)
    {
        if( i>1024 )  goto report_msg;

        // Iterate does prepare, poll, dispatch.
        // This takes 13 iterations to process connect.
        // Block = 1, blocks when nothing in queue, but allows timeout.
        err = pa_mainloop_iterate( pa_ml, 1, NULL );
        if( err < 0 )
            goto report_err;
       
        pa_state = pa_context_get_state( pa_ctxt );
        if( pa_state == PA_CONTEXT_READY )  break;
        if( pa_state >= PA_CONTEXT_FAILED )
            goto context_err;
    }

#ifdef DEBUG_PULSE
    printf( "PulseAudio: connected at i=%i iterations\n", i );
#endif
   
    // Setup parameters
    msg = "select audio mode";

    for( am = AM_max; ; am -- )  // choices
    {
        // End of list test
        if( am > AM_max )  // end of list  (unsigned wrap)
            goto report_msg;

        if( select_audio_mode( am ) > AM_max )  continue;
#ifdef DEBUG_INIT_SOUND
        GenPrintf(EMSG_debug, "PULSE: testing format %i, channels=%i\n", am , audio_channels);
#endif
#ifdef CHANNEL_MAP
        pa_channel_map_init_auto( &pachmap, audio_channels, PA_CHANNEL_MAP_ALSA ); // X11 only
#endif
        paspec.rate = SAMPLERATE;
        paspec.channels = audio_channels;
        paspec.format = pulse_format_table[ am ];
#ifdef CHANNEL_MAP
        pa_strm = pa_stream_new( pa_ctxt, "DoomLegacyArcade", &paspec, &pachmap ); // specific channel map  // [Arcade]
#else
        pa_strm = pa_stream_new( pa_ctxt, "DoomLegacyArcade", &paspec, NULL ); // default channel map  // [Arcade]
#endif
        if( pa_strm )  break;
    }

#ifdef DEBUG_PULSE
    printf( "PulseAudio: stream connected am=%i\n", am );
    pulse_err_cnt = 0;
#endif

    // This is dependent upon audio mode.
    paflags = PA_STREAM_ADJUST_LATENCY;
    // attributes for pulse buffer
    // Write size on doom2 is around 380 to 480 samples per loop.
#ifdef SNDSERV   
    // For latency, tlength and minreq are relevant
    paattr.maxlength = MIXBUFFER_BYTESIZE * 2;  // max buffer
    // Making this larger makes it wait longer to play
    // Default will try to fill entire buffer, 2 sec latency or worse.
    paattr.tlength = SAMPLES_TO_BYTES( ADJ_SAMPLECOUNT );  // target fill, when it will request more
    // Default will fill to tlength before playing.
    paattr.minreq = SAMPLES_TO_BYTES( ADJ_SAMPLECOUNT/4 ); // min request for more
//    paattr.minreq = -1; // min request for more
#else
    paattr.maxlength = MIXBUFFER_BYTESIZE * 2;  // max buffer
    // Making this larger makes it wait longer to play
    // Default will try to fill entire buffer, 2 sec latency or worse.
    paattr.tlength = SAMPLES_TO_BYTES( ADJ_SAMPLECOUNT * 16 / 10 );  // target fill, when it will request more
    // Default will fill to tlength before playing.
    paattr.minreq = SAMPLES_TO_BYTES( ADJ_SAMPLECOUNT * 3 / 10 ); // min request for more
#endif
    paattr.prebuf = 16;  // buffer amount needed to start playing
//    paattr.fragsize = SAMPLES_TO_BYTES( ADJ_SAMPLECOUNT/4 );  // recording max transfer size, keep low to help latency

    msg = "stream connect playback";
    err = pa_stream_connect_playback( pa_strm, NULL, &paattr, paflags, NULL, NULL );
    if( err < 0 )
        goto report_err;
   
    // Wait for stream connect.
    msg = "stream connect iterate";
    for(i=0; ;i++)
    {
        if( i>32000 )  goto report_msg;

        // Block = 1, allows timeout.
        // This takes 700 to 6000 iterations to connect stream.
        err = pa_mainloop_iterate( pa_ml, 0, NULL );
        if( err < 0 )
            goto report_err;
       
        pa_state = pa_stream_get_state( pa_strm );
        if( pa_state == PA_STREAM_READY )  break;
        if( pa_state != PA_STREAM_CREATING )
        {
            msg = (pa_state == PA_STREAM_FAILED)? "stream failed" : "stream unk";
            goto report_msg;
        }
    }
#ifdef DEBUG_PULSE
    printf( "PulseAudio: stream ready at i=%i iterations\n", i );
    pulse_err_cnt = 0;
#endif

    LXD_update_sound = LXD_update_sound_PULSE;
    port_volume = 128;
    return 0;

context_err:
    err = pa_context_errno( pa_ctxt );
    goto report_err;

report_err:
    GenPrintf(EMSG_error, "PulseAudio %s err: %s\n", msg, pa_strerror(err));
    goto report_done;
   
report_msg:
    GenPrintf(EMSG_error, "PulseAudio %s : Failed\n", msg);
    goto report_done;

report_done:
#ifdef DEBUG_PULSE
    printf( "pa context: msg=%s  : pa errmsgerr= %s \n",  msg, errmsg );
#endif
    LXD_shutdown_PULSE();
    return 4;
}

#endif // End PulseAudio


//============ INIT and SHUTDOWN SECTION

// Device Shutdown.
static void LXD_Shutdown_sound_device( void )
{
#ifdef SOUND_DEVICE_OPTION   
    switch( sound_device )
    {
#ifdef DEV_OSS
     case SD_OSS:
        LXD_shutdown_OSS();
        break;
#endif   
#ifdef DEV_ESD
     case SD_ESD:
        LXD_shutdown_ESD();
        break;
#endif   
#ifdef DEV_ALSA
     case SD_ALSA:
        LXD_shutdown_ALSA();
        break;
#endif
#ifdef DEV_JACK
     case SD_JACK:
        LXD_shutdown_JACK();
        break;
#endif
#ifdef DEV_PULSE
     case SD_PULSE:
        LXD_shutdown_PULSE();
        break;
#endif
     default:
        break;
    }
#else
    // Static sound device.
#ifdef DEV_OSS
    LXD_shutdown_OSS();
#endif   
#ifdef DEV_ESD
    LXD_shutdown_ESD();
#endif   
#ifdef DEV_ALSA
    LXD_shutdown_ALSA();
#endif
#ifdef DEV_JACK
    LXD_shutdown_JACK();
#endif
#ifdef DEV_PULSE
    LXD_shutdown_PULSE();
#endif
#endif
    sound_device = 0;
    sound_init = 1;
}

// Test and Init device.
//  cds : device selection
// return errcode, 0=success
static
byte  LXD_Init_sound_device( byte cds )
{
    byte tst = 99; // invalid device

    if( sound_device )
        LXD_Shutdown_sound_device();  // sound_device off

    // defaults
#ifdef MIXOUT_PROTECT
    mixer_protect = MIXBUFFER_END;
#endif
    limit_samples = ADJ_SAMPLECOUNT;
    want_more = 0;  // request more by individual driver

   
#ifdef DEBUG_MIXER
    memset( &mixbuffer[MIXBUFFER_BYTESIZE], MIXBUFFER_CANARY, 63 );  // DEBUG
#endif

    sound_device = cds; // The device selection.
   
#ifdef SOUND_DEVICE_OPTION
    switch( cds )
    {
#ifdef DEV_OSS
     case SD_OSS:
        tst = LXD_init_OSS();
        break;
#endif   
#ifdef DEV_ESD
     case SD_ESD:
        tst = LXD_init_ESD();
        break;
#endif   
#ifdef DEV_ALSA
     case SD_ALSA:
        tst = LXD_init_ALSA();
        break;
#endif
#ifdef DEV_JACK
     case SD_JACK:
        tst = LXD_init_JACK();
        break;
#endif
#ifdef DEV_PULSE
     case SD_PULSE:
        tst = LXD_init_PULSE();
        break;
#endif
     default:
        break;
    }
#else
    // Static sound device
#ifdef DEV_OSS
    tst = LXD_init_OSS();
#endif   
#ifdef DEV_ESD
    tst = LXD_init_ESD();
#endif   
#ifdef DEV_ALSA
    tst = LXD_init_ALSA();
#endif
#ifdef DEV_JACK
    tst = LXD_init_JACK();
#endif
#ifdef DEV_PULSE
    tst = LXD_init_PULSE();
#endif
#endif

    if( tst > 0 )  // failure
    {
        sound_device = 0; // cancel
        sound_init = 1;
        return tst;
    }
   
    sound_init = 4;
     
    // Protect against buffer overrun.
    if( limit_samples > BYTES_TO_SAMPLES( MIXBUFFER_BYTESIZE ) )
        limit_samples = BYTES_TO_SAMPLES( MIXBUFFER_BYTESIZE );

    // Finish initialization.
    setup_mixer_tables();  // dependent upon audio_mode
   
    // Make port_volume effective.
    // mix_sfxvolume is global in main program and in server.
    LXD_SetSfxVolume( mix_sfxvolume );

#ifdef SNDINTR
//    if( sound_device && (sound_init > 1) )
        LX_Init_interrupts();
#endif
   
    return 0;
}




#ifdef SOUND_DEVICE_OPTION

// OSS is old, not available on many systems, may be through ALSA.
// ESD was a popular interface, but is high latency.
// ALSA is standard, good quality.
// PULSE is new, available, but high latency.
// JACK is a low latency connector to other drivers.
static
byte search_devices_dev[4][6] = {
  { SD_ALSA, SD_OSS, 0, 0, SD_PULSE, 0  },  // search 0, default
  { SD_OSS, SD_ALSA, 0, 0, SD_PULSE, SD_JACK },  // search 1
  { SD_ALSA, SD_OSS, 0, 0, SD_PULSE, SD_JACK },  // search 2
  { SD_ALSA, 0, 0, SD_PULSE, SD_OSS, SD_JACK },  // search 3
};

// Indexed by sound_dev_e - SD_OSS
const char * sound_device_str[] = {
   "OSS", "ESD", "ALSA", "PulseAudio", "Jack", "DEV6", "DEV7", "DEV8", "DEV9"
};


//  selection : from cv_snd_opt, sound_dev_e
void LX_select_sound_device( byte selection )
{
    // Search for working device.
    byte * search_list = NULL;  // no search
    byte i;

    if( selection <= SD_S3 )
        search_list = & search_devices_dev[ selection ][0];  // search

    for(i=0; i<6; i++)
    {
        if( search_list )       
            selection = *(search_list++);  // search next

        // return errcode from test init
        if( LXD_Init_sound_device( selection ) == 0 )
            goto select_device;

        // Search
        if( ! search_list )  break;  // No search
    }
    GenPrintf(EMSG_info, "Sound Device: device not found\n" );
    return;

select_device:
    GenPrintf(EMSG_info, "Sound Device %s\n", sound_device_str[ selection - SD_OSS ] );
    return;
}

// Called by cv_snd_opt changes.
//  snd_opt : from cv_snd_opt, 99= none
void LXD_SetSoundOption( byte snd_opt )
{
    if( sound_device )
    {
        if( sound_device == snd_opt )
            return;  // this device is already active

        // Change of sound_device is expected.
        LXD_Shutdown_sound_device();
    }
   
    if( snd_opt > 9 )
        return; // Do not connect new device, yet.     

    LX_select_sound_device( snd_opt );
}
// SOUND_DEVICE_OPTION
#endif



//============ INTERFACE

// Caller has already set mix_sfxvolume, and with range checks.
// mix_sfxvolume is global in main program and in server.
//   volume : 0..31
void LXD_SetSfxVolume( int volume )
{
    // port_volume 0..255, with 128 as normal
    // mix_volume7 0..127
    mix_volume7 = (volume * port_volume) >> 5;

#ifdef DEBUG_VOLUME
    if( volume || mix_volume7 )
        GenPrintf(EMSG_debug,"SetSfxVolume  volume=%i  mix_volume7=%i\n", volume, mix_volume7 );
#endif

    if( mix_volume7 > 127 )   mix_volume7 = 127;

    // leftvol, rightvol : range 0..255
    // mix_volume7 : range 0..127
    // vol_lookup[0..127]

    // Update existing channel volumes.
    register mix_channel_t * chp;
    for( chp = &mix_channel[0]; chp < &mix_channel[cv_num_channels]; chp++ )
    {
        if( chp->data_ptr )
        {
            unsigned int leftvol = chp->left_volume;
            unsigned int rightvol = chp->right_volume;
            // (255 * 127) / 255 = 127
//            chp->left_vol_tab = &vol_lookup[(leftvol * volume) / 63][0];
//            chp->right_vol_tab = &vol_lookup[(rightvol * volume) / 63][0];
            // (255 * 127) >> 8 = 126
            chp->left_vol_tab = &vol_lookup[(leftvol * mix_volume7) >> 8][0];
            chp->right_vol_tab = &vol_lookup[(rightvol * mix_volume7) >> 8][0];
        }
    }
}



// Adds a sound to the current active sounds.
//  vol : volume, 0..255
//  sep : separation, +/- 127, SURROUND_SEP special operation
//  pitch : 128=normal
#ifdef SNDSERV
// Main program determined handle.
void  LXD_StartSound_handle ( sfxid_t sfxid, int vol, int sep, int pitch, int priority, int handle, unsigned int sound_age )
#else
// Return a channel handle.
int  LXD_StartSound ( sfxid_t sfxid, int vol, int sep, int pitch, int priority, unsigned int sound_age )
#endif
{
    int oldest, slot;
    int leftvol, rightvol;
    mix_channel_t * chp, * chp2;
   
    if( sound_device == 0 )
#ifdef SNDSERV
        return;
#else
        return 0;
#endif

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    // [WDJ] Implemented using flags from Sfx tables.
    if (S_sfx[sfxid].flags & SFX_single)
    {
        // Loop all channels, check.
        for( chp2 = &mix_channel[0]; chp2 < &mix_channel[cv_num_channels]; chp2++ )
        {
            // Active, and using the same SFX?
            if (chp2->data_ptr && (chp2->id == sfxid))
            {
                if( S_sfx[sfxid].flags & SFX_id_fin )
#ifdef SNDSERV
                    return;  // already have one
#else
                    return chp2->handle;  // already have one
#endif
                // Kill, Reset.
                chp2->data_ptr = NULL;  // close existing channel
                break;
            }
        }
    }

    // Find inactive channel, or oldest channel.
    chp = &mix_channel[0];  // default
    oldest = INT_MAX;
    for( slot = 0; slot < cv_num_channels; slot++ )  // need slot for handle
    {
        chp2 = &mix_channel[slot];
        if( chp2->data_ptr == NULL )
        {
            chp = chp2;  // Inactive channel
            break;
        }
        // handles sound_age wrap, by considering only diff
        register unsigned int  agpr = sound_age - chp2->age_priority;
        if( agpr > oldest )   // older
        {
            chp = chp2;  // older channel
            oldest = agpr;
        }
    }

    byte * header = S_sfx[sfxid].data;
   
    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    chp->id = sfxid;
    // Set pointer to raw data.
    chp->data_ptr = (byte *) S_sfx[sfxid].data + 8;  // after header
//    chp->data_ptr = & ((byte*)S_sfx[sfxid].data)[8];  // after header
    // Set pointer to end of raw data.
// SDL corrects S_sfx[].length at sound load.
    chp->data_end = chp->data_ptr + S_sfx[sfxid].length - 8; // without header

    // Get samplerate from the sfx header, 16 bit, big endian
    chp->sample_rate = (header[3] << 8) + header[2];

    // Set stepping
    chp->step = steptable[pitch] * chp->sample_rate / DOOM_SAMPLERATE;
    // 16.16 fixed point
    chp->step_remainder = 0;
    // balanced between age and priority
    // low priority (higher value) increases age
    chp->age_priority = sound_age - priority;  // age at start

#ifdef DEBUG_SOUND
    GenPrintf(EMSG_debug, "Start Sound %i, len=%i, data_len=%i  sample_rate=%i  pitch=%i  step=%i.%0i\n",
        sfxid, S_sfx[sfxid].length - 8, chp->data_end - chp->data_ptr, chp->sample_rate, pitch, chp->step>>16, ((chp->step&0xFFFF)*10000)>>16 );
#endif
   
    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.

#ifdef SURROUND_SOUND
    chp->invert_right = 0;
    if( sep == SURROUND_SEP )
    {
        // Use a normal sound data for the left channel (with pan left)
        // and an inverted sound data for the right channel (with pan right)
        leftvol = rightvol = (vol * (224 * 224)) >> 16;  // slight reduction going through panning
        chp->invert_right = 1;  // invert right channel
    }
    else
#endif
    {
        // Separation, that is, orientation/stereo.
        // sep : +/- 127, <0 is left, >0 is right
        sep += 129;  // 129 +/- 127 ; ( 1 - 256 )
        leftvol = vol - ((vol * sep * sep) >> 16);
        sep = 258 - sep;  // 129 +/- 127
        rightvol = vol - ((vol * sep * sep) >> 16);
    }

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 255)
    {
        I_SoftError("rightvol out of bounds\n");
        rightvol = ( rightvol < 0 ) ? 0 : 255;
    }
    chp->right_volume = rightvol; // normalized 0..255

    if (leftvol < 0 || leftvol > 255)
    {
        I_SoftError("leftvol out of bounds\n");
        leftvol = ( leftvol < 0 ) ? 0 : 255;
    }
    chp->left_volume = leftvol;  // normalized 0..255

    // SDL, left_volume is 0..64 at this point
    // vol : range 0..255
    // leftvol, rightvol : range 0..255
    // mix_volume7 : range 0..127
    // vol_lookup[0..127]

    // At sep=0, full volume should index 0..63.
    // Get the proper lookup table for this volume level.
    // (255 * 127) / 255 = 127
    // (255 * 127) >> 8 = 126
    chp->left_vol_tab = &vol_lookup[(leftvol * mix_volume7) >> 8][0];
    chp->right_vol_tab = &vol_lookup[(rightvol * mix_volume7) >> 8][0];

#ifdef DEBUG_SOUND
    GenPrintf(EMSG_debug, "  mixer_vol=%i, snd vol=%i chp(%i,%i) lookup(%i,%i)\n",
        mix_volume7, vol, leftvol, rightvol, (leftvol * mix_volume7)>>8, (rightvol * mix_volume7)>>8 );
#endif
   
    active_channels |= 0x80;  // force recheck
    quiet_samples = 0;
   
    // Assign current handle number.
#ifdef SNDSERV
    chp->handle = handle;  // main program determined handle
#else
    // Preserved so sounds could be stopped (unused).
    slot = chp - &mix_channel[0];
    chp->handle = ((chp->handle + NUM_MIX_CHANNEL) & ~CHANNEL_NUM_MASK) | slot;
    return chp->handle;
#endif
}


// Because of late setting of these conditionals.
#if defined(DEV_JACK) || defined(ALSA_CALLBACK)
byte  sound_callback_active = 0;
#endif

// Polling synchronous update of sound device interface.
void LXD_UpdateSound(void)
{
    if( sound_init < 4 )
        return;

#if defined(DEV_JACK) || defined(ALSA_CALLBACK)
    if( sound_callback_active )
        return;
#endif

    // The sound device update calls the mixer,
    // requesting how many samples it can use.
    if( LXD_update_sound )
    {
        LXD_update_sound();  // call by ptr

        // Mix more if driver is short.
        if( want_more )
            LXD_update_sound();  // call by ptr
    }
}


//   handle : the handle returned by StartSound.
//   vol : volume, 0..255
//   sep : separation, +/- 127
void LXD_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
    int slot = handle & CHANNEL_NUM_MASK;

    if( mix_channel[slot].handle == handle )
    {
        mix_channel_t  *  chp = & mix_channel[slot];  // channel to use

        // Per left/right channel.
        //  x^2 seperation,
        //  adjust volume properly.
        //    vol *= 8;

        int leftvol, rightvol;
       
#ifdef SURROUND_SOUND
        chp->invert_right = 0;
        if( sep == SURROUND_SEP )
        {
            // Use normal sound data for the left channel (pan left)
            // and inverted sound data for the right channel (pan right).
            leftvol = rightvol = (vol * (224 * 224)) >> 16;  // slight reduction going through panning
            chp->invert_right = 1;  // invert right channel
        }
        else
#endif
        {
            // Separation, that is, orientation/stereo.
            // sep : +/- 127, <0 is left, >0 is right
            sep += 129;  // 129 +/- 127 ; ( 1 - 256 )
            leftvol = vol - ((vol * sep * sep) >> 16);
            sep = 258 - sep;  // -129 +/- 127
            rightvol = vol - ((vol * sep * sep) >> 16);
        }

        // Sanity check, clamp volume.
        if (rightvol < 0 || rightvol > 255)
        {
            I_SoftError("rightvol out of bounds\n");
            rightvol = ( rightvol < 0 ) ? 0 : 255;
        }
        chp->right_volume = rightvol; // normalized 0..255

        if (leftvol < 0 || leftvol > 255)
        {
            I_SoftError("leftvol out of bounds\n");
            leftvol = ( leftvol < 0 ) ? 0 : 255;
        }
        chp->left_volume = leftvol;  // normalized 0..255

        // vol : range 0..255
        // mix_volume7 : range 0..127

        // Get the proper lookup table for this volume level.
        // (255 * 127) / 255 = 127
//        chp->left_vol_tab = &vol_lookup[(leftvol * mix_volume7) / 63][0];
//        chp->right_vol_tab = &vol_lookup[(rightvol * mix_volume7) / 63][0];
        // (255 * 127) >> 8 = 126
        chp->left_vol_tab = &vol_lookup[(leftvol * mix_volume7) >> 8][0];
        chp->right_vol_tab = &vol_lookup[(rightvol * mix_volume7) >> 8][0];

        // Set stepping
        chp->step = steptable[pitch] * chp->sample_rate / DOOM_SAMPLERATE;
    }
}


#ifdef SNDSERV
// Not usable with Server.
#else
// Direct driver access.
// Return 1 when the sound is still playing.
int LXD_SoundIsPlaying( int handle )
{
    int slot = handle & CHANNEL_NUM_MASK;
    if( mix_channel[slot].handle == handle )
    {
        // Done when data_ptr is cleared.
        return ( mix_channel[slot].data_ptr != NULL );
    }
    return 0;
}
#endif

void LXD_StopSound( int handle )
{
#ifdef SNDSERV
    // SoundServer does not have slot in handle.
    // Due to handle not having slot, must search for handle.
    mix_channel_t * chp;
    for( chp = &mix_channel[0]; chp < &mix_channel[NUM_MIX_CHANNEL]; chp++ )
    {
        if( chp->data_ptr && (chp->handle == handle) )
        {
            stop_sound( chp );
            break;
        }
    }
#else
    // Has slot in handle.
    int slot = handle & CHANNEL_NUM_MASK;
    if( mix_channel[slot].handle == handle )
        stop_sound( &mix_channel[slot] );
#endif
}


// Internal-Sound Interface, init.
void LXD_InitSound( void )
{
    // Secure and configure sound device first.
    GenPrintf(EMSG_info, "LXD_InitSound device: \n");
    sound_init = 1;  // ready for device

    // Initialize mixbuffer with zero.
    memset( mixbuffer, 0, sizeof(mixbuffer) );


#ifdef SOUND_DEVICE_OPTION
    // Caller does select sound device next.
#else
# ifndef SOUND_DEV1
#   error SOUND_DEV1 missing, requires one DEV_
# endif
    // Init the sound device.
    LXD_Init_sound_device( SOUND_DEV1 );
#endif

    // select device does init
    //   LXD_Init_sound_device
    //   sets audio_mode
    //   setup_mixer_tables()
}


// Internal-Sound Interface, Shutdown.
void LXD_ShutdownSound(void)
{
    // Fade out pending sounds.
    // mix_sfxvolume is global in main program and in server, 0..31.
    unsigned int fade_volume = mix_sfxvolume << 14;

    if( (sound_init < 4) || (sound_device == 0) )
        return;

    // Play last of current sounds.
    // Will end, even if output device is off.
    while( fade_volume > 0 )
    {
        LXD_UpdateSound();

        // fade out
        fade_volume --;
        LXD_SetSfxVolume( fade_volume >> 14 );  // 0..31

        if( quiet_samples > 0 )
            break; // no more sound
    }

    LXD_Shutdown_sound_device();

    // Done.
    sound_init = 0;
    return;
}


