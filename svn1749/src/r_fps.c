// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1999-2000 by Jess Haas, Nicolas Kalkhof, Colin Phipps,
//   Florian Schulze, Andrey Budko  (PrBoom / PrBoom-plus)
// Copyright (C) 2021 by Ryan Krafnick  (dsda-doom)
// Copyright (C) 2026 by DoomLegacy Arcade
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      [Arcade] Uncapped framerate: render-time interpolation.  See r_fps.h
//      for what this is and why the simulation is untouched by it.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "r_fps.h"
#include "r_state.h"
#include "p_spec.h"
#include "p_local.h"
#include "g_game.h"
#include "z_zone.h"


// [Arcade] Frames per second to aim for.  "Uncapped" (0) draws as fast as the
// machine will go, which on this cabinet meant ~600fps and a lot of wasted
// electricity for a 60Hz panel -- so the list is the common refresh rates and
// the default is the commonest of them.
//
// 35 is special: it is the tic rate, so it means one frame per tic, which is
// the stock engine exactly.  Interpolation is switched off at that setting
// rather than interpolating to a whole tic, since that would buy a tic of
// display lag for a picture no different from not interpolating at all.
CV_PossibleValue_t framerate_cap_cons_t[] = {
    {  0, "Uncapped"},
    { 35, "35"},
    { 60, "60"},
    { 75, "75"},
    {100, "100"},
    {120, "120"},
    {144, "144"},
    {165, "165"},
    {240, "240"},
    {  0, NULL}
};

consvar_t  cv_framerate_cap = { "framerate_cap", "60", CV_SAVE, framerate_cap_cons_t };

fixed_t  rendertic_frac = FRACUNIT;
boolean  interp_active = false;

// Set by R_Interp_Reset_View, consumed by the next frame: draw at the true
// position once, then resume interpolating from there.
static boolean  reset_view = true;

// True between a Frame_Begin that actually interpolated and its Frame_End,
// so End knows whether there is anything to put back.  Guards against a
// double End, and against restoring stale values from an earlier frame.
static boolean  frame_interpolated = false;

// Flipped once per tic; see R_Interp_Capture_Mobj.
static byte  interp_tic_parity = 0;


// =========================================================================
//   The moving-world registry
// =========================================================================
//
// Sector heights and texture panning are interpolated by *overwriting* the
// live value for the duration of the frame, so the plane and wall drawers
// need no changes.  This is the table of what is currently moving, built as
// the mover thinkers run and torn down as they finish.

typedef enum
{
    INTERP_sector_floor,
    INTERP_sector_ceiling,
    INTERP_floor_panning,
    INTERP_ceiling_panning,
    INTERP_wall_panning,
} interp_type_e;

typedef struct
{
    interp_type_e  type;
    void *         address;   // sector_t* or side_t*
} interp_t;

typedef fixed_t  fixed2_t[2];

static fixed2_t *  oldpos;    // value at the start of the current tic
static fixed2_t *  bakpos;    // the real value, saved across the frame
static interp_t *  curpos;
static int  num_interp = 0;
static int  max_interp = 0;


// Where a registration records its own slot, so removing one is O(1).
// Holds index+1; 0 means "not registered".
static int * slot_of( interp_type_e type, void * addr )
{
    switch( type )
    {
      case INTERP_sector_floor:
        return & ((sector_t*)addr)->interp_slot[0];
      case INTERP_sector_ceiling:
        return & ((sector_t*)addr)->interp_slot[1];
      case INTERP_floor_panning:
        return & ((sector_t*)addr)->interp_slot[2];
      case INTERP_ceiling_panning:
        return & ((sector_t*)addr)->interp_slot[3];
      case INTERP_wall_panning:
        return & ((side_t*)addr)->interp_slot;
    }
    return NULL;
}

// The one or two live fields a registration covers.  Returning pointers
// keeps capture, interpolate and restore from each needing their own copy of
// this switch -- three copies that must agree is how PrBoom's version reads,
// and it is the obvious place for them to drift apart.
static void  interp_fields( int i, fixed_t ** f1, fixed_t ** f2 )
{
    void * ad = curpos[i].address;

    *f1 = NULL;
    *f2 = NULL;
    switch( curpos[i].type )
    {
      case INTERP_sector_floor:
        *f1 = & ((sector_t*)ad)->floorheight;
        break;
      case INTERP_sector_ceiling:
        *f1 = & ((sector_t*)ad)->ceilingheight;
        break;
      case INTERP_floor_panning:
        *f1 = & ((sector_t*)ad)->floor_xoffs;
        *f2 = & ((sector_t*)ad)->floor_yoffs;
        break;
      case INTERP_ceiling_panning:
        *f1 = & ((sector_t*)ad)->ceiling_xoffs;
        *f2 = & ((sector_t*)ad)->ceiling_yoffs;
        break;
      case INTERP_wall_panning:
        *f1 = & ((side_t*)ad)->textureoffset;
        *f2 = & ((side_t*)ad)->rowoffset;
        break;
    }
}

