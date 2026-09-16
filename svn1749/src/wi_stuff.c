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



// [Arcade] ---- Intermission tables for more players than the classic
// layouts hold. ----
//
// DoomLegacy allows MAXPLAYERS=32, but neither intermission table was built
// for that many and neither said anything about the ones it left out.  The
// netgame table steps 16 base units per player from y 62 and its percentage
// patches are 12 tall, so player 9's row starts below the 200-line screen and
// is simply not drawn; the deathmatch rankings step 12 from y 60 and break out
// of the loop at the screen edge after 12.  The rankings are sorted highest
// first, so what went missing there was the bottom of the scoreboard.
//
// The fix is a compact fallback, NOT a replacement: the small hu_font in place
// of the WINUM patches, 8-unit rows, and a second column when one will not
// hold everyone.  It engages only when the classic layout cannot fit the
// players present, so 8 in a netgame and 12 in a deathmatch look exactly as
// they always did.
//
// A compact row is a 9-unit colour bar (one more than the pitch, so the bars
// of a column touch, as they do at the classic pitch).

#define WI_C_PITCH      8       // base units between compact rows
#define WI_C_ROW_H      9       // height of a compact row's colour bar
#define WI_C_BOTTOM   199       // last base line a compact row may touch

// How many compact rows fit in one column, from ytop down.
static int WI_Compact_Rows( int ytop )
{
    int  rows = ((WI_C_BOTTOM - (WI_C_ROW_H - 1) - ytop) / WI_C_PITCH) + 1;
    return (rows < 1) ? 1 : rows;
}

// [Arcade] Truncate a name to the widest prefix that fits max_w base units.
//
// The classic tables truncate to a character count, which is only ever right
// for one string: hu_font is proportional, 'M' and 'W' are 9 units and 'I' is
// 4, so six characters is anything from 24 to 54 units.  The compact columns
// have no spare width to absorb that -- and the classic 4th ranking column at
// x 245 already runs 8 units off the right of the screen with six 'M's in it.
// Measure instead.  dest must hold destsize bytes, and may be name itself:
// every character is read before the byte at that same index is written.
static void WI_Fit_Name( char * dest, int destsize, const char * name, int max_w )
{
    char  cb[2] = { 0, 0 };
    int   w = 0, n = 0;

    while( name[n] && (n < destsize - 1) )
    {
        cb[0] = name[n];
        w += V_StringWidth( cb );
        if( w > max_w )  break;
        dest[n] = name[n];
        n++;
    }
    dest[n] = 0;
}

// [Arcade] Draw a string with its right edge at x_right.
static void WI_Draw_String_RJ( int x_right, int y, int att, const char * str )
{
    V_DrawString( x_right - V_StringWidth(str), y, att, str );
}

// [Arcade] Rows per sub-column of a ranking table, balanced -- two half-full
// columns rather than one full column and a stub.  A table shorter than
// max_rows stays a single column, so the small team table is not split in
// two.  max_rows 0 never wraps, which is the classic table.
//   Its own function so tools/interfit-test.py can check the capacity this
// gives against the width WI_Rank_Fit hands out for it.
static int WI_Rank_Rows( int scorelines, int max_rows )
{
    int  rows = scorelines;

    if( (max_rows > 0) && (rows > max_rows) )
    {
        int  ncol = (scorelines + max_rows - 1) / max_rows;
        rows = (scorelines + ncol - 1) / ncol;
    }
    return (rows < 1) ? 1 : rows;
}


//  Quick-patch for the Cave party 19-04-1998 !!
//
//  width : the column width
// [Arcade] y_limit: base-unit y at which to stop, because the caller may have
// offset this block into a viewport cell.  The bottom-row cells of a 2x2 sit
// past base y 200, so the old hardcoded BASEVIDHEIGHT test broke out after a
// single row -- which read as "the players on 0 points are missing", since
// the table is sorted highest first.
// [Arcade] Where a compact ranking's colour bar, count and name sit inside a
// sub-column, given the widest count that table will draw.
//   Its own function so tools/interfit-test.py can drive it; the measuring
// loop that feeds it lives in the drawer, which is where the counts are.
#define WI_C_NAME_MIN  34       // the name never gets less than this

