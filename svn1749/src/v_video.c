// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: v_video.c 1686 2024-06-20 09:45:56Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2012 by DooM Legacy Team.
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
// $Log: v_video.c,v $
// Revision 1.36  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.35  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.34  2003/08/11 13:50:00  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.33  2003/07/13 13:16:15  hurdler
//
// Revision 1.32  2003/06/11 04:45:17  ssntails
// High-res patch drawer added.
//
// Revision 1.31  2003/06/10 23:36:09  ssntails
// Variable flat support (32x32 to 2048x2048)
//
// Revision 1.30  2003/05/04 04:20:19  sburke
// Use SHORT macro for big-endian machines.
//
// Revision 1.29  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.28  2001/07/28 16:18:37  bpereira
// Revision 1.27  2001/05/16 21:21:14  bpereira
// Revision 1.26  2001/04/28 14:33:41  metzgermeister
// Revision 1.25  2001/04/17 22:30:40  hurdler
// Revision 1.24  2001/04/09 20:20:46  metzgermeister
// fixed crash bug
//
// Revision 1.23  2001/04/01 17:35:07  bpereira
// Revision 1.22  2001/03/30 17:12:51  bpereira
//
// Revision 1.21  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.20  2001/02/28 17:50:55  bpereira
// Revision 1.19  2001/02/24 13:35:21  bpereira
//
// Revision 1.18  2001/02/19 17:40:34  hurdler
// Fix a bug with "chat on" in hw mode
//
// Revision 1.17  2001/02/10 13:05:45  hurdler
//
// Revision 1.16  2001/01/31 17:14:08  hurdler
// Add cv_scalestatusbar in hardware mode
//
// Revision 1.15  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.14  2000/11/06 20:52:16  bpereira
// Revision 1.13  2000/11/04 16:23:44  bpereira
// Revision 1.12  2000/11/02 19:49:37  bpereira
//
// Revision 1.11  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.10  2000/08/31 14:30:56  bpereira
//
// Revision 1.9  2000/04/27 17:43:19  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.8  2000/04/24 20:24:38  bpereira
//
// Revision 1.7  2000/04/24 15:10:57  hurdler
// Support colormap for text
//
// Revision 1.6  2000/04/22 21:12:15  hurdler
//
// Revision 1.5  2000/04/06 20:47:08  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.4  2000/03/29 20:10:50  hurdler
//
// Revision 1.3  2000/03/12 23:16:41  linuxcub
// Fixed definition of VID_BlitLinearScreen (Well, it now compiles under RH61)
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Gamma correction LUT stuff.
//      Functions to draw patches (by post) directly to screen.
//      Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "r_local.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "r_draw.h"
#include "r_data.h"
  // NearestColor
#include "console.h"

#include "i_video.h"
  // rendermode
#include "i_system.h"
  // I_GetTime
#include "z_zone.h"
#include "doomstat.h"
  // gamemode
#include "p_setup.h"
  // P_flatsize_to_index
#include "m_misc.h"
  // drawmode, configfile functions


#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif


// Enable when DEBUG and cannot see text.
//#define DEBUG_FORCE_COLOR
#ifdef DEBUG_FORCE_COLOR
# if ( defined(DEBUG_WINDOWED) && defined(WIN32) )
#  define DEBUG_FORCE_COLOR_WIN
# endif
#endif


#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 ) || defined( ENABLE_DRAW24 ) || defined( ENABLE_DRAW32 )
#define ENABLE_DRAWEXT
#endif

// Chexquest Newmaps has black bars and crosshairs which are drawn at (-1,-1).
// They are all black, not aligned, and the movement is slight,
// so moving the draw is acceptable.
// Enables the accurate clipping.  The default is to move the draw instead.
//#define ENABLE_CLIP_DRAWSCALED


// [WDJ] Interfaces to port video control, common to all

// Each screen is vid.screen_size (which may be larger than width * height)
// width*height is wrong for the Mac, which pads buffer to power of 2
// someone stuck in an extra screen ptr
byte * screens[NUMSCREENS+1];

rendermode_e   rendermode = render_init;
byte  rendermode_recalc = false;  // signal a change

byte  drawmode_recalc = false;

#ifdef HWRENDER
// patches are stored in HWR format, set when HWR rendermode.
byte  HWR_patchstore = false;
#endif

// Request to the drivers.
byte  req_drawmode = DRM_none;  // vid_drawmode_e
byte  req_bitpp = 0;  // DRM_explicit_bpp param
byte  req_alt_bitpp = 0;  // DRM_explicit_bpp param
byte  req_command_video_settings;  // command line
uint16_t  req_width, req_height;

// Driver state
byte  graphics_state = VGS_off; // Is used in console.c and screen.c
byte  native_bitpp;
byte  native_bytepp;
byte  native_drawmode; // vid_drawmode_e

// To disable fullscreen at startup. 
byte  allow_fullscreen = false;


void Set_drawmode_OnChange( void );

// values from vid_drawmode_e
CV_PossibleValue_t drawmode_sel_t[] = {
   {DRM_8pal,"Software 8bit"},
   {DRM_15,"Software 15bit"},
   {DRM_16,"Software 16bit"},
   {DRM_24,"Software 24bit"},
   {DRM_32,"Software 32bit"},
   {DRM_native,"Native"},
#ifdef HWRENDER
   {DRM_opengl,"OpenGL"},
#ifdef SMIF_WIN_NATIVE
   {DRM_minigl, "MiniGL"},
#endif
#ifdef USE_VOODOO_GLIDE
   {DRM_glide, "Glide"},
#endif
#ifdef SMIF_WIN_NATIVE
   {DRM_d3d,   "D3D"},
#endif
#endif
   {0,NULL} };
consvar_t cv_drawmode = { "drawmode", "Software 8bit", CV_SAVE | CV_CALL | CV_CFG1, drawmode_sel_t, Set_drawmode_OnChange  };

// fullscreen or window
const byte vid_mode_table[2] = { MODE_window, MODE_fullscreen };

byte set_drawmode = 255;  // vid_drawmode_e
// Do not use cv_drawmode to keep current drawmode because cv_drawmode can get changed so many ways.
static byte cur_drawmode = 254;  // vid_drawmode_e
const byte num_drawmode_sel = 8;

void Set_drawmode_OnChange( void )
{
    drawmode_recalc = true;
}



// indexed by vid_drawmode_e
byte drawmode_sel_avail[] = {
   0,  // DRM_none
   DRM_8pal,  // 8 bit
#ifdef ENABLE_DRAW15
   DRM_15,
#else
   0,
#endif
#ifdef ENABLE_DRAW16
   DRM_16,
#else
   0,
#endif
#ifdef ENABLE_DRAW24
   DRM_24,
#else
   0,
#endif
#ifdef ENABLE_DRAW32
   DRM_32,
#else
   0,
#endif
   DRM_explicit_bpp,  // -bpp -truecolor -highcolor
   DRM_native,  // Native
#ifdef HWRENDER
   DRM_opengl,  // OpenGL
#ifdef SMIF_WIN_NATIVE
   DRM_minigl,  // MiniGL
#else
   0,
#endif
#ifdef USE_VOODOO_GLIDE
   DRM_glide,   // Glide
#else
   0,
#endif
#ifdef SMIF_WIN_NATIVE
   DRM_d3d,     // D3D
#else
   0,
#endif
#else
   0, 0, 0,
#endif
   0,  // DRM_END
};

// indexed by vid_drawmode_e
static
byte drawmode_to_bpp[] = {0,8,15,16,24,32, 99,99,99,99,99,99,99};

// indexed by vid_drawmode_e
static
byte drawmode_to_rendermode[] = {
   render_soft, // none
   render_soft, // Software 8 bit
   render_soft, // Software 15 bit
   render_soft, // Software 16 bit
   render_soft, // Software 24 bit
   render_soft, // Software 32 bit
   render_soft, // Software req_bitpp
   render_soft, // Software native
   render_opengl, // OpenGL
   render_opengl, // MiniGL
   render_glide,  // Glide
   render_d3d,    // D3D
};

#if 0
// Indexed by rendermode_e
const char * rendermode_name[] = {
    "",
    "Software",
    "OpenGL",
#ifdef SMIF_WIN_NATIVE
    "D3D",
#else
     "",
#endif
#ifdef USE_VOODOO_GLIDE
    "Glide",
#else
     "",
#endif
    "None"
};
#endif



// Not used, conflict.
// There is a DRM_explicit_bpp config file.
// But there is not a DRM_explicit_bpp menu selection.
//#define CONVERT_BP_TO_DRM

#ifdef CONVERT_BP_TO_DRM
// Find the cv_drawmode for a bitpp.
static byte  bpp_to_drawmode( byte bitpp )
{
    byte dm;
    for( dm=0; dm<DRM_END; dm++ )
    {
        if( drawmode_to_bpp[dm] == bitpp )
            return dm;
    }
    return 99;  // invalid
}
#endif

//#define DEBUG_DRAWMODE

// Set rendermode
//  drawmode : vid_drawmode_e
//  change_config : boolean, save current config file, load another
// Called by D_DoomMain, SCR_SetMode
byte  V_switch_drawmode( byte drawmode, byte change_config )
{
    byte old_render = rendermode;

#ifdef DEBUG_DRAWMODE
    GenPrintf( EMSG_debug, "V_switch_drawmode  ( %i )\n", drawmode );
#endif

    if( drawmode >= DRM_END )  return 0;
    // restore of vs settings is last
   
    if( drawmode <= DRM_32 )
    {
        req_bitpp = drawmode_to_bpp[drawmode];
        if( ! V_CanDraw( req_bitpp ) )  goto candraw_reject;
        if( ! VID_Query_Modelist( drawmode, cv_fullscreen.EV, req_bitpp ) )  goto query_reject;
        req_drawmode = DRM_explicit_bpp;  // explicit bpp
        rendermode = render_soft;
    }
    else if( drawmode == DRM_native )  // Native
    {
        if( ! V_CanDraw( native_bitpp ) )  goto candraw_reject;
        if( ! VID_Query_Modelist( DRM_native, cv_fullscreen.EV, native_bitpp ) )  goto query_reject;
        req_drawmode = DRM_native;  // bpp of the default screen
        rendermode = render_soft;
    }
   
    if( drawmode >= DRM_opengl && drawmode < DRM_END )
    {
#ifdef HWRENDER
        // Hardware drawmodes use native or some internal drawmode,
        if( ! VID_Query_Modelist( drawmode, cv_fullscreen.EV, native_bitpp ) )  goto query_reject;
        req_drawmode = drawmode;  // let driver know which hardware mode
        rendermode = drawmode_to_rendermode[ drawmode ];
#else
        goto reject;
#endif
    }
    else if( drawmode == DRM_explicit_bpp )
    {
        // command line, bpp, highcolor, truecolor.
        // req_bitpp and req_alt_bpp have been set by caller

        // Error reporting uses req_bitpp and req_alt_bitpp,
        // so don't change them before error checking is done.

        byte cd = V_CanDraw( req_bitpp );
        byte cd_alt = V_CanDraw( req_alt_bitpp );
        if( !cd && !cd_alt )  goto candraw_reject;  // report error on req_

        if( cd && VID_Query_Modelist( DRM_explicit_bpp, cv_fullscreen.EV, req_bitpp ) )
        {
            // use req_bitpp
        }
        else if( cd_alt && VID_Query_Modelist( DRM_explicit_bpp, cv_fullscreen.EV, req_alt_bitpp ))
        {
            // use the alt
            req_bitpp = req_alt_bitpp;
            req_alt_bitpp = 0;
        }
        else
            goto query_reject;

        req_drawmode = DRM_explicit_bpp;
        rendermode = render_soft;

#ifdef CONVERT_BP_TO_DRM
        // Convert to drawmode valid for cv_drawmode.
        drawmode = bpp_to_drawmode( req_bitpp );
#endif
    }

    // Setup HWR calls so can set values.
    if( (cur_drawmode != drawmode) || (old_render != rendermode) )
    {
#ifdef DEBUG_DRAWMODE
        GenPrintf( EMSG_debug, "V_switch_drawmode  rendermode= %i\n", rendermode );
#endif
        rendermode_recalc = true;  // the only place this is set
    }

    // Any HWR functions triggered by any OnChange functions
    // must have been setup in I_Rendermode_setup.

    // [Arcade] Reconcile the screen depth with the drawmode, on the startup
    // path too.  The checks above validated this drawmode against its own bit
    // depth -- and then SCR_apply_video_settings, which runs next on both
    // paths, throws that away and takes req_bitpp straight from cv_scr_depth.
    // A config whose scr_depth does not match its drawmode therefore asks
    // I_RequestFullGraphics for a depth nothing validated.
    //
    // On a display that does not offer it, that is a dead end rather than a
    // fallback: "No 32 bpp modes", goto no_modes, "Change Graphics failed:
    // err=-102".  A cabinet with scr_depth "32 bits" left over from an OpenGL
    // config, switched to a software drawmode on a 24bpp X server, would not
    // start in that drawmode at all -- and every mode the display offered was
    // 24bpp, including the 320x200 being asked for.
    //
    // The identical assertion below covers the menu path, where it has to be
    // repeated because the drawmode's own config file is loaded in between and
    // carries its own scr_depth.  See docs/arcade/drawmode-switching.md.
    if( (drawmode >= DRM_8pal) && (drawmode <= DRM_32) )
        CV_SetValue( &cv_scr_depth, drawmode_to_bpp[drawmode] );

    // Must not execute on first drawmode change.
    if( change_config )
    {
        // Need to change the configfile_drawmode.
        if( cur_drawmode < DRM_END )
        {
            M_SaveConfig( CFG_drawmode, configfile_drawmode );
        }

        // Remove the config values from the previous drawmode.
        M_ClearConfig( CFG_drawmode );

        if( drawmode < DRM_END )
        {
#ifdef DEBUG_DRAWMODE
            GenPrintf( EMSG_debug, "V_switch_drawmode  LoadConfig\n" );
#endif
            // Change to the new drawmode config file.
            M_Set_configfile_drawmode( drawmode );
            // WARNING : this do a "COM_BufExecute()"
            M_LoadConfig( CFG_drawmode, configfile_drawmode );

            // [Arcade] Reconcile the screen depth with the drawmode.
            //
            // Each drawmode has its own config file carrying its own
            // scr_depth, and nothing checked it against the drawmode itself.
            // The modelist query above correctly used this drawmode's bit
            // depth, but the config just loaded overwrites cv_scr_depth, and
            // SCR_apply_video_settings takes req_bitpp straight from it -- so
            // a palette drawmode whose config says "32 bits" goes on to ask
            // for a video mode that cannot exist.
            //
            // The failure is silent and looks like a cabinet lockup: the
            // teardown for the new rendermode has already run, the graphics
            // change then fails ("No 32 bpp modes"), and the engine carries
            // on running against a dead display.  The game is still alive --
            // measured at over 900000 main-loop iterations afterwards -- but
            // the screen never comes back, so it reads as a freeze.
            //
            // The drawmode's own depth is not negotiable, so assert it here.
            // This also makes an already-wrong config file harmless, which
            // matters because create_initial_drawmode_config() writes
            // whatever depth was current when the file was created.
            if( (drawmode >= DRM_8pal) && (drawmode <= DRM_32) )
                CV_SetValue( &cv_scr_depth, drawmode_to_bpp[drawmode] );

            SCR_apply_video_settings( 1 );  // setmodeneeded
        }
    }

    if( drawmode < DRM_END )
    {
        cur_drawmode = drawmode;
        // save the new drawmode as temporary
        cv_drawmode.EV = drawmode;
        cv_drawmode.state |= CS_EV_PROT;  // protect against restore
        // To change the save value:  CV_SetValue( &cv_drawmode, cv_drawmode.EV );
    }

    return 1;

// Error handling
candraw_reject:
    if( verbose )
    {
        if( req_alt_bitpp )
            GenPrintf( EMSG_ver, "Cannot draw %i bitpp, nor %i bitpp.\n", req_bitpp, req_alt_bitpp );
        else
            GenPrintf( EMSG_ver, "Cannot draw %i bitpp.\n", req_bitpp );
    }
    goto reject;

query_reject:
    if( verbose )
        GenPrintf( EMSG_ver, "No modes for %s, %i bitpp\n", cv_fullscreen.EV ? "Fullscreen":"Window", req_bitpp );

reject:
#ifdef DEBUG_DRAWMODE
    GenPrintf( EMSG_debug, "  V_switch_drawmode  ( %i )  REJECT\n", drawmode );
#endif
    return 0;
}



