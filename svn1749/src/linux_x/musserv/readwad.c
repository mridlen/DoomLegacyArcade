// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: readwad.c 1579 2021-05-19 03:42:59Z wesleyjohnson $
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
 *  readwad.c
 *
 *  Copyright (C) 1995-1996 Michael Heasley (mheasley@hmc.edu)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "musserver.h"
#include "musseq.h"
  // stdint.h
#include "m_swap.h"
  // macros



#define WADHEADER 8	/* The wad header is 12 bytes, but we only care about
                           the last 4.  Thus we skip the first 8 */



/* header of music lump */
typedef struct {
    char	id[4];
    uint16_t	music_size;
    uint16_t	header_size;
    uint16_t	channels;
    uint16_t	sec_channels;
    uint16_t	instrnum;
    uint16_t	dummy;
} mus_header_t;


unsigned long genmidipos;

// [WDJ] 1/16/2010 replaced local defs of swap when little-endian (BE_SWAP),
// with opposite LE_SWAP16, LE_SWAP32
// Music in MIDI file is big-endian, but this reads wad headers which
// are little-endian (see w_wad.c).
#if 0
// [WDJ] Old swap code, just in case you don't believe the above stmt.
#ifndef BIGEND
#define LONG(x) (x)
#define SHORT(x) (x)
#else
#define LONG(x) ((long)SwapLONG((unsigned long) (x)))
#define SHORT(x) ((short)SwapSHORT((unsigned short) (x)))
unsigned long SwapLONG(unsigned long x)
{
    return
        (x>>24)
        | ((x>>8) & 0xff00)
        | ((x<<8) & 0xff0000)
        | (x<<24);
}
unsigned short SwapSHORT(unsigned short x)
{
    return
        (x>>8) | (x<<8);
}
#endif
#endif

// Wad header format.
typedef struct
{
    char       identification[4];   // should be "IWAD" or "PWAD"
    uint32_t   numlumps;            // how many resources
    uint32_t   infotableofs;        // the 'directory' of resources
} wadinfo_t;

// Wad directory format.
typedef struct
{
    uint32_t   filepos;             // file offset of the resource
    uint32_t   size;                // size of the resource
    char       name[8];             // name of the resource
} waddir_t;


// [WDJ] To not confuse with the readdir system call
//  lumpinfo : copy of the wad dir entry
// Return file position of the lumpnum in the wad file, 0 when fail.
static
uint32_t  read_wad_dir( FILE * wadfile, char * filename, int lumpnum,
                        /*OUT*/ waddir_t * lumpinfo )
{
    wadinfo_t  header;
    waddir_t   direntry;

    // Check the header
    fseek(wadfile, 0, SEEK_SET);
    fread(&header, sizeof(header), 1, wadfile );
    if( ( strncmp(header.identification, "IWAD", 4) != 0 )
      && ( strncmp(header.identification, "PWAD", 4) != 0 ) )
    {
      printf("musserver: Is not a WAD file, FILE=%s\n.", filename );
      return 0;
    }
    // [WDJ] 1/16/2010 changed from swap when little-endian to LE_SWAP32,
    // wad directory is little-endian (see w_wad.c)
    header.numlumps = LE_SWAP32(header.numlumps);
    header.infotableofs = LE_SWAP32(header.infotableofs);
   
    if( lumpnum > header.numlumps )
    {
      printf("musserver: Bad lump number for the file, lumpnum=%i, FILE=%s\n.", lumpnum, filename );
      return 0;
    }

    // Read wad file directory for the specified lumpnum.
    fseek(wadfile, header.infotableofs + (lumpnum * sizeof(waddir_t)), SEEK_SET);
    fread(&direntry, sizeof(direntry), 1, wadfile);
    direntry.filepos = LE_SWAP32(direntry.filepos);
    direntry.size = LE_SWAP32(direntry.size);

    // Out
    if( lumpinfo )
    {
        *lumpinfo = direntry;
    }

    return direntry.filepos;
}

