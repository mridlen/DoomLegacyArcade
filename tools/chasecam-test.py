#!/usr/bin/env python3
"""The chase camera must not change the simulation.

The cabinet records its record demos with the chase camera off and replays them
on the attract screen with it on (cv_chasecamdemo, d_main.c).  So if the camera
perturbs the simulation at all, the attract screen desyncs -- which is what it
did.  P_PlayerThink called P_CalcHeight only when the camera was *off*, so with
it on player->bob froze, and bob is read by A_WeaponReady to place the weapon
sprite; A_Lower/A_Raise then needed a different number of tics, weapon switches
landed on different tics, attacks fired on different tics, and the shared
P_Random index diverged from there.  See docs/arcade/demo-desync.md.

This replays each demo twice -- chase camera off, then on -- and compares the
-synclog tic by tic.  They must be identical.

**It must not pass -nodraw, and that is the whole point.**  R_Update_Chase_Camera
is called from D_Display (d_main.c), so under -nodraw the camera is never
created and none of this code runs at all.  `make demotest` passes -nodraw, so
it cannot see this class of bug however many demos it replays: measured on
doomu-sl_E1M2_sk0_tyson, a -nodraw run reported 0 camera moves and 0 spawns,
while the same demo drawn reported 10665 moves, 1 spawn and 16 unsticks.

Shown to fail as well as pass: against the build before the fix, two of the
four default demos diverged (first at leveltime 5845, prnd 248 against 251),
and all four are identical after it.

Cost: it draws every tic, so a demo costs far more than it does under
demotest's -nodraw -- about 14 s a run serially.  -j runs several demos at
once in their own slot directories (the engine writes synclog_play.txt into
its working directory, so slots cannot be shared), which is what makes --all
practical: ~57 minutes serial against ~7 with -j8.  Both halves of one demo's
pair always run in the same slot, back to back, so an A and its B see the same
machine.

    tools/chasecam-test.py [--all] [-j N] [-b BINARY] [--home DIR]
                           [--waddir DIR] [-k] [demo substring ...]
"""
import argparse, filecmp, io, os, queue, re, shutil, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)

# Demos that exercise the paths this bug lived on: a chainsaw run (the weapon
# whose timing first showed the divergence), a max run over the same map, and
# two speed runs on other maps.  --all replays every demo in the home.
DEFAULT = ['doomu-sl_E1M2_sk0_tyson', 'doomu-sl_E1M2_sk0_max',
           'doomu_E4M1_sk0_speed', 'doomu_E1M2_sk3_speed']


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


def prepare(scratch, binary, home, waddir):
    """A scratch copy: never run the engine in the cabinet's own directory."""
    shutil.copy2(binary, os.path.join(scratch, 'doomlegacyarcade'))
    os.chmod(os.path.join(scratch, 'doomlegacyarcade'), 0o755)
    shutil.copytree(home, os.path.join(scratch, 'legacyhome'))
    for nm in os.listdir(waddir):
        if nm.lower().endswith(('.wad', '.pk3')):
            dst = os.path.join(scratch, nm)
            if not os.path.exists(dst):
                os.symlink(os.path.join(waddir, nm), dst)
    hm = os.path.join(scratch, 'legacyhome')
    # A GL config under the dummy driver segfaults on the first console line
    # drawn (CLAUDE.md); the per-drawmode configs run after config.cfg and
    # would put the drawmode back, so they go.
    for f in ('config8p.cfg', 'configgl.cfg', 'confign.cfg', 'autoexec.cfg'):
        p = os.path.join(hm, f)
        if os.path.exists(p):
            os.remove(p)
    cfg = os.path.join(hm, 'config.cfg')
    s = io.open(cfg, encoding='utf-8', errors='replace').read()
    # Pinned rather than inherited from the cabinet, for two different reasons.
    #
    # framerate_cap 35: r_fps.c sets interp_active from
    # (cv_framerate_cap.value != TICRATE), so at the cabinet's 60 the frame is
    # drawn a wall-clock fraction of a tic ahead of the simulation and the
    # number of frames per tic depends on how fast the machine happens to be.
    # This check exists to compare code reached from D_Display, so letting the
    # draw rate float with load is exactly the wrong thing to leave loose.
    #
    # render_threads 1: each run would otherwise spawn 4 render workers, so -j8
    # puts 32 threads on 8 cores and parallelism buys nothing.  Measured: 8
    # workers at render_threads 4 completed 173 runs in 40 minutes, 4.3 a
    # minute -- the same rate as running them one at a time.
    for key, val in (('drawmode', '"Software 8bit"'),
                     ('framerate_cap', '"35"'),
                     ('render_threads', '"1"')):
        pat = r'(?m)^%s .*$' % key
        if re.search(pat, s):
            s = re.sub(pat, '%s %s' % (key, val), s)
        else:
            s += '\n%s %s\n' % (key, val)
    io.open(cfg, 'w', encoding='utf-8').write(s)
    return hm


