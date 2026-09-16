#!/usr/bin/env python3
"""Do the intermission tables fit all 32 players on the screen?

DoomLegacy allows MAXPLAYERS=32 and neither intermission table was built for
that many.  The netgame table stepped 16 base units per player and ran out of
screen after 8; the deathmatch rankings stepped 12 and broke out of their loop
after 12 -- and because those are sorted highest first, what went missing was
the bottom of the scoreboard.  Neither said anything about the players it
dropped, which is exactly why nobody noticed for years.

The compact fallback that now takes over past those counts is pure arithmetic
laid out to the base unit, and none of it can be seen from a headless run: a
name three units too wide for its column, or a row eight units below the
bottom of the screen, draws without complaint.  Only someone in front of the
cabinet would see it, in a 32 player game nobody is going to set up on demand.

So extract the layout functions from wi_stuff.c VERBATIM, by brace matching,
and drive them over every player count with the worst names hu_font can
produce.  A copied test drifts away from the code and then passes forever; an
extracted one tests the text that ships.

The glyph widths come from a real IWAD when one can be found (DOOMWADDIR, or
~/games/doom), and are checked against the table below when it is.  hu_font is
proportional -- 'M' and 'W' are 9 units, 'I' is 4 -- which is the whole reason
the columns are measured rather than counted in characters.

Run it from anywhere:  tools/interfit-test.py
  --selfcheck   reinstate each bug the checks claim to catch, and report
                whether each one actually goes red.  A clean result from a
                check that has never been shown to fail is worth nothing.
"""
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SRCDIR = os.path.join(HERE, os.pardir, 'svn1749', 'src')
WI = os.path.join(SRCDIR, 'wi_stuff.c')

wi_s = open(WI, encoding='latin-1').read()


def extract(text, sig):
    """Return the whole function text starting at the line holding sig."""
    i = text.index(sig)
    j = text.index('{', i)
    depth, k = 0, j
    while True:
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    return text[i:k + 1] + '\n'


def struct_def(text, name):
    """Return the typedef struct {...} <name>; text.

    Found from its closing line backwards: a forward search for
    'typedef struct {' lands on the first one in the file and then swallows
    everything down to this name.
    """
    end = re.search(r'\}\s*%s\s*;' % name, text)
    start = text.rindex('typedef struct', 0, end.start())
    return text[start:end.end()]


def define(text, name):
    m = re.search(r'^#define\s+%s\s+(-?\w+)' % name, text, re.M)
    if not m:
        raise SystemExit('cannot find #define %s in wi_stuff.c' % name)
    return m.group(1)


DEFINES = ['WI_C_PITCH', 'WI_C_ROW_H', 'WI_C_BOTTOM', 'WI_C_MARGIN',
           'WI_C_COLGAP', 'WI_C_MARK_W', 'WI_C_PCT_MIN', 'WI_C_PCT_MAX',
           'WI_C_FRAG_MIN', 'WI_C_FRAG_MAX',
           'WI_C_NAME_MIN', 'WI_C_NUM_PAD',
           'WI_NG_COMPACT_Y', 'WI_RANK_X0', 'WI_RANK_DX', 'WI_RANK_SUB_W',
           'RANKINGY', 'TEAMRANKINGY']

FUNCS = [
    'static int WI_Compact_Rows( int ytop )',
    'static void WI_Fit_Name( char * dest, int destsize, const char * name, int max_w )',
    'static int WI_Rank_Rows( int scorelines, int max_rows )',
    'static void WI_Rank_Col_Fit( int sub_w, int num_w, wi_rankcol_t * out )',
    'static void WI_Rank_Fit( int num_pl, int ytop, wi_rankfit_t * out )',
    'static void WI_Netgame_Fit( int num_pl, int pct_w, int frag_w, int name_want,',
    # The in-level rankings' wrapper, calling the stub below.
    'void WI_Draw_Ranking(const char * title, int x, int y, fragsort_t * fragtable,',
]


# ---------------------------------------------------------------- hu_font
#
# STCFN033..STCFN095 are '!'..'_'.  V_StringWidth folds to upper case and
# charges 4 units for anything it has no glyph for, including space.

