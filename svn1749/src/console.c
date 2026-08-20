// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: console.c 1711 2025-01-17 04:17:06Z wesleyjohnson $
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
// $Log: console.c,v $
// Revision 1.23  2003/08/11 13:50:03  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.22  2003/05/04 02:27:49  sburke
// Fix for big-endian machines.
//
// Revision 1.21  2002/09/10 19:29:46  hurdler
// Add log file under Linux
//
// Revision 1.20  2002/08/25 14:59:32  hurdler
//
// Revision 1.19  2002/07/23 15:07:09  mysterial
// Messages to second player appear on his half of the screen
//
// Revision 1.18  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.17  2001/08/20 20:40:39  metzgermeister
// Revision 1.16  2001/05/16 21:21:14  bpereira
//
// Revision 1.15  2001/03/03 19:41:22  ydario
// I_OutputMsg not implemented in OS/2
//
// Revision 1.14  2001/03/03 06:17:33  bpereira
// Revision 1.13  2001/02/24 13:35:19  bpereira
//
// Revision 1.12  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.11  2000/11/12 09:48:15  bpereira
//
// Revision 1.10  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.9  2000/09/28 20:57:14  bpereira
// Revision 1.8  2000/08/31 14:30:55  bpereira
//
// Revision 1.7  2000/08/10 15:01:06  ydario
// OS/2 port
//
// Revision 1.6  2000/08/03 17:57:41  bpereira
//
// Revision 1.5  2000/04/24 15:10:56  hurdler
// Support colormap for text
//
// Revision 1.4  2000/04/16 18:38:06  bpereira
//
// Revision 1.3  2000/04/07 23:09:12  metzgermeister
// fixed array boundary error
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      console for Doom LEGACY
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "console.h"
#include "g_game.h"
#include "g_input.h"
  // keys.h, gc_console
#include "hu_stuff.h"
#include "s_sound.h"
  // sounds.h, S_StartSound
#include "v_video.h"
#include "i_video.h"
#include "i_system.h"
  // I_OutputMessage
#include "z_zone.h"
#include "d_main.h"
#include "d_clisrv.h"   // D_NumViews, for the shared-screen message rule

//#include <unistd.h>

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#define CONSOLE_PROPORTIONAL

// External control
boolean  con_self_refresh=false;  // true at game startup, screen need refreshing
boolean  con_recalc;     // set true when screen size has changed

// Internal state
static boolean  con_started=false;  // console has been initialised
static boolean  con_video=false;  // text mode until video started
static boolean  con_forcepic=true;  // at startup toggle console translucency when
                             // first off

static int  con_tick;    // console ticker for anim or blinking prompt cursor
                         // con_scrollup should use time (currenttime - lasttime)..

static boolean  consoletoggle;  // true when console key pushed, ticker will handle
static boolean  console_ready;  // console prompt is ready
boolean  console_open = false;  // console is open

int      con_destlines;  // vid lines used by console at final position
static int  con_curlines;  // vid lines currently used by console

// Clip value for planes & sprites, so that the part of the view covered by the
// console is not drawn.
// It is set to the first drawable line under the console, and 0 when console is off.
int      con_clipviewtop;

// TODO: choose max hud msg lines
#define  CON_MAXHUDLINES      5

// Global interface with hu_stuff.
int      con_clearlines; // top screen lines to refresh when view reduced
boolean  con_hudupdate;  // when messages scroll, we need a backgrnd refresh

// Internal state.
static int  con_hudlines;  // number of console heads up message lines
static int  con_hudtime[CON_MAXHUDLINES];  // remaining time of display for hud msg lines

// To support splitscreen, 0=upper, 1=lower, 5=console only
static byte  con_viewnum[CON_MAXHUDLINES];


// console text output
static char* con_line;   // console text output current line
static int  con_cx;      // cursor position in current line
static int  con_cy;      // cursor line number in con_buffer, is always
                         //  increasing, and wrapped around in the text
                         //  buffer using modulo.

static int  con_totallines; // lines of console text into the console buffer
static int  con_width;      // columns of chars, depend on vid mode width
static int  con_indent;     // pixel indent of console

static int  con_scrollup;   // how many rows of text to scroll up (pgup/pgdn)


#define  CON_PROMPTCHAR        '>'

// Hold last CON_MAX_LINEHIST prompt lines.
// [WDJ] Power 2 only, due to INDEXMASK.
#define  CON_MAX_LINEHIST    32
#define  CON_MAX_LINEHIST_INDEXMASK  (CON_MAX_LINEHIST-1)
#define  CON_MAX_LINELEN    256

// First char is prompt.
static char inputlines[CON_MAX_LINEHIST][CON_MAX_LINELEN];

static int  inputline;  // current input line number
static int  inputhist;  // line number of history input line to restore
static int  input_cx;   // position in current input line

static pic_t*  con_backpic;  // console background picture, loaded static
static pic_t*  con_bordleft;
static pic_t*  con_bordright;  // console borders in translucent mode


// protos.
static void CON_Init_Input (void);
static void CON_Print (byte control, char * msg);
static void CONS_Clear_f (void);
static void CON_RecalcSize ( int width );

static void CONS_speed_Change (void);
static void CON_Draw_Backpic (pic_t * pic, int startx, int destwidth);


//======================================================================
//                   CONSOLE VARS AND COMMANDS
//======================================================================
#if defined( MACOS_DI ) && ! defined( __GNUC__ )
#define  CON_BUFFERSIZE   4096  //my compiler cant handle local vars >32k
#else
#define  CON_BUFFERSIZE   16384
#endif

static char  con_buffer[CON_BUFFERSIZE];


// how many seconds the hud messages lasts on the screen
consvar_t   cons_msgtimeout = {"con_hudtime","5",CV_VALUE|CV_SAVE,CV_Unsigned};

// number of lines console move per frame
consvar_t   cons_speed = {"con_speed","8",CV_VALUE|CV_CALL|CV_SAVE,CV_Unsigned,&CONS_speed_Change};

// percentage of screen height to use for console
consvar_t   cons_height = {"con_height","50",CV_SAVE,CV_Unsigned};

