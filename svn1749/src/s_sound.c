// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: s_sound.c 1733 2025-03-06 13:21:03Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: s_sound.c,v $
// Revision 1.33  2003/07/14 21:22:24  hurdler
// go RC1
//
// Revision 1.32  2003/07/13 13:16:15  hurdler
//
// Revision 1.31  2002/12/13 22:34:27  ssntails
// MP3/OGG support!
//
// Revision 1.30  2002/09/19 21:47:05  judgecutor
//
// Revision 1.29  2002/09/12 20:10:51  hurdler
// Added some cvars
//
// Revision 1.28  2002/08/16 20:19:36  judgecutor
// Sound pitching coming back
//
// Revision 1.27  2001/08/20 20:40:39  metzgermeister
// Revision 1.26  2001/05/27 13:42:48  bpereira
//
// Revision 1.25  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.24  2001/04/18 19:32:26  hurdler
//
// Revision 1.23  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.22  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.21  2001/04/02 18:54:32  bpereira
// Revision 1.20  2001/04/01 17:35:07  bpereira
// Revision 1.19  2001/03/03 11:11:49  hurdler
// Revision 1.18  2001/02/24 13:35:21  bpereira
// Revision 1.17  2001/01/27 11:02:36  bpereira
//
// Revision 1.16  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.15  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.14  2000/11/12 21:59:53  hurdler
// Please verify that sound bug
//
// Revision 1.13  2000/11/03 11:48:40  hurdler
// Fix compiling problem under win32 with 3D-Floors and FragglScript (to verify!)
//
// Revision 1.12  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.11  2000/10/27 20:38:20  judgecutor
// - Added the SurroundSound support
//
// Revision 1.10  2000/09/28 20:57:18  bpereira
// Revision 1.9  2000/05/07 08:27:57  metzgermeister
//
// Revision 1.8  2000/04/22 16:16:50  emanne
// Correction de l'interface.
// Une erreur s'y était glissé, d'où un segfault si on compilait sans SDL.
//
// Revision 1.7  2000/04/21 08:23:47  emanne
// To have SDL working.
// qmus2mid.h: force include of qmus2mid_sdl.h when needed.
//
// Revision 1.6  2000/03/29 19:39:48  bpereira
//
// Revision 1.5  2000/03/22 18:51:08  metzgermeister
// introduced I_PauseCD() for Linux
//
// Revision 1.4  2000/03/12 23:21:10  linuxcub
// Added consvars which hold the filenames and arguments which will be used
// when running the soundserver and musicserver (under Linux). I hope I
// didn't break anything ... Erling Jacobsen, linuxcub@email.dk
//
// Revision 1.3  2000/03/06 15:13:08  hurdler
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//    Sound control
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "g_game.h"
#include "m_argv.h"
#include "r_main.h"     //R_PointToAngle2() used to calc stereo sep.
#include "r_things.h"   // for skins
#include "p_info.h"

#include "i_sound.h"
#include "s_sound.h"
#include "qmus2mid.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_main.h"

#include "m_random.h"

// 3D Sound Interface
#include "hardware/hw3sound.h"

#ifdef SMIF_X11
#include "linux_x/lx_ctrl.h"
  // SOUND_DEVICE_OPTION
#endif

// #define DEBUG_SOUND_HEADER

#define MIDI_OPTIONS_CONTROL


#ifdef SOUND_DEVICE_OPTION
// SMIF_X11 only.

static void CV_snd_opt_OnChange( void )
{
    I_SetSoundOption( cv_snd_opt.EV );
}

// The values of snd_opt are defined in linux_x/i_sound.c
// to ensure that menu values and implementation always match.
consvar_t cv_snd_opt = { "snd_opt", "1", CV_SAVE | CV_CALL | CV_NOINIT, snd_opt_cons_t, CV_snd_opt_OnChange };
#endif

#ifdef MUSSERV
// SMIF_X11 only.
consvar_t cv_musserver_cmd = { "musserver_cmd", "musserver", CV_SAVE };
consvar_t cv_musserver_arg = { "musserver_arg", "-t 20", CV_SAVE };

#ifdef MUS_DEVICE_OPTION
static void CV_musserv_opt_OnChange( void )
{
    I_SetMusicOption( cv_musserver_opt.EV );
}

// The values of musserv_opt are defined in linux_x/i_sound.c
// to ensure that menu values and implementation always match.
consvar_t cv_musserver_opt = { "musserver_opt", "1", CV_SAVE | CV_CALL | CV_NOINIT,
             musserv_opt_cons_t, CV_musserv_opt_OnChange };
#endif
#endif

#ifdef MIDI_OPTIONS_CONTROL
static void CV_midi_options_OnChange( void );

// [WDJ] Enable of some OLD CODE.
consvar_t cv_midi_create_program = { "midi_create_program", "0", CV_SAVE | CV_CALL, CV_OnOff, CV_midi_options_OnChange };
// [WDJ] Enable of midi compression.  Partially disabled on WIN32.
consvar_t cv_midi_compress = { "midi_compress", "1", CV_SAVE | CV_CALL, CV_OnOff, CV_midi_options_OnChange };
    
static byte midi_music_num_playing = 0;

static void CV_midi_options_OnChange( void )
{
    EN_create_program = cv_midi_create_program.EV;
    EN_midi_compress = cv_midi_compress.EV;

    if( midi_music_num_playing )  // not on startup
    {
        S_StopMusic();
        S_ChangeMusic( midi_music_num_playing, 1 );
    }
}
#endif

#ifdef MACOS_DI
// specific to macos directory
consvar_t play_mode = { "play_mode", "0", CV_SAVE, CV_byte };
  // enum playmode_t (0..2)
#endif


// indexed by music_type_e
byte music_type_to_ADM[] = {
 ADM_MUS,  // MUSTYPE_MUS,
 ADM_MIDI, // MUSTYPE_MIDI,
 ADM_MP3,  // MUSTYPE_MP3,
 ADM_OGG,  // MUSTYPE_OGG,
 0,  // MUSTYPE_OTHER
};

// indexed by music_type_e
char * music_type_str[] = {
 "MUS",  // MUSTYPE_MUS,
 "MIDI", // MUSTYPE_MIDI,
 "MP3",  // MUSTYPE_MP3,
 "OGG",  // MUSTYPE_OGG,
 "OTHER",  // MUSTYPE_OTHER
};

byte  EN_port_music = ADM_MUS;  // ADM_ MP3, OGG music


#ifdef MUSIC_SOURCE_CONTROL
// To make the table easier to fill-in
#ifdef MUSIC_MP3
# define EN_ADM_MP3  ADM_MP3
#else
# define EN_ADM_MP3  0
#endif

#ifdef MUSIC_OGG
# define EN_ADM_OGG  ADM_OGG
#else
# define EN_ADM_OGG  0
#endif

static byte src_music_enables[] = {
  ADM_MUS | ADM_MIDI,   // MUS
  ADM_MUS | ADM_MIDI | EN_ADM_MP3 | EN_ADM_OGG, // Auto
  EN_ADM_MP3, // MP3 only
  EN_ADM_OGG, // OGG only
};

static byte  EN_src_music = 0;  // ADM_
static byte  music_num_playing = 0;

void CV_music_source_OnChange( void )
{
    EN_src_music = src_music_enables[ cv_music_source.EV ];
    if( music_num_playing )  // not on startup
        S_ChangeMusic( music_num_playing, 1 );

    cv_music_source.state &= ~CS_MODIFIED;
}

