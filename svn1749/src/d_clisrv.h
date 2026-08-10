// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_clisrv.h 1582 2021-08-10 20:41:33Z wesleyjohnson $
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
// $Log: d_clisrv.h,v $
// Revision 1.25  2003/07/13 13:16:15  hurdler
// Revision 1.24  2002/07/26 15:21:36  hurdler
//
// Revision 1.23  2001/12/31 12:30:11  metzgermeister
// fixed buffer overflow
//
// Revision 1.22  2001/11/17 22:12:53  hurdler
// Revision 1.21  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.20  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.19  2001/03/30 17:12:49  bpereira
// Revision 1.18  2001/02/19 18:00:49  hurdler
// Revision 1.17  2001/02/10 12:27:13  bpereira
// Revision 1.16  2000/11/11 13:59:45  bpereira
//
// Revision 1.15  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.14  2000/10/22 00:20:53  hurdler
// Updated for the latest master server code
//
// Revision 1.13  2000/10/21 08:43:28  bpereira
// Revision 1.12  2000/10/16 20:02:29  bpereira
// Revision 1.11  2000/10/08 13:29:59  bpereira
// Revision 1.10  2000/09/28 20:57:14  bpereira
// Revision 1.9  2000/09/10 10:37:28  metzgermeister
// Revision 1.8  2000/08/31 14:30:55  bpereira
// Revision 1.7  2000/04/30 10:30:10  bpereira
// Revision 1.6  2000/04/24 20:24:38  bpereira
//
// Revision 1.5  2000/04/19 10:56:51  hurdler
// commited for exe release and tag only
//
// Revision 1.4  2000/04/16 18:38:06  bpereira
// Revision 1.3  2000/04/06 20:32:26  hurdler
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      high level networking stuff
//
//-----------------------------------------------------------------------------

#ifndef D_CLISRV_H
#define D_CLISRV_H

#include <stddef.h>

#include "doomtype.h"
#include "d_ticcmd.h"
#include "d_netcmd.h"
#include "tables.h"
#include "d_items.h"
  // NUMINVENTORYSLOTS, NUMAMMO

//
// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.
//

// Networking and tick handling related.
#define BACKUPTICS            32

//
// Packet structure
//
// Index to packettypename[]
typedef enum   {
    PT_NOTHING,       // to send a nop through network :)
 // High priority
    PT_SERVERCFG,     // server config used in start game (stay 1 for backward compatibility issue)
                      // this is positive response to CLIENTJOIN request
    PT_CLIENTCMD,     // ticcmd of the client
    PT_CLIENTMIS,     // same as above but saying resend from
    PT_CLIENT2CMD,    // with player 2, ticcmd of the client  // NO LONGER USED
    PT_CLIENT2MIS,    // with player 2, same as above but saying resend from  // NO LONGER USED
    PT_NODEKEEPALIVE, // same but without ticcmd and consistancy
    PT_NODEKEEPALIVEMIS,
    PT_SERVERTICS,    // all cmd for the tic
    PT_SERVERREFUSE,  // server refuse joiner (reason inside)
    PT_SERVERSHUTDOWN,// server is shutting down
    PT_CLIENTQUIT,    // client close the connection
               
    // ASKINFO must be at 12 so can identify server version
    PT_ASKINFO = 12,  // to ask info of the server (anyone)
    PT_SERVERINFO,    // send game & server info (gamespy)
    PT_REQUESTFILE,   // client request a file transfer
    PT_DUMMY15,
    PT_ACKS,          // all acks
    PT_STATE,         // server pause state
    PT_DUMMY18,
    PT_DUMMY19,

 // Low Priority
    PT_CANFAIL,       // A priority boundary
                      // This packet can't occupy all slots.
 // with HSendPacket(,SP_reliable,,) these can return false
    PT_FILEFRAGMENT=PT_CANFAIL, // a part of a file
    PT_TEXTCMD,       // extra text command from the client
    PT_TEXTCMD2,      // extra text command from the client (splitscreen)  // NO LONGER USED
 // 23
    PT_CLIENTJOIN,    // client want to join used in start game
    PT_NODE_TIMEOUT,  // packet is sent to self when connection timeout
    PT_NETWAIT,       // network game wait timer info
    PT_CLIENTREADY,   // client is ready
    PT_REPAIR,        // repair position, consistency fix
    PT_CONTROL,       // server to client node specific control
    PT_REQ_SERVERPLAYER, // request players from server
    PT_SERVERPLAYER,  // player state from server
    PT_REQ_SERVERLEVEL,  // request level info from server
    PT_SERVERLEVEL,   // level info from server
    PT_REQ_CLIENTCFG, // request client config (via NetXCmd)
 // count for table
    NUMPACKETTYPE
} packettype_t;


