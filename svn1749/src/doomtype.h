// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: doomtype.h 1721 2025-01-28 15:15:00Z wesleyjohnson $
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
// $Log: doomtype.h,v $
// Revision 1.8  2001/05/16 22:33:34  bock
// Initial FreeBSD support.
//
// Revision 1.7  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.6  2001/02/24 13:35:19  bpereira
// Revision 1.5  2000/11/02 19:49:35  bpereira
// Revision 1.4  2000/10/21 08:43:28  bpereira
//
// Revision 1.3  2000/08/10 14:53:57  ydario
// OS/2 port
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      doom games standard types
//      Simple basic typedefs, isolated here to make it easier
//      separating modules.
//      Dependent upon system and compile flags, but not dependent upon
//      doomdef.h.
//    
//-----------------------------------------------------------------------------

#ifndef DOOMTYPE_H
#define DOOMTYPE_H
// General type defines, not dependent upon doomdef.h

#include <stdint.h>

#if defined( __WATCOMC__) && defined( _M_I386)
// _M_I386  means 32bit Intel
#ifndef WIN32
# define WIN32
#endif
#ifndef __WIN32__
# define __WIN32__
#endif
#endif

#ifdef WIN32
#define WINVER 0x0500  // require windows 2k or later
#include <windows.h>
#else
// WIN32 standard headers already define these!
// TODO this is just a temporary measure, all USHORT / ULONG / INT64 instances 
// in the code should be changed to stdint.h types or basic types
typedef uint32_t ULONG;
typedef uint16_t USHORT;
typedef  int64_t INT64;
#endif

// boolean type
#if defined( __APPLE_CC__ )
//# if ! defined( __GNUC__ )
# ifdef __clang__
//# define boolean uint8_t
typedef uint8_t  boolean;
// system header defines true and false as macros.
# else
# define boolean int
# endif
# ifndef true
# define false 0
# define true  1
# endif
#elif defined(WIN32)
# define false   FALSE           // use windows types
# define true    TRUE
# define boolean BOOL
#else
typedef enum {false, true} boolean;
#endif


typedef uint8_t    byte;
typedef uint32_t   tic_t;

// Return values
// Positive are positive indicators
// 0 is NULL result
// Negative are failure indicators
typedef enum {
    FAIL = -1,   // general fail
    FAIL_end = -2,  // end of valid input
    FAIL_invalid_input = -3,  // invalid input to the function
    FAIL_create = -100,
    FAIL_memory = -101,
    FAIL_select = -102,
} status_return_e;

typedef enum {
    FS_NOTFOUND,
    FS_FOUND,
    FS_REQUESTED,
    FS_DOWNLOADING,
    FS_OPEN,        // is opened and used in w_wad
    FS_MD5SUMBAD,
    FS_SECURITY,  // rejected for security reasons
    FS_FILEERR,   // access error
    FS_INVALID,   // invalid for the operation
    FS_NOTWAD,
    FS_ZIP,
} filestatus_e;

#ifdef __APPLE_CC__
  // Apple GNAT, GNU C 4.5, __APPLE_CC__ == 1
  // Apple C 4.2, __APPLE_CC__ == 5666
  //   They define __MACH__, __GNUC__, and conditionally __BIG_ENDIAN__
  //   Do not use __LITTLE_ENDIAN__, it is not defined on WIN, LINUX
# ifdef SMIF_SDL
   // Mac on SDL, is like Linux
   // Can also test for __APPLE_CC__ or __MACH__
#  define MAC_SDL
# else
   // Hardware direct interface using macos directory (NOT SDL)
#  define MACOS_DI
#  define DEBUG_LOG
#  ifndef HWRENDER
#   define HWRENDER
#  endif
# endif
#endif

#ifdef __GNUC__
// Mingw32 ignores this.
#define PACKED_ATTR  __attribute__((packed))
#else
#define PACKED_ATTR
#endif

#ifdef WIN32
# define ASMCALL __cdecl
#else
# define ASMCALL
#endif

// [WDJ] This program uses strcasecmp, strncasecmp.
#if defined( __MSC__) || defined( __OS2__)
    // Microsoft VisualC++
    #define strncasecmp             strnicmp
    #define strcasecmp              stricmp
    #define inline                  __inline
#else
    #ifdef __WATCOMC__
        #include <dos.h>
        #include <sys\types.h>
        #include <direct.h>
        #include <malloc.h>
        #define strncasecmp             strnicmp
        #define strcasecmp              strcmpi
    #endif
#endif


