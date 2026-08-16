// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: g_input.c 1697 2024-11-27 06:47:25Z wesleyjohnson $
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
// $Log: g_input.c,v $
// Revision 1.12  2002/08/24 22:42:02  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.11  2002/07/01 19:59:58  metzgermeister
// *** empty log message ***
//
// Revision 1.10  2001/03/30 17:12:49  bpereira
// Revision 1.9  2001/02/24 13:35:19  bpereira
// Revision 1.8  2001/02/10 12:27:13  bpereira
//
// Revision 1.7  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.6  2000/11/26 00:46:31  hurdler
//
// Revision 1.5  2000/10/04 17:03:57  hurdler
// This is the formule I propose for mouse sensitivity
//
// Revision 1.4  2000/04/16 18:38:07  bpereira
//
// Revision 1.3  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      handle mouse/keyboard/joystick inputs,
//      maps inputs to game controls (forward,use,open...)
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "g_input.h"
#include "keys.h"
#include "hu_stuff.h"   //need HUFONT start & end
#include "keys.h"
#include "d_net.h"
#include "console.h"
#include "i_joy.h"
#include "i_system.h"


#ifdef JOYSTICK_SUPPORT    
int num_joybindings = 0;
joybinding_t joybindings[MAX_JOYBINDINGS];
#endif


CV_PossibleValue_t mousesens_cons_t[]={{1,"MIN"},{MAXMOUSESENSITIVITY,"MAXCURSOR"},{INT_MAX,"MAX"},{0,NULL}};
CV_PossibleValue_t onecontrolperkey_cons_t[]={{1,"One"},{2,"Several"},{0,NULL}};

// [Arcade] Selectable control scheme, for a fixed cabinet key layout.
// Each player chooses independently; the two schemes differ only in which
// key pair turns and which strafes.
void ControlScheme1_OnChange(void);
void ControlScheme2_OnChange(void);
// CS_custom is what the guided setup (m_menu.c) selects.  It is not a layout
// of its own -- it means "leave these bindings alone", so whatever the
// operator bound survives in config.cfg as ordinary setcontrol lines.
CV_PossibleValue_t controlscheme_cons_t[] =
  { {0,"Look and Move"}, {1,"WASD"}, {CS_custom,"Custom"}, {0,NULL} };
consvar_t  cv_controlscheme[2] = {
  { "controlscheme",  "0", CV_SAVE | CV_CALL, controlscheme_cons_t, ControlScheme1_OnChange },
  { "controlscheme2", "0", CV_SAVE | CV_CALL, controlscheme_cons_t, ControlScheme2_OnChange }
};

// mouse values are used once
consvar_t  cv_mouse_sens_x    = {"mousesensx","10",CV_SAVE,mousesens_cons_t};
consvar_t  cv_mouse_sens_y    = {"mousesensy","10",CV_SAVE,mousesens_cons_t};
consvar_t  cv_controlperkey   = {"controlperkey","1",CV_SAVE,onecontrolperkey_cons_t};

CV_PossibleValue_t usemouse_cons_t[] = { {0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL} };
consvar_t cv_usemouse[2] = {
  { "use_mouse", "1", CV_SAVE | CV_CALL, usemouse_cons_t, CV_mouse_OnChange },
  { "use_mouse2", "0", CV_SAVE | CV_CALL, usemouse_cons_t, I_StartupMouse2 }
};

#ifdef MOUSE2
#ifdef MOUSE2_NIX
CV_PossibleValue_t mouse2port_cons_t[] = {
  {0, "gpmdata"},
  {1, "mouse2"},  // indirection
  {2, "ttyS0"}, {3, "ttyS1"}, {4, "ttyS2"}, {5, "ttyS3"}, {6, "ttyS4"}, {7, "ttyS5"}, {8, "ttyS6"},
  {0, NULL} };
#define CV_DEFAULT_PORT  "gpmdata"
#endif
#ifdef MOUSE2_WIN
CV_PossibleValue_t mouse2port_cons_t[] = {
  {1, "COM1"}, {2, "COM2"}, {3, "COM3"}, {4, "COM4"},
  {5, "COM5"}, {6, "COM6"}, {7, "COM7"}, {8, "COM8"},  // Some Windows are reported to connect USB or other devices to COM ports
  {0, NULL} };
#define CV_DEFAULT_PORT  "COM2"
#endif
#ifdef MOUSE2_DOS
CV_PossibleValue_t mouse2port_cons_t[] = {
  {1, "COM1"}, {2, "COM2"}, {3, "COM3"}, {4, "COM4"},
  {0, NULL} };
