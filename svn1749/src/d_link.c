// [Arcade] Cabinet Link: networked cabinets, Phase 1 -- identity, pairing,
// presence.  See d_link.h for the interface and docs/arcade/cabinet-link.md for
// the design, the security model and how it was verified.
//
// Threads.  Everything network-facing runs on one link thread: sockets, TLS,
// the passcode proof, keep-alives.  The game thread owns every piece of game
// state and every file write that can print; the two meet only in lk_shared,
// under lk_mutex.  The link thread never calls CONS_Printf/GenPrintf (the
// console is not thread-safe) -- it queues log lines that LK_Ticker prints.
//
// This is the only file that may test HAVE_LINK (see the Makefile).

#include "doomincl.h"
#include "doomstat.h"
#include "d_link.h"
#include "d_main.h"
#include "command.h"
#include "m_menu.h"
#include "r_state.h"   // rdraw_viewwidth, for the status line
#include "g_game.h"    // players[], for the status line
#include "m_misc.h"
#include "i_system.h"
#include "m_argv.h"
#include "v_video.h"
#include "d_linkgame.h"
#include "d_linkscore.h"
#include "d_linksel.h"    // [Arcade] Select Game Sync

// Draw text trimmed to fit a column, measured against the real font rather than
// counted in characters: hu_font is proportional, and names come off the wire.
static void  lk_draw_fit( int x, int y, int width, int option, const char * text )
{
    char buf[96];
    int  n;
    dl_strncpy( buf, text, sizeof(buf) );
    n = strlen( buf );
    while( n > 0 && V_StringWidth( buf ) > width )
        buf[--n] = 0;
    V_DrawString( x, y, option, buf );
}


// The game id other cabinets compare against: the same IWAD and level pack,
// spelled the way the high score tables spell it (HS_GameId_Mode).
const char * LK_Game_Id( void )
{
    static char  id[LK_GAME_LEN];
    const char * game = ( gamedesc.idstr && gamedesc.idstr[0] ) ? gamedesc.idstr : "game";
    const char * pack = M_LevelPack_LoadedName();
    if( pack )
        snprintf( id, sizeof(id), "%s+%s", game, pack );
    else
        snprintf( id, sizeof(id), "%s", game );
    return id;
}

const char * LK_State_Name( lk_state_e st )
{
    static const char * names[LK_NUM_STATES] =
      { "idle", "menu", "joining", "hosting", "playing", "signing", "devmode" };
    return ( st < LK_NUM_STATES ) ? names[st] : "?";
}

#ifndef HAVE_LINK
// ===========================================================================
//  Built without OpenSSL: every entry point is inert.
// ===========================================================================

boolean     LK_Built( void )        { return false; }
void        LK_Init( void )         { }
void        LK_Ticker( void )       { }
void        LK_Shutdown( void )     { }
lk_role_e   LK_Role( void )         { return LK_ROLE_OFF; }
const char* LK_Name( void )         { return ""; }
const char* LK_Id_Short( void )     { return ""; }
int         LK_Peers( lk_peer_info_t * out, int max )  { (void)out; (void)max; return 0; }
boolean     LK_Send( const byte * t, byte ty, const byte * d, int l )  { (void)t; (void)ty; (void)d; (void)l; return false; }
boolean     LK_Poll_Event( lk_event_t * ev )  { (void)ev; return false; }
const byte* LK_My_Fp( void )       { return NULL; }
boolean     LK_Peer_Find( const byte * fp, lk_peer_info_t * out )  { (void)fp; (void)out; return false; }
lk_state_e  LK_State( void )       { return LK_STATE_IDLE; }
const char* LK_Build( void )       { return ""; }
boolean     LK_Sync_Send( const byte * p, const byte * d, int l )  { (void)p; (void)d; (void)l; return false; }
boolean     LK_Sync_Poll( lk_sync_msg_t * out )  { (void)out; return false; }
int         LK_Sync_Peers( lk_peer_info_t * out, int max )  { (void)out; (void)max; return 0; }
void        LK_Sha256( const byte * d, int l, byte * out )  { (void)d; (void)l; memset( out, 0, 32 ); }
void        LK_Udp_Host_Begin( void )  { }
int         LK_Udp_Host_Add_Client( byte * k )  { (void)k; return 0; }
void        LK_Udp_Client_Begin( byte id, const byte * k )  { (void)id; (void)k; }
void        LK_Udp_End( void )     { }
int         LK_Net_Recv( const byte * in, int len, byte * out, int outsize, uint32_t ip, uint16_t port )
            { (void)in; (void)len; (void)out; (void)outsize; (void)ip; (void)port; return -1; }
int         LK_Net_Send( const byte * in, int len, byte * out, int outsize, uint32_t ip, uint16_t port )
            { (void)in; (void)len; (void)out; (void)outsize; (void)ip; (void)port; return -1; }

void  LK_Drawer( int y, int y_end )
{
    (void) y_end;
    lk_draw_fit( 6, y, 308, 0, "NOT BUILT INTO THIS BINARY" );
    lk_draw_fit( 6, y + 12, 308, V_WHITEMAP, "IT NEEDS OPENSSL AT BUILD TIME:" );
    lk_draw_fit( 6, y + 22, 308, V_WHITEMAP, "INSTALL IT AND RUN TOOLS/BUILD.SH" );
}

static const char lk_not_built[] = "not built into this binary";
void        LK_Setting_Get( lk_setting_e w, char * out, int n )  { (void)w; if( n > 0 ) out[0] = 0; }
const char* LK_Setting_Set( lk_setting_e w, const char * v )  { (void)w; (void)v; return lk_not_built; }
int         LK_Allow_Count( void )  { return 0; }
const char* LK_Allow_Get( int i )  { (void)i; return ""; }
const char* LK_Allow_Add( const char * a )  { (void)a; return lk_not_built; }
const char* LK_Allow_Remove( int i )  { (void)i; return lk_not_built; }
const char* LK_Forget_Pins( void )  { return lk_not_built; }

#else
// ===========================================================================

#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef __WIN32__
// [Arcade] Winsock, in the same place i_tcp.c puts it: doomtype.h has already
// included windows.h.  A Winsock socket is a SOCKET, not a file descriptor, but
// the kernel handle values fit an int and INVALID_SOCKET truncates to -1, so
// the int descriptors below stay; OpenSSL's SSL_set_fd takes an int here too.
# include <winsock2.h>
# include <ws2tcpip.h>
# if _WIN32_WINNT < 0x0600
#  error "Cabinet Link needs _WIN32_WINNT >= 0x0600 (Vista) for WSAPoll and inet_ntop"
# endif
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <poll.h>
# include <fcntl.h>
# include <sys/ioctl.h>   // [Arcade] FIONREAD, for lkt_log_trouble
# include <signal.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

// [Arcade] The socket calls that differ between POSIX and Winsock.  Everything
// else (socket, bind, listen, accept, connect, getaddrinfo, send, recv) has the
// same name and shape on both.
#ifdef __WIN32__
# define lk_closesocket( fd )   closesocket( fd )
# define lk_poll                WSAPoll
# define lk_sock_errno()        WSAGetLastError()
  // A non-blocking connect that has started reports WSAEWOULDBLOCK, not
  // WSAEINPROGRESS (which means a blocking call is already in progress).
# define LK_EINPROGRESS         WSAEWOULDBLOCK
#else
# define lk_closesocket( fd )   close( fd )
# define lk_poll                poll
# define lk_sock_errno()        errno
# define LK_EINPROGRESS         EINPROGRESS
#endif

// A socket error as text.  strerror() knows nothing of Winsock's codes, so on
// Windows it would print "Unknown error" for every one of them.
static const char *  lk_sock_strerror( int err, char * buf, int size )
{
#ifdef __WIN32__
    int n = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, err, 0, buf, size, NULL );
    while( n > 0 && ( buf[n-1] == '\r' || buf[n-1] == '\n' || buf[n-1] == ' ' || buf[n-1] == '.' ) )
        buf[--n] = 0;
    if( n <= 0 )  snprintf( buf, size, "error %d", err );
#else
    snprintf( buf, size, "%s", strerror( err ) );
#endif
    return buf;
}

#define LK_PROTO_VERSION    4   // 2: game id in presence, ROUTE, invites; 3: SYNC (shared scores); 4: Select Game Sync messages
#define LK_FP_LEN           32         // SHA-256 of the public key
#define LK_PASSCODE_MIN     10         // shorter is allowed, but warned about
#define LK_PBKDF2_ITER      60000
#define LK_BUF_SIZE         16384      // per connection, each direction
#define LK_HANDSHAKE_MS     10000      // TCP accept to authenticated
#define LK_PING_MS          5000
#define LK_DEAD_MS          15000
#define LK_LOCKOUT_FAILS    3
#define LK_LOCKOUT_MS       60000
#define LK_REFUSED_SHOW_MS  60000      // how long a refusal stays on the status list
#define LK_BACKOFF_MIN_MS   1000
#define LK_BACKOFF_MAX_MS   60000
// LK_MAX_ALLOW is in d_link.h (the Cabinet Link page lists it)
#define LK_MAX_PINS         64
#define LK_LOG_LINES        32
#define LK_LOG_LEN          160

// Message types and the largest payload each may carry.  A frame naming any
// other type, or longer than its type allows, closes the connection.
enum {
    LK_MSG_AUTH = 1,      // u8 version, 32 proof
    LK_MSG_HELLO,         // u16 proto, u8 role, u8 panels, u8 state, name[16], build[32]
    LK_MSG_PRESENCE,      // u8 state, u8 panels
    LK_MSG_PING,          // empty
    LK_MSG_PEERLIST,      // u8 count, count x LK_PEERLIST_ENTRY
    LK_MSG_ROUTE,         // target[32], source[32], u8 LK_GM_* type, data
    LK_MSG_SYNC,          // shared scores: opaque, member <-> master only (d_linkscore.c)
    LK_NUM_MSG
};
// A sync chunk is only moved into a connection's send buffer while this much
// would still be free after it, so presence, pings and invites always fit.
#define LK_SYNC_RESERVE       8192
#define LK_SYNC_QUEUE         16
#define LK_HELLO_LEN          (2+1+1+1+LK_NAME_LEN+32+LK_GAME_LEN)
#define LK_PRESENCE_LEN       (2+LK_GAME_LEN)
#define LK_PEERLIST_ENTRY     (LK_NAME_LEN + LK_ID_SHORT_LEN + 32 + 1 + 1 + 48 + LK_FP_BYTES + LK_GAME_LEN)
#define LK_ROUTE_HDR          (2*LK_FP_BYTES + 1)
#define LK_EVENTS             32
static const uint32_t  lk_msg_max[LK_NUM_MSG] =
{
    0,
    1 + LK_FP_LEN,
    LK_HELLO_LEN,
    LK_PRESENCE_LEN,
    0,
    1 + LK_MAX_PEERS * LK_PEERLIST_ENTRY,
    LK_ROUTE_HDR + LK_MSG_DATA_MAX,
    LK_SYNC_DATA_MAX
};

// ---------------------------------------------------------------------------
//  Settings and identity (game thread; copied for the link thread at start)
// ---------------------------------------------------------------------------

typedef struct
{
    lk_role_e   role;
    char        name[LK_NAME_LEN];
    char        master[128];
    int         port;
    char        passcode[128];
    char        allow[LK_MAX_ALLOW][128];
    int         num_allow;
} lk_settings_t;

typedef struct
{
    byte        fp[LK_FP_LEN];
    char        name[LK_NAME_LEN];
} lk_pin_t;

static lk_settings_t  lk_set;
static char  lk_dir[MAX_WADPATH];
static char  lk_cfgfile[MAX_WADPATH];
static char  lk_keyfile[MAX_WADPATH];
static char  lk_crtfile[MAX_WADPATH];
static char  lk_pinfile[MAX_WADPATH];

static EVP_PKEY *  lk_key = NULL;
static X509 *      lk_cert = NULL;
static byte        lk_fp[LK_FP_LEN];
static char        lk_id_short[LK_ID_SHORT_LEN] = "";
static boolean     lk_have_identity = false;
static boolean     lk_inited = false;

// ---------------------------------------------------------------------------
//  Shared between the threads, under lk_mutex
// ---------------------------------------------------------------------------

static SDL_mutex *   lk_mutex = NULL;
static SDL_Thread *  lk_thread = NULL;

static struct
{
    int             stop;                 // game thread asks the link thread to exit
    int             done;                 // link thread has exited
    lk_state_e      my_state;
    byte            my_panels;
    lk_peer_info_t  peers[LK_MAX_PEERS];
    int             num_peers;
    char            log[LK_LOG_LINES][LK_LOG_LEN];
    int             log_count;
    lk_pin_t        pins[LK_MAX_PINS];
    int             num_pins;
    int             pins_dirty;
    char            my_game[LK_GAME_LEN];
    // Game messages: in from the link (events) and out to it (outbox).
    // Small rings; a full one drops the newest, which the invite protocol
    // survives because every state it cares about is re-sent.
    lk_event_t      events[LK_EVENTS];
    int             ev_head, ev_count;
    lk_event_t      outbox[LK_EVENTS];    // source field holds the *target*, zero = everyone
    int             out_head, out_count;
    // Shared scores' chunks, in and out.  A full ring drops the newest; the
    // sync asks again.
    lk_sync_msg_t   sync_in[LK_SYNC_QUEUE];
    int             sin_head, sin_count;
    lk_sync_msg_t   sync_out[LK_SYNC_QUEUE];
    int             sout_head, sout_count;
} lk_shared;

// Written by the game thread to wake the link thread out of poll(): [0] is
// polled, [1] is written.  A pipe on POSIX; on Windows a connected pair of
// loopback UDP sockets, because WSAPoll accepts sockets and nothing else.
static int   lk_wake_pipe[2] = { -1, -1 };

static void  lk_lock( void )    { SDL_LockMutex( lk_mutex ); }
static void  lk_unlock( void )  { SDL_UnlockMutex( lk_mutex ); }

// Link thread: queue a line for the game thread to print.
static void  lk_log( const char * fmt, ... )
{
    va_list  ap;
    char     line[LK_LOG_LEN];
    va_start( ap, fmt );
    vsnprintf( line, sizeof(line), fmt, ap );
    va_end( ap );
    lk_lock();
    if( lk_shared.log_count < LK_LOG_LINES )
        dl_strncpy( lk_shared.log[lk_shared.log_count++], line, LK_LOG_LEN );
    lk_unlock();
}

static void  lk_wake( void )
{
    if( lk_wake_pipe[1] >= 0 )
    {
        char c = 1;
#ifdef __WIN32__
        if( send( lk_wake_pipe[1], &c, 1, 0 ) < 0 )  { /* full: already awake */ }
#else
        if( write( lk_wake_pipe[1], &c, 1 ) < 0 )  { /* full: already awake */ }
#endif
    }
}

// Names and build strings come off the wire and are drawn on screen.
static void  lk_sanitize( char * s, int size )
{
    int i;
    s[size-1] = 0;
    for( i = 0; s[i]; i++ )
    {
        if( ! isprint( (unsigned char) s[i] ) )  s[i] = '?';
    }
}

