// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video.c 1656 2023-12-08 14:54:47Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: i_video.c,v $
// Revision 1.13  2004/05/16 19:11:53  hurdler
// that should fix issues some people were having in 1280x1024 mode (and now support up to 1600x1200)
//
// Revision 1.12  2002/07/01 19:59:59  metzgermeister
//
// Revision 1.11  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
//
// Revision 1.10  2001/08/20 20:40:42  metzgermeister
//
// Revision 1.9  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.8  2001/04/28 14:25:03  metzgermeister
// fixed mouse and menu bug
//
// Revision 1.7  2001/04/27 13:32:14  bpereira
//
// Revision 1.6  2001/03/12 21:03:10  metzgermeister
//   * new symbols for rendererlib added in SDL
//   * console printout fixed for Linux&SDL
//   * Crash fixed in Linux SW renderer initialization
//
// Revision 1.5  2001/03/09 21:53:56  metzgermeister
//
// Revision 1.4  2001/02/24 13:35:23  bpereira
//
// Revision 1.3  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.2  2000/11/02 19:49:40  bpereira
// Revision 1.1  2000/09/10 10:56:00  metzgermeister
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
//
// DESCRIPTION:
//      DOOM graphics stuff for SDL
//
//-----------------------------------------------------------------------------

// Debugging unfinished MAC_SDL
//#define DEBUG_MAC  1

//#define DEBUG_VID

//#define TESTBPP
#ifdef TESTBPP
// [WDJ] Test drawing in a testbpp mode, using native mode conversion.
static int testbpp = 0;
#endif

#include <stdlib.h>

#include <SDL.h>

#include "doomincl.h"
#include "doomstat.h"

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
  // vid_mode_table, etc
#include "m_argv.h"
#include "m_menu.h"
#include "d_main.h"
#include "s_sound.h"
#include "g_input.h"
#include "st_stuff.h"
#include "g_game.h"
#include "hardware/hw_main.h"
#include "hardware/hw_drv.h"
#include "console.h"
#include "hwsym_sdl.h" // For dynamic referencing of HW rendering functions
#include "ogl_sdl.h"


//Hudler: 16/10/99: added for OpenGL gamma correction
RGBA_t  gamma_correction = {0x7F7F7F7F};

// SDL vars

#ifdef SDL2
SDL_Window * sdl_window = NULL;
SDL_Texture * sdl_texture = NULL;
SDL_Renderer * sdl_renderer = NULL;
uint16_t  display_index = 0;  // SDL2 can have multiple displays
#endif

// Only one vidSurface, else releasing oldest faults in SDL.
// Shared with ogl.
SDL_Surface * vidSurface = NULL;

static  SDL_Color    localPalette[256];


// Video mode list

// [WDJ] SDL returned modelist is not our memory to manage, do not free.
// Several SDL functions, on some platforms, will call SDL_ListModes again, which can release
// the previous modelist.
// The SDL modelist cannot be trusted after: SDL_VideoModeOK, SDL_GetVideoMode, SDL_ListModes.
// We will immediately make a copy of the modes we want, and let go of the SDL modelist.

// Fullscreen modelist
typedef struct {
    uint16_t  w, h;
} vid_mode_t;

static  vid_mode_t *  vid_modelist = NULL;
static  int  num_vid_mode_allocated = 0;
static  int  num_vid_mode = 0;
static  byte       modelist_bitpp = 0;  // with modelist

// indexed by fullscreen
const char * fullscreen_str[2] = {
  "Windowed",
  "Fullscreen",
};

#ifdef SDL2
// indexed by fullscreen
const static uint32_t sdl_window_flags[2] = {
  SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_SHOWN,   // windowed
  SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED,  // fullscreen
};

#else
// SDL 1.2
static  byte       request_NULL = 0;  // with modelist

// Surface flags, parameter for SDL 1.2
// indexed by fullscreen
const static Uint32 surface_flags[2] = {
#ifdef __MACOSX__
  // SDL_DOUBLEBUF is unsupported for Mac OS X
  SDL_SWSURFACE|SDL_HWPALETTE,
  // With SDL 1.2.6 there is an experimental software flipping that is
  // accessed using SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_FULLSCREEN
  SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_FULLSCREEN|SDL_HWPALETTE
#else
#if 1
  // NO DOUBLEBUF, as we already draw to buffer
  SDL_HWSURFACE|SDL_HWPALETTE,
  SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN
#else
  // DOUBLEBUF
  SDL_HWSURFACE|SDL_HWPALETTE|SDL_DOUBLEBUF,
  SDL_HWSURFACE|SDL_HWPALETTE|SDL_DOUBLEBUF|SDL_FULLSCREEN
#endif
#endif
};
#endif


// maximum number of windowed modes
#ifdef FIT_RATIO
#define MAXWINMODES (11)
#else
#define MAXWINMODES (8)
#endif
// windowed video modes from which to choose from.
static int windowedModes[MAXWINMODES+1][2] = {
   // hidden from display
    {INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT},  // initial mode
   // public  1..
    {MAXVIDWIDTH /*1600*/, MAXVIDHEIGHT/*1200*/},
#ifdef FIT_RATIO
    {1600, 1024},
#endif   
    {1280, 1024},
#ifdef FIT_RATIO
    {1280, 800},
#endif   
    {1024, 768},
#ifdef FIT_RATIO
    {1024, 600},
#endif   
    {800, 600},
    {640, 480},
    {512, 384},
    {400, 300},
    {320, 200}
};