// MUS is standard MUS->MIDI
CV_PossibleValue_t music_source_cons_t[] = { {0, "MUS"}, {1, "Auto"},
#ifdef MUSIC_MP3
   {2, "MP3" },
#endif
#ifdef MUSIC_OGG
   {3, "OGG" },
#endif
   {0, NULL} };
consvar_t cv_music_source = { "music_source", "1", CV_SAVE | CV_CALL,
    music_source_cons_t, CV_music_source_OnChange };

#undef EN_ADM_MP3
#undef EN_ADM_OGG
#endif

// stereo reverse 1=true, 0=false
consvar_t cv_stereoreverse = { "stereoreverse", "0", CV_SAVE, CV_OnOff };

// if true, all sounds are loaded at game startup
consvar_t cv_precachesound = { "precachesound", "0", CV_SAVE, CV_OnOff };

CV_PossibleValue_t soundvolume_cons_t[] = { {0, "MIN"}, {31, "MAX"}, {0, NULL} };

// actual general (maximum) sound & music volume, saved into the config
consvar_t cv_soundvolume = { "soundvolume", "15", CV_SAVE, soundvolume_cons_t };
consvar_t cv_musicvolume = { "musicvolume", "15", CV_SAVE, soundvolume_cons_t };
consvar_t cv_rndsoundpitch = { "rndsoundpitch", "Off", CV_SAVE, CV_OnOff };

// [Arcade] Attract volume, as a percentage of the ordinary volumes above.
// An arcade cabinet advertises itself with sound, but a machine that lives in
// a house cannot do it at the same volume as the game all day.  0 is a silent
// attract screen; 100 is the stock behaviour.  Applied to sound and music
// together, so one setting covers "how loud is the cabinet when nobody is
// playing" -- which is the question being asked.
CV_PossibleValue_t attractvolume_cons_t[] = { {0, "MIN"}, {100, "MAX"}, {0, NULL} };
consvar_t cv_attractvolume = { "attractvolume", "50", CV_SAVE, attractvolume_cons_t };

// number of channels available
static void SetChannelsNum(void);
consvar_t cv_numChannels = { "snd_channels", "16", CV_SAVE | CV_CALL, CV_byte, SetChannelsNum };

#ifdef SURROUND_SOUND
consvar_t cv_surround = { "surround", "0", CV_SAVE, CV_OnOff };
#endif

#define S_MAX_VOLUME            127

// when to clip out sounds
// Does not fit the large outdoor areas.
// added 2-2-98 in 8 bit volume control (befort  (1200*0x10000))
// Is 1200 in Boom, 1600 in Heretic.
#define S_FAR_DIST         1200

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
// Is 200 in Boom, 0 In Heretic.
// added 2-2-98 in 8 bit volume control (befort  (160*0x10000))
#define S_CLOSE_DIST        160

// Adjustable by menu.
#define NORM_VOLUME             snd_MaxVolume

#define NORM_PITCH              128
#define NORM_PRIORITY           64

#define S_PITCH_PERTURB         1
#define S_STEREO_SWING          (96<<FRACBITS)


// percent attenuation from front to back
#define S_IFRACVOL              30

typedef struct
{
    // When empty, sfxinfo=NULL, priority=-0x3FFF.
    // sound information (if null, channel avail.)
    sfxinfo_t * sfxinfo;
    const xyz_t * origin;    // origin of sound
    int16_t   priority;  // Heretic style signed priority, adjusted for dist,
    int       handle;    // handle of the sound being played
} channel_t;

// The set of channels available.
// Number of channels is set by cv_numChannels
static channel_t *channels;

// whether songs are mus_paused
static boolean mus_paused;

// music currently being played
static musicinfo_t *mus_playing = NULL;


// [WDJ] unused
#ifdef CLEANUP
static int nextcleanup;
#endif


//
// Internals.
//
typedef struct {
    int volume;
    int sep;  // +/- 127, <0 is left, >0 is right
    int pitch;
    int dist; // integer part of sound distance
} sound_param_t;

static
boolean S_AdjustSoundParams(const mobj_t * listener, const xyz_t * source,
                            /*OUT*/ sound_param_t * sp );

static void S_StopChannel(int cnum);


// Required, even if dedicated
static
consvar_t * sound_ded_cvar_list[] =
{
    // Any cv_ with CV_SAVE needs to be registered, even if it is not used.
    // Otherwise there will be error messages when config is loaded.
  &cv_stereoreverse,
  &cv_precachesound,
#ifdef SURROUND_SOUND
  &cv_surround,
#endif
#ifdef MUSIC_SOURCE_CONTROL
  &cv_music_source,
#endif
  NULL
};


// Only if not dedicated.
static
consvar_t * sound_option_cvar_list[] =
{
    // Port specific Controls
#ifdef SOUND_DEVICE_OPTION
  &cv_snd_opt,
#endif
#ifdef SNDSERV
  &cv_sndserver_cmd,
  &cv_sndserver_arg,
#endif
#ifdef MUSSERV
  &cv_musserver_cmd,
  &cv_musserver_arg,
  &cv_musserver_opt,
#endif
#ifdef MIDI_OPTIONS_CONTROL
  &cv_midi_create_program,
  &cv_midi_compress,
#endif
  NULL
};

void S_Register_SoundStuff(void)
{
    CV_RegisterVar_list( sound_ded_cvar_list );
    if (dedicated)
        return;

    CV_RegisterVar_list( sound_option_cvar_list );

#if 0
//[WDJ]  disabled in 143beta_macosx
//[segabor]
#ifdef MACOS_DI        //mp3 playlist stuff
// specific to macos directory
    {
        int i;
        for (i = 0; i < PLAYLIST_LENGTH; i++)
        {
            user_songs[i].name = malloc(7);
            sprintf(user_songs[i].name, "song%i%i", i / 10, i % 10);
            user_songs[i].defaultvalue = malloc(1);
            *user_songs[i].defaultvalue = 0;
            user_songs[i].flags = CV_SAVE;
            user_songs[i].PossibleValue = NULL;
            CV_RegisterVar(&user_songs[i]);
        }
        CV_RegisterVar(&play_mode);
    }
#endif
#endif
}

static void SetChannelsNum(void)
{
    int i;

    // Allocating the internal channels for mixing
    // (the maximum number of sounds rendered
    // simultaneously) within zone memory.
    if (channels)
        Z_Free(channels);

#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        HW3S_SetSourcesNum();
        return;
    }
#endif
    channels = (channel_t *) Z_Malloc(cv_numChannels.value * sizeof(channel_t), PU_STATIC, 0);

    // Free all channels for use
    for (i = 0; i < cv_numChannels.value; i++)
    {
        channels[i].sfxinfo = NULL;
        channels[i].origin = NULL;
    }

}

void S_InitRuntimeMusic()
{
    int i;

    for (i = mus_firstfreeslot; i < mus_lastfreeslot; i++)
        S_music[i].name = NULL;
}