typedef struct {
    int  bar_w;     // colour bar, behind the count
    int  num_x;     // right edge of the count, from the row x
    int  name_x;
    int  name_w;
} wi_rankcol_t;

#define WI_C_NUM_PAD    4       // colour bar overhang, plus the gap to the name

static void WI_Rank_Col_Fit( int sub_w, int num_w, wi_rankcol_t * out )
{
    int  floor_w = V_StringWidth("88");   // so a table of zeroes is not hairline
    // A count can in principle be enormous -- Buchholz multiplies frag counts
    // together -- and the name must not be squeezed out altogether.  Past
    // this the number runs into the name, which is what the classic table has
    // always done with anything over three digits.
    int  ceil_w  = sub_w - WI_C_NAME_MIN - WI_C_NUM_PAD;

    if( num_w > ceil_w )   num_w = ceil_w;
    if( num_w < floor_w )  num_w = floor_w;

    out->num_x  = num_w;
    out->bar_w  = num_w + 2;
    out->name_x = num_w + WI_C_NUM_PAD;
    out->name_w = sub_w - out->name_x;
}

// [Arcade] pitch, max_rows, col_dx, sub_w: the compact layout's extras, see
// the block above.  The table is laid out column-major -- one sub-column filled
// top to bottom, then the next -- so the sort order still reads downwards.
//   pitch    : base units between rows (12 is the classic pitch)
//   max_rows : wrap into another sub-column past this many rows, 0 to never
//              wrap (the classic table).  A table shorter than this stays a
//              single column, so the small team table is not split in two.
//   col_dx   : base units from one sub-column to the next
//   sub_w    : usable width of a sub-column, 0 for the classic fixed layout
static
void WI_Draw_Ranking_Cols(const char * title, int x, int y, fragsort_t * fragtable,
                    int scorelines, boolean large, int white, int colwidth,
                    int y_limit, int pitch, int max_rows, int col_dx, int sub_w)
{
    char  buf[33];
    int   i,j;
    int   skin_color, color;
    int   plnum;
    int   frags;
    int   colornum;
    int   rows;
    int   bar_w  = large ? 40 : 26;   // the colour bar, behind the count
    int   num_x  = large ? 32 : 24;   // right edge of the count, from the row x
    int   name_x = large ? 64 : 29;
    int   name_w = 0;                 // 0: truncate by colwidth, as ever
    fragsort_t temp;


    if( EN_heretic )
        colornum = 230;
    else
        colornum = 0x78;

    if( colwidth > 32 )  colwidth=32;

    rows = WI_Rank_Rows( scorelines, max_rows );

    if( sub_w > 0 )
    {
        // [Arcade] Compact: size the count field to the widest count actually
        // in this table rather than to three digits, and give what that saves
        // to the names.  With 32 players on two digit frags that is the
        // difference between "PLAYER" thirty-two times and "PLAYER12" -- two
        // of hu_font's 8-unit digits is most of another character.
        //   The classic table keeps its "%3i": V_StringWidth charges 4 units
        // for each padding space, so the format is part of where its numbers
        // sit and changing it would move them.
        wi_rankcol_t  rc;
        int  num_w = 0;

        for (i=0; i<scorelines; i++)
        {
            int  w;
            sprintf(buf, "%i", fragtable[i].count);
            w = V_StringWidth(buf);
            if( w > num_w )  num_w = w;
        }
        WI_Rank_Col_Fit( sub_w, num_w, &rc );
        bar_w  = rc.bar_w;
        num_x  = rc.num_x;
        name_x = rc.name_x;
        name_w = rc.name_w;
    }

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
        // [Arcade] Column-major placement.  With the classic ncol=1 this is
        // the old running y and x, one row per iteration.
        int  cx = x + ((i / rows) * col_dx);
        int  cy = y + ((i % rows) * pitch);

        if (cy >= y_limit)
            continue;         // dont draw past the bottom of this view

        frags = fragtable[i].count;
        plnum = fragtable[i].num;

        // draw color background
        skin_color = fragtable[i].color;
        color = (skin_color) ?
           SKIN_TO_SKINMAP(skin_color)[ colornum ]
         : reg_colormaps[ colornum ];  // default green skin
        V_DrawScaledFill (cx-1,cy-1, bar_w,9, color);

        // draw frags count, right justified
        sprintf(buf, (sub_w > 0) ? "%i" : "%3i", frags );
        V_DrawString (cx+num_x-V_StringWidth(buf), cy, 0, buf);

        // draw name, truncate to colwidth
        memset(buf, ' ', 32);  // to defeat string centering
        snprintf(buf, 31, "%s", fragtable[i].name );
        if( name_w > 0 )
            WI_Fit_Name( buf, sizeof(buf), buf, name_w );  // to the real width
        else
            buf[colwidth] = 0;  // truncate to column width
        V_DrawString (cx+name_x, cy,
                      ((plnum == white) ? V_WHITEMAP : 0), buf);
    }
}

