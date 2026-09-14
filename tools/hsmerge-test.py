#!/usr/bin/env python3
"""Test the shared high score merge (svn1749/src/hs_merge.c) without a cabinet.

Two cabinets sync by each running HSM_Merge over its own scores and the other's.
They only end up with the same boards if that merge is commutative, idempotent
and associative -- and those only hold if every comparison is a total order.
None of that is visible from a couple of play sessions: a tie between two
cabinets, or an entry that got its initials on one cabinet after the other had
already copied it, is exactly the case that breaks and exactly the one nobody
happens to play.

So this compiles the real hs_merge.c (it has no engine includes on purpose)
with a generated C harness and drives it:
  * the laws, on many random score sets drawn from a deliberately tiny domain
    (two tick counts, three cabinets, two set times) so ties and duplicates are
    the common case, not the rare one: merge(A,B) == merge(B,A),
    merge(merge(A,B),A) == merge(A,B), merge(merge(A,B),C) == merge(A,merge(B,C)),
    normalize is idempotent, every board within depth and in rank order,
    one record per split key, and the comparisons antisymmetric;
  * pinned cases for each rule the laws cannot see: equal times break on the
    set time and then the cabinet, an old (unknown time) record keeps its place,
    Survival ranks furthest first, initials entered on either cabinet survive,
    a clear on the master wipes what another cabinet held from before it but
    keeps what it set since, a full output reports -1.

Run it from anywhere:  tools/hsmerge-test.py
  --selfcheck   break each rule in a copy of hs_merge.c and report whether the
                test goes red for it.  A check never shown to fail is not
                evidence (CLAUDE.md, headless verification).
"""
import os, subprocess, sys, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRCDIR = os.path.join(HERE, os.pardir, 'svn1749', 'src')
MERGE_C = os.path.join(SRCDIR, 'hs_merge.c')
MERGE_H = os.path.join(SRCDIR, 'hs_merge.h')


def rules_check(linkscore_src=None):
    """Every gameplay setting a record demo's header carries must either be
    pinned by the ranked ruleset (hs_ranked_rules[], hs_stuff.c) or be part of
    the sync's "same rules" hash (lks_rules_hash, d_linkscore.c).  Otherwise
    two cabinets playing under different values would share one board, and
    nothing would say so.  Returns a list of problems."""
    import re
    g = open(os.path.join(SRCDIR, 'g_game.c'), encoding='latin-1').read()
    body = g[g.index('void G_BeginRecording'):]
    body = body[body.index('byte * demo_p_next'):body.index('Sync mark')]
    header = set(re.findall(r'\b(cv_\w+)\.(?:EV|value)', body))
    hs = open(os.path.join(SRCDIR, 'hs_stuff.c'), encoding='latin-1').read()
    tbl = hs[hs.index('hs_ranked_rules[] ='):]
    pinned = set(re.findall(r'&(cv_\w+)', tbl[:tbl.index('\n};')]))
    ls = linkscore_src if linkscore_src is not None else open(os.path.join(SRCDIR, 'd_linkscore.c')).read()
    fn = ls[ls.index('static void  lks_rules_hash'):]
    hashed = set(re.findall(r'(cv_\w+)\.EV', fn[:fn.index('\n}')]))
    return ['%s is in the demo header but neither pinned by the ranked ruleset nor in lks_rules_hash' % cv
            for cv in sorted(header - pinned - hashed)]

HARNESS = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hs_merge.h"

#define CAP 256
static unsigned long rng = 12345;
static unsigned rnd( unsigned n ) { rng = rng * 6364136223846793005ul + 1442695040888963407ul; return (unsigned)(rng >> 33) % n; }

static int fails = 0;
#define CHECK(cond, ...) do { if( !(cond) ) { if( fails < 12 ) { printf("FAIL: " __VA_ARGS__); printf("\n"); } fails++; } } while(0)

