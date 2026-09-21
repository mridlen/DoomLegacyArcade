// [Arcade] Cabinet Link: networked cabinets.
//
// Two or more cabinets on a home network pair with a passcode over TLS and
// report to each other what they are doing; later phases share high scores
// and demos and invite each other into multiplayer games.  The design, the
// security model and what has been verified are in docs/arcade/cabinet-link.md.
//
// Every function here exists in every build.  Without HAVE_LINK (no OpenSSL)
// d_link.c compiles them to stubs and LK_Built() says so; callers never test
// HAVE_LINK themselves, which is what lets the option change without a clean.

#ifndef D_LINK_H
#define D_LINK_H

#include "doomtype.h"

// Maximum cabinets a master accepts, beyond itself.  A cabinet is a node to
// the game netcode, which allows MAXNETNODES (32); the link never needs more.
#define LK_MAX_PEERS       31
#define LK_MAX_ALLOW       LK_MAX_PEERS   // addresses on a master's allow list
#define LK_NAME_LEN        16    // cabinet name, NUL included
#define LK_ID_SHORT_LEN    10    // "7F3A-91C2" and its NUL
#define LK_PORT_DEFAULT    5030
#define LK_FP_BYTES        32    // a cabinet's full id: SHA-256 of its public key
#define LK_GAME_LEN        40    // game id: IWAD name plus level pack, as scored
#define LK_MSG_DATA_MAX    256   // largest game message carried over the link

typedef enum
{
    LK_ROLE_OFF = 0,
    LK_ROLE_MASTER,
    LK_ROLE_MEMBER
} lk_role_e;

// What a cabinet is doing, as other cabinets see it.  See the table in
// cabinet-link.md ("Cabinet state, as the link sees it").
typedef enum
{
    LK_STATE_IDLE = 0,    // attract cycle, no menu
    LK_STATE_MENU,        // someone in the menus over attract
    LK_STATE_JOINING,     // its own join screen is up
    LK_STATE_HOSTING,     // its join screen has a remote cabinet in it
    LK_STATE_PLAYING,     // level, intermission or finale
    LK_STATE_SIGNING,     // high score initials entry
    LK_STATE_DEVMODE,     // an operator session
    LK_NUM_STATES
} lk_state_e;

typedef enum
{
    LK_PEER_EMPTY = 0,
    LK_PEER_CONNECTING,   // TCP/TLS in progress
    LK_PEER_AUTHENTICATING,
    LK_PEER_ONLINE,       // authenticated, exchanging presence
    LK_PEER_REFUSED,      // failed before authenticating; see reason
    LK_PEER_OFFLINE       // was online, then went away; see reason
} lk_peer_status_e;

// A snapshot of one peer for the operator page and the console.  Copied out
// of the link thread under its lock, so it is safe to read on the game thread.
typedef struct
{
    lk_peer_status_e  status;
    char        name[LK_NAME_LEN];
    char        id_short[LK_ID_SHORT_LEN];
    char        address[48];
    char        build[32];
    lk_state_e  state;
    byte        panels;
    char        reason[64];   // why REFUSED, or empty
    byte        fp[LK_FP_BYTES];     // full id, for addressing a message (zero if unknown)
    char        game[LK_GAME_LEN];   // what it is running ("doom2", "doom2+dwango5")
} lk_peer_info_t;

// Game messages the link carries between cabinets (d_linkgame.c).  The link is
// only transport, except that the master numbers every INVITE as it passes
// through and answers its sender with INVITE_ACK: that order is what decides
// which of two cabinets opening the same game at once becomes the host.
enum
{
    LK_GM_INVITE = 1,     // u32 seq (filled in by the master), u32 nonce, ...
    LK_GM_INVITE_ACK,     // u32 seq, u32 nonce
    LK_GM_CANCEL,
    LK_GM_STATUS,
    LK_GM_START,
    // [Arcade] Select Game Sync (d_linksel.c)
    LK_GM_GAME_SELECTED,  // game id: a member tells its master a player chose it
    LK_GM_GAME_SWITCH,    // u32 serial, game id: the master tells a member to follow
    LK_GM_GAME_CANNOT,    // u32 serial, game id, reason: a member could not
    LK_GM_NUM
};

typedef struct
{
    byte        source[LK_FP_BYTES];   // who sent it -- set by the link, never by the sender
    byte        type;                  // LK_GM_*
    uint16_t    len;
    byte        data[LK_MSG_DATA_MAX];
} lk_event_t;

// Is the link built into this binary (OpenSSL present at build time)?
boolean     LK_Built( void );

// Load the link settings and this cabinet's identity from legacyhome/link/.
// Called once from D_DoomMain after legacyhome is resolved.  Starts nothing.
void        LK_Init( void );

// Called every pass of D_DoomLoop.  Starts the link thread on first use when
// the link is switched on, publishes this cabinet's state and handles what
// the thread has queued for the game thread.  Cheap when there is nothing.
void        LK_Ticker( void );

// Stop the link thread and close every connection.  Called on quit.
void        LK_Shutdown( void );

// This cabinet's role, name and short identity (empty when not built).
lk_role_e   LK_Role( void );
const char* LK_Name( void );
const char* LK_Id_Short( void );

// Copy out up to max peers; returns how many were written.
int         LK_Peers( lk_peer_info_t * out, int max );

const char* LK_State_Name( lk_state_e st );

// --- Messages between cabinets (game thread) ---