#define CV_DEFAULT_PORT  "COM2"
#endif
consvar_t cv_mouse2port = { "mouse2port", CV_DEFAULT_PORT , CV_SAVE|CV_STRING|CV_CALL|CV_NOINIT, mouse2port_cons_t, I_StartupMouse2 };
consvar_t cv_mouse2opt = { "mouse2opt", "0", CV_SAVE|CV_STRING|CV_CALL|CV_NOINIT, NULL, I_StartupMouse2 };
#if defined( SMIF_SDL ) || defined( SMIF_WIN32 ) || defined( SMIF_X11 )
// Only in SDL, WIN32, X11 for now.
CV_PossibleValue_t mouse2type_cons_t[] = { {0, "PC"}, {1, "MS"}, {2, "PS2"}, {0, NULL} };
consvar_t cv_mouse2type = { "mouse2type", "0" , CV_SAVE|CV_STRING|CV_CALL|CV_NOINIT, mouse2type_cons_t, I_StartupMouse2 };
#endif
consvar_t  cv_mouse2_sens_x   = {"mouse2sensx","10",CV_SAVE,mousesens_cons_t};
consvar_t  cv_mouse2_sens_y   = {"mouse2sensy","10",CV_SAVE,mousesens_cons_t};
#endif

#ifdef SMIF_SDL
CV_PossibleValue_t mouse_motion_cons_t[]={{0,"Absolute"},{1,"Relative"},{0,NULL}};
consvar_t  cv_mouse_motion = {"mousemotion","0", CV_SAVE|CV_CALL|CV_NOINIT, mouse_motion_cons_t, CV_mouse_OnChange };
#endif

consvar_t  cv_grabinput = {"grabinput","1", CV_SAVE|CV_CALL, CV_OnOff, CV_mouse_OnChange };

// A normal double click is approx. 6 tics from previous click.
CV_PossibleValue_t double_cons_t[]={{1,"MIN"},{40,"MAX"},{0,NULL}};  // double click threshold
consvar_t  cv_mouse_double   = {"mousedouble","8",CV_SAVE, double_cons_t};
#ifdef JOYSTICK_SUPPORT
#ifdef JOY_BUTTONS_DOUBLE
consvar_t  cv_joy_double     = {"joydouble",  "8",CV_SAVE, double_cons_t};
#endif
#endif


// Called for cv_grabinput, cv_mouse_motion, use_mouse
void  CV_mouse_OnChange( void )
{
   I_StartupMouse( !(paused || menuactive) );
}


int  mousex, mousey;
int  mouse2x, mouse2y;

// [WDJ] When boolean, the compiler uses 4 bytes for one bit of information.
byte  gamekeydown[NUMINPUTS]; // Current state of the keys: true if the key is currently down.
byte  gamekeytapped[NUMINPUTS]; // True if the key has been pressed since the last G_BuildTiccmd. Useful for impulse-style controls.


// two key codes (or virtual key) per game control
int  gamecontrol[num_gamecontrols][2];
int  gamecontrol2[num_gamecontrols][2];        // secondary splitscreen player


// FIXME: this can be simplified to two bytes
// [WDJ] Got it down to 3 bytes.
typedef struct {
  byte  dtime;   // 0..21
  byte  key_down;
  byte  clicks;  // 0..2
} dclick_t;

// FIXME: only one mouse, only one joy
static  dclick_t  mouse_dclick[MOUSEBUTTONS];
#ifdef JOYSTICK_SUPPORT
#ifdef JOY_BUTTONS_DOUBLE
static  dclick_t  joy_dclick[JOYBUTTONS];
#endif
#endif

static tic_t  clicktic = 0;  // gametic of last button check
static unsigned int  click_delta;  // tics since last button check, limited 120

//
//  General double-click detection routine for any kind of input.
//
static boolean G_CheckDoubleClick(int key_id, dclick_t *dt, byte double_click_threshold )
{
    // [WDJ] This is only called for each event, so time cannot be determined by counting.
    byte key_down = gamekeydown[ key_id ];
    // Limited to 120 tics since last click, anything > 40 tics is a timeout.
    dt->dtime += click_delta;  // + 0..120, must not overflow
    // This is called infrequently, so check timeouts first.
    // A normal double click is approx. 6 tics from previous click.
    if( dt->dtime > double_click_threshold )     // 1..40
    {
        dt->dtime = 121;  // timeout (with room to add 120)
        dt->clicks = 0;
        dt->key_down = false;
    }

    // Check new state.
    if( (key_down != dt->key_down) && (dt->dtime > 1) )
    {
        dt->key_down = key_down;
        if( key_down )
        {
            dt->clicks++;
            if(dt->clicks == 2)
            {
                dt->clicks = 0;
                dt->dtime = 0;
                return true;
            }
        }
        dt->dtime = 0;
    }
    return false;
}



