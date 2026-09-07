#!/usr/bin/env python3
"""Test the real video mode menu logic -- paging and sorting -- without a screen.

The paging arithmetic is the part that can be wrong in ways a build will not
catch: a cursor that lands outside the drawn page, a mode that no sequence of
key presses can reach, a divide by zero on a short page.  None of that is
visible from a headless run, because nothing drives the menu there.

So extract vidm_page_modes, vidm_page_colsize, vidm_set_page and
M_VideoMode_key_handler VERBATIM from m_menu.c -- by brace matching, not by
copying them into this file -- stub the handful of things they touch, and drive
them exhaustively.  If m_menu.c changes, this tests the changed text.

For every list size, from every starting position, walk every reachable state
under the four arrow keys and PgUp/PgDn and check:

  * the cursor is always inside the page that is drawn
  * the row it lands on is inside the page's columns and inside the screen
  * every mode in the list is reachable using ONLY the four arrows -- the
    cabinet panel has no PgUp

A second harness does the same for vidm_sort_by_size: that the result is
DESCENDING by width then height, that it is a permutation of the input with
nothing lost or duplicated, that it is stable, and that the caller's
current-mode pointer comes back pointing at the same mode it went in as.

Run it from anywhere:  tools/vidmenu-navtest.py
A clean result from a check that has never been shown to fail is worth nothing,
so  tools/vidmenu-navtest.py --selfcheck  reinstates each bug the checks claim
to catch and reports whether each one goes red.
"""
import re, subprocess, sys, os, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, os.pardir, 'svn1749', 'src', 'm_menu.c')
TMP = tempfile.mkdtemp(prefix='vidmenu-navtest.') + os.sep
s = open(SRC, encoding='latin-1').read()


def extract(sig):
    """Return the whole function text starting at the line holding sig."""
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
    'static int  vidm_page_modes( int p )',
    'static int  vidm_page_colsize( int count )',
    'static void  vidm_set_page( void )',
    'void M_VideoMode_key_handler (int key)',
))

# The geometry constants, taken from m_menu.c rather than restated here.
defs = {}
for name in ('MAXCOLUMNMODES', 'MAXMODEDESCS', 'MAXVIDMODEDESCS',
             'MODES_X', 'MODES_Y', 'MODES_X_INC', 'MODES_Y_INC', 'MODES_PAGE_Y'):
    m = re.search(r'^#define\s+%s\s+(.+)$' % name, s, re.M)
    defs[name] = m.group(1).strip()
print('extracted %d lines of navigation code; geometry: %s'
      % (funcs.count('\n'), ', '.join('%s=%s' % kv for kv in defs.items())))