FONT_FALLBACK = {
    '!': 4, '"': 7, '#': 7, '$': 7, '%': 9, '&': 8, "'": 4, '(': 7, ')': 7,
    '*': 7, '+': 5, ',': 4, '-': 6, '.': 4, '/': 7, '0': 8, '1': 5, '2': 8,
    '3': 8, '4': 7, '5': 7, '6': 8, '7': 8, '8': 8, '9': 8, ':': 4, ';': 4,
    '<': 5, '=': 5, '>': 5, '?': 8, '@': 9, 'A': 8, 'B': 8, 'C': 8, 'D': 8,
    'E': 8, 'F': 8, 'G': 8, 'H': 8, 'I': 4, 'J': 8, 'K': 8, 'L': 8, 'M': 9,
    'N': 8, 'O': 8, 'P': 8, 'Q': 8, 'R': 8, 'S': 7, 'T': 8, 'U': 8, 'V': 7,
    'W': 9, 'X': 9, 'Y': 8, 'Z': 7, '[': 5, '\\': 7, ']': 5, '^': 7, '_': 8,
}


def wad_font():
    """hu_font glyph widths out of a real IWAD, or None."""
    cands = []
    for d in filter(None, [os.environ.get('DOOMWADDIR'),
                           os.path.expanduser('~/games/doom'),
                           os.path.expanduser('~/.doomlegacy')]):
        for nm in ('DOOM2.WAD', 'doom2.wad', 'DOOM.WAD', 'doom.wad'):
            p = os.path.join(d, nm)
            if os.path.isfile(p):
                cands.append(p)
    for path in cands:
        try:
            d = open(path, 'rb').read()
            _, n, off = struct.unpack('<4sii', d[:12])
            w = {}
            for i in range(n):
                lo, _, nm = struct.unpack('<ii8s', d[off + i * 16:off + i * 16 + 16])
                nm = nm.split(b'\0')[0].decode('ascii', 'replace')
                if nm.startswith('STCFN'):
                    c = int(nm[5:])
                    if 33 <= c <= 95:
                        w[chr(c)] = struct.unpack('<h', d[lo:lo + 2])[0]
            if len(w) > 50:
                return path, w
        except Exception:
            continue
    return None, None


wadpath, FONT = wad_font()
if FONT:
    diff = {c: (FONT_FALLBACK.get(c), FONT[c])
            for c in FONT if FONT_FALLBACK.get(c) != FONT[c]}
    print('hu_font from %s (%d glyphs)%s'
          % (wadpath, len(FONT),
             '' if not diff else '  DIFFERS from the built-in table: %r' % diff))
    if diff:
        print('  update FONT_FALLBACK in this file')
else:
    FONT = FONT_FALLBACK
    print('hu_font: no IWAD found, using the built-in table (%d glyphs)'
          % len(FONT))

font_c = ',\n'.join('    [%d] = %d' % (ord(c), w) for c, w in sorted(FONT.items()))


# ---------------------------------------------------------------- harness

HARNESS = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef unsigned char byte;
typedef int boolean;
#define false 0
#define true  1

#define BASEVIDWIDTH   320
#define BASEVIDHEIGHT  200
#define MAXPLAYERS      32
#define MAXPLAYERNAME   21

%(defines)s

/* hu_font glyph widths, from the IWAD */
static const int glyph_w[256] = {
%(font)s
};

/* V_StringWidth (v_video.c): fold to upper case, 4 units for anything with
   no glyph -- space included. */
static int V_StringWidth( const char * s )
{
    int  w = 0, i;
    for( i = 0; s[i]; i++ )
    {
        unsigned char uc = (unsigned char) s[i];
        int c = (uc >= 'a' && uc <= 'z') ? uc - 32 : uc;
        w += (c >= 33 && c <= 95 && glyph_w[c]) ? glyph_w[c] : 4;
    }
    return w;
}

%(structs)s

/* WI_Draw_Ranking_Cols stub: records the layout WI_Draw_Ranking asks for. */
typedef struct { int count, num, color; const char * name; } fragsort_t;
static int  cols_pitch, cols_max_rows, cols_col_dx;
static void WI_Draw_Ranking_Cols(const char * title, int x, int y, fragsort_t * fragtable,
                    int scorelines, boolean large, int white, int colwidth,
                    int y_limit, int pitch, int max_rows, int col_dx, int sub_w)
{
    cols_pitch = pitch;  cols_max_rows = max_rows;  cols_col_dx = col_dx;
}
static int WI_Rank_Rows( int scorelines, int max_rows );

