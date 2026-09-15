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
#include "w_wad.h"     // numwadfiles, for the status line; W_Md5_File
#include "md5.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

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
    // The master's game for a cabinet on its attract screen (lksel_send_defaults)
    boolean   seen;                   // online this tick
    tic_t     online_since;
    char      default_game[LK_GAME_LEN];
    tic_t     default_at;
    boolean   default_cannot;         // it said it cannot run default_game
} lksel_peer_t;

// A cabinet that has just connected may be about to announce a pick of its own.
#define LKSEL_DEFAULT_GRACE   (5*TICRATE)
// Told, and still on its old game this much later: it did not take it.
#define LKSEL_DEFAULT_RESEND  (20*TICRATE)
// Said it cannot run it (not installed, say): ask again this rarely.
#define LKSEL_DEFAULT_CANNOT  (5*60*TICRATE)

static boolean   lksel_inited;
static boolean   lksel_announce;              // a player chose this game here
static char      lksel_target[LK_GAME_LEN];   // master: the latest selection, "" none
static uint32_t  lksel_serial;
static uint32_t  lksel_self_done;             // master: the selection this cabinet dealt with
static char      lksel_self_note[LKSEL_NOTE_LEN];
static lksel_peer_t  lksel_peers[LK_MAX_PEERS];

// Copy Missing Wads: its state is here, its code further down.
enum { LKC_WANT = LKSEL_SYNC_FIRST, LKC_OFFER, LKC_NONE, LKC_GET, LKC_DATA };

#define LKC_NAME_LEN     64
#define LKC_DATA_HDR     (1 + 16 + 4)
#define LKC_CHUNK        (LK_SYNC_DATA_MAX - LKC_DATA_HDR)
#define LKC_WINDOW       12
#define LKC_MAX_SIZE     (192u*1024*1024)   // the largest IWAD is 18 MB
#define LKC_STALL_TICS   (3*TICRATE)
#define LKC_WANT_TICS    (30*TICRATE)   // no offer this long: give up
#define LKC_ASK_TICS     (3*TICRATE)    // ask again this often until then
#define LKC_AWAY_TICS    (60*TICRATE)   // a master gone this long mid-copy: give up
#define LKC_OFFERS       4

extern consvar_t  cv_link_copywads;   // m_menu.c

// master: files it has offered, served by md5
typedef struct
{
    boolean   used;
    byte      md5[16];
    uint32_t  size;
    char      path[MAX_WADPATH];
    char      name[LKC_NAME_LEN];
} lkc_offer_t;
static lkc_offer_t  lkc_offers[LKC_OFFERS];
static int          lkc_next_offer;
static int          lkc_test_corrupt = -1;   // -linktest -linkcorruptwad (int: boolean is an enum, and may be unsigned)

// member: the one copy in progress
enum { LKCM_NONE, LKCM_WANTED, LKCM_COPYING };
static int          lkc_phase = LKCM_NONE;
static char         lkc_game[LK_GAME_LEN];
static byte         lkc_what;
static byte         lkc_md5[16];
static uint32_t     lkc_size, lkc_got, lkc_win_end;
static char         lkc_name[LKC_NAME_LEN];
static char         lkc_dest[MAX_WADPATH], lkc_part[MAX_WADPATH + 8];
static FILE *       lkc_file;
static struct md5_ctx  lkc_ctx;
static tic_t        lkc_last, lkc_started;
static boolean      lkc_paused;
static boolean      lkc_master_back = true;   // false: reconnected mid-copy, offer not seen again yet
static char         lkc_pending[LK_GAME_LEN];   // the pick to follow once copied

static void  lkc_want( const char * game );
static void  lkc_member_tick( void );

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

// The master's choice: the latest pick while one stands, otherwise the game the
// master is running.  A pick made on a member while the master was busy is the
// choice even before the master has followed it.
static const char *  lksel_group_game( void )
{
    return lksel_target[0] ? lksel_target : LK_Game_Id();
}

// Is `bare` (an IWAD with no pack) the group game with its level pack dropped?
static boolean  lksel_drops_pack( const char * group, const char * bare )
{
    size_t n = strlen( bare );
    return n && ! strncasecmp( group, bare, n ) && group[n] == '+';
}

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

