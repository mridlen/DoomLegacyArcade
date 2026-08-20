// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_menu.c 1727 2025-02-07 05:03:05Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: m_menu.c,v $
// Revision 1.55  2005/12/20 14:58:25  darkwolf95
// Monster behavior CVAR - Affects how monsters react when they shoot each other
//
// Revision 1.54  2004/09/12 19:40:06  darkwolf95
// additional chex quest 1 support
//
// Revision 1.53  2004/07/27 08:19:36  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.52  2003/08/11 13:50:01  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.51  2003/03/22 22:35:59  hurdler
//
// Revision 1.50  2002/09/27 16:40:08  tonyd
// First commit of acbot
//
// Revision 1.49  2002/09/12 20:10:50  hurdler
// Added some cvars
//
// Revision 1.48  2002/08/24 22:42:03  hurdler
// Apply Robert Hogberg patches
//
// Revision 1.47  2001/12/31 13:47:46  hurdler
// Add setcorona FS command and prepare the code for beta 4
//
// Revision 1.46  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.45  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.44  2001/11/02 23:29:13  judgecutor
// Fixed "secondary player controls" bug
//
// Revision 1.43  2001/11/02 21:44:05  judgecutor
// Added Frag's weapon falling
//
// Revision 1.42  2001/08/20 21:37:34  hurdler
// fix palette in splitscreen + hardware mode
//
// Revision 1.41  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.40  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.39  2001/06/10 21:16:01  bpereira
// Revision 1.38  2001/05/27 13:42:47  bpereira
// Revision 1.37  2001/05/16 22:00:10  hurdler
// Revision 1.36  2001/05/16 21:21:14  bpereira
//
// Revision 1.35  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.34  2001/04/29 14:25:26  hurdler
// Revision 1.33  2001/04/01 17:35:06  bpereira
// Revision 1.32  2001/03/03 06:17:33  bpereira
// Revision 1.31  2001/02/24 13:35:20  bpereira
// Revision 1.30  2001/02/10 12:27:14  bpereira
//
// Revision 1.29  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.28  2000/11/26 20:36:14  hurdler
// Adding autorun2
//
// Revision 1.27  2000/10/21 08:43:29  bpereira
//
// Revision 1.26  2000/10/17 10:09:27  hurdler
// Update master server code for easy connect from menu
//
// Revision 1.25  2000/10/16 20:02:29  bpereira
// Revision 1.24  2000/10/08 13:30:01  bpereira
// Revision 1.23  2000/10/02 18:25:45  bpereira
//
// Revision 1.22  2000/10/01 15:20:23  hurdler
// Add private server
//
// Revision 1.21  2000/10/01 10:18:17  bpereira
//
// Revision 1.20  2000/10/01 09:09:36  hurdler
// Put the md2 code in #ifdef TANDL
//
// Revision 1.19  2000/09/15 19:49:22  bpereira
//
// Revision 1.18  2000/09/08 22:28:30  hurdler
// merge masterserver_ip/port in one cvar, add -private
//
// Revision 1.17  2000/09/02 15:38:24  hurdler
// Add master server to menus (temporaray)
//
// Revision 1.16  2000/08/31 14:30:55  bpereira
//
// Revision 1.15  2000/04/24 15:10:56  hurdler
// Support colormap for text
//
// Revision 1.14  2000/04/23 00:29:28  hurdler
// fix a small bug in skin color
//
// Revision 1.13  2000/04/23 00:25:20  hurdler
// fix a small bug in skin color
//
// Revision 1.12  2000/04/22 21:12:15  hurdler
//
// Revision 1.11  2000/04/22 20:27:35  metzgermeister
// support for immediate fullscreen switching
//
// Revision 1.10  2000/04/16 18:38:07  bpereira
// Revision 1.9  2000/04/13 16:26:41  hurdler
//
// Revision 1.8  2000/04/12 19:31:37  metzgermeister
// added use_mouse to menu
//
// Revision 1.7  2000/04/08 17:29:24  stroggonmeth
//
// Revision 1.6  2000/04/07 23:11:17  metzgermeister
// added mouse move
//
// Revision 1.5  2000/04/04 10:44:00  hurdler
//
// Revision 1.4  2000/04/04 00:32:46  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.3  2000/03/23 22:54:00  metzgermeister
// added support for HOME/.legacy under Linux
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      DOOM selection menu, options, episode etc.
//      Sliders and icons. Kinda widget stuff.
//
// NOTE:
//      Drawing is through V_SetupDraw() and V_DrawScaledPatch()
//      so that the menu is scaled to the screen size. The scaling is always
//      an integer multiple of the original size, so that the graphics look
//      good.
//
//-----------------------------------------------------------------------------

#include <unistd.h>
#include <fcntl.h>

#include "doomincl.h"
#include "am_map.h"
#include "dstrings.h"
// [Arcade] level pack scan (the dirent include further down is on the
// non-FTW path only, which this build does not take)
#include <sys/types.h>
#include <dirent.h>

#include "d_main.h"

#include "console.h"

#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "hs_stuff.h"
#include "g_input.h"

#include "m_argv.h"

#ifdef SMIF_X11
#include "linux_x/lx_ctrl.h"
  // SOUND_DEVICE_OPTION
#endif

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

#include "m_menu.h"
#include "v_video.h"
#include "i_video.h"

#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_fab.h"
#include "p_chex.h"
  // Chex_safe_pictures

#include "p_saveg.h"
  // savegame header read

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "d_clisrv.h"
  // server1, server2, server3
#include "mserv.h"
#include "p_inter.h"
#include "m_misc.h"
  // config


boolean                 menuactive;

// [WDJ] mouse sensitivity, 30 for old feel, 40..50 to reduce
#define MENU_MOUSE_TRIG   40

#define SKULLXOFF       -32
#define LINEHEIGHT       16
#define STRINGHEIGHT     10
#define FONTBHEIGHT      20
#define SMALLLINEHEIGHT   8
#define SLIDER_RANGE     10
#define SLIDER_WIDTH    (8*SLIDER_RANGE+6)
#define MAXSTRINGLENGTH  32

// [WDJ] Definition of slot.
// SLOT is the number attached to the savegame name.
// MSLOT is the number of MENU_SLOT.
// The savegamedisp table is indexed by slotindex [0..5],
// which for 99 savegames and with directories (slotindex=[1..6]),
// is different than the slot number [0..99].
// Entry savegamedisp[6] is reserved for quickSave.

// Save and Load menus must match the SAVEGAME_NUM_SLOT
#define SAVEGAME_NUM_MSLOT  6

// Map and Time on left side, otherwise on the right side
#define SAVEGAME_MTLEFT

// [WDJ] Fonts are variable width, so it does not actually overlap, most times.
#define SAVEGAME_MTLEN  12

#ifdef SAVEGAME_MTLEFT
// position in the line of map name and time
#define SAVE_DESC_POS   12
#define SAVE_DESC_XPOS  (SAVE_DESC_POS*8)
#define SAVELINELEN (SAVE_DESC_POS+SAVESTRINGSIZE-2)
#else
// position in the line of map name and time
#define SAVE_MT_POS     22
#define SAVELINELEN (SAVE_MT_POS+SAVEGAME_MTLEN)
#endif

#ifdef SAVEGAMEDIR
char  savegamedir[SAVESTRINGSIZE] = "";  // default is main legacy dir
#endif

typedef struct
{
#ifdef SAVEGAME99
    byte  savegameid;	// 0..99, else invalid
#endif
    char  desc[SAVESTRINGSIZE];
    char  levtime[SAVEGAME_MTLEN];
} savegame_disp_t;

// disp slots 0..5, and an extra 6=quicksave
static savegame_disp_t    savegamedisp[SAVEGAME_NUM_MSLOT+1];

static int   slotindex;  // MSLOT, used for reading and editing slot desc, 0..5
// quicksave_slotid: -1 = no slot picked!, -2 = select slot now, else is slot
static int   quicksave_slotid; // -1,-2, 0..5 or 0..99 (savegameid)

#if defined SAVEGAMEDIR || defined SAVEGAME99
#define     QUICKSAVE_INDEX   SAVEGAME_NUM_MSLOT
static int     scroll_index = 0;  // scroll position
static void    (*scroll_callback)(int amount) = NULL; // call to scroll
static void    (*delete_callback)(int ch) = NULL; // call to delete
#else
// otherwise the slot and index are the same
#define     QUICKSAVE_INDEX   quicksave_slotid
#endif


// menu string edit, used for entering a savegame string
static boolean edit_enable;
static int     edit_index;  // which char we're editing
// menu edit
static char    edit_buffer[SAVESTRINGSIZE];
static void    (*edit_done_callback)(void) = NULL;  // call upon edit done

// clear the savegamedisp from mslot to SAVEGAME_NUM_MSLOT
static
void clear_remaining_savegamedisp( int sgslot )
{
    // fill out as empty any remaining menu positions
    while( sgslot < SAVEGAME_NUM_MSLOT )  // do not overrun quicksave
    {
        savegamedisp[sgslot].desc[0] = '\0';
        savegamedisp[sgslot].levtime[0] = '\0';
#ifdef SAVEGAME99
        savegamedisp[sgslot].savegameid = 255;   // invalid
#endif
        sgslot ++;
    }
}

static
void setup_net_savegame( void )
{
    strcpy( savegamedir, "net" );	// default for network play
}

// flags for items in the menu
typedef enum {
// menu handle (what we do when key is pressed)
  IT_TYPE =  0x000E,   // field
// TYPE field values
  IT_SPACE =      0,   // no handling
  IT_CALL =       2,   // call the function
  IT_ARROWS =     4,   // call function with 0 for left arrow and 1 for right arrow in param
  IT_KEYHANDLER = 6,   // call with the key in param
  IT_SUBMENU =    8,   // go to sub menu
  IT_CVAR =      10,   // handle as a cvar
  IT_MSGHANDLER =12,   // same as key but with event and sometime can handle y/n key (special for message

  IT_DISPLAY =     0x00F0,  // field
// DISPLAY field values
  IT_NOTHING =          0,  // space
  IT_PATCH =       0x0010,  // a patch or a string with big font
  IT_STRING =      0x0020,  // little string (spaced with 10)
  IT_WHITESTRING = 0x0030,  // little string in white
  IT_DYBIGSPACE =  0x0040,  // same as nothing
  IT_DYLITLSPACE = 0x0050,  // little space
  IT_STRING2 =     0x0060,  // a simple string
  IT_GRAYPATCH =   0x0070,  // grayed patch or big font string
  IT_BIGSLIDER =   0x0080,  // volume sound use this
  IT_NODRAW =      0x0090,  // [Arcade] not drawn, and does not occupy a line
  IT_EXTERNAL  =   0x00F0,  // nothing, not even a skull

//consvar specific
  IT_CVARTYPE =	  0x0700,   // field
// CVARTYPE values
  IT_CV_NORMAL =       0,
  IT_CV_SLIDER =  0x0100,
  IT_CV_STRING =  0x0200,
  IT_CV_NOPRINT = 0x0300,
  IT_CV_NOMOD =   0x0400,
  IT_CV_DELAY =   0x0500,  // delayed effect

  IT_OPTION  = 0x3000,    // field
  IT_YOFFSET = 0x1000,    // alphaKey is offset
  IT_KEYID   = 0x2000,    // alphaKey is id

// in short for some common use
  IT_BIGSPACE =   (IT_SPACE  | IT_DYBIGSPACE),
  IT_LITLSPACE =  (IT_SPACE  | IT_DYLITLSPACE),
  IT_CONTROL =    (IT_STRING2| IT_CALL | IT_KEYID),
  IT_CVARMAX =    (IT_CVAR   | IT_CV_NOMOD),
  IT_DISABLED =   (IT_SPACE  | IT_GRAYPATCH),
  // [Arcade] Like IT_DISABLED, but the line vanishes entirely instead of
  // being drawn grayed.  IT_SPACE type means the cursor already skips it.
  IT_HIDDEN =     (IT_SPACE  | IT_NODRAW),
} menu_control_e;

typedef void (*menufunc_t)(int choice);

// [smite] dirty hack, contains a second parameter to IT_KEYHANDLER functions
// (int choice is the key)
static unsigned char input_char;
// Return 0= continue, 1= intercept key, 2= testing.
static byte (*key_handler2)(int key) = NULL;  // keyboard intercept 

static inline boolean is_printable(char c) { return c >= ' ' && c <= '~'; }

typedef union
{
    // [WDJ] can only init to the first union item.
    void             * init_void;
    struct menu_s    * submenu;               // IT_SUBMENU
    consvar_t        * cvar;                  // IT_CVAR
    menufunc_t         routine;  // IT_CALL, IT_KEYHANDLER, IT_ARROWS
} itemaction_t;

//
// MENU TYPEDEFS
//
typedef struct menuitem_s
{
    // show IT_xxx
    uint16_t  status;

    char     * patch;
    char     * text;  // used when FONTBxx lump is found

    // FIXME: should be itemaction_t !!!
    // [WDJ]  Cannot fix it, all those init will not work.
    // Can only init to the first union item, when it is anon embedded union,
    // and then the init union item must be in { }.
    void     * itemaction;

    // hotkey in menu
    // or y of the item when IT_YOFFSET (uses M_DrawGenericMenu)
    // or in control menus, the control to change (uses M_DrawControl)
    byte      alphaKey;
} menuitem_t;

typedef struct menu_s
{
    char          * menutitlepic;
    const char    * menutitle;              // title as string for display with fontb if present
    menuitem_t    * menuitems;              // menu items
    void            (*drawroutine)(void);   // draw routine
    boolean         (*quitroutine)(void);   // called before quit a menu return true if we can
    uint16_t        numitems;               // # of menu items
    uint16_t        x;
    uint16_t        y;                      // x,y of menu
    byte            lastOn;                 // last item user was on in menu
} menu_t;

#define  NUM_MENUSTACK  8
menu_t * menustack[ NUM_MENUSTACK+1 ];
byte  menucnt = 0;

// current menudef
static menu_t  * currentMenu = NULL;
static menuitem_t * menuline = NULL; // menu line that invoked a call or submenu
static byte    itemOn;             // 0..40, menu item skull is on
static int8_t  skullAnimCounter;   // 0..10, skull animation counter
static byte    whichSkull;         // 0,1 which skull to draw, >128 off
static int     SkullBaseLump;


#ifdef CONFIG_MENU_PAGE
static byte   menu_cfg = 0; // when non-zero, only those config are shown
static byte   menu_cfg_editing = 0;  // to disable live changes
static const char *  menu_cfg_string[4] = { "", "MAIN", "DRAWMODE", "OTHER" };
#endif

// graphic name of skulls
static char    skullName[2][9] = {"M_SKULL1","M_SKULL2"};

// [WDJ] menu sounds
static sfxid_t menu_sfx_updown = sfx_menuud;
static sfxid_t menu_sfx_val = sfx_menuva;
static sfxid_t menu_sfx_enter = sfx_menuen;
static sfxid_t menu_sfx_open = sfx_menuop;
static sfxid_t menu_sfx_esc = sfx_menuop;
static sfxid_t menu_sfx_action = sfx_menuac;

CV_PossibleValue_t menusound_cons_t[] =
  {{0,"Auto"}, {1,"Legacy"}, {2,"Doom"}, {3,"Heretic"}, {0,NULL}};
static void CV_menusound_OnChange(void);
consvar_t cv_menusound = {"menusound", "1", CV_SAVE | CV_CALL, menusound_cons_t, CV_menusound_OnChange };

// [Arcade] Whether the cabinet has a second set of controls at all.  An
// operator setting, so it is only saved from a -devmode session; applied in
// M_Configure because the config that carries it is not loaded until well
// after M_Init.  Off hides Two Player Game and the player 2 config screens.
consvar_t cv_twoplayer = {"twoplayer", "1", CV_SAVE, CV_OnOff };

// [Arcade] How many sets of controls the cabinet physically has, 1..4.  A
// multicade panel may have three or four.  Operator setting, saved only from
// a -devmode session, like cv_twoplayer above.
//
// This is the count of players that *join* on this machine, which is a
// separate thing from cv_splitscreen, the two-view render toggle.  Players
// past the second currently join, take controls and play, but have no
// viewport of their own -- the renderer splits into at most two halves (see
// r_main.c "rdraw_viewheight >>= 1" and r_draw.c's ylookup1/ylookup2).
// Raising this past 2 is only useful once that is addressed.
CV_PossibleValue_t localplayers_cons_t[] = {{1,"1"},{2,"2"},{3,"3"},{4,"4"},{0,NULL}};
consvar_t cv_localplayers = {"localplayers", "1", CV_SAVE, localplayers_cons_t };

// [Arcade] Seconds the join screen waits before starting with whoever has
// pressed in.  Operator setting like the two above.  0 skips the wait, which
// starts the game with panel 1 alone -- useful on a single panel cabinet that
// still has cv_localplayers set high for testing.
CV_PossibleValue_t jointime_cons_t[] = {{0,"MIN"},{60,"MAX"},{0,NULL}};
consvar_t cv_jointime = {"jointime", "20", CV_SAVE, jointime_cons_t };

// [Arcade] Which game the cabinet boots into, instead of whichever IWAD the
// engine's search happens to find first.  Also an operator setting, saved only
// from a -devmode session.
//
// The PossibleValue strings are the game_desc_table idstr names on purpose:
// config.cfg stores a cvar's *label*, and D_Read_Default_Game has to parse
// that line out of the file by hand long before the config is loaded, so the
// stored text needs to be usable as-is.  They are also exactly what -game
// takes, which makes the setting self-documenting for an operator.
//
// Not filtered by which IWADs are installed -- the list is fixed and startup
// falls back to the normal search if the chosen game is missing, which keeps
// this menu independent of the wad search.
CV_PossibleValue_t defaultgame_cons_t[] = {
  {0, "None"}, {1, "doomu"}, {2, "doom2"}, {3, "plutonia"}, {4, "tnt"}, {0, NULL} };
consvar_t cv_defaultgame = {"defaultgame", "None", CV_SAVE, defaultgame_cons_t };

static
void CV_menusound_OnChange(void)
{
    byte menusound = cv_menusound.EV;

    switch ( gamemode )
    {
      case doom2_commercial:
      case doom_shareware:
      case doom_registered:
      case ultdoom_retail:
        if ( menusound == 0 || menusound == 3 )
           menusound = 2;
        break;
      case heretic:
        if ( menusound == 0 || menusound == 2 )
           menusound = 3;
        break;
      case chexquest1:
        if ( menusound == 0 || menusound == 3 )
           menusound = 1;
        break;
      default:
        break;
    }
    switch ( menusound )
    {
      default:
      case 0: // auto
      case 1: // Legacy
        menu_sfx_updown = sfx_menuud;
        menu_sfx_val = sfx_menuva;
        menu_sfx_enter = sfx_menuen;
        menu_sfx_open = sfx_menuop;
        menu_sfx_esc = sfx_menuop;
        menu_sfx_action = sfx_menuac;
        break;
      case 2: // Doom
        //Boom
        // help, save, load, volume menu open = sfx_swtchn
        // backspace = sfx_swtchn
        // menu action = sfx_itemup
        // next menu = sfx_swtchx
        menu_sfx_updown = sfx_pstop;
        menu_sfx_val = sfx_stnmov;
        menu_sfx_enter = sfx_pistol;
        menu_sfx_open = sfx_swtchn;
        menu_sfx_esc = sfx_swtchx;
        menu_sfx_action = sfx_swtchx;
        break;
      case 3: // Heretic
        //heretic
        // quit, chat val = sfx_chat;
        // chat keys = sfx_keyup;
        // info, save, load, volume menu open  = sfx_dorcls;
        // enter, activate menu, deactivate menu = sfx_dorcls;
        // escape = none;
        // backspace = sfx_switch;
        menu_sfx_updown = sfx_swtchx;  // sfx_switch
        menu_sfx_val = sfx_keyup;
        menu_sfx_enter = sfx_dorcls;
        menu_sfx_open = sfx_dorcls;
        menu_sfx_esc = sfx_menuva;  // none
//        menu_sfx_action = sfx_chat;  // don't have sfx_chat
        menu_sfx_action = sfx_menuac;
        break;
    } 
}


#if defined(MAPTHING_ADJUST) || defined(MONSTER_VARY) || defined(ENABLE_TELE_CONTROL) || defined(DOORDELAY_CONTROL) || defined(ENABLE_SLOW_REACT)
# ifndef MAPADJUST_MENU
#   define MAPADJUST_MENU
# endif
#endif

//
// PROTOTYPES
//
static void M_Draw_SaveLoadBorder(int x, int y, boolean longer);
static void Push_Setup_Menu(menu_t *menudef);
static void Pop_Menu( void );

void M_DrawTextBox (int x, int y, int width, int lines);     //added:06-02-98:
static void M_DrawThermo(int x,int y,consvar_t *cv);
#if 0
static void M_DrawEmptyCell(menu_t *menu,int item);
static void M_DrawSelCell(menu_t *menu,int item);
#endif

static void M_DrawSlider (int x, int y, int range);
static void M_CentreText(int y, char* string); //added:30-01-98:writetext centered

static void M_StopMessage(int choice);
// [Arcade] Not static: the idle timeout closes menus from g_game.c.
// The definition below was already non-static; only this declaration said so.
void M_Clear_Menus (boolean callexitmenufunc);
static int  M_StringHeight(char* string);
static void M_GameOption(int choice);
static void M_AdvOption(int choice);
#ifdef MAPADJUST_MENU
static void M_MapAdjust(int choice);
#endif
static void M_BotOption(int choice);
static void M_NetOption(int choice);
//28/08/99: added by Hurdler
static void M_OpenGLOption(int choice);
static void M_PlayerDirector(int choice);

menu_t GameSelectDef;   // [Arcade] IWAD switcher
menu_t RecLayoutDef;    // [Arcade] recommended panel layout, informational
menu_t MainDef, SoundDef, EpiDef, NewDef,
  VideoModeDef, VideoOptionsDef, DrawmodeDef, MouseOptionsDef,
  PlayerDirectorDef, PlayerOptionsDef,
  SingleMultiDef, TwoPlayerDef, MultiPlayerDef, SetupMultiPlayerDef,
  ReadDef2, ReadDef1, SaveDef, LoadDef, 
  ControlDef, ControlDef2, ControlDef3, MControlDef,
#ifdef JOYSTICK_SUPPORT
  JoystickOptionsDef,
#endif
  OptionsDef, EffectsOption1Def, EffectsOption2Def, AdvOption1Def, AdvOption2Def,
  GameOptionDef, MenuOptionsDef, LightingDef, BotDef,
  NetOptionDef, ConnectOptionDef, ServerOptionsDef,
  MPOptionDef;

extern menu_t  SingleLevelDef;   // [Arcade]
extern menu_t  CheatsDef;        // [Arcade]


//===========================================================================
//Generic Stuffs (more easy to create menus :))
//===========================================================================

static
void M_DrawMenuTitle(void)
{
    if( FontBBaseLump && currentMenu->menutitle )
    {
        int xtitle = (BASEVIDWIDTH-V_TextBWidth(currentMenu->menutitle))/2;
        int ytitle = (currentMenu->y-V_TextBHeight(currentMenu->menutitle))/2;
        if(xtitle<0) xtitle=0;
        if(ytitle<0) ytitle=0;

        V_DrawTextB(currentMenu->menutitle, xtitle, ytitle);
    }
    else
    if( currentMenu->menutitlepic )
    {
        patch_t* tp = W_CachePatchName(currentMenu->menutitlepic,PU_CACHE);  // endian fix
#if 1
        //SoM: 4/7/2000: Old code was causing problems with large graphics.
//        int xtitle = (vid.width / 2) - (p->width / 2);
//        int ytitle = (y-p->height)/2;
        int xtitle = 94;
        int ytitle = 2;
#else
        int xtitle = (BASEVIDWIDTH - tp->width)/2;
        int ytitle = (currentMenu->y - tp->height)/2;
#endif

        if(xtitle<0) xtitle=0;
        if(ytitle<0) ytitle=0;
        V_DrawScaledPatch (xtitle, ytitle, tp);
    }
    else
    if( currentMenu->menutitle && !use_font1 )
    {
        int xtitle = (BASEVIDWIDTH-V_StringWidth(currentMenu->menutitle))/2;
        int ytitle = (currentMenu->y - 16)/2;
        if(xtitle<0) xtitle=0;
        if(ytitle<1) ytitle=1;

        V_DrawString(xtitle, ytitle, 0, currentMenu->menutitle);
    }
}

static
void M_DrawGenericMenu(void)
{
    fontinfo_t * fip = V_FontInfo();
    menuitem_t * mip;
    int x, y, w;
    int cursory=0;
    byte i;

    // DRAW MENU
    // Draw to screen0, scaled
    x = currentMenu->x;
    y = currentMenu->y;

    // draw title (or big pic)
    M_DrawMenuTitle();

#ifdef CONFIG_MENU_PAGE
    if( menu_cfg )
    {
        V_DrawString( 2, 1, V_WHITEMAP, menu_cfg_string[menu_cfg]);
        V_DrawString( BASEVIDWIDTH - (14*8), 1, V_WHITEMAP, "Insert Delete");
    }
#endif
   
    for (i=0; i<currentMenu->numitems; i++)
    {
        mip = & currentMenu->menuitems[i];
        // handle Y offsets independent of IT_STRING and IT_WHITESTRING
        if( ((mip->status & IT_OPTION) == IT_YOFFSET) && mip->alphaKey )
        {
            y = currentMenu->y + mip->alphaKey;
        }
        if (i==itemOn)
            cursory=y;

        switch (mip->status & IT_DISPLAY)
        {
           case IT_NODRAW:
               // [Arcade] hidden: draw nothing, and do not advance y
               break;
           case IT_PATCH  :
               if( FontBBaseLump && mip->text )
               {
                   V_DrawTextB(mip->text, x, y);
                   y += FONTBHEIGHT-LINEHEIGHT;
               }
               else 
               if( mip->patch && mip->patch[0] )
               {
                   V_DrawScaledPatch_Name (x,y, mip->patch );
               }
               // add lineheight	   
           case IT_NOTHING:
           case IT_EXTERNAL:
           case IT_DYBIGSPACE:
               y += LINEHEIGHT;
               break;
           case IT_BIGSLIDER :
               M_DrawThermo( x, y, (consvar_t *)mip->itemaction);
               y += LINEHEIGHT;
               break;
           case IT_STRING :
           case IT_WHITESTRING :
#ifdef CONFIG_MENU_PAGE
               if( menu_cfg && (mip->status & IT_TYPE) == IT_CVAR )
               {	   
                   consvar_t * cv = (consvar_t *) mip->itemaction;
                   if( ! (cv->flags & CV_SAVE) )
                       goto finish_string_line;  // not configurable
               }
#endif
               if( (mip->status & IT_DISPLAY)==IT_STRING ) 
                   V_DrawString(x,y,0,mip->text);
               else
                   V_DrawString(x,y,V_WHITEMAP,mip->text);

               // Cvar specific handling
               switch (mip->status & IT_TYPE)
               {
                 case IT_CVAR:
                 {
                   consvar_t * cv = (consvar_t *) mip->itemaction;
                   const char * cvsp = cv->string;
#ifdef CONFIG_MENU_PAGE
                   if( menu_cfg && ((cv->state & CS_CONFIG) != menu_cfg ) )
                   {
                       cvsp = CV_Get_Config_string( cv, menu_cfg );
                       if( ! cvsp )  goto finish_string_line;  // not present
                   }
#endif
                   switch (mip->status & IT_CVARTYPE)
                   {
                       case IT_CV_SLIDER :
                           M_DrawSlider (BASEVIDWIDTH - x - SLIDER_WIDTH, y,
                                         ( (cv->value - cv->PossibleValue[0].value) * 100 /
                                         (cv->PossibleValue[1].value - cv->PossibleValue[0].value)));
                       case IT_CV_NOPRINT: // color use this 
                           break;
                       case IT_CV_STRING:
                           w = V_StringWidth( cvsp );
                           if( use_font1 )
                           {
                               // Setup is centered, but this needs left justify.
                               M_DrawTextBox(-BASEVIDWIDTH/2,y+12,BASEVIDWIDTH/7,1);
                               const char * s = cvsp;
                               while( *s && (w > BASEVIDWIDTH - 8) )
                               {
                                   w -= fip->xinc;
                                   s++;
                               }
                               V_DrawString (x+8,y+12,0, cvsp);
//                             if( skullAnimCounter<4 && i==itemOn )
                               if( i==itemOn )
                                  V_DrawCharacter( x+8+w, y+12,  '_' | 0x80);  // white
                           }
                           else
                           {
                               M_DrawTextBox(x,y+4,MAXSTRINGLENGTH,1);
                               V_DrawString (x+8,y+12,0, cvsp);
                               if( skullAnimCounter<4 && i==itemOn )
                                  V_DrawCharacter( x+8+w, y+12,  '_' | 0x80);  // white
                           }
                           y+=16;
                           break;
                       default:
                           if( ! cvsp )
                           {
                               I_SoftError("GenMenu: cvar NULL string %s\n", cv->name );
                               break;
                           }
                           V_DrawString(BASEVIDWIDTH - x - V_StringWidth( cvsp ),
                                        y, V_WHITEMAP, 
                                        cvsp );
                           break;
                   }
                   break;
                 }
               } // switch IT_TYPE
#ifdef CONFIG_MENU_PAGE
          finish_string_line:
#endif
               y+=STRINGHEIGHT;
               break;
           case IT_STRING2:
               V_DrawString (x,y,0,mip->text);
           case IT_DYLITLSPACE:
               y+=SMALLLINEHEIGHT;
               break;
           case IT_GRAYPATCH:
               if( FontBBaseLump && mip->text )
               {
                   V_DrawTextBGray(mip->text, x, y);
                   y += FONTBHEIGHT-LINEHEIGHT;
               }
               else 
               if( mip->patch &&
                   mip->patch[0] )
               {
                   V_DrawMappedPatch_Name (x,y, mip->patch, graymap );
               }
               y += LINEHEIGHT;
               break;

        } // switch IT_DISPLAY
    }  // for menu lines

    if( whichSkull > 1 )  return;

    // DRAW THE SKULL CURSOR
    if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY)==IT_PATCH)
      ||((currentMenu->menuitems[itemOn].status & IT_DISPLAY)==IT_NOTHING) )
    {
        V_DrawScaledPatch_Name(currentMenu->x + SKULLXOFF, cursory-5,
                          skullName[whichSkull] );
    }
    else
    if (skullAnimCounter<4 * NEWTICRATERATIO)  //blink cursor
    {
        V_DrawCharacter(currentMenu->x - 10, cursory,
                        '*' | 0x80);  // white
    }

}


#ifdef CONFIG_MENU_PAGE
//===========================================================================
// Edit configfile values using menu_cfg.

static byte  temp_cvar_active = 0;
static consvar_t  temp_cvar;
static consvar_t * temp_cvar_parent;

// Handle editing cvar that are not the current cvar.
// May return ptr to a temp cvar, so must save_cv after editing.
// May return NULL, which means nothing to edit.
static
consvar_t *  config_cvar_edit_open( consvar_t * cv )
{
    temp_cvar_active = 0;

    if( menu_cfg && ((cv->state & CS_CONFIG) != menu_cfg) )
    {
        // Use a temp, to avoid having to modify so many command CV_Set functions.
        temp_cvar_parent = cv;
        // Make sure the temp is empty.
        if( temp_cvar.string )  // it should be NULL, but may not have been saved previously.
            CV_Free_cvar_string( &temp_cvar );
        // Get the hidden cvar value.
        temp_cvar_active = CV_Get_Pushed_cvar( temp_cvar_parent, menu_cfg, /*OUT*/ & temp_cvar );
        if( temp_cvar_active == 0 )
            return NULL;  // nothing to change

        // kill any effects that the current cvar would perform.
        temp_cvar.flags &= ~( CV_CALL | CV_NETVAR | CV_SHOWMODIF | CV_SHOWMODIF_ONCE );
        return &temp_cvar;
    }
    return cv;  // normal edit of current cvar
}

// If a temp cvar was used, then it will be saved.
static
void  config_cvar_edit_save( void )
{
    if( temp_cvar_active )
    {
        // Put value back into pushed cvar.
        // The string value of temp_cvar will be stolen.  Do not need to Z_Free it.
        CV_Put_Config_cvar( temp_cvar_parent, menu_cfg, /*IN*/ & temp_cvar );
        temp_cvar_active = 0;
        menu_cfg_editing = 0;
    }
}

// Edit a configfile cvar value, using menu_cfg.
static
void  config_cvar_edit_setvalue( consvar_t * cv_parent, int value )
{
    consvar_t * cv = config_cvar_edit_open( cv_parent );  // to temp_cvar
    if( cv )
    {
        // Here, cv may be current cvar, or temp_cvar,
        // either way it is safe to call CV_Set.
        CV_SetValue( cv, value );
        config_cvar_edit_save();  // saves temp_cvar
    }
}

// Create a new configfile cvar entry, using menu_cfg.
static
void  config_cvar_edit_insert( consvar_t * cv_parent, byte copy_flag )
{
    if( ! (cv_parent->flags & CV_SAVE) )
        goto done;  // not in config file

    if( (cv_parent->state & CS_CONFIG) == menu_cfg )
        goto done;  // already exists as current cvar

    if( CV_Get_Pushed_cvar( cv_parent, menu_cfg, NULL ) )  // test of existance
        goto done;  // already exists as pushed cvar

    // Create the cvar value, even if it is pushed and not current.
    const char * newstr = ( copy_flag )?
        cv_parent->string // Copy existing value.
      : cv_parent->defaultvalue ; // Get default value.
    CV_Put_Config_string( cv_parent, menu_cfg, newstr );

done:
    return;
}

// Delete the configfile cvar entry, using menu_cfg.
static
void  config_cvar_edit_delete( consvar_t * cv_parent )
{
    // This can delete a current cvar or pushed cvar value.
    CV_Delete_Config_cvar( cv_parent, menu_cfg );
}

// Edit configfile entries for most common menus.
static
byte  config_cvar_edit_key_handler( int key )
{
    menuitem_t * mip = & currentMenu->menuitems[itemOn];
    consvar_t * cv_parent;
   
    if( (mip->status & IT_TYPE) != IT_CVAR )
        goto fail;  // not a cvar menu entry

    cv_parent = (consvar_t *)mip->itemaction;
   
    switch( key )
    {
     case KEY_INS :  // insert config
        config_cvar_edit_insert( cv_parent, 0 );  // using menu_cfg
        goto done;

     case KEY_DELETE :  // delete config
        config_cvar_edit_delete( cv_parent );  // using menu_cfg
        goto done;

     default:
        goto fail;
    }
   
fail:
    return false; // did not use the key

done:
    return true;  // used the key
}

// Enter and leave menu_cfg mode.
static
byte  config_cvar_key_handler( int key )
{
    switch( key )
    {
#if 1
      case KEY_F2:
#else
      case 'M':
      case 'm':
#endif
        if( menu_cfg == CFG_main )  // toggle
            goto turn_menu_cfg_off;
        // Only show the values from the Main config file.
        menu_cfg = CFG_main;
        
        goto done;
#if 1
      case KEY_F3:
#else
      case 'D':
      case 'd':
#endif
        if( menu_cfg == CFG_drawmode )  // toggle
            goto turn_menu_cfg_off;
        // Only show the values from the Drawmode config file.
        menu_cfg = CFG_drawmode;
        goto done;
#if 1
      case KEY_F1:
      case KEY_F4:
#else
      case 'N':
      case 'n':
#endif
      turn_menu_cfg_off:
        // Normal menu values.
        menu_cfg = CFG_none;
        menu_cfg_editing = 0;
        goto done;

     default:
        break;
    }

    if( menu_cfg )
    {
        if( config_cvar_edit_key_handler( key ) )
            goto done;
    }

    return false; // did not use the key

done:
    return true;  // used the key
}
#endif


// Create initial drawmode config file.
static
void  create_initial_drawmode_config( void )
{
    // M_Set_configfile_drawmode( ) was done at mode switch.

    if( M_Have_configfile_drawmode() )
        return;
   
    S_StartSound(menu_sfx_enter);
    menu_cfg = CFG_drawmode; // using menu_cfg
    config_cvar_edit_insert( &cv_scr_width, 1 );
    config_cvar_edit_insert( &cv_scr_height, 1 );
    config_cvar_edit_insert( &cv_scr_depth, 1 );
    config_cvar_edit_insert( &cv_fullscreen, 1 );
    config_cvar_edit_insert( &cv_vidwait, 1 );
    config_cvar_edit_insert( &cv_gammafunc, 0 );
    config_cvar_edit_insert( &cv_usegamma, 0 );
    config_cvar_edit_insert( &cv_black, 0 );
    config_cvar_edit_insert( &cv_bright, 0 );

    M_Set_configfile_drawmode_present();
    M_SaveConfig( CFG_drawmode, configfile_drawmode );
}

//===========================================================================
// All ready playing, quit current game
//===========================================================================

// [WDJ] message temp buffer (replacing 3 shorter ones)
// StartMessage copies this, and only one possible message at a time
#define      MSGTMP_LEN  255
static char  msgtmp[MSGTMP_LEN+1];

//const char *ALLREADYPLAYING="You are already playing\n\nLeave this game first\n";
const char * ALLREADYPLAYING="You are already playing.\n\nAbort this game ? Y/N\n";
const char * ABORTGAME="\nAbort this game ? Y/N\n";

const event_t  reenter_event = { ev_keydown, KEY_ENTER, 0, 1 }; // see M_Responder

void M_Choose_to_quit_Response(int ch)
{
    if (ch == 'y')
    {
#if 1
        // [WDJ] The quick way to exitgame, including netgame.
        Command_ExitGame_f();
#else
        // [WDJ] This is why it is not being done this way.
        // Why is this system being used for other menus ??
        // The hard way to exitgame
        COM_BufAddText("exitgame\n");
        // unfortunately this takes time, and a couple tics
        int i;
        for( i=100; i>0; i-- )
        {
            COM_BufExecute( CFG_none ); // subject to com_wait and other delays
            if( ! Game_Playing()  ) break;
            // It must be not-playing before re-invoking the menu.
        }
#endif       
        // YES, re-invoke the caller routine at itemOn press
        D_PostEvent(&reenter_event);  // delayed reinvoke
    }
}

// [WDJ] Ask user to quit, if they already have a game in-progess.
// Return 1 if already playing, and rejects quitting.
boolean  M_already_playing( boolean check_netgame )
{
    if( Game_Playing() )
    {
        // [Arcade] Do not ask; abandon the game in progress and continue.
        // This ends the current game and posts reenter_event, which
        // re-presses the menu item that got us here.
        M_Choose_to_quit_Response('y');
        return 1;
    }
    if (check_netgame && netgame)
    {
        // [Arcade] Cannot start a new game while in a network game;
        // leave it and continue, without asking.
        M_Choose_to_quit_Response('y');
        return 1;
    }
    return 0;
}

//===========================================================================
//MAIN MENU
//===========================================================================