// Add a video mode to the list.
static
void  add_vid_mode( int w, int h )
{
    vid_mode_t * vm;

    if( num_vid_mode >= num_vid_mode_allocated )
    {
       // Expand the video mode list.
       int req_num = num_vid_mode_allocated + 16;
       vm = realloc( vid_modelist, sizeof(vid_mode_t) * req_num );
       if( vm == NULL )  return;
       vid_modelist = vm;
       num_vid_mode_allocated = req_num;
    }
    vm = & vid_modelist[ num_vid_mode++ ];
    vm->w = w;
    vm->h = h;
}

#ifdef SDL2
// indexed by sw_drawmode_e
static const uint32_t sdl_pixelformats[6] =
{
  SDL_PIXELFORMAT_INDEX8,  // DRAW8PAL
  SDL_PIXELFORMAT_RGB555,  // DRAW15
  SDL_PIXELFORMAT_RGB565,  // DRAW16
  SDL_PIXELFORMAT_RGB888,  // DRAW24
  SDL_PIXELFORMAT_RGBA8888,  // DRAW32
  SDL_PIXELFORMAT_RGBA32,  // DRAWGL
};
#endif

//
// I_StartFrame
//
void I_StartFrame(void)
{
    // no longer lock, no more assumed direct access
#if 0 
    if( rendermode == render_soft )
    {
        if(SDL_MUSTLOCK(vidSurface))
        {
            if(SDL_LockSurface(vidSurface) < 0)
                return;
        }
    }
#endif

    return;
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
    /* this function intentionally left empty */
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
    if( rendermode == render_soft )
    {
#ifdef SDL2
        // From Eternity Engine.
        // Windows alt-tab during fullscreen will hide fullscreen,
        // and bad things happen (unspecified) if we draw to it then.
        if( ! (SDL_GetWindowFlags( sdl_window ) & SDL_WINDOW_SHOWN) )
            return;
#else
        // [WDJ] Only lock during transfer itself.  The only access
        // to vid.direct is in this routine.
        if(SDL_MUSTLOCK(vidSurface))
            if(SDL_LockSurface(vidSurface) < 0)
                return;
#endif       

#ifdef TESTBPP
        // [WDJ] To test drawing in 15bpp, 16bpp, 24bpp, convert the
        // screen drawn in the testbpp mode to the native bpp mode.
        if( testbpp )
        {
            byte * vidmem = vid.direct;
            byte * src = vid.display;
            int h = vid.height;
            while( h-- )
            {
                int w=vid.width;
                switch( testbpp )
                {
                 case 15:
                  {
                    uint32_t * v32 = (uint32_t*) vidmem;
                    uint16_t * s16 = (uint16_t*) src;
                    while( w--)
                    {
                        *v32 = ((*s16&0x7C00)<<9)|((*s16&0x03E0)<<6)|((*s16&0x001F)<<3);
                        v32++;
                        s16++;
                    }
                  }
                  break;
                 case 16:
                  {
                    uint32_t * v32 = (uint32_t*) vidmem;
                    uint16_t * s16 = (uint16_t*) src;
                    while( w--)
                    {
                        *v32 = ((*s16&0xF800)<<8)|((*s16&0x07E0)<<5)|((*s16&0x001F)<<3);
                        v32++;
                        s16++;
                    }
                  }
                  break;
                 case 24:
                  {
                    byte* v = vidmem;
                    byte* s = src;
                    while( w--)
                    {
                        *(uint16_t*)v = *(uint16_t*)s;  // g, b
                        v[2] = s[2];  // r
                        v+=4;
                        s+=3;
                    }
                  }
                  break;
                 case 32:
                  memcpy( vidmem, src, vid.widthbytes );
                  break;
                }
                src += vid.ybytes;
                vidmem += vid.direct_rowbytes;
            }
        }
        else
#endif

#ifdef SDL2
        // Update the SDL_Texture that is in video memory.
        SDL_UpdateTexture( sdl_texture, NULL, vid.display, vid.direct_rowbytes );

        // SDL2 docs use RenderClear, but we do not have any conflicting drawers.
//        SDL_RenderClear( sdl_renderer );
        SDL_RenderCopy( sdl_renderer, sdl_texture, NULL, NULL );  // to video framebuffer
        SDL_RenderPresent( sdl_renderer );  // make it current
#else
        // SDL 1.2
        // [WDJ] SDL Spec says that you can directly read and write the surface
        // while it is locked.
        if(vid.display != vid.direct)
        {
#if 0	   
            VID_BlitLinearScreen( vid.display, vid.direct, vid.widthbytes, vid.height, vid.ybytes, vid.direct_rowbytes);
#else
            if( (vid.widthbytes == vid.direct_rowbytes) && (vid.ybytes == vid.direct_rowbytes))
            {
                // fast, copy entire buffer at once
                memcpy(vid.direct, vid.display, vid.direct_size);
                //screens[0] = vid.direct; //FIXME: we MUST render directly into the surface
            }
            else
            {
                // [WDJ] padded video buffer (Mac)
                // Some cards use the padded space, so DO NOT STOMP ON IT.
                int h = vid.height;
                byte * vidmem = vid.direct;
                byte * src = vid.display;
                while( h-- )
                {
                    memcpy(vidmem, src, vid.widthbytes);  // width limited
                    vidmem += vid.direct_rowbytes;
                    src += vid.ybytes;
                }
            }
#endif
        }

        if(SDL_MUSTLOCK(vidSurface))
        {
            SDL_UnlockSurface(vidSurface);
        }
        // If page flip involves changing vid.display, then must change screens[0] too
        // [WDJ] SDL spec says to not call UpdateRect while vidSurface is locked
        //SDL_Flip(vidSurface);
#ifdef __MACOSX__
        // Setup Flip of DOUBLEBUF
        SDL_Flip(vidSurface);
        // Hardware that does not support DOUBLEBUF does
        // SDL_UpdateRect(vidSurface, 0, 0, 0, 0);
#else
        SDL_UpdateRect(vidSurface, 0, 0, 0, 0);
#endif
#endif
    }
    else
    {
        OglSdl_FinishUpdate();
    }

    I_GetEvent();

    return;
}


