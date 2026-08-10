// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_netcmd.c 1699 2024-11-27 07:20:27Z wesleyjohnson $
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
// $Log: d_netcmd.c,v $
// Revision 1.36  2005/12/20 14:58:25  darkwolf95
// Monster behavior CVAR - Affects how monsters react when they shoot each other
//
// Revision 1.35  2003/10/15 18:53:39  darkwolf95
// "kill" command added to the console
//
// Revision 1.34  2003/07/13 13:16:15  hurdler
//
// Revision 1.33  2002/09/28 06:53:11  tonyd
// fixed CR problem, fixed game options crash
//
// Revision 1.32  2002/09/12 20:10:50  hurdler
// Added some cvars
//
// Revision 1.31  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.30  2001/11/02 21:39:45  judgecutor
// Added Frag's weapon falling
//
// Revision 1.29  2001/08/20 20:40:39  metzgermeister
// Revision 1.28  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.27  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.26  2001/05/21 14:57:04  crashrl
// Readded directory crawling file search function
//
// Revision 1.25  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.24  2001/04/01 17:35:06  bpereira
//
// Revision 1.23  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.22  2001/02/24 13:35:19  bpereira
// Revision 1.21  2001/02/10 12:27:13  bpereira
//
// Revision 1.20  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.19  2000/11/26 20:36:14  hurdler
// Adding autorun2
//
// Revision 1.18  2000/11/11 13:59:45  bpereira
// Revision 1.17  2000/11/02 19:49:35  bpereira
// Revision 1.16  2000/10/08 13:30:00  bpereira
// Revision 1.15  2000/09/10 10:39:06  metzgermeister
// Revision 1.14  2000/08/31 14:30:55  bpereira
// Revision 1.13  2000/08/16 14:10:01  hurdler
// add master server code
//
// Revision 1.12  2000/08/10 14:51:25  ydario
// OS/2 port
//
// Revision 1.11  2000/05/13 19:52:10  metzgermeister
// cd vol jiggle
//
// Revision 1.10  2000/04/23 16:19:52  bpereira
// Revision 1.9  2000/04/16 18:38:07  bpereira
//
// Revision 1.8  2000/04/07 23:11:17  metzgermeister
// added mouse move
//
// Revision 1.7  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.6  2000/03/29 19:39:48  bpereira
//
// Revision 1.5  2000/03/06 15:58:47  hurdler
// Add Bell Kin's changes
//
// Revision 1.4  2000/03/05 17:10:56  bpereira
// Revision 1.3  2000/02/27 00:42:10  hurdler
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      host/client network commands
//      commands are executed through the command buffer
//      like console commands
//      other miscellaneous commands (at the end)
//
//-----------------------------------------------------------------------------

#include "doomincl.h"

#include "console.h"
#include "command.h"

#include "d_netcmd.h"
#include "i_system.h"
  // I_ functions
#include "dstrings.h"
#include "d_main.h"
  // D_ functions
#include "g_game.h"
  // G_ functions
#include "byteptr.h"
  // WRITEBYTE, READBYTE

// cv_ vars and settings from many sources, needed by Command_ functions
#include "hu_stuff.h"
#include "g_input.h"
#include "r_local.h"
#include "r_things.h"
#include "p_inter.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "mserv.h"
#include "v_video.h"

// ------
// protos
// ------
void Command_Color_f(void);
void Command_Name_f(void);

void Command_BindJoyaxis_f();
void Command_UnbindJoyaxis_f();

void Command_WeaponPref(void);

void Got_NetXCmd_NameColor(xcmd_t * xc);
void Got_NetXCmd_WeaponPref(xcmd_t * xc);
void Got_NetXCmd_Mapcmd(xcmd_t * xc);
void Got_NetXCmd_ExitLevelcmd(xcmd_t * xc);
void Got_NetXCmd_LoadGame_cmd(xcmd_t * xc);
void Got_NetXCmd_SaveGame_cmd(xcmd_t * xc);
void Got_NetXCmd_Pause(xcmd_t * xc);
void Got_NetXCmd_UseArtifact(xcmd_t * xc);