CV_PossibleValue_t backpic_cons_t[]={{0,"translucent"},{1,"picture"},{0,NULL}};
// whether to use console background picture, or translucent mode
consvar_t   cons_backpic = {"con_backpic","0",CV_SAVE,backpic_cons_t};


//  Check CONS_speed value (must be positive and >0)
//
static void CONS_speed_Change (void)
{
    if (cons_speed.value<1)
        CV_SetValue (&cons_speed,1);
}


//  Clear console text buffer
//
static void CONS_Clear_f (void)
{
    memset(con_buffer,0,CON_BUFFERSIZE);

    con_cx = 0;
    con_cy = con_totallines-1;
    con_line = &con_buffer[con_cy*con_width];
    con_scrollup = 0;
}

// Keys defined by the BIND command.
static char * bindtable[NUMINPUTS];

static void CONS_Bind_f(void)
{
    int  key;
    COM_args_t  carg;
    
    COM_Args( &carg );

    if ( carg.num!=2 && carg.num!=3 )
    {
        int nb = 0;
        CONS_Printf("bind <keyname> [<command>]\n");
        CONS_Printf("\2bind table :\n");
        for(key=0;key<NUMINPUTS;key++)
        {
            if(bindtable[key])
            {
                CONS_Printf("%s : \"%s\"\n",G_KeynumToString (key),bindtable[key]);
                nb=1;
            }
        }
        if(!nb)
            CONS_Printf("Empty\n");
        return;
    }

    key=G_KeyStringtoNum( carg.arg[1] );
    if(!key)
    {
        CONS_Printf("Invalid key name\n");
        return;
    }

    if(bindtable[key]!=NULL)
    {
        Z_Free(bindtable[key]);
        bindtable[key]=NULL;
    }

    if( carg.num==3 )
        bindtable[key]=Z_StrDup( carg.arg[2] );
}


//======================================================================
//                          CONSOLE SETUP
//======================================================================

// Prepare a colormap for GREEN ONLY translucency over background
//
byte*   whitemap = NULL;
byte*   greenmap;
byte*   graymap;

// May be called again after command_restart
static void CON_SetupBackColormap (void)
{
    int   i,j,k;
    byte* pal;

//
//  setup the green translucent background colormap
//
    if( ! whitemap )
    {
        //  setup the green translucent background colormap
        greenmap = (byte *) Z_Malloc(256,PU_STATIC,NULL);
        whitemap = (byte *) Z_Malloc(256,PU_STATIC,NULL);
        graymap  = (byte *) Z_Malloc(256,PU_STATIC,NULL);
    }

    // wad containing PLAYPAL may not be found yet.
    if( ! VALID_LUMP( W_CheckNumForName( "PLAYPAL" ) ) )
        return;

    pal = W_CacheLumpName ("PLAYPAL",PU_CACHE); // temp, only used next loop

    for(i=0,k=0; i<768; i+=3,k++)
    {
        j = pal[i] + pal[i+1] + pal[i+2];

        if( EN_heretic )
        {
            greenmap[k] = 209 + (float)j*15/(3*255);   //remaps to greens(209-224)
            graymap[k]  =       (float)j*35/(3*255);   //remaps to grays(0-35)           
            whitemap[k] = 145 + (float)j*15/(3*255);   //remaps to reds(145-168)
        }
        else
            greenmap[k] = 127 - (j>>6);
    }

//
//  setup the white and gray text colormap
//
    // this one doesn't need to be aligned, unless you convert the
    // V_DrawMappedPatch() into optimised asm.

    if( EN_doom_etc )
    {
        for(i=0; i<256; i++)
        {
            whitemap[i] = i;        //remap each color to itself...
            graymap[i]  = i;
        }

        for(i=168;i<192;i++)
        {
            whitemap[i]=i-88;     //remaps reds(168-192) to whites(80-104)
            graymap[i]=i-80;      //remaps reds(168-192) to gray(88-...)
        }
        whitemap[45]=190-88; // the color[45]=color[190] !
        graymap [45]=190-80;
        whitemap[47]=191-88; // the color[47]=color[191] !
        graymap [47]=191-80;
    }
}


//  Setup the console text buffer
//
// Init messaging, before zone memory, before video
// CON buffer will save all messages for display, so must be started very early
void CON_Init_Setup(void)
{
    int i;

    // clear all lines
    con_width = 0;  // no current text
    CON_RecalcSize ( INITIAL_WINDOW_WIDTH );  // before vid is set
    CONS_Clear_f ();   // clear all lines
    con_destlines = INITIAL_WINDOW_HEIGHT;
    con_curlines = INITIAL_WINDOW_HEIGHT;

    con_hudlines = CON_MAXHUDLINES;
    CON_Clear_HUD ();

    // setup console input filtering
    CON_Init_Input ();

    for(i=0;i<NUMINPUTS;i++)
        bindtable[i]=NULL;

    consoletoggle = false;
    con_started = true;
}

// after zone memory init
void CON_Register(void)
{
    // register our commands
    CV_RegisterVar (&cons_msgtimeout);
    CV_RegisterVar (&cons_speed);
    CV_RegisterVar (&cons_height);
    CV_RegisterVar (&cons_backpic);
    COM_AddCommand ("cls", CONS_Clear_f, CC_console);
    COM_AddCommand ("bind", CONS_Bind_f, CC_console);
}

// after FullGraphics
void CON_Init_Video(void)
{
    // vid : from video setup
    if(dedicated)
        return;
    
    // make sure it is ready for the loading screen
    CON_RecalcSize ( vid.width );

    CON_SetupBackColormap ();

    //note: CON_Ticker should always execute at least once before D_Display()
    con_clipviewtop = 0;  // does not clip

    // load console background pic
    con_backpic = (pic_t*) W_CachePicName ("CONSBACK",PU_STATIC);

    // borders MUST be there
    con_bordleft  = (pic_t*) W_CachePicName ("CBLEFT",PU_STATIC);
    con_bordright = (pic_t*) W_CachePicName ("CBRIGHT",PU_STATIC);

    // set console full screen for game startup after FullGraphics
    con_destlines = vid.height;
    con_curlines = vid.height;

    con_self_refresh = true; // need explicit screen refresh
                        // until we are in Doomloop
    con_video = true;   // if move CON init to before video startup
}


