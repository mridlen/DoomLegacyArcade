#!/usr/bin/env python3
"""Test Menu Select / Menu Back -- which button selects and which backs out in
a menu, per panel -- and the "Control per key" exemption that lets them share
buttons with the game actions, without a screen.

Nothing drives the menus headlessly, and the cases that matter are mixtures no
single-pad test reaches: an arcade panel beside a gamepad, a pad with only one
of the two bound, a button that is Use in play and Select in a menu.  So
extract the real code VERBATIM by brace matching and drive it:

  g_input.c  G_Menu_Is_Control, G_Control_Bound, G_Any_Panel_Bound,
             G_Menu_Select_Control, G_Menu_Back_Control, G_CheckDoubleUsage,
             setcontrol (the config loader, with COM_Args stubbed), and the
             gamecontrolname[] table
  g_input.h  the gamecontrols_e enum
  m_menu.c   M_key_is_control, M_Cabinet_Menu_Key

Run it from anywhere:  tools/menukeys-test.py
  --selfcheck   reinstate each bug the checks claim to catch and report
                whether each one goes red.
"""
import os, re, subprocess, sys, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, os.pardir, 'svn1749', 'src')


def read(name):
    return open(os.path.join(SRC, name), encoding='utf-8').read()


def extract(s, sig):
    """The definition starting at sig -- the one followed by a '{', so a
    forward declaration of the same name earlier in the file is skipped."""
    m = re.search(re.escape(sig) + r'\s*\{', s)
    if not m:
        raise SystemExit('menukeys-test: cannot find definition: ' + sig)
    i, j = m.start(), m.end() - 1
    depth, k = 0, j
    while True:
        if s[k] == '{':
            depth += 1
        elif s[k] == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    return s[i:k + 1] + '\n'


def between(s, start, end):
    i = s.index(start)
    j = s.index(end, i) + len(end)
    return s[i:j] + '\n'


g_input_c, g_input_h, m_menu_c = read('g_input.c'), read('g_input.h'), read('m_menu.c')

ENUM = between(g_input_h, 'typedef enum\n{\n    gc_null', '} gamecontrols_e;')
NAMES = between(g_input_c, 'char * gamecontrolname[num_gamecontrols] =', '};')
GFUNCS = ''.join(extract(g_input_c, sig) for sig in (
    'boolean  G_Menu_Is_Control( int gc )',
    'boolean  G_Control_Bound( int pind, int gc )',
    'boolean  G_Any_Panel_Bound( int gc )',
    'int  G_Menu_Select_Control( int pind )',
    'int  G_Menu_Back_Control( int pind )',
    'void G_CheckDoubleUsage(int keynum, int control)',
    'void setcontrol(int (*gc)[2], const char * cstr)',
))
MFUNCS = ''.join(extract(m_menu_c, sig) for sig in (
    'boolean  M_key_is_control( uint16_t key, int gcnum )',
    'uint16_t  M_Cabinet_Menu_Key( uint16_t key )',
))

# The join screen reads raw keys per panel and must ask the same question
# M_Cabinet_Menu_Key does.  Extracting M_Join_Key would drag in the whole join
# state machine, so check it structurally: it must not name fire or use itself.
JOIN = extract(m_menu_c, 'boolean  M_Join_Key( uint16_t key )')

HARNESS = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <strings.h>

typedef unsigned char byte;
typedef int boolean;
#define true 1
#define false 0
#define static
#define ENABLE_COME_HERE
#define MAXSPLITSCREENPLAYERS 4

/* Menu keys: the values only have to be distinct from the buttons below. */
enum { KEY_NULL = 0, KEY_ENTER = 13, KEY_ESCAPE = 27,
       KEY_UPARROW = 1001, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW };

/* Buttons: an arcade panel's keys, and one gamepad's. */
/* Arcade keys are K_* + 20 * panel, so no two panels share a code. */
enum { K_FIRE = 200, K_USE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SL, K_SR,
       PAD_A = 600, PAD_B, PAD_X, PAD_Y, PAD_RT, PAD_LB, PAD_RB,
       PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_RSL, PAD_RSR };

@ENUM@

int gamecontrol_pl[MAXSPLITSCREENPLAYERS][num_gamecontrols][2];
#define gamecontrol    gamecontrol_pl[0]
#define gamecontrol2   gamecontrol_pl[1]

struct { int value; } cv_controlperkey = { 1 };   /* "One", the cabinet's */