void Command_Playdemo_f(void);
void Command_Timedemo_f(void);
void Command_Stopdemo_f(void);
void Command_Map_f(void);
void Command_Restart_f(void);

void Command_Addfile(void);
void Command_Pause(void);

void Command_Frags_f(void);
void Command_TeamFrags_f(void);
void Command_Version_f(void);
void Command_Quit_f(void);

void Command_ExitLevel_f(void);
void Command_Load_f(void);
void Command_Save_f(void);
void Command_ExitGame_f(void);

void Command_Kill(void);


// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================


// [WDJ] Or could just send both when any change is made?
// See Send_PlayerConfig
static
void Send_NameColor1(void)
{
    Send_NameColor_pind(0);
}
static
void Send_NameColor2(void)
{
    Send_NameColor_pind(1);
}

static
void Send_WeaponPref1(void)
{
    Send_WeaponPref_pind(0);
}
static
void Send_WeaponPref2(void)
{
    Send_WeaponPref_pind(1);
}

// Has CV_CFG1 where does not have support for insert into drawmode config file.
// these are just meant to be saved to the config
consvar_t cv_playername[2] = {
  { "name", NULL, CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, NULL, Send_NameColor1 },
  { "name2", "big b", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, NULL, Send_NameColor2 }
};

consvar_t cv_playercolor[2] = {
  { "color", "0", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, Color_cons_t, Send_NameColor1 },
  { "color2", "1", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, Color_cons_t, Send_NameColor2 }
};

// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin[2] = {
  { "skin", DEFAULTSKIN, CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, NULL /*skin_cons_t */ , Send_NameColor1 },
  { "skin2", DEFAULTSKIN, CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, NULL /*skin_cons_t */ , Send_NameColor2 }
};

consvar_t cv_autoaim[2] = {
  { "autoaim",  "1", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, CV_OnOff, Send_WeaponPref1 },
  { "autoaim2", "1", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, CV_OnOff, Send_WeaponPref2 }
};

consvar_t cv_weaponpref[2] = {
  { "weaponpref", "014576328", CV_SAVE | CV_STRING | CV_CALL | CV_NOINIT | CV_CFG1, NULL, Send_WeaponPref1 },
  { "weaponpref2", "014576328", CV_SAVE | CV_STRING | CV_CALL | CV_NOINIT | CV_CFG1, NULL, Send_WeaponPref2 },
};

consvar_t cv_originalweaponswitch[2] = {
  { "originalweaponswitch", "0", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, CV_OnOff, Send_WeaponPref1 },
  { "originalweaponswitch2", "0", CV_SAVE | CV_CALL | CV_NOINIT | CV_CFG1, CV_OnOff, Send_WeaponPref2 }
};
   

consvar_t cv_netstat = { "netstat", "0", 0, CV_OnOff };

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

