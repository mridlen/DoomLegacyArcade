// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by DoomLegacy Arcade contributors.
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
//-----------------------------------------------------------------------------
//
// [Arcade] Render worker threads for the software renderer.
//
// The cabinet draws up to four viewports per frame, one per panel, and each
// one writes into its own cell of the screen.  They are independent -- nothing
// view 2 draws can be seen by view 1 -- so they can be drawn at the same time
// on different cores, which is what this does: the main thread takes view 0
// and a worker takes each of the rest.
//
// What makes that possible is not this file.  It is that every mutable global
// the software renderer writes while drawing a frame is now R_TLS
// (thread-local), so each thread has its own copy and the renderer itself
// needed almost no changes.  The rule for which globals may be marked, and
// which must NOT be, is in docs/arcade/render-threads.md.  Getting one wrong
// in either direction fails a long way from here.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "r_threads.h"

#ifdef RENDER_THREADS

#include "r_main.h"
#include "r_draw.h"
#include "d_player.h"
#include "i_system.h"

#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <SDL_cpuinfo.h>

// One fewer than the number of views, because the main thread draws one.
#define MAX_RENDER_WORKERS   (MAXSPLITSCREENPLAYERS - 1)

CV_PossibleValue_t render_threads_cons_t[] = {
    {0, "Auto"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}, {0, NULL}
};

// Default 1 -- the stock serial renderer.  Threading is opt-in until it has
// been played on the cabinet; see docs/arcade/render-threads.md.
consvar_t  cv_render_threads = { "render_threads", "1", CV_SAVE,
                                 render_threads_cons_t, NULL };

typedef struct
{
    SDL_Thread *  thread;
    SDL_sem    *  go;          // posted to give this worker a view
    byte          index;       // 1 .. MAX_RENDER_WORKERS
    byte          vind;        // view to draw
    player_t   *  vpl;         // player of that view
    int           bx1, bx2;    // column band, or 0,0 for the whole view
    boolean       busy;        // submitted and not yet waited for
} render_worker_t;

static render_worker_t  worker[MAX_RENDER_WORKERS];
static SDL_sem *  worker_done = NULL;   // posted once per finished view
static byte       num_workers = 0;      // threads actually created
static byte       num_submitted = 0;    // views handed out this frame
static volatile boolean  threads_quit = false;
static SDL_mutex *  cache_mutex = NULL;

// Set by the main thread around the parallel section only.  It is written
// before the workers are posted and cleared after they are joined, and the
// semaphores order those against the workers' reads, so a plain flag is
// enough -- no atomics needed.
volatile boolean  r_threads_active = false;


// Relaxed atomics: the ordering that matters comes from the semaphores, but
// the flag itself is read by the workers while the main thread sets it, so it
// has to be a defined access rather than a plain one.
void  R_Cache_Lock( void )
{
    if( __atomic_load_n( &r_threads_active, __ATOMIC_RELAXED ) )
        SDL_LockMutex( cache_mutex );
}

void  R_Cache_Unlock( void )
{
    if( __atomic_load_n( &r_threads_active, __ATOMIC_RELAXED ) )
        SDL_UnlockMutex( cache_mutex );
}

// Which thread we are on.  0 is the main thread, which is what every
// non-threaded caller sees, so R_Thread_Index() is safe to call anywhere.
static R_TLS byte  this_thread_index = 0;


boolean  R_On_Render_Worker( void )
{
    return (this_thread_index != 0);
}

byte  R_Thread_Index( void )
{
    return this_thread_index;
}


// How many workers the operator setting asks for right now, clamped to what
// exists.  Read per frame: the cvar can change between frames, and the pool
// is created once at its full size, so a lower setting simply leaves the
// spare workers idle rather than tearing threads down.
byte  R_Thread_Workers( void )
{
    int  want = cv_render_threads.value;   // .value, not .EV -- 0 means Auto

    if( num_workers == 0 )  return 0;      // pool not up

    if( want <= 0 )
        want = SDL_GetCPUCount();          // Auto: one thread per core

    want -= 1;   // the main thread draws a view too

    if( want < 0 )  want = 0;
    if( want > num_workers )  want = num_workers;
    return (byte) want;
}