// A game with a level pack loaded is ending back to attract, and this cabinet is
// about to restart without the pack (M_Restart_Unload_Pack).  That drops the
// pack from the master's choice when the choice is that pack -- every cabinet
// drops it -- but it is not a pick: when the choice has since become another
// game (picked while this cabinet was playing), the choice stands.  A member
// then takes it on its attract screen; a master goes straight to it, which is
// the game this returns (NULL: restart without the pack as usual).
const char *  LKSEL_Unloading( const char * bare )
{
    static char  instead[LK_GAME_LEN];
    lk_peer_info_t  master;
    byte  msg[LK_GAME_LEN + 1];
    const char * group;

    if( ! M_Link_Game_Id_Valid( bare ) )  return NULL;
    switch( LK_Role() )
    {
     case LK_ROLE_MEMBER:
        if( LK_Sync_Peers( &master, 1 ) < 1 )  return NULL;
        memset( msg, 0, sizeof(msg) );
        dl_strncpy( (char*) msg, bare, LK_GAME_LEN );
        msg[LK_GAME_LEN] = 1;   // only dropping its pack
        if( LK_Send( master.fp, LK_GM_GAME_SELECTED, msg, sizeof(msg) ) )
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: told %s this cabinet dropped its level pack\n", master.name );
        return NULL;
     case LK_ROLE_MASTER:
        if( ! cv_link_gamesync.EV )  return NULL;
        group = lksel_group_game();
        if( lksel_drops_pack( group, bare ) )
        {
            lksel_new_target( bare );
            lksel_self_done = lksel_serial;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s selected here, dropping the level pack; the other cabinets drop it too\n", bare );
            lksel_send_switches();
            return NULL;
        }
        if( ! strcasecmp( group, bare ) )  return NULL;
        dl_strncpy( instead, group, LK_GAME_LEN );
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s was picked meanwhile; going there instead\n", instead );
        return instead;
     default:
        return NULL;
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
    if( lkc_phase != LKCM_NONE )
        GenPrintf( EMSG_errlog, "LINKCOPY phase=%d name=%s got=%u size=%u paused=%d\n",
                   lkc_phase, lkc_name[0] ? lkc_name : "-", (unsigned) lkc_got, (unsigned) lkc_size, lkc_paused );
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
    boolean unload;

    if( LK_Role() != LK_ROLE_MASTER )  return;
    // A byte more: the member only dropped its level pack (LKSEL_Unloading).
    unload = ( ev->len == LK_GAME_LEN + 1 && ev->data[LK_GAME_LEN] == 1 );
    if( ev->len != LK_GAME_LEN && ! unload )  return;
    lksel_field( game, ev->data, LK_GAME_LEN );
    if( ! M_Link_Game_Id_Valid( game ) )  return;

    if( ! cv_link_gamesync.EV )
    {
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s selected %s (Select Game Sync is off)\n",
                   lksel_peer_name( ev->source ), game );
        return;
    }
    if( unload )
    {
        const char * group = lksel_group_game();
        if( ! lksel_drops_pack( group, game ) )
        {
            // Another game was picked while it played: that stands, and the
            // member takes it on its attract screen.
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s dropped its level pack; the link stays on %s\n",
                       lksel_peer_name( ev->source ), group );
            return;
        }
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s dropped the level pack; the other cabinets drop it too\n",
                   lksel_peer_name( ev->source ) );
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
    if( lkc_pending[0] && strcasecmp( lkc_pending, game ) )
        lkc_pending[0] = 0;   // a newer pick replaces one waiting on a copy
    if( ! strcasecmp( game, LK_Game_Id() ) )  return;
    // Busy: the master asks again once this cabinet is free.
    if( ! lksel_can_switch_now( LK_State() ) )  return;
    // Serial 0: not a pick, the master's game for a cabinet left on its attract
    // screen.  Someone in the menus has not left it.
    if( serial == 0 && LK_State() != LK_STATE_IDLE )  return;

    why = M_Link_Game_Why_Not( game );
    if( ! why )
    {
        GenPrintf( EMSG_errlog, serial ? "LINKLOG Cabinet Link: switching to %s, selected on the link\n"
                                       : "LINKLOG Cabinet Link: switching to %s, the master's game\n", game );
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
    // Copy Missing Wads: the master says whether it will.
    lkc_want( game );
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
    if( ! strcasecmp( game, s->default_game ) || ! strcasecmp( game, lksel_group_game() ) )
    {
        dl_strncpy( s->default_game, game, LK_GAME_LEN );
        s->default_at = I_GetTime();
        s->default_cannot = true;
    }
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

// ---------------------------------------------------------------------------
//  Copy Missing Wads
// ---------------------------------------------------------------------------
//
// A member that cannot follow a pick because it lacks the IWAD or the level
// pack asks its master for that one file.  The master finds its own copy by the
// same rules the Select Game page uses -- never by a path it is sent -- and
// offers its name, size and md5 (remembered by W_Md5_File, so offering a wad
// the master has loaded costs it nothing).  The member pulls it a window of
// chunks at a time over the sync channel, as score demos travel, into
// "<file>.part"; checks size and md5; and renames it into place: an IWAD into
// wads/ beside the program (searched early), a pack into legacyhome/levels/.
// It never writes over a file that is there.  Then it follows the pick, if it
// is still the latest one.  A member in a game pauses the copy -- a copy must
// never cost a player a frame.
//
// Sync payloads (little-endian), member <-> master only:
//   WANT   u8 32, u8 what (1 IWAD, 2 pack), game[LK_GAME_LEN]
//   OFFER  u8 33, u8 what, md5[16], u32 size, name[64], game[LK_GAME_LEN]
//   NONE   u8 34, reason[48]
//   GET    u8 35, md5[16], u32 offset, u8 chunks
//   DATA   u8 36, md5[16], u32 offset, bytes

static const char *  lkc_what_word( const char * game, int what )
{
    static char  w[LK_GAME_LEN];
    const char * plus = strchr( game, '+' );
    if( what == 2 && plus )
        snprintf( w, sizeof(w), "LEVEL PACK %s", plus + 1 );
    else
        snprintf( w, sizeof(w), "%.*s", plus ? (int)( plus - game ) : (int) strlen( game ), game );
    lksel_upper( w );
    return w;
}

static boolean  lkc_master_fp( byte * fp )
{
    lk_peer_info_t  master;
    if( LK_Role() != LK_ROLE_MEMBER || LK_Sync_Peers( &master, 1 ) < 1 )  return false;
    memcpy( fp, master.fp, LK_FP_BYTES );
    return true;
}

static void  lkc_send_get( void )
{
    byte  msg[1 + 16 + 4 + 1], fp[LK_FP_BYTES];
    uint32_t chunks = ( lkc_size - lkc_got + LKC_CHUNK - 1 ) / LKC_CHUNK;
    if( chunks > LKC_WINDOW )  chunks = LKC_WINDOW;
    if( ! lkc_master_fp( fp ) )  return;
    msg[0] = LKC_GET;
    memcpy( msg + 1, lkc_md5, 16 );
    put32( msg + 17, lkc_got );
    msg[21] = (byte) chunks;
    if( LK_Sync_Send( fp, msg, sizeof(msg) ) )
        lkc_win_end = lkc_got + chunks * LKC_CHUNK;
    lkc_last = I_GetTime();
}

static void  lkc_abandon( const char * why )
{
    if( lkc_file )  { fclose( lkc_file );  lkc_file = NULL; }
    if( lkc_phase == LKCM_COPYING )  remove( lkc_part );
    if( why )
    {
        snprintf( lksel_self_note, sizeof(lksel_self_note), "GAME SYNC: NO %s - %s",
                  lkc_what_word( lkc_game, lkc_what ), why );
        lksel_upper( lksel_self_note );
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: copy of %s failed: %s\n",
                   lkc_name[0] ? lkc_name : lkc_game, why );
    }
    lkc_phase = LKCM_NONE;
}

