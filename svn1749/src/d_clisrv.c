// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_clisrv.c 1699 2024-11-27 07:20:27Z wesleyjohnson $
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
// $Log: d_clisrv.c,v $
// Revision 1.47  2004/07/27 08:19:34  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.46  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.45  2003/11/22 00:22:08  darkwolf95
// get rid of FS hud pics on level exit and new game, also added exl's fix for clearing hub variables on new game
//
// Revision 1.44  2003/05/04 04:30:30  sburke
// Ensure that big-endian machines encode/decode network packets as little-endian.
//
// Revision 1.43  2003/03/22 22:35:59  hurdler
//
// Revision 1.42  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.41  2001/08/20 20:40:39  metzgermeister
// Revision 1.40  2001/06/10 21:16:01  bpereira
//
// Revision 1.39  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.38  2001/05/14 19:02:57  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.37  2001/04/27 13:32:13  bpereira
// Revision 1.36  2001/04/01 17:35:06  bpereira
// Revision 1.35  2001/03/30 17:12:49  bpereira
// Revision 1.34  2001/03/03 06:17:33  bpereira
// Revision 1.33  2001/02/24 13:35:19  bpereira
// Revision 1.32  2001/02/10 12:27:13  bpereira
//
// Revision 1.31  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.30  2000/11/11 13:59:45  bpereira
//
// Revision 1.29  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.28  2000/10/22 00:20:53  hurdler
// Updated for the latest master server code
//
// Revision 1.27  2000/10/21 23:21:56  hurdler
// Revision 1.26  2000/10/21 08:43:28  bpereira
//
// Revision 1.25  2000/10/17 10:09:27  hurdler
// Update master server code for easy connect from menu
//
// Revision 1.24  2000/10/16 20:02:28  bpereira
// Revision 1.23  2000/10/08 13:29:59  bpereira
// Revision 1.22  2000/10/01 10:18:16  bpereira
// Revision 1.21  2000/09/28 20:57:14  bpereira
// Revision 1.20  2000/09/15 19:49:21  bpereira
// Revision 1.19  2000/09/10 10:37:28  metzgermeister
// Revision 1.18  2000/09/01 19:34:37  bpereira
// Revision 1.17  2000/08/31 14:30:55  bpereira
//
// Revision 1.16  2000/08/21 11:06:43  hurdler
// Add ping and some fixes
//
// Revision 1.15  2000/08/16 15:44:18  hurdler
// update master server code
//
// Revision 1.14  2000/08/16 14:10:01  hurdler
// add master server code
//
// Revision 1.13  2000/08/11 19:10:13  metzgermeister
//
// Revision 1.12  2000/08/11 12:25:23  hurdler
// latest changes for v1.30
//
// Revision 1.11  2000/08/03 17:57:41  bpereira
// Revision 1.10  2000/04/30 10:30:10  bpereira
// Revision 1.9  2000/04/24 20:24:38  bpereira
// Revision 1.8  2000/04/16 18:38:06  bpereira
//
// Revision 1.7  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.6  2000/03/29 19:39:48  bpereira
//
// Revision 1.5  2000/03/08 17:02:42  hurdler
// fix the joiningame problem under Linux
//
// Revision 1.4  2000/03/06 16:51:08  hurdler
// hack for OpenGL / Open Entry problem
//
// Revision 1.3  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      DOOM Network game communication and protocol,
//      High Level Client / Server communications and functions.
//
//-----------------------------------------------------------------------------


#include <time.h>
#include <unistd.h>

#include "doomincl.h"
#include "doomstat.h"
#include "d_clisrv.h"
#include "command.h"
#include "i_net.h"
#include "i_tcp.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "d_net.h"
#include "d_netcmd.h"
#include "d_netfil.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "keys.h"
#include "m_argv.h"
#include "m_menu.h"
#include "r_main.h"   // R_SetViewSize, when the view grid changes
#include "console.h"
#include "byteptr.h"

#include "p_saveg.h"
#include "p_setup.h"
#include "z_zone.h"
#include "p_tick.h"
#include "p_local.h"
#include "m_misc.h"
#include "am_map.h"
#include "m_random.h"
#include "mserv.h"
#include "t_script.h"

#include "b_game.h"	//added by AC for acbot
#include "r_things.h"
  // skins
#include "g_input.h"
  // gamecontrol
#include "s_sound.h"
  // StartSound
#include "d_items.h"
  // NUMINVENTORYSLOTS, NUMAMMO


//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tic that hasn't had control made for it yet
// server:
//   nettics is the tic for each node
//   next_tic_send is the lowest value of nettics
// client:
//   cl_need_tic is the tic needed by the client for run the game
//   next_tic_send is used to optimize a condition
// normaly maketic>=gametic>0,

// The addition of wait messages should be transparent to previous network
// versions.
const int  NETWORK_VERSION = 26; // separate version number for network protocol (obsolete)


#define JOININGAME


#if NUM_SERVERTIC_CMD < BACKUPTICS
# error Not enough NUM_SERVERTIC_CMD
#endif

// Used to signal network errors to higher level functions.
// Beware that server may also have a client.
typedef enum {
   NETS_idle,
   NETS_fatal,
   NETS_shutdown,
// Have network.
   NETS_open,      // Normal network.
   NETS_internal,  // Self server network.
// Client
   NETS_no_server, // Client lost the server.
   NETS_connecting,// Client connecting to the server
   NETS_active     // Client has server connection.
} network_state_e;

#ifdef DEBUG_WINDOWED
static network_state_e  network_state = NETS_idle;  // easier debugging
#else
static byte  network_state = NETS_idle;  // network_state_e
#endif
static byte  quit_netgame_status = 0;  // to avoid repeating shutdown
static byte  wait_netplayer = 0;

#define PREDICTIONQUEUE         BACKUPTICS
#define PREDICTIONMASK          (PREDICTIONQUEUE-1)

// Server state
boolean  server = true; // false when Client connected to other server
boolean  serverrunning = false;
byte     serverplayer = 255;  // 255= no server player (same as -1)

// Server specific vars.
// player=255 when unused
// nnode =255 when unused
// nnode_state[] = NOS_idle, when net node is unused
// The netnodes are counted, 0..31
static byte  player_to_nnode[MAXPLAYERS];
static byte  player_pind[MAXPLAYERS];
static byte  num_player_used = 0;
static byte  num_join_waiting_players = 0;
#ifdef WAIT_GAME_START_INTERMISSION
byte  num_wait_game_start = 0;  // waiting until next game
#endif


// Server net node state for
// tracking client nodes.
typedef enum {
  NOS_idle,  // node is unused
  NOS_fatal, // something bad
  NOS_shutdown,  // node is shutting down
  NOS_invalid,   // invalid node
// Recognized nodes.
  NOS_recognized,
// node during join procedure
  NOS_join,         // node join procedure
  NOS_join_file,    // downloading file
  NOS_join_savegame,// downloading savegame
  NOS_join_sg_loaded,  // downloading done
  NOS_join_timeout,
// node with a player
  NOS_client,
// waiting players
  NOS_wait_game_start, // waiting for next game start
// repair a client
  NOS_repair,          // node is being repaired
  NOS_repair_player,   // update player
  NOS_repair_pl_done,  // player update done
  NOS_repair_savegame, // downloading savegame
  NOS_repair_sg_loaded,   // downloading done
  NOS_repair_timeout,
// node normal play
  NOS_active,
// FOR CLIENT USE ONLY.  Server must not use for client state.
  NOS_server,   // client sees server at cl_servernode
  NOS_internal  // client sees server on self
} nnode_state_e;

// [WDJ] Used to be OR with nnode_to_player to indicate DRONE node.
// No longer done that way to keep player number clean, as it is used for indexing
// and the DRONE bit conflicted with 255=idle player.
// #define DRONE               0x80    // bit set in consoleplayer
// DRONE has playerpernode=0

// Server: net node state of clients.
// Node numbers seen by server are different than those seen by clients (determined by connection order).
// Index by server space nnode numbers.
#ifdef DEBUG_WINDOWED
static nnode_state_e   nnode_state[MAXNETNODES];  // easier debugging
#else
static byte     nnode_state[MAXNETNODES];  // nnode_state_e
#endif
// Index by pind, [0]=main player [1]=splitscreen player
static byte     nnode_to_player[MAXSPLITSCREENPLAYERS][MAXNETNODES];  // 255= unused
static byte     playerpernode[MAXNETNODES]; // used specialy for splitscreen
static byte     join_waiting[MAXNETNODES];  // num of players waiting to join
static byte     consistency_faults[MAXNETNODES];
static tic_t    nettics[MAXNETNODES];     // what tic the client have received
static tic_t    nextsend_tic[MAXNETNODES]; // what server sent to client

static tic_t    next_tic_send;     // min of the nettics
static tic_t    next_tic_clear=0;  // clear next_tic_clear to next_tic_send
static tic_t    maketic;
#ifdef CLIENTPREDICTION2
tic_t localgametic;
#endif

// Client specific.

// Client state
typedef enum {
   CLM_idle,
   CLM_fatal,
   CLM_searching,
   CLM_server_files,
   CLM_download_req,
   CLM_download_files,
   CLM_askjoin,
   CLM_wait_join_response,
   CLM_download_savegame,
   CLM_download_done,
   CLM_wait_game_start,  // ready but must wait for next game start
   CLM_connected
} cl_mode_t;

#ifdef DEBUG_WINDOWED
static cl_mode_t  cl_mode = CLM_idle;  // easier debugging
#else
static byte     cl_mode = CLM_idle;  // cl_mode_t
#endif

boolean         cl_drone; // client displays, no commands
static byte     cl_nnode; // net node for this client, assigned by server (server nnode space)
static byte     cl_error_status = 0;  // repair
static boolean  cl_packetmissed;
static tic_t    cl_need_tic;
static tic_t    cl_prev_tic = 0;  // client tests once per tic

// Client view of server :
//   Node numbers on client are not same as those on server, each has own independent nnode space.
//   Server net node, 251=none (to not match invalid node)
byte            cl_servernode = 251;  // in client nnode space, never let server use this
static byte     cl_server_state = NOS_idle; // nnode_state_e, client view of server

// Text buffer for textcmds.
// One extra byte at end for 0 termination, to protect against malicious use.
// Use textbuf_t from textcmdpak

// Client maketic
// [0]=main player [1]=splitscreen player
// 255 = unused.  [Arcade] This MUST be a static initializer, not a runtime
// one: M_LoadConfig runs long before D_Init_ClientServer, and the config's
// player name/skin lines reach Send_NameColor2, which tests
// localplayer[1] < MAXPLAYERS.  Left zeroed, that reads as "player 0" and
// sends a netxcmd with no server behind it -- an immediate segfault at
// startup, before any game exists.
byte              localplayer[MAXSPLITSCREENPLAYERS] = { 255, 255, 255, 255 };

// Guard rail for the list above: raising MAXSPLITSCREENPLAYERS must extend it,
// or the new slots default to 0, which is a valid player number.  Deliberately
// a compile error rather than a runtime surprise.
typedef char localplayer_init_covers_all_slots[ (MAXSPLITSCREENPLAYERS == 4)? 1 : -1 ];

// [Arcade] How many players share this machine.  Clamped to the compiled
// limit so a hand-edited config cannot ask for more slots than exist, and to
// at least 1 -- a cabinet with no local player would never spawn anybody.
// [Arcade] Players the join screen counted in, 0 = nobody asked yet, in which
// case every panel plays (which is what a cabinet without a join screen, or a
// game started from the console, wants).
static byte  local_join_count = 0;

void  D_Set_Join_Count( byte count )
{
    local_join_count = (count <= MAXSPLITSCREENPLAYERS) ? count : MAXSPLITSCREENPLAYERS;
}

void  D_Clear_Join_Count( void )
{
    local_join_count = 0;
}

byte  D_NumLocalPlayers( void )
{
    int n = local_join_count ? local_join_count : cv_localplayers.EV;

    // cv_splitscreen means at least two players, and it is the *only* thing
    // the Two Player Game menu sets -- that menu predates cv_localplayers and
    // still just turns the split on.  Without this, asking for two players
    // from the menu joined only one, because cv_localplayers defaults to 1.
    if( cv_splitscreen.EV && (n < 2) )  n = 2;

    if( n < 1 )  n = 1;
    if( n > MAXSPLITSCREENPLAYERS )  n = MAXSPLITSCREENPLAYERS;
    return (byte) n;
}


// [Arcade] Which cell of the view grid each local player occupies.  Players
// join in sequence -- panel 3 joining while panel 2 sits out still becomes
// local player 1 -- but on a cabinet your screen area should match where you
// are standing, so the join screen records the panel each player pressed and
// the drawing code asks here rather than using the join order.
//
// Identity by default, which is exactly the old behaviour when every panel
// joins in order.  Deliberately an indirection rather than making the join
// itself claim a slot: SV_commit_player hands out pind sequentially and the
// protocol sends a player *count*, so non-contiguous joins would have meant
// changing the netcode, while the draw loops already skip empty cells.
static byte  localplayer_cell[MAXSPLITSCREENPLAYERS] = { 0, 1, 2, 3 };

// [Arcade] And which physical panel drives each local player.  This is NOT
// the same question as the cell: a lone player who joined at panel 3 gets the
// whole screen (cell 0) but must still be driven by panel 3's buttons.  They
// were one table at first, and the compact 1-2 player layout overwrote the
// panel with the cell -- so joining at panel 3 handed you player 1's
// controls and the panel you were standing at did nothing.
static byte  localplayer_panel[MAXSPLITSCREENPLAYERS] = { 0, 1, 2, 3 };

typedef char localplayer_cell_covers_all_slots[ (MAXSPLITSCREENPLAYERS == 4)? 1 : -1 ];

byte  D_View_Cell( byte pind )
{
    return (pind < MAXSPLITSCREENPLAYERS) ? localplayer_cell[pind] : pind;
}

// [Arcade] The control panel driving this local player.
byte  D_Panel_Of( byte pind )
{
    return (pind < MAXSPLITSCREENPLAYERS) ? localplayer_panel[pind] : pind;
}

// [Arcade] Which local player this panel drives, or MAXSPLITSCREENPLAYERS if
// none does.  The inverse of D_Panel_Of.
//
// It must skip slots with no player.  localplayer_panel[] keeps its identity
// default for pinds that never joined, so with panels 2+3+4 it reads
// [1,2,3,3] and the unjoined pind 3 would otherwise answer for panel 4
// instead of pind 2, who is actually playing there.
byte  D_Pind_Of_Panel( byte panel )
{
    byte pind;

    for( pind = 0; pind < MAXSPLITSCREENPLAYERS; pind++ )
    {
        if( localplayer[pind] >= MAXPLAYERS )  continue;   // no player here
        if( D_Panel_Of(pind) == panel )  return pind;
    }

    return MAXSPLITSCREENPLAYERS;
}

void  D_Set_Panel( byte pind, byte panel )
{
    if( pind < MAXSPLITSCREENPLAYERS && panel < MAXSPLITSCREENPLAYERS )
        localplayer_panel[pind] = panel;
}

// Called by the join screen: local player 'pind' plays in grid cell 'cell'.
// R_SetViewSize because moving a player between cells can change how the
// screen is carved up (two players at panels 1 and 3 need the 2x2, not the
// stacked halves), and the viewport sizes are only recomputed on request --
// otherwise the new cell keeps the old cell's width.
void  D_Set_View_Cell( byte pind, byte cell )
{
    if( pind < MAXSPLITSCREENPLAYERS && cell < MAXSPLITSCREENPLAYERS
        && localplayer_cell[pind] != cell )
    {
        localplayer_cell[pind] = cell;
        R_SetViewSize();
    }
}

// Back to join order, for a game started without the join screen.
void  D_Reset_View_Cells( void )
{
    byte pind;
    boolean changed = false;
    for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )
    {
        if( localplayer_cell[pind] != pind )  changed = true;
        localplayer_cell[pind] = pind;
        localplayer_panel[pind] = pind;
    }
    if( changed )  R_SetViewSize();
}


// [Arcade] How many viewports are drawn, and so how the screen is carved up:
//   1 -> whole screen
//   2 -> two stacked halves, as splitscreen has always been
//   4 -> a 2x2 grid, used for three players too, leaving one quadrant unused
// Three players in a 2x2 grid rather than three strips because thirds would
// need a third layout and would not match a 4 player cabinet's muscle memory.
// Note a quadrant has very nearly the screen's own aspect ratio, so unlike
// the 2 view split it needs no projection fix.
byte  D_NumViews( void )
{
    byte pind, n;
    byte top = 0;   // highest cell any local player occupies

    // [Arcade] A demo has exactly one viewpoint, whatever the cabinet is set
    // up for, so it plays full screen.  Without this the attract screen was
    // carved into a 2x2 and the demo drew in the top-left quadrant, which is
    // the same trap cv_splitscreen had -- Command_ExitGame_f clears that on
    // the way to the title for exactly this reason, but cv_localplayers is an
    // operator setting and does not get cleared.  Covers the Single Level
    // "watch run" replays as well.
    if( demoplayback )  return 1;

    n = D_NumLocalPlayers();
    if( n <= 1 )  return 1;

    for( pind=0; pind<n && pind<MAXSPLITSCREENPLAYERS; pind++ )
    {
        byte c = D_View_Cell(pind);
        if( c > top )  top = c;
    }

    // Two players sitting at panels 1 and 3 need the 2x2, not the stacked
    // halves, or the second of them has no cell to be drawn in.
    return (top >= 2) ? 4 : 2;
}


// [Arcade] Mark every local player slot unused.
void CL_Init_localplayer( void )
{
    byte pind;
    for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )
        localplayer[pind] = 255;
}
static ticcmd_t   localcmds[MAXSPLITSCREENPLAYERS];
static textbuf_t  localtextcmd[MAXSPLITSCREENPLAYERS];

// engine
// Server packet state
static uint32_t ticcmd_player_mask = 0;  // one bit for each player that sends ticcmd, no bots
static byte     num_ticcmd_per_tic = 0;  // num in ticcmd_pid
static byte     ticcmd_pid[MAXPLAYERS];  // pid in order of ticcmd in packet
static byte     update_player_count_request = 0;  // for XNetCmd, as the list is in use during execution

// NetCmd and TextCmd store
// Index for netcmds and textcmds
#define BTIC_INDEX( tic )  ((tic)%BACKUPTICS)
// Index using BTIC_INDEX
static byte     netcmd_tic_hash[BACKUPTICS];  // tic hash for the BACKUPTIC
static byte     netseq[BACKUPTICS];   // bit per tic packet seq, ready=0xFF
ticcmd_t        netcmds[BACKUPTICS][MAXPLAYERS];

// [WDJ] Combined textcmd buffer for all players, and server.
// No apparant reason to keep separate buffers at this point.
// MAX_TEXTCMD_BUFF must be enough for map command, but less than max packet size.
#if defined(SMIF_PC_DOS) || defined(DOSNET_SUPPORT)
  // may have small packet limits
  // 32*15 - 3
# define MAX_TEXTCMD_BUFF       477
#else
  // 32*30 - 3
# define MAX_TEXTCMD_BUFF       957
#endif
typedef struct {
   uint16_t  len;  // 0..MAX_TEXTCMD_BUFF
   byte      buff[MAX_TEXTCMD_BUFF+1];  // format as array of textcmd_item_t
             // Format: pn, len, text[MAXTEXTCMD]
} textcmdbuff_t;
static textcmdbuff_t  textcmdbuff[BACKUPTICS];

static int16_t    consistency[BACKUPTICS];



consvar_t cv_playdemospeed  = {"playdemospeed","0",CV_VALUE,CV_Unsigned};

consvar_t cv_server1 = { "server1", "192.168.1.255", CV_STRING|CV_SAVE, NULL };
consvar_t cv_server2 = { "server2", "", CV_STRING|CV_SAVE, NULL };
consvar_t cv_server3 = { "server3", "", CV_STRING|CV_SAVE, NULL };

CV_PossibleValue_t download_cons_t[] = {{0,"No Download"},{1,"Allowed"},{0,NULL}};
consvar_t cv_download_files = {"download_files"  ,"1", CV_SAVE, download_cons_t};
consvar_t cv_SV_download_files = {"sv_download_files"  ,"1", CV_SAVE, download_cons_t};
consvar_t cv_download_savegame = {"download_savegame"  ,"1", CV_SAVE, download_cons_t};
consvar_t cv_SV_download_savegame = {"sv_download_savegame"  ,"1", CV_SAVE, download_cons_t};

CV_PossibleValue_t netrepair_cons_t[] = {{0,"None"},{1,"Minimal"},{2,"Medium"},{3,"Aggressive"},{0,NULL}};
consvar_t cv_netrepair = {"netrepair","2", CV_SAVE, netrepair_cons_t};
consvar_t cv_SV_netrepair = {"sv_netrepair","2", CV_SAVE, netrepair_cons_t};

// consistency check, index by cv_SV_netrepair
static byte consistency_limit_fatal[4] = { 1, 2, 5, 8 };
static byte consistency_sg_bit[4]    = { 0, 0, 0x04, 0x44 };  // bit on when should req savegame



// some software don't support largest packet
// (original sersetup, not exactly, but the probability of sending a packet
// of 512 octet is like 0.1)
uint16_t  software_MAXPACKETLENGTH;

// Align to 4 byte
#define ALIGN4(p) (byte *)(((uintptr_t)(p) + 3) & ~0x03)

// Special unaligned types for network packets.
// Detect which machines can do unaligned read and write, with LSB first.
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__x86_64__)
  # define UNALIGNED_INT_RW
#endif
#if defined(__ia64__) && defined(__GNUC__) && (__GNUC__ >= 3) && ! defined(__BIG_ENDIAN__)
  # define UNALIGNED_INT_RW
#endif
#if defined(__arm__) && defined(__ARM_FEATURE_UNALIGNED) && ! defined(__BIG_ENDIAN__)
  # define UNALIGNED_INT_RW
#endif

static inline
void  write_N16( N16_t * vf, uint16_t val )
{
    // LSB first
#if defined(UNALIGNED_INT_RW)
    *((uint16_t*)vf) = val;
#else
    vf->b[0] = val;
    vf->b[1] = val>>8;
#endif
}

static inline
uint16_t  read_N16( N16_t * vf )
{
    // LSB first
#if defined(UNALIGNED_INT_RW)
    return *((uint16_t*)vf);
#else
    return (((uint16_t)(vf->b[1]))<<8) | vf->b[0];
#endif
}

static inline
void  write_N32( N32_t * vf, uint32_t val )
{
    // LSB first
#if defined(UNALIGNED_INT_RW)
    *((uint32_t*)vf) = val;
#else
    vf->b[0] = val;
    vf->b[1] = val>>8;
    vf->b[2] = val>>16;
    vf->b[3] = val>>24;
#endif
}

static inline
uint32_t  read_N32( N32_t * vf )
{
    // LSB first
#if defined(UNALIGNED_INT_RW)
    return *((uint32_t*)vf);
#else
    return (((((((uint32_t)(vf->b[3]))<<8) | vf->b[2])<<8) | vf->b[1])<<8) | vf->b[0];
#endif
}


// Handle errors from HSendPacket consistently.
static void  generic_network_error_handler( byte errcode, const char * who )
{
    if( errcode == NE_not_netgame )  return;  // known problem during join
    if( errcode >= NE_fail )
        network_error_print( errcode, who );     
}

static byte btic_hash( tic_t tic )
{
    // Overlap with the BTIC_INDEX (tic % BACKUPTICS) is wasted.
    return (byte) (tic >> 4);
}

// By Client, Server
int ExpandTics (int low)
{
    int delta;

    delta = low - (maketic&0xff);

    if (delta >= -64 && delta <= 64)
        return (maketic&~0xff) + low;
    if (delta > 64)
        return (maketic&~0xff) - 256 + low;
    if (delta < -64)
        return (maketic&~0xff) + 256 + low;
#ifdef PARANOIA
    I_SoftError ("ExpandTics: strange value %i at maketic %i\n", low, maketic);
#endif
    return 0;
}

static void D_Clear_ticcmd(int tic)
{
    int i;
    int btic = BTIC_INDEX( tic );

    netcmd_tic_hash[btic] = 0;
    netseq[btic] = 0;

    for(i=0;i<MAXPLAYERS;i++)
    {
#ifdef TICCMD_148
        netcmds[btic][i].ticflags = 0; //  clear TC_received;
#else
        netcmds[btic][i].angleturn = 0; //&= ~TICCMD_RECEIVED;
#endif
    }
    textcmdbuff[btic].len = 0;  // all players
    DEBFILE(va("Clear tic %5d [%2d]\n", tic, btic));
}


// -----------------------------------------------------------------
//  Some extra data function for handle textcmd buffer
// -----------------------------------------------------------------

// --- Text command
// Client NetXCmd: XD_NAMEANDCOLOR, XD_WEAPONPREF, XD_USEARTIFACT, XD_SAY, XD_PAUSE
// Server NetXCmd: XD_KICK, XD_ADDPLAYER, XD_ADDBOT,
//    XD_MAP, XD_EXITLEVEL, XD_LOADGAME, XD_SAVEGAME

static void net_textcmd_handler( byte nnode );
static void update_player_state( byte pn, byte new_player_state );
static void update_player_counts(void);

// NetXCmd indirection.
static void (*netxcmd_func[MAXNETXCMD]) (xcmd_t * xc);

