// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_sound.c 1622 2022-04-03 22:02:37Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: i_sound.c,v $
// Revision 1.13  2004/05/13 11:09:38  andyp
// Removed extern int errno references for Linux
//
// Revision 1.12  2004/04/17 12:55:27  hurdler
// now compile with gcc 3.3.3 under Linux
//
// Revision 1.11  2003/07/13 13:16:15  hurdler
// go RC1
//
// Revision 1.10  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.9  2002/07/01 19:59:59  metzgermeister
// *** empty log message ***
//
// Revision 1.8  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.7  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.6  2000/08/11 19:11:07  metzgermeister
// *** empty log message ***
//
// Revision 1.5  2000/04/30 19:47:38  metzgermeister
// iwad support
//
// Revision 1.4  2000/03/28 16:18:42  linuxcub
// Added a command to the Linux sound-server which sets a master volume.
// Someone needs to check that this isn't too much of a performance drop
// on slow machines. (Works for me).
//
// Added code to the main parts of doomlegacy which uses this command to
// implement volume control for sound effects.
//
// Added code so the (really cool) cd music works for me. The volume didn't
// work for me (with a Teac 532E drive): It always started at max (31) no-
// matter what the setting in the config-file was. The added code "jiggles"
// the volume-control, and now it works for me :-)
// If this code is unacceptable, perhaps another solution is to periodically
// compare the cd_volume.value with an actual value _read_ from the drive.
// Ie. not trusting that calling the ioctl with the correct value actually
// sets the hardware-volume to the requested value. Right now, the ioctl
// is assumed to work perfectly, and the value in cd_volume.value is
// compared periodically with cdvolume.
//
// Updated the spec file, so an updated RPM can easily be built, with
// a minimum of editing. Where can I upload my pre-built (S)RPMS to ?
//
// Erling Jacobsen, linuxcub@email.dk
//
// Revision 1.3  2000/03/12 23:21:10  linuxcub
// Added consvars which hold the filenames and arguments which will be used
// when running the soundserver and musicserver (under Linux). I hope I
// didn't break anything ... Erling Jacobsen, linuxcub@email.dk
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System interface for sound.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
  // stdio, stdlib

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>

#if !defined(LINUX) && !defined(SCOOS5) && !defined(_AIX)
#include <sys/filio.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/msg.h>

// See ENABLE_SOUND in doomdef.h.

// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>

// for IPC between xdoom and musserver
#include <sys/ipc.h>

#include <errno.h>

#include "doomstat.h"
  // nomusic
  // nosoundfx
  // Flags for the -nosound and -nomusic options

#include "i_system.h"
#include "i_sound.h"
#include "s_sound.h"
#include "lx_ctrl.h"
  // SOUND_DEVICE_OPTION
  // DEV_xx

#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "searchp.h"
#include "d_main.h"
#include "z_zone.h"

#include "command.h"

#include "musserv/musserver.h"

#ifndef SNDSERV
// Internal-Sound Interface.
# include "snd_driver.h"
#endif


//#define DEBUG_SFX_PIPE


// ======== Sound Controls

#ifdef SNDSERV
consvar_t cv_sndserver_cmd = { "sndserver_cmd", "llsndserv", CV_SAVE };
consvar_t cv_sndserver_arg = { "sndserver_arg", "-quiet", CV_SAVE };
#endif

// commands for music and sound servers
#ifdef SOUND_DEVICE_OPTION

// Multiple sound devices only implemented in Linux X11
// SD_xx are from sound_dev_e in lx_ctrl.h
CV_PossibleValue_t snd_opt_cons_t[] = {
   {0, "Default dev"},
   {SD_S1, "Search 1"},
   {SD_S2, "Search 2"},
   {SD_S3, "Search 3"},
#ifdef DEV_OSS
   {SD_OSS, "OSS"},
#endif   
#ifdef DEV_ESD
   {SD_ESD, "ESD"},
#endif   
#ifdef DEV_ALSA
   {SD_ALSA, "ALSA"},
#endif
#ifdef DEV_JACK
   {SD_JACK, "Jack"},
#endif
#ifdef DEV_PULSE
   {SD_PULSE, "PulseAudio"},
#endif
#ifdef DEV_8  
   {SD_DEV8, "Dev8"},
#endif
#ifdef DEV_9
   {SD_DEV9, "Dev9"},
#endif
   {0, NULL}
};
#endif