static void M_Loadgame(int choice);
static void M_Savegame(int choice);
static void M_QuitDOOM(int choice);

enum
{
    // [Arcade] Single Level is inserted at index 1 and Cheats at 5, so these
    // are 6 and 7 rather than the stock 4 and 5.  Cheats sits *before* Read
    // This deliberately: the Doom 2 fixup below copies Quit over the Read
    // This slot and drops one item, which would cut off anything after it.
    MM_cheats   = 5,	// referenced
    MM_readthis = 6,	// referenced
    MM_quitdoom = 7,	// referenced
} main_e;

// Compatible with modifications to original graphics
menuitem_t MainMenu[]=
{
    {IT_SUBMENU | IT_PATCH,"M_NGAME" ,"NEW GAME" ,&SingleMultiDef,'n'},
    // [Arcade] Inserted here rather than appended, so the player sees
    // New Game / Single Level / Options / Quit.  Everything that addresses
    // this menu by index was shifted to match: MM_readthis and MM_quitdoom
    // above, and the Load/Save hiding in the lockdown below.
    {IT_SUBMENU | IT_PATCH,"M_SINLVL","SINGLE LEVEL",&SingleLevelDef,'s'},
    {IT_CALL    | IT_PATCH,"M_LOADG" ,"LOAD GAME",M_Loadgame,'l'},
    {IT_CALL    | IT_PATCH,"M_SAVEG" ,"SAVE GAME",M_Savegame,'s'},
    {IT_SUBMENU | IT_PATCH,"M_OPTION","OPTIONS"  ,&OptionsDef,'o'},
    // [Arcade] Devmode only; hidden by the lockdown for players.
    {IT_SUBMENU | IT_PATCH,"M_CHEATS","CHEATS"   ,&CheatsDef ,'c'},
    {IT_SUBMENU | IT_PATCH,"M_RDTHIS","INFO"     ,&ReadDef1  ,'r'},  // Another hickup with Special edition.
    {IT_CALL    | IT_PATCH,"M_QUITG" ,"QUIT GAME",M_QuitDOOM,'q'}
};

void HereticMainMenuDrawer(void)
{
    int frame = (I_GetTime()/3)%18;

    V_DrawScaledPatch_Num(40, 10, SkullBaseLump+(17-frame) );
    V_DrawScaledPatch_Num(232, 10, SkullBaseLump+frame );
    M_DrawGenericMenu();
}

menu_t  MainDef =
{
    "M_DOOM",
    NULL,
    MainMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MainMenu)/sizeof(menuitem_t),
    97,64,
    0
};

//===========================================================================
//SINGLE/MULTI PLAYER GAME MENU
//===========================================================================

static void M_SingleNewGame(int choice);
static void M_TwoPlayerMenu(int choice);
static void M_EndGame(int choice);

// DoomLegacy graphics from legacy.wad: M_SINGLE, M_2PLAYR, M_MULTI
menuitem_t SingleMulti_Menu[] =
{
    {IT_CALL | IT_PATCH,"M_SINGLE","SINGLE PLAYER",M_SingleNewGame ,'s'},
    // [Arcade] Local play is what a cabinet means by "multiplayer", so it
    // takes that name and the M_MULTI graphic that reads "MULTIPLAYER".
    // M_2PLAYR literally reads "TWO PLAYER GAME", which stopped being true
    // once the cabinet supported four panels.
    {IT_CALL | IT_PATCH,"M_MULTI","MULTIPLAYER",M_TwoPlayerMenu ,'n'},
    // [Arcade] The networked server menu is named for what it is and drawn as
    // plain text rather than the M_MULTI graphic, so it cannot be mistaken for
    // the line above.  Already devmode-only: the lockdown hides it.
    {IT_SUBMENU | IT_WHITESTRING, 0,"Networked Multiplayer >>",&MultiPlayerDef  ,'m'},
    {IT_CALL | IT_PATCH,"M_ENDGAM","END GAME",M_EndGame ,'e'}
};

menu_t  SingleMultiDef =
{
    "M_NGAME",
    "Single Multi New Game",
    SingleMulti_Menu,
    M_DrawGenericMenu,
    NULL,
    sizeof(SingleMulti_Menu)/sizeof(menuitem_t),
    97,64,
    0
};


//===========================================================================
// Connect Menu
//===========================================================================

CV_PossibleValue_t serversearch_cons_t[] = {{0,"Local Lan"}, {1,"Internet"}, {0,NULL}};
consvar_t cv_serversearch = {"serversearch"    ,"0",CV_HIDEN,serversearch_cons_t};

#define FIRSTSERVERLINE 3

void M_Connect( int choice )
{
    // do not call menuexitfunc 
    M_Clear_Menus(false);

    // Invoke Command_connect
    COM_BufAddText(va("connect node %d\n",
                      serverlist[choice-FIRSTSERVERLINE].server_node));
    setup_net_savegame();
}

static int localservercount;

void M_Refresh( int choice )
{
    CL_Update_ServerList( cv_serversearch.value );
}

menuitem_t  ConnectMenu[] =
{
    {IT_STRING | IT_CVAR ,0,"Search On"       ,&cv_serversearch       ,0},
    {IT_STRING | IT_CALL ,0,"Refresh"         ,M_Refresh              ,0},
    {IT_WHITESTRING | IT_SPACE,0,
                "Server Name                           ping players dm" ,0 ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
    {IT_STRING | IT_SPACE,0,""             ,M_Connect              ,0},
};

void M_DrawConnectMenu( void )
{
    int i, sly;
    char *p;

    for( i=FIRSTSERVERLINE; i<localservercount+FIRSTSERVERLINE; i++ )
        ConnectMenu[i].status = IT_STRING | IT_SPACE;

    sly = currentMenu->y + (FIRSTSERVERLINE * STRINGHEIGHT);
    if( serverlistcount <= 0 )
        V_DrawString (currentMenu->x, sly, 0, "No server found");
    else
    for( i=0; i<serverlistcount; i++ )
    {
        if( i >= ((sizeof(ConnectMenu)/sizeof(menuitem_t)) - FIRSTSERVERLINE) )
            break; // too many to draw all
        V_DrawString (currentMenu->x, sly, 0, serverlist[i].info.servername);
        p = va("%d", serverlist[i].info.trip_time);  // ping time
        V_DrawString (currentMenu->x + 200 - V_StringWidth(p), sly, 0, p);
        p = va("%d/%d  %d", serverlist[i].info.num_active_players,
                            serverlist[i].info.maxplayer,
                            serverlist[i].info.deathmatch);
        V_DrawString (currentMenu->x + 250 - V_StringWidth(p), sly, 0, p);
        sly += STRINGHEIGHT;

        ConnectMenu[i+FIRSTSERVERLINE].status = IT_STRING | IT_CALL;
    }
    localservercount = serverlistcount;

    M_DrawGenericMenu();
}

boolean M_CancelConnect(void)
{
    D_CloseConnection();
    return true;
}

menu_t  Connectdef =
{
    "M_CONNEC", // in legacy.wad
    "Connect Server",
    ConnectMenu,
    M_DrawConnectMenu,
    M_CancelConnect,
    sizeof(ConnectMenu)/sizeof(menuitem_t),
    27,40,
    0
};

// Select Connect Menu
void M_ConnectMenu(int choice)
{
    if( M_already_playing(0) )  return;

    // Restore user settings
    D_End_commandline();
   
    Push_Setup_Menu(&Connectdef);
    M_Refresh(0);
}

//===========================================================================
// Start Server Menu
//===========================================================================

CV_PossibleValue_t skill_cons_t[] = {{1,"I'm too young to die"}
                                    ,{2,"Hey, not too rough"}
                                    ,{3,"Hurt me plenty"}
                                    ,{4,"Ultra violence"}
                                    ,{5,"Nightmare!" }
                                    ,{0,NULL}};

CV_PossibleValue_t map_cons_t[] = {{ 1,"map01"} ,{ 2,"map02"} ,{ 3,"map03"}
                                  ,{ 4,"map04"} ,{ 5,"map05"} ,{ 6,"map06"}
                                  ,{ 7,"map07"} ,{ 8,"map08"} ,{ 9,"map09"}
                                  ,{10,"map10"} ,{11,"map11"} ,{12,"map12"}
                                  ,{13,"map13"} ,{14,"map14"} ,{15,"map15"}
                                  ,{16,"map16"} ,{17,"map17"} ,{18,"map18"}
                                  ,{19,"map19"} ,{20,"map20"} ,{21,"map21"}
                                  ,{22,"map22"} ,{23,"map23"} ,{24,"map24"}
                                  ,{25,"map25"} ,{26,"map26"} ,{27,"map27"}
                                  ,{28,"map28"} ,{29,"map29"} ,{30,"map30"}
                                  ,{31,"map31"} ,{32,"map32"} ,{0,NULL}};

CV_PossibleValue_t exmy_cons_t[] ={{11,"e1m1"} ,{12,"e1m2"} ,{13,"e1m3"}
                                  ,{14,"e1m4"} ,{15,"e1m5"} ,{16,"e1m6"}
                                  ,{17,"e1m7"} ,{18,"e1m8"} ,{19,"e1m9"}
                                  ,{21,"e2m1"} ,{22,"e2m2"} ,{23,"e2m3"}
                                  ,{24,"e2m4"} ,{25,"e2m5"} ,{26,"e2m6"}
                                  ,{27,"e2m7"} ,{28,"e2m8"} ,{29,"e2m9"}
                                  ,{31,"e3m1"} ,{32,"e3m2"} ,{33,"e3m3"}
                                  ,{34,"e3m4"} ,{35,"e3m5"} ,{36,"e3m6"}
                                  ,{37,"e3m7"} ,{38,"e3m8"} ,{39,"e3m9"}
                                  ,{41,"e4m1"} ,{42,"e4m2"} ,{43,"e4m3"}
                                  ,{44,"e4m4"} ,{45,"e4m5"} ,{46,"e4m6"}
                                  ,{47,"e4m7"} ,{48,"e4m8"} ,{49,"e4m9"}
                                  ,{41,"e5m1"} ,{42,"e5m2"} ,{43,"e5m3"}
                                  ,{44,"e5m4"} ,{45,"e5m5"} ,{46,"e5m6"}
                                  ,{47,"e5m7"} ,{48,"e5m8"} ,{49,"e5m9"}
                                  ,{0,NULL}};

consvar_t cv_skill    = {"skill"    ,"4",CV_HIDEN,skill_cons_t};
consvar_t cv_monsters = {"monsters" ,"0",CV_HIDEN,CV_YesNo};
// The bots use player slots, so number of bots is limited to MAXPLAYERS.
CV_PossibleValue_t bots_cons_t[] = {{0,"MIN"}, {MAXPLAYERS,"MAX"}, {0,NULL}};
consvar_t cv_bots = {"bots", "0", CV_HIDEN, bots_cons_t};
consvar_t cv_nextmap  = {"nextmap"  ,"1",CV_HIDEN,map_cons_t};
consvar_t cv_nextepmap  = {"nextepmap"  ,"11",CV_HIDEN,exmy_cons_t};

// To prevent changing game settings while changing between the possible settings.
extern CV_PossibleValue_t deathmatch_cons_t[];
void deathmatch_menu_OnChange( void );
consvar_t cv_deathmatch_menu  = {"dmm"  , "3", CV_HIDEN | CV_CALL, deathmatch_cons_t, deathmatch_menu_OnChange };

// [Arcade] The deathmatch round length, in minutes, as the menu shows it.
// A separate cvar from cv_timelimit for the same reason cv_deathmatch_menu is
// separate from cv_deathmatch: cv_timelimit is the *engine's* limit and is
// rewritten at every game start -- set to 0 for coop and single player, which
// must not be cut short -- so a value typed into the menu was overwritten
// before it could ever be used, and the menu then read back 0.  This one is
// only read, never written, so it keeps what the operator set.
CV_PossibleValue_t dmtimelimit_cons_t[] = {{0,"MIN"},{60,"MAX"},{0,NULL}};
consvar_t cv_dm_timelimit = {"dmtimelimit", "5", CV_SAVE, dmtimelimit_cons_t };

void deathmatch_menu_OnChange( void )
{
    // Default monsters, because it often gets forgotten, and it is not saved.
    CV_Set_by_OnChange( &cv_monsters, (cv_deathmatch_menu.EV > 4) );
}

CV_PossibleValue_t wait_players_cons_t[]=   {{0,"MIN"}, {32,"MAX"}, {0,NULL}};
consvar_t cv_wait_players = {"wait_players" ,"2",CV_HIDEN,wait_players_cons_t};
CV_PossibleValue_t wait_timeout_cons_t[]=   {{0,"MIN"}, {5,"INC"}, {244,"MAX"}, {0,NULL}};
consvar_t cv_wait_timeout = {"wait_timeout" ,"0",CV_HIDEN,wait_timeout_cons_t};

static boolean StartSplitScreenGame = false;

// [Arcade] Minutes on the clock for a deathmatch round.
// [Arcade] Superseded by cv_dm_timelimit, which is the same 5 minutes by
// default but can be changed from the menu.  Kept only as the documented
// origin of that default.

// Called from ServerMenu
// [Arcade] Join screen, defined further down.  True when the page was opened
// and the caller should return; the callback runs the real start.
boolean  M_Join_Open( void (*startfunc)(void), boolean first_press_starts );

static void  M_StartServer_Go( void );
static int   startserver_choice;

// [Arcade] Two Player Game -> Start Game reaches the game through here rather
// than through M_ChooseSkill, so it needs its own join-screen hook or that
// whole route would skip the page.
void M_StartServer( int choice )
{
    startserver_choice = choice;

    if( M_Join_Open( M_StartServer_Go, false ) )   // wait for everyone
        return;

    D_Clear_Join_Count();   // no join screen: every panel plays
    D_Reset_View_Cells();
    M_StartServer_Go();
}

static void  M_StartServer_Go( void )
{
    int choice = startserver_choice;

    M_Clear_Menus(true);

    single_level_mode = 0;   // [Arcade] campaign game, see M_ChooseSkill

    // [WDJ] May have been client.
    server = true;
    netgame = true;
    multiplayer = true;

    if( choice == 10 )  // menu
    {
        // Dedicated server menu choice.
        dedicated = true;
        nodrawers = true;
        vid.draw_ready = 0;        
        I_ShutdownGraphics();
    }

    // Need to set server before this func.
    D_WaitPlayer_Setup();

    // Before game start setup.
    // [Arcade] Deathmatch rounds get a time limit, coop clears it.  An
    // unattended cabinet has no other way out of a stalemate -- players
    // cannot reach End Game, and the idle timeout only fires when nobody is
    // touching the controls.  Applied per game start rather than as the
    // cv_timelimit default, which would also cut single player levels short.
    // In deathmatch_cons_t the DM modes are values 1..4; every coop variant
    // is 0 or >= 0x10.
    int  dmm = cv_deathmatch_menu.value;
    int  tl  = (dmm >= 1 && dmm <= 4) ? cv_dm_timelimit.value : 0;
    COM_BufAddText(va("stopdemo;splitscreen %d;deathmatch %d;timelimit %d\n",
                      StartSplitScreenGame, dmm, tl ) );

    // skin change
    if (StartSplitScreenGame
        && ! ( displayplayer2_ptr
             && displayplayer2_ptr->skin
             && (strcasecmp(cv_skin[1].string, skins[displayplayer2_ptr->skin]->name) == 0 )
             ) )
    {
        COM_BufAddText ( va("%s \"%s\"\n", cv_skin[1].name, cv_skin[1].string));
    }

    COM_BufAddText(va("map \"%s\" -skill %d -monsters %d\n", 
                      (gamemode==doom2_commercial)? cv_nextmap.string : cv_nextepmap.string,
                      cv_skill.value, cv_monsters.value));
   
#if 0
// Needs to be done after players have grabbed the player slots.
    // Add bots
    if( cv_bots.EV > 0 )
    {
        unsigned int cnt = cv_bots.EV;
        while( cnt-- )
        {
#if 1	   
            COM_BufAddText( "addbot;" );
#else
            // Delay to allow splitscreen to grab player2 first.
            COM_BufAddText( "wait 35; addbot;" );
#endif
        }
    }
#endif   
}

menuitem_t  ServerMenu[] =
{
    {IT_STRING | IT_CVAR,0,"Map"             ,&cv_nextmap          ,0},
    {IT_STRING | IT_CVAR,0,"Skill"           ,&cv_skill            ,0},
    {IT_STRING | IT_CVAR,0,"Coop/Deathmatch" ,&cv_deathmatch_menu  ,0},
    {IT_STRING | IT_CVAR,0,"Monsters"        ,&cv_monsters         ,0},
    {IT_STRING | IT_CVAR,0,"Bots"            ,&cv_bots             ,0},
    {IT_STRING | IT_CVAR,0,"Wait Players"    ,&cv_wait_players     ,0},
    {IT_STRING | IT_CVAR,0,"Wait Timeout"    ,&cv_wait_timeout     ,0},
    {IT_STRING | IT_CVAR,0,"Internet Server" ,&cv_internetserver   ,0},
    {IT_STRING | IT_CVAR
     | IT_CV_STRING     ,0,"Server Name"     ,&cv_servername       ,0},
    {IT_WHITESTRING | IT_CALL | IT_YOFFSET,
                         0,"Start"           ,M_StartServer        ,110}, // 9
    {IT_WHITESTRING | IT_CALL | IT_YOFFSET,
                         0,"Dedicated"       ,M_StartServer        ,120}  // 10
};

menuitem_t  ServerMenu_Map =
    {IT_STRING | IT_CVAR,0,"Map"             ,&cv_nextmap          ,0};
menuitem_t  ServerMenu_EpisodeMap =
    {IT_STRING | IT_CVAR,0,"Episode Map"     ,&cv_nextepmap        ,0};

//===========================================================================
//                          SINGLE LEVEL  [Arcade]
//===========================================================================
// Play one chosen map and come back here, with its own high score table.
// Deliberately reuses the Start Game screen's cvars -- cv_nextmap /
// cv_nextepmap already hold the per-gamemode map lists, and M_Configure
// already trims exmy_cons_t down to the episodes actually present.

static void M_SingleLevel_Start(int choice);
static void M_SingleLevel_WatchSpeed(int choice);
static void M_SingleLevel_WatchMax(int choice);
static void M_Draw_SingleLevel(void);

// Item indices, used by the enable/disable pass below.
enum { SL_map = 0, SL_skill, SL_start, SL_speeddemo, SL_maxdemo, SL_numitems };

menuitem_t  SingleLevelMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Map"             ,&cv_nextmap    ,0},
    {IT_STRING | IT_CVAR,0,"Skill"           ,&cv_skill      ,0},
    {IT_WHITESTRING | IT_CALL | IT_YOFFSET,
                         0,"Start"           ,M_SingleLevel_Start      ,50},
    {IT_WHITESTRING | IT_CALL,
                         0,"Watch speed run" ,M_SingleLevel_WatchSpeed ,0},
    {IT_WHITESTRING | IT_CALL,
                         0,"Watch max run"   ,M_SingleLevel_WatchMax   ,0},
};

// Swapped in by M_Configure, exactly as ServerMenu does: Doom 2 has a flat
// MAPxx list, the Doom 1 games are episode+map.
menuitem_t  SingleLevelMenu_Map =
    {IT_STRING | IT_CVAR,0,"Map"             ,&cv_nextmap    ,0};
menuitem_t  SingleLevelMenu_EpisodeMap =
    {IT_STRING | IT_CVAR,0,"Episode Map"     ,&cv_nextepmap  ,0};

menu_t  SingleLevelDef =
{
    "M_SINLVL",  // in legacy.wad
    "Single Level",
    SingleLevelMenu,
    M_Draw_SingleLevel,
    NULL,
    sizeof(SingleLevelMenu)/sizeof(menuitem_t),
    60,40,
    0
};


// The map name the menu is currently pointing at, in the engine's own form.
// cv_nextmap is a flat 1..32 for Doom 2; cv_nextepmap encodes episode*10+map.
static const char *  M_SingleLevel_MapName( void )
{
    if( gamemode == doom2_commercial )
        return G_BuildMapName( 1, cv_nextmap.value );

    return G_BuildMapName( cv_nextepmap.value / 10, cv_nextepmap.value % 10 );
}


// cv_skill is the *menu* skill cvar and skill_cons_t numbers it 1..5, which
// is what the "map ... -skill %d" command wants (Command_Map_f does atoi()-1)
// and what M_StartServer passes straight through.  G_DeferedInitNew and
// HS_* take a 0-based skill_e instead -- M_ChooseSkill hands it the New Game
// menu's item index.  Passing cv_skill.value to those raw launched and scored
// one skill too hard: picking Ultra violence ran sk_nightmare, with fast
// monsters and respawning, which is how this was noticed.
static skill_e  M_SingleLevel_Skill( void )
{
    return (skill_e)(cv_skill.value - 1);
}


// Grey out the two replay items when no demo has been recorded for the
// selection.  IT_DISABLED rather than IT_HIDDEN so the page does not change
// height as the player scrolls through maps, which looks broken.
static void  M_SingleLevel_Update_Items( void )
{
    const char * mn = M_SingleLevel_MapName();
    skill_e sk = M_SingleLevel_Skill();

    SingleLevelMenu[SL_speeddemo].status =
        HS_Demo_Path_For( mn, sk, 0, true, NULL )
        ? (IT_WHITESTRING | IT_CALL) : (IT_WHITESTRING | IT_DISABLED);

    SingleLevelMenu[SL_maxdemo].status =
        HS_Demo_Path_For( mn, sk, 1, true, NULL )
        ? (IT_WHITESTRING | IT_CALL) : (IT_WHITESTRING | IT_DISABLED);
}


static void  M_Draw_SingleLevel( void )
{
    char  buf[64], tbuf[16];
    const char * mn;
    tic_t t;
    int   y;

    M_SingleLevel_Update_Items();
    M_DrawGenericMenu();

    mn = M_SingleLevel_MapName();

    // Best times for the current selection, between the cvar rows and Start.
    // The generic drawer lays items out from currentMenu->y at itemheight
    // steps; this sits in the gap the Start item's IT_YOFFSET opens up.
    y = SingleLevelDef.y + 30;

    snprintf( buf, sizeof(buf), "BEST FOR %s", mn );
    V_DrawString( SingleLevelDef.x, y, V_WHITEMAP, buf );

    if( HS_Best_For( mn, M_SingleLevel_Skill(), 0, true, &t ) )
        HS_Format_Time_Str( t, tbuf, sizeof(tbuf) );
    else
        dl_strncpy( tbuf, "--:--", sizeof(tbuf) );
    snprintf( buf, sizeof(buf), "SPEED %s", tbuf );
    V_DrawString( SingleLevelDef.x, y+10, 0, buf );

    if( HS_Best_For( mn, M_SingleLevel_Skill(), 1, true, &t ) )
        HS_Format_Time_Str( t, tbuf, sizeof(tbuf) );
    else
        dl_strncpy( tbuf, "--:--", sizeof(tbuf) );
    snprintf( buf, sizeof(buf), "MAX   %s", tbuf );
    V_DrawString( SingleLevelDef.x + 110, y+10, 0, buf );
}


static void  M_SingleLevel_Start( int choice )
{
    if( M_already_playing(0) )  return;

    single_level_mode = 1;

    // [Arcade] Single Level is a scored single player mode: no join screen,
    // and it must not inherit a join count from an earlier multiplayer game.
    D_Set_Join_Count( 1 );
    D_Reset_View_Cells();

    // Same ordering rule as M_ChooseSkill: HS_NewGame must precede
    // G_DeferedInitNew so the player-create and map netxcmds land in the
    // demo stream.  It reads HS_GameId(), which now follows single_level_mode
    // -- hence setting that first.
    HS_NewGame();
    // false = not a splitscreen game.  That third argument is
    // StartSplitScreenGame, *not* resetplayer -- it feeds straight into the
    // "splitscreen %d" command G_DeferedInitNew issues.  Single Level is
    // always one player; passing true launched it in two player splitscreen.
    G_DeferedInitNew( M_SingleLevel_Skill(), M_SingleLevel_MapName(), false );
    M_Clear_Menus( true );
}


static void  M_SingleLevel_PlayDemo( int cat )
{
    char path[MAX_WADPATH];

    if( ! HS_Demo_Path_For( M_SingleLevel_MapName(), M_SingleLevel_Skill(),
                            cat, true, path ) )
        return;   // item should have been disabled

    // NOT singledemo: that makes G_CheckDemoStatus call I_Quit() when the
    // demo ends, which quit DoomLegacy the moment the exit switch was hit.
    // single_level_mode instead routes the end of the demo back to this menu,
    // the same way finishing a played level does.
    single_level_mode = 1;
    M_Clear_Menus( true );
    G_DeferedPlayDemo( path );
}

static void  M_SingleLevel_WatchSpeed( int choice )  { M_SingleLevel_PlayDemo(0); }
static void  M_SingleLevel_WatchMax  ( int choice )  { M_SingleLevel_PlayDemo(1); }

// Called from G_DoWorldDone when the chosen level is finished: drop back to
// this menu with the times refreshed, so a player grinding one map can go
// straight round again.  The idle timeout still rescues an abandoned cabinet.
void  M_SingleLevel_Finished( void )
{
    // Command_ExitGame_f clears single_level_mode, and it is left cleared.
    // The flag means "a single level run is in progress", not "the player is
    // looking at the Single Level menu" -- M_SingleLevel_Start and
    // M_SingleLevel_PlayDemo each set it again for the run they begin, and
    // this page's own display passes the mode to HS_Best_For /
    // HS_Demo_Path_For explicitly rather than reading it.
    //
    // It used to be re-set here, which left it stuck on for everything that
    // followed: a New Game started afterwards inherited it, so the campaign
    // ended after one map and a death dropped the player onto this page.
    Command_ExitGame_f();       // tears the game down and starts the title
    M_StartControlPanel();
    Push_Setup_Menu( &SingleLevelDef );
}


menu_t  ServerDef =
{
    "M_STSERV", // in legacy.wad
    "Start Game",
    ServerMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(ServerMenu)/sizeof(menuitem_t),
    27,40,
    0,
};

void M_StartServerMenu(int choice)
{
    if( M_already_playing(0) )  return;

    // Restore user settings
    D_End_commandline();
   
    ServerMenu[0] = (gamemode==doom2_commercial)?
         ServerMenu_Map  // Doom2
       : ServerMenu_EpisodeMap;  // Ult doom, Heretic

    // StartSplitScreenGame already set by TwoPlayer menu
    Push_Setup_Menu(&ServerDef);
    setup_net_savegame();
}

//===========================================================================
//                            MULTI PLAYER MENU
//===========================================================================
static void M_SetupMultiPlayer1 (int choice);
static void M_SetupMultiPlayer2 (int choice);
static void M_TwoPlayerMenu(int choice);

// index for MultiPlayerMenu
enum {
    MPM_player1 = 1, // referenced in M_Player2_MenuEnable
    MPM_player2 = 2, // referenced in M_Player2_MenuEnable
} multiplayer_e;

// DoomLegacy graphics from legacy.wad: M_STSERV, M_CONNEC, M_2PLAYR, M_SETUPA, M_SETUPB
menuitem_t MultiPlayerMenu[] =
{
    // BIG font menu. BIG font does not work, lump is missing.
    // Cannot put all three options here.
    {IT_CALL | IT_PATCH,"M_MULTI","MULTIPLAYER",M_TwoPlayerMenu ,'n'},
    {IT_CALL | IT_PATCH,"M_SETUPA","SETUP PLAYER 1" ,M_SetupMultiPlayer1 ,'s'},
    {IT_CALL | IT_PATCH,"M_SETUPB","SETUP PLAYER 2" ,M_SetupMultiPlayer2 ,'t'},
    {IT_SUBMENU | IT_PATCH,"M_OPTION","OPTIONS"     ,&MPOptionDef ,'o'},
    {IT_CALL | IT_PATCH,"M_CONNEC","CONNECT SERVER" ,M_ConnectMenu ,'c'},
    {IT_CALL | IT_PATCH,"M_STSERV","CREATE SERVER"  ,M_StartServerMenu ,'a'},
    {IT_CALL | IT_PATCH,"M_ENDGAM","END GAME"       ,M_EndGame ,'e'}
};

menu_t  MultiPlayerDef =
{
    "M_MULTI", // in legacy.wad
    "Multiplayer",
    MultiPlayerMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MultiPlayerMenu)/sizeof(menuitem_t),
    85,40,
    0
};


//===========================================================================
// Two Player menu
//===========================================================================

static void M_TwoPlayer_PlayerConfig(int choice);
static void M_SetupMultiPlayer_pind( byte pind );   // defined below

// DoomLegacy graphics from legacy.wad: M_STSERV, M_MULTI
menuitem_t TwoPlayerMenu[] =
{
    // [Arcade] One config entry per panel, replacing the two "SETUP PLAYER"
    // graphics (M_SETUPA/M_SETUPB), which only ever covered two players and
    // named a different page than the one they open.  Plain text, matching
    // Options -> Player, so panels 3 and 4 need no new artwork.
    {IT_CALL | IT_WHITESTRING, 0,"Player1 config >>", M_TwoPlayer_PlayerConfig, '1'},
    {IT_CALL | IT_WHITESTRING, 0,"Player2 config >>", M_TwoPlayer_PlayerConfig, '2'},
    {IT_CALL | IT_WHITESTRING, 0,"Player3 config >>", M_TwoPlayer_PlayerConfig, '3'},
    {IT_CALL | IT_WHITESTRING, 0,"Player4 config >>", M_TwoPlayer_PlayerConfig, '4'},
    {IT_CALL | IT_PATCH,"M_OPTION","OPTIONS"       ,M_NetOption ,'o'},
    {IT_CALL | IT_PATCH,"M_STSERV","START GAME"    ,M_StartServerMenu , 0},
    {IT_SUBMENU | IT_WHITESTRING, 0,"Networked Multiplayer >>",&MultiPlayerDef  ,'m'},
};

// [Arcade] This menu is addressed by position (the lockdown hides rows), so
// the indices are named.  Keep in step with TwoPlayerMenu above.
enum {
    twoplayer_p1_config = 0,
    twoplayer_p2_config,
    twoplayer_p3_config,
    twoplayer_p4_config,
    twoplayer_options,
    twoplayer_startgame,
    twoplayer_networked,
};


// Open a panel's config page.  Unlike M_PlayerDirectorChoice this does not
// Pop_Menu first, so backing out of the config returns to this page rather
// than skipping past it.
static void M_TwoPlayer_PlayerConfig(int choice)
{
    M_SetupMultiPlayer_pind( choice );   // pind = 0..3
    Push_Setup_Menu( &PlayerOptionsDef );
}

menu_t  TwoPlayerDef =
{
    "M_MULTI",  // [Arcade] from legacy.wad; reads "MULTIPLAYER"
    "Multiplayer",   // up to four local players now
    TwoPlayerMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(TwoPlayerMenu)/sizeof(menuitem_t),
    85,40,
    0
};


void M_TwoPlayerMenu(int choice)
{
    StartSplitScreenGame = true;
    M_Player2_MenuEnable( 1 );
    Push_Setup_Menu(&TwoPlayerDef);
}


//===========================================================================
// Second mouse config for the splitscreen player
//===========================================================================

menuitem_t  SecondMouseCfgMenu[] =
{
    {IT_STRING | IT_CVAR,0,"P2 Use Mouse2",     &cv_usemouse[1]      ,0},
    {IT_STRING | IT_CVAR,0,"P2 Always MouseLook", &cv_alwaysfreelook[1],0},
    {IT_STRING | IT_CVAR,0,"P2 Mouse Move",     &cv_mouse_move[1]    ,0},
#ifdef MOUSE2
    {IT_STRING | IT_CVAR,0,"Mouse2 Serial Port", &cv_mouse2port      ,0},
#if defined( SMIF_SDL ) || defined( SMIF_WIN32 )
    {IT_STRING | IT_CVAR,0,"Mouse2 type",       &cv_mouse2type       ,0},
#endif
    {IT_STRING | IT_CVAR,0,"Mouse2 Invert",     &cv_mouse2_invert    ,0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,"Mouse2 x Speed",    &cv_mouse2_sens_x    ,0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,"Mouse2 y Speed",    &cv_mouse2_sens_y    ,0},
#else
    {IT_STRING|IT_WHITESTRING|IT_NOTHING, 0, "NO MOUSE2",   0, 0},
#endif
};

menu_t  SecondMouseCfgdef =
{
    "M_OPTTTL",
    "Options",
    SecondMouseCfgMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(SecondMouseCfgMenu)/sizeof(menuitem_t),
    27,40,
    0
};

//===========================================================================
// Options for the main player and the splitscreen player
//===========================================================================

// [0]=main player, [1]=splitscreen player
static byte menu_pind = 0;
static byte menu_multiplayer = 0;

static void M_SetupMultiPlayer_pind( byte pind );
static void M_SetupMultiPlayer1(int choice);
static void M_SetupMultiPlayer2(int choice);
static void M_SetupMultiPlayer3(int choice);   // [Arcade] third panel
static void M_SetupMultiPlayer4(int choice);   // [Arcade] fourth panel
static void M_Setup_P1_Controls(int choice);
static void M_Setup_P2_Controls(int choice);
static void M_Setup_P3_Controls(int choice);   // [Arcade]
static void M_Setup_P4_Controls(int choice);   // [Arcade]
// [Arcade] guided panel setup, defined next to M_ChangeControl
static void M_Guided_Controls_P1(int choice);
static void M_Guided_Controls_P2(int choice);
static void M_Guided_Controls_P3(int choice);
static void M_Guided_Controls_P4(int choice);

// [Arcade] Widened to MAXSPLITSCREENPLAYERS.  The menu itself was already
// written against a pind (M_SetupMultiPlayer_pind repoints every cvar from
// the per-player arrays), so panels 3 and 4 needed only their own entries
// here and their own thin entry points.
static menufunc_t M_SetupMultiPlayer[MAXSPLITSCREENPLAYERS] =
  { M_SetupMultiPlayer1, M_SetupMultiPlayer2, M_SetupMultiPlayer3, M_SetupMultiPlayer4 };
static menufunc_t M_Setup_P_Controls[MAXSPLITSCREENPLAYERS] =
  { M_Setup_P1_Controls, M_Setup_P2_Controls, M_Setup_P3_Controls, M_Setup_P4_Controls };
static const char *  player_pind_str[MAXSPLITSCREENPLAYERS] =
  { "Player1", "Player2", "Player3", "Player4" };
static const char *  player_setup_str[MAXSPLITSCREENPLAYERS] =
  { "Player1 setup >>", "Player2 setup >>", "Player3 setup >>", "Player4 setup >>" };
static const char *  player_controls_str[MAXSPLITSCREENPLAYERS] =
  { "Player1 controls >>", "Player2 controls >>",
    "Player3 controls >>", "Player4 controls >>" };
static const char *  player_config_str[MAXSPLITSCREENPLAYERS] =
  { "Player1 config >>", "Player2 config >>",
    "Player3 config >>", "Player4 config >>" };

// Customized by M_SetupMultiPlayer1 and M_SetupMultiPlayer2
menuitem_t  PlayerOptionsMenu[] =
{
//    {IT_STRING | IT_CVAR,"Messages:"       ,&cv_showmessages2    ,0},
    {IT_STRING | IT_CVAR,0, "Always Run",  &cv_autorun[0]        ,0},
    {IT_STRING | IT_CVAR,0, "Crosshair",   &cv_crosshair[0]      ,0},
    {IT_STRING | IT_CVAR,0, "Autoaim",     &cv_autoaim[0]        ,0},
    {IT_STRING | IT_CVAR,0, "Use Mouse" ,  &cv_usemouse[0]       ,0},
    {IT_STRING | IT_CVAR,0, "Mouse Move",  &cv_mouse_move[0]     ,0},
    {IT_STRING | IT_CVAR,0, "Always MouseLook", &cv_alwaysfreelook[0], 0},
    {IT_STRING | IT_CVAR | IT_CV_STRING,0, "WeaponPref", &cv_weaponpref[1] ,0},
//    {IT_STRING | IT_CVAR,"Control per key" ,&cv_controlperkey2   ,0},
    {IT_CALL | IT_WHITESTRING, 0,"Player1 setup >>", M_SetupMultiPlayer1, 's'},
    {IT_CALL | IT_WHITESTRING, 0,"Player1 controls >>", M_Setup_P1_Controls, 'c'},
};

// index by above menu lines
enum {
    playeroption_alwaysrun,
    playeroption_crosshair,
    playeroption_autoaim,
    playeroption_usemouse,
    playeroption_mousemove,
    playeroption_mouselook,
    playeroption_weaponpref,
    playeroption_setupplayer,
    playeroption_setupcontrol,
    playeroption_end
};

menu_t  PlayerOptionsDef =
{
    NULL,
    "Player1",
    PlayerOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(PlayerOptionsMenu)/sizeof(menuitem_t),
    27,40,
    0,
};


static void M_PlayerDirectorChoice(int choice)
{
    // pind = choice
    M_SetupMultiPlayer_pind( choice );  // pind = 0,1
    Pop_Menu();
    Push_Setup_Menu( &PlayerOptionsDef );
}

menuitem_t  PlayerDirectorMenu[] =
{
    {IT_CALL | IT_WHITESTRING, 0,"Player1 config >>", M_PlayerDirectorChoice, '1'},
    {IT_CALL | IT_WHITESTRING, 0,"Player2 config >>", M_PlayerDirectorChoice, '2'},
    // [Arcade] Plain text, like the two above -- no menu graphic needed.  The
    // M_SETUPA/M_SETUPB patches live on the upstream Two Player and
    // Multiplayer screens, which the lockdown hides from players anyway.
    // Shown only when the cabinet has that many panels; see M_Configure.
    {IT_CALL | IT_WHITESTRING, 0,"Player3 config >>", M_PlayerDirectorChoice, '3'},
    {IT_CALL | IT_WHITESTRING, 0,"Player4 config >>", M_PlayerDirectorChoice, '4'}
};

menu_t  PlayerDirectorDef =
{
    "M_OPTTTL",
    "Player",
    PlayerDirectorMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(PlayerDirectorMenu)/sizeof(menuitem_t),
    27,40,
    0,
};

static void M_PlayerDirector(int choice)
{
    // Select the menu
    if( ! menu_multiplayer )
    {
        M_SetupMultiPlayer_pind( 0 );
        Push_Setup_Menu( &PlayerOptionsDef );
        return;        
    }
    Push_Setup_Menu( &PlayerDirectorDef );
}


//===========================================================================
//MULTI PLAYER SETUP MENU
//===========================================================================
static void M_DrawSetupMultiPlayerMenu(void);
static void M_MultiPlayer_Responder(int choice);
static boolean M_QuitMultiPlayerMenu(void);

#define PLBOXW    8
#define PLBOXH    9
#define PLBOXX    90
#define PLBOXY    8
#define PLSKINNAMEY 96

