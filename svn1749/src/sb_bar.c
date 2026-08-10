// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: sb_bar.c 1452 2019-08-03 07:03:27Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// $Log: sb_bar.c,v $
// Revision 1.8  2001/08/27 19:59:35  hurdler
// Fix colormap in heretic + opengl, fixedcolormap and NEWCORONA
//
// Revision 1.7  2001/08/02 19:15:59  bpereira
// fix player reset in secret level of doom2
//
// Revision 1.6  2001/06/30 15:06:01  bpereira
// fixed wrong next level name in intermission
//
// Revision 1.5  2001/06/10 21:16:01  bpereira
// Revision 1.4  2001/02/24 13:35:21  bpereira
// Revision 1.3  2001/02/10 13:20:55  hurdler
// update license
//
// DESCRIPTION:
//   Heretic Status bar display
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "w_wad.h"

#include "screen.h"
#include "i_video.h"
#include "v_video.h"
#include "s_sound.h"
#include "z_zone.h"

#include "r_local.h"
#include "p_inter.h"
#include "m_random.h"

#include "keys.h"
#include "dstrings.h"

//#include "am_map.h"
  // automapactive
#include "g_game.h"
#include "m_cheat.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "st_lib.h"

#ifdef HWRENDER
#include "hardware/hw_drv.h"
#include "hardware/hw_main.h"
#endif



// Public Data

int H_ArtifactFlash;

// Heretic yellow for overlay flash
#define FLASH_OV_COLOR   140
// Heretic color for ammo flash on black
#define FLASH_BLK_COLOR  138
// Heretic color for health, armor flash on gray
#define FLASH_GRY_COLOR  124
// Heretic color for keys
#define FLASH_KEYS_COLOR  2
// Heretic colors
//  2..9  dark grays
//  218  light green
//  140  yellow
//  138  dark yellow
//  124  light amber

// Macros

#define STARTREDPALS    1
#define NUMREDPALS      8
#define STARTBONUSPALS  9
#define NUMBONUSPALS    4

// Types

// Heretic Private Functions

static void ShadeChain(void);
static void DrINumber(signed int val, int x, int y);
static void DrBNumber(signed int val, int x, int y);
static void DrawCommonBar( player_t * plyr );
static void H_DrawMainBar( player_t * plyr );
static void H_DrawInventoryBar( player_t * plyr );
static void H_OverlayDrawer( byte status_position, player_t * plyr );


// Private Data

static int HealthMarker;
static int ChainWiggle;
// Use st_plyr for Single Player status.


// #define USE_PLAYPALETTE
#ifdef USE_PLAYPALETTE
// [WDJ] This gets set, but is never read.
static int H_playpalette;
#endif

// [WDJ] This might be useful for debugging, but is otherwise unused.
// #define USE_UPDATESTATE
#ifdef USE_UPDATESTATE
// Heretic status bar status bits.
enum{
  HUS_fullview = 0x01,
  HUS_statbar = 0x02,   // am drawing the status bar
  HUS_messages = 0x04,  // unused
  HUS_overlay = 0x08
};

// [WDJ] This gets set, but is never read ?
static int H_UpdateState;  // HUS_
#endif

// [WDJ] all patches loaded by CachePatch and are saved endian fixed
static patch_t *PatchLTFACE = NULL;
static patch_t *PatchRTFACE;
static patch_t *PatchBARBACK;
static patch_t *PatchCHAIN;
static patch_t *PatchSTATBAR;
static patch_t *PatchLIFEGEM;
//static patch_t *PatchEMPWEAP;
//static patch_t *PatchLIL4BOX;
static patch_t *PatchLTFCTOP;
static patch_t *PatchRTFCTOP;
//static patch_t *PatchARMORBOX;
//static patch_t *PatchARTIBOX;
static patch_t *PatchSELECTBOX;
//static patch_t *PatchKILLSPIC;
//static patch_t *PatchMANAPIC;
//static patch_t *PatchPOWERICN;
static patch_t *PatchINVLFGEM1;
static patch_t *PatchINVLFGEM2;
static patch_t *PatchINVRTGEM1;
static patch_t *PatchINVRTGEM2;
static patch_t *PatchINumbers[10];
static patch_t *PatchNEGATIVE;
static patch_t *PatchSmNumbers[10];
static patch_t *PatchBLACKSQ;
static patch_t *PatchINVBAR;
static patch_t *PatchARMCLEAR;
static patch_t *PatchCHAINBACK;

static int FontBNumBase;
static int spinbooklump;
static int spinflylump;

