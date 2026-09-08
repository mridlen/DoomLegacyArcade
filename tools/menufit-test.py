#!/usr/bin/env python3
#
# [Arcade] Where do a menu page's rows actually land?
#
# m_menu.c lays a generic page out from menu_t.y, one STRINGHEIGHT per
# ordinary row, and nothing anywhere warns when that runs past the bottom of
# the 200-line screen or when an IT_YOFFSET row lands on top of an ordinary
# one.  A page in either state looks complete: the row is simply not drawn, or
# two rows are drawn over each other, and the cursor still moves onto them.
# docs/arcade/menus.md has the history -- Video Options sat with an invisible
# OpenGL link for some time.
#
# So this measures it instead of trusting arithmetic done by hand.  The menu
# arrays are lifted verbatim out of m_menu.c by brace matching, the way
# tools/vidmenu-navtest.py and friends lift the functions they test: a copied
# table drifts away from the source and then passes forever, an extracted one
# measures the text that ships.
#
# Usage:
#   tools/menufit-test.py                     check every generic page
#   tools/menufit-test.py EffectsOption1Menu  check one page, and show it
#   tools/menufit-test.py --selfcheck         prove the checks can go red
#
# The row-advance rules below mirror M_DrawGenericMenu (m_menu.c).  The trap
# when doing this by hand is that IT_CV_SLIDER rows advance by STRINGHEIGHT
# like any other -- the y+=16 in that drawer belongs to the IT_CV_STRING
# text-entry branch, not the slider.

import os
import re
import sys

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   '..', 'svn1749', 'src')

SCREEN_HEIGHT = 200
STRINGHEIGHT = 10
LINEHEIGHT = 16
SMALLLINEHEIGHT = 8
FONTBHEIGHT = 20
GLYPH_HEIGHT = 7        # hu_font glyphs, see docs/arcade/high-scores.md
TEXTENTRY_EXTRA = 16    # IT_CV_STRING draws its box below the label


def read(name):
    with open(os.path.join(SRC, name), encoding='utf-8', errors='surrogateescape') as f:
        return f.read()


def brace_block(text, start):
    """Return the {...} block that starts at or after `start`, by brace matching."""
    i = text.index('{', start)
    depth = 0
    for j in range(i, len(text)):
        if text[j] == '{':
            depth += 1
        elif text[j] == '}':
            depth -= 1
            if depth == 0:
                return text[i + 1:j]
    raise ValueError('unterminated block')


def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
    s = re.sub(r'//[^\n]*', '', s)
    return s


# ---------------------------------------------------------------- preprocessor
#
# The arrays carry #ifdef rows, and which ones are compiled decides how tall
# the page is -- that is exactly what the lockdown note in menus.md warns
# about.  Resolve them from the real headers rather than assuming.

def defined_symbols():
    """Symbols #define'd in doomdef.h or m_menu.c, as far as this needs them."""
    syms = set()
    for fn in ('doomdef.h', 'm_menu.c'):
        for m in re.finditer(r'^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)',
                             read(fn), re.M):
            syms.add(m.group(1))
    # Set by the Makefile / make_options for this cabinet's build.
    syms.update({'HWRENDER', 'SMIF_SDL', 'LINUX', 'SDL2', 'HAVE_MIXER'})
    return syms


def resolve_ifdefs(block, syms):
    """Keep only the lines that this build compiles.  Nesting is shallow here."""
    out = []
    stack = []   # list of bools: is the current branch live?
    for line in block.split('\n'):
        st = line.strip()
        m = re.match(r'#\s*if(n?)def\s+([A-Za-z_][A-Za-z0-9_]*)', st)
        if m:
            live = (m.group(2) in syms) != bool(m.group(1))
            stack.append(live)
            continue
        if re.match(r'#\s*else', st):
            if stack:
                stack[-1] = not stack[-1]
            continue
        if re.match(r'#\s*endif', st):
            if stack:
                stack.pop()
            continue
        if re.match(r'#\s*if\b', st):
            stack.append(False)   # conservative: plain #if not evaluated
            continue
        if all(stack):
            out.append(line)
    return '\n'.join(out)


