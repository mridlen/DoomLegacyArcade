// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_sound.c 1622 2022-04-03 22:02:37Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: I_sound.c,v $
// Revision 1.6  2007/01/29 06:16:03  chiphog
// Possible fix for playing raw MIDI lumps for the os2 and win32 builds.
//
// Revision 1.5  2004/04/18 12:53:42  hurdler
// fix Heretic issue with SDL and OS/2
//
// Revision 1.4  2003/07/13 13:18:59  hurdler
//
// Revision 1.3  2000/08/16 16:32:27  ydario
// Fixed nosound&nomusic parameters
//
// Revision 1.2  2000/08/10 11:07:51  ydario
// Revision 1.1  2000/08/09 11:56:27  ydario
// OS/2 specific platform code
//
//
// DESCRIPTION:
//    System interface for sound.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <io.h>
#ifndef X_OK
#define X_OK 0
#endif

// do not know why this port does not use the common qmus2mid through buffers
//#define MIDI_FILE_TO_FILE

#include "I_os2.h"

// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>

#include "doomincl.h"
// added for 1.27 19990203 by Kin
#include "doomstat.h"

#include "i_system.h"
#include "i_sound.h"
#include "command.h"
#include "s_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"


#ifdef MIDI_FILE_TO_FILE
#include "qmus2mid2.h"
#else
#include "qmus2mid.h"
#define MIDBUFFERSIZE   128*1024L          // buffer size for Mus2Midi conversion  (ugly code)
static  char*           MidiData_buf;      // buffer allocated at program start for Mus2Mid conversion
#endif
#define MUSIC_MUS_FILE       "doom.mus"
#define MUSIC_MIDI_FILE      "doom.mid"
int                     music_started=0;

static void init_music(void);
static void shutdown_music(void);


// A quick hack to establish a protocol between
// synchronous mix buffer updates and asynchronous
// audio writes. Probably redundant with gametic.
static int flag = 0;


// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Needed for calling the actual sound output.
#define SAMPLECOUNT        512
#define NUM_CHANNELS        8
#define CHANNEL_NUM_MASK  (NUM_CHANNELS-1)
// It is 2 for 16bit, and 2 for two channels.
#define BUFMUL                  4
#define MIXBUFFERSIZE        (SAMPLECOUNT*BUFMUL)

#define SAMPLERATE        11025    // Hz
#define SAMPLESIZE        2       // 16bit

// The actual output device.
int    audio_fd;

// The global mixing buffer.
// Basically, samples from all active internal channels
//  are modifed and added, and stored in the buffer
//  that is submitted to the audio device.
signed short    mixbuffer[MIXBUFFERSIZE];

// The channel step amount...
unsigned int    channelstep[NUM_CHANNELS];
// ... and a 0.16 bit remainder of last step.
unsigned int    channelstepremainder[NUM_CHANNELS];


// The channel data pointers, start and end.
unsigned char*    channels[NUM_CHANNELS];
unsigned char*    channelsend[NUM_CHANNELS];


// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
int        channelstart[NUM_CHANNELS];