// Ask the master for lkc_game's part lkc_what.  Sent again until it answers:
// the master may be restarting -- following that very pick -- and a message
// sent into a closing connection is gone.
static void  lkc_send_want( void )
{
    byte  msg[2 + LK_GAME_LEN], fp[LK_FP_BYTES];
    lkc_last = I_GetTime();
    if( ! lkc_master_fp( fp ) )  return;
    msg[0] = LKC_WANT;
    msg[1] = lkc_what;
    memset( msg + 2, 0, LK_GAME_LEN );
    dl_strncpy( (char*) msg + 2, lkc_game, LK_GAME_LEN );
    LK_Sync_Send( fp, msg, sizeof(msg) );
}

// A member that cannot run game: ask for the first part of it that it lacks.
static void  lkc_want( const char * game )
{
    int  what = M_Link_Missing( game );

    if( ! what )  return;
    if( lkc_phase != LKCM_NONE )
    {
        if( ! strcasecmp( lkc_game, game ) )  return;   // already under way
        lkc_abandon( NULL );                            // a newer pick replaces it
    }
    lkc_phase = LKCM_WANTED;
    lkc_what = what;
    lkc_name[0] = 0;
    dl_strncpy( lkc_game, game, LK_GAME_LEN );
    dl_strncpy( lkc_pending, game, LK_GAME_LEN );
    lkc_started = I_GetTime();
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: asking the master for %s\n", lkc_what_word( game, what ) );
    lkc_send_want();
}