// [WDJ] The compilers are padding these, but not always the same.
// Must be careful with any integer types that are longer than a byte, as they will be aligned,
// as well as any struture that contains such.
// Aligns uint16_t to 2 bytes.
// Aligns uint32_t to 4 bytes.
// Aligns uint64_t, long, to 8 bytes.
// Aligns double to 8 bytes, but in Linux within a struct this may be 4 bytes on some compilers.
// Aligns struct to the alignment of the longest field in the struct.
// Aligns array elements to the alignment of the element type.  Thus it treats array elements
// as if they were padded out to meet this alignment.  This affects sizeof() too.
// On a 64 bit machines, some types (like any ptr) will align to 8 bytes,
// but 32 bit machines will align to 4 bytes.

// [WDJ] Packed structures.
// Do not use ordinary ptrs on any fields of packed structures.
// Some machines will fault, and this cannot be detected by testing on x86 machines.
// Mingw32 does not recognize PACKED_ATTR, which is "__attribute__((packed))".

// Network types
// These will not provoke alignment, and provide easy network endianess conversion.
// 16 bit, unsigned int or other usage
typedef struct {
    byte  b[2];
} N16_t;

// 32 bit, unsigned int or other usage
typedef struct {
    byte  b[4];
} N32_t;

//#pragma pack(1)


#define MAXTEXTCMD           255
#if MAXTEXTCMD > 255
# error  Textcmd len is a byte, cannot hold MAXTEXTCMD
#endif
// One extra byte at end for 0 termination, to protect against malicious use.
// Compatible format with DoomLegacy demo version 1.13 ..
// Actual transmission and saved copies are limited to the actual used length.
typedef struct {
    byte     len;  // 0..MAXTEXTCMD
    byte     text[MAXTEXTCMD+1];
} textbuf_t;
#define sizeof_textbuf_t(len)   (1+(len))

// Used by textcmd_pak_t, servertic_textcmd_t, textcmd storage.
typedef struct {
    byte        pn;    // Explicit player pid.
    textbuf_t   textbuf;
} textcmd_item_t;
#define sizeof_textcmd_item_t(len)   (offsetof( textcmd_item_t, textbuf ) + 1 + (len))

// Ver 1.48,  TEXTCMD
// Used by: PT_TEXTCMD
// Replaces PT_TEXTCMD2 which is no longer used.
// unaligned
typedef struct {
    byte            num_textitem; // num textitem present in this packet
    textcmd_item_t  textitem;  // repeated textcmd_item_t, each variable sized
} textcmd_pak_t;



// client to server packet
// Used by: PT_CLIENTCMD, PT_CLIENTMIS
// Used by: PT_NODEKEEPALIVE, PT_NODEKEEPALIVEMIS
// aligned to 4 bytes
typedef struct {
   byte        client_tic;
   byte        resendfrom;
   int16_t     consistency;
   byte        pind_mask;  // bit mask of pind
#ifdef CLIENTPREDICTION2
// aligned to 4 bytes
   byte        pad1, pad2, pad3;
#else
// aligned to 2 bytes
   byte        pad1;
#endif
   ticcmd_t    cmd[2]; // use only what is needed
} clientcmd_pak_t;


