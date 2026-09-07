#!/usr/bin/env python3
"""Test the video mode list's aspect ratio filter without a screen.

Nothing drives the menus headlessly, and the failures this filter can have are
all silent ones: a tolerance so tight that 1366x768 is not a 16:9 screen, or so
loose that 5:4 lands in the 4:3 bucket, or an AUTO that resolves to the wrong
shape and hides every mode the display can actually set.  A mode missing from a
list reads as "not supported", never as "the filter ate it" -- which is exactly
how three earlier caps hid video modes (software-fullscreen.md).

So extract VID_Aspect_Match from sdl/i_video.c and vidm_aspect_target and
vidm_aspect_label from m_menu.c VERBATIM, by brace matching, together with the
enum, the ratio table and the tolerance they use, stub what they touch, and
drive them.  A copied test drifts away from the code and then passes forever;
an extracted one tests the text that ships.

Run it from anywhere:  tools/vidaspect-test.py
  --selfcheck   reinstate each bug the checks claim to catch, and report
                whether each one actually goes red.  A clean result from a
                check that has never been shown to fail is worth nothing.
"""
import re, subprocess, sys, os, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRCDIR = os.path.join(HERE, os.pardir, 'svn1749', 'src')
MENU = os.path.join(SRCDIR, 'm_menu.c')
IVID = os.path.join(SRCDIR, 'sdl', 'i_video.c')
IVIDH = os.path.join(SRCDIR, 'i_video.h')

menu_s = open(MENU, encoding='latin-1').read()
ivid_s = open(IVID, encoding='latin-1').read()
ividh_s = open(IVIDH, encoding='latin-1').read()


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


def extract_block(text, start, end):
    """Text from start through the first end after it, inclusive."""
    i = text.index(start)
    j = text.index(end, i) + len(end)
    return text[i:j] + '\n'


# ---- the pieces that ship -------------------------------------------------

tol = re.search(r'^#define\s+VID_ASPECT_TOLERANCE\s+(\d+)', ividh_s, re.M).group(1)

aspect_enum = extract_block(menu_s, 'enum {\n  VIDM_ASPECT_AUTO', '};')
ratio_table = extract_block(menu_s, 'static const uint16_t  vidm_aspect_ratio',
                            '{32,9} };')
poss_values = extract_block(menu_s, 'CV_PossibleValue_t vid_aspect_cons_t[]', '};')

funcs = (extract(ivid_s, 'boolean  VID_Aspect_Match( int w1, int h1, int w2, int h2 )')
         + extract(menu_s, 'static boolean  vidm_aspect_target( int * tw, int * th )')
         + extract(menu_s, 'static void  vidm_mode_shape( int w, int h, int * sw, int * sh )')
         + extract(menu_s, 'static void  vidm_aspect_label( char * buf, int buflen )'))

print('extracted %d lines of filter code; VID_ASPECT_TOLERANCE=%s%%'
      % (funcs.count('\n'), tol))

# The filter decision itself is three lines inside M_DrawVideoMode and cannot be
# lifted out, so check its shape here instead: dropping the "! is_current" term
# would hide the mode in use, which is the entry drawn highlighted.
cond = re.search(r'vidm_mode_shape\(\s*ms\.width,\s*ms\.height,\s*&shape_w,\s*&shape_h\s*\);\s*\n'
                 r'\s*if\(\s*aspect_filtering\s*&&\s*!\s*is_current\s*\n'
                 r'\s*&&\s*!\s*VID_Aspect_Match\(\s*shape_w,\s*shape_h,'
                 r'\s*aspect_w,\s*aspect_h\s*\)\s*\)', menu_s)
if not cond:
    print('  FAIL: the mode filter condition in M_DrawVideoMode is not the '
          'expected "aspect_filtering && !is_current && !VID_Aspect_Match(...)"')
    sys.exit(1)
print('mode filter condition in M_DrawVideoMode: current mode is exempt  OK')