//
// I_ReadScreen
//
// Screen to screen copy
void I_ReadScreen(byte* scr)
{
    if( rendermode != render_soft )
        I_Error ("I_ReadScreen: called while in non-software mode");

#if 0	   
    VID_BlitLinearScreen( src, vid.display, vid.widthbytes, vid.height, vid.ybytes, vid.ybytes);
#else
    if( vid.widthbytes == vid.ybytes )
    {
        // fast, copy entire buffer at once
        memcpy (scr, vid.display, vid.screen_size);
    }
    else
    {
        // [WDJ] padded video buffer (Mac)
        int h = vid.height;
        byte * vidmem = vid.display;
        while( h-- )
        {
            memcpy(scr, vidmem, vid.widthbytes);
            vidmem += vid.ybytes;
            scr += vid.ybytes;
        }
    }
#endif
}



//
// I_SetPalette
//
void I_SetPalette(RGBA_t* palette)
{
    int i;

    for(i=0; i<256; i++)
    {
        localPalette[i].r = palette[i].s.red;
        localPalette[i].g = palette[i].s.green;
        localPalette[i].b = palette[i].s.blue;
    }

#ifdef SDL2
    // Need a surface for palette.
    if( ! vidSurface )
        return;

#if defined(MAC_SDL) && defined( DEBUG_MAC )
    if( SDL_SetPaletteColors(vidSurface->format->palette, localPalette, 0, 256) < 0 )
    {
        GenPrintf( EMSG_error,"Error: SDL_SetPaletteColors failed to set all colors\n");
    }
#else
    SDL_SetPaletteColors(vidSurface->format->palette, localPalette, 0, 256);
#endif
#else
#if defined(MAC_SDL) && defined( DEBUG_MAC )
    if( ! SDL_SetColors(vidSurface, localPalette, 0, 256) )
    {
        GenPrintf( EMSG_error,"Error: SDL_SetColors failed to set all colors\n");
    }
#else
    SDL_SetColors(vidSurface, localPalette, 0, 256);
#endif
#endif

    return;
}


//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
#ifdef SDL2
    SDL_DisplayMode  mode;
    int num_modes = SDL_GetNumDisplayModes( display_index );
    int i;

    if( request_bitpp == 8 || request_drawmode == DRM_opengl )
    {
        // 8 bit palette mode
        // SDL will convert to native, but there is no 8pal modelist.
        request_bitpp = native_bitpp;
    }

# ifdef DEBUG_VID
    printf( "VID_Query_Modelist: drawmode=%i, bitpp=%i   ",
        request_drawmode, request_bitpp, );
# endif
    for(i=0; i<num_modes; i++)
    {
        // Query display=0
        if( SDL_GetDisplayMode( 0, i, & mode ) < 0 )  continue;
        byte bpp = SDL_BITSPERPIXEL( mode.format );
        if( (bpp == request_bitpp)
            && (mode.w <= MAXVIDWIDTH)
            && (mode.h <= MAXVIDHEIGHT) )
        {
# ifdef DEBUG_VID
                printf( "VALID\n" );
# endif
                return true;
        }
    }
# ifdef DEBUG_VID
    printf( "NO\n" );
# endif
    return false;

#else
    // SDL 1.2
    SDL_PixelFormat    req_format;
    SDL_Rect   **modelist2;
   
    // Require modelist before rendermode is set.

    if( request_bitpp == 8 || request_drawmode == DRM_opengl )
    {
        // 8 bit palette mode
        // SDL will convert to native, but there is no 8pal modelist.
        req_format.BitsPerPixel = native_bitpp;
    }
    else
    {
        req_format.BitsPerPixel = request_bitpp;
    }
# ifdef DEBUG_VID
    printf( "VID_Query_Modelist: drawmode=%i, bitpp=%i  (%i)   ",
        request_drawmode, request_bitpp, req_format.BitsPerPixel );
# endif
    modelist2 = SDL_ListModes(&req_format, surface_flags[request_fullscreen] );
# ifdef DEBUG_VID
    printf( "%s modelist\n", modelist2? "VALID":"NO" );
# endif
    return ( modelist2 != NULL );
#endif   
}

