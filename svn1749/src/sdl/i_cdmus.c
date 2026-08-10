// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_cdmus.c 1625 2022-04-06 05:07:41Z wesleyjohnson $
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
// $Log: i_cdmus.c,v $
// Revision 1.3  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
// DESCRIPTION:
//      cd music interface
//
//-----------------------------------------------------------------------------

// Because of WINVER redefine, put before any include that might define WINVER
#include "doomincl.h"

#include <SDL.h>

#include "i_sound.h"
#include "command.h"
#include "m_argv.h"

#ifdef SDL2
// [WDJ] SDL2 does not have support for CDROM music.
// CDROM music is disabled in the Makefile.
#endif

// [WDJ] SDL cannot control CDROM volume.
// This is only present in the case someone wants to experiment, do not
// turn it on for releases.
// #define CDROM_VOLUME_CONTROL

// [WDJ] SDL_cdrom.h is not present on some MAC SDL installs.
// This detects if SDL.h includes SDL/SDL_cdrom.h
// [WDJ] SDL2 does not even have CDROM API.
#if defined(SDL_AUDIO_TRACK) && ! defined(USE_SDL2)

// Remap index is level map 1..34, or (episode-1)*9+map 1..36
#define MAX_MAPPING   40
#define MAX_CD_TRACKS 255

static SDL_CD *cdrom = NULL;  // if non-NULL, the CD-ROM system is initialized

static boolean cd_enabled = false; // do we want cd music? changed using the cd console command
static boolean play_looping = false;
static unsigned int playTrack; // track being played, 1..n
static byte  cdRemap[MAX_MAPPING];

static int open_cdrom(void);
static void cd_volume_onchange(void);

#ifdef CDROM_VOLUME_CONTROL
CV_PossibleValue_t cd_volume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};
consvar_t cd_volume = {"cd_volume", "31", CV_SAVE | CV_CALL, cd_volume_cons_t, cd_volume_onchange};
#else
// [WDJ] SDL cannot control CDROM volume.
CV_PossibleValue_t cd_volume_cons_t[]={{0,"MIN"},{1,"MAX"},{0,NULL}};
consvar_t cd_volume = {"cd_volume", "1", CV_SAVE | CV_CALL, cd_volume_cons_t, cd_volume_onchange};
#endif

static void cd_volume_onchange(void)
{

  if (!cdrom)
    return;

#ifdef CDROM_VOLUME_CONTROL
  // Experiment here.
#else
  // HACK: SDL does not support setting the CD volume.
  // Use pause instead and toggle between full and no music.
  if (cd_volume.value > 0 && cdrom->status == CD_PAUSED)
    I_ResumeCD();
  else if (cd_volume.value == 0 && cdrom->status == CD_PLAYING)
    I_PauseCD();
#endif
}


static Uint32  lastchk = 0;   // SDL time

/**************************************************************************
 *
 * function: CDAudio_GetAudioDiskInfo
 *
 * description:
 * update the SDL_CD status info behind the cdrom pointer
 * returns true if there's a cd in the drive and it's ok
 *
 **************************************************************************/
static boolean CDAudio_GetAudioDiskInfo(void)
{
  // use CD_INDRIVE() for cdValid
  CDstatus cdStatus = SDL_CDStatus(cdrom);

  if (cdStatus == CD_ERROR)
  {
      CONS_Printf("CD Error: %s\n", SDL_GetError());
      return false;
  }

  if (!CD_INDRIVE(cdStatus))
  {
      if( verbose )
          CONS_Printf("No CD in drive\n");
      return false;
  }
    
  return true;
}

/**************************************************************************
 *
 * function: StopCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_StopCD(void)
{
  if (!cdrom)
    return;
    
  lastchk = 0;

  if (SDL_CDStop(cdrom))
  {
      CONS_Printf("CD stop failed\n");
  }
}

/**************************************************************************
 *
 * function: I_EjectCD
 *
 * description:
 *
 *
 **************************************************************************/
static void I_EjectCD(void)
{
  if (!cdrom)
    return; // no cd init'd
    
  I_StopCD();
    
  if (SDL_CDEject(cdrom))
  {
      GenPrintf(EMSG_warn, "CD eject failed\n");
  }
}

/**************************************************************************
 *
 * function: command_CD_f
 *
 * description:
 * handles all CD commands from the console
 *
 **************************************************************************/