#ifdef MUS_DEVICE_OPTION
// values from mus_dev_e
CV_PossibleValue_t musserv_opt_cons_t[] = {
   {MDT_NULL, "Default dev"},
   {MDT_SEARCH1, "Search 1"},
   {MDT_SEARCH2, "Search 2"},
   {MDT_MIDI, "Midi"},    // any MIDI
   {MDT_SYNTH, "Synth"},  // any SYNTH
#ifdef DEV_TIMIDITY
   {MDT_TIMIDITY, "TiMidity"},
#endif
#ifdef DEV_FLUIDSYNTH
   {MDT_FLUIDSYNTH, "FluidSynth"},
#endif
#ifdef DEV_EXTMIDI
   {MDT_EXTMIDI, "Ext Midi"},
#endif
#ifdef DEV_FM_SYNTH
   {MDT_FM_SYNTH, "FM Synth"},
#endif
#ifdef DEV_AWE_32SYNTH
   {MDT_AWE32_SYNTH, "Awe32 Synth"},
#endif
#ifdef DEV_7  
   {MDT_DEV7, "Dev7"},
#endif
#ifdef DEV_8  
   {MDT_DEV8, "Dev8"},
#endif
#ifdef DEV_9
   {MDT_DEV9, "Dev9"},
#endif
   {0, NULL}
};
#endif



// ======== Sound State
byte  sound_init = 0;
  // 0 = not ready, shutdown
  // 1 = ready for selection, no device
  // 2 = sound server ready
  // 6 = sound device ready
  // 7 = sound device active

// Flag to signal CD audio support to not play a title
//int playing_title;


// -------- Sound Server

#ifdef SNDSERV
// SoundServer interface.

#include "sndserv/soundsrv.h"
#define DOOM_SAMPLERATE 11025 // Hz


// UNIX hack, to be removed.
static FILE *    sndserver = 0;
static uint16_t  handle_cnt = 0;

typedef struct {
    // Lowest bits are the channel num.
    // Server uses 16bit handle.
    uint16_t  handle;  // assigned
    // the channel data
    tic_t   lifetime_tic; // estimated end time
    byte    vol, pitch, sep;
} lx_sound_history_t;

// History of sfx sent to server.
#define NUM_LX_HIST  64
#define LX_HIST_INDEX_MASK   (NUM_LX_HIST - 1)
static  lx_sound_history_t  lx_snd_hist[ NUM_LX_HIST ];
// Lookup the sound history using a handle.
#define LX_SNDHIST( handle )  (lx_snd_hist[ (handle) & LX_HIST_INDEX_MASK ])

#endif


#ifdef SNDSERV
// Init SoundServer.
static
void LX_Init_SoundServer( void )
{
    char buffer[2048];
    char *fn_snd;

    memset( lx_snd_hist, 0, sizeof(lx_snd_hist));  // clear history

    fn_snd = searchpath(cv_sndserver_cmd.string);

    // start sound process
    if (!access(fn_snd, X_OK))
    {
        sprintf(buffer, "%s %s", fn_snd, cv_sndserver_arg.string);
        sndserver = popen(buffer, "w");
#ifdef  DEBUG_SFX_PIPE
        GenPrintf( EMSG_debug," Started Sound Server:\n" );
#endif
        sound_init = 2;
    }
    else
        GenPrintf(EMSG_error, "Could not start sound server [%s]\n", fn_snd);
}
#endif


#ifdef SNDSERV
static
void LX_Shutdown_SoundServer(void)
{
    // Send QUIT to SoundServer.
    if (sndserver)
    {
#ifdef  DEBUG_SFX_PIPE
        GenPrintf( EMSG_debug," Command Q:\n" );
#endif
        // Send a "quit" command.
        fputc('q', sndserver);
        fflush(sndserver);
    }

    // Done.
    sound_init = 0;
    return;
}
#endif
   


// ------------- Interface Functions