// Customized by M_SetupMultiPlayer1 and M_SetupMultiPlayer2
menuitem_t SetupMultiPlayerMenu[] =
{
    {IT_KEYHANDLER | IT_STRING          ,0,"Your name" ,M_MultiPlayer_Responder,0},
    {IT_CVAR | IT_STRING | IT_CV_NOPRINT | IT_YOFFSET, 0,"Your color",&cv_playercolor[0], 16},
    {IT_KEYHANDLER | IT_STRING | IT_YOFFSET, 0,"Your skin" ,M_MultiPlayer_Responder, PLSKINNAMEY},
    // [Arcade] Per-player control scheme, cvar repointed by M_SetupMultiPlayer_pind.
    {IT_CVAR | IT_STRING | IT_YOFFSET, 0,"Control scheme", &cv_controlscheme[0], PLSKINNAMEY+14},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"Player config >>", &PlayerOptionsDef, PLSKINNAMEY+24},
 // Player2 only
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Player2 Controls >>", M_Setup_P2_Controls, PLSKINNAMEY+34},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"Second Mouse config >>", &SecondMouseCfgdef, PLSKINNAMEY+44}
};

// index to above menu lines
enum {
    setupmultiplayer_name = 0,
    setupmultiplayer_color,
    setupmultiplayer_skin,
    setupmultiplayer_scheme,
    setupmultiplayer_options,
    setupmultiplayer_controls,
    setupmultiplayer_mouse2,
    setupmulti_end
};

menu_t  SetupMultiPlayerDef =
{
    "M_MULTI", // in legacy.wad
    "Multiplayer",
    SetupMultiPlayerMenu,
    M_DrawSetupMultiPlayerMenu,
    M_QuitMultiPlayerMenu,
    sizeof(SetupMultiPlayerMenu)/sizeof(menuitem_t),
    27,40,
    0,
};

static  int       multi_tics;
static  state_t*  multi_state;

// this is set before entering the MultiPlayer setup menu,
// for either player 1 or 2
static  char       setupm_name[MAXPLAYERNAME+1];
static  player_t*  setupm_player;
static  consvar_t* setupm_cvskin;
static  consvar_t* setupm_cvcolor;
static  consvar_t* setupm_cvname;
static  byte       setupm_skinindex;

static
void M_SetupMultiPlayer_pind( byte pind )
{
    menu_pind = pind;

    // SetupMultiPlayerMenu
    setupm_cvname = &cv_playername[pind];
    strcpy (setupm_name, cv_playername[pind].string);
    setupm_cvskin = &cv_skin[pind];
    setupm_skinindex = R_SkinAvailable( cv_skin[pind].string );
    setupm_cvcolor = &cv_playercolor[pind];

    SetupMultiPlayerMenu[setupmultiplayer_color].itemaction = setupm_cvcolor;
    SetupMultiPlayerMenu[setupmultiplayer_scheme].itemaction = &cv_controlscheme[pind];  // [Arcade]

    // PlayerOptionsMenu
    PlayerOptionsDef.menutitle = player_pind_str[pind];
    // [Arcade] cv_usemouse is genuinely [2] -- there are two mouse devices,
    // not four -- and an arcade panel has no mouse at all, so the three mouse
    // rows are hidden for panels 3 and 4 rather than inventing a use_mouse3.
    {
        // [Arcade] Also devmode-only: a player has no use for mouse settings
        // on a cabinet, and they are three rows of clutter on the page they
        // reach most often.
        boolean has_mouse = (pind < 2) && devmode;
        uint16_t mouse_status = has_mouse ? (IT_STRING | IT_CVAR) : IT_HIDDEN;

        PlayerOptionsMenu[playeroption_usemouse].itemaction =
            &cv_usemouse[ has_mouse ? pind : 0 ];
        PlayerOptionsMenu[playeroption_usemouse].status  = mouse_status;
        PlayerOptionsMenu[playeroption_mousemove].status = mouse_status;
        PlayerOptionsMenu[playeroption_mouselook].status = mouse_status;
    }
    PlayerOptionsMenu[playeroption_mouselook].itemaction = &cv_alwaysfreelook[pind];
    PlayerOptionsMenu[playeroption_mousemove].itemaction = &cv_mouse_move[pind];
    PlayerOptionsMenu[playeroption_alwaysrun].itemaction = &cv_autorun[pind];
    PlayerOptionsMenu[playeroption_crosshair].itemaction = &cv_crosshair[pind];
    PlayerOptionsMenu[playeroption_autoaim].itemaction = &cv_autoaim[pind];
    PlayerOptionsMenu[playeroption_weaponpref].itemaction = &cv_weaponpref[pind];
    PlayerOptionsMenu[playeroption_setupplayer].itemaction = M_SetupMultiPlayer[pind];
    PlayerOptionsMenu[playeroption_setupcontrol].itemaction = M_Setup_P_Controls[pind];
    PlayerOptionsMenu[playeroption_setupplayer].text = (char*) player_setup_str[pind];
    PlayerOptionsMenu[playeroption_setupcontrol].text = (char*) player_controls_str[pind];
   
    // skin display
    multi_state = &states[mobjinfo[MT_PLAYER].seestate];
    multi_tics = multi_state->tics;
}

static
void M_SetupMultiPlayer1 (int choice)
{
    // set for player 1
    setupm_player = consoleplayer_ptr;
    M_SetupMultiPlayer_pind(0);

    SetupMultiPlayerMenu[setupmultiplayer_options].text = "Player1 config >>";   
    SetupMultiPlayerDef.numitems = setupmultiplayer_options +1;      //remove player2 setup controls and mouse2 

    Push_Setup_Menu (&SetupMultiPlayerDef);
}

// start the multiplayer setup menu, for secondary player (splitscreen mode)
static
void M_SetupMultiPlayer2 (int choice)
{
    // set for splitscreen player 2
    setupm_player = displayplayer2_ptr;  // player 2
    M_SetupMultiPlayer_pind(1);

    SetupMultiPlayerMenu[setupmultiplayer_options].text = "Player2 config >>";   
    SetupMultiPlayerDef.numitems = setupmulti_end;          //activate the setup controls for player 2
   
    Push_Setup_Menu (&SetupMultiPlayerDef);
}


// [Arcade] Panels 3 and 4.  There is no display pointer for them -- that
// concept only ever covered the second of two views -- so the player comes
// from localplayer[], and is NULL when nobody has joined on that panel, which
// only costs the animated skin preview.
static
void M_SetupMultiPlayer_extra( byte pind )
{
    byte pn = localplayer[pind];
    setupm_player = (pn < MAXPLAYERS) ? &players[pn] : NULL;
    M_SetupMultiPlayer_pind( pind );

    SetupMultiPlayerMenu[setupmultiplayer_options].text =
        (char*) player_config_str[pind];
    SetupMultiPlayerDef.numitems = setupmulti_end;

    Push_Setup_Menu (&SetupMultiPlayerDef);
}

static
void M_SetupMultiPlayer3 (int choice)
{
    M_SetupMultiPlayer_extra( 2 );
}

static
void M_SetupMultiPlayer4 (int choice)
{
    M_SetupMultiPlayer_extra( 3 );
}


// Called at cv_splitscreen changes (SplitScreen_OnChange)
void M_Player2_MenuEnable( boolean player2_enable )
{
// activate setup for player 2
    menu_multiplayer = player2_enable;
    if ( player2_enable )
    {
        MultiPlayerMenu[MPM_player2].status = IT_CALL | IT_PATCH;
    }
    else
    {
        MultiPlayerMenu[MPM_player2].status = IT_DISABLED;
        if( MultiPlayerDef.lastOn == MPM_player2)
            MultiPlayerDef.lastOn = MPM_player1;
    }
}


//
//  Draw the multi player setup menu, had some fun with player anim
//
static
void M_DrawSetupMultiPlayerMenu(void)
{
    spritedef_t   * sprdef;
    sprite_frot_t * sprfrot;
    patch_t       * patch;
    byte          * colormap;
    int             mx,my;
    int             st;

    // Draw to screen0, scaled
    mx = SetupMultiPlayerDef.x;
    my = SetupMultiPlayerDef.y;

    // use generic drawer for cursor, items and title
    M_DrawGenericMenu();

    // draw name string
    // [Arcade] These are drawn outside the item loop, so they must be
    // suppressed separately when their menu lines are hidden.
    if( SetupMultiPlayerMenu[setupmultiplayer_name].status != IT_HIDDEN )
    {
        M_DrawTextBox(mx+PLBOXX, my-8, MAXPLAYERNAME, 1);
        V_DrawString (mx+PLBOXX+8 ,my, 0, setupm_name);
    }

    // draw skin string
    if( SetupMultiPlayerMenu[setupmultiplayer_skin].status != IT_HIDDEN )
        V_DrawString (mx+PLBOXX, my+PLSKINNAMEY, 0, setupm_cvskin->string);

    // draw text cursor for name
    if (itemOn==0
        && skullAnimCounter<4 )   //blink cursor
        V_DrawCharacter(mx+98+V_StringWidth(setupm_name), my, '_' | 0x80);  // white

    // anim the player in the box
    if (--multi_tics<=0)
    {
        st = multi_state->nextstate;
        if (st!=S_NULL)
            multi_state = &states[st];
        multi_tics = multi_state->tics;
        if (multi_tics==-1)
            multi_tics=15;
    }

    // skin 0 is default player sprite
    sprdef    = &skins[R_SkinAvailable(setupm_cvskin->string)]->spritedef;
    sprfrot = get_framerotation( sprdef, multi_state->frame & FF_FRAMEMASK, 0 );

    colormap = (setupm_cvcolor->value) ?
         SKIN_TO_SKINMAP( setupm_cvcolor->value )
       : reg_colormaps;  // default green skin

    // draw box around guy
    M_DrawTextBox(mx+PLBOXX,my+PLBOXY, PLBOXW, PLBOXH);

    // draw player sprite
    // temp usage of sprite lump, until end of function
    patch = W_CachePatchNum (sprfrot->pat_lumpnum, PU_CACHE_DEFAULT);  // endian fix
    if( itemOn>0 )  // Edit skin or color
    {
      // Some skins are too large for the screen, cause segfault.
      V_DrawMappedPatch_Box (mx+PLBOXX+8+(PLBOXW*8/2),my+PLBOXY+8+(PLBOXH*8)-8, patch, colormap,
                           1, 0, 300, my+PLBOXY+8+PLBOXH*8 );
    }
    else
    {
      // Some skins are too large for the box
      V_DrawMappedPatch_Box (mx+PLBOXX+8+(PLBOXW*8/2),my+PLBOXY+8+(PLBOXH*8)-8, patch, colormap,
                           mx+PLBOXX+8, my+PLBOXY+8, PLBOXW*8, PLBOXH*8 );
    }
}


//
// Handle Setup MultiPlayer Menu
//
static
void M_MultiPlayer_Responder (int key)
{
    int      l;

    switch (key)
    {
      case KEY_DOWNARROW:
        S_StartSound(menu_sfx_updown);
        if (itemOn+1 >= SetupMultiPlayerDef.numitems)
            itemOn = 0;
        else itemOn++;
        break;

      case KEY_UPARROW:
        S_StartSound(menu_sfx_updown);
        if (itemOn == 0)
            itemOn = SetupMultiPlayerDef.numitems-1;
        else itemOn--;
        break;

      case KEY_LEFTARROW:
        if (itemOn==2)       //player skin
        {
            // unsigned skin index
            if(setupm_skinindex == 0)
                setupm_skinindex = numskins;

            setupm_skinindex--;
            goto change_skin;
        }
        break;

      case KEY_RIGHTARROW:
        if (itemOn==2)       //player skin
        {
            setupm_skinindex++;
            goto change_skin;
        }
        break;

      case KEY_ENTER:
        S_StartSound(menu_sfx_enter);
        goto exitmenu;

      case KEY_ESCAPE:
        S_StartSound(menu_sfx_esc);
        goto exitmenu;

      case KEY_BACKSPACE:
        l = strlen(setupm_name);
        if( l!=0 && itemOn==0 )
        {
            S_StartSound(menu_sfx_val);
            setupm_name[l-1]=0;
        }
        break;

      default:
        if (!is_printable(input_char) || itemOn != 0)
          break;
        l = strlen(setupm_name);
        if (l<MAXPLAYERNAME-1)
        {
            S_StartSound(menu_sfx_val);
            setupm_name[l] = input_char;
            setupm_name[l+1] = 0;
        }
        break;
    }
    return;

change_skin:
    S_StartSound(menu_sfx_val);

    // check skin
    if( setupm_skinindex > numskins-1 )
        setupm_skinindex = 0;

    if( skins[setupm_skinindex] == NULL )  return;

    // check skin change
    // If not updated here then another chance after server start
    // The skin select is the name, not the index.
    CV_Set( setupm_cvskin, skins[setupm_skinindex]->name );  // does net update
    if( setupm_player )
    {
        setupm_player->skin = setupm_skinindex;
    }
    return;

exitmenu:
    // Exit to previous menu.
    Pop_Menu();
    return;
}

static
boolean M_QuitMultiPlayerMenu(void)
{
    int      l;
    // send name if changed
    if (strcmp(setupm_name, setupm_cvname->string))
    {
        // remove trailing whitespaces
        for (l= strlen(setupm_name)-1;
             l>=0 && setupm_name[l]==' '; l--)
            setupm_name[l]=0;

        CV_Set( setupm_cvname, setupm_name );  // does net update
    }
    return true;
}


//===========================================================================
//                              EPISODE SELECT
//===========================================================================

static void M_Episode(int choice);

menuitem_t EpisodeMenu[]=
{
    {IT_CALL | IT_PATCH,"M_EPI1","Knee-Deep in the Dead", M_Episode,'k'},
    {IT_CALL | IT_PATCH,"M_EPI2","The Shores of Hell"   , M_Episode,'t'},
    {IT_CALL | IT_PATCH,"M_EPI3","Inferno"              , M_Episode,'i'},
    {IT_CALL | IT_PATCH,"M_EPI4","Thy Flesh consumed"   , M_Episode,'t'},
    {IT_CALL | IT_PATCH,"M_EPI5","Episode 5"            , M_Episode,'t'},
};

menu_t  EpiDef =
{
    "M_EPISOD",
    "Which Episode?",
    EpisodeMenu,        // menuitem_t ->
    M_DrawGenericMenu,  // drawing routine ->
    NULL,
    sizeof(EpisodeMenu)/sizeof(menuitem_t),
    48,63,              // x,y
    0                   // lastOn, flags
};

//
//      M_Episode
//
int     epi;

static
void M_Episode(int choice)
{
    if ( (gamemode == doom_shareware)
         && choice)
    {
        Push_Setup_Menu(&ReadDef1);
        M_SimpleMessage( text[SWSTRING_NUM] );
        return;
    }

    // Yet another hack...
    if ( (gamemode == doom_registered)
         && (choice > 2))
    {
        GenPrintf( EMSG_warn, "M_Episode: 4th episode requires UltimateDOOM\n");
        choice = 0;
    }

    epi = choice;
    Push_Setup_Menu(&NewDef);
}


//===========================================================================
//                           NEW GAME FOR SINGLE PLAYER
//===========================================================================
static void M_DrawNewGame(void);

static void M_ChooseSkill(int choice);

enum
{
    NG_violence = 3,
    NG_nightmare = 4,
} newgame_e;

menuitem_t NewGameMenu[]=
{
    {IT_CALL | IT_PATCH,"M_JKILL","I'm too young to die.",M_ChooseSkill, 'i'},
    {IT_CALL | IT_PATCH,"M_ROUGH","Hey, not too rough."  ,M_ChooseSkill, 'h'},
    {IT_CALL | IT_PATCH,"M_HURT" ,"Hurt me plenty."      ,M_ChooseSkill, 'h'},
    {IT_CALL | IT_PATCH,"M_ULTRA","Ultra-Violence"       ,M_ChooseSkill, 'u'},
    {IT_CALL | IT_PATCH,"M_NMARE","Nightmare!"           ,M_ChooseSkill, 'n'}
};

menu_t  NewDef =
{
    "M_NEWG",
    "New Game",
    NewGameMenu,        // menuitem_t ->
    M_DrawNewGame,      // drawing routine ->
    NULL,
    sizeof(EpisodeMenu)/sizeof(menuitem_t),
    48,63,              // x,y
    NG_violence            // lastOn
};

static
void M_DrawNewGame(void)
{
    // Draw to screen0, scaled
    //faB: testing with glide
    patch_t* p = W_CachePatchName("M_SKILL",PU_CACHE);  // endian fix
    V_DrawScaledPatch ((BASEVIDWIDTH-p->width)/2,38, p);

    //    V_DrawScaledPatch_Name (54,38, "M_SKILL" );
    M_DrawGenericMenu();
}

static
void M_SingleNewGame(int choice)
{
    // to get out of two player game, and can then backout to multiplayer
    StartSplitScreenGame = false;
    M_Player2_MenuEnable( 0 );

    if( M_already_playing(1) )  return;

    // Restore user settings
    D_End_commandline();
   
    if ( gamemode == doom2_commercial
         || (gamemode == chexquest1 && !modifiedgame) //DarkWolf95: Support for Chex Quest
         )
        Push_Setup_Menu(&NewDef);
    else
        Push_Setup_Menu(&EpiDef);
}

// =========================================================================
//  [Arcade] Cheats menu
// =========================================================================
// Operator convenience, reachable only under -devmode -- the lockdown hides
// the main menu entry for players.  Every item voids the run's score through
// HS_Player_Cheated, exactly the way dying does, so a cheated run plays on but
// records nothing and says PLAYER CHEATED - UNRANKED on the HUD.
//
// Single player only, which the engine already enforces: Command_CheatGod_f
// and Command_CheatGimme_f both return early when multiplayer is set.  The
// menu greys the items out in that case rather than offering something that
// silently does nothing.

static void M_Cheat_God(int choice);
static void M_Cheat_GiveAll(int choice);
static void M_Cheat_NoClip(int choice);
static void M_Cheat_ExitLevel(int choice);
static void M_Draw_Cheats(void);

static menuitem_t  CheatsMenu[] =
{
    {IT_WHITESTRING | IT_CALL, NULL, "God Mode",             M_Cheat_God,       'g'},
    {IT_WHITESTRING | IT_CALL, NULL, "All Weapons and Keys", M_Cheat_GiveAll,   'a'},
    {IT_WHITESTRING | IT_CALL, NULL, "No Clipping",          M_Cheat_NoClip,    'n'},
    {IT_WHITESTRING | IT_CALL, NULL, "Exit Level",           M_Cheat_ExitLevel, 'e'},
};

#define  NUM_CHEATSMENU  (sizeof(CheatsMenu)/sizeof(menuitem_t))

menu_t  CheatsDef =
{
    "M_CHEATS",  // in legacy.wad
    "Cheats",
    CheatsMenu,
    M_Draw_Cheats,
    NULL,
    NUM_CHEATSMENU,
    60,40,
    0
};


// True when there is something to cheat in: a single player game, in a level.
static boolean  M_Cheats_Usable( void )
{
    return ( gamestate == GS_LEVEL )
           && ! multiplayer && ! netgame && ! demoplayback;
}


// Void the score first, then run the cheat, so the run is already marked
// before anything in the level changes.
static void  M_Cheat_Apply( const char * command )
{
    if( ! M_Cheats_Usable() )  return;

    HS_Player_Cheated();
    COM_BufAddText( command );
    COM_BufAddText( "\n" );
    M_Clear_Menus( true );
}


static void M_Cheat_God(int choice)
{
    M_Cheat_Apply( "god" );
}

static void M_Cheat_GiveAll(int choice)
{
    // IDKFA, in the terms the gimme command understands.
    M_Cheat_Apply( "gimme health ammo armor keys weapons" );
}

static void M_Cheat_NoClip(int choice)
{
    M_Cheat_Apply( "noclip" );
}

static void M_Cheat_ExitLevel(int choice)
{
    // Not a cheat in the classic sense, but it skips the rest of the map, so
    // it voids the run like the rest.  It is also much the fastest way to
    // reach the intermission and the high score write while testing.
    M_Cheat_Apply( "exitlevel" );
}


static void  M_Draw_Cheats( void )
{
    boolean usable = M_Cheats_Usable();
    unsigned int i;
    int y;

    // IT_DISABLED is (IT_SPACE | IT_GRAYPATCH) and so replaces IT_CALL --
    // it is not a flag to be OR-ed on top of the enabled status.
    for( i=0; i<NUM_CHEATSMENU; i++ )
        CheatsMenu[i].status = usable ? (IT_WHITESTRING | IT_CALL)
                                      : (IT_WHITESTRING | IT_DISABLED);

    M_DrawGenericMenu();

    y = CheatsDef.y + (NUM_CHEATSMENU * LINEHEIGHT) + 8;
    V_DrawString( CheatsDef.x, y, 0,
                  usable ? "USING ANY OF THESE ENDS SCORING"
                         : "START A ONE PLAYER GAME FIRST" );
}


// =========================================================================
//  [Arcade] Join screen
// =========================================================================
// After the skill is chosen, and before the game starts, each control panel
// presses fire to be counted in.  The page is laid out as the view grid it is
// about to become, so a player presses and watches *their own cell* claim
// itself -- which is the whole point: with panels 1, 3 and 4 joining there is
// nothing left to be confused about, because the square that lit up when you
// pressed is the square you play in.
//
// Only reached when the cabinet has more than one panel (cv_localplayers), so
// a single panel machine starts the game exactly as it did before.

// The page owns its own active flag rather than testing currentMenu:
// M_Clear_Menus leaves currentMenu pointing at the page it closed, so a
// currentMenu test kept firing the countdown every tic after the game had
// already been started -- G_DeferedInitNew once per tic, for ever.
static boolean join_active = false;
static byte  join_pressed[MAXSPLITSCREENPLAYERS];  // panel has pressed in
static int   join_endtic;         // gametic the countdown expires
// What to run once joining is done.  A callback because the two menu routes
// into a game start differently: the Single Player path ends at
// G_DeferedInitNew, while Multiplayer -> Start Game goes through
// M_StartServer, which issues its own command sequence.
static void (*join_startfunc)(void) = NULL;

// [Arcade] Single Player asks the same question -- which panel is playing --
// but there is nobody else to wait for, so the first fire press starts the
// game at once rather than sitting out the countdown.  That is what makes the
// page worth showing on that route: a lone player can take panel 3 and get
// the whole screen, instead of being assumed to be at panel 1.
static boolean join_first_press_starts = false;

static void  M_Join_Drawer(void);

static menuitem_t  JoinMenu[] =
{
    // One inert item: the page is driven entirely from M_Join_Key.
    {IT_SPACE | IT_NOTHING, 0, "", NULL, 0},
};

menu_t  JoinDef =
{
    "M_JOIN",           // supplied in legacy.wad, 223x17
    NULL,               // no fontb title; the graphic is the title
    JoinMenu,
    M_Join_Drawer,
    NULL,
    sizeof(JoinMenu)/sizeof(menuitem_t),
    56,40,
    0
};


// How many panels the cabinet has, and so how many cells the page shows.
static byte  M_Join_NumPanels( void )
{
    int n = cv_localplayers.EV;
    if( n < 1 )  n = 1;
    if( n > MAXSPLITSCREENPLAYERS )  n = MAXSPLITSCREENPLAYERS;
    return (byte) n;
}


// Hand the joined panels to the engine and start the game.  Local players are
// numbered in join order, but each keeps the cell of the panel that pressed --
// see D_Set_View_Cell.
static void  M_Join_Start( void )
{
    byte panel, i, joined = 0;
    byte joined_panel[MAXSPLITSCREENPLAYERS];

    if( ! join_active )  return;
    join_active = false;

    for( panel=0; panel < M_Join_NumPanels(); panel++ )
    {
        if( join_pressed[panel] )
            joined_panel[joined++] = panel;
    }

    // Nobody pressed: start for panel 1 rather than dead-ending on this page.
    if( joined == 0 )
    {
        joined_panel[0] = 0;
        joined = 1;
    }

    // One or two players get the big layout -- the whole screen, or the
    // stacked halves -- rather than a quarter each, however far apart their
    // panels are.  Keeping the panel's own cell only earns its keep at three
    // or more, where the 2x2 is used anyway and there is a gap to place; with
    // two players top-and-bottom there is nothing to be confused about, and a
    // quarter screen each would be a poor trade for it.
    for( i=0; i<joined; i++ )
    {
        // Input always follows the panel that pressed in; the cell only does
        // so once the 2x2 grid is in use.
        D_Set_Panel( i, joined_panel[i] );
        D_Set_View_Cell( i, (joined <= 2) ? i : joined_panel[i] );
    }

    D_Set_Join_Count( joined );

    M_Clear_Menus( true );

    if( join_startfunc )
        join_startfunc();
}


// Called from M_Ticker while the page is up.
void  M_Join_Ticker( void )
{
    if( ! join_active )  return;

    if( (int)gametic >= join_endtic )
        M_Join_Start();
}


// Raw key handling, taken before M_Cabinet_Menu_Key translates panel buttons
// into cursor movement -- this page needs to know *which* panel pressed, and
// that translation throws the identity away.  True when the key is consumed.
boolean  M_Join_Key( uint16_t key )
{
    byte panel, panels = M_Join_NumPanels();

    if( ! join_active )  return false;
    if( key == KEY_ESCAPE )
    {
        join_active = false;    // backing out abandons the join
        return false;
    }

    for( panel=0; panel < panels; panel++ )
    {
        // Fire joins this panel.
        if( key == gamecontrol_pl[panel][gc_fire][0]
            || key == gamecontrol_pl[panel][gc_fire][1] )
        {
            if( ! join_pressed[panel] )
            {
                join_pressed[panel] = 1;
                S_StartSound(menu_sfx_enter);
                if( join_first_press_starts )
                    M_Join_Start();
            }
            return true;
        }

        // Use/open from a panel that is already in starts the game now, so a
        // group that is ready need not sit out the rest of the countdown.
        if( join_pressed[panel]
            && ( key == gamecontrol_pl[panel][gc_use][0]
                 || key == gamecontrol_pl[panel][gc_use][1] ) )
        {
            M_Join_Start();
            return true;
        }
    }

    return true;   // swallow the rest; this page has no cursor to move
}


static void  M_Join_Drawer( void )
{
    byte  panel, panels = M_Join_NumPanels();
    int   secs = (join_endtic - (int)gametic) / TICRATE;
    char  buf[48];

    // One box per panel, laid out as the view grid will be: two panels stack
    // as halves, three or four are a 2x2, matching D_NumViews.
    for( panel=0; panel < panels; panel++ )
    {
        byte col = (panels >= 3) ? (panel & 1) : 0;
        byte row = (panels >= 3) ? (panel >> 1) : panel;
        int  cw  = (panels >= 3) ? (BASEVIDWIDTH/2) : BASEVIDWIDTH;
        int  cx  = col * cw;
        int  cy  = 60 + row * 50;
        const char * state = join_pressed[panel] ? "READY" : "PRESS FIRE";
        int  opt = join_pressed[panel] ? V_WHITEMAP : 0;

        snprintf(buf, sizeof(buf), "PLAYER %d", panel+1);
        V_DrawString( cx + (cw - V_StringWidth(buf))/2, cy, opt, buf );
        V_DrawString( cx + (cw - V_StringWidth((char*)state))/2, cy + 12,
                      opt, (char*) state );
    }

    if( secs < 0 )  secs = 0;
    if( join_first_press_starts )
        snprintf(buf, sizeof(buf), "PRESS FIRE ON YOUR PANEL   (%d)", secs);
    else
        snprintf(buf, sizeof(buf), "STARTING IN %d", secs);
    V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2, BASEVIDHEIGHT - 28, 0, buf );
}


// [Arcade] The Single Player route's game start, deferred until the join
// screen is done with (or run straight away when there is no join screen).
static skill_e  newgame_skill;
static char     newgame_map[16];
static boolean  newgame_split;

static void  M_NewGame_Go( void )
{
    // Reset cumulative timer and begin background recording.  Must precede
    // G_DeferedInitNew so the player-create and map netxcmds land in the demo
    // stream (see HS_NewGame).
    if( ! newgame_split )
        HS_NewGame();
    G_DeferedInitNew( newgame_skill, newgame_map, newgame_split );
}


// Open the page for a game that is about to start.  False when there is
// nothing to ask -- a single panel, or the countdown turned off -- and the
// caller should just start the game itself.
boolean  M_Join_Open( void (*startfunc)(void), boolean first_press_starts )
{
    byte panel;

    if( M_Join_NumPanels() < 2 )  return false;
    if( cv_jointime.EV == 0 )     return false;

    for( panel=0; panel<MAXSPLITSCREENPLAYERS; panel++ )
        join_pressed[panel] = 0;

    join_startfunc = startfunc;
    join_first_press_starts = first_press_starts;
    join_endtic = (int)gametic + (cv_jointime.EV * TICRATE);

    D_Reset_View_Cells();
    join_active = true;
    Push_Setup_Menu( &JoinDef );
    return true;
}


static
void M_ChooseSkill(int choice)
{
    // [Arcade] Nightmare starts without the "are you sure" confirmation.

    // [Arcade] This is a campaign game, so say so.  G_DeferedInitNew does not
    // funnel through Command_ExitGame_f, which is what normally clears the
    // flag, so starting a New Game from the menu *during* a single level run
    // would otherwise carry single_level_mode into the campaign.
    single_level_mode = 0;

    // [Arcade] Ask which panels are playing first, on a cabinet that has more
    // than one.  M_Join_Open returns false when there is nothing to ask, and
    // then the game starts here exactly as it always did.
    newgame_skill = (skill_e) choice;
    dl_strncpy( newgame_map, G_BuildMapName(epi+1,1), sizeof(newgame_map) );
    newgame_split = StartSplitScreenGame;

    if( M_Join_Open( M_NewGame_Go, true ) )   // one player: first press starts
        return;

    D_Clear_Join_Count();   // no join screen: every panel plays
    D_Reset_View_Cells();
    M_NewGame_Go();
    M_Clear_Menus (true);
}

//===========================================================================
//                             OPTIONS MENU
//===========================================================================
//
// M_Options
//

menuitem_t OptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Messages:"       ,&cv_showmessages    ,0},
    {IT_STRING | IT_CVAR,0,"Always Run"      ,&cv_autorun[0]      ,0},
    {IT_STRING | IT_CVAR,0,"Crosshair"       ,&cv_crosshair[0]    ,0},
//    {IT_STRING | IT_CVAR,0,"Crosshair scale" ,&cv_crosshairscale  ,0},
#if 1
    {IT_CALL    | IT_WHITESTRING,0,"Player >>"  ,M_PlayerDirector   ,0},
#else
    {IT_STRING | IT_CVAR,0,"Autoaim"         ,&cv_autoaim[0]      ,0},
#endif

    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"Effects Options >>",&EffectsOption1Def ,50},
    {IT_CALL    | IT_WHITESTRING,0,"Game Options >>"  ,M_GameOption       ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Connect Options >>",&ConnectOptionDef ,0},
    {IT_CALL    | IT_WHITESTRING,0,"Network Options >>",M_NetOption     ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Server Options >>",&ServerOptionsDef  ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Menu Options >>"  ,&MenuOptionsDef    ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Sound Volume >>"  ,&SoundDef          ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Video Options >>" ,&VideoOptionsDef   ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Setup Controls >>",&MControlDef       ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Select Game >>"   ,&GameSelectDef     ,0},  // [Arcade]
};

// [Arcade] index of the Select Game line, which is the last one
enum { OPT_selectgame = (sizeof(OptionsMenu)/sizeof(menuitem_t)) - 1 };

