#!/usr/bin/env python3
"""Check that overlay art is placed at the scale it is actually DRAWN at.

Two bugs, one cause.  The 2D layer is 320x200 base units multiplied by a
scale, and there are two different scales in play: the whole number
vid.dupx/vid.dupy that the SOFTWARE renderer draws art at, and the exact
vid.fdupx/vid.fdupy that the HARDWARE renderer draws art at and that spans the
real screen.  Mix them and the picture is wrong in a way that depends on the
resolution, which is what made both of these read as renderer bugs:

  - ST_drawOverlayNum stepped the status digits by wf * vid.dupx while OpenGL
    drew each one wf * vid.fdupx wide.  At 1366x768 that is a 42 pixel step for
    a 59.7 pixel digit, so "100" ran together.  Software was right by accident:
    there the two are the same number.

  - hu_stuff.c centred its overlay strings on BASEVIDWIDTH, which centres them
    in the 320*dupx box and not on the screen.  In software that box is smaller
    than the screen wherever the division is not exact, so PRESS FIRE TO START
    sat half the leftover left of centre -- 96px at 512x384, 203px at 1366x768,
    and nothing at all at 640x480 or 1920x1080.

Neither is visible to a headless run and neither is measured by anything else,
so extract the arithmetic that ships -- ST_drawOverlayNum from st_stuff.c, the
HU_* placement helpers from hu_stuff.c, and V_Setup_VideoDraw's scale
derivation from v_video.c -- VERBATIM, by brace matching, stub what they touch,
and drive them over a table of resolutions in both renderers.  A copied test
drifts away from the code and then passes forever; an extracted one tests the
text that ships.

Run it from anywhere:  tools/hudtext-test.py
  --selfcheck   reinstate each bug the checks claim to catch, and report
                whether each one actually goes red.  A clean result from a
                check that has never been shown to fail is worth nothing.
"""
import re, subprocess, sys, os, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SRCDIR = os.path.join(HERE, os.pardir, 'svn1749', 'src')
ST = os.path.join(SRCDIR, 'st_stuff.c')
HU = os.path.join(SRCDIR, 'hu_stuff.c')
VV = os.path.join(SRCDIR, 'v_video.c')

st_s = open(ST, encoding='latin-1').read()
hu_s = open(HU, encoding='latin-1').read()
vv_s = open(VV, encoding='latin-1').read()


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

# The digit drawer, and its advance.
f_overlaynum = extract(st_s, 'void ST_drawOverlayNum (int x, int y,')

# The overlay text placement helpers.
f_scalex = extract(hu_s, 'static float HU_Art_ScaleX(void)')
f_scaley = extract(hu_s, 'static float HU_Art_ScaleY(void)')
f_centerx = extract(hu_s, 'static int HU_Center_X( int base_w )')
f_screeny = extract(hu_s, 'static int HU_Screen_Y( int base_y )')
f_centery = extract(hu_s, 'static int HU_Center_Y( int base_h, int base_bottom )')

# How the two scales are derived from a resolution.  This is the whole reason
# the two differ, so it is taken from the source too rather than restated.
b_scales = extract_block(vv_s,
                         'vid.dupx_fill = vid.width / BASEVIDWIDTH;',
                         'vid.dupx = dx;\n    }')

