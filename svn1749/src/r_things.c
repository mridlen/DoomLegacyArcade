// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_things.c 1747 2025-04-25 07:59:18Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log: r_things.c,v $
// Revision 1.44  2002/11/12 00:06:05  ssntails
// Support for translated translucent columns in software mode.
//
// Revision 1.43  2002/06/30 21:37:48  hurdler
// Ready for 1.32 beta 5 release
//
// Revision 1.42  2001/12/31 16:56:39  metzgermeister
// see Dec 31 log
//
// Revision 1.41  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.40  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.39  2001/06/16 08:07:55  bpereira
// Revision 1.38  2001/06/10 21:16:01  bpereira
//
// Revision 1.37  2001/05/30 18:15:21  stroggonmeth
// Small crashing bug fix...
//
// Revision 1.36  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.35  2001/05/22 14:22:23  hurdler
// show 3d-floors bug + hack for filesearch with vc++
//
// Revision 1.34  2001/05/07 20:27:16  stroggonmeth
// Revision 1.33  2001/04/27 13:32:14  bpereira
//
// Revision 1.32  2001/04/17 21:12:08  stroggonmeth
// Little commit. Re-enables colormaps for trans columns in C and fixes some sprite bugs.
//
// Revision 1.31  2001/03/30 17:12:51  bpereira
//
// Revision 1.30  2001/03/21 18:24:56  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.29  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.28  2001/02/24 13:35:21  bpereira
//
// Revision 1.27  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.26  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.25  2000/11/12 14:15:46  hurdler
//
// Revision 1.24  2000/11/09 17:56:20  stroggonmeth
// Hopefully fixed a few bugs and did a few optimizations.
//
// Revision 1.23  2000/11/03 02:37:36  stroggonmeth
//
// Revision 1.22  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.21  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.20  2000/10/02 18:25:45  bpereira
// Revision 1.19  2000/10/01 10:18:19  bpereira
// Revision 1.18  2000/09/30 16:33:08  metzgermeister
// Revision 1.17  2000/09/28 20:57:18  bpereira
// Revision 1.16  2000/09/21 16:45:08  bpereira
// Revision 1.15  2000/08/31 14:30:56  bpereira
//
// Revision 1.14  2000/08/11 21:37:17  hurdler
// fix win32 compilation problem
//
// Revision 1.13  2000/08/11 19:10:13  metzgermeister
// Revision 1.12  2000/04/30 10:30:10  bpereira
// Revision 1.11  2000/04/24 20:24:38  bpereira
// Revision 1.10  2000/04/20 21:47:24  stroggonmeth
// Revision 1.9  2000/04/18 17:39:40  stroggonmeth
//
// Revision 1.8  2000/04/11 19:07:25  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.7  2000/04/09 02:30:57  stroggonmeth
// Fixed missing sprite def
//
// Revision 1.6  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.5  2000/04/06 21:06:20  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "console.h"
#include "g_game.h"
#include "r_local.h"
#include "sounds.h"             //skin sounds
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_local.h"
  // spr_light_t
#include "v_video.h"
  // pLocalPalette

#include "i_video.h"            //rendermode
#include "d_clisrv.h"
  // [Arcade] D_NumViews : the view grid
#include "m_swap.h"
#include "m_random.h"
#include "infoext.h"
  // remap_sprite



static void R_Init_Skins (void);

#define MINZ                  (FRACUNIT*4)
#define BASEYCENTER           (BASEVIDHEIGHT/2)

// put this in transmap of visprite to draw a shade
#define VIS_SMOKESHADE        ((void*)-1)       


//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t         pspritescale;
fixed_t         pspriteyscale;  //added:02-02-98:aspect ratio for psprites
fixed_t         pspriteiscale;

lighttable_t**  spritelights;	// selected scalelight for the sprite draw



//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t*    sprites;
uint16_t        numsprites;

static char*    spritename;
// For quick comparisons of the first 4 characters of the name.
// It may differ because of endianess, but that does not matter,
// because it is not stored in any file,
#define NAME4_AS_NUM( n )  (*((uint32_t*)(n)))

// spritetmp
#define MAX_FRAMES   29
static spriteframe_t   sprfrm[MAX_FRAMES];

#ifdef ROT16
#define  NUM_SPRITETMP_ROT   16
#else
#define  NUM_SPRITETMP_ROT   8
#endif
static sprite_frot_t   sprfrot[MAX_FRAMES * NUM_SPRITETMP_ROT];


// ==========================================================================
//
//  New sprite loading routines for Legacy : support sprites in pwad,
//  dehacked sprite renaming, replacing not all frames of an existing
//  sprite, add sprites at run-time, add wads at run-time.
//
// ==========================================================================


spriteframe_t *  get_spriteframe( const spritedef_t * spritedef, unsigned int frame_num )
{
   return & spritedef->spriteframe[ frame_num ];
}

sprite_frot_t *  get_framerotation( const spritedef_t * spritedef,
                                    unsigned int frame_num, byte rotation )
{
   return & spritedef->framerotation[ (frame_num * spritedef->frame_rot) + rotation ];
}

const byte srp_to_num_rot[4] = { 0, 1, 8, 16 };


// Convert sprfrot formats.

// The pattern of named rotations to draw rotations.
// Index rotation_char order.
static const byte rotation_char_to_draw[16] =
{ 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };

static
void  transfer_to_spritetmp( const spritedef_t * spritedef )
{
    int   src_frames =  spritedef->numframes;
    byte  src_frame_rot = spritedef->frame_rot;
    spriteframe_t * fmv, * fmp, * fmp_end;
    sprite_frot_t * rtv, * rtv_nxt, * rtp, * rtp_nxt;
    byte r, srp;

    // From
    fmv = spritedef->spriteframe;
    rtv = spritedef->framerotation;
    // To
    fmp = & sprfrm[0];
    fmp_end = & sprfrm[src_frames];
    rtp = & sprfrot[0];

    // Temp frame size is at the max (8 or 16).
    for( ; fmp < fmp_end; fmp++, fmv++ )
    {
        // Index the next frame first.
        rtv_nxt = rtv + src_frame_rot;
        rtp_nxt = rtp + NUM_SPRITETMP_ROT;
        // Adapt the rotation pattern to fill the temp array.
        srp = fmv->rotation_pattern;
        fmp->rotation_pattern = srp;
        switch( srp )
        {
         case SRP_1:
            // Duplicate into all rotations so later PWAD can alter it properly.
            for( r=0; r<NUM_SPRITETMP_ROT; r++ )
            {
                memcpy( rtp, rtv, sizeof(sprite_frot_t) );
                rtp++;
            }
            break;

         case SRP_8:
            // Copy 8 rotations.
            memcpy( rtp, rtv, sizeof(sprite_frot_t) * 8 );
            break;

#ifdef ROT16
         case SRP_16:
            // Copy 16 rotation draw order into rotation_char order.
            for( r=0; r<16; r++ )
            {
                int rd = rotation_char_to_draw[r];
                memcpy( &rtp[r], &rtv[rd], sizeof(sprite_frot_t) );
            }
            break;
#endif
         default:
            break;
        }
        rtv = rtv_nxt;
        rtp = rtp_nxt;
    }
}

static
void  transfer_from_spritetmp( spritedef_t * spritedef,
                               const byte dst_srp, const byte dst_frame_rot )
{
    int   dst_frames =  spritedef->numframes;
    spriteframe_t * fmv, * fmp, * fmp_end;
    sprite_frot_t * rtv, * rtv_nxt, * rtp, * rtp_nxt;
    byte srp;
#ifdef ROT16
    byte r;
#endif

    // From spritetmp
    fmp = & sprfrm[0];
    fmp_end = & sprfrm[dst_frames];
    rtp = & sprfrot[0];
    // To
    fmv = spritedef->spriteframe;
    rtv = spritedef->framerotation;
   
    for( ; fmp < fmp_end; fmp++, fmv++ )
    {
        rtv_nxt = rtv + dst_frame_rot;
        rtp_nxt = rtp + NUM_SPRITETMP_ROT;
        srp = fmp->rotation_pattern;
        fmv->rotation_pattern = srp;
        switch( srp )
        {
         case SRP_1:
            // copy 1 rotations
            memcpy( rtv, rtp, sizeof(sprite_frot_t) );
            break;

         case SRP_8:
            // copy 8 rotations
            memcpy( rtv, rtp, sizeof(sprite_frot_t) * 8 );
            break;

#ifdef ROT16
         case SRP_16:
            // copy 16 rotation draw order into rotation_char order.
            for( r=0; r<16; r++ )
            {
                int rd = rotation_char_to_draw[r];
                memcpy( &rtv[rd], &rtp[r], sizeof(sprite_frot_t) );
            }
            break;
#endif
         default:
           break;
        }
        rtv = rtv_nxt;
        rtp = rtp_nxt;
    }
}


//
//
//
static
void R_InstallSpriteLump ( lumpnum_t     pat_lumpnum,   // graphics patch
                           uint16_t      spritelump_id, // spritelump_t index
                           byte          frame,
                           char          rotation_char,
                           boolean       flipped )
{
    int    r;
    byte   rotation = 0;
    byte   frame_srp;
    spriteframe_t * fmp;
    sprite_frot_t * rtp;

#ifdef ROT16
    // The rotations are saved in the sprfrot in the rotation_char order.
    // They are converted to draw index order when the sprfrot is saved.
    if( rotation_char == '0' )
    {
        rotation = 0;
    }
    else if((rotation_char >= '1') && (rotation_char <= '9'))
    {
        rotation = rotation_char - '1';  // 0..8
    }
    else if((rotation_char >= 'A') && (rotation_char <= 'G'))
    {
        rotation = rotation_char - 'A' + 10 - 1;  // 9..15
    }
    else if((rotation_char >= 'a') && (rotation_char <= 'g'))
    {
        rotation = rotation_char - 'a' + 10 - 1;  // 9..15
    }
#else
    if( rotation_char == '0' )
    {
        rotation = 0;
    }
    else if((rotation_char >= '1') && (rotation_char <= '8'))
    {
        rotation = rotation_char - '1';  // 0..7
    }
#endif

    if( frame >= MAX_FRAMES || rotation >= NUM_SPRITETMP_ROT )
    {
        I_SoftError("R_InstallSpriteLump: Bad frame characters in lump %i\n",
                    spritelump_id);
        return;
    }

    fmp = & sprfrm[frame];
    frame_srp = fmp->rotation_pattern;
    rtp = & sprfrot[ (frame * NUM_SPRITETMP_ROT) + rotation ];

    if( rotation_char == '0' )
    {
        // the lump should be used for all rotations
        if( devparm )
        {
            if( frame_srp == SRP_1 )
            {
                GenPrintf(EMSG_dev,
                 "R_Init_Sprites: Sprite %s frame %c has multiple rot=0 lump\n",
                 spritename, 'A'+frame);
            }
            else if( frame_srp >= SRP_8 )
            {
                GenPrintf(EMSG_dev,
                 "R_Init_Sprites: Sprite %s frame %c has rotations and a rot=0 lump\n",
                 spritename, 'A'+frame);
            }
        }
        fmp->rotation_pattern = SRP_1;
#if 0
        // Only rotation 0.
        rtp->pat_lumpnum = pat_lumpnum;
        rtp->spritelump_id  = spritelump_id;
        rtp->flip = (byte)flipped;
#else
        // Fill the whole array with the single rotation so any later overwrites with
        // SRP_8 will keep the single rotation as the default.
        for (r=0 ; r<NUM_SPRITETMP_ROT ; r++)
        {
            rtp->pat_lumpnum = pat_lumpnum;
            rtp->spritelump_id  = spritelump_id;
            rtp->flip = (byte)flipped;
            rtp++;
        }
#endif
        return;
    }

    // The lump is one rotation in a set.
    if( (frame_srp == SRP_1) && devparm )
    {
        GenPrintf(EMSG_dev,
           "R_Init_Sprites: Sprite %s frame %c has rotations and a rot=0 lump\n",
           spritename, 'A'+frame);
    }
   
#ifdef ROT16
    byte new_frame_srp = ( rotation > 7 )? SRP_16 : SRP_8;
    if( fmp->rotation_pattern < new_frame_srp )
        fmp->rotation_pattern = new_frame_srp;
#else
    fmp->rotation_pattern = SRP_8;
#endif

    if( (rtp->spritelump_id != 0xFFFF) && devparm )
    {
        GenPrintf(EMSG_dev,
           "R_Init_Sprites: Sprite %s : %c : %c  overwrite previous spritelump mapping\n",
           spritename, 'A'+frame, rotation_char );
    }

    // [WDJ] The pat_lumpnum is the (file,lump) used to load the lump from the file.
    // The spritelump_id is the index into the spritelumps table, as maintained in r_data.
    // Any similarity within the original Doom is accidental.
    // This is the only func that changes them, and they are always both updated.
    rtp->pat_lumpnum = pat_lumpnum;
    rtp->spritelump_id = spritelump_id;
    rtp->flip = (byte)flipped;
}



#ifdef ENABLE_DEH_REMAP
// [WDJ] Lumps added, tracking.  Temporary.
byte  * lumptrack = NULL;
uint16_t  lumptrack_alloc = 0;
uint16_t  lumptrack_base = 0;  // first lump in lumptrack

// Return 0 when fail, 1 when already was there, 2 when added.
static
byte  mark_lumptrack( uint16_t lumpnum )
{
#ifdef PARANOIA
    if( (lumpnum < lumptrack_base ) 
        ||( lumpnum > (lumptrack_base + (lumptrack_alloc << 3)) ) )
      {
          GenPrintf(EMSG_error, "mark_lumptrack, lumpnum %i exceeds allocated range %i to %i\n",
                    lumpnum, lumptrack_base, lumptrack_base + (lumptrack_alloc << 3) );
          return 0;
      }
#endif

    int lni = lumpnum - lumptrack_base;
    byte  bitpat = 1 << (lni & 0x07);
    uint16_t ln_indx = lni >> 3;
    if( ! lumptrack )
        return 0;

    if( lumptrack[ ln_indx ] & bitpat )  // check bit for this lump
        return 1;
    
    lumptrack[ ln_indx ] |= bitpat;  // set bit for this lump
    return 0;
}

static
void  release_lump_added( void )
{
    if( lumptrack )
    {
        free( lumptrack );
    }
    lumptrack = NULL;
    lumptrack_alloc = 0;
}

// Allocate for a range of lumps.
// Will zero lumptrack array.
static
void  allocate_lumptrack( uint16_t ln1, uint16_t ln2 )
{
    uint16_t num_req = (ln2 - ln1 + 9) >> 3;
    lumptrack_base = ln1;

    if( lumptrack_alloc < num_req )
    {
        release_lump_added();

        lumptrack = malloc( num_req );
        lumptrack_alloc = num_req;
    }
    memset( lumptrack, 0, lumptrack_alloc );
}
#endif


// Install a single sprite, given its identifying name (4 chars)
//
// (originally part of R_AddSpriteDefs)
//
// Pass: name of sprite : 4 chars
//       spritedef_t
//       wadnum         : wad number, indexes wadfiles[], where patches
//                        for frames are found
//       startlump      : first lump to search for sprite frames
//       endlump        : AFTER the last lump to search
//
// Returns count of the sprites succesfully added
//
int R_AddSingleSpriteDef (char* sprname, spritedef_t* spritedef, int wadnum, int startlump, int endlump)
{
    spriteframe_t * fmp;
    sprite_frot_t * rtp;
    lumpinfo_t * lumpinfo;
    uint32_t    numname;
    lumpnum_t   lumpnum;
    lumpnum_t   fnd_lumpnum = 0;
    int         fnd_count = 0;
    int         l;
    int         spritelump_id;  // spritelump table index
    patch_t     patch;	// temp for read header
    byte        array_srp = SRP_NULL;
    byte        frame;
    byte        install_frame_cnt = 0;
    byte        rotation, frame_rot;
    char        rotation_char;

    memset (sprfrot, 0xFF, sizeof(sprfrot));  // init spritetmp for devparm check
    memset (sprfrm, 0, sizeof(sprfrm));

    // are we 'patching' a sprite already loaded ?
    // if so, it might patch only certain frames, not all
    if (spritedef->numframes) // (then spriteframes is not null)
    {
        // copy the already defined sprite frames
        // Extract to sprfrot format.
        transfer_to_spritetmp( spritedef );
        install_frame_cnt = spritedef->numframes;
    }

    // scan the lumps,
    //  filling in the frames for whatever is found
    lumpinfo = wadfiles[wadnum]->lumpinfo;
    if( lumpinfo == NULL )
        return 0;

    if( endlump > wadfiles[wadnum]->numlumps )
        endlump = wadfiles[wadnum]->numlumps;

    numname = NAME4_AS_NUM(sprname);
 
    // Add all lumps that start with that name.
    for (l=startlump ; l<endlump ; l++)
    {
        const char * lumpname = lumpinfo[l].name;
        if( NAME4_AS_NUM(lumpname) == numname )
        {
            frame = lumpname[4] - 'A';
            rotation_char = lumpname[5];

#ifdef ENABLE_DEH_REMAP
            mark_lumptrack(l);  // this lump has been processed
#endif

            lumpnum = WADLUMP(wadnum,l);  // as used by read lump routines
            // skip NULL sprites from very old dmadds pwads
            if( W_LumpLength( lumpnum ) <= 8 )
                continue;

            fnd_count ++;

            // store sprite info in lookup tables
            //FIXME:numspritelumps do not duplicate sprite replacements
            W_ReadLumpHeader (lumpnum, &patch, sizeof(patch_t)); // to temp
            // [WDJ] Do endian while translate temp to internal.
            spritelump_id = R_Get_spritelump();  // get next index, may expand and move the table
            spritelump_t * sl = &spritelumps[spritelump_id];  // sprite patch header

            // uint16_t conversion to block sign-extension of signed LE_SWAP16.
            // uint32_t conversion needed for shift
            sl->width = ((uint32_t)((uint16_t)( LE_SWAP16(patch.width) )))<<FRACBITS; // unsigned
            sl->leftoffset = ((int32_t)LE_SWAP16(patch.leftoffset))<<FRACBITS;  // signed
            sl->topoffset = ((int32_t)LE_SWAP16(patch.topoffset))<<FRACBITS;  // signed
            sl->height = ((uint32_t)((uint16_t)( LE_SWAP16(patch.height) )))<<FRACBITS;  // unsigned

#if 0
// [WDJ] see fig_topoffset, in hw_main.c
// This does not adjust to changes in drawmode, leaving objects embedded in floor.
#ifdef HWRENDER
            //BP: we cannot use special trick in hardware mode because feet in ground caused by z-buffer
            if( rendermode != render_soft )
            {
                // topoffset may be negative, use signed compare
                int16_t p_topoffset = LE_SWAP16(patch.topoffset);
                int16_t p_height = (uint16_t)( LE_SWAP16(patch.height) );
                if( p_topoffset>0 && p_topoffset<p_height) // not for psprite
                {
                    // perfect is patch.height but sometime it is too high
                    sl->topoffset =
                       min(p_topoffset+4, p_height)<<FRACBITS;
                }
            }
#endif
#endif

            //----------------------------------------------------

            fnd_lumpnum = lumpnum;
            R_InstallSpriteLump (lumpnum, spritelump_id, frame, rotation_char, false);
            if( frame >= install_frame_cnt )
              install_frame_cnt = frame + 1;

            if (lumpname[6])  // if flipped
            {
                frame = lumpname[6] - 'A';
                rotation_char = lumpname[7];
                R_InstallSpriteLump (lumpnum, spritelump_id, frame, rotation_char, true);
                if( frame >= install_frame_cnt )
                    install_frame_cnt = frame + 1;
            }
        }
    }

    //
    // if no frames found for this sprite, or no additional frames.
    //
    if( install_frame_cnt == 0 || fnd_count == 0 )
    {
        // the first time (which is for the original wad),
        // all sprites should have their initial frames
        // and then, patch wads can replace it
        // we will skip non-replaced sprite frames, only if
        // they have already have been initially defined (original wad)

        //check only after all initial pwads added
        //if (spritedef->numframes == 0)
        //    I_SoftError("R_AddSpriteDefs: no initial frames found for sprite %s\n",
        //             namelist[i]);

        // sprite already has frames, and is not replaced by this wad
        return 0;
    }

    array_srp = SRP_NULL;
    //
    //  some checks to help development
    //
    for (frame = 0 ; frame < install_frame_cnt ; frame++)
    {
        fmp = & sprfrm[ frame ];
        rtp = & sprfrot[((size_t)frame) * NUM_SPRITETMP_ROT];
        if( array_srp < fmp->rotation_pattern )
            array_srp = fmp->rotation_pattern;

        switch( fmp->rotation_pattern )
        {
          case SRP_NULL:
            // no rotations were found for that frame at all
#ifdef DEBUG_CHEXQUEST
            // [WDJ] 4/28/2009 Chexquest
            // [WDJ] not fatal, some wads have broken sprite but still play
            debug_Printf( "R_Init_Sprites: No patches found for %s frame %c \n",
                          sprname, frame+'A');
#else
            // [WDJ] Cannot assume that all frames are present.  Some wads like
            // sleepwalking.wad, only have some frames (SHTGA, SHTGM, ... )
            if( devparm )
            {
                I_SoftError ("R_Init_Sprites: No patches found for %s frame %c\n",
                         sprname, frame+'A');
            }
#endif
            break;

          case SRP_1:
            // only the first rotation is needed
            break;

          case SRP_8:
            // must have all 8 frames
            for (rotation=0 ; rotation<8 ; rotation++)
            {
                // we test the patch lump, or the id lump whatever
                // if it was not loaded the two are -1
                if( ! VALID_LUMP(rtp->pat_lumpnum) )
                {
                    I_SoftError("R_Init_Sprites: Sprite %s frame %c is missing rotation %i\n",
                             sprname, frame+'A', rotation);
                    // Limp, use the last sprite lump read for this sprite.
                    rtp->pat_lumpnum = fnd_lumpnum;
                }
                rtp++;
            }
            break;

#ifdef ROT16
          case SRP_16:
            // must have all 16 frames
            for (rotation=0 ; rotation<16 ; rotation++)
            {
                // we test the patch lump, or the id lump whatever
                // if it was not loaded the two are -1
                if( ! VALID_LUMP(rtp->pat_lumpnum) )
                {
                    I_SoftError("R_Init_Sprites: Sprite %s frame %c is missing rotation %i\n",
                             sprname, frame+'A', rotation);
                    // Limp, use the last sprite lump read for this sprite.
                    rtp->pat_lumpnum = fnd_lumpnum;
                }
                rtp++;
            }
            break;
#endif
        }
    }

    frame_rot = srp_to_num_rot[ array_srp ];
   
    // allocate space for the frames present and copy spritetmp to it
    if( spritedef->numframes                // has been allocated
        && (spritedef->numframes < install_frame_cnt  // more frames are defined
            || spritedef->frame_rot < frame_rot) ) // more rotations are defined
    {
        Z_Free (spritedef->spriteframe);
        Z_Free (spritedef->framerotation);
        spritedef->spriteframe = NULL;
        spritedef->framerotation = NULL;
    }

    // allocate this sprite's frames
    if (spritedef->spriteframe == NULL)
    {
        spritedef->spriteframe =
            Z_Malloc (((size_t)install_frame_cnt) * sizeof(spriteframe_t), PU_STATIC, NULL);
        spritedef->framerotation =
            Z_Malloc (((size_t)install_frame_cnt) * frame_rot * sizeof(sprite_frot_t), PU_STATIC, NULL);
    }

    spritedef->numframes = install_frame_cnt;
    spritedef->frame_rot = frame_rot;
    transfer_from_spritetmp( spritedef, array_srp, frame_rot );

    return fnd_count;
}



