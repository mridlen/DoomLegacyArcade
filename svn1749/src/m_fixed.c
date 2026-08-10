// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_fixed.c 1112 2014-06-03 21:54:41Z smite-meister $
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
// $Log: m_fixed.c,v $
// Revision 1.4  2001/03/30 17:12:50  bpereira
// Revision 1.3  2000/11/26 01:02:27  hurdler
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Fixed point implementation.
//
//-----------------------------------------------------------------------------

#include "i_system.h"
#include "m_fixed.h"

// Fixme. __USE_C_FIXED__ or something.
#ifndef USEASM
fixed_t FixedMul (fixed_t a, fixed_t b)
{
    return ((int64_t) a * (int64_t) b) >> FRACBITS;
}
fixed_t FixedDiv2 (fixed_t a, fixed_t b)
{
#if 0
    int64_t c;
    c = ((int64_t)a<<16) / ((int64_t)b);
    return (fixed_t) c;
#endif

    double c;

    c = ((double)a) / ((double)b) * FRACUNIT;

    if (c >= 2147483648.0 || c < -2147483648.0)
        I_Error("FixedDiv: divide by zero");
    return (fixed_t) c;
}

/*
//
// FixedDiv, C version.
//
fixed_t FixedDiv ( fixed_t   a, fixed_t    b )
{
    //I_Error("<a: %ld, b: %ld>",(long)a,(long)b);

    if ( (abs(a)>>14) >= abs(b))
        return (a^b)<0 ? FIXED_MIN : FIXED_MAX;

    return FixedDiv2 (a,b);
}
*/
#endif // useasm
