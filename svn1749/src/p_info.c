// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_info.c 1654 2023-11-14 09:19:26Z wesleyjohnson $
//
// Copyright(C) 2000 Simon Howard
// Copyright (C) 2001-2011 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log: p_info.c,v $
// Revision 1.15  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.14  2001/08/14 00:36:26  hurdler
// Revision 1.13  2001/08/13 22:53:39  stroggonmeth
//
// Revision 1.12  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.11  2001/08/02 19:15:59  bpereira
// fix player reset in secret level of doom2
//
// Revision 1.10  2001/06/30 15:06:01  bpereira
// fixed wrong next level name in intermission
//
// Revision 1.9  2001/05/16 21:21:14  bpereira
// Revision 1.8  2001/05/07 20:27:16  stroggonmeth
// Revision 1.7  2001/03/21 18:24:38  stroggonmeth
//
// Revision 1.6  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.5  2000/11/11 13:59:45  bpereira
// Revision 1.4  2000/11/09 17:56:20  stroggonmeth
// Revision 1.3  2000/11/06 20:52:16  bpereira
//
// Revision 1.2  2000/11/03 11:48:39  hurdler
// Fix compiling problem under win32 with 3D-Floors and FragglScript (to verify!)
//
// Revision 1.1  2000/11/03 02:00:44  stroggonmeth
// Added p_info.c and p_info.h
//
//
//--------------------------------------------------------------------------
//
// Level info.
//
// Under smmu, level info is stored in the level marker: ie. "mapxx"
// or "exmx" lump. This contains new info such as: the level name, music
// lump to be played, par time etc.
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "dehacked.h"
#include "dstrings.h"
#include "p_setup.h"
#include "p_info.h"
#include "p_mobj.h"
#include "t_script.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_local.h"

// [MB] 2023-03-12: Size of static buffers used for names of levels, etc.
#define NAME_BUFSIZE  50


//----------------------------------------------------------------------------
//
// Helper functions
//

void P_LowerCase(char *line)
{
  char *temp;

  for(temp=line; *temp; temp++)
    *temp = tolower((unsigned char)*temp);
}

void P_StripSpaces(char *line)
{
  char *temp;

  temp = line+strlen(line)-1;

  while(*temp == ' ')
  {
      *temp = '\0';
      temp--;
  }
}

#if 0
// [WDJ] found to be unused, 12/5/2009
static void P_RemoveComments(char *line)
{
  char *temp = line;

  while(*temp)
  {
      if(*temp=='/' && *(temp+1)=='/')
      {
          *temp = '\0'; return;
      }
      temp++;
  }
}
#endif

#if 0
static void P_RemoveEqualses(char *line)
{
  char *temp;

  temp = line;

  while(*temp)
  {
      if(*temp == '=')
      {
          *temp = ' ';
      }
      temp++;
  }
}
#endif

//----------------------------------------------------------------------------
//
//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

#ifdef ENABLE_UMAPINFO
// [MB] 2023-01-22: Changed to const (used for UMAPINFO too)
const char * info_creator;
const char * info_music;
const char * info_skyname;
const char * info_interpic;
const char * info_levelpic;
#else
char * info_creator;
char * info_music;
char * info_skyname;
char * info_interpic;
char * info_levelpic;
#endif

char * info_levelname;
char * info_nextlevel;
char * info_nextsecret;
char * info_intertext = NULL;
char * info_backdrop;
char * info_weapons;

int info_partime;
int info_scripts;       // has the current level got scripts?
int gravity;

enum
{
  IVT_STRING,
  IVT_INT,
  IVT_CONSOLECMD, // SoM: W00t I RULE
  IVT_END
};

typedef struct
{
  int type;
  const char *name;
  void *variable;
} levelvar_t;