//---------------------------------------------------------------------------
//
// PROC SB_Heretic_Init
//
//---------------------------------------------------------------------------

byte heretic_sb_patches_loaded = 0;
load_patch_t  heretic_sb_patches[] = 
{
  { &PatchLTFACE, "LTFACE" },
  { &PatchRTFACE, "RTFACE" },
  { &PatchBARBACK, "BARBACK" },
  { &PatchINVBAR, "INVBAR" },
  { &PatchCHAIN, "CHAIN" },
  { &PatchLTFCTOP, "LTFCTOP" },
  { &PatchRTFCTOP, "RTFCTOP" },
  { &PatchSELECTBOX, "SELECTBOX" },
  { &PatchINVLFGEM1, "INVGEML1" },
  { &PatchINVLFGEM2, "INVGEML2" },
  { &PatchINVRTGEM1, "INVGEMR1" },
  { &PatchINVRTGEM2, "INVGEMR2" },
  { &PatchBLACKSQ, "BLACKSQ" },
  { &PatchARMCLEAR, "ARMCLEAR" },
  { &PatchCHAINBACK, "CHAINBACK" },
  { &PatchNEGATIVE, "NEGNUM" },
  { NULL, NULL }
};

load_patch_t  heretic_sb_release_patches[] =
{
  { &PatchSTATBAR, NULL },
  { &PatchLIFEGEM, NULL },
  { NULL, NULL }
};


void SB_Heretic_Load_Graphics(void)
{
    int i;
    int startLump;

    // [WDJ] all patches are endian fixed
    load_patch_list( heretic_sb_patches );
    heretic_sb_patches_loaded = 1;

    PatchSTATBAR = W_CachePatchName(
        (( deathmatch )? "STATBAR" : "LIFEBAR" ), PU_LOCK_SB );

    if(!multiplayer)
    { // single player game uses red life gem
        PatchLIFEGEM = W_CachePatchName("LIFEGEM2", PU_LOCK_SB);
    }
    else
    {
        PatchLIFEGEM = W_CachePatchNum(W_GetNumForName("LIFEGEM0")
            + consoleplayer, PU_LOCK_SB);
    }

    startLump = W_GetNumForName("IN0");
    for(i = 0; i < 10; i++)
    {
        PatchINumbers[i] = W_CachePatchNum(startLump+i, PU_LOCK_SB);
    }

    FontBNumBase = W_GetNumForName("FONTB16");

    startLump = W_GetNumForName("SMALLIN0");
    for(i = 0; i < 10; i++)
    {
        PatchSmNumbers[i] = W_CachePatchNum(startLump+i, PU_LOCK_SB);
    }

#ifdef USE_PLAYPALETTE
    H_playpalette = W_GetNumForName("PLAYPAL");
#endif
    spinbooklump = W_GetNumForName("SPINBK0");
    spinflylump = W_GetNumForName("SPFLY0");
}


void SB_Heretic_Release_Graphics(void)
{
    if( ! heretic_sb_patches_loaded )   return;
    heretic_sb_patches_loaded = 0;

    release_patch_list( heretic_sb_patches );
    release_patch_list( heretic_sb_release_patches );
    release_patch_array( PatchINumbers, 10 );
    release_patch_array( PatchSmNumbers, 10 );
}



//---------------------------------------------------------------------------
//
// PROC SB_Heretic_Ticker
//
//---------------------------------------------------------------------------

void SB_Heretic_Ticker(void)
{
    int delta;
    int curHealth;

    if(leveltime&1)
    {
        // Heretic use of P_Random
        ChainWiggle = M_Random()&1;
    }

    curHealth = consoleplayer_ptr->mo->health;
    if(curHealth < 0)
    {
        curHealth = 0;
    }
    if(curHealth < HealthMarker)
    {
        delta = (HealthMarker - curHealth)>>2;
        if(delta < 1)
        {
            delta = 1;
        }
        else if(delta > 8)
        {
            delta = 8;
        }
        HealthMarker -= delta;
    }
    else if(curHealth > HealthMarker)
    {
        delta = (curHealth - HealthMarker)>>2;
        if(delta < 1)
        {
            delta = 1;
        }
        else if(delta > 8)
        {
            delta = 8;
        }
        HealthMarker += delta;
    }
}

//---------------------------------------------------------------------------
// Heretic
// PROC DrINumber
//
// Draws a three digit number.
//
//---------------------------------------------------------------------------