HARNESS = r'''
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned char byte;
typedef int boolean;
#define false 0
#define true  1

#define BASEVIDWIDTH   320
#define BASEVIDHEIGHT  200
#define ST_HEIGHT       32

// Draw flags: only ever OR'd together and handed to the stub.
#define FG                    0x01
#define V_NOSCALE             0x02
#define V_SCALEPATCH          0x04
#define V_TRANSLUCENTPATCH    0x08
#define FLASH_COLOR           0xB0

typedef enum { render_soft = 1, render_opengl = 2 } rendermode_t;
rendermode_t rendermode;

typedef struct {
    int   width, height;
    int   dupx_fill, dupy;
    float fdupx_fill, fdupy;
    float fdupx;
    byte  dupx;
} viddef_t;
viddef_t vid;

typedef struct { int width, height; } patch_t;
#define V_patch(p)  (p)

typedef struct { int value; int EV; } consvar_t;
consvar_t cv_pickupflash = { 0, 0 };

// --- what the extracted drawer calls --------------------------------------
#define MAXREC 32
static int  rec_x[MAXREC], rec_w[MAXREC];
static int  rec_n;
static int  fill_x, fill_w, fill_h, fill_n;

static void V_SetupDraw( unsigned int f ) { (void)f; }

static void V_DrawScaledPatch( int x, int y, patch_t * p )
{
    // The width a patch is ACTUALLY drawn: drawinfo takes vid.dupx from
    // V_SCALEPATCH in software, and HWR_DrawPatch takes vid.fdupx in hardware.
    float s = (rendermode == render_soft)? (float)vid.dupx : vid.fdupx;
    (void)y;
    if( rec_n < MAXREC )
    {
        rec_x[rec_n] = x;
        rec_w[rec_n] = (int)((p->width * s) + 0.5f);
        rec_n++;
    }
}

static void V_DrawVidFill( int x, int y, int w, int h, byte c )
{
    (void)y; (void)c;
    fill_x = x;  fill_w = w;  fill_h = h;  fill_n++;
}

// --- the scale derivation, verbatim ---------------------------------------
static void set_mode( int w, int h )
{
    vid.width = w;
    vid.height = h;
@SCALES@
}

// --- the code under test, verbatim ----------------------------------------
@OVERLAYNUM@
@SCALEX@
@SCALEY@
@CENTERX@
@SCREENY@
@CENTERY@

// --- the checks -----------------------------------------------------------
static int failures;

static void fail( const char * what, int w, int h, const char * ren,
                  const char * detail )
{
    printf("  FAIL  %-26s %4dx%-4d %-8s  %s\n", what, w, h, ren, detail);
    failures++;
}

// [Arcade] The whole-number pair vid.dupx/vid.dupy must be the closest
// achievable to the exact fdupx/fdupy ratio -- and where two pairs are equally
// close, it must not be the NARROWER of them.
//
// The tie-break is the whole point.  At 800x600 the exact ratio is 0.833 and
// both 2x2 (1.000) and 2x3 (0.667) are 0.167 away, so a plain error tolerance
// cannot tell them apart -- they are wrong by the same amount in opposite
// directions.  2x3 was what the code picked, and it is the one that looks
// broken: HUD art a third narrower than it is tall, on the only 4:3 mode in
// the list that does it, sitting between 640x480 and 1024x768 which both come
// out 1.000.  Erring wide is what every other resolution already does and what
// the art tolerates; erring narrow is what someone notices from across a room.
static void check_scale_pair( int w, int h, const char * ren )
{
    float  exact = vid.fdupx / vid.fdupy;
    float  chosen = (float)vid.dupx / (float)vid.dupy;
    float  chosen_err = fabsf( chosen - exact );
    int    maxx = w / BASEVIDWIDTH;      // the floors, before any lowering
    int    maxy = h / BASEVIDHEIGHT;
    float  best = 1.0e9f;
    int    dx, dy;
    char   buf[160];

    if( maxx < 1 )  maxx = 1;
    if( maxy < 1 )  maxy = 1;

    for( dy = 1; dy <= maxy; dy++ )
        for( dx = 1; dx <= maxx; dx++ )
        {
            float e = fabsf( ((float)dx / (float)dy) - exact );
            if( e < best )  best = e;
        }

    if( chosen_err > best + 1.0e-4f )
    {
        snprintf(buf, sizeof buf,
            "dupx/dupy %d/%d = %.3f, exact %.3f, err %.3f but %.3f was available",
            vid.dupx, vid.dupy, chosen, exact, chosen_err, best);
        fail("scale pair not the closest", w, h, ren, buf);
        return;
    }

    for( dy = 1; dy <= maxy; dy++ )
        for( dx = 1; dx <= maxx; dx++ )
        {
            float r = (float)dx / (float)dy;
            float e = fabsf( r - exact );
            if( e <= best + 1.0e-4f && r > chosen + 1.0e-4f )
            {
                snprintf(buf, sizeof buf,
                    "dupx/dupy %d/%d = %.3f is narrower than %d/%d = %.3f, "
                    "which is exactly as close to %.3f",
                    vid.dupx, vid.dupy, chosen, dx, dy, r, exact);
                fail("narrower of two tied pairs", w, h, ren, buf);
                return;
            }
        }
}


// STTNUM digits are 14x16; STYSNUM (the compact ones) are 4x6.
static patch_t tallnum[11];
static patch_t shortnum[11];

static void check_digits( int w, int h, const char * ren, patch_t * num,
                          const char * name )
{
    int i;
    char detail[256];

    rec_n = 0;  fill_n = 0;
    {
        patch_t * np[11];
        for( i = 0; i < 11; i++ )  np[i] = &num[i];
        // Three digits, right edge at 300 base units out.
        ST_drawOverlayNum( (int)(300 * vid.fdupx_fill), 100, 100, np, NULL, 0 );
    }

    if( rec_n != 3 )
    {
        snprintf(detail, sizeof(detail), "%s: drew %d digits, expected 3", name, rec_n);
        fail("digit count", w, h, ren, detail);
        return;
    }

    // The drawer walks right to left, so each call is FURTHER LEFT than the
    // one before: rec_x descends.  Digit i+1 must end exactly where digit i
    // begins -- no overlap, no gap.
    for( i = 0; i + 1 < rec_n; i++ )
    {
        int endsat = rec_x[i + 1] + rec_w[i + 1];   // where the left one ends
        int err = endsat - rec_x[i];                // + overlaps, - gaps
        if( err < -1 || err > 1 )
        {
            snprintf(detail, sizeof(detail),
                     "%s: digit spans %d..%d, the next starts at %d (%s %dpx)",
                     name, rec_x[i + 1], endsat, rec_x[i],
                     (err > 0)? "overlap" : "gap", (err > 0)? err : -err);
            fail("digit advance", w, h, ren, detail);
            return;
        }
    }
}

static void check_flash( int w, int h, const char * ren )
{
    char detail[256];
    patch_t * np[11];
    int i, span;
    for( i = 0; i < 11; i++ )  np[i] = &tallnum[i];

    cv_pickupflash.EV = 1;
    rec_n = 0;  fill_n = 0;
    ST_drawOverlayNum( (int)(300 * vid.fdupx_fill), 100, 100, np, NULL, 1 );
    cv_pickupflash.EV = 0;

    if( fill_n != 1 )
    {
        fail("pickup flash", w, h, ren, "no fill drawn");
        return;
    }
    // The flash is meant to sit behind three digits, so it must be as wide as
    // three digits are drawn and as tall as one is drawn.  rec_x descends.
    span = (rec_x[0] + rec_w[0]) - rec_x[rec_n - 1];
    if( abs(fill_w - span) > 3 )
    {
        snprintf(detail, sizeof(detail),
                 "fill is %dpx wide, three digits span %dpx", fill_w, span);
        fail("pickup flash width", w, h, ren, detail);
    }
    {
        float sy = (rendermode == render_soft)? (float)vid.dupy : vid.fdupy;
        int   want = (int)((tallnum[0].height * sy) + 0.5f);
        if( abs(fill_h - want) > 1 )
        {
            snprintf(detail, sizeof(detail),
                     "fill is %dpx tall, a digit is drawn %dpx", fill_h, want);
            fail("pickup flash height", w, h, ren, detail);
        }
    }
}

// A centred string must be centred ON THE SCREEN.  The base-unit result is
// multiplied by the art scale to reach pixels (V_DrawString: cx = x * dupx0),
// so it can only be exact to within one whole base unit -- that quantisation
// is the art scale, and the tolerance below is exactly it and no more.
static void check_center_x( int w, int h, const char * ren, int base_w,
                            const char * name )
{
    float s = (rendermode == render_soft)? (float)vid.dupx : vid.fdupx;
    int   x_px = (int)( HU_Center_X( base_w ) * s );
    int   drawn = (int)( base_w * s );
    int   left = x_px;
    int   right = vid.width - (x_px + drawn);
    char  detail[256];

    // x is in base units and reaches pixels as x * art scale, so it can only
    // land within one base unit of the ideal -- and the two margins then
    // differ by up to twice that.  The tolerance is that quantisation exactly.
    if( abs(left - right) > (int)((2.0f * s) + 1.0f) )
    {
        snprintf(detail, sizeof(detail),
                 "%s: %dpx left, %dpx right (off centre by %d)",
                 name, left, right, abs(left - right) / 2);
        fail("centred on screen", w, h, ren, detail);
    }
}

// PRESS FIRE TO START is anchored just above the status bar, at layout row
// BASEVIDHEIGHT - ST_HEIGHT - 8.
//
// "It clears the status bar" is NOT the check to write, and --selfcheck is how
// that was found out: the old code put the text 160*dupy down a screen that is
// 200*fdupy tall, which is far too HIGH -- it floated in mid screen at 512x384
// -- and a test that only asks whether it stays above the bar passes happily on
// exactly that.  The property is that the row lands where the layout puts it,
// so measure the anchor against the screen and then check it still clears both
// forms of the bar.
static void check_press_fire_y( int w, int h, const char * ren )
{
    float sy = (rendermode == render_soft)? (float)vid.dupy : vid.fdupy;
    int   row = BASEVIDHEIGHT - ST_HEIGHT - 8;
    int   y_px = (int)( HU_Screen_Y( row ) * sy );
    int   want = (int)( row * vid.fdupy );
    int   bottom = y_px + (int)(7 * sy);           // hu_font glyphs are 7 tall
    int   classic_px = (int)((BASEVIDHEIGHT - ST_HEIGHT) * vid.fdupy);
    int   overlay_px = (int)(198 * vid.fdupy) - (int)(16 * sy);
    char  detail[256];

    // Quantised to whole base units, as every V_SCALESTART position is.
    if( abs(y_px - want) > (int)(sy + 1.0f) )
    {
        snprintf(detail, sizeof(detail),
                 "row %d lands at %dpx, the screen puts it at %dpx",
                 row, y_px, want);
        fail("anchored to the screen", w, h, ren, detail);
    }
    if( bottom > classic_px )
    {
        snprintf(detail, sizeof(detail),
                 "text ends at %dpx, classic status bar starts at %dpx",
                 bottom, classic_px);
        fail("clears status bar", w, h, ren, detail);
    }
    if( bottom > overlay_px )
    {
        snprintf(detail, sizeof(detail),
                 "text ends at %dpx, overlay HUD row is at %dpx",
                 bottom, overlay_px);
        fail("clears overlay HUD", w, h, ren, detail);
    }
    if( y_px < 0 || bottom > vid.height )
    {
        snprintf(detail, sizeof(detail), "text at %d..%d, screen is %d tall",
                 y_px, bottom, vid.height);
        fail("on screen", w, h, ren, detail);
    }
}

// GAME OVER is centred in the band above the status bar.
static void check_center_y( int w, int h, const char * ren )
{
    float sy = (rendermode == render_soft)? (float)vid.dupy : vid.fdupy;
    int   band = (int)((BASEVIDHEIGHT - ST_HEIGHT) * vid.fdupy);
    int   art_h = 24;                              // a plausible M_GAMOVR
    int   y_px = (int)( HU_Center_Y( art_h, BASEVIDHEIGHT - ST_HEIGHT ) * sy );
    int   above = y_px;
    int   below = band - (y_px + (int)(art_h * sy));
    char  detail[256];

    if( abs(above - below) > (int)((2.0f * sy) + 1.0f) )
    {
        snprintf(detail, sizeof(detail), "%dpx above, %dpx below", above, below);
        fail("centred in band", w, h, ren, detail);
    }
}

// The whole status row, full screen, three digit values.  Widening the digits
// is what fixes the overlap, and the risk it carries is that they now reach
// back into the icon to their left -- so measure that, rather than assume it.
//
// ST_overlayDrawer's layout, cols=1 so x0=0 and xdiv is the exact fill scale:
//   health digits end at 50,  health icon at 52..68
//   ammo   digits end at 234, ammo   icon at 236..252
//   armor  digits end at 300, armor  icon at 302..318, keys end at 318
// The tightest pair is the ammo icon against the armor digits: 252 to 300 is
// 48 base units for three digits that are 14 wide.
static void check_hud_row( int w, int h, const char * ren )
{
    float xdiv = vid.fdupx_fill;                 // cols == 1
    float art  = (rendermode == render_soft)? (float)vid.dupx : vid.fdupx;
    int   wfv  = (int)((14 * art) + 0.5f);       // one STTNUM digit, as drawn
    int   icon = (int)((16 * art) + 0.5f);       // SBOHEALT and friends
    char  detail[256];

    struct { const char * name; int digits_end, icon_at; } el[3] = {
        { "health", 50,  52  },
        { "ammo",   234, 236 },
        { "armor",  300, 302 },
    };
    int i;

    for( i = 0; i < 3; i++ )
    {
        int dig_left = (int)(el[i].digits_end * xdiv) - (3 * wfv);

        if( i > 0 )
        {
            int prev_icon_right = (int)(el[i - 1].icon_at * xdiv) + icon;
            if( dig_left < prev_icon_right )
            {
                snprintf(detail, sizeof(detail),
                         "%s digits start at %dpx, the %s icon ends at %dpx",
                         el[i].name, dig_left, el[i - 1].name, prev_icon_right);
                fail("digits clear icon", w, h, ren, detail);
            }
        }
        else if( dig_left < 0 )
        {
            snprintf(detail, sizeof(detail),
                     "%s digits start at %dpx, off the left edge",
                     el[i].name, dig_left);
            fail("digits on screen", w, h, ren, detail);
        }
    }

    // The keys column ends at 318 and the armor icon at 318 too, so the row
    // must still fit the screen.
    if( (int)(318 * xdiv) > vid.width )
        fail("row fits the screen", w, h, ren, "the 318 column is off the right edge");
}

struct mode { int w, h; };
static struct mode modes[] = {
    { 320, 200 }, { 400, 300 }, { 512, 384 }, { 640, 400 }, { 640, 480 },
    { 800, 600 }, { 1024, 768 }, { 1280, 720 }, { 1280, 1024 },
    { 1366, 768 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1080 },
    { 2560, 1440 }, { 3440, 1440 }, { 3840, 1080 },
};
#define NMODES  (int)(sizeof(modes)/sizeof(modes[0]))

int main( int argc, char ** argv )
{
    int m, r, i;
    int verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

    for( i = 0; i < 11; i++ )
    {
        tallnum[i].width = 14;  tallnum[i].height = 16;   // STTNUM
        shortnum[i].width = 4;  shortnum[i].height = 6;   // STYSNUM
    }

    for( r = 0; r < 2; r++ )
    {
        const char * ren = r ? "OpenGL" : "software";
        rendermode = r ? render_opengl : render_soft;

        for( m = 0; m < NMODES; m++ )
        {
            int w = modes[m].w, h = modes[m].h;
            set_mode( w, h );

            if( verbose && r == 0 )
                printf("  %4dx%-4d  dupx=%d fdupx=%.4f  dupy=%d fdupy=%.4f\n",
                       w, h, vid.dupx, vid.fdupx, vid.dupy, vid.fdupy);

            if( r == 0 )   // integers, so once per mode is enough
                check_scale_pair( w, h, ren );
            check_digits( w, h, ren, tallnum, "STTNUM" );
            check_digits( w, h, ren, shortnum, "STYSNUM" );
            check_flash( w, h, ren );
            check_center_x( w, h, ren, 133, "PRESS FIRE TO START" );
            check_center_x( w, h, ren, 40, "CHASE CAM" );
            check_center_x( w, h, ren, 300, "a nearly full width caption" );
            check_hud_row( w, h, ren );
            check_press_fire_y( w, h, ren );
            check_center_y( w, h, ren );
        }
    }

    if( failures )
        printf("\n%d failure(s)\n", failures);
    else
        printf("  all checks pass over %d resolutions, both renderers\n", NMODES);
    return failures ? 1 : 0;
}
'''