levelvar_t levelvars[]=
{
  {IVT_STRING,    "levelpic",     &info_levelpic},
  {IVT_STRING,    "levelname",    &info_levelname},
  {IVT_INT,       "partime",      &info_partime},
  {IVT_STRING,    "music",        &info_music},
  {IVT_STRING,    "skyname",      &info_skyname},
  {IVT_STRING,    "creator",      &info_creator},
  {IVT_STRING,    "interpic",     &info_interpic},
  {IVT_STRING,    "nextlevel",    &info_nextlevel},
  {IVT_STRING,    "nextsecret",   &info_nextsecret},
  {IVT_INT,       "gravity",      &gravity},
  {IVT_STRING,    "inter-backdrop",&info_backdrop},
  {IVT_STRING,    "defaultweapons",&info_weapons},
  {IVT_CONSOLECMD,"consolecmd",    NULL},
  {IVT_END,       0,              0}
};

void P_ParseLevelVar(char *cmd)
{
  char *equals;
  char *varname;
  levelvar_t* current;

  if(!*cmd) return;

  // right, first find the variable name
  // [WDJ] Replace code that could overrun strings.
  // This code does not copy, cannot overrun strings.
  // Ignore the equals sign, parse around it.
  varname = cmd; // varname is first
  // leading spaces already removed by caller
  equals = varname;
  while(*equals && *equals != ' ' && *equals != '=')  equals++;  // span the name
  *equals++ = '\0'; // terminate varname

  // find what it equals
  while(*equals && (*equals == ' ' || *equals == '='))  equals++;  // span the '='
  // it is valid for equals to be null string

  current = levelvars;

  while(current->type != IVT_END)
  {
      if(!strcasecmp(current->name, varname))
      {
          switch(current->type)
          {
            case IVT_STRING:
              // Just drop the previous string value, it may be const.
              // Recover such strings at end-level
              *(char**)current->variable         // +5 for safety
                = Z_Malloc(strlen(equals)+5, PU_LEVEL, NULL);
              strcpy(*(char**)current->variable, equals);
              break;

            case IVT_INT:
              *(int*)current->variable = atoi(equals);
              break;

            case IVT_CONSOLECMD:
              {
                // consolecmd = <string>
                // pass the string as a console command
                char t[256];
                snprintf(t, 255, "%s\n", equals);
                t[255] = '\0';
                COM_BufAddText(t);
              }
              break;
          }
          break; // exit loop, only one name will match
      }
      current++;
  }
}


// clear all the level variables so that none are left over from a
// previous level

int isExMy(char * name)
{
//  if(strlen(name) != 4)  goto not_ExMy;  // redundant
  if( name[4] != '\0' )  // this also confirms strlen
    goto not_ExMy;
  if( (name[0] != 'E' && name[0] != 'e') || (name[2] != 'M' && name[2] != 'm') )
    goto not_ExMy;
  if( !isnumchar(name[1]) || !isnumchar(name[3]) )
    goto not_ExMy;

  return 1;

not_ExMy:
  return 0;
}

int isMAPxy(char * name)
{
//  if(strlen(name) != 5)  goto not_map;  // redundant
  if( name[5] != '\0' )  // this also confirms strlen
    goto not_map;
  if( (name[0] != 'M' && name[0] != 'm') || (name[1] != 'A' && name[1] != 'a') || (name[2] != 'P' && name[2] != 'p') )
    goto not_map;
  if( !isnumchar(name[3]) || !isnumchar(name[4]) )
    goto not_map;

  return 1;

not_map:
  return 0;
}

void P_Clear_LevelVars(void)
{
  info_levelname = "";
  info_skyname = "";
  info_levelpic = "";
  info_interpic = "";
  info_music = "";
  info_creator = "unknown";
  info_partime = -1;

  if(gamemode == doom2_commercial && isExMy(level_mapname))
  {
    static char nextlevel[10];
    info_nextlevel = nextlevel;

    // set the next episode
    strcpy(nextlevel, level_mapname);
    nextlevel[3] ++;
    if(nextlevel[3] > '9')  // next episode
    {
      nextlevel[3] = '1';
      nextlevel[1] ++;
    }

    info_music = level_mapname;
  }
  else
    info_nextlevel = "";

  info_nextsecret = "";

  info_weapons = "";
  gravity = FRACUNIT;     // default gravity

  if(info_intertext)
  {
    Z_Free(info_intertext);
    info_intertext = NULL;
  }

  info_backdrop = NULL;

  T_Clear_Scripts();
  info_scripts = false;
}