// Register client and server commands.
void D_Register_ClientCommands(void)
{
    int i;

    for (i = 0; i < NUMSKINCOLORS; i++)
        Color_cons_t[i].strvalue = Color_Names[i];

    //
    // register commands
    //
    Register_NetXCmd(XD_NAMEANDCOLOR, Got_NetXCmd_NameColor);
    Register_NetXCmd(XD_WEAPONPREF, Got_NetXCmd_WeaponPref);
    Register_NetXCmd(XD_MAP, Got_NetXCmd_Mapcmd);
    Register_NetXCmd(XD_EXITLEVEL, Got_NetXCmd_ExitLevelcmd);
    Register_NetXCmd(XD_PAUSE, Got_NetXCmd_Pause);
    Register_NetXCmd(XD_USEARTIFACT, Got_NetXCmd_UseArtifact);

    COM_AddCommand("playdemo", Command_Playdemo_f, CC_command);
    COM_AddCommand("timedemo", Command_Timedemo_f, CC_command);
    COM_AddCommand("stopdemo", Command_Stopdemo_f, CC_command);
    COM_AddCommand("map", Command_Map_f, CC_command);
    COM_AddCommand("restartlevel", Command_Restart_f, CC_command);
    COM_AddCommand("exitgame", Command_ExitGame_f, CC_command);
    COM_AddCommand("exitlevel", Command_ExitLevel_f, CC_command);

    COM_AddCommand("addfile", Command_Addfile, CC_command);
    COM_AddCommand("pause", Command_Pause, CC_command);

    COM_AddCommand("turbo", Command_Turbo_f, CC_command);   // turbo speed
    COM_AddCommand("quit", Command_Quit_f, CC_command);
    COM_AddCommand("screenshot", M_ScreenShot, CC_command);
    COM_AddCommand("kill", Command_Kill, CC_command);

    COM_AddCommand("chatmacro", Command_Chatmacro_f, CC_chat);   // hu_stuff.c
    COM_AddCommand("setcontrol", Command_Setcontrol_f, CC_control);
    COM_AddCommand("setcontrol2", Command_Setcontrol2_f, CC_control);
    COM_AddCommand("bindjoyaxis", Command_BindJoyaxis_f, CC_control);

    COM_AddCommand("version", Command_Version_f, CC_info);
    COM_AddCommand("frags", Command_Frags_f, CC_info);
    COM_AddCommand("teamfrags", Command_TeamFrags_f, CC_info);

    COM_AddCommand("saveconfig", Command_SaveConfig_f, CC_config);
    COM_AddCommand("loadconfig", Command_LoadConfig_f, CC_config);
    COM_AddCommand("changeconfig", Command_ChangeConfig_f, CC_config);


    //Added by Hurdler for master server connection
    MS_Register_Commands();

    // Any cv_ with CV_SAVE needs to be registered, even if it is not used.
    // Otherwise there will be error messages when config is loaded.

    // register these so it is saved to config
    cv_playername[0].defaultvalue = I_GetUserName();
    if (cv_playername[0].defaultvalue == NULL)
        cv_playername[0].defaultvalue = "gi john";

    // Main player
    CV_RegisterVar(&cv_playername[0]);
    CV_RegisterVar(&cv_playercolor[0]);
    CV_RegisterVar(&cv_skin[0]);  // r_things.c (skin NAME)
    CV_RegisterVar(&cv_autoaim[0]);
    CV_RegisterVar(&cv_weaponpref[0]);
    CV_RegisterVar(&cv_originalweaponswitch[0]);

    // Splitscreen player
    CV_RegisterVar(&cv_playername[1]);
    CV_RegisterVar(&cv_playercolor[1]);
    CV_RegisterVar(&cv_skin[1]);
    CV_RegisterVar(&cv_autoaim[1]);
    CV_RegisterVar(&cv_weaponpref[1]);
    CV_RegisterVar(&cv_originalweaponswitch[1]);
   
    //misc
    CV_RegisterVar(&cv_netstat);

    //
    //  The above commands are enough for dedicated server
    //
    if (dedicated)
        return;

    COM_AddCommand("load", Command_Load_f, CC_savegame);
    Register_NetXCmd(XD_LOADGAME, Got_NetXCmd_LoadGame_cmd);
    COM_AddCommand("save", Command_Save_f, CC_savegame);
    Register_NetXCmd(XD_SAVEGAME, Got_NetXCmd_SaveGame_cmd);

    // add cheat commands, I'm bored of deh patches renaming the idclev ! :-)
    COM_AddCommand("noclip", Command_CheatNoClip_f, CC_cheat);
    COM_AddCommand("god", Command_CheatGod_f, CC_cheat);
    COM_AddCommand("gimme", Command_CheatGimme_f, CC_cheat);

/* ideas of commands names from Quake
    "status"
    "notarget"
    "fly"
    "changelevel"
    "reconnect"
    "tell"
    "kill"
    "spawn"
    "begin"
    "prespawn"
    "ping"

    "startdemos"
    "demos"
    "stopdemo"
*/

}