//   request_bitpp :  8,15,16,24,32
//                    1 = ANY mode, for 8 bit palette
static
void  VID_make_fullscreen_modelist( byte request_bitpp )
{
#ifdef SDL2
    SDL_DisplayMode  mode;
    int num_modes = SDL_GetNumDisplayModes( display_index );
    int i;

    if( request_bitpp < 8 )
    {
        request_bitpp = native_bitpp;
    }
    modelist_bitpp = request_bitpp;

    num_vid_mode = 0;
    for(i=0; i<num_modes; i++)
    {
        // Query display=0
        // SDL2
        if( SDL_GetDisplayMode( 0, i, & mode ) < 0 )  continue;
        byte bpp = SDL_BITSPERPIXEL( mode.format );

        if( verbose )
        {
            // list the modes
            GenPrintf( EMSG_ver, "%s %ibpp %ix%i",
                (((i&0x03)==0)?(i)?"\nModes ":"Modes ":""), bpp, mode.w, mode.h );
        }

        if( (bpp == request_bitpp)
            && (mode.w <= MAXVIDWIDTH)
            && (mode.h <= MAXVIDHEIGHT) )
        {
            add_vid_mode( mode.w, mode.h );
        }
    }
    return;

#else
    SDL_Rect   ** modelist;
    SDL_PixelFormat    req_format;
    int i;

    num_vid_mode = 0;
   
    // Lets default to 8 bit.
    if( request_bitpp < 8 )
    {
        request_NULL = 1;
        request_bitpp = 8;
    }
    modelist_bitpp = request_bitpp;
    req_format.BitsPerPixel = request_bitpp;
    req_format.BytesPerPixel = 0;  // ignored

    // The SDL_ListModes only pays attention to the req_format BitsPerPixel, and the flags.
    modelist = SDL_ListModes( (request_NULL? NULL : &req_format), surface_flags[1] );
    if( modelist < 0 )
    {
         // SDL return value that indicates that all modes are valid.
         if( verbose )
         {
            GenPrintf( EMSG_ver, "All modes are valid.\n" );
         }
       
         // make a mode list from the windowed modes
         for( i=1; i<MAXWINMODES; i++ )
             add_vid_mode( windowedModes[i][0], windowedModes[i][1] );

         modelist_bitpp = request_bitpp;
         return;
    }

    if( ! modelist )
       return;
   
    // Prepare Mode List
    num_vid_mode = 0;
    // SDL modelist is array of ptr, last ptr is NULL.
    for( i=0; modelist[i]; i++ )
    {
        if( verbose )
        {
            // list the modes
            GenPrintf( EMSG_ver, "%s %ix%i",
                     (((i&0x03)==0)?(i)?"\nModes ":"Modes ":""),
                     modelist[i]->w, modelist[i]->h );
        }
        if((modelist[i]->w <= MAXVIDWIDTH) && (modelist[i]->h <= MAXVIDHEIGHT))
        {
            add_vid_mode( modelist[i]->w, modelist[i]->h );
        }
    }
    return;
#endif
}



// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };  // first is always 1
    mrange.last = (modetype == MODE_fullscreen) ?
        num_vid_mode  // fullscreen
      : MAXWINMODES;  // window
    return mrange;
}


modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    if( modenum.modetype == MODE_fullscreen )
    {
        // fullscreen modes  1..
        int mi = modenum.index - 1;
        if(mi >= num_vid_mode)   goto fail;

        ms.width = vid_modelist[mi].w;
        ms.height = vid_modelist[mi].h;
        ms.type = MODE_fullscreen;
        ms.mark = "";
    }
    else
    {
        // windowed modes  1.., sometimes 0
        if(modenum.index > MAXWINMODES)   goto fail;

        ms.width = windowedModes[modenum.index][0];
        ms.height = windowedModes[modenum.index][1];
        ms.type = MODE_window;
        ms.mark = "win ";
    }
    return  ms;

fail:
    ms.type = MODE_NOP;
    ms.width = ms.height = 0;
    ms.mark = NULL;
    return ms;
}

// Static mode name storage
#define  MAX_NUM_VIDMODENAME  42
#define  MAX_LEN_VIDMODENAME  16
static char  mode_name_store[MAX_NUM_VIDMODENAME][MAX_LEN_VIDMODENAME];

// The menu code will keep a ptr to this description string.
char * VID_GetModeName( modenum_t modenum )
{
    modestat_t  ms = VID_GetMode_Stat( modenum );
    if( ! ms.mark )
       return NULL;

    if( modenum.index >= MAX_NUM_VIDMODENAME )  // unreasonable
       return NULL;

    char * mode_name = & mode_name_store[modenum.index][0];
    snprintf(mode_name, MAX_LEN_VIDMODENAME, "%s%dx%d",
                ms.mark, ms.width, ms.height  );
    mode_name[MAX_LEN_VIDMODENAME-1] = 0;  // term string
    return mode_name;
}


//   rmodetype : modetype_e
// Returns MODE_NOP when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype )
{
    modenum_t  modenum = { MODE_NOP, 0 };
    int bestdist = INT_MAX;
    int tdist, i;

    if( rmodetype == MODE_fullscreen )
    {
#ifdef DEBUG_VID
        printf( "VID_GetModeFor_Size Fullscreen rw=%i, rh=%i, num_vid_mode=%i\n", rw, rh, num_vid_mode );
#endif

        if( num_vid_mode == 0 )
            goto done;

        modenum.index = num_vid_mode - 1;  // default is smallest mode
        // search our copy of the SDL modelist
        for(i=0; i<num_vid_mode; i++)
        {
            tdist = abs(vid_modelist[i].w - rw) + abs(vid_modelist[i].h - rh);
            // find closest dist
            if( bestdist > tdist )
            {
                bestdist = tdist;
                modenum.index = i + 1;  // 1..
                if( tdist == 0 )
                    goto ret_vidmode;   // found exact match
            }
        }
    }
    else
    {
        modenum.index = MAXWINMODES;  // default is smallest mode

        // window mode index returned 1..
        for(i=1; i<=MAXWINMODES; i++)
        {
            tdist = abs(windowedModes[i][0] - rw) + abs(windowedModes[i][1] - rh);
            // find closest dist
            if( bestdist > tdist )
            {
                bestdist = tdist;
                modenum.index = i; // 1..
                if( tdist == 0 )
                    goto ret_vidmode;   // found exact match
            }
        }
    }
ret_vidmode:   
    modenum.modetype = rmodetype;
done:
    return modenum;
}

