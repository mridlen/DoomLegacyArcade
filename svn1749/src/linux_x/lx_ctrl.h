// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: lx_ctrl.h 1403 2018-07-06 09:49:21Z wesleyjohnson $
//
// Copyright (C) 2021 by DooM Legacy Team.
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
//   Sound device defines, handling.
//   Sound Menu Controls.
//   Used by Internal-Sound Interface, and by SoundServer.
//
//-----------------------------------------------------------------------------

#ifndef LX_CTRL_DEV_LOGIC
#define LX_CTRL_DEV_LOGIC

#include "command.h"

// ============= SFX SOUND

// Sound Device selections for X11
typedef enum {
  SD_NULL = 0,
  SD_S1 = 1,
  SD_S2 = 2,
  SD_S3 = 3,
  SD_OSS,
  SD_ESD,
  SD_ALSA,
  SD_PULSE,
  SD_JACK,
  SD_DEV6,
  SD_DEV7,
  SD_DEV8,
  SD_DEV9,
} sound_dev_e;

// Detect optional devices.
// Detect multiple sound devices.
//  SOUND_DEV1  when 1 DEV
//  SOUND_DEVICE_OPTION  when 2 or more DEV

#ifdef DEV_OSS
#  if DEV_OSS == 3
#    define DEV_OPT_OSS
     // OSS does not have dynamic libraries
#  endif
#  define SOUND_DEV1  SD_OSS
#endif

#ifdef DEV_ALSA
#  if DEV_ALSA == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_ALSA
#    else
#      undef DEV_ALSA
#    endif
#  endif
#endif
#ifdef DEV_ALSA
#  ifdef SOUND_DEV1
#    ifndef SOUND_DEVICE_OPTION
#      define SOUND_DEVICE_OPTION
#    endif
#  else
#    define SOUND_DEV1  SD_ALSA
#  endif
#endif

#ifdef DEV_ESD
#  if DEV_ESD == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_ESD
#    else
#      undef DEV_ESD
#    endif
#  endif
#endif
#ifdef DEV_ESD
#  ifdef SOUND_DEV1
#    ifndef SOUND_DEVICE_OPTION
#      define SOUND_DEVICE_OPTION
#    endif
#  else
#    define SOUND_DEV1  SD_ESD
#  endif
#endif

#ifdef DEV_JACK
#  if DEV_JACK == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_JACK
#    else
#      undef DEV_JACK
#    endif
#  endif
#endif
#ifdef DEV_JACK
#  ifdef SOUND_DEV1
#    ifndef SOUND_DEVICE_OPTION
#      define SOUND_DEVICE_OPTION
#    endif
#  else
#    define SOUND_DEV1  SD_JACK
#  endif
#endif

#ifdef DEV_PULSE
#  if DEV_PULSE == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_PULSE
#    else
#      undef DEV_PULSE
#    endif
#  endif
#endif
#ifdef DEV_PULSE
#  ifdef SOUND_DEV1
#    ifndef SOUND_DEVICE_OPTION
#      define SOUND_DEVICE_OPTION
#    endif
#  else
#    define SOUND_DEV1  SD_PULSE
#  endif
#endif


// Sound device selection menu and control.

#ifdef SOUND_DEVICE_OPTION

// The values of snd_opt are defined in linux_x/i_sound.c
// to ensure that menu values and implementation always match.
extern CV_PossibleValue_t snd_opt_cons_t[];
// The actual consvar are declared in s_sound.c
extern consvar_t cv_snd_opt;

#else
// Static sound device selection
// Without SOUND_DEVICE_OPTION, there can only be one sound device.

// End Static sound device selection
#endif

#ifdef SNDSERV
extern consvar_t cv_sndserver_cmd;
extern consvar_t cv_sndserver_arg;
#endif

// ============= MUSIC

// Midi Music Device selections for X11
typedef enum {
  MDT_NULL = 0,
  MDT_SEARCH1 = 1,
  MDT_SEARCH2 = 2,
// MIDI devices
  MDT_MIDI,   // any MIDI
  MDT_TIMIDITY,
  MDT_FLUIDSYNTH,
  MDT_EXTMIDI,
// SYNTH devices
  MDT_SYNTH,  // any SYNTH
  MDT_FM_SYNTH,
  MDT_AWE32_SYNTH,
// unused devices
  MDT_DEV7,
  MDT_DEV8,
  MDT_DEV9,
  MDT_QUERY,  // return list of devices
} mus_dev_e;

// Detect multiple music devices.
//  MUS_DEV1  when 1 DEV
//  MUS_DEVICE_OPTION  when 2 or more DEV

#ifdef DEV_TIMIDITY
#  if DEV_TIMIDITY == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_TIMIDITY
#    else
#      undef DEV_TIMIDITY
#    endif
#  endif
#endif
#ifdef DEV_TIMIDITY
#  ifdef MUS_DEV1
#    ifndef MUS_DEVICE_OPTION
#      define MUS_DEVICE_OPTION
#    endif
#  else
#    define MUS_DEV1  MDT_TIMIDITY
#  endif
#endif

#ifdef DEV_FLUIDSYNTH
#  if DEV_FLUIDSYNTH == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_FLUIDSYNTH
#    else
#      undef DEV_FLUIDSYNTH
#    endif
#  endif
#endif
#ifdef DEV_FLUIDSYNTH
#  ifdef MUS_DEV1
#    ifndef MUS_DEVICE_OPTION
#      define MUS_DEVICE_OPTION
#    endif
#  else
#    define MUS_DEV1  MDT_FLUIDSYNTH
#  endif
#endif

#ifdef DEV_FM_SYNTH
#  if DEV_FM_SYNTH == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_FM_SYNTH
#    else
#      undef DEV_OPT_FM_SYNTH
#    endif
#  endif
#endif
#ifdef DEV_FM_SYNTH
#  ifdef MUS_DEV1
#    ifndef MUS_DEVICE_OPTION
#      define MUS_DEVICE_OPTION
#    endif
#  else
#    define MUS_DEV1  MDT_FM_SYNTH
#  endif
#endif

#ifdef DEV_AWE32_SYNTH
#  if DEV_AWE32_SYNTH == 3
#    define NEED_DLOPEN
#    ifdef HAVE_DLOPEN
#      define DEV_OPT_AWE32_SYNTH
#    else
#      undef DEV_AWE32_SYNTH
#    endif
#  endif
#endif
#ifdef DEV_AWE32_SYNTH
#  ifdef MUS_DEV1
#    ifndef MUS_DEVICE_OPTION
#      define MUS_DEVICE_OPTION
#    endif
#  else
#    define MUS_DEV1  MDT_AWE32_SYNTH
#  endif
#endif

#ifdef MUSSERV
// The values of musserv_opt are defined in linux_x/i_sound.c
// to ensure that menu values and implementation always match.
extern CV_PossibleValue_t musserv_opt_cons_t[];
// The actual consvar are declared in s_sound.c
extern consvar_t cv_musserver_opt;

extern consvar_t cv_musserver_cmd;
extern consvar_t cv_musserver_arg;
#endif

#endif
