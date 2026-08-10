// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// $Id: t_func.c 1732 2025-03-06 13:20:06Z wesleyjohnson $
//
// Copyright (C) 2000 Simon Howard
// Copyright (C) 2001-2016 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log: t_func.c,v $
// Revision 1.40  2005/11/07 22:54:39  iori_
// Kind of redundant unless we want a 1.43 release sometime.
//
// PlayerPitch - enabled setting the player's pitch
// ObjAngle - Enabled/Fixed setting the player's angle (was broken). May not work for MP..
// SectorEffect - kind of limited, but useful I guess. Incomplete (secret, dmg sectors)
//
// Revision 1.39  2005/05/21 08:41:23  iori_
// May 19, 2005 - PlayerArmor FS function;  1.43 can be compiled again.
//
// Revision 1.38  2004/09/17 23:04:48  darkwolf95
// playerkeysb (see comment), waittic and clocktic
//
// Revision 1.37  2004/08/26 10:53:51  iori_
// warpmap fs function
//
// Revision 1.36  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.35  2004/03/06 17:25:04  darkwolf95
// SetObjPosition to work around spawning and removing objects
//
// Revision 1.34  2003/12/07 14:59:40  darkwolf95
// changed objstate to set state and return result of function, user can use other clues to find out what the state of an object is
//
// Revision 1.33  2003/11/21 16:15:27  darkwolf95
// small fix to resurrect
//
// Revision 1.32  2003/11/16 02:01:33  darkwolf95
// objheal(): so there's no pain state or noise; resurrect(): completely bring 'em back
//
// Revision 1.31  2003/11/15 22:04:42  darkwolf95
// Added objstate(), which is modifed from Exl's code and playerselwep() to complement playerweapon().
//
// Revision 1.30  2003/10/15 14:57:04  darkwolf95
// added ability to set with objangle()
//
// Revision 1.29  2003/10/08 15:13:02  darkwolf95
// Small fix - spawnmissile now returns mobj
//
// Revision 1.28  2003/08/23 14:07:26  darkwolf95
// added gameskill() and fixed setcamera pitch
//
// Revision 1.27  2003/07/23 17:26:36  darkwolf95
// SetLineTexture function for Fraggle Script
//
// Revision 1.26  2003/07/21 11:33:57  hurdler
// Revision 1.25  2003/07/13 13:16:15  hurdler
//
// Revision 1.24  2003/05/30 22:44:07  hurdler
// add checkcvar function to FS
//
// Revision 1.23  2003/05/26 18:02:29  darkwolf95
// added playeraddfrag, skincolor, testlocation and radiusattack functions
//
// Revision 1.22  2003/04/21 19:55:26  darkwolf95
// Added playdemo, spawnmissle, mapthings, objtype, mapthingnumexist, and playerweapon.
//
// Revision 1.21  2002/09/07 16:46:47  hurdler
// Fix respawning things bug using FS
//
// Revision 1.20  2002/07/28 17:11:33  hurdler
// Change version number to reflect the big changes since v.30
//
// Revision 1.19  2002/06/30 21:37:48  hurdler
// Ready for 1.32 beta 5 release
//
// Revision 1.18  2002/06/15 13:39:26  ssntails
//
// Revision 1.17  2002/06/14 02:20:06  ssntails
// New FS function (SoM Request)
//
// Revision 1.16  2002/05/19 19:44:44  hurdler
// Revision 1.14  2002/01/05 16:39:19  hurdler
//
// Revision 1.13  2002/01/05 00:58:10  hurdler
// fix compiling problem when not using hwrender
//
// Revision 1.12  2001/12/31 14:44:50  hurdler
// Last fix for beta 4
//
// Revision 1.11  2001/12/31 13:47:46  hurdler
// Add setcorona FS command and prepare the code for beta 4
//
// Revision 1.10  2001/12/28 16:57:30  hurdler
// Add setcorona command to FS
//
// Revision 1.9  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.8  2001/08/14 00:36:26  hurdler
//
// Revision 1.7  2001/08/06 23:57:10  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.6  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.5  2001/03/21 18:24:56  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.4  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.3  2000/11/09 17:56:20  stroggonmeth
// Hopefully fixed a few bugs and did a few optimizations.
//
// Revision 1.2  2000/11/04 16:23:44  bpereira
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------
//
// Functions
//
// functions are stored as variables(see variable.c), the
// value being a pointer to a 'handler' function for the
// function. Arguments are stored in an argc/argv-style list
//
// this module contains all the handler functions for the
// basic FraggleScript Functions.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/* includes ************************/

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "info.h"
#include "infoext.h"
#include "m_random.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "p_spec.h"
//#include "p_hubs.h"
#include "p_inter.h"
#include "r_data.h"
#include "r_main.h"
#include "r_segs.h"
#include "r_defs.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_setup.h"
#include "d_think.h"
#include "i_video.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_script.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"
#include "t_array.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
  // lspr
#endif


fs_value_t  evaluate_expression(int start, int stop);


// array functions in t_array.c
void SF_NewArray(void);          // impls: array newarray(...)
void SF_NewEmptyArray(void);     // impls: array newemptyarray(...)
void SF_ArrayCopyInto(void);     // impls: void copyinto(array, array)
void SF_ArrayElementAt(void);    // impls: 'a elementat(array, int)
void SF_ArraySetElementAt(void); // impls: void setelementat(array, int, 'a)
void SF_ArrayLength(void);       // impls: int length(array)


// return a Z_Malloc string of the args( i1.. ) concatenated
// The Z_Malloc string has 3 extra chars allocated, to allow an append.
// The return string must be freed.
char *  Z_cat_args( int i1 )
{
    int strsize = 0;
    int i;
   
    for (i = i1; i < t_argc; i++)
        strsize += strlen(stringvalue(t_argv[i]));

    char * tempstr = Z_Malloc(strsize + 4, PU_IN_USE, 0);
    tempstr[0] = '\0';

    for (i = i1; i < t_argc; i++)
    {
        strcat(tempstr, stringvalue(t_argv[i])); // append
    }

    return tempstr;
}

// Some error handling
void  wrong_num_arg( const char * funcname, int num_args )
{
    script_error("%s: wrong num arg (%i)\n", funcname, num_args);
}

void  missing_arg( const char * funcname, int min_num_args )
{
    script_error("%s: missing arg (%i)\n", funcname, min_num_args);
}

void  missing_arg_str( const char * funcname, const char * argstr )
{
    script_error("%s: missing arg (%s)\n", funcname, argstr);
}

static
void  player_not_in_game( const char * funcname, byte playernum )
{
    // [WDJ] No reason for script to fail because a player is not in game
#if 0
    script_error("%s: player %i not in game\n", funcname, playernum);
#endif
}

static
byte  arg_playernum( fs_value_t * arg, const char * funcname )
{
    unsigned int playernum;
    if ( arg->type == FSVT_mobj)
    {
        if (! arg->value.mobj->player)  goto mobj_err;
        playernum = arg->value.mobj->player - players;
    }
    else
        playernum = intvalue(*arg);

    if ( playernum >= MAXPLAYERS
         || ! playeringame[playernum] )  goto player_err;
    return playernum;

mobj_err:
    script_error("%s: mobj arg not a player!\n", funcname);
    goto errexit;

player_err:
    player_not_in_game( funcname, playernum );
    goto errexit;

errexit:
    return  255;  //  > MAXPLAYERS, indicates error
}




// functions. SF_ means Script Function not, well.. heh, me

        /////////// actually running a function /////////////

/*******************
  FUNCTIONS
 *******************/

// Actual handler functions for the script functions

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command line options.

// values can be returned from the functions using
// the variable 't_return'

void SF_Print(void)
{
    int i;

    if (!t_argc)
        return;

    for (i = 0; i < t_argc; i++)
    {
        GenPrintf(EMSG_playmsg, "%s", stringvalue(t_argv[i]));
    }
}

// return a random number from 0 to 255
void SF_Rnd(void)
{
    t_return.type = FSVT_int;
    t_return.value.i = rand() % 256;
}

// return a random number from 0 to 255
void SF_PRnd(void)
{
    t_return.type = FSVT_int;
    // Legacy Fragglescript use of P_Random, not in Doom.
    t_return.value.i = PP_Random(pL_PRnd);
}

// Find the next outermost
// loop we are currently in and return the section_t for it.
fs_section_t * in_looping_section(void)
{
    // deepest level loop we're in that has been found so far
    fs_section_t * loopfnd = NULL;
    int n;

    // check thru all the hashchains
    for (n = 0; n < SECTIONSLOTS; n++)
    {
        fs_section_t *current = fs_current_script->sections[n];

        // check all the sections in this hashchain
        while (current)
        {
            if (current->type == FSST_loop) // a loop?
            {
                // check to see if it's a loop that we're inside
                if (fs_src_cp >= current->start && fs_src_cp <= current->end)
                {
                    // a deeper nesting level than already found ?
                    if (!loopfnd || (current->start > loopfnd->start))
                        loopfnd = current; // save it
                }
            }
            current = current->next;
        }
    }

    return loopfnd;        // return the closest one found
}

// "continue;" in FraggleScript is a function
void SF_Continue(void)
{
    fs_section_t *section;

    if (!(section = in_looping_section()))  goto err_notloop; // no loop found

    fs_src_cp = section->end;       // jump to the closing brace
    return;
   
err_notloop:
    script_error("Continue: not in loop\n");
    return;
}

void SF_Break(void)
{
    fs_section_t *section;

    if (!(section = in_looping_section()))  goto err_notloop;

    fs_src_cp = section->end + 1;   // jump out of the loop
    return;

err_notloop:
    script_error("Break: not in loop\n");
    return;
}

// Goto( label )
void SF_Goto(void)
{
    if (t_argc != 1)  goto err_numarg;

    // check argument is a labelptr
    if (t_argv[0].type != FSVT_label)  goto err_not_label;

    // go there then if everythings fine
    fs_src_cp = t_argv[0].value.labelptr;
done:   
    return;

err_numarg:
    wrong_num_arg("Goto", 1);
    goto done;

err_not_label:
    script_error("Goto: argument not a label\n");
    goto done;
}

void SF_Return(void)
{
    fs_killscript = true;  // kill the script
}

// Include( name )
void SF_Include(void)
{
    char tempstr[9];

    if (t_argc != 1)  goto err_numarg;

    memset(tempstr, 0, 9);

    if (t_argv[0].type == FSVT_string)
        strncpy(tempstr, t_argv[0].value.s, 8);
    else
        snprintf(tempstr, 8, "%s", stringvalue(t_argv[0]));
    tempstr[8] = '\0';

    parse_include(tempstr);
done:
    return;
   
err_numarg:
    wrong_num_arg("Include", 1);
    goto done;
}

void SF_Input(void)
{
#if 1
   // [WDJ] was disabled in 143beta_macosx
   // Doing gets() will probably freeze program until user cancels the input.
   // If it does errmsg "not available in doom", then why do it, FIXME maybe ??
        static char inputstr[128];
        // [WDJ] NEVER use gets(), it can overrun the buffer, use fgets()
        fgets(inputstr, 128, stdin);

        t_return.type = FSVT_string;
        t_return.value.s = inputstr;
#endif
    CONS_Printf("input() function not available in doom\a\n");
}

void SF_Beep(void)
{
    CONS_Printf("\3");
}

void SF_Clock(void)
{
    t_return.type = FSVT_int;
    t_return.value.i = (gametic * 100) / 35;
}

void SF_ClockTic(void)
{
    t_return.type = FSVT_int;
    t_return.value.i = gametic;
}

    /**************** doom stuff ****************/

void SF_ExitLevel(void)
{
    G_ExitLevel();
}

// 08/25/04 iori: warp(<skill>, <"map">, [reset 0|1]);
// Warp( skill, mapname, {reset} ) 
// skill= 1..5, reset= 0,1
void SF_Warp(void)
{
    int reset = 1;
    int skill;

    if(t_argc < 2)  goto err_numarg;

    skill = t_argv[0].value.i;
    if (skill < 1 || skill > 5)  goto err_skill;

    if(t_argc > 2)
    {
        reset = t_argv[2].value.i;
        if(reset != 0 && reset != 1)  goto err_reset;
    }
    // skill 0..4
    G_InitNew(skill - 1, t_argv[1].value.s, reset);
done:
    return;
   
err_numarg:
    missing_arg("Warp", 2);  // 2, 3
    goto done;

err_skill:
    script_error("Warp: Skill must be between 1 and 5.\n");
    goto done;
   
err_reset:
    script_error("Warp: Reset must be either 0 or 1.\n");
    goto done;
}
   
// centered msg, default display time
// To all players
// Tip( tipstring ... )   upto 128 strings
void SF_Tip(void)
{
    if (fs_current_script->trigger->player == displayplayer_ptr)
    {
        char * tempstr = Z_cat_args(0);  // concat arg0, arg1, ...
        HU_SetTip(tempstr, 53);
        Z_Free(tempstr);
    }
    return;
}

// SoM: Timed tip!
// To all players
// TimedTip( tiptime, tipstring, ... )   upto 127 strings
void SF_TimedTip(void)
{
    int tiptime;

    if (t_argc < 2)  goto err_numarg;

    tiptime = (intvalue(t_argv[0]) * 35) / 100;

    if (fs_current_script->trigger->player == displayplayer_ptr)
    {
        char * tempstr = Z_cat_args(1);  // concat arg1, arg2, ...
        //debug_Printf("%s\n", tempstr);
        HU_SetTip(tempstr, tiptime);
        Z_Free(tempstr);
    }
done:
    return;
   
err_numarg:
    missing_arg("TimedTip", 2);  // 2 or more, upto 128
    goto done;
}

// tip to a particular player
// PlayerTip( playernum, tipstring, ... )  upto 127 strings
// No restriction on playernum, contrary to docs.
void SF_PlayerTip(void)
{
    unsigned int plnum;

    if (t_argc < 2)  goto err_numarg;  // [WDJ] requires 2 args

    plnum = intvalue(t_argv[0]);
    if ( plnum == consoleplayer )
    {
        char * tempstr = Z_cat_args(1);  // concat arg1, arg2, ...
        //debug_Printf("%s\n", tempstr);
        HU_SetTip(tempstr, 53);
        Z_Free(tempstr);
    }
done:
    return;
   
err_numarg:
    missing_arg("PlayerTip", 2);  // 2 or more, upto 128
    goto done;
}

// message to all players
// Message( msgstring, ... )   upto 128 strings
void SF_Message(void)
{
    if (fs_current_script->trigger->player == displayplayer_ptr)
    {
        char * tempstr = Z_cat_args(0);  // concat arg0, arg1, ...
        GenPrintf(EMSG_playmsg, "%s\n", tempstr);
        Z_Free(tempstr);
    }
}


//DarkWolf95:July 28, 2003:Added unimplemented function
// Return skill 1..5
void SF_GameSkill(void)
{
    t_return.type = FSVT_int;
    t_return.value.i = gameskill + 1;  //make 1-5, rather than 0-4
}

// Returns what type of game is going on - Deathmatch, CoOp, or Single Player.
// Feature Requested by SoM! SSNTails 06-13-2002
// GameMode()
// Return 0=Single, 1=Coop, 2=Deathmatch
void SF_GameMode(void)
{
    t_return.type = FSVT_int;

    if( deathmatch )    // deathmatch!
        t_return.value.i = 2;
    else if (netgame || multiplayer)    // Cooperative
        t_return.value.i = 1;
    else        // Single Player
        t_return.value.i = 0;

    return;
}

