// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: win_vid.c 1590 2021-10-11 02:45:45Z wesleyjohnson $
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
// $Log: win_vid.c,v $
// Revision 1.22  2003/06/05 20:40:17  hurdler
// accept -glide command line switch
//
// Revision 1.21  2002/09/21 11:10:28  hurdler
//
// Revision 1.20  2001/08/06 23:57:11  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.19  2001/03/30 17:12:52  bpereira
// Revision 1.18  2001/02/10 12:27:15  bpereira
//
// Revision 1.17  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.16  2001/01/06 22:21:08  judgecutor
// Added NoDirectInput mouse input
//
// Revision 1.15  2001/01/05 18:19:48  hurdler
// add renderer version checking
//
// Revision 1.14  2000/11/04 16:23:45  bpereira
// Revision 1.13  2000/11/02 19:49:40  bpereira
// Revision 1.12  2000/10/21 08:43:32  bpereira
//
// Revision 1.11  2000/10/04 16:25:57  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.10  2000/10/01 10:18:23  bpereira
// Revision 1.9  2000/09/28 20:57:22  bpereira
//
// Revision 1.8  2000/09/25 19:30:17  hurdler
// Enable Direct3D support as OpenGL
//
// Revision 1.7  2000/09/01 19:34:38  bpereira
// Revision 1.6  2000/08/10 19:58:05  bpereira
//
// Revision 1.5  2000/08/10 17:04:22  hurdler
// add ticrate to hardware mode
//
// Revision 1.4  2000/08/10 14:19:56  hurdler
// add waitvbl
//
// Revision 1.3  2000/02/27 00:42:12  hurdler
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      win32 video driver for Doom Legacy
//
//-----------------------------------------------------------------------------

// Because of WINVER redefine, doomtype.h (via doomincl.h) is before any
// other include that might define WINVER
#include "doomincl.h"

#include <stdlib.h>
#include <stdarg.h>

#include "i_system.h"
#include "m_argv.h"
#include "v_video.h"
#include "st_stuff.h"
#include "i_video.h"
#include "z_zone.h"
#include "fabdxlib.h"       //wow! I can make use of my win32 test stuff!!

#include "win_main.h"
#include "win_vid.h"

#ifdef HWRENDER
#include "win_dll.h"             //loading DLL, such as opengl, and Glide Render
#include "hardware/hw_drv.h"     //calling Driver Init & Shutdown
#include "hardware/hw_main.h"    //calling HWR module Init & Shutdown
#endif


// -------
// Globals
// -------

static  BOOL        DIB_mode;  // means we are using DIB instead of DirectDraw surfaces
static  BITMAPINFO* bmi_main = NULL;
static  HDC         hDC_main = NULL;

static  BOOL  req_win;
static  byte  fdx_created = 0;


// -----------------
// Video modes stuff
// -----------------

#define MAX_EXTRA_MODES         30
#define MAX_LEN_VIDMODENAME     10
static  vmode_t     extra_modes[MAX_EXTRA_MODES] = {{NULL, NULL}};
static  char        names[MAX_EXTRA_MODES][MAX_LEN_VIDMODENAME];

//static  int     totalvidmem;

// All vidmodes are in the same list, all can be window modes.
// The first vidmodes are special modes, for window only.
// The DirectDraw vidmodes can be fullscreen or window.
static int     num_all_vidmodes = 0;   // total number of display modes
static int     num_full_vidmodes = 0;  // total number of DirectDraw display modes
static vmode_t * all_vidmodes = NULL;  // start of videomodes list (window and fullscreen)
static vmode_t * full_vidmodes = NULL; // start of fullscreen (DirectDraw, opengl) vidmodes
static vmode_t * currentmode_p = NULL; // the current active videomode.

static int VID_SetWindowedDisplayMode (viddef_t * lvid, vmode_t * newmode_p);

// this holds description of the startup video mode,
// the resolution is 320x200, windowed on the desktop
#define NUMSPECIALMODES  2
vmode_t specialmodes[NUMSPECIALMODES] = {
        {   // 0 mode, HIDDEN
            & specialmodes[1],
            "Initial",
            INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT,
            INITIAL_WINDOW_WIDTH, 1,     // rowbytes, bytes per pixel
            MODE_window, 2,  // windowed, numpages
            NULL,
            VID_SetWindowedDisplayMode,
            0          // misc
        },
        {   // original startup mode at mode 1
            NULL,
            "320x200",
            320, 200,   //(200.0/320.0)*(320.0/240.0),
            320, 1,     // rowbytes, bytes per pixel
            MODE_window, 2,  // windowed, numpages
            NULL,
            VID_SetWindowedDisplayMode,
            0          // misc
        }
};

// ---------------
// WindowMode_Init
// mode 0 and 1 are used for windowed console startup (works on all computers with no DirectX)
// ---------------
static void WindowMode_Init(void)
{
    // do not include Mode 0 (INITIAL) in count
    all_vidmodes = &specialmodes[1];
    num_all_vidmodes = NUMSPECIALMODES-1;
}