# ------------------------------------------------------------------- the pages

def split_items(block):
    """Split an initialiser list into its top-level {...} items."""
    items = []
    depth = 0
    cur = ''
    for ch in block:
        if ch == '{':
            depth += 1
            if depth == 1:
                cur = ''
                continue
        elif ch == '}':
            depth -= 1
            if depth == 0:
                items.append(cur)
                continue
        if depth >= 1:
            cur += ch
    return items


def parse_item(raw):
    """Pull the status flags, the label and the alphaKey out of one row."""
    fields = []
    depth = 0
    cur = ''
    instr = False
    for ch in raw:
        if instr:
            cur += ch
            if ch == '"':
                instr = False
            continue
        if ch == '"':
            instr = True
            cur += ch
        elif ch == '(':
            depth += 1
            cur += ch
        elif ch == ')':
            depth -= 1
            cur += ch
        elif ch == ',' and depth == 0:
            fields.append(cur.strip())
            cur = ''
        else:
            cur += ch
    fields.append(cur.strip())
    status = fields[0] if fields else ''
    label = ''
    for f in fields:
        m = re.search(r'"((?:[^"\\]|\\.)*)"', f)
        if m and not label:
            label = m.group(1)
    alpha = 0
    if len(fields) >= 4:
        m = re.match(r"^(\d+)$", fields[-1])
        if m:
            alpha = int(m.group(1))
        else:
            m = re.match(r"^'(.)'$", fields[-1])
            if m:
                alpha = ord(m.group(1))
            else:
                m = re.match(r'^([A-Z_][A-Z0-9_]*)\s*\+\s*(\d+)$', fields[-1])
                if m:
                    alpha = None   # symbolic; caller reports it as unknown
                elif re.match(r'^[A-Za-z_]', fields[-1]):
                    alpha = None
    return {'status': status, 'label': label, 'alpha': alpha, 'raw': raw}


def advance_and_extra(status):
    """(y advance, extra height drawn below y) for one row, per M_DrawGenericMenu."""
    if 'IT_NODRAW' in status or 'IT_HIDDEN' in status:
        return 0, 0
    if 'IT_STRING2' in status or 'IT_DYLITLSPACE' in status:
        return SMALLLINEHEIGHT, GLYPH_HEIGHT
    if 'IT_PATCH' in status or 'IT_GRAYPATCH' in status:
        return LINEHEIGHT, FONTBHEIGHT
    if 'IT_NOTHING' in status or 'IT_EXTERNAL' in status \
            or 'IT_DYBIGSPACE' in status or 'IT_BIGSLIDER' in status:
        return LINEHEIGHT, LINEHEIGHT
    # IT_STRING / IT_WHITESTRING, the ordinary row
    if 'IT_CV_STRING' in status:
        return STRINGHEIGHT + TEXTENTRY_EXTRA, TEXTENTRY_EXTRA + GLYPH_HEIGHT
    return STRINGHEIGHT, GLYPH_HEIGHT


def find_pages(text):
    """menuitem_t arrays paired with the menu_t that draws them generically."""
    arrays = {}
    for m in re.finditer(r'menuitem_t\s+(\w+)\s*\[\s*\]\s*=', text):
        arrays[m.group(1)] = m.end()
    pages = []
    for m in re.finditer(r'menu_t\s+(\w+)\s*=', text):
        body = strip_comments(brace_block(text, m.end()))
        fields = [f.strip() for f in body.split(',')]
        if len(fields) < 7:
            continue
        items_name = fields[2]
        if items_name not in arrays:
            continue
        if 'M_DrawGenericMenu' not in body:
            continue
        try:
            x = int(fields[6])
            y = int(fields[7])
        except (ValueError, IndexError):
            continue
        pages.append({'def': m.group(1), 'items': items_name,
                      'y': y, 'x': x, 'at': arrays[items_name]})
    return pages


