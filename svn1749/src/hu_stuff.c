// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hu_stuff.c 1711 2025-01-17 04:17:06Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: hu_stuff.c,v $
// Revision 1.18  2003/11/22 00:22:09  darkwolf95
// get rid of FS hud pics on level exit and new game, also added exl's fix
// for clearing hub variables on new game
//
// Revision 1.17  2003/07/14 12:39:12  darkwolf95
// Made rankings screen smaller for splitscreen.  Ditched the graphical title
// and limited max rankings to four.
//
// Revision 1.16  2002/07/23 15:07:10  mysterial
// Messages to second player appear on his half of the screen
//
// Revision 1.15  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.14  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.13  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.12  2001/05/16 21:21:14  bpereira
// Revision 1.11  2001/04/01 17:35:06  bpereira
// Revision 1.10  2001/02/24 13:35:20  bpereira
//
// Revision 1.9  2001/02/19 17:40:34  hurdler
// Fix a bug with "chat on" in hw mode
//
// Revision 1.8  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.7  2000/11/04 16:23:43  bpereira
//
// Revision 1.6  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/09/28 20:57:15  bpereira
// Revision 1.4  2000/08/31 14:30:55  bpereira
// Revision 1.3  2000/08/03 17:57:42  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      heads up displays, cleaned up (hasta la vista hu_lib)
//      because a lot of code was unnecessary now
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "hu_stuff.h"

#include "d_netcmd.h"
#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"

// Data.
#include "dstrings.h"
#include "st_stuff.h"
  //added:05-02-98: ST_HEIGHT
#include "r_local.h"
#include "wi_stuff.h"
  // for drawrankings
#include "hs_stuff.h"
  // [Arcade] HS_DemoLabel
#include "p_info.h"
#include "p_inter.h"
  // P_SetMessage

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif




// coords are scaled
#define HU_INPUTX       0
#define HU_INPUTY       0

//-------------------------------------------
//              heads up font
//-------------------------------------------
patch_t*                hu_font[HU_FONTSIZE];


static player_t*        plr;
boolean                 chat_on;

static boolean          headsup_active = false;

boolean                 hu_showscores;        // draw deathmatch rankings

static char             hu_tick;

//-------------------------------------------
//              misc vars
//-------------------------------------------

consvar_t*   chat_macros[10];

static
CV_PossibleValue_t crosshair_cons_t[] = {{0,"Off"},{1,"Cross"},{2,"Angle"},{3,"Point"},{0,NULL}};
consvar_t cv_crosshair[MAXSPLITSCREENPLAYERS] = {
  {"crosshair"   ,"0",CV_SAVE,crosshair_cons_t},
  {"crosshair2"   ,"0",CV_SAVE,crosshair_cons_t},
  {"crosshair3"   ,"0",CV_SAVE,crosshair_cons_t},
  {"crosshair4"   ,"0",CV_SAVE,crosshair_cons_t}
};
//consvar_t cv_crosshairscale   = {"crosshairscale","0",CV_SAVE,CV_YesNo};

// maximum 9
#define HU_CROSSHAIRS   3
//added:16-02-98: crosshair 0=off, 1=cross, 2=angle, 3=point, see m_menu.c
static patch_t * crosshair_patch[HU_CROSSHAIRS];     //3 precached crosshair graphics

byte  hu_fonts_loaded = 0; // 1=partially loaded, 2=fully loaded

#ifdef HWRENDER
// The settings of HWR_patchstore by SCR_SetMode seem to be adequate.
// This only would be needed if there were additional problems.
// It is more expensive.
//#define HU_HWR_PATCHSTORE_SAVE
#ifdef HU_HWR_PATCHSTORE_SAVE
static byte  hu_HWR_patchstore;  // the HWR_patchstore setting used by fonts.
#endif
#endif


// -------
// protos.
// -------
static void  HU_Draw_DeathmatchRankings( byte vind );

// [Arcade] Should this view show the ranking overlay?  Each panel answers for
// itself: its own player's death, or its own scores key.  gamecontrol_pl[vind]
// rather than gamecontrol, or every panel would follow player 1's key.
static boolean  HU_Rankings_For_View( byte vind, byte pn )
{
    {
        byte pan = D_Panel_Of(vind);   // [Arcade] this view's own panel
        if( gamekeydown[gamecontrol_pl[pan][gc_scores][0]]
            || gamekeydown[gamecontrol_pl[pan][gc_scores][1]] )
            return ! chat_on;
    }
    if( 0 )
        return ! chat_on;

    return ( players[pn].playerstate == PST_DEAD );
}
static void  HU_Draw_Crosshair( void );
// HU_Draw_Tip is declared in hu_stuff.h -- [Arcade] the intermission and
// finale call it directly for the idle-timeout countdown.



//======================================================================
//                          HEADS UP INIT
//======================================================================

void HU_Stop(void)
{
    headsup_active = false;
}

// 
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
    if (headsup_active)
        HU_Stop();

    plr = consoleplayer_ptr;
    chat_on = false;

    headsup_active = true;
}

void HU_Load_Graphics( void )
{
    int         i, j;
    char        buffer[9];

    hu_fonts_loaded = 2;
    // cache the heads-up font
    // Patches are endian fixed when loaded.
    j = (EN_heretic)? 1 : HU_FONTSTART;
    for (i=0; i<HU_FONTSIZE; i++)
    {
        if( EN_heretic_hexen )
            sprintf(buffer, "FONTA%.2d", ((j>59)? 59 : j));
        else
            sprintf(buffer, "STCFN%.3d", j);

        j++;
        if( ! VALID_LUMP( W_CheckNumForName( buffer ) ) )
        {
            // font not found
            hu_font[i] = NULL;
            hu_fonts_loaded = 1;  // partially loaded
            continue;
        }
        hu_font[i] = W_CachePatchName(buffer, PU_STATIC);
    }

    // cache the crosshairs, dont bother to know which one is being used,
    // just cache them 3 all, they're so small anyway.
    for(i=0; i<HU_CROSSHAIRS; i++)
    {
       sprintf(buffer, "CROSHAI%c", '1'+i);
       crosshair_patch[i] = W_CachePatchName(buffer, PU_STATIC);
    }

#ifdef HU_HWR_PATCHSTORE_SAVE
    hu_HWR_patchstore = HWR_patchstore;
#endif
}