void Register_NetXCmd(netxcmd_e cmd_id, void (*cmd_f) (xcmd_t * xc))
{
#ifdef PARANOIA
   if(cmd_id >= MAXNETXCMD)
      I_Error("NetXCmd id %d exceeds defined range", cmd_id);
   if(netxcmd_func[cmd_id]!=0)
      I_Error("NetXCmd id %d already registered", cmd_id);
#endif
   netxcmd_func[cmd_id] = cmd_f;
}

// [WDJ] Ver 1.48.
// Server NetXCmd use a server textcmd channel, that bypasses the localtextcmd.
// This is necessary because of bots, which must issue textcmds from the server.

// The server textcmd are appended to the textcmd buffers using SERVER_PID.
// This allows some checking for bogus server textcmd.
// During demo playback, server textcmd on [0] must also be kept open.
// The server NetXCmd do not use the client_pn, so either will technically work.

// Server NetXCmd need to transit the network logic, even when there
// is no server player. This used to be done by unexplained kludges.
// A server pind of TEXTCMD_PIND_SERVER is used for routing.  [Arcade] that
// was the literal 2, which is a real player index once a cabinet has more
// than two panels.

// The NetXCmd must be executed in all clients during a specific gametic.
// The current system has all movement commands and NetXCmd for one tick
// in the servertick message.  Once it is received the client game tick can advance.
// In Ver 1.48, the servertick message can be extended to several packets, when needed.

// Command sent over text cmd channel.  Default as main player.
//  cmd_id :  X command, XD_
//  param : parameter strings
//  param_len : number of parameter strings
void Send_NetXCmd(byte cmd_id, void *param, int param_len)
{
    byte pind = (localplayer[0] < MAXPLAYERS)? 0 : 1;  // main player default
    Send_NetXCmd_pind( cmd_id, param, param_len, pind );
}

// Client
// Command sent over text cmd channel.
//  cmd_id :  X command, XD_
//  param : parameter strings
//  param_len : number of parameter strings
//  pind : player index, [0]=main player, [1]=splitscreen player
void Send_NetXCmd_pind( byte cmd_id, void *param, int param_len, byte pind )
{
   if(demoplayback)
       return;

   // Save the NetXCmd in a localtextcmd buffer.
   textbuf_t * ltcbp = &localtextcmd[pind];
   int textlen = ltcbp->len;
   if( (textlen + 1 + param_len) > MAXTEXTCMD)  // with XD_ and param
   {
#ifdef PARANOIA
       I_SoftError("Net command exceeds buffer: pind=%d  netcmd=%d  netcmdlen=%d  total=%d\n",
                   pind, cmd_id, param_len, (textlen + 1 + param_len));
#else
       GenPrintf(EMSG_warn, "\2Net Command exceeds buffer: pind=%d  netcmd %d  netcmdlen=%d  total=%d\n",
                   pind, cmd_id, param_len, (textlen + 1 + param_len));
#endif
       return;
   }

   // Append to player text commands.
   // First byte is the cmd, followed by its parameters (binary or string).
   ltcbp->text[textlen++] = cmd_id; // XD_
   if(param && param_len)
   {
       memcpy(&ltcbp->text[textlen], param, param_len);
       textlen += param_len;
   }
   ltcbp->len = textlen;
}

// By Server or Client, routing determined by textcmd_pind.
//  cmd_id :  X command, XD_
//  param : parameter strings
//  param_len : number of parameter strings
//  textcmd_pind : the textcmd channel to use
//  pn : player textcmd dest, when textcmd_pind is TEXTCMD_PIND_SERVER
void Send_NetXCmd_auto( byte cmd_id, void *param, int param_len, byte textcmd_pind, byte pn )
{
    // Client on server can just as well use server channel too,
    // as long as textcmd ends up in correct textcmd channel.
    if( server || (textcmd_pind >= TEXTCMD_PIND_SERVER) || (pn == SERVER_PID) )  // routing
    {
        if( server )
        {
            // Sending NetXCmd using server channel.
            SV_Send_NetXCmd_pn( cmd_id, param, param_len, pn );
        }
    }
    else
    {
        // Clients send using their localtextcmd[pind].
        Send_NetXCmd_pind( cmd_id, param, param_len, textcmd_pind );
    }
}

// NetXCmd as server, always SERVER_PID.
void  SV_Send_NetXCmd( byte cmd_id, void *param, int param_len )
{
    SV_Send_NetXCmd_pn( cmd_id, param, param_len, SERVER_PID );
}

// NetXCmd as server, with 2 parameters.
static
void SV_Send_NetXCmd_p2( byte cmd_id, byte param1, byte param2 )
{
    byte buf[3];

    buf[0] = param1;
    buf[1] = param2;
    SV_Send_NetXCmd( cmd_id, &buf, 2 );  // always server
}

// By Server, directly to server.
// Command sent over server text cmd channel
//  cmd_id :  X command, XD_
//  param : parameter strings
//  param_len : number of parameter strings
//  pn : to textcmd channel, by player pid, or SERVER_PID
//       Server uses SERVER_PID, but bots use their player pid.
void SV_Send_NetXCmd_pn( byte cmd_id, void *param, int param_len, byte pn )
{
    textcmd_item_t * ip;

    // [WDJ] Server sends NetXCmd to self, even without serverplayer.
    // Ver 1.48 PT_TEXTCMD format puts the pn into the packet.

    // Make a textcmd packet,
    // to use existing logic for placing into correct textcmd buffers.
    ip = & netbuffer->u.textcmdpak.textitem;
    // Header
    ip->pn = pn;  // server or bot
   
    // First byte is the cmd, followed by its parameters (binary or string).
    ip->textbuf.text[0] = cmd_id;
    if(param && param_len)
    {
       memcpy(&ip->textbuf.text[1], param, param_len);
    }
    ip->textbuf.len = 1 + param_len;

    netbuffer->packettype = PT_TEXTCMD;
    netbuffer->u.textcmdpak.num_textitem = 1;
#if 1
    // Give it directly to handler.
    net_textcmd_handler( 0 );
#else
    // Send it through bounce back buffer,
    int  bufsize = offsetof(textcmd_pak_t, textitem) + sizeof_textcmd_item_t(ip->textbuf.len);
    HSendPacket(0, SP_reliable|SP_queue|SP_error_handler, 0, bufsize );  // lost on failure
#endif
}

// [WDJ] Server also needs to send NetXCmd, even without serverplayer.
// That is now done by a separate channel, sent separately.

// By Client
// Send accumulated client textcmd to server.
// Server textcmd is sent separate.
// Called by NetUpdate
static void Send_localtextcmd( void )
{
    textcmd_item_t * ip;
    byte pind;
    byte num_textcmd;

    // How this used to work is a mystery.
    if( ! ((cl_mode == CLM_connected) && (network_state >= NETS_open)) )
        goto clear_buffer;

    // Send textcmd of main player, and splitscreen player, in one packet.
    num_textcmd = 0;  // currenly only need 2
    ip = & netbuffer->u.textcmdpak.textitem;
    for( pind=0; pind<2; pind++)
    {
        // No test for playeringame, so that quit message is not blocked.
        textbuf_t * ltcp = &localtextcmd[pind];  // local textcmd
        int tc_len = ltcp->len;  // text len
        if( tc_len == 0 )  continue;

        // Header
        ip->pn = localplayer[pind];
        memcpy(&ip->textbuf, ltcp, sizeof_textbuf_t(tc_len) );  // len, text
        // no 0 term on text
        ip = (textcmd_item_t*)((byte*)ip + sizeof_textcmd_item_t(tc_len) ); // pn, len, text
        num_textcmd ++;
    }
    if( num_textcmd == 0 )
        return;  // nothing to send

    netbuffer->packettype = PT_TEXTCMD;
    netbuffer->u.textcmdpak.num_textitem = num_textcmd;
    int  bufsize = ((byte*)ip) - ((byte*)& netbuffer->u);
    byte errcode = HSendPacket( cl_servernode, SP_reliable, 0, bufsize ); // send can fail for some reasons...
    if( errcode >= NE_fail )
        return;  // retry later

    // all extra data has been sent

    // Clear NetXCmd that would overflow the buffers.
clear_buffer:
    localtextcmd[0].len = 0;
    localtextcmd[1].len = 0;
    return;
}


#define SERVER_TIC_BASE_SIZE offsetof(netbuffer_t, u.serverpak.cmds[0])


// To keep using tic size check code until tic format is updated.
#define CHECK_TIC_SIZE

// By Server
// PT_TEXTCMD
//   nnode : the network client
// Called by Net_Packet_Handler
static void net_textcmd_handler( byte nnode )
{
    textcmd_item_t * ip = & netbuffer->u.textcmdpak.textitem;
    textcmdbuff_t  * tcbuf;
    unsigned int itc_len;  // length of text in packet
    unsigned int buflen; // length of text in storage
    int btic;
#ifdef CHECK_TIC_SIZE
    int tc_limit;  // Max size of existing textcmd that can be included with this textcmd.
#endif
    tic_t tic;
    byte num_textitem, pn;

    // Handle NetXCmd that come from clients, and some from the server.
    // Server textcmd are sent with pn==SERVER_PID, and they must always go through.
    // The server places them into tick messages here, which are sent to all clients,
    // so even if placement is arbitrary here, all clients will execute them uniformly.

#ifdef CHECK_TIC_SIZE
#else
    // [WDJ] No longer necessary to check if tic message is too large, as Send_tic
    // will handle that problem.

    // All textcmd go into the current maketic, until the storage fills up.
#endif

    num_textitem = netbuffer->u.textcmdpak.num_textitem;
    if( num_textitem > 3 )  // currently only need 2
        goto drop_packet;  // corrupt packet

    while( num_textitem-- > 0 )
    {
        // Detect corrupt packets, limit decode to netbuffer.
        // Only 2 pind, so only 2 textcmd_item, yet.
        if( ((byte*)ip) > (((byte*)& netbuffer->u.textcmdpak.textitem) + (2 * sizeof_textcmd_item_t(MAXTEXTCMD+1))) )
            goto drop_packet; // corrupt packet

        // incoming length
        itc_len = sizeof_textcmd_item_t( ip->textbuf.len );

        // Do not trust the network, or the clients.
        pn = ip->pn;
        if( pn == SERVER_PID )
        {
            // Server textcmd also should have routing pind == 2.
            // Must not route pn==0 through here due to it being used by a player.
            if( nnode != 0 )
                goto next_textcmd; // server textcmd from non-server
        }
        else if( pn > MAXPLAYERS )
        {
            goto next_textcmd; // bogus player
        }
        else
        {
            // players and bots
            // Could test for (demoplayback && (pn == 0)), but that does not gain protection.
#if 1
            if( ! playeringame[pn] )
                goto next_textcmd; // wrong player, or bad player
#else
// [WDJ] Ideal test, but bots don't set everything up yet.
            if( player_to_nnode[pn] != nnode )
                goto next_textcmd; // wrong player, or bad player
#endif
        }

#ifdef CHECK_TIC_SIZE
    // Check if tic that we are making isn't getting too large.
    tc_limit = software_MAXPACKETLENGTH
         - SERVER_TIC_BASE_SIZE
         - ( num_game_players * sizeof(ticcmd_t) )
         - ( 1 + sizeof_servertic_textcmd_t(itc_len) );

    // Search for a tic that has enough space in the ticcmd.
#endif


        for( tic = maketic;  ; )
        {
            // Almost always, the first textcmd buffer is empty.
            btic = BTIC_INDEX( tic );
            tcbuf = & textcmdbuff[btic];
            buflen = tcbuf->len;
#ifdef CHECK_TIC_SIZE
            // The incoming textbuf needs a textbuf header in the packet,
            // and that is already accounted for in the tc_limit.
            if( (buflen < tc_limit)
                && ((itc_len + buflen) < MAX_TEXTCMD_BUFF) )
                break; // found one
#else	     
            if( (itc_len + buflen) < MAX_TEXTCMD_BUFF )
                break; // found one
#endif

            // When a textbuf is too full, use the next tic.	   
            tic++;
            if( tic >= (next_tic_send+BACKUPTICS) )
                goto drop_packet;  // ran out of tic buffers
        }

        // Move textcmd from netbuffer to the textbuf.
        DEBFILE(va("Textcmd: btic %d buff[%d] player %d len %d nxttic %d maketic %d\n",
               btic, buflen, pn, itc_len, next_tic_send, maketic));
       
        // Append text to the selected buffer, combine lengths.
        memcpy(&tcbuf->buff[ buflen ], ip, itc_len);  // copy textcmd_item_t
        tcbuf->len += itc_len;  // text len

    next_textcmd:
        ip = (textcmd_item_t*)((byte*)ip + itc_len ); // pn, len, text
    }
    return;
   
drop_packet:
    // Drop the packet, let the node resend it.
    DEBFILE(va("Textcmd dropped: size %d maketic %d nxttic %d node %d player %d\n",
               itc_len, maketic, next_tic_send, nnode, pn));
    Net_Cancel_Packet_Ack(nnode);
    return;
}

// SV_Tic will send ticcmd and textcmd to client.

// Client
// Execute NetXCmd for this gametic.
void ExtraDataTicker(void)
{
    textcmdbuff_t * tcbuf;
    textcmd_item_t * ip;
    byte * bufp;
    byte * endbufp;
    byte * endtxt;  // independent of changes by called xfunc
    byte save_pn;
    int btic = BTIC_INDEX( gametic );
    int pn;
    unsigned int textlen;
    xcmd_t  xcmd;

    // Textcmd for all players, and server, are in textcmdbuff, in order of insertion.
    tcbuf = & textcmdbuff[btic];  // all players
    // set extra termination byte at end of buffer
    tcbuf->buff[MAX_TEXTCMD_BUFF] = 0;  // Protect against malicious strings.
    bufp = & tcbuf->buff[0];
    endbufp = & bufp[ tcbuf->len ];  // end of textcmdbuff content
    while( bufp < endbufp )
    {
        // Execute commands of any player in the game, and always for pn=SERVER_PID.
        ip = (textcmd_item_t*) bufp;
        textlen = ip->textbuf.len;  // 0..255
        bufp += sizeof_textcmd_item_t(textlen);  // next item (pn, len, text)
            // bufp is advanced, so can test and continue.

        if( textlen == 0 )  continue;  // empty text (should never happen)
#if MAXTEXTCMD < 255
        if( textlen > MAXTEXTCMD )   textlen = MAXTEXTCMD;  // bad length
#endif

        pn = ip->pn;  // player pid for the textcmd

        // This test causes problems for demos, where some textcmd are issued by player 0
        // before that player is created.
        if( netgame
            && !demoplayback
            && (pn < MAXPLAYERS) )
        {
            // Protection against fake textcmds in netgame.
            if( ! playeringame[pn] )  continue;   // only if player quit the game suddenly
        }

        // Commands can have 0 strings, 1 string, or 2 strings.
        // Inventory has just a byte number.
        xcmd.playernum = pn;
        xcmd.curpos = & ip->textbuf.text[0];  // start of command
        endtxt = & ip->textbuf.text[textlen];  // after last char of text
        if( endtxt >= endbufp )  endtxt = endbufp;
        xcmd.endpos = endtxt;  // end of text + 1
       
        // [WDJ] This protects against hostile clients,
        // that may try to use an unterminated string to tamper with the server.
        // To put in a term0, we have to save the pn of the next command first.
        // Saving 1 byte is just cheaper than copying the whole command to a buffer.
        save_pn = endtxt[0];
        endtxt[0] = 0;  // Protect against malicious strings.

        // One or more commands are within curpos..endpos-1
        while(xcmd.curpos < endtxt)
        {
            xcmd.cmd = *(xcmd.curpos++);  // XD_ command
            if(xcmd.cmd < MAXNETXCMD && netxcmd_func[xcmd.cmd])
            {
                // [WDJ] TODO: Test for server xcmd restricted to SERVER_PID,
                // except when demoplayback.

                // Execute a NetXCmd.
                // The NetXCmd must update xcmd.curpos.
                DEBFILE(va("Executing xcmd %d player %d ", xcmd.cmd, pn));
                (netxcmd_func[xcmd.cmd])(&xcmd);
                // nextcmd_func updates curpos, without knowing textlen
                DEBFILE("Execute done\n");
            }
            else
            {
                // [WDJ] Why should a bad demo command byte be fatal.
                I_SoftError("Got unknown net/demo command [%d]=%d len=%d\n",
                           (xcmd.curpos - &(ip->textbuf.text[0])),
                           xcmd.cmd, textlen);
#if 1
                continue;	        
#else
                D_Clear_ticcmd(btic);
                break;
#endif
            }
        }
        // Put the pn of the next command back into the buffer.
        endtxt[0] = save_pn;

        // Does not clear textbuf when done because may be server,
        // and may have to send this to other nodes again.
    }

    // cannot call update_player_counts() direct from an XNetCmd
    if( update_player_count_request )
    {
        update_player_count_request = 0;
        update_player_counts();
    }
}


// -----------------------------------------------------------------
//  end of extra data function
// -----------------------------------------------------------------

// -----------------------------------------------------------------
//  extra data function for lmps
// -----------------------------------------------------------------

// desciption of extradate byte of LEGACY 1.12 not the same of the 1.20
// 1.20 don't have the extradata bits fields but a byte for each command
// see XD_xxx in d_netcmd.h
//
// if extradatabit is set, after the ziped tic you find this :
//
//   type   |  description
// ---------+--------------
//   byte   | size of the extradata
//   byte   | LEGACY 1.13:  XDNAMEANDCOLOR, XD_WEAPON_PREF bits
//            LEGACY 1.20:  XD_ codes
//            Determines what parameters follow.
//
// LEGACY 1.12 XD bits
// if(xd & XDNAMEANDCOLOR)
//   byte   | color
//   char[MAXPLAYERNAME] | name of the player
// endif
// if(xd & XD_WEAPON_PREF)
//   byte   | original weapon switch : boolean, true if use the old
//          | weapon switch methode
//   char[NUMWEAPONS] | the weapon switch priority
//   byte   | autoaim : true if use the old autoaim system
// endif

// LEGACY 1.48 LONG TEXTCMD FORMAT
// [WDJ] During record demo, commands for each player are saved independently.
// Most textcmd will be within the 255 provided by an ordinary textbuf.
// But 1.48 has a very long combined textbuf, containing all players.
// Also the commands to create a player are now issued by SERVER_PID,
// where in older demos they were issued by player 0, before player 0 existed.

// For Ver 1.48, all textcmd are appended into one textcmd buffer.
// Separating it into separate players, appending it again is just extra work,
// without any benefit.  And SERVER_PID does not have a slot in the demo.
// It is also now possible for textcmd to run over 255 bytes, even it would
// almost never happen.
// Several ways to deal with this were considered, all with flaws, and few
// actual benefits.
// Decided to just put all textcmd into the player 0 slot, which is being
// used by the demo for server textcmds anyways.  No need to separate
// them into individual buffers again.
// The player id are embedded in the text command format already.
// There are no format conversions needed this way, and this demo format
// is only used for DoomLegacy, not compatible with other ports anyway.
// The demoversion will be the only control for the demo format.

// Save textcmd to demo using textbuf_t format.
boolean AddLmpExtradata(byte **demo_point, int playernum)
{
    textcmdbuff_t * tcbuf;
    byte * dp;  // textcmd text
   
    int btic = BTIC_INDEX( gametic );
    tcbuf = & textcmdbuff[btic];
    if( tcbuf->len == 0 )  // anything in the buffer
        return false;

#if 1
    // DoomLegacy 1.48 Textcmd Format Only
    // All textcmd are in player 0.
    if( playernum != 0 )
        return false;

    dp = *demo_point;
   
    // Uses two byte length, MSB first
    unsigned int textlen = tcbuf->len;
    dp[0] = textlen >> 8; // MSB first
    dp[1] = textlen & 0xFF;

    memcpy( &dp[2], &tcbuf->buff, textlen );  // text
    dp += textlen + 2;
#endif

   
#if 0
    // Standard Textcmd Format in demo:
    //  length: byte
    //  textbuf with multiple commands
    textcmd_t * dtc;  // textcmd in demo
    byte * bufp, * endbufp;
    unsigned int  textlen;
    unsigned int  dt_len;
    byte  dt_cnt;

    bufp = & tcbuf->buff[0];
    endbufp = & tcbuf->buff[ tcbuf->len ];

    dp = *demo_point;
    dt_cnt = 0;

    // Demo format matches textbuf_t, length limited to 255.
    dtc = dp; // textcmd in demo
    dp = &dtc->text;
    dt_len = 0;
   
    while( bufp < endbufp )
    {
        textcmd_item_t * ip = (textcmd_item_t *)bufp;
        textlen = ip->textbuf.len;
        bufp += textlen + 2;  // next item (pn, len, text)
            // advanced bufp so can test and continue

        if( ip->pn == SERVER_PID )
        {
            // SERVER_PID into player[0], for now.
            // Cannot identify them to change them back to SERVER_PID though.
            // The demo needs to have a player[0].
            if( playernum != 0 )
                continue;
        }
        else if( ip->pn != playernum )
            continue;  // wrong player

        if( textlen == 0 )  // anything in the buffer
            continue;

        // Append together when total length < 255.
        dt_len += textlen;  // appended length
        if( (dt_len > 255) && dt_cnt )
        {
            // Overruns the max textcmd in demo.
        // Need a code to indicate that there is more textcmd
        // but the next byte is the next players bit enables, which
        // conflicts with every coding.
        // This forces another byte to be used within the textcmd,
        // which violates it being compatible with the old textcmd format.
            // Start another textcmd
            dtc = dp;
            dp = &dtc->text;
            dt_len = textlen;
        }
       
        memcpy( dp, &ip->textbuf.text, textlen );  // text
        dp += textlen;
        dtc->len = dt_len;
        dt_cnt++;
    }

    if( dt_cnt == 0 )
        return false;
#endif   

    *demo_point = dp;  // update the demo ptr
    return true;
}

void ReadLmpExtraData(byte **demo_pointer, int playernum)
{
    byte * dp;
    textcmdbuff_t * tcbuf;
    textcmd_item_t * ip;
    unsigned int buflen;
    unsigned int dt_len;
    int btic = BTIC_INDEX( gametic );

    if(!demo_pointer)  goto no_text_cmd;

    dp = *demo_pointer;
    if( dp == NULL )  goto no_text_cmd;

    tcbuf = & textcmdbuff[btic];
    buflen = tcbuf->len;
    
    // [WDJ] DoomLegacy 1.48.
    // SERVER_PID commands have been written to player 0.
    // There are no markers to change them back to SERVER_PID again.
    // Fixed in execute, by disabling safety checks when playing demos.
    // Can execute server textcmd as player 0, as long as can get past checks.
    // Executing player 0 textcmd as SERVER_PID would cause errors.
#if 0
    if( (playernum == 0) && ??? )
        playernum = SERVER_PID; // because of checks
#endif

#if 1
    if( (demoversion >= 148) && (playernum == 0) )
    {
        // DoomLegacy 1.48 Textcmd Format Only
        // All textcmd are in player 0, enforced by writer.

        // Uses two byte length, MSB first
        unsigned int textlen = (dp[0] << 8) + dp[1];

        if( textlen > MAX_TEXTCMD_BUFF )
            return; // cannot be valid textcmd

        // Do not have to append as only the demo can be entering textcmds.
        memcpy( &tcbuf->buff, &dp[2], textlen );  // text
        tcbuf->len = textlen;
        dp += textlen + 2;
        goto read_done;
    }
#endif


    // [WDJ] Read older demo code.
    // See Clean up textcmd, in TryRunTics, that allows this to just append.
    ip = (textcmd_item_t *)  & tcbuf->buff[ buflen ];
    ip->pn = playernum;

    dt_len = dp[0];   // demo textbuf->len

    if( (buflen + dt_len) >= (MAX_TEXTCMD_BUFF - 2) )
        goto no_text_cmd; // will not fit

    // [WDJ] Clean separation of old and new formating.
    if(demoversion==112) // support old demos v1.12
    {
        // Different, limited, XCmd format.
        byte  * p = dp;
        byte  * bufp = & ip->textbuf.text[0];  // new textbuf in textcmdbuff
        byte  ex;
        unsigned int textlen = 0;

        // dt_len is length of extra data, incl length.
        ex = p[1];  // XCmd bits
        p += 2;  // skip dt_len and XCmd
        if(ex & 1)
        {
            *(bufp++) = XD_NAMEANDCOLOR;
            memcpy( bufp, p, MAXPLAYERNAME+1);
            p+=MAXPLAYERNAME+1;
            textlen += MAXPLAYERNAME+1;
            bufp += MAXPLAYERNAME+1;
        }
        if(ex & 2)
        {
            *(bufp++) = XD_WEAPONPREF;
            memcpy( bufp, p, NUMWEAPONS+2);
            p+=NUMWEAPONS+2;
            textlen += NUMWEAPONS+2;
        }
        ip->textbuf.len = textlen;
        buflen += textlen + 2;
    }
    else
    {
        // DoomLegacy <= 1.47
        // Demo format matches textbuf_t, length limited.
        // dt_len is textbuf->len, excluding len field.
        dt_len ++;
        memcpy( &ip->textbuf, dp, dt_len);  // len,text
        buflen += dt_len + 1;
    }
    tcbuf->len = buflen;
    dp += dt_len;

read_done:
    // update demo pointer
    *demo_pointer = dp;
    return;

no_text_cmd:
    return;
}