//----------------------------------------------------------------------------
//
// P_ParseScriptLine
//
// FraggleScript: if we are reading in script lines, we add the new lines
// into the levelscript
//

//SoM: Dynamic limit set by lumpsize...
int   maxscriptsize = -1;

void P_ParseScriptLine(char *line)
{
  // [WDJ] FIXME ???
  if(fs_levelscript.data[0] == 0)
  {
    Z_Free(fs_levelscript.data);
    fs_levelscript.data = Z_Malloc(maxscriptsize, PU_LEVEL, 0);
    fs_levelscript.data[0] = '\0';
  }

#ifdef DEBUGFILE
  if( debugfile )
     fprintf( debugfile, "SL: %s\n", line );
#endif

  int lslen = strlen(fs_levelscript.data);
  int slen = strlen(line);

  // [WDJ] account for \n and 0 term
  if( (lslen+slen+2) > maxscriptsize)
  {
    I_SoftError("Script is larger than levelscript buffer,  %i char line rejected.\n", slen);
    return;
  }

  // add the new line to the current data
  // (ugh) sprintf(fs_levelscript.data, "%s%s\n", fs_levelscript.data, line);
  memcpy(&fs_levelscript.data[lslen], line, slen);
  fs_levelscript.data[lslen + slen ] = '\n';
  fs_levelscript.data[lslen + slen + 1] = '\0';
}

//-------------------------------------------------------------------------
//
// P_ParseInterText
//
// Add line to the custom intertext
//

void P_ParseInterText(char *line)
{
  while(*line==' ') line++;
  if(!*line) return;

  // [WDJ] Add safety term, use safe copy.
  if(info_intertext)
  {
    int textlen = strlen(info_intertext);
    int avail = maxscriptsize - textlen;
    if( avail > 2 )
    {
      // newline
      info_intertext[textlen] = '\n';
      textlen ++;
      avail --;

      // add line to end
      strncpy(&info_intertext[textlen], line, avail);
    }
  }
  else
  {
    // allocate 1 extra for safety term
    info_intertext = Z_Malloc(maxscriptsize+1, PU_STATIC, 0);
    info_intertext[maxscriptsize-1] = 0;  // overrun detector
    strncpy(info_intertext, line, maxscriptsize);
  }
  info_intertext[maxscriptsize] = 0;  // safety term
   
  if( info_intertext[maxscriptsize-1] != 0 )  // overrun detected
  {
    I_SoftError("Intermission text overruns buffer, truncated.\n");
  }
}

//---------------------------------------------------------------------------
//
// Setup/Misc. Functions
//

boolean default_weaponowned[NUMWEAPONS];

// Init weapon positions, dependent upon info.
void P_InitWeapons(void)
{
  char *s;

  memset(default_weaponowned, 0, sizeof(default_weaponowned));

  s = info_weapons;

  while(*s)
  {
      switch(*s)
      {
        case '3': default_weaponowned[wp_shotgun] = true; break;
        case '4': default_weaponowned[wp_chaingun] = true; break;
        case '5': default_weaponowned[wp_missile] = true; break;
        case '6': default_weaponowned[wp_plasma] = true; break;
        case '7': default_weaponowned[wp_bfg] = true; break;
        case '8': default_weaponowned[wp_supershotgun] = true; break;
        default: break;
      }
      s++;
  }
}

#if 0
// [WDJ] 8/26/2011 moved inline
//SoM: Moved from hu_stuff.c
#define HU_TITLE  (text[HUSTR_E1M1_NUM + (gameepisode-1)*9+gamemap-1])
#define HU_TITLE2 (text[HUSTR_1_NUM + gamemap-1])
#define HU_TITLEP (text[PHUSTR_1_NUM + gamemap-1])
#define HU_TITLET (text[THUSTR_1_NUM + gamemap-1])
#define HU_TITLEH (text[HERETIC_E1M1_NUM + (gameepisode-1)*9+gamemap-1])
#endif