static void append_full_vidmodes( vmode_t * newmodes, int nummodes )
{
    full_vidmodes = newmodes;
    specialmodes[NUMSPECIALMODES-1].next = newmodes;

    num_full_vidmodes += nummodes;
    num_all_vidmodes += nummodes;
}


// ------
// Protos
// ------
static  void VID_Command_NumModes_f (void);
static  void VID_Command_ModeInfo_f (void);
static  void VID_Command_ModeList_f (void);
static  void VID_Command_Mode_f     (void);
static  int VID_SetDirectDrawMode (viddef_t *lvid, vmode_t * newmode);
static  int VID_SetWindowedDisplayMode (viddef_t *lvid, vmode_t * newmode);
static  vmode_t * VID_GetModePtr (modenum_t modenum);
static  void VID_GetExtraModes ( byte xm_req_bitpp );

// judgecutor:
extern void I_RestartSysMouse();




// ------------
// I_StartFrame
// ------------
void I_StartFrame (void)
{
    //faB: no use
}


// --------------
// I_UpdateNoBlit
// --------------
void I_UpdateNoBlit (void)
{
    // what is this?
}


// --------------
// I_FinishUpdate
// --------------
void I_FinishUpdate (void)
{
    //RECT        Rect;

    //
    // If page flip involves changing vid.display, then must change screens[0] too
    if ( DIB_mode )
    {
        // paranoia
        if ( !hDC_main || !bmi_main || !vid.buffer )
            return;
        // main game loop, still in a window (-win parm)
        SetDIBitsToDevice (hDC_main,
                           0, 0, 320, 200,
                           0, 0, 0, 200,
                           vid.display, bmi_main, DIB_RGB_COLORS);
    }
#ifdef HWRENDER
    else
    if (rendermode != render_soft) {
        HWD.pfnFinishUpdate ( cv_vidwait.value );
    }
#endif
    else if( fdx_created )
    {
        // DIRECT DRAW
        // copy virtual screen to real screen
        // 26-12-99 BP: can fail when not active (alt-tab)
        if(FDX_LockScreen())
        {
            //faB: TODO: use directX blit here!!? a blit might use hardware with access
            //     to main memory on recent hardware, and software blit of directX may be
            //  optimized for p2 or mmx??
            VID_BlitLinearScreen (vid.display, ScreenPtr,
                                  vid.widthbytes, vid.height, // copy area
                                  vid.ybytes, ScreenPitch ); // scanline inc

            FDX_UnlockScreen();

            // swap screens
            FDX_ScreenFlip(cv_vidwait.value);
        }
    }
}



// for Win32 version
static byte  WndNumpages;


//
// This is meant to be called only by CONS_Printf() while game startup
//
// printf to loading screen
void I_LoadingScreen ( const char * msg )
{
    //PAINTSTRUCT ps;
    RECT        rect;
    //HDC         hdc;

    // paranoia
    if ( !hDC_main || !bmi_main || !vid.buffer || !hWnd_main )
        return;

    //hdc = BeginPaint (hWnd_main, &ps);
    GetClientRect (hWnd_main, &rect);

    SetDIBitsToDevice (hDC_main,
                       0, 0, 320, 200,
                       0, 0, 0, 200,
                       vid.display, bmi_main, DIB_RGB_COLORS);

    if ( msg )
    {
        if ( rect.bottom - rect.top > 32 )
            rect.top = rect.bottom - 32;        // put msg on bottom of window
        SetBkMode ( hDC_main, TRANSPARENT );
        SetTextColor ( hDC_main, RGB(0xff,0xff,0xff) );
        DrawText (hDC_main, msg, -1, &rect,
                  DT_WORDBREAK | DT_CENTER ); //| DT_SINGLELINE | DT_VCENTER);
    }
    //EndPaint (hWnd_main, &ps);
}


// ------------
// I_ReadScreen
// ------------
void I_ReadScreen (byte* scr)
{
#ifdef HWRENDER
    // DEBUGGING
    if (rendermode != render_soft)
        I_SoftError ("I_ReadScreen: called while in non-software mode");
#endif
    CopyMemory (scr, vid.display, vid.screen_size);
}


// ------------
// I_SetPalette
// ------------
void I_SetPalette (RGBA_t *palette)
{
    int   i;

    if ( DIB_mode )
    {
        // set palette in RGBQUAD format, NOT THE SAME ORDER as PALETTEENTRY, grmpf!
        RGBQUAD*    pColors;
        pColors = (RGBQUAD*) ((char*)bmi_main + bmi_main->bmiHeader.biSize);
        ZeroMemory (pColors, sizeof(RGBQUAD)*256);
        for (i=0; i<256; i++, pColors++,palette++)
        {
            pColors->rgbRed = palette->s.red;
            pColors->rgbGreen = palette->s.green;
            pColors->rgbBlue = palette->s.blue;
        }
    }
    else
    if( rendermode == render_soft )
    {
        PALETTEENTRY    mainpal[256];

        // this clears the 'flag' for each color in palette
        ZeroMemory (mainpal, sizeof(mainpal));

        // set palette in PALETTEENTRY format
        for (i=0; i<256; i++,palette++)
        {
            mainpal[i].peRed = palette->s.red;
            mainpal[i].peGreen = palette->s.green;
            mainpal[i].peBlue = palette->s.blue;
        }
#if ( defined(DEBUG_WINDOWED) && defined(WIN32) )
        // Palette fix during debug, otherwise black text on black background
        if( palette[6].s.red < 96 )
            mainpal[6].peRed = 96;  // at least get red text on black
        if( palette[7].s.green < 96 )
            mainpal[i].peGreen = 96;  // at least get green text on black
#endif
        if( graphics_state >= VGS_active && fdx_created )
            FDX_SetDDPalette (mainpal);         // set DirectDraw palette
    }
}


