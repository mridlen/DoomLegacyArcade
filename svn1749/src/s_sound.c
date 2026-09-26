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
#include "i_system.h"   // [Arcade] -sndlog: I_GetTime
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

// [Arcade] PC speaker emulation: play the DP* lumps -- the square-wave tones
// Doom shipped for machines with no sound card -- instead of the digital DS*
// sounds.  Off is the stock behaviour, which is what an unconfigured cabinet
// keeps.  Not gameplay: it draws no random numbers (see S_StartSoundAtVolume).
static void CV_pcspeaker_OnChange( void );
consvar_t cv_pcspeaker = { "pcspeaker", "Off", CV_SAVE | CV_CALL, CV_OnOff, CV_pcspeaker_OnChange };

#ifdef OPL_MUSIC
// [Arcade] OPL music: MUS and MIDI through prboom-plus's OPL2 player instead of
// SDL_mixer's MIDI synth.  Off keeps what the cabinet already plays.  MP3 and
// OGG music is not affected.  I_RegisterSong decides, per song.
static void CV_opl_music_OnChange( void );
consvar_t cv_opl_music = { "opl_music", "Off", CV_SAVE | CV_CALL, CV_OnOff, CV_opl_music_OnChange };
#endif

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
    // [Arcade] -sndlog: gametic at which the mobj this sound came from was
    // removed while the sound played on, 0 if it has not been.  Its origin
    // then points into freed memory -- see S_StopXYZSound.
    tic_t     orphan_tic;
    tic_t     start_tic;   // [Arcade] -sndlog: when it started
    // [Arcade] Where a sound that outlived its mobj plays on from; origin is
    // pointed here by S_StopXYZSound.
    xyz_t     orphan_pos;
} channel_t;


// [Arcade] -sndlog: a line for every sound cut off before it finished, with
// the reason.  Written for "the plasma rifle sound cuts off on E4M2 but not
// E1M1".  A sound can be stopped early in five places, in two layers --
// S_get_channel (same origin, or a lower priority stolen when every channel
// is busy), S_UpdateSounds (out of earshot), S_StopXYZSound (its source
// removed) and the mixer's own slot table in sdl/i_sound.c -- and nothing
// said which one did it.
//
// "-sndlog" alone logs every cut and every refused start.  "-sndlog plasma"
// logs only the lines naming that sfx (the lump name without DS), and also
// every start of it, so each plasma shot can be followed to how it ended.
// Writes sndlog.txt beside the program, flushed per line, for the same reason
// -volog does: the Windows build has no console.
byte  sndlog_on = 0;
static FILE * sndlog_fp = NULL;
static const char * sndlog_filter = NULL;

static void S_Sndlog_Init( void )
{
    int p = M_CheckParm( "-sndlog" );
    if( ! p )
        return;
    sndlog_on = 1;
    if( p + 1 < myargc && myargv[p+1][0] != '-' && myargv[p+1][0] != '+' )
        sndlog_filter = myargv[p+1];
    sndlog_fp = fopen( "sndlog.txt", "w" );
    S_Sndlog( "SNDLOG start, filter=%s, snd_channels=%d\n",
              sndlog_filter ? sndlog_filter : "(all)", cv_numChannels.value );
}

// True when a line about these sfx should be written.  Either may be NULL.
boolean S_Sndlog_Match( const sfxinfo_t * a, const sfxinfo_t * b )
{
    if( ! sndlog_on )
        return false;
    if( ! sndlog_filter )
        return true;
    return ( a && a->name && strcasecmp( a->name, sndlog_filter ) == 0 )
        || ( b && b->name && strcasecmp( b->name, sndlog_filter ) == 0 );
}

void S_Sndlog( const char * fmt, ... )
{
    char buf[512];
    va_list ap;
    int n;

    if( ! sndlog_on )
        return;
    // Game tic, then the wall clock in tics: they part company when the
    // game stalls or catches up, and a sound's length is wall-clock time.
    n = snprintf( buf, sizeof(buf), "T%-6u W%-6u ", (unsigned int) gametic,
                  (unsigned int) I_GetTime() );
    va_start( ap, fmt );
    vsnprintf( buf + n, sizeof(buf) - n, fmt, ap );
    va_end( ap );
    GenPrintf( EMSG_warn, "%s", buf );
    if( sndlog_fp )
    {
        fputs( buf, sndlog_fp );
        fflush( sndlog_fp );
    }
}

static const char * S_Sndlog_Name( const sfxinfo_t * sfx )
{
    return ( sfx && sfx->name ) ? sfx->name : "?";
}

// Channels in use, for the "busy" figure on a line.
static int S_Sndlog_Busy( void );

