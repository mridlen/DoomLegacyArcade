#!/usr/bin/env python3
"""Button presses that type a string on the Cabinet Link page's on-screen keyboard.

Used by tools/linktest.sh to drive the page through -linktest -linkkeys, the way a
person with only a stick and two buttons would.  The character table is read out
of m_menu.c (lkt_sets), not copied here, so a change to the layout changes the
presses rather than silently testing an old keyboard.

    linktest_kbd.py <kind> <text>      kind: name, passcode, master, port, allow

Prints space-separated tokens: u d l r (stick), f (fire), ending on DONE.  The
cursor model follows M_Link_Text_Move exactly; if the two ever disagree the test
types the wrong thing and fails, which is the point.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'svn1749', 'src', 'm_menu.c')

COLS = 13
ACTIONS = 5
SETS_FOR = {'name': 1, 'port': 1, 'master': 2, 'allow': 2, 'passcode': 3}


def c_string(body):
    """The value of a C string literal body (between the quotes)."""
    out, i = [], 0
    while i < len(body):
        ch = body[i]
        if ch == '\\':
            out.append(body[i + 1])
            i += 2
        else:
            out.append(ch)
            i += 1
    return ''.join(out)


def read_sets():
    text = open(SRC, encoding='utf-8').read()
    m = re.search(r'lkt_sets\[3\]\[3\]\s*=\s*\{(.*?)\n\};', text, re.S)
    if not m:
        sys.exit('linktest_kbd.py: lkt_sets not found in m_menu.c')
    rows = re.findall(r'\{\s*((?:"(?:[^"\\]|\\.)*"\s*,?\s*){3})\}', m.group(1))
    sets = []
    for r in rows:
        strs = [c_string(s) for s in re.findall(r'"((?:[^"\\]|\\.)*)"', r)]
        assert len(strs) == 3 and all(len(s) == COLS for s in strs), strs
        sets.append(strs)
    assert len(sets) == 3, sets
    return sets


class Keyboard:
    def __init__(self, sets, nsets):
        self.sets, self.nsets = sets, nsets
        self.set = self.line = self.col = 0
        self.out = []

    def move(self, dx, dy):
        # M_Link_Text_Move
        if dy:
            frm = self.line
            self.line = (self.line + dy + 4) % 4
            if frm < 3 and self.line == 3:
                self.col = self.col * ACTIONS // COLS
            elif frm == 3 and self.line < 3:
                self.col = (self.col * COLS + COLS // 2) // ACTIONS
        if dx:
            width = COLS if self.line < 3 else ACTIONS
            self.col = (self.col + dx + width) % width
        if self.line == 3 and self.col == 0 and self.nsets == 1:
            self.col = ACTIONS - 1 if dx < 0 else 1
        self.out.append({(0, -1): 'u', (0, 1): 'd', (-1, 0): 'l', (1, 0): 'r'}[(dx, dy)])

    def goto(self, line, col):
        while self.line != line:
            self.move(0, 1)
        width = COLS if line < 3 else ACTIONS
        right = (col - self.col) % width
        left = (self.col - col) % width
        while self.col != col:
            self.move(1, 0) if right <= left else self.move(-1, 0)

    def press(self):
        self.out.append('f')
        if self.line == 3 and self.col == 0:
            self.set = (self.set + 1) % self.nsets

    def type(self, ch):
        if ch == ' ':
            self.goto(3, 1)
            self.press()
            return
        for _ in range(self.nsets):
            for line in range(3):
                col = self.sets[self.set][line].find(ch)
                if col >= 0:
                    self.goto(line, col)
                    self.press()
                    return
            if self.nsets == 1:
                break
            self.goto(3, 0)
            self.press()
        sys.exit('linktest_kbd.py: cannot type %r' % ch)

    def done(self):
        self.goto(3, 4)
        self.press()


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in SETS_FOR:
        sys.exit(__doc__)
    kb = Keyboard(read_sets(), SETS_FOR[sys.argv[1]])
    for ch in sys.argv[2]:
        kb.type(ch)
    kb.done()
    print(' '.join(kb.out))


if __name__ == '__main__':
    main()