/* ---- verbatim from wi_stuff.c ---- */
%(funcs)s

/* ---- checks ---- */
static int failures = 0;
static void fail(const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if( failures++ < 30 ) { printf("  FAIL "); vprintf(fmt, ap); printf("\n"); }
    va_end(ap);
}

/* The worst names hu_font can make: every glyph 9 units wide, at the longest
   a name can be.  Whatever fits these fits anything. */
static char wide_name[MAXPLAYERNAME];

/* Bottom of a compact row's colour bar, drawn from cy-1, WI_C_ROW_H tall. */
#define ROW_BOTTOM(cy)  ((cy) - 1 + WI_C_ROW_H - 1)

/* ============ the compact netgame table ============ */
static void check_netgame(void)
{
    int  n, pi, fi, wi;

    /* Field widths the drawer can hand over.  A percentage field is the
       widest of the "K%%" heading and the values on the page, plus two; the
       frags field likewise, or 0 when nothing has frags.  The range here is
       wider than that on purpose, so the clamps are exercised too. */
    static const int pcts[]  = { 12, 17, 19, 21, 23, 26, 30 };
    static const int frags[] = { 0, 8, 10, 14, 20, 26, 32, 40 };
    /* What the longest name present asks for: 0 (take what is going), one
       narrow letter, a typical name, and more than any column can give. */
    static const int wants[] = { 0, 4, 48, 61, 100, 220 };

    for( fi = 0; fi < (int)(sizeof(frags)/sizeof(frags[0])); fi++ )
    for( pi = 0; pi < (int)(sizeof(pcts)/sizeof(pcts[0])); pi++ )
    for( wi = 0; wi < (int)(sizeof(wants)/sizeof(wants[0])); wi++ )
    for( n = 1; n <= MAXPLAYERS; n++ )
    {
        wi_ngfit_t  f;
        int  ytop = WI_NG_COMPACT_Y;
        int  c, last_bottom, hdr_y, pad;
        int  df = (frags[fi] > 0);
        char buf[MAXPLAYERNAME + 1];

        WI_Netgame_Fit( n, pcts[pi], frags[fi], wants[wi], ytop, &f );
        pad = f.col_x[0] - WI_C_MARGIN;

        /* 0. The block really is centred in its column -- the whole point of
              capping the name field, and a check that has to be able to tell
              "centred" from "pinned to the left", which is what a lone
              pad >= 0 could not. */
        {
            int  block = WI_C_MARK_W + f.name_w + (3 * f.pct_w) + f.frag_w;
            int  want_pad = (f.col_w - block) / 2;

            if( pad != want_pad || pad < 0 )
                fail("netgame n=%%d want=%%d: block %%d in a %%d column padded "
                     "%%d, expected %%d", n, wants[wi], block, f.col_w,
                     pad, want_pad);
            if( f.col_x[1] - f.col_x[0] != f.col_w + WI_C_COLGAP )
                fail("netgame n=%%d: columns %%d apart, expected %%d",
                     n, f.col_x[1] - f.col_x[0], f.col_w + WI_C_COLGAP);
        }
        /* The name field is what the longest name present asked for, floored
           at its own heading and capped at what the column has -- never wider
           than wanted just because the column was roomy. */
        if( wants[wi] > 0 )
        {
            int  floor_w = V_StringWidth("Player");
            int  avail = f.col_w - (3 * f.pct_w) - f.frag_w - WI_C_MARK_W;
            int  want = wants[wi] + 2;

            if( floor_w < WI_C_NAME_MIN )  floor_w = WI_C_NAME_MIN;
            if( want < floor_w )  want = floor_w;
            if( want > avail )   want = avail;
            if( f.name_w != want )
                fail("netgame n=%%d want=%%d: name field %%d, expected %%d",
                     n, wants[wi], f.name_w, want);
        }

        /* 1. Everybody gets a row.  This is the whole point: the old table
              simply stopped drawing. */
        if( f.ncol * f.rows < n )
            fail("netgame n=%%d pct=%%d frag=%%d: room for %%d of them",
                 n, pcts[pi], frags[fi], f.ncol * f.rows);

        /* 2. Balanced, and no empty column. */
        if( f.ncol > 1 && (f.ncol - 1) * f.rows >= n )
            fail("netgame n=%%d: %%d columns of %%d, the last is empty",
                 n, f.ncol, f.rows);

        /* 3. The last row is on the screen, headings and all. */
        last_bottom = ROW_BOTTOM( ytop + ((f.rows - 1) * WI_C_PITCH) );
        if( last_bottom > WI_C_BOTTOM )
            fail("netgame n=%%d: last row ends at y %%d", n, last_bottom);
        hdr_y = ytop - WI_C_PITCH - 1;
        if( hdr_y < 1 )
            fail("netgame n=%%d: headings at y %%d", n, hdr_y);

        /* 4. The fields are clamped into their stated range, whatever the
              caller measured. */
        if( f.pct_w < WI_C_PCT_MIN || f.pct_w > WI_C_PCT_MAX )
            fail("netgame pct field %%d outside %%d..%%d",
                 f.pct_w, WI_C_PCT_MIN, WI_C_PCT_MAX);
        if( df && (f.frag_w < WI_C_FRAG_MIN || f.frag_w > WI_C_FRAG_MAX) )
            fail("netgame frags field %%d outside %%d..%%d",
                 f.frag_w, WI_C_FRAG_MIN, WI_C_FRAG_MAX);
        if( !df && f.frag_w != 0 )
            fail("netgame reserved %%d units for a frags column nobody wants",
                 f.frag_w);

        for( c = 0; c < f.ncol; c++ )
        {
            int  cx = f.col_x[c];
            int  right = cx + (df ? f.x_frags : f.x_secret);

            /* 5. Inside the screen, both edges. */
            if( cx < 0 )
                fail("netgame n=%%d: column %%d starts at x %%d", n, c, cx);
            if( right > BASEVIDWIDTH )
                fail("netgame n=%%d pct=%%d frag=%%d: column %%d ends at x %%d",
                     n, pcts[pi], frags[fi], c, right);

            /* 6. The columns do not touch each other, padding and all. */
            if( c > 0 && cx - pad < f.col_x[c-1] - pad + f.col_w )
                fail("netgame n=%%d: column %%d overlaps %%d", n, c, c - 1);
        }

        /* 7. The fields really are the width they were given, in order, and
              the first of them clears the name bar. */
        if( f.x_items - f.x_kills != f.pct_w
            || f.x_secret - f.x_items != f.pct_w
            || f.x_frags - f.x_secret != f.frag_w )
            fail("netgame n=%%d: fields at %%d/%%d/%%d/%%d do not step by "
                 "%%d,%%d", n, f.x_kills, f.x_items, f.x_secret, f.x_frags,
                 f.pct_w, f.frag_w);
        if( f.x_kills - f.pct_w != WI_C_MARK_W + f.name_w )
            fail("netgame n=%%d: kills field starts at %%d, the name bar "
                 "ends at %%d", n, f.x_kills - f.pct_w,
                 WI_C_MARK_W + f.name_w);

        /* 8. A name field wide enough to be worth having: its own heading
              fits in it, which is the least that can be asked. */
        if( f.name_w < V_StringWidth("Player") )
            fail("netgame n=%%d pct=%%d frag=%%d: name field %%d units, "
                 "\"Player\" needs %%d", n, pcts[pi], frags[fi], f.name_w,
                 V_StringWidth("Player"));

        /* 9. The worst-case name, truncated as the drawer truncates it,
              stays inside its colour bar. */
        WI_Fit_Name( buf, sizeof(buf), wide_name, f.name_w - 2 );
        if( V_StringWidth(buf) > f.name_w - 2 )
            fail("netgame n=%%d: name %%d units in a %%d bar",
                 n, V_StringWidth(buf), f.name_w);
        if( buf[0] == 0 )
            fail("netgame n=%%d: no room for even one letter", n);
    }
}