//  Console input initialization
//
static void CON_Init_Input (void)
{
    int    i;

    // prepare the first prompt line
    memset (inputlines,0,sizeof(inputlines));
    for (i=0; i<CON_MAX_LINEHIST; i++)
        inputlines[i][0] = CON_PROMPTCHAR;
    inputline = 0;
    input_cx = 1;
}



//======================================================================
//                        CONSOLE EXECUTION
//======================================================================

// [WDJ] This originally was (BASEVIDWIDTH>>3)-2, but that gives 38,
// and the LARGE font overruns the right margin at 30.
#define MIN_CONWIDTH  32
#define MAX_CONWIDTH  200

//  Called at screen size change to set the rows and line size of the
//  console text buffer.
//
static void CON_RecalcSize ( int width )
{
    int   new_conwidth, oldcon_width, oldnumlines, oldcon_cy;
    int   i, conw;
    char  con2[CON_BUFFERSIZE];
    char  line2[MAX_CONWIDTH+4]; // BP: it is a line but who know ([WDJ] 30..94)

    con_recalc = false;

    // Calculate the new con_width.
    if( width != vid.width )
    {
        // Before vid setup, so without any vid information
        con_indent = 10;
        if( width > 630 )
           new_conwidth = (width>>4) - 2;
        else
           new_conwidth = (width>>3) - 2;
    }
    else
    {
        // Have graphics and vid information.
        con_indent = vid.dupx * 10;  // indent of console text to avoid edge
        V_SetupFont( cv_con_fontsize.value, NULL, 0 );
#ifdef CONSOLE_PROPORTIONAL
        // Averages shorter text, but could overrun right margin.
        // Proportional width of print is (hu_font[c].width * dupx + 1).
        // As drawfont.xinc is fixed, and some characters are wider ('M', 'W'),
        // it is still possible to overrun the right margin.
        new_conwidth = (width - con_indent - con_indent) / (drawfont.xinc + 1);
#else
        // Fixed font size, between left and right margin.
        new_conwidth = (width - con_indent - con_indent) / drawfont.xinc;
#endif
    }
    if ( new_conwidth < MIN_CONWIDTH )
    {
        con_indent -= MIN_CONWIDTH - new_conwidth;
        if( con_indent < 2 )  con_indent = 2; 
        new_conwidth = MIN_CONWIDTH;
    }
    if (new_conwidth > MAX_CONWIDTH)
        new_conwidth = MAX_CONWIDTH;

    // check for change of video width
    if (new_conwidth == con_width)
        return;                 // didnt change


    // save current
    oldcon_width = con_width;
    oldnumlines = con_totallines;
    oldcon_cy = con_cy;
    memcpy(con2, con_buffer, CON_BUFFERSIZE);

    // setup to new width
    con_width = new_conwidth;
    con_totallines = CON_BUFFERSIZE / con_width;
    CONS_Clear_f ();

    // re-arrange console text buffer to keep text
    if(oldcon_width) // not the first time
    {
        for(i=oldcon_cy+1; i<oldcon_cy+oldnumlines; i++)
        {
            char * con2p = &con2[(i% oldnumlines)*oldcon_width];
            if( *con2p )
            {
                memcpy(line2, con2p, oldcon_width);
                conw=oldcon_width-1;
                while(line2[conw]==' ' && conw) conw--;
                line2[conw+1]='\n';
                line2[conw+2]='\0';
                CON_Print( 5, line2);  // console only
            }
        }
    }
}


// Handles Console moves in/out of screen (per frame)
//
static void CON_MoveConsole (void)
{
    // up/down move to dest
    if (con_curlines < con_destlines)
    {
        con_curlines+=cons_speed.value;
        if (con_curlines > con_destlines)
           con_curlines = con_destlines;
    }
    else if (con_curlines > con_destlines)
    {
        con_curlines-=cons_speed.value;
        if (con_curlines < con_destlines)
            con_curlines = con_destlines;
    }

}


//  Clear time of console heads up messages
//
void CON_Clear_HUD (void)
{
    int    i;

    for(i=0; i<con_hudlines; i++)
        con_hudtime[i]=0;
}


// Force console to move out immediately
// note: con_ticker will set console_ready false
void CON_ToggleOff (void)
{
    if (!con_destlines)
        return;

    con_destlines = 0;
    con_curlines = 0;
    CON_Clear_HUD ();
    con_forcepic = 0;
    con_clipviewtop = 0;  // turn off console clip
    console_open = false;  // instant off
}


static event_t  con_autorepeat_ev;
static byte     con_autorepeat_tick = 0;


//  Console ticker : handles console move in/out, cursor blinking
//
// Call once per tic.
void CON_Ticker (void)
{
    // vid : from video setup
    int    i;

    // cursor blinking
    con_tick++;
    con_tick &= 7;

    // console key was pushed
    if (consoletoggle)
    {
        consoletoggle = false;

        if (con_destlines > 0)
        {
            // toggle off console
            con_destlines = 0;
            CON_Clear_HUD ();
        }
        else
        {
            // toggle console in
            con_destlines = (cons_height.value*vid.height)/100;
            if (con_destlines < 20)
                con_destlines = 20;
            else
            if (con_destlines > (vid.height - stbar_height) )
                con_destlines = vid.height - stbar_height;

            con_destlines &= ~0x3;      // multiple of text row height
        }
    }

    // console movement
    if (con_destlines!=con_curlines)
        CON_MoveConsole ();  // update con_curlines


    // clip the view, so that the part under the console is not drawn
    con_clipviewtop = 0;
    if (cons_backpic.value)   // clip only when using an opaque background
    {
        if (con_curlines > 0)
        {
            // [WDJ] Set to highest drawable line under console.	   
//            con_clipviewtop = con_curlines;  // [WDJ] what it should be
            con_clipviewtop = con_curlines - view_window_y - 10;
//NOTE: BIG HACK::SUBTRACT 10, SO THAT WATER DON'T COPY LINES OF THE CONSOLE
//      WINDOW!!! (draw some more lines behind the bottom of the console)
        }

        if (con_clipviewtop<0)
            con_clipviewtop = 0;  // limit to drawable window
    }

    // check if console ready for prompt
//    if ((con_curlines==con_destlines) && (con_destlines>=20))
    console_ready = (con_destlines >= 20);

    // To detect console.
    console_open = ((con_destlines + con_curlines) != 0);

    // make overlay messages disappear after a while
    for (i=0 ; i<con_hudlines; i++)
    {
        con_hudtime[i]--;
        if (con_hudtime[i]<0)
            con_hudtime[i]=0;
    }

    if( con_autorepeat_tick )
    {
        con_autorepeat_tick --;
        if( con_autorepeat_tick == 1 )
            CON_Responder( & con_autorepeat_ev );
    }
}