// Release SDL buffers
void VID_SDL_release( void )
{
    if( vidSurface )
    {
        SDL_FreeSurface(vidSurface);
        vidSurface = NULL;
    }

#ifdef SDL2
    // SDL2 may use vidSurface, for 8bit palette.
    if( sdl_texture )
    {
        SDL_DestroyTexture( sdl_texture );
        sdl_texture = NULL;
    }
    if( sdl_renderer )
    {
        SDL_DestroyRenderer( sdl_renderer );
        sdl_renderer = NULL;
    }
    if( sdl_window )
    {
        SDL_DestroyWindow( sdl_window );
        sdl_window = NULL;
    }
#endif
}


// Set video mode and vidSurface, with verbose
//   req_fullscreen :  1=fullscreen, used as index
static
void  VID_SetMode_vid( int req_width, int req_height, int req_fullscreen )
{
#ifdef SDL2
    uint32_t sdl_reqflags = sdl_window_flags[req_fullscreen];
    // SDL2 does not have test for VideoModeOK
#else
    // [WDJ] SDL_VideoModeOK calls SDL_ListModes which invalidates the previous modelist.
    // This is why we keep our own copy.
    uint32_t  sdl_reqflags = surface_flags[req_fullscreen];
    int cbpp = SDL_VideoModeOK(req_width, req_height, modelist_bitpp, sdl_reqflags);
    if( cbpp == 0 )
        return; // SetMode would have failed, keep current buffers
#endif

    if( verbose>1 )
    {
        GenPrintf( EMSG_ver,"SetVideoMode(%i,%i,%ibpp)  %s\n",
            req_width, req_height, modelist_bitpp, fullscreen_str[req_fullscreen] );
    }

    free(vid.buffer); // was malloc
    vid.display = NULL;
    vid.buffer = NULL;
    vid.direct = NULL;
    vid.width = req_width;
    vid.height = req_height;

#ifdef SDL2
    VID_SDL_release();

    SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "0" );  // disable
//    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "software" );  // no opengl
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );  // nearest, linear, best

#ifdef DEBUG_WINDOWED
//    sdl_window = SDL_CreateWindow( "Doom Legacy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    sdl_window = SDL_CreateWindow( "Doom Legacy", 0, 0,
                                   req_width, req_height, sdl_reqflags );
