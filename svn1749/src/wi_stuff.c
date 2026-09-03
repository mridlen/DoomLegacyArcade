// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: wi_stuff.c 1699 2024-11-27 07:20:27Z wesleyjohnson $
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
// $Log: wi_stuff.c,v $
// Revision 1.16  2004/09/12 20:24:26  darkwolf95
// fix: no more animations or "YAH" on top of FS specified interpics
//
// Revision 1.15  2003/05/04 04:21:39  sburke
// Use SHORT macro to convert little-endian shorts on big-endian machines.
//
// Revision 1.14  2001/06/30 15:06:01  bpereira
// fixed wronf next level name in intermission
//
// Revision 1.13  2001/05/16 21:21:15  bpereira
//
// Revision 1.12  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.11  2001/03/03 06:17:34  bpereira
// Revision 1.10  2001/02/24 13:35:21  bpereira
// Revision 1.9  2001/02/10 12:27:14  bpereira
// Revision 1.8  2001/01/27 11:02:36  bpereira
//
// Revision 1.7  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.6  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/09/21 16:45:09  bpereira
// Revision 1.4  2000/08/31 14:30:56  bpereira
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Intermission screens.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "wi_stuff.h"
#include "g_game.h"
#include "hs_stuff.h"
#include "hu_stuff.h"
#include "m_random.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "screen.h"
#include "z_zone.h"
#include "console.h"
#include "p_info.h"
#include "dehacked.h"
  // pars_valid_bex
#include "m_menu.h"
  // M_DrawTextBox

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different between registered DOOM (1994) and
//  Ultimate DOOM - Final edition (ultdoom_retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II) (doom2_commercial), which had 34 maps
//  in one episode. So there.
#define NUM_YAH_EPISODES     4
#define NUM_MAPS_PER_EPI        9


// GLOBAL LOCATIONS
#define WI_TITLEY               2
#define WI_SPACINGY             16
  //TODO: was 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX               50
// [Arcade] x of the blinking MAX indicator, clear of the percentages.
#define SP_MAXIND_X            287
#define SP_STATSY               50

#define SP_TIMEX                16
#define SP_TIMEY                (BASEVIDHEIGHT-32)


// NET GAME STUFF
#define NG_STATSY               50
#define NG_STATSX               32

#define NG_SPACINGX             64


// DEATHMATCH STUFF
#define DM_MATRIXX              16
#define DM_MATRIXY              24

#define DM_SPACINGX             32

#define DM_TOTALSX              269

#define DM_KILLERSX             0
#define DM_KILLERSY             100
#define DM_VICTIMSX             5
#define DM_VICTIMSY             50
// in sec
#define DM_WAIT                 20



typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL
} animtype_e;

typedef struct
{
    int         x;
    int         y;
} point_t;


//
// Background Animation in Intermission.
// Texture animation is in p_spec.c.
//
typedef struct
{
    // Setup data (Constant)
    byte        type;  // animtype_e

    // period in tics between animations
    uint16_t    period;  // nominal  35/3

    // number of animation frames
    byte        num_anims;  // nominal 3

    // location of animation
    point_t     loc;

    // type specific setup information
    // ALWAYS: n/a,
    // RANDOM: period deviation (<256),
    // LEVEL: level
    byte        data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    byte        data2;

    // Variables
   
    // actual graphics for frames of animations
    patch_t*    p[3];
   
    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    uint32_t    nexttic;

    // next frame number to animate, init to -1, -1 is off
    int8_t      frame_num;   // 0 .. num_anim-1  (0..3)

#if 0
    // [WDJ] Unused

    // last drawn animation frame
    int         lastdrawn;

    // used by RANDOM and LEVEL when animating
    int         state;
#endif

} anim_inter_t;

// Doom YAH nodes (lnode)
static point_t doom_YAH_nodes[NUM_YAH_EPISODES][NUM_MAPS_PER_EPI] =
{
    // Episode 0 World Map
    {
        { 185, 164 },   // location of level 0 (CJ)
        { 148, 143 },   // location of level 1 (CJ)
        { 69, 122 },    // location of level 2 (CJ)
        { 209, 102 },   // location of level 3 (CJ)
        { 116, 89 },    // location of level 4 (CJ)
        { 166, 55 },    // location of level 5 (CJ)
        { 71, 56 },     // location of level 6 (CJ)
        { 135, 29 },    // location of level 7 (CJ)
        { 71, 24 }      // location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
        { 254, 25 },    // location of level 0 (CJ)
        { 97, 50 },     // location of level 1 (CJ)
        { 188, 64 },    // location of level 2 (CJ)
        { 128, 78 },    // location of level 3 (CJ)
        { 214, 92 },    // location of level 4 (CJ)
        { 133, 130 },   // location of level 5 (CJ)
        { 208, 136 },   // location of level 6 (CJ)
        { 148, 140 },   // location of level 7 (CJ)
        { 235, 158 }    // location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
        { 156, 168 },   // location of level 0 (CJ)
        { 48, 154 },    // location of level 1 (CJ)
        { 174, 95 },    // location of level 2 (CJ)
        { 265, 75 },    // location of level 3 (CJ)
        { 130, 48 },    // location of level 4 (CJ)
        { 279, 23 },    // location of level 5 (CJ)
        { 198, 48 },    // location of level 6 (CJ)
        { 140, 25 },    // location of level 7 (CJ)
        { 281, 136 }    // location of level 8 (CJ)
    }
};

// Heretic YAH
static point_t Heretic_YAHspot[3][9] =
{
    {
        { 172, 78 },
        { 86, 90 },
        { 73, 66 },
        { 159, 95 },
        { 148, 126 },
        { 132, 54 },
        { 131, 74 },
        { 208, 138 },
        { 52, 101 }
    },
    {
        { 218, 57 },
        { 137, 81 },
        { 155, 124 },
        { 171, 68 },
        { 250, 86 },
        { 136, 98 },
        { 203, 90 },
        { 220, 140 },
        { 279, 106 }
    },
    {
        { 86, 99 },
        { 124, 103 },
        { 154, 79 },
        { 202, 83 },
        { 178, 59 },
        { 142, 58 },
        { 219, 66 },
        { 247, 57 },
        { 107, 80 }
    }
};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_inter_t epsd0_animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_inter_t epsd1_animinfo[] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_inter_t epsd2_animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static byte  num_anim[NUM_YAH_EPISODES] =
{
    sizeof(epsd0_animinfo)/sizeof(anim_inter_t),
    sizeof(epsd1_animinfo)/sizeof(anim_inter_t),
    sizeof(epsd2_animinfo)/sizeof(anim_inter_t)
};

