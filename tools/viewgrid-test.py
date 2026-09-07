#!/usr/bin/env python3
"""Test the view grid -- how many views, how the screen is carved, and which
column each panel gets -- without a screen.

A headless run always joins panels 1..N in order, so the cases that actually
break are the ones it can never reach: three players at panels 1+3+4, two at
1+3, a panel sitting out in the middle.  Those only happen through the join
screen, and nothing drives the join screen headlessly.  Get the mapping wrong
and a player's view is not where they are standing -- which is the one thing
the whole panel/cell split exists to prevent (multiplayer-views.md).

So extract D_Three_Column_Views, D_Panel_Rank, D_View_Cell, D_NumViews and
D_View_Grid VERBATIM from d_clisrv.c by brace matching, stub what they touch,
and drive every combination of joined panels.

Run it from anywhere:  tools/viewgrid-test.py
  --selfcheck   reinstate each bug the checks claim to catch and report
                whether each one goes red.
"""
import re, subprocess, sys, os, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, os.pardir, 'svn1749', 'src', 'd_clisrv.c')
s = open(SRC, encoding='latin-1').read()


def extract(sig):
    i = s.index(sig)
    j = s.index('{', i)
    depth, k = 0, j
    while True:
        if s[k] == '{':
            depth += 1
        elif s[k] == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    return s[i:k + 1] + '\n'


funcs = ''.join(extract(sig) for sig in (
    'static boolean  D_Three_Column_Views( void )',
    'static byte  D_Panel_Rank( byte pind )',
    'byte  D_View_Cell( byte pind )',
    'byte  D_NumViews( void )',
    'void  D_View_Grid( byte * out_cols, byte * out_rows )',
    'void  D_Grid_Cell_Pos( byte cell, byte cols, byte rows,',
))
print('extracted %d lines of view grid code' % funcs.count('\n'))