harness = r'''
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---- geometry, from m_menu.c ---- */
%s
#define MODETXT_Y        (MODES_Y + 60 + 24)
#define FONT_HEIGHT      7          /* measured: STCFN033 is 7 tall */
#define BASEVIDHEIGHT    200

/* ---- stubs for what the handler touches ---- */
enum { KEY_DOWNARROW = 1, KEY_UPARROW, KEY_LEFTARROW, KEY_RIGHTARROW,
       KEY_PGUP, KEY_PGDN, KEY_ESCAPE };
static int menu_sfx_updown, menu_sfx_val, menu_sfx_esc;
static void S_StartSound(int x) { (void)x; }
static int  no_handler(int key) { (void)key; return 0; }
static int  (*key_handler2)(int) = no_handler;
static int  popped = 0;
static void Pop_Menu(void) { popped = 1; }

static int vidm_current = 0;
static int vidm_nummodes = 0;
static int vidm_column_size = 0;
static int vidm_page_first = 0;
static int vidm_page_count = 0;

/* ---- verbatim from m_menu.c ---- */
%s

/* ---- checks ---- */
static int failures = 0;
static void fail(const char *what, int n, int cur)
{
    if( failures++ < 20 )
        printf("  FAIL %%s: nummodes=%%d vidm_current=%%d page_first=%%d "
               "page_count=%%d colsize=%%d\n",
               what, n, cur, vidm_page_first, vidm_page_count, vidm_column_size);
}

static void check_state(int n)
{
    int loc, col, row, y, x;

    if( vidm_current < 0 || vidm_current >= n )       fail("cursor off list", n, vidm_current);
    if( vidm_column_size < 1 )                        fail("colsize < 1", n, vidm_current);
    if( vidm_page_count < 1 || vidm_page_count > MAXMODEDESCS )
                                                      fail("bad page count", n, vidm_current);
    if( vidm_page_first %% MAXMODEDESCS )              fail("page not aligned", n, vidm_current);

    loc = vidm_current - vidm_page_first;
    if( loc < 0 || loc >= vidm_page_count )           fail("cursor off page", n, vidm_current);

    col = loc / vidm_column_size;
    row = loc %% vidm_column_size;
    if( col > 2 )                                     fail("4th column", n, vidm_current);
    if( row >= MAXCOLUMNMODES )                       fail("row past MAXCOLUMNMODES", n, vidm_current);

    /* the drawn row must clear the instruction block */
    y = MODES_Y + (row * MODES_Y_INC) + FONT_HEIGHT;
    if( y > MODETXT_Y )                               fail("row overlaps instructions", n, vidm_current);
    x = MODES_X + (col * MODES_X_INC);
    if( x + 92 > 320 )                                fail("column off screen", n, vidm_current);
}

int main(void)
{
    static const int keys[6] = { KEY_DOWNARROW, KEY_UPARROW, KEY_LEFTARROW,
                                 KEY_RIGHTARROW, KEY_PGUP, KEY_PGDN };
    static const int arrows = 4;   /* first four -- all the cabinet panel has */
    int n, worst_pages = 0;

    /* An empty list must not divide by zero.  Nothing to check afterwards --
       the drawer does not index modedescs[] when there are no modes -- so just
       drive it and let the process live or die. */
    vidm_nummodes = 0;
    vidm_set_page();
    {
        int k;
        for( k = 0; k < 6; k++ )
        {
            vidm_current = 0;
            vidm_set_page();
            M_VideoMode_key_handler( keys[k] );
        }
    }
    printf("empty list survived\n");

    for( n = 1; n <= MAXVIDMODEDESCS; n++ )
    {
        char reach_arrows[MAXVIDMODEDESCS + 1];
        int  start;

        vidm_nummodes = n;
        memset(reach_arrows, 0, sizeof(reach_arrows));

        /* Reachability with the four arrows alone, from mode 0 and, so a
           one-way trip does not pass, from the last mode back again. */
        for( start = 0; start < 2; start++ )
        {
            int stack[MAXVIDMODEDESCS + 1], sp = 0, k;
            int origin = start ? (n - 1) : 0;
            memset(reach_arrows, 0, sizeof(reach_arrows));
            reach_arrows[origin] = 1;
            stack[sp++] = origin;
            while( sp )
            {
                int from = stack[--sp];
                for( k = 0; k < arrows; k++ )
                {
                    vidm_current = from;
                    vidm_set_page();
                    M_VideoMode_key_handler( keys[k] );
                    vidm_set_page();
                    check_state(n);
                    if( ! reach_arrows[vidm_current] )
                    {
                        reach_arrows[vidm_current] = 1;
                        stack[sp++] = vidm_current;
                    }
                }
            }
            for( k = 0; k < n; k++ )
            {
                if( ! reach_arrows[k] )
                {
                    if( failures++ < 20 )
                        printf("  FAIL unreachable: nummodes=%%d, from mode %%d "
                               "the arrow keys cannot reach mode %%d\n",
                               n, origin, k);
                    break;
                }
            }
        }

        /* Every state, every key, from every start -- bounds, and where the
           page keys are supposed to land. */
        for( start = 0; start < n; start++ )
        {
            int k;
            for( k = 0; k < 6; k++ )
            {
                int page0, lastcol, col0, has_next, has_prev;

                vidm_current = start;
                vidm_set_page();
                page0    = vidm_page_first / MAXMODEDESCS;
                col0     = (vidm_current - vidm_page_first) / vidm_column_size;
                lastcol  = (vidm_page_count - 1) / vidm_column_size;
                has_next = ((page0 + 1) * MAXMODEDESCS) < n;
                has_prev = page0 > 0;

                M_VideoMode_key_handler( keys[k] );
                vidm_set_page();
                check_state(n);

                {
                    int page1 = vidm_page_first / MAXMODEDESCS;
                    int fwd = (keys[k] == KEY_RIGHTARROW && col0 >= lastcol)
                              || (keys[k] == KEY_PGDN);
                    int bak = (keys[k] == KEY_LEFTARROW && col0 == 0)
                              || (keys[k] == KEY_PGUP);

                    if( fwd && has_next && page1 != page0 + 1 )
                        fail("page forward went astray", n, vidm_current);
                    if( fwd && !has_next && page1 != page0 )
                        fail("page forward off the end", n, vidm_current);
                    if( bak && has_prev && page1 != page0 - 1 )
                        fail("page back went astray", n, vidm_current);
                    if( bak && !has_prev && page1 != page0 )
                        fail("page back off the front", n, vidm_current);
                }
            }
        }

        /* Stale cursor, as when returning from the drawmode menu. */
        vidm_current = 999;
        vidm_set_page();
        check_state(n);
        vidm_current = -5;
        vidm_set_page();
        check_state(n);

        if( ((n - 1) / MAXMODEDESCS) + 1 > worst_pages )
            worst_pages = ((n - 1) / MAXMODEDESCS) + 1;
    }

    printf("list sizes 1..%%d, up to %%d pages, %%d failures\n",
           MAXVIDMODEDESCS, worst_pages, failures);
    return failures ? 1 : 0;
}
''' % ('\n'.join('#define %-18s %s' % (k, v) for k, v in defs.items()), funcs)