static anim_inter_t * anim_inter_info[NUM_YAH_EPISODES] =
{
    epsd0_animinfo,
    epsd1_animinfo,
    epsd2_animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//

// States for the intermission

typedef enum
{
    // [WDJ] There is no good reason for using -1.
    NoState,
    StatCount,
    ShowNextLoc
} state_e;

// States for single-player
#define SP_KILLS                0
#define SP_ITEMS                2
#define SP_SECRET               4
#define SP_FRAGS                6
#define SP_TIME                 8
#define SP_PAR                  ST_TIME

#define SP_PAUSE                1

// in seconds
#define SHOWNEXTLOCDELAY        4
//#define SHOWLASTLOCDELAY      SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
static byte             accelerate_stage;

// signals to refresh everything for one frame
static byte             first_refresh;

// wbs->pnum
static int              me;

// specifies current state
static byte            state;  // state_e

// contains information passed into intermission
static wb_start_t     * wbs = NULL;

static wb_player_t    * wb_plyr;  // wbs->plyr[]

// used for general timing
static int              cnt;

// used for timing of background animation
static uint32_t         bcnt;

// [Arcade] Did the level just finished satisfy the max category (100% kills
// and 100% secrets)?  Set by WI_Init_Stats from the same expression it hands
// to HS_LevelExit, so the indicator and the scoring cannot disagree.
static boolean          sp_maxed;
static int              cnt_kills[MAXPLAYERS];
static int              cnt_items[MAXPLAYERS];
static int              cnt_secret[MAXPLAYERS];
static int              cnt_time;
static int              cnt_par;

// timers
       int              wait_game_start_timer = 0;  // subject to network sync
static int              effect_timer;


//
//      GRAPHICS
//
// [WDJ] all patches are saved endian fixed

// background (map of levels).
//static patch_t*       bg;
static char             bgname[9];

// You Are Here graphic
// [2]  - You Were Here - splat
static patch_t*         yah[3];

// %, : graphics
static patch_t*         percent;
static patch_t*         colon;

// 0-9 graphic
static patch_t*         num[10];

// minus sign
static patch_t*         wiminus;

// "Entering" and "Finished!" graphics
static patch_t*         finished;
static patch_t*         entering;

// "secret"
static patch_t*         sp_secret;

 // "Kills", "Scrt", "Items", "Frags"
static patch_t*         kills;
static patch_t*         secret;
static patch_t*         items;
static patch_t*         frags;

// Time sucks.
static patch_t*         timePatch;
static patch_t*         par;
static patch_t*         sucks;

// "killers", "victims"
static patch_t*         killers;
static patch_t*         victims;

// "Total", your face, your dead face
static patch_t*         total;
static patch_t*         pl_face;
static patch_t*         dead_face;

//added:08-02-98: use STPB0 for all players, but translate the colors
static patch_t*         stpb;

// Name graphics of each level (centered)
static patch_t**        lnames = NULL;

// # of doom2_commercial levels
static int              num_lnames = 0;


#ifdef RANGECHECK_XXX
void detect_range_violation( int item, int lowlim, int highlim )
{
    if( item < lowlim || item > highlim )
        GenPrintf( EMSG_error, "Range violation  %i, limits=(%i..%i)\n", item, lowlim, highlim );
}
#endif

// [WDJ] All patch endian conversion is done in W_CachePatchNum

//
// CODE
//

#ifdef ENABLE_UMAPINFO
// [MB] 2023-03-19: Support for UMAPINFO added
// Moved out of WI_Load_Data() into separate function because with UMAPINFO
// the LF and EL screens can use different background pictures.
static void WI_Prepare_Background(void)
{
    // Prepare new background from bgname for software renderer
    if (rendermode == render_soft)
    {
        memset(screens[0], 0, vid.screen_size);

        // clear backbuffer from status bar stuff and borders
        memset(screens[1], 0, vid.screen_size);

        // Draw background on screen1
        // [Arcade] V_SCALEEXACT: cover the screen, as the attract pages do.
        // Same 320x200-into-a-whole-multiple letterbox otherwise -- see
        // docs/arcade/screen-fill.md.
        V_SetupDraw(1 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ | V_SCALEEXACT); // screen 1
        V_DrawScaledPatch(0, 0, W_CachePatchName(bgname, PU_CACHE));
        V_SetupDraw(drawinfo.prev_screenflags);  // restore
    }
}
#endif

// slam background
// UNUSED static unsigned char *background=0;

// Called by WI_Draw_Stats, WI_Draw_DeathmatchStats, WI_Draw_TeamsStats
// Called by WI_Draw_NetgameStats, WI_Draw_ShowNextLoc
static void WI_Slam_Background(void)
{
    // all WI_Draw_ is from WI_Drawer, draw screen0, scale
   
    // vid : from video setup
    // draw background on screen0
    if( EN_heretic && state == StatCount)
        V_ScreenFlatFill( W_CheckNumForName("FLOOR16") );
    else
    if( rendermode == render_soft ) 
    {
        memcpy(screens[0], screens[1], vid.screen_size);  // background to display
#ifdef DIRTY_RECT
        V_MarkRect (0, 0, vid.width, vid.height);
#endif
    }
    else 
    {
        // [WDJ] was draw to screen1, but hw draw does not differentiate
        // hardware draw, ( to screen0 same as above )
        V_DrawScaledPatch(0, 0, W_CachePatchName(bgname, PU_CACHE));
    }
}

// The ticker is used to detect keys
//  because of timing issues in netgames.
boolean WI_Responder(event_t* ev)
{
    return false;
}


// Draws "<Levelname> Finished!"
static void WI_Draw_LF(void)
{
    // Hardware or software render.
    patch_t * pp, * pf;
#ifdef ENABLE_UMAPINFO
    int x = 0;
#endif
    int y = WI_TITLEY;

#ifdef ENABLE_UMAPINFO
    // [MB] 2023-03-12: Support for UMAPINFO added
    //
    // Quoted from UMAPINFO specification Rev 2.2 definition of levelpic:
    // Specifies the patch that is used on the status screen for 'entering' and
    // 'finished'. [...]
    // If not given, the status screen will instead print the map's name with a
    // suitable font (PrBoom uses STFxxx) to ensure that the proper name is
    // used. If the author field is set, it will also be shown.
    if (wbs->umapinfo_done && wbs->umapinfo_done->levelpic)
    {
        pp = W_CachePatchName(wbs->umapinfo_done->levelpic, PU_CACHE);
        pf = V_patch(pp);  // access patch fields
        x = (BASEVIDWIDTH - pf->width) / 2;
        V_DrawScaledPatch(x, y, pp);

        y += (5 * pf->height) / 4;

        x = (BASEVIDWIDTH - (V_patch(finished)->width)) / 2;
        V_DrawScaledPatch(x, y, finished);
    }
    else if (wbs->umapinfo_done && wbs->umapinfo_done->levelname)
    {
        const char * level_string = wbs->umapinfo_done->levelname;

        x = (BASEVIDWIDTH - V_StringWidth(level_string)) / 2;
        V_DrawString(x, y, V_WHITEMAP, level_string);

        if (wbs->umapinfo_done && wbs->umapinfo_done->author)
        {
            const char * author_string = wbs->umapinfo_done->author;

            y += (5 * V_FontInfo()->height) / 4;

            x = (BASEVIDWIDTH - V_StringWidth(author_string)) / 2;
            V_DrawString(x, y, V_WHITEMAP, author_string);
        }

        y += 2 * V_FontInfo()->height;

        x = (BASEVIDWIDTH - (V_patch(finished)->width)) / 2;
        V_DrawScaledPatch(x, y, finished);
    }
    // Normal behaviour without UMAPINFO
    else
    {
        // draw <LevelName>
        if (FontBBaseLump)
        {
            x = (BASEVIDWIDTH - V_TextBWidth(P_LevelName())) / 2;
            V_DrawTextB(P_LevelName(), x, y);

            y += (5 * V_TextBHeight(P_LevelName())) / 4;

            x = (BASEVIDWIDTH - V_TextBWidth("Finished")) / 2;
            V_DrawTextB("Finished", x, y);
        }
        else
        {
            //[segabor]: 'SHORT' BUG !  [WDJ] Patch read does endian conversion
            pp = lnames[wbs->lev_prev];
            pf = V_patch( pp );  // access patch fields
            x = (BASEVIDWIDTH - pf->width) / 2;
            V_DrawScaledPatch(x, y, pp);

            y += (5 * pf->height) / 4;

            x = (BASEVIDWIDTH - (V_patch(finished)->width)) / 2;
            // draw "Finished!"
            V_DrawScaledPatch(x, y, finished);
        }
    }
#else
    // draw <LevelName>
    if( FontBBaseLump )
    {
        V_DrawTextB(P_LevelName(), (BASEVIDWIDTH - V_TextBWidth(P_LevelName()))/2, y);
        y += (5*V_TextBHeight(P_LevelName()))/4;
        V_DrawTextB("Finished", (BASEVIDWIDTH - V_TextBWidth("Finished"))/2, y);
    }
    else
    {
        //[segabor]: 'SHORT' BUG !  [WDJ] Patch read does endian conversion
        pp = lnames[wbs->lev_prev];
        pf = V_patch( pp );  // access patch fields
        V_DrawScaledPatch ((BASEVIDWIDTH - pf->width)/2, y, pp);
        y += (5 * pf->height)/4;
        // draw "Finished!"
        V_DrawScaledPatch ((BASEVIDWIDTH - (V_patch(finished)->width))/2,
                            y, finished);
    }
#endif	
}



// Draws "Entering <LevelName>"
static void WI_Draw_EL(void)
{
    // Hardware or software render.
    patch_t * pp, * pf;
#ifdef ENABLE_UMAPINFO
    int x = 0;
#endif
    int y = WI_TITLEY;

#ifdef ENABLE_UMAPINFO
    // [MB] 2023-03-12: Support for UMAPINFO added
    // See WI_Draw_LF() for additional notes about levelpic
    if (wbs->umapinfo_next && wbs->umapinfo_next->levelpic)
    {
        x = (BASEVIDWIDTH - (V_patch(entering)->width)) / 2;
        V_DrawScaledPatch(x, y, entering);

        y += (5 * V_patch(entering)->height) / 4;

        pp = W_CachePatchName(wbs->umapinfo_next->levelpic, PU_CACHE);
        pf = V_patch(pp);  // access patch fields
        x = (BASEVIDWIDTH - pf->width) / 2;
        V_DrawScaledPatch(x, y, pp);
    }
    else if (wbs->umapinfo_next && wbs->umapinfo_next->levelname)
    {
        const char * levname = wbs->umapinfo_next->levelname;

        x = (BASEVIDWIDTH - (V_patch(entering)->width)) / 2;
        V_DrawScaledPatch(x, y, entering);

        y += V_patch(entering)->height + V_FontInfo()->height;

        x = (BASEVIDWIDTH - V_StringWidth(levname)) / 2;
        V_DrawString(x, y, V_WHITEMAP, levname);

        if (wbs->umapinfo_next && wbs->umapinfo_next->author)
        {
            const char * author = wbs->umapinfo_next->author;

            y += (5 * V_FontInfo()->height) / 4;

            x = (BASEVIDWIDTH - V_StringWidth(author)) / 2;
            V_DrawString(x, y, V_WHITEMAP, author);
        }
    }
    // Normal behaviour without UMAPINFO
    else
    {
        // draw "Entering"
        if (FontBBaseLump)
        {
            const char * levname = P_LevelNameByNum(wbs->epsd + 1,
                                                         wbs->lev_next + 1);

            x = (BASEVIDWIDTH - V_TextBWidth("Entering")) / 2;
            V_DrawTextB("Entering", x, y);

            y += (5 * V_TextBHeight("Entering")) / 4;

            x = (BASEVIDWIDTH - V_TextBWidth(levname)) / 2;
            V_DrawTextB(levname, x, y);
        }
        else
        {
            //[segabor]: 'SHORT' BUG !  [WDJ] Patch read does endian conversion
            x = (BASEVIDWIDTH - (V_patch(entering)->width)) / 2;
            V_DrawScaledPatch(x, y, entering);

            // draw level
            pp = lnames[wbs->lev_next];
            pf = V_patch(pp);  // access patch fields

            y += (5 * pf->height) / 4;

            x = (BASEVIDWIDTH - pf->width) / 2;
            V_DrawScaledPatch(x, y, pp);
        }
    }
#else
    // draw "Entering"
    if( FontBBaseLump )
    {
        const char * levname = P_LevelNameByNum(wbs->epsd+1, wbs->lev_next+1);
        V_DrawTextB("Entering", (BASEVIDWIDTH - V_TextBWidth("Entering"))/2, y);
        y += (5*V_TextBHeight("Entering"))/4;
        V_DrawTextB( levname, (BASEVIDWIDTH - V_TextBWidth(levname))/2, y);
    }
    else
    {
        //[segabor]: 'SHORT' BUG !    [WDJ] Patch read does endian conversion
        V_DrawScaledPatch((BASEVIDWIDTH - (V_patch(entering)->width))/2,
                          y, entering);
        // draw level
        pp = lnames[wbs->lev_next];
        pf = V_patch( pp );  // access patch fields
        y += (5 * pf->height)/4;

        V_DrawScaledPatch((BASEVIDWIDTH - pf->width)/2, y, pp);
    }
#endif
}

// [WDJ] Made more resistent to segfault.
// Doom YAH draw
//  n : YAH index
//  yi : yah index, 2=splat
static void WI_Doom_Draw_YAH ( int  n, int yi )
{
    // Hardware or software render.
    patch_t   * p;
    patch_t   * pf;
    point_t   * lnodes;
    int         left, top;

    lnodes = &doom_YAH_nodes[wbs->epsd][n];

    for(;;)
    {
        p = yah[yi];
        pf = V_patch( p );
        left   = lnodes->x - pf->leftoffset;
        top    = lnodes->y - pf->topoffset;
        if (left >= 0
            && (left + pf->width) < BASEVIDWIDTH
            && top >= 0
            && (top + pf->height) < BASEVIDHEIGHT)
        {
            V_DrawScaledPatch(lnodes->x, lnodes->y, p);
            return;
        }

        // yah[0] -> yah[1]
        if( yi > 0 )  break;
        yi++;
    }

    // DEBUG
    debug_Printf("Could not place patch on level %d\n", n+1);
}


//========================================================================
//
// WI_Heretic_Draw_YAH
//
//========================================================================
static void WI_Heretic_Draw_YAH(void)
{
    int i;
    int x;
    int prevmap;
    point_t *  yah_pts = Heretic_YAHspot[gameepisode-1];

    x = (BASEVIDWIDTH-V_StringWidth("NOW ENTERING:"))/2;
    V_DrawString(x, 10, 0, "NOW ENTERING:");

    x = (BASEVIDWIDTH-V_TextBWidth(P_LevelNameByNum(wbs->epsd+1, wbs->lev_next+1)))/2;
    V_DrawTextB(P_LevelNameByNum(wbs->epsd+1, wbs->lev_next+1), x, 20);

    prevmap = (wbs->lev_prev == 8) ? wbs->lev_next - 1 : wbs->lev_prev;

    for(i=0; i<=prevmap; i++)
    {
        V_DrawScaledPatch(yah_pts[i].x, yah_pts[i].y, yah[2]);  // splat
    }
    if(players[consoleplayer].GF_flags & GF_didsecret)
    {
        V_DrawScaledPatch(yah_pts[8].x, yah_pts[8].y, yah[2]);  // splat
    }
    if(!(bcnt&16) || state == ShowNextLoc)
    { // draw the destination 'X'
        V_DrawScaledPatch(yah_pts[wbs->lev_next].x, yah_pts[wbs->lev_next].y, yah[0]);
    }
}



// Called by WI_Start->WI_Init_Stats
// Called by WI_Start->WI_Init_DeathmatchStats
// Called by WI_Init_ShowNextLoc
// Called by WI_update_ShowNextLoc
static void WI_Init_AnimatedBack(void)
{
    byte       i;
    anim_inter_t*  ai;

        //DarkWolf95:September 12, 2004: Don't draw animations for FS changed interpic
    if (gamemode == doom2_commercial || gamemode == heretic || *info_interpic)
        return;

    if (wbs->epsd > 2)  // allow episodes 1,2,3
        return;

    // episodes 1 to 3 have animation
    for (i=0; i<num_anim[wbs->epsd]; i++)
    {
        ai = &anim_inter_info[wbs->epsd][i];

        // init variables
        ai->frame_num = -1;

        // specify the next time to draw it
        if (ai->type == ANIM_ALWAYS)
            ai->nexttic = bcnt + 1 + (M_Random()%ai->period);
        else if (ai->type == ANIM_RANDOM)
        {
            // data1 = period deviation, data2 = period base
            ai->nexttic = bcnt + 1 + ai->data2+(M_Random()%ai->data1);
        }
        else if (ai->type == ANIM_LEVEL)
            ai->nexttic = bcnt + 1;
    }

}

static void WI_update_AnimatedBack(void)
{
    byte       i;
    anim_inter_t*  ai;

        //DarkWolf95:September 12, 2004: Don't draw animations for FS changed interpic
    if (gamemode == doom2_commercial || gamemode == heretic || *info_interpic)
        return;

    if (wbs->epsd > 2)
        return;

    // episodes 1 to 3 have animation
    for (i=0; i<num_anim[wbs->epsd]; i++)
    {
        ai = &anim_inter_info[wbs->epsd][i];

        if( ai->nexttic > bcnt )  continue;

        switch (ai->type)
        {
          case ANIM_ALWAYS:
            if (++ai->frame_num >= ai->num_anims)   ai->frame_num = 0;
            ai->nexttic = bcnt + ai->period;
            break;

          case ANIM_RANDOM:
            ai->frame_num++;
            if (ai->frame_num == ai->num_anims)
            {
                ai->frame_num = -1;
                // data1 = period deviation, data2 = period base
                ai->nexttic = bcnt + ai->data2 + (M_Random()%ai->data1);
            }
            else
            {
                ai->nexttic = bcnt + ai->period;
            }
            break;

          case ANIM_LEVEL:
            // gawd-awful hack for level anims
            if( state == StatCount && i == 7 )  break;
            // data1 = level
            if( wbs->lev_next == ai->data1 )
            {
                ai->frame_num++;
                if (ai->frame_num == ai->num_anims)   ai->frame_num--;
                ai->nexttic = bcnt + ai->period;
            }
            break;
        }
    }
}

static void WI_Draw_AnimatedBack(void)
{
    byte  i;
    anim_inter_t*  ai; // interpic animation data

    //BP: fixed it was "if (doom2_commercial)" 
        //DarkWolf95:September 12, 2004: Don't draw animations for FS changed interpic
    if (gamemode == doom2_commercial || gamemode == heretic || *info_interpic)
        return;

    if (wbs->epsd > 2)
        return;

    // episodes 1 to 3 have animation
    for (i=0 ; i<num_anim[wbs->epsd] ; i++)
    {
        ai = &anim_inter_info[wbs->epsd][i];

        if(ai->frame_num >= 0)
            V_DrawScaledPatch(ai->loc.x, ai->loc.y, ai->p[ai->frame_num]);
    }

}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//
//  n : number to be drawn.  NON_NUMBER is not drawn
//  digits : number of digits,  -1 is variable length

static int WI_Draw_Num ( int  x, int  y,
                        int  n,
                        int  digits )
{
    // Hardware or software render, access patch fields.
    int  fontwidth = V_patch( num[0] )->width;
    int  neg;
    int  temp;

    if (digits < 0)
    {
        if (!n)
        {
            // make variable-length zeros 1 digit long
            digits = 1;
        }
        else
        {
            // figure out # of digits in #
            digits = 0;
            temp = n;

            while (temp)
            {
                temp /= 10;
                digits++;
            }
        }
    }

    neg = n < 0;
    if (neg)
        n = -n;

    // if non-number, do not draw it
    if (n == NON_NUMBER)
        return 0;

    // draw the new number
    while (digits--)
    {
        x -= fontwidth;
        V_DrawScaledPatch(x, y, num[ n % 10 ]);
        n /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
        V_DrawScaledPatch(x-=8, y, wiminus);

    return x;

}

// draw a percentage at x,y, blank when -1, a dash when -100
static void WI_Draw_Percent( int  x, int  y, int  pernum )
{
    if (pernum < 0)
    {
        if (pernum == -100 )  // no secrets, items, etc..
           V_DrawScaledPatch(x, y, wiminus);
        return;
    }

    V_DrawScaledPatch(x, y, percent);
    WI_Draw_Num(x, y, pernum, -1);
}



//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
static void WI_Draw_Time ( int x, int y, int t )
{
    int  timediv;  // div is keyword
    int  n;

    if (t<0)
        return;

    // [WDJ] 1/12/2009 fix crashes in heretic, no sucks
    // Old PAR behavior for id wads, otherwise allow them 24 hrs.
    if( (t <= ((gamedesc.gameflags & GD_idwad)? (61*59) : (24*60*60)) )
        || (sucks == NULL) )
    {
        timediv = 1;

        // Hardware or software render.
        do
        {
            n = (t / timediv) % 60;
            x = WI_Draw_Num(x, y, n, 2) - V_patch(colon)->width;
            timediv *= 60;

            // draw
            if (timediv==60 || t / timediv)
                V_DrawScaledPatch(x, y, colon);

        } while (t / timediv);
    }
    else
    {
        // "sucks"
        V_DrawScaledPatch(x - (V_patch(sucks)->width), y, sucks);
    }
}

// For startup wait, and deathmatch wait.
void WI_Draw_wait( int net_nodes, int net_players, int wait_players, int wait_tics )
{
    int  length = 25, lines = 1;
    char * waitmsg;
    char * msg2 = NULL;

    // Using va_buffer (m_misc.c)
    if( wait_players )
    {
        waitmsg = va("WAIT PLAYERS %2d/%2d : NODES %2d : TIMEOUT %4d",
                    net_players, wait_players, net_nodes, wait_tics/TICRATE);
        length = 38;  // Doom 28, but Heretic text uses more
        if( server )
        {
            lines = 2;
            msg2 = " s = start now,  q = escape";
        }
    }
    else
    {
        waitmsg = va("START IN %4d", wait_tics/TICRATE);
        length = 18;  // Doom 10
    }
    // Heretic: The wait message barely fits within the screen width.
    //i=V_StringWidth(num);
    M_DrawTextBox( 2, 20, length, lines );
    V_DrawString( 12, 28, V_WHITEMAP, waitmsg );
    if( msg2 )
        V_DrawString( 12, 36, V_WHITEMAP, msg2 );
}



// used for write introduce next level
void WI_Init_NoState(void)
{
    state = NoState;
    accelerate_stage = 0;
    cnt = 10;
}


static boolean          snl_pointeron = false;


// Called by WI_update_NetgameStats, WI_update_Stats
static void WI_Init_ShowNextLoc(void)
{
    state = ShowNextLoc;
    accelerate_stage = 0;
    cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_Init_AnimatedBack();
}

static void WI_update_ShowNextLoc(void)
{
    if (!--cnt || accelerate_stage)
        WI_Init_NoState();
    else
        snl_pointeron = (cnt & 31) < 20;
}

// Called by WI_Drawer, WI_Draw_NoState
static void WI_Draw_ShowNextLoc(void)
{

    int  i;
    int  last;

    if (cnt<=0)  // all removed no draw !!!
        return;

#ifdef ENABLE_UMAPINFO
    // [MB] 2023-04-01: Support for UMAPINFO added
    if (wbs->umapinfo_done)
    {
        if (wbs->umapinfo_done->flags & (UMA_endgame_enabled | UMA_endbunny | UMA_endcast) )
            return;
    }

    if (wbs->umapinfo_next && wbs->umapinfo_next->enterpic)
    {
        strcpy(bgname, wbs->umapinfo_next->enterpic);
        WI_Prepare_Background();
    }
#endif

    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    if( EN_heretic )
    {
        if( gameepisode < 4 )
            WI_Heretic_Draw_YAH();
    }
        //DarkWolf95:September 12, 2004: Don't draw YAH for FS changed interpic
    else
    if ( (gamemode != doom2_commercial)
         && wbs->epsd<=2 && !*info_interpic)  // episode 1,2,3 have animation
    {
        // You are here  (YAH).
        last = (wbs->lev_prev == 8) ? wbs->lev_next - 1 : wbs->lev_prev;

        // draw a yah splat on taken cities.
        for (i=0 ; i<=last ; i++)
            WI_Doom_Draw_YAH(i, 2);  // splat

        // yah splat the secret level?
        if (wbs->didsecret)
            WI_Doom_Draw_YAH(8, 2);  // splat

        // draw flashing ptr
        if (snl_pointeron)
            WI_Doom_Draw_YAH(wbs->lev_next, 0);  // yah[0] or yah[1]
    }

    // draws which level you are entering..
#ifdef ENABLE_UMAPINFO
    if (EN_doom_etc)
    {
        uint8_t draw_el = ! ((gamemode == doom2_commercial) && (wbs->lev_next == 30));  // any map but map31
        // [MB] 2023-04-02: Draw EL if UMAPINFO has disabled endgame
        if( wbs->umapinfo_done )
           draw_el |= (wbs->umapinfo_done->flags & UMA_endgame_disabled); // or explicit umapinfo endgame disable

        if ( draw_el )
            WI_Draw_EL();
    }
#else
    if ( EN_doom_etc
         && !((gamemode == doom2_commercial) && (wbs->lev_next == 30)) )   // any map but map31
        WI_Draw_EL();
#endif
}

// Called by WI_Drawer
static void WI_Draw_NoState(void)
{
    snl_pointeron = true;
    WI_Draw_ShowNextLoc();
}


static int              dm_frags[MAXPLAYERS][MAXPLAYERS];
static int              dm_totals[MAXPLAYERS];

// Called by WI_Start
static void WI_Init_DeathmatchStats(void)
{
    int i, j;

    state = StatCount;
    accelerate_stage = 0;

    memset( dm_frags, 0, sizeof(dm_frags) );  // for new players
    memset( dm_totals, 0, sizeof(dm_totals) );

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
         if (playeringame[i])
         {
             for(j=0; j<MAXPLAYERS; j++)
             {
                 if( playeringame[j] )
                     dm_frags[i][j] = wb_plyr[i].frags[j];
             }
             
             dm_totals[i] = ST_PlayerFrags(i);
         }
    }

    WI_Init_AnimatedBack();
}



//  Quick-patch for the Cave party 19-04-1998 !!
//
//  width : the column width
// [Arcade] y_limit: base-unit y at which to stop, because the caller may have
// offset this block into a viewport cell.  The bottom-row cells of a 2x2 sit
// past base y 200, so the old hardcoded BASEVIDHEIGHT test broke out after a
// single row -- which read as "the players on 0 points are missing", since
// the table is sorted highest first.
void WI_Draw_Ranking(const char * title, int x, int y, fragsort_t * fragtable,
                    int scorelines, boolean large, int white, int colwidth,
                    int y_limit)
{
    char  buf[33];
    int   i,j;
    int   skin_color, color;
    int   plnum;
    int   frags;
    int   colornum;
    fragsort_t temp;

   
    if( EN_heretic )
        colornum = 230;
    else
        colornum = 0x78;
   
    if( colwidth > 32 )  colwidth=32;

    // sort the frags count
    for (i=0; i<scorelines; i++)
    {
        for(j=0; j<scorelines-1-i; j++)
        {
            if( fragtable[j].count < fragtable[j+1].count )
            {
                temp = fragtable[j];
                fragtable[j] = fragtable[j+1];
                fragtable[j+1] = temp;
            }
        }
    }

    if(title)
        V_DrawString (x, y-14, 0, title);
    // draw rankings
    for (i=0; i<scorelines; i++)
    {
        frags = fragtable[i].count;
        plnum = fragtable[i].num;

        // draw color background
        skin_color = fragtable[i].color;
        color = (skin_color) ?
           SKIN_TO_SKINMAP(skin_color)[ colornum ]
         : reg_colormaps[ colornum ];  // default green skin
        V_DrawScaledFill (x-1,y-1, (large ? 40 : 26),9, color);

        // draw frags count, right justified
        sprintf(buf,"%3i", frags );
        V_DrawString (x+(large ? 32 : 24)-V_StringWidth(buf), y, 0, buf);

        // draw name, truncate to colwidth
        memset(buf, ' ', 32);  // to defeat string centering
        snprintf(buf, 31, "%s", fragtable[i].name );
        buf[colwidth] = 0;  // truncate to column width
        V_DrawString (x+(large ? 64 : 29), y,
                      ((plnum == white) ? V_WHITEMAP : 0), buf);

        y += 12;
        if (y >= y_limit)
            break;            // dont draw past the bottom of this view
    }
}

#define RANKINGY 60

// Called by WI_Drawer
static void WI_Draw_DeathmatchStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    fragsort_t   fragtab[MAXPLAYERS];

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();
    WI_Draw_LF();

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quickly see who's the
    //  view.
    whiteplayer = demoplayback ? displayplayer : consoleplayer;

    // count frags for each present player
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = dm_totals[i];
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }
    WI_Draw_Ranking("Frags", 5, RANKINGY, fragtab, scorelines, false, whiteplayer, 6,
                    BASEVIDHEIGHT);

    // count buchholz
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (playeringame[j] && i!=j)
                     fragtab[scorelines].count+= dm_frags[i][j]*(dm_totals[j]+dm_frags[j][j]);

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }
    WI_Draw_Ranking("Buchholz", 85, RANKINGY, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);

    // count individual
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (playeringame[j] && i!=j)
                {
                     if(dm_frags[i][j]>dm_frags[j][i])
                         fragtab[scorelines].count+=3;
                     else
                         if(dm_frags[i][j]==dm_frags[j][i])
                              fragtab[scorelines].count+=1;
                }
            }

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }
    WI_Draw_Ranking("indiv.", 165, RANKINGY, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);

    // count deads
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (playeringame[j])
                     fragtab[scorelines].count+=dm_frags[j][i];
            }
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];

            scorelines++;
        }
    }
    WI_Draw_Ranking("deads", 245, RANKINGY, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);
}