// Remember where this thing is now; the next frame interpolates from here.
static void  interp_capture( int i )
{
    fixed_t * f1;
    fixed_t * f2;

    interp_fields( i, &f1, &f2 );
    if( f1 )  oldpos[i][0] = *f1;
    if( f2 )  oldpos[i][1] = *f2;
}

static void  interp_register( interp_type_e type, void * addr )
{
    int * sp;

    if( !addr )
        return;

    sp = slot_of( type, addr );
    if( !sp || *sp )   // already registered
        return;

    if( num_interp >= max_interp )
    {
        int  newmax = max_interp ? (max_interp * 2) : 256;
        fixed2_t * n_old = realloc( oldpos, sizeof(fixed2_t) * newmax );
        fixed2_t * n_bak = realloc( bakpos, sizeof(fixed2_t) * newmax );
        interp_t * n_cur = realloc( curpos, sizeof(interp_t) * newmax );

        // Out of memory: keep whatever we already had and stop growing.  The
        // movers past this point simply are not interpolated, which is a
        // cosmetic loss, not a failure worth taking the game down for.
        if( !n_old || !n_bak || !n_cur )
        {
            if( n_old )  oldpos = n_old;
            if( n_bak )  bakpos = n_bak;
            if( n_cur )  curpos = n_cur;
            return;
        }
        oldpos = n_old;
        bakpos = n_bak;
        curpos = n_cur;
        max_interp = newmax;
    }

    curpos[num_interp].type = type;
    curpos[num_interp].address = addr;
    interp_capture( num_interp );
    num_interp++;
    *sp = num_interp;   // index + 1
}

static void  interp_unregister( interp_type_e type, void * addr )
{
    int * sp;
    int * lastsp;
    int   i;

    if( !addr )
        return;

    sp = slot_of( type, addr );
    if( !sp || !*sp )
        return;

    i = *sp - 1;
    num_interp--;

    // Move the last entry down into the hole, and tell it where it went.
    if( i != num_interp )
    {
        oldpos[i][0] = oldpos[num_interp][0];
        oldpos[i][1] = oldpos[num_interp][1];
        bakpos[i][0] = bakpos[num_interp][0];
        bakpos[i][1] = bakpos[num_interp][1];
        curpos[i] = curpos[num_interp];

        lastsp = slot_of( curpos[i].type, curpos[i].address );
        if( lastsp )
            *lastsp = i + 1;
    }
    *sp = 0;
}

// [Arcade] Called per level: the old table points at the previous level's
// sectors, which have been freed.  Everything must go.
void  R_Interp_Level_Init(void)
{
    int i;

    num_interp = 0;
    frame_interpolated = false;
    reset_view = true;

    for( i = 0; i < numsectors; i++ )
    {
        sectors[i].interp_slot[0] = 0;
        sectors[i].interp_slot[1] = 0;
        sectors[i].interp_slot[2] = 0;
        sectors[i].interp_slot[3] = 0;
    }
    for( i = 0; i < numsides; i++ )
        sides[i].interp_slot = 0;

    // A level can start with movers already running (a door left open by a
    // voodoo doll, scrollers, a perpetual platform), and those thinkers were
    // created before this ran.  Pick them up now rather than waiting for the
    // one-off P_AddThinker notification that has already been and gone.
    {
        sector_t * sec = sectors;

        for( i = 0; i < numsectors; i++, sec++ )
        {
            if( sec->floordata )
                interp_register( INTERP_sector_floor, sec );
            if( sec->ceilingdata )
                interp_register( INTERP_sector_ceiling, sec );
        }
    }
}


// =========================================================================
//   Thinker hooks -- the THINKER_INTERPOLATIONS calls in p_tick.c
// =========================================================================