static int  R_Worker_Main( void * arg )
{
    render_worker_t * w = (render_worker_t*) arg;

    this_thread_index = w->index;

    for(;;)
    {
        SDL_SemWait( w->go );
        if( threads_quit )  break;

        // Place this thread's own draw tables on the view's cell, then draw
        // it.  Both write thread-local state only, so nothing here is shared
        // with the main thread or with another worker.
        R_Set_View_Window( w->vind );
        // A band job draws one vertical slice of the view; a view job draws
        // the lot.  R_Set_View_Window has just reset the range to the whole
        // view, so this must come after it.
        if( w->bx2 > w->bx1 )
            R_Set_Render_Band( w->bx1, w->bx2 );
        R_RenderPlayerView( w->vind, w->vpl );

        SDL_SemPost( worker_done );
    }
    return 0;
}


void  R_Threads_Init( void )
{
    byte i;

    if( num_workers )  return;   // already up

    cache_mutex = SDL_CreateMutex();   // recursive, so nesting is safe
    worker_done = SDL_CreateSemaphore(0);
    if( ! worker_done || ! cache_mutex )
    {
        GenPrintf( EMSG_warn,
                   "Render threads: no semaphore (%s); drawing serially.\n",
                   SDL_GetError() );
        return;
    }

    threads_quit = false;

    for( i = 0; i < MAX_RENDER_WORKERS; i++ )
    {
        char name[16];

        worker[i].index = i + 1;
        worker[i].busy = false;
        worker[i].go = SDL_CreateSemaphore(0);
        if( ! worker[i].go )  break;

        snprintf( name, sizeof(name), "dl_render%d", i + 1 );
        worker[i].thread = SDL_CreateThread( R_Worker_Main, name, &worker[i] );
        if( ! worker[i].thread )
        {
            SDL_DestroySemaphore( worker[i].go );
            worker[i].go = NULL;
            break;
        }
        num_workers++;
    }

    GenPrintf( EMSG_info, "Render threads: %d worker%s, %d cores.\n",
               num_workers, (num_workers == 1) ? "" : "s", SDL_GetCPUCount() );
}


void  R_Threads_Shutdown( void )
{
    byte i;

    if( ! num_workers )  return;

    threads_quit = true;
    for( i = 0; i < num_workers; i++ )
        SDL_SemPost( worker[i].go );        // wake it so it can see the flag

    for( i = 0; i < num_workers; i++ )
    {
        SDL_WaitThread( worker[i].thread, NULL );
        SDL_DestroySemaphore( worker[i].go );
        worker[i].go = NULL;
        worker[i].thread = NULL;
    }
    num_workers = 0;

    SDL_DestroySemaphore( worker_done );
    worker_done = NULL;
    SDL_DestroyMutex( cache_mutex );
    cache_mutex = NULL;
}


boolean  R_Thread_Submit_View( byte vind, player_t * vpl )
{
    byte  avail = R_Thread_Workers();
    byte  i;

    if( num_submitted >= avail )  return false;   // draw it on the main thread

    for( i = 0; i < num_workers; i++ )
    {
        if( worker[i].busy )  continue;

        worker[i].vind = vind;
        worker[i].vpl  = vpl;
        worker[i].bx1  = 0;      // whole view
        worker[i].bx2  = 0;
        worker[i].busy = true;
        num_submitted++;
        __atomic_store_n( &r_threads_active, true, __ATOMIC_RELAXED );
        SDL_SemPost( worker[i].go );
        return true;
    }
    return false;
}


// [Arcade] Hand one column band of a view to a worker.
boolean  R_Thread_Submit_Band( byte vind, player_t * vpl, int x1, int x2 )
{
    byte  avail = R_Thread_Workers();
    byte  i;

    if( num_submitted >= avail )  return false;
    if( x2 <= x1 )  return false;

    for( i = 0; i < num_workers; i++ )
    {
        if( worker[i].busy )  continue;

        worker[i].vind = vind;
        worker[i].vpl  = vpl;
        worker[i].bx1  = x1;
        worker[i].bx2  = x2;
        worker[i].busy = true;
        num_submitted++;
        __atomic_store_n( &r_threads_active, true, __ATOMIC_RELAXED );
        SDL_SemPost( worker[i].go );
        return true;
    }
    return false;
}


void  R_Threads_Wait( void )
{
    byte i;

    while( num_submitted )
    {
        SDL_SemWait( worker_done );
        num_submitted--;
    }
    for( i = 0; i < num_workers; i++ )
        worker[i].busy = false;
    __atomic_store_n( &r_threads_active, false, __ATOMIC_RELAXED );
}

#endif  // RENDER_THREADS
