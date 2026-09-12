#!/bin/bash
#
# [Arcade] Demo desync regression test.
#
# Replays every record demo on the cabinet and checks that the simulation
# still runs exactly as it did.  This is the check to run after ANY change
# that could touch gameplay -- and "could touch gameplay" is a much wider
# net than it looks, because PP_Random is one shared index (see
# docs/arcade/gameplay-defaults.md), so anything that draws it, however
# decorative, shifts every monster decision that follows.
#
# It works by replaying each demo with -synclog, which writes one line of
# simulation state per tic:
#
#   leveltime prnd x y angle momx momy fwd side aturn btn tflags
#
# Two runs of the same engine produce byte-identical logs.  A change that
# alters gameplay produces a log that diverges, and the first differing line
# names the tic it happened on -- which is the whole point: "the demos still
# play" is not evidence, since a demo that fails to load also plays
# identically to another that fails to load.
#
# Usage:
#   tools/demotest.sh --baseline      # record the reference, on known-good code
#   tools/demotest.sh                 # compare against it
#
# Output is deliberately tiny -- a few lines on success, one line per failing
# demo otherwise -- so that checking for desync is cheap to run and cheap to
# read.  Everything else goes in the work directory.
#
# See docs/arcade/demo-desync.md.

set -u

#---------------------------------------------------------------------------
#  Where things are
#---------------------------------------------------------------------------

SELF_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
REPO=$( cd "$SELF_DIR/.." && pwd )

BINARY="$REPO/svn1749/bin/doomlegacyarcade"
WORKDIR="$REPO/.demotest"
HOMEDIR=""                       # defaults to <binary dir>/legacyhome
DEMODIR=""                       # defaults to <legacyhome>/demos
LEVELDIR=""                      # defaults to <legacyhome>/levels
WADDIR="${DOOMWADDIR:-$HOME/games/doom}"

MODE=compare
# How the engine is asked to replay.  Both produce the same simulation -- that
# is checked, see below -- but -timedemo replays flat out instead of pacing to
# 35 tics a second, which is most of what the suite costs.
PLAYMODE=timedemo
JOBS=$( nproc 2>/dev/null || echo 4 )
PERDEMO_TIMEOUT=1800
QUICK=0
KEEP=0
VERBOSE=0
LISTONLY=0
FILTERS=()

usage()
{
    sed -n '3,40p' "$0" | sed 's/^# \{0,1\}//'
    cat <<'EOF'

Options:
  --baseline        Record the reference logs instead of comparing.
  -j N              Parallel demos (default: number of cores).
  --quick           Cap each demo at 60s and compare only the tics both runs
                    reached.  Minutes instead of half an hour, but it does not
                    see a divergence that happens late in a long demo.
  --timeout N       Per-demo wall clock limit, seconds (default 1800).
  --playdemo        Replay with -playdemo, at the normal 35 tics a second,
                    instead of -timedemo which replays flat out.  Both give
                    the same simulation; this is here to check that they still
                    do, and as an escape hatch if they ever stop.
  -b PATH           Engine binary (default svn1749/bin/doomlegacyarcade).
  --home DIR        legacyhome to take the demos, level packs and config from
                    (default: the one beside the binary).  Use this to test a
                    binary built somewhere else -- a worktree, say -- against
                    the cabinet's own demos:
                      tools/demotest.sh -b svn1749/bin/doomlegacyarcade \
                          --home /path/to/cabinet/svn1749/bin/legacyhome
  -d DIR            Demo directory (default <legacyhome>/demos).
  -w DIR            Work directory (default <repo>/.demotest).
  --waddir DIR      IWAD directory (default $DOOMWADDIR or ~/games/doom).
  -l                List the demos and the arguments derived for each, and
                    stop.  Use this to check a new demo is understood.
  -k                Keep the per-slot scratch directories.
  -v                Print a line per demo as it finishes.
  -h                This help.

Arguments after the options filter the demo list by substring, e.g.
  tools/demotest.sh doomu_E1M1
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --baseline) MODE=baseline ;;
        --playdemo) PLAYMODE=playdemo ;;
        --timedemo) PLAYMODE=timedemo ;;
        -j) JOBS="$2"; shift ;;
        --quick) QUICK=1 ;;
        --timeout) PERDEMO_TIMEOUT="$2"; shift ;;
        -b) BINARY="$2"; shift ;;
        --home) HOMEDIR="$2"; shift ;;
        -d) DEMODIR="$2"; shift ;;
        -w) WORKDIR="$2"; shift ;;
        --waddir) WADDIR="$2"; shift ;;
        -l) LISTONLY=1 ;;
        -k) KEEP=1 ;;
        -v) VERBOSE=1 ;;
        -h|--help) usage; exit 0 ;;
        -*) echo "demotest: unknown option $1" >&2; exit 2 ;;
        *) FILTERS+=( "$1" ) ;;
    esac
    shift