//  Handles console key input
//
boolean CON_Responder(event_t * ev)
{
// sequential completions a la 4dos
static char    completion[80];
static int     comskips,varskips;

    const char * cmd = NULL;

    // [WDJ]  The compiler re-optimizes the returns.  Collecting them
    // into common return true, and return false, has no net effect.
    //  
    // Return true: eat the key
    //        false: reject the key
    
    if(chat_on)
        return false; 

#ifdef SDL2
    // [WDJ] A key will come as ev_keydown, ev_keyup, and ev_textchar events.
    // Depending on cv_sdl2_textchar,
    // either ev_keydown, or ev_textchar, may have valid ch, the other will have ch = 0.
    char con_ch = ev->data2;  // translated text
    if (ev->type == ev_textchar)
    {
        // key = data1 = STX, an unused key
    }
    else if (ev->type == ev_keydown)
    {
        // ch may be 0
    }
    else
#else      
    // SDL1, ev_keydown event has key and translated char.
    // interpret it as input char
    char con_ch = ev->data2;
    if (ev->type != ev_keydown)
#endif
    {
        // let go keyup events, don't eat them
        con_autorepeat_tick = 0;
        return false;
    }

    int key = ev->data1;

    // Detect console activate key (user definable).
    if (key == gamecontrol[gc_console][0] ||
        key == gamecontrol[gc_console][1] )   goto toggle_console;

    if (! console_ready)
    {
        // Console prompt not active.  This is the path during game play.
        // Check game playing keys defined by BIND command.
        // metzgermeister: boundary check !!
        if((key < NUMINPUTS) && bindtable[key])
        {
            // [WDJ] Must be done as one string, it could try to execute a partial string.
            COM_BufAddText ( va( "%s\n", bindtable[key] ) );
            return true;
        }
        return false;
    }

    if( key >= KEY_NUMKB )
        return false;  // mouse, joystick button

   
    // Console prompt active
    // [WDJ] Trying to use a switch stmt, increases the size for unknown
    // reasons related to optimization.  It uses extra tests to gain speed.
    // It optimizes the repeated tests of the key better than the switch.

    // eat shift only if console active
    if (key == KEY_RSHIFT || key == KEY_LSHIFT)
      return true;

    // escape key toggle off console
    if (key == KEY_ESCAPE)   goto toggle_console;

    // command completion forward (tab) and backward (shift-tab)
    if (key == KEY_TAB)
    {
        // TOTALLY UTTERLY UGLY NIGHT CODING BY FAB!!! :-)
        //
        // sequential command completion forward and backward

        // remember typing for several completions (…-la-4dos)
        if (inputlines[inputline][input_cx-1] != ' ')
        {
            if (strlen (inputlines[inputline]+1)<80)
                strcpy (completion, inputlines[inputline]+1);
            else
                completion[0] = 0;

            comskips = varskips = 0;
        }
        else
        {
            // comskips < 0  indicates var name completion
            if (shiftdown)
            {
                if (comskips<0)
                {
                    if (--varskips<0)
                        comskips = -(comskips+2);
                }
                else
                if (comskips>0)
                    comskips--;
            }
            else
            {
                if (comskips<0)
                    varskips++;
                else
                    comskips++;
            }
        }

        if (comskips>=0)
        {
            cmd = COM_CompleteCommand (completion, comskips);
            if (!cmd)
            {
                // No command, try var completion.
                // dirty:make sure if comskips is zero, to have a neg value
                comskips = -(comskips+1);
            }
        }
        if (comskips<0)
            cmd = CV_CompleteVar (completion, varskips);

        if (cmd)
        {
            memset(inputlines[inputline]+1,0,CON_MAX_LINELEN-1);
            strcpy (inputlines[inputline]+1, cmd);
            input_cx = strlen(cmd)+1;
            inputlines[inputline][input_cx] = ' ';
            input_cx++;
            inputlines[inputline][input_cx] = 0;
        }
        else
        {
            // No command, no var completion.  Backup off this candidate.
            if (comskips>0)
                comskips--;
            else
            if (varskips>0)
                varskips--;
        }

        return true;
    }

    // move up (backward) in console textbuffer
    if (key == KEY_PGUP)
    {
        if (con_scrollup < (con_totallines-((con_curlines-16)>>3)) )
            con_scrollup++;
        goto enable_autorepeat;
    }
    if (key == KEY_PGDN)
    {
        if (con_scrollup>0)
            con_scrollup--;
        goto enable_autorepeat;
    }

    // oldset text in buffer
    if (key == KEY_HOME)
    {
        con_scrollup = (con_totallines-((con_curlines-16)>>3));
        return true;
    }
    // most recent text in buffer
    if (key == KEY_END)
    {
        con_scrollup = 0;
        return true;
    }

    // command enter
    if (key == KEY_ENTER)
    {
        if (input_cx<2)
            return true;  // nothing significant

        // push the command
        // [WDJ] Must be done as one string, it could try to execute a partial string.
        // The first char is prompt, not part of the command.
        COM_BufAddText ( va( "%s\n", inputlines[inputline]+1 ));

        CONS_Printf("%s\n",inputlines[inputline]);

        // Advance to next inputline.
        inputline = (inputline+1) & CON_MAX_LINEHIST_INDEXMASK;
        inputhist = inputline;

        memset(inputlines[inputline],0,CON_MAX_LINELEN);
        inputlines[inputline][0] = CON_PROMPTCHAR;
        input_cx = 1;

        return true;
    }

    // backspace command prompt
    if (key == KEY_BACKSPACE)
    {
        if (input_cx>1)  // back to prompt
        {
            input_cx--;
            inputlines[inputline][input_cx] = 0;
        }
        return true;
    }

    // move back in input history
    if (key == KEY_UPARROW)
    {
        // copy one of the previous inputlines to the current
        do{
            inputhist = (inputhist - 1) & CON_MAX_LINEHIST_INDEXMASK; // cycle back
        }while (inputhist!=inputline && !inputlines[inputhist][1]);

        // stop at the last history input line, which is the
        // current line + 1 because we cycle through the 32 input lines
        if (inputhist==inputline)
            inputhist = (inputline + 1) & CON_MAX_LINEHIST_INDEXMASK;

        memcpy (inputlines[inputline],inputlines[inputhist],CON_MAX_LINELEN);
        input_cx = strlen(inputlines[inputline]);

        return true;
    }

    // move forward in input history
    if (key == KEY_DOWNARROW)
    {
        if (inputhist==inputline) return true;

        do{
            inputhist = (inputhist + 1) & CON_MAX_LINEHIST_INDEXMASK;
        } while (inputhist!=inputline && !inputlines[inputhist][1]);

        memset (inputlines[inputline],0,CON_MAX_LINELEN);

        // back to currentline
        if (inputhist==inputline)
        {
            inputlines[inputline][0] = CON_PROMPTCHAR;
            input_cx = 1;
        }
        else
        {
            // [WDJ] GCC 10 complains that strings may overlap so cannot use strcpy.
            // But overlap case handled above.
            memmove( inputlines[inputline], inputlines[inputhist], CON_MAX_LINELEN );
            input_cx = strlen(inputlines[inputline]);
        }
        return true;
    }

    // allow people to use keypad in console (good for typing IP addresses) - Calum
    if (key >= KEY_KEYPAD0 && key <= KEY_PLUSPAD)
    {
      const char keypad_translation[] = {'0','1','2','3','4','5','6','7','8','9','.','/','*','-','+'};
      con_ch = keypad_translation[key - KEY_KEYPAD0];
    }

    // This is console open mode.
    if( con_ch == 0 )
        return true; // eat it to keep from closing console

    // enter a printable char into the command prompt
    if (con_ch < ' ' || con_ch > '~')
      return false;

    // add key to cmd line here
    if (input_cx<CON_MAX_LINELEN)
    {
        // make sure letters are lowercase for commands & cvars
        if (con_ch >= 'A' && con_ch <= 'Z')
            con_ch = con_ch + 'a' - 'A';

        inputlines[inputline][input_cx] = con_ch;
        input_cx++;
        inputlines[inputline][input_cx] = 0;
    }

    return true;
 
toggle_console:
    consoletoggle = true;  // signal to CON_Ticker
    return true;

enable_autorepeat:
    con_autorepeat_ev = *ev;
    con_autorepeat_tick = 4;
    return true;
}


