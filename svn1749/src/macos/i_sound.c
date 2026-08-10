// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_sound.c 1622 2022-04-03 22:02:37Z wesleyjohnson $
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
// Revision 1.2  2003/07/13 13:18:59  hurdler
// Revision 1.1  2001/04/17 22:23:38  calumr
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
// DESCRIPTION:
//	System interface for sound.
//
//-----------------------------------------------------------------------------

#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <Carbon/Carbon.h>

#include "doomincl.h"
#include "doomstat.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "w_wad.h"
  // ?? no longer gets own lumps
#include "s_sound.h"
#include "command.h"
  // consvar_t
#include "m_swap.h"
#include "d_main.h"
#include "z_zone.h"


static void COM_PlaySong (void);
static void init_music(void);
static void shutdown_music(void);

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Needed for calling the actual sound output.
static int SAMPLECOUNT=		512;
#define NUM_CHANNELS		16

#define SAMPLERATE		11025	// Hz

// The channel data pointers, start and end.
unsigned char*	channels[NUM_CHANNELS];

// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
int		channelstart[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int		channelids[NUM_CHANNELS];			

// Flags for the -nosound and -nomusic options
extern boolean nosoundfx;
extern boolean nomusic;

//start of mac stuff
static SndChannelPtr	soundChannels[NUM_CHANNELS];
static int		channelbusy[NUM_CHANNELS];

static pascal void soundCallback (SndChannelPtr soundChannel, SndCommand *pCmd)
{
    if (pCmd->param1 == 0x1234)
    {
        int *channelInUse = (int *)pCmd->param2;
        *channelInUse = 0;
    }
}


//
// This function adds a sound to the list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
//  vol : volume, 0..255
//  sep : separation, +/- 127, SURROUND_SEP special operation
static int addsfx ( int		sfxid,
		    int		vol,
		    int		step,
		    int		sep )
{
    int  i;
    int	 slot;
    int	 rightvol, leftvol;

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
		    return i;  // already have one, return slot
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
        if ( channels[i] == 0 )  // unused
        {
	    slot = i;
	    break;
	}
        if (channelstart[i] < oldest)  // older
        {
	    slot = i;
	    oldest = channelstart[i];
	}
    }

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    channels[slot] = (unsigned char *) S_sfx[sfxid].data;

    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // volume : range 0..255
    // mix_sfxvolume : range 0..31
    vol = (vol * mix_sfxvolume) >> 6;
    // Notice : sdldoom replaced all the calls to avoid this conversion
    
#ifdef SURROUND_SOUND
    if( sep > 128 )   sep = 0;  // No SURROUND
#endif
    // Separation, that is, orientation/stereo.
    // sep : +/- 127, <0 is left, >0 is right
    sep += 129;  // 129 +/- 127 ; ( 1 - 256 )
    leftvol = vol - ((vol * sep * sep) >> 16);
    sep = 258 - sep;  // 129 +/- 127
    rightvol = vol - ((vol * sep * sep) >> 16);

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

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = sfxid;
    
    {
        ExtSoundHeader theSndBuffer;
        SndCommand theCmd;
		
        theCmd.param1 = 0;
        theCmd.param2 = (rightvol << 16) + leftvol;
        theCmd.cmd = volumeCmd;
        SndDoImmediate (soundChannels[slot], &theCmd);

        theSndBuffer.samplePtr = (Ptr) S_sfx[sfxid].data;
        theSndBuffer.numFrames = S_sfx[sfxid].length;
        theSndBuffer.numChannels = 1; // 2 for stereo
        theSndBuffer.sampleRate = rate11025hz;
        theSndBuffer.encode = extSH;
        theSndBuffer.sampleSize = 8; // 8-bit data

        // Send the buffer to the channel
        theCmd.param1 = 0;
        theCmd.param2 = (long) &theSndBuffer;
        theCmd.cmd = bufferCmd;
		
        //SndDoCommand (soundChannels[slot], &theCmd, false);
        SndDoImmediate (soundChannels[slot], &theCmd);
        channelbusy[slot] = 1;
		
        theCmd.param1 = 0x1234;
        theCmd.param2 = (long) &channelbusy[slot];
        theCmd.cmd = callBackCmd;
        SndDoCommand (soundChannels[slot], &theCmd, false);
    }
    
    return slot;   // handle is slot
}

// Called by NumChannels_OnChange, S_Init
//  num_sfx_channels : the number of sfx maintained at one time.
void I_SetSfxChannels( byte num_channels )
{
    // Uses constant number of mix channels.
#if 0
    // set from cv_numChannels   
    mix_num_channels = ( num_channels > NUM_CHANNELS ) ?
          NUM_CHANNELS  // max
        : num_channels;
#endif
}

