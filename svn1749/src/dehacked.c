// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: dehacked.c 1747 2025-04-25 07:59:18Z wesleyjohnson $
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
// $Log: dehacked.c,v $
// Revision 1.17  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.16  2003/11/21 17:52:05  darkwolf95
// added "Monsters Infight" for Dehacked patches
//
// Revision 1.15  2002/01/12 12:41:05  hurdler
// Revision 1.14  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.13  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.12  2001/06/30 15:06:01  bpereira
// fixed wronf next level name in intermission
//
// Revision 1.11  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.10  2001/02/10 12:27:13  bpereira
//
// Revision 1.9  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.8  2000/11/04 16:23:42  bpereira
//
// Revision 1.7  2000/11/03 13:15:13  hurdler
// Some debug comments, please verify this and change what is needed!
//
// Revision 1.6  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/08/31 14:30:55  bpereira
// Revision 1.4  2000/04/16 18:38:07  bpereira
//
// Revision 1.3  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Load dehacked file and change table and text from the exe
//
//-----------------------------------------------------------------------------


#include "doomincl.h"

#include "dehacked.h"
#include "command.h"
#include "console.h"

#include "g_game.h"
#include "p_local.h"
  // monster_infight

#include "info.h"
#include "infoext.h"
#include "sounds.h"
#include "action.h"

#include "m_cheat.h"
#include "d_think.h"
#include "dstrings.h"
#include "m_argv.h"
#include "p_inter.h"

//SoM: 4/4/2000: DEHACKED LUMPS
#include "z_zone.h"
#include "w_wad.h"

#include "p_fab.h"
  // translucent change
#include "m_misc.h"
  // dl_strcasestr

boolean deh_loaded = false;
byte  thing_flags_valid_deh = false;  // flags altered flags (from DEH), boolean
byte  pars_valid_bex = false;  // have valid PAR values (from BEX), boolean

uint16_t helper_MT = 0xFFFF;  // Substitute helper thing (like DOG).

static boolean  bex_include_notext = 0;  // bex include with skip deh text

// Save compare values, to handle multiple DEH files and lumps
static action_fi_t  deh_actions[NUMSTATES_DEF];
static char*        deh_sprnames[NUMSPRITES_DEF];
static char*        deh_sfxnames[NUMSFX_DEF];
static char*        deh_musicname[NUMMUSIC_DEF];
static char*        deh_text[NUMTEXT];

#define NAME4_AS_NUM( n )  (*((uint32_t*)(n)))


// Avactor.wad has long lines.
// PrBoom is using buffer of 1024.
#define MAXLINELEN  1024

// The code was first written for a file
// and converted to use memory with these functions.
typedef struct {
    char * data;
    char * curpos;
    int size;
} myfile_t;

#define myfeof( a )  (a->data+a->size<=a->curpos)

unsigned int  linenum = 0;

// Get string upto \n, or eof.
static
char* myfgets( char * buf, int bufsize, myfile_t * f )
{
    int i=0;

    if( myfeof(f) )
        return NULL;

    linenum ++;
    bufsize--;  // we need a extra byte for null terminated string
    while(i<bufsize && !myfeof(f) )
    {
        char c = *f->curpos++;
        if( c!='\r' )
            buf[i++]=c;
        if( c=='\n' )
            break;
    }
    buf[i] = '\0';
#ifdef DEBUG_DEH
    memset( &buf[i], 0, bufsize-i-1 );
#endif
    //debug_Printf( "fgets [0]=%d [1]=%d '%s'\n",buf[0],buf[1],buf);

    if( (devparm > 2) || (verbose>1) )
    {
        // List DEH lines, but not blank lines.
        if( i > 1 )
          GenPrintf(EMSG_errlog, "DEH: %s", buf);  // buf has \n
    }
    return buf;
}

// Get line, skipping comments.
static
char* myfgets_nocom(char * buf, int bufsize, myfile_t * f)
{
    char* ret;

    do {
        ret = myfgets( buf, bufsize, f );
    } while( ret && ret[0] == '#' );   // skip comment
    return ret;
}


// Read multiple lines into buf
// Only used for text.
static
size_t  myfread( char * buf, size_t reqsize, myfile_t * f )
{
    size_t byteread = f->size - (f->curpos-f->data);  // bytes left
    if( reqsize < byteread )
        byteread = reqsize;
#ifdef DEBUG_DEH
    memset( &buf[0], 0, reqsize-1 );
#endif
    if( byteread>0 )
    {
        uint32_t i;
        // Read lines except for any '\r'.
        // But should only be taking the '\r' off end of line (/cr/lf)
        for(i=0; i<byteread; )
        {
            char c=*f->curpos++;
            if( c!='\r' )
                buf[i++]=c;
        }
    }
    buf[byteread] = '\0';
    return byteread;
}

static int deh_num_error = 0;

static void deh_error(const char * fmt, ...)
{
    va_list   ap;

    // Show the DEH errors for devparm, devgame, or verbose switches.   
    if (devparm || verbose)
    {
       va_start(ap, fmt);
       GenPrintf_va( EMSG_error, fmt, ap );
       va_end(ap);
    }

    deh_num_error++;
}

// A value that is not valid.
#define  INVALID_VALUE   0xFF100FFL
// Searches for '='.  This is not an error
// unless the caller actually needs the value.
// Many functions must call this before they actually
// have determined they need the value, or if it is just EOL.
// Returns the value found.
static
int searchvalue(char * s)
{
  while(s[0]!='=' && s[0]!='\0') s++;
  if (s[0]=='=')
    return atoi(&s[1]);

  return INVALID_VALUE;
}

// confirm, or report error
static
byte confirm_searchvalue( int val )
{
    if( val == INVALID_VALUE )
    {
        deh_error("No value found\n");
        return 0;
    }
    return 1;  // good value
}


// Reject src string if greater than maxlen, or has non-alphanumeric
// For filename protection must reject
//  ( . .. ../ / \  & * [] {} space leading-dash  <32  >127 133 254 255 )
boolean  filename_reject( char * src, int maxlen )
{
     int j;
     for( j=0; ; j++ )
     {
         if( j >= maxlen ) goto reject;

         register char ch = src[j];
         // legal values, all else is illegal
         if(! (( ch >= 'A' && ch <= 'Z' )
            || ( ch >= 'a' && ch <= 'z' )
            || ( ch >= '0' && ch <= '9' )
            || ( ch == '_' ) ))
            goto reject;
     }
     return false; // no reject

  reject:
     return true; // rejected
}


// [WDJ] Do not write into const strings, segfaults will occur on Linux.
// Without fixing all sources of strings to be consistent, the best that
// can be done is to abandon the original string (may be some lost memory)
// Dehacked is only run once at startup and the lost memory is not enough
// to dedicate code to recover.

typedef enum { DRS_nocheck, DRS_name, DRS_string, DRS_format } DRS_type_e;

typedef struct {
    uint16_t   text_id;
    byte  num_s;
} PACKED_ATTR  format_ref_t;

format_ref_t  format_ref_table[] =
{
   {QSPROMPT_NUM, 1},
   {QLPROMPT_NUM, 1},
   {DOSY_NUM, 1},
   {DEATHMSG_SUICIDE, 1},
   {DEATHMSG_TELEFRAG, 2},
   {DEATHMSG_FIST, 2},
   {DEATHMSG_GUN, 2},
   {DEATHMSG_SHOTGUN, 2},
   {DEATHMSG_MACHGUN, 2},
   {DEATHMSG_ROCKET, 2},
   {DEATHMSG_GIBROCKET, 2},
   {DEATHMSG_PLASMA, 2},
   {DEATHMSG_BFGBALL, 2},
   {DEATHMSG_CHAINSAW, 2},
   {DEATHMSG_SUPSHOTGUN, 2},
   {DEATHMSG_PLAYUNKNOW, 2},
   {DEATHMSG_HELLSLIME, 1},
   {DEATHMSG_NUKE, 1},
   {DEATHMSG_SUPHELLSLIME, 1},
   {DEATHMSG_SPECUNKNOW, 1},
   {DEATHMSG_BARRELFRAG, 2},
   {DEATHMSG_BARREL, 1},
   {DEATHMSG_POSSESSED, 1},
   {DEATHMSG_SHOTGUY, 1},
   {DEATHMSG_VILE, 1},
   {DEATHMSG_FATSO, 1},
   {DEATHMSG_CHAINGUY, 1},
   {DEATHMSG_TROOP, 1},
   {DEATHMSG_SERGEANT, 1},
   {DEATHMSG_SHADOWS, 1},
   {DEATHMSG_HEAD, 1},
   {DEATHMSG_BRUISER, 1},
   {DEATHMSG_UNDEAD, 1},
   {DEATHMSG_KNIGHT, 1},
   {DEATHMSG_SKULL, 1},
   {DEATHMSG_SPIDER, 1},
   {DEATHMSG_BABY, 1},
   {DEATHMSG_CYBORG, 1},
   {DEATHMSG_PAIN, 1},
   {DEATHMSG_WOLFSS, 1},
   {DEATHMSG_DEAD, 1},
   {0xFFFF, 0}
};

// [WDJ] 8/26/2011  DEH/BEX replace string
// newstring is a temp buffer ptr, it gets modified for backslash literals
// oldstring is ptr to text ptr  ( &text[i] )
void deh_replace_string( char ** oldstring, char * newstring, DRS_type_e drstype )
{
#ifdef DEH_RECOVER_STRINGS
    // Record bounds for replacement strings.
    // If an oldstring is found within these bounds, it can be free().
    // This is depends on const and malloc heap being two separate areas.
    static char * deh_string_ptr_min = NULL;
    static char * deh_string_ptr_max = NULL;
#endif
    // Most text strings are format strings, and % are significant.
    // New string must not have have any %s %d etc. not present in old string.
    // New freedoom.bex has "1%", that is not present in original string
    // Strings have %s, %s %s (old strings also had %c and %d).
    // Music and sound strings may have '-' and '\0'.
    // [WDJ] Newer compiler could not tolerate these as unsigned char.
    char * newp = &newstring[0];
    if( drstype == DRS_string )
    {
        // Test new string against table
        // Fixes when Chex replacement strings have fewer %s in them,
        // so Chex newmaps.wad can replace that string again.
        int text_id = oldstring - (&text[0]);
        byte num_s = 0;
        int i;
        // look up in table
        for(i=0; ; i++)
        {
            if( format_ref_table[i].text_id > NUMTEXT )  break;
            if( format_ref_table[i].text_id == text_id )
            {
                num_s = format_ref_table[i].num_s;
                drstype = DRS_format;
                break;
            }
        }

        for(i=0; ;)
        {
            // new string must have same or fewer %
            newp = strchr( newp, '%' );
            if( newp == NULL ) break;
            if( drstype == DRS_format )
            {
                // must block %n, write to memory
                // Only %s are left in the text strings
                switch( newp[1] )
                {
                 case '%': // literal %
                   break;
                 case 's':
                   i++;
                   if( i > num_s )   goto bad_replacement;
                   break;
                 default:
                   goto bad_replacement;
                }
            }
            else
            {
                // only  %% allowed
                if( newp[1] != '%' )
                    newp[0] = ' '; // rubout the %
            }
            newp +=2;
        }
    }

    // rewrite backslash literals into newstring, because it only gets shorter
    char * chp = &newstring[0];
    for( newp = &newstring[0]; *newp ; newp++ )
    {
        // Backslash in DEH and BEX strings are not interpreted by printf
        // Must convert \n to LF.
        register char ch = *newp;
        if( ch == 0x5C ) // backslash
        {
            char * endvp = NULL;
            unsigned long v;
            ch = *(++newp);
            switch( ch )
            {
             case 'N': // some file are all caps
             case 'n': // newline
               ch = '\n';  // LF char
               goto incl_char;
             case '\0': // safety
               goto term_string;
             case '0': // NUL, should be unnecessary
               goto term_string;
             case 'x':  // hex
               // These do not get interpreted unless we do it here.
               // Need this for foreign language ??
               v = strtoul( &newp[1], &endvp, 16);  // get hex
               goto check_backslash_value;
             default:
               if( ch >= '1' && ch <= '9' )  // octal
               {
                   // These do not get interpreted unless we do it here.
                   // Need this for foreign language ??
                   v = strtoul( newp, &endvp, 8);  // get octal
                   goto check_backslash_value;
               }
            }
            continue; // ignore unrecognized backslash

         check_backslash_value:
            if( v > 255 ) goto bad_char;  // long check
            ch = v;
            newp = endvp - 1; // continue after number
            // check value against tests
        }
        // reject special character attacks
#if defined( FRENCH_INLINE ) || defined( BEX_LANGUAGE )
        // place checks for allowed foreign lang chars here
        // reported dangerous escape chars
        if( (unsigned char)ch == 133 )  goto bad_char;
        if( (unsigned char)ch >= 254 )  goto bad_char;
//          if( ch == 27 ) continue;  // ESCAPE
#else
        if( (unsigned char)ch > 127 )  goto bad_char;
#endif       
        if( ch < 32 )
        {
            if( ch == '\t' )  ch = ' ';  // change to space
            if( ch == '\r' )  continue;  // remove
            if( ch == '\n' )  goto incl_char;
            if( ch == '\0' )  goto term_string;   // end of string
            goto bad_char;
        }
     incl_char:
        // After a backslash, chp < newp
        *chp++ = ch; // rewrite
    }
 term_string:
    *chp = '\0'; // term rewrite

    if( drstype == DRS_name )
    {
        if( strlen(newstring) > 10 )  goto bad_replacement;
    }

    char * nb = strdup( newstring );  // by malloc
    if( nb == NULL )
        I_Error( "Dehacked/BEX string memory allocate failure" );
#ifdef DEH_RECOVER_STRINGS
    // check if was in replacement string bounds
    if( *oldstring && deh_string_ptr_min
        && *oldstring >= deh_string_ptr_min
        && *oldstring <= deh_string_ptr_max )
        free( *oldstring );
    // track replacement string bounds
    if( deh_string_ptr_min == NULL || nb < deh_string_ptr_min )
        deh_string_ptr_min = nb;
    if( nb > deh_string_ptr_max )
        deh_string_ptr_max = nb;
#else
    // Abandon old strings, might be const
    // Linux GCC programs will segfault if try to free a const string (correct behavior).
    // The lost memory is small and this occurs only once in the program.
#endif
    *oldstring = nb;  // replace the string in the tables
    return;

  bad_char:
    if( chp )
        *chp = '\0'; // hide the bad character
  bad_replacement:
    CONS_Printf( "Replacement string illegal : %s\n", newstring );
    return;
}


/* ======================================================================== */
// Load a dehacked file format 6 I (BP) don't know other format
/* ======================================================================== */
/* a sample to see
                   Thing 1 (Player)       {           // MT_PLAYER
int doomednum;     ID # = 3232              -1,             // doomednum
int spawnstate;    Initial frame = 32       S_PLAY,         // spawnstate
int spawnhealth;   Hit points = 3232        100,            // spawnhealth
int seestate;      First moving frame = 32  S_PLAY_RUN1,    // seestate
int seesound;      Alert sound = 32         sfx_None,       // seesound
int reactiontime;  Reaction time = 3232     0,              // reactiontime
int attacksound;   Attack sound = 32        sfx_None,       // attacksound
int painstate;     Injury frame = 32        S_PLAY_PAIN,    // painstate
int painchance;    Pain chance = 3232       255,            // painchance
int painsound;     Pain sound = 32          sfx_plpain,     // painsound
int meleestate;    Close attack frame = 32  S_NULL,         // meleestate
int missilestate;  Far attack frame = 32    S_PLAY_ATK1,    // missilestate
int deathstate;    Death frame = 32         S_PLAY_DIE1,    // deathstate
int xdeathstate;   Exploding frame = 32     S_PLAY_XDIE1,   // xdeathstate
int deathsound;    Death sound = 32         sfx_pldeth,     // deathsound
int speed;         Speed = 3232             0,              // speed
int radius;        Width = 211812352        16*FRACUNIT,    // radius
int height;        Height = 211812352       56*FRACUNIT,    // height
int mass;          Mass = 3232              100,            // mass
int damage;        Missile damage = 3232    0,              // damage
int activesound;   Action sound = 32        sfx_None,       // activesound
int flags;         Bits = 3232              MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
int raisestate;    Respawn frame = 32       S_NULL          // raisestate
                                         }, */
// [WDJ] mobjinfo fields added
/*
int flags2;        Bits2 = 3232             MF2_xxx,        // implemented, Heretic
    // Mostly does not match flags2 values from Eternity Engine,
    LOGRAV
    FLOATBOB
 */

// [WDJ] mobjinfo fields from Eternity Engine, not implemented
/*
int flags3;        Bits3 = 3233             ??? Heretic
int translucency;  Translucency = 3         ???
int bloodcolor;    Blood color = 3          ???
 */

// [WDJ] BDTC_game_detection_e bits
uint16_t deh_detect; // BDTC_xxx, detected deh standards
uint16_t deh_key_detect;  // BDTC_game_detection_e
uint16_t deh_thing_detect; // BDTC_game_detection_e


// [WDJ] BEX flags 9/10/2011
// These are only used in the dehacked tables, so defined them unconditionally.
typedef enum { BFexit,
    BF1, BF1_MBF,
    BF2_mix, BF2x, BF2_HTC, BF2_M21,
    /*BF3, BF3x,*/ BF3_M21, BF3_HEX,
    BFT,  // Boom TRANSLATION
    BFR_M21,  // MBF21 Frame bits
} bex_flags_ctrl_e;

typedef struct {
    char *    name;
    bex_flags_ctrl_e  ctrl;
    uint32_t  flagval;
} flag_name_t;