sort_func = extract('static modedesc_t *  vidm_sort_by_size( modedesc_t * current )')
print('extracted %d lines of sort code' % sort_func.count('\n'))

sort_harness = r'''
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXVIDMODEDESCS  128
#define MAXVIDWIDTH      1600
#define MAXVIDHEIGHT     1200

/* ---- stubs: the engine types this function touches ---- */
typedef int  boolean;
typedef unsigned char byte;
enum { MODE_NOP = 0, MODE_window, MODE_fullscreen };
typedef struct { byte modetype; byte index; } modenum_t;
typedef struct { int width, height; const char * mark; byte type; } modestat_t;
typedef struct { modenum_t modenum; char * desc; } modedesc_t;

static modedesc_t  modedescs[MAXVIDMODEDESCS];
static int         vidm_nummodes;

/* Sizes the stubbed VID_GetMode_Stat reports, by modenum.index - 1.
   A width of 0 stands for a mode it cannot describe (mark == NULL). */
static int stub_w[MAXVIDMODEDESCS], stub_h[MAXVIDMODEDESCS];

static modestat_t VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t ms;
    int i = modenum.index - 1;
    ms.type = modenum.modetype;
    if( i < 0 || i >= MAXVIDMODEDESCS || stub_w[i] == 0 )
    {
        ms.width = ms.height = 0;
        ms.mark = NULL;
        return ms;
    }
    ms.width = stub_w[i];
    ms.height = stub_h[i];
    ms.mark = "";
    return ms;
}

/* ---- verbatim from m_menu.c ---- */
%s

/* ---- checks ---- */
static int failures = 0;
static int in_w[MAXVIDMODEDESCS], in_h[MAXVIDMODEDESCS];  /* by index-1 */

static void fail(const char *what, const char *set, int n)
{
    if( failures++ < 20 )
        printf("  FAIL %%s [%%s, n=%%d]\n", what, set, n);
}

/* Load a list.  sizes is n pairs; a width of 0 means "no size". */
static void setup(const int *sizes, int n)
{
    int i;
    vidm_nummodes = n;
    for( i = 0; i < n; i++ )
    {
        modedescs[i].modenum.modetype = MODE_fullscreen;
        modedescs[i].modenum.index = i + 1;
        modedescs[i].desc = NULL;
        stub_w[i] = in_w[i] = sizes[i*2];
        stub_h[i] = in_h[i] = sizes[i*2 + 1];
    }
}

static void check(const char *set, int n, int pick)
{
    int i, seen[MAXVIDMODEDESCS + 1];
    modedesc_t * before = (pick >= 0 && pick < n) ? &modedescs[pick] : NULL;
    byte want_index = before ? before->modenum.index : 0;
    modedesc_t * after;

    after = vidm_sort_by_size( before );

    /* a permutation: every original index exactly once */
    memset(seen, 0, sizeof(seen));
    for( i = 0; i < n; i++ )
    {
        int ix = modedescs[i].modenum.index;
        if( ix < 1 || ix > n ) { fail("index out of range after sort", set, n); return; }
        if( seen[ix]++ )       { fail("entry duplicated by the sort", set, n); return; }
    }
    for( i = 1; i <= n; i++ )
        if( !seen[i] ) { fail("entry lost by the sort", set, n); return; }

    /* descending by width then height, sizeless entries (width 0) last --
       which falls out of the descending order rather than needing a special
       case, and is exactly what the sort must not get backwards */
    for( i = 1; i < n; i++ )
    {
        int pw = in_w[modedescs[i-1].modenum.index - 1];
        int ph = in_h[modedescs[i-1].modenum.index - 1];
        int cw = in_w[modedescs[i].modenum.index - 1];
        int ch = in_h[modedescs[i].modenum.index - 1];
        if( (pw < cw) || ((pw == cw) && (ph < ch)) )
        {
            fail("not descending by width then height", set, n);
            return;
        }
        /* stable: equal sizes keep their original order */
        if( (pw == cw) && (ph == ch)
            && (modedescs[i-1].modenum.index > modedescs[i].modenum.index) )
        {
            fail("equal sizes reordered (unstable)", set, n);
            return;
        }
    }

    /* the caller's pointer comes back pointing at the same mode */
    if( before )
    {
        if( after == NULL )
            fail("current mode lost by the sort", set, n);
        else if( after->modenum.index != want_index )
            fail("current mode points at a different mode", set, n);
        else if( after < modedescs || after >= modedescs + n )
            fail("current mode points outside the list", set, n);
    }
    else if( after != NULL )
        fail("NULL current came back non-NULL", set, n);
}

static void run(const char *set, const int *sizes, int n)
{
    int pick;
    for( pick = -1; pick < n; pick++ )
    {
        setup(sizes, n);
        check(set, n, pick);
    }
}

int main(void)
{
    /* This laptop after the i_video.c dedup: SDL's order, then the appended
       scaled modes.  Exactly the shape the reported bug had. */
    static const int laptop[] = { 1366,768, 1280,720, 1024,768, 800,600, 640,480,
                                  320,200, 400,300, 512,384 };
    /* Ascending input too, so the sort is not handed something already nearly
       right in the direction it is meant to produce. */
    static const int ascending[] = { 320,200, 400,300, 512,384, 640,480, 800,600,
                                     1024,768, 1280,720, 1366,768 };
    /* windowedModes[] from i_video.c, which runs the other way. */
    static const int windowed[] = { 1600,1200, 1280,1024, 1024,768, 800,600,
                                    640,480, 512,384, 400,300, 320,200 };
    /* A display with a lot of modes, unsorted, with duplicates in size and a
       couple the engine cannot describe. */
    static const int messy[] = { 1024,768, 640,480, 1024,768, 0,0, 1280,1024,
                                 640,350, 640,400, 0,0, 800,600, 320,200,
                                 1280,720, 640,480, 512,384, 400,300, 720,480 };
    int i, k, n;
    static int rnd[MAXVIDMODEDESCS * 2];

    run("laptop", laptop, sizeof(laptop) / (2 * sizeof(int)));
    run("ascending", ascending, sizeof(ascending) / (2 * sizeof(int)));
    run("windowed", windowed, sizeof(windowed) / (2 * sizeof(int)));
    run("messy", messy, sizeof(messy) / (2 * sizeof(int)));

    /* Empty and single-entry lists. */
    run("empty", laptop, 0);
    run("one", laptop, 1);

    /* Random lists, fixed seed.  Sizes drawn from a small set so equal sizes
       are common -- that is what the stability check needs. */
    srand(12345);
    for( k = 0; k < 400; k++ )
    {
        n = 1 + (rand() %% MAXVIDMODEDESCS);
        for( i = 0; i < n; i++ )
        {
            int r = rand() %% 12;
            if( r == 0 ) { rnd[i*2] = 0; rnd[i*2+1] = 0; }      /* sizeless */
            else {
                rnd[i*2]   = 320 * (1 + (rand() %% 5));
                rnd[i*2+1] = 200 * (1 + (rand() %% 6));
            }
        }
        /* pick is exercised by run(), but that is O(n^2) on a 128 entry list
           400 times over -- still under a second, and worth it. */
        run("random", rnd, n);
    }

    printf("sort: 4 lists, empty, single, 400 random lists, %%d failures\n",
           failures);
    return failures ? 1 : 0;
}
''' % sort_func

