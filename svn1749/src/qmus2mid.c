// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: qmus2mid.c 1648 2023-02-10 13:23:27Z wesleyjohnson $
//
// Copyright (C) 1995 by Sebastien Bacquet.
// Portions Copyright (C) 1998-2017 by DooM Legacy Team.
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
// $Log: qmus2mid.c,v $
// Revision 1.6  2001/03/09 21:53:56  metzgermeister
//
// Revision 1.5  2001/03/03 19:45:12  ydario
// Do not compile under OS/2
//
// Revision 1.4  2000/10/08 13:30:01  bpereira
// Revision 1.3  2000/09/10 10:46:15  metzgermeister
// merged with SDL version
//
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      convert Doom MUS data to a MIDI music data
//      used by both DOS/WIN32
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "qmus2mid.h"
#include "w_wad.h"
#include "z_zone.h"

//defined OS2_MIDI_FILE_TO_FILE
#if defined __OS2__ && defined OS2_MIDI_FILE_TO_FILE
// OS2 is using a file to file version of qmus, in os2/qmus2mid2.c

// Call this function to load all music lumps.
// OS2 does not need to consider big endian.
void* S_CacheMusicLump(int lump)
{
    return W_CacheLumpNum(lump, PU_MUSIC);
}

#else

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "i_system.h"
#include "byteptr.h"
#include "m_swap.h"

// Controls for questionable options.
byte  EN_create_program = 0;
byte  EN_midi_compress = 1;


// MUS events
#define MUS_EV_SCOREEND     6

// MIDI
#define MIDIHEADER    "MThd\000\000\000\006\000\001"
  // length 6, format 1
#define MIDICREATEPROG  "\000\377\002\026"
  // 0x00, 0xff, 0x02, 0x16
#define MIDIKEY  "\000\377\131\002\000\000"
  // 0x00, 0xff, 0x59, 0x02, 0x00 0x00    // C major
#define MIDITEMPO  "\000\377\121\003\011\243\032"
  // 0x00, 0xff, 0x51, 0x03, 0x09, 0xa3, 0x1a   // usec/quarter_note
#define MIDIEND  "\000\377\057\000"
  // 0x00, 0xff, 0x2f, 0x00    // end of track, header
#define MIDITRACKSRC  "\000\377\003\035"
  // 0x00, 0xff, 0x03, 0x1d

struct MUSheader_s
{
    char          ID[4];            // identifier "MUS" 0x1A
    uint16_t      scoreLength;      // length of score in bytes
    uint16_t      scoreStart;       // absolute file pos of the score
    uint16_t      channels;         // count of primary channels
    uint16_t      sec_channels;      // count of secondary channels
    uint16_t      instrCnt;
    uint16_t      dummy;
    // variable-length part starts here
    uint16_t      instruments[0];
};
typedef struct MUSheader_s MUSheader;


// Call this function to load all music lumps.
// Handle endian at lump load time, avoiding excessive byteswapping.
// Placed here because this file knows the header format, and music lumps
// are only loaded every few minutes, not hundred times a second.
// Function from ppc bug report, by Adam Stylinski.
// Modified by WDJ.
void* S_CacheMusicLump(int lump)
{
#ifdef __BIG_ENDIAN__
    // Cache Music Lump Endian
    void *data = W_CacheLumpNum(lump, PU_MUSIC); 
    if (lump_read && memcmp(data, MUSHEADER, 4) == 0) {
        /* Header should be at beginning of opaque data pointer */
        MUSheader *mh = (MUSheader*)data;
        mh->scoreLength = (uint16_t)( LE_SWAP16(mh->scoreLength) );
        mh->scoreStart = (uint16_t)( LE_SWAP16(mh->scoreStart) );
        mh->channels = (uint16_t)( LE_SWAP16(mh->channels) );
        mh->sec_channels = (uint16_t)( LE_SWAP16(mh->sec_channels) );
        mh->instrCnt = (uint16_t)( LE_SWAP16(mh->instrCnt) );
        /* Probably unnecessary if this is just padding */
        mh->dummy = (uint16_t)( LE_SWAP16(mh->dummy) );

        uint16_t i;
        for (i = 0; i < mh->instrCnt; ++i) {
            mh->instruments[i] = (uint16_t)( LE_SWAP16(mh->instruments[i]) );
        }
    }

    return data;
#else
    return W_CacheLumpNum(lump, PU_MUSIC);
#endif
}



// internal
typedef struct Track
{
    // [WDJ] Arranged for better packing.
    byte      *data;     // Track MIDI data
    uint32_t  current;   // current buffer position
    uint32_t  limit;     // write buffer limit
    uint32_t  DeltaTime;
    byte      vel;
    byte      LastEvent; // test last event, for compression
} track_t;

