// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: musserver.h 1579 2021-05-19 03:42:59Z wesleyjohnson $
//
// Copyright (C) 1995-1996 Michael Heasley (mheasley@hmc.edu)
//   GNU General Public License
// Portions Copyright (C) 1996-2016 by DooM Legacy Team.
//   GNU General Public License
//   Heavily modified for use with Doom Legacy.
//   Removed wad search and Doom version dependencies.
//   Is now dependent upon IPC msgs from the Doom program
//   for all wad information, and the music lump id.

/*************************************************************************
 *  musserver.h
 *
 *  Copyright (C) 1995 Michael Heasley (mheasley@hmc.edu)
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

#ifndef MUSSERVER_H


/**************************************************/
/* User-configurable parameters: program defaults */
/**************************************************/


/*************************************************************************
 * Change this to your preferred default playback device: external midi, *
 * FM synth, or AWE32 synth                                              *
 *************************************************************************/

/* #define DEFAULT_EXT_MIDI */
#define DEFAULT_FM_SYNTH
/* #define DEFAULT_AWE32_SYNTH */


/************************************************************************
 * To compile in support for AWE32 synth (requires AWE32 kernel driver, *
 * see README) regardless of the default playback device, define the    *
 * following                                                            *
 ************************************************************************/

/* #define AWE32_SYNTH_SUPPORT */


/***************************************************************************
 * If you normally need the -u command-line switch to specify a particular *
 * device type, uncomment this line and change the type as needed          *
 ***************************************************************************/

/* #define DEFAULT_TYPE 8 */


// A unique key for getting the right queue.
#define MUSSERVER_MSG_KEY  ((key_t)53075)

/************************************/
/* End of user-configurable section */
/************************************/

#include "doomdef.h"
  // MAX_WADPATH
#include "doomtype.h"
  // byte
#include <stdint.h>

#include "../lx_ctrl.h"
  // MUS_DEVICE_OPTION
  // DEV_xx

#ifdef DEFAULT_AWE32_SYNTH
#  define AWE32_SYNTH_SUPPORT
#endif

#ifdef AWE32_SYNTH_SUPPORT
# ifndef DEV_AWE32_SYNTH
#   define DEV_AWE32_SYNTH
# endif
#endif

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

#define MUS_VERSION "1.49_DoomLegacy"


// Length of mtext, which now includes a directory/filename.
#define MUS_MSG_MTEXT_LENGTH    MAX_WADPATH
// This message structure is dictated by message operations msgrcv, msgsnd.
typedef struct
{
    uint32_t  mtype;      /* type of received/sent message */
    char  mtext[MUS_MSG_MTEXT_LENGTH];  /* text of the message */
} mus_msg_t;

#if __GNUC_PREREQ(2,2)
  // Since glibc 2.2 this parameter has been void* as required by SUSv2 an SUSv3.
# define MSGBUF(x)  ((void*)&(x))
#else
  // Previous to glib 2.2
# define MSGBUF(x)  ((struct msgbuf*)&(x))
#endif

// Messages (as text):
// First char is command.
// "O-sA-dT"    // option string
// "Dname"      // print music name
// "Wdoom2.wad" // wad name
// Second char is sub-command.
// "V 23"    // volume,  100 = normal, 120 = loud
// "G 102"  // genmidi lump number
// "S 97"   // start playing lump number, once
// "SC97"   // start playing lump number, continuous looping
// "P 1"    // Pause, 0=off  1=on
// "I 2013" // Parent pid 2013, exit when parent exits
// "X"      // Stop song
// "QQ"     // Quit, Exit musserver.

// Option string:
// "-v"    // verbose
// "-V100" // fixed volume, 100 = normal
// "-sA"   // sound interface  A=ALSA, O=OSS, Z=off
// "-dT"   // music device
//         // T=TiMidity,  F=Fluidsynth
//         // E=Ext MIDI,
//         // F=FM Synth,  A=Awe32 Synth
//         // Z=off
// "-p3"   // port num

#if 0
// music play state, set by IPC messages
typedef enum {
   PLAY_OFF,
   PLAY_START,
   PLAY_RUNNING,
   PLAY_PAUSE,
   PLAY_STOP,
   PLAY_RESTART,
   PLAY_QUITMUS
} play_e;
#endif



#endif