// The classic single-column table, at the classic 12-unit pitch.
void WI_Draw_Ranking(const char * title, int x, int y, fragsort_t * fragtable,
                    int scorelines, boolean large, int white, int colwidth,
                    int y_limit)
{
    WI_Draw_Ranking_Cols( title, x, y, fragtable, scorelines, large, white,
                          colwidth, y_limit, 12, 1, 0, 0 );
}

#define RANKINGY 60
// [Arcade] The team tables have always started 20 units lower than the
// deathmatch ones; named so the layout can be told which it is dealing with.
#define TEAMRANKINGY 80

// [Arcade] ---- How the ranking tables are laid out for this many players. ----
//
// Compact sub-columns are 79 base units apart from x 4, so the fourth ends at
// 241 and its name field at 319 -- inside the 320-unit screen, which the
// classic fourth column at x 245 is not once a name is six 'M's wide.
#define WI_RANK_X0      4
#define WI_RANK_DX     79
#define WI_RANK_SUB_W  78       // usable width of a sub-column

typedef struct {
    boolean  compact;    // small rows
    byte     ntable;     // ranking tables drawn: the usual 4, or 2 when each
                         // needs two sub-columns to hold everyone
    int      pitch;
    int      max_rows;   // rows per sub-column, 0 for the classic single column
    int      col_dx;
    int      sub_w;      // usable width of one sub-column, 0 for classic
    int      y_limit;
    int      x[4];       // left edge of table 0..ntable-1
} wi_rankfit_t;

// Pure arithmetic, so tools/interfit-test.py can lift it out and check every
// player count against the screen.
//   num_pl : lines the tallest of the tables will hold
//   ytop   : base y of the first row.  The deathmatch tables start at 60, the
//            team tables at 80, so they do not hold the same number of rows.
static void WI_Rank_Fit( int num_pl, int ytop, wi_rankfit_t * out )
{
    int  i, per_col, ncol;
    // Rows the classic table holds: it steps 12 from ytop and breaks once y
    // has reached the bottom of the screen, so the last row starts above it.
    int  classic_rows = (BASEVIDHEIGHT - ytop + 11) / 12;

    // Everybody fits the classic table, so nothing changes.
    if( num_pl <= classic_rows )
    {
        out->compact  = false;
        out->ntable   = 4;
        out->pitch    = 12;
        out->max_rows = 0;
        out->col_dx   = 0;
        out->sub_w    = 0;
        out->y_limit  = BASEVIDHEIGHT;
        out->x[0] = 5;  out->x[1] = 85;  out->x[2] = 165;  out->x[3] = 245;
        return;
    }

    per_col = WI_Compact_Rows( ytop );          // 17 from y 60, 14 from y 80
    ncol    = (num_pl + per_col - 1) / per_col; // sub-columns each table needs

    out->compact  = true;
    out->pitch    = WI_C_PITCH;
    out->max_rows = per_col;
    out->col_dx   = WI_RANK_DX;
    out->sub_w    = WI_RANK_SUB_W;
    out->y_limit  = ytop + (per_col * WI_C_PITCH);

    // There are four sub-column widths across the screen to share out.  Each
    // table needs ncol of them, so the tables that fit are 4/ncol -- and the
    // ones that drop out are Buchholz and indiv. first.  They are tie-break
    // curiosities, while Frags and deads answer "how did I do"; a scoreboard
    // that silently omits half the players is worse than one that omits two
    // of its four rankings.
    out->ntable = 4 / ncol;
    if( out->ntable < 1 )  out->ntable = 1;
    for( i = 0; i < 4; i++ )
        out->x[i] = WI_RANK_X0 + (i * ncol * WI_RANK_DX);
}

