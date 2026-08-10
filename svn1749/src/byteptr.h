// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: byteptr.h 1368 2017-11-01 01:17:48Z wesleyjohnson $
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
// $Log: byteptr.h,v $
// Revision 1.6  2003/05/04 02:26:39  sburke
// Fix problems in adapting to __BIG_ENDIAN__ machines.
//
// Revision 1.5  2000/10/21 08:43:28  bpereira
// Revision 1.4  2000/04/16 18:38:06  bpereira
//
//
// DESCRIPTION:
//    Macro to read/write from/to a char*, used for packet creation and such...
//
//-----------------------------------------------------------------------------

#ifndef BYTEPTR_H
#define BYTEPTR_H

#include <string.h>
  // memcpy
#include "m_swap.h"

// These are used in save game, network communications, and reading some wad lumps.

// TODO FIXME the reliance on specific sizes for longs, shorts etc in this file is like asking for horrible bugs

static inline int16_t read_16(byte **p)
{
  int16_t temp = *(int16_t *)*p;
  *p += sizeof(int16_t);
  return LE_SWAP16(temp);
}

static inline int32_t read_32(byte **p)
{
  int32_t temp = *(int32_t *)*p;
  *p += sizeof(int32_t);
  return LE_SWAP32(temp);
}


static inline void write_16(byte **p, int16_t val)
{
  *(int16_t *)*p = LE_SWAP16(val);
  *p += sizeof(int16_t);
}

static inline void write_32(byte **p, int32_t val)
{
  *(int32_t *)*p = LE_SWAP32(val);
  *p += sizeof(int32_t);
}

// These are used in d_netcmd, d_netfil, and p_saveg

// [WDJ] Change all short,long to stdint types.
#define WRITEBYTE(p,b)      *(p)++ = (b)
#define WRITECHAR(p,b)      *(p)++ = (byte)(b)
#define WRITE16(p,b)        write_16(&p, b)
#define WRITEU16(p,b)       write_16(&p, b)
#define WRITE32(p,b)        write_32(&p, b)
#define WRITEU32(p,b)       write_32(&p, b)
#define WRITEFIXED(p,b)     write_32(&p, b)
#define WRITEANGLE(p,b)     write_32(&p, b)
#define WRITEBOOLEAN(p,b)   *(p)++ = (b?1:0)

// [WDJ]
// Put {} around all stmt macros with more than one line to protect against
// use as body of if,while, etc..
// Would make these inline functions, but cannot because they
// modify their parameters.
#define WRITEMEM(p,s,n)     { memcpy((p),(s),(n)); (p)+=(n); }

// Replace complicated WRITESTRING macros, with proper functions.
// Implemented in d_netcmd.c
byte *  write_string( byte *dst, const char* src);
byte *  write_stringn( byte *dst, const char* src, int num );


// [WDJ] Put () around all macros that return values, to ensure closure in expr.
// Change all short,long to stdint types.
#define READBYTE(p)         (*(p)++)
#define READCHAR(p)         ((char)*(p)++)
#define READ16(p)           ((int16_t)read_16(&p))
#define READU16(p)          ((uint16_t)read_16(&p))
#define READ32(p)           ((int32_t)read_32(&p))
#define READU32(p)          ((uint32_t)read_32(&p))
#define READFIXED(p)        ((fixed_t)read_32(&p))
#define READANGLE(p)        ((angle_t)read_32(&p))
#define READBOOLEAN(p)      ((boolean)(*(p)++))

// [WDJ]
// Put {} around all stmt macros with more than one line to protect against
// use as body of if,while, etc..
// Would make these inline functions, but cannot because they
// modify their parameters.
#define SKIPSTRING(p)       { while(READBYTE(p)); }
#define READMEM(p,s,n)      { memcpy(s, p, n); p+=n; }


#endif