def build_and_run(subs=None, quiet=False, verbose=False):
    """Compile the harness and run it.  subs replaces text in the extracted
    source, which is how --selfcheck reinstates a bug."""
    pieces = dict(SCALES=b_scales, OVERLAYNUM=f_overlaynum, SCALEX=f_scalex,
                  SCALEY=f_scaley, CENTERX=f_centerx, SCREENY=f_screeny,
                  CENTERY=f_centery)
    if subs:
        for k, (a, b) in subs.items():
            if a not in pieces[k]:
                raise SystemExit("selfcheck: %r not found in %s" % (a[:50], k))
            pieces[k] = pieces[k].replace(a, b)
    src = HARNESS
    for k, v in pieces.items():
        src = src.replace('@' + k + '@', v)

    with tempfile.TemporaryDirectory() as d:
        csrc = os.path.join(d, 'h.c')
        exe = os.path.join(d, 'h')
        open(csrc, 'w').write(src)
        cc = subprocess.run(['gcc', '-w' if quiet else '-Wall', '-O1',
                             '-o', exe, csrc, '-lm'],
                            capture_output=True, text=True)
        if cc.returncode:
            print(cc.stderr)
            raise SystemExit("harness did not compile")
        args = [exe] + (['-v'] if verbose else [])
        run = subprocess.run(args, capture_output=True, text=True)
        return run.returncode, run.stdout


