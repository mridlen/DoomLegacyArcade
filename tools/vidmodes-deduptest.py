#!/usr/bin/env python3
"""Check add_vid_mode's deduplication against this display's real mode list.

The thing that could go wrong is not "does it remove duplicates" but "does it
remove anything else" -- a resolution silently lost from the menu is exactly
the bug being fixed, so prove the set of distinct sizes is unchanged.

As with tools/vidmenu-navtest.py the function is extracted from i_video.c by
brace matching rather than copied, so this tests the shipped text.

Run it from anywhere:  tools/vidmodes-deduptest.py
"""
import re, subprocess, sys, os, tempfile, shutil, ctypes, ctypes.util

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, os.pardir, 'svn1749', 'src', 'sdl', 'i_video.c')
TMP = tempfile.mkdtemp(prefix='vidmodes-deduptest.') + os.sep
s = open(SRC, encoding='latin-1').read()

i = s.index('void  add_vid_mode( int w, int h )')
j = s.index('{', i)
depth, k = 0, j
while True:
    if s[k] == '{':
        depth += 1
    elif s[k] == '}':
        depth -= 1
        if depth == 0:
            break
    k += 1
add_vid_mode = s[i:k + 1]

# The engine's size bound, from screen.h rather than restated here.
scr = open(os.path.join(HERE, os.pardir, 'svn1749', 'src', 'screen.h'),
           encoding='latin-1').read()
MAXVIDWIDTH = int(re.search(r'#define MAXVIDWIDTH\s+(\d+)', scr).group(1))
MAXVIDHEIGHT = int(re.search(r'#define MAXVIDHEIGHT\s+(\d+)', scr).group(1))

# The real modes this display advertises, through the same bpp/size filter the
# engine applies, duplicates and all.  A machine with no display still gets the
# synthetic case below, which is the one the change is really for.
sdl = ctypes.CDLL(ctypes.util.find_library('SDL2'))
sdl.SDL_Init(0x20)


class DisplayMode(ctypes.Structure):
    _fields_ = [('format', ctypes.c_uint32), ('w', ctypes.c_int),
                ('h', ctypes.c_int), ('refresh_rate', ctypes.c_int),
                ('driverdata', ctypes.c_void_p)]


def bpp(fmt):
    return (fmt >> 8) & 0xFF


dm = DisplayMode()
sdl.SDL_GetDesktopDisplayMode(0, ctypes.byref(dm))
native = bpp(dm.format)
raw = []
for n in range(sdl.SDL_GetNumDisplayModes(0)):
    if sdl.SDL_GetDisplayMode(0, n, ctypes.byref(dm)) < 0:
        continue
    if bpp(dm.format) == native and dm.w <= MAXVIDWIDTH and dm.h <= MAXVIDHEIGHT:
        raw.append((dm.w, dm.h))
sdl.SDL_Quit()

# Plus a synthetic display with several refresh rates, which is the case the
# change is actually for -- this laptop only has one duplicate.
synthetic = [(w, h) for _ in range(4)
             for (w, h) in [(1920, 1080), (1600, 1200), (1280, 1024), (1280, 720),
                            (1024, 768), (800, 600), (720, 480), (640, 480),
                            (640, 400), (640, 350)]]

cases = {'synthetic 4 refresh rates': synthetic}
if raw:
    cases['this display'] = raw
else:
    print('no display modes available here; synthetic case only')

harness = r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
typedef struct { uint16_t w, h; } vid_mode_t;
static vid_mode_t * vid_modelist = NULL;
static int num_vid_mode_allocated = 0;
static int num_vid_mode = 0;
%s
int main(int argc, char **argv)
{
    int i;
    for( i = 1; i + 1 < argc; i += 2 )
        add_vid_mode( atoi(argv[i]), atoi(argv[i+1]) );
    printf("%%d\n", num_vid_mode);
    for( i = 0; i < num_vid_mode; i++ )
        printf("%%dx%%d\n", vid_modelist[i].w, vid_modelist[i].h);
    return 0;
}
''' % add_vid_mode

open(TMP + 'deduptest.c', 'w').write(harness)
r = subprocess.run(['gcc', '-Wall', '-O1', '-o', TMP + 'deduptest', TMP + 'deduptest.c'],
                   capture_output=True, text=True)
if r.returncode:
    shutil.rmtree(TMP, ignore_errors=True)
    sys.exit(r.stdout + r.stderr)
if r.stderr.strip():
    print('compiler warnings:\n' + r.stderr)

bad = 0
for name, modes in cases.items():
    args = [TMP + 'deduptest']
    for w, h in modes:
        args += [str(w), str(h)]
    out = subprocess.run(args, capture_output=True, text=True).stdout.split()
    kept = out[1:]
    want = []
    for w, h in modes:                      # first occurrence, order preserved
        d = '%dx%d' % (w, h)
        if d not in want:
            want.append(d)
    ok = (kept == want)
    bad += not ok
    print('%-28s %2d in -> %2d out, %2d distinct  %s'
          % (name, len(modes), len(kept), len(want), 'OK' if ok else '*** MISMATCH ***'))
    if not ok:
        print('   kept: %s\n   want: %s' % (kept, want))

shutil.rmtree(TMP, ignore_errors=True)
print()
print('every distinct size survives, in first-seen order' if not bad else 'FAILED')
sys.exit(1 if bad else 0)