void I_SetSfxVolume(int volume)
{
    // Can use mix_sfxvolume (0..31), or set local volume vars.
    // mix_sfxvolume = volume;
}


void I_GetSfx (sfxinfo_t*  sfx)
{
    unsigned char*      sfxdata;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;
   
    S_GetSfxLump( sfx );  // lump to sfx
    sfxdata = (unsigned char*) sfx->data;
    if( ! sfxdata ) return;
    size = sfx->length;

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
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
    sfx->data = (void *) (paddedsfx + 8);  // skip header
}

void I_FreeSfx (sfxinfo_t* sfx)
{
    // use default Free
    if( sfx->data )
        sfx->data -= 8;   // undo skip header, for Z_Free
}

//  vol : volume, 0..255
// Return a channel handle.
int I_StartSound(sfxid_t sfxid, int vol, int sep, int pitch, int priority)
{
    // UNUSED
    priority = 0;

    if(nosoundfx)
	return 0;
	
    id = addsfx( id, vol, pitch, sep );
    
    return id;
}

//   handle : the handle returned by StartSound.
void I_StopSound (int handle)
{
    SndCommand theCmd;
	
    if (handle < 0 || handle >= NUM_CHANNELS) return;

    // Immediately stop this sound
    theCmd.param1 = 0;
    theCmd.param2 = 0;
    theCmd.cmd = quietCmd;
    SndDoImmediate (soundChannels[handle], &theCmd);
    theCmd.cmd = flushCmd;
    SndDoImmediate (soundChannels[handle], &theCmd);
	
    channelbusy[handle] = 0;
}

//   handle : the handle returned by StartSound.
int I_SoundIsPlaying(int handle)
{
    return channelbusy[handle];
}

void I_UpdateSound (void)
{
    // Mac does mixing of channels.

    MusicEvents();  //for QuickTime music playing
}

// You need the handle returned by StartSound.
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
    SndCommand theCmd;
    int leftvol, rightvol;
	
    if(nosoundfx)
        return;

    // vol : range 0..255
    // mix_sfxvolume : range 0..31
    vol = (vol * mix_sfxvolume) >> 6;

#ifdef SURROUND_SOUND
    if( sep > 128 )   sep = 0;  // No SURROUND
#endif
    // Separation, that is, orientation/stereo.
    // sep : +/- 127, <0 is left, >0 is right
    sep += 129;  // 129 +/- 127 ; ( 1 - 256 )
    leftvol = vol - ((vol * sep * sep) >> 16);
    sep = 258 - sep;  // 129 +/- 127
    rightvol = vol - ((vol * sep * sep) >> 16);

    // Send the volume to the channel
    theCmd.param1 = 0;
    theCmd.param2 = (rightvol << 16) + leftvol;
    theCmd.cmd = volumeCmd;
    SndDoImmediate (soundChannels[handle], &theCmd);
}


void I_ShutdownSound(void)
{    
    int i;
	
    if(nosoundfx)
	return;
	
    CONS_Printf("I_ShutdownSound:\n");
	
    for (i = 0; i < NUM_CHANNELS; i++)
    {
        SndDisposeChannel (soundChannels[i], true);
    }
    
    // Music
    shutdown_music();
}


void I_StartupSound()
{ 
    int i;
    int err;
  
    if(nosoundfx)
	return;
    
    // Configure sound device
    CONS_Printf("I_InitSound: \n");
    
    for (i = 0; i < NUM_CHANNELS; i ++)
    {
	soundChannels[i] = NULL;
	channelbusy[i] = 0;
	err = SndNewChannel (&soundChannels[i], sampledSynth, initMono, NewSndCallBackUPP(soundCallback));
    }

#if 0   
    CONS_Printf("\tload all sound data\n");
    for (i=1 ; i<NUMSFX ; i++)
    { 
	// Alias? Example is the chaingun sound linked to pistol.
	if (S_sfx[i].name) { 
            if (!S_sfx[i].link)
	    {
	        // Load data from WAD file.
		S_sfx[i].data = getsfx( &S_sfx[i] );
	    }
	    else
	    {
	        // Previously loaded already?
	        S_sfx[i].data = S_sfx[i].link->data;
	        S_sfx[i].length = S_sfx[i].link->length;
	    }
	}
    }
#endif
   
    // InitMusic
    init_music();
}

//
// MUSIC API.
//

#include <QuickTime/Movies.h>

static int mus_song = 0;
static int musicVolume = 15;

static Movie		midiMovie;
static Boolean		midiLoop;

consvar_t user_songs[PLAYLIST_LENGTH];

boolean PlayThis(char *name);

