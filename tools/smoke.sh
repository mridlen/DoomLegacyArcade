#!/bin/bash
#
# [Arcade] Headless smoke test for the DoomLegacy arcade build.
#
# There is no unit test suite and there cannot easily be one, but a lot more can
# be checked without a screen than "does it compile".  This runs the build under
# SDL's dummy and offscreen drivers and exercises startup, config load, level
# setup, the OpenGL path, a level exit and the high-score write.  Every check
# here corresponds to a bug that actually shipped.
#
#   tools/smoke.sh                  # run every check
#   tools/smoke.sh startup warp     # run named checks only
#   tools/smoke.sh -l               # list the checks
#   tools/smoke.sh -k               # keep the scratch directory for inspection
#
# Exit status is 0 only if every check selected passed.
#
# It never touches the live cabinet.  Since the portable install landed,
# legacyhome is found *next to the binary* and $HOME is not consulted, so
# running svn1749/bin/doomlegacy directly would read and write the cabinet's
# real scores, demos and config.  Everything below happens in a scratch copy.
#
# See CLAUDE.md, "Headless verification", for why each of these is shaped the
# way it is -- in particular SDL_NO_SIGNAL_HANDLERS, and why a timeout exit code
# of 124 does not mean the run survived.

set -u

#---------------------------------------------------------------------------
# Locations
#---------------------------------------------------------------------------

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/.." && pwd)"
binary="$root/svn1749/bin/doomlegacy"
staged_home="$root/svn1749/bin/legacyhome"
tracked_home="$root/cabinet/legacyhome"

# Where to find IWADs.  DOOMWADDIR wins; otherwise the paths the engine itself
# searches, so an existing install is normally picked up without configuration.
wad_dirs=(
    "${DOOMWADDIR:-}"
    "$HOME/games/doom"
    "$HOME/games/doomwads"
    "$HOME/games/doomlegacy/wads"
    "$root/svn1749/bin"
    /usr/share/games/doom
    /usr/local/share/games/doom
)

keep=0
list=0

#---------------------------------------------------------------------------
# Output
#---------------------------------------------------------------------------

if [ -t 1 ]; then
    c_pass=$'\e[32m'; c_fail=$'\e[31m'; c_skip=$'\e[33m'; c_off=$'\e[0m'
else
    c_pass=; c_fail=; c_skip=; c_off=
fi

pass_n=0; fail_n=0; skip_n=0
failed_names=""