// for debuging
void IO_Color( byte color, byte r, byte g, byte b )
{
/*
outportb( 0x03c8 , color );                // registre couleur
outportb( 0x03c9 , (r>>2) & 0x3f );       // R
outportb( 0x03c9 , (g>>2) & 0x3f );       // G
outportb( 0x03c9 , (b>>2) & 0x3f );       // B
    */
}



// return number of video modes in vidmodes lists
//
// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };
    // INITIAL_WINDOW mode 0 is not included
    if(modetype == MODE_fullscreen)
    {   // fullscreen  2..
        mrange.first = NUMSPECIALMODES;
        mrange.last = mrange.first + num_full_vidmodes - 1;
    }
    else
    {   // window   1..
        // does not include mode 0
        mrange.last = num_all_vidmodes;
    }
    return mrange;
}


// return a video mode number from the dimensions
// returns closest available video mode if the mode was not found
// rmodetype is of modetype_e
// Returns {MODE_NOP, 0} when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype )
{
    modenum_t  modenum = { MODE_NOP, 0 };
    int tdist;
    int bestdist = INT_MAX;
    int mi = 1;  // window modes
    vmode_t * pv = all_vidmodes;  // window modes

    if( rmodetype == MODE_fullscreen )
    {
        if( num_full_vidmodes == 0 )  goto done;
        mi = NUMSPECIALMODES;  // fullscreen modes start after
        pv = full_vidmodes;   // fullscreen modes
    }
    modenum.modetype = rmodetype;
    for ( ; pv!=NULL; pv=pv->next )
    {
        tdist = abs(pv->width - rw) + abs(pv->height - rh);
        // find closest dist
        if( bestdist > tdist )
        {
            bestdist = tdist;
            modenum.index = mi;
            if( tdist == 0 )  break;   // found exact match
        }
        mi++;
    }

done:
    return modenum;
}


//
// Enumerate DirectDraw modes available
//
static  int     nummodes=0;
static  byte    mode_bitpp;  // bitpp of the mode list

// Called from FDX_EnumDisplayModes through DirectX EnumDisplayModes
static BOOL VID_DDModes_callback (int width, int height, int bpp)
{
    vmode_t * nmp;

    if( verbose > 1 )
       GenPrintf( EMSG_ver, "mode %d x %d x %d bpp\n", width, height, bpp);

    // skip all unwanted modes
    if( bpp != mode_bitpp )  goto skip

#if 1
    if( (width > MAXVIDWIDTH) || (height > MAXVIDHEIGHT) )
#else
    if ((bpp > 16) ||
        (width > MAXVIDWIDTH) ||
        (height > MAXVIDHEIGHT))
#endif
    {
        goto skip;
    }

    // check if we have space for this mode
    if (nummodes>=MAX_EXTRA_MODES)
    {
        GenPrintf( EMSG_warn, "mode skipped (too many)\n");
        return FALSE;  // stop
    }

        //DEBUG: test without 320x200 standard mode
        //if (width<640 || height<400)
        //    goto skip;

    // store mode info
    nmp = & extra_modes[nummodes];
    nmp->next = &extra_modes[nummodes+1];
    // [WDJ] Same print for all, easier to read in columns without extra spaces
    snprintf (&names[nummodes][0], MAX_LEN_VIDMODENAME, "%dx%d", width, height);
    names[nummodes][MAX_LEN_VIDMODENAME-1] = 0;
    nmp->name = &names[nummodes][0];
    nmp->width = width;
    nmp->height = height;

    // exactly, the current FinishUpdate() gets the rowbytes itself after locking the video buffer
    // so for now we put anything here
    nmp->rowbytes = width;
    nmp->modetype = MODE_either;
    nmp->misc = 0;         // unused
    nmp->extradata = NULL;
    nmp->setmode_func = VID_SetDirectDrawMode;

    nmp->numpages = 2;     // double-buffer (but this value is not used)

    nmp->bytesperpixel = (bpp+1)>>3;

    nummodes++;
skip:
    return TRUE;  // continue
}


//
// Collect info about DirectDraw display modes we use
//
static
void VID_GetExtraModes ( byte xm_req_bitpp )
{
    mode_bitpp = xm_req_bitpp;
    nummodes = 0;
   
    if( ! fdx_created )  return;

    FDX_EnumDisplayModes (VID_DDModes_callback);   // fabdxlib

    // add the extra modes (non 320x200) at the start of the mode list (if there are any)
    if (nummodes)
    {
        extra_modes[nummodes-1].next = NULL;
        append_full_vidmodes( &extra_modes[0], nummodes );
    }
}



