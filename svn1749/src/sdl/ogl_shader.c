// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------
// [Arcade] CRT post-process shaders for the OpenGL drawmode.
//
// Once a frame is finished, and before the swap, the back buffer is copied
// into a texture, box filtered down to roughly Doom's own 200 lines, and
// drawn back over the whole screen through one of the libretro CRT shaders
// in sdl/crt/.  Those shaders treat each texel of their input as one line of
// the tube, so the downsample is what makes the scanlines the size of the
// game's pixels rather than of the monitor's.
//
// The renderer is fixed-function and caches its GL state (SetBlend's
// cur_polyflags), so everything here is bracketed by glPushAttrib and
// glPopAttrib and leaves no trace.  See docs/arcade/crt-shaders.md.
//-----------------------------------------------------------------------------

#include "doomincl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL.h>
// Will use gl.h, so block SDL glext redefine.
#define NO_SDL_GLEXT
#include <SDL_opengl.h>

#include "hardware/r_opengl/r_opengl.h"
#include "hardware/hw_main.h"
  // cv_grshader, gr_shader_status
#include "screen.h"
  // vid
#include "ogl_shader.h"

#ifndef APIENTRY
#define APIENTRY
#endif

// GL 2.0 and framebuffer object values, which a GL 1.1 gl.h does not have.
#define CRT_FRAGMENT_SHADER        0x8B30
#define CRT_VERTEX_SHADER          0x8B31
#define CRT_COMPILE_STATUS         0x8B81
#define CRT_LINK_STATUS            0x8B82
#define CRT_INFO_LOG_LENGTH        0x8B84
#define CRT_SHADING_LANGUAGE_VERSION  0x8B8C
#define CRT_FRAMEBUFFER            0x8D40
#define CRT_COLOR_ATTACHMENT0      0x8CE0
#define CRT_FRAMEBUFFER_COMPLETE   0x8CD5
#define CRT_CLAMP_TO_EDGE          0x812F

typedef struct {
    const char * name;
    const char * src;
    byte  linear;   // sample the input with GL_LINEAR, as the RetroArch preset does
} crt_shader_def_t;

#include "ogl_crt_glsl.h"

#define NUM_CRT_SHADERS  ((int)(sizeof(crt_shader_defs)/sizeof(crt_shader_defs[0])))

// The downsample factor is a whole number, so every line of the tube covers
// the same number of screen rows; a fractional one beats against the pixel
// grid and the scanlines come out uneven.  This caps its cost at 4K.
#define CRT_MAX_FACTOR  10

// Texture names.  The renderer does NOT use glGenTextures for its own: it
// counts up from no_texture_id (next_texture_id in r_opengl.c) and binds
// whatever number comes next, so a name glGenTextures hands out here is soon
// reused for a patch or a flat, whose upload replaces the frame copy.  That
// showed the title screen upside down and cropped.  These are far above
// anything the counter reaches, and binding an unused name creates it.
#define CRT_TEX_SRC  0x7FFFFF00u
#define CRT_TEX_LOW  0x7FFFFF01u