typedef struct { hsm_split_t s[CAP]; hsm_run_t r[CAP]; hsm_set_t set; } box_t;
static void box_init( box_t * b ) { memset( b, 0, sizeof(*b) ); b->set.splits = b->s; b->set.runs = b->r; b->set.max_splits = CAP; b->set.max_runs = CAP; }

static const char * games[] = { "doomu", "doomu-sl" };
static const char * maps[]  = { "E1M1", "E1M2", "E2M1", "E1M9" };
static const char * cabs[]  = { "", "AAAA-1111", "BBBB-2222" };
static const char * inis[]  = { "", "AAA", "ZZZ" };
static const unsigned times[] = { 0, HSM_TIME_VALID_FROM + 5, HSM_TIME_VALID_FROM + 9 };

static void rand_split( hsm_split_t * s )
{
    memset( s, 0, sizeof(*s) );
    strcpy( s->game, games[rnd(2)] );
    strcpy( s->map, maps[rnd(4)] );
    s->skill = rnd(2); s->cat = rnd(2);
    s->tics = 100 + 100 * rnd(2);
    strcpy( s->startmap, rnd(2) ? "E1M1" : "" );
    s->set_time = times[rnd(3)];
    strcpy( s->cab, cabs[rnd(3)] );
    if( rnd(2) ) memset( s->sha, 0x11, HSM_SHA_LEN );
}
static void rand_run( hsm_run_t * r )
{
    memset( r, 0, sizeof(*r) );
    strcpy( r->game, games[rnd(2)] );
    strcpy( r->endmap, maps[rnd(4)] );
    strcpy( r->startmap, rnd(2) ? "E1M1" : r->endmap );
    r->skill = rnd(2); r->cat = rnd(2);
    r->tics = 100 + 100 * rnd(2);
    strcpy( r->initials, inis[rnd(3)] );
    r->set_time = times[rnd(3)];
    strcpy( r->cab, cabs[rnd(3)] );
    if( rnd(2) ) memset( r->sha, 0x22, HSM_SHA_LEN );
}
static void rand_box( box_t * b, int epoch_choices )
{
    int i;
    box_init( b );
    // Epochs between the set times, so a clear drops some records and keeps others.
    { static const unsigned ep[] = { 0, HSM_TIME_VALID_FROM + 7, HSM_TIME_VALID_FROM + 9 }; b->set.epoch = ep[ rnd( epoch_choices ) ]; }
    b->set.nsplits = rnd(24);
    for( i = 0; i < b->set.nsplits; i++ ) rand_split( &b->s[i] );
    b->set.nruns = rnd(24);
    for( i = 0; i < b->set.nruns; i++ ) rand_run( &b->r[i] );
}

static box_t A, B, C, M1, M2, M3, M4, T;

static void merge( const box_t * x, const box_t * y, box_t * out )
{
    box_init( out );
    if( HSM_Merge( &x->set, &y->set, &out->set ) != 0 ) { printf("FAIL: merge overflow\n"); fails++; }
}

// Structural properties of any merge result.
static void check_shape( const box_t * m, const char * what )
{
    int i, j;
    for( i = 0; i < m->set.nsplits; i++ )
        for( j = i + 1; j < m->set.nsplits; j++ )
            CHECK( !( strcmp(m->s[i].game, m->s[j].game) == 0 && strcmp(m->s[i].map, m->s[j].map) == 0
                      && m->s[i].cat == m->s[j].cat && m->s[i].skill == m->s[j].skill ),
                   "%s: split key %s %s twice", what, m->s[i].game, m->s[i].map );
    for( i = 0; i < m->set.nruns; i++ )
    {
        int n = 0;
        for( j = 0; j < m->set.nruns; j++ )
            if( HSM_Same_Board( &m->r[i], &m->r[j] ) )
            {
                n++;
                if( j > i ) CHECK( HSM_Run_Rank_Cmp( &m->r[i], &m->r[j] ) < 0, "%s: board out of rank order", what );
            }
        CHECK( n <= HSM_Board_Depth( HSM_Id_Is_Single(m->r[i].game) ), "%s: board over depth (%d)", what, n );
    }
}

