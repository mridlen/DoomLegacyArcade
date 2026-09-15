#!/usr/bin/env python3
"""Test the Windows restart's command-line quoting, without Windows.

A restart on Windows (M_Restart_Windows, m_menu.c) cannot hand the new process
an argv: Windows passes one command line, and the new process splits it again
by the rules CommandLineToArgvW documents.  M_Restart_Quote_Arg builds that
line.  If it gets a rule wrong, an argument comes back different -- a level
pack under "C:\\Users\\...\\My Games\\" arrives as two nonsense -file names, and
the restart silently drops the pack.  Nothing about that is visible on Linux,
and the Windows binary has never been played.

So extract M_Restart_Quote_Arg from m_menu.c VERBATIM, by brace matching,
compile it natively (wchar_t is 4 bytes here and 2 on Windows; the function
does not care), quote a list of awkward arguments plus a few thousand random
ones, and split the result back with a Python implementation of the documented
parsing rules.  Every argument must come back exactly.  A copied test drifts
away from the code and then passes forever; an extracted one tests the text
that ships.

Run it from anywhere:  tools/restartquote-test.py
  --selfcheck   reinstate each quoting bug the test claims to catch, and
                report whether each one actually goes red.
"""
import random, subprocess, sys, os, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
MENU = os.path.join(HERE, os.pardir, 'svn1749', 'src', 'm_menu.c')

menu_s = open(MENU, encoding='utf-8').read()


def extract(text, sig):
    """Return the whole function text starting at the line holding sig, plus
    any #define/#undef lines inside it (they are part of the text)."""
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


SIG = 'static boolean M_Restart_Quote_Arg('
func = extract(menu_s, SIG)
print('extracted %d lines of M_Restart_Quote_Arg' % func.count('\n'))


HARNESS = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef int boolean;
#define false 0
#define true 1

%(func)s

/* stdin: one argument per line, as hex UTF-32 code points separated by
   spaces (a line holding only "-" is the empty argument); a line "END" ends
   one argument list.  stdout: one line per list, the command line the same
   way, or FULL. */