//  Insert a new line in the console text buffer
//
//  viewnum : splitscreen 0=upper, 1=lower, single player uses 0, 5=console only
static void CON_Linefeed (byte viewnum)
{
    con_viewnum[con_cy%con_hudlines] = viewnum; // May be msg for player2

    // set time for heads up messages
    con_hudtime[con_cy%con_hudlines] =
        (viewnum < 2)? cons_msgtimeout.value*TICRATE : 0;

    con_cy++;
    con_cx = 0;

    con_line = &con_buffer[(con_cy%con_totallines)*con_width];
    memset(con_line,' ',con_width);

    // make sure the view borders are refreshed if hud messages scroll
    con_hudupdate = true;         // see HU_Erase()
}


//  Outputs text into the console text buffer
//
static void CON_Print (byte control, char * msg)
{
    int  l;  // word length
    int  text_color=0;  // 0x80 is white text flag
    int  viewnum=0;  // 0=upper, 1=lower, Single player uses 0. 5=console only

    viewnum = control & 0x0F;

    //TODO: finish text colors
    if (*msg<5)
    {
        switch( *msg )
        {
         case '\2' :  // white text
            text_color = 0x80;
            break;
         case '\3' :  // white text + sound
            text_color = 0x80; // white text
            if ( gamemode == doom2_commercial )
                S_StartSound(sfx_radio);
            else
                S_StartSound(sfx_tink);
            break;
         default:
            break;
        }
    }

    while (*msg)
    {
        // handle non-printable characters and white spaces
        while ( *msg <= ' ' )
        {
            switch( *msg )
            {
             case '\r':  // carriage return
                con_cy--;
                CON_Linefeed (viewnum);
                break;
             case '\n':  // linefeed
                CON_Linefeed (viewnum);
                break;
             case ' ':  // leading space
                con_line[con_cx++] = ' ';
                if (con_cx >= con_width)
                    CON_Linefeed(viewnum);
                break;
             case '\t':  // tab
                //adds tab spaces for nice layout in console
                do
                {
                    con_line[con_cx++] = ' ';
                } while (con_cx%4 != 0);
                
                if (con_cx>=con_width)
                    CON_Linefeed(viewnum);
                break;
             case 0:  // End of string
                return;

             default:
                break;
            }
            msg++;
        }

        // printable character
        // Find end of word
        for (l=0; l<con_width; l++)
            if( msg[l] <= ' ' )  break;  // until space, or EOS

        // word wrap when word is too long for the rest of the width
        if (con_cx+l>con_width)
            CON_Linefeed (viewnum);

        // a word at a time
        for ( ; l>0; l--)
            con_line[con_cx++] = *(msg++) | text_color;
    }
}


//  Console print! Wahooo! Lots o fun!
//
#define CONS_BUF_SIZE 1024


// Tables for comparison to cv_gameplay.
// {0,"Off"},{1,"Minimal"},{2,"Play"},{3,"Verbose"},{4,"Debug"},{5,"Dev"},