//
//  Remaps the inputs to game controls.
//
//  A game control can be triggered by one or more keys/buttons.
//
//  Each key/mousebutton/joybutton triggers ONLY ONE game control.
//
//
void  G_MapEventsToControls (event_t *ev)
{
  int i;

    switch (ev->type)
    {
      case ev_keydown:
        if (ev->data1 < NUMINPUTS)
        {
            gamekeydown[ev->data1] = true;
            gamekeytapped[ev->data1] = true; // reset in G_BuildTiccmd
        }
        break;

      case ev_keyup:
        if (ev->data1 < NUMINPUTS)
          gamekeydown[ev->data1] = false;
        break;

      case ev_mouse:           // buttons are virtual keys
        // [WDJ] To handle multiple mouse motion events per frame instead
        // of letting the last event be the sole mouse motion,
        // add the mouse events.
        // Necessary for OpenBSD which reports x and y in separate events.
        mousex += ev->data2*((cv_mouse_sens_x.value*cv_mouse_sens_x.value)/110.0f + 0.1);
        mousey += ev->data3*((cv_mouse_sens_y.value*cv_mouse_sens_y.value)/110.0f + 0.1);
        break;

#ifdef MOUSE2
      case ev_mouse2:           // buttons are virtual keys
        // add multiple mouse motions
        mouse2x += ev->data2*((cv_mouse2_sens_x.value*cv_mouse2_sens_x.value)/110.0f + 0.1);
        mouse2y += ev->data3*((cv_mouse2_sens_y.value*cv_mouse2_sens_y.value)/110.0f + 0.1);
        break;
#endif

      default:
        break;
    }

    // [WDJ] Only called for events, so click time needs to be based upon gametic.
    // This is overflow safe, it wraps.
    click_delta = (gametic - clicktic) / NEWTICRATERATIO;  // must be 35 tic/sec based.
    if( click_delta > 120 )  click_delta = 120;
    clicktic = gametic;

    // ALWAYS check for mouse & joystick double-clicks
    // even if no mouse event
    // FIXME: first MOUSE only
    for (i=0;i<MOUSEBUTTONS;i++)
      gamekeydown[KEY_MOUSE1DBL+i] = G_CheckDoubleClick( KEY_MOUSE1+i, &mouse_dclick[i], cv_mouse_double.value );

#ifdef JOYSTICK_SUPPORT
#ifdef JOY_BUTTONS_DOUBLE
    // joystick doubleclicks
    // FIXME: JOY0 only
    for (i=0;i<JOYBUTTONS;i++)
      gamekeydown[KEY_JOY0BUT0DBL+i] = G_CheckDoubleClick( KEY_JOY0BUT0+i, &joy_dclick[i], cv_joy_double.value );
#endif
#endif
}




typedef struct {
    int  keynum;
    char name[16];
} keyname_t;