static char * levelname;

// Determine game specific level name.
// From P_SetupLevel, level_mapname.
// Called by P_Load_LevelInfo
void P_FindLevelName(void)
{
  // Determine the level name.
  // There are a number of sources from which it can come from,
  // getting the right one is the tricky bit =).
  
  if(*info_levelname)
  {
      // info level name from level lump (p_info.c) ?
      levelname = info_levelname;
  }
  else if(!newlevel || deh_loaded)
  {
      // not a new level or dehacked level names
      if( gamemode == heretic )
      {
        // Heretic shareware, Heretic, Blasphemer
        levelname = text[HERETIC_E1M1_NUM + (gameepisode-1)*9+gamemap-1];
      }
      else if(isMAPxy(level_mapname))
      {
        switch( gamedesc_id )
        {
         case GDESC_plutonia:
           // Plutonia
           levelname = text[PHUSTR_1_NUM + gamemap-1];
           break;
         case GDESC_tnt:
           // TNT
           levelname = text[THUSTR_1_NUM + gamemap-1];
           break;
         default:
           // Doom2, FreeDoom, FreeDM
           levelname = text[HUSTR_1_NUM + gamemap-1];
           break;
        }
      }
      else if(isExMy(level_mapname))
      {
        // Doom shareware, Doom, UltDoom, UltFreeDoom, ChexQuest
        levelname = text[HUSTR_E1M1_NUM + (gameepisode-1)*9+gamemap-1];
      }
      else
        levelname = level_mapname;
  }
  else        //  otherwise just put "new level"
  {
      static char newlevelstr[50];

      sprintf(newlevelstr, "%s: new level", level_mapname);
      levelname = newlevelstr;
  }
}

//-------------------------------------------------------------------------
//
// P_ParseInfoCmd
//
// We call the relevant function to deal with the line we are given,
// based on info_readtype. If we get a section divider ([] bracketed) we
// change readtype.
//

enum
{
  RT_LEVELINFO,
  RT_SCRIPT,
  RT_OTHER,
  RT_INTERTEXT
} info_readtype;   // global parameter to I_ParseInfoCmd


void P_ParseInfoCmd(char *line)
{
  if(!*line)
    return;

  if(info_readtype != RT_SCRIPT)       // not for scripts
  {
      //      P_LowerCase(line);
      while(*line == ' ') line++;
      if(!*line) return;
      if((line[0] == '/' && line[1] == '/')     // comment
         || line[0] == '#' || line[0] == ';')   return;
  }

  if(*line == '[')                // a new section seperator
  {
      line++;
      if(!strncasecmp(line, "level info", 10))
        info_readtype = RT_LEVELINFO;
      else if(!strncasecmp(line, "scripts", 7))
      {
        info_readtype = RT_SCRIPT;
        info_scripts = true;    // has scripts
      }
      else if(!strncasecmp(line, "intertext", 9))
        info_readtype = RT_INTERTEXT;
      return;
  }

  // Parse the line. May alter line if necessary.
  switch(info_readtype)
  {
    case RT_LEVELINFO:
      P_ParseLevelVar(line);
      break;

    case RT_SCRIPT:
      P_ParseScriptLine(line);
      break;

    case RT_INTERTEXT:
      P_ParseInterText(line);
      break;

    case RT_OTHER:
      break;
  }
}

//-------------------------------------------------------------------------
//
// P_Load_LevelInfo
//
// Load the info lump for a level. Call P_ParseInfoCmd for each
// line of the lump.

