// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ogl_sdl.c 1624 2022-04-03 22:06:03Z wesleyjohnson $
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
// $Log: ogl_sdl.c,v $
// Revision 1.6  2001/06/25 20:08:06  bock
// Fix bug (BSD?) with color depth > 16 bpp
//
// Revision 1.5  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.4  2001/03/09 21:53:56  metzgermeister
// Revision 1.3  2000/11/02 19:49:40  bpereira
// Revision 1.2  2000/09/10 10:56:01  metzgermeister
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
//
// DESCRIPTION:
//      SDL specific part of the OpenGL API for Doom Legacy
//
//-----------------------------------------------------------------------------

// Debugging unfinished MAC_SDL
//#define DEBUG_MAC  1
//#define DEBUG_SDL
//#define DEBUG_WIN

// Because of WINVER redefine, before any include that could define WINVER
#include "doomincl.h"

#include <SDL.h>
// Will use gl.h, so block SDL glext redefine.
#define NO_SDL_GLEXT
#include <SDL_opengl.h>

#include "hardware/r_opengl/r_opengl.h"
  // OpenGL, gl.h, glu.h
//#include "v_video.h"


#ifdef MAC_SDL

// As per an apple demo program, OpenGL is a framework
# include <OpenGL/OpenGL.h>
  // maybe OpenGL brings in the two below ??
//# include <OpenGL/CGLCurrent.h>
//# include <OpenGL/CGLTypes.h>
// CGLCurrent.h and CGLTypes.h are in
//    /System/Library/Frameworks/OpenGL.framework/Versions/A/Header

GLint  majorver, minorver;
GLint  numscreens;
// CGL (Core OpenGL)
// It is low-level interface to setup portion of Apple OpenGL implementation.
// Can query the hardware and prepare a drawing context for OpenGL.
// Does similar interfacing for Mac, as GLX does for X11.
// NSOpenGL (higher level class interface to OpenGL)
CGLError  cglerr;
CGLContextObj  cglcon;
CGLPixelFormatObj  cglpix;   // ptr, reference counted, release needed
CGLPixelFormatAttribute  cglattrib[12] = { kCGLPFADoubleBuffer, 0 };
int numattrib = 1;
byte created_context = 0;


void  mac_cgl_error( char * str, int cglerr )
{
   if( cglerr )
   {
      const char * errstr = CGLErrorString ( cglerr );
      GenPrintf( EMSG_error,"%s has CGL error: %s\n", str, errstr );
   }
}

void  mac_report_context_var( char * str, GLint varid, int num )
{
    GLint paar[8];

    cglerr = CGLGetParameter ( cglcon, varid, paar );
    if( cglerr )
      mac_cgl_error( "GetParameter", cglerr );
    else
    {
      if( num == 1 )
	 GenPrintf( EMSG_info, "  %s = %i\n", paar[0] );
      else
	 GenPrintf( EMSG_info, "  %s = %i,%i,%i,%i\n", paar[0], paar[1], paar[2], paar[3] );
    }
}


void mac_init( void )
{
   CGLGetVersion ( & majorver, & minorver );
#ifdef DEBUG_MAC   
   GenPrintf( EMSG_info, "Found CGL Version  %i.%i\n", majorver, minorver );
#endif
}