// Called by WI_Drawer
static void WI_Draw_DeathmatchStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    int          num_pl = 0;
    wi_rankfit_t fit;
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

    // [Arcade] Pick the layout from the head count, before any table is drawn.
    for (i=0; i<MAXPLAYERS; i++)
        if (playeringame[i])  num_pl++;
    WI_Rank_Fit( num_pl, RANKINGY, &fit );

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
    WI_Draw_Ranking_Cols("Frags", fit.x[0], RANKINGY, fragtab, scorelines, false,
                    whiteplayer, 6, fit.y_limit,
                    fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);

    // [Arcade] Buchholz and indiv. are drawn only while there is width for
    // them; past 17 players their two columns go to the second half of the
    // Frags and deads tables.
    if( fit.ntable == 4 )
    {
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
        WI_Draw_Ranking_Cols("Buchholz", fit.x[1], RANKINGY, fragtab, scorelines, false,
                        whiteplayer, 6, fit.y_limit,
                        fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);

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
        WI_Draw_Ranking_Cols("indiv.", fit.x[2], RANKINGY, fragtab, scorelines, false,
                        whiteplayer, 6, fit.y_limit,
                        fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);
    }

    // count deads
    if( fit.ntable >= 2 )
    {
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
        WI_Draw_Ranking_Cols("deads", fit.x[fit.ntable - 1], RANKINGY, fragtab, scorelines,
                        false, whiteplayer, 6, fit.y_limit,
                        fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);
    }
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
    int          num_teams = 0;
    wi_rankfit_t fit;
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

    // [Arcade] Pick the layout from the team count, before any table is drawn.
    // These tables start at y 80, not 60, so they hold two rows fewer than the
    // deathmatch ones -- WI_Rank_Fit is told where they start rather than
    // assuming.
    for (i=0; i<MAXPLAYERS; i++)
        if (teamingame(i))  num_teams++;
    WI_Rank_Fit( num_teams, TEAMRANKINGY, &fit );

    // count frags for each present player
    scorelines = HU_Create_TeamFragTbl(fragtab,dm_totals,dm_frags);

    WI_Draw_Ranking_Cols("Frags", fit.x[0], TEAMRANKINGY, fragtab, scorelines, false,
                    whiteplayer, 6, fit.y_limit,
                    fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);

    if( fit.ntable == 4 )
    {
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
        WI_Draw_Ranking_Cols("Buchholz", fit.x[1], TEAMRANKINGY, fragtab, scorelines, false,
                        whiteplayer, 6, fit.y_limit,
                        fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);

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
        WI_Draw_Ranking_Cols("indiv.", fit.x[2], TEAMRANKINGY, fragtab, scorelines, false,
                        whiteplayer, 6, fit.y_limit,
                        fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);
    }

    // count deads
    if( fit.ntable >= 2 )
    {
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
        WI_Draw_Ranking_Cols("deads", fit.x[fit.ntable - 1], TEAMRANKINGY, fragtab,
                        scorelines, false, whiteplayer, 6, fit.y_limit,
                        fit.pitch, fit.max_rows, fit.col_dx, fit.sub_w);
    }
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

