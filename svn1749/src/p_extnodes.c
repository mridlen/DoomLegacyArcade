// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_extnodes.c,v 1.1 2020/05/19 11:20:16 micha Exp $
//
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//  Copyright(C) 2015-2020 Fabian Greffrath
//  Copyright (C) 2021 by DooM Legacy Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
//  DESCRIPTION:
//  Support maps with NODES in compressed or uncompressed ZDBSP format or
//  DeePBSP format.
//
//-----------------------------------------------------------------------------

// [MB] 2020-04-21: Support for ZDoom extended nodes based on woof 1.2.0
//      Modified to use C99 fixed width data types
//      Added some checks taken from DooM Legacy code for regular nodes
//      Fixed endianess and use LE_SWAP* macros
//      Added #ifdef switches for all code that requires zlib


// [FG] support maps with NODES in compressed ZDBSP format
#include "doomincl.h"

#include "p_extnodes.h"
#include "w_wad.h"
#include "z_zone.h"


// [FG] support maps with NODES in compressed or uncompressed ZDBSP format
//      or DeePBSP format

mapformat_t P_CheckMapFormat ( lumpnum_t lumpnum )
{
    mapformat_t format = MFMT_DOOMBSP;
    byte *nodes = NULL;
    int b;

#if 0  // [MB] 2020-04-21: Hexen format was already checked in p_setup.c
    if ((b = lumpnum+ML_BLOCKMAP+1) < numlumps &&
        !strcasecmp(lumpinfo[b].name, "BEHAVIOR"))
        I_Error("P_CheckMapFormat: Hexen map format not supported in %s.\n",
                lumpinfo[lumpnum-ML_NODES].name);
#endif

    // [MB] 2020-04-21: Check for <numlumps removed (numlumps not available)
    b = lumpnum+ML_NODES;
    if ((nodes = W_CacheLumpNum(b, PU_IN_USE)) && W_LumpLength(b) > 8)
    {
        if (!memcmp(nodes, "xNd4\0\0\0\0", 8))
            format = MFMT_DEEPBSP;
        else if (!memcmp(nodes, "XNOD", 4))
            format = MFMT_ZDBSPX;
        else if (!memcmp(nodes, "ZNOD", 4))
            format = MFMT_ZDBSPZ;
    }

    if (nodes)
        Z_Free(nodes);

    return format;
}



#ifdef DEEPSEA_EXTENDED_NODES

#include "r_main.h"
#include "r_splats.h"  // [MB] 2020-04-21: Added for hardware renderer
#include "m_swap.h"


// [crispy] support maps with NODES in DeePBSP format

typedef PACKED_STRUCT (
{
    int32_t v1;
    int32_t v2;
    uint16_t angle;
    uint16_t linedef;
    int16_t side;
    uint16_t offset;
}) mapseg_deepbsp_t;

typedef PACKED_STRUCT (
{
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    int16_t bbox[2][4];
    int32_t children[2];
}) mapnode_deepbsp_t;

typedef PACKED_STRUCT (
{
    uint16_t numsegs;
    int32_t firstseg;
}) mapsubsector_deepbsp_t;

// [crispy] support maps with NODES in ZDBSP format

typedef PACKED_STRUCT (
{
    int32_t v1, v2;
    uint16_t linedef;
    unsigned char side;
}) mapseg_zdbsp_t;

typedef PACKED_STRUCT (
{
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    int16_t bbox[2][4];
    int32_t children[2];
}) mapnode_zdbsp_t;

typedef PACKED_STRUCT (
{
    uint32_t numsegs;
}) mapsubsector_zdbsp_t;



// [FG] recalculate seg offsets

static
fixed_t GetOffset(vertex_t *v1, vertex_t *v2)
{
    fixed_t dx, dy;
    fixed_t r;

    dx = (v1->x - v2->x)>>FRACBITS;
    dy = (v1->y - v2->y)>>FRACBITS;
    r = (fixed_t)(sqrt(dx*dx + dy*dy))<<FRACBITS;

    return r;
}

