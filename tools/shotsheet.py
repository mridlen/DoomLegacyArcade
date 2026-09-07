#!/usr/bin/env python3
"""Screenshot the game at a list of drawing sizes and build one web page of them.

Point of it: checking that a screen *looks right* is the one thing a headless
run cannot do and a person can do in seconds.  This runs the game once per
drawing size, takes a screenshot, and writes a single self-contained HTML file
with every shot on it, at its true pixel shape, labelled with what the engine
actually did.  Open it, scroll, done -- no need to describe anything to anyone.

    tools/shotsheet.py                       # the default ladder, in-game
    tools/shotsheet.py --sizes 3440x1440,1280x360
    tools/shotsheet.py --aspect 21:9,32:9    # only those shapes
    tools/shotsheet.py --scenes game,title   # attract screen too
    tools/shotsheet.py --out /tmp/before     # then again into /tmp/after
    tools/shotsheet.py --resume              # continue a run that was killed

Two runs into two directories is the way to check a change: open both pages
side by side.

How it works, and the traps it already goes round (all from CLAUDE.md and
docs/arcade/):

  * SDL_VIDEODRIVER=offscreen, never dummy -- under dummy the capture comes out
    entirely black even for screens that obviously have content.
  * SDL_NO_SIGNAL_HANDLERS=1, or SDL turns a stray signal into SDL_QUIT and the
    run ends early with no error at all.
  * fullscreen "Yes" -- the offscreen driver has no window manager, and a
    windowed request comes back with no visual and dies with "cannot draw 0
    bits per pixel", which reads as a colour depth problem and is not one.
  * A COPY of legacyhome per run, never the live one next to the binary: that
    one holds the cabinet's real config, high scores and record demos.
  * config8p/configgl/confign.cfg deleted -- they execute after config.cfg and
    would put the drawmode back.
  * localplayers "1", or the cabinet's own config gives you a 2x2 grid.
  * -width/-height, which are exact for the software renderer (i_video.c), so a
    21:9 drawing size can be previewed on a 16:9 monitor.
  * framerate_cap "35".  r_fps.c sets interp_active from
    (cv_framerate_cap.value != TICRATE), so at the tic rate the frame is drawn
    at the tic instead of a wall-clock fraction past it.

REPRODUCIBILITY, which matters if you want to compare two sheets rather than
only look at them: a shot of a live level is NOT reproducible run to run.  The
cap above removes the sub-tic interpolation, but monsters are moving and the
shot can land a tic either side, so the same binary twice gives two different
pictures.  Pass --nomonsters and it is bit identical run to run (verified: two
runs, cmp clean).  Then two sheets can be compared mechanically, which narrows
"look at everything" down to "look at what moved":

    for f in before/*.png; do cmp -s $f after/$(basename $f) \
      && echo "same    $(basename $f)" || echo "CHANGED $(basename $f)"; done

Without --nomonsters that comparison is pure noise, and the noise looks like a
regression: the first before/after pair taken here reported four sizes as
changed that the change under test could not touch, 1024x768 among them, which
is 4:3 and provably unaffected by an aspect cap.

Even WITH it, a comparison across two different builds is a lead and not a
proof.  Two runs of one binary are identical, but two binaries start up at
slightly different speeds, the shot lands a tic either side, and a level's
animated textures advance per tic -- so a size can come out CHANGED with no
code path between the two builds that could have done it.  When a diff says a
resolution changed that arithmetic says cannot have, believe the arithmetic and
look at the pictures.

The screenshots are the engine's own drawing buffer, which on a real fullscreen
display is then stretched to fill the monitor.  So a shot's own shape is the
shape the monitor must be for it to look right -- which is exactly what makes
the wrong ones obvious on the page.
"""
import argparse, base64, os, re, shutil, struct, subprocess, sys, tempfile, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, os.pardir))
BIN = os.path.join(ROOT, 'svn1749', 'bin', 'doomlegacyarcade')
SCREEN_H = os.path.join(ROOT, 'svn1749', 'src', 'screen.h')