// Send a game message.  target NULL goes to every other cabinet on the link.
// False when the link is not running or the message is too big.
boolean     LK_Send( const byte * target, byte type, const byte * data, int len );

// Take the next message that arrived for this cabinet, if any.
boolean     LK_Poll_Event( lk_event_t * ev );

// This cabinet's full id, or NULL before the link has an identity.
const byte* LK_My_Fp( void );

// [Arcade] Cabinet names this one knows: its own first, then every pinned
// peer.  Copies into out and returns how many were written.
//
// The names come from the pins (legacyhome/link/pins.txt), which LK_Init loads
// *before* config.cfg is read -- that ordering is what lets Music Cabinet be an
// ordinary saved cvar whose value is a cabinet name, instead of one that
// forgets the operator's choice on every restart because the list was still
// empty when the config loaded.
int         LK_Known_Names( char (*out)[LK_NAME_LEN], int max );

// [Arcade] Is a linked game running on this cabinet right now?  True only
// while the sealed game channel is up, which is exactly "playing with other
// cabinets" -- a solo game, Single Level, or the attract screen is false.
boolean     LK_In_Linked_Game( void );

// Look a cabinet up by full id among the peers this one can see.
boolean     LK_Peer_Find( const byte * fp, lk_peer_info_t * out );

// This cabinet's game id, the same string other cabinets are told.
const char* LK_Game_Id( void );

// What this cabinet is doing right now, as it tells the others.
lk_state_e  LK_State( void );

// This cabinet's build, as HELLO tells the others ("v1.2-3-gabc1234"): the
// same string a peer's build field is compared with.
const char* LK_Build( void );

// --- Shared scores: the sync channel (d_linkscore.c) ---
//
// Opaque chunks between a member and its master only, never relayed: scores
// sync through the master, which is what keeps two members from having to
// trust each other's copies.  The receiver pulls (d_linkscore.c asks for a few
// chunks at a time), so these queues are small and a full one simply means
// "ask again next tic".

#define LK_SYNC_DATA_MAX   4096

typedef struct
{
    byte        peer[LK_FP_BYTES];   // from (Poll) or to (Send)
    uint16_t    len;
    byte        data[LK_SYNC_DATA_MAX];
} lk_sync_msg_t;

boolean     LK_Sync_Send( const byte * peer, const byte * data, int len );
boolean     LK_Sync_Poll( lk_sync_msg_t * out );
// The cabinets this one syncs scores with, online right now: a member's master,
// or every member of a master.
int         LK_Sync_Peers( lk_peer_info_t * out, int max );
// SHA-256 (all zero when the link is not built).
void        LK_Sha256( const byte * data, int len, byte * out32 );

// --- The game channel (docs/arcade/cabinet-link.md, "The game channel") ---
//
// While a linked game is on, every UDP packet of the game netcode is sealed
// with ChaCha20-Poly1305 under keys the host handed out over the link, and
// anything that does not open is dropped before the netcode sees a byte.  A
// cabinet with the link switched on accepts no unauthenticated game traffic
// at all, linked game or not.

#define LK_UDP_KEYS      64      // two 32-byte keys: member-to-host, host-to-member
#define LK_UDP_OVERHEAD  21      // key id, counter, tag

// Host: start a session with no clients, then add one per joining cabinet.
// Add_Client writes that cabinet's keys and returns its key id (1..31, 0 = full).
void        LK_Udp_Host_Begin( void );
int         LK_Udp_Host_Add_Client( byte * keys_out );
// A joining cabinet: its key id and keys, from the host's START.
void        LK_Udp_Client_Begin( byte keyid, const byte * keys );
// The linked game is over: forget every key.
void        LK_Udp_End( void );

// For i_tcp.c.  ip and port in network byte order, as in a sockaddr_in.
// Recv: returns the plaintext length written to out, 0 to drop the packet,
// or -1 when the link is not involved (use the packet as it came).
// Send: returns the sealed length written to out, 0 to drop, or -1 to send the
// packet as it is.
int         LK_Net_Recv( const byte * in, int len, byte * out, int outsize,
                         uint32_t ip, uint16_t port );
int         LK_Net_Send( const byte * in, int len, byte * out, int outsize,
                         uint32_t ip, uint16_t port );

// The operator page's status block (Arcade Options -> Cabinet Link): whether
// the link is running, then every other cabinet it can see, from y down to
// y_end.  The caller sets up drawing and draws the settings rows above it.
void        LK_Drawer( int y, int y_end );

// --- Settings, for the Cabinet Link page and link_set ---
//
// Only in an operator (-devmode) session.  Every change is saved to
// legacyhome/link/link.cfg at once and restarts the link, exactly as link_set
// does -- the functions below are what link_set calls.  Each returns NULL when
// it worked, or a short reason when it did not.

typedef enum
{
    LK_SET_ROLE,        // "off", "master", "member"
    LK_SET_NAME,
    LK_SET_MASTER,      // the master's address, on a member
    LK_SET_PORT,        // 1..65535
    LK_SET_PASSCODE
} lk_setting_e;

// The value as text -- the passcode in the clear: only an operator sees it.
void        LK_Setting_Get( lk_setting_e which, char * out, int outsize );
const char* LK_Setting_Set( lk_setting_e which, const char * value );

// A master's allow list.
int         LK_Allow_Count( void );
const char* LK_Allow_Get( int i );
const char* LK_Allow_Add( const char * address );
const char* LK_Allow_Remove( int i );

// Forget every paired cabinet (a replaced master, a reinstalled member).
const char* LK_Forget_Pins( void );

#endif