// Darker background
CV_PossibleValue_t darkback_sel_t[] = {
   {0,"Half"},
   {1,"Med"},
   {2,"Dark"},
   {0,NULL} };
consvar_t cv_darkback = { "darkback", "1", CV_SAVE, darkback_sel_t, NULL };

void CV_fontsize_OnChange(void)
{
    con_recalc = 1;
}

CV_PossibleValue_t fontsize_t[] = {
   {1,"Small"},
   {2,"Med2"},
   {3,"Med3"},
   {4,"Med4"},
   {5,"Large"},
   {0,NULL} };
consvar_t cv_con_fontsize =
  { "con_fontsize", "Med2", CV_SAVE|CV_CALL, fontsize_t, CV_fontsize_OnChange };
consvar_t cv_msg_fontsize =
  { "msg_fontsize", "Large", CV_SAVE|CV_CALL, fontsize_t, CV_fontsize_OnChange };

// Controls FinalText output.
CV_PossibleValue_t textout_sel_t[] = {
   {0,"Off"},
   {1,"Vanilla"},
   {2,"UTF8"},
   {0,NULL} };
consvar_t cv_textout = { "textout", "2", CV_SAVE, textout_sel_t, NULL };


CV_PossibleValue_t ticrate_sel_t[] = {
   {0,"Off"},
   {1,"Graph"},
   {2,"Numeric"},
   {0,NULL} };
consvar_t cv_ticrate = { "vid_ticrate", "0", 0, ticrate_sel_t, NULL };


// synchronize page flipping with screen refresh
// unused and for compatibility reason
consvar_t cv_vidwait = {"vid_wait", "1", CV_SAVE, CV_OnOff};



void CV_usegamma_OnChange();
void CV_gammafunc_OnChange();
// In m_menu.c
void MenuGammaFunc_dependencies( byte gamma_en,
                                 byte black_en, byte bright_en );

CV_PossibleValue_t gamma_func_t[] = {
   {0,"Gamma"},
   {1,"Gamma_black"},
   {2,"Gamma_full"},  // gamma bright black controls
   {3,"Linear"},
   {0,NULL} };
consvar_t cv_gammafunc = { "gammafunc", "2", CV_SAVE | CV_CALL, gamma_func_t, CV_gammafunc_OnChange };
CV_PossibleValue_t gamma_bl_cons_t[] = { {-12, "MIN"}, {12, "MAX"}, {0, NULL} };
consvar_t cv_black = { "black", "0", CV_VALUE | CV_SAVE | CV_CALL, gamma_bl_cons_t, CV_usegamma_OnChange };
CV_PossibleValue_t gamma_br_cons_t[] = { {-12, "MIN"}, {12, "MAX"}, {0, NULL} };
consvar_t cv_bright = { "bright", "0", CV_VALUE | CV_SAVE | CV_CALL, gamma_br_cons_t, CV_usegamma_OnChange };
CV_PossibleValue_t gamma_cons_t[] = { {-12, "MIN"}, {12, "MAX"}, {0, NULL} };
consvar_t cv_usegamma = { "gamma", "0", CV_VALUE | CV_SAVE | CV_CALL, gamma_cons_t, CV_usegamma_OnChange };

static byte gammatable[256];	// shared by all gamma table generators

static void put_gammatable( int i, float fv )
{
#ifdef __USE_ISOC99
    // roundf is ISOC99
    int gv = (int) roundf( fv );
#else
    int gv = (int) rint( fv );
#endif
    if( gv < 0 )
        gv = 0; 
    if( gv > 255 )
        gv = 255;
    gammatable[i] = gv;
}

// Build a gamma table
static void R_BuildGammaTable(float gamma)
{
    int i;

    // Calculate gammatable anew each time.
    for (i=0; i<256; i++)
    {
        // Split this calculation, and use the put_gammatable function
        // to control possible errors in non-Linux systems.
        double di = (double)(i+1) / 256.0;
        put_gammatable( i, pow( di, gamma) * 255.0f );
    }
}

// Declaring table with f literal (1.28f) seems to make no difference.
// table of gamma value for each slider position
float gamma_lookup_table[25] = {
    1.48, 1.44, 1.4, 1.36, 1.32, 1.28, 1.24, 1.2, 1.16, 1.12, 1.08, 1.04,
    1.0,	// doom gamma table 1   // at index 0
    0.96, 0.92,
    0.88,	// doom gamma table 2
    0.836, 0.793,
    0.75,	// doom gamma table 3
    0.706, 0.663,
    0.62,	// doom gamma table 4
    0.58, 0.54,
    0.50	// doom gamma table 5
};

// ind: -12 .. +12
static inline float gamma_lookup( int ind )
{
    return gamma_lookup_table[ ind + 12 ];
}


// Generate a power law table from gamma, plus a black level offset
static void
  R_Generate_gamma_black_table( void )
{
    int i;
//   float b0 = ((float) cv_black.value ) * (16.0 / 12.0); // black
    float b0 = ((float) cv_black.value ) / 2.0f; // black
    float pow_max = 255.0f - b0;
    float gam = gamma_lookup( cv_usegamma.value );  // gamma

    gammatable[0] = 0;	// absolute black

    for( i=1; i<=255; i++ )
    {
        float fi = ((float) i) / 255.0f;
        put_gammatable( i, b0 + (powf( fi, gam ) * pow_max) );
    }
}

#if 0
// Generate a power curve table from gamma,
// with a power curve black level adjustment
static void
  R_Generate_gamma_black_adj_table( void )
{
    // limits of black adjustment
#  define BLACK_SIZE  48
    int i, gv;
    float gvf;
    float gam = gamma_lookup( cv_usegamma.value );  // gamma
    float blkgam = gamma_lookup( cv_black.value ); // black

    gammatable[0] = 0;	// absolute black

    for( i=1; i<=255; i++ )
    {
        float fi = ((float) i) / 255.0f;
        gvf = powf( fi, gam ) * 255.0f;
        if( i < BLACK_SIZE )
        {
            // Black adjustment, using a power function over the black range.
            // At neutral, powf = i, so adj = powf - i.
            fi = ((float) i) / BLACK_SIZE;
            gvf += (powf( fi, blkgam ) * BLACK_SIZE) - ((float)i);
        }
        put_gammatable( i, gvf );
    }
}
#endif

// Generate a gamma with black adj, and bright adj
static void
  R_Generate_gamma_bright_black_table( void )
{
#  define BRIGHT_MIN  60
#  define BRIGHT_MID  130
    int i, di, start_index, end_index;
    float bf = ((float)cv_bright.value) * (256.0f / 6.0f / 12.0f);
    float n3 = bf*bf*bf;  // -1728 .. 1728
    float d2 = bf*bf;  // 144 .. 0 .. 144
    float gf, w0;

    R_Generate_gamma_black_table();

    // The following only modifies the gamma table with brightness.
    // Linux handles d2=0 without error, but MINGW does not.
    if( d2 < 0.1 )  return;
   
    // bright correct using curve: witch of agnesi
    // y = (d**3)/(x**2 + d**2)
    // MIN to MID
    start_index = BRIGHT_MIN;
    end_index = BRIGHT_MID;
    do
    {
        di = end_index - start_index;
        w0 = (n3 / ( (di*di) + d2 )) / di; 	// witch at low point / di
        for( i=start_index; i<=end_index; i++ )
        {
            di = abs(BRIGHT_MID - i);
            gf = n3 / ( (di*di) + d2 );	// witch of agnesi
            gf -= w0 * di; // smooth transition on tail
            // add adjustment to table
            put_gammatable( i, gammatable[i] + gf );
        }
        // MID to 255
        start_index = BRIGHT_MID + 1;
        end_index = 255;
    } while( i < 255 );
}

static void
  R_Generate_smooth5_linear_gamma_table( void )
{
    const int bl_index = 28;
    const int bl_ref_offset = 20; // (8 .. 28 .. 50);
    const int wl_index = 128;
    const int wl_ref_offset = 48; // (60 .. 128 .. 176);
    float bl_offset = ((float) cv_black.value ) * bl_ref_offset / 12.0f;
    float wl_offset = ((float) cv_bright.value ) * wl_ref_offset / 12.0f;
    float b0 = 0.0, lf = 1.0;
    int i, start_index, end_index, seg = 0;

    // monotonic checks
    if( (wl_offset + wl_index) < (bl_offset + bl_index + 5) ) {
        // enforce monotonic by altering wl
        wl_offset = bl_offset + bl_index + 5 - wl_index;
    }
    if( (wl_offset + wl_index) > 250.0f ) {
        // enforce monotonic by altering wl
        wl_offset = 250.0f - wl_index;
    }
    // eqn: bl_offset = ( b0 + (lf * bl_index))
    b0 = bl_offset * 5 / 16;
    if( b0 < 0.0 ) b0 = 0;
    gammatable[0] = 0;	// absolute black
    gammatable[1] = (b0 * 5)/16;	// near black
    gammatable[2] = (b0 * 11)/16;
    gammatable[3] = (b0 * 15)/16;

    // generate rest of table in three linear segments
    end_index = 3; // start at 4
    for( seg=0; seg<=2; seg++ )
    {
        start_index = end_index + 1;
        switch( seg )
        {
         case 0:
           // linear from [1] to [bl_index]
           end_index = bl_index;
           lf = (bl_offset - b0) / bl_index;
           break;
         case 1:
           // linear from [bl_index+1] to [wl_index]
           // eqn: bl_index + bl_offset = bl_index + ( b0 + (lf * bl_index))
           // eqn: wl_index + wl_offset = wl_index + ( b0 + (lf * wl_index))
           end_index = wl_index;
           lf = ( wl_offset - bl_offset ) / ( wl_index - bl_index );
           b0 = bl_offset - (lf * bl_index);
           break;
         case 2:
           // linear from [wl_index+1] to [255]
           end_index = 255;
           lf =  - wl_offset / (255 - wl_index);
           b0 = wl_offset - (lf * wl_index);
           break;
        }
      
        for( i=start_index; i<=end_index; i++ )
        {
            put_gammatable( i, (b0 + ( lf * i ) + i)); // linear
            // smooth over 5 using weights 3 3 4 3 3
            gammatable[i-2] = ((gammatable[i-4] + gammatable[i-3]
                                + gammatable[i-1] + gammatable[i])*3
                               + gammatable[i-2]*4 ) / 16;
        }
    }
}


// [WDJ] Default palette for Launch, font1, error messages
#define DEFAULT_PALSIZE  (3*9)
byte default_pal[ DEFAULT_PALSIZE ] = {
    0, 0, 0,
    10, 10, 10,
    253, 253, 253,
    250, 0, 0,
    0, 250, 0,
    0, 0, 250,
    120, 0, 0,  // protection for DEBUG_FORCE_COLOR
    0, 120, 0,
    0, 0, 120,
};

// local copy of the palette for V_GetColor()
RGBA_t *pLocalPalette = NULL;
#ifdef DEBUG_FORCE_COLOR_WIN
int    num_palette = 0;
#endif

// Update working palette when palette loaded, or gamma changes.
// Keep a copy of the palette so that we can get the RGB
// value for a color index at any time.
static void LoadPalette(const char *lumpname)
{
  lumpnum_t ln;
  int palsize, i;
  byte * pal;  // stepping through pal lump;
   
  if( ! VALID_LUMP( W_CheckNumForName( lumpname ) ) )
  {
      // [WDJ] Missing palette lump will be detected later,
      // but need a palette to print the messages.
      palsize = DEFAULT_PALSIZE / 3;
      pal = default_pal;
  }
  else
  {
      // load the palette from a wad
      ln = W_GetNumForName(lumpname);
      // the palsize will be multiples of 256, ( and 3 colors )
      palsize = W_LumpLength(ln) / 3;
      pal = W_CacheLumpNum(ln, PU_CACHE);
  }

  if (pLocalPalette)
    Z_Free(pLocalPalette);

  pLocalPalette = Z_Malloc(sizeof(RGBA_t) * palsize, PU_STATIC, NULL);
#ifdef DEBUG_FORCE_COLOR_WIN
  num_palette = palsize >> 8;  // number of 256 byte palettes
#endif

  for (i = 0; i < palsize; i++)
  {
      pLocalPalette[i].s.red = gammatable[*pal++];
      pLocalPalette[i].s.green = gammatable[*pal++];
      pLocalPalette[i].s.blue = gammatable[*pal++];
//        if( (i&0xff) == HWR_PATCHES_CHROMAKEY_COLORINDEX )
//            pLocalPalette[i].s.alpha = 0;
//        else
      pLocalPalette[i].s.alpha = 0xff;
  }

  // update our console colors from whatever is available
  // in palette 0 of pLocalPalette
  ci_black = NearestColor( 0, 0, 0 );
  ci_white = NearestColor( 250, 250, 250 );
  ci_green = NearestColor( 0, 250, 0 );
  ci_grey = NearestColor( 10, 10, 10 );
}

// -------------+
// V_SetPalette : Set the current palette to use for palettized graphics
//              : (that is, most if not all of Doom's original graphics)
// -------------+
// Called by D_Display, SCR_Startup, SCR_SetMode, SB_PaletteFlash
// Called by ST_doPaletteStuff, ST_Stop, CV_usegamma_OnChange, CV_Gammaxxx_ONChange
void V_SetPalette(int palettenum)
{
    // vid : from video setup
    if (!pLocalPalette)
        LoadPalette("PLAYPAL");

#ifdef DEBUG_FORCE_COLOR_WIN
    // Enable when DEBUG and cannot see text.
    if( palettenum <= num_palette )
    {
        // Palette fix during debug, otherwise black text on black background
        RGBA_t * pp = &pLocalPalette[palettenum * 256];
        if( pp[6].s.red < 96 )
            pp[6].s.red = 96;  // at least get red text on black
        if( pp[7].s.green < 96 )
            pp[7].s.green = 96;  // at least get green text on black
    }
#endif

#ifdef HWRENDER
    if( rendermode != render_soft )
        HWR_SetPalette(&pLocalPalette[palettenum * 256]);
    else
#endif
    {
#ifdef ENABLE_DRAWEXT
        if ( vid.bytepp > 1 )  // highcolor, truecolor
            R_Init_color8_translate(&pLocalPalette[palettenum * 256]);  // palette change
        else
#endif
        {
#ifdef ENABLE_DRAW8_USING_12
            R_Init_color12_translate( &pLocalPalette[palettenum * 256] );
#endif
            I_SetPalette(&pLocalPalette[palettenum * 256]);
        }
    }
}

// Called by finale: F_DrawHeretic, F_Responder
void V_SetPaletteLump(const char *pal)
{
    // vid : from video setup
    LoadPalette(pal);

#ifdef HWRENDER
    if( rendermode != render_soft )
        HWR_SetPalette(pLocalPalette);
    else
#endif
    {
#ifdef ENABLE_DRAWEXT
        if ( vid.bytepp > 1 )  // highcolor, truecolor
            R_Init_color8_translate(pLocalPalette);  // palette change
        else
#endif
        {
#ifdef ENABLE_DRAW8_USING_12
            R_Init_color12_translate( pLocalPalette );
#endif
            I_SetPalette(pLocalPalette);
        }
    }
}

void CV_usegamma_OnChange(void)
{
    switch( cv_gammafunc.EV ){
     case 1:
        R_Generate_gamma_black_table();
        break;
     case 2:
        R_Generate_gamma_bright_black_table();
        break;
     case 3:
        R_Generate_smooth5_linear_gamma_table();
        break;
     case 0:
     default:
        R_BuildGammaTable( gamma_lookup( cv_usegamma.value));
        break;
#if 0       
    // old-style gamma levels are defined by gamma == 1-0.125*cv_usegamma.value
    R_BuildGammaTable(1.0f - 0.125f * cv_usegamma.value);
#endif
    }
    if( graphics_state >= VGS_active )
    {
        // reload palette
        LoadPalette("PLAYPAL");
        V_SetPalette(0);
    }
}

