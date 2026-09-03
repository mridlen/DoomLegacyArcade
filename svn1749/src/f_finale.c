// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: f_finale.c 1685 2024-06-20 09:45:15Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: f_finale.c,v $
// Revision 1.13  2004/09/12 19:40:05  darkwolf95
// additional chex quest 1 support
//
// Revision 1.12  2001/06/10 21:16:01  bpereira
// Revision 1.11  2001/05/27 13:42:47  bpereira
// Revision 1.10  2001/05/16 21:21:14  bpereira
// Revision 1.9  2001/04/01 17:35:06  bpereira
//
// Revision 1.8  2001/03/21 18:24:38  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.7  2001/03/03 11:11:49  hurdler
// Revision 1.6  2001/02/24 13:35:19  bpereira
// Revision 1.5  2001/02/10 13:05:45  hurdler
//
// Revision 1.4  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.3  2000/08/03 17:57:41  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "doomstat.h"
#include "am_map.h"
#include "dstrings.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_info.h"
#include "p_chex.h"
  // Chex_safe_pictures

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast,  3 = title
int             finalestage;

int             finalecount;

#define TEXTSPEED       3
#define TEXTWAIT        250


#ifdef ENABLE_UMAPINFO
// [WDJ] For compatibility with umapinfo?  But, does not look anything like CONST.
const char*   finaletext;  // [MB] 2023-01-29: Changed to const
const char*   finaleflat;  // [MB] 2023-01-29: Changed to const
#else
char*   finaletext;
char*   finaleflat;
#endif
static boolean keypressed=false;
static byte    finale_palette = 0;  // [WDJ] 0 is PLAYPAL

static void    F_StartCast (void);
static void    F_CastTicker (void);
static boolean F_CastResponder (event_t *ev);
static void    F_CastDrawer (void);
static void    F_Draw_interpic_Name( const char * name );

//
// F_StartFinale
//