void HU_Release_Graphics( void )
{
#ifdef HU_HWR_PATCHSTORE_SAVE
    byte saved_HWR_patchstore = HWR_patchstore;
#endif

    if( hu_fonts_loaded )
    {
        hu_fonts_loaded = 0;
        
#ifdef HU_HWR_PATCHSTORE_SAVE
        // Must use setting from when fonts were saved.
        // This is necessary because releasing the font is delayed until the last second.
        HWR_patchstore = hu_HWR_patchstore;
#endif

        // Has protection against individual NULL ptr in array.
        release_patch_array( hu_font, HU_FONTSIZE );
        release_patch_array( crosshair_patch, HU_CROSSHAIRS );
       
#ifdef HU_HWR_PATCHSTORE_SAVE
        HWR_patchstore = saved_HWR_patchstore;
#endif
    }
}


//======================================================================
//                            EXECUTION
//======================================================================


// SAY: Broadcast to all players.
void Command_Say_f (void)
{
    char buf[255];
    int i,j;

    if((j=COM_Argc())<2)
    {
        CONS_Printf ("say <message> : send a message\n");
        return;
    }

    buf[0]=(unsigned char)255;  // broadcast
    strcpy(&buf[1], COM_Argv(1));
    for(i=2; i<j; i++)
    {
        strcat(&buf[1]," ");
        strcat(&buf[1], COM_Argv(i));
    }
    // as mainplayer
    Send_NetXCmd(XD_SAY, buf, strlen(buf+1)+2);
       // +2 because 1 for buf[0] and the other for null terminated string
}

// SAYTO: Send to a player.
void Command_Sayto_f (void)
{
    byte playernum;
    int i,j;
    char buf[255];

    if((j=COM_Argc())<3)
    {
        CONS_Printf ("sayto <playername|playernum> <message> : send a message to a player\n");
        return;
    }

    // Players 0..(MAXPLAYERS-1) are known as Player 1 to MAXPLAYERS to user.
    playernum = player_name_to_num(COM_Argv(1));
    if(playernum > MAXPLAYERS)
        return;  // not found

    buf[0] = (unsigned char)playernum;    // 0..127
    strcpy(&buf[1],COM_Argv(2));
    for(i=3; i<j; i++)
    {
        strcat(&buf[1]," ");
        strcat(&buf[1],COM_Argv(i));
    }
    // as mainplayer
    Send_NetXCmd(XD_SAY, buf, strlen(buf+1)+2);
}

// SAYTEAM: To all team members of this player.
void Command_Sayteam_f (void)
{
    char buf[255];
    int i,j;

    if((j=COM_Argc())<2)
    {
        CONS_Printf ("sayteam <message> : send a message to your team\n");
        return;
    }

    // Players 0..(MAXPLAYERS-1) are known as Player 1 to MAXPLAYERS to user.
    buf[0] = (unsigned char)( consoleplayer & 0x80 );  // 128..254
    strcpy(&buf[1],COM_Argv(1));
    for(i=2; i<j; i++)
    {
        strcat(&buf[1]," ");
        strcat(&buf[1],COM_Argv(i));
    }
    // as mainplayer
    Send_NetXCmd(XD_SAY, buf, strlen(buf+1)+2);
        // +2 because 1 for buf[0] and the other for null terminated string
}

// [WDJ] Previous Say/Sayto/Sayteam system was broken.
// New as of DoomLegacy 1.46:
//  to: 0..127 player
//      0x80 & team number, team= 0..126
//      255 broadcast

void Got_NetXCmd_Saycmd( xcmd_t * xc )
{
    // Command: ( byte: to_player_id, string0: message )
    // XCmd buffer has forced 0 term to protect against malicious message.
    const char * tostr = "";
    char * fromstr;
    byte * p = xc->curpos;
    byte to = *(p++);
    byte pn = (to & 0x7F); // to player num 0..126

    if( xc->playernum >= MAXPLAYERS )  goto done;  // cannot index player_names
    fromstr = player_names[xc->playernum];  // never NULL

    if( to==255 )
    {
        // Broadcast
        tostr = " All";
    }
    else
    {
        if( pn >= MAXPLAYERS )  goto done;  // cannot index tables
        if( to & 0x80 )
        {
            // To Team
            tostr = " Team";
        }
    }

    if(xc->playernum == consoleplayer
       || to==255 // broadcast
       || ( (to < MAXPLAYERS) && pn==consoleplayer )
       || ( (to & 0x80) // Team broadcast from pn
            && ST_SameTeam(consoleplayer_ptr,&players[pn])) )
    {
        GenPrintf( EMSG_playmsg, "\3%s%s: %s\n", fromstr, tostr, p);
    }

    if(  displayplayer2_ptr )
    {
        // Splitscreen
        if(xc->playernum == displayplayer2
           || to==255 // broadcast
           || ( (to < MAXPLAYERS) && pn==displayplayer2 )
           || ( (to & 0x80) // Team broadcast from pn
                && ST_SameTeam(displayplayer2_ptr,&players[pn])) )
        {
            GenPrintf( EMSG_playmsg2, "\3%s%s: %s\n", fromstr, tostr, p);
        }
    }

done:
    p += strlen((char*)p) + 1;  // incl term 0
    xc->curpos = p;
}




