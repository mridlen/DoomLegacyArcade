#!/usr/bin/env python3
"""Test the real video mode paging navigation, without a screen.

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


try:
    if '--selfcheck' in sys.argv:
        print('mutations that must be caught:')
        worst = 0
        for i, mut in enumerate(MUTATIONS):
            name, subs = mut[0], mut[1:]
            text = harness
            for a, b in zip(subs[0::2], subs[1::2]):
                assert text.count(a), 'mutation %r no longer applies' % name
                text = text.replace(a, b)
            rc, out = build_and_run(text, 'mut%d' % i, quiet=True)
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
        rc, out = build_and_run(harness, 'ctl')
        print(''.join('  ' + l + chr(10) for l in out.splitlines()))
        sys.exit(worst or rc)

    rc, out = build_and_run(harness, 'navtest')
    if rc is None:
        print(out)
        sys.exit('harness did not compile')
    print(out)
    sys.exit(rc)
finally:
    shutil.rmtree(TMP, ignore_errors=True)