//--- string
// [WDJ] The compiler will likely inline these.
// The macro versions were unreadable, and thus unmaintainable.

// Only use this on internal strings that are known to have 0 term.
// Will always terminate the string.
// Return next buffer location.
byte *  write_string(byte *dst, const char* src)
{
  // copy src str0 to buffer dst, until reach 0 term.
  do {
    WRITECHAR(dst, *src);
  }
  while ( *(src++) );
  return dst;
}

// Will always terminate the string.
// Return next buffer location
byte *  write_stringn( byte *dst, const char* src, int num )
{
  // copy src str0 to buffer dst, until reach 0 term or num of char reached.
  for(;;) {
    WRITECHAR(dst, *src);
    if ( *(src++) == 0 )  break;
    num--;
    if(num == 0) {  // do not exceed num char
      dst[-1] = 0;  // overwrite last char with 0
      break;
    }
  }
  return dst;
}


// =========================================================================
//                            CLIENT STUFF
// =========================================================================

// [WDJ] Currently, these are being sent without cv_splitscreen knowledge,
// so when not splitscreen, they may be mysterious settings to other nodes.


#if 0
// By Server
//   pn : player pid
void Send_NameColor_player( byte pn, byte pind )
{
    player_t * plp = & players[pn];
    const char * skinname = ( skins[plp->skin] )? skins[plp->skin]->name : NULL;
    Send_NameColor_pn( pn, player_names[pn], plp->skincolor, skinname, pind );
}
#endif


// By Client.
//  name, color, or skin has changed
//  pind : player index, [0]=main player, [1]=splitscreen player
void  Send_NameColor_pind( byte pind )
{
    byte pn = localplayer[pind];
    if( pn < MAXPLAYERS )
        Send_NameColor_pn( pn, cv_playername[pind].string, cv_playercolor[pind].EV, cv_skin[pind].string, pind );
}

// Server, Client
//   playername : player name
//   skinname : skin name, NULL if not skins
//   textcmd_pind : textcmd channel index, [0]=main player, [1]=splitscreen player, [2]=server (bots)
void  Send_NameColor_pn( byte pn, const char * playername, byte color, const char * skinname, byte textcmd_pind )
{
    byte buf[MAXPLAYERNAME + 1 + SKINNAMESIZE + 1];
    byte *p;

    p = buf;
    // Format:  color byte, player_name str0, skin_name str0.
    WRITEBYTE(p, color);
    p = write_stringn(p, playername, MAXPLAYERNAME);

    // Send the skin by name.
    // Check if player has the skin loaded
    // (it may be the name of a skin that was available in the previous game).
    if( (! skinname) || (! R_SkinAvailable( skinname )) )
        skinname = DEFAULTSKIN;
    p = write_stringn(p, skinname, SKINNAMESIZE);

    // Automatic routing, for Clients, and Bots.
    Send_NetXCmd_auto(XD_NAMEANDCOLOR, buf, (p - buf), textcmd_pind, pn);
}

void Got_NetXCmd_NameColor(xcmd_t * xc)
{
    int  pn = xc->playernum;
    char * lcp = (char*)xc->curpos; // local cp
    char * pname;
    player_t * p;
    byte  sk;
   
    if( pn >= MAXPLAYERS )
    {
        GenPrintf( EMSG_error, "NameColor: invalid player num %i\n", pn );       
        return;
    }

    pname = player_names[pn];
    p = &players[pn];

    // Format:  color byte, player_name str0, skin_name str0.
    // color
    sk = READBYTE(lcp); // unsigned read
    P_SetPlayer_color( p, sk );

    // Players 0..(MAXPLAYERS-1) are init as Player 1 ..
    // name
    if( EV_legacy >= 128 )
    {
        // compacted string space in message
        if (strcasecmp(pname, lcp))
            CONS_Printf("%s renamed to %s\n", pname, lcp);
        // [WDJ] String overflow safe
        {
            int pn_len = strlen( lcp ) + 1;
            int read_len = min( pn_len, MAXPLAYERNAME-1 );  // length safe
            memcpy(pname, lcp, read_len);
            pname[MAXPLAYERNAME-1] = '\0';
            lcp += pn_len;  // whole
        }
    }
    else
    {
        // constant string space in message
        memcpy(pname, lcp, MAXPLAYERNAME);
        lcp += MAXPLAYERNAME;
    }

    // Protection against malicious packet.
    if( (byte*)lcp >= xc->endpos )  goto done;

    // skin
    if( EV_legacy < 120 || EV_legacy >= 125 )
    {
        if( EV_legacy >= 128 )
        {
            // compacted string space in message
            SetPlayerSkin(pn, lcp);
            SKIPSTRING(lcp);
        }
        else
        {
            // constant string space in message
            SetPlayerSkin(pn, lcp);
            lcp += (SKINNAMESIZE + 1);
        }
    }
done:
    xc->curpos = (byte*)lcp;  // OUT once
}