int main(void)
{
    enum { SIZE = 32768 };
    static wchar_t line[SIZE];
    static wchar_t arg[4096];
    static char buf[65536];
    int pos = 0, full = 0;
    line[0] = 0;
    while( fgets( buf, sizeof(buf), stdin ) )
    {
        int n = 0;
        char * p = buf, * end;
        if( ! strncmp( buf, "END", 3 ) )
        {
            if( full )  puts( "FULL" );
            else
            {
                for( int i = 0; line[i]; i++ )  printf( "%%lx ", (unsigned long) line[i] );
                putchar( '\n' );
            }
            pos = 0;  full = 0;  line[0] = 0;
            continue;
        }
        if( full )  continue;
        if( buf[0] != '-' )
        {
            for( ;; )
            {
                unsigned long v = strtoul( p, &end, 16 );
                if( end == p )  break;
                arg[n++] = (wchar_t) v;
                p = end;
            }
        }
        arg[n] = 0;
        if( ! M_Restart_Quote_Arg( line, &pos, SIZE, arg ) )  full = 1;
    }
    return 0;
}
'''


def split_cmdline(s):
    """Split a command line the way the Microsoft C runtime does for
    argv[1..] (CommandLineToArgvW, and the 2008+ CRT's rule that "" inside a
    quoted run is a literal quote).  argv[0] is split by the simpler rule
    those functions use for the program name: no backslash escapes, a quote
    only toggles quoting."""
    args, i, n = [], 0, len(s)
    # argv[0]
    cur, inq = '', False
    while i < n and (inq or s[i] not in ' \t'):
        if s[i] == '"':
            inq = not inq
        else:
            cur += s[i]
        i += 1
    args.append(cur)
    while True:
        while i < n and s[i] in ' \t':
            i += 1
        if i >= n:
            break
        cur, inq = '', False
        while i < n:
            c = s[i]
            if c == '\\':
                j = i
                while j < n and s[j] == '\\':
                    j += 1
                nb = j - i
                if j < n and s[j] == '"':
                    cur += '\\' * (nb // 2)
                    if nb % 2:
                        cur += '"'
                        i = j + 1
                    else:
                        i = j      # the quote is handled below
                else:
                    cur += '\\' * nb
                    i = j
                continue
            if c == '"':
                if inq and i + 1 < n and s[i + 1] == '"':
                    cur += '"'
                    i += 2
                    continue
                inq = not inq
                i += 1
                continue
            if c in ' \t' and not inq:
                break
            cur += c
            i += 1
        args.append(cur)
    return args


CASES = [
    r'C:\Program Files\Doom Legacy Arcade\doomlegacyarcade.exe',   # argv[0]
    '-game', 'doom2',
    '-file', r'C:\Users\Mark\My Games\packs\scythe 2.wad',
    '',                                  # the empty argument
    r'C:\My Games\\',                    # trailing backslash inside quotes
    'trailing\\',                        # trailing backslash, no quotes needed
    r'a\\b\\\c',                         # backslashes not before a quote
    'say "hello"',                       # quotes and a space
    'a"b',                               # a quote and no space
    '\\"',                               # backslash then quote
    '\\\\"x y',                          # two backslashes then quote
    '"',                                 # a lone quote
    '\\',                                # a lone backslash
    'tab\there',
    'J\u00f6rg \u2603 packs',            # outside the local code page
    '-linkselected',
]


def run_many(binary, lists):
    """Quote each argument list; return each command line, or None if full."""
    inp = []
    for args in lists:
        for a in args:
            inp.append(' '.join('%x' % ord(ch) for ch in a) if a else '-')
        inp.append('END')
    out = subprocess.run([binary], input='\n'.join(inp) + '\n', capture_output=True,
                         text=True).stdout.split('\n')
    res = []
    for o in out[:len(lists)]:
        o = o.strip()
        res.append(None if o == 'FULL' else ''.join(chr(int(h, 16)) for h in o.split()))
    return res


def build(func_text, tmp, name):
    src = os.path.join(tmp, name + '.c')
    exe = os.path.join(tmp, name)
    open(src, 'w').write(HARNESS % {'func': func_text})
    r = subprocess.run(['cc', '-std=gnu17', '-Wall', '-o', exe, src], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr)
        raise SystemExit('harness did not compile')
    return exe


def check(binary, verbose):
    fails = 0
    rng = random.Random(1749)
    alphabet = ['a', 'b', ' ', '\t', '\\', '\\', '"', '\u00e9']
    randoms = [[r'C:\x y\doom.exe'] + [
                   ''.join(rng.choice(alphabet) for _ in range(rng.randrange(0, 9)))
                   for _ in range(rng.randrange(1, 5))]
               for _ in range(3000)]
    lists = [CASES] + randoms + [['x' * 4000] * 9]
    lines = run_many(binary, lists)
    for args, line in zip(lists[:-1], lines[:-1]):
        got = split_cmdline(line) if line is not None else None
        if got != args:
            fails += 1
            if verbose and fails <= 4:
                print('FAIL %r\n  line %r -> %r' % (args, line, got))
    # A line that does not fit must be refused, not truncated.
    if lines[-1] is not None:
        fails += 1
        if verbose:
            print('FAIL an over-long command line was not refused')
    return fails


# The parser above is the test's oracle, so it is checked first against the
# examples Microsoft publishes for these rules ("Parsing C++ command-line
# arguments"), each after a program name.
MS_EXAMPLES = [
    (r'"abc" d e',          ['abc', 'd', 'e']),
    (r'a\\b d"e f"g h',     [r'a\\b', 'de fg', 'h']),
    (r'a\\\"b c d',         [r'a\"b', 'c', 'd']),
    (r'a\\\\"b c" d e',     [r'a\\b c', 'd', 'e']),
    (r'a"b"" c d',          ['ab" c d']),
]
for text, want in MS_EXAMPLES:
    got = split_cmdline('prog ' + text)[1:]
    if got != want:
        raise SystemExit('the parser disagrees with Microsoft on %r: %r, not %r' % (text, got, want))


# Each bug the test claims to catch: (description, text to find, replacement).
MUTANTS = [
    ('backslashes before a quote not escaped', 'slashes * 2 + 1', 'slashes'),
    ('trailing backslashes not doubled', 'k < slashes * 2; k++ )  M_RESTART_PUT( L\'\\\\\' );\n                break;',
     'k < slashes; k++ )  M_RESTART_PUT( L\'\\\\\' );\n                break;'),
    ('a quote alone does not force quoting', 'L" \\t\\n\\v\\""', 'L" \\t\\n\\v"'),
    ('the empty argument is dropped', 'arg[0] && ', ''),
    ('no separator between arguments', "if( p > 0 )  M_RESTART_PUT( L' ' );", ''),
    ('the size limit is not enforced', 'if( p >= size - 1 ) return false;', ''),
]


def main():
    selfcheck = '--selfcheck' in sys.argv
    tmp = tempfile.mkdtemp(prefix='restartquote-')
    try:
        fails = check(build(func, tmp, 'real'), True)
        print('%s: M_Restart_Quote_Arg round-trips %d fixed and 3000 random argument lists'
              % ('ok' if fails == 0 else 'FAIL', len(CASES)))
        if not selfcheck:
            return 1 if fails else 0
        bad = 0
        for i, (desc, old, new) in enumerate(MUTANTS):
            if func.count(old) != 1:
                print('selfcheck: cannot place mutant "%s" (text found %d times)' % (desc, func.count(old)))
                bad += 1
                continue
            m = func.replace(old, new)
            try:
                red = check(build(m, tmp, 'mut%d' % i), False) > 0
            except Exception:
                red = True
            print('selfcheck: %-45s %s' % (desc, 'caught' if red else 'NOT CAUGHT'))
            bad += 0 if red else 1
        return 1 if (fails or bad) else 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


sys.exit(main())