menu_t  OptionsDef =
{
    "M_OPTTTL",
    "Options",
    OptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(OptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};


// =========================================================================
//   [Arcade] GAME SELECT
// =========================================================================
// Switching IWAD needs the startup sequence to run again.  The engine can
// do that (the Launcher's "Iwad" item reaches `goto restart_command` in
// D_DoomMain), but only before D_DoomLoop is entered, and D_DoomLoop is a
// while(1) that never returns.  So instead shut down cleanly and re-exec
// ourselves with a different -game, which re-runs startup from scratch.
//
// -game takes the short name from the gamedesc table in d_main.c, and the
// engine then finds the matching IWAD along its usual search paths, so no
// absolute wad path is needed here.
static void M_SelectGame(int choice);

// Short names from the gamedesc table in d_main.c.  Entries whose IWAD is
// not installed are hidden by M_Configure, so listing extra games is free.
static const char * gameselect_arg[] =
  { "doomu", "doom2", "plutonia", "tnt" };
enum { GS_numgames = 4 };   // the IWAD entries, ahead of the level packs

// [Arcade] Level packs, scanned from legacyhome/levels/ by
// M_Scan_LevelPacks.  Unlike an IWAD switch these need no restart: the map
// command accepts a wad filename, and P_SetupLevel then calls
// P_AddWadFile() to load it and jump to its first map.
#define MAX_LEVELPACK   16
#define LEVELPACK_DIR   "levels"

// map styles a level pack may contain, see M_LevelPack_MapStyle
enum { LPM_mapxx = 0x01, LPM_exmy = 0x02 };

static char  levelpack_path[MAX_LEVELPACK][MAX_WADPATH];
static char  levelpack_name[MAX_LEVELPACK][28];
static char  levelpack_label[MAX_LEVELPACK][48];  // "<game> wad: <NAME>"
static boolean  levelpack_isloaded[MAX_LEVELPACK];
static int   num_levelpack = 0;
static boolean  levelpack_loaded = false;   // see M_LevelPack_Loaded

menuitem_t GameSelectMenu[ GS_numgames + MAX_LEVELPACK ] =
{
    {IT_STRING | IT_CALL, 0, "Ultimate Doom",  M_SelectGame, 0},
    {IT_STRING | IT_CALL, 0, "Doom II",        M_SelectGame, 0},
    {IT_STRING | IT_CALL, 0, "Final Doom: Plutonia", M_SelectGame, 0},
    {IT_STRING | IT_CALL, 0, "Final Doom: TNT",      M_SelectGame, 0},
    // remainder filled in from the levels directory
};

menu_t  GameSelectDef =
{
    "M_OPTTTL",
    "Select Game",
    GameSelectMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(GameSelectMenu)/sizeof(menuitem_t),
    // x=20: pack lines are long ("* Ultimate Doom wad: mapsofchaos-hc"), and
    // at 8 pixels a character they run off a 320 wide screen from x=60.
    20,60,
    0
};

// [Arcade] Was this pack passed on the command line with -file?
// That is how a restart re-adds the packs being kept, so they must come back
// marked as loaded.  -file takes one or more filenames, up to the next switch.
static
boolean  M_LevelPack_InArgv( const char * path )
{
    int  i;
    boolean in_file_list = false;

    for( i = 1; i < myargc; i++ )
    {
        if( strcasecmp(myargv[i], "-file") == 0 )
        {
            in_file_list = true;
            continue;
        }
        if( ! in_file_list )  continue;
        if( myargv[i][0] == '-' )
        {
            in_file_list = false;
            continue;
        }
        if( strcmp(myargv[i], path) == 0 )  return true;
    }
    return false;
}


// [Arcade] Build a pack's menu line: which game it belongs to, and whether
// it is loaded.  gamedesc.gname is the running game ("Doom2", "Ultimate
// Doom", ...), so this stays right for whatever is loaded rather than being
// hardcoded.  A leading "*" marks a pack already added to this session.
static
void  M_LevelPack_SetLabel( int i )
{
    snprintf( levelpack_label[i], sizeof(levelpack_label[0]), "%s%s wad: %s",
              levelpack_isloaded[i] ? "* " : "",
              gamedesc.gname ? gamedesc.gname : "Game",
              levelpack_name[i] );
}


// [Arcade] Classify a wad's maps by reading its lump directory directly.
// Doing it by hand avoids loading the wad into the engine just to find out
// whether it is usable, which is the whole point of the check.
// Return a bitmask, because a pack may carry both: some (Maps of Chaos, for
// one) ship MAPxx and ExMy versions of every level in a single wad, and
// stopping at the first map lump pinned them to whichever style happened to
// come first in the directory, hiding them under the other game.
//   LPM_mapxx = has MAPxx (Doom 2 style),  LPM_exmy = has ExMy (episodic)
static
int  M_LevelPack_MapStyle( const char * path )
{
    unsigned char hdr[12], ent[16];
    FILE * f;
    unsigned int numlumps, infotableofs, i;
    int  style = 0;

    f = fopen( path, "rb" );
    if( ! f )  return 0;

    if( fread( hdr, 1, 12, f ) != 12 )        goto done;
    if( memcmp( hdr, "IWAD", 4 ) != 0
        && memcmp( hdr, "PWAD", 4 ) != 0 )    goto done;

    // wad header is little endian
    numlumps     = hdr[4] | (hdr[5]<<8) | (hdr[6]<<16) | ((unsigned)hdr[7]<<24);
    infotableofs = hdr[8] | (hdr[9]<<8) | (hdr[10]<<16) | ((unsigned)hdr[11]<<24);

    if( numlumps > 65536 )  numlumps = 65536;   // sanity, do not trust the file
    if( fseek( f, infotableofs, SEEK_SET ) != 0 )  goto done;

    for( i = 0; i < numlumps; i++ )
    {
        char nm[9];
        if( fread( ent, 1, 16, f ) != 16 )  break;
        memcpy( nm, &ent[8], 8 );
        nm[8] = '\0';

        // MAPxx
        if( toupper(nm[0])=='M' && toupper(nm[1])=='A' && toupper(nm[2])=='P'
            && isdigit(nm[3]) && isdigit(nm[4]) )
        {
            style |= LPM_mapxx;
        }
        // ExMy
        else if( toupper(nm[0])=='E' && isdigit(nm[1])
            && toupper(nm[2])=='M' && isdigit(nm[3]) )
        {
            style |= LPM_exmy;
        }

        if( style == (LPM_mapxx | LPM_exmy) )  break;  // nothing more to learn
    }

done:
    fclose( f );
    return style;
}


// [Arcade] Find selectable level packs in legacyhome/levels/.
// Every .wad there is offered.  Keeping them out of the iwad search
// directories means no name filtering is needed, so an IWAD or legacy.wad
// can never be mistaken for a level pack.
void M_Scan_LevelPacks( void )
{
    char dirpath[MAX_WADPATH];
    DIR * dp;
    struct dirent * dent;
    int  i, j;

    num_levelpack = 0;

    cat_filename( dirpath, legacyhome, LEVELPACK_DIR );
    if( access( dirpath, R_OK ) < 0 )
    {
        // Create it so it is obvious where level packs belong.
        I_mkdir( dirpath, 0700 );
        return;
    }

    dp = opendir( dirpath );
    if( ! dp )  return;

    while( num_levelpack < MAX_LEVELPACK )
    {
        char * extp;
        int len;

        dent = readdir( dp );
        if( dent == NULL )  break;

        extp = strrchr( dent->d_name, '.' );
        if( (extp == NULL) || (strcasecmp( extp, ".wad" ) != 0) )  continue;

        cat_filename( levelpack_path[num_levelpack], dirpath, dent->d_name );

        // Only offer packs carrying maps the running game can load: a
        // MAPxx-only pack under Ultimate Doom (or an ExMy-only pack under
        // Doom 2) fails.  A pack holding both is offered under either.
        // gamemode is valid here because the scan runs from M_Configure.
        {
            int style = M_LevelPack_MapStyle( levelpack_path[num_levelpack] );
            int want = (gamemode == doom2_commercial) ? LPM_mapxx : LPM_exmy;
            if( ! (style & want) )  continue;   // wrong style, or no maps
        }

        // Menu label is the filename without its extension.
        len = extp - dent->d_name;
        if( len > (int)sizeof(levelpack_name[0]) - 1 )
            len = sizeof(levelpack_name[0]) - 1;
        memcpy( levelpack_name[num_levelpack], dent->d_name, len );
        levelpack_name[num_levelpack][len] = '\0';

        // A pack re-added by a restart arrives on the command line, so it is
        // already loaded and must show as such -- otherwise selecting it
        // would add it a second time.
        levelpack_isloaded[num_levelpack] =
            M_LevelPack_InArgv( levelpack_path[num_levelpack] );
        if( levelpack_isloaded[num_levelpack] )
            levelpack_loaded = true;
        M_LevelPack_SetLabel( num_levelpack );

        num_levelpack ++;
    }
    closedir( dp );

    // readdir order is arbitrary, so sort for a stable menu.
    for( i = 1; i < num_levelpack; i++ )
    {
        for( j = i; j > 0
             && strcasecmp( levelpack_name[j-1], levelpack_name[j] ) > 0; j-- )
        {
            char tmpname[ sizeof(levelpack_name[0]) ];
            char tmplabel[ sizeof(levelpack_label[0]) ];
            char tmppath[ MAX_WADPATH ];
            memcpy( tmpname, levelpack_name[j-1], sizeof(tmpname) );
            memcpy( levelpack_name[j-1], levelpack_name[j], sizeof(tmpname) );
            memcpy( levelpack_name[j], tmpname, sizeof(tmpname) );
            memcpy( tmplabel, levelpack_label[j-1], sizeof(tmplabel) );
            memcpy( levelpack_label[j-1], levelpack_label[j], sizeof(tmplabel) );
            memcpy( levelpack_label[j], tmplabel, sizeof(tmplabel) );
            memcpy( tmppath, levelpack_path[j-1], sizeof(tmppath) );
            memcpy( levelpack_path[j-1], levelpack_path[j], sizeof(tmppath) );
            memcpy( levelpack_path[j], tmppath, sizeof(tmppath) );
        }
    }
}


// [Arcade] Restart the program, optionally switching game.
// Shuts down cleanly and re-execs; does not return.
//   game_idstr : the -game short name, or NULL to keep the current game
//   keep_packs : re-add the currently loaded level packs with -file.
//                false discards them, which is how a pack is unloaded --
//                the engine cannot remove a wad's lumps once loaded.
void M_Restart_Program( const char * game_idstr, boolean keep_packs )
{
    char ** newargv;
    int  i, n = 0;
    boolean in_file_list = false;

    // +3 for "-game" and its name and the NULL terminator, +2 per pack.
    newargv = (char**) malloc( (myargc + 3 + (2*MAX_LEVELPACK)) * sizeof(char*) );
    if( ! newargv )  return;

    newargv[n++] = myargv[0];
    for( i = 1; i < myargc; i++ )
    {
        // When switching game, drop any previous selection, which would
        // otherwise override or conflict with the new one.
        if( game_idstr
            && ( strcasecmp(myargv[i], "-game") == 0
              || strcasecmp(myargv[i], "-iwad") == 0 ) )
        {
            if( (i+1) < myargc )  i++;   // skip its parameter too
            in_file_list = false;
            continue;
        }

        // Always drop any -file list: the set of packs is rebuilt below, so
        // carrying the old one forward would duplicate or resurrect packs.
        // -file takes one or more filenames, up to the next switch.
        if( strcasecmp(myargv[i], "-file") == 0 )
        {
            in_file_list = true;
            continue;
        }
        if( in_file_list )
        {
            if( myargv[i][0] != '-' )  continue;   // still a filename
            in_file_list = false;
        }

        newargv[n++] = myargv[i];
    }
    if( game_idstr )
    {
        newargv[n++] = "-game";
        newargv[n++] = (char*) game_idstr;
    }
    if( keep_packs )
    {
        for( i = 0; i < num_levelpack; i++ )
        {
            if( ! levelpack_isloaded[i] )  continue;
            newargv[n++] = "-file";
            newargv[n++] = levelpack_path[i];
        }
    }
    newargv[n] = NULL;

    // Flush config (devmode only), high scores, demos, and shut down the
    // video/sound devices, but do not exit -- exec replaces us instead.
    // QUIT_normal is required: the other severities force a 3 second sleep
    // in D_Quit_Save.  Suppress the ENDOOM screen it would otherwise print,
    // since we are relaunching rather than returning to a terminal.
    cv_textout.EV = 0;
    D_Quit_Save( QUIT_normal );

    execvp( newargv[0], newargv );

    // Only reached if exec failed; the devices are already down, so there
    // is nothing sensible left to return to.
    I_Error("Could not restart DoomLegacy\n");
}


// [Arcade] Name of the loaded level pack, or NULL when none is loaded.
// Only one can be loaded at a time.  High scores fold this into their key,
// so a pack's records are kept apart from the bare game's.
const char * M_LevelPack_LoadedName( void )
{
    int i;

    for( i = 0; i < num_levelpack; i++ )
    {
        if( levelpack_isloaded[i] )  return levelpack_name[i];
    }
    return NULL;
}


// [Arcade] Has a level pack been loaded into this session?
// Once one is, the attract screen cannot be trusted: the pack overrides the
// IWAD maps, so the built-in demos play back against the wrong levels.
// G_Ticker's idle timeout restarts instead of returning to the title.
boolean  M_LevelPack_Loaded( void )
{
    return levelpack_loaded;
}


//  choice : index into GameSelectMenu; the first GS_numgames entries are
//           IWADs, the rest are level packs
static
void M_SelectGame(int choice)
{
    if( choice >= GS_numgames )
    {
        // Level pack: only load it.  Adding a PWAD at runtime is supported
        // (unlike swapping the IWAD), and its maps then replace the IWAD's,
        // so the ordinary One or Two Player flow plays the pack.  Starting
        // a game here instead would force the player into whichever mode
        // this code picked, and into the pack's first map.
        int lp = choice - GS_numgames;
        int i;
        if( lp >= num_levelpack )  return;

        // One pack at a time.  The cabinet keeps a state a player cannot get
        // wrong -- one IWAD and at most one pack -- and it avoids stacked
        // packs fighting over the same map and texture lumps.
        if( levelpack_isloaded[lp] )
        {
            // Unload.  The engine cannot remove a wad's lumps once loaded,
            // so this restarts with no pack at all.
            levelpack_isloaded[lp] = false;
            CONS_Printf( "\2Unloading %s, restarting.\n", levelpack_name[lp] );
            M_Restart_Program( NULL, false );   // no return
            return;
        }

        if( levelpack_loaded )
        {
            // Another pack is loaded and cannot be removed in place, so
            // restart with this one instead.  Marking it here is what the
            // -file list is built from.
            for( i = 0; i < num_levelpack; i++ )
                levelpack_isloaded[i] = (i == lp);
            CONS_Printf( "\2Switching to %s, restarting.\n", levelpack_name[lp] );
            M_Restart_Program( NULL, true );   // no return
            return;
        }

        // Nothing loaded yet, so it can just be added, with no restart.
        COM_BufAddText( va("addfile \"%s\"\n", levelpack_path[lp]) );
        levelpack_isloaded[lp] = true;
        levelpack_loaded = true;   // attract demos are no longer valid
        M_LevelPack_SetLabel( lp );

        CONS_Printf( "\2%s loaded. Start a One or Two Player game to play it.\n",
                     levelpack_name[lp] );
        return;
    }

    if( choice < 0 )  return;

    // Switching IWAD needs the startup sequence to run again, which is only
    // reachable by restarting the program.
    M_Restart_Program( gameselect_arg[choice], false );   // no return
}

//
//  A smaller 'Thermo', with range given as percents (0-100)
//
static
void M_DrawSlider (int x, int y, int range)
{
    int i;

    // Draw to screen0, scaled
    if (range < 0)
        range = 0;
    if (range > 100)
        range = 100;

    V_DrawScaledPatch_Name(x-8, y, "M_SLIDEL"); // in legacy.wad

    for (i=0 ; i<SLIDER_RANGE ; i++)
      V_DrawScaledPatch_Name(x+i*8, y, "M_SLIDEM"); // in legacy.wad

    V_DrawScaledPatch_Name(x+SLIDER_RANGE*8, y, "M_SLIDER"); // in legacy.wad

    // draw the slider cursor
    V_DrawMappedPatch_Name(x + ((SLIDER_RANGE-1)*8*range)/100, y,
                           "M_SLIDEC", whitemap); // in legacy.wad
}

//===========================================================================
//                        Menu OPTIONS MENU
//===========================================================================

menuitem_t MenuOptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0, "Menu Sounds"     , &cv_menusound     , 0},
    {IT_STRING | IT_CVAR,0, "Screens Link"    , &cv_screenslink   , 0},
    // [Arcade] Operator setting; this whole menu is hidden from players, so
    // it is only reachable under -devmode.  Appended rather than inserted --
    // the lockdown addresses menu items by hardcoded index.
    {IT_STRING | IT_CVAR,0, "Two Player Mode" , &cv_twoplayer     , 0},
    {IT_STRING | IT_CVAR,0, "Control Panels"  , &cv_localplayers  , 0},
    {IT_STRING | IT_CVAR,0, "Join Time"       , &cv_jointime      , 0},
    {IT_STRING | IT_CVAR,0, "Boot Game"       , &cv_defaultgame   , 0},
};

menu_t  MenuOptionsDef =
{
    "M_OPTTTL",
    "Effects",
    MenuOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MenuOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};

//===========================================================================
//                        Effects OPTIONS MENU
//===========================================================================

menuitem_t EffectsOption1Menu[]=
{
    {IT_SUBMENU | IT_WHITESTRING, 0, "Light Options >>"  , &LightingDef  , 'l'},
    {IT_STRING | IT_CVAR,0, "Translucency"    , &cv_translucency  , 0},
    {IT_STRING | IT_CVAR,0, "Fuzzy Shadow"    , &cv_fuzzymode     , 0},
    {IT_STRING | IT_CVAR,0, "Splats"          , &cv_splats        , 0},
    {IT_STRING | IT_CVAR,0, "Max splats"      , &cv_maxsplats     , 0},
    {IT_STRING | IT_CVAR,0, "BloodTime"       , &cv_bloodtime     , 0},
    {IT_STRING | IT_CVAR,0, "Sprites limit"   , &cv_spritelim     , 0},
    {IT_STRING | IT_CVAR,0, "Pickup Flash"    , &cv_pickupflash   , 0},
    {IT_STRING | IT_CVAR,0, "Sky"             , &cv_sky_gen       , 0},
    {IT_STRING | IT_CVAR,0, "Water Effect"    , &cv_water_effect  , 0},
    {IT_STRING | IT_CVAR,0, "Fog Effect"      , &cv_fog_effect    , 0},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"Next"  , &EffectsOption2Def,130},
};

menuitem_t EffectsOption2Menu[]=
{
    {IT_STRING | IT_CVAR,0, "Invul skymap"    , &cv_invul_skymap  , 0},
    {IT_STRING | IT_CVAR,0, "Boom Colormap"   , &cv_boom_colormap , 0},
    {IT_STRING | IT_CVAR,0, "Sound oof 2s"    , &cv_oof_2s , 0},
    {IT_STRING | IT_CVAR,0, "Width Clip"      , &cv_corr_clip_width , 0},
};


menu_t  EffectsOption1Def =
{
    "M_OPTTTL",
    "Effects",
    EffectsOption1Menu,
    M_DrawGenericMenu,
    NULL,
    sizeof(EffectsOption1Menu)/sizeof(menuitem_t),
    60,40,
    0
};

menu_t  EffectsOption2Def =
{
    "M_OPTTTL",
    "Effects",
    EffectsOption2Menu,
    M_DrawGenericMenu,
    NULL,
    sizeof(EffectsOption2Menu)/sizeof(menuitem_t),
    60,40,
    0
};

//===========================================================================
//                        Video OPTIONS MENU
//===========================================================================

// Which line to modify for some menu options.
enum
{
#ifdef __DJGPP__
    VO_gamma = 3,
#else
    VO_gamma = 4,  // Index of Gamma
#endif
} videooptions_e;

menuitem_t VideoOptionsMenu[]=
{
    {IT_STRING | IT_WHITESTRING | IT_SUBMENU,0, "Drawing Options >>"   , &DrawmodeDef, 0},
    {IT_STRING | IT_WHITESTRING | IT_SUBMENU,0, "Video Modes >>"   , &VideoModeDef       , 0},
#ifndef __DJGPP__
    {IT_STRING | IT_CVAR,0,    "Fullscreen"       , &cv_fullscreen    , 0},
#endif
// if these are moved then fix MenuGammaFunc_dependencies
    {IT_STRING | IT_CVAR,0,    "Gamma Function"   , &cv_gammafunc     , 0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,    "Gamma"            , &cv_usegamma      , 0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,    "Black level"      , &cv_black         , 0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,    "Brightness"       , &cv_bright        , 0},
    {IT_STRING | IT_CVAR,0,    "Wait Retrace"     , &cv_vidwait       , 0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,    "Screen Size"      , &cv_viewsize      , 0},
#ifdef FIT_RATIO
    {IT_STRING | IT_CVAR,0,    "View fit"         , &cv_viewfit       , 0},
#endif
    {IT_STRING | IT_CVAR,0,    "Scale Status Bar" , &cv_scalestatusbar, 0},
    {IT_STRING | IT_CVAR,0,    "Dark Back"        , &cv_darkback      , 0},
    {IT_STRING | IT_CVAR,0,    "Console font"     , &cv_con_fontsize  , 0},
    {IT_STRING | IT_CVAR,0,    "Message font"     , &cv_msg_fontsize  , 0},
    {IT_STRING | IT_CVAR,0,    "Show Ticrate"     , &cv_ticrate       , 0},
#ifdef HWRENDER
    //17/10/99: added by Hurdler
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0, "OpenGL 3D Card Options >>", M_OpenGLOption    ,0},
#endif
};

menu_t  VideoOptionsDef =
{
    "M_OPTTTL",
    "Video Options",
    VideoOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(VideoOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};


// Called by CV_gammafunc_OnChange.
void MenuGammaFunc_dependencies( byte gamma_en,
                                 byte black_en, byte bright_en )
{
   // Update menu highlights
   // Gamma
   VideoOptionsMenu[VO_gamma].status = 
     ( gamma_en ) ? (IT_STRING | IT_CVAR | IT_CV_SLIDER )
       : (IT_WHITESTRING | IT_SPACE);
   // Black Level
   VideoOptionsMenu[VO_gamma+1].status = 
     ( black_en ) ? (IT_STRING | IT_CVAR | IT_CV_SLIDER )
       : (IT_WHITESTRING | IT_SPACE);
   // Brightness
   VideoOptionsMenu[VO_gamma+2].status = 
     ( bright_en ) ? (IT_STRING | IT_CVAR | IT_CV_SLIDER )
       : (IT_WHITESTRING | IT_SPACE);
}

//===========================================================================
//                        Mouse OPTIONS MENU
//===========================================================================

menuitem_t MouseOptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Use Mouse",        &cv_usemouse[0]  ,0},
    {IT_STRING | IT_CVAR,0,"Always MouseLook", &cv_alwaysfreelook[0]  ,0},
    {IT_STRING | IT_CVAR,0,"Mouse Move",    &cv_mouse_move[0]   ,0},
    {IT_STRING | IT_CVAR,0,"Invert Mouse",  &cv_mouse_invert    ,0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,"Mouse x Speed", &cv_mouse_sens_x    ,0},
    {IT_STRING | IT_CVAR
     | IT_CV_SLIDER     ,0,"Mouse y Speed", &cv_mouse_sens_y    ,0},
    {IT_STRING | IT_CVAR,0,"Mouse Doubleclick" ,&cv_mouse_double  ,0},
#ifdef SMIF_SDL
    {IT_STRING | IT_CVAR,0,"Mouse motion",  &cv_mouse_motion    ,0},
#endif
    {IT_STRING | IT_CVAR,0,"Grab input", &cv_grabinput ,0},
#if 0
//[WDJ] disabled in 143beta_macosx
//[segabor]
# ifdef MACOS_DI
// specific to macos directory
    ,{IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Configure Input Sprocket >>"  ,macConfigureInput     ,60}
# endif
#endif
};

menu_t  MouseOptionsDef =
{
    "M_OPTTTL",
    "Mouse Options",
    MouseOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MouseOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};

#ifdef JOYSTICK_SUPPORT
//===========================================================================
//                        Joystick OPTIONS MENU
//===========================================================================

menuitem_t JoystickOptionsMenu[]=
{
#ifdef JOY_BUTTONS_DOUBLE
    {IT_STRING | IT_CVAR, 0,"Joystick Doubleclick" ,&cv_joy_double ,0},
#endif
#if 1
    {IT_STRING | IT_CVAR, 0,"Joystick Deadzone" ,&cv_joy_deadzone ,0},
#else
    {IT_STRING | IT_CVAR | IT_CV_SLIDER, 0,"Deadzone" ,&cv_joy_deadzone ,0},
#endif
};

menu_t  JoystickOptionsDef =
{
    "M_OPTTTL",
    "Joystick Options",
    JoystickOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(JoystickOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};
#endif

//===========================================================================
//                        Game OPTIONS MENU
//===========================================================================

menuitem_t GameOptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Item Respawn"        ,&cv_itemrespawn        ,0},
    {IT_STRING | IT_CVAR,0,"Item Respawn time"   ,&cv_itemrespawntime    ,0},
    {IT_STRING | IT_CVAR,0,"Monster Respawn"     ,&cv_respawnmonsters    ,0},
    {IT_STRING | IT_CVAR,0,"Monster Respawn time",&cv_respawnmonsterstime,0},
    {IT_STRING | IT_CVAR,0,"Monster Behavior"	 ,&cv_monbehavior        ,0},
    {IT_STRING | IT_CVAR,0,"Fast Monsters"       ,&cv_fastmonsters       ,0},
    {IT_STRING | IT_CVAR,0,"Predicting Monsters" ,&cv_predictingmonsters ,0},	//added by AC for predmonsters
    {IT_STRING | IT_CVAR,0,"Solid corpse"        ,&cv_solidcorpse        ,0},
#ifdef ENABLE_TIRED_RUN
    {IT_STRING | IT_CVAR,0,"Tired Run"           ,&cv_tired_run          ,0},
    {IT_STRING | IT_CVAR,0,"Drown"               ,&cv_drown              ,0},
#endif
#ifdef MAPADJUST_MENU
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Adv Options >>"      ,M_AdvOption     ,110},
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Map variation >>"    ,M_MapAdjust  ,120},
#else
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Adv Options >>"      ,M_AdvOption     ,120},
#endif
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Bot Options >>"      ,M_BotOption     ,130},
//    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Network Options >>"  ,M_NetOption     ,130}
    {IT_CALL | IT_WHITESTRING,0,"Network Options >>"  ,M_NetOption     ,0}
};

menu_t  GameOptionDef =
{
    "M_OPTTTL",
    "Game Options",
    GameOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(GameOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};

static
void M_GameOption(int choice)
{
    if(!server)
    {
        M_SimpleMessage("You are not the server\nYou cannot change game options\n");
        return;
    }
    Push_Setup_Menu(&GameOptionDef);
}

//===========================================================================
//                        Adv OPTIONS MENU
//===========================================================================

menuitem_t AdvOption1Menu[]=
{
    {IT_STRING | IT_CVAR,0,"Gravity"             ,&cv_gravity            ,0},
    {IT_STRING | IT_CVAR,0,"Monster gravity"     ,&cv_monstergravity     ,0},  // [WDJ]
    {IT_STRING | IT_CVAR,0,"Monster friction"    ,&cv_monsterfriction    ,0},  // [WDJ]
    {IT_STRING | IT_CVAR,0,"Monster door stuck"  ,&cv_doorstuck          ,0},  // [WDJ]
    {IT_STRING | IT_CVAR,0,"Monster remember"    ,&cv_monster_remember   ,0},
    {IT_STRING | IT_CVAR,0,"Mon avoid hazard"    ,&cv_mbf_monster_avoid_hazard ,0},
    {IT_STRING | IT_CVAR,0,"Monster backing"     ,&cv_mbf_monster_backing ,0},
    {IT_STRING | IT_CVAR,0,"Monster pursuit"     ,&cv_mbf_pursuit        ,0},
    {IT_STRING | IT_CVAR,0,"Monster dropoff"     ,&cv_mbf_dropoff        ,0},
    {IT_STRING | IT_CVAR,0,"Monster staylift"    ,&cv_mbf_staylift       ,0},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"Next"  , &AdvOption2Def,130},
};

menuitem_t AdvOption2Menu[]=
{
    {IT_STRING | IT_CVAR,0,"Distance friend"     ,&cv_mbf_distfriend     ,0},
    {IT_STRING | IT_CVAR,0,"Help friend"         ,&cv_mbf_help_friend    ,0},
#ifdef DOGS   
    {IT_STRING | IT_CVAR,0,"Dogs"                ,&cv_mbf_dogs           ,0},
    {IT_STRING | IT_CVAR,0,"Dog jumping"         ,&cv_mbf_dog_jumping    ,0},
#endif
    {IT_STRING | IT_CVAR,0,"Monkeys"             ,&cv_mbf_monkeys        ,0},
    {IT_STRING | IT_CVAR,0,"Falloff"             ,&cv_mbf_falloff        ,0},
    {IT_STRING | IT_CVAR,0,"Voodoo mode"         ,&cv_voodoo_mode        ,0},  // [WDJ]
    {IT_STRING | IT_CVAR,0,"Insta-death"         ,&cv_instadeath         ,0},  // [WDJ]
    {IT_STRING | IT_CVAR,0,"Zero Tags"           ,&cv_zerotags           ,0},
#ifdef GENERATE_BLOCKMAP
    {IT_STRING | IT_CVAR,0,"Blockmap"            ,&cv_blockmap_gen       ,0},  // [MB,WDJ]
#endif
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Games Options >>"    ,M_GameOption    ,130},
};

menu_t  AdvOption1Def =
{
    "M_OPTTTL",
    "Adv1 Options",
    AdvOption1Menu,
    M_DrawGenericMenu,
    NULL,
    sizeof(AdvOption1Menu)/sizeof(menuitem_t),
    60,40,
    0
};

menu_t  AdvOption2Def =
{
    "M_OPTTTL",
    "Adv2 Options",
    AdvOption2Menu,
    M_DrawGenericMenu,
    NULL,
    sizeof(AdvOption2Menu)/sizeof(menuitem_t),
    60,40,
    0
};

static
void M_AdvOption(int choice)
{
    if(!server)
    {
        M_SimpleMessage("You are not the server\nYou cannot change adv options\n");
        return;
    }
    Push_Setup_Menu(&AdvOption1Def);
}

#ifdef MAPADJUST_MENU
//===========================================================================
//                        Map Variation OPTIONS MENU
//===========================================================================

menuitem_t MapAdjustMenu[]=
{
# ifdef MAPTHING_ADJUST_MASTER
    {IT_STRING | IT_CVAR,0,"Variation master"    ,&cv_mapthing_adjust_master      ,0},
        // DO NOT USE. Only controlled the things below, so is not worth the added complexity.
# endif
#ifdef MAPTHING_ADJUST
    {IT_STRING | IT_CVAR,0,"Monster health"      ,&cv_monster_health     ,0},
    {IT_STRING | IT_CVAR,0,"Health pickup"       ,&cv_health_pickup      ,0},
    {IT_STRING | IT_CVAR,0,"Armor pickup"        ,&cv_armor_pickup       ,0},
    {IT_STRING | IT_CVAR,0,"Ammo pickup"         ,&cv_ammo_pickup        ,0},
#endif
#ifdef DOORDELAY_CONTROL
    {IT_STRING | IT_CVAR,0,"Door Delay"          ,&cv_doordelay          ,0},
#endif
#ifdef MONSTER_VARY
    {IT_STRING | IT_CVAR,0,"Monster Vary"        ,&cv_monster_vary       ,0},
    {IT_STRING | IT_CVAR,0,"Vary percent"        ,&cv_vary_percent       ,0},
    {IT_STRING | IT_CVAR,0,"Vary size"           ,&cv_vary_size          ,0},
#endif
#ifdef ENABLE_TELE_CONTROL
    {IT_STRING | IT_CVAR,0,"Teleport mons"       ,&cv_tele_control       ,0},
#endif
#ifdef ENABLE_SLOW_REACT
    {IT_STRING | IT_CVAR,0,"Slow react"          ,&cv_slow_react         ,0},
#endif
};

menu_t  MapVarDef =
{
    NULL,
    "Map Variation",
    MapAdjustMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MapAdjustMenu)/sizeof(menuitem_t),
    60,40,
    0
};

# ifdef MAPTHING_ADJUST_MASTER
// Called by M_MapAdjust, CV_mapthing_adjust_master_OnChange
void M_mapthing_adjust_master_menu_setup( void )
{
    int i;
    for( i = 1; i < MapVarDef.numitems; i++ )
    {
        MapAdjustMenu[i].status = (cv_mapthing_adjust_master.EV)? (IT_STRING | IT_CVAR | IT_WHITESTRING) : (IT_STRING | IT_CVAR);
    }
}
# endif

static
void M_MapAdjust(int choice)
{
    if(!server)
    {
        M_SimpleMessage("You are not the server\nYou cannot change Map Adjust options\n");
        return;
    }
# ifdef MAPTHING_ADJUST_MASTER
    M_mapthing_adjust_master_menu_setup();
# endif
    Push_Setup_Menu(&MapVarDef);
}
#endif

//===========================================================================
//                        Bot OPTIONS MENU
//===========================================================================

menuitem_t BotOptionMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Bot skill"           ,&cv_bot_skill          ,0},
    {IT_STRING | IT_CVAR,0,"Bot speed"           ,&cv_bot_speed          ,0},
    {IT_STRING | IT_CVAR,0,"Bot skin"            ,&cv_bot_skin           ,0},
    {IT_STRING | IT_CVAR,0,"Bot respawn"         ,&cv_bot_respawn_time   ,0},
    {IT_STRING | IT_CVAR,0,"Bot seed"            ,&cv_bot_randseed       ,0},
    {IT_STRING | IT_CVAR,0,"Bot gen"             ,&cv_bot_gen            ,0},
    {IT_STRING | IT_CVAR,0,"Bot grab"            ,&cv_bot_grab           ,0},
    {IT_STRING | IT_CVAR,0,"Bots"                ,&cv_bots               ,0},
};

menu_t  BotDef =
{
    NULL,
    "Bot Options",
    BotOptionMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(BotOptionMenu)/sizeof(menuitem_t),
    60,40,
    0
};

static
void M_BotOption(int choice)
{
    if(!server)
    {
        M_SimpleMessage("You are not the server\nYou cannot change Bot options\n");
        return;
    }
    Push_Setup_Menu(&BotDef);
}

//===========================================================================
//                        Network OPTIONS MENU
//===========================================================================

menuitem_t NetOptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Allow Jump"      ,&cv_allowjump       ,0},
    //SoM: 3/28/2000
    {IT_STRING | IT_CVAR,0,"Allow Rocket Jump",&cv_allowrocketjump,0},
    {IT_STRING | IT_CVAR,0,"Allow autoaim"   ,&cv_allowautoaim    ,0},
    {IT_STRING | IT_CVAR,0,"Allow turbo"     ,&cv_allowturbo      ,0},
    {IT_STRING | IT_CVAR,0,"Allow exitlevel" ,&cv_allowexitlevel  ,0},
    {IT_STRING | IT_CVAR,0,"Allow join player",&cv_allownewplayer ,0},
    {IT_STRING | IT_CVAR,0,"Teamplay"        ,&cv_teamplay        ,0},
    {IT_STRING | IT_CVAR,0,"TeamDamage"      ,&cv_teamdamage      ,0},
    {IT_STRING | IT_CVAR,0,"Fraglimit"       ,&cv_fraglimit       ,0},
    {IT_STRING | IT_CVAR,0,"Timelimit"       ,&cv_dm_timelimit    ,0},
    {IT_STRING | IT_CVAR,0,"Deathmatch Type" ,&cv_deathmatch      ,0},
    {IT_STRING | IT_CVAR,0,"Frag's Weapon Falling", &cv_fragsweaponfalling, 0},
    {IT_STRING | IT_CVAR,0,"Maxplayers"      ,&cv_maxplayers      ,0},
    {IT_CALL | IT_WHITESTRING | IT_YOFFSET, 0,"Games Options >>" ,M_GameOption ,132},
};

// [Arcade] Addressed by position by the lockdown; keep in step with the array.
enum {
    netoption_allowjump = 0,
    netoption_allowrocketjump,
    netoption_allowautoaim,
    netoption_allowturbo,
    netoption_allowexit,
    netoption_allowjoin,
    netoption_teamplay,
    netoption_teamdamage,
    netoption_fraglimit,
    netoption_timelimit,
    netoption_dmtype,
    netoption_fragweapfall,
    netoption_maxplayers,
    netoption_gameoptions,
};

menu_t  NetOptionDef =
{
    "M_OPTTTL",
    "Net Options",
    NetOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(NetOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};

static
void M_NetOption(int choice)
{
    if(!server)
    {
        M_SimpleMessage("You are not the server\nYou cannot change network options\n");
        return;
    }
    Push_Setup_Menu(&NetOptionDef);
}

//===========================================================================
//                    Connect OPTIONS MENU
//===========================================================================


menuitem_t ConnectOptionMenu[]=
{
    {IT_STRING | IT_CVAR,0,"Download files", &cv_download_files , 0},
    {IT_STRING | IT_CVAR,0,"Download savegame", &cv_download_savegame , 0},
    {IT_STRING | IT_CVAR,0,"Netgame repair", &cv_netrepair , 0},
    {IT_STRING | IT_CVAR | IT_CV_STRING,0,"Server 1", &cv_server1 , 0},
    {IT_STRING | IT_CVAR | IT_CV_STRING,0,"Server 2", &cv_server2 , 0},
    {IT_STRING | IT_CVAR | IT_CV_STRING,0,"Server 3", &cv_server3 , 0},
};

menu_t  ConnectOptionDef =
{
    NULL,
    "Connect Options",
    ConnectOptionMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(ConnectOptionMenu)/sizeof(menuitem_t),
    28,40,
    0
};

void M_ConnectOption(int choice)
{
    Push_Setup_Menu(&ConnectOptionDef);
}


//===========================================================================
//                        Server OPTIONS MENU
//===========================================================================
menuitem_t ServerOptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0, "Internet server",     &cv_internetserver   ,  0},
    {IT_STRING | IT_CVAR
        | IT_CV_STRING  ,0, "Master server",       &cv_masterserver     ,  0},
    {IT_STRING | IT_CVAR
        | IT_CV_STRING  ,0, "Server name",         &cv_servername       ,  0},
    {IT_STRING | IT_CVAR,0, "Serve files",    &cv_SV_download_files , 0},
    {IT_STRING | IT_CVAR,0, "Serve savegame", &cv_SV_download_savegame , 0},
    {IT_STRING | IT_CVAR,0, "Serve repair",   &cv_SV_netrepair , 0},
};

menu_t  ServerOptionsDef =
{
    NULL,
    "Server Settings",
    ServerOptionsMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(ServerOptionsMenu)/sizeof(menuitem_t),
    28,40,
    0
};

//===========================================================================
//                    MultiPlayer OPTIONS MENU
//===========================================================================


menuitem_t MPOptionMenu[]=
{
    {IT_SUBMENU | IT_WHITESTRING,0,"Connect Options >>",&ConnectOptionDef ,0},
    {IT_CALL    | IT_WHITESTRING,0,"Network Options >>",M_NetOption       ,0},
    {IT_SUBMENU | IT_WHITESTRING,0,"Server Options >>",&ServerOptionsDef  ,0},
    {IT_CALL    | IT_WHITESTRING,0,"Game Options >>"  ,M_GameOption       ,0},
};

menu_t  MPOptionDef =
{
    "M_OPTTTL",
    "MultiPlayer Options",
    MPOptionMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MPOptionMenu)/sizeof(menuitem_t),
    60,40,
    0
};


//===========================================================================
//                          Read This! MENU 1
//===========================================================================

void M_DrawReadThis1(void);
void M_DrawReadThis2(void);

menuitem_t ReadMenu1[] =
{
    {IT_SUBMENU | IT_NOTHING,0,"",&ReadDef2,0}
};

menu_t  ReadDef1 =
{
    NULL,
    NULL,
    ReadMenu1,
    M_DrawReadThis1,
    NULL,
    sizeof(ReadMenu1)/sizeof(menuitem_t),
    280,185,
    0
};

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
    // Draw to screen0, scaled
    switch ( gamemode )
    {
      case doom2_commercial:
        V_DrawScaledPatch_Name (0,0, "HELP");
        break;
      case doom_shareware:
      case doom_registered:
      case ultdoom_retail:
        V_DrawScaledPatch_Name (0,0, "HELP1");
        break;
      case heretic:
        V_DrawRawScreen_Num(0,0,W_GetNumForName("HELP1"), 320, 200);
        break;
      default:
        break;
    }
    return;
}

//===========================================================================
//                          Read This! MENU 2
//===========================================================================

menuitem_t ReadMenu2[]=
{
    {IT_SUBMENU | IT_NOTHING,0,"",&MainDef,0}
};

menu_t  ReadDef2 =
{
    NULL,
    NULL,
    ReadMenu2,
    M_DrawReadThis2,
    NULL,
    sizeof(ReadMenu2)/sizeof(menuitem_t),
    330,175,
    0
};


//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
    // Draw to screen0, scaled
    switch ( gamemode )
    {
      case ultdoom_retail:
      case doom2_commercial:
        // This hack keeps us from having to change menus.
        V_DrawScaledPatch_Name (0,0, "CREDIT");
        break;
      case doom_shareware:
      case doom_registered:
        V_DrawScaledPatch_Name (0,0, "HELP2");
        break;
      case heretic :
        V_DrawRawScreen_Num(0,0,W_GetNumForName("HELP2"), 320, 200);
      default:
        break;
    }
    return;
}

//===========================================================================
//                        SOUND VOLUME MENU
//===========================================================================

void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_CDAudioVol (int choice);

// [WDJ] unique names, mostly unused
enum
{
    SVM_sfx_vol = 0,
} SVM_sound_e;

// DoomLegacy graphics from legacy.wad: M_CDVOL
menuitem_t SoundMenu[]=
{
    {IT_CVARMAX   | IT_PATCH ,"M_SFXVOL","Sound Volume",&cv_soundvolume  ,'s'},
    {IT_BIGSLIDER | IT_SPACE ,NULL      ,NULL          ,&cv_soundvolume      },
    {IT_CVARMAX   | IT_PATCH ,"M_MUSVOL","Music Volume",&cv_musicvolume  ,'m'},
    {IT_BIGSLIDER | IT_SPACE ,NULL      ,NULL          ,&cv_musicvolume      },
#ifdef CDMUS
#ifdef SMIF_SDL
    // [WDJ] SDL cannot control CDROM volume.
    {IT_SPACE, NULL, NULL, NULL },
    {IT_STRING | IT_CVAR, 0, "CD Volume on", &cd_volume },  // on off
#else   
    {IT_CVARMAX   | IT_PATCH ,"M_CDVOL" ,"CD Volume"   ,&cd_volume       ,'c'}, // in legacy.wad
    {IT_BIGSLIDER | IT_SPACE ,NULL      ,NULL          ,&cd_volume           },
#endif
#endif
#if !defined(CDMUS) || !defined(SMIF_SDL)
    {IT_SPACE, NULL, NULL, NULL },
#endif
#ifdef MUSIC_SOURCE_CONTROL
    {IT_STRING | IT_CVAR | IT_CV_DELAY,  0, "Music src",   &cv_music_source, 0},
#endif
#ifdef SOUND_DEVICE_OPTION
    {IT_STRING | IT_CVAR | IT_CV_DELAY,  0, "Sound Pref",   &cv_snd_opt, 0},
#endif
#ifdef MUSSERV
    {IT_STRING | IT_CVAR | IT_CV_DELAY,  0, "Music Pref",  &cv_musserver_opt, 0},
#endif
    {IT_STRING | IT_CVAR,  0, "Random sound pitch",   &cv_rndsoundpitch, 0},
};

menu_t  SoundDef =
{
    "M_SVOL",
    "Sound Volume",
    SoundMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(SoundMenu)/sizeof(menuitem_t),
#if defined(SOUND_DEVICE_OPTION) || defined(MUSSERV) || defined(MUSIC_SOURCE_CONTROL)
    64,32,
#else
    80,50,
#endif
    0
};


//===========================================================================
//                          CONTROLS MENU
//===========================================================================
menuitem_t MControlMenu[]=
{
    // [Arcade] Guided panel setup first: it is the fast path when standing
    // up a new cabinet, and the pages below are for everything it skips.
    // Appended-in-spirit at the top is safe here -- nothing indexes this
    // menu by position (the lockdown hides its whole Options entry instead).
    {IT_CALL | IT_WHITESTRING, 0,"Guided setup P1", M_Guided_Controls_P1, 0},
    {IT_CALL | IT_WHITESTRING, 0,"Guided setup P2", M_Guided_Controls_P2, 0},
    // [Arcade] Shown only when the cabinet has that many panels; M_Configure.
    {IT_CALL | IT_WHITESTRING, 0,"Guided setup P3", M_Guided_Controls_P3, 0},
    {IT_CALL | IT_WHITESTRING, 0,"Guided setup P4", M_Guided_Controls_P4, 0},
    {IT_SUBMENU | IT_WHITESTRING, 0,"Recommended layout", &RecLayoutDef, 0},
    {IT_STRING | IT_CVAR, 0,"Control per key" ,&cv_controlperkey   ,0},
    {IT_SUBMENU | IT_WHITESTRING, 0,"Mouse Options >>" ,&MouseOptionsDef   , 'm'},
    {IT_SUBMENU | IT_WHITESTRING, 0,"Second Mouse config >>", &SecondMouseCfgdef, 0},
    {IT_CALL | IT_WHITESTRING, 0,"Player1 Controls >>", M_Setup_P1_Controls, '1'},
    {IT_CALL | IT_WHITESTRING, 0,"Player2 Controls >>", M_Setup_P2_Controls, '2'},
    // [Arcade] The full per-action binding page for the extra panels.  The
    // guided setup above only teaches the ten controls a standard six button
    // panel needs; a panel with more buttons binds the rest here.  Shown only
    // when the cabinet has that many panels; see M_Configure.
    {IT_CALL | IT_WHITESTRING, 0,"Player3 Controls >>", M_Setup_P3_Controls, '3'},
    {IT_CALL | IT_WHITESTRING, 0,"Player4 Controls >>", M_Setup_P4_Controls, '4'},
#ifdef JOYSTICK_SUPPORT   
    {IT_SUBMENU | IT_WHITESTRING, 0,"Joystick Options >>" ,&JoystickOptionsDef   , 'j'},
#endif
};