// [WDJ] Common routine to handling sfx names and get the sound lump.
// Much easier to maintain here.
// Replace S_GetSfxLumpNum
// Called by I_GetSfx
void S_GetSfxLump( sfxinfo_t * sfx )
{
    char lmpname_buf[20] = "\0\0\0\0\0\0\0\0";  // do not leave this to chance [WDJ]
    char * lumpname_p = lmpname_buf;
    byte * sfx_lump_data;
    lumpnum_t  sfx_lumpnum;

    if (EN_heretic) {	// [WDJ] heretic names are different
       sprintf(lmpname_buf, "%s", sfx->name);
    }else{
       sprintf(lmpname_buf, "ds%s", sfx->name);
    }

    // Now, there is a severe problem with the sound handling,
    // in it is not (yet/anymore) gamemode aware. That means, sounds from
    // DOOM II will be requested even with DOOM shareware.
    // The sound list is wired into sounds.c, which sets the external variable.
    // I do not do runtime patches to that variable. Instead, we will use a
    // default sound for replacement.

    if( ! VALID_LUMP( W_CheckNumForName(lmpname_buf) ) )
    {
        // sound not found
        // try plain name too (hth2.wad amb*)
        if( VALID_LUMP( W_CheckNumForName(sfx->name) ) )
        {
            lumpname_p = sfx->name;
            goto lump_found;
        }

        // [WDJ] Wads are getting more prone to bugs that cause us segfaults.
        // Protect ourselves.

        if( verbose > 1 )
            GenPrintf(EMSG_ver, "Sound missing: %s, Using default sound\n", lmpname_buf);

        // Heretic shareware: get many missing sound names at sound init,
        // but not after game starts.  These come from list of sounds
        // in sounds.c, but not all those are in the game.
        lumpname_p = (EN_heretic) ? "keyup" : "dspistol";

        if( ! VALID_LUMP( W_CheckNumForName(lumpname_p) ) )
        {
            // It may be that there are no wads with valid sound lumps,
            // so fail gracefully.
            return;
        }
    }

 lump_found:
    sfx_lumpnum = W_GetNumForName(lumpname_p);
    // if lump not found, W_GetNumForName would have done I_Error
    sfx->lumpnum = sfx_lumpnum;

    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sfx->length = W_LumpLength(sfx_lumpnum);
    // Copy is necessary because lump may be used by multiple sfx.
    // Free of shared lump would corrupt other sfx using it.
    sfx_lump_data = W_CacheLumpNum(sfx_lumpnum, PU_SOUND);
    sfx->data = Z_Malloc( sfx->length, PU_SOUND, 0 );
    memcpy( sfx->data, sfx_lump_data, sfx->length );
    Z_ChangeTag( sfx_lump_data, PU_CACHE );

#ifdef DEBUG_SOUND_HEADER
    // DEBUG
    byte * h = (byte*) sfx->data;
    printf( "sound header  %0i %0i  %0i %0i  %0i %0i  %0i %0i\n",
             h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7] );
#endif
   
    // sound data header format
    // 0,1: 03
    // 2,3: sample rate (11,2B)=11025, (56,22)=22050
    // 4,5: number of samples
    // 6,7: 00

    // caller must fix size and data ptr for the mixer
}


// [WDJ] Common routine to Get data for a sfx
static void S_GetSfx( sfxinfo_t * sfx )
{
    if ( sfx->name )
    {
//        debug_Printf("cached sound %s\n", sfx->name);
        if( sfx->link_id )
        {
            // [WDJ] Very rarely used, chaingun mostly.
            sfxinfo_t * link = & S_sfx[sfx->link_id];
            // NOTE: linked sounds use the link data at StartSound time
            // Example is the chaingun sound linked to pistol.
            if( ! link->data )
                I_GetSfx( link );

            // OK, even if I_GetSfx failed.
            // Linked to previously loaded
            sfx->data = link->data;
            sfx->length = link->length;
        }
        else
        {
            // Load data from WAD file.
            I_GetSfx( sfx );
        }
    }
}

// [WDJ] Common routine to Free data for a sfx
void S_FreeSfx( sfxinfo_t * sfx )
{
    if( sfx->link_id )  // do not free linked data
    {
        sfx->data = NULL; // ptr to shared data
    }
    else if( sfx->data )
    {
        I_FreeSfx( sfx );  // some must free their own buffers

        if( sfx->data )    // if not already free
        {
            Z_Free( sfx->data );
            sfx->data = NULL;
        }
    }
}


//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume)
{
    sfxid_t i;

    if (dedicated)
        return;

    //debug_Printf( "S_Init: default sfx volume %d\n", sfxVolume);

    S_SetSfxVolume(sfxVolume);
    S_SetMusicVolume(musicVolume);

    SetChannelsNum();

    // no sounds are playing, and they are not mus_paused
    mus_paused = false;

    // Note that sounds have not been cached (yet).
    for (i = 1; i < NUMSFX_EXT; i++)
    {
        sfxinfo_t * sfx = & S_sfx[i];
        sfx->usefulness = -1;    // for I_GetSfx()
        sfx->lumpnum = NO_LUMP;
        sfx->data = NULL;
        sfx->length = 0;
#if 1
        // [WDJ] Single Saw sound fix.
        // SFX_saw marks some additional sounds that may need SFX_single.
        // The need for these may be obsolete.
        // Need to know the situation that required single saw sound.
        if( sfx->flags & SFX_saw )
           sfx->flags |= SFX_single;
#endif
    }

    //
    //  precache sounds if requested by cmdline, or cv_precachesound var true
    //
    if (!nosoundfx && (M_CheckParm("-precachesound") || cv_precachesound.value))
    {
        // Initialize external data (all sounds) at start, keep static.
//        GenPrintf(EMSG_info, "Loading sounds... ");
        GenPrintf(EMSG_info, "Caching sound data (%d sfx)... ", NUMSFX_DEF);

        for (i = 1; i < NUMSFX_DEF; i++)
        {
            // NOTE: linked sounds use the link's data at StartSound time
            if (S_sfx[i].name && !S_sfx[i].link_id)
                S_GetSfx( & S_sfx[i] );
        }

#if 0
// [WDJ] From linux_x i_sound.
// This should not be done in sound driver.
// If this is to be done anywhere, it should be here.
// Do not know of any need to do this, but keep it until figure out its history.
        // Do we have a sound lump for the chaingun?
        if (W_CheckNumForName("dschgun") == -1)
        {
            // No, so link it to the pistol sound
            S_sfx[sfx_chgun].link_id = sfx_pistol;
            S_sfx[sfx_chgun].pitch = 150;
            S_sfx[sfx_chgun].volume = 0;
            S_sfx[sfx_chgun].data = 0;
            GenPrintf(EMSG_info, "linking chaingun sound to pistol sound,");
        }
        else
        {
            GenPrintf(EMSG_info, "found chaingun sound,");
        }
#endif

        GenPrintf(EMSG_info, " pre-cached all sound data\n");
    }
}


//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//

//SoM: Stop all sounds, load level info, THEN start sounds.
void S_Stop_LevelSound(void)
{
    int cnum;

#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        HW3S_Stop_LevelSound();
        return;
    }
#endif

    // kill all playing sounds at start of level
    //  (trust me - a good idea)
    for (cnum = 0; cnum < cv_numChannels.value; cnum++)
    {
        S_StopChannel(cnum);  // has all tests needed
    }
}

// Called by P_SetupLevel.
void S_Start_LevelSound(void)
{
    int mnum;

    // start new music for the level
    mus_paused = false;

    if (gamemode == doom2_commercial)
        mnum = mus_runnin + gamemap - 1;
    else if (gamemode == heretic)
        mnum = mus_he1m1 + (gameepisode - 1) * 9 + gamemap - 1;
    else
    {
        const int spmus[] = {
            // Song - Who? - Where?

            mus_e3m4,   // American     e4m1
            mus_e3m2,   // Romero       e4m2
            mus_e3m3,   // Shawn        e4m3
            mus_e1m5,   // American     e4m4
            mus_e2m7,   // Tim  e4m5
            mus_e2m4,   // Romero       e4m6
            mus_e2m6,   // J.Anderson   e4m7 CHIRON.WAD
            mus_e2m5,   // Shawn        e4m8
            mus_e1m9    // Tim          e4m9
        };

        if (gameepisode < 4)
            mnum = mus_e1m1 + (gameepisode - 1) * 9 + gamemap - 1;
        else
            mnum = spmus[gamemap - 1];
    }

    // HACK FOR COMMERCIAL
    //  if (gamemode==doom2_commercial && mnum > mus_e3m9)
    //      mnum -= mus_e3m9;

    if (info_music && *info_music)
        S_ChangeMusicName(info_music, true);
    else
        S_ChangeMusic(mnum, true);

#ifdef CLEANUP
    nextcleanup = 15;
#endif
}