//
//
void HU_Ticker(void)
{
    player_t    *pl;

    if(dedicated)
        return;
    
    hu_tick++;
    hu_tick &= 7;        //currently only to blink chat input cursor

    // display message if necessary
    // (display the viewplayer's messages)
    pl = displayplayer_ptr;
    if ( pl->message )
    {
        // Player message blocking is handled by P_SetMessage.
        GenPrintf(EMSG_playmsg, "%s\n", pl->message);
        pl->message = NULL;
        pl->msglevel = 0;
    }

    // In splitscreen, display second player's messages
    if (cv_splitscreen.value && displayplayer2_ptr )
    {
        pl = displayplayer2_ptr;
        if ( pl->message )
        {
            // Player message blocking is handled by P_SetMessage.
            GenPrintf(EMSG_playmsg2, "%s\n", pl->message);
            pl->message = NULL;
            pl->msglevel = 0;
        }
    }
    
    //deathmatch rankings overlay if press key or while in death view
    if( deathmatch )
    {
        if (gamekeydown[gamecontrol[gc_scores][0]] ||
            gamekeydown[gamecontrol[gc_scores][1]] )
            hu_showscores = !chat_on;
        else
            hu_showscores = playerdeadview; //hack from P_DeathThink()
    }
    else
        hu_showscores = false;
}



// [smite] there's no reason to use a queue here, a normal buffer will do
static char     w_chat[HU_MAXMSGLEN+1]; // always NUL-terminated
static unsigned tail = 0; // first free cell, should contain NUL

// simplified stl::vector implementation
static boolean HU_Chat_push_back(char c)
{
  if (tail >= HU_MAXMSGLEN)
    return false;

  w_chat[tail++] = c;
  w_chat[tail] = '\0';
  return true;
}

static boolean HU_Chat_pop_back( void )
{
  if (tail == 0)
    return false;

  tail--;
  w_chat[tail] = '\0';
  return true;
}

static void HU_Chat_clear( void )
{
  tail = 0;
  w_chat[tail] = '\0';
}

static inline boolean HU_Chat_empty( void )
{
  return tail == 0;
}

// global
//  w_chat : string
static void HU_Chat_send( void )
{
  COM_BufInsertText(va("say %s", w_chat));
}



//
//  Returns true if key eaten
//
boolean HU_Responder (event_t *ev)
{
  // Depending on cv_sdl2_textchar,
  // either ev_keydown, or ev_textchar, may have valid ch, the other will have ch = 0.
  int hu_ch = ev->data2; // ASCII character

#ifdef SDL2
  // [WDJ] A key will come as ev_keydown, ev_keyup, and ev_textchar events.
  // This is horrible, and not safe, but necessary to stop the Text command
  // key from also being entered as the first letter of the chat.
  static byte block_hu_ch = 0;

  if (ev->type == ev_textchar)
  {
      // key = data1 = STX, an unused key
      // textchar to chat
      if( hu_ch == block_hu_ch )
      {
          block_hu_ch = 0;
          return true;  // eat it
      }

      block_hu_ch = 0;
  }
  else if (ev->type == ev_keydown)
  {
  }
  else
    return false;
#else
  // SDL1, ev_keydown event has key and translated char.
  if (ev->type != ev_keydown)
    return false;
 #endif

  // only KeyDown events now...
  int key = ev->data1;

  if (!chat_on)
  {
      // enter chat mode
      if (key == gamecontrol[gc_talkkey][0] || key == gamecontrol[gc_talkkey][1])
      {
#ifdef SDL2
          // [WDJ] Beware that another event with the ev_textchar translation of the key
          // is incoming.  If not intercepted it will be entered as the first
          // char of the chat.  This is because SDL2 posts the key event,
          // and the translated text char of that key, as separate events.
	  block_hu_ch = key;
#endif
          chat_on = true;
          HU_Chat_clear();
          return true;
      }
  }
  else
  {
      // send a macro
      if (altdown)
      {
          hu_ch = hu_ch - '0';
          if (hu_ch > 9 || hu_ch < 0)
            return false;

          // current message stays unchanged

          // send the macro message
          COM_BufInsertText(va("say %s", chat_macros[hu_ch]->string));

          // if there is no unfinished message, leave chat mode and notify that it was sent
          if (HU_Chat_empty())
            chat_on = false;
      }
      else
      {
          // chat input
          if (key == KEY_ESCAPE)
          {
              // close chat
              chat_on = false;
          }
          else if (key == KEY_ENTER)
          {
              // send the message
              if (tail > 1)
                HU_Chat_send();  // w_chat

              HU_Chat_clear();
              chat_on = false;
          }
          else if (key == KEY_BACKSPACE)
          {
              // erase a char
              HU_Chat_pop_back();
          }
          else if ( hu_ch >= ' ' &&  hu_ch <= '~')
          {
              // add a char
              if (!HU_Chat_push_back(hu_ch))
                P_SetMessage( plr, HUSTR_MSGU, 63);  // out of space
          }
#ifdef SDL2
          // Because SDL2 sends key and ch as separate events.
          else if ( key >= ' ' &&  key <= '~')  // to eat the key
            return true; // eat the key
#endif
          else
            return false; // let the event go
      }

      return true; // ate the key
  }

  return false; // let the event go
}


//======================================================================
//                         HEADS UP DRAWING
//======================================================================