static keyname_t keynames[] =
{
  {KEY_NULL,      "null"},

  {KEY_BACKSPACE, "backspace"},
  {KEY_TAB,       "tab"},
  {KEY_ENTER,     "enter"},
  {KEY_PAUSE,     "pause"},  // irrelevant, since this key cannot be remapped...
  {KEY_ESCAPE,    "escape"}, // likewise
  {KEY_SPACE,     "space"},

  {KEY_CONSOLE,    "console"},

  {KEY_NUMLOCK,    "num lock"},
  {KEY_CAPSLOCK,   "caps lock"},
  {KEY_SCROLLLOCK, "scroll lock"},
  {KEY_SYSREQ,     "sysreq"},
  {KEY_RSHIFT,     "right shift"},
  {KEY_LSHIFT,     "left shift"},
  {KEY_RCTRL,      "right ctrl"},
  {KEY_LCTRL,      "left ctrl"},
  {KEY_RALT,       "right alt"},
  {KEY_LALT,       "left alt"},
  {KEY_LWIN,       "left win"},
  {KEY_RWIN,       "right win"},
  {KEY_MODE,       "altgr"},
  {KEY_MENU,       "menu"},

  // keypad keys
  {KEY_KEYPAD0, "keypad 0"},
  {KEY_KEYPAD1, "keypad 1"},
  {KEY_KEYPAD2, "keypad 2"},
  {KEY_KEYPAD3, "keypad 3"},
  {KEY_KEYPAD4, "keypad 4"},
  {KEY_KEYPAD5, "keypad 5"},
  {KEY_KEYPAD6, "keypad 6"},
  {KEY_KEYPAD7, "keypad 7"},
  {KEY_KEYPAD8, "keypad 8"},
  {KEY_KEYPAD9, "keypad 9"},
  {KEY_KPADPERIOD,"keypad ."},
  {KEY_KPADSLASH, "keypad /"},
  {KEY_KPADMULT,  "keypad *"},
  {KEY_MINUSPAD,  "keypad -"},
  {KEY_PLUSPAD,   "keypad +"},

  // extended keys (not keypad)
  {KEY_UPARROW,   "up arrow"},
  {KEY_DOWNARROW, "down arrow"},
  {KEY_RIGHTARROW,"right arrow"},
  {KEY_LEFTARROW, "left arrow"},
  {KEY_INS,       "ins"},
  {KEY_DELETE,    "del"},
  {KEY_HOME,      "home"},
  {KEY_END,       "end"},
  {KEY_PGUP,      "pgup"},
  {KEY_PGDN,      "pgdown"},

  // other keys
  {KEY_F1, "F1"},
  {KEY_F2, "F2"},
  {KEY_F3, "F3"},
  {KEY_F4, "F4"},
  {KEY_F5, "F5"},
  {KEY_F6, "F6"},
  {KEY_F7, "F7"},
  {KEY_F8, "F8"},
  {KEY_F9, "F9"},
  {KEY_F10,"F10"},
  {KEY_F11,"F11"},
  {KEY_F12,"F12"},

  // virtual keys for mouse buttons and joystick buttons
  {KEY_MOUSE1,  "mouse 1"},
  {KEY_MOUSE1+1,"mouse 2"},
  {KEY_MOUSE1+2,"mouse 3"},
  {KEY_MOUSEWHEELUP, "mwheel up"},
  {KEY_MOUSEWHEELDOWN,"mwheel down"},
  {KEY_MOUSE1+5,"mouse 6"},
  {KEY_MOUSE1+6,"mouse 7"},
  {KEY_MOUSE1+7,"mouse 8"},
  {KEY_MOUSE2,  "mouse2 1"},
  {KEY_MOUSE2+1,"mouse2 2"},
  {KEY_MOUSE2+2,"mouse2 3"},
  {KEY_MOUSE2WHEELUP,"m2 mwheel up"},
  {KEY_MOUSE2WHEELDOWN,"m2 mwheel down"},
  {KEY_MOUSE2+5,"mouse2 6"},
  {KEY_MOUSE2+6,"mouse2 7"},
  {KEY_MOUSE2+7,"mouse2 8"},

  {KEY_MOUSE1DBL,   "mouse 1 d"},
  {KEY_MOUSE1DBL+1, "mouse 2 d"},
  {KEY_MOUSE1DBL+2, "mouse 3 d"},
  {KEY_MOUSE1DBL+3, "mouse 4 d"},
  {KEY_MOUSE1DBL+4, "mouse 5 d"},
  {KEY_MOUSE1DBL+5, "mouse 6 d"},
  {KEY_MOUSE1DBL+6, "mouse 7 d"},
  {KEY_MOUSE1DBL+7, "mouse 8 d"},
  {KEY_MOUSE2DBL,  "mouse2 2 d"},
  {KEY_MOUSE2DBL+1,"mouse2 1 d"},
  {KEY_MOUSE2DBL+2,"mouse2 3 d"},
  {KEY_MOUSE2DBL+3,"mouse2 4 d"},
  {KEY_MOUSE2DBL+4,"mouse2 5 d"},
  {KEY_MOUSE2DBL+5,"mouse2 6 d"},
  {KEY_MOUSE2DBL+6,"mouse2 7 d"},
  {KEY_MOUSE2DBL+7,"mouse2 8 d"},

#ifdef JOYSTICK_SUPPORT
  // This is translation table, index does not matter, so this can be optional.
  {KEY_JOY0BUT0, "Joy0 b0"},
  {KEY_JOY0BUT1, "Joy0 b1"},
  {KEY_JOY0BUT2, "Joy0 b2"},
  {KEY_JOY0BUT3, "Joy0 b3"},
  {KEY_JOY0BUT4, "Joy0 b4"},
  {KEY_JOY0BUT5, "Joy0 b5"},
  {KEY_JOY0BUT6, "Joy0 b6"},
  {KEY_JOY0BUT7, "Joy0 b7"},
  {KEY_JOY0BUT8, "Joy0 b8"},
  {KEY_JOY0BUT9, "Joy0 b9"},
  {KEY_JOY0BUT10, "Joy0 b10"},
  {KEY_JOY0BUT11, "Joy0 b11"},
  {KEY_JOY0BUT12, "Joy0 b12"},
  {KEY_JOY0BUT13, "Joy0 b13"},
  {KEY_JOY0BUT14, "Joy0 b14"},
  {KEY_JOY0BUT15, "Joy0 b15"},

  {KEY_JOY1BUT0, "Joy1 b0"},
  {KEY_JOY1BUT1, "Joy1 b1"},
  {KEY_JOY1BUT2, "Joy1 b2"},
  {KEY_JOY1BUT3, "Joy1 b3"},
  {KEY_JOY1BUT4, "Joy1 b4"},
  {KEY_JOY1BUT5, "Joy1 b5"},
  {KEY_JOY1BUT6, "Joy1 b6"},
  {KEY_JOY1BUT7, "Joy1 b7"},
  {KEY_JOY1BUT8, "Joy1 b8"},
  {KEY_JOY1BUT9, "Joy1 b9"},
  {KEY_JOY1BUT10, "Joy1 b10"},
  {KEY_JOY1BUT11, "Joy1 b11"},
  {KEY_JOY1BUT12, "Joy1 b12"},
  {KEY_JOY1BUT13, "Joy1 b13"},
  {KEY_JOY1BUT14, "Joy1 b14"},
  {KEY_JOY1BUT15, "Joy1 b15"},

  {KEY_JOY2BUT0, "Joy2 b0"},
  {KEY_JOY2BUT1, "Joy2 b1"},
  {KEY_JOY2BUT2, "Joy2 b2"},
  {KEY_JOY2BUT3, "Joy2 b3"},
  {KEY_JOY2BUT4, "Joy2 b4"},
  {KEY_JOY2BUT5, "Joy2 b5"},
  {KEY_JOY2BUT6, "Joy2 b6"},
  {KEY_JOY2BUT7, "Joy2 b7"},
  {KEY_JOY2BUT8, "Joy2 b8"},
  {KEY_JOY2BUT9, "Joy2 b9"},
  {KEY_JOY2BUT10, "Joy2 b10"},
  {KEY_JOY2BUT11, "Joy2 b11"},
  {KEY_JOY2BUT12, "Joy2 b12"},
  {KEY_JOY2BUT13, "Joy2 b13"},
  {KEY_JOY2BUT14, "Joy2 b14"},
  {KEY_JOY2BUT15, "Joy2 b15"},

  {KEY_JOY3BUT0, "Joy3 b0"},
  {KEY_JOY3BUT1, "Joy3 b1"},
  {KEY_JOY3BUT2, "Joy3 b2"},
  {KEY_JOY3BUT3, "Joy3 b3"},
  {KEY_JOY3BUT4, "Joy3 b4"},
  {KEY_JOY3BUT5, "Joy3 b5"},
  {KEY_JOY3BUT6, "Joy3 b6"},
  {KEY_JOY3BUT7, "Joy3 b7"},
  {KEY_JOY3BUT8, "Joy3 b8"},
  {KEY_JOY3BUT9, "Joy3 b9"},
  {KEY_JOY3BUT10, "Joy3 b10"},
  {KEY_JOY3BUT11, "Joy3 b11"},
  {KEY_JOY3BUT12, "Joy3 b12"},
  {KEY_JOY3BUT13, "Joy3 b13"},
  {KEY_JOY3BUT14, "Joy3 b14"},
  {KEY_JOY3BUT15, "Joy3 b15"},
  
  {KEY_JOY0HATUP, "Joy0 hu"},
  {KEY_JOY0HATRIGHT, "Joy0 hr"},
  {KEY_JOY0HATDOWN, "Joy0 hd"},
  {KEY_JOY0HATLEFT, "Joy0 hl"},
  
  {KEY_JOY1HATUP, "Joy1 hu"},
  {KEY_JOY1HATRIGHT, "Joy1 hr"},
  {KEY_JOY1HATDOWN, "Joy1 hd"},
  {KEY_JOY1HATLEFT, "Joy1 hl"},
  
  {KEY_JOY2HATUP, "Joy2 hu"},
  {KEY_JOY2HATRIGHT, "Joy2 hr"},
  {KEY_JOY2HATDOWN, "Joy2 hd"},
  {KEY_JOY2HATLEFT, "Joy2 hl"},
  
  {KEY_JOY3HATUP, "Joy3 hu"},
  {KEY_JOY3HATRIGHT, "Joy3 hr"},
  {KEY_JOY3HATDOWN, "Joy3 hd"},
  {KEY_JOY3HATLEFT, "Joy3 hl"},
  
  {KEY_JOY0LEFTTRIGGER, "Joy0 lt"},
  {KEY_JOY0RIGHTTRIGGER, "Joy0 rt"},
  
  {KEY_JOY1LEFTTRIGGER, "Joy1 lt"},
  {KEY_JOY1RIGHTTRIGGER, "Joy1 rt"},
  
  {KEY_JOY2LEFTTRIGGER, "Joy2 lt"},
  {KEY_JOY2RIGHTTRIGGER, "Joy2 rt"},
  
  {KEY_JOY3LEFTTRIGGER, "Joy3 lt"},
  {KEY_JOY3RIGHTTRIGGER, "Joy3 rt"},
#endif
};

