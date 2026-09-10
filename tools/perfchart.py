#!/usr/bin/env python3
"""Measure the frame rate at a list of resolutions and write the table.

The README's Performance table, produced by a script instead of by hand: the
game plays a fixed record demo once per drawing size with -timedemo, and the
engine's own result line gives the frame rate.  Same demo, same settings, every
size, so two runs of this -- before and after a change, or on two machines --
can be compared line for line.

    tools/perfchart.py                          # every size in the README table
    tools/perfchart.py --quick                  # 512x384 640x480 800x600 1024x576
    tools/perfchart.py --sizes 640x350,800x600  # just these
    tools/perfchart.py --runs 3                 # each size 3 times: "61-64"
    tools/perfchart.py --compare old.csv        # add a before column
    tools/perfchart.py --set render_threads=1   # any cvar, for this run only
    tools/perfchart.py --headless               # no window (offscreen driver)

It writes perfchart-<host>-<date>.md (paste-ready for the README) and a .csv of
the same numbers, which is what --compare reads back.

What it measures.  -timedemo draws one frame per game tic as fast as it can and
reports frames per second of wall clock; the engine switches vsync off for it,
so a 60Hz panel does not cap the result.  That is the drawing speed of the
machine, which is what Framerate Cap "Uncapped" shows in play too -- within a
few percent, because in play the simulation runs 35 tics a second whatever the
frame rate, where a timedemo runs one per frame.

The demo is tools/bench/doomu_E1M1_sk3_speed.lmp, a copy of the cabinet's UV
speed record for E1M1, kept in the repo on purpose: the cabinet rewrites its
record demos whenever a record is beaten, and a benchmark whose workload
changes under it cannot be compared with anything.  Any other demo can be given
with --demo; its game is taken from the cabinet's naming (doomu_..., doom2_...)
unless --game says otherwise.

Checks it makes, because a benchmark that quietly measured the wrong thing is
worse than none:

  * The first size is run once and thrown away before the chart starts: the
    first run of a chart starts cold and read up to a quarter slow on the Pi.
  * The header lists the settings that change the numbers -- the ones the
    chart forces, and the ones it takes from the config (Row Padding, view
    size, frame cap), so two charts can always be told apart.  And the size
    the frames are scaled to, which is the desktop in software fullscreen: on
    a Pi 3, 1280x720 instead of 1920x1080 was worth 15-45% at every size.
  * The engine's result line says what it drew ("640x480 8bpp software").  A
    size the engine did not actually use is reported, not credited.
  * Every size must play the same number of game tics.  Drawing cannot change
    the simulation, so a different count means that run went wrong -- the demo
    did not load, or it quit early.
  * The time must add up.  With -frameprofile the engine reports, for exactly
    the frames it counted, how long went on Views (the 3D view), Present (onto
    the screen) and Other (game logic, HUD); those are columns in the table, and
    their sum must be 1000/fps.  If it is not, wall time went somewhere no frame
    was drawn, and the size is flagged.  That check found the screen wipe.
  * On a Raspberry Pi it records the temperature after each run and reads
    `vcgencmd get_throttled` at the end.  A Pi that got hot and slowed itself
    down produces numbers that look like the game is slow; the chart says so.

Traps it goes round (CLAUDE.md, Headless verification):

  * A COPY of legacyhome, never the live one next to the binary, which holds
    the cabinet's real config, scores and demos.  Copied once per chart, not
    once per size: the level packs are tens of MB and a Pi's SD card is slow.
  * The wads are linked in from where the engine would find them, the
    binary's own directory FIRST.  The scratch copy is a different directory,
    so a legacy.wad kept next to the binary -- which is how the Pi is set up --
    is otherwise left behind, and the engine stops with "No legacy.wad file".
    Then DOOMWADDIR and the rest of the list tools/smoke.sh searches.
  * screenlink "None".  The level load restarts the timedemo clock and the
    wipe that follows runs on its own clock, so with a wipe about a second is
    counted in the fps figure and in no frame -- half the result on a fast
    machine.  The first Pi chart had it in every row.
  * config8p/configgl/confign.cfg deleted -- they run after config.cfg and
    would put the drawmode back.  autoexec.cfg deleted.
  * SDL_NO_SIGNAL_HANDLERS=1, or a stray signal becomes SDL_QUIT and the run
    ends early with no error at all.
  * --headless uses SDL_VIDEODRIVER=offscreen, never dummy: under dummy the
    player view is never drawn, which would make every size look fast.
  * fullscreen "Yes" -- required by the offscreen driver, and how a cabinet
    runs anyway.  -width/-height are exact for the software renderer.
"""
import argparse, csv, datetime, os, re, shutil, socket, statistics, subprocess
import sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, os.pardir))
BIN = os.path.join(ROOT, 'svn1749', 'bin', 'doomlegacyarcade')
SCREEN_H = os.path.join(ROOT, 'svn1749', 'src', 'screen.h')
DEMO = os.path.join(HERE, 'bench', 'doomu_E1M1_sk3_speed.lmp')

