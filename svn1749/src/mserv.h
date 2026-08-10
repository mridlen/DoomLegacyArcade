// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: mserv.h 1153 2015-04-03 13:57:45Z wesleyjohnson $
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
// $Log: mserv.h,v $
// Revision 1.9  2000/10/22 00:20:53  hurdler
// Updated for the latest master server code
//
// Revision 1.8  2000/10/17 10:09:27  hurdler
// Update master server code for easy connect from menu
//
// Revision 1.7  2000/10/08 13:30:01  bpereira
//
// Revision 1.6  2000/10/01 15:20:23  hurdler
// Add private server
//
// Revision 1.5  2000/09/08 22:28:30  hurdler
// merge masterserver_ip/port in one cvar, add -private
//
// Revision 1.4  2000/09/02 15:38:24  hurdler
// Add master server to menus (temporaray)
//
// Revision 1.3  2000/08/29 15:53:47  hurdler
// Remove master server connect timeout on LAN (not connected to Internet)
//
// Revision 1.2  2000/08/16 17:21:50  hurdler
// update master server code (bis)
//
// Revision 1.1  2000/08/16 14:04:57  hurdler
// add master server code
//
//
// DESCRIPTION:
//      Header file for the master server routines
//
//-----------------------------------------------------------------------------

#ifndef MSERV_H
#define MSERV_H

#include "command.h"
  // consvar_t

// I want that structure 8 bytes aligned (current size is 80)
typedef struct
{
    char    header[16];     // information such as password
    char    ip[16];
    char    port[8];
    char    name[32];       
    char    version[8];     // format is: x.yy.z (like 1.30.2 or 1.31)
} msg_server_t;


// ================================ GLOBALS ===============================

extern consvar_t cv_masterserver;
extern consvar_t cv_servername;
extern consvar_t cv_internetserver;

char * MS_Get_MasterServerPort(void);
char * MS_Get_MasterServerIP(void);

// MasterServer address is in cv_masterserver.
void MS_RegisterServer(void);
void MS_UnregisterServer(void);

void MS_SendPing_MasterServer( tic_t cur_time );

msg_server_t * MS_Get_ShortServersList(void);

void MS_Register_Commands(void);

#endif