// -----------------------------------------------------------------
//  end extra data function for lmps
// -----------------------------------------------------------------

// ----- Server/Client Responses
static void CL_ConnectToServer(void);
static int16_t  Consistency(void);
static void Net_Packet_Handler(void);
static void SV_Reset_NetNode(byte nnode);
static boolean SV_Add_Join_Waiting(void);
static void SV_Send_player_desc( player_desc_t * pdesc, byte desc_flags, byte pn, byte to_node );
static void CL_init_playerdesc_receive( uint32_t r_gametic );
static void CL_player_desc_handler( player_desc_t * pdesc, const char * msg );


// By Server
// Broadcast to all connected nodes
//   flags: SP_ for HSendPacket
static void SV_SendPacket_All( sendpacket_flag_e flags, size_t size_packet, const char * msg )
{
    int nn;
    for(nn=1; nn<MAXNETNODES; nn++)
    {
        if( nnode_state[nn] >= NOS_recognized )
        {
            HSendPacket( nn, flags, 0, size_packet );  // ignore failures
            if( msg )	   
                debug_Printf( "%s[ %d ]\n", msg, nn );
        }
    }
}

// Server, Client
// send a simple packet
//   nnode : client node, cl_servernode, BROADCAST_NODE
// Return  NE_xx
static byte  SendPacket( byte nnode, byte packettype )
{
    netbuffer->packettype = packettype;

    if( nnode == BROADCAST_NODE )  // only by server
    {
        SV_SendPacket_All( SP_reliable|SP_queue|SP_error_handler, 0, NULL );
        return NE_success;
    }

    if( nnode < MAXNETNODES )
        return HSendPacket( nnode, SP_reliable|SP_queue|SP_error_handler, 0, 0 );

    return NE_fail;
}


// Flags used by Join msg.
typedef enum {
  NF_drone = 0x80,
  NF_big_endian = 0x40,
  NF_download_savegame = 0x10,
} join_flags_e;

// By Client.
// Send a request to join game.
// Called by CL_ConnectToServer.
static boolean  CL_Send_Join( void )
{
    byte flg = 0;
    if( cv_download_savegame.EV )  flg |= NF_download_savegame;
    if( cl_drone )  flg |= NF_drone;
#ifdef __BIG_ENDIAN__
    flg |= NF_big_endian;
#endif

    GenPrintf(EMSG_hud, "Send join request...\n");
    netbuffer->packettype=PT_CLIENTJOIN;
    netbuffer->u.clientcfg.version = VERSION;
    netbuffer->u.clientcfg.ver1 = 0;
    netbuffer->u.clientcfg.ver2 = VERSION;
    netbuffer->u.clientcfg.ver3 = REVISION;
    netbuffer->u.clientcfg.subversion = LE_SWAP32(NETWORK_VERSION);
    netbuffer->u.clientcfg.mode = 0;
    netbuffer->u.clientcfg.flags = flg;

    // Declare how many players at this node.
    // [Arcade] The panel's control sets, not the render split: cv_splitscreen
    // only says whether two *views* are drawn.
    netbuffer->u.clientcfg.num_node_players =
        (cl_drone)? 0 : D_NumLocalPlayers();

    byte errcode = HSendPacket( cl_servernode, SP_reliable|SP_queue|SP_error_handler, 0, sizeof(clientconfig_pak_t) );
    return  (errcode < NE_fail);
}


// ----- Server Info

// By Server.
// Reply to request for server info.
//   reqtime : the send time of the request
static void SV_Send_ServerInfo(int to_node, tic_t reqtime)
{
    byte * p;

    netbuffer->packettype=PT_SERVERINFO;
    netbuffer->u.serverinfo.version = VERSION;
    netbuffer->u.serverinfo.ver1 = 0;
    netbuffer->u.serverinfo.ver2 = VERSION;
    netbuffer->u.serverinfo.ver3 = REVISION;
    netbuffer->u.serverinfo.subversion = LE_SWAP32(NETWORK_VERSION);
    // return back the time value so client can compute their ping
    netbuffer->u.serverinfo.trip_time = LE_SWAP32(reqtime);
    netbuffer->u.serverinfo.num_active_players = num_game_players;
    netbuffer->u.serverinfo.maxplayer = cv_maxplayers.value;
    netbuffer->u.serverinfo.load = 0;        // unused for the moment
    netbuffer->u.serverinfo.deathmatch = cv_deathmatch.EV;  // menu setting
    dl_strncpy(netbuffer->u.serverinfo.servername, cv_servername.string, MAXSERVERNAME);
    if(game_map_filename[0])
    {
        // Map command external wad file.
        strcpy(netbuffer->u.serverinfo.mapname,game_map_filename);
    }
    else
    {
        // existing map       
        strcpy(netbuffer->u.serverinfo.mapname,G_BuildMapName(gameepisode,gamemap));
    }

    p=Put_Server_FileNeed();

    HSendPacket( to_node, 0, 0, p - ((byte *)& netbuffer->u) );  // msg lost when too busy
}


// By Server.
// Accept player joining the game.
// Send the initial command with the config to avoid sending a second message,
// which could arrive out-of-order.
//   command : CTRL_ command to send with config
static boolean SV_Send_ServerConfig( byte to_node, byte command )
{
    xcmd_t xc;

#if 1
    // Send mask for all players and bots
    int   i,playermask=0;
    for(i=0;i<MAXPLAYERS;i++)
    {
         if(playeringame[i])
              playermask|=1<<i;
    }
    write_N32( & netbuffer->u.servercfg.playerdetected, playermask );
#else
    // Send mask for all players, but not bots.
    update_player_counts();   // not necessary, already done
    write_N32( & netbuffer->u.servercfg.playerdetected, ticcmd_player_mask );
#endif

    netbuffer->packettype=PT_SERVERCFG;
    netbuffer->u.servercfg.version         = VERSION;
    netbuffer->u.servercfg.ver1 = 0;
    netbuffer->u.servercfg.ver2 = VERSION;
    netbuffer->u.servercfg.ver3 = REVISION;
    netbuffer->u.servercfg.subversion      = LE_SWAP32(NETWORK_VERSION);

    netbuffer->u.servercfg.serverplayer    = serverplayer;
    netbuffer->u.servercfg.num_game_players= num_game_players;
    write_N32( & netbuffer->u.servercfg.gametic, gametic );
    netbuffer->u.servercfg.clientnode      = to_node;  // client node (in server space)
    netbuffer->u.servercfg.gamestate       = gamestate;
    netbuffer->u.servercfg.command         = command;

    xc.playernum = 0;
    xc.curpos = netbuffer->u.servercfg.netvar_buf;
    xc.endpos = xc.curpos + NETVAR_BUFF_LEN - 1;
    CV_SaveNetVars( &xc );
    // curpos is 1 past last cvar (if none then is at netvar_buf)

    byte errcode = HSendPacket( to_node, SP_reliable|SP_queue|SP_error_handler, 0, xc.curpos - ((byte *)& netbuffer->u) );
    return  (errcode < NE_fail);
}



#ifdef WAIT_GAME_START_INTERMISSION

// By Server.
// Send the state of all players.
static void SV_Send_PlayerState( byte to_node )
{
    // Which players are in game, which are bots.
    // Get name and color by asking nodes for refresh.
    update_player_counts();

    netbuffer->packettype = PT_SERVERPLAYER;
    write_N32( & netbuffer->u.playerstate.gametic, gametic );

    netbuffer->u.playerstate.serverplayer = serverplayer;

    // Easier to determine state when all player slots are present.
    // byte array of player_state
    // Identifies player and bot.
    memcpy( &netbuffer->u.playerstate.playerstate, player_state, MAXPLAYERS );
   
    // Only those players that are present
    // Sends one or more packets.
    SV_Send_player_desc( &netbuffer->u.playerstate.player_desc, PDI_inventory, 255, to_node );
}


// By Client
// PT_SERVERPLAYER
static
void  server_player_handler( byte nnode )
{
    byte pn;

    // Do not know what to do with gametic yet.
    unsigned int sph_gametic = read_N32( & netbuffer->u.playerstate.gametic );
    // Ideally this should be done at the matching gametic.
    CL_init_playerdesc_receive( sph_gametic );
    
    serverplayer = netbuffer->u.playerstate.serverplayer;
    gameskill = netbuffer->u.playerstate.skill;  // for create bots
   
    // Add and remove players
    for( pn=0; pn<MAXPLAYERS; pn++ )
    {
        update_player_state( pn, netbuffer->u.playerstate.playerstate[pn] );
    }
    update_player_counts();
   
    // Receive all player desc.
    CL_player_desc_handler( &netbuffer->u.playerstate.player_desc, NULL );
}
#endif


// By Server.
// Send the level state.
static void SV_Send_LevelCfg( byte to_node )
{
    netbuffer->packettype = PT_SERVERLEVEL;

    // The game state, level map identity and name.
    netbuffer->u.levelcfg.gamestate = gamestate;
    netbuffer->u.levelcfg.gameepisode = gameepisode;
    netbuffer->u.levelcfg.gamemap = gamemap;
    netbuffer->u.levelcfg.skill = gameskill;
    netbuffer->u.levelcfg.nomonsters = nomonsters;
    netbuffer->u.levelcfg.deathmatch = cv_deathmatch.EV;

#if 0   
    // TODO: support changing mapname by server
    // TODO: allow server to cycle through various map and map files.
    if(game_map_filename[0])
    {
        // Map command external wad file.
        strcpy(netbuffer->u.levelcfg.mapname,game_map_filename);
    }
    else
    {
        // existing map       
        strcpy(netbuffer->u.levelcfg.mapname,G_BuildMapName(gameepisode,gamemap));
    }
#endif   

    HSendPacket(to_node, SP_reliable|SP_queue|SP_error_handler, 0, sizeof(levelcfg_pak_t) );
    // nothing to be done if this fails
}

// By Client
// PT_SERVERLEVEL
static
void  server_level_handler( byte nnode )
{
    // level info should not change during game, only during intermission or pause
    gamestate = netbuffer->u.levelcfg.gamestate;
    gameepisode = netbuffer->u.levelcfg.gameepisode;
    gamemap = netbuffer->u.levelcfg.gamemap;
    gameskill = netbuffer->u.levelcfg.skill;
    nomonsters = netbuffer->u.levelcfg.nomonsters;
    CV_SetParam( &cv_deathmatch, netbuffer->u.levelcfg.deathmatch );
    // TODO: support changing mapname by server
    // TODO: allow server to cycle through various map and map files.
}


// ----- Control Message

// Network commands
typedef enum {
    CTRL_state,  // no node command
    CTRL_normal,
    CTRL_download_savegame,
    CTRL_wait_game_start,
    CTRL_game_start,
    CTRL_wait_timer,
} net_control_command_e;

// By Server
//   ctrl_command : net_control_e
//   to_gamestate : new gamestate, command dependent
//   data1 : 8 bit value
//   data2 : 16 bit value, timer value or other data specific to the command
//   player_num : player num (255 when unused)
static void SV_Send_control( byte nnode, byte ctrl_command, byte data1, uint16_t data2, byte player_num )
{
    netbuffer->packettype = PT_CONTROL;
    netbuffer->u.control.command = ctrl_command;
    netbuffer->u.control.player_num = player_num;
    netbuffer->u.control.player_state = (player_num < MAXPLAYERS)? player_state[player_num] : PS_unused;
    netbuffer->u.control.gamemap = gamemap;
    netbuffer->u.control.gameepisode = gameepisode;
    write_N32( & netbuffer->u.control.gametic, gametic );
    write_N16( & netbuffer->u.control.data2, data2 );
    netbuffer->u.control.data1 = data1;

    if( nnode == BROADCAST_NODE )
    {
        // Broadcast it
        SV_SendPacket_All( SP_reliable, sizeof(control_pak_t), NULL );
    }
    else if( nnode < MAXNETNODES )
    {
        byte errcode = HSendPacket( nnode, SP_reliable|SP_queue, 0, sizeof(control_pak_t) );
        if( errcode >= NE_fail )
        {
            generic_network_error_handler( errcode, "Control msg" );  // should not happen
        }
    }
}

// By Client (maybe client on server)
static void control_msg_handler( void )
{
    // Command from server to client.
    byte pn = netbuffer->u.control.player_num;
    byte ps = netbuffer->u.control.player_state;
    uint16_t data2 = read_N16( & netbuffer->u.control.data2 );

    if( !server )
    {
        // Client not on server.
        // When sent a player state, update it on client.
        if( pn < MAXPLAYERS )
            update_player_state( pn, ps );

        if( cl_mode < CLM_connected )
        {
            // Initialize tics on new client.	   
            gametic = read_N32( & netbuffer->u.control.gametic );
            cl_need_tic = maketic = gametic;  // sync
        }

        gamemap = netbuffer->u.control.gamemap;
        gameepisode = netbuffer->u.control.gameepisode;
    }

    // Situation specific
    switch( netbuffer->u.control.command )
    {
     case CTRL_normal:
        if( cl_mode < CLM_connected )
            cl_mode = CLM_connected;
        break;
#ifdef WAIT_GAME_START_INTERMISSION
     case CTRL_wait_game_start:
        if( (cl_mode < CLM_connected) && (ps == PS_join_wait_game_start) )
        {
            cl_mode = CLM_wait_game_start;

            if( server )  break;  // protection, should not happen

            wait_netplayer = 0;  // turn off wait display
            gamestate = GS_INTERMISSION;
            // TODO: watch another player
        }
        break;
     case CTRL_game_start:
        if( cl_mode == CLM_wait_game_start )
        {
            cl_mode = CLM_connected;

            if( server )  break;  // protection, should not happen

            byte mode = netbuffer->u.control.data1;
            gametic = read_N32( & netbuffer->u.control.gametic );
            cl_need_tic = maketic = gametic;  // sync

            if( mode )
            {
                // Invoked by Map command at server, when there are wait_game_start players.
#if 0
// [WDJ] TODO: NOT WORKING YET.
// Map command is overriding in server, but is missing in client.

                // Map command will take it to GS_LEVEL.
                gamestate = GS_NULL;
//                G_DoLoadLevel(1);  // if can get map first ?
#endif
            }
            else
            {
                // Server in GS_INTERMISSION.
                G_Start_Intermission();  // setup intermission
            }

            // wait timer only works in GS_INTERMISSION
            // update from server
            wait_game_start_timer = data2;
//          wait_netplayer = 0;  // only want the timer display

            // Warn the player, who probably has dozed off by now.	   
//            S_StartSound(sfx_menuop);  // always present
            S_StartSound(sfx_telept);  // longer sound, likely to be in all games
        }
        break;
#endif
     case CTRL_wait_timer:
        if( server )  break;  // protection, should not happen

        wait_game_start_timer = data2;
        break;
     default:
        break;
    }
}


// ----- Random State Update

// Send: Fill in the fields
static void get_random_state( random_state_t * rs )
{
    uint32_t rand2;
    rs->p_rand_index = P_Rand_GetIndex(); // to sync P_Random
    rs->b_rand_index = B_Rand_GetIndex(); // to sync B_Random
    write_N32( &rs->e_rand1, E_Rand_Get( & rand2 ) ); // to sync E_Random
    write_N32( &rs->e_rand2, rand2 );
}

typedef enum  {
   RSC_server_report, // Server reporting a client message.
   RSC_client_set     // Client, sets values from server
}  checkset_e;

// Client
// Receive: Update client random state.
//   mode : checkset_e
static void
  random_state_checkset( random_state_t * rs, const char * msg, byte mode )
{
    uint32_t  o_ernd1, o_ernd2, rs_ernd1, rs_ernd2;
    byte  o_pr, o_br, rs_pr, rs_br;
    // RSC_server_report
    const char * u1 = "server";
    const char * u2 = "client";
   
    if( mode == RSC_client_set )
    {
        // Client, set to server values
        u1 = "update client";
        u2 = "to server";
    }

    o_pr = P_Rand_GetIndex();
    rs_pr = rs->p_rand_index;
    if( o_pr != rs_pr )
    {
        // Warn when different P_Random index.
        GenPrintf( EMSG_warn, "%s: gametic %i, %s P_random index %i %s %i\n",
             msg, gametic, u1, o_pr, u2, rs_pr );
        if( mode == RSC_client_set )
            P_Rand_SetIndex( rs_pr ); // to sync P_Random
    }
   
    o_br = B_Rand_GetIndex();
    rs_br = rs->b_rand_index;
    if( o_br != rs_br )
    {
        GenPrintf( EMSG_warn, "%s: %s B_Random index %i %s %i\n", msg, u1, o_br, u2, rs_br );
        if( mode == RSC_client_set )
            B_Rand_SetIndex( rs_br ); // to sync B_Random
    }

    o_ernd1 = E_Rand_Get( & o_ernd2 );
    rs_ernd1 = read_N32( &rs->e_rand1 );
    rs_ernd2 = read_N32( &rs->e_rand2 );
    if( o_ernd1 != rs_ernd1 || o_ernd2 != rs_ernd2 )
    {
        GenPrintf( EMSG_warn, "%s: %s E_Random (%08X,%08X) %s (%08X,%08X)\n",
                   msg, u1, o_ernd1, o_ernd2, u2, rs_ernd1, rs_ernd2 );
        if( mode == RSC_client_set )
            E_Rand_Set( rs_ernd1, rs_ernd2 ); // to sync E_Random
    }
}


// ----- Gametic and Pause State

// [WDJ] Update state by sever.
// By Server
void SV_Send_State( byte server_pause )
{
    netbuffer->packettype=PT_STATE;
    write_N32( & netbuffer->u.state.gametic, gametic );
    get_random_state( & netbuffer->u.state.rs ); // to sync P_Random
    netbuffer->u.state.server_pause = server_pause;

    SV_SendPacket_All( SP_reliable, sizeof(state_pak_t), NULL );
}

// By Client
static void state_handler( void )
{
    byte nnode = doomcom->remotenode;

    // Message is PT_STATE
    tic_t serv_gametic = read_N32( & netbuffer->u.state.gametic );
    if( serv_gametic != gametic )
    {
        if( verbose > 1 )
            GenPrintf( EMSG_ver, "PT_STATE: node %i, client gametic %i, server gametic %i\n", nnode, gametic, serv_gametic );
        // Gametic cannot be fixed directly, need game commands.        
        // It may just be behind by a tic or two.
    }
    else
    {
        // Update the random generators.
        random_state_checkset( & netbuffer->u.state.rs, "PT_STATE", RSC_client_set ); // to sync P_Random
    }
    paused = netbuffer->u.state.server_pause;

#if 0
    debug_Printf( "STATE: gametic %i, P_Random [%i], paused %i\n",
          gametic, netbuffer->u.state.rs.p_rand_index, netbuffer->u.state.server_pause );
#endif
}


// ----- Wait for Client Update to Finish

// NETWORK_WAIT_ACTIVE_FLAG is carefully choosen to avoid boolean values 0/1
#define NETWORK_WAIT_ACTIVE_FLAG   0x40
static uint16_t  network_wait_timer = 0;
static byte      network_wait_pause = 0;

// Server
//  wait_timeout : wait timeout in ticks
void  SV_network_wait_timer( uint16_t wait_timeout )
{
    if( wait_timeout > network_wait_timer )   network_wait_timer = wait_timeout;

    if( ! network_wait_pause )
    {
        // save pause state, and network pause state
        network_wait_pause = paused | NETWORK_WAIT_ACTIVE_FLAG;
        if( ! paused )
        {
            // All clients except the server.
            SV_Send_State( 1 );  // pause game
        }
        paused = network_wait_pause;
    }
}

// Server
// Wait when netgame is forced to pause.
static
void  SV_network_wait_handler( void )
{
    byte nn, ns;
   
    // Is everybody ready.
    for( nn=0; nn<MAXNETNODES; nn++ )
    {
        ns = nnode_state[nn];
        if((ns >= NOS_join) && (ns <= NOS_join_sg_loaded))  goto keep_waiting;
        if((ns >= NOS_repair) && (ns <= NOS_repair_sg_loaded))  goto keep_waiting;
    }
    goto unpause_game;
    
keep_waiting:
    if( network_wait_timer > 1 )
    {
        network_wait_timer--;
        return;
    }
        
    // Timeout the wait
    network_wait_timer = 0;
    for( nn=0; nn<MAXNETNODES; nn++ )
    {
        byte ns = nnode_state[nn];
        if( ns == NOS_idle ) continue;
       
        GenPrintf(EMSG_warn, "Network timeout: node=%i  nnode_state=%i\n", nn, ns );
        if((ns >= NOS_join) && (ns < NOS_join_timeout))
        {
            nnode_state[nn] = NOS_join_timeout;
            network_wait_timer = 2;
        }
        else if((ns >= NOS_repair) && (ns < NOS_repair_timeout))
        {
            nnode_state[nn] = NOS_repair_timeout;
            network_wait_timer = 2;
        }
        else if( ns == NOS_repair_timeout || ns == NOS_join_timeout || ns == NOS_fatal )
        {
            Net_CloseConnection(nn, 1); // force close
            SV_Reset_NetNode(nn);
        }
    }

    if( network_wait_timer == 0 )  goto unpause_game;  // let other players continue
    return;

unpause_game:
    if( network_wait_pause == NETWORK_WAIT_ACTIVE_FLAG )  // originally was not paused
    {
        // All clients except the server.
        SV_Send_State( 0 );  // unpause the game
    }
    paused &= ~NETWORK_WAIT_ACTIVE_FLAG;
    network_wait_pause = 0;
    network_wait_timer = 0;
    return;
}


// By Server
static void  ready_handler( byte nnode )
{
    byte  ns = nnode_state[nnode];

    // Client is ready
    if( ns == NOS_join_savegame )
    {
        nnode_state[nnode] = NOS_join_sg_loaded;  // signal to SV_Add_Join_Waiting()
        SV_Add_Join_Waiting();
    }
}

// ---- Client Update Player
// This supports network repair and client update.

// By Server.
//  mo : the mobj to be sent
//  mpp : mobj_pos_t in a packet message
static void SV_get_mobj( mobj_t * mo, /*OUT*/ mobj_pos_t * mpp )
{
    if( mo == NULL )
    {
        memset( mpp, 0, sizeof(mobj_pos_t) );
        mpp->momz = LE_SWAP32(0x7FFF7FFF);  // too large to be valid
        return;
    }

    // Player or monster position.
    mpp->angle = LE_SWAP32( mo->angle );
    mpp->x = LE_SWAP32( mo->x );
    mpp->y = LE_SWAP32( mo->y );
    mpp->z = LE_SWAP32( mo->z );
    mpp->momx = LE_SWAP32( mo->momx );
    mpp->momy = LE_SWAP32( mo->momy );
    mpp->momz = LE_SWAP32( mo->momz );

    return;
}

// By Client.
//   mpp : mobj_pos_t in a packet message
//   objtype : create this type of object, else 0xFFFF
//   msg : report message header, if NULL then no reports
//   mo : the mobj being received, may be NULL
//   mopp : write the new mobj, if it is created, may be NULL
// Called by CL_player_repair, CL_player_desc_handler.
static void  CL_set_mobj( mobj_pos_t * mpp, unsigned int objtype, const char * msg, /*OUT*/ mobj_t * mo, mobj_t ** mopp )
{
    angle_t  r_angle = LE_SWAP32( mpp->angle );
    fixed_t  r_x = LE_SWAP32( mpp->x );
    fixed_t  r_y = LE_SWAP32( mpp->y );
    fixed_t  r_z = LE_SWAP32( mpp->z );
    fixed_t  r_momx = LE_SWAP32( mpp->momx );
    fixed_t  r_momy = LE_SWAP32( mpp->momy );
    fixed_t  r_momz = LE_SWAP32( mpp->momz );
    byte  setthingpos = 0;

    // [WDJ] During GS_LEVEL, need mobj as position is critical.
    // During GS_INTERMISSION, position does not matter, and mobj is not needed.
    if( ! mo )
    {
        if((objtype >= NUMMOBJTYPES) || (mopp == NULL))
            return;

        // mobj is missing, create new objtype
#if 1
        // SpawnMobj needs sector map information.
        // When not GS_LEVEL, cannot know if level map is present yet.
        if( gamestate != GS_LEVEL )  return;

        mo = P_SpawnMobj(r_x, r_y, r_z, objtype);
        // set thing pos already done by P_SpawnMobj
        *mopp = mo;  // save new mobj
#else
        // incomplete
        mo = Z_Malloc(sizeof(*mobj), PU_LEVEL, NULL);
        *mopp = mo;  // save new mobj
        memset(pp->mo, 0, sizeof(mobj_t));
        if( gamestate == GS_LEVEL )
            setthingpos = 1;
#endif
        msg = NULL;  // no checks on new mobj
    }
    else if( gamestate == GS_LEVEL )
    {
        P_UnsetThingPosition(mo);
        setthingpos = 1;
    }

    // Only check and report when it is repair.
    if( msg )
    {
        if( mo->angle != r_angle )
        {
            GenPrintf( EMSG_warn, "%s: ANGLE  client %x  server %x\n", msg, mo->angle, r_angle );
        }

        if(  mo->x != r_x || mo->y != r_y || mo->z != r_z )
        {
            GenPrintf( EMSG_warn, "%s: POS  client (%x.%x, %x.%x, %x.%x)  server (%x.%x, %x.%x, %x.%x)\n", msg,
                   mo->x>>16, mo->x&0xFFFF, mo->y>>16, mo->y&0xFFFF, mo->z>>16, mo->z&0xFFFF,
                   r_x>>16, r_x&0xFFFF, r_y>>16, r_y&0xFFFF, r_z>>16, r_z&0xFFFF  );
        }

        if(  mo->momx != r_momx || mo->momy != r_momy || mo->momz != r_momz )
        {
            GenPrintf( EMSG_warn, "%s: MOM  client (%x.%x, %x.%x, %x.%x)  server (%x.%x, %x.%x, %x.%x)\n", msg,
                   mo->momx>>16, mo->momx&0xFFFF, mo->momy>>16, mo->momy&0xFFFF, mo->momz>>16, mo->momz&0xFFFF,
                   r_momx>>16, r_momx&0xFFFF, r_momy>>16, r_momy&0xFFFF, r_momz>>16, r_momz&0xFFFF  );
        }
    }

    mo->angle = r_angle;
    mo->x = r_x;
    mo->y = r_y;
    mo->z = r_z;
    mo->momx = r_momx;
    mo->momy = r_momy;
    mo->momz = r_momz;

    if( setthingpos )
    {
        P_SetThingPosition(mo);  // set subsector, links
    }
}

