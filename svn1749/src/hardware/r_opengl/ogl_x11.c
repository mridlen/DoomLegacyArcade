// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: ogl_x11.c 1245 2016-08-04 14:21:00Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: ogl_x11.c,v $
// Revision 1.11  2001/03/09 21:53:56  metzgermeister
//
// Revision 1.10  2001/02/19 17:43:38  hurdler
// Fix the problem of fullbright with Matrox's drivers under Linux
//
// Revision 1.9  2001/02/14 20:59:27  hurdler
// fix texture bug under Linux
//
// Revision 1.8  2001/02/13 20:37:27  metzgermeister
// Revision 1.7  2000/08/11 16:32:29  metzgermeister
//
// Revision 1.6  2000/05/13 19:54:54  metzgermeister
// no tex flush on setmode
//
// Revision 1.5  2000/04/12 19:32:29  metzgermeister
// added GetRenderer function
//
// Revision 1.4  2000/04/07 23:10:15  metzgermeister
// fullscreen support under X in Linux
//
// Revision 1.3  2000/03/07 03:31:14  hurdler
// fix linux compilation
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      X11 specific part of the OpenGL API for Doom Legacy (uses GLX)
//
//-----------------------------------------------------------------------------

#ifndef SMIF_X11
// Linux X11 or BSD X11 only.
# error ogl_x11 is X11 only
#endif

#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <GL/glx.h>

#include "r_opengl.h"


// **************************************************************************
//                                                                    GLOBALS
// **************************************************************************

// [WDJ] These vars are only accessible by dlsym function calls of the dll.
// Other accesses will create separate local copies.
// Direct calls of the dll functions will create separate local copies.
// Attempts to access global vars of the main program will fail at dlopen.
static GLXContext ctx   = NULL;
static Display *dpy     = NULL;
static Window win       = 0; // metzgermeister: No pointer!
static XVisualInfo *vis = NULL;

#if 0
// [WDJ] Unused
#define MAX_VIDEO_MODES   32
static  vmode_t     video_modes[MAX_VIDEO_MODES];
#endif

// **************************************************************************
//                                                                  FUNCTIONS
// **************************************************************************

//
// FAB --- SORRY, THIS SHOULD BE UPDATED LIKE ABOVE, PLUS THE LINUX ADDS
//
// [WDJ] Must use the same HWRAPI interface, and call, or else the static
// global variables set in this function, are not the same variables used in
// the other functions of this dll.  There are no compile errors.
// Called on any createWindow
EXPORT Window HWRAPI( HookXwin ) (Display *dsp,int width,int height, boolean vidmode_active)
{
    int scrnum;
    int attrib[] = { GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 16, /* bug? 19990908 by Kin */
        None };
    unsigned long mask;
    Window root;
    XSetWindowAttributes attr;

    DBG_Printf ("HookXwin()\n");

    if (ctx != NULL) {        // si ce n'est pas la premiere fois qu'on
	// this flush destroys textures with the UTAH DRI driver !?
        //VIDGL_Flush_GL_textures(); // Flush here, otherwise textures will be trashed after resolution change
        //glXMakeCurrent(NULL, NULL); // initialise l'environnement OpenGL, il
        glXDestroyContext(dpy,ctx);// faut d'abord supprimer l'ancien
        ctx = NULL; 
        // not very clean; use a function UnhookXwin instead?
        XDestroyWindow(dsp, win);
        win = 0;
    }

    dpy = dsp;
    scrnum = DefaultScreen( dsp );
    root = RootWindow( dsp, scrnum );
    vis = glXChooseVisual(dsp,scrnum,attrib);
    if (!vis) {
        return 0;
    }

    /* window attributes */
    if (vidmode_active) {
        mask = CWColormap | CWSaveUnder | CWBackingStore | 
            CWEventMask | CWOverrideRedirect;
        
        attr.override_redirect = True;
        attr.backing_store = NotUseful;
        attr.save_under = False;
    }
    else {
        mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
        
        attr.background_pixel = 0;
        attr.border_pixel = 0;
    }

    // is this acceptable by GLX? 19991226 by Kin
    attr.colormap = XCreateColormap( dsp, root, vis->visual, AllocNone);
    attr.event_mask = KeyPressMask | KeyReleaseMask
#ifndef POLL_POINTER
        | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
#endif
        | ExposureMask | StructureNotifyMask;

    win = XCreateWindow(dsp, 
                        root, 
                        0, 0, 
                        width, height,
                        0, 
                        vis->depth, 
                        InputOutput,
                        vis->visual, 
                        mask, 
                        &attr);
    XMapWindow(dsp, win);
    //VIDGL_Setup_GL_PixelFormat();
    if ((ctx=glXCreateContext(dpy,vis,NULL,True))==NULL) {
        DBG_Printf("glXCreateContext() FAILED\n");
        return 0;
    }
    if (!glXMakeCurrent(dpy, win, ctx)) {
        DBG_Printf("glXMakeCurrent() FAILED\n");
        return 0;
    }

    VIDGL_Query_GL_info( GLF_NOTEXENV ); // Linux specific test

    screen_depth = vis->depth;
    if( screen_depth > 16)
        textureformatGL = GL_RGBA;
    else
        textureformatGL = GL_RGB5_A1;

    VIDGL_Set_GL_Model_View( width, height );
    VIDGL_Set_GL_States();

    // we need to clear the depth buffer. Very important!!!
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    //VIDGL_Flush_GL_textures(); // peut être qu'il faut mettre çà dans resize ou create

    //lvid->buffer = NULL;   // unless we use the software view
    //lvid->direct = NULL;   // direct access to video memory, old DOS crap
    //lvid->numpages = 2;    // this is normally not used

    return win; // on renvoie une valeur pour dire que cela s'est bien pass
}


// -----------------+
// Shutdown         : Shutdown OpenGL, restore the display mode
// -----------------+
EXPORT void HWRAPI( Shutdown ) ( void )
{
    DBG_Printf ("HWRAPI Shutdown()\n");

    if(ctx != NULL) {
       VIDGL_Flush_GL_textures();
       //glXMakeCurrent(NULL,0,0);
       glXDestroyContext(dpy,ctx);
    }
    DBG_Printf ("HWRAPI Shutdown(DONE)\n");
    DBG_close(); // shutdown log
}


// -----------------+
// FinishUpdate     : Swap front and back buffers
// -----------------+
EXPORT void HWRAPI( FinishUpdate ) (int waitvbl)
{
    // DBG_Printf ("FinishUpdate()\n");
    // TODO: implement waitvbl
    glXSwapBuffers(dpy,win);
}


// -----------------+
// SetPalette       : Set the color lookup table for paletted textures
//                  : in OpenGL, we store values for conversion of paletted graphics when
//                  : they are downloaded to the 3D card.
// -----------------+
EXPORT void HWRAPI( SetPalette ) ( RGBA_t *pal, RGBA_t *gamma )
{
    int i;
    //DBG_Printf ("SetPalette()\n");

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((pal[i].s.red*gamma->s.red)/127,     255);
        myPaletteData[i].s.green = MIN((pal[i].s.green*gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((pal[i].s.blue*gamma->s.blue)/127,   255);
        myPaletteData[i].s.alpha = pal[i].s.alpha;
    }
    // on a changé de palette, il faut recharger toutes les textures
    VIDGL_Flush_GL_textures();
}