//  pind : player index, [0]=main player, [1]=splitscreen player
void Send_WeaponPref_pind( byte pind )
{
    char buf[NUMWEAPONS + 4];  // need NUMWEAPONS+2

    // Format: original_weapon_switch  byte,
    //         weapon_pref  char[NUMWEAPONS],
    //         autoaim  byte.
    buf[0] = cv_originalweaponswitch[pind].value;
   
    int wplen = strlen(cv_weaponpref[pind].string);
    memcpy(buf + 1, cv_weaponpref[pind].string, wplen);
    if( wplen != NUMWEAPONS)
    {
        CONS_Printf("weaponpref invalid length: %d, should be %d, player pind=%d\n", wplen, NUMWEAPONS, pind);
        // pad with 0
        for( ; wplen < NUMWEAPONS; wplen++ )  buf[wplen+1] = '0';
    }
    buf[1 + NUMWEAPONS] = cv_autoaim[pind].value;

    Send_NetXCmd_pind(XD_WEAPONPREF, buf, NUMWEAPONS + 2, pind);
}

void Got_NetXCmd_WeaponPref(xcmd_t * xc)
{
    player_t * p = &players[xc->playernum];
    // Format: original_weapon_switch  byte,
    //         weapon_pref  char[NUMWEAPONS],
    //         autoaim  byte.
    p->GF_flags &= ~(GF_original_weapon|GF_autoaim);
    if( *(xc->curpos++) )  p->GF_flags |= GF_original_weapon;
    memcpy(p->favoritweapon, xc->curpos, NUMWEAPONS);
    xc->curpos += NUMWEAPONS;
    if( *(xc->curpos++) )  p->GF_flags |= GF_autoaim;
}

void D_Send_PlayerConfig(void)
{
    Send_NameColor_pind(0);
    Send_WeaponPref_pind(0);
    if( cv_splitscreen.EV && ( localplayer[1] < MAXPLAYERS ))
    {
        Send_NameColor_pind(1);
        Send_WeaponPref_pind(1);
    }
}

// ========================================================================

//  play a demo, add .lmp for external demos
//  eg: playdemo demo1 plays the internal game demo
//

void Command_Playdemo_f(void)
{
    char name[MAX_WADPATH];  // MAX_WADPATH for length checking
    COM_args_t  carg;
    
    COM_Args( &carg );

    if (carg.num != 2)
    {
        CONS_Printf("playdemo <demoname> : playback a demo\n");
        return;
    }

    // disconnect from server here ?
    if (demoplayback)
        G_StopDemo();
    // Ignore seq playdemo command issued during menu, if since disabled
    if( demo_ctrl == (DEMO_seq_playdemo | DEMO_seq_disabled))
    {
        demo_ctrl = DEMO_seq_disabled;
        return;
    }
    demo_ctrl &= ~ DEMO_seq_playdemo;
    if (netgame)
    {
        CONS_Printf("\nYou can't play a demo while in net game\n");
        return;
    }

    // copy demo lump name, or demo file name (.lmp will be added later)
    dl_strncpy(name, carg.arg[1], MAX_WADPATH);
    // dont add .lmp so internal game demos can be played

    CONS_Printf("Playing back demo '%s'.\n", name);

    G_DoPlayDemo(name);
}

