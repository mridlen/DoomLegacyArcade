// [Arcade] Cabinet Link: Select Game Sync.  See d_linksel.h and
// docs/arcade/cabinet-link.md, "Select Game Sync".
//
// A selection is an event, not a standing target: the master remembers the
// latest one and tells each cabinet about it once.  A cabinet that followed,
// or said it cannot, is not asked again -- so a cabinet whose idle timeout
// later unloads the pack it followed is not dragged back onto it, and a
// selection nobody can follow does not nag for ever.  One that is busy (in a
// game, signing the board, on a join screen, in an operator session) is asked
// again once it is back on its attract screen or in its menus.
//
// The latest selection lives only in the master's memory.  A restart that a
// selection causes carries -linkselected (M_Restart_Program_Ex), so a master
// that switched game still knows why, and a member knows to tell its master
// once the link is up again.  Any other restart forgets it.
//
// Message payloads (little-endian):
//   GAME_SELECTED  game[LK_GAME_LEN]
//   GAME_SWITCH    u32 serial, game[LK_GAME_LEN]
//   GAME_CANNOT    u32 serial, game[LK_GAME_LEN], reason[LKSEL_REASON_LEN]

#include "doomincl.h"
#include "doomstat.h"
#include "d_link.h"
#include "d_linksel.h"
#include "m_menu.h"
#include "m_argv.h"
#include "i_system.h"
#include "command.h"
#include "m_misc.h"
#include "w_wad.h"     // numwadfiles, for the status line

#include <ctype.h>
#include <stdlib.h>

#define LKSEL_SWITCH_LEN   (4 + LK_GAME_LEN)
#define LKSEL_REASON_LEN   48
#define LKSEL_CANNOT_LEN   (4 + LK_GAME_LEN + LKSEL_REASON_LEN)
#define LKSEL_NOTE_LEN     96
// A member told to switch restarts and drops off the link; one still online
// with the old game this long after being told did not do it (it was busy by
// the time the message landed), so it is told again.
#define LKSEL_RESEND_TICS  (15*TICRATE)

extern consvar_t  cv_link_gamesync;   // m_menu.c

typedef struct
{
    boolean   used;
    byte      fp[LK_FP_BYTES];
    uint32_t  done_serial;    // this selection is dealt with: followed, or cannot
    uint32_t  sent_serial;
    uint32_t  waited_serial;  // said once that it is busy
    tic_t     sent_at;
    char      note[LKSEL_NOTE_LEN];   // why it did not follow, for the page
} lksel_peer_t;

static boolean   lksel_inited;
static boolean   lksel_announce;              // a player chose this game here
static char      lksel_target[LK_GAME_LEN];   // master: the latest selection, "" none
static uint32_t  lksel_serial;
static uint32_t  lksel_self_done;             // master: the selection this cabinet dealt with
static char      lksel_self_note[LKSEL_NOTE_LEN];
static lksel_peer_t  lksel_peers[LK_MAX_PEERS];

// member: the last selection it could not follow, re-sent whenever its master
// comes back online -- a master restarted into an operator session has
// forgotten, and that session is exactly when someone looks at the page.
static boolean   lksel_master_online;
static byte      lksel_cannot[LKSEL_CANNOT_LEN];
static boolean   lksel_have_cannot;

// -linktest -linkselectat S name: choose a game on the Select Game page S
// seconds after the link starts, as a player would.
static const char * lksel_test_name;
static int       lksel_test_secs;
static tic_t     lksel_test_start;

static void  put32( byte * p, uint32_t v )
{
    p[0] = v & 0xff;  p[1] = (v >> 8) & 0xff;  p[2] = (v >> 16) & 0xff;  p[3] = (v >> 24) & 0xff;
}
static uint32_t  get32( const byte * p )
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Copy a fixed-size string field off the wire, terminated and printable.
static void  lksel_field( char * out, const byte * in, int size )
{
    int i;
    memcpy( out, in, size );
    out[size-1] = 0;
    for( i = 0; out[i]; i++ )
        if( out[i] < ' ' || out[i] > '~' )  out[i] = '?';
}

