// [Arcade] Cabinet Link, Phase 2: shared high scores and demos.  See d_linkscore.h
// and docs/arcade/cabinet-link.md, "Shared high scores and demos".
//
// The protocol, over LK_Sync_Send (member <-> master only):
//
//   OFFER  u8 1, u32 proto, sha256 manifest, u32 length
//            "this is what I hold now" -- sent when it changes, when the peer
//            comes online, and every ten minutes as a backstop.
//   GET    u8 2, u8 blob, sha256, u32 offset, u8 chunks
//            the receiver pulls: a manifest by its hash, or a record demo by
//            its hash.  Nothing is sent that was not asked for, so the link's
//            queues never fill and a lost chunk is simply asked for again.
//   DATA   u8 3, u8 blob, sha256, u32 total, u32 offset, bytes
//   NONE   u8 4, u8 blob, sha256
//            "I no longer hold that" -- the offer moved on, or the demo changed.
//
// A manifest is text, one record per line, for the game this cabinet is
// running.  Everything read from one is validated as if hostile before it is
// used: game ids and map names become file names.

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <SDL.h>

#include "doomincl.h"
#include "doomstat.h"
#include "d_link.h"
#include "d_linkscore.h"
#include "d_linksel.h"
#include "d_netfil.h"
#include "hs_stuff.h"
#include "hs_merge.h"
#include "m_misc.h"
#include "p_local.h"    // cv_viewheight
#include "p_fab.h"      // cv_rocket_trails
#include "r_main.h"     // cv_invul_skymap

#define LKS_PROTO          1
#define LKS_HDR            42          // DATA header: kind, blob, sha, total, offset
#define LKS_CHUNK          (LK_SYNC_DATA_MAX - LKS_HDR)
#define LKS_WINDOW         4           // chunks per GET
#define LKS_MANIFEST_MAX   (1024*1024)
#define LKS_DEMO_MAX       (4*1024*1024)
#define LKS_READY_MAX      (32*1024*1024)   // demo bytes held for one apply
#define LKS_STALL_MS       4000
#define LKS_RETRY_MS       30000
#define LKS_RETRIES        6
#define LKS_REOFFER_MS     (10*60*1000)
#define LKS_REBUILD_MS     1000
#define LKS_APPLY_POLL_MS  1000
#define LKS_MAX_REJECT     64
#define LKS_MAX_READY      256
#define LKS_TICS_MAX       (35u * 60 * 60 * 24)   // a day: anything longer is not a real run

enum { LKS_OFFER = 1, LKS_GET, LKS_DATA, LKS_NONE };
enum { LKS_BLOB_MANIFEST = 0, LKS_BLOB_DEMO = 1 };

// Per peer: what is being fetched from it.
typedef enum
{
    LKSP_IDLE = 0,     // nothing to do (or done)
    LKSP_MANIFEST,     // pulling its manifest
    LKSP_DEMO,         // pulling a record demo
    LKSP_APPLY         // everything is here; waiting for the cabinet to be idle
} lks_phase_e;

typedef struct
{
    boolean      used;
    byte         fp[LK_FP_BYTES];
    char         name[LK_NAME_LEN];
    // What we last offered it, and when.
    byte         offered[32];
    uint32_t     offer_ms;
    // Its latest offer, and the last manifest of its we finished with.
    byte         their[32];
    uint32_t     their_len;
    byte         done[32];
    // The transfer in progress.
    lks_phase_e  phase;
    byte         want[32];        // sha of the blob being pulled
    byte *       buf;
    uint32_t     total, got;
    uint32_t     window_end;      // got reaches this: ask for the next window
    uint32_t     last_ms;
    uint32_t     retry_at;        // a failed manifest fetch starts again after this
    int          retries;
    // Its manifest, parsed (NULL until one has been).
    boolean      have_remote;
    boolean      in_scope;        // same build, wads and rules: records count
    char         status[48];
    hsm_set_t    remote;
    // Demos of its that failed to arrive or to check out.
    byte         rejected[LKS_MAX_REJECT][32];
    int          nrejected;
} lks_peer_t;

static lks_peer_t  lks_peer[LK_MAX_PEERS];

// Demos that arrived and checked out, waiting for the apply.  Kept in memory:
// they are written atomically, straight into place, only when the merge is
// applied, so an apply that never happens leaves nothing behind on disk.
typedef struct { byte sha[32]; byte * data; uint32_t len; } lks_ready_t;
static lks_ready_t  lks_ready[LKS_MAX_READY];
static int          lks_nready = 0;
static uint32_t     lks_ready_bytes = 0;

// This cabinet's manifest.
static char *    lks_man = NULL;
static uint32_t  lks_man_len = 0;
static byte      lks_man_sha[32];
static unsigned  lks_man_gen = ~0u;
static char      lks_man_game[LK_GAME_LEN];
static uint32_t  lks_man_ms = 0;
static boolean   lks_man_valid = false;
// The in-scope records it offers, for finding a demo by its hash.
static hsm_set_t lks_offer;

// Work sets, allocated once.
static hsm_set_t lks_raw, lks_local, lks_merged, lks_filtered;
static boolean   lks_alloc_failed = false;

static uint32_t  lks_now( void )  { return SDL_GetTicks(); }

// [Arcade] Milliseconds from then to now, never "negative".  LKS_Ticker reads
// now before it handles the messages that stamp last_ms, so a stamp can be
// later than now and the plain subtraction wraps to 49 days: a transfer that
// had just answered read as stalled, was asked for again, and after
// LKS_RETRIES gave up as "stopped answering".  See lk_since in d_link.c.
static uint32_t  lks_since( uint32_t now, uint32_t then )
{
    return ( now > then ) ? now - then : 0;
}

// ---------------------------------------------------------------------------
//  Little helpers

