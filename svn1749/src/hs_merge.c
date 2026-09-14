// [Arcade] Shared high scores: the merge.  See hs_merge.h.
//
// No engine includes on purpose -- tools/hsmerge-test.py compiles this file by
// itself.  Every comparison below is a total order over the fields it is
// given; the merge depends on that for commutativity and associativity, and
// the test checks it rather than trusting this comment.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hs_merge.h"

int  HSM_Map_Order( const char * mapname )
{
    int  e, m;

    // Doom 2 hides MAP31/32 behind MAP15's secret exit and returns to MAP16
    // afterwards, so plain numeric order would list them ten levels from where
    // they are played: the run is 15 -> 31 -> 32 -> 16.  Doom 1 is episode
    // major, map minor; E?M9 is a secret level too but sits at the end of its
    // episode, which is where numeric order already puts it.
    if( sscanf(mapname, "MAP%d", &m) == 1 )
    {
        if( m <= 15 )  return m;          // 1..15
        if( m == 31 )  return 16;         // secret, straight after MAP15
        if( m == 32 )  return 17;         // secret, then back out to MAP16
        if( m <= 30 )  return m + 2;      // 16..30 shifted past the two above
        return m + 100;                   // MAP33+, if a pack has them
    }

    if( sscanf(mapname, "E%dM%d", &e, &m) == 2 )
        return (e * 100) + m;

    return 100000;   // unrecognized: park it at the end
}

int  HSM_Episode_Of( const char * mapname )
{
    int e, m;
    if( sscanf(mapname, "E%dM%d", &e, &m) == 2 )  return e;
    return 1;
}

int  HSM_Id_Is_Single( const char * game )
{
    size_t n = strnlen( game, HSM_GAME_LEN );
    return (n >= 3) && (strncmp(game + n - 3, "-sl", 3) == 0);
}

int  HSM_Board_Depth( int single )
{
    return single ? HSM_DEPTH_SINGLE : HSM_DEPTH_SURVIVAL;
}

