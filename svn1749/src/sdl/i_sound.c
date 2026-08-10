// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sound.c 1690 2024-06-23 20:42:33Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2000-2022 by Doom Legacy team
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log: i_sound.c,v $
// Revision 1.14  2007/01/28 14:45:01  chiphog
// This patch addresses 2 issues:
// * Separated the SDL and HAVE_MIXER assumptions.  We can now compile with
//   SDL=1 but without specifying HAVE_MIXER=1.
// * Fixed the playing of MIDI (from MUS) music under SDL.  One needs to
//   compile with SDL=1 and HAVE_MIXER=1 to get SDL music.  This involved
//   two things:
//   + Removed/replaced the conflicting functions explained here:
//     http://jonatkins.org/SDL_mixer/SDL_mixer.html#SEC5
//   + Backported some SDL mixer mystery magic from legacy-2
// This patch has been tested on linux with:
// - make LINUX=1                       # you get sound, but it ain't SDL
// - make LINUX=1 SDL=1                 # you get SDL sound but no music
// - make LINUX=1 SDL=1 HAVE_MIXER=1    # you get SDL sound and SDL music
// Compiling with HAVE_MIXER=1 but without SDL=1 will fail.
//
// Revision 1.13  2006/07/22 15:38:07  hurdler
// Quick fix for SDL_mixer compiling issue
//
// Revision 1.12  2004/04/18 12:53:42  hurdler
// fix Heretic issue with SDL and OS/2
//
// Revision 1.11  2003/07/13 13:16:15  hurdler
// Revision 1.10  2001/08/20 20:40:42  metzgermeister
//
// Revision 1.9  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.8  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.7  2001/04/14 14:15:14  metzgermeister
// fixed bug no sound device
//
// Revision 1.6  2001/04/09 20:21:56  metzgermeister
// dummy for I_FreeSfx
//
// Revision 1.5  2001/03/25 18:11:24  metzgermeister
//   * SDL sound bug with swapped stereo channels fixed
//   * separate hw_trick.c now for HW_correctSWTrick(.)
//
// Revision 1.4  2001/03/09 21:53:56  metzgermeister
// Revision 1.3  2000/11/02 19:49:40  bpereira
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
//
// DESCRIPTION:
//      System interface for sound.
//
//-----------------------------------------------------------------------------

#include <math.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_mutex.h>
#include <SDL_version.h>
#if ((SDL_MAJOR_VERSION*100)+(SDL_MINOR_VERSION*10)) < 120
# include <SDL_byteorder.h>
#else
# include <SDL_endian.h>
#endif

#ifdef HAVE_MIXER
# define  USE_RWOPS
# include <SDL_mixer.h>
#endif

#include "doomincl.h"
#include "doomstat.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"
#include "w_wad.h"

#include "s_sound.h"

#include "d_main.h"
#include "z_zone.h"

#include "qmus2mid.h"



// MIDI music buffer
#define MIDI_BUFFER_SIZE   (128*1024)

#define MUSIC_FADE_TIME 400 // ms

// The number of internal mixing channels,
//  mixing buffer, and the samplerate of the raw data.

// Doom sound effects
#define DOOM_SAMPLERATE 11025 // Hz

// Needed for calling the actual sound output.
// max. number of simultaneous sounds
#define NUM_CHANNELS  16
#define CHANNEL_NUM_MASK  (NUM_CHANNELS-1)
#define SAMPLERATE    22050 // Hz
// requested audio buffer size (512 means about 46 ms at 11 kHz)
#define SAMPLECOUNT   512

typedef struct {
   
  // The channel data pointers, start and end.
  byte * data_ptr;  // NULL when inactive
  byte * data_end;  // past last sound sample

  unsigned int step;  // The channel step amount...
  unsigned int step_remainder;   // ... and a 0.16 bit remainder of last step.

  // When the channel starts playing, and there are too many sounds,
  // determine which to kill by oldest and priority.
  unsigned int age_priority;
   
  // The data sample rate
  unsigned int samplerate;

  // The sound in channel handles,
  //  determined on registration,
  //  might be used to unregister/stop/modify,
  // Lowest bits are the channel num.
  int handle;

  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  int sfxid;

  // Hardware left and right channel volume lookup.
  int * leftvol_lookup;
  int * rightvol_lookup;

#ifdef SURROUND_SOUND
  byte  invert_right;
#endif
  
} mix_channel_t;