enum{ GFU_GAMMA = 0x01, GFU_BLACK = 0x02, GFU_BRIGHT = 0x04 };
byte  gammafunc_usage[4] =
{
     GFU_GAMMA,  // gamma
     GFU_GAMMA | GFU_BLACK, // gamma_black
     GFU_GAMMA | GFU_BLACK | GFU_BRIGHT,  // gamma_bright_black
     GFU_BLACK | GFU_BRIGHT,  // Linear
};
  
void CV_gammafunc_OnChange(void)
{
    byte gu = gammafunc_usage[cv_gammafunc.EV];
    MenuGammaFunc_dependencies( gu&GFU_GAMMA, gu&GFU_BLACK, gu&GFU_BRIGHT );
    CV_usegamma_OnChange();
}

static
consvar_t * video_cvar_list[] =
{
  &cv_vidwait,
  &cv_ticrate,
  &cv_textout,
  &cv_darkback,
  &cv_con_fontsize,
  &cv_msg_fontsize,
  &cv_drawmode,
    // Needs be done for config loading
  &cv_usegamma,
  &cv_black,
  &cv_bright,
  &cv_gammafunc,

    // Screen
  &cv_fullscreen,     // only for opengl so use differant name please and move it to differant place
  &cv_scr_depth,
  &cv_scr_width,
  &cv_scr_height,
  &cv_fuzzymode,
  NULL
};

// [WDJ] Init before calling port video control.
// Common init to all port video control.
// Register video interface controls
// Called once
void V_Init_VideoControl( void )
{
    // vid : from video setup
    // default size for startup
    vid.width = INITIAL_WINDOW_WIDTH;
    vid.height = INITIAL_WINDOW_HEIGHT;
   
    vid.display = NULL;
    vid.screen1 = NULL;
    vid.buffer = NULL;
    vid.recalc = true;

    vid.bytepp = 1; // not optimized yet...
    vid.bitpp = 8;

    vid.modenum = (modenum_t){ MODE_window, 0 };
    rendermode = render_soft;

    CV_RegisterVar_list( video_cvar_list );
}


#ifdef DIRTY_RECT
// [WDJ] Only kept in case want to put game on handheld device, limited CPU.
// V_MarkRect : this used to refresh only the parts of the screen
//              that were modified since the last screen update
//              it is useless today
//
int dirtybox[4];
void V_MarkRect(int x, int y, int width, int height)
{
    M_AddToBox(dirtybox, x, y);
    M_AddToBox(dirtybox, x + width - 1, y + height - 1);
}
#endif

// [WDJ] 2012-02-06 Draw functions for all bpp, bytepp, and padded lines.

// Return true if engine can draw using bitpp
boolean V_CanDraw( byte bitpp )
{
    if( bitpp==8
#ifdef ENABLE_DRAW15
        || (bitpp==15)
#endif
#ifdef ENABLE_DRAW16
        || (bitpp==16)
#endif
#ifdef ENABLE_DRAW24
        || (bitpp==24)
#endif
#ifdef ENABLE_DRAW32
        || (bitpp==32)
#endif
       ) return 1;
    return 0;
}

// [WDJ] Common calc of the display buffer address for an x and y
byte * V_GetDrawAddr( int x, int y )
{
    // vid : from video setup
    return  vid.display + (y * vid.ybytes) + (x * vid.bytepp);
}

#ifdef ENABLE_DRAWEXT
// [WDJ] Draw a palette color to a single pixel
void V_DrawPixel(byte * line, int x, byte color)
{
    // vid : from video setup
    switch(vid.drawmode)
    {
     default:
     case DRAW8PAL:
        line[x] = color;
        break;
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
     case DRAW15:
     case DRAW16:
        {
            register uint16_t * s16 = (uint16_t*) line;
            s16[x] = color8.to16[ color ];
        }
        break;
#endif
#ifdef ENABLE_DRAW24
     case DRAW24:
        {
            pixelunion32_t c32;
            c32.ui32 = color8.to32[ color ];
            register pixel24_t * s24 = (pixel24_t*) line;
            s24[x] = c32.pix24;
        }
        break;
#endif       
#ifdef ENABLE_DRAW32
     case DRAW32:
        {
            register uint32_t * s32 = (uint32_t*) line;
            s32[x] = color8.to32[ color ];
        }
        break;
#endif
    }
}
#else
#if 0
// [WDJ] Draw a palette color to a single pixel
void V_DrawPixel(byte * line, int x, byte color)
{
   line[x] = color;
}
#else
// Degenerate case when only have DRAW8PAL, and want to save calls locally
# define  V_DrawPixel( line, x, color)     (line)[(x)]=(color)
#endif
#endif

// [WDJ] Draw a palette src to a screen line
void V_DrawPixels(byte * line, int x, int count, byte* src)
{
    // vid : from video setup
    switch(vid.drawmode)
    {
     default:
     case DRAW8PAL:
        memcpy( &line[x], src, count );
        break;
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
     case DRAW15:
     case DRAW16:
        line += x * 2;
        while(count--)
        {
            *(uint16_t*)line = color8.to16[ *(src++) ];
            line += 2;
        }
        break;
#endif
#ifdef ENABLE_DRAW24
     case DRAW24:
        line += x * 3;  // 3 byte per pixel
        while(count--)
        {
            pixelunion32_t c32;
            c32.ui32 = color8.to32[ *(src++) ];
            *(pixel24_t*)line = c32.pix24;
            line += 3;
        }
        break;
#endif
#ifdef ENABLE_DRAW32
     case DRAW32:
        line += x * 4;
        while(count--)
        {
            *(uint32_t*)line = color8.to32[ *(src++) ];
            line += 4;
        }
        break;
#endif
    }
}


//
// V_CopyRect
//
// position and width is in src pixels
// srcsrcn, destscn include V_SCALESTART flag
void V_CopyRect(int srcx, int srcy, int srcscrn, int width, int height, int destx, int desty, int destscrn)
{
    // vid : from video setup
    byte *src;
    byte *dest;

    // WARNING don't mix
    if ((srcscrn & V_SCALESTART) || (destscrn & V_SCALESTART))
    {
        srcx *= vid.dupx;
        srcy *= vid.dupy;
        width *= vid.dupx;
        height *= vid.dupy;
        destx *= vid.dupx;
        desty *= vid.dupy;
    }
    srcscrn &= V_SCREENMASK;
    destscrn &= V_SCREENMASK;

#ifdef RANGECHECK
    if (srcx < 0 || srcx + width > vid.width || srcy < 0 || srcy + height > vid.height || destx < 0 || destx + width > vid.width || desty < 0 || desty + height > vid.height || (unsigned) srcscrn > 4
        || (unsigned) destscrn > 4)
    {
        I_Error("Bad V_CopyRect %d %d %d %d %d %d %d %d", srcx, srcy, srcscrn, width, height, destx, desty, destscrn);
    }
#endif
#ifdef DIRTY_RECT
    V_MarkRect(destx, desty, width, height);
#endif

#ifdef DEBUG_COPYRECT
    CONS_Printf("V_CopyRect: vidwidth %d screen[%d]=%x to screen[%d]=%x\n", vid.width, srcscrn, screens[srcscrn], destscrn, screens[destscrn]);
    CONS_Printf("..........: srcx %d srcy %d width %d height %d destx %d desty %d\n", srcx, srcy, width, height, destx, desty);
#endif

    // [WDJ] Copy screens, by line, padded, 8bpp .. 32bpp
    src = screens[srcscrn] + (srcy * vid.ybytes) + (srcx * vid.bytepp);
    dest = screens[destscrn] + (desty * vid.ybytes) + (destx * vid.bytepp);
    width *= vid.bytepp;

    for (; height > 0; height--)
    {
        memcpy(dest, src, width);
        src += vid.ybytes;
        dest += vid.ybytes;
    }
}


#if !defined(USEASM) || defined(WIN_NATIVE)
// --------------------------------------------------------------------------
// Copy a rectangular area from one bitmap to another (8bpp)
// srcPitch, destPitch : width of source and destination bitmaps
// --------------------------------------------------------------------------
// width is in bytes (defined by ASM routine)
void VID_BlitLinearScreen(byte * srcptr, byte * destptr, int width, int height, int srcrowbytes, int destrowbytes)
{
    // vid : from video setup
    if (srcrowbytes == destrowbytes && width == vid.widthbytes)
        memcpy(destptr, srcptr, srcrowbytes * height);
    else
    {
        while (height--)
        {
            memcpy(destptr, srcptr, width);

            destptr += destrowbytes;
            srcptr += srcrowbytes;
        }
    }
}
#endif

// clear to black
void V_Clear_Display( void )
{
    // vid : from video setup
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Screen vid.b
        HWR_DrawVidFill(0, 0, vid.width, vid.height, 0);
    }
    else
#endif
    {
        if( vid.display )
           memset( vid.display, 0, vid.screen_size );
    }
}

// [WDJ] parameterized draw, used by V_DrawScaled, V_DrawMapped
// From V_SetupDraw
drawinfo_t  drawinfo;
  
// [WDJ] setup drawinfo for window, scaling and flag options
// Normally, also calls V_SetupFont for Large text.
//  screenflags : combination of drawflags_e
// usage:
//  desttop = drawinfo.drawp + (y * drawinfo.y0bytes) + (x * drawinfo.x0bytes);
//  destend = desttop + (patch->width * drawinfo.xbytes);  // test against desttop
void V_SetupDraw( uint32_t screenflags )
{
    // vid : from video setup
    // save current draw
    drawinfo.prev_screenflags = drawinfo.screenflags;

    drawinfo.screenflags = screenflags;
    drawinfo.effectflags = drawinfo.screen_effectflags = screenflags & V_EFFECTMASK;

    if (screenflags & V_FINESCALEPATCH)
    {   // Fine scaling, Scaled text
        // Sizing slider factor is set in drawfont.ratio by V_SetFont.
        drawinfo.fdupx = (vid.fdupx * drawfont.ratio) + (1.0 - drawfont.ratio);
        drawinfo.fdupy = (vid.fdupy * drawfont.ratio) + (1.0f - drawfont.ratio);
        drawinfo.dupx = (int)(drawinfo.fdupx + 0.5f);
        drawinfo.dupy = (int)(drawinfo.fdupy + 0.5f);
    }
    else if (screenflags & V_SCALEPATCH)
    {   // Scaled patches and Large text.
        drawinfo.dupx = vid.dupx;
        drawinfo.dupy = vid.dupy;
        drawinfo.fdupx = vid.fdupx;
        drawinfo.fdupy = vid.fdupy;
    }
    else
    {   // Unscaled and Small text.
        drawinfo.dupx = drawinfo.dupy = 1;
        drawinfo.fdupx = drawinfo.fdupy = 1.0f;
    }
    drawinfo.ybytes = drawinfo.dupy * vid.ybytes;  // bytes per source line
    drawinfo.xbytes = drawinfo.dupx * vid.bytepp;  // bytes per source pixel

    // [Arcade] The scale the software drawers actually step by.
    //
    // Stock only ever had the whole number dupx/dupy, so a 320x200 layout came
    // out (320*dupx) x (200*dupy) and the rest of the screen was left over --
    // 1280x600 of a 1366x768 cabinet screen, with a flat-tiled band down two
    // sides and across the bottom.  The hardware renderer never had this,
    // because it scales by the exact fdupx/fdupy.
    //
    // V_SCALEEXACT asks for the same exact scale in software.  Without it the
    // scale is the whole number as before, so this is arithmetically identical
    // to what stock did for every screen that does not ask.
    if( screenflags & V_SCALEEXACT )
    {
        drawinfo.x_scale = (fixed_t)( drawinfo.fdupx * FRACUNIT );
        drawinfo.y_scale = (fixed_t)( drawinfo.fdupy * FRACUNIT );
    }
    else
    {
        drawinfo.x_scale = drawinfo.dupx << FRACBITS;
        drawinfo.y_scale = drawinfo.dupy << FRACBITS;
    }
    if( drawinfo.x_scale < 1 )  drawinfo.x_scale = FRACUNIT;
    if( drawinfo.y_scale < 1 )  drawinfo.y_scale = FRACUNIT;

    drawinfo.x_unitfrac = FixedDiv(FRACUNIT, drawinfo.x_scale);
    drawinfo.y_unitfrac = FixedDiv(FRACUNIT, drawinfo.y_scale);


    if (screenflags & V_SCALESTART)
    {
        drawinfo.dupx0 = vid.dupx;  // scaled
        drawinfo.dupy0 = vid.dupy;
#ifdef HWRENDER
        drawinfo.fdupx0 = vid.fdupx;
        drawinfo.fdupy0 = vid.fdupy;
#endif
    }
    else
    {
        drawinfo.dupx0 = 1;  // unscaled
        drawinfo.dupy0 = 1;
#ifdef HWRENDER
        drawinfo.fdupx0 = 1.0f;
        drawinfo.fdupy0 = 1.0f;
#endif
    }
    drawinfo.x0bytes_saved = drawinfo.x0bytes = drawinfo.dupx0 * vid.bytepp;
    drawinfo.y0bytes_saved = drawinfo.y0bytes = drawinfo.dupy0 * vid.ybytes;

    // [Arcade] Start coords scale by dupx0/dupy0, which is not always the
    // patch scale: V_SCALESTART without V_SCALEPATCH positions by vid.dupx
    // while drawing at 1:1.  Mirror dupx0/dupy0 exactly, so this is the same
    // arithmetic as before wherever V_SCALEEXACT is not asked for.
    if( screenflags & V_SCALESTART )
    {
        drawinfo.x0_scale = ( screenflags & V_SCALEEXACT )?
              (fixed_t)( vid.fdupx * FRACUNIT ) : (drawinfo.dupx0 << FRACBITS);
        drawinfo.y0_scale = ( screenflags & V_SCALEEXACT )?
              (fixed_t)( vid.fdupy * FRACUNIT ) : (drawinfo.dupy0 << FRACBITS);
    }
    else
    {
        drawinfo.x0_scale = FRACUNIT;
        drawinfo.y0_scale = FRACUNIT;
    }
    if( drawinfo.x0_scale < 1 )  drawinfo.x0_scale = FRACUNIT;
    if( drawinfo.y0_scale < 1 )  drawinfo.y0_scale = FRACUNIT;
    drawinfo.x0_scale_saved = drawinfo.x0_scale;
    drawinfo.y0_scale_saved = drawinfo.y0_scale;
#ifdef HWRENDER
    drawinfo.fdupx0_saved = drawinfo.fdupx0;
    drawinfo.fdupy0_saved = drawinfo.fdupy0;
#endif
   
    // The screen buffer, at an offset
    drawinfo.start_offset = 0;
    if (screenflags & V_CENTERHORZ)
    {
        // Center horizontally the finale, and other screens in the fullscreen.
        // [Arcade] Against the width actually drawn.  With V_SCALEEXACT the
        // 320 columns already span the screen and this is zero.
        drawinfo.start_offset += (vid.widthbytes - (V_scale_x(BASEVIDWIDTH) * vid.bytepp)) / 2;
    }
    if (screenflags & V_CENTERMENU)
    {
        // Center the menu by adding a left and top margin.
        drawinfo.start_offset = vid.centerofs;
        // as previously was performed by scaleofs.
        // Enabled when the menu is displayed, and crosshairs.
        // The menu is scaled, a round multiple of the original pixels to
        // keep the graphics clean, then it is centered a little.
        // Except the menu, scaled graphics don't have to be centered.
    }
    drawinfo.screen = screenflags & V_SCREENMASK;  // screen number (usually 0)
    drawinfo.screen_start = screens[drawinfo.screen];  // screen buffer [0]
    drawinfo.drawp = drawinfo.screen_start + drawinfo.start_offset;

    if ( ! (screenflags & V_FINESCALEPATCH))
    {
        // Setup the standard scaled font. Pass V_SCALESTART.
        V_SetupFont( 0, NULL, screenflags );
    }
}

// [WDJ] Layered drawing routines (such as DrawString) will have a problem
// with V_SCALESTART needing to be applied to their x,y parameters, and not
// to the x,y they generate for drawing individual characters, and the like.
// Provide support for turning off the SCALESTART, temporarily.
// This only supports two layers of drawing. Not using local saved copies
// gains speed, simplicity, and hides the mechanism.