// The set of channels available.
// Number of channels is set by cv_numChannels
static channel_t *channels;

// whether songs are mus_paused
static boolean mus_paused;

// music currently being played
static musicinfo_t *mus_playing = NULL;

#ifdef OPL_MUSIC
// [Arcade] Restart the song through whichever player now applies.  On config
// load nothing is playing yet, so this does nothing.  Loops, as the music
// source switch above does.
static void CV_opl_music_OnChange( void )
{
    if( mus_playing )
    {
        int music_num = mus_playing - S_music;
        S_StopMusic();
        S_ChangeMusic( music_num, 1 );
    }
}
#endif


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
        channels[i].orphan_tic = 0;   // [Arcade] -sndlog
    }

}

void S_InitRuntimeMusic()
{
    int i;

    for (i = mus_firstfreeslot; i < mus_lastfreeslot; i++)
        S_music[i].name = NULL;
}


// [Arcade] PC speaker emulation.
//
// A DP* lump is not sampled sound: it is a list of tones, one per 1/140 s,
// that DMX fed to the speaker's timer.  Format: uint16 0 (format), uint16
// count, then count tone bytes, 0 meaning silence.  They are rendered here to
// ordinary 8-bit DMX sound data, so the mixer and every sound backend play
// them like any other lump.  Behaviour follows prboom-plus's emulation (which
// dsda-doom inherited, then removed in v0.27): its tone table, the six sounds
// the speaker never played, and one voice at a time, the newest sound cutting
// off the one playing (S_StartSoundAtVolume).

// Tone number -> Hz, from prboom-plus i_pcsound.c (after pcspkr10.zip), with
// one tone added: the stock DOOM2.WAD lumps go up to tone 96, which that table
// stopped short of and so played as silence.  The table rises a quarter tone
// per entry, 2^(1/24), and tone 96 is the next step up.  Tones past the end
// are silence.
static const float pcs_frequencies[] = {
    0.0f, 175.00f, 180.02f, 185.01f, 190.02f, 196.02f, 202.02f, 208.01f, 214.02f, 220.02f,
    226.02f, 233.04f, 240.02f, 247.03f, 254.03f, 262.00f, 269.03f, 277.03f, 285.04f,
    294.03f, 302.07f, 311.04f, 320.05f, 330.06f, 339.06f, 349.08f, 359.06f, 370.09f,
    381.08f, 392.10f, 403.10f, 415.01f, 427.05f, 440.12f, 453.16f, 466.08f, 480.15f,
    494.07f, 508.16f, 523.09f, 539.16f, 554.19f, 571.17f, 587.19f, 604.14f, 622.09f,
    640.11f, 659.21f, 679.10f, 698.17f, 719.21f, 740.18f, 762.41f, 784.47f, 807.29f,
    831.48f, 855.32f, 880.57f, 906.67f, 932.17f, 960.69f, 988.55f, 1017.20f, 1046.64f,
    1077.85f, 1109.93f, 1141.79f, 1175.54f, 1210.12f, 1244.19f, 1281.61f, 1318.43f,
    1357.42f, 1397.16f, 1439.30f, 1480.37f, 1523.85f, 1569.97f, 1614.58f, 1661.81f,
    1711.87f, 1762.45f, 1813.34f, 1864.34f, 1921.38f, 1975.46f, 2036.14f, 2093.29f,
    2157.64f, 2217.80f, 2285.78f, 2353.41f, 2420.24f, 2490.98f, 2565.97f, 2639.77f,
    2716.00f,
};
#define PCS_NUM_TONES   (sizeof(pcs_frequencies) / sizeof(pcs_frequencies[0]))
#define PCS_TONE_RATE   140     // tones per second
#define PCS_SAMPLERATE  22050   // rendered rate; the header carries it
#define PCS_OVERSAMPLE  8       // box-filtered, to tame the square's aliasing
// Of 127.  A square wave's RMS is its amplitude, and 32 is the median RMS of
// the DS* sounds in DOOM2.WAD (28.8), so a speaker sound is about as loud as
// the sound it replaces.  Its peaks are lower: sampled sound is spikier.
#define PCS_AMPLITUDE   32

// Whether the loaded wads have speaker lumps at all.  Heretic has none, so
// there the option leaves the digital sounds alone rather than muting them.
static byte  pcs_lumps_present = 2;  // 2 = not yet looked

// True when sounds are being played as the PC speaker.
static boolean S_PCSpeaker_Active( void )
{
    if( ! cv_pcspeaker.EV || EN_heretic )
        return false;
    if( pcs_lumps_present == 2 )
        pcs_lumps_present = VALID_LUMP( W_CheckNumForName("dppistol") );
    return pcs_lumps_present;
}

