// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: musseq.h 1294 2021-03-4 wesleyjohnson $
//
// Copyright (C) 1996-2016 by DooM Legacy Team.
//   GNU General Public License
//   Heavily modified for use with Doom Legacy.
//   Removed wad search and Doom version dependencies.
//   Is now dependent upon IPC msgs from the Doom program
//   for all wad information, and the music lump id.

/*
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
 */

#ifndef MUSSEQ_H
#define MUSSEQ_H

// Local definitions from musserver.h, moved here.
// These are private to the musserver.

#include "doomdef.h"
  // MAX_WADPATH

#include "doomtype.h"
  // byte

#include <stdint.h>


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

// FM SYNTH structs
// Defined by wad, instead of by soundcard, or OSS.
typedef byte  sbi_instr_wad_t[32];
  // Identical to sbi_instr_data of soundcard.h.

// Defined by wad.
typedef struct {
    uint16_t   flags;
    byte   finetune;
    byte   note;
    byte   pat;
    sbi_instr_wad_t    patchdata;
} opl_instr_t;


// Music structs
typedef struct {
  uint32_t  filepos;  // position in file
  uint32_t  size;  // music data size
  byte *  data;  // music data, malloc
} music_data_t;

typedef struct {
  char * wad_name;  // malloc
  int  lumpnum;
  byte state;   // play_e
} music_wad_t;


// from IPC message
extern char * option_string;
extern int qid;  // IPC message queue id

// Command state
extern byte option_pending;  // msg has set the option string
extern byte continuous_looping;
extern byte music_paused;
extern byte verbose;
extern byte changevol_allowed;
extern byte no_devices_exit;
extern byte parent_check;  // check parent process
extern char parent_proc[32];  // parent process /proc/num


// Musserver interface

// Exit the program.
void cleanup_exit(int status, char * exit_msg);


// Sequencer interface
extern uint32_t  seq_error_count;


//  channel :  0..15
extern void (*note_on)(int note, int channel, int volume);
extern void (*note_off)(int note, int channel, int volume);
//#define PITCHBEND_128
extern void (*pitch_bend)(int channel, int value);
extern void (*control_change)(int controller, int channel, int value);
extern void (*patch_change)(int patch, int channel);
extern void (*midi_wait)( uint32_t wtime );
extern int  (*get_queue_avail)( void );
extern byte (*device_playing)( void );

// Music midi timer.
typedef enum { MMT_START, MMT_STOP, MMT_CONT }  mmt_e;
// action : mmt_e
void midi_timer(byte action);

void all_off_midi(void);
void pause_midi(void);
void reset_midi(void);
void volume_change( int channel, int volume );
void master_volume_change(int volume);

// Init, load, setup the selected device
void seq_init_setup( byte sel_snddev, byte sel_dev, int dev_type, int port_num);
void seq_shutdown(void);
int list_devs(void);

// Playmus interface

void playmus(music_data_t * music_data, byte check_msg);

enum{ MSG_NOWAIT=0, MSG_WAIT=1 };
//  wait_flag : MSG_WAIT, MSG_NOWAIT
void get_mesg(byte wait_flag);

void restart_playing( void );


// ==== Read Wad Interface

extern music_wad_t  music_lump;
extern music_wad_t  genmidi_lump;

// Wad read
// Read the GENMIDI lump from a wad.
//  gen_wad : the wad name and lumpnum
void read_wad_genmidi( music_wad_t * gen_wad );

// Return music size.
//  music_wad : the wad name and lumpnum
//  music_data : the music lump read
int read_wad_music( music_wad_t * music_wad,
             /* OUT */  music_data_t * music_data );

void release_music_data( music_data_t * music_data );

#endif