// Show messages on hud when cv_gameplay is set at or higher than this table.
// Otherwise route message only to the console.
// 0 always shows the message on the hud.
// indexed by EMSG_cat
static byte gameplay_hud_message_table[ 16 ] =
{
 3, // EMSG_CONS
 0, // EMSG_playmsg
 0, // EMSG_playmsg2
 0, // unk3
 0, // unk4
 250, // EMSG_console
 1, // EMSG_hud
 0, // unk7
 2, // EMSG_info
 3, // EMSG_ver
 4, // EMSG_debug
 5, // EMSG_dev
 2, // EMSG_warn
 0, // EMSG_errlog
 0, // EMSG_error
 0  // EMSG_error2
};
// Show messages on console when cv_gameplay is set at or higher than this table.
// 0 always shows the message on the console.
// indexed by EMSG_cat
static byte gameplay_con_message_table[ 16 ] =
{
 0, // EMSG_CONS  (cannot be blocking CONS_Printf from the console)
 0, // EMSG_playmsg
 0, // EMSG_playmsg2
 0, // unk3
 0, // unk4
 0, // EMSG_console  (interactive console specific)
 0, // EMSG_hud
 0, // unk7
 1, // EMSG_info
 2, // EMSG_ver
 3, // EMSG_debug
 4, // EMSG_dev
 0, // EMSG_warn
 0, // EMSG_errlog
 0, // EMSG_error
 0  // EMSG_error2
};

// [WDJ] print from va_list
// Caller must have va_start, va_end, or else run-time segfault will occur.
void GenPrintf_va (const byte emsg, const char * fmt, va_list ap)
{
    byte eout = EOUT_flags;  // default for CONS_Printf
    byte ecat = emsg & EMSG_cat;
    byte viewnum = 0;
    // vid : from video setup
    char  txt[CONS_BUF_SIZE];

    // print the error
    vsnprintf(txt, CONS_BUF_SIZE, fmt, ap);
    txt[CONS_BUF_SIZE-1] = '\0'; // term, when length limited

    // Route the message to various outputs according to category.
    switch( ecat )
    {
     case EMSG_playmsg:
        eout = EOUT_hud | EOUT_con;
        viewnum = 0;
        break;
     case EMSG_playmsg2:  // player2 splitscreen
        eout = EOUT_hud | EOUT_con;
        viewnum = 1;
        break;
     case EMSG_console:  // console interactive
        eout = EOUT_con;  // console only
        viewnum = 5;       
        break;
     case EMSG_hud:
        eout |= EOUT_hud | EOUT_con;
        break;
     case EMSG_warn:
        eout |= EOUT_all;
        break;
     case EMSG_errlog:  // stderr and log, but not console
        eout = EOUT_text | EOUT_log;
        break;
     case EMSG_error: // error soft
        eout |= EOUT_all;
        break;
     case EMSG_error2: // error severe
        eout = EOUT_all | EOUT_hud;
        break;
     case EMSG_debug: // debug category
#if defined(SMIF_PC_DOS) || defined(WIN32) || defined(SMIF_OS2_NATIVE)
        eout = EOUT_text | EOUT_con | EOUT_log;
#else
        // Linux, Mac
        eout = EOUT_text | EOUT_log;
#endif
        if( cv_showmessages.EV >= 4 )
            eout |= EOUT_con | EOUT_hud;
        break;
     case EMSG_dev:   // development category
        eout &= ~EOUT_hud;
#if defined(SMIF_PC_DOS) || defined(WIN32) || defined(SMIF_OS2_NATIVE)
        eout |= EOUT_con;
#else
        // Linux, Mac
        eout = EOUT_text | EOUT_log;
#endif
        break;
     default:
        break;
    }

    if( emsg & EMSG_all )
       eout |= EOUT_text | EOUT_con | EOUT_log;

#ifdef LOGMESSAGES
    if( eout & EOUT_log )
    { 
        // echo console prints to log file
        if (logstream)
            fputs(txt, logstream);
    }
#endif
    DEBFILE(txt);

#ifndef  DEBUG_MESSAGES_ON
    // Hide debug messages for release version
    if((ecat == EMSG_debug) && (verbose == 0) && (cv_showmessages.EV < 4))
       goto done;  // disable debug messages
#endif

    if( (eout & EOUT_text) || (! vid.draw_ready) )
    {
        // Errors to terminal, and before graphics
        I_OutputMsg ("%s",txt);
        fflush(NULL);
    }

#if 0
#ifdef LINUX
    // Keep debug messages off console, for some versions
    if( ecat == EMSG_debug )  goto done;
#endif
#endif

    if( ! con_started )  goto done;

    // During gameplay the hud is dedicated to the game, according
    // to the cv_showmessages setting.
    if( gameplay_msg )
    {
        // [Arcade] Keep gameplay messages off the HUD in two cases.  They
        // still reach the console either way, and the idle-timeout countdown
        // (HU_SetTip) is a separate path and unaffected.
        //
        // A shared screen: a pickup line belongs to whoever triggered it but
        // is painted across the top of the whole screen, so on a splitscreen
        // or 2x2 cabinet it sits over somebody else's view and tells them
        // nothing.
        //
        // A demo replay: the attract screen is a shop window, and somebody
        // else's pickups scrolling across it are noise.  This covers the
        // Single Level "watch run" replays too.  Note HS_DemoLabel still
        // draws on the second text line rather than the first, which is now
        // free -- left there so the caption does not move.
        if( (D_NumViews() > 1) || demoplayback )
            viewnum = 5;  // console only

        // During game playing, honor the showmessage option.
        if( cv_showmessages.EV < gameplay_hud_message_table[ ecat ] )
            viewnum = 5;  // console only	    
        if( cv_showmessages.EV < gameplay_con_message_table[ ecat ] )
            return;
    }

#if 0
    // Other ecat tests have already forced EOUT_con for error messages.
#ifdef LAUNCHER
    // Allow errors to go to Launcher fatal error display.
#else
    if( (ecat == EMSG_error) || (ecat == EMSG_error2) )
    {
        // Errors to CON, unless no con_video yet.
        if( ! con_video )  goto done;
    }
#endif
#endif

    // Situations inherited from EMSG_ settings.
    if( (eout & (EOUT_hud|EOUT_con)) == 0  )  goto done;  // no CONS flag

    if( (eout & (EOUT_hud|EOUT_con)) == EOUT_con )
       viewnum = 5;  // console only

    // Output to EOUT_con, EOUT_hud, splitscreen.
    // Write the message in con text buffer.
    CON_Print ( viewnum, txt );

    // make sure new text is visible
    con_scrollup = 0;

    // if not in display loop, force screen update
    if ( con_self_refresh || (emsg & EMSG_now) )
    {
        // Protect against segfaults during video mode switch.
        if( ! vid.draw_ready )   goto done;
        // Have graphics, but do not have refresh loop running.
#if defined(SMIF_WIN_NATIVE) || defined(SMIF_OS2_NATIVE) 
        // show startup screen and message using only 'software' graphics
        // (rendermode may be hardware accelerated, but the video mode is not set yet)
        CON_Draw_Backpic (con_backpic, 0, vid.width);    // put console background
        I_LoadingScreen ( txt );
#else
        V_Clear_Display();
        // here we display the console background and console text
        // (no hardware accelerated support for these versions)
        CON_Drawer ();
        I_FinishUpdate ();              // page flip or blit buffer
#endif
    }
    else if ( ! con_video )
    {
        // Protect against segfaults during video mode switch.
        if( ! vid.draw_ready )   goto done;
        // Text messages without con_video graphics.
        CON_Draw_Console ();  // Text with or without con_video
        I_FinishUpdate ();
    }
 done:
    return;
}