// Only in network message.
typedef enum {
    PF_BACKPACK = 0x08,
    PF_AUTOAIM = 0x10,
    PF_ORIGWEAPONSWITCH = 0x20,
} player_state_flags_e;

// Return player_state_flags_e
static byte  get_player_state_flags( player_t *  pp )
{
    // Convert to network message flags
    byte flags = 0;
    uint16_t gf = pp->GF_flags;  // autoaim, originalweaponswitch, backpack
    if( gf & GF_autoaim )  flags |= PF_AUTOAIM;
    if( gf & GF_original_weapon )  flags |= PF_ORIGWEAPONSWITCH;
    if( gf & GF_backpack )  flags |= PF_BACKPACK;
    return flags;
}

// Receive
static uint32_t  playerdesc_gametic = 0;
static byte  playerdesc_seq;   // bit per tic packet seq, ready=0xFF
static byte  check_output_warn;

// Initialize the playerdesc receive (conditionally on gametic).
static
void  CL_init_playerdesc_receive( uint32_t r_gametic )
{
    if( r_gametic != playerdesc_gametic )
    {
        playerdesc_seq = 0;
        playerdesc_gametic = r_gametic;
    }
}

static
void check_output( unsigned int cv, unsigned int nv, const char * field, const char * msg  )
{
    if( cv != nv )
    {
        if( ! check_output_warn )
        {
            check_output_warn = 1;
            GenPrintf( EMSG_warn, "%s: ", msg );
        }
        GenPrintf( EMSG_warn, "  %s  client %x  server %x\n", field, cv, nv );
    }
}


// By Server.
// Send the state of one or all players, within the caller supplied packet.
// Packettype is already filled in by caller.
//   pdesc : position of player_desc_t in netbuffer
//   desc_flags : playerdesc_flags_e
//   pn : player pid, or 255 for all players
//   to_node : dest node
static void SV_Send_player_desc( player_desc_t * pdesc, byte desc_flags, byte pn, byte to_node )
{
    player_t *  pp;
    byte *  bufp;
    byte seq_num = 0;
    byte pn_end;
    byte errcode;
    int i;
    int entry_count;
    unsigned int paksize;
    unsigned int paksize_limit;
   
    // Limit how many can be in one packet.
    paksize_limit = sizeof( netbuffer->u );
    if( paksize_limit > software_MAXPACKETLENGTH )   paksize_limit = software_MAXPACKETLENGTH;
    paksize_limit -= offsetof(pd_player_t, optional) + 1;

    if( EN_inventory )
    {
        paksize_limit -= sizeof(pd_inventory_t);
    }
    else   
    {
        desc_flags &= ~ PDI_inventory;
    }
   
    // Only those players that are present
    entry_count = 0;
    paksize = 0;
    bufp = (byte*) &pdesc->pd;

    if( pn < MAXPLAYERS )
    {
        // one player
        pn_end = pn;
    }
    else
    {
        // all players
        pn = 0;
        pn_end = MAXPLAYERS;
    }

    for(  ; pn<pn_end; pn++ )
    {
        if( ! playeringame[pn] )  continue;

        // As of 1.48: sizeof(netbuffer->u) = 4160, max paksize = 3072
        if( paksize > paksize_limit )
        {
            // Would exceed netbuffer, send as two packets
            pdesc->desc_flags = PDI_more | desc_flags | seq_num;
            errcode = HSendPacket(to_node, SP_reliable|SP_queue|SP_error_handler, 0, paksize);
            if( errcode >= NE_fail )  goto abort_send;
            entry_count = 0;
            seq_num++;
        }

        // Player state that would be valid during intermission, or repair.
        pp = &players[pn];
        if( ! pp )  continue;

        bufp = ALIGN4(bufp); // align to 4 bytes

        pd_player_t * pdp = (pd_player_t*) bufp;
        pdp->pid = pn;
        pdp->playerstate = pp->playerstate;  // DEAD
        pdp->health = LE_SWAP16( pp->health );
        pdp->armor = LE_SWAP16( pp->armorpoints );
        pdp->armortype = pp->armortype;
        pdp->readyweapon = pp->readyweapon;
        pdp->flags = get_player_state_flags( pp );

        SV_get_mobj( pp->mo, &pdp->pos );

        uint32_t wo = 0;
        for( i=0; i<NUMWEAPONS; i++)  // 19
            if( pp->weaponowned[i] )  wo |= 1<<i;
        pdp->weaponowned = LE_SWAP32( wo );
       
        for( i=0; i<NUMAMMO; i++ )
        {
            pdp->ammo[i] = LE_SWAP16( pp->ammo[i] );
            pdp->maxammo[i] = LE_SWAP16( pp->maxammo[i] );
        }

        // advance bufp upto the optional field
        bufp += offsetof( pd_player_t, optional );

        // Optional unaligned
        if( desc_flags & PDI_inventory )
        {
            pd_inventory_t * pdip = (pd_inventory_t*) bufp;
            pdip->inventoryslotnum = pp->inventorySlotNum;

            // Because it is a structure it might be aligned on some machines
            for( i=0; i<NUMINVENTORYSLOTS; i++ )
            {
                pdip->inventory[i].type = pp->inventory[i].type;
                pdip->inventory[i].count = pp->inventory[i].count;
            }
            bufp += sizeof( pd_inventory_t );
        }
       
        entry_count ++;
        pdesc->entry_count = entry_count;
        paksize = bufp - ((byte *)& netbuffer->u);
    }

    pdesc->desc_flags = desc_flags | seq_num;
    errcode = HSendPacket(to_node, SP_reliable|SP_queue|SP_error_handler, 0, paksize);
    if( errcode >= NE_fail )  goto abort_send;
    return;

abort_send:
    // clean up
    return;
}


// By Client
// PT_SERVERPLAYER, or PT_REPAIR
//   msg : repair msg, otherwise NULL
static
void  CL_player_desc_handler( player_desc_t * pdesc, const char * msg )
{
    player_t *  pp;
    byte * bufp;
    int i;
    int entry_count = 0;
    byte desc_flags;
    byte seqbits;
    byte pn;

    desc_flags = pdesc->desc_flags;
    seqbits = 1 << (desc_flags & PDI_seq);
    if( ! (desc_flags & PDI_more) )  seqbits = -seqbits;  // 0x01=>0xFF, 0x08=>0xF8
    playerdesc_seq |= seqbits;  // record receiving this packet

    // Only those players that are present
    bufp = (byte*) &pdesc->pd;
    entry_count = pdesc->entry_count;
    for( ; entry_count > 0; entry_count-- )
    {
        bufp = ALIGN4(bufp); // align to 4 bytes

        pd_player_t * pdp = (pd_player_t*) bufp;
        bufp += offsetof( pd_player_t, optional );

        pn = pdp->pid;
        if( pn >= MAXPLAYERS )  break;
       
        pp = &players[pn];
        if( ! playeringame[pn] || ! pp )
        {
            // should not happen, skip over this entry
            if( desc_flags & PDI_inventory )
                bufp += sizeof(pd_inventory_t);
            continue;
        }

        // Player state that would be valid during intermission.
        check_output_warn = 0;  // reset msg header

        uint16_t r_health = LE_SWAP16( pdp->health );
        uint16_t r_armorpoints = LE_SWAP16( pdp->armor );

        if( msg )
        {
            check_output( pp->playerstate, pdp->playerstate, "PLAYERSTATE", msg );
            check_output( pp->health, r_health, "HEALTH", msg );
            check_output( pp->armorpoints, r_armorpoints, "ARMORPOINTS", msg );
            check_output( pp->armortype, pdp->armortype, "ARMORTYPE", msg );
            check_output( pp->readyweapon, pdp->readyweapon, "READYWEAPON", msg );
        }

        pp->playerstate = pdp->playerstate;  // DEAD
        pp->health = r_health;
        pp->armorpoints = r_armorpoints;
        pp->armortype = pdp->armortype;
        pp->readyweapon = pdp->readyweapon;
       
        CL_set_mobj( &pdp->pos, MT_PLAYER, msg, /*OUT*/ pp->mo, &(pp->mo) );
       
        byte flags = pdp->flags;
        if( msg )
        {
            byte cflags = get_player_state_flags( pp );
            check_output( cflags, flags, "FLAGS", msg );
        }

        {
            uint16_t gf = 0;
            pp->GF_flags &= ~(GF_backpack|GF_autoaim|GF_original_weapon);
            if( flags & PF_AUTOAIM )  gf |= GF_autoaim;
            if( flags & PF_ORIGWEAPONSWITCH )  gf |= GF_original_weapon;
            if( flags & PF_BACKPACK )  gf |= GF_backpack;
            pp->GF_flags |= gf;
        }

        uint32_t wo = LE_SWAP32( pdp->weaponowned );
        uint32_t cwo = 0;
        for( i=0; i<NUMWEAPONS; i++)  // 19
        {
            if( pp->weaponowned[i] )  cwo |= 1<<i;
            pp->weaponowned[i] = ( wo & (1<<i) )? 1:0;
        }
        if( msg )
        {
            check_output( cwo, wo, "WEAPONOWNED", msg );
        }

        for( i=0; i<NUMAMMO; i++ )
        {
            uint16_t r_ammo = LE_SWAP16( pdp->ammo[i] );
            uint16_t r_maxammo = LE_SWAP16( pdp->maxammo[i] );
            if( msg )
            {
                // Not going to be specific about which ammo is wrong.
                check_output( pp->ammo[i], r_ammo, "AMMO", msg );
                check_output( pp->maxammo[i], r_maxammo, "MAXAMMO", msg );
            }
            pp->ammo[i] = r_ammo;
            pp->maxammo[i] = r_maxammo;
        }

        // Optional unaligned
        if( desc_flags & PDI_inventory )
        {
            pd_inventory_t * pdip = (pd_inventory_t*) bufp;
            if( msg )
            {
                check_output( pp->inventorySlotNum, pdip->inventoryslotnum, "INVENTORYSLOTNUM", msg );
            }
            pp->inventorySlotNum = pdip->inventoryslotnum;

            // Because it is a structure it might be aligned on some machines
            for( i=0; i<NUMINVENTORYSLOTS; i++ )
            {
                if( msg )
                {
                    // Not going to be specific about which inventory is wrong.
                    check_output( pp->inventory[i].type, pdip->inventory[i].type, "INVENTORY_TYPE", msg );
                    check_output( pp->inventory[i].count, pdip->inventory[i].count, "INVENTORY_COUNT", msg );
                }
                pp->inventory[i].type = pdip->inventory[i].type;
                pp->inventory[i].count = pdip->inventory[i].count;
            }
            bufp += sizeof(pd_inventory_t);
        }
    }
}


// ---- Client Update Savegame

#ifdef JOININGAME

// [WDJ] For little-endian and big-endian machines to play together, must fix savegames.
// Each machine writes savegames in its native format, and must be able
// to read its own older savegames, so cannot just change to LE savegames.
// A big-endian machine needs to be able to read either endian savegames.
// Other possibilities require also changing the write of savegames, at least for big-endian servers.

// By Server.
// Send a save game to the client.
static void SV_Send_SaveGame(int to_node)
{
    size_t  length;

    SV_network_wait_timer( 90 );  // pause game during download

    P_Alloc_savebuffer( 1 );	// large buffer, but no header
    if(! savebuffer)   goto buffer_err;

    P_Savegame_Write_header( NULL, 1 );  // Netgame header
    P_Savegame_Save_game();  // fill buffer with game data
    // buffer will automatically grow as needed.

    length = P_Savegame_length();
    if( length >= SAVEBUF_OVERFLOW_LENGTH )
        goto buffer_err;	// overrun buffer

    // then send it !
    SV_SendData(to_node, "SAVEGAME", savebuffer, length, TAH_MALLOC_FREE, SAVEGAME_FILEID);
    // SendData frees the savebuffer using free() after it is sent.
    // This is the only use of TAH_MALLOC_FREE.

    // Wait for reply. Let wait timer do unpause.
    return;

buffer_err:
    GenPrintf(EMSG_error, "Send_savegame: cannot allocate large enough buffer\n" );
    return;   
}

// Dummy name for a savegame file.
static const char *tmpsave="$$$.sav";

// By Client.
// Act upon the received save game from server.
// Success: cl_mode = CLM_download_done
// Fail:    cl_mode = CLM_fatal
static void CL_Load_Received_Savegame(void)
{
    savegame_info_t   sginfo;  // read header info

    // Use savebuffer and save_p from p_saveg.c.
    // There cannot be another savegame in progress when this occurs.
    // [WDJ] Changed to use new load savegame file, with smaller buffer.
    if( P_Savegame_Readfile( tmpsave ) < 0 )  goto cannot_read_file;
    // file is open and savebuffer allocated

    // Read netgame header.
    sginfo.msg[0] = 0;
    if( ! P_Savegame_Read_header( & sginfo, 1 ) )  goto load_failed;

    GenPrintf(EMSG_hud, "Loading savegame\n");

    G_setup_VERSION();

    // Sever will control pause during download.
    demoplayback  = false;
    automapactive = false;

    // load a base level
    playerdeadview = false;

    P_Savegame_Load_game(); // read game data in savebuffer, defer error test
    if( P_Savegame_Closefile( 0 ) < 0 )  goto load_failed;
    // savegame buffer deallocated, and file closed

    // done
    unlink(tmpsave);  // delete file (posix)
    consistency[ BTIC_INDEX( gametic ) ] = Consistency();
    CON_ToggleOff ();

    cl_mode = CLM_download_done;
    return;

cannot_read_file:
    I_SoftError ("Can't read savegame sent\n");
    goto failed_exit; // must deallocate savebuffer

load_failed:
    GenPrintf(EMSG_error, "Can't load the level !!!\n%s", sginfo.msg);
failed_exit:
    // needed when there are error tests before Closefile.
    P_Savegame_Error_Closefile();  // deallocate savebuffer
    cl_mode = CLM_fatal;
    return;
}

#endif



// ----- Consistency fail, repair position.

static repair_type_e  active_repair_type = 0;

static void  fill_repair_header( byte repair_type )
{
    netbuffer->packettype = PT_REPAIR;
    netbuffer->u.repair.repair_type = repair_type;
    // The fields are there, so fill them.  Easier to fill them everytime.
    write_N32( & netbuffer->u.repair.gametic, gametic );
    get_random_state( & netbuffer->u.repair.rs ); // to sync P_Random
}

// By Server.
// Send a player repair message.
//   pn : player_id, 255 for all players
//   severity : 0..3, see cv_netrepair
//   to_node : the net node
static void SV_Send_player_repair( byte pn, byte severity, byte to_node )
{
    const char * fail_msg = "";
    byte desc_flags = PDI_inventory;

    if( severity < 2 )  desc_flags = 0;
    if( cv_SV_netrepair.EV < 2 )  desc_flags = 0;

    if( pn < MAXPLAYERS )
    {
        if( ! playeringame[pn] )
        {
            fail_msg = ", not in game";
            goto msg_out;
        }

        mobj_t * mo = players[pn].mo;
        if( ! mo )
        {
            fail_msg = ", no mobj";
            goto msg_out;
        }
    }

    // Enough to fix a small difference in player position.
    fill_repair_header( RQ_PLAYER );
    SV_Send_player_desc( & netbuffer->u.repair.u.player_desc, desc_flags, pn, to_node );

msg_out:
    if( pn < MAXPLAYERS )
    {
        GenPrintf( EMSG_warn, "Server Send_player_repair: player %i%s\n", pn, fail_msg );
    }
    else
    {
        GenPrintf( EMSG_warn, "Server Send_player_repair: ALL player\n", pn );
    }
    return;
}

// By Client.
// Repair the player from the repair message in the netbuffer.
static void CL_player_repair( void )
{
    // ignore random state for now

    // Compare gametic as packet field, not as a tic_t
    uint32_t  r_gametic = read_N32( & netbuffer->u.repair.gametic );

    // Receive all player desc.
    CL_init_playerdesc_receive( r_gametic );
    CL_player_desc_handler( & netbuffer->u.repair.u.player_desc, "Client player_repair" );
}


// By Server.
//  repair_type : RQ_SUG_SAVEGAME
//  to_node : to the player node
static void SV_Send_repair( byte repair_type, byte to_node )
{
    // simple messages
    fill_repair_header( repair_type );
    byte errcode = HSendPacket(to_node, SP_reliable|SP_queue, 0, sizeof(repair_pak_t));
    if( errcode >= NE_fail )
    {
        nnode_state[to_node] = NOS_fatal;
        generic_network_error_handler( errcode, "Send Repair" );
    }
    return;
}


// By Client.
//  repair_type : RQ_REQ_SAVEGAME, RQ_REQ_PLAYER, RQ_CLOSE_ACK
static void CL_Send_Req_repair( repair_type_e repair_type )
{
    fill_repair_header( repair_type );
    playerdesc_seq = 0;  // ready to receive
    byte errcode = HSendPacket(cl_servernode, SP_reliable|SP_queue, 0, sizeof(repair_pak_t));
    if( errcode >= NE_fail )
    {
        network_state = NETS_fatal;
        generic_network_error_handler( errcode, "Send Repair" );
    }
}


// [WDJ] Attempt to fix consistency errors.
// PT_REPAIR
// By Client
static void repair_handler_client( byte nnode )
{
    // Message is PT_REPAIR
    byte rq_type = netbuffer->u.repair.repair_type;

    if( server )
        return;  // Ignore attempts to corrupt server.

    if( rq_type < RQ_REQ_TO_SERVER )
    {
        // Server repairs client.
        uint32_t net_gametic = read_N32( & netbuffer->u.repair.gametic );
        if( gametic != net_gametic )
        {
            GenPrintf( EMSG_warn, "Client repair: gametic client %u  server %u\n", gametic, net_gametic );
            gametic = net_gametic;
        }
        random_state_checkset( & netbuffer->u.repair.rs, "Client repair", RSC_client_set ); // to sync P_Random
    }

    switch( rq_type )
    {
     case RQ_SUG_SAVEGAME:
#ifdef JOININGAME
        // CLIENT: Server suggests downloading a savegame from the server.
        if(( cv_download_savegame.EV == 0 ) || ( cv_netrepair.EV < 2 ))
        {
            CL_Send_Req_repair( RQ_REQ_PLAYER ); // to server
            break;
        }

        // Client: prepare for savegame download.
        cl_mode = CLM_download_savegame;
        CL_Prepare_download_savegame(tmpsave);
        CL_Send_Req_repair( RQ_REQ_SAVEGAME ); // to server
        // Loop here while savegame is downloaded.
        while( netfile_download )
        {
            Net_Packet_Handler();
            if( network_state < NETS_active )  // Need client to server connection
                goto reset_to_title_exit;  // connection closed by cancel or timeout
#if 1
            if( !server && !netgame )
                goto reset_to_title_exit;  // connection closed by cancel or timeout
#endif

            Net_AckTicker();

            // Operations performed only once every tic.
            if( cl_prev_tic != I_GetTime() )
            {
                cl_prev_tic = I_GetTime();

                // User response handler
                I_OsPolling();
                switch( I_GetKey() )
                {
                  case 'q':
                  case KEY_ESCAPE:
                     goto reset_to_title_exit;
                }

                // Server upkeep
                if( Filetx_file_cnt )  // File download in progress.
                    Filetx_Ticker();
            }
        }

        // Have received the savegame from the server.
        CL_Load_Received_Savegame();  // CLM_download_done, or CLM_fatal
        if( cl_mode != CLM_download_done )
        {
            GenPrintf( EMSG_error, "Client player_repair: savegame failed\n" );
            if( cv_netrepair.EV < 3 )  goto reset_to_title_exit;
            // aggressive
            if( cl_error_status > 2 )  goto reset_to_title_exit;
            cl_error_status = 3;  // retry once
            // Try again
            CL_Send_Req_repair( RQ_REQ_SAVEGAME ); // to server
            break;
        }
        // Download done
        goto close_repair;
#else
        CL_Send_Req_repair( RQ_REQ_PLAYER ); // to server
#endif
        break;

     case RQ_SAVEGAME_REJ:
        // Server rejects savegame.
        CL_Send_Req_repair( RQ_REQ_PLAYER ); // to server
        break;

     case RQ_PLAYER:
        // Server repairs player position.
        CL_player_repair();
        consistency[ BTIC_INDEX( gametic ) ] = Consistency();
        if( playerdesc_seq == 0xFF )  goto close_repair; // all parts of player desc received
        break;

     default:
        break;
    }
    return;

close_repair:
    cl_mode = CLM_connected;

    // Client closes the repair state. 
    if( cl_mode != CLM_fatal )
        CL_Send_Req_repair( RQ_CLOSE_ACK ); // to server
    cl_error_status = 1;
    active_repair_type = RQ_NULL;
    return;

reset_to_title_exit:
    if( cl_mode != CLM_fatal )
        D_Quit_NetGame(); // to server
    CL_Reset();
    D_StartTitle();
    active_repair_type = RQ_NULL;
    return;
}

// PT_REPAIR
// By Server only.
static void repair_handler_server( byte nnode )
{
    // Message is PT_REPAIR
    random_state_checkset( & netbuffer->u.repair.rs, "Report", RSC_server_report ); // to check P_Random

    byte rq_type = netbuffer->u.repair.repair_type;
    switch( rq_type )
    {
     case RQ_REQ_SAVEGAME:
        // Client has requested a savegame repair
#ifdef JOININGAME
        if( (cv_SV_download_savegame.EV == 0) || (cv_SV_netrepair.EV < 2) )
        {
            GenPrintf(EMSG_info, "Repair: Send savegame not allowed\n" );
        }
        else if(gamestate == GS_LEVEL)
        {
            nnode_state[nnode] = NOS_repair_savegame;
            SV_Send_SaveGame( nnode ); // send game data
            // netwait timer is running
            GenPrintf(EMSG_info, "Repair: Send savegame\n");
            // Client will return  RQ_REQ_SAVEGAME, RQ_REQ_PLAYER, RQ_CLOSE_ACK, or PT_CLIENTQUIT
            break;	    
        }
#else
        GenPrintf(EMSG_info, "Repair: Send savegame not allowed, no Join-in-game.\n" );
#endif
        SV_Send_repair( RQ_SAVEGAME_REJ, nnode );
        break;
       
     case RQ_REQ_ALLPLAYER:
        netbuffer->u.repair.u.player_id = 255;  // all players in game
     case RQ_REQ_PLAYER:
        // Client has requested a player repair
        SV_network_wait_timer( 18 );  // keep alive
        nnode_state[nnode] = NOS_repair_player;
        SV_Send_player_repair( netbuffer->u.repair.u.player_id, 3, nnode );
        break;

     case RQ_CLOSE_ACK:
        // Client is ending the repair.
//        random_state_checkset( & netbuffer->u.repair.rs, "Repair Close", RSC_server_report ); // to check P_Random
        nnode_state[nnode] = NOS_active;
        GenPrintf(EMSG_warn, "Repair: closed.\n" );
        SV_network_wait_handler();
        break;

     default:
        break;
    }
}


// ----- Wait for Server to start net game.
//#define WAITPLAYER_DEBUG

static byte  num_netnodes;
static byte  num_netplayer;  // wait for netplayer, some nodes are 2 players
static tic_t wait_tics  = 0;

// By Server
static void SV_Send_NetWait( void )
{
    netbuffer->packettype = PT_NETWAIT;
    netbuffer->u.netwait.num_netplayer = num_netplayer;
    netbuffer->u.netwait.wait_netplayer = wait_netplayer;
    netbuffer->u.netwait.wait_tics = LE_SWAP16( wait_tics );
    get_random_state( & netbuffer->u.netwait.rs ); // to sync P_Random
#ifdef WAITPLAYER_DEBUG
    debug_Printf( "WaitPlayer update: num_netnodes=%d num_netplayer=%d  wait_netplayer=%d  wait_tics=%d\n",
               num_netnodes, num_netplayer, wait_netplayer, wait_tics );
    SV_SendPacket_All( 0, sizeof(netwait_pak_t), "  sent to player" );
#else
    SV_SendPacket_All( 0, sizeof(netwait_pak_t), NULL );
#endif
}

// By Client
static void netwait_handler( void )
{
    // Updates of wait for players, from the server.
    num_netplayer = netbuffer->u.netwait.num_netplayer;
    wait_netplayer = netbuffer->u.netwait.wait_netplayer;
    wait_tics = LE_SWAP16( netbuffer->u.netwait.wait_tics );
    random_state_checkset( & netbuffer->u.netwait.rs, "Client netwait", RSC_client_set ); // to sync P_Random
}