// The sound in channel handles,
//  determined on registration,
//  might be used to unregister/stop/modify,
//  currently unused.
int         channelhandles[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int        channelids[NUM_CHANNELS];

// Pitch to stepping lookup, unused.
int        steptable[256];

// Volume lookups.
int        vol_lookup[128*256];

// Hardware left and right channel volume lookup.
int*        channelleftvol_lookup[NUM_CHANNELS];
int*        channelrightvol_lookup[NUM_CHANNELS];
#ifdef SURROUND_SOUND
byte        invert_right[NUM_CHANNELS];
#endif


//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
//  vol : volume, 0..255
//  sep : separation, +/- 127, SURROUND_SEP special operation
int  addsfx( int sfxid, int vol, int step, int sep )
{
    static unsigned short    handlenums = 0;

    int        i;
    int        slot;
    int        leftvol, rightvol;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if (S_sfx[sfxid].flags & SFX_single)
    {
        // Loop all channels, check.
        for (i=0 ; i<NUM_CHANNELS ; i++)
        {
	    // Active, and using the same SFX?
	    if ( (channels[i])
		  && (channelids[i] == sfxid) )
	    {
	        if( S_sfx[sfxid].flags & SFX_id_fin )
		    return channelhandles[i];  // already have one
	        // Reset.
	        channels[i] = 0;
	        break;
	    }
	}
    }

    // Loop all channels to find oldest SFX.
    slot = 0;  // default
    int oldest = INT_MAX;
    for (i=0; i<NUM_CHANNELS; i++)
    {
        if (channels[i] == 0)  // unused
        {
	    slot = i;
	    break;
	}
        if (channelstart[i] < oldest)
        {
	    slot = i;
	    oldest = channelstart[i];
	}
    }

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    channels[slot] = (unsigned char *) S_sfx[sfxid].data;
    // Set pointer to end of raw data.
    channelsend[slot] = channels[slot] + S_sfx[sfxid].length;

    // Set stepping???
    // Kinda getting the impression this is never used.
    channelstep[slot] = step;
    // ???
    channelstepremainder[slot] = 0;
    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // vol : range 0..255
    // mix_sfxvolume : range 0..31
    vol = (vol * mix_sfxvolume) >> 6;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
#ifdef SURROUND_SOUND
    invert_right[slot] = 0;
    if( sep == SURROUND_SEP )
    {
        // Use a normal sound data for the left channel (with pan left)
        // and an inverted sound data for the right channel (with pan right)
        leftvol = rightvol = (vol * (224 * 224)) >> 16;  // slight reduction going through panning
        invert_right[slot] = 1;  // invert right channel
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
        I_SoftError("rightvol (%d) out of bounds\n", rightvol);
        rightvol = ( rightvol < 0 ) ? 0 : 127;
    }
    if (leftvol < 0 || leftvol > 127)
    {
        I_SoftError("leftvol (%d) out of bounds\n", leftvol);
        leftvol = ( leftvol < 0 ) ? 0 : 127;
    }

    // Get the proper lookup table piece
    //  for this volume level???
    channelleftvol_lookup[slot] = &vol_lookup[leftvol*256];
    channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = sfxid;

    // Assign current handle number.
    // Preserved so sounds could be stopped.
    channelhandles[slot] = slot | ((channelhandles[slot] + NUM_CHANNELS) & ~CHANNEL_NUM_MASK);
    return channelhandles[slot];
}



// At one time was called by S_Init.
// Called by I_InitSound.
void  setup_mixer_tables( void )
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int  i, j;
  int * steptablemid = steptable + 128;

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(pow(2.0, (i/64.0))*65536.0);


  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/127;
}


//
// SFX API

// Called by NumChannels_OnChange, S_Init
//  num_sfx_channels : the number of sfx maintained at one time.
void I_SetSfxChannels( byte num_sfx_channels )
{
  // Uses constant number of mix channels.
#if 0   
  printf( "I_SetSfxChannels %d\n", num_channels);

  // set from cv_numChannels   
  mix_num_channels = ( num_sfx_channels > NUM_CHANNELS ) ?
          NUM_CHANNELS  // max
        : num_sfx_channels;
#endif
}

void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.

  // Can use mix_sfxvolume (0..31), or set local volume vars.
  // mix_sfxvolume = volume;
  printf( "I_SetSfxVolume %d\n", volume);
}

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  //snd_MusicVolume = volume;
  printf( "I_SetMusicVolume %d\n", volume);
  // Now set volume on output device.
   SetMIDIVolume( pmData, volume);

  // Whatever( snd_MusciVolume );
}