// Nothing dropped from a board outranks something kept, and every split kept
// is the best of its key.
static void check_best( const box_t * x, const box_t * y, const box_t * m )
{
    const box_t * in[2] = { x, y };
    int s, i, j;
    for( s = 0; s < 2; s++ )
    {
        int behind = in[s]->set.epoch < m->set.epoch;   // this side missed a clear
        for( i = 0; i < in[s]->set.nsplits; i++ )
        {
            const hsm_split_t * a = &in[s]->s[i];
            if( behind && a->set_time < m->set.epoch ) continue;
            for( j = 0; j < m->set.nsplits; j++ )
                if( !strcmp(a->game, m->s[j].game) && !strcmp(a->map, m->s[j].map) && a->cat == m->s[j].cat && a->skill == m->s[j].skill )
                    break;
            CHECK( j < m->set.nsplits, "split key lost" );
            if( j < m->set.nsplits ) CHECK( HSM_Split_Cmp( &m->s[j], a ) <= 0, "kept split is not the best" );
        }
        for( i = 0; i < in[s]->set.nruns; i++ )
        {
            const hsm_run_t * a = &in[s]->r[i];
            int found = 0, worst = -1;
            if( behind && a->set_time < m->set.epoch ) continue;
            for( j = 0; j < m->set.nruns; j++ )
                if( HSM_Same_Board( a, &m->r[j] ) )
                {
                    worst = j;
                    if( HSM_Run_Rank_Cmp( a, &m->r[j] ) == 0 )
                    {
                        found = 1;
                        CHECK( strcmp( m->r[j].initials, a->initials ) >= 0, "initials lost in merge" );
                    }
                }
            if( ! found )
                CHECK( worst >= 0 && HSM_Run_Rank_Cmp( &m->r[worst], a ) < 0, "a dropped entry outranks a kept one" );
        }
    }
}