static mix_channel_t  mix_channel[ NUM_CHANNELS ];  // channel


// Pitch to stepping lookup, 16.16 fixed point
static Sint32 steptable[256];

// Volume lookups.
static int vol_lookup[128 * 256];

// Buffer for MIDI
static byte *midi_buffer;

// Flags for the -nosound and -nomusic options
extern boolean nosoundfx;
extern boolean nomusic;

static boolean musicStarted = false;
static boolean soundStarted = false;

static unsigned int sound_age = 1000;  // age counter


//
// SFX API

// At one time was called by S_Init.
// Called by I_InitSound    (emanne@absysteme.fr)
static void setup_mixer_tables(void)
{
    // Init internal lookups (raw data, mixing buffer, channels).
    // This function sets up internal lookups used during
    //  the mixing process.
    int i, j;

    if (nosoundfx)
        return;

    double base_step = (double)DOOM_SAMPLERATE / (double)SAMPLERATE;

    // This table provides step widths for pitch parameters.
    for (i = 0; i < 256; i++)
    {
      steptable[i] = (Sint32)(base_step * pow(2.0, ((i-128) / 64.0)) * 65536.0);
    }

    // Generates volume lookup tables
    //  which also turn the u8 samples into s16 samples.
    for (i = 0; i < 128; i++)
    {
        for (j = 0; j < 256; j++)
        {
            vol_lookup[i * 256 + j] = (i * (j - 128) * 256) / 127;
        }
    }
}

// Called by NumChannels_OnChange, S_Init
//  num_sfx_channels : the number of sfx maintained at one time.
void I_SetSfxChannels( byte num_sfx_channels )
{
    // Fixed number of channels for now.
    // Test with X11 first (2021/1/25)
#if 0   
//    printf( "I_SetSfxChannels %d\n", num_sfx_channels);
#endif
}

void I_SetSfxVolume(int volume)
{
    // Can use mix_sfxvolume (0..31), or set local volume vars.
    // mix_sfxvolume = volume;
}


void I_GetSfx(sfxinfo_t * sfx)
{
    S_GetSfxLump( sfx ); // lump to sfx
    // [WDJ] If save sfx->data += 8 to skip header,
    // then would need to undo it to Free the mem.  Caused Z_Free failure.
    if( sfx->length > 8 )
    {
        sfx->length -= 8;  // length of sound
    }
}

void I_FreeSfx(sfxinfo_t * sfx)
{
    // normal Z_Free in S_FreeSfx
}

#if 0
// cleanly stop a channel
static void stop_channel( mix_channel_t * chanp )
{
    chanp->data_ptr = NULL;
    // Do not release sound lump, it gets used too often,
    // and would have to check for other sound channels using it.
}
#endif