// Looked up at runtime: opengl32.dll exports GL 1.1 and nothing later.
static GLuint (APIENTRY * p_CreateShader)( GLenum );
static void   (APIENTRY * p_ShaderSource)( GLuint, GLsizei, const char * const *, const GLint * );
static void   (APIENTRY * p_CompileShader)( GLuint );
static void   (APIENTRY * p_GetShaderiv)( GLuint, GLenum, GLint * );
static void   (APIENTRY * p_GetShaderInfoLog)( GLuint, GLsizei, GLsizei *, char * );
static void   (APIENTRY * p_DeleteShader)( GLuint );
static GLuint (APIENTRY * p_CreateProgram)( void );
static void   (APIENTRY * p_AttachShader)( GLuint, GLuint );
static void   (APIENTRY * p_BindAttribLocation)( GLuint, GLuint, const char * );
static void   (APIENTRY * p_LinkProgram)( GLuint );
static void   (APIENTRY * p_GetProgramiv)( GLuint, GLenum, GLint * );
static void   (APIENTRY * p_GetProgramInfoLog)( GLuint, GLsizei, GLsizei *, char * );
static void   (APIENTRY * p_DeleteProgram)( GLuint );
static void   (APIENTRY * p_UseProgram)( GLuint );
static GLint  (APIENTRY * p_GetUniformLocation)( GLuint, const char * );
static void   (APIENTRY * p_Uniform1i)( GLint, GLint );
static void   (APIENTRY * p_Uniform1f)( GLint, GLfloat );
static void   (APIENTRY * p_Uniform2f)( GLint, GLfloat, GLfloat );
static void   (APIENTRY * p_UniformMatrix4fv)( GLint, GLsizei, GLboolean, const GLfloat * );
static void   (APIENTRY * p_VertexAttribPointer)( GLuint, GLint, GLenum, GLboolean, GLsizei, const void * );
static void   (APIENTRY * p_EnableVertexAttribArray)( GLuint );
static void   (APIENTRY * p_DisableVertexAttribArray)( GLuint );
static void   (APIENTRY * p_VertexAttrib4f)( GLuint, GLfloat, GLfloat, GLfloat, GLfloat );
// Framebuffer objects, optional: without them the shader samples the full
// size frame directly, which still gives the right number of scanlines.
static void   (APIENTRY * p_GenFramebuffers)( GLsizei, GLuint * );
static void   (APIENTRY * p_BindFramebuffer)( GLenum, GLuint );
static void   (APIENTRY * p_FramebufferTexture2D)( GLenum, GLenum, GLenum, GLuint, GLint );
static GLenum (APIENTRY * p_CheckFramebufferStatus)( GLenum );

typedef struct {
    void ** fp;
    const char * name;
    const char * alt;   // extension name, same entry point
    byte  optional;
} crt_proc_t;

#define PROC(f, alt, opt)  { (void **) &p_##f, "gl" #f, alt, opt }
static const crt_proc_t crt_procs[] = {
    PROC( CreateShader, NULL, 0 ),
    PROC( ShaderSource, NULL, 0 ),
    PROC( CompileShader, NULL, 0 ),
    PROC( GetShaderiv, NULL, 0 ),
    PROC( GetShaderInfoLog, NULL, 0 ),
    PROC( DeleteShader, NULL, 0 ),
    PROC( CreateProgram, NULL, 0 ),
    PROC( AttachShader, NULL, 0 ),
    PROC( BindAttribLocation, NULL, 0 ),
    PROC( LinkProgram, NULL, 0 ),
    PROC( GetProgramiv, NULL, 0 ),
    PROC( GetProgramInfoLog, NULL, 0 ),
    PROC( DeleteProgram, NULL, 0 ),
    PROC( UseProgram, NULL, 0 ),
    PROC( GetUniformLocation, NULL, 0 ),
    PROC( Uniform1i, NULL, 0 ),
    PROC( Uniform1f, NULL, 0 ),
    PROC( Uniform2f, NULL, 0 ),
    PROC( UniformMatrix4fv, NULL, 0 ),
    PROC( VertexAttribPointer, NULL, 0 ),
    PROC( EnableVertexAttribArray, NULL, 0 ),
    PROC( DisableVertexAttribArray, NULL, 0 ),
    PROC( VertexAttrib4f, NULL, 0 ),
    PROC( GenFramebuffers, "glGenFramebuffersEXT", 1 ),
    PROC( BindFramebuffer, "glBindFramebufferEXT", 1 ),
    PROC( FramebufferTexture2D, "glFramebufferTexture2DEXT", 1 ),
    PROC( CheckFramebufferStatus, "glCheckFramebufferStatusEXT", 1 ),
};
#undef PROC

// Everything below dies with the GL context; OGL_Shader_Context_Lost forgets it.
static byte   crt_init_state;     // 0 not tried, 1 ready, 2 cannot run shaders
static byte   crt_have_fbo;
static GLuint crt_prog[16];
static byte   crt_prog_state[16]; // 0 not tried, 1 built, 2 failed
static GLuint box_prog;
static int    box_factor;         // box_prog is built for this factor
static GLuint src_tex, low_tex, low_fbo;
static int    src_w, src_h, low_w, low_h;
static int    crt_frame_count;
// src_tex holds the unfiltered frame that is on screen now, for the wipe.
static byte   crt_frame_saved;