static void COM_SkipNext (void)
{
    mus_song++;
    
    DisposeMovie (midiMovie);
    midiMovie = NULL;

    if (mus_song==PLAYLIST_LENGTH)
       mus_song = 0;
	
    if (PlayThis(user_songs[mus_song].string))
    {
        CONS_Printf("Playing next song\n");
    }
    else
    {
        CV_Set(&user_songs[mus_song], " ");
    }
}

static void COM_SkipPrev (void)
{
    mus_song--;
    
    DisposeMovie (midiMovie);
    midiMovie = NULL;
	
    if (mus_song==-1)
        mus_song = PLAYLIST_LENGTH;
	
    if (PlayThis(user_songs[mus_song].string))
    {
        CONS_Printf("Playing next song\n");
    }
    else
    {
        CV_Set(&user_songs[mus_song], " ");
    }
}

static void COM_PlayListRandom (void)
{
    CV_SetValue(&play_mode, playlist_random);
    mus_song = M_Random() % PLAYLIST_LENGTH;
    
    CONS_Printf("Playing random user_songs from play list\n");
}

static void COM_PlayList (void)
{
    CV_SetValue(&play_mode, playlist_normal);
    mus_song = 0;
    
    CONS_Printf("Playing play list\n");
}

static void COM_PlayListStop (void)
{
    CV_SetValue(&play_mode, music_normal);
    mus_song = 0;
    DisposeMovie (midiMovie);
    midiMovie = NULL;
	
    CONS_Printf("Stopped play list\n");
}

void MusicEvents (void)
{
    if (nomusic)
        return;
	
    if (midiMovie)
    {
        // Let QuickTime get some time
        MoviesTask (midiMovie, 0);

        // If this song is looping, restart it
        if (IsMovieDone (midiMovie))
        {
	    if (midiLoop)
	    {
	        GoToBeginningOfMovie (midiMovie);
	        StartMovie (midiMovie);
	    }
	    else
	    {
	        DisposeMovie (midiMovie);
	        midiMovie = NULL;
	    }
	}
    }
    else if (play_mode.value == playlist_normal)
    {
        mus_song++;
        if (mus_song==PLAYLIST_LENGTH)
	    mus_song = 0;
        if (PlayThis(user_songs[mus_song].string))
        {
	    CONS_Printf("Playing next song\n");
	}
        else
        {
            CV_Set(&user_songs[mus_song], "");
        }
    }
    else if (play_mode.value == playlist_random)
    {
        mus_song = M_Random() % PLAYLIST_LENGTH;
        if (PlayThis(user_songs[mus_song].string))
        {
    	    CONS_Printf("Playing next song\n");
	}
        else
        {
            CV_Set(&user_songs[mus_song], "");
        }
    }
}

static
void shutdown_music(void) 
{
    if(nomusic)
	return;
	
    CONS_Printf("shutdown_music:\n");
	
    if (midiMovie)
    {
        StopMovie (midiMovie);
        DisposeMovie (midiMovie);
        ExitMovies ();
        midiMovie = NULL;
    }
}

static
void init_music(void)
{
    if(nomusic)
	return;
	
    CONS_Printf("init_music:\n");
	
    if (EnterMovies () != noErr)
    {
        CONS_Printf("\tinit_music: Couldn't initialise Quicktime\n");
        nomusic = true;
    }

    mus_song = 0;
    midiMovie = NULL;

    COM_AddCommand ("playsong",COM_PlaySong);
    COM_AddCommand ("playrandom",COM_PlayListRandom);
    COM_AddCommand ("playlist",COM_PlayList);
    COM_AddCommand ("stopplaylist",COM_PlayListStop);

    COM_AddCommand ("nextsong",COM_SkipNext);
    COM_AddCommand ("prevsong",COM_SkipPrev);
}

void I_PlaySong(int handle, int looping)
{
    if(nomusic)
        return;
    
    if (play_mode.value != music_normal)
        return;
	
    midiLoop = looping;
    if (midiMovie)
    {
        StartMovie (midiMovie);
    }
}

void I_PauseSong (int handle)
{
    if(nomusic)
        return;
	
    if (play_mode.value != music_normal)
        return;
	
    if (midiMovie)
    {
        StopMovie (midiMovie);
    }
}

void I_ResumeSong (int handle)
{
    if(nomusic)
	return;
	
    if (play_mode.value != music_normal)
        return;
	
    if (midiMovie)
    {
        StartMovie (midiMovie);
    }
}

void I_StopSong(int handle)
{
    if(nomusic)
	return;
	
    if (play_mode.value != music_normal)
        return;
	
    if (midiMovie)
    {
        StopMovie (midiMovie);
    }
}

