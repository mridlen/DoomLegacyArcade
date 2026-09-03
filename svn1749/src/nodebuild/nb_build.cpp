// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// [Arcade] Drive the vendored ZDBSP node builder from plain C structs.
//
// This is the only translation unit that sees ZDBSP's headers.  ZDBSP and
// DoomLegacy both declare a node_t, so nothing here may include the engine's
// headers -- the interface is nb_build.h, which mentions neither.
//
// ZDBSP is Copyright (C) 2002-2006 Randy Heit, GPL v2 or later; see
// COPYING.zdbsp in this directory.  The three FLevel members below are
// reimplemented rather than vendored because their originals live in
// processor.cpp, which exists only to read and write wad files.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "zdbsp.h"
#include "nodebuild.h"
#include "nb_build.h"

// ---------------------------------------------------------------------------
// The handful of globals and helpers ZDBSP keeps in main.cpp, which exists only
// to parse a command line.  Values are its defaults (-p, -s, -d).
// ---------------------------------------------------------------------------

int  MaxSegs      = 64;
int  SplitCost    = 8;
int  AAPreference = 16;

angle_t PointToAngle( fixed_t x, fixed_t y )
{
    double ang = atan2( double(y), double(x) );
    const double rad2bam = double(1 << 30) / M_PI;
    double dbam = ang * rad2bam;
    // Convert to signed first, since negative double to unsigned is undefined.
    return angle_t(int(dbam)) << 1;
}

// The builder warns about degenerate geometry in the map.  Those are the
// mapper's problem, not ours, and the cabinet runs unattended, so swallow
// them rather than spraying the console on every level load.
void Warn( const char * format, ... )
{
    (void) format;
}

// ---------------------------------------------------------------------------
// The bits of FLevel the builder needs, without processor.cpp's file I/O.
// ---------------------------------------------------------------------------

FLevel::FLevel()
{
    memset(this, 0, sizeof(*this));
}

FLevel::~FLevel()
{
    // The builder's output is handed to the caller, so nothing here owns it.
    // Only the vertex array we filled in is ours.
    if (Vertices)  delete[] Vertices;
    Vertices = NULL;
}

void FLevel::FindMapBounds()
{
    fixed_t minx, maxx, miny, maxy;

    if (NumVertices < 1)
    {
        MinX = MinY = MaxX = MaxY = 0;
        return;
    }

    minx = maxx = Vertices[0].x;
    miny = maxy = Vertices[0].y;

    for (int i = 1; i < NumVertices; ++i)
    {
        if (Vertices[i].x < minx)       minx = Vertices[i].x;
        else if (Vertices[i].x > maxx)  maxx = Vertices[i].x;
        if (Vertices[i].y < miny)       miny = Vertices[i].y;
        else if (Vertices[i].y > maxy)  maxy = Vertices[i].y;
    }

    MinX = minx;
    MinY = miny;
    MaxX = maxx;
    MaxY = maxy;
}

// Never called by the node builder; defined so the vendored objects link.
void FLevel::RemoveExtraLines()   {}
void FLevel::RemoveExtraSides()   {}
void FLevel::RemoveExtraSectors() {}

// ---------------------------------------------------------------------------