void  V_SetupDraw_NO_SCALESTART( void )
{
    drawinfo.x0bytes = vid.bytepp;
    drawinfo.y0bytes = vid.ybytes;
    drawinfo.x0_scale = FRACUNIT;  // [Arcade]
    drawinfo.y0_scale = FRACUNIT;
#ifdef HWRENDER
    drawinfo.fdupx0  = 1.0f;
    drawinfo.fdupy0  = 1.0f;
#endif
}


void  V_SetupDraw_Restore_SCALESTART( void )
{
    drawinfo.x0bytes = drawinfo.x0bytes_saved;
    drawinfo.y0bytes = drawinfo.y0bytes_saved;
    drawinfo.x0_scale = drawinfo.x0_scale_saved;  // [Arcade]
    drawinfo.y0_scale = drawinfo.y0_scale_saved;
#ifdef HWRENDER
    drawinfo.fdupx0 = drawinfo.fdupx0_saved;
    drawinfo.fdupy0 = drawinfo.fdupy0_saved;
#endif
}


//
//  V_DrawMappedPatch : like V_DrawScaledPatch, but with a colormap.
//  per drawinfo
//
//
//added:05-02-98:
// [WDJ] all patches are cached endian fixed 1/5/2010
//  x, y : drawinfo coordinates
// Called by draw char/string, menu, wi_stuff (screen0, scaled)
// Called by ST_refreshBackground to draw face on status bar (with flags)
void V_DrawMappedPatch(int x, int y, patch_t * patch, byte * colormap)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    column_t *column;
    byte *source;  // within column
    byte *desttop, *dest;  // within video buffer

    int count;
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches.
    int cur_topdelta;
#endif
    int post_top;  // [Arcade] source row this post starts at
    fixed_t col, wf, ofs;

    // draw a hardware converted patch
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Fully subject to drawinfo.
        HWR_DrawMappedPatch((MipPatch_t *) patch, x, y, drawinfo.effectflags, colormap);
        return;
    }
#endif

    // [WDJ] Draw to screens, by line, padded, 8bpp .. 32bpp
    desttop = drawinfo.drawp + (V_start_y(y) * vid.ybytes) + (V_start_x(x) * vid.bytepp);
    // [WDJ] offsets are subject to DRAWSCALE dup.
    desttop -= (V_scale_y(patch->topoffset) * vid.ybytes) + (V_scale_x(patch->leftoffset) * vid.bytepp);
//    destend = desttop + (V_scale_x(patch->width) * vid.bytepp);  // test against desttop

#ifdef DIRTY_RECT
    if (drawinfo.screen == 0)
        V_MarkRect(x, y, V_scale_x(patch->width), V_scale_y(patch->height));