//
// Search for sprites replacements in a wad whose names are in namelist
//
//   namelist: is just sprnames, cannot be anything else
//             limited to 0 to NUMSPRITE_DEF, extra are in mapped
void R_AddSpriteDefs (char** namelist, int wadnum)
{
    lumpinfo_t * lumpinfo;
    lumpnum_t  start_ln, end_ln;
    int         i, ln1, ln2;
    int         addsprites;
#ifdef ENABLE_DEH_REMAP
    uint16_t  remap_index = 0;
#endif
    uint16_t  sprindx;

    // scan the lumps,
    //  filling in the frames for whatever is found
    lumpinfo = wadfiles[wadnum]->lumpinfo;
    if( lumpinfo == NULL )
        return;
    
    // find the sprites section in this pwad
    // we need at least the S_END
    // (not really, but for speedup)

    start_ln = W_CheckNumForNamePwad ("S_START",wadnum,0);
    if( ! VALID_LUMP(start_ln) )
        start_ln = W_CheckNumForNamePwad ("SS_START",wadnum,0); //deutex compatib.
    if( ! VALID_LUMP(start_ln) )
    {
        // search frames from start of wad
        ln1 = 0;
    }
    else
    {
        // just after S_START
        ln1 = LUMPNUM( start_ln ) + 1;
    }


    end_ln = W_CheckNumForNamePwad ("S_END",wadnum,0);
    if( ! VALID_LUMP(end_ln) )
        end_ln = W_CheckNumForNamePwad ("SS_END",wadnum,0);     //deutex compatib.
    if( ! VALID_LUMP(end_ln) )
    {
        if (devparm)
            GenPrintf(EMSG_dev, "no sprites in pwad %d\n", wadnum);
        return;
        //I_Error ("R_AddSpriteDefs: S_END, or SS_END missing for sprites "
        //         "in pwad %d\n",wadnum);
    }
    ln2 = LUMPNUM( end_ln );

#ifdef ENABLE_DEH_REMAP
    allocate_lumptrack( ln1, ln2 );  // setup lumptrack
#endif
    
    //
    // scan through lumps, for each sprite, find all the sprite frames
    //
    // [WDJ] Going through known SPR_ names is not effective, as this may be a PWAD,
    // and it may not change any of those.  Also, must handle new names not in any list.

    addsprites = 0;
    for (i=0 ;  ; i++)
    {
        char * sprname;

        if( i < numsprites )
        {
#ifdef ENABLE_DEH_REMAP
            // Only those that are predef for this game mode.
            if( ! is_predef_sprite( i ) )  continue;
#endif
            sprname = namelist[i];
            sprindx = i;
            goto add_this_sprname;
        }
#ifdef ENABLE_DEH_REMAP
        else if( i < 0xFFFE )
        {
            // after all known named sprite
            // Get entry sprite_remap[x]
            sprindx = get_remap_sprite( remap_index ++ );  // 0xFFFF when done
            if( sprindx < 0xFFF1 )
            {
                sprname = (char *) sprite_name( sprindx );
                if( verbose > 1 )
                    GenPrintf( EMSG_debug, "Add Sprite remap: sprindx=%i, sprname=%s\n", sprindx, sprname );
                goto add_this_sprname;
            }
            i = 0xFFFE;  // beware the for loop ++
        }
#endif
        else
        {
            // All lumps should have been installed by now, but wads have bugs.
            for(; ln1 <= ln2; ln1++)  // skip over lumps already processed
            {
                if( mark_lumptrack( ln1 ) == 2 )  // not seen before
                {
                    sprname = lumpinfo[ln1].name;  // use whatever name is in the first lump
                    sprindx = remap_sprite( 0xFFF1, sprname );  // because sprites get remapped
                    GenPrintf( EMSG_warn, "Found an orphan lump: sprname=%s, sprindx=%i\n", sprname, sprindx );
                    if( sprindx == 0xFFFF )
                    {
                        GenPrintf(EMSG_warn, "  not remapped\n");
                    }
                    goto add_this_sprname;
                }
            }
            goto lumps_exhausted;
        }
        continue;

     add_this_sprname:
        {
            // Load sprites from lumps with sprname.
            int fnd_count = R_AddSingleSpriteDef ( sprname, &sprites[sprindx], wadnum, ln1, ln2);
            if( fnd_count )
            {
                if( verbose )
                {
#ifdef ENABLE_DEH_REMAP
                    if( remap_index && (verbose > 1) )
                        GenPrintf( EMSG_debug, "  Found remap sprite: %s\n", sprname );
#endif
                    GenPrintf(EMSG_debug, "Sprite %s: Wad %i, Loaded to %i\n", sprname, wadnum, sprindx );
                }

                // if a new sprite was added (not just replaced)
                addsprites++;
                if (devparm)
                    GenPrintf(EMSG_dev, "sprite %s set in pwad %d\n", sprname, wadnum);//Fab
            }
        }
    }

lumps_exhausted:
    {
        GenPrintf(EMSG_info, "%d sprites added from file %s\n", addsprites, wadfiles[wadnum]->filename);//Fab
        //CONS_Error ("press enter\n");
    }
}



//
// GAME FUNCTIONS
//

// [WDJ] Remove sprite limits. This is the soft limit, the hard limit is twice this value.
CV_PossibleValue_t spritelim_cons_t[] = {
   {128, "128"}, {192,"192"}, {256, "256"}, {384,"384"},
   {512,"512"}, {768,"768"}, {1024,"1024"}, {1536,"1536"},
   {2048,"2048"}, {3072, "3072"}, {4096,"4096"}, {6144, "6144"},
   {8192,"8192"}, {12288, "12288"}, {16384,"16384"},
   {0, NULL} };
consvar_t  cv_spritelim = { "sprites_limit", "512", CV_SAVE, spritelim_cons_t, NULL };

// [WDJ] Remove sprite limits.
static int  vspr_change_delay = 128;  // quick first allocate
static unsigned int  vspr_random = 0x7f43916;
static int  vspr_halfcnt; // count to halfway
  
static int  vspr_count = 0;	// count of sprites in the frame
static int  vspr_needed = 64;     // max over several frames
static int  vspr_max = 0;	// size of array - 1
static vissprite_t*    vissprites = NULL;  // [0 .. vspr_max]
static vissprite_t*    vissprite_last;	   // last vissprite in array
static vissprite_t*    vissprite_p;    // next free vissprite
static vissprite_t*    vissprite_far;  // a far vissprite, can be replaced

static vissprite_t     vsprsortedhead;  // sorted list head (circular linked)


// Call between frames, it does not copy contents, and does not init.
void vissprites_tablesize ( void )
{
    int request;
    // sprite needed over several frames
    if ( vspr_count > vspr_needed )
        vspr_needed = vspr_count;  // max
    else
        vspr_needed -= (vspr_needed - vspr_count) >> 8;  // slow decay
   
    request = ( vspr_needed > cv_spritelim.value )?
              (cv_spritelim.value + vspr_needed)/2  // soft limit
            : vspr_needed;  // under limit
        
    // round-up to avoid trivial adjustments
    request = (request < (256*6))?
              (request + 0x003F) & ~0x003F   // 64
            : (request + 0x00FF) & ~0x00FF;  // 256
    // hard limit
    if ( request > (cv_spritelim.value * 2) )
        request = cv_spritelim.value * 2;
    
    if ( request == (vspr_max+1) )
        return;		// same as existing allocation

    if( vspr_change_delay < INT_MAX )
    {
        vspr_change_delay ++;
    }
    if ( request < vspr_max )
    {
        // decrease allocation
        if ( ( request < cv_spritelim.value )  // once up to limit, stay there
             || ( request > (vspr_max / 4))  // avoid vacillation
             || ( vspr_change_delay < 8192 )  )  // delay decreases
        {
            if (vspr_max <= cv_spritelim.value * 2)  // unless user setting was reduced
                return;
        }
        if ( request < 64 )
             request = 64;  // absolute minimum
    }
    else
    {
        // delay to get max sprites needed for new scene
        // but not too much or is very visible
        if( vspr_change_delay < 16 )  return;
    } 
    vspr_change_delay = 0;
    // allocate
    if ( vissprites )
    {
        free( vissprites );
    }
    do {  // repeat allocation attempt until success
        vissprites = (vissprite_t*) malloc ( sizeof(vissprite_t) * request );
        if( vissprites )
        {
            vspr_max = request-1;
            return;	// normal successful allocation
        }
        // allocation failed
        request /= 2;  // halve the request
    }while( request > 63 );
       
    I_Error ("Cannot allocate vissprites\n");
}


//
// R_Init_Sprites
// Called at program start.
//
void R_Init_Sprites (char** namelist)
{
    int         i;

    vissprites_tablesize();  // initial allocation

#if 1
    // namelist is always sprnames, the length is known.
    numsprites = NUMSPRITES_DEF;
#else
// [WDJ] This does not always work because there could be a NULL in the sprite table.
// Dehacked has done that.  See wotdoom3.wad.
    //
    // count the number of sprite names, and allocate sprites table
    //
    char** check = namelist;
    while (*check != NULL)
        check++;
    numsprites = check - namelist;

    if (!numsprites)
        I_Error ("R_AddSpriteDefs: no sprites in namelist\n");
#endif

    sprites = Z_Malloc(((size_t)numsprites) * sizeof(*sprites), PU_STATIC, NULL);
    memset (sprites, 0, ((size_t)numsprites) * sizeof(*sprites));

    // find sprites in each -file added pwad
    for (i=0; i<numwadfiles; i++)
        R_AddSpriteDefs (namelist, i);

    //
    // now check for skins
    //

    // all that can be before loading config is to load possible skins
    R_Init_Skins ();
    for (i=0; i<numwadfiles; i++)
        R_AddSkins (i);


    //
    // check if all sprites have frames
    //
    /*
    for (i=0; i<numsprites; i++)
         if (sprites[i].numframes<1)
             I_SoftError("R_Init_Sprites: sprite %s has no frames at all\n", sprite_name(i));
    */
    
#ifdef ENABLE_DEH_REMAP
    release_lump_added();
#endif    
}



//
// R_Clear_Sprites
// Called at frame start.
//
void R_Clear_Sprites (void)
{
    vissprites_tablesize();  // re-allocation
    vspr_random += (vspr_count & 0xFFFF0) + 0x010001;  // just keep it changing
    vissprite_last = &vissprites[vspr_max];
    vissprite_p = vissprites;  // first free vissprite
    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;  // init sorted
    vsprsortedhead.scale = FIXED_MAX;  // very near, so it is rejected in farthest search
    vissprite_far = & vsprsortedhead;
    vspr_halfcnt = 0; // force vissprite_far init
    vspr_count = 0;  // stat for allocation
}


//
// R_NewVisSprite
//
static vissprite_t     overflowsprite;

// [WDJ] New vissprite sorted by scale
// Closer sprites get preference in the vissprite list when too many.
// Sorted list is farthest to nearest (circular).
//   scale : the draw scale, representing distance from viewer
//   dist_pri : distance priority, 0..255, dead are low, monsters are high
//   copysprite : copy this sprite
static
vissprite_t* R_NewVisSprite ( fixed_t scale, byte dist_pri,
                              vissprite_t * copysprite )
{
    vissprite_t * vs;
    register vissprite_t * ns;

    vspr_count ++;	// allocation stat
    if (vissprite_p == vissprite_last)  // array full ?
    { 
        unsigned int rn, cnt;
        // array is full
        vspr_random += 0x021019; // semi-random stirring (prime)
        if ( vsprsortedhead.next->scale > scale )
        { 
            // New sprite is farther than farthest sprite in array,
            // even far sprites have random chance of being seen (flicker)
            // Avg (pri=128) monster has 1/16 chance.
            if (((vspr_random >> 8) & 0x7FF) > dist_pri)
                return &overflowsprite;
        }
        // Must remove a sprite to make room.
        // Sacrifice a random sprite from farthest half.
        // Skip a random number of sprites, at least 1.
        // Try for a tapering distance effect.
        rn = (vspr_random & 0x000F) + ((vspr_max - vspr_halfcnt) >> 7);
        for( cnt = 2; ; cnt-- )  // tries to find lower priority
        {
            if( vspr_halfcnt <= 0 ) // halfway count trigger
            {
                // init, or re-init
                vspr_halfcnt = vspr_max / 2;
                vissprite_far = vsprsortedhead.next; // farthest
            }
            vs = vissprite_far;
            rn ++;  // at least 1
            // Move vissprite_far off the sprite that will be removed.
            vspr_halfcnt -= rn;  // count down to halfway
            for( ; rn > 0 ; rn -- )
            {
                vissprite_far = vissprite_far->next; // to nearer sprites
            }
            // Compare priority, but only a few times.
            if( cnt == 0 )  break;
            if( vs->dist_priority <= dist_pri )   break;
        }

        // unlink it so it can be re-linked by distance
        vs->next->prev = vs->prev;
        vs->prev->next = vs->next;
    }
    else
    {
        // still filling up array
        vs = vissprite_p ++;
    }

    // Set links so order is farthest to nearest.
    // Check the degenerate case first and avoid this test in the loop below.
    // Empty list looks to have head as max nearest sprite, so first is farthest.
    if (vsprsortedhead.next->scale > scale)
    {
        // New is farthest, this will happen often because of close preference.
        ns = &vsprsortedhead; // farthest is linked after head
    }
    else
    {
        // Search nearest to farthest.
        // The above farthest check ensures that search will hit something farther.
        ns = vsprsortedhead.prev; // nearest
        while( ns->scale > scale )  // while new is farther
        {
            ns = ns->prev;
        }
    }

    vs->clip_top = NULL;
    vs->clip_bot = NULL;
    // When copied, keep existing clip arrays.  They are still valid limits.

    // ns is farther than new
    // Copy before linking, is easier.
    if( copysprite )
        memcpy( vs, copysprite, sizeof(vissprite_t));

    // link new vs after ns (nearer than ns)
    vs->next = ns->next;
    vs->next->prev = vs;
    ns->next = vs;
    vs->prev = ns;
    vs->dist_priority = dist_pri;

    return vs;
}


// ---- Sprite Drawing

// Drawing constant clip arrays
//  Used for sprite and psprite clipping.
//  Set to last pixel row inside the drawable screen bounds.
//  This makes limit tests easier, do not need to do +1 or -1;
//  Initialized in R_ExecuteSetViewSize (in r_main).
int16_t  clip_screen_top_min[MAXVIDWIDTH];
int16_t  clip_screen_bot_max[MAXVIDWIDTH];

void  set_int16( int16_t * dest, int x1, int x2, int16_t value )
{
    int x;
    for( x = x1; x <= x2; x++ )
       dest[x] = value;
}


//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
// The colfunc_2s function for TM_patch and TM_combine_patch
//
// draw masked global parameters
// clipping array[x], in int screen coord.
int16_t      *  dm_floorclip;
int16_t      *  dm_ceilingclip;

fixed_t         dm_yscale;  // world to fixed_t screen coord
// draw masked column top and bottom, in fixed_t screen coord.
fixed_t         dm_top_patch, dm_bottom_patch;
// window clipping in fixed_t screen coord., set to FIXED_MAX to disable
// to draw, require dm_windowtop < dm_windowbottom
fixed_t         dm_windowtop, dm_windowbottom;
// for masked draw of patch, to form dc_texturemid
fixed_t         dm_texturemid;