// [FG] support maps with DeePBSP nodes

void P_LoadSegs_DeePBSP ( lumpnum_t lump )
{
    byte * data;
    uint32_t vn1, vn2;  // not valid negative

    numsegs = W_LumpLength(lump) / sizeof(mapseg_deepbsp_t);
    uint32_t sizesegs = numsegs * sizeof(seg_t);
    segs = Z_Malloc(sizesegs, PU_LEVEL, NULL);
    memset(segs, 0, sizesegs);
    data = W_CacheLumpNum(lump, PU_STATIC);
    // [WDJ] Do endian as read from segs lump temp

    if( !data || (numsegs < 1))
    {
        I_SoftError( "Bad segs data\n" );
        return;
    }

    uint32_t si;
    for (si=0; si<numsegs; si++)
    {
        seg_t * li = & segs[si];
        mapseg_deepbsp_t * ml = (mapseg_deepbsp_t *) data + si;

        int side, linedef;
        line_t * ldef;

        vn1 = LE_SWAP32(ml->v1);
        vn2 = LE_SWAP32(ml->v2);
        // [MB] 2020-04-21: Detect buggy wad (same as for normal nodes)
        if( vn1 > numvertexes || vn2 > numvertexes )
        {
            I_SoftError("P_LoadSegs_DeePBSP: Seg vertex bad %u,%u\n",
                         vn1, vn2 );
            // zero both out together, make seg safer
            // (otherwise will cross another line)
            vn1 = vn2 = 0;
        }
        li->v1 = &vertexes[vn1];
        li->v2 = &vertexes[vn2];

#ifdef HWRENDER
        // [MB] 2020-04-21: Added (same as for normal nodes)
        li->pv1 = li->pv2 = NULL;
        li->length = P_SegLength (li);
        li->lightmaps = NULL; // list of static lightmap for this seg
#endif

        li->angle = ((uint16_t)( LE_SWAP16(ml->angle) ))<<16;
        li->offset = (LE_SWAP16(ml->offset))<<16;
        linedef = (uint16_t)( LE_SWAP16(ml->linedef) );

        // [MB] 2020-04-21: Detect buggy wad (same as for normal nodes)
        if( linedef > numlines )
        {
            I_SoftError( "P_LoadSegs_DeePBSP: linedef #%i > numlines %i\n",
                         linedef, numlines );
            linedef = 0; // default
        }

        ldef = &lines[linedef];
        li->linedef = ldef;
        side = LE_SWAP16(ml->side);

        // [MB] 2020-04-21: Detect buggy wad (same as for normal nodes)
        if( side != 0 && side != 1 )
        {
            I_SoftError( "P_LoadSegs_DeePBSP: bad side index\n");
            side = 0;  // assume was using wrong side
        }
        // side1 required to have sidenum != NULL_INDEX
        if( ldef->sidenum[side] == NULL_INDEX )
        {
            I_SoftError( "P_LoadSegs_DeePBSP: using missing sidedef\n");
            side = 0;  // assume was using wrong side
        }

        li->side = side;
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        // killough 5/3/98: ignore 2s flag if second sidedef missing:
        if (ldef->flags & ML_TWOSIDED && ldef->sidenum[side^1]!=NULL_INDEX)
            li->backsector = sides[ldef->sidenum[side^1]].sector;
        else
            li->backsector = 0;

        // [MB] 2020-04-21: Added (same as for normal nodes)
        li->numlights = 0;
        li->rlights = NULL;
    }

    Z_Free (data);
}


void P_LoadSubsectors_DeePBSP ( lumpnum_t lump )
{
    mapsubsector_deepbsp_t * data;

    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_deepbsp_t);
    uint32_t sizesubsec = numsubsectors * sizeof(subsector_t);
    subsectors = Z_Malloc(sizesubsec, PU_LEVEL, 0);
    data = (mapsubsector_deepbsp_t *)W_CacheLumpNum(lump, PU_STATIC);

    memset(subsectors, 0, sizesubsec);

    uint32_t i;
    for (i=0; i<numsubsectors; i++)
    {
        subsectors[i].numlines = (uint16_t)( LE_SWAP16(data[i].numsegs) );
        subsectors[i].firstline = (uint32_t)( LE_SWAP32(data[i].firstseg) );
    }

    Z_Free (data);
}