static track_t  track[16];

static byte  MUS2MIDcontrol[15] =
{
        0,                  /* Program change - not a MIDI control change */
        0x00,               /* Bank select */
        0x01,               /* Modulation pot */
        0x07,               /* Volume */
        0x0A,               /* Pan pot */
        0x0B,               /* Expression pot */
        0x5B,               /* Reverb depth */
        0x5D,               /* Chorus depth */
        0x40,               /* Sustain pedal */
        0x43,               /* Soft pedal */
        0x78,               /* All sounds off */
        0x7B,               /* All notes off */
        0x7E,               /* Mono */
        0x7F,               /* Poly */
        0x79                /* Reset all controllers */
};

static byte    track_full;

// [WDJ] Simpler fwritemem
#define fwritemem(p,n,f)  memcpy(*f,p,n);*f+=n

#define fwrite16(x,f)    BE_write_16(f,x)
#define fwrite32(x,f)    BE_write_32(f,x)
// [WDJ] Proper big-endian midi read/write
static void BE_write_16(byte **p, int16_t val)
{
  *(int16_t *)*p = BE_SWAP16_FAST(val);
  *p += sizeof(int16_t);
}

static void BE_write_32(byte **p, int32_t val)
{
  *(int32_t *)*p = BE_SWAP32_FAST(val);
  *p += sizeof(int32_t);
}

#define last(e)         ((byte)(e & 0x80))
#define event_type(e)   ((byte)((e & 0x7F)>>4))
#define channel(e)      ((byte)(e & 0x0F))

// [WDJ] The original qmus2midi tested for -1 returned by getc().
// The MUS format in a lump does not have -1 terminator.
// This test has been removed everywhere.


static void FreeTracks ( void )
{
    int i;

    for (i = 0; i < 16; i++ )
    {
        if (track[i].data )
            free( track[i].data );
    }
}


// [WDJ] buffersize will now grow dynamically.
// Only a few music tracks will realloc.
// A few with one track at 21K, a 39K, a 45K.
#define TRACK_BUFFERSIZE      16000
#define TRACK_BUFFERSIZE_INC  16000
#define TRACK_GUARD           16

static void TWriteByte (track_t * trk, byte bbyte)
{
    uint32_t pos;

    pos = trk->current;
    if (pos > trk->limit )
    {
        // Incremental allocation is more reasonable.
        uint32_t  new_size = trk->limit + (TRACK_GUARD + TRACK_BUFFERSIZE_INC);
#if 0
        GenPrintf( EMSG_debug, "Midi track realloc %i\n", new_size);
#endif
        byte *  new_data = realloc( trk->data, new_size );
        if( new_data )
        {
            trk->data = new_data;
            trk->limit = new_size - TRACK_GUARD;
	}
        else
	{
            if ( track_full == 0 )
            {
                I_SoftError("Mus Convert Error : Track buffer full.\n");
                track_full = 1;  // track full
                // finish MIDI event, play what we got.
            }
            if( pos > (trk->limit + (TRACK_GUARD - 1)) )
            {
	        // Do not write
                return;
            }
        }
    }
    trk->data[pos] = bbyte;
    trk->current++;
    return;
}

// write midi format, big endian
// value: all valid bytes have bit8 set except the last byte (LSB)
static void TWriteVarLen ( track_t * trk, uint32_t value)
{
    uint32_t buffer;

    // Shift into buffer last (LSB), to first
    buffer = value & 0x7f;  // last (LSB) has bit8 clear
    while( (value >>= 7) )  // detect valid bytes
    {
        buffer <<= 8;
        buffer |= 0x80;  // not last byte
        buffer |= (value & 0x7f);
    }
    while( 1 )
    {
        TWriteByte( trk, (byte)buffer);
        if ((buffer & 0x80) == 0 )  // detect last byte
            break;
        buffer >>= 8;
    }
}

static int WriteMIDheader( uint16_t ntrks, uint16_t division, byte **file )
{
    fwritemem( MIDIHEADER, 10, file );
    fwrite16( ntrks, file);
    fwrite16( division, file );
    return 0;
}