// *************************************************************************************
// VID_Init
// Initialize Video modes subsystem
// *************************************************************************************
// Called from I_StartupGraphics
static
void VID_Init (void)
{
    // initialize the appropriate display device

    // setup the videomodes list,
    // note that mode 1 must always be VGA mode 0x13
    WindowMode_Init();
    num_full_vidmodes = 0;
    full_vidmodes = NULL;
    currentmode_p = NULL;
    // we startup in windowed mode using DIB bitmap
    // we will use DirectDraw when switching fullScreen and entering main game loop
    DIB_mode = TRUE;
    vid.fullscreen = FALSE;
}


// To handle Query, must be able to load and release the drivers.
static byte  loaded_driver = 0;  // vid_drawmode_e

static
void VID_release_driver(void)
{
#ifdef HWRENDER
    if ( loaded_driver >= DRM_opengl )
    {
        HWD.pfnShutdown ();   //close 3d card display
        Shutdown3DDriver ();  //free the driver DLL
    }
    else
#endif
    if( loaded_driver == DRM_native )
    {
        FDX_CloseDirectDraw ();
        fdx_created = false;
    }
    loaded_driver = 0;
}

static
int VID_load_driver( byte request_drawmode )
{
    char * errmsg = NULL;
    char * drvname = NULL;

    if( loaded_driver == request_drawmode )
        return 1;

    if( loaded_driver )
        VID_release_driver();
   
    if( request_drawmode <= DRM_native )
    {
        // Software draw, direct draw driver
        // Only load if not "windows modes only".
        if( ! fdx_created &&  ! req_win )
        {
            FDX_create_main_instance();
            fdx_created = true;
            loaded_driver = DRM_native;
        }
        return 1;
    }

#ifdef HWRENDER
    // Hardware drivers only.
    // initialize the appropriate display device
    switch ( request_drawmode )
    {
      case DRM_opengl:
        drvname = "r_opengl.dll";
        errmsg = "OpenGL";
        break;
#ifdef USE_VOODOO_GLIDE
      case DRM_glide:
        drvname = "r_glide.dll";
        errmsg = "Glide";
        break;
#endif       
      case DRM_minigl:
        // Here is the only difference between OpenGL and MiniGL in the main code
        drvname = "r_minigl.dll";
        errmsg = "MiniGL";
        break;
      case DRM_d3d:
        drvname = "r_d3d.dll";
        errmsg = "D3d";
        break;
      default:
        I_SoftError ("Unknown hardware drawmode");
        return FAIL_invalid_input;
    }

    if( drvname )
    {
        // load the DLL
        if ( Init3DDriver (drvname) )
        {
            int hwdversion = HWD.pfnGetRenderVersion();
            if ( hwdversion != VERSION)
            {
#ifdef USE_VOODOO_GLIDE
                if( request_drawmode == DRM_glide )
                {
                    GenPrintf( EMSG_warn, "WARNING: This r_glide version is not supported, use it at your own risks.\n");
                }
                else
#endif		 
                {   
                    I_Error ("The version of the renderer (v%d.%d) does not match the version of the executable (v%d.%d)\n"
                             "Be sure you have installed Doom Legacy properly.\n"
                             "Eventually verify the launcher settings.\n", 
                             hwdversion/100, hwdversion%100,
                             VERSION/100, VERSION%100);
                }
            }

            // perform initialisations
            HWD.pfnInit ((I_Error_t)I_Error);
            loaded_driver = request_drawmode;
            return 1;
        }
        else
        {
            I_SoftError( "Error initializing %s, driver %s", errmsg, drvname );
            return FAIL_create;
        }
    }

#endif
    return 0;  // not an error
}

// Get Fullscreen modes, append to SPECIAL window modes
// This may change the video driver.
static
int VID_GetModes ( byte request_drawmode, byte select_bitpp )
{
    vmode_t * pv;
    int  iMode;
    // This must be able to get modes before rendermode is set.

#ifdef HWRENDER
    // initialize the appropriate display device
    if( request_drawmode >= DRM_opengl )
    {
        // Hardware draw
        // get available display modes for the device
        HWD.pfnGetModeList (&pv, &nummodes);
        append_full_vidmodes( pv, nummodes ); // append to window modes
    }
#endif
   
    if( request_drawmode < DRM_opengl && !req_win )
    {
        // Software draw, extra modes
        // test for the requested bpp
        // get available display modes for the device
        VID_GetExtraModes ( select_bitpp );
    }

    if( num_full_vidmodes == 0 )
        goto no_modes;

    //DEBUG
    for (iMode=1,pv=full_vidmodes; pv; pv=pv->next)
    {
        GenPrintf( EMSG_debug, "%#02d: %dx%dx%dbpp (desc: '%s')\n",iMode++,
                     pv->width,pv->height,pv->bytesperpixel,pv->name);
    }
    return 1;

no_modes:
    return FAIL_select;
}