#endif

    wf = patch->width << FRACBITS;

    for (col=0; col < wf; col += drawinfo.x_unitfrac)
    {
        column = (column_t *) ((byte *) patch + patch->columnofs[col >> FRACBITS]);

#ifdef DEEPSEA_TALL_PATCH
        cur_topdelta = -1;
#endif
        while (column->topdelta != 0xff)
        {
            source = (byte *) column + 3;
#ifdef DEEPSEA_TALL_PATCH
            // [MB] [WDJ] DeePsea tall patch.
            // DeepSea allows the patch to exceed 254 height.
            // A Doom patch has monotonic ascending topdelta values, 0..254.
            // DeePsea tall patches have an optional relative topdelta.	
            // When the column topdelta is <= the current topdelta,
            // it is a DeePsea tall patch relative topdelta.
            if( (int)column->topdelta <= cur_topdelta )
            {
                cur_topdelta += column->topdelta;  // DeePsea relative topdelta
            }
            else
            {
                cur_topdelta = column->topdelta;  // Normal Doom patch
            }
            post_top = cur_topdelta;
#else
            post_top = column->topdelta;
#endif
            // [Arcade] Both ends rounded through the same scale, so this post
            // ends exactly where the next one starts: no seam, no overlap.
            dest = desttop + (V_scale_y(post_top) * vid.ybytes);
            count = V_scale_y(post_top + column->length) - V_scale_y(post_top);

            ofs = 0;
#ifdef ENABLE_DRAWEXT
            if(vid.drawmode != DRAW8PAL)
            {
                while (count--)
                {
                    V_DrawPixel( dest, 0, colormap[ source[ofs >> FRACBITS]] );
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
            }
            else
#endif
            {
                // DRAW8PAL
                while (count--)
                { 
                    *dest = colormap[ source[ofs >> FRACBITS]];
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
        desttop += vid.bytepp;
    }

}


// Limited by box.
//  x, y : draw at screen coordinates, scaled by drawinfo
//  box_x, box_y : box upper left corner
//  box_w, box_h : box size
void V_DrawMappedPatch_Box(int x, int y, patch_t * patch, byte * colormap, int box_x, int box_y, int box_w, int box_h )
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    column_t *column;
    byte *source;  // within column
    byte *dest;  // within video buffer

    int count, draw_x, draw_y, draw_y1, bx1, bx2, by1, by2;
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches.
    int cur_topdelta;
#endif
    int post_top;  // [Arcade] source row this post starts at
    fixed_t col, wf, ofs;

    // draw a hardware converted patch
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Fully subject to drawinfo.
        HWR_DrawMappedPatch((MipPatch_t *) patch, x, y, drawinfo.effectflags, colormap);
        return;
    }
#endif

    // [WDJ] Draw to screens, by line, padded, 8bpp .. 32bpp
    // Offsets are subject to DRAWSCALE dup.
    draw_y1 = (V_start_y(y) * vid.ybytes) - (V_scale_y(patch->topoffset) * vid.ybytes);
    draw_x = (V_start_x(x) * vid.bytepp) - (V_scale_x(patch->leftoffset) * vid.bytepp);
    by1 = (V_start_y(box_y) * vid.ybytes);
    by2 = by1 + (V_scale_y(box_h) * vid.ybytes);
    bx1 = (V_start_x(box_x) * vid.bytepp);
    bx2 = bx1 + (V_scale_x(box_w) * vid.bytepp);

#ifdef DIRTY_RECT
    if (drawinfo.screen == 0)
        V_MarkRect(box_x, box_y, V_scale_x(box_w), V_scale_y(box_h));
#endif

    col = 0;
    if( draw_x < bx1 )  // Left edge of box
    {
        // Clip at left of box
        col = ((bx1 - draw_x) / vid.bytepp) * drawinfo.x_unitfrac;
        draw_x = bx1;
    }

    wf = patch->width << FRACBITS;

    for ( ; col < wf; col += drawinfo.x_unitfrac)
    {
        column = (column_t *) ((byte *) patch + patch->columnofs[col >> FRACBITS]);

#ifdef DEEPSEA_TALL_PATCH
        cur_topdelta = -1;
#endif
        while (column->topdelta != 0xff)
        {
            source = (byte *) column + 3;
#ifdef DEEPSEA_TALL_PATCH
            // [MB] [WDJ] DeePsea tall patch.
            // DeepSea allows the patch to exceed 254 height.
            // A Doom patch has monotonic ascending topdelta values, 0..254.
            // DeePsea tall patches have an optional relative topdelta.	
            // When the column topdelta is <= the current topdelta,
            // it is a DeePsea tall patch relative topdelta.
            if( (int)column->topdelta <= cur_topdelta )
            {
                cur_topdelta += column->topdelta;  // DeePsea relative topdelta
            }
            else
            {
                cur_topdelta = column->topdelta;  // Normal Doom patch
            }
            post_top = cur_topdelta;
#else
            post_top = column->topdelta;
#endif
            // [Arcade] Rounded run bounds; see V_DrawMappedPatch.
            draw_y = draw_y1 + (V_scale_y(post_top) * vid.ybytes);
            count = V_scale_y(post_top + column->length) - V_scale_y(post_top);
            column = (column_t *) ((byte *) column + column->length + 4);  // next column in patch
            ofs = 0;

            if( draw_y < by1 )  // Top of box
            {
                // Clip at top of box
                int diff = ((by1 - draw_y) / vid.ybytes);  // excess count
                count -= diff;
                if( count <= 0 )  continue;
                ofs = diff * drawinfo.y_unitfrac;	   
                draw_y += diff * vid.ybytes;
            }
            if( (draw_y + (count * vid.ybytes)) > by2 )  // Bottom of box
            {
                // Clip at bottom of box
                count = (draw_y - by2) / vid.ybytes;
                if( count <= 0 )  continue;
            }

            dest = drawinfo.drawp + draw_y + draw_x;
#ifdef ENABLE_DRAWEXT
            if(vid.drawmode != DRAW8PAL)
            {
                while (count--)
                {
                    V_DrawPixel( dest, 0, colormap[ source[ofs >> FRACBITS]] );
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
            }
            else
#endif
            {
                // DRAW8PAL
                while (count--)
                { 
                    *dest = colormap[ source[ofs >> FRACBITS]];
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
            }
        }
        draw_x += vid.bytepp;
        if( draw_x > bx2 )  break;  // Right edge of box
    }
}


//  per drawinfo
// with temp patch load to cache
void V_DrawMappedPatch_Name ( int x, int y, const char* name, byte* colormap )
{
   // The patch is used only in this function
   V_DrawMappedPatch ( x, y,
                       W_CachePatchName( name, PU_CACHE ),  // endian fix
                       colormap );
}


//
// V_DrawScaledPatch
//   like V_DrawPatch, but scaled 2,3,4 times the original size and position
//   this is used for menu and title screens, with high resolutions
//  per drawinfo, with V_SCALESTART, V_SCALEPATCH
//
//added:05-02-98:
// default params : scale patch and scale start
// [WDJ] all patches are cached endian fixed 1/5/2010
// Called by menu, status bar, and wi_stuff
void V_DrawScaledPatch(int x, int y, patch_t * patch)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    int count;
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches.
    int cur_topdelta;
#endif
    int post_top;  // [Arcade] source row this post starts at
    fixed_t col = 0;
    column_t *column;
    byte *source;  // within column
    byte *dest, *desttop, *destend;  // within video buffer

    fixed_t ofs;
    fixed_t colfrac;

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Draw a hardware converted patch, using drawinfo scaling.
        HWR_DrawPatch((MipPatch_t *) patch, x, y,
                      drawinfo.effectflags | V_DRAWINFO );
        return;
    }
#endif

    colfrac = drawinfo.x_unitfrac;
   
    // [WDJ] Draw to screens, by line, padded, 8bpp .. 32bpp
    desttop = drawinfo.drawp + (V_start_y(y) * vid.ybytes) + (V_start_x(x) * vid.bytepp);
    // [WDJ] offsets are subject to DRAWSCALE dup.
    desttop -= (V_scale_y(patch->topoffset) * vid.ybytes) + (V_scale_x(patch->leftoffset) * vid.bytepp);
    destend = desttop + (V_scale_x(patch->width) * vid.bytepp);  // test against desttop

#ifndef ENABLE_CLIP_DRAWSCALED
    if( desttop < drawinfo.screen_start )
    {
        // Protect against drawing outside of screen.
        if( y < 0 )
        {
            // Clip y
            desttop = drawinfo.drawp + (V_start_x(x) * vid.bytepp);
        }
        // Compensate for the change in y.
        destend = desttop + (V_scale_x(patch->width) * vid.bytepp);
        if( desttop < drawinfo.screen_start )
        {
            // Clip x too.
            desttop = drawinfo.screen_start;
        }

#if 1
        if( gamemode == chexquest1 && y == -1 )
        {
            // Chexquest Newmaps black bars and crosshairs.
            // Were designed for OpenGL drawing, looks better when stretched.
            x = 0;
            y = 0;
            colfrac = colfrac * (vid.dupx * BASEVIDWIDTH) / vid.width;
            desttop = drawinfo.screen_start;
            destend = desttop + vid.ybytes;
        }
#endif
    }
#endif

    // only used in f_finale:F_CastDrawer
    if (drawinfo.effectflags & V_FLIPPEDPATCH)
    {
        colfrac = -colfrac;
        col = (patch->width << FRACBITS) + colfrac;
    }
    else
        col = 0;

    while( desttop < destend )
    {
        column = (column_t *) ((byte *) patch + patch->columnofs[col >> FRACBITS]);
        col += colfrac;

#ifdef DEEPSEA_TALL_PATCH
        cur_topdelta = -1;
#endif
        while (column->topdelta != 0xff)
        {
            source = (byte *) column + 3;
#ifdef DEEPSEA_TALL_PATCH
            // [MB] [WDJ] DeePsea tall patch.
            // DeepSea allows the patch to exceed 254 height.
            // A Doom patch has monotonic ascending topdelta values, 0..254.
            // DeePsea tall patches have an optional relative topdelta.	
            // When the column topdelta is <= the current topdelta,
            // it is a DeePsea tall patch relative topdelta.
            if( (int)column->topdelta <= cur_topdelta )
            {
                cur_topdelta += column->topdelta;  // DeePsea relative topdelta
            }
            else
            {
                cur_topdelta = column->topdelta;  // Normal Doom patch
            }
            post_top = cur_topdelta;
#else
            post_top = column->topdelta;
#endif
            // [Arcade] Both ends rounded through the same scale, so this post
            // ends exactly where the next one starts: no seam, no overlap.
            dest = desttop + (V_scale_y(post_top) * vid.ybytes);
            count = V_scale_y(post_top + column->length) - V_scale_y(post_top);

            ofs = 0;
#ifdef ENABLE_DRAWEXT
            if(vid.drawmode != DRAW8PAL)
            {
                while (count--)
                {
#ifdef ENABLE_CLIP_DRAWSCALED
                    if( dest >= drawinfo.screen_start )
                       V_DrawPixel( dest, 0, source[ofs >> FRACBITS] );
#else
                    V_DrawPixel( dest, 0, source[ofs >> FRACBITS] );
#endif
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
            }
            else
#endif
            {
                while (count--)
                {
#ifdef ENABLE_CLIP_DRAWSCALED
                    if( dest >= drawinfo.screen_start )
                       *dest = source[ofs >> FRACBITS];
#else
                    *dest = source[ofs >> FRACBITS];
#endif
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
            }

            column = (column_t *) ((byte *) column + column->length + 4);
        }
        desttop += vid.bytepp;
    }
}

//  per drawinfo, with V_SCALESTART, V_SCALEPATCH
// with temp patch load to cache
void V_DrawScaledPatch_Name(int x, int y, const char * name )
{
   // The patch is used only in this function
   V_DrawScaledPatch ( x, y,
                       W_CachePatchName( name, PU_CACHE ) );  // endian fix
}

//  per drawinfo, with V_SCALESTART, V_SCALEPATCH
// with temp patch load to cache
void V_DrawScaledPatch_Num(int x, int y, int patch_num )
{
   // The patch is used only in this function
   V_DrawScaledPatch ( x, y,
                       W_CachePatchNum( patch_num, PU_CACHE ) );  // endian fix
}

#if 0
//[WDJ] 2012-02-06 DrawSmallPatch found to be unused

void HWR_DrawSmallPatch(MipPatch_t * gpatch, int x, int y, int option, byte * colormap);
// Draws a patch 2x as small. SSNTails 06-10-2003
// [WDJ] all patches are cached endian fixed 1/5/2010
void V_DrawSmallScaledPatch(int x, int y, int scrn, patch_t * patch, byte * colormap)
{
    // vid : from video setup
    int count;
    int col;
    column_t *column;
    byte *source;  // within column
    byte *desttop, *dest, *destend;  // within video buffer

    int dupx=1, dupy=1;
    int count_dupy, dup_ybytes;
    int ofs;
    fixed_t colfrac, rowfrac, colfrac_inc, rowfrac_inc;
//    boolean skippixels = false;

    // draw an hardware converted patch
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_DrawSmallPatch((MipPatch_t *) patch, x, y, scrn, colormap);
        return;
    }
#endif

    colfrac = FixedDiv(FRACUNIT, dupx << FRACBITS);
    rowfrac = FixedDiv(FRACUNIT, dupy << FRACBITS);

    if (scrn & V_FLIPPEDPATCH)
    {
        colfrac = -colfrac;
        col = (patch->width << FRACBITS) + colfrac;
    }
    else
        col = 0;

    colfrac_inc = colfrac;
    rowfrac_inc = rowfrac;

    desttop = screens[scrn & 0xFF] + (y * vid.ybytes) + (x * vid.bytepp);
    // [WDJ] offsets are subject to DRAWSCALE dup.
    desttop -= (V_scale_y(patch->topoffset) * vid.ybytes) + (V_scale_x(patch->leftoffset) * vid.bytepp);
    destend = desttop;

    if (vid.dupx > 1 && vid.dupy > 1)
    {
        destend += (patch->width * dupx * vid.bytepp);
        count_dupy = dupy << 1;  // count_dupy = dupy * 2, will be dupy after >> 1
    }
    else
    {
//        skippixels = true;
        // double the inc, halve the count
        destend += (patch->width / 2 * dupx * vid.bytepp);
        colfrac_inc += colfrac_inc;  // * 2
        rowfrac_inc += rowfrac_inc;  // * 2
        count_dupy = dupy; // will be dupy/2 after >> 1
    }
    dup_ybytes = dupy * vid.ybytes;

    // [WDJ] Use same loop for normal and skippixels, with some predefined inc
    for (  ; desttop < destend; desttop+=vid.bytepp)
    {
        column = (column_t *) ((byte *) patch + patch->columnofs[col >> FRACBITS]);
        col += colfrac_inc;
        while (column->topdelta != 0xff)
        {
            source = (byte *) column + 3;
            dest = desttop + (column->topdelta * dup_ybytes);
            count = (column->length * count_dupy) >> 1;  // dupy or dupy/2
            ofs = 0;
            while (count--)
            {
                V_DrawPixel( dest, 0, colormap[source[ofs >> FRACBITS]] );
                dest += vid.ybytes;
                ofs += rowfrac_inc;
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
    }
}
#endif

//added:16-02-98: now used for crosshair
//
//  This draws a patch over a background with translucency
//  per drawinfo
//
// [WDJ] all patches are cached endian fixed 1/5/2010
void V_DrawTranslucentPatch(int x, int y, patch_t * patch)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    int count;
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches.
    int cur_topdelta;
#endif
    int post_top;  // [Arcade] source row this post starts at
    column_t *column;
    byte *source;  // within column
    byte *desttop, *dest;  // within video buffer
    fixed_t ofs;
    fixed_t col, wf;

    // draw an hardware converted patch
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Enable drawinfo scaling.
        HWR_DrawPatch((MipPatch_t *) patch, x, y,
                      drawinfo.screenflags|drawinfo.effectflags|V_DRAWINFO );
        return;
    }
#endif


#ifdef DIRTY_RECT
    if (!(scrn & 0xff))
//    y -= patch->topoffset * drawinfo.dupy;
//    x -= patch->leftoffset * drawinfo.dupx;
        V_MarkRect(x, y, V_scale_x(patch->width), V_scale_y(patch->height));
#endif

    // [WDJ] Draw to screens, by line, padded, 8bpp .. 32bpp
    desttop = drawinfo.drawp + (V_start_y(y) * vid.ybytes) + (V_start_x(x) * vid.bytepp);
    // [WDJ] offsets are subject to DRAWSCALE dup.
    desttop -= (V_scale_y(patch->topoffset) * vid.ybytes) + (V_scale_x(patch->leftoffset) * vid.bytepp);
//    destend = desttop + (V_scale_x(patch->width) * vid.bytepp);  // test against desttop

    wf = patch->width << FRACBITS;

    for ( col=0; col < wf; col += drawinfo.x_unitfrac)
    {
        column = (column_t *) ((byte *) patch + patch->columnofs[col >> FRACBITS]);

#ifdef DEEPSEA_TALL_PATCH
        cur_topdelta = -1;
#endif
        while (column->topdelta != 0xff)
        {
            source = (byte *) column + 3;
#ifdef DEEPSEA_TALL_PATCH
            // [MB] [WDJ] DeePsea tall patch.
            // DeepSea allows the patch to exceed 254 height.
            // A Doom patch has monotonic ascending topdelta values, 0..254.
            // DeePsea tall patches have an optional relative topdelta.	
            // When the column topdelta is <= the current topdelta,
            // it is a DeePsea tall patch relative topdelta.
            if( (int)column->topdelta <= cur_topdelta )
            {
                cur_topdelta += column->topdelta;  // DeePsea relative topdelta
            }
            else
            {
                cur_topdelta = column->topdelta;  // Normal Doom patch
            }
            post_top = cur_topdelta;
#else
            post_top = column->topdelta;
#endif
            // [Arcade] Both ends rounded through the same scale, so this post
            // ends exactly where the next one starts: no seam, no overlap.
            dest = desttop + (V_scale_y(post_top) * vid.ybytes);
            count = V_scale_y(post_top + column->length) - V_scale_y(post_top);

            ofs = 0;
#ifdef ENABLE_DRAWEXT
            switch(vid.drawmode)
            {
             default:
             case DRAW8PAL:
                while (count--)
                {
                    register unsigned int color = source[ofs >> FRACBITS];
                    *dest = translucenttables[ ((color << 8) & 0xFF00) + (*dest & 0xFF) ];
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
                break;
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
             case DRAW15:
             case DRAW16:
                while (count--)
                {
                    register unsigned int color = source[ofs >> FRACBITS];
                    register uint16_t * s16 = (uint16_t*) dest;
                    *s16 =( ((color8.to16[color]>>1) & mask_01111) +
                            (((*s16)>>1) & mask_01111) );
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
                break;
#endif
#ifdef ENABLE_DRAW24
             case DRAW24:
                while (count--)
                {
                    register unsigned int color = source[ofs >> FRACBITS];
                    pixelunion32_t c32;
                    c32.ui32 = (color8.to32[ color ]>>1) & 0x7F7F7F; // 01111111 on pix24
                    register pixel24_t * s24 = (pixel24_t*) dest;
                    s24->r = c32.pix24.r + (s24->r>>1);
                    s24->g = c32.pix24.g + (s24->g>>1);
                    s24->b = c32.pix24.b + (s24->b>>1);
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
                break;
#endif
#ifdef ENABLE_DRAW32
             case DRAW32:
                while (count--)
                {
                    register unsigned int color = source[ofs >> FRACBITS];
                    register uint32_t * s32 = (uint32_t*) dest;
                    *s32 = ((color8.to32[ color ]>>1) & 0x007F7F7F)
                         + (((*s32)>>1) & 0x007F7F7F) + (*s32 & 0xFF000000);
                    dest += vid.ybytes;
                    ofs += drawinfo.y_unitfrac;
                }
                break;
#endif
            }
#else
            // Degenerate DRAW8PAL only
            while (count--)
            {
                register unsigned int color = source[ofs >> FRACBITS];
                *dest = translucenttables[ ((color << 8) & 0xFF00) + (*dest & 0xFF) ];
                dest += vid.ybytes;
                ofs += drawinfo.y_unitfrac;
            }
#endif

            column = (column_t *) ((byte *) column + column->length + 4);
        }
        desttop += vid.bytepp;
    }
}

//
// V_DrawPatch
// Masks a column based masked pic to the screen. NO SCALING!!!
//
// [WDJ] all patches are cached endian fixed 1/5/2010
// Called by R_FillBackScreen, map
void V_DrawPatch(int x, int y, int scrn, patch_t * patch)
{
    // vid : from video setup
    column_t *column;
    byte *source;  // within column
    byte *desttop, *dest;  // within video buffer
    int count;
#ifdef DEEPSEA_TALL_PATCH
    // [MB] [WDJ]  Support for DeePsea tall patches.
    int cur_topdelta;
#endif
    int col, wi;

    // draw an hardware converted patch
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Vid coordinates.
        HWR_DrawPatch((MipPatch_t *) patch, x, y, V_NOSCALE);
        return;
    }
#endif

    // No scaling, so can apply offsets in the old method.
    y -= patch->topoffset;
    x -= patch->leftoffset;
#ifdef RANGECHECK
    if (x < 0 || x + patch->width > vid.width || y < 0 || y + patch->height > vid.height || (unsigned) scrn > 4)
    {
        GenPrintf(EMSG_warn, "Patch at %d,%d exceeds LFB\n", x, y);
        // No I_Error abort - what is up with TNT.WAD?
        GenPrintf(EMSG_warn, "V_DrawPatch: bad patch (ignored)\n");
        return;
    }
#endif

#ifdef DIRTY_RECT
    if (!scrn)
        V_MarkRect(x, y, patch->width, patch->height);
#endif

    desttop = screens[scrn] + (y * vid.ybytes) + (x * vid.bytepp);

    wi = patch->width;

    for ( col=0; col < wi; col++)
    {
        column = (column_t *) ((byte *) patch + patch->columnofs[col]);

#ifdef DEEPSEA_TALL_PATCH
        cur_topdelta = -1;
#endif

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *) column + 3;
#ifdef DEEPSEA_TALL_PATCH
            // [MB] [WDJ] DeePsea tall patch.
            // DeepSea allows the patch to exceed 254 height.
            // A Doom patch has monotonic ascending topdelta values, 0..254.
            // DeePsea tall patches have an optional relative topdelta.	
            // When the column topdelta is <= the current topdelta,
            // it is a DeePsea tall patch relative topdelta.
            if( (int)column->topdelta <= cur_topdelta )
            {
                cur_topdelta += column->topdelta;  // DeePsea relative topdelta
            }
            else
            {
                cur_topdelta = column->topdelta;  // Normal Doom patch
            }
            dest = desttop + (cur_topdelta * vid.ybytes);
#else
            dest = desttop + (column->topdelta * vid.ybytes);
#endif
            count = column->length;

            while (count--)
            {
                V_DrawPixel(dest, 0, *source++);
                dest += vid.ybytes;
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
        desttop += vid.bytepp;
    }
}


#if 0
// [WDJ] Replaced by VID_BlitLinearScreen and V_CopyRect because
// were being used to copy screens
//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
// src: is not a screen
// dest: scrn is a screen, x,y in pixel coord
void V_DrawBlock(int x, int y, int scrn, int width, int height, byte * src)
{
    // vid : from video setup
    byte *dest;  // within video buffer

#ifdef RANGECHECK
    if (x < 0 || x + width > vid.width || y < 0 || y + height > vid.height || (unsigned) scrn > 4)
    {
        I_Error("Bad V_DrawBlock");
    }
#endif

#ifdef DIRTY_RECT
    //V_MarkRect (x, y, width, height);
#endif

    // [WDJ] Copy screens, by line, padded, 8bpp .. 32bpp
    width *= vid.bytepp;
    dest = screens[scrn] + (y * vid.ybytes) + (x * vid.bytepp);

    while (height--)
    {
        memcpy(dest, src, width);

        src += width;
        dest += vid.ybytes;
    }
}

//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
// src: scrn is a screen, x,y in pixel coord
// dest: is not a screen
void V_GetBlock(int x, int y, int scrn, int width, int height, byte * dest)
{
    // vid : from video setup
    byte *src;  // within video buffer

    if( rendermode != render_soft )
        I_Error("V_GetBlock: called in non-software mode");

#ifdef RANGECHECK
    if (x < 0 || x + width > vid.width || y < 0 || y + height > vid.height || (unsigned) scrn > 4)
    {
        I_Error("Bad V_GetBlock");
    }
#endif

    src = screens[scrn] + (y * vid.ybytes) + (x * vid.bytepp);

    while (height--)
    {
        memcpy(dest, src, width);
        src += vid.ybytes;
        dest += width;
    }
}
#endif



//  per drawinfo, scaled, abs start coord.
// [WDJ] all pic are cached endian fixed 1/5/2010
static void V_BlitScalePic(int x1, int y1, pic_t * pic)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    int dupx, dupy;
    int x, y;
    byte *src, *dest;
    int pic_width = pic->width;
    int pic_height = pic->height;
   
    if (pic->mode != 0)
    {
        CONS_Printf("pic mode %d not supported in Software\n", pic->mode);
        return;
    }

    // [Arcade] Through the drawinfo scale rather than vid.dupx/vid.dupy, so a
    // raw 320x200 screen (the Heretic attract pages) fills the display when
    // V_SCALEEXACT asks it to.  Without the flag the scale is the same whole
    // number vid.dupx/vid.dupy this always used.
    //
    // Rows and columns are emitted from rounded run bounds, the same way the
    // patch drawers do it, so neither leaves a seam at a fractional scale.

    // scaled, with x centering
    dest = drawinfo.drawp + (max(0, y1) * vid.ybytes) + (max(0, x1) * vid.bytepp);
    // y clipping to the screen
    if (y1 + V_scale_y(pic_height) >= vid.height)
    {
        // Largest source height that still fits below y1.
        while( pic_height > 0 && (y1 + V_scale_y(pic_height)) >= vid.height )
            pic_height--;
    }
    // WARNING no x clipping (not needed for the moment)

    for (y = max(0, -y1 / (int)max(1, V_scale_y(1))); y < pic_height; y++)
    {
        int rows = V_scale_y(y + 1) - V_scale_y(y);
        for (dupy = rows; dupy; dupy--)
        {
            int xb = 0;
            src = pic->data + (y * pic_width);
            for (x = 0; x < pic_width; x++)
            {
                int cols = V_scale_x(x + 1) - V_scale_x(x);
                for (dupx = cols; dupx; dupx--)
                    V_DrawPixel(dest, xb++, *src);
                src++;
            }
            dest += vid.ybytes;
        }
    }
}

//  Draw a linear pic, scaled
//  CURRENTLY USED FOR StatusBarOverlay, scale pic but not starting coords
//  per drawinfo, scaled, abs start coord.
//
void V_DrawScalePic_Num(int x1, int y1, lumpnum_t lumpnum)
{
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_DrawPic(x1, y1, lumpnum);
        return;
    }
#endif

    // [WDJ] Get pic and fix endian, then display
    V_BlitScalePic(x1, y1, W_CachePicNum(lumpnum, PU_CACHE));
}

// Heretic raw pic
//  per drawinfo, scaled, abs start coord.
void V_DrawRawScreen_Num(int x1, int y1, lumpnum_t lumpnum, int width, int height)
{
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // save size somewhere and mark lump as a raw pic !
        MipPatch_t *grpatch = &(wadfiles[WADFILENUM(lumpnum)]->hwrcache[LUMPNUM(lumpnum)]);
        grpatch->width = width;
        grpatch->height = height;
        grpatch->mipmap.tfflags |= TF_Her_Raw_Pic;  // Heretic Raw Pic
        HWR_DrawPic(x1, y1, lumpnum);
        return;
    }
#endif

    V_BlitScalePic(x1, y1,
                   W_CacheRawAsPic(lumpnum, width, height, PU_CACHE));
}


//
//  Fills a box of pixels with a single color
//
// Vid range coordinates.
// per drawinfo centering, always screen 0
//  x, y : screen coord. in vid range.
void V_DrawVidFill(int x, int y, int w, int h, byte color)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    byte *dest;  // within screen buffer
    int u, v;

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_DrawVidFill(x, y, w, h, color);
        return;
    }
#endif

#if 0
    dest = screens[0] + (y * vid.ybytes) + (x * vid.bytepp);
    dest += drawinfo.start_offset;
#else
    dest = drawinfo.drawp + (y * vid.ybytes) + (x * vid.bytepp);
#endif

    for (v = 0; v < h; v++, dest += vid.ybytes)
    {
        for (u = 0; u < w; u++)
            V_DrawPixel(dest, u, color);
    }
}

// Scaled to (320,200)
//   x, y, w, h : (320,200)
void V_DrawFill(int x, int y, int w, int h, byte color)
{
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_DrawVidFill(x * vid.fdupx, y * vid.fdupy, w * vid.fdupx, h * vid.fdupy, color);
        return;
    }
#endif
    // vid : from video setup
    V_DrawVidFill( x * vid.dupx, y * vid.dupy, w * vid.dupx, h * vid.dupy, color);
}

//  per drawinfo, scaled start and size
//  x, y : screen coord.
void V_DrawScaledFill(int x, int y, int w, int h, byte color)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    byte *dest;  // within screen buffer
    int u, v;

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_DrawVidFill( x * drawinfo.fdupx0, y * drawinfo.fdupy0,
                         w * drawinfo.fdupx, h * drawinfo.fdupy, color );
        return;
    }