done

[ "$QUICK" = 1 ] && PERDEMO_TIMEOUT=60

if [ ! -x "$BINARY" ]; then
    echo "demotest: no engine binary at $BINARY" >&2
    echo "          build it first, or point at one with -b" >&2
    exit 2
fi
BINARY=$( cd "$( dirname "$BINARY" )" && pwd )/$( basename "$BINARY" )
BINDIR=$( dirname "$BINARY" )
[ -n "$HOMEDIR" ]  || HOMEDIR="$BINDIR/legacyhome"
[ -n "$DEMODIR" ]  || DEMODIR="$HOMEDIR/demos"
[ -n "$LEVELDIR" ] || LEVELDIR="$HOMEDIR/levels"

# [Arcade] Every one of these is used from inside a slot directory the run
# cd's into, so a relative --home / -d / --waddir silently points somewhere
# else once it gets there.  The engine then finds no demo, and -playdemo on a
# file that is not there is not an error: it sits on the title screen until
# the per-demo timeout, which is 1800 seconds.  Eight of those in parallel
# reads as the suite having hung rather than as a mistyped path -- that is
# exactly the "a failed demo run looks like a passing test" trap in CLAUDE.md,
# wearing a different hat.  Absolutise them here, once, where BINARY already
# was.
abspath()
{
    case "$1" in
        /*) echo "$1" ;;
        *)  echo "$( cd "$( dirname "$1" )" 2>/dev/null && pwd )/$( basename "$1" )" ;;
    esac
}
HOMEDIR=$( abspath "$HOMEDIR" )
DEMODIR=$( abspath "$DEMODIR" )
LEVELDIR=$( abspath "$LEVELDIR" )
[ -n "$WADDIR" ] && WADDIR=$( abspath "$WADDIR" )
WORKDIR=$( abspath "$WORKDIR" )

if [ ! -d "$DEMODIR" ]; then
    echo "demotest: no demo directory at $DEMODIR" >&2
    exit 2
fi

BASEDIR="$WORKDIR/baseline"
CURDIR="$WORKDIR/current"
SLOTDIR="$WORKDIR/slots"
OUTDIR=$( [ "$MODE" = baseline ] && echo "$BASEDIR" || echo "$CURDIR" )

#---------------------------------------------------------------------------
#  What arguments does a demo need?
#---------------------------------------------------------------------------
#
# The filename is the high score game id (HS_GameId, hs_stuff.c) plus the map,
# skill and category:  <game>[+<pack>][-sl]_<map>_sk<n>_<cat>.lmp
#
# The game id is the -game switch name; the pack, when there is one, is a wad
# in legacyhome/levels.  The "-sl" suffix marks a Single Level run, which is a
# scoring mode and needs nothing on the command line.
#
# Getting this wrong does not fail loudly -- the engine loads a different
# IWAD and the demo desyncs or refuses -- so demo_args returns empty for
# anything it does not recognise and the demo is reported as skipped rather
# than quietly mis-run.

demo_args()   # $1 = demo basename -> echoes engine args, or nothing
{
    local base id game pack
    base="${1%.lmp}"
    id="${base%%_*}"          # everything before the first underscore
    id="${id%-sl}"            # Single Level is a scoring mode only
    game="${id%%+*}"
    pack=""
    case "$id" in
        *+*) pack="${id#*+}" ;;
    esac

    case "$game" in
        doom|doomu|doom2|tnt|plutonia|heretic|chex) ;;
        *) return 1 ;;
    esac

    if [ -n "$pack" ]; then
        local wad="$LEVELDIR/$pack.wad"
        [ -f "$wad" ] || return 1
        echo "-game $game -file $wad"
    else
        echo "-game $game"
    fi
}

# Which maps a demo loads is RECORDED, not predicted.
#
# The first version of this predicted the start map from the filename and got
# it wrong for 18 of the 95 demos, because the map in the name is the map the
# record is *for* -- where the run reached -- and not where the demo begins
# (HS_BuildDemoPath, hs_stuff.c).  A campaign record at E2M7 is a demo that
# starts at E2M1.  The other scheme, HS_BuildSurvivalDemoPath, puts "ep<N>"
# there instead and names no map at all.
#
# Reading the start map out of the demo header instead is no better: the
# header is patched after recording (G_Update_Demo_Header), and some demos on
# the cabinet have skill/episode/map bytes that disagree with what the engine
# actually loads from them -- they predate a header change.
#
# So the baseline records the maps each demo really loaded, and the comparison
# requires them to be unchanged.  That is a stronger check than the prediction
# ever was: it catches a demo that starts loading a different map without
# anyone having to know the naming convention, and it cannot be wrong about
# what the convention is.

# Each level load prints
#     Level: E1M1  skill 1  demo  chasecam off  views 1
# so this records the map, the skill and what drove the level, once per load.
demo_maps()   # $1 = engine output file
{
    # Colour escapes first: ENDOOM interleaves them per character, so a plain
    # grep on the raw output finds nothing and hands back a false pass.
    sed 's/\x1b\[[0-9;]*m//g' "$1" | grep -a '^Level:' \
        | awk '{print $2, "skill", $4, $5}'
}

#---------------------------------------------------------------------------
#  Build the demo list, longest first so the slowest starts first
#---------------------------------------------------------------------------

mapfile -t ALL < <( cd "$DEMODIR" && ls -S *.lmp 2>/dev/null )
if [ ${#ALL[@]} -eq 0 ]; then
    echo "demotest: no .lmp files in $DEMODIR" >&2
    exit 2
fi

# Demos declared unusable as fixtures, with a reason, in tools/demotest-ignore.txt.
# Counted and reported separately rather than failed on -- but reported, so the
# list cannot quietly grow into "the suite passes because it tests nothing".
IGNORE_FILE="$SELF_DIR/demotest-ignore.txt"
declare -A IGNORED
if [ -f "$IGNORE_FILE" ]; then
    while read -r name _rest; do
        case "$name" in ''|'#'*) continue ;; esac
        IGNORED["$name"]=1
    done < "$IGNORE_FILE"
fi

DEMOS=()
SKIPPED=()
QUARANTINED=()
for d in "${ALL[@]}"; do
    if [ ${#FILTERS[@]} -gt 0 ]; then
        local_match=0
        for f in "${FILTERS[@]}"; do
            case "$d" in *"$f"*) local_match=1 ;; esac
        done
        [ "$local_match" = 1 ] || continue
    fi
    if [ -n "${IGNORED[$d]:-}" ]; then
        QUARANTINED+=( "$d" )
        continue
    fi
    if demo_args "$d" >/dev/null; then
        DEMOS+=( "$d" )
    else
        SKIPPED+=( "$d" )
    fi
done

if [ "$LISTONLY" = 1 ]; then
    for d in "${DEMOS[@]}"; do
        printf '%-46s %s\n' "$d" "$(demo_args "$d")"
    done
    for d in "${SKIPPED[@]}"; do
        printf '%-46s %s\n' "$d" "SKIPPED - game id or level pack not recognised"
    done
    exit 0
fi

if [ ${#DEMOS[@]} -eq 0 ]; then
    echo "demotest: no demos matched" >&2
    exit 2
fi

#---------------------------------------------------------------------------
#  Scratch directories
#---------------------------------------------------------------------------
#
# One per parallel slot, reused across demos rather than one per demo: the
# engine finds legacyhome next to its own binary, so each concurrent run needs
# its own directory, but eight of them is enough for any number of demos.
#
# NEVER the cabinet's own bin/ directory.  legacyhome there holds the live
# config, scores and demos, and running the engine in it rewrites them.

setup_slot()   # $1 = slot dir
{
    local s="$1"
    rm -rf "$s"
    mkdir -p "$s/legacyhome"

    # A hard link where possible: the binary is 8MB and this happens per slot.
    ln "$BINARY" "$s/doomlegacyarcade" 2>/dev/null \
        || cp "$BINARY" "$s/doomlegacyarcade"

    # The cabinet's own config, because that is what its demos were recorded
    # under and what it will replay them under.  Its hash goes in the manifest,
    # so a config change that invalidates the baseline is visible rather than
    # silent.
    if [ -f "$HOMEDIR/config.cfg" ]; then
        # Software, because a GL config under the dummy video driver does not
        # fall back -- it segfaults in HWR_DrawPic.  See CLAUDE.md.
        sed 's/^drawmode .*/drawmode "Software 8bit"/' \
            "$HOMEDIR/config.cfg" > "$s/legacyhome/config.cfg"
    fi
    # The per-drawmode configs are executed after config.cfg and would put
    # the drawmode back, so they are deliberately not copied.

    [ -d "$LEVELDIR" ] && ln -s "$LEVELDIR" "$s/legacyhome/levels"

    local w
    for w in "$WADDIR"/*.wad "$WADDIR"/*.WAD; do
        [ -e "$w" ] && ln -sf "$w" "$s/"
    done
    return 0
}

#---------------------------------------------------------------------------
#  Run one demo
#---------------------------------------------------------------------------
#
# Writes  <OUTDIR>/<demo>.log   the per-tic simulation log
#         <OUTDIR>/<demo>.status  one word: ok, or why not
#
# The engine quits by itself at the end of a demo named with -playdemo
# (singledemo, g_game.c), so a healthy run exits 0 and needs no timeout.
#
# SDL_NO_SIGNAL_HANDLERS is not optional: without it SDL turns a stray signal
# into SDL_QUIT, the engine quits cleanly a second or two in, and the run looks
# like a crash wherever it happened to be.  See CLAUDE.md.

run_demo()   # $1 = slot dir, $2 = demo file
{
    local s="$1" d="$2" args rc
    args=$( demo_args "$d" ) || { echo "badargs" > "$OUTDIR/$d.status"; return 1; }

    rm -f "$s/synclog_play.txt" "$s/out.txt"

    ( cd "$s" && \
      SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_NO_SIGNAL_HANDLERS=1 \
      timeout "$PERDEMO_TIMEOUT" ./doomlegacyarcade $args \
          "-$PLAYMODE" "$DEMODIR/$d" -synclog -nodraw > out.txt 2>&1 )
    rc=$?

    demo_maps "$s/out.txt" > "$OUTDIR/$d.maps"

    # [Arcade] Fingerprint the demo file itself.
    #
    # The cabinet rewrites a demo whenever somebody beats that record, so a
    # .lmp can be replaced under the baseline's feet between one run and the
    # next.  The log then differs for a reason that has nothing to do with the
    # code, and it looks exactly like a desync -- three of them did, the first
    # time the suite ran after a play session.  It cuts the other way too: a
    # replaced demo could just as easily hide a real regression.
    sha256sum "$DEMODIR/$d" | cut -d' ' -f1 > "$OUTDIR/$d.sha"

    # A demo that failed to load still produces a plausible looking run -- two
    # runs that both loaded nothing agree perfectly -- so prove a demo really
    # drove a level before believing anything the run produced.
    if [ ! -s "$s/synclog_play.txt" ]; then
        echo "nosynclog" > "$OUTDIR/$d.status"; return 1
    fi
    # The Level: line names what is driving the level.  Without this, a run
    # that fell through to the attract cycle and played some *other* demo
    # would pass.
    if ! grep -qa ' demo$' "$OUTDIR/$d.maps"; then
        echo "notdemodriven" > "$OUTDIR/$d.status"; return 1
    fi
    # Under --quick the timeout is expected to fire; otherwise it is a hang.
    if [ "$rc" != 0 ] && [ "$QUICK" = 0 ]; then
        echo "exit$rc" > "$OUTDIR/$d.status"; return 1
    fi

    cp "$s/synclog_play.txt" "$OUTDIR/$d.log"
    echo "ok" > "$OUTDIR/$d.status"
    return 0
}

#---------------------------------------------------------------------------
#  Go
#---------------------------------------------------------------------------

# Clear the output directory only for a whole-corpus run.  With a filter this
# must NOT wipe everything: re-baselining the one demo whose record was just
# beaten would otherwise throw away the other ninety-odd baselines and quietly
# leave the suite testing nothing until somebody noticed.  Filtered runs
# overwrite the entries they touch and leave the rest alone.
if [ ${#FILTERS[@]} -eq 0 ]; then
    rm -rf "$OUTDIR"
fi
mkdir -p "$OUTDIR" "$SLOTDIR"

START=$( date +%s )
echo "demotest: ${#DEMOS[@]} demos, $JOBS at a time$( [ "$QUICK" = 1 ] && echo ', quick' )" >&2

# Deal the demos round-robin over the slots.  The list is longest-first, so
# every slot gets a share of the long ones and they all finish together.
for (( slot=0; slot<JOBS; slot++ )); do
    (
        s="$SLOTDIR/$slot"
        setup_slot "$s"
        i=$slot
        while [ $i -lt ${#DEMOS[@]} ]; do
            d="${DEMOS[$i]}"
            if run_demo "$s" "$d"; then
                [ "$VERBOSE" = 1 ] && echo "  ok   $d" >&2
            else
                [ "$VERBOSE" = 1 ] && echo "  FAIL $d ($(cat "$OUTDIR/$d.status"))" >&2
            fi
            i=$(( i + JOBS ))
        done
    ) &
done
wait

[ "$KEEP" = 1 ] || rm -rf "$SLOTDIR"
ELAPSED=$(( $( date +%s ) - START ))

#---------------------------------------------------------------------------
#  Report
#---------------------------------------------------------------------------

CONFIG_HASH=$( sha256sum "$HOMEDIR/config.cfg" 2>/dev/null | cut -c1-12 )
DESCRIBE=$( git -C "$REPO" describe --always --dirty 2>/dev/null )

if [ "$MODE" = baseline ]; then
    {
        echo "# demotest baseline"
        echo "date    $( date -Is )"
        echo "commit  $DESCRIBE"
        echo "binary  $( sha256sum "$BINARY" | cut -c1-12 )"
        echo "config  $CONFIG_HASH"
        echo "mode    $PLAYMODE"
        # Enumerated from what is on disk, not from the list this run drove:
        # a filtered re-baseline touches a few entries and must leave the
        # manifest describing all of them.
        allst=( "$BASEDIR"/*.status )
        echo "demos   ${#allst[@]}"
        for f in "${allst[@]}"; do
            d=$( basename "$f" .status )
            printf '%s %s %s\n' \
                "$( cat "$f" )" \
                "$( [ -f "$BASEDIR/$d.log" ] && wc -l < "$BASEDIR/$d.log" || echo 0 )" \
                "$d"
        done
    } > "$BASEDIR/manifest.txt"

    nall=$( grep -ac '\.lmp$' "$BASEDIR/manifest.txt" )
    nok=$( grep -ac '^ok ' "$BASEDIR/manifest.txt" )
    echo "baseline recorded: $nok/$nall demos, ${ELAPSED}s, commit $DESCRIBE"
    if [ "$nok" != "$nall" ]; then
        echo "WARNING: these did not replay cleanly and are NOT in the baseline:"
        grep -av '^ok ' "$BASEDIR/manifest.txt" | grep -a '\.lmp$' | sed 's/^/  /'
        exit 1
    fi
    [ ${#SKIPPED[@]} -gt 0 ] && echo "note: ${#SKIPPED[@]} demo(s) skipped, run -l to see why"
    exit 0
fi

if [ ! -f "$BASEDIR/manifest.txt" ]; then
    echo "demotest: no baseline in $BASEDIR" >&2
    echo "          record one on known-good code first:  tools/demotest.sh --baseline" >&2
    exit 2
fi

base_commit=$( awk '$1=="commit"{print $2}' "$BASEDIR/manifest.txt" )
base_config=$( awk '$1=="config"{print $2}' "$BASEDIR/manifest.txt" )
base_mode=$( awk '$1=="mode"{print $2}' "$BASEDIR/manifest.txt" )

fails=0; missing=0; checked=0; rerecorded=0; lengths=0
FAILLINES=()
RERECORDED=()
LENGTHNOTES=()
for d in "${DEMOS[@]}"; do
    st=$( cat "$OUTDIR/$d.status" 2>/dev/null || echo "norun" )
    if [ ! -f "$BASEDIR/$d.log" ]; then
        missing=$(( missing + 1 ))
        continue
    fi

    # Is this still the same demo file the baseline was taken from?  If the
    # player beat that record since, it is not, and nothing below can mean
    # anything.  Reported as its own category: it is not a desync, and telling
    # the two apart is the whole point of keeping the hash.
    if [ -f "$BASEDIR/$d.sha" ] \
       && [ "$( cat "$BASEDIR/$d.sha" )" != "$( cat "$OUTDIR/$d.sha" )" ]; then
        rerecorded=$(( rerecorded + 1 ))
        RERECORDED+=( "$d" )
        continue
    fi
    if [ "$st" != ok ]; then
        fails=$(( fails + 1 ))
        FAILLINES+=( "$d: did not replay ($st)" )
        continue
    fi
    checked=$(( checked + 1 ))

    # Did it still load the same levels, at the same skill, still driven by
    # the demo?  This is the wrong-IWAD / wrong-demo check, done by comparison
    # rather than by predicting what the answer ought to be.
    #
    # Noted but not reported yet: when the simulation diverged as well, the tic
    # it diverged on is the more useful fact and a changed level list is a
    # consequence of it, so the tic is reported first and this is appended.
    mapsdiff=""
    if [ -f "$BASEDIR/$d.maps" ] && ! cmp -s "$BASEDIR/$d.maps" "$OUTDIR/$d.maps"; then
        mapsdiff="$( tr '\n' '/' < "$OUTDIR/$d.maps" ) vs baseline $( tr '\n' '/' < "$BASEDIR/$d.maps" )"
    fi

    # Compare the tics both runs reached, and only those.
    #
    # Where a demo *stops* is not reproducible, so it is not a signal.  A demo
    # that ends with the player standing still keeps being simulated after its
    # input runs out, and the point at which the engine finally quits moves
    # with how loaded the machine is -- measured on one demo, six runs of one
    # binary: 1570, 1214, 1656, 1238, 1555, 1365 tics, the short ones being the
    # runs with eight busy cores alongside.  The demo-end path is never
    # reached in any of them (the -timedemo timing line is never printed), so
    # the run is ending by some other, wall-clock-dependent route.  That is a
    # pre-existing engine wart in the same never-quits family as the -timedemo
    # attract-cycle bug; see docs/arcade/demo-desync.md.
    #
    # So a length difference is reported, and counted, but it is not a failure:
    # it carries no information. A difference *within* the shared prefix is a
    # real desync and is what this exists to catch.
    #
    # The cost of that is honest and worth stating: a change that made a demo
    # genuinely end earlier would show up here as a length note rather than a
    # failure.  Nothing is lost that was previously reliable -- that signal was
    # already noise -- but it is not free either.
    n=$( wc -l < "$BASEDIR/$d.log" )
    m=$( wc -l < "$OUTDIR/$d.log" )
    shortest=$n
    [ "$m" -lt "$shortest" ] && shortest="$m"
    if [ "$n" != "$m" ]; then
        lengths=$(( lengths + 1 ))
        LENGTHNOTES+=( "$d: baseline $n tics, now $m (compared the first $shortest)" )
    fi
    head -n "$shortest" "$BASEDIR/$d.log" > "$OUTDIR/.b.$$"
    head -n "$shortest" "$OUTDIR/$d.log"  > "$OUTDIR/.c.$$"
    diffline=$( cmp "$OUTDIR/.b.$$" "$OUTDIR/.c.$$" 2>&1 | sed -n 's/.*line \([0-9]*\).*/\1/p' )
    rm -f "$OUTDIR/.b.$$" "$OUTDIR/.c.$$"

    if [ -n "$diffline" ]; then
        fails=$(( fails + 1 ))
        # The first field of a synclog line is leveltime, which restarts at 1
        # on each level of a multi-level demo, so report both.
        btic=$( sed -n "${diffline}p" "$BASEDIR/$d.log" | awk '{print $1}' )
        FAILLINES+=( "$d: DESYNC at log line $diffline (leveltime $btic)$( [ -n "$mapsdiff" ] && echo ", and the level sequence changed" )" )
    elif [ -n "$mapsdiff" ]; then
        # The simulation matched as far as it goes but the run did not visit
        # the same levels -- a different IWAD or level pack, or a demo that
        # stopped early.
        fails=$(( fails + 1 ))
        FAILLINES+=( "$d: loaded different levels ($mapsdiff)" )
    fi
done

echo "demotest: $checked compared, $fails desynced,$( [ "$lengths" -gt 0 ] && echo " $lengths ended at a different tic," )$( [ ${#QUARANTINED[@]} -gt 0 ] && echo " ${#QUARANTINED[@]} quarantined," ) ${ELAPSED}s"
if [ "$lengths" -gt 0 ]; then
    # Not a failure: where a demo stops is not reproducible.  Listed so the
    # count cannot drift upwards unnoticed.
    echo "  ended at a different tic than the baseline (not a desync; the shared prefix matched):"
    printf '    %s\n' "${LENGTHNOTES[@]}"
fi
if [ ${#QUARANTINED[@]} -gt 0 ]; then
    # Named every run on purpose.  A quarantined demo is one that has stopped
    # watching for regressions, so the list should stay visible and short.
    echo "  not tested (see tools/demotest-ignore.txt): ${QUARANTINED[*]}"
fi
if [ "$rerecorded" -gt 0 ]; then
    # Not a failure.  Somebody beat these records and the cabinet rewrote the
    # .lmp, so the baseline describes a demo that no longer exists.
    echo "  $rerecorded demo(s) re-recorded since the baseline (a record was beaten):"
    printf '    %s\n' "${RERECORDED[@]}"
    echo "    re-baseline just these:  tools/demotest.sh --baseline ${RERECORDED[*]%.lmp}"
fi
if [ "$missing" -gt 0 ]; then
    echo "  $missing demo(s) have no baseline entry -- re-record with --baseline"
fi
if [ "$base_config" != "$CONFIG_HASH" ]; then
    echo "  note: config.cfg has changed since the baseline ($base_config -> $CONFIG_HASH)"
fi
if [ -n "$base_mode" ] && [ "$base_mode" != "$PLAYMODE" ]; then
    # Not a problem: the two replay modes are supposed to produce the same
    # simulation, and comparing across them is how that gets checked.
    echo "  note: baseline was recorded with -$base_mode, this run used -$PLAYMODE"
fi

if [ "$fails" -gt 0 ]; then
    echo "  baseline was commit $base_commit, now $DESCRIBE"
    printf '  %s\n' "${FAILLINES[@]}" | head -20
    [ ${#FAILLINES[@]} -gt 20 ] && echo "  ... and $(( ${#FAILLINES[@]} - 20 )) more"
    echo "  logs: $BASEDIR/<demo>.log vs $CURDIR/<demo>.log"
    exit 1
fi

[ "$missing" -gt 0 ] && exit 1
exit 0
