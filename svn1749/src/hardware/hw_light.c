// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_light.c 1636 2022-11-03 01:44:00Z wesleyjohnson $
//
// Copyright (C) 1998-2015 by DooM Legacy Team.
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
// $Log: hw_light.c,v $
// Revision 1.45  2004/07/27 08:19:38  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.44  2002/09/14 12:36:54  hurdler
// Add heretic lights (I'm happy with that for now)
//
// Revision 1.43  2002/09/10 20:49:07  hurdler
// Add lights to Heretic weapons
//
// Revision 1.42  2001/12/28 16:57:45  hurdler
// Add setcorona command to FS
//
// Revision 1.41  2001/12/26 17:24:47  hurdler
// Update Linux version
//
// Revision 1.40  2001/08/27 19:59:35  hurdler
// Fix colormap in heretic + opengl, fixedcolormap and NEWCORONA
//
// Revision 1.39  2001/08/26 15:27:29  bpereira
// added fov for glide and fixed newcoronas code
//
// Revision 1.38  2001/08/11 01:24:30  hurdler
// Fix backface culling problem with floors/ceiling
//
// Revision 1.37  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.36  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.35  2001/08/06 14:13:45  hurdler
// Crappy MD2 implementation (still need lots of work)
//
// Revision 1.34  2001/04/28 15:18:46  hurdler
// newcoronas defined again
//
// Revision 1.33  2001/04/17 22:30:40  hurdler
// Revision 1.32  2001/04/09 14:17:45  hurdler
// Revision 1.31  2001/02/28 17:50:56  bpereira
// Revision 1.30  2001/02/24 13:35:22  bpereira
// Revision 1.29  2001/01/25 18:56:27  bpereira
// Revision 1.28  2000/11/18 15:51:25  bpereira
// Revision 1.27  2000/11/02 19:49:39  bpereira
// Revision 1.26  2000/10/04 16:21:57  hurdler
// Revision 1.25  2000/09/28 20:57:20  bpereira
// Revision 1.24  2000/09/21 16:45:11  bpereira
// Revision 1.23  2000/08/31 14:30:57  bpereira
// Revision 1.22  2000/08/11 19:11:57  metzgermeister
// Revision 1.21  2000/08/11 12:27:43  hurdler
// Revision 1.20  2000/08/10 19:58:04  bpereira
// Revision 1.19  2000/08/10 14:16:25  hurdler
// Revision 1.18  2000/08/03 17:57:42  bpereira
// Revision 1.17  2000/07/01 09:23:50  bpereira
//
// Revision 1.16  2000/05/09 21:09:18  hurdler
// people prefer coronas on plasma riffles
//
// Revision 1.15  2000/04/24 20:24:38  bpereira
//
// Revision 1.14  2000/04/24 15:46:34  hurdler
// Support colormap for text
//
// Revision 1.13  2000/04/23 16:19:52  bpereira
//
// Revision 1.12  2000/04/18 12:52:21  hurdler
//
// Revision 1.10  2000/04/14 16:34:26  hurdler
// some nice changes for coronas
//
// Revision 1.9  2000/04/12 16:03:51  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.8  2000/04/11 01:00:59  hurdler
// Better coronas support
//
// Revision 1.7  2000/04/09 17:18:01  hurdler
// modified coronas' code for 16 bits video mode
//
// Revision 1.6  2000/04/06 20:50:23  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.5  2000/03/29 19:39:49  bpereira
//
// Revision 1.4  2000/03/07 03:31:45  hurdler
// fix linux compilation
//
// Revision 1.3  2000/03/05 17:10:56  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Dynamic/Static lighting & coronas add on by Hurdler
//      !!! Under construction !!!
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "hw_light.h"
#include "hw_main.h"
#include "i_video.h"
#include "z_zone.h"
#include "m_random.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "w_wad.h"
#include "r_state.h"
#include "r_main.h"
#include "p_local.h"

//=============================================================================
//                                                                      DEFINES
//=============================================================================

// [WDJ] Makes debugging difficult when references like these are hidden.
// Make them visible in the code.
//#define DL_SQRRADIUS(x)     dynlights->p_lspr[(x)]->dynamic_sqrradius
//#define DL_RADIUS(x)        dynlights->p_lspr[(x)]->dynamic_radius
//#define LIGHT_POS(i)        dynlights->position[(i)]

#define DL_HIGH_QUALITY
//#define LIGHTMAPFLAGS  (PF_Masked|PF_Clip|PF_NoAlphaTest)  // debug see overdraw
#define LIGHTMAPFLAGS (PF_Modulated|PF_Additive|PF_Clip)