// [WDJ] From boomdeh.txt, and DoomLegacy2.0
flag_name_t  BEX_flag_name_table[] = 
{
  {"SPECIAL",    BF1, MF_SPECIAL }, // Call TouchSpecialThing when touched.
  {"SOLID",      BF1, MF_SOLID }, // Blocks
  {"SHOOTABLE",  BF1, MF_SHOOTABLE }, // Can be hit
  {"NOSECTOR",   BF1, MF_NOSECTOR },  // Don't link to sector (invisible but touchable)
  {"NOBLOCKMAP", BF1, MF_NOBLOCKMAP }, // Don't link to blockmap (inert but visible)
  {"AMBUSH",     BF1, MF_AMBUSH }, // Not to be activated by sound, deaf monster.
  {"JUSTHIT",    BF1, MF_JUSTHIT }, // Will try to attack right back.
  {"JUSTATTACKED", BF1, MF_JUSTATTACKED }, // Will take at least one step before attacking.
  {"SPAWNCEILING", BF1, MF_SPAWNCEILING }, // Spawned hanging from the ceiling
  {"NOGRAVITY",  BF1, MF_NOGRAVITY }, // Does not feel gravity
  {"DROPOFF",    BF1, MF_DROPOFF }, // Can jump/drop from high places
  {"PICKUP",     BF1, MF_PICKUP }, // Can/will pick up items. (players)
  // NOCLIP is std, so does not detect Legacy extensions.
  // two clip bits, set them both
  {"NOCLIP",     BF1, MF_NOCLIP }, // Does not clip against lines.
  {"NOCLIP",     BF2_mix, MF2_NOCLIPTHING }, // Does not clip against Actors.
  // two slide bits, set them both
  {"SLIDE",      BF1, MF_SLIDE }, // Player: keep info about sliding along walls.
  {"SLIDE",      BF2_mix, MF2_SLIDE }, // Slides against walls

  {"FLOAT",      BF1, MF_FLOAT }, // Active floater, can move freely in air (cacodemons etc.)
  {"TELEPORT",   BF1, MF_TELEPORT }, // Don't cross lines or look at heights on teleport.
  {"MISSILE",    BF1, MF_MISSILE }, // Missile. Don't hit same species, explode on block.
  {"DROPPED",    BF1, MF_DROPPED }, // Dropped by a monster
  {"SHADOW",     BF1, MF_SHADOW }, // Partial invisibility (spectre). Makes targeting harder.
  {"NOBLOOD",    BF1, MF_NOBLOOD }, // Does not bleed when shot (furniture)
  {"CORPSE",     BF1, MF_CORPSE }, // Acts like a corpse, falls down stairs etc.
  {"INFLOAT",    BF1, MF_INFLOAT }, // Don't auto float to target's height.
  {"COUNTKILL",  BF1, MF_COUNTKILL }, // On kill, count towards intermission kill total.
  {"COUNTITEM",  BF1, MF_COUNTITEM }, // On pickup, count towards intermission item total.
  {"SKULLFLY",   BF1, MF_SKULLFLY }, // Flying skulls, neither a cacodemon nor a missile.
  {"NOTDMATCH",  BF1, MF_NOTDMATCH }, // Not spawned in DM (keycards etc.)
  // 4 bits of player color translation (gray/red/brown)
  // PrBoom, MBF, EternityEngine have only 2 bits of color translation.
  {"TRANSLATION1", BFT, (1<<MFT_TRANSSHIFT) },  // Boom
  {"TRANSLATION2", BFT, (2<<MFT_TRANSSHIFT) },  // Boom
  {"TRANSLATION3", BFT, (4<<MFT_TRANSSHIFT) },
  {"TRANSLATION4", BFT, (8<<MFT_TRANSSHIFT) },
  {"TRANSLATION",  BFT, (1<<MFT_TRANSSHIFT) },  // Boom/prboom compatibility
  {"UNUSED1     ", BFT, (2<<MFT_TRANSSHIFT) },  // Boom/prboom compatibility
  // Boom/BEX
  {"TRANSLUCENT", BF1, MF_TRANSLUCENT },  // Boom translucent
  // MBF/Prboom Extensions
  {"TOUCHY",  BF1_MBF, MF_TOUCHY }, // (MBF) Reacts upon contact
  {"BOUNCES", BF1_MBF, MF_BOUNCES },  // (MBF) Bounces off walls, etc.
  {"FRIEND",  BF1_MBF, MF_FRIEND }, // (MBF) Friend to player (dog, etc.)

  {"MF2CLEAR",       BF2x, 0 }, // clear MF2 bits, no bits set
  // DoomLegacy 1.4x Extensions
  {"FLOORHUGGER",    BF2x, MF2_FLOORHUGGER }, // [WDJ] moved to MF2
  // Heretic
  {"LOWGRAVITY",     BF2_HTC, MF2_LOGRAV }, // Experiences only 1/8 gravity
  {"WINDTHRUST",     BF2_HTC, MF2_WINDTHRUST }, // Is affected by wind
//  {"FLOORBOUNCE",    BF2_mix, MF2_FLOORBOUNCE }, // Bounces off the floor
      // see MBF/Prboom "BOUNCES"
  {"HERETICBOUNCE",  BF2_HTC, MF2_FLOORBOUNCE }, // Bounces off the floor
  {"THRUGHOST",      BF2_HTC, MF2_THRUGHOST }, // Will pass through ghosts (missile)
  {"FLOORCLIP",      BF2_HTC, MF2_FOOTCLIP }, // Feet may be be clipped
  {"SPAWNFLOAT",     BF2_HTC, MF2_SPAWNFLOAT }, // Spawned at random height
  {"NOTELEPORT",     BF2_HTC, MF2_NOTELEPORT }, // Does not teleport
  {"RIPPER",         BF2_HTC, MF2_RIP }, // Rips through solid targets (missile)
  {"PUSHABLE",       BF2_HTC, MF2_PUSHABLE }, // Can be pushed by other moving actors
//  {"SLIDE",          BF2_HTC, MF2_SLIDE }, // Slides against walls
     // see other "SLIDE", and MF_SLIDE
  {"PASSMOBJ",       BF2_HTC, MF2_PASSMOBJ }, // Enable z block checking.
      // If on, this flag will allow the mobj to pass over/under other mobjs.
  {"CANNOTPUSH",     BF2_HTC, MF2_CANNOTPUSH }, // Cannot push other pushable actors
  {"BOSS",           BF2_HTC, MF2_BOSS }, // Is a major boss, not as easy to kill
  {"FIREDAMAGE",     BF2_HTC, MF2_FIREDAMAGE }, // does fire damage
  {"NODAMAGETHRUST", BF2_HTC, MF2_NODMGTHRUST }, // Does not thrust target when damaging
  {"TELESTOMP",      BF2_HTC, MF2_TELESTOMP }, // Can telefrag another Actor
  {"FLOATBOB",       BF2_HTC, MF2_FLOATBOB }, // use float bobbing z movement
  {"DONTDRAW",       BF2_HTC, MF2_DONTDRAW }, // Invisible (does not generate a vissprite)
  // DoomLegacy 1.4x Internal flags, non-standard
  // Exist but have little use being set by a WAD
  {"ONMOBJ",         BF2_mix, MF2_ONMOBJ }, // mobj is resting on top of another (Heretic, but also DoomLegacy)
//  {"FEETARECLIPPED", BF2_HTC, MF2_FEETARECLIPPED }, // a mobj's feet are now being cut
//  {"FLY",            BF2_HTC, MF2_FLY }, // Fly mode

#ifdef MBF21
  // MBF21 Extensions
  {"LOGRAV",         BF2_M21, MF2_LOGRAV }, // Experiences only 1/8 gravity (see Heretic LOWGRAVITY)
//  {"BOSS", BF2_M21, MF2_BOSS }, // full volume sounds (see, death), splash immunity  (duplicate, see BOSS)
  {"RIP",            BF2_M21, MF2_RIP }, // projectile ripfull volume sounds (see, death), splash immunity (see RIPPER)
  // MBF21 unique extensions
  {"SHORTMRANGE",    BF3_M21, MF3_SHORTMRANGE }, // short missile range
  {"DMGIGNORED",     BF3_M21, MF3_DMGIGNORED }, // others ignore its attacks (like vile) 
  {"NORADIUSDMG",    BF3_M21, MF3_NORADIUSDMG }, // does not take damage from blast radius
  {"FORCERADIUSDMG", BF3_M21, MF3_FORCERADIUSDMG }, // force damage from blast radius
  {"HIGHERMPROB",    BF3_M21, MF3_HIGHERMPROB }, // missile attack prob min 37.5% (norm 22%)
  {"RANGEHALF",      BF3_M21, MF3_RANGEHALF }, // missile attack prob uses half distance
  {"NOTHRESHOLD",    BF3_M21, MF3_NOTHRESHOLD }, // no target threshold
  {"LONGMELEE",      BF3_M21, MF3_LONGMELEE }, // long melee range (like revenant)
  {"MAP07BOSS1",     BF3_M21, MF3_MAP07_BOSS1 }, // MAP07 boss1 type, (TAG 666)
  {"MAP07BOSS2",     BF3_M21, MF3_MAP07_BOSS2 }, // MAP07 boss1 type, (TAG 667)
  {"E1M8BOSS",       BF3_M21, MF3_E1M8_BOSS }, // E1M8 boss type
  {"E2M8BOSS",       BF3_M21, MF3_E2M8_BOSS }, // E2M8 boss type
  {"E3M8BOSS",       BF3_M21, MF3_E3M8_BOSS }, // E3M8 boss type
  {"E4M6BOSS",       BF3_M21, MF3_E4M6_BOSS }, // E4M6 boss type
  {"E4M8BOSS",       BF3_M21, MF3_E4M8_BOSS }, // E4M8 boss type
  {"FULLVOLSOUNDS",  BF3_M21, MF3_FULLVOLSOUNDS }, // full volume sounds (see, death)
#endif

#ifdef HEXEN  
  {"HEXENBOUNCE",    BF3_HEX, MF3_FULLBOUNCE }, // Bounces off walls and floor
  {"DONTBLAST",      BF3_HEX, MF3_NONBLAST }, // missile passes through   (see MF2_THRUGHOST)
  {"ACTIVATEIMPACT", BF3_HEX, MF3_IMPACT }, // Can activate SPAC_IMPACT
  {"CANPUSHWALLS",   BF3_HEX, MF3_PUSHWALL }, // Can activate SPAC_PUSH
  {"ACTIVATEMCROSS", BF3_HEX, MF3_MCROSS }, // Can activate SPAC_MCROSS
  {"ACTIVATEPCROSS", BF3_HEX, MF3_PCROSS }, // Can activate SPAC_PCROSS
  {"CANTLEAVEFLOORPIC", BF3_HEX, MF3_CANTLEAVEFLOORPIC }, // Stays within a certain floor texture
  {"NONSHOOTABLE",   BF3_HEX, MF3_NONSHOOTABLE }, // Transparent to missiles
  {"INVULNERABLE",   BF3_HEX, MF3_INVULNERABLE }, // Does not take damage
  {"DORMANT",        BF3_HEX, MF3_DORMANT }, // Cannot be damaged, is not noticed by seekers
  {"ICEDAMAGE",      BF3_HEX, MF3_ICEDAMAGE }, // does ice damage
  {"SEEKERMISSILE",  BF3_HEX, MF3_SEEKERMISSILE }, // Is a seeker (for reflection)
  {"REFLECTIVE",     BF3_HEX, MF3_REFLECTIVE }, // Reflects missiles
#endif

  // DoomLegacy 2.0 Extensions
  // Heretic/Hexen/ZDoom additions
//  {"SLIDESONWALLS",  BF2x, MF2_SLIDE }, // Slides against walls
//  {"QUICKTORETALIATE", BF2x, MF2_QUICKTORETALIATE },
//  {"NOTARGET",       BF2x, MF2_NOTARGET }, // Will not be targeted by other monsters of same team (like Arch-Vile)
//  {"CANPASS",        BF2x, 0, }, // TODO inverted!  Can move over/under other Actors
//  {"DONTSPLASH", BFC_x, MF_NOSPLASH }, // Does not cause splashes in liquid.
//  {"ISMONSTER", BFC_x, MF_MONSTER },
//  {"CEILINGHUGGER",  BF2x, MF2_CEILINGHUGGER },
// duplicate of others
//  {"FLOORHUGGER",    BF2x, MF2_FLOORHUGGER },  (duplicate)
//  {"FLOATBOB",       BF2x, MF2_FLOATBOB }, // Bobs up and down in the air (item) (duplicate)

// ZDoom (known from MBF21 via DSDA)
//  {"???",       BF2Z, MF2_CANUSEWALLS }, // Can activate use walls
//  {"???",       BF2Z, MF2_COUNTSECRET }, // pickup thing, is counted as a secret
  {NULL, BFexit, 0} // terminator
};

#ifdef MBF21
// [WDJ] From MBF developers spec.
flag_name_t  frame_flag_name_table[] = 
{
  {"SKILL5FAST",    BFR_M21, FRF_SKILL5_FAST }, // MBF21, tics halve on nightmare skill
  {NULL, BFexit, 0} // terminator
};
#endif

//#define CHECK_FLAGS2_DEFAULT
#ifdef CHECK_FLAGS2_DEFAULT
// Old PWAD do not know of MF2 bits.
// Default for PWAD that do not set MF2 flags
const uint32_t flags2_default_value = 0; // other ports like prboom
  // | MF2_PASSMOBJ // heretic monsters
  // | MF2_FOOTCLIP // heretic only
  // | MF2_WINDTHRUST; // requires heretic wind sectors
#endif



// Standardized state ranges
enum {
// Doom
  STS_TECH2LAMP4 = 966,
// TNT
  STS_TNT1 = 967,
// MBF
  STS_GRENADE = 968,
  STS_DOGS_PR_STND = 972,
  STS_DOGS_RAISE6 = 998,
  STS_MUSHROOM = 1075,
// Doom Beta
  STS_OLDBFG1 = 999,  // 1..43
  STS_PLS1BALL = 1042,
  STS_PLS2BALL = 1049,
  STS_BON3 = 1054,
  STS_BSKUL_STND = 1056,
  STS_BSKUL_DIE8 = 1074,
// HERETIC (as in vanilla)
  STH_FREETARGMOBJ = 1,
  STH_PODGENERATOR = 114,
  STH_SPLASH1 = 115,  // is S_HSPLASH1
  STH_TFOG1 = 223,  // is S_HTFOG1
  STH_TFOG13 = 235,
  STH_LIGHTDONE = 236,  // is S_HLIGHTDONE, but unused
  STH_STAFFREADY = 237,
  STH_CRBOWFX4_2 = 562,
  STH_BLOOD1 = 563,  // mapped to S_BLOOD1 (S_HBLOOD dis)
  STH_BLOOD3 = 565,
  STH_BLOODSPLATTER1 = 566,
  STH_BLOODSPLATTERX = 569,
  STH_PLAY = 570, // mapped to S_PLAY (Heretic S_PLAY dis)
  STH_PLAY_XDIE9 = 596,
  STH_PLAY_FDTH1 = 597,  // unmapped, TODO
  STH_PLAY_FDTH20 = 616,
  STH_BLOODYSKULL1 = 617,
  STH_REDAXE3 = 986,
  STH_SND_WATERFALL = 1204,
};

// Static remap
typedef struct {
    uint16_t  deh_value1, deh_value2;  // source range
    uint16_t  to_index;  // Legacy frame or sprite index
} remap_range_t;

remap_range_t  heretic_frame_remap[] =
{
   { 0, STH_CRBOWFX4_2,  S_FREETARGMOBJ },
   { STH_BLOOD1, STH_BLOOD3,  S_BLOOD1 },  // STH_BLOOD Mapped to the doom blood
   { STH_BLOODSPLATTER1, STH_BLOODSPLATTERX,  S_BLOODSPLATTER1 },
   { STH_PLAY, STH_PLAY_XDIE9,  S_PLAY },  // STH_PLAY Mapped to the doom player, except for STH_PLAY_FDTH.
//   { STH_PLAY_FDTH1, STH_PLAY_FDTH20, S_PLAY_FDTH1 },       // TODO
   { STH_BLOODYSKULL1, STH_SND_WATERFALL,  S_BLOODYSKULL1 },
};
#define NUM_heretic_frame_remap  (sizeof(heretic_frame_remap)/sizeof(remap_range_t))

remap_range_t  prboom_frame_remap[] =
{
   { 0, STS_TECH2LAMP4,  0 },
   { STS_TNT1, STS_TNT1,  S_TNT1 },
   { STS_GRENADE, STS_DOGS_RAISE6,  S_GRENADE },
   { STS_MUSHROOM, STS_MUSHROOM,  S_MUSHROOM },
   { STS_OLDBFG1, STS_BSKUL_DIE8,  S_FREETARGMOBJ },  // prboom deh compatibility blanks
};
#define NUM_prboom_frame_remap  (sizeof(prboom_frame_remap)/sizeof(remap_range_t))

remap_range_t  heretic_sprite_remap[] =
{
   { 0, 83,    SPR_HERETIC_START },
   { 84, 84,   SPR_BLUD },  // SPR_BLOD Mapped to the doom blood
   { 85, 85,   SPR_PLAY },  // SPR_PLAY Mapped to the doom player
   { 86, 125,  SPR_FDTH },
};
#define NUM_heretic_sprite_remap  (sizeof(heretic_sprite_remap)/sizeof(remap_range_t))

remap_range_t  prboom_sprite_remap[] =
{
   { 0, SPR_TLP2,  0 },
   { 138, 138,  SPR_TNT1 },
   { 139, 139,  SPR_DOGS },
};
#define NUM_prboom_sprite_remap  (sizeof(prboom_sprite_remap)/sizeof(remap_range_t))

remap_range_t  dsda_sprite_remap[] =
{
   { 0, SPR_TLP2,  0 },
   { 138, 138,  SPR_TNT1 },
   { 139, 139,  SPR_DOGS },
   // Not really supported, so put them somewhere where protected.
   { 140, 143,  SPR_AMS1 },  // 140 Plamsa fireball SPR_PLS1
                             // 141 Plasma fireball SPR_PLS2
                             // 142 Evil sceptre    SPR_BON3
                             // 143 Unholy bible    SPR_BON4
};
#define NUM_dsda_sprite_remap  (sizeof(dsda_sprite_remap)/sizeof(remap_range_t))

// woof, dsda: Index 140 to 144
const char *  dsda_sprite_names[] =
{
   "PLS1", // Plamsa fireball SPR_PLS1
   "PLS2", // Plamsa fireball SPR_PLS2
   "BON3", // Evil sceptre    SPR_BON3
   "BON4", // Unholy bible    SPR_BON4
   "BLD2", // Doom retro
};


#ifdef ENABLE_DEH_REMAP
#else
static byte blanked_flag = 0;  // blanking of prboom deh compatibility states
#endif

// Translate deh frame number to internal state index.
// return S_NULL when invalid state (deh frame)
// Necessary for DEH that add new states, like chex newmaps.wad, and MBF21.
statenum_t  DEH_frame_to_state( int deh_frame )
{
  remap_range_t  * remap;
  int  num_remap;
  uint16_t  stnum;

  all_detect |= deh_detect | deh_thing_detect | deh_key_detect;

  // Some old wads had negative frame numbers, which should be S_NULL.
  if( deh_frame <= 0 )  goto null_frame;
 
  // remapping
  if( EN_heretic )
  {
      remap = heretic_frame_remap;
      num_remap = NUM_heretic_frame_remap;
  }
  else
  {
      remap = prboom_frame_remap;
      num_remap = NUM_prboom_frame_remap;
  }

  // Fixed remapping
  while( num_remap-- > 0 )
  {
      if( (deh_frame >= remap->deh_value1) && (deh_frame <= remap->deh_value2) )
      {
          return  (deh_frame - remap->deh_value1) + remap->to_index;
      }
      remap++;
  }

#ifdef ENABLE_DEH_REMAP
  // This also blanks the states.
  stnum = remap_state( deh_frame );  // unknown to free states
#else
  stnum = deh_frame;

  // blank the new states
  if( !(blanked_flag & 0x01) && (stnum > NUMSTATES_DEF) )
  {
      blanked_flag |= 0x01;

      zero_states( NUMSTATES_DEF, NUMSTATES_EXT-1 );
  }

  if( EN_heretic )
  {
      // Too many doom states also used in heretic. Blanking would have to be selective.
  }
  else if( all_detect & (BDTC_boom|BDTC_prboom|BDTC_MBF|BDTC_MBF21|BDTC_dsda) )
  {
      if( !(blanked_flag & 0x02) && (stnum >= S_FREETARGMOBJ) && (stnum <= S_SND_WATERFALL) )
      {
          blanked_flag |= 0x02;
          zero_states( S_FREETARGMOBJ, S_SND_WATERFALL);
      }
      if( !(blanked_flag & (0x02|0x04)) && (stnum >= STS_OLDBFG1) && (stnum <= STS_BSKUL_DIE8)
//          && (all_detect & BDTC_prboom)
        )
      {
          blanked_flag |= 0x04;
          zero_states( S_FREETARGMOBJ, S_FREETARGMOBJ + (STS_BSKUL_DIE8 - STS_OLDBFG1) );
      }
  }
#endif

  if( stnum >= 0 && stnum < NUMSTATES_EXT )
       return  stnum;  // remapping range accepted


null_frame:
  return S_NULL;
}



static
uint16_t DEH_sprite_remapping( int deh_value )
{
  const char *  spr_rename = NULL;
  char buf1[10];
  remap_range_t  * remap;
  int  num_remap;
  uint16_t  max_predef_index = SPR_TNT1;
  uint16_t  spr_index = deh_value;

  all_detect |= deh_detect | deh_thing_detect | deh_key_detect;

  // Some old wads had negative frame numbers, which should be S_NULL.
  if( deh_value <= 0 )  goto null_sprite;
 
  // remapping
  if( EN_heretic )
  {
      // mapping from vanilla Heretic
      remap = heretic_sprite_remap;
      num_remap = NUM_heretic_sprite_remap;
      max_predef_index = SPR_HERETIC_MAX - SPR_HERETIC_START;
  }
  else if( all_detect & BDTC_dsda )
  {
      remap = dsda_sprite_remap;
      num_remap = NUM_dsda_sprite_remap;
      max_predef_index = SPR_TNT1;
  }
  else
  {
      remap = prboom_sprite_remap;
      num_remap = NUM_prboom_sprite_remap;
      max_predef_index = SPR_TNT1;
  }

  // Fixed remapping
  while( num_remap-- > 0 )
  {
      if( (deh_value >= remap->deh_value1) && (deh_value <= remap->deh_value2) )
      {
          spr_index = (deh_value - remap->deh_value1) + remap->to_index;
          break;  // need name too
      }
      remap++;
  }

  // Default names, fixed later for special cases.
  if( spr_index < SPR_HERETIC_MAX )
      spr_rename = sprnames[spr_index];

  // prboom and DSDA special names
  if( all_detect & (BDTC_dsda) )
  {
      // [WDJ] These will have been remapped to other slots with other names.
      if( (deh_value >= 140) && (deh_value <= 144 ) )
      {
          spr_rename = dsda_sprite_names[ deh_value - 140 ];
          if( verbose > 1 )
              GenPrintf(EMSG_info, "Sprite DSDA: sprite %i -> %s\n", deh_value, spr_rename );
      }
  }

#ifdef ENABLE_DEHEXTRA
  // [WDJ] The dehextra flag is set from finding lumps only used by DEHEXTRA.
  if( all_detect & (BDTC_dehextra | BDTC_dsda) )
  {
      if( (deh_value >= 145) && (deh_value <= 245 ) )  // SPR_SP00
      {
          // DEHEXTRA, SP00 to SP99
          snprintf( buf1, 8, "SP%02i", deh_value - 145);
          spr_rename = buf1;
          if( verbose > 1 )
              GenPrintf(EMSG_info, "Sprite DEHEXTRA: sprite %i -> %s\n", deh_value, spr_rename );
      }
      // If Heretic, then remap must be active.
  }
#endif

#ifdef ENABLE_DEH_REMAP
  if( deh_value > max_predef_index )
      spr_index = remap_sprite( deh_value, spr_rename );  // unknown to free sprites
#else
  // Would have to add these names to sprnames somehow.
#endif

  // Ensure that is within valid range.
  if( spr_index >= 0 && spr_index < NUMSPRITES_EXT )
       return  spr_index;

null_sprite:
  return 0xFFFF;  // Do not really have a NULL sprite.
}



// Implement assigns of a state num, from deh.
static
void  set_state( statenum_t * snp, int deh_frame_id )
{
  statenum_t si = DEH_frame_to_state(deh_frame_id);
  *snp = si;  // valid or S_NULL
  if( (deh_frame_id > 0) && (si == S_NULL) )
    GenPrintf(EMSG_errlog, "DEH/BEX set state has bad frame id: %i\n", deh_frame_id );
}


#ifdef MBF21
// [WDJ] MBF21 group assignment.
// This handles invalid group ids.
//   str : error report name
//   deh_group_0 : DEH declared groups start at this 0.
//   value : the input group value
//   deh_thing_id : the input thing id, for error reporting
// Returns a valid group id.
// Returns 0 when error, which is not a valid group id for deh assigned groups.
static byte  get_mbf21_group_id( const char * str, byte deh_group_0, int value, int deh_thing_id )
{
    if( value < 0 )
    {
        // Negative group not allowed
        deh_error("Thing %d : %s group invalid '%i'\n", deh_thing_id, str, value);
        return 0;
    }

    // DEH assigned group id start at deh_group_0
    if( value > (0xFF - deh_group_0) )  // DoomLegacy only allows byte for groups
    {
        int deh_value = value;
        // fold the group id, so to avoid rejecting it altogether.
        value = (((uint32_t)value) % 251);  // value too large, fold it
        deh_error("Thing %d : %s group invalid '%i', max=%i, fold into group=%i\n", deh_thing_id, str, deh_value, (0xFF - deh_group_0), value);
    }
    return (byte) (value + deh_group_0);  // DEH groups start at deh_group_0
}
#endif


// [WDJ] Thing type code substitutions.
// Not all ports agree on what these type codes are.
typedef struct {
    const char *  desc;
    uint16_t  prboom_type;
    uint16_t  ee_type; // eternity
    uint16_t  mt_type; // legacy
} deh_thing_desc_t;

// Thing ids in dehacked files are 1.., MT_xx codes are 0..
// These are all MT_xxx codes.
deh_thing_desc_t  deh_thing_desc_table[] =
{
  { "push",  137, 138, MT_PUSH },  // Boom
  { "pull",  138, 139, MT_PULL },  // Boom
  { "dog",   139, 140, MT_DOGS },  // MBF
  { "plasma1", 0, 141, MT_PLASMA1 },  // beta, from prboom
  { "plasma2", 0, 142, MT_PLASMA2 },  // beta, from prboom
  { "soul",    0, 142, MT_PLASMA2 },  // antaxyz
//  { "soul",    0, 142, MT_IMP },  // eviternity, antaxyz
  { "plasma3", 0, 144, MT_FIREBOMB }, // burst bfg, use Heretic missile
  { "camera",  0, 143, MT_CAMERA },  // SMMU
  { "lunch",   0, 143, MT_CAMERA },  // antaxyz
//  { "lunch",   0, 143, MT_HSPLASH },  // antaxyz
     // use Heretic splash to stop it using MT_DOGS.
  { "splash",  0,   0, MT_SPLASH },  // Legacy
  { "smoke",   0,   0, MT_SMOK },    // Legacy
  { "spirit",  0,   0, MT_SPIRIT },  // Legacy
};


// Use TTC codes
typedef enum { TTC_unk, TTC_legacy, TTC_boom, TTC_prboom, TTC_ee, TTC_mbf, TTC_heretic }  TTC_spec_code_e;
static TTC_spec_code_e  deh_thing_code = TTC_unk;  // command line setting

// Indexed by TTC_spec_code_e, skipping TTC_unk.
const char *  ttc_name_table[] = {
   "legacy",  // TTC_legacy
   "boom",    // TTC_boom
   "prboom",  // TTC_prboom
   "ee",      // TTC_ee
   "mbf",     // TTC_mbf
   "heretic", // TTC_heretic
};
const uint16_t ttc_name_to_bdtc[] = {
  0,
  BDTC_legacy,  // TTC_legacy
  BDTC_boom,    // TTC_boom
  BDTC_prboom,  // TTC_prboom
  BDTC_ee,      // TTC_ee
  BDTC_MBF,     // TTC_mbf
  BDTC_heretic, // TTC_heretic
};

// From command line
static
void  DEH_set_deh_thing_code( const char *  str )
{
    TTC_spec_code_e  ttt;

    for( ttt = TTC_legacy; ttt <= TTC_heretic; ttt++ )
    {
        if( strcasecmp( ttc_name_table[ttt-1], str ) == 0 )
        {
            deh_thing_code = ttt;   // DEH global setting
            all_detect |= ttc_name_to_bdtc[ ttt ];
            break;
        }
    }
}