static int  hsm_int_cmp( long a, long b )
{
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

// ---------------------------------------------------------------------------
//  Split records

static int  hsm_split_key_cmp( const hsm_split_t * a, const hsm_split_t * b )
{
    int c = strncmp( a->game, b->game, HSM_GAME_LEN );
    if( c )  return c;
    c = hsm_int_cmp( HSM_Map_Order(a->map), HSM_Map_Order(b->map) );
    if( c )  return c;
    c = strncmp( a->map, b->map, HSM_MAP_LEN );
    if( c )  return c;
    c = hsm_int_cmp( a->cat, b->cat );
    if( c )  return c;
    return hsm_int_cmp( a->skill, b->skill );
}

int  HSM_Split_Cmp( const hsm_split_t * a, const hsm_split_t * b )
{
    int c = hsm_int_cmp( a->tics, b->tics );
    if( c )  return c;
    c = hsm_int_cmp( a->set_time, b->set_time );   // earlier (or unknown) first
    if( c )  return c;
    c = strncmp( a->cab, b->cab, HSM_CAB_LEN );
    if( c )  return c;
    c = strncmp( a->startmap, b->startmap, HSM_MAP_LEN );
    if( c )  return c;
    return memcmp( a->sha, b->sha, HSM_SHA_LEN );
}

static int  hsm_split_sort( const void * pa, const void * pb )
{
    const hsm_split_t * a = pa, * b = pb;
    int c = hsm_split_key_cmp( a, b );
    return c ? c : HSM_Split_Cmp( a, b );
}

// ---------------------------------------------------------------------------
//  Run board entries

int  HSM_Same_Board( const hsm_run_t * a, const hsm_run_t * b )
{
    if( a->skill != b->skill || a->cat != b->cat )  return 0;
    if( strncmp(a->game, b->game, HSM_GAME_LEN) != 0 )  return 0;
    // A single level board is per map.  A Survival board is per episode.
    if( HSM_Id_Is_Single(a->game) )
        return strncmp(a->endmap, b->endmap, HSM_MAP_LEN) == 0;
    return HSM_Episode_Of(a->endmap) == HSM_Episode_Of(b->endmap);
}

// The board an entry is on, as an order: the same boards are adjacent, and
// HSM_Same_Board is exactly "this compares equal".
static int  hsm_board_cmp( const hsm_run_t * a, const hsm_run_t * b )
{
    int c = strncmp( a->game, b->game, HSM_GAME_LEN );
    if( c )  return c;
    if( HSM_Id_Is_Single(a->game) )
    {
        c = hsm_int_cmp( HSM_Map_Order(a->endmap), HSM_Map_Order(b->endmap) );
        if( c )  return c;
        c = strncmp( a->endmap, b->endmap, HSM_MAP_LEN );
    }
    else
        c = hsm_int_cmp( HSM_Episode_Of(a->endmap), HSM_Episode_Of(b->endmap) );
    if( c )  return c;
    c = hsm_int_cmp( a->skill, b->skill );
    if( c )  return c;
    return hsm_int_cmp( a->cat, b->cat );
}

int  HSM_Run_Rank_Cmp( const hsm_run_t * a, const hsm_run_t * b )
{
    int c;
    // Survival runs rank furthest first; single level runs are all one map.
    if( ! HSM_Id_Is_Single(a->game) )
    {
        c = hsm_int_cmp( HSM_Map_Order(b->endmap), HSM_Map_Order(a->endmap) );
        if( c )  return c;
    }
    c = hsm_int_cmp( a->tics, b->tics );
    if( c )  return c;
    c = hsm_int_cmp( a->set_time, b->set_time );
    if( c )  return c;
    c = strncmp( a->cab, b->cab, HSM_CAB_LEN );
    if( c )  return c;
    c = strncmp( a->endmap, b->endmap, HSM_MAP_LEN );
    if( c )  return c;
    c = strncmp( a->startmap, b->startmap, HSM_MAP_LEN );
    if( c )  return c;
    return memcmp( a->sha, b->sha, HSM_SHA_LEN );
}

// Board, then rank, then the *larger* initials first: an entry both cabinets
// hold, one with initials and one still waiting for them, sorts with the
// initials version in front, and the dedupe below keeps the front one.
static int  hsm_run_sort( const void * pa, const void * pb )
{
    const hsm_run_t * a = pa, * b = pb;
    int c = hsm_board_cmp( a, b );
    if( c )  return c;
    c = HSM_Run_Rank_Cmp( a, b );
    if( c )  return c;
    return -strncmp( a->initials, b->initials, HSM_INI_LEN );
}

// ---------------------------------------------------------------------------

int  HSM_Merge( const hsm_set_t * a, const hsm_set_t * b, hsm_set_t * out )
{
    const hsm_set_t * side[2] = { a, b };
    int  s, i, n;

    // A clear on the master moves the epoch to the time of the clear.  A set
    // still under an older epoch has not seen that clear, so of its records
    // only those set since then are kept -- everywhere, whichever side merges.
    // Filtering on the larger epoch alone is what keeps this associative: a
    // later, larger epoch filters again, and a record that passed a larger
    // filter passes every smaller one.
    uint32_t  keep_from[2];
    uint32_t  epoch = ( a->epoch > b->epoch ) ? a->epoch : b->epoch;
    keep_from[0] = ( a->epoch < epoch ) ? epoch : 0;
    keep_from[1] = ( b->epoch < epoch ) ? epoch : 0;
    out->epoch = epoch;

    // --- splits: the best record of each key
    n = 0;
    for( s = 0; s < 2; s++ )
    {
        for( i = 0; i < side[s]->nsplits; i++ )
        {
            if( keep_from[s] && side[s]->splits[i].set_time < keep_from[s] )  continue;
            if( n >= out->max_splits )  return -1;
            out->splits[n++] = side[s]->splits[i];
        }
    }
    if( n > 1 )
        qsort( out->splits, n, sizeof(hsm_split_t), hsm_split_sort );
    out->nsplits = 0;
    for( i = 0; i < n; i++ )
    {
        if( out->nsplits > 0
            && hsm_split_key_cmp( &out->splits[out->nsplits-1], &out->splits[i] ) == 0 )
            continue;   // the key's best is already in
        if( out->nsplits != i )
            out->splits[out->nsplits] = out->splits[i];
        out->nsplits++;
    }

    // --- boards: the union, each entry once, trimmed to depth
    n = 0;
    for( s = 0; s < 2; s++ )
    {
        for( i = 0; i < side[s]->nruns; i++ )
        {
            if( keep_from[s] && side[s]->runs[i].set_time < keep_from[s] )  continue;
            if( n >= out->max_runs )  return -1;
            out->runs[n++] = side[s]->runs[i];
        }
    }
    if( n > 1 )
        qsort( out->runs, n, sizeof(hsm_run_t), hsm_run_sort );
    out->nruns = 0;
    {
        int  placed = 0;   // entries kept on the current board
        for( i = 0; i < n; i++ )
        {
            const hsm_run_t * r = &out->runs[i];
            if( out->nruns > 0 )
            {
                const hsm_run_t * last = &out->runs[out->nruns-1];
                if( hsm_board_cmp( last, r ) == 0 )
                {
                    if( HSM_Run_Rank_Cmp( last, r ) == 0 )
                        continue;   // the same entry again, with no better initials
                }
                else
                    placed = 0;
            }
            if( placed >= HSM_Board_Depth( HSM_Id_Is_Single(r->game) ) )
                continue;   // off the bottom of this board
            if( out->nruns != i )
                out->runs[out->nruns] = *r;
            out->nruns++;
            placed++;
        }
    }
    return 0;
}

int  HSM_Normalize( hsm_set_t * a, hsm_set_t * scratch )
{
    hsm_set_t  none;
    memset( &none, 0, sizeof(none) );
    none.epoch = a->epoch;
    if( HSM_Merge( a, &none, scratch ) != 0 )  return -1;
    if( scratch->nsplits > a->max_splits || scratch->nruns > a->max_runs )  return -1;
    memcpy( a->splits, scratch->splits, scratch->nsplits * sizeof(hsm_split_t) );
    memcpy( a->runs, scratch->runs, scratch->nruns * sizeof(hsm_run_t) );
    a->nsplits = scratch->nsplits;
    a->nruns = scratch->nruns;
    return 0;
}

int  HSM_Equal( const hsm_set_t * a, const hsm_set_t * b )
{
    int i;
    if( a->epoch != b->epoch || a->nsplits != b->nsplits || a->nruns != b->nruns )
        return 0;
    for( i = 0; i < a->nsplits; i++ )
        if( hsm_split_sort( &a->splits[i], &b->splits[i] ) != 0 )  return 0;
    for( i = 0; i < a->nruns; i++ )
        if( hsm_run_sort( &a->runs[i], &b->runs[i] ) != 0 )  return 0;
    return 1;
}