boolean teamingame(int teamnum)
{
   int i;

   if( cv_teamplay.EV == 1 )
   {
       for(i=0;i<MAXPLAYERS;i++)
       {
          if(playeringame[i] && players[i].skincolor==teamnum)
              return true;
       }
   }
   else
   if( cv_teamplay.EV == 2)
   {
       for(i=0;i<MAXPLAYERS;i++)
       {
          if(playeringame[i] && players[i].skin==teamnum)
              return true;
       }
   }
   return false;
}

// Called by WI_Drawer
static void WI_Draw_TeamsStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    fragsort_t   fragtab[MAXPLAYERS];

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();
    WI_Draw_LF();

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quickly see who's the
    //  view.
    if( cv_teamplay.EV == 1 )
        whiteplayer = demoplayback ? displayplayer_ptr->skincolor
                                   : consoleplayer_ptr->skincolor;
    else
        whiteplayer = demoplayback ? displayplayer_ptr->skin
                                   : consoleplayer_ptr->skin;

    // count frags for each present player
    scorelines = HU_Create_TeamFragTbl(fragtab,dm_totals,dm_frags);

    WI_Draw_Ranking("Frags", 5, 80, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);

    // count buchholz
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (teamingame(j) && i!=j)
                    fragtab[scorelines].count+= dm_frags[i][j]*dm_totals[j];
            }

            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = get_team_name(i);
            scorelines++;
        }
    }
    WI_Draw_Ranking("Buchholz", 85, 80, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);

    // count individuel
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (teamingame(j) && i!=j)
                {
                     if(dm_frags[i][j]>dm_frags[j][i])
                         fragtab[scorelines].count+=3;
                     else
                         if(dm_frags[i][j]==dm_frags[j][i])
                              fragtab[scorelines].count+=1;
                }
            }

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = get_team_name(i);
            scorelines++;
        }
    }
    WI_Draw_Ranking("indiv.", 165, 80, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);

    // count deads
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (teamingame(j))
                     fragtab[scorelines].count+=dm_frags[j][i];
            }
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = get_team_name(i);

            scorelines++;
        }
    }
    WI_Draw_Ranking("deads", 245, 80, fragtab, scorelines, false, whiteplayer, 6, BASEVIDHEIGHT);
}