// --------------------------
// VID_SetWindowedDisplayMode
// Display the startup 320x200 console screen into a window on the desktop,
// switching to fullscreen display only when we will enter the main game loop.
// - we can display error message boxes for startup errors
// - we can set the last used resolution only once, when entering the main game loop
// --------------------------
static int VID_SetWindowedDisplayMode (viddef_t * lvid, vmode_t * newmode_p)
{
    int     screen_width, screen_height;
    int     window_width, window_height;
    //RECT    Rect;

#ifdef DEBUG
    GenPrintf( EMSG_debug, "VID_SetWindowedDisplayMode()\n");
#endif

    WndNumpages = 1;      // not used

    // allocate screens
    if (!VID_FreeAndAllocVidbuffer (lvid))
        return FAIL_memory;

    // lvid->buffer should be NULL here!

    if ((bmi_main = (void*)GlobalAlloc (GPTR, sizeof(BITMAPINFO) + (sizeof(RGBQUAD)*256)))==NULL)
        I_Error ("VID_SWDM(): No mem");

    // setup a BITMAPINFO to allow copying our video buffer to the desktop,
    // with color conversion as needed
    bmi_main->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi_main->bmiHeader.biWidth = lvid->width;
    bmi_main->bmiHeader.biHeight= -(lvid->height);
    bmi_main->bmiHeader.biPlanes = 1;
    bmi_main->bmiHeader.biBitCount = 8;
    bmi_main->bmiHeader.biCompression = BI_RGB;

    // center window on the desktop
    screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
    screen_height = GetSystemMetrics(SM_CYFULLSCREEN);

    window_width = lvid->width;
    window_width += GetSystemMetrics(SM_CXFIXEDFRAME) * 2;

    window_height = lvid->height;
    window_height += GetSystemMetrics(SM_CYCAPTION);
    window_height += GetSystemMetrics(SM_CYFIXEDFRAME) * 2;

    if( devparm )
        MoveWindow (hWnd_main, (screen_width - window_width)   , (screen_height - window_height)   , window_width, window_height, TRUE);
    else
        MoveWindow (hWnd_main, (screen_width - window_width)>>1, (screen_height - window_height)>>1, window_width, window_height, TRUE);

    SetFocus(hWnd_main);
    ShowWindow(hWnd_main, SW_SHOW);

    hDC_main = GetDC(hWnd_main);
    if( !hDC_main )
        I_Error ("VID_SWDM(): GetDC FAILED");
    //SetStretchBltMode (hDC_main, COLORONCOLOR);

    return 1;
}


// ========================================================================
// Returns a vmode_t from the video modes list, given a video mode number.
// ========================================================================
// Return NULL on fail
static
vmode_t * VID_GetModePtr (modenum_t modenum)
{
    vmode_t *pv = all_vidmodes;  // window
    int mi = modenum.index - 1;  // window 1..  -> 0..

    if ( modenum.index == 0 )
    {
        return &specialmodes[0];  // the HIDDEN INITIAL_WINDOW
    }

    if ( modenum.modetype == MODE_fullscreen )
    {
        pv = full_vidmodes;
        mi = modenum.index - NUMSPECIALMODES;  // 2..  -> 0..
    }
    if (!pv || mi < 0 )  goto fail;

    while (mi--)
    {
        pv = pv->next;
        if (!pv)  goto fail;
    }
    return pv;
fail:
    return NULL;
}


modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    // fullscreen and window modes  1..
    vmode_t *pv = VID_GetModePtr(modenum);
    if( pv )
    {
        ms.width = pv->width;
        ms.height = pv->height;
        ms.type = MODE_either;
        ms.mark = "";
    }
    else
    {
        ms.type = MODE_NOP;
        ms.width = ms.height = 0;
        ms.mark = NULL;
    }
    return ms;
}


//
// return the name of a video mode
//
char * VID_GetModeName( modenum_t modenum )
{
    // fullscreen and window modes  1..
    vmode_t *pv = VID_GetModePtr(modenum);
    return (pv)? pv->name : NULL;
}