void P_LoadNodes_DeePBSP ( lumpnum_t lump )
{
    byte * data;

    numnodes = W_LumpLength (lump) / sizeof(mapnode_deepbsp_t);
    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
    data = W_CacheLumpNum (lump, PU_STATIC);

    // [FG] skip header
    data += 8;

    // [MB] 2020-04-21: Added similar check as for normal nodes
    // No nodes and one subsector is a trivial but legal map.
    if( (numnodes < 1) && (numsubsectors > 1) )
    {
        I_SoftError("P_LoadNodes_DeePBSP: Bad node data\n");
        return;
    }

    uint32_t i;
    for (i=0; i<numnodes; i++)
    {
        node_t * no = nodes + i;
        mapnode_deepbsp_t * mn = (mapnode_deepbsp_t *) data + i;

        no->x = LE_SWAP16(mn->x)<<FRACBITS;
        no->y = LE_SWAP16(mn->y)<<FRACBITS;
        no->dx = LE_SWAP16(mn->dx)<<FRACBITS;
        no->dy = LE_SWAP16(mn->dy)<<FRACBITS;

        uint32_t j;
        for (j=0 ; j<2 ; j++)
        {
# ifdef DEEPSEA_EXTENDED_SIGNED_NODES
            no->children[j] = LE_SWAP32(mn->children[j]);
# else
            // [WDJ]  There are no valid negative node children.
            no->children[j] = (uint32_t)( LE_SWAP32(mn->children[j]) );
# endif

            uint32_t k;
            for (k=0 ; k<4 ; k++)
                no->bbox[j][k] = LE_SWAP16(mn->bbox[j][k])<<FRACBITS;
        }
    }

    W_CacheLumpNum(lump, PU_CACHE);
}


#ifdef HAVE_ZLIB
#include <string.h>  // [MB] 2020-05-19: For memcpy()
#include <zlib.h>

#if HAVE_ZLIB == 3
// Optional, load using dlopen.
#include <dlfcn.h>
  // dlopen

// Indirections to zlib functions.
static int ZEXPORT (*DL_inflateInit_)(z_streamp, const char *, int);
// inflateInit is a macro in zlib
#define inflateInit_   (*DL_inflateInit_)

static int ZEXPORT (*DL_inflate)(z_streamp, int);
#define inflate  (*DL_inflate)

static int ZEXPORT (*DL_inflateEnd)(z_streamp);
#define inflateEnd  (*DL_inflateEnd)

byte  zlib_present = 0;

#ifdef LINUX
# define ZLIB_NAME   "libz.so"
#else
# define ZLIB_NAME   "libz.so"
#endif


void ZLIB_available( void )
{
    // Test for zlib being loaded.
    void * lzp = dlopen( ZLIB_NAME, RTLD_LAZY );
    // No reason to close it as it would dec the reference count.
    
    zlib_present = ( lzp != NULL );

    if( ! zlib_present )  return;
   
    // Get ptrs for libzip functions.
    DL_inflateInit_ = dlsym( lzp, "inflateInit_" );
    DL_inflate = dlsym( lzp, "inflate" );
    DL_inflateEnd = dlsym( lzp, "inflateEnd" );
}

#undef ZLIB_NAME
#endif
#endif  // HAVE_ZLIB


// [FG] support maps with compressed or uncompressed ZDBSP nodes
// adapted from prboom-plus/src/p_setup.c:1040-1331
// heavily modified, condensed and simplified
// - removed most paranoid checks, brought in line with Vanilla P_LoadNodes()
// - removed const type punning pointers
// - inlined P_LoadZSegs()
// - added support for compressed ZDBSP nodes
// - added support for flipped levels