// Called from D_Display when GS_WAITINGPLAYERS, CL_ConnectToServer?
void D_WaitPlayer_Drawer( void )
{
    WI_Draw_wait( num_netnodes, num_netplayer, wait_netplayer, wait_tics );
}

// Called by Menu M_StartServer
void D_WaitPlayer_Setup( void )
{
    if( netgame )
    {
        if( server )
        {
            // Wait for all player nodes, during netgame.
            wait_netplayer = cv_wait_players.value;
            wait_tics = cv_wait_timeout.value * TICRATE;
        }
        else
        {
            // Wait indefinite, until server updates the wait.
            wait_netplayer = 99;
            wait_tics = 0;
        }
    }
    else
    {
        // Single player and local games.
        wait_netplayer = 0;
        wait_tics = 0;
    }
    gamestate = wipegamestate = GS_WAITINGPLAYERS;
}

// Return true when start game.
static boolean  D_WaitPlayer_Ticker(void)
{
    int  nn;

    if( server )
    {
        // Count the net nodes.
        num_netnodes=0;
        num_netplayer=0;
        for(nn=0; nn<MAXNETNODES; nn++)
        {
            // Only counting nodes with players.
            if( nnode_state[nn] >= NOS_client )
            {
                if( playerpernode[nn] > 0 )
                {
                    num_netnodes ++;
                    num_netplayer += playerpernode[nn];
                }
            }
        }

        if( wait_tics > 0 || wait_netplayer > 0 )
        {
            // Service the wait tics.
            if( wait_tics > 1 )
            {
                wait_tics--;  // count down to 1
            }

            static  byte net_update_cnt = 0;
            if( ++net_update_cnt > 4 )
            {
                net_update_cnt = 0;
                // Update all net nodes
                SV_Send_NetWait();
            }
        }
    }

    // Clients and Server do same tests.
    if( wait_netplayer > 0 )
    {
        // Waiting on player net nodes, with or without timeout.
        if( num_netplayer < wait_netplayer )
        {
            // Waiting for player net nodes.
            if( wait_tics != 1 )  // timeout at 1
               goto wait_ret;  // waiting only for number of players
        }
    }
    else if( wait_tics > 1 )
        goto wait_ret;  // waiting for players by timeout

    if( server )
    {
        // All nodes need to get info to stop waiting.
        SV_Send_NetWait();
#ifdef WAITPLAYER_DEBUG
        debug_Printf( "Start game sent to players at tic=%d\n", gametic   );
#endif
    }
    return true;  // start game
    

wait_ret:
    return false;  // keep waiting
}

boolean  D_WaitPlayer_Response( int key )
{
    // [WDJ] This should not respond to the ENTER key.  Users will be pressing ENTER quickly and often.
    // This message will stop them, and only the correct response will start the unreversible action.

    // Translate for joystick.  The PAUSE is not otherwise used during WAITPLAYER and does not conflict.
    if( key == gamecontrol[gc_pause][0] || key == gamecontrol[gc_pause][1]
        || (key >= KEY_JOY0BUT2 && key <= KEY_JOY0BUT5 ) )
        key = 's';
   
    // User response handler
    switch( key )
    {
     // Also gc_menuesc (translated to KEY_ESCAPE)
     case 'q':
     case KEY_ESCAPE:
        if( ! dedicated )
        {
            D_Quit_NetGame();
            SV_StopServer();
            SV_ResetServer();
            D_StartTitle();
            netgame = multiplayer = false;
            return true;
        }
        break;
     case 's':
        if( server )
        {
            // Start game, stop waiting for player nodes.
            wait_netplayer = 0;
            wait_tics = 0;
            SV_Send_NetWait();
#ifdef WAITPLAYER_DEBUG
            debug_Printf( "Start game (key) sent at tic=%d\n", gametic );
#endif
            return true;
        }
        break;
    }
    return false;
}


// ----- Connect to Server

// By Client.
// Ask the server for some info.
//   to_node : when BROADCAST_NODE then all servers will respond
// Called by: CL_Broadcast_AskInfo, CL_Update_ServerList, CL_ConnectToServer
static void CL_Send_AskInfo( byte to_node )
{
    netbuffer->packettype = PT_ASKINFO;
    netbuffer->u.askinfo.version = VERSION;
    netbuffer->u.askinfo.ver1 = 0;
    netbuffer->u.askinfo.ver2 = VERSION;
    netbuffer->u.askinfo.ver3 = REVISION;
    netbuffer->u.askinfo.send_time = LE_SWAP32(I_GetTime());
    HSendPacket( to_node, 0, 0, sizeof(askinfo_pak_t) );  // msg lost when too busy
}


// By Client.
// Broadcast to find some servers.
//   addrstr: broadcast addr string
// Called by: CL_Update_ServerList
static void CL_Broadcast_AskInfo( char * addrstr )
{
    // Modifies the broadcast address.
    if( addrstr
        && Bind_Node_str( BROADCAST_NODE, addrstr, server_sock_port ) )
    {
//        debugPrintf( "CL_Broadcast_AskInfo  server_sock_port = %d\n", server_sock_port );       
       
        CL_Send_AskInfo( BROADCAST_NODE );
    }
}


// --- ServerList

server_info_t serverlist[MAXSERVERLIST];
int serverlistcount=0;

// Clear the serverlist, closing connections.
//  keep_node: except this server node
static void SL_Clear_ServerList( int keep_node )
{
    int i;
    for( i=0; i<serverlistcount; i++ )
    {
        if( serverlist[i].server_node != keep_node )
        {
            Net_CloseConnection(serverlist[i].server_node, 0);
            serverlist[i].server_node = 0;
        }
    }
    serverlistcount = 0;
}

// Find the server in the serverlist.
// Called by: SL_InsertServer, CL_ConnectToServer
static int SL_Find_Server( byte nnode )
{
    int i;
    for( i=0; i<serverlistcount; i++ )
    {
        if( serverlist[i].server_node == nnode )
            return i;
    }

    return -1;
}

// Insert the server into the serverlist.
static void SL_InsertServer( serverinfo_pak_t * info, byte nnode)
{
    tic_t  test_time;
    server_info_t se;
    int i, i2;

    // search if not already on it
    i = SL_Find_Server( nnode );
    if( i < 0 )
    {
        // not found add it
        if( serverlistcount >= MAXSERVERLIST )
            return; // list full
        i=serverlistcount++;  // last entry
    }

    // Update info
    serverlist[i].info = *info;
    serverlist[i].server_node = nnode;

    // List is sorted by trip_time (has been converted to ping time)
    // so move the entry until it is sorted (shortest time to [0]).
    se = serverlist[i];  // this is always the updated entry
    test_time = info->trip_time;
    for(;;) {
        i2 = i;  // prev position of updated entry
        if( i>0
            && test_time < serverlist[i-1].info.trip_time )
        {
            i--;
        }
        else
        if( (i+1)<serverlistcount
            && test_time > serverlist[i+1].info.trip_time )
        {
            i++;
        }
        else
            break;  // done
        serverlist[i2] = serverlist[i];  // move other to prev position
        serverlist[i] = se;  // new position
    }
}

// By user, future Client.
// Called by M_Connect.
void CL_Update_ServerList( boolean internetsearch )
{
    int  i;

    SL_Clear_ServerList(0);

    if( !netgame )
    {
        server = false;  // To get correct port
        if( ! I_NetOpenSocket() )  return;  // failed to get socket

        netgame = true;
        multiplayer = true;
        network_state = NETS_open;
        quit_netgame_status = 0;
    }

    // Search for local servers.
    CL_Broadcast_AskInfo( cv_server1.string );
    CL_Broadcast_AskInfo( cv_server2.string );
    CL_Broadcast_AskInfo( cv_server3.string );

    if( internetsearch )
    {
        msg_server_t *server_list;

        server_list = MS_Get_ShortServersList();
        if( server_list )
        {
            // Poll the servers on the list to get ping time.
            for (i=0; server_list[i].header[0]; i++)
            {
                int  node;
                char addr_str[24];

                // insert ip (and optionaly port) in node list
                sprintf(addr_str, "%s:%s", server_list[i].ip, server_list[i].port);
                node = I_NetMakeNode(addr_str);
                if( node >= MAXNETNODES )
                    break; // no more node free

                CL_Send_AskInfo( node );
            }
        }
    }
}



// ----- Connect to Server

// By User, future Client, and by server not dedicated.
// Use adaptive send using net_bandwidth and stat.sendbytes.
// Called by Command_connect, SV_SpawnServer
//   cl_servernode: if set then reconnect, else search
static void CL_ConnectToServer( void )
{
    int  i;
    tic_t   askinfo_tic = 0;  // to repeat askinfo

    if( network_state < NETS_open )
         goto reset_to_title_exit;  // connection closed by cancel or timeout

    network_state = NETS_connecting;
   
    cl_mode = CLM_searching;
    D_WaitPlayer_Setup();

    if( cl_servernode >= MAXNETNODES )
    {
        // init value and BROADCAST_NODE
        GenPrintf(EMSG_hud, "Searching for a DoomLegacy server ...\n");
    }
    else
        GenPrintf(EMSG_hud, "Contacting the DoomLegacy server ...\n");

    DEBFILE(va("Waiting %d players\n", wait_netplayer));

    SL_Clear_ServerList(cl_servernode);  // clear all except the current server

    GenPrintf(EMSG_hud, "Press Q or ESC to abort\n");
    // Every player goes through here to connect to game, including a
    // single player on the server.
    // Loop until connected or user escapes.
    // Because of the combination above, this loop must include code for
    // server responding.
    do
    {
        // Because of console draw in the loop
        V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );

        switch(cl_mode) {
            case CLM_searching :
                // serverlist is updated by GetPacket function
                if( serverlistcount > 0 )
                {
                    cl_mode = CLM_server_files;
                    break;
                }
                // Don't have a serverlist.
                // Poll the server (askinfo packet).
                if( askinfo_tic <= I_GetTime() )
                {
                    // Don't be noxious on the network.
                    // Every 2 seconds is often enough.
                    askinfo_tic = I_GetTime() + (TICRATE*2);
                    if( cl_servernode < MAXNETNODES )
                    {
                        // Specific server.
                        CL_Send_AskInfo(cl_servernode);
                    }
                    else
                    {
                        // Any
                        CL_Update_ServerList( false );
                    }
                }
                break;
            case CLM_server_files :
                // Have a serverlist, serverlistcount > 0.
                // This can be a response to our broadcast request
                if( cl_servernode < MAXNETNODES )
                {
                    // Have a current server.  Find it in the serverlist.
                    i = SL_Find_Server(cl_servernode);
                    // Check if it shutdown, or is missing for some reason.
                    if (i<0)
                        return;  // go back to user
                }
                else
                {
                    // Invalid cl_servernode, get best server from serverlist.
                    i = 0;
                    cl_servernode = serverlist[i].server_node;
                    GenPrintf(EMSG_hud, " Found, ");
                }
                // Check server for files needed.
                CL_Got_Fileneed(serverlist[i].info.num_fileneed,
                                serverlist[i].info.fileneed    );
                GenPrintf(EMSG_hud, " Checking files ...\n");
                switch( CL_CheckFiles() )
                {
                 case CFR_no_files:
                 case CFR_all_found:
                    cl_mode = CLM_askjoin;
                    break;
                 case CFR_download_needed:
                    cl_mode = CLM_download_req;
                    break;
                 case CFR_iwad_error: // cannot join for some reason
                 case CFR_insufficient_space:
                 default:
                    goto reset_to_title_exit;
                }
                break;
            case CLM_download_req:
                // Must download something.
                // Check -nodownload switch, cv_download_files, or request downloads.
                switch( Send_RequestFile() )
                {
                 case RFR_success:
                    Net_GetNetStat();  // init for later display
                    cl_mode = CLM_download_files;
                    break;
                 case RFR_send_fail:
                    break;  // retry later
                 case RFR_insufficient_space: // Does not seem to be used.
                 case RFR_nodownload:
                 default:
                    // Due to -nodownload switch, or other fatal error.
                    goto  reset_to_title_exit;
                }
                break;
            case CLM_download_files :
                // Wait test, and display loading files.
                if( CL_waiting_on_fileneed() )
                    break; // continue looping
                // Have all downloaded files.
                cl_mode = CLM_askjoin;
                // continue into next case
            case CLM_askjoin :
                if( ! CL_Load_ServerFiles() )
                {
                    // Cannot load some file.
                    goto  reset_to_title_exit;
                }
#ifdef JOININGAME
                // prepare structures to save the file
                // WARNING: this can be useless in case of server not in GS_LEVEL
                // but since the network layer don't provide ordered packet ...
                // This can be repeated, if CL_Send_Join fails.
                CL_Prepare_download_savegame(tmpsave);
#endif
                if( CL_Send_Join() )  // join game
                    cl_mode = CLM_wait_join_response;
                if( network_state < NETS_connecting )  // Need client to server connection
                    goto reset_to_searching;
#if 1
                if( !server && !netgame )
                    goto reset_to_searching;
#endif
                break;
            case CLM_wait_join_response :
                // waiting for server response	   
                // see server_cfg_handler()
                break;
            case CLM_download_savegame :
                // client, download a savegame
#ifdef JOININGAME
                M_DrawTextBox( 2, NETFILE_BOX_Y, 38, 4);
                V_DrawString (30, NETFILE_BOX_Y+8, 0, "Download Savegame");
                if( netfile_download )
                    break; // continue loop

                // Have received the savegame from the server.
                CL_Load_Received_Savegame();
                if( cl_mode != CLM_download_done )
                {
                    // send client quit
                    goto reset_to_searching;
                }

                // got savegame
                if( SendPacket( cl_servernode, PT_CLIENTREADY ) >= NE_fail )
                    goto reset_to_searching;

                gamestate = GS_LEVEL;  // game loaded
                cl_mode = CLM_connected;
                break;
#else	   
                goto reset_to_searching;  // should not end up in this state
#endif

#ifdef WAIT_GAME_START_INTERMISSION
            case CLM_wait_game_start :
                // Not going to get a download.
                if( netfile_download )
                    CL_Cancel_download_savegame();

                M_DrawTextBox( 2, NETFILE_BOX_Y, 38, 4);
                V_DrawString (30, NETFILE_BOX_Y+8, 0, "Wait Game Start");
                // wait for control msg
                break;
#endif

            default:
                break;
            // CLM_connected will exit the loop
        }

        Net_Packet_Handler();
        if( network_state < NETS_connecting )  // Need client to server.
            goto reset_to_searching;  // connection closed by cancel or timeout
        if( !server && !netgame )
            goto reset_to_searching;  // connection closed by cancel or timeout

        Net_AckTicker();

        // Operations performed only once every tic.
        if( cl_prev_tic != I_GetTime() )
        {
            cl_prev_tic = I_GetTime();

            // User response handler
            I_OsPolling();
            switch( I_GetKey() )
            {
              case 'q':
              case KEY_ESCAPE:
                 goto quit_ret;
            }

            if( Filetx_file_cnt )  // File download in progress.
                Filetx_Ticker();

#if 1
   if( cl_mode > CLM_searching )
   {
      // Nothing bad, just not the problem
            // Supporting the wait during connect, like it was in the previous
            // code, has marginal value.  Seems to cause more problems.
            D_WaitPlayer_Ticker();
            if( wait_tics > 0 || wait_netplayer > 0 )
                D_WaitPlayer_Drawer();
   }
#endif
            CON_Drawer ();
            I_FinishUpdate ();              // page flip or blit buffer
        }
    } while ( cl_mode != CLM_connected );

    // If this is still set, then did not get a savegame, so turn it off.
    if( netfile_download )
        CL_Cancel_download_savegame();
   
    network_state = NETS_active;

    DEBFILE(va("Synchronization Finished\n"));

    // [WDJ] consoleplayer no longer has DRONE bit
    displayplayer = consoleplayer;
    consoleplayer_ptr = displayplayer_ptr = &players[consoleplayer];
    return;

reset_to_searching:
    if( cl_mode >= CLM_download_req )
    {
        SendPacket( cl_servernode, PT_CLIENTQUIT );  // ignore failure
    }
    cl_mode = CLM_searching;
    return;

quit_ret:
    M_SimpleMessage ("Network game synchronization aborted.\n\nPress ESC\n");
    goto reset_to_title_exit;

reset_to_title_exit:
    CL_Reset();
    D_StartTitle();
    return;
}


// By User, future Client.
void Command_connect(void)
{
    if( COM_Argc()<2 )
    {
        CONS_Printf ("connect <serveraddress> : connect to a server\n"
                     "connect ANY : connect to the first lan server found\n"
                     "connect SELF: connect to self server\n");
        return;
    }
    server = false;
    quit_netgame_status = 0;

    if( strcasecmp(COM_Argv(1),"self")==0 )
    {
        cl_servernode = 0;  // preset connect to server on self
        cl_server_state = NOS_internal;  // otherwise may reject
        server = true;
        // should be but...
        //SV_SpawnServer();
    }
    else
    {
        // used in menu to connect to a server in the list
        if( netgame && strcasecmp(COM_Argv(1),"node")==0 )
        {
            // example: "connect node 4"
            cl_servernode = atoi(COM_Argv(2));
        }
        else
        if( netgame )
        {
            CONS_Printf("You cannot connect while in netgame\n"
                        "Leave this game first\n");
            return;
        }
        else
        {
            I_NetOpenSocket();
            netgame = true;
            multiplayer = true;
            network_state = NETS_open;

            if( strcasecmp(COM_Argv(1),"any")==0 )
            {
                // Connect to first lan server found.
                cl_servernode = BROADCAST_NODE;
            }
            else
            if( I_NetMakeNode )
            {
                // Connect to server at IP addr.
                cl_servernode = I_NetMakeNode(COM_Argv(1));
                if( cl_servernode >= MAXNETNODES )
                {
                    // Error from I_NetMakeNode()
                    CONS_Printf("Network address error\n");
                    goto close_connection;
                }
            }
            else
            {
                CONS_Printf("There is no server identification with this network driver\n");
                goto close_connection;
            }
        }
    }
    CL_ConnectToServer();
    return;

close_connection:
    D_CloseConnection();
    network_state = NETS_idle;
    return;
}


// Called by Kick cmd.
static void CL_RemovePlayer( byte playernum )
{
    player_t * player;
    int i;

    // Cannot trust an index from a network message.
    if( playernum >= MAXPLAYERS )  return;
    player = & players[playernum];
   
    if( server && !demoplayback )
    {
        byte nnode = player_to_nnode[playernum];
        if( playerpernode[nnode] )
            playerpernode[nnode]--;
        if( playerpernode[nnode] == 0 )
        {
            // No more players at this node.	   
            Net_CloseConnection(nnode, 0);
            SV_Reset_NetNode(nnode);  // node_state
        }
    }

    playeringame[playernum] = false;
    player_state[playernum] = PS_unused;
    {
        byte pind;   // [Arcade] any of the local slots may hold this player
        for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )
        {
            if( localplayer[pind] == playernum )   localplayer[pind] = 255;
        }
    }
    player_to_nnode[playernum] = 255;

    // we should use a reset player but there is not such function
    // Count the players.
    update_player_counts();

    for(i=0;i<MAXPLAYERS;i++)
    {
        players[i].addfrags += players[i].frags[playernum];
        players[i].frags[playernum] = 0;
        player->frags[i] = 0;
    }
    player->addfrags = 0;

    // remove avatar of player
    if( player->mo )
    {
        player->mo->player = NULL;
        P_RemoveMobj( player->mo );
    }
    player->mo = NULL;
   
    B_Destroy_Bot( player );
}

// By Client and non-specific code, to reset client connect.
void CL_Reset (void)
{
    if (demorecording)
        G_CheckDemoStatus ();

    // Reset client
    DEBFILE(va("==== Client reset ====\n"));

    if( cl_servernode < MAXNETNODES )
    {
        // Close connection to server
        // Client keeps nnode_state for server.	
        cl_server_state = NOS_idle;
        Net_CloseConnection(cl_servernode, 0);
    }
    D_CloseConnection();         // netgame=false
    cl_servernode = 0;  // preset client to server on self, but not connected
    cl_error_status = 0;

    // Enable self server
    multiplayer = false;
    server=true;
#ifdef DOSNET_SUPPORT
    doomcom->num_player_netnodes=1;
#endif
    SV_StopServer();
    SV_ResetServer();

    T_Clear_HubScript(); //DarkWolf95: Originally implemented by Exl
    fs_fadealpha = 0;
    HU_Clear_FSPics();

    // reset game engine
    //D_StartTitle ();
}

void Command_PlayerInfo(void)
{
    int i;

    for(i=0;i<MAXPLAYERS;i++)
    {
        if(playeringame[i])
        {
            // Mark the server player in white
            CONS_Printf("%snum:%2d  node:%2d  %s\n",
                        (serverplayer==i)?"\2":"",
                        i, player_to_nnode[i], player_names[i]);
        }
    }
}

// Name can be player number, or player name.
// Players 0..(MAXPLAYERS-1) are known as Player 1 .. to the user.
// Return player number, 0..(MAXPLAYERS-1).
// Return 255, and put msg to console, when name not found.
byte  player_name_to_num( const char * name )
{
    // Player num can be 0..250 (limited to MAXPLAYERS).
    int pn;  // player num

    pn=atoi(name);   // test as player number 1..MAXPLAYERS
    // pn=0 when not numeric
    if((pn > 0) && (pn <= MAXPLAYERS))
    {
        // Name was numeric, and in player num range
        pn --;  // convert to 0..MAXPLAYERS
        if( ! playeringame[pn])  goto no_player;
        return pn;
    }

    // Search for player by name.
    for(pn=0; pn<MAXPLAYERS; pn++)
    {
        if(playeringame[pn] && strcasecmp(player_names[pn],name)==0)
            return pn;
    }
    
no_player:   
    CONS_Printf("There is no player named\"%s\"\n",name);
    return 255;
}

// network kick message codes
typedef enum {
  KICK_MSG_GO_AWAY     = 1,
  KICK_MSG_CON_FAIL    = 2,
  KICK_MSG_PLAYER_QUIT = 3,
  KICK_MSG_TIMEOUT     = 4,
} kick_msg_e;

void Command_Kick(void)
{
    if (COM_Argc() != 2)
    {
        CONS_Printf ("kick <playername> or <playernum> : kick a player\n");
        return;
    }

    if(server)
    {
        int pn = player_name_to_num(COM_Argv(1));
        if(pn < MAXPLAYERS)
           SV_Send_NetXCmd_p2(XD_KICK, pn, KICK_MSG_GO_AWAY);
    }
    else
    {
        CONS_Printf("You are not the server\n");
    }
}

void Got_NetXCmd_KickCmd(xcmd_t * xc)
{
    byte pnum=READBYTE(xc->curpos);  // unsigned player num
    byte msg =READBYTE(xc->curpos);  // unsigned kick message

    if( pnum >= MAXPLAYERS )  return;
    GenPrintf(EMSG_hud, "\2%s ", player_names[pnum]);

    switch(msg)
    {
       case KICK_MSG_GO_AWAY:
               GenPrintf(EMSG_hud, "has been kicked (Go away)\n");
               break;
       case KICK_MSG_CON_FAIL:
               GenPrintf(EMSG_hud, "has been kicked (Consistency failure)\n");
               break;
       case KICK_MSG_TIMEOUT:
               GenPrintf(EMSG_hud, "left the game (Connection timeout)\n");
               break;
       case KICK_MSG_PLAYER_QUIT:
               GenPrintf(EMSG_hud, "left the game\n");
               break;
    }

    if( pnum == (byte)consoleplayer )
    {
        CL_Reset();
        D_StartTitle();
        M_SimpleMessage("You have been kicked by the server\n\nPress ESC\n");
    }
    else
    {
        CL_RemovePlayer(pnum);
    }
}

CV_PossibleValue_t maxplayers_cons_t[]={{1,"MIN"},{32,"MAX"},{0,NULL}};

consvar_t cv_allownewplayer = {"sv_allownewplayers","1",0,CV_OnOff};
consvar_t cv_maxplayers     =
  {"sv_maxplayers","32",CV_NETVAR,maxplayers_cons_t,NULL,32};

static void Got_NetXCmd_AddPlayer(xcmd_t * xc);
static void Got_NetXCmd_AddBot(xcmd_t * xc);	//added by AC for acbot