// Dependent upon level_mapname, level_lumpnum.
void P_Load_LevelInfo( void )
{
  char      *lump;
  char      *endlump_cp;
  char      *readline;
  char      *rlp;
  int       lumpsize;

  // [WDJ] Fix to use char ptrs, and to use normal char append.
  // Replace operations that could overrun strings.
  info_readtype = RT_OTHER;  // global to I_ParseInfoCmd
  P_Clear_LevelVars();  // clear vars and init levelscript

  lumpsize = maxscriptsize = W_LumpLength(level_lumpnum);
  readline = Z_Malloc(lumpsize + 1, PU_IN_USE, 0);
  rlp = &readline[0];

  if(lumpsize > 0)
  {
    fs_src_cp = lump = W_CacheLumpNum(level_lumpnum, PU_IN_USE);  // level info
    endlump_cp = lump + lumpsize; // end of lump
    while(fs_src_cp < endlump_cp)
    {
        register unsigned char ch = *fs_src_cp;
        if(ch == '\n') // end of line
        {
          *rlp = '\0';
          P_ParseInfoCmd(readline);  // parse line
          rlp = &readline[0];  // clear for next line
        }
        else
        {
          // add to line if valid char
          if(isprint(ch) || ch == '{' || ch == '}')
          {
            *rlp++ = ch; // add char
          }
        }
        fs_src_cp++;
    }

    // parse last line
    *rlp = '\0';
    P_ParseInfoCmd(readline);
    Z_Free(lump);
  }
  Z_Free(readline);

  // These are affected by P_Clear_LevelVars().
  // Init weapon positions, dependent upon weapons info.
  P_InitWeapons();
  // Determine game specific level name.
  // Dependent upon info level vars and level_mapname.
  P_FindLevelName();

  //Set the gravity for the level!
  if( cv_gravity.value != gravity )
  {
      if(gravity != FRACUNIT)
          COM_BufAddText(va("gravity %f\n", ((double)gravity) / 100));
      else
          COM_BufAddText(va("gravity %f\n", ((double)gravity) / FRACUNIT));
  }

  COM_BufExecute( CFG_none ); //Hurdler: flush the command buffer
  return;
}

//-------------------------------------------------------------------------
//
// Console Commands
//

void COM_MapInfo_f(void)
{
#ifdef ENABLE_UMAPINFO
  // [MB] 2023-02-04: Call P_LevelName() to catch levelname from UMAPINFO
  CONS_Printf("Name: %s\n", P_LevelName());
#else
  CONS_Printf("Name: %s\n", levelname);
#endif
  CONS_Printf("Author: %s\n", info_creator);
}


void P_Register_Info_Commands(void)
{
  COM_AddCommand("mapinfo",  COM_MapInfo_f, CC_info);
}


#ifdef ENABLE_UMAPINFO
// [WDJ] If it must be static anyway, then put it where everybody can access it.
static char  umapinfo_level_str[NAME_BUFSIZE];


// [MB] 2023-03-12: Get label (prefix) for UMAPINFO levelname
// Helper function for P_LevelName()
// Puts the label in umapinfo_level_str.
//
// Returns the length of the label, even if larger than buffer size
// (but does not write beyond end of buffer).
static
int  P_LevelName_umapinfo_getlabel(void)
{
    int len = 0;

    // [WDJ] This will never print into just any buffer,
    // so always print into the only possible dest, umapinfo_level_str.
    if( game_umapinfo && game_umapinfo->label )
    {
        // Empty string, if "clear" was specified in UMAPINFO
        const char * label = game_umapinfo->label;

        // When is this used, and how long is it likely to be.  Give example.
        len = snprintf(umapinfo_level_str, NAME_BUFSIZE, "%s", label);
    }
    else
    {
        // Quoted from UMAPINFO specification Rev 2.2 definition of map
        // entry:
        // The [episode and map] numbers [...] can exceed their original
        // limits, though, so MAP50, E5M6 or even MAP100 or E1M10 are valid
        // map names for their respective game.
        //
        // Quoted from UMAPINFO specification Rev 2.2 definition of label:
        // If not specified the mapname will be used by default followed by
        // a colon and a space character (e.g. "E1M1: ").

#if 0
        // does nothing, they are already byte, data hiding confuses what they are and makes me have to search where they were defined
        byte e = gameepisode;
        byte m = gamemap;
#endif

        if( gamemode == doom2_commercial )
// [WDJ] Why is this "level 14" and not "MAP 14", or better "MAP14".
//            len = snprintf(umapinfo_level_str, NAME_BUFSIZE, "level %u: ", m);
            len = snprintf(umapinfo_level_str, NAME_BUFSIZE, "MAP%u: ", gamemap);
        else
            len = snprintf(umapinfo_level_str, NAME_BUFSIZE, "E%uM%u: ", gameepisode, gamemap);
    }
        
    if( len < 0 ) // snprintf error
        len = 0;  // the appropriate return value

    // Only this function knows why this happens.
#if 1
    // Allow only half of the buffer (25 char) for the label.
    // This exceeds anything reasonable for the label.
    if( len > NAME_BUFSIZE/2 )  // limiting usage, ignore truncated
        len = NAME_BUFSIZE/2;
#else
    // Leave room for next append to this buffer, so do not get negative remaining.
    if( len > NAME_BUFSIZE-2 )  // truncated, not going to happen
        len = NAME_BUFSIZE-2;
#endif
   
    umapinfo_level_str[len] = 0;  // proper term.

    // [WDJ] Cannot trust external umapinfo to be sane.
    umapinfo_level_str[NAME_BUFSIZE-1] = 0;

    return len;  // when label too long, will be == NAME_BUFSIZE-1, but nobody cares.
}
#endif
 
