// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_tcp.h 1377 2017-12-18 17:29:50Z wesleyjohnson $
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
// $Log: i_tcp.h,v $
// Revision 1.4  2000/10/16 20:02:29  bpereira
//
// Revision 1.3  2000/08/16 14:10:01  hurdler
// add master server code
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   TCP network packet handling.
//
//
//-----------------------------------------------------------------------------

#ifndef I_TCP_H
#define I_TCP_H

extern uint16_t server_sock_port;

void I_Init_TCP_Network(void);

//Hurdler: temporary addition for master server
void I_Init_TCP_Driver(void);
void I_Shutdown_TCP_Driver(void);

#endif