#ifdef DEBUG_MAC   
void mac_check_context( char * str )
{
   cglcon = CGLGetCurrentContext();
   if( cglcon )
   {
      GenPrintf( EMSG_debug, "%s: CGL reports existing context %p\n", str, cglcon );
   }
   else
   {
      GenPrintf( EMSG_debug, "%s: CGL reports no current context\n", str);
   }
}
#endif
   
   
void  mac_set_context( void )
{
   cglcon = CGLGetCurrentContext();
   if( cglcon == NULL )
   {
      if( vid.fullscreen )
      {
	 cglattrib[numattrib++] = kCGLPFAFullScreen;
      }
      cglattrib[numattrib++] = 0;
      
      cglerr = CGLChoosePixelFormat ( &cglattrib,  & cglpix, & numscreens );
      mac_cgl_error( "Create CGL pixel format", cglerr );
      
      cglerr = CGLCreateContext ( cglpix, NULL, &cglcon );
      mac_cgl_error( "Create CGL context", cglerr );
      
      GenPrintf( EMSG_info, "Created CGL context %p\n", cglcon );
      
      created_context = 1;
   }
   
   if( cglcon )
   {
      cglerr = CGLSetCurrentContext ( cglcon );
      mac_cgl_error( "Set CGL Context", cglerr );
      GenPrintf( EMSG_info," GL_RENDERER = %s\n", glGetString(GL_RENDERER) );
      GenPrintf( EMSG_info," GL_VENDOR = %s\n", glGetString(GL_VENDOR) );
      GenPrintf( EMSG_info," GL_VERSION = %s\n", glGetString(GL_VERSION) );

// [WDJ] Defined guard added. This is how OS_X VERSION is tested on a mac.  As set by AvailabilityMacros.h
#if defined( MAC_OS_X_VERSION_MAX_ALLOWED ) && (MAC_OS_X_VERSION_MAX_ALLOWED < 101000)
      // Carbon Khronos Group calls.
      // Gibbon reports this is only valid for OS X < 10.10.
      // Context parameters are available for OS X 10.0 and later.
      GenPrintf( EMSG_info, "CGL Context values\n" );
      mac_report_context_var( "SwapRectangle", kCGLCPSwapRectangle, 4 );
      mac_report_context_var( "SwapInterval", kCGLCPSwapInterval, 1 );
      mac_report_context_var( "SurfaceBackingSize", kCGLCPSurfaceBackingSize, 1 );
      mac_report_context_var( "SurfaceSurfaceVolatile", kCGLCPSurfaceSurfaceVolatile, 1 );
      mac_report_context_var( "HasDrawable", kCGLCPHasDrawable, 1 );
      mac_report_context_var( "CurrentRendererID", kCGLCPCurrentRendererID, 1 );
#endif
   }
   
   // Create context retains the pixel format, so can release it
   CGLReleasePixelFormat( cglpix );
}

void mac_close_context( void )
{
   cglerr = CGLSetCurrentContext( NULL );
   mac_cgl_error( "Close CGL context", cglerr );
   if( created_context )
     CGLReleaseContext( cglcon );
}

#endif

// public
#ifdef SDL2
extern SDL_Window * sdl_window;
SDL_GLContext sdl_gl_context = NULL;  // where OpenGL draws
#else
// Only one vidSurface, else releasing oldest faults in SDL.
extern SDL_Surface * vidSurface;
#endif
byte  ogl_active = 0;

// indexed by fullscreen
extern const char * fullscreen_str[2];

#ifdef SDL2
// indexed by fullscreen
const static uint32_t sdl_ogl_window_flags[2] = {
  SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_SHOWN,   // windowed
  SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED,  // fullscreen
};
#else
// SDL 1.2
// These flags do not affect the GL attributes, only the 2d blitting.
// indexed by fullscreen
Uint32 sdl_ogl_surface_flags[2] = {
#ifdef MAC_SDL
  // Mac, Edge   
  SDL_OPENGL|SDL_DOUBLEBUF,   // windowed
  SDL_OPENGL|SDL_DOUBLEBUF|SDL_FULLSCREEN,  // fullscreen
#else
  SDL_OPENGL,  // windowed
  SDL_OPENGL|SDL_FULLSCREEN,  // fullscreen
#endif
};
#endif   

// i_video.c
void VID_SDL_release( void );