#ifdef ENABLE_UMAPINFO
void F_StartFinale (boolean secretexit)
#else
void F_StartFinale (void)
#endif
{
    gamestate = GS_FINALE;

    // [MB] 2023-01-29: Provide defaults for flat and text
    finaleflat = text[BGFLATE1_NUM];  // Doom E1, FLOOR4_8
    finaletext = "";  // Skip text screen

#ifdef ENABLE_UMAPINFO
    // [MB] 2023-01-29: Support for UMAPINFO added
    if(game_umapinfo)  // have umapinfo
    {
        if(!secretexit && game_umapinfo->intertext)
        {
            // An empty string is set for 'clear' identifier
            finaletext = game_umapinfo->intertext;
            goto beyond_std_setup;
        }
        else if(secretexit && game_umapinfo->intertextsecret)
        {
            // An empty string is set for 'clear' identifier
            finaletext = game_umapinfo->intertextsecret;
            goto beyond_std_setup;
        }
    }
#endif

    if(info_intertext)
    {
        //SoM: Use FS level info intermission.
        finaletext = info_intertext;
        if(info_backdrop)
            finaleflat = info_backdrop;
        // else finaleflat = text[BGFLATE1_NUM];  // Doom E1, FLOOR4_8
        goto beyond_std_setup;
    }

    // Okay - IWAD dependent stuff.
    // This has been changed severly, and
    //  some stuff might have changed in the process.
    switch ( gamemode )
    {

      // DOOM 1 - E1, E3 or E4, but each nine missions
      case doom_shareware:
      case doom_registered:
      case ultdoom_retail:
      {
        S_ChangeMusic(mus_victor, true);

        switch (gameepisode)
        {
          case 1:
            finaleflat = text[BGFLATE1_NUM]; // Doom E1, FLOOR4_8
            finaletext = text[E1TEXT_NUM];
            break;
          case 2:
            finaleflat = text[BGFLATE2_NUM]; // Doom E2, SFLR6_1
            finaletext = text[E2TEXT_NUM];
            break;
          case 3:
            finaleflat = text[BGFLATE3_NUM]; // Doom E3, MFLR8_4
            finaletext = text[E3TEXT_NUM];
            break;
          case 4:
            finaleflat = text[BGFLATE4_NUM]; // Doom E3, MFLR8_3
            finaletext = text[E4TEXT_NUM];
            break;
          default:
#ifdef ENABLE_UMAPINFO
            // With UMAPINFO game can end after arbitrary level.
            goto beyond_std_setup;
#else
            // Ouch.
            // break;
            goto ouch_setup;  // [MB] Use defaults
#endif
        }
        break;
      }

      // DOOM II and missions packs with E1, M34
      case doom2_commercial:
      {
          // Doom2, Plutonia, TNT
          // These text are always sequential in text[].
          int textnum = 0;

          S_ChangeMusic(mus_read_m, true);

          switch (gamemap)
          {
            case 6:
              finaleflat = text[BGFLAT06_NUM]; // Doom2 MAP06, SLIME16
              textnum = 0; // text[C1TEXT_NUM];
              break;
            case 11:
              finaleflat = text[BGFLAT11_NUM]; // Doom2 MAP11, RROCK14
              textnum = 1; // text[C2TEXT_NUM];
              break;
            case 20:
              finaleflat = text[BGFLAT20_NUM]; // Doom2 MAP20, RROCK07
              textnum = 2; // text[C3TEXT_NUM];
              break;
            case 30:
              finaleflat = text[BGFLAT30_NUM]; // Doom2 MAP30, RROCK17
              textnum = 3; // text[C4TEXT_NUM];
              break;
            case 15:
              finaleflat = text[BGFLAT15_NUM]; // Doom2 MAP15, RROCK13
              textnum = 4; // text[C5TEXT_NUM];
              break;
            case 31:
              finaleflat = text[BGFLAT31_NUM]; // Doom2 MAP31, RROCK19
              textnum = 5; // text[C6TEXT_NUM];
              break;
            default:
#ifdef ENABLE_UMAPINFO
              // [MB] With UMAPINFO game can end after arbitrary level.
              //
              goto beyond_std_setup;
#else
              // Ouch.
              // break;
              goto ouch_setup;  // something better than break
#endif
          }
          switch( gamedesc_id )
          {
           case GDESC_plutonia:
             // P1TEXT, P2TEXT, P3TEXT, P4TEXT, P5TEXT, P6TEXT
             textnum += P1TEXT_NUM;
             break;
           case GDESC_tnt:
             // T1TEXT, T2TEXT, T3TEXT, T4TEXT, T5TEXT, T6TEXT
             textnum += T1TEXT_NUM;
             break;
           default:
             // C1TEXT, C2TEXT, C3TEXT, C4TEXT, C5TEXT, C6TEXT
             textnum += C1TEXT_NUM;
             break;
          }
          finaletext = text[textnum];
#if 0
// [WDJ] Might as well put this outside the case, where it is easier to understand where it eventually goes.
use_defaults:
#endif
          break;
      }

      case heretic :
       {
          S_ChangeMusic(mus_hcptd, true);
          switch(gameepisode)
          {
              case 1:
                  finaleflat = "FLOOR25";
                  finaletext = text[HERETIC_E1TEXT];
                  break;
              case 2:
                  finaleflat = "FLATHUH1";
                  finaletext = text[HERETIC_E2TEXT];
                  break;
              case 3:
                  finaleflat = "FLTWAWA2";
                  finaletext = text[HERETIC_E3TEXT];
                  break;
              case 4:
                  finaleflat = "FLOOR28";
                  finaletext = text[HERETIC_E4TEXT];
                  break;
              case 5:
                  finaleflat = "FLOOR08";
                  finaletext = text[HERETIC_E5TEXT];
                  break;
              default:
                  goto ouch_setup;  // something better than break
          }
          break;
        }

      case chexquest1: //DarkWolf95: Support for Chex Quest
        {
          S_ChangeMusic(mus_victor, true);
          finaleflat = "FLOOR0_6";
          finaletext = E1TEXT;
          break;
        }

      // Indeterminate.
      default:
        S_ChangeMusic(mus_read_m, true);
        finaleflat = "F_SKY1"; // Not used anywhere else.
        finaletext = C1TEXT;  // FIXME - other text, music?
        break;
    }

    // [WDJ] Collector for detection errors, if ever needed.
ouch_setup:
    // does nothing different for now.
   
beyond_std_setup:

#ifdef ENABLE_UMAPINFO
    // [MB] 2023-01-29: Skip text screen for empty string
    if (finaletext[0] == 0)
    {
        // Fake state when text screen terminates
        // Use logic in F_Ticker() to decide what to do next
        // FIXME: Screen wipe does not work correctly
        finalestage = 0;
        finalecount = INT_MAX - 1;
// [WDJ] ??? fake keypress
        keypressed  = true;
        return;
    }

    // [MB] 2023-01-29: Support for UMAPINFO added
    if (game_umapinfo && game_umapinfo->interbackdrop)
        finaleflat = game_umapinfo->interbackdrop;

    if (game_umapinfo && game_umapinfo->intermusic)
    {
        const char * mus_umi = UMI_Get_MusicLumpName(game_umapinfo->intermusic);

        S_ChangeMusicName(mus_umi, true);
    }
#endif

    // all return
    finalestage = 0;
    finalecount = 0;
}