HARNESS = r'''
#include <stdio.h>
#include <string.h>

typedef unsigned char byte;
typedef int boolean;
#define true 1
#define false 0
#define MAXSPLITSCREENPLAYERS 4

/* ---- stubs ---- */
typedef struct { byte EV; } consvar_t;
static consvar_t cv_split4, cv_splitvertical, cv_panelorder;
static int demoplayback = 0;

/* The joined panels, lowest pind first, as the join screen leaves them. */
static byte stub_panel[MAXSPLITSCREENPLAYERS] = {0,1,2,3};
static byte stub_numlocal = 1;
static byte localplayer_cell[MAXSPLITSCREENPLAYERS] = {0,1,2,3};

static byte D_NumLocalPlayers(void) { return stub_numlocal; }
byte  D_Panel_Of( byte pind )
{ return (pind < MAXSPLITSCREENPLAYERS) ? stub_panel[pind] : pind; }

/* ---- verbatim from d_clisrv.c ---- */
%(funcs)s

/* ---- checks ---- */
static int failures = 0;
#define FAIL(...) do { if(failures++ < 30) { printf("  FAIL "); printf(__VA_ARGS__); printf("\n"); } } while(0)

static const char * panels_str( int n )
{
    static char b[16];
    int i;
    for( i = 0; i < n; i++ )  b[i] = '1' + stub_panel[i];
    b[n] = 0;
    return b;
}

/* Set the joined panels from a list of 0-based panel numbers. */
static void join( const byte * p, int n )
{
    int i;
    stub_numlocal = n;
    for( i = 0; i < MAXSPLITSCREENPLAYERS; i++ )  stub_panel[i] = i;
    for( i = 0; i < n; i++ )  stub_panel[i] = p[i];
    /* localplayer_cell is what the join screen records: the cell of the
       four-cell grid belonging to that panel. */
    for( i = 0; i < MAXSPLITSCREENPLAYERS; i++ )  localplayer_cell[i] = stub_panel[i];
}

int main(void)
{
    byte cols, rows;
    int  i, j;

    /* ---- 1. Three players, Columns: one column each, in PANEL order.
       Every 3 of the 4 panels, in every join order.  The join order matters
       and is the whole point: players press fire whenever they like, pind is
       handed out in join order, and it is the PANEL that has to decide the
       column -- otherwise the player at panel 4 who pressed first gets the
       left of the screen while standing on the right. ---- */
    cv_split4.EV = 1;
    {
        static const byte sets[4][3] = { {0,1,2},{0,1,3},{0,2,3},{1,2,3} };
        static const byte perm[6][3] = { {0,1,2},{0,2,1},{1,0,2},
                                         {1,2,0},{2,0,1},{2,1,0} };
        int p;
        for( i = 0; i < 4; i++ )
        for( p = 0; p < 6; p++ )
        {
            byte order[3], seen[3] = {0,0,0};
            for( j = 0; j < 3; j++ )  order[j] = sets[i][ perm[p][j] ];
            join( order, 3 );

            if( D_NumViews() != 3 )
                FAIL("panels %%s: %%d views, expected 3", panels_str(3), D_NumViews());
            D_View_Grid( &cols, &rows );
            if( cols != 3 || rows != 1 )
                FAIL("panels %%s: grid %%dx%%d, expected 3x1", panels_str(3), cols, rows);

            for( j = 0; j < 3; j++ )
            {
                /* the column this player should get: how many of the joined
                   panels are to their left */
                byte want = 0, k;
                byte c = D_View_Cell(j);
                for( k = 0; k < 3; k++ )
                    if( order[k] < order[j] )  want++;

                if( c > 2 ) { FAIL("panels %%s (join order): panel %%d got column %%d",
                                   panels_str(3), order[j]+1, c); continue; }
                if( seen[c] )
                    FAIL("panels %%s: two players in column %%d", panels_str(3), c);
                seen[c] = 1;
                if( c != want )
                    FAIL("panels %%s joined in that order: panel %%d got column %%d, "
                         "expected %%d", panels_str(3), order[j]+1, c, want);
            }
        }
    }

    /* ---- 2. The rule as stated: who lands in the MIDDLE column.
       1+2+3 and 1+2+4 put panel 2 there; 1+3+4 and 2+3+4 put panel 3 there --
       whatever order they joined in. ---- */
    {
        static const byte sets[4][3] = { {0,1,2},{0,1,3},{0,2,3},{1,2,3} };
        static const byte middle[4]  = {    1,      1,      2,      2    };
        static const byte perm[6][3] = { {0,1,2},{0,2,1},{1,0,2},
                                         {1,2,0},{2,0,1},{2,1,0} };
        int p;
        for( i = 0; i < 4; i++ )
        for( p = 0; p < 6; p++ )
        {
            byte order[3];
            int  found = 0;
            for( j = 0; j < 3; j++ )  order[j] = sets[i][ perm[p][j] ];
            join( order, 3 );
            for( j = 0; j < 3; j++ )
            {
                if( D_View_Cell(j) != 1 )  continue;
                found = 1;
                if( order[j] != middle[i] )
                    FAIL("panels %%s: panel %%d took the middle, expected panel %%d",
                         panels_str(3), order[j]+1, middle[i]+1);
            }
            if( ! found )
                FAIL("panels %%s: nobody took the middle column", panels_str(3));
        }
    }

    /* ---- 3. Grid setting: three players are unchanged, a 2x2 with a gap. ---- */
    cv_split4.EV = 0;
    {
        static const byte sets[4][3] = { {0,1,2},{0,1,3},{0,2,3},{1,2,3} };
        for( i = 0; i < 4; i++ )
        {
            join( sets[i], 3 );
            if( D_NumViews() != 4 )
                FAIL("Grid, panels %%s: %%d views, expected 4", panels_str(3), D_NumViews());
            D_View_Grid( &cols, &rows );
            if( cols != 2 || rows != 2 )
                FAIL("Grid, panels %%s: grid %%dx%%d, expected 2x2", panels_str(3), cols, rows);
        }
    }

    /* ---- 4. Four players follow the setting; two are untouched by it. ---- */
    {
        static const byte all4[4] = {0,1,2,3};
        cv_split4.EV = 0;  join( all4, 4 );
        D_View_Grid( &cols, &rows );
        if( D_NumViews() != 4 || cols != 2 || rows != 2 )
            FAIL("4 players Grid: %%d views, %%dx%%d", D_NumViews(), cols, rows);
        cv_split4.EV = 1;  join( all4, 4 );
        D_View_Grid( &cols, &rows );
        if( D_NumViews() != 4 || cols != 4 || rows != 1 )
            FAIL("4 players Columns: %%d views, %%dx%%d", D_NumViews(), cols, rows);

        /* Two players: cv_split4 must not touch them, in either state, and
           panels 1+3 still need the 2x2 (they occupy cells 0 and 2). */
        static const byte two_adj[2] = {0,1};
        static const byte two_gap[2] = {0,2};
        for( i = 0; i < 2; i++ )
        {
            cv_split4.EV = i;
            cv_splitvertical.EV = 0;
            join( two_adj, 2 );
            D_View_Grid( &cols, &rows );
            if( D_NumViews() != 2 || cols != 1 || rows != 2 )
                FAIL("2 players stacked (split4=%%d): %%d views, %%dx%%d",
                     i, D_NumViews(), cols, rows);
            cv_splitvertical.EV = 1;
            join( two_adj, 2 );
            D_View_Grid( &cols, &rows );
            if( D_NumViews() != 2 || cols != 2 || rows != 1 )
                FAIL("2 players side by side (split4=%%d): %%d views, %%dx%%d",
                     i, D_NumViews(), cols, rows);
            join( two_gap, 2 );
            if( D_NumViews() != 4 )
                FAIL("2 players at panels 1+3 (split4=%%d): %%d views, expected 4",
                     i, D_NumViews());
        }
        cv_splitvertical.EV = 0;
    }

    /* ---- 5. A demo is one view whatever the cabinet says. ---- */
    {
        static const byte sets[3] = {0,1,2};
        cv_split4.EV = 1;  join( sets, 3 );
        demoplayback = 1;
        if( D_NumViews() != 1 )
            FAIL("demo playback: %%d views, expected 1", D_NumViews());
        demoplayback = 0;
    }

    /* ---- 6. Every cell of a 3 column grid maps to its own column. ---- */
    {
        byte col, row, seen[3] = {0,0,0};
        for( i = 0; i < 3; i++ )
        {
            D_Grid_Cell_Pos( i, 3, 1, &col, &row );
            if( row != 0 )  FAIL("3 column grid: cell %%d has row %%d", i, row);
            if( col > 2 || seen[col] )  FAIL("3 column grid: cell %%d -> column %%d", i, col);
            else seen[col] = 1;
        }
    }

    if( failures )  printf("%%d failures\n", failures);
    else  printf("view grid: 3 and 4 player layouts, every panel combination, "
                 "2 player and demo unchanged -- 0 failures\n");
    return failures ? 1 : 0;
}
'''