// unaligned
typedef struct {
   byte            tic;
   N16_t           len;  // length of textitem array
   textcmd_item_t  textitem;  // repeated textitem, limited by packet size
} servertic_textcmd_t;
#define sizeof_servertic_textcmd_t(len)   (offsetof( servertic_textcmd_t, textitem )+(len))

typedef enum {
   TPF_seq = 0x07, // sequence number
   TPF_more = 0x08,  // not the last packet
} tic_packet_flag_e;

// Server to client packet
// [WDJ] Ver 1.48, servertic can be sent using multiple packets.
// Will adapt to MAXPACKETSIZE.
#define NUM_SERVERTIC_CMD   45
typedef struct {
   byte        starttic; // low byte of gametic
   byte        numtics;  // 1..
   N32_t       cmd_player_mask;  // bit for each player with ticcmd
   byte        flags;    // tic_packet_flag_e
   byte        cmds_offset;  // extension packet offset
   byte        num_cmds_present;  // in this packet, to locate textcmd
   byte        num_textcmd;  // count of servertic_textcmd_t
#ifdef CLIENTPREDICTION2
   byte        pad1, pad2;
// aligned to 4 bytes
#else
// aligned to 2 bytes
#endif
   ticcmd_t    cmds[NUM_SERVERTIC_CMD];
     // number of cmds used is (numtics*numplayers)
     // normaly [BACKUPTIC][MAXPLAYERS] but too large
// unaligned
// After variable number of ticcmd_t
//   servertic_textcmd_t  st[1];  // variable number
} servertics_pak_t;


// Player updates, Ver 1.48
// aligned to 4 bytes
typedef struct {
   uint32_t    angle;
   int32_t     x, y, z;
   int32_t     momx, momy, momz;
} mobj_pos_t;

// message format for inventory
// unaligned
typedef struct {
    byte type, count;   
} ps_inventory_t; 

// unaligned
typedef struct {
    byte  inventoryslotnum;
    ps_inventory_t  inventory[NUMINVENTORYSLOTS];
} pd_inventory_t;

// aligned to 4 bytes
typedef struct {
    byte  pid; // player index
    byte  playerstate;  // alive or DEAD
    byte  flags;
    byte  readyweapon;
// aligned to 4 bytes
    mobj_pos_t  pos;
    uint16_t  health;
    uint16_t  armor;
    uint32_t  weaponowned;
    uint16_t  ammo[NUMAMMO];
    uint16_t  maxammo[NUMAMMO];
// unaligned
    byte  armortype;
    // optional parts determined by desc_flags, should be unaligned
    byte  optional;
} pd_player_t;

typedef enum   {
  PDI_seq = 0x07,  // sequence number field
  PDI_more = 0x08, // this is not the last sequence number, more packets follow
  PDI_inventory = 0x40,  // optional inventory
} playerdesc_flags_e;

// aligned to 4 bytes
typedef struct {
    byte  desc_flags;  // playerdesc_flags_e
    byte  entry_count; // number of players in playerdesc
    byte  pad1, pad2;  // alignment
// aligned to 4 bytes
    pd_player_t    pd;  // array
      // variable sized entries, but each entry aligned to 4 bytes
} player_desc_t;

// PT_SERVERPLAYER
// aligned to 4 bytes
 typedef struct {
    N32_t  gametic;
    byte  serverplayer;
    byte  skill;  // needed if any bots are to be made
    byte  pad3, pad4;
    byte  playerstate[MAXPLAYERS];  // PS_
// aligned to 4 bytes
    player_desc_t  player_desc;
} playerstate_pak_t;

// PT_SERVERLEVEL
typedef struct {
    byte  gamestate;
    byte  gameepisode;
    byte  gamemap;
    byte  skill;
    byte  nomonsters;
    byte  deathmatch;
} levelcfg_pak_t;