static void DrINumber(signed int val, int x, int y)
{
    patch_t *patch;
    int oldval;

    // draw stbar_fg, maybe scaled or not
    oldval = val;
    if(val < 0)
    {
        if(val < -9)
        {
            V_DrawScaledPatch_Name(x+1, y+1, "LAME");
        }
        else
        {
            val = -val;
            V_DrawScaledPatch(x+18, y, PatchINumbers[val]);
            V_DrawScaledPatch(x+9, y, PatchNEGATIVE);
        }
        return;
    }

    if(val > 99)
    {
        patch = PatchINumbers[val/100];
        V_DrawScaledPatch(x, y, patch);
    }

    val = val%100;
    if(val > 9 || oldval > 99)
    {
        patch = PatchINumbers[val/10];
        V_DrawScaledPatch(x+9, y, patch);
    }

    val = val%10;
    patch = PatchINumbers[val];
    V_DrawScaledPatch(x+18, y, patch);
}

//---------------------------------------------------------------------------
// Heretic
// PROC DrBNumber
//
// Draws a three digit number using FontB
//
//---------------------------------------------------------------------------

// Called by DrBNumber
// Draw screen0, scaled start (V_SCALESTART|0)
//#define V_DrawShadowedPatch(x,y,p) V_DrawTranslucentPatch(x,y,p)
void V_DrawShadowedPatch(int x,int y, patch_t *p)
{
//    V_DrawTranslucentPatch(x+2,y+2, p );
    V_DrawScaledPatch(x,y,p);
}

// Called by H_OverlayDrawer
// Draw screen0, scaled start (V_SCALESTART|0)
#define V_DrawFuzzPatch(x,y,p)     V_DrawTranslucentPatch(x,y,p)

static void DrBNumber(signed int val, int x, int y)
{
        patch_t *patch;
        int xpos;
        int oldval;

        // Draw to stbar_fg, maybe scaled or not
        oldval = val;
        xpos = x;
        if(val < 0)
        {
                val = 0;
        }
        if(val > 99)
        {
                patch = W_CachePatchNum(FontBNumBase+val/100, PU_CACHE);
                V_DrawShadowedPatch(xpos+6-patch->width/2, y, patch);
        }
        val = val%100;
        xpos += 12;
        if(val > 9 || oldval > 99)
        {
                patch = W_CachePatchNum(FontBNumBase+val/10, PU_CACHE);
                V_DrawShadowedPatch(xpos+6-patch->width/2, y, patch);
        }
        val = val%10;
        xpos += 12;
        patch = W_CachePatchNum(FontBNumBase+val, PU_CACHE);
        V_DrawShadowedPatch(xpos+6-patch->width/2, y, patch);
}

//---------------------------------------------------------------------------
// Heretic
// PROC DrSmallNumber
//
// Draws a small two digit number.
//
//---------------------------------------------------------------------------

static void DrSmallNumber(int val, int x, int y)
{
    patch_t *patch;

    if(val == 1)
       return;

    if(val > 9)
    {
        patch = PatchSmNumbers[val/10];
        V_DrawScaledPatch(x, y, patch);
    }
    val = val%10;
    patch = PatchSmNumbers[val];
    V_DrawScaledPatch(x+4, y, patch);
}

//---------------------------------------------------------------------------
// Heretic
// PROC ShadeLine
//
//---------------------------------------------------------------------------

static byte chain_shift_table[8] = {
   0, 0, 0, 1, 1, 1, 2, 2
};
static uint32_t chain_mask_table32[8] = {
   0xFCFCFCFC, 0xF3F3F3F3, 0xEBEBEBEB, 0x7F7F7F7F,
   0x6F6F6F6F, 0x67676767, 0x3F3F3F3F, 0x27272727
};
static uint16_t chain_mask_table16[8] = {
   0xE73C, // 11100 111001 11100
   0xDEDA, // 11011 110110 11010
   0xBDD6, // 10111 101110 10110
   0x7BEF, // 01111 011111 01111
   0x6B8D, // 01101 011010 01101
   0x5ACB, // 01011 010110 01011
   0x39E7, // 00111 001111 00111
   0x2965  // 00101 001011 00101
};
static uint16_t chain_mask_table15[8] = {
   0x739C, // 0 11100 11100 11100
   0x6F7B, // 0 11011 11011 11011
   0x5EF7, // 0 10111 10111 10111
   0x3DEF, // 0 01111 01111 01111
   0x35AD, // 0 01101 01101 01101
   0x2D6B, // 0 01011 01011 01011
   0x1CE7, // 0 00111 00111 00111
   0x14A5  // 0 00101 00101 00101
};


