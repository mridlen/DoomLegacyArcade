// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_switch.c 1728 2025-02-15 10:16:38Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: p_switch.c,v $
// Revision 1.10  2001/04/18 21:00:22  metzgermeister
// fix crash bug
//
// Revision 1.9  2001/02/24 13:35:21  bpereira
//
// Revision 1.8  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.7  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.6  2000/09/28 20:57:17  bpereira
// Revision 1.5  2000/04/16 18:38:07  bpereira
//
// Revision 1.4  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "g_game.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_main.h"
#include "w_wad.h"
  // SoM: 3/22/2000
#include "z_zone.h"
#include "t_script.h"
#include "m_swap.h"
  // LE_SWAP16

//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//
switchlist_t doom_alphSwitchList[] =
{
    // Doom shareware episode 1 switches
    {"SW1BRCOM","SW2BRCOM",     1},
    {"SW1BRN1", "SW2BRN1",      1},
    {"SW1BRN2", "SW2BRN2",      1},
    {"SW1BRNGN","SW2BRNGN",     1},
    {"SW1BROWN","SW2BROWN",     1},
    {"SW1COMM", "SW2COMM",      1},
    {"SW1COMP", "SW2COMP",      1},
    {"SW1DIRT", "SW2DIRT",      1},
    {"SW1EXIT", "SW2EXIT",      1},
    {"SW1GRAY", "SW2GRAY",      1},
    {"SW1GRAY1","SW2GRAY1",     1},
    {"SW1METAL","SW2METAL",     1},
    {"SW1PIPE", "SW2PIPE",      1},
    {"SW1SLAD", "SW2SLAD",      1},
    {"SW1STARG","SW2STARG",     1},
    {"SW1STON1","SW2STON1",     1},
    {"SW1STON2","SW2STON2",     1},
    {"SW1STONE","SW2STONE",     1},
    {"SW1STRTN","SW2STRTN",     1},

    // Doom registered episodes 2&3 switches
    {"SW1BLUE", "SW2BLUE",      2},
    {"SW1CMT",  "SW2CMT",       2},
    {"SW1GARG", "SW2GARG",      2},
    {"SW1GSTON","SW2GSTON",     2},
    {"SW1HOT",  "SW2HOT",       2},
    {"SW1LION", "SW2LION",      2},
    {"SW1SATYR","SW2SATYR",     2},
    {"SW1SKIN", "SW2SKIN",      2},
    {"SW1VINE", "SW2VINE",      2},
    {"SW1WOOD", "SW2WOOD",      2},

    // Doom II switches
    {"SW1PANEL","SW2PANEL",     3},
    {"SW1ROCK", "SW2ROCK",      3},
    {"SW1MET2", "SW2MET2",      3},
    {"SW1WDMET","SW2WDMET",     3},
    {"SW1BRIK", "SW2BRIK",      3},
    {"SW1MOD1", "SW2MOD1",      3},
    {"SW1ZIM",  "SW2ZIM",       3},
    {"SW1STON6","SW2STON6",     3},
    {"SW1TEK",  "SW2TEK",       3},
    {"SW1MARB", "SW2MARB",      3},
    {"SW1SKULL","SW2SKULL",     3},

    {"\0",      "\0",           0}
};

switchlist_t heretic_alphSwitchList[] =
{
    // heretic
    {"SW1OFF",  "SW1ON",        1},
    {"SW2OFF",  "SW2ON",        1},

    {"\0",      "\0",           0}
};

//SoM: 3/22/2000: Switch limit removal

// Increment in switches array.  May be as small as 2, or more than 40.
#define NUMSWITCHES_INC  32

static int *  switchlist = NULL;  // malloc
static int    max_numswitches = 0;  // allocated switch array
static int    numswitches = 0;  // actual number of entries in switchlist
static int    numswitch_pairs = 0;  // strangness from Doom, renamed numswitches

button_t      buttonlist[MAXBUTTONS];


