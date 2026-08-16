// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_input.h 1697 2024-11-27 06:47:25Z wesleyjohnson $
//
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: g_input.h,v $
// Revision 1.8  2002/08/24 22:42:03  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.7  2002/07/01 19:59:58  metzgermeister
//
// Revision 1.6  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.5  2001/02/24 13:35:20  bpereira
//
// Revision 1.4  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.3  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      handle mouse/keyboard/joystick inputs,
//      maps inputs to game controls (forward,use,open...)
//
//-----------------------------------------------------------------------------

#ifndef G_INPUT_H
#define G_INPUT_H

#include <stdio.h>
  // FILE

#include "doomdef.h"
  // SDL, MOUSE
#include "doomtype.h"
#include "d_event.h"
  // event_t
#include "keys.h"
#include "command.h"
  // consvar_t

#define MAXMOUSESENSITIVITY   40        // sensitivity steps


typedef enum
{
    gc_null = 0,        //a key/button mapped to gc_null has no effect
    gc_forward,
    gc_backward,
    gc_strafe,
    gc_straferight,
    gc_strafeleft,
    gc_speed,
    gc_turnleft,
    gc_turnright,
    gc_fire,
    gc_use,
    gc_lookup,
    gc_lookdown,
    gc_centerview,
    gc_mouseaiming,     // mouse aiming is momentary (toggleable in the menu)
    gc_weapon1,
    gc_weapon2,
    gc_weapon3,
    gc_weapon4,
    gc_weapon5,
    gc_weapon6,
    gc_weapon7,
    gc_weapon8,
    gc_talkkey,
    gc_scores,
    gc_jump,
    gc_console,
    gc_nextweapon,
    gc_prevweapon,
    gc_bestweapon,
    gc_invnext,
    gc_invprev,
    gc_invuse,
    gc_flydown,     // flyup is jump !
    gc_screenshot,
#ifdef ENABLE_COME_HERE
    gc_comehere,
#endif
    
// Mouse and joystick only, Fixed assignment for keyboard.
    gc_menuesc,  // joystick menu enter and escape key
    gc_pause,
    gc_automap,

    num_gamecontrols
} gamecontrols_e;


extern consvar_t   cv_grabinput;

// Player control
// [0]=main player [1]=splitscreen player
extern consvar_t   cv_autorun[2];
extern consvar_t   cv_usemouse[2];
extern consvar_t   cv_mouse_move[2];
extern consvar_t   cv_alwaysfreelook[2];

// [Arcade] Selectable control scheme, per player (see ControlScheme_Apply)
extern consvar_t   cv_controlscheme[2];

// [Arcade] The ten controls a cabinet panel needs, in the order the guided
// setup captures them and the order they are stored in cv_customcontrols.
// The two "pair" entries are what the scheme swaps: under "Look and Move"
// pair A turns and pair B strafes, under "WASD" it is the other way round.
enum {
    CK_forward = 0,
    CK_backward,
    CK_fire,
    CK_use,
    CK_nextweapon,
    CK_prevweapon,
    CK_pair_a_left,     // turn left  under "Look and Move", strafe under WASD
    CK_pair_a_right,
    CK_pair_b_left,     // strafe left under "Look and Move", turn under WASD
    CK_pair_b_right,
    CK_NUMKEYS
};

// [Arcade] Operator-defined key table, per player, written by the guided
// setup as ten space separated key codes.  Empty means "use the built-in
// scheme_keys[] preset".  This is what moves a cabinet's control layout out
// of the hardcoded table and into config.cfg.
extern consvar_t   cv_customcontrols[2];

// Store keys[CK_NUMKEYS] into cv_customcontrols[pind] and apply immediately.
void  G_Save_CustomControls( int pind, const int * keys );

// mouse1
extern consvar_t   cv_mouse_invert;
extern consvar_t   cv_mouse_sens_x;
extern consvar_t   cv_mouse_sens_y;
#ifdef SMIF_SDL
extern consvar_t   cv_mouse_motion;
#endif

// mouse2
extern consvar_t   cv_mouse2_invert;
extern consvar_t   cv_mouse2_sens_x;
extern consvar_t   cv_mouse2_sens_y;
#ifdef MOUSE2
extern consvar_t   cv_mouse2port;
extern consvar_t   cv_mouse2opt;
# if defined( SMIF_SDL ) || defined( SMIF_WIN32 ) || defined( SMIF_X11 )
extern consvar_t   cv_mouse2type;
# endif
# ifdef LINUX
#  define MOUSE2_NIX
# endif
# if defined( MAC ) || defined( __MACH__ )
#  define MOUSE2_NIX
# endif
# ifdef WIN32
#  define MOUSE2_WIN
# else
# ifdef OS2
#  define MOUSE2_DOS
# endif
# ifdef DOS
#  define MOUSE2_DOS
# endif
# endif
#endif

extern consvar_t   cv_mouse_double;

extern consvar_t   cv_joy_deadzone;
#ifdef JOY_BUTTONS_DOUBLE     
extern consvar_t   cv_joy_double;
#endif

extern int             mousex;
extern int             mousey;
extern int             mouse2x;
extern int             mouse2y;

extern int             dclicktime;
extern int             dclickstate;
extern int             dclicks;
extern int             dclicktime2;
extern int             dclickstate2;
extern int             dclicks2;

extern byte  gamekeydown[NUMINPUTS];
extern byte  gamekeytapped[NUMINPUTS];

// two key codes (or virtual key) per game control
extern  int     gamecontrol[num_gamecontrols][2];
extern  int     gamecontrol2[num_gamecontrols][2];    // secondary splitscreen player

// peace to my little coder fingers!
// check a gamecontrol being active or not

// remaps the input event to a game control.
void  G_MapEventsToControls (event_t *ev);

// returns the name of a key
char* G_KeynumToString (int keynum);
int   G_KeyStringtoNum(char *keystr);

// detach any keys associated to the given game control
void  G_Clear_ControlKeys (int (*setupcontrols)[2], int control);
void  Command_Setcontrol_f(void);
void  Command_Setcontrol2_f(void);
void  G_Controldefault(void);
void  G_SaveKeySetting(FILE *f);
void  G_CheckDoubleUsage(int keynum);

// Called for cv_usemouse, cv_grabinput, cv_mouse_motion
void  CV_mouse_OnChange( void );

#endif