// What, if anything, does this thinker move that the renderer can smooth?
static void  interp_data_of( thinker_t * th,
                             interp_type_e * type1, void ** addr1,
                             interp_type_e * type2, void ** addr2 )
{
    *addr1 = NULL;
    *addr2 = NULL;

    switch( th->function )
    {
      case TFI_MoveFloor:
        *type1 = INTERP_sector_floor;
        *addr1 = ((floormove_t*)th)->sector;
        break;

      case TFI_PlatRaise:
        *type1 = INTERP_sector_floor;
        *addr1 = ((plat_t*)th)->sector;
        break;

      case TFI_MoveCeiling:
        *type1 = INTERP_sector_ceiling;
        *addr1 = ((ceiling_t*)th)->sector;
        break;

      case TFI_VerticalDoor:
        *type1 = INTERP_sector_ceiling;
        *addr1 = ((vldoor_t*)th)->sector;
        break;

      case TFI_MoveElevator:
        // The only one that moves both surfaces of a sector at once.
        *type1 = INTERP_sector_floor;
        *addr1 = ((elevator_t*)th)->sector;
        *type2 = INTERP_sector_ceiling;
        *addr2 = ((elevator_t*)th)->sector;
        break;

      case TFI_Scroll:
      {
        scroll_t * s = (scroll_t*)th;

        // SCROLL_carry and SCROLL_carry_ceiling move *things*, not the
        // texture, so there is nothing here for the renderer to smooth --
        // the things they carry are interpolated as mobjs like any other.
        switch( s->type )
        {
          case SCROLL_side:
            *type1 = INTERP_wall_panning;
            *addr1 = & sides[ s->affectee ];
            break;
          case SCROLL_floor:
            *type1 = INTERP_floor_panning;
            *addr1 = & sectors[ s->affectee ];
            break;
          case SCROLL_ceiling:
            *type1 = INTERP_ceiling_panning;
            *addr1 = & sectors[ s->affectee ];
            break;
          default:
            break;
        }
        break;
      }

      default:
        break;
    }
}

void  R_ActivateThinkerInterpolations( thinker_t * th )
{
    interp_type_e  type1 = 0, type2 = 0;
    void *  addr1;
    void *  addr2;

    interp_data_of( th, &type1, &addr1, &type2, &addr2 );
    if( addr1 )
    {
        interp_register( type1, addr1 );
        if( addr2 )
            interp_register( type2, addr2 );
    }
}

void  R_StopInterpolationIfNeeded( thinker_t * th )
{
    interp_type_e  type1 = 0, type2 = 0;
    void *  addr1;
    void *  addr2;

    interp_data_of( th, &type1, &addr1, &type2, &addr2 );
    if( addr1 )
    {
        interp_unregister( type1, addr1 );
        if( addr2 )
            interp_unregister( type2, addr2 );
    }
}

// [Arcade] Once per tic, from P_Ticker, before the thinkers run: everything
// registered records where it is starting from.
void  R_UpdateInterpolations(void)
{
    int i;


#ifdef PARANOIA
    // [Arcade] A tic must never begin with the world still holding
    // interpolated values.  That would mean some frame's
    // R_Interp_Frame_Begin was not matched by an End, and the capture below
    // would then record an interpolated height as if the simulation had
    // produced it -- the one way this feature can affect the game rather
    // than just the picture, so it is worth a test per tic to catch.
    if( frame_interpolated )
        I_SoftError( "R_UpdateInterpolations: unbalanced frame at tic %u\n",
                     (unsigned)gametic );
#endif

    // Mobjs are captured lazily rather than in a sweep here -- see
    // R_Interp_Capture_Mobj.  This is the flip it compares against.
    interp_tic_parity ^= 1;

    for( i = num_interp - 1; i >= 0; i-- )
        interp_capture( i );
}


// =========================================================================
//   Mobjs
// =========================================================================
//
// A mobj cannot simply be snapshotted at the top of its own think function:
// plenty of things are moved by something *other* than their own thinker --
// a lift carrying a player, a crusher pushing one down -- and if that runs
// first, the "previous" position captured afterwards is already the new one
// and the thing does not interpolate at all that tic.  Sweeping every mobj
// before the thinkers would fix it but costs a pass over every thing in the
// level, every tic, almost all of which never move.
//
// So instead: a parity bit flips once per tic, and each mobj remembers which
// parity it last captured at.  Whichever code touches the thing first this
// tic takes the snapshot and the rest are no-ops.  The capture calls go at
// the top of anything that can move a thing.

void  R_Interp_Capture_Mobj( mobj_t * mo )
{
    if( mo->interp_parity == interp_tic_parity )
        return;   // already captured this tic

    mo->PrevX = mo->x;
    mo->PrevY = mo->y;
    mo->PrevZ = mo->z;
    mo->PrevAngle = mo->angle;
    mo->interp_parity = interp_tic_parity;
}