//
// S_get_channel :
//   Kill origin sounds, dependent upon sfx flags.
//   Reuse the channel, or find another channel.
//   Return channel number, if none available, return -1.
//
//  priority : Heretic style ascending signed priority adjusted for distance
static
int S_get_channel(const xyz_t * origin, sfxinfo_t * sfxinfo,
                         int16_t priority )
{
    // [WDJ] Like PrBoom, separate channel for player tagged sfx
    uint32_t kill_flags = (sfxinfo->flags & (SFX_player|SFX_saw)) | SFX_org_kill;
    int16_t low_priority = priority;  // neg is lower priority
    int pick_cnum = -1;
    int chanlimit = sfxinfo->limit_channels; // 1..99
    int cnum;  // channel number to use
    channel_t * c;

    // Using the Heretic system, higher num is higher priority.
    // Priority adjusted by dist.
    //  pri *= ( 10 - (dist/160));

    // Find an open channel, or lowest priority
    // Stop previous origin sound, so do not break from loop
    // Done in one loop for efficiency
    for (cnum = cv_numChannels.value-1; cnum >= 0 ; cnum--)
    {
        c = & channels[cnum];
        // stop previous origin sound
        if (origin && c->origin == origin)
        {
            if( ! c->sfxinfo )  goto reuse_cnum;  // empty
            // reuse channel with same origin, flags, when SFX_org_kill
            if((c->sfxinfo->flags & (SFX_player|SFX_saw|SFX_org_kill)) == kill_flags )
                goto reuse_cnum;
        }
        if (! c->sfxinfo)   // empty
        {
            pick_cnum = cnum;
            low_priority = -0x3FFF;  // empty is already lowest priority
            continue;
        }
        // Heretic style channel limits per sfx.
        if (c->sfxinfo == sfxinfo) 
            chanlimit --;
        // Find lowest priority ( neg is lowest ).
        if (c->priority < low_priority)
        {
            pick_cnum = cnum;
            low_priority = c->priority;
        }
    }

    // Heretic style channel limits.
    if( chanlimit <= 0 )
#if 1     
        priority -= (NORM_PRIORITY/2);  // soft limit
#else
        return -1;  // already at or over limit
#endif

    cnum = pick_cnum;
    if( pick_cnum >= 0 )
    {
        if( low_priority == -0x3FFF )  // found empty
            goto use_cnum;
        if( priority >= low_priority )  // can replace this sound
            goto reuse_cnum;
    }
    // No lower priority.  Sorry, Charlie.
    return -1;

 reuse_cnum:
    S_StopChannel(cnum);
 use_cnum:   
    c = &channels[cnum];

    // channel is decided to be cnum.
    c->sfxinfo = sfxinfo;
    c->priority = priority;
    c->origin = origin;

    return cnum;
}


// Does the sfx special case handling for all drivers.
// [WDJ] Due to special mobj tests that this has acquired, sectors were
// being cast as mobj and tested for fields they do not have.  Have split
// the sound origin parameter from the mobj attribute tests.
//  volume : 0..255
//  origin : x,y,z of the sector or mobj (saved, don't use temps)
//  mo : the origin mobj, for testing attributes
// Called by StartSound.
// Called by hardware S_StartAmbientSound.
static
void S_StartSoundAtVolume(const xyz_t * origin, const mobj_t * mo,
                          sfxid_t sfx_id, int volume,
                          channel_type_t ct_type )
{
    sound_param_t sp1;
    int priority;  // Heretic style signed priority, nominally -10 .. 2560.
    sfxinfo_t * sfx;
    int cnum;

    if (nosoundfx || (mo && mo->type == MT_SPIRIT))
        goto done;

#if 0
    if( EN_heretic )
    {
        if( origin == NULL )
            origin = & consoleplayer_ptr->mo->x;
        // volume = (volume*(snd_MaxVolume+1)*8)>>7;
    }
#endif
   
#if 0
    // Debug.
    debug_Printf( "S_StartSoundAtVolume: playing sound %d (%s), volume = %i\n",
                sfx_id, S_sfx[sfx_id].name, volume );
#endif

#ifdef PARANOIA
    // check for bogus sound #
    if (sfx_id < 1 || sfx_id > NUMSFX_EXT)
    {
        I_SoftError("Bad sfx #: %d\n", sfx_id);
        goto done;
    }
#endif

    sfx = &S_sfx[sfx_id];
//    priority = sfx->priority;  // Heretic
//    priority = NORM_PRIORITY;  // Boom
    sp1.pitch = NORM_PITCH;
    sp1.sep = 0;

    if( (sfx->skinsound < NUMSKINSOUNDS) && mo && mo->skin )
    {
        // redirect player sound to the sound in the skin table
        sfx_id = ((skin_t *) mo->skin)->soundsid[sfx->skinsound];
        sfx = &S_sfx[sfx_id];
    }

    // Initialize sound parameters
    if( sfx->link_id )
    {
        // [WDJ] Very rarely used, chaingun mostly.
        sfxinfo_t * link = & S_sfx[sfx->link_id];
        if( sfx->link_mod > 0 )
        {
            // Doom only
            // Only modifies pitch, and we don't even implement that.
            // The only link entry had NORM_PRIORITY.
            sp1.pitch = link_mods[sfx->link_mod].pitch;
//          priority = sfx->priority;  // Boom
            volume += link_mods[sfx->link_mod].mod_volume;
#if 0
            // There are no sfx link mods that would trigger this.
            if (volume < 1)
                goto done;
#endif
        }

        // added 2-2-98 SfxVolume is now the hardware volume, don't mix up
        //    if (volume > SfxVolume)
        //      volume = SfxVolume;

        // update reference from link, it may have been purged
        sfx->data = link->data;  // ref to shared data
        sfx->length = link->length;
    }
    else
    {
//        pitch = NORM_PITCH;
//        priority = NORM_PRIORITY;  // Boom ignored the sfx priority.
    }

    sp1.volume = volume;
    sp1.dist = 0;
    // Check to see if it is audible,
    //  and if not, modify the params

    //added:16-01-98:changed consoleplayer to displayplayer
    //[WDJ] added displayplayer2_ptr tests, stop segfaults
    if( origin
        && (mo != displayplayer_ptr->mo)
        && !(cv_splitscreen.value && displayplayer2_ptr
             && (mo == displayplayer2_ptr->mo) ) )
    {
        sound_param_t sp2 = sp1;  // must save before AdjustSound
        boolean audible1, audible2;

        audible1 = S_AdjustSoundParams(displayplayer_ptr->mo, origin, &sp1);

        // sp1 has been adjusted for dist and angle, optional additional adjustments follow.
        if (cv_splitscreen.value && displayplayer2_ptr)
        {
            // splitscreen sound for player2
            audible2 = S_AdjustSoundParams(displayplayer2_ptr->mo, origin, &sp2);
            if (!audible2)
            {
                if (!audible1)
                    goto done;
            }
            else if (!audible1 || (audible1 && (sp2.volume > sp1.volume)))
            {
                sp1 = sp2;  // as heard by player 2
                if (origin->x == displayplayer2_ptr->mo->x
                    && origin->y == displayplayer2_ptr->mo->y)
                {
                    sp1.sep = 0;
                }
            }
        }
        else if (!audible1)
            goto done;

        if (origin->x == displayplayer_ptr->mo->x
            && origin->y == displayplayer_ptr->mo->y)
        {
            sp1.sep = 0;
        }
    }
    else
    {
        sp1.sep = 0;
    }

    // hacks to vary the sfx pitches

    //added:16-02-98: removed by Fab, because it used M_Random() and it
    //                was a big bug, and then it doesnt change anything
    //                dont hear any diff. maybe I'll put it back later
    //                but of course not using M_Random().
    //added 16-08-02: added back by Judgecutor
    //Sound pitching for both Doom and Heretic
    if( cv_rndsoundpitch.EV )
    {
        if (EN_heretic)
        {
            // Heretic
            sp1.pitch = 128 + (M_Random() & 7);
            sp1.pitch -= (M_Random() & 7);
        }
        else
        {
            // From Boom
            if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
                sp1.pitch += 8 - (M_Random() & 15);
            else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
                sp1.pitch += 16 - (M_Random() & 31);
        }
    }
    else if( demoplayback )
    {
        M_Random(); M_Random();  // to keep demo sync
    }

    if (sp1.pitch < 0)
        sp1.pitch = NORM_PITCH;
    if (sp1.pitch > 255)
        sp1.pitch = 255;

    if( EN_heretic )
    {
        // Heretic highest priority is 256, lowest 1.
        priority = sfx->priority * (10 - (sp1.dist/160) );
    }
    else
    {
        // [WDJ] Boom was using NORM_PRIORITY for everything, ignoring the
        // sfx priority, but we have the Heretic system, so why not use it.
        // Doom highest priority is 1, lowest is 256.
        priority = 257 - sfx->priority;  // Convert to Heretic system.
        // Because of wads with 100 monsters, mod for dist too.
        priority -= NORM_PRIORITY * sp1.dist / S_FAR_DIST;
    }

#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        HW3S_I_StartSound(origin, NULL, ct_type, sfx_id, priority,
                          sp1.volume, sp1.pitch, sp1.sep);
        goto done;
    };