boolean F_Responder (event_t *event)
{
    if (event->type != ev_keydown)
        goto prop_event;

    switch (finalestage)
    {
      case 0:  // text
        if( ! keypressed )
            goto catch_event;
        break;
      case 1:  // art
        // test for any palette that is not PLAYPAL (Heretic underwater)
        if( finale_palette )
        {
           V_SetPaletteLump("PLAYPAL");  // clear finale_palette
           finale_palette = 0;
           V_DrawFill(0, 0, 320, 200, 0);  // clear pic to black
        }
        if( EN_heretic )
        {
           // no support to draw heretic cast
           finalestage = 3;  // goto title
           goto catch_event;
        }
        break;
      case 2:  // cast
        return F_CastResponder (event);
    }
prop_event:
    return false;  // to other handlers

catch_event:
    keypressed = true;
    return true;  // handled event
}


//
// F_Ticker
//
void F_Ticker (void)
{
    // advance animation
    finalecount++;

    switch (finalestage) {
        case 0 :
            // check for skipping
            if( keypressed )
            {
                keypressed = false;
                if( (unsigned)finalecount < strlen (finaletext)*TEXTSPEED )
                {
                    // force text to be write 
                    finalecount += INT_MAX/2;
                }
                else
                {
                    if( gamemode == doom2_commercial )
                    {
#ifdef ENABLE_UMAPINFO
                        // [MB] 2023-03-26: Support for UMAPINFO added
                        byte umap_flags = (game_umapinfo)? game_umapinfo->flags : 0;

                        if (umap_flags & (UMA_endgame_enabled | UMA_endcast)) // endgame enabled or endcast
                            F_StartCast ();
                        else if (gamemap == 30 && ! (umap_flags & UMA_endgame_disabled))
                            F_StartCast ();  // standard endgame after last map
#else
                        if (gamemap == 30)
                            F_StartCast ();
#endif
                        else
                        {
                            gameaction = ga_worlddone;
                            finalecount = INT_MIN;    // wait until map is launched
                        }
                    }
                    else
                        // next animation state (just above)
                        finalecount = INT_MAX;
                }
            }

            if( gamemode != doom2_commercial )
            {
                uint32_t  f = finalecount;
                if( f >= INT_MAX/2 )
                    f -= INT_MAX/2;

                if( f > strlen (finaletext)*TEXTSPEED + TEXTWAIT )
                {
                    finalecount = 0;
                    finalestage = 1;
                    wipegamestate = GS_FORCEWIPE;             // force a wipe
#ifdef ENABLE_UMAPINFO
                    if( (EN_doom_etc && gameepisode == 3)
                         || (game_umapinfo && (game_umapinfo->flags & UMA_endbunny)) )
#else
                    if( EN_doom_etc && gameepisode == 3 )
#endif
                        S_StartMusic (mus_bunny);
                }
            }
            break;
        case 2 : 
            F_CastTicker ();
            break;
        default:
            break;
    }
}



