// [Arcade] Cabinet Link, Phase 3: invites and linked games.  See d_linkgame.h
// and docs/arcade/cabinet-link.md.
//
// Every decision here runs on the game thread, fed by the link's event queue.
// The link numbers invites in the order the master sees them (LK_GM_INVITE),
// which is the only thing two cabinets need to agree on to settle who hosts.

#include "doomincl.h"
#include "doomstat.h"
#include "d_link.h"
#include "d_linkgame.h"
#include "d_linksel.h"
#include "d_clisrv.h"
#include "d_main.h"
#include "m_menu.h"
#include "m_argv.h"
#include "i_tcp.h"
#include "g_game.h"
#include "m_misc.h"
#include "command.h"
#include "d_event.h"
#include "g_input.h"

#include <SDL.h>

// Message payloads (little-endian).  The first eight bytes of every one are
// the invite's seq and nonce, so a stale message from an earlier invite is
// recognised and ignored.
//   INVITE  seq, nonce, u8 category, u8 skill, u16 secs, map[9], game[LK_GAME_LEN]
//   CANCEL  seq, nonce
//   STATUS  seq, nonce, u8 joined, u8 all_locked, u16 secs_left
//   START   seq, nonce, u16 host udp port, u8 key id, keys[LK_UDP_KEYS],
//           u16 idle timeout secs, u16 idle warning secs
#define LKG_INVITE_LEN   (8 + 1 + 1 + 2 + 9 + LK_GAME_LEN)
#define LKG_STATUS_LEN   (8 + 1 + 1 + 2)
#define LKG_START_LEN    (8 + 2 + 1 + LK_UDP_KEYS + 2 + 2)
#define LKG_START_IDLE   (8 + 2 + 1 + LK_UDP_KEYS)   // offset of the idle settings

#define LKG_STATUS_MS    1000     // re-send status this often even unchanged
#define LKG_SILENT_MS    6000     // a remote this quiet has left
#define LKG_GRACE_MS     8000     // a remote waits this long past the countdown
#define LKG_NOGAME_MS    30000    // a linked game that never began is over

typedef enum
{
    LKGM_NONE = 0,
    LKGM_HOST,           // our join screen is up and has invited
    LKGM_REMOTE,         // another cabinet's join screen is up on ours
    LKGM_GAME_HOST,      // a linked game we host
    LKGM_GAME_CLIENT     // a linked game we joined
} lkg_mode_e;

typedef struct
{
    boolean   used;
    byte      fp[LK_FP_BYTES];
    byte      joined, locked;
    uint32_t  last_ms;
} lkg_remote_t;

static lkg_mode_e  lkg_mode = LKGM_NONE;

// The invite this cabinet is part of, as host or remote.
static uint32_t    lkg_seq, lkg_nonce;
static byte        lkg_category;
static char        lkg_map[9];
static byte        lkg_skill;

// Host
static lkg_remote_t  lkg_remotes[LK_MAX_PEERS];
static int         lkg_remote_players;
static byte        lkg_sent_joined = 255, lkg_sent_locked = 255;
static uint32_t    lkg_status_ms;

// Remote
static byte        lkg_host_fp[LK_FP_BYTES];
static char        lkg_host_name[LK_NAME_LEN];
static byte        lkg_host_joined;
static uint32_t    lkg_deadline_ms;

// A linked game in progress
static uint32_t    lkg_game_ms;
static boolean     lkg_seen_level;
// The host's idle timeout and warning, from START: a joined game runs on them.
static int         lkg_host_idle_secs, lkg_host_warn_secs;

// -linktest hooks (tools/linktest.sh): honoured only with -linktest.
static int         lkg_test = -1;
static byte        lkg_test_host_cat;
static byte        lkg_test_join;           // 1 -linkautojoin, 2 -linkautopress
static boolean     lkg_test_host_done;
static int         lkg_test_poll_sleep;
static int         lkg_test_press_secs;     // -linkpressafter S: a real fire press S s into an invite
static uint32_t    lkg_test_press_at;
static int         lkg_test_lock_secs;      // -linklockafter S: panel 1 locks in S s into an invite
static uint32_t    lkg_test_lock_at;
static boolean     lkg_test_msgpress;       // -linkmsgpress: fire at any message box, 1 s in
static int         lkg_test_move_ms;
static boolean     lkg_test_chaos;          // -linkchaos: every panel, a new random mix of buttons every 50 ms
static int         lkg_test_cmd_secs;       // -linkcmdafter S "text": console text S s into a linked level
static const char *lkg_test_cmd;
#define LKG_TEST_AT_MAX  4
static int         lkg_test_at_secs[LKG_TEST_AT_MAX];   // -linkcmdat S "text": console text S s after the link starts
static const char *lkg_test_at_cmd[LKG_TEST_AT_MAX];
static int         lkg_test_at_n;
static uint32_t    lkg_test_start_ms;
static int         lkg_test_join_panels = 1;
static boolean     lkg_test_host_in_demo;   // -linkhostindemo: host only while an attract demo plays // -linkjoinpanels N: -linkautojoin locks N panels        // -linkmoveevery MS: a player at the panel, turning     // -linkpollsleep N: N ms between ticker start and events
static int         lkg_test_host_after;     // -linkhostafter N: host after N linked games
static int         lkg_test_end_secs;       // -linkendgame S: a host ends its game after S seconds
static boolean     lkg_test_end_sent;
static int         lkg_games_done;          // linked games this cabinet has seen end