// [Arcade] ---- The compact netgame table. ----
//
// The classic table gives each player a 16-unit row and draws the percentages
// with the 12-unit WINUM patches, which is 8 rows between the headers at y 62
// and the bottom of the screen.  The compact one draws the numbers in the
// small hu_font instead, at the 8-unit compact pitch, and wraps into a second
// column when one will not hold everyone -- 17 rows a column, so two columns
// cover MAXPLAYERS.
//
// The percentages lose their '%' glyph here (9 units each, 27 a row, which is
// most of a name): the column headings carry it instead.  Everything is right
// justified into its field, so the ragged edge is on the left where the eye
// is not comparing them.
//
// The fields are sized to the widest value that will actually be drawn, not
// to "100" -- three digits cost 21 units where two cost 16, three times over,
// and that width is the difference between a name that reads "PLAYER" and one
// that reads "PLAYER12".  Never from the counters, which climb over several
// seconds: a field that widened part way through the count-up would shove
// every name in the table sideways while the player was reading it.
#define WI_C_MARGIN     3       // screen edge to the first column
#define WI_C_COLGAP     4       // between the two columns
#define WI_C_MARK_W     5       // gutter for the "you are here" marker
// Floors and ceilings on the measured fields.  The floor keeps a table of
// zeroes from looking like a mistake; the ceiling is what stops the numbers
// eating the name column.
//   23 is "100" (21 units, and '1' is only 5 of them) plus the gap: no
// percentage can be wider, so this only ever catches a caller that measured
// something else.  32 is "-999" plus the gap.  At both ceilings at once the
// name field is 49 units in a two-column table, which is still wider than its
// own "Player" heading -- tools/interfit-test.py checks exactly that, and
// caught the ceiling being 26 before it was 23.
#define WI_C_PCT_MIN   12
#define WI_C_PCT_MAX   23
#define WI_C_FRAG_MIN  10
#define WI_C_FRAG_MAX  32
// First row of the compact table, with its headings on the line above.  Ten
// units above the classic first row, which the compact headings do not need.
#define WI_NG_COMPACT_Y   60

typedef struct {
    byte  ncol;        // columns of rows
    byte  rows;        // rows in each column
    int   col_x[2];    // left edge of each column
    int   col_w;
    int   name_w;      // the colour bar, and the name drawn in it
    int   pct_w;       // a percentage field, as clamped
    int   frag_w;      // the frags field, 0 when there is no frags column
    int   x_kills;     // right edge of each number field, from col_x
    int   x_items;
    int   x_secret;
    int   x_frags;
} wi_ngfit_t;

// Pure arithmetic, so tools/interfit-test.py can lift it out and check every
// player count against the screen.
//   num_pl    : players to place
//   pct_w     : width wanted for one percentage field, measured by the caller
//   frag_w    : width wanted for the frags field, 0 for no frags column
//   name_want : width the longest name present wants, 0 to take what is going
//   ytop      : base y of the first row
static void WI_Netgame_Fit( int num_pl, int pct_w, int frag_w, int name_want,
                            int ytop, wi_ngfit_t * out )
{
    int  per_col = WI_Compact_Rows( ytop );
    int  numw, avail, pad;

    if( pct_w < WI_C_PCT_MIN )  pct_w = WI_C_PCT_MIN;
    if( pct_w > WI_C_PCT_MAX )  pct_w = WI_C_PCT_MAX;
    if( frag_w > 0 )
    {
        if( frag_w < WI_C_FRAG_MIN )  frag_w = WI_C_FRAG_MIN;
        if( frag_w > WI_C_FRAG_MAX )  frag_w = WI_C_FRAG_MAX;
    }
    out->pct_w  = pct_w;
    out->frag_w = frag_w;
    numw = (3 * pct_w) + frag_w;

    out->ncol = (num_pl > per_col) ? 2 : 1;
    // Balanced, so 20 players are 10 and 10 rather than 17 and 3.
    out->rows = (num_pl + out->ncol - 1) / out->ncol;
    if( out->rows > per_col )  out->rows = per_col;
    if( out->rows < 1 )  out->rows = 1;

    out->col_w = (BASEVIDWIDTH - (2 * WI_C_MARGIN)
                  - ((out->ncol - 1) * WI_C_COLGAP)) / out->ncol;

    // The name gets what is left, but no more than the longest name present
    // actually wants.  Nine players in a single column otherwise get a colour
    // bar 211 units long with a short name at one end of it and the
    // percentages stranded at the other, which reads as a broken layout
    // rather than a roomy one.  Whatever that leaves over is split either
    // side, so the block sits in the middle of its column.
    avail = out->col_w - numw - WI_C_MARK_W;
    out->name_w = avail;
    if( name_want > 0 )
    {
        // Never narrower than the "Player" heading the drawer writes over it,
        // whatever the names are: a heading that did not fit its own column
        // would be the one thing on the page nobody could explain.
        int  floor_w = V_StringWidth("Player");
        int  want = name_want + 2;   // the gap inside the colour bar

        if( floor_w < WI_C_NAME_MIN )  floor_w = WI_C_NAME_MIN;
        if( want < floor_w )  want = floor_w;
        if( want < avail )  out->name_w = want;
    }

    pad = (out->col_w - (WI_C_MARK_W + out->name_w + numw)) / 2;
    out->col_x[0] = WI_C_MARGIN + pad;
    out->col_x[1] = WI_C_MARGIN + out->col_w + WI_C_COLGAP + pad;

    out->x_kills  = WI_C_MARK_W + out->name_w + pct_w;
    out->x_items  = out->x_kills + pct_w;
    out->x_secret = out->x_items + pct_w;
    out->x_frags  = out->x_secret + frag_w;
}