void P_LoadNodes_ZDBSP ( lumpnum_t lump, boolean compressed )
{
    const char * msg;
    byte * lumpdata;  // lump contents
    byte * data;  // nodes data to be processed, may come from decompression.
#ifdef HAVE_ZLIB
    z_stream * zstream = NULL;  // zlib input
    byte * zoutput = NULL;  // zlib output of compressed nodes data
    uint32_t alloc_len;
#endif  // HAVE_ZLIB

    uint32_t orgVerts, z_newVerts;
    uint32_t z_numSubs, currSeg;
    uint32_t z_numSegs;
    uint32_t z_numNodes;
    vertex_t * newvertarray = NULL;

    lumpdata = W_CacheLumpNum(lump, PU_LEVEL);  // Z_Malloc
    data = lumpdata;  // gets incremented

    // 0. Uncompress nodes lump (or simply skip header)

    if (compressed)
    {
        // Compressed extended nodes.
#ifdef HAVE_ZLIB
        const int len =  W_LumpLength(lump);
        int outlen, err;

# if HAVE_ZLIB == 3
        if( ! zlib_present )  // zlib optional and not present
        {
            msg = "ZDBSP nodes decompression, zlib not present";
            goto nodes_fail;
        }
# endif

        // first estimate for compression rate:
        // zoutput buffer size == 2.5 * input size
        outlen = 2.5 * len;
        // [WDJ] Use malloc for temps that are released before start of the level map.
        // These do not use any Z_Malloc abilities.
        zoutput = malloc( outlen );
        // [WDJ] Test both, may have avail mem = 2.1 * len.
        if( zoutput == NULL )
        {
            alloc_len = outlen;
            goto memory_err;
        }

        // initialize stream state for decompression
        alloc_len = sizeof( *zstream );  // also for failure reporting
        zstream = malloc( alloc_len );
        if( zstream == NULL )
        {
            goto memory_err;
        }

        memset(zstream, 0, sizeof(*zstream));
        zstream->next_in = data + 4;
        zstream->avail_in = len - 4;
        zstream->next_out = zoutput;
        zstream->avail_out = outlen;

        if (inflateInit(zstream) != Z_OK)
        {
            msg = "ZDBSP nodes decompression failed";
            goto nodes_fail;
        }

        // Decompress using zlib, until buffer is full, or is done.
        while ((err = inflate(zstream, Z_SYNC_FLUSH)) == Z_OK)
        {
            // When zlib returns Z_OK (not done), increase zoutput buffer size, keeping previous work.
            int outlen_old = outlen;
            outlen = 2 * outlen_old;
            byte * zoutput2 = realloc(zoutput, outlen);
            if( zoutput2 == NULL )  // realloc allocation fail
            {
                alloc_len = outlen;
                goto memory_err;  // must free zoutput ourselves (graceful recovery)
            }

            zoutput = zoutput2;
            zstream->next_out = zoutput + outlen_old;
            zstream->avail_out = outlen - outlen_old;
        }

        if (err != Z_STREAM_END)
        {
            msg = "ZDBSP nodes decompression failed";
            goto nodes_fail;
        }

        if( verbose )
        {
            GenPrintf(EMSG_info,
                  "ZDBSP nodes compression ratio %.3f\n",
                  (float)zstream->total_out/zstream->total_in);
        }

        if (inflateEnd(zstream) != Z_OK)
        {
            msg = "ZDBSP nodes decompression failed";
            goto nodes_fail;
        }

        data = zoutput;

        // release the original data lump
        Z_Free (lumpdata);  // [WDJ] Free it, not worth caching.
        lumpdata = NULL;

        free(zstream);
        zstream = NULL;

#else  // HAVE_ZLIB
        msg = "Compressed ZDBSP nodes, requires zlib";
        goto nodes_fail;
#endif  // HAVE_ZLIB
    }
    else
    {
        // Uncompressed extended nodes.
        // skip header
        data = lumpdata + 4;
    }

    // 1. Load new vertices added during node building

    orgVerts = (uint32_t)LE_SWAP32(*((uint32_t*)data));
    data += sizeof(orgVerts);

    z_newVerts = (uint32_t)LE_SWAP32(*((uint32_t*)data));
    data += sizeof(z_newVerts);

    if ((orgVerts + z_newVerts) == numvertexes)
    {
        newvertarray = vertexes;
    }
    else
    {
        uint32_t sizeold = orgVerts * sizeof(vertex_t);
        uint32_t sizenew = z_newVerts * sizeof(vertex_t);
        newvertarray = Z_Malloc( sizeold + sizenew, PU_LEVEL, 0);
        memcpy(newvertarray, vertexes, sizeold);
        memset( &newvertarray[orgVerts], 0, sizenew);
    }

    uint32_t iv;
    for (iv = 0; iv < z_newVerts; iv++)
    {
#if 0  // [MB] 2020-04-21: DooM Legacy has no separate renderer coordinates
        newvertarray[iv + orgVerts].r_x =
#endif
        newvertarray[iv + orgVerts].x = LE_SWAP32(*((uint32_t*)data));
        data += sizeof(newvertarray[0].x);

#if 0  // [MB] 2020-04-21: DooM Legacy has no separate renderer coordinates
        newvertarray[iv + orgVerts].r_y =
#endif
        newvertarray[iv + orgVerts].y = LE_SWAP32(*((uint32_t*)data));
        data += sizeof(newvertarray[0].y);
    }

    if (vertexes != newvertarray)
    {
        uint32_t i3;
        for (i3 = 0; i3 < (uint32_t)numlines; i3++)
        {
            // update lines from old vertexes to newvertarray
            line_t * lip = & lines[i3];
            lip->v1 = lip->v1 - vertexes + newvertarray;
            lip->v2 = lip->v2 - vertexes + newvertarray;
        }

        Z_Free(vertexes);
        vertexes = newvertarray;
        numvertexes = (orgVerts + z_newVerts);
    }

    // 2. Load subsectors

    z_numSubs = (uint32_t)LE_SWAP32(*((uint32_t*)data));
    data += sizeof(z_numSubs);

    if( z_numSubs == 0 )
    {
        msg = "No subsectors in map";
        goto nodes_fail;
    }

    numsubsectors = z_numSubs;
    subsectors = Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);

    currSeg = 0;
    uint32_t si;
    for (si = 0; si < numsubsectors; si++)
    {
        mapsubsector_zdbsp_t * mseg = (mapsubsector_zdbsp_t*) data + si;

        subsectors[si].firstline = currSeg;
        subsectors[si].numlines = (uint32_t)LE_SWAP32(mseg->numsegs);
        currSeg += (uint32_t)LE_SWAP32(mseg->numsegs);
    }

    data += numsubsectors * sizeof(mapsubsector_zdbsp_t);

    // 3. Load segs

    z_numSegs = (uint32_t)LE_SWAP32(*((uint32_t*)data));
    data += sizeof(z_numSegs);

    // The number of stored segs should match the number of segs used by
    // subsectors
    if (z_numSegs != currSeg)
    {
        msg = "ZDBSP nodes, Incorrect number of segs";
        goto nodes_fail;
    }

    numsegs = z_numSegs;
    segs = Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);

    uint32_t ti;
    for (ti = 0; ti < numsegs; ti++)
    {
        line_t * ldef;
        unsigned int linedef;
        unsigned char side;
        seg_t * li = & segs[ti];
        mapseg_zdbsp_t * ml = (mapseg_zdbsp_t *) data + ti;
        uint32_t vn1, vn2;

        vn1 = LE_SWAP32(ml->v1);
        vn2 = LE_SWAP32(ml->v2);
        // [MB] 2020-04-21: Detect buggy wad (same as for normal nodes)
        if( vn1 > numvertexes || vn2 > numvertexes )
        {
            I_SoftError("P_LoadSegs_ZDBSP: Seg vertex bad %u,%u\n",
                         vn1, vn2 );
            // zero both out together, make seg safer
            // (otherwise will cross another line)
            vn1 = vn2 = 0;
        }
        li->v1 = &vertexes[vn1];
        li->v2 = &vertexes[vn2];

#ifdef HWRENDER
        // [MB] 2020-04-22: Added (same as for normal nodes)
        li->pv1 = li->pv2 = NULL;
        li->length = P_SegLength (li);
        li->lightmaps = NULL; // list of static lightmap for this seg
#endif

        linedef = (uint16_t)( LE_SWAP16(ml->linedef) );
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = ml->side;

        // e6y: check for wrong indexes
        // These are both unsigned now.
        if (ldef->sidenum[side] >= numsides)
        {
            I_SoftError("Linedef %u for seg %u references a non-existent sidedef %u",
                    linedef, ti, ldef->sidenum[side]);
            ldef->sidenum[side] = 0;  // [WDJ] arbitrary valid sidedef
        }

        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        // seg angle and offset are not included
        li->angle = R_PointToAngle2(li->v1->x, li->v1->y,
                                    li->v2->x, li->v2->y);
        li->offset = GetOffset(li->v1, (ml->side ? ldef->v2 : ldef->v1));

        // killough 5/3/98: ignore 2s flag if second sidedef missing:
        if( (ldef->flags & ML_TWOSIDED) && (ldef->sidenum[side^1] != NULL_INDEX) )
            li->backsector = sides[ldef->sidenum[side^1]].sector;
        else
            li->backsector = 0;
    }

    data += numsegs * sizeof(mapseg_zdbsp_t);

    // 4. Load nodes

    z_numNodes = (uint32_t)LE_SWAP32(*((uint32_t*)data));
    data += sizeof(z_numNodes);

    numnodes = z_numNodes;
    nodes = Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);

    uint32_t ni;
    for (ni = 0; ni < numnodes; ni++)
    {
        node_t * no = & nodes[ni];
        mapnode_zdbsp_t * mn = (mapnode_zdbsp_t *) data + ni;

        no->x = LE_SWAP16(mn->x)<<FRACBITS;
        no->y = LE_SWAP16(mn->y)<<FRACBITS;
        no->dx = LE_SWAP16(mn->dx)<<FRACBITS;
        no->dy = LE_SWAP16(mn->dy)<<FRACBITS;

        int j;
        for (j = 0; j < 2; j++)
        {
# ifdef DEEPSEA_EXTENDED_SIGNED_NODES
            no->children[j] = (uint32_t)LE_SWAP32(mn->children[j]);
# else
            // [WDJ]  There are no valid negative node children.
            no->children[j] = (uint32_t)( LE_SWAP32(mn->children[j]) );
# endif

            int k;	    
            for (k = 0; k < 4; k++)
                no->bbox[j][k] = LE_SWAP16(mn->bbox[j][k])<<FRACBITS;
        }
    }

free_memory:
    // [WDJ] Handle error conditions too, all combinations of memory allocations.
    // DoomLegacy recovers gracefully, instead of using I_Error everywhere.
#ifdef HAVE_ZLIB
    if( compressed )
    {
        if( lumpdata == zoutput )
            lumpdata = NULL;
        free( zoutput );
        free( zstream );
    }
#endif  // HAVE_ZLIB

    if( lumpdata )
    {
        Z_Free( lumpdata );  // Not worth caching.
//        W_CacheLumpNum(lump, PU_CACHE);
    }
    return;

// Error handlers, rare.
#ifdef HAVE_ZLIB
memory_err:
    // [WDJ] Report size that comes from WAD that we do not control.
    GenPrintf(EMSG_warn, "Allocate %i bytes failed.", alloc_len );
    msg = "ZDBSP nodes decompression, out-of-memory";
    goto nodes_fail;
#endif

nodes_fail:
    I_SoftError( "P_LoadNodes_ZDBSP: %s\n", msg );
    goto free_memory;
}


#endif  // DEEPSEA_EXTENDED_NODES