char * P_LevelName(void)
{
#ifdef ENABLE_UMAPINFO
    // [MB] 2023-01-22: Support for UMAPINFO added
    if( game_umapinfo && game_umapinfo->levelname )
    {
        int label_len = P_LevelName_umapinfo_getlabel(); // into umapinfo_level_str
            // Reports accurate usage of buffer.
            // If truncated, nobody cares, and really should never happen.

        int remaining = (NAME_BUFSIZE - 1) - label_len;  // has NUL-termination,  should not be negative

#if 0
            // If too long, then label_len will be > NAME_BUFSIZE-1.  Indicates truncation.
// [WDJ] This does not make use of label_len > NAME_BUFSIZE as a signal.
// Just reporting the truncated length accurately would do exactly the same thing.
// None of this cares that it was truncated, and does nothing special about it.
        if( label_len > remaining )
            label_len = remaining;
        remaining -= label_len;
#endif

        // Avoid two var names that only differ by ONE letter.  Keep this easy to read.
        // Using size_t would break if calculation ever went negative.  Proof against future changes.
        int levlen = strlen(game_umapinfo->levelname);

        if( levlen > remaining )
            levlen = remaining;

        if( levlen > 0 )
        {
            memcpy(&umapinfo_level_str[label_len], game_umapinfo->levelname, (size_t) levlen);
            umapinfo_level_str[label_len + levlen] = 0;
        }
        // [WDJ] Cannot trust external umapinfo to be sane.
        umapinfo_level_str[NAME_BUFSIZE-1] = 0;  // redundant safety
        return umapinfo_level_str;
    }
#endif
    // standard Doom
    return levelname;
}
 

// todo : make this use mapinfo lump
// Called by WI_Draw_EL "Entering <LevelName>"
// Called by IN_Draw_YAH "NOW ENTERING"
const char * P_LevelNameByNum( int episode, int map )
{
    register const char * levnam = "New map";
    switch(gamemode) 
    {
       case  doom_shareware :
       case  doom_registered :
       case  ultdoom_retail :  // and UltFreeDoom
       case  chexquest1 :
           levnam = text[HUSTR_E1M1_NUM + (episode-1)*9+map-1];
           break;
       case  doom2_commercial :  // and FreeDoom, FreeDM
           switch( gamedesc_id )
           {
               case GDESC_tnt :
                   levnam = text[THUSTR_1_NUM + map-1];
                   break;
               case GDESC_plutonia :
                   levnam = text[PHUSTR_1_NUM + map-1];
                   break;
               default :
                   levnam = text[HUSTR_1_NUM + map-1];
                   break;
           }
           break;
       case  heretic:  // and shareware, blasphemer
           levnam = text[HERETIC_E1M1_NUM + (episode-1)*9+map-1];
           break;
       case  hexen:
           break;
       default:
           break;
    }
    return levnam;
}
