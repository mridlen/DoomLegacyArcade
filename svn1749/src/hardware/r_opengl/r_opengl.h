// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_opengl.h 1245 2016-08-04 14:21:00Z wesleyjohnson $
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
// $Log: r_opengl.h,v $
// Revision 1.16  2001/03/09 21:53:56  metzgermeister
//
// Revision 1.15  2001/02/19 17:45:20  hurdler
// Fix the problem of fullbright with Matrox's drivers under Linux
//
// Revision 1.14  2000/11/02 19:49:40  bpereira
//
// Revision 1.13  2000/10/22 14:17:17  hurdler
// Adjust version string
//
// Revision 1.12  2000/09/25 19:29:24  hurdler
// Maintenance modifications
//
// Revision 1.11  2000/08/11 12:28:08  hurdler
// latest changes for v1.30
//
// Revision 1.10  2000/08/10 19:58:05  bpereira
// Revision 1.9  2000/08/03 17:57:42  bpereira
// Revision 1.8  2000/05/09 21:10:04  hurdler
//
// Revision 1.7  2000/04/18 12:45:09  hurdler
// change a little coronas' code
//
// Revision 1.6  2000/04/07 23:10:15  metzgermeister
// fullscreen support under X in Linux
//
// Revision 1.5  2000/03/07 03:31:14  hurdler
// fix linux compilation
//
// Revision 1.4  2000/03/06 15:29:32  hurdler
// Revision 1.3  2000/02/27 16:37:14  hurdler
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      
//
//-----------------------------------------------------------------------------

#ifndef R_OPENGL_H
#define R_OPENGL_H

// Because of redefine WINVER, doomtype.h (via doomdef.h) needs to be before
// any other possible WINVER users, like gl and glu
#include "doomdef.h"

//[segabor]
#ifdef __MACH__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// necessary for Unix AND Windows (param to hw_drv)
#define  HWRAPI_CREATE_DLL
#include "hardware/hw_drv.h"

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#define MIN(x,y) ( ((x)<(y)) ? (x) : (y) )
#define MAX(x,y) ( ((x)>(y)) ? (x) : (y) )

// maybe defined in previous *.h
#undef DEBUG_TO_FILE
// output debugging msgs to ogllog.txt
#define DEBUG_TO_FILE   "ogllog.txt"

#ifndef MINI_GL_COMPATIBILITY
//    #define USE_PALETTED_TEXTURE
#define DRIVER_STRING "HWRAPI Init(): DooM Legacy OpenGL renderer"
#endif

#ifndef WIN32
typedef unsigned int    DWORD;
typedef char*           LPCTSTR;
typedef int             HANDLE;
#endif

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

void DBG_Printf(LPCTSTR lpFmt, ...);                                          
// [WDJ] Print a long string as multiple lines of LOGLINELEN
void DBG_Print_lines( const char * longstr );
void DBG_close( void );

// r_opengl functions
int  VIDGL_isExtAvailable(char *extension);
void VIDGL_Set_GL_Model_View(GLint w, GLint h);
void VIDGL_Set_GL_States(void);
void VIDGL_Flush_GL_textures(void);

// [WDJ] Query the GL hardware strings
// sets oglflags and gl_extensions
// Do not call before initializing GL
void VIDGL_Query_GL_info( int ogltest );

#ifdef WIN32
// Win32 functions to manipulate window attributes
// Only implemented in ogl_win.c
int  VIDGL_SetVidMode(viddef_t *lvid, vmode_t *pcurrentmode);
void VIDGL_UnSetVidMode(void);
boolean VIDGL_SetupPixelFormat(int WantColorBits, int WantStencilBits, int WantDepthBits);
#endif

// ==========================================================================
//                                                                     GLOBAL
// ==========================================================================

extern const GLubyte    *gl_extensions;
extern RGBA_t           myPaletteData[];
extern GLint            screen_width;
extern GLint            screen_height;
extern GLbyte           screen_depth;
extern int              oglflags;
extern GLint            textureformatGL;

typedef enum {
    GLF_NOZBUFREAD = 0x01,
    GLF_NOTEXENV   = 0x02,
} oglflags_t;

#endif