#endif

    // [WDJ] Draw to screens, by line, padded, 8bpp .. 32bpp
    dest = drawinfo.drawp + (V_start_y(y) * vid.ybytes) + (V_start_x(x) * vid.bytepp);
    w = V_scale_x(x + w) - V_scale_x(x);   // [Arcade] rounded, meets the next fill
    h = V_scale_y(y + h) - V_scale_y(y);

    for (v = 0; v < h; v++, dest += vid.ybytes)
    {
        for (u = 0; u < w; u++)
            V_DrawPixel(dest, u, color);
    }
}



// Indexed by flat size_index.
static byte  fill_sizeshift_tab[ 8 ] =
{
    0,  // 0
    5,  // 32x32 flat
    6,  // 64x64 flat
    7,  // 128x128 flat
    8,  // 256x256 flat
    9,  // 512x512 flat
    10,  // 1024x1024 flat
    11,  // 2048x2048 flat
};

// Fill Flat Index mask
// Indexed by flat size_index.
static uint16_t  fill_mask_tab[ 8 ] =
{
    0, // 0
    32 - 1, // 32x32 flat
    64 - 1, // 64x64 flat
    128 - 1, // 128x128 flat
    256 - 1, // 256x256 flat
    512 - 1, // 512x512 flat
    1024 - 1, // 1024x1024 flat
    2048 - 1, // 2048x2048 flat
};

//  Fills a box of pixels using a flat texture as a pattern.
//  Per drawinfo, scaled, centering.
//  For fullscreen, set w=vid.width.
//
//   x, y, w, h : drawinfo coordinates (if w=vid.width then vid coordinates)
//   scale : 0..15, where 0=unscaled, 15=full scaled
// Called by M_DrawTextBox
void V_DrawFlatFill(int x, int y, int w, int h, int scale, lumpnum_t flatnum)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    byte *dest;  // within screen buffer
    int u, v;
    fixed_t dx, dy, xfrac, yfrac;
    byte *src;
    byte *flat;
    int imask, sizeshift;

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        if( w == vid.width )
            HWR_DrawVidFlatFill(x, y, w, h, scale, flatnum);
        else
            HWR_DrawVidFlatFill((x * drawinfo.fdupx0), (y * drawinfo.fdupy0),
                (w * drawinfo.fdupx), (h * drawinfo.fdupy), scale, flatnum);
        return;
    }
#endif

#if 0   
    int size = W_LumpLength(flatnum);

    switch (size)
    {
        case 4194304:  // 2048x2048 lump
            flatsize = 2048;
            flatshift = 10;
            break;
        case 1048576:  // 1024x1024 lump
            flatsize = 1024;
            flatshift = 9;
            break;
        case 262144:   // 512x512 lump
            flatsize = 512;
            flatshift = 8;
            break;
        case 65536:    // 256x256 lump
            flatsize = 256;
            flatshift = 7;
            break;
        case 16384:    // 128x128 lump
            flatsize = 128;
            flatshift = 7;
            break;
        case 1024:     // 32x32 lump
            flatsize = 32;
            flatshift = 5;
            break;
        default:       // 64x64 lump
            flatsize = 64;
            flatshift = 6;
            break;
    }
#endif   

//    int sizeindex = levelflats[picnum].size_index;
    int sizeindex = P_flatsize_to_index( W_LumpLength(flatnum), NULL );
    sizeshift = fill_sizeshift_tab[sizeindex];
    imask = fill_mask_tab[sizeindex];
    flat = W_CacheLumpNum(flatnum, PU_CACHE);

    if( w == vid.width )
    {
        // fullscreen, assume that also x=0, y=0
        dest = screens[0];
//        dest = screens[0] + (y * vid.ybytes) + (x * vid.bytepp);
    }
    else
    {
        // Draw per drawinfo
        dest = drawinfo.drawp + (V_start_y(y) * vid.ybytes) + (V_start_x(x) * vid.bytepp);
        w = V_scale_x(x + w) - V_scale_x(x);   // [Arcade] rounded
        h = V_scale_y(y + h) - V_scale_y(y);
    }

    // Scale flat proportional, 0..15 => 1..vid.dup
    dx = FixedDiv(FRACUNIT,
         ((((vid.dupx-1) << (FRACBITS-4)) * scale) + (1<<FRACBITS)) );
    dy = FixedDiv(FRACUNIT,
         ((((vid.dupy-1) << (FRACBITS-4)) * scale) + (1<<FRACBITS)) );

    yfrac = 0;
    for (v = 0; v < h; v++)
    {
        xfrac = 0;
        src = & flat[((yfrac >> (FRACBITS - 1)) & imask) << sizeshift];
        for (u = 0; u < w; u++)
        {
            V_DrawPixel(dest, u, src[(xfrac >> FRACBITS) & imask]);
            xfrac += dx;
        }
        yfrac += dy;
        dest += vid.ybytes;
    }
}


// Fill entire screen with flat.
// Called by WI_slamBackground, F_TextWrite (entire screen), M_DrawTextBox
void V_ScreenFlatFill( lumpnum_t flatnum )
{
#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        HWR_DrawVidFlatFill(0, 0, vid.width, vid.height, 15, flatnum);
        return;
    }
#endif
    V_DrawFlatFill( 0, 0, vid.width, vid.height, 15, flatnum);
}



//  General Fade Rectangle.
//   x1, x2, y2 : affected ranges in pixels,  (always y1 = 0)
//   fade_alpha : 1 (no fade) .. 255 (faded)
//   fade_index : from fadescreen_draw8, or fadecons_draw8 table
//   tint_rgba : added color tint, small color values only
void V_FadeRect(int x1, int x2, int y2,
                uint32_t fade_alpha, unsigned int fade_index,
                uint32_t tint_rgba )
{
    // vid : from video setup
    RGBA_t tint;
    byte * fadetab, * greentab;
#ifdef ENABLE_DRAWEXT
    int w4 = x2 - x1;
#endif
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
    uint32_t mask_g2, mask_r2, mask_b2;
    uint32_t tint_g, tint_r, tint_b;
#endif
    int x, y;
    uint32_t *buf;

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Note: y1 is always 0.
        // OpenGL requires stronger color tint.
        HWR_FadeScreenMenuBack( tint_rgba, 0xFF - fade_alpha, y2 );
        return;
    }
#endif

    tint.rgba = tint_rgba;
    switch(vid.drawmode)
    {
     case DRAW8PAL:
        // 8bpp palette, accessed 4 bytes at a time
        fadetab = (byte*) ( reg_colormaps )?
             & reg_colormaps[ LIGHTTABLE(fade_index) ]
           : graymap;  // at startup, before reg_colormaps loaded
        // Palette draw only has facility for console green tint.
        greentab = ( tint.s.green )?  greenmap : & reg_colormaps[ 0 ];
        x1 >>= 2;
        x2 >>= 2;
        for (y = 0; y < y2; y++)
        {
            buf = (uint32_t *) V_GetDrawAddr( 0, y );
            for (x = x1; x < x2; x++)
            {
                register uint32_t quad = buf[x];
                register uint32_t q2 = greentab[fadetab[quad & 0xFF]];
                q2 |= ((uint32_t) greentab[fadetab[(quad >> 8) & 0xFF]]) << 8;
                q2 |= ((uint32_t) greentab[fadetab[(quad >> 16) & 0xFF]]) << 16;
                q2 |= ((uint32_t) greentab[fadetab[quad >> 24]]) << 24;
                buf[x] = q2;
            }
        }
        break;
#ifdef ENABLE_DRAW15
     case DRAW15:
        // 2 pixels at a time  (5,5,5)
        mask_r2 = 0x04000400L;  // 0 00001 00000 00000
        mask_g2 = 0x00200020L;  // 0 00000 00001 00000
        goto fade15_16;
#endif
#ifdef ENABLE_DRAW16
     case DRAW16:
        // 2 pixels at a time  (5,6,5)
        mask_r2 = 0x08000800L;  // 00001 000000 00000
        mask_g2 = 0x00400040L;  // 00000 000010 00000
        goto fade15_16;
#endif
#if defined( ENABLE_DRAW15 ) || defined( ENABLE_DRAW16 )
     fade15_16:
        // 2 pixels at a time
        tint_r = (tint.s.red >> 3) * mask_r2;
        tint_g = (tint.s.green >> 3) * mask_g2;
        tint_b = (uint32_t)(tint.s.blue >> 3) * 0x00010001L;  // 0 00000 00000 00001
        // Must do components separately because of carry from one pixel to next.
        mask_r2 = ((uint32_t) mask_r << 16 ) | mask_r;
        mask_g2 = ((uint32_t) mask_g << 16 ) | mask_g;
        mask_b2 = ((uint32_t) mask_b << 16 ) | mask_b;
        w4 >>= 1;  // 2 bytes at a time
        for (y = 0; y < y2; y++)
        {
            buf = (uint32_t *) V_GetDrawAddr( x1, y );
            for (x = w4; x > 0; x--)
            {
                *buf = (((((*buf & mask_g2) >> 8) * fade_alpha) + tint_g) & mask_g2)
                     | (((((*buf & mask_r2) >> 8) * fade_alpha) + tint_r) & mask_r2)
                     | (((((*buf & mask_b2) * fade_alpha) >> 8) + tint_b) & mask_b2);
                buf++;  // compiler complains when combined above
            }
        }
        break;
#endif       
#ifdef ENABLE_DRAW32
     case DRAW32:
        // RGB
        for (y = 0; y < y2; y++)
        {
            pixel32_t * p32 = (pixel32_t*) V_GetDrawAddr( x1, y );
            for (x = w4; x > 0; x--)
            { 
                p32->b = ((p32->b * fade_alpha) >> 8) + tint.s.blue; // blue
                p32->g = ((p32->g * fade_alpha) >> 8) + tint.s.green; // green
                p32->r = ((p32->r * fade_alpha) >> 8) + tint.s.red; // red
                p32 ++;
            }
        }
        break;
#endif       
#ifdef ENABLE_DRAW24
     case DRAW24:
        // RGB
        for (y = 0; y < y2; y++)
        {
            pixel24_t * p24 = (pixel24_t*) V_GetDrawAddr( x1, y );
            for (x = w4; x > 0; x--)
            { 
                p24->b = ((p24->b * fade_alpha) >> 8) + tint.s.blue; // blue
                p24->g = ((p24->g * fade_alpha) >> 8) + tint.s.green; // green
                p24->r = ((p24->r * fade_alpha) >> 8) + tint.s.red; // red
                p24 ++;
            }
        }
        break;
#endif
     default:
        break;
    }
}


// [WDJ] Tables for darkback
// index by cv_darkback
// LIGHTTABLE[ 0 .. 31 ]
byte fadescreen_draw8[3] =
{
  16,  // half
  20,  // med
  24   // dark
};

// Dark enough that menu is readable.
byte fadescreen_alpha[3] =
{
  0x70,  // half
  0x44,  // med
  0x2C   // dark
};


//
//  Fade all the screen buffer, so that the menu is more readable,
//  especially now that we use the small hufont in the menus...
//
void V_FadeScreen(void)
{
    // vid : from video setup
    V_FadeRect( 0, vid.width, vid.height,
                fadescreen_alpha[ cv_darkback.EV ],
                fadescreen_draw8[ cv_darkback.EV ],
                0 );
}


//  [WDJ] Tables for darkback
// index by cv_darkback
byte fadecons_draw8[3] =
{
   3,  // half
  15,  // med
  23   // dark
};

// Dark enough that red text is lighter and easily readable.
byte fadecons_alpha[3] =
{
  0x80,  // half
  0x40,  // med
  0x20   // dark
};

byte fadecons_green[3] =
{
  0x16,  // half
  0x12,  // med
  0x08   // dark
};


// Fade the console background with fade alpha and green tint per cv_darkback.
//
//added:20-03-98: console test
//   x1, x2, y2 : affected ranges in pixels,  (always y1 = 0)
void V_FadeConsBack(int x1, int x2, int y2)
{
    uint32_t tint = RGBA(0, fadecons_green[ cv_darkback.EV ], 0, 0);

#ifdef HWRENDER
    // The green tint is weak for OpenGL.
    if( rendermode != render_soft )
       tint = tint*2;  // works for small values of green < 127
#endif
    V_FadeRect( x1, x2, y2,
                fadecons_alpha[ cv_darkback.EV ],
                fadecons_draw8[ cv_darkback.EV ],
                tint );
}