extern "C" int NB_Build( nb_level_t * lv )
{
    if (lv == NULL)  return 0;

    lv->out_vertexes = NULL;    lv->num_out_vertexes = 0;
    lv->out_segs = NULL;        lv->num_out_segs = 0;
    lv->out_subsectors = NULL;  lv->num_out_subsectors = 0;
    lv->out_nodes = NULL;       lv->num_out_nodes = 0;
    lv->out_line_v1 = NULL;     lv->out_line_v2 = NULL;

    if (lv->num_in_vertexes < 3 || lv->num_in_lines < 1 || lv->num_in_sides < 1)
        return 0;

    FLevel level;

    level.NumVertices = (int) lv->num_in_vertexes;
    level.Vertices = new WideVertex[lv->num_in_vertexes];
    for (uint32_t i = 0; i < lv->num_in_vertexes; ++i)
    {
        level.Vertices[i].x = lv->in_vertexes[i].x;
        level.Vertices[i].y = lv->in_vertexes[i].y;
        level.Vertices[i].index = 0;
    }
    level.NumOrgVerts = level.NumVertices;

    for (uint32_t i = 0; i < lv->num_in_sides; ++i)
    {
        IntSideDef sd;
        memset(&sd, 0, sizeof(sd));
        sd.sector = lv->in_sides[i].sector;
        level.Sides.Push(sd);
    }

    for (uint32_t i = 0; i < lv->num_in_lines; ++i)
    {
        IntLineDef ld;
        memset(&ld, 0, sizeof(ld));
        ld.v1 = lv->in_lines[i].v1;
        ld.v2 = lv->in_lines[i].v2;
        ld.sidenum[0] = (lv->in_lines[i].sidenum[0] == NB_NO_SIDE)
                        ? NO_INDEX : lv->in_lines[i].sidenum[0];
        ld.sidenum[1] = (lv->in_lines[i].sidenum[1] == NB_NO_SIDE)
                        ? NO_INDEX : lv->in_lines[i].sidenum[1];
        level.Lines.Push(ld);
    }

    // The builder reads no sector property, only the count.
    for (uint32_t i = 0; i < lv->num_in_sectors; ++i)
    {
        IntSector sec;
        memset(&sec.data, 0, sizeof(sec.data));
        level.Sectors.Push(sec);
    }

    level.FindMapBounds();

    // No polyobjects: this runs on Doom-format maps.
    TArray<FNodeBuilder::FPolyStart> polyspots, anchors;

    WideVertex   * verts = NULL;   int nverts = 0;
    MapNodeEx    * nodes = NULL;   int nnodes = 0;
    MapSegEx     * segs  = NULL;   int nsegs  = 0;
    MapSubsectorEx * subs = NULL;  int nsubs  = 0;

    {
        FNodeBuilder builder(level, polyspots, anchors, "map", false);
        builder.GetVertices(verts, nverts);
        builder.GetNodes(nodes, nnodes, segs, nsegs, subs, nsubs);
    }

    if (nnodes < 1 || nsubs < 1 || nsegs < 1 || nverts < 1)
    {
        if (verts) delete[] verts;
        if (nodes) delete[] nodes;
        if (segs)  delete[] segs;
        if (subs)  delete[] subs;
        return 0;
    }

    // Copy into malloc'd arrays so the C side can free them with NB_Free
    // without needing to match new[]/delete[].
    lv->num_out_vertexes = (uint32_t) nverts;
    lv->out_vertexes = (nb_vertex_t *) malloc(sizeof(nb_vertex_t) * nverts);
    for (int i = 0; i < nverts; ++i)
    {
        lv->out_vertexes[i].x = verts[i].x;
        lv->out_vertexes[i].y = verts[i].y;
    }

    lv->num_out_segs = (uint32_t) nsegs;
    lv->out_segs = (nb_seg_t *) malloc(sizeof(nb_seg_t) * nsegs);
    for (int i = 0; i < nsegs; ++i)
    {
        lv->out_segs[i].v1      = segs[i].v1;
        lv->out_segs[i].v2      = segs[i].v2;
        lv->out_segs[i].angle   = segs[i].angle;
        lv->out_segs[i].linedef = segs[i].linedef;
        lv->out_segs[i].side    = segs[i].side;
        lv->out_segs[i].offset  = segs[i].offset;
    }

    lv->num_out_subsectors = (uint32_t) nsubs;
    lv->out_subsectors = (nb_subsector_t *) malloc(sizeof(nb_subsector_t) * nsubs);
    for (int i = 0; i < nsubs; ++i)
    {
        lv->out_subsectors[i].numlines  = subs[i].numlines;
        lv->out_subsectors[i].firstline = subs[i].firstline;
    }

    lv->num_out_nodes = (uint32_t) nnodes;
    lv->out_nodes = (nb_node_t *) malloc(sizeof(nb_node_t) * nnodes);
    for (int i = 0; i < nnodes; ++i)
    {
        lv->out_nodes[i].x  = nodes[i].x;
        lv->out_nodes[i].y  = nodes[i].y;
        lv->out_nodes[i].dx = nodes[i].dx;
        lv->out_nodes[i].dy = nodes[i].dy;
        for (int j = 0; j < 2; ++j)
            for (int k = 0; k < 4; ++k)
                lv->out_nodes[i].bbox[j][k] = nodes[i].bbox[j][k];
        // ZDBSP marks a subsector child with NFX_SUBSECTOR (0x80000000),
        // which is what NB_SUBSECTOR is; pass it through unchanged.
        lv->out_nodes[i].children[0] = nodes[i].children[0];
        lv->out_nodes[i].children[1] = nodes[i].children[1];
    }

    // FindUsedVertices rewrote each line's v1/v2 to index the new vertex
    // list; hand that mapping back so the caller can re-point its linedefs.
    lv->out_line_v1 = (uint32_t *) malloc(sizeof(uint32_t) * lv->num_in_lines);
    lv->out_line_v2 = (uint32_t *) malloc(sizeof(uint32_t) * lv->num_in_lines);
    if (lv->out_line_v1 && lv->out_line_v2)
    {
        for (uint32_t i = 0; i < lv->num_in_lines; ++i)
        {
            lv->out_line_v1[i] = level.Lines[i].v1;
            lv->out_line_v2[i] = level.Lines[i].v2;
        }
    }

    delete[] verts;
    delete[] nodes;
    delete[] segs;
    delete[] subs;

    if (lv->out_vertexes == NULL || lv->out_segs == NULL
        || lv->out_subsectors == NULL || lv->out_nodes == NULL
        || lv->out_line_v1 == NULL || lv->out_line_v2 == NULL)
    {
        NB_Free(lv);
        return 0;
    }
    return 1;
}

extern "C" void NB_Free( nb_level_t * lv )
{
    if (lv == NULL)  return;
    free(lv->out_vertexes);    lv->out_vertexes = NULL;    lv->num_out_vertexes = 0;
    free(lv->out_segs);        lv->out_segs = NULL;        lv->num_out_segs = 0;
    free(lv->out_subsectors);  lv->out_subsectors = NULL;  lv->num_out_subsectors = 0;
    free(lv->out_nodes);       lv->out_nodes = NULL;       lv->num_out_nodes = 0;
    free(lv->out_line_v1);     lv->out_line_v1 = NULL;
    free(lv->out_line_v2);     lv->out_line_v2 = NULL;
}