def set_chasecam(cfg, v):
    s = io.open(cfg, encoding='utf-8', errors='replace').read()
    s = re.sub(r'(?m)^chasecam .*\n', '', s) + '\nchasecam "%d"\n' % v
    io.open(cfg, 'w', encoding='utf-8').write(s)


def main():
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument('-b', '--binary',
                    default=os.path.join(REPO, 'svn1749', 'bin', 'doomlegacyarcade'))
    ap.add_argument('--home')
    ap.add_argument('--waddir',
                    default=os.environ.get('DOOMWADDIR',
                                           os.path.expanduser('~/games/doom')))
    ap.add_argument('--all', action='store_true')
    ap.add_argument('-j', '--jobs', type=int, default=0,
                    help='demos at once (default: cores, capped at the demo count)')
    # Generous: render_threads is pinned to 1 (see prepare), so the longest
    # record demos take far longer here than they do on the cabinet.  At 300
    # the two E1M6 runs were cut off mid-comparison.
    ap.add_argument('--timeout', type=int, default=1200)
    ap.add_argument('-k', '--keep', action='store_true')
    ap.add_argument('-h', '--help', action='store_true')
    ap.add_argument('filters', nargs='*')
    a = ap.parse_args()
    if a.help:
        print(__doc__)
        return 0

    binary = os.path.abspath(a.binary)
    if not os.path.isfile(binary):
        print('chasecam-test: no binary at %s' % binary)
        return 2
    home = os.path.abspath(a.home) if a.home else os.path.join(
        os.path.dirname(binary), 'legacyhome')
    demodir = os.path.join(home, 'demos')
    if not os.path.isdir(demodir):
        print('chasecam-test: no demos at %s' % demodir)
        return 2

    names = sorted(d[:-4] for d in os.listdir(demodir) if d.endswith('.lmp'))
    if not a.all:
        names = [n for n in names if n in DEFAULT] or names[:4]
    if a.filters:
        names = [n for n in names if any(f in n for f in a.filters)]
    if not names:
        print('chasecam-test: no demos selected')
        return 2

    jobs = a.jobs or (os.cpu_count() or 1)
    jobs = max(1, min(jobs, len(names)))

    root = tempfile.mkdtemp(prefix='doomlegacy-chasecam.')
    print('chasecam-test: %d demo(s), %d at a time\n  binary : %s\n  scratch: %s'
          % (len(names), jobs, binary, root))
    try:
        # One slot per worker.  The engine writes synclog_play.txt into its
        # working directory and each run rewrites config.cfg to set chasecam,
        # so slots cannot be shared -- two workers in one directory would
        # overwrite each other's log and each other's setting.
        slots = queue.Queue()
        for i in range(jobs):
            d = os.path.join(root, 'slot%d' % i)
            os.mkdir(d)
            slots.put((d, prepare(d, binary, home, a.waddir)))
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                   SDL_NO_SIGNAL_HANDLERS='1')

        def run_one(n):
            """-> (name, status, detail).  Both halves share one slot."""
            slot, hm = slots.get()
            try:
                args = demo_args(n, os.path.join(hm, 'levels'))
                if args is None:
                    return (n, 'skip', 'game id or level pack not recognised')
                cfg = os.path.join(hm, 'config.cfg')
                logs = []
                for v in (0, 1):
                    set_chasecam(cfg, v)
                    sl = os.path.join(slot, 'synclog_play.txt')
                    if os.path.exists(sl):
                        os.remove(sl)
                    try:
                        subprocess.run(
                            ['./doomlegacyarcade'] + args +
                            # No -nodraw: see the note at the top of this file.
                            ['-timedemo',
                             os.path.join('legacyhome', 'demos', n + '.lmp'),
                             '-synclog'],
                            cwd=slot, env=env, capture_output=True,
                            timeout=a.timeout)
                    except subprocess.TimeoutExpired:
                        # Not a desync: nothing was compared.  Reporting it as
                        # one is how a check starts crying wolf -- and this one
                        # is partly self-inflicted, since pinning
                        # render_threads 1 makes a long demo take substantially
                        # longer than it does on the cabinet's 4.
                        return (n, 'timeout',
                                'no comparison: exceeded %ds (raise --timeout)'
                                % a.timeout)
                    if not os.path.exists(sl):
                        # Two runs that both failed to start compare equal, so
                        # this must never read as a pass.
                        return (n, 'bad', 'demo did not run (no synclog)')
                    dst = os.path.join(slot, 'ab_%s_%d.txt' % (n, v))
                    os.replace(sl, dst)
                    logs.append(dst)
                if filecmp.cmp(logs[0], logs[1], shallow=False):
                    return (n, 'ok', '%6d tics' % sum(1 for _ in open(logs[0])))
                x = open(logs[0]).read().splitlines()
                y = open(logs[1]).read().splitlines()
                where = next((i for i, (p, q) in enumerate(zip(x, y)) if p != q),
                             None)
                if where is None:
                    # One run ended earlier with every shared tic identical.
                    # A -timedemo under load does not always stop at the same
                    # tic; demotest.sh reports the same case separately, for the
                    # same reason.  The simulation agreed for as long as both
                    # were running, which is what is being asserted -- but it is
                    # weaker evidence, so it is named rather than called ok.
                    return (n, 'short', 'agreed for %d tics, then lengths %d and %d'
                            % (min(len(x), len(y)), len(x), len(y)))
                return (n, 'bad',
                        'first differing line %d\n    camera off: %s\n'
                        '    camera on : %s' % (where + 1, x[where], y[where]))
            finally:
                slots.put((slot, hm))

        label = {'ok': '  ok        ', 'skip': '  SKIP      ',
                 'short': '  ok(short) ', 'bad': '  DESYNC    ',
                 'timeout': '  TIMEOUT   '}
        bad = skipped = short = timedout = 0
        done = 0
        # Printed as each demo finishes, not collected and printed at the end.
        # --all takes long enough to hit a wall-clock limit, and a run that
        # holds every result until the last one turns a timeout into no
        # information at all -- which is what happened: 173 of 238 runs
        # completed and the report was empty.  ex.map yields in *input* order,
        # not completion order, so a slow demo early in the list still stalls
        # the output behind it; that is the price of a stable, diffable report,
        # and it still means a killed run leaves most of its findings on
        # screen.
        with ThreadPoolExecutor(max_workers=jobs) as ex:
            for n, st, detail in ex.map(run_one, names):
                done += 1
                print('%s [%3d/%3d] %-30s %s'
                      % (label[st], done, len(names), n, detail), flush=True)
                if st == 'bad':
                    bad += 1
                elif st == 'skip':
                    skipped += 1
                elif st == 'short':
                    short += 1
                elif st == 'timeout':
                    timedout += 1
        print('\n%d demo(s) compared, %d skipped, %d timed out, %d ended at a '
              'different tic (prefix matched), %d desynced'
              % (len(names) - skipped - timedout, skipped, timedout, short, bad))
        # A timeout compared nothing, so it is not a pass either -- it exits
        # non-zero on its own, but it is never counted as a desync.
        return 1 if (bad or timedout) else 0
    finally:
        if a.keep:
            print('kept: %s' % root)
        else:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
