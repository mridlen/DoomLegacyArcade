// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: soundsrv.h 1577 2021-02-25 03:45:20Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: soundsrv.h,v $
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      UNIX soundserver, separate process. 
//
//-----------------------------------------------------------------------------

#ifndef SNDSERVER_H
#define SNDSERVER_H

// External Interface.

// Device Selection.
// Command: 'd'
// byte  device_selection;  // snd_opt_cons_t.

// Play sound command.
// Command: 'p'
typedef struct {
    uint16_t  sfxid;
    byte      vol;
    byte      pitch;
    int16_t   sep;   // +/- 127, SURROUND_SEP
    int16_t   priority;  // -10..2560
    uint16_t  handle;
} server_play_sound_t;

// Update sound
// Command: 'r'
typedef struct {
    uint16_t  handle;
    byte      vol;
    byte      pitch;
    int16_t   sep;   // +/- 127, SURROUND_SEP
} server_update_sound_t;

// Stop Sound
// Command: 's'
// uint16_t  handle;

// Load Sound
// Command: 'l'
typedef struct {
    uint16_t id;
    uint32_t flags;
    uint32_t snd_len;
} server_load_sound_t;

// Volume control
// Command: 'v'
// byte  volume

// Server Quit
// Command: 'q'


// ------ Sound driver interface
// These are only a few of the fields from sfxinfo_t.
typedef struct {
    int32_t   length;
    uint32_t  flags;
    byte *    data;
} server_sfx_t;


#endif