//
// P_Init_SwitchList
// - this is now called at P_SetupLevel () time.
//
//SoM: 3/22/2000: Use boom code.
void P_Init_SwitchList(void)
{
  int            i, index = 0;
  int            episode; // select switches based on game
  switchlist_t   * alphSwitchList = NULL;
  switchlist_t   * switches_lump = NULL;

  //SoM: 3/22/2000: No Switches lump? Use old table!
  // DOOM, HERETIC, and HERETIC shareware do not have SWITCHES lump.
  alphSwitchList = doom_alphSwitchList;  // default
  switch (gamemode){
      case doom_registered :
      case ultdoom_retail:
          episode = 2;
          break;
      case doom2_commercial :
          episode = 3;
          break;
      case heretic :
          episode = 4;  // As was in 1.45, Blasphemer may need this as test.
#if 0
          // Like Heretic.  But it would not gain anything because
          // there are only two switches in the list and both are needed.
          if( gamedesc_id == GDESC_heretic )
            episode = 2;
          if( gamedesc_id == GDESC_heretic_shareware )
            episode = 1;
#endif
          alphSwitchList = heretic_alphSwitchList;
          break;
      default:
          episode = 1;
  }

  // Check for Boom SWITCHES lump.
  if( VALID_LUMP( W_CheckNumForName("SWITCHES") ) )
  {
    // Load the SWITCHES lump.
    switches_lump = (switchlist_t *)W_CacheLumpName("SWITCHES", PU_IN_USE);
    alphSwitchList = switches_lump;
// __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#ifdef __BIG_ENDIAN__
    // [WDJ] Endian conversion, only when BIG_ENDIAN, when from wad,
    // and not when cache hit.
    if( lump_read )
    {
      // endian conversion only when loading from extra lump
      for (i=0;alphSwitchList[i].episode!=0;i++)
        alphSwitchList[i].episode = (uint16_t)( LE_SWAP16(alphSwitchList[i].episode) );
    }
#endif
  }

  // initialization for artificial levels without switches (yes, they exist!)
  if( switchlist == NULL )
      switchlist = malloc(sizeof(*switchlist));
  
  for (i=0;alphSwitchList[i].episode!=0;i++)
  {
    if (index+1 >= max_numswitches)  // use 2 at a time
    {
      // [WDJ] Unnecessary complicated *2 size, simplified to inc size.
      // Must have 1 extra for terminating -1 entry.
      max_numswitches += NUMSWITCHES_INC;
      switchlist = realloc(switchlist, sizeof(*switchlist) * (max_numswitches+1) );
    }

    // [WDJ] 11/9/2012, remove restriction on Heretic using SWITCHES lump.
    // [WDJ] 9/1/2016, handle default list without error messages.
    // Blasphemer may have a SWITCHES lump, cannot assume contents.
    if (alphSwitchList[i].episode <= episode)
    {
      // Check, without error first.
      int tex1 = R_CheckTextureNumForName(alphSwitchList[i].name1);
      int tex2 = R_CheckTextureNumForName(alphSwitchList[i].name2);
      // default missing textures to texture 1
      if( tex1 == -1 )
      {
          tex1 = (i>1) ? switchlist[0] : 1;
          I_SoftError("Switch %i missing texture %s, using %i\n", i, alphSwitchList[i].name1, tex1 );
      }
      if( tex2 == -1 )
      {
          tex2 = (i>1) ? switchlist[1] : 1;
          I_SoftError("Switch %i missing texture %s, using %i\n", i, alphSwitchList[i].name2, tex2 );
      }
      switchlist[index++] = tex1;
      switchlist[index++] = tex2;
    }
  }

  // [WDJ] The index/2 is from Doom and Heretic.  It accomplishes nothing as
  // the only usage had it *2 again. Made it less misleading.
  numswitch_pairs = index/2;
  numswitches = numswitch_pairs * 2;
  switchlist[index] = -1;  // unnecessary, never checked

  //SoM: 3/22/2000: Don't change tag if not from lump
  if( switches_lump )
    Z_ChangeTag( switches_lump, PU_CACHE);
}


//
// Start a button counting down till it turns off.
//
static
void P_StartButton ( line_t*       line,
                     bwhere_e      w,
                     int           texture,
                     int           timer )
{
    int         i;

    // See if button is already pressed
    for (i = 0;i < MAXBUTTONS;i++)
    {
      if (buttonlist[i].btimer && buttonlist[i].line == line)
        return;
    }

    for (i = 0;i < MAXBUTTONS;i++)
    {
        if (!buttonlist[i].btimer)
        {
            buttonlist[i].line = line;
            buttonlist[i].where = w;
            buttonlist[i].btexture = texture;
            buttonlist[i].btimer = timer;
            // Bug fix: Save button sound origin as sector (xyz_t*)
            buttonlist[i].soundorg = &line->frontsector->soundorg;
            return;
        }
    }

    I_Error("P_StartButton: no button slots left!");
}





