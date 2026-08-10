// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: t_array.h 1112 2014-06-03 21:54:41Z smite-meister $
//
// Copyright(C) 2000 James Haley
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
// $Log: t_array.h,v $
// Revision 1.1  2004/07/27 08:22:01  exl
// Add fs arrys files
//
//
//--------------------------------------------------------------------------

#ifndef T_ARRAY_H
#define T_ARRAY_H

#include "t_parse.h"

void T_Add_FSArray(fs_array_t *);
void T_Init_FSArrayList(void);

//#define FS_ARRAYLIST_STRUCTHEAD
// The structure head seems to be unused, can just be a ptr.
#ifdef FS_ARRAYLIST_STRUCTHEAD
extern fs_array_t fs_arraylist;
#else
extern fs_array_t * fs_arraylist;
#endif

#endif