//  Draw chat input
//
static void HU_Draw_Chat (void)
{
    // vid : from video setup
    int  i,x,y;
   
    V_SetupFont( cv_msg_fontsize.value, NULL, V_NOSCALE );

    i=0;
    x=HU_INPUTX;
    y=HU_INPUTY;
    while (w_chat[i])
    {
#if 1
        // Proportional Font  
        x += V_DrawCharacter( x, y, w_chat[i++] | 0x80 );  // white
#else
        // Fixed width Font  
        V_DrawCharacter( x, y, w_chat[i++] | 0x80 );  // white
        x += drawfont.xinc;
#endif

        if (x >= vid.width)
        {
            x = HU_INPUTX;
            y += drawfont.yinc;
        }
    }

    // Cursor blink
    if (hu_tick<4)
        V_DrawCharacter( x, y, '_' | 0x80 );  // white
}


extern consvar_t cv_chasecam;

//  Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
    // draw chat string plus cursor
    if (chat_on)
        HU_Draw_Chat ();

    // draw deathmatch rankings
    // [Arcade] One ranking overlay per view, drawn only for a player who is
    // actually dead (or holding his own scores key).  It used to be a single
    // global test painted across the whole screen, so one player dying took
    // the view away from everyone else on the cabinet.
    if( deathmatch )
    {
        byte vind, num_views = D_NumViews();
        for( vind=0; vind<num_views; vind++ )
        {
            byte pn = localplayer[vind];

            if( pn >= MAXPLAYERS )
            {
                // [Arcade] A cell with no player gets the rankings, always.
                //
                // Three players use the 2x2 grid and leave one quadrant
                // unclaimed, which D_Display fills black -- dead space on a
                // screen where the one thing everybody keeps wanting is the
                // score, and the only way to see it is to hold your own
                // scores key and lose your view while you do.  Put it in the
                // empty quadrant instead, permanently, where all three can
                // glance at it without giving anything up.
                //
                // Safe for a cell with no player: HU_Draw_DeathmatchRankings
                // positions from D_View_Cell(vind) and takes its highlighted
                // player from consoleplayer, never from localplayer[vind].
                // HU_Drawer runs after the black fill in D_Display, so this
                // paints over it rather than being erased by it.
                //
                // Only three players can reach this -- one and two get
                // layouts with no spare cell, four fill the grid -- so it
                // needs no player-count test of its own.
                HU_Draw_DeathmatchRankings( vind );
                continue;
            }

            if( HU_Rankings_For_View( vind, pn ) )
                HU_Draw_DeathmatchRankings( vind );
        }
    }

    // draw the crosshair, not when viewing demos nor with chasecam
    if (!automapactive && !demoplayback && !cv_chasecam.value)
        HU_Draw_Crosshair ();

    // [Arcade] Caption the attract screen's record demos, so a player can
    // see whose category and time they are watching.  Second text line, not
    // the first: item pickups still print messages at y=0 during playback.
    if( demoplayback )
    {
        const char * label = HS_DemoLabel();
        if( label )
        {
            V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH );
            V_DrawString( (BASEVIDWIDTH - V_StringWidth(label)) / 2, 8,
                          V_WHITEMAP, (char*) label );
        }
    }

    // [Arcade] The arcade "insert coin", on a cabinet that takes no coins:
    // a blinking prompt over the attract screen's demos telling a passer-by
    // how to start.  It is not a lie -- G_Responder already pops up the menu
    // on any keypress while a demo is playing, so fire really does open the
    // way to a game; nothing on the attract screen said so.
    if( demoplayback && ! menuactive && (gametic & 16) )
    {
        // Blink cadence matches the intermission's NEW RECORD marker, and
        // wi_stuff.c's own flashing pointer: gametic (not leveltime), 16 tics
        // on and 16 off.
        //
        // Placed clear of the status bar in *either* of its forms, since the
        // demo is drawn at whatever viewsize the cabinet is set to: the
        // classic bar's top is BASEVIDHEIGHT - ST_HEIGHT = 168, and the
        // overlay's lower row sits at 182 (st_stuff.c's SCY(198) - 16).  At
        // 168 - 8 = 160 the text spans 160..167 (hu_font glyphs are 7 tall),
        // clearing both.  Width was measured from the real STCFN lumps at
        // 133px of 320, so it centres with room to spare.
        //
        // It does cross the weapon sprite, which draws up the middle of the
        // screen in every mode -- unavoidable for anything centred down here,
        // and the blink is what keeps it readable against the moving demo.
        static const char press_fire[] = "PRESS FIRE TO START";
        V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH );
        V_DrawString( (BASEVIDWIDTH - V_StringWidth(press_fire)) / 2,
                      BASEVIDHEIGHT - ST_HEIGHT - 8,
                      V_WHITEMAP, (char*) press_fire );
    }

    // [Arcade] Standing reminder that this run will not be scored, so a
    // player who changed a setting is not surprised at the intermission.
    //
    // Single player only.  "UNRANKED" is meaningless in a multiplayer game --
    // there is no board for it to be off -- and hs_run_ranked is not reset
    // when one starts (HS_NewGame runs only on the single player and Single
    // Level routes), so without this test a flag left false by an earlier
    // solo run would paint the marker across a deathmatch too.
    if( !demoplayback && !devmode && HS_Scored_Game() && !HS_Run_Is_Ranked() )
    {
        // Name the reason when it was a death: "UNRANKED" alone reads as a
        // settings problem, and the player needs to know the retry they are
        // about to play is no longer being scored.  Measured against the
        // real STCFN0xx widths: 158px of 320, so it centers without clipping.
        // [Arcade] Cheating is named before dying: if both happened, the
        // cheat is the thing the player chose to do.
        const char * mark = HS_Run_Cheated() ? "PLAYER CHEATED - UNRANKED"
                          : HS_Run_Died()    ? "PLAYER DIED - UNRANKED"
                          :                    "UNRANKED";
        V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH );
        V_DrawString( (BASEVIDWIDTH - V_StringWidth(mark)) / 2, 0,
                      V_WHITEMAP, (char*) mark );
    }

    HU_Draw_Tip();
    HU_Draw_FSPics();
}