SORT_MUTATIONS = [
    ('sorted ascending instead of descending',
     'while( (j > 0)\n               && ((w[j-1] < hw) || ((w[j-1] == hw) && (h[j-1] < hh))) )',
     'while( (j > 0)\n               && ((w[j-1] > hw) || ((w[j-1] == hw) && (h[j-1] > hh))) )'),
    ('height tiebreak dropped (same width unordered)',
     '&& ((w[j-1] < hw) || ((w[j-1] == hw) && (h[j-1] < hh))) )',
     '&& (w[j-1] < hw) )'),
    # Breaks stability ONLY: identical sizes get swapped, while the sort order
    # stays perfectly valid.  An earlier version of this mutation loosened the
    # width comparison too, which broke the ordering as well -- so it was the
    # ordering check that caught it and the stability check was never
    # exercised.  A mutation that trips a different check than the one it was
    # aimed at has tested nothing.
    ('unstable: identical sizes reordered, ordering still valid',
     '&& ((w[j-1] < hw) || ((w[j-1] == hw) && (h[j-1] < hh))) )',
     '&& ((w[j-1] < hw) || ((w[j-1] == hw) && (h[j-1] <= hh))) )'),
    ('current mode not found again after the sort',
     'return & modedescs[i];', 'return NULL;'),
    # The sentinel key for an undescribable mode has to flip with the sort
    # direction.  Ascending it was MAXVIDWIDTH+1, to push those entries past
    # every real size; descending, that same value drags them to the TOP.
    ('sizeless modes sort to the front instead of the end',
     'w[i] = ( ms.mark )? ms.width : 0;',
     'w[i] = ( ms.mark )? ms.width : MAXVIDWIDTH + 1;'),
]