void OGL_Shader_Context_Lost( void )
{
    crt_init_state = 0;
    crt_have_fbo = 0;
    memset( crt_prog, 0, sizeof(crt_prog) );
    memset( crt_prog_state, 0, sizeof(crt_prog_state) );
    box_prog = 0;
    box_factor = 0;
    src_tex = low_tex = low_fbo = 0;
    src_w = src_h = low_w = low_h = 0;
    crt_frame_saved = 0;
}


// The screen wipe captures its outgoing frame from the front buffer, which
// with a shader running holds the filtered picture; the wipe would then run
// it through the shader a second time.  Hand it the unfiltered copy instead.
// Fills image bottom-up, as glReadPixels would.  See ReadScreenRect.
static int crt_read_front( int x, int y, int width, int height, byte * image )
{
    const size_t rowbytes = (size_t)src_w * 3;
    byte * whole;
    int i;

    if( ! crt_frame_saved || ! src_tex )
        return 0;
    if( x < 0 || y < 0 || x + width > src_w || y + height > src_h )
        return 0;

    whole = (byte *) malloc( rowbytes * (size_t)src_h );
    if( ! whole )
        return 0;

    glPushAttrib( GL_TEXTURE_BIT );
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glBindTexture( GL_TEXTURE_2D, src_tex );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, whole );
    glPopClientAttrib();
    glPopAttrib();

    for( i = 0; i < height; i++ )
        memcpy( &image[ (size_t)i * width * 3 ],
                &whole[ (size_t)(y + i) * rowbytes + (size_t)x * 3 ],
                (size_t)width * 3 );

    free( whole );
    return 1;
}


static boolean crt_init( void )
{
    const char * glsl;
    int i;

    if( crt_init_state )
        return ( crt_init_state == 1 );

    crt_init_state = 2;
    gr_shader_status = 1;

    glsl = (const char *) glGetString( CRT_SHADING_LANGUAGE_VERSION );
    glGetError();  // a GL 1.x driver flags the enum; do not leave that behind
    if( ! glsl )
    {
        GenPrintf( EMSG_warn, "CRT shader: this OpenGL driver has no shaders (GL %s)\n",
                   glGetString( GL_VERSION ) );
        return false;
    }

    for( i = 0; i < (int)(sizeof(crt_procs)/sizeof(crt_procs[0])); i++ )
    {
        const crt_proc_t * p = &crt_procs[i];
        void * f = SDL_GL_GetProcAddress( p->name );
        if( ! f && p->alt )
            f = SDL_GL_GetProcAddress( p->alt );
        *p->fp = f;
        if( ! f && ! p->optional )
        {
            GenPrintf( EMSG_warn, "CRT shader: OpenGL driver lacks %s\n", p->name );
            return false;
        }
    }
    crt_have_fbo = p_GenFramebuffers && p_BindFramebuffer
                && p_FramebufferTexture2D && p_CheckFramebufferStatus;

    GenPrintf( EMSG_info, "CRT shader: GLSL %s%s\n", glsl,
               crt_have_fbo ? "" : ", no framebuffer objects" );

    ogl_read_front_hook = crt_read_front;
    crt_init_state = 1;
    gr_shader_status = 0;
    return true;
}


static GLuint crt_compile( GLenum type, const char * const * parts, int nparts,
                           const char * name )
{
    GLuint sh = p_CreateShader( type );
    GLint ok = 0;

    p_ShaderSource( sh, nparts, parts, NULL );
    p_CompileShader( sh );
    p_GetShaderiv( sh, CRT_COMPILE_STATUS, &ok );
    if( ! ok )
    {
        char log[1024];
        log[0] = 0;
        p_GetShaderInfoLog( sh, sizeof(log), NULL, log );
        GenPrintf( EMSG_warn, "CRT shader %s: %s shader failed to compile:\n%s\n",
                   name, (type == CRT_VERTEX_SHADER) ? "vertex" : "fragment", log );
        p_DeleteShader( sh );
        return 0;
    }
    return sh;
}