void I_UnRegisterSong(int handle)
{
    if(nomusic)
	return;
	
    if (play_mode.value != music_normal)
        return;
	
    if (midiMovie)
    {
        StopMovie (midiMovie);
        DisposeMovie (midiMovie);
        midiMovie = NULL;
    }
}

boolean PlayThis(char *name)
{
    FSSpec midiSpec;
    OSErr err;
    short midiRef;
    char  mid_file[256];
    FSRef ref;
	
    if(nomusic)
        return false;
	
    if (midiMovie)
        DisposeMovie (midiMovie);
    midiMovie = NULL;
	
    if (!name || *name == 0)
        return false;
	
    {
        char *path;

        if (getenv("DOOMMUSICDIR"))
        {
	    path = getenv("DOOMMUSICDIR");
	    sprintf(mid_file, "%s/%s", path, name);
	}
        else
        {
#ifdef __MACH__
	    //[segabor]: If Music folder is in the Resources folder get mid from there
	    extern char mac_music_home[256];

	    if (mac_music_home[0] == '.')
	        sprintf(mid_file, "./Music/%s", name);
	    else
	        sprintf(mid_file, "%s/%s", mac_music_home, name);
#else
	    path = malloc(256);
	    if ( getcwd(path, 256) == NULL )
	        path = ".";
	    sprintf(mid_file, "%s/Music/%s", path, name);
	    free(path);
#endif
	}
    }
	
    I_OutputMsg("i_sound: Attempting to play %s\n", mid_file);
	
    err = FSPathMakeRef(mid_file, &ref, NULL);
    if (err)
    {
        I_OutputMsg("PlayThis: FSPathMakeRef = %i\n", err);
        return false;
    }
	
    err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &midiSpec, NULL);
    if (err)
    {
        I_OutputMsg("PlayThis: FSGetCatalogInfo = %i\n", err);
        return false;
    }
	
    err = OpenMovieFile (&midiSpec, &midiRef, fsRdPerm);
    if (err)
    {
        I_OutputMsg("PlayThis: OpenMovieFile = %i\n", err);
        return false;
    }
	
    err = NewMovieFromFile (&midiMovie, midiRef, NULL, NULL, newMovieActive, NULL);
    if (err)
    {
        I_OutputMsg("PlayThis: NewMovieFromFile = %i\n", err);
        return false;
    }
	
    GoToBeginningOfMovie (midiMovie);
    PrerollMovie (midiMovie, 0, 0x10000);
    SetMovieVolume (midiMovie, musicVolume << 3);
    StartMovie (midiMovie);
	
    CloseMovieFile (midiRef);
	
    return true;
}

// New interface
int I_RegisterSong( byte music_type, void* data, int len);

// [WDJ] len is unused, keep compatible API
// THIS IS NOT A VALID INTERFACE
int I_RegisterSong(int handle, int len)
{
    Str63 name = "";
	
    if (play_mode.value != music_normal)
        return handle;
	
    // Make sure song number is valid
    if (handle < mus_e1m1 || handle > NUMMUSIC)
        return -1;
	
    mus_song = handle;
	
    strcat ((char *)name, S_music[handle].name);
    strcat((char *)name, ".mid!");
    strupr(name);
    PlayThis(name);

    return handle;
}

static void COM_PlaySong (void)
{
    char  *name;

    if (COM_Argc()<2)
    {
        CONS_Printf("Usage: playsong \"name\"\n\tplaysong <number>\n");
        CONS_Printf("\tRemember to use quotes around the name!\n");
        return;
    }

    name = Z_StrDup (COM_Argv(1));
    
    if (strlen(name)==1)
    {
        CONS_Printf("Playing song 0%i from playlist\n", (name[0]-'0'));
        PlayThis(user_songs[(name[0]-'0')].string);
    }
    else if (strlen(name)==2)
    {
        CONS_Printf("Playing song %i from playlist\n", (name[1]-'0') + (name[0]-'0')*10);
        PlayThis(user_songs[(name[1]-'0') + (name[0]-'0')*10].string);
    }
    else
        PlayThis(name);
    
    Z_Free(name);
}

void I_SetMusicVolume(int volume)
{
    if(nomusic)
	return;
	
    musicVolume = volume;
    if (midiMovie)
        SetMovieVolume (midiMovie, musicVolume << 3);
}

//Hurdler: TODO
void I_StartFMODSong()
{
    CONS_Printf("I_StartFMODSong: Not yet supported under MacOS.\n");
}

void I_StopFMODSong()
{
    CONS_Printf("I_StopFMODSong: Not yet supported under MacOS.\n");
}
void I_SetFMODVolume(int volume)
{
    CONS_Printf("I_SetFMODVolume: Not yet supported under MacOS.\n");
}