#endif

    // Kill origin sound, reuse channel, or find a channel
    // Dependent upon sfx flags
    cnum = S_get_channel(origin, sfx, priority);
    if (cnum < 0)
        goto done;

    // cache data if necessary
    // NOTE : set sfx->data NULL sfx->lump -1 to force a reload
    if (!sfx->data)
        S_GetSfx( sfx );  // handles linked sfx too

    // [WDJ] usefulness of a recent sound
    if( sfx->usefulness < 10 )
       sfx->usefulness = 10;  // min
    else if( sfx->usefulness > 800 )
       sfx->usefulness = 800;  // max
    sfx->usefulness += 3;   // increasing

    // [WDJ] From PrBoom, wad dakills has zero length sounds
    // (DSBSPWLK, DSBSPACT, DSSWTCHN, DSSWTCHX)
    if (sfx->length <= 0)
       goto done;

#ifdef SURROUND_SOUND
    // judgecutor:
    // Avoid channel reverse if surround
    if (cv_stereoreverse.value && sp1.sep < SURROUND_SEP )
        sp1.sep = -sp1.sep;
#else
    //added:11-04-98:
    if (cv_stereoreverse.value)
        sp1.sep = -sp1.sep;
#endif

//    debug_Printf("stereo sep %d reverse %d\n", sp1.sep, cv_stereoreverse.value);

    // Returns a handle to a mixer/output channel.
    channels[cnum].handle =
      I_StartSound(sfx_id, sp1.volume, sp1.sep, sp1.pitch, priority);
done:
    return;
}

// Most sfx sounds are called through this interface.
//  origin : the position
//  mo : mobj for testing attributes
static inline
void S_StartNormSound( const xyz_t * origin, const mobj_t * mo, sfxid_t sfx_id )
{
    // the volume is handled 8 bits
    S_StartSoundAtVolume( origin, mo, sfx_id, 255, CT_NORMAL );
}

// Most plain sfx sounds are called through this interface.
void S_StartSound( sfxid_t sfx_id )
{
    S_StartNormSound( NULL, NULL, sfx_id );  // No origin
}

// Most switch sounds are called through this interface.
void S_StartXYZSound( const xyz_t * origin, sfxid_t sfx_id )
{
    S_StartNormSound( origin, NULL, sfx_id );  // No mobj
}

// Most sector sfx sounds are called through this interface.
void S_StartSecSound( const sector_t *sec, sfxid_t sfx_id )
{
    S_StartNormSound( &sec->soundorg, NULL, sfx_id );  // xyz_t *
}

// Most Mobj sfx sounds are called through this interface.
void S_StartObjSound( const mobj_t * mo, sfxid_t sfx_id )
{
    // Requires that the x,y,z in an mobj_t be the same as xyz_t.
    S_StartNormSound( (xyz_t*)&(mo->x), mo, sfx_id );  // xyz_t *
}

void S_StartAttackSound(const mobj_t * mo, sfxid_t sfx_id)
{
    S_StartSoundAtVolume( (xyz_t*)&(mo->x), mo, sfx_id, 255, CT_ATTACK);
}

void S_StartScreamSound(const mobj_t * mo, sfxid_t sfx_id)
{  
    S_StartSoundAtVolume( (xyz_t*)&(mo->x), mo, sfx_id, 255, CT_SCREAM);
}

void S_StartAmbientSound(sfxid_t sfx_id, int volume)
{
#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        volume += 30;
        if (volume > 255)
            volume = 255;
    }
#endif
    S_StartSoundAtVolume(NULL, NULL, sfx_id, volume, CT_AMBIENT);
}

//
// S_StartSoundName
//  origin : the position
//  mo : mobj for testing attributes
// Starts an general sound using the given name.
// Called from Fraggle script.
void S_StartXYZSoundName(const xyz_t * origin, const mobj_t * mo,
                         const char * soundname)
{
    int sfxid;
   
    //Search existing sounds...
    for (sfxid = sfx_None + 1; sfxid < NUMSFX_EXT; sfxid++)
    {
        if (!S_sfx[sfxid].name)
            continue;

        if (!strcasecmp(S_sfx[sfxid].name, soundname))
            goto play_sfx;  // found name
    }

    // add soundname to S_sfx
    // [WDJ] S_AddSoundFx now handles search for free slot and remove
    // of least useful sfx when full.
    sfxid = S_AddSoundFx(soundname, 0);

 play_sfx:
    S_StartNormSound(origin, mo, sfxid);
}


static
void S_StopXYZSound(const xyz_t * origin)
{
    int cnum;

    // SoM: Sounds without origin can have multiple sources, they shouldn't
    // be stopped by new sounds.
    if (!origin)
        return;

#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        HW3S_StopSound(origin);
        return;
    }
#endif
    for (cnum = 0; cnum < cv_numChannels.value; cnum++)
    {
        if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
        {
            if( (channels[cnum].sfxinfo->flags & SFX_org_kill) )
            {
                S_StopChannel(cnum);
            }
        }
    }
}

void S_StopSecSound(const sector_t *sec)
{
    S_StopXYZSound( &sec->soundorg );
}

void S_StopObjSound(const mobj_t *mo)
{
    S_StopXYZSound( (xyz_t*)&mo->x );
}


#ifdef MBF21
// MBF21
// [WDJ] These look like sound support functions for MBF21, from DSDS-Doom.
// Not much choice.

