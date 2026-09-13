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
#include "m_misc.h"
#include "i_system.h"
#include "m_argv.h"
#include "v_video.h"

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

void  LK_Drawer( void )
{
    lk_draw_fit( 6, 40, 308, 0, "NOT BUILT INTO THIS BINARY" );
    lk_draw_fit( 6, 52, 308, V_WHITEMAP, "IT NEEDS OPENSSL AT BUILD TIME:" );
    lk_draw_fit( 6, 62, 308, V_WHITEMAP, "INSTALL IT AND RUN TOOLS/BUILD.SH" );
}

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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>

#define LK_PROTO_VERSION    1
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
#define LK_MAX_ALLOW        LK_MAX_PEERS
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
    LK_NUM_MSG
};
#define LK_HELLO_LEN          (2+1+1+1+LK_NAME_LEN+32)
#define LK_PEERLIST_ENTRY     (LK_NAME_LEN + LK_ID_SHORT_LEN + 32 + 1 + 1 + 48)
static const uint32_t  lk_msg_max[LK_NUM_MSG] =
{
    0,
    1 + LK_FP_LEN,
    LK_HELLO_LEN,
    2,
    0,
    1 + LK_MAX_PEERS * LK_PEERLIST_ENTRY
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
} lk_shared;

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
        if( write( lk_wake_pipe[1], &c, 1 ) < 0 )  { /* full: already awake */ }
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
    if( fw )
        fchmod( fileno( fw ), 0600 );
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
    byte        role, state, panels;
    boolean     got_hello;
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
    if( c->fd >= 0 )  close( c->fd );
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
}