def layout(text, page, syms, override=None):
    block = resolve_ifdefs(strip_comments(brace_block(text, page['at'])), syms)
    items = [parse_item(r) for r in split_items(block)]
    if override:
        items = override(items)
    y = page['y']
    rows = []
    for it in items:
        if ('IT_YOFFSET' in it['status']) and it['alpha']:
            y = page['y'] + it['alpha']
        adv, extra = advance_and_extra(it['status'])
        if adv:
            rows.append({'y': y, 'bottom': y + extra, 'label': it['label'],
                         'unknown_offset': ('IT_YOFFSET' in it['status'])
                                          and it['alpha'] is None})
        y += adv
    return rows


def check(rows):
    problems = []
    seen = {}
    for r in rows:
        if r['unknown_offset']:
            problems.append('row "%s" has a symbolic IT_YOFFSET; not measured'
                            % r['label'])
            continue
        if r['bottom'] > SCREEN_HEIGHT:
            problems.append('row "%s" at y=%d runs to %d, past the %d-line screen'
                            % (r['label'], r['y'], r['bottom'], SCREEN_HEIGHT))
        if r['y'] in seen:
            problems.append('row "%s" shares y=%d with "%s"'
                            % (r['label'], r['y'], seen[r['y']]))
        else:
            seen[r['y']] = r['label']
    return problems


def main():
    args = sys.argv[1:]
    selfcheck = '--selfcheck' in args
    args = [a for a in args if not a.startswith('--')]

    text = read('m_menu.c')
    syms = defined_symbols()
    pages = find_pages(text)
    wanted = args or None

    failed = 0
    shown = 0
    for page in pages:
        if wanted and page['items'] not in wanted and page['def'] not in wanted:
            continue
        shown += 1
        rows = layout(text, page, syms)
        problems = check(rows)
        last = max((r['bottom'] for r in rows), default=page['y'])
        slack = (SCREEN_HEIGHT - last) // STRINGHEIGHT
        status = 'FAIL' if problems else 'ok  '
        print('%s %-24s %2d rows, y %d..%d, room for %d more'
              % (status, page['items'], len(rows), page['y'], last, slack))
        if wanted:
            for r in rows:
                print('        y=%3d  %s' % (r['y'], r['label']))
        for p in problems:
            print('        %s' % p)
        if problems:
            failed += 1

    print('\n%d generic page(s) measured, %d with problems' % (shown, failed))

    if selfcheck:
        # A check never shown to fail is not evidence.  Reinstate both bugs on
        # the page this was written for and confirm each one goes red.
        print('\n-- selfcheck: reinstating the bugs this is meant to catch --')
        page = next(p for p in pages if p['items'] == 'EffectsOption1Menu')
        ok = True

        def pile_on_rows(items):
            # Drag "Next" up onto the last ordinary row: a y collision.
            out = list(items)
            for it in out:
                if 'IT_YOFFSET' in it['status'] and it['alpha']:
                    it['alpha'] = 110
            return out

        def overflow(items):
            # Twenty more ordinary rows: straight off the bottom.
            extra = dict(items[1])
            extra = {'status': 'IT_STRING | IT_CVAR', 'label': 'overflow',
                     'alpha': 0, 'raw': ''}
            return list(items) + [dict(extra) for _ in range(20)]

        for name, mut, want in (('y collision', pile_on_rows, 'shares y'),
                                ('off the bottom', overflow, 'past the')):
            probs = check(layout(text, page, syms, override=mut))
            hit = any(want in p for p in probs)
            print('   %-16s -> %s' % (name, 'caught' if hit else 'NOT CAUGHT'))
            if not hit:
                ok = False
        if not ok:
            return 2

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