void S_LoopSound( void * origin, int sfx_id, int timeout )
{
//    S_StartSoundAtVolume_MBF21( origin, sfx_id, EN_heretic_hexen ? 127 : sfx_volume, timeout );
//    S_StartSoundAtVolume( origin, NULL, sfx_id, EN_heretic_hexen ? 127 : volume, timeout );
    S_StartSoundAtVolume( origin, NULL, sfx_id, EN_heretic_hexen ? 127 : 200, CT_AMBIENT );
//    S_StartSoundAtVolume( origin, mo, sfx_id, 255, CT_NORMAL );
}

// [WDJ] These are all are guarded by SECF_SILENT, which is UDMF.
// We do not implement UDMF.
// For now, these mostly duplicate existing sound functions.
// At least until we get past debugging the installation of MBF21.

void S_StartSectorSound( sector_t * sector, int sfx_id )
{
#ifdef UDMF
    if( sector->flags & SECF_SILENT )
      return;
#endif    

    S_StartNormSound( &sector->soundorg, NULL, sfx_id );
}

void S_LoopSectorSound( sector_t * sector, int sfx_id, int timeout )
{
#ifdef UDMF
    if( sector->flags & SECF_SILENT )
      return;
#endif

    S_LoopSound( (mobj_t *) &sector->soundorg, sfx_id, timeout );
}

void S_StartMobjSound( mobj_t * mobj, int sfx_id )
{
#ifdef UDMF
    if( mobj && mobj->subsector
        && mobj->subsector->sector->flags & SECF_SILENT )
      return;
#endif

    // Requires that the x,y,z in an mobj_t be the same as xyz_t.
    S_StartNormSound( (xyz_t*)&(mobj->x), mobj, sfx_id );  // xyz_t *
//    S_StartXYZSound(mobj, sfx_id);
}

void S_LoopMobjSound( mobj_t * mobj, int sfx_id, int timeout )
{
#ifdef UDMF
    if( mobj && mobj->subsector
        && mobj->subsector->sector->flags & SECF_SILENT )
      return;
#endif

    S_LoopSound(mobj, sfx_id, timeout);
}

void S_StartVoidSound( int sfx_id )
{
    S_StartSound( sfx_id );
}

void S_LoopVoidSound( int sfx_id, int timeout )
{
    S_LoopSound( NULL, sfx_id, timeout );
}

void S_StartLineSound( line_t * line, xyz_t * soundorg, int sfx_id )
{
#ifdef UDMF
    if( line && line->frontsector
        && line->frontsector->flags & SECF_SILENT )
      return;
#endif

    S_StartXYZSound( soundorg, sfx_id );
}

#endif



//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
    if (mus_playing && !mus_paused)
    {
        I_PauseSong(mus_playing->handle);
        mus_paused = true;
    }

#ifdef CDMUS
    // pause cd music
    I_PauseCD();
#endif
}

void S_ResumeSound(void)
{
    if (mus_playing && mus_paused)
    {
        I_ResumeSong(mus_playing->handle);
        mus_paused = false;
    }

#ifdef CDMUS
    // resume cd music
    I_ResumeCD();
#endif
}

//
// Updates music & sounds
//

// The volumes for the hardware and software mixers.
// Range 0..31
int mix_sfxvolume = 0;
int mix_musicvolume = 0;

// [Arcade] vol scaled by cv_attractvolume when the attract cycle is on screen,
// unchanged when a game is being played.
static int S_Attract_Scaled( int vol )
{
    if( ! D_Attract_Running() )  return vol;

    // [Arcade] The moment somebody presses a key the cabinet is in use, even
    // though the attract cycle is technically still what is on screen: any
    // keypress over a demo raises the menu, and the menu's own sounds were
    // then played at advertising volume -- the first thing a player hears
    // after touching the machine came out quieter than the demo that drew
    // them to it.
    //
    // D_Menu_Over_Attract() is exactly "a menu is open over the attract
    // screen", already defined for the menu backdrop, so both readers share
    // one definition rather than this growing a second opinion about what
    // counts as the attract screen.  Backing out of the menu without starting
    // anything drops the volume again by itself, since this is reconciled
    // every frame.
    if( D_Menu_Over_Attract() )  return vol;

    return (vol * cv_attractvolume.value) / 100;
}


// Called by D_DoomLoop upon tics.
// Not called when dedicated.
void S_UpdateSounds(void)
{
    sound_param_t sp1;
    int cnum;
    sfxinfo_t *sfx;
    channel_t *c;

    mobj_t *listener = displayplayer_ptr->mo;

    // Update sound/music volumes, if changed manually at console.
    //
    // [Arcade] ... and scale them down while the attract cycle is what is on
    // screen.  This is the only place the mixer volume is reconciled with the
    // cvars, and it runs every frame from D_DoomLoop whatever the gamestate,
    // so the attract scaling has to happen *here*: setting the mixer directly
    // from somewhere else would be undone by this comparison on the next
    // frame.  Coming out of attract into a game is likewise automatic -- the
    // target changes and this restores full volume within a frame.
    //
    // Not skipped in devmode, unlike most cabinet behaviour: an operator
    // changing this setting needs to hear what it does, and there is no
    // lockdown reason to suppress it.
    {
        int  want_sfx = S_Attract_Scaled( cv_soundvolume.value );
        int  want_mus = S_Attract_Scaled( cv_musicvolume.value );

        if (mix_sfxvolume != want_sfx)
            S_SetSfxVolume(want_sfx);
        if (mix_musicvolume != want_mus)
            S_SetMusicVolume(want_mus);
    }

#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        HW3S_UpdateSources();
        return;
    }
#endif

#ifdef CLEANUP
       Clean up unused data.
       if (gametic > nextcleanup)
       {
       for (i=1 ; i<NUMSFX ; i++)
       {
       if (S_sfx[i].usefulness==0)
       {
       //S_sfx[i].usefulness--;

       // don't forget to unlock it !!!
       // __dmpi_unlock_....
       //Z_ChangeTag(S_sfx[i].data, PU_CACHE);
       //S_sfx[i].data = 0;

       CONS_Printf ("\2flushed sfx %.6s\n", S_sfx[i].name);
       }
       }
       nextcleanup = gametic + 15;
       }
#endif

    for (cnum = 0; cnum < cv_numChannels.value; cnum++)
    {
        c = &channels[cnum];
        sfx = c->sfxinfo;

        if (c->sfxinfo)
        {
            if ( ! I_SoundIsPlaying(c->handle))
            {
                // if channel is allocated but sound has stopped,
                //  free it
                S_StopChannel(cnum);
                continue;
            }

            // Sound is still playing, adjust for player or source movement.
            // Initialize parameters
            sp1.volume = 255;   //8 bits internal volume precision
            sp1.pitch = NORM_PITCH;
            sp1.sep = 0;

            if( sfx->link_id )  // strange (BP)
            {
                // [WDJ] Very rarely used, chaingun mostly.
//                sfxinfo_t * link = & S_sfx[sfx->link_id];
                if( sfx->link_mod > 0 )
                {
                    // Doom only
                    // Only modifies pitch, and we don't even implement that.
                    sp1.pitch = link_mods[sfx->link_mod].pitch;
                    sp1.volume += link_mods[sfx->link_mod].mod_volume;
#if 0
                    // There are no sfx link mods that would trigger this.
                    if (sp1.volume < 1)
                    {
                        S_StopChannel(cnum);
                        continue;
                    }
#endif
                }
            }

            // check non-local sounds for distance clipping
            //  or modify their params
            if (c->origin
                && (((xyz_t*)&listener->x) != c->origin)
                && !(cv_splitscreen.value && displayplayer2_ptr
                     && (c->origin == (xyz_t*)&displayplayer2_ptr->mo->x) ) )
            {
                sound_param_t sp2 = sp1;
                boolean audible1, audible2;

                audible1 = S_AdjustSoundParams(listener, c->origin, &sp1);

                if (cv_splitscreen.value && displayplayer2_ptr)
                {
                    // splitscreen sound for player2
                    audible2 = S_AdjustSoundParams(displayplayer2_ptr->mo, c->origin, &sp2);
                    if (audible2
                        && (!audible1 || (sp2.volume > sp1.volume)) )
                    {
                        audible1 = true;
                        sp1 = sp2;
                    }
                }

                if (!audible1)
                {
                    S_StopChannel(cnum);
                    continue;		   
                }

#ifdef SURROUND_SOUND
                // judgecutor:
                // Avoid channel reverse if surround
                if (cv_stereoreverse.value && sp1.sep < SURROUND_SEP )
                    sp1.sep = -sp1.sep;
#else
                if (cv_stereoreverse.value)
                    sp1.sep = -sp1.sep;
#endif
                I_UpdateSoundParams(c->handle, sp1.volume, sp1.sep, sp1.pitch);
            }
        }
    }
    // kill music if it is a single-play && finished
    // if (     mus_playing
    //      && !I_QrySongPlaying(mus_playing->handle)
    //      && !mus_paused )
    // S_StopMusic();

}