// Interface
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void I_GetSfx(sfxinfo_t * sfx)
{
    byte * dssfx;
    int size_data;

    S_GetSfxLump( sfx ); // lump to sfx
    // Linked sounds will reuse the data.
    // Can set data to NULL, but cannot change its format.
    
    dssfx = (byte *) sfx->data;  // header
    if( ! dssfx )  return;

    // Sound data header format.
    // 0,1: 03
    // 2,3: sample rate (11,2B)=11025, (56,22)=22050
    // 4,5: number of samples
    // 6,7: 00
    size_data = sfx->length - 8;  // bytes
    if( size_data <= 0 )
    {
        sfx->length = 8;
        GenPrintf( EMSG_warn, "GetSfx, short sound: %s\n", sfx->name );
        // must load something, StartSound will use this sfx_id.
    }

#ifdef SNDSERV
    // Send Sfx to SoundServer.
    // write data to llsndserv 19990201 by Kin
    if (sndserver)
    {
        server_load_sound_t  sls;
        // Send sound data to server.
        sls.flags = sfx->flags;
        // The sound server does not need padded sound, and if it did
        // it could more easily do that itself.
        sls.snd_len = size_data;
        // [WDJ] No longer send volume with load, as it interferes with
        // the automated volume update.
        sls.id = sfx - S_sfx;

#ifdef  DEBUG_SFX_PIPE
        GenPrintf( EMSG_debug," Command L:  Sfx  length=%x\n", sfx->length );
        GenPrintf( EMSG_debug," Load sound, sfx=%i, snd_len=%i  flagx=%x\n ", sls.id, sls.snd_len, sls.flags );
#endif
        // sfx data loaded to sound server at sfx id.
        fputc('l', sndserver);
        fwrite((byte*)&sls, sizeof(sls), 1, sndserver);  // sfx id, flags, size
        // sndserver needs the header
        fwrite(&dssfx[0], 1, sls.snd_len + 8, sndserver);  // including header
//        fwrite(&dssfx[8], 1, sls.snd_len, sndserver);  // without headr
        // send 8 NULL commands, in case of data de-sync
        fwrite( "\0\0\0\0\0\0\0", 8, 1, sndserver );
        fflush(sndserver);
    }

#else
    // Direct sound driver.
#ifdef  HAVE_ALLEGRO   
    // Internal-Sound Interface, with Allegro.
    // convert raw data and header from Doom sfx to a SAMPLE for Allegro
    // Linked sound will already be padded.
    // Round-up to buffer to whole SAMPLECOUNT.
    int sampsize = (size_data + (SAMPLECOUNT - 1)) / SAMPLECOUNT;
    int reqsize = (sampsize * SAMPLECOUNT) + 8;
    if( reqsize > size )
    {
        // Only reallocate when necessary.
        byte *paddedsfx = (byte *) Z_Malloc(reqsize, PU_STATIC, 0);
        memcpy(paddedsfx, dssfx, size);
        for (i = size; i < reqsize; i++)
            paddedsfx[i] = 128;
        sfx->data = (void *) paddedsfx;
        sfx->length = reqsize;
        // Remove the cached lump.
        Z_Free(dssfx);
        dssfx = paddedsfx;
    }
    *((uint32_t *) dssfx) = sampsize;
#endif

#endif
}

void I_FreeSfx(sfxinfo_t * sfx)
{
    // normal free
}


//  volume : 0..31
// Caller has already set mix_sfxvolume, and with range checks.
void I_SetSfxVolume(int volume)
{
    // Basically, this should propagate the menu/config file setting
    //  to the state variable used in the mixing.

    // Can use mix_sfxvolume (0..31), or set local volume vars.
    // Assert: volume == mix_sfxvolume;
   
#ifdef SNDSERV
    // Send Volume Control to SoundServer.

    if (sndserver)
    {
        byte mbuf[4];

#ifdef  DEBUG_SFX_PIPE
        GenPrintf( EMSG_debug," Command V:  volume=%i\n", volume );
#endif
        mbuf[0] = 'v';
        mbuf[1] = volume;
        mbuf[2] = cv_numChannels.EV;
        fwrite( mbuf, 1, 3, sndserver);
        fflush(sndserver);
    }

#else
    // Direct sound driver.
    LXD_SetSfxVolume( volume );  // from master volume  0..31
#endif
}


