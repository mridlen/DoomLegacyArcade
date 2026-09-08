#!/usr/bin/env python3
"""Test where the finished frame lands on the panel, without a screen.

Present_Fit_Rect decides whether the software renderer's frame is stretched to
fill the display or fitted inside it with black bars, and where.  Nothing about
that is visible from a headless run: a screenshot is of the engine's own draw
buffer, taken before this runs, so a rectangle that is off-centre, off the
panel, or letterboxing the wrong axis looks identical in every capture.  Only
someone in front of the cabinet would see it.

So extract Present_Fit_Rect from sdl/i_video.c VERBATIM, by brace matching,
and drive it over every panel and drawing size that matters -- including
portrait panels, where the bars must move to the top and bottom.  A copied
test drifts away from the code and then passes forever; an extracted one tests
the text that ships.

Run it from anywhere:  tools/screenfit-test.py
  --selfcheck   reinstate each bug the checks claim to catch, and report
                whether each one actually goes red.  A clean result from a
                check that has never been shown to fail is worth nothing --
                two of vidmenu-navtest's five checks were silently useless
                until this was run on them.
"""
import re, subprocess, sys, os, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRCDIR = os.path.join(HERE, os.pardir, 'svn1749', 'src')
IVID = os.path.join(SRCDIR, 'sdl', 'i_video.c')

ivid_s = open(IVID, encoding='latin-1').read()


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


SIG = 'void Present_Fit_Rect( int src_w, int src_h, int out_w, int out_h,'
func = extract(ivid_s, SIG)
snap = re.search(r'^#define\s+PRESENT_FIT_SNAP\s+(\d+)', ivid_s, re.M).group(1)

print('extracted %d lines of Present_Fit_Rect; PRESENT_FIT_SNAP=%s'
      % (func.count('\n'), snap))