void Command_Timedemo_f(void)
{
    char name[MAX_WADPATH];  // MAX_WADPATH for length checking
    COM_args_t  carg;
    
    COM_Args( &carg );

    if (carg.num != 2)
    {
        CONS_Printf("timedemo <demoname> : time a demo\n");
        return;
    }

    // disconnect from server here ?
    if (demoplayback)
        G_StopDemo();
    if (netgame)
    {
        CONS_Printf("\nYou can't play a demo while in net game\n");
        return;
    }

    // copy demo lump name, or demo file name (.lmp will be added later)
    dl_strncpy(name, carg.arg[1], MAX_WADPATH);
    // dont add .lmp so internal game demos can be played

    CONS_Printf("Timing demo '%s'.\n", name);

    G_TimeDemo(name);
}

//  stop current demo
//
void Command_Stopdemo_f(void)
{
    G_CheckDemoStatus();
    CONS_Printf("Stopped demo.\n");
}

//  Warp to map code.
//  Called either from map <mapname> console command, or idclev cheat.
//
void Command_Map_f(void)
{
    // Build complex net command in buf.
    char buf[MAX_WADPATH + 3];
#define MAPNAME (&buf[2])
    int i;
    COM_args_t  carg;
    
    COM_Args( &carg );

    if (carg.num < 2 || carg.num > 7)
    {
        CONS_Printf("map <mapname[.wad]> [-skill <1..5>] [-monsters <0/1>] [-noresetplayers]: warp to map\n");
        return;
    }

    if (!server)
    {
        CONS_Printf("Only the server can change the map\n");
        return;
    }

    // By Server.
    dl_strncpy(MAPNAME, carg.arg[1], MAX_WADPATH);

    if (FIL_CheckExtension(MAPNAME))
    {
        // here check if file exist !!!
        // Owner security permissions.
        if (!findfile(MAPNAME, NULL, false, NULL))
        {
            CONS_Printf("\2File %s' not found\n", MAPNAME);
            return;
        }
    }
    else
    {
        // internal wad lump
        if( ! VALID_LUMP( W_CheckNumForName(MAPNAME) ) )
        {
            CONS_Printf("\2Internal game map '%s' not found\n" "(use .wad extension for external maps)\n", MAPNAME);
            return;
        }
    }

    // Format: skill byte, (no_reset_players, no_monsters) byte,
    //         map_name str0.

    // Options of the map command.
    if ((i = COM_CheckParm("-skill")) != 0)
        buf[0] = atoi(COM_Argv(i + 1)) - 1;
    else
        buf[0] = gameskill;

    // Signal using single bits.
    //  bit 0: no monsters
    //  bit 1: no reset players
    buf[1] = 0;
    if ((i = COM_CheckParm("-monsters")) != 0)
    {
        if( atoi(COM_Argv(i + 1)) == 0 )
            buf[1] = 0x01;
    }
    else if( nomonsters )
        buf[1] = 0x01;

    if (COM_CheckParm("-noresetplayers"))
        buf[1] |= 0x02;

    // Spawn the server if needed.
    // When that detects a new player, then Reset players.
    if (SV_SpawnServer())
    {
        // Added a new player.
        buf[1] &= ~0x02;
    }

#ifdef WAIT_GAME_START_INTERMISSION
    if(server && netgame && num_wait_game_start)
    {
#if 1
        // [WDJ] 1.48 Warn user that Map command does not handle waiting players.       
        GenPrintf(EMSG_warn,"Waiting players: use exitlevel\n");
#else
#if 0
// [WDJ] The Map command will override any attempt to stay in Intermission, so THIS DOES NOT WORK.
        // [WDJ] Adding players seems to only work using Intermission.
        if( gamestate != GS_INTERMISSION )
        {
            G_ExitLevel();
            if( gamestate == GS_LEVEL )
                G_DoCompleted ();
            G_Start_Intermission();  // setup intermission
        }
#endif
        // Activate waiting clients
        // TODO: make this work.
        // Server won't stay in Intermission, client gets stuck in Intermission.
        SV_Add_game_start_waiting_players( 1 );
#endif
    }
#endif

    SV_Send_NetXCmd(XD_MAP, buf, 2 + strlen(MAPNAME) + 1); // as server
}