// Starts a sound in a particular sound channel.
//  vol : volume, 0..255
//  sep : separation, +/- 127, SURROUND_SEP special operation
// Return a channel handle.
int I_StartSound ( sfxid_t sfxid, int vol, int sep, int pitch, int priority )
{
#ifdef SNDSERV
    server_play_sound_t  sps;

    // SoundServer: Send sound, sfxid, and param, to server.
    // Sounds have been loaded to Server previously.
    if( ! sndserver)
        return 0;

    // Prepare play message.
    sps.sfxid = sfxid;
    sps.priority = priority;
    // Lowest bits of handle are index to lx_snd_hist.
    sps.handle = handle_cnt++;   // 16 bit handle
    // Record in history.
    lx_sound_history_t * lxsh = & LX_SNDHIST( sps.handle );
    lxsh->handle = sps.handle;
    lxsh->vol = sps.vol = vol;
    lxsh->pitch = sps.pitch = pitch;
    lxsh->sep = sps.sep = sep;

#ifdef  DEBUG_SFX_PIPE
    GenPrintf( EMSG_debug," Command P:" );
    GenPrintf( EMSG_debug," Play sound, sfx=%i, vol=%i, pitch=%i, sep=%i, handle=%i\n ", sps.sfxid, sps.vol, sps.pitch, sps.sep, sps.handle );
#endif
   
    // Send play sound
    fputc('p', sndserver);
    fwrite((byte*)&sps, sizeof(sps), 1, sndserver);
    fflush(sndserver);
       
    // estimated sound endtime, rounded up at 10%
    lxsh->lifetime_tic = gametic + ((S_sfx[sfxid].length + ((DOOM_SAMPLERATE/TICRATE) * 9/10)) / (DOOM_SAMPLERATE/TICRATE));
    return sps.handle;

#else
    // Direct sound driver.
    //  sound_age : vrs priority 0..255
    // return handle.
    return  LXD_StartSound( sfxid, vol, sep, pitch, priority, gametic << 4 );
#endif
}


// You need the handle returned by StartSound.
void I_StopSound(int handle)
{
#ifdef SNDSERV
    // SoundServer: Send STOP to channel in server.
    uint16_t  handle16 = handle;

#ifdef  DEBUG_SFX_PIPE
    GenPrintf( EMSG_debug," Command S:  Stop  handle=%i\n", handle );
#endif
   
    // Send stop sound.
    fputc('s', sndserver);
    fwrite(&handle16, sizeof(uint16_t), 1, sndserver);  // handle
    fflush(sndserver);

#else
    // Direct sound driver.
    // Has slot in handle, and direct access to channel.
    LXD_StopSound( handle );
#endif
}


int I_SoundIsPlaying(int handle)
{
#ifdef SNDSERV
    // SoundServer does not have slot in handle, cannot ask server.
    // SoundServer, guess based on expected time.
    lx_sound_history_t * lxsh = & LX_SNDHIST( handle );
    if( lxsh->handle != handle )
        return 0;  // lost from history, not expected to be playing

    // lifetime_tic is when sound is estimated to end.
    // Due to gametic wrap, compare only using unsigned diff
    return  (lxsh->lifetime_tic - gametic) < 0x3FFF;  // gametic < lifetime_tic

#else
    // Direct sound driver.
    // Has slot in handle, and direct access to channel.
    return  LXD_SoundIsPlaying( handle );
#endif
}


// Interface
//   handle : the handle returned by StartSound.
//   vol : volume, 0..255
//   sep : separation, +/- 127
//   pitch : 128=normal
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
#ifdef SNDSERV
    // SoundServer: communicate new settings to server.
    server_update_sound_t  sus;

    if( ! sndserver)
        return;

    lx_sound_history_t * lxsh = & LX_SNDHIST( handle );
    if( lxsh->handle != handle )
        return;

    if( (lxsh->vol == vol) && (lxsh->pitch == pitch) && (lxsh->sep == sep) )
        return; // no difference

    lxsh->vol = sus.vol = vol;
    lxsh->pitch = sus.pitch = pitch;
    lxsh->sep = sus.sep = sep;
    sus.handle = handle;