static char        lkg_line[96];

static uint32_t  lkg_now( void )  { return SDL_GetTicks(); }

// Milliseconds from then to now, never "negative".  A stamp taken after now was
// read -- a message handled later in the same tick -- is 0 ms old, not 49 days:
// the unsigned wrap is what made a joining cabinet declare its linked game over
// the instant it began, about one START in five on the Pi.
static uint32_t  lkg_since( uint32_t now, uint32_t then )
{
    return ( now > then ) ? now - then : 0;
}

static void  put32( byte * p, uint32_t v )
{
    p[0] = v & 0xff;  p[1] = (v >> 8) & 0xff;  p[2] = (v >> 16) & 0xff;  p[3] = (v >> 24) & 0xff;
}
static uint32_t  get32( const byte * p )
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void  put16( byte * p, uint16_t v )  { p[0] = v & 0xff;  p[1] = (v >> 8) & 0xff; }
static uint16_t  get16( const byte * p )  { return p[0] | (p[1] << 8); }

static const char *  lkg_cat_word( byte cat )
{
    return ( cat == LKG_CAT_DEATHMATCH ) ? "DEATHMATCH" : "CAMPAIGN";
}

static void  lkg_set_mode( lkg_mode_e m )
{
    lkg_mode = m;
    if( m == LKGM_NONE )
    {
        memset( lkg_remotes, 0, sizeof(lkg_remotes) );
        lkg_sent_joined = lkg_sent_locked = 255;
    }
}

// ---------------------------------------------------------------------------

boolean  LKG_Would_Invite( byte category )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    int i, n;
    if( category == LKG_CAT_NONE || lkg_mode != LKGM_NONE )  return false;
    n = LK_Peers( peers, LK_MAX_PEERS );
    for( i = 0; i < n; i++ )
    {
        if( peers[i].status != LK_PEER_ONLINE )  continue;
        if( strcmp( peers[i].game, LK_Game_Id() ) )  continue;
        if( peers[i].state == LK_STATE_IDLE || peers[i].state == LK_STATE_MENU
            || peers[i].state == LK_STATE_JOINING )
            return true;
    }
    return false;
}

static void  lkg_send_invite( void )
{
    byte p[LKG_INVITE_LEN];
    memset( p, 0, sizeof(p) );
    put32( p, 0 );                 // the master numbers it
    put32( p + 4, lkg_nonce );
    p[8] = lkg_category;
    p[9] = lkg_skill;
    put16( p + 10, 0 );            // filled below
    memcpy( p + 12, lkg_map, 9 );
    dl_strncpy( (char*) p + 21, LK_Game_Id(), LK_GAME_LEN );
    {
        int secs = 0;
        byte joined;
        boolean locked;
        if( M_Join_Counts( &joined, &locked, &secs ) )
            put16( p + 10, secs );
    }
    LK_Send( NULL, LK_GM_INVITE, p, sizeof(p) );
}

void  LKG_Host_Begin( byte category, const char * map, byte skill, int secs )
{
    (void) secs;
    if( lkg_mode != LKGM_NONE || ! LKG_Would_Invite( category ) )  return;
    lkg_set_mode( LKGM_HOST );
    lkg_category = category;
    dl_strncpy( lkg_map, map, sizeof(lkg_map) );
    lkg_skill = skill;
    lkg_seq = 0;
    lkg_nonce = ( (uint32_t) rand() << 16 ) ^ (uint32_t) rand() ^ lkg_now();
    lkg_remote_players = 0;
    lkg_send_invite();
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: invited other cabinets to %s\n",
               lkg_cat_word( category ) );
}

boolean  LKG_Hosting_Remote( void )
{
    int i;
    if( lkg_mode != LKGM_HOST )  return false;
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lkg_remotes[i].used && lkg_remotes[i].joined )  return true;
    return false;
}

// [Arcade] The join screen's countdown ran out: say which other cabinets still
// had someone in who had not locked in.  Lock-in starts the game early only
// when every panel that pressed in, on every cabinet, has locked in, and
// nothing on screen says which one it is waiting for.
void  LKG_Log_Waiting( void )
{
    int i;
    lk_peer_info_t  info;
    if( lkg_mode != LKGM_HOST )  return;
    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        const lkg_remote_t * r = &lkg_remotes[i];
        if( ! r->used || ! r->joined || r->locked )  continue;
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: join screen: the countdown ran out waiting on %s"
                   " (%d in, not all locked in)\n",
                   LK_Peer_Find( r->fp, &info ) ? info.name : "another cabinet", r->joined );
    }
}

boolean  LKG_Remotes_All_Locked( void )
{
    int i;
    if( lkg_mode != LKGM_HOST )  return true;
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lkg_remotes[i].used && lkg_remotes[i].joined && ! lkg_remotes[i].locked )
            return false;
    return true;
}

