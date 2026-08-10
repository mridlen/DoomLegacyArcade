// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: snd_driver.h 1403 2018-07-06 09:49:21Z wesleyjohnson $
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
//      System drivers sound.
//      Used by Internal-Sound Interface, and by SoundServer.
//
//-----------------------------------------------------------------------------


#ifndef SOUND_DRIVER_H
#define SOUND_DRIVER_H


#ifdef DEBUG_WINDOWED
extern sound_dev_e  sound_device;
#else
extern byte  sound_device;  // sound_dev_e
#endif



// Doom sound effects  
#define DOOM_SAMPLERATE 11025 // Hz
#if 1
// Settings from SDL, ver 1.47.
// requested audio buffer size (512 means about 46 ms at 11 kHz)
#define SAMPLECOUNT     512
#define SAMPLERATE      22050 // Hz
// SAMPLESIZE is detected
#else
// Settings from 1.42.
#define SAMPLECOUNT             1024
#define SAMPLERATE              11025   // Hz
#define SAMPLESIZE              2       // 16bit
#endif


void LXD_SetSfxVolume( int volume );

//  snd_opt : from cv_snd_opt, 99= none
void LXD_SetSoundOption( byte snd_opt );

#ifdef SNDSERV
// Main program determined handle.
void LXD_StartSound_handle ( sfxid_t sfxid, int vol, int sep, int pitch, int priority, int handle, unsigned int sound_age );
#else
// Return a channel handle.
int LXD_StartSound ( sfxid_t sfxid, int vol, int sep, int pitch, int priority, unsigned int sound_age );
int LXD_SoundIsPlaying( int handle );
#endif

void LXD_UpdateSoundParams(int handle, int vol, int sep, int pitch);

void LXD_StopSound( int handle );

void LXD_UpdateSound(void);


void LXD_InitSound( void );
void LXD_ShutdownSound(void);



#ifdef SNDINTR
// Update all 30 millisecs, approx. 30fps synchronized.
// Linux resolution is allegedly 10 millisecs,
//  scale is microseconds.
#define SOUND_INTERVAL     10000
void LX_Init_interrupts( void );

// Get the interrupt. Set duration in millisecs.
int  LX_SoundSetTimer(int duration_of_tick);
void LX_SoundDelTimer(void);
#endif


#endif