static GLuint crt_link( const char * const * vparts, int nv,
                        const char * const * fparts, int nf, const char * name )
{
    GLuint vs, fs, prog;
    GLint ok = 0;

    vs = crt_compile( CRT_VERTEX_SHADER, vparts, nv, name );
    if( ! vs )
        return 0;
    fs = crt_compile( CRT_FRAGMENT_SHADER, fparts, nf, name );
    if( ! fs )
    {
        p_DeleteShader( vs );
        return 0;
    }

    prog = p_CreateProgram();
    p_AttachShader( prog, vs );
    p_AttachShader( prog, fs );
    // The libretro attribute names.  Location 0 is the one that makes a vertex.
    p_BindAttribLocation( prog, 0, "VertexCoord" );
    p_BindAttribLocation( prog, 1, "TexCoord" );
    p_BindAttribLocation( prog, 2, "COLOR" );
    p_LinkProgram( prog );
    // Flagged for deletion; they go when the program does.
    p_DeleteShader( vs );
    p_DeleteShader( fs );

    p_GetProgramiv( prog, CRT_LINK_STATUS, &ok );
    if( ! ok )
    {
        char log[1024];
        log[0] = 0;
        p_GetProgramInfoLog( prog, sizeof(log), NULL, log );
        GenPrintf( EMSG_warn, "CRT shader %s: failed to link:\n%s\n", name, log );
        p_DeleteProgram( prog );
        return 0;
    }
    return prog;
}


// Each "#pragma parameter NAME "label" default min max step" line becomes a
// uniform set to its default, which is what RetroArch does.  The shaders'
// fallback #defines for the other mode are not usable: crt-geom's
// "#define lum 0.0" collides with a local variable called lum.
static void crt_set_parameters( GLuint prog, const char * src )
{
    static const char tag[] = "#pragma parameter ";
    const char * p = src;

    while( ( p = strstr( p, tag ) ) )
    {
        char name[64];
        float value;
        const char * q;
        int n = 0;

        p += sizeof(tag) - 1;
        while( *p == ' ' )  p++;
        while( n < 63 && *p && *p != ' ' && *p != '\n' )
            name[n++] = *p++;
        name[n] = 0;
        // Skip the quoted label.
        q = strchr( p, '"' );
        if( ! q || strchr( p, '\n' ) < q )  continue;
        q = strchr( q + 1, '"' );
        if( ! q )  break;
        if( sscanf( q + 1, "%f", &value ) == 1 )
        {
            GLint loc = p_GetUniformLocation( prog, name );
            if( loc >= 0 )
                p_Uniform1f( loc, value );
        }
        p = q + 1;
    }
}


// A libretro shader is one file compiled twice, with VERTEX or FRAGMENT
// defined.  GLSL 1.20 is GL 2.1, which is what the Pi's driver offers; the
// shaders pick their old-style keywords for anything below 1.30.
static GLuint crt_program( int which )
{
    const crt_shader_def_t * def = &crt_shader_defs[which];
    const char * vparts[2];
    const char * fparts[2];
    GLuint prog;

    if( crt_prog_state[which] )
        return crt_prog[which];

    // Settled before building: a failure prints to the console, and the
    // console can redraw the screen, which comes straight back here.
    crt_prog_state[which] = 2;

    vparts[0] = "#version 120\n#define VERTEX\n#define PARAMETER_UNIFORM\n";
    vparts[1] = def->src;
    fparts[0] = "#version 120\n#define FRAGMENT\n#define PARAMETER_UNIFORM\n";
    fparts[1] = def->src;
    prog = crt_link( vparts, 2, fparts, 2, def->name );
    if( prog )
    {
        // Uniform values belong to the program, so this is done once.
        p_UseProgram( prog );
        crt_set_parameters( prog, def->src );
        p_UseProgram( 0 );
        crt_prog[which] = prog;
        crt_prog_state[which] = 1;
        GenPrintf( EMSG_info, "CRT shader %s: ready\n", def->name );
    }
    return prog;
}


// The box filter averages each factor x factor block of screen pixels into
// one texel.  It is generated per factor with every tap unrolled, since the
// Pi's GPU cannot run a loop, and uses bilinear sampling to read two pixels
// per tap along each axis where it can.
static int crt_box_axis( int factor, float * offset, float * weight )
{
    int n = 0, k;
    for( k = 0; k + 1 < factor; k += 2 )
    {
        offset[n] = k + 1.0f;   // between pixels k and k+1: averages both
        weight[n++] = 2.0f;
    }
    if( factor & 1 )
    {
        offset[n] = factor - 0.5f;   // centre of the last pixel
        weight[n++] = 1.0f;
    }
    return n;
}