//======================================================================
//                          PLAYER TIPS
//======================================================================
#define MAXTIPLINES 20
char    *tiplines[MAXTIPLINES];
int     numtiplines = 0;
int     tiptime = 0;
int     largestline = 0;



void HU_SetTip(char *tip, int displaytics)
{
  int    i;
  char   *rover, *ctipline, *ctipline_p;


  for(i = 0; i < numtiplines; i++)
    Z_Free(tiplines[i]);


  numtiplines = 0;

  rover = tip;
  ctipline = ctipline_p = Z_Malloc(128, PU_STATIC, NULL);
  *ctipline = 0;
  largestline = 0;

  while(*rover)
  {
    if(*rover == '\n' || strlen(ctipline) + 2 >= 128 || V_StringWidth(ctipline) + 16 >= BASEVIDWIDTH)
    {
      if(numtiplines > MAXTIPLINES)
        break;
      if(V_StringWidth(ctipline) > largestline)
        largestline = V_StringWidth(ctipline);

      tiplines[numtiplines] = ctipline;
      ctipline = ctipline_p = Z_Malloc(128, PU_STATIC, NULL);
      *ctipline = 0;
      numtiplines ++;
    }
    else
    {
      *ctipline_p = *rover;
      ctipline_p++;
      *ctipline_p = 0;
    }
    rover++;

    if(!*rover)
    {
      if(V_StringWidth(ctipline) > largestline)
        largestline = V_StringWidth(ctipline);
      tiplines[numtiplines] = ctipline;
      numtiplines ++;
    }
  }

  tiptime = displaytics;
}




void HU_Draw_Tip(void)
{
  int    i;
  if(!numtiplines) return;
  if(!tiptime)
  {
    for(i = 0; i < numtiplines; i++)
      Z_Free(tiplines[i]);
    numtiplines = 0;
    return;
  }
  tiptime--;


  // Draw screen0, scaled
  V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH );
  for(i = 0; i < numtiplines; i++)
  {
    V_DrawString((BASEVIDWIDTH - largestline) / 2,
                 ((BASEVIDHEIGHT - (numtiplines * 8)) / 2) + ((i + 1) * 8),
                 0,
                 tiplines[i]);
  }
}


void HU_Clear_Tips()
{
  int    i;

  for(i = 0; i < numtiplines; i++)
    Z_Free(tiplines[i]);
  numtiplines = 0;

  tiptime = 0;
}


//======================================================================
//                           FS HUD Grapics!
//======================================================================
typedef struct
{
  lumpnum_t lumpnum;
  int       xpos;
  int       ypos;
  patch_t   *data;
  boolean   draw;
} fspic_t;

fspic_t*   piclist = NULL;	// realloc, never deallocated
int        num_piclist_alloc = 0;


// HU_InitFSPics
// This function is called when Doom starts and every time the piclist needs
// to be expanded.
void HU_Init_FSPics()
{
  fspic_t * npp;
  int  newstart, newend, i;

  newstart = num_piclist_alloc;
  if( num_piclist_alloc == 0 )
  {
    // Initial allocation.
    newend = 128;
  }
  else
  {
    // Double current allocation.
    newend = (num_piclist_alloc * 2);
  }

  npp = realloc(piclist, sizeof(fspic_t) * newend);
  // Check allocation fail [WDJ]
  if( npp == NULL )
     return;
  
  // Commit to new allocation.
  piclist = npp;
  num_piclist_alloc = newend;

  // Init the added slots to empty.
  for(i = newstart; i < newend; i++)
  {
    piclist[i].lumpnum = NO_LUMP;
    piclist[i].data = NULL;
    piclist[i].draw = false;
    piclist[i].xpos = 0;
    piclist[i].ypos = 0;
  }
}

// Return slot number (handle) for pic, [ 0 .. num_piclist_alloc-1 ].
int  HU_Get_FSPic( lumpnum_t lumpnum, int xpos, int ypos )
{
  int  i;

  if(!num_piclist_alloc)
    HU_Init_FSPics();

getpic_retry:  // retry
  for(i = 0; i < num_piclist_alloc; i++)
  {
    // Find empty slot
    if( VALID_LUMP(piclist[i].lumpnum) )
      continue;

    piclist[i].lumpnum = lumpnum;
    piclist[i].xpos = xpos;
    piclist[i].ypos = ypos;
    piclist[i].draw = false;
    return i;
  }

  // Did not find an empty slot.
  HU_Init_FSPics();
  goto getpic_retry;
}


int  HU_Delete_FSPic(int handle)
{
  if(handle < 0 || handle >= num_piclist_alloc)
    return -1;

  piclist[handle].lumpnum = NO_LUMP;
  piclist[handle].data = NULL;
  return 0;
}


int  HU_Modify_FSPic( int handle, lumpnum_t lumpnum, int xpos, int ypos )
{
  if(handle < 0 || handle >= num_piclist_alloc)
    return -1;

  if( ! VALID_LUMP(piclist[handle].lumpnum) )
    return -1;

  piclist[handle].lumpnum = lumpnum;
  piclist[handle].xpos = xpos;
  piclist[handle].ypos = ypos;
  piclist[handle].data = NULL;
  return 0;
}


// Enable or disable the drawing of a Pic.
int  HU_FS_Display(int handle, boolean enable_draw)
{
  if(handle < 0 || handle >= num_piclist_alloc)
    return -1;
  if( ! VALID_LUMP(piclist[handle].lumpnum) )
    return -1;

  piclist[handle].draw = enable_draw;
  return 0;
}