static void  lkc_finish( void )
{
    byte  md5[16];
    int   fail;

    md5_finish_ctx( &lkc_ctx, md5 );
    fail = ( fflush( lkc_file ) != 0 );
#ifndef _WIN32
    if( ! fail )  fail = ( fsync( fileno( lkc_file ) ) != 0 );
#endif
    fail |= ( fclose( lkc_file ) != 0 );
    lkc_file = NULL;
    if( fail )  { lkc_abandon( "COULD NOT WRITE IT" );  return; }
    if( memcmp( md5, lkc_md5, 16 ) )  { lkc_abandon( "IT ARRIVED DAMAGED" );  return; }
    if( access( lkc_dest, F_OK ) == 0 || rename( lkc_part, lkc_dest ) != 0 )
    {
        lkc_abandon( "COULD NOT PUT IT IN PLACE" );
        return;
    }
    lkc_phase = LKCM_NONE;
    lksel_self_note[0] = 0;
    lksel_have_cannot = false;
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: copied %s from the master to %s\n", lkc_name, lkc_dest );
    // A pack may still be missing after its IWAD: ask for that next.
    if( M_Link_Missing( lkc_game ) )
        lkc_want( lkc_game );
}

static void  lkc_master_want( const lk_sync_msg_t * m )
{
    byte  msg[1 + 1 + 16 + 4 + LKC_NAME_LEN + LK_GAME_LEN];
    char  game[LK_GAME_LEN], path[MAX_WADPATH];
    const char * why = NULL, * base, * c;
    struct stat  st;
    lkc_offer_t * o;
    lksel_peer_t * s;
    lk_peer_info_t  peers[LK_MAX_PEERS];
    int  what = m->data[1], i;

    if( LK_Role() != LK_ROLE_MASTER || m->len != 2 + LK_GAME_LEN )  return;
    lksel_field( game, m->data + 2, LK_GAME_LEN );
    if( ! M_Link_Game_Id_Valid( game ) || ( what != 1 && what != 2 ) )  return;

    if( ! cv_link_copywads.EV )
        why = "COPY MISSING WADS IS OFF";
    else if( ! M_Link_Wad_Path( game, what, path ) || stat( path, &st ) != 0 )
        why = "THE MASTER DOES NOT HAVE IT";
    else if( st.st_size <= 0 || (uint64_t) st.st_size > LKC_MAX_SIZE )
        why = "TOO BIG TO COPY";
    if( why )
    {
        byte  none[1 + LKSEL_REASON_LEN];
        none[0] = LKC_NONE;
        memset( none + 1, 0, LKSEL_REASON_LEN );
        dl_strncpy( (char*) none + 1, why, LKSEL_REASON_LEN );
        LK_Sync_Send( m->peer, none, sizeof(none) );
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s asked for %s: %s\n",
                   lksel_peer_name( m->peer ), lkc_what_word( game, what ), why );
        return;
    }

    for( base = c = path; *c; c++ )
        if( *c == '/' || *c == '\\' )  base = c + 1;
    // One offer per file: the md5 names it on the wire.
    for( i = 0; i < LKC_OFFERS; i++ )
        if( lkc_offers[i].used && ! strcmp( lkc_offers[i].path, path ) )  break;
    if( i == LKC_OFFERS )
    {
        i = lkc_next_offer;
        lkc_next_offer = ( lkc_next_offer + 1 ) % LKC_OFFERS;
    }
    o = &lkc_offers[i];
    o->used = true;
    dl_strncpy( o->path, path, sizeof(o->path) );
    dl_strncpy( o->name, base, sizeof(o->name) );
    o->size = (uint32_t) st.st_size;
    W_Md5_File( path, o->md5 );

    msg[0] = LKC_OFFER;
    msg[1] = (byte) what;
    memcpy( msg + 2, o->md5, 16 );
    put32( msg + 18, o->size );
    memset( msg + 22, 0, LKC_NAME_LEN + LK_GAME_LEN );
    dl_strncpy( (char*) msg + 22, o->name, LKC_NAME_LEN );
    dl_strncpy( (char*) msg + 22 + LKC_NAME_LEN, game, LK_GAME_LEN );
    LK_Sync_Send( m->peer, msg, sizeof(msg) );
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: copying %s (%u bytes) to %s\n",
               o->name, (unsigned) o->size, lksel_peer_name( m->peer ) );
    s = lksel_slot( m->peer, peers, LK_Peers( peers, LK_MAX_PEERS ) );
    if( s )
    {
        snprintf( s->note, sizeof(s->note), "GAME SYNC: COPYING %s", o->name );
        lksel_upper( s->note );
    }
}