// message to a particular player
// PlayerMsg( playernum, msgstring, ... )  upto 127 strings
// No restriction on playernum, contrary to docs.
void SF_PlayerMsg(void)
{
    unsigned int plnum;

    if (t_argc < 2)  goto err_numarg;

    plnum = intvalue(t_argv[0]);
    if( (plnum == displayplayer) || (plnum == displayplayer2) )
    {
        char * tempstr = Z_cat_args(1);  // concat arg1, arg2, ...
        GenPrintf( (plnum == displayplayer)? EMSG_playmsg: EMSG_playmsg2,
                   "%s\n", tempstr);
        Z_Free(tempstr);
    }
done:
    return;
   
err_numarg:
    missing_arg("PlayerMsg", 2);  // 2 or more, upto 128
    goto done;
}

// PlayerInGame( playernum )
void SF_PlayerInGame(void)
{
    unsigned int plnum;

    if (t_argc != 1)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0; // default

    plnum = intvalue(t_argv[0]);
    if( plnum < MAXPLAYERS )  // [WDJ] safe limit
        t_return.value.i = playeringame[plnum];
done:
    return;
   
err_numarg:
    wrong_num_arg("PlayerInGame", 1);
    goto done;
}

// PlayerName( {playernum} )
void SF_PlayerName(void)
{
    unsigned int plnum;

    t_return.type = FSVT_string;
    t_return.value.s = "";

    if (!t_argc)
    {
        // player that triggered script
        player_t *pl = fs_current_script->trigger->player;
        if (pl == NULL)  goto err_notplayer;
        plnum = pl - players;
    }
    else
    { 
        plnum = intvalue(t_argv[0]);
    }

    if( plnum < MAXPLAYERS )  // [WDJ] safe limit
        t_return.value.s = player_names[plnum];
done:
    return;

err_notplayer:   
    script_error("PlayerName: script not started by player\n");
    goto done;
}

// PlayerAddFrag( playernum1, {playernum2} )
void SF_PlayerAddFrag(void)
{
    unsigned int playernum1;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.f = 0;  // default

    playernum1 = intvalue(t_argv[0]);
    if ( playernum1 < MAXPLAYERS        // [WDJ] Safe limit
         && playeringame[playernum1] )  // [WDJ] Only if player in game
    {
        if (t_argc == 1)
        {
            // player1 fragged
            players[playernum1].addfrags++;
            t_return.value.f = players[playernum1].addfrags;
        }
        else
        {
            // player1 fragged by player2
            int playernum2 = intvalue(t_argv[1]);

            if ( playernum2 < MAXPLAYERS        // [WDJ] Safe limit
                 && playeringame[playernum2] )  // [WDJ] Only if player in game
            {
                players[playernum1].frags[playernum2]++;
                t_return.value.f = players[playernum1].frags[playernum2];
            }
        }
    }
done:
    return;
   
err_numarg:
    missing_arg_str("PlayerAddFrag", "1 or 2");
    goto done;
}

// player mobj
// PlayerObj( {playernum} )
void SF_PlayerObj(void)
{
    unsigned int plnum;

    t_return.type = FSVT_mobj;
    t_return.value.mobj = NULL;  // default

    if (!t_argc)
    {
        // player that triggered script
        player_t *pl;
        pl = fs_current_script->trigger->player;
        if (pl == NULL)  goto err_notplayer;
        plnum = pl - players;
    }
    else
    {
        plnum = intvalue(t_argv[0]);
    }

    if ( plnum < MAXPLAYERS        // [WDJ] Safe limit
         && playeringame[plnum] )  // [WDJ] Only if player in game
        t_return.value.mobj = players[plnum].mo;
done:
    return;

err_notplayer:   
    script_error("PlayerObj: script not started by player\n");
    goto done;
}

// MobjIsPlayer( {mobj} )
void SF_MobjIsPlayer(void)
{
    t_return.type = FSVT_int;
    if (t_argc == 0)
    {
        // if player triggered script
        t_return.value.i = fs_current_script->trigger->player ? 1 : 0;
    }
    else
    {
        mobj_t * mobj = MobjForSvalue(t_argv[0]);
        t_return.value.i = (mobj)? (mobj->player ? 1 : 0) : 0;
    }
    return;
}

// Test or Set
// SkinColor( player, {colornum} )
void SF_SkinColor(void)
{
    byte playernum;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    playernum = arg_playernum( &t_argv[0], "SkinColor" );
    if( playernum >= MAXPLAYERS )  goto done;

    if(t_argc == 2)
    {
        // set skincolor
        int colour = intvalue(t_argv[1]);

        if(colour > NUMSKINCOLORS-1)  // [WDJ] was NUMSKINCOLORS
        {
            script_error("SkinColor: skin colour %i > %i\n", colour, NUMSKINCOLORS-1);
            goto done;
        }

        P_SetPlayer_color( &players[playernum], colour );

        // Test for netplay and splitscreen usage.
#if 1
        // Would automatically trigger a NetXCmd to other clients, but is that necessary ?
        // Should not affect user settings.
        if( playernum == displayplayer )
        {
            cv_playercolor[0].EV = colour;
//            Send_NameColor_pind( 0 );
        }
        else  if( playernum == displayplayer2 )
        {
            cv_playercolor[1].EV = colour;
//            Send_NameColor_pind( 1 );
        }
#else
        // This will affect user settings, and trigger a NetXCmd send to other clients.
        if( playernum == displayplayer )
            CV_SetValue (&cv_playercolor[0], colour);  // affects user config value
        else  if( playernum == displayplayer2 )
            CV_SetValue (&cv_playercolor[1], colour);  // affects user config value
#endif
    }

    t_return.value.i = players[playernum].skincolor;
done:   
    return;
   
err_numarg:
    missing_arg_str("SkinColor", "1 or 2");
    goto done;
}

// Tests and modifies the usual keys 0..5
// PlayerKeys( player, keynum, {value} )
// [WDJ] keynum: 0..7, the usual 0..5, and additional keys 6,7
// value: 0,1
void SF_PlayerKeys(void)
{
    byte playernum;
    int keynum;
    byte keymask;

    if (t_argc < 2)   goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    playernum = arg_playernum( &t_argv[0], "PlayerKeys" );
    if( playernum >= MAXPLAYERS )  goto done;

    keynum = intvalue(t_argv[1]);
    if (keynum > 7)  goto bad_keyvalue;  // [WDJ] was 5
    keymask = (1 << keynum);

    if (t_argc == 2)
    {
        // test, player has key
        t_return.value.i = (players[playernum].cards & keymask) ? 1 : 0;
    }
    else
    {
        // give or take key
        int givetake = intvalue(t_argv[2]);
        if (givetake)
            players[playernum].cards |= keymask;  // give key
        else
            players[playernum].cards &= ~keymask;  // take key
    }

done:   
    return;
   
err_numarg:
    missing_arg_str("PlayerKeys", "2 or 3");
    goto done;

bad_keyvalue:
    script_error("PlayerKeys: keynum must be 0..5! %i\n", keynum);
    goto done;
}

/*  DarkWolf95:September 17, 2004:playerkeysb

    Returns players[i].cards as a whole, since FS supports binary operators.
    Also allows you to set upper two bits of cards (64 & 128).  Thus the user
    can have two new boolean values to work with.  CTF, Runes, Tag...
 */

// Test or Set
// PlayerKeysByte(player, {newbyte})
void SF_PlayerKeysByte(void)
{
    byte playernum = 0;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    // [WDJ] full player arg like other functions
    playernum = arg_playernum( &t_argv[0], "PlayerKeysByte" );
    if( playernum >= MAXPLAYERS )  goto done;

    if(t_argc == 2)
    {
        // set keys
        unsigned int keybyte = intvalue(t_argv[1]);  
        if(keybyte > 255)  // don't overflow
            keybyte = 0;

        players[playernum].cards = keybyte;  // set
    }
    t_return.value.i = players[playernum].cards;

done:
    return;

err_numarg:
    missing_arg_str("PlayerKeysByte", "1 or 2");
    goto done;
}

// iori 05/17/2005: playerarmor
// [WDJ] Test or Set
// PlayerArmor( player, {armor_value} )
void SF_PlayerArmor(void)
{
    int armor;
    byte playernum;
    player_t * player;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    // [WDJ] full player arg like other functions
    playernum = arg_playernum( &t_argv[0], "PlayerArmor" );
    if( playernum >= MAXPLAYERS )  goto done;
    player = & players[playernum];

    if ( t_argc == 2 )
    {
        // Set armor
        armor = t_argv[1].value.i;
        player->armorpoints = armor;
        player->armortype = (armor>100)? 2 : 1;
    }

    t_return.value.i = player->armorpoints;
done:
    return;

err_numarg:
    missing_arg_str("PlayerArmor", "1 or 2");
    goto done;
}

// Test or Set
// PlayerAmmo( player, ammonum, {ammo_value} )
void SF_PlayerAmmo(void)
{
    int ammonum;
    byte playernum;
    player_t * player;

    if (t_argc < 2)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    playernum = arg_playernum( &t_argv[0], "PlayerAmmo" );
    if( playernum >= MAXPLAYERS )  goto done;
    player = & players[playernum];

    ammonum = intvalue(t_argv[1]);
    if (ammonum >= NUMAMMO || ammonum < 0)  goto bad_ammo;

    if (t_argc == 3)
    {
        // set player ammo
        uint16_t maxammo = player->maxammo[ammonum];
        uint16_t newammo = intvalue(t_argv[2]);
        newammo = (newammo > maxammo) ? maxammo : newammo;
        player->ammo[ammonum] = newammo;
    }
    t_return.value.i = player->ammo[ammonum];  // test ammo
done:   
    return;

err_numarg:
    missing_arg_str("PlayerAmmo", "2 or 3");
    goto done;
   
bad_ammo:
    script_error("PlayerAmmo: invalid ammonum %i\n", ammonum);
    goto done;
}

// Test or Set
// MaxPlayerAmmo( player, ammonum, {ammo_value} )
void SF_MaxPlayerAmmo(void)
{
    byte playernum;
    int ammonum;
    player_t * player;

    if (t_argc < 2)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    playernum = arg_playernum( &t_argv[0], "MaxPlayerAmmo" );
    if( playernum >= MAXPLAYERS )  goto done;
    player = & players[playernum];

    ammonum = intvalue(t_argv[1]);
    if (ammonum >= NUMAMMO || ammonum < 0)  goto bad_ammo;

    if (t_argc == 3)
    {
        // set player max ammo
        int newmax = intvalue(t_argv[2]);
        player->maxammo[ammonum] = newmax;
    }
    t_return.value.i = player->maxammo[ammonum]; // test player max ammo
done:
    return;

err_numarg:
    missing_arg_str("MaxPlayerAmmo", "2 or 3");
    goto done;

bad_ammo:
    script_error("MaxPlayerAmmo: invalid ammonum %i\n", ammonum);
    goto done;
}

// Test or Set
// PlayerWeapon(player, weaponnum, [give])
void SF_PlayerWeapon(void)
{
    int weaponnum;
    byte playernum;
    player_t * player;

    if (t_argc < 2)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    playernum = arg_playernum( &t_argv[0], "PlayerWeapon" );
    if( playernum >= MAXPLAYERS )  goto done;
    player = & players[playernum];

    weaponnum = intvalue(t_argv[1]);
    if (weaponnum >= NUMWEAPONS || weaponnum < 0)  goto bad_weapon;

    if (t_argc == 3)
    {
        // give or take weapon
        int newweapon = intvalue(t_argv[2]);

        if (newweapon != 0)  // boolean
            newweapon = 1;

        player->weaponowned[weaponnum] = newweapon;
    }
    t_return.value.i = player->weaponowned[weaponnum]; // test weapon
done:
    return;

err_numarg:
    missing_arg_str("PlayerWeapon", "2 or 3");
    goto done;

bad_weapon:
    script_error("PlayerWeapon: invalid weaponnum %i\n", weaponnum);
    goto done;
}

// Test or Set
// PlayerSelectedWeapon( player, {selected_weapon} )
void SF_PlayerSelectedWeapon(void)
{
    int weaponnum;
    byte playernum;
    player_t * player;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    playernum = arg_playernum( &t_argv[0], "PlayerSelectedWeapon" );
    if( playernum >= MAXPLAYERS )  goto done;
    player = & players[playernum];

    if(t_argc == 2)
    {
        weaponnum = intvalue(t_argv[1]);
        if (weaponnum >= NUMWEAPONS || weaponnum < 0)  goto bad_weapon;
        player->pendingweapon = weaponnum;
    }
    t_return.value.i = player->readyweapon;  // test weapon
done:
    return;

err_numarg:
    missing_arg_str("PlayerSelectedWeapon", "1 or 2");
    goto done;

bad_weapon:
    script_error("PlayerSelectedWeapon: invalid weaponnum %i\n", weaponnum);
    goto done;
}


// Exl: Toxicfluff's pitchview function.
// Returns a player's view pitch in a range useful for the FS trig functions
// iori: added ability to modify player's pitch
// Test or Set
// PlayerPitch( player, {pitch} )
void SF_PlayerPitch(void)
{
    byte playernum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_fixed;
    t_return.value.f = 0;  // default

    // [WDJ] full player arg like other functions
    playernum = arg_playernum( &t_argv[0], "PlayerPitch" );
    if( playernum >= MAXPLAYERS )  goto done;

    if(t_argc == 2)
    {
        angle_t new_angle = FixedToAngle(fixedvalue(t_argv[1]));
        // Test for netplay and splitscreen usage.
        if( playernum == displayplayer )
            localaiming[0] = new_angle;
        else if( playernum == displayplayer2 )
            localaiming[1] = new_angle;
    }

    t_return.value.f = AngleToFixed(players[playernum].aiming);

done:
    return;

err_numarg:
    missing_arg_str("PlayerPitch", "1 or 2");
    goto done;
}


// Set player properties
// This could (or rather, should) be expanded to support all players
// Set
// PlayerProperty( select, value )
void SF_PlayerProperty(void)
{
    int arg_value;

    if (t_argc != 2)  goto err_numarg;

    arg_value = intvalue(t_argv[1]);

    switch(intvalue(t_argv[0]))
    {
       // Speed
     case 0:
        extramovefactor = arg_value;
        break;

     case 1:
        jumpgravity = arg_value * FRACUNIT / NEWTICRATERATIO;
        break;

     case 2:
        if( arg_value )
            consoleplayer_ptr->GB_flags |= GB_locked;
        else
            consoleplayer_ptr->GB_flags &= ~(GB_locked);
        break;

     default:
        script_error("PlayerProperty, invalid property specified\n");
        break;
    }
done:
   return;

err_numarg:
    wrong_num_arg("PlayerProperty", 2);
    goto done;
}


extern void SF_StartScript(void);   // in t_script.c
extern void SF_ScriptRunning(void);
extern void SF_Wait(void);
extern void SF_WaitTic(void);
extern void SF_TagWait(void);
extern void SF_ScriptWait(void);

/*********** Mobj code ***************/

// Return playernum, or -1
// Player( {mobj} )
void SF_Player(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_int;

    if (mo && mo->player)  // [WDJ] check player
    {
        t_return.value.i = (unsigned int) (mo->player - players);
    }
    else
    {
        t_return.value.i = -1;  // programs may depende on it being negative
    }
}