// [Arcade] This thing did not travel to where it now is -- it was placed
// there.  Spawning, teleporting, respawning.  Drawing the step it appears to
// have taken would smear it across the level, so give it no step to draw.
void  R_Interp_Reset_Mobj( mobj_t * mo )
{
    mo->PrevX = mo->x;
    mo->PrevY = mo->y;
    mo->PrevZ = mo->z;
    mo->PrevAngle = mo->angle;
    mo->interp_parity = interp_tic_parity;
}

// [Arcade] Interpolate an angle the short way round.  Angles are unsigned
// and wrap, so the plain difference between 359 degrees and 1 degree is 358
// degrees the wrong way -- read as a signed value it is the -2 degrees
// actually turned.  Getting this wrong sends the view spinning a full circle
// every time the player crosses north.
angle_t  R_Interp_Angle( angle_t prev, angle_t now, fixed_t frac )
{
    int32_t  delta = (int32_t)( now - prev );

    return prev + (angle_t)(int32_t)( ((int64_t)delta * frac) >> FRACBITS );
}


// =========================================================================
//   Per frame
// =========================================================================

void  R_Interp_Reset_View(void)
{
    reset_view = true;

    // Also take effect on the frame in progress.  R_SetupFrame is where a
    // change of view target is noticed, and that runs after the frac for
    // this frame has already been chosen -- without this the very frame that
    // spots the discontinuity is the one that smears across it.
    rendertic_frac = FRACUNIT;
}

boolean  R_Interp_View_Active(void)
{
    return interp_active && !reset_view;
}

// Called once per frame, before anything is drawn.  Decides whether this
// frame interpolates at all, and at what fraction.
void  R_Interp_Set_Frac( fixed_t frac )
{
    // Nothing is moving while paused or in a menu -- P_Ticker returns early,
    // so oldpos would be interpolated towards a value that never changes and
    // the world would creep for one tic and then sit still.
    boolean  world_running = !paused
        && !( menuactive && !netgame && !demoplayback );

    // Interpolate whenever we are drawing at something other than the tic
    // rate.  At exactly TICRATE the frames land on the tics and there is
    // nothing between them to draw.
    interp_active = ( cv_framerate_cap.value != TICRATE )
                    && world_running && !singletics;

    if( !interp_active )
    {
        rendertic_frac = FRACUNIT;
        // A stopped world resumes from where it actually is.
        reset_view = true;
        return;
    }

    if( frac > FRACUNIT )  frac = FRACUNIT;
    if( frac < 0 )         frac = 0;


    // The frame after a discontinuity is drawn whole, at the true position.
    if( reset_view )
        frac = FRACUNIT;

    rendertic_frac = frac;
}

void  R_Interp_Frame_Begin(void)
{
    int i;

    if( frame_interpolated )   // unbalanced Begin; ignore rather than corrupt
        return;

    if( !interp_active || reset_view || rendertic_frac == FRACUNIT )
        return;

    for( i = num_interp - 1; i >= 0; i-- )
    {
        fixed_t * f1;
        fixed_t * f2;

        interp_fields( i, &f1, &f2 );
        if( f1 )
        {
            bakpos[i][0] = *f1;
            *f1 = oldpos[i][0]
                + FixedMul( *f1 - oldpos[i][0], rendertic_frac );
        }
        if( f2 )
        {
            bakpos[i][1] = *f2;
            *f2 = oldpos[i][1]
                + FixedMul( *f2 - oldpos[i][1], rendertic_frac );
        }
    }
    frame_interpolated = true;

}

void  R_Interp_Frame_End(void)
{
    int i;

    // The view has now been drawn once since the discontinuity, so the
    // history is good again.  This has to happen on the reset frame itself,
    // which is exactly the frame that does not interpolate and so returns
    // below -- clearing it after that test leaves it set forever.
    reset_view = false;

    if( !frame_interpolated )
        return;

    // Put the simulation's real values back.  Everything outside the render
    // -- collision, sound, sight checks, the next tic -- must see these and
    // never the interpolated ones.
    for( i = num_interp - 1; i >= 0; i-- )
    {
        fixed_t * f1;
        fixed_t * f2;

        interp_fields( i, &f1, &f2 );
        if( f1 )  *f1 = bakpos[i][0];
        if( f2 )  *f2 = bakpos[i][1];
    }
    frame_interpolated = false;

}