static void  put32( byte * p, uint32_t v )  { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static uint32_t  get32( const byte * p )  { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

static const byte  lks_zero[32];

static void  hex_encode( const byte * in, int n, char * out )
{
    static const char d[] = "0123456789abcdef";
    int i;
    for( i = 0; i < n; i++ )  { out[2*i] = d[in[i] >> 4]; out[2*i+1] = d[in[i] & 15]; }
    out[2*n] = 0;
}

static boolean  hex_decode( const char * in, byte * out, int n )
{
    int i;
    if( (int) strlen( in ) != 2 * n )  return false;
    for( i = 0; i < 2 * n; i++ )
    {
        int c = tolower( (unsigned char) in[i] ), v;
        if( c >= '0' && c <= '9' )  v = c - '0';
        else if( c >= 'a' && c <= 'f' )  v = c - 'a' + 10;
        else  return false;
        if( i & 1 )  out[i/2] |= v;  else  out[i/2] = v << 4;
    }
    return true;
}

static boolean  alloc_set( hsm_set_t * s )
{
    memset( s, 0, sizeof(*s) );
    s->max_splits = HS_Sync_Max_Splits();
    s->max_runs = HS_Sync_Max_Runs() * 2;   // a merge can briefly hold both sides
    s->splits = malloc( s->max_splits * sizeof(hsm_split_t) );
    s->runs = malloc( s->max_runs * sizeof(hsm_run_t) );
    return s->splits && s->runs;
}

static boolean  lks_alloc( void )
{
    if( lks_raw.splits || lks_alloc_failed )  return ! lks_alloc_failed;
    if( alloc_set( &lks_raw ) && alloc_set( &lks_local ) && alloc_set( &lks_merged )
        && alloc_set( &lks_filtered ) && alloc_set( &lks_offer ) )
        return true;
    GenPrintf( EMSG_warn, "Cabinet Link: shared scores are off, out of memory.\n" );
    lks_alloc_failed = true;
    return false;
}

static void  copy_set( hsm_set_t * dst, const hsm_set_t * src )
{
    dst->epoch = src->epoch;
    dst->nsplits = src->nsplits;
    dst->nruns = src->nruns;
    memcpy( dst->splits, src->splits, src->nsplits * sizeof(hsm_split_t) );
    memcpy( dst->runs, src->runs, src->nruns * sizeof(hsm_run_t) );
}

// ---------------------------------------------------------------------------
//  What makes two cabinets' records comparable

// The game this cabinet is running, and its "-sl" twin: the only ids shared.
static boolean  lks_game_in_scope( const char * game )
{
    const char * g = LK_Game_Id();
    size_t n = strlen( g );
    if( ! strcmp( game, g ) )  return true;
    return strncmp( game, g, n ) == 0 && ! strcmp( game + n, "-sl" );
}

// SHA-256 over the md5sums of every wad a linked game would require.  The
// same game id on two IWAD versions is two different games (Doom 2 v1.666
// against v1.9 desynced 16 of 18 record demos), and a level pack of the same
// file name may be a different version.
static void  lks_wad_fingerprint( char * hex )
{
    byte md5s[16 * MAX_WADFILES], sha[32];
    int n = D_Net_Wad_Md5s( md5s, MAX_WADFILES );
    LK_Sha256( md5s, 16 * n, sha );
    hex_encode( sha, 32, hex );
}

// The settings a record demo carries in its header that the ranked ruleset
// does not pin (hs_ranked_rules[]).  A record played under different ones is a
// different board, so they must match before records are shared.
// tools/hsmerge-test.py checks every header setting is either pinned or here.
static void  lks_rules_hash( char * hex )
{
    byte v[8], sha[32];
    v[0] = cv_rocket_trails.EV;
    v[1] = cv_viewheight.EV;
    v[2] = cv_invul_skymap.EV;
    LK_Sha256( v, 3, sha );
    hex_encode( sha, 8, hex );
}

// ---------------------------------------------------------------------------
//  Demo files: hashes, cached by size and modification time

typedef struct { char path[MAX_WADPATH]; off_t size; time_t mtime; byte sha[32]; } lks_hashcache_t;
#define LKS_HASHCACHE  512
static lks_hashcache_t  lks_hc[LKS_HASHCACHE];
static int              lks_hc_next = 0;

// Read a whole file (at most max bytes).  NULL if missing or too big.
static byte *  read_file( const char * path, uint32_t max, uint32_t * len )
{
    FILE * f = fopen( path, "rb" );
    long n;
    byte * b;
    if( ! f )  return NULL;
    fseek( f, 0, SEEK_END );
    n = ftell( f );
    fseek( f, 0, SEEK_SET );
    if( n <= 0 || (uint32_t) n > max )  { fclose( f ); return NULL; }
    b = malloc( n );
    if( b && fread( b, 1, n, f ) != (size_t) n )  { free( b ); b = NULL; }
    fclose( f );
    if( b )  *len = n;
    return b;
}

// The SHA-256 of the file at path; false when there is no such file.
static boolean  file_sha( const char * path, byte * out )
{
    struct stat st;
    int i;
    byte * data;
    uint32_t len;
    if( stat( path, &st ) != 0 )  return false;
    for( i = 0; i < LKS_HASHCACHE; i++ )
    {
        lks_hashcache_t * c = &lks_hc[i];
        if( c->size == st.st_size && c->mtime == st.st_mtime && ! strcmp( c->path, path ) )
        {
            memcpy( out, c->sha, 32 );
            return true;
        }
    }
    data = read_file( path, LKS_DEMO_MAX, &len );
    if( ! data )  return false;
    LK_Sha256( data, len, out );
    free( data );
    {
        lks_hashcache_t * c = &lks_hc[lks_hc_next];
        lks_hc_next = (lks_hc_next + 1) % LKS_HASHCACHE;
        dl_strncpy( c->path, path, MAX_WADPATH );
        c->size = st.st_size;
        c->mtime = st.st_mtime;
        memcpy( c->sha, out, 32 );
    }
    return true;
}

// Does this look like a whole record demo?  The DoomLegacy demo header at the
// front, and the end marker G_SnapshotDemo writes last -- which a truncated
// file has lost.
static boolean  demo_looks_whole( const byte * d, uint32_t len )
{
    return len >= 128 && d[0] == 144 && d[1] == 'D' && d[2] == 'L' && d[len-1] == 0x80;
}

// ---------------------------------------------------------------------------
//  This cabinet's scores, as the sync sees them

// raw: exactly what hs_stuff holds.  local: the same with each in-scope
// record's demo hash filled in.
//
// A record from before shared scores has no cabinet id, and it keeps none: two
// cabinets that both hold the same old record (a copied runs.dat, say) must
// see one record, not two.  Filling in each side's own id made the same run
// two entries and pushed a real one off a three deep board.
static boolean  lks_load_local( void )
{
    char path[MAX_WADPATH];
    int i;

    lks_raw.max_runs = HS_Sync_Max_Runs();
    HS_Sync_Export( &lks_raw );
    copy_set( &lks_local, &lks_raw );
    for( i = 0; i < lks_local.nsplits; i++ )
    {
        hsm_split_t * s = &lks_local.splits[i];
        memset( s->sha, 0, 32 );
        if( lks_game_in_scope( s->game ) )
        {
            HS_Sync_Split_Demo_Path( s, path );
            file_sha( path, s->sha );
        }
    }
    for( i = 0; i < lks_local.nruns; i++ )
    {
        hsm_run_t * r = &lks_local.runs[i];
        memset( r->sha, 0, 32 );
        if( lks_game_in_scope( r->game ) && HS_Sync_Run_Demo_Path( r, path ) )
            file_sha( path, r->sha );
    }
    return true;
}

// Build the manifest when the scores (or the game) changed.
static void  lks_build_manifest( void )
{
    static const char * none = "-";
    char  fp[65], rules[17], sha[65];
    uint32_t cap, n = 0;
    int i;

    if( lks_man_valid && lks_man_gen == HS_Sync_Generation()
        && ! strcmp( lks_man_game, LK_Game_Id() )
        && lks_now() - lks_man_ms < LKS_REOFFER_MS )
        return;
    if( lks_man_valid && lks_now() - lks_man_ms < LKS_REBUILD_MS )
        return;
    lks_load_local();

    cap = 512 + (lks_local.nsplits + lks_local.nruns) * 200;
    free( lks_man );
    lks_man = malloc( cap );
    if( ! lks_man )  { lks_man_valid = false; return; }

    lks_wad_fingerprint( fp );
    lks_rules_hash( rules );
    n += snprintf( lks_man + n, cap - n, "DLA-SCORES %d\nbuild %s\ngame %s %s\nrules %s\nepoch %u\n",
                   LKS_PROTO, LK_Build(), LK_Game_Id(), fp, rules, (unsigned) lks_local.epoch );

    lks_offer.epoch = lks_local.epoch;
    lks_offer.nsplits = lks_offer.nruns = 0;
    for( i = 0; i < lks_local.nsplits; i++ )
    {
        const hsm_split_t * s = &lks_local.splits[i];
        // A record goes out only with its demo: one without could never be
        // checked, and the other cabinet would refuse it anyway.
        if( ! lks_game_in_scope( s->game ) || ! memcmp( s->sha, lks_zero, 32 ) )  continue;
        hex_encode( s->sha, 32, sha );
        n += snprintf( lks_man + n, cap - n, "S %s %s %d %u %d %s %u %s %s\n",
                       s->game, s->map, s->skill, (unsigned) s->tics, s->cat,
                       s->startmap[0] ? s->startmap : none, (unsigned) s->set_time,
                       s->cab[0] ? s->cab : none, sha );
        lks_offer.splits[lks_offer.nsplits++] = *s;
    }
    for( i = 0; i < lks_local.nruns; i++ )
    {
        const hsm_run_t * r = &lks_local.runs[i];
        boolean single = HSM_Id_Is_Single( r->game );
        if( ! lks_game_in_scope( r->game ) )  continue;
        if( ! single && ! memcmp( r->sha, lks_zero, 32 ) )  continue;   // a Survival entry needs its demo
        if( single )  strcpy( sha, none );  else  hex_encode( r->sha, 32, sha );
        n += snprintf( lks_man + n, cap - n, "R %s %s %s %d %d %u %s %u %s %s\n",
                       r->game, r->startmap[0] ? r->startmap : none, r->endmap, r->skill, r->cat,
                       (unsigned) r->tics, r->initials[0] ? r->initials : "---",
                       (unsigned) r->set_time, r->cab[0] ? r->cab : none, sha );
        lks_offer.runs[lks_offer.nruns++] = *r;
    }
    n += snprintf( lks_man + n, cap - n, "end\n" );

    lks_man_len = n;
    LK_Sha256( (byte*) lks_man, n, lks_man_sha );
    lks_man_gen = HS_Sync_Generation();
    dl_strncpy( lks_man_game, LK_Game_Id(), LK_GAME_LEN );
    lks_man_ms = lks_now();
    lks_man_valid = true;
}

// ---------------------------------------------------------------------------
//  Reading another cabinet's manifest

static boolean  valid_word( const char * s, int maxlen, const char * extra )
{
    int i, n = strlen( s );
    if( n < 1 || n > maxlen )  return false;
    for( i = 0; i < n; i++ )
        if( ! isalnum( (unsigned char) s[i] ) && ! strchr( extra, s[i] ) )  return false;
    return true;
}

static boolean  valid_map( const char * m )
{
    int a, b;
    char tail;
    if( ! valid_word( m, 8, "" ) )  return false;
    return sscanf( m, "MAP%2d%c", &a, &tail ) == 1 || sscanf( m, "E%1dM%1d%c", &a, &b, &tail ) == 2;
}

// A cabinet id, or "-" for a record from before ids were kept.
static boolean  valid_cab( const char * c )  { return valid_word( c, HSM_CAB_LEN - 1, "-" ); }
static void  read_cab( char * dst, const char * c )  { dl_strncpy( dst, strcmp( c, "-" ) ? c : "", HSM_CAB_LEN ); }

// Parse a manifest into p->remote.  Records for other games, and anything that
// does not validate, are dropped; the header decides p->in_scope.
static boolean  lks_parse( lks_peer_t * p, char * text, uint32_t len )
{
    char * line, * next, * end = text + len;
    char game[64], map[16], start[16], endm[16], cab[16], sha[80], ini[16], w1[64], w2[80];
    int  skill, cat, proto = 0;
    unsigned tics, st, epoch = 0;
    boolean build_ok = false, game_ok = false, rules_ok = false, got_end = false;
    char my_fp[65], my_rules[17];

    if( ! p->remote.splits && ! alloc_set( &p->remote ) )  return false;
    p->remote.nsplits = p->remote.nruns = 0;
    lks_wad_fingerprint( my_fp );
    lks_rules_hash( my_rules );
    dl_strncpy( p->status, "SCORES SHARED", sizeof(p->status) );

    for( line = text; line < end; line = next )
    {
        next = memchr( line, '\n', end - line );
        if( ! next )  break;
        *next++ = 0;

        if( sscanf( line, "DLA-SCORES %d", &proto ) == 1 )  continue;
        if( sscanf( line, "build %63s", w1 ) == 1 )  { build_ok = ! strcmp( w1, LK_Build() ); continue; }
        if( sscanf( line, "game %63s %79s", w1, w2 ) == 2 )
        {
            game_ok = ! strcmp( w1, LK_Game_Id() ) && ! strcmp( w2, my_fp );
            if( strcmp( w1, LK_Game_Id() ) )
                snprintf( p->status, sizeof(p->status), "SCORES: PLAYING %.24s", w1 );
            else if( ! game_ok )
                dl_strncpy( p->status, "SCORES: DIFFERENT WADS", sizeof(p->status) );
            continue;
        }
        if( sscanf( line, "rules %63s", w1 ) == 1 )  { rules_ok = ! strcmp( w1, my_rules ); continue; }
        if( sscanf( line, "epoch %u", &epoch ) == 1 )  continue;
        if( ! strcmp( line, "end" ) )  { got_end = true; break; }

        if( line[0] == 'S' && sscanf( line, "S %63s %15s %d %u %d %15s %u %15s %79s",
                                      game, map, &skill, &tics, &cat, start, &st, cab, sha ) == 9 )
        {
            hsm_split_t * s;
            if( p->remote.nsplits >= p->remote.max_splits )  continue;
            if( ! valid_word( game, HSM_GAME_LEN - 1, "-_.+" ) || ! lks_game_in_scope( game ) )  continue;
            if( ! valid_map( map ) || skill < 0 || skill >= HSM_NUMSKILLS || cat < 0 || cat >= HSM_NUMCAT )  continue;
            if( tics == 0 || tics > LKS_TICS_MAX || ! valid_cab( cab ) )  continue;
            if( strcmp( start, "-" ) && ! valid_map( start ) )  continue;
            s = &p->remote.splits[p->remote.nsplits];
            memset( s, 0, sizeof(*s) );
            if( ! hex_decode( sha, s->sha, 32 ) || ! memcmp( s->sha, lks_zero, 32 ) )  continue;
            dl_strncpy( s->game, game, HSM_GAME_LEN );
            dl_strncpy( s->map, map, HSM_MAP_LEN );
            if( strcmp( start, "-" ) )  dl_strncpy( s->startmap, start, HSM_MAP_LEN );
            s->skill = skill;  s->cat = cat;  s->tics = tics;  s->set_time = st;
            read_cab( s->cab, cab );
            p->remote.nsplits++;
            continue;
        }
        if( line[0] == 'R' && sscanf( line, "R %63s %15s %15s %d %d %u %15s %u %15s %79s",
                                      game, start, endm, &skill, &cat, &tics, ini, &st, cab, sha ) == 10 )
        {
            hsm_run_t * r;
            boolean single;
            int k;
            if( p->remote.nruns >= p->remote.max_runs )  continue;
            if( ! valid_word( game, HSM_GAME_LEN - 1, "-_.+" ) || ! lks_game_in_scope( game ) )  continue;
            single = HSM_Id_Is_Single( game );
            if( ! valid_map( endm ) || ( strcmp( start, "-" ) && ! valid_map( start ) ) )  continue;
            if( skill < 0 || skill >= HSM_NUMSKILLS || cat < 0 || cat >= HSM_NUMCAT )  continue;
            if( tics == 0 || tics > LKS_TICS_MAX || ! valid_cab( cab ) )  continue;
            r = &p->remote.runs[p->remote.nruns];
            memset( r, 0, sizeof(*r) );
            if( single )
            {
                if( strcmp( sha, "-" ) )  continue;
            }
            else if( ! hex_decode( sha, r->sha, 32 ) || ! memcmp( r->sha, lks_zero, 32 ) )
                continue;
            if( strcmp( ini, "---" ) )
            {
                // Initials are drawn with hu_font: printable, upper case, three.
                if( strlen( ini ) > HSM_INI_LEN - 1 )  continue;
                for( k = 0; ini[k]; k++ )
                    if( ! isgraph( (unsigned char) ini[k] ) )  break;
                if( ini[k] )  continue;
                for( k = 0; ini[k]; k++ )  r->initials[k] = toupper( (unsigned char) ini[k] );
            }
            dl_strncpy( r->game, game, HSM_GAME_LEN );
            if( strcmp( start, "-" ) )  dl_strncpy( r->startmap, start, HSM_MAP_LEN );
            dl_strncpy( r->endmap, endm, HSM_MAP_LEN );
            r->skill = skill;  r->cat = cat;  r->tics = tics;  r->set_time = st;
            read_cab( r->cab, cab );
            p->remote.nruns++;
            continue;
        }
    }

    if( proto != LKS_PROTO || ! got_end )
        return false;
    p->remote.epoch = epoch;
    p->in_scope = build_ok && game_ok && rules_ok;
    if( ! build_ok )
        dl_strncpy( p->status, "SCORES: DIFFERENT BUILD", sizeof(p->status) );
    else if( game_ok && ! rules_ok )
        dl_strncpy( p->status, "SCORES: DIFFERENT SETTINGS", sizeof(p->status) );
    p->have_remote = true;
    return true;
}

// ---------------------------------------------------------------------------
//  Sending

static void  lks_send_offer( lks_peer_t * p )
{
    byte m[1 + 4 + 32 + 4];
    m[0] = LKS_OFFER;
    put32( m + 1, LKS_PROTO );
    memcpy( m + 5, lks_man_sha, 32 );
    put32( m + 37, lks_man_len );
    if( LK_Sync_Send( p->fp, m, sizeof(m) ) )
    {
        memcpy( p->offered, lks_man_sha, 32 );
        p->offer_ms = lks_now();
    }
}

static void  lks_send_get( lks_peer_t * p, byte blob )
{
    byte m[1 + 1 + 32 + 4 + 1];
    m[0] = LKS_GET;
    m[1] = blob;
    memcpy( m + 2, p->want, 32 );
    put32( m + 34, p->got );
    m[38] = LKS_WINDOW;
    LK_Sync_Send( p->fp, m, sizeof(m) );
    p->window_end = p->got + LKS_WINDOW * LKS_CHUNK;
    p->last_ms = lks_now();
}

static void  lks_send_none( const byte * fp, byte blob, const byte * sha )
{
    byte m[1 + 1 + 32];
    m[0] = LKS_NONE;
    m[1] = blob;
    memcpy( m + 2, sha, 32 );
    LK_Sync_Send( fp, m, sizeof(m) );
}

// The demo file being served, kept while a peer pulls it.
static byte *    lks_serve_data = NULL;
static uint32_t  lks_serve_len = 0;
static byte      lks_serve_sha[32];

static void  lks_serve( const byte * fp, byte blob, const byte * sha, uint32_t offset, int chunks )
{
    const byte * data = NULL;
    uint32_t len = 0;
    byte m[LK_SYNC_DATA_MAX];
    int k;

    if( blob == LKS_BLOB_MANIFEST )
    {
        if( lks_man_valid && ! memcmp( sha, lks_man_sha, 32 ) )
            { data = (byte*) lks_man; len = lks_man_len; }
    }
    else if( blob == LKS_BLOB_DEMO )
    {
        if( lks_serve_data && ! memcmp( sha, lks_serve_sha, 32 ) )
            { data = lks_serve_data; len = lks_serve_len; }
        else
        {
            // Only a demo this cabinet offered, found by its hash: never an
            // arbitrary path a peer named.
            char path[MAX_WADPATH];
            int i;
            path[0] = 0;
            for( i = 0; i < lks_offer.nsplits && ! path[0]; i++ )
                if( ! memcmp( lks_offer.splits[i].sha, sha, 32 ) )
                    HS_Sync_Split_Demo_Path( &lks_offer.splits[i], path );
            for( i = 0; i < lks_offer.nruns && ! path[0]; i++ )
                if( ! memcmp( lks_offer.runs[i].sha, sha, 32 ) )
                    HS_Sync_Run_Demo_Path( &lks_offer.runs[i], path );
            if( path[0] )
            {
                byte check[32];
                free( lks_serve_data );
                lks_serve_data = read_file( path, LKS_DEMO_MAX, &lks_serve_len );
                if( lks_serve_data )
                {
                    LK_Sha256( lks_serve_data, lks_serve_len, check );
                    if( memcmp( check, sha, 32 ) )   // changed since it was offered
                        { free( lks_serve_data ); lks_serve_data = NULL; }
                }
                if( lks_serve_data )
                {
                    memcpy( lks_serve_sha, sha, 32 );
                    data = lks_serve_data;
                    len = lks_serve_len;
                }
            }
        }
    }

    if( ! data )
    {
        lks_send_none( fp, blob, sha );
        return;
    }
    if( chunks > LKS_WINDOW )  chunks = LKS_WINDOW;
    for( k = 0; k < chunks && offset < len; k++ )
    {
        uint32_t n = len - offset;
        if( n > LKS_CHUNK )  n = LKS_CHUNK;
        m[0] = LKS_DATA;
        m[1] = blob;
        memcpy( m + 2, sha, 32 );
        put32( m + 34, len );
        put32( m + 38, offset );
        memcpy( m + LKS_HDR, data + offset, n );
        if( ! LK_Sync_Send( fp, m, LKS_HDR + n ) )  break;   // queue full: it asks again
        offset += n;
    }
}

// ---------------------------------------------------------------------------
//  The merge, planned and applied

static boolean  lks_rejected( const lks_peer_t * p, const byte * sha )
{
    int i;
    for( i = 0; i < p->nrejected; i++ )
        if( ! memcmp( p->rejected[i], sha, 32 ) )  return true;
    return false;
}

static void  lks_reject( lks_peer_t * p, const byte * sha, const char * why )
{
    char hex[17];
    hex_encode( sha, 8, hex );
    GenPrintf( EMSG_errlog, "LINKLOG Scores: refused a demo from %s (%s): %s\n", p->name, hex, why );
    if( p->nrejected < LKS_MAX_REJECT && ! lks_rejected( p, sha ) )
        memcpy( p->rejected[p->nrejected++], sha, 32 );
}

static lks_ready_t *  lks_find_ready( const byte * sha )
{
    int i;
    for( i = 0; i < lks_nready; i++ )
        if( ! memcmp( lks_ready[i].sha, sha, 32 ) )  return &lks_ready[i];
    return NULL;
}

static void  lks_free_ready( void )
{
    int i;
    for( i = 0; i < lks_nready; i++ )  free( lks_ready[i].data );
    lks_nready = 0;
    lks_ready_bytes = 0;
}

// Compute merge(local, their records) into lks_merged.  Returns how many demos
// the result needs that are not on disk, and the first one not yet fetched in
// *missing (zero if all of them have arrived).
static int  lks_plan( lks_peer_t * p, byte * missing )
{
    int i, need = 0;
    char path[MAX_WADPATH];
    byte have[32];

    memset( missing, 0, 32 );
    lks_load_local();

    // Their records only if they are comparable, and without any whose demo
    // did not check out.  Their epoch counts either way: a clear on the master
    // spreads even to a cabinet running another game.
    lks_filtered.epoch = p->remote.epoch;
    lks_filtered.nsplits = lks_filtered.nruns = 0;
    if( p->in_scope )
    {
        for( i = 0; i < p->remote.nsplits; i++ )
            if( ! lks_rejected( p, p->remote.splits[i].sha ) )
                lks_filtered.splits[lks_filtered.nsplits++] = p->remote.splits[i];
        for( i = 0; i < p->remote.nruns; i++ )
            if( HSM_Id_Is_Single( p->remote.runs[i].game ) || ! lks_rejected( p, p->remote.runs[i].sha ) )
                lks_filtered.runs[lks_filtered.nruns++] = p->remote.runs[i];
    }
    if( HSM_Merge( &lks_local, &lks_filtered, &lks_merged ) != 0 )
        return -1;

    for( i = 0; i < lks_merged.nsplits + lks_merged.nruns; i++ )
    {
        const byte * sha;
        if( i < lks_merged.nsplits )
        {
            const hsm_split_t * s = &lks_merged.splits[i];
            if( ! memcmp( s->sha, lks_zero, 32 ) )  continue;
            HS_Sync_Split_Demo_Path( s, path );
            sha = s->sha;
        }
        else
        {
            const hsm_run_t * r = &lks_merged.runs[i - lks_merged.nsplits];
            if( ! memcmp( r->sha, lks_zero, 32 ) || ! HS_Sync_Run_Demo_Path( r, path ) )  continue;
            sha = r->sha;
        }
        if( file_sha( path, have ) && ! memcmp( have, sha, 32 ) )  continue;
        need++;
        if( ! lks_find_ready( sha ) && ! memcmp( missing, lks_zero, 32 ) )
            memcpy( missing, sha, 32 );
    }
    return need;
}

// Write the merge.  Every demo it needs has arrived (lks_plan said so a moment
// ago, on the same local scores).
static void  lks_apply( lks_peer_t * p )
{
    char path[MAX_WADPATH];
    int i, demos = 0, gone = 0, before_s = lks_raw.nsplits, before_r = lks_raw.nruns;
    boolean changed;

    for( i = 0; i < lks_merged.nsplits + lks_merged.nruns; i++ )
    {
        const byte * sha;
        byte have[32];
        lks_ready_t * rd;
        FILE * f;
        if( i < lks_merged.nsplits )
        {
            if( ! memcmp( lks_merged.splits[i].sha, lks_zero, 32 ) )  continue;
            HS_Sync_Split_Demo_Path( &lks_merged.splits[i], path );
            sha = lks_merged.splits[i].sha;
        }
        else
        {
            const hsm_run_t * r = &lks_merged.runs[i - lks_merged.nsplits];
            if( ! memcmp( r->sha, lks_zero, 32 ) || ! HS_Sync_Run_Demo_Path( r, path ) )  continue;
            sha = r->sha;
        }
        if( file_sha( path, have ) && ! memcmp( have, sha, 32 ) )  continue;
        rd = lks_find_ready( sha );
        if( ! rd )  return;   // cannot happen: planned just now
        // Atomic, like every file here: a power cut leaves the old demo or the
        // new one, never half of either (M_Atomic_Write_Open_Binary).
        f = M_Atomic_Write_Open_Binary( path );
        if( ! f )  return;
        fwrite( rd->data, 1, rd->len, f );
        M_Atomic_Write_Close( f, path );
        demos++;
    }

    // A clear on the master reached this cabinet: the record demos it made
    // obsolete go too, exactly as clearhighscores removes them there.
    if( lks_merged.epoch > lks_raw.epoch )
    {
        DIR * dp = opendir( HS_Sync_Demo_Dir() );
        struct dirent * de;
        while( dp && (de = readdir( dp )) != NULL )
        {
            const char * ext = strrchr( de->d_name, '.' );
            boolean keep = false;
            char full[MAX_WADPATH];
            if( ! ext || strcasecmp( ext, ".lmp" ) )  continue;
            cat_filename( full, (char*) HS_Sync_Demo_Dir(), de->d_name );
            for( i = 0; i < lks_merged.nsplits && ! keep; i++ )
            {
                HS_Sync_Split_Demo_Path( &lks_merged.splits[i], path );
                keep = ! strcmp( path, full );
            }
            for( i = 0; i < lks_merged.nruns && ! keep; i++ )
                keep = HS_Sync_Run_Demo_Path( &lks_merged.runs[i], path ) && ! strcmp( path, full );
            if( ! keep && remove( full ) == 0 )  gone++;
        }
        if( dp )  closedir( dp );
        GenPrintf( EMSG_errlog, "LINKLOG Scores: %s cleared the high scores; %d demo(s) removed\n",
                   p->name, gone );
    }

    // Import only when the tables differ from what hs_stuff holds.  Compared
    // without the demo hashes, which it does not keep.
    {
        hsm_set_t a = lks_raw, b = lks_filtered;   // reuse the filtered set as scratch
        b.nsplits = b.nruns = 0;
        for( i = 0; i < lks_merged.nsplits; i++ )
            memset( lks_merged.splits[i].sha, 0, 32 );
        for( i = 0; i < lks_merged.nruns; i++ )
            memset( lks_merged.runs[i].sha, 0, 32 );
        a.max_splits = lks_raw.max_splits;
        a.max_runs = lks_raw.max_runs;
        if( HSM_Normalize( &a, &b ) != 0 )
            changed = true;
        else
            changed = ! HSM_Equal( &a, &lks_merged );
        lks_raw.nsplits = a.nsplits;   // Normalize worked in place on lks_raw's arrays
        lks_raw.nruns = a.nruns;
    }
    if( changed && ! HS_Sync_Import( &lks_merged ) )
    {
        GenPrintf( EMSG_errlog, "LINKLOG Scores: the merged tables from %s did not fit\n", p->name );
        changed = false;
    }
    if( changed || demos )
        GenPrintf( EMSG_errlog, "LINKLOG Scores: merged with %s: %d record(s) and %d board entr%s"
                   " now (was %d and %d), %d demo(s) received\n",
                   p->name, lks_merged.nsplits, lks_merged.nruns, lks_merged.nruns == 1 ? "y" : "ies",
                   before_s, before_r, demos );
    lks_free_ready();
}

// Where a peer stands after a manifest or a demo arrived, or a demo failed:
// fetch the next demo, or wait to apply.
static void  lks_next( lks_peer_t * p )
{
    byte missing[32];
    int need = lks_plan( p, missing );
    if( need < 0 )
    {
        GenPrintf( EMSG_errlog, "LINKLOG Scores: merging with %s ran out of room\n", p->name );
        p->phase = LKSP_IDLE;
        memcpy( p->done, p->their, 32 );
        return;
    }
    if( memcmp( missing, lks_zero, 32 ) )
    {
        memcpy( p->want, missing, 32 );
        free( p->buf );
        p->buf = NULL;
        p->total = p->got = 0;
        p->retries = 0;
        p->phase = LKSP_DEMO;
        lks_send_get( p, LKS_BLOB_DEMO );
        return;
    }
    p->phase = LKSP_APPLY;
    p->last_ms = 0;
}

static void  lks_reset( lks_peer_t * p )
{
    free( p->buf );
    p->buf = NULL;
    p->phase = LKSP_IDLE;
    p->total = p->got = 0;
}

// ---------------------------------------------------------------------------
//  Receiving

static lks_peer_t *  lks_peer_of( const byte * fp )
{
    int i;
    for( i = 0; i < LK_MAX_PEERS; i++ )
        if( lks_peer[i].used && ! memcmp( lks_peer[i].fp, fp, LK_FP_BYTES ) )  return &lks_peer[i];
    return NULL;
}

static void  lks_on_message( const lk_sync_msg_t * m )
{
    lks_peer_t * p = lks_peer_of( m->peer );
    const byte * d = m->data;
    if( ! p || m->len < 1 )  return;

    switch( d[0] )
    {
     case LKS_OFFER:
        if( m->len != 1 + 4 + 32 + 4 || get32( d + 1 ) != LKS_PROTO )  return;
        memcpy( p->their, d + 5, 32 );
        p->their_len = get32( d + 37 );
        p->retry_at = 0;   // a fresh offer: fetch it now (LKS_Ticker)
        if( ! memcmp( p->their, p->done, 32 ) )  return;              // seen it
        if( p->phase == LKSP_MANIFEST && ! memcmp( p->want, p->their, 32 ) )  return;   // fetching it
        lks_reset( p );    // a newer manifest replaces whatever was under way
        return;

     case LKS_GET:
        if( m->len != 1 + 1 + 32 + 4 + 1 )  return;
        lks_serve( m->peer, d[1], d + 2, get32( d + 34 ), d[38] );
        return;

     case LKS_NONE:
        if( m->len != 1 + 1 + 32 || memcmp( d + 2, p->want, 32 ) )  return;
        if( p->phase == LKSP_MANIFEST && d[1] == LKS_BLOB_MANIFEST )
            lks_reset( p );    // it moved on; its next offer says to what
        else if( p->phase == LKSP_DEMO && d[1] == LKS_BLOB_DEMO )
        {
            lks_reject( p, p->want, "no longer held" );
            lks_reset( p );
            lks_next( p );
        }
        return;

     case LKS_DATA:
     {
        uint32_t total, offset, n;
        byte sha[32];
        if( m->len < LKS_HDR || memcmp( d + 2, p->want, 32 ) )  return;
        if( ! ( (p->phase == LKSP_MANIFEST && d[1] == LKS_BLOB_MANIFEST)
                || (p->phase == LKSP_DEMO && d[1] == LKS_BLOB_DEMO) ) )  return;
        total = get32( d + 34 );
        offset = get32( d + 38 );
        n = m->len - LKS_HDR;
        if( total == 0 || total > (d[1] == LKS_BLOB_MANIFEST ? LKS_MANIFEST_MAX : LKS_DEMO_MAX) )
        {
            if( d[1] == LKS_BLOB_DEMO )  { lks_reject( p, p->want, "too big" ); lks_reset( p ); lks_next( p ); }
            else  lks_reset( p );
            return;
        }
        if( ! p->buf )
        {
            p->buf = malloc( total + 1 );
            if( ! p->buf )  { lks_reset( p ); return; }
            p->total = total;
            p->got = 0;
        }
        if( total != p->total || offset != p->got || offset + n > total )  return;   // stale or out of order
        memcpy( p->buf + offset, d + LKS_HDR, n );
        p->got += n;
        p->last_ms = lks_now();
        p->retries = 0;
        if( p->got < p->total )
        {
            if( p->got >= p->window_end )
                lks_send_get( p, d[1] );   // the window is used up: the next one
            return;
        }

        LK_Sha256( p->buf, p->total, sha );
        if( d[1] == LKS_BLOB_MANIFEST )
        {
            boolean ok = ! memcmp( sha, p->want, 32 );
            if( ok )
            {
                p->buf[p->total] = 0;
                ok = lks_parse( p, (char*) p->buf, p->total );
            }
            memcpy( p->done, p->want, 32 );   // good or bad, do not fetch it again
            lks_reset( p );
            if( ! ok )
            {
                GenPrintf( EMSG_errlog, "LINKLOG Scores: %s sent a manifest that did not read\n", p->name );
                return;
            }
            p->nrejected = 0;   // a new manifest: its demos get a fresh chance
            lks_next( p );
            return;
        }
        // A demo.
        if( memcmp( sha, p->want, 32 ) )
            lks_reject( p, p->want, "did not match its hash" );
        else if( ! demo_looks_whole( p->buf, p->total ) )
            lks_reject( p, p->want, "not a whole demo" );
        else if( lks_nready >= LKS_MAX_READY || lks_ready_bytes + p->total > LKS_READY_MAX )
            lks_reject( p, p->want, "too many demos at once" );
        else
        {
            lks_ready_t * rd = &lks_ready[lks_nready++];
            memcpy( rd->sha, sha, 32 );
            rd->data = p->buf;
            rd->len = p->total;
            lks_ready_bytes += p->total;
            p->buf = NULL;
        }
        lks_reset( p );
        lks_next( p );
        return;
     }
    }
}

// ---------------------------------------------------------------------------

void  LKS_Ticker( void )
{
    lk_peer_info_t  list[LK_MAX_PEERS];
    lk_sync_msg_t   m;
    int i, j, n;
    uint32_t now = lks_now();

    if( ! LK_My_Fp() || ! lks_alloc() )  return;

    // Peers come and go with the link.
    n = LK_Sync_Peers( list, LK_MAX_PEERS );
    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        lks_peer_t * p = &lks_peer[i];
        boolean still = false;
        if( ! p->used )  continue;
        for( j = 0; j < n; j++ )
            if( ! memcmp( list[j].fp, p->fp, LK_FP_BYTES ) )  still = true;
        if( ! still )
        {
            hsm_set_t keep = p->remote;   // keep its arrays for the next peer in this slot
            lks_reset( p );
            memset( p, 0, sizeof(*p) );
            p->remote = keep;
        }
    }
    for( j = 0; j < n; j++ )
    {
        if( lks_peer_of( list[j].fp ) )  continue;
        for( i = 0; i < LK_MAX_PEERS; i++ )
            if( ! lks_peer[i].used )
            {
                lks_peer_t * p = &lks_peer[i];
                hsm_set_t keep = p->remote;
                memset( p, 0, sizeof(*p) );
                p->remote = keep;
                p->used = true;
                memcpy( p->fp, list[j].fp, LK_FP_BYTES );
                dl_strncpy( p->name, list[j].name, LK_NAME_LEN );
                break;
            }
    }

    while( LK_Sync_Poll( &m ) )
    {
        // [Arcade] Copy Missing Wads shares the sync channel (d_linksel.c).
        if( m.len >= 1 && m.data[0] >= LKSEL_SYNC_FIRST )
            LKSEL_On_Sync( &m );
        else
            lks_on_message( &m );
    }

    lks_build_manifest();

    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        lks_peer_t * p = &lks_peer[i];
        if( ! p->used )  continue;

        // Offer what we hold when it changed, or as a backstop.
        if( lks_man_valid && ( memcmp( p->offered, lks_man_sha, 32 )
                               || lks_since( now, p->offer_ms ) > LKS_REOFFER_MS ) )
            lks_send_offer( p );

        // A manifest it offered that we have not read yet.
        if( p->phase == LKSP_IDLE && memcmp( p->their, lks_zero, 32 ) && memcmp( p->their, p->done, 32 )
            && now >= p->retry_at && p->their_len > 0 && p->their_len <= LKS_MANIFEST_MAX )
        {
            memcpy( p->want, p->their, 32 );
            p->got = p->total = 0;
            p->retries = 0;
            p->phase = LKSP_MANIFEST;
            lks_send_get( p, LKS_BLOB_MANIFEST );
        }

        // A transfer that went quiet: ask again, then give up on it.
        if( ( p->phase == LKSP_MANIFEST || p->phase == LKSP_DEMO ) && lks_since( now, p->last_ms ) > LKS_STALL_MS )
        {
            if( ++p->retries > LKS_RETRIES )
            {
                GenPrintf( EMSG_errlog, "LINKLOG Scores: %s stopped answering\n", p->name );
                if( p->phase == LKSP_DEMO )
                {
                    // Go on without that demo (and so without its record).
                    lks_reject( p, p->want, "timed out" );
                    lks_reset( p );
                    lks_next( p );
                }
                else
                {
                    lks_reset( p );
                    p->retry_at = now + LKS_RETRY_MS;
                }
                continue;
            }
            lks_send_get( p, p->phase == LKSP_MANIFEST ? LKS_BLOB_MANIFEST : LKS_BLOB_DEMO );
        }

        // Apply when nobody is mid-run, signing initials, or in a game.
        if( p->phase == LKSP_APPLY && lks_since( now, p->last_ms ) > LKS_APPLY_POLL_MS )
        {
            lk_state_e st = LK_State();
            p->last_ms = now;
            if( HS_Sync_Busy() || ( st != LK_STATE_IDLE && st != LK_STATE_MENU ) )
                continue;
            {
                byte missing[32];
                int need = lks_plan( p, missing );   // the local scores may have moved on
                if( need < 0 )  { p->phase = LKSP_IDLE; continue; }
                if( memcmp( missing, lks_zero, 32 ) )  { lks_next( p ); continue; }
            }
            lks_apply( p );
            p->phase = LKSP_IDLE;
            memcpy( p->done, p->their, 32 );
            lks_man_valid = false;   // offer the result straight away
        }
    }
}

static const char *  lks_phase_name( lks_phase_e ph )
{
    static const char * names[] = { "idle", "manifest", "demo", "apply" };
    return names[ph];
}

void  LKS_Status_Print( void )
{
    int i;
    for( i = 0; i < LK_MAX_PEERS; i++ )
    {
        const lks_peer_t * p = &lks_peer[i];
        if( ! p->used )  continue;
        GenPrintf( EMSG_errlog, "LINKSCORE peer=%s phase=%s scope=%d records=%d/%d epoch=%u status=%s\n",
                   p->name, lks_phase_name( p->phase ), p->in_scope,
                   p->have_remote ? p->remote.nsplits : -1, p->have_remote ? p->remote.nruns : -1,
                   (unsigned) ( p->have_remote ? p->remote.epoch : 0 ), p->status[0] ? p->status : "-" );
    }
}

const char *  LKS_Peer_Status( const byte * fp )
{
    lks_peer_t * p = lks_peer_of( fp );
    return ( p && p->have_remote ) ? p->status : "";
}