static void WriteTrack (int tracknum, byte **file)
{
    uint16_t trksize;

    /* Do we risk overflow here ? */
    trksize = (uint16_t)track[tracknum].current + 4;
    fwritemem( "MTrk", 4, file );  // track header

#ifdef TRACK_PAD_33
    // [WDJ] OLD CODE. Track pad, unknown usage.
    // Disabled. Not needed by Linux or Modern windows.
    // Win10, Win11: Causes track to be delayed by minutes, and changed to piano.
    if (tracknum == 0)
        trksize += 33;
#endif

    fwrite32( trksize, file );

#ifdef TRACK_PAD_33
    // [WDJ] OLD CODE. Track pad, unknown reasons.
    if (tracknum == 0)
    {
        memset(*file,'\0',33);
        *file+=33;
    }
#endif

#ifdef TRACK_4KBLOCK
    // [WDJ] OLD CODE. Have no idea why writing track out this way was done.
    // Disabled, has no effect on memcpy.
    size_t quot, rem, quot_blk;
    quot = (size_t) (track[tracknum].current / 4096);
    quot_blk = quot * 4096;
    rem = (size_t) (track[tracknum].current - quot_blk);
    fwritemem (track[tracknum].data, quot_blk, file );
    fwritemem (((byte *) track[tracknum].data) + quot_blk, rem, file );
#else
    // [WDJ] Write it out in one operation.
    fwritemem( track[tracknum].data, track[tracknum].current, file );
#endif
    fwritemem( MIDIEND, 4, file );
}


static void WriteFirstTrack (byte **file)
{
    uint16_t trksize = 17;
    if( EN_create_program )
        trksize += 4 + 22;

    fwritemem( "MTrk", 4, file );  // track header
    fwrite32( trksize, file );

    if( EN_create_program )
    {
        // [WDJ] OLD CODE, OPTIONAL. Create_program command.  Not in PrBoom.
	// Seems to have no effect.
        fwritemem( MIDICREATEPROG , 4, file );
        memset(*file,'\0',22);  // create prog string
        *file+=22;
    }

    fwritemem( MIDIKEY, 6, file );
    fwritemem( MIDITEMPO, 7, file );
    fwritemem( MIDIEND, 4, file );
}

// Read a MUS time from file
static uint32_t ReadTime( byte **musfp )
{
    uint32_t timev = 0;
    byte bbyte;

    // MUS time is variable length, last byte has bit8=0
    do
    {
        bbyte = *(*musfp)++;
        timev = (timev << 7) + (bbyte & 0x7F);
    } while( bbyte & 0x80 );

    return timev;
}

// return first MIDI channel available, except percussion channel 9
// fixed ch assign for percussion: MUS ch15 -> midi ch9
static byte FirstChannelAvailable (byte MUS2MIDchannel[])
{
    int i;
    int maxch = -1;  // so first is 0

    // note: skip channel 15 which is percussions
    for (i = 0; i < 15; i++ )
    {
        // Channel 0xFF is unused.
        byte mch = MUS2MIDchannel[i];
        if( mch == 0xFF )  continue;  // unused

        if ( (int)mch > maxch )  // find max of assigned ch
            maxch = mch;
    }

    maxch ++; // next channel is free
    // MIDI channel 9 is used for percussions
    return (maxch == 9 ? 10 : maxch);
}

// write MIDI event code, with compression
static void MidiEvent(track_t * trk, byte newevent)
{
    // compression does not repeat same event
    if( EN_midi_compress && (newevent == trk->LastEvent) )
        return;

    trk->LastEvent = newevent;
    TWriteByte( trk, newevent );
}


