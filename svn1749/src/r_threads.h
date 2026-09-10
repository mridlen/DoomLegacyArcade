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
// See docs/arcade/render-threads.md before changing anything here.
//
//-----------------------------------------------------------------------------

#ifndef R_THREADS_H
#define R_THREADS_H

#include "doomtype.h"
#include "command.h"
#include "z_zone.h"
  // PU_LUMP, PU_CACHE, for R_DRAW_LUMP_TAG

struct player_s;

#ifdef RENDER_THREADS

// Operator setting: how many cores the software renderer may use.
// "Auto" follows the CPU count; 1 is the stock serial renderer.
extern consvar_t  cv_render_threads;

void   R_Threads_Init( void );
void   R_Threads_Shutdown( void );

// How many worker threads are actually running and wanted right now.
// 0 means render everything on the calling thread, exactly as before.
byte   R_Thread_Workers( void );

// True while called from a render worker.  The play-side bookkeeping that
// R_RenderPlayerView does on the way past -- NetUpdate in particular -- must
// only happen on the main thread.
boolean  R_On_Render_Worker( void );

// A worker's index, 1 .. R_Thread_Workers(); 0 on the main thread.  Used to
// index the per-thread scratch that cannot be thread_local because something
// outside the renderer has to reach it.
byte   R_Thread_Index( void );

// Hand view 'vind' to a worker.  False means no worker took it and the caller
// must render it itself.
boolean  R_Thread_Submit_View( byte vind, struct player_s * vpl );

// Hand one column band of a view to a worker, for the case where there is
// only one view and per-view splitting has nothing to divide.
boolean  R_Thread_Submit_Band( byte vind, struct player_s * vpl,
                               int x1, int x2 );

// Block until every submitted view has been drawn.  Always call it, even when
// nothing was submitted.
void   R_Threads_Wait( void );

// [Arcade] Run fn( part, nparts, ctx ) for every part 0 .. nparts-1 -- part 0
// on the calling thread, the rest on idle workers -- and return once all of
// them have finished.  For whole-frame work outside the view drawing: the
// draw8bpp expansion at present time.
//
// nparts follows R_Thread_Workers(), so at render_threads 1 it is 1 and fn
// runs inline exactly as a plain call would.  It is also 1 if views are
// still in flight, because the workers are not free.  Deliberately NOT
// R_Threads_Wait: that also releases the lumps the drawers pinned, which is
// only right at the end of the view drawing.
typedef void (*r_parallel_fn_t)( int part, int nparts, void * ctx );
void   R_Threads_Parallel( r_parallel_fn_t fn, void * ctx );

// True only while workers are dispatched.  Read by the cache lock below so
// that everything outside a threaded frame -- startup, level load, the whole
// play side -- pays a single boolean test and never touches a mutex.
extern volatile boolean  r_threads_active;

// Guard for state shared between render threads that is NOT thread-local:
// the zone allocator and the lump cache.  W_CacheLumpNum mutates the cache on
// every call, hit or miss (it re-tags the block), and R_GetFlat calls it once
// per visplane from every thread -- so this is a live race on every threaded
// frame, not a theoretical one.  Taken per visplane / per wall / per lump, a
// long way outside the per-pixel loops, so the cost does not show up.
void   R_Cache_Lock( void );
void   R_Cache_Unlock( void );

// True while the workers are dispatched, for code that has to behave
// differently inside the parallel section.  See R_DRAW_LUMP_TAG.
#define R_Threads_Drawing()     (r_threads_active)

#else

#define R_Thread_Workers()      0
#define R_On_Render_Worker()    false
#define R_Thread_Index()        0
#define R_Threads_Init()        do {} while(0)
#define R_Threads_Shutdown()    do {} while(0)
#define R_Threads_Wait()        do {} while(0)
typedef void (*r_parallel_fn_t)( int part, int nparts, void * ctx );
#define R_Threads_Parallel( fn, ctx )   ((fn)( 0, 1, (ctx) ))
#define R_Cache_Lock()          do {} while(0)
#define R_Cache_Unlock()        do {} while(0)
#define R_Threads_Drawing()     false

#endif  // RENDER_THREADS


// [Arcade] The tag a drawer caches a lump under while it is reading from it.
//
// Serially, PU_CACHE is safe: the drawer is the only thing running, so
// nothing can allocate -- and therefore nothing can purge the block -- between
// caching the lump and finishing with it.  With workers running that is no
// longer true.  Any other thread's Z_Malloc purges PU_CACHE blocks to make
// room, and it will happily take the flat or the sprite patch that this thread
// is in the middle of drawing from.
//
// So inside the parallel section a drawer pins its lump instead, and
// R_Threads_Wait lets every pinned lump go again once the last worker has
// stopped reading.  Outside it, the tag is PU_CACHE exactly as before.
// See docs/arcade/render-threads.md.
#define R_DRAW_LUMP_TAG   ( R_Threads_Drawing() ? PU_LUMP : PU_CACHE )

#endif  // R_THREADS_H