static void  lkc_master_get( const lk_sync_msg_t * m )
{
    byte  msg[LK_SYNC_DATA_MAX];
    lkc_offer_t * o = NULL;
    uint32_t offset;
    int  i, chunks;
    FILE * f;

    if( LK_Role() != LK_ROLE_MASTER || m->len != 1 + 16 + 4 + 1 )  return;
    for( i = 0; i < LKC_OFFERS; i++ )
        if( lkc_offers[i].used && ! memcmp( lkc_offers[i].md5, m->data + 1, 16 ) )
            { o = &lkc_offers[i];  break; }
    offset = get32( m->data + 17 );
    chunks = m->data[21];
    if( ! o || offset >= o->size || chunks < 1 || chunks > LKC_WINDOW )  return;
    if( lkc_test_corrupt < 0 )
        lkc_test_corrupt = M_CheckParm( "-linktest" ) && M_CheckParm( "-linkcorruptwad" );

    f = fopen( o->path, "rb" );
    if( ! f )  return;
    if( fseek( f, offset, SEEK_SET ) == 0 )
    {
        for( i = 0; i < chunks && offset < o->size; i++ )
        {
            size_t n = fread( msg + LKC_DATA_HDR, 1, LKC_CHUNK, f );
            if( n == 0 )  break;
            msg[0] = LKC_DATA;
            memcpy( msg + 1, o->md5, 16 );
            put32( msg + 17, offset );
            if( lkc_test_corrupt && offset == 0 )
                msg[LKC_DATA_HDR + n/2] ^= 0x5A;   // the test: one byte damaged in transit
            if( ! LK_Sync_Send( m->peer, msg, LKC_DATA_HDR + n ) )
                break;   // queue full: the member asks again
            offset += n;
        }
    }
    fclose( f );

    {
        lk_peer_info_t  peers[LK_MAX_PEERS];
        lksel_peer_t * s = lksel_slot( m->peer, peers, LK_Peers( peers, LK_MAX_PEERS ) );
        if( s )
        {
            if( offset >= o->size )
                snprintf( s->note, sizeof(s->note), "GAME SYNC: COPIED %s", o->name );
            else
                snprintf( s->note, sizeof(s->note), "GAME SYNC: COPYING %s %u%%", o->name,
                          (unsigned)( (uint64_t) offset * 100 / o->size ) );
            lksel_upper( s->note );
        }
    }
}