HARNESS = r'''
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#define PRESENT_FIT_SNAP  %(snap)s

/* ---- verbatim from sdl/i_video.c ---- */
%(func)s

/* ---- checks ---- */
static int failures = 0;
static void fail(const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if( failures++ < 25 ) { printf("  FAIL "); vprintf(fmt, ap); printf("\n"); }
    va_end(ap);
}

/* Panels the cabinet or a port of it might actually have, landscape and
   portrait, plus the engine's extremes. */
static const int panels[][2] = {
    {1366,768}, {1920,1080}, {1280,1024}, {1024,768}, {800,600}, {640,480},
    {2560,1440}, {3440,1440}, {5120,1440}, {3840,2160}, {1280,800},
    /* portrait: a monitor turned on its side, which is the case the bars
       have to move axis for */
    {768,1366}, {1080,1920}, {1200,1600}, {600,800}, {1440,2560},
    /* square-ish, where neither axis is obviously the binding one */
    {1000,1000}, {1024,1000}, {1000,1024},
};
#define NPANELS (int)(sizeof(panels)/sizeof(panels[0]))

static const int draws[][2] = {
    {320,200}, {320,240}, {640,400}, {640,480}, {800,600}, {1024,768},
    {1280,720}, {1280,800}, {1366,768}, {1600,1200}, {1920,1080},
    {2560,1440}, {3440,1440}, {5120,1440}, {5120,2160}, {3840,2160},
    {768,1366}, {600,800}, {1080,1920}, {1000,1000},
};
#define NDRAWS (int)(sizeof(draws)/sizeof(draws[0]))

int main(void)
{
    int p, d;
    int checked = 0, barred = 0, letterboxed = 0, pillarboxed = 0, filled = 0;

    for( p = 0; p < NPANELS; p++ )
    {
        int ow = panels[p][0], oh = panels[p][1];

        for( d = 0; d < NDRAWS; d++ )
        {
            int sw = draws[d][0], sh = draws[d][1];
            int dx, dy, dw, dh;

            /* ---- keep_aspect OFF: must always fill, exactly as the stock
               NULL-rectangle RenderCopy did.  Any deviation changes what
               every existing install sees. ---- */
            Present_Fit_Rect( sw, sh, ow, oh, 0, &dx, &dy, &dw, &dh );
            if( dx != 0 || dy != 0 || dw != ow || dh != oh )
                fail("keep_aspect off did not fill: %%dx%%d in %%dx%%d -> "
                     "%%d,%%d %%dx%%d", sw, sh, ow, oh, dx, dy, dw, dh);

            /* ---- keep_aspect ON ---- */
            Present_Fit_Rect( sw, sh, ow, oh, 1, &dx, &dy, &dw, &dh );
            checked++;

            /* 1. Never off the panel.  An off-by-one here draws past the
                  edge of the display. */
            if( dw <= 0 || dh <= 0 || dx < 0 || dy < 0
                || dx + dw > ow || dy + dh > oh )
            {
                fail("rect outside the panel: %%dx%%d in %%dx%%d -> "
                     "%%d,%%d %%dx%%d", sw, sh, ow, oh, dx, dy, dw, dh);
                continue;
            }

            /* 2. Centred: the two bars differ by at most one pixel, which is
                  all integer division can leave. */
            {
                int lead_x = dx, trail_x = ow - dw - dx;
                int lead_y = dy, trail_y = oh - dh - dy;
                if( abs(lead_x - trail_x) > 1 || abs(lead_y - trail_y) > 1 )
                    fail("not centred: %%dx%%d in %%dx%%d -> bars x %%d/%%d "
                         "y %%d/%%d", sw, sh, ow, oh,
                         lead_x, trail_x, lead_y, trail_y);
            }

            if( dw == ow && dh == oh )
            {
                filled++;

                /* 3. It only fills when the shapes really do agree.  The snap
                      tolerance is meant for a rounding-width mismatch like
                      1280x720 inside 1366x768, not for hiding a stretch.
                      Allow at most what SNAP can account for on either axis. */
                long fit_h = ((long)ow * sh) / sw;
                long fit_w = ((long)oh * sw) / sh;
                if( (oh - fit_h) > PRESENT_FIT_SNAP
                    && (ow - fit_w) > PRESENT_FIT_SNAP )
                    fail("filled but shapes differ: %%dx%%d in %%dx%%d "
                         "(would fit %%ldx%%d or %%dx%%ld)",
                         sw, sh, ow, oh, fit_w, oh, ow, fit_h);
            }
            else
            {
                barred++;

                /* 4. Bars on exactly one axis, and the right one.  A frame
                      proportionally wider than the panel is bounded by width,
                      so its bars go top and bottom -- the portrait case. */
                if( dw < ow && dh < oh )
                    fail("bars on BOTH axes: %%dx%%d in %%dx%%d -> %%dx%%d",
                         sw, sh, ow, oh, dw, dh);

                if( (long)sw * oh > (long)ow * sh )
                {
                    letterboxed++;
                    if( dw != ow )
                        fail("wider frame should fit the width: %%dx%%d in "
                             "%%dx%%d -> %%dx%%d", sw, sh, ow, oh, dw, dh);
                }
                else
                {
                    pillarboxed++;
                    if( dh != oh )
                        fail("taller frame should fit the height: %%dx%%d in "
                             "%%dx%%d -> %%dx%%d", sw, sh, ow, oh, dw, dh);
                }

                /* 5. The shape is actually kept.  Compare by cross-multiply
                      and allow one pixel of truncation on the fitted axis. */
                {
                    long a = (long)dw * sh, b = (long)dh * sw;
                    long tol = (long)sw + sh;   /* one pixel either way */
                    if( labs(a - b) > tol )
                        fail("aspect not kept: %%dx%%d in %%dx%%d -> %%dx%%d "
                             "(%%.4f vs %%.4f)", sw, sh, ow, oh, dw, dh,
                             (double)dw/dh, (double)sw/sh);
                }
            }
        }
    }

    /* 6. The specific mismatch the snap exists for: a 16:9 drawing size on a
          1366x768 panel is 0.05%% off and must come out filled, not with a
          single black column down one side. */
    {
        int dx, dy, dw, dh;
        Present_Fit_Rect( 1280, 720, 1366, 768, 1, &dx, &dy, &dw, &dh );
        if( dw != 1366 || dh != 768 )
            fail("1280x720 in 1366x768 should snap to full, got %%dx%%d",
                 dw, dh);
    }

    /* 7. The case that started all this: 4:3 on the cabinet's panel must be
          pillarboxed, not stretched. */
    {
        int dx, dy, dw, dh;
        Present_Fit_Rect( 1024, 768, 1366, 768, 1, &dx, &dy, &dw, &dh );
        if( dw != 1024 || dh != 768 || dx != 171 )
            fail("1024x768 in 1366x768 should be 1024x768 at x=171, got "
                 "%%dx%%d at x=%%d", dw, dh, dx);
    }

    /* 8. Turned on its side: 4:3 on a portrait panel must letterbox. */
    {
        int dx, dy, dw, dh;
        Present_Fit_Rect( 1024, 768, 768, 1366, 1, &dx, &dy, &dw, &dh );
        if( dw != 768 || dh != 576 || dx != 0 || dy != 395 )
            fail("1024x768 in 768x1366 should be 768x576 at y=395, got "
                 "%%dx%%d at %%d,%%d", dw, dh, dx, dy);
    }

    printf("  %%d combinations: %%d filled, %%d barred "
           "(%%d letterboxed, %%d pillarboxed)\n",
           checked, filled, barred, letterboxed, pillarboxed);
    if( failures )
    {
        printf("  %%d FAILURES\n", failures);
        return 1;
    }
    printf("  all checks passed\n");
    return 0;
}
'''