void Got_NetXCmd_Mapcmd(xcmd_t * xc)
{
    char mapname[MAX_WADPATH];
    byte opt, skill;
    int  resetplayer = 1;

    // Format: skill byte, (no_reset_players, no_monsters) byte,
    //         map_name str0.
    skill = READBYTE(xc->curpos);
    if( EV_legacy >= 128 )
    {
        // [WDJ] Do not use boolean nomonsters as an int.
        opt = READBYTE(xc->curpos);
        if( EV_legacy >= 129 )
        {
            nomonsters = ( (opt & 0x01) != 0 );
            resetplayer = ( (opt & 0x02) == 0 );
        }
        else
        {
            nomonsters = (opt > 0);
        }
    }
    dl_strncpy(mapname, (char*)xc->curpos, MAX_WADPATH);

    xc->curpos += strlen(mapname) + 1;

    CONS_Printf("Warping to map...\n");
    if (demoplayback && !timingdemo)
        precache = false;
    G_InitNew(skill, mapname, resetplayer);
    if (demoplayback && !timingdemo)
        precache = true;
    CON_ToggleOff();
    if (timingdemo)
        G_DoneLevelLoad();
}

void Command_Restart_f(void)
{
    if (netgame)
    {
        CONS_Printf("Restartlevel don't work in network\n");
        return;
    }

    if (gamestate == GS_LEVEL)
        G_DoLoadLevel(true);
    else
        CONS_Printf("You should be in a level to restart it !\n");
}

// Command, or KEY_PAUSE
void Command_Pause(void)
{
    char buf;
    // Format: (pause) byte.
    if (COM_Argc() > 1)
        buf = atoi(COM_Argv(1)) != 0;
    else
        buf = !paused;

    Send_NetXCmd(XD_PAUSE, &buf, 1);  // as mainplayer
}

void Got_NetXCmd_Pause(xcmd_t * xc)
{
    // Format: (pause) byte.
    if( EV_legacy < 131 )
        paused ^= 1;
    else
        paused = READBYTE(xc->curpos);

    if (!demoplayback)
    {
        if (netgame)
        {
            char * bystr = player_names[xc->playernum];
            if (paused)
                GenPrintf(EMSG_hud, "Game paused by %s\n", bystr);
            else
                GenPrintf(EMSG_hud, "Game unpaused by %s\n", bystr);
        }

        if (paused)
        {
            if (!menuactive || netgame)
                S_PauseSound();
        }
        else
            S_ResumeSound();

        // Pause updates mouse, grab.
        I_StartupMouse( !(paused || menuactive) );
    }
}

//  Add a pwad at run-time
//  Search for sounds, maps, musics, etc..
//
void Command_Addfile(void)
{
    if (COM_Argc() != 2)
    {
        CONS_Printf("addfile <wadfile.wad> : load wad file\n");
        return;
    }

    P_AddWadFile(COM_Argv(1), NULL);
}

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

void Command_Frags_f(void)
{
    int i, j;

    if( ! deathmatch )
    {
        CONS_Printf("Frags : show the frag table\n");
        CONS_Printf("Only for deathmatch games\n");
        return;
    }

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            CONS_Printf("%-16s", player_names[i]);
            for (j = 0; j < MAXPLAYERS; j++)
                if (playeringame[j])
                    CONS_Printf(" %3d", players[i].frags[j]);
            CONS_Printf("\n");
        }
    }
}