static void  lk_fp_short( const byte * fp, char * out )
{
    snprintf( out, LK_ID_SHORT_LEN, "%02X%02X-%02X%02X", fp[0], fp[1], fp[2], fp[3] );
}

static uint32_t  lk_now( void )  { return SDL_GetTicks(); }

// [Arcade] Milliseconds from then to now, never "negative".  lkt_main reads
// now once per pass and then stamps connections as it goes, so a stamp can be
// a millisecond or two *later* than now.  The plain unsigned subtraction wraps
// to 49 days, and the check it feeds fires at once: a member's connection was
// closed as "timed out before authenticating" straight after connect()
// whenever resolving and connecting crossed a millisecond.  On Linux that is
// almost never; on Windows it was most attempts.  Same fix as lkg_since in
// d_linkgame.c.
static uint32_t  lk_since( uint32_t now, uint32_t then )
{
    return ( now > then ) ? now - then : 0;
}

// tools/linktest.sh --selfcheck builds with LK_SELFCHECK and switches off one
// named check at a time (LK_SELFCHECK=passcode, ...) to prove that the test for
// it can fail.  In every normal build this is the constant false and the checks
// cannot be switched off.
#ifdef LK_SELFCHECK
static boolean  lk_selfcheck_off( const char * name )
{
    const char * e = getenv( "LK_SELFCHECK" );
    return e && strstr( e, name ) != NULL;
}
#else
# define lk_selfcheck_off( name )  false
#endif

// ===========================================================================
//  Settings file: legacyhome/link/link.cfg
// ===========================================================================
//
// Plain "key value" lines.  Kept out of config.cfg on purpose: that file has a
// tracked copy in git, and a passcode or the home network's addresses must
// never reach it.  Mode 0600.
//
//   role master|member|off
//   name LAPTOP
//   master laptop.local
//   port 5030
//   passcode some long phrase
//   allow 192.168.1.68        (repeatable; master only)

static const char * lk_role_names[] = { "off", "master", "member" };

static void  lk_settings_defaults( void )
{
    char host[64];
    memset( &lk_set, 0, sizeof(lk_set) );
    lk_set.role = LK_ROLE_OFF;
    lk_set.port = LK_PORT_DEFAULT;
    if( gethostname( host, sizeof(host) ) == 0 )
    {
        host[sizeof(host)-1] = 0;
        char * dot = strchr( host, '.' );
        if( dot )  *dot = 0;
        dl_strncpy( lk_set.name, host, LK_NAME_LEN );
    }
    if( ! lk_set.name[0] )
        dl_strncpy( lk_set.name, "CABINET", LK_NAME_LEN );
}

static void  lk_settings_load( void )
{
    FILE * f;
    char   line[256];

    lk_settings_defaults();
    f = fopen( lk_cfgfile, "r" );
    if( ! f )  return;
    while( fgets( line, sizeof(line), f ) )
    {
        char * key = line, * val;
        char * nl = strpbrk( line, "\r\n" );
        if( nl )  *nl = 0;
        while( *key == ' ' || *key == '\t' )  key++;
        if( *key == '#' || *key == 0 )  continue;
        val = key;
        while( *val && *val != ' ' && *val != '\t' )  val++;
        if( *val )  *val++ = 0;
        while( *val == ' ' || *val == '\t' )  val++;

        if( ! strcasecmp( key, "role" ) )
        {
            int r;
            for( r = 0; r < 3; r++ )
                if( ! strcasecmp( val, lk_role_names[r] ) )  lk_set.role = r;
        }
        else if( ! strcasecmp( key, "name" ) )
        {
            dl_strncpy( lk_set.name, val, LK_NAME_LEN );
            lk_sanitize( lk_set.name, LK_NAME_LEN );
        }
        else if( ! strcasecmp( key, "master" ) )
            dl_strncpy( lk_set.master, val, sizeof(lk_set.master) );
        else if( ! strcasecmp( key, "port" ) )
        {
            int p = atoi( val );
            if( p > 0 && p < 65536 )  lk_set.port = p;
        }
        else if( ! strcasecmp( key, "passcode" ) )
            dl_strncpy( lk_set.passcode, val, sizeof(lk_set.passcode) );
        else if( ! strcasecmp( key, "allow" ) )
        {
            if( lk_set.num_allow < LK_MAX_ALLOW && *val )
                dl_strncpy( lk_set.allow[lk_set.num_allow++], val, 128 );
        }
        else
            GenPrintf( EMSG_warn, "Cabinet Link: %s: unknown setting \"%s\"\n", lk_cfgfile, key );
    }
    fclose( f );
}

// Open for an atomic write with the file private from its first byte.
static FILE *  lk_private_open( const char * filename )
{
    FILE * fw = M_Atomic_Write_Open( filename );
#ifndef __WIN32__
    // Windows has no mode bits: the file takes the permissions of the folder
    // legacyhome is in, which on a cabinet PC is its only user's anyway.
    if( fw )
        fchmod( fileno( fw ), 0600 );
#endif
    return fw;
}

static boolean  lk_settings_save( void )
{
    int i;
    FILE * fw = lk_private_open( lk_cfgfile );
    if( ! fw )  return false;
    fprintf( fw, "# Doom Legacy Arcade: Cabinet Link settings.  Private: holds the passcode.\n" );
    fprintf( fw, "role %s\n", lk_role_names[lk_set.role] );
    fprintf( fw, "name %s\n", lk_set.name );
    if( lk_set.master[0] )    fprintf( fw, "master %s\n", lk_set.master );
    fprintf( fw, "port %d\n", lk_set.port );
    if( lk_set.passcode[0] )  fprintf( fw, "passcode %s\n", lk_set.passcode );
    for( i = 0; i < lk_set.num_allow; i++ )
        fprintf( fw, "allow %s\n", lk_set.allow[i] );
    return M_Atomic_Write_Close( fw, lk_cfgfile );
}

// ===========================================================================
//  Pins: legacyhome/link/pins.txt, "<64 hex> <name>" per line
// ===========================================================================

static void  lk_pins_load( void )
{
    FILE * f = fopen( lk_pinfile, "r" );
    char line[256];
    lk_shared.num_pins = 0;
    if( ! f )  return;
    while( fgets( line, sizeof(line), f ) && lk_shared.num_pins < LK_MAX_PINS )
    {
        lk_pin_t * pin = &lk_shared.pins[lk_shared.num_pins];
        char hex[LK_FP_LEN*2+1], name[64] = "";
        int i;
        if( sscanf( line, "%64s %63s", hex, name ) < 1 || strlen(hex) != LK_FP_LEN*2 )
            continue;
        for( i = 0; i < LK_FP_LEN; i++ )
        {
            unsigned int b;
            if( sscanf( hex + 2*i, "%2x", &b ) != 1 )  break;
            pin->fp[i] = b;
        }
        if( i != LK_FP_LEN )  continue;
        dl_strncpy( pin->name, name, LK_NAME_LEN );
        lk_shared.num_pins++;
    }
    fclose( f );
}

// Game thread, with lk_mutex held by the caller or no thread running.
static void  lk_pins_save( void )
{
    int i, j;
    FILE * fw = lk_private_open( lk_pinfile );
    if( ! fw )  return;
    fprintf( fw, "# Cabinet Link: cabinets this one has authenticated, by public key.\n" );
    for( i = 0; i < lk_shared.num_pins; i++ )
    {
        for( j = 0; j < LK_FP_LEN; j++ )
            fprintf( fw, "%02x", lk_shared.pins[i].fp[j] );
        fprintf( fw, " %s\n", lk_shared.pins[i].name[0] ? lk_shared.pins[i].name : "-" );
    }
    M_Atomic_Write_Close( fw, lk_pinfile );
}

// Link thread.  Returns the pin index, or -1.
static int  lk_pin_find( const byte * fp )
{
    int i, found = -1;
    lk_lock();
    for( i = 0; i < lk_shared.num_pins; i++ )
        if( ! memcmp( lk_shared.pins[i].fp, fp, LK_FP_LEN ) )  { found = i; break; }
    lk_unlock();
    return found;
}

// Link thread.  A member pins exactly one master: this replaces any other pin.
static void  lk_pin_add( const byte * fp, const char * name, boolean only )
{
    int i;
    lk_lock();
    if( only )
        lk_shared.num_pins = 0;
    for( i = 0; i < lk_shared.num_pins; i++ )
        if( ! memcmp( lk_shared.pins[i].fp, fp, LK_FP_LEN ) )  break;
    if( i == lk_shared.num_pins && i < LK_MAX_PINS )
        lk_shared.num_pins++;
    if( i < LK_MAX_PINS )
    {
        memcpy( lk_shared.pins[i].fp, fp, LK_FP_LEN );
        dl_strncpy( lk_shared.pins[i].name, name, LK_NAME_LEN );
        lk_shared.pins_dirty = 1;
    }
    lk_unlock();
}

// ===========================================================================
//  Identity: legacyhome/link/cabinet.key and cabinet.crt
// ===========================================================================

static boolean  lk_pubkey_fp( EVP_PKEY * key, byte * fp )
{
    unsigned char * der = NULL;
    int len = i2d_PUBKEY( key, &der );
    if( len <= 0 )  return false;
    SHA256( der, len, fp );
    OPENSSL_free( der );
    return true;
}

static boolean  lk_identity_generate( void )
{
    EVP_PKEY * key = EVP_EC_gen( "P-256" );
    X509 * x = NULL;
    X509_NAME * nm;
    uint64_t serial;
    FILE * fw;

    if( ! key )  return false;
    x = X509_new();
    if( ! x )  goto fail;
    X509_set_version( x, 2 );
    RAND_bytes( (unsigned char*) &serial, sizeof(serial) );
    ASN1_INTEGER_set_int64( X509_get_serialNumber( x ), (int64_t)(serial >> 2) );
    // Fixed, wide validity.  Nothing checks it -- identity is the pinned key --
    // and a Pi with no real-time clock boots into 1970.
    ASN1_TIME_set_string( X509_getm_notBefore( x ), "19700101000000Z" );
    ASN1_TIME_set_string( X509_getm_notAfter( x ), "99991231235959Z" );
    X509_set_pubkey( x, key );
    nm = X509_get_subject_name( x );
    X509_NAME_add_entry_by_txt( nm, "CN", MBSTRING_ASC,
                                (const unsigned char*) "Doom Legacy Arcade cabinet", -1, -1, 0 );
    X509_set_issuer_name( x, nm );
    if( ! X509_sign( x, key, EVP_sha256() ) )  goto fail;

    fw = lk_private_open( lk_keyfile );
    if( ! fw )  goto fail;
    if( ! PEM_write_PrivateKey( fw, key, NULL, NULL, 0, NULL, NULL ) )
    {
        fclose( fw );
        goto fail;
    }
    if( ! M_Atomic_Write_Close( fw, lk_keyfile ) )  goto fail;

    fw = lk_private_open( lk_crtfile );
    if( ! fw )  goto fail;
    PEM_write_X509( fw, x );
    if( ! M_Atomic_Write_Close( fw, lk_crtfile ) )  goto fail;

    lk_key = key;
    lk_cert = x;
    GenPrintf( EMSG_info, "Cabinet Link: generated this cabinet's identity\n" );
    return true;

fail:
    if( x )  X509_free( x );
    EVP_PKEY_free( key );
    return false;
}

static boolean  lk_identity_load( void )
{
    FILE * f;
    if( lk_have_identity )  return true;

    f = fopen( lk_keyfile, "r" );
    if( f )
    {
        lk_key = PEM_read_PrivateKey( f, NULL, NULL, NULL );
        fclose( f );
        f = fopen( lk_crtfile, "r" );
        if( f )
        {
            lk_cert = PEM_read_X509( f, NULL, NULL, NULL );
            fclose( f );
        }
        if( ! lk_key || ! lk_cert || X509_check_private_key( lk_cert, lk_key ) != 1 )
        {
            GenPrintf( EMSG_warn, "Cabinet Link: %s or %s is unreadable or does not match;"
                       " not replacing it -- delete both to make a new identity\n",
                       lk_keyfile, lk_crtfile );
            if( lk_key )   { EVP_PKEY_free( lk_key ); lk_key = NULL; }
            if( lk_cert )  { X509_free( lk_cert ); lk_cert = NULL; }
            return false;
        }
    }
    else if( ! lk_identity_generate() )
    {
        GenPrintf( EMSG_warn, "Cabinet Link: could not create an identity in %s\n", lk_dir );
        return false;
    }

    if( ! lk_pubkey_fp( lk_key, lk_fp ) )  return false;
    lk_fp_short( lk_fp, lk_id_short );
    lk_have_identity = true;
    return true;
}

// ===========================================================================
//  Link thread: connections
// ===========================================================================

typedef enum
{
    LKC_EMPTY = 0,
    LKC_TCP,          // member: non-blocking connect in progress
    LKC_TLS,          // TLS handshake
    LKC_AUTH,         // TLS up, passcode proofs being exchanged
    LKC_ONLINE
} lk_phase_e;

typedef struct
{
    lk_phase_e  phase;
    int         fd;
    SSL *       ssl;
    boolean     outbound;           // this end connected (a member to its master)
    struct in_addr  ip;
    char        addr[48];
    uint32_t    started, last_rx, last_tx;
    byte        peer_fp[LK_FP_LEN];
    byte        key[32];            // PBKDF2 of the passcode, salted with both ids
    byte        exporter[32];
    byte        inbuf[LK_BUF_SIZE];
    int         inlen;
    byte        outbuf[LK_BUF_SIZE];
    int         outlen;
    // What the peer has told us.
    char        name[LK_NAME_LEN];
    char        build[32];
    char        game[LK_GAME_LEN];
    byte        role, state, panels;
    boolean     got_hello;
    // [Arcade] For saying what a connection was doing when it went quiet or
    // broke (lkt_log_trouble): a Windows member kept being dropped as
    // "stopped responding" with nothing to show which end had stopped what.
    uint32_t    rx_bytes, tx_bytes, rx_frames, tx_frames, send_stalls;
    byte        last_rx_type, last_tx_type;
} lk_conn_t;

typedef struct
{
    struct in_addr  ip;
    int         fails;
    uint32_t    until;
} lk_lockout_t;

typedef struct
{
    lk_peer_info_t  info;
    uint32_t    when;
} lk_refusal_t;

// Owned by the link thread alone.
static lk_settings_t  lkt_set;        // copy taken at thread start
static SSL_CTX *      lkt_ctx_server = NULL;
static SSL_CTX *      lkt_ctx_client = NULL;
static lk_conn_t      lkt_conn[LK_MAX_PEERS];
static int            lkt_listen = -1;
static lk_lockout_t   lkt_lockout[LK_MAX_PEERS];
static lk_refusal_t   lkt_refused[8];
static struct in_addr lkt_allow_ip[LK_MAX_ALLOW * 4];
static int            lkt_num_allow_ip;
static uint32_t       lkt_allow_resolved;
static uint32_t       lkt_next_attempt;
static uint32_t       lkt_backoff = LK_BACKOFF_MIN_MS;
static char           lkt_member_reason[64];
static lk_peer_info_t lkt_remote[LK_MAX_PEERS];   // member: the master's roster
static int            lkt_num_remote;
static byte           lkt_sent_state = 255, lkt_sent_panels = 255;
static uint32_t       lkt_roster_hash;