// ========================================================================
// Sets a video mode
// ========================================================================
// Returns FAIL_end, FAIL_create, of status_return_e, 1 on success;
int VID_SetMode (modenum_t modenum)
{
    int     stat;
    vmode_t *newmode_p, *oldmode_p;
    viddef_t oldvid = vid;   // to back out
    boolean  set_fullscreen = (modenum.modetype == MODE_fullscreen);
    range_t  range = VID_ModeRange( modenum.modetype );

    vid.draw_ready = 0;  // disable print reaching console

    GenPrintf( EMSG_info, "VID_SetMode(%d,%d)\n", modenum.modetype, modenum.index);

    if ((modenum.index > range.last) || (modenum.index < range.first))
    {
        if (currentmode_p == NULL)
            modenum.index = 0;    // revert to the default base vid mode
        else
        {
            I_SoftError ("Unknown video mode\n");
            return  FAIL_end;
        }
    }

    newmode_p = VID_GetModePtr (modenum);
    if( newmode_p == NULL )
       return  FAIL_end;

    // dont switch to the same display mode
    if (newmode_p == currentmode_p)   goto done;

    // initialize the new mode
    oldmode_p = currentmode_p;
    currentmode_p = newmode_p;

    // initialize vidbuffer size for setmode_func
    vid.width  = currentmode_p->width;
    vid.height = currentmode_p->height;
    vid.bytepp = currentmode_p->bytesperpixel;
    vid.bitpp = (vid.bytepp==1)? 8:15;
#ifdef HWRENDER
    //hurdler: 15/10/99: added
    if (modenum.index)  // if not 320x200 windowed mode
    {
        // it's actually a hack
        if ( (rendermode == render_opengl) || (rendermode == render_d3d) )
        {
#if 0
	    // req_bitpp from cv_scr_depth, or command line.
            // don't accept depth < 16 for OpenGL mode (too much ugly)
            if( req_bitpp < 16 )
                req_bitpp = 16;	     
            vid.bitpp = req_bitpp;
            vid.bytepp = (req_bitpp + 7) >> 3;
#else
            // don't accept depth < 16 for OpenGL mode (too much ugly)
            if (cv_scr_depth.value<16)
                CV_SetValue (&cv_scr_depth,  16);
            vid.bitpp = cv_scr_depth.value;
            vid.bytepp = cv_scr_depth.value/8;
#endif
            vid.fullscreen = set_fullscreen;
            currentmode_p->bytesperpixel = vid.bytepp;
            currentmode_p->modetype = modenum.modetype;  // redundant
        }
    }
#endif

    stat = (*currentmode_p->setmode_func) (&vid, currentmode_p);
      // sets vid.direct, vid.buffer, vid.display, vid.ybytes, vid.screen_size, vid.screen1
      // SetRes for opengl modes in hardware/r_opengl/ogl_win.c
      // VID_SetDirectDrawMode for DD modes
      // VID_SetWindowedDisplayMode for basic window modes
    if (stat < 0)
    {
        if (stat == FAIL_create)
        {
            // hardware could not setup mode
            I_SoftError ("Fail set video mode %d (%dx%d %d bits)\n", modenum.index, vid.width, vid.height, vid.bitpp);
        }
        else if (stat == FAIL_memory)
        {
            I_SoftError ("Not enough mem for VID_SetMode\n");
        }
        if( oldvid.display )
        {
            // restore previous state
            currentmode_p = oldmode_p;
            // cannot just copy oldvid because of buffer pointers that
            // are no longer valid
            vid.width  = oldvid.width;
            vid.height = oldvid.height;
            vid.bytepp = oldvid.bytepp;
            vid.bitpp = oldvid.bitpp;
            (*currentmode_p->setmode_func) (&vid, currentmode_p);
            return FAIL_create;
        }
    }

    vid.drawmode = (vid.bytepp==1)? DRAW8PAL:DRAW15;
    vid.widthbytes = vid.width * vid.bytepp;
    vid.direct_rowbytes = currentmode_p->rowbytes;
    vid.direct_size = vid.direct_rowbytes * vid.height;
    vid.modenum = modenum;

    if ( ! set_fullscreen )
    {
        // we are in a windowed mode
        vid.fullscreen = false;
        DIB_mode = TRUE;
    }
    else
    {
        // we switch to fullscreen
        vid.fullscreen = fdx_fullscreen;
        DIB_mode = FALSE;
    }

    // tell game engine to recalc all tables and realloc buffers based on
    // new vid values
    vid.recalc = 1;

    // judgecutor:
    I_RestartSysMouse();

 done:
    return 1;
}


// ========================================================================
// Free the video buffer of the last video mode,
// allocate a new buffer for the video mode to set.
// ========================================================================
BOOL    VID_FreeAndAllocVidbuffer (viddef_t *lvid)
{
    int  vidbuffersize;

    // Must agree with FinishUpdate, which uses VID_BlitLinearScreen
#if 1 
    // screen size same as video buffer, simple copy
    lvid->ybytes = lvid->direct_rowbytes;
    lvid->screen_size = lvid->direct_size;
#else
    // minimal screen buffer, must copy by line (VID_BlitLinearScreen)
    lvid->ybytes = lvid->widthbytes;
    lvid->screen_size = lvid->ybytes * lvid->height;
#endif
    vidbuffersize = (lvid->screen_size * NUMSCREENS);

    // free allocated buffer for previous video mode
    if (lvid->buffer)
        GlobalFree (lvid->buffer);

    // allocate & clear the new screen buffer
    lvid->buffer = GlobalAlloc (GPTR, vidbuffersize);
    lvid->display = lvid->buffer;  // display = buffer, screen[0]
    if( lvid->buffer == NULL )
    {
        lvid->screen1 = NULL;
        return FALSE;
    }
    lvid->screen1 = lvid->buffer + lvid->screen_size;

#ifdef DEBUG
    GenPrintf( EMSG_debug, "VID_FreeAndAllocVidbuffer done, vidbuffersize: %x\n",vidbuffersize);
#endif
    return TRUE;
}