// Every cabinet in a linked game must time out on the same settings.  The
// idle check already measures everybody's input, so all of them agree on how
// long the game has been idle -- but each compared it with its own idletimeout,
// so a joining cabinet set shorter than the host left a game the host was
// still running, and each screen counted its warning down from its own number.
// The host's are what count, as the host's rules do.  Taken from START rather
// than applied as netvars: a netvar overwrites the setting itself, and nothing
// restores a joining cabinet's own value once the linked game is over.
boolean  LKG_Host_Idle_Settings( int * timeout_secs, int * warn_secs )
{
    if( lkg_mode != LKGM_GAME_CLIENT || ! netgame )  return false;
    *timeout_secs = lkg_host_idle_secs;
    *warn_secs = lkg_host_warn_secs;
    return true;
}

int  LKG_Remote_Players( void )
{
    return ( lkg_mode == LKGM_GAME_HOST ) ? lkg_remote_players : 0;
}

int  LKG_Host_Start( void )
{
    int i, players = 0;
    byte p[LKG_START_LEN];
    byte cancel[8];

    if( lkg_mode != LKGM_HOST )  return 0;

    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        lkg_remote_t * r = &lkg_remotes[i];
        int id;
        if( ! r->used || ! r->joined )  continue;
        if( players == 0 )
            LK_Udp_Host_Begin();
        id = LK_Udp_Host_Add_Client( p + 11 );
        if( ! id )  continue;
        put32( p, lkg_seq );
        put32( p + 4, lkg_nonce );
        put16( p + 8, server_sock_port );
        p[10] = id;
        // The idle timeout as this cabinet applies it: a -devmode host never
        // times out (G_Idle_Timeout_Check), so neither does its game.
        put16( p + LKG_START_IDLE, devmode ? 0 : cv_idletimeout.value );
        put16( p + LKG_START_IDLE + 2, cv_idlewarntime.value );
        LK_Send( r->fp, LK_GM_START, p, sizeof(p) );
        players += r->joined;
    }
    memset( p, 0, sizeof(p) );

    // Everyone else: the invite is over.  A cabinet that got START ignores
    // this; it arrives after START on the same connection.
    put32( cancel, lkg_seq );
    put32( cancel + 4, lkg_nonce );
    LK_Send( NULL, LK_GM_CANCEL, cancel, sizeof(cancel) );

    lkg_remote_players = players;
    if( players )
    {
        lkg_set_mode( LKGM_GAME_HOST );
        lkg_game_ms = lkg_now();
        lkg_seen_level = false;
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: starting a linked game with %d player(s)"
                   " from other cabinets\n", players );
    }
    else
        lkg_set_mode( LKGM_NONE );
    return players;
}

void  LKG_Join_Abandoned( void )
{
    byte p[LKG_STATUS_LEN];
    if( lkg_mode == LKGM_HOST )
    {
        put32( p, lkg_seq );
        put32( p + 4, lkg_nonce );
        LK_Send( NULL, LK_GM_CANCEL, p, 8 );
        lkg_set_mode( LKGM_NONE );
    }
    else if( lkg_mode == LKGM_REMOTE )
    {
        // Tell the host nobody here is coming.
        memset( p, 0, sizeof(p) );
        put32( p, lkg_seq );
        put32( p + 4, lkg_nonce );
        LK_Send( lkg_host_fp, LK_GM_STATUS, p, sizeof(p) );
        lkg_set_mode( LKGM_NONE );
    }
}

const char *  LKG_Join_Line( void )
{
    lkg_line[0] = 0;
    if( lkg_mode == LKGM_HOST )
    {
        int i, len = 0;
        lk_peer_info_t  info;
        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lkg_remote_t * r = &lkg_remotes[i];
            if( ! r->used || ! r->joined )  continue;
            if( ! LK_Peer_Find( r->fp, &info ) )  continue;
            len += snprintf( lkg_line + len, sizeof(lkg_line) - len, "%s%s: %d IN",
                             len ? "  " : "", info.name, r->joined );
            if( len >= (int) sizeof(lkg_line) )  break;
        }
        if( ! lkg_line[0] )
            snprintf( lkg_line, sizeof(lkg_line), "OTHER CABINETS INVITED" );
    }
    else if( lkg_mode == LKGM_REMOTE )
    {
        // No brackets: the menu font draws ( and ) as shapes that read as
        // other letters (seen in an OpenGL capture of this screen).
        snprintf( lkg_line, sizeof(lkg_line), "%s ON %s, %d IN THERE",
                  lkg_cat_word( lkg_category ), lkg_host_name, lkg_host_joined );
    }
    return lkg_line[0] ? lkg_line : NULL;
}

int  LKG_Players_In_Game( void )
{
    int i, n = 0;
    for( i = 0; i < MAXPLAYERS; i++ )
        if( playeringame[i] )  n++;
    return n;
}

const char *  LKG_Mode_Name( void )
{
    static const char * names[] = { "none", "host", "remote", "game-host", "game-client" };
    return names[lkg_mode];
}

// ---------------------------------------------------------------------------
//  -linktest -linkkeys "<tokens>": the Cabinet Link page, pressed by a script
// ---------------------------------------------------------------------------
//
// Space separated, one every 50 ms:
//   open         open the page, as Arcade Options would
//   u d l r f b  panel 1's forward, backward, turn left, turn right, fire, use
//                -- the buttons a cabinet has, through the input queue
//   esc enter bs a keyboard's escape, enter, backspace
//   c=X          a keyboard typing X
//   wN           wait N milliseconds
//   shot         take a screenshot (after the next frame is drawn)
// It starts 3 seconds after the link is set up, and only under -linktest.