def build_and_run(funcs_text=None, quiet=False):
    tmp = tempfile.mkdtemp(prefix='viewgrid-test.') + os.sep
    try:
        csrc, exe = tmp + 't.c', tmp + 't'
        open(csrc, 'w').write(HARNESS % dict(
            funcs=funcs_text if funcs_text is not None else funcs))
        r = subprocess.run(['gcc', '-w' if quiet else '-Wall', '-O1', '-o', exe, csrc],
                           capture_output=True, text=True)
        if r.returncode:
            return None, r.stderr
        r = subprocess.run([exe], capture_output=True, text=True)
        return r.returncode, r.stdout
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


rc, out = build_and_run()
if rc is None:
    print(out)
    sys.exit(2)
print(out.rstrip())

if '--selfcheck' in sys.argv:
    print('\nselfcheck -- each reinstates a real bug; every one must go RED')
    ok = True
    cases = [
        ('rank by join order instead of panel number (panels 1+3+4 then put '
         'the wrong player in the middle)',
         funcs.replace('if( D_Panel_Of(i) < panel )  rank++;',
                       'if( i < pind )  rank++;')),
        ('three players fall through to the 2x2 even under Columns',
         funcs.replace('    if( D_Three_Column_Views() )  return 3;\n', '')),
        ('D_View_Grid left at two columns for three views',
         funcs.replace('        cols = 3;\n        rows = 1;',
                       '        cols = 2;\n        rows = 1;')),
        ('cv_split4 also swallows the two player layouts',
         funcs.replace('return ( ! demoplayback ) && ( D_NumLocalPlayers() == 3 ) && cv_split4.EV;',
                       'return ( ! demoplayback ) && ( D_NumLocalPlayers() >= 2 ) && cv_split4.EV;')),
    ]
    for name, ft in cases:
        if ft == funcs:
            print('  %-74s COULD NOT PATCH' % name)
            ok = False
            continue
        rc2, out2 = build_and_run(funcs_text=ft, quiet=True)
        red = (rc2 != 0)
        print('  %-74s %s' % (name, 'RED (good)' if red else 'still green'))
        ok = ok and red
    sys.exit(0 if ok else 1)

sys.exit(rc)