// Called by VID_SetMode
// SDL-OpenGL version of VID_SetMode
boolean OglSdl_SetMode(int w, int h, byte req_fullscreen)
{
#ifdef SDL2
    // SDL 2
    // Release OpenGL specific.
    if( sdl_gl_context )
    {
        SDL_GL_DeleteContext( sdl_gl_context );
        sdl_gl_context = NULL;
    }

    VID_SDL_release();
#else
    // SDL 1.2
    int cbpp;  // bits per pixel

    if( vidSurface )
    {
        VID_SDL_release();
#ifdef VOODOOSAFESWITCHING
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
    }
#endif   

#ifdef SDL2
    uint32_t window_reqflags = sdl_ogl_window_flags[req_fullscreen];
#else
    // SDL 1.2
    // These flags do not affect the GL attributes, only the 2d blitting.
    Uint32 reqflags = sdl_ogl_surface_flags[req_fullscreen];
#endif   


#ifdef MAC_SDL   
#ifdef DEBUG_MAC
    GenPrintf( EMSG_debug, "Detect: "
# ifdef __MACOSX__
	    " __MACOSX__ "
# endif
# ifdef __MACOS__
	    " __MACOS__ "
# endif
# ifdef __LINUX__
	    " __LINUX__ "
# endif
	    "\n" );
#endif

    mac_init( );

#ifdef DEBUG_MAC
    mac_check_context( "OglSdlSurface 1" );
#endif

    mac_set_context();
//#define MAC_REINIT_AFTER_CONTEXT  1
#ifdef MAC_REINIT_AFTER_CONTEXT   
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
#endif

#if 1   
    // We want at least 4 bit R, G, and B, and at least 16 bpp.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);  // Mac, because Edge does it
#else
    // We want at least 1 bit R, G, and B, and at least 16 bpp.
    // Why 1 bit? May be more?
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
#endif

#ifdef SDL2
    SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "1" );  // enable
    // SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengl" );  // allow opengl
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );  // nearest, linear, best

    sdl_window = SDL_CreateWindow( "Doom Legacy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   req_width, req_height, window_reqflags );
    if( sdl_window == NULL)
        return false;  // Modes were prechecked, SDL should not fail.

    // SDL2 Wiki:
    // On the Apple OS X you must set the NSHighResolutionCapable Info.plist  property to YES,
    // otherwise you will not receive a High DPI OpenGL canvas.

    // Create OpenGL drawing connection to the window.
    // Do not need renderer, nor texture.
    sdl_gl_context = SDL_GL_CreateContext( sdl_window );
    if( sdl_gl_context == NULL)
        return false;

    SDL_DisplayMode sdl_displaymode;
    SDL_GetWindowDisplayMode( sdl_window, /*OUT*/ & sdl_displaymode );
   
    // Not used in OpenGL drawmode
    vid.bitpp = SDL_BITSPERPIXEL( sdl_displaymode.format );
    vid.bytepp = (vid.bitpp + 7) >> 3;
    vid.width = sdl_displaymode.w;
    vid.height = sdl_displaymode.h;
    vid.ybytes = vid.width * vid.bytepp;
   
    if( verbose )
    {
        GenPrintf( EMSG_ver,"  OpenGL Got %ix%i, %i bpp\n",
            vid.width, vid.height, vid.bitpp );
    }

#else
    // SDL 1.2
    cbpp = SDL_VideoModeOK(w, h, 16, reqflags);
    if (cbpp < 16)
        return false;

    if( verbose>1 )
    {
        GenPrintf( EMSG_ver,"OpenGL SDL_SetVideoMode(%i,%i,%i,0x%X)  %s\n",
		w, h, 16, reqflags, fullscreen_str[ req_fullscreen ] );
    }

    vidSurface = SDL_SetVideoMode(w, h, cbpp, reqflags);
    if(vidSurface == NULL)
        return false;

    vid.bitpp = vidSurface->format->BitsPerPixel;
    vid.bytepp = vidSurface->format->BytesPerPixel;
    vid.width = vidSurface->w;
    vid.height = vidSurface->h;
    vid.ybytes = vidSurface->pitch;

    if( verbose )
    {
        int32_t vflags = vidSurface->flags;
        GenPrintf( EMSG_ver,"  OpenGL Got %ix%i, %i bpp, %i byte\n",
		vid.width, vid.height, vid.bitpp, vid.bytepp );
        GenPrintf( EMSG_ver,"  HW-surface= %x, HW-palette= %x, HW-accel= %x, Doublebuf= %x, Async= %x \n",
		vflags&SDL_HWSURFACE, vflags&SDL_HWPALETTE, vflags&SDL_HWACCEL, vflags&SDL_DOUBLEBUF, vflags&SDL_ASYNCBLIT );
        if(SDL_MUSTLOCK(vidSurface))
	    GenPrintf( EMSG_ver,"  Notice: MUSTLOCK video surface\n" );
    }
   
#ifdef DEBUG_SDL
    GenPrintf( EMSG_debug, " vid set: height=%i, width=%i\n", vid.height, vid.width );
    if( vidSurface->pitch != (vid.width * vid.bytepp))
    {
        GenPrintf( EMSG_debug," Notice: Unusual buffer width = %i, where width x bytes = %i\n",
		vidSurface->pitch, (vid.width * vid.bytepp) );
    }
#endif
#endif

    vid.recalc = true;
    ogl_active = 1;

#ifdef MAC_SDL   
#ifdef DEBUG_MAC
    mac_check_context( "OglSdlSurface gl call" );
#endif
    mac_set_context();
#endif

#ifdef DEBUG_SDL     
    GenPrintf( EMSG_debug, " glClear: height=%i, width=%i\n", h, w );
#endif

#ifdef MAC_SDL
    glClearColor(0.0,0.0,0.0,0.0);
#endif
#ifdef DEBUG_WIN
    V_SetPalette(0);
    glClearColor(0.0, 0.0, 0.0, 0.0);
#endif

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

#ifdef DEBUG_SDL
    GenPrintf( EMSG_debug, " SDL_GL_SwapBuffers: height=%i, width=%i\n", h, w );
#endif

#ifdef MAC_SDL   
    // [WDJ] SDL_GL_SwapBuffers is required here to prevent crashes on Mac.
    // Do not know why.  (From Edge)
    SDL_GL_SwapBuffers();
#endif
#ifdef DEBUG_WIN
    SDL_GL_SwapBuffers();
#endif

#ifdef DEBUG_SDL     
    GenPrintf( EMSG_debug, " VIDGL_Set_GL_Model_View: height=%i, width=%i\n", h, w );
#endif

    // Moved these after, from Edge, which does not crash on Mac
    VIDGL_Set_GL_Model_View(vid.width, vid.height);
    VIDGL_Set_GL_States();

#ifdef SDL2
    textureformatGL = (vid.bitpp > 16)?GL_RGBA:GL_RGB5_A1;
#else
    textureformatGL = (cbpp > 16)?GL_RGBA:GL_RGB5_A1;
#endif

#if 1
    VIDGL_Query_GL_info( -1 ); // all tests
#endif
    return true;
}

