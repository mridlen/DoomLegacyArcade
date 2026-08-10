// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_net.c 1257 2016-09-20 17:14:21Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      network interface
//      
//-----------------------------------------------------------------------------


#include <errno.h>

#include "doomincl.h"
#include "doomstat.h"

#include "i_system.h"
#include "i_net.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"
#include "z_zone.h"

//
// NETWORKING
//


//
// I_InitNetwork
//
boolean I_InitNetwork (void)
{
    I_InitTcpNetwork();
    
    //return true;
  if( M_CheckParm ("-net") )
    {
        I_Error("-net not supported, use -server and -connect\n"
            "see docs for more\n");
    }
    return false;
}