//
// F_TextWrite
//
// Called by F_Drawer
static
void F_TextWrite (void)
{
    // vid : from video setup
    int         w;
    int         count;
#ifdef ENABLE_UMAPINFO
    const char* ch;  // [MB] 2023-01-29: Changed to const
#else
    char*       ch;
#endif
    int         c;
    int         cx, cy;

    // Draw to screen0, scaled

    // erase the entire screen0 to a tiled background
    V_ScreenFlatFill( W_GetNumForName(finaleflat) );

#ifdef DIRTY_RECT
    V_MarkRect (0, 0, vid.width, vid.height);
#endif

    // draw some of the text onto the screen
    if( EN_heretic_hexen )
    {
        cx = 20;
        cy = 5;
    }
    else
    {
        cx = 10;
        cy = 10;
    }
    ch = finaletext;

    count = (finalecount - 10)/TEXTSPEED;
    if (count < 0)
        count = 0;
    for ( ; count ; count-- )
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = EN_heretic_hexen ? 20 : 10;
            cy += EN_heretic_hexen ? 9 : 11;
            continue;
        }

        // hufont only has uppercase
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        // hu_font is endian fixed
        w = V_patch( hu_font[c] )->width;
        if (cx+w > vid.width)
            break;
        V_DrawScaledPatch(cx, cy, hu_font[c]);
        cx+=w;
    }

}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
    char       *name;
    mobjtype_t  type;
} castinfo_t;

castinfo_t      castorder[] = {
    {NULL, MT_POSSESSED},
    {NULL, MT_SHOTGUY},
    {NULL, MT_CHAINGUY},
    {NULL, MT_TROOP},
    {NULL, MT_SERGEANT},
    {NULL, MT_SKULL},
    {NULL, MT_HEAD},
    {NULL, MT_KNIGHT},
    {NULL, MT_BRUISER},
    {NULL, MT_BABY},
    {NULL, MT_PAIN},
    {NULL, MT_UNDEAD},
    {NULL, MT_FATSO},
    {NULL, MT_VILE},
    {NULL, MT_SPIDER},
    {NULL, MT_CYBORG},
    {NULL, MT_PLAYER},

    {NULL,0}
};

byte            castnum;  // index castorder
byte            castframes; // 0..24 frames
int             casttics;
state_t*        caststate;
boolean         castdeath;
int             castonmelee;
boolean         castattacking;


//
// F_StartCast
//
static
void F_StartCast (void)
{
    int i;

    for(i=0;i<17;i++)
       castorder[i].name = text[CC_ZOMBIE_NUM+i];

    wipegamestate = -1;         // force a screen wipe
    castnum = 0;
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    casttics = caststate->tics;
    castdeath = false;
    finalestage = 2;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
    S_ChangeMusic(mus_evil, true);
}