void HU_Draw_FSPics()
{
  // vid : from video setup
  int  i;

  // [WDJ] Fragglescript overlays must be centered.
  // Needed for Chexquest-newmaps scope with crosshairs.
  // Draw screen0, scaled, menu centering.
  V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART | V_CENTERMENU );

  for(i = 0; i < num_piclist_alloc; i++)
  {
    if( ! VALID_LUMP(piclist[i].lumpnum) )
      continue;

    if( ! piclist[i].draw )
      continue;  // not enabled

    if(piclist[i].xpos >= vid.width || piclist[i].ypos >= vid.height)
      continue;  // off screen right

    if(!piclist[i].data)
      piclist[i].data = (patch_t *) W_CachePatchNum(piclist[i].lumpnum, PU_STATIC); // endian fix

    if((piclist[i].xpos + piclist[i].data->width) < 0
       || (piclist[i].ypos + piclist[i].data->height) < 0)
      continue;  // off screen left

    V_DrawScaledPatch(piclist[i].xpos, piclist[i].ypos, piclist[i].data);
  }
  // restore
  //V_SetupDraw( drawinfo.prev_screenflags );
}

void HU_Clear_FSPics()
{
        piclist = NULL;
        num_piclist_alloc = 0;

        HU_Init_FSPics();
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

//  Clear old messages from the borders around the view window
//  (only for reduced view, refresh the borders when needed)
//
//  startline  : y coord to start clear,
//  clearlines : how many lines to clear.
//
static int     oldclearlines;

void HU_Erase (void)
{
    static  int     secondframelines;

    // vid : from video setup
    int topline;
    int bottomline;
    int y,yoffset;

    //faB: clear hud msgs on double buffer (Glide mode)
    boolean secondframe;

    if (con_clearlines==oldclearlines && !con_hudupdate && !chat_on)
        return;

    // clear the other frame in double-buffer modes
    secondframe = (con_clearlines!=oldclearlines);
    if (secondframe)
        secondframelines = oldclearlines;

    // clear the message lines that go away, so use _oldclearlines_
    bottomline = oldclearlines;
    oldclearlines = con_clearlines;
    if( chat_on )
        if( bottomline < 8 )
            bottomline=8;

    if (automapactive || view_window_x==0)   // hud msgs don't need to be cleared
        return;

    if( rendermode == render_soft )
    {
        // software mode copies view border pattern & beveled edges from the backbuffer
        topline = 0;
        for (y=topline,yoffset=y*vid.width; y<bottomline ; y++,yoffset+=vid.width)
        {
            if (y < view_window_y || y >= view_window_y + rdraw_viewheight)
                R_VideoErase(yoffset, vid.width); // erase entire line
            else
            {
                R_VideoErase(yoffset, view_window_x); // erase left border
                // erase right border
                R_VideoErase(yoffset + view_window_x + rdraw_viewwidth, view_window_x);
            }
        }
        con_hudupdate = false;      // if it was set..
    }
#ifdef HWRENDER 
    else
    {
        // refresh just what is needed from the view borders
        HWR_DrawViewBorder (secondframelines);
        con_hudupdate = secondframe;
    }
#endif
}



//======================================================================
//                   IN-LEVEL DEATHMATCH RANKINGS
//======================================================================

// count frags for each team
int HU_Create_TeamFragTbl(fragsort_t *fragtab,
                         int dmtotals[], int fragtbl[MAXPLAYERS][MAXPLAYERS])
{
    int i,j,k,scorelines,team;

    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            team = (cv_teamplay.EV==1) ? players[i].skincolor
                                       : players[i].skin;

            for(j=0; j<scorelines; j++)
            {
                if (fragtab[j].num == team)
                { // found there team
                     if(fragtbl)
                     {
                         for(k=0; k<MAXPLAYERS; k++)
                         {
                             if(playeringame[k])
                             {
                                 int k_indx = (cv_teamplay.EV==1) ?
                                     players[k].skincolor : players[k].skin;
                                 fragtbl[team][k_indx] += players[i].frags[k];
                             }
                         }
                     }

                     fragtab[j].count += ST_PlayerFrags(i);
                     if(dmtotals)
                         dmtotals[team]=fragtab[j].count;
                     break;
                }
            }  // for j

            if (j==scorelines)
            {   // team not found, add it

                if(fragtbl)
                {
                    for(k=0; k<MAXPLAYERS; k++)
                        fragtbl[team][k] = 0;
                }

                fragtab[scorelines].count = ST_PlayerFrags(i);
                fragtab[scorelines].num   = team;
                fragtab[scorelines].color = players[i].skincolor;
                fragtab[scorelines].name  = get_team_name(team);

                if(fragtbl)
                {
                    for(k=0; k<MAXPLAYERS; k++)
                    {
                        if(playeringame[k])
                        {
                            int k_indx = (cv_teamplay.EV==1) ?
                                players[k].skincolor : players[k].skin;
                            fragtbl[team][k_indx] += players[i].frags[k];
                        }
                    }
                }

                if(dmtotals)
                    dmtotals[team]=fragtab[scorelines].count;

                scorelines++;
            }
        }
    }
    return scorelines;
}