pass() { printf '  %sPASS%s  %s\n' "$c_pass" "$c_off" "$1"; pass_n=$((pass_n+1)); }
skip() { printf '  %sSKIP%s  %s (%s)\n' "$c_skip" "$c_off" "$1" "$2"; skip_n=$((skip_n+1)); }
fail() {
    printf '  %sFAIL%s  %s\n' "$c_fail" "$c_off" "$1"
    [ $# -gt 1 ] && printf '        %s\n' "$2"
    fail_n=$((fail_n+1))
    failed_names="$failed_names $1"
}

note() { printf '        %s\n' "$1"; }

#---------------------------------------------------------------------------
# Helpers
#---------------------------------------------------------------------------

# ENDOOM text is interleaved with colour escapes per character, so a plain grep
# on the raw log finds nothing and hands back a false "still running".  Strip
# them before reading anything out of a log.
clean() { sed 's/\x1b\[[0-9;]*m//g' "$1"; }

# The reliable "it quit on its own" check.  timeout's exit code is not one:
# a quit through I_Quit hangs in shutdown instead of exiting, so timeout kills
# it and reports 124 exactly as it does for a healthy run.  ENDOOM is only
# printed on a real quit.
quit_cleanly() { [ "$(clean "$1" | grep -c 'READ THE DOCS')" -gt 0 ]; }

crashed() { clean "$1" | grep -qiE 'segmentation fault|I_Error|signal 11'; }

# run <logname> <autoexec-content> <args...>
# Runs the engine headless with the given autoexec.cfg and command line.
run_game() {
    local log="$rundir/$1.log"; shift
    local autoexec="$1"; shift

    printf '%s' "$autoexec" > "$rundir/legacyhome/autoexec.cfg"
    ( cd "$rundir" && \
      SDL_VIDEODRIVER="${SMOKE_VIDEODRIVER:-dummy}" \
      SDL_AUDIODRIVER=dummy \
      SDL_NO_SIGNAL_HANDLERS=1 \
      DISPLAY= \
      timeout "${SMOKE_TIMEOUT:-60}" ./doomlegacy "$@" ) > "$log" 2>&1
    echo "$log"
}

#---------------------------------------------------------------------------
# Checks
#
# Each is  check_<name>  and prints exactly one pass/fail/skip line.
#---------------------------------------------------------------------------

CHECKS="startup warp exitlevel opengl config"

# Does it start, reach the attract screen and quit cleanly?  The commonest
# breakage is a startup-order change that dies before the loop.
check_startup() {
    local log
    log=$(run_game startup 'wait 105
quit
' -game "$game")

    if crashed "$log"; then
        fail startup "crashed during startup; see $log"
    elif quit_cleanly "$log"; then
        pass startup
    else
        fail startup "no ENDOOM: still running when the timeout hit, or died silently. $log"
        note "a silent early stop is almost always a missing SDL_NO_SIGNAL_HANDLERS=1"
    fi
}

# Does a level actually load?  Exercises P_SetupLevel, the renderer and the
# view layout, which is where most of the arcade changes reach.
check_warp() {
    local log
    log=$(run_game warp 'wait 105
quit
' -game "$game" -skill 3 -warp "$map")

    if crashed "$log"; then
        fail warp "crashed with a level loaded; see $log"
    elif ! quit_cleanly "$log"; then
        fail warp "no ENDOOM with -warp $map; see $log"
    else
        pass warp
    fi
}

# Drive a level exit and check the high-score write.  This is the only way to
# reach the intermission headlessly: "exitlevel" alone will not do, since a bare
# +exitlevel runs before the level exists.  Combined with "wait" it exercises
# WI_Init_Stats, HS_LevelExit and the whole scoring path.
check_exitlevel() {
    local log scores line
    rm -f "$rundir/legacyhome/highscores.dat"

    log=$(run_game exitlevel 'wait 105
exitlevel
wait 140
quit
' -game "$game" -skill 3 -warp "$map")

    scores="$rundir/legacyhome/highscores.dat"
    # The file is written with a comment header even when nothing scored, so
    # "the file exists" proves nothing -- require an actual record line.
    line=$(grep -v '^#' "$scores" 2>/dev/null | grep -m1 . || true)

    if crashed "$log"; then
        fail exitlevel "crashed around the level exit; see $log"
    elif [ ! -s "$scores" ]; then
        fail exitlevel "no highscores.dat written at all; see $log"
    elif [ -z "$line" ]; then
        fail exitlevel "highscores.dat has its header but no record: the exit did not score. $log"
        note "check the run was ranked -- an altered ruleset, a cheat or a death records nothing"
    else
        pass exitlevel
        note "scored: $line"
    fi
}

# The hardware renderer, on the real GPU.  "dummy" gives no GL context and
# silently falls back to software, which is how a GL state leak once shipped;
# "offscreen" brings up a real accelerated context with no window, so it does
# not disturb a live X session.  The tell either way is HWR_Startup.
check_opengl() {
    local log cfg

    # The drawmode has to be set in config.cfg, not from the autoexec: the
    # console "drawmode" command does not actually switch drawmode (only the
    # menu does), and by the time an autoexec runs the renderer is long since
    # up.  The value must be one of the exact strings in drawmode_sel_t
    # (v_video.c) -- anything else silently leaves the previous mode selected
    # and looks like the setting being ignored.
    #
    # Leave fullscreen alone -- it must stay "Yes".  The offscreen driver has
    # no window manager, so a windowed mode request comes back with no visual
    # and the run dies with "SetMode: cannot draw 0 bits per pixel", which
    # reads as a colour-depth problem and sends you looking at scr_depth (which
    # is correctly "32 bits" the whole time).
    cfg="$rundir/legacyhome/config.cfg"
    cp "$cfg" "$cfg.smokebak"
    sed -i -e 's/^drawmode .*/drawmode "OpenGL"/' \
           -e 's/^fullscreen .*/fullscreen "Yes"/' "$cfg"
    grep -q '^drawmode ' "$cfg" || echo 'drawmode "OpenGL"' >> "$cfg"

    log=$(SMOKE_VIDEODRIVER=offscreen run_game opengl 'wait 105
quit
' -game "$game" -skill 3 -warp "$map")

    mv -f "$cfg.smokebak" "$cfg"

    if crashed "$log"; then
        fail opengl "crashed on the hardware path; see $log"
    elif ! clean "$log" | grep -q 'HWR_Startup'; then
        skip opengl "no GL context here (fell back to software)"
    elif ! quit_cleanly "$log"; then
        fail opengl "no ENDOOM on the hardware path; see $log"
    else
        pass opengl
        note "$(clean "$log" | grep -m1 'Renderer *:' || echo 'GL context up')"
    fi
}

# Settings that fail to apply are reported at startup by line number.  A config
# that suddenly reports a lot more than it used to has been truncated or has
# outgrown something -- which is exactly how the 8 KB command buffer bug
# presented, silently losing 28 of 188 settings at every load.
check_config() {
    local log n
    log=$(run_game config 'wait 35
quit
' -game "$game")

    n=$(clean "$log" | grep -ci 'config.*line\|did not take effect\|unknown command' || true)

    if crashed "$log"; then
        fail config "crashed before the config check; see $log"
    elif [ "$n" -gt "${SMOKE_MAX_CONFIG_COMPLAINTS:-20}" ]; then
        fail config "$n settings failed to apply -- the config is being truncated or is stale. $log"
        note "this is how the 8 KB command buffer bug presented: 28 of 188 settings lost per load"
    else
        pass config
        note "$n config complaint line(s); a healthy cabinet reports about four"
    fi
}

#---------------------------------------------------------------------------
# Setup
#---------------------------------------------------------------------------

usage() {
    sed -n '3,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit 0
}

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)  usage ;;
        -l|--list)  list=1; shift ;;
        -k|--keep)  keep=1; shift ;;
        --)         shift; break ;;
        -*)         echo "unknown option: $1" >&2; exit 2 ;;
        *)          break ;;
    esac
