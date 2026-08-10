// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_cd.c 1264 2016-09-20 17:23:11Z wesleyjohnson $
//
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: win_cd.c,v $
// Revision 1.3  2000/09/01 19:34:38  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      cd music interface (uses MCI).
//
//-----------------------------------------------------------------------------

// Because of WINVER redefine, doomtype.h (via doomincl.h) is before any
// other include that might define WINVER
#include "doomincl.h"

#include "win_main.h"
#include <mmsystem.h>

#include "command.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_system.h"

#include "s_sound.h"

#define MAX_CD_TRACKS       100

typedef struct {
    BOOL    IsAudio;
    DWORD   Start, End;
    DWORD   Length;         // minutes
} CDTrack_t;

// -------
// private
// -------
static  CDTrack_t           track_stack[MAX_CD_TRACKS];
static  int                 num_tracks;  // up to MAX_CD_TRACKS
static  MCI_STATUS_PARMS    mci_status;
static  MCI_OPEN_PARMS      mci_open;

// ------
// protos
// ------
static void Command_Cd_f (void);
static void I_StopCD (void);
static int I_SetVolumeCD (int volume);


// -------------------
// MCIErrorMessageBox
// Retrieve error message corresponding to return value from
//  mciSendCommand() or mciSenString()
// -------------------
static void CD_MCI_ErrorMessageBox (MCIERROR error_code)
{
    char errtext[128];
    if (!mciGetErrorString (error_code, errtext, sizeof(errtext)))
        wsprintf(errtext,"MCI CD Audio Unknown Error #%d\n", error_code);
    GenPrintf(EMSG_error, errtext);
    /*MessageBox (GetActiveWindow(), szTemp+1, "LEGACY",
                MB_OK | MB_ICONSTOP );*/
}


// --------
// CD_Reset
// --------
static void CD_Reset (void)
{
    // no win32 equivalent
    //faB: for DOS, some odd drivers like to be reset sometimes.. useless in MCI I guess
}


// ----------------
// CD_ReadTrackInfo
// Read in number of tracks, and length of each track in minutes/seconds
// returns true if error
// ----------------
static BOOL CD_ReadTrackInfo (void)
{
    int         i;
    int         track_len;
    MCIERROR    merr;
    
    num_tracks = 0;
                
    mci_status.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    if ( (merr = mciSendCommand(mci_open.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM|MCI_WAIT, (DWORD)(LPVOID)&mci_status)) )
    {
        CD_MCI_ErrorMessageBox (merr);
        return FALSE;
    }
    num_tracks = mci_status.dwReturn;
    if (num_tracks > MAX_CD_TRACKS)
        num_tracks = MAX_CD_TRACKS;

    for (i=0; i<num_tracks; i++)
    {
        mci_status.dwTrack = i+1;
        mci_status.dwItem = MCI_STATUS_LENGTH;
        if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_STATUS, MCI_TRACK|MCI_STATUS_ITEM|MCI_WAIT, (DWORD)(LPVOID)&mci_status)))
        {
            CD_MCI_ErrorMessageBox (merr);
            return FALSE;
        }
        track_len = (DWORD)(MCI_MSF_MINUTE(mci_status.dwReturn)*60 + MCI_MSF_SECOND(mci_status.dwReturn));
        track_stack[i].Length = track_len;

        mci_status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
        if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_STATUS, MCI_TRACK|MCI_STATUS_ITEM|MCI_WAIT, (DWORD)(LPVOID)&mci_status)))
        {
            CD_MCI_ErrorMessageBox (merr);
            return FALSE;
        }
        track_stack[i].IsAudio = (mci_status.dwReturn == MCI_CDA_TRACK_AUDIO);
    }
        
    return TRUE;
}


// ------------
// CD_TotalTime
// returns total time for all audio tracks in seconds
// ------------
static int CD_TotalTime (void)
{
    int total_len = 0;  // total of all tracks
    int track;
    for (track=0; track<num_tracks; track++)
    {
        if (track_stack[track].IsAudio)
             total_len += track_stack[track].Length;
    }
    return total_len;
}


//======================================================================
//                   CD AUDIO MUSIC SUBSYSTEM
//======================================================================

byte   cdaudio_started=0;   // for system startup/shutdown

static boolean cdPlaying = false;
static int     cdPlayTrack;         // when cdPlaying is true
static boolean cdLooping = false;
static byte    cdRemap[MAX_CD_TRACKS];
static boolean cdEnabled=true;      // cd info available
static boolean cdValid;             // true when last cd audio info was ok
static boolean cd_was_playing;   // was playing, then stopped
//static int     cdVolume=0;          // current cd volume (0-31)

// 0-31 like Music & Sfx, though CD hardware volume is 0-255.
consvar_t   cd_volume = {"cd_volume","31",CV_SAVE,soundvolume_cons_t};