static void law_checks( int iters )
{
    int it;
    for( it = 0; it < iters; it++ )
    {
        rand_box( &A, 3 ); rand_box( &B, 3 ); rand_box( &C, 3 );
        merge( &A, &B, &M1 ); merge( &B, &A, &M2 );
        CHECK( HSM_Equal( &M1.set, &M2.set ), "not commutative (iteration %d)", it );
        check_shape( &M1, "merge" );
        check_best( &A, &B, &M1 );
        merge( &M1, &A, &M3 );
        CHECK( HSM_Equal( &M1.set, &M3.set ), "not idempotent: merge(merge(A,B),A) != merge(A,B) (iteration %d)", it );
        merge( &M1, &M1, &M3 );
        CHECK( HSM_Equal( &M1.set, &M3.set ), "not idempotent: merge(M,M) != M" );
        // Associative among sets on one epoch.  Across a clear it is not, and
        // cannot be: merging two sets trims a board, which can drop a newer,
        // slower entry that a later clear would have left standing.  That only
        // bites on the first sync after a clear -- from then on every cabinet
        // holds the new epoch and this is the plain case again -- so what is
        // checked across epochs is the rule the first sync applies: a merge is
        // both sides filtered to the new epoch, then merged there.
        B.set.epoch = C.set.epoch = A.set.epoch;
        merge( &A, &B, &M1 );
        merge( &M1, &C, &M3 );            // (A+B)+C
        merge( &B, &C, &M4 ); merge( &A, &M4, &M2 );   // A+(B+C)
        CHECK( HSM_Equal( &M3.set, &M2.set ), "not associative on one epoch (iteration %d)", it );
        rand_box( &B, 3 );
        {
            uint32_t E = A.set.epoch > B.set.epoch ? A.set.epoch : B.set.epoch;
            box_t * sides[2] = { &M3, &M4 };
            const box_t * from[2] = { &A, &B };
            int k, i;
            for( k = 0; k < 2; k++ )
            {
                box_init( sides[k] );
                sides[k]->set.epoch = E;
                for( i = 0; i < from[k]->set.nsplits; i++ )
                    if( from[k]->set.epoch == E || from[k]->s[i].set_time >= E ) sides[k]->s[ sides[k]->set.nsplits++ ] = from[k]->s[i];
                for( i = 0; i < from[k]->set.nruns; i++ )
                    if( from[k]->set.epoch == E || from[k]->r[i].set_time >= E ) sides[k]->r[ sides[k]->set.nruns++ ] = from[k]->r[i];
            }
            merge( &A, &B, &M1 );
            merge( &M3, &M4, &M2 );
            CHECK( HSM_Equal( &M1.set, &M2.set ), "a merge across a clear is not the filtered merge (iteration %d)", it );
            check_best( &A, &B, &M1 );
        }
        // normalize == merge with nothing, and is idempotent
        T = A; T.set.splits = T.s; T.set.runs = T.r;
        box_init( &M4 );
        CHECK( HSM_Normalize( &T.set, &M4.set ) == 0, "normalize failed" );
        { box_t e; box_init( &e ); e.set.epoch = A.set.epoch; merge( &A, &e, &M2 ); }
        CHECK( HSM_Equal( &T.set, &M2.set ), "normalize differs from merge with nothing" );
        box_init( &M4 ); CHECK( HSM_Normalize( &T.set, &M4.set ) == 0, "normalize failed" );
        CHECK( HSM_Equal( &T.set, &M2.set ), "normalize not idempotent" );
    }
    // Antisymmetry of both orders over random pairs.
    for( it = 0; it < iters * 4; it++ )
    {
        hsm_split_t s1, s2; hsm_run_t r1, r2;
        rand_split( &s1 ); rand_split( &s2 );
        int a = HSM_Split_Cmp( &s1, &s2 ), b = HSM_Split_Cmp( &s2, &s1 );
        CHECK( (a < 0) == (b > 0) && (a == 0) == (b == 0), "split order not antisymmetric" );
        if( a == 0 ) CHECK( s1.tics == s2.tics && s1.set_time == s2.set_time && !strcmp(s1.cab, s2.cab) && !strcmp(s1.startmap, s2.startmap) && !memcmp(s1.sha, s2.sha, HSM_SHA_LEN), "split order says equal for different records" );
        rand_run( &r1 ); rand_run( &r2 );
        if( HSM_Same_Board( &r1, &r2 ) )
        {
            a = HSM_Run_Rank_Cmp( &r1, &r2 ); b = HSM_Run_Rank_Cmp( &r2, &r1 );
            CHECK( (a < 0) == (b > 0) && (a == 0) == (b == 0), "run order not antisymmetric" );
            if( a == 0 ) CHECK( r1.tics == r2.tics && r1.set_time == r2.set_time && !strcmp(r1.cab, r2.cab) && !strcmp(r1.startmap, r2.startmap) && !strcmp(r1.endmap, r2.endmap) && !memcmp(r1.sha, r2.sha, HSM_SHA_LEN), "run order says equal for different entries" );
        }
    }
}

static hsm_run_t mkrun( const char * game, const char * start, const char * end, unsigned tics, unsigned t, const char * cab, const char * ini )
{
    hsm_run_t r; memset( &r, 0, sizeof(r) );
    strcpy( r.game, game ); strcpy( r.startmap, start ); strcpy( r.endmap, end );
    r.tics = tics; r.set_time = t; strcpy( r.cab, cab ); strcpy( r.initials, ini );
    return r;
}
static hsm_split_t mksplit( unsigned tics, unsigned t, const char * cab )
{
    hsm_split_t s; memset( &s, 0, sizeof(s) );
    strcpy( s.game, "doomu-sl" ); strcpy( s.map, "E1M1" ); strcpy( s.startmap, "E1M1" );
    s.tics = tics; s.set_time = t; strcpy( s.cab, cab );
    return s;
}