#ifdef  DEBUG_SFX_PIPE
    GenPrintf( EMSG_debug," Command R:" );
    GenPrintf( EMSG_debug," Update sound, vol=%i, pitch=%i, sep=%i, handle=%i\n ", sus.vol, sus.pitch, sus.sep, sus.handle );
#endif
   
    // Update sound params
    fputc('r', sndserver);
    fwrite((byte*)&sus, sizeof(sus), 1, sndserver);
    fflush(sndserver);

#else
    // Direct sound driver.
    // Has slot in handle, direct access to sfx.
    LXD_UpdateSoundParams( handle, vol, sep, pitch);
#endif
}

// SoundServer: when SNDSERV, d_main.c does not call UpdateSound()
#ifndef SNDSERV
// Interface
void I_UpdateSound( void )
{
    LXD_UpdateSound();
}
#endif


#ifdef SOUND_DEVICE_OPTION
// Called by cv_snd_opt changes.
//  snd_opt : SD_xx, from sound_dev_e in lx_ctrl.h
void I_SetSoundOption( byte snd_opt )
{
    byte mbuf[4];

    if( sound_init == 0 )
        return;  // sound system not ready for this

#ifdef SNDSERV
    if( sndserver == NULL )  // during init
        return;

    // Send selection to SoundServer.
    mbuf[0] = 'd';
    mbuf[1] = snd_opt;
    fwrite( mbuf, 1, 2, sndserver );
    fflush(sndserver);
#else
    // Direct sound driver.
    LXD_SetSoundOption( snd_opt );
#endif
}
#endif



// ================ MUSIC
// #define DEBUG_MUSSERV

// ======== Music Controls

// Music volume may be set before calling LX_InitMusic.
static int music_volume = 0;

static byte music_looping = 0;
static uint32_t  music_dies = -1;


// ======== Music Server

// UNIX hack too, unlikely to be removed.
#ifdef MUSSERV
static int musserver = -1;
static int msg_id = -1;  // IPC pipe
#endif


#ifdef MUSSERV
// MusicServer, support functions.
mus_msg_t  msg_buffer;

void send_val_musserver( char command, char sub_command, int val )
{
    if( msg_id < 0 )  return;

    msg_buffer.mtype = 5;
    memset(msg_buffer.mtext, 0, MUS_MSG_MTEXT_LENGTH);
    snprintf(msg_buffer.mtext, MUS_MSG_MTEXT_LENGTH-1, "%c%c%i",
             command, sub_command, val);
    msg_buffer.mtext[MUS_MSG_MTEXT_LENGTH-1] = 0;
#ifdef  DEBUG_MUSSERV
    GenPrintf( EMSG_debug, "Send musserver: %s\n", msg_buffer.mtext );
#endif
    msgsnd(msg_id, MSGBUF(msg_buffer), 12, IPC_NOWAIT);
    usleep(2);  // just enough for musserver to respond promptly.
}
#endif


// ======== Music Interface

#ifdef MUSSERV
//
// MUSIC API.
// Music done now, we'll use Michael Heasley's musserver.
//
static
void LX_InitMusic(void)
{
    // MusicServer, INIT.
    char buffer[MAX_WADPATH];
    char *fn_mus;

    fn_mus = searchpath(cv_musserver_cmd.string);

    // Try to start the music server process.
    if ( access(fn_mus, X_OK) < 0)
    {
        GenPrintf(EMSG_error, "Could not find music server [%s]\n", fn_mus);
        return;
    }

    // [WDJ] Use IPC for settings, not command line.
    snprintf(buffer, MAX_WADPATH-1, "%s %s &", fn_mus, cv_musserver_arg.string);
    buffer[MAX_WADPATH-1] = 0;
    GenPrintf( EMSG_info, "Starting music server [%s]\n", buffer);
    // Sys call "system()"  seems to work, and does not need \n.
    // It returns 0 on success.
    musserver = system(buffer);
    if( musserver < 0 )
    {
        GenPrintf( EMSG_error, "Could not start music server [%s]\n", fn_mus);
        return;
    }

    // Start the IPC after, and mussserver is supposed to wait.
    msg_id = msgget(MUSSERVER_MSG_KEY, IPC_CREAT | 0777);
   
    if( verbose > 1 )
        GenPrintf( EMSG_info, "Started Musicserver = %i, IPC = %i\n", musserver, msg_id );

    send_val_musserver( 'v', ' ', music_volume );
    // [WDJ] Starting with system() gives the process a PPID of 1, which is Init.
    // When DoomLegacy is killed, it is not detected by musserver.
    send_val_musserver( 'I', ' ', getpid() ); // our pid
    // Send this again because it was too early at configure time.
//    I_SetMusicOption();
    usleep(100);
}
#endif