// [Arcade] This menu *is* addressed by position now (the lockdown hides the
// rows for panels the cabinet does not have), so the indices are named.  Keep
// in step with MControlMenu above.
enum {
    mcontrol_guided_p1 = 0,
    mcontrol_guided_p2,
    mcontrol_guided_p3,
    mcontrol_guided_p4,
    mcontrol_reclayout,
    mcontrol_controlperkey,
    mcontrol_mouseopt,
    mcontrol_mouse2cfg,
    mcontrol_p1_controls,
    mcontrol_p2_controls,
    mcontrol_p3_controls,
    mcontrol_p4_controls,
};

menu_t  MControlDef =
{
    "M_OPTTTL",
    "Controls",
    MControlMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(MControlMenu)/sizeof(menuitem_t),
    60,40,
    0
};


//===========================================================================
//  [Arcade] Recommended panel layout (informational)
//===========================================================================
// The layout the cabinet was actually play-tested with, on the usual
// six-button arrangement: 1 2 3 across the top row, 4 5 6 below.
//
// Everything is placed at an explicit x rather than drawn as monospace ASCII
// art, because hu_font is *proportional* (V_DrawString advances by each
// patch's own width) and space is only 4px, so columns built out of spaces
// do not line up.  Only characters in '!'..'_' exist -- no '|', and
// lowercase is folded to uppercase -- hence "V" for the down arrow.

// True while the page is being shown as the guided setup's opening screen,
// waiting for a button before the prompts start.  M_Responder consumes the
// next keypress; see the guided setup block below.
static boolean  guided_intro = false;

static void  M_Centre_At( int cx, int y, int option, const char * s )
{
    V_DrawString( cx - (V_StringWidth((char*)s) / 2), y, option, (char*)s );
}

#define RL_STICK_X    58        // stick column centre
#define RL_BTN_X      152       // centre of button 1 / 4
#define RL_BTN_STEP   46        // to buttons 2/5 and 3/6
#define RL_ROW1_Y     54        // top button row
#define RL_ROW2_Y     78        // bottom button row

static void M_Draw_RecLayout( void )
{
    static const char * btn_top[3] = { "(1)", "(2)", "(3)" };
    static const char * btn_bot[3] = { "(4)", "(5)", "(6)" };
    static const char * legend_l[3] =
      { "1  FIRE", "2  STRAFE LEFT", "3  STRAFE RIGHT" };
    static const char * legend_r[3] =
      { "4  USE / OPEN", "5  WEAPON DOWN", "6  WEAPON UP" };
    int  i, y;

    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );

    M_Centre_At( BASEVIDWIDTH/2, 16, V_WHITEMAP, "RECOMMENDED PANEL LAYOUT" );

    // Stick, drawn as a four-way.
    M_Centre_At( RL_STICK_X,      RL_ROW1_Y - 8, 0, "^" );
    M_Centre_At( RL_STICK_X - 22, RL_ROW1_Y + 8, 0, "<" );
    M_Centre_At( RL_STICK_X,      RL_ROW1_Y + 8, 0, "O" );
    M_Centre_At( RL_STICK_X + 22, RL_ROW1_Y + 8, 0, ">" );
    M_Centre_At( RL_STICK_X,      RL_ROW1_Y + 24, 0, "V" );
    M_Centre_At( RL_STICK_X,      RL_ROW2_Y + 20, V_WHITEMAP, "STICK" );

    // Buttons, two rows of three.
    for( i=0; i<3; i++ )
    {
        int bx = RL_BTN_X + (i * RL_BTN_STEP);
        M_Centre_At( bx, RL_ROW1_Y, V_WHITEMAP, btn_top[i] );
        M_Centre_At( bx, RL_ROW2_Y, V_WHITEMAP, btn_bot[i] );
    }

    y = 112;
    for( i=0; i<3; i++ )
    {
        V_DrawString( 24,  y, 0, (char*) legend_l[i] );
        V_DrawString( 168, y, 0, (char*) legend_r[i] );
        y += 12;
    }

    M_Centre_At( BASEVIDWIDTH/2, 158, 0, "STICK MOVES AND TURNS" );

    if( guided_intro )
    {
        M_Centre_At( BASEVIDWIDTH/2, 174, V_WHITEMAP, "PRESS ANY BUTTON TO BEGIN" );
        M_Centre_At( BASEVIDWIDTH/2, 186, 0, "ESC TO CANCEL" );
    }
    else
    {
        M_Centre_At( BASEVIDWIDTH/2, 174, 0, "SETUP ASKS STICK FIRST, THEN 1-6" );
    }
}

menuitem_t RecLayoutMenu[] =
{
    // Invisible item: any select backs out to the controls menu, as the
    // Read This screens do.
    {IT_SUBMENU | IT_NOTHING, 0, "", &MControlDef, 0}
};

menu_t  RecLayoutDef =
{
    NULL,
    NULL,
    RecLayoutMenu,
    M_Draw_RecLayout,
    NULL,
    sizeof(RecLayoutMenu)/sizeof(menuitem_t),
    160, 190,
    0
};


void M_DrawControl(void);               // added 3-1-98
void M_ChangeControl(int choice);

//
// this is the same for all control pages
//
// IT_CONTROL: alphaKey is the control to be changed
menuitem_t ControlMenu[]=
{
    {IT_CONTROL, 0,"Fire"        ,M_ChangeControl,gc_fire       },
    {IT_CONTROL, 0,"Use/Open"    ,M_ChangeControl,gc_use        },
    {IT_CONTROL, 0,"Jump"        ,M_ChangeControl,gc_jump       },
    {IT_CONTROL, 0,"Forward"     ,M_ChangeControl,gc_forward    },
    {IT_CONTROL, 0,"Backpedal"   ,M_ChangeControl,gc_backward   },
    {IT_CONTROL, 0,"Turn Left"   ,M_ChangeControl,gc_turnleft   },
    {IT_CONTROL, 0,"Turn Right"  ,M_ChangeControl,gc_turnright  },
    {IT_CONTROL, 0,"Run"         ,M_ChangeControl,gc_speed      },
    {IT_CONTROL, 0,"Strafe On"   ,M_ChangeControl,gc_strafe     },
    {IT_CONTROL, 0,"Strafe Left" ,M_ChangeControl,gc_strafeleft },
    {IT_CONTROL, 0,"Strafe Right",M_ChangeControl,gc_straferight},
    {IT_CONTROL, 0,"Look Up"     ,M_ChangeControl,gc_lookup     },
    {IT_CONTROL, 0,"Look Down"   ,M_ChangeControl,gc_lookdown   },
    {IT_CONTROL, 0,"Center View" ,M_ChangeControl,gc_centerview },
    {IT_CONTROL, 0,"Mouselook"   ,M_ChangeControl,gc_mouseaiming},

    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"next" ,&ControlDef2,128}
};

menu_t  ControlDef =
{
    "M_CONTRO", // in legacy.wad
    "Setup Controls",
    ControlMenu,
    M_DrawControl,
    NULL,
    sizeof(ControlMenu)/sizeof(menuitem_t),
    50,40,
    0
};

// IT_CONTROL: alphaKey is the control to be changed
menuitem_t ControlMenu2[]=
{
  {IT_CONTROL, 0,"Fist/Chainsaw"  ,M_ChangeControl,gc_weapon1},
  {IT_CONTROL, 0,"Pistol"         ,M_ChangeControl,gc_weapon2},
  {IT_CONTROL, 0,"Shotgun/Double" ,M_ChangeControl,gc_weapon3},
  {IT_CONTROL, 0,"Chaingun"       ,M_ChangeControl,gc_weapon4},
  {IT_CONTROL, 0,"Rocket Launcher",M_ChangeControl,gc_weapon5},
  {IT_CONTROL, 0,"Plasma rifle"   ,M_ChangeControl,gc_weapon6},
  {IT_CONTROL, 0,"BFG"            ,M_ChangeControl,gc_weapon7},
  {IT_CONTROL, 0,"Chainsaw"       ,M_ChangeControl,gc_weapon8},
  {IT_CONTROL, 0,"Previous Weapon",M_ChangeControl,gc_prevweapon},
  {IT_CONTROL, 0,"Next Weapon"    ,M_ChangeControl,gc_nextweapon},
  {IT_CONTROL, 0,"Best Weapon"    ,M_ChangeControl,gc_bestweapon},
  {IT_CONTROL, 0,"Inventory Left" ,M_ChangeControl,gc_invprev},  
  {IT_CONTROL, 0,"Inventory Right",M_ChangeControl,gc_invnext},
  {IT_CONTROL, 0,"Inventory Use"  ,M_ChangeControl,gc_invuse },
  {IT_CONTROL, 0,"Fly down"       ,M_ChangeControl,gc_flydown},
                       
  {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"next"    ,&ControlDef3,148}
};

menu_t  ControlDef2 =
{
    "M_CONTRO", // in legacy.wad
    "Setup Controls",
    ControlMenu2,
    M_DrawControl,
    NULL,
    sizeof(ControlMenu2)/sizeof(menuitem_t),
    50,40,
    0
};

// IT_CONTROL: alphaKey is the control to be changed
menuitem_t ControlMenu3[]=
{
  {IT_CONTROL, 0,"Talk key"       ,M_ChangeControl,gc_talkkey},
  {IT_CONTROL, 0,"Rankings/Scores",M_ChangeControl,gc_scores },
  {IT_CONTROL, 0,"Console"        ,M_ChangeControl,gc_console},
  {IT_CONTROL, 0,"Screenshot"     ,M_ChangeControl,gc_screenshot},
  {IT_WHITESTRING | IT_SPACE, 0, "Joystick and Mouse Only" ,0},
  {IT_CONTROL, 0,"Main menu"      ,M_ChangeControl,gc_menuesc},
  {IT_CONTROL, 0,"Pause"          ,M_ChangeControl,gc_pause},
  {IT_CONTROL, 0,"Automap"        ,M_ChangeControl,gc_automap},
                       
  {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0,"next"    ,&ControlDef,128}
};

menu_t  ControlDef3 =
{
    "M_CONTRO", // in legacy.wad
    "Setup Controls",
    ControlMenu3,
    M_DrawControl,
    NULL,
    sizeof(ControlMenu3)/sizeof(menuitem_t),
    50,40,
    0
};


//
// Start the controls menu, setting it up for either the console player,
// or the secondary splitscreen player
//
static  byte  controls_player;
static  int   (*setupcontrols)[2];  // pointer to the gamecontrols of the player being edited

// called by player1 multiplayer setup menu
void M_Setup_P1_Controls(int choice)
{
    // set the gamecontrols to be edited
    // was called from main Controls (for console player, then)
    controls_player = 0;
    setupcontrols = gamecontrol;
    currentMenu->lastOn = itemOn;
    Push_Setup_Menu(&ControlDef);
}

// called by player2 multiplayer setup menu
void M_Setup_P2_Controls(int choice)
{
    // set the gamecontrols to be edited
    controls_player = 1;
    setupcontrols = gamecontrol2;
    currentMenu->lastOn = itemOn;
    Push_Setup_Menu(&ControlDef);
}

// [Arcade] Panels 3 and 4.  gamecontrol_pl is the indexed table the old
// gamecontrol / gamecontrol2 names are macros onto.
void M_Setup_P3_Controls(int choice)
{
    controls_player = 2;
    setupcontrols = gamecontrol_pl[2];
    currentMenu->lastOn = itemOn;
    Push_Setup_Menu(&ControlDef);
}

void M_Setup_P4_Controls(int choice)
{
    controls_player = 3;
    setupcontrols = gamecontrol_pl[3];
    currentMenu->lastOn = itemOn;
    Push_Setup_Menu(&ControlDef);
}


//
//  Draws the Customized Controls menu
//
void M_DrawControl(void)
{
    char     tmp[50];
    int      i;
    int      keys[2];
    menuitem_t * mip;

    // draw title, strings and submenu
    M_DrawGenericMenu();

    // [Arcade] Name the actual panel.  This was a two-way choice, so with
    // four panels the third and fourth both announced themselves as PLAYER2 --
    // which is exactly the confusion this screen must not create when an
    // operator is binding buttons.
    {
        char hdr[64];
        snprintf( hdr, sizeof(hdr),
                  "PLAYER%d: ENTER TO CHANGE, BACKSPACE TO CLEAR",
                  (int)controls_player + 1 );
        M_CentreText( ControlDef.y-12, hdr );
    }

    for(i=0;i<currentMenu->numitems;i++)
    {
        mip = & currentMenu->menuitems[i];
        if (mip->status != IT_CONTROL)
            continue;

        // alphaKey is the control to be changed
        keys[0] = setupcontrols[mip->alphaKey][0];
        keys[1] = setupcontrols[mip->alphaKey][1];

        tmp[0]='\0';
        if (keys[0] == KEY_NULL && keys[1] == KEY_NULL)
        {
            strcpy(tmp, "---");
        }
        else
        {
            if( keys[0] != KEY_NULL )
                strcat (tmp, G_KeynumToString (keys[0]));

            if( keys[0] != KEY_NULL && keys[1] != KEY_NULL )
                strcat(tmp," or ");

            if( keys[1] != KEY_NULL )
                strcat (tmp, G_KeynumToString (keys[1]));


        }
        V_DrawString(ControlDef.x+220-V_StringWidth(tmp), ControlDef.y + i*8,V_WHITEMAP, tmp);
    }

}

static int controltochange;

void M_ChangecontrolResponse(event_t* ev)
{
    int        control;
    int        found;
    int        ch=ev->data1;

    // The new key for a control function.
    // ESCAPE cancels assignment.
    // Not allowed to assign KEY_ESCAPE nor KEY_PAUSE to another control function.

    // [WDJ] Test has interference with joystick assign.
    // Input gamecontrol[gc_menuesc] is translated to KEY_ESCAPE.
    //   This prevents reprogram of joystick buttons using button already programmed for gc_menuesc, which is probably a good thing.
    // If cancel gamecontrol[gc_menuesc] assignment, menus may become inaccessible to joystick.
    // Joystick gamecontrol[gc_pause]: does not cancel this assign, does not interfere.
    //   Pause game does test of gamecontrol[gc_pause] directly, not translated.
    if (ch!=KEY_ESCAPE && ch!=KEY_PAUSE)
    {

        switch (ev->type)
        {
          // ignore mouse/joy movements, just get buttons
          case ev_mouse:
            ch = KEY_NULL;      // no key
            break;

          // keypad arrows are converted for the menu in cursor arrows
          // so use the event instead of ch
          case ev_keydown:
            ch = ev->data1;
            break;

          default:
            break;
        }

        control = controltochange;

        // check if we already entered this key
        found = -1;
        if (setupcontrols[control][0]==ch)
            found = 0;
        else
        if (setupcontrols[control][1]==ch)
            found = 1;
        if (found>=0)
        {
            // If controltochange has existing assignment of same button, then
            // replace mouse and joy clicks by double clicks.
#if 1
            if( ch>=KEY_MOUSE1 && ch<KEY_JOY0BUT0 )  // For all MOUSE input
            {
                // Spacing of KEY_MOUSEx to KEY_MOUSExDBL is uniform for all MOUSE.
                setupcontrols[control][found] = ch + (KEY_MOUSE1DBL - KEY_MOUSE1);
                goto done;
            }
#else
            // FIXME: first mouse only
            if (ch>=KEY_MOUSE1 && ch<=KEY_MOUSE1+MOUSEBUTTONS)
            {
                setupcontrols[control][found] = ch + (KEY_MOUSE1DBL - KEY_MOUSE1);
                goto done;
            }
#endif

#ifdef JOYSTICK_SUPPORT
#ifdef JOY_BUTTONS_DOUBLE
            else if( ch>=KEY_JOY0BUT0 && ch<=(KEY_JOYLAST) )  // all JOY input
            {
                // Change to joystick doubleclicks.
                // Spacing of JOYxBUTn to JOYxBUTnDBL is uniform for all JOY and all BUT.
                setupcontrols[control][found] = ch + (KEY_JOY0BUT0DBL - KEY_JOY0BUT0);
                goto done;
            }
#endif
#endif
        }

        // Direct replacement is default.
        {
            // [WDJ] FIXME: This is flaky logic.
            // check if change key1 or key2, or replace the two by the new
            found = 0;
            if (setupcontrols[control][0] == KEY_NULL)
                found++;
            if (setupcontrols[control][1] == KEY_NULL)
                found++;

            if( (controls_player == 0) && (control == gc_menuesc)
                && ((ch == KEY_NULL) || (ch == KEY_BACKSPACE)) )
            {
                // Protect gc_menuesc against being set to NULL.
                goto done;
            }

            if (found==2)
            {
                found = 0;
                setupcontrols[control][1] = KEY_NULL;  //replace key1 ,clear key2
            }
            G_CheckDoubleUsage(ch);
            setupcontrols[control][found] = ch;
        }
    }

done:
    M_StopMessage(0);
}

//===========================================================================
//  [Arcade] Guided control setup
//===========================================================================
// Walks an operator through the ten actions a cabinet panel actually needs,
// binding each to whatever button they press: a 4-way stick plus six
// buttons.  Everything else (run, jump, weapon slots, console, automap) is
// left to the ordinary Setup Controls pages, which are devmode-only anyway.
//
// Built on the same MM_EVENTHANDLER message plumbing M_ChangeControl uses,
// so key capture, the message box and event routing are all shared.  Mouse
// and joystick buttons arrive as ev_keydown with codes in the key space, so
// a panel wired through any of the three works without special handling.

// Prompts name the *physical* control, not what it does in the simulation:
// an operator wiring a panel knows they are pushing the stick left, and does
// not want to reason about whether that turns or strafes.  The four stick
// directions still land in the Look-and-Move slots underneath (the stick's
// left/right is pair A, the strafe buttons are pair B), so the "Look and
// Move" / "WASD" selector goes on swapping them for the player afterwards.
typedef struct
{
    int          ck;        // CK_* slot in the key table
    const char * label;
} guided_step_t;

// Stick first, then the six buttons in *button number* order, matching the
// recommended layout page so an operator can follow it straight down.  The
// numbers are only the recommendation -- a panel wired differently still
// works, the operator just presses whatever they want for that action.
static const guided_step_t  guided_steps[] =
{
    { CK_forward,      "STICK UP"                },
    { CK_backward,     "STICK DOWN"              },
    { CK_pair_a_left,  "STICK LEFT"              },
    { CK_pair_a_right, "STICK RIGHT"             },
    { CK_fire,         "BUTTON 1 - FIRE"         },
    { CK_pair_b_left,  "BUTTON 2 - STRAFE LEFT"  },
    { CK_pair_b_right, "BUTTON 3 - STRAFE RIGHT" },
    { CK_use,          "BUTTON 4 - USE / OPEN"   },
    { CK_prevweapon,   "BUTTON 5 - WEAPON DOWN"  },
    { CK_nextweapon,   "BUTTON 6 - WEAPON UP"    },
};

#define GUIDED_NUM_STEPS  ((int)(sizeof(guided_steps)/sizeof(guided_steps[0])))

static int  guided_step = -1;   // -1 when not running
static int  guided_keys[CK_NUMKEYS];

static void M_Guided_Response(event_t * ev);

static void M_Guided_Prompt( void )
{
    snprintf( msgtmp, MSGTMP_LEN,
              "PLAYER %d CONTROL SETUP\n\n"
              "Press the control for\n%s\n\n"
              "%d of %d\nESC to cancel",
              controls_player + 1,
              guided_steps[guided_step].label,
              guided_step + 1, GUIDED_NUM_STEPS );
    msgtmp[MSGTMP_LEN-1] = '\0';

    M_StartMessage( msgtmp, M_Guided_Response, MM_EVENTHANDLER );
}

// The wizard opens by pushing RecLayoutDef, so on the way out that page is
// still the menu the message system will restore to -- leaving the operator
// staring at the diagram again instead of back at the controls menu.  Drop
// it off the stack.  Guarded because M_StopMessage has already restored
// currentMenu by the time this runs, and only that page should be popped.
static void M_Guided_Leave_Intro_Page( void )
{
    if( currentMenu == &RecLayoutDef )
        Pop_Menu();
}

static void M_Guided_Response( event_t * ev )
{
    int  ch;

    // Only presses.  Without this the key-up of the very same press would
    // land on the next prompt and bind two actions to one button.
    if( ev->type != ev_keydown )  return;
    ch = ev->data1;

    M_StopMessage(0);

    // Cancel abandons the whole table; a half-taught panel is worse than the
    // one that was working before.  KEY_PAUSE is refused for the same reason
    // M_ChangecontrolResponse refuses it.
    if( ch == KEY_ESCAPE || ch == KEY_PAUSE || ch == KEY_NULL )
    {
        guided_step = -1;
        M_Guided_Leave_Intro_Page();
        return;
    }

    guided_keys[ guided_steps[guided_step].ck ] = ch;

    guided_step++;
    if( guided_step < GUIDED_NUM_STEPS )
    {
        M_Guided_Prompt();
        return;
    }

    guided_step = -1;

    // Hand the table to the scheme machinery rather than writing bindings
    // directly: ControlScheme_Apply owns exactly these ten actions, and this
    // way the "Look and Move" / "WASD" selector keeps working on the custom
    // layout instead of overwriting it.
    G_Save_CustomControls( controls_player, guided_keys );

    // Pop before raising the message, so the message restores to the
    // controls menu rather than to the layout page we came in on.
    M_Guided_Leave_Intro_Page();

    M_SimpleMessage( "Controls saved.\n\n"
                     "Written to config.cfg when this\ndevmode session quits." );
}

// Called from M_Responder when the intro page's "press any button" is
// answered.  Split out so the page and the prompts stay separate screens:
// M_StartMessage makes MessageDef the current menu, so the layout cannot be
// shown behind the prompt box anyway.
static void M_Guided_Begin_Steps( void )
{
    guided_step = 0;
    M_Guided_Prompt();
}

// Menu entries; controls_player selects which panel is being wired.  Both
// open on the recommended-layout page so the operator can see what they are
// about to be asked for, in the order they will be asked.
static void M_Guided_Start( int pind )
{
    controls_player = pind;
    setupcontrols = gamecontrol_pl[pind];   // [Arcade] was a 0/1 choice
    guided_step = -1;
    guided_intro = true;
    Push_Setup_Menu( &RecLayoutDef );
}

static void M_Guided_Controls_P1( int choice )
{
    (void)choice;
    M_Guided_Start( 0 );
}

static void M_Guided_Controls_P2( int choice )
{
    (void)choice;
    M_Guided_Start( 1 );
}

static void M_Guided_Controls_P3( int choice )   // [Arcade]
{
    (void)choice;
    M_Guided_Start( 2 );
}

static void M_Guided_Controls_P4( int choice )   // [Arcade]
{
    (void)choice;
    M_Guided_Start( 3 );
}