static void  lkt_refusal_status( const char * addr, const char * name, const char * reason,
                                 lk_peer_status_e status )
{
    int i, oldest = 0;
    for( i = 0; i < 8; i++ )
    {
        if( ! strcmp( lkt_refused[i].info.address, addr ) )  { oldest = i; break; }
        if( lkt_refused[i].when < lkt_refused[oldest].when )  oldest = i;
    }
    memset( &lkt_refused[oldest], 0, sizeof(lk_refusal_t) );
    lkt_refused[oldest].when = lk_now() | 1;
    lkt_refused[oldest].info.status = status;
    dl_strncpy( lkt_refused[oldest].info.address, addr, 48 );
    dl_strncpy( lkt_refused[oldest].info.name, name ? name : "", LK_NAME_LEN );
    dl_strncpy( lkt_refused[oldest].info.reason, reason, 64 );
}

static void  lkt_refusal( const char * addr, const char * name, const char * reason )
{
    lkt_refusal_status( addr, name, reason, LK_PEER_REFUSED );
}

static lk_lockout_t *  lkt_lockout_find( struct in_addr ip, boolean create )
{
    int i, oldest = 0;
    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        if( lkt_lockout[i].ip.s_addr == ip.s_addr && (lkt_lockout[i].fails || lkt_lockout[i].until) )
            return &lkt_lockout[i];
        if( lkt_lockout[i].until < lkt_lockout[oldest].until )  oldest = i;
    }
    if( ! create )  return NULL;
    memset( &lkt_lockout[oldest], 0, sizeof(lk_lockout_t) );
    lkt_lockout[oldest].ip = ip;
    return &lkt_lockout[oldest];
}

// Close a connection.  reason is shown to the operator; count_fail feeds the
// lockout (master side only, and only for failures before authentication).
static void  lkt_close( lk_conn_t * c, const char * reason, boolean count_fail )
{
    if( c->phase == LKC_EMPTY )  return;

    // A master that refuses a member just closes the connection -- it tells a
    // stranger nothing -- so the member has to say what that means.
    if( reason && c->outbound
        && ( ! strcmp( reason, "disconnected" ) || ! strcmp( reason, "connection lost" )
             || ! strcmp( reason, "TLS handshake failed" ) ) )
    {
        if( c->phase == LKC_AUTH )
            reason = "refused by the master: passcodes differ?";
        else if( c->phase == LKC_TLS )
            reason = "refused by the master: not allowed, or locked out?";
    }

    if( reason )
    {
        lk_log( "Cabinet Link: %s%s%s: %s", c->name[0] ? c->name : "", c->name[0] ? " at " : "",
                c->addr, reason );
        if( c->outbound )
            dl_strncpy( lkt_member_reason, reason, sizeof(lkt_member_reason) );
        else
            // A cabinet that was authenticated and then left was not refused:
            // red REFUSED on the operator page for a member that was simply
            // switched off sends someone hunting for a security problem.
            lkt_refusal_status( c->addr, c->name, reason,
                                c->phase == LKC_ONLINE ? LK_PEER_OFFLINE : LK_PEER_REFUSED );
    }
    if( count_fail && ! c->outbound )
    {
        lk_lockout_t * lo = lkt_lockout_find( c->ip, true );
        if( ++lo->fails >= LK_LOCKOUT_FAILS )
        {
            lo->fails = 0;
            lo->until = lk_now() + LK_LOCKOUT_MS;
            lk_log( "Cabinet Link: %s locked out for %d seconds after %d failures",
                    c->addr, LK_LOCKOUT_MS / 1000, LK_LOCKOUT_FAILS );
        }
    }
    if( c->ssl )  SSL_free( c->ssl );
    if( c->fd >= 0 )  lk_closesocket( c->fd );
    OPENSSL_cleanse( c->key, sizeof(c->key) );
    memset( c, 0, sizeof(*c) );
    c->fd = -1;
}

static void  lkt_queue( lk_conn_t * c, byte type, const byte * payload, uint32_t len )
{
    if( c->phase == LKC_EMPTY )  return;
    if( c->outlen + 5 + (int)len > LK_BUF_SIZE )
    {
        lkt_close( c, "not reading (send buffer full)", false );
        return;
    }
    c->outbuf[c->outlen]   = len & 0xff;
    c->outbuf[c->outlen+1] = (len >> 8) & 0xff;
    c->outbuf[c->outlen+2] = (len >> 16) & 0xff;
    c->outbuf[c->outlen+3] = (len >> 24) & 0xff;
    c->outbuf[c->outlen+4] = type;
    if( len )  memcpy( c->outbuf + c->outlen + 5, payload, len );
    c->outlen += 5 + len;
    c->tx_frames++;
    c->last_tx_type = type;
}

// [Arcade] What a connection was doing when it went quiet or broke, logged
// just before it is closed.  A Windows member kept being dropped by its master
// as "stopped responding", and neither end said anything that could tell
// "the other end stopped sending" from "packets are being lost" from "this end
// stopped reading".  The bytes still unread in this end's own socket buffer
// answer the last; on Linux the kernel's retransmit counters answer the second.
static const char *  lk_msg_name( byte type )
{
    static const char * names[LK_NUM_MSG] =
        { "-", "AUTH", "HELLO", "PRESENCE", "PING", "PEERLIST", "ROUTE", "SYNC" };
    return type < LK_NUM_MSG ? names[type] : "?";
}

static void  lkt_log_trouble( lk_conn_t * c, const char * what, int ssl_err, int sock_err )
{
    uint32_t  now = lk_now();
    unsigned long  unread = 0;
#ifdef __WIN32__
    u_long  avail = 0;
    if( ioctlsocket( c->fd, FIONREAD, &avail ) == 0 )  unread = avail;
#else
    int  avail = 0;
    if( ioctl( c->fd, FIONREAD, &avail ) == 0 && avail > 0 )  unread = avail;
#endif
    lk_log( "Cabinet Link: %s: %s -- heard %u ms ago, sent %u ms ago; ssl err %d, socket err %d",
            c->name[0] ? c->name : c->addr, what,
            lk_since( now, c->last_rx ), lk_since( now, c->last_tx ), ssl_err, sock_err );
    lk_log( "Cabinet Link: %s: in %u bytes/%u msgs (last %s), out %u bytes/%u msgs (last %s),",
            c->name[0] ? c->name : c->addr, c->rx_bytes, c->rx_frames, lk_msg_name( c->last_rx_type ),
            c->tx_bytes, c->tx_frames, lk_msg_name( c->last_tx_type ) );
    lk_log( "Cabinet Link: %s: %d bytes waiting to send, %u send stalls, %lu bytes unread in socket, %d in TLS",
            c->name[0] ? c->name : c->addr, c->outlen, c->send_stalls, unread,
            c->ssl ? SSL_pending( c->ssl ) : 0 );
#ifdef __linux__
    {
        struct tcp_info  ti;
        socklen_t  tl = sizeof(ti);
        memset( &ti, 0, sizeof(ti) );
        if( getsockopt( c->fd, IPPROTO_TCP, TCP_INFO, &ti, &tl ) == 0 )
            lk_log( "Cabinet Link: %s: tcp unacked %u, retransmitting %u, total retrans %u, rtt %u ms,"
                    " data in %u ms ago, out %u ms ago",
                    c->name[0] ? c->name : c->addr, ti.tcpi_unacked, ti.tcpi_retransmits,
                    ti.tcpi_total_retrans, ti.tcpi_rtt / 1000, ti.tcpi_last_data_recv,
                    ti.tcpi_last_data_sent );
    }
#endif
}

static void  lkt_flush( lk_conn_t * c )
{
    while( c->outlen > 0 && c->ssl )
    {
        int n = SSL_write( c->ssl, c->outbuf, c->outlen );
        if( n <= 0 )
        {
            int err = SSL_get_error( c->ssl, n );
            if( err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ )
            {
                c->send_stalls++;
                return;
            }
            lkt_log_trouble( c, "send failed", err, lk_sock_errno() );
            lkt_close( c, "connection lost", false );
            return;
        }
        memmove( c->outbuf, c->outbuf + n, c->outlen - n );
        c->outlen -= n;
        c->tx_bytes += n;
        c->last_tx = lk_now();
    }
}

// The proof this end sends: HMAC(key, exporter || role || own id || peer id).
// Binding it to the TLS exporter is what stops a proof captured in one session
// being replayed in another, or relayed by something sitting in the middle.
static void  lkt_proof( lk_conn_t * c, boolean from_member, byte * out )
{
    byte msg[32 + 6 + 2*LK_FP_LEN];
    // An outbound connection is a member reaching its master.  The ids always
    // go member first, so both ends build the same message for a direction.
    const byte * member_fp = c->outbound ? lk_fp : c->peer_fp;
    const byte * master_fp = c->outbound ? c->peer_fp : lk_fp;
    unsigned int outlen = 32;
    if( lk_selfcheck_off( "exporter" ) )
        memset( msg, 0, 32 );
    else
        memcpy( msg, c->exporter, 32 );
    memcpy( msg + 32, from_member ? "member" : "master", 6 );
    memcpy( msg + 38, member_fp, LK_FP_LEN );
    memcpy( msg + 38 + LK_FP_LEN, master_fp, LK_FP_LEN );
    HMAC( EVP_sha256(), c->key, sizeof(c->key), msg, sizeof(msg), out, &outlen );
}

static void  lkt_send_auth( lk_conn_t * c )
{
    byte payload[1 + LK_FP_LEN];
    payload[0] = LK_PROTO_VERSION;
    // A member's connection is outbound; it proves as member.  The master proves as master.
    lkt_proof( c, c->outbound, payload + 1 );
    lkt_queue( c, LK_MSG_AUTH, payload, sizeof(payload) );
}

static const char *  lkt_build_short( void )
{
    const char * sp = strrchr( VERSION_BANNER, ' ' );
    return sp ? sp + 1 : VERSION_BANNER;
}

static char  lkt_my_game[LK_GAME_LEN];   // link thread's copy, taken each pass

static void  lkt_send_hello( lk_conn_t * c, lk_state_e st, byte panels )
{
    byte p[LK_HELLO_LEN];
    memset( p, 0, sizeof(p) );
    p[0] = LK_PROTO_VERSION & 0xff;
    p[1] = (LK_PROTO_VERSION >> 8) & 0xff;
    p[2] = lkt_set.role;
    p[3] = panels;
    p[4] = st;
    dl_strncpy( (char*) p + 5, lkt_set.name, LK_NAME_LEN );
    dl_strncpy( (char*) p + 5 + LK_NAME_LEN, lkt_build_short(), 32 );
    dl_strncpy( (char*) p + 5 + LK_NAME_LEN + 32, lkt_my_game, LK_GAME_LEN );
    lkt_queue( c, LK_MSG_HELLO, p, sizeof(p) );
}

// Called once TLS is up: identify the peer and derive the proof key.
static boolean  lkt_tls_established( lk_conn_t * c )
{
    X509 * peer = SSL_get1_peer_certificate( c->ssl );
    byte salt[16 + 2*LK_FP_LEN];
    const byte * lo, * hi;

    if( ! peer )
    {
        lkt_close( c, "no certificate", ! c->outbound );
        return false;
    }
    if( ! lk_pubkey_fp( X509_get0_pubkey( peer ), c->peer_fp ) )
    {
        X509_free( peer );
        lkt_close( c, "unreadable certificate", ! c->outbound );
        return false;
    }
    X509_free( peer );

    if( ! memcmp( c->peer_fp, lk_fp, LK_FP_LEN ) )
    {
        lkt_close( c, "that is this cabinet's own identity", ! c->outbound );
        return false;
    }

    // A member checks its master against the pin before sending anything.
    if( c->outbound )
    {
        int pinned;
        lk_lock();
        pinned = lk_shared.num_pins;
        lk_unlock();
        if( pinned && lk_pin_find( c->peer_fp ) < 0 && ! lk_selfcheck_off( "pin" ) )
        {
            char s[LK_ID_SHORT_LEN];
            char why[64];
            lk_fp_short( c->peer_fp, s );
            snprintf( why, sizeof(why), "MASTER IDENTITY CHANGED (now %s)", s );
            lkt_close( c, why, false );
            lkt_backoff = LK_BACKOFF_MAX_MS;
            return false;
        }
    }

    if( SSL_export_keying_material( c->ssl, c->exporter, sizeof(c->exporter),
                                    "dla-link-auth", 13, NULL, 0, 0 ) != 1 )
    {
        lkt_close( c, "TLS exporter failed", false );
        return false;
    }

    // Salt: protocol label and both identities, lower one first, so both ends
    // derive the same key without agreeing who is who.
    if( memcmp( lk_fp, c->peer_fp, LK_FP_LEN ) < 0 )  { lo = lk_fp; hi = c->peer_fp; }
    else  { lo = c->peer_fp; hi = lk_fp; }
    memcpy( salt, "dla-link-v1-salt", 16 );
    memcpy( salt + 16, lo, LK_FP_LEN );
    memcpy( salt + 16 + LK_FP_LEN, hi, LK_FP_LEN );
    if( ! PKCS5_PBKDF2_HMAC( lkt_set.passcode, strlen( lkt_set.passcode ), salt, sizeof(salt),
                             LK_PBKDF2_ITER, EVP_sha256(), sizeof(c->key), c->key ) )
    {
        lkt_close( c, "key derivation failed", false );
        return false;
    }

    c->phase = LKC_AUTH;
    // The member proves first.  The master only answers a correct proof, so a
    // stranger guessing at the master takes away nothing to crack offline.
    if( c->outbound )
        lkt_send_auth( c );
    return true;
}

// ---------------------------------------------------------------------------
//  Game messages: routing (link thread)
// ---------------------------------------------------------------------------

static const byte  lk_zero_fp[LK_FP_BYTES];

// Hand a message to the game thread.
static void  lkt_push_event( const byte * source, byte type, const byte * data, uint32_t len )
{
    lk_event_t * ev;
    if( len > LK_MSG_DATA_MAX )  return;
    lk_lock();
    if( lk_shared.ev_count < LK_EVENTS )
    {
        ev = &lk_shared.events[(lk_shared.ev_head + lk_shared.ev_count) % LK_EVENTS];
        memcpy( ev->source, source, LK_FP_BYTES );
        ev->type = type;
        ev->len = len;
        if( len )  memcpy( ev->data, data, len );
        lk_shared.ev_count++;
    }
    lk_unlock();
}

static void  lkt_send_route( lk_conn_t * c, const byte * target, const byte * source,
                             byte type, const byte * data, uint32_t len )
{
    byte f[LK_ROUTE_HDR + LK_MSG_DATA_MAX];
    if( c->phase != LKC_ONLINE || len > LK_MSG_DATA_MAX )  return;
    memcpy( f, target, LK_FP_BYTES );
    memcpy( f + LK_FP_BYTES, source, LK_FP_BYTES );
    f[2*LK_FP_BYTES] = type;
    if( len )  memcpy( f + LK_ROUTE_HDR, data, len );
    lkt_queue( c, LK_MSG_ROUTE, f, LK_ROUTE_HDR + len );
}