void LX_ShutdownMusic(void)
{
    if (nomusic)
        return;

#ifdef MUSSERV
    // MusicServer, Shutdown.
    // [WDJ] It is a race between the quit command and the queue destruction.
    // Rely upon one or the other.
#if 1
    // send a "quit" command.
    send_val_musserver( 'Q', 'Q', 0 );  // "QQ" required
#else
    if (musserver > -1)
    {
        // Close the queue.
        if (msg_id != -1)
            msgctl(msg_id, IPC_RMID, (struct msqid_ds *) NULL);
    }
#endif
#endif
}


// ======== Music Interface

// MUSIC API
void I_SetMusicVolume(int volume)
{
    // Internal state variable.
    music_volume = volume;
    // Now set volume on output device.
    // Whatever( snd_MusciVolume );

    if (nomusic)
        return;

#ifdef MUSSERV
    // Send Volume control to MusicServer.
    send_val_musserver( 'v', ' ', volume );
#endif
}

// MUSIC API

// indexed by sound_dev_e, values of snd_opt_cons_t
char snd_ipc_opt_tab[] = {
  'd', // Default
  'a', // Search 1
  'b', // Search 2
  'c', // Search 3
  'O', // OSS
  'E', // ESD
  'A', // ALSA
  'P', // PulseAudio
  'J', // JACK
  'g', // Dev6
  'h', // Dev7
  'j', // Dev8
  'k'  // Dev9
};

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

void I_SetMusicOption( byte mus_opt )
{
#ifdef MUSSERV
#ifdef SOUND_DEVICE_OPTION
    byte si = cv_snd_opt.EV;
#else
    byte si = SOUND_DEV1;
#endif
    char sc, mc;

    if( musserver < 0 )  // during init
        return;

    if( msg_id < 0 )  return;
   
    // Use 'Z' for MUTE.
    sc = ( si < SD_DEV9 )? snd_ipc_opt_tab[si] : 'Z';
    mc = (mus_opt < MDT_DEV9)? mus_ipc_opt_tab[mus_opt] : 'Z';
   
    msg_buffer.mtype = 6;
    memset(msg_buffer.mtext, 0, MUS_MSG_MTEXT_LENGTH);
    snprintf(msg_buffer.mtext, MUS_MSG_MTEXT_LENGTH-1, "O-s%c-d%c", sc, mc );
    msg_buffer.mtext[MUS_MSG_MTEXT_LENGTH-1] = 0;
#ifdef  DEBUG_MUSSERV
    GenPrintf( EMSG_debug, "Send musserver option: %s\n", msg_buffer.mtext );
#endif
    msg_buffer.mtext[MUS_MSG_MTEXT_LENGTH-1] = 0;
    msgsnd(msg_id, MSGBUF(msg_buffer), MUS_MSG_MTEXT_LENGTH, IPC_NOWAIT);
#endif
}



void I_PauseSong(int handle)
{
    if (nomusic)
        return;

#ifdef MUSSERV
    // Send PAUSE to MusicServer.
    send_val_musserver( 'P', 'P', 1 );
#endif
    handle = 0;  // UNUSED
}

void I_ResumeSong(int handle)
{
    if (nomusic)
        return;

#ifdef MUSSERV
    // Send PAUSE-RESUME to MusicServer.
    send_val_musserver( 'P', 'R', 0 );
#endif
    handle = 0;  // UNUSED
}

void I_StopSong(int handle)
{
    if (nomusic)
        return;

#ifdef MUSSERV
    // Send STOP to MusicServer.
    send_val_musserver( 'X', 'X', 0 );
#endif
    handle = 0; // UNUSED.
    music_looping = 0;
    music_dies = 0;
}