static void ShadeLine(int x, int y, int shade)
{
    byte *dest = V_GetDrawAddr( x,  y );
    byte *shades;
    uint32_t mask;
    int shadeshift;
    int height = 10 * stbar_scaley;

    switch(vid.drawmode)
    {
     case DRAW8PAL:
        // [WDJ] This only works in 8 bit palette mode.
        shades = & reg_colormaps[ LIGHTTABLE( 9 + shade*2 ) ];
        while(height--)
        {
            *(dest) = *(shades + *dest);
            dest += vid.ybytes;
        }
        break;
#ifdef ENABLE_DRAW15
     case DRAW15:
        mask = chain_mask_table15[shade];
        goto shadeline_15_16;
#endif
#ifdef ENABLE_DRAW16
     case DRAW16:
        mask = chain_mask_table16[shade];
#endif
#ifdef ENABLE_DRAW15
     shadeline_15_16:
#endif
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
        shadeshift = chain_shift_table[shade];
        while(height--)
        {
            *((uint16_t*)dest) = (*((uint16_t*)dest) >> shadeshift) & mask;
            dest += vid.ybytes;
        }
        break;
#endif
     default:
        shadeshift = chain_shift_table[shade];
        mask = chain_mask_table32[shade];
        while(height--)
        {
            *((uint32_t*)dest) = (*((uint32_t*)dest) >> shadeshift) & mask;
            dest += vid.ybytes;
        }
        break;
    }
}

//---------------------------------------------------------------------------
// Heretic
// PROC ShadeChain
//
//---------------------------------------------------------------------------

static void ShadeChain(void)
{
    int chain_x1 = (stbar_x+277) * stbar_scalex;
    int chain_x2 = (stbar_x+19) * stbar_scalex;
    int chain_y = (stbar_y+32) * stbar_scaley;
    int i1, i2;

    if( rendermode != render_soft )
        return;
    
    for(i1 = 0; i1 < 8; i1++)
    {
        for(i2 = i1*2*stbar_scalex; i2 < ((i1+1)*2*stbar_scalex); i2++ )
        {
            ShadeLine(chain_x1+i2, chain_y, i1);
            ShadeLine(chain_x2+i2, chain_y, 7-i1);
        }
    }
}



//---------------------------------------------------------------------------
//
// PROC SB_Heretic_Drawer
//
//---------------------------------------------------------------------------

char patcharti[][10] =
{
        {"ARTIBOX"},    // none
        {"ARTIINVU"},   // invulnerability
        {"ARTIINVS"},   // invisibility
        {"ARTIPTN2"},   // health
        {"ARTISPHL"},   // superhealth
        {"ARTIPWBK"},   // tomeofpower
        {"ARTITRCH"},   // torch
        {"ARTIFBMB"},   // firebomb
        {"ARTIEGGC"},   // egg
        {"ARTISOAR"},   // fly
        {"ARTIATLP"}    // teleport
};

char ammopic[][10] =
{
        {"INAMGLD"},
        {"INAMBOW"},
        {"INAMBST"},
        {"INAMRAM"},
        {"INAMPNX"},
        {"INAMLOB"}
};

typedef enum { SBS_refresh, SBS_status, SBS_inventory } SB_state_e;
static int SB_state = SBS_refresh;

static int oldarti = -1;
static int oldartiCount = 0;
static int oldfrags = -9999;
static int oldammo = -1;
static int oldarmor = -1;
static int oldweapon = -1;
static int oldhealth = -1;
static int oldlife = -1;
// oldkeys now uses st_card