// Index gamecontrols_e
char * gamecontrolname[num_gamecontrols] =
{
    "nothing",        //a key/button mapped to gc_null has no effect
    "forward",
    "backward",
    "strafe",
    "straferight",
    "strafeleft",
    "speed",
    "turnleft",
    "turnright",
    "fire",
    "use",
    "lookup",
    "lookdown",
    "centerview",
    "mouseaiming",
    "weapon1",
    "weapon2",
    "weapon3",
    "weapon4",
    "weapon5",
    "weapon6",
    "weapon7",
    "weapon8",
    "talkkey",
    "scores",
    "jump",
    "console",
    "nextweapon",
    "prevweapon",
    "bestweapon",
    "inventorynext",
    "inventoryprev",
    "inventoryuse",
    "down",
    "screenshot",
// Mouse and joystick only, Fixed assignment for keyboard.
    "menuesc",  // joystick enter menu, and menu escape key
    "pause",
    "automap",
#ifdef ENABLE_COME_HERE
    "comehere",
#endif
};

#define NUMKEYNAMES (sizeof(keynames)/sizeof(keyname_t))

//
//  Detach any keys associated to the given game control
//  - pass the pointer to the gamecontrol table for the player being edited
void  G_Clear_ControlKeys (int (*setupcontrols)[2], int control)
{
    setupcontrols[control][0] = KEY_NULL;
    setupcontrols[control][1] = KEY_NULL;
}