done

if [ "$list" = 1 ]; then
    for c in $CHECKS; do echo "$c"; done
    exit 0
fi

selected="${*:-$CHECKS}"

[ -x "$binary" ] || {
    echo "No binary at $binary -- build first (cd svn1749/src && make)." >&2
    exit 2
}
command -v timeout >/dev/null || { echo "timeout(1) is required." >&2; exit 2; }

rundir="$(mktemp -d "${TMPDIR:-/tmp}/doomlegacy-smoke.XXXXXX")"
cleanup() {
    if [ "$keep" = 1 ]; then
        echo "Scratch directory kept: $rundir"
    else
        rm -rf "$rundir"
    fi
}
trap cleanup EXIT

cp "$binary" "$rundir/"

# A real config.cfg is needed; copying a whole legacyhome covers it.  Prefer the
# staged one beside the binary, fall back to the tracked cabinet copy.
if [ -d "$staged_home" ]; then
    cp -a "$staged_home" "$rundir/legacyhome"
elif [ -d "$tracked_home" ]; then
    cp -a "$tracked_home" "$rundir/legacyhome"
else
    echo "No legacyhome to copy from; need $staged_home or $tracked_home." >&2
    exit 2
fi
mkdir -p "$rundir/legacyhome"

# Normalise the scratch config's video settings.  Everything except the opengl
# check runs under the dummy driver, which has no GL context -- and a config
# selecting OpenGL there does not merely fall back to software, it *crashes*:
# the mode change fails ("Change Graphics failed"), the engine carries on, and
# the first console line drawn goes through HWR_DrawPic into gluBuild2DMipmaps
# with no context and segfaults during D_DoomMain.  That is pre-existing engine
# behaviour, not something a change under test caused -- it reproduces on any
# build with a GL-configured legacyhome -- but it makes every dummy-driver check
# fail for a reason that has nothing to do with what is being tested.
#
# So: force software here and let check_opengl select OpenGL for itself under
# the offscreen driver, which is the only place a real context exists.  The
# per-drawmode configs are removed as well, since legacyhome keeps one per
# drawmode and whichever matches is executed *after* config.cfg, carrying its
# own scr_depth and undoing this.
smoke_cfg="$rundir/legacyhome/config.cfg"
rm -f "$rundir/legacyhome"/config8p.cfg* \
      "$rundir/legacyhome"/configgl.cfg* \
      "$rundir/legacyhome"/confign.cfg*
if [ -f "$smoke_cfg" ]; then
    sed -i 's/^drawmode .*/drawmode "Software 8bit"/' "$smoke_cfg"
    grep -q '^drawmode ' "$smoke_cfg" || echo 'drawmode "Software 8bit"' >> "$smoke_cfg"
fi

# Link the wads in rather than copying: they are large and read-only here.
found_wads=0
for d in "${wad_dirs[@]}"; do
    [ -n "$d" ] && [ -d "$d" ] || continue
    for w in "$d"/*.[wW][aA][dD]; do
        [ -e "$w" ] || continue
        ln -sf "$w" "$rundir/" && found_wads=1
    done
done

[ "$found_wads" = 1 ] || {
    echo "No wads found. Set DOOMWADDIR to a directory holding your IWADs." >&2
    exit 2
}
[ -e "$rundir/legacy.wad" ] || {
    echo "legacy.wad not found among the wads; the engine needs it." >&2
    exit 2
}

# Pick a game that is actually installed, and a map name that suits it.
game=""
for cand in doom2:DOOM2.WAD:1 doomu:DOOM.WAD:1 plutonia:PLUTONIA.WAD:1 tnt:TNT.WAD:1; do
    idstr="${cand%%:*}"; rest="${cand#*:}"; wad="${rest%%:*}"
    for f in "$rundir"/*; do
        [ "$(basename "$f" | tr 'a-z' 'A-Z')" = "$wad" ] || continue
        game="$idstr"
        break 2
    done
done
[ -n "$game" ] || { echo "No recognised IWAD among the wads found." >&2; exit 2; }

case "$game" in
    doomu) map="E1M1" ;;
    *)     map="1" ;;
esac

echo "DoomLegacy arcade smoke test"
echo "  binary : $binary"
echo "  game   : $game (map $map)"
echo "  scratch: $rundir"
echo

#---------------------------------------------------------------------------
# Run
#---------------------------------------------------------------------------

for name in $selected; do
    case " $CHECKS " in
        *" $name "*) "check_$name" ;;
        *) fail "$name" "no such check (try -l)" ;;
    esac
done

echo
printf 'passed %d, failed %d, skipped %d\n' "$pass_n" "$fail_n" "$skip_n"
if [ "$fail_n" -gt 0 ]; then
    printf 'failed:%s\n' "$failed_names"
    exit 1
fi
exit 0