typedef struct { byte num; char * arg[4]; } COM_args_t;
static COM_args_t  stub_args;
void  COM_Args( COM_args_t * c ) { *c = stub_args; }
int   G_KeyStringtoNum( char * s ) { return atoi( s ); }
void  CONS_Printf( const char * f, ... ) { (void) f; }

@NAMES@

boolean  G_Menu_Is_Control( int gc );
boolean  G_Control_Bound( int pind, int gc );
int  G_Menu_Select_Control( int pind );
int  G_Menu_Back_Control( int pind );

@GFUNCS@
@MFUNCS@

/* Load one config line the way config.cfg does: setcontrolN "name" "key". */
static void load( int pind, const char * name, int key )
{
    static char a0[] = "setcontrol", a1[32], a2[16];
    snprintf( a1, sizeof a1, "%s", name );
    snprintf( a2, sizeof a2, "%d", key );
    stub_args.num = 3;
    stub_args.arg[0] = a0;  stub_args.arg[1] = a1;  stub_args.arg[2] = a2;
    setcontrol( gamecontrol_pl[pind], "" );
}

static void reset( void ) { memset( gamecontrol_pl, 0, sizeof gamecontrol_pl ); }

static void arcade( int pind )
{
    int (*g)[2] = gamecontrol_pl[pind];
    int o = 20 * pind;
    g[gc_fire][0] = K_FIRE + o;   g[gc_use][0] = K_USE + o;
    g[gc_forward][0] = K_UP + o;  g[gc_backward][0] = K_DOWN + o;
    g[gc_turnleft][0] = K_LEFT + o;  g[gc_turnright][0] = K_RIGHT + o;
    g[gc_strafeleft][0] = K_SL + o;  g[gc_straferight][0] = K_SR + o;
}

/* The Xbox preset as G_Apply_Xbox_Preset lays it out (one pad, offset per
   joystick so two pads never share a code). */
static void pad( int pind, int joy, boolean with_menu )
{
    int (*g)[2] = gamecontrol_pl[pind], o = 100 * joy;
    g[gc_fire][0] = PAD_RT + o;  g[gc_use][0] = PAD_A + o;  g[gc_jump][0] = PAD_B + o;
    g[gc_forward][0] = PAD_UP + o;  g[gc_backward][0] = PAD_DOWN + o;
    g[gc_turnleft][0] = PAD_LEFT + o;  g[gc_turnright][0] = PAD_RIGHT + o;
    g[gc_strafeleft][0] = PAD_RSL + o;  g[gc_straferight][0] = PAD_RSR + o;
    g[gc_prevweapon][0] = PAD_LB + o;  g[gc_nextweapon][0] = PAD_RB + o;
    if( with_menu )
    {
        g[gc_menuselect][0] = PAD_A + o;
        g[gc_menuback][0] = PAD_B + o;
    }
}

static int fails = 0;
static void expect( const char * what, int got, int want )
{
    if( got != want )
    {
        printf( "FAIL  %s: got %d, want %d\n", what, got, want );
        fails++;
    }
}
#define MK(k) M_Cabinet_Menu_Key( (uint16_t)(k) )
static boolean has( int pind, int gc, int key )
{
    return gamecontrol_pl[pind][gc][0] == key || gamecontrol_pl[pind][gc][1] == key;
}