// Render sfx's DP lump into sfx->data.  A sound with no speaker lump, or a
// malformed one, is left without data and so is silent, as it was on the
// speaker -- it must not fall back to the sampled sound, or to dspistol.
static void S_PCSpeaker_Lump( sfxinfo_t * sfx )
{
    char  lmpname_buf[20];
    lumpnum_t  lumpnum;
    byte * lump;
    int  lumplen, count, nsamples, i;
    uint32_t  phase = 0;
    byte * out;

    snprintf( lmpname_buf, sizeof(lmpname_buf), "dp%s", sfx->name );
    lumpnum = W_CheckNumForName( lmpname_buf );
    if( ! VALID_LUMP( lumpnum ) )
        return;

    lumplen = W_LumpLength( lumpnum );
    if( lumplen < 4 )
        return;
    // Held as PU_SOUND while reading: the Z_Malloc below may purge PU_CACHE.
    lump = W_CacheLumpNum( lumpnum, PU_SOUND );
    count = lump[2] | (lump[3] << 8);
    if( lump[0] != 0 || lump[1] != 0 || count <= 0 || count > lumplen - 4 )
    {
        Z_ChangeTag( lump, PU_CACHE );
        return;
    }

    nsamples = (count * PCS_SAMPLERATE + PCS_TONE_RATE - 1) / PCS_TONE_RATE;
    out = Z_Malloc( nsamples + 8, PU_SOUND, 0 );
    // DMX header: format 3, sample rate, sample count; see S_GetSfxLump.
    out[0] = 3;  out[1] = 0;
    out[2] = PCS_SAMPLERATE & 0xFF;  out[3] = PCS_SAMPLERATE >> 8;
    out[4] = nsamples & 0xFF;  out[5] = (nsamples >> 8) & 0xFF;
    out[6] = (nsamples >> 16) & 0xFF;  out[7] = 0;

    for( i = 0; i < nsamples; i++ )
    {
        int tone = lump[ 4 + (i * PCS_TONE_RATE / PCS_SAMPLERATE) ];
        float freq = ( tone < (int)PCS_NUM_TONES ) ? pcs_frequencies[tone] : 0.0f;
        uint32_t step;
        int k, sum = 0;

        if( freq <= 0.0f )
        {
            out[8 + i] = 128;   // silence; the phase carries on into the next tone
            continue;
        }
        // Phase is a 32-bit fraction of a cycle: high for the first half.
        step = (uint32_t)( freq * (4294967296.0 / ((double)PCS_SAMPLERATE * PCS_OVERSAMPLE)) );
        for( k = 0; k < PCS_OVERSAMPLE; k++ )
        {
            sum += ( phase < 0x80000000u ) ? 1 : -1;
            phase += step;
        }
        out[8 + i] = 128 + (PCS_AMPLITUDE * sum) / PCS_OVERSAMPLE;
    }
    Z_ChangeTag( lump, PU_CACHE );

    sfx->lumpnum = lumpnum;
    sfx->data = out;
    sfx->length = nsamples + 8;   // I_GetSfx takes the header back off
}

// The two kinds of sound data are cached side by side, and a switch swaps
// them.  Nothing is freed: the SDL mixer mixes from its own copy of a channel
// outside mix_lock, so data freed on a menu change could still be read for a
// buffer.  PU_SOUND is never purged, so the other set stays valid.
static void *     pcs_other_data[NUMSFX_EXT];
static int32_t    pcs_other_length[NUMSFX_EXT];
static lumpnum_t  pcs_other_lumpnum[NUMSFX_EXT];

