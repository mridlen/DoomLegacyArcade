// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_defs.h 1593 2021-10-16 07:36:40Z wesleyjohnson $
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
// $Log: hw_defs.h,v $
// Revision 1.17  2001/12/26 15:56:12  hurdler
// Manage transparent wall a little better
//
// Revision 1.16  2001/12/15 18:41:36  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.15  2001/08/26 15:27:29  bpereira
// added fov for glide and fixed newcoronas code
//
// Revision 1.14  2001/08/19 15:40:07  bpereira
// added Treansform (and lighting) to glide
//
// Revision 1.13  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.12  2001/08/07 00:44:05  hurdler
// MD2 implementation is getting better but still need lots of work
//
// Revision 1.11  2001/02/28 17:50:56  bpereira
// Revision 1.10  2001/02/10 12:27:14  bpereira
// Revision 1.9  2001/01/25 18:56:27  bpereira
// Revision 1.8  2000/11/02 19:49:39  bpereira
// Revision 1.7  2000/08/31 14:30:57  bpereira
// Revision 1.6  2000/07/01 09:23:50  bpereira
// Revision 1.5  2000/05/05 18:00:05  bpereira
//
// Revision 1.4  2000/04/18 16:07:16  hurdler
// better support of decals
//
// Revision 1.3  2000/04/11 01:00:59  hurdler
// Better coronas support
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      3D hardware renderer API definitions
//
//-----------------------------------------------------------------------------

#ifndef HW_DEFS_H
#define HW_DEFS_H

#include "doomdef.h"
#include "doomtype.h"

// ==========================================================================
//                                                               SIMPLE TYPES
// ==========================================================================

#if 1
// 64bit safe
typedef int32_t    FINT;  // unused
typedef uint32_t   FUINT;
//typedef unsigned char   FUBYTE;
typedef uint32_t   FBITFIELD;
//typedef float           FLOAT;
//typedef unsigned char   FBOOLEAN;
#else
typedef long            FINT;  // unused
typedef unsigned long   FUINT;
typedef unsigned char   FUBYTE;
typedef unsigned long   FBITFIELD;
typedef float           FLOAT;
typedef unsigned char   FBOOLEAN;
#endif

// ==========================================================================
//                                                                      MATHS
// ==========================================================================

// Constants
#undef  PI
#define PI     (3.1415926535897932)
#define DEGREE (.01745328f)     // 2*PI/360


// ==========================================================================
//                                                                     COLORS
// ==========================================================================

// byte value for paletted graphics, which represent the transparent color
#define HWR_PATCHES_CHROMAKEY_COLORINDEX   247
#define HWR_CHROMAKEY_EQUIVALENTCOLORINDEX   0

// the chroma key color shows on border sprites, set it to black
#define HWR_PATCHES_CHROMAKEY_COLORVALUE     (0x00000000)    //RGBA format as in grSstWinOpen()

// RGBA Color components with float type ranging [ 0 ... 1 ]
typedef struct
{
    float  red;
    float  green;
    float  blue;
    float  alpha;
} RGBA_float_t;

#if 0
// Unused
typedef struct
{
    byte  alpha;
    byte  red;
    byte  green;
    byte  blue;
} ARGB_t;
#endif



// ==========================================================================
//                                                                    VECTORS
// ==========================================================================

// Simple 2D coordinate
typedef struct 
{
    float x,y;
} v2d_t;

// Simple 3D vector
typedef struct FVector
{
    float x,y,z;
} v3d_t;

// [WDJ] joint 3D model vector (vertex coords + texture coords)
// used for wallVerts and HWR API
// No more extra copying
typedef struct 
{
    float  x,y,z;
    float  sow,tow;  // texture coordinates (s over w) (t over w)
//    float  w;      // only set to 1.0 (unused)
#ifdef _GLIDE_ARGB_
    FUINT  argb;     // flat-shaded color (used only in Glide, maybe)
#endif
} vxtx3d_t;

//Hurdler: Transform (coords + angles)
//BP: transform order : scale(rotation_x(rotation_y(translation(v))))
typedef struct 
{
    float  x,y,z;           // position
    float  anglex,angley;   // aimingangle / viewangle 
    float  scalex,scaley,scalez;
    float  fovxangle, fovyangle;
    int	   splitscreen;
} FTransform_t;


// ==========================================================================
//                                                               RENDER MODES
// ==========================================================================

// [WDJ] Conditional so can keep original around for comparision.
// Do not know what gotchas may be lurking in the code.
#define BLEND_FIELD