// Called one time at init, by D_Startup_NetGame.
void D_Init_ClientServer (void)
{
  DEBFILE(va("==== %s debugfile ====\n", VERSION_BANNER));

    network_state = NETS_idle;
    cl_drone = false;

    // [Arcade] localplayer[] has no static initializer -- zero would read as
    // player 0 in every slot -- so mark them unused before anything joins.
    CL_Init_localplayer();

    // drone server generating the left view of three screen view
    if(M_CheckParm("-left"))
    {
        cl_drone = true;
        viewangleoffset = ANG90;
    }
    // drone server generating the right view of three screen view
    if(M_CheckParm("-right"))
    {
        cl_drone = true;
        viewangleoffset = -ANG90;
    }
    // [WDJ] specify secondary screen angle in degrees (general case of left/right)
    if(M_CheckParm("-screendeg"))
    {
        cl_drone = true;
        if( M_IsNextParm() )
        {
            // does not accept negative numbers, use 270, 315, etc
            viewangleoffset = atoi(M_GetNextParm()) * (ANG90 / 90);
            // it is cheating to have screen looking behind player
            if( viewangleoffset < -ANG90 )  viewangleoffset = -ANG90;
            if( viewangleoffset > ANG90 )  viewangleoffset = ANG90;
        }
    }
//    debug_Printf( "viewangleoffset=%i\n", viewangleoffset );

    COM_AddCommand("playerinfo",Command_PlayerInfo, CC_info);
    COM_AddCommand("kick",Command_Kick, CC_net);
    COM_AddCommand("connect",Command_connect, CC_net);

    Register_NetXCmd(XD_KICK, Got_NetXCmd_KickCmd);
    Register_NetXCmd(XD_ADDPLAYER, Got_NetXCmd_AddPlayer);
    Register_NetXCmd(XD_ADDBOT, Got_NetXCmd_AddBot);	//added by AC for acbot

    CV_RegisterVar (&cv_allownewplayer);
    CV_RegisterVar (&cv_maxplayers);

    gametic = 0;
#ifdef CLIENTPREDICTION2
    localgametic = 0;
#else
    leveltime = 0;
#endif

    // do not send anything before the real begin
    SV_StopServer();  // as an Init
}

// By Server
// nnode: 0..(MAXNETNODES-1)
static void SV_Reset_NetNode(byte nnode)
{
    nnode_state[nnode] = NOS_idle;
    nnode_to_player[0][nnode] = 255;
    nnode_to_player[1][nnode] = 255;
    nettics[nnode]=gametic;
    nextsend_tic[nnode]=gametic;
    join_waiting[nnode]=0;
    playerpernode[nnode]=0;
}

// Called by D_Init_ClientServer, SV_SpawnServer, CL_Reset, D_WaitPlayer_Response
void SV_ResetServer( void )
{
    int    i;

    // +1 because this command will be executed in com_executebuffer in
    // tryruntic so gametic will be incremented, anyway maketic > gametic 
    // is not a issue

    maketic=gametic+1;
    cl_need_tic=maketic;
#ifdef CLIENTPREDICTION2
    localgametic = gametic;
#endif
    next_tic_clear=maketic;

    for (i=0 ; i<MAXNETNODES ; i++)
        SV_Reset_NetNode(i);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        playeringame[i]=false;
        player_state[i] = PS_unused;
        player_to_nnode[i] = 255;
    }

    cl_nnode=0;
    cl_packetmissed=false;
    viewangleoffset=0;

    if( dedicated )
    {
        // [WDJ] FIXME, why dedicated required node_in_game[0].
        nnode_state[0] = NETS_internal;
        serverplayer = 255;  // no server player
    }
    else
        serverplayer=consoleplayer;

    if(server)
    {
        cl_servernode = 0;  // client sees server on self
        cl_server_state = NOS_internal;
    }

    update_player_counts();
    quit_netgame_status = 0;

    DEBFILE(va("==== Server Reset ====\n"));
}

//
// D_Quit_NetGame
// Server or Client
// Called before quitting to leave a net game
// without hanging the other players
//
// Called by D_Quit_Save, D_WaitPlayer_Response, Command_ExitGame_f, repair_handler
void D_Quit_NetGame (void)
{
    if (!netgame)
        return;

    // [WDJ] Avoid the tight loop when the network fails.
    if( quit_netgame_status > 0 )
        return;

    quit_netgame_status = 1;
     
    DEBFILE("==== Quiting Game, closing connection ====\n" );

    // abort send/receive of files
    Close_NetFile();

    if( server )
    {
        // Server sends shutdown to all clients.
        SendPacket( BROADCAST_NODE, PT_SERVERSHUTDOWN );  // to NOS_recognized

        // Close registration with the Master Server.
        if ( serverrunning && cv_internetserver.value )
             MS_UnregisterServer(); 
    }
    else  // Client not server
    if( (cl_servernode < MAXNETNODES)
       && (cl_server_state >= NOS_server) )  // client connected to server
    {
        // Client sends quit to server.
        SendPacket( cl_servernode, PT_CLIENTQUIT );  // ignore failure
    }

    D_CloseConnection();
    cl_server_state = NOS_idle;
    network_state = NETS_shutdown;

    DEBFILE("==== Log finish ====\n" );
#ifdef DEBUGFILE
    if (debugfile)
    {
        fclose (debugfile);
        debugfile = NULL;
    }
#endif
}

// By Server.
// Add a node to the game (player will follow at map change or at savegame....)
//  set_nnode_state : the new nnode_state
static
void SV_AddNode(byte nnode, byte set_nnode_state)
{
    nettics[nnode]       = gametic;
    nextsend_tic[nnode]  = gametic;

    // [WDJ] nnode_state is client state maintained by server.
    // This includes the client on the server.  It will not start up correctly
    // if the nnode_state is not kept correctly.
    // This used to be blocked for nnode==0 because the server was setting
    // node_in_game to get packets working.  This bypass is no longer done.
    if( nnode_state[nnode] < NOS_client )
        nnode_state[nnode] = set_nnode_state;
}

// Get a free player node.  Obey the rules.
byte  SV_get_player_num( void )
{
    // The server player_state determines availability.
    byte pn = 0;
    while( player_state[pn] )  // find free player slot
    {
       pn++;
       if( pn >= MAXPLAYERS )  return  251;
    }
    
    player_state[pn] = PS_added; // pending
    return pn;
}

// Commit to using a player num slot.
//   nnode : node of the player
//   new_state : the new player state
// Return the new player num.
// Return 255 if too many players or too many player for the node.
byte SV_commit_player( byte nnode, byte new_state )
{
    byte newplayernum, pind;
    // Search for a free playernum.
    // New players will have playeringame set as a result of XCmd AddPlayer.
    pind = playerpernode[nnode];  // next pind
    if( pind >= MAXSPLITSCREENPLAYERS )   // [Arcade] was hardcoded 2
        return 255;
    
    newplayernum = SV_get_player_num();
            
    // Should never happen because we check the number of players
    // before accepting the join.
    if(newplayernum >= MAXPLAYERS)
    {
#ifdef PARANOIA
        I_SoftError("SV_commit_player: Reached MAXPLAYERS\n");
#endif
        return 255;
    }
    
    // Commit the server network settings.
    playerpernode[nnode]++;

    player_state[newplayernum] = new_state;
    player_pind[newplayernum] = pind;
    player_to_nnode[newplayernum] = nnode;
    nnode_to_player[pind][nnode] = newplayernum;
    return newplayernum;
}


// Client
// Add and remove players
static void update_player_state( byte pn, byte new_player_state )
{
    byte pst = player_state[pn];

    if( (new_player_state >= PS_player) && (new_player_state <= PS_bot) && (pst == PS_unused) )
    {
        G_AddPlayer(pn);
        if( new_player_state == PS_bot )
        {
            B_Create_Bot( & players[pn] );
            pst = PS_bot;
        }
        else if( new_player_state == PS_player )
            new_player_state = PS_player_from_server;  // easier to track

        playeringame[pn]=true;  // enable this player
    }
    else if( new_player_state == PS_unused )
    {
        if( pst > PS_unused )
            CL_RemovePlayer(pn);
        return;
    }
    player_state[pn] = new_player_state;
}

// Server and Client
// More accurate than purely inc and dec. There are too many odd ways to kill a node.
static void  update_player_counts( void )
{
    int pn, nn;

    num_player_used = 0;   
    num_game_players = 0;
    num_ticcmd_per_tic = 0;
    ticcmd_player_mask = 0;

    if( server )
    {
#ifdef WAIT_GAME_START_INTERMISSION
        num_wait_game_start = 0;
#endif
        num_join_waiting_players = 0;
        for(nn=0; nn<MAXNETNODES; nn++)
        {
            num_join_waiting_players += join_waiting[nn];
        }
    }

    for(pn=0; pn<MAXPLAYERS; pn++)
    {
        byte ps = player_state[pn];
        if( ps == PS_unused )  continue;

        num_player_used++;

#ifdef WAIT_GAME_START_INTERMISSION
        if( server && (ps == PS_join_wait_game_start) )
            num_wait_game_start++;
#endif

        if( playeringame[pn] )
        {
            num_game_players++;
            if( ps >= PS_player && ps < PS_bot )
            {
                ticcmd_player_mask |= 1<<pn;  // players that send ticcmds
                ticcmd_pid[num_ticcmd_per_tic++] = pn;  // list of pid
            }
        }
    }
}

// Client
//    pmask : player_mask
// The number of bits set in the player_mask.
static void CL_num_ticcmd_per( uint32_t pmask )
{
    byte pn = 0;
    num_ticcmd_per_tic = 0;
    ticcmd_player_mask = pmask;  // players that send ticcmds
    while( pmask )
    {
        if( pmask & 0x0001 )  // has a ticcmd in the packet
        {
            ticcmd_pid[num_ticcmd_per_tic++] = pn;  // list of pid
        }
        pmask >>= 1;
        pn++;
    }
}


#ifdef PARANOIA   
// FIXME: probably do not need this.
//   Only use the player mask for message content, not the way to update players.
// Client
static
void  CL_player_mask_error_detection( uint32_t playerdet )
{
    int pn;

    if( playerdet == ticcmd_player_mask )  return;  // quick check

    // When joining game, this is normal, as client has not been given all the players yet.
    GenPrintf(EMSG_warn, "Player mask error: server= %4X client= %4X\n", playerdet, ticcmd_player_mask );

    // Client only uses ticcmd_player_mask for this check.
    // Player_mask should match playeringame.
    // Does not affect players or bot set by client.
    for(pn=0; pn<MAXPLAYERS; pn++)
    {
        uint32_t pnb = 1<<pn;
        if( (playerdet & pnb) != (ticcmd_player_mask & pnb) )
        {
            GenPrintf(EMSG_warn, "  Player %d: server= %4X client= %4X\n", pn, playerdet & pnb, ticcmd_player_mask & pnb  );
        }
    }

//  Causes servertic error because this is the test reference for the player list.
//    ticcmd_player_mask = playerdet; // trust the server message, because the server made the message content
}
#endif   



// Broadcast the XD_ADDPLAYER
//  nnode : new player is at this node
//  pn : new player num
//  flags : any flags that may be needed (future)
static
void  SV_Send_AddPlayer( byte nnode, byte pn, byte flags )
{
    byte buf[6];
   
    player_state[pn] = PS_added_commit;

    // Format: XD_ADDPLAYER   (ver 1.48)
    //  byte:  nnode
    //  byte:  player num
    //  byte:  pind
    //  byte:  flags
    buf[0] = nnode;
    buf[1] = pn;
    buf[2] = player_pind[pn];
    buf[3] = flags;

    // Message from server to everyone, to add the player.
    SV_Send_NetXCmd(XD_ADDPLAYER, buf, 4);

    DEBFILE(va("Server added player %d net node %d\n", pn, nnode));

    if( nnode_state[nnode] < NOS_client )
        nnode_state[nnode] = NOS_active;
}


// Xcmd XD_ADDPLAYER
// Sent by server to all client.
static
void Got_NetXCmd_AddPlayer(xcmd_t * xc)
{
    static uint32_t sendconfigtic = 0xffffffff;
    byte nnode, newplayernum, pind;
    byte flags = 0;

    // Format: XD_ADDPLAYER
    // Older, from demo
    //  byte:  nnode
    //  byte:  (player num) | (splitscreen pind at 0x80)
    // ver 1.48
    //  byte:  nnode
    //  byte:  player num
    //  byte:  pind
    //  byte:  flags

    // [WDJ] Having error due to sign extension of byte read (signed char).
    nnode = READBYTE(xc->curpos);  // unsigned
    newplayernum = READBYTE(xc->curpos);  // unsigned
    // NetXCmd are issued by demos.
    if( EV_legacy >= 148 )
    {
        pind = READBYTE(xc->curpos);
        flags = READBYTE(xc->curpos);
        if( flags )
            GenPrintf(EMSG_warn, "AddPlayer %d unsupported flags=%x\n", (newplayernum+1), flags);
    }
    else
    {
        // Old format
        pind = (newplayernum & 0x80)? 1:0;
        newplayernum &= 0x1F;
    }

    // Make play engine player data.
    // Do not set playeringame until player is created.
    G_AddPlayer(newplayernum);
    playeringame[newplayernum]=true;  // enable this player
    player_state[newplayernum]= PS_player;
    update_player_count_request = 1;  // cannot call update_player_counts() direct, as it is in use

    // [WDJ] Players are 1..MAXPLAYERS to the user.
    GenPrintf(EMSG_hud, "Player %d is in the game (node %d)\n", (newplayernum+1), nnode);

    if(nnode==cl_nnode)
    {
        // The server is creating my player.
        if( ! server )
        {
            // normally server info
            player_to_nnode[newplayernum] = cl_nnode;  // for information only
            player_pind[newplayernum] = pind;
        }
        localplayer[pind] = newplayernum;

        if( pind == 0 )
        {
            // mainplayer
            consoleplayer=newplayernum;
            displayplayer=newplayernum;
            displayplayer_ptr = consoleplayer_ptr = &players[newplayernum];
        }
        else if( pind == 1 )
        {
            // splitscreen
            displayplayer2=newplayernum;
            displayplayer2_ptr = &players[displayplayer2];
        }
        // [Arcade] pind 2 and 3 join and play but get no viewport: the
        // renderer has only the two halves.  Deliberately not assigned to
        // displayplayer2, which would hand the second view to the last
        // player to join.

        if( sendconfigtic == gametic )
        {
            // [WDJ] When players have previously started on the same tic,
            // the new player config gets sent here.
            // Otherwise starting a splitscreen game, will not send the player2 config.
            Send_NameColor_pind(pind);
            Send_WeaponPref_pind(pind);
        }
       
        DEBFILE(va("Spawning player[%i] pind=%i at this node.\n", newplayernum, pind));
    }

    // the new player send their config
    // and the existing players send their config to the new node
    // WARNING : this can cause a bottleneck in the txtcmd
    //           this can also produce consistency failure if packet get lost
    //           because everybody knows the actual config except the joiner
    //    TODO : fixthis

    //  Don't send config more than once per tic (more than one player join)
    if( sendconfigtic!=gametic )
    {
        sendconfigtic=gametic;
        // Send all our player config to server, and other players.
        D_Send_PlayerConfig();
        if( server )
            B_Send_all_bots_NameColor();
    }
}

// Xcmd XD_ADDBOT
static
void Got_NetXCmd_AddBot(xcmd_t * xc)  //added by AC for acbot
{
    char * lcp = (char*)xc->curpos; // local cp
    char * botname; // bot name, so messages have correct name.
    int name_len;
    byte colour;

    // [WDJ] Having error due to sign extension of byte read (signed char).
    byte newplayernum = READBYTE(lcp);  // unsigned
    newplayernum&=0x7F;  // remove flag bit, and any sign extension

    if( EV_legacy < 148 )
    {
        // name and color from botinfo tables
        botname = botnames[botinfo[newplayernum].name_index];  // strings from a table
        colour = botinfo[newplayernum].colour;
        name_len = strlen( botname );
    }
    else
    {
        // Ver 1.48
        colour = READBYTE(lcp);
        botname = lcp;  // bot name, so messages have correct name.

        // [WDJ] String overflow safe
        name_len = strlen( lcp );
        lcp += name_len + 1;  // string and term
    }
   
    xc->curpos = (byte*)lcp;  // NetXCmd reading done
    
    if( playeringame[newplayernum] )
    {
        GenPrintf(EMSG_warn, "Bot %i: player slot already in use.\n", (newplayernum+1) );
        return;
    }

    G_AddPlayer(newplayernum);

    player_t * pl = & players[newplayernum];
    B_Create_Bot( pl );

    // [WDJ] AddBot sends the name and color because the server has that information,
    // and it is desirable to set those attributes correctly for the first draw.
    // Update bot name and color.
    P_SetPlayer_color( pl, colour );
    if( name_len > MAXPLAYERNAME-1 ) // dest protection
        name_len = MAXPLAYERNAME-1;  // sizeof player_names
    memcpy( & player_names[newplayernum], botname, (name_len + 1));  // [MAXPLAYERNAME]
    player_names[newplayernum][MAXPLAYERNAME-1] = '\0';  // safe

    playeringame[newplayernum]=true;  // enable this player
    player_state[newplayernum]= PS_bot;
    update_player_count_request = 1;  // cannot call update_player_counts() direct, as it is in use

    multiplayer=1;

    GenPrintf(EMSG_hud, "Bot %s has entered the game\n", botname);

    if( server )
    {
        // Bot exists, so now can send the bot info.
        B_Send_bot_NameColor( newplayernum );
    }
}

// By Server.
// Called by SV_SpawnServer, client_join_handler.
// Return true when a new player is added.
static
boolean SV_Add_Join_Waiting(void)
{
    boolean  newplayer_added = false;  // return
    byte nnode, ns;
    byte newplayernum;

    if( num_join_waiting_players == 0 )  return 0;

    // The code below usually clears the join_waiting queues.
    num_join_waiting_players = 0;
   
    for(nnode=0; nnode<MAXNETNODES; nnode++)
    {
        ns = nnode_state[nnode];
        if( (ns >= NOS_join_file) && (ns <= NOS_join_savegame) )
        {
            // Join download in progress
            num_join_waiting_players += join_waiting[nnode];
            continue;
        }
 
        // splitscreen can allow 2 player in one node
        for(; join_waiting[nnode]>0; join_waiting[nnode]--)
        {
            // Search for a free playernum.
            // New players will have playeringame set as a result of XCmd AddPlayer.
            newplayernum = SV_commit_player( nnode, PS_added_commit );
            if( newplayernum >= MAXPLAYERS )  continue;

            // Message from server to everyone, to add the player.
            SV_Send_AddPlayer( nnode, newplayernum, 0 );  // PS_added_commit
            // XD_ADDPLAYER commands will stay in the cmd buffers.
            // When cl_mode==CLM_connected is achieved, they will be transmitted and acted upon.

            newplayer_added = true;
        }
    }

    update_player_counts();

    return newplayer_added;
}


#ifdef WAIT_GAME_START_INTERMISSION

#define GAME_START_WAIT    22

// Add players waiting for game start
//  mode : 0= from intermission
void SV_Add_game_start_waiting_players( byte mode )
{
    byte pn, nnode, cnt = 0;
    // wait_timer must be at least 1 to escape from GS_INTERMISSION.
    int wait_timer = (mode == 0)? TICRATE*GAME_START_WAIT : 1 ;

    // This seems to only work using GS_INTERMISSION
    for( pn=0; pn<MAXPLAYERS; pn++)
    {
        if( player_state[pn] == PS_join_wait_game_start )
        {
            nnode = player_to_nnode[pn];
            if( nnode_state[nnode] == NOS_wait_game_start )
            {
                nnode_state[nnode] = NOS_active;
                nextsend_tic[nnode] = gametic;

                SV_Send_LevelCfg( nnode );
                SV_Send_PlayerState( nnode );
            }

            // Take node out of wait_game_start.
            SV_Send_control( nnode, CTRL_game_start, mode, wait_timer, pn );

            // Update everyone else.
            SV_Send_AddPlayer( nnode, pn, 0 );  // NetXCmd, PS_added_commit

//            SV_Send_PlayerState( nnode );
            cnt++;
        }
    }

    if( cnt == 0 )
        return;

    // Update gametic and random state.
    SV_Send_State( paused | network_wait_pause );
    SendPacket( BROADCAST_NODE, PT_REQ_CLIENTCFG );  // req all nodes to send client cfg
    B_Send_all_bots_NameColor();  // Bot NameColor to everybody, by NetXCmd

    // Invoke the next level wait timer.
    wait_game_start_timer = wait_timer;
}
#endif


void CL_Splitscreen_Player_Manager( void )
{
    if( cl_mode != CLM_connected )
        return;

    if( cv_splitscreen.EV )
    {
        if( ! displayplayer2_ptr )
            CL_Send_Join();  // join game
        return;
    }

    // Remove splitscreen player
    if( displayplayer2_ptr )
        SV_Send_NetXCmd_p2(XD_KICK, displayplayer2, KICK_MSG_PLAYER_QUIT);  // player 2
}


// Is there a game running.
boolean Game_Playing( void )
{
    return (server && serverrunning) || (!server && cl_mode==CLM_connected);
}

// By Server and Server-only commands.
// Called by D_Startup_NetGame (dedicated server).
// Called by Command_Map_f, Command_Load_f (server).
// Return true when a new player is added.
boolean SV_SpawnServer( void )
{
    D_DisableDemo();

    quit_netgame_status = 0;

    if( serverrunning == false )
    {
        GenPrintf(EMSG_hud, "Starting Server ...\n");
        serverrunning = true;
        SV_ResetServer();
        network_state = NETS_internal;  // Self server network.
        if( netgame )
        {
            I_NetOpenSocket();
            network_state = NETS_open;
            // Register with the Master Server.
            if( cv_internetserver.value )
            {
                // MasterServer address is in cv_masterserver.
                MS_RegisterServer();
            }
        }

        D_WaitPlayer_Setup();

        // server just connect to itself
        if( !dedicated )
            CL_ConnectToServer();
    }

    if( num_join_waiting_players )
        return SV_Add_Join_Waiting();

    return 0;
}

// Called by D_Init_ClientServer, G_StopDemo, CL_Reset, SV_StartSinglePlayerServer,
// D_WaitPlayer_Response.
void SV_StopServer( void )
{
    int i;

    gamestate = wipegamestate = GS_NULL;

    {
        byte pind;   // [Arcade] one channel per local player
        for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )
            localtextcmd[pind].len = 0;  // text len
    }

    for(i=0; i<BACKUPTICS; i++)
        D_Clear_ticcmd(i);

    consoleplayer=0;
    cl_mode = CLM_searching;
    maketic=gametic+1;
    cl_need_tic=maketic;
    serverrunning = false;
}

// called at singleplayer start and stopdemo
void SV_StartSinglePlayerServer(void)
{
    server        = true;
    netgame       = false;
    multiplayer   = false;
    cl_server_state = NOS_internal; // no quit game message

    // no more tic the game with this settings !
    SV_StopServer();

    // [Arcade] More than one player on this machine is a multiplayer game,
    // whether or not the screen is split for them.
    if( D_NumLocalPlayers() > 1 )
        multiplayer    = true;
}

// By Server.
// Called by client_join_handler.
static void SV_Send_Refuse(int to_node, char *reason)
{
    dl_strncpy(netbuffer->u.stringpak.str, reason, MAX_STRINGPAK_LEN);

    netbuffer->packettype = PT_SERVERREFUSE;
    HSendPacket( to_node, SP_reliable|SP_queue, 0, strlen(netbuffer->u.stringpak.str)+1 );  // ignore failure
    Net_CloseConnection(to_node, 0);
}

// By Server.
// PT_ASKINFO from Client.
// Handle a client request for server info.
static void server_askinfo_handler( byte nnode )
{
    if(serverrunning)
    {
        // Make the send_time the round trip ping time.
        SV_Send_ServerInfo(nnode, LE_SWAP32(netbuffer->u.askinfo.send_time));
        Net_CloseConnection(nnode, 0);  // a temp connection
    }
}

// By Server.
// PT_CLIENTJOIN from future client.
//   nnode: net node that is joining
static void client_join_handler( byte nnode )
{
    byte num_to_join, command;
    byte join_flags = netbuffer->u.clientcfg.flags;
    boolean newnode=false;

    if( netbuffer->u.clientcfg.version != VERSION
        || LE_SWAP32(netbuffer->u.clientcfg.subversion) != NETWORK_VERSION)
    {
        SV_Send_Refuse(nnode,
           // Text is automatically centered, must not be too long.
           va("Incompatible client\nServer %i Net %i\nLegacy %i Net %i",
               VERSION, NETWORK_VERSION, netbuffer->u.clientcfg.version, LE_SWAP32(netbuffer->u.clientcfg.subversion) )
        );
        return;
    }

    // nnode==0 is self, which is always accepted.
    if(!cv_allownewplayer.value && nnode!=0 )
    {
        SV_Send_Refuse(nnode,
          "The server is not accepting players\nat this time");
        return;
    }

    num_to_join = netbuffer->u.clientcfg.num_node_players - playerpernode[nnode];
    // DRONE will have 0 players
   
    // Compute it using join and player counts.
    update_player_counts();
    if( (num_player_used + num_to_join) > cv_maxplayers.value )
    {
        SV_Send_Refuse(nnode,
           va("Maximum players reached (max:%d)", cv_maxplayers.value));
        return;
    }

    // To determine the intial command to send with the config.
    command = CTRL_normal;
    // If there is an existing player at the node, then will not be newnode.
    newnode = ( nnode_state[nnode] < NOS_join );
    if( newnode && (gamestate == GS_LEVEL) )
    {
#ifdef JOININGAME
        // If cannot download, then wait for next game start.
        if(( join_flags & NF_download_savegame )  // joiner allows savegame download
          && ( cv_SV_download_savegame.EV )    )  // server allows savegame download
            command = CTRL_download_savegame;
        else
            command = CTRL_wait_game_start;

        // For savegame downloads to work, the endian must match.
#ifdef __BIG_ENDIAN__
        // Server is big-endian
        if( (join_flags & NF_big_endian) == 0 )  // joiner is little-endian
#else
        // Server is little-endian
        if( join_flags & NF_big_endian )  // joiner is big-endian
#endif
            command = CTRL_wait_game_start;

#else
        // No join-in-game, no savegame download
        command = CTRL_wait_game_start;
#endif
    }

    // Client authorized to join.
    if( newnode )
    {
        // The nnode is new to this server.
        SV_AddNode(nnode, NOS_join);

        // Send inital command with the config to avoid sending a separate
        // command message, which could arrive out-of-order.
        if(! SV_Send_ServerConfig(nnode, command) )
            goto kill_node;

        DEBFILE("New node joined\n");
    }
   
    if( join_flags & NF_drone )
    {
        // drone only watches another player
        playerpernode[nnode] = 0;  // indicator that is DRONE node
        return;
    }

    if( num_to_join )
    {
        if( command == CTRL_wait_game_start )
            goto wait_for_game_start;

#ifdef JOININGAME
        if( command == CTRL_download_savegame )
        {
            // New node with new players joining existing game.
            // Update the nnode with game in progress.
            nnode_state[nnode] = NOS_join_savegame;
            SV_Send_SaveGame(nnode); // send game data
            // netwait timer is running
            GenPrintf(EMSG_info, "Send savegame\n");
            // Client will return  PT_CLIENTREADY or PT_CLIENTQUIT
        }
#endif

        // Add node players to join_waiting
        join_waiting[nnode] = num_to_join;
        num_join_waiting_players += num_to_join;
        SV_Add_Join_Waiting();
    }
    return;
 
wait_for_game_start:
    // These players will wait until the next game start.
#ifdef WAIT_GAME_START_INTERMISSION
    while( num_to_join-- )
    {
        byte pn = SV_commit_player( nnode, PS_join_wait_game_start );
        SV_Send_control( nnode, CTRL_wait_game_start, 0, 0, pn );
        DEBFILE(va("Client Join: node=%i, wait game start player=%i.\n", nnode, pn));
    }

    if( nnode_state[nnode] < NOS_client )
        nnode_state[nnode] = NOS_wait_game_start;  // release network_wait

    update_player_counts();
    return;
#else
    SV_Send_Refuse(nnode, "This server cannot handle\nwait_for_game_start players");
    goto kill_node;
#endif
    
kill_node:
    DEBFILE("Client Join: Failure to Send.\n");
    GenPrintf(EMSG_error, "Client Join: Failure to Send\n" );
    SV_Reset_NetNode(nnode);
    return;
}