void M_ChangeControl(int choice)
{
    controltochange = currentMenu->menuitems[choice].alphaKey;
    snprintf (msgtmp, MSGTMP_LEN,
              "Hit the new key for\n%s\nESC for Cancel", currentMenu->menuitems[choice].text);
    msgtmp[MSGTMP_LEN-1] = '\0';

    M_StartMessage (msgtmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
}


//===========================================================================
// Video mode and drawmode test and draw support.

//max modes displayed in one column
//#define MAXCOLUMNMODES   10
#define MAXCOLUMNMODES   8
#define MAXMODEDESCS     (MAXCOLUMNMODES*3)
#define MODES_X          16
#define MODES_Y          44
#define MODES_X_INC      (8*13)
#define MODES_Y_INC      8
#define MODETXT_Y        (MODES_Y + 60 + 24)

static int vidm_testing_cnt=0;  // test videomode failsafe
static int vidm_current=0;  // modedesc index
static int vidm_nummodes;
static int vidm_column_size;


// Draw the instructions for the video mode setting
//   vm_mode : 1 for setting video mode
//   current_mode_name : the desc string for the current mode
//   mode_name : the desc string for the selected mode
static
void  draw_set_mode_instructions( byte vm_mode, const char * current_mode_name, const char * sel_mode_name )
{
    char  temp[80];
    byte  test_mkcfg = 0;

    if (vidm_testing_cnt>0)
    {
        sprintf(temp, "TESTING MODE %s", sel_mode_name );
        M_CentreText(MODETXT_Y + 20, temp );
        M_CentreText(MODETXT_Y + 30, "Please wait 5 seconds..." );
    }
#ifdef CONFIG_MENU_PAGE
    else if( menu_cfg_editing )
    {
        M_CentreText(MODETXT_Y,"Press ENTER to set mode");
        M_CentreText(MODETXT_Y + 40,"Press ESC to exit");
        test_mkcfg = 1;
    }
#endif
    else
    {
//        M_CentreText(MODETXT_Y,"Press ENTER to set mode");
        M_CentreText(MODETXT_Y,"Press S to set mode");

        M_CentreText(MODETXT_Y + 10,"T to test mode for 5 seconds");

        if( current_mode_name )
        {
            sprintf(temp, "D to set default to  %s", current_mode_name );
            M_CentreText(MODETXT_Y + 20,temp);
        }

        if( vm_mode )
        {
          sprintf(temp, "Current default : %dx%d (%d bits)", cv_scr_width.value, cv_scr_height.value, cv_scr_depth.value);
        }
        else
        {
#if 1
//          sprintf(temp, "Current drawmode : %s", current_mode_name );
//          sprintf(temp, "Current default : %s", CV_get_possiblevalue_string( drawmode_sel_t, cv_drawmode.value ) );
          sprintf(temp, "Current default : %s", cv_drawmode.string );
#else
          // Redundant, looks like an error.
          sprintf(temp, "Current drawmode : %s %s", current_mode_name, rendermode_name[rendermode] );
#endif
          test_mkcfg = 1;
        }
        M_CentreText(MODETXT_Y + 30,temp);

        M_CentreText(MODETXT_Y + 40,"Press ESC to exit");
    }

    if( test_mkcfg && ! M_Have_configfile_drawmode() )
    {
        // is current_mode_name only during drawmode menu
        sprintf(temp, "C to make config: %s", CV_get_possiblevalue_string( drawmode_sel_t, cv_drawmode.EV ) );
#if 1
        V_DrawString( 2, 24, V_WHITEMAP, temp);
#else
        M_CentreText(MODETXT_Y + 20,temp);
#endif
    }

    // Draw the cursor for the VidMode menu
    if (skullAnimCounter<4)    //use the Skull anim counter to blink the cursor
//    if( (itemOn > 0) && skullAnimCounter<4 )    //use the Skull anim counter to blink the cursor
    {
        int i = MODES_X - 10 + ((vidm_current / vidm_column_size) * MODES_X_INC);
        int j = MODES_Y + ((vidm_current % vidm_column_size) * MODES_Y_INC);
        V_DrawCharacter( i, j, '*' | 0x80);  // white
    }
}

// Stay in the column.
// Alternative is to jump from column to column.
#define VIDMODE_COLUMNAR_MOVEMENT

//added:30-01-98: special menuitem key handler for video mode list
void M_VideoMode_key_handler (int key)
{
#ifdef VIDMODE_COLUMNAR_MOVEMENT       
    byte old_col, new_col;
#endif

    // Test specific key handler
    if( key_handler2(key) )  return;

#ifdef VIDMODE_COLUMNAR_MOVEMENT       
    old_col = vidm_current / vidm_column_size;
#endif

    switch( key )
    {
      case KEY_DOWNARROW:
        S_StartSound(menu_sfx_updown);
        vidm_current++;
#ifdef VIDMODE_COLUMNAR_MOVEMENT       
        new_col = vidm_current / vidm_column_size;
        if( ( vidm_current >= vidm_nummodes )
            || new_col != old_col )
        {
            // Move to top of the column
            vidm_current = old_col * vidm_column_size;
        }
#else
        if( vidm_current >= vidm_nummodes )
        {
            // Move to the first item of the mode list.
            vidm_current = 0;
        }
#endif
        break;

      case KEY_UPARROW:
        S_StartSound(menu_sfx_updown);
        vidm_current--;
#ifdef VIDMODE_COLUMNAR_MOVEMENT       
        new_col = vidm_current / vidm_column_size;
        if( ( vidm_current < 0 )
            || new_col != old_col )
        {
            // Move to bottom of the column
            vidm_current = (old_col * vidm_column_size) + vidm_column_size - 1;
        }
#else
        if( vidm_current < 0 )
        {
            // Move to the last item of the mode list.
            vidm_current = vidm_nummodes-1;
        }
#endif
        break;

      case KEY_LEFTARROW:
        S_StartSound(menu_sfx_val);
        if( (vidm_current - vidm_column_size) < 0  )  return;
        vidm_current -= vidm_column_size;
        break;

      case KEY_RIGHTARROW:
        S_StartSound(menu_sfx_val);
        if( (vidm_current + vidm_column_size) >= vidm_nummodes )  return;
        vidm_current += vidm_column_size;
        break;

      case KEY_ESCAPE:      //this one same as M_Responder
        key_handler2 = NULL;
        S_StartSound(menu_sfx_esc);
        Pop_Menu();
        break;

      default:
        break;
    }

    if( vidm_current >= vidm_nummodes )
        vidm_current = vidm_nummodes-1;
    if( vidm_current < 0 )
        vidm_current = 0;
    return;
}


//===========================================================================
//                        VIDEO MODE MENU
//===========================================================================
static void M_DrawVideoMode(void);             //added:30-01-98:

static byte  video_test_key_handler( int key );
static byte  drawmode_test_key_handler( int key );

menuitem_t VideoModeMenu[]=
{
    {IT_KEYHANDLER | IT_EXTERNAL, 0, "", M_VideoMode_key_handler, '\0'},     // dummy menuitem for the control func
};


menu_t  VideoModeDef =
{
    "M_VIDEO", // in legacy.wad
    "Video Mode",
    VideoModeMenu,      // menuitem_t ->
    M_DrawVideoMode,    // drawing routine ->
    NULL,
    sizeof(VideoModeMenu)/sizeof(menuitem_t),
    48,36,              // x,y
    0                   // lastOn
};

typedef struct
{
    modenum_t  modenum; // video mode number in format of setmodeneeded
    char    *  desc;    // XXXxYYY
} modedesc_t;

static modedesc_t   modedescs[MAXMODEDESCS];
static modenum_t    vidm_previousmode;  // modenum in format of setmodeneeded


//
// Draw the video modes list, a-la-Quake
//
static
void M_DrawVideoMode(void)
{
    modenum_t  mode_320x200 = VID_GetModeForSize( 320, 200, MODE_fullscreen );
    range_t moderange;
    modenum_t  dmode;  // draw modenum
#ifdef CONFIG_MENU_PAGE
    modenum_t  cfg_vid_mode;
#endif
    modedesc_t * mdp;  // modedesc
    modedesc_t * current_modedesc;
    const char * current_modename = "";
    int     i, row, col;
    char    *desc;

    // setup key handler for video modes
    key_handler2 = video_test_key_handler;  // key handler
   
    // draw title
    M_DrawMenuTitle();

#ifdef CONFIG_MENU_PAGE
    // Current video mode as default.
    cfg_vid_mode.modetype = vid.modenum.modetype;
    cfg_vid_mode.index = vid.modenum.index;
    menu_cfg_editing = 0;  // normal
    if( menu_cfg )
    {
        V_DrawString( 2, 1, V_WHITEMAP, menu_cfg_string[menu_cfg]);
        V_DrawString( BASEVIDWIDTH - (14*8), 1, V_WHITEMAP, "Insert Delete");
        if( (cv_scr_width.state & CS_CONFIG) != menu_cfg )
        {
            consvar_t temp_cvar2;
            int  temp_height, temp_fullscreen;

            // modify video display and key handlers
            menu_cfg_editing = 2;  // edit background, not live data

            // Get current video mode, dependent upon menu_cfg.
            // No display if no values.
            if( ! config_cvar_edit_open( & cv_scr_width ) )
                goto draw_instructions;  // cv_scr_width missing, wait for insert
            if( ! CV_Get_Pushed_cvar( &cv_scr_height, menu_cfg, /*OUT*/ &temp_cvar2 ) )
                goto draw_instructions;  // cv_scr_height missing, wait for insert

            temp_height = temp_cvar2.value;
            // use fullscreen from menu_cfg when available, else from current config
            temp_fullscreen = ( CV_Get_Pushed_cvar( &cv_fullscreen, menu_cfg, /*OUT*/ &temp_cvar2 ))?
                                temp_cvar2.value : cv_fullscreen.value;

            cfg_vid_mode = VID_GetModeForSize( temp_cvar.value, temp_height, temp_fullscreen );
        }
    }
#endif

    dmode.modetype = vid_mode_table[ cv_fullscreen.EV ];  // fullscreen or window
    vidm_nummodes = 0;
    current_modedesc = NULL;
    current_modename = NULL;
    moderange = VID_ModeRange( dmode.modetype );   // indexing
    for (i=moderange.first ; i<=moderange.last ; i++)
    {
        dmode.index = i;
        desc = VID_GetModeName (dmode);
        if (desc)
        {
            int j;

            //when a resolution exists both under VGA and VESA, keep the
            // VESA mode, which is always a higher modenum
            for (j=0 ; j<vidm_nummodes ; j++)
            {
                mdp = & modedescs[j];
                if (!strcmp (mdp->desc, desc))
                {
                    // 320x200 fullscreen is always standard VGA, not vesa
                    if (mdp->modenum.modetype != mode_320x200.modetype
                        || mdp->modenum.index != mode_320x200.index)
                    {
                        // replace previous entry (VGA)
                        mdp->modenum = dmode;
                    }
                    goto  detect_current_setting;
                }
            }

            // Create a new mode descriptor.
            mdp = & modedescs[vidm_nummodes++];
            mdp->desc = desc;
            mdp->modenum = dmode;

        detect_current_setting:
            // Detect current setting, for highlight
#ifdef CONFIG_MENU_PAGE
            if (dmode.modetype == cfg_vid_mode.modetype
                && dmode.index == cfg_vid_mode.index )
#else
            if (dmode.modetype == vid.modenum.modetype
                && dmode.index == vid.modenum.index )
#endif
            {
                current_modedesc = mdp;
                current_modename = mdp->desc;
            }

            // Must be after the detection.
            if( vidm_nummodes >= MAXMODEDESCS )  break;
        }
    }

    vidm_column_size = (vidm_nummodes+2) / 3;

    // list down col first
    col = MODES_X;
    row = MODES_Y;
    for(i=0; i<vidm_nummodes; i++)
    {
        mdp = & modedescs[i];

        V_DrawString (col, row, (mdp == current_modedesc) ? V_WHITEMAP : 0, mdp->desc);

        row += MODES_Y_INC;
        if((i % vidm_column_size) == (vidm_column_size-1))
        {
            col += MODES_X_INC;
            row = MODES_Y;
        }
    }

#ifdef CONFIG_MENU_PAGE
draw_instructions:
#endif
    draw_set_mode_instructions( 1, current_modename, modedescs[vidm_current].desc );
}


// keyboard intercept 
// Return 0= continue, 1= intercept key, 2= testing.
static
byte  video_test_key_handler( int key )
{
    set_drawmode = DRM_none;
    req_drawmode = DRM_none;  // cancel any command line setup

    if (vidm_testing_cnt>0)
    {
       // change back to the previous mode quickly
       if (key==KEY_ESCAPE)
       {
           setmodeneeded = vidm_previousmode;
           vidm_testing_cnt = 0;
       }
       return 2;
    }

#ifdef CONFIG_MENU_PAGE
    // Turn menu_cfg on and off.
    if( config_cvar_key_handler( key ) )
        goto used_key;

    if( menu_cfg_editing )
    {
        // edit video mode that is not current
        switch( key )
        {
          case KEY_ENTER:
            {
                modestat_t ms = VID_GetMode_Stat( modedescs[vidm_current].modenum );
                config_cvar_edit_setvalue( &cv_scr_width, ms.width );
                config_cvar_edit_setvalue( &cv_scr_height, ms.height );
            }
            goto used_key;
          case KEY_INS :  // insert config
            config_cvar_edit_insert( &cv_scr_width, 1 );  // using menu_cfg
            config_cvar_edit_insert( &cv_scr_height, 1 );  // using menu_cfg
            goto used_key;
         case KEY_DELETE :  // delete config
            config_cvar_edit_delete( &cv_scr_width );  // using menu_cfg
            config_cvar_edit_delete( &cv_scr_height );  // using menu_cfg
            goto used_key;
         case 'c' :
         case 'C' :
            create_initial_drawmode_config();
            goto used_key;
         default:
            break;
        }
        // block live video changes
        return 0;
    }
#endif
   
    switch( key )
    {
      case KEY_ENTER:
      case 'S':
      case 's':
        S_StartSound(menu_sfx_enter);
        req_command_video_settings = 0;  // disable command line video settings
        goto change_mode;

      case 'T':
      case 't':
        S_StartSound(menu_sfx_action);
        vidm_testing_cnt = TICRATE*5;
        goto change_mode;

      case 'D':
      case 'd':
        // current active mode becomes the default mode.
        S_StartSound(menu_sfx_action);
        SCR_SetDefaultMode ();
        req_command_video_settings = 0;  // disable command line video settings
        goto used_key;

      default:
        break;
     }
    return 0;

 change_mode:
    // Change the active video mode.
    vidm_previousmode = vid.modenum;
    if( setmodeneeded.modetype == MODE_NOP ) //in case the previous setmode was not finished
        setmodeneeded = modedescs[vidm_current].modenum;
    goto used_key;

 used_key:
    return 1;
}


//===========================================================================
//                        DRAWING OPTIONS MENU
//===========================================================================

static byte  vidm_previous_drawmode = 0;  // cv_drawmode
static byte  vidm_drawmode[MAXCOLUMNMODES+2];  // drawmode for a menu row

static void  M_Draw_drawmode(void);

menuitem_t DrawmodeMenu[]=
{
    {IT_KEYHANDLER | IT_EXTERNAL, 0, "", M_VideoMode_key_handler, '\0'},     // dummy menuitem for the control func
//    {IT_STRING | IT_CVAR, 0, "Draw Mode", &cv_drawmode      , 0},
};

menu_t  DrawmodeDef =
{
    NULL,
    "Drawmode Options",
    DrawmodeMenu, // menuitem_t ->
    M_Draw_drawmode,
    NULL,
    sizeof(DrawmodeMenu)/sizeof(menuitem_t),
    48,36,              // x,y
    0                   // lastOn
};

// keyboard intercept 
// Return 0= continue, 1= intercept key, 2= testing.
static
byte  drawmode_test_key_handler( int key )
{
    set_drawmode = DRM_none;
    req_drawmode = DRM_none;  // cancel any command line setup

    if (vidm_testing_cnt>0)
    {
       // change back to the previous mode quickly
       if (key==KEY_ESCAPE)
       {
           set_drawmode = vidm_previous_drawmode;  // drawmode for menu row
           drawmode_recalc = true;
           vidm_testing_cnt = 0;
       }
       return 2;
    }

    switch( key )
    {
      case KEY_ENTER:
      case 'S':
      case 's':
        S_StartSound(menu_sfx_enter);
        goto change_drawmode;

      case 'T':
      case 't':
        S_StartSound(menu_sfx_action);
        vidm_testing_cnt = TICRATE*5;
        goto change_drawmode;

      case 'D':
      case 'd':
        // current active mode becomes the default mode.
        S_StartSound(menu_sfx_action);
        CV_SetValue( &cv_drawmode, cv_drawmode.EV );
        goto used_key;

      case 'c' :
      case 'C' :
        create_initial_drawmode_config();
        goto used_key;

      default:
        break;
    }
    return 0;

change_drawmode:
    vidm_previous_drawmode = cv_drawmode.EV;
    if( ! rendermode_recalc ) // in case the previous setmode was not finished
    {
        // Do not change the graphics and video settings while using them for the menus.
        // Safer to change the graphics and video mode setups in between
        // frame drawing cycles.
        set_drawmode = vidm_drawmode[vidm_current];  // drawmode for menu row
        drawmode_recalc = true;
    }
    goto used_key;
   
 used_key:
    return 1;
}


static
void M_Draw_drawmode(void)
{
    int  i, row, col;

    // draw title
    M_DrawMenuTitle();

    vidm_nummodes = 0;
    vidm_column_size = MAXCOLUMNMODES;

    // list down col first
    col = MODES_X;
    row = MODES_Y;
    // step through cv_drawmode settings
    for(i=0; i<num_drawmode_sel; i++)
    {
        byte dm = drawmode_sel_t[i].value; // vid_drawmode_e
        const char * dmstr = drawmode_sel_t[i].strvalue;
        if( dmstr == NULL )  break;   // end of drawmode_sel_t
        if( drawmode_sel_avail[dm] == 0 )  continue;

        vidm_drawmode[vidm_nummodes++] = dm;  // the drawmode at this row

        // current drawmode: cv_drawmode.EV
        // default drawmode: cv_drawmode.value
        // Both have values from vid_drawmode_e.
        // whitemap the current
        V_DrawString (col, row, ((dm != cv_drawmode.EV) ? 0 : V_WHITEMAP), dmstr);

        if( vidm_nummodes > MAXCOLUMNMODES )  break;

        row += MODES_Y_INC;
        if((i % vidm_column_size) == (vidm_column_size-1))
        {
            col += MODES_X_INC;
            row = MODES_Y;
        }
    }

    byte sel_dm = vidm_drawmode[vidm_current];  // selected drawmode
    const char * sel_drawmode_str = CV_get_possiblevalue_string( drawmode_sel_t, sel_dm );
    const char * cur_drawmode_str = CV_get_possiblevalue_string( drawmode_sel_t, cv_drawmode.EV );
    draw_set_mode_instructions( 0, cur_drawmode_str, sel_drawmode_str );

    // setup key handler for video modes
    key_handler2 = drawmode_test_key_handler;  // key handler
}


#ifdef SAVEGAMEDIR
//===========================================================================
// GAME DIR MENU
//===========================================================================
static void M_DrawDir(void);

static void M_DirSelect(int choice);
static void M_Get_SaveDir(int choice);
static void M_DirEnter(int choice);

#define NUM_DIRLINE  6
menuitem_t LoadDirMenu[]=
{
    {IT_CALL | IT_NOTHING,"",0, M_DirEnter,'/'},
    {IT_CALL | IT_NOTHING,"",0, M_DirSelect,'1'},
    {IT_CALL | IT_NOTHING,"",0, M_DirSelect,'2'},
    {IT_CALL | IT_NOTHING,"",0, M_DirSelect,'3'},
    {IT_CALL | IT_NOTHING,"",0, M_DirSelect,'4'},
    {IT_CALL | IT_NOTHING,"",0, M_DirSelect,'5'},
    {IT_CALL | IT_NOTHING,"",0, M_DirSelect,'6'}
};

menu_t  DirDef =
{
//    "M_LOADG",	// LOAD GAME, really need SELECT DIR
    NULL,
    "Game Directory",
    LoadDirMenu,
    M_DrawDir,
    NULL,
    NUM_DIRLINE+1,
//    80,54,
    (176-(SAVELINELEN*8/2)), 54-LINEHEIGHT,
    0
};


// Draw the current DIR line above list
static
void draw_dir_line( int line_y )
{
    V_DrawString( DirDef.x, line_y, 0, "DIR");
    M_Draw_SaveLoadBorder( DirDef.x+32, line_y, 0);
    V_DrawString( DirDef.x+32, line_y, 0, savegamedir);
}

// Draw the dir list and DIR line
static
void M_DrawDir(void)
{
    int i;
    int line_y = DirDef.y;

    M_DrawGenericMenu();

    if (edit_enable)
    {
        // draw string and cursor in the edit position
        V_DrawString( DirDef.x, line_y, 0, "NEW DIR");
        int line_x = DirDef.x+64;
        M_Draw_SaveLoadBorder( line_x, line_y, 0);
        V_DrawString( line_x, line_y, 0, edit_buffer);
        i = V_StringWidth(edit_buffer);
        V_DrawString( line_x + i, line_y, 0, "_");
        return;
    }

    // Draw non-edit directory listing
    draw_dir_line( line_y );
    for (i = 0; i < NUM_DIRLINE; i++)
    {
        line_y += LINEHEIGHT;
        M_Draw_SaveLoadBorder( DirDef.x, line_y, 0);
        V_DrawString( DirDef.x, line_y, 0, savegamedisp[i].desc);
    }
    // Put some message in the UP-TO-LEGACY dir entry.
    // The actual dir name remains blank.
    if( scroll_index == 0 )
    {
        V_DrawString( DirDef.x, DirDef.y+LINEHEIGHT, 0, "..");
    }
}

static void M_ReadSaveStrings( int scroll_direction );

// Called from DIR game menu to select a directory
static
void M_DirSelect(int choice)
{
    // LoadDirMenu: slots 0..5 are menu 1..6
    int sgslot = choice - 1;
    if( (scroll_index == 0 && choice == 1) // UP-TO-LEGACY dir
        || ( savegamedisp[sgslot].desc[0] != '\0' ) )  // existing dir
    {
        // Existing directory selected
        strcpy( savegamedir, savegamedisp[sgslot].desc );
        scroll_index = 0; // start at top of directory
//        DirDef.prevMenu->lastOn = 1;
        menustack[menucnt-1]->lastOn = 1;
        Pop_Menu();
    }
    else if( slotindex )
    {
        // empty entry (other than UP entry), then make new directory
        M_DirEnter(0);  // calls back here, M_DirSelect( 1 )
    }
}

// Callback after editing new directory name, setup by M_DirEnter
static
void M_NewDir( void )
{
    char dirname[256];
   
    // normal savegame select, set savegamedir
    M_DirSelect( 1 ); // LoadDirMenu: slot=0 is menu 1
    if( savegamedir[0] )
    {
        // make new directory
        snprintf( dirname, 255, "%s%s", legacyhome, savegamedir );
        dirname[255] = '\0';
        I_mkdir( dirname, 0700 ); // octal permissions
    }
}


// Called from DIR game menu to select a directory
static
void M_DirEnter(int choice)
{
    slotindex = 0; // edit
    // initiate edit of dir string, we are going to be intercepting all chars
    edit_enable = 1;
    edit_buffer[0] = '\0';
    edit_index = 0;
    edit_done_callback = M_NewDir;
    // when done editing, will goto M_NewDir
}

static void M_Dir_scroll (int amount);

// Called from DIR game menu to delete a directory
static
void M_Dir_delete (int ch)
{
    if( ch=='y' && savegamedisp[slotindex].desc[0] )
    {
        char dirname[256];
        // if is current directory
        if( strcmp( savegamedir, savegamedisp[slotindex].desc ) == 0 )
        {
            savegamedir[0] = '\0';
        }
        // remove directory
        snprintf( dirname, 255, "%s%s", legacyhome, savegamedisp[slotindex].desc );
        dirname[255] = '\0';
        remove( dirname );
        savegamedisp[slotindex].desc[0] = '\0';
    }
    // fixup after the message undo, which does not record callbacks
    M_StopMessage(0);
    scroll_callback = M_Dir_scroll;
    delete_callback = M_Dir_delete;
}


// [smite] MinGW compatibility
#ifndef WIN32
#define USE_FTW 
#endif

#ifdef USE_FTW
#include <ftw.h>
// Callback from ftw system call
static
int  ftw_directory_entry( const char *file, const struct stat * sb, int flag )
{
    if( flag == FTW_D )  // only want directories
    {
        if( slotindex >= 0 )  // because of dir list scrolling
        {
            // Only want the name after legacyhome
            dl_strncpy( savegamedisp[slotindex].desc, &file[legacyhome_len], SAVESTRINGSIZE );
        }
        slotindex++;
    }
    if( slotindex >= NUM_DIRLINE )  return 1;  // done, stop ftw
    return 0;
}

#else

#include <sys/types.h>
#include <dirent.h>

#ifndef _DIRENT_HAVE_D_TYPE
#include <sys/stat.h>
#include "m_misc.h"
#endif

#endif

// Get directories into savegamedisp, starting at skip_count.
static
void  get_directory_entries( int skip_count )
{
#ifdef USE_FTW
    // Use ftw
    slotindex = -skip_count;
    ftw( legacyhome, ftw_directory_entry, 1 );
#else
    // Use dirent

#ifndef _DIRENT_HAVE_D_TYPE
    // Have to use stat to identify directories.
    char dentfile[MAX_WADPATH];
    struct stat dentstat;
#endif
   
    struct dirent * dent;
    DIR * legdir;

    legdir = opendir( legacyhome );
    if( legdir == NULL )  return;

    slotindex = -skip_count;
    for (;;)
    {
        dent = readdir( legdir );  // Read directory entry
        if( dent == NULL )  break;
        // Ignore the self reference.
        if( strcmp( dent->d_name, "." ) == 0 )   continue;
#ifdef _DIRENT_HAVE_D_TYPE
        // Unix systems have the D_TYPE, but others are unlikely.
        if( dent->d_type != DT_DIR )  continue;  // Only want directories
#else
        // Get status to check if is a directory.
        cat_filename( dentfile, legacyhome, dent->d_name );       
        stat( dentfile, &dentstat );
        if( ! S_ISDIR( dentstat.st_mode ))  continue;  // Only want directories
#endif

        if( slotindex >= 0 )  // because of dir list scrolling
        {
            // Only want the name after legacyhome
            dl_strncpy( savegamedisp[slotindex].desc, dent->d_name, SAVESTRINGSIZE );
            // The up-dir is passed as an empty string.
            if( strcmp( savegamedisp[slotindex].desc, ".." ) == 0 )
                savegamedisp[slotindex].desc[0] = 0;
        }
        slotindex++;
        if( slotindex >= NUM_DIRLINE )  break;  // full
    }
    closedir( legdir );
#endif
}


static
void M_Dir_scroll (int amount)
{
    // Do not scroll if at end of list
    if( (amount > 0) && ( savegamedisp[SAVEGAME_NUM_MSLOT-1].desc[0] == '\0' ))
        return;  // at end of dir list

    clear_remaining_savegamedisp( 0 );
    // countdown reading dir entries
    scroll_index += amount;
    if( scroll_index < 0 )   scroll_index = 0;
    get_directory_entries( scroll_index );
}

// Called from menu
static
void M_Get_SaveDir (int choice)
{
    // Any mode, directory is personal choice
    // Directory menu with choices
    Push_Setup_Menu(&DirDef);
    scroll_callback = M_Dir_scroll;
    delete_callback = M_Dir_delete;

    clear_remaining_savegamedisp( 0 );
    scroll_index = 0;  // start at top of dir list
    get_directory_entries( 0 );
}
   
#endif

//===========================================================================
//LOAD GAME MENU
//===========================================================================
static void M_Draw_Loadgame(void);

static void M_LoadSelect(int choice);

// SAVEGAME_NUM_MSLOT dependent
#ifdef SAVEGAMEDIR
#define SAVEGAME_MSLOT_0      1
#define SAVEGAME_MSLOT_LAST   6
#else
#define SAVEGAME_MSLOT_0      0
#define SAVEGAME_MSLOT_LAST   5
#endif


// Has SAVEGAME_NUM_MSLOT entries, and dir entry
menuitem_t LoadgameMenu[]=
{
#ifdef SAVEGAMEDIR
    {IT_CALL | IT_NOTHING,"",0, M_Get_SaveDir,'/'},
#endif   
    {IT_CALL | IT_NOTHING,"",0, M_LoadSelect,'1'},
    {IT_CALL | IT_NOTHING,"",0, M_LoadSelect,'2'},
    {IT_CALL | IT_NOTHING,"",0, M_LoadSelect,'3'},
    {IT_CALL | IT_NOTHING,"",0, M_LoadSelect,'4'},
    {IT_CALL | IT_NOTHING,"",0, M_LoadSelect,'5'},
    {IT_CALL | IT_NOTHING,"",0, M_LoadSelect,'6'}
};

menu_t  LoadDef =
{
    "M_LOADG",
    "Load Game",
    LoadgameMenu,
    M_Draw_Loadgame,
    NULL,
    sizeof(LoadgameMenu)/sizeof(menuitem_t),
//    80,54,
#ifdef SAVEGAMEDIR
    (176-(SAVELINELEN*8/2)),54-LINEHEIGHT,
#else
    (176-(SAVELINELEN*8/2)),54,
#endif   
    0
};

//
// M_Loadgame & Cie.
//
static
void M_Draw_Loadgame(void)
{
    int i;
    int line_y = LoadDef.y;

    M_DrawGenericMenu();

#ifdef SAVEGAMEDIR
    draw_dir_line( line_y );
    for (i = 0; i < SAVEGAME_NUM_MSLOT; i++)
    {
        line_y += LINEHEIGHT;
        M_Draw_SaveLoadBorder( LoadDef.x, line_y, 1);
#ifdef SAVEGAME_MTLEFT
        V_DrawString( LoadDef.x, line_y, 0, savegamedisp[i].levtime);
        V_DrawString( LoadDef.x+SAVE_DESC_XPOS, line_y, 0, savegamedisp[i].desc);
#else
        V_DrawString( LoadDef.x, line_y, 0, savegamedisp[i].desc);
        V_DrawString( LoadDef.x+(SAVE_MT_POS*8), line_y, 0, savegamedisp[i].levtime);
#endif
    }
#else
    for (i = 0; i < SAVEGAME_NUM_MSLOT; i++)
    {
        M_Draw_SaveLoadBorder( LoadDef.x, line_y, 0);
#ifdef SAVEGAME_MTLEFT
        V_DrawString( LoadDef.x, line_y, 0, savegamedisp[i].levtime);
        V_DrawString( LoadDef.x+SAVE_DESC_XPOS, line_y, 0, savegamedisp[i].desc);
#else
        V_DrawString( LoadDef.x, line_y, 0, savegamedisp[i].desc);
        V_DrawString( LoadDef.x+(SAVE_MT_POS*8), line_y, 0, savegamedisp[i].levtime);
#endif
        line_y += LINEHEIGHT;
    }
#endif
}

//
// User wants to load this game
//
// Called from load game menu to load selected save game
static
void M_LoadSelect(int choice)
{
    // Issue command to save game
    // SAVEGAMEDIR: slots 0..5 are menu 1..6
    short sgslot = choice - SAVEGAME_MSLOT_0;
#ifdef SAVEGAME99
    if( savegamedisp[sgslot].savegameid <= 99 )
      G_Load_Game ( savegamedisp[sgslot].savegameid );  // slot id
#else
    G_Load_Game (sgslot);
#endif
    M_Clear_Menus (true);
}


//
// M_ReadSaveStrings
//  read the strings from the savegame files
//  and put it in savegame global variable
//
#ifdef SAVEGAME99
static
void M_ReadSaveStrings( int scroll_direction )
{
    // [WDJ] saves considerable size and hassle having this test here
    boolean skip_unloadable = (currentMenu != &SaveDef);
    int     sgslot, nameid, slot_status, i;
    int     first_nameid = 0;
    int     last_nameid = 0;  // disable unless searching
    int     handle;
    char  * slot_str;
    savegame_disp_t *sgdp;
    savegame_info_t  sginfo;
    char    name[256];

    P_Alloc_savebuffer( 0 );  // header only
    // savegamedisp is statically alloc

    if( scroll_direction < 0 )
    {
        // Because unused slots are skipped, cannot predict what id will be
        // at top when paging backwards, so must start from 0 and read
        // forward (while scrolling) until the last_nameid test is statisfied.
        // The top of previous display will be the last item in next display.
        if((scroll_index > 0) && (savegamedisp[0].savegameid <= 99))
           last_nameid = savegamedisp[0].savegameid;
        scroll_index = first_nameid = 0;
    }
    else if( scroll_direction > 0 )
    {
        // The bottom of previous display becomes top of next display.
        if( savegamedisp[SAVEGAME_NUM_MSLOT-1].savegameid <= 99 )
           first_nameid = savegamedisp[SAVEGAME_NUM_MSLOT-1].savegameid;
        else if( savegamedisp[0].savegameid <= 99 )
           first_nameid = savegamedisp[0].savegameid;
        scroll_index = first_nameid;
    }
    else
    {
        // no scroll
        if( scroll_index >= 0 && scroll_index <= 99 )
        {
            // redisplay from last usage
            first_nameid = scroll_index;
        }
    }

    // read savegame headers into savegame slots 0..5
    sgslot = 0;
    for (nameid = first_nameid; nameid <= 99; nameid++)
    {
        if( sgslot >= SAVEGAME_NUM_MSLOT ) break;
        sgdp = &savegamedisp[sgslot];
        sgdp->levtime[0] = '\0';

        G_Savegame_Name( name, nameid );

        handle = open (name, O_RDONLY | 0, 0666);
        if (handle == -1)
        {
            // read error
            if( skip_unloadable )  continue;
            slot_str = text[EMPTYSTRING_NUM];
            sprintf( &sgdp->levtime[0], "%2i", nameid );
            slot_status = IT_SPACE | IT_NOTHING;
        }
        else
        {
            // read the savegame header and react
            read( handle, savebuffer, savebuffer_size );
            close (handle);
            if( P_Savegame_Read_header( &sginfo, 0 ) )
            {
                if( sginfo.map == NULL ) sginfo.map = " -  ";
                if( sginfo.levtime == NULL ) sginfo.levtime = "";
                // info from a valid legacy save game
                snprintf( &sgdp->levtime[0], SAVEGAME_MTLEN-1,
                          "%s %s", sginfo.map, sginfo.levtime);
                sgdp->levtime[SAVEGAME_MTLEN-1] = '\0';  // term snprintf
                slot_str = sginfo.name;
                slot_status = IT_CALL | IT_NOTHING | 1;
            }
            else
            {
                // bad header, not a valid legacy savegame, or an old one
                if( skip_unloadable )  continue;
                slot_str = sginfo.msg;	// error message
                slot_status = IT_SPACE | IT_NOTHING;
            }
        }
        sgdp->levtime[SAVEGAME_MTLEN-1] = '\0';  // safe
        // fill in savegame strings for menu display
        dl_strncpy( &sgdp->desc[0], slot_str, SAVESTRINGSIZE );
        sgdp->savegameid = nameid;
        LoadgameMenu[sgslot + SAVEGAME_MSLOT_0].status = slot_status;
        sgslot++; // uses savegamedisp[0..5]
        // When scroll_direction < 0 only, until last test satisfied
        if((sgslot >= SAVEGAME_NUM_MSLOT) && (nameid < last_nameid))
        {
            // Display is full and still searching for last_nameid.
            // Scroll them to make room at last slot for next read

            // [WDJ] Fix bug: upon scroll down and up, EMPTY SLOT message remained
            // Must scroll the status too.
            for( i = SAVEGAME_MSLOT_0; i<SAVEGAME_MSLOT_LAST; i++ )
               LoadgameMenu[i].status = LoadgameMenu[i+1].status;
            LoadgameMenu[SAVEGAME_MSLOT_LAST].status = IT_SPACE | IT_NOTHING;

            memmove( &savegamedisp[0], &savegamedisp[1],
                         sizeof( savegame_disp_t ) * (SAVEGAME_NUM_MSLOT-1));
            sgslot --;  // read one more, come back here
            scroll_index = savegamedisp[0].savegameid;
        }
    }
    free( savebuffer );

    clear_remaining_savegamedisp( sgslot );  // if sgslot < 5, upto [5]
    // clear remaining menu slot status, to prevent attempts to load
    for( i = sgslot+SAVEGAME_MSLOT_0; i<=SAVEGAME_MSLOT_LAST; i++ )
       LoadgameMenu[i].status = IT_SPACE | IT_NOTHING;
}

#else
static
void M_ReadSaveStrings(void)
{
    int     handle;
    int     i;
    savegame_disp_t *sgdp;
    savegame_info_t  sginfo;
    char    name[256];

    P_Alloc_savebuffer( 0 );  // header only
    // savegamedisp is statically alloc

    for (i = 0; i < SAVEGAME_NUM_MSLOT; i++)
    {
        sgdp = &savegamedisp[i];
        sgdp->levtime[0] = '\0';

        G_Savegame_Name( name, i );

        handle = open (name, O_RDONLY | 0, 0666);
        if (handle == -1)
        {
            // read error
            strcpy(&sgdp->desc[0], text[EMPTYSTRING_NUM]);
            LoadgameMenu[i].status = IT_SPACE | IT_NOTHING;
            continue;
        }
        read( handle, savebuffer, savebuffer_size );
        close (handle);
        if( P_Read_Savegame_Header( &sginfo ) )
        {
            // info from a valid legacy save game
            dl_strncpy( &sgdp->desc[0], sginfo.name, SAVESTRINGSIZE );
            if( sginfo.map == NULL ) sginfo.map = " -  ";
            if( sginfo.levtime == NULL ) sginfo.levtime = "";
            snprintf( &sgdp->levtime[0], SAVEGAME_MTLEN,
                      "%s %s", sginfo.map, sginfo.levtime);
            sgdp->levtime[SAVEGAME_MTLEN-1] = '\0';
            LoadgameMenu[i].status = IT_CALL | IT_NOTHING | 1;
        }
        else
        {
            dl_strncpy( &sgdp->desc[0], sginfo.msg, SAVESTRINGSIZE );
            LoadgameMenu[i].status = IT_SPACE | IT_NOTHING;
        }
    }
    free( savebuffer );
}
#endif


#ifdef SAVEGAME99
// scroll_callback
static
void M_Savegame_scroll (int amount)
{
    M_ReadSaveStrings( amount ); // skip unloadable
}

//void M_Save_scroll (int amount);

// delete_callback
static
void M_Savegame_delete (int ch)
{
    if( ch=='y' && (savegamedisp[slotindex].savegameid <= 99) )
    {
        char savename[256];
        G_Savegame_Name( savename, savegamedisp[slotindex].savegameid );
        // remove savegame
        remove( savename );
        savegamedisp[slotindex].desc[0] = '\0';
        savegamedisp[slotindex].levtime[0] = '\0';
        savegamedisp[slotindex].savegameid = 255;
        // slot no longer loadable
        LoadgameMenu[slotindex + SAVEGAME_MSLOT_0].status = IT_SPACE | IT_NOTHING;
    }
    // fixup after the message undo, which does not record callbacks
    M_StopMessage(0);
    scroll_callback = M_Savegame_scroll;
    delete_callback = M_Savegame_delete;
}
#endif

// Called from pop menu, M_Loadgame, M_Savegame
static
void attach_savegame_menu( void )
{
#ifdef SAVEGAME99
    scroll_callback = M_Savegame_scroll;
    delete_callback = M_Savegame_delete;
    M_ReadSaveStrings( 0 ); // show unloadable
#else
    M_ReadSaveStrings();
#endif
}


//
// Selected from DOOM menu
//
// Called from menu (0 to 5), and key F3 (0)
static
void M_Loadgame (int choice)
{
// change can't load message to can't load in server mode
    if (netgame && !server)
    {
        // running network game and am not the server, cannot load
        M_SimpleMessage( text[LOADNET_NUM] );
        return;
    }

    // Load game menu with slot choices
    Push_Setup_Menu(&LoadDef);
    attach_savegame_menu();
}


//===========================================================================
//                                SAVE GAME MENU
//===========================================================================
static void M_Draw_Savegame(void);

static void M_SaveSelect(int choice);

// Must have SAVEGAME_NUM_MSLOT entries, plus dir
menuitem_t SavegameMenu[]=
{
#ifdef SAVEGAMEDIR
    {IT_CALL | IT_NOTHING,"",0, M_Get_SaveDir,'/'},
#endif   
    {IT_CALL | IT_NOTHING,"",0, M_SaveSelect,'1'},
    {IT_CALL | IT_NOTHING,"",0, M_SaveSelect,'2'},
    {IT_CALL | IT_NOTHING,"",0, M_SaveSelect,'3'},
    {IT_CALL | IT_NOTHING,"",0, M_SaveSelect,'4'},
    {IT_CALL | IT_NOTHING,"",0, M_SaveSelect,'5'},
    {IT_CALL | IT_NOTHING,"",0, M_SaveSelect,'6'}
};

menu_t  SaveDef =
{
    "M_SAVEG",
    "Save Game",
    SavegameMenu,
    M_Draw_Savegame,
    NULL,
    sizeof(SavegameMenu)/sizeof(menuitem_t),
//    80,54,
#ifdef SAVEGAMEDIR
    (176-(SAVELINELEN*8/2)),54-LINEHEIGHT,
#else
    (176-(SAVELINELEN*8/2)),54,
#endif   
    0
};

//
// Draw border for the savegame description
//
static
void M_Draw_SaveLoadBorder(int x, int y, boolean longer )
{
    int i;

    // Draw to screen0, scaled
    if( gamemode == heretic )
    {
#ifdef SAVEGAME_MTLEFT
        V_DrawScaledPatch_Name(x-8, y-4, "M_FSLOT");
#if SAVELINELEN > 24
        if( longer )
        { 
            V_DrawScaledPatch_Name(x-8 + SAVE_DESC_XPOS, y-4, "M_FSLOT");
        }
#endif
#else
#if SAVELINELEN > 24
        if( longer )
        {
            V_DrawScaledPatch_Name(x-8 + ((SAVELINELEN-24)*8), y-4, "M_FSLOT");
        }
#endif
        V_DrawScaledPatch_Name(x-8, y-4, "M_FSLOT");
#endif
    }
    else
    {
        V_DrawScaledPatch_Name (x-8,y+7, "M_LSLEFT");
        
        for (i = (longer?SAVELINELEN:SAVESTRINGSIZE); i>0; i--)
        {
            V_DrawScaledPatch_Name (x,y+7, "M_LSCNTR");
            x += 8;
        }
        
        V_DrawScaledPatch_Name (x,y+7, "M_LSRGHT");
    }
}


//
//  M_Savegame & Cie.
//
static
void M_Draw_Savegame(void)
{
    int line_y = LoadDef.y;

    if (edit_enable)
    {
//        M_Draw_Loadgame();	// optional, keep other slots displayed
        // draw string and cursor in the original slot position
#ifdef SAVEGAMEDIR
        line_y = LoadDef.y+(LINEHEIGHT*slotindex)+LINEHEIGHT; // dir is 0
#else
        line_y = LoadDef.y+LINEHEIGHT*slotindex;
#endif
        M_Draw_SaveLoadBorder( LoadDef.x, line_y, 1);
#ifdef SAVEGAME_MTLEFT
        V_DrawString( LoadDef.x, line_y, 0, "DESCRIPTION:");
        V_DrawString( LoadDef.x+SAVE_DESC_XPOS, line_y, 0, edit_buffer);
        int i = V_StringWidth(edit_buffer);
        V_DrawString( LoadDef.x+SAVE_DESC_XPOS + i, line_y, 0, "_");
#else
        V_DrawString( LoadDef.x, line_y, 0, edit_buffer);
        int i = V_StringWidth(edit_buffer);
        V_DrawString( LoadDef.x + i, line_y, 0, "_");
#endif
    }
    else
    {
        M_Draw_Loadgame();
    }
}

//
// M_Responder calls this when user is finished
//
// Called from save menu by M_Responder,
// and from quick Save by M_QuickSaveResponse
#if defined SAVEGAMEDIR || defined SAVEGAME99
// slti = savegame index 0..5, or quicksave 6
static
void M_DoSavegame(int slti)
{
    if( savegamedisp[slti].savegameid > 99 )
        return;
    // Issue command to save game
    G_Save_Game (savegamedisp[slti].savegameid, savegamedisp[slti].desc);
    M_Clear_Menus (true);

    // PICK QUICKSAVE SLOT YET?
    if (quicksave_slotid == -2)
    {
        quicksave_slotid = savegamedisp[slti].savegameid;  // 0..99
        savegamedisp[QUICKSAVE_INDEX] = savegamedisp[slti];  // save whole thing
    }
}
#else
// slot = game id and menu index 0..5
static
void M_DoSavegame(int slot)
{
    // Issue command to save game
    G_Save_Game (slot, savegamedisp[slot].desc);
    M_Clear_Menus (true);

    // PICK QUICKSAVE SLOT YET?
    if (quicksave_slotid == -2)
    {
        quicksave_slotid = slot;
    }
}
#endif

// Called when desc editing is done
static
void M_SaveEditDone( void )
{
    M_DoSavegame(slotindex);  // index 0..5
}

//
// User wants to save. Start string input for M_Responder
//
// Called from save game menu to select save game
static
void M_SaveSelect(int choice)
{
#ifdef SAVEGAMEDIR   
    slotindex = choice - SAVEGAME_MSLOT_0; // menu 1..6 -> index 0..5
#else   
    slotindex = choice;  // line being edited  0..5
#endif
    if( savegamedisp[slotindex].savegameid > 99 )
        return;
    // clear out EMPTY STRING and other err msgs
    // LoadSaveStrings puts existing status in LoadgameMenu, MSLOT index
    if ( (LoadgameMenu[choice].status & 1) != 1 )  // invalid name
        savegamedisp[slotindex].desc[0] = 0;
    // [WDJ] edit_enable overwrites entire line
    // initiate edit of desc string, we are going to be intercepting all chars
    strcpy(edit_buffer, savegamedisp[slotindex].desc);
    edit_index = strlen(edit_buffer);
    edit_done_callback = M_SaveEditDone;
    edit_enable = 1;
}


//
// Selected from DOOM menu
//
// Called from menu (0 to 5), key F2 (0), and quicksave (-2)
static
void M_Savegame (int choice)
{
    if(demorecording)
    {
        M_SimpleMessage("You cannot save while recording demos\n\nPress a key\n");
        return;
    }

    if (demoplayback || demorecording)
    {
        M_SimpleMessage( text[SAVEDEAD_NUM] );
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    if (netgame && !server)
    {
        M_SimpleMessage("You are not the server");
        return;
    }

    // Save game menu with slot choices
    Push_Setup_Menu(&SaveDef);
    attach_savegame_menu();
}

//===========================================================================
//                            QuickSAVE & QuickLOAD
//===========================================================================

//
// M_QuickSave
//

// Handles quick save ack from M_QuickSave, and initiates the save.
static
void M_QuickSaveResponse(int ch)
{
    if (ch == 'y')
    {
        M_DoSavegame( QUICKSAVE_INDEX ); // initiate game save, network message
        S_StartSound(menu_sfx_action);
    }
    else
    {
        // response was "No"
        // Give opportunity to pick a new slot
        quicksave_slotid = -2;     // means to pick a slot now
        M_Savegame( -2 );
    }
}

// Invoked by key F6
static
void M_QuickSave(void)
{
    if (demoplayback || demorecording)
    {
        S_StartSound(sfx_oof);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    if (quicksave_slotid < 0)   goto pick_slot; // No slot yet.
    M_QuickSaveResponse('y');   // [Arcade] save immediately, no confirmation
    return;

pick_slot:   
    // have not selected a quick save slot yet
    M_StartControlPanel();
    quicksave_slotid = -2;     // signal to save as a quicksave slot
    M_Savegame( -2 );
    return;
}

//
// M_QuickLoad
//
// Handles quick load ack from M_QuickLoad, and initiates the save.
static
void M_QuickLoadResponse(int ch)
{
    if (ch == 'y')
    {
        // quicksave_slotid is known valid, slot id
        G_Load_Game( quicksave_slotid ); // initiate game load, network message
        M_Clear_Menus (true);
        S_StartSound(menu_sfx_action);
    }
}


static
void M_QuickLoad(void)
{
    if (netgame)
    {
        M_SimpleMessage( text[QLOADNET_NUM] );
        return;
    }

    if (quicksave_slotid < 0)
    {
        // No save slot selected
        M_SimpleMessage( text[QSAVESPOT_NUM] );
        return;
    }
    M_QuickLoadResponse('y');   // [Arcade] load immediately, no confirmation
}


//===========================================================================
//                                 END GAME
//===========================================================================

//
// M_EndGame
//
static
void M_EndGameResponse(int ch)
{
    if (ch != 'y')
        return;

    currentMenu->lastOn = itemOn;
    M_Clear_Menus (true);

    // [Arcade] A level pack overrides the IWAD maps, so the attract screen's
    // built-in demos would play back against the wrong levels.  Restart for
    // a clean attract screen, as the idle timeout does.
    if( M_LevelPack_Loaded() )
        M_Restart_Program( NULL, false );   // no return

    COM_BufAddText("exitgame\n");
}

static
void M_EndGame(int choice)
{
    choice = 0;
    if (demoplayback || demorecording)
    {
        S_StartSound(sfx_oof);
        return;
    }
/*
    if (netgame)
    {
        M_SimpleMessage( text[NETEND_NUM] );
        return;
    }
*/
    M_EndGameResponse('y');   // [Arcade] end immediately, no confirmation
}

//===========================================================================
//                                 Quit Game
//===========================================================================

//
// M_QuitDOOM
//
int     quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

int     quitsounds2[8] =
{
    sfx_vilact,
    sfx_getpow,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};


// Called from port drivers.
void M_QuitResponse(int ch)
{
    tic_t   dlyd_time;
    if (ch != 'y')
        return;

    if (!netgame)
    {
#ifdef USE_QUITSOUNDS2
        //added:12-02-98: quitsounds are much more fun than quisounds2
        if (gamemode == doom2_commercial)
            S_StartSound(quitsounds2[(gametic>>2)&7]);
        else
#endif
            S_StartSound(quitsounds[(gametic>>2)&7]);

        //added:12-02-98: do that instead of I_WaitVbl which does not work
        if(!nosoundfx)
        {
            dlyd_time = I_GetTime() + TICRATE*2;
            while (dlyd_time > I_GetTime()) ;
        }
    }
    I_Quit();  // No return
}

static
void M_QuitDOOM(int choice)
{
  // Arcade cabinet: quit immediately, skip the Y/N confirmation prompt.
  M_QuitResponse('y');
}


//===========================================================================
//                              Some Draw routine
//===========================================================================

//
//      Menu Functions
//
static
void M_DrawThermo ( int   x,
                    int   y,
                    consvar_t *cv)
{
    int xx,i, cursory;
    int leftlump,rightlump,centerlump[2],cursorlump;

    // Draw to screen0, scaled
    xx = x;
    if( EN_heretic_hexen )
    {
        xx -= 32-8;
        leftlump      = W_GetNumForName("M_SLDLT");
        rightlump     = W_GetNumForName("M_SLDRT"); 
        centerlump[0] = W_GetNumForName("M_SLDMD1"); 
        centerlump[1] = W_GetNumForName("M_SLDMD2"); 
        cursorlump    = W_GetNumForName("M_SLDKB");  
        cursory = y+7;
    }
    else
    {
        leftlump      = W_GetNumForName("M_THERML");
        rightlump     = W_GetNumForName("M_THERMR"); 
        centerlump[0] = W_GetNumForName("M_THERMM"); 
        centerlump[1] = W_GetNumForName("M_THERMM"); 
        cursorlump    = W_GetNumForName("M_THERMO");  
        cursory = y;
    }
    { // temp use of left thermo patch
      patch_t *pt = W_CachePatchNum(leftlump,PU_CACHE);  // endian fix
      V_DrawScaledPatch (xx,y,pt);
      xx += pt->width - pt->leftoffset;  // add width to offset
    }
    for (i=0;i<16;i++)
    {
        // Has alternate center patches for heretic, hexen.
        V_DrawScaledPatch_Num (xx,y, centerlump[i & 1] );
        xx += 8;
    }
    V_DrawScaledPatch_Num (xx,y, rightlump );

    xx = (cv->value - cv->PossibleValue[0].value) * (15*8) /
         (cv->PossibleValue[1].value - cv->PossibleValue[0].value);

    V_DrawScaledPatch_Num ((x+8) + xx, cursory, cursorlump );
}


#if 0
static
void M_DrawEmptyCell( menu_t*       menu,
                      int           item )
{
    V_DrawScaledPatch_Name (menu->x - 10,  menu->y+item*LINEHEIGHT - 1,
                       "M_CELL1" );
}

static
void M_DrawSelCell ( menu_t*       menu,
                     int           item )
{
    V_DrawScaledPatch_Name (menu->x - 10,  menu->y+item*LINEHEIGHT - 1,
                       "M_CELL2" );
}
#endif


//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
//added:06-02-98:
//  x, y : position (320,200)
// Called by M_DrawGenericMenu, M_DrawSetupMultiPlayerMenu, M_DrawMessageMenu
// The caller must call V_SetupDraw with V_SCALESTART | V_SCALEPATCH,
// selecting V_CENTERHORZ or V_CENTERMENU, to have consistent positioning.
// V_CENTERMENU has a y shift, which differs from V_CENTERHORZ.
void M_DrawTextBox (int x, int y, int width, int lines)
{
    fontinfo_t * fip = V_FontInfo();
    patch_t  *p;
    int      cx, cy;
    int      n;
    int      step,boff;

    // Draw to screen0, scaled

    if( gamemode == heretic )
    {
        // humf.. border will stand if we do not adjust size ...
        x+=4;
        y+=4;
        lines = (lines+1)/2;
        width = (width+1)/2;
        step = 16;
        boff = 4; // borderoffset
    }
    else
    {
        step = fip->yinc;
        boff = 8;
    }
    if( ! viewborderlump[0] )   goto grey_bar;

    // draw left side
    cx = x;
    cy = y;
    V_DrawScaledPatch_Num (cx, cy, viewborderlump[BRDR_TL] );
    cy += boff;
   
    // temp use patch in loop
    p = W_CachePatchNum (viewborderlump[BRDR_L],PU_CACHE);  // endian fix
    for (n = 0; n < lines; n++)
    {
        V_DrawScaledPatch (cx, cy, p);
        cy += step;
    }
   
    V_DrawScaledPatch_Num (cx, cy, viewborderlump[BRDR_BL] );

    // draw background
    // Flat fill per drawinfo, centering.
    // Reduce scale of Doom background to (0..2) so text is easier to read.
    V_DrawFlatFill (x+boff, y+boff, width*step, lines*step,
                   (EN_heretic_hexen ? 15:0), st_borderflat_num);

    // draw top and bottom
    cx += boff;
    cy = y;
    while (width > 0)
    {
        V_DrawScaledPatch_Num (cx, cy, viewborderlump[BRDR_T] );

        V_DrawScaledPatch_Num (cx, y+boff+lines*step, viewborderlump[BRDR_B] );
        width --;
        cx += step;
    }

    // draw right side
    cy = y;
    V_DrawScaledPatch_Num (cx, cy, viewborderlump[BRDR_TR] );
    cy += boff;
   
    // temp use patch in loop
    p = W_CachePatchNum (viewborderlump[BRDR_R],PU_CACHE);  // endian fix
    for (n = 0; n < lines; n++)
    {
        V_DrawScaledPatch (cx, cy, p);
        cy += step;
    }
   
    V_DrawScaledPatch_Num (cx, cy, viewborderlump[BRDR_BR] );
    

  done:   
    return;

  grey_bar:
    // Message box centers string, using 8 bit width assumption.
    // For now, correct the position.
//    V_DrawFill(x,y, width*fip->xinc, lines*fip->yinc, ci_grey );
    V_DrawFill(x/8, y, width*fip->xinc, lines*fip->yinc, ci_grey );
    goto done;
}

//==========================================================================
//                        Message is now a (hackable) Menu
//==========================================================================
static void M_DrawMessageMenu(void);
static void M_SetupMenu(menu_t *menudef);

menuitem_t MessageMenu[]=
{
    // TO HACK
    {0 ,NULL , 0, NULL ,0}
};

menu_t MessageDef =
{
    NULL,               // title
    NULL,
    MessageMenu,        // menuitem_t ->
    M_DrawMessageMenu,  // drawing routine ->
    NULL,
    sizeof(MessageMenu)/sizeof(menuitem_t),
    0,0,                // x,y                 (TO HACK)
    0                   // lastOn, flags       (TO HACK)
};

static menu_t * message_menu_back;
static byte message_lines, message_length;


void M_StartMessage ( const char*       string,
                      void*             routine,
                      menumessagetype_t itemtype )
{
    int   maxlen, i, lines;
    char * chp;

#define msgline  MessageDef.menuitems[0]   
#define msgtext  msgline.text
    msgtmp[MSGTMP_LEN] = '\0';  // make sure it is terminated
    if( msgtext )
        Z_Free( msgtext );
    msgtext = Z_StrDup(string);
    DEBFILE(msgtext);

    M_StartControlPanel(); // can't put menuactiv to true
    msgline.text     = msgtext;
    msgline.alphaKey = itemtype;
    switch(itemtype) {
        case MM_NOTHING:
             msgline.status     = IT_MSGHANDLER;
             msgline.itemaction = M_StopMessage;
             break;
        case MM_YESNO:
             msgline.status     = IT_MSGHANDLER;
             msgline.itemaction = routine;
             break;
        case MM_EVENTHANDLER:
             msgline.status     = IT_MSGHANDLER;
             msgline.itemaction = routine;
             break;
    }
    //added:06-02-98: now draw a textbox around the message
    // compute length max and the numbers of lines
    maxlen = 4; // minimum box
    chp = msgtext;
    for (lines=0;  ; lines++)
    {
        for (i = 0;  ; i++)
        {
            if (*chp == 0)   // end of line escape
                break;

            if (*chp == '\n')
            {
                chp ++;
                break;
            }
            chp ++;
        }
        if (i > maxlen)
            maxlen = i;
        if (*chp == 0)   // end of line counts
            break;
    }
    if((i > 0) || (lines==0))  // missing \n or empty string
        lines++;  // count as a line

    MessageDef.x=(BASEVIDWIDTH - (8*maxlen) - 16)/2;
    MessageDef.y=(BASEVIDHEIGHT - M_StringHeight(msgtext))/2;

    // [WDJ] lastOn is not large enough for these parameters, nor appropriate.
    message_lines = lines;
    message_length = maxlen;
    message_menu_back = currentMenu;

    currentMenu = &MessageDef;
    itemOn=0;
}

void M_SimpleMessage ( const char * string )
{
    M_StartMessage ( string, NULL, MM_NOTHING );
}


#define MAXMSGLINELEN 256

// Called by M_Drawer.
static
void M_DrawMessageMenu(void)
{
    int    y;
    short  i,max;
    char   string[MAXMSGLINELEN];
    int    start,lines;
    char   *msg=currentMenu->menuitems[0].text;

    // Draw to screen0, scaled
    // Not part of a menu.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );
   
    // Draw to screen0, scaled
    y=currentMenu->y;
    start = 0;
    lines = message_lines;
    max = ((int)message_length) * 8;
    M_DrawTextBox( currentMenu->x, y-8, (max+7)>>3,lines);

    while(*(msg+start))
    {
        for (i = 0; i < (int)strlen(msg+start); i++)
        {
            if (*(msg+start+i) == '\n')
            {
                memset(string,0,MAXMSGLINELEN);
                if(i >= MAXMSGLINELEN)
                {
                    CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
                    return;
                }
                else
                {
                    strncpy(string,msg+start,i);
                    start += i+1;
                    i = -1; //added:07-02-98:damned!
                }
                
                break;
            }
        }

        if (i == (int)strlen(msg+start))
        {
            if(i >= MAXMSGLINELEN)
            {
                CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
                return;
            }
            else
            {
                strcpy(string,msg+start);
                start += i;
            }
        }

        V_DrawString((BASEVIDWIDTH - V_StringWidth(string))/2,y,0,string);
        y += 8; //hu_font[0]->height;
    }
}

// default message handler
static
void M_StopMessage(int choice)
{
    // Do not interfere with response menu changes
    if( (currentMenu == &MessageDef) && message_menu_back )
    {
         M_SetupMenu(message_menu_back); // NULLS callbacks, caller must fix
         S_StartSound(menu_sfx_action);
         message_menu_back = NULL;
    }
}

//==========================================================================
//                        Menu stuffs
//==========================================================================

//added:30-01-98:
//
//  Write a string centered using the hu_font
//
static
void M_CentreText (int y, char* string)
{
    int x;
    //added:02-02-98:centre on 320, because default option is SCALED
    x = (BASEVIDWIDTH - V_StringWidth(string))>>1;
    V_DrawString(x,y,0,string);
}


//
// CONTROL PANEL
//

// Used for inc/dec screen size.
static
void  cvar_incdec( consvar_t * cv, int incr )
{
#ifdef CONFIG_MENU_PAGE
    cv = config_cvar_edit_open( cv );
    if( ! cv )
        return;  // cannot edit
#endif

    CV_SetValue( cv, cv->value + incr );
#ifdef CONFIG_MENU_PAGE
    config_cvar_edit_save();
#endif

    S_StartSound(menu_sfx_enter);
}

// For delayed effective CV change.
static consvar_t * menu_delay_cv = NULL;
static byte  menu_delay_ticks = 0;

static
void M_Change_cvar_value(int choice)
{
    consvar_t * cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
    int d = choice * 2 - 1;  // -1 or +1

#ifdef CONFIG_MENU_PAGE
    cv = config_cvar_edit_open( cv );
    if( ! cv )
        return;  // cannot edit
#endif

    // Process cvar modification
    uint16_t menuline_status_cvartype = currentMenu->menuitems[itemOn].status & IT_CVARTYPE;
    if(( menuline_status_cvartype == IT_CV_SLIDER )
     ||( menuline_status_cvartype == IT_CV_NOMOD  ))
    {
        CV_SetValue(cv, cv->value + d );
    }
    else
    {
        if(cv->flags & CV_FLOAT)
        {
            // This is the string that gets displayed too.
            char s[20];
            sprintf(s,"%.4f",(float)cv->value/FRACUNIT + d * (1.0/16.0));
            CV_Set(cv,s);
        }
        else if( menuline_status_cvartype == IT_CV_DELAY )
        {
            // Do not CALL until after delay.
            uint32_t flags = cv->flags;
            cv->flags &= ~CV_CALL;
            CV_ValueIncDec( cv, d ); // INC/DEC without CALL
            cv->flags = flags;  // Put back the CV_CALL.
            menu_delay_cv = cv;
            menu_delay_ticks = 45;  // 1.5 sec
        }
        else
        {
            CV_ValueIncDec( cv, d );
        }
    }

#ifdef CONFIG_MENU_PAGE
    config_cvar_edit_save();
#endif
}


#define MAX_CVAR_STRING  512

static
boolean M_Change_cvar_string(int key, char ch)
{
    consvar_t * cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
    char buf[ MAX_CVAR_STRING + 1 ];
    int  len;

#ifdef CONFIG_MENU_PAGE
    cv = config_cvar_edit_open( cv );
    if( ! cv )
        return false;  // cannot edit
#endif
   
    switch (key)
    {
      case KEY_BACKSPACE :
        len=strlen(cv->string);
        if( len > MAX_CVAR_STRING )  len = MAX_CVAR_STRING;
        if( len>0 )
        {
            memcpy(buf,cv->string,len);
            buf[len-1]=0;
            CV_Set(cv, buf);
#ifdef CONFIG_MENU_PAGE
            config_cvar_edit_save();
#endif
        }
        return true;
      default:
        if (is_printable(ch))
        {
            len=strlen(cv->string);
            if( len < MAX_CVAR_STRING-1 )
            {
                memcpy(buf,cv->string,len);
                buf[len++] = ch;
                buf[len] = 0;
                CV_Set(cv, buf);
#ifdef CONFIG_MENU_PAGE
                config_cvar_edit_save();
#endif
            }
            return true;
        }
        break;
    }
    return false;
}

//
// M_Responder
//
// [Arcade] Is this key bound to gamecontrol action gcnum, for either player?
static
boolean  M_key_is_control( uint16_t key, int gcnum )
{
    return ( gamecontrol[gcnum][0]  == key || gamecontrol[gcnum][1]  == key
          || gamecontrol2[gcnum][0] == key || gamecontrol2[gcnum][1] == key );
}

// [Arcade] The cabinet panel has no arrow keys, Enter or Escape, so map both
// players' buttons onto the menu keys: move = cursor, fire = select,
// use/open = back out.  Read from gamecontrol[]/gamecontrol2[] rather than
// hardcoding, so this follows the selected control scheme and any rebinding.
// Turn and strafe both act as left/right, which is what the player expects
// from a menu regardless of which pair the scheme assigns to turning.
static
uint16_t  M_Cabinet_Menu_Key( uint16_t key )
{
    if( key == KEY_NULL )  return key;   // unbound actions are KEY_NULL

    if( M_key_is_control(key, gc_forward) )     return KEY_UPARROW;
    if( M_key_is_control(key, gc_backward) )    return KEY_DOWNARROW;
    if( M_key_is_control(key, gc_turnleft)
     || M_key_is_control(key, gc_strafeleft) )  return KEY_LEFTARROW;
    if( M_key_is_control(key, gc_turnright)
     || M_key_is_control(key, gc_straferight) ) return KEY_RIGHTARROW;
    if( M_key_is_control(key, gc_fire) )        return KEY_ENTER;
    if( M_key_is_control(key, gc_use) )         return KEY_ESCAPE;

    return key;
}


boolean M_Responder (event_t* ev)
{
    static  boolean button_down = 0;
    static  tic_t  mousewait = 0;
    static  int  mousey = 0;
    static  int  mousex = 0;

    menufunc_t routine;  // for some casting problem
    menuitem_t * r_menuline;

    int i;
    int updown_limit;  // [Arcade] bounds the cursor search, see KEY_UPARROW
    int updown_from;   // [Arcade] where the cursor was before that search
    uint16_t key = KEY_NULL; // key pressed (if any)
    uint16_t button_key = 0; // mouse and joystick specific
    unsigned char ch = '\0';  // ASCII char it corresponds to

    switch (ev->type )
    {
     case ev_keydown :
        key = ev->data1;  // keycode

        // [Arcade] The join screen needs to know *which* panel pressed, and
        // M_Cabinet_Menu_Key below translates every panel's buttons into the
        // same cursor keys -- so this has to come first, before that identity
        // is thrown away.
        if( menuactive && M_Join_Key( key ) )
            return true;

        // [Arcade] Drive the menus from the cabinet buttons.  Only while a
        // menu is up, or "use" would open the menu during play instead of
        // opening doors.
        //
        // Applied in devmode too.  It used to be skipped there so an
        // operator's keyboard behaved normally, but that left a keyboard-mode
        // panel (a leverless/hitbox controller is just a keyboard) unable to
        // work the menus while configuring it, and left joystick panels at the
        // mercy of upstream's hardcoded KEY_JOY0BUT0 -> ENTER /
        // KEY_JOY0BUT1 -> BACKSPACE mapping further down.  That mapping is by
        // button *index*, so which physical buttons select and cancel changed
        // with an arcade stick's mode switch -- on a Mayflash F300,
        // DirectInput happened to put fire and use on buttons 0 and 1 and so
        // appeared correct, while XInput put A and B there and the operator's
        // own fire button did nothing.
        //
        // The cost is that in devmode a letter bound to a control no longer
        // reaches the menu's letter-shortcut search, since the translation
        // claims it first.
        //
        // *Text entry must be excluded.*  IT_KEYHANDLER items -- the name and
        // skin fields on the Setup Player screens -- are dispatched much later
        // in this function, so without this guard the translation would eat
        // their characters first: with "use" on 'a', typing a name would exit
        // the menu instead of typing an A.
        if( menuactive
            && ! ( currentMenu
                   && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER ) )
            key = M_Cabinet_Menu_Key( key );

        // [Arcade] The guided setup opens on the recommended-layout page and
        // waits here for any button.  Taken before the generic menu handling
        // so the page's own (invisible) item cannot swallow the press.
        if( guided_intro )
        {
            guided_intro = false;
            if( key != KEY_ESCAPE )
            {
                M_Guided_Begin_Steps();
                return true;
            }
            // ESC falls through and backs out of the page as usual.
        }

#ifdef SDL2
        // SDL2 has separate ASCII event for translated char,
        // but that will only be used if cv_sdl2_textchar is enabled.
        // Otherwise, this will be 0.
        ch  = ev->data2;  // ASCII char, maybe
#else
        ch  = ev->data2;  // ASCII char
#endif

        if( key >= KEY_MOUSE1 )
        {
            button_key = key;  // mouse or joystick

            // Menu slider, all buttons of mouse1, 1st button of mouse2 ???
            if( key <= KEY_MOUSE2 )
                button_down = 1;
        
            // added 5-2-98 remap virtual keys (mouse & joystick buttons)
            switch(key)
            {
             case KEY_MOUSE1:
             case KEY_JOY0BUT0:
                // [WDJ] No mouse ENTER key on the sliders, it makes them move right.
                key = ( currentMenu
                       && (currentMenu->menuitems[itemOn].status & (IT_BIGSLIDER | IT_CV_SLIDER | IT_CV_NOMOD ) )
                       ) ? KEY_NULL : KEY_ENTER;
                break;
             case KEY_MOUSE1+1:
             case KEY_JOY0BUT1:
                key = KEY_BACKSPACE;
                break;
             default:
                // [Leonardo Montenegro]
                // Custom binding for accessing main menu through other means
                // beside ESC key. Useful for allowing menu navigation through
                // controllers, per example
                if (key == gamecontrol[gc_menuesc][0] || key == gamecontrol[gc_menuesc][1] )
                {
                    key = KEY_ESCAPE;
                }
                else if(menuactive)
                {
                    // Navigating menus through joy hat of first controller.
                    if(key == KEY_JOY0HATUP)
                    {
                        key = KEY_UPARROW;
                    }
                    else if(key == KEY_JOY0HATDOWN)
                    {
                        key = KEY_DOWNARROW;
                    }
                    else if(key == KEY_JOY0HATLEFT)
                    {
                        key = KEY_LEFTARROW;
                    }
                    else if(key == KEY_JOY0HATRIGHT)
                    {
                        key = KEY_RIGHTARROW;
                    }
                }
            }
        }
        else
        {   // Keyboard keys,  key < KEY_MOUSE1.
            // on key press, inhibit menu responses to the mouse for a while
            mousewait = I_GetTime() + TICRATE*2;  // 4 sec
        }
        break;
     case ev_keyup:
        // Menu slider, all buttons of mouse1, 1st button of mouse2 ???
        if( ev->data1 >= KEY_MOUSE1 && ev->data1 <= KEY_MOUSE2 )
            button_down = 0;
        break;
     case ev_mouse:
        if( menuactive )
        {
            // [WDJ] This code only triggered when movement exceeded MENU_MOUSE_TRIG
            // so there is no need for mouse position recording.
            // Y movement overrides X movement, there can be ony one key.
            if( mousewait >= I_GetTime() )  break;  // delay between triggers
            mousex += ev->data2;
            mousey += ev->data3;
            if (mousey < -MENU_MOUSE_TRIG)
            {
                key = KEY_DOWNARROW;
            }
            else if (mousey > MENU_MOUSE_TRIG)
            {
                key = KEY_UPARROW;
            }
            else if (mousex < -MENU_MOUSE_TRIG)
            {
                if( button_down )   key = KEY_LEFTARROW;
            }
            else if (mousex > MENU_MOUSE_TRIG)
            {
                if( button_down )   key = KEY_RIGHTARROW;
            }

            if( key )
            {
                button_key = key;
                // On any trigger, zero everything, so cannot drift off slider.
                mousewait = I_GetTime() + TICRATE/7;
                mousex = mousey = 0;
            }
        }
        break;
#ifdef SDL2	
     case ev_textchar:  // SDL2 translated
	key = ev->data1;  // STX
	ch = ev->data2;  // if cv_sdl2_textchar is enabled, otherwise 0 and blocked.
        break;
#endif
     default:
        break;
    }

    if (key == KEY_NULL)
        return false;


    // Save Game string input
    if (edit_enable)
    {
        switch(key)
        {
          case KEY_BACKSPACE:
            if (edit_index > 0)
            {
                edit_index--;
                edit_buffer[edit_index] = 0;
            }
            break;

          case KEY_ESCAPE:
            edit_enable = 0;
            // restore from source
            strcpy(edit_buffer, &savegamedisp[slotindex].desc[0]);
            break;

          case KEY_ENTER:
            edit_enable = 0;
            if (edit_buffer[0])
            {
                strcpy(&savegamedisp[slotindex].desc[0], edit_buffer);
                if( edit_done_callback )   edit_done_callback();
                edit_done_callback = NULL;
            }
            break;

          default:
	    // [WDJ] Edit buffer with string, such as savegame slot desc.
#ifdef SDL2
            // Depending on cv_sdl2_textchar,
            // either ev_keydown, or ev_textchar, may have valid ch, the other will have ch = 0.
// if( ch )
//    printf( "EDIT EVENT: type=%i, data1=%i, data2=%i\n", ev->type, ev->data1, ev->data2 );
#endif
            if (ch
		&& is_printable(ch)
                && (edit_index < SAVESTRINGSIZE-1)
                && (V_StringWidth(edit_buffer) < (SAVESTRINGSIZE-2)*8) )
            {
                edit_buffer[edit_index++] = ch;
                edit_buffer[edit_index] = 0;
// printf("EDIT_BUFFER + %c => %s\n", ch, edit_buffer );
            }
            break;
        }
        goto ret_true;
    }

    // [Arcade] Both bindings, not just the first: this tested only slot [0],
    // so a second key assigned to Screenshot silently did nothing.
    if( (devparm && key == KEY_F1)
       || (key && key == gamecontrol[gc_screenshot][0])
       || (key && key == gamecontrol[gc_screenshot][1]) )
    {
        COM_BufAddText("screenshot\n");
        goto ret_true;
    }

    if( gamestate == GS_WAITINGPLAYERS )
    {
        if( D_WaitPlayer_Response( key ) )
            goto ret_true;
    }

    // when the menu is not open
    if (!menuactive)
    {
        switch(key)
        {
          case '-':         // Screen size down
            if (automapactive || chat_on || con_destlines)     // DIRTY !!!
                return false;
            cvar_incdec( &cv_viewsize, -1 );
            goto ret_true;

          case '=':        // Screen size up
            if (automapactive || chat_on || con_destlines)     // DIRTY !!!
                return false;
            cvar_incdec( &cv_viewsize, +1 );
            goto ret_true;

          case KEY_F1:            // Help key
            M_StartControlPanel ();

            if ( gamemode == ultdoom_retail )
              currentMenu = &ReadDef2;
            else
              currentMenu = &ReadDef1;

            itemOn = 0;
            S_StartSound(menu_sfx_open);
            goto ret_true;

          case KEY_F2:            // Save
            M_StartControlPanel();
            S_StartSound(menu_sfx_open);
            M_Savegame(0);
            goto ret_true;

          case KEY_F3:            // Load
            M_StartControlPanel();
            S_StartSound(menu_sfx_open);
            M_Loadgame(0);
            goto ret_true;

          case KEY_F4:            // Sound Volume
            M_StartControlPanel ();
            currentMenu = &SoundDef;
            itemOn = SVM_sfx_vol;
            S_StartSound(menu_sfx_open);
            goto ret_true;

          //added:26-02-98: now F5 calls the Video Menu
          case KEY_F5:
            S_StartSound(menu_sfx_open);
            M_StartControlPanel();
            Push_Setup_Menu (&VideoModeDef);
            //M_ChangeDetail(0);
            goto ret_true;

          case KEY_F6:            // Quicksave
            S_StartSound(menu_sfx_open);
            M_QuickSave();
            goto ret_true;

          //added:26-02-98: F7 changed to Options menu
          case KEY_F7:            // originally was End game
            S_StartSound(menu_sfx_open);
            M_StartControlPanel();
            Push_Setup_Menu (&OptionsDef);
            //M_EndGame(0);
            goto ret_true;

          case KEY_F8:            // Toggle messages
            CV_ValueIncDec(&cv_showmessages,+1);
            S_StartSound(menu_sfx_open);
            goto ret_true;

          case KEY_F9:            // Quickload
            S_StartSound(menu_sfx_open);
            M_QuickLoad();
            goto ret_true;

          case KEY_F10:           // Quit DOOM
            S_StartSound(menu_sfx_open);
            M_QuitDOOM(0);
            goto ret_true;

          //added:10-02-98: the gamma toggle is now also in the Options menu
          case KEY_F11:
            S_StartSound(menu_sfx_open);
            // bring up the gamma menu
            M_StartControlPanel();
            Push_Setup_Menu (&VideoOptionsDef);
            goto ret_true;

          // Pop-up menu
          case KEY_ESCAPE:
            M_StartControlPanel ();
            S_StartSound(menu_sfx_open);
            goto ret_true;
        }
        return false;
    }

    if( ! currentMenu )  return false;
    r_menuline = & currentMenu->menuitems[itemOn];
    routine = r_menuline->itemaction;

    //added:30-01-98:
    // Handle menuitems which need a specific key handling
    if( routine && (r_menuline->status & IT_TYPE) == IT_KEYHANDLER )
    {
      input_char = ch;
      routine(key);
      goto ret_true;
    }

    // Handle overriding keyboard input
    if( key_handler2 )
    {
        if( key_handler2(key) )  goto ret_true;
    }

    if( r_menuline->status==IT_MSGHANDLER )
    {
        // special message menu
        if( r_menuline->alphaKey == true )
        {
          // [smite] just for this purpose since unraveling the IT_MSGHANDLER hack would be too harrowing
          if (tolower(ch) == 'n')
            key = 'n';
          else if (tolower(ch) == 'y')
            key = 'y';

          if( button_key )  // Translate only for mouse and joystick.
          {
              if(key == KEY_ENTER) key = 'y'; // Convert Enter keypress to 'y' in menus, allowing game exit from controller
              if(key == KEY_BACKSPACE) key = 'n'; // Convert Backspace keypress to 'n' in menus, making possible to go back
          }

          // [WDJ] Important that message boxes not be cleared away by ENTER or any normal keypress.
          // If a message box pops up, it must ignore the normal keypresses.  Otherwise someone hitting
          // keys quickly will blow it away before they even read it.
          if(key == KEY_SPACE || key == 'n' || key == 'y' || key == KEY_ESCAPE)
          {
                if(routine) routine(key);
                M_StopMessage(0);
          }
        }
        else
        {
            //added:07-02-98:dirty hak:for the customize controls, I want only
            //      buttons/keys, not moves
            if (ev->type == ev_mouse)
                goto ret_true;

            // Call the itemaction routine, with the key.
            void (*cc_action)(event_t *) = r_menuline->itemaction;
            if (cc_action)   cc_action(ev);
        }
        goto ret_true;
    }

    // BP: one of the more big hack i have never made
    if( routine && (r_menuline->status & IT_TYPE) == IT_CVAR )
    {
        if( (r_menuline->status & IT_CVARTYPE) == IT_CV_STRING )
        {
            if( M_Change_cvar_string(key, ch) )
                goto ret_true;

            routine = NULL;
        }
        else
            routine = M_Change_cvar_value;
    }

#ifdef CONFIG_MENU_PAGE
    if( config_cvar_key_handler( key ) )
        goto ret_true;
#endif
   
    // Keys usable within menu
    switch (key)
    {
#if defined SAVEGAMEDIR || defined SAVEGAME99
      case KEY_DELETE:	// delete directory or savegame
#ifdef SAVEGAMEDIR
        // The Dir, Loadgame, Savegame menus all have dir at MSLOT_0
        if( delete_callback && itemOn >= SAVEGAME_MSLOT_0 )
        {
            slotindex = itemOn - SAVEGAME_MSLOT_0;
            M_StartMessage("Delete Y/N?", delete_callback, MM_YESNO);
            goto ret_action;
        }
#else
        if( delete_callback && itemOn >= 0 )
        {
            slotindex = itemOn;
            M_StartMessage("Delete Y/N?", delete_callback, MM_YESNO);
            goto ret_action;
        }
#endif
        break;
       
      case '[':
      case KEY_PGUP:
        if( scroll_callback && (scroll_index > 0))
        {
            scroll_callback( -6 );  // some functions need to correct
            goto ret_updown;
        }
        break;

      case ']':
      case KEY_PGDN:
        if( scroll_callback && (scroll_index < (99-6)))
        {
            scroll_callback( 6 );
            goto ret_updown;
        }
        break;
#endif

      case KEY_DOWNARROW:
#if defined SAVEGAMEDIR || defined SAVEGAME99
        if( scroll_callback && (scroll_index < 99) && (itemOn >= SAVEGAME_MSLOT_LAST))
        {
            // scrolling menu scrolls preferentially
            scroll_index ++;
            scroll_callback( 1 );
            goto ret_updown;
        }
#endif
        // [Arcade] Bounded, see the note on the up arrow below.
        updown_limit = currentMenu->numitems;
        updown_from = itemOn;
        do
        {
            if( --updown_limit < 0 )
            {
                itemOn = updown_from;  // nothing selectable, stay put
                goto ret_updown;
            }

            if (itemOn+1 > currentMenu->numitems-1)
            {
                if( scroll_callback )  // only wrap when not scrolling
                    goto ret_updown;
                itemOn = 0;
            }
            else itemOn++;
        } while((currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_SPACE);
        goto ret_updown;

      case KEY_UPARROW:
#if defined SAVEGAMEDIR || defined SAVEGAME99
        if( scroll_callback && (scroll_index > 0) && (itemOn < (SAVEGAME_MSLOT_0+1)))
        {
            // scrolling menu scrolls preferentially
            scroll_index --;
            if( scroll_index < 0 )   scroll_index = 0;
            scroll_callback( -1 );  // some functions need to correct
            goto ret_updown;
        }
#endif       
        // [Arcade] Bounded by the item count.  A menu can legitimately have
        // *no* selectable item -- the Cheats page greys every row out when
        // there is no single player game to cheat in, and the cabinet lockdown
        // hides whole menus with IT_HIDDEN, which is an IT_SPACE type as well.
        // Unbounded, this searches for a selectable item that does not exist
        // and spins for ever inside the event handler: a hard lockup with no
        // way out, which is exactly what opening Cheats from the attract
        // screen and pressing down used to do.
        updown_limit = currentMenu->numitems;
        updown_from = itemOn;
        do
        {
            if( --updown_limit < 0 )
            {
                itemOn = updown_from;  // nothing selectable, stay put
                goto ret_updown;
            }

            if (!itemOn)
                itemOn = currentMenu->numitems-1;
            else itemOn--;
        } while((currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_SPACE);
        goto ret_updown;

      case KEY_LEFTARROW:
        if (  routine &&
            ( (r_menuline->status & IT_TYPE) == IT_ARROWS
            ||(r_menuline->status & IT_TYPE) == IT_CVAR   ))
        {
            S_StartSound(menu_sfx_val);
            routine(0);
        }
        goto ret_true;

      case KEY_RIGHTARROW:
        if ( routine &&
            ( (r_menuline->status & IT_TYPE) == IT_ARROWS
            ||(r_menuline->status & IT_TYPE) == IT_CVAR   ))
        {
            S_StartSound(menu_sfx_val);
            routine(1);
        }
        goto ret_true;

      case KEY_ENTER:
        currentMenu->lastOn = itemOn;
        menuline = r_menuline;
        if ( routine )
        {
            switch (menuline->status & IT_TYPE)  {
                case IT_CVAR:
                case IT_ARROWS:
                    routine(1);            // right arrow
                    S_StartSound(menu_sfx_val);
                    break;
                case IT_CALL:
                    routine(itemOn);
                    S_StartSound(menu_sfx_enter);
                    break;
                case IT_SUBMENU:
                    Push_Setup_Menu((menu_t *)menuline->itemaction);
                    S_StartSound(menu_sfx_enter);
                    break;
            }
        }
        goto ret_true;

      case KEY_ESCAPE:
        currentMenu->lastOn = itemOn;
        if( init_sequence == 1 )
            goto ret_true;  // No escape from Launcher

        if( menucnt )
        {
            Pop_Menu();
            itemOn = currentMenu->lastOn;
            S_StartSound(menu_sfx_open); // a matter of taste which sound to choose
        }
        else
        {
            M_Clear_Menus (true);
            S_StartSound(menu_sfx_esc);
            // Exit menus, return to demo or game
            if( ! Game_Playing() )
                D_StartTitle();  // restart title screen and demo
        }
        goto ret_true;

      case KEY_BACKSPACE:
        if( r_menuline->status == IT_CONTROL )
        {
            S_StartSound(menu_sfx_val);
            // detach any keys associated to the game control
            G_Clear_ControlKeys (setupcontrols, currentMenu->menuitems[itemOn].alphaKey);
            goto ret_true;
        }
        currentMenu->lastOn = itemOn;
        if( menucnt )
        {
            menucnt --;
            currentMenu = menustack[ menucnt ];
            itemOn = currentMenu->lastOn;
            S_StartSound(menu_sfx_open);
        }
        goto ret_true;

       
      default:
#if 1
        // any other key: if a letter, try to find the corresponding menuitem
        if (!isalpha(ch))
#else
        if( ch == 0 )
#endif
          goto ret_true;

        // [Arcade] No letter shortcuts on the cabinet: the controls are
        // buttons, and several are letters that collide with these
        // shortcuts -- pressing player 1's turn-right button ('e') on the
        // New Game menu jumped straight to END GAME.  Arrow keys and Enter
        // are all a cabinet needs.  Text entry is unaffected: IT_KEYHANDLER
        // items consume the key earlier in this function.
        if( ! devmode )
          goto ret_true;

        // Skip IT_SPACE items, which covers IT_HIDDEN and IT_DISABLED as
        // well as plain spacers.  Without this the hotkey of a hidden entry
        // (for instance 's' for the hidden Save Game) still moved the
        // cursor onto that invisible row.
#define MENU_HOTKEY_MATCH(i) \
        (    (currentMenu->menuitems[i].alphaKey == ch) \
          && ((currentMenu->menuitems[i].status & IT_OPTION) == 0) \
          && ((currentMenu->menuitems[i].status & IT_TYPE) != IT_SPACE) )

        // from itemOn to bottom
        for (i = itemOn+1;i < currentMenu->numitems;i++)
        {
            if( MENU_HOTKEY_MATCH(i) )
            {
                itemOn = i;
                goto ret_action;
            }
        }
        // search from top to itemOn
        for (i = 0;i <= itemOn;i++)
        {
            if( MENU_HOTKEY_MATCH(i) )
            {
                itemOn = i;
                goto ret_action;
            }
        }
#undef MENU_HOTKEY_MATCH
        break;

    }
   
ret_true:   
    return true;

ret_action:
    S_StartSound(menu_sfx_action);
    return true;

ret_updown:
    S_StartSound(menu_sfx_updown);
    return true;
}

//
//      Find string height from hu_font chars
//
static
int M_StringHeight(char* string)
{
    int      i;
    int      h;
    int      height = 8; //(hu_font[0]->height);

    h = height;
    for (i = 0;i < (int)strlen(string);i++)
    {
        if (string[i] == '\n')
            h += height;
    }

    return h;
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
// [Arcade] Tell the player, on the screens where they can cause it, that the
// current settings have taken the session out of the running for records.
// Drawn from M_Drawer rather than by giving each menu its own drawroutine,
// so the three option screens that reach these cvars are covered in one
// place -- and so it follows the player down into Adv Options.
static void M_Draw_Unranked_Warning( void )
{
    if( devmode )  return;   // the operator is expected to change things
    if( currentMenu != &GameOptionDef
        && currentMenu != &AdvOption1Def
        && currentMenu != &AdvOption2Def )  return;
    if( HS_Ruleset_Is_Ranked() )  return;

    // Under the item list, above the bottom edge.  All three menus start at
    // y=40 and their lowest entry is an IT_YOFFSET at +130, so the item text
    // ends at 178 -- the first line has to clear that, and the second line's
    // 8-tall glyphs still have to land inside BASEVIDHEIGHT (200).
    V_DrawString( 8, BASEVIDHEIGHT-20, V_WHITEMAP,
                  "SETTINGS CHANGED - NO HIGH SCORES" );
    V_DrawString( 8, BASEVIDHEIGHT-10, 0,
                  "OR RECORDINGS THIS GAME. END GAME TO RESET." );
}


void M_Drawer (void)
{
    if (!menuactive)
        return;

    // center the scaled graphics for the menu,
    V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART | V_CENTERMENU );

    // now that's more readable with a faded background (yeah like Quake...)
    if ( reg_colormaps )  // not before colormaps loaded
        V_FadeScreen ();

    // menu drawing
    if (currentMenu->drawroutine)
        currentMenu->drawroutine();      // call current menu Draw routine

    M_Draw_Unranked_Warning();   // [Arcade]

    V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART | V_CENTERHORZ );  // restore

}

//
// M_StartControlPanel
//
// Called by G_Responder, M_Responder, M_QuickSave, M_StartMessage.
void M_StartControlPanel (void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;

    menuactive = 1;
    currentMenu = &MainDef;         // JDC
    itemOn = currentMenu->lastOn;   // JDC

    if(demoplayback)  // menus without the demo interference
    { 
        G_StopDemo();
        R_FillBackScreen ();
    }

    CON_ToggleOff ();   // dirty hack : move away console

    I_StartupMouse( false );  // menu mode
}

//
// M_Clear_Menus
//
void M_Clear_Menus (boolean callexitmenufunc)
{
#ifdef CONFIG_MENU_PAGE
    menu_cfg = CFG_none;  // cancel any config editing
    menu_cfg_editing = 0;
#endif

    if(!menuactive)
        return;

    if( currentMenu->quitroutine && callexitmenufunc)
    {
        if( !currentMenu->quitroutine())
            return; // we can't quit this menu (also used to set parameter from the menu)
    }

    menuactive = 0;
    I_StartupMouse( !paused );  // play mode if not paused
}


static
void M_SetupMenu(menu_t *menudef)
{
    if( currentMenu && (currentMenu->quitroutine) )
    {
        if( !currentMenu->quitroutine())
            return; // we can't quit this menu (also used to set parameter from the menu)
    }

    // Menu history
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;

    // in case of...
    if (itemOn >= currentMenu->numitems)
        itemOn = currentMenu->numitems - 1;

    // the curent item can be disabled,
    // this code go up until a enabled item found
    while( (currentMenu->menuitems[itemOn].status==IT_DISABLED
            || currentMenu->menuitems[itemOn].status==IT_HIDDEN) && itemOn )
        itemOn--;

    delete_callback = NULL;
    scroll_callback = NULL;
}


//
// Push_Setup_Menu
//
static
void Push_Setup_Menu(menu_t *menudef)
{
    int i;

    // [WDJ] Menus use dynamic ordering, from multiple parent menus.
    // Check for duplicate
    if( menucnt > 0 )
    {
        for( i = 0; i < menucnt; i++ )
        {
            if( menustack[i] == currentMenu )
            {
                // User is looping through menus.
                menucnt = i+1;  // cut off the loop
                goto setup;
            }
        }
    }

    if( menucnt < NUM_MENUSTACK )
    {
        // Normal push menu
        menustack[ menucnt++ ] = currentMenu;
    }

setup:   
    M_SetupMenu( menudef );
}

// Go back to the previous menu, reloading data if necessary
static
void Pop_Menu( void )
{
    if( menucnt )
    {
       M_SetupMenu( menustack[ -- menucnt ] );  // back out

       // refresh data
       if(   currentMenu == &LoadDef 
          || currentMenu == &SaveDef )
               attach_savegame_menu();
    }
    else
    {
       M_Clear_Menus (true);
       // Exit menus, return to demo or game
       if( ! Game_Playing() )
           D_StartTitle();  // restart title screen and demo
    }
}


//
// M_Ticker
//
// Call once per tic.
void M_Ticker (void)
{
    M_Join_Ticker();   // [Arcade] join screen countdown

    if (--skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8 * NEWTICRATERATIO;
    }
   
    //added:30-01-98:test mode for five seconds
    if( vidm_testing_cnt > 0 )
    {
        if( --vidm_testing_cnt == 0 )
        {
            // restore the previous video mode
            if( key_handler2 )  // video test key handler
            {
                vidm_testing_cnt = 1;  // necessary to process ESCAPE
                key_handler2( KEY_ESCAPE );
            }
        }
    }

    if( menu_delay_ticks > 0)
    {
        if( --menu_delay_ticks == 0 )
        {
            if( menu_delay_cv )
            {
                // Make current value effective.
                CV_cvar_call( menu_delay_cv, 1 );
                menu_delay_cv = NULL;
            }
        }
    }
}

static
consvar_t * menu_init_cvar_list[] =
{
  &cv_skill,
  &cv_monsters,
  &cv_nextmap,
  &cv_nextepmap,
  &cv_deathmatch_menu,
  &cv_dm_timelimit,     // [Arcade]
  &cv_wait_players,
  &cv_wait_timeout,
  &cv_serversearch,
  &cv_download_files,
  &cv_download_savegame,
  &cv_netrepair,
  &cv_SV_download_files,
  &cv_SV_download_savegame,
  &cv_SV_netrepair,
  &cv_menusound,
  NULL
};


//
// M_Init
//
// Called once, very early
void M_Init (void)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;

    whichSkull = 0;
    skullAnimCounter = 10;

    quicksave_slotid = -1;

#ifdef CONFIG_MENU_PAGE
    temp_cvar.string = NULL;
#endif

    if( ! devmode )
    {
        // Locked-down (e.g. arcade cabinet) build: no multiplayer server,
        // no save/load or options tampering.
        SingleMulti_Menu[2].status = IT_HIDDEN;  // Networked Multiplayer
        if( SingleMultiDef.lastOn == 2 )
            SingleMultiDef.lastOn = 0;

        TwoPlayerMenu[twoplayer_networked].status = IT_HIDDEN;
        if( TwoPlayerDef.lastOn == twoplayer_networked )
            TwoPlayerDef.lastOn = 0;

        // [Arcade] The Net Options page is the deathmatch ruleset a player
        // can reasonably choose from; the rest is server and network
        // plumbing that means nothing on a cabinet.  Kept: Allow exitlevel,
        // Teamplay, TeamDamage, Fraglimit, Timelimit, Deathmatch Type,
        // Frag's Weapon Falling, and the Game Options link.
        NetOptionsMenu[netoption_allowjump].status       = IT_HIDDEN;
        NetOptionsMenu[netoption_allowrocketjump].status = IT_HIDDEN;
        NetOptionsMenu[netoption_allowautoaim].status    = IT_HIDDEN;
        NetOptionsMenu[netoption_allowturbo].status      = IT_HIDDEN;
        NetOptionsMenu[netoption_allowjoin].status       = IT_HIDDEN;
        NetOptionsMenu[netoption_maxplayers].status      = IT_HIDDEN;
        if( NetOptionDef.lastOn < netoption_allowexit )
            NetOptionDef.lastOn = netoption_allowexit;

        MainMenu[2].status = IT_HIDDEN;  // Load Game  (2/3 since Single Level
        MainMenu[3].status = IT_HIDDEN;  // Save Game   took index 1)
        if( MainDef.lastOn >= 2 && MainDef.lastOn <= 3 )
            MainDef.lastOn = 0;

        MainMenu[MM_cheats].status = IT_HIDDEN;   // [Arcade] operator only
        if( MainDef.lastOn == MM_cheats )
            MainDef.lastOn = 0;

        // Options stays reachable, but pared down to the few settings a
        // player may change.  Leaves Crosshair, Player >>, Game Options >>.
        OptionsMenu[0].status  = IT_HIDDEN;  // Messages:
        OptionsMenu[1].status  = IT_HIDDEN;  // Always Run
        OptionsMenu[4].status  = IT_HIDDEN;  // Effects Options >>
        OptionsMenu[6].status  = IT_HIDDEN;  // Connect Options >>
        OptionsMenu[7].status  = IT_HIDDEN;  // Network Options >>
        OptionsMenu[8].status  = IT_HIDDEN;  // Server Options >>
        OptionsMenu[9].status  = IT_HIDDEN;  // Menu Options >>
        OptionsMenu[10].status = IT_HIDDEN;  // Sound Volume >>
        OptionsMenu[11].status = IT_HIDDEN;  // Video Options >>
        OptionsMenu[12].status = IT_HIDDEN;  // Setup Controls >>
        OptionsDef.lastOn = 2;   // Crosshair, the first item still shown

        // Game Options reaches Network Options too; hide that as well.
        // It is the last entry, and the array length varies with the
        // MAPADJUST_MENU and ENABLE_TIRED_RUN build options, so index it
        // from the end rather than by a fixed number.
        GameOptionsMenu[ GameOptionDef.numitems - 1 ].status = IT_HIDDEN;
        if( GameOptionDef.lastOn >= GameOptionDef.numitems - 1 )
            GameOptionDef.lastOn = 0;

        ServerMenu[5].status = IT_HIDDEN;  // Wait Players
        ServerMenu[6].status = IT_HIDDEN;  // Wait Timeout
        ServerMenu[7].status = IT_HIDDEN;  // Internet Server
        ServerMenu[8].status = IT_HIDDEN;  // Server Name
        ServerMenu[10].status = IT_HIDDEN; // Dedicated
        if( (ServerDef.lastOn >= 5 && ServerDef.lastOn <= 8)
            || ServerDef.lastOn == 10 )
            ServerDef.lastOn = 0;

        // Setup Player 1/2 (shared array for both players): fixed
        // name/skin, no per-player mouse config or control rebinding.
        SetupMultiPlayerMenu[setupmultiplayer_name].status = IT_HIDDEN;  // Your name
        SetupMultiPlayerMenu[setupmultiplayer_skin].status = IT_HIDDEN;  // Your skin
        SetupMultiPlayerMenu[setupmultiplayer_controls].status = IT_HIDDEN;  // Player2 Controls >>
        SetupMultiPlayerMenu[setupmultiplayer_mouse2].status = IT_HIDDEN;  // Second Mouse config >>
        if( SetupMultiPlayerDef.lastOn == setupmultiplayer_name
            || SetupMultiPlayerDef.lastOn == setupmultiplayer_skin
            || SetupMultiPlayerDef.lastOn == setupmultiplayer_controls
            || SetupMultiPlayerDef.lastOn == setupmultiplayer_mouse2 )
            SetupMultiPlayerDef.lastOn = setupmultiplayer_color;

        // Player config screen (also shared for both players).
        // Leaves Crosshair and the Player setup screen.
        PlayerOptionsMenu[playeroption_alwaysrun].status = IT_HIDDEN;  // Always Run
        PlayerOptionsMenu[playeroption_autoaim].status = IT_HIDDEN;  // Autoaim
        PlayerOptionsMenu[playeroption_usemouse].status = IT_HIDDEN;  // Use Mouse
        PlayerOptionsMenu[playeroption_mousemove].status = IT_HIDDEN;  // Mouse Move
        PlayerOptionsMenu[playeroption_mouselook].status = IT_HIDDEN;  // Always MouseLook
        PlayerOptionsMenu[playeroption_weaponpref].status = IT_HIDDEN;  // WeaponPref
        PlayerOptionsMenu[playeroption_setupcontrol].status = IT_HIDDEN;  // Player1/2 controls >>
        PlayerOptionsDef.lastOn = playeroption_crosshair;  // first item still shown
    }

    CV_RegisterVar_list( menu_init_cvar_list );
}

#ifdef ENABLE_UMAPINFO
// [MB] 2023-03-23: Modify episode menu with data from UMAPINFO
// If clear is false, the new entries are appended to the existing menu
static
void M_EpisodeMenuFromUMAPINFO (void)
{
    boolean   modified = false;
    emenu_t * episode  = umapinfo.emenu_list;

    // Accept clear only for the first UMAPINFO map entry
    if( umapinfo.emenu_clear )
    {
        modified        = true;
        EpiDef.numitems = 0;
# ifdef DEBUG_UMAPINFO
        if( EN_umi_debug_out )
            GenPrintf(EMSG_debug, "UMAPINFO: Default episode menu cleared\n");
# endif
    }

    while( episode )
    {
        // UMAPINFO allows 8 episodes, currently only 5 are supported
        if( EpiDef.numitems < 5 )
        {
            // EpisodeMenu[EpiDef.numitems].status is below
            // FIXME: Cast to char* is ugly
            // Is it possible to declared elements const in menuitem_t?
            EpisodeMenu[EpiDef.numitems].patch = (char*)episode->patch;
            EpisodeMenu[EpiDef.numitems].text  = (char*)episode->name;
            // EpisodeMenu[EpiDef.numitems].itemaction is not changed
            if( episode->key[0] )
            {
                // UMAPINFO value is defined as quoted string
                // The first character is used, if string is not empty
                byte k = (byte)episode->key[0];

                EpisodeMenu[EpiDef.numitems].alphaKey = k;
            }
            EpiDef.numitems++;
# ifdef DEBUG_UMAPINFO
            if( EN_umi_debug_out )
                GenPrintf(EMSG_debug, "UMAPINFO: Episode entry added\n");
# endif
        }
        modified = true;
        episode  = episode->next;
    }

    // Use patches only if all episodes have a valid patch
    if( modified )
    {
        boolean  use_patches = true;
        uint16_t i;

        // FIXME: This should check if patches are really usable
        for( i = 0; i < EpiDef.numitems; i++ )
        {
            if( EpisodeMenu[i].patch == NULL || EpisodeMenu[i].patch[0] == 0 )
            {
                use_patches = false;
                break;
            }
        }

# ifdef DEBUG_UMAPINFO
        if( EN_umi_debug_out )
        {
            GenPrintf(EMSG_debug, "UMAPINFO: Modified episode menu ");
            if( use_patches )
                GenPrintf(EMSG_debug, "(using patches):\n");
            else
                GenPrintf(EMSG_debug, "(using strings):\n");
        }
# endif

        for( i = 0; i < EpiDef.numitems; i++ )
        {
# ifdef DEBUG_UMAPINFO
            if( EN_umi_debug_out )
            {
                GenPrintf(EMSG_debug, "    Episode %u: ", (unsigned int) i + 1u);
                if( use_patches )
                    GenPrintf(EMSG_debug, "%s\n", EpisodeMenu[i].patch);
                else
                    GenPrintf(EMSG_debug, "%s\n", EpisodeMenu[i].text);
            }
# endif

            EpisodeMenu[i].status = IT_CALL;
            if( use_patches )
                EpisodeMenu[i].status |= IT_PATCH;
            else
                EpisodeMenu[i].status |= IT_STRING2;
        }
    }
}
#endif

// Called once after gamemode has been determined, game dependent
void M_Configure (void)
{
    int i;
    int cval;

    if(dedicated)
        return;

    // [Arcade] The Single Level page's map list depends on the gamemode, which
    // is not known at M_Init -- IdentifyVersion runs later.  Doom 2 has a flat
    // MAPxx list, the Doom 1 games are episode+map.  Done here rather than on
    // menu open because the main menu reaches SingleLevelDef with IT_SUBMENU,
    // which has no handler to hook.
    SingleLevelMenu[SL_map] = (gamemode==doom2_commercial)?
         SingleLevelMenu_Map
       : SingleLevelMenu_EpisodeMap;

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.

    // Reversible
    // remove the inventory key from the menu !
    cval = ( EN_inventory )? (IT_CONTROL) : IT_LITLSPACE;
    for( i=0; i<ControlDef2.numitems; i++)
    {
        if( ControlMenu2[i].alphaKey == gc_invprev ||
            ControlMenu2[i].alphaKey == gc_invnext ||
            ControlMenu2[i].alphaKey == gc_invuse )
            ControlMenu2[i].status = cval;
    }

    // remove the fly down key from the menu !
    cval = ( gamemode == heretic )? (IT_CONTROL) : IT_LITLSPACE;
    for( i=0; i<ControlDef2.numitems; i++)
    {
        if( ControlMenu2[i].alphaKey == gc_flydown )
            ControlMenu2[i].status = cval;
    }

    // irreversible
    if( ! VALID_LUMP( W_CheckNumForName("E2M1") ) )
    {
        exmy_cons_t[9].value = 0;
        exmy_cons_t[9].strvalue = NULL;
    }
    else
    if( ! VALID_LUMP( W_CheckNumForName("E3M1") ) )
    {
        exmy_cons_t[18].value = 0;
        exmy_cons_t[18].strvalue = NULL;
    }
    else
    if( ! VALID_LUMP( W_CheckNumForName("E4M1") ) )
    {
        exmy_cons_t[27].value = 0;
        exmy_cons_t[27].strvalue = NULL;
    }
    else
    if( ! VALID_LUMP( W_CheckNumForName("E5M1") ) )
    {
        exmy_cons_t[36].value = 0;
        exmy_cons_t[36].strvalue = NULL;
    }

        // [Arcade] Cabinets without a second set of controls hide two player
    // play entirely.  Must be here, not in M_Init's lockdown: cv_twoplayer
    // comes from config.cfg, which D_DoomMain does not load until long after
    // M_Init runs, so the value would still be the default there.
    if( ! devmode && ! cv_twoplayer.EV )
    {
        SingleMulti_Menu[1].status = IT_HIDDEN;  // Two Player Game
        if( SingleMultiDef.lastOn == 1 )
            SingleMultiDef.lastOn = 0;

        // Player 2's config screen is unreachable in play and meaningless
        // without a second panel, so take it down with the rest.
        PlayerDirectorMenu[1].status = IT_HIDDEN;
        if( PlayerDirectorDef.lastOn == 1 )
            PlayerDirectorDef.lastOn = 0;

        TwoPlayerMenu[twoplayer_p2_config].status = IT_HIDDEN;
        if( TwoPlayerDef.lastOn == twoplayer_p2_config )
            TwoPlayerDef.lastOn = 0;
    }

    // [Arcade] Panels 3 and 4 exist only on a cabinet configured for them.
    // Hidden rather than removed, like the rest of the lockdown, because
    // several menus are indexed by position.  Applied here in M_Configure,
    // not in M_Init: cv_localplayers comes from config.cfg, which is not
    // loaded until long after M_Init runs -- the same rule as cv_twoplayer
    // and the game selector.
    {
        byte panels = cv_localplayers.EV;
        byte i;

        if( panels > MAXSPLITSCREENPLAYERS )  panels = MAXSPLITSCREENPLAYERS;

        for( i=2; i<MAXSPLITSCREENPLAYERS; i++ )
        {
            if( i < panels )  continue;   // this panel exists, leave it shown

            PlayerDirectorMenu[i].status = IT_HIDDEN;
            if( PlayerDirectorDef.lastOn == i )
                PlayerDirectorDef.lastOn = 0;

            TwoPlayerMenu[ twoplayer_p1_config + i ].status = IT_HIDDEN;
            if( TwoPlayerDef.lastOn == (twoplayer_p1_config + i) )
                TwoPlayerDef.lastOn = 0;

            // Both of this panel's rows on the Controls menu: the guided
            // setup and the full binding page.
            MControlMenu[ mcontrol_guided_p1 + i ].status = IT_HIDDEN;
            MControlMenu[ mcontrol_p1_controls + i ].status = IT_HIDDEN;
            if( MControlDef.lastOn == (mcontrol_guided_p1 + i)
                || MControlDef.lastOn == (mcontrol_p1_controls + i) )
                MControlDef.lastOn = 0;
        }
    }

    // [Arcade] Only offer games whose IWAD is actually present.  Done here
    // because the doomwaddir search paths are not set up as early as M_Init.
    {
        int gs;
        int avail = 0;
        int first = -1;

        M_Scan_LevelPacks();

        for( gs = 0; gs < GS_numgames; gs++ )
        {
            if( D_Game_Available( gameselect_arg[gs] ) )
            {
                if( first < 0 )  first = gs;
                avail++;
            }
            else
                GameSelectMenu[gs].status = IT_HIDDEN;
        }

        // Append a line per level pack found.
        for( gs = 0; gs < num_levelpack; gs++ )
        {
            int mi = GS_numgames + gs;
            GameSelectMenu[mi].status = IT_STRING | IT_CALL;
            GameSelectMenu[mi].text = levelpack_label[gs];
            GameSelectMenu[mi].itemaction = M_SelectGame;
            GameSelectMenu[mi].alphaKey = 0;
            if( first < 0 )  first = mi;
            avail++;
        }
        GameSelectDef.numitems = GS_numgames + num_levelpack;
        if( first >= 0 )
            GameSelectDef.lastOn = first;   // start on a shown item

        // Nothing worth switching to: only the game already running, or none.
        if( avail < 2 )
            OptionsMenu[OPT_selectgame].status = IT_HIDDEN;
    }

    switch ( gamemode )
    {
      case doom2_commercial:
        // This is used because DOOM 2 had only one HELP
        //  page. I use CREDIT as second page now, but
        //  kept this hack for educational purposes.
        MainMenu[MM_readthis] = MainMenu[MM_quitdoom];
        MainDef.numitems--;
        MainDef.y += 8;
        ReadDef1.drawroutine = M_DrawReadThis1;
        ReadDef1.x = 330;
        ReadDef1.y = 165;
        ReadMenu1[0].itemaction = &MainDef;
        break;
      case doom_shareware:
        // Episode 2 and 3 are handled,
        //  branching to an ad screen.
      case doom_registered:
          // We need to remove the fourth episode.
          EpiDef.numitems--;
      case ultdoom_retail:
          // We are fine.
          // [Arcade] Doom 2 replaces the "Read This!" entry just above, but
          // the Doom 1 gamemodes keep it, and it is the help / order-form
          // screens -- of no use on a cabinet.  Done here rather than in
          // M_Init because gamemode is not known that early.
          if( ! devmode )
          {
              MainMenu[MM_readthis].status = IT_HIDDEN;
              if( MainDef.lastOn == MM_readthis )
                  MainDef.lastOn = 0;
          }
          cv_nextmap.PossibleValue = exmy_cons_t;
          cv_nextmap.defaultvalue = "11";
          // We need to remove the fifth episode.
          EpiDef.numitems--;
#ifdef ENABLE_UMAPINFO
          // [MB] 2023-03-23: Support for UMAPINFO added
          M_EpisodeMenuFromUMAPINFO();
 // [WDJ] This should be after this switch, for all gamemodes ??
 // I expect the hertic users will be asking for it.
#endif
          break;
      case heretic:
          cv_nextmap.PossibleValue = exmy_cons_t;
          cv_nextmap.defaultvalue = "11";

          MainDef.menutitlepic = "M_HTIC";
          MainDef.drawroutine = HereticMainMenuDrawer;
          SkullBaseLump = W_GetNumForName("M_SKL00");
          strcpy(skullName[0], "M_SLCTR1");
          strcpy(skullName[1], "M_SLCTR2");

          EpisodeMenu[0].text = "CITY OF THE DAMNED";
          EpisodeMenu[1].text = "HELL'S MAW";
          EpisodeMenu[2].text = "THE DOME OF D'SPARIL";
          EpisodeMenu[3].text = "THE OSSUARY";
          EpisodeMenu[4].text = "THE STAGNANT DEMESNE";

          NewGameMenu[0].text = "THOU NEEDETH A WET-NURSE";
          NewGameMenu[1].text = "YELLOWBELLIES-R-US";
          NewGameMenu[2].text = "BRINGEST THEM ONETH";
          NewGameMenu[3].text = "THOU ART A SMITE-MEISTER";
          NewGameMenu[4].text = "BLACK PLAGUE POSSESSES THEE";
          break;

      case chexquest1:
          if( Chex_safe_pictures( "M_EPI1", NULL ) == NULL )
          {
              // Found Doom episode title in wad.
              // [WDJ] Coverup for Doom episode titles in chexquest1 wad
              // According to Chexquest3.
              EpisodeMenu[0].text = "Rescue on Baziok";
              EpisodeMenu[1].text = "Terror in Chex-City"; // Avoid needing (R)
              EpisodeMenu[2].text = "Invasion";
              EpisodeMenu[3].text = "Episode 4";
              EpisodeMenu[4].text = "Episode 5";
              EpisodeMenu[0].status = IT_CALL | IT_STRING2;
              EpisodeMenu[1].status = IT_CALL | IT_STRING2;
              EpisodeMenu[2].status = IT_CALL | IT_STRING2;
              EpisodeMenu[3].status = IT_CALL | IT_STRING2;
              EpisodeMenu[4].status = IT_CALL | IT_STRING2;
          }
          break;

       
      default:
        break;
    }

#ifdef ENABLE_UMAPINFO
#if 0   
    if( game_umapinfo )
    {
        // [MB] 2023-03-23: Support for UMAPINFO added
        M_EpisodeMenuFromUMAPINFO();
 // [WDJ] This should be after the switch, for all gamemodes ??
 // I expect the hertic users will be asking for it.
    }
# endif   
#endif

    CV_menusound_OnChange();
}

//======================================================================
// Lighting

menuitem_t LightingMenu[]=
{
    {IT_STRING | IT_CVAR ,0, "Corona"             , &cv_corona          , 0},
    {IT_STRING | IT_CVAR, 0, "Corona size"        , &cv_coronasize      , 0},
    {IT_STRING | IT_CVAR ,0, "Corona draw"        , &cv_corona_draw_mode, 0},
//    {IT_STRING | IT_CVAR, 0, "Dynamic lighting"   , &cv_dynamiclighting , 0},
//    {IT_STRING | IT_CVAR, 0, "Static lighting"    , &cv_staticlighting  , 0},
//    {IT_STRING | IT_CVAR, 0, "Monster ball light" , &cv_monball_light    , 0},
};

menu_t  LightingDef =
{
    "M_OPTTTL",
    "OPTIONS",
    LightingMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(LightingMenu)/sizeof(menuitem_t),
    60,40,
    0,
};


//======================================================================
// OpenGL specifics options
//======================================================================

#ifdef HWRENDER

static void M_DrawOpenGLMenu(void);
static void M_OGL_DrawFogMenu(void);
static void M_OGL_DrawColorMenu(void);
static void M_HandleFogColor (int choice);

menu_t OGL_LightingDef, OGL_FogDef, OGL_ColorDef, OGL_DevDef;

#define QUALITY_ITEM   2
menuitem_t OpenGLOptionsMenu[]=
{
    {IT_STRING | IT_CVAR,0, "Mouse look"          , &cv_grmlook_extends_fov ,  0},
    {IT_STRING | IT_CVAR | IT_YOFFSET, 0, "Field of view"       , &cv_grfov             , 10},
    {IT_STRING | IT_CVAR | IT_YOFFSET, 0, "Quality"             , &cv_scr_depth         , 20},
    {IT_STRING | IT_CVAR | IT_YOFFSET, 0, "Texture Filter"      , &cv_grfiltermode      , 30},
    {IT_STRING | IT_CVAR | IT_CV_SLIDER | IT_YOFFSET, 0, "Translucent HUD", &cv_grtranslucenthud  , 40},

    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0, "Lighting >>"       , &OGL_LightingDef   , 65},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0, "Fog >>"            , &OGL_FogDef        , 75},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0, "Gamma >>"          , &OGL_ColorDef      , 85},
    {IT_SUBMENU | IT_WHITESTRING | IT_YOFFSET, 0, "Development >>"    , &OGL_DevDef        , 95},
};