static void  lkg_post_key( int key, int ch )
{
    event_t  ev;
    memset( &ev, 0, sizeof(ev) );
    ev.type = ev_keydown;  ev.data1 = key;  ev.data2 = ch;
    D_PostEvent( &ev );
    ev.type = ev_keyup;
    D_PostEvent( &ev );
}

static int  lkg_panel_key( int gc )
{
    return gamecontrol_pl[0][gc][0] ? gamecontrol_pl[0][gc][0] : gamecontrol_pl[0][gc][1];
}

void  LKG_Test_Keys( void )
{
    static const char * script = NULL;
    static int  state = -1;          // -1 unread, 0 none, 1 running, 2 done
    static uint32_t  next_ms;
    char  tok[64];
    int   n;

    if( state == 0 || state == 2 )  return;
    if( state < 0 )
    {
        state = 0;
        if( ! M_CheckParm( "-linktest" ) || ! M_CheckParm( "-linkkeys" ) || ! M_IsNextParm() )
            return;
        script = M_GetNextParm();
        state = 1;
        next_ms = lkg_now() + 3000;
        return;
    }
    if( lkg_now() < next_ms )  return;
    next_ms = lkg_now() + 50;

    while( *script == ' ' )  script++;
    if( ! *script )
    {
        state = 2;
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: keys done\n" );
        return;
    }
    for( n = 0; script[n] && script[n] != ' ' && n < (int) sizeof(tok) - 1; n++ )
        tok[n] = script[n];
    tok[n] = 0;
    script += n;

    if( ! strcmp( tok, "open" ) )        M_Link_Page_Open();
    else if( ! strcmp( tok, "arcade" ) ) M_Link_Arcade_Open();
    else if( ! strcmp( tok, "u" ) )      lkg_post_key( lkg_panel_key( gc_forward ), 0 );
    else if( ! strcmp( tok, "d" ) )      lkg_post_key( lkg_panel_key( gc_backward ), 0 );
    else if( ! strcmp( tok, "l" ) )      lkg_post_key( lkg_panel_key( gc_turnleft ), 0 );
    else if( ! strcmp( tok, "r" ) )      lkg_post_key( lkg_panel_key( gc_turnright ), 0 );
    else if( ! strcmp( tok, "f" ) )      lkg_post_key( lkg_panel_key( gc_fire ), 0 );
    else if( ! strcmp( tok, "b" ) )      lkg_post_key( lkg_panel_key( gc_use ), 0 );
    else if( ! strcmp( tok, "esc" ) )    lkg_post_key( KEY_ESCAPE, 0 );
    else if( ! strcmp( tok, "enter" ) )  lkg_post_key( KEY_ENTER, 0 );
    else if( ! strcmp( tok, "bs" ) )     lkg_post_key( KEY_BACKSPACE, 0 );
    else if( tok[0] == 'c' && tok[1] == '=' && tok[2] )  lkg_post_key( (unsigned char) tok[2], (unsigned char) tok[2] );
    else if( tok[0] == 'w' )             next_ms = lkg_now() + atoi( tok + 1 );
    else if( ! strcmp( tok, "shot" ) )   COM_BufAddText( "screenshot\n" );
    else
        GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: unknown key token %s\n", tok );
}

// ---------------------------------------------------------------------------
//  Messages
// ---------------------------------------------------------------------------

static void  lkg_become_remote( const lk_event_t * ev, boolean convert )
{
    const byte * p = ev->data;
    lk_peer_info_t  info;
    int secs = get16( p + 10 );

    lkg_set_mode( LKGM_REMOTE );
    memcpy( lkg_host_fp, ev->source, LK_FP_BYTES );
    lkg_seq = get32( p );
    lkg_nonce = get32( p + 4 );
    lkg_category = p[8];
    lkg_skill = p[9];
    memcpy( lkg_map, p + 12, 9 );
    lkg_map[8] = 0;
    lkg_host_joined = 0;
    dl_strncpy( lkg_host_name, LK_Peer_Find( ev->source, &info ) ? info.name : "ANOTHER CABINET",
                LK_NAME_LEN );
    if( secs < 3 )  secs = 3;
    lkg_deadline_ms = lkg_now() + secs * 1000;
    lkg_sent_joined = lkg_sent_locked = 255;

    if( convert )
        M_Join_Convert_To_Remote( secs );
    else
        M_Join_Remote_Open( secs );
    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s invited this cabinet to %s%s\n",
               lkg_host_name, lkg_cat_word( lkg_category ), convert ? " (joining their game)" : "" );

    if( lkg_test_join )
    {
        int  panel;
        for( panel = lkg_test_join_panels - 1; panel >= 0; panel-- )
            M_Join_Test_Lock( panel, lkg_test_join == 1 );
    }
    if( lkg_test_press_secs > 0 )
        lkg_test_press_at = lkg_now() + lkg_test_press_secs * 1000;   // -linkautopress: fire, never lock
    if( lkg_test_lock_secs > 0 )
        lkg_test_lock_at = lkg_now() + lkg_test_lock_secs * 1000;
}