static void  lkc_member_offer( const lk_sync_msg_t * m )
{
    char  name[LKC_NAME_LEN], game[LK_GAME_LEN];
    uint32_t size;

    if( m->len != 1 + 1 + 16 + 4 + LKC_NAME_LEN + LK_GAME_LEN )  return;
    if( lkc_phase == LKCM_COPYING && ! memcmp( m->data + 2, lkc_md5, 16 ) )
    {
        lkc_master_back = true;
        lkc_send_get();   // the same file offered again: carry on from where it got to
        return;
    }
    if( lkc_phase != LKCM_WANTED )  return;
    lksel_field( name, m->data + 22, LKC_NAME_LEN );
    lksel_field( game, m->data + 22 + LKC_NAME_LEN, LK_GAME_LEN );
    size = get32( m->data + 18 );
    if( strcasecmp( game, lkc_game ) || m->data[1] != lkc_what )  return;
    dl_strncpy( lkc_name, name, sizeof(lkc_name) );
    if( size == 0 || size > LKC_MAX_SIZE )  { lkc_abandon( "TOO BIG TO COPY" );  return; }
    if( ! M_Link_Wad_Dest( game, lkc_what, name, lkc_dest ) )
    {
        lkc_abandon( "NOWHERE TO PUT IT" );
        return;
    }
    snprintf( lkc_part, sizeof(lkc_part), "%s.part", lkc_dest );
    lkc_file = fopen( lkc_part, "wb" );
    if( ! lkc_file )  { lkc_abandon( "COULD NOT WRITE IT" );  return; }
    memcpy( lkc_md5, m->data + 2, 16 );
    lkc_size = size;
    lkc_got = 0;
    md5_init_ctx( &lkc_ctx );
    lkc_phase = LKCM_COPYING;
    lkc_paused = false;
    lkc_master_back = true;
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: copying %s (%u bytes) from the master\n", name, (unsigned) size );
    lkc_send_get();
}

static void  lkc_member_data( const lk_sync_msg_t * m )
{
    uint32_t offset, n;
    if( lkc_phase != LKCM_COPYING || m->len <= LKC_DATA_HDR || memcmp( m->data + 1, lkc_md5, 16 ) )  return;
    offset = get32( m->data + 17 );
    n = m->len - LKC_DATA_HDR;
    if( offset != lkc_got || lkc_got + n > lkc_size )  return;   // a repeat, or out of order
    if( fwrite( m->data + LKC_DATA_HDR, 1, n, lkc_file ) != n )  { lkc_abandon( "COULD NOT WRITE IT" );  return; }
    md5_process_bytes( m->data + LKC_DATA_HDR, n, &lkc_ctx );
    lkc_got += n;
    lkc_last = I_GetTime();
    snprintf( lksel_self_note, sizeof(lksel_self_note), "GAME SYNC: COPYING %s %u%%", lkc_name,
              (unsigned)( (uint64_t) lkc_got * 100 / lkc_size ) );
    lksel_upper( lksel_self_note );
    if( lkc_got == lkc_size )
        lkc_finish();
    else if( lkc_got >= lkc_win_end && ! lkc_paused )
        lkc_send_get();
}

void  LKSEL_On_Sync( const lk_sync_msg_t * m )
{
    switch( m->data[0] )
    {
     case LKC_WANT:   lkc_master_want( m );  break;
     case LKC_GET:    lkc_master_get( m );  break;
     case LKC_OFFER:  lkc_member_offer( m );  break;
     case LKC_DATA:   lkc_member_data( m );  break;
     case LKC_NONE:
        if( lkc_phase == LKCM_WANTED && m->len == 1 + LKSEL_REASON_LEN )
        {
            char  why[LKSEL_REASON_LEN];
            lksel_field( why, m->data + 1, LKSEL_REASON_LEN );
            lkc_pending[0] = 0;
            lkc_abandon( why );
        }
        break;
    }
}

