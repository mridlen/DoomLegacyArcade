#!/usr/bin/env python3
"""Measure the chase camera: how often it cannot see the player, and how much
it jitters.

Whether the chase camera is behaving is the kind of thing that gets argued
about from memory of watching the attract screen.  It does not have to be.
The engine's -camlog prints one line per tic giving the camera position, the
player position, and whether anything is between them; this runs two builds
over the same demo and tabulates the difference.

    tools/camstats.py --before <binary> [--after <binary>] [--home DIR]
                      [--waddir DIR] [demo ...]

What the numbers mean:

  tics with no sight of player      The camera is looking at a wall with the
                                    player behind it.  The complaint.
  longest unbroken blind stretch    Worse than the average: one five-second
                                    stretch is far more noticeable on an
                                    attract screen than the same tics spread
                                    thinly.  With the half-second recovery in
                                    place this should sit near TICRATE/2.
  direction reversals while moving  The jitter.  A camera easing toward a spot
                                    it can reach moves smoothly, so consecutive
                                    per-tic steps point much the same way; a
                                    camera being shoved into a wall and
                                    recoiling reverses over and over.  Counted
                                    only while actually moving, so a camera
                                    sitting still does not register as calm.
  visible reversals                 The same, but only when both steps exceed
                                    2 map units.  The bare count includes
                                    sub-pixel wobble nobody can see; this is
                                    the one to read.
  target pulled in                  How often a wall forced the follow distance
                                    shorter than cv_cam_dist.  Context, not a
                                    score: on an indoor map it is normal for
                                    this to be most of the run.

Both runs need a demo the camera actually follows, so the scratch config forces
chasecam on, and framerate_cap 35 / render_threads 1 for the same reasons
tools/chasecam-test.py pins them.
"""
import argparse, io, math, os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
FRAC = 65536.0
NOISE = 0.5        # map units; below this a step is not really movement
VISIBLE = 2.0      # map units; a reversal this big is one you can see

LINE = re.compile(r'CAM t=(\d+) x=(-?\d+) y=(-?\d+) z=(-?\d+) '
                  r'px=(-?\d+) py=(-?\d+) blocked=(\d) pulled=(\d)')


def demo_args(base, leveldir):
    """Engine args for a demo filename, or None -- mirrors demotest.sh."""
    base = base[:-4] if base.endswith('.lmp') else base
    ident = base.split('_')[0]
    if ident.endswith('-sl'):
        ident = ident[:-3]
    game, _, pack = ident.partition('+')
    if game not in ('doom', 'doomu', 'doom2', 'tnt', 'plutonia', 'heretic', 'chex'):
        return None
    if pack:
        wad = os.path.join(leveldir, pack + '.wad')
        if not os.path.isfile(wad):
            return None
        return ['-game', game, '-file', wad]
    return ['-game', game]


def prepare(slot, binary, home, waddir):
    shutil.copy2(binary, os.path.join(slot, 'doomlegacyarcade'))
    os.chmod(os.path.join(slot, 'doomlegacyarcade'), 0o755)
    shutil.copytree(home, os.path.join(slot, 'legacyhome'))
    for nm in os.listdir(waddir):
        if nm.lower().endswith(('.wad', '.pk3')):
            dst = os.path.join(slot, nm)
            if not os.path.exists(dst):
                os.symlink(os.path.join(waddir, nm), dst)
    hm = os.path.join(slot, 'legacyhome')
    for f in ('config8p.cfg', 'configgl.cfg', 'confign.cfg', 'autoexec.cfg'):
        q = os.path.join(hm, f)
        if os.path.exists(q):
            os.remove(q)
    cfg = os.path.join(hm, 'config.cfg')
    s = io.open(cfg, encoding='utf-8', errors='replace').read()
    for key, val in (('drawmode', '"Software 8bit"'),
                     ('framerate_cap', '"35"'),
                     ('render_threads', '"1"'),
                     ('chasecam', '"1"')):     # the whole point
        pat = r'(?m)^%s .*$' % key
        s = (re.sub(pat, '%s %s' % (key, val), s) if re.search(pat, s)
             else s + '\n%s %s\n' % (key, val))
    io.open(cfg, 'w', encoding='utf-8').write(s)
    return hm


def run(slot, args, demo, timeout):
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
               SDL_NO_SIGNAL_HANDLERS='1')
    r = subprocess.run(['./doomlegacyarcade'] + args +
                       ['-timedemo', os.path.join('legacyhome', 'demos',
                                                  demo + '.lmp'), '-camlog'],
                       cwd=slot, env=env, capture_output=True, text=True,
                       errors='replace', timeout=timeout)
    out = re.sub(r'\x1b\[[0-9;]*m', '', r.stdout + r.stderr)
    return [tuple(int(g) for g in m.groups()) for m in LINE.finditer(out)]