// Return an mt_xxx, or 0.
//  deh_thing_code: thing type coding in dehacked
//  ttin : thing type as read in dehacked
uint16_t  lookup_thing_type( const char * str, uint16_t ttin )
{
    deh_thing_desc_t * dtd;

    all_detect |= deh_detect | deh_thing_detect | deh_key_detect;  // BDTC_xxx

    TTC_spec_code_e  ttc = deh_thing_code;  // DEH command line setting
    if( ttc == TTC_unk )
    {
        if( all_detect & BDTC_legacy )  ttc = TTC_legacy;
//      else if( all_detect & BDTC_MBF )  ttc = TTC_ee;
        else if( all_detect & BDTC_boom )  ttc = TTC_boom;
    }

    // Thing code translations
    if( (ttc == TTC_prboom)
        || ((ttc == TTC_boom) && (ttin <= 139)) )
    {
        for( dtd = & deh_thing_desc_table[0]; (byte*)dtd < ((byte*)deh_thing_desc_table + sizeof(deh_thing_desc_table)); dtd++ )
        {
            if( dtd->prboom_type == ttin )
                 return dtd->mt_type;
        }
    }

    if( ttc == TTC_ee )
    {
        for( dtd = & deh_thing_desc_table[0]; (byte*)dtd < ((byte*)deh_thing_desc_table + sizeof(deh_thing_desc_table)); dtd++ )
        {
            if( dtd->ee_type == ttin )
                 return dtd->mt_type;
        }
    }

    // Look at the comments for characteristic words.
    for( dtd = & deh_thing_desc_table[0]; (byte*)dtd < ((byte*)deh_thing_desc_table + sizeof(deh_thing_desc_table)); dtd++ )
    {
//#if defined( __MINGW32__ ) || defined( __WATCOM__ ) || defined( SMIF_X11 )
#if 1
        if( dl_strcasestr( str, dtd->desc ) )
#else
// must #define  __USE_GNU to get strcasestr from strings.h
        if( strcasestr( str, dtd->desc ) )
#endif
        {
            byte ttc = dtd->mt_type;
            deh_thing_detect |= ttc;
            return ttc;  // if no direct code is found
        }
    }

    return ttin;  // default, keep same
}

//  thing_in : thing id input
static
uint16_t translate_thing_id( uint16_t thing_in, const char * thing_word )
{
    uint16_t thing_id = thing_in;
    
    if( thing_id >= 138 && thing_id <= 150 )  // problem area
    {
        // Test for keywords in description
        const char * nc = thing_word + strlen(thing_word) + 1;  // skip past token to get to comments
        // substitute to thing_id
        thing_id = lookup_thing_type( nc, thing_id - 1 ) + 1;
    }

    if( thing_id <= 0 || thing_id > NUMMOBJTYPES )
    {
        deh_error("Thing %d don't exist\n", (int)thing_in );
        thing_id = MT_UNK1 + 1;  // 1..
    }

    return thing_id;
}

#ifdef MBF21
// [WDJ] DoomLegacy has 32bit flags.

// The Standard flag order for MBF21 functions, according to DSDA.
// MF_SPECIAL to MF_NOTDMATCH are as in Doom2.
// MF_TRANSLATION1 to MF_TRANSLATION2 are as in Doom2, but we have moved them.
// MF_TOUCHY to MF_FRIEND are as in MBF
// MF_TRANSLUCENT as in Boom

static inline
uint32_t convert_std_flags_to_DL2( uint32_t flags )
{
    // Do not know exactly which of the Heretic flags that MBF21 has adopted.
    return (flags & (MF2_HERETIC_BIT_MASK) );  // At least these match in DSDA.
    // This is a guess.
//    return (value64 && (MF2_FLOORBOUNCE|MF2_RIP|MF2_BOSS|MF2_FIREDAMAGE|MF2_NODMGTHRUST) );
}

// The MBF21 flag order for MBF21 functions, according to DSDA.
// MF2_LOGRAV
// MF2_SHORTMRANGE to MF2_LONGMELEE as in MBF21 bits.
// MF2_BOSS
// MF2_MAP07BOSS to MF2_E4M8BOSS as in MBF21 bits.
// MF2_RIP
// MF2_FULLVOLSOUNDS

static inline
uint32_t convert_MBF21_flags_to_DL2( uint32_t mbf21_flags )
{
    uint32_t flags2 = 0;
    if( mbf21_flags & 0x0001 )
      flags2 |= MF2_LOGRAV;
    if( mbf21_flags & 0x0200 )
      flags2 |= MF2_BOSS;
    if( mbf21_flags & 0x2000 )
      flags2 |= MF2_RIP;
    return flags2;
}

static inline
uint32_t convert_MBF21_flags_to_DL3( uint32_t mbf21_flags )
{
    // [WDJ] Convert to flags3, as declared in p_mobj.h.
    // Because the bits are in the same order, except for the ones above,
    // we can shift them into place in groups.
    return (  ( (mbf21_flags & (0x000001FE)) >>1)
              | ( (mbf21_flags & (0x0001FC00)) >>2)
              | ( (mbf21_flags & (0x00040000)) >>3) );
}
#endif


#ifdef MBF21
// [WDJ] Fix DEH args.
// Note: Standard default is to set them all to 0.
// Any that default to 0, does not need to be done explicitly.
typedef enum {
    DFAF_args_select_mask = 0x03,
    DFAF_mask_id  = 0x0C,
    DFAF_thing_id = 0x04,  // args[0] is a thing_id
    DFAF_state_id = 0x08,  // args[0] is a state_id
    DFAF_sound_id = 0x0C,  // args[0] is a sound_id
//    DFAF_thing_id_parm1 = 0x10,
    DFAF_mobjflags = 0x20,
    // [WDJ] Others were defined for DSDA, but were not used.
} deh_fix_args_flags_e;

typedef struct deh_fix_args_s
{
    action_fi_t  action;  // action index
    byte       fix_flags;
    // [WDJ] Seem to only need 3 default args
    byte       first_default_arg_index;
    byte       num_default_args;
    uint32_t   def_args[3];
} deh_fix_args_t;

