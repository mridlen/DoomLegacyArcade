#!/usr/bin/env python3
"""Line up the NETTRACE logs of the cabinets in one linked game.

    tools/nettrace-diff.py hostlog.txt pilog.txt winlog.txt

Each cabinet run with -logfile writes, among everything else:
  NETTRACE snap tic tic=N ...   every 5 s, at the same gametic everywhere
  NETTRACE consfault / repair received / waitpause / pause / color

Prints, per cabinet, the events that explain a PAUSE nobody pressed, then the
first gametic whose snapshot differs between cabinets and what differed.  The
server= field is ignored (it differs by design).  Also prints each cabinet's
build banner, since cabinets on different builds can drift apart.

See docs/arcade/cabinet-link.md, "Tracing an unexplained PAUSE".
"""
import re
import sys

ESC = re.compile(r'\x1b\[[0-9;]*m')
EVENTS = ('consfault', 'repair', 'waitpause', 'pause', 'pausecmd', 'statepause', 'color')


def load(path):
    snaps, events, build, cons = {}, [], None, {}
    with open(path, 'rb') as f:
        for raw in f:
            line = ESC.sub('', raw.decode('utf-8', 'replace')).rstrip()
            if build is None:
                m = re.search(r'Doom Legacy Arcade (v\S+)', line)
                if m:
                    build = m.group(1)
            i = line.find('NETTRACE ')
            if i < 0:
                continue
            body = line[i + len('NETTRACE '):]
            word = body.split(' ', 1)[0]
            if body.startswith('snap tic '):
                m = re.search(r'tic=(\d+)', body)
                # drop the fields that differ by design
                key = re.sub(r' server=\d', '', body)
                snaps[int(m.group(1))] = key
            elif word == 'conshist':
                # per-tic consistency around a fault: tic:value ...
                for t, v in re.findall(r' (\d+):([0-9A-F]+)', body):
                    cons[int(t)] = v
            elif word in EVENTS:
                events.append(body)
    return build, snaps, events, cons


def fields(snap):
    head, _, players = snap.partition('|')
    out = dict(kv.split('=', 1) for kv in head.split() if '=' in kv)
    for kv in players.split():
        pn, v = kv.split('=', 1)
        out['player ' + pn] = v
    return out


def main(paths):
    if len(paths) < 1:
        print(__doc__)
        return 2
    logs = [(p,) + load(p) for p in paths]
    conss = [l[4] for l in logs]
    logs = [l[:4] for l in logs]
    for p, build, snaps, events in logs:
        print('== %s  build %s  snapshots %d' % (p, build or '?', len(snaps)))
        for e in events:
            if not e.startswith('color') or 'ingame=1' in e:
                print('   ' + e[:200])
    builds = {b for _, b, _, _ in logs}
    if len(builds) > 1:
        print('\n!! the cabinets are on DIFFERENT BUILDS: %s' % ', '.join(sorted(map(str, builds))))
    if len(logs) < 2:
        return 0
    # The exact tic, where a fault left history on more than one cabinet.
    have = [c for c in conss if c]
    if len(have) >= 2:
        both = sorted(set.intersection(*(set(c) for c in have)))
        diff = [t for t in both if len({c[t] for c in have}) > 1]
        if diff:
            print('\nconsistency first differs at tic %d (%s)'
                  % (diff[0], ' vs '.join(c[diff[0]] for c in have)))
    common = sorted(set.intersection(*(set(s) for _, _, s, _ in logs)))
    for tic in common:
        vals = [s[tic] for _, _, s, _ in logs]
        if len(set(vals)) == 1:
            continue
        print('\nfirst difference at tic %d (5 s snapshots; it happened after the previous one)' % tic)
        fs = [fields(v) for v in vals]
        for k in sorted(set().union(*fs)):
            got = [f.get(k, '-') for f in fs]
            if len(set(got)) > 1:
                print('   %-10s ' % k + '   '.join('%s' % g for g in got))
        return 1
    print('\nno difference in %d common snapshots' % len(common))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