static GLuint crt_box_program( int factor )
{
    static const char * vsrc =
        "#version 120\n"
        "attribute vec4 VertexCoord;\n"
        "void main() { gl_Position = VertexCoord; }\n";
    char * fsrc;
    char * p;
    float off[CRT_MAX_FACTOR], wt[CRT_MAX_FACTOR];
    int n, i, j;
    const char * vparts[1];
    const char * fparts[1];
    GLuint prog;

    if( box_factor == factor )
        return box_prog;   // 0 if it failed: not retried every frame
    if( box_prog )
        p_DeleteProgram( box_prog );
    box_prog = 0;
    box_factor = factor;

    n = crt_box_axis( factor, off, wt );
    fsrc = (char *) malloc( 512 + n * n * 96 );
    if( ! fsrc )
        return 0;
    p = fsrc;
    p += sprintf( p,
        "#version 120\n"
        "uniform sampler2D Texture;\n"
        "uniform vec2 SrcPix;\n"
        "void main() {\n"
        "  vec2 b = floor(gl_FragCoord.xy) * %d.0;\n"
        "  vec3 s = vec3(0.0);\n", factor );
    for( j = 0; j < n; j++ )
        for( i = 0; i < n; i++ )
            p += sprintf( p, "  s += %.6f * texture2D(Texture, (b + vec2(%.1f, %.1f)) * SrcPix).rgb;\n",
                          wt[i] * wt[j] / (float)(factor * factor), off[i], off[j] );
    sprintf( p, "  gl_FragColor = vec4(s, 1.0);\n}\n" );

    vparts[0] = vsrc;
    fparts[0] = fsrc;
    prog = crt_link( vparts, 1, fparts, 1, "downsample" );
    free( fsrc );
    box_prog = prog;
    return prog;
}


static void crt_tex_alloc( GLuint * tex, GLuint name, int w, int h )
{
    *tex = name;
    glBindTexture( GL_TEXTURE_2D, *tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, CRT_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, CRT_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );
}

static void crt_tex_filter( GLuint tex, byte linear )
{
    GLint f = linear ? GL_LINEAR : GL_NEAREST;
    glBindTexture( GL_TEXTURE_2D, tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f );
}

static void crt_draw_quad( void )
{
    static const GLfloat pos[16] = { -1,-1,0,1,  1,-1,0,1,  -1,1,0,1,  1,1,0,1 };
    static const GLfloat tc[16]  = {  0, 0,0,0,  1, 0,0,0,   0,1,0,0,  1,1,0,0 };

    p_EnableVertexAttribArray( 0 );
    p_VertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, pos );
    p_EnableVertexAttribArray( 1 );
    p_VertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, 0, tc );
    p_VertexAttrib4f( 2, 1.0f, 1.0f, 1.0f, 1.0f );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    p_DisableVertexAttribArray( 0 );
    p_DisableVertexAttribArray( 1 );
}


