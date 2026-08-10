// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_cdmus.c 1524 2020-05-09 12:07:09Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: i_cdmus.c,v $
// Revision 1.8  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.7  2000/05/13 19:52:37  metzgermeister
// cd vol jiggle
//
// Revision 1.6  2000/04/28 19:28:00  metzgermeister
// changed to CDROMPLAYMSF for CD music
//
// Revision 1.5  2000/04/07 23:12:38  metzgermeister
// fixed some minor bugs
//
// Revision 1.4  2000/03/28 16:18:42  linuxcub
// Added a command to the Linux sound-server which sets a master volume...
//
// Revision 1.3  2000/03/22 18:53:53  metzgermeister
// Ripped CD code out of Quake and put it here
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      cd music interface
//
//-----------------------------------------------------------------------------


#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <linux/cdrom.h>
#include "doomincl.h"
#include "i_sound.h"
#include "command.h"
#include "m_argv.h"
#include "d_main.h"

// Remap index is level map 1..34, or (episode-1)*9+map 1..36
#define MAX_MAPPING   40
#define MAX_CD_TRACKS 256

static boolean cdValid = false;
static boolean playing = false;
static boolean wasPlaying = false;
static boolean initialized = false;
static boolean cd_enabled = false;
static boolean play_looping = false;
static byte    playTrack; // track being played, 1..n
static byte    maxTrack;
static byte    cdRemap[MAX_MAPPING];
static int     cdvolume = -1;
static time_t  lastchk = 0;
static time_t  play_time = 0;

static int open_cdrom(void);
static void I_StopCD(void);
static void I_SetVolumeCD (int volume);
static void cd_volume_onchange(void);

CV_PossibleValue_t cd_volume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};

consvar_t cd_volume = {"cd_volume","31", CV_SAVE|CV_CALL, cd_volume_cons_t, cd_volume_onchange};
//consvar_t cdUpdate  = {"cd_update","1",CV_SAVE};
consvar_t cv_jigglecdvol = {"jigglecdvolume", "0", CV_SAVE};

static void cd_volume_onchange(void)
{
    I_SetVolumeCD( cd_volume.value ); 
}

static int cd_fd = -1;
static char cd_dev[64] = "/dev/cdrom";


// return 0 when CD TOC has been read
static byte  CDAudio_GetAudioDiskInfo(void)
{
    struct cdrom_tochdr tochdr;
    int ierr;
    char * errstr = NULL;

    cdValid = false;
    if( cd_fd < 0 )  return 1;

    // Read TOC header
    if ( ioctl(cd_fd, CDROMREADTOCHDR, &tochdr) < 0 ) {
        errstr = "CDROM Read TOC";
        goto errexit;
    }
        
    if (tochdr.cdth_trk0 < 1) {
        CONS_Printf("CDAudio_GetAudioDiskInfo: no music tracks\n");
        return 2;
    }

    cdValid = true;
    maxTrack = tochdr.cdth_trk1;

    return 0;

errexit:
    ierr = errno;
    GenPrintf( EMSG_error, "%s: %s\n", errstr, strerror(ierr));
    return 3;
}

static boolean CDAudio_GetStartStop(struct cdrom_msf *msf, int track, struct cdrom_tocentry *entry)
{
    struct cdrom_tocentry endentry;

    // start of track
    msf->cdmsf_min0   = entry->cdte_addr.msf.minute;
    msf->cdmsf_sec0   = entry->cdte_addr.msf.second;
    msf->cdmsf_frame0 = entry->cdte_addr.msf.frame; 

    // read following track
    if(track == maxTrack) {
        endentry.cdte_track = CDROM_LEADOUT;
        endentry.cdte_format = CDROM_MSF;
    }
    else {
        endentry.cdte_track = track+1;
        endentry.cdte_format = CDROM_MSF;
    }
    
    if(ioctl(cd_fd, CDROMREADTOCENTRY, &endentry))
        return false;
        
    // end of track
    msf->cdmsf_min1   = endentry.cdte_addr.msf.minute;
    msf->cdmsf_sec1   = endentry.cdte_addr.msf.second;
    msf->cdmsf_frame1 = endentry.cdte_addr.msf.frame;

    return true;
}