static void  lksel_upper( char * s )
{
    for( ; *s; s++ )  *s = toupper( (unsigned char) *s );
}

// Attract screen or menus: what an invite may interrupt, and so what a game
// switch may.  Never a game, the initials page, a join screen or an operator.
static boolean  lksel_can_switch_now( lk_state_e st )
{
    return st == LK_STATE_IDLE || st == LK_STATE_MENU;
}

static const char *  lksel_peer_name( const byte * fp )
{
    static lk_peer_info_t  info;
    if( LK_Peer_Find( fp, &info ) && info.name[0] )  return info.name;
    return "another cabinet";
}

static lksel_peer_t *  lksel_slot( const byte * fp, const lk_peer_info_t * online, int n )
{
    int i, j;
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lksel_peers[i].used && ! memcmp( lksel_peers[i].fp, fp, LK_FP_BYTES ) )
            return &lksel_peers[i];
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( ! lksel_peers[i].used )  break;
    if( i == LK_MAX_PEERS )
    {
        // Full: take the slot of a cabinet that is not online now.
        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            for( j = 0; j < n; j++ )
                if( ! memcmp( lksel_peers[i].fp, online[j].fp, LK_FP_BYTES ) )  break;
            if( j == n )  break;
        }
        if( i == LK_MAX_PEERS )  return NULL;
    }
    memset( &lksel_peers[i], 0, sizeof(lksel_peers[i]) );
    lksel_peers[i].used = true;
    memcpy( lksel_peers[i].fp, fp, LK_FP_BYTES );
    return &lksel_peers[i];
}

// Master: a new selection.  Everything learned about the last one is dropped.
static void  lksel_new_target( const char * game )
{
    int i;
    dl_strncpy( lksel_target, game, LK_GAME_LEN );
    lksel_serial++;
    if( lksel_serial == 0 )  lksel_serial = 1;
    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        lksel_peers[i].done_serial = 0;
        lksel_peers[i].sent_serial = 0;
        lksel_peers[i].waited_serial = 0;
        lksel_peers[i].note[0] = 0;
    }
    lksel_self_done = 0;
    lksel_self_note[0] = 0;
}

static void  lksel_send_switches( void );

// ---------------------------------------------------------------------------

void  LKSEL_Selected( void )
{
    lksel_announce = true;
}

// A pick is about to restart this cabinet: tell the others now, so they start
// switching while this one restarts instead of after it has reconnected.
// Measured on the laptop, the other cabinet began its own switch 3.8 s after
// the pick when it waited (1.8 s of it this cabinet hashing its music wads
// again, 0.9 s the link's handshake); on a Pi 3 the wait is longer still.
// -linkselected still goes on the restart, in case this does not arrive.
void  LKSEL_Before_Restart( const char * game )
{
    lk_peer_info_t  master;
    byte  msg[LK_GAME_LEN];

    if( ! M_Link_Game_Id_Valid( game ) )  return;
    switch( LK_Role() )
    {
     case LK_ROLE_MEMBER:
        if( LK_Sync_Peers( &master, 1 ) < 1 )  return;
        memset( msg, 0, sizeof(msg) );
        dl_strncpy( (char*) msg, game, LK_GAME_LEN );
        if( LK_Send( master.fp, LK_GM_GAME_SELECTED, msg, sizeof(msg) ) )
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: told %s this cabinet selected %s, before restarting\n",
                       master.name, game );
        break;
     case LK_ROLE_MASTER:
        if( ! cv_link_gamesync.EV )  return;
        if( strcasecmp( game, lksel_target ) )
        {
            lksel_new_target( game );
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s selected here, before restarting; the other cabinets follow\n", game );
        }
        lksel_self_done = lksel_serial;
        lksel_send_switches();
        break;
     default:
        break;
    }
}

const char *  LKSEL_Peer_Status( const byte * fp )
{
    int i;
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lksel_peers[i].used && ! memcmp( lksel_peers[i].fp, fp, LK_FP_BYTES ) )
            return lksel_peers[i].note;
    return "";
}