// Convert MUS to MIDI
// Return QMUS_error_code_e, 0 on success
int qmus2mid (byte *mus,  // input mus
              int muslength,  // input mus length
              uint16_t division, // ticks per quarter note
              int nocomp,     // no compression, is ignored
              int midbuffersize, // output buffer length
    /*INOUT*/
              byte *mid,  // output buffer in memory
              unsigned long* midilength)    //faB: returns midi file length in here
{
  static   MUSheader* MUSh;

    byte*    file_mus = mus;
    byte*    file_mid = mid;     // pointer into output buffer
    byte*    musend = mus + muslength;

    byte     MUSchannel;
    byte     MUS2MIDchannel[16];
    byte     MIDIchan2track[16];
    byte     TrackCnt=0;
    byte     MIDIchannel;
    byte     MIDItrack;
    track_t* trk;

    uint32_t DeltaTime;
//    uint32_t TotalTime=0;

    int      i;
    unsigned int  event;
    byte     data;
    byte     et;

    track_full = 0; // global abort flag

    MUSh = (MUSheader *)mus;
    if (memcmp(MUSh->ID, MUSHEADER, 4))
        return QM_NOTMUSFILE;

    file_mus = mus + MUSh->scoreStart;

    if (MUSh->channels > 15)      /* <=> MUSchannels+drums > 16 */
        return QM_TOOMCHAN;

    for (i=0; i<16; i++)
    {
        MUS2MIDchannel[i] = 0xFF;  // channel not used (yet)
        track[i].current = 0;
        track[i].vel = 64;
        track[i].DeltaTime = 0;
        track[i].LastEvent = 0xFF;
        track[i].data = NULL;
        track[i].limit = 0;
    }

    if (division == 0)
        division = 89;
        // prboom defaults to 70, all callers pass 64

    while (( file_mus < musend ) && (track_full == 0))
    {
        event = *(file_mus++);
        et = event_type( event );
        if( et == MUS_EV_SCOREEND )    break;

        MUSchannel = channel( event );

        if (MUS2MIDchannel[MUSchannel] == 0xFF)
        {
            // Previously Unused, need to allocate track.
            MIDIchannel = MUS2MIDchannel[MUSchannel] =
                // if percussion use channel 9
                ((MUSchannel == 15) ? 9
              : FirstChannelAvailable (MUS2MIDchannel) );
            MIDItrack = MIDIchan2track[MIDIchannel] = (byte)TrackCnt++;
            if (!(track[MIDItrack].data = (byte *) malloc(TRACK_BUFFERSIZE) ))
            {
                FreeTracks();
                return QM_MEMALLOC;
            }
            track[MIDItrack].limit = TRACK_BUFFERSIZE - TRACK_GUARD;
        }
        else
        {
            MIDIchannel = MUS2MIDchannel[MUSchannel];
            MIDItrack   = MIDIchan2track [MIDIchannel];
        }
        trk = &track[ MIDItrack ];
        TWriteVarLen( trk, trk->DeltaTime );
        trk->DeltaTime = 0;
        switch (et)
        {
         case 0 :  // release note
#ifdef WIN32
            // From prboom
            // killough 10/7/98: Fix noise problems by not allowing compression.
            // [WDJ] Prevent compression of this event.
            trk->LastEvent = 0xFF;
#endif
            MidiEvent( trk, (byte)0x90 | MIDIchannel);
            
            data = *(file_mus++);
            TWriteByte( trk, (byte)(data & 0x7F));
            TWriteByte( trk, 0 );
            break;
            
         case 1 :  // play note
            MidiEvent( trk, (byte)0x90 | MIDIchannel);
            
            data = *(file_mus++);
            TWriteByte( trk, (byte)(data & 0x7F));
            if (data & 0x80)
                trk->vel = (*file_mus++) & 0x7F;
            TWriteByte( trk, track[MIDItrack].vel );
            break;
            
         case 2 :  // bend note
            MidiEvent( trk, (byte)0xE0 | MIDIchannel);
            
            data = *(file_mus++);
            TWriteByte( trk, (data & 1) << 6 );
            TWriteByte( trk, data >> 1 );
            break;
         case 3 :  // sys event
            MidiEvent( trk, (byte)0xB0 | MIDIchannel);
            
            data = *(file_mus++);
            TWriteByte( trk, MUS2MIDcontrol[data] );
            if (data == 12 )
                TWriteByte( trk, MUSh->channels+1 );
            else
                TWriteByte( trk, 0 );
            break;
         case 4 :  // control change
            data = *(file_mus++);
            if (data )
            {
                MidiEvent( trk, (byte)0xB0 | MIDIchannel);
                
                TWriteByte( trk, MUS2MIDcontrol[data] );
            }
            else  /* program change */
            {
                MidiEvent( trk, (byte)0xC0 | MIDIchannel);
            }
            
            data = *(file_mus++);
            TWriteByte( trk, data & 0x7F );
            break;
         case 5 :  // unknown
         case 7 :  // unknown
            FreeTracks();
            return QM_MUSFILECOR;
         case 6 :  // score end
         default : break;
        }

        if (last( event ) )
        {
            DeltaTime = ReadTime( &file_mus );
//            TotalTime += DeltaTime;
            for (i = 0; i < (int) TrackCnt; i++ )
                track[i].DeltaTime += DeltaTime;
        }
    }

#ifndef SMIF_SDL
    for (i = 0; i < 16; i++)
    {
        trk = &track[i];
        if (trk->current && trk->data)
        {
            TWriteByte( trk, (byte)0x00); // midi end of track code
            TWriteByte( trk, (byte)0xFF);
            TWriteByte( trk, (byte)0x2F);
            TWriteByte( trk, (byte)0x00);
        }
    }
#endif

#if 0
    if (et != MUS_EV_SCOREEND)
      GenPrintf(EMSG_warn,"QMUS end without score end\n");
#endif

    // Write out MIDI.
    WriteMIDheader( TrackCnt+1, division, &file_mid );
    WriteFirstTrack( &file_mid );
    for (i = 0; i < (int) TrackCnt; i++ )
        WriteTrack( i, &file_mid );

    if (file_mid > (mid + midbuffersize))
        return QM_MIDTOOLARGE;

    FreeTracks();

    //faB: return length of Midi data
    if( midilength )
        *midilength = (file_mid - mid);

    return 0;
}


#endif // __OS2__