//
//  Returns the name of a key (or virtual key for mouse and joy)
//  the input value being an keynum
//
char* G_KeynumToString (int keynum)
{
static char keynamestr[8];

    int    j;

    // return a string with the ascii char if displayable
    if (keynum>' ' && keynum<='z' && keynum!=KEY_CONSOLE)
    {
        keynamestr[0] = keynum;
        keynamestr[1] = '\0';
        return keynamestr;
    }

    // find a description for special keys
    for (j=0;j<NUMKEYNAMES;j++)
    {
        if (keynames[j].keynum==keynum)
            return keynames[j].name;
    }

    // create a name for Unknown key
    sprintf(keynamestr,"key%d",keynum);
    return keynamestr;
}


int G_KeyStringtoNum(char *keystr)
{
    int j;
    int len = strlen(keystr);
    byte stat = 0;


    if(keystr[1]==0 && keystr[0]>' ' && keystr[0]<='z')
        return keystr[0];

  retry_search:
    // caseless comparison, uppercase or lowercase input will work
    for (j=0; j<NUMKEYNAMES; j++)
    {
        if (strcasecmp(keynames[j].name,keystr)==0)
            return keynames[j].keynum;
    }
   
    // failed, try to change _ to space
    if( stat == 0 )
    {
        // [WDJ] Got tired trying to fix bugs about a quoted button name
        // within a the quoted script string like alias.
        // Let them specify the button name with underlines and we will
        // equate it to the name with spaces, and it does not require those
        // escaped quotes.
        //   mouse_3  --> "mouse 3"
        for (j=0; j<len; j++ )
        {
            if ( keystr[j] == '_' )
            {
                keystr[j] = ' ';
                stat = 1;
            }
        }
        if ( stat )   goto retry_search;  // found some '_', try it again
    }
    // not found, try to interpret it as a number

    if(strlen(keystr)>3)
        return atoi(&keystr[3]);

    return 0;
}


// [Arcade] Fixed cabinet key layout, per player.
// Keys are the characters the Dvorak layout produces, which is what the
// engine captures (SDL keycodes are layout-aware, see sdl/i_system.c).
// Player 1 uses the left-hand ,aoe cluster (physical WASD), player 2 the
// right-hand yuid cluster (physical TFGH).
//   pair_a = the A/D-position pair: turns in "Look and Move", strafes in "WASD"
//   pair_b = the complementary pair, doing whichever job pair_a is not
typedef struct {
    int  forward, backward, fire, use, nextweapon, prevweapon;
    int  pair_a_left, pair_a_right;
    int  pair_b_left, pair_b_right;
} controlkeys_t;

static const controlkeys_t  scheme_keys[2] =
{
    { ',', 'o', 'h', KEY_SPACE, 'w', 'v',   'a','e',   't','n' },  // player 1
    { 'y', 'i', 'g', 'l',       '/', '=',   'u','d',   'c','r' }   // player 2
};

//  pind : 0 = player 1, 1 = player 2 (splitscreen)
static void ControlScheme_Apply( int pind )
{
    const controlkeys_t * k = & scheme_keys[pind];
    int (* gc)[2] = (pind == 0) ? gamecontrol : gamecontrol2;
    boolean wasd;

    // [Arcade] Custom: the guided setup owns these ten bindings, so step
    // aside entirely and let the config's setcontrol lines stand.  Without
    // this, every config load would stamp the preset back over them --
    // config.cfg lists controlscheme well before the setcontrol lines, but
    // any later toggle of the scheme cvar would still wipe the operator's
    // work, and the intent would be invisible in the file.
    if( cv_controlscheme[pind].value == CS_custom )
        return;

    wasd = (cv_controlscheme[pind].value != 0);

    // Only the actions the scheme defines are touched; weapon keys, run,
    // jump, console, menu keys etc. are left as configured.
#define HS_BIND(act,key)   { gc[act][0] = (key);  gc[act][1] = KEY_NULL; }

    HS_BIND( gc_forward,    k->forward );
    HS_BIND( gc_backward,   k->backward );
    HS_BIND( gc_fire,       k->fire );
    HS_BIND( gc_use,        k->use );
    HS_BIND( gc_nextweapon, k->nextweapon );
    HS_BIND( gc_prevweapon, k->prevweapon );

    HS_BIND( wasd ? gc_strafeleft  : gc_turnleft,     k->pair_a_left );
    HS_BIND( wasd ? gc_straferight : gc_turnright,    k->pair_a_right );
    HS_BIND( wasd ? gc_turnleft    : gc_strafeleft,   k->pair_b_left );
    HS_BIND( wasd ? gc_turnright   : gc_straferight,  k->pair_b_right );

#undef HS_BIND
}

void ControlScheme1_OnChange(void)  { ControlScheme_Apply(0); }
void ControlScheme2_OnChange(void)  { ControlScheme_Apply(1); }