void OglSdl_FinishUpdate(void)
{
#ifdef SDL2
    SDL_GL_SwapWindow( sdl_window );
#else
    // SDL 1.2
    SDL_GL_SwapBuffers();
#endif
}

void OglSdl_Shutdown(void)
{
    ogl_active = 0;

    // Release OpenGL specific.
#ifdef SDL2
    if( sdl_gl_context )
    {
        SDL_GL_DeleteContext( sdl_gl_context );
        sdl_gl_context = NULL;
    }
#endif   

    VID_SDL_release();

#ifdef MAC_SDL   
#ifdef DEBUG_MAC
    mac_check_context( "OglSdl_Shutdown" );
#endif
    mac_close_context();
#endif
}

//  hwSym SetPalette
void OglSdl_SetPalette(RGBA_t *palette, RGBA_t *gamma)
{
    int i;

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((palette[i].s.red   * gamma->s.red)  /127, 255);
        myPaletteData[i].s.green = MIN((palette[i].s.green * gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((palette[i].s.blue  * gamma->s.blue) /127, 255);
        myPaletteData[i].s.alpha = palette[i].s.alpha;
    }
    // on a changé de palette, il faut recharger toutes les textures
    // jaja, und noch viel mehr ;-)
    VIDGL_Flush_GL_textures();
}