void OGL_Shader_Present( void )
{
    static const GLfloat identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    const int w = vid.width, h = vid.height;
    int which = cv_grshader.value - 1;
    int factor, in_w, in_h;
    GLuint prog, in_tex;
    GLint loc;
    static byte busy = 0;

    // A message printed from in here can make the console redraw and swap.
    if( busy )
        return;

    crt_frame_saved = 0;
    if( which < 0 || which >= NUM_CRT_SHADERS || w <= 0 || h <= 0 )
    {
        if( crt_init_state != 2 )
            gr_shader_status = 0;
        return;
    }
    busy = 1;
    prog = crt_init() ? crt_program( which ) : 0;
    busy = 0;
    if( crt_init_state == 1 )
        gr_shader_status = prog ? 0 : 2;
    if( ! prog )
        return;

    factor = h / 200;
    if( factor < 1 )  factor = 1;
    if( factor > CRT_MAX_FACTOR )  factor = CRT_MAX_FACTOR;
    in_w = w / factor;
    in_h = h / factor;

    glPushAttrib( GL_ALL_ATTRIB_BITS );
    glPushClientAttrib( GL_CLIENT_ALL_ATTRIB_BITS );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_FOG );
    glDisable( GL_SCISSOR_TEST );
    glDisable( GL_CULL_FACE );
    glDisable( GL_STENCIL_TEST );
    glDepthMask( GL_FALSE );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    // Conventional arrays alias the generic ones; none may be left enabled.
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );

    // 1. The finished frame, unfiltered.
    if( src_w != w || src_h != h )
    {
        crt_tex_alloc( &src_tex, CRT_TEX_SRC, w, h );
        src_w = w;
        src_h = h;
    }
    glReadBuffer( GL_BACK );
    glBindTexture( GL_TEXTURE_2D, src_tex );
    glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h );
    in_tex = src_tex;

    // 2. Down to one texel per line of the tube.
    if( factor > 1 && crt_have_fbo )
    {
        if( low_w != in_w || low_h != in_h || ! low_fbo )
        {
            crt_tex_alloc( &low_tex, CRT_TEX_LOW, in_w, in_h );
            low_w = in_w;
            low_h = in_h;
            if( ! low_fbo )
                p_GenFramebuffers( 1, &low_fbo );
            p_BindFramebuffer( CRT_FRAMEBUFFER, low_fbo );
            p_FramebufferTexture2D( CRT_FRAMEBUFFER, CRT_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D, low_tex, 0 );
            if( p_CheckFramebufferStatus( CRT_FRAMEBUFFER ) != CRT_FRAMEBUFFER_COMPLETE )
            {
                GenPrintf( EMSG_warn, "CRT shader: framebuffer object incomplete, not downsampling\n" );
                crt_have_fbo = 0;
            }
            p_BindFramebuffer( CRT_FRAMEBUFFER, 0 );
        }
        busy = 1;
        if( crt_have_fbo )
            crt_box_program( factor );
        busy = 0;
        if( crt_have_fbo && box_prog )
        {
            p_BindFramebuffer( CRT_FRAMEBUFFER, low_fbo );
            glViewport( 0, 0, in_w, in_h );
            p_UseProgram( box_prog );
            crt_tex_filter( src_tex, 1 );
            p_Uniform1i( p_GetUniformLocation( box_prog, "Texture" ), 0 );
            p_Uniform2f( p_GetUniformLocation( box_prog, "SrcPix" ), 1.0f / w, 1.0f / h );
            crt_draw_quad();
            p_BindFramebuffer( CRT_FRAMEBUFFER, 0 );
            in_tex = low_tex;
        }
    }

    // 3. The tube, over the whole screen.  Without the downsample the shader
    // is still told the small size, so it draws as many lines either way.
    glViewport( 0, 0, w, h );
    p_UseProgram( prog );
    crt_tex_filter( in_tex, crt_shader_defs[which].linear );
    loc = p_GetUniformLocation( prog, "Texture" );
    if( loc >= 0 )  p_Uniform1i( loc, 0 );
    loc = p_GetUniformLocation( prog, "MVPMatrix" );
    if( loc >= 0 )  p_UniformMatrix4fv( loc, 1, GL_FALSE, identity );
    loc = p_GetUniformLocation( prog, "TextureSize" );
    if( loc >= 0 )  p_Uniform2f( loc, (GLfloat)in_w, (GLfloat)in_h );
    loc = p_GetUniformLocation( prog, "InputSize" );
    if( loc >= 0 )  p_Uniform2f( loc, (GLfloat)in_w, (GLfloat)in_h );
    loc = p_GetUniformLocation( prog, "OutputSize" );
    if( loc >= 0 )  p_Uniform2f( loc, (GLfloat)w, (GLfloat)h );
    loc = p_GetUniformLocation( prog, "FrameCount" );
    if( loc >= 0 )  p_Uniform1i( loc, crt_frame_count );
    loc = p_GetUniformLocation( prog, "FrameDirection" );
    if( loc >= 0 )  p_Uniform1i( loc, 1 );
    crt_draw_quad();

    p_UseProgram( 0 );
    glPopClientAttrib();
    glPopAttrib();

    crt_frame_count++;
    crt_frame_saved = 1;
}