//
// F_CastTicker
//
static
void F_CastTicker (void)
{
    int         st;
    int         sfx;

    if (--casttics > 0)
        return;                 // not time to change state yet

    if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
        // switch from deathstate to next monster
        castnum++;
        castdeath = false;
        if (castorder[castnum].name == NULL)
            castnum = 0;
        if (mobjinfo[castorder[castnum].type].seesound)
            S_StartSound(mobjinfo[castorder[castnum].type].seesound);
        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        castframes = 0;
    }
    else
    {
        // just advance to next state in animation
        if (caststate == &states[S_PLAY_ATK1])
            goto stopattack;    // Oh, gross hack!
        st = caststate->nextstate;
        caststate = &states[st];
        castframes++;

        // sound hacks....
        switch (st)
        {
          case S_PLAY_ATK1:     sfx = sfx_dshtgn; break;
          case S_POSS_ATK2:     sfx = sfx_pistol; break;
          case S_SPOS_ATK2:     sfx = sfx_shotgn; break;
          case S_VILE_ATK2:     sfx = sfx_vilatk; break;
          case S_SKEL_FIST2:    sfx = sfx_skeswg; break;
          case S_SKEL_FIST4:    sfx = sfx_skepch; break;
          case S_SKEL_MISS2:    sfx = sfx_skeatk; break;
          case S_FATT_ATK8:
          case S_FATT_ATK5:
          case S_FATT_ATK2:     sfx = sfx_firsht; break;
          case S_CPOS_ATK2:
          case S_CPOS_ATK3:
          case S_CPOS_ATK4:     sfx = sfx_shotgn; break;
          case S_TROO_ATK3:     sfx = sfx_claw; break;
          case S_SARG_ATK2:     sfx = sfx_sgtatk; break;
          case S_BOSS_ATK2:
          case S_BOS2_ATK2:
          case S_HEAD_ATK2:     sfx = sfx_firsht; break;
          case S_SKULL_ATK2:    sfx = sfx_sklatk; break;
          case S_SPID_ATK2:
          case S_SPID_ATK3:     sfx = sfx_shotgn; break;
          case S_BSPI_ATK2:     sfx = sfx_plasma; break;
          case S_CYBER_ATK2:
          case S_CYBER_ATK4:
          case S_CYBER_ATK6:    sfx = sfx_rlaunc; break;
          case S_PAIN_ATK3:     sfx = sfx_sklatk; break;
          default: sfx = 0; break;
        }

        if (sfx)
            S_StartSound(sfx);
    }

    if (castframes == 12)
    {
        // go into attack frame
        castattacking = true;
        if (castonmelee)
            caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
        else
            caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
        castonmelee ^= 1;
        if (caststate == &states[S_NULL])
        {
            if (castonmelee)
                caststate=
                    &states[mobjinfo[castorder[castnum].type].meleestate];
            else
                caststate=
                    &states[mobjinfo[castorder[castnum].type].missilestate];
        }
    }

    if (castattacking)
    {
        if (castframes == 24
            ||  caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
        {
          stopattack:
            castattacking = false;
            castframes = 0;
            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
    }

    casttics = caststate->tics;
    if (casttics == -1)
        casttics = 15;
}


//
// F_CastResponder
//
static
boolean F_CastResponder (event_t* ev)
{
    if (castdeath)
        return true;                    // already in dying frames

    // go into death frame
    castdeath = true;
    caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
    casttics = caststate->tics;
    castframes = 0;
    castattacking = false;
    if (mobjinfo[castorder[castnum].type].deathsound)
        S_StartSound(mobjinfo[castorder[castnum].type].deathsound);

    return true;
}


#define CASTNAME_Y   180                // where the name of actor is drawn
static
void F_CastPrint (char* text)
{
    V_DrawString ((BASEVIDWIDTH-V_StringWidth (text))/2, CASTNAME_Y, 0, text);
}


//
// F_CastDrawer
//
static
void F_CastDrawer (void)
{
    // drawinfo : from V_SetupDraw
    spritedef_t*        sprdef;
    sprite_frot_t *     sprfrot;
    patch_t*            patch;

    // erase the entire screen to a background
    //V_DrawPatch (0,0,0, W_CachePatchName ("BOSSBACK", PU_CACHE));
    D_PageDrawer ("BOSSBACK");

    F_CastPrint (castorder[castnum].name);

    // draw the current frame in the middle of the screen
    sprdef = &sprites[caststate->sprite];
    sprfrot = get_framerotation( sprdef, caststate->frame & FF_FRAMEMASK, 0 );
    patch = W_CachePatchNum (sprfrot->pat_lumpnum, PU_CACHE);  // endian fix

    // set draw effect flag for this draw only
    if( sprfrot->flip )
      drawinfo.effectflags = drawinfo.screen_effectflags | V_FLIPPEDPATCH;
    V_DrawScaledPatch (BASEVIDWIDTH>>1, 170, patch);
    drawinfo.effectflags = drawinfo.screen_effectflags;  // restore
}


//
// F_DrawPatchCol
//
// [WDJ] patch is already endian fixed
// Called by F_BunnyScroll
// sx is x position in 320 width screen
static
void F_DrawPatchCol (int sx, patch_t* patch, int col )
{
    // vid : from video setup
    column_t*   column;
    byte*       source;
    byte*       desttop;  // within screen buffer
    byte*       dest;
    int         count;

    // [WDJ] Draw for all bpp, bytepp, and padded lines.
    column = (column_t *)((byte *)patch + patch->columnofs[col]);
    desttop = screens[0] + (sx * vid.dupx * vid.bytepp);

    // step through the posts in a column
    while (column->topdelta != 0xff )
    {
        source = (byte *)column + 3;
        dest = desttop + (column->topdelta * vid.ybytes);
        count = column->length;

        while (count--)
        {
            int dupycount=vid.dupy;
            while(dupycount--)
            {
                int dupxcount=vid.dupx;
                while(dupxcount--)  // 3, 2, 1, 0
                     V_DrawPixel( dest, dupxcount, *source );
                dest += vid.ybytes;
            }
            source++;
        }
        column = (column_t *)(  (byte *)column + column->length + 4 );
    }
}


//
// F_BunnyScroll
//
static
void F_BunnyScroll (void)
{
    // vid : from video setup
    int         scrolled;
    int         sx;  // scroll offset
    patch_t*    p1;
    patch_t*    p2;
    char        name[15]; // only 10 used
    int         stage;
    static int  laststage;

    // Draw to screen0, scaled
   
    // patches are endian fixed
    p1 = W_CachePatchName ("PFUB2", PU_LEVEL);
    p2 = W_CachePatchName ("PFUB1", PU_LEVEL);

    if( gamemode == chexquest1 )
    {
        // if these are the bunny pictures, then replace them.
        p1 = Chex_safe_pictures( "PFUB2", p1 );
        p2 = Chex_safe_pictures( "PFUB1", p2 );
    }

#ifdef DIRTY_RECT
    V_MarkRect (0, 0, vid.width, vid.height);
#endif

    scrolled = 320 - (finalecount-230)/2;
    if (scrolled > 320)
        scrolled = 320;
    if (scrolled < 0)
        scrolled = 0;
    //faB:do equivalent for hw mode ?
    if( rendermode == render_soft )
    {
        for ( sx=0 ; sx<320 ; sx++)
        {
            if (sx+scrolled < 320)
                F_DrawPatchCol (sx, p1, sx+scrolled);
            else
                F_DrawPatchCol (sx, p2, sx+scrolled - 320);
        }
    }
    else
    {
        // HWR render
        if( scrolled>0 )
            V_DrawScaledPatch(320-scrolled, 0, p2 );
        if( scrolled<320 )
            V_DrawScaledPatch(-scrolled, 0, p1 );
    }

    // end0 .. end6
    if (finalecount < 1130)
        return;
    if (finalecount < 1180)
    {
        stage = 0;
        laststage = 0;
    }
    else
    {
        stage = (finalecount-1180) / 5;
        if (stage > 6)
            stage = 6;
        if (stage > laststage)
        {
            S_StartSound(sfx_pistol);
            laststage = stage;
        }
    }

    // Minumum buf of 14 because of GCC length complaints.  Only uses 5 bytes.
    snprintf (name, 14, "END%i", stage);  // stage 0..6
    name[14] = 0;
    V_DrawScaledPatch_Name( (320-13*8)/2, (200-8*8)/2, name );
}


// **  Heretic Finale Drawing

// Heretic
static
void F_DemonScroll(void)
{
    // vid : from video setup
    int scrolled;

    scrolled = (finalecount-70)/3;
    if (scrolled > 200)
        scrolled = 200;
    if (scrolled < 0)
        scrolled = 0;

    V_DrawRawScreen_Num(0, scrolled*vid.dupy, W_CheckNumForName("FINAL1"),320,200);
    if( scrolled>0 )
        V_DrawRawScreen_Num(0, (scrolled-200)*vid.dupy, W_CheckNumForName("FINAL2"),320,200);
}

static
void F_DrawHeretic( void )
{
    switch (gameepisode)
    {
      case 1:
        if( finalestage > 1 )  goto credit_page;
        if( VALID_LUMP( W_CheckNumForName("e2m1") ) )  goto credit_page;
        V_DrawRawScreen_Num(0, 0, W_CheckNumForName("ORDER"),320,200);
        break;
      case 2:
        // was F_DrawUnderwater()
        switch(finalestage)
        {
          case 1:
            if( finale_palette != 12 )  // [WDJ] heretic E2PAL
            {
                finale_palette = 12;
                V_SetPaletteLump("E2PAL");
            }
            V_DrawRawScreen_Num(0, 0, W_CheckNumForName("E2END"),320,200);
            break;
          case 3:
            V_DrawRawScreen_Num(0, 0, W_CheckNumForName("TITLE"),320,200);
            //D_StartTitle(); // go to intro/demo mode.
        }
        break;
      case 3:
        F_DemonScroll();
        break;
      case 4: // Just show credits screen for extended episodes
      case 5:
        goto credit_page;
    }
    return;

credit_page:
    // BP: search only in the first pwad since legacy define a patch with same name
    V_DrawRawScreen_Num(0, 0, W_CheckNumForNamePwad("CREDIT",0,0),320,200);
    return;
}

// ** End of Heretic Finale Drawing


// Called from F_Drawer, to draw full screen interpic, selected by name.
static
void F_Draw_interpic_Name( const char * name )
{
   patch_t*  pic = W_CachePatchName( name, PU_CACHE );  // endian fix
   // Intercept some doom pictures that chex.wad left in (a young kids game).
   if( gamemode == chexquest1 )
     pic = Chex_safe_pictures( name, pic );
   // Draw to screen0, scaled
   V_DrawScaledPatch(0,0, pic );
}

//
// F_Drawer
//
void F_Drawer (void)
{
    // Draw to screen0, scaled
    // [Arcade] V_SCALEEXACT: the ending screens are whole-screen 320x200 pages
    // like the attract ones, and letterboxed the same way without it.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ | V_SCALEEXACT );

    if( finalestage == 2 )
    {
        F_CastDrawer ();
        return;
    }

    if( !finalestage )
    {
#ifdef ENABLE_UMAPINFO
        // [MB] 2023-03-29: Skip for empty string
        if( finaletext[0] )
            F_TextWrite();
#else
        F_TextWrite ();
#endif
    }
    else
    {
        if( gamemode == heretic )
        {
            F_DrawHeretic();
        }
        else
        {
#ifdef ENABLE_UMAPINFO
           // [MB] 2023-03-29: Support for UMAPINFO added
           if( game_umapinfo && (game_umapinfo->flags & UMA_endbunny))
               F_BunnyScroll();
           else if( game_umapinfo && game_umapinfo->endpic )
               F_Draw_interpic_Name( game_umapinfo->endpic );
           else switch( gameepisode )
#else
           switch (gameepisode)
#endif
           {
            case 1:
              if( gamemode == ultdoom_retail || gamemode == chexquest1 )
                F_Draw_interpic_Name( text[CREDIT_NUM] );
              else
                F_Draw_interpic_Name( text[HELP2_NUM] );
              break;
            case 2:
              F_Draw_interpic_Name( text[VICTORY2_NUM] );
              break;
            case 3:
              F_BunnyScroll ();
              break;
            case 4:
              F_Draw_interpic_Name( text[ENDPIC_NUM] );
              break;
           }
        }
    }
}