//
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//
void P_ChangeSwitchTexture ( line_t*       line,
                             int           useAgain )
{
    int     texTop, texMid, texBot;
    int     i;
    int     sound;
    side_t * swside;
    
    if ( line->sidenum[0] == NULL_INDEX )
        goto done;  // fragglescript dummy line

    swside = & sides[line->sidenum[0]];

    if (!useAgain)
        line->special = 0;

    // texture num are either 0=no-texture, or valid
    texTop = swside->toptexture;
    texMid = swside->midtexture;
    texBot = swside->bottomtexture;

    sound = sfx_swtchn;

    // EXIT SWITCH?
    if (line->special == 11)
        sound = sfx_swtchx;

    for (i = 0; i < numswitches; i++)
    {
        if (switchlist[i] == texTop)
        {
            S_StartXYZSound(buttonlist->soundorg, sound);
            swside->toptexture = switchlist[i^1];

            if (useAgain)
                P_StartButton(line, B_top_texture, switchlist[i], BUTTONTIME);

            goto done;
        }
        else
        {
            if (switchlist[i] == texMid)
            {
                S_StartXYZSound(buttonlist->soundorg, sound);
                swside->midtexture = switchlist[i^1];
                if (useAgain)
                    P_StartButton(line, B_middle_texture, switchlist[i], BUTTONTIME);

                return;
            }
            else
            {
                if (switchlist[i] == texBot)
                {
                    S_StartXYZSound(buttonlist->soundorg, sound);
                    swside->bottomtexture = switchlist[i^1];

                    if (useAgain)
                        P_StartButton(line, B_bottom_texture, switchlist[i], BUTTONTIME);

                    goto done;
                }
            }
        }
    }
done:
    return;
}