static void I_EjectCD(void)
{
    cdValid = false;
    if (cd_fd < 0)
        return; // no cd init'd

    I_StopCD();

    if ( ioctl(cd_fd, CDROMEJECT) < 0 ) 
        CONS_Printf("CD eject: %s\n", strerror(errno));
}

static void command_CD_f (void)
{
    char    *command;
    int     ret;
    int     n;

    if (COM_Argc() < 2) {
        CONS_Printf ("cd [on] [off] [remap] [reset] [open]\n"
                     "   [info] [play <track>] [resume]\n"
                     "   [stop] [pause] [loop <track>]\n");
        return;
    }

    command = COM_Argv (1);

    if (!strncmp(command, "on", 2)) {
            cd_enabled = true;
            return;
    }

    if (!strncmp(command, "off", 3)) {
            I_ShutdownCD();  // so can change CD disc
            return;
    }
    
    if (!strncmp(command, "remap", 5)) {
        ret = COM_Argc() - 2;
        if (ret <= 0) {
            for (n = 1; n < MAX_MAPPING; n++)
                if (cdRemap[n] != (n+1))
                    CONS_Printf("  %u -> %u\n", n, cdRemap[n]);
            return;
        }
        for (n = 1; n <= ret; n++)
            cdRemap[n] = atoi(COM_Argv (n+1));
        return;
    }
        
    if (!strncmp(command, "reset", 5)) {
        cd_enabled = true;
        I_StopCD();
            
        for (n = 0; n < MAX_MAPPING; n++)
            cdRemap[n] = (n+1);

        CDAudio_GetAudioDiskInfo();
        return;
    }

    if (cd_fd < 0)
       open_cdrom();
   
    if (!strncmp(command, "info", 4)) {
        if(cd_fd < 0)
        {
	    CONS_Printf("No CDROM");
	    return;
	}
        CDAudio_GetAudioDiskInfo();
        CONS_Printf("%u tracks\n", maxTrack);
        if (playing)
            CONS_Printf("Currently %s track %u\n", play_looping ? "looping" : "playing", playTrack);
        else if (wasPlaying)
            CONS_Printf("Paused %s track %u\n", play_looping ? "looping" : "playing", playTrack);
        CONS_Printf("Volume is %d\n", cdvolume);
        return;
    }

    if (!initialized)
       return;
        
    if (!cdValid) {
        if( CDAudio_GetAudioDiskInfo() )
            return;
    }

   
    if (!strncmp(command, "open", 4)) {
        I_EjectCD();
        return;
    }

    if (!strncmp(command, "play", 4)) {
            I_PlayCD((byte)atoi(COM_Argv (2)), false);
            return;
    }

    if (!strncmp(command, "loop", 4)) {
            I_PlayCD((byte)atoi(COM_Argv (2)), true);
            return;
    }

    if (!strncmp(command, "stop", 4)) {
            I_StopCD();
            return;
    }
        
    if (!strncmp(command, "pause", 5)) {
            I_PauseCD();
            return;
    }
        
    if (!strncmp(command, "resume", 6)) {
            I_ResumeCD();
            return;
    }
        
    CONS_Printf("Invalid command \"cd %s\"\n", COM_Argv (1));
}

static
void I_StopCD(void)
{
    if (cd_fd < 0)
        return;
    
    if (!(playing || wasPlaying))
        return;

    if ( ioctl(cd_fd, CDROMSTOP, 0) < 0 )
        CONS_Printf("CD stop: %s\n", strerror(errno));

    wasPlaying = false;
    playing = false;
    lastchk = 0;
}

void I_PauseCD (void)
{
    if (cd_fd < 0 || !cd_enabled)
        return;
    
    if (!playing)
        return;

    if ( ioctl(cd_fd, CDROMPAUSE) < 0 )
        CONS_Printf("CD pause: %s\n", strerror(errno));

    wasPlaying = playing;
    playing = false;
    lastchk = 0;
}