#else
    sdl_window = SDL_CreateWindow( "Doom Legacy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   req_width, req_height, sdl_reqflags );
#endif
    if( sdl_window == NULL)
        return;  // Modes were prechecked, SDL should not fail.

    // Can get the window surface, and draw to that.
    // Or can get a texture, and copy our screen to that.
    // Do not try to do both, bad things happen.
    // An SDL_renderer is needed to render anything to GPU memory.
    // An SDL_Texture resides in GPU memory.
    // With coronas and other stuff, we are doing software rendering.
    // But we still must get it into video memory.

#if 1
    // Note from Eternity Engine: SDL_RENDERER_SOFTWARE fails, use SDL_RENDERER_TARGETTEXTURE.
    uint32_t rend_reqflags = SDL_RENDERER_TARGETTEXTURE;
    if( cv_vidwait.EV )   rend_reqflags |= SDL_RENDERER_PRESENTVSYNC;

    sdl_renderer = SDL_CreateRenderer( sdl_window, -1, rend_reqflags );
    if( sdl_renderer == NULL)
        goto failed;
#endif

    // Will need an SDL_Texture.
    // Get Pixel format (Eternity Engine).
    uint32_t pixel_format = SDL_GetWindowPixelFormat( sdl_window );
    if( pixel_format == SDL_PIXELFORMAT_UNKNOWN )
        pixel_format = sdl_pixelformats[ vid.drawmode ];

    // A texture that is updated each frame (streaming).
    sdl_texture = SDL_CreateTexture( sdl_renderer, pixel_format,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    req_width, req_height );
    if( sdl_texture == NULL)
        goto failed;

    // Get surface for palette draw
//    if( modelist_bitpp == 8 )
    {
        vidSurface = SDL_GetWindowSurface( sdl_window );

        vid.bitpp = vidSurface->format->BitsPerPixel;
        vid.bytepp = vidSurface->format->BytesPerPixel;

        // The video buffer might be padded to power of 2, for some modes (Mac)
        vid.direct_rowbytes = vidSurface->pitch; // correct, even on Mac
        vid.direct_size = vidSurface->pitch * vid.height; // correct, even on Mac
        vid.direct = vidSurface->pixels;
    }

   
#else
    // SDL 1.2
    VID_SDL_release();

    vidSurface = SDL_SetVideoMode(vid.width, vid.height, modelist_bitpp, surface_flags[req_fullscreen] );
    if(vidSurface == NULL)
        goto failed;  // Modes were prechecked, SDL should not fail.
 
    if( verbose )
    {
        int32_t vflags = vidSurface->flags;
        GenPrintf( EMSG_ver,"  Got %ix%i, %i bpp, %i bytes\n",
                vidSurface->w, vidSurface->h,
                vidSurface->format->BitsPerPixel, vidSurface->format->BytesPerPixel );
        GenPrintf( EMSG_ver,"  HW-surface= %x, HW-palette= %x, HW-accel= %x, Doublebuf= %x, Async= %x \n",
                vflags&SDL_HWSURFACE, vflags&SDL_HWPALETTE, vflags&SDL_HWACCEL, vflags&SDL_DOUBLEBUF, vflags&SDL_ASYNCBLIT );
        if(SDL_MUSTLOCK(vidSurface))
            GenPrintf( EMSG_ver,"  Notice: MUSTLOCK video surface\n" );
    }
    if( vidSurface->w != vid.width || vidSurface->h != vid.height )
    {
        GenPrintf( EMSG_ver,"  Adapting to VideoMode: requested %ix%i, got %ix%i\n",
                vid.width, vid.height,
                vidSurface->w, vidSurface->h );
        vid.width = vidSurface->w;
        vid.height = vidSurface->h;
    }
    if( vidSurface->format->BitsPerPixel != modelist_bitpp )
    {
        GenPrintf( EMSG_ver,"  Notice: requested %i bpp, got %i bpp\n",
                modelist_bitpp, vidSurface->format->BitsPerPixel );
    }

    vid.bitpp = vidSurface->format->BitsPerPixel;
    vid.bytepp = vidSurface->format->BytesPerPixel;

    // The video buffer might be padded to power of 2, for some modes (Mac)
    vid.direct_rowbytes = vidSurface->pitch; // correct, even on Mac
    vid.direct_size = vidSurface->pitch * vid.height; // correct, even on Mac
    vid.direct = vidSurface->pixels;
#endif

#ifdef TESTBPP
    if( testbpp )
    {
        // [WDJ] Force the testbpp drawing mode
        vid.bitpp = testbpp;
        switch( testbpp )
        {
         case 15:
         case 16:
           vid.bytepp = 2;
           break;
         case 24:
           vid.bytepp = 3;
           break;
         case 32:
           vid.bytepp = 4;
           break;
        }
    }
#endif

#if 1
 // normal
    // Because we have to copy by row anyway, buffer can be normal
    // Have option to change this for special cases,
    // most code uses vid.ybytes now, and is padded video safe.
    vid.ybytes = vid.width * vid.bytepp;
    vid.screen_size = vid.ybytes * vid.height;
#else
 // DEBUG padded video buffer code
    vid.ybytes = vid.width * vid.bytepp + 8;  // force odd size
    vid.screen_size = vid.ybytes * vid.height;
#endif
    // display is buffer
    vid.buffer = malloc(vid.screen_size * NUMSCREENS);
    vid.display = vid.buffer;
    vid.screen1 = vid.buffer + vid.screen_size;
    vid.recalc = true;
    return;

failed:
    // To avoid leaving bad fullscreen.
    if( ! req_fullscreen )
        return;

    VID_SDL_release();
}
 

// SDL version of VID_SetMode
// Returns FAIL_end, FAIL_create, of status_return_e, 1 on success;
int VID_SetMode(modenum_t modenum)
{
    int req_width, req_height;
    byte set_fullscreen = (modenum.modetype == MODE_fullscreen);

    vid.draw_ready = 0;  // disable print reaching console
    vid.recalc = true;

    GenPrintf( EMSG_info, "VID_SetMode(%s,%i)\n",
               modetype_string[modenum.modetype], modenum.index);

    I_UngrabMouse();
   
    if( ogl_active )
    {
        OglSdl_Shutdown();
    }

    if( set_fullscreen )
    {
        // fullscreen
        int mi = modenum.index - 1;   // modenum.index is 1..
        if( mi >= num_vid_mode )  goto fail;
        // Our copy of the modelist.
        req_width = vid_modelist[mi].w;
        req_height = vid_modelist[mi].h;

        if( rendermode == render_soft )
        {
            VID_SetMode_vid(req_width, req_height, 1 );  // fullscreen
        }
        else
        {
            // HWR rendermode, fullscreen
            if(!OglSdl_SetMode(req_width, req_height, true))
                goto fail;
        }
    }
    else
    {
        // not fullscreen, window, 1..
        // modenum == 0 is INITIAL_WINDOW_WIDTH
        int mi = modenum.index;
        req_width = windowedModes[mi][0];
        req_height = windowedModes[mi][1];

        if( rendermode == render_soft )
        {
            VID_SetMode_vid( req_width, req_height, 0 );  // window
        }
        else
        {
            // HWR rendermode, window
            if(!OglSdl_SetMode(req_width, req_height, 0))
                goto fail;
        }
    }

#ifdef SDL2
    // sdl_window is shared and required for both sw and hw.
    if( (sdl_window == NULL) || (sdl_texture == NULL) )  goto fail;
#else
    // vidSurface is shared and required for both sw and hw.
    if( vidSurface == NULL )  goto fail;
#endif

    vid.modenum = modenum;
    vid.fullscreen = set_fullscreen;
    vid.widthbytes = vid.width * vid.bytepp;

//    V_Setup_VideoDraw(); // setup screen for print messages, redundant.

    I_StartupMouse( false );

#if defined(MAC_SDL) && defined( DEBUG_MAC )
    SDL_Delay( 2 * 1000 );  // [WDJ] DEBUG: to see if errors are due to startup or activity
#endif
    return 1;

fail:
    I_SoftError("VID_SetMode failed to provide display\n");
    return  FAIL_create;
}


// Voodoo card has video switch, produces fullscreen 3d graphics,
// and we cannot use window mode with it.
boolean  have_voodoo = false;