// spawn an object: type, x, y, [angle]
// Spawn( type, x, y, {angle}, {z} )
void SF_Spawn(void)
{
    mapthing_t * mthing;
    int x, y, z, objtype;
    angle_t angle = 0;

    if (t_argc < 3)  goto err_numarg;

    t_return.type = FSVT_mobj;
    t_return.value.mobj = NULL;  // default

    objtype = intvalue(t_argv[0]);
    x = intvalue(t_argv[1]) << FRACBITS;
    y = intvalue(t_argv[2]) << FRACBITS;
    if (t_argc >= 5)
        z = intvalue(t_argv[4]) << FRACBITS;
    else
    {
        // SoM: Check thing flags for spawn-on-ceiling types...
        z = R_PointInSubsector(x, y)->sector->floorheight;
    }

    if (t_argc >= 4)
        angle = intvalue(t_argv[3]) * (ANG45 / 45);

    // check invalid object to spawn
    if (objtype < 0 || objtype >= NUMMOBJTYPES)  goto err_objtype;

    t_return.value.mobj = P_SpawnMobj(x, y, z, objtype);
    t_return.value.mobj->angle = angle;

    // [WDJ] Only MF_SPECIAL things can respawn, and MF_COUNTKILL can nightmare
    // respawn, so only they need a spawnpoint.
    if (t_return.value.mobj->flags & (MF_SPECIAL|MF_COUNTKILL))
    {
        // create a unique mapthing for this spawn
        mthing = P_Get_Extra_Mapthing( MTF_FS_SPAWNED );  // [WDJ]
        t_return.value.mobj->spawnpoint = mthing;
        if (mthing)
        {
            //Hurdler: fix the crashing bug of respawning monster
            // 2002/9/7
            mthing->x = x >> FRACBITS;
            mthing->y = y >> FRACBITS;
            mthing->z = z >> FRACBITS;
            mthing->angle = angle >> FRACBITS;
            mthing->type = mobjinfo[objtype].doomednum;  // objtype;
            mthing->mobj = t_return.value.mobj;
        }
    }
done:
    return;

err_numarg:
    missing_arg_str("Spawn", "3, 4 or 5");
    goto done;

err_objtype:
    script_error("Spawn: unknown object type: %i\n", objtype);
    goto done;
}

// [WDJ] Docs say: SpawnExplosion ( damage, spot, {source} )
// but implemented as SpawnExplosion ( type, x, y, {z} )
void SF_SpawnExplosion(void)
{
    int type;
    fixed_t x, y, z;
    mobj_t *spawn;

    if (t_argc < 3)  goto err_numarg;

    type = intvalue(t_argv[0]);
    if (type < 0 || type >= NUMMOBJTYPES)  goto err_spawntype;

    x = fixedvalue(t_argv[1]);
    y = fixedvalue(t_argv[2]);
    if (t_argc > 3)
        z = fixedvalue(t_argv[3]);
    else
        z = R_PointInSubsector(x, y)->sector->floorheight;

    spawn = P_SpawnMobj(x, y, z, type);
    t_return.type = FSVT_int;
    t_return.value.i = P_SetMobjState(spawn, spawn->info->deathstate);
    if (spawn->info->deathsound)
        S_StartObjSound(spawn, spawn->info->deathsound);
done:
    return;

err_numarg:
    missing_arg_str("SpawnExplosion", "3 or 4");
    goto done;

err_spawntype:
    script_error("SpawnExplosion: Invalid type number %i\n", type);
    goto done;
}

// RadiusAttack( location_mobj, source_mobj, damage )
void SF_RadiusAttack(void)
{
    mobj_t *spot;
    mobj_t *source;
    int damage;

    if (t_argc != 3)  goto err_numarg;

    spot = MobjForSvalue(t_argv[0]);    // where
    source = MobjForSvalue(t_argv[1]);  // who gets blame
    damage = intvalue(t_argv[2]);

    if (spot && source)
    {
        P_RadiusAttack(spot, source, damage);
    }
done:
    return;

err_numarg:
    wrong_num_arg("RadiusAttack", 3);
    goto done;
}

// RemoveObj( mobj )
void SF_RemoveObj(void)
{
    mobj_t *mo;

    if (t_argc != 1)  goto err_numarg;

    mo = MobjForSvalue(t_argv[0]);
    if (mo)     // nullptr check
        P_RemoveMobj(mo);
done:
    return;

err_numarg:
    wrong_num_arg("RemoveObj", 1);
    goto done;
}

// KillObj( {mobj} )
void SF_KillObj(void)
{
    mobj_t *mo;

    if (t_argc)
        mo = MobjForSvalue(t_argv[0]);
    else
        mo = fs_current_script->trigger;   // default to trigger object

    if (mo)     // nullptr check
        P_KillMobj(mo, NULL, fs_current_script->trigger);  // kill it
}

// mobj x, y, z
// ObjX( {mobj} )
void SF_ObjX(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_fixed;
    t_return.value.f = mo ? mo->x : 0;  // null ptr check
}

// ObjY( {mobj} )
void SF_ObjY(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_fixed;
    t_return.value.f = mo ? mo->y : 0;  // null ptr check
}

// ObjZ( {mobj} )
void SF_ObjZ(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_fixed;
    t_return.value.f = mo ? mo->z : 0;  // null ptr check
}

// SetObjPosition( mobj, x, {y}, {z} )
void SF_SetObjPosition(void)
{
    mobj_t* mobj;

    if (t_argc < 2)  goto err_numarg;  // [WDJ] requires 2 arg

    mobj = MobjForSvalue(t_argv[0]);
    if( mobj )  // [WDJ]
    { 
        P_UnsetThingPosition(mobj);

        mobj->x = intvalue(t_argv[1]) << FRACBITS;

        if(t_argc >= 3)
            mobj->y = intvalue(t_argv[2]) << FRACBITS;
        if(t_argc == 4)
            mobj->z = intvalue(t_argv[3]) << FRACBITS;

        P_SetThingPosition(mobj);
    }
done:
    return;

err_numarg:
    missing_arg_str("SetObjPosition", "2, 3 or 4");
    goto done;
}


// Resurrect( mobj )
void SF_Resurrect(void)
{
    mobj_t *mo;

    if(t_argc != 1)  goto err_numarg;

    mo = MobjForSvalue(t_argv[0]);
    if( mo )  // [WDJ]
    {
        if(!mo->info->raisestate)  //Don't resurrect things that can't be resurrected
            goto done;

        P_SetMobjState (mo, mo->info->raisestate);
        if( demoversion<129 )
            mo->height <<= 2;
        else
        {
            mo->height = mo->info->height;
            mo->radius = mo->info->radius;
        }

        mo->flags = mo->info->flags;
        mo->health = mo->info->spawnhealth;
        mo->target = NULL;
    }
done:
    return;

err_numarg:
    wrong_num_arg("Resurrect", 1);
    goto done;
}

// TestLocation( {mobj} )
void SF_TestLocation(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_int;
    t_return.value.f = 0;  // default

    if (mo && P_TestMobjLocation(mo))
    {
        t_return.value.f = 1;
    }
}

// mobj angle
// ObjAngle( {mobj}, {angle} )
void SF_ObjAngle(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_fixed;
    t_return.value.f = 0;  // default

    if( mo )  // [WDJ]
    {
        if(t_argc > 1)
        {
            // set angle
            angle_t new_angle = FixedToAngle(fixedvalue(t_argv[1]));
            //iori: now able to change the player's angle, not just mobj's
            // Test for netplay and splitscreen usage.
            if(mo == consoleplayer_ptr->mo)
            {
                localangle[0] = new_angle;
            }
            else if(displayplayer2_ptr && (mo == displayplayer2_ptr->mo))
            {
                localangle[1] = new_angle;
            }
            else
            {
                mo->angle = new_angle;
            }
        }
        t_return.value.f = (int)AngleToFixed(mo->angle);
    }
}

// CheckSight( obj1, {obj2} )
void SF_CheckSight(void)
{
    mobj_t *obj1;
    mobj_t *obj2;

    if(!t_argc)  goto err_numarg;

    obj1 = MobjForSvalue(t_argv[0]);
    obj2 = (t_argc == 2) ? MobjForSvalue(t_argv[1]) : fs_current_script->trigger;

    t_return.type = FSVT_int;
    t_return.value.i = P_CheckSight(obj1, obj2);
done:
    return;

err_numarg:
    missing_arg_str("CheckSight", "1 or 2");
    goto done;
}


// Teleport( {mobj}, sector_tag )
void SF_Teleport(void)
{
    line_t sf_tmpline;  // temp teleport linedef
    mobj_t *mo;

    if (t_argc == 0)  goto err_numarg;

    if (t_argc == 1)       // 1 argument: sector tag
    {
        // teleport trigger mobj
        mo = fs_current_script->trigger;   // default to trigger
        sf_tmpline.tag = intvalue(t_argv[0]);
    }
    else
    {
        // teleport the arg mobj
        mo = MobjForSvalue(t_argv[0]);
        sf_tmpline.tag = intvalue(t_argv[1]);
    }
    sf_tmpline.dx = sf_tmpline.dy = 1;  // [WDJ] used by EV_Teleport

    if (mo)
        EV_Teleport(&sf_tmpline, 0, mo);
done:
    return;

err_numarg:
    missing_arg_str("Teleport", "1 or 2");
    goto done;
}

// SilentTeleport( {mobj}, sector_tag )
void SF_SilentTeleport(void)
{
    line_t sf_tmpline;                // dummy line for teleport function
    mobj_t *mo;

    if (t_argc == 0)   goto err_numarg;

    if (t_argc == 1)       // 1 argument: sector tag
    {
        // teleport trigger mobj
        mo = fs_current_script->trigger;   // default to trigger
        sf_tmpline.tag = intvalue(t_argv[0]);
    }
    else
    {
        // teleport the arg mobj
        mo = MobjForSvalue(t_argv[0]);
        sf_tmpline.tag = intvalue(t_argv[1]);
    }
    sf_tmpline.dx = sf_tmpline.dy = 1;  // [WDJ] used by EV_SilentTeleport

    if (mo)
        EV_SilentTeleport(&sf_tmpline, 0, mo);
done:
    return;

err_numarg:
    missing_arg_str("SilentTeleport", "1 or 2");
    goto done;
}

// DamageObj( {mobj}, damage );
void SF_DamageObj(void)
{
    mobj_t *mo;
    int damageamount;

    if (t_argc == 0)  goto err_numarg;

    if (t_argc == 1)       // 1 argument: damage trigger by amount
    {
        // damage the trigger mobj
        mo = fs_current_script->trigger;   // default to trigger
        damageamount = intvalue(t_argv[0]);
    }
    else
    {
        // damage the arg mobj
        mo = MobjForSvalue(t_argv[0]);
        damageamount = intvalue(t_argv[1]);
    }

    if (mo)
        P_DamageMobj(mo, NULL, fs_current_script->trigger, damageamount);
done:
    return;

err_numarg:
    missing_arg_str("DamageObj", "1 or 2");
    goto done;
}


// the tag number of the sector the thing is in
// ObjSector( {mobj} )
void SF_ObjSector(void)
{
    // use trigger object if not specified
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_int;
    // [WDJ] dsv4 map28 has buttons that hurt player, causes segfault here
    // when pressed after getting yellow key, mo with no subsector.
    t_return.value.i = (mo && mo->subsector) ? mo->subsector->sector->tag : 0;     // nullptr check
}

// the health number of an object
// ObjHealth( {mobj} )
void SF_ObjHealth(void)
{
    // use trigger object if not specified
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_int;
    t_return.value.i = mo ? mo->health : 0;
}

// ObjDead( {mobj} )
void SF_ObjDead(void)
{
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_int;
    if (mo && (mo->health <= 0 || mo->flags & MF_CORPSE))
        t_return.value.i = 1;
    else
        t_return.value.i = 0;
}

// Test or Set
// ObjFlag( {mobj}, flagnum, {flagvalue} )
// flagvalue= 0,1
void SF_ObjFlag(void)
{
    mobj_t *mo;
    int flagnum;

    if (t_argc == 0)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    if (t_argc == 1)       // use trigger, arg0 is flagnum
    {
        // test flags on trigger:
        mo = fs_current_script->trigger;
        flagnum = intvalue(t_argv[0]);
    }
    else
    {
        // test flags on arg mobj
        mo = MobjForSvalue(t_argv[0]);
        flagnum = intvalue(t_argv[1]);
    }
    
    if (mo && flagnum >= 0 && flagnum < 32)  // [WDJ]
    {
        uint32_t flagmsk = (1 << flagnum);
        if (t_argc == 3)
        {
            // set flags on arg mobj
            if( intvalue(t_argv[2]) )   // 0 or 1
                mo->flags |= flagmsk;  // set the flag
            else
                mo->flags &= ~flagmsk;  // clear the flag
            //P_UpdateThinker(&mo->thinker);     // update thinker
        }

        t_return.value.i = !!(mo->flags & flagmsk);  // test flag, to boolean
    }
done:
    return;

err_numarg:
    missing_arg_str("ObjFlag", "1, 2 or 3");
    goto done;
}


// Just copy n paste :>
// Test or Set
// ObjFlag2( {mobj}, flagnum, {flagvalue} )
// flagvalue= 0,1
void SF_ObjFlag2(void)
{
    mobj_t *mo;
    int flagnum;

    if (t_argc == 0)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    if (t_argc == 1)       // use trigger, arg0 is flagnum
    {
        // test flags on trigger:
        mo = fs_current_script->trigger;
        flagnum = intvalue(t_argv[0]);
    }
    else
    {
        // test flags on arg mobj
        mo = MobjForSvalue(t_argv[0]);
        flagnum = intvalue(t_argv[1]);
    }
    
    if (mo && flagnum >= 0 && flagnum < 32)  // [WDJ]
    {
        uint32_t flagmsk = (1 << flagnum);
        if (t_argc == 3)
        {
            // set flags on arg mobj
            if( intvalue(t_argv[2]) )   // 0 or 1
                mo->flags2 |= flagmsk;  // set the flag
            else
                mo->flags2 &= ~flagmsk;  // clear the flag
            //P_UpdateThinker(&mo->thinker);     // update thinker
        }

        t_return.value.i = !!(mo->flags2 & flagmsk);  // test flag, to boolean
    }
done:
    return;

err_numarg:
    missing_arg_str("ObjFlag2", "1, 2 or 3");
    goto done;
}


// Extra flags, too
void SF_ObjEFlag(void)
{
    mobj_t *mo;
    int flagnum;

    if (t_argc == 0)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    if (t_argc == 1)       // use trigger, arg0 is flagnum
    {
        // test flags on trigger:
        mo = fs_current_script->trigger;
        flagnum = intvalue(t_argv[0]);
    }
    else
    {
        // test flags on arg mobj
        mo = MobjForSvalue(t_argv[0]);
        flagnum = intvalue(t_argv[1]);
    }
    
    if (mo && flagnum >= 0 && flagnum < 32)  // [WDJ]
    {
        uint32_t flagmsk = (1 << flagnum);
        if (t_argc == 3)
        {
            // set flags on arg mobj
            if( intvalue(t_argv[2]) )   // 0 or 1
                mo->eflags |= flagmsk;  // set the flag
            else
                mo->eflags &= ~flagmsk;  // clear the flag
            //P_UpdateThinker(&mo->thinker);     // update thinker
        }

        t_return.value.i = !!(mo->eflags & flagmsk);  // test flag, to boolean
    }
done:
    return;

err_numarg:
    missing_arg_str("ObjEFlag", "1, 2 or 3");
    goto done;
}