/* ============ the in-level rankings ============ */
/* WI_Draw_Ranking's rows must all land in different places.  Placement is
   the drawer's own: cx = x + (i/rows)*col_dx, cy = y + (i%%rows)*pitch.  It
   asked for max_rows 1 once, and every line was drawn on the first. */
static void check_classic_wrapper(void)
{
    int  n, i, j;
    for( n = 1; n <= MAXPLAYERS; n++ )
    {
        int  rows;
        WI_Draw_Ranking( "Teams", 0, 0, NULL, n, 0, 0, 32, BASEVIDHEIGHT );
        rows = WI_Rank_Rows( n, cols_max_rows );
        for( i = 0; i < n; i++ )
        for( j = i + 1; j < n; j++ )
        {
            if( (i / rows) * cols_col_dx == (j / rows) * cols_col_dx
                && (i %% rows) * cols_pitch == (j %% rows) * cols_pitch )
            {
                fail("in-level rankings n=%%d: rows %%d and %%d drawn in the same place",
                     n, i, j);
                return;
            }
        }
    }
}

/* ============ the ranking tables ============ */
static void check_rankings(void)
{
    int  n, t, ti, ni;
    const int tops[2] = { RANKINGY, TEAMRANKINGY };
    /* The widest count a table might hold, as the drawer measures it:
       "0" through "-999999", plus absurd ones to exercise the clamp. */
    static const int nums[] = { 0, 8, 16, 24, 30, 40, 56, 80 };

    for( ti = 0; ti < 2; ti++ )
    for( ni = 0; ni < (int)(sizeof(nums)/sizeof(nums[0])); ni++ )
    for( n = 1; n <= MAXPLAYERS; n++ )
    {
        wi_rankfit_t  f;
        wi_rankcol_t  rc;
        int  ytop = tops[ti];
        int  rows, ncol_used, last_bottom;
        char buf[MAXPLAYERNAME + 1];

        WI_Rank_Fit( n, ytop, &f );

        rows = WI_Rank_Rows( n, f.max_rows );
        ncol_used = (n + rows - 1) / rows;

        /* 1. Everybody gets a row. */
        if( rows * ncol_used < n )
            fail("rank y=%%d n=%%d: room for %%d", ytop, n, rows * ncol_used);

        /* 2. Nothing below the screen.  The classic table is allowed to put
              its last colour bar on the bottom line, which it always has. */
        last_bottom = ROW_BOTTOM( ytop + ((rows - 1) * f.pitch) );
        if( last_bottom > (f.compact ? WI_C_BOTTOM : BASEVIDHEIGHT) )
            fail("rank y=%%d n=%%d: last row ends at y %%d",
                 ytop, n, last_bottom);

        /* 3. Rows the drawer will actually draw: it skips any at or past
              y_limit, so y_limit must not cut one off. */
        if( ytop + ((rows - 1) * f.pitch) >= f.y_limit )
            fail("rank y=%%d n=%%d: y_limit %%d clips the last row at %%d",
                 ytop, n, f.y_limit, ytop + ((rows - 1) * f.pitch));

        /* 4. The classic layout, unchanged, for every count that fits it.
              Anything here is a change to a screen that was never wrong. */
        if( !f.compact )
        {
            if( f.pitch != 12 || f.ntable != 4 || f.max_rows != 0
                || f.col_dx != 0 || f.sub_w != 0
                || f.y_limit != BASEVIDHEIGHT
                || f.x[0] != 5 || f.x[1] != 85 || f.x[2] != 165 || f.x[3] != 245 )
                fail("rank y=%%d n=%%d: classic layout altered "
                     "(pitch %%d ntable %%d x %%d/%%d/%%d/%%d)",
                     ytop, n, f.pitch, f.ntable,
                     f.x[0], f.x[1], f.x[2], f.x[3]);
            continue;
        }

        /* 5. Compact only once the classic one has run out. */
        {
            int classic = (BASEVIDHEIGHT - ytop + 11) / 12;
            if( n <= classic )
                fail("rank y=%%d n=%%d: went compact while %%d still fit",
                     ytop, n, classic);
        }

        WI_Rank_Col_Fit( f.sub_w, nums[ni], &rc );

        /* 6. Count, bar and name in that order, none of them overlapping,
              and the name still worth printing however wide the count is. */
        if( rc.num_x > rc.bar_w || rc.name_x < rc.bar_w
            || rc.name_w != f.sub_w - rc.name_x )
            fail("rank sub_w=%%d num=%%d: bar %%d num %%d name %%d+%%d",
                 f.sub_w, nums[ni], rc.bar_w, rc.num_x, rc.name_x, rc.name_w);
        if( rc.name_w < WI_C_NAME_MIN )
            fail("rank sub_w=%%d num=%%d: name field down to %%d units",
                 f.sub_w, nums[ni], rc.name_w);
        if( rc.num_x < nums[ni] 
            && rc.num_x != f.sub_w - WI_C_NAME_MIN - WI_C_NUM_PAD )
            fail("rank sub_w=%%d num=%%d: count field %%d, too narrow for it",
                 f.sub_w, nums[ni], rc.num_x);

        /* 7. The sub-columns a table needs really are the ones it was given
              room for, and the widest name in the last one stays on screen. */
        for( t = 0; t < f.ntable; t++ )
        {
            int  right = f.x[t] + ((ncol_used - 1) * f.col_dx)
                         + rc.name_x + rc.name_w;
            if( right > BASEVIDWIDTH )
                fail("rank y=%%d n=%%d: table %%d ends at x %%d",
                     ytop, n, t, right);
            if( t > 0 && f.x[t] < f.x[t-1] + (ncol_used * f.col_dx) )
                fail("rank y=%%d n=%%d: table %%d overlaps %%d",
                     ytop, n, t, t - 1);
        }

        WI_Fit_Name( buf, sizeof(buf), wide_name, rc.name_w );
        if( V_StringWidth(buf) > rc.name_w )
            fail("rank y=%%d n=%%d: name %%d units in %%d",
                 ytop, n, V_StringWidth(buf), rc.name_w);
        if( buf[0] == 0 )
            fail("rank y=%%d n=%%d: no room for even one letter", ytop, n);

        /* 8. A ranking always shows a count and a name: never no tables. */
        if( f.ntable < 1 )
            fail("rank y=%%d n=%%d: no tables at all", ytop, n);
    }
}

