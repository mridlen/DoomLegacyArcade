// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_netcmd.h 1580 2021-07-23 20:55:58Z wesleyjohnson $
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
// $Log: d_netcmd.h,v $
// Revision 1.11  2003/03/22 22:35:59  hurdler
//
// Revision 1.10  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.9  2002/09/12 20:10:50  hurdler
// Added some cvars
//
// Revision 1.8  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.7  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.6  2000/11/02 19:49:35  bpereira
//
// Revision 1.5  2000/08/31 14:30:55  bpereira
//
// Revision 1.4  2000/04/07 23:11:17  metzgermeister
// added mouse move
//
// Revision 1.3  2000/03/06 15:50:02  hurdler
// Add Bell Kin's changes
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      host/client network commands
//      commands are executed through the command buffer
//      like console commands
//
//-----------------------------------------------------------------------------

#ifndef D_NETCMD_H
#define D_NETCMD_H

#include "command.h"

// console vars
extern consvar_t   cv_splitscreen;

// main player and splitscreen player
extern consvar_t   cv_playername[2];
extern consvar_t   cv_playercolor[2];
extern consvar_t   cv_skin[2];
extern consvar_t   cv_autoaim[2];
extern consvar_t   cv_weaponpref[2];

extern consvar_t   cv_controlperkey;

extern consvar_t   cv_itemrespawntime;
extern consvar_t   cv_itemrespawn;
extern consvar_t   cv_respawnmonsters;
extern consvar_t   cv_respawnmonsterstime;

extern consvar_t   cv_teamplay;
extern consvar_t   cv_teamdamage;
extern consvar_t   cv_fraglimit;
extern consvar_t   cv_timelimit;
extern uint32_t  timelimit_tics;

extern consvar_t   cv_netstat;
extern consvar_t   cv_translucency;
extern consvar_t   cv_splats;
extern consvar_t   cv_maxsplats;
extern consvar_t   cv_screenslink;

// Network XCmd.
// These are also saved in DoomLegacy demo, do not alter the order.
typedef enum {
    XD_NAMEANDCOLOR=1,
    XD_WEAPONPREF,
    XD_EXIT,      // keep it for backward compatibility with demos
    XD_QUIT,      // keep it for backward compatibility with demos
    XD_KICK,
    XD_NETVAR,
    XD_SAY,
    XD_MAP,
    XD_EXITLEVEL,
    XD_LOADGAME,
    XD_SAVEGAME,
    XD_PAUSE,
    XD_ADDPLAYER,
    XD_ADDBOT,	//added by AC for acbot
    XD_USEARTIFACT,
    MAXNETXCMD
} netxcmd_e;

// add game commands, needs cleanup
void D_Register_ClientCommands (void);
void D_Send_PlayerConfig(void);

void Command_ExitGame_f(void);

void Send_NameColor_pind( byte pind );
void Send_WeaponPref_pind( byte pind );
//void Send_NameColor_player( byte pn, byte pind );
void Send_NameColor_pn( byte pn, const char * playername, byte color, const char * skinname, byte textcmd_pind );

#endif