# Each entry is a bug the checks above are supposed to catch, expressed as a
# substitution on the extracted source.  --selfcheck applies them one at a time
# and reports whether the harness notices.  Two of these were NOT caught by the
# first version of this file: reachability was only tested forwards from mode
# 0, so breaking the Left-edge page-back left everything still reachable by
# going right, and an unclamped page step self-healed because vidm_set_page
# resets an out-of-range cursor.  Both checks were strengthened until they went
# red, which is the only reason to believe the green result.
MUTATIONS = [
    ('11 rows per column (overlaps the instructions)',
     '#define MAXCOLUMNMODES     10', '#define MAXCOLUMNMODES     11'),
    ('left edge stops instead of paging back',
     '        goto prev_page;', '        return;'),
    ('right edge stops instead of paging forward',
     '        goto next_page;', '        return;'),
    ('page target not clamped to a short page',
     'if( t >= pc )  t = pc - 1;', ';'),
    ('both divide-by-zero guards removed (SIGFPE on an empty list)',
     'return (cs > 0) ? cs : 1;', 'return cs;',
     'if( vidm_column_size < 1 )  vidm_column_size = 1;', ';'),
]


def build_and_run(text, name, quiet=False):
    """Compile text, run it, return (returncode, output)."""
    csrc = TMP + name + '.c'
    exe = TMP + name
    open(csrc, 'w').write(text)
    r = subprocess.run(['gcc', '-w' if quiet else '-Wall', '-O1', '-o', exe, csrc],
                       capture_output=True, text=True)
    if r.returncode:
        return None, r.stdout + r.stderr
    if r.stderr.strip() and not quiet:
        print('compiler warnings:\n' + r.stderr)
    r = subprocess.run([exe], capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


SUITES = [('navigation', harness, MUTATIONS),
          ('sort', sort_harness, SORT_MUTATIONS)]

try:
    if '--selfcheck' in sys.argv:
        worst = 0
        for suite, text0, muts in SUITES:
            print('%s -- mutations that must be caught:' % suite)
            for i, mut in enumerate(muts):
                name, subs = mut[0], mut[1:]
                text = text0
                for a, b in zip(subs[0::2], subs[1::2]):
                    assert text.count(a), \
                        '%s: mutation %r no longer applies' % (suite, name)
                    text = text.replace(a, b)
                rc, out = build_and_run(text, '%s-mut%d' % (suite, i), quiet=True)
                if rc is None:
                    print('  %-58s DID NOT COMPILE' % name)
                    worst = 1
                elif rc:
                    why = ([l.strip() for l in out.splitlines() if 'FAIL' in l]
                           or ['exit %d' % rc])[0]
                    print('  %-58s caught -> %s' % (name, why))
                else:
                    print('  %-58s *** NOT CAUGHT ***' % name)
                    worst = 1
            print()
        print('control (unmutated):')
        for suite, text0, _ in SUITES:
            rc, out = build_and_run(text0, 'ctl-' + suite)
            print(''.join('  ' + l + chr(10) for l in out.splitlines()))
            worst = worst or rc
        sys.exit(worst)

    bad = 0
    for suite, text0, _ in SUITES:
        rc, out = build_and_run(text0, suite)
        if rc is None:
            print(out)
            sys.exit('%s harness did not compile' % suite)
        print(out, end='' if out.endswith(chr(10)) else chr(10))
        bad = bad or rc
    sys.exit(bad)
finally:
    shutil.rmtree(TMP, ignore_errors=True)