// Called by R_RenderMaskedSegRange, R_RenderThickSideRange, R_RenderFog
void R_DrawMaskedColumn ( byte * column_data )
{
    fixed_t     top_post_sc, bottom_post_sc;  // fixed_t screen coord.
#ifdef DEEPSEA_TALL_PATCH
    // [MB (M. Bauerle), from crispy Doom]  Support for DeePsea tall patches, [WDJ]
    int         cur_topdelta = -1;  // [crispy]
#endif

    column_t * column = (column_t*) column_data;
   
    // over all column posts for this column
    for ( ; column->topdelta != 0xff ; )
    {
        // calculate unclipped screen coordinates
        //  for post
#ifdef DEEPSEA_TALL_PATCH
        // [crispy] [MB] [WDJ] DeePsea tall patch.
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
        top_post_sc = dm_top_patch + (dm_yscale * cur_topdelta);
#else
        // Normal patch
        top_post_sc = dm_top_patch + (dm_yscale * column->topdelta);
#endif

        bottom_post_sc = (dm_yscale * column->length)
            + ((dm_bottom_patch == FIXED_MAX) ? top_post_sc : dm_bottom_patch );

// dc_yl in the column drawer, seems to need scaling with scale and draw_yscale.
// It is currently treating patch pixels as exactly the same scale as dc_yl and texturemid.
// It needs to position using scale, and draw patch using yscale.
        // fixed_t to int screen coord.
        dc_yl = (top_post_sc+FRACUNIT-1)>>FRACBITS;
        dc_yh = (bottom_post_sc-1)>>FRACBITS;

        if(dm_windowtop != FIXED_MAX && dm_windowbottom != FIXED_MAX)
        {
          // screen coord. where +y is down screen
          if(dm_windowtop > top_post_sc)
            dc_yl = (dm_windowtop + FRACUNIT - 1) >> FRACBITS;
          if(dm_windowbottom < bottom_post_sc)
            dc_yh = (dm_windowbottom - 1) >> FRACBITS;
        }

        if (dc_yh > dm_floorclip[dc_x])
            dc_yh = dm_floorclip[dc_x];
        if (dc_yl < dm_ceilingclip[dc_x])
            dc_yl = dm_ceilingclip[dc_x];

        // [WDJ] limit to split screen area above status bar,
        // instead of whole screen,
        if (dc_yl <= dc_yh && dc_yl < rdraw_viewheight && dc_yh > 0)  // [WDJ] exclude status bar
        {

#ifdef RANGECHECK_DRAW_LIMITS
    // Temporary check code.
    // Due to better clipping, this extra clip should no longer be needed.
    if( dc_yl < 0 )
    {
        printf( "DrawMasked dc_yl  %i < 0\n", dc_yl );
        dc_yl = 0;
    }
    if ( dc_yh >= rdraw_viewheight )
    {
        printf( "DrawMasked dc_yh  %i > rdraw_viewheight\n", dc_yh );
        dc_yh = rdraw_viewheight - 1;
    }
#endif

#ifdef CLIP2_LIMIT
            //[WDJ] phobiata.wad has many views that need clipping
            if ( dc_yl < 0 )   dc_yl = 0;
            if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
#endif

            dc_source = (byte *)column + 3;
#ifdef DEEPSEA_TALL_PATCH
            // [crispy] Support for DeePsea tall patches
            dc_texturemid = dm_texturemid - (cur_topdelta<<FRACBITS);
            // dc_source = (byte *)column + 3 - cur_topdelta;
#else
            // Normal patch	     
            dc_texturemid = dm_texturemid - (column->topdelta<<FRACBITS);
            // dc_source = (byte *)column + 3 - column->topdelta;
#endif
            fog_col_length = column->length;

            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            colfunc ();
        }
        column = (column_t *)(  (byte *)column + column->length + 4);
    }
}



//
// R_DrawVisSprite
//  dm_floorclip and dm_ceilingclip should also be set.
//
//   dlx1, dlx2 : drawing limits
static void R_DrawVisSprite ( vissprite_t *  vis,
                              int  dlx1,  int  dlx2 )
{
    int        texturecolumn;
    fixed_t    texcol_frac;
    patch_t  * patch;


    //Fab:R_Init_Sprites now sets a wad lump number
    // Use common patch read, all patch in cache have endian fixed.
    patch = W_CachePatchNum (vis->patch_lumpnum, PU_CACHE);

    dc_colormap = vis->colormap;

    // Support for translated and translucent sprites. SSNTails 11-11-2002
    dr_alpha = 0;  // ensure use of translucent normally for all drawers
    if( vis->translucentmap )
    {
        dc_translucentmap = vis->translucentmap;
        if( vis->translucentmap == VIS_SMOKESHADE )
        {
            // Draw smoke
            // shadecolfunc uses 'reg_colormaps'
            colfunc = shadecolfunc;
            // does not use dc_translucentmap, it is a fake colormap
        }
        else if( vis->mobj_flags & MFT_TRANSLATION6 )
        {
            // Player skins
            colfunc = skintranscolfunc;
            // uses dc_translucentmap
            dc_translucent_index = vis->translucent_index;
            dc_skintran = MFT_TO_SKINMAP( vis->mobj_flags ); // skins 1..
        }
        else
        {
            // Shadow/Fuzzy sprite
//        colfunc = fuzzcolfunc;
            colfunc = (vis->mobj_flags & MF_SHADOW)? fuzzcolfunc : transcolfunc;
            // uses dc_translucentmap
            dc_translucent_index = vis->translucent_index;
        }
    }
    else if (vis->mobj_flags & MFT_TRANSLATION6)
    {
        // translate green skin to another color
        colfunc = skincolfunc;
        dc_skintran = MFT_TO_SKINMAP( vis->mobj_flags ); // skins 1..
    }

    if((vis->extra_colormap || view_colormap) && !fixedcolormap)
    {
       // reverse indexing, and change to extra_colormap, default 0
       int lightindex = dc_colormap? (dc_colormap - reg_colormaps) : 0;
       lighttable_t* cm = (view_colormap? view_colormap : vis->extra_colormap->colormap);
       dc_colormap = & cm[ lightindex ];
    }
    if(!dc_colormap)
      dc_colormap = & reg_colormaps[0];

    dc_texheight = 0;  // no wrap repeat

    dm_windowtop = dm_windowbottom = dm_bottom_patch = FIXED_MAX; // disable
    dm_texturemid = vis->texturemid;

    // dc_iscale: fixed_t texture step per pixel, for draw function
    //dc_iscale = abs(vis->tex_x_iscale)>>detailshift;  ???
#ifdef MONSTER_VARY
    dm_yscale = vis->yscale;  // y draw scale
    dc_iscale = FixedDiv (FRACUNIT, dm_yscale);  // col step
    fixed_t top_patch = FixedMul(dm_texturemid, vis->scale);
    dm_top_patch = centeryfrac - top_patch;  // base for dc_yl, which must be actual screen position

    // dc_yl will be screen position, which is relative to centery
    dc_y0 = centery;
    
    if(vis->scale != vis->yscale)  // monster_vary is in effect
    {
        // col draw scale, dist scale
        fixed_t pos_iscale = FixedDiv (FRACUNIT, vis->scale);  // col step
        // move draw reference from centery to sprite 0 (top)
        // because positioning the sprite uses vis->scale, but drawing at sprite size uses vis->yscale.
        dc_y0 = dm_top_patch >> FRACBITS;
        // modify dc_texturemid for position and scale,
        // correct for what rdraw column drawer would have done
        dm_texturemid += (dc_y0 - centery) * pos_iscale;
    }
#else
    // col draw scale is same as dist scale
    dm_yscale = vis->scale;  // col draw scale, dist scale
    dc_iscale = FixedDiv (FRACUNIT, dm_yscale);  // col step
    dm_top_patch = centeryfrac - FixedMul(dm_texturemid, dm_yscale);
#endif

    // texture x at dlx1
    texcol_frac = ((dlx1 - vis->x0) * vis->tex_x_iscale) + vis->tex_x0; // offset starting
    for (dc_x=dlx1 ; dc_x<=dlx2 ; dc_x++, texcol_frac += vis->tex_x_iscale)
    {
        texturecolumn = texcol_frac>>FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= patch->width) {
            // [WDJ] Give msg and don't draw it
            I_SoftError ("R_DrawVisSprite: bad texturecolumn\n");
            return;
        }
#endif

        byte * col_data = ((byte *)patch) + patch->columnofs[texturecolumn];
        R_DrawMaskedColumn( col_data );
    }

    colfunc = basecolfunc;
}




// Split a sprite with a horizontal cut.
// Return the bottom half of the sprite.  The top half of the sprite remains with the original.
//   sprite : the original sprite
//   cut_y  : the screen cut y
//   cut_gz : the world z cut
vissprite_t * split_sprite_horz_cut( vissprite_t * sprite, int cut_y, fixed_t cut_gz )
{
    vissprite_t * bot_sprite = NULL;

    // Make copy of the sprite
    // The copy of vsp writes scale correctly.
    bot_sprite = R_NewVisSprite( sprite->scale, sprite->dist_priority, sprite );

    sprite->cut |= SC_BOTTOM;
    sprite->gz_bot = cut_gz;
   
    bot_sprite->cut |= SC_TOP;
    bot_sprite->gz_top = cut_gz;
   
    // [WDJ] 11/14/2009 clip at window again, fix split sprites corrupt status bar
    sprite->y_bot = (cut_y < rdraw_viewheight)? cut_y : rdraw_viewheight;
#if 1
    // No overlap with top of sprite
    bot_sprite->y_top = cut_y + 1;
#else
    // This is how R_Split_Sprite_over_FFloor had it, with a slight overlap with top.
    bot_sprite->y_top = cut_y - 1;
#endif
   
    if( cut_gz < sprite->mobj_top_z
           && cut_gz > sprite->mobj_bot_z)
    {
        sprite->mobj_bot_z = bot_sprite->mobj_top_z = cut_gz;
    }
    else
    {
        bot_sprite->mobj_bot_z = bot_sprite->gz_bot;
        bot_sprite->mobj_top_z = bot_sprite->gz_top;
    }
   
    return bot_sprite;
}


//
// R_Split_Sprite_over_FFloor
// Makes a separate sprite for each floor in a sector lightlist that it touches.
// These must be drawn interspersed with the ffloor floors and ceilings.
//   sprite : to be split
//   thing  : parent object of the sprite, for flag checking
// Called by R_ProjectSprite
static
void R_Split_Sprite_over_FFloor (vissprite_t* sprite, mobj_t* thing)
{
  int           i;
  int           cut_y;  // where lightheight cuts on screen
  fixed_t       lightheight;
  sector_t*     sector;
  ff_light_t*   ff_light; // lightlist item
  vissprite_t * bot_sprite;

  sector = sprite->sector;

  for(i = 1; i < sector->numlights; i++)	// from top to bottom
  {
    ff_light = &frontsector->lightlist[i];
    lightheight = ff_light->height;
     
    // must be a caster
    if(lightheight >= sprite->gz_top || !(ff_light->caster->flags & FF_CUTSPRITES))
      continue;
    if(lightheight <= sprite->gz_bot)
      return;

    // where on screen the lightheight cut appears
    cut_y = (centeryfrac - FixedMul(lightheight - viewz, sprite->scale)) >> FRACBITS;
    if(cut_y < 0)
            continue;
    if(cut_y > rdraw_viewheight)	// [WDJ] 11/14/2009
            return;
        
    // Found a split! Make a new sprite, copy the old sprite to it, and
    // adjust the heights.  Below the cut is the bot_sprite.
    // Cut the sprite at cut_y on screen, and lightheight in world coord..
    bot_sprite = split_sprite_horz_cut( sprite, cut_y, lightheight );

    if(!(ff_light->caster->flags & FF_NOSHADE))
    {
      lightlev_t  vlight = *ff_light->lightlevel  // visible light 0..255
          + ((ff_light->caster->flags & FF_FOG)? extralight_fog : extralight);

      spritelights =
          (vlight < 0) ? scalelight[0]
        : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
        : scalelight[vlight>>LIGHTSEGSHIFT];

      bot_sprite->extra_colormap = ff_light->extra_colormap;

      if (thing->frame & FF_SMOKESHADE)
        ;
      else
      {
/*        if (thing->frame & FF_TRANSMASK)
          ;
        else if (thing->flags & MF_SHADOW)
          ;*/

        if (fixedcolormap )
          ;
        else if ((thing->frame & (FF_FULLBRIGHT|FF_TRANSMASK)
                  || thing->flags & MF_SHADOW)
                 && !(bot_sprite->extra_colormap && bot_sprite->extra_colormap->fog))
          ;
        else
        {
          int dlit = sprite->xscale>>(LIGHTSCALESHIFT-detailshift);
          if (dlit >= MAXLIGHTSCALE)
            dlit = MAXLIGHTSCALE-1;
          bot_sprite->colormap = spritelights[dlit];
        }
      }
    }
    // Any more cuts will be due to ffloor below this one.
    sprite = bot_sprite;
  }
}