// continue after a pause
void I_ResumeCD (void)
{
    if (cd_fd < 0 || !cd_enabled)
        return;
    
    if (!cdValid)
        return;

    if (!wasPlaying)
        return;
    
    if ( ioctl(cd_fd, CDROMRESUME) < 0 ) 
        CONS_Printf("CD resume: %s\n", strerror(errno));

    playing = true;
    wasPlaying = false;
    lastchk = ( play_looping )? 2 : 0;

    if(cv_jigglecdvol.value)
    {
        I_SetVolumeCD(31-cd_volume.value);
        I_SetVolumeCD(cd_volume.value);
    }

    return;
}


void I_ShutdownCD (void)
{
    if (!initialized)
        return;

    I_StopCD();
    close(cd_fd);
    cd_fd = -1;

    cdValid = false;
    cd_enabled = false;
}

void I_InitCD (void)
{
    // Don't start music on a dedicated server
    if (M_CheckParm("-dedicated"))
            return ;
    
    // Has been checked in d_main.c, but doesn't hurt here
    if (M_CheckParm ("-nocd"))
            return ;
    
    // New commandline switch -cddev 
    if ( M_CheckParm("-cddev") && M_IsNextParm() ) {
        strncpy(cd_dev, M_GetNextParm(), sizeof(cd_dev));
        cd_dev[sizeof(cd_dev) - 1] = 0;
    }

    COM_AddCommand ("cd", command_CD_f, CC_command);

    CONS_Printf("CD Audio Initialized\n");

    initialized = true;
    return;
}


static
int  open_cdrom(void)
{
    int i;

    if( cd_fd >= 0 )  return 1;   // already open

    // As of Linux 2.1, must use O_NONBLOCK.  See /usr/include/linux/cdrom.h
    cd_fd = open(cd_dev, O_RDONLY|O_NONBLOCK);
    if ( cd_fd < 0 )
    {
        int myerrno = errno;
        CONS_Printf("Open \"%s\" failed:\n %s\n",
		    cd_dev, strerror(myerrno));
        if(EACCES == myerrno)
        {
	    // permission denied -> very common problem with IDE drives
	    // Shall we add a line about this in the README?
            CONS_Printf("-------------------------------------\n"
                        "Permission denied to open device %s\n"
                        "Set read permission or run as root\n"
                        "if in doubt *READ THE DOCS*\n"
                        "-------------------------------------\n", cd_dev);
	}
        return -1;
    }
    for (i = 0; i < MAX_MAPPING; i++)
        cdRemap[i] = (i+1);
        
    cd_enabled = true;
    cdValid = false;  // force read of TOC

    if(verbose)
        CONS_Printf( "CD %s open\n", cd_dev );

    if(cv_jigglecdvol.value)
    {
        I_SetVolumeCD(31-cd_volume.value);
    }
    I_SetVolumeCD(cd_volume.value);

    return 2;
}


// Check for done and loop track.
void I_UpdateCD (void)
{
    if(dedicated)
        return;
    
    if (!cd_enabled || !play_looping || !playing )
        return;
   
    if( lastchk < 3 )
    {
        if( lastchk == 2 )
        {
	  // Est. time to play,  + 4 secs.
          lastchk = time(NULL) + (play_time + 4);
        }
        return;
    }

    // FIXME: Do we have a "hicup" here every 2 secs?
    if ( playing && (lastchk < time(NULL)) )
    {
        struct cdrom_subchnl subchnl;
    
        // [WDJ] Checking the CD status blocks for 1/4 second,
        // which is very visible during play.
            lastchk = time(NULL) + 4; // 4 seconds between chks
            subchnl.cdsc_format = CDROM_MSF;
            if (ioctl(cd_fd, CDROMSUBCHNL, &subchnl) < 0 ) {
                CONS_Printf("CD subchnl: %s\n", strerror(errno));
                playing = false;
                return;
            }
            if (subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
                subchnl.cdsc_audiostatus != CDROM_AUDIO_PAUSED) {
                playing = false;
                if (play_looping)
                    I_PlayCD(playTrack|0x100, true);
            }
    }
}