//  volume : volume control,  0..31
void S_SetMusicVolume(int volume)
{
    if (volume < 0 || volume > 31)
    {
        GenPrintf( EMSG_warn, "musicvolume should be between 0-31\n");
        volume = ( volume < 0 ) ? 0 : 31;  // clamp
    }

    mix_musicvolume = volume;  // check for change of var

    I_SetMusicVolume(volume);

#ifdef __DJGPP__
    I_SetMusicVolume(31);       //faB: this is a trick for buggy dos drivers.. I think.
#endif
}

//  volume : volume control,  0..31
void S_SetSfxVolume(int volume)
{
    if (volume < 0 || volume > 31)
    {
        GenPrintf( EMSG_warn, "sfxvolume should be between 0-31\n");
        volume = ( volume < 0 ) ? 0 : 31;  // clamp
    }

    mix_sfxvolume = volume; // check for change of var

#ifdef HW3SOUND
    hws_mode == HWS_DEFAULT_MODE ? I_SetSfxVolume(volume) : HW3S_SetSfxVolume(volume & 31);
#else
    // now hardware volume
    I_SetSfxVolume(volume);
#endif

}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
    S_ChangeMusic(m_id, true);
}

//
// S_ChangeMusicName
// Changes music by name
//   looping : non-zero if continuous looping of music
void S_ChangeMusicName( const char * name, byte looping )
{
    int music_id;

    if (!strncmp(name, "-", 6))
    {
        S_StopMusic();
        return;
    }

    music_id = S_FindMusic( name ); // standard names
    if( music_id == mus_None )
    {
        // Where new music does not have a lump with the standard name.	
        music_id = S_AddMusic( name, NULL );  // non-standard name, std prefix
    }
#ifdef MUSIC_SOURCE_CONTROL
    if( music_id == mus_None )
    {
        // Where new music does not have a lump with the standard name.	
        music_id = S_AddMusic( name, "o_" ); // MP3, OGG lumps
    }
#endif

    if (music_id > mus_None && music_id < NUMMUSIC)
    {
        S_ChangeMusic(music_id, looping);
    }
    else
    {
        GenPrintf(EMSG_warn, "Music not found: %s\n", name);
        S_StopMusic();  // stop music anyway
    }
}


// Detect the music type.
//   can_play_adm:  ADM_ of music types that can be played
byte detect_music_type( lumpnum_t music_ln, byte can_play_adm )
{
    byte  head[10];
    W_ReadLumpHeader( music_ln, &head, 8 );
    // Check music header identifiers
    byte music_type = MUSTYPE_OTHER;

// GenPrintf( EMSG_debug, "Music header %c%c%c%c\n", head[0], head[1], head[2], head[3] );
    if( memcmp(head,"MUS",3) == 0 )  // MUS
    {
        music_type = MUSTYPE_MUS;
    }
    else if( memcmp(head,"MThd",4) == 0 )  // MIDI
    {
        music_type = MUSTYPE_MIDI;
    }
# ifdef MUSIC_MP3
    else if( memcmp(head,"ID3",3) == 0 )  // MP3
    {
        music_type = MUSTYPE_MP3;
    }
    else if( memcmp(head,"\xFF\xFB\x90", 3) == 0 )  // MPEG III (mp3), using some other header style
    {
        music_type = MUSTYPE_MP3;
    }
# endif
# ifdef MUSIC_OGG
    else if( memcmp(head,"Ogg",3) == 0 )  // OGG
    {
        music_type = MUSTYPE_OGG;
    }
# endif

    if( ! (can_play_adm & music_type_to_ADM[music_type]) )  // have decoder for MP3, OGG, etc
        GenPrintf(EMSG_warn, "MUSIC %s: cannot play, header= %x %x %x\n", music_type_str[music_type], head[0], head[1], head[2] );

    return music_type;
}


void S_ChangeMusic(int music_num, byte looping)
{
    musicinfo_t * music;
#if defined(MUSIC_SOURCE_CONTROL) || ! defined(MUSSERV)
    byte  music_type = MUSTYPE_MUS;
#endif

    if (dedicated)
        return;

    if (nomusic)
        return;

    if ((music_num <= mus_None) || (music_num >= NUMMUSIC))
    {
        GenPrintf(EMSG_error, "Bad music number %d\n", music_num);
        return;
    }
    else
        music = &S_music[music_num];

#ifdef MUSIC_SOURCE_CONTROL
    if((mus_playing == music)
       && ( ! ( cv_music_source.state & CS_MODIFIED ) ) )
        return;
#else   
    if(mus_playing == music)
        return;
#endif

    // shutdown old music
    S_StopMusic();

    // get lumpnum if neccessary
    // Test of the music ever being looked up, not a test of VALID_LUMP.
#ifdef MUSIC_SOURCE_CONTROL
    music_num_playing = 0;
    if( (music->lumpnum == 0)  // lookup music test
        || ( cv_music_source.state & CS_MODIFIED ) )  // music source has changed
    {
        lumpnum_t music_ln = NO_LUMP;
        byte  adv_music = EN_port_music & EN_src_music;

        // MP3 or OGG
        if( adv_music & (ADM_MP3 | ADM_OGG) )
        {
            // Check for "o_name", which is MP3 or OGG music.
            music_ln = S_FindExtMusic( music->name, "o_" );  // Doom MP3
            if( VALID_LUMP(music_ln) )
            {
                music_type = detect_music_type( music_ln, EN_port_music );
                byte music_adm = music_type_to_ADM[ music_type ];
                if( ! (adv_music & music_adm) )  // check on select and required decoder
                    music_ln = NO_LUMP;
            }
        }

        if( ! VALID_LUMP(music_ln) )
        {
            // Default music names
            music_ln = S_FindExtMusic( music->name, NULL );  // Doom MP3
            if( VALID_LUMP(music_ln) )
            {
                music_type = detect_music_type( music_ln, EN_port_music );
            }
        }

        music->lumpnum = music_ln;
        if( ! VALID_LUMP(music_ln) )
            return;

# ifdef MUSIC_SELECT_ALT_IS_SILENCE
        // If cannot play MP3, OGG, then silence
        if( (cv_music_source.EV > 1)   // selected MP3, OGG, etc.
            && (music_type < MUSTYPE_MP3) )
            return;  // silence
# endif
    }
# ifndef MUSSERV
    else
    {
        music_type = detect_music_type( music->lumpnum, EN_port_music );
    }
# endif
#else
    // NOT MUSIC_SOURCE_CONTROL
    if( music->lumpnum == 0 ) // lookup music test
    {
        music->lumpnum = S_FindExtMusic( music->name, NULL );  // standard names
        // no more I_Error, graceful failure
    }
# ifndef MUSSERV
    if( VALID_LUMP(music->lumpnum) )
    {
        music_type = detect_music_type( music->lumpnum, EN_port_music );
    }
# endif
#endif

#ifdef MUSSERV
    // Play song, with information for ports with music servers.
    music->data = NULL;
    music->handle = I_PlayServerSong( music->name, music->lumpnum, looping );
#else
    // load & register it
    music->data = (void *) S_CacheMusicLump(music->lumpnum);
    music->handle = I_RegisterSong( music_type, music->data, W_LumpLength(music->lumpnum));
    // play it
    I_PlaySong(music->handle, looping);
#endif

    mus_playing = music;
#ifdef MUSIC_SOURCE_CONTROL
    music_num_playing = music_num;
#endif
#ifdef MIDI_OPTIONS_CONTROL
    midi_music_num_playing = music_num;
#endif
}