// ========================================================================
// Set video mode routine for DirectDraw display modes
// Out: 1 ok, FAIL negative codes
// ========================================================================
static int VID_SetDirectDrawMode (viddef_t *lvid, vmode_t * newmode)
{

    if( ! fdx_created )  return -1;
   
#ifdef DEBUG
    GenPrintf( EMSG_debug, "VID_SetDirectDrawMode...\n");
#endif

    // DD modes do double-buffer page flipping, but the game engine doesn't need this..
    WndNumpages = 2;

//MessageBox (hWnd_main, "switch full screen","bla",MB_OK|MB_ICONERROR);

    // release ddraw surfaces etc..
    FDX_ReleaseChtuff();

    // clean up any old vid buffer lying around, alloc new if needed
    if (!VID_FreeAndAllocVidbuffer (lvid))
        return FAIL_memory;                  //no mem

    //added:20-01-98: should clear video mem here

    if (! FDX_InitDDMode(hWnd_main, lvid->width, lvid->height, lvid->bitpp, lvid->fullscreen))
        return FAIL_create;     // could not set mode

    // this is NOT used with DirectDraw modes, game engine should never use this directly
    // but rather render to memory bitmap buffer
    lvid->direct = NULL;

    return 1;
}


// ========================================================================
//                     VIDEO modelist query
// ========================================================================


//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    int ret_value;
    byte  old_loaded_driver = loaded_driver; // must put this back

    // Require modelist before rendermode is set.
   
    // if '-win' is specified on the command line, do not add DirectDraw modes
    req_win = M_CheckParm ("-win");

    if( req_win || ! request_fullscreen )
        return true;  // uses window modes
   
    // Fullscreen modes
    // This may change the video driver.
    ret_value = VID_load_driver( request_drawmode );
    if( ret_value < 0 )
        return ret_value;

    ret_value = VID_GetModes( request_drawmode, request_bitpp );
   
    if( loaded_driver != old_loaded_driver )
    {
        // restore the driver
        VID_load_driver( old_loaded_driver );
    }

    if( ret_value < 0 )
        return false; 
    
    return ( num_full_vidmodes > 0 );
}




// ========================================================================
//                     VIDEO STARTUP and SHUTDOWN
// ========================================================================


// -----------------
// I_StartupGraphics
// Initialize video mode, setup dynamic screen size variables,
// and allocate screens.
// -----------------
// Initialize the graphics system, with a initial window.
void I_StartupGraphics(void)
{
    modenum_t initial_mode = {MODE_window, 0};
    // pre-init by V_Init_VideoControl

    graphics_state = VGS_startup;

    VID_Init();

    COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f);
    COM_AddCommand ("vid_modeinfo", VID_Command_ModeInfo_f);
    COM_AddCommand ("vid_modelist", VID_Command_ModeList_f);
    COM_AddCommand ("vid_mode", VID_Command_Mode_f);

    //added:03-01-98: register exit code for graphics
    I_AddExitFunc (I_ShutdownGraphics);

    native_drawmode = DRM_native;
    native_bitpp = GetDeviceCaps( GetDC( hWnd_main ), BITSPIXEL );
    native_bytepp = (vid.bitpp + 7) >> 3;

    // set the startup window
    if( VID_SetMode ( initial_mode ) < 0 )
    {
        initial_mode.index = 1;  // 320
        if( VID_SetMode ( initial_mode ) < 0 )  goto abort_error;
    };

    graphics_state = VGS_active;
    return;

abort_error:
    // cannot return without a display screen
    I_Error("StartupGraphics Abort\n");
}


// Called to start rendering graphic screen according to the request switches.
// Fullscreen modes are possible.
// param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
// Returns FAIL_select, FAIL_end, FAIL_create, of status_return_e, 1 on success;
int I_RequestFullGraphics( byte select_fullscreen )
{
    modenum_t initial_mode = {MODE_window, 0};
    byte  select_bitpp = 8;  // to select modes
    int ret_value = 0;

    // if '-win' is specified on the command line, do not add DirectDraw modes
    req_win = M_CheckParm ("-win");
//    if ( req_win )
//        rendermode  = render_soft;


//  unsigned int screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
//  unsigned int screen_height = GetSystemMetrics(SM_CYFULLSCREEN);

    switch(req_drawmode)
    {
     case DRM_native:
       if( V_CanDraw( native_bitpp )) {
           select_bitpp = native_bitpp;
       }else{
           // Use 8 bit and do the palette lookup.
#if 0
           if( verbose )
               GenPrintf(EMSG_ver, "Native %i bpp rejected\n", native_bitpp );
#else
           GenPrintf(EMSG_ver, "Native %i bpp rejected\n", native_bitpp );
#endif
           select_bitpp = 8;
       }
       break;
     case DRM_explicit_bpp:
       select_bitpp = req_bitpp;
       break;
     case DRM_opengl:
     case DRM_minigl:
#ifdef USE_VOODOO_GLIDE
     case DRM_glide:
#endif
     case DRM_d3d:
       select_bitpp = native_bitpp;
       break;       
     default:
       select_bitpp = 8;  // default native
       break;
    }
   
    // Change the video driver.
    ret_value = VID_load_driver( request_drawmode );
    if( ret_value < 0 )
        return ret_value;

    ret_value = VID_GetModes( req_drawmode, select_bitpp );
    if( ret_value < 0 )
        return ret_value;

    if( select_fullscreen && (num_full_vidmodes == 0) )
    {
        // if modes not found
        if( req_drawmode == DRM_explicit_bpp && !req_win )
        {
            GenPrintf(EMSG_error, "No %i bpp modes\n", select_bitpp );
            goto no_modes;
        }

        // the game boots in 320x200 standard VGA, but
        // we need a highcolor mode to run the game in highcolor
        if( select_bitpp>8 )
        {
            GenPrintf( EMSG_info,"No highcolor/truecolor VESA2 video mode found, cannot run in highcolor/truecolor.\n");
            goto no_modes;
        }
   
        GenPrint(EMSG_info, "No display modes available.\n");
        goto no_modes;
    }
   
    if( num_all_vidmodes == 0 )
        goto no_modes;

    vid.bitpp = select_bitpp;
    vid.bytepp = (select_bitpp + 7) >> 3;
    vid.width = req_width;
    vid.height = req_height;

    // set the startup screen
    byte  select_fullscreen_mode = vid_mode_table[select_fullscreen];
    initial_mode = VID_GetModeForSize( req_width, req.height, select_fullscreen_mode );
    ret_value = VID_SetMode ( initial_mode );
    if( ret_value < 0 )
        return ret_value;

    graphics_state = VGS_fullactive;
    return 1;

no_modes:
    return FAIL_select;
}