// Have to determine how to detect a voodoo card
// If anyone ever tries a voodoo card again, they will have to fix this.
static boolean detect_voodoo( void )
{
#ifdef SDL2
    const char * vb = SDL_GetCurrentVideoDriver();  // name
#else
    char vb[1024];
    SDL_VideoDriverName( vb, 1022 );
#endif

    if( strstr( "Voodoo", vb ) != NULL )
       have_voodoo = true;
    return have_voodoo;
}


// Setup HWR calls according to rendermode.
int I_Rendermode_setup( void )
{
    if( rendermode == render_opengl )
    {
       HWD.pfnInit             = hwSym("Init");
       HWD.pfnFinishUpdate     = hwSym("FinishUpdate");
       HWD.pfnDraw2DLine       = hwSym("Draw2DLine");
       HWD.pfnDrawPolygon      = hwSym("DrawPolygon");
       HWD.pfnSetBlend         = hwSym("SetBlend");
       HWD.pfnClearBuffer      = hwSym("ClearBuffer");
       HWD.pfnSetTexture       = hwSym("SetTexture");
       HWD.pfnReadRect         = hwSym("ReadRect");
       HWD.pfnGClipRect        = hwSym("GClipRect");
       HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache");
       HWD.pfnSetSpecialState  = hwSym("SetSpecialState");
       HWD.pfnSetPalette       = hwSym("SetPalette");
       HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed");

       HWD.pfnDrawMD2          = hwSym("DrawMD2");
       HWD.pfnSetTransform     = hwSym("SetTransform");
       HWD.pfnGetRenderVersion = hwSym("GetRenderVersion");

       // check gl renderer lib
       if (HWD.pfnGetRenderVersion() != DOOMLEGACY_COMPONENT_VERSION )
       {
           I_Error ("The version of the renderer doesn't match the version of the executable\n"
                    "Be sure you have installed DoomLegacy properly.\n");
       }
    }
    return 1;
}


// Initialize the graphics system, with a initial window.
void I_StartupGraphics( void )
{
    modenum_t  initialmode = {MODE_window,0};  // the initial mode
    // pre-init by V_Init_VideoControl

    modelist_bitpp = 8;
   
    graphics_state = VGS_startup;
    native_drawmode = DRM_native;

#ifdef SDL2
    int vi;
    int num_video_drivers = SDL_GetNumVideoDrivers();
    for(vi=0; vi < num_video_drivers; vi++)
    {
        if( verbose )
        {
            GenPrintf( EMSG_ver,"SDL video driver = %s\n",
                SDL_GetVideoDriver( vi ) );
        }
    }

    native_bitpp = 0;
    int num_video_displays = SDL_GetNumVideoDisplays();
    for(vi=0; vi < num_video_displays; vi++)
    {
        SDL_DisplayMode dispmode;
        int err = SDL_GetCurrentDisplayMode( vi, &dispmode );
        if( err < 0 )  continue;
       
        // Decode the pixel format.
        uint32_t pformat = dispmode.format;
        byte     bpp = SDL_BITSPERPIXEL( pformat );
        byte     bytepp = SDL_BYTESPERPIXEL( pformat );
       
        if( bpp > native_bitpp )
        {
            native_bitpp = bpp;
            native_bytepp = bytepp;
        }

        if( verbose )
        {
            GenPrintf( EMSG_ver,"SDL video display [%i] = %s { %i bpp, %i byte }\n",
                    vi, SDL_GetDisplayName( vi ), bpp, bytepp );
            if( verbose > 1 )
            {
                GenPrintf( EMSG_ver,"    %s\n",
                    SDL_GetPixelFormatName( pformat ) );
            }
        }
    }

    if( native_bitpp == 0 )
    {
        GenPrintf( EMSG_info,"No SDL video info, use default\n" );
        native_bitpp = 8;
        native_bytepp = 1;
    }
#else
    // Get and report video info
    const SDL_VideoInfo * videoInfo = (const SDL_VideoInfo *) SDL_GetVideoInfo();
    if( videoInfo )
    {
        native_bitpp = videoInfo->vfmt->BitsPerPixel;
        native_bytepp = videoInfo->vfmt->BytesPerPixel;
        if( verbose )
        {
            GenPrintf( EMSG_ver,"SDL video info = { %i bpp, %i byte }\n",
                videoInfo->vfmt->BitsPerPixel, videoInfo->vfmt->BytesPerPixel );
            if( verbose > 1 )
            {
                GenPrintf( EMSG_ver," HW_surfaces= %i, blit_hw= %i, blit_sw = %i\n",
                    videoInfo->hw_available, videoInfo->blit_hw, videoInfo->blit_sw );
                GenPrintf( EMSG_ver," video_mem= %i K\n",
                    videoInfo->video_mem );
            }
        }
    }
    else
    {
        GenPrintf( EMSG_info,"No SDL video info, use default\n" );
        native_bitpp = 8;
        native_bytepp = 1;
    }
#endif   
   
    if( VID_SetMode( initialmode ) <= 0 )
       goto abort_error;

    SDL_ShowCursor(SDL_DISABLE);
    I_StartupMouse( false );
//    I_UngrabMouse();

    graphics_state = VGS_active;
    return;

abort_error:
    // cannot return without a display screen
    I_Error("StartupGraphics Abort\n");
}


