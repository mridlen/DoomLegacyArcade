// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_parse.h 1633 2022-10-30 10:06:31Z wesleyjohnson $
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
// $Log: t_parse.h,v $
// Revision 1.5  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.4  2003/05/30 22:44:07  hurdler
// add checkcvar function to FS
//
// Revision 1.3  2001/08/06 23:57:10  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.2  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------


#ifndef T_PARSE_H
#define T_PARSE_H

#include "doomdef.h"
#include "p_mobj.h"
  // doomtype.h, m_fixed.h, mobj_t

#define T_MAXTOKENS 128
#define TOKENLENGTH 128
#define VARIABLESLOTS 16


typedef struct fs_array_s fs_array_t;
typedef struct script_s   script_t;
typedef struct fs_value_s fs_value_t;
typedef struct operator_s operator_t;
typedef struct fs_variable_s fs_variable_t;



struct fs_value_s
{
  int type;
  union
  {
    int32_t  i;
    fixed_t  f;
    const char  * s;
    mobj_t      * mobj;
    fs_array_t  * a;   // arrays
    char        * labelptr; // goto() label
  } value;
};


struct fs_array_s
{
   fs_array_t * next; // next array in save list
   int saveindex;	   // index for saving

   unsigned int length;	   // number of values currently initialized   
   fs_value_t * values;	   // array of contained values
};


#define intvalue(v)                                    \
  ( (v).type == FSVT_string ? atoi((v).value.s) :       \
    (v).type == FSVT_fixed ? (int)((v).value.f / FRACUNIT) : \
    (v).type == FSVT_mobj ? -1 : \
    (v).type == FSVT_array ? -1 : (v).value.i )

#if 1
// [WDJ] handle FSVT_int directly, not by including duplicate code
#define fixedvalue(v)                                         \
  ( (v).type == FSVT_fixed ? (v).value.f :                     \
    (v).type == FSVT_string ? (fixed_t)(atof((v).value.s) * FRACUNIT) : \
    (v).type == FSVT_mobj ? -1*FRACUNIT : \
    (v).type == FSVT_array ? -1*FRACUNIT : (v).value.i * FRACUNIT )
#else
#define fixedvalue(v)                                         \
  ( (v).type == FSVT_fixed ? (v).value.f :                     \
    (v).type == FSVT_string ? (fixed_t)(atof((v).value.s) * FRACUNIT) : \
    (v).type == FSVT_mobj ? -1*FRACUNIT : \
    (v).type == FSVT_array ? -1*FRACUNIT : intvalue(v) * FRACUNIT )
#endif



const char * stringvalue(fs_value_t v);

#include "t_vari.h"
#include "t_prepro.h"

#define MAXSCRIPTS 256

struct script_s
{
  // script data
  
  char * data;
  int scriptnum;  // this script's number
  int len;
  
  // {} sections
  fs_section_t * sections[SECTIONSLOTS];
  
  // variables:
  fs_variable_t * variables[VARIABLESLOTS];
  
  // ptr to the parent script
  // the parent script is the script above this level
  // eg. individual linetrigger scripts are children of the levelscript,
  // which is a child of the global_script
  script_t * parent;

  // child scripts.
  // levelscript holds ptrs to all of the level's scripts
  // here.
  
  script_t * children[MAXSCRIPTS];
  
  mobj_t * trigger;        // object which triggered this script

  //SoM: Used for if/elseif/else statements
  boolean  lastiftrue;
};

struct operator_s
{
  char * string;
  fs_value_t (*handler)(int, int, int); // left, mid, right
  int direction;
};

enum
{
  D_forward,
  D_backward
};

void run_script(script_t *script);
void continue_script(script_t *script, char *continue_point);
void parse_include(char *lumpname);
void run_statement( void );
void script_error(const char *fmt, ...);
void wrong_num_arg( const char * funcname, int num_args );
void missing_arg( const char * funcname, int min_num_args );
void missing_arg_str( const char * funcname, const char * argstr );

fs_value_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, const char *value);
int find_operator_backwards(int start, int stop, const char *value);

/******* tokens **********/

typedef enum
{
  TT_name,   // a name, eg 'count1' or 'frag'
  TT_number,
  TT_operator,
  TT_string,
  TT_unset,
  TT_function          // function name
} tokentype_t;

enum    // brace types: where current_section is a { or }
{
  BRACKET_open,
  BRACKET_close
};

extern fs_value_t nullvar;
extern byte script_debug;

extern script_t * fs_current_script;
extern mobj_t * fs_trigger_obj;
extern int fs_killscript;

extern char *tokens[T_MAXTOKENS];
extern tokentype_t tokentype[T_MAXTOKENS];
extern int num_tokens;
// [WDJ] Common fragglescript vars.
extern char * fs_src_cp;     // current point reached in parsing
extern char * fs_linestart_cp; // start of the current expression
extern fs_section_t * fs_current_section;
extern fs_section_t * fs_prev_section;
extern int fs_bracetype;

// the global_script is the root
// script and contains only built-in
// FraggleScript variables/functions

extern script_t global_script; 
extern script_t hub_script;

#endif
