// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_vari.h 1368 2017-11-01 01:17:48Z wesleyjohnson $
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
// $Log: t_vari.h,v $
// Revision 1.3  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.2  2003/05/30 22:44:07  hurdler
// add checkcvar function to FS
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------


#ifndef T_VARIABLE_H
#define T_VARIABLE_H

#include "t_parse.h"
  // m_fixed.h, p_mobj.h

// hash the variables for speed: this is the hashkey

#define variable_hash(n)                \
              (   ( (n)[0] + (n)[1] +   \
                   ((n)[1] ? (n)[2] +   \
                   ((n)[2] ? (n)[3]  : 0) : 0) ) % VARIABLESLOTS )

// fs_variable_t
struct fs_variable_s
{
  char *name;
  int type;       // vt_string or vt_int: same as in fs_value_t
  union
  {
    int32_t    i;
    fixed_t    fixed;
    char      *s;
    mobj_t    *mobj;
    fs_array_t *a;           // arrays
    
    char **pS;              // pointer to game string
    int *pI;                // pointer to game int
    fixed_t *pFixed;
    mobj_t **pMobj;         // pointer to game obj
    double *pf;
    fs_array_t **pA;         // arrays
    
    void (*handler)();      // for functions
    char *labelptr;         // for labels
  } value;
  fs_variable_t *next;       // for hashing
};

// variable types

enum
{
  FSVT_string,
  FSVT_int,
  FSVT_fixed,
  FSVT_mobj,         // a map object
  FSVT_function,     // functions are stored as variables
  FSVT_label,        // labels for goto calls are variables
  FSVT_const,        // const
  FSVT_array,        // array
  FSVT_pInt,         // pointer to game int
  FSVT_pFixed,
  FSVT_pString,      // pointer to game string
  FSVT_pMobj,        // pointer to game mobj
  FSVT_pArray,       // haleyjd: 05/27: pointer to game array
};

// variables

void T_Clear_HubScript();

void T_Init_variables();
fs_variable_t * new_variable(script_t *script, const char *name, int vtype);
fs_variable_t * find_variable(const char *name);
fs_variable_t * variable_for_name(script_t *script, const char *name);
fs_value_t getvariablevalue(fs_variable_t *v);
void setvariablevalue(fs_variable_t *v, fs_value_t newvalue);
void clear_variables(script_t *script);

fs_variable_t * add_game_int(const char *name, int *var);
fs_variable_t * add_game_string(const char *name, char **var);
fs_variable_t * add_game_mobj(const char *name, mobj_t **mo);

// functions

fs_value_t evaluate_function(int start, int stop);   // actually run a function
fs_variable_t * new_function(const char *name, void (*handler)() );

// arguments to handler functions

#define MAXARGS 128
extern int t_argc;
extern fs_value_t * t_argv;
extern fs_value_t t_return;

#endif