# The README's Performance table, smallest first.
SIZES = [(320, 200), (320, 240), (400, 300), (512, 384), (640, 350),
         (640, 360), (640, 400), (720, 400), (640, 480), (720, 480),
         (768, 480), (800, 500), (864, 486), (800, 600), (960, 540),
         (928, 580), (960, 600), (1024, 576), (1024, 768), (1152, 720),
         (1280, 720), (1152, 864), (1280, 800), (1280, 960)]
# A fixed handful for before/after checks (render-threads.md).
QUICK = [(512, 384), (640, 480), (800, 600), (1024, 576)]

# What the README table was measured under.  --set overrides any of these.
SETTINGS = [('drawmode', 'Software 8bit'),
            ('draw8bpp', 'On'),
            ('render_threads', 'Auto'),
            ('fullscreen', 'Yes'),
            ('localplayers', '1'),
            # No wipe.  The level load restarts the timedemo clock, and the
            # crossfade or melt that follows runs on its own clock inside that
            # same pass -- about a second counted in the fps figure and in no
            # frame.  The first Pi chart had it in every row: roughly 20% off
            # at 320x200, a few percent at 1280x960.  See the "adds up" check.
            ('screenlink', 'None')]

# Settings the chart does not force but that change the numbers.  They come
# from the cabinet's own config, so the header has to say what they were --
# two charts of one build, one with Row Padding and one without, looked
# identical at the top until this was added.
REPORTED = ('row_padding', 'viewsize', 'framerate_cap')
# Their compiled defaults, for a config that has no line for one yet.
REPORTED_DEFAULT = {'row_padding': 'Off', 'viewsize': '10', 'framerate_cap': '60'}

GAMES = ('doom1', 'doomu', 'doom2', 'tnt', 'plutonia', 'heretic', 'hexen',
         'freedoom1', 'freedoom2', 'freedm', 'chex')

STRIP_ANSI = re.compile(r'\x1b\[[0-9;]*m')
RESULT = re.compile(r'timedemo: (\d+) gametics in (\d+) realtics, ([0-9.]+) avg fps'
                    r'(?:, (\d+)x(\d+) (\d+)bpp (\w+))?')
PROFILE = re.compile(r'timedemo profile: (\d+) frames, ms per frame: tics ([0-9.]+) '
                     r'views ([0-9.]+) present ([0-9.]+) hud/other ([0-9.]+) total ([0-9.]+)'
                     r'(?: expand ([0-9.]+))?')
RENDER = re.compile(r'^Render: (.*)$', re.M)
# The size SDL scales each frame to -- the desktop, for software fullscreen.
# The last one wins: the renderer is made again at the mode set.
OUTPUT = re.compile(r'^SDL renderer: .*, output (\d+)x(\d+)\s*$', re.M)
VERSION = re.compile(r'Doom Legacy Arcade (v\S+)')

SHAPES = [('4:3', 4 / 3), ('16:10', 16 / 10), ('16:9', 16 / 9), ('3:2', 3 / 2),
          ('5:4', 5 / 4), ('21:9', 64 / 27), ('32:9', 32 / 9)]


def shape(w, h):
    """'4:3' when exact, '~16:9' when within 5%, else the ratio."""
    r = w / h
    name, val = min(SHAPES, key=lambda s: abs(s[1] - r))
    if abs(val - r) < 0.005:
        return name
    if abs(val - r) / val < 0.05:
        return '~' + name
    return '%.2f:1' % r


