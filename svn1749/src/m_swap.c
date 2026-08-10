// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_swap.c 1112 2014-06-03 21:54:41Z smite-meister $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2010 by DooM Legacy Team.
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
// $Log: m_swap.c,v $
// Revision 1.3  2001/02/24 13:35:20  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

// Not needed with little endian, unless the BE_SWAP* get used.
// __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#ifdef __BIG_ENDIAN__

#include "m_swap.h"

// [WDJ] Swap as callable functions.
// Reduces executable bloat and is not a speed issue for most of wad reading.
int16_t swap_int16( uint16_t x)
{
    return (int16_t)
     (  (( x & (uint16_t)0x00ffU) << 8)
      | (( x & (uint16_t)0xff00U) >> 8)
     );
}

int32_t swap_int32( uint32_t x)
{
    return (int32_t)
     (  (( x & (uint32_t)0x000000ffUL) << 24)
      | (( x & (uint32_t)0x0000ff00UL) <<  8)
      | (( x & (uint32_t)0x00ff0000UL) >>  8)
      | (( x & (uint32_t)0xff000000UL) >> 24)
     );
}

#endif