void G_Controldefault(void)
{
    gamecontrol[gc_forward    ][0]=KEY_UPARROW;
    gamecontrol[gc_forward    ][1]=KEY_MOUSE1+2;
    gamecontrol[gc_backward   ][0]=KEY_DOWNARROW;
    gamecontrol[gc_strafe     ][0]=KEY_LALT;
    gamecontrol[gc_strafe     ][1]=KEY_MOUSE1+1;
    gamecontrol[gc_straferight][0]='.';
    gamecontrol[gc_strafeleft ][0]=',';
    gamecontrol[gc_speed      ][0]=KEY_LSHIFT;
    gamecontrol[gc_turnleft   ][0]=KEY_LEFTARROW;
    gamecontrol[gc_turnright  ][0]=KEY_RIGHTARROW;
    gamecontrol[gc_fire       ][0]=KEY_RCTRL;
    gamecontrol[gc_fire       ][1]=KEY_MOUSE1;
    gamecontrol[gc_use        ][0]=KEY_SPACE;
    gamecontrol[gc_lookup     ][0]=KEY_PGUP;
    gamecontrol[gc_lookdown   ][0]=KEY_PGDN;
    gamecontrol[gc_centerview ][0]=KEY_END;
    gamecontrol[gc_mouseaiming][0]='s';
    gamecontrol[gc_weapon1    ][0]='1';
    gamecontrol[gc_weapon2    ][0]='2';
    gamecontrol[gc_weapon3    ][0]='3';
    gamecontrol[gc_weapon4    ][0]='4';
    gamecontrol[gc_weapon5    ][0]='5';
    gamecontrol[gc_weapon6    ][0]='6';
    gamecontrol[gc_weapon7    ][0]='7';
    gamecontrol[gc_weapon8    ][0]='8';
    gamecontrol[gc_talkkey    ][0]='t';
    gamecontrol[gc_scores     ][0]='f';
    gamecontrol[gc_jump       ][0]='/';
    gamecontrol[gc_console    ][0]=KEY_CONSOLE;
    //gamecontrol[gc_nextweapon ][1]=KEY_JOY0BUT4;
    //gamecontrol[gc_prevweapon ][1]=KEY_JOY0BUT5;
    gamecontrol[gc_screenshot ][0]=KEY_SYSREQ;

    if( gamemode == heretic )
    {
        gamecontrol[gc_invnext    ][0] = ']';
        gamecontrol[gc_invprev    ][0] = '[';
        gamecontrol[gc_invuse     ][0] = KEY_ENTER;
        gamecontrol[gc_jump       ][0] = KEY_INS;
        gamecontrol[gc_flydown    ][0] = KEY_DELETE;
    }
    else
    {
        gamecontrol[gc_nextweapon ][0]=']';
        gamecontrol[gc_prevweapon ][0]='[';
    }

#ifdef ENABLE_COME_HERE
    gamecontrol[gc_comehere ][0]='c';
#endif

// Mouse and joystick only, Fixed assignment for keyboard.
    gamecontrol[gc_menuesc    ][0]=KEY_JOY0BUT7; // Start button on Xbox360 controllers
    gamecontrol[gc_pause      ][0]=KEY_JOY0BUT6; // Back button on Xbox360 controllers
    gamecontrol[gc_automap    ][0]=KEY_JOY0BUT8;
}

void G_SaveKeySetting(FILE *f)
{
    int i;

    for(i=1;i<num_gamecontrols;i++)
    {
        fprintf(f,"setcontrol \"%s\" \"%s\"",
                gamecontrolname[i],
                G_KeynumToString(gamecontrol[i][0]));

        if(gamecontrol[i][1])
            fprintf(f," \"%s\"\n",
                        G_KeynumToString(gamecontrol[i][1]));
        else
            fprintf(f,"\n");
    }

    for(i=1;i<num_gamecontrols;i++)
    {
        fprintf(f, "setcontrol2 \"%s\" \"%s\"",
                gamecontrolname[i],
                G_KeynumToString(gamecontrol2[i][0]));

        if(gamecontrol2[i][1])
            fprintf(f, " \"%s\"\n",
                    G_KeynumToString(gamecontrol2[i][1]));
        else
            fprintf(f,"\n");
    }

#ifdef JOYSTICK_SUPPORT    
    // Writes the joystick axis binding commands to the config file.
    for (i=0; i<num_joybindings; i++)
    {
        joybinding_t jb = joybindings[i];
        fprintf(f, "bindjoyaxis %d %d %d %d %f\n",
                jb.joynum, jb.axisnum, jb.playnum, (int)(jb.action), jb.scale);
    }
#endif
}

void G_CheckDoubleUsage(int keynum)
{
    if( cv_controlperkey.value==1 )
    {
        int i;
        for(i=0;i<num_gamecontrols;i++)
        {
            if( gamecontrol[i][0]==keynum )
                gamecontrol[i][0]= KEY_NULL;
            if( gamecontrol[i][1]==keynum )
                gamecontrol[i][1]= KEY_NULL;
            if( gamecontrol2[i][0]==keynum )
                gamecontrol2[i][0]= KEY_NULL;
            if( gamecontrol2[i][1]==keynum )
                gamecontrol2[i][1]= KEY_NULL;
        }
    }
}