void release_music_data( music_data_t * music_data )
{
    if( music_data->data )
    {
        free( music_data->data );
        music_data->data = NULL;
    }
}

// Return music size.
//  music_wad : the wad name and lumpnum
//  music_data : the music lump read
// Return music size.
int read_wad_music( music_wad_t * music_wad,
             /* OUT */  music_data_t * music_data )
{
  FILE * wadfile;
  waddir_t  direntry;
  uint32_t  lump_pos;
  
  // [WDJ] The MUS header
  // All other ports are reading this as little-endian.
  mus_header_t   mus_header;

  wadfile = fopen( music_wad->wad_name, "r" );
  if( wadfile == NULL )
      return 0;

  lump_pos = read_wad_dir( wadfile, music_wad->wad_name, music_wad->lumpnum,
                         /*OUT*/ &direntry );
  if (lump_pos == 0)   goto done;
   
  fseek(wadfile, lump_pos, SEEK_SET);
  fread(&mus_header, sizeof(mus_header), 1, wadfile);
  // [WDJ] this is read of MUS lump header, which is little-endian
  // changed from BE_SWAP to LE_SWAP
  mus_header.music_size = LE_SWAP16( mus_header.music_size );
  mus_header.header_size = LE_SWAP16( mus_header.header_size );
//  mus_header.channels = LE_SWAP16( mus_header.channels );
//  mus_header.sec_channels = LE_SWAP16( mus_header.sec_channels );
//  mus_header.instrnum = LE_SWAP16( mus_header.instrnum );

  fseek(wadfile, lump_pos, SEEK_SET);
  fseek(wadfile, mus_header.header_size, SEEK_CUR);

  if( music_data == NULL )   goto done;
  release_music_data( music_data );

  // Read the music lump to the OUT music_data.
  music_data->data = malloc(mus_header.music_size);
  if (music_data->data == NULL)
  {
    fclose( wadfile );
    printf("musserver: could not allocate %d bytes for music data.\n", mus_header.music_size);
    cleanup_exit(2, "");
    // NO RETURN
  }
  music_data->size = mus_header.music_size;
  fread(music_data->data, 1, mus_header.music_size, wadfile);  // the music lump

done:
  fclose( wadfile );

  return mus_header.music_size;
}

#ifdef DEV_FM_SYNTH   
// [WDJ] Defined by wad structs, not by OSS.
extern opl_instr_t  fm_instruments[175];   // sequencer.c

// Read the GENMIDI lump from a wad.
//  gen_wad : the wad name and lumpnum
void read_wad_genmidi( music_wad_t * gen_wad )
{
  FILE * wadfile;
  waddir_t  direntry;
  uint32_t  lump_pos;
  char header[9];

  wadfile = fopen( gen_wad->wad_name, "r" );
  if( wadfile == NULL )   goto genmidi_not_found;

  lump_pos = read_wad_dir( wadfile, gen_wad->wad_name, gen_wad->lumpnum,
                         /*OUT*/ &direntry );
  if(lump_pos == 0)   goto genmidi_not_found;
  if( strcmp(direntry.name, "GENMIDI") != 0 )   goto genmidi_not_found;

  // Read GENMIDI lump
  fseek(wadfile, lump_pos, SEEK_SET);
  fread(&header, 1, 8, wadfile);
  if (strncmp(header, "#OPL_II#", 8) != 0)   goto genmidi_not_found;

  // Read the GENMIDI instruments.
  fread(&fm_instruments, sizeof(opl_instr_t), 175, wadfile);
  gen_wad->state = PLAY_RESTART;

  fclose( wadfile );
  return;

genmidi_not_found:
  fclose( wadfile );
  printf( "GENMIDI wadfile = %s\n", gen_wad->wad_name ); 
  cleanup_exit(1, "could not find GENMIDI entry in wadfile");
  return;
}
#endif


