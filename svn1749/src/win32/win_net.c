// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_net.c 1257 2016-09-20 17:14:21Z wesleyjohnson $
//
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
// $Log: win_net.c,v $
// Revision 1.4  2000/09/01 19:34:38  bpereira
//
// Revision 1.3  2000/09/01 19:13:09  hurdler
// fix some issues with latest network code changes
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      network interface
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "m_argv.h"

//
// NETWORKING
//

//
// I_InitNetwork
//
boolean I_InitNetwork (void)
{
    if( M_CheckParm ("-net") )
    {
      I_Error("The Win32 version of Legacy don't work with external driver like ipxsetup, sersetup, or doomatic\n"
              "Read the documentation about \"-server\" and \"-connect\" parameters or just use the launcher\n");
    }

    return false;
}
