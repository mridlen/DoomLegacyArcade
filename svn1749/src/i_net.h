// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_net.h 1499 2020-03-17 02:27:41Z wesleyjohnson $
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
// $Log: i_net.h,v $
// Revision 1.9  2001/02/10 12:27:13  bpereira
// Revision 1.8  2000/10/16 20:02:29  bpereira
// Revision 1.7  2000/09/10 10:40:06  metzgermeister
// Revision 1.6  2000/09/01 19:34:37  bpereira
//
// Revision 1.5  2000/09/01 18:23:42  hurdler
// fix some issues with latest network code changes
//
// Revision 1.4  2000/08/31 14:30:55  bpereira
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System specific network interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef I_NET_H
#define I_NET_H

#include <stdint.h>

#ifdef __GNUG__
#pragma interface
#endif

#define DOOMCOM_ID       0x12345678l

#define MAXPACKETLENGTH  1450 // For use in a LAN
#define INETPACKETLENGTH 512 // For use on the internet

#ifdef SMIF_PC_DOS
#define DOSNET_SUPPORT
#endif

extern uint16_t  hardware_MAXPACKETLENGTH;
extern uint32_t  net_bandwidth; // in byte/sec

// [WDJ] Can simplify doomcom when drop support for DOS net.
// Referenced by external DosDoom driver, fields cannot be moved.
// Fixed as stdint sizes.
typedef struct
{
    // Supposed to be DOOMCOM_ID
    uint32_t            id;

#ifdef DOSNET_SUPPORT   
    // (DOSNET) DOOM executes an int to execute commands.
    int16_t             intnum;
    // (DOSNET) Communication between DOOM and the driver.
    // Is CMD_SEND or CMD_GET.
    uint16_t            command;
#endif
    // Index to net_nodes:
    // Send: to node
    // Get:  from node (set to -1 = no packet).
    // Used by TCP, UDP.
    int16_t             remotenode;

    // Number of bytes in doomdata to be sent
    uint16_t            datalength;

#ifdef DOSNET_SUPPORT   
    // Info common to all nodes.
    // Console is always node 0.
    uint16_t            num_player_netnodes;
    // Flag: 1 = no duplication, 2-5 = dup for slow nets.
    uint16_t            unused_ticdup;
#endif
    // Number of extratics in each packet.
    // Used by TCP, UDP
    uint16_t            extratics;
#ifdef DOSNET_SUPPORT   
    // deathmatch type 0=coop, 1=deathmatch 1 ,2 = deathmatch 2.
    uint16_t            unused_deathmatch;
    // Flag: -1 = new game, 0-5 = load savegame
    int16_t             unused_savegame;
    int16_t             unused_episode;        // 1-3
    int16_t             unused_map;            // 1-9
    int16_t             unused_skill;          // 1-5

    // Info specific to this node.
    int16_t             consoleplayer;
    // Number total of players
    uint16_t            numplayers;

    // These are related to the 3-display mode,
    //  in which two drones looking left and right
    //  were used to render two additional views
    //  on two additional computers.
    // Probably not operational anymore. (maybe a day in Legacy)
    // 1 = left, 0 = center, -1 = right
    int16_t             unused_angleoffset;
    // 1 = drone
    uint16_t            unused_drone;
#endif

    // The packet data to be sent.
    // Used by all net ports.
    char                data[MAXPACKETLENGTH];

} doomcom_t;

extern doomcom_t *doomcom;
// Called by D_DoomMain.

// Report network errors with global because so many callers ignore it,
// and it is difficult to pass back up through so many layers.
// This also allows for simpler error returns in the functions.
// This fits in a byte.
typedef enum {
   NE_success = 0,
   // warnings
   NE_queued = 1,
   // These values are greater than MAX_CON_NETNODE.
   NE_empty = 200,   // no packet (Get)
   NE_not_netgame,   // still happens often
   // some doomatic errors
   NE_fail,  // generic
   NE_refused_again, // refused, try again
   NE_network_reset,
   // Fatal errors
   NE_fatal,
   NE_network_unreachable,
   NE_unknown_net_error,
   NE_node_unconnected,
   NE_nodes_exhausted,
   NE_queue_full,
   NE_empty_packet,  // debugging, should never happen
   NE_network_down
} network_error_e;

// This is only set on error, it does not indicate success.
// If a test of success is necessary, clear it before the network call.
extern network_error_e  net_error;

// Indirections, to be instantiated by the network driver
// Return packet into doomcom struct.
// Return 0 when got packet, else net_error.  Error in net_error.
extern byte  (*I_NetGet) (void);
// Send packet from within doomcom struct.
// Return 0 when got packet, else net_error.  Error in net_error.
extern byte  (*I_NetSend) (void);
// Return true if network is ready to send.
extern boolean (*I_NetCanSend) (void);
// Close the net node connection.
extern void    (*I_NetFreeNode) (byte nodenum);
// Open a net node connection with a specified address.
// Return the net node number, or network_error_e > MAXNETNODES.   Error in net_error.
extern byte    (*I_NetMakeNode) (char *address);
// Open the network socket.
// Return true if the socket is open.
extern boolean (*I_NetOpenSocket) (void);
// Close the network socket, and all net node connections.
extern void    (*I_NetCloseSocket) (void);

// Set address and port of special nodes.
//  saddr: IP address in network byte order
//  port: port number in host byte order
void UDP_Bind_Node( int nnode, unsigned int saddr, uint16_t port );

// Bind an inet or ipx address string to a net node.
boolean  Bind_Node_str( int nnode, char * addrstr, uint16_t port );


boolean I_InitNetwork (void);

#endif