// [WDJ]  Default Font
#define FONT1_WIDTH 7
#define FONT1_HEIGHT 9
#define FONT1_START  32
static byte font1_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sp
   0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00, // !
   0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // "
   0x00, 0x14, 0x14, 0x7f, 0x14, 0x7f, 0x14, 0x14, 0x00, // #
   0x1c, 0x2a, 0x0a, 0x0c, 0x18, 0x28, 0x2a, 0x1c, 0x00, // $
   0x42, 0x25, 0x12, 0x08, 0x08, 0x24, 0x52, 0x21, 0x00, // %
   0x08, 0x14, 0x14, 0x08, 0x14, 0x12, 0x22, 0x5c, 0x00, // &
   0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // apostrophe
   0x08, 0x04, 0x04, 0x02, 0x02, 0x04, 0x04, 0x08, 0x00, // (
   0x08, 0x10, 0x10, 0x20, 0x20, 0x10, 0x10, 0x08, 0x00, // )
   0x08, 0x49, 0x2a, 0x1c, 0x1c, 0x2a, 0x49, 0x08, 0x00, // *
   0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x00, 0x00, // +
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x04, // comma
   0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, // -
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, // .
   0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x00, // /

   0x18, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x18, 0x00, // 0
   0x08, 0x0c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00, // 1
   0x1c, 0x22, 0x20, 0x10, 0x08, 0x04, 0x02, 0x3e, 0x00,
   0x1e, 0x20, 0x20, 0x18, 0x20, 0x20, 0x20, 0x1e, 0x00,
   0x10, 0x18, 0x14, 0x12, 0x3e, 0x10, 0x10, 0x10, 0x00,
   0x3e, 0x02, 0x02, 0x1e, 0x20, 0x20, 0x20, 0x1e, 0x00,
   0x1c, 0x02, 0x02, 0x1e, 0x22, 0x22, 0x22, 0x1c, 0x00,
   0x3e, 0x20, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x00,
   0x1c, 0x22, 0x22, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00,
   0x1c, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x22, 0x1c, 0x00, // 9

   0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, // :
   0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x04, // ;
   0x00, 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00, // <
   0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00, 0x00, // =
   0x00, 0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x00, // >
   0x1c, 0x22, 0x20, 0x10, 0x08, 0x08, 0x00, 0x08, 0x00, // ?
   0x3e, 0x41, 0x41, 0x5d, 0x55, 0x45, 0x45, 0x39, 0x00, // @

   0x1c, 0x22, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x22, 0x00, // A
   0x1e, 0x22, 0x22, 0x1e, 0x22, 0x22, 0x22, 0x1e, 0x00,
   0x38, 0x04, 0x02, 0x02, 0x02, 0x02, 0x04, 0x38, 0x00,
   0x0e, 0x12, 0x22, 0x22, 0x22, 0x22, 0x12, 0x0e, 0x00,
   0x3e, 0x02, 0x02, 0x02, 0x1e, 0x02, 0x02, 0x3e, 0x00,
   0x3e, 0x02, 0x02, 0x1e, 0x02, 0x02, 0x02, 0x02, 0x00,
   0x1c, 0x22, 0x02, 0x02, 0x32, 0x22, 0x22, 0x1c, 0x00,
   0x22, 0x22, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x22, 0x00,
   0x1c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00,
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x1c, 0x00,
   0x22, 0x12, 0x0a, 0x06, 0x06, 0x0a, 0x12, 0x22, 0x00,
   0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x3e, 0x00,
   0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x00,
   0x22, 0x26, 0x26, 0x2a, 0x2a, 0x32, 0x32, 0x22, 0x00,
   0x0c, 0x12, 0x21, 0x21, 0x21, 0x21, 0x12, 0x0c, 0x00,
   0x1e, 0x22, 0x22, 0x22, 0x1e, 0x02, 0x02, 0x02, 0x00,
   0x0c, 0x12, 0x21, 0x21, 0x21, 0x29, 0x12, 0x2c, 0x00,
   0x1e, 0x22, 0x22, 0x22, 0x1e, 0x0a, 0x12, 0x22, 0x00,
   0x1c, 0x22, 0x02, 0x0e, 0x38, 0x20, 0x22, 0x1c, 0x00,
   0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00,
   0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x00,
   0x22, 0x22, 0x22, 0x22, 0x14, 0x14, 0x14, 0x08, 0x00,
   0x41, 0x41, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00,
   0x41, 0x22, 0x14, 0x08, 0x08, 0x14, 0x22, 0x41, 0x00,
   0x22, 0x22, 0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x00,
   0x3e, 0x20, 0x10, 0x08, 0x08, 0x04, 0x02, 0x3e, 0x00, // Z

   0x1c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1c, 0x00, // [
   0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x00, // backslash
   0x1c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1c, 0x00, // ]
   0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ^
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, // _
   0x04, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // accent
   
   0x00, 0x00, 0x0c, 0x10, 0x1c, 0x12, 0x12, 0x1c, 0x00, // a
   0x02, 0x02, 0x02, 0x0e, 0x12, 0x12, 0x12, 0x0e, 0x00,
   0x00, 0x00, 0x00, 0x1c, 0x02, 0x02, 0x02, 0x1c, 0x00,
   0x10, 0x10, 0x10, 0x1c, 0x12, 0x12, 0x12, 0x1c, 0x00,
   0x00, 0x00, 0x00, 0x0c, 0x12, 0x1e, 0x02, 0x1c, 0x00,
   0x08, 0x14, 0x04, 0x04, 0x0e, 0x04, 0x04, 0x04, 0x00,
   0x00, 0x00, 0x00, 0x0c, 0x12, 0x12, 0x1c, 0x10, 0x0e,
   0x02, 0x02, 0x02, 0x0e, 0x12, 0x12, 0x12, 0x12, 0x00,
   0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00,
   0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x08, 0x08, 0x0c,
   0x02, 0x02, 0x02, 0x12, 0x0a, 0x06, 0x0a, 0x12, 0x00,
   0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x18, 0x00,
   0x00, 0x00, 0x00, 0x36, 0x49, 0x49, 0x49, 0x49, 0x00,
   0x00, 0x00, 0x00, 0x1a, 0x26, 0x22, 0x22, 0x22, 0x00,
   0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00,
   0x00, 0x00, 0x00, 0x0e, 0x12, 0x12, 0x0e, 0x02, 0x02,
   0x00, 0x00, 0x00, 0x1c, 0x12, 0x12, 0x1c, 0x10, 0x30,
   0x00, 0x00, 0x00, 0x1a, 0x26, 0x02, 0x02, 0x02, 0x00,
   0x00, 0x00, 0x00, 0x3c, 0x02, 0x1c, 0x20, 0x1e, 0x00,
   0x04, 0x04, 0x04, 0x0e, 0x04, 0x04, 0x24, 0x18, 0x00,
   0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x00,
   0x00, 0x00, 0x00, 0x22, 0x22, 0x14, 0x14, 0x08, 0x00,
   0x00, 0x00, 0x00, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00,
   0x00, 0x00, 0x00, 0x22, 0x14, 0x08, 0x14, 0x22, 0x00,
   0x00, 0x00, 0x00, 0x42, 0x24, 0x18, 0x08, 0x04, 0x02,
   0x00, 0x00, 0x00, 0x3e, 0x10, 0x08, 0x04, 0x3e, 0x00, // z
     
   0x30, 0x08, 0x08, 0x0c, 0x0c, 0x08, 0x08, 0x30, 0x00, // {
   0x0c, 0x10, 0x10, 0x30, 0x30, 0x10, 0x10, 0x0c, 0x00, // |
   0x08, 0x08, 0x08, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, // }
   0x00, 0x06, 0x49, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, // ~
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // DEL
};

byte use_font1 = 1;
// default console colors
// Will be updated to nearest color at palette load
byte ci_black = 0;
byte ci_white = 4;
byte ci_green = 123;
byte ci_grey = 2;

fontinfo_t font1_info =
  { FONT1_WIDTH, FONT1_HEIGHT, FONT1_WIDTH+1, FONT1_HEIGHT+1 };

// from console.c
fontinfo_t doomfont_info =
  { 7, 7, 8, 8 };

fontinfo_t * V_FontInfo( void )
{
    return (use_font1)? &font1_info : &doomfont_info;
}

//  Draw font1 fixed width character
//  NOT FOR OPENGL ***
//  Scaled or Unscaled x, y.
static
int  V_Drawfont1_char(int x, int y, byte c)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    int  fb, fy, i, d, chwidth;
    byte fbit;
    byte fcolor = (c & 0x80)? ci_white : ci_green;  // white : green
    byte * dp;
    byte * lbp;
    byte lb[FONT1_WIDTH*8];

    chwidth = FONT1_WIDTH * drawinfo.dupx;
    c &= 0x7f;
    if( c < 33 )  return chwidth;  // space and non-printing

    fb = (c - FONT1_START) * FONT1_HEIGHT;  // in font addressing

    dp = drawinfo.drawp + (y * drawinfo.y0bytes) + (x * drawinfo.x0bytes);

    if(((x * drawinfo.dupx0) + chwidth) > vid.width)
        return 0;
   
#if 0
    // check for overdraw of underline
    fy = 0;
    if( c == '_' )
    {
        fy = 6;
        dp += fy * vid.ybytes;
    }
    for( ; fy<FONT1_HEIGHT; fy++ )
#else   
    for( fy=0; fy<FONT1_HEIGHT; fy++ )
#endif
    {
        fbit = font1_bits[ fb + fy ];
        lbp = &lb[0];
        for( i=0; i<FONT1_WIDTH; i++ )  // convert bit map to color bytes
        {
           for( d=drawinfo.dupx; d>0; d-- )
               *(lbp++) = ( fbit & 0x01 )? fcolor : ci_black;
           fbit >>= 1;
        }
        for( d=drawinfo.dupy; d>0; d-- )
        {
            V_DrawPixels(dp, 0, chwidth, lb);
            dp += vid.ybytes;
        }
    }
    return chwidth;
}

//  Draw font1 fixed width string
//  NOT FOR OPENGL ***
//  Scaled or Unscaled x, y.
//  Called by V_DrawString.
void V_Drawfont1_string(int x, int y, int option, const char *string)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    int  fb, fy, i, d, chwidth, chwidth_bytes, chspace_bytes, x0, cx;
    byte *dp0, *dp;
    const char * ch = string;
    byte fbit, c;
    byte fcolor = (option & V_WHITEMAP)? ci_white : ci_green;  // white : green
    byte * lbp;
    byte lb[FONT1_WIDTH*MAXVIDWIDTH/BASEVIDWIDTH];  // * max dupx

    if( !string )  return;

    chwidth = FONT1_WIDTH * drawinfo.dupx;
    chwidth_bytes = chwidth * vid.bytepp;
    chspace_bytes = (FONT1_WIDTH+1) * drawinfo.xbytes;
    dp0 = drawinfo.drawp + (y * drawinfo.y0bytes);
    x0 = (x * drawinfo.x0bytes);  // scaled x
    cx = x0;

    for(;;)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x0;
            dp0 += (FONT1_HEIGHT+3) * drawinfo.dupy;
            continue;
        }

        if((cx + chwidth_bytes) > vid.widthbytes)
            break;

        if (c >= 33)
        {
            dp = dp0 + cx;  // dest of char
            fb = ((c & 0x7f) - FONT1_START) * FONT1_HEIGHT;  // in font addressing
            for( fy=0; fy<FONT1_HEIGHT; fy++ )  // font lines
            {
                fbit = font1_bits[ fb + fy ];
                lbp = &lb[0];
                for( i=0; i<FONT1_WIDTH; i++ )  // convert bit map to color bytes
                {
                    for( d=drawinfo.dupx; d>0; d-- )  // dup width
                      *(lbp++) = ( fbit & 0x01 )? fcolor : ci_black;
                    fbit >>= 1;
                }
                for( d=drawinfo.dupy; d>0; d-- )  // dup height
                {
                    V_DrawPixels(dp, 0, chwidth, lb);
                    dp += vid.ybytes;
                }
            }
        }
        cx += chspace_bytes;
    }
}

// Current draw font info.
drawfont_t  drawfont;

// Setup drawfont for DrawCharacter and DrawString.
// Uses V_SetupDraw.
//  option : V_SCALESTART
void  V_SetupFont( int font_size, fontinfo_t * fip, uint32_t option )
{
    // V_SetupDraw calls here with font_size = 0, do not call back.
    if( font_size > 0 )
    {
        // Scale, Large = 5.0, Small = 1.0
        drawfont.scale = (float)font_size + 0.000001f;
        // Small is not quite at 1.0, let it round down only when it has to.
        drawfont.ratio = (drawfont.scale - 0.90f) / (5.0f - 0.90f);  // 0.0 .. 1.0
        // V_SetupDraw( V_FINESCALEPATCH ) scales by drawfont.scale.
        // Do not pass option V_SCALESTART to draw.  Handled by DrawString.
        V_SetupDraw( V_FINESCALEPATCH | option );
    }
    else
    {
        drawfont.scale = 5.0f;
        drawfont.ratio = 1.0f;
    }
   
    if( fip == NULL )
       fip = V_FontInfo();  // default

    drawfont.font_height = fip->height;

#ifdef HWRENDER
    if( rendermode == render_soft )
    {
        drawfont.xinc = V_scale_x( fip->xinc );  // [Arcade] exact when asked
        drawfont.yinc = V_scale_y( fip->yinc );
    }
    else
    {
        drawfont.xinc = (int) fip->xinc * drawinfo.fdupx;
        drawfont.yinc = (int) fip->yinc * drawinfo.fdupy;
    }
#else
    drawfont.xinc = V_scale_x( fip->xinc );  // [Arcade] exact when asked
    drawfont.yinc = V_scale_y( fip->yinc );
#endif

    // SCALESTART for font drawing, separate from drawinfo.
    if( option & V_SCALESTART )
    {
        drawfont.dupx0 = vid.dupx;
        drawfont.dupy0 = vid.dupy;
        drawfont.fdupx0 = vid.fdupx;
        drawfont.fdupy0 = vid.fdupy;
    }
    else
    {
        drawfont.dupx0 = 1;
        drawfont.dupy0 = 1;
        drawfont.fdupx0 = 1.0f;
        drawfont.fdupy0 = 1.0f;
    }
}


// Writes a single character (draw WHITE if bit 7 set)
//
//added:20-03-98:
// Return pixel width.
int V_DrawCharacter(int x, int y, byte c)
{
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    int w;
    boolean white = c & 0x80;
    c &= 0x7f;

    if( use_font1 && (rendermode == render_soft))
    {
         return  V_Drawfont1_char( x, y, c);;
    }

    // hufont only has uppercase
    c = toupper(c) - HU_FONTSTART;
    if (c >= HU_FONTSIZE)
        return  V_scale_x(4);  // space and non-printing chars

    // Hardware or software render, access patch fields.
    w = V_scale_x( V_patch( hu_font[c] )->width );  // proportional width
    // [Arcade] Through the drawinfo start scale, so the test matches where
    // the glyph is actually going to land.
    if ((V_start_x(x) + w) > vid.width)
        return 0;

    if (white)
        V_DrawMappedPatch(x, y, hu_font[c], whitemap);
    else
        V_DrawScaledPatch(x, y, hu_font[c]);

    return w + 1;
}

//
//  Write a string using the hu_font
//  NOTE: the text is centered for screens larger than the base width
//
//added:05-02-98:
// Default is V_SCALESTART and V_SCALEPATCH
// option V_SCALESTART controls spacing
// Can use DrawInfo which is also set to SCALESTART and SCALEPATCH
// Called am_map: option=0
// Called d_main netstats: option = V_WHITEMAP, BASEWIDTH relative
// Called f_finale cast member names: option=0
// Called hu_stuff tips: SetupDraw(SCALESTART, SCALEPATCH)
// Called menu: option=0 or V_WHITEMAP, within SetupDraw(SCALESTART, SCALEPATCH)
// Called st_stuff status overlay: (does its own scaling), option=0
//    within SetupDraw( NOSCALE, SCALEPATCH )
// Called wi_stuff YAH: option 0 or V_WHITEMAP, within SetupDraw(SCALESTART, SCALEPATCH)
void V_DrawString(int x, int y, int option, const char *string)
{
    // Save draw SCALESTART setting, and switch to NO SCALESTART drawing.
    // The combination of SCALESTART to this DrawString, and NO SCALEPATCH
    // drawing, cannot be handled with dup. Must turn off SCALESTART.
    // vid : from video setup
    // drawinfo : from V_SetupDraw
    float w;
    const char *ch;
    int c;
    float cx, cy;  // to support hw_draw
    float dupx, dupy; // to fix spacing scaling

#if 0
    // Moved from V_DrawCenteredString, as an option.
    // unused
    if( option & V_CENTERHORZ )
    {
       x = (vid.width - V_StringWidth(string)) / 2;
       if( x < 0 )   x = 0;
    }
#endif

    if ( use_font1 && (rendermode == render_soft))
    {
         V_Drawfont1_string( x, y, option, string);
         return;
    }

    ch = string;

#ifdef HWRENDER
    if( rendermode != render_soft )
    {
        // Character spacing must be scaled here.
        dupx = drawinfo.fdupx;
        dupy = drawinfo.fdupy;
   
        // V_SCALESTART to DrawString must be handled here.
        cx = x * drawfont.fdupx0;
        cy = y * drawfont.fdupy0;
    }
    else
#endif
    {
        // Character spacing must be scaled here.
        // [Arcade] From the drawinfo scale, which is the whole number dupx/dupy
        // unless V_SCALEEXACT asked for the exact one.  Stepping by the whole
        // number while the glyphs themselves were scaled exactly would drift a
        // pixel per character and pull a line apart across a wide screen.
        dupx = FIXED_TO_FLOAT( drawinfo.x_scale );
        dupy = FIXED_TO_FLOAT( drawinfo.y_scale );

        // V_SCALESTART to DrawString must be handled here.
        cx = x * FIXED_TO_FLOAT( drawinfo.x0_scale );
        cy = y * FIXED_TO_FLOAT( drawinfo.y0_scale );
    }

    // Change draw to NO SCALESTART, for positioning of characters.
    V_SetupDraw_NO_SCALESTART();

    for(;;)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x;
            cy += 12 * dupy;
            continue;
        }

        // hufont only has uppercase
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += 4 * dupx;
            continue;
        }

        //[segabor]
        // hu_font is endian fixed
        // Hardware or software render, access patch fields.
        w = V_patch( hu_font[c] )->width * dupx;  // proportional width
        if (cx + w > vid.width)
            break;
        if (option & V_WHITEMAP)
            V_DrawMappedPatch( (int)cx, (int)cy, hu_font[c], whitemap);
        else
            V_DrawScaledPatch( (int)cx, (int)cy, hu_font[c]);
        cx += w;
    }

    V_SetupDraw_Restore_SCALESTART();  // Restore SetupDraw
}


//
// Find string width from hu_font chars
//
// Used extensively
int V_StringWidth( const char *string)
{
    int i;
    int sw = 0;
    int ln = strlen(string);

    if(use_font1)
    {
        return ln * (FONT1_WIDTH+1);  // fixed width font
    }

    // variable width font, total up chars in string
    for (i = 0; i < ln; i++)
    {
        // hufont only has uppercase
        // Prevent using signed char as index.
        unsigned char uc = string[i];
        int c = toupper(uc) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
            sw += 4;
        else
        {
            //[segabor]
            // hu_font is endian fixed
            // Hardware or software render, access patch fields.
            sw += V_patch( hu_font[c] )->width;
        }
    }

    return sw;
}

#if 0
// Unused, see V_FontInfo, and drawinfo
//
// Find string height from hu_font chars
//
int V_StringHeight( const char *string)
{
    return (hu_font[0]->height);
}
#endif

//---------------------------------------------------------------------------
//
// PROC MN_DrTextB
//
// Draw text using font B.
//
//---------------------------------------------------------------------------
lumpnum_t  FontBBaseLump;