void Command_TeamFrags_f(void)
{
    int i, j;
    fragsort_t unused[MAXPLAYERS];
    int frags[MAXPLAYERS];
    int fragtbl[MAXPLAYERS][MAXPLAYERS];

    if( ! (deathmatch && cv_teamplay.EV) )
    {
        CONS_Printf("teamfrags : show the frag table for teams\n");
        CONS_Printf("Only for deathmatch teamplay games\n");
        return;
    }

    HU_Create_TeamFragTbl(unused, frags, fragtbl);

    for (i = 0; i < 11; i++)
    {
        if (teamingame(i))
        {
            CONS_Printf("%-8s", get_team_name(i));
            for (j = 0; j < 11; j++)
                if (teamingame(j))
                    CONS_Printf(" %3d", fragtbl[i][j]);
            CONS_Printf("\n");
        }
    }
}

//  Returns program version.
//
void Command_Version_f(void)
{
  CONS_Printf("%s (" __DATE__ " " __TIME__ ")\n", VERSION_BANNER);
}

//  Quit the game immediately
//
void Command_Quit_f(void)
{
    I_Quit();  // No return
}


void Command_ExitLevel_f(void)
{
    if (!server)
    {
        CONS_Printf("Only the server can exit the level\n");
        return;
    }

    // By Server.
    if (gamestate != GS_LEVEL || demoplayback)
        CONS_Printf("You should be in a level to exit it !\n");

    SV_Send_NetXCmd(XD_EXITLEVEL, NULL, 0);  // as server
}

void Got_NetXCmd_ExitLevelcmd(xcmd_t * xc)
{
    G_ExitLevel();
}

void Command_Load_f(void)
{
    byte slot;

    if (COM_Argc() != 2)
    {
        CONS_Printf("load <slot>: load a saved game\n");
        return;
    }

    if (!server)
    {
        CONS_Printf("Only server can do a load game\n");
        return;
    }

    // By Server.
    D_DisableDemo();

    // spawn a server if needed
    SV_SpawnServer();

    // Format: save_slot byte.
    slot = atoi(COM_Argv(1));
    SV_Send_NetXCmd(XD_LOADGAME, &slot, 1); // as server
}

void Got_NetXCmd_LoadGame_cmd(xcmd_t * xc)
{
    // Format: save_slot byte.
    byte slot = *(xc->curpos++);
    G_DoLoadGame(slot);
}

void Command_Save_f(void)
{
    char p[SAVESTRINGSIZE + 1];

    if (COM_Argc() != 3)
    {
        CONS_Printf("save <slot> <description>: save game\n");
        return;
    }

    if (!server)
    {
        CONS_Printf("Only server can do a save game\n");
        return;
    }

    // Format: save_slot byte, save_description str0.
    // By Server.
    p[0] = atoi(COM_Argv(1));  // slot num 0..99
    // save description string at p[1], p[SAVESTRINGSIZE + 1]
    dl_strncpy(&p[1], COM_Argv(2), SAVESTRINGSIZE);

    SV_Send_NetXCmd(XD_SAVEGAME, &p, strlen(&p[1]) + 2);  // as server
}

void Got_NetXCmd_SaveGame_cmd(xcmd_t * xc)
{
    byte slot;
    char description[SAVESTRINGSIZE];

    // Format: save_slot byte, save_description str0.
    slot = *(xc->curpos++);
    // Transmitted as SAVESTRINGSIZE, but protect against net error or attack.
    dl_strncpy(description, (char*)xc->curpos, SAVESTRINGSIZE);
    xc->curpos += strlen(description) + 1;

    // Write the save game file
    G_DoSaveGame(slot, description);
}

void Command_ExitGame_f(void)
{
    D_Quit_NetGame();
    CL_Reset();
    D_StartTitle();
}

void Got_NetXCmd_UseArtifact(xcmd_t * xc)
{
    // Format: artifact  byte.
    byte art = READBYTE(xc->curpos);
    P_PlayerUseArtifact(&players[xc->playernum], art);
}

void Command_Kill(void)
{
    P_KillMobj(players[consoleplayer].mo, NULL, players[consoleplayer].mo);
}