// ------------------
// I_ShutdownGraphics
// Close the screen, restore previous video mode.
// ------------------
void I_ShutdownGraphics (void)
{
    if( graphics_state <= VGS_shutdown )
        return;

    graphics_state = VGS_shutdown;  // to catch some repeats due to errors

    GenPrintf( EMSG_info, "I_ShutdownGraphics()\n");

    // release windowed startup stuff
    if (hDC_main) {
        ReleaseDC (hWnd_main, hDC_main);
        hDC_main = NULL;
    }
    if (bmi_main) {
        GlobalFree (bmi_main);
        bmi_main = NULL;
    }

    if( loaded_driver )
    {
        // graphics are released by render shutdown
        VID_release_driver();
    }

    // free the last video mode screen buffers
    if (vid.buffer) {
        GlobalFree (vid.buffer);
        vid.buffer = NULL;
        vid.display = NULL;
    }

    graphics_state = VGS_off;
}

// Setup HWR calls according to rendermode.
int I_Rendermode_setup( void )
{
    return 1;
}

// ========================================================================
//                     VIDEO MODE CONSOLE COMMANDS
// ========================================================================


//  vid_nummodes
//
static  void VID_Command_NumModes_f (void)
{
    range_t  mr = VID_ModeRange( MODE_fullscreen );
    GenPrintf( EMSG_info, "Video modes %d to %d available(s)\n", mr.first, mr.last );
}


//  vid_modeinfo <modenum>
//
static  void VID_Command_ModeInfo_f (void)
{
    range_t  mr = VID_ModeRange( MODE_fullscreen );
    modenum_t   mn = {MODE_fullscreen, 0};
    vmode_t     *pv;

    if (COM_Argc()!=2)
        mn = vid.modenum;          // describe the current mode
    else
        mn.index = atoi (COM_Argv(1));   //    .. the given mode number

    if (mn.index > mr.last || mn.index < mr.first ) //faB: dont accept the windowed mode 0
    {
        GenPrintf( EMSG_warn, "No such video mode\n");
        return;
    }

    pv = VID_GetModePtr (mn);

    GenPrintf( EMSG_info, "%s\n", VID_GetModeName (mn));
    GenPrintf( EMSG_info, "width : %d\n"
                "height: %d\n", pv->width, pv->height);
    if (rendermode==render_soft)
    {
        GenPrintf( EMSG_info, "bytes per scanline: %d\n"
                    "bytes per pixel: %d\n"
                    "numpages: %d\n",
                    pv->rowbytes,
                    pv->bytesperpixel,
                    pv->numpages);
    }
}


//  vid_modelist
//
static  void VID_Command_ModeList_f (void)
{
    range_t  mr = VID_ModeRange( MODE_fullscreen );
    modenum_t   mn = {MODE_fullscreen, 0};
    int         i;
    char        *modename;
    char        *attr_str;
    vmode_t     *pv;
    boolean     na;

    na = false;
    for (i=mr.first ; i<=mr.last ; i++)
    {
        mn.index = i;
        modename = VID_GetModeName (mn);
        pv = VID_GetModePtr (mn);
        if( pv )
        {
            attr_str = (pv->bytesperpixel==1)? " (hicolor)" : "";
            GenPrintf( EMSG_info, "%d: %s%s\n", i, modename, attr_str);
        }
    }
}


//  vid_mode <modenum>
//
static  void VID_Command_Mode_f (void)
{
    range_t  mr = VID_ModeRange( MODE_fullscreen );
    modenum_t   mn = {MODE_fullscreen, 0};

    if (COM_Argc()!=2)
    {
        GenPrintf( EMSG_warn, "vid_mode <modenum> : set video mode\n");
        return;
    }

    mn.index = atoi(COM_Argv(1));

    if (mn.index > mr.last || mn.index < mr.first ) //faB: dont accept the windowed mode 0
        GenPrintf( EMSG_warn, "No such video mode\n");
    else
    {
        // request vid mode change
        setmodeneeded = mn;
    }
}