void I_UnRegisterSong(int handle)
{
    handle = 0; // UNUSED.
}


#ifdef MUSSERV
// MusicServer, player.
// Information for ports with music servers.
//  name : name of song
// Return handle
int I_PlayServerSong( char * name, lumpnum_t lumpnum, byte looping )
{
    if (nomusic)
    {
        return 1;
    }

    music_dies = gametic + (TICRATE * 30);

    music_looping = looping;

    if (msg_id != -1)
    {
        static  byte sent_genmidi = 0;
        wadfile_t * wadp;
       
        msg_buffer.mtype = 6;
        if( sent_genmidi == 0 )
        {
            sent_genmidi = 1;
            // Music server needs the GENMIDI lump, which may depend
            // upon the IWAD and PWAD order.
            lumpnum_t  genmidi_lumpnum = W_GetNumForName( "GENMIDI" );
            wadp = lumpnum_to_wad( genmidi_lumpnum );
            if( wadp )
            {
                memset(msg_buffer.mtext, 0, MUS_MSG_MTEXT_LENGTH);
                snprintf(msg_buffer.mtext, MUS_MSG_MTEXT_LENGTH-1, "W%s", wadp->filename);
                msg_buffer.mtext[MUS_MSG_MTEXT_LENGTH-1] = 0;
#ifdef  DEBUG_MUSSERV
                GenPrintf( EMSG_debug, "Send musserver wad: %s\n", msg_buffer.mtext );
#endif
                msgsnd(msg_id, MSGBUF(msg_buffer), MUS_MSG_MTEXT_LENGTH, IPC_NOWAIT);
            }
            // Sending genmidi lumpnum to musserver.
            send_val_musserver( 'G', ' ', LUMPNUM(genmidi_lumpnum) );
        }
        // Send song name to musserver
        memset(msg_buffer.mtext, 0, MUS_MSG_MTEXT_LENGTH);
        sprintf(msg_buffer.mtext, "D %s", name);
#ifdef  DEBUG_MUSSERV
        GenPrintf( EMSG_debug, "Send musserver song: %s\n", msg_buffer.mtext );
#endif
        msgsnd(msg_id, MSGBUF(msg_buffer), 12, IPC_NOWAIT);
        // Song info
        wadp = lumpnum_to_wad( lumpnum );
        if( wadp )
        {
            // Send song wad information to server
            memset(msg_buffer.mtext, 0, MUS_MSG_MTEXT_LENGTH);
            snprintf(msg_buffer.mtext, MUS_MSG_MTEXT_LENGTH-1, "W%s", wadp->filename);
            msg_buffer.mtext[MUS_MSG_MTEXT_LENGTH-1] = 0;
#ifdef  DEBUG_MUSSERV
            GenPrintf( EMSG_debug, "Send musserver wad: %s\n", msg_buffer.mtext );
#endif
            msgsnd(msg_id, MSGBUF(msg_buffer), MUS_MSG_MTEXT_LENGTH, IPC_NOWAIT);
        }
        // Sending song lumpnum to musserver.
        send_val_musserver( 'S', (looping?'C':' '), LUMPNUM(lumpnum) );
    }
    return 1;
}


#else
// Internal-Music Interface.
// not MUSSERV

// Interface
int I_RegisterSong( byte music_type, void* data, int len )
{
    if (nomusic)
    {
        return 0;
    }

    data = NULL;
    return 1;
}

// Interface
void I_PlaySong(int handle, byte looping)
{
    music_dies = gametic + (TICRATE * 30);

    if (nomusic)
        return;

    music_looping = looping;
    handle = 0;
}
#endif


#if 0
// Disabled call, no interface.
// Is the song playing?
int I_QrySongPlaying(int handle)
{
    handle = 0;  // UNUSED
    return music_looping || (music_dies > gametic);
}
#endif

#ifdef FMOD_SONG
//Hurdler: TODO
void I_StartFMODSong()
{
    CONS_Printf("I_StartFMODSong: Not yet supported under Linux.\n");
}

void I_StopFMODSong()
{
    CONS_Printf("I_StopFMODSong: Not yet supported under Linux.\n");
}
void I_SetFMODVolume(int volume)
{
    CONS_Printf("I_SetFMODVolume: Not yet supported under Linux.\n");
}
#endif


