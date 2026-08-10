// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_prepro.h 1548 2020-09-29 10:26:04Z wesleyjohnson $
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
// $Log: t_prepro.h,v $
// Revision 1.4  2003/05/04 04:19:06  sburke
// Don't typedef label_t on Solaris.
//
// Revision 1.3  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.2  2001/05/16 22:33:34  bock
// Initial FreeBSD support.
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//--------------------------------------------------------------------------


#ifndef T_PREPRO_H
#define T_PREPRO_H


#define SECTIONSLOTS 17
#define LABELSLOTS 17


void preprocess(script_t *script);

/***** {} sections **********/
typedef struct fs_section_s fs_section_t;

struct fs_section_s
{
  char *start;    // offset of starting brace {
  char *end;      // offset of ending brace   }
  int type;       // section type: for() loop, while() loop etc
  
  union
  {
    struct
    {
      char *loopstart;  // positioned before the while()
    } data_loop;
  } data; // data for section
  
  fs_section_t *next;        // for hashing
};

fs_section_t * find_section_start(char *brace);
fs_section_t * find_section_end(char *brace);


typedef enum    // section types
{
  FSST_empty,       // none: empty {} braces
  FSST_if,          // if() statement
  FSST_elseif,      // elseif() statement
  FSST_else,        // else() statement
  FSST_loop,        // loop
} fs_section_e;

/****** goto labels ***********/

#if 0
// Unused, no body.
// label_t is not present on Linux, or other major OS.

#ifdef FREEBSD
#if __FreeBSD__ < 5
#include <machine/types.h>
#endif
#if __FreeBSD__ > 4
typedef struct label_s label_t;
#endif
#endif

#if !defined(FREEBSD) && !defined(SOLARIS)
typedef struct label_s label_t;
#endif

label_t * labelforname(char *labelname);
#endif

#endif