// ver 1.48
// included in other pak
typedef struct {
   byte        p_rand_index; // to sync P_Random
   byte        b_rand_index; // to sync B_Random
   N32_t       e_rand1, e_rand2;  // to sync E_Random
} random_state_t;

// Repair messages triggered by consistency fault.
typedef enum   {
  RQ_NULL,
// to client
  RQ_PLAYER,       // repair of player
  RQ_SUG_SAVEGAME, // server suggests a savegame
  RQ_MONSTER,  // not yet implemented
  RQ_OBJECT,   // not yet implemented
// to server
  RQ_REQ_TO_SERVER = 32,
  RQ_REQ_SAVEGAME,  // Request of savegame
  RQ_REQ_PLAYER,    // Request of player update
  RQ_REQ_ALLPLAYER, // Request all game players
// Ack/Nak
  RQ_CLOSE_ACK = 64,  // client repair done
  RQ_CLOSE_NACK, // failed in repair
  RQ_SAVEGAME_REJ, // server reject savegame
} repair_type_e;

// Server update of client.
// ver 1.48
// PT_REPAIR
// aligned to 4 bytes
typedef struct {
    byte        repair_type;
    N32_t       gametic;
    random_state_t  rs;  // P_Random, etc.
    byte        pad1, pad2;
// aligned to 4 bytes
    union {
      byte           player_id;
      player_desc_t  player_desc;
    } u;
} repair_pak_t;

// ver 1.48
// PT_STATE
// aligned to 4 bytes
typedef struct {
   N32_t       gametic;
   random_state_t  rs;  // P_Random, etc.
   byte        server_pause; // silent pause
} state_pak_t;

// [WDJ] As of 9/2016 there are 37 CV_NETVAR.
// Ver 1.48 (10/2019) there are 49 CV_NETVAR.
#define NETVAR_BUFF_LEN  4096
// PT_SERVERCFG
// aligned to 4 bytes
typedef struct {
   byte        version;    // exe from differant version don't work
   byte        ver1, ver2, ver3;  // reserve for future version
// align to 4 bytes
   uint32_t    subversion; // contain build version and maybe crc

   // server lunch stuffs
   byte        serverplayer;
   byte        num_game_players;
   N32_t       gametic;
   byte        clientnode;
   byte        gamestate;
   byte        command;   // CTRL_ command
   N32_t       playerdetected; // playeringame vector in bit field
// unaligned
   byte        netvar_buf[NETVAR_BUFF_LEN];
} serverconfig_pak_t;

// PT_CLIENTJOIN
// aligned to 4 bytes
typedef struct {
   byte        version;    // different versions are not compatible
   byte        ver1, ver2, ver3;  // reserve for future version
// align to 4 bytes
   uint32_t    subversion; // build version
   byte        num_node_players; // 0,1,2
   byte        mode;
   byte        flags;  // NF_drone, NF_download_savegame
} clientconfig_pak_t;

// PT_FILEFRAGMENT
// aligned to 4 bytes
typedef struct {
   char        fileid;
   byte        pad1, pad2, pad3;
// align to 4 bytes
   uint32_t    position;
   uint16_t    size;
// unaligned
   byte        data[100];  // size is variable using hardare_MAXPACKETLENGTH
} filetx_pak_t;

// ver 1.48
// PT_NETWAIT
// aligned to 4 bytes
typedef struct {
    byte       num_netplayer;  // count players due to 2 player nodes
    byte       wait_netplayer;  // if non-zero, wait for player net nodes
    uint16_t   wait_tics;  // if non-zero, the timeout tics
    random_state_t  rs;  // P_Random, etc.
} netwait_pak_t;

// ver 1.48
// PT_CONTROL
// aligned to 4 bytes
typedef struct {
    byte       command;  // net_control_command_e
    byte       player_num;  // player num (may be 255)
    byte       player_state;  // the state to put the player into
    byte       gamemap, gameepisode;  // current map
    N32_t      gametic;
    N16_t      data2;
    byte       data1;
} control_pak_t;