def build_and_run(func_text, label):
    tmp = tempfile.mkdtemp(prefix='screenfit-test.')
    try:
        src = os.path.join(tmp, 't.c')
        exe = os.path.join(tmp, 't')
        with open(src, 'w') as f:
            f.write(HARNESS % {'func': func_text, 'snap': snap})
        r = subprocess.run(['gcc', '-O1', '-Wall', '-o', exe, src],
                           capture_output=True, text=True)
        if r.returncode:
            print('%s: COMPILE FAILED\n%s' % (label, r.stderr))
            return None
        r = subprocess.run([exe], capture_output=True, text=True)
        return r.returncode, r.stdout
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# ---- the real run ---------------------------------------------------------

rc, out = build_and_run(func, 'Present_Fit_Rect')
print(out, end='')
ok = (rc == 0)

# ---- selfcheck ------------------------------------------------------------

if '--selfcheck' in sys.argv:
    print('\n--selfcheck: reinstating each bug the checks claim to catch')

    bugs = [
        ('comparison reversed (bars on the wrong axis)',
         lambda s: s.replace('(int64_t)src_w * out_h > (int64_t)out_w * src_h',
                             '(int64_t)src_w * out_h < (int64_t)out_w * src_h')),
        ('centring dropped (frame pinned to the corner)',
         lambda s: s.replace('*dx = (out_w - w) / 2;', '*dx = 0;')
                    .replace('*dy = (out_h - h) / 2;', '*dy = 0;')),
        ('snap removed (a one pixel bar at 1280x720)',
         lambda s: s.replace('<= PRESENT_FIT_SNAP', '< 0')),
        ('keep_aspect ignored (always fits, even when told to stretch)',
         lambda s: s.replace('if( keep_aspect && src_w > 0',
                             'if( 1 && src_w > 0')),
        ('fitted axis swapped (fits the wrong side)',
         lambda s: s.replace('h = (int)(((int64_t)out_w * src_h) / src_w);',
                             'w = (int)(((int64_t)out_h * src_w) / src_h);')),
    ]

    all_red = True
    for name, mutate in bugs:
        broken = mutate(func)
        if broken == func:
            print('  %-58s NOT APPLIED (text moved)' % name)
            all_red = False
            continue
        res = build_and_run(broken, name)
        if res is None:
            print('  %-58s compile failed' % name)
            all_red = False
            continue
        brc, _ = res
        if brc != 0:
            print('  %-58s caught' % name)
        else:
            print('  %-58s NOT CAUGHT -- the check is useless' % name)
            all_red = False

    if not all_red:
        print('\nselfcheck: at least one bug was not caught')
        sys.exit(1)
    print('\nselfcheck: every reinstated bug was caught')

sys.exit(0 if ok else 1)