// Flags describing how to render a polygon
// You pass a combination of these flags to DrawPolygon()
typedef enum
{
#ifdef BLEND_FIELD
    // Reserve 4 bits for the blend field, but only use 3 for now.
    // Blends are mutually exclusive, and allows use of lookup tables.
    PF_blend_field      = 0x0000000F,   // Blend field
    PF_blend_field8     = 0x00000007,   // Blend field with only 8 entries.
    // Values for blend field
    PF_Overdraw         = 0x00000000,   // Poly writes over existing pixels.
    PF_Masked           = 0x00000001,   // Poly is alpha scaled and 0 alpha pels are discarded (holes in texture)
    PF_Translucent      = 0x00000002,   // Poly is transparent, alpha = level of transparency
    PF_AdditiveAlpha    = 0x00000003,   // Poly is added to the frame buffer
    PF_Additive         = 0x00000083,   // Poly is added to the frame buffer, no alpha test.
    PF_Environment      = 0x00000004,   // Poly should be drawn environment mapped.
                                        // Hurdler: used for text drawing
    PF_Subtractive      = 0x00000005,   // for splat
    PF_Fog		= PF_Translucent,   // Fog sheet, alpha = translucency
   
    PF_InvisibleB       = 0x00000006,   // Invisible poly affects Z-buffer
    PF_blend7           = 0x00000007,   // TBD
    PF_invalid          = 0x0000000F,
     
    // additional effects
    PF_NoAlphaTest      = 0x00000080,   // hidden param, used by Additive
#else   
    // the first 5 are mutually exclusive Blending
    PF_Masked           = 0x00000001,   // Poly is alpha scaled and 0 alpha pels are discarded (holes in texture)
    PF_Translucent      = 0x00000002,   // Poly is transparent, alpha = level of transparency
    PF_Additive         = 0x00000084,   // Poly is added to the frame buffer
    PF_Environment      = 0x00000008,   // Poly should be drawn environment mapped.
                                        // Hurdler: used for text drawing
    PF_Subtractive      = 0x00000010,   // for splat
    PF_Fog		= PF_Translucent,   // Fog sheet, alpha = translucency
//    PF_Fog		= 0x00000020,   // Fog sheet, alpha = translucency
     
    // additional effects
    PF_NoAlphaTest      = 0x00000080,   // hidden param, used by Additive
    PF_Blending         = (PF_Environment|PF_Additive|PF_Translucent|PF_Masked|PF_Substractive)&~PF_NoAlphaTest,
#endif

    // other flag bits
    PF_Occlude          = 0x00000100,   // Update the depth buffer
    PF_NoDepthTest      = 0x00000200,   // Disable the depth test mode
#ifndef BLEND_FIELD
    // This disables the color mask.
    // Only Glide was using it for the Invisible implementation.
    PF_InvisibleColor   = 0x00000400,   // Disable write to color buffer
#endif
    PF_Decal            = 0x00000800,   // Enable polygon offset
    PF_Modulated        = 0x00001000,   // Modulation ( multiply output with constant ARGB )
                                        // When set, pass the color constant into the FSurfaceInfo -> FlatColor
    PF_NoTexture        = 0x00002000,   // Use the small white texture
    PF_Corona           = 0x00004000,   // Tell the rendrer we are drawing a corona
    PF_MD2              = 0x00008000,   // Tell the rendrer we are drawing an MD2
    PF_Clip             = 0x40000000,   // clip to frustum and nearz plane (glide only, automatic in opengl)
    PF_NoZClip          = 0x20000000,   // in conjonction with PF_Clip
    PF_Debug            = 0x80000000    // print debug message in driver :)
}  PolyFlags_e;


typedef enum
{
    SF_DYNLIGHT         = 0x00000001,

}  SurfFlags_e;

typedef enum
{
    TF_WRAPX            = 0x00000001,            // wrap around X
    TF_WRAPY            = 0x00000002,            // wrap around Y
    TF_WRAPXY           = TF_WRAPY | TF_WRAPX,   // very common so use alias is more easy
    TF_CHROMAKEYED      = 0x00000010,
    TF_Her_Raw_Pic      = 0x00000020,   // the lump is a raw heretic pic
    TF_TRANSPARENT      = 0x00000040,            // texture with some alpha=0
    TF_Opaquetrans      = 0x00000100,   // Some translucent pixels are opaque (fx1)
    TF_Fogsheet         = 0x00000200,   // Generate a fog sheet
}  TextureFlags_e;

#ifdef TODO
struct FTextureInfo_s
{
    FUINT       Width;              // Pixels
    FUINT       Height;             // Pixels
    byte        *TextureData;       // Image data
    FUINT       Format;             // FORMAT_RGB, ALPHA ...
    FBITFIELD   Flags;              // Flags to tell driver about texture (see ETextureFlags)
    void        DriverExtra;        // (OpenGL texture object nr, ... )
                                    // chromakey enabled,...
    
    struct FTextureInfo_s * next;   // Manage list of downloaded textures.
};
typedef struct FTextureInfo_s  FTextureInfo_t;
#else
typedef struct Mipmap_s FTextureInfo_t;
#endif

// Description of a renderable surface
typedef struct
{
     FUINT    polyflags;      // PF_ flags (used for blend)
     FUINT    texflags;       // TF_ flags (used for transparent)
     RGBA_t   FlatColor;      // Flat-shaded color used with PF_Modulated mode
} FSurfaceInfo_t;

//Hurdler: added for backward compatibility
typedef enum {
    HWD_SET_FOG_TABLE = 1,
    HWD_SET_FOG_MODE,
    HWD_SET_FOG_COLOR,
    HWD_SET_FOG_DENSITY,
    HWD_SET_FOV,
    HWD_SET_POLYGON_SMOOTH,
    HWD_SET_TINT_COLOR,  // damage and special object palette
    HWD_SET_TEXTUREFILTERMODE,
    HWD_NUMSTATE,
    // [WDJ] Stifle compiler complaints
    // Unknown where this value is ever set
    HWD_MIRROR_77 = 77  // see SetSpecialState where it does ClearBuffer
} hwd_specialstate_e;

typedef enum {
    HWD_SET_TEXTUREFILTER_POINTSAMPLED, 
    HWD_SET_TEXTUREFILTER_BILINEAR,
    HWD_SET_TEXTUREFILTER_TRILINEAR,
    HWD_SET_TEXTUREFILTER_MIXED1,
    HWD_SET_TEXTUREFILTER_MIXED2,
} hwd_filtermode_e;


#endif // HW_DEFS_H
