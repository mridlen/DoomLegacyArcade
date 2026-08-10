// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: keys.h 1466 2019-10-01 02:37:37Z wesleyjohnson $
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
//-----------------------------------------------------------------------------

#ifndef KEYS_H
#define KEYS_H

enum aux_controller_e
{
  MOUSEBUTTONS =  8,
  MAXJOYSTICKS =  4,  // "Only" 4 joysticks per machine.
  JOYBUTTONS   = 16,  // Max number of buttons for a joystick.
  JOYHATBUTTONS = 4,  // Four hat directions.
  XBOXTRIGGERS =  2,  // Two triggers on Xbox-like controllers.
};

/// These are the key event codes posted by the keyboard handler, and closely match the SDLKey symbols.
/// 0-127 are ASCII codes. The codes KEY_NUMKB- are reserved for virtual keys.
enum key_input_e
{
  KEY_NULL = 0, // null key, triggers nothing

  KEY_BACKSPACE  = 8,
  KEY_TAB        = 9,
  KEY_ENTER      = 13,
  KEY_PAUSE      = 19,
  KEY_ESCAPE     = 27,
  KEY_SPACE      = 32,

  // numbers
  // big letters
  KEY_BACKQUOTE  = 96,
  KEY_CONSOLE    = KEY_BACKQUOTE,
  // small letters
  KEY_DELETE     = 127, // ascii ends here

  // SDL international keys 160-255

  // keypad
  KEY_KEYPAD0 = 256,
  KEY_KEYPAD1,
  KEY_KEYPAD2,
  KEY_KEYPAD3,
  KEY_KEYPAD4,
  KEY_KEYPAD5,
  KEY_KEYPAD6,
  KEY_KEYPAD7,
  KEY_KEYPAD8,
  KEY_KEYPAD9,
  KEY_KPADPERIOD,
  KEY_KPADSLASH,
  KEY_KPADMULT,
  KEY_MINUSPAD,
  KEY_PLUSPAD,
  KEY_KPADENTER,
  KEY_KPADEQUALS,
  // arrows + home/end pad
  KEY_UPARROW,
  KEY_DOWNARROW,
  KEY_RIGHTARROW,
  KEY_LEFTARROW,
  KEY_INS,
  KEY_HOME,
  KEY_END,
  KEY_PGUP,
  KEY_PGDN,
  // function keys
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,

  // modifier keys
  KEY_NUMLOCK = 300,
  KEY_CAPSLOCK,
  KEY_SCROLLLOCK,
  KEY_RSHIFT,
  KEY_LSHIFT,
  KEY_RCTRL,
  KEY_LCTRL,
  KEY_RALT,
  KEY_LALT,
  KEY_unused1,
  KEY_unused2,
  KEY_LWIN,
  KEY_RWIN,
  KEY_MODE, // altgr
  KEY_unused3,

  // other function keys
  KEY_HELP = 315,
  KEY_PRINT,
  KEY_SYSREQ,
  KEY_BREAK,
  KEY_MENU,

  KEY_NUMKB, // all real keyboard codes are under this value

  // mouse and joystick buttons are handled as 'virtual' keys
  KEY_MOUSE1          = KEY_NUMKB, // mouse buttons, including the wheel
  KEY_MOUSEWHEELUP    = KEY_MOUSE1 + 3, // usually
  KEY_MOUSEWHEELDOWN,
  KEY_MOUSE1DBL      = KEY_MOUSE1     + MOUSEBUTTONS, // double clicks

  KEY_MOUSE2         = KEY_MOUSE1DBL  + MOUSEBUTTONS, // second mouse buttons
  KEY_MOUSE2WHEELUP  = KEY_MOUSE2 + 3,
  KEY_MOUSE2WHEELDOWN,
  KEY_MOUSE2DBL      = KEY_MOUSE2    + MOUSEBUTTONS,

  KEY_JOY0BUT0 = KEY_MOUSE2DBL + MOUSEBUTTONS, // joystick buttons
  KEY_JOY0BUT1,
  KEY_JOY0BUT2,
  KEY_JOY0BUT3,
  KEY_JOY0BUT4,
  KEY_JOY0BUT5,
  KEY_JOY0BUT6,
  KEY_JOY0BUT7,
  KEY_JOY0BUT8,
  KEY_JOY0BUT9,
  KEY_JOY0BUT10,
  KEY_JOY0BUT11,
  KEY_JOY0BUT12,
  KEY_JOY0BUT13,
  KEY_JOY0BUT14,
  KEY_JOY0BUT15,