// [Arcade] A percentage as the compact table shows it: blank while it is
// still counting up, "-" where there was nothing of that kind on the map.
// Mirrors WI_Draw_Percent's two negative cases.
static void WI_Percent_Str( char * buf, int bufsize, int pernum )
{
    if( pernum == -100 )
        snprintf( buf, bufsize, "-" );     // none on the map
    else if( pernum < 0 )
        buf[0] = '\0';                     // not counted up yet
    else
        snprintf( buf, bufsize, "%d", pernum );
}

// [Arcade] The compact netgame table, drawn in place of the classic one when
// there are more players than that one holds.  Called by WI_Draw_NetgameStats,
// which has already drawn the background and the level name.
static void WI_Draw_Netgame_Compact( int ytop )
{
    char        buf[MAXPLAYERNAME + 1];
    wi_ngfit_t  fit;
    int         num_pl = 0;
    int         i, n, colornum;
    int         pct_w, frag_w, name_want;
    int         y_hdr = ytop - WI_C_PITCH - 1;

    for (i=0 ; i<MAXPLAYERS ; i++)
        if( playeringame[i] )  num_pl++;

    // Field widths from the widest value that will actually be drawn, taken
    // from the final figures rather than from the counters -- see the block
    // above.  The headings set the floor, since a heading that did not fit
    // its own column would be the one thing on the page nobody could explain.
    pct_w  = V_StringWidth("K%");
    frag_w = dofrags ? V_StringWidth("F") : 0;
    name_want = V_StringWidth("Player");    // the heading over that column
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        int  final[3], k, w;

        if( !playeringame[i] )  continue;

        w = V_StringWidth( player_names[i] );
        if( w > name_want )  name_want = w;

        final[0] = (wbs->maxkills  > 0) ? (wb_plyr[i].skills  * 100) / wbs->maxkills  : -100;
        final[1] = (wbs->maxitems  > 0) ? (wb_plyr[i].sitems  * 100) / wbs->maxitems  : -100;
        final[2] = (wbs->maxsecret > 0) ? (wb_plyr[i].ssecret * 100) / wbs->maxsecret : -100;
        for( k = 0; k < 3; k++ )
        {
            WI_Percent_Str( buf, sizeof(buf), final[k] );
            w = V_StringWidth( buf );
            if( w > pct_w )  pct_w = w;
        }

        if( dofrags )
        {
            snprintf( buf, sizeof(buf), "%d", ST_PlayerFrags(i) );
            w = V_StringWidth( buf );
            if( w > frag_w )  frag_w = w;
        }
    }
    pct_w += 2;                        // a gap to whatever is on its left
    if( dofrags )  frag_w += 2;

    WI_Netgame_Fit( num_pl, pct_w, frag_w, name_want, ytop, &fit );

    colornum = ( EN_heretic ) ? 230 : 0x78;

    // Column headings.  These carry the '%' that the rows do not.
    for (i=0 ; i<fit.ncol ; i++)
    {
        int  cx = fit.col_x[i];

        V_DrawString( cx + WI_C_MARK_W, y_hdr, V_WHITEMAP, "Player" );
        WI_Draw_String_RJ( cx + fit.x_kills,  y_hdr, V_WHITEMAP, "K%" );
        WI_Draw_String_RJ( cx + fit.x_items,  y_hdr, V_WHITEMAP, "I%" );
        WI_Draw_String_RJ( cx + fit.x_secret, y_hdr, V_WHITEMAP, "S%" );
        if( dofrags )
            WI_Draw_String_RJ( cx + fit.x_frags, y_hdr, V_WHITEMAP, "F" );
    }

    n = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        int  cx, cy;
        byte skin_color, color;

        if (!playeringame[i])
            continue;

        if( n >= (fit.ncol * fit.rows) )
            break;                  // no room left, cannot happen at MAXPLAYERS

        cx = fit.col_x[ n / fit.rows ];
        cy = ytop + ((n % fit.rows) * WI_C_PITCH);
        n++;

        skin_color = players[i].skincolor;
        color = (skin_color) ?
           SKIN_TO_SKINMAP(skin_color)[ colornum ]
         : reg_colormaps[ colornum ];  // default green skin

        // The colour bar behind the name is the player's identity here: there
        // is no room for the status-bar face the classic table marks the
        // console player with, so that gets the marker gutter instead.
        V_DrawScaledFill( cx + WI_C_MARK_W, cy - 1, fit.name_w, WI_C_ROW_H, color );
        if( i == me )
            V_DrawString( cx, cy, V_WHITEMAP, ">" );

        WI_Fit_Name( buf, sizeof(buf), player_names[i], fit.name_w - 2 );
        V_DrawString( cx + WI_C_MARK_W + 1, cy, V_WHITEMAP, buf );

        WI_Percent_Str( buf, sizeof(buf), cnt_kills[i] );
        WI_Draw_String_RJ( cx + fit.x_kills, cy, V_WHITEMAP, buf );
        WI_Percent_Str( buf, sizeof(buf), cnt_items[i] );
        WI_Draw_String_RJ( cx + fit.x_items, cy, V_WHITEMAP, buf );
        WI_Percent_Str( buf, sizeof(buf), cnt_secret[i] );
        WI_Draw_String_RJ( cx + fit.x_secret, cy, V_WHITEMAP, buf );

        if( dofrags )
        {
            snprintf( buf, sizeof(buf), "%d", cnt_frags[i] );
            WI_Draw_String_RJ( cx + fit.x_frags, cy, V_WHITEMAP, buf );
        }
    }
}