// ======== Sound and Music, Startup and Shutdown

//--- Sound system Interface
// Interface, Start sound system.
void I_StartupSound()
{
    if(! nosoundfx )
    {
#ifdef SNDSERV
        // Sound Server
        LX_Init_SoundServer();
#else
        // Direct sound driver.
        LXD_InitSound();
#endif
    }

#ifdef SOUND_DEVICE_OPTION
    // Select device from config.
    // config file has been loaded
    I_SetSoundOption( cv_snd_opt.EV );
#endif

    // Only has a MUS player.
    EN_port_music = ADM_MUS;

    if(! nomusic )
    {
#ifdef MUSSERV
        LX_InitMusic();

#ifdef MUS_DEVICE_OPTION
        // Select device from config.
        // config file has been loaded
        I_SetMusicOption( cv_musserver_opt.EV );
#else
        I_SetMusicOption( MUS_DEV1 );
#endif
#endif
    }
}

// Interface, Shutdown sound system.
void I_ShutdownSound(void)
{
#ifdef SNDINTR
    LX_SoundTimer_del();
#endif

#ifdef SNDSERV
    LX_Shutdown_SoundServer();
#else
    // Direct sound driver.
    LXD_ShutdownSound();
#endif

    LX_ShutdownMusic();
}

// ======== Interrupts

#ifdef SNDINTR
// Internal-Sound Interface, Interrupts.
//
// Experimental stuff.
// A Linux timer interrupt, for asynchronous
//  sound output.
// I ripped this out of the Timer class in
//  our Difference Engine, including a few
//  SUN remains...
//  
#ifdef sun
typedef sigset_t tSigSet;
#else
typedef int tSigSet;
#endif

// We might use SIGVTALRM and ITIMER_VIRTUAL, if the process
//  time independent timer happens to get lost due to heavy load.
// SIGALRM and ITIMER_REAL doesn't really work well.
// There are issues with profiling as well.

//static int /*__itimer_which*/  itimer = ITIMER_REAL;
static int /*__itimer_which*/ itimer = ITIMER_VIRTUAL;

//static int sig = SIGALRM;
static int sig = SIGVTALRM;

// Interrupt handler.
static void LX_SoundTimer_handler(int ignore)
{
    // Debug.
    //GenPrintf(EMSG_debug, "%c", '+' ); fflush( stderr );

    // Feed sound device if necesary.
    LXD_UpdateSound();   // mix and service device

    // UNUSED, but required.
    ignore = 0;
    return;
}

// Get the interrupt. Set duration in millisecs.
static int LX_SoundTimer_set(int duration_of_tick)
{
    // Needed for gametick clockwork.
    struct itimerval value;
    struct itimerval ovalue;
    struct sigaction act;
    struct sigaction oact;

    int res;

    // This sets to SA_ONESHOT and SA_NOMASK, thus we can not use it.
    //     signal( _sig, handle_SIG_TICK );

    // Now we have to change this attribute for repeated calls.
    act.sa_handler = LX_SoundTimer_handler;
#ifndef sun
    //ac  t.sa_mask = _sig;
#endif
    act.sa_flags = SA_RESTART;

    sigaction(sig, &act, &oact);

    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = duration_of_tick;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = duration_of_tick;

    // Error is -1.
    res = setitimer(itimer, &value, &ovalue);

    // Debug.
    if (res == -1)
        GenPrintf(EMSG_debug, "LX_SoundTimer_set: interrupt n.a.\n");

    return res;
}

// Remove the interrupt. Set duration to zero.
static void LX_SoundTimer_del( void )
{
    // Debug.
    if (LX_SoundTimer_set(0) == -1)
        GenPrintf(EMSG_debug, "I_SoundDelTimer: failed to remove interrupt. Doh!\n");
}

static void  LX_Init_interrupts( void )
{
    // Internal-Sound Interface, Interrupts.
    GenPrintf(EMSG_info, "LX_SoundTimer_set: %d microsecs\n", SOUND_INTERVAL);
    LX_SoundTimer_set(SOUND_INTERVAL);
}

#endif