//
// R_ProjectSprite
// Generates a vissprite for a thing, if it might be visible.
//
static void R_ProjectSprite (mobj_t* thing)
{
    fixed_t             tr_x, tr_y;
    fixed_t             tx, tz;

    fixed_t             xscale;
    fixed_t             yscale; //added:02-02-98:aaargll..if I were a math-guy!!!
    fixed_t             iscale;  // x texture step per pixel

    int                 x1, x2;

    sector_t*		thingsector;	 // [WDJ] 11/14/2009
   
    spritedef_t*        sprdef;
    spriteframe_t *     sprframe;
    sprite_frot_t *     sprfrot;
    spritelump_t *      sprlump;  // sprite patch header (no pixels)
    vissprite_t*        vis;
    ff_light_t *        ff_light = NULL;  // lightlist light

    uint16_t            sprite_id = thing->sprite;
    unsigned int        rot, fr;
    byte                flip;

    byte                dist_pri;  // distance priority
#ifdef MONSTER_VARY
    byte		vary_active = 0;
#endif

    angle_t             ang;

    //SoM: 3/17/2000
    // [WDJ] Due to some sprites not matching the info height.
    fixed_t             gz_top;
#ifdef MONSTER_VARY
    fixed_t             gz_bot;
#endif
    int                 thingmodelsec;
    boolean	        thing_has_model;  // has a model, such as water


    // transform the origin point
    tr_x = thing->x - viewx;
    tr_y = thing->y - viewy;

    tz = FixedMul(tr_x,viewcos) + FixedMul(tr_y,viewsin);

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    // aspect ratio stuff :
    xscale = FixedDiv(projection_x, tz);
    yscale = FixedDiv(projection_y, tz);

    tx = FixedMul(tr_x,viewsin) - FixedMul(tr_y,viewcos);

    // too far off the side?
    if (abs(tx)>(tz<<2))
        return;

    // decide which patch to use for sprite relative to player
    if(sprite_id >= numsprites)
    {
#ifdef RANGECHECK
        // [WDJ] Give msg and don't draw it
        I_SoftError ("R_ProjectSprite: invalid sprite number %i\n", sprite_id);
#endif
        return;
    }

    //Fab:02-08-98: 'skin' override spritedef currently used for skin
    if (thing->skin)
        sprdef = &((skin_t *)thing->skin)->spritedef;
    else
        sprdef = &sprites[sprite_id];

    fr = thing->frame & FF_FRAMEMASK;
    if( fr >= sprdef->numframes )
    {
#ifdef RANGECHECK
        // [WDJ] Give msg and don't draw it
        I_SoftError ("R_ProjectSprite %i %s: invalid sprite frame %i\n",
                 sprite_id, sprite_name(sprite_id), fr);
#endif
        return;
    }

    // [WDJ] segfault control in heretic shareware, not all sprites present
    if( (byte*)sprdef->spriteframe < (byte*)0x1000 )
    {
        I_SoftError("R_ProjectSprite %i %s: sprframe ptr NULL\n", sprite_id, sprite_name(sprite_id) );
        return;
    }

    sprframe = get_spriteframe( sprdef, fr );

    if( sprframe->rotation_pattern == SRP_1 )
    {
        // use single rotation for all views
        rot = 0;  //Fab: for vis->patch_lumpnum below
    }
    else
    {
        // choose a different rotation based on player view
        ang = R_PointToAngle(thing->x, thing->y);       // uses viewx,viewy

        if( sprframe->rotation_pattern == SRP_8)
        {
            // 8 direction rotation pattern
            rot = (ang - thing->angle + (unsigned) (ANG45/2) * 9) >> 29;
        }
#ifdef ROT16
        else if( sprframe->rotation_pattern == SRP_16)
        {
            // 16 direction rotation pattern
            rot = (ang - thing->angle + (unsigned) (ANG45/4) * 17) >> 28;
        }
#endif
        else return;
    }

    sprfrot = get_framerotation( sprdef, fr, rot );
    //Fab: [WDJ] spritelump_id is the index
    sprlump = &spritelumps[sprfrot->spritelump_id];  // sprite patch header
    flip = sprfrot->flip;

#ifdef MONSTER_VARY
    // Draw width has variation.
    uint32_t draw_width = sprlump->width; // normal, default
    if( cv_monster_vary.EV )
    {
        // Only for those few monsters that have vary enabled.
        vary_active = thing->vary_index;
        if( vary_active )
        {
            draw_width = width_varied( thing, draw_width );
        }
    }
#endif

    // calculate edges of the shape
    if( flip )
    {
        // [WDJ] Flip offset, as suggested by Fraggle (seen in prboom 2003)
#ifdef MONSTER_VARY
        // tx is only used for edge position.  Actual texture index must be separate.	
        tx -= draw_width - sprlump->leftoffset;
#else
        tx -= sprlump->width - sprlump->leftoffset;
#endif
    }
    else
    {
        // apply offset from sprite lump normally
        tx -= sprlump->leftoffset;
    }
    x1 = (centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    // off the right side?
    if (x1 > rdraw_viewwidth)
        return;

#ifdef MONSTER_VARY
    x2 = ((centerxfrac + FixedMul (tx + draw_width, xscale) ) >>FRACBITS) - 1;
#else
    x2 = ((centerxfrac + FixedMul (tx + sprlump->width, xscale) ) >>FRACBITS) - 1;
#endif

    // off the left side
    if (x2 < 0)
        return;

    //SoM: 3/17/2000: Disregard sprites that are out of view..

    // Sprite scale is same as physical scale.
#ifdef MONSTER_VARY
    fixed_t draw_yscale = yscale;
    if( vary_active )
    {
        // world coord of sprite (feet or cut), may be higher or lower
        fixed_t gz_topoffset = height_varied( thing, sprlump->topoffset );
        fixed_t gz_height = height_varied( thing, sprlump->height );
        gz_top = thing->z + gz_topoffset;
        gz_bot = gz_top - gz_height;
        draw_yscale = height_varied( thing, yscale );
    }
    else
    {
        gz_top = thing->z + sprlump->topoffset;  // world coord of sprite top (head or cut)
        gz_bot = gz_top - sprlump->height;  // world coord of sprite bot (feet or cut)
    }
    fixed_t texturemid = gz_top - viewz;
#else
    gz_top = thing->z + sprlump->topoffset;  // world coord of sprite top (head or cut)
#endif

    thingsector = thing->subsector->sector;	 // [WDJ] 11/14/2009
    if(thingsector->numlights)
    {
      lightlev_t  vlight;
      ff_light = R_GetPlaneLight(thingsector, gz_top);
      vlight = *ff_light->lightlevel;
      if(!( ff_light->caster && (ff_light->caster->flags & FF_FOG) ))
        vlight += extralight;

      spritelights =
          (vlight < 0) ? scalelight[0]
        : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
        : scalelight[vlight>>LIGHTSEGSHIFT];
    }

    thingmodelsec = thingsector->modelsec;
    thing_has_model = thingsector->model > SM_fluid; // water

    if (thing_has_model)   // only clip things which are in special sectors
    {
      sector_t * thingmodsecp = & sectors[thingmodelsec];

      // [WDJ] 4/20/2010  Added some structure and ()
      // [WDJ] Could use viewer_at_water to force view of objects above and
      // below to be seen simultaneously.
      // Instead have choosen to have objects underwater not be seen until
      // viewer_underwater.
      // When at viewer_at_water, will not see objects above nor below the water.
      // As this has some validity in reality, and does not generate HOM,
      // will live with it.  It is transient, and most players will not notice.
      if (viewer_has_model)
      {
          // [WDJ] FakeFlat uses viewz<=floor, and thing used viewz<floor,
          // They both should be the same or else things do not
          // appear when just underwater.
          if( viewer_underwater ?
              (thing->z >= thingmodsecp->floorheight)
              : (gz_top < thingmodsecp->floorheight)
              )
              return;
          // [WDJ] FakeFlat uses viewz>=ceiling, and thing used viewz>ceiling,
          // They both should be the same or else things do not
          // appear when just over ceiling.
          if( viewer_overceiling ?
              ((gz_top < thingmodsecp->ceilingheight) && (viewz > thingmodsecp->ceilingheight))
              : (thing->z >= thingmodsecp->ceilingheight)
              )
              return;
      }
    }

    // Store information in a vissprite.
    dist_pri = thing->height >> 16;  // height (fixed_t), 0..120, 56=norm.
    if( thing->flags & MF_MISSILE )
        dist_pri += 60;  // missiles are important
    else
    {
        // CORPSE may not be MF_SHOOTABLE.
        if( thing->flags & MF_CORPSE )
            dist_pri >>= 2;  // corpse has much less priority
        else if( thing->flags & (MF_SHOOTABLE|MF_COUNTKILL) )
            dist_pri += 20;  // monsters are important too
    }

    // yscale is the distance scale
    vis = R_NewVisSprite ( yscale, dist_pri, NULL );
    // do not waste time on the massive number of sprites in the distance
    if( vis == &overflowsprite )  // test for rejected, or too far
        return;

    // [WDJ] Only pass water models, not colormap model sectors
    vis->modelsec = thing_has_model ? thingmodelsec : -1 ; //SoM: 3/17/2000
    vis->mobj_flags = (thing->flags & MF_SHADOW) | (thing->tflags & MFT_TRANSLATION6);
    vis->mobj = thing;
    // world coord
    vis->mobj_x = thing->x;
    vis->mobj_y = thing->y;
 //   vis->mobj_height = thing->height;  // unused
    vis->mobj_bot_z = thing->z;  // world coord of thing bot (feet or cut)
    vis->mobj_top_z = thing->z + thing->height;  // world coord of thing top (head or cut)
    vis->gz_top = gz_top;  // word coord of sprite top (head or cut)
#ifdef MONSTER_VARY
    vis->gz_bot = gz_bot;  // world coord of sprite bot (feet or cut)
    vis->texturemid = texturemid;
#else
    vis->gz_bot = gz_top - sprlump->height;  // world coord of sprite bot (feet or cut)
    vis->texturemid = vis->gz_top - viewz;
#endif
    // foot clipping
    if(thing->flags2&MF2_FEETARECLIPPED
       && thing->z <= thingsector->floorheight)
    { 
         vis->texturemid -= 10*FRACUNIT;
    }

    vis->x1 = (x1 < 0) ? 0 : x1;
    vis->x2 = (x2 >= rdraw_viewwidth) ? rdraw_viewwidth-1 : x2;
    vis->xscale = xscale; // SoM: 4/17/2000
#ifdef MONSTER_VARY
    vis->yscale = draw_yscale;  // column draw scale
#endif
    vis->scale = yscale;  // distance scale
    // screen location of top and bot
    vis->y_top = (centeryfrac - FixedMul(vis->gz_top - viewz, yscale)) >> FRACBITS;
    vis->y_bot = (centeryfrac - FixedMul(vis->gz_bot - viewz, yscale)) >> FRACBITS;
    vis->cut = SC_NONE;	// none, false

#ifdef MONSTER_VARY
    if( vary_active )
    {
        fixed_t draw_xscale = width_varied( thing, xscale);
        iscale = FixedDiv (FRACUNIT, draw_xscale);
    }
    else
    {
        iscale = FixedDiv (FRACUNIT, xscale);
    }
#else
    iscale = FixedDiv (FRACUNIT, xscale);
#endif

    vis->x0 = x1;  // texture alignment, can be < 0
    if (flip)
    {
        vis->tex_x_iscale = -iscale;
        vis->tex_x0 = sprlump->width - 1;  // fixed_t
    }
    else
    {
        vis->tex_x_iscale = iscale;
        vis->tex_x0 = 0;
    }

    // [WDJ] The patch_lumpnum is the (file,lump) used to load the lump from the file.
    // The spritelump_id is the index to the sprite lump table.
    vis->patch_lumpnum = sprfrot->pat_lumpnum;

    vis->sector = thingsector;
    vis->extra_colormap = (ff_light)?
        ff_light->extra_colormap
        : thingsector->extra_colormap;

//
// determine the colormap (lightlevel & special effects)
//
    vis->translucentmap = NULL;
    vis->translucent_index = 0;
    
    // specific translucency
    if (thing->frame & FF_SMOKESHADE)
    {
        // Draw smoke
        // not really a colormap ... see R_DrawVisSprite
//        vis->colormap = VIS_SMOKESHADE; 
        vis->colormap = NULL;
        vis->translucentmap = VIS_SMOKESHADE; 
    }
    else
    {
        if (thing->frame & FF_TRANSMASK)
        {
            vis->translucent_index = (thing->frame&FF_TRANSMASK)>>FF_TRANSSHIFT;
            vis->translucentmap = & translucenttables[ FF_TRANSLU_TABLE_INDEX(thing->frame) ];
        }
        else if (thing->flags & MF_SHADOW)
        {
            // actually only the player should use this (temporary invisibility)
            // because now the translucency is set through FF_TRANSMASK
            vis->translucent_index = TRANSLU_hi;
            vis->translucentmap = & translucenttables[ TRANSLU_TABLE_hi ];
        }

    
        if (fixedcolormap )
        {
            // fixed map : all the screen has the same colormap
            //  eg: negative effect of invulnerability
            vis->colormap = fixedcolormap;
        }
        else if( ( (thing->frame & (FF_FULLBRIGHT|FF_TRANSMASK))
                   || (thing->flags & MF_SHADOW) )
                 && (!vis->extra_colormap || !vis->extra_colormap->fog)  )
        {
            // full bright : goggles
            vis->colormap = & reg_colormaps[0];
        }
        else
        {
            // diminished light
            int index = xscale>>(LIGHTSCALESHIFT-detailshift);

            if (index >= MAXLIGHTSCALE)
                index = MAXLIGHTSCALE-1;

            vis->colormap = spritelights[index];
        }
    }

    if(thingsector->numlights)
        R_Split_Sprite_over_FFloor(vis, thing);
}




//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites (sector_t* sec, int lightlevel)
{
    mobj_t*   thing;

    if (rendermode != render_soft)
        return;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;

    // Well, now it will be done.
    sec->validcount = validcount;

    if(!sec->numlights)  // otherwise see ProjectSprite
    {
      if(sec->model < SM_fluid)   lightlevel = sec->lightlevel;

      lightlev_t  vlight = lightlevel + extralight;

      spritelights =
          (vlight < 0) ? scalelight[0]
        : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
        : scalelight[vlight>>LIGHTSEGSHIFT];
    }

    // Handle all things in sector.
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
    {
        if((thing->flags2 & MF2_DONTDRAW)==0)
            R_ProjectSprite (thing);
    }
}


const int PSpriteSY[NUMWEAPONS] =
{
     0,             // staff
     5*FRACUNIT,    // goldwand
    15*FRACUNIT,    // crossbow
    15*FRACUNIT,    // blaster
    15*FRACUNIT,    // skullrod
    15*FRACUNIT,    // phoenix rod
    15*FRACUNIT,    // mace
    15*FRACUNIT,    // gauntlets
    15*FRACUNIT     // beak
};

//
// R_DrawPSprite, Draw one player sprite.
//
// Draw parts of the viewplayer weapon
static
void R_DrawPSprite (pspdef_t* psp)
{
    fixed_t             tx;
    int                 x1, x2;
    spritedef_t*        sprdef;
//    spriteframe_t*      sprframe;
    sprite_frot_t *     sprfrot;
    spritelump_t*       sprlump;
    vissprite_t*        vis;
    vissprite_t         avis;

    // [WDJ] 11/14/2012 use viewer variables, which will be for viewplayer
    state_t * p_state = psp->state;
    uint16_t  p_sprite_id = p_state->sprite;

    // decide which patch to use
#ifdef RANGECHECK
    if ( p_sprite_id >= numsprites) {
        // [WDJ] Give msg and don't draw it, (** Heretic **)
        I_SoftError ("R_DrawPSprite: invalid sprite number %i\n", p_sprite_id );
        return;
    }
#endif

    sprdef = &sprites[p_sprite_id];
    uint16_t fr = p_state->frame & FF_FRAMEMASK;

#ifdef RANGECHECK
    if ( fr >= sprdef->numframes) {
        // [WDJ] Give msg and don't draw it
        I_SoftError ("R_DrawPSprite: invalid sprite frame %i : %i for %s\n",
                 p_sprite_id, fr, sprite_name(p_sprite_id) );
        return;
    }
#endif
   
    // [WDJ] segfault control in heretic shareware, not all sprites present
    if( (byte*)sprdef->spriteframe < (byte*)0x1000 )
    {
        I_SoftError("R_DrawPSprite: sprframe ptr NULL for state %d\n", p_state );
        return;
    }

//    sprframe = get_spriteframe( sprdef, fr );

    // use single rotation for all views
    sprfrot = get_framerotation( sprdef, fr, 0 );

    //Fab: see the notes in R_ProjectSprite about spritelump_id, pat_lumpnum
    sprlump = &spritelumps[sprfrot->spritelump_id];  // sprite patch header

    // calculate edges of the shape

    //added:08-01-98:replaced mul by shift
    tx = psp->sx-((BASEVIDWIDTH/2)<<FRACBITS); //*FRACUNITS);

    //added:02-02-98:spriteoffset should be abs coords for psprites, based on
    //               320x200
#if 0
    // [WDJ] I don't think that weapon sprites need flip, but prboom
    // and prboom-plus are still supporting it, so maybe there are some.
    // There being one viewpoint per offset, probably do not need this.
    if( sprfrot->flip )
    {
        // debug_Printf("Player weapon flip detected!\n" );
        tx -= sprlump->width - sprlump->leftoffset;  // Fraggle's flip offset
    }
    else
    {
        // apply offset from sprite lump normally
        tx -= sprlump->leftoffset;
    }
#else
    tx -= sprlump->leftoffset;
#endif
    x1 = (centerxfrac + FixedMul (tx,pspritescale) ) >>FRACBITS;

    // off the right side
    if (x1 > rdraw_viewwidth)
        return;

    tx += sprlump->width;
    x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vis = &avis;
    vis->mobj_flags = 0;
    // [Arcade] The view layout, not cv_splitscreen.  This base centre is a
    // hand tuning for the two-view split, where the weapon is drawn at the
    // *full screen* y scale inside a half height view (pspriteyscale keeps
    // vid.height while rdraw_viewwidth is unhalved), so it has to be shifted
    // up to sit right.  A 2x2 cell is not that case: rdraw_viewwidth is
    // halved with it, so the weapon is scaled proportionally and wants the
    // ordinary BASEYCENTER, exactly as a full screen view does.
    //
    // Keyed on the cvar it was wrong twice over: cv_localplayers can put four
    // views on screen with the cvar off, and the Multiplayer menu sets the
    // cvar for a game that then runs as a 2x2 -- which shifted the weapon up
    // by 20 base units, about 38px of a 384px cell, leaving the wrist ending
    // in mid-air.  The hardware renderer already draws this distinction the
    // same way (atransform.splitscreen = (D_NumViews() == 2), and its own
    // weapon nudge).
    // [Arcade] D_View_Squash() == 1 is exactly "half height, full width".
    // A side-by-side view is full height and wants the ordinary centre, the
    // same as a 2x2 cell and a full screen.
    vis->texturemid = (D_View_Squash() == 1) ?
        (120<<(FRACBITS)) + FRACUNIT/2 - (psp->sy - sprlump->topoffset)
        : (BASEYCENTER<<FRACBITS) + FRACUNIT/2 - (psp->sy - sprlump->topoffset);

    if( EN_heretic_hexen )
    {
        if( rdraw_viewheight == vid.height
            || (!cv_scalestatusbar.EV && vid.dupy>1) )
            vis->texturemid -= PSpriteSY[viewplayer->readyweapon];
    }

    //vis->texturemid += FRACUNIT/2;

    vis->x1 = (x1 < 0) ? 0 : x1;
    vis->x2 = (x2 >= rdraw_viewwidth) ? rdraw_viewwidth-1 : x2;
    vis->scale = pspriteyscale;  //<<detailshift;
#ifdef MONSTER_VARY
    vis->yscale = pspriteyscale; // same scale
#endif

    vis->x0 = x1;  // texture alignment, can be < 0
    if( sprfrot->flip )
    {
        vis->tex_x_iscale = -pspriteiscale;
        vis->tex_x0 = sprlump->width - 1;  // fixed_t
    }
    else
    {
        vis->tex_x_iscale = pspriteiscale;
        vis->tex_x0 = 0;
    }

    //Fab: see above for more about spritelump_id,lumppat
    vis->patch_lumpnum = sprfrot->pat_lumpnum;
    vis->translucentmap = NULL;
    vis->translucent_index = 0;
    if (viewplayer->mo->flags & MF_SHADOW)      // invisibility effect
    {
        vis->colormap = NULL;   // use translucency

        // in Doom2, it used to switch between invis/opaque the last seconds
        // now it switch between invis/less invis the last seconds
        if (viewplayer->powers[pw_invisibility] > 4*TICRATE
                 || viewplayer->powers[pw_invisibility] & 8)
        {
            vis->translucent_index = TRANSLU_hi;
            vis->translucentmap = & translucenttables[ TRANSLU_TABLE_hi ];
        }
        else
        {
            vis->translucent_index = TRANSLU_med;
            vis->translucentmap = & translucenttables[ TRANSLU_TABLE_med ];
        }
    }
    else if (fixedcolormap)
    {
        // fixed color
        vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = & reg_colormaps[0]; // [0]
    }
    else
    {
        // local light
        vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }

    if(viewer_sector->numlights)
    {
      lightlev_t  vlight;  // 0..255
      ff_light_t * ff_light =
        R_GetPlaneLight(viewer_sector, viewmobj->z + (41 << FRACBITS));
      vis->extra_colormap = ff_light->extra_colormap;
      vlight = *ff_light->lightlevel + extralight;

      spritelights =
          (vlight < 0) ? scalelight[0]
        : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
        : scalelight[vlight>>LIGHTSEGSHIFT];

      vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }
    else
      vis->extra_colormap = viewer_sector->extra_colormap;

    R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
// Draw the viewplayer weapon, render_soft.
void R_DrawPlayerSprites (void)
{
    lightlev_t  vlight;  // visible light 0..255
    pspdef_t*   psp;

    int kikhak_centery;

    // rendermode == render_soft
    // [WDJ] 11/14/2012 use viewer variables for viewplayer

    // get light level
    if(viewer_sector->numlights)
    {
      ff_light_t * ff_light =
        R_GetPlaneLight(viewer_sector, viewmobj->z + viewmobj->info->height);
      vlight = *ff_light->lightlevel + extralight;
    }
    else
      vlight = viewer_sector->lightlevel + extralight;

    spritelights =
        (vlight < 0) ? scalelight[0]
      : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
      : scalelight[vlight>>LIGHTSEGSHIFT];

    // clip to screen bounds
    dm_floorclip = clip_screen_bot_max;  // clip at bottom of screen
    dm_ceilingclip = clip_screen_top_min;  // clip at top of screen

    //added:06-02-98: quickie fix for psprite pos because of freelook
    kikhak_centery = centery;
    centery = centerypsp;             //for R_DrawColumn

    // [Arcade] Hang the weapon from the bottom of a side-by-side view.
    //
    // pspriteyscale is derived from the view *width*
    // ( (vid.height * rdraw_viewwidth / vid.width) / BASEVIDHEIGHT , r_main.c ),
    // which is right only while the view's height is proportional to its
    // width -- a full screen or a 2x2 cell.  A side-by-side view is half
    // width and *full* height, so the weapon is drawn at half scale while
    // centerypsp still sits at half the full height, and it ends up floating
    // a quarter of the screen above the floor.  Measured at 320x200, weapon
    // top..bottom against the bottom of the view:
    //
    //     single        320x200 view   138..200   of 200   correct
    //     2x2           160x100 view    69..100   of 100   correct
    //     side by side  160x200 view   119..150   of 200   50px short
    //
    // Anchoring it to the view's bottom instead of its middle reproduces the
    // full screen result exactly: the weapon is designed to hang from
    // BASEYCENTER, so that is where the bottom of the view has to be.  For a
    // full screen and a 2x2 cell this is arithmetically the same value
    // centerypsp already holds, which is why it is applied only where it is
    // not -- there is no need to risk the cases that are already right.
    //
    // The stacked halves are deliberately left alone.  They are the opposite
    // error -- full scale in a half height view, so the weapon overhangs the
    // bottom by 30px and is clipped -- and they carry a hand tuned centre of
    // 120 in R_DrawPSprite.  The same formula would seat them properly too,
    // but it would also show the whole weapon in a half height view instead of
    // the cropped one that has been shipping, which is a change to how the
    // cabinet looks rather than a fix.  See docs/arcade/multiplayer-views.md.
    if( D_View_Squash() == 2 )
        centery = rdraw_viewheight
                  - ( FixedMul( BASEYCENTER<<FRACBITS, pspriteyscale ) >> FRACBITS );

    centeryfrac = centery<<FRACBITS;  //for R_DrawVisSprite

#ifdef MONSTER_VARY
    dc_y0 = centery;
#endif
    
    // add all active psprites
    int i;
    for (i=0, psp=viewplayer->psprites;
         i<NUMPSPRITES;
         i++,psp++)
    {
        if (psp->state)
            R_DrawPSprite (psp);
    }

    //added:06-02-98: oooo dirty boy
    centery = kikhak_centery;
    centeryfrac = centery<<FRACBITS;
}



// =======
//  Corona

static patch_t *  corona_patch = NULL;
static spritelump_t  corona_sprlump;

// One or the other.
#ifdef ENABLE_DRAW_ALPHA
#else
#define ENABLE_COLORED_PATCH
// [WDJ] Wad patches usable as corona.
// It is easier to recolor during drawing than to pick one of each.
// Only use patches that are likely to be round in every instance (teleport fog is often not round).
// Corona alternatives list.
const char * corona_name[] = {
  "CORONAP",  // patch version of corona, from legacy.wad
  "PLSEA0", "PLSSA0", "APBXA0",  // Doom1, Doom2
//  "AMB2A0", "PUF2A0", "FX01A0",  // Heretic
  "PUF2A0", "FX01A0",  // Heretic
  NULL
};
#endif


#ifdef ENABLE_COLORED_PATCH
static int corona_patch_size = 0;

typedef struct {
    RGBA_t  corona_color;
    patch_t * colored_patch;  // Z_Malloc
} corona_image_t;

static corona_image_t  corona_image[NUMLIGHTS];
#endif
   
// Also does release, after corona_patch_size is set.
static
void init_corona_data( void )
{
#ifdef ENABLE_DRAW_ALPHA
#endif

#ifdef ENABLE_COLORED_PATCH
    int i;
    for( i = 0; i< NUMLIGHTS; i++ )
    {
        if( corona_patch_size )
        {
            if( corona_image[i].colored_patch )
                Z_Free( corona_image[i].colored_patch );
        }
        corona_image[i].corona_color.rgba = 0;
        corona_image[i].colored_patch = NULL;
    }
#endif
}
   
#ifdef ENABLE_COLORED_PATCH
static
void setup_colored_corona( corona_image_t * ccp, RGBA_t corona_color )
{
    byte  colormap[256];
    patch_t * pp;

    // when draw alpha is intense cannot have faint color in corona image
    int alpha = (255 + corona_color.s.alpha) >> 1;
    int za = (255 - alpha);
    int c;

    // A temporary colormap
    for( c = 0; c<256; c++ )
    {
        // make a colormap that is mostly of the corona color
        RGBA_t rc = pLocalPalette[ reg_colormaps[c] ];
        int r = (corona_color.s.red * alpha + rc.s.red * za) >> 8;
        int g = (corona_color.s.green * alpha + rc.s.green * za) >> 8;
        int b = (corona_color.s.blue * alpha + rc.s.blue * za) >> 8;
        colormap[c] = NearestColor( r, g, b );
    }
    
    // Allocate a copy of the corona patch.
    ccp->corona_color = corona_color;
    if( ccp->colored_patch )
        Z_Free( ccp->colored_patch );
    pp = Z_Malloc( corona_patch_size, PU_STATIC, NULL );
    ccp->colored_patch = pp;
    memcpy( pp, corona_patch, corona_patch_size );

    // Change the corona copy to the corona color.
    for( c=0; c < corona_patch->width; c++ )
    {
        column_t * cp = (column_t*)((byte*)pp + pp->columnofs[c]);
        while( cp->topdelta != 0xff )  // end of posts
        {
            byte * s = (byte*)cp + 3;
            int count = cp->length;
            while( count-- )
            {
                *s = colormap[*s];
                s++;
            }
            // next source post, adv by (length + 2 byte header + 2 extra bytes)
            cp = (column_t *)((byte *)cp + cp->length + 4);
        }
    }
}

static
patch_t * get_colored_corona( int sprite_light_num )
{
    corona_image_t * cc = & corona_image[ sprite_light_num ];
    RGBA_t corona_color = sprite_light[ sprite_light_num ].corona_color;

    if( cc->corona_color.rgba != corona_color.rgba || cc->colored_patch == NULL )
    {
        setup_colored_corona( cc, corona_color );
    }

    return  cc->colored_patch;
}
#endif
   


// Called by SCR_SetMode
void R_Load_Corona( void )
{
#ifdef ENABLE_COLORED_PATCH
    lumpnum_t  lumpid;
#endif

    Setup_sprite_light( cv_monball_light.EV );
   
    // must call at least once, before setting corona_patch_size
    init_corona_data();
   
#ifdef HWRENDER   
    if( rendermode != render_soft )
    {
        return;
    }
#endif

#ifdef ENABLE_DRAW_ALPHA
    if( ! corona_patch )
    {
        pic_t * corona_pic = (pic_t*) W_CachePicName( "corona", PU_STATIC );
        if( corona_pic )
        {
            // Z_Malloc
            // The corona pic is INTENSITY_ALPHA, bytepp=2, blank=0
            corona_patch = (patch_t*) R_Create_Patch( corona_pic->width, corona_pic->height,
                /*SRC*/    TM_row_image, & corona_pic->data[0], 2, 1, 0,
                /*DEST*/   TM_patch, CPO_blank_trim, NULL );
            Z_ChangeTag( corona_patch, PU_STATIC );
            corona_patch->leftoffset += corona_pic->width/2;
            corona_patch->topoffset += corona_pic->height/2;
            // Do not need the corona pic_t anymore
            Z_Free( corona_pic );
            goto setup_corona;
        }
    }
#endif

#ifdef ENABLE_COLORED_PATCH
    if( ! corona_patch )
    {
        // Find first valid patch in corona_name list
        const char ** namep = &corona_name[0];
        while( *namep )
        {
            lumpid = W_Check_Namespace( *namep, LNS_patch );
            if( VALID_LUMP(lumpid) )  goto setup_corona;
            namep++;
        }
    }
#endif
    return; // fail
   
setup_corona :
#ifdef ENABLE_COLORED_PATCH
    // setup the corona support
    corona_patch_size = W_LumpLength( lumpid );
    corona_patch = W_CachePatchNum( lumpid, PU_STATIC );
#endif

    // The patch endian conversion is already done.
    corona_sprlump.width = corona_patch->width << FRACBITS;
    corona_sprlump.height = corona_patch->height << FRACBITS;
    corona_sprlump.leftoffset = corona_patch->leftoffset << FRACBITS;
    corona_sprlump.topoffset = corona_patch->topoffset << FRACBITS;
    return;
}


void R_Release_Corona( void )
{
    init_corona_data( );  // does release too

    if( corona_patch )
    {
        Z_Free( corona_patch );
        corona_patch = NULL;
    }
}

// Propotional fade of corona from Z1 to Z2
#define  Z1  (250.0f)
#define  Z2  ((255.0f*8) + 250.0f)

#ifdef SPDR_CORONAS
// --------------------------------------------------------------------------
// coronas lighting with the sprite
// --------------------------------------------------------------------------

// corona state
spr_light_t  * corona_lsp = NULL;
fixed_t   corona_x0, corona_x1, corona_x2;
fixed_t   corona_xscale, corona_yscale;
float     corona_size;
byte      corona_alpha;
byte      corona_bright; // used by software draw to brighten active light sources
byte      corona_index;  // corona_lsp index
byte      corona_draw = 0;  // 1 = before sprite, 2 = after sprite

byte spec_dist[ 16 ] = {
  10,  // SPLT_unk
  35,  // SPLT_rocket
  20,  // SPLT_lamp
  45,  // SPLT_fire
   0, 0, 0, 0, 0, 0, 0, 0,
  60,  // SPLT_light
  30,  // SPLT_firefly
  80,  // SPLT_random
  80,  // SPLT_pulse
};

typedef enum {
   FADE_FAR = 0x01,
   FADE_NEAR = 0x02
} sprite_corona_fade_e;
   
#define  NUM_FIRE_PATTERN  64
static  int8_t  fire_pattern[ NUM_FIRE_PATTERN ];
static  byte  fire_pattern_tic[ NUM_FIRE_PATTERN ];

#define  NUM_RAND_PATTERN  32
static  byte  rand_pattern_cnt[ NUM_RAND_PATTERN ];
static  byte  rand_pattern_state[ NUM_RAND_PATTERN ];
static  byte  rand_pattern_tic[ NUM_RAND_PATTERN ];

//  sprnum : sprite number
//
//  Return: corona_index, corona_lsp
//  Return NULL when no draw.
spr_light_t *  Sprite_Corona_Light_lsp( int sprnum, state_t * sprstate )
{
    spr_light_t  * lsp;
   
    // Sprite explosion, light substitution
    byte li = LT_NOLIGHT;
    if( sprnum < NUMSPRITES_DEF ) // known sprites
        li = sprite_light_ind[sprnum];
    if( (sprstate >= &states[S_EXPLODE1]
         && sprstate <= &states[S_EXPLODE3])
     || (sprstate >= &states[S_FATSHOTX1]
         && sprstate <= &states[S_FATSHOTX3]))
    {
        li = LT_ROCKETEXP;
    }

    corona_index = li;
    if( li == LT_NOLIGHT )
      return NULL;
   
    lsp = &sprite_light[li];
    corona_lsp = lsp;

    return lsp;
}

//  lsp : sprite light
//  cz : distance to corona
//
//  Return: corona_alpha, corona_size
//  Return 0 when no draw.
byte  Sprite_Corona_Light_fade( spr_light_t * lsp, float cz, int objid )
{
    float  relsize;
    uint16_t  type, cflags;
    byte   fade;
    unsigned int index, v;

    // Objects which emit light.
    type = lsp->impl_flags & SPLT_type_field;  // working type setting
    cflags = lsp->splgt_flags;
    corona_alpha = lsp->corona_color.s.alpha;
    corona_bright = 0;

    // Update flagged by corona setting change, and fragglescript settings.
    if( lsp->impl_flags & SLI_changed )
    {
        lsp->impl_flags &= ~SLI_changed;

        // [WDJ] Fixes must be determined here because Phobiata and other wads,
        // do not set all the dependent fields at one time.
        // They never set some fields, like type, at all.

        type = cflags & SPLT_type_field;  // table or fragglescript setting

        if( cv_corona.EV == 20 )  // Old
        {
            // Revert the new tables to use
            // only that flags that existed in Old.
            cflags &= (SPLGT_corona|SPLGT_dynamic);
            if( type != SPLT_rocket )
            {
               if( cflags & SPLGT_dynamic )
                  type = SPLT_lamp;  // closest to old SPLGT_light
               else
                  type = SPLT_unk;  // corona only
            }
        }
        else
       
        // We have no way of determining the intended version compatibility.  This limits
        // the characteristics that we can check.
        // Some older wads just used the existing corona without setting the type.
        // The default type of some of the existing corona have changed to use the new
        // corona types for ordinary wads, version 1.47.3.
        if( (lsp->impl_flags & SLI_corona_set)  // set by fragglescript
            && ( !(lsp->impl_flags & SLI_type_set) || (type == SPLT_unk) ) )
        {
            // Correct corona settings made by older wads, such as Phobiata, and newmaps.
            // Has the old default type, or type was never set.
#if 0
            // In the original code, the alpha from the corona color was ignored,
            // even though it was set in the tables.  Instead the draw code used 0xff.
            if( corona_alpha == 0 )
                corona_alpha = lsp->corona_color.s.alpha = 0xff; // previous default
#endif
 
            // Refine some of the old wad settings, to use new capabilities correctly.
            // Check for Phobiata and newmaps problems.
            if( corona_alpha > 0xDF )
            {
                // Default radius is 20 to 120.
                // Phobiata flies have a radius of 7
                if( lsp->corona_radius < 10.0f )
                {
                    // newmaps and phobiata firefly
                    type = SPLT_light;
                }
                else if( lsp->corona_radius < 80.0f )
                {
                    // torches
                    type = SPLT_lamp;
                }
            }
        }
        // update the working type
        lsp->impl_flags = (lsp->impl_flags & ~SPLT_type_field) | type;
    }

    if( (type == SPLT_unk) && !(cflags & SPLGT_corona) )
        goto no_corona;  // no corona set

    if( corona_alpha < 3 )
        goto no_corona;  // too faint to see, effectively off

    if( cv_corona.EV == 20 )  // Old
    {
        // alpha settings were ignored
        corona_alpha = 0xff;
    }
    else if( cv_corona.EV == 16 )  // Bright
    {
        corona_bright = 20;  // brighten the default cases
        corona_alpha = (((int)corona_alpha * 3) + 255) >> 2; // +25%
    }
    else if( cv_corona.EV == 14 )  // Dim
    {
        corona_alpha = ((int)corona_alpha * 3) >> 2; // -25%
    }
    else if( cv_corona.EV <= 2 )  // Special, Most
    {
        int spec = spec_dist[type>>4];
       
        if( lsp->impl_flags & SLI_corona_set )  // set by wad
            spec <<= 2;
       
        if( cv_corona.EV == 2 )  // Most
        {
            // Must do this before any flicker modifications, or else they blink.
            if( corona_alpha < 40 )  // ignore the dim corona
                goto no_corona;
            if( corona_alpha + spec + Z1 < cz )
                goto no_corona;  // not close enough
        }
        else
        {
            if( (spec < 33) && ( cz > (Z1+Z2)/2 ) )
                goto no_corona; // not special enough
            if( corona_alpha < 20 )  // ignore the dim corona
                goto no_corona;
        }
    }
   
    relsize = 1.0f;
    fade = FADE_FAR | FADE_NEAR;
   
    // Each of these types has a corona.
    switch( type )
    {
      case SPLT_unk: // corona only
        // object corona
        relsize = ((cz+60.0f)/100.0f);
        break;
      case SPLT_rocket: // flicker
        // svary the alpha
        relsize = ((cz+60.0f)/100.0f);
        corona_alpha = 7 + (A_Random()>>1);
        corona_bright = 128;
        break;
      case SPLT_lamp:  // lamp with a corona
        // lamp corona
        relsize = ((cz+120.0f)/950.0f);
        corona_bright = 40;
        break;
      case SPLT_fire: // slow flicker, torch
        // torches
        relsize = ((cz+120.0f)/950.0f);
        index = objid & (NUM_FIRE_PATTERN - 1);  // obj dependent
        if( fire_pattern_tic[ index ] != gametic )
        {
            fire_pattern_tic[ index ] = gametic;
            if( A_Random() > 35 )
            {
                register int r = A_Random();
                r = ((r - 128) >> 3) + fire_pattern[index];
                if( r > 50 )  r = 40;
                else if( r < -50 )  r = -40;
                fire_pattern[index] = r;
            }
        }
        v = (int)corona_alpha + (int)fire_pattern[index];
        if( v > 255 )  v = 255;
        if( v < 4 )    v = 4;
        corona_alpha = v;
        corona_bright = 45;
        break;
      case SPLT_light: // no corona fade
        // newmaps and phobiata firefly
        // dimming with distance
        relsize = ((cz+120.0f)/950.0f);
#if 0
        if( ( cz < Z1 ) & ((lsp->splgt_flags & SPLGT_source) == 0 ))
        {
            // Fade corona partial to 0 when get too close
            corona_alpha = (int)(( (float)corona_alpha * corona_alpha + (255 - corona_alpha) * (corona_alpha * cz / Z1)) / 255.0f);
        }
#endif
        // Version 1.42 had corona_alpha = 0xff
        corona_bright = 132;
        fade = FADE_FAR;
        break;
      case SPLT_firefly: // firefly blink, un-synch
        // lower 6 bits gives a repeat rate of 1.78 seconds
        if( ((gametic + objid) & 0x003F) < 0x20 )   // obj dependent phase
          goto no_corona; // blink off
        fade = FADE_FAR;
        break;
      case SPLT_random: // random LED, un-synch
        index = objid & (NUM_RAND_PATTERN-1);   // obj dependent counter
        if( rand_pattern_tic[ index ] != gametic )
        {
            rand_pattern_tic[ index ] = gametic;
            if( rand_pattern_cnt[ index ] == 0 )
            {
                rand_pattern_cnt[ index ] = A_Random();
                rand_pattern_state[ index ] ++;
            }
            rand_pattern_cnt[ index ] --;
        }
        if( (rand_pattern_state[ index ] & 1) == 0 )
          goto no_corona; // off
        corona_bright = 128;
        fade = 0;
        break;
      case SPLT_pulse: // slow pulsation, un-synch
        index = (gametic + objid) & 0xFF;  // obj dependent phase
        index -= 128; // -128 to +127
        // Make a positive parabola pulse, min does not quite reach 0.
        register float f = 1.0f - ((index*index) * 0.000055f);
        relsize = f;
        corona_alpha = corona_alpha * f;
        corona_bright = 80;
        fade = 0;
        break;
      default:
        I_SoftError("Draw_Sprite_Corona_Light: unknown light type %x", type);
        goto no_corona;
    }

    if( cz > Z1 )
    {
        if( fade & FADE_FAR )
        {
            // Proportional fade from Z1 to Z2
            corona_alpha = (int)( corona_alpha * ( Z2 - cz ) / ( Z2 - Z1 ));
        }
    }
    else if( fade & FADE_NEAR )
    {
        // Fade to 0 when get too close
        corona_alpha = (int)( corona_alpha *  cz / Z1 );
    }

    if (relsize > 1.0) 
        relsize = 1.0;
    corona_size = lsp->corona_radius * relsize * FIXED_TO_FLOAT( cv_coronasize.value );
    return corona_alpha;
 
no_corona:
   corona_alpha = 0;
   return 0;
}
       
   
static
void Sprite_Corona_Light_setup( vissprite_t * vis )
{
    mobj_t       * vismobj = vis->mobj;
    spr_light_t  * lsp;

    lsp = Sprite_Corona_Light_lsp( vismobj->sprite, vismobj->state );
    if( lsp == NULL )  goto no_corona;
   
    // Objects which emit light.
    if( (lsp->splgt_flags & (SPLGT_corona|SPLT_type_field)) == 0  )  goto no_corona;
   
    fixed_t tz = FixedDiv( projection_y, vis->scale );
    float cz = FIXED_TO_FLOAT( tz );
    // more realistique corona !
    if( cz >= Z2 )  goto no_corona;

    int mobjid = (uintptr_t)vismobj; // mobj dependent id
    if( Sprite_Corona_Light_fade( lsp, cz, mobjid>>1 ) == 0 )  goto no_corona;
   
    if( corona_bright )
    {
        // brighten the corona for software draw
        corona_alpha = (((int)corona_alpha * (255 - corona_bright)) + (255 * (int)corona_bright)) >> 8;
    }
   
    float size = corona_size / FIXED_TO_FLOAT(corona_sprlump.width);
    corona_xscale = (int)( (double)vis->xscale * size );
    corona_yscale = (int)( (double)vis->scale * size );

    // Corona specific.
    // Corona offsets are from center of drawn sprite.
    // no flip on corona

    // Position of the corona
# if 1
    fixed_t  midx = (vis->x1 + vis->x2) << (FRACBITS-1);  // screen
# else
    // same as spr, but not stored in vissprite so must recalculate it
//    fixed_t tr_x = vismobj->x - viewx;
//    fixed_t tr_y = vismobj->y - viewy;
    fixed_t tr_x = vis->mobj_x - viewx;
    fixed_t tr_y = vis->mobj_y - viewy;
    fixed_t  tx = (FixedMul(tr_x,viewsin) - FixedMul(tr_y,viewcos));
    fixed_t  midx = centerxfrac + FixedMul(tx, vis->xscale);
# endif
    corona_x0 = corona_x1 = (midx - FixedMul(corona_sprlump.leftoffset, corona_xscale)) >>FRACBITS;
    corona_x2 = ((midx + FixedMul(corona_sprlump.width - corona_sprlump.leftoffset, corona_xscale)) >>FRACBITS) - 1;
    if( corona_x1 < 0 )  corona_x1 = 0;
    if( corona_x1 > rdraw_viewwidth )  goto no_corona;  // off the right side
    if( corona_x2 >= rdraw_viewwidth )  corona_x2 = rdraw_viewwidth - 1;
    if( corona_x2 < 0 )  goto no_corona;  //  off the left side

    corona_draw = 2;
    return;

no_corona:
    corona_draw = 0;
    return;
}



static
void Draw_Sprite_Corona_Light( vissprite_t * vis )
{
    int            texturecolumn;
   
    // Sprite has a corona, and coronas are enabled.
    dr_alpha = (((int)corona_alpha * 7) + (2 * (16-7))) >> 4; // compensate for different HWR alpha 

#ifdef ENABLE_DRAW_ALPHA
    colfunc = alpha_colfunc;  // R_DrawAlphaColumn
    patch_t * corona_cc_patch = corona_patch;
    dr_color = corona_lsp->corona_color;
# ifndef ENABLE_DRAW8_USING_12
    if( vid.drawmode == DRAW8PAL )
    {
        dr_color8 = NearestColor( dr_color.s.red, dr_color.s.green, dr_color.s.blue);
    }
# endif
    dr_alpha_mode = cv_corona_draw_mode.EV;
    // alpha to dim the background through the corona   
    dr_alpha_background = (cv_corona_draw_mode.EV == 1)? (255 - dr_alpha) : 240;
#else
    colfunc = transcolfunc;  // R_DrawTranslucentColumn
    // Get the corona patch specifically colored for this light.
    patch_t * corona_cc_patch = get_colored_corona( corona_index );
//    dc_colormap = & reg_colormaps[0];
    dc_translucent_index = 0;  // translucent dr_alpha
    dc_translucentmap = & translucenttables[ translucent_alpha_table[dr_alpha >> 4] ];  // for draw8
#endif
   
    fixed_t light_yoffset = (int)(corona_lsp->light_yoffset * FRACUNIT); // float to fixed
   
#if 1   
    // [WDJ] This is the one that puts the center closest to where OpenGL puts it.
    fixed_t g_midy = (vis->gz_bot + vis->gz_top)>>1;  // mid of sprite
#else
    // Too high
    mobj_t * vismobj = vis->mobj;
#if 0
    fixed_t g_midy = vismobj->z + ((vis->gz_top - vis->gz_bot)>>1);
#else
    fixed_t g_midy = (vismobj->z + vis->gz_top)>>1;  // mid of sprite
#endif
#endif
    fixed_t g_cp = g_midy + light_yoffset - viewz;  // corona center point in vissprite scale
    fixed_t tp_cp = FixedMul(g_cp, vis->scale) + FixedMul(corona_sprlump.topoffset, corona_yscale);
    dm_top_patch = centeryfrac - tp_cp;
    dm_texturemid = FixedDiv( tp_cp, corona_yscale );
    dm_yscale = corona_yscale;
    dc_iscale = FixedDiv (FRACUNIT, dm_yscale);  // y texture step
    dc_texheight = 0;  // no wrap repeat
//    dc_texheight = corona_patch->height;
#ifdef MONSTER_VARY
    dc_y0 = centery;  // default ref to center, same scale for draw and position
#endif

   
// not flipped so
//  tex_x0 = 0
//  tex_x_iscale = iscale
//    fixed_t tex_x_iscale = (int)( (double)vis->iscale * size );
    fixed_t tex_x_iscale = FixedDiv (FRACUNIT, corona_xscale);
    fixed_t texcol_frac = 0;  // tex_x0, not flipped
    if( (corona_x1 - corona_x0) > 0 )  // it was clipped
        texcol_frac = tex_x_iscale * (corona_x1 - corona_x0);
 
    for (dc_x=corona_x1 ; dc_x<=corona_x2 ; dc_x++, texcol_frac += tex_x_iscale)
    {
        texturecolumn = texcol_frac>>FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= corona_patch->width) {
            // [WDJ] Give msg and don't draw it
            I_SoftError ("Sprite_Corona: bad texturecolumn\n");
            return;
        }
#endif
        byte * col_data = ((byte *)corona_cc_patch) + corona_cc_patch->columnofs[texturecolumn];
        R_DrawMaskedColumn( col_data );
    }

    colfunc = basecolfunc;
}
#endif


// ========
// [WDJ] 2019 With the CLIP3 improvements.
#define CLIPTOP_MIN   -2
// Larger than any rdraw_viewheight.
#define CLIPBOT_MAX   0x7FFE

//
// R_DrawSprite
//
//Fab:26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of sprites hidden under the console
//  dbx1, dbx2 // drawing bounds
//  env_clip_top, env_clip_bot // valid draw area array [dbx1 .. dbx2]
//        Always both env_clip_top and env_clip_bot, or both NULL.
static
void R_DrawSprite ( vissprite_t * spr, int dbx1, int dbx2, int16_t * env_clip_top, int16_t * env_clip_bot )
{
    drawseg_t*          ds;
    // Clip limit is the last drawable row inside the drawable area.
    // This makes limit tests easier, not needing +1 or -1.
    int16_t             clipbot[MAXVIDWIDTH];
    int16_t             cliptop[MAXVIDWIDTH];
    int                 x;
    int                 cx1, cx2; // clipping bounds
    int                 r1, r2;
    fixed_t             c_scale;  // clipping scale
    fixed_t             ds_highscale, ds_lowscale;
    int                 silhouette;
    // clip the unclipped columns between console and status bar
    //Fab:26-04-98: was -1, now clips against console bottom
    // [WDJ] These clips are all of a constant value across the entire sprite.
    // One traverse of the clip array with the severest clip is sufficient.
    int16_t  ht = con_clipviewtop;
    int16_t  hb = rdraw_viewheight - 1;

    c_scale = spr->scale;
    // clipping, restricted x range
    cx1 = ((dbx1 > spr->x1)? dbx1:spr->x1);
    cx2 = ((dbx2 < spr->x2)? dbx2:spr->x2);

#ifdef SPDR_CORONAS
    corona_draw = 0;
#if 0
    // Exclude Split sprites that are cut on the bottom, so there
    // is only one corona per object.
    // Their position would be off too.
    if( cv_corona.EV
        && ( (spr->cut & SC_BOTTOM) == 0 ) )
#else
    if( cv_corona.EV )
#endif
    {
        // setup corona state
        Sprite_Corona_Light_setup( spr );  // set corona_draw
        if( corona_draw )
        {
            // Expand clipping to include corona draw
            if( corona_x1 < cx1 )   cx1 = corona_x1;
            if( corona_x2 > cx2 )   cx2 = corona_x2;
        }
    }
#endif

    //SoM: 3/17/2000: Clip sprites in water.
    // [WDJ] vissprite uses a heightsec, which is only used for selected modelsec.
    if( spr->modelsec >= 0 )  // only things in specially marked sectors, not colormaps
    {
        fixed_t h,mh;
        // model sector for special sector clipping
        sector_t * spr_heightsecp = & sectors[spr->modelsec];

        // beware, this test does two assigns to mh, and an assign to h
        if ((mh = spr_heightsecp->floorheight) > spr->gz_bot
            && (h = centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0
            && (h >>= FRACBITS) < rdraw_viewheight)
        {
            // Assert: 0 <= h < rdraw_viewheight
            if (mh <= 0 || (viewer_has_model && !viewer_underwater))
            {                          // clip bottom
                // water cut clips that cover x1..x2
                if( h < hb )
                    hb = h;
            }
            else                        // clip top
            {
                // water cut clips that cover x1..x2
                if( h > ht )
                    ht = h;
            }
        }

        // beware, this test does an assign to mh, and an assign to h
        if ((mh = spr_heightsecp->ceilingheight) < spr->gz_top
            && (h = centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0
            && (h >>= FRACBITS) < rdraw_viewheight)
        {
            // Assert: 0 <= h < rdraw_viewheight
            if (viewer_overceiling)
            {                         // clip bottom
                // overceiling cut clips that cover x1..x2
                if( h < hb )
                    hb = h;
            }
            else                       // clip top
            {
                // water cut clips that cover x1..x2
                if( h > ht )
                    ht = h;
            }
        }
    }

    // Sprite cut clips that cover x1..x2
    // This would work just as well without the SC_TOP and SC_BOTTOM tests.
    if( (spr->cut & SC_TOP)
       && spr->y_top > ht )
            ht = spr->y_top;  // a lower clip
    if( (spr->cut & SC_BOTTOM)
       && spr->y_bot < hb )
            hb = spr->y_bot;  // a higher clip
    
    for (x = cx1 ; x <= cx2 ; x++)
    {
        cliptop[x] = ht;
        clipbot[x] = hb;
    }

    // Scan drawsegs from end to start for obscuring segs.
    // Apply all the clipping to cliptop and clipbot.
    //SoM: 4/8/2000:
    // Pointer check was originally nonportable
    // and buggy, by going past LEFT end of array:

    //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code
    for (ds=ds_p ; ds-- > drawsegs ; )
    {
        if( ds->silhouette == 0 )
             continue;  // cannot clip sprite

        // determine if the drawseg obscures the sprite
        if(   ds->x1 > cx2
           || ds->x2 < cx1 )
        {
            // does not cover sprite
            continue;
        }

        // r1..r2 where drawseg overlaps sprite (intersect)
        r1 = ds->x1 < cx1 ? cx1 : ds->x1;  // max x1
        r2 = ds->x2 > cx2 ? cx2 : ds->x2;  // min x2

        // if( c_scale < ds->scale1 && c_scale < ds->scale2 )  continue; // ds is behind sprite
        // if( c_scale > ds->scale1 && c_scale > ds->scale2 )  clip; // ds is in front of sprite
        // (lowscale,scale) = minmax( ds->scale1, ds->scale2 )
        if (ds->scale1 > ds->scale2)
        {
            ds_lowscale = ds->scale2;
            ds_highscale = ds->scale1;
        }
        else
        {
            ds_lowscale = ds->scale1;
            ds_highscale = ds->scale2;
        }

        if (ds_highscale < c_scale
            || ( ds_lowscale < c_scale
                 && !R_PointOnSegSide (spr->mobj_x, spr->mobj_y, ds->curline) ) )
        {
            // seg is behind sprite
            continue;  // next drawseg
        }

        // clip this piece of the sprite
        silhouette = ds->silhouette;

        // check sprite bottom above clip height
        if (spr->gz_bot >= ds->sil_bottom_height)
            silhouette &= ~SIL_BOTTOM;

        // check sprite top above clip height
        if (spr->gz_top <= ds->sil_top_height)
            silhouette &= ~SIL_TOP;

        if( silhouette == 0 )  // most often
             continue;

        if (silhouette == SIL_BOTTOM)
        {
            // bottom sil
            for (x=r1 ; x<=r2 ; x++)
                if( clipbot[x] > ds->spr_bottomclip[x] )
                    clipbot[x] = ds->spr_bottomclip[x];
        }
        else if (silhouette == SIL_TOP)
        {
            // top sil
            for (x=r1 ; x<=r2 ; x++)
                if( cliptop[x] < ds->spr_topclip[x] )
                    cliptop[x] = ds->spr_topclip[x];
        }
        else if (silhouette == (SIL_BOTTOM|SIL_TOP))
        {
            // both
            for (x=r1 ; x<=r2 ; x++)
            {
                if( clipbot[x] > ds->spr_bottomclip[x] )
                    clipbot[x] = ds->spr_bottomclip[x];
                if( cliptop[x] < ds->spr_topclip[x] )
                    cliptop[x] = ds->spr_topclip[x];
            }
        }
    }

    // [WDJ] Act on most severe combined cut clips that cover x1..x2.
    // This now always clips to (0, rdraw_viewheight-1) or better.
    if( env_clip_top )  // always both env_clip_top and env_clip_bot
    {
        for(x = cx1; x <= cx2; x++)
        {
            register int16_t bot2 = env_clip_bot[x - dbx1];
            if( clipbot[x] > bot2 )
                clipbot[x] = bot2;

            register int16_t top2 = env_clip_top[x - dbx1];
            if( cliptop[x] < top2 )
                cliptop[x] = top2;
        }
    }

    // All clipping has been performed, so draw the sprite.
    // [WDJ] No longer any need to check for unclipped columns.

    dm_floorclip = clipbot;
    dm_ceilingclip = cliptop;

#if 0   
#ifdef SPDR_CORONAS
    if( corona_draw == 1 )
    {
        // Draw corona before sprite, occlude
        Draw_Sprite_Corona_Light( spr );
    }
#endif
#endif

#if 1    
    // draw sprite, restricted x range
    R_DrawVisSprite (spr, ((dbx1 > spr->x1)? dbx1:spr->x1),
                          ((dbx2 < spr->x2)? dbx2:spr->x2));
#else
    R_DrawVisSprite (spr, cx1, cx2 );
#endif

#ifdef SPDR_CORONAS
//    if( corona_draw == 2 )
    if( corona_draw == 2  && ((spr->cut & 0x80) == 0) )
    {
        Draw_Sprite_Corona_Light( spr );
        spr->cut |= 0x80;
    }
#endif
}




//=============
#define  NUM_DRAWSPRITE_INC   128

// drawsprite
static drawsprite_t *  drawsprite_list = NULL;
static drawsprite_t *  ds_sprite_first = NULL;
static drawsprite_t *  ds_sprite_last = NULL;


static drawsprite_t *  drawsprite_freelist = NULL;  // free, linked by near, NULL term


static
void  drawsprite_alloc_to_freelist( int num_alloc )
{
    drawsprite_t * dnsp;
    drawsprite_t * dns_chunkp = malloc( sizeof(drawsprite_t) * num_alloc );
  
    if( dns_chunkp == NULL )
        return;

    dnsp = & (dns_chunkp[num_alloc - 1]);  // last one
    dnsp->nearer = drawsprite_freelist;  // freelist usually empty
    while( dnsp > dns_chunkp )
    {
        // link the new nodes together
        drawsprite_t * prev_dnsp = dnsp;
        dnsp--;
        dnsp->nearer = prev_dnsp;
    }
    drawsprite_freelist = dnsp;
}

// Not linked into drawnode
static
drawsprite_t *  create_drawsprite( void )
{
    if( drawsprite_freelist == NULL )
    {
        // make some more drawsprite
        drawsprite_alloc_to_freelist( NUM_DRAWSPRITE_INC );
        if( drawsprite_freelist == NULL )
        {
            // Malloc failed.
            // Give user a chance to savegame.
            drawsprite_alloc_to_freelist( 2 );
            if( drawsprite_freelist == NULL )  // malloc failed again
            {
                // Unlikely to happen, and should not devote much code to it.
                // Only have unreasonable ideas that might crash the drawer anyway.
                I_Error( "create_drawsprite: malloc failed." );
            }
            I_SoftError( "create_drawsprite: malloc failed." );
        }
    }

    // always use node from free list
    drawsprite_t * dspr = drawsprite_freelist;
    drawsprite_freelist = dspr->nearer;  // freelist is linked using near
    dspr->order_type = DSO_none;
    dspr->order_id = 0;
    return dspr;
}

// Frees from the drawsprite_list, and ds_sprite_first.
static
void  free_drawsprite( drawsprite_t * dspr )
{
    // unlink
    if( dspr->nearer )
        dspr->nearer->prev = dspr->prev;
    if( dspr->prev )
        dspr->prev->nearer = dspr->nearer;
    
    if( drawsprite_list == dspr )
        drawsprite_list = dspr->nearer;
    if( ds_sprite_first == dspr )
        ds_sprite_first = dspr->nearer;

#ifdef PARANOIA
    dspr->prev = NULL;
#endif
  
    // link to freelist
    dspr->nearer = drawsprite_freelist;
    drawsprite_freelist = dspr;
}


// Put to end of list.
//   list : head of drawsprite_t list
static
void  put_sprite_to_sprite_list( drawsprite_t * dspr, drawsprite_t * * list )
{
    // Sprite list order is farthest to nearest.
    // Sprites are entered nearest first.
    // Only need to put sprite at head.
    drawsprite_t * dnl = *list;
    *list = dspr;
    dspr->nearer = dnl;
    dspr->prev = NULL;
    if( dnl )
        dnl->prev = dspr;
}

// Insert to list
//   after_member : member of a list
static
void  insert_into_sprite_list( drawsprite_t * dspr, drawsprite_t * after_member )
{
    // Sprite list order is farthest to nearest.
    dspr->prev = after_member;
    dspr->nearer = after_member->nearer;
    if( dspr->nearer )
        dspr->nearer->prev = dspr;
    after_member->nearer = dspr;
}




// =====  R_sort_drawseg_masked

// In a drawseg
//  Drawseg are created from sub-sector lines, not sectors.
//  The thickside is the edge of a ffloor that abuts the seg linedef.
//  The thickside nearest the view will most often be from the adjacent drawseg
//  that is nearer the viewer.  This drawseg will not have the planes of the farther subsector.

//  The ffloor planes are in the nearest drawseg that touches the ffloor.  They will be extended
//  as far as they can.


// Sort drawseg contents.  Create drawsprite list.
// called by R_Draw_Masked
static
void R_sort_drawseg_masked( void )
{
    visplane_t *  plane_draw[MAXFFLOORS];  // in draw order, far planes to near planes
    drawseg_t  *  ds;
    draw_ffplane_t * dffp;
    vissprite_t * vsp;  // rover vissprite

   
    // [WDJ] Sort plane dist as they are put into the list.
    // This avoids repeating searching through the same entries, PlaneBounds() and tests.
    plane_draw[0] = NULL;

    // Drawsegs are in drawsegs array, in bsp draw order, nearest first.
    // Drawseg ds_p is farthest at end of array, ds_p-- is towards near.
    // Using drawsegs, so do not need to link.
    for( ds = ds_p; ds-- > drawsegs; )
    {
//      ds->near_scale = (ds->scale1 > ds->scale2)? ds->scale1 : ds->scale2;
//      scale_near = dnp->near_scale;
//      scale_far = ( ds->scale2 > ds->scale1 )? ds->scale1 : ds->scale2;

        dffp = ds->draw_ffplane;
        if( dffp && dffp->numffloorplanes )
        {
            // plane boundaries are independent of this seg.
            visplane_t * plane;
            int p, vk;
            byte  num_plane = 0; // number of planes in plane_draw

            for(p = 0; p < dffp->numffloorplanes; p++)
            {
                plane = dffp->ffloorplanes[p];
                if( ! plane )
                    continue;

                dffp->ffloorplanes[p] = NULL;  // remove from floorplanes

                R_PlaneBounds(plane);  // set highest_top, lowest_bottom
                  // in screen coord, where 0 is top (hi)

                // Drawable area is con_clipviewtop to (rdraw_viewheight - 1)
                if(plane->lowest_bottom < con_clipviewtop
                   || plane->highest_top >= rdraw_viewheight  // [WDJ] rdraw window, not vid.height
                   || plane->highest_top > plane->lowest_bottom)
                {
                    continue;  // not visible, next plane
                }

                // Rare to have more than one plane per ffloor, so optimize for one.
                vk = num_plane;  // insert at plane slot
                if( num_plane )
                {
                    // Floor planes are sorted, farthest from viewz first.
                    fixed_t delta = abs(plane->height - viewz); // new entry distance
                    // shuffle existing planes that are nearer.
                    for( ; vk > 0; vk-- )  // near planes to far planes
                    {
                        // test for plane closer to viewz
                        visplane_t * vpp = plane_draw[vk-1]; // next nearer plane
                        if( abs(vpp->height - viewz) > delta )
                            break;  // new plane is nearer

                        // shuffle existing draw_plane entry
                        plane_draw[vk] = vpp;
                    }
                }
                // Add to plane_draw list
                plane_draw[vk] = plane;
                num_plane ++;
            }  // planes

            // Store sorted planes back in the drawseg
            dffp->numffloorplanes = num_plane;
            memcpy( & dffp->ffloorplanes[0], & plane_draw[0], (sizeof(visplane_t*) * num_plane) );

            // Calculate nearest_scale for this ffloor.
            dffp->nearest_scale = 0;
            if( num_plane )
            {
                // backscale is the nearest edge of the plane.
                // This is the same for all plane from the same drawseg.
                int x;
                fixed_t nearest_backscale = FIXED_MIN;
                plane = plane_draw[0];
                for(x = plane->minx; x <= plane->maxx; x++)
                {
                    // Larger scale is nearer.	   
                    if( dffp->backscale_r[x] > nearest_backscale )
                    {
//                      if( dffp->backscale_r[x] < FIXED_MAX )   //   FIX ??
                        nearest_backscale = dffp->backscale_r[x];
                    }
                }
                dffp->nearest_scale = nearest_backscale;

            } // if num_plane

        }  // if dffp && dffp->numffloorplanes
    }  // for drawsegs

    // Sprites
    drawsprite_list = NULL;
    
    // sprite list is sorted from vprsortedhead    
    // Traverse vissprite sorted list, nearest to farthest.
    for(vsp = vsprsortedhead.prev; vsp != &vsprsortedhead; vsp = vsp->prev)
    {
        if(vsp->y_top > vid.height || vsp->y_bot < 0)
            continue;

        // As the sprites are put into sprite lists, the farthest sprites
        // will be at the head of each list.
        fixed_t v_x1 = vsp->x1;
        fixed_t v_x2 = vsp->x2;

#if 1
        // Sprite off the screen test.
        if(( v_x1 > vid.width ) || ( v_x2 < 0 ) )
        {
#ifdef PARANOIA	 
            printf( "sprite off screen %i..%i\n", v_x1, v_x2 );
#endif
            continue;  // off screen
        }
#endif

        drawsprite_t *  dspr = create_drawsprite(); // DSO_none
        dspr->sprite = vsp;
        dspr->x1 = v_x1;
        dspr->x2 = v_x2;

#ifdef DEBUG_DRAWSPRITE
        dspr->type = vsp->mobj->type; // debug
#endif

        // All sprites already in this sprite list are nearer than this sprite.
        put_sprite_to_sprite_list( dspr, &drawsprite_list );

    } // sprite vsp

}  // R_sort_drawseg_masked



//  dspr : drawsprite, has bounds of the draw
static
void draw_masked_sprite( vissprite_t * vsp, drawsprite_t * dspr )
{
#ifdef PARANOIA
    if( vsp == NULL )
    {
        GenPrintf( EMSG_warn, "draw_masked_sprite  vsp == NULL\n" );
        return;
    }
#endif

    if( vsp->clip_top )
    {
        // Has clipping
        int xc = dspr->x1 - vsp->x1;  // position of dspr within vsp range.
        R_DrawSprite( vsp, dspr->x1, dspr->x2, &vsp->clip_top[xc], &vsp->clip_bot[xc] );
    }
    else
    {
        R_DrawSprite( vsp, dspr->x1, dspr->x2, NULL, NULL );
    }
    // Vissprite does not need to be released.
}


enum {
    DMSP_remainder     = 0x01,
    DMSP_remainder_top = 0x02,
    DMSP_remainder_bot = 0x04,
    DMSP_masked = 0x10,
    DMSP_conflict = 0x80,
};

// Sprite is behind the plane.
// This failed with the previous render system.  The tests are more complicated here.
// If the plane is solid, then it blocks the sprite.
// Draw only the part of the sprite that is blocked by the plane.
static
void draw_sprite_vrs_plane( drawsprite_t * dspr, visplane_t * plane )
{
    int16_t mask_clip_top[MAXVIDWIDTH];
    int16_t mask_clip_bot[MAXVIDWIDTH];
    int16_t vr_clip_top[MAXVIDWIDTH];
    int16_t vr_clip_bot[MAXVIDWIDTH];

    vissprite_t * vsp = dspr->sprite;
    vis_cover_t * pl_cover = plane->cover;

    int vsp_range = (vsp->x2 - vsp->x1 + 1);
    int vsp_copy_size = vsp_range * sizeof(int16_t);
    int x1 = dspr->x1;
    int x2 = dspr->x2;
    byte sprite_visible = false;
    byte dmsp = 0;  // DMSP_xx flags
    int x;

    if( plane->ffloor )
    {
        ffloor_t * ffp = plane->ffloor;
        if( ffp->flags & (FF_TRANSLUCENT|FF_FOG) )
        {
            int xrange = x2 - x1 + 1;
            int xcopy_size = xrange * sizeof(int16_t);

            sprite_visible = true; // sprite can be seen through ffloor

            // Initialize mask to block everything.
            memset( &mask_clip_top[x1], 0x7F, xcopy_size );  // 0x7F7F
            memset( &mask_clip_bot[x1], 0, xcopy_size );     // 0
        }
    }

    // To reduce number of test cases below.
    if( vsp->clip_top )  // always both
    {
        // Default result to same.
        memcpy( &vr_clip_top[vsp->x1], &vsp->clip_top[0], vsp_copy_size );
        memcpy( &vr_clip_bot[vsp->x1], &vsp->clip_bot[0], vsp_copy_size );
    }
    else
    { 
        // Initialize sprite mask to sprite screen values
        set_int16( vr_clip_top, vsp->x1, vsp->x2, vsp->y_top );  // sprite top
        set_int16( vr_clip_bot, vsp->x1, vsp->x2, vsp->y_bot );  // sprite bottom
    }

    // over sprite width, where covered by plane
    if( x1 < plane->minx )
        x1 = plane->minx;
    if( x2 > plane->maxx )
        x2 = plane->maxx;

    for( x = x1; x <= x2; x++ )
    {
        if( pl_cover[x].top > vr_clip_top[x] )
        {
            // Top of plane is below top of sprite. Sprite is visible.
            if( pl_cover[x].top > vr_clip_bot[x] )
            {
                // Plane is entirely below sprite, no clip.
                dmsp |= DMSP_remainder;
            }
            else
            {
#ifdef PARANOID    
                if( pl_cover[x].bot < vr_clip_bot[x] )
                {
                    // some of sprite is also visible below the plane
                    dmsp |= DMSP_remainder_bot | DMSP_conflict;
                }
#endif
                // Plane clips sprite bottom.
                mask_clip_top[x] = pl_cover[x].top;
                mask_clip_bot[x] = vr_clip_bot[x];
                vr_clip_bot[x] = pl_cover[x].top - 1;
                dmsp |= DMSP_remainder_top | DMSP_masked;		
            }
        }
        else if( pl_cover[x].bottom < vr_clip_bot[x] )
        {
            // Bottom of plane is above bottom of sprite. Sprite is visible.
            if( pl_cover[x].bottom < vr_clip_top[x] )
            {
                // plane is above sprite, no clip
                dmsp |= DMSP_remainder;
            }
            else
            {
#ifdef PARANOID
                if( pl_cover[x].top > vr_clip_top[x] )  // cannot happen because of first test
                {
                    // some of sprite is also visible above the plane
                    dmsp |= DMSP_remainder_top | DMSP_conflict;
                }
#endif
                // Plane clips sprite top.
                mask_clip_top[x] = vr_clip_top[x];
                mask_clip_bot[x] = pl_cover[x].bottom;
                vr_clip_top[x] = pl_cover[x].bottom + 1;
                dmsp |= DMSP_remainder_bot | DMSP_masked;
            }
        }
        else
        {
            // Plane entirely covers the sprite.
            mask_clip_top[x] = vr_clip_top[x];
            mask_clip_bot[x] = vr_clip_bot[x];
            vr_clip_top[x] = 0x7fff;
            vr_clip_bot[x] = 0;
            dmsp |= DMSP_masked;
        }
        // Else, plane does not touch sprite.
    }
 
    if( sprite_visible && (dmsp & DMSP_masked) )
    {
        // Draw sprite covered by the plane because the plane is translucent.
        R_DrawSprite( vsp, x1, x2, &mask_clip_top[x1], &mask_clip_bot[x1] );
    }

#ifdef PARANOID    
    if( ( ~dmsp & (DMSP_remainder_top | DMSP_remainder_bot)) == 0 )  // both set
    {
        // Do not think this can happen because sprite vrs plane is only called
        // when sprite is above the plane, or below the plane.
        // Sprite cut by plane is handled separately.
        GenPrint( EMSG_error, "Sprite vrs Plane, plane splits sprite." );
    }
#endif

    if( dmsp & (DMSP_remainder | DMSP_remainder_top | DMSP_remainder_bot) )
    {
        // Some of the sprite remains to be drawn.
        if( vsp->clip_top == NULL )  // Did not have clip arrays.
        {
            // Always allocate both
            vsp->clip_top = get_pool16_array( vsp_range );
            vsp->clip_bot = get_pool16_array( vsp_range );
        }
        // Save the clipping arrays.
        memcpy( vsp->clip_top, &vr_clip_top[vsp->x1], vsp_copy_size );
        memcpy( vsp->clip_bot, &vr_clip_bot[vsp->x1], vsp_copy_size );
        dspr->order_type = DSO_none;  // continue
    }
    else
    {
        // There is nothing left to draw for this sprite.
        // This also removes it from any other collision detection.
        free_drawsprite( dspr );
    }
}

// ============================================================


// Resolve the odd conflict in sprite drawing order when affected by planes.
static
void  sprite_vrs_sprite_list( drawsprite_t * dspr, drawsprite_t * first, drawsprite_t * last )	    
{
    drawsprite_t * dspr2;
    drawsprite_t * dspr_end = (last)? last->nearer : NULL;
    
    vissprite_t * vsp = dspr->sprite;
    fixed_t v_x1 = dspr->x1;
    fixed_t v_x2 = dspr->x2;

    for( dspr2 = first; dspr2 != dspr_end; dspr2 = dspr2->nearer )
    {
        // All sprites already in the ds_sprite_first list are farther than dspr.
        if( dspr2->order_type >= DSO_ffloor_area )
        {
            register vissprite_t * vsp2 = dspr2->sprite;

            // Limiting v_x1 and v_x2, by sprite splitting
            // would not change the outcome of this test.
            if( dspr2->x1 > v_x2 || dspr2->x2 < v_x1 )
              continue;  // no x overlap

            if( vsp2->y_top > vsp->y_bot || vsp2->y_bot < vsp->y_top )
              continue;  // no y overlap

            // The dspr sprite must be drawn after the dspr2 sprite.
            // The dspr sprite is nearer in the list, it will get drawn after dspr2, as long
            // as they have order_type that put them in the same drawing search.
            if( dspr2->order_type == DSO_ffloor_area )
            {
                // Make dspr2 the same tag as dspr, it will get drawn during the same drawing search.
                dspr2->order_type = dspr->order_type;
                dspr2->order_id = dspr->order_id;
            }
            else if( dspr->order_type == dspr2->order_type )
            {
                if( dspr2->order_id > dspr->order_id )
                    dspr2->order_id = dspr->order_id;  // force draw of dspr2 earlier
            }
        }
    }
}


// The sprites in the same scale range are compared against the drawseg.
void drawseg_vrs_sprites( drawseg_t * ds )
{
    draw_ffplane_t * dffp;
    draw_ffside_t  * dffs;
    vissprite_t * vsp;
    drawsprite_t * dspr, * dspr_next;

    byte   conflict;
    fixed_t  v_x1, v_x2, v_scale;
    fixed_t  occ_x1, occ_x2;  // occluding seg or plane

    fixed_t  d_scale_near;  // the nearest scale of the drawseg.
//    fixed_t  d_scale_far;
    fixed_t  d_x1, d_x2;
    
    d_x1 = ds->x1;
    d_x2 = ds->x2;
    occ_x1 = d_x1;  // occluding seg init
    occ_x2 = d_x2;
    
    if( ds->scale1 > ds->scale2 )
    {
        d_scale_near = ds->scale1;
//	d_scale_far = ds->scale2;
    }
    else
    {
        d_scale_near = ds->scale2;
//	d_scale_far = ds->scale1;
    }

    dffs = ds->draw_ffside;
    dffp = ds->draw_ffplane;

    // Many times do not need to search entire sprite list when the next drawseg is nearer.
    // Due to the drawseg ordering by BSP, this happens often.
    // However, when the next drawseg covers several previous drawsegs, any of the previous
    // passed over sprites may be behind the drawseg, and may need to be drawn.
    
    // Sprites within drawseg range spread.
    for( dspr = drawsprite_list; dspr; dspr = dspr_next )
    {
        conflict = DSO_none;
        // sprite may get drawn and removed
        dspr_next = dspr->nearer;
        vsp = dspr->sprite;
        v_scale = vsp->scale;
        v_x1 = dspr->x1;
        v_x2 = dspr->x2;

        // It does not matter if the seg has textures or sides.
        // If the sprite is drawn at the seg of the correct distance, it will correctly
        // be drawn relative to any other seg and the sprites at those segs.

        // Sprite vrs seg.
        if( (v_scale <= d_scale_near) // farther than nearest part of seg
            && (v_x1 <= d_x2) && (v_x2 >= d_x1) )  // within x-range of seg
        {
            // sprite is within seg x-range and scale.

            // If the sprite is turned relative to the seg, one end of it may be nearer than a seg side.
            // The collision detection only uses the center of the sprite,
            // so drawing it should entirely be behind a seg (railing, transparency, etc.)
            // If one side of the sprite overhangs the drawseg, it might be sitting
            // on a plane, that will get drawn over it.
            fixed_t point_scale_v1 = ds->scale1 + (ds->scalestep * (v_x1 - d_x1));
            fixed_t point_scale_v2 = ds->scale1 + (ds->scalestep * (v_x2 - d_x1));
            if( v_scale < point_scale_v1 )
            {
                if( v_scale < point_scale_v2 )
                  goto sprite_farther_than_ds; // sprite is farther
            }
            else
            {
                if( v_scale > point_scale_v2 )
                  goto sprite_nearer_than_ds; // sprite is nearer
            }

            // Sprite is over the seg line.
            // Can put it entirely on one side or the other, or can split it.

            // Sprite would be behind the seg, if it is occluded.
#if 1
            fixed_t mid_point_scale = (point_scale_v1 + point_scale_v2) >> 1;
#else
            fixed_t v_mid_x = (v_x1 + v_x2)>>1;
            fixed_t mid_point_scale = ds->scale1 + (ds->scalestep * (v_mid_x - d_x1));
#endif
            if( v_scale < mid_point_scale )
            {
                // sprite is farther than any texture of the drawseg
                if( ds->maskedtexturecol )
                {
                    // sprite occluded by texture, decision is forced
                    goto sprite_farther_than_ds; // sprite is farther
                }

                // Thicksides
                if( dffs )
                {
                    // Is sprite occluded by any thickside.
                    int tsi;
                    for( tsi = 0; tsi < dffs->numthicksides; tsi++ )
                    {
                        ffloor_t * ffloor = dffs->thicksides[tsi];
                        fixed_t topheight = *(ffloor->topheight);
                        fixed_t bottomheight = *(ffloor->bottomheight);
                        if(  (topheight >= viewz && bottomheight <= viewz)
                             || (topheight < viewz  && vsp->gz_top < topheight)
                             || (bottomheight > viewz && vsp->gz_bot > bottomheight) )
                        {
                            // sprite occluded by thickside, decision is forced
                            goto sprite_farther_than_ds; // sprite is farther
                        }
                    }
                }
            }
            goto check_ffloor;

    sprite_farther_than_ds:
            // Sprite is behind the drawseg and needs to be drawn.
            conflict = DSO_before_drawseg;
            occ_x1 = d_x1;  // occluding seg
            occ_x2 = d_x2;

            // However, part of it may also be behind a floor plane, so just splitting it now
            // may leave a portion that has not been checked for conflict with the planes.
            // This is an optimization that also eliminates a check for conflict when no ffloor.
            if( dffp && dffp->numffloorplanes )
              goto check_floor_planes;  // might also be in conflict with the ffloor planes

            goto sprite_before_drawseg;

        } // Sprite vrs seg.

        // The bsp draw order draws all the segs behind the plane, then draws the planes.
        // A sprite can only be on a plane of this drawseg, if it is in front of all the adjacent drawseg
        // that make up the far edge.

    sprite_nearer_than_ds:
        // Sprite is before the drawseg.
        // It may be on ffloor planes of that drawseg.
        // The BSP puts the planes with the nearest drawseg that it can.

    check_ffloor:
        // Check if the sprite is within the effects of a ffloor and its planes.
        // Otherwise, it will be left in the list for the next drawseg.
        if( (dffp == NULL) || (dffp->numffloorplanes == 0) )
        {
            // No ffloor.
            // Because DSO_before_drawseg bypasses this test, do not have to check conflict here.
            if( v_scale > d_scale_near ) // nearer than nearest part of seg
              break;  // rest of sprites are nearer than this drawseg.

            continue;
        }

    check_floor_planes:
        // The ffloor x-range covers multiple adjacent drawsegs, so it is different than the drawseg x-range.
        // The ffloor were put in the first adjacent drawseg, as they were created nearest to farthest.
        // That puts them in the last possible adjacent drawseg when traversing farthest to nearest.
        // So when the ffloor are encountered here, all the tests against the far edge, from adjacent
        // drawsegs (sprite vrs seg test above), will have been completed.

        // Sprite vrs ffloor planes.
        {
            visplane_t * plane;
            int p;

            // Sprite vrs planes.
            // This is independent of this drawseg, as this seg is only part of the far edge of the ffloor.

            // The far seg and backscale are the same for all ffloor of this seg.
            if( v_scale > dffp->nearest_scale )
                break;  // this sprite, and rest of sprites, are closer than any of the planes

            // overlap of x1..x2
            plane = dffp->ffloorplanes[0];
            fixed_t p_x1 = plane->minx;
            fixed_t p_x2 = plane->maxx;

            if( v_x1 > p_x1 )  p_x1 = v_x1;
            if( v_x2 < p_x2 )  p_x2 = v_x2;
            if( p_x1 > p_x2 )  // overlap of plane and sprite
                goto sprite_not_in_ffloor;  // sprite not in ffloor x-range, next sprite

#if 0
            if( v_scale < backscale_far )
                   goto  sprite_within_plane;  // behind near edge of plane

            // between backscale_near and backscale_far
#endif


            {
                // SoM: NOTE: Because a visplane's shape and scale is not directly
                // bound to any single linedef, a simple poll of it's scale is
                // not adequate. We must check the entire scale array for any
                // part that is in front of the sprite.

                fixed_t * bsr = & dffp->backscale_r[ p_x1 ];
                for(  ; bsr <= & dffp->backscale_r[ p_x2 ]; bsr++ )
                {
                    // keeps sprite from being seen through floors
                    // backscale is nearest edge of the plane.
                    if( *bsr > v_scale )
                    {
                        // some part of sprite is farther than backscale
//		      if( *bsr == FIXED_MAX )  break;

                        goto  sprite_touches_ffloor;
                    }
                }
            }

  sprite_not_in_ffloor:
            // the sprite is not in the ffloor area
            if( conflict == DSO_before_drawseg )
              goto sprite_before_drawseg;

            continue;  // next sprite


  sprite_touches_ffloor:
            // The sprite touches a plane (by scale).  Check for draw occlusion, and which plane.
            for( p = 0; p < dffp->numffloorplanes; p++ )
            {
                // A ffplane is minx to maxx.
                // The far distance is bounded by several drawseg.
                // The near distance is the backscale array.
                plane = dffp->ffloorplanes[p];
                if( ! plane )
                  continue; // next plane

                // Would like to trust that all planes have same minx, maxx,
                // but do not trust that yet.
                if( v_x1 > plane->maxx || v_x2 < plane->minx )
                  continue; // next plane

                // test overlap of plane and sprite on screen
                if( vsp->y_top > plane->lowest_bottom )
                  continue; // next plane
                if( vsp->y_bot < plane->highest_top )
                  continue; // next plane

#if 1	       
                // [WDJ] test mid of sprite instead of toe and head
                // to avoid whole sprite affected by a marginal overlap
                fixed_t mobj_mid_z = (vsp->mobj_bot_z + vsp->mobj_top_z) >> 1;
                if( plane->height < viewz )
                {
                    // looking down at floor
                    if( plane->height < mobj_mid_z )
                      continue;  // sprite over floor, next plane
                }
                else
                {
                    // looking up at ceiling
                    if( plane->height > mobj_mid_z )
                      continue;  // sprite under ceiling, next plane
                }
#else
                if( plane->height < viewz )
                {
                    // looking down at floor
                    if( plane->height <= vsp->mobj_bot_z )
                      continue;  // sprite over floor, next plane
                }
                else
                {
                    // looking up at ceiling
                    if( plane->height >= vsp->mobj_top_z )
                      continue;  // sprite under ceiling, next plane
                }
#endif

                if( conflict == DSO_before_drawseg )
                {
                    // The sprite is before the drawseg, but some of it is also covered by this plane.
                    // occ_x1 and occ_x2 are of the ds, set when DSO_before_drawseg was set.
                    if( plane->minx < occ_x1 )  occ_x1 = plane->minx;
                    if( plane->maxx > occ_x2 )  occ_x2 = plane->maxx;

                    // Draw as sprite_before_drawseg, but with wider clip limits.
                    // Otherwise would have to run the split sprites (from this sprite), through the ffloor tests too.
                    goto sprite_before_drawseg;
                }

                // The sprite is overdrawn by a plane on the screen.
                // Put the sprite in this drawseg, tagged to specific ffloor.
                dspr->order_type = DSO_before_ffloor;
                dspr->order_id = p;
                goto sprite_conflict_ffloor;

            } // for planes

            // Sprite in ffloor area but not overdrawn by any ffloor planes.
            if( conflict == DSO_before_drawseg )
              goto sprite_before_drawseg;


            // Sprite is not overdrawn by any floor planes.
            // Must ensure that cannot conflict any other nearer ffloor.
            {
                // SoM: NOTE: Because a visplane's shape and scale is not directly
                // bound to any single linedef, a simple poll of it's scale is
                // not adequate. We must check the entire scale array for any
                // part that is in front of the sprite.

                fixed_t * bsr = & dffp->backscale_r[ p_x1 ];
                for(  ; bsr <= & dffp->backscale_r[ p_x2 ]; bsr++ )
                {
                    // backscale is nearest edge of the plane.
                    if(*(bsr++) < v_scale)
                    {
                        // some part of sprite is nearer than backscale
                        goto  sprite_continue; 
                    }
                }
            }

            // The sprite is in the ffloor area, but is not behind
            // any specific plane.

            // These may overlap some later planes, so must check carefully
            // that sprite is entirely within the ffloor area.
            // Will get drawn after all floor planes.
            dspr->order_type = DSO_ffloor_area;

            // Could be occluded by another sprite.
            // But checking that here caused sprite draw order_type to change,
            // and then sprite splits could be drawn over nearer planes.
            // Let the nearer sprites check for conflict with it.

            if( ! plane )  // if no plane, then cannot split on its boundary
              goto sprite_to_active_list;  // does not have to be drawn with this drawseg

            // Not occluded by a plane, so can split on vertical line at plane boundary.
            occ_x1 = plane->minx;
            occ_x2 = plane->maxx;
            goto sprite_splitting;


  sprite_conflict_ffloor:
            // Sprite could be at edge of a ffloor.
            // The available horz. and vert. sprite splitting will not work for this sprite,
            // because the edge is slanted and free form, sprite occlusion must get more complicated.

            // There were other tests here, but they did not work.  Other tests may be put here in the future.

            // It might be occluded by another sprite.
            sprite_vrs_sprite_list( dspr, ds_sprite_first, ds_sprite_last );

            // [WDJ] Cannot do sprite splitting vrs planes, using sprite clipping instead.
            // Splits at lineseg edges would need to follow the top and bottom edges of the plane.
            // The plane minx and maxx are at the drawscale of the seg anchoring the far edge of the plane.
            // The actual edges of the near plane edges can be smaller or larger.
            // The x1..x2 of the sprite is at the depth of the sprite.  They do not directly compare.

            // Masking by the plane will be handled by draw_sprite_vrs_plane, draw_masked_sprite, and R_DrawSprite.
            // After drawing, the remainder of the drawsprite will continue.
            goto sprite_to_active_list;

        } // Sprite vrs ffloor planes.

  sprite_continue:
        // Sprite not in this drawseg.
        continue;  // next sprite


  sprite_before_drawseg:
        // The sprite may get split by multiple drawsegs, so there may be several slivers of sprite.
        // Sprites behind the drawseg will get drawn in depth order due to being entered in the list
        // farthest to nearest.  It does not matter if any sprite occludes another.
        dspr->order_type = DSO_before_drawseg;

  sprite_splitting:
        // Sprite might exceed seg, on either or both sides.
        // That part of sprite must be occluded by a different seg or plane.
        // This can only be used where sprite can be split on a vertical line (see plane edge tests).

        if( v_x1 < occ_x1 )
        {
            // create a partial sprite
            drawsprite_t * dspr1 = create_drawsprite(); // DSO_none
#ifdef DEBUG_DRAWSPRITE
            dspr1->type = dspr->type; // debug
#endif
            dspr1->sprite = vsp;
            // cut the sprite
            dspr1->x1 = v_x1;
            dspr1->x2 = occ_x1 - 1;
            v_x1 = occ_x1;
            dspr->x1 = v_x1;
            insert_into_sprite_list( dspr1, dspr );  // link in after dspr
        }

        if( v_x2 > occ_x2 )
        {
            // create a partial sprite
            drawsprite_t * dspr2 = create_drawsprite(); // DSO_none
#ifdef DEBUG_DRAWSPRITE
            dspr2->type = dspr->type; // debug
#endif
            dspr2->sprite = vsp;
            // cut the sprite
            dspr2->x1 = occ_x2 + 1;
            dspr2->x2 = v_x2;
            v_x2 = occ_x2;
            dspr->x2 = v_x2;
            insert_into_sprite_list( dspr2, dspr );  // link in after dspr
        }

        // Part of sprite that is not occluded.
        // May be partial width sprite.

        if( dspr->order_type == DSO_before_drawseg )
        {
            // Draw it now.
            draw_masked_sprite( vsp, dspr );
            free_drawsprite( dspr );
            continue;  // next sprite
        }

  sprite_to_active_list:
        // Include in drawseg sprite list (within drawsprite_list).
        ds_sprite_last = dspr;
        if( ds_sprite_first == NULL )
          ds_sprite_first = dspr;
    } // for sprites
}

   

void R_Init_draw_masked()
{
}

//
// R_Draw_Masked
//
void R_Draw_Masked (void)
{
    drawseg_t * ds;
    draw_ffplane_t * dffp;
    draw_ffside_t  * dffs;
    drawsprite_t * dspr_end = NULL;

    R_sort_drawseg_masked();


    // Drawsegs are in bsp draw order, partially sorted.
    // However, the BSP order is depth first, and erratic across the screen width.
    // Optimizations that depend upon an expected BSP order, will fail.
    for( ds = ds_p; ds-- > drawsegs; )
    {
        ds_sprite_first = NULL;
        ds_sprite_last = NULL;

        if( drawsprite_list )
        {
            // Will draw the sprites behind the seg, immediately.
            // Others are marked.
            drawseg_vrs_sprites( ds );
            dspr_end = ds_sprite_last ? ds_sprite_last->nearer : NULL;
        }


        if( ds->maskedtexturecol )
        {
            // draw masked texture
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);
            ds->maskedtexturecol = NULL;
        } // if maskedtexturecol

        // Draw thicksides before planes, because some thicksides are hidden
        // behind the planes.
        dffs = ds->draw_ffside;
        if( dffs && dffs->numthicksides )
        {
            int tsi;
            for( tsi = 0; tsi < dffs->numthicksides; tsi++)
            {
                // draw thickside
                R_RenderThickSideRange( ds, ds->x1, ds->x2, dffs->thicksides[tsi] );
            }
        }  // if numthicksides

        dffp = ds->draw_ffplane;
        if( dffp && dffp->numffloorplanes )
        {
            // [WDJ] Floor planes are combined subsectors with the same flat.
            // They are split into drawable planes by bsp, with a vertical cut at each offending vertex.
            // This does NOT make them convex, as there can still be inward angles. But they are drawable by columns (rows??).
            // A sprite that is on such a cut can be drawn too soon for an adjacent floor subsector.
            // The x1 and x2 are the limits of the plane, not the cut.

            int  p;
            visplane_t * plane;
            drawsprite_t * dspr, * dspr_next;

            // Floor planes are sorted by vertical distance from viewz, farthest from viewz first.
            for(p = 0; p < dffp->numffloorplanes; p++)   // far planes to near planes
            {
                plane = dffp->ffloorplanes[p];
                dffp->ffloorplanes[p] = NULL;  // remove from floorplanes

                // Find sprites in this list that must be drawn before this plane.
                for( dspr = ds_sprite_first; dspr; dspr = dspr_next )
                {
                    if( dspr == dspr_end )
                      break;

                    dspr_next = dspr->nearer;  // may get unlinked

                    if( (dspr->order_type == DSO_before_ffloor) && (dspr->order_id == p) )
                    {
                        // May release dspr.
                        draw_sprite_vrs_plane( dspr, plane );
                    }
                } // for sprites

                // draw floor plane
                R_DrawSinglePlane( plane );
            } // for planes

            dffp->numffloorplanes = 0;

        } // if draw_ffplane

        if( ds_sprite_first )
        {
            // The sprites in this list: overlap x1..x2, and are farther than near_scale.
            drawsprite_t * dspr, * dspr_next;
            for( dspr = ds_sprite_first; dspr; dspr = dspr_next )
            {
                if( dspr == dspr_end )
                  break;

                dspr_next = dspr->nearer;  // may get unlinked

                if( dspr->order_type == DSO_ffloor_area )
                {
                    draw_masked_sprite( dspr->sprite, dspr );
                    free_drawsprite( dspr );
                }
            }
        }

        // After draw of planes

    } // for drawnodes

    if( drawsprite_list )
    {
        // Sprites left in the drawsprite_list, are nearest.
        // Draw them now.
        drawsprite_t * dspr, * dspr_next;
        for(dspr = drawsprite_list; dspr; dspr = dspr_next)
        {
            dspr_next = dspr->nearer;
            draw_masked_sprite( dspr->sprite, dspr );
            free_drawsprite( dspr );
        }
    }
   
}  // R_Draw_Masked





// ==========================================================================
//
//                              SKINS CODE
//
// ==========================================================================

// This does not deallocate the skins memory.
#define SKIN_ALLOC   8
int         numskins = 0;
skin_t *    skins[MAXSKINS+1];
skin_t *    skin_free = NULL;
skin_t      marine;


static
int  get_skin_slot(void)
{
    skin_t * sk;
    int si, i;

    // Find unused skin slot, or add one.
    for(si=0; si<numskins; si++ )
    {
        if( skins[si] == NULL )  break;
    }
    if( si >= MAXSKINS )  goto none;

    // Get skin alloc.
    if( skin_free == NULL )
    {
        i = SKIN_ALLOC;
        sk = (skin_t*) malloc( sizeof(skin_t) * SKIN_ALLOC );
        if( sk == NULL )   goto none;
        // Link to free list
        while( i-- )
        {
            *(skin_t**)sk = skin_free;  // link
            skin_free = sk++;
        }
    }

    sk = skin_free;
    skin_free = *(skin_t**)sk;  // unlink
    skins[si] = sk;
    if( si >= numskins )  numskins = si+1;
    return si;

none:
    return 0xFFFF;
}

static
void  free_skin( int skin_num )
{
    skin_t * sk;
    
    if( skin_num >= MAXSKINS )  return;
    sk = skins[skin_num];
    if( sk == NULL )  return;

    skins[skin_num] = NULL;
    *(skin_t**)sk = skin_free;  // Link into free list
    skin_free = sk;

    while( numskins>0 && (skins[numskins-1] == NULL) )
    {
        numskins --;
    }
    // Cannot move existing skins
}

static
void Skin_SetDefaultValue(skin_t *skin)
{
    int   i;

    // setup the 'marine' as default skin
    memset (skin, 0, sizeof(skin_t));
    strcpy (skin->name, DEFAULTSKIN);
    strcpy (skin->faceprefix, "STF");
    for (i=0;i<NUMSFX_DEF;i++)  // not sfx_free, nor DEHEXTRA slots
    {
        // SKS_NONE = none
        byte skinsnd_id = S_sfx[i].skinsound;
        if( skinsnd_id < NUMSKINSOUNDS )
        {
            // change the skin sound for this skin	    
            skin->soundsid[skinsnd_id] = i;
        }
    }
//    memcpy(&skin->spritedef, &sprites[SPR_PLAY], sizeof(spritedef_t));
}

//
// Initialize the basic skins
//
void R_Init_Skins (void)
{
    skin_free = NULL;

    memset (skins, 0, sizeof(skins));
   
    // make the standard Doom2 marine as the default skin
    // skin[0] = marine skin
    skins[0] = & marine;
    Skin_SetDefaultValue( & marine );
    memcpy(&marine.spritedef, &sprites[SPR_PLAY], sizeof(spritedef_t));
    numskins = 1;
}

// Returns the skin index if the skin name is found (loaded from pwad).
// Return 0 (the default skin) if not found.
int R_SkinAvailable (const char* name)
{
    int  i;

    for (i=0;i<numskins;i++)
    {
        if( skins[i] && strcasecmp(skins[i]->name, name)==0)
            return i;
    }
    return 0;
}


void SetPlayerSkin_by_index( player_t * player, int index )
{
    skin_t * sk;

    if( index >= numskins )   goto default_skin;
   
    sk = skins[index];
    if( sk == NULL )   goto default_skin;
    
    // Change the face graphics
    if( player == &players[statusbarplayer]
        // for save time test it there is a real change
        && !( skins[player->skin] && strcmp (skins[player->skin]->faceprefix, sk->faceprefix)==0 )
        )
    {
        ST_Release_FaceGraphics();
        ST_Load_FaceGraphics(sk->faceprefix);
    }

set_skin:
    // Record the player skin.
    player->skin = index;
 
    // A copy of the skin value so that dead body detached from
    // respawning player keeps the skin
    if( player->mo )
        player->mo->skin = sk;
    return;

default_skin:
    index = 0;  // the old marine skin
    sk = &marine;
    goto set_skin;
}


// network code calls this when a 'skin change' is received
void  SetPlayerSkin (int playernum, const char *skinname)
{
    int   i;

    for(i=0;i<numskins;i++)
    {
        // search in the skin list
        if( skins[i] && strcasecmp(skins[i]->name,skinname)==0)
        {
            SetPlayerSkin_by_index( &players[playernum], i );
            return;
        }
    }

    GenPrintf(EMSG_warn, "Skin %s not found\n", skinname);
    // not found put the old marine skin
    SetPlayerSkin_by_index( &players[playernum], 0 );
}


//
// Add skins from a pwad, each skin preceded by 'S_SKIN' marker
//

// Does the same as in w_wad, but check only for
// the first 6 characters (this is so we can have S_SKIN1, S_SKIN2..
// for wad editors that don't like multiple resources of the same name)
//
static
int W_CheckForSkinMarkerInPwad (int wadid, int startlump)
{
    lump_name_t name8;
    uint64_t mask6;  // big endian, little endian
    int  numlumps = wadfiles[wadid]->numlumps;
    lumpinfo_t * lumpinfo = wadfiles[wadid]->lumpinfo;

    name8.namecode = -1; // make 6 char mask
    name8.s[6] = name8.s[7] = 0;
    mask6 = name8.namecode;

    numerical_name( "S_SKIN", & name8 );  // fast compares

    // scan forward, start at <startlump>
    if( (startlump < numlumps) && lumpinfo )
    {
        lumpinfo_t * lump_p = & lumpinfo[ startlump ];
        int i;
        for (i = startlump; i < numlumps; i++,lump_p++)
        {
            // Only check first 6 characters.
            if( (*(uint64_t *)lump_p->name & mask6) == name8.namecode )
            {
                return WADLUMP(wadid,i);
            }
        }
    }
    return -1; // not found
}

//
// Find skin sprites, sounds & optional status bar face, & add them
//
void R_AddSkins (int wadnum)
{
    int         lumpnum, lastlump, lumpn;

    lumpinfo_t* lumpinfo;
    char*       sprname;
    uint32_t    numname;

    char*       buf;
    char*       buf2;

    char*       token;
    char*       value;
   
    skin_t *    sk;
    int         skin_index;

    int         i,size;

    //
    // search for all skin markers in pwad
    //

    lastlump = 0;
    for(;;)
    {
        sprname = NULL;

        lumpnum = W_CheckForSkinMarkerInPwad (wadnum, lastlump);
        if( lumpnum == -1 )  break;

        lumpn = LUMPNUM(lumpnum);
        lastlump = lumpn + 1;  // prevent repeating same skin

        skin_index = get_skin_slot();
        if( skin_index > MAXSKINS )
        {
            GenPrintf(EMSG_warn, "ignored skin lump %d (%d skins maximum)\n", lumpn, MAXSKINS);
            continue; //faB:so we know how many skins couldn't be added
        }
        sk = skins[skin_index];

        // set defaults
        Skin_SetDefaultValue(sk);
        sprintf (sk->name,"skin %d", numskins-1);

        buf  = W_CacheLumpNum (lumpnum, PU_CACHE);
        size = W_LumpLength (lumpnum);

        // for strtok
        buf2 = (char *) malloc (size+1);
        if(!buf2)
        {
             I_SoftError("R_AddSkins: No more free memory\n");
             goto skin_error;
        }
        memcpy (buf2,buf,size);
        buf2[size] = '\0';

        // parse
        token = strtok (buf2, "\r\n= ");
        while (token)
        {
            if(token[0]=='/' && token[1]=='/') // skip comments
            {
                token = strtok (NULL, "\r\n"); // skip end of line
                goto next_token;               // find the real next token
            }

            value = strtok (NULL, "\r\n= ");
//            CONS_Printf("token = %s, value = %s",token,value);
//            CONS_Error("ga");

            if (!value)
            {
                I_SoftError("R_AddSkins: syntax error in S_SKIN lump# %d in WAD %s\n", lumpn, wadfiles[wadnum]->filename);
                goto skin_error;
            }

            if (!strcasecmp(token,"name"))
            {
                // the skin name must uniquely identify a single skin
                // I'm lazy so if name is already used I leave the 'skin x'
                // default skin name set above
                if (!R_SkinAvailable (value))
                {
                    strncpy (sk->name, value, SKINNAMESIZE);
                    strlwr (sk->name);
                }
            }
            else
            if (!strcasecmp(token,"face"))
            {
                strncpy (sk->faceprefix, value, 3);
                sk->faceprefix[3] = 0;
                strupr (sk->faceprefix);
            }
            else
            if (!strcasecmp(token,"sprite"))
            {
                sprname = value;
                strupr(sprname);
            }
            else
            {
                int found=false;
                // copy name of sounds that are remapped for this skin
                for (i=0;i<NUMSFX_DEF;i++)  // not sfx_free nor DEHEXTRA
                {
                    if (!S_sfx[i].name)
                      continue;

                    if( (S_sfx[i].skinsound < NUMSKINSOUNDS)
                        && strcasecmp(S_sfx[i].name, token+2) == 0 )
                    {
                        sk->soundsid[S_sfx[i].skinsound]=
                            S_AddSoundFx(value+2, S_sfx[i].flags);
                        found=true;
                    }
                }
                if(!found)
                {
                    I_SoftError("R_AddSkins: Unknown keyword '%s' in S_SKIN lump# %d (WAD %s)\n",
                               token, lumpn, wadfiles[wadnum]->filename);
                    goto skin_error;
                }
            }
next_token:
            token = strtok (NULL,"\r\n= ");
        }

        // if no sprite defined use sprite just after this one
        if( !sprname )
        {
            lumpn++;
            lumpinfo = wadfiles[wadnum]->lumpinfo;
            if( lumpinfo == NULL )
                return;

            // get the base name of this skin's sprite (4 chars)
            sprname = lumpinfo[lumpn].name;
            numname = *(uint32_t*)sprname;

            // skip to end of this skin's frames
            lastlump = lumpn;
            while (*(uint32_t*)lumpinfo[lastlump].name == numname)
                lastlump++;
            // allocate (or replace) sprite frames, and set spritedef
            R_AddSingleSpriteDef (sprname, &sk->spritedef, wadnum, lumpn, lastlump);
        }
        else
        {
            // search in the normal sprite tables
            char **name;
            boolean found = false;
            for(name = sprnames;*name;name++)
            {
                if( strcmp(*name, sprname) == 0 )
                {
                    found = true;
                    sk->spritedef = sprites[sprnames-name];
                }
            }

            // not found so make a new one
            if( !found )
                R_AddSingleSpriteDef (sprname, &sk->spritedef, wadnum, 0, INT_MAX);

        }

        CONS_Printf ("added skin '%s'\n", sk->name);

        free(buf2);
    }
    return;
   
skin_error:
    free_skin(skin_index);	       
    return;
}