// Functions that have args that need adjustment.
deh_fix_args_t  deh_fix_args_table[] =
{
    // [WDJ] Some functions have thing_id that need translation.  Discovered in DSDA code.
//    {AI_Spawn_MBF, (DFAF_thing_id_parm1 | 0), 0, 0, {0, 0, 0} },  // Spawn object, parm1, parm2
      // parm1 is thing_id
    {AI_SpawnObject_MBF21, (DFAF_thing_id | 0), 0, 0, {0, 0, 0} },  // Spawn object, args[0..7]
      // args[0] is thing_id
      // default {0}
    {AI_MonsterProjectile_MBF21, (DFAF_thing_id | 0), 0, 0, {0, 0, 0} },  // Monster projectile attack, args[0..4]
      // args[0] is thing_id
      // default {0}
    {AP_WeaponJump_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // CheckAmmo, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AP_CheckAmmo_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // CheckAmmo, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AP_RefireTo_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Check refire button, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AP_GunFlashTo_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Gunflash, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AP_WeaponSound_MBF21, (DFAF_sound_id | 0), 0, 0, {0, 0, 0} },  // Weapon sound, args[0..1]
      // args[0] (parm1) is sound_id
    {AI_HealChase_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Heal, Jump if corpse, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AI_MonsterBulletAttack_MBF21, 0,  2, 3, {1, 3, 5}},  // Monster bullet attack, args[0..4]
      // default {0, 0, 1, 3, 5}
    {AI_MonsterMeleeAttack_MBF21, (DFAF_sound_id | 0), 0, 2, {3, 8, 0}},  // Monster melee attack, args[0..3]
      // args[0] (parm1) is sound_id
      // default {3, 8, 0, 0}
    {AP_WeaponProjectile_MBF21, (DFAF_thing_id | 0), 0, 0, {0, 0, 0}},  // Player projectile attack, args[0..4]
      // args[0] is thing_id
      // default {0}
    {AP_WeaponBulletAttack_MBF21, 0, 2, 3, {1, 5, 3}},  // Player bullet attack, args[0..4]
      // default {0, 0, 1, 5, 3 }
    {AP_WeaponMeleeAttack_MBF21, 0, 0, 3, {2, 10, 1*FRACUNIT}},  // Player melee attack, args[0..4]
      // default {2, 10, 1*FRACUNIT, 0, 0 }
    {AI_AddFlags_MBF21, (DFAF_mobjflags | 0), 0, 0, {0, 0, 0} },  // Add flags to mobj, args[0..1]
      // args[0] are mobj flags
      // args[1] are MBF21 flags => args[1], args[7]
    {AI_RemoveFlags_MBF21, (DFAF_mobjflags | 0), 0, 0, {0, 0, 0} },  // Add flags to mobj, args[0..1]
      // args[0] are mobj flags
      // args[1] are MBF21 flags => args[1], args[7]
    {AI_JumpIfHealthBelow_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Jump if Health, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AI_JumpIfTargetInSight_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Jump if Target seen, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AI_JumpIfTargetCloser_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Jump if Target close, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AI_JumpIfTracerInSight_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Jump if Tracer seen, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AI_JumpIfTracerCloser_MBF21, (DFAF_state_id | 0), 0, 0, {0, 0, 0} },  // Jump if Tracer close, set state, args[0..1]
      // args[0] (parm1) is state_id
    {AI_JumpIfFlagsSet_MBF21, (DFAF_state_id | DFAF_mobjflags | 1), 0, 0, {0, 0, 0} },  // Add flags to mobj, args[0..2]
      // args[0] (parm1) is state_id
      // args[1] are mobj flags
      // args[2] are MBF21 flags => args[2], args[7]
};
#define NUM_deh_fix_args  (sizeof(deh_fix_args_table)/sizeof(deh_fix_args_t))


void fix_action_args( action_fi_t action1, state_t * st )
{
    state_ext_t * sep;
    deh_fix_args_t * dfap = deh_fix_args_table;

    // Lookup the action in the fix table.
    int i;
    for( i=0; i<NUM_deh_fix_args ; i++, dfap++ )
    {
        // [WDJ] all types of actions comparable, as are now enum.
        if( dfap->action == action1 )  // match
            goto match_action;
    }
    return;

match_action:
    // All functions in the list need to modify parameters, so need real parameters.
    if( st->state_ext_id )
    {
        sep = P_state_ext( st );  // get existing args
        if( sep->args_set == 0xFF )
          return; // were already handled
    }
    else
    {
        sep = P_create_state_ext( st );  // there were no parameters, create some
    }

    if( dfap->num_default_args )
    {
        int aj = dfap->first_default_arg_index;
        for( i=0; i< dfap->num_default_args; i++, aj++ )
        {
            // If still default 0, then replace with default value.
            if( (sep->parm_args[aj] == 0) && !(sep->args_set & (1<<aj)) )
            {
//              sep->args_set |= (1<<i);  // default only once
                sep->parm_args[aj] = dfap->def_args[i];
            }
        }
    }

#if 0
    // Args fixed interferes with parm fixed.
    if( (dfap->fix_flags & DFAF_thing_id_parm1) && (sep->parm_fixed == 0) )
    {
        // Translate DEH thing_id to DoomLegacy thing id.
        // This is always arg[0].
        sep->parm1 = translate_thing_id( sep->parm1, "" );
        sep->parm_fixed = 0xFF;  // done with fixing
    }
#endif
    
    if( dfap->fix_flags & DFAF_mask_id )
    {
        byte fixarg0 = dfap->fix_flags & DFAF_mask_id;
        // must ensure this is done only once
        if( fixarg0 == DFAF_thing_id )
        {
            // Translate DEH thing_id to DoomLegacy thing id.
            // This is always arg[0].
            sep->parm_args[0] = translate_thing_id( sep->parm_args[0], "" );
        }
        if( fixarg0 == DFAF_state_id )
        {
            // Translate DEH state_id to DoomLegacy state id.
            // This is always arg[0].
            sep->parm_args[0] = DEH_frame_to_state( sep->parm_args[0] );
        }
        if( fixarg0 == DFAF_sound_id )
        {
            // Translate DEH sound_id to DoomLegacy sound id.
            // This is always arg[0].
#ifdef ENABLE_DEH_REMAP
            all_detect |= deh_detect | deh_thing_detect | deh_key_detect;
            sep->parm_args[0] = remap_sound( sep->parm_args[0] );
#else
            // will fail
#endif
        }
    }

    if( dfap->fix_flags & DFAF_mobjflags )
    {
        int i2 = dfap->fix_flags & DFAF_args_select_mask;
        sep->parm_args[ i2 ] = convert_std_flags_to_DL2( sep->parm_args[ i2 ] );  // MF_
        // The mbf21 flags are in MF2_ and MF3_ in DoomLegacy.
        uint32_t mbf21_flags = sep->parm_args[ i2+1 ];
        sep->parm_args[ i2+1 ] = convert_MBF21_flags_to_DL2( mbf21_flags );  // MF2_
        sep->parm_args[ 7 ] = convert_MBF21_flags_to_DL3( mbf21_flags );  // MF3_
    }

    sep->args_set = 0xFF;  // done with defaulting
    return;
}

#endif

void fix_action_parm( action_fi_t action1, state_t * st )
{
    if( action1 == AI_Spawn_MBF )  // match
    {
        state_ext_t * sep;
        if( st->state_ext_id )
        {
            sep = P_state_ext( st );  // get existing args
            if( sep->parm_fixed == 0xFF )
              return; // were already handled
        }
        else
        {
            sep = P_create_state_ext( st );  // there were no parameters, create some
        }

        // Translate DEH thing_id to DoomLegacy thing id.
        // This is always parm1.
        sep->parm1 = translate_thing_id( sep->parm1, "" );  // parm1
        sep->parm_fixed = 0xFF;  // done with fixing
    }

    return;
}



static void  check_invalid_state( int si, state_t * st )
{
    byte st1 = lookup_state_acp( si );
    byte f1 = action_acp( st->action );
    byte sp1 = lookup_sprite_acp( st->sprite );
    uint16_t nxt = st->nextstate;
    byte t1 = st1 | f1 | sp1;  // 0,1,2 values
    
    if( t1 > 2 )
    {
        deh_error( "DEH: state %i acp inconsistent, state %i, sprite %i, action %i\n",
                    si, st1, sp1, f1 );
    }
    if( t1 && nxt )  // do not check state 0.
    {
        state_t * nxtp = & states[nxt];
        byte st2 = lookup_state_acp( nxt );
        byte sp2 = lookup_sprite_acp( nxtp->sprite );
        byte f2 = st2 ? action_acp( nxtp->action ) : 0;
        byte t2 = st2 | f2 | sp2;  // 0,1,2 values
        if( (t1 | t2) > 2 )
        {
            deh_error( "DEH: state %i acp inconsistent (%i) with next state (%i): state %i, sprite %i, action %i\n",
                    si, t1, nxt, st2, sp2, f2 );
        }
    }
}




// Have read  "Thing <deh_thing_id>"
static
void read_thing( myfile_t * f, int deh_thing_id )
{
  // DEH thing 1.. , but mobjinfo array is 0..
  uint16_t thing_id = deh_thing_id - 1;
  mobjinfo_t *  mip = & mobjinfo[ thing_id ];
  char s[MAXLINELEN];
  char * word;
  int value;
  uint32_t flags1, flags2, tflags;
#ifdef MOBJ_HAS_FLAGS3
  uint32_t flags3;  // MBF21 and HEXEN
#endif
#ifdef MBF21
  byte explicit_mbf21_bits;
#endif
#ifdef HEXEN
  byte explicit_hexen_bits;  // ???
#endif
  byte flags_clear = 0;  // 0x01 flags1, 0x02 flags2, 0x04 flags3
  byte flags_lock = 0;  // once cleared or set, they are locked for duration of this thing block

  // [WDJ] Attempt to clear thing first, as it may be Heretic.
  // Not clearing caused floating trees in MBF21 wads.
  if( ! is_predef_sprite( thing_id ) )
  {
      // Init the mobjinfo
      //  memset( mip, 0, sizeof(mobjinfo_t) );
      *mip = mobjinfo[ MT_UNK1 ];  // dummy default state
      // Must init properly because they may not set Bits at all.
      mip->flags = 0;
      mip->flags2 = 0;  // Bits has only partial clear
      flags_clear = 0xFF;  // all
  }

  do{
    if(myfgets(s,sizeof(s),f)!=NULL)  // get line
    {
      if(s[0]=='\n') break;
      if( s[0] == '#' ) continue;  // comment
      value=searchvalue(s);
      word=strtok(s," ");
      if( !word || word[0]=='\n' )  break;
      if( ! confirm_searchvalue(value)  )  continue;

      // Wads that use Bits: phobiata, hth2, DRCRYPT
#ifdef MBF21
      explicit_mbf21_bits = (strcasecmp(word,"MBF21") == 0);  // MBF21 bits assign
      if( explicit_mbf21_bits || !strcasecmp(word,"Bits"))  // "MBF21", "MBF21 Bits", "Bits"
#else
      if(!strcasecmp(word,"Bits"))
#endif
      {
          flag_name_t * fnp; // BEX flag names ptr

          flags1 = flags2 = tflags = 0;
#ifdef MOBJ_HAS_FLAGS3
          flags3 = 0;
#endif
          // flags_clear may have been set already.
          // Otherwise, except for explicit replacement, this assumes
          // it is only setting additional bits in an existing mobj.

          for(;;)  // to collect bit values
          {
              word = strtok(NULL, " +|\t=\n");

              // If no value, then still must clear bits.
              if( word == NULL )  goto set_flags_next_line;
              if( word[0] == '\n' )  goto set_flags_next_line;

              if( !strcasecmp(word,"Bits") ) continue;  // was "MBF21 Bits"

              // detect bits by integer value
              // MBF DRCRYPT.WAD sets color and MF_FRIEND using Bits, with neg value.
              // Prevent using signed char as index.
              if( isdigit( (unsigned char)word[0] ) || word[0] == '-' )
              {
#ifdef MBF21
                  if( explicit_mbf21_bits )
                  {
                      uint32_t mbf21_v32 = str_to_uint32(word);
                      // Handle MBF21 bits numeric assign.
                      // This is according to chart in MBF21 developer_spec.md.
                      flags2 |= convert_MBF21_flags_to_DL2( mbf21_v32 );
                      flags3 = convert_MBF21_flags_to_DL3( mbf21_v32 );  // overwrite all of MF3 flags
                      // OR of flags2 and flags3 to mip flags is done in set_flags_next_line
                      mip->flags2 &= ~(MF2_HERETIC_BIT_MASK); // clear the same bits cleared in DSDA-Doom
                      mip->flags3 &= ~(MF3_MBF21_BIT_MASK);   // clear the MBF21 bits
                      // do not need to use flags_clear due to explicit clear here
                      goto set_flags_next_line;
                  }
                  else
                  {
                      flags_clear |= 0x01;  // flags1
                  }
#else
                  // Bits value replaces existing bits
                  flags_clear |= 0x01;  // flags1
#endif
                  // old style integer value, flags only (not flags2)
                  flags1 = atoi(word);  // numeric entry
                  if( flags1 & MF_TRANSLUCENT )
                  {
                      // Boom bit defined in boomdeh.txt
                      // Was MF_FLOORHUGGER bit, and now need to determine which the PWAD means.
                      // MBF DRCRYPT.WAD has object with bits =
                      // MF_TRANSLUCENT, MF_COUNTKILL, MF_SHADOW, MF_SOLID, MF_SHOOTABLE.
                      GenPrintf(EMSG_errlog, "Sets flag MF_FLOORHUGGER or MF_TRANSLUCENT by numeric, guessing ");
                      if( flags1 & (MF_NOBLOCKMAP|MF_MISSILE|MF_NOGRAVITY|MF_COUNTITEM|MF_SHADOW))
                      {
                          // assume TRANSLUCENT, check for known exceptions
                          GenPrintf(EMSG_errlog, "MF_TRANSLUCENT\n");
                      }
                      else
                      {
                          // Don't know of any wads setting FLOORHUGGER using
                          // bits.
                          // assume FLOORHUGGER, check for known exceptions
                          flags1 &= ~MF_TRANSLUCENT;
                          flags2 |= MF2_FLOORHUGGER;
                          GenPrintf(EMSG_errlog, "MF_FLOORHUGGER\n");
                      }
                  }
                  if( flags1 & MFO_TRANSLATION4 )
                  {
                      // Color translate, moved to tflags.
                      GenPrintf(EMSG_errlog, "Sets color flag MF_TRANSLATE using Bits\n" );
                      tflags |= (flags1 & MFO_TRANSLATION4) >> (MFO_TRANSSHIFT - MFT_TRANSSHIFT);
                      flags1 &= ~MFO_TRANSLATION4;
                  }
                  // we are still using same flags bit order
                  goto set_flags_next_line;
              }
              // handle BEX flag names
              for( fnp = &BEX_flag_name_table[0]; ; fnp++ )
              {
                  if(fnp->name == NULL)  goto name_unknown;
                  if(!strcasecmp( word, fnp->name ))  // find name
                  {
                      switch( fnp->ctrl )
                      {
                       case BF1:
                         flags1 |= fnp->flagval;
                         break;
                       case BF1_MBF: // MBF names
                         deh_thing_detect |= BDTC_MBF;
                         flags1 |= fnp->flagval;
                         break;
                       case BF2x: // DoomLegacy extension BEX name
                         deh_thing_detect |= BDTC_legacy;
                       case BF2_mix: // standard name that happens to be MF2
                         flags2 |= fnp->flagval;
                         break;
                       case BF2_HTC: // Heretic names
                         deh_thing_detect |= BDTC_heretic;
                         flags2 |= fnp->flagval;
                         break;
#ifdef MBF21
                       case BF2_M21: // MBF21 name
                         deh_thing_detect |= BDTC_MBF21;
                         flags2 |= fnp->flagval;
                         break;
                       case BF3_M21: // MBF21 name
                         deh_thing_detect |= BDTC_MBF21;
                         flags3 |= fnp->flagval;
                         break;
#endif
#ifdef HEXEN
                       case BF3_HEX: // Hexen name
                         deh_thing_detect |= BDTC_hexen;
                         flags3 |= fnp->flagval;
                         break;
#endif
                       case BFT: // standard name that happens to be MFT
                         tflags |= fnp->flagval;
                         break;
                       default:
                         goto name_unknown;
                      }
                      // if next entry is same keyword then process it too
                      if( (fnp[1].name != fnp[0].name) )
                         goto next_word; // done with this word
                      // next entry is same word, process it too
                  }
              }
            name_unknown:
              deh_error("Bits name unknown: %s\n", word);
              // continue with next keyword
            next_word:
              continue;
          }  // for

        set_flags_next_line:
          // Clear the flags only once for a Thing
          // and only if Bits was used.
          flags_clear &= ~ flags_lock;  // block locked flags
          if( flags_clear & 0x01 )
          {
              flags_lock |= 0x01;
              mip->flags = 0;
              // should also clear any flags that got moved to flags2 and flags3.
          }
          if( flags_clear & 0x02 )
          {
              flags_lock |= 0x02;
              if( deh_thing_detect & (BDTC_legacy | BDTC_heretic) )
              {
                  // clear extension flags2 only if some extension names or heretic names appeared
                  mip->flags2 &= ~(MF2_FLOORHUGGER|MF2_NOCLIPTHING|(MF2_HERETIC_BIT_MASK & ~MF2_SLIDE));
              }
              // Flags adopted from Heretic that we must have now, and most dehacked lumps do not know about.
              if( thing_id == MT_PLAYER || thing_id == MT_CHASECAM || thing_id == MT_CAMERA || thing_id == MT_SPIRIT )
              {
                  // PLAYER: do not clear flags MF2_WINDTHRUST | MF2_FOOTCLIP | MF2_SLIDE | MF2_PASSMOBJ | MF2_TELESTOMP
                  // that Legacy has adopted for player.
                  mip->flags2 &= ~(MF2_FLOORBOUNCE);
                  mip->flags2 |= player_default_flags2;
              }
              else
              {
                  // clear std flags in flags2
                  mip->flags2 &= ~(MF2_SLIDE|MF2_FLOORBOUNCE);
                  mip->flags2 |= mon_default_flags2;  // is currently 0
              }
          }
#ifdef MOBJ_HAS_FLAGS3
          if( flags_clear & 0x04 )
          {
              flags_lock |= 0x04;
              mip->flags3 = 0;
          }
#endif
          flags_clear = 0;

          mip->flags |= flags1;
          if( flags1 )
              flags_lock |= 0x01;

          mip->flags2 |= flags2;
          if( flags2 )
              flags_lock |= 0x02;

#ifdef MOBJ_HAS_FLAGS3
          mip->flags3 |= flags3;
          if( flags3 )
              flags_lock |= 0x04;
#endif
          mip->tflags = tflags;  // info extension has color now
          thing_flags_valid_deh = true;
          continue; // next line
      }

      // set the value in apropriet field
      else if(!strcasecmp(word,"ID"))           mip->doomednum   =value;
      else if(!strcasecmp(word,"Initial"))      set_state( &mip->spawnstate, value );
      else if(!strcasecmp(word,"Hit"))          mip->spawnhealth =value;
      else if(!strcasecmp(word,"First"))        set_state( &mip->seestate, value );
      else if(!strcasecmp(word,"Alert"))        mip->seesound    =value;
      else if(!strcasecmp(word,"Reaction"))     mip->reactiontime=value;
      else if(!strcasecmp(word,"Attack"))       mip->attacksound =value;
      else if(!strcasecmp(word,"Injury"))       set_state( &mip->painstate, value );
      else if(!strcasecmp(word,"Pain"))
      {
          word=strtok(NULL," ");
          if( word )
          {
             if(!strcasecmp(word,"chance"))     mip->painchance  =value;
             else if(!strcasecmp(word,"sound")) mip->painsound   =value;
          }
      }
      else if(!strcasecmp(word,"Close"))        set_state( &mip->meleestate, value );
      else if(!strcasecmp(word,"Far"))          set_state( &mip->missilestate, value );
      else if(!strcasecmp(word,"Death"))
      {
         word=strtok(NULL," ");
         if( word )
         {
             if(!strcasecmp(word,"frame"))      set_state( &mip->deathstate, value );
             else if(!strcasecmp(word,"sound")) mip->deathsound  =value;
         }
      }
      else if(!strcasecmp(word,"Exploding"))    set_state( &mip->xdeathstate, value );
      else if(!strcasecmp(word,"Speed"))        mip->speed       =value;
      else if(!strcasecmp(word,"Width"))        mip->radius      =value;
      else if(!strcasecmp(word,"Height"))       mip->height      =value;
      else if(!strcasecmp(word,"Mass"))         mip->mass        =value;
      else if(!strcasecmp(word,"Missile"))      mip->damage      =value;
      else if(!strcasecmp(word,"Action"))       mip->activesound =value;
      else if(!strcasecmp(word,"Bits2"))        mip->flags2      =value;
      else if(!strcasecmp(word,"Respawn"))      set_state( &mip->raisestate, value );
#ifdef MBF21
      // MBF21
      // [WDJ] From examination of DSDA-Doom, as source of MBF21 changes.
      // infighting group
      else if(!strcasecmp(word,"infighting"))
      {
          deh_key_detect |= BDTC_MBF21;
          byte deh_group = get_mbf21_group_id( "infighting", IG_DEH_GROUP, value, deh_thing_id );
          if( deh_group == 0 )  // invalid, rejected
              continue;

          mip->infight_group = deh_group;
      }
      // projectile group
      else if(!strcasecmp(word,"projectile"))
      {
          deh_key_detect |= BDTC_MBF21;
          byte deh_group;
          if( value < 0 )
          {
              // Negative group means NO IMMUNITY to any projectile.
              deh_group = PG_NO_IMMUNITY;
          }
          else
          {
              deh_group = get_mbf21_group_id( "projectile", PG_DEH_GROUP, value, deh_thing_id );
              // cannot be rejected
          }
          mip->projectile_group = deh_group;
      }
      // splash group
      else if(!strcasecmp(word,"splash"))
      {
          deh_key_detect |= BDTC_MBF21;
          byte deh_group = get_mbf21_group_id( "splash", SG_DEH_GROUP, value, deh_thing_id );
          if( deh_group == 0 )  // invalid, rejected
              continue;

          mip->splash_group = deh_group;
      }
      // rip sound
      else if(!strcasecmp(word,"rip sound"))
      {
          deh_key_detect |= BDTC_MBF21;
          mip->rip_sound = value;
      }
      // fast speed
      else if(!strcasecmp(word,"fast speed"))
      {
          // [WDJ] Store in monster alt info structure.
          set_alt_speed( deh_thing_id, mip->speed, value ); // normal speed, fast speed
          deh_key_detect |= BDTC_MBF21;
      }
      // melee range
      else if(!strcasecmp(word,"melee range"))
      {
          deh_key_detect |= BDTC_MBF21;
          mip->melee_range = value;
      }
#endif
      else if(!strcasecmp(word,"dropped"))  // Dropped item
      {
          deh_key_detect |= BDTC_dehextra;
#ifdef ENABLE_DEHEXTRA
# if 0
          // [WDJ] The items are specified by number, but this could be expanded to use text string
          if( value == 0 )
              value = parse_mobj_name( s );
# endif
          mip->dropped_item = value;
#else
          deh_error("Thing %d : Dropped not supported\n", deh_thing_id);
#endif
      }
      else deh_error("Thing %d : unknown word '%s'\n", deh_thing_id, word);
    }
  } while(s[0]!='\n' && !myfeof(f)); //finish when the line is empty
}


//  si : state index
static void action_function_store( uint16_t deh_frame_num, uint16_t si, const char * funcname );


// Have read  "Frame <deh_frame_id>"
/*
Sprite number = 10
Sprite subnumber = 32968
Duration = 200
Next frame = 200
// used as param 1 and param2 by MBF functions
Unknown 1 = 5
Unknown 2 = 17
*/
static
void read_frame( myfile_t* f, int deh_frame_id )
{
  state_t * fsp;
  char s[MAXLINELEN];
  char * word1, * word2;
  int value;
   
  // Syntax: "Frame <num>"
  int si = DEH_frame_to_state(deh_frame_id);
  if( si == S_NULL )
  {
    deh_error("Frame %d don't exist\n", deh_frame_id);
    return;
  }

  fsp = & states[ si ];
   
  do{
    if(myfgets_nocom(s,sizeof(s),f)!=NULL)
    {
      if(s[0]=='\n') break;
      value=searchvalue(s);
      // set the value in appropriate field
      word1=strtok(s," ");
      if(!word1 || word1[0]=='\n') break;

      word2=strtok(NULL," ");  // may not be needed, so may be NULL
      if( ! confirm_searchvalue(value)  )  continue;

      if(!strcasecmp(word1,"Sprite"))
      {
        if( word2 )
        {
          // Syntax: Sprite number = <num>
          if(!strcasecmp(word2,"number"))
          {
              fsp->sprite = DEH_sprite_remapping( value );
          }
          // Syntax: Sprite subnumber = <num>
          else if(!strcasecmp(word2,"subnumber"))
              fsp->frame = value;
        }
      }
        // Syntax: Duration = <num>
      else if(!strcasecmp(word1,"Duration"))
          fsp->tics = value;
        // Syntax: Next frame = <num>
      else if(!strcasecmp(word1,"Next"))
      {
#ifdef STATE_FUNC_CHECK
//          if( cv_state_check.EV )
          if( verbose >= 2 )
          {
              byte si_state_acp = lookup_state_acp( si );
              byte next_state_acp = lookup_state_acp( value );
              if( si_state_acp != next_state_acp )
              {
                  deh_error( "BEX: state(%i) acp(%i) gets new next state(%i) acp(%i) \n", si, si_state_acp, value, next_state_acp );
              }
          }
#endif
          set_state( &fsp->nextstate, value );
      }
      else if(!strcasecmp(word1,"Unknown"))
      {
        // Syntax: Unknown 2 = <num>
        // MBF uses these for parameters (parm1, parm2), and to draw weapon sprite.
        // MBF21 uses these while also using args[] in A_xxx functions.
        if( word2 )
        {
            state_ext_t * sep = P_create_state_ext( fsp );
            if( word2[0] == '1' ) sep->parm1 = value;  // misc1
            else if( word2[0] == '2' ) sep->parm2 = value;  // misc2
        }
      }
#ifdef MBF21
      else if( strcasecmp(word1,"Args") > 0 )  // Args1 to Args8
      {
          // MBF21 uses these for A_xxx function parameters.
          // Syntax: Args4 = <num>
          if( strlen(word1) != 5 )  goto argerr;
          char an = word1[4]; // 1 to 8
          if( an >='1' && an <= '8' )
          {
              int ai = an - '1';  // 0 to 7
              state_ext_t * sep = P_create_state_ext( fsp );  // default args to 0
              // [WDJ] The MBF21 flags are 32bit in the DEH file.
              // DSDA-Doom converts them to internal format in DEH post-processing, which is hard to follow.
              // Because cannot determine the function yet, must use DEH_Finalize to convert
              // them to DoomLegacy flags.  That will split the MBF21 flags into two registers.
              sep->parm_args[ai] = value; // 32 bit flags
              sep->args_set |= 1<<ai;  // set by DEH, to prevent defaulting
          }
          else
          {
     argerr:
              deh_error("Frame %d : Bad Args '%s'\n", deh_frame_id, word1);
          }
      }
      else if(!strcasecmp(word1,"MBF21"))
      {
          if(!strcasecmp(word2,"bits"))
          {
              // [WDJ] Could not make this a function because everything is slightly different.
              uint16_t frflags = 0;
              for(;;)
              {
                  char * word3 = strtok(NULL, " +|\t=\n");
                  if( word3 == NULL )  goto frame_flags_set;
                  if( word3[0] == '\n' )  goto frame_flags_set;
                  // detect bits by integer value
                  // Prevent using signed char as index.
                  if( isdigit( (unsigned char)word3[0] ) || word3[0] == '-' )
                  {
                      uint32_t mbf21_v32 = str_to_uint32(word3);
                      // Handle MBF21 bits numeric assign.
                      // This is according to chart in MBF21 developer_spec.md.
                      if( mbf21_v32 & 0x0001 )  // SKILL5FAST
                        frflags |= FRF_SKILL5_FAST;
                      goto frame_flags_set;
                  }
                  // handle frame flag names
                  flag_name_t * fnp; // flag name table ptr
                  for( fnp = &frame_flag_name_table[0]; ; fnp++ )
                  {
                      if(fnp->name == NULL)  goto frame_flags_name_unknown;
                      if(!strcasecmp( word3, fnp->name ))  // find name
                      {
                          switch( fnp->ctrl )
                          {
                           case BFR_M21:
                              frflags |= fnp->flagval;
                              goto frame_flags_next_word;
                           default:
                              goto frame_flags_name_unknown;
                          }
                          goto frame_flags_next_word; // done with this word
                      }
                  }
          frame_flags_name_unknown:
                  deh_error("Frame Bits name unknown: %s\n", word3);
                  // continue with next keyword
          frame_flags_next_word:
                  continue;
              }

          frame_flags_set:
              // clear MBF21 frame flags and set those in frflags
              P_set_frame_flags( fsp, (FRF_SKILL5_FAST | FRF_SKILL5_MOD_APPLIED), frflags );
              deh_key_detect |= BDTC_MBF21;
              continue; // next line
          }
      }
#endif
      // [WDJ] action function, DoomLegacy extension
      else if(!strcasecmp(word1,"action"))
      {
          // syntax:  action = functionname
          char * funcname = strtok(NULL, " ");
          if( !funcname || strlen(funcname) < 2 )
          {
              deh_error( "Bad ACTION syntax\n" );
              continue;
          }
          action_function_store( deh_frame_id, si, funcname );
      }
      else deh_error("Frame %d : unknown word '%s'\n", deh_frame_id, word1);
    }
  } while(s[0]!='\n' && !myfeof(f));

  if( fsp->nextstate == 0 )
  {
      // some dehacked explicitly set nextstate to 0, it is STOP.
      if( verbose > 1 )
          GenPrintf(EMSG_info, "Frame %d: sets nextstate = 0\n", deh_frame_id );
  }
}


// Have read  "Pointer <xref>"
// The xref is a dehacked cross ref number.
static
void  read_pointer( myfile_t* f, int xref )
{
  char s[MAXLINELEN];
  int  i;

  // Syntax: "Pointer <xref> (Frame  <deh_frame_id>)"
  char * word1 = strtok(NULL," "); // get keyword "Frame"
  char * word2 = strtok(NULL,")");
  if( !word1 || word1[0]=='\n' || ! word2 || word2[0]=='\n' )
  {
    deh_error("Pointer %i (Frame ... ) : missing ')'\n", xref );
    return;
  }
   
  i = atoi(word2);
  statenum_t si = DEH_frame_to_state(i);
  if( si == S_NULL )
  {
    deh_error("Pointer %i : Frame %d don't exist\n", xref, i);
    return;
  }

  // [WDJ] For some reason PrBoom and EE do this in a loop,
  // and then print out a BEX equivalent line.
  // Syntax: "Codep Frame <deh_frame_id>"
  if( myfgets(s,sizeof(s),f) != NULL )
  {
    int value = searchvalue(s);
    if( ! confirm_searchvalue(value)  )
        return;

    statenum_t sj = DEH_frame_to_state( value );
    if( (sj != S_NULL) && (sj < NUMSTATES_DEF) )
        states[si].action = deh_actions[sj];
  }
}


static
void read_sound( myfile_t* f, int deh_sound_id )
{
  sfxinfo_t *  ssp = & S_sfx[ deh_sound_id ];
  char s[MAXLINELEN];
  char *word;
  int value;

  do{
    if(myfgets_nocom(s,sizeof(s),f)!=NULL)
    {
      if(s[0]=='\n') break;
      value=searchvalue(s);
      word=strtok(s," ");
      if( !word || word[0]=='\n' )  break;
      if( ! confirm_searchvalue(value)  )  continue;

      if(!strcasecmp(word,"Offset"))
      {
          value-=150360;
          if(value<=64)
              value/=8;
          else if(value<=260)
              value=(value+4)/8;
          else
              value=(value+8)/8;
          if(value>=-1 && value<(NUMSFX_DEF-1))
              ssp->name=deh_sfxnames[value+1];
          else
              deh_error("Sound %d : offset out of bound\n", deh_sound_id);
      }
      else if(!strcasecmp(word,"Zero/One"))
          ssp->flags = ( value? SFX_single : 0 );
      else if(!strcasecmp(word,"Value"))
          ssp->priority = value;
      else deh_error("Sound %d : unknown word '%s'\n", deh_sound_id,word);
    }
  } while(s[0]!='\n' && !myfeof(f));
}

// [WDJ] Some strings have been altered, preventing match to text in DEH file.
// This is hash of the original Doom strings.
typedef struct {
    uint32_t  hash;      // computed hash
    uint16_t  indirect;  // id of actual text
} PACKED_ATTR  hash_text_t;

// [WDJ] This is constructed from wads where DEH fails.
// To get hash print use -devparm -v.
// > doomlegacy -devparm -v -game doom2 -file xxx.wad 2> xxx.log
static hash_text_t   hash_text_table[] =
{
    {0x26323511, QUITMSG3_NUM},  // dos -> your os
    {0x33033301, QUITMSG4_NUM},  // dos -> your os
    {0x8e3b425e, QUIT2MSG1_NUM}, // dos -> shell
    {0x0042d2cd, DEATHMSG_TELEFRAG}, // telefraged -> telefragged
    {0x106f76c2, DEATHMSG_ROCKET}, // catched -> caught
  // because Chex1PatchEngine changes text before newmaps DEH makes its changes
    {0xea264ed7, E1TEXT_NUM},
    {0xcee03ff5, QUITMSG_NUM},
    {0x55b48886, QUITMSG1_NUM},
    {0xe980b2e0, QUITMSG2_NUM},
    {0x26323511, QUITMSG3_NUM},
    {0x33033301, QUITMSG4_NUM},
    {0x84311a52, QUITMSG5_NUM},
    {0xb6d256a1, QUITMSG6_NUM},
    {0x303ea545, QUITMSG7_NUM},
    {0x8e3b425e, QUIT2MSG1_NUM},
    {0x507e7be5, QUIT2MSG2_NUM},
    {0xc5e635e9, QUIT2MSG3_NUM},
    {0x70136b3e, QUIT2MSG4_NUM},
    {0xe5efb6a8, QUIT2MSG5_NUM},
    {0x04fc8b21, QUIT2MSG6_NUM},
    {0x68d8f2ce, NIGHTMARE_NUM},
    {0x001bcf94, GOTARMOR_NUM},
    {0x01bcfc54, GOTMEGA_NUM},
    {0x01bc5ffd, GOTHTHBONUS_NUM},
    {0x01bc817d, GOTARMBONUS_NUM},
    {0x00010e33, GOTSUPER_NUM},
    {0x01bc64a4, GOTBLUECARD_NUM},
    {0x06f2bfa4, GOTYELWCARD_NUM},
    {0x00de4424, GOTREDCARD_NUM},
    {0x003795f5, GOTSTIM_NUM},
    {0x8e914c80, GOTMEDINEED_NUM},
    {0x001bc72a, GOTMEDIKIT_NUM},
    {0x00000b0b, GOTBERSERK_NUM},
    {0x00e1e245, GOTINVIS_NUM},
    {0x06b5b172, GOTSUIT_NUM},
    {0x0d3bf614, GOTVISOR_NUM},
    {0x000378aa, GOTCLIP_NUM},
    {0x0378e27f, GOTCLIPBOX_NUM},
    {0x000de4c2, GOTROCKET_NUM},
    {0x0378e527, GOTROCKBOX_NUM},
    {0x01bc81b0, GOTCELL_NUM},
    {0x1bc81c85, GOTCELLBOX_NUM},
    {0x06f0ec53, GOTSHELLS_NUM},
    {0xde396c53, GOTSHELLBOX_NUM},
    {0xde1fc095, GOTBACKPACK_NUM},
    {0x048972a1, GOTBFG9000_NUM},
    {0x004899e4, GOTCHAINGUN_NUM},
    {0x0260d3b2, GOTCHAINSAW_NUM},
    {0x1228dc04, GOTLAUNCHER_NUM},
    {0x009143bc, GOTPLASMA_NUM},
    {0x00245214, GOTSHOTGUN_NUM},
    {0x015aa620, STSTR_DQDON_NUM},
    {0x02b54c46, STSTR_DQDOFF_NUM},
    {0x000b945e, STSTR_FAADDED_NUM},
    {0x00824ede, STSTR_KFAADDED_NUM},
    {0x000188ff, STSTR_CHOPPERS_NUM},
    {0x00007ba0, HUSTR_E1M1_NUM},
    {0x001f7c84, HUSTR_E1M2_NUM},
    {0x003fc441, HUSTR_E1M3_NUM},
    {0x007d8282, HUSTR_E1M4_NUM},
    {0x0003f9ac, HUSTR_E1M5_NUM},
//{0x0000199f, ??}, // "GREEN KEY"
    {0x00004375, DEATHMSG_SUICIDE},
    {0x0042d2cd, DEATHMSG_TELEFRAG},
    {0x020d3f9d, DEATHMSG_FIST},
    {0x0004296d, DEATHMSG_GUN},
    {0x0010c1fd, DEATHMSG_SHOTGUN},
    {0x0211416d, DEATHMSG_MACHGUN},
    {0x106f76c2, DEATHMSG_ROCKET},
    {0x020f86c2, DEATHMSG_GIBROCKET},
    {0x00075264, DEATHMSG_PLASMA},
    {0x07904664, DEATHMSG_BFGBALL},
    {0xe3a3462f, DEATHMSG_CHAINSAW},
    {0x000426ad, DEATHMSG_PLAYUNKNOW},
    {0x001cd8c3, DEATHMSG_HELLSLIME},
    {0x01f47e6f, DEATHMSG_NUKE},
    {0x75e39428, DEATHMSG_SUPHELLSLIME},
    {0x01ce59b8, DEATHMSG_SPECUNKNOW},
    {0x020e19cd, DEATHMSG_BARRELFRAG},
    {0x39b61448, DEATHMSG_BARREL},
    {0x021835d2, DEATHMSG_POSSESSED},
    {0x0861051f, DEATHMSG_SHOTGUY},
    {0x0021255e, DEATHMSG_TROOP},
    {0x109d6e18, DEATHMSG_SERGEANT},
    {0x10b701a8, DEATHMSG_BRUISER},
    {0x00000372, DEATHMSG_DEAD},
    {0, 0xFFFF}  // last has invalid indirect
};

static
void read_text( myfile_t* f, int len1, int len2 )
{
  char s[2001];
  char * str2;
  int i;

  // it is hard to change all the text in doom
  // here I implement only vital things
  // yes, Text can change some tables like music, sound and sprite name
  
  if(len1+len2 > 2000)
  {
      deh_error("Text too big\n");
      return;
  }

  if( myfread(s, len1+len2, f) )
  {
    if( bex_include_notext )
       return;  // BEX INCLUDE NOTEXT is active, blocks Text replacements

#if 0
    // Debugging trigger
    if( strncmp(s,"GREATER RUNES", 13) == 0 )
    {
        GenPrintf(EMSG_errlog, "Text:%s\n", s);
    }
#endif

    str2 = &s[len1];
    s[len1+len2]='\0';

    if( devparm > 1 )
    {
        // Readable listing of DEH text replacement.
        // Make copy of str1 so can terminate string.
        int len3 = (len1<999)?len1:999;
        char str3[1000];
        strncpy( str3, s, len3);
        str3[len3] = 0;
        GenPrintf(EMSG_errlog, "FROM:%s\nTO:%s\n", str3, str2);
    }

    if((len1 == 4) && (len2 == 4))  // sprite names are always 4 chars
    {
      // sprite table
      for(i=0;i < NUMSPRITES_DEF;i++)
      {
        if(!strncmp(deh_sprnames[i],s,len1))
        {
          // May be const string, which will segfault on write
          deh_replace_string( &sprnames[i], str2, DRS_name );
          return;
        }
      }
    }
    if((len1 <= 6) && (len2 <= 6))  // sound effect names limited to 6 chars
    {
      // these names are strings, so compare them correctly
      char str1[8];  // make long enough for 0 term too
      strncpy( str1, s, len1 ); // copy name to proper string
      str1[len1] = '\0';
      // sound table, before DEHEXTRA
      for(i=0;i<NUMSFX_DEF;i++)
      {
        const char * dsfx = deh_sfxnames[i];
        if( dsfx && !strcmp(dsfx,str1))
        {
          // sfx name may be Z_Malloc(7) or a const string
          // May be const string, which will segfault on write
          deh_replace_string( &S_sfx[i].name, str2, DRS_name );
          return;
        }
      }
      // music names limited to 6 chars
      // music table
      for(i=1;i<NUMMUSIC_DEF;i++)
      {
        const char * dmus = deh_musicname[i];
        if( dmus && (!strcmp(dmus, str1)) )
        {
          // May be const string, which will segfault on write
          deh_replace_string( &S_music[i].name, str2, DRS_name );
          return;
        }
      }
    }
    // Limited by buffer size.
    // text table
    for(i=0;i<SPECIALDEHACKED;i++)
    {
      if(!strncmp(deh_text[i],s,len1) && strlen(deh_text[i])==(unsigned)len1)
      {
        // May be const string, which will segfault on write
        deh_replace_string( &text[i], str2, DRS_string );
        return;
      }
    }

    // special text : text changed in Legacy but with dehacked support
    for(i=SPECIALDEHACKED;i<NUMTEXT;i++)
    {
       int ltxt = strlen(deh_text[i]);

       if(len1>ltxt && strstr(s,deh_text[i]))
       {
           // found text to be replaced
           char *t;

           // remove space for center the text
           t=&s[len1+len2-1];
           while(t[0]==' ') { t[0]='\0'; t--; }
           // skip the space
           while(s[len1]==' ') len1++;

           // remove version string identifier
           t=strstr(&(s[len1]),"v%i.%i");
           if(!t) {
              t=strstr(&(s[len1]),"%i.%i");
              if(!t) {
                 t=strstr(&(s[len1]),"%i");
                 if(!t) {
                      t=&s[len1]+strlen(&(s[len1]));
                 }
              }
           }
           t[0]='\0';
           // May be const string, which will segfault on write
           deh_replace_string( &text[i], &(s[len1]), DRS_string );
           return;
       }
    }
    
    // [WDJ] Lookup by hash
    {
       uint32_t hash = 0;
       for(i=0; i<len1; i++)
       {
           if( s[i] >= '0' )
           {
               if( hash&0x80000000 )
                  hash ++;
               hash <<= 1;
               // Prevent using signed char as index.
               hash += toupper( (unsigned char)s[i] ) - '0';
           }
       }
       for(i=0;;i++)
       {
           if( hash_text_table[i].indirect >= NUMTEXT ) break;  // not found
           if( hash_text_table[i].hash == hash )
           {
               deh_replace_string( &text[hash_text_table[i].indirect], &(s[len1]), DRS_string );
               return;
           }
       }
       if( devparm > 1 )
           GenPrintf(EMSG_errlog, "Text hash= 0x%08x :", hash);
    }

    s[len1]='\0';
    deh_error("Text not changed :%s\n",s);
  }
}

// [WDJ] 8/27/2011 BEX text strings
typedef struct {
    char *    kstr;
    uint16_t  text_num;
} bex_text_t;

// must count entries in bex_string_table
uint16_t  bex_string_start_table[3] = { 46+15, 46, 0 };  // start=

// BEX entries from boom202s/boomdeh.txt
bex_text_t  bex_string_table[] =
{
// start=3 language changes
// BEX that language may change, but PWAD should not change
   { "D_DEVSTR", D_DEVSTR_NUM },  // dev mode
   { "D_CDROM", D_CDROM_NUM },  // cdrom version
   { "LOADNET", LOADNET_NUM }, // only server can load netgame
   { "QLOADNET", QLOADNET_NUM }, // cant quickload
   { "QSAVESPOT", QSAVESPOT_NUM },  // no quicksave slot
   { "SAVEDEAD", SAVEDEAD_NUM },  // cannot save when not playing
   { "QSPROMPT", QSPROMPT_NUM }, // quicksave, has %s
   { "QLPROMPT", QLPROMPT_NUM }, // quickload, has %s
   { "NEWGAME", NEWGAME_NUM }, // cant start newgame
   { "SWSTRING", SWSTRING_NUM }, // shareware version
   { "MSGOFF", MSGOFF_NUM },
   { "MSGON", MSGON_NUM },
   { "NETEND", NETEND_NUM }, // cant end netgame
   { "ENDGAME", ENDGAME_NUM }, // want to end game ?
   { "DOSY", DOSY_NUM },  // quit to DOS, has %s
   { "EMPTYSTRING", EMPTYSTRING_NUM }, // savegame empty slot
   { "GGSAVED", GGSAVED_NUM }, // game saved
   { "HUSTR_MSGU", HUSTR_MSGU_NUM }, // message not sent
   { "HUSTR_MESSAGESENT", HUSTR_MESSAGESENT_NUM }, // message sent
   { "AMSTR_FOLLOWON", AMSTR_FOLLOWON_NUM },  // Automap follow
   { "AMSTR_FOLLOWOFF", AMSTR_FOLLOWOFF_NUM },
   { "AMSTR_GRIDON", AMSTR_GRIDON_NUM },  // Automap grid
   { "AMSTR_GRIDOFF", AMSTR_GRIDOFF_NUM },
   { "AMSTR_MARKEDSPOT", AMSTR_MARKEDSPOT_NUM },  // Automap marks
   { "AMSTR_MARKSCLEARED", AMSTR_MARKSCLEARED_NUM },
   { "STSTR_MUS", STSTR_MUS_NUM },  // Music
   { "STSTR_NOMUS", STSTR_NOMUS_NUM },
   { "STSTR_NCON", STSTR_NCON_NUM },  // No Clip
   { "STSTR_NCOFF", STSTR_NCOFF_NUM },
   { "STSTR_CLEV", STSTR_CLEV_NUM },  // change level
// BEX not used in DoomLegacy, but have strings
   { "DETAILHI", DETAILHI_NUM },
   { "DETAILLO", DETAILLO_NUM },
   { "GAMMALVL0", GAMMALVL0_NUM },
   { "GAMMALVL1", GAMMALVL1_NUM },
   { "GAMMALVL2", GAMMALVL2_NUM },
   { "GAMMALVL3", GAMMALVL3_NUM },
   { "GAMMALVL4", GAMMALVL4_NUM },
// BEX not used in DoomLegacy, but have strings, was only used in define of other strings
   { "PRESSKEY", PRESSKEY_NUM },
   { "PRESSYN", PRESSYN_NUM },
// BEX not present in DoomLegacy
   { "RESTARTLEVEL", 9999 },
   { "HUSTR_PLRGREEN", 9999 },
   { "HUSTR_PLRINDIGO", 9999 },
   { "HUSTR_PLRBROWN", 9999 },
   { "HUSTR_PLRRED", 9999 },
   { "STSTR_COMPON", 9999 }, // Doom compatibility mode
   { "STSTR_COMPOFF",9999 },
   
// start=2 personal changes
   { "HUSTR_CHATMACRO0", HUSTR_CHATMACRO0_NUM },
   { "HUSTR_CHATMACRO1", HUSTR_CHATMACRO1_NUM },
   { "HUSTR_CHATMACRO2", HUSTR_CHATMACRO2_NUM },
   { "HUSTR_CHATMACRO3", HUSTR_CHATMACRO3_NUM },
   { "HUSTR_CHATMACRO4", HUSTR_CHATMACRO4_NUM },
   { "HUSTR_CHATMACRO5", HUSTR_CHATMACRO5_NUM },
   { "HUSTR_CHATMACRO6", HUSTR_CHATMACRO6_NUM },
   { "HUSTR_CHATMACRO7", HUSTR_CHATMACRO7_NUM },
   { "HUSTR_CHATMACRO8", HUSTR_CHATMACRO8_NUM },
   { "HUSTR_CHATMACRO9", HUSTR_CHATMACRO9_NUM },
   { "HUSTR_TALKTOSELF1", HUSTR_TALKTOSELF1_NUM },
   { "HUSTR_TALKTOSELF2", HUSTR_TALKTOSELF2_NUM },
   { "HUSTR_TALKTOSELF3", HUSTR_TALKTOSELF3_NUM },
   { "HUSTR_TALKTOSELF4", HUSTR_TALKTOSELF4_NUM },
   { "HUSTR_TALKTOSELF5", HUSTR_TALKTOSELF5_NUM },

// start=0 normal game changes
   { "QUITMSG",  QUITMSG_NUM },
   { "NIGHTMARE",  NIGHTMARE_NUM },
   { "GOTARMOR",  GOTARMOR_NUM },
   { "GOTMEGA",  GOTMEGA_NUM },
   { "GOTHTHBONUS",  GOTHTHBONUS_NUM },
   { "GOTARMBONUS",  GOTARMBONUS_NUM },
   { "GOTSTIM", GOTSTIM_NUM },
   { "GOTMEDINEED", GOTMEDINEED_NUM },
   { "GOTMEDIKIT", GOTMEDIKIT_NUM },
   { "GOTSUPER", GOTSUPER_NUM },
   { "GOTBLUECARD", GOTBLUECARD_NUM },
   { "GOTYELWCARD", GOTYELWCARD_NUM },
   { "GOTREDCARD", GOTREDCARD_NUM },
   { "GOTBLUESKUL", GOTBLUESKUL_NUM },
   { "GOTYELWSKUL", GOTYELWSKUL_NUM },
   { "GOTREDSKULL", GOTREDSKULL_NUM },
   { "GOTINVUL", GOTINVUL_NUM },
   { "GOTBERSERK", GOTBERSERK_NUM },
   { "GOTINVIS", GOTINVIS_NUM },
   { "GOTSUIT", GOTSUIT_NUM },
   { "GOTMAP", GOTMAP_NUM },
   { "GOTVISOR", GOTVISOR_NUM },
   { "GOTMSPHERE", GOTMSPHERE_NUM },
   { "GOTCLIP", GOTCLIP_NUM },
   { "GOTCLIPBOX", GOTCLIPBOX_NUM },
   { "GOTROCKET", GOTROCKET_NUM },
   { "GOTROCKBOX", GOTROCKBOX_NUM },
   { "GOTCELL", GOTCELL_NUM },
   { "GOTCELLBOX", GOTCELLBOX_NUM },
   { "GOTSHELLS", GOTSHELLS_NUM },
   { "GOTSHELLBOX", GOTSHELLBOX_NUM },
   { "GOTBACKPACK", GOTBACKPACK_NUM },
   { "GOTBFG9000", GOTBFG9000_NUM },
   { "GOTCHAINGUN", GOTCHAINGUN_NUM },
   { "GOTCHAINSAW", GOTCHAINSAW_NUM },
   { "GOTLAUNCHER", GOTLAUNCHER_NUM },
   { "GOTPLASMA", GOTPLASMA_NUM },
   { "GOTSHOTGUN", GOTSHOTGUN_NUM },
   { "GOTSHOTGUN2", GOTSHOTGUN2_NUM },
   { "PD_BLUEO", PD_BLUEO_NUM },
   { "PD_REDO", PD_REDO_NUM },
   { "PD_YELLOWO", PD_YELLOWO_NUM },
   { "PD_BLUEK", PD_BLUEK_NUM },
   { "PD_REDK", PD_REDK_NUM },
   { "PD_YELLOWK", PD_YELLOWK_NUM },
   { "PD_BLUEC", PD_BLUEC_NUM },
   { "PD_REDC", PD_REDC_NUM },
   { "PD_YELLOWC", PD_YELLOWC_NUM },
   { "PD_BLUES", PD_BLUES_NUM },
   { "PD_REDS", PD_REDS_NUM },
   { "PD_YELLOWS", PD_YELLOWS_NUM },
   { "PD_ANY", PD_ANY_NUM },
   { "PD_ALL3", PD_ALL3_NUM },
   { "PD_ALL6", PD_ALL6_NUM },
   { "HUSTR_MSGU", HUSTR_MSGU_NUM },
   { "HUSTR_E1M1", HUSTR_E1M1_NUM },
   { "HUSTR_E1M2", HUSTR_E1M2_NUM },
   { "HUSTR_E1M3", HUSTR_E1M3_NUM },
   { "HUSTR_E1M4", HUSTR_E1M4_NUM },
   { "HUSTR_E1M5", HUSTR_E1M5_NUM },
   { "HUSTR_E1M6", HUSTR_E1M6_NUM },
   { "HUSTR_E1M7", HUSTR_E1M7_NUM },
   { "HUSTR_E1M8", HUSTR_E1M8_NUM },
   { "HUSTR_E1M9", HUSTR_E1M9_NUM },
   { "HUSTR_E2M1", HUSTR_E2M1_NUM },
   { "HUSTR_E2M2", HUSTR_E2M2_NUM },
   { "HUSTR_E2M3", HUSTR_E2M3_NUM },
   { "HUSTR_E2M4", HUSTR_E2M4_NUM },
   { "HUSTR_E2M5", HUSTR_E2M5_NUM },
   { "HUSTR_E2M6", HUSTR_E2M6_NUM },
   { "HUSTR_E2M7", HUSTR_E2M7_NUM },
   { "HUSTR_E2M8", HUSTR_E2M8_NUM },
   { "HUSTR_E2M9", HUSTR_E2M9_NUM },
   { "HUSTR_E3M1", HUSTR_E3M1_NUM },
   { "HUSTR_E3M2", HUSTR_E3M2_NUM },
   { "HUSTR_E3M3", HUSTR_E3M3_NUM },
   { "HUSTR_E3M4", HUSTR_E3M4_NUM },
   { "HUSTR_E3M5", HUSTR_E3M5_NUM },
   { "HUSTR_E3M6", HUSTR_E3M6_NUM },
   { "HUSTR_E3M7", HUSTR_E3M7_NUM },
   { "HUSTR_E3M8", HUSTR_E3M8_NUM },
   { "HUSTR_E3M9", HUSTR_E3M9_NUM },
   { "HUSTR_E4M1", HUSTR_E4M1_NUM },
   { "HUSTR_E4M2", HUSTR_E4M2_NUM },
   { "HUSTR_E4M3", HUSTR_E4M3_NUM },
   { "HUSTR_E4M4", HUSTR_E4M4_NUM },
   { "HUSTR_E4M5", HUSTR_E4M5_NUM },
   { "HUSTR_E4M6", HUSTR_E4M6_NUM },
   { "HUSTR_E4M7", HUSTR_E4M7_NUM },
   { "HUSTR_E4M8", HUSTR_E4M8_NUM },
   { "HUSTR_E4M9", HUSTR_E4M9_NUM },
   { "HUSTR_1", HUSTR_1_NUM },
   { "HUSTR_2", HUSTR_2_NUM },
   { "HUSTR_3", HUSTR_3_NUM },
   { "HUSTR_4", HUSTR_4_NUM },
   { "HUSTR_5", HUSTR_5_NUM },
   { "HUSTR_6", HUSTR_6_NUM },
   { "HUSTR_7", HUSTR_7_NUM },
   { "HUSTR_8", HUSTR_8_NUM },
   { "HUSTR_9", HUSTR_9_NUM },
   { "HUSTR_10", HUSTR_10_NUM },
   { "HUSTR_11", HUSTR_11_NUM },
   { "HUSTR_12", HUSTR_12_NUM },
   { "HUSTR_13", HUSTR_13_NUM },
   { "HUSTR_14", HUSTR_14_NUM },
   { "HUSTR_15", HUSTR_15_NUM },
   { "HUSTR_16", HUSTR_16_NUM },
   { "HUSTR_17", HUSTR_17_NUM },
   { "HUSTR_18", HUSTR_18_NUM },
   { "HUSTR_19", HUSTR_19_NUM },
   { "HUSTR_20", HUSTR_20_NUM },
   { "HUSTR_21", HUSTR_21_NUM },
   { "HUSTR_22", HUSTR_22_NUM },
   { "HUSTR_23", HUSTR_23_NUM },
   { "HUSTR_24", HUSTR_24_NUM },
   { "HUSTR_25", HUSTR_25_NUM },
   { "HUSTR_26", HUSTR_26_NUM },
   { "HUSTR_27", HUSTR_27_NUM },
   { "HUSTR_28", HUSTR_28_NUM },
   { "HUSTR_29", HUSTR_29_NUM },
   { "HUSTR_30", HUSTR_30_NUM },
   { "HUSTR_31", HUSTR_31_NUM },
   { "HUSTR_32", HUSTR_32_NUM },
   { "PHUSTR_1", PHUSTR_1_NUM },
   { "PHUSTR_2", PHUSTR_2_NUM },
   { "PHUSTR_3", PHUSTR_3_NUM },
   { "PHUSTR_4", PHUSTR_4_NUM },
   { "PHUSTR_5", PHUSTR_5_NUM },
   { "PHUSTR_6", PHUSTR_6_NUM },
   { "PHUSTR_7", PHUSTR_7_NUM },
   { "PHUSTR_8", PHUSTR_8_NUM },
   { "PHUSTR_9", PHUSTR_9_NUM },
   { "PHUSTR_10", PHUSTR_10_NUM },
   { "PHUSTR_11", PHUSTR_11_NUM },
   { "PHUSTR_12", PHUSTR_12_NUM },
   { "PHUSTR_13", PHUSTR_13_NUM },
   { "PHUSTR_14", PHUSTR_14_NUM },
   { "PHUSTR_15", PHUSTR_15_NUM },
   { "PHUSTR_16", PHUSTR_16_NUM },
   { "PHUSTR_17", PHUSTR_17_NUM },
   { "PHUSTR_18", PHUSTR_18_NUM },
   { "PHUSTR_19", PHUSTR_19_NUM },
   { "PHUSTR_20", PHUSTR_20_NUM },
   { "PHUSTR_21", PHUSTR_21_NUM },
   { "PHUSTR_22", PHUSTR_22_NUM },
   { "PHUSTR_23", PHUSTR_23_NUM },
   { "PHUSTR_24", PHUSTR_24_NUM },
   { "PHUSTR_25", PHUSTR_25_NUM },
   { "PHUSTR_26", PHUSTR_26_NUM },
   { "PHUSTR_27", PHUSTR_27_NUM },
   { "PHUSTR_28", PHUSTR_28_NUM },
   { "PHUSTR_29", PHUSTR_29_NUM },
   { "PHUSTR_30", PHUSTR_30_NUM },
   { "PHUSTR_31", PHUSTR_31_NUM },
   { "PHUSTR_32", PHUSTR_32_NUM },
   { "THUSTR_1", THUSTR_1_NUM },
   { "THUSTR_2", THUSTR_2_NUM },
   { "THUSTR_3", THUSTR_3_NUM },
   { "THUSTR_4", THUSTR_4_NUM },
   { "THUSTR_5", THUSTR_5_NUM },
   { "THUSTR_6", THUSTR_6_NUM },
   { "THUSTR_7", THUSTR_7_NUM },
   { "THUSTR_8", THUSTR_8_NUM },
   { "THUSTR_9", THUSTR_9_NUM },
   { "THUSTR_10", THUSTR_10_NUM },
   { "THUSTR_11", THUSTR_11_NUM },
   { "THUSTR_12", THUSTR_12_NUM },
   { "THUSTR_13", THUSTR_13_NUM },
   { "THUSTR_14", THUSTR_14_NUM },
   { "THUSTR_15", THUSTR_15_NUM },
   { "THUSTR_16", THUSTR_16_NUM },
   { "THUSTR_17", THUSTR_17_NUM },
   { "THUSTR_18", THUSTR_18_NUM },
   { "THUSTR_19", THUSTR_19_NUM },
   { "THUSTR_20", THUSTR_20_NUM },
   { "THUSTR_21", THUSTR_21_NUM },
   { "THUSTR_22", THUSTR_22_NUM },
   { "THUSTR_23", THUSTR_23_NUM },
   { "THUSTR_24", THUSTR_24_NUM },
   { "THUSTR_25", THUSTR_25_NUM },
   { "THUSTR_26", THUSTR_26_NUM },
   { "THUSTR_27", THUSTR_27_NUM },
   { "THUSTR_28", THUSTR_28_NUM },
   { "THUSTR_29", THUSTR_29_NUM },
   { "THUSTR_30", THUSTR_30_NUM },
   { "THUSTR_31", THUSTR_31_NUM },
   { "THUSTR_32", THUSTR_32_NUM },

   { "E1TEXT", E1TEXT_NUM },
   { "E2TEXT", E2TEXT_NUM },
   { "E3TEXT", E3TEXT_NUM },
   { "E4TEXT", E4TEXT_NUM },
   { "C1TEXT", C1TEXT_NUM },
   { "C2TEXT", C2TEXT_NUM },
   { "C3TEXT", C3TEXT_NUM },
   { "C4TEXT", C4TEXT_NUM },
   { "C5TEXT", C5TEXT_NUM },
   { "C6TEXT", C6TEXT_NUM },
   { "P1TEXT", P1TEXT_NUM },
   { "P2TEXT", P2TEXT_NUM },
   { "P3TEXT", P3TEXT_NUM },
   { "P4TEXT", P4TEXT_NUM },
   { "P5TEXT", P5TEXT_NUM },
   { "P6TEXT", P6TEXT_NUM },
   { "T1TEXT", T1TEXT_NUM },
   { "T2TEXT", T2TEXT_NUM },
   { "T3TEXT", T3TEXT_NUM },
   { "T4TEXT", T4TEXT_NUM },
   { "T5TEXT", T5TEXT_NUM },
   { "T6TEXT", T6TEXT_NUM },
   { "STSTR_DQDON", STSTR_DQDON_NUM },  // Invincible
   { "STSTR_DQDOFF", STSTR_DQDOFF_NUM },
   { "STSTR_FAADDED", STSTR_FAADDED_NUM },  // Full Ammo
   { "STSTR_KFAADDED", STSTR_KFAADDED_NUM },  // Full Ammo Keys
   { "STSTR_BEHOLD", STSTR_BEHOLD_NUM },  // Power-up
   { "STSTR_BEHOLDX", STSTR_BEHOLDX_NUM }, // Power-up toggle
   { "STSTR_CHOPPERS", STSTR_CHOPPERS_NUM },  // Chainsaw

   { "CC_ZOMBIE", CC_ZOMBIE_NUM },
   { "CC_SHOTGUN", CC_SHOTGUN_NUM },
   { "CC_HEAVY", CC_HEAVY_NUM },
   { "CC_IMP", CC_IMP_NUM },
   { "CC_DEMON", CC_DEMON_NUM },
   { "CC_LOST", CC_LOST_NUM },
   { "CC_CACO", CC_CACO_NUM },
   { "CC_HELL", CC_HELL_NUM },
   { "CC_BARON", CC_BARON_NUM },
   { "CC_ARACH", CC_ARACH_NUM },
   { "CC_PAIN", CC_PAIN_NUM },
   { "CC_REVEN", CC_REVEN_NUM },
   { "CC_MANCU", CC_MANCU_NUM },
   { "CC_ARCH", CC_ARCH_NUM },
   { "CC_SPIDER", CC_SPIDER_NUM },
   { "CC_CYBER", CC_CYBER_NUM },
   { "CC_HERO", CC_HERO_NUM },

   { "BGFLATE1", BGFLATE1_NUM },
   { "BGFLATE2", BGFLATE2_NUM },
   { "BGFLATE3", BGFLATE3_NUM },
   { "BGFLATE4", BGFLATE4_NUM },
   { "BGFLAT06", BGFLAT06_NUM },
   { "BGFLAT11", BGFLAT11_NUM },
   { "BGFLAT20", BGFLAT20_NUM },
   { "BGFLAT30", BGFLAT30_NUM },
   { "BGFLAT15", BGFLAT15_NUM },
   { "BGFLAT31", BGFLAT31_NUM },
#ifdef BEX_SAVEGAMENAME     
   { "SAVEGAMENAME", SAVEGAMENAME_NUM },  // [WDJ] Added 9/5/2011
#else
   { "SAVEGAMENAME", 9998 },  // [WDJ] Do not allow, because of security risk
#endif

// BEX not present in DoomLegacy
   { "BGCASTCALL", 9998 },
   { "STARTUP1", 9998 },
   { "STARTUP2", 9998 },
   { "STARTUP3", 9998 },
   { "STARTUP4", 9998 },
   { "STARTUP5", 9998 },

   { NULL, 0 }  // table term
};


#define BEX_MAX_STRING_LEN   2000
#define BEX_KEYW_LEN  20

// BEX [STRINGS] section
// permission: 0=game, 1=adv, 2=language
static
void bex_strings( myfile_t* f, byte bex_permission )
{
  char stxt[BEX_MAX_STRING_LEN+1];
  char keyw[BEX_KEYW_LEN];
  char sb[MAXLINELEN];
  char * stp;
  char * word;
  char * cp;
  int perm_min = bex_string_start_table[bex_permission];
  int i;

  // string format, no quotes:
  // [STRINGS]
  // #comment, ** Maybe ** comment within replacement string
  // <keyw> = <text>
  // <keyw> = <text> backslash
  //   <text> backslash
  //   <text>

  for(;;) {
    if( ! myfgets_nocom(sb, sizeof(sb), f) )  // get line, skipping comments
       break; // no more lines
    if( sb[0] == 0 ) break;
    if( sb[0] == '\n' ) break;  // blank line  (PrBoom, Eternity)
    if( sb[0] == '#' ) continue;  // comment
    cp = strchr(sb,'=');  // find =
    word=strtok(sb," ");
    if( !word || word[0]=='\n')  break;
    strncpy( keyw, word, BEX_KEYW_LEN-1 );  // because continuation lines use sb
    keyw[BEX_KEYW_LEN-1] = '\0';
    // Limited by buffer size.
    if( cp == NULL ) goto no_text_change;
    cp++; // skip '='
    stxt[BEX_MAX_STRING_LEN] = '\0'; // protection
    stp = &stxt[0];
    // Get the new text
    do {
      while( *cp == ' ' || *cp == '\t' )  cp++; // skip leading space
      if( *cp == '\n' ) break;  // blank line
      if( *cp == 0 ) break;
      if( *cp == '\"' )  cp++;  // skip leading double quote
      while( *cp )
      {   // copy text upto CR
          if( *cp == '\n' ) break;
          *stp++ = *cp++;
          if( stp >= &stxt[BEX_MAX_STRING_LEN] ) break;
      }
      // remove trailing space
      while( stp > stxt && stp[-1] == ' ')
          stp --;
      // test for continuation line
      if( ! (stp > stxt && stp[-1] == '\\') )
          break;  // no backslash continuation
      stp--;  // remove backslash
      if( stp > stxt && stp[-1] == '\"' )
          stp --;  // remove trailing doublequote
      // get continuation line to sb, skipping comments.
      // [WDJ] questionable, but boom202 code skips comments between continuation lines.
      if( ! myfgets_nocom(sb, sizeof(sb), f) )
          break; // no more lines
      cp = &sb[0];
    } while ( *cp );
    if( stp > stxt && stp[-1] == '\"' )
        stp --;  // remove trailing doublequote
    *stp++ = '\0';  // term BEX replacement string in stxt
     
    // search text table for keyw
    for(i=0;  ;i++)
    {
        if( bex_string_table[i].kstr == NULL )  goto no_text_change;
        if(!strcasecmp(bex_string_table[i].kstr, keyw))  // BEX keyword search
        {
            int text_index = bex_string_table[i].text_num;
#ifdef BEX_SAVEGAMENAME
            // protect file names against attack
            if( i == SAVEGAMENAME_NUM )
            {
                if( filename_reject( stxt, 10 ) )  goto no_text_change;
            }
#endif
            if( i >= perm_min && text_index < NUMTEXT)
            {
                // May be const string, which will segfault on write
                deh_replace_string( &text[text_index], stxt, DRS_string );
            }
            else
            {
                // change blocked, but not an error
            }
            goto next_keyw;
        }
    }
  no_text_change:
    deh_error("Text not changed :%s\n", keyw);
     
  next_keyw:
    continue;
  }
}

// BEX [PARS] section
static
void bex_pars( myfile_t* f )
{
  char s[MAXLINELEN];
  int  episode, level, partime;
  int  nn;
   
  // format:
  // [PARS]
  // par <episode> <level> <seconds>
  // par <map_level> <seconds>

  for(;;) {
    if( ! myfgets_nocom(s, sizeof(s), f) )
       break; // no more lines
    if( s[0] == 0 ) break;
    if( s[0] == '\n' ) break;  // blank line  (PrBoom, Eternity)
    if( s[0] == '#' ) continue;  // comment
    if( strncasecmp( s, "par", 3 ) != 0 )  break;  // not a par line
    nn = sscanf( &s[3], " %i %i %i", &episode, &level, &partime );
    if( nn == 3 )
    { // Doom1 Episode, level, time format
      if( (episode < 1) || (episode > 3) || (level < 1) || (level > 9) )
        deh_error( "Bad par E%dM%d\n", episode, level );
      else
      {
        pars[episode][level] = partime;
        pars_valid_bex = true;
      }
    }
    else if( nn == 2 )
    { // Doom2 map, time format
      partime = level;
      level = episode;
      if( (level < 1) || (level > 32))
        deh_error( "Bad PAR MAP%d\n", level );
      else
      {
        cpars[level-1] = partime;
        pars_valid_bex = true;
      }
    }
    else
      deh_error( "Invalid par format\n" );
  }
}


// [WDJ] BEX codeptr strings and function
typedef struct {
    const char *    kstr;
    action_fi_t     action;  // action enum
    uint16_t        bdtc;    // standard tracking, impact tracking
} PACKED_ATTR  bex_codeptr_t;

// BEX entries from boom202s/boomdeh.txt
// acp2 functions first, then acp1 functions
bex_codeptr_t  bex_action_table[] = {
   {"NULL", AI_NULL, 0},  // to clear a ptr
// player weapon actions, acp2, (player *, pspdef_t *)
   {"WeaponReady", AP_WeaponReady, (BDTC_doom|BDTC_acp2) },  // [1]
   {"Lower", AP_Lower, (BDTC_doom|BDTC_acp2)},
   {"Raise", AP_Raise, (BDTC_doom|BDTC_acp2)},
   {"Punch", AP_Punch, (BDTC_doom|BDTC_acp2)},
   {"FirePistol", AP_FirePistol, (BDTC_doom|BDTC_acp2)},
   {"FireShotgun", AP_FireShotgun, (BDTC_doom|BDTC_acp2)},
   {"FireShotgun2", AP_FireShotgun2, (BDTC_doom|BDTC_acp2)},
   {"CheckReload", AP_CheckReload, (BDTC_doom|BDTC_acp2)},
   {"OpenShotgun2", AP_OpenShotgun2, (BDTC_doom|BDTC_acp2)},
   {"LoadShotgun2", AP_LoadShotgun2, (BDTC_doom|BDTC_acp2)},
   {"CloseShotgun2", AP_CloseShotgun2, (BDTC_doom|BDTC_acp2)},
   {"FireCGun", AP_FireCGun, (BDTC_doom|BDTC_acp2)},
   {"FireMissile", AP_FireMissile, (BDTC_doom|BDTC_acp2)},
   {"FirePlasma", AP_FirePlasma, (BDTC_doom|BDTC_acp2)},
   {"BFGsound", AP_BFGsound, (BDTC_doom|BDTC_acp2)},
   {"FireBFG", AP_FireBFG, (BDTC_doom|BDTC_acp2)},
   {"Saw", AP_Saw, (BDTC_doom|BDTC_acp2)},
   {"ReFire", AP_ReFire, (BDTC_doom|BDTC_acp2)},
   {"GunFlash", AP_GunFlash, (BDTC_doom|BDTC_acp2)},
   {"Light0", AP_Light0, (BDTC_doom|BDTC_acp2)},
   {"Light1", AP_Light1, (BDTC_doom|BDTC_acp2)},
   {"Light2", AP_Light2, (BDTC_doom|BDTC_acp2)},
   // MBF21
#ifdef MBF21
   // [WDJ] MBF21 player function ptrs, from MBF21 spec and DSDA-Doom.
   // DSDA-Doom is a poor source for MBF21, as it is mangled and obtuse.
   // I have kept some extra information from DSDA, until I am sure that MBF21
   // has been implemented (as well as we can).
   {"WeaponProjectile", AP_WeaponProjectile_MBF21, (BDTC_MBF21|BDTC_acp2)},  // Player projectile attack, args[0..4]
      // args[0] is thing_id
      // default {0}
   {"WeaponBulletAttack", AP_WeaponBulletAttack_MBF21, (BDTC_MBF21|BDTC_acp2)},  // Player bullet attack, args[0..4]
      // default {0, 0, 1, 5, 3 }
   {"WeaponMeleeAttack", AP_WeaponMeleeAttack_MBF21, (BDTC_MBF21|BDTC_acp2)},  // Player melee attack, args[0..4]
      // default {2, 10, 1*FRACUNIT, 0, 0 }
   {"WeaponSound", AP_WeaponSound_MBF21, (BDTC_MBF21|BDTC_acp2)},  // args[0..1]
   {"WeaponAlert", AP_WeaponAlert_MBF21, (BDTC_MBF21|BDTC_acp2)},  // args none
   {"WeaponJump", AP_WeaponJump_MBF21, (BDTC_MBF21|BDTC_acp2)},  // args[0..1]
   {"ConsumeAmmo", AP_ConsumeAmmo_MBF21, (BDTC_MBF21|BDTC_acp2)}, // args[0]
   {"CheckAmmo", AP_CheckAmmo_MBF21, (BDTC_MBF21|BDTC_acp2)}, // args[0..1]
   {"RefireTo", AP_RefireTo_MBF21, (BDTC_MBF21|BDTC_acp2)},  // args[0..1]
   {"GunFlashTo", AP_GunFlashTo_MBF21, (BDTC_MBF21|BDTC_acp2)},  // args[0..1]
#endif

// player actor actions, acp1, (mobj_t *)
   {"FaceTarget", AI_FaceTarget, (BDTC_doom)},
   {"Look", AI_Look, (BDTC_doom)},
   {"Chase", AI_Chase, (BDTC_doom)},
   {"Pain", AI_Pain, (BDTC_doom)},
   {"Fall", AI_Fall, (BDTC_doom)},
   {"XScream", AI_XScream, (BDTC_doom)},
   {"Scream", AI_Scream, (BDTC_doom)},
   {"PlayerScream", AI_PlayerScream, (BDTC_doom)},
   {"BFGSpray", AI_BFGSpray, (BDTC_doom)},
   {"Explode", AI_Explode, (BDTC_doom)},
   {"PosAttack", AI_PosAttack, (BDTC_doom)},
   {"SPosAttack", AI_SPosAttack, (BDTC_doom)},
   {"VileChase", AI_VileChase, (BDTC_doom)},
   {"VileStart", AI_VileStart, (BDTC_doom)},
   {"VileTarget", AI_VileTarget, (BDTC_doom)},
   {"VileAttack", AI_VileAttack, (BDTC_doom)},
   {"StartFire", AI_StartFire, (BDTC_doom)},
   {"Fire", AI_Fire, (BDTC_doom)},
   {"FireCrackle", AI_FireCrackle, (BDTC_doom)},
   {"Tracer", AI_Tracer, (BDTC_doom)},
   {"SkelWhoosh", AI_SkelWhoosh, (BDTC_doom)},
   {"SkelFist", AI_SkelFist, (BDTC_doom)},
   {"SkelMissile", AI_SkelMissile, (BDTC_doom)},
   {"FatRaise", AI_FatRaise, (BDTC_doom)},
   {"FatAttack1", AI_FatAttack1, (BDTC_doom)},
   {"FatAttack2", AI_FatAttack2, (BDTC_doom)},
   {"FatAttack3", AI_FatAttack3, (BDTC_doom)},
   {"BossDeath", AI_BossDeath, (BDTC_doom)},
   {"CPosAttack", AI_CPosAttack, (BDTC_doom)},
   {"CPosRefire", AI_CPosRefire, (BDTC_doom)},
   {"TroopAttack", AI_TroopAttack, (BDTC_doom)},
   {"SargAttack", AI_SargAttack, (BDTC_doom)},
   {"HeadAttack", AI_HeadAttack, (BDTC_doom)},
   {"BruisAttack", AI_BruisAttack, (BDTC_doom)},
   {"SkullAttack", AI_SkullAttack, (BDTC_doom)},
   {"Metal", AI_Metal, (BDTC_doom)},
   {"SpidRefire", AI_SpidRefire, (BDTC_doom)},
   {"BabyMetal", AI_BabyMetal, (BDTC_doom)},
   {"BspiAttack", AI_BspiAttack, (BDTC_doom)},
   {"Hoof", AI_Hoof, (BDTC_doom)},
   {"CyberAttack", AI_CyberAttack, (BDTC_doom)},
   {"PainAttack", AI_PainAttack, (BDTC_doom)},
   {"PainDie", AI_PainDie, (BDTC_doom)},
   {"KeenDie", AI_KeenDie, (BDTC_doom)},
   {"BrainPain", AI_BrainPain, (BDTC_doom)},
   {"BrainScream", AI_BrainScream, (BDTC_doom)},
   {"BrainDie", AI_BrainDie, (BDTC_doom)},
   {"BrainAwake", AI_BrainAwake, (BDTC_doom)},
   {"BrainSpit", AI_BrainSpit, (BDTC_doom)},
   {"SpawnSound", AI_SpawnSound, (BDTC_doom)},
   {"SpawnFly", AI_SpawnFly, (BDTC_doom)},
   {"BrainExplode", AI_BrainExplode, (BDTC_doom)},

   // [WDJ] MBF function ptrs, from MBF, EternityEngine.
   {"Detonate", AI_Detonate_MBF, (BDTC_MBF)},  // Radius damage, variable damage
   {"Mushroom", AI_Mushroom_MBF, (BDTC_MBF)},  // Mushroom explosion
   {"Die", AI_Die_MBF, (BDTC_MBF)},  // MBF, kill an object
   {"Spawn", AI_Spawn_MBF, (BDTC_MBF)},  // SpawnMobj(x,y, parm2, parm1-1)
   {"Turn", AI_Turn_MBF, (BDTC_MBF)},  // Turn by parm1 degrees
   {"Face", AI_Face_MBF, (BDTC_MBF)},  // Turn to face parm1 degrees
   {"Scratch", AI_Scratch_MBF, (BDTC_MBF)},  // Melee attack
   {"PlaySound", AI_PlaySound_MBF, (BDTC_MBF)},  // Play Sound parm1
      // if parm2 = 0 then mobj sound, else unassociated sound.
   {"RandomJump", AI_RandomJump_MBF, (BDTC_MBF)},  // Random transition to mobj state parm1
      // probability parm2
   {"LineEffect", AI_LineEffect_MBF, (BDTC_MBF)},  // Trigger line type parm1, tag = parm2

   {"KeepChasing", AI_KeepChasing_MBF, (BDTC_MBF)},  // MBF, from EnternityEngine
   
#ifdef MBF21
   // [WDJ] MBF21 actor function ptrs, from MBF21 spec and DSDA-Doom.
   // DSDA-Doom is a poor source for MBF21, as it is mangled and obtuse.
   // I have kept some extra information from DSDA, until I am sure that MBF21
   // has been implemented (as well as we can).
   {"CheckAmmo", AP_CheckAmmo_MBF21, (BDTC_MBF21)},  // args[0..1]
   {"SpawnObject", AI_SpawnObject_MBF21, (BDTC_MBF21)},  // Spawn object, args[0..7]
      // args[0] is thing_id
      // default {0}
   {"MonsterProjectile", AI_MonsterProjectile_MBF21, (BDTC_MBF21)},  // Monster projectile attack, args[0..4]
      // args[0] is thing_id
      // default {0}
   {"MonsterBulletAttack", AI_MonsterBulletAttack_MBF21, (BDTC_MBF21)},  // Monster bullet attack, args[0..4]
      // default {0, 0, 1, 3, 5}
   {"MonsterMeleeAttack", AI_MonsterMeleeAttack_MBF21, (BDTC_MBF21)},  // Monster melee attack, args[0..3]
      // default {3, 8, 0, 0}
   {"NoiseAlert", AI_NoiseAlert_MBF21, (BDTC_MBF21)},  // Alert nearby monsters, args none
   {"HealChase", AI_HealChase_MBF21, (BDTC_MBF21)},  // Heal monster, args[0..1]
   {"SeekTracer", AI_SeekTracer_MBF21, (BDTC_MBF21)},  // Seeker missile, args[0..1]
   {"ClearTracer", AI_ClearTracer_MBF21, (BDTC_MBF21)},  // Clear target, args none
   {"FindTracer", AI_FindTracer_MBF21, (BDTC_MBF21)},  // Find target, args[0..1]
      // default {0, 10}
   {"HealChase", AI_HealChase_MBF21, (BDTC_MBF21)},  // args[0..1]
   {"RadiusDamage", AI_RadiusDamage_MBF21, (BDTC_MBF21)},  // args[0..1]
   {"JumpIfHealthBelow", AI_JumpIfHealthBelow_MBF21, (BDTC_MBF21)},  // To state, if health below arg[1], args[0..1]
   {"JumpIfTargetInSight", AI_JumpIfTargetInSight_MBF21, (BDTC_MBF21)},  // To state, if target in line-of-sight, args[0..1]
   {"JumpIfTargetCloser", AI_JumpIfTargetCloser_MBF21, (BDTC_MBF21)},  // To state, if target closer than args[1], args[0..1]
   {"JumpIfTracerInSight", AI_JumpIfTracerInSight_MBF21, (BDTC_MBF21)},  // To state, if tracer in line-of-sight, args[0..1]
   {"JumpIfTracerCloser", AI_JumpIfTracerCloser_MBF21, (BDTC_MBF21)},  // To state, if target closer than args[1], args[0..1]
   {"JumpIfFlagsSet", AI_JumpIfFlagsSet_MBF21, (BDTC_MBF21)},  // To state, if all arg thing flags are set, args[0..2]
   {"AddFlags", AI_AddFlags_MBF21, (BDTC_MBF21)},  // Set thing flags, args[0..1]
   {"RemoveFlags", AI_RemoveFlags_MBF21, (BDTC_MBF21)},  // Remove thing flags, args[0..1]
#endif

#ifdef ETERNITY_ENGINE_BEX
   // [WDJ] Code pointers that are BEX available in EternityEngine.
   {"Nailbomb", AI_Nailbomb, (BDTC_ee)},
#endif

#ifdef ETERNITY_ENGINE_GEN2
   // haleyjd: start new eternity codeptrs
   {"StartScript", AI_StartScript, (BDTC_ee)},
   {"PlayerStartScript", AI_PlayerStartScript, (BDTC_ee)},
   {"SetFlags", AI_SetFlags, (BDTC_ee)},
   {"UnSetFlags", AI_UnSetFlags, (BDTC_ee)},
   {"BetaSkullAttack", AI_BetaSkullAttack, (BDTC_ee)},
   {"GenRefire", AI_GenRefire, (BDTC_ee)},
   {"FireGrenade", AI_FireGrenade, (BDTC_ee)},
   {"FireCustomBullets", AI_FireCustomBullets, (BDTC_ee)},
   {"FirePlayerMissile", AI_FirePlayerMissile, (BDTC_ee)},
   {"CustomPlayerMelee", AI_CustomPlayerMelee, (BDTC_ee)},
   {"GenTracer", AI_GenTracer, (BDTC_ee)},
   {"BFG11KHit", AI_BFG11KHit, (BDTC_ee)},
   {"BouncingBFG", AI_BouncingBFG, (BDTC_ee)},
   {"BFGBurst", AI_BFGBurst, (BDTC_ee)},
   {"FireOldBFG", AI_FireOldBFG, (BDTC_ee)},
   {"Stop", AI_Stop, (BDTC_ee)},
   {"PlayerThunk", AI_PlayerThunk, (BDTC_ee)},
   {"MissileAttack", AI_MissileAttack, (BDTC_ee)},
   {"MissileSpread", AI_MissileSpread, (BDTC_ee)},
   {"BulletAttack", AI_BulletAttack, (BDTC_ee)},
   {"HealthJump", AI_HealthJump, (BDTC_ee)},
   {"CounterJump", AI_CounterJump, (BDTC_ee)},
   {"CounterSwitch", AI_CounterSwitch, (BDTC_ee)},
   {"SetCounter", AI_SetCounter, (BDTC_ee)},
   {"CopyCounter", AI_CopyCounter, (BDTC_ee)},
   {"CounterOp", AI_CounterOp, (BDTC_ee)},
   {"SetTics", AI_SetTics, (BDTC_ee)},
   {"AproxDistance", AI_AproxDistance, (BDTC_ee)},
   {"ShowMessage", AI_ShowMessage, (BDTC_ee)},
   {"RandomWalk", AI_RandomWalk, (BDTC_ee)},
   {"TargetJump", AI_TargetJump, (BDTC_ee)},
   {"ThingSummon", AI_ThingSummon, (BDTC_ee)},
   {"KillChildren", AI_KillChildren, (BDTC_ee)},
   {"WeaponCtrJump", AI_WeaponCtrJump, (BDTC_ee)},
   {"WeaponCtrSwitch", AI_WeaponCtrSwitch, (BDTC_ee)},
   {"WeaponSetCtr", AI_WeaponSetCtr, (BDTC_ee)},
   {"WeaponCopyCtr", AI_WeaponCopyCtr, (BDTC_ee)},
   {"WeaponCtrOp", AI_WeaponCtrOp, (BDTC_ee)},
   {"AmbientThinker", AI_AmbientThinker, (BDTC_ee)},
   {"SteamSpawn", AI_SteamSpawn, (BDTC_ee)},
   {"EjectCasing", AI_EjectCasing, (BDTC_ee)},
   {"CasingThrust", AI_CasingThrust, (BDTC_ee)},
   {"JumpIfNoAmmo", AI_JumpIfNoAmmo, (BDTC_ee)},
   {"CheckReloadEx", AI_CheckReloadEx, (BDTC_ee)},
#else
   {"FireOldBFG", AP_FireBFG, (BDTC_ee)},  // Some wad uses it, so default it
#endif

#ifdef ETERNITY_ENGINE_NUKE_SPEC
   // haleyjd 07/13/03: nuke specials
   {"PainNukeSpec", AI_PainNukeSpec, (BDTC_haley|BDTC_ee)},
   {"SorcNukeSpec", AI_SorcNukeSpec, (BDTC_haley|BDTC_ee)},
#endif

   // haleyjd: Heretic pointers
#ifdef HERETIC_ETERNITY_ENGINE
   // Unique to EternityEngine, parameterized.
   {"SpawnGlitter", AI_SpawnGlitter, (BDTC_heretic|BDTC_ee)},
   {"AccelGlitter", AI_AccelGlitter, (BDTC_heretic|BDTC_ee)},
   {"SpawnAbove", AI_SpawnAbove, (BDTC_heretic|BDTC_ee)},
   {"HticDrop", AI_HticDrop, (BDTC_heretic|BDTC_ee)},  // Deprecated
   {"HticTracer", AI_HticTracer, (BDTC_heretic|BDTC_ee)},  // parameterized
   {"HticExplode", AI_HticExplode, (BDTC_heretic|BDTC_ee)},
   {"HticBossDeath", AI_HticBossDeath, (BDTC_heretic|BDTC_ee)},
#endif

   // Heretic normal
   {"MummyAttack", AI_MummyAttack, (BDTC_heretic)},
   {"MummyAttack2", AI_MummyAttack2, (BDTC_heretic)},
   {"MummySoul", AI_MummySoul, (BDTC_heretic)},
   {"ClinkAttack", AI_ClinkAttack, (BDTC_heretic)},
   {"BlueSpark", AI_BlueSpark, (BDTC_heretic)},
   {"GenWizard", AI_GenWizard, (BDTC_heretic)},
   {"WizardAtk1", AI_WizAtk1, (BDTC_heretic)},
   {"WizardAtk2", AI_WizAtk2, (BDTC_heretic)},
   {"WizardAtk3", AI_WizAtk3, (BDTC_heretic)},
   {"SorcererRise", AI_SorcererRise, (BDTC_heretic)},
   {"Srcr1Attack", AI_Srcr1Attack, (BDTC_heretic)},
   {"Srcr2Decide", AI_Srcr2Decide, (BDTC_heretic)},
   {"Srcr2Attack", AI_Srcr2Attack, (BDTC_heretic)},
   {"Sor1Chase", AI_Sor1Chase, (BDTC_heretic)},
   {"Sor1Pain", AI_Sor1Pain, (BDTC_heretic)},
   {"Sor2DthInit", AI_Sor2DthInit, (BDTC_heretic)},
   {"Sor2DthLoop", AI_Sor2DthLoop, (BDTC_heretic)},
   {"PodPain", AI_PodPain, (BDTC_heretic)},
   {"RemovePod", AI_RemovePod, (BDTC_heretic)},
   {"MakePod", AI_MakePod, (BDTC_heretic)},
   {"KnightAttack", AI_KnightAttack, (BDTC_heretic)},
   {"DripBlood", AI_DripBlood, (BDTC_heretic)},
   {"BeastAttack", AI_BeastAttack, (BDTC_heretic)},
   {"BeastPuff", AI_BeastPuff, (BDTC_heretic)},
   {"SnakeAttack", AI_SnakeAttack, (BDTC_heretic)},
   {"SnakeAttack2", AI_SnakeAttack2, (BDTC_heretic)},
   {"VolcanoBlast", AI_VolcanoBlast, (BDTC_heretic)},
   {"VolcBallImpact", AI_VolcBallImpact, (BDTC_heretic)},
   {"MinotaurDecide", AI_MinotaurDecide, (BDTC_heretic)},
   {"MinotaurAtk1", AI_MinotaurAtk1, (BDTC_heretic)},
   {"MinotaurAtk2", AI_MinotaurAtk2, (BDTC_heretic)},
   {"MinotaurAtk3", AI_MinotaurAtk3, (BDTC_heretic)},
   {"MinotaurCharge", AI_MinotaurCharge, (BDTC_heretic)},
   {"MntrFloorFire", AI_MntrFloorFire, (BDTC_heretic)},
//   {"LichFire", AI_LichFire, (BDTC_heretic)},
//   {"LichWhirlwind", AI_LichWhirlwind, (BDTC_heretic)},
   {"LichAttack", AI_HHeadAttack, (BDTC_heretic)},
   {"WhirlwindSeek", AI_WhirlwindSeek, (BDTC_heretic)},
   {"LichIceImpact", AI_HeadIceImpact, (BDTC_heretic)},
   {"LichFireGrow", AI_HeadFireGrow, (BDTC_heretic)},
//   {"ImpChargeAtk", AI_ImpChargeAtk, (BDTC_heretic)},
   {"ImpMeleeAtk", AI_ImpMeAttack, (BDTC_heretic)},
   {"ImpMissileAtk", AI_ImpMsAttack, (BDTC_heretic)},  // Boss Imp
   {"ImpDeath", AI_ImpDeath, (BDTC_heretic)},
   {"ImpXDeath1", AI_ImpXDeath1, (BDTC_heretic)},
   {"ImpXDeath2", AI_ImpXDeath2, (BDTC_heretic)},
   {"ImpExplode", AI_ImpExplode, (BDTC_heretic)},
   {"PhoenixPuff", AI_PhoenixPuff, (BDTC_heretic)},
//   {"PlayerSkull", AI_PlayerSkull, (BDTC_heretic)},
//   {"ClearSkin", AI_ClearSkin, (BDTC_heretic)},
   
#ifdef HEXEN
   // haleyjd 10/04/08: Hexen pointers
   {"SetInvulnerable", AI_SetInvulnerable, (BDTC_hexen)},
   {"UnSetInvulnerable", AI_UnSetInvulnerable, (BDTC_hexen)},
   {"SetReflective", AI_SetReflective, (BDTC_hexen)},
   {"UnSetReflective", AI_UnSetReflective, (BDTC_hexen)},
   {"PigLook", AI_PigLook, (BDTC_hexen)},
   {"PigChase", AI_PigChase, (BDTC_hexen)},
   {"PigAttack", AI_PigAttack, (BDTC_hexen)},
   {"PigPain", AI_PigPain, (BDTC_hexen)},
   {"HexenExplode", AI_HexenExplode, (BDTC_hexen)},
   {"SerpentUnHide", AI_SerpentUnHide, (BDTC_hexen)},
   {"SerpentHide", AI_SerpentHide, (BDTC_hexen)},
   {"SerpentChase", AI_SerpentChase, (BDTC_hexen)},
   {"RaiseFloorClip", AI_RaiseFloorClip, (BDTC_hexen)},
   {"LowerFloorClip", AI_LowerFloorClip, (BDTC_hexen)},
   {"SerpentHumpDecide", AI_SerpentHumpDecide, (BDTC_hexen)},
   {"SerpentWalk", AI_SerpentWalk, (BDTC_hexen)},
   {"SerpentCheckForAttack", AI_SerpentCheckForAttack, (BDTC_hexen)},
   {"SerpentChooseAttack", AI_SerpentChooseAttack, (BDTC_hexen)},
   {"SerpentMeleeAttack", AI_SerpentMeleeAttack, (BDTC_hexen)},
   {"SerpentMissileAttack", AI_SerpentMissileAttack, (BDTC_hexen)},
   {"SerpentSpawnGibs", AI_SerpentSpawnGibs, (BDTC_hexen)},
   {"SubTics", AI_SubTics, (BDTC_hexen)},
   {"SerpentHeadCheck", AI_SerpentHeadCheck, (BDTC_hexen)},
   {"CentaurAttack", AI_CentaurAttack, (BDTC_hexen)},
   {"CentaurAttack2", AI_CentaurAttack2, (BDTC_hexen)},
   {"DropEquipment", AI_DropEquipment, (BDTC_hexen)},
   {"CentaurDefend", AI_CentaurDefend, (BDTC_hexen)},
   {"BishopAttack", AI_BishopAttack, (BDTC_hexen)},
   {"BishopAttack2", AI_BishopAttack2, (BDTC_hexen)},
   {"BishopMissileWeave", AI_BishopMissileWeave, (BDTC_hexen)},
   {"BishopDoBlur", AI_BishopDoBlur, (BDTC_hexen)},
   {"SpawnBlur", AI_SpawnBlur, (BDTC_hexen)},
   {"BishopChase", AI_BishopChase, (BDTC_hexen)},
   {"BishopPainBlur", AI_BishopPainBlur, (BDTC_hexen)},
   {"DragonInitFlight", AI_DragonInitFlight, (BDTC_hexen)},
   {"DragonFlight", AI_DragonFlight, (BDTC_hexen)},
   {"DragonFlap", AI_DragonFlap, (BDTC_hexen)},
   {"DragonFX2", AI_DragonFX2, (BDTC_hexen)},
   {"PainCounterBEQ", AI_PainCounterBEQ, (BDTC_hexen)},
   {"DemonAttack1", AI_DemonAttack1, (BDTC_hexen)},
   {"DemonAttack2", AI_DemonAttack2, (BDTC_hexen)},
   {"DemonDeath", AI_DemonDeath, (BDTC_hexen)},
   {"Demon2Death", AI_Demon2Death, (BDTC_hexen)},
   {"WraithInit", AI_WraithInit, (BDTC_hexen)},
   {"WraithRaiseInit", AI_WraithRaiseInit, (BDTC_hexen)},
   {"WraithRaise", AI_WraithRaise, (BDTC_hexen)},
   {"WraithMelee", AI_WraithMelee, (BDTC_hexen)},
   {"WraithMissile", AI_WraithMissile, (BDTC_hexen)},
   {"WraithFX2", AI_WraithFX2, (BDTC_hexen)},
   {"WraithFX3", AI_WraithFX3, (BDTC_hexen)},
   {"WraithFX4", AI_WraithFX4, (BDTC_hexen)},
   {"WraithLook", AI_WraithLook, (BDTC_hexen)},
   {"WraithChase", AI_WraithChase, (BDTC_hexen)},
   {"EttinAttack", AI_EttinAttack, (BDTC_hexen)},
   {"DropMace", AI_DropMace, (BDTC_hexen)},
   {"AffritSpawnRock", AI_AffritSpawnRock, (BDTC_hexen)},
   {"AffritRocks", AI_AffritRocks, (BDTC_hexen)},
   {"SmBounce", AI_SmBounce, (BDTC_hexen)},
   {"AffritChase", AI_AffritChase, (BDTC_hexen)},
   {"AffritSplotch", AI_AffritSplotch, (BDTC_hexen)},
   {"IceGuyLook", AI_IceGuyLook, (BDTC_hexen)},
   {"IceGuyChase", AI_IceGuyChase, (BDTC_hexen)},
   {"IceGuyAttack", AI_IceGuyAttack, (BDTC_hexen)},
   {"IceGuyDie", AI_IceGuyDie, (BDTC_hexen)},
   {"IceGuyMissileExplode", AI_IceGuyMissileExplode, (BDTC_hexen)},
   {"CheckFloor", AI_CheckFloor, (BDTC_hexen)},
   {"FreezeDeath", AI_FreezeDeath, (BDTC_hexen)},
   {"IceSetTics", AI_IceSetTics, (BDTC_hexen)},
   {"IceCheckHeadDone", AI_IceCheckHeadDone, (BDTC_hexen)},
   {"FreezeDeathChunks", AI_FreezeDeathChunks, (BDTC_hexen)},
#endif
};
#define NUM_bex_action_table   (sizeof(bex_action_table) / sizeof(bex_codeptr_t))


//  si : state index
static
void action_function_store( uint16_t deh_frame_num, uint16_t si, const char * funcname )
{
    // search action table
    int i;
    for(i=0; i<NUM_bex_action_table; i++)
    {
        bex_codeptr_t * bcp = & bex_action_table[i];
        if(!strcasecmp(bcp->kstr, funcname))  // BEX action search
        {
            // change the sprite behavior at the framenum
#ifdef STATE_FUNC_CHECK
            if( verbose && bcp->action )
            {
                static const char * action_str[3] = { "", "acp1", "acp2" };
                byte func_acp = action_acp( bcp->action );
                byte state_acp = lookup_state_acp(si);
                byte sprite_acp = lookup_sprite_acp(states[si].sprite);
                if( (func_acp | state_acp | sprite_acp) > 2 )
                {
                    deh_error( "BEX puts %s action function (%s) into conflicting state (%i)\n",
                                action_str[func_acp], funcname, si );
                }
            }
#endif
            // change the sprite behavior at the framenum
            states[si].action = bcp->action;
            deh_key_detect |= bcp->bdtc;  // standards used
            return;
        }
    }

//  no_action_change:
    deh_error("Action not changed : FRAME %d\n", deh_frame_num );
    return;
}


// BEX [CODEPTR] section
static
void bex_codeptr( myfile_t* f )
{
  char funcname[BEX_KEYW_LEN];
  char s[MAXLINELEN];
  int  framenum, nn, si;
   
  // format:
  // [CODEPTR]
  // FRAME <framenum> = <funcname>

  for(;;) {
    if( ! myfgets_nocom(s, sizeof(s), f) )
       break; // no more lines
    if( s[0] == 0 ) break;
    if( s[0] == '\n' ) break;  // blank line  (PrBoom, Eternity)
    if( s[0] == '#' ) continue;  // comment
    if( strncasecmp( s, "FRAME", 5 ) != 0 )  break;  // not a FRAME line
    nn = sscanf( &s[5], "%d = %s", &framenum, funcname );
    if( nn != 2 )
    {
        deh_error( "Bad FRAME syntax\n" );
        continue;
    }
    si = DEH_frame_to_state(framenum);
    if( si == S_NULL )
    {
        deh_error( "Bad BEX FRAME number %d\n", framenum );
        continue;
    }
    action_function_store( framenum, si, funcname );
  }
}


// [WDJ] MBF helper, From PrBoom (not in MBF)
// haleyjd 9/22/99
//
// Allows handy substitution of any thing for helper dogs.  DEH patches
// are being made frequently for this purpose and it requires a complete
// rewiring of the DOG thing.  I feel this is a waste of effort, and so
// have added this new [HELPER] BEX block

// BEX [HELPER] section
static
void bex_helper( myfile_t* f )
{
  char s[MAXLINELEN];
  int  tn, nn;
   
  // format:
  // [HELPER]
  // TYPE = <num>

  for(;;)
  {
    if( ! myfgets_nocom(s, sizeof(s), f) )
       break; // no more lines

    if( s[0] == 0 ) break;
    if( s[0] == '\n' ) break;  // blank line  (PrBoom, Eternity)
    if( s[0] == '#' ) continue;  // comment
    if( strncasecmp( s, "TYPE", 4 ) != 0 )  break;  // not a TYPE line
    nn = sscanf( &s[4], " = %d", &tn );

    if( nn != 1 )
    {
        deh_error( "Bad TYPE syntax\n" );
        continue;
    }

    // In BEX things are 1..
    if( tn <= 0 || tn > ENDDOOM_MT )
    {
        deh_error( "Bad BEX thing number %d\n", tn );
        continue;
    }

    helper_MT = tn - 1;  // mobj MT  (in BEX things are 1.. )
  }
}
   

// include another DEH or BEX file
void bex_include( char * inclfilename )
{
  static boolean include_nested = 0;
  
  // myfile_t is local to DEH_LoadDehackedLump
  
  if( include_nested )
  {
    deh_error( "BEX INCLUDE, only one level allowed\n" );
    return;
  }
  // save state
  
  include_nested = 1;
//  DEH_LoadDehackedFile( inclfile );  // do the include file
  W_Load_WadFile (inclfilename);
  include_nested = 0;
   
  // restore state
}


#ifdef MBF21
// MBF21 weapon flags
typedef struct {
    char *    name;
    byte      flagval; // weapon_flags_e
} weapon_flag_name_t;

// From DSDA
weapon_flag_name_t MBF21_weapon_flag_name_table[] =
{
  { "NOTHRUST", WPF_NO_THRUST },  // weapon does not thrust mobj it hit
  { "SILENT", WPF_SILENT },      // weapon is silent
  { "NOAUTOFIRE", WPF_NO_AUTOFIRE },  // weapon will not autofire
  { "FLEEMELEE", WPF_FLEE_MELEE },    // monsters consider it a melee weapon
  { "AUTOSWITCHFROM", WPF_AUTOSWITCH_FROM },  // can switch away by picking up other ammo
  { "NOAUTOSWITCHTO", WPF_NO_AUTOSWITCH_TO }, // cannot switch to by picking up ammo
};
#define NUM_MBF21_weapon_flag_name_table  (sizeof(MBF21_weapon_flag_name_table)/sizeof(weapon_flag_name_t))
#endif


/*
Ammo type = 2
Deselect frame = 11
Select frame = 12
Bobbing frame = 13
Shooting frame = 17
Firing frame = 10
*/
static
void read_weapon( myfile_t * f, int deh_weapon_id )
{
  weaponinfo_t * wip = & doomweaponinfo[ deh_weapon_id ];
  char s[MAXLINELEN];
  char *word;
  int value;

  do{
    if(myfgets(s,sizeof(s),f)!=NULL)
    {
      if(s[0]=='\n') break;
      value=searchvalue(s);
      word=strtok(s," ");
      if( !word || word[0]=='\n' )  break;
      if( ! confirm_searchvalue(value)  )  continue;

#ifdef MBF21
      if(!strcasecmp(word,"Ammo"))
      {
          char * word2 = strtok(NULL," ");
          if(!strcasecmp(word2,"per"))  // "Ammo per shot" spec MBF21
          {
              // This field already existed in DoomLegacy
              wip->ammo_per_shot = value;
              wip->eflags |= WEF_MBF21_PER_SHOT;  // MBF21 interpretation (WIF_ENABLEAPS)
          }
          else
          {
              wip->ammo = value;  // ammo type
          }
      }
      else if(!strcasecmp(word,"MBF21"))   // "MBF21", "MBF21 Bits", "Bits"
      {
          // MBF21 Bits for weapon
          weapon_flag_name_t * fnp; // BEX flag names ptr
          deh_key_detect |= BDTC_MBF21;

          byte wflags = 0;
          for(;;)
          {
              word = strtok(NULL, " +|\t=\n");
              if( word == NULL )  goto set_wflags_next_line;
              if( word[0] == '\n' )   goto set_wflags_next_line;

              if( !strcasecmp(word,"Bits") ) continue;  // was "MBF21 Bits"

              // detect bits by integer value
              // Prevent using signed char as index.
              if( isdigit( (unsigned char)word[0] ) || word[0] == '-' )
              {
                  wflags = str_to_uint32(word);
                  // Handle MBF21 bits numeric assign.
                  goto set_wflags_next_line;
              }

              // handle MBF21 flag names
              int fnpi;
              for( fnpi = 0 ; fnpi < NUM_MBF21_weapon_flag_name_table; fnpi++ )
              {
                  fnp = &MBF21_weapon_flag_name_table[fnpi];
                  if(!strcasecmp( word, fnp->name ))  // find name
                  {
                     // MBF21 name
                      deh_key_detect |= BDTC_MBF21;
                      wflags |= fnp->flagval;
                      goto next_word; // done with this word
                  }
              }
              deh_error("Bits name unknown: %s\n", word);
              // continue with next keyword
            next_word:
              continue;
          }

        set_wflags_next_line:
          wip->flags = wflags;
          continue; // next line
      }
#else
           if(!strcasecmp(word,"Ammo"))       wip->ammo      =value;
#endif
      else if(!strcasecmp(word,"Deselect"))   set_state( &wip->upstate, value );
      else if(!strcasecmp(word,"Select"))     set_state( &wip->downstate, value );
      else if(!strcasecmp(word,"Bobbing"))    set_state( &wip->readystate, value );
      else if(!strcasecmp(word,"Shooting"))
      {
         set_state( &wip->atkstate, value );
         wip->holdatkstate = wip->atkstate;
//         set_state( &wip->holdatkstate, wip->atkstate );
      }
      else if(!strcasecmp(word,"Firing"))     set_state( &wip->flashstate, value );
      else deh_error("Weapon %d : unknown word '%s'\n", deh_weapon_id,word);
    }
  } while(s[0]!='\n' && !myfeof(f));
}

/*
Ammo 1
Max ammo = 400
Per ammo = 40

where:
  Ammo 0 = Bullets
  Ammo 1 = Shells (shotgun)
  Ammo 2 = Cells
  Ammo 3 = Rockets
*/

extern uint16_t clipammo[];
extern uint16_t WeaponAmmo_pickup[];

static
void read_ammo( myfile_t * f, int num )
{
  char s[MAXLINELEN];
  char *word;
  int value;

  do{
    if(myfgets(s,sizeof(s),f)!=NULL)
    {
      if(s[0]=='\n') break;
      value=searchvalue(s);
      word=strtok(s," ");
      if( !word || word[0]=='\n' )  break;
      if( ! confirm_searchvalue(value)  )  continue;

      if(!strcasecmp(word,"Max"))
          maxammo[num] =value;
      else if(!strcasecmp(word,"Per"))
      {
          clipammo[num]=value;  // amount of ammo for small item
          WeaponAmmo_pickup[num] = 2*value;  // per weapon
      }
      else if(!strcasecmp(word,"Perweapon"))
          WeaponAmmo_pickup[num] = 2*value; 
      else
          deh_error("Ammo %d : unknown word '%s'\n",num,word);
    }
  } while(s[0]!='\n' && !myfeof(f));
}

// i don't like that but do you see a other way ?
extern int idfa_armor;
extern int idfa_armor_class;
extern int idkfa_armor;
extern int idkfa_armor_class;
extern int god_health;
extern int initial_health;
extern int initial_bullets;
extern int MAXHEALTH;
extern int max_armor;
extern int green_armor_class;
extern int blue_armor_class;
extern int max_soul_health;
extern int soul_health;
extern int mega_health;


static
void read_misc( myfile_t * f )
{
  char s[MAXLINELEN];
  char *word,*word2;
  int value;
  do{
    if(myfgets(s,sizeof(s),f)!=NULL)
    {
      if(s[0]=='\n') break;
      value=searchvalue(s);  // none -> 0
      word=strtok(s," ");
      if(!word || word[0]=='\n') break;
      if( ! confirm_searchvalue(value)  )  continue;

      word2=strtok(NULL," ");

      if(!strcasecmp(word,"Initial"))
      {
        if( word2 )
        {
          if(!strcasecmp(word2,"Health"))          initial_health=value;
          else if(!strcasecmp(word2,"Bullets"))    initial_bullets=value;
        }
      }
      else if(!strcasecmp(word,"Max"))
      {
        if( word2 )
        {
          if(!strcasecmp(word2,"Health"))          MAXHEALTH=value;
          else if(!strcasecmp(word2,"Armor"))      max_armor=value;
          else if(!strcasecmp(word2,"Soulsphere")) max_soul_health=value;
        }
      }
      else if(!strcasecmp(word,"Green"))         green_armor_class=value;
      else if(!strcasecmp(word,"Blue"))          blue_armor_class=value;
      else if(!strcasecmp(word,"Soulsphere"))    soul_health=value;
      else if(!strcasecmp(word,"Megasphere"))    mega_health=value;
      else if(!strcasecmp(word,"God"))           god_health=value;
      else if(!strcasecmp(word,"IDFA"))
      {
         // Syntax: IDFA Armor, IDFA Armor class
         char * word3=strtok(NULL," ");
         if( word3 )
         {
           if(!strcmp(word3,"="))               idfa_armor=value;
           else if(!strcasecmp(word3,"Class"))  idfa_armor_class=value;
         }
      }
      else if(!strcasecmp(word,"IDKFA"))
      {
         // Syntax: IDKFA Armor, IDKFA Armor class
         char * word3=strtok(NULL," ");
         if( word3 )
         {
           if(!strcmp(word3,"="))               idkfa_armor=value;
           else if(!strcasecmp(word3,"Class"))  idkfa_armor_class=value;
         }
      }
      else if(!strcasecmp(word,"BFG"))        doomweaponinfo[wp_bfg].ammo_per_shot = value;
      else if(!strcasecmp(word,"Monsters Ignore"))
      {
          // ZDoom at least
          // Looks like a coop setting
          switch( value )
          {
           case 0:
             monster_infight_deh = INFT_infight_off; // infight off
             break;
           default:
             monster_infight_deh = INFT_coop; // coop
             break;
          }
      }
      else if(!strcasecmp(word,"Monsters"))
      {
          //DarkWolf95:November 21, 2003: Monsters Infight!
          //[WDJ] from prboom the string is "Monsters Infight"
          // cannot confirm any specific valid numbers
          switch( value )
          {
           case 0: // previous behavior: default to on
           case 1: // if user tries to set it on
              monster_infight_deh = INFT_infight; // infight on
              break;
           case 3: // extended behavior, coop monsters
              monster_infight_deh = INFT_coop;
              break;
           case 221: // value=221 -> on (prboom)
              // PrBoom implements full infight
              monster_infight_deh = INFT_full_infight; // infight on
              break;
           case 202: // value=202 -> off (prboom)
           default:  // off
              monster_infight_deh = INFT_infight_off; // infight off
              break;
          }
      }
      else deh_error("Misc : unknown  '%s'\n", word);
    }
  } while(s[0]!='\n' && !myfeof(f));
}

extern byte cheat_mus_seq[];
extern byte cheat_choppers_seq[];
extern byte cheat_god_seq[];
extern byte cheat_ammo_seq[];
extern byte cheat_ammonokey_seq[];
extern byte cheat_noclip_seq[];
extern byte cheat_commercial_noclip_seq[];
extern byte cheat_powerup_seq[7][10];
extern byte cheat_clev_seq[];
extern byte cheat_mypos_seq[];
extern byte cheat_amap_seq[];

static void change_cheat_code(byte *cheatseq, byte *newcheat)
{
  byte *i,*j;

  // encrypt data
  for(i=newcheat;i[0]!='\0';i++)
      i[0]=SCRAMBLE(i[0]);

  for(i=cheatseq,j=newcheat;j[0]!='\0' && j[0]!=0xff;i++,j++)
  {
      if(i[0]==1 || i[0]==0xff) // no more place in the cheat
      {
         deh_error("Cheat too long\n");
         return;
      }
      else
         i[0]=j[0];
  }

  // newcheatseq < oldcheat
  j=i;
  // search special cheat with 100
  for(;i[0]!=0xff;i++)
  {
      if(i[0]==1)
      {
         *j++=1;
         *j++=0;
         *j++=0;
         break;
      }
  }
  *j=0xff;

  return;
}

// Read cheat section
static
void read_cheat( myfile_t * f )
{
  char s[MAXLINELEN];
  char *word,*word2;
  byte * valuestr;

  do{
    if(myfgets(s,sizeof(s),f)!=NULL)
    {
      // for each line "<word> = <value>"
      if(s[0]=='\n') break;
      char * word1 = strtok(s,"=");  // after '='
      if( !word1 || word1[0]=='\n' )  break;
      valuestr = (byte *)strtok(NULL," \n");         // skip the space
      if( !valuestr || valuestr[0]=='\n' )  break;
      strtok(NULL," \n");              // finish the string
      word=strtok(s," ");
      if( !word || word[0]=='\n' )  break;

      if(!strcasecmp(word     ,"Change"))        change_cheat_code(cheat_mus_seq,valuestr);
      else if(!strcasecmp(word,"Chainsaw"))      change_cheat_code(cheat_choppers_seq,valuestr);
      else if(!strcasecmp(word,"God"))           change_cheat_code(cheat_god_seq,valuestr);
      else if(!strcasecmp(word,"Ammo"))
      {
             word2=strtok(NULL," ");
             if(word2 && !strcmp(word2,"&")) change_cheat_code(cheat_ammo_seq,valuestr);
             else                            change_cheat_code(cheat_ammonokey_seq,valuestr);
      }
      else if(!strcasecmp(word,"No"))
      {
             word2=strtok(NULL," ");
             if(word2)
                word2=strtok(NULL," ");

             if(word2 && !strcmp(word2,"1")) change_cheat_code(cheat_noclip_seq,valuestr);
             else                            change_cheat_code(cheat_commercial_noclip_seq,valuestr);

      }
      else if(!strcasecmp(word,"Invincibility")) change_cheat_code(cheat_powerup_seq[0],valuestr);
      else if(!strcasecmp(word,"Berserk"))       change_cheat_code(cheat_powerup_seq[1],valuestr);
      else if(!strcasecmp(word,"Invisibility"))  change_cheat_code(cheat_powerup_seq[2],valuestr);
      else if(!strcasecmp(word,"Radiation"))     change_cheat_code(cheat_powerup_seq[3],valuestr);
      else if(!strcasecmp(word,"Auto-map"))      change_cheat_code(cheat_powerup_seq[4],valuestr);
      else if(!strcasecmp(word,"Lite-Amp"))      change_cheat_code(cheat_powerup_seq[5],valuestr);
      else if(!strcasecmp(word,"BEHOLD"))        change_cheat_code(cheat_powerup_seq[6],valuestr);
      else if(!strcasecmp(word,"Level"))         change_cheat_code(cheat_clev_seq,valuestr);
      else if(!strcasecmp(word,"Player"))        change_cheat_code(cheat_mypos_seq,valuestr);
      else if(!strcasecmp(word,"Map"))           change_cheat_code(cheat_amap_seq,valuestr);
      else deh_error("Cheat : unknown word '%s'\n",word);
    }
  } while(s[0]!='\n' && !myfeof(f));
}


// Initialize what is reported in Finalize
// This is called for each file with dehacked.
void DEH_Init_Load_Dehacked( void )
{
  deh_num_error=0;
  deh_key_detect = 0;
  deh_thing_detect = 0;
}

// permission: 0=game, 1=adv, 2=language
// This is called for each file with dehacked.
void DEH_LoadDehackedFile( myfile_t* f, byte bex_permission )
{
  char       s[1000];
  char       *word,*word2;
  int        i;

  deh_num_error=0;
  deh_key_detect = 0;
  deh_thing_detect = 0;

  // it don't test the version of doom
  // and version of dehacked file
  while(!myfeof(f))
  {
    myfgets(s,sizeof(s),f);
    if(s[0]=='\n' || s[0]=='#')  // skip blank lines and comments
      continue;

    word=strtok(s," ");  // first keyword
    if(word!=NULL)
    {
      if( word[0] == '\n' )  // ignore blank line
        continue;
      if(!strncmp(word, "[STRINGS]", 9))
      {
        bex_strings(f, bex_permission);
        continue;
      }
      else if(!strncmp(word, "[PARS]", 6))
      {
        bex_pars(f);
        continue;
      }
      else if(!strncmp(word, "[CODEPTR]", 9))
      {
        bex_codeptr(f);
        continue;
      }
      else if(!strncmp(word, "[HELPER]", 8))
      {
        bex_helper(f);
        continue;
      }
      else if(!strncmp(word, "INCLUDE", 7))
      {
        word2=strtok(NULL," ");
        if( word2 )
        {
          if(!strcasecmp( word2, "NOTEXT" ))
          {
            bex_include_notext = 1;
            word2=strtok(NULL," "); // filename
          }
          if( word2 )
              bex_include( word2 ); // include file
          bex_include_notext = 0;
        }
        continue;
      }
      else if(!strncmp(word, "Engine", 6)
              || !strncmp(word, "Data", 4)
              || !strncmp(word, "IWAD", 4) )
         continue; // WhackEd2 data, ignore it

      word2=strtok(NULL," ");  // id number
      if(word2!=NULL)
      {
        i=atoi(word2);

        if(!strcasecmp(word,"Thing"))
        {
          // "Thing <num>", 1..
          // deh_thing_id = MT_xxx + 1
          if( i<=0 || i>NUMMOBJTYPES )
          {
            deh_error("Thing %d beyond limits\n",i);
            i = MT_UNK1 + 1;  // 1..
          }

          if( i >= 138 && i <= 150 )  // problem area
          {
            // Test for keywords in description
            char * nc = word2 + strlen(word2) + 1;
            // substitute to deh_thing_id
            int i2 = lookup_thing_type( nc, i - 1 ) + 1;
            if( (i2 != i) && (verbose) )
            {
                GenPrintf( EMSG_warn, "Remap thing %i to thing %i\n", i, i2 );
            }
            i = i2;
          }

          read_thing(f,i);
        }
        else if(!strcasecmp(word,"Frame"))
             {
               // "Frame <num>"
               read_frame(f,i);
             }
        else if(!strcasecmp(word,"Pointer"))
             {
               // "Pointer <xref>"
               read_pointer(f,i);
             }
        else if(!strcasecmp(word,"Sound"))
             {
               // "Sound <num>"
#ifdef ENABLE_DEH_REMAP
               uint16_t sound_indx = remap_sound(i);
#else
               uint16_t sound_indx = i;
               if( sound_indx>=0 && sound_indx<NUMSFX_EXT )
               {
                   deh_error("Sound %d beyond limits\n", sound_indx);
                   sound_indx = sfx_freeslot_last; // must still read the sound line
               }
#endif
               read_sound( f, sound_indx );
             }
        else if(!strcasecmp(word,"Sprite"))
             {
               // "Sprite <num>"
               // PrBoom does not support the DEH sprite block.
#ifdef ENABLE_DEH_REMAP
               uint16_t spr_indx = remap_sprite(i, 0);
#else
               uint16_t spr_indx = i;
#endif
               // [WDJ] Cannot even tell what this is supposed to do.
               // It looks like it changes the standard sprite names to something else.
               // Some other ports are not supporting this.
               if( spr_indx>=0 && spr_indx<NUMSPRITES_EXT )
               {
                 if(myfgets(s,sizeof(s),f)!=NULL)
                 {
                   int koff = searchvalue(s);
                   if( confirm_searchvalue(koff) )
                   {
                     int k = (koff - 151328)/8;
                     if( k>=0 && k<NUMSPRITES_DEF )
                     {
                       sprnames[spr_indx]=deh_sprnames[k];
                     }
                     else
                     {
                       deh_error("Sprite %d: sprite %i (off=%i) beyond limits\n", spr_indx, k, koff);
                     }
                   }
                 }
               }
               else
               {
                  deh_error("Sprite %d beyond limits\n",spr_indx);
               }
             }
        else if(!strcasecmp(word,"Text"))
             {
               // "Text <num>"
               word2 = strtok(NULL," ");
               if( word2 )
               {
                 int j=atoi(word);
                 read_text(f,i,j);
               }
               else
               {
                   deh_error("Text : missing second number\n");
               }
             }
        else if(!strcasecmp(word,"Weapon"))
             {
               // "Weapon <num>"
               if(i<NUMWEAPONS && i>=0)
                   read_weapon(f,i);
               else
                   deh_error("Weapon %d don't exist\n",i);
             }
        else if(!strcasecmp(word,"Ammo"))
             {
               // "Ammo <num>"
               if(i<NUMAMMO && i>=0)
                   read_ammo(f,i);
               else
                   deh_error("Ammo %d don't exist\n",i);
             }
        else if(!strcasecmp(word,"Misc"))
               // "Misc <num>"
               read_misc(f);
        else if(!strcasecmp(word,"Cheat"))
               // "Cheat <num>"
               read_cheat(f);
        else if(!strcasecmp(word,"Doom"))
        {
               // "Doom <num>"
               // version 19: hth2, pl2_lev,
               // version 21: phobiata, antaxyz, newmaps (chex), valiant
               // version 21, MBF21: infectio, sleepwalking, whatlies
             word2 = strtok(NULL,"\n");
             if( word2 )
             {
               int ver = searchvalue(word2);
               if( confirm_searchvalue(ver) )
               {
                 CONS_Printf("Dehacked version:%i\n", ver);
#if 0
                 if( ver == 21 )
                 {
                 }
#endif
                 if( ver > 21)
                    deh_error("Warning : dehacked version > 2.1 are partially supported\n",ver);
               }
             }
        }
        else if(!strcasecmp(word,"Patch"))
        {
               // "Patch <num>"
               word2 = strtok(NULL," ");
               if(word2 && !strcasecmp(word2,"format"))
               {
                 char * word3 = strtok(NULL,"\n");
                 int patver = searchvalue(word3);
                 if( confirm_searchvalue(patver) )
                 {
                   if(patver!=6)
                     deh_error("Warning : Patch format not supported");
                 }
               }
        }
        else if( ! strncmp( word, "//", 2 ))
        {
            // Some people are using "//" as comment, see Avactor.wad
        }
        else deh_error("Line %i: Unknown word '%s'\n", linenum, word);
      }
      else
          deh_error("Line %i: missing argument for '%s'\n", linenum, word);
    }
    else
    {
        deh_error("Line %i: No word in this line:\n%s\n", linenum, s);
    }
  } // end while

  if (deh_num_error>0)
  {
      CONS_Printf("%d warning(s) in the dehacked file\n",deh_num_error);
      if (devparm)
          getchar();
  }

  deh_loaded = true;
  if( thing_flags_valid_deh )
      Translucency_OnChange();  // ensure translucent updates
}

// read dehacked lump in a wad (there is special trick for for deh 
// file that are converted to wad in w_wad.c)
void DEH_LoadDehackedLump( lumpnum_t lumpnum )
{
    myfile_t f;
    
    f.size = W_LumpLength(lumpnum);
    f.data = Z_Malloc(f.size + 1, PU_IN_USE, 0);  // temp
    W_ReadLump(lumpnum, f.data);
    f.curpos = f.data;
    f.data[f.size] = 0;

    DEH_LoadDehackedFile( &f, 0 );
    Z_Free(f.data);
}


// [WDJ] Before any changes, save all comparison info, so that multiple
// DEH files and lumps can be handled without interfering with each other.
// Called once
void DEH_Init(void)
{
  int i;
  // save value for cross reference
  for(i=0;i<NUMSTATES_DEF;i++)
      deh_actions[i]=states[i].action;
  for(i=0;i<NUMSPRITES_DEF;i++)
      deh_sprnames[i]=sprnames[i];
  for(i=0;i<NUMSFX_DEF;i++)
      deh_sfxnames[i]=S_sfx[i].name;
  for(i=1;i<NUMMUSIC_DEF;i++)
      deh_musicname[i]=S_music[i].name;
  for(i=0;i<NUMTEXT;i++)
      deh_text[i]=text[i];

  monster_infight_deh = INFT_none; // not set

  if( M_CheckParm("-dehthing") )
  {
      DEH_set_deh_thing_code( M_GetNextParm() );
  }
   
#if defined DEBUG_WINDOWED || defined PARANOIA
  // [WDJ] Checks of constants that I could not do at compile-time (enums).
  if( (int)STS_TECH2LAMP4 != (int)S_TECH2LAMP4 )
    I_Error( "DEH: state S_TECH2LAMP4 error\n" );
  if( STH_PODGENERATOR != (S_PODGENERATOR - S_FREETARGMOBJ + STH_FREETARGMOBJ ))
    I_Error( "DEH: state S_PODGENERATOR error" );
  if( STH_SND_WATERFALL != (S_SND_WATERFALL - S_BLOODYSKULL1 + STH_BLOODYSKULL1 ))
    I_Error( "DEH: state S_SND_WATERFALL error\n" );
#endif
    
  deh_thing_detect = 0;
}


// ----
// [WDJ] Check state chains for inconsistencies.

const char * lookup_action_name( action_fi_t ai )
{
    int i;
    for( i = 0; i < NUM_bex_action_table; i++ )
    {
        bex_codeptr_t * bcp = &bex_action_table[i];
        if( bcp->action == ai )
            return bcp->kstr;
    }
    return "unknown";
}

// For state uses that are fixed in code.
// Thing invocations will all be acp1 states.
typedef struct fixed_state_rep
{
    uint16_t state1, state2;  // range of states
    byte     acp;  // 0=none, 1=acp1, 2=acp2
} fixed_state_t;

fixed_state_t  fixed_state_table[] =
{
// Doom    
  { S_SAW, S_SAW1, 2 },  // Doom weaponinfo
  { S_PUNCH, S_PUNCH1, 2 },  // Doom weaponinfo
  { S_PISTOL, S_PISTOL1, 2 },  // Doom weaponinfo
  { S_PISTOLFLASH, S_PISTOLFLASH, 2 },  // Doom weaponinfo
  { S_SGUN, S_SGUN1, 2 },  // Doom weaponinfo
  { S_SGUNFLASH1, S_SGUNFLASH1, 2 },  // Doom weaponinfo
  { S_DSGUN, S_DSGUN1, 2 },  // Doom weaponinfo
  { S_DSGUNFLASH1, S_DSGUNFLASH1, 2 },  // Doom weaponinfo
  { S_CHAIN, S_CHAIN1, 2 },  // Doom weaponinfo
  { S_CHAINFLASH1, S_CHAINFLASH1, 2 },  // Doom weaponinfo
  { S_MISSILE, S_MISSILE1, 2 },  // Doom weaponinfo
  { S_MISSILEFLASH1, S_MISSILEFLASH1, 2 },  // Doom weaponinfo
  { S_PLASMA, S_PLASMA1, 2 },  // Doom weaponinfo
  { S_PLASMAFLASH1, S_PLASMAFLASH1, 2 },  // Doom weaponinfo
  { S_BFG, S_BFG1, 2 },  // Doom weaponinfo
  { S_BFGFLASH1, S_BFGFLASH1, 2 },  // Doom weaponinfo
  { S_BFGSHOT, S_BFGEXP4, 1 },  // p_fab TRANSLU_ changes
  { S_PLAY, S_PLAY, 1 },  // Doom A_WeaponReady
  { S_PLAY_ATK1, S_PLAY_ATK2, 1 },  // Doom A_WeaponReady, A_GunFlash, A_FirePistol, A_FireShotgun, A_FireShotgun2, etc..
//  { S_DSNR1, S_DSNR1, 2 },  // Doom A_CheckReload
#define NUM_DOOM_state_trace  19

// Heretic
  { S_STAFFREADY, S_STAFFREADY2_1, 2 },  // Heretic weaponinfo
  { S_STAFFDOWN2, S_STAFFUP2, 2 },  // Heretic weaponinfo
  { S_STAFFATK1_1, S_STAFFATK1_1, 2 },  // Heretic weaponinfo
  { S_STAFFATK2_1, S_STAFFATK2_1, 2 },  // Heretic weaponinfo
  { S_CRBOW1, S_CRBOW1, 2 },  // Heretic weaponinfo
  { S_CRBOWDOWN, S_CRBOWATK1_1, 2 },  // Heretic weaponinfo
  { S_CRBOWATK2_1, S_CRBOWATK2_1, 2 },  // Heretic weaponinfo
  { S_BLASTERREADY, S_BLASTERATK1_3, 2 },  // Heretic weaponinfo
  { S_BLASTERATK2_1, S_BLASTERATK2_3, 2 },  // Heretic weaponinfo
  { S_MACEREADY, S_MACEATK1_2, 2 },  // Heretic weaponinfo
  { S_MACEATK2_1, S_MACEATK2_2, 2 },  // Heretic weaponinfo
  { S_HORNRODREADY, S_HORNRODATK1_1, 2 },  // Heretic weaponinfo
  { S_HORNRODATK2_1, S_HORNRODATK2_1, 2 },  // Heretic weaponinfo
  { S_GOLDWANDREADY, S_GOLDWANDATK1_1, 2 },  // Heretic weaponinfo
  { S_GOLDWANDATK2_1, S_GOLDWANDATK2_1, 2 },  // Heretic weaponinfo
  { S_PHOENIXREADY, S_PHOENIXATK1_1, 2 },  // Heretic weaponinfo
  { S_PHOENIXATK2_1, S_PHOENIXATK2_2, 2 },  // Heretic weaponinfo,
  { S_PHOENIXATK2_4, S_PHOENIXATK2_4, 2 },  // Heretic A_FirePhoenixPL2
  { S_GAUNTLETREADY, S_GAUNTLETUP, 2 },  // Heretic weaponinfo
  { S_GAUNTLETATK1_1, S_GAUNTLETATK1_1, 2 },  // Heretic weaponinfo
  { S_GAUNTLETREADY2_1, S_GAUNTLETREADY2_1, 2 },  // Heretic weaponinfo
  { S_GAUNTLETDOWN2, S_GAUNTLETUP2, 2 },  // Heretic weaponinfo
  { S_GAUNTLETATK1_1, S_GAUNTLETATK1_3, 2 },  // Heretic weaponinfo
  { S_GAUNTLETATK2_1, S_GAUNTLETATK2_3, 2 },  // Heretic weaponinfo
  { S_BEAKREADY, S_BEAKATK2_1, 2 },  // Heretic weaponinfo, P_ActivateBeak
  { S_PLAY_ATK1, S_PLAY_ATK2, 1 },  // Heretic P_FireWeapon
  { S_HRODFX1_2, S_HRODFX1_2, 1 },  // Heretic FireSkullRodPL1
  { S_RAINAIRXPLR1_1, S_RAINAIRXPLR1_1+8, 1 },  // Heretic A_RainImpact
  { S_CHICPLAY, S_CHICPLAY, 1 },  // Heretic P_BeakReady
  { S_CHICPLAY_ATK1, S_CHICPLAY_ATK1, 1 },  // Heretic P_BeakReady
#define NUM_Heretic_state_trace  30
};

void DEH_state_trace ()
{
    // If a state chain has more than 200 states, we are not going to follow it all.
#define VISITED_SIZE  200
    uint16_t visited[ VISITED_SIZE + 1 ]; // array of visited
    uint16_t visited_next = 0;

    uint16_t st_err_cnt = 0;
    uint16_t k1 = 0;
    uint16_t k2 = NUM_DOOM_state_trace;
    if( EN_heretic )
    {
        k1 = k2;
        k2 += NUM_Heretic_state_trace;
    }

    if( verbose )
      GenPrintf(EMSG_info, "DEH state verify\n" );

    int i1;
    for( i1= k1; i1< k2; i1++ )
    {
        fixed_state_t * fs = & fixed_state_table[i1];
        byte acpx = fs->acp;
        uint16_t si1, si;
        for( si1= fs->state1; si1 < fs->state2; si1++ )
        {
            visited_next = 0;
            // follow chain of states
            si = si1;
            for(;;) {
                // Check if have visited this state already (have a loop)
                int k;
                for( k = 0; k < visited_next; k++ )
                {
                    if( visited[k] == si )  // we have visited this state already, circular loop
                        goto chain_done;
                }
                visited[visited_next++] = si;  // put into visited
                if( visited_next >= VISITED_SIZE )    // safety test, should have found end or loop
                   goto chain_done;


                state_t * sp = & states[si];
                action_fi_t ai = sp->action;
                byte siap = action_acp(ai);
                if( siap && (siap != acpx) )
                {
                    st_err_cnt++;
                    deh_error( "DEH: state %i, has action %s acp(%i), chain from state %i acp(%i)\n",
                               si, lookup_action_name( ai ), siap, fs->state1, acpx );
                }

                si = sp->nextstate; // next state in chain
            }

    chain_done:
            if( visited_next >= VISITED_SIZE )
            {
                st_err_cnt++;
                GenPrintf( EMSG_warn, "DEH: state %i, has chain longer than %i states\n", visited_next );
            }
        }
    }
    if( verbose )
      GenPrintf(EMSG_info, "DEH states verified, %i errors\n", st_err_cnt );
}


// Dehacked post processing.
// This is called for each file with a dehacked lump.
void DEH_Finalize( void )
{
    // [WDJ] DSDA also checks that the number of parameters does not exceed what is valid for the action function.
    // Args that refer to thing numbers need to be translated. (Now done in lookup_fix_args).

    // Apply default code pointer parameters.  Translate some of the parameters.
    int i;
    for (i=0;i<NUMSTATES_EXT;i++)
    {
        state_t * st = & states[i];
        if( st->action )  // any action function
        {
#ifdef MBF21
            fix_action_args( st->action, st );
#endif
            fix_action_parm( st->action, st );
            check_invalid_state( i, st );
        }
    }

    DEH_state_trace ();

#ifdef ENABLE_DEH_REMAP
#else
    if( verbose && blanked_flag )
    {
        if( blanked_flag & 0x01 )
            GenPrintf(EMSG_info, "DEH free states blanked\n" );
        if( blanked_flag & 0x02 )
            GenPrintf(EMSG_info, "DEH Heretic states blanked\n" );
    }
#endif

    report_detect( "DEH/BEX", deh_key_detect );
    report_detect( "DEH Things", deh_thing_detect );  // BDTC codes
    deh_detect |= deh_key_detect | deh_thing_detect;
    all_detect |= deh_detect;
    
    if( deh_key_detect & BDTC_dehextra )
        GenPrintf(EMSG_info, "DEH DEHEXTRA detected\n" );
    if( deh_key_detect & BDTC_legacy )
        GenPrintf(EMSG_info, "DEH DoomLegacy extension detected\n" );
    if( deh_key_detect & BDTC_acp2 )
        GenPrintf(EMSG_info, "DEH Player weapon state changes detected\n" );
    
#ifdef MBF21
    if( all_detect & BDTC_MBF21 )
    {
        EN_mbf21 = 1;
        EN_mbf21_action = 1;
    }
#endif
}

#ifdef BEX_LANGUAGE
#include <fcntl.h>
#include <unistd.h>

// Load a language BEX file, by name (french, german, etc.)
void BEX_load_language( char * langname, byte bex_permission )
{
    int  handle;
    int  bytesread;
    struct stat  bufstat;
    char filename[MAX_WADPATH];
    myfile_t f;

    f.data = NULL;
    if( langname == NULL )
       langname = "lang";  // default name
    strncpy(filename, langname, MAX_WADPATH);
    filename[ MAX_WADPATH - 6 ] = '\0';
    strcat( filename, ".bex" );
    handle = open (filename,O_RDONLY|O_BINARY,0666);
    if( handle == -1)
    {
        CONS_Printf( "Lang file not found: %s\n", filename );
        goto errexit;
    }
    fstat(handle,&bufstat);
    f.size = bufstat.st_size;
    f.data = malloc( f.size );
    if( f.data == NULL )
    {
        CONS_Printf("Lang file too large for memory\n" );
        goto errexit;
    }
    bytesread = read (handle, f.data, f.size);
    if( bytesread < f.size )
    {
        CONS_Printf( "Lang file read failed\n" );
        goto errexit;
    }
    f.curpos = f.data;
    f.data[f.size] = 0;
    DEH_LoadDehackedFile( &f, bex_permission );

   errexit:
    free( f.data );
}
#endif
