// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_joy.h 1112 2014-06-03 21:54:41Z smite-meister $
//
// Copyright (C) 1998-2010 by DooM Legacy Team.
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
// $Log: i_joy.h,v $
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      share joystick information with game control code
//
//-----------------------------------------------------------------------------

#ifndef I_JOY_H
#define I_JOY_H

//! All possible actions joystick axes can be bound to.
typedef enum
{
  ja_pitch,  //!< Set up/down looking angle.
  ja_move,   //!< Moving front and back.
  ja_turn,   //!< Turn left or right.
  ja_strafe, //!< Strafing.
  num_joyactions
} joyactions_e;

//! Contains the mappings from joystick axes to game actions.
typedef struct
{
  int joynum;          //!< Which joystick to use.
  int axisnum;         //!< Which axis is the important one.
  int playnum;         //!< What player is controlled.
  joyactions_e action; //!< What should be done.
  float scale;         //!< A scaling factor. Set negative to flip axis.
} joybinding_t;


#define MAX_JOYSTICKS 4 // 4 should be enough for most purposes
extern int num_joysticks;

#define MAX_JOYBINDINGS 4*MAX_JOYSTICKS // hope this is enough
extern int num_joybindings;
extern joybinding_t joybindings[MAX_JOYBINDINGS];

void I_InitJoystick();
// Axes is plual of Axis
int I_JoystickNumAxes(int joynum);
int I_JoystickGetAxis(int joynum, int axisnum);

#endif // I_JOY_H