/* old code
#define FB  0
static void WI_ddrawDeathmatchStats(void)
{

    int         i;
    int         j;
    int         x;
    int         y;
    int         w;

    int         lh;     // line height

    byte*       colormap;       //added:08-02-98:see below

    lh = WI_SPACINGY;

    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();
    WI_Draw_LF();

    // draw stat titles (top line)
    V_DrawScaledPatch(DM_TOTALSX - V_patch(total)->width/2,
                DM_MATRIXY-WI_SPACINGY+10,
                total);

    V_DrawScaledPatch(DM_KILLERSX, DM_KILLERSY, killers);
    V_DrawScaledPatch(DM_VICTIMSX, DM_VICTIMSY, victims);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            //added:08-02-98: use V_DrawMappedPatch instead of
            //                    V_DrawScaledPatch, so that the
            // graphics are 'colormapped' to the player's colors!
            if (players[i].skincolor==0)
                colormap = colormaps;
            else
                colormap = (byte *) translationtables - 256 + (players[i].skincolor<<8);

            V_DrawMappedPatch(x - (V_patch(stpb)->width/2),
                        DM_MATRIXY - WI_SPACINGY,
                        stpb,      //p[i], now uses a common STPB0 translated
                        colormap); //      to the right colors

            V_DrawMappedPatch(DM_MATRIXX - (V_patch(stpb)->width/2),
                        y,
                        stpb,      //p[i]
                        colormap);

            if (i == me)
            {
                V_DrawScaledPatch(x - (V_patch(stpb)->width/2),
                            DM_MATRIXY - WI_SPACINGY,
                            dead_face);

                V_DrawScaledPatch(DM_MATRIXX - (V_patch(stpb)->width/2),
                            y,
                            pl_face);
            }
        }
        else
        {
            // V_DrawPatch(x - (V_patch(bp[i])->width/2),
            //   DM_MATRIXY - WI_SPACINGY, FB, bp[i]);
            // V_DrawPatch(DM_MATRIXX - (V_patch(bp[i])->width/2),
            //   y, FB, bp[i]);
        }
        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = V_patch(num[0])->width;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
            {
                if (playeringame[j])
                    WI_Draw_Num(x+w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }
            WI_Draw_Num(DM_TOTALSX+w, y, dm_totals[i], 2);
        }
        y += WI_SPACINGY;
    }
}

*/