// General printf interface for CONS_Printf
// Due to script files and indirect commands, many error messages
// still go through here, so they are seen on stderr.
// Global param: EOUT_flags
void CONS_Printf (const char * fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    GenPrintf_va( EMSG_CONS, fmt, ap );
    va_end(ap);
}

//  Print an error message, and wait for ENTER key to continue.
//  To make sure the user has seen the message
//
void CONS_Error (char * msg)
{
#ifdef SMIF_WIN_NATIVE
    if( graphics_state < VGS_active )
    {
        I_MsgBox (msg);
        return;
    }
#endif
    // Must pass msg through an interface that uses va_start, va_end.
    GenPrintf( EMSG_error, "\2%s", msg);   // write error msg in different colour

    // CONS_Printf ("Press ENTER to continue\n");
    // dirty quick hack, but for the good cause
    // while (I_GetKey() != KEY_ENTER)
    //   ;
}

// Console interaction printf interface.
// This only routes the print to the console, not the logs, not to stderr.
// Do not use this for command error messages, because many commands are
// used in scripts, or exec indirectly, and the user would not see
// the error messages.
void con_Printf (const char * fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    GenPrintf_va( EMSG_console, fmt, ap );
    va_end(ap);
}

// Debug printf interface.
// Easy to use replacement for debug messages going through CONS_Printf.
void debug_Printf (const char * fmt, ...)
{
    va_list ap;

    // It is still possible to use GenPrintf(EMSG_debug, ) which is
    // why there will be no special tests here.
    va_start(ap, fmt);
    GenPrintf_va( EMSG_debug, fmt, ap );
    va_end(ap);
}

// For info, debug, dev, verbose messages.
// Print to output set by EOUT_flags.
void GenPrintf (const byte emsg, const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    GenPrintf_va( emsg, fmt, ap );  // print to text, console, and logs
    va_end(ap);
}


//======================================================================
//                          CONSOLE DRAW
//======================================================================


// draw console prompt line
//
static void CON_DrawInput ( int y )
{
    char *  p;
    int     x;

    // Draw console text, screen0
    // V_SetupDraw( 0 | V_NOSCALE );

    // input line scrolls left if it gets too long
    //
    p = inputlines[inputline];
    if (input_cx >= con_width)
        p += input_cx - con_width + 1;

#ifdef CONSOLE_PROPORTIONAL
    int xj, xcursor = 0;
    x = con_indent;
    for(xj=0; xj<con_width; xj++)
    {
        if( xj == input_cx )
           xcursor = x;
        x += V_DrawCharacter( x, y, p[xj] );  // red
    }
#else
    // Fixed width font.
    for(x=0; x<con_width; x++)
        V_DrawCharacter( x * drawfont.xinc + con_indent, y, p[x] );  // red
#endif

    // draw the blinking cursor
    //
#ifdef CONSOLE_PROPORTIONAL
    if (con_tick<4)
        V_DrawCharacter( xcursor, y, 0x80 | '_' );  // white
#else
    x = (input_cx>=con_width) ? con_width - 1 : input_cx;
    if (con_tick<4)
        V_DrawCharacter( x * drawfont.xinc + con_indent, y, 0x80 | '_' );  // white
#endif
}


// draw the last lines of console text to the top of the screen
//
#ifdef HWRENDER //Added by Mysterial
    extern float gr_viewheight; //needed for drawing second player's messages
                                //halfway down
#endif

static void CON_Draw_Hudlines (void)
{
    fontinfo_t * fip = V_FontInfo();  // draw font1 and wad font strings
    byte     viewnum;
    char *  p;
    int     y1, y2, x, y, i;

    if (con_hudlines<=0)
        return;

    V_SetupFont( cv_msg_fontsize.value, fip, V_NOSCALE );

    // player1 message y
    y1 = (chat_on) ?
      drawfont.yinc  // leave place for chat input in the first row of text
      : 0;
    y = y1;
    // player2 message y in splitscreen
#ifdef HWRENDER
    // by Mysterial, moved by [WDJ]
    y2 = (rendermode==render_soft)? rdraw_viewheight : gr_viewheight;
#else    
    y2 = rdraw_viewheight;
#endif

    for (i= con_cy-con_hudlines+1; i<=con_cy; i++)
    {
        if (i < 0)
            continue;
        if (con_hudtime[i%con_hudlines] == 0)
            continue;

        viewnum = con_viewnum[i%con_hudlines];
        // viewnum: 0=upper, 1=lower, 5=console only
        if( viewnum > 1 )  continue;  // console only
        y = (viewnum == 1)? y2 : y1;

        p = &con_buffer[(i%con_totallines)*con_width];

#ifdef CONSOLE_PROPORTIONAL
        x = drawfont.xinc;  // indent
        int xj;
        for (xj=0; xj<con_width; xj++)
        {
            // red, proportional width font
            x += V_DrawCharacter ( x, y, p[xj] );
//            x += V_DrawCharacter ( x, y, (p[x]&0x7f) );  // force red
        }
#else
        for (x=0; x<con_width; x++)
        {
            // red, fixed width font
            V_DrawCharacter ( (x+1)*drawfont.xinc, y, p[x] );
//            V_DrawCharacter ( (x+1)*drawfont.xinc, y, (p[x]&0x7f) );
        }
#endif

        // viewnum: 0=upper, 1=lower
        if ( viewnum == 1 )
           y2 += drawfont.yinc;
        else
           y1 += drawfont.yinc;
    }

    // top screen lines that might need clearing when view is reduced
    con_clearlines = y;      // this is handled by HU_Erase ();
}