void S_StopMusic()
{
    if (mus_playing)
    {
        if (mus_paused)
            I_ResumeSong(mus_playing->handle);

        I_StopSong(mus_playing->handle);
        I_UnRegisterSong(mus_playing->handle);
#ifndef MUSSERV
        if( mus_playing->data )
            Z_ChangeTag(mus_playing->data, PU_CACHE);
        mus_playing->data = NULL;
#endif

        mus_playing = NULL;
    }
}

static void S_StopChannel(int cnum)
{
    channel_t *c = &channels[cnum];

    if (c->sfxinfo)
    {
        // stop the sound playing
        if (I_SoundIsPlaying(c->handle))
        {
            I_StopSound(c->handle);
        }

#if 0
// [WDJ] Does nothing       
        // check to see
        //  if other channels are playing the sound
        int i;
        for (i = 0; i < cv_numChannels.value; i++)
        {
            if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
            {
                break;
            }
        }
#endif

#ifdef CLEANUP
        // degrade usefulness of sound data
        c->sfxinfo->usefulness--;
#endif

        if( (c->sfxinfo->flags & SFX_org_kill) == 0 )
           c->origin = NULL;  // do not reuse
        c->sfxinfo = NULL;
        c->priority = -0x3FFF;
    }
}

//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
//   sp : /*OUT*/ sep, volume, 0..255
// Return true if the sound is audible.
static
boolean S_AdjustSoundParams(const mobj_t * listener, const xyz_t * source,
                            /*OUT*/ sound_param_t * sp )
{
    int approx_dist;  // integer part of dist
    fixed_t adx, ady;
    angle_t angle;
    int v;
   
    if( ! listener )  return 0;  // [WDJ] Stop splitscreen segfault.

    // calculate the distance to sound origin
    //  and clip it if necessary
    adx = abs(listener->x - source->x);
    ady = abs(listener->y - source->y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = (adx + ady - (((adx < ady)? adx : ady) >> 1)) >> FRACBITS;
    // [WDJ] Used everywhere coarsely, so pass integer part.
    sp->dist = approx_dist;

    // Original has MAP08 without sound clipping by distance
    // Boom    if(approx_dist > 1200)
    // Heretic if(approx_dist > 1600)
    // Vanilla Doom2: (gamemap != 8 && approx_dist > S_FAR_DIST)
    //   doom2 map 8 apparantly was a joke level.
    if ( approx_dist > S_FAR_DIST )
    {
        return false;  // not audible
    }

    // angle of source to listener
    angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

    if (angle > listener->angle)
        angle = angle - listener->angle;
    else
        angle = angle + (0xffffffff - listener->angle);

#ifdef SURROUND_SOUND
    // Produce a surround sound for angle from 105 till 255
    if (cv_surround.value
        && (angle > (ANG90 + (ANG45 / 3)) && angle < (ANG270 - (ANG45 / 3))))
        sp->sep = SURROUND_SEP;
    else
    {
#endif
        // stereo separation, <0 is left
        sp->sep =  - (FixedMul(S_STEREO_SWING, sine_ANG(angle)) >> FRACBITS);

#ifdef SURROUND_SOUND
    }
#endif

    // volume calculation
    // Multiplication by snd_SfxVolume has been moved to port drivers.
    // This generates a position relative volume, 0..255.
    if (approx_dist < S_CLOSE_DIST)
    {
        // added 2-2-98 SfxVolume is now hardware volume
        sp->volume = 255;     //snd_SfxVolume;
        return true;
    }

    // Original had MAP08 making distant sound effects louder than near.
    // removed hack here for gamemap==8 (it made far sound still present)
    if( EN_heretic )
    {
        // Heretic distance effect
        // Used sndmax: 0..31, default was 31.
        // Heretic  volume= (sndmax*16 + dist * (-sndmax*16)/MAX_SND_DIST) >> 9;
        //          volume= ((sndmax*16) - ((dist*sndmax*16)/MAX_SND_DIST)) >> 9;
        // Heretic has sndcurve lump:  volume= sndcurve[dist];
        v = ( (255*16) - ( (255*16*approx_dist) / S_FAR_DIST ) ) >> 4;
    }
    else
    {
        // Doom/Boom distance effect.
        // Used snd_SfxVolume: 0..15, default was 15.
        //  We use volume 0..255.
        // PrBoom: v = (snd_SfxVolume * ((S_FAR_DIST-approx_dist)>>FRACBITS)*8)
        //             / ((S_FAR_DIST-S_CLOSE_DIST)>>FRACBITS)
#if 1
        v = (240 * (S_FAR_DIST - approx_dist)) / (S_FAR_DIST-S_CLOSE_DIST);
#else
        // added 2-2-98 in 8 bit volume control (befort  remove the +4)
        // Range 0..240
        v = (15 * (S_FAR_DIST - approx_dist))
             / ((S_FAR_DIST-S_CLOSE_DIST)>>4);
//#define S_ATTENUATOR   ((S_FAR_DIST-S_CLOSE_DIST)>>(FRACBITS+4))
//      v = (15 * ((S_FAR_DIST - approx_dist) >> FRACBITS)) / S_ATTENUATOR;
#endif
    }

    if( v > 255 )
    {
        v = 255;
        if( devparm )
            GenPrintf( EMSG_dev, "AdjustSound maxxed volume.\n" );
    }
    sp->volume = v;

    return (v > 0);
}


// SoM: Searches through the channels and checks for origin or id.
//   origin : the origin position to check,  if NULL do not check it
//   sfxid : the sfx to check,  if sfx_None do not check it
// returns true if either is found.
// Is called by S_AddSoundFx (with origin==NULL)
boolean  S_SoundPlaying( xyz_t *origin, sfxid_t sfxid)
{
    sfxinfo_t * sfx;
    int cnum;

#ifdef HW3SOUND
    if (hws_mode != HWS_DEFAULT_MODE)
    {
        return HW3S_SoundPlaying(origin, id);
    }
#endif

    // Enable match test when sfxid specified.
    sfx = ( sfxid == sfx_None )? NULL : & S_sfx[sfxid];
   
    for (cnum = 0; cnum < cv_numChannels.value; cnum++)
    {
        if (origin && channels[cnum].origin == origin)
            return 1;

        if ( sfx && (channels[cnum].sfxinfo == sfx) )
            return 1;
    }
    return 0;
}