void SB_Heretic_Drawer( boolean refresh )
{
    static boolean hitCenterFrame;

    player_t * plyr;
    int frame;

    if( stbar_recalc )
    {
        ST_CalcPos();
        refresh = 1;
    }

    if( refresh )
        SB_state = SBS_refresh;

    // Draw to stbar_fg, screen0, flags selected by status bar style
    // Maybe scaled or not scaled.
    V_SetupDraw( stbar_fg );

    plyr = st_plyr = displayplayer_ptr;
    if( st_overlay_on )
    {
        // Overlay status
        if( cv_splitscreen.EV )
        {
            if((vid.drawmode != DRAW8PAL) && st_palette != 0 )
                ST_Palette0();

            // player 1 is upper
            H_OverlayDrawer ( 1, displayplayer_ptr );
            if( displayplayer2_ptr )
            {
                // player 2 is lower
                H_OverlayDrawer( 0, displayplayer2_ptr );
            }
        }
        else if( !playerdeadview )
        {
            H_OverlayDrawer( 0, displayplayer_ptr );
        }
        SB_state = SBS_refresh;
    }
    else if( stbar_on )
    {
        // Status bar on.
        // Single Player only, keeps old status.
        if(SB_state == SBS_refresh)
        {
            if( rendermode == render_soft )
            {
                V_CopyRect(0, vid.height-stbar_height, BG, vid.width,
                           stbar_height, 0, vid.height-stbar_height, FG);
            }

            V_DrawScaledPatch(stbar_x, stbar_y, PatchBARBACK);
            if(consoleplayer_ptr->cheats&CF_GODMODE)
            {
                V_DrawScaledPatch_Name(stbar_x+16, stbar_y+9, "GOD1");
                V_DrawScaledPatch_Name(stbar_x+287, stbar_y+9, "GOD2");
            }
            oldhealth = -1;
        }

        DrawCommonBar( plyr );

        if(!plyr->st_inventoryTics)
        {
            // Display main status bar
            if(SB_state != SBS_status)
            {
                // Main interface
                V_DrawScaledPatch(stbar_x+34, stbar_y+2, PatchSTATBAR);
                // Refresh all status
                oldarti = -1;
                oldammo = -1;
                oldarmor = -1;
                oldweapon = -1;
                oldfrags = -9999; //can't use -1, 'cuz of negative frags
                oldlife = -1;
                st_card = -1;
            }
            H_DrawMainBar( plyr );
            SB_state = SBS_status;
        }
        else
        {
            // Display inventory bar
            if(SB_state != SBS_inventory)
            {
                V_DrawScaledPatch(stbar_x+34, stbar_y+2, PatchINVBAR);
            }
            H_DrawInventoryBar( plyr );
            SB_state = SBS_inventory;
        }
    }

    // Palette flash effects moved to R_SetupFrame and RenderPlayer.
    
    // Flight icons
    if(plyr->powers[pw_flight])
    {
        // Draw screen0, scaled
        V_SetupDraw( FG );

        if(plyr->powers[pw_flight] > BLINKTHRESHOLD
            || !(plyr->powers[pw_flight]&0x10))  // blink rate
        {
            frame = (leveltime/3)&15;
            if(plyr->mo->flags2&MF2_FLY)
            {
                if(hitCenterFrame && (frame != 15 && frame != 0))
                {
                    V_DrawScaledPatch_Num(20, 17, spinflylump+15);
                }
                else
                {
                    V_DrawScaledPatch_Num(20, 17, spinflylump+frame);
                    hitCenterFrame = false;
                }
            }
            else
            {
                if(!hitCenterFrame && (frame != 15 && frame != 0))
                {
                    V_DrawScaledPatch_Num(20, 17, spinflylump+frame);
                    hitCenterFrame = false;
                }
                else
                {
                    V_DrawScaledPatch_Num(20, 17, spinflylump+15);
                    hitCenterFrame = true;
                }
            }
            //                  BorderTopRefresh = true;
#ifdef USE_UPDATESTATE
            //                  H_UpdateState |= HUS_messages;
#endif
        }
        else
        {
            //                  BorderTopRefresh = true;
#ifdef USE_UPDATESTATE
            //                  H_UpdateState |= HUS_messages;
#endif
        }
    }
    
    if(plyr->powers[pw_weaponlevel2] && !plyr->chickenTics)
    {
        // Draw screen0, scaled
        V_SetupDraw( FG );

        if(plyr->powers[pw_weaponlevel2] > BLINKTHRESHOLD
            || !(plyr->powers[pw_weaponlevel2]&0x10)) // blink rate
        {
            frame = (leveltime/3)&15;
            V_DrawScaledPatch_Num(300, 17, spinbooklump+frame);
            //                  BorderTopRefresh = true;
#ifdef USE_UPDATESTATE
            //                  H_UpdateState |= HUS_messages;
#endif
        }
        else
        {
            //                  BorderTopRefresh = true;
#ifdef USE_UPDATESTATE
            //                  H_UpdateState |= HUS_messages;
#endif
        }
    }
#if 0
    if(plyr->powers[pw_weaponlevel2] > BLINKTHRESHOLD
       || (plyr->powers[pw_weaponlevel2]&0x08))  // blink rate
    {
        V_DrawScaledPatch_Name(stbar_x+291, 0, 0, "ARTIPWBK");  // tome of power
    }
    else
    {
//        BorderTopRefresh = true;
    }
#endif
}