//  Scale a pic_t at 'startx' pos, to 'destwidth' columns.
//                startx,destwidth is resolution dependent
//  Used to draw console borders, console background.
//  The pic must be sized BASEVIDHEIGHT height.
//
//  TODO: ASM routine!!! lazy Fab!!
//
static void CON_Draw_Backpic (pic_t * pic, int startx, int destwidth)
{
    // vid : from video setup
    int   pic_h = pic->height;
    int   pic_w = pic->width;
    int   x, y;
    int   v;
    fixed_t  frac, fracstep;
    byte * src;
    byte * dest;  // within screen buffer
   
    // [WDJ] Draw picture for all bpp, bytepp, and padded lines.
    dest = V_GetDrawAddr( startx, 0 );  // screen0 buffer

    for (y=0 ; y<con_curlines ; y++, dest += vid.ybytes)
    {
        // scale the picture to the resolution
        v = pic_h - ((con_curlines - y)*(BASEVIDHEIGHT-1)/vid.height) - 1;

        src = pic->data + v*pic_w;

        // in case of the console backpic, simplify
        if (pic_w == destwidth && vid.bytepp == 1)
            memcpy (dest, src, destwidth);
        else
        {
            // scale pic to screen width
            frac = 0;
            fracstep = (pic_w<<16)/destwidth;
            for (x=0 ; x<destwidth ; x+=4)
            {
                V_DrawPixel( dest, x, src[frac>>16] );
                frac += fracstep;
                V_DrawPixel( dest, x+1, src[frac>>16] );
                frac += fracstep;
                V_DrawPixel( dest, x+2, src[frac>>16] );
                frac += fracstep;
                V_DrawPixel( dest, x+3, src[frac>>16] );
                frac += fracstep;
            }
        }
    }

}


// Draw the console background, text, and prompt if enough places.
// May use font1 or wad fonts.
// Uses screens[0].
//
void CON_Draw_Console (void)
{
    // vid : from video setup
    fontinfo_t * fip = V_FontInfo();  // draw font1 and wad font strings
    char * p;
    int   i, x, y;
    int   w = 0, x2 = 0;

    if (con_curlines <= 0)
        return;

    if ( rendermode != render_soft && use_font1 )
        return;  // opengl graphics, hu_font not loaded yet

    V_SetupFont( cv_con_fontsize.value, fip, V_NOSCALE );

    //FIXME: refresh borders only when console bg is translucent
    con_clearlines = con_curlines;    // clear console draw from view borders
    con_hudupdate = true;             // always refresh while console is on

    // draw console background
    if (!con_video)
    {
        V_Clear_Display();
    }
    else
    if (cons_backpic.value || con_forcepic)
    {
#ifdef HWRENDER // not win32 only 19990829 by Kin
        if (rendermode!=render_soft)
            V_DrawScalePic_Num (0, con_curlines-200*vid.fdupy,
                                W_GetNumForName ("CONSBACK") );
        else
#endif
            CON_Draw_Backpic (con_backpic,0,vid.width);   // picture as background
    }
    else
    {
#ifdef HWRENDER // not win32 only 19990829 by Kin
        if( rendermode == render_soft )
#endif
        {
            w = fip->xinc * vid.dupx;  // font1 or wad font
            x2 = vid.width - w;
            CON_Draw_Backpic (con_bordleft,0,w);
            CON_Draw_Backpic (con_bordright,x2,w);
        }
        // translucent background
        //Hurdler: what's the correct value of w and x2 in hardware mode ???
#if 0
        // Darken the borders too
        if( cv_darkback.value )
            V_FadeConsBack (0, vid.width, con_curlines);
        else
            V_FadeConsBack (w, x2, con_curlines);
#else
        V_FadeConsBack (w, x2, con_curlines);
#endif
    }

    // draw console text lines from bottom to top
    // (going backward in console buffer text)
    //
    if (con_curlines <20)       //8+8+4
        return;

    i = con_cy - con_scrollup;

    // skip the last empty line due to the cursor being at the start
    // of a new line
    if (!con_scrollup && !con_cx)
        i--;

    // draw lines with font1 or wad font
    for (y=con_curlines - (drawfont.yinc * 2); y>=0; y-=drawfont.yinc)
    {
        if (i<0)
            i=0;

        p = &con_buffer[(i%con_totallines)*con_width];

#ifdef CONSOLE_PROPORTIONAL
        x = con_indent;  // indent
        int xj;
        for (xj=0; xj<con_width; xj++)
        {
            // red, proportional width font
            x += V_DrawCharacter ( x, y, p[xj] );
//            x += V_DrawCharacter ( x, y, (p[x]&0x7f) );  // force red
        }
#else
        // red, fixed width font
        for (x=0;x<con_width;x++)
            V_DrawCharacter( x * drawfont.xinc + con_indent, y, p[x] );
#endif
        i--;
    }


    // draw prompt if enough place (not while game startup)
    //
    if ((con_curlines==con_destlines) && (con_curlines>=20) && !con_self_refresh)
        CON_DrawInput ( con_curlines - drawfont.yinc );
}


//  Console refresh drawer, call each frame
//
void CON_Drawer (void)
{
    if (!con_started)
        return;

    if (con_recalc)
        CON_RecalcSize ( vid.width );
   
    if ( use_font1 )
        return;  // hu_font not loaded yet

#ifndef CONSOLE_PROPORTIONAL
    //Fab: bighack: patch 'I' letter leftoffset so it centers
    hu_font['I'-HU_FONTSTART]->leftoffset = -2;
#endif

    if (con_curlines>0)
        CON_Draw_Console ();
    else
    if (gamestate==GS_LEVEL)
        CON_Draw_Hudlines ();

#ifndef CONSOLE_PROPORTIONAL
    hu_font['I'-HU_FONTSTART]->leftoffset = 0;
#endif
}