const char *  LKSEL_Self_Status( void )
{
    return lksel_self_note;
}

void  LKSEL_Status_Print( void )
{
    // wads= is what really loaded: a pack added in place reports "Added file"
    // on the console only, and game= comes from the menu's own bookkeeping.
    GenPrintf( EMSG_errlog, "LINKSEL sync=%d game=%s wads=%d target=%s announce=%d note=%s\n",
               cv_link_gamesync.EV, LK_Game_Id(), numwadfiles, lksel_target[0] ? lksel_target : "-",
               lksel_announce, lksel_self_note[0] ? lksel_self_note : "-" );
    {
        int i;
        for( i = 0; i < LK_MAX_PEERS; i++ )
            if( lksel_peers[i].used && lksel_peers[i].note[0] )
                GenPrintf( EMSG_errlog, "LINKSELPEER %s %s\n",
                           lksel_peer_name( lksel_peers[i].fp ), lksel_peers[i].note );
    }
}

// ---------------------------------------------------------------------------

static void  lksel_on_selected( const lk_event_t * ev )
{
    char  game[LK_GAME_LEN];

    if( LK_Role() != LK_ROLE_MASTER || ev->len != LK_GAME_LEN )  return;
    lksel_field( game, ev->data, LK_GAME_LEN );
    if( ! M_Link_Game_Id_Valid( game ) )  return;

    if( ! cv_link_gamesync.EV )
    {
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s selected %s (Select Game Sync is off)\n",
                   lksel_peer_name( ev->source ), game );
        return;
    }
    // The same pick again: a member tells its master just before it restarts
    // and once more after (-linkselected), in case the first did not arrive.
    if( strcasecmp( game, lksel_target ) )
    {
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s selected %s; the other cabinets follow\n",
                   lksel_peer_name( ev->source ), game );
        lksel_new_target( game );
    }
    // The cabinet that chose it is there, or on its way: it may still be
    // showing its old game while it restarts.
    {
        lk_peer_info_t  peers[LK_MAX_PEERS];
        lksel_peer_t * s = lksel_slot( ev->source, peers, LK_Peers( peers, LK_MAX_PEERS ) );
        if( s )  s->done_serial = lksel_serial;
    }
}

static void  lksel_on_switch( const lk_event_t * ev )
{
    lk_peer_info_t  master;
    char  game[LK_GAME_LEN], reason[LKSEL_REASON_LEN];
    const char * why;
    uint32_t serial;

    if( LK_Role() != LK_ROLE_MEMBER || ev->len != LKSEL_SWITCH_LEN )  return;
    // Only this cabinet's own master may tell it to switch.  A member's
    // message relayed by the master carries that member as its source.
    if( LK_Sync_Peers( &master, 1 ) < 1 || memcmp( master.fp, ev->source, LK_FP_BYTES ) )  return;
    serial = get32( ev->data );
    lksel_field( game, ev->data + 4, LK_GAME_LEN );
    if( ! M_Link_Game_Id_Valid( game ) )  return;

    // A choice made here, not yet told to the master, is the newer one.
    if( lksel_announce )  return;
    if( ! strcasecmp( game, LK_Game_Id() ) )  return;
    // Busy: the master asks again once this cabinet is free.
    if( ! lksel_can_switch_now( LK_State() ) )  return;

    why = M_Link_Game_Why_Not( game );
    if( ! why )
    {
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: switching to %s, selected on the link\n", game );
        why = M_Link_Follow_Game( game, false );   // does not return when it restarts
    }
    if( ! why )
    {
        lksel_self_note[0] = 0;
        lksel_have_cannot = false;
        return;
    }

    snprintf( lksel_self_note, sizeof(lksel_self_note), "GAME SYNC: %s", why );
    lksel_upper( lksel_self_note );
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: cannot switch to %s: %s\n", game, why );
    memset( reason, 0, sizeof(reason) );
    dl_strncpy( reason, why, sizeof(reason) );
    put32( lksel_cannot, serial );
    memset( lksel_cannot + 4, 0, LK_GAME_LEN );
    dl_strncpy( (char*) lksel_cannot + 4, game, LK_GAME_LEN );
    memcpy( lksel_cannot + 4 + LK_GAME_LEN, reason, LKSEL_REASON_LEN );
    lksel_have_cannot = true;
    LK_Send( master.fp, LK_GM_GAME_CANNOT, lksel_cannot, LKSEL_CANNOT_LEN );
}