menuitem_t OGL_LightingMenu[]=
{
    {IT_STRING | IT_CVAR, 0, "Corona Draw"         , &cv_grcorona_draw ,  0},
    {IT_STRING | IT_CVAR, 0, "Dynamic lighting"    , &cv_grdynamiclighting,  0},
    {IT_STRING | IT_CVAR, 0, "Static lighting"     , &cv_grstaticlighting,  0},
    {IT_STRING | IT_CVAR, 0, "Monster ball light"  , &cv_monball_light,  0},
};

#define FOG_COLOR_ITEM  1
menuitem_t OGL_FogMenu[]=
{
    {IT_STRING | IT_CVAR | IT_YOFFSET, 0,"Fog"             , &cv_grfog              ,  0},
    {IT_STRING | IT_KEYHANDLER| IT_YOFFSET, 0, "Fog color" , M_HandleFogColor       , 10},
    {IT_STRING | IT_CVAR | IT_YOFFSET, 0,"Fog density"     , &cv_grfogdensity       , 20},
};                                         

menuitem_t OGL_ColorMenu[]=
{
    //{IT_STRING | NOTHING, "Gamma correction", NULL                   ,  0},
    {IT_STRING | IT_CVAR | IT_CV_SLIDER | IT_YOFFSET, 0,"red"  , &cv_grgammared     , 10},
    {IT_STRING | IT_CVAR | IT_CV_SLIDER | IT_YOFFSET, 0,"green", &cv_grgammagreen   , 20},
    {IT_STRING | IT_CVAR | IT_CV_SLIDER | IT_YOFFSET, 0,"blue" , &cv_grgammablue    , 30},
    //{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Constrast", &cv_grcontrast , 50},
};