//#define DYN_LIGHT_VERTEX

//=============================================================================
//                                                                       GLOBAL
//=============================================================================


consvar_t cv_grdynamiclighting = {"gr_dynamiclighting",  "On", CV_SAVE, CV_OnOff };
consvar_t cv_grstaticlighting  = {"gr_staticlighting",   "On", CV_SAVE, CV_OnOff };
#ifdef CORONA_CHOICE
CV_PossibleValue_t grcorona_draw_cons_t[] = { {0, "Off"}, {1, "Sprite"}, {2, "Dyn"}, {3, "Auto"}, {0, NULL} };
consvar_t cv_grcorona_draw     = {"gr_corona_draw",      "Auto", CV_SAVE, grcorona_draw_cons_t };
#endif

// Select by view, using indirection into view_dynlights.
static dynlights_t view_dynlights[2]; // 2 players in splitscreen mode
static dynlights_t *dynlights = &view_dynlights[0];


// Enum type  sprite_light_e  is defined in r_defs.h.
// It defines SPLGT_xxx for the xxx_SPR names used in scripts.


//=============================================================================
//                                                                       EXTERN
//=============================================================================

extern  float   gr_viewludsin;
extern  float   gr_viewludcos;


//=============================================================================
//                                                                       PROTOS
//=============================================================================

static void  HWR_SetLight( void );

// --------------------------------------------------------------------------
// calcul la projection d'un point sur une droite (determinée par deux 
// points) et ensuite calcul la distance (au carré) de ce point au point
// projecté sur cette droite
// --------------------------------------------------------------------------
static float HWR_DistP2D(vxtx3d_t *p1, vxtx3d_t *p2, v3d_t *p3, /*OUT*/ v3d_t *inter)
{
    if (p1->z == p2->z) {
        inter->x = p3->x;
        inter->z = p1->z;
    } else if (p1->x == p2->x) {
        inter->x = p1->x;
        inter->z = p3->z;
    } else {
        register float local, pente;
        // Wat een mooie formula! Hurdler's math ;-)
        pente = ( p1->z - p2->z ) / ( p1->x - p2->x );
        local = p1->z - p1->x*pente;
        inter->x = (p3->z - local + p3->x/pente) * (pente/(pente*pente+1));
        inter->z = inter->x*pente + local;
    }

    return (p3->x - inter->x) * (p3->x - inter->x)
         + (p3->z - inter->z) * (p3->z - inter->z);
}

// check if sphere (radius r) centred in p3 touch the bounding box defined by p1, p2
static
boolean SphereTouchBBox3D(vxtx3d_t *p1, vxtx3d_t *p2, v3d_t *p3, float r)
{
    float minx=p1->x,maxx=p2->x,miny=p2->y,maxy=p1->y,minz=p2->z,maxz=p1->z;

    if( minx>maxx )
    {
        minx=maxx;
        maxx=p1->x;
    }
    if( minx-r > p3->x ) return false;
    if( maxx+r < p3->x ) return false;

    if( miny>maxy )
    {
        miny=maxy;
        maxy=p2->y;
    }
    if( miny-r > p3->y ) return false;
    if( maxy+r < p3->y ) return false;

    if( minz>maxz )
    {
        minz=maxz;
        maxz=p2->z;
    }
    if( minz-r > p3->z ) return false;
    if( maxz+r < p3->z ) return false;

    return true;
}

// Hurdler: The old code was removed by me because I don't think it will be used one day.
//          (It's still available on the CVS for educational purpose: Revision 1.8)

