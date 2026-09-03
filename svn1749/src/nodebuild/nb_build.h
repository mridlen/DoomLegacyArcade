// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// [Arcade] C interface to the vendored ZDBSP node builder.
//
// Deliberately mentions neither DoomLegacy's types nor ZDBSP's: both declare a
// node_t, so no translation unit may see both headers.  p_setup.c fills these
// plain structs from the loaded map and reads the results back; nb_build.cpp
// is the only file that includes ZDBSP.
//
// Why rebuild at all: the WAD's NODES lump stores each partition as a rounded
// copy of the seg the builder split on, and the rounding is enough to put a
// viewpoint a unit or two from a partition on the wrong side.  The BSP is then
// walked out of order and a far wall occludes a near one -- the slime trail.
// ZDBSP keeps its partitions and vertices in fixed point, so the precision is
// never lost.  See docs/arcade/gotchas.md.
//
//-----------------------------------------------------------------------------

#ifndef NB_BUILD_H
#define NB_BUILD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// All coordinates are fixed_t (16.16), same as the engine uses.
typedef struct {
    int32_t   x, y;
} nb_vertex_t;

typedef struct {
    uint32_t  v1, v2;          // index into in_vertexes
    uint32_t  sidenum[2];      // NB_NO_SIDE when absent
} nb_line_t;

typedef struct {
    int32_t   sector;          // index into the engine's sectors
} nb_side_t;

typedef struct {
    uint32_t  v1, v2;          // index into out_vertexes
    uint16_t  angle;
    uint16_t  linedef;
    int16_t   side;
    int16_t   offset;
} nb_seg_t;

typedef struct {
    uint32_t  numlines, firstline;
} nb_subsector_t;

typedef struct {
    int32_t   x, y, dx, dy;    // full precision, unlike the NODES lump
    int16_t   bbox[2][4];
    uint32_t  children[2];     // NB_SUBSECTOR flags a subsector index
} nb_node_t;

#define NB_NO_SIDE    0xFFFFFFFFu
#define NB_SUBSECTOR  0x80000000u

typedef struct {
    // input, owned by the caller
    const nb_vertex_t * in_vertexes;
    uint32_t            num_in_vertexes;
    const nb_line_t   * in_lines;
    uint32_t            num_in_lines;
    const nb_side_t   * in_sides;
    uint32_t            num_in_sides;
    uint32_t            num_in_sectors;

    // output, allocated by NB_Build and released by NB_Free
    nb_vertex_t       * out_vertexes;
    uint32_t            num_out_vertexes;
    nb_seg_t          * out_segs;
    uint32_t            num_out_segs;
    nb_subsector_t    * out_subsectors;
    uint32_t            num_out_subsectors;
    nb_node_t         * out_nodes;
    uint32_t            num_out_nodes;

    // The builder de-duplicates the vertices it is given and keeps only the
    // ones a linedef uses, so out_vertexes is NOT the input array with extras
    // appended.  These are each input line's v1/v2 re-expressed as indices
    // into out_vertexes; num_in_lines entries.
    uint32_t          * out_line_v1;
    uint32_t          * out_line_v2;
} nb_level_t;

// Returns 1 on success, 0 on failure (the level is left with no output).
// On success the caller must eventually call NB_Free.
int  NB_Build( nb_level_t * lv );
void NB_Free ( nb_level_t * lv );

#ifdef __cplusplus
}
#endif

#endif // NB_BUILD_H