static void  lkg_on_invite( const lk_event_t * ev )
{
    const byte * p = ev->data;
    const byte * me = LK_My_Fp();
    char game[LK_GAME_LEN];
    byte cat;
    lk_state_e st = LK_State();

    if( ev->len != LKG_INVITE_LEN || ( me && ! memcmp( ev->source, me, LK_FP_BYTES ) ) )  return;
    cat = p[8];
    if( cat != LKG_CAT_DEATHMATCH && cat != LKG_CAT_CAMPAIGN )  return;
    memcpy( game, p + 21, LK_GAME_LEN );
    game[LK_GAME_LEN-1] = 0;
    if( strcmp( game, LK_Game_Id() ) )  return;   // cannot play that here

    if( lkg_mode == LKGM_NONE )
    {
        // Idle on the attract screen, or someone in the menus: the invite takes
        // over.  A game, the initials page and an operator session are left alone.
        if( st == LK_STATE_IDLE || st == LK_STATE_MENU )
            lkg_become_remote( ev, false );
        return;
    }

    if( lkg_mode == LKGM_HOST && ! LKG_Hosting_Remote() && cat == lkg_category )
    {
        // Two cabinets opened the same game.  The one the master numbered
        // first hosts; the other joins it, keeping whoever already pressed in.
        uint32_t theirs = get32( p );
        if( lkg_seq == 0 || theirs < lkg_seq )
        {
            byte cancel[8];
            put32( cancel, lkg_seq );
            put32( cancel + 4, lkg_nonce );
            LK_Send( NULL, LK_GM_CANCEL, cancel, sizeof(cancel) );
            lkg_become_remote( ev, true );
        }
    }
}

static void  lkg_on_event( const lk_event_t * ev )
{
    const byte * p = ev->data;
    uint32_t nonce;

    if( ev->type == LK_GM_INVITE )
    {
        lkg_on_invite( ev );
        return;
    }
    // [Arcade] Select Game Sync shares this queue; its messages are its own.
    if( ev->type == LK_GM_GAME_SELECTED || ev->type == LK_GM_GAME_SWITCH || ev->type == LK_GM_GAME_CANNOT )
    {
        LKSEL_On_Event( ev );
        return;
    }
    if( ev->len < 8 )  return;
    nonce = get32( p + 4 );

    switch( ev->type )
    {
     case LK_GM_INVITE_ACK:
        if( lkg_mode == LKGM_HOST && nonce == lkg_nonce )
            lkg_seq = get32( p );
        break;

     case LK_GM_CANCEL:
        if( lkg_mode == LKGM_REMOTE && nonce == lkg_nonce
            && ! memcmp( ev->source, lkg_host_fp, LK_FP_BYTES ) )
        {
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s's invite is over\n", lkg_host_name );
            M_Join_Remote_Close();
            lkg_set_mode( LKGM_NONE );
        }
        break;

     case LK_GM_STATUS:
        if( ev->len != LKG_STATUS_LEN )  break;
        if( lkg_mode == LKGM_HOST && nonce == lkg_nonce )
        {
            int i, slot = -1;
            for( i = 0; i < LK_MAX_PEERS; i++ )
            {
                if( lkg_remotes[i].used && ! memcmp( lkg_remotes[i].fp, ev->source, LK_FP_BYTES ) )
                    { slot = i; break; }
                if( slot < 0 && ! lkg_remotes[i].used )  slot = i;
            }
            if( slot >= 0 )
            {
                lkg_remote_t * r = &lkg_remotes[slot];
                // [Arcade] Log each change, so a countdown that ran out can be
                // read back: who was in, and who never locked in.
                if( ! r->used || r->joined != p[8] || r->locked != p[9] )
                {
                    lk_peer_info_t  info;
                    GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: join screen: %s has %d in, %s\n",
                               LK_Peer_Find( ev->source, &info ) ? info.name : "another cabinet",
                               p[8], p[9] ? "all locked in" : p[8] ? "not all locked in" : "nobody locked in" );
                }
                r->used = true;
                memcpy( r->fp, ev->source, LK_FP_BYTES );
                r->joined = p[8];
                r->locked = p[9];
                r->last_ms = lkg_now();
                // A remote locking in may be the last thing the join screen
                // was waiting for.
                M_Join_Recheck_Locked();
            }
        }
        else if( lkg_mode == LKGM_REMOTE && nonce == lkg_nonce
                 && ! memcmp( ev->source, lkg_host_fp, LK_FP_BYTES ) )
        {
            int secs = get16( p + 10 );
            lkg_host_joined = p[8];
            lkg_deadline_ms = lkg_now() + secs * 1000;
            M_Join_Set_Countdown( secs );
        }
        break;

     case LK_GM_START:
        if( ev->len != LKG_START_LEN || lkg_mode != LKGM_REMOTE || nonce != lkg_nonce
            || memcmp( ev->source, lkg_host_fp, LK_FP_BYTES ) )
            break;
        {
            byte joined;
            boolean locked;
            int secs;
            lk_peer_info_t  info;
            M_Join_Counts( &joined, &locked, &secs );
            if( ! joined || ! LK_Peer_Find( lkg_host_fp, &info ) )
            {
                M_Join_Remote_Close();
                lkg_set_mode( LKGM_NONE );
                break;
            }
            LK_Udp_Client_Begin( p[10], p + 11 );
            lkg_set_mode( LKGM_GAME_CLIENT );
            lkg_game_ms = lkg_now();
            lkg_seen_level = false;
            lkg_host_idle_secs = get16( p + LKG_START_IDLE );
            lkg_host_warn_secs = get16( p + LKG_START_IDLE + 2 );
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: joining %s at %s port %d"
                       " (idle timeout %d s, warning %d s)\n",
                       lkg_host_name, info.address, get16( p + 8 ),
                       lkg_host_idle_secs, lkg_host_warn_secs );
            M_Join_Remote_Connect( info.address, get16( p + 8 ) );
        }
        break;
    }
}