menuitem_t OGL_DevMenu[]=
{
//    {IT_STRING | IT_CVAR, "Polygon smooth"  , &cv_grpolygonsmooth    ,  0},
    {IT_STRING | IT_CVAR, 0, "MD2 models"      , &cv_grmd2              , 10},
#ifdef TRANSWALL_CHOICE
    {IT_STRING | IT_CVAR, 0, "Translucent walls", &cv_grtranswall       , 20},
#endif
    {IT_STRING | IT_CVAR, 0, "Polygon shape"  , &cv_grpolyshape         , 30},
};

menu_t  OpenGLOptionDef =
{
    "M_OPTTTL",
    "OPTIONS",
    OpenGLOptionsMenu,
    M_DrawOpenGLMenu,
    NULL,
    sizeof(OpenGLOptionsMenu)/sizeof(menuitem_t),
    60,40,
    0
};

menu_t  OGL_LightingDef =
{
    "M_OPTTTL",
    "OPTIONS",
    OGL_LightingMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(OGL_LightingMenu)/sizeof(menuitem_t),
    60,40,
    0,
};

menu_t  OGL_FogDef =
{
    "M_OPTTTL",
    "OPTIONS",
    OGL_FogMenu,
    M_OGL_DrawFogMenu,
    NULL,
    sizeof(OGL_FogMenu)/sizeof(menuitem_t),
    60,40,
    0,
};

menu_t  OGL_ColorDef =
{
    "M_OPTTTL",
    "OPTIONS",
    OGL_ColorMenu,
    M_OGL_DrawColorMenu,
    NULL,
    sizeof(OGL_ColorMenu)/sizeof(menuitem_t),
    60,40,
    0,
};

menu_t  OGL_DevDef =
{
    "M_OPTTTL",
    "OPTIONS",
    OGL_DevMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(OGL_DevMenu)/sizeof(menuitem_t),
    60,40,
    0,
};


//======================================================================
// M_DrawOpenGLMenu()
//======================================================================
static
void M_DrawOpenGLMenu(void)
{
    int             mx,my;

    mx = OpenGLOptionDef.x;
    my = OpenGLOptionDef.y;
    M_DrawGenericMenu(); // use generic drawer for cursor, items and title
    // Draw the number in the Quality menu item.
    V_DrawString(BASEVIDWIDTH-mx-V_StringWidth(cv_scr_depth.string),
                 my+currentMenu->menuitems[QUALITY_ITEM].alphaKey,
                 V_WHITEMAP,
                 cv_scr_depth.string);
}


//======================================================================
// M_OGL_DrawFogMenu()
//======================================================================
static
void M_OGL_DrawFogMenu(void)
{
    int             mx,my;

    mx = OGL_FogDef.x;
    my = OGL_FogDef.y;
    M_DrawGenericMenu(); // use generic drawer for cursor, items and title
    // Draw the fog color number in the menu.
    V_DrawString(BASEVIDWIDTH-mx-V_StringWidth (cv_grfogcolor.string),
                 my+currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey,
                 V_WHITEMAP,
                 cv_grfogcolor.string);
    if (itemOn==FOG_COLOR_ITEM && skullAnimCounter<4) //blink cursor on FOG_COLOR_ITEM if selected
        V_DrawCharacter( BASEVIDWIDTH-mx, my+currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, '_' | 0x80);  // white
}


//======================================================================
// M_OGL_DrawColorMenu()
//======================================================================
static
void M_OGL_DrawColorMenu(void)
{
    int             mx,my;

    mx = OGL_ColorDef.x;
    my = OGL_ColorDef.y;
    M_DrawGenericMenu(); // use generic drawer for cursor, items and title
    V_DrawString(mx, my+currentMenu->menuitems[0].alphaKey-10,
                 V_WHITEMAP,"Gamma correction");
}


//======================================================================
// M_OpenGLOption()
//======================================================================
static
void M_OpenGLOption(int choice)
{
    if( rendermode != render_soft )
        Push_Setup_Menu(&OpenGLOptionDef);
    else
        M_SimpleMessage("You are in software mode\nYou cannot change GL options\n");
}


//======================================================================
// M_HandleFogColor()
//======================================================================
static
void M_HandleFogColor(int key)
{
    int      i, l;
    char     temp[8];
    boolean  exitmenu = false;  // exit to previous menu and send name change

    switch( key )
    {
      case KEY_DOWNARROW:
        S_StartSound(menu_sfx_updown);
        itemOn++;
        break;

      case KEY_UPARROW:
        S_StartSound(menu_sfx_updown);
        itemOn--;
        break;

      case KEY_ESCAPE:
        S_StartSound(menu_sfx_esc);
        exitmenu = true;
        break;

      case KEY_BACKSPACE:
        S_StartSound(menu_sfx_val);
        strcpy(temp, cv_grfogcolor.string);
        strcpy(cv_grfogcolor.string, "000000");
        l = strlen(temp)-1;
        for (i=0; i<l; i++)
            cv_grfogcolor.string[i+6-l] = temp[i];
        break;

      default:
        input_char = tolower(input_char);
        if ((input_char >= '0' && input_char <= '9') ||
            (input_char >= 'a' && input_char <= 'f'))
        {
            S_StartSound(menu_sfx_val);
            strcpy(temp, cv_grfogcolor.string);
            strcpy(cv_grfogcolor.string, "000000");
            l = strlen(temp);
            for (i=0; i<l; i++)
                cv_grfogcolor.string[5-i] = temp[l-i];
            cv_grfogcolor.string[5] = input_char;
        }
        break;
    }
    if (exitmenu)
    {
        Pop_Menu();
    }
}

#endif

//===========================================================================
// Register Menu cv_ commands that do not have another Init to use.

#ifdef SDL2
consvar_t cv_sdl2_textchar    = {"sdl2_textchar","1",CV_SAVE,CV_OnOff};
#endif

static
consvar_t * menu_command_cvar_list[] =
{
    // Any cv_ with CV_SAVE needs to be registered, even if it is not used.
    // Otherwise there will be error messages when config is loaded.

    // Player1
  &cv_autorun[0],
  &cv_crosshair[0],

    // Player2
  &cv_autorun[1],
  &cv_crosshair[1],

// &cv_crosshairscale, // doesn't work for now
  &cv_showmessages,
// &cv_showmessages2,
// &cv_controlperkey2,

  &cv_screenslink,
  &cv_twoplayer,        // [Arcade]
  &cv_localplayers,     // [Arcade]
  &cv_jointime,         // [Arcade]
  &cv_defaultgame,      // [Arcade]

    // p_mobj.c
  &cv_itemrespawntime,
  &cv_itemrespawn,
  &cv_respawnmonsters,
  &cv_respawnmonsterstime,
  &cv_fastmonsters,
  &cv_predictingmonsters,     //added by AC for predmonsters
  &cv_splats,
  &cv_maxsplats,

#ifdef MAPTHING_ADJUST
# ifdef MAPTHING_ADJUST_MASTER
  &cv_mapthing_adjust_master,  // DO NOT USE
# endif
  &cv_monster_health,
  &cv_health_pickup,
  &cv_armor_pickup,
  &cv_ammo_pickup,
#endif

    // [WDJ] 2/7/2011 Voodoo
  &cv_instadeath,
  &cv_voodoo_mode,
  &cv_pickupflash,
  &cv_weapon_recoil,
  &cv_zerotags,
#ifdef GENERATE_BLOCKMAP
  &cv_blockmap_gen,
#endif
#ifdef ENABLE_TIRED_RUN
  &cv_tired_run,
  &cv_drown,
#endif
#ifdef ENABLE_TELE_CONTROL
  &cv_tele_control,
#endif
    // misc
  &cv_deathmatch,  // after cv_itemrespawn
  &cv_teamplay,
  &cv_teamdamage,
  &cv_timelimit,
  &cv_fraglimit,

    // d_clisrv
  &cv_playdemospeed,
  &cv_server1,
  &cv_server2,
  &cv_server3,

    // p_inter
  &cv_fragsweaponfalling,

    // g_game
  &cv_allowjump,
  &cv_allowrocketjump,
  &cv_allowautoaim,
  &cv_allowturbo,
  &cv_allowmlook,
  &cv_allowexitlevel,
  &cv_idletimeout,
  &cv_idlewarntime,

    // g_input.c
  &cv_grabinput,
    // WARNING : the order is important when init mouse
    // Call of mouse1 init occurs with Register cv_usemouse[1].
#ifdef SMIF_SDL
  &cv_mouse_motion,
#endif
    // Call of mouse1 init occurs here.
  &cv_usemouse[0],
  &cv_alwaysfreelook[0],
  &cv_mouse_move[0],
  &cv_mouse_invert,
  &cv_mouse_sens_x,
  &cv_mouse_sens_y,

    // WARNING : the order is important when init mouse2 
#ifdef MOUSE2
#if defined( MOUSE2_NIX ) || defined( MOUSE2_WIN ) || defined( MOUSE2_DOS )
    // Call of mouse2 init occurs with Register cv_usemouse[1].
#if defined( SMIF_SDL ) || defined( SMIF_WIN32 ) || defined( SMIF_X11 )
  &cv_mouse2type,
#endif
  &cv_mouse2port,
  &cv_mouse2opt,
#endif
#endif

    // Call of mouse2 init occurs here.
  &cv_usemouse[1],
  &cv_alwaysfreelook[1],
  &cv_mouse_move[1],
#ifdef MOUSE2
  &cv_mouse2_invert,
  &cv_mouse2_sens_x,
  &cv_mouse2_sens_y,
#endif

  &cv_mouse_double,
#ifdef JOY_BUTTONS_DOUBLE
  &cv_joy_double,
#endif
  &cv_joy_deadzone,

#ifdef SDL2
  &cv_sdl2_textchar,
#endif

  &cv_controlperkey,
  &cv_controlscheme[0],   // [Arcade]
  &cv_controlscheme[1],
  &cv_customcontrols[0],  // [Arcade] guided setup key table
  &cv_customcontrols[1],

    // s_sound.c
  &cv_soundvolume,
  &cv_musicvolume,
  &cv_numChannels,
  &cv_rndsoundpitch,

#ifdef CDMUS
    // i_cdmus.c
  &cd_volume,
#endif

    // p_lights.c
  &cv_corona,
  &cv_coronasize,
  &cv_corona_draw_mode,
// &cv_dynamiclighting,
// &cv_staticlighting,
  &cv_monball_light,

    // bots
  &cv_bots,
  &cv_bot_skill,
  &cv_bot_speed,
  &cv_bot_skin,
  &cv_bot_respawn_time,
  &cv_bot_random,
  &cv_bot_randseed,
  &cv_bot_gen,
  &cv_bot_grab,

    // p_map.c
  &cv_oof_2s,

  NULL
};

extern consvar_t * enemy_cvar_list[];

void M_Register_Menu_Controls( void )
{
    CV_RegisterVar_list( fab_cvar_list ); // [WDJ] more than just DeathMatch
    CV_RegisterVar_list( menu_command_cvar_list );
    CV_RegisterVar_list( enemy_cvar_list );
}


#ifdef LAUNCHER
//===========================================================================
//                        LAUNCH MENU
//===========================================================================

// cv_ menu items
// out only
consvar_t cv_switch = {"switches", "", CV_HIDEN, NULL};
consvar_t cv_config = {"config", "", CV_HIDEN, NULL};

static void CV_game_OnChange(void);
CV_PossibleValue_t game_cons_t[] = {{-1,"MIN"},{64,"MAX"},{0,NULL}};
consvar_t cv_game = {"game", "-1", CV_HIDEN|CV_CALL, NULL, CV_game_OnChange};

consvar_t * launch_cvar_list[] =
{
// &cv_iwad),
  &cv_config,
  &cv_switch,
  &cv_game,
  NULL
};

static
void CV_game_OnChange(void)
{
    // Strings come from GameDesc, not a CV_PossibleValue_t
    // Cannot call CV_Set within CV_CALL routine
    const char * rs;
    game_desc_t * gamedesc = D_GameDesc( cv_game.value );
    if( gamedesc )
    {
        rs = gamedesc->gname;  // const string from table
    }
    else
    {
        rs = "Auto";
        cv_game.value = (cv_game.value <1)? -1 : GDESC_other;
    }
    // Update display string from GameDesc.
    if( cv_game.string )
       Z_Free( cv_game.string );  // remove the string from CV_Set
    cv_game.string = Z_StrDup( rs );
    return;
}

static
void M_LaunchCont( void )
{
    init_sequence = 2;
    M_Clear_Menus(true);
}

menuitem_t LaunchMenu[]=
{
    {IT_WHITESTRING | IT_CVAR | IT_CV_STRING, NULL, "Home", &cv_home,  0},
    {IT_WHITESTRING | IT_CVAR | IT_CV_STRING, NULL, "Doomwaddir", &cv_doomwaddir, 0},
    {IT_WHITESTRING | IT_CVAR | IT_CV_STRING, NULL, "Config", &cv_config,  0},
    {IT_WHITESTRING | IT_CVAR | IT_CV_STRING, NULL, "Switch", &cv_switch, 0},
    {IT_WHITESTRING | IT_CVAR | IT_CV_STRING, NULL, "Iwad", &cv_iwad, 0},
 // the CVAR strings will intercept the 'g', 'c', and 'q'
    {IT_WHITESTRING | IT_CVAR, NULL, "Game", &cv_game,  'g'},
    {IT_WHITESTRING | IT_CALL, NULL, "Continue", M_LaunchCont, 'c'},
    {IT_WHITESTRING | IT_CALL, NULL, "QUIT GAME", M_QuitDOOM, 'q'}
};

menu_t  LaunchDef =
{
    NULL,
    "Launch",
    LaunchMenu,
    M_DrawGenericMenu,
    NULL,
    sizeof(LaunchMenu)/sizeof(menuitem_t),
    10,10,  // x, y  (cursor at x-10)
    0
};

// call only after memory, video, command, and menu inits
void M_LaunchMenu( void )
{
    M_StartControlPanel();
    Push_Setup_Menu (&LaunchDef);

    M_Clear_Add_Param();  // clear previous param from this routine
    if( cv_game.string == NULL ) // first time inits
    {
        CV_RegisterVar_list( launch_cvar_list );
    }
    skullAnimCounter = 0;

    do
    {
        V_Clear_Display();
        I_OsPolling();
        D_Process_Events ();  // menu responder
        M_Drawer(); // menu drawer
        I_UpdateNoBlit();
        I_FinishUpdate();       // page flip or blit buffer
    } while( menuactive );

    // add home
    if(cv_home.state & CS_MODIFIED)
        M_Change_2Param( "-home", cv_home.string );
    // add doomwaddir
    if( (cv_doomwaddir.state & CS_MODIFIED) && cv_doomwaddir.string )
        doomwaddir[0] = cv_doomwaddir.string;
    // add config
    if(cv_config.state & CS_MODIFIED)
        M_Change_2Param( "-config", cv_config.string );
    // add iwad
    if(cv_iwad.state & CS_MODIFIED)
        M_Change_2Param( "-iwad", cv_iwad.string );
    // add switches
    if( (cv_switch.state & CS_MODIFIED) && cv_switch.string[0] )
        M_Add_Param( cv_switch.string, NULL );
    // add game
    if( (cv_game.state & CS_MODIFIED)
        && cv_game.value >= 0 && cv_game.value < GDESC_other)
    {
        game_desc_t * gamedesc = D_GameDesc( cv_game.value );
        if( gamedesc )
            M_Add_Param( "-game", gamedesc->idstr );
    }
}

#endif