#define MAXSERVERNAME 32
#define FILENEED_BUFF_LEN  4096
// PT_SERVERINFO
// [WDJ] Do not change this, so older server version can be identified.
// aligned to 4 bytes
typedef struct {
// byte[0]
    byte       version;  // identification version (usually VERSION)
    byte       ver1, ver2, ver3;  // reserve for future version
// byte[4]
 // align to 4 bytes
    uint32_t   subversion;
    byte       num_active_players;  // num of players using the server
    byte       maxplayer;   // control var
    byte       deathmatch;  // control var
    byte       pad4;
// byte[12]
 // align to 4 bytes
    uint32_t   trip_time;   // askinfo time in packet, ping time in list
    float      load;        // unused for the moment
    char       mapname[8];
// byte[28]
    char       servername[MAXSERVERNAME];  // at byte[28]
// byte[60]
    byte       num_fileneed;
    byte       fileneed[FILENEED_BUFF_LEN];   // is filled with writexxx (byteptr.h)
} serverinfo_pak_t;

#define MAXSERVERLIST 32  // limited by the display
// aligned to 4 bytes
typedef struct { 
    serverinfo_pak_t   info;
    byte  server_node;  // network node this server is on
} server_info_t;

// FIXME: use less memory, this uses 32 * 4K, and it is only used temporarily
extern server_info_t  serverlist[MAXSERVERLIST];
extern int serverlistcount;


// PT_ASKINFO
typedef struct {
   byte        version;  // identification version (usually VERSION)
   byte        ver1, ver2, ver3;  // reserve for future version
// align to 4 bytes
   uint32_t    send_time;      // used for ping evaluation
} askinfo_pak_t;

#define MAX_STRINGPAK_LEN  255
// Used by: Server Refuse: PT_SERVERREFUSE
typedef struct {
    char       str[MAX_STRINGPAK_LEN];
} string_pak_t;

#define MAX_NETBYTE_LEN  256
// Used by: Send_AcksPacket: PT_ACKS
// Used by: Net_ConnectionTimeout: PT_NODE_TIMEOUT
// Used by: Send_RequestFile: PT_REQUESTFILE
typedef struct {
      byte     b[MAX_NETBYTE_LEN];
} byte_pak_t;

//
// Network packet data.
//
typedef struct
{                
    uint32_t   checksum;
    byte       ack_req;       // Ask for an acknowlegement with this ack num.
                              // 0= no ack
    byte       ack_return;    // Return the ack number of a packet.
                              // 0= no ack

    byte       packettype;
    byte       pad9;          // padding
// align to 4 bytes
    union  {
      byte_pak_t         bytepak;
      clientcmd_pak_t    clientpak;
      servertics_pak_t   serverpak;
      serverconfig_pak_t servercfg;
      textcmd_pak_t      textcmdpak;
      filetx_pak_t       filetxpak;
      clientconfig_pak_t clientcfg;
      serverinfo_pak_t   serverinfo;
      string_pak_t       stringpak;
      askinfo_pak_t      askinfo;
      netwait_pak_t      netwait;
      repair_pak_t       repair;
      state_pak_t        state;
      control_pak_t      control;
      playerstate_pak_t  playerstate;
      levelcfg_pak_t     levelcfg;
           } u;

} netbuffer_t;

//#pragma pack()

// points inside doomcom
extern  netbuffer_t*   netbuffer;        

extern consvar_t cv_playdemospeed;
extern consvar_t cv_server1;
extern consvar_t cv_server2;
extern consvar_t cv_server3;
extern consvar_t cv_download_files;
extern consvar_t cv_download_savegame;
extern consvar_t cv_netrepair;
extern consvar_t cv_SV_download_files;
extern consvar_t cv_SV_download_savegame;
extern consvar_t cv_SV_netrepair;
extern consvar_t cv_wait_players;
extern consvar_t cv_wait_timeout;
extern consvar_t cv_allownewplayer;
extern consvar_t cv_maxplayers;