static void  lksel_on_cannot( const lk_event_t * ev )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    lksel_peer_t * s;
    char  game[LK_GAME_LEN], reason[LKSEL_REASON_LEN];
    uint32_t serial;
    int n;

    if( LK_Role() != LK_ROLE_MASTER || ev->len != LKSEL_CANNOT_LEN )  return;
    serial = get32( ev->data );
    lksel_field( game, ev->data + 4, LK_GAME_LEN );
    lksel_field( reason, ev->data + 4 + LK_GAME_LEN, LKSEL_REASON_LEN );
    n = LK_Peers( peers, LK_MAX_PEERS );
    s = lksel_slot( ev->source, peers, n );
    if( ! s )  return;
    // Kept even for an earlier selection: a member re-sends it when this
    // master comes back, which is how an operator session still sees it.
    snprintf( s->note, sizeof(s->note), "GAME SYNC: %s", reason );
    lksel_upper( s->note );
    if( serial == lksel_serial && ! strcasecmp( game, lksel_target ) )
        s->done_serial = lksel_serial;
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s cannot switch to %s: %s\n",
               lksel_peer_name( ev->source ), game, reason );
}

void  LKSEL_On_Event( const lk_event_t * ev )
{
    switch( ev->type )
    {
     case LK_GM_GAME_SELECTED:  lksel_on_selected( ev );  break;
     case LK_GM_GAME_SWITCH:    lksel_on_switch( ev );  break;
     case LK_GM_GAME_CANNOT:    lksel_on_cannot( ev );  break;
    }
}

// ---------------------------------------------------------------------------

// Master: tell every cabinet that has not dealt with the latest pick, and is
// free to switch, to switch now.
static void  lksel_send_switches( void )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    static const byte zero[LK_FP_BYTES];
    byte  msg[LKSEL_SWITCH_LEN];
    tic_t now = I_GetTime();
    int   i, n;

    n = LK_Peers( peers, LK_MAX_PEERS );
    for( i = 0; i < n; i++ )
    {
        lk_peer_info_t * p = &peers[i];
        lksel_peer_t * s;
        if( p->status != LK_PEER_ONLINE || ! memcmp( p->fp, zero, LK_FP_BYTES ) )  continue;
        if( ! p->game[0] )  continue;   // connected, but has not said what it runs yet
        s = lksel_slot( p->fp, peers, n );
        if( ! s || s->done_serial == lksel_serial )  continue;
        if( ! strcasecmp( p->game, lksel_target ) )
        {
            s->done_serial = lksel_serial;
            s->note[0] = 0;
            continue;
        }
        if( ! lksel_can_switch_now( p->state ) )
        {
            if( s->waited_serial != lksel_serial )
                GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s is %s; it switches to %s when it is free\n",
                           p->name[0] ? p->name : p->address, LK_State_Name( p->state ), lksel_target );
            s->waited_serial = lksel_serial;
            continue;
        }
        if( s->sent_serial == lksel_serial && now - s->sent_at < LKSEL_RESEND_TICS )  continue;

        put32( msg, lksel_serial );
        memset( msg + 4, 0, LK_GAME_LEN );
        dl_strncpy( (char*) msg + 4, lksel_target, LK_GAME_LEN );
        if( LK_Send( p->fp, LK_GM_GAME_SWITCH, msg, sizeof(msg) ) )
        {
            if( s->sent_serial != lksel_serial )
                GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: telling %s to switch to %s\n",
                           p->name[0] ? p->name : p->address, lksel_target );
            s->sent_serial = lksel_serial;
            s->sent_at = now;
        }
    }

}