static uint32_t  lkt_invite_seq = 0;

// Master only: deliver a message to its target, or to everyone but its
// source.  An INVITE is numbered here, in the order the master sees them, and
// its sender is told the number -- see LK_GM_INVITE.
static void  lkt_route( const byte * target, const byte * source, byte type,
                        const byte * data_in, uint32_t len )
{
    byte data[LK_MSG_DATA_MAX];
    int i;
    boolean from_self = ! memcmp( source, lk_fp, LK_FP_BYTES );

    if( len > LK_MSG_DATA_MAX )  return;
    if( len )  memcpy( data, data_in, len );

    if( type == LK_GM_INVITE && len >= 8 )
    {
        uint32_t seq = ++lkt_invite_seq;
        data[0] = seq & 0xff;  data[1] = (seq >> 8) & 0xff;
        data[2] = (seq >> 16) & 0xff;  data[3] = (seq >> 24) & 0xff;
    }

    if( ! memcmp( target, lk_zero_fp, LK_FP_BYTES ) )
    {
        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase != LKC_ONLINE || ! memcmp( c->peer_fp, source, LK_FP_BYTES ) )  continue;
            lkt_send_route( c, lk_zero_fp, source, type, data, len );
        }
        if( ! from_self )
            lkt_push_event( source, type, data, len );
    }
    else if( ! memcmp( target, lk_fp, LK_FP_BYTES ) )
        lkt_push_event( source, type, data, len );
    else
    {
        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase == LKC_ONLINE && ! memcmp( c->peer_fp, target, LK_FP_BYTES ) )
                lkt_send_route( c, target, source, type, data, len );
        }
    }

    if( type == LK_GM_INVITE && len >= 8 )
    {
        // Tell the sender its number: seq and its own nonce.
        if( from_self )
            lkt_push_event( lk_fp, LK_GM_INVITE_ACK, data, 8 );
        else
        {
            for( i = 0; i < LK_MAX_PEERS; i++ )
            {
                lk_conn_t * c = &lkt_conn[i];
                if( c->phase == LKC_ONLINE && ! memcmp( c->peer_fp, source, LK_FP_BYTES ) )
                    lkt_send_route( c, source, lk_fp, LK_GM_INVITE_ACK, data, 8 );
            }
        }
    }
}

// Shared scores' chunks the game thread queued.  Each goes only once its
// connection has room for it with LK_SYNC_RESERVE to spare, so a burst of them
// can never fill the send buffer (which closes the connection) or crowd out a
// ping.  One that cannot go yet stays at the head of the queue; one for a
// cabinet no longer online is dropped, and the sync asks again later.
static void  lkt_drain_sync( void )
{
    for( ;; )
    {
        lk_sync_msg_t  m;
        lk_conn_t *    to = NULL;
        int i;

        lk_lock();
        if( lk_shared.sout_count == 0 )  { lk_unlock();  return; }
        m = lk_shared.sync_out[lk_shared.sout_head];
        lk_unlock();

        for( i = 0; i < LK_MAX_PEERS; i++ )
            if( lkt_conn[i].phase == LKC_ONLINE && ! memcmp( lkt_conn[i].peer_fp, m.peer, LK_FP_BYTES ) )
                { to = &lkt_conn[i];  break; }
        if( to && to->outlen + 5 + m.len > LK_BUF_SIZE - LK_SYNC_RESERVE )
            return;   // not yet: wait for this connection to flush

        if( to )
            lkt_queue( to, LK_MSG_SYNC, m.data, m.len );
        lk_lock();
        lk_shared.sout_head = (lk_shared.sout_head + 1) % LK_SYNC_QUEUE;
        lk_shared.sout_count--;
        lk_unlock();
    }
}

// What the game thread asked to send.
static void  lkt_drain_outbox( void )
{
    lk_event_t  batch[LK_EVENTS];
    int i, n;
    lk_lock();
    n = lk_shared.out_count;
    for( i = 0; i < n; i++ )
        batch[i] = lk_shared.outbox[(lk_shared.out_head + i) % LK_EVENTS];
    lk_shared.out_head = (lk_shared.out_head + n) % LK_EVENTS;
    lk_shared.out_count = 0;
    lk_unlock();

    for( i = 0; i < n; i++ )
    {
        lk_event_t * m = &batch[i];   // m->source holds the target
        if( lkt_set.role == LK_ROLE_MASTER )
            lkt_route( m->source, lk_fp, m->type, m->data, m->len );
        else if( lkt_conn[0].phase == LKC_ONLINE )
            lkt_send_route( &lkt_conn[0], m->source, lk_fp, m->type, m->data, m->len );
    }
}

static void  lkt_handle_frame( lk_conn_t * c, byte type, const byte * p, uint32_t len,
                               lk_state_e my_state, byte my_panels )
{
    if( c->phase == LKC_AUTH )
    {
        byte expect[LK_FP_LEN];
        if( type != LK_MSG_AUTH || len != 1 + LK_FP_LEN || p[0] != LK_PROTO_VERSION )
        {
            lkt_close( c, "protocol error before authentication", ! c->outbound );
            return;
        }
        // The peer's proof: a member's peer is the master and vice versa.
        lkt_proof( c, ! c->outbound, expect );
        if( CRYPTO_memcmp( expect, p + 1, LK_FP_LEN ) != 0
            && ! lk_selfcheck_off( c->outbound ? "masterproof" : "passcode" ) )
        {
            lkt_close( c, c->outbound ? "master did not prove the passcode (passcodes differ?)"
                                      : "wrong passcode", ! c->outbound );
            return;
        }
        if( ! c->outbound )
            lkt_send_auth( c );   // the master proves back only now

        c->phase = LKC_ONLINE;
        lkt_backoff = LK_BACKOFF_MIN_MS;
        lkt_send_hello( c, my_state, my_panels );
        {
            char s[LK_ID_SHORT_LEN];
            lk_fp_short( c->peer_fp, s );
            lk_log( "Cabinet Link: %s authenticated (%s)", c->addr, s );
        }
        if( c->outbound )
            lkt_member_reason[0] = 0;
        lkt_roster_hash = 0;   // master: send the roster again
        return;
    }

    switch( type )
    {
     case LK_MSG_HELLO:
        if( len != LK_HELLO_LEN )  break;
        c->role = p[2];
        c->panels = p[3];
        c->state = ( p[4] < LK_NUM_STATES ) ? p[4] : LK_STATE_IDLE;
        memcpy( c->name, p + 5, LK_NAME_LEN );
        lk_sanitize( c->name, LK_NAME_LEN );
        memcpy( c->build, p + 5 + LK_NAME_LEN, 32 );
        lk_sanitize( c->build, 32 );
        memcpy( c->game, p + 5 + LK_NAME_LEN + 32, LK_GAME_LEN );
        lk_sanitize( c->game, LK_GAME_LEN );
        if( ! c->got_hello )
        {
            c->got_hello = true;
            lk_pin_add( c->peer_fp, c->name, c->outbound );
        }
        lkt_roster_hash = 0;
        return;
     case LK_MSG_PRESENCE:
        if( len != LK_PRESENCE_LEN )  break;
        c->state = ( p[0] < LK_NUM_STATES ) ? p[0] : LK_STATE_IDLE;
        c->panels = p[1];
        memcpy( c->game, p + 2, LK_GAME_LEN );
        lk_sanitize( c->game, LK_GAME_LEN );
        lkt_roster_hash = 0;
        return;
     case LK_MSG_ROUTE:
        if( len < LK_ROUTE_HDR || p[2*LK_FP_BYTES] == 0 || p[2*LK_FP_BYTES] >= LK_GM_NUM )  break;
        if( c->outbound )
        {
            // From the master, which vouches for the source.
            lkt_push_event( p + LK_FP_BYTES, p[2*LK_FP_BYTES], p + LK_ROUTE_HDR, len - LK_ROUTE_HDR );
        }
        else
        {
            // From a member: whatever source it wrote is replaced by who it is.
            lkt_route( p, c->peer_fp, p[2*LK_FP_BYTES], p + LK_ROUTE_HDR, len - LK_ROUTE_HDR );
        }
        return;
     case LK_MSG_SYNC:
        // Shared scores.  Every connection is a member and its master, so the
        // peer is always one this cabinet syncs with; nothing is relayed.
        lk_lock();
        if( lk_shared.sin_count < LK_SYNC_QUEUE )
        {
            lk_sync_msg_t * m = &lk_shared.sync_in[(lk_shared.sin_head + lk_shared.sin_count) % LK_SYNC_QUEUE];
            memcpy( m->peer, c->peer_fp, LK_FP_BYTES );
            m->len = len;
            if( len )  memcpy( m->data, p, len );
            lk_shared.sin_count++;
        }
        lk_unlock();
        return;
     case LK_MSG_PING:
        return;
     case LK_MSG_PEERLIST:
        if( ! c->outbound || len < 1 || len != 1 + (uint32_t)p[0] * LK_PEERLIST_ENTRY
            || p[0] > LK_MAX_PEERS )
            break;
        {
            int i, n = p[0];
            const byte * e = p + 1;
            lkt_num_remote = 0;
            for( i = 0; i < n; i++, e += LK_PEERLIST_ENTRY )
            {
                lk_peer_info_t * r = &lkt_remote[lkt_num_remote];
                memset( r, 0, sizeof(*r) );
                r->status = LK_PEER_ONLINE;
                memcpy( r->name, e, LK_NAME_LEN );
                lk_sanitize( r->name, LK_NAME_LEN );
                memcpy( r->id_short, e + LK_NAME_LEN, LK_ID_SHORT_LEN );
                lk_sanitize( r->id_short, LK_ID_SHORT_LEN );
                memcpy( r->build, e + LK_NAME_LEN + LK_ID_SHORT_LEN, 32 );
                lk_sanitize( r->build, 32 );
                r->state = e[LK_NAME_LEN + LK_ID_SHORT_LEN + 32];
                if( r->state >= LK_NUM_STATES )  r->state = LK_STATE_IDLE;
                r->panels = e[LK_NAME_LEN + LK_ID_SHORT_LEN + 33];
                memcpy( r->address, e + LK_NAME_LEN + LK_ID_SHORT_LEN + 34, 48 );
                lk_sanitize( r->address, 48 );
                memcpy( r->fp, e + LK_NAME_LEN + LK_ID_SHORT_LEN + 82, LK_FP_BYTES );
                memcpy( r->game, e + LK_NAME_LEN + LK_ID_SHORT_LEN + 82 + LK_FP_BYTES, LK_GAME_LEN );
                lk_sanitize( r->game, LK_GAME_LEN );
                if( ! strcmp( r->id_short, lk_id_short ) )  continue;   // this cabinet
                lkt_num_remote++;
            }
        }
        return;
    }
    lkt_close( c, "protocol error", false );
}

static void  lkt_read( lk_conn_t * c, lk_state_e my_state, byte my_panels )
{
    for( ;; )
    {
        int n;
        if( c->inlen >= LK_BUF_SIZE )
        {
            lkt_close( c, "frame too large", c->phase < LKC_ONLINE && ! c->outbound );
            return;
        }
        n = SSL_read( c->ssl, c->inbuf + c->inlen, LK_BUF_SIZE - c->inlen );
        if( n <= 0 )
        {
            int err = SSL_get_error( c->ssl, n );
            if( err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE )  break;
            if( c->phase == LKC_ONLINE )
                lkt_log_trouble( c, err == SSL_ERROR_ZERO_RETURN ? "closed by the other end" : "receive failed",
                                 err, lk_sock_errno() );
            lkt_close( c, err == SSL_ERROR_ZERO_RETURN ? "disconnected" : "connection lost", false );
            return;
        }
        c->inlen += n;
        c->rx_bytes += n;
        c->last_rx = lk_now();

        // Every complete frame in the buffer.
        while( c->phase != LKC_EMPTY && c->inlen >= 5 )
        {
            uint32_t len = c->inbuf[0] | (c->inbuf[1] << 8) | (c->inbuf[2] << 16)
                           | ((uint32_t)c->inbuf[3] << 24);
            byte type = c->inbuf[4];
            if( type == 0 || type >= LK_NUM_MSG
                || ( len > lk_msg_max[type] && ! lk_selfcheck_off( "framesize" ) ) )
            {
                lkt_close( c, "bad frame", c->phase < LKC_ONLINE && ! c->outbound );
                return;
            }
            if( c->inlen < 5 + (int)len )  break;
            c->rx_frames++;
            c->last_rx_type = type;
            lkt_handle_frame( c, type, c->inbuf + 5, len, my_state, my_panels );
            if( c->phase == LKC_EMPTY )  return;
            memmove( c->inbuf, c->inbuf + 5 + len, c->inlen - 5 - len );
            c->inlen -= 5 + len;
        }
    }
}

static void  lkt_handshake( lk_conn_t * c )
{
    int r = SSL_do_handshake( c->ssl );
    if( r == 1 )
    {
        lkt_tls_established( c );
        return;
    }
    r = SSL_get_error( c->ssl, r );
    if( r == SSL_ERROR_WANT_READ || r == SSL_ERROR_WANT_WRITE )  return;
    lkt_close( c, "TLS handshake failed", ! c->outbound );
}

static boolean  lkt_set_nonblocking( int fd )
{
#ifdef __WIN32__
    u_long on = 1;
    return ioctlsocket( fd, FIONBIO, &on ) == 0;
#else
    int fl = fcntl( fd, F_GETFL, 0 );
    return fl >= 0 && fcntl( fd, F_SETFL, fl | O_NONBLOCK ) == 0;
#endif
}

static lk_conn_t *  lkt_free_slot( void )
{
    int i;
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lkt_conn[i].phase == LKC_EMPTY )  return &lkt_conn[i];
    return NULL;
}