static void command_CD_f (void)
{
    char	*command;
    int		ret;
    int		n;

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
	I_StopCD();
	cd_enabled = false;
	return;
    }
	
    if (!strncmp(command, "remap", 5)) {
	ret = COM_Argc() - 2;
	if (ret <= 0) {
	    // list the mapping
	    for (n = 0; n < MAX_MAPPING; n++)
	    {
		if (cdRemap[n] != (n+1))
		    CONS_Printf("  %d -> %d\n", n, cdRemap[n]);
	    }
	    return;
	}

	// set a mapping
	for (n = 0; n < ret; n++)
	    cdRemap[n] = atoi(COM_Argv(n+1));
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

    if (!cdrom)
       open_cdrom();

    if (!strncmp(command, "info", 4))
    {
        if( !cdrom )
        {
	    CONS_Printf("No CDROM");
	    return;
	}
        SDL_CDStatus( cdrom );  // update status
        CONS_Printf("CDROM: %s\n", SDL_CDName(0) );
        CONS_Printf("%d tracks\n", cdrom->numtracks);
	if (cdrom->status == CD_PLAYING)
    	    CONS_Printf("Currently %s track %d\n", play_looping ? "looping" : "playing", playTrack);
	else if (cdrom->status == CD_PAUSED)
	    CONS_Printf("Paused %s track %d\n", play_looping ? "looping" : "playing", playTrack);
	else
            CONS_Printf("Not playing\n");
#ifdef CDROM_VOLUME_CONTROL
	CONS_Printf("Volume is %d\n", cd_volume.value);
#endif
	return;
    }

    // from this point on, make sure the cd is ok        
    SDL_CDStatus( cdrom );  // update status
    if ( !(CD_INDRIVE(cdrom->status)) ) {
      if (!CDAudio_GetAudioDiskInfo()) // check if situation has changed
        return;
    }

    if (!strncmp(command, "open", 4)) {
	I_EjectCD();
	return;
    }

    if (!strncmp(command, "play", 4)) {
	I_PlayCD(atoi(COM_Argv (2)), false);
	return;
    }

    if (!strncmp(command, "loop", 4)) {
	I_PlayCD(atoi(COM_Argv (2)), true);
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


/**************************************************************************
 *
 * function: PauseCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_PauseCD (void)
{
  if (!cdrom || !cd_enabled)
    return;
    
  lastchk = 0;

  if (SDL_CDPause(cdrom))
  {
      CONS_Printf("CD pause failed\n");
  }
}

/**************************************************************************
 *
 * function: ResumeCD
 *
 * description:
 *
 *
 **************************************************************************/
// continue after a pause
void I_ResumeCD (void)
{
  if (!cdrom || !cd_enabled)
    return;

  // only enable play check if play looping
  lastchk = ( play_looping )? 2 : 0;
   
  if (SDL_CDResume(cdrom))
  {
      CONS_Printf("CD resume failed\n");
  }
}


/**************************************************************************
 *
 * function: ShutdownCD
 *
 * description:
 *
 *
 **************************************************************************/
void I_ShutdownCD (void)
{
  if (!cdrom)
    return;

  I_StopCD();

  SDL_CDClose(cdrom);
  cdrom = NULL;
}

/**************************************************************************
 *
 * function: InitCD
 *
 * description:
 * Initialize the first CD drive SDL detects and add console command 'cd'
 *
 **************************************************************************/
void I_InitCD (void)
{
  cdrom = NULL;

  // Initialize SDL cdrom subsystem
  if (SDL_InitSubSystem(SDL_INIT_CDROM) < 0)
  {
      CONS_Printf(" Couldn't initialize SDL CD-ROM subsystem: %s\n", SDL_GetError());
      return;
  }

  if (SDL_CDNumDrives() < 1)
  {
      CONS_Printf(" No CD-ROM drives found.\n");
      return;
  }

  COM_AddCommand ("cd", command_CD_f, CC_command);
}


static
int open_cdrom(void)
{
  int i;

  if( cdrom )  return 1;   // already open
   
  // Open a drive
  const char *cdName = SDL_CDName(0);
  cdrom = SDL_CDOpen(0);
    
  if (!cdrom)
  {
      CONS_Printf("Error CD-ROM drive %s: %s\n",
		  ( cdName? cdName : "NULL" ), SDL_GetError());
      return -1;
  }
  CONS_Printf("CD-ROM drive %s initialized.\n", cdName);

  // init track mapping
  for (i = 0; i < MAX_MAPPING; i++)
    cdRemap[i] = (i+1);
    
  cd_enabled = true;

  if (CDAudio_GetAudioDiskInfo())
    CONS_Printf(" %d tracks found.\n", cdrom->numtracks);
  
  return 2;
}


//
/**************************************************************************
 *
 * function: UpdateCD
 *
 * description:
 * checks the cd status and restarts play if looping
 *
 **************************************************************************/
void I_UpdateCD (void)
{
  if (!cdrom || !cd_enabled || !play_looping )
    return;

  if( lastchk < 3 )
  {
      if( lastchk == 2 )
      {
	  // Est. time to play,  + 4 secs.
	  SDL_CDStatus( cdrom );
	  int pt = cdrom->track[playTrack-1].length/CD_FPS;  // play time, secs
	  if( verbose )
	     CONS_Printf( "Play time %i secs\n", pt );
          lastchk = SDL_GetTicks() + ( pt + 4 ) * 1000;
	  
      }
      return;
  }

  if( lastchk < SDL_GetTicks() ) 
  {
      lastchk = SDL_GetTicks() + 4000; // 4 seconds between chks

      // [WDJ] Checking the CD status blocks for 1/4 second,
      // which is very visible during play.
      // So this is done only once per track played.

      // check the status
//      if (!CDAudio_GetAudioDiskInfo())
//	return; // no valid cd in drive
      SDL_CDStatus( cdrom );  // update status
      if (cdrom->status == CD_STOPPED && play_looping)
	I_PlayCD(playTrack|0x100, true);
  }
}



/**************************************************************************
 *
 * function: PlayCD
 *
 * description:
 * play the requested track and set the looping flag
 * pauses the CD if volume is 0
 * 
 **************************************************************************/
//  track : 1 .. n
void I_PlayCD (unsigned int track, boolean looping)
{
  unsigned int  s_track;  // sdl track 0..n-1

  if( ! cdrom ) {  // to allow insert of disk after starting DoomLegacy
      if( open_cdrom() < 0 )   return;
  }

  if (!cdrom || !cd_enabled)
    return;

  // Avoid checking status when possible.
  if ( !(CD_INDRIVE(cdrom->status)) )
  {
      if (!CDAudio_GetAudioDiskInfo()) // check if situation has changed
      {
	    CONS_Printf("No CD in drive.\n");
	    return;
      }
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
      // SDL tracks are 0 .. n-1
  }
  s_track = track - 1;
    
  if (track > cdrom->numtracks)
  {
      CONS_Printf("I_PlayCD: Bad track number %d.\n", track);
      return;
  }
    
  // don't try to play a non-audio track
  if (cdrom->track[s_track].type == SDL_DATA_TRACK)
  {
      CONS_Printf("I_PlayCD: track %d is not audio.\n", track);
      return;
  }
	
  if (cdrom->status == CD_PLAYING)
  {
      if (playTrack == track)
	return; // already playing it

      I_StopCD();
  }

  if (SDL_CDPlayTracks(cdrom, s_track, 0, 1, 0))
  {
      CONS_Printf("Error playing track %d: %s\n", track, SDL_GetError());
      return;
  }
    
  play_looping = looping;
  // only enable play check if play looping
  lastchk = ( play_looping )? 2 : 0;
  playTrack = track;

  if (cd_volume.value == 0)
    I_PauseCD(); // cd "volume" hack
}


#else
// [WDJ] SDL2 does not have cdrom API.
// [WDJ] some MAC do not have SDL_cdrom.h
// This will stop errors for now.

CV_PossibleValue_t cd_volume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};
consvar_t cd_volume = {"cd_volume", "31", CV_SAVE | CV_CALL, cd_volume_cons_t, NULL};

void I_StopCD(void)
{
}

void I_PauseCD (void)
{
}

void I_ResumeCD (void)
{
}

void I_ShutdownCD (void)
{
}

void I_InitCD (void)
{
}

void I_UpdateCD (void)
{
}

void I_PlayCD (unsigned int track, boolean looping)
{
}

#endif