static void CV_pcspeaker_OnChange( void )
{
    int i;

    for( i = 1; i < NUMSFX_EXT; i++ )
    {
        sfxinfo_t * sfx = & S_sfx[i];
        void *    d;
        int32_t   n;
        lumpnum_t l;

        if( sfx->link_id )
        {
            // Only a reference to its link's data, refreshed at every start.
            sfx->data = NULL;
            sfx->length = 0;
            continue;
        }
        d = sfx->data;  n = sfx->length;  l = sfx->lumpnum;
        sfx->data = pcs_other_data[i];
        sfx->length = pcs_other_length[i];
        sfx->lumpnum = pcs_other_data[i] ? pcs_other_lumpnum[i] : NO_LUMP;
        pcs_other_data[i] = d;
        pcs_other_length[i] = n;
        pcs_other_lumpnum[i] = l;
    }
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

    // [Arcade] PC speaker emulation: the DP lump, rendered, or silence.
    if( S_PCSpeaker_Active() )
    {
        S_PCSpeaker_Lump( sfx );
        return;
    }

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
    S_Sndlog_Init();   // [Arcade] -sndlog

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
    const char * cut_why = "same origin";   // [Arcade] -sndlog

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
        {
            cut_why = "all channels busy, lowest priority stolen";  // [Arcade]
            goto reuse_cnum;
        }
    }
    // No lower priority.  Sorry, Charlie.
    // [Arcade] -sndlog
    if( S_Sndlog_Match( sfxinfo, NULL ) )
    {
        // Name what is holding the channels: sfx/age in tics, * = orphaned.
        char held[256];
        int  hn = 0;
        held[0] = 0;
        for( cnum = 0; cnum < cv_numChannels.value && hn < (int)sizeof(held) - 16; cnum++ )
        {
            c = &channels[cnum];
            hn += snprintf( held + hn, sizeof(held) - hn, " %s/%u%s",
                            S_Sndlog_Name(c->sfxinfo),
                            (unsigned int)(gametic - c->start_tic),
                            c->orphan_tic ? "*" : "" );
        }
        S_Sndlog( "REFUSED %s pri=%d: all %d channels busy, lowest pri=%d%s; held by%s\n",
                  S_Sndlog_Name(sfxinfo), priority, cv_numChannels.value,
                  low_priority, (chanlimit <= 0) ? " (over its limit)" : "", held );
    }
    return -1;

 reuse_cnum:
    // [Arcade] -sndlog: only a sound still playing is being cut off.
    c = &channels[cnum];
    if( c->sfxinfo && I_SoundIsPlaying(c->handle)
        && S_Sndlog_Match( c->sfxinfo, sfxinfo ) )
        S_Sndlog( "CUT %s ch=%d pri=%d by %s pri=%d: %s, busy=%d/%d\n",
                  S_Sndlog_Name(c->sfxinfo), cnum, c->priority,
                  S_Sndlog_Name(sfxinfo), priority, cut_why,
                  S_Sndlog_Busy(), cv_numChannels.value );
    S_StopChannel(cnum);
 use_cnum:   
    c = &channels[cnum];

    // channel is decided to be cnum.
    c->sfxinfo = sfxinfo;
    c->priority = priority;
    c->origin = origin;
    c->orphan_tic = 0;   // [Arcade] -sndlog
    c->start_tic = gametic;

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
// [Arcade] -volog counters: how many sounds were asked for, and how many were
// thrown away as inaudible.  A cabinet whose listener is wrong drops every
// positional sound here while the mixer sits at full volume, which is silence
// that no volume setting can explain -- see S_Update_Volumes.
unsigned int  volog_snd_req = 0, volog_snd_inaudible = 0;
// [Arcade] -volog: the other two ways a request dies before it reaches the
// mixer.  "asked minus inaudible" is not "accepted", which is what the
// previous run's numbers were read as: S_get_channel can refuse, and a
// zero-length lump is dropped later still.
unsigned int  volog_snd_nochan = 0, volog_snd_nodata = 0;