def stats(rows):
    if not rows:
        return None
    n = len(rows)
    dists = [math.hypot((r[1] - r[4]) / FRAC, (r[2] - r[5]) / FRAC)
             for r in rows]
    steps = [((rows[i][1] - rows[i - 1][1]) / FRAC,
              (rows[i][2] - rows[i - 1][2]) / FRAC) for i in range(1, n)]
    rev = moving = big = 0
    for i in range(1, len(steps)):
        ax, ay = steps[i - 1]
        bx, by = steps[i]
        la, lb = math.hypot(ax, ay), math.hypot(bx, by)
        if la < NOISE or lb < NOISE:
            continue
        moving += 1
        if (ax * bx + ay * by) < 0:
            rev += 1
            if la > VISIBLE and lb > VISIBLE:
                big += 1
    worst = cur = 0
    for r in rows:
        cur = cur + 1 if r[6] else 0
        worst = max(worst, cur)
    ds = sorted(dists)
    return dict(tics=n, blind=100.0 * sum(r[6] for r in rows) / n,
                worst=worst, pulled=100.0 * sum(r[7] for r in rows) / n,
                median=ds[n // 2], p95=ds[int(n * 0.95)], mx=ds[-1],
                rev=rev, big=big,
                rev_pct=(100.0 * rev / moving) if moving else 0.0)


ROWS = [('tics with no sight of player', 'blind', '%.1f%%'),
        ('longest unbroken blind stretch', 'worst', '%d tics'),
        ('direction reversals while moving', 'rev_pct', '%.1f%%'),
        ('  (count)', 'rev', '%d'),
        ('visible reversals (>2 units)', 'big', '%d'),
        ('camera-to-player median', 'median', '%.0f'),
        ('camera-to-player p95', 'p95', '%.0f'),
        ('camera-to-player max', 'mx', '%.0f'),
        ('tics with target pulled in', 'pulled', '%.1f%%')]


def main():
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument('--before', required=True)
    ap.add_argument('--after',
                    default=os.path.join(REPO, 'svn1749', 'bin',
                                         'doomlegacyarcade'))
    ap.add_argument('--home')
    ap.add_argument('--waddir',
                    default=os.environ.get('DOOMWADDIR',
                                           os.path.expanduser('~/games/doom')))
    ap.add_argument('--timeout', type=int, default=1200)
    ap.add_argument('-k', '--keep', action='store_true')
    ap.add_argument('-h', '--help', action='store_true')
    ap.add_argument('demos', nargs='*',
                    default=['doomu-sl_E1M2_sk0_tyson', 'doomu_E4M1_sk0_speed'])
    a = ap.parse_args()
    if a.help:
        print(__doc__)
        return 0

    before, after = os.path.abspath(a.before), os.path.abspath(a.after)
    for b in (before, after):
        if not os.path.isfile(b):
            print('camstats: no binary at %s' % b)
            return 2
    home = os.path.abspath(a.home) if a.home else os.path.join(
        os.path.dirname(after), 'legacyhome')
    if not os.path.isdir(os.path.join(home, 'demos')):
        print('camstats: no demos at %s' % os.path.join(home, 'demos'))
        return 2

    root = tempfile.mkdtemp(prefix='doomlegacy-camstats.')
    try:
        slots = {}
        for tag, b in (('before', before), ('after', after)):
            d = os.path.join(root, tag)
            os.mkdir(d)
            slots[tag] = (d, prepare(d, b, home, a.waddir))
        leveldir = os.path.join(slots['after'][1], 'levels')
        rc = 0
        for demo in a.demos:
            args = demo_args(demo, leveldir)
            if args is None:
                print('camstats: %s -- game id not recognised\n' % demo)
                rc = 2
                continue
            got = {}
            for tag in ('before', 'after'):
                got[tag] = stats(run(slots[tag][0], args, demo, a.timeout))
            if not got['before'] or not got['after']:
                # Two runs that produced no log compare equal; never let that
                # read as "no change".
                print('camstats: %s -- no -camlog output (is the camera on, '
                      'and does this build have -camlog?)\n' % demo)
                rc = 2
                continue
            print('demo: %s   (%d tics)\n' % (demo, got['before']['tics']))
            print('%-38s %10s %10s' % ('', 'before', 'after'))
            for label, key, fmt in ROWS:
                print('%-38s %10s %10s'
                      % (label, fmt % got['before'][key], fmt % got['after'][key]))
            print()
        return rc
    finally:
        if a.keep:
            print('kept: %s' % root)
        else:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