//
// Starting a sound means adding it
//  to the current list of active sounds in the internal channels.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
//  vol : volume, 0..255
//  sep : separation, +/- 127, SURROUND_SEP special operation
// Return a channel handle.
int I_StartSound(sfxid_t sfxid, int vol, int sep, int pitch, int priority)
{
    int handle;
    mix_channel_t  *  chanp;
    int i;
    int slot;

    if (nosoundfx)
        return 0;

#ifndef HAVE_MIXER
    SDL_LockAudio();
#endif

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if (S_sfx[sfxid].flags & SFX_single)
    {
        // Loop all channels, check.
        for (i = 0; i < NUM_CHANNELS; i++)
        {
            chanp = & mix_channel[i];
            // if Active, and using the same SFX
            if ((chanp->data_ptr) && (chanp->sfxid == sfxid))
            {
                if( S_sfx[sfxid].flags & SFX_id_fin )
                    return chanp->handle;  // already have one
                // Kill, Reset.
                chanp->data_ptr = 0;
                break;
            }
        }
    }

    // Loop all channels to find unused channel, or oldest SFX.
    slot = 0;  // default
    int oldest = -1;
    for (i = 0; (i < NUM_CHANNELS); i++)
    {
        if (! mix_channel[i].data_ptr )  // unused
        {
            slot = i;
            break;
        }
        // handles sound_age wrap, by considering only diff
        register unsigned int  agpr = sound_age - mix_channel[i].age_priority;
        if (agpr > oldest)   // older
        {
            slot = i;
            oldest = agpr;
        }
    }
   
    chanp = & mix_channel[slot];  // channel to use

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    chanp->sfxid = sfxid;

    byte * header = S_sfx[sfxid].data;
    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data, skipping header.
    chanp->data_ptr = (unsigned char *) S_sfx[sfxid].data + 8;
//    chanp->data_ptr = & header[8];
    // Set pointer to end of raw data.
    chanp->data_end = chanp->data_ptr + S_sfx[sfxid].length;
   
    // Get samplerate from the sfx header, 16 bit, big endian
    chanp->samplerate = (header[3] << 8) + header[2];

    // Set stepping
    chanp->step = steptable[pitch] * chanp->samplerate / DOOM_SAMPLERATE;
    // 16.16 fixed point
    chanp->step_remainder = 0;
    // balanced between age and priority
    // low priority (higher value) increases age
    chanp->age_priority = sound_age - priority;  // age at start
    sound_age += 16;  // vrs priority 0..256

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    //    vol *= 8;

    // vol : range 0..255
    // mix_sfxvolume : range 0..31
    vol = (vol * mix_sfxvolume) >> 7;
    // Notice : sdldoom replaced all the calls to avoid this conversion

    int leftvol, rightvol;

#ifdef SURROUND_SOUND
    chanp->invert_right = 0;
    if( sep == SURROUND_SEP )
    {
        // Use a normal sound data for the left channel (with pan left)
        // and an inverted sound data for the right channel (with pan right)
        leftvol = rightvol = (vol * (224 * 224)) >> 16;  // slight reduction going through panning
        chanp->invert_right = 1;  // invert right channel
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
    if (rightvol < 0 || rightvol > 127)
    {
        I_SoftError("rightvol out of bounds\n");
        rightvol = ( rightvol < 0 ) ? 0 : 127;
    }

    if (leftvol < 0 || leftvol > 127)
    {
        I_SoftError("leftvol out of bounds\n");
        leftvol = ( leftvol < 0 ) ? 0 : 127;
    }

    // Get the proper lookup table piece
    //  for this volume level
    chanp->leftvol_lookup = &vol_lookup[leftvol * 256];
    chanp->rightvol_lookup = &vol_lookup[rightvol * 256];

    // Assign current handle number.
    // Preserved so sounds could be stopped.
    handle = slot | ((chanp->handle + NUM_CHANNELS) & ~CHANNEL_NUM_MASK);
    chanp->handle = handle;

#ifndef HAVE_MIXER
    SDL_UnlockAudio();
#endif

    // Returns a handle
    return handle;
}


//   handle : the handle returned by StartSound.
//  vol : volume, 0..255
//  sep : separation, +/- 127
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
    int slot = handle & CHANNEL_NUM_MASK;

    if( mix_channel[slot].handle == handle )
    {
        mix_channel_t  *  chanp = & mix_channel[slot];  // channel to use

        // Per left/right channel.
        //  x^2 seperation,
        //  adjust volume properly.
        //    vol *= 8;
        // vol : range 0..255
        // mix_sfxvolume : range 0..31
        vol = (vol * mix_sfxvolume) >> 7;

        int leftvol, rightvol;

#ifdef SURROUND_SOUND
        chanp->invert_right = 0;
        if( sep == SURROUND_SEP )
        {
            // Use normal sound data for the left channel (pan left)
            // and inverted sound data for the right channel (pan right).
            leftvol = rightvol = (vol * (224 * 224)) >> 16;  // slight reduction going through panning
            chanp->invert_right = 1;  // invert right channel
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
        if (rightvol < 0 || rightvol > 127)
        {
            I_SoftError("rightvol out of bounds\n");
            rightvol = ( rightvol < 0 ) ? 0 : 127;
        }

        if (leftvol < 0 || leftvol > 127)
        {
            I_SoftError("leftvol out of bounds\n");
            leftvol = ( leftvol < 0 ) ? 0 : 127;
        }

        // Get the proper lookup table piece
        //  for this volume level
        chanp->leftvol_lookup = &vol_lookup[leftvol * 256];
        chanp->rightvol_lookup = &vol_lookup[rightvol * 256];

        // Set stepping
//        chanp->step = steptable[pitch];
        chanp->step = steptable[pitch] * chanp->samplerate / DOOM_SAMPLERATE;
    }
}


//   handle : the handle returned by StartSound.
void I_StopSound(int handle)
{
    int slot = handle & CHANNEL_NUM_MASK;
    if( mix_channel[slot].handle == handle )
    {
        // outside caller should lock
#ifndef HAVE_MIXER
        SDL_LockAudio();
#endif

        mix_channel[slot].data_ptr = NULL;
//        stop_channel( & mix_channel[slot] );

#ifndef HAVE_MIXER
        SDL_UnlockAudio();
#endif
    }
}

//   handle : the handle returned by StartSound.
int I_SoundIsPlaying(int handle)
{
    int slot = handle & CHANNEL_NUM_MASK;
    if( mix_channel[slot].handle == handle )
    {
        return mix_channel[slot].data_ptr != NULL;
    }
    return 0;
}

//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the given
//  mixing buffer, and clamping it to the allowed
//  range.
//
// This function currently supports only 16bit.
//
void I_UpdateSound(void)
{
    // Uses SDL callback to I_UpdateSound_sdl.

    /*
       Pour une raison que j'ignore, la version SDL n'appelle jamais
       ce truc directement. Fonction vide pour garder une compatibilité
       avec le point de vue de legacy...
     */

    // Himmel, Arsch und Zwirn
}

static void I_UpdateSound_sdl(void *unused, Uint8 *stream, int len)
{
    int chan;
    // Mix current sound data.
    // Data, from raw sound, for right and left.
    if (nosoundfx)
        return;

    // Pointers in audio stream, left, right, end.
    // Left and right channels are multiplexed in the audio stream, alternating.
    Sint16 *leftout  = (Sint16 *)stream;
    Sint16 *rightout = leftout + 1;

    // Step in stream, left and right channels, thus two.
    int step = 2;

    // first Sint16 at least partially outside the buffer
    Sint16 *buffer_end = ((Sint16 *)stream) +len/sizeof(Sint16);

    // SDL2 Does not zero the buffer before use.  BUT, the mixer has already
    // put the music into this buffer, so cannot zero it.

    // Mix sounds into the mixing buffer.
    while (rightout < buffer_end)
    {
        // take the current audio output (incl. music) and mix (add) in our sfx
        register int dl = *leftout;
        register int dr = *rightout;

        // Love thy L2 chache - made this a loop.
        // Now more channels could be set at compile time
        //  as well. Thus loop those  channels.
        // Mixing channel index.
        register mix_channel_t * chanp = & mix_channel[ 0 ];
        for (chan = NUM_CHANNELS; chan > 0; chan--)
        {
            register byte * chan_data_ptr = chanp->data_ptr;
            // Check channel, if active.
            if ( chan_data_ptr )
            {
                // Get the raw data from the channel.
                register unsigned int sample = * chan_data_ptr;
                // Add left and right part for this channel (sound)
                //  to the current data.
                // Adjust volume accordingly.
                dl += chanp->leftvol_lookup[sample];
#ifdef SURROUND_SOUND
                if( chanp->invert_right )
                  dr -= chanp->rightvol_lookup[sample];
                else
                  dr += chanp->rightvol_lookup[sample];
#else
                dr += chanp->rightvol_lookup[sample];
#endif
                // 16.16 fixed point step forward in the sound data
                chanp->step_remainder += chanp->step;
                // take full steps
                chan_data_ptr += chanp->step_remainder >> 16;
                // remainder, save for next round
                chanp->step_remainder &= 0xFFFF;

                // Check whether we are done.
                if (chan_data_ptr >= chanp->data_end)
                    chan_data_ptr = NULL;

                chanp->data_ptr = chan_data_ptr;
            }
            chanp ++;  // next channel
        }

        // Clamp to range. Left hardware channel.
        // Has been char instead of short.

        if (dl > 0x7fff)
            *leftout = 0x7fff;
        else if (dl < -0x8000)
            *leftout = -0x8000;
        else
            *leftout = dl;

        // Same for right hardware channel.
        if (dr > 0x7fff)
            *rightout = 0x7fff;
        else if (dr < -0x8000)
            *rightout = -0x8000;
        else
            *rightout = dr;

        // Increment current pointers in stream
        leftout += step;
        rightout += step;
    }
}




//
// MUSIC API.
//

#ifdef HAVE_MIXER
// The SDL_mixer interface.
// The "registered" piece of music
static struct music_channel_t
{
  Mix_Music * mus;  //
  SDL_RWops * rwop; // must not be freed before music is halted
} music = { NULL, NULL };

# ifdef MUSIC_MP3
static byte  rwop_music_type = 0;
# endif


#if ((SDL_MIXER_MAJOR_VERSION*10000)+(SDL_MIXER_MINOR_VERSION*100)+SDL_MIXER_PATCHLEVEL) >= 10210
#  define MIX_INIT_PRESENT
#endif

#if ((SDL_MIXER_MAJOR_VERSION*10000)+(SDL_MIXER_MINOR_VERSION*100)+SDL_MIXER_PATCHLEVEL) < 10207
  // Older SDL without RWOPS
#define OLD_SDL_MIXER

#ifdef SMIF_PC_DOS
static char * midiname = "DoomMUS.mid";
#else
static char midiname[24] = "/tmp/DoomMUSXXXXXX";
#endif
FILE * midifile;

void Init_OLD_SDL_MIXER( void )
{
#ifndef SMIF_PC_DOS
    // Make temp file name
    mkstemp( midiname );
//    strcat( midiname, ".mid" );
#endif
//    fprintf( stderr, "Midiname= %s\n", midiname );
}

void Free_OLD_SDL_MIXER( void )
{
    // delete the temp file
    remove( midiname );
}


#include <errno.h>
extern int errno;

void Midifile_OLD_SDL_MIXER( byte* midibuf, unsigned long midilength )
{
    midifile = fopen( midiname, "wb" );
    if( midifile )
    {
          fwrite( midibuf, midilength, 1, midifile );
          fclose( midifile );
          if(verbose)
              fprintf( stderr, "Midifile written: %s size=%li\n", midiname, midilength );

          // wants file to have .MID extension, but mkstemp file cannot have extension
          music.mus = Mix_LoadMUS( midiname );
          if( music.mus == NULL )
          {
             I_SoftError("Music load file failed\n");
             perror( "Mix_LoadMUS fails when not cd to doomlegacy directory" );
          }
    }
}
#endif
#endif

#ifdef DONE_CALLBACK
#ifdef HAVE_MIXER
// Callback from SDL
SDLCALL I_music_done_sdl(void)
{
    // ??
}
#endif
#endif


#ifdef USE_RWOPS
// [WDJ] sometimes double free faults when when mixer 1.2.12, playing MP3.
static void free_music_rwop( void )
{
  if (music.rwop)
  {
      // [WDJ] Note eternity never frees the rwops.  Don't know if that is right or not.
      // Prboom and crispy do not use rwops.
      // The sample programs playmus, and playmidi, do free the rwops.
# ifdef MUSIC_MP3
      // Will double free fault if mixer 1.2.12 was playing MP3.
      // Due to autofree of rwops by something else?
      if( rwop_music_type < MUSTYPE_MP3 )
# endif
      {
# ifdef MIX_INIT_PRESENT
        SDL_RWclose( music.rwop );  // as in mixer 1.2.12 playmus playmidi
        // This calls SDL_FreeRW
# else
        SDL_FreeRW( music.rwop );   // worked for MUS
# endif
      }
  }

  music.rwop = NULL;
}
#endif


void I_PlaySong(int handle, byte looping)
{
#ifdef HAVE_MIXER
  if (nomusic)
    return;

  if (music.mus)
  {
      Mix_FadeInMusic(music.mus, looping ? -1 : 1, MUSIC_FADE_TIME);
  }
#endif
}

void I_PauseSong(int handle)
{
#ifdef HAVE_MIXER
  if (nomusic)
    return;

  Mix_PauseMusic();
#endif
}

void I_ResumeSong(int handle)
{
#ifdef HAVE_MIXER
  if (nomusic)
    return;

  Mix_ResumeMusic();
#endif
}

void I_StopSong(int handle)
{
#ifdef HAVE_MIXER
  if (nomusic)
    return;

  Mix_FadeOutMusic(MUSIC_FADE_TIME);
#endif
}


void I_UnRegisterSong(int handle)
{
#ifdef HAVE_MIXER
  if (nomusic)
    return;

  if (music.mus)
  {
      Mix_FreeMusic(music.mus);
      music.mus = NULL;
  }

# ifdef USE_RWOPS
  // [WDJ] sometimes faults when when mixer 1.2.12, playing MP3.
  free_music_rwop();
# endif
#endif
}

// indexed by music_type_e
static byte music_type_to_MUS[] = {
  MUS_NONE,  //   MUSTYPE_MUS
  MUS_MID,   //   MUSTYPE_MIDI
  MUS_MP3,   //   MUSTYPE_MP3
  MUS_OGG,   //   MUSTYPE_OGG
  MUS_NONE,  //   MUSTYPE_OTHER
};
extern char * music_type_str[];

// return handle (always 0)
//  music_type: music_type_e
//  data : ptr to lump data
//  len : length of data
int I_RegisterSong( byte music_type, void* data, int len )
{
#ifdef HAVE_MIXER
  byte mus_type = MUS_NONE;  // SDL_mixer

  if (nomusic)
    return 0;

  if (music.mus)
  {
      I_SoftError("Two registered pieces of music simultaneously!\n");
      return 0;
  }

# ifdef USE_RWOPS
  rwop_music_type = music_type;
# endif
  if( music_type == MUSTYPE_MUS )
  {
      unsigned long midilength;  // per qmus2mid, SDL_RWFromConstMem wants int
      // convert mus to mid in memory with a wonderful function
      // thanks to S.Bacquet for the source of qmus2mid
      int err = qmus2mid(data, len, 89, 0, MIDI_BUFFER_SIZE,
               /*INOUT*/ midi_buffer, &midilength);
      if ( err != QM_success )
      {
          I_SoftError("Cannot convert MUS to MIDI: error %d.\n", err);
          return 0;
      }
      mus_type = MUS_MID;  // SDL_mixer
#ifdef OLD_SDL_MIXER
      Midifile_OLD_SDL_MIXER( midi_buffer, midilength );
#else
      music.rwop = SDL_RWFromConstMem(midi_buffer, midilength);
#endif
  }
  else
  {
      // MIDI, MP3, Ogg Vorbis, various module formats
//      mus_type = MUS_NONE;  // SDL_mixer auto-detect
      mus_type = music_type_to_MUS[ music_type ];
// printf( "mus_type = %s  mus_type = %i\n",  music_type_str[music_type], mus_type );

#ifdef OLD_SDL_MIXER
      Midifile_OLD_SDL_MIXER( data, len );
#else
//      music.rwop = SDL_RWFromMem(data, len);
      music.rwop = SDL_RWFromConstMem(data, len);
#endif
  }

#ifdef OLD_SDL_MIXER
  // In old mixer Mix_LoadMUS_RW does not work.
#else
  // SDL_mixer automatically frees the rwop when the music is stopped.
  // LoadMUS reads the music incrementally, so music must be kept until done playing.
  // LoadWAV copies the music to a buffer, but uses memory.
# ifdef SDL2
  // By explicit music type.
  music.mus = Mix_LoadMUSType_RW(music.rwop, mus_type, 0 );  // free when done
# else
#   ifdef MIX_INIT_PRESENT
  music.mus = Mix_LoadMUSType_RW(music.rwop, mus_type, 0 );
#   else
  if( verbose > 1 )
    GenPrintf( EMSG_ver, "music_type = %s  mus_type = %i\n",  music_type_str[music_type], mus_type );

  music.mus = Mix_LoadMUS_RW(music.rwop);
#   endif
# endif
#endif

  if (!music.mus)
  {
      I_SoftError("Couldn't load music lump: %s\n", Mix_GetError());
# ifdef USE_RWOPS
      free_music_rwop();
# endif
  }

//  debug_Printf("register song\n"); 	// [WDJ] debug
#endif

  return 0;
}


void I_SetMusicVolume(int volume)
{
  // volume: 0--31
#ifdef HAVE_MIXER
  if (nomusic)
    return;

  Mix_VolumeMusic((MIX_MAX_VOLUME * volume) / 32);
#endif
}




void I_StartupSound(void)
{
  static SDL_AudioSpec audspec;  // [WDJ] desc name, too many audio in this file

  if( nosoundfx )
  {
      nomusic = true;
      return;
  }

  // Configure sound device
  CONS_Printf("I_InitSound: \n");

  // Open the audio device
  audspec.freq = SAMPLERATE;
  audspec.format = AUDIO_S16SYS;
  audspec.channels = 2;
  // From eternity, adjust for new samplerate
  audspec.samples = SAMPLECOUNT * SAMPLERATE / DOOM_SAMPLERATE;
  audspec.callback = I_UpdateSound_sdl;

  setup_mixer_tables();

  // InitMusic
#ifdef HAVE_MIXER
  // Use SDL_mixer for music

  // Use normal music when not MP3 or OGG capable.
  EN_port_music = ADM_MUS | ADM_MIDI;

# ifdef MIX_INIT_PRESENT
   // Mixer 1.2.10
   // Do not have to preload these decoders, but if loaded
   // here, then can test for their being present.
   // Otherwise, could use
   uint32_t mi = Mix_Init(
# ifdef SDL2
    // FLUIDSYNTH does not have a Load INIT in SDL2
    MIX_INIT_MID
# else
    // SDL 1.2			 
    MIX_INIT_FLUIDSYNTH
# endif
# ifdef MUSIC_MP3
    | MIX_INIT_MP3
# endif
# ifdef MUSIC_OGG
    | MIX_INIT_OGG
# endif
    );

  if( verbose )
  {
      char mb[64];  // uses 21
      mb[0] = 0;
# ifdef MUSIC_MP3
      if( mi & MIX_INIT_MP3 )  strcat( mb, "MP3, " );
# endif
# ifdef MUSIC_OGG
      if( mi & MIX_INIT_OGG )  strcat( mb, "OGG, " );
# endif
# ifdef SDL2
      // FLUIDSYNTH does not have a Load INIT in SDL2
      if( mi & MIX_INIT_MID )  strcat( mb, "MIDI, " );
# else
      // SDL 1.2.10
      if( mi & MIX_INIT_FLUIDSYNTH )  strcat(mb, "FLUIDSYNTH" );
# endif
      GenPrintf( EMSG_ver, "Mixer: Loaded %s\n", mb );
  }

# ifdef MUSIC_MP3
  if( mi & MIX_INIT_MP3 )
     EN_port_music |= ADM_MP3;
# endif
# ifdef MUSIC_OGG
  if( mi & MIX_INIT_OGG )
     EN_port_music |= ADM_OGG;
# endif
  // end MIX_INIT_PRESENT
# endif

  // because we use SDL_mixer, audio is opened here.
  if (Mix_OpenAudio(audspec.freq, audspec.format, audspec.channels, audspec.samples) < 0)
  {
    // [WDJ] On sound cards without midi ports, opening audio will block music.
    // When midi music is played through Timidity, it will also try to use the
    // dsp port, which is already in use.  Need to use a mixer on sound
    // effect and Timidity output.  Some sound cards have two dsp ports.

        CONS_Printf("Unable to open audio: %s\n", Mix_GetError());
        nosoundfx = nomusic = true;
        return;
  }

  int number_channels;	// for QuerySpec
  if (!Mix_QuerySpec(&audspec.freq, &audspec.format, &number_channels))
  {
      CONS_Printf("Mix_QuerySpec: %s\n", Mix_GetError());
      nosoundfx = nomusic = true;
      return;
  }

  Mix_SetPostMix(audspec.callback, NULL);  // after mixing music, add sound fx
#ifdef DONE_CALLBACK
  Mix_HookMusicFinished( I_music_done_sdl );
#endif
  Mix_Resume(-1); // start all sound channels (although they are not used)

#if 1
  if( verbose > 1 )
  {
      CONS_Printf("Mixer: freq %d Hz, %d channel, %d buffer\n",
              audspec.freq, number_channels, audspec.samples );
  }
#else
  CONS_Printf("Audio device initialized: %d Hz, %d samples/slice.\n",
              audspec.freq, audspec.samples);
#endif

#ifdef SDL2
//  Explicit music cmd.
//  Only for music type MUS_CMD, and only loaded from file.
//  Mix_SetMusicCMD( "" );
#endif
   
  if (!nomusic)
  {
      Mix_ResumeMusic();  // start music playback
      midi_buffer = (byte *)Z_Malloc(MIDI_BUFFER_SIZE, PU_STATIC, NULL);

#ifdef OLD_SDL_MIXER
      Init_OLD_SDL_MIXER();
#endif

#if defined( MUSIC_MP3) || defined( MUSIC_OGG )
# ifdef MIX_INIT_PRESENT
      {
          // Print out to console the Mixer decoders available.
          int  j;
          int  num_mus_dec = Mix_GetNumMusicDecoders();
          if( verbose > 1 )
              GenPrintf( EMSG_ver, "Music decoders:\n");
          for( j = 0; j < num_mus_dec; j++ )
          {
              if( verbose > 1 )
                  GenPrintf( EMSG_ver, "%i: %s\n", j, Mix_GetMusicDecoder(j));
          }

          int  num_chunk_dec = Mix_GetNumChunkDecoders();
          if( verbose > 1 )
              GenPrintf( EMSG_ver, "Music type decoders:\n");
          for( j = 0; j < num_chunk_dec; j++ )
          {
              if( verbose > 1 )
                  GenPrintf( EMSG_ver, "%i: %s\n", j, Mix_GetChunkDecoder(j));
          }
      }
# endif
#endif

      CONS_Printf(" Music initialized.\n");
      musicStarted = true;
  }

#else
  // No SDL_mixer

  // no mixer, no music
  nomusic = true;

  // Open the audio device
  if (SDL_OpenAudio(&audspec, NULL) < 0)
  {
      CONS_Printf("Couldn't open audio with desired format.\n");
      SDL_CloseAudio();
      nosoundfx = nomusic = true;
      return;
  }

  SDL_PauseAudio(0);
#endif

  // Finished initialization.
#ifdef DEBUG
  CONS_Printf("I_InitSound: sound module ready.\n");
#endif
  soundStarted = true;
}


void I_ShutdownSound(void)
{
  if (nosoundfx || !soundStarted)
    return;

  CONS_Printf("I_ShutdownSound: ");

#ifdef HAVE_MIXER
//  Mix_HaltMusic();  // Fade-out
  Mix_CloseAudio();
#  ifdef MIX_INIT_PRESENT
  Mix_Quit();  // Mixer 1.2.10
#  endif
#else
  SDL_CloseAudio();
#endif

  CONS_Printf("shut down\n");
  soundStarted = false;

  // ShutdownMusic
  if (musicStarted)
  {
      Z_Free(midi_buffer);

#ifdef OLD_SDL_MIXER
      Free_OLD_SDL_MIXER();
#endif

      musicStarted = false;
  }
}
