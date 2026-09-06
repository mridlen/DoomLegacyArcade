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
//      [Arcade] Uncapped framerate: render-time interpolation.
//
//      Adapted from PrBoom-plus / dsda-doom's r_fps.c, which is where this
//      technique comes from and which is GPL v2-or-later, the same licence
//      this tree carries.
//
//      The simulation is NOT touched.  It still runs at exactly TICRATE and
//      every ticcmd, every random number and every stored time is what it
//      always was -- which is what keeps record demos and high score times
//      valid.  All that changes is that D_Display may now run several times
//      per tic, drawing the world *between* two positions the simulation has
//      already computed.
//
//      Everything that moves therefore has to remember where it was at the
//      start of the current tic.  Given "frac", how far through the tic this
//      frame falls (0..FRACUNIT), the renderer draws at
//          prev + (now - prev) * frac
//      so nothing is ever predicted -- only drawn part way between two known
//      states.  The cost is that the picture trails the simulation by up to
//      one tic (28ms).
//
//      Sector heights and texture panning are handled differently from
//      mobjs: rather than teach every plane and wall drawer about frac, the
//      interpolated value is written into the live sector_t/side_t before the
//      frame and the real value put back after it (R_Interp_Frame_Begin /
//      R_Interp_Frame_End).  The drawers need no changes at all, but it does
//      mean nothing outside the renderer may run between those two calls.
//
//-----------------------------------------------------------------------------

#ifndef R_FPS_H
#define R_FPS_H

#include "doomtype.h"
#include "d_think.h"
#include "m_fixed.h"
#include "tables.h"
  // angle_t
#include "command.h"

// "uncapped": draw more frames than there are tics.  Render only, so it is
// deliberately NOT a netvar and NOT in the demo header -- two machines with
// different settings still simulate identically.
extern consvar_t  cv_uncapped;

// How far through the current tic this frame falls, 0..FRACUNIT.
// FRACUNIT exactly whenever interpolation is off, so every consumer can
// multiply unconditionally and get the stock result.
extern fixed_t  rendertic_frac;

// Is interpolation actually running right now?  False when the cvar is off,
// when the game is paused or menued (nothing is moving to interpolate), and
// during demo recording playback edges.  Consumers must ask this rather than
// reading the cvar, because it is the switch the suppression code uses.
extern boolean  interp_active;

void  R_Interp_Level_Init(void);     // per level, after sectors/sides exist
void  R_Interp_Set_Frac(fixed_t frac);   // called once per frame by D_Display

// [Arcade] Discard the previous frame's history: the next frame is drawn at
// its true position with no interpolation.  Anything that moves the view or
// the world discontinuously must call this, or the camera visibly slides
// across the level.  Level load, teleport, respawn, and unpausing.
void  R_Interp_Reset_View(void);

// Bracket the world render.  Begin writes interpolated sector heights and
// panning into the live structures; End puts the real values back.  They must
// be paired -- End is what keeps the simulation's view of the map correct.
void  R_Interp_Frame_Begin(void);
void  R_Interp_Frame_End(void);

// The three hooks p_tick.c has always had (THINKER_INTERPOLATIONS), matching
// the PrBoom names it was written against.
void  R_UpdateInterpolations(void);                   // once per tic
void  R_ActivateThinkerInterpolations(thinker_t * th);
void  R_StopInterpolationIfNeeded(thinker_t * th);

// Was the view interpolated this frame?  R_SetupFrame asks, so that the
// chase camera and the psprites agree with the world.
boolean  R_Interp_View_Active(void);

// Mobjs.  Capture is lazy and idempotent within a tic -- call it at the top
// of anything that can move a thing.  Reset is for a thing that was *placed*
// rather than moved (spawn, teleport, respawn): it leaves no step to draw.
struct mobj_s;
void  R_Interp_Capture_Mobj( struct mobj_s * mo );
void  R_Interp_Reset_Mobj( struct mobj_s * mo );

// Interpolate an angle the short way round the circle.
angle_t  R_Interp_Angle( angle_t prev, angle_t now, fixed_t frac );

// [Arcade] Interpolate a position towards where it is now.
//
// Deliberately unconditional: rendertic_frac is FRACUNIT whenever
// interpolation is off or this frame is the first after a discontinuity, and
// FixedMul carries a 64-bit intermediate, so this is then *exactly* `now`
// for every input -- stale or even uninitialised history included.  That is
// what lets the call sites read as plain assignments with no "is it on?"
// branch around each one, and it is why turning the cvar off restores the
// stock picture bit for bit rather than approximately.
static inline fixed_t  R_Interp_Fixed( fixed_t prev, fixed_t now )
{
    return prev + FixedMul( rendertic_frac, now - prev );
}

// The same for an angle.  R_Interp_Angle( p, n, FRACUNIT ) == n likewise.
static inline angle_t  R_Interp_View_Angle( angle_t prev, angle_t now )
{
    return R_Interp_Angle( prev, now, rendertic_frac );
}

#endif // R_FPS_H
