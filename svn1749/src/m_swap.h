// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_swap.h 1112 2014-06-03 21:54:41Z smite-meister $
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
// $Log: m_swap.h,v $
// Revision 1.5  2003/05/07 03:03:03  sburke
// Make the SHORT and LONG macros cast their value to short and long.
//
// Revision 1.4  2001/04/17 22:26:07  calumr
// Initial Mac add
//
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

#ifndef M_SWAP_H
#define M_SWAP_H

#include <stdint.h>

// WAD files are always little-endian.
// Other files, such as MIDI files, are always big-endian.

#if 1
#define SWAP_INT16_FAST(x) ((int16_t)( \
(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) | \
(((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ))

#define SWAP_INT32_FAST(x) ((int32_t)( \
(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) | \
(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) | \
(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) | \
(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))

#else
// [WDJ] as inline functions, which will show a difference if there are any
// uses with executable parameters.  Compiler will optimize
// them to nearly the same thing, except for parameter handling.
// Inline func are always safer in unfamiliar code.
static inline int16_t SWAP_INT16_FAST( uint16_t x)
{
    return (int16_t)
     (  (( x & (uint16_t)0x00ffU) << 8)
      | (( x & (uint16_t)0xff00U) >> 8)
     );
}

static inline int32_t SWAP_INT32_FAST( uint32_t x)
{
    return (int32_t)
     (  (( x & (uint32_t)0x000000ffUL) << 24)
      | (( x & (uint32_t)0x0000ff00UL) <<  8)
      | (( x & (uint32_t)0x00ff0000UL) >>  8)
      | (( x & (uint32_t)0xff000000UL) >> 24)
     );
}
#endif

// [WDJ] name changed to indicate SWAP, size, Endianness, and just so I can
// grep the old SHORT uses to find them.  To make 64bit clean, change all uses
// of SHORT and LONG; they are not 16 and 32 bit on some machines.
// (short) ===> (int16_t),  (long)  ===> (int32_t)
// 
// Use LE_SWAP* to convert to and from external little-endian value.
//    Wads, lumps, MUS format, TGA format, network
// Use BE_SWAP* to convert to and from external big-endian value.
//    Midi
// __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#ifdef __BIG_ENDIAN__

// [WDJ] swap functions, reduces executable bloat.
int16_t swap_int16( uint16_t x);
int32_t swap_int32( uint32_t x);

// [WDJ] Fast inline where must do swap during play (other than load).
# define LE_SWAP16_FAST(x)  SWAP_INT16_FAST(x)
# define LE_SWAP32_FAST(x)  SWAP_INT32_FAST(x)
# define LE_SWAP16(x)  swap_int16(x)
# define LE_SWAP32(x)  swap_int32(x)

# define BE_SWAP16_FAST(x)  (x)
# define BE_SWAP32_FAST(x)  (x)
//# define BE_SWAP16(x)  (x)
//# define BE_SWAP32(x)  (x)

#else // little-endian machine

// [WDJ] Fast inline where must do swap during play (other than load).
# define LE_SWAP16_FAST(x)  (x)
# define LE_SWAP32_FAST(x)  (x)
# define LE_SWAP16(x)  (x)
# define LE_SWAP32(x)  (x)

# define BE_SWAP16_FAST(x)  SWAP_INT16_FAST(x)
# define BE_SWAP32_FAST(x)  SWAP_INT32_FAST(x)
//# define BE_SWAP16(x)  swap_int16(x)
//# define BE_SWAP32(x)  swap_int32(x)

#endif

#endif
