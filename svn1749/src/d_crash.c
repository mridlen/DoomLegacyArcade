// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by Doom Legacy Arcade.
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
// DESCRIPTION:
//      [Arcade] Crash report and class-list black box.
//
//      Written after a Raspberry Pi cabinet segfaulted in PIT_FindTarget,
//      half an hour into an untouched attract cycle, on a friends-list entry
//      whose memory had been freed and reused by a door.  The core dump said
//      where it fell over and nothing about who had put that entry in the
//      list, and the demo it was playing had to be recovered from header
//      bytes because 'demoname' is cut at 32 characters.  This records the
//      two things that were missing, and prints them when it happens.
//
//      See docs/arcade/crash-diagnostics.md.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "p_mobj.h"
#include "m_misc.h"
#include "d_crash.h"

#include <string.h>
#include <time.h>

#if defined(__GLIBC__)
# define CRASH_HANDLER
# include <signal.h>
# include <unistd.h>
# include <fcntl.h>
# include <execinfo.h>
#endif


// The demo playing, with its full path.
static char  crash_demo[ MAX_WADPATH ];

// Last friends-list additions, oldest overwritten.  Friends are rare in a
// single-player game (the player below half health, MBF friendly monsters),
// so 32 covers a long stretch of play.
#define FRIEND_RING  32
typedef struct {
    tic_t   gametic, leveltime;
    void *  thinker;
    void *  caller;
    int     type;     // -1 when not an object thinker
    int     health;
    byte    function;
} friend_rec_t;

static friend_rec_t  friend_ring[ FRIEND_RING ];
static unsigned int  friend_count = 0;   // total ever recorded

// Anomalies logged, capped so a corrupt list cannot flood the terminal.
#define ANOMALY_LOG_MAX  20
static unsigned int  anomaly_count = 0;


void  D_Crash_Set_Demo( const char * path )
{
    dl_strncpy( crash_demo, path ? path : "", sizeof(crash_demo) );
    // EMSG_errlog: the terminal and -logfile, not the in-game console.
    GenPrintf( EMSG_errlog, "Demo file: %s\n", crash_demo );
}


void  D_Crash_Friend_Added( thinker_t * th, void * caller )
{
    friend_rec_t * r = & friend_ring[ friend_count % FRIEND_RING ];

    r->gametic = gametic;
    r->leveltime = leveltime;
    r->thinker = th;
    r->caller = caller;
    r->function = th->function;
    if( th->function == TFI_MobjThinker )
    {
        mobj_t * mo = (mobj_t *) th;
        r->type = mo->type;
        r->health = mo->health;
    }
    else
    {
        r->type = -1;
        r->health = 0;
    }
    friend_count++;
}


int  D_Crash_Not_Object( thinker_t * th )
{
    switch( th->function )
    {
     case TFI_MobjThinker:
     case TFI_MobjNullThinker:
     case TFI_BlasterMobjThinker:
        return 0;
    }
    return 1;
}


// The caller as "binary(+0xoffset)", which addr2line resolves to a file and
// line: addr2line -f -e doomlegacyarcade 0xoffset.
static void  describe_caller( void * caller, char * buf, size_t len )
{
#ifdef CRASH_HANDLER
    char ** s = backtrace_symbols( &caller, 1 );
    if( s )
    {
        dl_strncpy( buf, s[0], len );
        free( s );
        return;
    }
#endif
    snprintf( buf, len, "%p", caller );
}

void  D_Crash_Class_Anomaly( const char * what, thinker_t * th, void * caller )
{
    char  cbuf[ 256 ];

    anomaly_count++;
    if( anomaly_count > ANOMALY_LOG_MAX )  return;

    describe_caller( caller, cbuf, sizeof(cbuf) );
    GenPrintf( EMSG_warn,
       "CLASS-LIST ANOMALY: %s on thinker %p (function %d, not an object)"
       " gametic %u leveltime %u map %d/%d, called from %s\n",
       what, (void*)th, th->function, (unsigned)gametic, (unsigned)leveltime,
       gameepisode, gamemap, cbuf );
    if( th->function == TFI_RemoveThinker )
    {
        // Removed but not yet freed: the object is still intact, so say what
        // it was.  Something still holds a pointer to it -- with no reference
        // counting (REFERENCE_COUNTING is off), usually a 'target'.
        mobj_t * mo = (mobj_t *) th;
        GenPrintf( EMSG_warn,
           "CLASS-LIST ANOMALY:   it is a removed object: type %d health %d\n",
           mo->type, mo->health );
    }
    if( anomaly_count == ANOMALY_LOG_MAX )
        GenPrintf( EMSG_warn, "CLASS-LIST ANOMALY: no more of these will be logged\n" );
}


#ifdef CRASH_HANDLER
// ---------------------------------------------------------------------------
//  The signal handler.  Only async-signal-safe calls from here down: write,
//  open, close, time, raise, and backtrace, which is primed at install so it
//  does not load libgcc (and malloc) for the first time inside the handler.
// ---------------------------------------------------------------------------

static char  crash_path[ MAX_WADPATH ];
static int   crash_fds[2] = { 2, -1 };   // stderr, crash.txt
static int   crash_nfds = 1;

static void  cw_str( const char * s )
{
    size_t n = strlen( s );
    int i;
    for( i = 0; i < crash_nfds; i++ )
        if( write( crash_fds[i], s, n ) < 0 )  {}
}