// Sets the new palette based upon current values of player->damagecount
// and player->bonuscount.
void H_PaletteFlash( player_t * plyr )
{
    int palette;

    if(plyr->damagecount)
    {
        palette = STARTREDPALS + ((plyr->damagecount+7)>>3);

        if (palette >= (STARTREDPALS+NUMREDPALS))
            palette = STARTREDPALS+NUMREDPALS-1;
    }
    else if(plyr->bonuscount && (cv_pickupflash.EV >= 2))
    {
        // Pickup object palette flash.
        palette = STARTBONUSPALS
           + ((plyr->bonuscount+7)>>(pickupflash_table[cv_pickupflash.EV]));

        if (palette >= (STARTBONUSPALS+NUMBONUSPALS))
            palette = STARTBONUSPALS+NUMBONUSPALS-1;
    }
    else
    {
        palette = 0;
    }


    if(palette != st_palette)
    {
        st_palette = palette;

#ifdef HWRENDER
        if( EN_HWR_flashpalette )
        {
            // Imitate the palette flash
            HWR_SetFlashPalette( palette );
        }
        else
#endif
        {
            // Splitscreen cannot use palette effects when 8bit palette draw,
            // but other draw modes can.
            if( ((cv_splitscreen.EV == 0) || (vid.drawmode != DRAW8PAL))
                || !palette )
                V_SetPalette (palette);
        }
    }
}

//---------------------------------------------------------------------------
//
// PROC DrawCommonBar
//
//---------------------------------------------------------------------------

// To stbar_fg, screen0 status bar, maybe scaled or not scaled.
// Called by SB_Heretic_Drawer
static void DrawCommonBar( player_t * plyr )
{
    int chainY;
    int healthPos;
   
    V_DrawScaledPatch(stbar_x, stbar_y-10, PatchLTFCTOP);
    V_DrawScaledPatch(stbar_x+290, stbar_y-10, PatchRTFCTOP);

    if(oldhealth != HealthMarker)
    {
        oldhealth = HealthMarker;
        healthPos = HealthMarker;
        if(healthPos < 0)
        {
            healthPos = 0;
        }
        if(healthPos > 100)
        {
            healthPos = 100;
        }
        healthPos = (healthPos*256)/100;
        V_DrawScaledPatch(stbar_x, stbar_y+32, PatchCHAINBACK);
        chainY = stbar_y + 33;
        if(HealthMarker != plyr->mo->health)
            chainY += ChainWiggle;
        V_DrawScaledPatch(stbar_x+2+(healthPos%17), chainY, PatchCHAIN);
        V_DrawScaledPatch(stbar_x+17+healthPos, chainY, PatchLIFEGEM);
        V_DrawScaledPatch(stbar_x, stbar_y+32, PatchLTFACE);
        V_DrawScaledPatch(stbar_x+276, stbar_y+32, PatchRTFACE);
        ShadeChain();
#ifdef USE_UPDATESTATE
        H_UpdateState |= HUS_statbar;
#endif
    }
}

//---------------------------------------------------------------------------
//
// PROC H_DrawMainBar
//
//---------------------------------------------------------------------------