// apply momentum to a thing
// PushThing( mobj, angle, force )
void SF_PushThing(void)
{
    mobj_t *mo;
    angle_t angle;
    fixed_t force;

    if (t_argc != 3)  goto err_numarg;

    mo = MobjForSvalue(t_argv[0]);
    if (!mo)  goto done;

    angle = FixedToAngle(fixedvalue(t_argv[1]));
    force = fixedvalue(t_argv[2]);

    mo->momx += FixedMul( cosine_ANG(angle), force);
    mo->momy += FixedMul( sine_ANG(angle), force);
done:
    return;

err_numarg:
    wrong_num_arg("PushThing", 3);
    goto done;
}

// Test or Set
// ReactionTime( mobj, {reactiontime} )
void SF_ReactionTime(void)
{
    mobj_t *mo;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    mo = MobjForSvalue(t_argv[0]);
    if (!mo)  goto done;

    if (t_argc > 1)
    {   // set
        mo->reactiontime = (intvalue(t_argv[1]) * 35) / 100;
    }

    t_return.value.i = mo->reactiontime;  // test
done:
    return;

err_numarg:
    missing_arg_str("ReactionTime", "1 or 2");
    goto done;
}

// Sets a mobj's Target! >:)
// Test or Set
// MobjTarget( mobj, {target_mobj} )
void SF_MobjTarget(void)
{
    mobj_t *mo;
    mobj_t *target;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_mobj;
    t_return.value.mobj = NULL;  // default

    mo = MobjForSvalue(t_argv[0]);
    if (!mo)  goto done;

    if (t_argc >= 2)
    {   // set
        if (t_argv[1].type != FSVT_mobj && intvalue(t_argv[1]) == -1)
        {
            // Set target to NULL
            mo->target = NULL;
            P_SetMobjState(mo, mo->info->spawnstate);
        }
        else
        {
            target = MobjForSvalue(t_argv[1]);

            // Also remember node here
            if (target->type == MT_NODE)
                mo->targetnode = target;

            mo->target = target;
            P_SetMobjState(mo, mo->info->seestate);
        }
    }

    t_return.value.mobj = mo->target;  // test
done:
    return;

err_numarg:
    missing_arg_str("MobjTarget", "1 or 2");
    goto done;
}

// MobjMomx( mobj )
void SF_MobjMomx(void)
{
    mobj_t *mo;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_fixed;
    t_return.value.f = 0;  // default

    mo = MobjForSvalue(t_argv[0]);
    if( mo )
    {
        if (t_argc > 1)
            mo->momx = fixedvalue(t_argv[1]);  // set
        t_return.value.f = mo->momx;  // test
    }
done:
    return;

err_numarg:
    missing_arg_str("MobjMomx", "1 or 2");
    goto done;
}

// MobjMomy( mobj )
void SF_MobjMomy(void)
{
    mobj_t *mo;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_fixed;
    t_return.value.f = 0;  // default

    mo = MobjForSvalue(t_argv[0]);
    if( mo )
    {
        if (t_argc > 1)
            mo->momy = fixedvalue(t_argv[1]);  // set
        t_return.value.f = mo->momy;  // test
    }
done:
    return;

err_numarg:
    missing_arg_str("MobjMomy", "1 or 2");
    goto done;
}

// MobjMomz( mobj )
void SF_MobjMomz(void)
{
    mobj_t *mo;

    if (t_argc < 1)  goto err_numarg;

    t_return.type = FSVT_fixed;
    t_return.value.f = 0;  // default

    mo = MobjForSvalue(t_argv[0]);
    if( mo )
    {
        if (t_argc > 1)
            mo->momz = fixedvalue(t_argv[1]);  // set
        t_return.value.f = mo->momz;  // test
    }
done:
    return;

err_numarg:
    missing_arg_str("MobjMomz", "1 or 2");
    goto done;
}

// SpawnMissile( mobj, target_mobj, missiletype )
void SF_SpawnMissile(void)
{
    mobj_t *mobj;
    mobj_t *target;
    int objtype;

    if (t_argc != 3)  goto err_numarg;

    t_return.type = FSVT_mobj;
    t_return.value.mobj = NULL;  // default

    objtype = intvalue(t_argv[2]);
    if (objtype < 0 || objtype >= NUMMOBJTYPES)  goto err_objtype;

    mobj = MobjForSvalue(t_argv[0]);
    target = MobjForSvalue(t_argv[1]);

    t_return.value.mobj = P_SpawnMissile(mobj, target, objtype);
done:
    return;

err_numarg:
    wrong_num_arg("SpawnMissile", 3);
    goto done;

err_objtype:
    script_error("SpawnMissile: unknown object type: %i\n", objtype);
    goto done;
}


// Exl: Modified by Tox to take a pitch parameter
// LineAttack( mobj, angle, damage, {pitch})
void SF_LineAttack(void)
{
    mobj_t * mo;
    angle_t aiming;
    int  damage, angle, slope;
    const int short fixedtodeg = 182; // 182.033

    if (t_argc < 3)  goto err_numarg; // [WDJ]

    mo = MobjForSvalue(t_argv[0]);
    if( !mo )  goto done;  // [WDJ]

    damage = intvalue(t_argv[2]);
    angle = (intvalue(t_argv[1]) * (ANG45 / 45));

    if(t_argc == 4)
    {
        aiming = fixedvalue(t_argv[3]) * fixedtodeg;
        slope = AIMINGTOSLOPE(aiming);
    }
    else
    {
        slope = P_AimLineAttack(mo, angle, MISSILERANGE, 0);  // auto aim
    }

    P_LineAttack(mo, angle, MISSILERANGE, slope, damage);
done:
    return;

err_numarg:
    missing_arg_str("LineAttack", "3 or 4");
    goto done;
}


//checks to see if a Map Thing Number exists; used to avoid script errors
// MapthingNumExist( thingnum )
void SF_MapthingNumExist(void)
{
    int intval;

    if (t_argc != 1)  goto err_numarg;

    t_return.type = FSVT_int;

    intval = intvalue(t_argv[0]);
    if (intval < 0 || intval >= nummapthings || !mapthings[intval].mobj)
    {
        t_return.value.i = 0;
    }
    else
    {
        t_return.value.i = 1;
    }
done:
    return;

err_numarg:
    wrong_num_arg("MapthingNumExist", 1);
    goto done;
}

// Mapthings()
void SF_Mapthings(void)
{
    t_return.type = FSVT_int;
    t_return.value.i = nummapthings;
}

// ObjType( {mobj} )
void SF_ObjType(void)
{
    // use trigger object if not specified
    mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : fs_current_script->trigger;

    t_return.type = FSVT_int;
    t_return.value.i = (mo)? mo->type : 0;  // [WDJ] check mo exist
}


// Exl: sets an object's properties (tox)
// SetObjProperty( attribute, property_value, {mobj} )
void SF_SetObjProperty(void)
{
    mobj_t * mo;
    int attrib;
    int32_t  value;  // int and fixed_t
  
    if (t_argc < 2)  goto err_numarg;

    attrib = intvalue(t_argv[0]); 
    value  = intvalue(t_argv[1]);
  // FIXME fixed_t vals, syntax does not make sense (operate on MTs instead of mobjs...
    
    mo = (t_argc >= 3) ? MobjForSvalue(t_argv[2]) : fs_current_script->trigger;
    if( ! mo)  goto done;  // [WDJ]

    switch (attrib)
    {
    case 0:
      mo->info->radius = (value*FRACUNIT);
#ifdef MONSTER_VARY
        // FIXME: should update every mobj of mo type, but there is likely only one
      mobj_varied( mo );
#endif
      break;
    case 1:
      mo->info->height = (value*FRACUNIT);
#ifdef MONSTER_VARY
        // FIXME: should update every mobj of mo type, but there is likely only one
      mobj_varied( mo );
#endif
      break;
    case 2:
      mo->info->mass = value;
      break;
    case 3:
      mo->info->spawnhealth = value;
      break;
    case 4:
      mo->info->damage = value;
      break;
    case 5:
      mo->info->speed = value;
#ifdef MONSTER_VARY
        // FIXME: must update every mobj of mo type
      mobj_varied( mo );
#endif
      break;
    case 6:
      mo->info->reactiontime = value;
      break;
    case 7:
      mo->info->painchance = value;
      break;

    case 8:
#ifdef ENABLE_DEH_REMAP
      mo->info->spawnstate = remap_state( value );
#else
      mo->info->spawnstate = value;
#endif
      break;
    case 9:
#ifdef ENABLE_DEH_REMAP
      mo->info->seestate = remap_state( value );
#else
      mo->info->seestate = value;
#endif
      break;
    case 10:
#ifdef ENABLE_DEH_REMAP
      mo->info->meleestate = remap_state( value );
#else
      mo->info->meleestate = value;
#endif
      break;
    case 11:
#ifdef ENABLE_DEH_REMAP
      mo->info->missilestate = remap_state( value );
#else
      mo->info->missilestate = value;
#endif
      break;
    case 12:
#ifdef ENABLE_DEH_REMAP
      mo->info->painstate = remap_state( value );
#else
      mo->info->painstate = value;
#endif
      break;
    case 13:
#ifdef ENABLE_DEH_REMAP
      mo->info->deathstate = remap_state( value );
#else
      mo->info->deathstate = value;
#endif
      break;
    case 14:
#ifdef ENABLE_DEH_REMAP
      mo->info->xdeathstate = remap_state( value );
#else
      mo->info->xdeathstate = value;
#endif
      break;
    case 15:
#ifdef ENABLE_DEH_REMAP
      mo->info->crashstate = remap_state( value );
#else
      mo->info->crashstate = value;
#endif
      break;
    case 16:
#ifdef ENABLE_DEH_REMAP
      mo->info->raisestate = remap_state( value );
#else
      mo->info->raisestate = value;
#endif
      break;

    case 17:
#ifdef ENABLE_DEH_REMAP
      mo->info->seesound = remap_sound( value );
#else
      mo->info->seesound = value;
#endif	
      break;
    case 18:
#ifdef ENABLE_DEH_REMAP
      mo->info->seesound = remap_sound( value );
#else
      mo->info->activesound = value;
#endif
      break;
    case 19:
#ifdef ENABLE_DEH_REMAP
      mo->info->attacksound = remap_sound( value );
#else
      mo->info->attacksound = value;
#endif
      break;
    case 20:
#ifdef ENABLE_DEH_REMAP
      mo->info->painsound = remap_sound( value );
#else
      mo->info->painsound = value;
#endif
      break;
    case 21:
#ifdef ENABLE_DEH_REMAP
      mo->info->deathsound = remap_sound( value );
#else
      mo->info->deathsound = value;
#endif
      break;
    default:
      script_error("SetObjProperty: invalid attribute %i\n", attrib);
      break;
    }
done:
    return;

err_numarg:
    missing_arg_str("SetObjProperty", "2 or 3");
    goto done;
}


// Exl: Returns an object's properties (tox)
// GetObjProperty( {mobj}, attrib )
void SF_GetObjProperty(void)
{
    int attrib = 0, retval = 0;	// might be used uninit
    mobj_t *mo = NULL;

    if(t_argc == 1)
    {
        mo = fs_current_script->trigger;
        attrib = intvalue(t_argv[0]);
    }
    else if(t_argc == 2)
    {
        mo = MobjForSvalue(t_argv[0]);
        attrib = intvalue(t_argv[1]);
    }
    else goto err_numarg;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    if( !mo )  goto done;  // [WDJ]

    switch (attrib)
    {
     case 0:
        // Has had mobj radius, but access info->radius instead.
        retval = mo->info->radius / FRACUNIT;
        break;
     case 1:
        // Has had mobj height, but access info->height instead.
        retval = mo->info->height / FRACUNIT;
        break;
     case 2:
        retval = mo->info->mass;		  
        break;
     case 3:
        retval = mo->info->spawnhealth;
        break;
     case 4:
        retval = mo->info->damage;
        break;
     case 5:
#ifdef MONSTER_VARY
        retval = mo->speed;  // individual speed
#else
        retval = mo->info->speed;
#endif
        break;
     case 6:
        retval = mo->info->reactiontime;
        break;
     case 7:
        retval = mo->info->painchance;
        break;
     case 8:
        retval = mo->info->spawnstate;
        break;
     case 9:
        retval = mo->info->seestate;
        break;
     case 10:
        retval = mo->info->meleestate;
        break;
     case 11:
        retval = mo->info->missilestate;
        break;
     case 12:
        retval = mo->info->painstate;
        break;
     case 13:
        retval = mo->info->deathstate;
        break;
     case 14:
        retval = mo->info->xdeathstate;
        break;
     case 15:
        retval = mo->info->crashstate;
        break;
     case 16:
        retval = mo->info->raisestate;
        break;
     case 17:
        retval = mo->info->seesound;
        break;
     case 18:
        retval = mo->info->activesound;
        break;
     case 19:
        retval = mo->info->attacksound;
        break;
     case 20:
        retval = mo->info->painsound;
        break;
     case 21:
        retval = mo->info->deathsound;
        break;
     default:
        script_error("GetObjProperty: invalid attribute %i\n", attrib);
        return;
    }
    t_return.value.i = retval;
done:
    return;

err_numarg:
    missing_arg_str("GetObjProperty", "1 or 2");
    goto done;
}


//DarkWolf95:November 15, 2003: Adaptation of Exl's code
//DarkWolf95:December 7, 2003: Change to set only
// Set
// ObjState( {mobj}, state )
void SF_ObjState(void)
{
    int  select;
    uint16_t  newstate;
    mobj_t * mo;

    t_return.type = FSVT_int;
    t_return.value.i = 0;  // default

    if(t_argc == 1)
    {
        mo = fs_current_script->trigger;
        select = intvalue(t_argv[0]);  // coded enum selector
    }
    else if(t_argc == 2)
    {
        mo = MobjForSvalue(t_argv[0]);
        select = intvalue(t_argv[1]);  // coded enum selector
    }
    else goto err_numarg;
    if( ! mo )  goto done; // [WDJ]

    switch (select)
    {
     case 8:
        newstate = mo->info->spawnstate;
        break;
     case 9:
        newstate = mo->info->seestate;
        break;
     case 10:
        newstate = mo->info->meleestate;
        break;
     case 11:
        newstate = mo->info->missilestate;
        break;
     case 12:
        newstate = mo->info->painstate;
        break;
     case 13:
        newstate = mo->info->deathstate;
        break;
     case 14:
        newstate = mo->info->xdeathstate;
        break;
     case 15:
        newstate = mo->info->crashstate;
        break;
     case 16:
        newstate = mo->info->raisestate;
        break;
     default:
        script_error("ObjState: invalid state\n");
        return;
    }

    t_return.value.i = P_SetMobjState(mo, newstate);
done:
    return;

err_numarg:
    missing_arg_str("ObjState", "1 or 2");
    goto done;
}