// --------------------------------------------------------------------------
// calcul du dynamic lighting sur les murs
// lVerts contient les coords du mur sans le mlook (up/down)
// --------------------------------------------------------------------------
void HWR_WallLighting(vxtx3d_t *wlVerts)
{
#ifdef DYN_LIGHT_VERTEX
    vxtx3d_t  dlv[4];
#endif
    FSurfaceInfo_t  Surf;
    spr_light_t   * lsp;  // dynlights sprite_light
    v3d_t *         light_pos;
    v3d_t           inter;
    int             i, j;
    float           dist_p2d, d[4], s;

#ifdef DYN_LIGHT_VERTEX
    memcpy( dlv, wlVerts, sizeof(vxtx3d_t) * 4 );
#endif
    
    // dynlights->nb == 0 if cv_grdynamiclighting.value is not set
    for (j=0; j<dynlights->nb; j++) {
        lsp = dynlights->p_lspr[j];
        light_pos = & dynlights->position[j];
        // check bounding box first
        if( ! SphereTouchBBox3D(&wlVerts[2], &wlVerts[0], light_pos, lsp->dynamic_radius ) )
             continue;

        d[0] = wlVerts[2].x - wlVerts[0].x;
        d[1] = wlVerts[2].z - wlVerts[0].z;
        d[2] = light_pos->x - wlVerts[0].x;
        d[3] = light_pos->z - wlVerts[0].z;
        // backface cull
        if( d[2]*d[1] - d[3]*d[0] < 0 )
            continue;

        // check exact distance
        dist_p2d = HWR_DistP2D(&wlVerts[2], &wlVerts[0], light_pos, &inter);
        if (dist_p2d >= lsp->dynamic_sqrradius)
            continue;

        d[0] = sqrt((wlVerts[0].x-inter.x)*(wlVerts[0].x-inter.x)+(wlVerts[0].z-inter.z)*(wlVerts[0].z-inter.z));
        d[1] = sqrt((wlVerts[2].x-inter.x)*(wlVerts[2].x-inter.x)+(wlVerts[2].z-inter.z)*(wlVerts[2].z-inter.z));
        //dAB = sqrt((wlVerts[0].x-wlVerts[2].x)*(wlVerts[0].x-wlVerts[2].x)+(wlVerts[0].z-wlVerts[2].z)*(wlVerts[0].z-wlVerts[2].z));
        //if ( (d[0] < dAB) && (d[1] < dAB) ) // test if the intersection is on the wall
        //{
        //    d[0] = -d[0]; // if yes, the left distance must be negative for texcoord
        //}
        // test if the intersection is on the wall
        if ( (wlVerts[0].x<inter.x && wlVerts[2].x>inter.x) ||
             (wlVerts[0].x>inter.x && wlVerts[2].x<inter.x) ||
             (wlVerts[0].z<inter.z && wlVerts[2].z>inter.z) ||
             (wlVerts[0].z>inter.z && wlVerts[2].z<inter.z) )
        {
            d[0] = -d[0]; // if yes, the left distance must be negative for texcoord
        }
        d[2] = d[1]; d[3] = d[0];
#ifdef DL_HIGH_QUALITY
        s = 0.5f / lsp->dynamic_radius;
#else
        s = 0.5f / sqrt(lsp->dynamic_sqrradius - dist_p2d);
#endif

#ifdef DYN_LIGHT_VERTEX
        for (i=0; i<4; i++) {
            dlv[i].sow = 0.5f + d[i]*s;
            dlv[i].tow = 0.5f + (wlVerts[i].y - light_pos->y)*s*1.2f;
        }
#else
        for (i=0; i<4; i++) {
            wlVerts[i].sow = 0.5f + d[i]*s;
            wlVerts[i].tow = 0.5f + (wlVerts[i].y - light_pos->y)*s*1.2f;
        }
#endif

        HWR_SetLight();

#if 1
        Surf.FlatColor.rgba = dynlights->p_lspr[j]->dynamic_color.rgba;
#else
        // [WDJ] FIXME: Do not know why this is swap, it is not done for corona.
        // Is hardware little-endian ??
        Surf.FlatColor.rgba = LE_SWAP32(dynlights->p_lspr[j]->dynamic_color.rgba);
#endif
#ifdef DL_HIGH_QUALITY
        Surf.FlatColor.s.alpha *= (1 - dist_p2d/lsp->dynamic_sqrradius);
#endif
        if( !dynlights->mo[j]->state )
            return;
        // next state is null so fade out with alpha
        if( dynlights->mo[j]->state->nextstate == S_NULL )
            Surf.FlatColor.s.alpha *= (float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics;

#ifdef DYN_LIGHT_VERTEX
        HWD.pfnDrawPolygon ( &Surf, dlv, 4, LIGHTMAPFLAGS );
#else
        HWD.pfnDrawPolygon ( &Surf, wlVerts, 4, LIGHTMAPFLAGS );
#endif

    } // end for (j=0; j<dynlights->nb; j++)
}


// BP: big hack for a test in lighting ref:1249753487AB
extern int * bsp_bbox;
extern FTransform_t atransform;
// --------------------------------------------------------------------------
// calcul du dynamic lighting sur le sol
// clVerts contient les coords du sol avec le mlook (up/down)
// --------------------------------------------------------------------------
void HWR_PlaneLighting(vxtx3d_t *clVerts, int nrClipVerts)
{
    FSurfaceInfo_t  Surf;
    spr_light_t   * lsp;  // dynlights sprite_light
    v3d_t *         light_pos;
    float           dist_p2d, s;
    int     i, j;
    vxtx3d_t p1,p2;

    p1.z=FIXED_TO_FLOAT( bsp_bbox[BOXTOP] );
    p1.x=FIXED_TO_FLOAT( bsp_bbox[BOXLEFT] );
    p2.z=FIXED_TO_FLOAT( bsp_bbox[BOXBOTTOM] );
    p2.x=FIXED_TO_FLOAT( bsp_bbox[BOXRIGHT] );
    p2.y=clVerts[0].y;
    p1.y=clVerts[0].y;

    for (j=0; j<dynlights->nb; j++) {
        lsp = dynlights->p_lspr[j];
        light_pos = & dynlights->position[j];
        // BP: The kickass Optimization: check if light touch bounding box
        if( ! SphereTouchBBox3D(&p1, &p2, light_pos, lsp->dynamic_radius) )
             continue;

        // backface cull
        //Hurdler: doesn't work with new TANDL code
        if( (clVerts[0].y > atransform.z)       // true mean it is a ceiling false is a floor
             ^ (light_pos->y < clVerts[0].y) ) // true mean light is down plane, false light is up plane
             continue;

        dist_p2d = (clVerts[0].y - light_pos->y);
        dist_p2d *= dist_p2d;
        // done in SphereTouchBBox3D
        //if (dist_p2d >= lsp->dynamic_sqrradius)
        //    continue;
        
#ifdef DL_HIGH_QUALITY
        s = 0.5f / lsp->dynamic_radius;
#else
        s = 0.5f / sqrt(lsp->dynamic_sqrradius - dist_p2d);
#endif
        for (i=0; i<nrClipVerts; i++) {
            clVerts[i].sow = 0.5f + (clVerts[i].x - light_pos->x)*s;
            clVerts[i].tow = 0.5f + (clVerts[i].z - light_pos->z)*s*1.2f;
        }

        HWR_SetLight();

#if 1
        Surf.FlatColor.rgba = dynlights->p_lspr[j]->dynamic_color.rgba;
#else
        // [WDJ] FIXME: Do not know why this is swap, it is not done for corona.
        // Is hardware little-endian ??
        Surf.FlatColor.rgba = LE_SWAP32(dynlights->p_lspr[j]->dynamic_color.rgba);
#endif
#ifdef DL_HIGH_QUALITY
        // dist_p2d < lsp->dynamic_sqrradius
        Surf.FlatColor.s.alpha *= (1 - dist_p2d/lsp->dynamic_sqrradius);
#endif
        if( !dynlights->mo[j]->state )
            return;

        // next state is null so fade out with alpha
        if( dynlights->mo[j]->state->nextstate == S_NULL )
            Surf.FlatColor.s.alpha *= (float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics;

        HWD.pfnDrawPolygon ( &Surf, clVerts, nrClipVerts, LIGHTMAPFLAGS );

    } // end for (j=0; j<dynlights->nb; j++)
}


//======
// Corona

static lumpnum_t  corona_lumpnum;

// Proportional fade of corona from Z1 to Z2
#define  Z1  (250.0f)
#define  Z2  ((255.0f*8) + 250.0f)


#ifdef SPDR_CORONAS
// --------------------------------------------------------------------------
// coronas lighting with the sprite
// --------------------------------------------------------------------------
void HWR_DoCoronasLighting(vxtx3d_t *outVerts, gr_vissprite_t *spr) 
{
    FSurfaceInfo_t  Surf;
    vxtx3d_t        light[4];
    spr_light_t   * lsp;

    lsp = Sprite_Corona_Light_lsp( spr->mobj->sprite, spr->mobj->state );
    if( lsp == NULL )  goto no_corona;
   
    // Objects which emit light.
    if( lsp->splgt_flags & (SPLGT_corona|SPLT_type_field) )
    {
        float cz = (outVerts[0].z + outVerts[2].z) / 2.0;

        // more realistique corona !
        if( cz >= Z2 )
            return;

        int mobjid = (uintptr_t)spr->mobj;  // mobj dependent light selector
        if( Sprite_Corona_Light_fade( lsp, cz, mobjid>>1 ) ==  0 )  goto no_corona;

        // Sprite has a corona, and coronas are enabled.
        Surf.FlatColor.rgba = lsp->corona_color.rgba;
        Surf.FlatColor.s.alpha = corona_alpha;

        float size = corona_size * 2.0;

#if 1
        // compute position doing average
        float cx = (outVerts[0].x + outVerts[2].x) / 2.0;
        float cy = (outVerts[0].y + outVerts[2].y) / 2.0;
#else
        float cx=0.0f, cy=0.0f; // gravity center
        // compute position doing average
        int i;
        for (i=0; i<4; i++) {
            cx += outVerts[i].x;
            cy += outVerts[i].y;
        }
        cx /= 4.0f;  cy /= 4.0f;
#endif

        // put light little forward of the sprite so there is no 
        // z-blocking or z-fighting
        if( cz > 0.5f )  // correction for side drift due to cz change
        {  // -0.75 per unit of cz
           cx += cx * ((-6.0f) / cz);
           cy += cy * ((-6.0f) / cz);
        }
        // need larger value to avoid z-blocking when look down
        cz -= 8.0f;  // larger causes more side-to-side drift

        // Bp; je comprend pas, ou est la rotation haut/bas ?
        //     tu ajoute un offset a y mais si la tu la reguarde de haut 
        //     sa devrais pas marcher ... comprend pas :(
        //     (...) bon je croit que j'ai comprit il est tout pourit le code ?
        //           car comme l'offset est minime sa ce voit pas !
        cy += lsp->light_yoffset;
        light[0].x = light[3].x = cx - size;
        light[1].x = light[2].x = cx + size;
        light[0].y = light[1].y = cy - (size*1.33f); 
        light[2].y = light[3].y = cy + (size*1.33f); 
        light[0].z = light[1].z = light[2].z = light[3].z = cz;
        light[0].sow = 0.0f;   light[0].tow = 0.0f;
        light[1].sow = 1.0f;   light[1].tow = 0.0f;
        light[2].sow = 1.0f;   light[2].tow = 1.0f;
        light[3].sow = 0.0f;   light[3].tow = 1.0f;

        HWR_GetPic(corona_lumpnum);  // TODO: use different coronas

        HWD.pfnDrawPolygon ( &Surf, light, 4, PF_Modulated | PF_Additive | PF_Clip | PF_Corona | PF_NoDepthTest);
    }
no_corona:
    return;
}
#endif

#ifdef DYLT_CORONAS
// Draw coronas from dynamic light list
// Fireflys created by Fragglescript, do not get entered into dynamic list.
void HWR_DL_Draw_Coronas( void )
{
    int j;
    float           size;
    FSurfaceInfo_t  Surf;
    vxtx3d_t        light[4];
    v3d_t *         light_pos;
    float           cx, cy, cz;
    spr_light_t   * lsp;

    if( dynlights->nb == 0 )
         return;
    
    HWR_GetPic(corona_lumpnum);  // TODO: use different coronas

    for( j=0;j<dynlights->nb;j++ )
    {
        lsp = dynlights->p_lspr[j];
        
        // it's an object which emits light
        if ( !(lsp->splgt_flags & SPLGT_corona) )
            continue;

        // gravity center
        light_pos = & dynlights->position[j];
        transform_world_to_gr(light_pos->x, light_pos->y, light_pos->z,
                              /*OUT*/ &cx, &cy, &cz);

        // more realistique corona !
        if( cz >= Z2 )
            continue;

        Surf.FlatColor.rgba = lsp->corona_color.rgba;

        if( Sprite_Corona_Light_fade( lsp, cz, j ) ==  0 )  goto no_corona;

        size = corona_size * 2.0;
        Surf.FlatColor.s.alpha = corona_alpha;

        // put light little forward the sprite so there is no 
        // z-buffer problem (coplanaire polygons)
        // BP: use PF_Decal do not help :(
        if( cz > 0.5f )  // correction for side drift due to cz change
        {
           cx += cx * ((-3.8f) / cz);
           cy += cy * ((-3.8f) / cz);
        }
        cz = cz - 5.0f; 

        light[0].x = light[3].x = cx - size;
        light[1].x = light[2].x = cx + size;
        light[0].y = light[1].y = cy - (size*1.33f); 
        light[2].y = light[3].y = cy + (size*1.33f); 
        light[0].z = light[1].z = light[2].z = light[3].z = cz;
        light[0].sow = 0.0f;   light[0].tow = 0.0f;
        light[1].sow = 1.0f;   light[1].tow = 0.0f;
        light[2].sow = 1.0f;   light[2].tow = 1.0f;
        light[3].sow = 0.0f;   light[3].tow = 1.0f;

        HWD.pfnDrawPolygon ( &Surf, light, 4, PF_Modulated | PF_Additive | PF_Clip | PF_NoDepthTest | PF_Corona );
    }
no_corona:
    return;
}
#endif

// --------------------------------------------------------------------------
// Remove all the dynamic lights at each frame
// --------------------------------------------------------------------------
// Called from P_SetupLevel, and maybe from HWR_RenderPlayerView
void HWR_Reset_Lights(void)
{
    dynlights->nb = 0;
}

// --------------------------------------------------------------------------
// Change view, thus change lights (splitscreen)
// --------------------------------------------------------------------------
// Called from HWR_RenderPlayerView
void HWR_Set_Lights(byte viewnumber)
{
    dynlights = &view_dynlights[viewnumber];
}

// --------------------------------------------------------------------------
// Add a light for dynamic lighting
// The light position is already transformed except for mlook
// --------------------------------------------------------------------------
void HWR_DL_AddLightSprite(gr_vissprite_t *spr, MipPatch_t *mpatch)
{
    v3d_t *    light_pos;
    spr_light_t * lsp;
    mobj_t *   sprmobj;
    byte li;

    //Hurdler: moved here because it's better ;-)
    if (!cv_grdynamiclighting.value)
        return;

#ifdef PARANOIA
    if(!spr->mobj)
    {
        I_SoftError("AddLight: vissprite without mobj !!!");
        return;
    }
#endif

    sprmobj = spr->mobj;

    // check if sprite contain dynamic light
    li = sprite_light_ind[sprmobj->sprite];
    if( li == LT_NOLIGHT )
        return;

    lsp = &sprite_light[li];
    //CONS_Printf("sprite (sprite): %d (%s): %d\n", spr->mobj->sprite, sprnames[spr->mobj->sprite], lsp->splgt_flags);
    if ( (lsp->splgt_flags & SPLGT_dynamic)
         && (((lsp->splgt_flags & (SPLGT_light|SPLGT_corona|SPLGT_rocket)) != SPLGT_light) || cv_grstaticlighting.value) 
         && (dynlights->nb < DL_MAX_LIGHT) 
         && sprmobj->state )
    {
        // Create a dynamic light.
        // Dynamic light position.
        light_pos = & dynlights->position[ dynlights->nb ];
        light_pos->x = FIXED_TO_FLOAT( sprmobj->x );
        light_pos->y = FIXED_TO_FLOAT( sprmobj->z )
         + FIXED_TO_FLOAT( sprmobj->height>>1 ) + lsp->light_yoffset;
        light_pos->z = FIXED_TO_FLOAT( sprmobj->y );

        dynlights->mo[dynlights->nb] = sprmobj;
        if( (sprmobj->state >= &states[S_EXPLODE1]
             && sprmobj->state <= &states[S_EXPLODE3])
         || (sprmobj->state >= &states[S_FATSHOTX1]
             && sprmobj->state <= &states[S_FATSHOTX3]))
        {
            // Rocket explosion
            lsp = &sprite_light[LT_ROCKETEXP];
        }

        dynlights->p_lspr[dynlights->nb] = lsp;
        
        dynlights->nb++;
    } 
}

static MipPatch_t lightmappatch;

void HWR_Init_Light( void )
{
    int i;

    // precalculate sqr radius
    for(i=0;i<NUMLIGHTS;i++)
    {
        spr_light_t *  h = & sprite_light[i];
        h->dynamic_sqrradius = h->dynamic_radius * h->dynamic_radius;
    }

    lightmappatch.mipmap.downloaded = false;
    corona_lumpnum = W_GetNumForName("corona");
}

// -----------------+
// HWR_SetLight     : Download a disc shaped alpha map for rendering fake lights
// -----------------+
void HWR_SetLight( void )
{
    int    i, j;

    if( !lightmappatch.mipmap.downloaded && !lightmappatch.mipmap.GR_data )
    {

        uint16_t * data = Z_Malloc( 128*128*sizeof(uint16_t), PU_HWRCACHE, &lightmappatch.mipmap.GR_data );
                
        for( i=0; i<128; i++ )
        {
            for( j=0; j<128; j++ )
            {
                int pos = ((i-64)*(i-64))+((j-64)*(j-64));
                data[i*128+j] = (pos <= 63*63)?
                    ( ((byte)(255-(4*sqrt(pos)))) << 8 | 0xff )
                   : 0;
            }
        }
        lightmappatch.mipmap.GR_format = GR_TEXFMT_ALPHA_INTENSITY_88;

        lightmappatch.width = 128;
        lightmappatch.height = 128;
        lightmappatch.mipmap.width = 128;
        lightmappatch.mipmap.height = 128;
#ifdef USE_VOODOO_GLIDE
        lightmappatch.mipmap.grInfo.smallLodLog2 = GR_LOD_LOG2_128;
        lightmappatch.mipmap.grInfo.largeLodLog2 = GR_LOD_LOG2_128;
        lightmappatch.mipmap.grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
#endif
        lightmappatch.mipmap.tfflags = 0; //TF_WRAPXY; // DEBUG: view the overdraw !
    }
    HWD.pfnSetTexture( &lightmappatch.mipmap );
}


void HWR_DynamicShadowing(vxtx3d_t *clVerts, int nrClipVerts, player_t *p)
{
    int  i;
    FSurfaceInfo_t  Surf;

    if (!cv_grdynamiclighting.value)
        return;

    for (i=0; i<nrClipVerts; i++) {
        clVerts[i].sow = 0.5f + clVerts[i].x*0.01f;
        clVerts[i].tow = 0.5f + clVerts[i].z*0.01f*1.2f;
    }
    
    HWR_SetLight();

    Surf.FlatColor.rgba = RGBA(0x70, 0x70, 0x70, 0x70);

    HWD.pfnDrawPolygon ( &Surf, clVerts, nrClipVerts, LIGHTMAPFLAGS );
    //HWD.pfnDrawPolygon ( &Surf, clVerts, nrClipVerts, PF_Modulated|PF_Environment|PF_Clip );
}


#ifdef STATICLIGHTMAPS
//**********************************************************
// Hurdler: new code for faster static lighting and and T&L
//**********************************************************

// est ce bien necessaire ?
//static sector_t *gr_frontsector;
static sector_t *gr_backsector ;
static seg_t    *gr_curline;



/*
static void HWR_StoreWallRange (int startfrac, int endfrac)
{
...(voir hw_main.c)...
}
*/


// p1 et p2 c'est le deux bou du seg en float
static
void HWR_Create_WallLightmaps(v3d_t *p1, v3d_t *p2, int lightnum, seg_t *line)
{
    lightmap_t *lp;

    // (...) calcul presit de la projection et de la distance

//    if (dist_p2d >= dynamic_light->lsp[lightnum]->dynamic_sqrradius)
//        return;

    // (...) attention faire le backfase cull histoir de faire mieux que Q3 !

    lp = malloc(sizeof(lightmap_t));
    lp->next = line->lightmaps;
    line->lightmaps = lp;
    
    // (...) encore des bô calcul bien lourd et on stock tout sa dans la lightmap
}

static void HWR_AddLightMapForLine( int lightnum, seg_t *line)
{
    /*
    int                 x1;
    int                 x2;
    angle_t             angle1;
    angle_t             angle2;
    angle_t             span;
    angle_t             tspan;
    */
    v3d_t  p1,p2;
    
    gr_curline = line;
    gr_backsector = line->backsector;
    
    // Reject empty lines used for triggers and special events.
    // Identical floor and ceiling on both sides,
    //  identical light levels on both sides,
    //  and no middle texture.
/*
    if (   gr_backsector->ceilingpic == gr_frontsector->ceilingpic
        && gr_backsector->floorpic == gr_frontsector->floorpic
        && gr_backsector->lightlevel == gr_frontsector->lightlevel
        && gr_curline->sidedef->midtexture == 0)
    {
        return;
    }
*/

    p1.y=FIXED_TO_FLOAT( gr_curline->v1->y );
    p1.x=FIXED_TO_FLOAT( gr_curline->v1->x );
    p2.y=FIXED_TO_FLOAT( gr_curline->v2->y );
    p2.x=FIXED_TO_FLOAT( gr_curline->v2->x );

#if 0   
    // check bbox of the seg
    if( CircleTouchBBox(&p1, &p2,
                        &dynlights->position[lightnum],
                        dynlights->p_lspr[lightnum]->dynamic_radius )
        ==false )
        return;
#endif

    HWR_Create_WallLightmaps(&p1, &p2, lightnum, line);
}


//TODO: see what HWR_AddLine does
static void HWR_CheckSubsector( int num, fixed_t *bbox )
{
    int         count;
    seg_t       *line;
    subsector_t *sub;
    v3d_t       p1,p2;
    int         lightnum;

    p1.y=FIXED_TO_FLOAT( bbox[BOXTOP] );
    p1.x=FIXED_TO_FLOAT( bbox[BOXLEFT] );
    p2.y=FIXED_TO_FLOAT( bbox[BOXBOTTOM] );
    p2.x=FIXED_TO_FLOAT( bbox[BOXRIGHT] );


    if (num < numsubsectors)
    {
        sub = &subsectors[num];         // subsector
        for(lightnum=0; lightnum<dynlights->nb; lightnum++)
        {
#if 0   
            // check bbox of the seg
            if( CircleTouchBBox(&p1, &p2,
                &dynlights->position[lightnum],
                dynlights->p_lspr[lightnum]->dynamic_radius )
                ==false )    
                continue;
#endif

            count = sub->numlines;          // how many linedefs
            line = &segs[sub->firstline];   // first line seg
            while (count--)
            {
                HWR_AddLightMapForLine (lightnum, line);       // compute lightmap
                line++;
            }
        }
    }
}


// --------------------------------------------------------------------------
// Hurdler: this adds lights by mobj.
// --------------------------------------------------------------------------
static void HWR_AddMobjLights(mobj_t *thing)
{
    byte li = sprite_light_ind[thing->sprite];
    if( li == LT_NOLIGHT )  return;

    spr_light_t * lsp = &sprite_light[li];
    if ( lsp->splgt_flags & SPLGT_corona )
    {
        // Sprite has a corona.
        // Create a corona dynamic light.
        v3d_t * light_pos = & dynlights->position[ dynlights->nb ];
        light_pos->x = FIXED_TO_FLOAT( thing->x );
        light_pos->y = FIXED_TO_FLOAT( thing->z ) + lsp->light_yoffset;
        light_pos->z = FIXED_TO_FLOAT( thing->y );
        
        dynlights->p_lspr[dynlights->nb] = lsp;
        
        dynlights->nb++;
        if (dynlights->nb>DL_MAX_LIGHT)
            dynlights->nb = DL_MAX_LIGHT;  // reuse last
    }
}

//Hurdler: The goal of this function is to walk through all the bsp starting
//         on the top. 
//         We need to do that to know all the lights in the map and all the walls
static void HWR_ComputeLightMapsInBSPNode(int bspnum, fixed_t *bbox)
{
    if (bspnum & NF_SUBSECTOR) // Found a subsector?
    {
        if (bspnum == -1)
            HWR_CheckSubsector(0, bbox);  // probably unecessary: see boris' comment in hw_bsp
        else
            HWR_CheckSubsector(bspnum&(~NF_SUBSECTOR), bbox);
        return;
    }
    HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[0], nodes[bspnum].bbox[0]);
    HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[1], nodes[bspnum].bbox[1]);
}