# Drawing sizes worth looking at, by shape.  Sizes above the engine's own caps
# are dropped automatically, so this list does not have to be kept in step with
# them.
LADDER = [
    ('4:3',   [(320, 200), (640, 480), (800, 600), (1024, 768), (1600, 1200)]),
    ('16:10', [(1280, 800), (1680, 1050)]),
    ('16:9',  [(1280, 720), (1366, 768), (1920, 1080), (2560, 1440)]),
    ('21:9',  [(1280, 540), (2560, 1080), (3440, 1440)]),
    ('32:9',  [(1280, 360), (2560, 720), (3840, 1080), (5120, 1440)]),
]


def engine_caps():
    """MAXVIDWIDTH/MAXVIDHEIGHT, read from screen.h rather than restated."""
    s = open(SCREEN_H, encoding='latin-1').read()
    w = int(re.search(r'^#define\s+MAXVIDWIDTH\s+(\d+)', s, re.M).group(1))
    h = int(re.search(r'^#define\s+MAXVIDHEIGHT\s+(\d+)', s, re.M).group(1))
    return w, h


# ---- TGA in, PNG out.  No image library: the engine writes uncompressed 24bpp
# TGA and zlib is in the standard library, so there is nothing to install.

def read_tga(path):
    d = open(path, 'rb').read()
    idlen, imgtype, bpp, desc = d[0], d[2], d[16], d[17]
    w, h = struct.unpack_from('<HH', d, 12)
    if imgtype != 2 or bpp != 24:
        raise ValueError('%s: not an uncompressed 24bpp TGA (type %d, %d bpp)'
                         % (path, imgtype, bpp))
    off = 18 + idlen
    rows = [d[off + y * w * 3: off + (y + 1) * w * 3] for y in range(h)]
    if not (desc & 0x20):
        rows.reverse()              # TGA origin is bottom left by default
    return w, h, rows