static int      cnt_frags[MAXPLAYERS];
static int      ng_state;
static byte     dofrags;

// Called by WI_Start
static void WI_Init_NetgameStats(void)
{
    int i;
    int cnt_playfrags = 0;

    state = StatCount;
    accelerate_stage = 0;
    ng_state = 1;

    effect_timer = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

        if (!playeringame[i])
            continue;

        cnt_playfrags += ST_PlayerFrags(i);
    }

    dofrags = (cnt_playfrags > 0);

    WI_Init_AnimatedBack();
}



static void WI_update_NetgameStats(void)
{

    int  i, cnt_target;
    boolean     stillticking = false;

    if (accelerate_stage && ng_state != 10)
    {
        accelerate_stage = 0;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] = ( wbs->maxkills > 0 ) ?
               (wb_plyr[i].skills * 100) / wbs->maxkills : -100;
            cnt_items[i] = ( wbs->maxitems > 0 ) ?
               (wb_plyr[i].sitems * 100) / wbs->maxitems : -100;
            cnt_secret[i] = ( wbs->maxsecret > 0 ) ?
               (wb_plyr[i].ssecret * 100) / wbs->maxsecret : -100;

            if (dofrags)
                cnt_frags[i] = ST_PlayerFrags(i);
        }
        S_StartSound(sfx_barexp);
        ng_state = 10;
    }

    if (ng_state == 2)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            if( wbs->maxkills <= 0 )
            {
               // no kills
               cnt_kills[i] = -100;
               continue;
            }

            cnt_target = (wb_plyr[i].skills * 100) / wbs->maxkills;
            cnt_kills[i] += 2;
            if (cnt_kills[i] >= cnt_target)
                cnt_kills[i] = cnt_target;
            else
                stillticking = true;
        }

        if (!stillticking)
            goto next_state;
    }
    else if (ng_state == 4)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            if( wbs->maxitems <= 0 )
            {
               // no items
               cnt_items[i] = -100;
               continue;
            }

            cnt_target = (wb_plyr[i].sitems * 100) / wbs->maxitems;
            cnt_items[i] += 2;
            if (cnt_items[i] >= cnt_target)
                cnt_items[i] = cnt_target;
            else
                stillticking = true;
        }
        if (!stillticking)
            goto next_state;
    }
    else if (ng_state == 6)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            if( wbs->maxsecret <= 0 )
            {
               // no secrets
               cnt_secret[i] = -100;
               continue;
            }

            cnt_target = (wb_plyr[i].ssecret * 100) / wbs->maxsecret;
            cnt_secret[i] += 2;
            if (cnt_secret[i] >= cnt_target)
                cnt_secret[i] = cnt_target;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            // skip ng_state 8 if no frags
            if ( !dofrags )  ng_state += 2;
            goto next_state;
        }
    }
    else if (ng_state == 8)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_target = ST_PlayerFrags(i);
            cnt_frags[i] += 1;
            if (cnt_frags[i] >= cnt_target)
                cnt_frags[i] = cnt_target;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(sfx_pldeth);
            ng_state++;
            goto done;
        }
    }
    else if (ng_state == 10)
    {
        if (accelerate_stage)
        {
            S_StartSound(sfx_sgcock);
            if ( gamemode == doom2_commercial || finale_after_intermission
                 || single_level_mode )
                WI_Init_NoState();   // [Arcade] no next location, see WI_update_Stats
            else
                WI_Init_ShowNextLoc();
        }
        goto done;
    }
    else if (ng_state & 1)
    {
        if ( --effect_timer == 0 )
        {
            ng_state++;
            effect_timer = TICRATE;
        }
        goto done;
    }

    // tick sound, a single place to change it
    if (!(bcnt&3))
        S_StartSound(sfx_pistol);
    return;
   
next_state:
    S_StartSound(sfx_barexp);
    ng_state++;
done:
    return;
}


#define NETGAME_STAT_148