static void HWR_SearchLightsInMobjs(void)
{
    thinker_t*          th;
    //mobj_t*             mobj;

    // search in the list of thinkers
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        // a mobj ?
        if (th->function.acp1 == (actionf_p1)P_MobjThinker)
            HWR_AddMobjLights((mobj_t *)th);
    }
}
#endif

//
// HWR_Create_StaticLightmaps()
//
// Called from P_SetupLevel
void HWR_Create_StaticLightmaps( void )
{
#ifdef STATICLIGHTMAPS
    //Hurdler: TODO!
    CONS_Printf("HWR_CreateStaticLightmaps\n");

    dynlights->nb = 0;

    // First: Searching for lights
    // BP: if i was you, I will make it in create mobj since mobj can be create 
    //     at runtime now with fragle scipt
    HWR_SearchLightsInMobjs();
    CONS_Printf("%d lights found\n", dynlights->nb);

    // Second: Build all lightmap for walls covered by lights
    validcount++; // to be sure
    HWR_ComputeLightMapsInBSPNode( numnodes-1, NULL);

    dynlights->nb = 0;
#endif
    return;
}

/*
TODO:

  - Les coronas ne sont pas gérer avec le nouveau systeme, seul le dynamic lighting l'est
  - calculer l'offset des coronas au chargement du level et non faire la moyenne
    au moment de l'afficher
     BP: euh non en fait il faux encoder la position de la light dans le sprite
         car c'est pas focement au mileux de plus il peut en y avoir plusieur (chandelier)
  - changer la comparaison pour l'affichage des coronas (+ un epsilon)
    BP: non non j'ai trouver mieux :) : lord du AddSprite tu rajoute aussi la coronas
        dans la sprite list ! avec un z de epsilon (attention au ZCLIP_PLANE) et donc on 
        l'affiche en dernier histoir qu'il puisse etre cacher par d'autre sprite :)
        Bon fait metre pas mal de code special dans hwr_project sprite mais sa vaux le 
        coup
  - gerer dynamic et static : retenir le nombre de lightstatic et clearer toute les 
        light>lightstatic (les dynamique) et les lightmap correspondant dans les segs
        puit refaire une passe avec le code si dessus mais rien que pour les dynamiques
        (tres petite modification)
  - finalement virer le hack splitscreen, il n'est plus necessaire !
*/