static void  lksel_master_tick( void )
{

    if( ! cv_link_gamesync.EV )
    {
        // Switched off: forget the selection, so switching it back on does not
        // replay an old one.
        lksel_announce = false;
        if( lksel_target[0] )  lksel_new_target( "" );
        return;
    }
    if( lksel_announce )
    {
        lksel_announce = false;
        if( strcasecmp( LK_Game_Id(), lksel_target ) )
        {
            lksel_new_target( LK_Game_Id() );
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s selected here; the other cabinets follow\n", lksel_target );
        }
        lksel_self_done = lksel_serial;
    }
    if( ! lksel_target[0] )  return;

    lksel_send_switches();

    // This cabinet follows a selection made on a member, when it is free to.
    if( lksel_self_done != lksel_serial && strcasecmp( LK_Game_Id(), lksel_target )
        && lksel_can_switch_now( LK_State() ) )
    {
        const char * why = M_Link_Game_Why_Not( lksel_target );
        if( ! why )
        {
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: switching to %s, selected on the link\n", lksel_target );
            // -linkselected: after the restart this cabinet still holds the
            // selection, and tells the members that have not followed yet.
            why = M_Link_Follow_Game( lksel_target, true );   // does not return when it restarts
        }
        lksel_self_done = lksel_serial;
        if( why )
        {
            snprintf( lksel_self_note, sizeof(lksel_self_note), "GAME SYNC: %s", why );
            lksel_upper( lksel_self_note );
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: cannot switch to %s: %s\n", lksel_target, why );
        }
    }
    else if( ! strcasecmp( LK_Game_Id(), lksel_target ) )
        lksel_self_done = lksel_serial;
}

static void  lksel_member_tick( void )
{
    lk_peer_info_t  master;
    boolean online = LK_Sync_Peers( &master, 1 ) >= 1;

    lksel_target[0] = 0;
    if( online && ! lksel_master_online && lksel_have_cannot )
        LK_Send( master.fp, LK_GM_GAME_CANNOT, lksel_cannot, LKSEL_CANNOT_LEN );
    lksel_master_online = online;

    if( lksel_announce && online )
    {
        byte  msg[LK_GAME_LEN];
        memset( msg, 0, sizeof(msg) );
        dl_strncpy( (char*) msg, LK_Game_Id(), LK_GAME_LEN );
        if( LK_Send( master.fp, LK_GM_GAME_SELECTED, msg, sizeof(msg) ) )
        {
            lksel_announce = false;
            lksel_have_cannot = false;
            lksel_self_note[0] = 0;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: told %s this cabinet selected %s\n",
                       master.name, LK_Game_Id() );
        }
    }
}

void  LKSEL_Ticker( void )
{
    if( ! lksel_inited )
    {
        lksel_inited = true;
        // A selection restarted the program: it still has to be told.
        if( M_CheckParm( "-linkselected" ) )
            lksel_announce = true;
        if( M_CheckParm( "-linktest" ) && M_CheckParm( "-linkselectat" ) && M_IsNextParm() )
        {
            lksel_test_secs = atoi( M_GetNextParm() );
            if( M_IsNextParm() )  lksel_test_name = M_GetNextParm();
            lksel_test_start = I_GetTime();
            // The choice restarts the program with the same arguments: only
            // the process that has not made it yet may make it.
            if( lksel_announce )  lksel_test_name = NULL;
        }
    }

    if( lksel_test_name && I_GetTime() - lksel_test_start >= (tic_t) lksel_test_secs * TICRATE )
    {
        const char * name = lksel_test_name;
        lksel_test_name = NULL;
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: selecting %s\n", name );
        M_Link_Test_Select( name );   // does not return when it restarts
    }

    switch( LK_Role() )
    {
     case LK_ROLE_MASTER:  lksel_master_tick();  break;
     case LK_ROLE_MEMBER:  lksel_member_tick();  break;
     default:
        lksel_announce = false;
        lksel_target[0] = 0;
        break;
    }
}