// Called by WI_Drawer
static void WI_Draw_NetgameStats(void)
{
    // Hardware or software render.
    int  i, x, y, y10;
    int  pwidth, ngsx;

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    WI_Draw_LF();

    ngsx = NG_STATSX + (V_patch(pl_face)->width/2) + (dofrags? 0 : 32);
    // draw stat titles (top line)
    if( FontBBaseLump )
    {
        // use FontB if any
        V_DrawTextB("Kills", ngsx +  NG_SPACINGX - V_TextBWidth("Kills"), NG_STATSY);
        V_DrawTextB("Items", ngsx + 2*NG_SPACINGX - V_TextBWidth("Items"), NG_STATSY);
        V_DrawTextB("Scrt", ngsx + 3*NG_SPACINGX - V_TextBWidth("Scrt"), NG_STATSY);
        if (dofrags)
            V_DrawTextB("Frgs", ngsx + 4*NG_SPACINGX - V_TextBWidth("Frgs"), NG_STATSY);

        y = NG_STATSY + V_TextBHeight("Kills");
    }
    else
    {
        V_DrawScaledPatch(ngsx + NG_SPACINGX - (V_patch(kills)->width),
            NG_STATSY, kills);
        
        V_DrawScaledPatch(ngsx + 2*NG_SPACINGX - (V_patch(items)->width),
            NG_STATSY, items);
        
        V_DrawScaledPatch(ngsx + 3*NG_SPACINGX - (V_patch(secret)->width),
            NG_STATSY, secret);
        if (dofrags)
            V_DrawScaledPatch(ngsx + 4*NG_SPACINGX - (V_patch(frags)->width),
                              NG_STATSY, frags);
        // draw stats
        y = NG_STATSY + (V_patch(kills)->height);
    }


#ifdef NETGAME_STAT_148
    char  buf[33];
    const byte namex = 4;
    byte name_width = ngsx - namex;
    if( name_width > 32 ) name_width = 32;

    byte colornum;
    if( EN_heretic )
        colornum = 230;
    else
        colornum = 0x78;
#endif
   
    pwidth = V_patch(percent)->width;
    //added:08-02-98: p[i] replaced by stpb (see WI_Load_Data for more)
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        byte skin_color = players[i].skincolor;
        int ds_att = 0;
        x = ngsx;
        y10 = y+10;

#ifdef NETGAME_STAT_148
        byte color = (skin_color) ?
           SKIN_TO_SKINMAP(skin_color)[ colornum ]
         : reg_colormaps[ colornum ];  // default green skin

        V_DrawScaledFill( namex,y10, ngsx-namex,10, color );  // color bar
       
        if (i == me)
        {
//            V_DrawScaledFill (namex, y10+2, ngsx-namex+10,6, color);  // me, mark
            V_DrawScaledFill (namex, NG_STATSY+2, 42,10, color);  // me, shoulder banner under face
            V_DrawScaledPatch(namex+4, (NG_STATSY + 8 - V_patch(pl_face)->height), pl_face);  // face
            ds_att = V_WHITEMAP;
        }
       
        // draw name, truncate to colwidth
        memset(buf, ' ', 32);  // to defeat string centering
#if defined( __GNUC__ ) && ( __GNUC__ >= 12 )
        // [WDJ] GCC introduced their second guessing of every usage in ver 7.1, and since then
        // every programmer has to circumvent it.
        // With every version of GCC, I have to fix this code again, and it was never broken in the first place.
        // This truncation is intentional as we cannot ever supply an infinite buffer for arbitrary names.
        // Because we support other compilers, which behave better, this cannot just be thrown in the compile flags.
//# pragma GCC diagnostic push
# pragma GCC diagnostic ignored	  "-Wformat-truncation"
        snprintf(buf, 31, "%s", player_names[i] );
//# pragma GCC diagnostic pop
#else
        snprintf(buf, 31, "%s", player_names[i] );
#endif
        buf[name_width] = 0;  // truncate to column width
        V_DrawString(namex+1, y10+1, ds_att, buf);
#else
     // previous to 1.48
        byte*  colormap;   //added:08-02-98: remap STBP0 to player color
        colormap = (skin_color) ?
             SKIN_TO_SKINMAP( skin_color ) // skins 1..
           : & reg_colormaps[0]; // no translation table for green guy

     // color patch is too large, face is too large, much overlap
        V_DrawMappedPatch(x - (V_patch(stpb)->width), y, stpb, colormap);

        if (i == me)
            V_DrawScaledPatch(x - (V_patch(stpb)->width), y, pl_face);
#endif

        x += NG_SPACINGX;
        WI_Draw_Percent(x-pwidth, y10, cnt_kills[i]);
        x += NG_SPACINGX;
        WI_Draw_Percent(x-pwidth, y10, cnt_items[i]);
        x += NG_SPACINGX;
        WI_Draw_Percent(x-pwidth, y10, cnt_secret[i]);
        x += NG_SPACINGX;

        if (dofrags)
            WI_Draw_Num(x, y10, cnt_frags[i], -1);

        y += WI_SPACINGY;
    }

}

static int sp_state;

// Called by WI_Start
static void WI_Init_Stats(void)
{
    state = StatCount;
    accelerate_stage = 0;
    sp_state = 1;
    cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
    cnt_time = cnt_par = -1;
    effect_timer = TICRATE;

    // [Arcade] Single-player only (this is WI_Start's non-deathmatch,
    // non-coop branch). wb_plyr[me].stime is this level's leveltime.
    // A "max" exit is 100% kills and 100% secrets; items are not required.
    // A map with none of a category (max* <= 0) counts as satisfied -- the
    // percentage display treats those the same way (see the -100 cases).
    // [Arcade] all_kills is tyson's per-level condition: 100% kills, secrets
    // not required.  Split out rather than recomputed at the call site so the
    // two categories cannot drift apart, the same reason sp_maxed exists.
    boolean all_kills =
        ( wbs->maxkills  <= 0 || wb_plyr[me].skills  >= wbs->maxkills );
    boolean maxed = all_kills
     && ( wbs->maxsecret <= 0 || wb_plyr[me].ssecret >= wbs->maxsecret );
    sp_maxed = maxed;   // [Arcade] for the MAX indicator, see WI_Draw_Stats
    HS_LevelExit( gameepisode, gamemap, gameskill, wb_plyr[me].stime, maxed,
                  all_kills );

    WI_Init_AnimatedBack();
}

static void WI_update_Stats(void)
{
    if (accelerate_stage && sp_state != 10)
    {
        accelerate_stage = 0;
        cnt_kills[0] = ( wbs->maxkills > 0 ) ?
           (wb_plyr[me].skills * 100) / wbs->maxkills : -100;
        cnt_items[0] = ( wbs->maxitems > 0 ) ?
           (wb_plyr[me].sitems * 100) / wbs->maxitems : -100;
        cnt_secret[0] = ( wbs->maxsecret > 0 ) ?
           (wb_plyr[me].ssecret * 100) / wbs->maxsecret : -100;
        cnt_time = wb_plyr[me].stime / TICRATE;
        cnt_par = wbs->partime / TICRATE;
        S_StartSound(sfx_barexp);
        sp_state = 10;
    }

    if (sp_state == 2)
    {
        if ( wbs->maxkills <= 0 )
        {
            cnt_kills[0] = -100;
            goto next_state;  // no kills
        }

        cnt_kills[0] += 2;

        if (cnt_kills[0] >= (wb_plyr[me].skills * 100) / wbs->maxkills)
        {
            cnt_kills[0] = (wb_plyr[me].skills * 100) / wbs->maxkills;
            goto next_state;
        }
    }
    else if (sp_state == 4)
    {
        if ( wbs->maxitems <= 0 )
        {
            cnt_items[0] = -100;
            goto next_state;  // no items
        }

        cnt_items[0] += 2;

        if (cnt_items[0] >= (wb_plyr[me].sitems * 100) / wbs->maxitems)
        {
            cnt_items[0] = (wb_plyr[me].sitems * 100) / wbs->maxitems;
            goto next_state;
        }
    }
    else if (sp_state == 6)
    {
        if ( wbs->maxsecret <= 0 )
        {
            cnt_secret[0] = -100;
            goto next_state;  // no secrets
        }

        cnt_secret[0] += 2;

        if (cnt_secret[0] >= (wb_plyr[me].ssecret * 100) / wbs->maxsecret)
        {
            cnt_secret[0] = (wb_plyr[me].ssecret * 100) / wbs->maxsecret;
            goto next_state;
        }
    }

    else if (sp_state == 8)
    {
        cnt_time += 3;

        if (cnt_time >= wb_plyr[me].stime / TICRATE)
            cnt_time = wb_plyr[me].stime / TICRATE;

        cnt_par += 3;

        if (cnt_par >= wbs->partime / TICRATE)
        {
            cnt_par = wbs->partime / TICRATE;

            if (cnt_time >= wb_plyr[me].stime / TICRATE)
                goto next_state;
        }
    }
    else if (sp_state == 10)
    {
        if (accelerate_stage)
        {
            S_StartSound(sfx_sgcock);

            // [Arcade] finale_after_intermission: the episode's last level
            // has no next location, so showing the "Entering ..." map would
            // point at E?M1.  Go straight to NoState, which is what
            // doom2_commercial already does.
            //
            // single_level_mode: same reasoning, for a different reason --
            // the run ends here by definition (G_DoWorldDone returns early),
            // so "Entering E1M2" names a level the player is not going to
            // and directly contradicts the mode they chose.  Doom II never
            // showed it, being doom2_commercial, so this only ever appeared
            // on the ExMy games.
            if (gamemode == doom2_commercial || finale_after_intermission
                || single_level_mode)
                WI_Init_NoState();
            else
                WI_Init_ShowNextLoc();
        }
        goto done;
    }
    else if (sp_state & 1)
    {
        if ( --effect_timer == 0 )
        {
            sp_state++;
            effect_timer = TICRATE;
        }
        goto done;
    }
   
    // tick sound, a single place to change it
    if (!(bcnt&3))
        S_StartSound(sfx_pistol);
    return;

next_state:
    // done incrementing the count
    S_StartSound(sfx_barexp);
    sp_state++;
done:
    return;
}