static void pinned_checks( void )
{
    unsigned t0 = HSM_TIME_VALID_FROM;
    box_init( &A ); box_init( &B );

    // Equal times: the earlier set time wins, whichever cabinet merges.
    A.s[0] = mksplit( 500, t0 + 50, "AAAA-1111" ); A.set.nsplits = 1;
    B.s[0] = mksplit( 500, t0 + 10, "BBBB-2222" ); B.set.nsplits = 1;
    merge( &A, &B, &M1 ); merge( &B, &A, &M2 );
    CHECK( M1.set.nsplits == 1 && !strcmp( M1.s[0].cab, "BBBB-2222" ), "equal tics: the earlier record did not win" );
    CHECK( M2.set.nsplits == 1 && !strcmp( M2.s[0].cab, "BBBB-2222" ), "equal tics: the earlier record did not win (other order)" );
    // Equal time and set time: the cabinet id settles it.
    B.s[0].set_time = t0 + 50;
    merge( &B, &A, &M1 );
    CHECK( M1.set.nsplits == 1 && !strcmp( M1.s[0].cab, "AAAA-1111" ), "full tie not settled by cabinet id" );
    // A faster time beats an older one.
    B.s[0] = mksplit( 499, t0 + 99, "BBBB-2222" );
    merge( &A, &B, &M1 );
    CHECK( M1.set.nsplits == 1 && M1.s[0].tics == 499, "faster time did not win" );
    // An old record (unknown set time) keeps its place on a tie.
    B.s[0] = mksplit( 500, 0, "BBBB-2222" );
    merge( &A, &B, &M1 );
    CHECK( M1.set.nsplits == 1 && M1.s[0].set_time == 0, "an old record lost a tie to a newer one" );

    // Survival: further beats faster.
    box_init( &A ); box_init( &B );
    A.r[0] = mkrun( "doomu", "E1M1", "E1M6", 9000, t0, "AAAA-1111", "SLO" ); A.set.nruns = 1;
    B.r[0] = mkrun( "doomu", "E1M1", "E1M3", 1000, t0, "BBBB-2222", "FST" ); B.set.nruns = 1;
    merge( &A, &B, &M1 );
    CHECK( M1.set.nruns == 1 && !strcmp( M1.r[0].endmap, "E1M6" ), "Survival did not rank furthest first" );
    // Doom 2: MAP31 comes after MAP15, before MAP16.
    CHECK( HSM_Map_Order("MAP31") > HSM_Map_Order("MAP15") && HSM_Map_Order("MAP31") < HSM_Map_Order("MAP16"), "MAP31 progression order" );

    // Single level: three deep, fastest first.
    box_init( &A ); box_init( &B );
    A.r[0] = mkrun( "doomu-sl", "E1M1", "E1M1", 300, t0, "AAAA-1111", "AAA" );
    A.r[1] = mkrun( "doomu-sl", "E1M1", "E1M1", 100, t0, "AAAA-1111", "BBB" ); A.set.nruns = 2;
    B.r[0] = mkrun( "doomu-sl", "E1M1", "E1M1", 200, t0, "BBBB-2222", "CCC" );
    B.r[1] = mkrun( "doomu-sl", "E1M1", "E1M1", 400, t0, "BBBB-2222", "DDD" ); B.set.nruns = 2;
    merge( &A, &B, &M1 );
    CHECK( M1.set.nruns == 3 && M1.r[0].tics == 100 && M1.r[1].tics == 200 && M1.r[2].tics == 300, "single level board not the top three" );

    // Initials entered on one cabinet after the other copied the entry.
    box_init( &A ); box_init( &B );
    A.r[0] = mkrun( "doomu-sl", "E1M1", "E1M1", 100, t0, "AAAA-1111", "" ); A.set.nruns = 1;
    B.r[0] = mkrun( "doomu-sl", "E1M1", "E1M1", 100, t0, "AAAA-1111", "MLR" ); B.set.nruns = 1;
    merge( &A, &B, &M1 ); merge( &B, &A, &M2 );
    CHECK( M1.set.nruns == 1 && !strcmp( M1.r[0].initials, "MLR" ), "initials lost (a+b)" );
    CHECK( M2.set.nruns == 1 && !strcmp( M2.r[0].initials, "MLR" ), "initials lost (b+a)" );

    // A clear on the master (a later epoch) wipes what the other side held
    // from before it -- including old records with no set time -- but keeps a
    // record set on that side since the clear.
    box_init( &A ); box_init( &B );
    A.set.epoch = t0 + 100;
    B.set.epoch = 0;
    B.s[0] = mksplit( 1, t0, "BBBB-2222" );
    B.s[1] = mksplit( 1, 0, "BBBB-2222" ); strcpy( B.s[1].map, "E1M2" ); B.set.nsplits = 2;
    B.r[0] = mkrun( "doomu", "E1M1", "E1M8", 1, t0, "BBBB-2222", "OLD" );
    B.r[1] = mkrun( "doomu-sl", "E1M3", "E1M3", 7, t0 + 200, "BBBB-2222", "NEW" ); B.set.nruns = 2;
    merge( &A, &B, &M1 ); merge( &B, &A, &M2 );
    CHECK( M1.set.epoch == t0 + 100 && M1.set.nsplits == 0, "a cleared record came back from before the clear" );
    CHECK( M1.set.nruns == 1 && !strcmp( M1.r[0].initials, "NEW" ), "the clear lost a record set after it, or kept one from before" );
    CHECK( HSM_Equal( &M1.set, &M2.set ), "epoch merge not commutative" );

    // Too small an output says so.
    box_init( &A ); box_init( &B );
    A.s[0] = mksplit( 1, t0, "AAAA-1111" ); A.s[1] = mksplit( 2, t0, "AAAA-1111" ); strcpy( A.s[1].map, "E1M2" ); A.set.nsplits = 2;
    box_init( &M1 ); M1.set.max_splits = 1;
    CHECK( HSM_Merge( &A.set, &B.set, &M1.set ) == -1, "overflow not reported" );
}