# ---- the bugs each check claims to catch ---------------------------------
# Each entry reinstates exactly the code that shipped, so --selfcheck proves
# the check goes red for the real defect and not for a strawman.
SELFCHECKS = [
    ("digit advance stepped by the whole-number scale (the OpenGL overlap)",
     {'OVERLAYNUM': ('int  wfv = (int)(( wf * art_dupx ) + 0.5f);',
                     'int  wfv = wf * vid.dupx;')}),
    ("pickup flash sized by the whole-number scale",
     {'OVERLAYNUM': ('V_DrawVidFill(x - (wfv*3), y, wfv*3, hfv, FLASH_COLOR);',
                     'V_DrawVidFill(x - (wfv*3), y, wfv*3, hf*vid.dupy, FLASH_COLOR);')}),
    ("strings centred on BASEVIDWIDTH instead of the screen",
     {'CENTERX': ('int   x = (int)(( (float)vid.width - (base_w * sx) ) / (2.0f * sx));',
                  'int   x = (BASEVIDWIDTH - base_w) / 2;')}),
    ("digits so wide they reach back into the icon beside them",
     {'OVERLAYNUM': ('int  wfv = (int)(( wf * art_dupx ) + 0.5f);',
                     'int  wfv = (int)(( wf * art_dupx * 1.6f ) + 0.5f);')}),
    ("rows placed down 200*dupy instead of down the screen",
     {'SCREENY': ('return (int)(( base_y * vid.fdupy ) / HU_Art_ScaleY());',
                  'return base_y;')}),
    ("vertical centring against the layout box instead of the screen",
     {'CENTERY': ('int   y = (int)(( (base_bottom * vid.fdupy) - (base_h * sy) ) / (2.0f * sy));',
                  'int   y = (base_bottom - base_h) / 2;')}),
]


def main():
    selfcheck = '--selfcheck' in sys.argv
    verbose = '-v' in sys.argv

    print("Overlay placement, extracted from st_stuff.c / hu_stuff.c / v_video.c")
    rc, out = build_and_run(verbose=verbose)
    print(out, end='')

    if not selfcheck:
        return rc

    print("\n--selfcheck: reinstating each bug and confirming the check goes red")
    bad = 0
    for name, subs in SELFCHECKS:
        src, out2 = build_and_run(subs=subs, quiet=True)
        if src:
            n = len([l for l in out2.splitlines() if l.strip().startswith('FAIL')])
            print("  caught   %-62s (%d failures)" % (name, n))
        else:
            print("  MISSED   %-62s -- the check cannot fail!" % name)
            bad += 1
    if bad:
        print("\n%d check(s) are not actually testing anything" % bad)
    return rc or (1 if bad else 0)


if __name__ == '__main__':
    sys.exit(main())