#if !defined(WIN32) && !defined(__WINDOWS__)
#define min(x,y) ( ((x)<(y)) ? (x) : (y) )
#define max(x,y) ( ((x)>(y)) ? (x) : (y) )

# if !defined(__DJGPP__)
int strupr(char *n);
int strlwr(char *n);
# endif
#endif

#ifndef O_BINARY
#define O_BINARY 0 // stupid windows text files
#endif


// [WDJ] This is needs to be fixed throughout all the files.
// But, not until it is determined which systems support which names.
// Predefined with some OS.
#ifdef __WIN32__
#include <limits.h>
#elif defined( MACOS_DI ) || defined( __MACH__ ) || defined( FREEBSD ) || defined( NETBSD )
#include <limits.h>
#else
// Linux GNU, which also includes limits.h
// obsolete header file
//#include <values.h>
#include <limits.h>
#endif

#if ! defined(MAXINT) && defined( INT_MAX )
// [WDJ] Define the values.h symbols using defines from limits.h.
// This is what values.h does now.
#define MAXINT    INT_MAX
#define MININT    INT_MIN
#endif
#if ! defined(MAXCHAR) && defined( CHAR_MAX )
#define MAXCHAR   CHAR_MAX
#define MINCHAR   CHAR_MIN
#endif
#if ! defined(MAXSHORT) && defined( SHRT_MAX )
#define MAXSHORT  SHRT_MAX
#define MINSHORT  SHRT_MIN
#endif

#if 0
// [WDJ] This is very dangerous considering 32 bit and 64 bit systems,
// should use stdint.h values instead.
// These are obsolete defines from values.h.
#ifndef MAXCHAR
// unused
#define MAXCHAR   ((char)0x7f)
#endif

#ifndef MAXSHORT
// defined in values.h
// used in r_segs.c
#define MAXSHORT  ((short)0x7fff)
#endif

#ifndef MAXINT
// defined in values.h
// used in many places
#define MAXINT    ((int)0x7fffffff)
#endif

#ifndef MINCHAR
// unused
#define MINCHAR   ((char)0x80)
#endif

#ifndef MINSHORT
// defined in values.h
// unused
#define MINSHORT  ((short)0x8000)
#endif

#ifndef MININT
// defined in values.h
// used in many places
#define MININT    ((int)0x80000000)
#endif
#endif

// Sound effect id type.
typedef  uint16_t  sfxid_t;

// This is compatible with SDL_color (R,G,B,-).
typedef union {
    uint32_t  rgba;
    struct {  // component memory order ( R, G, B, A )
        byte  red;    // LITTLE_ENDIAN LSB
        byte  green;
        byte  blue;
        byte  alpha;
    } s;
} RGBA_t;

// [WDJ] Note that RGBA cannot be trusted to be the order of the components.
// SDL uses the term RGBA, SDL opengl uses it extensively.
// The literal RGBA() is the same order as RGBA_t, which works for SDL calls,
// The order of RGBA in memory is (A,B,G,R).
// In a uint32_t, BIG_ENDIAN is (R,G,B,A), and LITTLE_ENDIAN is (A,B,G,R).
// UINT2RGBA reverses the byte order for LITTLE_ENDIAN, which is often not
// what you want.  Mostly, it is just confusing, so avoid it.
// SDL_PixelFormat identifies the actual component order and fields.
// The component order for 32bit pixels in video buffers is (A,R,G,B),
// which is different.
// Use the RGBA_t components, and the RGBA() macro.

#ifdef __BIG_ENDIAN__
    // __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#define UINT2RGBA(a) a
#define RGBA( r, g, b, a )  (((r)<<24)|((g)<<16)|((b)<<8)|(a))
#else
#define UINT2RGBA(a) (((a)&0xff)<<24)|(((a)&0xff00)<<8)|(((a)&0xff0000)>>8)|((((uint32_t)(a))&0xff000000)>>24)
#define RGBA( r, g, b, a )  ((r)|((g)<<8)|((b)<<16)|((a)<<24))
#endif

// Lights values 0..255, but signed to detect underflow.
typedef int16_t   lightlev_t;

typedef uint16_t  statenum_t;

// [WDJ] I would prefer this was uint32_t, but it is being kept signed so that
// tests for -1 can be preserved.  This reduces the chance of logical errors
// due to older fail tests that have not been discovered yet (2018).
// The number of wads is limited to 32, and signed allows over 8000 wads.
typedef int32_t  lumpnum_t;

typedef struct {
    uint16_t first;
    uint16_t last;
} range_t;

#endif  //__DOOMTYPE__