// To stbar_fg, maybe scaled or not scaled.
static void H_DrawMainBar( player_t * plyr )
{
    int temp;

    // Ready artifact
    if(H_ArtifactFlash)
    {
        V_DrawScaledPatch(stbar_x+180, stbar_y+3, PatchBLACKSQ);
        V_DrawScaledPatch(stbar_x+182, stbar_y+3,
            W_CachePatchNum( W_GetNumForName("useartia") + H_ArtifactFlash - 1,
            PU_CACHE));
        H_ArtifactFlash--;
        oldarti = -1; // so that the correct artifact fills in after the flash
#ifdef USE_UPDATESTATE
        H_UpdateState |= HUS_statbar;
#endif
    }
    else if(oldarti != plyr->inv_ptr
            || oldartiCount != plyr->inventory[plyr->inv_ptr].count)
    {
        V_DrawScaledPatch(stbar_x+180, stbar_y+3, PatchBLACKSQ);
        if( plyr->inventory[plyr->inv_ptr].type > 0 )
        {
            V_DrawScaledPatch_Name(stbar_x+179,stbar_y+2,
                 patcharti[plyr->inventory[plyr->inv_ptr].type]);
            DrSmallNumber(plyr->inventory[plyr->inv_ptr].count, stbar_x+201, stbar_y+24);
        }
        oldarti = plyr->inv_ptr;
        oldartiCount = plyr->inventory[plyr->inv_ptr].count;
#ifdef USE_UPDATESTATE
        H_UpdateState |= HUS_statbar;
#endif
    }

    // Frags
    if( deathmatch )
    {
        temp = ST_PlayerFrags(plyr-players);
        if(temp != oldfrags)
        {
            V_DrawScaledPatch(stbar_x+57, stbar_y+13, PatchARMCLEAR);
            DrINumber(temp, stbar_x+61, stbar_y+12);
            oldfrags = temp;
#ifdef USE_UPDATESTATE
            H_UpdateState |= HUS_statbar;
#endif
        }
    }
    else
    {
        if(plyr->health_pickup && ( cv_pickupflash.EV == 1 ))
        {
            // Health pickup flash
            V_DrawScaledFill(stbar_x+57, stbar_y+13, 32, 11, FLASH_GRY_COLOR);
            oldlife = -2;
        }
        temp = min(max(0,HealthMarker),100);
        if(oldlife != temp)
        {
            if( oldlife > -2 )  // not flash
                V_DrawScaledPatch(stbar_x+57, stbar_y+13, PatchARMCLEAR);
            oldlife = temp;
            DrINumber(temp, stbar_x+61, stbar_y+12);
#ifdef USE_UPDATESTATE
            H_UpdateState |= HUS_statbar;
#endif
        }
    }

    // Keys
    if(plyr->key_pickup && ( cv_pickupflash.EV == 1 ))
    {
        // Key pickup flash
        V_DrawScaledFill(stbar_x+153, stbar_y+6, 10, 23, FLASH_KEYS_COLOR);
        st_card = 0;
        if(plyr->key_pickup == 1)
        {
            SB_state = SBS_refresh;
            plyr->key_pickup = 0;
        }
    }
    if(plyr->cards != st_card)
    {
        st_card = plyr->cards;
        if(plyr->cards & it_yellowcard)
        {
            V_DrawScaledPatch_Name(stbar_x+153, stbar_y+6, "ykeyicon");
        }
        if(plyr->cards & it_redcard)
        {
            V_DrawScaledPatch_Name(stbar_x+153, stbar_y+14, "gkeyicon");
        }
        if(plyr->cards & it_bluecard)
        {
            V_DrawScaledPatch_Name(stbar_x+153, stbar_y+22, "bkeyicon");
        }
#ifdef USE_UPDATESTATE
        H_UpdateState |= HUS_statbar;
#endif
    }
    // Ammo
    if(plyr->ammo_pickup && ( cv_pickupflash.EV == 1 ))
    {
        // Ammo pickup flash
        V_DrawScaledFill(stbar_x+108, stbar_y+5, 32, 11, FLASH_BLK_COLOR);
        oldammo = -2;
    }
    temp = plyr->ammo[wpnlev1info[plyr->readyweapon].ammo];
    if(oldammo != temp || oldweapon != plyr->readyweapon)
    {
        if( oldammo > -2 )  // not flash
            V_DrawScaledPatch(stbar_x+108, stbar_y+3, PatchBLACKSQ);
        if(temp && plyr->readyweapon > 0 && plyr->readyweapon < 7)
        {
            DrINumber(temp, stbar_x+109, stbar_y+4);
            V_DrawScaledPatch_Name(stbar_x+111, stbar_y+14,
                                   ammopic[plyr->readyweapon-1]);
        }
        oldammo = temp;
        oldweapon = plyr->readyweapon;
#ifdef USE_UPDATESTATE
        H_UpdateState |= HUS_statbar;
#endif
    }

    // Armor
    if(plyr->armor_pickup && ( cv_pickupflash.EV == 1 ))
    {
        // Armor pickup flash
        V_DrawScaledFill(stbar_x+224, stbar_y+13, 32, 11, FLASH_GRY_COLOR);
        oldarmor = -2;  // no background
    }
    if(oldarmor != plyr->armorpoints)
    {
        if( oldarmor > -2 )  // not flash
            V_DrawScaledPatch(stbar_x+224, stbar_y+13, PatchARMCLEAR);
        DrINumber(plyr->armorpoints, stbar_x+228, stbar_y+12);
        oldarmor = plyr->armorpoints;
#ifdef USE_UPDATESTATE
        H_UpdateState |= HUS_statbar;
#endif
    }
}

//---------------------------------------------------------------------------
//
// PROC H_DrawInventoryBar
//
//---------------------------------------------------------------------------

