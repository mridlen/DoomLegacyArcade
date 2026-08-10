// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_fixed.h 1694 2024-11-27 06:44:58Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: m_fixed.h,v $
// Revision 1.9  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.8  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.7  2000/09/28 20:57:15  bpereira
//
// Revision 1.6  2000/04/24 23:52:23  hurdler
// Apply cph patch
//
// Revision 1.5  2000/04/24 20:24:38  bpereira
// Revision 1.4  2000/04/23 16:19:52  bpereira
//
// Revision 1.3  2000/03/07 03:32:24  hurdler
// fix linux compilation
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Fixed point arithmetics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef M_FIXED_H
#define M_FIXED_H

#include <stdint.h>
#include <stdlib.h>

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS                16
#define FRACUNIT                (1<<FRACBITS)
typedef int32_t fixed_t;
#define FIXED_MAX   INT32_MAX
#define FIXED_MIN   INT32_MIN

// Fixed literal
#define FIXINT(x)        ((x)<<FRACBITS)

#define INT_TO_FIXED(x)  ((x)<<FRACBITS)
#define FIXED_TO_INT(x)  ((x)>>FRACBITS)
#define FIXED_TO_FLOAT_MULT    (1.0f / 65536.0f)
//#define FIXED_TO_FLOAT(x) (((float)(x)) / 65536.0)
#define FIXED_TO_FLOAT(x) (((float)(x)) * FIXED_TO_FLOAT_MULT)


typedef struct
{
    fixed_t    x, y;
} xy_t;

typedef struct
{
    fixed_t    x, y, z;
} xyz_t;


//
// Declare those functions:
/*
fixed_t FixedMul (fixed_t a, fixed_t b);
fixed_t FixedDiv (fixed_t a, fixed_t b);
fixed_t FixedDiv2 (fixed_t a, fixed_t b);
*/

#ifndef USEASM
    fixed_t FixedMul (fixed_t a, fixed_t b);
    fixed_t FixedDiv2 (fixed_t a, fixed_t b);
#else
#ifdef __WIN32__
    //Microsoft VisualC++ (no asm inline :( )
    fixed_t __cdecl FixedMul (fixed_t a, fixed_t b);
    fixed_t __cdecl FixedDiv2 (fixed_t a, fixed_t b);
#else
#ifdef __WATCOMC__
#   pragma aux FixedMul =  \
        "imul ebx",         \
        "shrd eax,edx,16"   \
        parm    [eax] [ebx] \
        value   [eax]       \
        modify exact [eax edx]

#   pragma aux FixedDiv2 = \
        "cdq",              \
        "shld edx,eax,16",  \
        "sal eax,16",       \
        "idiv ebx"          \
        parm    [eax] [ebx] \
        value   [eax]       \
        modify exact [eax edx]
#else
    //DJGPP or linux
    //Hurdler: changed with the fix for gcc 2.95.x provided by cph
    static inline fixed_t FixedMul (fixed_t a, fixed_t b)         //asm
    {
          fixed_t ret;
          int dummy;

# if 1
          // [WDJ] Clang does not accept %cc.
	  // Gcc would accept with and without.
          asm("  imull %3 ;"
              "  shrdl $16,%1,%0 ;"
              : "=a" (ret),          /* eax is always the result */
                "=d" (dummy)            /* cphipps - fix compile problem with gcc-2.95.1
                                           edx is clobbered, but it might be an input */
              : "0" (a),                /* eax is also first operand */
                "r" (b)                 /* second operand could be mem or reg before,
                                           but gcc compile problems mean i can only us reg */
              : "cc"                   /* edx and condition codes clobbered */
              );
# else
          // [WDJ] This kept for any older asm that requires it.
          asm("  imull %3 ;"
              "  shrdl $16,%1,%0 ;"
              : "=a" (ret),          /* eax is always the result */
                "=d" (dummy)            /* cphipps - fix compile problem with gcc-2.95.1
                                           edx is clobbered, but it might be an input */
              : "0" (a),                /* eax is also first operand */
                "r" (b)                 /* second operand could be mem or reg before,
                                           but gcc compile problems mean i can only us reg */
              : "%cc"                   /* edx and condition codes clobbered */
              );
# endif

          return ret;
    }

    static inline fixed_t FixedDiv2 (fixed_t a, fixed_t b)
    {
            fixed_t ret;
            asm (
                   "movl  %%eax,%%edx      \n" // these two instructions allow the next
                   "sarl  $31,%%edx        \n" // two to pair, on the Pentium processor.
                   "shldl $16,%%eax,%%edx  \n"
                   "sall  $16,%%eax        \n"
                   "idivl %%ecx            \n"

                 : "=a" (ret)
                 : "a" (a), "c" (b)
                 : "dx"
                 );
            return ret;
    }
#endif
#endif
#endif // useasm

static inline fixed_t FixedDiv (fixed_t a, fixed_t b)
{
    //I_Error("<a: %ld, b: %ld>",(long)a,(long)b);

    if ( (abs(a)>>14) >= abs(b))
        return (a^b)<0 ? FIXED_MIN : FIXED_MAX;

    return FixedDiv2 (a,b);
}

#endif // M_FIXED_H