static void  lkt_resolve_allow( void )
{
    int i;
    lkt_num_allow_ip = 0;
    for( i = 0; i < lkt_set.num_allow; i++ )
    {
        struct addrinfo hints, * res, * ai;
        memset( &hints, 0, sizeof(hints) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if( getaddrinfo( lkt_set.allow[i], NULL, &hints, &res ) != 0 )
        {
            lk_log( "Cabinet Link: allow list: cannot resolve \"%s\"", lkt_set.allow[i] );
            continue;
        }
        for( ai = res; ai && lkt_num_allow_ip < LK_MAX_ALLOW * 4; ai = ai->ai_next )
            lkt_allow_ip[lkt_num_allow_ip++] = ((struct sockaddr_in*) ai->ai_addr)->sin_addr;
        freeaddrinfo( res );
    }
    lkt_allow_resolved = lk_now() | 1;
}

static void  lkt_accept( void )
{
    for( ;; )
    {
        struct sockaddr_in  sa;
        socklen_t  salen = sizeof(sa);
        char  addr[48];
        int   i, allowed = 0;
        lk_lockout_t * lo;
        lk_conn_t * c;
        int fd = accept( lkt_listen, (struct sockaddr*) &sa, &salen );
        if( fd < 0 )  return;

        inet_ntop( AF_INET, &sa.sin_addr, addr, sizeof(addr) );

        // Refuse before a single TLS byte: a stranger never reaches OpenSSL.
        for( i = 0; i < lkt_num_allow_ip; i++ )
            if( lkt_allow_ip[i].s_addr == sa.sin_addr.s_addr )  { allowed = 1; break; }
        if( ! allowed && ! lk_selfcheck_off( "allow" ) )
        {
            lk_closesocket( fd );
            lkt_refusal( addr, NULL, lkt_set.num_allow ? "not on the allow list"
                                                       : "allow list is empty" );
            continue;
        }
        lo = lkt_lockout_find( sa.sin_addr, false );
        if( lo && lo->until && lk_now() < lo->until && ! lk_selfcheck_off( "lockout" ) )
        {
            lk_closesocket( fd );
            lkt_refusal( addr, NULL, "locked out after repeated failures" );
            continue;
        }
        c = lkt_free_slot();
        if( ! c || ! lkt_set_nonblocking( fd ) )
        {
            lk_closesocket( fd );
            lkt_refusal( addr, NULL, "too many cabinets" );
            continue;
        }
        memset( c, 0, sizeof(*c) );
        c->fd = fd;
        c->ip = sa.sin_addr;
        dl_strncpy( c->addr, addr, sizeof(c->addr) );
        c->started = c->last_rx = c->last_tx = lk_now();
        c->ssl = SSL_new( lkt_ctx_server );
        if( ! c->ssl )  { c->phase = LKC_TLS; lkt_close( c, "out of memory", false ); continue; }
        SSL_set_fd( c->ssl, fd );
        SSL_set_accept_state( c->ssl );
        c->phase = LKC_TLS;
        lkt_handshake( c );
    }
}

static void  lkt_member_connect( void )
{
    struct addrinfo hints, * res;
    char port[8];
    lk_conn_t * c = &lkt_conn[0];
    int fd;

    lkt_next_attempt = lk_now() + lkt_backoff;
    lkt_backoff = ( lkt_backoff * 2 > LK_BACKOFF_MAX_MS ) ? LK_BACKOFF_MAX_MS : lkt_backoff * 2;

    memset( &hints, 0, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    snprintf( port, sizeof(port), "%d", lkt_set.port );
    if( getaddrinfo( lkt_set.master, port, &hints, &res ) != 0 )
    {
        snprintf( lkt_member_reason, sizeof(lkt_member_reason), "cannot resolve %.40s", lkt_set.master );
        return;
    }
    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( fd < 0 || ! lkt_set_nonblocking( fd ) )
    {
        if( fd >= 0 )  lk_closesocket( fd );
        freeaddrinfo( res );
        return;
    }
    memset( c, 0, sizeof(*c) );
    c->fd = fd;
    c->outbound = true;
    c->ip = ((struct sockaddr_in*) res->ai_addr)->sin_addr;
    dl_strncpy( c->addr, lkt_set.master, sizeof(c->addr) );
    c->started = c->last_rx = c->last_tx = lk_now();
    c->phase = LKC_TCP;
    if( connect( fd, res->ai_addr, res->ai_addrlen ) < 0 && lk_sock_errno() != LK_EINPROGRESS )
    {
        char why[64], msg[48];
        snprintf( why, sizeof(why), "connect: %s", lk_sock_strerror( lk_sock_errno(), msg, sizeof(msg) ) );
        freeaddrinfo( res );
        lkt_close( c, why, false );
        return;
    }
    freeaddrinfo( res );
}

static void  lkt_member_connected( lk_conn_t * c )
{
    int err = 0;
    socklen_t el = sizeof(err);
    // (void*): Winsock declares the option buffer char*, POSIX void*.
    getsockopt( c->fd, SOL_SOCKET, SO_ERROR, (void*) &err, &el );
    if( err )
    {
        char why[64], msg[48];
        snprintf( why, sizeof(why), "connect: %s", lk_sock_strerror( err, msg, sizeof(msg) ) );
        lkt_close( c, why, false );
        return;
    }
    c->ssl = SSL_new( lkt_ctx_client );
    if( ! c->ssl )  { lkt_close( c, "out of memory", false ); return; }
    SSL_set_fd( c->ssl, c->fd );
    SSL_set_connect_state( c->ssl );
    c->phase = LKC_TLS;
    lkt_handshake( c );
}

// Master: tell every member who else is on the link.
static void  lkt_broadcast_roster( lk_state_e my_state, byte my_panels )
{
    byte p[1 + (LK_MAX_PEERS + 1) * LK_PEERLIST_ENTRY];
    byte * e;
    int i, n = 0;
    uint32_t hash = 2166136261u;

    memset( p, 0, sizeof(p) );
    e = p + 1;
    // The master itself first, then every online member.
    for( i = -1; i < LK_MAX_PEERS && n < LK_MAX_PEERS; i++ )
    {
        const char * name, * idp, * addr, * game;
        const byte * fp;
        char ids[LK_ID_SHORT_LEN];
        byte st, pan;
        if( i < 0 )
        {
            name = lkt_set.name;  idp = lk_id_short;  addr = "master";
            st = my_state;  pan = my_panels;  fp = lk_fp;  game = lkt_my_game;
        }
        else
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase != LKC_ONLINE || ! c->got_hello )  continue;
            lk_fp_short( c->peer_fp, ids );
            name = c->name;  idp = ids;  addr = c->addr;
            st = c->state;  pan = c->panels;  fp = c->peer_fp;  game = c->game;
        }
        dl_strncpy( (char*) e, name, LK_NAME_LEN );
        dl_strncpy( (char*) e + LK_NAME_LEN, idp, LK_ID_SHORT_LEN );
        dl_strncpy( (char*) e + LK_NAME_LEN + LK_ID_SHORT_LEN,
                    i < 0 ? lkt_build_short() : lkt_conn[i].build, 32 );
        e[LK_NAME_LEN + LK_ID_SHORT_LEN + 32] = st;
        e[LK_NAME_LEN + LK_ID_SHORT_LEN + 33] = pan;
        dl_strncpy( (char*) e + LK_NAME_LEN + LK_ID_SHORT_LEN + 34, addr, 48 );
        memcpy( e + LK_NAME_LEN + LK_ID_SHORT_LEN + 82, fp, LK_FP_BYTES );
        dl_strncpy( (char*) e + LK_NAME_LEN + LK_ID_SHORT_LEN + 82 + LK_FP_BYTES, game, LK_GAME_LEN );
        e += LK_PEERLIST_ENTRY;
        n++;
    }
    p[0] = n;
    for( i = 0; i < (int)(e - p); i++ )  hash = (hash ^ p[i]) * 16777619u;
    if( hash == lkt_roster_hash )  return;
    lkt_roster_hash = hash;

    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lkt_conn[i].phase == LKC_ONLINE )
            lkt_queue( &lkt_conn[i], LK_MSG_PEERLIST, p, 1 + n * LK_PEERLIST_ENTRY );
}

// Publish what the game thread shows the operator.
static void  lkt_publish( void )
{
    lk_peer_info_t  list[LK_MAX_PEERS];
    int i, n = 0;
    uint32_t now = lk_now();

    memset( list, 0, sizeof(list) );
    if( lkt_set.role == LK_ROLE_MEMBER )
    {
        lk_conn_t * c = &lkt_conn[0];
        lk_peer_info_t * m = &list[n++];
        dl_strncpy( m->address, lkt_set.master, 48 );
        if( c->phase == LKC_ONLINE )
        {
            m->status = LK_PEER_ONLINE;
            dl_strncpy( m->name, c->name, LK_NAME_LEN );
            lk_fp_short( c->peer_fp, m->id_short );
            dl_strncpy( m->build, c->build, 32 );
            m->state = c->state;
            m->panels = c->panels;
            memcpy( m->fp, c->peer_fp, LK_FP_BYTES );
            dl_strncpy( m->game, c->game, LK_GAME_LEN );
        }
        else
        {
            m->status = ( c->phase == LKC_EMPTY ) ? LK_PEER_REFUSED
                      : ( c->phase == LKC_AUTH ) ? LK_PEER_AUTHENTICATING : LK_PEER_CONNECTING;
            if( m->status == LK_PEER_REFUSED && ! lkt_member_reason[0] )
                m->status = LK_PEER_CONNECTING;
            dl_strncpy( m->reason, lkt_member_reason, 64 );
        }
        if( c->phase == LKC_ONLINE )
        {
            // The rest of the roster, minus the master (already listed).
            for( i = 0; i < lkt_num_remote && n < LK_MAX_PEERS; i++ )
            {
                if( ! strcmp( lkt_remote[i].id_short, m->id_short ) )  continue;
                list[n++] = lkt_remote[i];
            }
        }
    }
    else
    {
        for( i = 0; i < LK_MAX_PEERS && n < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            lk_peer_info_t * m;
            if( c->phase == LKC_EMPTY )  continue;
            m = &list[n++];
            m->status = ( c->phase == LKC_ONLINE ) ? LK_PEER_ONLINE : LK_PEER_AUTHENTICATING;
            dl_strncpy( m->address, c->addr, 48 );
            dl_strncpy( m->name, c->name, LK_NAME_LEN );
            if( c->phase >= LKC_AUTH )  lk_fp_short( c->peer_fp, m->id_short );
            dl_strncpy( m->build, c->build, 32 );
            m->state = c->state;
            m->panels = c->panels;
            if( c->phase == LKC_ONLINE )  memcpy( m->fp, c->peer_fp, LK_FP_BYTES );
            dl_strncpy( m->game, c->game, LK_GAME_LEN );
        }
        for( i = 0; i < 8 && n < LK_MAX_PEERS; i++ )
        {
            if( ! lkt_refused[i].when || lk_since( now, lkt_refused[i].when ) > LK_REFUSED_SHOW_MS )  continue;
            list[n++] = lkt_refused[i].info;
        }
    }

    lk_lock();
    memcpy( lk_shared.peers, list, sizeof(list) );
    lk_shared.num_peers = n;
    lk_unlock();
}

static int  lkt_main( void * unused )
{
    struct pollfd  pfd[2 + LK_MAX_PEERS];
    int   pfd_conn[2 + LK_MAX_PEERS];
    (void) unused;

    for( ;; )
    {
        int i, np = 0, stop;
        lk_state_e my_state;
        byte my_panels;
        uint32_t now;

        char my_game[LK_GAME_LEN];
        boolean game_changed;
        lk_lock();
        stop = lk_shared.stop;
        my_state = lk_shared.my_state;
        my_panels = lk_shared.my_panels;
        memcpy( my_game, lk_shared.my_game, LK_GAME_LEN );
        lk_unlock();
        if( stop )  break;
        game_changed = ( memcmp( my_game, lkt_my_game, LK_GAME_LEN ) != 0 );
        memcpy( lkt_my_game, my_game, LK_GAME_LEN );
        lkt_drain_outbox();

        now = lk_now();

        // Timers first.
        if( lkt_set.role == LK_ROLE_MASTER && lk_since( now, lkt_allow_resolved ) > 60000 )
            lkt_resolve_allow();
        if( lkt_set.role == LK_ROLE_MEMBER && lkt_conn[0].phase == LKC_EMPTY
            && now >= lkt_next_attempt )
            lkt_member_connect();

        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase == LKC_EMPTY )  continue;
            if( c->phase != LKC_ONLINE && lk_since( now, c->started ) > LK_HANDSHAKE_MS )
                lkt_close( c, "timed out before authenticating", ! c->outbound );
            else if( c->phase == LKC_ONLINE && lk_since( now, c->last_rx ) > LK_DEAD_MS )
            {
                lkt_log_trouble( c, "silent", 0, 0 );
                lkt_close( c, "stopped responding", false );
            }
            else if( c->phase == LKC_ONLINE )
            {
                if( my_state != lkt_sent_state || my_panels != lkt_sent_panels || game_changed )
                {
                    byte pr[LK_PRESENCE_LEN];
                    pr[0] = my_state;
                    pr[1] = my_panels;
                    memcpy( pr + 2, lkt_my_game, LK_GAME_LEN );
                    lkt_queue( c, LK_MSG_PRESENCE, pr, sizeof(pr) );
                }
                else if( lk_since( now, c->last_tx ) > LK_PING_MS )
                    lkt_queue( c, LK_MSG_PING, NULL, 0 );
            }
        }
        lkt_sent_state = my_state;
        lkt_sent_panels = my_panels;
        if( lkt_set.role == LK_ROLE_MASTER )
            lkt_broadcast_roster( my_state, my_panels );

        // Records OpenSSL has already pulled off the socket do not make poll()
        // fire -- the member's proof often arrives in the same read as the end
        // of its handshake -- so drain them explicitly.
        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase >= LKC_AUTH && SSL_has_pending( c->ssl ) )
                lkt_read( c, my_state, my_panels );
        }
        lkt_drain_sync();
        for( i = 0; i < LK_MAX_PEERS; i++ )
            if( lkt_conn[i].phase >= LKC_TLS )  lkt_flush( &lkt_conn[i] );
        lkt_drain_sync();   // flushing may have made room for the next chunk

        lkt_publish();

        // Wait for something to do.
        pfd[np].fd = lk_wake_pipe[0];  pfd[np].events = POLLIN;  pfd_conn[np++] = -1;
        if( lkt_listen >= 0 )
        {
            pfd[np].fd = lkt_listen;  pfd[np].events = POLLIN;  pfd_conn[np++] = -2;
        }
        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase == LKC_EMPTY )  continue;
            pfd[np].fd = c->fd;
            pfd[np].events = POLLIN;
            if( c->phase == LKC_TCP || c->outlen > 0 )  pfd[np].events |= POLLOUT;
            pfd_conn[np++] = i;
        }
        // On Windows before 10 version 2004, WSAPoll does not report a refused
        // non-blocking connect at all; the member then gives up through
        // LK_HANDSHAKE_MS ("timed out before authenticating") instead.
        if( lk_poll( pfd, np, 500 ) <= 0 )  continue;

        for( i = 0; i < np; i++ )
        {
            lk_conn_t * c;
            if( ! pfd[i].revents )  continue;
            if( pfd_conn[i] == -1 )
            {
                char buf[64];
#ifdef __WIN32__
                while( recv( lk_wake_pipe[0], buf, sizeof(buf), 0 ) > 0 )  { }
#else
                while( read( lk_wake_pipe[0], buf, sizeof(buf) ) > 0 )  { }
#endif
                continue;
            }
            if( pfd_conn[i] == -2 )
            {
                lkt_accept();
                continue;
            }
            c = &lkt_conn[pfd_conn[i]];
            if( c->phase == LKC_TCP )
                lkt_member_connected( c );
            else if( c->phase == LKC_TLS )
            {
                lkt_handshake( c );
                if( c->phase >= LKC_AUTH )
                {
                    lkt_read( c, my_state, my_panels );
                    if( c->phase >= LKC_AUTH )  lkt_flush( c );
                }
            }
            else if( c->phase >= LKC_AUTH )
            {
                lkt_read( c, my_state, my_panels );
                if( c->phase >= LKC_AUTH )  lkt_flush( c );
            }
        }
    }

    // [Arcade] Send what the game thread queued last before closing.  A game
    // picked on Select Game tells the other cabinets just before this cabinet
    // restarts (LKSEL_Before_Restart), so they start switching now rather than
    // after this one has restarted and reconnected -- seconds on a Pi.
    lkt_drain_outbox();
    for( int i = 0; i < LK_MAX_PEERS; i++ )
        if( lkt_conn[i].phase == LKC_ONLINE )
            lkt_flush( &lkt_conn[i] );
    for( int i = 0; i < LK_MAX_PEERS; i++ )
        lkt_close( &lkt_conn[i], NULL, false );
    if( lkt_listen >= 0 )  { lk_closesocket( lkt_listen ); lkt_listen = -1; }
    lk_lock();
    lk_shared.done = 1;
    lk_unlock();
    return 0;
}