static
void setcontrol(int (*gc)[2], const char * cstr)
{
    int numctrl;
    char *namectrl;
    int keynum;
    COM_args_t  carg;
    
    COM_Args( &carg );
   
    if ( carg.num!= 3 && carg.num!=4 )
    {
        CONS_Printf ("setcontrol%s <controlname> <keyname> [<2nd keyname>]\n", cstr);
        return;
    }

    namectrl=carg.arg[1];
    for(numctrl=0;numctrl<num_gamecontrols
                  && strcasecmp(namectrl,gamecontrolname[numctrl])
                 ;numctrl++);
    if(numctrl==num_gamecontrols)
    {
        CONS_Printf("Control '%s' unknown\n",namectrl);
        return;
    }
    keynum=G_KeyStringtoNum(carg.arg[2]);
    G_CheckDoubleUsage(keynum);
    gc[numctrl][0]=keynum;

    if(carg.num==4)
        gc[numctrl][1]=G_KeyStringtoNum(carg.arg[3]);
    else
        gc[numctrl][1]=0;
}

void Command_Setcontrol_f(void)
{
    setcontrol(gamecontrol, "");
}

void Command_Setcontrol2_f(void)
{
    setcontrol(gamecontrol2, "2");
}



//! Magically converts a console command to a joystick axis binding. Also releases bindings.
void Command_BindJoyaxis_f()
{
#ifdef JOYSTICK_SUPPORT
  joybinding_t jb;
  COM_args_t  carg;
    
  COM_Args( &carg );

  if(carg.num == 1)
  { // Print bindings.
    CONS_Printf("%d joysticks found.\n", num_joysticks);
    if(num_joybindings == 0)
    {
      CONS_Printf("No joystick axis bindings defined.\n");
      return;
    }
    CONS_Printf("Current axis bindings.\n");
    unsigned int i;
    for(i=0; i<num_joybindings; i++)
    {
      jb = joybindings[i];
      CONS_Printf("%d %d %d %d %f\n", jb.joynum, jb.axisnum,
                  jb.playnum, (int)jb.action, jb.scale);
    }
    return;
  }

  if (carg.num == 4 || carg.num > 6)
  {
    CONS_Printf("bindjoyaxis [joynum] [axisnum] [playnum] [action] [scale]  to bind\n"
                "bindjoyaxis [joynum] [axisnum]  to unbind\n");
    return;
  }

  jb.joynum  = atoi( carg.arg[1] );
  if(jb.joynum < 0 || jb.joynum >= num_joysticks)
  {
    CONS_Printf("Attempting to bind/release non-existent joystick %d.\n", jb.joynum);
    return;
  }

  jb.axisnum = (carg.num >= 3) ? atoi( carg.arg[2] ) : -1;
  if(jb.axisnum < -1 || jb.axisnum >= I_JoystickNumAxes(jb.joynum))
  {
    CONS_Printf("Attempting to bind/release non-existent axis %d.\n", jb.axisnum);
    return;
  }

  if (carg.num == 3)
  { // release binding(s)
    /* Takes one or two parameters. The first one is the joystick number
       and the second is the axis number. If either is not specified, all
       values are assumed to match.
    */

    int num_keep_bindings = 0;
    joybinding_t keep_bindings[MAX_JOYBINDINGS];

    if(num_joybindings == 0)
    {
      CONS_Printf("No bindings to unset.\n");
      return;
    }

    unsigned int i;
    for(i=0; i<num_joybindings; i++)
    {
      joybinding_t temp = joybindings[i];
      if((jb.joynum == temp.joynum) &&
         (jb.axisnum == -1 || jb.axisnum == temp.axisnum))
        continue; // We have a binding to prune.

      keep_bindings[num_keep_bindings++] = temp; // keep it
    }

    // We have the new bindings.
    if (num_keep_bindings == num_joybindings)
    {
      CONS_Printf("No bindings matched the parameters.\n");
      return;
    }

    // replace the bindings
    num_joybindings = num_keep_bindings;
    for(i=0; i<num_keep_bindings; i++)
      joybindings[i] = keep_bindings[i];
  }
  else
  { // create a binding
    jb.playnum = atoi( carg.arg[3] );
    // carg.arg[0..3] only, use COM_Argv for others
    int j_action  = atoi(COM_Argv(4));
    if (carg.num == 6)
      jb.scale = atof(COM_Argv(5));
    else
      jb.scale = 1.0f;

    if(j_action < 0 || j_action >= num_joyactions)
    {
      CONS_Printf("Attempting to bind non-existent action %d.\n", j_action);
      return;
    }
    jb.action = (joyactions_e)j_action;  // test actual value before convert to enum

    // Overwrite existing binding, if any. Otherwise just append.
    unsigned int i;
    for(i=0; i<num_joybindings; i++)
    {
      joybinding_t j2 = joybindings[i];
      if(j2.joynum == jb.joynum && j2.axisnum == jb.axisnum)
      {
        joybindings[i] = jb;
        CONS_Printf("Joystick binding modified.\n");
        return;
      }
    }
    // new binding
    if (num_joybindings < MAX_JOYBINDINGS)
    {
        joybindings[num_joybindings++] = jb;
        CONS_Printf("Joystick binding added.\n");
    }
    else
      CONS_Printf("Maximum number of joystick bindings reached.\n");
  }
#endif
}