HARNESS = r'''
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef unsigned char byte;
typedef int boolean;
#define true 1
#define false 0

#define VID_ASPECT_TOLERANCE  %(tol)s

/* ---- stubs ---- */
typedef struct { int value; const char * name; } CV_PossibleValue_t;
typedef struct { const char * name; const char * defaultvalue; int flags;
                 CV_PossibleValue_t * pv; byte EV; const char * string; } consvar_t;
#define CV_SAVE 0

static int  stub_display_w = 0, stub_display_h = 0;
static boolean VID_Display_Size( int * w, int * h )
{
    if( stub_display_w <= 0 )  return false;
    *w = stub_display_w;  *h = stub_display_h;  return true;
}

/* ---- verbatim from m_menu.c ---- */
%(enum)s
%(poss)s
static consvar_t cv_vid_aspect = {"vid_aspect", "AUTO", CV_SAVE, vid_aspect_cons_t };
%(table)s

/* ---- verbatim from i_video.c and m_menu.c ---- */
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

static void set_filter( byte ev )
{
    cv_vid_aspect.EV = ev;
    cv_vid_aspect.string = vid_aspect_cons_t[ev].name;
}

/* Would this mode be listed?  The three lines from M_DrawVideoMode, with the
   current-mode exemption left out -- its presence is checked in python. */
static boolean listed( int w, int h )
{
    int tw = 0, th = 0, sw, sh;
    vidm_mode_shape( w, h, &sw, &sh );
    if( ! vidm_aspect_target( &tw, &th ) )  return true;
    return VID_Aspect_Match( sw, sh, tw, th );
}

struct sized { int w, h; const char * why; };

/* Real resolutions, by the bucket they must land in. */
/* 320x200 belongs here, not in 16:10: its pixels are not square.  It is the
   one entry in the whole list whose displayed shape is not its pixel shape. */
static const struct sized r_4_3[]   = { {320,240},{320,200},{640,480},{800,600},{1024,768},
                                        {1152,864},{1280,960},{1400,1050},{1600,1200},{0,0,0} };
/* 1600x1024 is 1.5625, 2.3 percent off 16:10 and so inside the tolerance. */
static const struct sized r_16_10[] = { {1280,800},{1440,900},{1600,1024},{1680,1050},
                                        {1920,1200},{2560,1600},{0,0,0} };
static const struct sized r_16_9[]  = { {1280,720},{1366,768},{1600,900},{1920,1080},
                                        {2560,1440},{3840,2160},{0,0,0} };
static const struct sized r_21_9[]  = { {2560,1080},{3440,1440},{3840,1600},{5120,2160},{0,0,0} };
static const struct sized r_32_9[]  = { {3840,1080},{5120,1440},{0,0,0} };
/* Shapes that belong to no named bucket and so are reachable only under All. */
static const struct sized r_none[]  = { {1280,1024},{1024,600},{1280,600},{0,0,0} };

static const struct sized * const buckets[] =
  { r_4_3, r_16_10, r_16_9, r_21_9, r_32_9 };
static const byte bucket_ev[] =
  { VIDM_ASPECT_4_3, VIDM_ASPECT_16_10, VIDM_ASPECT_16_9,
    VIDM_ASPECT_21_9, VIDM_ASPECT_32_9 };
static const char * bucket_name[] = { "4:3", "16:10", "16:9", "21:9", "32:9" };

int main(void)
{
    int b, c, i, ev;
    char lbl[32];

    /* 1. Every named filter accepts every resolution of its own shape... */
    for( b = 0; b < 5; b++ )
    {
        set_filter( bucket_ev[b] );
        for( i = 0; buckets[b][i].w; i++ )
            if( ! listed( buckets[b][i].w, buckets[b][i].h ) )
                fail("%%s filter rejected %%dx%%d, which is %%s",
                     bucket_name[b], buckets[b][i].w, buckets[b][i].h, bucket_name[b]);
    }

    /* 2. ...and no resolution of any OTHER named shape.  This is the check
          that pins the tolerance from above: at 7%% or more, 5:4 (1280x1024)
          joins the 4:3 bucket. */
    for( b = 0; b < 5; b++ )
    {
        set_filter( bucket_ev[b] );
        for( c = 0; c < 5; c++ )
        {
            if( c == b )  continue;
            for( i = 0; buckets[c][i].w; i++ )
                if( listed( buckets[c][i].w, buckets[c][i].h ) )
                    fail("%%s filter accepted %%dx%%d, which is %%s",
                         bucket_name[b], buckets[c][i].w, buckets[c][i].h, bucket_name[c]);
        }
        for( i = 0; r_none[i].w; i++ )
            if( listed( r_none[i].w, r_none[i].h ) )
                fail("%%s filter accepted %%dx%%d, which is no named shape",
                     bucket_name[b], r_none[i].w, r_none[i].h);
    }

    /* 3. "All" hides nothing, including the shapes no bucket owns. */
    set_filter( VIDM_ASPECT_ALL );
    for( b = 0; b < 5; b++ )
        for( i = 0; buckets[b][i].w; i++ )
            if( ! listed( buckets[b][i].w, buckets[b][i].h ) )
                fail("All hid %%dx%%d", buckets[b][i].w, buckets[b][i].h);
    for( i = 0; r_none[i].w; i++ )
        if( ! listed( r_none[i].w, r_none[i].h ) )
            fail("All hid %%dx%%d", r_none[i].w, r_none[i].h);

    /* 4. AUTO follows the display, for each display shape in turn. */
    set_filter( VIDM_ASPECT_AUTO );
    for( b = 0; b < 5; b++ )
    {
        stub_display_w = buckets[b][0].w;
        stub_display_h = buckets[b][0].h;
        for( c = 0; c < 5; c++ )
            for( i = 0; buckets[c][i].w; i++ )
            {
                boolean want = (c == b);
                if( listed( buckets[c][i].w, buckets[c][i].h ) != want )
                    fail("AUTO on a %%s display %%s %%dx%%d (%%s)",
                         bucket_name[b], want ? "hid" : "listed",
                         buckets[c][i].w, buckets[c][i].h, bucket_name[c]);
            }
    }

    /* 5. AUTO on a display whose size cannot be read must hide NOTHING.
          Guessing would empty the list on the machines least able to say so. */
    stub_display_w = stub_display_h = 0;
    set_filter( VIDM_ASPECT_AUTO );
    for( i = 0; r_none[i].w; i++ )
        if( ! listed( r_none[i].w, r_none[i].h ) )
            fail("AUTO with no display size hid %%dx%%d", r_none[i].w, r_none[i].h);

    /* 6. A value out of range -- config.cfg is a text file and can say
          anything -- must also hide nothing rather than index off the table. */
    cv_vid_aspect.EV = VIDM_ASPECT_NUM + 3;
    cv_vid_aspect.string = "rubbish";
    if( ! listed( 1280, 1024 ) )
        fail("an out of range vid_aspect hid a mode");

    /* 7. The A key cycles every value and comes back round. */
    for( ev = 0; ev < VIDM_ASPECT_NUM; ev++ )
        if( vid_aspect_cons_t[ev].value != ev )
            fail("vid_aspect_cons_t[%%d] has value %%d, so EV cannot index the "
                 "ratio table", ev, vid_aspect_cons_t[ev].value);
    if( ((VIDM_ASPECT_NUM - 1) + 1) %% VIDM_ASPECT_NUM != VIDM_ASPECT_AUTO )
        fail("cycling past the last value does not return to AUTO");

    /* 8. The label names the display AUTO resolved to. */
    stub_display_w = 3440; stub_display_h = 1440;
    set_filter( VIDM_ASPECT_AUTO );
    vidm_aspect_label( lbl, sizeof(lbl) );
    if( strcmp( lbl, "AUTO 3440x1440" ) )
        fail("AUTO label is \"%%s\", expected \"AUTO 3440x1440\"", lbl);
    stub_display_w = stub_display_h = 0;
    vidm_aspect_label( lbl, sizeof(lbl) );
    if( strcmp( lbl, "AUTO" ) )
        fail("AUTO label with no display size is \"%%s\", expected \"AUTO\"", lbl);
    set_filter( VIDM_ASPECT_21_9 );
    vidm_aspect_label( lbl, sizeof(lbl) );
    if( strcmp( lbl, "21:9" ) )
        fail("21:9 label is \"%%s\"", lbl);

    if( failures )
        printf("%%d failures\n", failures);
    else
        printf("aspect filter: 5 shapes, %%d resolutions, AUTO, All, "
               "out of range, labels -- 0 failures\n",
               (int)(sizeof(r_4_3)/sizeof(r_4_3[0]) - 1
                   + sizeof(r_16_10)/sizeof(r_16_10[0]) - 1
                   + sizeof(r_16_9)/sizeof(r_16_9[0]) - 1
                   + sizeof(r_21_9)/sizeof(r_21_9[0]) - 1
                   + sizeof(r_32_9)/sizeof(r_32_9[0]) - 1
                   + sizeof(r_none)/sizeof(r_none[0]) - 1));
    return failures ? 1 : 0;
}
'''