def png_bytes(w, h, rows, shrink=1):
    """BGR rows to PNG.  shrink takes every Nth pixel and row -- nearest
    neighbour on purpose, so the page shows the real pixels rather than a
    smoothed guess at them."""
    ow = len(range(0, w, shrink))
    raw = bytearray()
    for y in range(0, h, shrink):
        r = rows[y]
        raw.append(0)
        for x in range(0, w, shrink):
            i = x * 3
            raw += bytes((r[i + 2], r[i + 1], r[i]))
    oh = len(range(0, h, shrink))

    def chunk(t, data):
        c = t + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c))
    return (b'\x89PNG\r\n\x1a\n'
            + chunk(b'IHDR', struct.pack('>IIBBBBB', ow, oh, 8, 2, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(bytes(raw), 6))
            + chunk(b'IEND', b'')), ow, oh


# ---- one run ---------------------------------------------------------------

STRIP_ANSI = re.compile(r'\x1b\[[0-9;]*m')


def run_one(binary, w, h, scene, game, warp, wait, timeout, keep_dir,
            extra_cvars=(), nomonsters=False):
    """Run the engine once and return (tga_path or None, info dict)."""
    rd = tempfile.mkdtemp(prefix='shotsheet.')
    info = {}
    try:
        shutil.copy2(binary, rd)
        home_src = os.path.join(os.path.dirname(binary), 'legacyhome')
        home = os.path.join(rd, 'legacyhome')
        shutil.copytree(home_src, home)
        for stale in ('config8p.cfg', 'configgl.cfg', 'confign.cfg',
                      'autoexec.cfg'):
            p = os.path.join(home, stale)
            if os.path.exists(p):
                os.remove(p)

        waddir = os.environ.get('DOOMWADDIR',
                                os.path.expanduser('~/games/doom'))
        for f in os.listdir(waddir):
            try:
                os.symlink(os.path.join(waddir, f), os.path.join(rd, f))
            except OSError:
                pass

        cfg = os.path.join(home, 'config.cfg')
        text = open(cfg, encoding='latin-1').read()
        settings = [('drawmode', '"Software 8bit"'),
                    ('fullscreen', '"Yes"'),
                    ('viewfit', '"AUTO"'),
                    ('localplayers', '"1"'),
                    # Without this the shots are NOT reproducible, and the way
                    # they fail is quiet: r_fps.c interpolates the drawn frame
                    # between tics by a fraction taken from the wall clock, so
                    # the same binary on a busy machine draws the world a
                    # fraction of a tic further along.  Two sheets generated
                    # while the machine was doing different things then differ
                    # at sizes the change under test never touched, and that
                    # reads as a regression.  Capped at the tic rate the
                    # fraction is fixed and the image is bit identical run to
                    # run -- the same trick render-threads.md uses to compare
                    # threaded frames against serial ones.
                    ('framerate_cap', '"35"')]
        settings += [(k, '"%s"' % v) for k, v in extra_cvars]
        for key, val in settings:
            text, n = re.subn(r'(?m)^%s .*$' % key, '%s %s' % (key, val), text)
            if not n:
                text += '\n%s %s\n' % (key, val)
        open(cfg, 'w', encoding='latin-1').write(text)

        # The screenshot has to happen once the level is up, and a command that
        # starts a game only takes effect after the command buffer drains --
        # hence -warp on the command line and only the shot from here.
        open(os.path.join(home, 'autoexec.cfg'), 'w').write(
            'wait %d\nscreenshot\n' % wait)

        argv = ['./doomlegacyarcade', '-game', game,
                '-width', str(w), '-height', str(h)]
        if nomonsters:
            argv += ['-nomonsters']
        if scene == 'game':
            argv += ['-skill', '3', '-warp', str(warp)]
        env = dict(os.environ,
                   DISPLAY='',
                   SDL_VIDEODRIVER='offscreen',
                   SDL_AUDIODRIVER='dummy',
                   SDL_NO_SIGNAL_HANDLERS='1')
        try:
            p = subprocess.run(argv, cwd=rd, env=env, timeout=timeout,
                               capture_output=True)
            log = p.stdout.decode('latin-1', 'replace')
        except subprocess.TimeoutExpired as e:
            log = (e.stdout or b'').decode('latin-1', 'replace')
        log = STRIP_ANSI.sub('', log)

        m = re.search(r'Draw (\d+)x(\d+), (\d+) bpp', log)
        if m:
            info['drew'] = (int(m.group(1)), int(m.group(2)))
        err = re.search(r'^.*Error.*$', log, re.M)
        if err:
            info['error'] = err.group(0).strip()[:160]

        shots = sorted(f for f in os.listdir(rd) if f.lower().endswith('.tga'))
        if not shots:
            info['error'] = info.get('error', 'no screenshot was written')
            return None, info
        # Move it out before the scratch directory goes away.
        out = tempfile.mktemp(suffix='.tga')
        shutil.move(os.path.join(rd, shots[0]), out)
        return out, info
    finally:
        if keep_dir:
            print('  kept %s' % rd)
        else:
            shutil.rmtree(rd, ignore_errors=True)


# ---- the page --------------------------------------------------------------

PAGE_HEAD = """<!doctype html>
<meta charset="utf-8"><title>%(title)s</title>
<style>
 :root { color-scheme: light dark;
   --bg:#f7f6f4; --fg:#1a1a1a; --dim:#5c5c5c; --card:#fff; --line:#dcd8d2;
   --bad:#b3261e; }
 @media (prefers-color-scheme: dark) { :root {
   --bg:#141414; --fg:#ececec; --dim:#9a9a9a; --card:#1e1e1e; --line:#333;
   --bad:#f2b8b5; } }
 body { margin:0; background:var(--bg); color:var(--fg);
   font:14px/1.5 ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,sans-serif; }
 header { padding:24px 24px 8px; }
 h1 { font-size:20px; margin:0 0 4px; }
 .meta { color:var(--dim); font-size:13px; }
 h2 { font-size:15px; margin:28px 24px 8px; padding-top:12px;
   border-top:1px solid var(--line); color:var(--dim);
   text-transform:uppercase; letter-spacing:.08em; }
 .grid { display:flex; flex-direction:column; gap:16px; padding:0 24px 24px; }
 figure { margin:0; background:var(--card); border:1px solid var(--line);
   border-radius:8px; padding:12px; }
 figcaption { display:flex; flex-wrap:wrap; gap:12px; align-items:baseline;
   margin-bottom:8px; }
 .size { font-weight:600; font-variant-numeric:tabular-nums; }
 .note { color:var(--dim); font-size:12.5px; }
 .err { color:var(--bad); font-size:12.5px; }
 img { display:block; max-width:100%%; height:auto; image-rendering:pixelated;
   border:1px solid var(--line); border-radius:4px; background:#000; }
 .missing { padding:24px; color:var(--bad); font-size:13px; }
</style>
<header>
<h1>%(title)s</h1>
<div class="meta">%(meta)s</div>
</header>
"""


def build_page(path, title, meta, groups, embed=True):
    out = [PAGE_HEAD % dict(title=title, meta=meta)]
    for gname, items in groups:
        if not items:
            continue
        out.append('<h2>%s</h2>\n<div class="grid">\n' % gname)
        for it in items:
            out.append('<figure><figcaption>')
            out.append('<span class="size">%dx%d</span>' % (it['w'], it['h']))
            out.append('<span class="note">%.2f:1%s</span>'
                       % (it['w'] / it['h'],
                          '' if not it.get('scene') else ' &middot; ' + it['scene']))
            if it.get('note'):
                out.append('<span class="note">%s</span>' % it['note'])
            if it.get('error'):
                out.append('<span class="err">%s</span>' % it['error'])
            out.append('</figcaption>')
            if it.get('file'):
                if embed:
                    # One image in memory at a time.  Holding all of them was
                    # enough, on a 7GB laptop with a browser open, for the
                    # whole run to be killed for memory before it wrote a
                    # single shot.
                    with open(it['file'], 'rb') as fh:
                        b64 = base64.b64encode(fh.read()).decode('ascii')
                    out.append('<img alt="%dx%d" src="data:image/png;base64,%s">'
                               % (it['w'], it['h'], b64))
                    del b64
                else:
                    out.append('<img alt="%dx%d" src="%s">'
                               % (it['w'], it['h'], os.path.basename(it['file'])))
            else:
                out.append('<div class="missing">no screenshot</div>')
            out.append('</figure>\n')
        out.append('</div>\n')
    open(path, 'w', encoding='utf-8').write(''.join(out))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', default=BIN)
    ap.add_argument('--sizes', help='comma separated WxH, instead of the ladder')
    ap.add_argument('--aspect', help='comma separated shape names from the '
                                     'ladder, e.g. "21:9,32:9"')
    ap.add_argument('--scenes', default='game',
                    help='game (in a level) and/or title (attract screen)')
    ap.add_argument('--game', default='doom2')
    ap.add_argument('--warp', type=int, default=1)
    ap.add_argument('--wait', type=int, default=105,
                    help='tics to wait before the shot (35 = 1 second)')
    ap.add_argument('--timeout', type=int, default=45)
    ap.add_argument('--shrink', type=int, default=0,
                    help='take every Nth pixel for the page (0 = automatic, '
                         'so nothing on the page is wider than about 1600)')
    ap.add_argument('--out', default=os.path.join(ROOT, 'shotsheet'))
    ap.add_argument('--keep', action='store_true', help='keep the scratch dirs')
    ap.add_argument('--resume', action='store_true',
                    help='keep shots already in --out instead of retaking them, '
                         'so a run killed part way can be continued')
    ap.add_argument('--nomonsters', action='store_true',
                    help='pass -nomonsters, for a scene that holds still')
    ap.add_argument('--cvar', action='append', default=[], metavar='NAME=VALUE',
                    help='set a cvar in the scratch config, repeatable -- e.g. '
                         '--cvar localplayers=4 --cvar split4="4 Columns". '
                         'Applied after the defaults, so it can override them.')
    ap.add_argument('--title', default=None)
    ap.add_argument('--no-embed', action='store_true',
                    help='reference the PNG files beside the page instead of '
                         'embedding them, for a much smaller HTML file that is '
                         'no longer self-contained')
    args = ap.parse_args()

    if not os.path.exists(args.binary):
        sys.exit('no binary at %s -- build first' % args.binary)
    maxw, maxh = engine_caps()

    if args.sizes:
        groups = [('requested',
                   [tuple(int(v) for v in s.lower().split('x'))
                    for s in args.sizes.split(',')])]
    else:
        want = set(a.strip() for a in args.aspect.split(',')) if args.aspect else None
        groups = [(n, sz) for n, sz in LADDER if want is None or n in want]
        if not groups:
            sys.exit('no shapes matched --aspect; ladder has: %s'
                     % ', '.join(n for n, _ in LADDER))

    scenes = [s.strip() for s in args.scenes.split(',') if s.strip()]
    extra = []
    for c in args.cvar:
        if '=' not in c:
            sys.exit('--cvar wants NAME=VALUE, got %r' % c)
        k, v = c.split('=', 1)
        extra.append((k.strip(), v))
    os.makedirs(args.out, exist_ok=True)

    total = sum(len(sz) for _, sz in groups) * len(scenes)
    print('%d run(s) into %s' % (total, args.out))
    done, out_groups = 0, []
    for gname, sizes in groups:
        items = []
        for (w, h) in sizes:
            if w > maxw or h > maxh:
                print('  skip %dx%d -- over the engine cap of %dx%d'
                      % (w, h, maxw, maxh))
                continue
            for scene in scenes:
                done += 1
                name = '%dx%d-%s.png' % (w, h, scene)
                path = os.path.join(args.out, name)
                # [Arcade] --resume: a shot already on disk is kept and the run
                # skipped.  This machine's low-memory watchdog kills a long
                # sheet part way through, and without this every kill threw away
                # everything it had done.
                if args.resume and os.path.exists(path):
                    print('  [%d/%d] %dx%d %s ... kept' % (done, total, w, h, scene))
                    items.append({'w': w, 'h': h,
                                  'scene': scene if len(scenes) > 1 else None,
                                  'file': path})
                    continue
                print('  [%d/%d] %dx%d %s ... ' % (done, total, w, h, scene),
                      end='', flush=True)
                tga, info = run_one(args.binary, w, h, scene, args.game,
                                    args.warp, args.wait, args.timeout, args.keep,
                                    extra, args.nomonsters)
                it = {'w': w, 'h': h,
                      'scene': scene if len(scenes) > 1 else None}
                if info.get('error'):
                    it['error'] = info['error']
                drew = info.get('drew')
                if drew and drew != (w, h):
                    it['note'] = ('engine drew %dx%d instead' % drew)
                if tga:
                    tw, th, rows = read_tga(tga)
                    shrink = args.shrink or max(1, -(-tw // 1600))
                    data, ow, oh = png_bytes(tw, th, rows, shrink)
                    open(path, 'wb').write(data)
                    it['file'] = path
                    del data, rows
                    if shrink > 1:
                        it['note'] = ((it.get('note', '') + ' ') if it.get('note') else '') \
                                     + 'shown at 1/%d' % shrink
                    os.remove(tga)
                    print('%dx%d ok' % (tw, th))
                else:
                    print('FAILED -- %s' % it.get('error', 'unknown'))
                items.append(it)
        out_groups.append((gname, items))

    title = args.title or 'DoomLegacy Arcade -- drawing sizes'
    rev = subprocess.run(['git', '-C', ROOT, 'describe', '--always', '--dirty'],
                         capture_output=True, text=True).stdout.strip()
    meta = ('%d screenshots &middot; %s &middot; engine cap %dx%d &middot; '
            'software renderer, %s map %s'
            % (sum(1 for _, i in out_groups for x in i if x.get('file')),
               rev or 'unknown build', maxw, maxh, args.game, args.warp))
    index = os.path.join(args.out, 'index.html')
    build_page(index, title, meta, out_groups, embed=not args.no_embed)
    print('\nwrote %s' % index)
    print('open it with:  xdg-open %s' % index)


if __name__ == '__main__':
    main()