// ===========================================================================
//  Game thread
// ===========================================================================

static int   lk_verify_cb( int preverify, X509_STORE_CTX * ctx )
{
    // Identity is the pinned public key and the passcode proof, not a CA
    // chain: every certificate is self-signed, so accept it here and decide
    // after the handshake (lkt_tls_established).
    (void) preverify;  (void) ctx;
    return 1;
}

static SSL_CTX *  lk_make_ctx( boolean server )
{
    SSL_CTX * ctx = SSL_CTX_new( server ? TLS_server_method() : TLS_client_method() );
    if( ! ctx )  return NULL;
    SSL_CTX_set_min_proto_version( ctx, TLS1_3_VERSION );
    SSL_CTX_set_max_proto_version( ctx, TLS1_3_VERSION );
    if( SSL_CTX_use_certificate( ctx, lk_cert ) != 1
        || SSL_CTX_use_PrivateKey( ctx, lk_key ) != 1
        || SSL_CTX_check_private_key( ctx ) != 1 )
    {
        SSL_CTX_free( ctx );
        return NULL;
    }
    SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, lk_verify_cb );
    // No resumption: a resumed session would skip the certificates.
    SSL_CTX_set_options( ctx, SSL_OP_NO_TICKET );
    SSL_CTX_set_session_cache_mode( ctx, SSL_SESS_CACHE_OFF );
    SSL_CTX_set_num_tickets( ctx, 0 );
    SSL_CTX_set_mode( ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER );
    return ctx;
}

static char  lk_status_reason[96] = "";

// Create lk_wake_pipe.  See its declaration for why Windows differs.
static boolean  lk_wake_open( void )
{
#ifdef __WIN32__
    struct sockaddr_in  sa;
    socklen_t  salen = sizeof(sa);
    int  rd = socket( AF_INET, SOCK_DGRAM, 0 );
    int  wr = socket( AF_INET, SOCK_DGRAM, 0 );

    memset( &sa, 0, sizeof(sa) );
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    sa.sin_port = 0;   // any free port; getsockname says which
    if( rd < 0 || wr < 0
        || bind( rd, (struct sockaddr*) &sa, sizeof(sa) ) < 0
        || getsockname( rd, (struct sockaddr*) &sa, &salen ) < 0
        || connect( wr, (struct sockaddr*) &sa, sizeof(sa) ) < 0
        || ! lkt_set_nonblocking( rd ) || ! lkt_set_nonblocking( wr ) )
    {
        if( rd >= 0 )  lk_closesocket( rd );
        if( wr >= 0 )  lk_closesocket( wr );
        return false;
    }
    lk_wake_pipe[0] = rd;
    lk_wake_pipe[1] = wr;
#else
    if( pipe( lk_wake_pipe ) < 0 )  return false;
    lkt_set_nonblocking( lk_wake_pipe[0] );
    lkt_set_nonblocking( lk_wake_pipe[1] );
#endif
    return true;
}

static boolean  lk_start( void )
{
    int   sock_err = 0;
    char  sock_msg[64];

    lk_status_reason[0] = 0;
    if( lk_thread )  return true;
    if( lk_set.role == LK_ROLE_OFF )  return false;

    if( ! lk_identity_load() )
    {
        snprintf( lk_status_reason, sizeof(lk_status_reason), "no identity (see console)" );
        return false;
    }
    if( ! lk_set.passcode[0] )
    {
        snprintf( lk_status_reason, sizeof(lk_status_reason), "no passcode set" );
        return false;
    }
    if( lk_set.role == LK_ROLE_MEMBER && ! lk_set.master[0] )
    {
        snprintf( lk_status_reason, sizeof(lk_status_reason), "no master address set" );
        return false;
    }
    if( strlen( lk_set.passcode ) < LK_PASSCODE_MIN )
        GenPrintf( EMSG_warn, "Cabinet Link: the passcode is shorter than %d characters\n",
                   LK_PASSCODE_MIN );

#ifndef __WIN32__
    // A peer that vanishes mid-write must not kill the process.  (Winsock
    // has no SIGPIPE: the send simply fails.)
    signal( SIGPIPE, SIG_IGN );
#endif

    lkt_set = lk_set;
    memset( lkt_conn, 0, sizeof(lkt_conn) );
    for( int i = 0; i < LK_MAX_PEERS; i++ )  lkt_conn[i].fd = -1;
    memset( lkt_lockout, 0, sizeof(lkt_lockout) );
    memset( lkt_refused, 0, sizeof(lkt_refused) );
    lkt_num_remote = 0;
    lkt_member_reason[0] = 0;
    lkt_backoff = LK_BACKOFF_MIN_MS;
    lkt_next_attempt = 0;
    lkt_allow_resolved = 0;
    lkt_roster_hash = 0;
    lkt_sent_state = lkt_sent_panels = 255;

    if( ! lkt_ctx_server )  lkt_ctx_server = lk_make_ctx( true );
    if( ! lkt_ctx_client )  lkt_ctx_client = lk_make_ctx( false );
    if( ! lkt_ctx_server || ! lkt_ctx_client )
    {
        snprintf( lk_status_reason, sizeof(lk_status_reason), "TLS setup failed" );
        return false;
    }

    if( lk_set.role == LK_ROLE_MASTER )
    {
        struct sockaddr_in sa;
        int one = 1;
        lkt_listen = socket( AF_INET, SOCK_STREAM, 0 );
        if( lkt_listen < 0 )
        {
            sock_err = lk_sock_errno();
            goto sockfail;
        }
#ifdef __WIN32__
        // Not SO_REUSEADDR: on Windows that lets another program bind the same
        // port while this one is listening on it.  Windows does not hold a
        // listening port in TIME_WAIT, which is the only reason POSIX needs it.
        setsockopt( lkt_listen, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (void*) &one, sizeof(one) );
#else
        setsockopt( lkt_listen, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) );
#endif
        memset( &sa, 0, sizeof(sa) );
        sa.sin_family = AF_INET;
        sa.sin_port = htons( lk_set.port );
        sa.sin_addr.s_addr = htonl( INADDR_ANY );
        if( bind( lkt_listen, (struct sockaddr*) &sa, sizeof(sa) ) < 0
            || listen( lkt_listen, 8 ) < 0 || ! lkt_set_nonblocking( lkt_listen ) )
        {
            sock_err = lk_sock_errno();   // before the close can overwrite it
            lk_closesocket( lkt_listen );
            lkt_listen = -1;
            goto sockfail;
        }
        lkt_resolve_allow();
    }

    if( lk_wake_pipe[0] < 0 && ! lk_wake_open() )
    {
        snprintf( lk_status_reason, sizeof(lk_status_reason), "could not create the wake channel" );
        GenPrintf( EMSG_warn, "Cabinet Link: %s\n", lk_status_reason );
        if( lkt_listen >= 0 )  { lk_closesocket( lkt_listen ); lkt_listen = -1; }
        return false;
    }

    lk_lock();
    lk_shared.stop = 0;
    lk_shared.done = 0;
    lk_shared.num_peers = 0;
    lk_unlock();

    lk_thread = SDL_CreateThread( lkt_main, "cabinet-link", NULL );
    if( ! lk_thread )
    {
        if( lkt_listen >= 0 )  { lk_closesocket( lkt_listen ); lkt_listen = -1; }
        snprintf( lk_status_reason, sizeof(lk_status_reason), "could not start the link thread" );
        return false;
    }
    GenPrintf( EMSG_info, "Cabinet Link: %s \"%s\" (%s) started%s%s\n",
               lk_role_names[lk_set.role], lk_set.name, lk_id_short,
               lk_set.role == LK_ROLE_MEMBER ? ", master " : "",
               lk_set.role == LK_ROLE_MEMBER ? lk_set.master : "" );
    return true;

sockfail:
    snprintf( lk_status_reason, sizeof(lk_status_reason), "cannot listen on port %d: %s",
              lk_set.port, lk_sock_strerror( sock_err, sock_msg, sizeof(sock_msg) ) );
    GenPrintf( EMSG_warn, "Cabinet Link: %s\n", lk_status_reason );
    return false;
}

static void  lk_stop( void )
{
    int waited;
    if( ! lk_thread )  return;
    lk_lock();
    lk_shared.stop = 1;
    lk_unlock();
    lk_wake();
    // A thread stuck in a DNS lookup must not hang quit: give it a moment,
    // then let it go.
    for( waited = 0; waited < 3000; waited += 20 )
    {
        int done;
        lk_lock();
        done = lk_shared.done;
        lk_unlock();
        if( done )  break;
        SDL_Delay( 20 );
    }
    if( waited < 3000 )
        SDL_WaitThread( lk_thread, NULL );
    else
        SDL_DetachThread( lk_thread );
    lk_thread = NULL;
    lk_lock();
    lk_shared.num_peers = 0;
    lk_unlock();
}

// Presence: what this cabinet is doing, for the other cabinets.
static lk_state_e  lk_compute_state( void )
{
    if( devmode )  return LK_STATE_DEVMODE;
    if( M_Initials_Active() )  return LK_STATE_SIGNING;
    if( M_Join_Active() )  return LKG_Hosting_Remote() ? LK_STATE_HOSTING : LK_STATE_JOINING;
    if( D_Attract_Running() )  return menuactive ? LK_STATE_MENU : LK_STATE_IDLE;
    return LK_STATE_PLAYING;
}

static boolean  lk_start_failed_reported = false;
static lk_state_e  lk_last_state = LK_NUM_STATES;
static void  Command_Link_f( void );
static void  lk_net_status( void );
// -linkstatus: print the link status to the terminal every 2 seconds of wall
// time.  For tools/linktest.sh, whose engines run many to a core and cannot
// count on game tics keeping up, and for an operator watching a terminal.
static int       lk_status_every = -1;    // -1 unchecked, 0 off, else ms
static uint32_t  lk_status_next = 0;

// -linkstatus, whether or not the link is running.
static void  lk_status_tick( void )
{
    if( lk_status_every < 0 )
        lk_status_every = M_CheckParm( "-linkstatus" ) ? 2000 : 0;
    if( lk_status_every && lk_now() >= lk_status_next )
    {
        lk_status_next = lk_now() + lk_status_every;
        Command_Link_f();
    }
}

void  LK_Ticker( void )
{
    int i, nlog = 0, save_pins = 0;
    char log[LK_LOG_LINES][LK_LOG_LEN];
    lk_state_e st;
    byte panels;

    if( ! lk_inited )  return;

    // -linktest -linkkeys drives the Cabinet Link page, which is how a cabinet
    // with the link off gets it switched on -- so before the role test.
    LKG_Test_Keys();

    if( lk_set.role == LK_ROLE_OFF )
    {
        lk_status_tick();
        return;
    }
    if( ! lk_thread )
    {
        if( lk_start_failed_reported )  { lk_status_tick();  return; }
        if( ! lk_start() )
        {
            GenPrintf( EMSG_warn, "Cabinet Link: not started: %s\n", lk_status_reason );
            lk_start_failed_reported = true;
            return;
        }
    }

    st = lk_compute_state();
    panels = cv_localplayers.EV;

    lk_lock();
    lk_shared.my_state = st;
    lk_shared.my_panels = panels;
    dl_strncpy( lk_shared.my_game, LK_Game_Id(), LK_GAME_LEN );
    nlog = lk_shared.log_count;
    memcpy( log, lk_shared.log, sizeof(log) );
    lk_shared.log_count = 0;
    if( lk_shared.pins_dirty )
    {
        lk_shared.pins_dirty = 0;
        save_pins = 1;
        lk_pins_save();
    }
    lk_unlock();

    if( st != lk_last_state )
    {
        lk_last_state = st;
        lk_wake();
    }
    lk_status_tick();
    for( i = 0; i < nlog; i++ )
    {
        GenPrintf( EMSG_info, "%s\n", log[i] );
        // And to the terminal alone, where a headless test can read it.
        GenPrintf( EMSG_errlog, "LINKLOG %s\n", log[i] );
    }
    (void) save_pins;

    // Invites and linked games: after the log lines, so a message's effect
    // is printed after the line that says it arrived.
    LKG_Ticker();
    LKS_Ticker();   // [Arcade] shared scores
    LKSEL_Ticker(); // [Arcade] Select Game Sync
}

// ---------------------------------------------------------------------------
//  Console
// ---------------------------------------------------------------------------

static const char * lk_peer_status_names[] =
  { "-", "connecting", "authenticating", "online", "refused", "offline" };

static void  Command_Link_f( void )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    int i, n;

    if( ! lk_have_identity && lk_set.role != LK_ROLE_OFF )
        lk_identity_load();
    GenPrintf( EMSG_info, "Cabinet Link: %s, name \"%s\", id %s%s%s\n",
                 lk_role_names[lk_set.role], lk_set.name,
                 lk_id_short[0] ? lk_id_short : "(none yet)",
                 lk_status_reason[0] ? " -- " : "", lk_status_reason );
    if( lk_set.role == LK_ROLE_MEMBER )
        GenPrintf( EMSG_info, "  master %s port %d\n", lk_set.master, lk_set.port );
    if( lk_set.role == LK_ROLE_MASTER )
    {
        GenPrintf( EMSG_info, "  port %d, allowed:", lk_set.port );
        for( i = 0; i < lk_set.num_allow; i++ )  GenPrintf( EMSG_info, " %s", lk_set.allow[i] );
        GenPrintf( EMSG_info, lk_set.num_allow ? "\n" : " (nobody)\n" );
    }
    n = LK_Peers( peers, LK_MAX_PEERS );
    for( i = 0; i < n; i++ )
    {
        lk_peer_info_t * p = &peers[i];
        GenPrintf( EMSG_info, "  %-15s %-9s %-14s %-8s %s %s%s%s\n",
                     p->address, p->id_short[0] ? p->id_short : "-",
                     lk_peer_status_names[p->status],
                     p->status == LK_PEER_ONLINE ? LK_State_Name( p->state ) : "",
                     p->name, p->build, p->reason[0] ? " -- " : "", p->reason );
    }
    // One machine-readable line per peer, to the terminal only (not the
    // console), for tools/linktest.sh.
    lk_net_status();
    GenPrintf( EMSG_errlog, "LINKSELF %s %s %s\n", lk_role_names[lk_set.role],
               lk_id_short[0] ? lk_id_short : "-", lk_status_reason[0] ? lk_status_reason : "-" );
    for( i = 0; i < n; i++ )
        GenPrintf( EMSG_errlog, "LINKPEER %s %s %s %s %s|%s\n", peers[i].address,
                   peers[i].id_short[0] ? peers[i].id_short : "-",
                   lk_peer_status_names[peers[i].status],
                   LK_State_Name( peers[i].state ),
                   peers[i].name[0] ? peers[i].name : "-", peers[i].reason );
}