// HealObj( {mobj}, {health} )
void SF_HealObj(void)
{
    mobj_t *mo;
    int heal = 0;

    switch(t_argc)
    {
     case 0:
        // Heal trigger to default health
        mo = fs_current_script->trigger;
        heal = mo->info->spawnhealth;
        break;
     case 1:
        // Heal arg mobj to default health
        mo = MobjForSvalue(t_argv[0]);
        heal = mo->info->spawnhealth;
        break;
     case 2:
        // Heal arg mobj to given health
        mo = MobjForSvalue(t_argv[0]);
        heal = intvalue(t_argv[1]);
        break;
     default:
        goto err_numarg;
    }

    if( mo ) // [WDJ]
        mo->health = heal;
done:
    return;

err_numarg:
    missing_arg_str("HealObj", "0, 1 or 2");
    goto done;
}


// Set next node mobj
// SetNodeNext( mobj, next_mobj )
void SF_SetNodeNext(void)
{
    mobj_t* mo;  // Affected
    mobj_t* nextmo;

    if (t_argc != 2)  goto err_numarg;

    mo = MobjForSvalue(t_argv[0]);
    if( !mo )  goto done;  // [WDJ]

    nextmo = MobjForSvalue(t_argv[1]);
    if( !nextmo )  goto done;  // [WDJ]

    // Check if both mobjs are NODE
    if (mo->type != MT_NODE || nextmo->type != MT_NODE)  goto err_notnode;
    mo->nextnode = nextmo;
done:
    return;

err_numarg:
    wrong_num_arg("SetNodeNext", 2);
    goto done;

err_notnode:
    script_error("SetNodeNext: mobj is not a node\n");
    goto done;
}


// Set the time to wait at a node
// SetNodePause( mobj, waittime )
void SF_SetNodePause(void)
{
    mobj_t* mo;

    if (t_argc != 2)  goto err_numarg;

    mo = MobjForSvalue(t_argv[0]);
    if( !mo )  goto done;  // [WDJ]
    if (mo->type != MT_NODE)  goto err_notnode;
    mo->nodewait = t_argv[1].value.i;
done:
    return;

err_numarg:
    wrong_num_arg("SetNodePause", 2);
    goto done;

err_notnode:
    script_error("SetNodePause: mobj is not a node\n");
    goto done;
}


// Run a script when touching a node
// SetNodeScript( mobj, scriptnum )
void SF_SetNodeScript(void)
{
    mobj_t*  mo;
    int      sn;

    if (t_argc != 2)  goto err_numarg;

    mo = MobjForSvalue(t_argv[0]);
    if( !mo )  goto done;  // [WDJ]
    if (mo->type != MT_NODE)  goto err_notnode;

    sn = intvalue(t_argv[1]);

    // Check if the script is defined
    if(!fs_levelscript.children[sn])  goto err_notscript;
    mo->nodescript = sn + 1;  // +1 because 0 = none
done:
    return;

err_numarg:
    wrong_num_arg("SetNodeScript", 2);
    goto done;

err_notnode:
    script_error("SetNodeScript: mobj is not a node\n");
    goto done;

err_notscript:
    script_error("SetNodeScript: script not defined\n");
    goto done;
}



/****************** Trig *********************/

// PointToAngle( x1, y1, x2, y2 )
void SF_PointToAngle(void)
{
    angle_t angle;
    int x1, y1, x2, y2;

    if (t_argc != 4)   goto err_numarg;

    x1 = intvalue(t_argv[0]) << FRACBITS;
    y1 = intvalue(t_argv[1]) << FRACBITS;
    x2 = intvalue(t_argv[2]) << FRACBITS;
    y2 = intvalue(t_argv[3]) << FRACBITS;

    angle = R_PointToAngle2(x1, y1, x2, y2);

    t_return.type = FSVT_fixed;
    t_return.value.f = AngleToFixed(angle);
done:
    return;

err_numarg:
    wrong_num_arg("PointToAngle", 4);
    goto done;
}

// PointToDist( x1, y1, x2, y2 )
void SF_PointToDist(void)
{
    int dist;
    int x1, x2, y1, y2;

    if (t_argc != 4)   goto err_numarg;

    x1 = intvalue(t_argv[0]) << FRACBITS;
    y1 = intvalue(t_argv[1]) << FRACBITS;
    x2 = intvalue(t_argv[2]) << FRACBITS;
    y2 = intvalue(t_argv[3]) << FRACBITS;

    dist = R_PointToDist2(x1, y1, x2, y2);
    t_return.type = FSVT_fixed;
    t_return.value.f = dist;
done:
    return;

err_numarg:
    wrong_num_arg("PointToDist", 4);
    goto done;
}

/************* Camera functions ***************/

camera_t script_camera = { NULL, NULL, 0, 0, 0, 0 };
boolean script_camera_on;

// SetCamera(obj, [angle], [viewheight], [pitch])
// pitch= -50..50
void SF_SetCamera(void)
{
    mobj_t * mo;

    if (t_argc < 1)  goto err_numarg;

#if 0
    // [WDJ] Docs: returns void
    t_return.type = FSVT_fixed;
    t_return.value.f = 0; // default
#endif

    mo = MobjForSvalue(t_argv[0]);
    if (!mo)  goto done;

    if (script_camera.mo != mo)
    {
        if (script_camera.mo)
            script_camera.mo->angle = script_camera.startangle;

        script_camera.startangle = mo->angle;
    }

    script_camera.mo = mo;
    script_camera.mo->angle = (t_argc < 2) ? mo->angle : FixedToAngle(fixedvalue(t_argv[1]));
    script_camera.mo->z = (t_argc < 3) ?
     (((mo->subsector)? mo->subsector->sector->floorheight : 0) + (41 << FRACBITS))
     : fixedvalue(t_argv[2]);

    angle_t aiming = (t_argc < 4) ? 0 : FixedToAngle(fixedvalue(t_argv[3]));
    script_camera.aiming = G_ClipAimingPitch(aiming);

    script_camera_on = true;
   
#if 0
    // [WDJ] Docs: returns void
    t_return.value.f = script_camera.aiming; // FIXME really?
#endif
done:
    return;

err_numarg:
    missing_arg_str("SetCamera", "1, 2, 3 or 4");
}

// ClearCamera( )
void SF_ClearCamera(void)
{
    script_camera_on = false;
    if ( ! script_camera.mo )  goto done;

    script_camera.mo->angle = script_camera.startangle;
    script_camera.mo = NULL;
done:
    return;

#if 0
   // [WDJ] Docs: no error  (and it is unnecessary)
err_camera:
    script_error("Clearcamera: called without setcamera.\n");
    goto done;
#endif
}

// MoveCamera(cameraobj, targetobj, targetheight, movespeed, targetangle, anglespeed)
void SF_MoveCamera(void)
{
    fixed_t x, y, z;
    fixed_t xdist, ydist, zdist, xydist, movespeed;
    fixed_t xstep, ystep, zstep, targetheight;
    angle_t anglespeed, anglestep = 0, angledist, targetangle, mobjangle, bigangle, smallangle;
    // I have to use floats for the math where angles are divided by fixed
    // values.
    double fangledist, fanglestep, fmovestep;
    int angledir = 0;
    mobj_t *camera;
    mobj_t *target;
    int moved = 0; // camera moved
    int quad1, quad2;

#if 1
    // Phobiata.wad has at least one movecamera with 7 parameters
    // movecamera( 154, 157, 780, 6, 270, 0.5, -30 )
    if (t_argc < 6)  goto err_numarg;
    if (t_argc > 6)
        I_SoftError( "movecamera: wrong num arg (%i), extra ignored\n", t_argc );
#else
    // This is correct, but causes Phobiata.wad to fail.
    if (t_argc != 6)  goto err_numarg;
#endif

    camera = MobjForSvalue(t_argv[0]);
    if( ! camera )  goto err_nomobj;
    target = MobjForSvalue(t_argv[1]);
    if( ! target )  goto err_nomobj;
    targetheight = fixedvalue(t_argv[2]);
    movespeed = fixedvalue(t_argv[3]);
    targetangle = FixedToAngle(fixedvalue(t_argv[4]));
    anglespeed = FixedToAngle(fixedvalue(t_argv[5]));

    // Figure out how big the step will be
    xdist = target->x - camera->x;
    ydist = target->y - camera->y;
    zdist = targetheight - camera->z;

    // Angle checking...
    //    90
    //   Q1|Q0
    //180--+--0
    //   Q2|Q3
    //    270
    quad1 = targetangle / ANG90;
    quad2 = camera->angle / ANG90;
    bigangle = targetangle > camera->angle ? targetangle : camera->angle;
    smallangle = targetangle < camera->angle ? targetangle : camera->angle;
    if ((quad1 > quad2 && quad1 - 1 == quad2) || (quad2 > quad1 && quad2 - 1 == quad1) || quad1 == quad2)
    {
        angledist = bigangle - smallangle;
        angledir = targetangle > camera->angle ? 1 : -1;
    }
    else
    {
        if (quad2 == 3 && quad1 == 0)
        {
            angledist = (bigangle + ANG180) - (smallangle + ANG180);
            angledir = 1;
        }
        else if (quad1 == 3 && quad2 == 0)
        {
            angledist = (bigangle + ANG180) - (smallangle + ANG180);
            angledir = -1;
        }
        else
        {
            angledist = bigangle - smallangle;
            if (angledist > ANG180)
            {
                angledist = (bigangle + ANG180) - (smallangle + ANG180);
                angledir = targetangle > camera->angle ? -1 : 1;
            }
            else
                angledir = targetangle > camera->angle ? 1 : -1;
        }
    }

    //debug_Printf("angle: cam=%i, target=%i; dir: %i; quads: 1=%i, 2=%i\n", camera->angle / ANGLE_1, targetangle / ANGLE_1, angledir, quad1, quad2);
    // set the step variables based on distance and speed...
    mobjangle = R_PointToAngle2(camera->x, camera->y, target->x, target->y);

    if (movespeed)
    {
        xydist = R_PointToDist2(camera->x, camera->y, target->x, target->y);
        xstep = FixedMul( cosine_ANG(mobjangle), movespeed);
        ystep = FixedMul( sine_ANG(mobjangle), movespeed);
        if (xydist)
            zstep = FixedDiv(zdist, FixedDiv(xydist, movespeed));
        else
            zstep = zdist > 0 ? movespeed : -movespeed;

        if (xydist && !anglespeed)
        {
#if 1
            // [WDJ] Without using ANGLE_1, which has a significant round-off error.
            fangledist = ((double) angledist * 45.0f / ANG45);
            fmovestep = ((double) FixedDiv(xydist, movespeed) / FRACUNIT);
            if (fmovestep)
                fanglestep = (fangledist / fmovestep);
            else
                fanglestep = 360.0f;

            //debug_Printf("fstep: %f, fdist: %f, fmspeed: %f, ms: %i\n", fanglestep, fangledist, fmovestep, FixedDiv(xydist, movespeed) >> FRACBITS);

            anglestep = (fanglestep * ANG45 / 45.0f);
#else
            // [WDJ] ANGLE_1 (from Heretic) has a significant round-off error.
            fangledist = ((double) angledist / ANGLE_1);
            fmovestep = ((double) FixedDiv(xydist, movespeed) / FRACUNIT);
            if (fmovestep)
                fanglestep = (fangledist / fmovestep);
            else
                fanglestep = 360;

            //debug_Printf("fstep: %f, fdist: %f, fmspeed: %f, ms: %i\n", fanglestep, fangledist, fmovestep, FixedDiv(xydist, movespeed) >> FRACBITS);

            anglestep = (fanglestep * ANGLE_1);
#endif
        }
        else
            anglestep = anglespeed;

        if (abs(xstep) >= (abs(xdist) - 1))
            x = target->x;
        else
        {
            x = camera->x + xstep;
            moved = 1;
        }

        if (abs(ystep) >= (abs(ydist) - 1))
            y = target->y;
        else
        {
            y = camera->y + ystep;
            moved = 1;
        }

        if (abs(zstep) >= abs(zdist) - 1)
            z = targetheight;
        else
        {
            z = camera->z + zstep;
            moved = 1;
        }
    }
    else
    {
        // instantaneous
        x = camera->x;
        y = camera->y;
        z = camera->z;
    }

    if (anglestep >= angledist)
        camera->angle = targetangle;
    else
    {
        if (angledir == 1)
        {
            moved = 1;
            camera->angle += anglestep;
        }
        else if (angledir == -1)
        {
            moved = 1;
            camera->angle -= anglestep;
        }
    }

    if (x != camera->x || y != camera->y)
    {
       if ( !P_TryMove(camera, x, y, true) )
       {
#if 1
          // [WDJ] force it past the obstruction, no errors,
          // keep game playable, do not police script during gameplay
          camera->x = x;
          camera->y = y;
#else
          goto err_move;
#endif
       }
    }
    camera->z = z;

done:
    t_return.type = FSVT_int;
    t_return.value.i = moved;
    return;

err_numarg:
    wrong_num_arg("MoveCamera", 6);
    goto done;

err_nomobj:
    script_error("MoveCamera: Invalid mobj\n");
    goto done;
    
#if 0
err_move:
    // [WDJ] Docs: no error
    // [WDJ] Questionable, not even good for debugging path
//    script_error("MoveCamera: Illegal camera move\n");
    moved = 0;  // to exit loop
    goto done;
#endif
}

/*********** sounds ******************/

// start sound from thing
// StartSound( mobj, sound_lump_name )
void SF_StartSound(void)
{
    mobj_t *mo;

    if (t_argc != 2)  goto err_numarg;
    if (t_argv[1].type != FSVT_string)  goto err_notsound;

    mo = MobjForSvalue(t_argv[0]);
    if (!mo)  goto done;

    // [WDJ] Sound by name, any remap is handled in S_StartXYZSoundName.
    S_StartXYZSoundName((xyz_t*)&(mo->x), mo, t_argv[1].value.s);
done:
    return;

err_numarg:
    wrong_num_arg("StartSound", 2);
    goto done;

err_notsound:
    script_error("StartSound: sound lump argument not a string!\n");
    goto done;
}

// start sound from sector
// StartSectorSound( tagnum, sound_lump_name )
void SF_StartSectorSound(void)
{
    int secnum;
    uint16_t tagnum;

    if (t_argc != 2)  goto err_numarg;
    if (t_argv[1].type != FSVT_string)  goto err_notsound;

    tagnum = intvalue(t_argv[0]);  // sector tag

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;

    secnum = -1; // init for search FindSector
    while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        // all sectors with the tagnum
        sector_t * sector = &sectors[secnum];
        // [WDJ] Sound by name, any remap is handled in S_StartXYZSoundName.
        S_StartXYZSoundName(&sector->soundorg, NULL, t_argv[1].value.s);
    }
done:
    return;

err_numarg:
    wrong_num_arg("StartSectorSound", 2);
    goto done;

err_notsound:
    script_error("StartSectorSound: sound lump argument not a string!\n");
    goto done;

err_nosector:
    script_error("StartSectorSound: sector not found with tagnum %i\n", tagnum);
    goto done;
}

// AmbiantSound( sound_lump_name )
void SF_AmbiantSound(void)
{
    if (t_argc != 1)  goto err_numarg;
    if (t_argv[0].type != FSVT_string)  goto err_notsound;

    // [WDJ] Sound by name, any remap is handled in S_StartXYZSoundName.
    S_StartXYZSoundName(NULL, NULL, t_argv[0].value.s);
done:
    return;

err_numarg:
    wrong_num_arg("AmbiantSound", 1);
    goto done;

err_notsound:
    script_error("AmbiantSound: sound lump argument not a string!\n");
    goto done;
}

/************* Sector functions ***************/

