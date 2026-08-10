// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: s_amb.c 1361 2017-10-16 16:26:45Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// $Log: s_amb.c,v $
// Revision 1.6  2001/06/10 21:16:01  bpereira
// no message
//
// Revision 1.5  2001/05/27 13:42:48  bpereira
// Revision 1.4  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.3  2001/02/24 13:35:21  bpereira
// Revision 1.2  2001/02/10 13:20:55  hurdler
// update license
//
//
//
// DESCRIPTION:
//   Ambient sound
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "g_game.h"
#include "p_local.h"
#include "p_setup.h"
  //levelflats for flat animation
#include "r_data.h"
#include "m_random.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "dstrings.h"
  //SoM: 3/10/2000
#include "r_main.h"
  //Two extra includes.
#include "t_script.h"

// 3D sound
//#include "hardware/hw3sound.h"

// [WDJ] To support Heretic need 8. Added more to support larger Heretic wads.
#define MAX_AMBIENT_SFX   16 // Per level

// From Heretic.
// Types

typedef enum
{                          // parameters in seq after the command
   afxcmd_play,            // (sound)
   afxcmd_playabsvol,      // (sound, volume)
   afxcmd_playrelvol,      // (sound, volume)
   afxcmd_delay,           // (ticks)
   afxcmd_delayrand,       // (andbits)
   afxcmd_end              // ()
} afxcmd_t;

// Heretic used int. That is too large, and will be worse with 64 bit machine.
// So far, largest value is sfx_amb10, which is 0xf7.
typedef int16_t   ambseq_t;

// Data

static ambseq_t * LevelAmbientSfx[MAX_AMBIENT_SFX];
static ambseq_t * AmbSfxPtr;  // ptr to cmds and parameters
static int AmbSfxCount;
static int AmbSfxTics;
static int AmbSfxVolume;

// Heretic Ambient Sfx Sequences
ambseq_t AmbSndSeqInit[] =
{ // Startup
        afxcmd_end
};
ambseq_t AmbSndSeq1[] =
{ // Scream
        afxcmd_play, sfx_amb1,
        afxcmd_end
};
ambseq_t AmbSndSeq2[] =
{ // Squish
        afxcmd_play, sfx_amb2,
        afxcmd_end
};
ambseq_t AmbSndSeq3[] =
{ // Drops
        afxcmd_play, sfx_amb3,
        afxcmd_delay, 16,
        afxcmd_delayrand, 31,
        afxcmd_play, sfx_amb7,
        afxcmd_delay, 16,
        afxcmd_delayrand, 31,
        afxcmd_play, sfx_amb3,
        afxcmd_delay, 16,
        afxcmd_delayrand, 31,
        afxcmd_play, sfx_amb7,
        afxcmd_delay, 16,
        afxcmd_delayrand, 31,
        afxcmd_play, sfx_amb3,
        afxcmd_delay, 16,
        afxcmd_delayrand, 31,
        afxcmd_play, sfx_amb7,
        afxcmd_delay, 16,
        afxcmd_delayrand, 31,
        afxcmd_end
};
ambseq_t AmbSndSeq4[] =
{ // SlowFootSteps
        afxcmd_play, sfx_amb4,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb4, -3,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb4, -3,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb4, -3,
        afxcmd_delay, 15,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_end
};
ambseq_t AmbSndSeq5[] =
{ // Heartbeat
        afxcmd_play, sfx_amb5,
        afxcmd_delay, 35,
        afxcmd_play, sfx_amb5,
        afxcmd_delay, 35,
        afxcmd_play, sfx_amb5,
        afxcmd_delay, 35,
        afxcmd_play, sfx_amb5,
        afxcmd_end
};
ambseq_t AmbSndSeq6[] =
{ // Bells
        afxcmd_play, sfx_amb6,
        afxcmd_delay, 17,
        afxcmd_playrelvol, sfx_amb6, -8,
        afxcmd_delay, 17,
        afxcmd_playrelvol, sfx_amb6, -8,
        afxcmd_delay, 17,
        afxcmd_playrelvol, sfx_amb6, -8,
        afxcmd_end
};
ambseq_t AmbSndSeq7[] =
{ // Growl
        afxcmd_play, sfx_bstsit,
        afxcmd_end
};
ambseq_t AmbSndSeq8[] =
{ // Magic
        afxcmd_play, sfx_amb8,
        afxcmd_end
};
ambseq_t AmbSndSeq9[] =
{ // Laughter
        afxcmd_play, sfx_amb9,
        afxcmd_delay, 16,
        afxcmd_playrelvol, sfx_amb9, -4,
        afxcmd_delay, 16,
        afxcmd_playrelvol, sfx_amb9, -4,
        afxcmd_delay, 16,
        afxcmd_playrelvol, sfx_amb10, -4,
        afxcmd_delay, 16,
        afxcmd_playrelvol, sfx_amb10, -4,
        afxcmd_delay, 16,
        afxcmd_playrelvol, sfx_amb10, -4,
        afxcmd_end
};
ambseq_t AmbSndSeq10[] =
{ // FastFootsteps
        afxcmd_play, sfx_amb4,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb4, -3,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb4, -3,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb4, -3,
        afxcmd_delay, 8,
        afxcmd_playrelvol, sfx_amb11, -3,
        afxcmd_end
};