// Called by WI_Drawer
static void WI_Draw_Stats(void)
{
    // all WI is draw screen0, scale
    // [WDJ] Display PAR for certain id games, unless modified,
    // but not PWAD unless BEX has set PARS.
    boolean draw_pars = pars_valid_bex
     || ( EN_doom_etc
          && (gamedesc.gameflags & GD_idwad)
          && (wbs->epsd < 3)  // episodes 1 to 3 have animation
          && !modifiedgame );
    // Hardware or softare render.
    // line height
    int lh = (3 * (V_patch(num[0])->height))/2;

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    WI_Draw_LF();

    if( FontBBaseLump )
    {
        // use FontB if any
        V_DrawTextB("Kills", SP_STATSX, SP_STATSY);
        V_DrawTextB("Items", SP_STATSX, SP_STATSY+lh);
        V_DrawTextB("Secrets", SP_STATSX, SP_STATSY+2*lh);
        V_DrawTextB("Time", SP_TIMEX, SP_TIMEY);
        if (draw_pars)
            V_DrawTextB("Par", BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY);
    }
    else
    {
        V_DrawScaledPatch(SP_STATSX, SP_STATSY, kills);
        V_DrawScaledPatch(SP_STATSX, SP_STATSY+lh, items);
        V_DrawScaledPatch(SP_STATSX, SP_STATSY+2*lh, sp_secret);
        V_DrawScaledPatch(SP_TIMEX, SP_TIMEY, timePatch);
        if (draw_pars)
            V_DrawScaledPatch(BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY, par);
    }
    WI_Draw_Percent(BASEVIDWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);
    WI_Draw_Percent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+lh, cnt_items[0]);
    WI_Draw_Percent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);

    // [Arcade] Blinking MAX beside the two rows the max category is scored
    // on, when this level satisfied it -- 100% kills and 100% secrets, the
    // same test WI_Init_Stats hands to HS_LevelExit.  Items deliberately get
    // none: they are not part of the category, and the gap says so.
    //
    // Measured: the percentages are right-justified so their WIPCNT '%' patch
    // (13 wide) sits at BASEVIDWIDTH - SP_STATSX = 270 and ends at 283.
    // "MAX" is 26px against the real STCFN lumps, so at 287 it spans 287..313
    // of 320.  The percent patches are 12 tall and hu_font glyphs 7, so +2
    // centres the text on the row.  Option 0 is the font's native red, which
    // reads on this screen's grey where V_WHITEMAP would not.
    if( sp_maxed && (gametic & 16) )
    {
        V_DrawString( SP_MAXIND_X, SP_STATSY + 2,        0, "MAX" );
        V_DrawString( SP_MAXIND_X, SP_STATSY + 2*lh + 2, 0, "MAX" );
    }
    WI_Draw_Time(BASEVIDWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);

    if (draw_pars)
        WI_Draw_Time(BASEVIDWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);

    // [Arcade] Cumulative run time, tucked under the Time row and aligned to
    // its columns.  WITIME and the WINUM digits are both 12 tall and start at
    // SP_TIMEY, so that row ends at SP_TIMEY + 12; the + 16 leaves a 4px gap.
    // hu_font glyphs are 7 tall, so this occupies 184..191 of the 200 high
    // screen.  Only the Par row shares this band and it is right of centre.
    HS_Draw_TotalTime( SP_TIMEX, BASEVIDWIDTH/2 - SP_TIMEX, SP_TIMEY + 16 );

    // [Arcade] Best-time-per-skill table for the map just exited, centered in
    // the gap between the Secrets row and the Time row.  The +12 is needed
    // because the table draws its own header 14 above the y given here, which
    // otherwise lands on the Secrets percentage (lh is 18, the percent patches
    // are 12 tall, so that row ends at SP_STATSY + 2*lh + 12).
    // [Arcade] 128, not the original 156: the two time columns widened by
    // 40px when they went to hundredths (see HS_COL_TIME), and the Survival
    // block that replaced them ends in the initials, which are 27px at their
    // widest ("MMM"/"WWW") rather than the 24 of "AAA" it was measured with --
    // at 138 the last initial ran to 323 and was cut off by the right edge.
    // The block now spans 128..313 of BASEVIDWIDTH 320, and "NEW RECORD" is
    // still centred in the free space to its left (23..100).
    HS_Draw_IntermissionTable( 116, SP_STATSY + 3*lh + 12 );

    // [Arcade] PACIFIST and TYSON announce that the run just exited is still
    // holding those conditions.  Toward the top of the page, above the stats
    // block, because they describe the *whole run* rather than any one number
    // on it -- and because that is the only clear band left: SP_STATSX is 50
    // and the Kills row starts at SP_STATSY 50, so 50..49 is free full width.
    //
    // Blink on `gametic & 16`, matching NEW RECORD lower down, the MAX
    // indicator beside the percentages and PRESS FIRE on the attract screen,
    // so the whole cabinet flashes on one beat.  Option 0 is the font's
    // native red -- V_WHITEMAP is grey and vanishes into this background, the
    // rule the rest of this screen already follows.
    //
    // Two can be true at once (a tyson run that never fired at a monster is
    // both), so they are laid out as one centred line with a gap rather than
    // each centred on its own and overlapping.  Widths come from V_StringWidth
    // at draw time, so nothing here is measured by hand.
    if( (gametic & 16) && ! deathmatch && ! netgame )
    {
        const char * pac = HS_Run_Is_Pacifist() ? "PACIFIST" : NULL;
        const char * tys = HS_Run_Is_Tyson()    ? "TYSON"    : NULL;
        int  wp = pac ? V_StringWidth((char*)pac) : 0;
        int  wt = tys ? V_StringWidth((char*)tys) : 0;
        int  gap = (pac && tys) ? 12 : 0;
        int  x = (BASEVIDWIDTH - (wp + gap + wt)) / 2;

        if( pac )
            V_DrawString( x, SP_STATSY - 14, 0, (char*) pac );
        if( tys )
            V_DrawString( x + wp + gap, SP_STATSY - 14, 0, (char*) tys );
    }
}

// Called by WI_Ticker
static void WI_checkForAccelerate(void)
{
    int   i;
    player_t * player;

    // check for button presses to skip delays
    for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
    {
        if (playeringame[i])
        {
            byte gb = player->GB_flags;
            if (player->cmd.buttons & BT_ATTACK)
            {
                if( ! (gb & GB_attackdown) )
                    accelerate_stage = 1;
                player->GB_flags |= GB_attackdown;
            }
            else
                player->GB_flags &= ~(GB_attackdown);

            if (player->cmd.buttons & BT_USE)
            {
                if( ! (gb & GB_usedown) )
                    accelerate_stage = 1;
                player->GB_flags |= GB_usedown;
            }
            else
                player->GB_flags &= ~(GB_usedown);
        }
    }
}



// Updates stuff each client tick.
void WI_Ticker(void)
{

    // counter for general background animation
    bcnt++;

    if (bcnt == 1)
    {
        // intermission music
        if ( gamemode == doom2_commercial )
          S_ChangeMusic(mus_dm2int, true);
        else
          S_ChangeMusic(mus_inter, true);
    }

    WI_checkForAccelerate();
    WI_update_AnimatedBack();

    switch (state)
    {
      case StatCount:
        if( deathmatch )
            break;

        if (multiplayer)  // coop
            WI_update_NetgameStats();
        else
            WI_update_Stats();
        break;

      case ShowNextLoc:
        WI_update_ShowNextLoc();
        break;

      case NoState:  // transition to next level
        if( --cnt == 0 )
        {
            WI_Release_Data();
            G_NextLevel();
        }
        break;
    }
}

// [WDJ] Patch lists.

byte doom_wi_patches_loaded = 0;
load_patch_t  doom_wi_patches[] =
{
  { &finished, "WIF" },
  { &entering, "WIENTER" },
  { &kills, "WIOSTK" },
  { &secret, "WIOSTS" },
  { &sp_secret, "WISCRT2" },
  { &items, "WIOSTI" },
  { &frags, "WIFRGS" },
  { &timePatch, "WITIME" },
  { &sucks, "WISUCKS" },
  { &par, "WIPAR" },
  { &killers, "WIKILRS" },  // vertical
  { &victims, "WIVCTMS" },  // horiz
  { &total, "WIMSTT" },
  { &yah[0], "WIURH0" },   // yah point RH
  { &yah[1], "WIURH1" },   // yad point LH
  { &yah[2], "WISPLAT" },  // yah splat
  { &colon, "WICOLON" },
  { &percent, "WIPCNT" },
  { &wiminus, "WIMINUS" },
  { NULL, NULL }
};


byte heretic_wi_patches_loaded = 0;
load_patch_t  heretic_wi_patches[13] =
{
  { &yah[0], "IN_YAH" }, // yah point
  { &yah[2], "IN_X" },  // yah splat
  { &colon, "FONTB26" },
  { &percent, "FONTB05" },
  { &wiminus, "FONTB13" },
  { NULL, NULL }
};

     