//#define PACKET_BASE_SIZE     ((int)&( ((netbuffer_t *)0)->u))
#define PACKET_BASE_SIZE     offsetof(netbuffer_t, u)
//#define FILETX_HEADER_SIZE       ((int)   ((filetx_pak *)0)->data)
#define FILETX_HEADER_SIZE   offsetof(filetx_pak_t, data)

extern boolean   server;
extern uint16_t  software_MAXPACKETLENGTH;

extern byte      num_wait_game_start;  // waiting until next game
extern boolean   cl_drone;  // is a drone client
extern byte      cl_servernode;  // client send to server net node, 251=none (client nnode space)
extern byte      localplayer[2];  // client player number


typedef struct xcmd_s {
    byte * curpos;  // text position, updated by command execution
    byte * endpos;  // end+1 of command text
    byte cmd;
    byte playernum; // from player
} xcmd_t;

// Used in d_net, the only dependence.
void    D_Init_ClientServer (void);
int     ExpandTics (int low);

// Register a NetXCmd to an id.
void    Register_NetXCmd(netxcmd_e cmd_id, void (*cmd_f) (xcmd_t * xc));

// default, always main player
void    Send_NetXCmd(byte cmd_id, void *param, int nparam);
//  pind : player index, [0]=main player, [1]=splitscreen player
void    Send_NetXCmd_pind(byte cmd_id, void *param, int param_len, byte pind);
//  pn : player textcmd dest, when textcmd_pind is 2
void    Send_NetXCmd_auto( byte cmd_id, void *param, int param_len, byte textcmd_pind, byte pn );

// Server textcmd uses separate channel, SERVER_PID.
// This appears in Demo 1.48, must be above MAXPLAYERS.
#define SERVER_PID   250
// default, always SERVER_PID
void    SV_Send_NetXCmd(byte cmd_id, void *param, int param_len);
//  pn : SERVER_PID, or player pid (bots)
void    SV_Send_NetXCmd_pn(byte cmd_id, void *param, int param_len, byte pn);

// command.c
void Got_NetXCmd_NetVar(xcmd_t * xc);
// load/save gamestate (load and save option and for network join in game)
void CV_SaveNetVars(xcmd_t * xc);
void CV_LoadNetVars(xcmd_t * xc);

// Create any new ticcmds and broadcast to other players.
void    NetUpdate (void);
void    D_PredictPlayerPosition(void);

byte    SV_get_player_num( void );
void    SV_Add_game_start_waiting_players( byte mode );
void    SV_StartSinglePlayerServer(void);
boolean SV_SpawnServer( void );
void    SV_SpawnPlayer(byte playernum, int x, int y, angle_t angle);
void    SV_StopServer( void );
void    SV_ResetServer( void );

// [WDJ] Update state by sever.
// By Server
void    SV_Send_State( byte server_pause );
//  wait_timeout : wait timeout in ticks
void    SV_network_wait_timer( uint16_t wait_timeout );

// By Client
void    CL_Splitscreen_Player_Manager( void );
void    CL_Reset (void);
void    CL_Update_ServerList( boolean internetsearch );
// is there a game running
boolean Game_Playing( void );


// Broadcasts special packets to other players
// to notify of game exit.
void    D_Quit_NetGame (void);

// Wait Player interface.
void    D_WaitPlayer_Setup( void );
void    D_WaitPlayer_Drawer( void );
boolean D_WaitPlayer_Response( int key );

// How many ticks to run.
void    TryRunTics (tic_t realtic);

// extra data for lmps
boolean AddLmpExtradata(byte **demo_p,int playernum);
void    ReadLmpExtraData(byte **demo_pointer,int playernum);

// Name can be player number, or player name.
// Return player number.
// Return 255, and put msg to console, when name not found.
byte  player_name_to_num( const char * name );

#endif