int main( int argc, char ** argv )
{
    int iters = argc > 1 ? atoi( argv[1] ) : 3000;
    pinned_checks();
    law_checks( iters );
    if( fails ) { printf( "%d check(s) failed\n", fails ); return 1; }
    printf( "all checks passed (%d random rounds)\n", iters );
    return 0;
}
'''

# Each: (name, old text, new text) -- one rule broken in a copy of hs_merge.c.
BREAKS = [
    ('set time does not break split ties',
     '    c = hsm_int_cmp( a->set_time, b->set_time );   // earlier (or unknown) first\n    if( c )  return c;\n',
     ''),
    ('cabinet id does not break ties',
     '    c = strncmp( a->cab, b->cab, HSM_CAB_LEN );\n    if( c )  return c;\n    c = strncmp( a->endmap, b->endmap, HSM_MAP_LEN );',
     '    c = strncmp( a->endmap, b->endmap, HSM_MAP_LEN );'),
    ('the later set time wins',
     'c = hsm_int_cmp( a->set_time, b->set_time );   // earlier (or unknown) first',
     'c = hsm_int_cmp( b->set_time, a->set_time );'),
    ('Survival ranks fastest, not furthest',
     'c = hsm_int_cmp( HSM_Map_Order(b->endmap), HSM_Map_Order(a->endmap) );',
     'c = hsm_int_cmp( HSM_Map_Order(a->endmap), HSM_Map_Order(b->endmap) );'),
    ('the smaller initials win the dedupe',
     'return -strncmp( a->initials, b->initials, HSM_INI_LEN );',
     'return strncmp( a->initials, b->initials, HSM_INI_LEN );'),
    ('a clear is ignored',
     'if( keep_from[s] && side[s]->runs[i].set_time < keep_from[s] )  continue;',
     ''),
    ('a clear also wipes records set since it',
     'keep_from[0] = ( a->epoch < epoch ) ? epoch : 0;\n    keep_from[1] = ( b->epoch < epoch ) ? epoch : 0;',
     'keep_from[0] = ( a->epoch < epoch ) ? 0xffffffffu : 0;\n    keep_from[1] = ( b->epoch < epoch ) ? 0xffffffffu : 0;'),
    ('the result takes the smaller epoch',
     '    out->epoch = epoch;\n',
     '    out->epoch = ( a->epoch < b->epoch ) ? a->epoch : b->epoch;\n'),
    ('a duplicate takes a board place',
     '                    if( HSM_Run_Rank_Cmp( last, r ) == 0 )\n                        continue;',
     '                    if( 0 )\n                        continue;'),
    ('boards are not trimmed',
     'if( placed >= HSM_Board_Depth( HSM_Id_Is_Single(r->game) ) )',
     'if( placed >= 99 )'),
    ('the worst split of a key is kept',
     'return c ? c : HSM_Split_Cmp( a, b );',
     'return c ? c : -HSM_Split_Cmp( a, b );'),
]


def run(merge_src, iters=3000, quiet=False):
    d = tempfile.mkdtemp(prefix='hsmerge.')
    try:
        shutil.copy(MERGE_H, d)
        open(os.path.join(d, 'hs_merge.c'), 'w').write(merge_src)
        open(os.path.join(d, 'harness.c'), 'w').write(HARNESS)
        exe = os.path.join(d, 'harness')
        cc = subprocess.run(['gcc', '-std=gnu17', '-O1', '-Wall', '-Wextra', '-Wno-unused-parameter',
                             '-I', d, '-o', exe, os.path.join(d, 'hs_merge.c'), os.path.join(d, 'harness.c')],
                            capture_output=True, text=True)
        if cc.returncode != 0:
            print(cc.stdout + cc.stderr)
            return 2, 'compile failed'
        if cc.stderr.strip() and not quiet:
            print(cc.stderr)
        r = subprocess.run([exe, str(iters)], capture_output=True, text=True, timeout=300)
        return r.returncode, r.stdout
    finally:
        shutil.rmtree(d)


def main():
    src = open(MERGE_C).read()
    if '--selfcheck' in sys.argv:
        bad = 0
        ls = open(os.path.join(SRCDIR, 'd_linkscore.c')).read()
        broken = ls.replace('    v[0] = cv_rocket_trails.EV;\n', '')
        red = broken != ls and rules_check(broken) != []
        print('  %s  rocket trails left out of the rules hash' % ('red ' if red else 'GREEN'))
        if not red:
            bad += 1
        for name, old, new in BREAKS:
            n = src.count(old)
            if n != 1:
                print('  ????  %s: the text to break occurs %d times -- update BREAKS' % (name, n))
                bad += 1
                continue
            code, out = run(src.replace(old, new), iters=1500, quiet=True)
            red = code == 1
            print('  %s  %s' % ('red ' if red else 'GREEN', name))
            if not red:
                bad += 1
                print('        ' + out.strip().replace('\n', '\n        '))
        print('selfcheck: %d of %d breaks caught' % (len(BREAKS) + 1 - bad, len(BREAKS) + 1))
        sys.exit(1 if bad else 0)
    problems = rules_check()
    for pr in problems:
        print('FAIL: ' + pr)
    code, out = run(src)
    print(out.strip())
    if not problems:
        print('every demo header setting is pinned or in the rules hash')
    sys.exit(code or (1 if problems else 0))


if __name__ == '__main__':
    main()