// allow Update for next/loop track
// some crap cd drivers take up to
// a second for a simple 'busy' check..
// (on those Update can be disabled)
consvar_t   cd_update  = {"cd_update","1",CV_SAVE};

// time in hour,minutes,seconds
static char* time_hms(int seconds)
{
    int hours, minutes;
    static char s[9];

    minutes = seconds / 60;
    seconds %= 60;
    hours = minutes / 60;
    minutes %= 60;
    if (hours>0)
        sprintf (s, "%d:%02d:%02d", hours, minutes, seconds);
    else
        sprintf (s, "%2d:%02d", minutes, seconds);
    return s;
}

// console command handler
static void Command_Cd_f (void)
{
    char*     s;
    int       i,j;

    if (!cdaudio_started)
        return;

    if (COM_Argc()<2)
    {
        CONS_Printf ("cd [on] [off] [remap] [reset] [open]\n"
                     "   [info] [play <track>] [loop <track>]\n"
		     "   [stop] [pause] [resume]\n");
        return;
    }

    s = COM_Argv(1);

    // activate cd music
    if (!strncmp(s,"on",2))
    {
        cdEnabled = true;
        return;
    }

    // stop/deactivate cd music
    if (!strncmp(s,"off",3))
    {
        if (cdPlaying)
            I_StopCD ();
        cdEnabled = false;
        return;
    }

    // remap tracks
    if (!strncmp(s,"remap",5))
    {
        i = COM_Argc() - 2;
        if (i <= 0)
        {
            CONS_Printf ("CD tracks remapped in that order :\n");
            for (j = 1; j < MAX_CD_TRACKS; j++)
                if (cdRemap[j] != j)
                    CONS_Printf (" %2d -> %2d\n", j, cdRemap[j]);
            return;
        }
        for (j = 1; j <= i; j++)
            cdRemap[j] = atoi (COM_Argv (j+1));
        return;
    }

    // reset the CD driver, useful on some odd cd's
    if (!strncmp(s,"reset",5))
    {
        cdEnabled = true;
        if (cdPlaying)
            I_StopCD ();
        for (i = 0; i < MAX_CD_TRACKS; i++)
            cdRemap[i] = i;
        CD_Reset();
        cdValid = CD_ReadTrackInfo();
        return;
    }

    // any other command is not allowed until we could retrieve cd information
    if (!cdValid)
    {
        CONS_Printf ("CD is not ready.\n");
        return;
    }

    /* faB: not with MCI, didn't find it, useless anyway
    if (!strncmp(s,"open",4))
    {
        if (cdPlaying)
            I_StopCD ();
        bcd_open_door();
        cdValid = false;
        return;
    }*/

    if (!strncmp(s,"info",4))
    {
        if (!CD_ReadTrackInfo()) {
            cdValid = false;
            return;
        }

        cdValid = true;

        if (num_tracks <= 0)
            CONS_Printf ("No audio tracks\n");
        else
        {
            // display list of tracks
            // highlight current playing track
            for (i = 0; i<num_tracks; i++)
            {
                CONS_Printf    ("%s%2d. %s  %s\n",
                                cdPlaying && (cdPlayTrack == i) ? "\2 " : " ",
                                i+1, track_stack[i].IsAudio ? "audio" : "data ",
                                time_hms(track_stack[i].Length));
            }
            CONS_Printf ("\2Total time : %s\n", time_hms(CD_TotalTime()));
        }
        if (cdPlaying)
        {
            CONS_Printf ("%s track : %d\n", cdLooping ? "looping" : "playing",
                         cdPlayTrack);
        }
        return;
    }

    if (!strncmp(s,"play",4))
    {
        I_PlayCD (atoi (COM_Argv (2)), false);
        return;
    }

    if (!strncmp(s,"stop",4))
    {
        I_StopCD ();
        return;
    }

    if (!strncmp(s,"loop",4))
    {
        I_PlayCD (atoi (COM_Argv (2)), true);
        return;
    }

    if (!strncmp(s, "pause", 5)) {
	I_PauseCD();
	return;
    }

    if (!strncmp(s,"resume",4))
    {
        I_ResumeCD ();
        return;
    }

    CONS_Printf ("cd command '%s' unknown\n", s);
}


// ------------
// I_ShutdownCD
// Shutdown CD Audio subsystem, release whatever was allocated
// ------------
void I_ShutdownCD (void)
{
    MCIERROR    merr;

    if (!cdaudio_started)
        return;

    CONS_Printf ("I_ShutdownCD()\n");

    I_StopCD();

    // closes MCI CD
    if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_CLOSE, 0, (DWORD)NULL)))
        CD_MCI_ErrorMessageBox (merr);
}