#define NUM_AMBIENT_SFX  10
ambseq_t * AmbientSfx[NUM_AMBIENT_SFX] =
{
        AmbSndSeq1,             // Scream
        AmbSndSeq2,             // Squish
        AmbSndSeq3,             // Drops
        AmbSndSeq4,             // SlowFootsteps
        AmbSndSeq5,             // Heartbeat
        AmbSndSeq6,             // Bells
        AmbSndSeq7,             // Growl
        AmbSndSeq8,             // Magic
        AmbSndSeq9,             // Laughter
        AmbSndSeq10             // FastFootsteps
};

//----------------------------------------------------------------------------
//
// PROC P_Init_AmbientSound
//
//----------------------------------------------------------------------------

void P_Init_AmbientSound(void)
{
        AmbSfxCount = 0;
        AmbSfxVolume = 0;
        AmbSfxTics = 10*TICRATE;
        AmbSfxPtr = AmbSndSeqInit;
}

//----------------------------------------------------------------------------
//
// PROC P_AddAmbientSfx
//
// Called by (P_mobj):P_SpawnMapThing during (P_setup):P_SetupLevel.
//
//----------------------------------------------------------------------------

// Ambient sound object in level map.
//  sequence : the thing type - 1200, ambient things are 1200..1300
void P_AddAmbientSfx(int sequence)
{
    if( sequence < NUM_AMBIENT_SFX )
    {
        if(AmbSfxCount == MAX_AMBIENT_SFX)
        {
            I_SoftError("Too many ambient sound sequences");
            return;
        }
        LevelAmbientSfx[AmbSfxCount++] = AmbientSfx[sequence];
    }
}

//----------------------------------------------------------------------------
//
// PROC P_AmbientSound
//
// Called every tic by (P_tick):P_Ticker.
//
//----------------------------------------------------------------------------

// Called by P_Ticker.
void P_AmbientSound(void)
{
    afxcmd_t cmd;
    sfxid_t sound;

    if(!AmbSfxCount)
    { // No ambient sound sequences on current level
        return;
    }

    if(--AmbSfxTics)  // sequence delay
    {
        return;
    }

    // Loops until it starts another delay.
    for(;;)
    {
        cmd = *AmbSfxPtr++;
        switch(cmd)
        {
         case afxcmd_play:
            AmbSfxVolume = A_Random()>>2;
            S_StartAmbientSound(*AmbSfxPtr++, AmbSfxVolume);
            break;
         case afxcmd_playabsvol:
            sound = *AmbSfxPtr++;
            AmbSfxVolume = *AmbSfxPtr++;
            S_StartAmbientSound(sound, AmbSfxVolume);
            break;
	 case afxcmd_playrelvol:
            sound = *AmbSfxPtr++;
            AmbSfxVolume += *AmbSfxPtr++;
            if(AmbSfxVolume < 0)
            {
                AmbSfxVolume = 0;
            }
            else if(AmbSfxVolume > 127)
            {
                AmbSfxVolume = 127;
            }
            S_StartAmbientSound(sound, AmbSfxVolume);
            break;
	 case afxcmd_delay:
            AmbSfxTics = *AmbSfxPtr++;
	    goto done;
	 case afxcmd_delayrand:
	    AmbSfxTics = A_Random()&(*AmbSfxPtr++);
	    goto done;
	 default:
	    GenPrintf( EMSG_error, "P_AmbientSound: Unknown afxcmd %d", cmd);
	    // fall into end to start another sequence.
	 case afxcmd_end:
	    // Delay, and start another ambient sound.
            AmbSfxTics = 6*TICRATE+A_Random();
            AmbSfxPtr = LevelAmbientSfx[A_Random()%AmbSfxCount];
	    goto done;
        }
    }
done:
    return;
}