// Draw to stbar_fg, screen0 status bar
static void H_DrawInventoryBar( player_t * plyr )
{
    int i;
    int x;

    x = plyr->inv_ptr - plyr->st_curpos;
#ifdef USE_UPDATESTATE
    H_UpdateState |= HUS_statbar;
#endif
    V_DrawScaledPatch(stbar_x+34, stbar_y+2, PatchINVBAR);
    for(i = 0; i < 7; i++)
    {
        //V_DrawScaledPatch(stbar_x+50+i*31, stbar_y+2, 0, W_CachePatchName("ARTIBOX", PU_CACHE));
        if(plyr->inventorySlotNum > x+i
           && plyr->inventory[x+i].type != arti_none)
        {
            V_DrawScaledPatch_Name(stbar_x+50+i*31, stbar_y+2,
                                   patcharti[plyr->inventory[x+i].type]);
            DrSmallNumber(plyr->inventory[x+i].count, stbar_x+69+i*31, stbar_y+24);
        }
    }
    V_DrawScaledPatch(stbar_x+50+plyr->st_curpos*31, stbar_y+31, PatchSELECTBOX);
    if(x != 0)
    {
        V_DrawScaledPatch(stbar_x+38, stbar_y+1,
            !(leveltime&4) ? PatchINVLFGEM1 : PatchINVLFGEM2);
    }
    if(plyr->inventorySlotNum-x > 7)
    {
        V_DrawScaledPatch(stbar_x+269, stbar_y+1,
            !(leveltime&4) ? PatchINVRTGEM1 : PatchINVRTGEM2);
    }
}

// Overlay status, no status bar.
// to stbar_fg
//   status_position : 0=lower, 1=upper
static void H_OverlayDrawer( byte status_position, player_t * plyr )
{
    int i;
    int x;
    int temp;
    // Draw at original size.
    int sby = cv_splitscreen.EV ?
       (status_position ? (BASEVIDHEIGHT/2) - (BASEVIDHEIGHT-stbar_y) // player 1
       : stbar_y ) //  player 2
     : stbar_y;  // Single player

#ifdef USE_UPDATESTATE
    H_UpdateState |= HUS_overlay;
#endif

    if( cv_pickupflash.EV == 1 )
    {
        // OpenGL, the inventory at right edge of screen is at stbar_x+286
        // pickup flashes
        if(plyr->key_pickup)
        {
            V_DrawFill(stbar_x+260, sby+36, 20, 6, FLASH_OV_COLOR);
        }
        if(plyr->armor_pickup)
        {
            V_DrawFill(stbar_x+200, sby+36, 20, 6, FLASH_OV_COLOR);
        }
        if(plyr->ammo_pickup)
        {
            V_DrawFill(100, sby+36, 20, 6, FLASH_OV_COLOR);
        }
        if(plyr->health_pickup)
        {
            // Assume 3 digits  0..200
            V_DrawFill(5, sby+24, 36, 16, FLASH_OV_COLOR);
        }
    }

    // Health overlay
    if(plyr->mo->health > 0)
        DrBNumber(plyr->mo->health, 5, sby+22);
    else
        DrBNumber(0, 5, sby+22);

    if( deathmatch )
    {
        temp = ST_PlayerFrags(plyr - players);
        DrINumber(temp, 45, sby+27);
    }

    if(!plyr->st_inventoryTics)
    {
        if( plyr->inventory[plyr->inv_ptr].type > 0)
        {
            V_DrawFuzzPatch(stbar_x+286, sby+12,
                W_CachePatchName("ARTIBOX", PU_CACHE) );
            V_DrawScaledPatch_Name(stbar_x+286, sby+12,
                patcharti[plyr->inventory[plyr->inv_ptr].type]);
            DrSmallNumber(plyr->inventory[plyr->inv_ptr].count, stbar_x+307, sby+34);
        }
    }
    else
    {
        // Overlay inventory
        x = plyr->inv_ptr - plyr->st_curpos;
        for(i = 0; i < 7; i++)
        {
            V_DrawFuzzPatch(stbar_x+50+i*31, sby+10,
                W_CachePatchName("ARTIBOX", PU_CACHE) );
            if(plyr->inventorySlotNum > x+i
               && plyr->inventory[x+i].type != arti_none)
            {
                V_DrawScaledPatch_Name(stbar_x+50+i*31, sby+10,
                    patcharti[plyr->inventory[x+i].type] );
                DrSmallNumber(plyr->inventory[x+i].count, 69+i*31, sby+32);
            }
        }
        V_DrawScaledPatch(stbar_x+50+plyr->st_curpos*31, sby+39,
            PatchSELECTBOX );
        if(x != 0)
        {
            V_DrawScaledPatch(stbar_x+38, sby+9,
                !(leveltime&4) ? PatchINVLFGEM1 : PatchINVLFGEM2);
        }
        if(plyr->inventorySlotNum-x > 7)
        {
            V_DrawScaledPatch(stbar_x+269, sby+9,
                !(leveltime&4) ? PatchINVRTGEM1 : PatchINVRTGEM2);
        }
    }
}

