#!/usr/bin/env python3
"""Every CV_PossibleValue_t list, checked for the invariant CV_ValueIncDec needs.

CV_ValueIncDec (command.c) is what the menu's left/right arrows and the console
"Toggle" command call.  For a plain list of values it finds the current entry by
scanning for a matching value and keeping the LAST match, then steps by index.
Its own comment says the precondition:

    // this code do not support more than same value for differant PossibleValue

Nothing enforced it.  cv_winningbanner shipped with a duplicate {1,"On"} kept
for config compatibility: the cvar set, read and displayed perfectly, and the
menu arrows stepped from the wrong index -- right off Rainbow went to Off, left
went to Cycle, and right from Cycle landed on the duplicate.  A check that only
loads and prints a value cannot see this; it is positional.

Bounded lists (first entry "MIN") are ranges, not lists of choices, and take the
other branch of CV_ValueIncDec entirely -- they are reported and skipped.

    tools/cvarlist-test.py [--selfcheck]
"""
import os, re, sys

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'svn1749', 'src')
DECL = re.compile(r'CV_PossibleValue_t\s+(\w+)\s*\[\s*\]\s*=\s*', re.S)
ENTRY = re.compile(r'\{\s*([^,{}]+?)\s*,\s*(NULL|"(?:[^"\\]|\\.)*")\s*\}')


def tables(path, text):
    """Every CV_PossibleValue_t initialiser in one file, by brace matching."""
    for m in DECL.finditer(text):
        i = text.find('{', m.end())
        if i < 0:
            continue
        depth, j = 0, i
        while j < len(text):
            if text[j] == '{':
                depth += 1
            elif text[j] == '}':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        body = text[i:j + 1]
        entries = [(v.strip(), s) for v, s in ENTRY.findall(body) if s != 'NULL']
        yield m.group(1), entries, text.count('\n', 0, m.start()) + 1, path


def collect():
    out = []
    for root, _, files in os.walk(SRC):
        for f in sorted(files):
            if not f.endswith(('.c', '.h')):
                continue
            p = os.path.join(root, f)
            try:
                t = open(p, encoding='utf-8').read()
            except (OSError, UnicodeDecodeError):
                continue
            out += list(tables(p, t))
    return out


# Known and accepted.  exmy_cons_t (m_menu.c) lists e1m1..e5m9 and episodes 4
# and 5 genuinely share values 41..49.  It is not reachable through this bug:
# its two cvars (cv_nextepmap, cv_dm_nextepmap) are CV_HIDEN, so no menu row
# steps them, and M_Configure truncates the list at runtime to the episodes the
# IWAD actually has.  Listed here so this check stays a usable green gate; if
# either cvar ever becomes visible on a page, remove this and fix the table.
ALLOW = {'exmy_cons_t'}


def check(found, quiet=False):
    bad, skipped, checked, allowed = [], 0, 0, []
    for name, entries, line, path in found:
        if not entries:
            continue
        if entries[0][1].strip('"').upper() == 'MIN':
            skipped += 1
            continue
        if name in ALLOW:
            allowed.append(name)
            continue
        checked += 1
        seen = {}
        for val, s in entries:
            seen.setdefault(val, []).append(s)
        dups = {v: ss for v, ss in seen.items() if len(ss) > 1}
        if dups:
            bad.append((name, os.path.relpath(path, SRC), line, dups))
    if not quiet:
        for name, rel, line, dups in bad:
            for v, ss in sorted(dups.items()):
                print('FAIL %-24s %s:%d  value %s used by %s'
                      % (name, rel, line, v, ' and '.join(ss)))
        print('\n%d value list(s) checked, %d bounded MIN..MAX list(s) skipped, '
              '%d allowed (%s), %d with a repeated value'
              % (checked, skipped, len(allowed),
                 ', '.join(allowed) or '-', len(bad)))
    return bad


def main():
    found = collect()
    if '--selfcheck' in sys.argv:
        # Reinstate the bug: give the first clean list a duplicate of its own
        # first value, and confirm the check goes red.  A check never shown to
        # fail is not evidence.
        base = check(found, quiet=True)
        victim = None
        for name, entries, line, path in found:
            if len(entries) >= 2 and entries[0][1].strip('"').upper() != 'MIN':
                victim = (name, entries, line, path)
                break
        if not victim:
            print('selfcheck: no suitable list found')
            return 1
        name, entries, line, path = victim
        salted = [t for t in found if t[0] != name]
        salted.append((name, entries + [(entries[0][0], '"DUPLICATE"')], line, path))
        after = check(salted, quiet=True)
        ok = len(after) == len(base) + 1
        print('selfcheck: clean=%d, with an injected duplicate in %s=%d -> %s'
              % (len(base), name, len(after), 'the check goes red' if ok
                 else 'THE CHECK DID NOT NOTICE'))
        return 0 if ok else 1
    return 1 if check(found) else 0


if __name__ == '__main__':
    sys.exit(main())