// ---------------------------------------------------------------------------
//  Settings (the Cabinet Link page and link_set)
// ---------------------------------------------------------------------------

// Saved, then applied by restarting the link; the ticker starts it again.
static const char *  lk_settings_apply( void )
{
    const char * err = NULL;
    if( ! lk_settings_save() )
        err = "could not write link.cfg";
    lk_stop();
    lk_start_failed_reported = false;
    return err;
}

void  LK_Setting_Get( lk_setting_e which, char * out, int outsize )
{
    if( outsize <= 0 )  return;
    out[0] = 0;
    switch( which )
    {
     case LK_SET_ROLE:     dl_strncpy( out, lk_role_names[lk_set.role], outsize );  break;
     case LK_SET_NAME:     dl_strncpy( out, lk_set.name, outsize );  break;
     case LK_SET_MASTER:   dl_strncpy( out, lk_set.master, outsize );  break;
     case LK_SET_PORT:     snprintf( out, outsize, "%d", lk_set.port );  break;
     case LK_SET_PASSCODE: dl_strncpy( out, lk_set.passcode, outsize );  break;
    }
}

const char *  LK_Setting_Set( lk_setting_e which, const char * value )
{
    int i;
    if( ! devmode )  return "only in an operator session";
    switch( which )
    {
     case LK_SET_ROLE:
        for( i = 0; i < 3; i++ )
            if( ! strcasecmp( value, lk_role_names[i] ) )  break;
        if( i == 3 )  return "role is off, master or member";
        lk_set.role = i;
        break;
     case LK_SET_NAME:
        if( ! value[0] )  return "a cabinet needs a name";
        dl_strncpy( lk_set.name, value, LK_NAME_LEN );
        lk_sanitize( lk_set.name, LK_NAME_LEN );
        break;
     case LK_SET_MASTER:
        dl_strncpy( lk_set.master, value, sizeof(lk_set.master) );
        break;
     case LK_SET_PORT:
        i = atoi( value );
        if( i <= 0 || i > 65535 )  return "port is 1 to 65535";
        lk_set.port = i;
        break;
     case LK_SET_PASSCODE:
        dl_strncpy( lk_set.passcode, value, sizeof(lk_set.passcode) );
        break;
    }
    return lk_settings_apply();
}

int  LK_Allow_Count( void )  { return lk_set.num_allow; }

const char *  LK_Allow_Get( int i )
{
    return ( i >= 0 && i < lk_set.num_allow ) ? lk_set.allow[i] : "";
}

const char *  LK_Allow_Add( const char * address )
{
    int i;
    if( ! devmode )  return "only in an operator session";
    if( ! address[0] )  return "no address";
    for( i = 0; i < lk_set.num_allow; i++ )
        if( ! strcasecmp( lk_set.allow[i], address ) )  return NULL;   // already there
    if( lk_set.num_allow >= LK_MAX_ALLOW )  return "allow list is full";
    dl_strncpy( lk_set.allow[lk_set.num_allow++], address, 128 );
    return lk_settings_apply();
}

const char *  LK_Allow_Remove( int i )
{
    if( ! devmode )  return "only in an operator session";
    if( i < 0 || i >= lk_set.num_allow )  return "no such address";
    memmove( &lk_set.allow[i], &lk_set.allow[i+1], (lk_set.num_allow - i - 1) * 128 );
    lk_set.num_allow--;
    return lk_settings_apply();
}

const char *  LK_Forget_Pins( void )
{
    if( ! devmode )  return "only in an operator session";
    lk_stop();
    lk_lock();
    lk_shared.num_pins = 0;
    lk_pins_save();
    lk_unlock();
    lk_start_failed_reported = false;
    return NULL;
}

// link_set <role|name|master|port|passcode|allow|unallow> <value>
static void  Command_LinkSet_f( void )
{
    const char * key = COM_Argv( 1 );
    const char * val = COM_Argv( 2 );
    const char * err;
    int i;

    if( ! devmode )
    {
        CONS_Printf( "link_set: only in an operator (-devmode) session\n" );
        return;
    }
    if( COM_Argc() < 3 )
    {
        CONS_Printf( "link_set role|name|master|port|passcode|allow|unallow <value>\n" );
        return;
    }
    if( ! strcasecmp( key, "role" ) )          err = LK_Setting_Set( LK_SET_ROLE, val );
    else if( ! strcasecmp( key, "name" ) )     err = LK_Setting_Set( LK_SET_NAME, val );
    else if( ! strcasecmp( key, "master" ) )   err = LK_Setting_Set( LK_SET_MASTER, val );
    else if( ! strcasecmp( key, "port" ) )     err = LK_Setting_Set( LK_SET_PORT, val );
    else if( ! strcasecmp( key, "passcode" ) )
    {
        // The rest of the line, so a passphrase may contain spaces.
        char pass[128] = "";
        for( i = 2; i < COM_Argc(); i++ )
        {
            if( i > 2 )  strncat( pass, " ", sizeof(pass) - strlen(pass) - 1 );
            strncat( pass, COM_Argv( i ), sizeof(pass) - strlen(pass) - 1 );
        }
        err = LK_Setting_Set( LK_SET_PASSCODE, pass );
    }
    else if( ! strcasecmp( key, "allow" ) )    err = LK_Allow_Add( val );
    else if( ! strcasecmp( key, "unallow" ) )
    {
        err = NULL;
        for( i = 0; i < lk_set.num_allow; i++ )
            if( ! strcasecmp( lk_set.allow[i], val ) )  { err = LK_Allow_Remove( i ); break; }
    }
    else
    {
        CONS_Printf( "link_set: unknown setting %s\n", key );
        return;
    }
    if( err )
        CONS_Printf( "link_set: %s\n", err );
    else
        CONS_Printf( "Cabinet Link: %s updated\n", key );
}

// Forget every pinned cabinet (a replaced master, a reinstalled member).
static void  Command_LinkForget_f( void )
{
    const char * err = LK_Forget_Pins();
    if( err )
        CONS_Printf( "link_forget: %s\n", err );
    else
        CONS_Printf( "Cabinet Link: forgot every paired cabinet\n" );
}


// ---------------------------------------------------------------------------
//  Operator page (Arcade Options -> Cabinet Link), read only for now
// ---------------------------------------------------------------------------
//
// Colours read backwards in V_DrawString: 0 is red, V_WHITEMAP is grey.  Red
// marks what needs the operator.  Glyphs are 7 tall; 9 is the row pitch.

void  LK_Drawer( int y, int y_end )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    char  buf[96];
    int   i, n;
    static const char * status_words[] = { "", "CONNECTING", "CHECKING", "ONLINE", "REFUSED", "OFFLINE" };

    if( ! lk_inited )
    {
        lk_draw_fit( 6, y, 308, 0, "NOT STARTED (SEE CONSOLE)" );
        return;
    }

    // One line: is it running, and this cabinet's id for pairing.
    if( lk_set.role == LK_ROLE_OFF )
        lk_draw_fit( 6, y, 308, V_WHITEMAP, "LINK IS OFF" );
    else if( ! lk_thread )
    {
        snprintf( buf, sizeof(buf), "NOT RUNNING: %.60s", lk_status_reason[0] ? lk_status_reason : "?" );
        lk_draw_fit( 6, y, 308, 0, buf );
    }
    else
    {
        snprintf( buf, sizeof(buf), "RUNNING   THIS CABINET'S ID %s", lk_id_short[0] ? lk_id_short : "-" );
        lk_draw_fit( 6, y, 308, V_WHITEMAP, buf );
    }
    if( lk_set.role == LK_ROLE_OFF )
        return;

    y += 14;
    V_DrawString( 6, y, V_WHITEMAP, "CABINET" );
    V_DrawString( 106, y, V_WHITEMAP, "ID" );
    V_DrawString( 176, y, V_WHITEMAP, "STATUS" );

    y += 11;
    n = LK_Peers( peers, LK_MAX_PEERS );
    if( n == 0 )
        lk_draw_fit( 6, y, 308, V_WHITEMAP, "NO OTHER CABINETS YET" );
    for( i = 0; i < n && y <= y_end; i++, y += 9 )
    {
        lk_peer_info_t * p = &peers[i];
        int  bad = ( p->status == LK_PEER_REFUSED );
        lk_draw_fit( 6, y, 96, bad ? 0 : V_WHITEMAP, p->name[0] ? p->name : p->address );
        lk_draw_fit( 106, y, 66, V_WHITEMAP, p->id_short[0] ? p->id_short : "-" );
        if( p->status == LK_PEER_ONLINE )
            snprintf( buf, sizeof(buf), "ONLINE %s", LK_State_Name( p->state ) );
        else
            snprintf( buf, sizeof(buf), "%s", status_words[p->status] );
        lk_draw_fit( 176, y, 138, bad ? 0 : V_WHITEMAP, buf );
        // The reason is what the operator acts on, and trimmed to the status
        // column it lost its meaning ("REFUSED: LOCKED OU"), so it gets a
        // full-width line of its own.
        if( p->status != LK_PEER_ONLINE && p->reason[0] && y + 9 <= y_end )
        {
            y += 9;
            lk_draw_fit( 18, y, 296, bad ? 0 : V_WHITEMAP, p->reason );
        }
        // [Arcade] Shared scores that are not being shared, and why -- a
        // different build or different wads would otherwise just never sync,
        // with nothing on screen to say so.  Nothing when all is well.
        else if( p->status == LK_PEER_ONLINE && y + 9 <= y_end )
        {
            const char * sc = LKS_Peer_Status( p->fp );
            if( sc[0] && strcmp( sc, "SCORES SHARED" ) )
            {
                y += 9;
                lk_draw_fit( 18, y, 296, 0, sc );
            }
        }
        // [Arcade] Select Game Sync: it could not follow the game chosen, and
        // why -- otherwise that cabinet just stays on its own game, silently.
        if( y + 9 <= y_end )
        {
            const char * gs = LKSEL_Peer_Status( p->fp );
            if( gs[0] )
            {
                y += 9;
                lk_draw_fit( 18, y, 296, 0, gs );
            }
        }
    }
    // [Arcade] ...and this cabinet itself, when it could not.
    if( n == 0 )
        y += 9;   // under "NO OTHER CABINETS YET"
    if( LKSEL_Self_Status()[0] && y <= y_end )
        lk_draw_fit( 6, y + 3, 308, 0, LKSEL_Self_Status() );
}

// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//  Game messages (game thread side)
// ---------------------------------------------------------------------------

boolean  LK_Send( const byte * target, byte type, const byte * data, int len )
{
    lk_event_t * m;
    boolean ok = false;
    if( ! lk_thread || len < 0 || len > LK_MSG_DATA_MAX || type == 0 || type >= LK_GM_NUM )
        return false;
    lk_lock();
    if( lk_shared.out_count < LK_EVENTS )
    {
        m = &lk_shared.outbox[(lk_shared.out_head + lk_shared.out_count) % LK_EVENTS];
        memset( m->source, 0, LK_FP_BYTES );
        if( target )  memcpy( m->source, target, LK_FP_BYTES );
        m->type = type;
        m->len = len;
        if( len )  memcpy( m->data, data, len );
        lk_shared.out_count++;
        ok = true;
    }
    lk_unlock();
    lk_wake();
    return ok;
}

boolean  LK_Sync_Send( const byte * peer, const byte * data, int len )
{
    boolean ok = false;
    if( ! lk_thread || ! peer || len <= 0 || len > LK_SYNC_DATA_MAX )  return false;
    lk_lock();
    if( lk_shared.sout_count < LK_SYNC_QUEUE )
    {
        lk_sync_msg_t * m = &lk_shared.sync_out[(lk_shared.sout_head + lk_shared.sout_count) % LK_SYNC_QUEUE];
        memcpy( m->peer, peer, LK_FP_BYTES );
        m->len = len;
        memcpy( m->data, data, len );
        lk_shared.sout_count++;
        ok = true;
    }
    lk_unlock();
    if( ok )  lk_wake();
    return ok;
}

boolean  LK_Sync_Poll( lk_sync_msg_t * out )
{
    boolean got = false;
    if( ! lk_inited || ! lk_mutex )  return false;
    lk_lock();
    if( lk_shared.sin_count )
    {
        *out = lk_shared.sync_in[lk_shared.sin_head];
        lk_shared.sin_head = (lk_shared.sin_head + 1) % LK_SYNC_QUEUE;
        lk_shared.sin_count--;
        got = true;
    }
    lk_unlock();
    return got;
}

int  LK_Sync_Peers( lk_peer_info_t * out, int max )
{
    lk_peer_info_t  list[LK_MAX_PEERS];
    int i, n = LK_Peers( list, LK_MAX_PEERS ), k = 0;
    static const byte zero[LK_FP_BYTES];
    for( i = 0; i < n && k < max; i++ )
    {
        if( list[i].status != LK_PEER_ONLINE || ! memcmp( list[i].fp, zero, LK_FP_BYTES ) )  continue;
        // A member's first entry is its master; the rest are other members,
        // seen through the master's roster, which it does not talk to.
        if( lk_set.role == LK_ROLE_MEMBER && i > 0 )  break;
        out[k++] = list[i];
    }
    return k;
}

void  LK_Sha256( const byte * data, int len, byte * out32 )
{
    SHA256( data, len > 0 ? len : 0, out32 );
}

const char *  LK_Build( void )  { return lkt_build_short(); }

boolean  LK_Poll_Event( lk_event_t * ev )
{
    boolean got = false;
    if( ! lk_inited || ! lk_mutex )  return false;
    lk_lock();
    if( lk_shared.ev_count )
    {
        *ev = lk_shared.events[lk_shared.ev_head];
        lk_shared.ev_head = (lk_shared.ev_head + 1) % LK_EVENTS;
        lk_shared.ev_count--;
        got = true;
    }
    lk_unlock();
    return got;
}

const byte * LK_My_Fp( void )  { return lk_have_identity ? lk_fp : NULL; }

boolean  LK_Peer_Find( const byte * fp, lk_peer_info_t * out )
{
    lk_peer_info_t  list[LK_MAX_PEERS];
    int i, n = LK_Peers( list, LK_MAX_PEERS );
    for( i = 0; i < n; i++ )
    {
        if( list[i].status == LK_PEER_ONLINE && ! memcmp( list[i].fp, fp, LK_FP_BYTES ) )
        {
            *out = list[i];
            return true;
        }
    }
    return false;
}

lk_state_e  LK_State( void )  { return lk_inited ? lk_compute_state() : LK_STATE_IDLE; }

// ---------------------------------------------------------------------------
//  The game channel: sealing the game netcode's UDP (game thread only)
// ---------------------------------------------------------------------------
//
// Wire format of a sealed packet:  u8 key id | u32 counter (LE) | ciphertext | 16 tag
// The key id says which joining cabinet's keys; the counter is the nonce and
// the replay guard.  21 bytes, so a full 1450-byte game packet still fits one
// Ethernet frame.  Each cabinet has one key each way, fresh for every START,
// so a counter is never reused under a key.  The header is authenticated as
// associated data.

#define LK_UDP_IDS   (LK_MAX_PEERS + 1)
#define LK_UDP_MAP   64