// Member, every tick: keep a copy moving, and follow the pick once it is here.
static void  lkc_member_tick( void )
{
    tic_t now = I_GetTime();
    boolean free_now = lksel_can_switch_now( LK_State() );

    if( lkc_phase == LKCM_WANTED )
    {
        if( now - lkc_started > LKC_WANT_TICS )
        {
            lkc_pending[0] = 0;
            lkc_abandon( "THE MASTER DID NOT ANSWER" );
        }
        else if( lksel_master_online && now - lkc_last > LKC_ASK_TICS )
            lkc_send_want();
    }
    if( lkc_phase == LKCM_COPYING )
    {
        if( ! lksel_master_online )
        {
            // Restarting, perhaps: it offers the file again when it is back.
            if( now - lkc_last > LKC_AWAY_TICS )
            {
                lkc_pending[0] = 0;
                lkc_abandon( "THE MASTER WENT AWAY" );
            }
        }
        else if( ! lkc_master_back && now - lkc_last > LKC_STALL_TICS )
            lkc_send_want();   // back: its offers were forgotten in its restart
        else if( ! free_now )
            lkc_paused = true;   // a game is on: not a byte until it is over
        else if( lkc_paused || now - lkc_last > LKC_STALL_TICS )
        {
            lkc_paused = false;
            lkc_send_get();
        }
    }
    if( lkc_phase == LKCM_NONE && lkc_pending[0] && free_now )
    {
        char  game[LK_GAME_LEN];
        dl_strncpy( game, lkc_pending, LK_GAME_LEN );
        lkc_pending[0] = 0;
        if( ! M_Link_Missing( game ) && strcasecmp( game, LK_Game_Id() ) )
        {
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: switching to %s, selected on the link\n", game );
            M_Link_Follow_Game( game, false );   // does not return when it restarts
        }
    }
}

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
        if( ! s )  continue;
        if( ! strcasecmp( p->game, lksel_target ) )
        {
            // Followed -- perhaps after a copy, whose progress note goes too.
            s->done_serial = lksel_serial;
            s->note[0] = 0;
            continue;
        }
        if( s->done_serial == lksel_serial )  continue;
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

// Master: every cabinet left on its attract screen runs the master's choice.
// A cabinet that played a single level (or anything) while the game was changed
// elsewhere, one that booted into its own Boot Game, one whose pick never
// arrived -- once it is back on attract with nobody at its menus, it takes the
// choice.  A pick still being followed is send_switches' business, not this.
static void  lksel_send_defaults( void )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    static const byte zero[LK_FP_BYTES];
    byte  msg[LKSEL_SWITCH_LEN];
    const char * group = lksel_group_game();
    tic_t now = I_GetTime();
    int   i, n = LK_Peers( peers, LK_MAX_PEERS );

    for( i = 0; i < LK_MAX_PEERS; i++ )
        lksel_peers[i].seen = false;
    for( i = 0; i < n; i++ )
    {
        lk_peer_info_t * p = &peers[i];
        lksel_peer_t * s;
        if( p->status != LK_PEER_ONLINE || ! memcmp( p->fp, zero, LK_FP_BYTES ) || ! p->game[0] )  continue;
        s = lksel_slot( p->fp, peers, n );
        if( ! s )  continue;
        s->seen = true;
        if( ! s->online_since )  s->online_since = now | 1;
        if( p->state != LK_STATE_IDLE || ! strcasecmp( p->game, group ) )  continue;
        if( now - s->online_since < LKSEL_DEFAULT_GRACE )  continue;
        if( lksel_target[0] && s->done_serial != lksel_serial )  continue;   // a pick is on its way to it
        if( ! strcasecmp( s->default_game, group )
            && now - s->default_at < ( s->default_cannot ? LKSEL_DEFAULT_CANNOT : LKSEL_DEFAULT_RESEND ) )
            continue;

        put32( msg, 0 );
        memset( msg + 4, 0, LK_GAME_LEN );
        dl_strncpy( (char*) msg + 4, group, LK_GAME_LEN );
        if( LK_Send( p->fp, LK_GM_GAME_SWITCH, msg, sizeof(msg) ) )
        {
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s is on its attract screen with %s; switching it to %s\n",
                       p->name[0] ? p->name : p->address, p->game, group );
            dl_strncpy( s->default_game, group, LK_GAME_LEN );
            s->default_at = now;
            s->default_cannot = false;
        }
    }
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( ! lksel_peers[i].seen )
            lksel_peers[i].online_since = 0;
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
    if( ! lksel_target[0] )
    {
        lksel_send_defaults();   // no pick standing: the master's own game
        return;
    }

    lksel_send_switches();
    lksel_send_defaults();

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
    if( online && ! lksel_master_online && lkc_phase != LKCM_NONE )
    {
        lkc_master_back = false;
        if( lkc_phase == LKCM_WANTED )
            lkc_send_want();
    }
    lksel_master_online = online;
    lkc_member_tick();

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