// Not in Docs
// [WDJ] Does not match code: SectorEffect(tagnum, [effect]) 
// [WDJ] Does not test, no need for return value
// Set, no return value
// SectorEffect(tagnum, effect)
void SF_SectorEffect(void)
{
    sector_t * sector;
    int select, secnum;
    uint16_t tagnum;

    if (t_argc != 2)  goto err_numarg;  // [WDJ]

    tagnum = intvalue(t_argv[0]);
    select = intvalue(t_argv[1]);

    secnum = -1; // init for search FindSector
    // silent when no sectors found
    while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        // all sectors with the tagnum
        sector = &sectors[secnum];

        switch (select)
        {
          case 1:
            // FLICKERING LIGHTS
            P_SpawnLightFlash (sector);
            break;

          case 2:
            // STROBE FAST
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            break;

          case 3:
            // STROBE SLOW
            P_SpawnStrobeFlash(sector,SLOWDARK,0);
            break;

          case 8:
            // GLOWING LIGHT
            P_SpawnGlowingLight(sector);
            break;

          //case 9: SECRET SECTOR

          case 10:
            // DOOR CLOSE IN 30 SECONDS
            P_SpawnDoorCloseIn30 (sector);
            break;

          case 12:
            // SYNC STROBE SLOW
            P_SpawnStrobeFlash (sector, SLOWDARK, 1);
            break;

          case 13:
            // SYNC STROBE FAST
            P_SpawnStrobeFlash (sector, FASTDARK, 1);
            break;

          case 14:
            // DOOR RAISE IN 5 MINUTES
            P_SpawnDoorRaiseIn5Mins (sector, secnum);
            break;

          case 17:
            //LIGHT FLICKERS RANDOMLY
            P_SpawnFireFlicker(sector);
            break;
        }
    }

#if 0
    // [WDJ] Why would this need to return an input arg
    t_return.type = FSVT_int;
    t_return.value.i = select;
#endif
done:
    return;

err_numarg:
    wrong_num_arg("SectorEffect", 1);
    goto done;
}

// floor height of sector
// Test or Set
// FloorHeight( tagnum, {floorheight}, {crush} )
// Return Test floorheight
// Return when Set floorheight: 1=default, 0=crushing
void SF_FloorHeight(void)
{
    int returnval = 1;  // When Set floorheight: 1=default, 0=crushing
    int secnum;
    uint16_t tagnum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_int;

    tagnum = intvalue(t_argv[0]);

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;

    if (t_argc > 1)     // > 1: set floorheight
    {
        // set floorheight
        fixed_t arg_flrheight = fixedvalue(t_argv[1]);  // set floorheight
        boolean arg_crush = (t_argc == 3) ? intvalue(t_argv[2]) : false;

        // set all sectors with tag
        secnum = -1;
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sector_t * sec = &sectors[secnum];
            result_e res =
             T_MovePlane(sec,
                         abs(arg_flrheight - sec->floorheight), // speed
                         arg_flrheight, arg_crush, 0,  // dest, crush, move floor
                         (arg_flrheight > sec->floorheight) ? 1 : -1); // direction
            if ( res == MP_crushed)
                returnval = 0;  // signal crushing
        }
    }
    else
    {
        // get floorheight
        returnval = sectors[secnum].floorheight >> FRACBITS;
    }

    // return floorheight, or crushing
    t_return.value.i = returnval;
done:
    return;

err_numarg:
    missing_arg_str("FloorHeight", "1, 2 or 3");
    goto done;

err_nosector:
    script_error("FloorHeight: sector not found with tagnum %i\n", tagnum);
    goto done;
}

// MoveFloor( tagnum, destheight, {speed} )
void SF_MoveFloor(void)
{
    int secnum;
    int platspeed;
    fixed_t destheight;
    uint16_t tagnum;

    if (t_argc < 2)  goto err_numarg;

    tagnum = intvalue(t_argv[0]);
    destheight = intvalue(t_argv[1]) << FRACBITS;
    platspeed = FLOORSPEED;
    if(t_argc > 2)
        platspeed *= intvalue(t_argv[2]);  // speed multiplier

    // move all sectors with tag
    secnum = -1;  // init search
    while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        sector_t * sec = &sectors[secnum];
        floormove_t * mfloor;

        // Don't start a second thinker on the same floor
        if (P_SectorActive( S_floor_special, sec))
            continue;

        mfloor = Z_Malloc(sizeof(floormove_t), PU_LEVSPEC, 0);
        P_AddThinker(&mfloor->thinker);
        sec->floordata = mfloor;
        mfloor->thinker.function = TFI_MoveFloor;
        mfloor->type = -1;       // not done by line
        mfloor->crush = false;

        mfloor->direction = (destheight < sec->floorheight) ? -1 : 1;
        mfloor->sector = sec;
        mfloor->speed = platspeed;
        mfloor->floordestheight = destheight;
    }
done:
    return;

err_numarg:
    missing_arg_str("MoveFloor", "2 or 3");
    goto done;
}

// ceiling height of sector
// CeilingHeight( tagnum, {ceilheight}, {crush} )
// Return Test ceilingheight
// Return when Set ceilingheight: 1=default, 0=crushing
void SF_CeilingHeight(void)
{
    int returnval = 1;  // When Set ceilingheight: 1=default, 0=crushing
    int secnum;
    uint16_t tagnum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_int;

    tagnum = intvalue(t_argv[0]);

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;

    if (t_argc > 1)     // > 1: set ceilheight
    {
        // set ceilingheight
        fixed_t arg_cheight = fixedvalue(t_argv[1]);  // set ceilingheight
        boolean arg_crush = (t_argc == 3) ? intvalue(t_argv[2]) : false;

        // set all sectors with tag
        secnum = -1;
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sector_t * sec = &sectors[secnum];
            result_e res =
             T_MovePlane(sec,
                         abs(arg_cheight - sec->ceilingheight), // speed
                         arg_cheight, arg_crush, 1,  // dest, crush, move ceiling
                         (arg_cheight > sec->ceilingheight) ? 1 : -1); // direction
            if ( res == MP_crushed)
                returnval = 0;  // signal crushing
        }
    }
    else
    {
        // get ceilingheight
        returnval = sectors[secnum].ceilingheight >> FRACBITS;
    }

    // return floorheight, or crushing
    t_return.value.i = returnval;
done:
    return;

err_numarg:
    missing_arg_str("CeilingHeight", "1, 2 or 3");
    goto done;

err_nosector:
    script_error("CeilingHeight: sector not found with tagnum %i\n", tagnum);
    goto done;
}

// MoveCeiling( tagnum, destheight, {speed} )
void SF_MoveCeiling(void)
{
    int secnum;
    int platspeed;
    fixed_t destheight;
    uint16_t tagnum;

    if (t_argc < 2)  goto err_numarg;

    tagnum = intvalue(t_argv[0]);
    destheight = intvalue(t_argv[1]) << FRACBITS;
    platspeed = FLOORSPEED;
    if(t_argc > 2)
        platspeed *= intvalue(t_argv[2]);  // speed multiplier

    // move all sectors with tag
    secnum = -1;  // init search
    while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
    {
        sector_t * sec = &sectors[secnum];
        ceiling_t * mceiling;

        // Don't start a second thinker on the same floor
        if (P_SectorActive( S_ceiling_special, sec))
            continue;

        mceiling = Z_Malloc(sizeof(ceiling_t), PU_LEVSPEC, 0);
        P_AddThinker(&mceiling->thinker);
        sec->ceilingdata = mceiling;
        mceiling->thinker.function = TFI_MoveCeiling;
        mceiling->type = CT_genCeiling;     // not done by line
        mceiling->crush = false;

        mceiling->direction = destheight < sec->ceilingheight ? -1 : 1;
        mceiling->sector = sec;
        mceiling->speed = platspeed;
        // just set top and bottomheight the same
        mceiling->topheight = mceiling->bottomheight = destheight;

        mceiling->tag = sec->tag;
        P_AddActiveCeiling(mceiling);
    }
done:
    return;

err_numarg:
    missing_arg_str("MoveCeiling", "2 or 3");
    goto done;
}

// Test or Set
// Lightlevel( tagnum, {lightlevel} )
void SF_LightLevel(void)
{
    sector_t *sector;
    int secnum;
    uint16_t tagnum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_int;

    tagnum = intvalue(t_argv[0]);

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;
    sector = &sectors[secnum];  // first sector for return value

    if (t_argc > 1)     // > 1: set
    {
        // set all sectors with tag
        lightlev_t arg_light = intvalue(t_argv[1]);
        secnum = -1;  // init search
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            // all sectors with tagnum
            sectors[secnum].lightlevel = arg_light;
        }
    }

    // return lightlevel
    t_return.value.i = sector->lightlevel;
done:
    return;

err_numarg:
    missing_arg_str("LightLevel", "1 or 2");
    goto done;

err_nosector:
    script_error("LightLevel: sector not found with tagnum %i\n", tagnum);
    goto done;
}

// Set
// Lightlevel( tagnum, lightlevel, {speed} )
void SF_FadeLight(void)
{
    int sectag, destlevel, speed = 1;

    if (t_argc < 2)  goto err_numarg;

    sectag = intvalue(t_argv[0]);
    destlevel = intvalue(t_argv[1]);
    speed = (t_argc > 2) ? intvalue(t_argv[2]) : 1;

    P_FadeLight(sectag, destlevel, speed);
done:
    return;

err_numarg:
    missing_arg_str("FadeLight", "2 or 3");
    goto done;
}

// Test or Set
// FloorTexture( tagnum, {flatname} )
void SF_FloorTexture(void)
{
    sector_t *sector;
    int secnum;
    uint16_t tagnum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_string;
    t_return.value.s = "";  // default

    tagnum = intvalue(t_argv[0]);

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;
    sector = &sectors[secnum];  // first sector for return value

    if (t_argc > 1)
    {
        // set texture
        lumpnum_t picnum = R_FlatNumForName(t_argv[1].value.s);
            // when flat not found, defaults to first flat

        // set all sectors with tag
        secnum = -1;  // init search
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sectors[secnum].floorpic = picnum;
        }
    }

    t_return.value.s = P_FlatNameForNum(sector->floorpic);
done:
    return;

err_numarg:
    missing_arg_str("FloorTexture", "1 or 2");
    goto done;

err_nosector:
    script_error("FloorTexture: sector not found with tagnum %i\n", tagnum);
    goto done;
}

// Test or Set
// SectorColormap( tagnum, {mapnum} )
// mapnum= -1 removes the colormap
void SF_SectorColormap(void)
{
    sector_t *sector;
    int secnum;
    uint16_t tagnum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_string;
    t_return.value.s = "";  // default

    tagnum = intvalue(t_argv[0]);

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;
    sector = &sectors[secnum];  // first sector for return value

    if (t_argc > 1)
    {
        int mapnum = R_ColormapNumForName(t_argv[1].value.s);

        // set all sectors with tag
        secnum = -1; // init search
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sector_t * sec = &sectors[secnum];
            if (mapnum == -1)
            {
                // remove colormap
                sec->midmap = 0;
                sec->model = SM_normal;
                sec->modelsec = 0;
            }
            else
            {
                // set colormap
                sec->midmap = mapnum;
                sec->model = SM_colormap;
                sec->modelsec = 0;
            }
        }
    }

    t_return.value.s = R_ColormapNameForNum(sector->midmap);
done:
    return;

err_numarg:
    missing_arg_str("SectorColormap", "1 or 2");
    goto done;

err_nosector:
    script_error("SectorColormap: sector not found with tagnum %i\n", tagnum);
    goto done;
}

// Test or Set
// CeilingTexture( tagnum, {flatname} )
void SF_CeilingTexture(void)
{
    sector_t *sector;
    int secnum;
    uint16_t tagnum;

    if (!t_argc)  goto err_numarg;

    t_return.type = FSVT_string;
    t_return.value.s = "";  // default

    tagnum = intvalue(t_argv[0]);

    // check on existing tagged sector
    secnum = P_FindSectorFromTag(tagnum, -1);
    if (secnum < 0)  goto err_nosector;
    sector = &sectors[secnum];  // first sector for return value

    if (t_argc > 1)
    {
        // set texture
        int picnum = R_FlatNumForName(t_argv[1].value.s);
            // when flat not found, defaults to first flat

        // set all sectors with tag
        secnum = -1; // init search
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sectors[secnum].ceilingpic = picnum;
        }
    }

    t_return.value.s = P_FlatNameForNum(sector->ceilingpic);
done:
    return;

err_numarg:
    missing_arg_str("CeilingTexture", "1 or 2");
    goto done;

err_nosector:
    script_error("CeilingTexture: sector not found with tagnum %i\n", tagnum);
    goto done;
}

void SF_ChangeHubLevel(void)
{
/*  uint16_t tagnum;

  if(!t_argc)
    {
      script_error("hub level to go to not specified!\n");
      return;
    }
  if(t_argv[0].type != FSVT_string)
    {
      script_error("level argument is not a string!\n");
      return;
    }

  // second argument is tag num for 'seamless' travel
  if(t_argc > 1)
    tagnum = intvalue(t_argv[1]);
  else
    tagnum = -1;

  P_SavePlayerPosition(fs_current_script->trigger->player, tagnum);
  P_ChangeHubLevel(t_argv[0].value.s);*/
}

// for start map: start new game on a particular skill
// StartSkill( skill )
// skill = 1..5
void SF_StartSkill(void)
{
    int skill;

    if (t_argc != 1)  goto err_numarg;

    // -1: 1-5 is how we normally see skills
    // 0-4 is how doom sees them

    skill = intvalue(t_argv[0]) - 1;

    // skill 0..4
    G_DeferedInitNew(skill, G_BuildMapName(1, 1), false);
done:
    return;

err_numarg:
    wrong_num_arg("StartSkill", 1);
    goto done;
}

//////////////////////////////////////////////////////////////////////////
//
// Doors
//

// OpenDoor(tagnum, [delay], [speed])
void SF_OpenDoor(void)
{
    int speed;
    int wait_time;
    uint16_t tagnum;

    if (t_argc < 1)  goto err_numarg;

    // got sector tag
    tagnum = intvalue(t_argv[0]);

    // door wait time
    if (t_argc > 1)     // door wait time
        wait_time = (intvalue(t_argv[1]) * 35) / 100;
    else
        wait_time = 0;  // 0= stay open

    // door speed
    if (t_argc > 2)
        speed = intvalue(t_argv[2]);
    else
        speed = 1;      // 1= normal speed

    EV_OpenDoor(tagnum, speed, wait_time);
done:
    return;

err_numarg:
    missing_arg_str("OpenDoor", "1, 2, or 3");
    goto done;
}

// CloseDoor(tagnum, [speed])
void SF_CloseDoor(void)
{
    int speed;
    uint16_t tagnum;

    if (t_argc < 1)  goto err_numarg;

    // got sector tag
    tagnum = intvalue(t_argv[0]);

    // door speed
    if (t_argc > 1)
        speed = intvalue(t_argv[1]);
    else
        speed = 1;      // 1= normal speed

    EV_CloseDoor(tagnum, speed);
done:
    return;

err_numarg:
    missing_arg_str("CloseDoor", "1 or 2");
    goto done;
}