static void  cw_num( unsigned long v )
{
    char  buf[24];
    int   p = sizeof(buf) - 1;
    buf[p] = 0;
    do { buf[--p] = '0' + (v % 10); v /= 10; } while( v && p > 0 );
    cw_str( &buf[p] );
}

static void  cw_int( long v )
{
    if( v < 0 )  { cw_str( "-" ); cw_num( (unsigned long)(-v) ); }
    else  cw_num( (unsigned long)v );
}

static void  cw_hex( unsigned long v )
{
    static const char  hx[] = "0123456789abcdef";
    char  buf[24];
    int   p = sizeof(buf) - 1;
    buf[p] = 0;
    do { buf[--p] = hx[v & 15]; v >>= 4; } while( v && p > 2 );
    buf[--p] = 'x';
    buf[--p] = '0';
    cw_str( &buf[p] );
}

static void  cw_symbols( void ** addrs, int n )
{
    int i;
    for( i = 0; i < crash_nfds; i++ )
        backtrace_symbols_fd( addrs, n, crash_fds[i] );
}

static const char *  signal_name( int sig )
{
    switch( sig )
    {
     case SIGSEGV: return "SIGSEGV";
     case SIGBUS:  return "SIGBUS";
     case SIGILL:  return "SIGILL";
     case SIGFPE:  return "SIGFPE";
     case SIGABRT: return "SIGABRT";
    }
    return "signal";
}

static void  crash_handler( int sig )
{
    void *  frames[64];
    int     nframes, i;
    unsigned int  first, n;

    crash_nfds = 1;
    if( crash_path[0] )
    {
        crash_fds[1] = open( crash_path, O_WRONLY | O_CREAT | O_APPEND, 0644 );
        if( crash_fds[1] >= 0 )  crash_nfds = 2;
    }

    cw_str( "\n==== CRASH: " );
    cw_str( VERSION_BANNER );
    cw_str( ", " );
    cw_str( signal_name( sig ) );
    cw_str( " (" );  cw_int( sig );
    cw_str( ") at unix time " );  cw_num( (unsigned long) time( NULL ) );
    cw_str( "\ngametic " );  cw_num( gametic );
    cw_str( "  leveltime " );  cw_num( leveltime );
    cw_str( "  gamestate " );  cw_int( gamestate );
    cw_str( "  episode " );  cw_int( gameepisode );
    cw_str( "  map " );  cw_int( gamemap );
    cw_str( "  skill " );  cw_int( gameskill );
    cw_str( "  demoplayback " );  cw_int( demoplayback );
    cw_str( "\ndemo file: " );
    cw_str( crash_demo[0] ? crash_demo : "(none)" );

    cw_str( "\nbacktrace (resolve with: addr2line -f -i -e doomlegacyarcade ADDR,\n  where ADDR is the +0x offset when one is shown, else the [0x...] address):\n" );
    nframes = backtrace( frames, 64 );
    cw_symbols( frames, nframes );

    n = (friend_count < FRIEND_RING)? friend_count : FRIEND_RING;
    first = friend_count - n;
    cw_str( "friends-list additions, oldest first (" );
    cw_num( friend_count );
    cw_str( " this session):\n" );
    for( i = 0; i < (int)n; i++ )
    {
        friend_rec_t * r = & friend_ring[ (first + i) % FRIEND_RING ];
        cw_str( "  gametic " );  cw_num( r->gametic );
        cw_str( " leveltime " );  cw_num( r->leveltime );
        cw_str( " thinker " );  cw_hex( (unsigned long) r->thinker );
        cw_str( " function " );  cw_int( r->function );
        cw_str( " type " );  cw_int( r->type );
        cw_str( " health " );  cw_int( r->health );
        cw_str( " from " );
        cw_symbols( &r->caller, 1 );   // ends in its own newline
    }
    cw_str( "class-list anomalies this session: " );
    cw_num( anomaly_count );
    cw_str( "\n==== end of crash report\n" );

    if( crash_nfds == 2 )  close( crash_fds[1] );

    // SA_RESETHAND put the default action back: re-raise, so the process dies
    // of the original signal and the core dump is written exactly as before.
    raise( sig );
}

void  D_Crash_Init( void )
{
    static byte  altstack[ 64 * 1024 ];
    static const int  sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
    struct sigaction  sa;
    stack_t  ss;
    void *  prime[1];
    int  i;

    if( legacyhome )
        cat_filename( crash_path, legacyhome, "crash.txt" );

    // Load libgcc's unwinder now, not inside the handler.
    backtrace( prime, 1 );

    // A stack overflow would otherwise leave the handler no stack to run on.
    ss.ss_sp = altstack;
    ss.ss_size = sizeof(altstack);
    ss.ss_flags = 0;
    sigaltstack( &ss, NULL );

    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = crash_handler;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = SA_RESETHAND | SA_ONSTACK;
    for( i = 0; i < (int)(sizeof(sigs)/sizeof(sigs[0])); i++ )
        sigaction( sigs[i], &sa, NULL );

    GenPrintf( EMSG_ver, "Crash report: %s\n", crash_path );
}

#else

void  D_Crash_Init( void )
{
    // No handler on this platform yet; the black box and demo line still work.
}

#endif