// --------
// I_InitCD
// Init CD Audio subsystem
// --------
// Interface i_sound.h
void I_InitCD (void)
{
    MCI_SET_PARMS   mci_set;
    MCIERROR    merr;
    int         i;
    
    // We don't have an open device yet
    mci_open.wDeviceID = 0;
    num_tracks = 0;

    cdaudio_started = false;

    mci_open.lpstrDeviceType = (LPCTSTR)MCI_DEVTYPE_CD_AUDIO;
    if ((merr = mciSendCommand((MCIDEVICEID)NULL, MCI_OPEN, MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID, (DWORD)&mci_open)))
    {
        CD_MCI_ErrorMessageBox (merr);
        return;
    }

        // Set the time format to track/minute/second/frame (TMSF).
    mci_set.dwTimeFormat = MCI_FORMAT_TMSF;
    if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mci_set)))
    {
        CD_MCI_ErrorMessageBox (merr);
        mciSendCommand(mci_open.wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
        return;
    }  

    I_AddExitFunc (I_ShutdownCD);
    cdaudio_started = true;

    CONS_Printf ("I_InitCD: CD Audio started\n");

    // last saved in config.cfg
    i = cd_volume.value;
    //I_SetVolumeCD (0);   // initialize to 0 for some odd cd drivers
    I_SetVolumeCD (i);   // now set the last saved volume

    for (i = 0; i < MAX_CD_TRACKS; i++)
        cdRemap[i] = i;

    if (!CD_ReadTrackInfo())
    {
        CONS_Printf("\2I_InitCD: no CD in player.\n");
        cdEnabled = false;
        cdValid = false;
    }
    else
    {
        cdEnabled = true;
        cdValid = true;
    }
    
    COM_AddCommand ("cd", Command_Cd_f);
}



// loop/go to next track when track is finished (if cd_update var is true)
// update the volume when it has changed (from console/menu)
// TODO: check for cd change and restart music ?
//
// Interface i_sound.h
void I_UpdateCD (void)
{
        
}


//
// Interface i_sound.h
void I_PlayCD (unsigned int track, boolean looping)
{
    MCI_PLAY_PARMS  mci_play;
    MCIERROR        merr;
        
    if (!cdaudio_started || !cdEnabled)
        return;

    //faB: try again if it didn't work (just free the user of typing 'cd reset' command)
    if (!cdValid)
        cdValid = CD_ReadTrackInfo();
    if (!cdValid)
        return;

    // tracks start at 0 in the code..
    track--;
    if (track < 0 || track >= num_tracks)
        track = track % num_tracks;

    track = cdRemap[track];

    if (cdPlaying)
    {
        if (cdPlayTrack == track)
            return;
        I_StopCD ();
    }
    
    cdPlayTrack = track;

    if (!track_stack[track].IsAudio)
    {
        CONS_Printf ("\2CD Play: not an audio track\n");
        return;
    }

    cdLooping = looping;

    //faB: stop MIDI music, MIDI music will restart if volume is upped later
    cv_musicvolume.value = 0;
    I_StopSong (0);

    //faB: I don't use the notify message, I'm trying to minimize the delay
    mci_play.dwCallback = (DWORD)hWnd_main;
    mci_play.dwFrom = MCI_MAKE_TMSF(track+1, 0, 0, 0);
    if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_PLAY, MCI_FROM|MCI_NOTIFY, (DWORD)&mci_play)))
    {
        CD_MCI_ErrorMessageBox (merr);
        cdValid = false;
        cdPlaying = false;
        return;
    }

    cdPlaying = true;
    cd_was_playing = false;
}


// pause cd music
static void I_StopCD (void)
{
    MCIERROR    merr;

    if (!cdaudio_started || !cdEnabled)
        return;

    if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD)NULL)))
        CD_MCI_ErrorMessageBox (merr);
    else {
        cd_was_playing = cdPlaying;
        cdPlaying = false;
    }
}


// Interface i_sound.h
void I_PauseCD (void)
{
    MCIERROR    merr;

    if (!cdaudio_started || !cdEnabled)
        return;

    if (!cdValid)
        return;

    if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD)NULL)))
        CD_MCI_ErrorMessageBox (merr);
    else {
        cd_was_playing = cdPlaying;
        cdPlaying = false;
    }
}


// continue after a pause
// Interface i_sound.h
void I_ResumeCD (void)
{
    MCIERROR    merr;

    if (!cdaudio_started || !cdEnabled)
        return;

    if (!cdValid)
        return;

    if (!cd_was_playing)
        return;

    if ((merr = mciSendCommand(mci_open.wDeviceID, MCI_RESUME, MCI_WAIT, (DWORD)NULL)))
        CD_MCI_ErrorMessageBox (merr);
    else
        cdPlaying = true;
}


// volume : logical cd audio volume 0-31 (hardware is 0-255)
static int I_SetVolumeCD (int volume)
{
    return 1;
}