int main( void )
{
    int p;

    /* 1. Nothing bound anywhere: exactly the old behaviour, on every panel. */
    reset();
    for( p = 0; p < MAXSPLITSCREENPLAYERS; p++ )  arcade( p );
    for( p = 0; p < MAXSPLITSCREENPLAYERS; p++ )
    {
        expect( "arcade fire selects", MK(K_FIRE + 20*p), KEY_ENTER );
        expect( "arcade use backs out", MK(K_USE + 20*p), KEY_ESCAPE );
        expect( "arcade forward is up", MK(K_UP + 20*p), KEY_UPARROW );
        expect( "arcade backward is down", MK(K_DOWN + 20*p), KEY_DOWNARROW );
        expect( "arcade turn left", MK(K_LEFT + 20*p), KEY_LEFTARROW );
        expect( "arcade strafe right", MK(K_SR + 20*p), KEY_RIGHTARROW );
    }
    expect( "an unbound key passes through", MK('z'), 'z' );
    expect( "KEY_NULL passes through", MK(KEY_NULL), KEY_NULL );

    /* 2. A pad with the preset: A selects although it is also Use, B backs
       out although it is also Jump, and RT (fire) is swallowed rather than
       passed through raw. */
    reset();
    pad( 0, 0, true );
    expect( "pad A selects", MK(PAD_A), KEY_ENTER );
    expect( "pad B backs out", MK(PAD_B), KEY_ESCAPE );
    expect( "pad RT (fire) is swallowed", MK(PAD_RT), KEY_NULL );
    expect( "pad stick still moves", MK(PAD_UP), KEY_UPARROW );
    expect( "pad right stick still left", MK(PAD_RSL), KEY_LEFTARROW );
    expect( "pad X passes through", MK(PAD_X), PAD_X );

    /* 3. The same pad *without* Menu Select/Back (assigned before this
       feature): fire and use, as it always was. */
    reset();
    pad( 0, 0, false );
    expect( "old pad RT selects", MK(PAD_RT), KEY_ENTER );
    expect( "old pad A backs out", MK(PAD_A), KEY_ESCAPE );

    /* 4. Only Menu Select bound: fire is replaced, use is not. */
    reset();
    pad( 0, 0, false );
    gamecontrol_pl[0][gc_menuselect][0] = PAD_X;
    expect( "select-only: X selects", MK(PAD_X), KEY_ENTER );
    expect( "select-only: RT swallowed", MK(PAD_RT), KEY_NULL );
    expect( "select-only: use still backs out", MK(PAD_A), KEY_ESCAPE );
    /* and only Menu Back bound */
    reset();
    pad( 0, 0, false );
    gamecontrol_pl[0][gc_menuback][0] = PAD_Y;
    expect( "back-only: Y backs out", MK(PAD_Y), KEY_ESCAPE );
    expect( "back-only: A (use) swallowed", MK(PAD_A), KEY_NULL );
    expect( "back-only: fire still selects", MK(PAD_RT), KEY_ENTER );

    /* 5. Mixed cabinet: arcade panels 1 and 2, pads on 3 and 4.  Replacement
       is per panel -- the pads binding Menu Select must not stop an arcade
       panel's fire from selecting. */
    reset();
    arcade( 0 );  arcade( 1 );  pad( 2, 2, true );  pad( 3, 3, true );
    expect( "mixed: panel 1 fire selects", MK(K_FIRE), KEY_ENTER );
    expect( "mixed: panel 2 use backs out", MK(K_USE + 20), KEY_ESCAPE );
    expect( "mixed: panel 3 A selects", MK(PAD_A + 200), KEY_ENTER );
    expect( "mixed: panel 4 RT swallowed", MK(PAD_RT + 300), KEY_NULL );
    expect( "mixed: panel 4 B backs out", MK(PAD_B + 300), KEY_ESCAPE );

    /* 6. Control per key, through the real config loader.  config.cfg is
       written in enum order, so "use" loads before "menuselect": a one-way
       exemption would strip A off Use at every start. */
    reset();
    load( 0, "use", PAD_A );
    load( 0, "jump", PAD_B );
    load( 0, "menuselect", PAD_A );
    load( 0, "menuback", PAD_B );
    expect( "load order: A kept on Use", has( 0, gc_use, PAD_A ), 1 );
    expect( "load order: A on Menu Select", has( 0, gc_menuselect, PAD_A ), 1 );
    expect( "load order: B kept on Jump", has( 0, gc_jump, PAD_B ), 1 );
    expect( "load order: B on Menu Back", has( 0, gc_menuback, PAD_B ), 1 );
    /* the other direction: binding a game action keeps the menu button */
    load( 0, "fire", PAD_A );
    expect( "game rebind: A kept on Menu Select", has( 0, gc_menuselect, PAD_A ), 1 );
    expect( "game rebind: A taken off Use (still one per key)", has( 0, gc_use, PAD_A ), 0 );
    /* within the menu pair the rule still holds */
    load( 0, "menuback", PAD_A );
    expect( "menu pair: A taken off Menu Select", has( 0, gc_menuselect, PAD_A ), 0 );
    expect( "menu pair: A on Menu Back", has( 0, gc_menuback, PAD_A ), 1 );
    expect( "menu pair: A kept on Fire", has( 0, gc_fire, PAD_A ), 1 );
    /* and between game actions it is unchanged */
    load( 0, "jump", PAD_X );
    load( 0, "speed", PAD_X );
    expect( "game pair: X taken off Jump", has( 0, gc_jump, PAD_X ), 0 );
    /* Several: nothing is ever stripped */
    cv_controlperkey.value = 2;
    reset();
    load( 0, "use", PAD_A );  load( 0, "fire", PAD_A );
    expect( "Several: both keep A", has( 0, gc_use, PAD_A ) + has( 0, gc_fire, PAD_A ), 2 );
    cv_controlperkey.value = 1;

    /* 7. The two new names round-trip and sit where the enum says. */
    expect( "name: menuselect", strcmp( gamecontrolname[gc_menuselect], "menuselect" ), 0 );
    expect( "name: menuback", strcmp( gamecontrolname[gc_menuback], "menuback" ), 0 );
    expect( "name: devmode unmoved", strcmp( gamecontrolname[gc_devmode], "devmode" ), 0 );

    printf( fails ? "%d FAILED\n" : "all menu key checks passed\n", fails );
    return fails ? 1 : 0;
}
'''

# Each mutation reinstates one bug a check above claims to catch.
MUTANTS = [
    ('no exemption from Control per key',
     'g', 'if( G_Menu_Is_Control( i ) != menu_family )', 'if( 0 )'),
    ('Menu Select checked after the game actions',
     'm', 'if( M_key_is_control(key, gc_menuselect) )  return KEY_ENTER;', ''),
    ('replaced fire/use passed through raw, not swallowed',
     'm', 'if( replaced )  return KEY_NULL;', ''),
    ('replacement global, not per panel',
     'm', 'G_Menu_Select_Control( pind ) == gc_fire', '! G_Any_Panel_Bound( gc_menuselect )'),
    ('Menu Select never replaces fire',
     'g', 'G_Control_Bound( pind, gc_menuselect ) ? gc_menuselect : gc_fire', 'gc_fire'),
    ('Menu Back never replaces use',
     'g', 'G_Control_Bound( pind, gc_menuback ) ? gc_menuback : gc_use', 'gc_use'),
]


def run(gfuncs, mfuncs, quiet=False):
    src = (HARNESS.replace('@ENUM@', ENUM).replace('@NAMES@', NAMES)
                  .replace('@GFUNCS@', gfuncs).replace('@MFUNCS@', mfuncs))
    d = tempfile.mkdtemp(prefix='menukeys-')
    try:
        c, exe = os.path.join(d, 't.c'), os.path.join(d, 't')
        open(c, 'w').write(src)
        r = subprocess.run(['cc', '-std=gnu17', '-Wall', '-Wno-unused-function',
                            '-o', exe, c], capture_output=True, text=True)
        if r.returncode:
            print(r.stderr)
            raise SystemExit('menukeys-test: harness did not compile')
        r = subprocess.run([exe], capture_output=True, text=True)
        if not quiet:
            sys.stdout.write(r.stdout)
        return r.returncode == 0
    finally:
        shutil.rmtree(d, ignore_errors=True)


def join_check(join=None):
    bad = re.findall(r'gc_fire|gc_use\b', JOIN if join is None else join)
    if bad:
        print('FAIL  M_Join_Key names fire/use directly (%d); ask'
              ' G_Menu_Select_Control / G_Menu_Back_Control' % len(bad))
        return False
    return True


if '--selfcheck' in sys.argv:
    ok = True
    for name, where, old, new in MUTANTS:
        g, m = GFUNCS, MFUNCS
        src = g if where == 'g' else m
        if src.count(old) != 1:
            print('selfcheck: mutation no longer applies: %s' % name)
            ok = False
            continue
        if where == 'g':
            g = g.replace(old, new)
        else:
            m = m.replace(old, new)
        red = not run(g, m, quiet=True)
        print('%s  %s' % ('red  ' if red else 'GREEN', name))
        ok &= red
    old = 'G_Menu_Select_Control( panel )'
    if JOIN.count(old) < 1:
        print('selfcheck: join mutation no longer applies')
        ok = False
    else:
        red = not join_check(JOIN.replace(old, 'gc_fire', 1))
        print('%s  join screen reads fire directly' % ('red  ' if red else 'GREEN'))
        ok &= red
    print('selfcheck: every reinstated bug was caught' if ok
          else 'selfcheck: SOME BUGS WENT UNNOTICED')
    sys.exit(0 if ok else 1)

print('extracted %d lines of menu key code'
      % (GFUNCS.count('\n') + MFUNCS.count('\n')))
ok = run(GFUNCS, MFUNCS) & join_check()
sys.exit(0 if ok else 1)