// Called by WI_Start, SCR_SetMode
void WI_Load_Data(void)
{
    // vid : from video setup
    int   i;
    anim_inter_t*  ai; // interpic animation data
    // [Stylinski] Compiler warns buffer overrun, requires [17], maybe up to [27].
    char  name[28];
    byte  j;
    byte  wb_epsd;

    if( wbs == NULL )  return;

    // To support entering Intermission without entering level.
    if( info_interpic == NULL )   info_interpic = "";
   
    // [WDJ] Lock the interpic graphics against release by other users.

    wb_epsd = wbs->epsd;
    // choose the background of the intermission
    if (*info_interpic)  // if not empty string
        strcpy(bgname, info_interpic);
    else if (gamemode == doom2_commercial)
        strcpy(bgname, "INTERPIC");
    else if( gamemode == heretic )
        sprintf(bgname, "MAPE%d", wb_epsd+1);
    else
        sprintf(bgname, "WIMAP%d", wb_epsd);

    if ( gamemode == ultdoom_retail )
    {
        if (wb_epsd == 3)
            strcpy(bgname,"INTERPIC");
    }

#ifdef ENABLE_UMAPINFO
    WI_Prepare_Background();
#else
    if( rendermode == render_soft )
    {
        memset(screens[0], 0, vid.screen_size);

        // clear backbuffer from status bar stuff and borders
        memset(screens[1], 0, vid.screen_size);
  
        // Draw background on screen1
        // [Arcade] V_SCALEEXACT: cover the screen, as the attract pages do.
        // Same 320x200-into-a-whole-multiple letterbox otherwise -- see
        // docs/arcade/screen-fill.md.
        V_SetupDraw( 1 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ | V_SCALEEXACT ); // screen 1
        V_DrawScaledPatch(0, 0, W_CachePatchName(bgname, PU_CACHE));
        V_SetupDraw( drawinfo.prev_screenflags );  // restore
    }
#endif

    // UNUSED unsigned char * pic = screens[1];
    // if (gamemode == doom2_commercial)
    // {
    // darken the background image
    // while (pic != screens[1] + SCREENHEIGHT*SCREENWIDTH)
    // {
    //   *pic = colormaps[256*25 + *pic];
    //   pic++;
    // }
    //}

    if (gamemode == doom2_commercial)
    {
        num_lnames = 32;
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * num_lnames,
                                       PU_STATIC, 0);
        for (i=0 ; i<num_lnames ; i++)
        {
            sprintf(name, "CWILV%2.2d", i);
            lnames[i] = W_CachePatchName(name, PU_LOCK_SB);
        }
    }
    else
    {
        // doom1, doom, doomu
        num_lnames = NUM_MAPS_PER_EPI;
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUM_MAPS_PER_EPI,
                                       PU_STATIC, 0);
        for (i=0 ; i<NUM_MAPS_PER_EPI ; i++)
        {
            sprintf(name, "WILV%d%d", wb_epsd, i);
            lnames[i] = W_CachePatchName(name, PU_LOCK_SB);
        }

        if (wb_epsd < 3)
        {
            for (j=0; j<num_anim[wb_epsd]; j++)
            {
                ai = &anim_inter_info[wb_epsd][j];
                for (i=0; i<ai->num_anims; i++)
                {
                    if(wb_epsd == 1 && j == 8)  // shares
                    {
                        // [1][8] shares the patch of [1][4]
                        ai->p[i] = anim_inter_info[1][4].p[i];
                        continue;
                    }

                    // animations
                    sprintf(name, "WIA%d%.2d%.2d", wb_epsd, j, i);
                    ai->p[i] = W_CachePatchName(name, PU_LOCK_SB);
                }
            }
        }
    }

    for (i=0;i<10;i++)
    {
         // numbers 0-9
        if( EN_heretic )
            sprintf(name, "FONTB%d", 16+i);
        else
            sprintf(name, "WINUM%d", i);
        num[i] = W_CachePatchName(name, PU_LOCK_SB);
    }

    if( EN_doom_etc )
    {
        load_patch_list( doom_wi_patches );
        doom_wi_patches_loaded = 1;
    }
    else if( EN_heretic )
    {
        load_patch_list( heretic_wi_patches );
        heretic_wi_patches_loaded = 1;
    }
    
    // your face
    pl_face = W_CachePatchName("STFST01", PU_LOCK_SB);  // never unlocked

    // dead face
    dead_face = W_CachePatchName("STFDEAD0", PU_LOCK_SB);  // never unlocked


    //added:08-02-98: now uses a single STPB0 which is remapped to the
    //                player translation table. Whatever new colors we add
    //                since we'll have to define a translation table for
    //                it, we'll have the right colors here automatically.
    stpb = W_CachePatchName("STPB0", PU_LOCK_SB);  // never unlocked
}

// Called by  WI_update_NoState, SCR_SetMode
void WI_Release_Data(void)
{
    byte j;
    byte wb_epsd;

    //faB: never Z_ChangeTag() a pointer returned by W_CachePatchxxx()
    //     it doesn't work and is unecessary
    if( lnames )
    {
      release_patch_array( num, 10 );
      release_patch_array( lnames, num_lnames );

      Z_Free(lnames);
      lnames = NULL;

      if( (gamemode != doom2_commercial) && wbs )
      {
        wb_epsd = wbs->epsd;
        if (wb_epsd < 3)  // episodes 1 to 3 have animation
        {
            for (j=0; j<num_anim[wb_epsd]; j++)
            {
                if(wb_epsd == 1 && j == 8)  continue;  // shared

                release_patch_array( anim_inter_info[wb_epsd][j].p,
                                     anim_inter_info[wb_epsd][j].num_anims );
            }
        }
      }
    }

    if( doom_wi_patches_loaded )
    {
        release_patch_list( doom_wi_patches );
        doom_wi_patches_loaded = 0;
    }

    if( heretic_wi_patches_loaded )
    {
        release_patch_list( heretic_wi_patches );
        heretic_wi_patches_loaded = 0;
    }
}

void WI_Drawer (void)
{
    // all WI is draw screen0, scale
    // [Arcade] V_SCALEEXACT, matching the background above and the attract
    // pages: the whole intermission -- the level name, the Time/TOTAL rows and
    // the arcade record tables on it -- is one 320x200 page, and stock drew it
    // into a whole multiple of that with the rest of the screen left black.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ | V_SCALEEXACT );

    switch (state)
    {
      case StatCount:
        if( deathmatch )
        {
            if( cv_teamplay.EV )
                WI_Draw_TeamsStats();
            else
                WI_Draw_DeathmatchStats();
        }
        else if (multiplayer)  // coop
            WI_Draw_NetgameStats();
        else
            WI_Draw_Stats();
        break;

      case ShowNextLoc:
        WI_Draw_ShowNextLoc();
        break;

      case NoState:
        WI_Draw_NoState();
        break;
    }
   
    if( wait_game_start_timer )
        WI_Draw_wait( 0, 0, 0, wait_game_start_timer );
}


static void WI_Init_Variables( wb_start_t * wb_start)
{

    wbs = wb_start;

#ifdef RANGECHECK_XXX
// [WDJ] Many maps will violate these assumptions.
// [WDJ] Verified that doom2 will call here with maps 1..32.     
// printf( "WI RANGECHECK: episode=%i, lev_prev=%i, lev_next=%i\n", wbs->epsd, wbs->lev_prev, wbs->lev_next );
    int map_max = 8;  // This has been 8, (checked 1.43 and in prboom), 0 based so 8 means 9 episodes.
    int episode_max = 0;
    if (gamemode == doom2_commercial)
    {
        map_max = 33;  // Doom2 maps (normally 1..32), DoomII had 34 maps in one episode.
        episode_max = 0; // stored 0 based,  FIXME
    }
    else
    {
        // Doom1 and Heretic
        // 4 episodes, 9 maps per episode.
        map_max = 8;  // 0..8, as 0 based index
        episode_max = ( gamemode == ultdoom_retail )? 3 : 2;  // 0..3, as 0 based index
# ifdef ENABLE_UMAPINFO
        if( game_umapinfo )  episode_max = 254;  // 0..254
# endif
    }
# ifdef ENABLE_UMAPINFO
    if( game_umapinfo )  map_max = 254;  // 0..254
# endif
    detect_range_violation(wbs->epsd, 0, episode_max);
    detect_range_violation(wbs->lev_prev, 0, map_max);
    detect_range_violation(wbs->lev_next, 0, map_max);
    detect_range_violation(wbs->pnum, 0, MAXPLAYERS);
//    detect_range_violation(wbs->pnum, 0, MAXPLAYERS);  // duplicate, appears in PrBoom too
#endif

    accelerate_stage = 0;
    cnt = bcnt = 0;
    first_refresh = 1;
    me = wbs->pnum;
    wb_plyr = wbs->plyr;

    if ( gamemode != ultdoom_retail )
    {
      if (wbs->epsd > 2) // 0 based
        wbs->epsd -= 3;
    }
}

void WI_Start(wb_start_t * wb_start)
{
    WI_Init_Variables(wb_start);
    WI_Load_Data();

    if( deathmatch )
    {
        WI_Init_DeathmatchStats();
        wait_game_start_timer = TICRATE*DM_WAIT;
    }
    else if (multiplayer)  // coop
    {
        WI_Init_NetgameStats();
        // wait_game_start_timer will be set by network
    }
#ifdef ENABLE_UMAPINFO
    // [MB] 2023-03-29: Support for UMAPINFO added
    else if( game_umapinfo && (game_umapinfo->flags & UMA_nointermission) )
    {
        if ( gamemode == doom2_commercial )
            WI_Init_NoState();
        else
            WI_Init_ShowNextLoc();
    }
#endif
    else
        WI_Init_Stats();
}