def engine_caps():
    """MAXVIDWIDTH/MAXVIDHEIGHT from screen.h, or None when the source is not
    there (an install without the tree)."""
    try:
        s = open(SCREEN_H, encoding='latin-1').read()
        return (int(re.search(r'^#define\s+MAXVIDWIDTH\s+(\d+)', s, re.M).group(1)),
                int(re.search(r'^#define\s+MAXVIDHEIGHT\s+(\d+)', s, re.M).group(1)))
    except (OSError, AttributeError):
        return None


def cpu_name():
    try:
        txt = open('/proc/cpuinfo').read()
    except OSError:
        return ''
    # A Pi says what board it is on the "Model" line; x86 has "model name".
    for key in ('Model', 'model name'):
        m = re.search(r'^%s\s*:\s*(.+)$' % key, txt, re.M)
        if m:
            return m.group(1).strip()
    return ''


def vcgencmd(*args):
    """A Raspberry Pi's firmware query tool, or None anywhere else."""
    if not shutil.which('vcgencmd'):
        return None
    try:
        return subprocess.run(['vcgencmd'] + list(args), capture_output=True,
                              text=True, timeout=5).stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        return None


def pi_temp():
    out = vcgencmd('measure_temp')        # "temp=61.2'C"
    m = re.search(r'([0-9.]+)', out or '')
    return float(m.group(1)) if m else None


# get_throttled bits: current state in the low bits, "has happened since boot"
# in bits 16 and up.
THROTTLE_BITS = [(0x1, 'under-voltage now'), (0x2, 'CPU speed capped now'),
                 (0x4, 'throttled now'), (0x8, 'soft temperature limit now'),
                 (0x10000, 'under-voltage has occurred'),
                 (0x20000, 'CPU speed capping has occurred'),
                 (0x40000, 'throttling has occurred'),
                 (0x80000, 'soft temperature limit has occurred')]


def pi_throttled():
    out = vcgencmd('get_throttled')       # "throttled=0x50000"
    m = re.search(r'0x([0-9a-fA-F]+)', out or '')
    return int(m.group(1), 16) if m else None


def describe_throttle(v):
    return ', '.join(t for bit, t in THROTTLE_BITS if v & bit) or 'never'


def game_from_demo(path):
    m = re.match(r'([a-z0-9]+)', os.path.basename(path))
    return m.group(1) if m and m.group(1) in GAMES else None


# ---- the scratch install ---------------------------------------------------

def wad_dirs(binary):
    """Where to look for wads, in order.  The binary's own directory first:
    the engine looks for legacy.wad next to itself before anywhere else, and a
    Pi keeps it (and often the IWADs) there rather than in ~/games/doom.  The
    rest is the list tools/smoke.sh searches."""
    dirs = [os.path.dirname(os.path.abspath(binary)),
            os.environ.get('DOOMWADDIR', ''),
            os.path.expanduser('~/games/doom'),
            os.path.expanduser('~/games/doomwads'),
            os.path.expanduser('~/games/doomlegacy/wads'),
            '/usr/share/games/doom',
            '/usr/local/share/games/doom']
    out = []
    for d in dirs:
        if d and os.path.isdir(d) and os.path.realpath(d) not in [os.path.realpath(x) for x in out]:
            out.append(d)
    return out


def find_wads(binary):
    """{file name: path} for every .wad/.pk3 in wad_dirs(), first found wins.
    Only those: the binary's directory also holds legacyhome and backups."""
    wads = {}
    for d in wad_dirs(binary):
        for f in sorted(os.listdir(d)):
            if f.lower().endswith(('.wad', '.pk3')) and f not in wads:
                path = os.path.join(d, f)
                if os.path.isfile(path):
                    wads[f] = path
    return wads


def make_rundir(binary, home_src, demo, settings, wads):
    rd = tempfile.mkdtemp(prefix='perfchart.')
    shutil.copy2(binary, os.path.join(rd, 'doomlegacyarcade'))
    home = os.path.join(rd, 'legacyhome')
    # Not the demos (the benchmark demo is passed on its own) and not the
    # config backups; the level packs are kept, a --demo may need one.
    shutil.copytree(home_src, home,
                    ignore=shutil.ignore_patterns('demos', '*.bak*', '*.pre*'))
    for stale in ('config8p.cfg', 'configgl.cfg', 'confign.cfg', 'autoexec.cfg'):
        p = os.path.join(home, stale)
        if os.path.exists(p):
            os.remove(p)
    cfg = os.path.join(home, 'config.cfg')
    text = open(cfg, encoding='latin-1').read() if os.path.exists(cfg) else ''
    for key, val in settings:
        line = '%s "%s"' % (key, val)
        text, n = re.subn(r'(?m)^%s .*$' % re.escape(key), line, text)
        if not n:
            text += '\n%s\n' % line
    open(cfg, 'w', encoding='latin-1').write(text)

    # Linked, not copied: they are large and only read.
    for f, path in wads.items():
        os.symlink(os.path.abspath(path), os.path.join(rd, f))
    shutil.copy2(demo, os.path.join(rd, 'bench.lmp'))
    return rd


