#!/usr/bin/env python3
"""[Arcade] Which settings in config.cfg actually shape how the cabinet plays?

    tools/cfgaudit.py svn1749/src svn1749/bin/legacyhome/config.cfg

Answers a question that is otherwise guesswork: of everything in config.cfg,
which entries affect gameplay, are NOT overridden by the ranked ruleset, and
differ from the engine's compiled default?  Those few are what every scored run
is actually played under.

Why it is needed.  config.cfg overrides compiled defaults, so changing a
default in source does nothing on a machine that already has a config -- the
trap in CLAUDE.md that has now bitten four times, most recently leaving rocket
trails Off on the cabinet after the default was deliberately corrected to On.
Nothing reported it: M_Verify_Config only checks that a setting *loaded*, never
whether it is one you still mean to keep.

How it decides what counts:

  gameplay-affecting = written into the demo header by G_BeginRecording,
                       or reset by G_demo_defaults, or flagged CV_NETVAR
  pinned             = listed in hs_ranked_rules[] (hs_stuff.c), which the
                       ranked ruleset forces for scored play

Values are resolved through each cvar's CV_PossibleValue_t table before being
compared, so "On" and "2" count as equal where the table says they are the same
value.  Comparing raw strings instead reports 33 differences that are nothing
but a label sitting next to its own number -- which is what the first version
of this did, and it buried the one real finding.

Anything it cannot resolve is listed separately rather than assumed equal.
"""
import re, sys, os, glob

SRC, CFG = sys.argv[1], sys.argv[2]

srcs = [p for p in glob.glob(os.path.join(SRC, '**', '*.c'), recursive=True)
        if 'nodebuild' not in p]
alltxt = {p: open(p, encoding='utf-8', errors='surrogateescape').read() for p in srcs}

# ---- CV_PossibleValue_t tables: table name -> {label.lower(): value}
tables = {}
for txt in alltxt.values():
    for m in re.finditer(r'CV_PossibleValue_t\s+(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;',
                         txt, re.S):
        d = {}
        for e in re.finditer(r'\{\s*(-?\d+)\s*,\s*"([^"]*)"\s*\}', m.group(2)):
            d[e.group(2).lower()] = int(e.group(1))
        if d:
            tables[m.group(1)] = d

# ---- consvar_t: C name -> (cvar name, default, flags, table name)
decls = {}
for txt in alltxt.values():
    for m in re.finditer(
            r'consvar_t\s+(cv_\w+)\s*=\s*\{\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,'
            r'([^,]*),\s*(\w+)', txt):
        decls[m.group(1)] = (m.group(2), m.group(3), m.group(4), m.group(5))

def resolve(cname, text):
    """Resolve a config value (label or number) to an int, if we can."""
    if cname not in decls:
        return None
    tbl = tables.get(decls[cname][3])
    t = text.strip().lower()
    if tbl and t in tbl:
        return tbl[t]
    try:
        return int(float(t))
    except ValueError:
        return None

byname = {v[0]: k for k, v in decls.items()}

# ---- ruleset pins
hs = alltxt[os.path.join(SRC, 'hs_stuff.c')]
tbl = hs[hs.index('hs_ranked_rules[]'):]
tbl = tbl[:tbl.index('\n};')]
pinned = {decls[m.group(1)][0] for m in re.finditer(r'\{\s*&(cv_\w+)\s*,', tbl)
          if m.group(1) in decls}

# ---- gameplay set: demo header + demo defaults + NETVAR
gg = alltxt[os.path.join(SRC, 'g_game.c')]
gameplay = set()
for fn in ('void G_BeginRecording', 'void G_demo_defaults'):
    blk = gg[gg.index(fn):]
    blk = blk[:blk.index('\n}')]
    for m in re.finditer(r'(cv_\w+)\.(?:EV|value)', blk):
        if m.group(1) in decls:
            gameplay.add(decls[m.group(1)][0])
gameplay |= {v[0] for v in decls.values() if 'CV_NETVAR' in v[2]}

# ---- compare
eff, over, unknown = [], [], []
for i, line in enumerate(open(CFG, encoding='utf-8', errors='replace'), 1):
    m = re.match(r'\s*(\w+)\s+"([^"]*)"', line)
    if not m:
        continue
    name, val = m.group(1), m.group(2)
    if name not in gameplay or name not in byname:
        continue
    cname = byname[name]
    got, dflt = resolve(cname, val), resolve(cname, decls[cname][1])
    if got is None or dflt is None:
        unknown.append((i, name, val, decls[cname][1]))
    elif got != dflt:
        (over if name in pinned else eff).append((i, name, val, decls[cname][1], got, dflt))

print("Gameplay settings in config.cfg that differ from the engine default")
print("=" * 70)
print(f"\n*** IN EFFECT -- not pinned by the ranked ruleset ({len(eff)}) ***")
for i, n, v, d, g, df in sorted(eff, key=lambda r: r[1]):
    print(f"  line {i:>4}  {n:<20} = {v!r} ({g})   default {d!r} ({df})")
print(f"\nOverridden for scored play -- ruleset pins these ({len(over)}):")
for i, n, v, d, g, df in sorted(over, key=lambda r: r[1]):
    print(f"  line {i:>4}  {n:<20} = {v!r} ({g})   default {d!r} ({df})")
if unknown:
    print(f"\nCould not resolve ({len(unknown)}) -- check by hand:")
    for i, n, v, d in unknown:
        print(f"  line {i:>4}  {n:<20} = {v!r}   default {d!r}")