// play demo, internal lump or external file
// PlayDemo( lumpname )
void SF_PlayDemo(void)
{
    if (t_argc != 1)  goto err_numarg;
    if (t_argv[0].type != FSVT_string)  goto err_notlump;

    G_DoPlayDemo(t_argv[0].value.s);
    // if name is lump, then play lump,
    // otherwise it adds ".lmp" and reads demo file
done:
    return;

err_numarg:
    wrong_num_arg("PlayDemo", 1);
    goto done;

err_notlump:
    script_error("PlayDemo: not a lump name\n");
    goto done;
}

// run console cmd
// RunCommand( cmdstring, ... )   upto 128 strings
void SF_RunCommand(void)
{
    char * tempstr = Z_cat_args(0);  // concat arg0, arg1, ...
    // [WDJ] May be too long to terminate with va( "%s\n", tempstr ).
    // Z_cat_args allocates 3 extra chars for termination.
    strcat(tempstr, "\n" );
    COM_BufAddText( tempstr );
    Z_Free(tempstr);
}

// return the (string) value of a cvar
void SF_CheckCVar(void)
{
    consvar_t *cvar;

    if (t_argc != 1)  goto err_numarg;

    t_return.type = FSVT_string;

    if ((cvar = CV_FindVar(stringvalue(t_argv[0]))))
    {
        t_return.value.s = cvar->string;
    }
    else
    {
        t_return.value.s = "";
    }
done:
    return;

err_numarg:
    wrong_num_arg("CheckCVar", 1);
    goto done;
}


//DarkWolf95:July 23, 2003:Return/Set LineTexture Yay!
// Set
// SetLineTexture( tagnum, texturename, sidenum, sectionflags )
// sidenum: 0, 1
// sectionflags: 1 = top 2 = mid 4 = bot
void SF_SetLineTexture(void)
{
    int linenum;
    unsigned int side;
    int sectionflags;
//    line_t *line;
    uint16_t tagnum;

    if(t_argc != 4)  goto err_numarg;

    tagnum = intvalue(t_argv[0]);

    // check on existing line
    linenum = P_FindLineFromTag(tagnum, -1);
    if(linenum < 0)  goto err_noline;
//    line = &lines[linenum];  // for return value

    side = (unsigned) intvalue(t_argv[2]);
    if( side > 1 )  side = 1;  // easier than an error
    sectionflags = intvalue(t_argv[3]);

    {
      // set textures
      short picnum = R_TextureNumForName(t_argv[1].value.s);
        // returns 0=no-texture, or default texture, otherwise valid

      // set all sectors with tag
      linenum = -1; // init search
      while (( linenum = P_FindLineFromTag(tagnum, linenum)) >= 0)
      {
          line_t * linep = &lines[linenum];
          if (linep->sidenum[side] == NULL_INDEX)
          {
#if 0
              // [WDJ] Unnecessary error, error is not in script
              script_error("nonexistant side\n");
              goto done;
#endif
          }
          else
          {
              side_t * sidep = & sides[linep->sidenum[side]];
              // textures, 0=no-texture, otherwise valid
              if(sectionflags & 1)
                 sidep->toptexture = picnum;
              if(sectionflags & 2)
                 sidep->midtexture = picnum;
              if(sectionflags & 4)
                 sidep->bottomtexture = picnum;
          }
      }
    }
done:
    return;

err_numarg:
    wrong_num_arg("SetLineTexture", 1);
    goto done;

err_noline:   
    script_error("line not found with tagnum %i\n", tagnum);
    goto done;
}

// Imitate a linedef trigger of any linedef type
// LineTrigger( special, {tagnum} )
// With tagnum, or with tag of 0
void SF_LineTrigger(void)
{
    line_t sf_tmpline;

    if (!t_argc)  goto err_numarg;

    sf_tmpline.special = intvalue(t_argv[0]);
    sf_tmpline.tag = (t_argc>1)? intvalue(t_argv[1]) : 0;
    // [WDJ] used by P_UseSpecialLine and functions in p_genlin
    sf_tmpline.flags = 0;
    sf_tmpline.sidenum[0] = NULL_INDEX;
    sf_tmpline.sidenum[1] = NULL_INDEX;
    sf_tmpline.backsector = NULL;
    sf_tmpline.frontsector = NULL;

    P_UseSpecialLine(fs_run_trigger, &sf_tmpline, 0);      // Try using it
    P_CrossSpecialLine(&sf_tmpline, 0, fs_run_trigger); // Try crossing it
done:
    return;

err_numarg:
    missing_arg_str("LineTrigger", "1 or 2");
    goto done;
}

// Test or Set
// LineFlag( linenum, {flagnum}, {flagvalue} )
// flagnum: flag bit position, 0..31
// flagvalue: 0, 1
void SF_LineFlag(void)
{
    line_t *line;
    int linenum;
    int flagnum;
    uint32_t flagmsk;

    if (t_argc < 2)  goto err_numarg;

    t_return.type = FSVT_int;

    linenum = intvalue(t_argv[0]);
    if (linenum < 0 || linenum > numlines)  goto err_noline;
    line = & lines[linenum];  // for test, set, clear

    flagnum = intvalue(t_argv[1]);
    if (flagnum < 0 || flagnum > 32)  goto err_flagnum;
    flagmsk = (1 << flagnum);

    if (t_argc > 2)
    {   // Clear or Set flags
        line->flags &= ~flagmsk;
        if (intvalue(t_argv[2]))
            line->flags |= flagmsk;
    }

    t_return.value.i = line->flags & flagmsk;
done:
    return;

err_numarg:
    missing_arg_str("LineFlag", "1 or 2");
    goto done;

err_noline:
    script_error("LineFlag: invalid line number %i\n", linenum);
    goto done;

err_flagnum:
    script_error("LineFlag: invalid flag number %i\n", flagnum);
    goto done;
}

// ChangeMusic( namestring )
void SF_ChangeMusic(void)
{
    if (t_argc != 1)  goto err_numarg;
    if (t_argv[0].type != FSVT_string)  goto err_notstring;

    S_ChangeMusicName(t_argv[0].value.s, 1);
done:
    return;

err_numarg:
    wrong_num_arg("ChangeMusic", 1);
    goto done;

err_notstring:
    script_error("ChangeMusic: require music name string\n");
    goto done;
}

// SoM: Max and Min math functions.
// Max( v1, v2 )
void SF_Max(void)
{
    fixed_t n1, n2;

    if (t_argc != 2)  goto err_numarg;

    n1 = fixedvalue(t_argv[0]);
    n2 = fixedvalue(t_argv[1]);

    t_return.type = FSVT_fixed;
    t_return.value.f = n1 > n2 ? n1 : n2;
done:
    return;

err_numarg:
    wrong_num_arg("Max", 2);
    goto done;
}

// Min( v1, v2 )
void SF_Min(void)
{
    fixed_t n1, n2;

    if (t_argc != 2)  goto err_numarg;

    n1 = fixedvalue(t_argv[0]);
    n2 = fixedvalue(t_argv[1]);

    t_return.type = FSVT_fixed;
    t_return.value.f = n1 < n2 ? n1 : n2;
done:
    return;

err_numarg:
    wrong_num_arg("Min", 2);
    goto done;
}

// Abs( v1 )
void SF_Abs(void)
{
    fixed_t n1;

    if (t_argc != 1)  goto err_numarg;

    n1 = fixedvalue(t_argv[0]);

    t_return.type = FSVT_fixed;
//    t_return.value.f = n1 < 0 ? n1 * -1 : n1;
    t_return.value.f = abs(n1);
done:
    return;

err_numarg:
    wrong_num_arg("Abs", 1);
    goto done;
}

//Hurdler: some new math functions
fixed_t double2fixed(double t)
{
    double fl = floor(t);
    return ((int) fl << 16) | (int) ((t - fl) * 65536.0);
}