// Called to start rendering graphic screen according to the request switches.
// Fullscreen modes are possible.
// 
// param: req_drawmode, req_bitpp, req_alt_bitpp, req_width, req_height.
// Set modelist to NULL when fail.
// Returns FAIL_select, FAIL_end, FAIL_create, of status_return_e, 1 on success;
int I_RequestFullGraphics( byte select_fullscreen )
{
    modenum_t initialmode;
    byte  select_bitpp, select_bytepp;
    byte  select_fullscreen_mode;
    int  ret_value = 0;

    vid.draw_ready = 0;  // disable print reaching console

    // Get video info for screen resolutions.

    switch(req_drawmode)
    {
     case DRM_explicit_bpp:
       select_bitpp = req_bitpp;
       select_bytepp = (select_bitpp + 7) >> 3;
       break;
     case DRM_native:
       if( ! V_CanDraw( native_bitpp ) )
       {
           // Use 8 bit and let SDL do the palette lookup.
           GenPrintf( EMSG_info,"Native %i bpp rejected\n", native_bitpp );
           goto draw_8pal;
       }
       select_bitpp = native_bitpp;
       select_bytepp = native_bytepp;
       goto get_modelist;
     case DRM_opengl:
       // SDL does drawing
       select_bitpp = native_bitpp;
       select_bytepp = native_bytepp;
       goto get_modelist;
     default:
       goto draw_8pal;
    }

#ifdef TESTBPP
    // [WDJ] Detect testbpp flag
    // Requested bpp will succeed, driver will convert drawn screen to native bpp.
    testbpp = 0;
    if( M_CheckParm( "-testbpp" ))
    {
        if( select_bitpp == 8 )
           I_Error( "Invalid for SDL port driver: -bpp 8 -testbpp" );
        testbpp = select_bitpp;
        select_bitpp = 32;  // native mode
    }
#endif

get_modelist:
    VID_make_fullscreen_modelist( select_bitpp );
    if( num_vid_mode )
        goto found_modes;

    if(req_drawmode == DRM_explicit_bpp && select_bitpp > 8)
    {
         GenPrintf( EMSG_info,"No %i bpp modes\n", req_bitpp );
         goto no_modes;
    }

draw_8pal:
    // Let SDL handle 8 bit mode, using conversion.
    if( verbose )
        GenPrintf( EMSG_info,"Draw 8bpp using palette, SDL must convert to %i bpp video modes\n", native_bitpp );

    select_bytepp = 1;
    VID_make_fullscreen_modelist( 1 );  // request_NULL or native
    if( num_vid_mode == 0 )
    {
        // should not happen with fullscreen modes
        GenPrintf( EMSG_error, "No usable fullscreen video modes.\n");
        goto no_modes;
    }

found_modes:
    // Have some requested video modes in modelist
    vid.bitpp = modelist_bitpp;
    vid.bytepp = select_bytepp;
    // Mode List has been prepared

    if( verbose )
       GenPrintf( EMSG_ver, "\nFound %d Video Modes at %i bpp\n", num_vid_mode, vid.bitpp);

    vid.width = req_width;
    vid.height = req_height;

    // Need the modenum, even for opengl.
    select_fullscreen_mode = vid_mode_table[select_fullscreen];
    initialmode = VID_GetModeForSize( req_width, req_height, select_fullscreen_mode );

// [WDJ] To be safe, make it conditional
#ifdef MAC_SDL
    //[segabor]: Mac hack
//    if( (cv_drawmode.EV == DRM_opengl) || rendermode == render_opengl ) 
    if( rendermode == render_opengl )
#else     
    if( rendermode == render_opengl )
#endif  
    {
       // keep voodoo fixes from messing with the initial window size
       if( detect_voodoo() )
       {
           vid.width = 640; // hack to make voodoo cards work in 640x480
           vid.height = 480;
       }
       vid.fullscreen = select_fullscreen;
       vid.widthbytes = vid.width * vid.bytepp;

       if( verbose>1 )
          GenPrintf( EMSG_ver, "OglSdl_SetMode(%i,%i,%i)\n", req_width, req_height, select_fullscreen_mode);
       if( ! OglSdl_SetMode(req_width, req_height, select_fullscreen) )
       {	 
          return FAIL_create;
       }
    }

    if( rendermode == render_soft )
    {
        ret_value = VID_SetMode( initialmode );
        if( ret_value < 0 )
            return ret_value;

#ifdef SDL2
        if( sdl_window == NULL )
#else
        if(vidSurface == NULL)
#endif
        {
            GenPrintf( EMSG_error,"Could not set vidmode\n");
            return FAIL_create;
        }
    }

    vid.modenum = initialmode;

    I_StartupMouse( false );

    graphics_state = VGS_fullactive;

#if defined(MAC_SDL) && defined( DEBUG_MAC )
    SDL_Delay( 4 * 1000 );  // [WDJ] DEBUG: to see if errors are due to startup or activity
#endif
    return ret_value;  // have video mode

no_modes:
    return FAIL_select;
}


void I_ShutdownGraphics( void )
{
    // was graphics initialized anyway?
    if( graphics_state <= VGS_shutdown )
        return;

    graphics_state = VGS_shutdown;  // to catch some repeats due to errors

    if( rendermode == render_soft )
    {
        VID_SDL_release();       
    }
    else
    {
        OglSdl_Shutdown();
    }
    graphics_state = VGS_off;

#if defined(MAC_SDL) && defined( DEBUG_MAC )
    GenPrintf( EMSG_info,"SDL_Quit()\n");  // [WDJ] DEBUG:
#endif
    SDL_Quit();
}