/* ============ the measured truncation ============ */
static void check_fit_name(void)
{
    static const char * names[] = {
        "MMMMMMMMMMMMMMMMMMMM", "IIIIIIIIIIIIIIIIIIII", "WWWWWWWWWW",
        "Player", "Mark", "A", "", "iiiiMMMMiiii", "1111111111",
    };
    int  i, w;

    for( i = 0; i < (int)(sizeof(names)/sizeof(names[0])); i++ )
    for( w = 0; w <= 120; w++ )
    {
        char buf[MAXPLAYERNAME + 1];
        char more[MAXPLAYERNAME + 2];
        int  n;

        WI_Fit_Name( buf, sizeof(buf), names[i], w );

        /* 1. Never wider than asked for. */
        if( V_StringWidth(buf) > w )
            fail("fit \"%%s\" to %%d gave \"%%s\" (%%d units)",
                 names[i], w, buf, V_StringWidth(buf));

        /* 2. A prefix of the name, not something else. */
        if( strncmp(buf, names[i], strlen(buf)) != 0 )
            fail("fit \"%%s\" to %%d gave \"%%s\", not a prefix",
                 names[i], w, buf);

        /* 3. As much of it as will fit: one more letter would be too wide,
              or there are no more letters. */
        n = strlen(buf);
        if( names[i][n] )
        {
            strcpy(more, buf);
            more[n] = names[i][n];
            more[n+1] = 0;
            if( V_StringWidth(more) <= w )
                fail("fit \"%%s\" to %%d stopped early at \"%%s\"",
                     names[i], w, buf);
        }
    }
}