//
//  draw Deathmatch Rankings
//
static
// [Arcade] Draw the rankings inside one view's cell, at half scale, instead
// of once across the whole screen.  A dead player's team-mates should not
// lose their view to his score table -- see HU_Drawer, which now asks per
// view whether that player is dead.
//   vind : which view, 0 .. D_NumViews()-1
void HU_Draw_DeathmatchRankings ( byte vind )
{
    fragsort_t   fragtab[MAXPLAYERS];
    int          i;
    int          scorelines;
    int          whiteplayer;
    int          y;
    char*	 title;
    boolean	 large;

    byte  num_views = D_NumViews();
    byte  cell  = (num_views >= 2) ? D_View_Cell(vind) : 0;   // [Arcade] panel's cell, not join order.
    // Clamped to 0 for a single view: one player gets the whole screen
    // whichever panel they are at, and an unclamped cell 1 would still
    // push row to 1 and offset everything into a half that is not drawn.
    byte  col   = (num_views >= 4) ? (cell & 1) : 0;
    byte  row   = (num_views >= 4) ? (cell >> 1) : cell;
    int   offx  = 0, offy = 0;   // in base units, at the scale set below
    // Global draw scale, restored at the end (single exit).
    byte  sv_dupx  = vid.dupx,  sv_dupy  = vid.dupy;
    float sv_fdupx = vid.fdupx, sv_fdupy = vid.fdupy;

    // Halve the scale uniformly so the 320x200 ranking block covers a quarter
    // of the screen, which is exactly a 2x2 cell and half of a 2-view cell.
    // V_SCALESTART takes its start-coordinate scale from vid.dupx/dupy too,
    // so these four fields cover both the art and the positions.
    if( num_views >= 2 )
    {
        vid.fdupx = sv_fdupx / 2.0f;
        vid.fdupy = sv_fdupy / 2.0f;
        vid.dupx  = (byte)(vid.fdupx + 0.5f);
        vid.dupy  = (byte)(vid.fdupy + 0.5f);
        if( vid.dupx < 1 )  vid.dupx = 1;
        if( vid.dupy < 1 )  vid.dupy = 1;

        // Pin the float scale to the integer one.  V_DrawScaledFill and the
        // patches position by the integer dupx0 while V_DrawString uses the
        // float fdupx0, so 2 vs 2.13 lets a name slide off the colour bar it
        // belongs on -- 35px at the x this block sits at, more than half the
        // bar's width.  Losing 6% of scale is the smaller price.
        vid.fdupx = (float) vid.dupx;
        vid.fdupy = (float) vid.dupy;
    }

    // Draw screen0, scaled.
    //
    // V_CENTERHORZ deliberately only for the single full-screen view.  It
    // puts its centering into drawinfo.start_offset, and in hardware mode
    // V_DrawString does NOT apply that -- it positions text as
    // x * drawfont.fdupx0 with no start offset, while V_DrawScaledFill and
    // the patches do apply it.  The two therefore disagree by exactly
    // start_offset.  At full scale that was 43px and went unnoticed; with a
    // half-width block it becomes 363px, which threw the text right off the
    // left of the screen while the colour bars stayed put.  Placing the block
    // by explicit coordinates instead keeps text and fills on the same origin.
    V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART
                 | ((num_views < 2) ? V_CENTERHORZ : 0) );

    // Centre the block in its own cell, in base units.  Derived from pixels
    // rather than as multiples of BASEVIDWIDTH/HEIGHT: with dup rounded to 2
    // at 1366x768 a 200 unit block is 400px tall in a 384px cell, so stepping
    // a row by BASEVIDHEIGHT would push the lower row off the screen bottom.
    if( num_views >= 2 )
    {
        int cell_w = vid.width / ((num_views >= 4) ? 2 : 1);
        int cell_h = vid.height / 2;
        int block_w = BASEVIDWIDTH * vid.dupx;

        offx = ((col * cell_w) + ((cell_w - block_w) / 2)) / vid.dupx;
        offy = (row * cell_h) / vid.dupy;
    }

    // draw the ranking title panel
    if( num_views < 2 )
    {
        patch_t*  p = W_CachePatchName("RANKINGS",PU_CACHE);  // endian fix
        V_DrawScaledPatch ((BASEVIDWIDTH-p->width)/2, 5, p);
    }

    // count frags for each present player
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = ST_PlayerFrags(i);
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quickly see who's the
    //  view.
    whiteplayer = demoplayback ? displayplayer : consoleplayer;

    if (scorelines>9)
        scorelines = 9; //dont draw past bottom of screen, show the best only
    else if ((num_views >= 2) && scorelines > 4)
        scorelines = 4;

    if( num_views >= 2 )
    {
        y = (100 - (12 * (scorelines + 1) / 2)) + 15;
        title = "Rankings";
        large = false;
    }
    else
    {
        y = 70;
        title = NULL;
        large = true;
    }

    if(cv_teamplay.EV==0)
        WI_Draw_Ranking(title, 80 + offx, y + offy, fragtab, scorelines, large,
                        whiteplayer, 32, offy + BASEVIDHEIGHT);
    else
    {
        // draw the frag to the right
//        WI_Draw_Ranking("Individual",170,70,fragtab,scorelines,true,whiteplayer);

        scorelines = HU_Create_TeamFragTbl(fragtab,NULL,NULL);

        // and the team frag to the left
        WI_Draw_Ranking("Teams", 80 + offx, y + offy, fragtab, scorelines, large,
                        players[whiteplayer].skincolor, 32, offy + BASEVIDHEIGHT);
    }

    // [Arcade] Undo the half scale.
    vid.dupx  = sv_dupx;   vid.dupy  = sv_dupy;
    vid.fdupx = sv_fdupx;  vid.fdupy = sv_fdupy;
}


// draw the Crosshair, at the exact center of the view.
//
// Crosshairs are pre-cached at HU_Init
#ifdef HWRENDER // not win32 only 19990829 by Kin
    extern float gr_basewindowcenterx;
    extern float gr_basewindowcentery;
    extern float gr_viewheight;
#endif

