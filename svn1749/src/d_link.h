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
#define LK_NAME_LEN        16    // cabinet name, NUL included
#define LK_ID_SHORT_LEN    10    // "7F3A-91C2" and its NUL
#define LK_PORT_DEFAULT    5030

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
} lk_peer_info_t;

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

// The operator page body (Arcade Options -> Cabinet Link).  The caller sets
// up drawing and the title, as the Audit page does for AU_Drawer.
void        LK_Drawer( void );

#endif