def config_values(rd, keys):
    """The values the scratch config actually holds for these keys."""
    try:
        text = open(os.path.join(rd, 'legacyhome', 'config.cfg'), encoding='latin-1').read()
    except OSError:
        return []
    out = []
    for k in keys:
        m = re.search(r'(?m)^%s\s+"?([^"\n]*)"?\s*$' % re.escape(k), text)
        out.append((k, m.group(1) if m else
                    '%s (default)' % REPORTED_DEFAULT.get(k, '?')))
    return out


def run_one(rd, game, w, h, headless, timeout, extra):
    argv = ['./doomlegacyarcade', '-game', game, '-width', str(w),
            '-height', str(h), '-timedemo', 'bench.lmp', '-frameprofile'] + extra
    env = dict(os.environ, SDL_NO_SIGNAL_HANDLERS='1', SDL_AUDIODRIVER='dummy')
    if headless:
        env.update(DISPLAY='', SDL_VIDEODRIVER='offscreen')
    t0 = time.time()
    try:
        p = subprocess.run(argv, cwd=rd, env=env, timeout=timeout,
                           capture_output=True)
        log = (p.stdout + p.stderr).decode('latin-1', 'replace')
    except subprocess.TimeoutExpired as e:
        log = ((e.stdout or b'') + (e.stderr or b'')).decode('latin-1', 'replace')
        log += '\ntimed out after %ds' % timeout
    log = STRIP_ANSI.sub('', log)
    r = {'wall': time.time() - t0, 'log': log}
    m = RESULT.search(log)
    if m:
        r['tics'] = int(m.group(1))
        r['fps'] = float(m.group(3))
        if m.group(4):
            r['drew'] = (int(m.group(4)), int(m.group(5)))
            r['bpp'] = int(m.group(6))
            r['renderer'] = m.group(7)
    m = PROFILE.search(log)
    if m:
        tics, views, present, hud, total = (float(m.group(i)) for i in range(2, 7))
        r['views'], r['present'], r['other'], r['total'] = views, present, tics + hud, total
        # The draw8bpp expansion, a part of present; absent from older engines
        # and zero when 8bpp Draw is off.
        if m.group(7) is not None:
            r['expand'] = float(m.group(7))
    m = RENDER.search(log)
    if m:
        r['render'] = m.group(1).strip()
    outs = OUTPUT.findall(log)
    if outs:
        r['output'] = '%sx%s' % outs[-1]
    m = VERSION.search(log)
    if m:
        r['version'] = m.group(1)
    if 'fps' not in r:
        err = re.search(r'^.*(Error|error:|timed out).*$', log, re.M)
        r['error'] = err.group(0).strip()[:160] if err else 'no timedemo result in the output'
    return r


# ---- output -----------------------------------------------------------------

def fps_cell(vals):
    if not vals:
        return '—'
    lo, hi = round(min(vals)), round(max(vals))
    return str(lo) if lo == hi else '%d–%d' % (lo, hi)