// play the cd
//  track : 1 .. n
void I_PlayCD (unsigned int track, boolean looping)
{
    struct cdrom_tocentry entry;
    struct cdrom_msf msf;

    if ( !cd_enabled )
        return;
     
    if( cd_fd < 0 ) {  // to allow insert of disk after starting DoomLegacy
        if( open_cdrom() < 0 )   return;
    }

    if (!cdValid)
    {
        if( CDAudio_GetAudioDiskInfo() )
            return;
    }

    if( track & 0x100 )
    {
        // looping bypasses remap
        track = track & 0x3f;
    }
    else
    {
        if( track < 1 )   track = 1;
        if( track > MAX_MAPPING-1 )  track = MAX_MAPPING-1;
        // cdRemap index is 0 .. n-1
        track = cdRemap[track-1];
        if( track == 0 )  return;
        // Linux CDROM tracks are 1 .. n
    }

#if 0
    if( ioctl(cd_fd, CDROMSTART) < 0 )
    {
        errstr = "CDROM Start";
        goto errexit;
    }
#endif
   
    // don't try to play a non-audio track
    entry.cdte_track = track;
    entry.cdte_format = CDROM_MSF;
    if ( ioctl(cd_fd, CDROMREADTOCENTRY, &entry) < 0 )
    {
        CONS_Printf("CD read TOC: %s\n", strerror(errno));
        return;
    }
    if (entry.cdte_ctrl == CDROM_DATA_TRACK)
    {
        CONS_Printf("I_PlayCD: track %i is not audio\n", track);
        return;
    }

#if 0   
    if(cv_jigglecdvol.value)
    {
        I_SetVolumeCD(31-cd_volume.value);
        I_SetVolumeCD(cd_volume.value);
    }
#endif   

    if (playing)
    {
        if (playTrack == track)
            return;
        I_StopCD();
    }

    if(!CDAudio_GetStartStop(&msf, track, &entry)) 
        return;

    if( ioctl(cd_fd, CDROMPLAYMSF, &msf) < 0 ) {
        CONS_Printf("CD play msf: %s\n", strerror(errno));
        return;
    }
#if 0
        // FIXME: is this necessary??
    if( ioctl(cd_fd, CDROMRESUME) < 0 ) 
        CONS_Printf("CD resume: %s\n", strerror(errno));
#endif   

    if(cv_jigglecdvol.value)
    {
        I_SetVolumeCD(31-cd_volume.value);
    }
    I_SetVolumeCD(cd_volume.value);
    
    play_looping = looping;
    // only enable play check if play looping
    lastchk = ( play_looping )? 2 : 0;
    // play time, secs
    play_time = ((msf.cdmsf_min1 - msf.cdmsf_min0) * 60)
	     + (msf.cdmsf_sec1 - msf.cdmsf_sec0);
    if( verbose )
        CONS_Printf( "Play time %u secs\n", play_time );
    playTrack = track;
    playing = true;
}


// volume : logical cd audio volume 0-31 (hardware is 0-255)
static
void I_SetVolumeCD (int volume)
{
    struct cdrom_volctrl volctrl;
    
    if( cd_fd < 0 )  return;

    if(volume < 0 || volume > 31)
    {
        CONS_Printf("cdvolume should be between 0-31\n");
        volume = 0;
    }

    // volume control for CD music
    volctrl.channel0 = (volume * 255.0) / 31.0;
    volctrl.channel1 = volctrl.channel0;
    volctrl.channel2 = 0;
    volctrl.channel3 = 0;

    if(ioctl(cd_fd, CDROMVOLCTRL, &volctrl) < 0){
        CONS_Printf("CD volume: %s\n", strerror(errno));
    }

    // Read back volume
    cdvolume = 0;
    if(ioctl(cd_fd, CDROMVOLREAD, &volctrl) >= 0){
        cdvolume = (volctrl.channel0 * 31) / 255;
    }
}