// Sin( angle )
// angle: radians
void SF_Sin(void)
{
    if (t_argc != 1)  goto err_numarg;
   
    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(sin(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("Sin", 1);
    goto done;
}

// ASin( v1 )
// Return angle: radians
void SF_ASin(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(asin(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("ASin", 1);
    goto done;
}

// Cos( angle )
// angle: radians
void SF_Cos(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(cos(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("Cos", 1);
    goto done;
}

// ACos( v1 )
// Return angle: radians
void SF_ACos(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(acos(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("ACos", 1);
    goto done;
}

// Tan( angle )
// angle: radians
void SF_Tan(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(tan(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("Tan", 1);
    goto done;
}

// ATan( v1 )
// Return angle: radians
void SF_ATan(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(atan(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("ATan", 1);
    goto done;
}

void SF_Exp(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(exp(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("Exp", 1);
    goto done;
}

// Log( v1 )
void SF_Log(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(log(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("Log", 1);
    goto done;
}

void SF_Sqrt(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = double2fixed(sqrt(FIXED_TO_FLOAT(n1)));
    }
done:
    return;

err_numarg:
    wrong_num_arg("Sqrt", 1);
    goto done;
}

// Floor( v1 )
void SF_Floor(void)
{
    if (t_argc != 1)  goto err_numarg;

    {
        fixed_t n1 = fixedvalue(t_argv[0]);
        t_return.type = FSVT_fixed;
        t_return.value.f = n1 & 0xffFF0000;
    }
done:
    return;

err_numarg:
    wrong_num_arg("Floor", 1);
    goto done;
}

// Pow( v1, v2 )
void SF_Pow(void)
{
    fixed_t n1, n2;

    if (t_argc != 2)  goto err_numarg;

    n1 = fixedvalue(t_argv[0]);
    n2 = fixedvalue(t_argv[1]);

    t_return.type = FSVT_fixed;
    t_return.value.f = double2fixed(pow(FIXED_TO_FLOAT(n1), FIXED_TO_FLOAT(n2)));
done:
    return;

err_numarg:
    wrong_num_arg("Pow", 2);
    goto done;
}




// Type forcing functions -- useful with arrays et al

// MobjValue( mobj )
void SF_MobjValue(void)
{
    if(t_argc != 1)  goto err_numarg;
    t_return.type = FSVT_mobj;
    t_return.value.mobj = MobjForSvalue(t_argv[0]);
done:
    return;

err_numarg:
    wrong_num_arg("MobjValue", 1);
    goto done;
}

// StringValue( v )
void SF_StringValue(void)
{  
   if(t_argc != 1)  goto err_numarg;
   t_return.type = FSVT_string;
   t_return.value.s = Z_Strdup(stringvalue(t_argv[0]), PU_LEVEL, 0);
done:
    return;

err_numarg:
    wrong_num_arg("StringValue", 1);
    goto done;
}

// IntValue( v1 )
void SF_IntValue(void)
{
   if(t_argc != 1)  goto err_numarg;
   t_return.type = FSVT_int;
   t_return.value.i = intvalue(t_argv[0]);
done:
    return;

err_numarg:
    wrong_num_arg("IntValue", 1);
    goto done;
}

// FixedValue( v1 )
void SF_FixedValue(void)
{
   if(t_argc != 1)  goto err_numarg;
   t_return.type = FSVT_fixed;
   t_return.value.f = fixedvalue(t_argv[0]);
done:
    return;

err_numarg:
    wrong_num_arg("FixedValue", 1);
    goto done;
}




//////////////////////////////////////////////////////////////////////////
// FraggleScript HUD graphics
//////////////////////////////////////////////////////////////////////////

// alias createpic
// NewHUPic( lumpname, x, y )
// Return pic_handle
void SF_NewHUPic(void)
{
    if (t_argc != 3)  goto err_numarg;
    t_return.type = FSVT_int;
    t_return.value.i =
      HU_Get_FSPic( W_GetNumForName(stringvalue(t_argv[0])),
                    intvalue(t_argv[1]), intvalue(t_argv[2]));
done:
    return;

err_numarg:
    wrong_num_arg("NewHUPic", 3);
    goto done;
}

// alias deletehupic
// DeleteHUPic( pic_handle )
void SF_DeleteHUPic(void)
{
    int handle;

    if (t_argc != 1)  goto err_numarg;

    handle = intvalue(t_argv[0]);
    if (HU_Delete_FSPic(handle) == -1)  goto err_delete;
done:
    return;

err_numarg:
    wrong_num_arg("DeleteHUPic", 1);
    goto done;
 
err_delete:
    script_error("DeleteHUPic: invalid sfpic handle %i\n", handle);
    goto done;
}

// alias modifyhupic, modifypic
// ModifyHUPic( handle, lumpname, x, y )
void SF_ModifyHUPic(void)
{
    int handle;

    if (t_argc != 4)  goto err_numarg;

    handle = intvalue(t_argv[0]);
    if (HU_Modify_FSPic(handle, W_GetNumForName(stringvalue(t_argv[1])),
                       intvalue(t_argv[2]), intvalue(t_argv[3])) == -1)
        goto err_handle;
done:
    return;

err_numarg:
    wrong_num_arg("ModifyHUPic", 4);
    goto done;
 
err_handle:
    script_error("ModifyHUPic: invalid sfpic handle %i\n", handle);
    goto done;
}

// alias setpicvisible
// SetHUPicDisplay( handle, drawenable )
// drawenable: 0,1
void SF_SetHUPicDisplay(void)
{
    int handle;

    if (t_argc != 2)  goto err_numarg;

    handle = intvalue(t_argv[0]);
    if (HU_FS_Display(handle, intvalue(t_argv[1]) > 0 ? 1 : 0) == -1)
        goto err_handle;
done:
    return;

err_numarg:
    wrong_num_arg("SetHUPicDisplay", 4);
    goto done;
 
err_handle:
    script_error("SetHUPicDisplay: invalid sfpic handle %i\n", handle);
    goto done;
}

// Hurdler: I'm enjoying FS capability :)


// Debug color setting.
//#define SHOW_COLOR_SETTING

// [WDJ] Rewritten to process Hex ARGB and RGB string, any length, any machine.
uint32_t String_to_RGBA( const char *s)
{
    // [WDJ] Handles any length hex value, and no macros.
    RGBA_t   valrgb;  // Cannot trust byte order of rgba component.
    uint32_t valhex = 0;
    // Convert string to hex
    while( *s ) {
       register byte v = *s;
       if( v >= '0' && v <= '9' )
       {
           v -= '0';
       }else if( v >= 'A' && v <= 'F' )
       {
           v -= ('A' - 10);
       }else if( v >= 'a' && v <= 'f' )
       {
           v -= ('a' - 10);
       }
       else
         break;  // Not a hex digit

       valhex = (valhex<<4) + v;
       s++;
    }
    // Convert hex to RGB bytes.
    // Macro UINT2RGBA does not work, gives wrong colors.
    // Was read as A R G B
    valrgb.s.blue = valhex;  // LSB byte
    valrgb.s.green = valhex>>8;
    valrgb.s.red = valhex>>16;
    valrgb.s.alpha = valhex>>24;
    return valrgb.rgba;
}


// SetCorona( id, attribute, value )
// id: corona id number
// attribute: corona attribute selection
// 
// SetCorona( id, type, xoffset, yoffset, color, radius, dynamic_color, dynamic_radius )
// id: corona id number
// type: bits (
//  CORONA_SPR=0x01,    // emit corona
//  DYNLIGHT_SPR=0x02,  // dynamic light
//  LIGHT_SPR=0x03, // CORONA_SPR|DYNLIGHT_SPR
//  ROCKET_SPR=0x13 // CORONA_SPR|DYNLIGHT_SPR with random radius
//  )
// color: rgba color
// radius: corona size
// dynamic_color: rgba wall lighting color
// dynamic_radius: wall lighting size
void SF_SetCorona(void)
{
    int num;
    spr_light_t * sl;

    if (t_argc != 3 && t_argc != 7)   goto err_numarg;

    num = t_argv[0].value.i;    // which corona we want to modify
    if( num >= NUMLIGHTS )
        return;
   
    sl = & sprite_light[num];

    //this function accept 2 kinds of parameters
    if (t_argc == 3)
    {
        int what = t_argv[1].value.i;   // what we want to modify (type, color, offset,...)
        int ival = t_argv[2].value.i;   // new value
        double fval = FIXED_TO_FLOAT( t_argv[2].value.f );  // fixed param
       
        // The fragglescript corona sets.
        switch (what)
        {
            case 0:  // CORONA_TYPE is int
                // Set sprite light corona lights.
                sl->splgt_flags = ival;
                sl->impl_flags |= SLI_type_set;  // a type was set
                break;
            case 1:  // CORONA_OFFX is fixed
                sl->light_xoffset = fval;  // unused
                break;
            case 2:  // CORONA_OFFY is fixed
                sl->light_yoffset = fval;
                break;
            case 3:  // CORONA_COLOR is (string or int)
                sl->corona_color.rgba = (t_argv[2].type == FSVT_string)?
                    String_to_RGBA(t_argv[2].value.s)
                    : ival;

#ifdef SHOW_COLOR_SETTING
                // Show the corona color setting.
                if( devparm > 2 )
                {
                    if(t_argv[2].type == FSVT_string)
                       debug_Printf( "CORONA_COLOR = %s, rgba=%x\n",
                                  t_argv[2].value.s, sl->corona_color.rgba );
                    else
                       debug_Printf( "CORONA_COLOR = %x, rgba=%x\n",
                                  ival, sl->corona_color.rgba );
                }
#endif
                // Phobiata fix. Color with no alpha, uses default of 0xff.
                if( sl->corona_color.s.alpha == 0 )
                {
                    sl->corona_color.s.alpha = 0xff;  // previous default
                }

                // Chex newmaps fix. The flags are set 0 for Chex1.
                // If CORONA_COLOR is set, then corona should be enabled.
                if( sl->splgt_flags == 0 )
                {
                    sl->splgt_flags = SPLGT_dynamic|SPLGT_corona|SPLT_light;  // firefly light
                }
                break;
            case 4:  // CORONA_SIZE is fixed
                sl->corona_radius = fval;
                break;
            case 5:  // LIGHT_COLOR is (string or int)
                sl->dynamic_color.rgba = (t_argv[2].type == FSVT_string)?
                    String_to_RGBA(t_argv[2].value.s)
                    : ival;
                // 0 means off, dynamic_alpha has not ever been defaulted

#ifdef SHOW_COLOR_SETTING
                // Show the dynamic color setting.
                if( devparm > 2 )
                {
                    if(t_argv[2].type == FSVT_string)
                       debug_Printf( "LIGHT_COLOR = %s, rgba=%x\n",
                                  t_argv[2].value.s, sl->dynamic_color.rgba );
                    else
                       debug_Printf( "LIGHT_COLOR = %x, rgba=%x\n",
                                  ival, sl->dynamic_color.rgba );
                }
#endif
                break;
            case 6:  // LIGHT_SIZE is fixed
                sl->dynamic_radius = fval;
                // According to usage and init this is squared-radius.
                sl->dynamic_sqrradius = fval * fval;
                break;
            default:
                I_SoftError("SetCorona: what %i\n", what);
                break;
        }
    }
    else
    {
        // Set all fields of sprite corona light.
        sl->splgt_flags = t_argv[1].value.i;
        sl->impl_flags |= SLI_type_set;  // a type was set
        sl->light_xoffset = FIXED_TO_FLOAT(t_argv[2].value.f);  // unused
        sl->light_yoffset = FIXED_TO_FLOAT(t_argv[3].value.f);
        sl->corona_color.rgba = (t_argv[4].type == FSVT_string)?
            String_to_RGBA(t_argv[4].value.s)
            : t_argv[4].value.i;

        sl->corona_radius = FIXED_TO_FLOAT(t_argv[5].value.f);

        // Phobiata fix. Color with no alpha, uses default of 0xff.
        if( sl->corona_color.s.alpha == 0 )
        {
            sl->corona_color.s.alpha = 0xff;  // previous default
        }

        sl->dynamic_color.rgba = (t_argv[6].type == FSVT_string)?
            String_to_RGBA(t_argv[6].value.s)
            : t_argv[6].value.i;
        // 0 means off, dynamic_alpha has not ever been defaulted

        sl->dynamic_radius = FIXED_TO_FLOAT(t_argv[7].value.f);
        // According to usage and init this is squared-radius.
        sl->dynamic_sqrradius = sl->dynamic_radius * sl->dynamic_radius;

#ifdef SHOW_COLOR_SETTING
        // Show the corona color setting.
        if( devparm > 2 )
        {
            if(t_argv[4].type == FSVT_string)
               debug_Printf( "CORONA_COLOR = %s, rgba=%x\n",
                          t_argv[4].value.s, sl->corona_color.rgba );
            else
               debug_Printf( "CORONA_COLOR = %x, rgba=%x\n",
                          t_argv[4].value.i, sl->corona_color.rgba );
            if(t_argv[2].type == FSVT_string)
               debug_Printf( "LIGHT_COLOR = %s, rgba=%x\n",
                          t_argv[6].value.s, sl->dynamic_color.rgba );
            else
               debug_Printf( "LIGHT_COLOR = %x, rgba=%x\n",
                          t_argv[6].value.i, sl->dynamic_color.rgba );
        }
#endif
    }

    sl->impl_flags |= (SLI_changed | SLI_corona_set);  // trigger check for missing settings
    return;

err_numarg:
    missing_arg_str("SetCorona", "3 or 7");
    return;
}


// Background color fades for FS
uint32_t  fs_fadecolor;
int       fs_fadealpha;


// Background color fades
// SetFade( red, green, blue, alpha )
void SF_SetFade(void)
{
    if (t_argc != 4)   goto err_numarg;

    // Calculate the background color value
    // Before was (R,G,B,A) format, which got reversed to (A,B,G,R).
//    fadecolor = (256 * b) + (65536 * g) + (16777216 * r);
    fs_fadecolor = RGBA( t_argv[0].value.i, t_argv[1].value.i, t_argv[2].value.i, 0);
    fs_fadealpha = t_argv[3].value.i;
done:
    return;

err_numarg:
    wrong_num_arg("SetFade", 4);
    goto done;
}


//////////////////////////////////////////////////////////////////////////
//
// Init Functions
//

//extern int fov; // r_main.c
int fov;

void T_Init_functions(void)
{
    // add all the functions
    add_game_int("consoleplayer", &consoleplayer);
    add_game_int("displayplayer", &displayplayer);
    add_game_int("fov", &fov);
    add_game_int("zoom", &fov); //SoM: BAKWARDS COMPATABILITY!
    add_game_mobj("trigger", &fs_trigger_obj);

    // important C-emulating stuff
    new_function("break", SF_Break);
    new_function("continue", SF_Continue);
    new_function("return", SF_Return);
    new_function("goto", SF_Goto);
    new_function("include", SF_Include);

    // standard FraggleScript functions
    new_function("print", SF_Print);
    new_function("rnd", SF_Rnd);
    new_function("prnd", SF_PRnd);
    new_function("input", SF_Input);    // Hurdler: TODO: document this function
    new_function("beep", SF_Beep);
    new_function("clock", SF_Clock);
    new_function("clocktic", SF_ClockTic);
    new_function("wait", SF_Wait);
    new_function("waittic", SF_WaitTic);
    new_function("tagwait", SF_TagWait);
    new_function("scriptwait", SF_ScriptWait);
    new_function("startscript", SF_StartScript);
    new_function("scriptrunning", SF_ScriptRunning);

    // doom stuff
    new_function("startskill", SF_StartSkill);
    new_function("exitlevel", SF_ExitLevel);
    new_function("warp", SF_Warp);
    new_function("tip", SF_Tip);
    new_function("timedtip", SF_TimedTip);
    new_function("message", SF_Message);
    new_function("gameskill", SF_GameSkill);
    new_function("gamemode", SF_GameMode);      // SoM Request SSNTails 06-13-2002

    // player stuff
    new_function("playermsg", SF_PlayerMsg);
    new_function("playertip", SF_PlayerTip);
    new_function("playeringame", SF_PlayerInGame);
    new_function("playername", SF_PlayerName);
    new_function("playeraddfrag", SF_PlayerAddFrag);
    new_function("playerobj", SF_PlayerObj);
    new_function("isobjplayer", SF_MobjIsPlayer);
    new_function("isplayerobj", SF_MobjIsPlayer);       // Hurdler: due to backward and eternity compatibility
    new_function("skincolor", SF_SkinColor);
    new_function("playerkeys", SF_PlayerKeys);
    new_function("playerkeysb", SF_PlayerKeysByte);
    new_function("playerarmor", SF_PlayerArmor);
    new_function("playerammo", SF_PlayerAmmo);
    new_function("maxplayerammo", SF_MaxPlayerAmmo);
    new_function("playerweapon", SF_PlayerWeapon);
    new_function("playerselwep", SF_PlayerSelectedWeapon);
    new_function("playerpitch", SF_PlayerPitch);
    new_function("playerproperty", SF_PlayerProperty);

    // mobj stuff
    new_function("spawn", SF_Spawn);
    new_function("spawnexplosion", SF_SpawnExplosion);
    new_function("radiusattack", SF_RadiusAttack);
    new_function("kill", SF_KillObj);
    new_function("removeobj", SF_RemoveObj);
    new_function("objx", SF_ObjX);
    new_function("objy", SF_ObjY);
    new_function("objz", SF_ObjZ);
    new_function("testlocation", SF_TestLocation);
    new_function("teleport", SF_Teleport);
    new_function("silentteleport", SF_SilentTeleport);
    new_function("damageobj", SF_DamageObj);
    new_function("healobj", SF_HealObj);
    new_function("player", SF_Player);
    new_function("objsector", SF_ObjSector);
    new_function("objflag", SF_ObjFlag);
    new_function("objflag2", SF_ObjFlag2);
    new_function("objeflag", SF_ObjEFlag);
    new_function("pushobj", SF_PushThing);
    new_function("pushthing", SF_PushThing);    // Hurdler: due to backward and eternity compatibility
    new_function("objangle", SF_ObjAngle);
    new_function("checksight", SF_CheckSight);
    new_function("objhealth", SF_ObjHealth);
    new_function("objdead", SF_ObjDead);
    new_function("objreactiontime", SF_ReactionTime);
    new_function("reactiontime", SF_ReactionTime);      // Hurdler: due to backward and eternity compatibility
    new_function("objtarget", SF_MobjTarget);
    new_function("objmomx", SF_MobjMomx);
    new_function("objmomy", SF_MobjMomy);
    new_function("objmomz", SF_MobjMomz);
    new_function("spawnmissile", SF_SpawnMissile);
    new_function("mapthings", SF_Mapthings);
    new_function("objtype", SF_ObjType);
    new_function("mapthingnumexist", SF_MapthingNumExist);
    new_function("objstate", SF_ObjState);
    new_function("resurrect", SF_Resurrect);
    new_function("lineattack", SF_LineAttack);
    new_function("setobjposition", SF_SetObjPosition);
    new_function("setobjproperty", SF_SetObjProperty);
    new_function("getobjproperty", SF_GetObjProperty);
    new_function("setnodenext", SF_SetNodeNext);
    new_function("setnodewait", SF_SetNodePause);
    new_function("setnodescript", SF_SetNodeScript);

    // sector stuff
    new_function("sectoreffect", SF_SectorEffect);
    new_function("floorheight", SF_FloorHeight);
    new_function("floortext", SF_FloorTexture);
    new_function("floortexture", SF_FloorTexture);      // Hurdler: due to backward and eternity compatibility
    new_function("movefloor", SF_MoveFloor);
    new_function("ceilheight", SF_CeilingHeight);
    new_function("ceilingheight", SF_CeilingHeight);    // Hurdler: due to backward and eternity compatibility
    new_function("moveceil", SF_MoveCeiling);
    new_function("moveceiling", SF_MoveCeiling);        // Hurdler: due to backward and eternity compatibility
    new_function("ceiltext", SF_CeilingTexture);
    new_function("ceilingtexture", SF_CeilingTexture);  // Hurdler: due to backward and eternity compatibility
    new_function("lightlevel", SF_LightLevel);
    new_function("fadelight", SF_FadeLight);
    new_function("colormap", SF_SectorColormap);

    // cameras!
    new_function("setcamera", SF_SetCamera);
    new_function("clearcamera", SF_ClearCamera);
    new_function("movecamera", SF_MoveCamera);

    // trig functions
    new_function("pointtoangle", SF_PointToAngle);
    new_function("pointtodist", SF_PointToDist);

    // sound functions
    new_function("startsound", SF_StartSound);
    new_function("startsectorsound", SF_StartSectorSound);
    new_function("startambiantsound", SF_AmbiantSound);
    new_function("ambientsound", SF_AmbiantSound);      // Hurdler: due to backward and eternity compatibility
    new_function("changemusic", SF_ChangeMusic);

    // hubs!
    new_function("changehublevel", SF_ChangeHubLevel);  // Hurdler: TODO: document this function

    // doors
    new_function("opendoor", SF_OpenDoor);
    new_function("closedoor", SF_CloseDoor);

    new_function("playdemo", SF_PlayDemo);
    new_function("runcommand", SF_RunCommand);
    new_function("checkcvar", SF_CheckCVar);
    new_function("setlinetexture", SF_SetLineTexture);
    new_function("linetrigger", SF_LineTrigger);
    new_function("lineflag", SF_LineFlag);

    new_function("max", SF_Max);
    new_function("min", SF_Min);
    new_function("abs", SF_Abs);

    //Hurdler: new math functions
    new_function("sin", SF_Sin);
    new_function("asin", SF_ASin);
    new_function("cos", SF_Cos);
    new_function("acos", SF_ACos);
    new_function("tan", SF_Tan);
    new_function("atan", SF_ATan);
    new_function("exp", SF_Exp);
    new_function("log", SF_Log);
    new_function("sqrt", SF_Sqrt);
    new_function("floor", SF_Floor);
    new_function("pow", SF_Pow);

    // forced coercion functions
    new_function("mobjvalue", SF_MobjValue);
    new_function("stringvalue", SF_StringValue);
    new_function("intvalue", SF_IntValue);
    new_function("fixedvalue", SF_FixedValue);

    // HU Graphics
    new_function("newhupic", SF_NewHUPic);
    new_function("createpic", SF_NewHUPic);
    new_function("deletehupic", SF_DeleteHUPic);
    new_function("modifyhupic", SF_ModifyHUPic);
    new_function("modifypic", SF_ModifyHUPic);
    new_function("sethupicdisplay", SF_SetHUPicDisplay);
    new_function("setpicvisible", SF_SetHUPicDisplay);

    // Arrays
    new_function("newarray", SF_NewArray);
    new_function("newemptyarray", SF_NewEmptyArray);
    new_function("copyinto", SF_ArrayCopyInto);
    new_function("elementat", SF_ArrayElementAt);
    new_function("setelementat", SF_ArraySetElementAt);
    new_function("length", SF_ArrayLength);

    // Hurdler's stuff :)
#ifdef HWRENDER
    new_function("setcorona", SF_SetCorona);
    new_function("setfade", SF_SetFade);
#endif
}