def read_compare(path):
    old = {}
    with open(path, newline='') as fh:
        for row in csv.DictReader(fh):
            if row.get('fps_median'):
                old[row['size']] = float(row['fps_median'])
    return old


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', default=BIN)
    ap.add_argument('--home', help='legacyhome to copy (default: the one next '
                                   'to the binary)')
    ap.add_argument('--demo', default=DEMO)
    ap.add_argument('--game', help='default: from the demo name, e.g. doomu_...')
    ap.add_argument('--sizes', help='comma separated WxH')
    ap.add_argument('--quick', action='store_true',
                    help='only ' + ' '.join('%dx%d' % s for s in QUICK))
    ap.add_argument('--runs', type=int, default=1, help='runs per size (default 1)')
    ap.add_argument('--set', action='append', default=[], metavar='CVAR=VALUE',
                    help='override a setting, e.g. render_threads=1 (repeatable)')
    ap.add_argument('--headless', action='store_true',
                    help='offscreen driver, no window; the numbers then leave '
                         'out the real display\'s present')
    ap.add_argument('--args', default='', help='extra engine arguments, e.g. '
                                               '"-file legacyhome/levels/x.wad"')
    ap.add_argument('--compare', help='a previous perfchart .csv to compare with')
    ap.add_argument('--out', default='.', help='directory for the .md and .csv')
    ap.add_argument('--timeout', type=int, default=300, help='seconds per run')
    ap.add_argument('--keep', action='store_true', help='keep the scratch directory')
    a = ap.parse_args()

    if not os.access(a.binary, os.X_OK):
        sys.exit('No engine at %s -- build it first, or pass --binary.' % a.binary)
    home_src = a.home or os.path.join(os.path.dirname(os.path.abspath(a.binary)),
                                      'legacyhome')
    if not os.path.isdir(home_src):
        sys.exit('No legacyhome at %s -- pass --home.' % home_src)
    if not os.path.exists(a.demo):
        sys.exit('No demo at %s.' % a.demo)
    game = a.game or game_from_demo(a.demo)
    if not game:
        sys.exit('Cannot tell the game from %s -- pass --game.' % a.demo)
    # Checked here rather than left to the engine, whose "No legacy.wad file"
    # does not say where it looked -- and where it looked is a scratch copy.
    wads = find_wads(a.binary)
    legacy = [f for f in wads if f.lower() == 'legacy.wad']
    if not legacy:
        sys.exit('No legacy.wad found.  Looked in:\n  %s\nSet DOOMWADDIR to the '
                 'directory that holds it.' % '\n  '.join(wad_dirs(a.binary)))
    print('legacy.wad from %s' % os.path.dirname(wads[legacy[0]]))
    if not a.headless and not (os.environ.get('DISPLAY') or
                               os.environ.get('WAYLAND_DISPLAY')):
        print('No DISPLAY: running --headless.  (From SSH on a Pi, set '
              'DISPLAY=:0 to measure on its real screen.)')
        a.headless = True

    if a.sizes:
        sizes = [tuple(int(v) for v in s.lower().split('x'))
                 for s in a.sizes.split(',') if s.strip()]
    else:
        sizes = QUICK if a.quick else SIZES
    sizes = sorted(set(sizes), key=lambda s: (s[0] * s[1], s[0]))
    caps = engine_caps()
    if caps:
        over = [s for s in sizes if s[0] > caps[0] or s[1] > caps[1]]
        for s in over:
            print('Skipping %dx%d: above the engine\'s %dx%d limit.' % (s + caps))
        sizes = [s for s in sizes if s not in over]

    settings = dict(SETTINGS)
    for kv in a.set:
        k, _, v = kv.partition('=')
        settings[k.strip()] = v.strip().strip('"')
    settings = list(settings.items())
    extra = a.args.split()

    host = socket.gethostname()
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M')
    os.makedirs(a.out, exist_ok=True)
    base = os.path.join(a.out, 'perfchart-%s-%s' % (host, stamp))
    compare = read_compare(a.compare) if a.compare else {}
    throttled_before = pi_throttled()

    print('perfchart: %d sizes x %d run(s), demo %s, game %s%s'
          % (len(sizes), a.runs, os.path.basename(a.demo), game,
             ', headless' if a.headless else ''))
    rd = make_rundir(a.binary, home_src, a.demo, settings, wads)
    forced = set(k for k, _ in settings)
    reported = config_values(rd, [k for k in REPORTED if k not in forced])
    rows, info = [], {}

    # A throwaway run first.  The first run of a chart starts cold -- the
    # wads, the demo and the engine itself coming off the SD card -- and on
    # the Pi that cost the first size up to a quarter of its frame rate
    # (320x200 read 85 fps first in one chart and 114 in the next).  Every
    # size is then measured warm, the same way.
    if sizes:
        print('  warming up at %dx%d' % sizes[0])
        run_one(rd, game, sizes[0][0], sizes[0][1], a.headless, a.timeout, extra)
    csvf = open(base + '.csv', 'w', newline='')
    cw = csv.writer(csvf)
    cw.writerow(['size', 'width', 'height', 'shape', 'fps_min', 'fps_median',
                 'fps_max', 'runs', 'gametics', 'drew', 'temp_c',
                 'views_ms', 'present_ms', 'other_ms', 'expand_ms', 'note'])
    try:
        for (w, h) in sizes:
            fps, tics, notes, drew, temp = [], set(), [], None, None
            split = {'views': [], 'present': [], 'other': [], 'expand': []}
            for n in range(a.runs):
                r = run_one(rd, game, w, h, a.headless, a.timeout, extra)
                for k in ('render', 'version', 'renderer', 'bpp', 'output'):
                    if k in r and k not in info:
                        info[k] = r[k]
                if 'error' in r:
                    notes.append(r['error'])
                    if a.keep:
                        open(os.path.join(rd, 'log-%dx%d-%d.txt' % (w, h, n)),
                             'w').write(r['log'])
                    print('  %5dx%-4d  failed: %s' % (w, h, r['error']))
                    if not rows and not fps and n == 0:
                        # The very first run of the chart.  A failure here is
                        # the setup (demo, game, wads, display), not the size,
                        # and every other size would sit out the same timeout.
                        print('\nThe first run gave no result, so the rest would not '
                              'either.  Last lines of its output:\n')
                        print('\n'.join(r['log'].strip().splitlines()[-15:]))
                        raise SystemExit(2)
                    continue
                fps.append(r['fps'])
                tics.add(r['tics'])
                if 'total' in r:
                    for k in split:
                        if k in r:
                            split[k].append(r[k])
                    # Does the time add up?  The profile covers the frames the
                    # fps figure counts, so its total must be 1000/fps.  If the
                    # fps figure is lower, wall time went somewhere no frame
                    # was drawn -- which is how the screen wipe was found.
                    measured = 1000.0 / r['fps']
                    if abs(r['total'] - measured) > 0.10 * measured:
                        notes.append('time does not add up: frames account for %.1f ms '
                                     'each, the fps figure says %.1f' % (r['total'], measured))
                drew = r.get('drew')
                if drew and drew != (w, h):
                    notes.append('drew %dx%d instead' % drew)
                temp = pi_temp()
                print('  %5dx%-4d  %7.1f fps  %5.1fs%s%s' % (
                    w, h, r['fps'], r['wall'],
                    ('   views %.2f  present %.2f  other %.2f ms%s'
                     % (r['views'], r['present'], r['other'],
                        ('  (expand %.2f)' % r['expand']) if r.get('expand') else ''))
                    if 'total' in r else '',
                    ('   %.0f°C' % temp) if temp is not None else ''))
            if drew and drew != (w, h):
                fps = []            # measured something else: do not credit it
            med = {k: (statistics.median(v) if v and fps else None) for k, v in split.items()}
            row = dict(size='%dx%d' % (w, h), w=w, h=h, fps=fps, tics=tics,
                       notes=sorted(set(notes)), temp=temp, drew=drew, split=med)
            rows.append(row)
            cw.writerow([row['size'], w, h, shape(w, h),
                         '%.1f' % min(fps) if fps else '',
                         '%.1f' % statistics.median(fps) if fps else '',
                         '%.1f' % max(fps) if fps else '',
                         len(fps), '/'.join(str(t) for t in sorted(tics)),
                         '%dx%d' % drew if drew else '',
                         '%.1f' % temp if temp is not None else '',
                         ] + ['%.3f' % med[k] if med[k] is not None else ''
                              for k in ('views', 'present', 'other', 'expand')] + [
                         '; '.join(row['notes'])])
            csvf.flush()
    except KeyboardInterrupt:
        print('\nInterrupted -- writing what was measured.')
    finally:
        csvf.close()
        if a.keep:
            print('kept %s' % rd)
        else:
            shutil.rmtree(rd, ignore_errors=True)

    # Drawing cannot change the simulation, so every size plays the same tics.
    all_tics = set().union(*(r['tics'] for r in rows)) if rows else set()
    tics_ok = len(all_tics) == 1
    throttled = pi_throttled()

    lines = []
    lines.append('**%s**%s — %s' % (host, (' (%s)' % cpu_name()) if cpu_name() else '',
                                    datetime.datetime.now().strftime('%Y-%m-%d %H:%M')))
    lines.append('')
    lines.append('Build %s. Demo `%s` (%s game tics). %s%s.'
                 % (info.get('version', '?'), os.path.basename(a.demo),
                    '/'.join(str(t) for t in sorted(all_tics)) or '?',
                    ', '.join('%s "%s"' % kv for kv in
                              [kv for kv in settings if kv[0] not in ('fullscreen', 'localplayers')]
                              + reported),
                    ', headless' if a.headless else ''))
    if info.get('render'):
        lines.append('Renderer: %s.%s' % (info['render'],
                     (' Frames scaled to %s (the display).' % info['output'])
                     if info.get('output') else ''))
    lines.append('')
    has_temp = any(r['temp'] is not None for r in rows)
    has_split = any(r['split']['views'] is not None for r in rows)
    has_expand = any(r['split']['expand'] for r in rows)
    head = ['Resolution', 'Shape', 'FPS']
    if has_split:
        head += ['Views ms', 'Present ms', 'Other ms']
    if has_expand:
        head += ['(of Present) Expand ms']
    if compare:
        head += ['Before', 'Change']
    if has_temp:
        head += ['Temp']
    lines.append('| ' + ' | '.join(head) + ' |')
    lines.append('| ' + ' | '.join('---' for _ in head) + ' |')
    for r in rows:
        cells = [r['size'], shape(r['w'], r['h']), fps_cell(r['fps'])]
        if r['notes'] and not r['fps']:
            cells[2] = '— (%s)' % r['notes'][0]
        if has_split:
            cells += ['%.2f' % r['split'][k] if r['split'][k] is not None else '—'
                      for k in ('views', 'present', 'other')]
        if has_expand:
            cells.append('%.2f' % r['split']['expand'] if r['split']['expand'] is not None else '—')
        if compare:
            old = compare.get(r['size'])
            new = statistics.median(r['fps']) if r['fps'] else None
            cells.append('%d' % round(old) if old else '—')
            cells.append('%+.0f%%' % (100.0 * (new - old) / old) if (old and new) else '—')
        if has_temp:
            cells.append('%.0f°C' % r['temp'] if r['temp'] is not None else '')
        lines.append('| ' + ' | '.join(cells) + ' |')
    if has_split:
        lines.append('')
        lines.append('Per frame: **Views** is drawing the 3D view (what Render Threads '
                     'spreads over the cores). **Present** is getting the finished frame '
                     'onto the screen: the 8bpp palette expansion, the upload and the '
                     'scale to the display. **Other** is the game logic and the HUD. '
                     'They add up to 1000 / FPS.'
                     + (' **Expand** is the part of Present that is the 8bpp palette '
                        'expansion, which Render Threads splits across the cores; the rest of '
                        'Present is the upload and the scale, inside SDL and the driver.'
                        if has_expand else ''))

    warnings = []
    if rows and not all_tics:
        warnings.append('No size produced a result.')
    elif rows and not tics_ok:
        warnings.append('Sizes played different numbers of game tics (%s). Drawing cannot '
                        'change the simulation, so at least one run did not play the whole '
                        'demo; distrust this chart.' % ', '.join(str(t) for t in sorted(all_tics)))
    for r in rows:
        for n in r['notes']:
            warnings.append('%s: %s' % (r['size'], n))
    if throttled is not None:
        newly = throttled & ~(throttled_before or 0)
        if throttled & 0xF:
            warnings.append('The Pi is being slowed right now: %s. These numbers are '
                            'lower than the board can do.' % describe_throttle(throttled & 0xF))
        elif newly:
            warnings.append('During this run the Pi recorded: %s. Some sizes were measured '
                            'on a slowed-down CPU.' % describe_throttle(newly))
        else:
            lines.append('')
            lines.append('Pi throttling during the run: none.')
            if throttled:
                # The "has occurred" bits stick from boot, so this happened
                # before the chart started -- worth knowing, not a fault in it.
                lines.append('Earlier since boot: %s (get_throttled 0x%x).'
                             % (describe_throttle(throttled), throttled))
    if warnings:
        lines.append('')
        lines += ['> **Warning:** ' + w for w in warnings]

    md = '\n'.join(lines) + '\n'
    open(base + '.md', 'w', encoding='utf-8').write(md)
    print()
    print(md)
    print('Wrote %s.md and %s.csv' % (base, base))
    return 1 if warnings else 0


if __name__ == '__main__':
    sys.exit(main())