// ---------------------------------------------------------------------------

static void  lkg_send_status( const byte * target, byte joined, byte locked, int secs )
{
    byte p[LKG_STATUS_LEN];
    put32( p, lkg_seq );
    put32( p + 4, lkg_nonce );
    p[8] = joined;
    p[9] = locked;
    put16( p + 10, secs < 0 ? 0 : secs );
    LK_Send( target, LK_GM_STATUS, p, sizeof(p) );
    lkg_sent_joined = joined;
    lkg_sent_locked = locked;
    lkg_status_ms = lkg_now();
}

void  LKG_Ticker( void )
{
    lk_event_t  ev;
    uint32_t now;
    byte joined = 0;
    boolean locked = false;
    int secs = 0, i;

    if( lkg_test < 0 )
    {
        lkg_test = M_CheckParm( "-linktest" ) ? 1 : 0;
        if( lkg_test )
        {
            if( M_CheckParm( "-linkautohost" ) && M_IsNextParm() )
            {
                const char * c = M_GetNextParm();
                lkg_test_host_cat = ! strcasecmp( c, "campaign" ) ? LKG_CAT_CAMPAIGN : LKG_CAT_DEATHMATCH;
            }
            lkg_test_join = M_CheckParm( "-linkautojoin" ) ? 1 : M_CheckParm( "-linkautopress" ) ? 2 : 0;
            if( M_CheckParm( "-linkhostafter" ) && M_IsNextParm() )
            {
                lkg_test_host_after = atoi( M_GetNextParm() );
                if( ! lkg_test_host_cat )  lkg_test_host_cat = LKG_CAT_DEATHMATCH;
            }
            if( M_CheckParm( "-linkendgame" ) && M_IsNextParm() )
                lkg_test_end_secs = atoi( M_GetNextParm() );
            if( M_CheckParm( "-linkpollsleep" ) && M_IsNextParm() )
                lkg_test_poll_sleep = atoi( M_GetNextParm() );
            if( M_CheckParm( "-linkpressafter" ) && M_IsNextParm() )
                lkg_test_press_secs = atoi( M_GetNextParm() );
            if( M_CheckParm( "-linklockafter" ) && M_IsNextParm() )
                lkg_test_lock_secs = atoi( M_GetNextParm() );
            lkg_test_msgpress = M_CheckParm( "-linkmsgpress" ) != 0;
            if( M_CheckParm( "-linkjoinpanels" ) && M_IsNextParm() )
                lkg_test_join_panels = atoi( M_GetNextParm() );
            lkg_test_host_in_demo = M_CheckParm( "-linkhostindemo" ) != 0;
            lkg_test_chaos = M_CheckParm( "-linkchaos" ) != 0;
            if( M_CheckParm( "-linkmoveevery" ) && M_IsNextParm() )
                lkg_test_move_ms = atoi( M_GetNextParm() );
            if( M_CheckParm( "-linkcmdafter" ) && M_IsNextParm() )
            {
                lkg_test_cmd_secs = atoi( M_GetNextParm() );
                if( M_IsNextParm() )  lkg_test_cmd = M_GetNextParm();
            }
            {
                // Up to four, each "-linkcmdat S text": a clear and then a
                // restore, say.
                int a;
                for( a = 1; a + 2 < myargc && lkg_test_at_n < LKG_TEST_AT_MAX; a++ )
                {
                    if( strcasecmp( myargv[a], "-linkcmdat" ) )  continue;
                    lkg_test_at_secs[lkg_test_at_n] = atoi( myargv[a+1] );
                    lkg_test_at_cmd[lkg_test_at_n] = myargv[a+2];
                    lkg_test_at_n++;
                }
            }
            lkg_test_start_ms = lkg_now();
        }
    }

    // -linktest -linkcmdat: console text at a wall-clock time, whatever the
    // cabinet is doing -- the shared score cases clear the board from the
    // attract screen and leave a game with it.
    {
        int a;
        for( a = 0; a < lkg_test_at_n; a++ )
        {
            if( ! lkg_test_at_cmd[a] || lkg_now() - lkg_test_start_ms < (uint32_t) lkg_test_at_secs[a] * 1000 )
                continue;
            GenPrintf( EMSG_errlog, "LINKTEST console: %s\n", lkg_test_at_cmd[a] );
            COM_BufAddText( lkg_test_at_cmd[a] );
            COM_BufAddText( "\n" );
            lkg_test_at_cmd[a] = NULL;
        }
    }

    // -linktest -linkcmdafter: console text typed S seconds into a linked
    // level, on wall time -- a tic "wait" in autoexec runs far behind with a
    // dozen engines to a core.
    if( lkg_test_cmd && gamestate == GS_LEVEL && netgame )
    {
        static uint32_t  level_at = 0;
        if( ! level_at )  level_at = lkg_now() | 1;
        if( lkg_now() - level_at >= (uint32_t) lkg_test_cmd_secs * 1000 )
        {
            GenPrintf( EMSG_errlog, "LINKTEST console: %s\n", lkg_test_cmd );
            COM_BufAddText( lkg_test_cmd );
            COM_BufAddText( "\n" );
            lkg_test_cmd = NULL;
        }
    }

    // -linktest -linkpollsleep: the clock moves on while this tick starts, as
    // it does on a slow cabinet, so every stamp the events take is newer.
    if( lkg_test_poll_sleep > 0 )
        SDL_Delay( lkg_test_poll_sleep );

    while( LK_Poll_Event( &ev ) )
        lkg_on_event( &ev );

    // -linktest -linkchaos: people at every joined panel doing something different
    // nearly every tic -- walking, backing, turning either way, strafing,
    // firing -- as real players on analog sticks do.  -linkmoveevery holds the
    // same buttons for 400 ms, so a tic run with its neighbour's ticcmds looked
    // exactly like the right one and a whole class of desync went unseen.
    if( lkg_test_chaos && gamestate == GS_LEVEL && netgame )
    {
        static uint32_t  next_ms = 0, rng = 987654321u;
        static byte      held[MAXSPLITSCREENPLAYERS];
        static const int  gcs[6] = { gc_forward, gc_backward, gc_turnleft, gc_turnright, gc_strafeleft, gc_fire };
        if( lkg_now() >= next_ms )
        {
            int  panel, g;
            next_ms = lkg_now() + 50;
            for( panel = 0; panel < lkg_test_join_panels && panel < MAXSPLITSCREENPLAYERS; panel++ )
            {
                byte  want;
                rng = rng * 1664525u + 1013904223u;
                want = (byte)( rng >> 24 );
                for( g = 0; g < 6; g++ )
                {
                    int key = gamecontrol_pl[panel][gcs[g]][0] ? gamecontrol_pl[panel][gcs[g]][0]
                                                               : gamecontrol_pl[panel][gcs[g]][1];
                    boolean now_down = ( want >> g ) & 1, was_down = ( held[panel] >> g ) & 1;
                    event_t  ev_c;
                    if( ! key || now_down == was_down )  continue;
                    memset( &ev_c, 0, sizeof(ev_c) );
                    ev_c.type = now_down ? ev_keydown : ev_keyup;
                    ev_c.data1 = key;
                    D_PostEvent( &ev_c );
                }
                held[panel] = want & 0x3F;
            }
        }
    }

    // -linktest -linkmoveevery: someone at panel 1 who turns now and then, while
    // a level is up -- held for 200 ms, so the ticcmds built meanwhile see it.
    if( lkg_test_move_ms > 0 && gamestate == GS_LEVEL )
    {
        static uint32_t  down_at = 0;
        static boolean   held = false;
        static const int  gcs[2] = { gc_turnright, gc_forward };
        int  panel, g;
        event_t  ev_turn;
        boolean  press = ! held && lkg_now() - down_at >= (uint32_t) lkg_test_move_ms;
        boolean  release = held && lkg_now() - down_at >= 400;
        if( press || release )
        {
            // Every panel pressed in: turn and walk forward together, so the
            // players really move and a simulation that drifts shows.
            for( panel = 0; panel < lkg_test_join_panels && panel < MAXSPLITSCREENPLAYERS; panel++ )
                for( g = 0; g < 2; g++ )
                {
                    int key = gamecontrol_pl[panel][gcs[g]][0] ? gamecontrol_pl[panel][gcs[g]][0]
                                                               : gamecontrol_pl[panel][gcs[g]][1];
                    if( ! key )  continue;
                    memset( &ev_turn, 0, sizeof(ev_turn) );
                    ev_turn.data1 = key;
                    ev_turn.type = press ? ev_keydown : ev_keyup;
                    D_PostEvent( &ev_turn );
                }
            held = press;
            if( press )  down_at = lkg_now();
        }
    }

    // -linktest -linkmsgpress: a person pressing fire at a message box once they
    // have read it -- through the input queue, like -linkpressafter.
    if( lkg_test_msgpress )
    {
        static const char * seen_text = NULL;
        static uint32_t  seen_ms;
        static boolean   pressed;
        const char * text = M_Message_Text();
        if( text != seen_text )
        {
            seen_text = text;
            seen_ms = lkg_now();
            pressed = false;
        }
        if( text && ! pressed && lkg_now() - seen_ms > 1000 )
        {
            event_t  ev_fire;
            int  key = gamecontrol_pl[0][gc_fire][0] ? gamecontrol_pl[0][gc_fire][0]
                                                     : gamecontrol_pl[0][gc_fire][1];
            char  first[40];
            int  i;
            for( i = 0; i < 39 && text[i] && text[i] != '\n'; i++ )  first[i] = text[i];
            first[i] = 0;
            pressed = true;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: message \"%s\", pressing fire\n", first );
            memset( &ev_fire, 0, sizeof(ev_fire) );
            ev_fire.type = ev_keydown;  ev_fire.data1 = key;
            D_PostEvent( &ev_fire );
            ev_fire.type = ev_keyup;
            D_PostEvent( &ev_fire );
        }
    }
    // Read the clock after the events: they stamp times of their own (START,
    // a remote's STATUS), and none of them may be newer than now.
    now = lkg_now();

    switch( lkg_mode )
    {
     case LKGM_NONE:
        if( lkg_test_host_cat && ! lkg_test_host_done && lkg_games_done >= lkg_test_host_after
            && ( ! lkg_test_host_in_demo || demoplayback )
            && D_Attract_Running() && LKG_Would_Invite( lkg_test_host_cat ) )
        {
            lkg_test_host_done = true;
            M_Link_Test_Host( lkg_test_host_cat );
        }
        break;

     case LKGM_HOST:
        if( ! M_Join_Counts( &joined, &locked, &secs ) )
        {
            // The join screen closed without starting a linked game.
            LKG_Join_Abandoned();
            break;
        }
        for( i = 0; i < LK_MAX_PEERS; i++ )
            if( lkg_remotes[i].used && lkg_since( now, lkg_remotes[i].last_ms ) > LKG_SILENT_MS )
                lkg_remotes[i].joined = 0;
        if( joined != lkg_sent_joined || locked != lkg_sent_locked
            || lkg_since( now, lkg_status_ms ) > LKG_STATUS_MS )
            lkg_send_status( NULL, joined, locked, secs );
        // -linktest: once someone on another cabinet is in, lock this panel in
        // too, so the game starts without waiting out the countdown.
        if( lkg_test_host_cat && LKG_Hosting_Remote() && ! locked )
        {
            int  panel;
            for( panel = lkg_test_join_panels - 1; panel >= 0; panel-- )   // panel 1 last: it may start the game
                M_Join_Test_Lock( panel, true );
        }
        break;

     case LKGM_REMOTE:
        if( ! M_Join_Counts( &joined, &locked, &secs ) )
        {
            LKG_Join_Abandoned();
            break;
        }
        if( joined != lkg_sent_joined || locked != lkg_sent_locked
            || lkg_since( now, lkg_status_ms ) > LKG_STATUS_MS )
            lkg_send_status( lkg_host_fp, joined, locked, secs );
        // -linktest -linkpressafter: panel 1's fire button, as the input
        // code posts it -- through the responders, so a join screen that is
        // no longer up does not get it.  The test hooks above reach into the
        // join screen directly and could never see it closed underneath them.
        // -linktest -linklockafter: panel 1 locks in late, after the host has.
        if( lkg_test_lock_at && now >= lkg_test_lock_at )
        {
            lkg_test_lock_at = 0;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: locking in\n" );
            M_Join_Test_Lock( 0, true );
        }
        if( lkg_test_press_at && now >= lkg_test_press_at )
        {
            event_t  ev;
            int  key = gamecontrol_pl[0][gc_fire][0] ? gamecontrol_pl[0][gc_fire][0]
                                                     : gamecontrol_pl[0][gc_fire][1];
            lkg_test_press_at = 0;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: pressing fire (key %d)\n", key );
            memset( &ev, 0, sizeof(ev) );
            ev.type = ev_keydown;  ev.data1 = key;
            D_PostEvent( &ev );
            ev.type = ev_keyup;
            D_PostEvent( &ev );
        }
        if( now > lkg_deadline_ms + LKG_GRACE_MS )
        {
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: %s did not start the game\n", lkg_host_name );
            M_Join_Remote_Close();
            lkg_set_mode( LKGM_NONE );
        }
        break;

     case LKGM_GAME_HOST:
     case LKGM_GAME_CLIENT:
        // Over when this cabinet is no longer in a network game -- whichever
        // way it left: the game ended, the host went away, or a joining cabinet
        // never got in.  netgame, not D_Attract_Running: the attract marker is
        // only as good as every route into a game remembering to clear it, and
        // the client route did not (D_Link_Connect now does).  A test run caught
        // a joining cabinet dropping its keys 30 seconds into a live game.
        if( gamestate == GS_LEVEL && netgame )
            lkg_seen_level = true;
        // -linktest -linkendgame: the host ends the game, as a time limit would.
        if( lkg_mode == LKGM_GAME_HOST && lkg_test_end_secs && lkg_seen_level && ! lkg_test_end_sent
            && lkg_since( now, lkg_game_ms ) > (uint32_t) lkg_test_end_secs * 1000 )
        {
            lkg_test_end_sent = true;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: test: ending the linked game\n" );
            COM_BufAddText( "exitgame\n" );
        }
        if( ! netgame && ( lkg_seen_level || lkg_since( now, lkg_game_ms ) > LKG_NOGAME_MS ) )
        {
            lkg_games_done++;
            lkg_test_end_sent = false;
            GenPrintf( EMSG_errlog, "LINKLOG Cabinet Link: linked game over (%d so far)\n", lkg_games_done );
            LK_Udp_End();
            D_Link_Restore_Port();
            lkg_remote_players = 0;
            lkg_set_mode( LKGM_NONE );
        }
        break;
    }
}