static void  lkt_flush( lk_conn_t * c )
{
    while( c->outlen > 0 && c->ssl )
    {
        int n = SSL_write( c->ssl, c->outbuf, c->outlen );
        if( n <= 0 )
        {
            int err = SSL_get_error( c->ssl, n );
            if( err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ )  return;
            lkt_close( c, "connection lost", false );
            return;
        }
        memmove( c->outbuf, c->outbuf + n, c->outlen - n );
        c->outlen -= n;
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
        if( ! c->got_hello )
        {
            c->got_hello = true;
            lk_pin_add( c->peer_fp, c->name, c->outbound );
        }
        lkt_roster_hash = 0;
        return;
     case LK_MSG_PRESENCE:
        if( len != 2 )  break;
        c->state = ( p[0] < LK_NUM_STATES ) ? p[0] : LK_STATE_IDLE;
        c->panels = p[1];
        lkt_roster_hash = 0;
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
            lkt_close( c, err == SSL_ERROR_ZERO_RETURN ? "disconnected" : "connection lost", false );
            return;
        }
        c->inlen += n;
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
    int fl = fcntl( fd, F_GETFL, 0 );
    return fl >= 0 && fcntl( fd, F_SETFL, fl | O_NONBLOCK ) == 0;
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
            close( fd );
            lkt_refusal( addr, NULL, lkt_set.num_allow ? "not on the allow list"
                                                       : "allow list is empty" );
            continue;
        }
        lo = lkt_lockout_find( sa.sin_addr, false );
        if( lo && lo->until && lk_now() < lo->until && ! lk_selfcheck_off( "lockout" ) )
        {
            close( fd );
            lkt_refusal( addr, NULL, "locked out after repeated failures" );
            continue;
        }
        c = lkt_free_slot();
        if( ! c || ! lkt_set_nonblocking( fd ) )
        {
            close( fd );
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
        if( fd >= 0 )  close( fd );
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
    if( connect( fd, res->ai_addr, res->ai_addrlen ) < 0 && errno != EINPROGRESS )
    {
        char why[64];
        snprintf( why, sizeof(why), "connect: %s", strerror( errno ) );
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
    getsockopt( c->fd, SOL_SOCKET, SO_ERROR, &err, &el );
    if( err )
    {
        char why[64];
        snprintf( why, sizeof(why), "connect: %s", strerror( err ) );
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
        const char * name, * idp, * addr;
        char ids[LK_ID_SHORT_LEN];
        byte st, pan;
        if( i < 0 )
        {
            name = lkt_set.name;  idp = lk_id_short;  addr = "master";
            st = my_state;  pan = my_panels;
        }
        else
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase != LKC_ONLINE || ! c->got_hello )  continue;
            lk_fp_short( c->peer_fp, ids );
            name = c->name;  idp = ids;  addr = c->addr;
            st = c->state;  pan = c->panels;
        }
        dl_strncpy( (char*) e, name, LK_NAME_LEN );
        dl_strncpy( (char*) e + LK_NAME_LEN, idp, LK_ID_SHORT_LEN );
        dl_strncpy( (char*) e + LK_NAME_LEN + LK_ID_SHORT_LEN,
                    i < 0 ? lkt_build_short() : lkt_conn[i].build, 32 );
        e[LK_NAME_LEN + LK_ID_SHORT_LEN + 32] = st;
        e[LK_NAME_LEN + LK_ID_SHORT_LEN + 33] = pan;
        dl_strncpy( (char*) e + LK_NAME_LEN + LK_ID_SHORT_LEN + 34, addr, 48 );
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
        }
        for( i = 0; i < 8 && n < LK_MAX_PEERS; i++ )
        {
            if( ! lkt_refused[i].when || now - lkt_refused[i].when > LK_REFUSED_SHOW_MS )  continue;
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

        lk_lock();
        stop = lk_shared.stop;
        my_state = lk_shared.my_state;
        my_panels = lk_shared.my_panels;
        lk_unlock();
        if( stop )  break;

        now = lk_now();

        // Timers first.
        if( lkt_set.role == LK_ROLE_MASTER && now - lkt_allow_resolved > 60000 )
            lkt_resolve_allow();
        if( lkt_set.role == LK_ROLE_MEMBER && lkt_conn[0].phase == LKC_EMPTY
            && now >= lkt_next_attempt )
            lkt_member_connect();

        for( i = 0; i < LK_MAX_PEERS; i++ )
        {
            lk_conn_t * c = &lkt_conn[i];
            if( c->phase == LKC_EMPTY )  continue;
            if( c->phase != LKC_ONLINE && now - c->started > LK_HANDSHAKE_MS )
                lkt_close( c, "timed out before authenticating", ! c->outbound );
            else if( c->phase == LKC_ONLINE && now - c->last_rx > LK_DEAD_MS )
                lkt_close( c, "stopped responding", false );
            else if( c->phase == LKC_ONLINE )
            {
                if( my_state != lkt_sent_state || my_panels != lkt_sent_panels )
                {
                    byte pr[2] = { my_state, my_panels };
                    lkt_queue( c, LK_MSG_PRESENCE, pr, 2 );
                }
                else if( now - c->last_tx > LK_PING_MS )
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
        for( i = 0; i < LK_MAX_PEERS; i++ )
            if( lkt_conn[i].phase >= LKC_TLS )  lkt_flush( &lkt_conn[i] );

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
        if( poll( pfd, np, 500 ) <= 0 )  continue;

        for( i = 0; i < np; i++ )
        {
            lk_conn_t * c;
            if( ! pfd[i].revents )  continue;
            if( pfd_conn[i] == -1 )
            {
                char buf[64];
                while( read( lk_wake_pipe[0], buf, sizeof(buf) ) > 0 )  { }
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

    for( int i = 0; i < LK_MAX_PEERS; i++ )
        lkt_close( &lkt_conn[i], NULL, false );
    if( lkt_listen >= 0 )  { close( lkt_listen ); lkt_listen = -1; }
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

static boolean  lk_start( void )
{
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

    // A peer that vanishes mid-write must not kill the process.
    signal( SIGPIPE, SIG_IGN );

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
            goto sockfail;
        setsockopt( lkt_listen, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) );
        memset( &sa, 0, sizeof(sa) );
        sa.sin_family = AF_INET;
        sa.sin_port = htons( lk_set.port );
        sa.sin_addr.s_addr = htonl( INADDR_ANY );
        if( bind( lkt_listen, (struct sockaddr*) &sa, sizeof(sa) ) < 0
            || listen( lkt_listen, 8 ) < 0 || ! lkt_set_nonblocking( lkt_listen ) )
        {
            close( lkt_listen );
            lkt_listen = -1;
            goto sockfail;
        }
        lkt_resolve_allow();
    }

    if( lk_wake_pipe[0] < 0 )
    {
        if( pipe( lk_wake_pipe ) < 0 )  goto sockfail;
        lkt_set_nonblocking( lk_wake_pipe[0] );
        lkt_set_nonblocking( lk_wake_pipe[1] );
    }

    lk_lock();
    lk_shared.stop = 0;
    lk_shared.done = 0;
    lk_shared.num_peers = 0;
    lk_unlock();

    lk_thread = SDL_CreateThread( lkt_main, "cabinet-link", NULL );
    if( ! lk_thread )
    {
        if( lkt_listen >= 0 )  { close( lkt_listen ); lkt_listen = -1; }
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
              lk_set.port, strerror( errno ) );
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
    if( M_Join_Active() )  return LK_STATE_JOINING;
    if( D_Attract_Running() )  return menuactive ? LK_STATE_MENU : LK_STATE_IDLE;
    return LK_STATE_PLAYING;
}

static boolean  lk_start_failed_reported = false;
static lk_state_e  lk_last_state = LK_NUM_STATES;
static void  Command_Link_f( void );
// -linkstatus: print the link status to the terminal every 2 seconds of wall
// time.  For tools/linktest.sh, whose engines run many to a core and cannot
// count on game tics keeping up, and for an operator watching a terminal.
static int       lk_status_every = -1;    // -1 unchecked, 0 off, else ms
static uint32_t  lk_status_next = 0;

void  LK_Ticker( void )
{
    int i, nlog = 0, save_pins = 0;
    char log[LK_LOG_LINES][LK_LOG_LEN];
    lk_state_e st;
    byte panels;

    if( ! lk_inited || lk_set.role == LK_ROLE_OFF )  return;

    if( ! lk_thread )
    {
        if( lk_start_failed_reported )  return;
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
    if( lk_status_every < 0 )
        lk_status_every = M_CheckParm( "-linkstatus" ) ? 2000 : 0;
    if( lk_status_every && lk_now() >= lk_status_next )
    {
        lk_status_next = lk_now() + lk_status_every;
        Command_Link_f();
    }
    for( i = 0; i < nlog; i++ )
    {
        GenPrintf( EMSG_info, "%s\n", log[i] );
        // And to the terminal alone, where a headless test can read it.
        GenPrintf( EMSG_errlog, "LINKLOG %s\n", log[i] );
    }
    (void) save_pins;
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
    GenPrintf( EMSG_errlog, "LINKSELF %s %s %s\n", lk_role_names[lk_set.role],
               lk_id_short[0] ? lk_id_short : "-", lk_status_reason[0] ? lk_status_reason : "-" );
    for( i = 0; i < n; i++ )
        GenPrintf( EMSG_errlog, "LINKPEER %s %s %s %s %s|%s\n", peers[i].address,
                   peers[i].id_short[0] ? peers[i].id_short : "-",
                   lk_peer_status_names[peers[i].status],
                   LK_State_Name( peers[i].state ),
                   peers[i].name[0] ? peers[i].name : "-", peers[i].reason );
}

// link_set <role|name|master|port|passcode|allow|unallow> <value>
static void  Command_LinkSet_f( void )
{
    const char * key = COM_Argv( 1 );
    const char * val = COM_Argv( 2 );
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
    if( ! strcasecmp( key, "role" ) )
    {
        for( i = 0; i < 3; i++ )
            if( ! strcasecmp( val, lk_role_names[i] ) )  break;
        if( i == 3 )  { CONS_Printf( "role is off, master or member\n" ); return; }
        lk_set.role = i;
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
        if( p <= 0 || p > 65535 )  { CONS_Printf( "port 1..65535\n" ); return; }
        lk_set.port = p;
    }
    else if( ! strcasecmp( key, "passcode" ) )
    {
        // The rest of the line, so a passphrase may contain spaces.
        char pass[128] = "";
        for( i = 2; i < COM_Argc(); i++ )
        {
            if( i > 2 )  strncat( pass, " ", sizeof(pass) - strlen(pass) - 1 );
            strncat( pass, COM_Argv( i ), sizeof(pass) - strlen(pass) - 1 );
        }
        dl_strncpy( lk_set.passcode, pass, sizeof(lk_set.passcode) );
    }
    else if( ! strcasecmp( key, "allow" ) )
    {
        if( lk_set.num_allow >= LK_MAX_ALLOW )  { CONS_Printf( "allow list is full\n" ); return; }
        dl_strncpy( lk_set.allow[lk_set.num_allow++], val, 128 );
    }
    else if( ! strcasecmp( key, "unallow" ) )
    {
        for( i = 0; i < lk_set.num_allow; i++ )
        {
            if( strcasecmp( lk_set.allow[i], val ) )  continue;
            memmove( &lk_set.allow[i], &lk_set.allow[i+1], (lk_set.num_allow - i - 1) * 128 );
            lk_set.num_allow--;
            break;
        }
    }
    else
    {
        CONS_Printf( "link_set: unknown setting %s\n", key );
        return;
    }

    if( ! lk_settings_save() )
        CONS_Printf( "link_set: could not write %s\n", lk_cfgfile );
    // Apply by restarting the link.
    lk_stop();
    lk_start_failed_reported = false;
    CONS_Printf( "Cabinet Link: %s updated\n", key );
}

// Forget every pinned cabinet (a replaced master, a reinstalled member).
static void  Command_LinkForget_f( void )
{
    if( ! devmode )
    {
        CONS_Printf( "link_forget: only in an operator (-devmode) session\n" );
        return;
    }
    lk_stop();
    lk_lock();
    lk_shared.num_pins = 0;
    lk_pins_save();
    lk_unlock();
    lk_start_failed_reported = false;
    CONS_Printf( "Cabinet Link: forgot every paired cabinet\n" );
}


// ---------------------------------------------------------------------------
//  Operator page (Arcade Options -> Cabinet Link), read only for now
// ---------------------------------------------------------------------------
//
// Colours read backwards in V_DrawString: 0 is red, V_WHITEMAP is grey.  Red
// marks what needs the operator.  Glyphs are 7 tall; 9 is the row pitch.

void  LK_Drawer( void )
{
    lk_peer_info_t  peers[LK_MAX_PEERS];
    char  buf[96];
    int   i, n, y;
    static const char * status_words[] = { "", "CONNECTING", "CHECKING", "ONLINE", "REFUSED", "OFFLINE" };

    if( ! lk_inited )
    {
        lk_draw_fit( 6, 40, 308, 0, "NOT STARTED (SEE CONSOLE)" );
        return;
    }

    snprintf( buf, sizeof(buf), "%s   %s   ID %s", lk_role_names[lk_set.role], lk_set.name,
              lk_id_short[0] ? lk_id_short : "-" );
    lk_draw_fit( 6, 28, 308, V_WHITEMAP, buf );

    y = 38;
    if( lk_set.role == LK_ROLE_OFF )
        lk_draw_fit( 6, y, 308, V_WHITEMAP, "LINK IS OFF" );
    else if( ! lk_thread )
    {
        snprintf( buf, sizeof(buf), "NOT RUNNING: %.60s", lk_status_reason[0] ? lk_status_reason : "?" );
        lk_draw_fit( 6, y, 308, 0, buf );
    }
    else
        lk_draw_fit( 6, y, 308, V_WHITEMAP, "RUNNING" );

    y = 48;
    if( lk_set.role == LK_ROLE_MEMBER )
    {
        snprintf( buf, sizeof(buf), "MASTER %.60s PORT %d", lk_set.master[0] ? lk_set.master : "(NONE)",
                  lk_set.port );
        lk_draw_fit( 6, y, 308, lk_set.master[0] ? V_WHITEMAP : 0, buf );
    }
    else if( lk_set.role == LK_ROLE_MASTER )
    {
        int len;
        len = snprintf( buf, sizeof(buf), "PORT %d  ALLOWED:", lk_set.port );
        for( i = 0; i < lk_set.num_allow && len < (int)sizeof(buf) - 2; i++ )
            len += snprintf( buf + len, sizeof(buf) - len, " %s", lk_set.allow[i] );
        if( ! lk_set.num_allow )
            snprintf( buf + len, sizeof(buf) - len, " NOBODY" );
        lk_draw_fit( 6, y, 308, lk_set.num_allow ? V_WHITEMAP : 0, buf );
    }

    if( lk_set.role != LK_ROLE_OFF )
    {
        int plen = strlen( lk_set.passcode );
        y = 58;
        if( ! plen )
            lk_draw_fit( 6, y, 308, 0, "NO PASSCODE SET" );
        else if( plen < LK_PASSCODE_MIN )
            lk_draw_fit( 6, y, 308, 0, "PASSCODE IS SHORT: USE 10 OR MORE" );
        else
            lk_draw_fit( 6, y, 308, V_WHITEMAP, "PASSCODE SET" );
    }

    y = 74;
    V_DrawString( 6, y, V_WHITEMAP, "CABINET" );
    V_DrawString( 106, y, V_WHITEMAP, "ID" );
    V_DrawString( 176, y, V_WHITEMAP, "STATUS" );

    n = LK_Peers( peers, LK_MAX_PEERS );
    if( n == 0 && lk_set.role != LK_ROLE_OFF )
        lk_draw_fit( 6, 86, 308, V_WHITEMAP, "NO OTHER CABINETS YET" );
    for( i = 0, y = 86; i < n && y <= 166; i++, y += 9 )
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
        if( p->status != LK_PEER_ONLINE && p->reason[0] && y + 9 <= 166 )
        {
            y += 9;
            lk_draw_fit( 18, y, 296, bad ? 0 : V_WHITEMAP, p->reason );
        }
    }
}

// ---------------------------------------------------------------------------

boolean  LK_Built( void )  { return true; }

void  LK_Init( void )
{
    if( lk_inited )  return;
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