def build_and_run(tolerance=None, funcs_text=None, quiet=False):
    tmp = tempfile.mkdtemp(prefix='vidaspect-test.') + os.sep
    try:
        csrc = tmp + 't.c'
        exe = tmp + 't'
        body = HARNESS % dict(tol=tolerance if tolerance is not None else tol,
                              enum=aspect_enum, poss=poss_values,
                              table=ratio_table,
                              funcs=funcs_text if funcs_text is not None else funcs)
        open(csrc, 'w').write('#include <stdarg.h>\n' + body)
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
    print('\nselfcheck -- each of these reinstates a real bug; every one must go RED')
    ok = True
    cases = [
        ('tolerance 0% (nothing but an exact ratio matches, so 1366x768 '
         'stops being a 16:9 screen)', dict(tolerance='0')),
        ('tolerance 8% (5:4 joins the 4:3 bucket)', dict(tolerance='8')),
        ('AUTO falls back to "hide everything" when the display size is '
         'unknown', dict(funcs_text=funcs.replace(
             '        return VID_Display_Size( tw, th );',
             '        { VID_Display_Size( tw, th ); return true; }'))),
        ('the ratio table row for 21:9 written as 21:9 rather than 64:27',
         dict(funcs_text=funcs)),
    ]
    for name, kw in cases[:3]:
        rc2, out2 = build_and_run(quiet=True, **kw)
        red = (rc2 != 0)
        print('  %-72s %s' % (name, 'RED (good)' if red else 'still green -- '
                              'the check does not catch it'))
        ok = ok and red
    # The 21:9 row is 64:27; writing it as 21:9 (2.333) is only 1.5% off 64:27
    # (2.370) and inside the tolerance, so it is NOT a bug the checks can catch.
    # Recorded here so nobody adds a check that claims to.
    print('  %-72s %s' % ('(21:9 written as 21:9 instead of 64:27 is within '
                          'tolerance -- not testable)', 'n/a'))
    sys.exit(0 if ok else 1)

sys.exit(rc)