// per drawinfo
void V_DrawTextB(const char *text, int x, int y)
{
    unsigned char c;
    patch_t *p;

    while ((c = *text++) != 0)
    {
        if (c < 33)
        {
            x += 8;
        }
        else
        {
            // FontB only has uppercase
            p = W_CachePatchNum(FontBBaseLump + toupper(c) - 33, PU_CACHE);  // endian fix
            V_DrawScaledPatch(x, y, p);
            // Hardware or software render, access patch fields.
            x += V_patch(p)->width - 1;
        }
    }
}

// per drawinfo
void V_DrawTextBGray(const char *text, int x, int y)
{
    unsigned char c;
    patch_t *p;

    while ((c = *text++) != 0)
    {
        if (c < 33)
        {
            x += 8;
        }
        else
        {
            // FontB only has uppercase
            p = W_CachePatchNum(FontBBaseLump + toupper(c) - 33, PU_CACHE);  // endian fix
            V_DrawMappedPatch(x, y, p, graymap);
            // Hardware or software render, access patch fields.
            x += V_patch(p)->width - 1;
        }
    }
}

//---------------------------------------------------------------------------
//
// FUNC MN_TextBWidth
//
// Returns the pixel width of a string using font B.
//
//---------------------------------------------------------------------------

int V_TextBWidth(const char *text)
{
    unsigned char c;
    int width;
    patch_t *p;

    width = 0;
    while ((c = *text++) != 0)
    {
        if (c < 33)
        {
            width += 5;
        }
        else
        {
            // FontB only has uppercase
            p = W_CachePatchNum(FontBBaseLump + toupper(c) - 33, PU_CACHE);  // endian fix
            // Hardware or software render, access patch fields.
            width += V_patch(p)->width - 1;
        }
    }
    return (width);
}

int V_TextBHeight(const char *text)
{
    return 16;
}

// Setup wad loadable video resources.
// Also called before wad is read, to supply defaults.
void V_Setup_Wad_VideoResc(void)
{
    LoadPalette("PLAYPAL");
    FontBBaseLump = W_CheckNumForName("FONTB_S") + 1;

#ifdef ENABLE_DRAWEXT
    // This is also done, better, by SetPalette
    //fab highcolor
    if (( vid.bytepp > 1 ) && pLocalPalette )  // highcolor, truecolor
    {
        R_Init_color8_translate( pLocalPalette );  // no palette change
    }
#endif
}


// Setup Video and Drawing according to render and vidmode.
// Software stuff, buffers are allocated at video mode setup
// here we set the screens[x] pointers accordingly
// WARNING :
// - called at runtime (don't init cvar here)
// Must be called after every video Init and SetMode.
// Some port video control may call this directly, because of print stmts.
void V_Setup_VideoDraw(void)
{
    // vid : from video setup
    int i;

    // Must init everything needed by DrawPixel, as that gets used for
    // many intro screens, before SCR_SetMode or SCR_Recalc are called.

    // [WDJ] Use vid.draw_ready to indicate that this setup has been done.
    // Attempts to test other vars such as screen or drawmode is complicated
    // by the odd usage of those vars in video functions, or OpenGL mode
    // not using them at all.
    // Using vid.draw_ready is less likely to get damaged later.
   
    // Setup everything needed to draw console, pics, and error messages.

    // scale 1,2,3 times in x and y the patches for the
    // menus and overlays... calculated once and for all
    // used by routines in v_video.c
    // leave it be 1 in hardware accelerated modes
    vid.dupx = vid.width / BASEVIDWIDTH;
    vid.dupy = vid.height / BASEVIDHEIGHT;
    vid.fdupx = (float)vid.width / BASEVIDWIDTH;
    vid.fdupy = (float)vid.height / BASEVIDHEIGHT;
    //vid.baseratio = FixedDiv(vid.height << FRACBITS, BASEVIDHEIGHT << FRACBITS); //Hurdler: not used anymore
    vid.fx_center = (float) vid.width * 0.5f;   
    vid.fx_scale2 = 2.0f / (float)vid.width;
    vid.fy_center = (float) vid.height * 0.5f;   
    vid.fy_scale2 = 2.0f / (float)vid.height;

    //added:18-02-98: calculate centering offset for the scaled menu
    // Adds a left margin and top margin for CENTERMENU
    // Fixed to account for video buffer line padding.
    vid.centerofs = (((vid.height%BASEVIDHEIGHT)/2) * vid.ybytes) +
                    (((vid.width%BASEVIDWIDTH)/2)  * vid.bytepp) ;

//    if( cv_gammafunc.value != cv_gammafunc.EV )
    {
        cv_gammafunc.EV = cv_gammafunc.value;
        CV_gammafunc_OnChange();
    }

#ifdef HWRENDER
    // hardware modes do not use screens[] pointers
    if( rendermode != render_soft )
    {
        // Hardware draw only.
        // be sure to cause a NULL read/write error so we detect it, in case of..
        for (i = 0; i < NUMSCREENS; i++)
            screens[i] = NULL;

        vid.drawmode = DRAWGL;
        if( graphics_state >= VGS_active )
            vid.draw_ready = 1;

        HWR_Startup_Render();  // hardware render init
       
        return;
    }
#endif

    // Software draw only.
    EN_HWR_flashpalette = 0;  // software and default

    if( vid.display == NULL )
    {
        GenPrintf( EMSG_warn, "V_Setup_VideoDraw: No display\n" );
        return;  // allocation failed
    }

    // [WDJ] screens usage
    // [0] = display or direct video
    // [1] = background, status bar
    // [2] = wipe start screen, screenshot, (? Horz. draw)
    // [3] = wipe end screen
    screens[0] = vid.display;  // buffer or direct video
    // buffers allocated by port video control, 0..(NUMSCREENS-1)
    for (i = 1; i < NUMSCREENS; i++)
        screens[i] = vid.screen1 + ((i-1) * vid.screen_size);

    // [WDJ] statusbar buffer was not within driver allocated memory
    // and is not used.
    //added:26-01-98: statusbar buffer
//    screens[4] = base + NUMSCREENS * screensize;
    screens[4] = NULL;

    //!debug
#ifdef DEBUG
    CONS_Printf("V_Setup_VideoDraw:\n");
    for (i = 0; i < NUMSCREENS + 1; i++)
        CONS_Printf(" screens[%d] = %x\n", i, screens[i]);
#endif

    // port video control should check CanDraw before setting vid.bitpp
    switch( vid.bitpp )
    {
     case 8:
        vid.drawmode = DRAW8PAL;
        break;
#ifdef ENABLE_DRAW15
     case 15:
        vid.drawmode = DRAW15;
        break;
#endif
#ifdef ENABLE_DRAW16
     case 16:
        vid.drawmode = DRAW16;
        break;
#endif
#ifdef ENABLE_DRAW24
     case 24:
        vid.drawmode = DRAW24;
        break;
#endif
#ifdef ENABLE_DRAW32
     case 32:
        vid.drawmode = DRAW32;
        break;
#endif
     default:
        I_Error ("V_Setup_VideoDraw invalid bits per pixel: %d\n", vid.bitpp);
    }

    if( screens[0] && (graphics_state >= VGS_active))
        vid.draw_ready = 1;
    return;
}


// ---- Draw Frame-per-second graph

#define FPS_POINTS  35
#define FPS_SCALE    4
#define FPS_MAXTICKS 20
#define FPS_SHIFTTIC 10

static byte fpsgraph[FPS_POINTS];

// [WDJ] Draw ticrate graph at bottom of screen.
// Removed from port drivers so do not have to maintain 4 copies of it.
void V_Draw_ticrate_graph( void )
{
    // vid : from video setup
    static tic_t lasttic;
    static tic_t shifttic = 0;
    tic_t        tics, nt;
    int i;
    int k,j;

    nt = I_GetTime();
    tics = nt - lasttic;
    lasttic = nt;
    if (tics > FPS_MAXTICKS) tics = FPS_MAXTICKS;

    // Display a graph of ticrate.
    if(cv_ticrate.value == 1)
    {
        if( nt - shifttic > FPS_SHIFTTIC )
        {
            // try to get somewhat constant horz. time scale
            shifttic += FPS_SHIFTTIC;
            // shift it left
            memmove( &fpsgraph[0], &fpsgraph[1], (FPS_POINTS-1)*sizeof(fpsgraph[0]));
        }
        fpsgraph[FPS_POINTS-1]= FPS_MAXTICKS - tics;

        if( rendermode == render_soft )
        {
            // draw grid of dots
            for(j=0; j<=FPS_MAXTICKS*FPS_SCALE*vid.dupy; j+=2*FPS_SCALE*vid.dupy)
            {
                byte * dest = V_GetDrawAddr( 0, (vid.height-1-j) );
                for (i=0; i<FPS_POINTS*FPS_SCALE*vid.dupx; i+=2*FPS_SCALE*vid.dupx)
                    V_DrawPixel( dest, i, 0xff );
            }

            // draw the graph
            for (i=0; i<FPS_POINTS; i++)
            {
                byte * dest = V_GetDrawAddr( 0, vid.height-1-(fpsgraph[i]*FPS_SCALE*vid.dupy) );
                // draw line at the graph height
                for(k=0; k<FPS_SCALE*vid.dupx; k++)
                    V_DrawPixel( dest, (i*FPS_SCALE*vid.dupx)+k, 0xff );
            }
        }
#ifdef HWRENDER
        else
        {
            fline_t p;
            for(j=0; j<=20*FPS_SCALE*vid.dupy; j+=2*FPS_SCALE*vid.dupy)
            {
                k=(vid.height-1-j);
                p.a.y = k;
                p.b.y = k;
                for (i=0; i<FPS_POINTS*FPS_SCALE*vid.dupx; i+=2*FPS_SCALE*vid.dupx)
                {
                    p.a.x = i;
                    p.b.x = i+1;
                    HWR_drawAMline(&p, 0xff);
                }
            }

            for (i=1; i<FPS_POINTS; i++)
            {
                p.a.x = FPS_SCALE * vid.dupx * (i-1);
                p.a.y = vid.height-1-fpsgraph[i-1]*FPS_SCALE*vid.dupy;
                p.b.x = FPS_SCALE * vid.dupx * i;
                p.b.y = vid.height-1-fpsgraph[i]*FPS_SCALE*vid.dupy;
                HWR_drawAMline(&p, 0xff);
            }
        }
#endif
    }
    else if(cv_ticrate.value == 2)
    {
        static byte accum_frame = 0;
        int accum_tic = 0;
        int i;
        
        // Sometimes tics = 0, for a frame.
        // Use fpsgraph for smoothing the FPS over several frames.
        memmove( &fpsgraph[0], &fpsgraph[1], (FPS_POINTS-1)*sizeof(fpsgraph[0]));
        fpsgraph[FPS_POINTS-1]= tics;
        if( accum_frame < FPS_POINTS )   accum_frame++;  // startup
        for( i=0; i<accum_frame; i++ )
        {
            accum_tic += fpsgraph[i];  // total tics over the frames
        }
        if( accum_tic > 1 )
        {
            char buff[20];
            sprintf(buff,"FPS: %4i", ((int)accum_frame * TICRATE)/accum_tic );
            V_SetupDraw( V_SCALEPATCH | V_SCALESTART );
            V_DrawString ( 8, 160, V_WHITEMAP, buff);
            V_SetupDraw( drawinfo.prev_screenflags );  // restore
        }
    }
}


//
//
//
typedef struct
{
    int px;
    int py;
} modelvertex_t;

void R_DrawSpanNoWrap(void);    //tmap.S

//
//  Tilts the view like DukeNukem...
//
//added:12-02-98:
#ifdef TILTVIEW
#ifdef HWRENDER
void V_DrawTiltView(byte * viewbuffer)  // don't touch direct video I'll find something..
{
}
#else

static modelvertex_t vertex[4];

// Called instead of I_FinishUpdate
void V_DrawTiltView(byte * viewbuffer)
{
    // vid : from video setup
    fixed_t leftxfrac;
    fixed_t leftyfrac;
    fixed_t xstep;
    fixed_t ystep;

    int y;

    vertex[0].px = 0;   // tl
    vertex[0].py = 53;
    vertex[1].px = 212; // tr
    vertex[1].py = 0;
    vertex[2].px = 264; // br
    vertex[2].py = 144;
    vertex[3].px = 53;  // bl
    vertex[3].py = 199;

    // resize coords to screen
    for (y = 0; y < 4; y++)
    {
        vertex[y].px = (vertex[y].px * vid.width) / BASEVIDWIDTH;
        vertex[y].py = (vertex[y].py * vid.height) / BASEVIDHEIGHT;
    }

    ds_colormap = fixedcolormap;
    ds_source = viewbuffer;

    // starting points top-left and top-right
    leftxfrac = vertex[0].px << FRACBITS;
    leftyfrac = vertex[0].py << FRACBITS;

    // steps
    xstep = ((vertex[3].px - vertex[0].px) << FRACBITS) / vid.height;
    ystep = ((vertex[3].py - vertex[0].py) << FRACBITS) / vid.height;

#if 0
    // [WDJ] WRONG, ds_y is y line index, not a ptr
    // vid.direct not allowed without locking the video buffer
    ds_y = (int) vid.direct;
#else
    ds_y = 0;
#endif
    ds_x1 = 0;
    ds_x2 = vid.width - 1;
    ds_xstep = ((vertex[1].px - vertex[0].px) << FRACBITS) / vid.width;
    ds_ystep = ((vertex[1].py - vertex[0].py) << FRACBITS) / vid.width;

//    I_Error("ds_y %d ds_x1 %d ds_x2 %d ds_xstep %x ds_ystep %x \n"
//            "ds_xfrac %x ds_yfrac %x ds_source %x\n", ds_y,
//                      ds_x1,ds_x2,ds_xstep,ds_ystep,leftxfrac,leftyfrac,
//                      ds_source);

    // render spans
    for (y = 0; y < vid.height; y++)
    {
        // FAST ASM routine!
        ds_xfrac = leftxfrac;
        ds_yfrac = leftyfrac;
        R_DrawSpanNoWrap();
#if 0
        // [WDJ] WRONG, ds_y is y line index, not a ptr
        ds_y += vid.ybytes;
#else
        ds_y++; 
#endif

        // move along the left and right edges of the polygon
        leftxfrac += xstep;
        leftyfrac += ystep;
    }

}
#endif
#endif


#ifdef PERSPCORRECT
//
// Test 'scrunch perspective correction' tm (c) ect.
//
//added:05-04-98:

// Called by D_Display
// - instead of I_Finish update with page flip
void V_DrawPerspView(byte * viewbuffer, int aiming)
{
    // vid : from video setup
    byte *source;
    byte *dest;  // direct screen
    int y;
    int x1, w;
    int offs;

    fixed_t topfrac, bottomfrac, scale, scalestep;
    fixed_t xfrac, xfracstep;

    source = viewbuffer;

    //+16 to -16 fixed
    offs = ((aiming * 20) << 16) / 100;

    topfrac = ((vid.width - 40) << 16) - (offs * 2);
    bottomfrac = ((vid.width - 40) << 16) + (offs * 2);

    scalestep = (bottomfrac - topfrac) / vid.height;
    scale = topfrac;

    for (y = 0; y < vid.height; y++)
    {
        x1 = ((vid.width << 16) - scale) >> 17;
    // vid.direct not allowed without locking the video buffer
        dest = vid.direct + (y * vid.ybytes) + (x1 * vid.bytepp);

        xfrac = (20 << FRACBITS) + ((!x1) & 0xFFFF);
        xfracstep = FixedDiv((vid.width << FRACBITS) - (xfrac << 1), scale);
        w = scale >> 16;
#ifdef ENABLE_DRAWEXT
        if( vid.bytepp > 1 )
        {
          while (w--)
          {
            V_DrawPixel( dest, 0, source[xfrac >> FRACBITS] );
            dest += vid.bytepp;
            xfrac += xfracstep;
          }
        }
        else
#endif
        {
          // 8 bit per pixel
          while (w--)
          {
            *dest++ = source[xfrac >> FRACBITS];
            xfrac += xfracstep;
          }
        }

        scale += scalestep;
        source += vid.ybytes;
    }

}
#endif