// BY Server.
// PT_NODE_TIMEOUT, PT_CLIENTQUIT
//   nnode : the network client node quitting
//   client_pn : the client player num that sent the quit
static void client_quit_handler( byte nnode, byte client_pn )
{
    // Set nnode_state to NOS_shutdown at the end of the kick command.
    // This allows the sending of some packets to the quiting client
    // and to have them ack back.
    join_waiting[nnode]= 0;
    if( (client_pn < MAXPLAYERS) && playeringame[client_pn])
    {
        byte reason = (netbuffer->packettype == PT_NODE_TIMEOUT) ?
           KICK_MSG_TIMEOUT : KICK_MSG_PLAYER_QUIT;
        // Update other players by kicking nnode.
        SV_Send_NetXCmd_p2(XD_KICK, client_pn, reason);  // kick player
        nnode_to_player[0][nnode] = 255;

        byte pn2 = nnode_to_player[1][nnode];  // splitscreen player at the nnode
        if( pn2 < MAXPLAYERS )
        {
            if( playeringame[pn2] )
            {
               // kick player2
               SV_Send_NetXCmd_p2(XD_KICK, pn2, reason);
            }
            nnode_to_player[1][nnode] = 255;
        }
    }
    Net_CloseConnection(nnode, 0);
    nnode_state[nnode] = NOS_shutdown;
}



// By Client
// PT_SERVERINFO from Server.
//  nnode : remote node
static void server_info_handler( byte nnode )
{
    // Compute ping in ms.
    netbuffer->u.serverinfo.trip_time =
     (I_GetTime() - LE_SWAP32(netbuffer->u.serverinfo.trip_time))*1000/TICRATE; 
    netbuffer->u.serverinfo.servername[MAXSERVERNAME-1]=0;

    SL_InsertServer( & netbuffer->u.serverinfo, nnode);
}


// By Client
// PT_SERVERREFUSE from Server.
static void server_refuse_handler( byte nnode )
{
    if( cl_mode == CLM_wait_join_response )
    {
        M_SimpleMessage(va("Server %i refuses connection\n\nReason :\n%s",
                           nnode,
                           netbuffer->u.stringpak.str));
        CL_Reset();
        D_StartTitle();
    }
}


// By Client
// PT_SERVERCFG from Server.
// Received acceptance of the player node joining the game.
static void server_cfg_handler( byte nnode )
{
    xcmd_t xc;

    if( cl_mode != CLM_wait_join_response )
        return;

    if(!server)
    {
        // Clients not on the server, update to server time.
        maketic = gametic = cl_need_tic = read_N32( &  netbuffer->u.servercfg.gametic );
    }

    // Client keeps server state, even on the server.
    if( cl_servernode < MAXNETNODES )
        cl_server_state = NOS_server;  // connected

#ifdef CLIENTPREDICTION2
    localgametic = gametic;
#endif

    // Handle a player on the server.
    serverplayer = netbuffer->u.servercfg.serverplayer;
    // Client use of player_to_nnode, see Command_PlayerInfo, AddPlayer
    if (serverplayer < MAXPLAYERS)  // 255 = no player
        player_to_nnode[serverplayer] = cl_servernode;

    cl_nnode = netbuffer->u.servercfg.clientnode;  // assigned by server

    GenPrintf(EMSG_hud, "Join accepted, wait next map change ...\n");
    DEBFILE(va("Server accept join gametic=%d, client net node=%d\n",
               gametic, cl_nnode));

    // No need for the server to update itself from message from server.
    if( ! server )
    {
        // Client
#if 1
#ifdef PARANOIA   
        uint32_t  playerdet = read_N32( & netbuffer->u.servercfg.playerdetected );
        CL_player_mask_error_detection( playerdet );
#endif
#else
        // This was for servertic cmds, no longer need to do this.
        uint32_t  playerdet = read_N32( & netbuffer->u.servercfg.playerdetected );
        for(j=0;j<MAXPLAYERS;j++)
        {
            playeringame[j] = (( playerdet & (1<<j) ) != 0);
            player_state[j] = (playeringame[j])? PS_player_from_server : 0;
        }
#endif

        xc.playernum = 0;
        xc.curpos = netbuffer->u.servercfg.netvar_buf;
        xc.endpos = xc.curpos + NETVAR_BUFF_LEN - 1;
        CV_LoadNetVars( &xc );
    }

    // Initial command, to avoid a separate command message.
    switch( netbuffer->u.servercfg.command )
    {
     case CTRL_normal:
        cl_mode = CLM_connected;
        break;
#ifdef JOININGAME
     case CTRL_download_savegame:
        if(netbuffer->u.servercfg.gamestate == GS_LEVEL)
            GenPrintf(EMSG_hud, "Server Config: Download savegame when NOT GS_LEVEL\n");
        cl_mode = CLM_download_savegame;
        break;
#endif
#ifdef WAIT_GAME_START_INTERMISSION
     case CTRL_wait_game_start:
        cl_mode = CLM_wait_game_start;
        break;
#endif
    }
}

// By Client
// PT_SERVERSHUTDOWN from Server.
static void server_shutdown_handler()
{
    network_state = NETS_no_server;
    if( cl_mode != CLM_searching )
    {
        M_SimpleMessage("Server has Shutdown\n\nPress Esc");
        CL_Reset();
        D_StartTitle();
    }
}

// By Client
// PT_NODE_TIMEOUT
static void server_timeout_handler()
{
    network_state = NETS_no_server;
    if( cl_mode != CLM_searching )
    {
        M_SimpleMessage("Server Timeout\n\nPress Esc");
        CL_Reset();
        D_StartTitle();
    }
}


// Copy a single ticcmd_t, swapping between host and network byte order.
static void TicCmdCopy( ticcmd_t * dst, ticcmd_t * src )
{
#ifdef CLIENTPREDICTION2
    dst->x = LE_SWAP32_FAST(src->x);
    dst->y = LE_SWAP32_FAST(src->y);
#endif
    dst->forwardmove = src->forwardmove;
    dst->sidemove    = src->sidemove;
    dst->angleturn   = LE_SWAP16_FAST(src->angleturn);
    dst->aiming      = LE_SWAP16_FAST(src->aiming);
    dst->buttons     = src->buttons;
#ifdef TICCMD_148
    dst->ticflags    = src->ticflags;
#endif
}

// Detected a consistency fault.
//  nnode : the client node
//  fault_tic : tick with consistency fault
//  btic : BTIC_INDEX for this network message
static void SV_consistency_fault( byte nnode, tic_t fault_tic, int btic )
{
    // It is the node that is not consistent.
    byte confault = ++consistency_faults[nnode];  // failure count
    uint16_t sv_con = consistency[btic];
    uint16_t cl_con = LE_SWAP16(netbuffer->u.clientpak.consistency);

    // No consistency fault during intermission, because of joining players who don't have position yet.
    if( gamestate == GS_INTERMISSION )
        return;

    if( verbose )
    {
        GenPrintf(EMSG_warn, "Consistency failure tic %d: node %d   consistency( server=%X client=%X )\n",
            fault_tic, nnode, sv_con, cl_con );
    }

    if( confault >= consistency_limit_fatal[cv_SV_netrepair.EV] )
    {
        // Failed the consistency check too many times
        if(! verbose )
            GenPrintf(EMSG_warn, "Consistency failure ( server=%X client=%X ), msg tic %d, Kick node %i\n",
                 sv_con, cl_con, fault_tic, nnode );
        DEBFILE(va("Consistency failure ( server=%i client=%i ), msg tic %d, Kick node %i\n",
                 sv_con, cl_con, fault_tic, nnode ));

#ifdef DEBUG_DISABLE_KICK_PLAYERS
        // Debug message instead.
//        GenPrintf(EMSG_warn, "Kick node %d at tic %d, consistency failure\n", nnode, fault_tic);
        SV_Send_player_repair( 255, 3, nnode );
#else
        // Kick all players at the nnode.
        byte pn = nnode_to_player[0][nnode];
        SV_Send_NetXCmd_p2(XD_KICK, pn, KICK_MSG_CON_FAIL);
        GenPrintf(EMSG_warn, "Kick player %d\n", pn );
        DEBFILE(va("Kick player %d\n", pn ));

        pn = nnode_to_player[1][nnode];
        if( pn < MAXPLAYERS )
        {
            // Kick splitscreen player
            SV_Send_NetXCmd_p2(XD_KICK, pn, KICK_MSG_CON_FAIL);
            GenPrintf(EMSG_warn, "Kick player %d\n", pn );
            DEBFILE(va("Kick player %d\n", pn ));
        }
#endif
    }
#ifdef JOININGAME
    else if( ( (consistency_sg_bit[cv_SV_netrepair.EV] >> (confault-1)) & 0x01)
             && ( cv_SV_download_savegame.EV ))
    {
        // try to use savegame to fix consistency
        SV_Send_repair(RQ_SUG_SAVEGAME, nnode);
    }
#endif
    else if( confault > 1 )
    {
        // try to fix consistency, all players
        SV_Send_player_repair(255, 3, nnode);
    }
    else
    {
        // try to fix consistency cheap
        SV_Send_player_repair(nnode_to_player[0][nnode], 2, nnode);
    }
}


// By Client.
// Send the client packet to the server
// Called by NetUpdate, 
static void CL_Send_ClientCmd (void)
{
    // index by  [mis]
    static byte  PT_CLIENTCMD_options[2] = {PT_CLIENTCMD, PT_CLIENTMIS};
    // index by  [mis]
    static byte  PT_NODEKEEPALIVE_options[2] = {PT_NODEKEEPALIVE, PT_NODEKEEPALIVEMIS};

/* oops can do that until i have implemented a real dead reckoning
    static ticcmd_t lastcmdssent;
    static int      lastsenttime=-TICRATE;

    if( memcmp(&localcmds[0],&lastcmdssent,sizeof(ticcmd_t))!=0 || lastsenttime+TICRATE/3<I_GetTime())
    {
        lastsenttime=I_GetTime();
*/

    int packetsize=0;

    byte  cmd_options = 0;  // easier to understand and maintain
    if (cl_packetmissed)
        cmd_options = 1;  // MIS bit
   
    netbuffer->packettype = PT_CLIENTCMD_options[cmd_options];
    netbuffer->u.clientpak.resendfrom = cl_need_tic;
    netbuffer->u.clientpak.client_tic = gametic;  // byte

    if( gamestate == GS_WAITINGPLAYERS )
    {
        // Server is waiting for network players before starting the game.
        // send NODEKEEPALIVE, or NODEKEEPALIVEMIS packet
        netbuffer->packettype = PT_NODEKEEPALIVE_options[cmd_options];
//        packetsize = sizeof(clientcmd_pak_t)-sizeof(ticcmd_t)-sizeof(int16_t);
        packetsize = offsetof(clientcmd_pak_t, consistency);
        HSendPacket( cl_servernode, 0, 0, packetsize );  // msg lost when too busy
    }
    else
    if( gamestate != GS_NULL )
    {
        int btic = BTIC_INDEX( gametic );
        netbuffer->u.clientpak.consistency = LE_SWAP16_FAST(consistency[btic]);

        // [Arcade] One ticcmd per local player on this machine.  The packet
        // already carried a pind_mask and was sized to "use only what is
        // needed", so widening past two players fits the existing format --
        // only the hardcoded 0x03 / cmd[1] had to go.
        byte pind, nlocal = D_NumLocalPlayers();
        byte mask = 0;
        for( pind=0; pind<nlocal; pind++ )
        {
            TicCmdCopy( &netbuffer->u.clientpak.cmd[pind], /*src*/ &localcmds[pind] );
            mask |= (1<<pind);
        }
        netbuffer->u.clientpak.pind_mask = mask;
        packetsize = offsetof(clientcmd_pak_t, cmd) + (nlocal * sizeof(ticcmd_t));
        
        HSendPacket( cl_servernode, 0, 0, packetsize );  // msg lost when too busy
    }
}


// By Server
// PT_CLIENTCMD, PT_CLIENTMIS,
// PT_NODEKEEPALIVE, PT_NODEKEEPALIVEMIS from Client.
static void client_cmd_handler( byte netcmd, byte nnode )
{
    tic_t  start_tic, end_tic;
    int  btic;
    byte pind;

    // To save bytes, only the low byte of tic numbers are sent
    // Figure out what the rest of the bytes are
    start_tic  = ExpandTics (netbuffer->u.clientpak.client_tic);
    end_tic = ExpandTics (netbuffer->u.clientpak.resendfrom);

    if(  netcmd == PT_CLIENTMIS
         || netcmd == PT_NODEKEEPALIVEMIS
         || nextsend_tic[nnode] < end_tic )
    {
        nextsend_tic[nnode] = end_tic;
    }

    // Discard out of order packet
    if( end_tic < nettics[nnode] )
    {
        DEBFILE(va("Late client ticcmd discarded: nettics %d\n",
                   nettics[nnode]));
        return;
    }

    // Update the nettics.
    nettics[nnode] = end_tic;

    // Don't do any tic cmds for drones, just update their nettics.
    if( playerpernode[nnode] == 0 )   // DRONE node indicator
       return;

    if( netcmd==PT_NODEKEEPALIVE || netcmd==PT_NODEKEEPALIVEMIS )
       return;

    // Check consistency
    if((start_tic <= gametic)
       && (start_tic > (gametic - BACKUPTICS + 1)) )
    {
        // within previous tics
        btic = BTIC_INDEX(start_tic);
        if(consistency[btic] != LE_SWAP16_FAST(netbuffer->u.clientpak.consistency))
        {
            // Failed the consistency check.
            SV_consistency_fault( nnode, start_tic, btic );  // node has failed consistency
            return;  // packet contents lost when other messages sent
        }
        else if( consistency_faults[nnode] > 0 )
        {
            consistency_faults[nnode] -- ;
        }
    }

    // Copy the ticcmd
    btic = BTIC_INDEX( maketic );
    // This packet may carry up to MAXSPLITSCREENPLAYERS players
    byte pind_mask = netbuffer->u.clientpak.pind_mask;
    for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )
    {
        if( (pind_mask & (1<<pind)) == 0 )  continue;
        byte pn = nnode_to_player[pind][nnode];
        if( pn < MAXPLAYERS )
            TicCmdCopy( &netcmds[btic][pn], /*src*/ &netbuffer->u.clientpak.cmd[pind] );
    }
    return;
}


// [WDJ] The ver 1.48 server tic packet has a new format.  It can send extension
// packets when needed.
// All of the textcmd will be included.

// By Server.
// Send PT_SERVERTICS, the server packet.
// Send tic from next_tic_send to maketic-1.
static void SV_Send_Tics (void)
{
    static byte resend_cnt = 0;  // spread resends at less cost than Get_Time

    ticcmd_t * netcmd_p;
    byte * bufpos;  // net txtcmd text
    byte * endbuf, * end_ticcmd;

    tic_t start_tic, end_tic, ti;
    int  btic, j;
    int  packsize;
    int  total_textcmd_size;
    byte seq_num, num_cmds, num_txt, flags;
    byte nnode, pn;

    // update_player_counts sets ticcmd_player_mask, num_ticcmd_per_tic, ticcmd_pid[].

    // Send PT_SERVERTIC to all client, but not to myself.
    // For each node create a packet with num_tics and send it.
    // Compute num_tics using nextsend_tic[n], max packet size, and maketic.
    // Assume cl_servernode=0.
    for(nnode=1; nnode<MAXNETNODES; nnode++)
    {
        if( nnode_state[nnode] < NOS_client )  continue;
        // Need this to send the NetXCmd Add Player

        // For each node create a packet with x tics and send it.
        // x is computed using nextsend_tic[n], max packet size and maketic.

        // Send a packet to each client node in this game.
        // They may be different, with individual status.
        end_tic = maketic;
          // The last tic to send is one before maketic.
          // Does not send end_tic.

        // assert nextsend_tic[nnode]>=nettics[nnode]
        start_tic = nextsend_tic[nnode];  // last tic sent to this nnode

        if(start_tic >= maketic)
        {
            // We have sent all tics, so we will use the extrabandwidth
            // to resend packets that are supposed lost.
            // This is necessary since lost packet detection works when we
            // have received packet with (firsttic > cl_need_tic)
            // (in the getpacket servertics case).
            DEBFILE(va("Send Tics none: node %d maketic %u nxttic %u nettic %u\n",
                       nnode, maketic, nextsend_tic[nnode], nettics[nnode]));
            // Client sends a tic, then server re-sends the ticcmds to everyone.	   
            start_tic = nettics[nnode];  // last client tic received by server
            if( start_tic >= maketic )
                continue;  // all tic are ok, do next node
            if( (nnode + resend_cnt++)&3 )  // some kind of randomness
                continue;  // skip it
            DEBFILE(va("Send Tics Resend: node %d from tic %d\n", nnode, start_tic));
        }

        // Limit start to first new tic we have.
        if( start_tic < next_tic_send )
            start_tic = next_tic_send;

        // Compute the length of the packet and cut it if too large.
        packsize = SERVER_TIC_BASE_SIZE;
        total_textcmd_size = 0;
        for(ti=start_tic; ti<end_tic; ti++)
        {
            // All of the ticcmd
            packsize += sizeof(ticcmd_t) * num_ticcmd_per_tic;
            // All of the textcmd buffer and support fields
            // Optional textcmd support fields, are only needed when there are textcmd.
            int tcblen = textcmdbuff[ BTIC_INDEX(ti) ].len;
            if( tcblen )
            {
                total_textcmd_size += sizeof_servertic_textcmd_t( tcblen );
            }

#if 1
            // Limit each to 1 packet.
            if( (packsize > software_MAXPACKETLENGTH) || (total_textcmd_size > software_MAXPACKETLENGTH) )
#else
            // Limit together at 1 packet.
            if( (packsize + total_textcmd_size) > software_MAXPACKETLENGTH )
#endif
            {
                // Exceeds max packet size.
                // Will send extension packets to carry excess ticcmd, and all textcmd.
                if(ti == start_tic)  ti++;  // do at least one tic, will send extension packet

                DEBFILE(va("Packet size limited (%d) (tics %d to %d)  send tic %d \n",
                            packsize, start_tic, end_tic - 1, ti - 1));

                end_tic = ti;  // limit this packet due to size
                break;
            }
        }
            
        // Send the tics, start_tic..(end_tic-1).

        // Header
        netbuffer->packettype = PT_SERVERTICS;
        netbuffer->u.serverpak.starttic = start_tic;
        netbuffer->u.serverpak.numtics = (end_tic - start_tic); // num tics
        write_N32( &netbuffer->u.serverpak.cmd_player_mask, ticcmd_player_mask );  // players that send ticcmd

        // init
        netbuffer->u.serverpak.cmds_offset = 0;
        netbuffer->u.serverpak.num_textcmd = 0;
        flags = seq_num = num_cmds = num_txt = 0;

        bufpos= (byte *)&netbuffer->u.serverpak.cmds;
        // limits
        endbuf= ((byte*)netbuffer) + software_MAXPACKETLENGTH - 1;
        end_ticcmd = endbuf - sizeof(ticcmd_t);
        if( end_ticcmd > (byte*)& netbuffer->u.serverpak.cmds[NUM_SERVERTIC_CMD-1] )
            end_ticcmd = (byte*)& netbuffer->u.serverpak.cmds[NUM_SERVERTIC_CMD-1];

        // All the ticcmd_t, start_tic..(end_tic-1)
        for(ti=start_tic; ti<end_tic; ti++)
        {
            btic = BTIC_INDEX( ti );
            netcmd_p = netcmds[btic];

            // Send only the ticcmd of players in game.
            for( j=0; j<num_ticcmd_per_tic; j++ )
            {
                if( bufpos > end_ticcmd )
                {
                    // send packet section
                    netbuffer->u.serverpak.flags = TPF_more | flags | seq_num;
                    netbuffer->u.serverpak.num_cmds_present = num_cmds;
                    packsize = bufpos - (byte*)&(netbuffer->u);
                    HSendPacket(nnode, 0, 0, packsize);  // msg lost when too busy

                    // init next seq packet
                    netbuffer->u.serverpak.cmds_offset += num_cmds;
                    num_cmds = 0;
                    seq_num ++;
                    bufpos = (byte*)&netbuffer->u.serverpak.cmds;
                }

                // Use list of pid generated from playeringame and player_state.
                pn = ticcmd_pid[j];
                TicCmdCopy( (ticcmd_t*) bufpos, /*src*/ &netcmd_p[pn] );
                bufpos += sizeof(ticcmd_t);
                num_cmds++;  // ticcmd count
            }
        }
        netbuffer->u.serverpak.num_cmds_present = num_cmds;
        // Done writing ticcmds

        if( total_textcmd_size )
        {
            // All the textcmd, start_tic..(end_tic-1)
            // [WDJ] Most often there are no textcmd.  Make that case simple and small.

            // [WDJ] Ver 1.48, easy to send in multiple packets

            // The bufpos is always at end-of-packet; cannot update until the textcmd section actually gets text.
            for(ti=start_tic; ti<end_tic; ti++)
            {
                // textcmd from players, bots, server
                btic = BTIC_INDEX( ti );
                textcmdbuff_t * tcbuf = & textcmdbuff[btic];
                unsigned int buflen = tcbuf->len;
                if( buflen == 0 )  continue;

                if(((bufpos + sizeof_servertic_textcmd_t(buflen)) > endbuf)  // not enough room in packet
                    || ( num_txt >= 250 ) )   // force send
                {
                    // Send packet, start new extension packet.
                    // Rarely executed.  Required when many players are texting at once.
                    // This may be ticcmd only, or both, or textcmd only.
                    netbuffer->u.serverpak.flags = TPF_more | flags | seq_num;
                    netbuffer->u.serverpak.num_textcmd = num_txt; // the number of textcmd
                    packsize = bufpos - (byte *)&(netbuffer->u);
                    HSendPacket(nnode, 0, 0, packsize);  // msg lost when too busy

                    // init next seq packet
                    netbuffer->u.serverpak.num_cmds_present = 0;
                    seq_num ++;

                    // Reset to start of data area, keep same packet header.
                    bufpos = (byte *)&netbuffer->u.serverpak.cmds;
                    num_txt = 0;
                }

                // Tics with textcmd, use a servertic_textcmd_t, unaligned.
                // All players and server textcmd are already packed into the buffer.
                servertic_textcmd_t * stcp = (servertic_textcmd_t *)bufpos;
                stcp->tic = ti;
                write_N16( & stcp->len, buflen );
                // Send the textcmdbuf, length limited.
                memcpy(&stcp->textitem, &tcbuf->buff, buflen);
                // Cannot add field sizes as the structure might be padded.
                bufpos += sizeof_servertic_textcmd_t( buflen );  // tic, len(2 bytes), buf
                num_txt++;  // num_textcmd, the count of the servertic_textcmd_t
            }
            netbuffer->u.serverpak.num_textcmd = num_txt; // the number of servertic_textcmd_t
        }

        netbuffer->u.serverpak.flags = flags | seq_num; // last packet
        packsize = bufpos - (byte*)&(netbuffer->u);
        HSendPacket( nnode, 0, 0, packsize );  // msg lost when too busy

        // Record next tic for this net node.
        // Extratic causes redundant transmission of tics.
        ti = (end_tic - doomcom->extratics);  // normal
        // When tic is too large, only one tic is sent so don't go backward !
        if( ti <= start_tic )
           ti = end_tic;  // num tics is too small for extratics
        if( ti < nettics[nnode] )
           ti = nettics[nnode];
        nextsend_tic[nnode] = ti;
    }
    // node 0 is me !
    nextsend_tic[0] = maketic;
}