// [Arcade] One crosshair per view, in that view's own cell.
//
// This drew exactly two, the second only when cv_splitscreen was set, and
// both at vid.width>>1 -- the middle of the *screen*.  That is right for the
// stacked halves, which span the full width, but a 2x2 cell's centre is not
// the screen's: a quadrant's crosshair would have sat on the boundary between
// two players' views.  And with cv_localplayers the second one never appeared
// at all unless the game was started from the Multiplayer menu, the only
// thing that sets that cvar, so panels 2-4 usually had none.
//
// Each view answers for itself, like HU_Rankings_For_View: the setting is
// read as cv_crosshair[D_Panel_Of(vind)], the panel that player is standing
// at, so it follows them into whichever cell they occupy.
static
void HU_Draw_Crosshair( void )
{
    // vid : from video setup
    byte  vind, num_views = D_NumViews();
    byte  any = 0;
    int   span_w, span_h;   // one cell of the view grid, in screen pixels
    int   base_x, base_y;   // centre of the first view

    for( vind=0; vind<num_views; vind++ )
    {
        if( localplayer[vind] < MAXPLAYERS
            && (cv_crosshair[ D_Panel_Of(vind) ].value & 3) )
        {
            any = 1;
            break;
        }
    }
    if( ! any )
        return;

#if 0
    if (cv_crosshairscale.value)
        V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART );
    else
        V_SetupDraw( 0 | V_SCALEPATCH | V_NOSCALE );
#else
    V_SetupDraw( 0 | V_SCALEPATCH | V_NOSCALE );
#endif

    R_View_Cell_Size( &span_w, &span_h );

    // Centre of the first view.  The software draw tables and the hardware
    // draw window are both left on view 0 by the time HU_Drawer runs, so the
    // other cells are this plus their own offset.
    // reduce this to one rendermode test
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        base_x = (int) gr_basewindowcenterx;
        base_y = (int) gr_basewindowcentery;
    }
    else
#endif
    {
        base_x = view_window_x + (rdraw_viewwidth>>1);
        base_y = view_window_y + (rdraw_viewheight>>1);
    }

    for( vind=0; vind<num_views; vind++ )
    {
        byte cell, col, row;
        int  chv;

        if( localplayer[vind] >= MAXPLAYERS )  continue;   // panel with no player

        chv = cv_crosshair[ D_Panel_Of(vind) ].value & 3;
        if( ! chv )  continue;

        // The panel's cell, not the join order -- see D_View_Cell.
        cell = (num_views >= 2) ? D_View_Cell(vind) : 0;
        col  = (num_views >= 4) ? (cell & 1) : 0;
        row  = (num_views >= 4) ? (cell >> 1) : cell;

        V_DrawTranslucentPatch( base_x + (col * span_w),
                                base_y + (row * span_h),
                                crosshair_patch[chv-1] );
    }
    // V_SetupDraw( drawinfo.prev_screenflags );  // restore
}


//======================================================================
//                    CHAT MACROS COMMAND & VARS
//======================================================================

// better do HackChatmacros() because the strings are NULL !!

consvar_t cv_chatmacro1 = {"_chatmacro1", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro2 = {"_chatmacro2", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro3 = {"_chatmacro3", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro4 = {"_chatmacro4", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro5 = {"_chatmacro5", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro6 = {"_chatmacro6", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro7 = {"_chatmacro7", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro8 = {"_chatmacro8", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro9 = {"_chatmacro9", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro0 = {"_chatmacro0", NULL, CV_SAVE,NULL};


// Set the chatmacros original text, before config is executed.
// If a dehacked patch is loaded, it will change the default text strings.
// The config.cfg strings will override these.
//
void HU_Init_Chatmacros (void)
{
    int    i;

    // this is the original text, dehacked can modify it as a default value
    cv_chatmacro0.defaultvalue = HUSTR_CHATMACRO0;
    cv_chatmacro1.defaultvalue = HUSTR_CHATMACRO1;
    cv_chatmacro2.defaultvalue = HUSTR_CHATMACRO2;
    cv_chatmacro3.defaultvalue = HUSTR_CHATMACRO3;
    cv_chatmacro4.defaultvalue = HUSTR_CHATMACRO4;
    cv_chatmacro5.defaultvalue = HUSTR_CHATMACRO5;
    cv_chatmacro6.defaultvalue = HUSTR_CHATMACRO6;
    cv_chatmacro7.defaultvalue = HUSTR_CHATMACRO7;
    cv_chatmacro8.defaultvalue = HUSTR_CHATMACRO8;
    cv_chatmacro9.defaultvalue = HUSTR_CHATMACRO9;

    // link chatmacros to cvars
    chat_macros[0] = &cv_chatmacro0;
    chat_macros[1] = &cv_chatmacro1;
    chat_macros[2] = &cv_chatmacro2;
    chat_macros[3] = &cv_chatmacro3;
    chat_macros[4] = &cv_chatmacro4;
    chat_macros[5] = &cv_chatmacro5;
    chat_macros[6] = &cv_chatmacro6;
    chat_macros[7] = &cv_chatmacro7;
    chat_macros[8] = &cv_chatmacro8;
    chat_macros[9] = &cv_chatmacro9;

    // register chatmacro vars ready for config.cfg
    for (i=0; i<10; i++)
       CV_RegisterVar (chat_macros[i]);
}


//  chatmacro <0-9> "chat message"
//
void Command_Chatmacro_f (void)
{
    int    i;

    if (COM_Argc()<2)
    {
        CONS_Printf ("chatmacro <0-9> : view chatmacro\n"
                     "chatmacro <0-9> \"chat message\" : change chatmacro\n");
        return;
    }

    i = atoi(COM_Argv(1)) % 10;

    if (COM_Argc()==2)
    {
        CONS_Printf("chatmacro %d is \"%s\"\n",i,chat_macros[i]->string);
        return;
    }

    // change a chatmacro
    CV_Set (chat_macros[i], COM_Argv(2));
}


void HU_Register_Commands( void )
{
    COM_AddCommand ("say"    , Command_Say_f, CC_chat);
    COM_AddCommand ("sayto"  , Command_Sayto_f, CC_chat);
    COM_AddCommand ("sayteam", Command_Sayteam_f, CC_chat);
    Register_NetXCmd(XD_SAY, Got_NetXCmd_Saycmd);
}