int main(void)
{
    int  i;
    wi_ngfit_t   f8;
    wi_rankfit_t r12;

    for( i = 0; i < MAXPLAYERNAME - 1; i++ )  wide_name[i] = 'M';
    wide_name[MAXPLAYERNAME - 1] = 0;

    check_netgame();
    check_rankings();
    check_classic_wrapper();
    check_fit_name();

    /* The two counts this was all about: 32 players must land in two
       columns of 16 in the netgame table, and two sub-columns of 16 in the
       deathmatch rankings. */
    WI_Netgame_Fit( 32, 23, 10, 0, WI_NG_COMPACT_Y, &f8 );
    if( f8.ncol != 2 || f8.rows != 16 )
        fail("32 players should be 2 columns of 16, got %%d of %%d",
             f8.ncol, f8.rows);

    WI_Rank_Fit( 32, RANKINGY, &r12 );
    if( r12.ntable != 2 || WI_Rank_Rows(32, r12.max_rows) != 16 )
        fail("32 frags should be 2 tables of 16 rows, got %%d tables of %%d",
             r12.ntable, WI_Rank_Rows(32, r12.max_rows));

    /* And the counts that must NOT have changed. */
    WI_Rank_Fit( 12, RANKINGY, &r12 );
    if( r12.compact )
        fail("12 players in a deathmatch should still use the classic table");
    WI_Rank_Fit( 10, TEAMRANKINGY, &r12 );
    if( r12.compact )
        fail("10 teams should still use the classic table");

    /* The names are the point of the measured field widths: 32 players all
       called PLAYERnn must not all read "PLAYER".  This is what that cost
       before the fields were sized to their contents. */
    {
        char buf[MAXPLAYERNAME + 1];
        WI_Netgame_Fit( 32, 19, 0, 0, WI_NG_COMPACT_Y, &f8 );   /* no 100%%, no frags */
        WI_Fit_Name( buf, sizeof(buf), "PLAYER32", f8.name_w - 2 );
        if( strcmp(buf, "PLAYER32") != 0 )
            fail("a 32 player campaign table shows \"%%s\" for PLAYER32", buf);
    }

    printf("  netgame 1..32 x 7 pct x 8 frag widths, rankings 1..32 x "
           "two origins x 8 count widths, name fit 9 names x 121 widths\n");
    if( failures )
    {
        printf("  %%d FAILURES\n", failures);
        return 1;
    }
    printf("  all checks passed\n");
    return 0;
}
'''


def build_and_run(funcs_text, label):
    tmp = tempfile.mkdtemp(prefix='interfit-test.')
    try:
        src = os.path.join(tmp, 't.c')
        exe = os.path.join(tmp, 't')
        with open(src, 'w') as f:
            f.write(HARNESS % {
                'defines': '\n'.join('#define %s %s' % (d, define(wi_s, d))
                                     for d in DEFINES),
                'font': font_c,
                'structs': '\n'.join([struct_def(wi_s, 'wi_rankfit_t'),
                                      struct_def(wi_s, 'wi_rankcol_t'),
                                      struct_def(wi_s, 'wi_ngfit_t')]),
                'funcs': funcs_text,
            })
        r = subprocess.run(['gcc', '-O1', '-Wall', '-o', exe, src],
                           capture_output=True, text=True)
        if r.returncode:
            print('%s: COMPILE FAILED\n%s' % (label, r.stderr))
            return None
        r = subprocess.run([exe], capture_output=True, text=True)
        return r.returncode, r.stdout
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


funcs = ''.join(extract(wi_s, sig) + '\n' for sig in FUNCS)
print('extracted %d lines from wi_stuff.c: %s'
      % (funcs.count('\n'),
         ', '.join(re.search(r'(WI_\w+)\(', s).group(1) for s in FUNCS)))

rc, out = build_and_run(funcs, 'layout')
if out is None:
    sys.exit(2)
print(out, end='')
ok = (rc == 0)


# ---- selfcheck ------------------------------------------------------------

if '--selfcheck' in sys.argv:
    print('\n--selfcheck: reinstating each bug the checks claim to catch')

    bugs = [
        ('netgame never wraps to a second column',
         lambda s: s.replace('out->ncol = (num_pl > per_col) ? 2 : 1;',
                             'out->ncol = 1;')),
        ('netgame columns overlap (no gap accounted for)',
         lambda s: s.replace('- ((out->ncol - 1) * WI_C_COLGAP)) / out->ncol',
                             ') / out->ncol')),
        ('netgame name field eats the number columns',
         lambda s: s.replace('avail = out->col_w - numw - WI_C_MARK_W;',
                             'avail = out->col_w - WI_C_MARK_W;')),
        ('netgame block not centred in its column (pinned to the left)',
         lambda s: s.replace('pad = (out->col_w - (WI_C_MARK_W + out->name_w + numw)) / 2;',
                             'pad = 0;')),
        ('netgame name field takes the whole column however short the names',
         lambda s: s.replace('if( want < avail )  out->name_w = want;', '')),
        ('netgame name field ignores its own heading width',
         lambda s: s.replace('int  floor_w = V_StringWidth("Player");',
                             'int  floor_w = 0;')),
        ('netgame percentage field ceiling raised past what "100" needs',
         lambda s: s.replace('if( pct_w > WI_C_PCT_MAX )  pct_w = WI_C_PCT_MAX;',
                             'if( pct_w > 30 )  pct_w = 30;')),
        ('netgame number fields no longer step by their own width',
         lambda s: s.replace('out->x_items  = out->x_kills + pct_w;',
                             'out->x_items  = out->x_kills + pct_w + 1;')),
        ('ranking count field sized to three digits regardless',
         lambda s: s.replace('if( num_w < floor_w )  num_w = floor_w;',
                             'num_w = V_StringWidth("888");')),
        ('ranking name floor dropped (a huge count squeezes it out)',
         lambda s: s.replace('int  ceil_w  = sub_w - WI_C_NAME_MIN - WI_C_NUM_PAD;',
                             'int  ceil_w  = sub_w;')),
        ('netgame rows unbalanced (fill one column, stub the other)',
         lambda s: s.replace('out->rows = (num_pl + out->ncol - 1) / out->ncol;',
                             'out->rows = per_col;')),
        ('compact rows overrun the bottom of the screen',
         lambda s: s.replace('int  rows = ((WI_C_BOTTOM - (WI_C_ROW_H - 1) - ytop) / WI_C_PITCH) + 1;',
                             'int  rows = ((BASEVIDHEIGHT - ytop) / WI_C_PITCH) + 1;')),
        ('rankings keep four tables when each needs two columns',
         lambda s: s.replace('out->ntable = 4 / ncol;', 'out->ntable = 4;')),
        ('rankings go compact one player too early',
         lambda s: s.replace('if( num_pl <= classic_rows )',
                             'if( num_pl < classic_rows )')),
        ('ranking classic x positions shifted',
         lambda s: s.replace('out->x[0] = 5;  out->x[1] = 85;',
                             'out->x[0] = 6;  out->x[1] = 85;')),
        ('ranking y_limit clips the last compact row',
         lambda s: s.replace('out->y_limit  = ytop + (per_col * WI_C_PITCH);',
                             'out->y_limit  = ytop + ((per_col - 1) * WI_C_PITCH);')),
        ('name truncation counts characters instead of measuring them',
         lambda s: s.replace('w += V_StringWidth( cb );', 'w += 6;')),
        ('name truncation is off by one (one glyph too wide)',
         lambda s: s.replace('if( w > max_w )  break;',
                             'if( w > max_w + 1 )  break;')),
        ('in-level rankings drawn on top of each other (max_rows 1)',
         lambda s: s.replace('colwidth, y_limit, 12, 0, 0, 0 );',
                             'colwidth, y_limit, 12, 1, 0, 0 );')),
        ('ranking rows not balanced across the sub-columns',
         lambda s: s.replace('rows = (scorelines + ncol - 1) / ncol;',
                             'rows = max_rows;')),
    ]

    all_red = True
    for name, mutate in bugs:
        broken = mutate(funcs)
        if broken == funcs:
            print('  %-60s NOT APPLIED (text moved)' % name)
            all_red = False
            continue
        res = build_and_run(broken, name)
        if res is None:
            print('  %-60s COMPILE FAILED' % name)
            all_red = False
            continue
        brc, bout = res
        if brc == 0:
            print('  %-60s NOT CAUGHT' % name)
            all_red = False
        else:
            first = [l for l in bout.splitlines() if 'FAIL' in l]
            print('  %-60s caught  %s' % (name, first[0].strip()[:60] if first else ''))

    print('  selfcheck: %s' % ('every bug went red'
                               if all_red else 'SOME BUGS WERE NOT CAUGHT'))
    ok = ok and all_red

sys.exit(0 if ok else 1)