// By Client. Server does not send to self.
// PT_SERVERTICS from Server.
static void servertic_handler( byte nnode )
{
    ticcmd_t * netcmd_p;  // player array
    byte * bufpos;  // netbuffer position, end-of-data
    byte * endbuffer;  // for buffer overrun tests
    servertic_textcmd_t * stcp;
    textcmdbuff_t * tcbuf;
    int    btic, j;
    uint16_t buflen;
    uint32_t cmd_player_mask;
    tic_t  start_tic, end_tic, ti;
    byte   start_tic_hash;
    byte   packetflags;
    byte   seqbits;
    byte   cmds_offset, num_cmds, num_txt;

    start_tic = ExpandTics (netbuffer->u.serverpak.starttic);
    end_tic   = start_tic + netbuffer->u.serverpak.numtics;

    if( end_tic > (gametic + BACKUPTICS))
        end_tic = (gametic + BACKUPTICS);  // limit to backup capability

    // Check if missed any packets.
    cl_packetmissed = (start_tic > cl_need_tic);

    if((start_tic > cl_need_tic) || (end_tic <= cl_need_tic))
       goto not_in_packet;

    // The needed tic is within this packet.
    packetflags = netbuffer->u.serverpak.flags;  // tic_packet_flag_e
    // one bit per packet section by seq, last fills upper bits to make 0xFF
    seqbits = 1 << (packetflags & TPF_seq);  // 0..7 => 0x01 .. 0x80
    if( ! (packetflags & TPF_more) )  seqbits = -seqbits;  // 0x01=>0xFF, 0x08=>0xF8

    // First the nettics then the net textcmds.
    bufpos = (byte*)netbuffer->u.serverpak.cmds;  // first ticcmd
    endbuffer = (byte*)& netbuffer->u.serverpak.cmds[NUM_SERVERTIC_CMD];  // after last content

    cmd_player_mask = read_N32( &netbuffer->u.serverpak.cmd_player_mask );
    cmds_offset = netbuffer->u.serverpak.cmds_offset;
    num_cmds = netbuffer->u.serverpak.num_cmds_present;
    if( num_cmds > NUM_SERVERTIC_CMD )  goto corrupt_packet;

    // Hash differs between this packet and previous usage of the same btic.
    // Could compute it for every tic in the packet, but is not necessary.
    // All extension packet have the same start_tic.
    // Do clear separately so out-of-order packets cannot invoke errors.
    // Otherwise, would have to appear in both ticcmds and textcmds code.
    start_tic_hash = btic_hash( start_tic ); 
    for( ti=start_tic; ti<end_tic; ti++ )
    {
        // client only
        btic = BTIC_INDEX( ti );
        if( netcmd_tic_hash[btic] != start_tic_hash )
        {
            // first packet for this tic, clear old stuff
            D_Clear_ticcmd(btic);
            netcmd_tic_hash[btic] = start_tic_hash;  // after clear
        }
        // When this was in textcmds, it would only record for those that
        // had actually sent a textcmd.
        // If that packet had num_cmds=0, then empty textcmds would never be ready.
        netseq[btic] |= seqbits; // record receiving this packet section
    }

    // Nettics
    // Some extension packets may have num_cmds = 0
    if( num_cmds && cmd_player_mask )
    {
        // client only
        // Update the player list according to cmd_player_mask
        if( cmd_player_mask != ticcmd_player_mask )
            CL_num_ticcmd_per( cmd_player_mask );  // num_ticcmd_per_tic

        ti = start_tic;
        if( cmds_offset > num_ticcmd_per_tic )
        {
            // extension packet after multiple tics
            byte whole_tics = cmds_offset / num_ticcmd_per_tic;  // whole tics in previous packet

            ti += whole_tics;
            cmds_offset -= whole_tics * num_ticcmd_per_tic;  // 0..num_ticcmd_per_tic
        }

        for( ; ti<end_tic; ti++)
        {
            if( num_cmds == 0 )  break;  // limited to ticcmd in this packet
            num_cmds --;

            // Copy the tics
            btic = BTIC_INDEX( ti );
            // btic limited to BACKUPTICS-1

            netcmd_p = netcmds[btic];  // player dest

            // when extension packet, cmds_offset is non-zero
            for( j = cmds_offset; j<num_ticcmd_per_tic; j++ )
            {
                // Use list of pid generated from cmd_player_mask.
                byte pn = ticcmd_pid[j];
                TicCmdCopy( &netcmd_p[pn], /*src*/ (ticcmd_t*) bufpos );
                bufpos += sizeof(ticcmd_t);
            }
            cmds_offset = 0;
        }
    }

    // [WDJ] Ver 1.48, easy to send in multiple packets
    // Some packets have textcmds, most do not.
    num_txt = netbuffer->u.serverpak.num_textcmd;  // number textcmd sections field
    while( num_txt-- )
    {
        // Tics that have a textcmd have a servertic_textcmd_t, unaligned.
        // All players and server textcmd are already packed into the buffer.
        stcp = (servertic_textcmd_t *)bufpos;

        buflen = read_N16( & stcp->len );  // length of servertic_textcmd_t content 
        if( (bufpos + buflen) > endbuffer )  goto textcmd_exceed_buffer;
        if( buflen > MAX_TEXTCMD_BUFF+1 )  goto textcmd_exceed_buffer;  // prevent dest overrun

        ti = ExpandTics( stcp->tic );
        if( (ti < start_tic) || (ti > end_tic) )  goto corrupt_packet;

        btic = BTIC_INDEX( ti );
        tcbuf = & textcmdbuff[btic];
        memcpy(&tcbuf->buff, &stcp->textitem, buflen);
        tcbuf->len = buflen;
        // Cannot add field sizes as the structure might be padded.
        bufpos += sizeof_servertic_textcmd_t( buflen );  // tic, len(2 bytes), buf
    }

    // Only advance to the next tic when all packet extensions are received.
    if( netseq[ BTIC_INDEX(start_tic) ] == 0xFF )
        cl_need_tic = end_tic;

    return;

 corrupt_packet:
    I_SoftError("Nettics: corrupt packet.\n");
    return;
 textcmd_exceed_buffer:
    I_SoftError("Nettics: textcmd exceed buffer: len=%i\n", buflen);
    return;
 not_in_packet:
    DEBFILE(va("Needed tic not in packet: needed tic %u, packet tic %u to %u\n", cl_need_tic, start_tic, end_tic));
    return;
}


//
// Get Packets and invoke their Server and Client handlers.
//
static void Net_Packet_Handler(void)
{
    byte client_pn;
    byte nnode, nodestate, packettype;

    while ( HGetPacket() )
    {
        nnode = doomcom->remotenode;  // 0..MAXNETNODES-1, BROADCAST_NODE
        nodestate = (nnode < MAXNETNODES)?  nnode_state[nnode] : NOS_invalid;
        packettype = netbuffer->packettype;

        // [WDJ] Run-time messages are given priority in speed of handling.

        // ---- SERVER handling packets of known clients nodes.
        if( server && (nodestate >= NOS_recognized))
        {
            // [WDJ] pn no longer has DRONE bit.
            client_pn = nnode_to_player[0][nnode];  // the player
            // Every message type must handle the invalid client_pn individually.

            // Messages handled by server, for known client nodes.
            switch(packettype)
            {
             case PT_CLIENTCMD  :
             case PT_CLIENT2CMD :
             case PT_CLIENTMIS  :
             case PT_CLIENT2MIS :
             case PT_NODEKEEPALIVE :
             case PT_NODEKEEPALIVEMIS :
                // updates nettics for invalid client_pn
                client_cmd_handler( packettype, nnode );
                continue;
             case PT_TEXTCMD2 : // splitscreen special
                client_pn = nnode_to_player[1][nnode];
                // fall through
             case PT_TEXTCMD :
                if( client_pn >= MAXPLAYERS )  // unused = 255
                {
                    // Do not ACK the packet from a strange client_pn.
                    Net_Cancel_Packet_Ack(nnode);
                    continue;
                }
                net_textcmd_handler( nnode );
                continue;
             case PT_NODE_TIMEOUT:
             case PT_CLIENTQUIT:
                // still closes the connection when invalid client_pn
                client_quit_handler( nnode, client_pn );
                continue;
             default:
                break;
            }
        }
           
        // ---- CLIENT Handling Server packets.
        if( (nnode == cl_servernode) && (cl_mode >= CLM_server_files) )  // from the known server
        {
            // Messages from the server, for known server.
            switch(packettype)
            {
             case PT_SERVERTICS :
                servertic_handler( nnode );
                continue;
             case PT_FILEFRAGMENT :
                if( !server )
                    Got_Filetxpak();
                continue;
             case PT_NETWAIT:
                if( !server )
                    netwait_handler();
                continue;
             case PT_STATE:
                if( !server )
                    state_handler();  // to client
                continue;
             case PT_SERVERPLAYER :
#ifdef WAIT_GAME_START_INTERMISSION
                if( ! server )
                    server_player_handler( nnode );
#endif
                continue;
             case PT_SERVERLEVEL :
                if( ! server )
                    server_level_handler( nnode );
                continue;
             case PT_REPAIR:
                repair_handler_client( nnode );  // from server
                continue;
             default:
                break;
            } // end switch
        }

        // Infrequent messages from server.
        if( nnode == cl_servernode )
        {
            // Any Client that knows the server.
            // These messages are recognized, but application is limited.
            switch(packettype)
            {
             case PT_STATE:
                if( !server )
                    state_handler();  // to client
                continue;
             case PT_CONTROL:
                control_msg_handler();
                continue;
             case PT_REQ_CLIENTCFG : // request client config
                D_Send_PlayerConfig();  // Client players, via NetXCmd to everybody
                    // client players only, does not cover bots
                continue;
             case PT_SERVERSHUTDOWN:
                if( ! server )
                    server_shutdown_handler();  // only client not on server
                continue;
             case PT_NODE_TIMEOUT:  // from server
                // must be before PT_NODE_TIMEOUT from client.
                if( ! server )
                    server_timeout_handler();  // only client not on server
                continue;
            }
        }
       
        // Infrequent messages, from anybody.
        if( server )
        {
            // Packet can be from client trying to join server.
            if( (nnode != cl_servernode) && (nodestate < NOS_recognized) )
            {
                // Client trying to Join.
                DEBFILE(va("Received packet from unknown host %d\n",nnode));
            }

            // Can only be handled by a server.
            switch(packettype)
            {
             case PT_TEXTCMD :
                // Server to server use of client channel for server NetXCmd, with no players.
                // No valid player num for this nnode.
                net_textcmd_handler( nnode );
                continue;
             case PT_ASKINFO:  // client has asked server for info
                // May have been sent to BROADCAST_NODE, but nnode is sender.
                server_askinfo_handler( nnode );
                continue;
             case PT_REQUESTFILE :
                if( cv_SV_download_files.EV == 0 )
                {
                    GenPrintf(EMSG_ver, "RequestFile: Blocked, Not Allowed\n" );
                    continue;
                }
                Got_RequestFilePak(nnode);
                continue;
             case PT_CLIENTJOIN:
                client_join_handler( nnode );
                continue;
             case PT_CLIENTREADY:
                ready_handler( nnode );
                continue;
             case PT_REQ_SERVERPLAYER :
#ifdef WAIT_GAME_START_INTERMISSION
                SV_Send_PlayerState( nnode );
#endif
                continue;
             case PT_REQ_SERVERLEVEL :
                SV_Send_LevelCfg( nnode );
                continue;
             case PT_REPAIR:
                if( nodestate >= NOS_recognized )
                    repair_handler_server( nnode );  // from client
                continue;
             case PT_NODE_TIMEOUT:  // from unknown client
             case PT_CLIENTQUIT:  // when unknown client
                Net_CloseConnection(nnode, 0);  // normal closing
                continue;
            }
        }

        // Prospective Client, Client handling of server messages.
        if( cl_mode != CLM_connected )  // Protect against rogue interference.
        {
            // Messages to client before connected. They change client state.
            switch(packettype)
            {
             case PT_SERVERINFO:
                // response from server to BROADCAST_NODE message, should have nnode same as new cl_servernode
                server_info_handler( nnode );
                continue;
             case PT_SERVERREFUSE : // negative response of client join request
                server_refuse_handler( nnode );
                continue;
             case PT_SERVERCFG :    // positive response of client join request
                server_cfg_handler( nnode );
                continue;
             case PT_FILEFRAGMENT :
                // handled in d_netfil.c
                if( !server )
                    Got_Filetxpak();
                continue;
             case PT_SERVERTICS:
                // do not remove my own server
                // (we have just to get a out of order packet)
                if( nnode == cl_servernode )  continue;
#if 1
                goto invalid;  // Server tic from unknown source.
#else
                break;  // kill it
#endif
            }
        }

        switch(packettype)
        {
         case PT_ASKINFO:  // broadcast, non-servers should ignore it
         case PT_SERVERCFG :  // server cfg at wrong time
         case PT_NODE_TIMEOUT:  // errant
            continue;  // ignore
         case PT_REQUESTFILE :
            if( ! server )  goto server_only;
            continue;
        }

        // Packet not accepted.
        if((nnode >= MAXNETNODES) || (nodestate < NOS_recognized))
        {
            DEBFILE(va("Unknown packet received (%d) from unknown host !\n", packettype));
            goto close_node;
        }

        DEBFILE(va("Unknown packet type: type %d node %d\n", packettype, nnode));
        continue;
       
    server_only:
        GenPrintf(EMSG_warn, "Recv warn: unknown node=%i, Client ignores server only packet type (%d).\n", nnode, packettype );
        DEBFILE(va("Recv warn: unknown node=%i, Client ignores server only packet type (%d).\n", nnode, packettype));
        goto close_node;

#if 1
    invalid:
        GenPrintf(EMSG_warn, "Recv warn: unknown node=%i, packet with unknown type (%d)\n", nnode, packettype );
        DEBFILE(va("Recv warn: unknown node=%i, packet received unknown type (%d)\n", nnode, packettype));
        goto close_node;
#endif
       
    close_node:
        Net_CloseConnection(nnode, 0);  // a temp connection
        continue;       
    } // end while

    if( server && network_wait_pause )
        SV_network_wait_handler();
}


// ----- NetUpdate
// Builds ticcmds for console player,
// sends out a packet

// no more use random generator, because at very first tic isn't yet synchronized
static int16_t Consistency(void)
{
    int16_t ret=0;
    int   pn;

    DEBFILE(va("TIC %d ",gametic));
    for(pn=0; pn<MAXPLAYERS; pn++)
    {
        if( playeringame[pn] && players[pn].mo )
        {
            DEBFILE(va("p[%d].x = %f ", pn, FIXED_TO_FLOAT(players[pn].mo->x)));
            ret += players[pn].mo->x;
        }
    }
    DEBFILE(va("pos = %d, rnd %d\n", ret, P_Rand_GetIndex()));
    ret+=P_Rand_GetIndex();

    return ret;
}



//
// TryRunTics
//
static void Local_Maketic(int realtics)
{
    rendergametic=gametic;
    // translate inputs (keyboard/mouse/joystick) into game controls
    // [Arcade] One per local player.  Gated on the player actually having
    // joined (localplayer[pind] valid) rather than on cv_splitscreen and
    // displayplayer2_ptr, which only ever described the second of two: a
    // third or fourth panel has controls and a player but no view.
    {
        byte pind, nlocal = D_NumLocalPlayers();
        for( pind=0; pind<nlocal; pind++ )
        {
            if( pind > 0 && localplayer[pind] >= MAXPLAYERS )  continue;
            G_BuildTiccmd(&localcmds[pind], realtics, pind);
        }
    }

#ifdef CLIENTPREDICTION2
    if( !paused && localgametic<gametic+BACKUPTICS)
    {
        P_MoveSpirit ( &players[consoleplayer], &localcmds[0], realtics );
        localgametic+=realtics;
    }
#endif
#ifdef TICCMD_148
    // moved to G_BuildTiccmd
#else
    localcmds[0].angleturn |= TICCMD_RECEIVED;
#endif
}

void SV_SpawnPlayer( byte playernum, int x, int y, angle_t angle )
{
    // for futur copytic use the good x,y and angle!
    if( server )
    {
        ticcmd_t * tc = &netcmds[ BTIC_INDEX(maketic) ][playernum];
#ifdef CLIENTPREDICTION2
        tc->x = x;
        tc->y = y;
#endif
#ifdef TICCMD_148
        tc->angleturn = (angle>>16);
        tc->ticflags = TC_received;
#else
        tc->angleturn=(angle>>16) | TICCMD_RECEIVED;
#endif
    }
}

// create missed tic
void SV_Maketic(void)
{
    int btic = BTIC_INDEX( maketic );
    int bticprev, i, player;
    byte nnode;

    for(nnode=0; nnode<MAXNETNODES; nnode++)
    {
        if(playerpernode[nnode] == 0)  continue;  // not in use or DRONE
       
        // Detect missing ticcmd, using only player[0] at the node.
        player = nnode_to_player[0][nnode];
#ifdef TICCMD_148
        if((netcmds[btic][player].ticflags & TC_received) == 0)
#else
        if((netcmds[btic][player].angleturn & TICCMD_RECEIVED) == 0)
#endif
        {
            // Catch startup glitch where playerpernode gets set
            // before the player is actually setup.
            if( ! playeringame[player] )  continue;

            DEBFILE(va("MISS tic %4u for node %d\n", maketic, nnode));
#ifdef PARANOIA
            if( devparm > 1 )
                GenPrintf(EMSG_dev, "\2Client %d Miss tic %d\n", nnode, maketic);
#endif
            // Copy the previous tic
            bticprev = BTIC_INDEX(maketic-1);
            for(i=0; i<playerpernode[nnode]; i++)
            {
                // All players at the node
                netcmds[btic][player] = netcmds[bticprev][player];
#ifdef TICCMD_148
                netcmds[btic][player].ticflags &= ~TC_received;
#else
                netcmds[btic][player].angleturn &= ~TICCMD_RECEIVED;
#endif
                player = nnode_to_player[1][nnode];
            }
        }
    }
    // all tic are now present, make the next
    maketic++;
}

#ifdef DEBUGFILE
static  int     net_load;
#endif

//  realtics: 0..5
void TryRunTics (tic_t realtics)
{
    // the machine have laged but is not so bad
    if(realtics>TICRATE/7) // FIXME: consistency failure!!
    {
        if(server)
            realtics=1;
        else
            realtics=TICRATE/7;
    }

    if(singletics)
        realtics = 1;

    if( realtics > 0 )
        COM_BufExecute( CFG_none );

    NetUpdate();

    if(demoplayback)
    {
        cl_need_tic = gametic + realtics + cv_playdemospeed.value;
        // start a game after a demo
        maketic+=realtics;
        next_tic_send=maketic;
        next_tic_clear=next_tic_send;
    }

#ifdef DEBUGFILE
    if(realtics==0)
        if(net_load) net_load--;
#endif
    Net_Packet_Handler();

#ifdef DEBUGFILE
    if (debugfile && (realtics || cl_need_tic>gametic))
    {
        //SoM: 3/30/2000: Need long int in the format string for args 4 & 5.
        fprintf (debugfile,
                 "------------ Tryruntic : REAL:%lu NEED:%lu GAME:%lu LOAD: %i\n",
                 (unsigned long)realtics, (unsigned long)cl_need_tic,
                 (unsigned long)gametic, net_load);
        net_load=100000;
    }
#endif

    if( gamestate == GS_WAITINGPLAYERS )
    {
        // Server is waiting for network players.
        // To wait, execution must not reach G_Ticker.
        if( network_state < NETS_open )
            goto error_ret;  // connection closed by cancel or timeout
       
        if( realtics <= 0 )
            return;
       
        // Once per tic
        // Wait for players before netgame starts.
        if( ! D_WaitPlayer_Ticker() )
            return;  // Waiting

        // Start game
        if( dedicated )
            gamestate = GS_DEDICATEDSERVER;
        // Others must wait for load game to set gamestate to GS_LEVEL.
    }
    else if( gamestate == GS_INTERMISSION )
    {
        // [WDJ] Server runs timer, even when client is not-active.
        // G_Ticker does the GS_INTERMISSION effects.
        // Timer for start of next game level.
        if(( wait_game_start_timer > 0 ) && ! paused && (realtics > 0))
        {
            wait_game_start_timer--;
            if( wait_game_start_timer == 0 )
            {
                S_StartSound(sfx_slop);

                WI_Init_NoState();  // start transition to G_NextLevel
            }

            if( (wait_game_start_timer & 0x1F) == 0x03 )
            {
                // Update the wait timer, to keep everyone in sync
                SV_Send_control( BROADCAST_NODE, CTRL_wait_timer, 0, wait_game_start_timer, 255 );
            }
        }
    }

    // Server keeps forcing (cl_need_tic = maketic) in SV_Send_Tic_Update,
    // as it does not send tic packets to itself.
    if( cl_need_tic > gametic )
    {
        // All ticcmd and textcmd for gametic to cl_need_tic are present.

        if (demo_ctrl == DEMO_seq_advance)  // and not disabled
        {
            D_DoAdvanceDemo ();
            return;
        }

        // Run the count * tics
        while( cl_need_tic > gametic )
        {
#ifdef CLIENTPREDICTION2
            DEBFILE(va("==== Run tic %u (local %d)\n",gametic, localgametic));
#else
            DEBFILE(va("==== Run tic %u\n",gametic));
#endif

            if(demoplayback)
            {
                // Clean up textcmd, so demoplay can append.
                D_Clear_ticcmd( gametic );
            }

            G_Ticker ();
            ExtraDataTicker();  // execute NetXCmd
            gametic++;
            // skip paused tic in a demo
            if(demoplayback)
            {
                if(paused)
                   cl_need_tic++;
            }
            else
            {
                // Consistency is calculated first thing in gametic.
                consistency[ BTIC_INDEX( gametic ) ] = Consistency();
            }
        }
    }
    return;

error_ret:
    D_StartTitle();
    return;
}

// By Server
// Send Tic updates
static void SV_Send_Tic_Update( int count )
{
    byte nn;

    // Find lowest tic, over all nodes.
    next_tic_send = gametic;
    for( nn=0; nn<MAXNETNODES; nn++)
    {
        // Max of gametic and nettics[].
        if( (nnode_state[nn] >= NOS_active)
            && (nettics[nn] < next_tic_send) )
        {
           next_tic_send = nettics[nn];
        }
    }

    // Don't erase tics not acknowledged
    if( (maketic+count) >= (next_tic_send+BACKUPTICS) )
        count = (next_tic_send+BACKUPTICS) - maketic - 1;

    while( count-- > 0 )
        SV_Maketic();  // create missed tics and increment maketic

    // clear only when acknowledged
    for( ; next_tic_clear<next_tic_send; next_tic_clear++)
        D_Clear_ticcmd(next_tic_clear);  // clear the maketic the new tic

    SV_Send_Tics();

    // The server is a client too, but does not send cmdtic packets to itself.
    cl_need_tic=maketic;
}


void NetUpdate(void)
{
    static tic_t prev_netupdate_time=0;
    tic_t        nowtime;
    int          realtics;	// time is actually long [WDJ]

    nowtime  = I_GetTime();
    realtics = nowtime - prev_netupdate_time;

    if( realtics <= 0 )
    {
        if( realtics > -100000 )  // [WDJ] 1/16/2009  validity check
            return;     // same tic as previous

        // [WDJ] 1/16/2009 something is wrong, like time has wrapped.
        // Program gets stuck waiting for this, so force it out.
        realtics = 1;
    }
    if( realtics > 5 )
    {
        realtics = ( server )? 1 : 5;
    }

    // Executed once per tic, with realtics = 1..5.
    prev_netupdate_time = nowtime;

    if( !server )
        maketic = cl_need_tic;

    if( ! dedicated )
    {
        // Local Client
        I_OsPolling();       // i_getevent
        D_Process_Events ();
          // menu responder ???!!!
          // Cons responder
          // game responder call :
          //    HU_responder,St_responder, Am_responder
          //    F_responder (final)
          //    and G_MapEventsToControls

        Local_Maketic (realtics);  // make local tic

        if( server && !demoplayback )
            CL_Send_ClientCmd();     // send server tic
    }

    Send_localtextcmd();  // includes server NetXCmd cmds
   
    Net_Packet_Handler();  // get packet from client or from server

    // Client sends its commands after a receive of the server tic.
    // The server sends before because in single player is better.

    if( !server )
        CL_Send_ClientCmd();   // send tic cmd
    else
    {
        // By Server
        //Hurdler: added for acking the master server
        if( cv_internetserver.value )
            MS_SendPing_MasterServer( nowtime );

        if(!demoplayback)
            SV_Send_Tic_Update( realtics );  // realtics > 0

        if( Filetx_file_cnt )  // Rare to have file download in progress.
            Filetx_Ticker();

        if( num_join_waiting_players )
            SV_Add_Join_Waiting();
    }

    Net_AckTicker();

    if( ! dedicated )
    {
        M_Ticker ();
        CON_Ticker();
    }
}