  KEY_JOY1BUT0,
  KEY_JOY1BUT1,
  KEY_JOY1BUT2,
  KEY_JOY1BUT3,
  KEY_JOY1BUT4,
  KEY_JOY1BUT5,
  KEY_JOY1BUT6,
  KEY_JOY1BUT7,
  KEY_JOY1BUT8,
  KEY_JOY1BUT9,
  KEY_JOY1BUT10,
  KEY_JOY1BUT11,
  KEY_JOY1BUT12,
  KEY_JOY1BUT13,
  KEY_JOY1BUT14,
  KEY_JOY1BUT15,

  KEY_JOY2BUT0,
  KEY_JOY2BUT1,
  KEY_JOY2BUT2,
  KEY_JOY2BUT3,
  KEY_JOY2BUT4,
  KEY_JOY2BUT5,
  KEY_JOY2BUT6,
  KEY_JOY2BUT7,
  KEY_JOY2BUT8,
  KEY_JOY2BUT9,
  KEY_JOY2BUT10,
  KEY_JOY2BUT11,
  KEY_JOY2BUT12,
  KEY_JOY2BUT13,
  KEY_JOY2BUT14,
  KEY_JOY2BUT15,

  KEY_JOY3BUT0,
  KEY_JOY3BUT1,
  KEY_JOY3BUT2,
  KEY_JOY3BUT3,
  KEY_JOY3BUT4,
  KEY_JOY3BUT5,
  KEY_JOY3BUT6,
  KEY_JOY3BUT7,
  KEY_JOY3BUT8,
  KEY_JOY3BUT9,
  KEY_JOY3BUT10,
  KEY_JOY3BUT11,
  KEY_JOY3BUT12,
  KEY_JOY3BUT13,
  KEY_JOY3BUT14,
  KEY_JOY3BUT15,
  KEY_JOYLAST = KEY_JOY3BUT15,

  KEY_JOY0HATUP,
  KEY_JOY0HATRIGHT,
  KEY_JOY0HATDOWN,
  KEY_JOY0HATLEFT,
  
  KEY_JOY1HATUP,
  KEY_JOY1HATRIGHT,
  KEY_JOY1HATDOWN,
  KEY_JOY1HATLEFT,
  
  KEY_JOY2HATUP,
  KEY_JOY2HATRIGHT,
  KEY_JOY2HATDOWN,
  KEY_JOY2HATLEFT,
 
  KEY_JOY3HATUP,
  KEY_JOY3HATRIGHT,
  KEY_JOY3HATDOWN,
  KEY_JOY3HATLEFT,
 
  KEY_JOY0LEFTTRIGGER,
  KEY_JOY0RIGHTTRIGGER,
  
  KEY_JOY1LEFTTRIGGER,
  KEY_JOY1RIGHTTRIGGER,
  
  KEY_JOY2LEFTTRIGGER,
  KEY_JOY2RIGHTTRIGGER,
  
  KEY_JOY3LEFTTRIGGER,
  KEY_JOY3RIGHTTRIGGER,
     
#ifdef JOY_BUTTONS_DOUBLE     
  // duplicate all joy, all buttons, KEY_JOY0BUT0 .. KEY_JOY3BUT15
  KEY_JOY0BUT0DBL,
  KEY_JOY1BUT0DBL = KEY_JOY0BUT0DBL + JOYBUTTONS,
  KEY_JOY2BUT0DBL = KEY_JOY0BUT0DBL + JOYBUTTONS,
  KEY_JOY3BUT0DBL = KEY_JOY0BUT0DBL + JOYBUTTONS,
  KEY_JOYLASTDBL  = KEY_JOY0BUT0DBL + JOYBUTTONS - 1,
#endif

  // number of total 'button' inputs, includes keyboard keys, plus virtual
  // keys (mousebuttons and joybuttons become keys)
  NUMINPUTS
};

#endif