// Called by WI_Drawer
static void WI_Draw_NetgameStats(void)
{
    // Hardware or software render.
    int  i, x, y, y10;
    int  pwidth, ngsx;
    // [Arcade] Deciding between the two layouts, before either is drawn.
    int  num_pl = 0, classic_rows, hdr_h;

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    WI_Draw_LF();

    // [Arcade] Does the classic table hold everyone?  Its first row sits under
    // the heading patches and each one is WI_SPACINGY lower, with a percentage
    // patch drawn 10 below the row's own y -- so the last row that fits is the
    // last whose percentage still ends above the bottom of the screen.
    // Measured rather than assumed, because the heading is FontB in Heretic
    // and a patch in Doom, and they are not the same height.
    for (i=0 ; i<MAXPLAYERS ; i++)
        if( playeringame[i] )  num_pl++;

    hdr_h = FontBBaseLump ? V_TextBHeight("Kills") : V_patch(kills)->height;
    classic_rows = ((BASEVIDHEIGHT - V_patch(percent)->height - 10
                     - (NG_STATSY + hdr_h)) / WI_SPACINGY) + 1;
    if( classic_rows < 1 )  classic_rows = 1;

    if( num_pl > classic_rows )
    {
        WI_Draw_Netgame_Compact( WI_NG_COMPACT_Y );
        return;
    }

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
        // [Arcade] Every name in grey.  Upstream drew the console player's name
        // in V_WHITEMAP and everyone else's in attribute 0, which is red -- with
        // four local players that singled out panel 1 for no reason a player
        // could see.  The face and shoulder banner still mark the console player.
        int ds_att = V_WHITEMAP;
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