void S_StartSoundAtVolume(const xyz_t * origin, const mobj_t * mo,
                          sfxid_t sfx_id, int volume,
                          channel_type_t ct_type )
{
    sound_param_t sp1;
    int priority;  // Heretic style signed priority, nominally -10 .. 2560.
    sfxinfo_t * sfx;
    int cnum;

    volog_snd_req++;   // [Arcade] -volog: every request, however it ends

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
        {
            volog_snd_inaudible++;   // [Arcade] -volog
            goto done;
        }

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

    // [Arcade] PC speaker: one voice, no distance, no stereo, no pitch.  Only
    // here, after every M_Random draw above, so that switching it cannot
    // change which random numbers a demo sees.
    if( S_PCSpeaker_Active() )
    {
        // The speaker never played these; see prboom-plus I_PCS_StartSound.
        if( sfx_id == sfx_posact || sfx_id == sfx_bgact || sfx_id == sfx_dmact
            || sfx_id == sfx_dmpain || sfx_id == sfx_popain || sfx_id == sfx_sawidl )
            goto done;

        if( !sfx->data )
            S_GetSfx( sfx );
        if( sfx->length <= 0 )
        {
            volog_snd_nodata++;   // no DP lump: silent, and cuts nothing off
            goto done;
        }

        sp1.volume = (volume > 255) ? 255 : volume;
        sp1.sep = 0;
        sp1.pitch = NORM_PITCH;
        // Newest wins: whatever was playing stops, as on the real speaker.
        for (cnum = 0; cnum < cv_numChannels.value; cnum++)
            S_StopChannel(cnum);
    }

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
    {
        volog_snd_nochan++;   // [Arcade] -volog
        goto done;
    }

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
    {
       volog_snd_nodata++;   // [Arcade] -volog
       goto done;
    }

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

    // [Arcade] -sndlog: starts are only logged for the filtered sfx, where
    // they are what lets each one be followed to how it ended.
    if( sndlog_filter && S_Sndlog_Match( sfx, NULL ) )
        S_Sndlog( "START %s ch=%d pri=%d vol=%d dist=%d from=%s busy=%d/%d\n",
                  S_Sndlog_Name(sfx), cnum, priority, sp1.volume, sp1.dist,
                  ( ! origin ) ? "none" : ( mo && mo == displayplayer_ptr->mo ) ? "player"
                  : mo ? "mobj" : "sector",
                  S_Sndlog_Busy(), cv_numChannels.value );
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
                // [Arcade] -sndlog
                if( I_SoundIsPlaying(channels[cnum].handle)
                    && S_Sndlog_Match( channels[cnum].sfxinfo, NULL ) )
                    S_Sndlog( "CUT %s ch=%d: its source was stopped or removed\n",
                              S_Sndlog_Name(channels[cnum].sfxinfo), cnum );
                S_StopChannel(cnum);
            }
            else
            {
                // [Arcade] The sound plays on, but when this is a mobj being
                // removed (P_RemoveMobj) its origin is about to be freed, and
                // S_UpdateSounds would go on reading a position out of freed
                // zone memory every tic -- whatever was allocated there next.
                // A missile's firing sound (plasma, rocket, imp fireball) is
                // played from the missile itself, so this is every missile
                // that hits something within its sound's length.  Keep the
                // last position in the channel and play on from there.
                channel_t * c = &channels[cnum];
                c->orphan_pos = *origin;
                c->origin = &c->orphan_pos;
                c->orphan_tic = gametic ? gametic : 1;   // -sndlog
                if( S_Sndlog_Match( c->sfxinfo, NULL ) )
                    S_Sndlog( "ORPHAN %s ch=%d: source removed, plays on from where it was\n",
                              S_Sndlog_Name(c->sfxinfo), cnum );
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


// Reconcile the mixer volumes with the volume cvars.
//
// Update sound/music volumes, if changed manually at console.
//
// [Arcade] ... and scale them down while the attract cycle is what is on
// screen.  This is the only place the mixer volume is reconciled with the
// cvars, so the attract scaling has to happen *here*: setting the mixer
// directly from somewhere else would be undone by this comparison on the next
// pass.  Coming out of attract into a game is likewise automatic -- the target
// changes and this restores full volume.
//
// [Arcade] Split out of S_UpdateSounds and called from D_DoomLoop on every
// pass, plus once at startup as soon as S_Init has brought the mixer up.
// S_UpdateSounds is called only when a tic advanced, and at boot the first tic
// does not run until the local client/server handshake finishes -- measured 51
// tics, 1.4 seconds, during which the title music had already started and was
// playing at full volume.  That is the cabinet "coming in hot and then calming
// down".  The mixer must already be at attract volume before anything can play
// through it, which means before D_DoomLoop, not one tic into it.
//
// Not skipped in devmode, unlike most cabinet behaviour: an operator changing
// this setting needs to hear what it does, and there is no lockdown reason to
// suppress it.
void S_Update_Volumes(void)
{
    int  want_sfx, want_mus;

    if( dedicated )
        return;   // S_Init did not bring a mixer up

    want_sfx = S_Attract_Scaled( cv_soundvolume.value );
    want_mus = S_Attract_Scaled( cv_musicvolume.value );

    // [Arcade] Music Cabinet: while cabinets are playing a linked game, only
    // the chosen one carries the music (m_menu.c, cv_link_musiccab).  Sound
    // effects are untouched -- they belong where they happen; it is music that
    // cannot be played on several machines at once without drifting.  A
    // cabinet playing on its own is never muted.
    //
    // **The mute pauses the music.  It must never set the music volume to 0.**
    //
    // It used to do exactly that, and on the Windows cabinet it took the sound
    // effects with it: the machine went completely silent in every linked game
    // it joined, and came back the moment the game ended.  Every engine number
    // said the sound was fine -- effects volume at full, 52 sounds started,
    // the mixer reading them every buffer, the post-mix callback never missing
    // one -- because the engine *was* fine.  The loss was underneath it.
    //
    // SDL_mixer plays MIDI on Windows through the system synth (winmm), and
    // Mix_VolumeMusic() reaches that backend as midiOutSetVolume(), which
    // attenuates the program's whole audio output rather than the MIDI stream
    // alone.  Zero there is zero for everything the process plays.  Linux
    // mixes MIDI into the same buffer as the sound effects and never sees it,
    // so this reproduces on no machine here: musicvolume "0" on Linux leaves
    // sfxpeak at a healthy 12593.
    //
    // Pausing touches no volume control, on any platform.  If a backend ever
    // ignores the pause the failure is music playing on two cabinets at once,
    // which is the complaint this feature started from -- not a silent cabinet.
    {
        extern boolean M_Link_Music_Muted( void );
        static boolean  musiccab_paused = false;

        if( M_Link_Music_Muted() )
        {
            // Re-asserted every tic rather than set once on the edge: a level
            // change runs S_ChangeMusic, and I_PlaySong knows nothing about
            // this mute, so a one-shot pause would be undone at every map.
            I_PauseSong(0);
            musiccab_paused = true;
        }
        else if( musiccab_paused )
        {
            musiccab_paused = false;
            // Not while the game itself has the music paused (menu, pause key)
            // -- S_ResumeSound will do it at the right moment.
            if( ! mus_paused )
                I_ResumeSong(0);
        }
    }

    // [Arcade] -volog: say what the mixer was set to and what decided it.
    //
    // Added for a cabinet that went completely silent -- sound as well as
    // music -- while in a linked game it had joined, and came back afterwards.
    // Three plausible explanations were each ruled out by reading the code
    // (this function only ever assigns want_mus for the Music Cabinet; the
    // attract scaling makes things quiet, not silent; the join path does
    // disable the attract cycle), so the next step is to stop reasoning and
    // read the numbers off the machine that does it.
    //
    // Prints only when something changes, so a whole session is a handful of
    // lines.  Every input is on the line: if one of the volumes is 0, this
    // says which cvar or which scaling made it 0 -- and if they are both
    // healthy while the cabinet is silent, the mixer or the audio device is
    // the place to look instead, not this code.
    //
    // **It writes its own file, and that is not a nicety.**  On Windows the
    // program is a GUI binary with no console attached, so nothing printed
    // reaches a command line -- and LOGMESSAGES, which would give the engine a
    // log.txt, is commented out of a normal build (doomdef.h).  A diagnostic
    // that only calls GenPrintf is therefore invisible on the one machine that
    // shows the fault.  Flushed per line, so a cabinet switched off at the
    // wall still leaves what it had.
    if( M_CheckParm("-volog") )
    {
        extern boolean M_Link_Music_Muted( void );
        static FILE * volog = NULL;
        static byte   volog_tried = 0;
        static int  last_sfx = -1, last_mus = -1, last_flags = -1;

        if( ! volog_tried )
        {
            volog_tried = 1;
            volog = fopen( "volog.txt", "w" );
        }
        int flags = (D_Attract_Running() ? 1 : 0) | (D_Menu_Over_Attract() ? 2 : 0)
                  | (M_Link_Music_Muted() ? 4 : 0) | (netgame ? 8 : 0)
                  | (dedicated ? 16 : 0);
        // The whole chain from "a sound was asked for" to "samples were mixed
        // into the buffer handed to the audio device", reported every two
        // seconds whether or not the volumes moved.  The silent cabinet has
        // now cleared every layer above this one -- full volume, a valid
        // listener, sounds accepted, and the mixer callback running without a
        // single missed buffer -- so what is left is the handful of steps
        // between S_StartSoundAtVolume deciding to play a sound and the mixer
        // reading samples out of a channel.  Each one gets a number:
        //
        //   asked    requests, however they end
        //   inaud    dropped as out of earshot
        //   nochan   S_get_channel refused
        //   nodata   the lump has no samples
        //   started  reached I_StartSound and was published to the mixer
        //   mixchan  channel-passes the mixer actually read samples from
        //   peakvol  loudest left/right volume started since the last
        //            report, 0..127
        //   sfxpeak  loudest sound-effect SAMPLE actually written into the
        //            buffer handed to SDL, 0..32767 -- every other number is
        //            intent, this one is output
        //
        // sfxpeak is the one that ends the search.  Healthy, and real audio
        // reached the buffer SDL sends to the device: the engine is done and
        // the fault is the device, its volume, or which device was opened --
        // and "VOLOG audio", printed once, says what that device is.  Zero
        // while started and mixchan climb, and the samples themselves are
        // silent, which is a different bug entirely.  started climbing with mixchan flat means the
        // mixer never sees the channels.  A gap between asked and started
        // names which of the three rejections is eating them.
        {
            extern unsigned int volog_snd_req, volog_snd_inaudible;
            extern unsigned int volog_snd_nochan, volog_snd_nodata;
            extern unsigned int volog_mix_calls, volog_snd_started, volog_mix_chan;
            extern int volog_vol_peak_l, volog_vol_peak_r;
            extern int volog_sfx_peak;
            extern char volog_audio_info[128];
            static byte  said_audio = 0;
            extern tic_t I_GetTime( void );
            static uint32_t  next_ms = 0;
            static unsigned int  last_req = 0, last_mix = 0, last_chan = 0;
            uint32_t now = (uint32_t) I_GetTime();
            if( now >= next_ms )
            {
                next_ms = now + (2*TICRATE);
                if( ! said_audio )
                {
                    said_audio = 1;
                    GenPrintf( EMSG_warn, "VOLOG audio %s\n", volog_audio_info );
                    if( volog )
                    {
                        fprintf( volog, "VOLOG audio %s\n", volog_audio_info );
                        fflush( volog );
                    }
                }
                if( volog_snd_req != last_req || volog_mix_calls != last_mix
                    || volog_mix_chan != last_chan )
                {
                    const char * sfmt =
                      "VOLOG sounds asked=%u inaud=%u nochan=%u nodata=%u started=%u "
                      "mixchan=%u peakvol=%d/%d sfxpeak=%d mixcalls=%u "
                      "nosoundfx=%d consoleplayer=%d displayplayer=%d listener_mo=%s\n";
                    int dp = displayplayer_ptr ? (int)(displayplayer_ptr - players) : -1;
                    const char * lm = (displayplayer_ptr && displayplayer_ptr->mo) ? "yes" : "NULL";
                    last_req = volog_snd_req;  last_mix = volog_mix_calls;
                    last_chan = volog_mix_chan;
                    GenPrintf( EMSG_warn, sfmt, volog_snd_req, volog_snd_inaudible,
                               volog_snd_nochan, volog_snd_nodata, volog_snd_started,
                               volog_mix_chan, volog_vol_peak_l, volog_vol_peak_r, volog_sfx_peak,
                               volog_mix_calls, nosoundfx ? 1 : 0, (int)consoleplayer, dp, lm );
                    if( volog )
                    {
                        fprintf( volog, sfmt, volog_snd_req, volog_snd_inaudible,
                                 volog_snd_nochan, volog_snd_nodata, volog_snd_started,
                                 volog_mix_chan, volog_vol_peak_l, volog_vol_peak_r, volog_sfx_peak,
                                 volog_mix_calls, nosoundfx ? 1 : 0, (int)consoleplayer, dp, lm );
                        fflush( volog );
                    }
                    // peaks are per report window, so they can go back down
                    volog_vol_peak_l = volog_vol_peak_r = -1;
                    volog_sfx_peak = 0;
                }
#ifdef OPL_MUSIC
                // [Arcade] OPL music: what the last song registration decided
                // (and why, when it fell back to MIDI), how often SDL_mixer
                // ran the hook, and the loudest sample the hook wrote.
                {
                    extern char volog_opl_info[160];
                    extern unsigned int volog_opl_hooks;
                    extern int volog_opl_peak;
                    static char  last_info[160] = "";
                    static unsigned int  last_hooks = 0;
                    if( strcmp( last_info, volog_opl_info ) != 0
                        || volog_opl_hooks != last_hooks )
                    {
                        const char * ofmt = "VOLOG opl cv=%d %s hooks=%u peak=%d\n";
                        strcpy( last_info, volog_opl_info );
                        last_hooks = volog_opl_hooks;
                        GenPrintf( EMSG_warn, ofmt, cv_opl_music.EV, volog_opl_info,
                                   volog_opl_hooks, volog_opl_peak );
                        if( volog )
                        {
                            fprintf( volog, ofmt, cv_opl_music.EV, volog_opl_info,
                                     volog_opl_hooks, volog_opl_peak );
                            fflush( volog );
                        }
                        volog_opl_peak = 0;
                    }
                }
#endif
            }
        }

        if( want_sfx != last_sfx || want_mus != last_mus || flags != last_flags )
        {
            last_sfx = want_sfx;  last_mus = want_mus;  last_flags = flags;
            const char * fmt =
              "VOLOG sfx=%d mus=%d  cv_sfx=%d cv_mus=%d cv_attract=%d "
              "attract=%d menuover=%d musiccab_muted=%d netgame=%d gamestate=%d\n";
            GenPrintf( EMSG_warn, fmt,
              want_sfx, want_mus,
              cv_soundvolume.value, cv_musicvolume.value, cv_attractvolume.value,
              (flags & 1) ? 1 : 0, (flags & 2) ? 1 : 0, (flags & 4) ? 1 : 0,
              (flags & 8) ? 1 : 0, (int)gamestate );
            if( volog )
            {
                fprintf( volog, fmt,
                  want_sfx, want_mus,
                  cv_soundvolume.value, cv_musicvolume.value, cv_attractvolume.value,
                  (flags & 1) ? 1 : 0, (flags & 2) ? 1 : 0, (flags & 4) ? 1 : 0,
                  (flags & 8) ? 1 : 0, (int)gamestate );
                fflush( volog );
            }
        }
    }

    if (mix_sfxvolume != want_sfx)
        S_SetSfxVolume(want_sfx);
    if (mix_musicvolume != want_mus)
        S_SetMusicVolume(want_mus);
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

    S_Update_Volumes();

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
                // [Arcade] -sndlog: finished, or cut by the mixer (which
                // logs that itself).  Only for the filtered sfx.
                if( sndlog_filter && S_Sndlog_Match( c->sfxinfo, NULL ) )
                    S_Sndlog( "END %s ch=%d\n", S_Sndlog_Name(c->sfxinfo), cnum );
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
                    // [Arcade] -sndlog
                    if( S_Sndlog_Match( c->sfxinfo, NULL ) )
                    {
                        if( c->orphan_tic )
                            S_Sndlog( "CUT %s ch=%d: out of earshot, dist=%d (source removed %u tics ago)\n",
                                      S_Sndlog_Name(c->sfxinfo), cnum, sp1.dist,
                                      (unsigned int)(gametic - c->orphan_tic) );
                        else
                            S_Sndlog( "CUT %s ch=%d: out of earshot, dist=%d\n",
                                      S_Sndlog_Name(c->sfxinfo), cnum, sp1.dist );
                    }
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
                // [Arcade] The speaker has no volume or stereo to follow a
                // moving source with; its driver ignored this call too.
                if( ! S_PCSpeaker_Active() )
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


// [Arcade] The newest MUS or MIDI lump with this music's standard name, in any
// loaded wad, or NO_LUMP.  For when a later wad has replaced it with OGG or MP3
// under the same name; see its use in S_ChangeMusic.
static lumpnum_t  S_Find_Midi_Music( const char * name, /*OUT*/ byte * type )
{
    char  lumpname[16];
    int   w;

    snprintf( lumpname, sizeof(lumpname), (EN_heretic ? "%.8s" : "d_%.6s"), name );
    for( w = numwadfiles - 1; w >= 0; w-- )
    {
        byte  head[4];
        lumpnum_t  ln = W_CheckNumForNamePwad( lumpname, w, 0 );
        if( ! VALID_LUMP(ln) || W_LumpLength(ln) < 4 )
            continue;
        W_ReadLumpHeader( ln, head, 4 );
        if( memcmp( head, "MUS", 3 ) == 0 )
        {
            *type = MUSTYPE_MUS;
            return ln;
        }
        if( memcmp( head, "MThd", 4 ) == 0 )
        {
            *type = MUSTYPE_MIDI;
            return ln;
        }
    }
    return NO_LUMP;
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
    // [Arcade] A soundtrack wad (IDKFAv2.wad, Doom2OST.wad) replaces the
    // IWAD's D_ lumps by name with OGG data, so asking for MUS -- Music src
    // "MUS", or OPL music, which plays nothing else -- still found the OGG.
    // Then look under it for the newest MUS or MIDI of the same name, which is
    // the IWAD's own song.  Chosen per play and not cached in music->lumpnum,
    // so switching either setting back picks the OGG up again.
    lumpnum_t  play_ln = music->lumpnum;
    if( VALID_LUMP(play_ln)
        && music_type != MUSTYPE_MUS && music_type != MUSTYPE_MIDI
        && ( 0
#ifdef MUSIC_SOURCE_CONTROL
             || cv_music_source.EV == 0
#endif
#ifdef OPL_MUSIC
             || cv_opl_music.EV
#endif
           ) )
    {
        byte  alt_type;
        lumpnum_t  alt_ln = S_Find_Midi_Music( music->name, &alt_type );
        if( VALID_LUMP(alt_ln) )
        {
            play_ln = alt_ln;
            music_type = alt_type;
        }
    }

    // load & register it
    music->data = (void *) S_CacheMusicLump(play_ln);
    music->handle = I_RegisterSong( music_type, music->data, W_LumpLength(play_ln));
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

// [Arcade] -sndlog
static int S_Sndlog_Busy( void )
{
    int cnum, n = 0;
    for( cnum = 0; cnum < cv_numChannels.value; cnum++ )
        if( channels[cnum].sfxinfo )  n++;
    return n;
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