//
// P_UseSpecialLine
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
//
// For monsters, return true when is a door that actor can use ??
// (is actually when any monster operated switch).
// For players, return true when switch is activated.
boolean P_UseSpecialLine ( mobj_t*       thing,
                           line_t*       line,
                           int           side )
{

    // Err...
    // Use the back sides of VERY SPECIAL lines...
    if (side)  goto nopass;

    //SoM: 3/18/2000: Add check for Generalized linedefs.
    if (EN_boom)
    {
      // pointer to line function is NULL by default, set non-null if
      // line special is push or switch generalized linedef type
      int (*linefunc)(line_t *line)=NULL;

      // check each range of generalized linedefs
      if ((unsigned)line->special >= (GenFloorBase+0x2000))
      {} // not boom generalized
      else if ((unsigned)line->special >= GenFloorBase)
      {
        if (!thing->player)
          if ((line->special & FloorChange) || !(line->special & FloorModel))
            goto nopass; // FloorModel is "Allow Monsters" if FloorChange is 0
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag
        linefunc = EV_DoGenFloor;
      }
      else if ((unsigned)line->special >= GenCeilingBase)
      {
        if (!thing->player)
          if ((line->special & CeilingChange) || !(line->special & CeilingModel))
            goto nopass;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag
        linefunc = EV_DoGenCeiling;
      }
      else if ((unsigned)line->special >= GenDoorBase)
      {
        if (!thing->player)
        {
          if (!(line->special & DoorMonster))
            goto nopass;  // monsters disallowed from this door
          if (line->flags & ML_SECRET) // they can't open secret doors either
            goto nopass;
        }
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag
        linefunc = EV_DoGenDoor;
      }
      else if ((unsigned)line->special >= GenLockedBase)
      {
        if (!thing->player)
          goto nopass;  // monsters disallowed from unlocking doors
        if (!P_CanUnlockGenDoor(line,thing->player))
          goto nopass;
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag

        linefunc = EV_DoGenLockedDoor;
      }
      else if ((unsigned)line->special >= GenLiftBase)
      {
        if (!thing->player)
          if (!(line->special & LiftMonster))
            goto nopass;  // monsters disallowed
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag
        linefunc = EV_DoGenLift;
      }
      else if ((unsigned)line->special >= GenStairsBase)
      {
        if (!thing->player)
          if (!(line->special & StairMonster))
            goto nopass;  // monsters disallowed
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag
        linefunc = EV_DoGenStairs;
      }
      else if ((unsigned)line->special >= GenCrusherBase)
      {
        if (!thing->player)
          if (!(line->special & CrusherMonster))
            goto nopass;  // monsters disallowed
        if (!line->tag && ((line->special&6)!=6)) //all non-manual
          goto nopass;  //generalized types require tag
        linefunc = EV_DoGenCrusher;
      }

      if (linefunc)
      {
        switch((line->special & TriggerType) >> TriggerTypeShift)
        {
          case TRIG_PushOnce:
            if (!side)
              if (linefunc(line))
                line->special = 0;
            goto pass;
          case TRIG_PushMany:
            if (!side)
              linefunc(line);
            goto pass;
          case TRIG_SwitchOnce:
            if (linefunc(line))
              P_ChangeSwitchTexture(line,0);
            goto pass;
          case TRIG_SwitchMany:
            if (linefunc(line))
              P_ChangeSwitchTexture(line,1);
            goto pass;
          default:  // if not a switch/push type, do nothing here
            goto nopass;
        }
      }
    }



    // Switches that other things can activate.
    if (!thing->player)
    {
        // monsters never open secret doors
        if (line->flags & ML_SECRET)
            goto nopass;

        // the only doors and switches that monsters can activate
        switch(line->special)
        {
          case 1:       // MANUAL DOOR RAISE
          case 32:      // MANUAL BLUE
          case 33:      // MANUAL RED
          case 34:      // MANUAL YELLOW
          //SoM: 3/18/2000: add ability to use teleporters for monsters
          case 195:       // switch teleporters
          case 174:
          case 210:       // silent switch teleporters
          case 209:
            break;

          default:
            goto nopass;  // monsters exit
        }
    }
    // monsters past this point have been checked OK for using the switch

    if (EN_boom && !P_CheckTag(line))  //disallow zero tag on some types
      goto nopass;

    // do something
    switch (line->special)
    {
        // MANUALS
      case 1:           // Vertical Door
      case 26:          // Blue Door/Locked
      case 27:          // Yellow Door /Locked
      case 28:          // Red Door /Locked

      case 31:          // Manual door open
      case 32:          // Blue locked door open
      case 33:          // Red locked door open
      case 34:          // Yellow locked door open

      case 117:         // Blazing door raise
      case 118:         // Blazing door open
        EV_VerticalDoor (line, thing);
        break;

        //UNUSED - Door Slide Open&Close
        // case 124:
        // EV_SlidingDoor (line, thing);
        // break;

        // SWITCHES
      case 7:
        // Build Stairs
        if (EV_BuildStairs( line, (EN_heretic)? 8*FRACUNIT : ST_build8))
            goto switch_1_clear;
        break;

      case 107:
        if( EN_heretic )
        {
            if (EV_BuildStairs (line, 16 * FRACUNIT))
                goto switch_1_clear;
        }
        break;

      case 9:
        // Change Donut
        if (EV_DoDonut(line))
            goto switch_1_clear;
        break;

      case 11:
        // Exit level
        if(cv_allowexitlevel.EV)
        {
            P_ChangeSwitchTexture(line,0);
            G_ExitLevel ();
        }
        break;

      case 14:
        // Raise Floor 32 and change texture
        if (EV_DoPlat( line, PLATT_raiseAndChange, 32))
            goto switch_1_clear;
        break;

      case 15:
        // Raise Floor 24 and change texture
        if (EV_DoPlat( line, PLATT_raiseAndChange, 24))
            goto switch_1_clear;
        break;

      case 18:
        // Raise Floor to next highest floor
        if (EV_DoFloor( line, FT_raiseFloorToNearest))
            goto switch_1_clear;
        break;

      case 20:
        // Raise Plat next highest floor and change texture
        if (EV_DoPlat( line, PLATT_raiseToNearestAndChange, 0))
            goto switch_1_clear;
        break;

      case 21:
        // PlatDownWaitUpStay
        if (EV_DoPlat( line, PLATT_downWaitUpStay, 0))
            goto switch_1_clear;
        break;

      case 23:
        // Lower Floor to Lowest
        if (EV_DoFloor( line, FT_lowerFloorToLowest))
            goto switch_1_clear;
        break;

      case 29:
        // Raise Door
        if (EV_DoDoor( line, VD_normalDoor, VDOORSPEED))
            goto switch_1_clear;
        break;

      case 41:
        // Lower Ceiling to Floor
        if (EV_DoCeiling( line, CT_lowerToFloor))
            goto switch_1_clear;
        break;

      case 71:
        // Turbo Lower Floor
        if (EV_DoFloor( line, FT_turboLower))
            goto switch_1_clear;
        break;

      case 49:
        // Ceiling Crush And Raise
        if (EV_DoCeiling( line, (EN_heretic)? CT_lowerAndCrush : CT_crushAndRaise))
            goto switch_1_clear;
        break;

      case 50:
        // Close Door
        if (EV_DoDoor( line, VD_doorclose, VDOORSPEED))
            goto switch_1_clear;
        break;

      case 51:
        // Secret EXIT
        P_ChangeSwitchTexture(line,0);
        G_SecretExitLevel ();
        break;

      case 55:
        // Raise Floor Crush
        if (EV_DoFloor( line, FT_raiseFloorCrush))
            goto switch_1_clear;
        break;

      case 101:
        // Raise Floor
        if (EV_DoFloor( line, FT_raiseFloor))
            goto switch_1_clear;
        break;

      case 102:
        // Lower Floor to Surrounding floor height
        if (EV_DoFloor( line, FT_lowerFloor))
            goto switch_1_clear;
        break;

      case 103:
        // Open Door
        if (EV_DoDoor( line, VD_dooropen, VDOORSPEED))
            goto switch_1_clear;
        break;

      case 111:
        // Blazing Door Raise (faster than TURBO!)
        if (EV_DoDoor( line, VD_blazeRaise, 4*VDOORSPEED))
            goto switch_1_clear;
        break;

      case 112:
        // Blazing Door Open (faster than TURBO!)
        if (EV_DoDoor( line, VD_blazeOpen, 4*VDOORSPEED))
            goto switch_1_clear;
        break;

      case 113:
        // Blazing Door Close (faster than TURBO!)
        if (EV_DoDoor( line, VD_blazeClose, 4*VDOORSPEED))
            goto switch_1_clear;
        break;

      case 122:
        // Blazing PlatDownWaitUpStay
        if (EV_DoPlat( line, PLATT_blazeDWUS, 0))
            goto switch_1_clear;
        break;

      case 127:
        // Build Stairs Turbo 16
        if (EV_BuildStairs( line, ST_turbo16))
            goto switch_1_clear;
        break;

      case 131:
        // Raise Floor Turbo
        if (EV_DoFloor( line, FT_raiseFloorTurbo))
            goto switch_1_clear;
        break;

      case 133:
        // BlzOpenDoor BLUE
      case 135:
        // BlzOpenDoor RED
      case 137:
        // BlzOpenDoor YELLOW
        if (EV_DoLockedDoor( line, VD_blazeOpen, thing, 4*VDOORSPEED))
            goto switch_1_clear;
        break;

      case 140:
        // Raise Floor 512
        if (EV_DoFloor( line, FT_raiseFloor512))
            goto switch_1_clear;
        break;

      //SoM: FraggleScript!
      case 276:
      case 277:
        T_RunScript(line->tag, thing);
        if(line->special == 277)
        {
          line->special = 0;         // clear tag
          P_ChangeSwitchTexture(line,0);
        }
        else
          P_ChangeSwitchTexture(line,1);
        break;

      default:
        if (EN_boom)
        {
          switch (line->special)
          {
            // added linedef types to fill all functions out so that
            // all possess SR, S1, WR, W1 types

            case 158:
              // Raise Floor to shortest lower texture
              if (EV_DoFloor( line, FT_raiseToTexture))
                goto switch_1_clear;
              break;
  
            case 159:
              // Raise Floor to shortest lower texture
              if (EV_DoFloor( line, FT_lowerAndChange))
                goto switch_1_clear;
              break;
        
            case 160:
              // Raise Floor 24 and change
              if (EV_DoFloor( line, FT_raiseFloor24AndChange))
                goto switch_1_clear;
              break;

            case 161:
              // Raise Floor 24
              if (EV_DoFloor( line, FT_raiseFloor24))
                goto switch_1_clear;
              break;

            case 162:
              // Moving floor min n to max n
              if (EV_DoPlat( line, PLATT_perpetualRaise, 0))
                goto switch_1_clear;
              break;

            case 163:
              // Stop Moving floor
              EV_StopPlat(line);
              goto switch_1_clear;

            case 164:
              // Start fast crusher
              if (EV_DoCeiling( line, CT_fastCrushAndRaise))
                goto switch_1_clear;
              break;

            case 165:
              // Start slow silent crusher
              if (EV_DoCeiling( line, CT_silentCrushAndRaise))
                goto switch_1_clear;
              break;

            case 166:
              // Raise ceiling, Lower floor
              if (EV_DoCeiling( line, CT_raiseToHighest) ||
                  EV_DoFloor( line, FT_lowerFloorToLowest))
                goto switch_1_clear;
              break;

            case 167:
              // Lower floor and Crush
              if (EV_DoCeiling( line, CT_lowerAndCrush))
                goto switch_1_clear;
              break;

            case 168:
              // Stop crusher
              if (EV_CeilingCrushStop(line))
                goto switch_1_clear;
              break;

            case 169:
              // Lights to brightest neighbor sector
              EV_LightTurnOn(line,0);
              goto switch_1_clear;

            case 170:
              // Lights to near dark
              EV_LightTurnOn(line,35);
              goto switch_1_clear;

            case 171:
              // Lights on full
              EV_LightTurnOn(line,255);
              goto switch_1_clear;

            case 172:
              // Start Lights Strobing
              EV_StartLightStrobing(line);
              goto switch_1_clear;

            case 173:
              // Lights to Dimmest Near
              EV_TurnTagLightsOff(line);
              goto switch_1_clear;

            case 174:
              // Teleport
              if (EV_Teleport(line,side,thing))
                goto switch_1_clear;
              break;

            case 175:
              // Close Door, Open in 30 secs
              if (EV_DoDoor( line, VD_close30ThenOpen, VDOORSPEED))
                goto switch_1_clear;
              break;

            case 189: //create texture change no motion type
              // Texture Change Only (Trigger)
              if (EV_DoChange( line, CH_MODEL_trig_only))
                goto switch_1_clear;
              break;

            case 203:
              // Lower ceiling to lowest surrounding ceiling
              if (EV_DoCeiling( line, CT_lowerToLowest))
                goto switch_1_clear;
              break;

            case 204:
              // Lower ceiling to highest surrounding floor
              if (EV_DoCeiling( line, CT_lowerToMaxFloor))
                goto switch_1_clear;
              break;

            case 209:
              // killough 1/31/98: silent teleporter
              if (EV_SilentTeleport(line, side, thing))
                goto switch_1_clear;
              break;

            case 241: //jff 3/15/98 create texture change no motion type
              // Texture Change Only (Numeric)
              if (EV_DoChange( line, CH_MODEL_num_only))
                goto switch_1_clear;
              break;

            case 221:
              // Lower floor to next lowest floor
              if (EV_DoFloor( line, FT_lowerFloorToNearest))
                goto switch_1_clear;
              break;

            case 229:
              // Raise elevator next floor
              if (EV_DoElevator( line, ET_elevateUp))
                goto switch_1_clear;
              break;

            case 233:
              // Lower elevator next floor
              if (EV_DoElevator( line, ET_elevateDown))
                goto switch_1_clear;
              break;

            case 237:
              // Elevator to current floor
              if (EV_DoElevator( line, ET_elevateCurrent))
                goto switch_1_clear;
              break;


            //end of added S1 linedef types

            //added linedef types to fill all functions out so that
            //all possess SR, S1, WR, W1 types
            
            case 78:
              // Texture/type Change Only (Numeric)
              if (EV_DoChange( line, CH_MODEL_num_only))
                goto switch_R;
              break;

            case 176:
              // Raise Floor to shortest lower texture
              if (EV_DoFloor( line, FT_raiseToTexture))
                goto switch_R;
              break;

            case 177:
              // Raise Floor to shortest lower texture
              if (EV_DoFloor( line, FT_lowerAndChange))
                goto switch_R;
              break;

            case 178:
              // Raise Floor 512
              if (EV_DoFloor( line, FT_raiseFloor512))
                goto switch_R;
              break;

            case 179:
              // Raise Floor 24 and change
              if (EV_DoFloor( line, FT_raiseFloor24AndChange))
                goto switch_R;
              break;

            case 180:
              // Raise Floor 24
              if (EV_DoFloor( line, FT_raiseFloor24))
                goto switch_R;
              break;

            case 181:
              // Moving floor min n to max n
              EV_DoPlat( line, PLATT_perpetualRaise, 0);
              goto switch_R;
              break;

            case 182:
              // Stop Moving floor
              EV_StopPlat(line);
              goto switch_R;
              break;

            case 183:
              // Start fast crusher
              if (EV_DoCeiling( line, CT_fastCrushAndRaise))
                goto switch_R;
              break;

            case 184:
              // Start slow crusher
              if (EV_DoCeiling( line, CT_crushAndRaise))
                goto switch_R;
              break;

            case 185:
              // Start slow silent crusher
              if (EV_DoCeiling( line, CT_silentCrushAndRaise))
                goto switch_R;
              break;

            case 186:
              // Raise ceiling, Lower floor
              if (EV_DoCeiling( line, CT_raiseToHighest) ||
                  EV_DoFloor( line, FT_lowerFloorToLowest))
                goto switch_R;
              break;

            case 187:
              // Lower floor and Crush
              if (EV_DoCeiling( line, CT_lowerAndCrush))
                goto switch_R;
              break;

            case 188:
              // Stop crusher
              if (EV_CeilingCrushStop(line))
                goto switch_R;
              break;

            case 190: //jff 3/15/98 create texture change no motion type
              // Texture Change Only (Trigger)
              if (EV_DoChange( line, CH_MODEL_trig_only))
                goto switch_R;
              break;

            case 191:
              // Lower Pillar, Raise Donut
              if (EV_DoDonut(line))
                goto switch_R;
              break;

            case 192:
              // Lights to brightest neighbor sector
              EV_LightTurnOn(line,0);
              goto switch_R;
              break;

            case 193:
              // Start Lights Strobing
              EV_StartLightStrobing(line);
              goto switch_R;
              break;

            case 194:
              // Lights to Dimmest Near
              EV_TurnTagLightsOff(line);
              goto switch_R;
              break;

            case 195:
              // Teleport
              if (EV_Teleport(line,side,thing))
                goto switch_R;
              break;

            case 196:
              // Close Door, Open in 30 secs
              if (EV_DoDoor( line, VD_close30ThenOpen, VDOORSPEED))
                goto switch_R;
              break;

            case 205:
              // Lower ceiling to lowest surrounding ceiling
              if (EV_DoCeiling( line, CT_lowerToLowest))
                goto switch_R;
              break;

            case 206:
              // Lower ceiling to highest surrounding floor
              if (EV_DoCeiling( line, CT_lowerToMaxFloor))
                goto switch_R;
              break;

            case 210:
              // Silent teleporter
              if (EV_SilentTeleport(line, side, thing))
                goto switch_R;
              break;

            case 211:
              // Toggle Floor Between C and F Instantly
              if (EV_DoPlat( line, PLATT_toggleUpDn, 0))
                goto switch_R;
              break;

            case 222:
              // Lower floor to next lowest floor
              if (EV_DoFloor( line, FT_lowerFloorToNearest))
                goto switch_R;
              break;

            case 230:
              // Raise elevator next floor
              if (EV_DoElevator( line, ET_elevateUp))
                goto switch_R;
              break;

            case 234:
              // Lower elevator next floor
              if (EV_DoElevator( line, ET_elevateDown))
                goto switch_R;
              break;

            case 238:
              // Elevator to current floor
              if (EV_DoElevator( line, ET_elevateCurrent))
                goto switch_R;
              break;

            case 258:
              // Build stairs, step 8
              if (EV_BuildStairs( line, ST_build8))
                goto switch_R;
              break;

            case 259:
              // Build stairs, step 16
              if (EV_BuildStairs( line, ST_turbo16))
                goto switch_R;
              break;

            // end of added SR linedef types

          }
        }
        break;


        // BUTTONS
      case 42:
        // Close Door
        if (EV_DoDoor( line, VD_doorclose, VDOORSPEED))
            goto switch_R;
        break;

      case 43:
        // Lower Ceiling to Floor
        if (EV_DoCeiling( line, CT_lowerToFloor))
            goto switch_R;
        break;

      case 45:
        // Lower Floor to Surrounding floor height
        if (EV_DoFloor( line, FT_lowerFloor))
            goto switch_R;
        break;

      case 60:
        // Lower Floor to Lowest
        if (EV_DoFloor( line, FT_lowerFloorToLowest))
            goto switch_R;
        break;

      case 61:
        // Open Door
        if (EV_DoDoor( line, VD_dooropen, VDOORSPEED))
            goto switch_R;
        break;

      case 62:
        // PlatDownWaitUpStay
        if (EV_DoPlat( line, PLATT_downWaitUpStay, 1))
            goto switch_R;
        break;

      case 63:
        // Raise Door
        if (EV_DoDoor( line, VD_normalDoor, VDOORSPEED))
            goto switch_R;
        break;

      case 64:
        // Raise Floor to ceiling
        if (EV_DoFloor( line, FT_raiseFloor))
            goto switch_R;
        break;

      case 66:
        // Raise Floor 24 and change texture
        if (EV_DoPlat( line, PLATT_raiseAndChange, 24))
            goto switch_R;
        break;

      case 67:
        // Raise Floor 32 and change texture
        if (EV_DoPlat( line, PLATT_raiseAndChange, 32))
            goto switch_R;
        break;

      case 65:
        // Raise Floor Crush
        if (EV_DoFloor( line, FT_raiseFloorCrush))
            goto switch_R;
        break;

      case 68:
        // Raise Plat to next highest floor and change texture
        if (EV_DoPlat( line, PLATT_raiseToNearestAndChange, 0))
            goto switch_R;
        break;

      case 69:
        // Raise Floor to next highest floor
        if (EV_DoFloor( line, FT_raiseFloorToNearest))
            goto switch_R;
        break;

      case 70:
        // Turbo Lower Floor
        if (EV_DoFloor( line, FT_turboLower))
            goto switch_R;
        break;

      case 114:
        // Blazing Door Raise (faster than TURBO!)
        if (EV_DoDoor( line, VD_blazeRaise, 4*VDOORSPEED))
            goto switch_R;
        break;

      case 115:
        // Blazing Door Open (faster than TURBO!)
        if (EV_DoDoor( line, VD_blazeOpen, 4*VDOORSPEED))
            goto switch_R;
        break;

      case 116:
        // Blazing Door Close (faster than TURBO!)
        if (EV_DoDoor( line, VD_blazeClose, 4*VDOORSPEED))
            goto switch_R;
        break;

      case 123:
        // Blazing PlatDownWaitUpStay
        if (EV_DoPlat( line, PLATT_blazeDWUS, 0))
            goto switch_R;
        break;

      case 132:
        // Raise Floor Turbo
        if (EV_DoFloor( line, FT_raiseFloorTurbo))
            goto switch_R;
        break;

      case 99:
        if( EN_heretic ) // used for right scrolling texture
            break;
        // BlzOpenDoor BLUE
      case 134:
        // BlzOpenDoor RED
      case 136:
        // BlzOpenDoor YELLOW
        if (EV_DoLockedDoor( line, VD_blazeOpen, thing, 4*VDOORSPEED))
            goto switch_R;
        break;

      case 138:
        // Light Turn On
        EV_LightTurnOn(line,255);
        goto switch_R;

      case 139:
        // Light Turn Off
        EV_LightTurnOn(line,35);
        goto switch_R;

    }
    return true;
   
switch_1_clear:
    // Clear the special.
    P_ChangeSwitchTexture(line, 0);
    goto pass;
    
   
switch_R:
    // Allow retrigger.
    P_ChangeSwitchTexture(line, 1);

pass:
    // Opened the door, or operated the switch.
    return true;

nopass:
    // Blocked due to monster, or wrong side, or illegal tag.
    return false;
}