void I_GetSfx (sfxinfo_t*  sfx)
{
    unsigned char*      sfxdata;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;

    S_GetSfxLump( sfx );
    if( ! sfx->data ) return;
    size = sfx->length;
    sfxdata = (unsigned char*) sfx->data;

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    // ddt: (unsigned char *) realloc(sfxdata, paddedsize+8);
    // This should interfere with zone memory handling,
    //  which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(  paddedsfx, sfxdata, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;

    // Remove the cached lump.
    Z_Free( sfxdata );

    // Preserve padded length.
    sfx->length = paddedsize;
    // Return allocated padded data.
    sfx->data = (void*) (paddedsfx + 8);  // skip header
}


void I_FreeSfx (sfxinfo_t* sfx)
{
    byte*    dssfx;

    if( ! VALID_LUMP(sfx->lumpnum) )
        return;

    // free sample data
    if(sfx->data)
    {
        Z_Free( sfx->data - 8 );  // undo skip header
    }

    sfx->data = NULL;
    sfx->lumpnum = NO_LUMP;
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
//  vol : volume, 0..255
//  sep : separation, +/- 127, SURROUND_SEP special operation
// Return a channel handle.
int I_StartSound(sfxid_t sfxid, int vol, int sep, int pitch, int priority)
{

    if (nosoundfx)
        return -1;

    // Debug.
    //printf( "I_StartSound: starting sound %d\n", id );

    // Returns a handle (not used).
    id = addsfx( id, vol, steptable[pitch], sep );

    // fprintf( stderr, "/handle is %d\n", id );

    return id;
}



//   handle : the handle returned by StartSound.
void I_StopSound (int handle)
{
    int slot = handle & CHANNEL_NUM_MASK;
    if (channelhandles[slot] == handle)
    {
        channels[i] = 0;
    }
}


//   handle : the handle returned by StartSound.
int I_SoundIsPlaying(int handle)
{
    int slot = handle & CHANNEL_NUM_MASK;
    if( channelhandles[slot] == handle )
    {
        return channels[slot] != NULL;
    }
    return 0;
}




//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
//  Writes the mixer buffer to the DSP device.
//
// This function currently supports only 16bit.
//
void I_UpdateSound( void )
{

  // Mix current sound data.
  // Data, from raw sound, for right and left.
  register unsigned int    sample;
  register int        dl;
  register int        dr;

  // Pointers in global mixbuffer, left, right, end.
  signed short*        leftout;
  signed short*        rightout;
  signed short*        leftend;
  // Step in mixbuffer, left and right, thus two.
  int                step;

  // Mixing channel index.
  int                chan;

    // Left and right channel
    //  are in global mixbuffer, alternating.
   leftout = pmData->MixBuffers[ pmData->FillBuffer].pBuffer;
   rightout = leftout+1;// pmData->BufferParms.ulBufferSize/2;
      // next fill buffer
   pmData->FillBuffer++;
   if (pmData->FillBuffer >= pmData->BufferParms.ulNumBuffers)
       pmData->FillBuffer = 0;
   step = 2;

      // Determine end, for left channel only
      //  (right channel is implicit).
      // ulBufferSize is len in bytes (8 bit), ulBufferSize/2 is 16bit length
   leftend = leftout + pmData->BufferParms.ulBufferSize/2;

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT,
    //  that is 512 values for two channels.
   while (leftout != leftend)
   {
      // Reset left/right value.
      dl = 0;
      dr = 0;

      // Love thy L2 chache - made this a loop.
      // Now more channels could be set at compile time
      //  as well. Thus loop those  channels.
      for ( chan = 0; chan < NUM_CHANNELS; chan++ )
      {
         // Check channel, if active.
         if (channels[ chan ])
         {
            //printf( "I_UpdateSound: channel %d active\n", chan);

            // Get the raw data from the channel.
            sample = *channels[ chan ];
            // Add left and right part
            //  for this channel (sound)
            //  to the current data.
            // Adjust volume accordingly.
            dl += channelleftvol_lookup[ chan ][sample];
#ifdef SURROUND_SOUND
            if( chp->invert_right )
              dr -= channelrightvol_lookup[ chan ][sample];
            else
              dr += channelrightvol_lookup[ chan ][sample];
#else
            dr += channelrightvol_lookup[ chan ][sample];
#endif
            // Increment index ???
            channelstepremainder[ chan ] += channelstep[ chan ];
            // MSB is next sample???
            channels[ chan ] += channelstepremainder[ chan ] >> 16;
            // Limit to LSB???
            channelstepremainder[ chan ] &= 65536-1;

            // Check whether we are done.
            if (channels[ chan ] >= channelsend[ chan ])
                channels[ chan ] = 0;
         }
      }

      // Clamp to range. Left hardware channel.
      // Has been char instead of short.
      // if (dl > 127) *leftout = 127;
      // else if (dl < -128) *leftout = -128;
      // else *leftout = dl;

      //dl <<= 4;
      //dr <<= 4;

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

      // Increment current pointers in mixbuffer.
      leftout += step;
      rightout += step;
/*
         // mono out
      *leftout = (dl+dr)/2;
      leftout++;
      rightout++;
*/
   }

  // Submit.
  // Write out the mixbuffer to the DSP device
  //  during each game loop update.
  write(audio_fd, mixbuffer, SAMPLECOUNT*BUFMUL);
}



// You need the handle returned by StartSound.
void I_UpdateSoundParams( int handle, int vol, int sep, int pitch)
{
  // I fail too see that this is used.
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  // UNUSED.
  handle = vol = sep = pitch = 0;
}




void I_ShutdownSound(void)
{
   // Wait till all pending sounds are finished.
   int done = 0;
   int i;

   //added:03-01-98:
   if( !sound_started )
       return;

   // FIXME (below).
   printf( "I_ShutdownSound: NOT finishing pending sounds\n");

   while ( !done )
   {
      for( i=0 ; i<8 && !channels[i] ; i++);

      // FIXME. No proper channel output.
      //if (i==8)
      done=1;
      DosSleep( 100);                      // wait 0.1 sec
   }

   ShutdownDART( pmData);

   // Music
   shutdown_music();

   // Done.
   sound_started = false;
   return;
}


void I_StartupSound()
{
   int i;

   if (nosoundfx)
      return;

      // init dart audio
   InitDART( pmData);

   // Secure and configure sound device first.

   // Initialize external data (all sounds) at start, keep static.
   printf( "I_InitSound\n");

   // Now initialize mixbuffer with zero.
   for ( i = 0; i< MIXBUFFERSIZE; i++ )
      mixbuffer[i] = 0;

   setup_mixer_tables();

   // Finished initialization.
   printf( "I_InitSound: sound module ready\n");

   //Music
   init_music();
   
   //added:08-01-98:we use a similar startup/shutdown scheme as Allegro.
   I_AddExitFunc(I_ShutdownSound);
   sound_started = true;
}




//
// MUSIC API.
// Still no music done.
// Remains. Dummies.
//
static
void init_music(void)
{
   printf( "init_music\n");

   if (nomusic)
      return;
#ifdef MIDI_FILE_TO_FILE
#else
   // initialisation of midicard by I_StartupSound
   MidiData_buf = (char *)Z_Malloc (MIDBUFFERSIZE,PU_STATIC,NULL);
#endif

//   I_AddExitFunc( shutdown_music );  // also done by I_ShutdownSound
   EN_port_music = ADM_MUS | ADM_MIDI;
   music_started = true;
}

void shutdown_music(void)
{
   printf( "shutdown_music\n");

   if (!music_started)
      return;

   I_StopSong( 0);
   I_UnRegisterSong( 0);

   music_started=false;
}

void I_PlaySong(int handle, int looping)
{
   if (nomusic)
      return;

   printf( "I_PlaySong looping=%d\n", looping);
   PlayMIDI( pmData, looping);
      // need to set volume again, because before midi device was closed
   SetMIDIVolume( pmData, cv_musicvolume.value);
}

void I_PauseSong (int handle)
{
   if (nomusic)
      return;

   PauseMIDI( pmData);
}

void I_ResumeSong (int handle)
{
   if (nomusic)
      return;

   // UNUSED.
   handle = 0;
   ResumeMIDI( pmData);
}

void I_StopSong(int handle)
{
   if (nomusic)
      return;

   // UNUSED.
   ShutdownMIDI( pmData);
}

void I_UnRegisterSong(int handle)
{
   if (nomusic)
      return;

   // UNUSED.
   handle = 0;
   printf( "I_UnRegisterSong\n");
      // remove files
   unlink( MUSIC_MIDI_FILE );
   unlink( MUSIC_MUS_FILE );
}

// ---------------
// I_SaveMemToFile
// Save as much as iLength bytes starting at pData, to
// a new file of given name. The file is overwritten if it is present.
// ---------------
void I_SaveMemToFile (unsigned char* pData, unsigned long iLength, char* sFileName)
{
    int     fileHandle;

    fileHandle = open( sFileName, O_CREAT | O_BINARY | O_TRUNC | O_WRONLY,
                       S_IWRITE);
    if (fileHandle == -1)
    {
        I_Error ("SaveMemToFile");
    }
    write( fileHandle, pData, iLength);
    close( fileHandle);
}

int I_RegisterSong( byte music_type, void* data, int len )
{
    int             er;
    char*           MidiData = NULL;       // MIDI music buffer to be played or NULL
    unsigned long   MidiSize;               // size of Midi output data
//   FILE* blah;

    if (nomusic)
        return 1;

#ifdef MIDI_FILE_TO_FILE
    I_SaveMemToFile (data, len, MUSIC_MUS_FILE );
    qmus2mid_file( MUSIC_MUS_FILE, MUSIC_MIDI_FILE, 0, 89,64,1);
#else

#ifdef DEBUGMIDISTREAM
    CONS_Printf("I_RegisterSong: \n");
#endif
    if( music_type == MUSTYPE_MUS )
    {
        // convert mus to mid with a wonderful function
        // thanks to S.Bacquet for the sources of qmus2mid
        // convert mus to mid and load it in memory
        er = qmus2mid((char *)data, 89, 0, len, MIDBUFFERSIZE,
            /*INOUT*/ MidiData_buf, &MidiSize);
        if( er != QM_success )
        {
            CONS_Printf("Cannot convert mus to mid, converterror :%d\n", er);
            return 0;
        }
        MidiData = MidiData_buf;
    }
    else
    {
        // MIDI, MP3, OGG
        // only supports MIDI
        if( music_type != MUSTYPE_MIDI )
	    goto cannot_play;

        // support mid file in WAD !!! (no conversion needed)
        MidiData = data;
	MidiSize = len;
    }

    if (MidiData == NULL)
    {
        CONS_Printf ("Not a valid MIDI file : %c%c%c%c\n",
		     (char)data[0], (char)data[1], (char)data[2], (char)data[3]);
        return 0;
    }
#ifdef DEBUGMIDISTREAM
    else
    {
        I_SaveMemToFile (MidiData, MidiSize, "c:/temp/debug.mid");
    }
#endif
    I_SaveMemToFile (MidiData, MidiSize, MUSIC_MIDI_FILE );
#endif

    OpenMIDI( pmData);

    return 1;

cannot_play:
    CONS_Printf("Music Lump is not MID or MUS lump\n");
    return 0;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
   // UNUSED.
   handle = 0;
   printf( "I_QrySongPlaying\n");

   return 0;
}

#ifdef FMOD_SOUND
//Hurdler: TODO
void I_StartFMODSong()
{
    CONS_Printf("I_StartFMODSong: Not yet supported under OS/2.\n");
}

void I_StopFMODSong()
{
    CONS_Printf("I_StopFMODSong: Not yet supported under OS/2.\n");
}
void I_SetFMODVolume(int volume)
{
    CONS_Printf("I_SetFMODVolume: Not yet supported under OS/2.\n");
}
#endif
