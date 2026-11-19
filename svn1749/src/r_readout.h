// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 Doom Legacy Arcade.  GNU GPL version 3 or later.
//
// [Arcade] Busy Computer Panels (r_readout.c).  docs/arcade/derp-glow.md.
//
//-----------------------------------------------------------------------------

#ifndef R_READOUT_H
#define R_READOUT_H

#include "command.h"

extern consvar_t cv_readouts;

void R_Readout_Register( void );

// Once a frame, before any view is drawn.
void R_Readout_Frame( void );

#endif