typedef struct
{
    boolean   used;
    byte      key_up[32];     // joining cabinet -> host
    byte      key_down[32];   // host -> joining cabinet
    uint32_t  send_ctr;       // counter for what this end sends under this id
    uint32_t  recv_max;       // highest counter accepted
    uint64_t  recv_mask;      // the 64 counters below it
    boolean   recv_any;
} lk_udpkey_t;

typedef enum { LKU_NONE = 0, LKU_HOST, LKU_CLIENT } lk_udp_mode_e;

static lk_udp_mode_e  lku_mode = LKU_NONE;
static lk_udpkey_t    lku_key[LK_UDP_IDS];
static byte           lku_client_id;
// Host: which address uses which key id, learned from the first packet that
// opens -- the host cannot seal a reply until the cabinet has spoken.
static struct { uint32_t ip; uint16_t port; byte id; } lku_map[LK_UDP_MAP];
static int            lku_map_n;
static EVP_CIPHER_CTX * lku_ctx = NULL;
// Counters for the status line and tools/linktest.sh.
static uint32_t       lku_sealed, lku_opened, lku_dropped;
static uint64_t       lku_ns;
static uint32_t       lku_ns_n;

static uint64_t  lk_ns_now( void )
{
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return (uint64_t) ts.tv_sec * 1000000000u + ts.tv_nsec;
}

void  LK_Udp_End( void )
{
    OPENSSL_cleanse( lku_key, sizeof(lku_key) );
    memset( lku_key, 0, sizeof(lku_key) );
    lku_map_n = 0;
    lku_mode = LKU_NONE;
}

void  LK_Udp_Host_Begin( void )
{
    LK_Udp_End();
    lku_mode = LKU_HOST;
    lku_sealed = lku_opened = lku_dropped = 0;
    lku_ns = 0;  lku_ns_n = 0;
}

int  LK_Udp_Host_Add_Client( byte * keys_out )
{
    int id;
    if( lku_mode != LKU_HOST )  return 0;
    for( id = 1; id < LK_UDP_IDS; id++ )
        if( ! lku_key[id].used )  break;
    if( id >= LK_UDP_IDS )  return 0;
    if( RAND_bytes( lku_key[id].key_up, 32 ) != 1 || RAND_bytes( lku_key[id].key_down, 32 ) != 1 )
        return 0;
    lku_key[id].used = true;
    memcpy( keys_out, lku_key[id].key_up, 32 );
    memcpy( keys_out + 32, lku_key[id].key_down, 32 );
    return id;
}

void  LK_Udp_Client_Begin( byte keyid, const byte * keys )
{
    LK_Udp_End();
    if( keyid == 0 || keyid >= LK_UDP_IDS )  return;
    lku_mode = LKU_CLIENT;
    lku_client_id = keyid;
    lku_key[keyid].used = true;
    memcpy( lku_key[keyid].key_up, keys, 32 );
    memcpy( lku_key[keyid].key_down, keys + 32, 32 );
    lku_sealed = lku_opened = lku_dropped = 0;
    lku_ns = 0;  lku_ns_n = 0;
}

// One AEAD operation.  Returns the output length, or -1 on failure (which on
// decrypt means the tag did not verify).
static int  lku_aead( boolean enc, const byte * key, const byte * iv, const byte * aad,
                      const byte * in, int len, byte * out, byte * tag )
{
    int n = 0, f = 0;
    if( ! lku_ctx && ! ( lku_ctx = EVP_CIPHER_CTX_new() ) )  return -1;
    if( EVP_CipherInit_ex( lku_ctx, EVP_chacha20_poly1305(), NULL, key, iv, enc ) != 1 )  return -1;
    if( ! enc && EVP_CIPHER_CTX_ctrl( lku_ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag ) != 1 )  return -1;
    if( EVP_CipherUpdate( lku_ctx, NULL, &n, aad, 5 ) != 1 )  return -1;
    if( len > 0 && EVP_CipherUpdate( lku_ctx, out, &n, in, len ) != 1 )  return -1;
    if( EVP_CipherFinal_ex( lku_ctx, out + n, &f ) != 1 )  return -1;
    if( enc && EVP_CIPHER_CTX_ctrl( lku_ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag ) != 1 )  return -1;
    return n + f;
}

static void  lku_iv( byte * iv, byte keyid, byte dir, const byte * ctr_le )
{
    memset( iv, 0, 12 );
    iv[0] = keyid;
    iv[1] = dir;          // 'U' up to the host, 'D' down to a cabinet
    memcpy( iv + 4, ctr_le, 4 );
}

// Replay window: accept each counter once, and nothing 64 or more behind the newest.
static boolean  lku_replay_ok( lk_udpkey_t * k, uint32_t ctr )
{
    if( ! k->recv_any || ctr > k->recv_max )  return true;
    if( k->recv_max - ctr >= 64 )  return false;
    return ! ( k->recv_mask & ((uint64_t)1 << (k->recv_max - ctr)) );
}

static void  lku_replay_mark( lk_udpkey_t * k, uint32_t ctr )
{
    if( ! k->recv_any )
    {
        k->recv_any = true;  k->recv_max = ctr;  k->recv_mask = 1;
    }
    else if( ctr > k->recv_max )
    {
        uint32_t shift = ctr - k->recv_max;
        k->recv_mask = ( shift >= 64 ) ? 1 : ( (k->recv_mask << shift) | 1 );
        k->recv_max = ctr;
    }
    else
        k->recv_mask |= (uint64_t)1 << (k->recv_max - ctr);
}

int  LK_Net_Recv( const byte * in, int len, byte * out, int outsize, uint32_t ip, uint16_t port )
{
    byte iv[12], keyid, tag[16];
    lk_udpkey_t * k;
    uint32_t ctr;
    uint64_t t0;
    int n, i;

    if( ! lk_inited || lk_set.role == LK_ROLE_OFF )  return -1;

    // -linktest -linknetloss P: throw away P percent of the linked game's
    // packets, as a poor Wi-Fi link does (tools/linktest.sh).  Its own random
    // numbers -- never the game's.
    if( lku_mode != LKU_NONE )
    {
        static int  loss = -1;
        static uint32_t  seed = 12345;
        if( loss < 0 )
            loss = ( M_CheckParm( "-linktest" ) && M_CheckParm( "-linknetloss" ) && M_IsNextParm() )
                   ? atoi( M_GetNextParm() ) : 0;
        if( loss > 0 )
        {
            seed = seed * 1103515245u + 12345u;
            if( (int)( (seed >> 16) % 100 ) < loss )
                return 0;
        }
    }

    if( lku_mode == LKU_NONE || len < LK_UDP_OVERHEAD || len - LK_UDP_OVERHEAD > outsize )
        goto drop;
    t0 = lk_ns_now();
    keyid = in[0];
    if( keyid == 0 || keyid >= LK_UDP_IDS || ! lku_key[keyid].used )  goto drop;
    if( lku_mode == LKU_CLIENT && keyid != lku_client_id )  goto drop;
    k = &lku_key[keyid];
    ctr = in[1] | (in[2] << 8) | (in[3] << 16) | ((uint32_t)in[4] << 24);
    if( ! lku_replay_ok( k, ctr ) )  goto drop;
    memcpy( tag, in + len - 16, 16 );
    lku_iv( iv, keyid, lku_mode == LKU_HOST ? 'U' : 'D', in + 1 );
    n = lku_aead( false, lku_mode == LKU_HOST ? k->key_up : k->key_down, iv, in,
                  in + 5, len - LK_UDP_OVERHEAD, out, tag );
    if( n < 0 )  goto drop;
    lku_replay_mark( k, ctr );

    if( lku_mode == LKU_HOST )
    {
        for( i = 0; i < lku_map_n; i++ )
            if( lku_map[i].ip == ip && lku_map[i].port == port )  break;
        if( i == lku_map_n && lku_map_n < LK_UDP_MAP )  lku_map_n++;
        if( i < LK_UDP_MAP )
        {
            lku_map[i].ip = ip;  lku_map[i].port = port;  lku_map[i].id = keyid;
        }
    }
    lku_opened++;
    lku_ns += lk_ns_now() - t0;
    lku_ns_n++;
    return n;

drop:
    // Before the netcode has seen a byte, and before SOCK_Get has given the
    // sender a node: a stranger cannot even use up a slot.
    //
    // The selfcheck switch lets only the failures through, so the linked game
    // itself still runs and the stranger case fails for the reason it tests.
    if( lk_selfcheck_off( "udp" ) )  return -1;
    lku_dropped++;
    return 0;
}

int  LK_Net_Send( const byte * in, int len, byte * out, int outsize, uint32_t ip, uint16_t port )
{
    byte iv[12], keyid = 0, *tag;
    lk_udpkey_t * k;
    uint64_t t0;
    int i, n;

    if( ! lk_inited || lk_set.role == LK_ROLE_OFF )  return -1;
    if( lku_mode == LKU_NONE )  return -1;   // nothing linked: the netcode as it was
    if( len + LK_UDP_OVERHEAD > outsize )  return 0;
    t0 = lk_ns_now();

    if( lku_mode == LKU_CLIENT )
        keyid = lku_client_id;
    else
    {
        for( i = 0; i < lku_map_n; i++ )
            if( lku_map[i].ip == ip && lku_map[i].port == port )  { keyid = lku_map[i].id; break; }
        if( ! keyid )  { lku_dropped++; return 0; }   // not a cabinet of this game
    }
    k = &lku_key[keyid];
    k->send_ctr++;
    out[0] = keyid;
    out[1] = k->send_ctr & 0xff;  out[2] = (k->send_ctr >> 8) & 0xff;
    out[3] = (k->send_ctr >> 16) & 0xff;  out[4] = (k->send_ctr >> 24) & 0xff;
    lku_iv( iv, keyid, lku_mode == LKU_HOST ? 'D' : 'U', out + 1 );
    tag = out + 5 + len;
    n = lku_aead( true, lku_mode == LKU_HOST ? k->key_down : k->key_up, iv, out, in, len, out + 5, tag );
    if( n != len )  { lku_dropped++; return 0; }
    lku_sealed++;
    lku_ns += lk_ns_now() - t0;
    lku_ns_n++;
    return len + LK_UDP_OVERHEAD;
}

// One line on the game channel and one on the linked game, terminal only.
static void  lk_net_status( void )
{
    static const char * modes[] = { "none", "host", "client" };
    GenPrintf( EMSG_errlog, "LINKNET %s sealed=%u opened=%u dropped=%u avg_us=%.1f\n",
               modes[lku_mode], lku_sealed, lku_opened, lku_dropped,
               lku_ns_n ? (double) lku_ns / lku_ns_n / 1000.0 : 0.0 );
    GenPrintf( EMSG_errlog, "LINKGAME gamestate=%d netgame=%d server=%d players=%d console=%d menu=%d"
               " views=%d viewport=%dx%d screen=%dx%d locals=%d,%d,%d,%d trail=%d p1=%d,%d %s\n",
               (int) gamestate, netgame, server, LKG_Players_In_Game(), consoleplayer,
               M_Message_Text() ? 2 : menuactive ? 1 : 0,
               D_NumViews(), rdraw_viewwidth, rdraw_viewheight, vid.width, vid.height,
               localplayer[0] == 255 ? -1 : localplayer[0], localplayer[1] == 255 ? -1 : localplayer[1],
               localplayer[2] == 255 ? -1 : localplayer[2], localplayer[3] == 255 ? -1 : localplayer[3],
               // Smoke trail phase against the game clock: fixed during a
               // game, so two cabinets in one game must print the same.
               (int)( game_comp_tic - gametic ),
               ( localplayer[0] < MAXPLAYERS && players[localplayer[0]].mo ) ? players[localplayer[0]].mo->x >> FRACBITS : 0,
               ( localplayer[0] < MAXPLAYERS && players[localplayer[0]].mo ) ? players[localplayer[0]].mo->y >> FRACBITS : 0,
               LKG_Mode_Name() );
    LKS_Status_Print();   // [Arcade] shared scores, per peer
    LKSEL_Status_Print(); // [Arcade] Select Game Sync
    // Every player in the game, as this cabinet has them: two cabinets in
    // one game must print the same line.
    if( gamestate == GS_LEVEL )
    {
        char  buf[MAXPLAYERS * (MAXPLAYERNAME + 8) + 1];
        int   i, len = 0;
        for( i = 0; i < MAXPLAYERS; i++ )
            if( playeringame[i] )
                len += snprintf( buf + len, sizeof(buf) - len, " %d=%s/%d", i, player_names[i], players[i].skincolor );
        buf[len] = '\0';
        GenPrintf( EMSG_errlog, "LINKNAMES%s\n", buf );
    }
}

boolean  LK_Built( void )  { return true; }

void  LK_Init( void )
{
    if( lk_inited )  return;
#ifdef __WIN32__
    // Winsock must be started before any socket call -- gethostname() in
    // lk_settings_defaults() included.  i_tcp.c starts it too, but only once
    // a network game is set up, and asks for version 1.1, which predates
    // getaddrinfo and WSAPoll.  WSAStartup counts its callers, so the two do
    // not interfere; there is no WSACleanup, as the process is exiting (or
    // being replaced by a restart) when the link stops for good.
    {
        WSADATA  wsa;
        if( WSAStartup( MAKEWORD(2,2), &wsa ) != 0 )
        {
            GenPrintf( EMSG_warn, "Cabinet Link: Winsock did not start; disabled\n" );
            return;
        }
    }
#endif
    cat_filename( lk_dir, legacyhome, "link" );
    cat_filename( lk_cfgfile, lk_dir, "link.cfg" );
    cat_filename( lk_keyfile, lk_dir, "cabinet.key" );
    cat_filename( lk_crtfile, lk_dir, "cabinet.crt" );
    cat_filename( lk_pinfile, lk_dir, "pins.txt" );
    if( access( lk_dir, R_OK ) < 0 )
        I_mkdir( lk_dir, 0700 );

    lk_mutex = SDL_CreateMutex();
    if( ! lk_mutex )
    {
        GenPrintf( EMSG_warn, "Cabinet Link: no mutex; disabled\n" );
        return;
    }
    lk_settings_load();
    lk_pins_load();
    COM_AddCommand( "link", Command_Link_f, CC_info );
    COM_AddCommand( "link_set", Command_LinkSet_f, CC_command );
    COM_AddCommand( "link_forget", Command_LinkForget_f, CC_command );
    lk_inited = true;
    if( lk_set.role != LK_ROLE_OFF )
        lk_identity_load();
}

void  LK_Shutdown( void )
{
    if( ! lk_inited )  return;
    lk_stop();
}

lk_role_e    LK_Role( void )      { return lk_set.role; }
const char * LK_Name( void )      { return lk_set.name; }
const char * LK_Id_Short( void )  { return lk_id_short; }

int  LK_Peers( lk_peer_info_t * out, int max )
{
    int n;
    if( ! lk_inited )  return 0;
    lk_lock();
    n = lk_shared.num_peers < max ? lk_shared.num_peers : max;
    memcpy( out, lk_shared.peers, n * sizeof(lk_peer_info_t) );
    lk_unlock();
    return n;
}

#endif  // HAVE_LINK
