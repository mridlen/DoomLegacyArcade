#!/bin/bash
#
# [Arcade] Cabinet Link test: pairing, presence, and every refusal.
#
# Runs headless engines on this machine -- a master and members on loopback,
# each with its own scratch copy of legacyhome -- plus tools/linktest_peer.py
# playing a hostile cabinet, and checks what each side reports.
#
# Usage:
#   tools/linktest.sh                 # all cases
#   tools/linktest.sh pair passcode   # just these
#   tools/linktest.sh -l              # list cases
#   tools/linktest.sh --selfcheck     # prove each case can fail (see below)
#   tools/linktest.sh -k              # keep the scratch directory
#   tools/linktest.sh -b PATH         # engine binary (default svn1749/bin)
#   tools/linktest.sh -j N            # cases at once (default 2; each is up to 3 engines)
#
# Security checks that have never been seen to fail are not evidence.
# --selfcheck builds a copy of the engine with LK_SELFCHECK, which lets an
# environment variable switch off one check at a time (d_link.c,
# lk_selfcheck_off), runs each case with its own check switched off, and
# requires the case to FAIL.  It rebuilds d_link.o twice and puts the normal
# one back.
#
# The engine prints LINKSELF / LINKPEER / LINKLOG lines to the terminal only
# (EMSG_errlog); the "link" console command, run from each scratch
# autoexec.cfg, produces the LINKPEER lines.
#
# See docs/arcade/cabinet-link.md.

set -u

SELF_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
REPO=$( cd "$SELF_DIR/.." && pwd )
BIN="$REPO/svn1749/bin/doomlegacyarcade"
PEER="$SELF_DIR/linktest_peer.py"
WADDIR="${DOOMWADDIR:-$HOME/games/doom}"
KEEP=0
SELFCHECK=0
JOBS=2
CASES=()

ALL_CASES="pair passcode allow emptyallow lockout identity fakemaster pinnedfake garbage bigframe unbound linkgame campaign nojoin noshow stranger convert iwadname iwadversion memberhost rehost memberpress slowclock idlejoin musicwad gamewad msgfire menusetup idleshared idleall idlehost joinview rehostview demojoin slowjoin8 move8 lossy8 chaos8 names8 nojoinmaster scores scoreclear scoreclearlive scorebad scorerules scorebusy scoreslarge scorerestore scorerestorepath gamesync gamesyncoff gamesyncmissing gamesyncbusy gamesyncpack gamesynccopy gamesynccopypack gamesynccopybad"

# Which check each case proves, for --selfcheck.  "-" = nothing to switch off.
selfcheck_of() {
    case "$1" in
        pair) echo "-" ;;
        passcode) echo passcode ;;
        allow|emptyallow) echo allow ;;
        lockout) echo lockout ;;
        identity|pinnedfake) echo pin ;;
        fakemaster) echo masterproof ;;
        garbage) echo "-" ;;
        bigframe) echo framesize ;;
        unbound) echo exporter ;;
        linkgame|campaign|nojoin|noshow|convert|iwadname|iwadversion|memberhost|rehost|memberpress|slowclock|idlejoin|musicwad|gamewad|msgfire|menusetup|idleshared|idleall|idlehost|joinview|rehostview|demojoin|slowjoin8|move8|lossy8|chaos8|names8|nojoinmaster|scores|scoreclear|scoreclearlive|scorebad|scorerules|scorebusy|scoreslarge|scorerestore|scorerestorepath|gamesync|gamesyncoff|gamesyncmissing|gamesyncbusy|gamesyncpack|gamesynccopy|gamesynccopypack|gamesynccopybad) echo "-" ;;
        stranger) echo udp ;;
    esac
}

while [ $# -gt 0 ]; do
    case "$1" in
        -l) for c in $ALL_CASES; do echo "$c"; done; exit 0 ;;
        -k) KEEP=1 ;;
        -b) BIN=$2; shift ;;
        -j) JOBS=$2; shift ;;
        --selfcheck) SELFCHECK=1 ;;
        -h|--help) sed -n '3,27p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) CASES+=("$1") ;;
    esac
    shift
done
[ ${#CASES[@]} -gt 0 ] || CASES=($ALL_CASES)

command -v openssl >/dev/null || { echo "linktest: needs the openssl command" >&2; exit 2; }
command -v python3 >/dev/null || { echo "linktest: needs python3" >&2; exit 2; }
[ -x "$BIN" ] || { echo "linktest: no engine at $BIN" >&2; exit 2; }

WORK=$(mktemp -d "${TMPDIR:-/tmp}/linktest.XXXXXX")
cleanup() { [ "$KEEP" = 1 ] && echo "  scratch: $WORK" || rm -rf "$WORK"; }
trap cleanup EXIT

# --------------------------------------------------------------------------
if [ "$SELFCHECK" = 1 ]; then
    SRC="$REPO/svn1749/src"
    echo "linktest: building the LK_SELFCHECK engine"
    touch "$SRC/d_link.c"
    ( cd "$SRC" && make LINK_CFLAGS="-DHAVE_LINK -DLK_SELFCHECK" >"$WORK/build-sc.txt" 2>&1 ) \
        || { echo "linktest: selfcheck build failed ($WORK/build-sc.txt)"; KEEP=1; exit 2; }
    cp "$SRC/../bin/doomlegacyarcade" "$WORK/doomlegacyarcade-selfcheck"
    touch "$SRC/d_link.c"
    ( cd "$SRC" && make >"$WORK/build-normal.txt" 2>&1 ) \
        || { echo "linktest: could not rebuild the normal engine ($WORK/build-normal.txt)"; KEEP=1; exit 2; }
    BIN="$WORK/doomlegacyarcade-selfcheck"
fi

if ! ldd "$BIN" 2>/dev/null | grep -q libssl; then
    echo "linktest: $BIN was built without Cabinet Link (no OpenSSL; HAVE_LINK unset)" >&2
    exit 2
fi

# --------------------------------------------------------------------------
#  Helpers
# --------------------------------------------------------------------------

# mkcab <dir> : a scratch cabinet with its own legacyhome.
mkcab() {
    local d=$1
    local home="$REPO/svn1749/bin/legacyhome"
    [ -d "$home" ] || home="$REPO/cabinet/legacyhome"
    mkdir -p "$d"
    cp "$BIN" "$d/doomlegacyarcade"
    cp -a "$home" "$d/legacyhome"
    rm -rf "$d/legacyhome/link"
    # KEEPDEMOS=1 keeps the record demos, so the attract cycle plays them the
    # way a real cabinet does -- a join that arrives mid-demo is a different
    # path from one that arrives on a title page.
    [ "${KEEPDEMOS:-0}" = 1 ] || rm -rf "$d/legacyhome/demos"
    mkdir -p "$d/legacyhome/demos" "$d/legacyhome/link"
    rm -f "$d/legacyhome"/config8p.cfg* "$d/legacyhome"/configgl.cfg* \
          "$d/legacyhome"/confign.cfg* "$d/legacyhome/autoexec.cfg"
    # DRAWMODE=OpenGL (with VIDEO=offscreen) draws on the real GPU, as the cabinets do.
    sed -i -e "s/^drawmode .*/drawmode \"${DRAWMODE:-Software 8bit}\"/" -e 's/^fullscreen .*/fullscreen "Yes"/' \
           -e "s/^localplayers .*/localplayers \"${LOCALPLAYERS:-1}\"/" "$d/legacyhome/config.cfg"
    for w in "$WADDIR"/*; do ln -sf "$w" "$d/"; done
}

# cfg <dir> <lines...> : write link.cfg
cfg() {
    local d=$1; shift
    printf '%s\n' "$@" > "$d/legacyhome/link/link.cfg"
}

# run <dir> <seconds> <unused> [args...] : run in the background.
# -linkstatus makes the engine print its link status every 2 seconds of wall
# time.  Reports scheduled in game tics (autoexec "wait") were tried first and
# do not work here: with a dozen engines to a core the tics run far behind, the
# report misses the timeout, and that reads as a failure of the link.
run() {
    local d=$1 secs=$2; shift 3
    ( cd "$d" && env LK_SELFCHECK="${LKSC:-}" SDL_VIDEODRIVER="${VIDEO:-dummy}" DISPLAY= SDL_AUDIODRIVER=dummy \
        SDL_NO_SIGNAL_HANDLERS=1 timeout "$secs" ./doomlegacyarcade -game "${GAME:-doom2}" ${NODRAW--nodraw} -nosound -nomusic -linkstatus "$@" \
        > out.txt 2>&1 ) &
}

out() { sed 's/\x1b\[[0-9;]*m//g' "$1/out.txt"; }

# expect <description> <dir> <extended regex> : the output must contain it
FAILS=""
expect() {
    if out "$2" | grep -aEq "$3"; then
        return 0
    fi
    FAILS="$FAILS
      expected in $(basename "$2"): $1  /$3/"
    return 1
}
expect_not() {
    if out "$2" | grep -aEq "$3"; then
        FAILS="$FAILS
      not expected in $(basename "$2"): $1  /$3/"
        return 1
    fi
    return 0
}
expect_file() {   # <description> <file> <regex>
    if grep -aEq "$3" "$2" 2>/dev/null; then return 0; fi
    FAILS="$FAILS
      expected: $1  /$3/ in $(basename "$2")"
    return 1
}

PASS='passcode correct horse battery staple'

# --------------------------------------------------------------------------
#  Cases.  Each gets a directory and a port, and appends to FAILS.
# --------------------------------------------------------------------------

case_pair() {   # dir port
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/m1"; mkcab "$d/m2"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/m1" "role member" "name MEMBERONE" "master 127.0.0.1" "port $p" "$PASS"
    cfg "$d/m2" "role member" "name MEMBERTWO" "master 127.0.0.1" "port $p" "$PASS"
    run "$d/master" 30 420
    sleep 2
    run "$d/m1" 22 350 -devmode
    run "$d/m2" 22 350
    wait
    expect "master sees member one online, in devmode" "$d/master" "^LINKPEER 127\.0\.0\.1 [0-9A-F]{4}-[0-9A-F]{4} online devmode MEMBERONE\|"
    expect "master sees member two online, idle" "$d/master" "^LINKPEER 127\.0\.0\.1 [0-9A-F]{4}-[0-9A-F]{4} online idle MEMBERTWO\|"
    expect "member two sees the master" "$d/m2" "^LINKPEER 127\.0\.0\.1 [0-9A-F]{4}-[0-9A-F]{4} online idle MASTER\|"
    expect "member two sees member one through the master's roster" "$d/m2" "^LINKPEER 127\.0\.0\.1 [0-9A-F]{4}-[0-9A-F]{4} online devmode MEMBERONE\|"
    # The ids each side reports for the other must agree.
    local mid m1id
    mid=$(out "$d/master" | awk '/^LINKSELF/ {print $3; exit}')
    m1id=$(out "$d/m1" | awk '/^LINKSELF/ {print $3; exit}')
    expect "member one knows the master as $mid" "$d/m1" "^LINKPEER 127\.0\.0\.1 $mid online"
    expect "master knows member one as $m1id" "$d/master" "^LINKPEER 127\.0\.0\.1 $m1id online"
    # The members quit two seconds before the master: that is going offline,
    # not being refused.
    expect "a member that quits is listed offline" "$d/master" "^LINKPEER 127\.0\.0\.1 - offline .*MEMBER(ONE|TWO)\|"
    expect_not "a member that quits is not listed as refused" "$d/master" "^LINKPEER .* refused .*MEMBER(ONE|TWO)\|"
    for c in master m1 m2; do
        local perms
        perms=$(stat -c %a "$d/$c/legacyhome/link/cabinet.key" 2>/dev/null)
        [ "$perms" = 600 ] || FAILS="$FAILS
      $c: cabinet.key mode is '$perms', not 600"
        perms=$(stat -c %a "$d/$c/legacyhome/link/pins.txt" 2>/dev/null)
        [ "$perms" = 600 ] || FAILS="$FAILS
      $c: pins.txt mode is '$perms', not 600"
    done
}

case_passcode() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "passcode wrong horse battery staple"
    run "$d/master" 18 420
    sleep 2
    run "$d/member" 16 350
    wait
    expect "master refuses: wrong passcode" "$d/master" "^LINKPEER 127\.0\.0\.1 .* refused .*\|wrong passcode"
    expect "member says the passcodes differ" "$d/member" "\|refused by the master: passcodes differ"
    expect_not "nobody gets online" "$d/master" "^LINKPEER .* online "
}

case_allow() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.9"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "$PASS"
    run "$d/master" 18 420
    sleep 2
    run "$d/member" 16 350
    wait
    expect "master refuses an address not on the list" "$d/master" "^LINKPEER 127\.0\.0\.1 - refused .*\|not on the allow list"
    expect "member says it was not allowed" "$d/member" "\|refused by the master: not allowed"
    expect_not "nobody gets online" "$d/master" "^LINKPEER .* online "
}

case_emptyallow() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "$PASS"
    run "$d/master" 18 420
    sleep 2
    run "$d/member" 16 350
    wait
    expect "master with no allow list refuses everyone" "$d/master" "^LINKPEER 127\.0\.0\.1 - refused .*\|allow list is empty"
    expect_not "nobody gets online" "$d/master" "^LINKPEER .* online "
}

case_lockout() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "passcode wrong horse battery staple"
    run "$d/master" 24 630
    sleep 2
    run "$d/member" 22 350
    wait
    expect "master locks the address out after three failures" "$d/master" "^LINKLOG .*127\.0\.0\.1 locked out for 60 seconds"
    expect "a later attempt is refused as locked out" "$d/master" "^LINKPEER 127\.0\.0\.1 - refused .*\|locked out after repeated failures"
}

case_identity() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "$PASS"
    # Pair once, so the member pins this master.
    run "$d/master" 14 350
    sleep 2
    run "$d/member" 12 280
    wait
    expect "first pairing works" "$d/member" "^LINKPEER 127\.0\.0\.1 .* online .*MASTER\|" || return
    # Same address and passcode, new key: a different machine claiming to be it.
    rm -f "$d/master/legacyhome/link/cabinet.key" "$d/master/legacyhome/link/cabinet.crt"
    mv "$d/member/out.txt" "$d/member/out-first.txt"
    run "$d/master" 18 420
    sleep 2
    run "$d/member" 16 350
    wait
    expect "member refuses the changed master" "$d/member" "MASTER IDENTITY CHANGED"
    expect_not "member does not go online with it" "$d/member" "^LINKPEER .* online "
}

case_fakemaster() {
    local d=$1 p=$2
    mkcab "$d/member"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "$PASS"
    python3 "$PEER" fakemaster "$p" "$d/peer" 16 > "$d/peer.txt" 2>&1 &
    sleep 1
    run "$d/member" 16 420
    wait
    # First contact has no pin, so the member does prove itself (by design) --
    # but a master that cannot prove the passcode back must not be accepted.
    expect "member rejects a master that cannot prove the passcode" "$d/member" "master did not prove the passcode"
    expect_not "member does not go online with it" "$d/member" "^LINKPEER .* online "
}

case_pinnedfake() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "$PASS"
    run "$d/master" 14 350
    sleep 2
    run "$d/member" 12 280
    wait
    expect "first pairing works" "$d/member" "^LINKPEER 127\.0\.0\.1 .* online .*MASTER\|" || return
    mv "$d/member/out.txt" "$d/member/out-first.txt"
    # Now something else answers on the master's address.
    python3 "$PEER" fakemaster "$p" "$d/peer" 16 > "$d/peer.txt" 2>&1 &
    sleep 1
    run "$d/member" 16 420
    wait
    expect_file "a pinned member sends no proof to a different master" "$d/peer.txt" "RESULT fakemaster no_member_proof"
}

case_garbage() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name MEMBER" "master 127.0.0.1" "port $p" "$PASS"
    run "$d/master" 24 560
    sleep 4
    python3 "$PEER" garbage 127.0.0.1 "$p" > "$d/peer.txt" 2>&1
    run "$d/member" 16 350
    wait
    expect_file "master drops a client that sends junk" "$d/peer.txt" "RESULT garbage closed"
    expect "master still pairs a real member afterwards" "$d/master" "^LINKPEER 127\.0\.0\.1 .* online .*MEMBER\|"
}

case_bigframe() {
    local d=$1 p=$2
    mkcab "$d/master"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    run "$d/master" 16 420
    sleep 4
    python3 "$PEER" bigframe 127.0.0.1 "$p" "$d/peer" > "$d/peer.txt" 2>&1
    wait
    expect_file "master drops an oversized frame at once" "$d/peer.txt" "RESULT bigframe closed"
}


# ---- Phase 3: invites and linked games ------------------------------------
# The engines run with -linktest, which honours -linkautohost (start a
# Deathmatch join screen once another cabinet is online) and -linkautojoin
# (press fire and lock in on an invite).  Each cabinet gets its own game UDP
# ports so they can share one machine: -udpport is the port a host serves on,
# -clientport the one a joining cabinet sends from.

# gamecfg <dir> <jointime> : a short countdown, one panel.  20 is the shortest
# jointime accepts -- it is a named list, and anything else is refused.
gamecfg() {
    sed -i -e "s/^jointime .*/jointime \"$2\"/" -e "s/^localplayers .*/localplayers \"${LOCALPLAYERS:-1}\"/" \
        "$1/legacyhome/config.cfg"
    grep -q '^jointime ' "$1/legacyhome/config.cfg" || echo "jointime \"$2\"" >> "$1/legacyhome/config.cfg"
}

# The last LINKGAME / LINKNET line an engine printed.
lastline() { out "$1" | grep -a "^$2 " | tail -1; }

# Host and joiner, in the same game, run the smoke trail phase in step.
# trail= is game_comp_tic minus gametic, fixed for the length of a game, so
# it compares across two status lines printed at different tics. Unsynced it
# differs on essentially every run, not only when a rocket happens to fly --
# which is the only time the desync itself shows.
same_trail() {
    local h j
    h=$(out "$1" | grep -a "^LINKGAME gamestate=1 netgame=1 server=1 " | tail -1 | grep -o ' trail=[-0-9]*')
    j=$(out "$2" | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 " | tail -1 | grep -o ' trail=[-0-9]*')
    [ -n "$h" ] && [ "$h" = "$j" ] || FAILS="$FAILS
      the two cabinets' smoke trail phase differs: host${h:- none} joiner${j:- none}"
}

case_linkgame() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    # 55 seconds: long enough that a game channel dropping out part way
    # through shows up well before the processes are stopped.
    run "$d/master" 57 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 53 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    # The first version of this case passed while the joining cabinet dropped
    # its keys 30 seconds into the game (it thought the game was over): check
    # that the linked game is still linked on both sides at the end.
    expect_not "the joiner never leaves the linked game while in the level" "$d/member" "^LINKGAME gamestate=1 netgame=1 .* none$"
    expect_not "the host never leaves the linked game while in the level" "$d/master" "^LINKGAME gamestate=1 netgame=1 .* none$"
    local hdrop
    hdrop=$(lastline "$d/master" LINKNET | sed -n 's/.*dropped=\([0-9]*\).*/\1/p')
    [ -n "$hdrop" ] && [ "$hdrop" -lt 20 ] || FAILS="$FAILS
      the host dropped ${hdrop:-?} packets from its own linked game (expected almost none)"
    expect "the host invited the other cabinet" "$d/master" "^LINKLOG .*invited other cabinets to DEATHMATCH"
    expect "the other cabinet was invited" "$d/member" "^LINKLOG .*HOSTCAB invited this cabinet to DEATHMATCH"
    expect "the host started a linked game" "$d/master" "^LINKLOG .*starting a linked game with 1 player"
    expect "the other cabinet joined it" "$d/member" "^LINKLOG .*joining HOSTCAB at 127\.0\.0\.1 port $((p+100))"
    expect "the host is in the level as server, two players" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the joiner is in the level as client, two players" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
    expect "the host opens sealed packets" "$d/master" "^LINKNET host sealed=[1-9][0-9]* opened=[1-9]"
    expect "the joiner opens sealed packets" "$d/member" "^LINKNET client sealed=[1-9][0-9]* opened=[1-9]"
}

case_campaign() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 45 0 -linktest -linkautohost campaign -udpport $((p+100))
    sleep 2
    run "$d/member" 42 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    # One player here and one there is a coop campaign, not a solo run.
    expect "the other cabinet was invited to a campaign" "$d/member" "^LINKLOG .*HOSTCAB invited this cabinet to CAMPAIGN"
    expect "the host is in a two player game" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the joiner is in it" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
    same_trail "$d/master" "$d/member"
}

case_nojoin() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 48 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 46 0 -linktest -clientport $((p+101))
    wait
    expect "the other cabinet was invited" "$d/member" "^LINKLOG .*HOSTCAB invited this cabinet"
    expect "nobody joined, so the invite ended there" "$d/member" "^LINKLOG .*HOSTCAB's invite is over"
    expect_not "the host did not start a linked game" "$d/master" "starting a linked game"
    expect "the host plays alone" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=1 "
    expect_not "the other cabinet never joined" "$d/member" "^LINKGAME gamestate=1 netgame=1"
}

# --- Shared scores (Phase 2) -------------------------------------------------
# Record demos for the score cases: six small real Doom 2 record demos, stand-ins
# for whatever a record's demo is.  Real ones, because the attract cycle may
# play them while a case runs.
TDEMOS="$REPO/tools/linktest-demos"

# scorefiles <dir> <highscores lines> <runs lines> : write a cabinet's boards
scorefiles() {
    printf '# test\n%s\n' "$2" > "$1/legacyhome/highscores.dat"
    printf '# test\n%s\n' "$3" > "$1/legacyhome/runs.dat"
}
# demo <dir> <test demo> <record demo name>
demo() { cp "$TDEMOS/$2.lmp" "$1/legacyhome/demos/$3.lmp"; }

# same_file <description> <file a> <file b>
same_file() {
    cmp -s "$2" "$3" || FAILS="$FAILS
      $1: $(basename "$2") differs
$(diff "$2" "$3" 2>&1 | head -12 | sed 's/^/        /')"
}

# Two cabinets with different boards, each holding records and demos the other
# lacks, one on the old file format and one on the new: after syncing they hold
# the same boards, byte for byte, every record's demo came with it, and on each
# key the better record won -- a faster single level time, a Survival run that
# got further even though it was slower.
case_scores() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    # The master: old format, no set time or cabinet.
    scorefiles "$m" "doom2-sl MAP01 2 500 speed MAP01
doom2-sl MAP02 2 700 speed MAP02" "doom2-sl MAP01 MAP01 2 speed 500 AAA
doom2-sl MAP02 MAP02 2 speed 700 BBB
doom2 MAP01 MAP05 2 speed 9000 SUR"
    demo "$m" d1 doom2-sl_MAP01_sk2_speed
    demo "$m" d2 doom2-sl_MAP02_sk2_speed
    demo "$m" d3 doom2_ep1_sk2_speed
    # The member: new format, a faster MAP01, a MAP03 the master lacks, and a
    # Survival run that is faster but did not get as far.
    scorefiles "$b" "doom2-sl MAP01 2 450 speed MAP01 1789000000 BBBB-2222
doom2-sl MAP03 3 900 max MAP03 1789000100 BBBB-2222" "doom2-sl MAP01 MAP01 2 speed 450 MEM 1789000000 BBBB-2222
doom2-sl MAP03 MAP03 3 max 900 MEM 1789000100 BBBB-2222
doom2 MAP01 MAP03 2 speed 5000 LOW 1789000200 BBBB-2222"
    demo "$b" d4 doom2-sl_MAP01_sk2_speed
    demo "$b" d5 doom2-sl_MAP03_sk3_max
    demo "$b" d6 doom2_ep1_sk2_speed

    run "$m" 40 0 -linktest -udpport $((p+100))
    sleep 2
    run "$b" 37 0 -linktest -clientport $((p+101))
    wait

    expect "the master merged the member's scores" "$m" "^LINKLOG Scores: merged with JOINCAB"
    expect "the member merged the master's scores" "$b" "^LINKLOG Scores: merged with HOSTCAB"
    same_file "the two cabinets' split tables" "$m/legacyhome/highscores.dat" "$b/legacyhome/highscores.dat"
    same_file "the two cabinets' run boards" "$m/legacyhome/runs.dat" "$b/legacyhome/runs.dat"
    local want
    for want in "doom2-sl MAP01 2 450 speed MAP01 1789000000 BBBB-2222" \
                "doom2-sl MAP02 2 700 speed MAP02 0 -" \
                "doom2-sl MAP03 3 900 max MAP03 1789000100 BBBB-2222"; do
        grep -qxF "$want" "$m/legacyhome/highscores.dat" || FAILS="$FAILS
      the master's split table lacks: $want"
    done
    for want in "doom2-sl MAP01 MAP01 2 speed 450 MEM 1789000000 BBBB-2222" \
                "doom2-sl MAP01 MAP01 2 speed 500 AAA 0 -" \
                "doom2-sl MAP02 MAP02 2 speed 700 BBB 0 -" \
                "doom2 MAP01 MAP05 2 speed 9000 SUR 0 -"; do
        grep -qxF "$want" "$b/legacyhome/runs.dat" || FAILS="$FAILS
      the member's run board lacks: $want"
    done
    grep -q " LOW " "$m/legacyhome/runs.dat" && FAILS="$FAILS
      a Survival run that did not get as far is still on the board"
    # Each record's demo is the one that set it, on both cabinets.
    same_file "the MAP01 record's demo (the member's) on the master" "$TDEMOS/d4.lmp" "$m/legacyhome/demos/doom2-sl_MAP01_sk2_speed.lmp"
    same_file "the MAP02 record's demo on the member" "$TDEMOS/d2.lmp" "$b/legacyhome/demos/doom2-sl_MAP02_sk2_speed.lmp"
    same_file "the MAP03 record's demo on the master" "$TDEMOS/d5.lmp" "$m/legacyhome/demos/doom2-sl_MAP03_sk3_max.lmp"
    same_file "the Survival demo (the master's) on the member" "$TDEMOS/d3.lmp" "$b/legacyhome/demos/doom2_ep1_sk2_speed.lmp"
}

# A real cabinet's history against an empty cabinet: SCOREHOME=<a legacyhome>
# copies its highscores.dat, runs.dat and demos onto the master (a copy -- the
# source is only read), GAME picks which of its games is shared.  Skipped
# without SCOREHOME.  The member must end with the same files, and every record
# demo for the shared game identical to the master's.
case_scoreslarge() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    [ -n "${SCOREHOME:-}" ] && [ -d "$SCOREHOME/demos" ] || { echo "SKIP" > "$d/skip"; return; }
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    cp "$SCOREHOME/highscores.dat" "$SCOREHOME/runs.dat" "$m/legacyhome/"
    cp "$SCOREHOME"/demos/*.lmp "$m/legacyhome/demos/"
    scorefiles "$b" "" ""

    run "$m" 60 0 -linktest -udpport $((p+100))
    sleep 2
    run "$b" 57 0 -linktest -clientport $((p+101))
    wait

    expect "the member merged the master's scores" "$b" "^LINKLOG Scores: merged with HOSTCAB"
    local f g n=0
    g="${GAME:-doom2}"
    for f in "$b"/legacyhome/demos/*.lmp; do
        n=$((n+1))
        same_file "a received demo" "$m/legacyhome/demos/$(basename "$f")" "$f"
    done
    # Every one of the master's demos for the shared game that has a record.
    for f in $(grep -v '^#' "$b/legacyhome/highscores.dat" | awk '{print $1"_"$2"_sk"$3"_"$5".lmp"}'); do
        [ -e "$b/legacyhome/demos/$f" ] || FAILS="$FAILS
      the member has the record but not its demo: $f"
    done
    [ "$n" -gt 0 ] || FAILS="$FAILS
      no demo reached the member"
    echo "scoreslarge: $n demo(s) for $g reached the member; $(grep -vc '^#' "$b/legacyhome/highscores.dat") record(s), $(grep -vc '^#' "$b/legacyhome/runs.dat") board entries" > "$d/note"
}

# A clear is undone: the master's clear keeps a backup, the member takes the
# clear, then restorehighscores on the master brings every record and demo back
# on both -- under the clear's time, so the member keeps them rather than
# clearing them again.  A restore on the member is refused.
case_scorerestore() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    scorefiles "$m" "doom2-sl MAP01 2 500 speed MAP01" "doom2-sl MAP01 MAP01 2 speed 500 AAA
doom2 MAP01 MAP05 2 speed 9000 SUR"
    demo "$m" d1 doom2-sl_MAP01_sk2_speed
    demo "$m" d3 doom2_ep1_sk2_speed
    scorefiles "$b" "doom2-sl MAP02 2 700 speed MAP02" "doom2-sl MAP02 MAP02 2 speed 700 BBB"
    demo "$b" d2 doom2-sl_MAP02_sk2_speed

    run "$m" 55 0 -linktest -linkcmdat 14 clearhighscores -linkcmdat 30 restorehighscores -udpport $((p+100))
    sleep 2
    run "$b" 52 0 -linktest -linkcmdat 40 restorehighscores -clientport $((p+101))
    wait

    expect "they synced before the clear" "$b" "^LINKLOG Scores: merged with HOSTCAB"
    expect "the clear made a backup" "$m" "^High scores backed up to scores-backup/.* \\(3 record demo"
    expect "the member took the clear" "$b" "^LINKLOG Scores: HOSTCAB cleared the high scores"
    expect "the master restored" "$m" "^High scores restored from .*scores-backup/.*: 2 record\\(s\\) and 3 board entries now \\(was 0 and 0\\), 3 demo\\(s\\) copied"
    expect "the member's restore was refused" "$b" "restore them on the master"
    same_file "the two cabinets' split tables" "$m/legacyhome/highscores.dat" "$b/legacyhome/highscores.dat"
    same_file "the two cabinets' run boards" "$m/legacyhome/runs.dat" "$b/legacyhome/runs.dat"
    grep -q "^# epoch [1-9]" "$b/legacyhome/highscores.dat" || FAILS="$FAILS
      the restore lost the clear's time: $(grep epoch "$b/legacyhome/highscores.dat")"
    local want
    for want in "doom2-sl MAP01 2 500 speed MAP01 0 -" "doom2-sl MAP02 2 700 speed MAP02 0 -"; do
        grep -qxF "$want" "$b/legacyhome/highscores.dat" || FAILS="$FAILS
      the member did not get back: $want"
    done
    grep -qxF "doom2 MAP01 MAP05 2 speed 9000 SUR 0 -" "$b/legacyhome/runs.dat" || FAILS="$FAILS
      the member did not get the Survival run back"
    same_file "the MAP01 demo back on the member" "$TDEMOS/d1.lmp" "$b/legacyhome/demos/doom2-sl_MAP01_sk2_speed.lmp"
    same_file "the MAP02 demo back on the member" "$TDEMOS/d2.lmp" "$b/legacyhome/demos/doom2-sl_MAP02_sk2_speed.lmp"
    same_file "the Survival demo back on the member" "$TDEMOS/d3.lmp" "$b/legacyhome/demos/doom2_ep1_sk2_speed.lmp"
    same_file "the MAP02 demo back on the master" "$TDEMOS/d2.lmp" "$m/legacyhome/demos/doom2-sl_MAP02_sk2_speed.lmp"
}

# An old copy of a cabinet's scores, restored by folder after a clear has
# already spread: exactly the files copying back by hand could not bring back.
case_scorerestorepath() {
    local d=$1 p=$2 m="$1/master" b="$1/member" old="$1/oldhome"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    printf '# test\n# epoch 1789000000\n' > "$m/legacyhome/highscores.dat"
    printf '# test\n# epoch 1789000000\n' > "$m/legacyhome/runs.dat"
    cp "$m/legacyhome/highscores.dat" "$m/legacyhome/runs.dat" "$b/legacyhome/"
    mkdir -p "$old/demos"
    printf '# old\ndoom2-sl MAP01 2 500 speed MAP01\n' > "$old/highscores.dat"
    printf '# old\ndoom2-sl MAP01 MAP01 2 speed 500 OLD\n' > "$old/runs.dat"
    cp "$TDEMOS/d1.lmp" "$old/demos/doom2-sl_MAP01_sk2_speed.lmp"

    run "$m" 40 0 -linktest -linkcmdat 8 "restorehighscores $old" -udpport $((p+100))
    sleep 2
    run "$b" 37 0 -linktest -clientport $((p+101))
    wait

    expect "the master restored the old folder" "$m" "^High scores restored from $old: 1 record"
    same_file "the two cabinets' split tables" "$m/legacyhome/highscores.dat" "$b/legacyhome/highscores.dat"
    same_file "the two cabinets' run boards" "$m/legacyhome/runs.dat" "$b/legacyhome/runs.dat"
    grep -qxF "doom2-sl MAP01 2 500 speed MAP01 0 -" "$b/legacyhome/highscores.dat" || FAILS="$FAILS
      the old record did not reach the member (or was cleared again)"
    grep -qxF "# epoch 1789000000" "$b/legacyhome/highscores.dat" || FAILS="$FAILS
      the clear's time was not kept"
    same_file "the old record's demo on the member" "$TDEMOS/d1.lmp" "$b/legacyhome/demos/doom2-sl_MAP01_sk2_speed.lmp"
}

# The master clears the scores while the member is switched off.  When the
# member comes back its old records must go -- not come back to the master --
# while a record it set after the clear stays and reaches the master.
case_scoreclear() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    scorefiles "$m" "doom2-sl MAP01 2 500 speed MAP01" "doom2-sl MAP01 MAP01 2 speed 500 AAA"
    demo "$m" d1 doom2-sl_MAP01_sk2_speed
    # 2100000000 is 2036: set after any clear this test can make.
    scorefiles "$b" "doom2-sl MAP02 2 700 speed MAP02
doom2-sl MAP03 3 900 max MAP03 2100000000 BBBB-2222" "doom2-sl MAP02 MAP02 2 speed 700 OLD
doom2-sl MAP03 MAP03 3 max 900 NEW 2100000000 BBBB-2222"
    demo "$b" d2 doom2-sl_MAP02_sk2_speed
    demo "$b" d5 doom2-sl_MAP03_sk3_max

    run "$m" 45 0 -linktest -linkcmdat 3 clearhighscores -udpport $((p+100))
    sleep 12
    run "$b" 32 0 -linktest -clientport $((p+101))
    wait

    expect "the master cleared" "$m" "^LINKTEST console: clearhighscores"
    expect "the member heard of the clear" "$b" "^LINKLOG Scores: HOSTCAB cleared the high scores; 1 demo"
    same_file "the two cabinets' split tables" "$m/legacyhome/highscores.dat" "$b/legacyhome/highscores.dat"
    same_file "the two cabinets' run boards" "$m/legacyhome/runs.dat" "$b/legacyhome/runs.dat"
    grep -q "^# epoch [1-9]" "$b/legacyhome/highscores.dat" || FAILS="$FAILS
      the member did not take the master's epoch: $(grep epoch "$b/legacyhome/highscores.dat")"
    grep -q "MAP02" "$m/legacyhome/highscores.dat" "$m/legacyhome/runs.dat" "$b/legacyhome/runs.dat" && FAILS="$FAILS
      a record from before the clear came back"
    grep -q "MAP01" "$b/legacyhome/highscores.dat" && FAILS="$FAILS
      the master's cleared record reached the member"
    grep -qxF "doom2-sl MAP03 3 900 max MAP03 2100000000 BBBB-2222" "$m/legacyhome/highscores.dat" || FAILS="$FAILS
      the member's record from after the clear did not reach the master"
    [ -e "$b/legacyhome/demos/doom2-sl_MAP02_sk2_speed.lmp" ] && FAILS="$FAILS
      the member kept the demo of a cleared record"
    same_file "the record from after the clear's demo on the master" "$TDEMOS/d5.lmp" "$m/legacyhome/demos/doom2-sl_MAP03_sk3_max.lmp"
}

# Both online and synced, then the master clears: the member's boards and
# demos go too.
case_scoreclearlive() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    scorefiles "$m" "doom2-sl MAP01 2 500 speed MAP01" "doom2-sl MAP01 MAP01 2 speed 500 AAA"
    demo "$m" d1 doom2-sl_MAP01_sk2_speed
    scorefiles "$b" "doom2-sl MAP02 2 700 speed MAP02" "doom2-sl MAP02 MAP02 2 speed 700 BBB"
    demo "$b" d2 doom2-sl_MAP02_sk2_speed

    run "$m" 45 0 -linktest -linkcmdat 22 clearhighscores -udpport $((p+100))
    sleep 2
    run "$b" 42 0 -linktest -clientport $((p+101))
    wait

    expect "they synced before the clear" "$b" "^LINKLOG Scores: merged with HOSTCAB"
    expect "the member heard of the clear" "$b" "^LINKLOG Scores: HOSTCAB cleared the high scores; 2 demo"
    same_file "the two cabinets' split tables" "$m/legacyhome/highscores.dat" "$b/legacyhome/highscores.dat"
    same_file "the two cabinets' run boards" "$m/legacyhome/runs.dat" "$b/legacyhome/runs.dat"
    grep -vq '^#' "$b/legacyhome/highscores.dat" "$b/legacyhome/runs.dat" && FAILS="$FAILS
      the member still has records after the clear"
    ls "$b/legacyhome/demos/"*.lmp >/dev/null 2>&1 && FAILS="$FAILS
      the member still has record demos after the clear: $(ls "$b/legacyhome/demos")"
}

# A demo that is not whole (cut short) is refused, and so is its record; the
# rest of the sync goes ahead.
case_scorebad() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    scorefiles "$m" "" ""
    scorefiles "$b" "doom2-sl MAP01 2 450 speed MAP01 1789000000 BBBB-2222
doom2-sl MAP03 3 900 max MAP03 1789000100 BBBB-2222" ""
    head -c 600 "$TDEMOS/d4.lmp" > "$b/legacyhome/demos/doom2-sl_MAP01_sk2_speed.lmp"
    demo "$b" d5 doom2-sl_MAP03_sk3_max

    run "$m" 35 0 -linktest -udpport $((p+100))
    sleep 2
    run "$b" 32 0 -linktest -clientport $((p+101))
    wait

    expect "the master refused the cut-short demo" "$m" "^LINKLOG Scores: refused a demo from JOINCAB .*: not a whole demo"
    expect "the rest was merged" "$m" "^LINKLOG Scores: merged with JOINCAB"
    grep -q "MAP01" "$m/legacyhome/highscores.dat" && FAILS="$FAILS
      the master took the record whose demo was cut short"
    [ -e "$m/legacyhome/demos/doom2-sl_MAP01_sk2_speed.lmp" ] && FAILS="$FAILS
      the cut-short demo was written on the master"
    same_file "the good record's demo on the master" "$TDEMOS/d5.lmp" "$m/legacyhome/demos/doom2-sl_MAP03_sk3_max.lmp"
}

# A gameplay setting that record demos carry but the ranked ruleset does not
# pin -- rocket trails -- differs: those boards are not the same competition,
# so nothing is shared, and the status says why.
case_scorerules() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    sed -i '/^rockettrails /d' "$b/legacyhome/config.cfg"
    echo 'rockettrails "0"' >> "$b/legacyhome/config.cfg"
    scorefiles "$m" "doom2-sl MAP01 2 500 speed MAP01" "doom2-sl MAP01 MAP01 2 speed 500 AAA"
    demo "$m" d1 doom2-sl_MAP01_sk2_speed
    scorefiles "$b" "doom2-sl MAP02 2 700 speed MAP02" "doom2-sl MAP02 MAP02 2 speed 700 BBB"
    demo "$b" d2 doom2-sl_MAP02_sk2_speed

    run "$m" 30 0 -linktest -udpport $((p+100))
    sleep 2
    run "$b" 27 0 -linktest -clientport $((p+101))
    wait

    expect "the master says the settings differ" "$m" "^LINKSCORE peer=JOINCAB .*status=SCORES: DIFFERENT SETTINGS"
    expect_not "nothing merged on the master" "$m" "^LINKLOG Scores: merged"
    expect_not "nothing merged on the member" "$b" "^LINKLOG Scores: merged"
    grep -q "MAP02" "$m/legacyhome/highscores.dat" && FAILS="$FAILS
      the member's record reached the master anyway"
}

# The member is in a game when the master's scores arrive: nothing is applied
# until it is back on the attract screen.
case_scorebusy() {
    local d=$1 p=$2 m="$1/master" b="$1/member"
    mkcab "$m"; mkcab "$b"
    cfg "$m" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$b" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    scorefiles "$m" "doom2-sl MAP01 2 500 speed MAP01" "doom2-sl MAP01 MAP01 2 speed 500 AAA"
    demo "$m" d1 doom2-sl_MAP01_sk2_speed
    scorefiles "$b" "" ""

    run "$m" 45 0 -linktest -udpport $((p+100))
    sleep 2
    run "$b" 42 0 -linktest -skill 3 -warp 1 -linkcmdat 22 exitgame -clientport $((p+101))
    wait

    expect "the member played a level" "$b" "^LINKGAME gamestate=1 netgame=0 "
    expect "the member merged in the end" "$b" "^LINKLOG Scores: merged with HOSTCAB"
    local left merged
    left=$(out "$b" | grep -an "^LINKTEST console: exitgame" | head -1 | cut -d: -f1)
    merged=$(out "$b" | grep -an "^LINKLOG Scores: merged with HOSTCAB" | head -1 | cut -d: -f1)
    [ -n "$left" ] && [ -n "$merged" ] && [ "$merged" -gt "$left" ] || FAILS="$FAILS
      the member merged during its game (merge at line ${merged:-none}, left the game at line ${left:-none})"
}

# nojoin the other way round, as Mark's cabinets were: the member hosts, the
# master is invited during its attract cycle and nobody there presses in.
case_nojoinmaster() {
    local d=$1 p=$2
    KEEPDEMOS=1 mkcab "$d/master"; KEEPDEMOS=1 mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 89 0 -linktest -udpport $((p+102)) -clientport $((p+101))
    # The master's first record demo starts about 20 s in: invite during it.
    sleep ${NJDELAY:-24}
    run "$d/member" 62 0 -linktest -linkautohost deathmatch -udpport $((p+100)) -clientport $((p+103))
    wait
    expect "the master was invited" "$d/master" "^LINKLOG .*LAPCAB invited this cabinet"
    expect "the member plays alone" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=1 players=1 "
    expect_not "the master never joined" "$d/master" "^LINKGAME gamestate=1 netgame=1"
    local before after
    before=$(out "$d/master" | sed -n '1,/LAPCAB invited this cabinet/p' | grep -a '^LINKGAME ' | tail -1)
    case "$before" in "LINKGAME gamestate=1 netgame=0 "*) ;; *) FAILS="$FAILS
      the invite did not arrive during an attract demo (NJDELAY): $before" ;; esac
    # Mark's freeze: the page closed (menu=0) with the demo it had stopped
    # still stopped, gamestate 0, the join screen's last frame on the panel.
    # The attract cycle must be running again: a title page or a demo.
    after=$(out "$d/master" | sed -n '/LAPCAB.s invite is over/,$p' | grep -a '^LINKGAME ' | tail -1)
    case "$after" in "LINKGAME gamestate="[14]" netgame=0 "*" menu=0 "*" none") ;; *) FAILS="$FAILS
      after the invite the master was not back in the attract cycle: ${after:-no status}" ;; esac
}

case_stranger() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 44 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 42 0 -linktest -linkautojoin -clientport $((p+101))
    # Once the linked game is running -- polled for, not assumed after a fixed
    # sleep, which lost the race on a laptop short of memory -- a stranger
    # talks to the host's game port.
    local i
    for i in $(seq 1 80); do
        grep -aq "^LINKGAME gamestate=1 netgame=1 server=1 players=2 " "$d/master/out.txt" 2>/dev/null && break
        sleep 0.5
    done
    python3 "$PEER" udpjunk 127.0.0.1 $((p+100)) 60 > "$d/peer.txt" 2>&1
    wait
    expect "the linked game was running" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 " || return
    local dropped
    dropped=$(lastline "$d/master" LINKNET | sed -n 's/.*dropped=\([0-9]*\).*/\1/p')
    if [ -z "$dropped" ] || [ "$dropped" -lt 60 ]; then
        FAILS="$FAILS
      the host dropped ${dropped:-no} packets; all 60 from the stranger should have been"
    fi
    expect "the game carried on with two players" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
}

case_noshow() {
    local d=$1 p=$2 i
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 50 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 45 0 -linktest -linkautojoin -clientport $((p+101))
    # The joining cabinet is switched off the moment it is told to connect.
    for i in $(seq 1 60); do
        grep -aq "^LINKLOG .*joining HOSTCAB" "$d/member/out.txt" 2>/dev/null && break
        sleep 0.5
    done
    pkill -9 -f "$d/member/doomlegacyarcade" 2>/dev/null
    ( cd "$d/member" && pkill -9 -f "^./doomlegacyarcade -game doom2 -nodraw -nosound -nomusic -linkstatus -linktest -linkautojoin -clientport $((p+101))" ) 2>/dev/null
    wait
    expect "the host was told a player was coming" "$d/master" "^LINKLOG .*starting a linked game with 1 player"
    # 15 seconds of waiting, then the game starts with whoever is there.
    expect "the host gives up waiting and plays" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=1 "
}


# ---- The IWAD is matched by content, not by file name ---------------------

# Ultimate Doom is DOOM.WAD here and doomu.wad there: the same file, so the
# two cabinets must play together.  The engine used to refuse this by name.
case_iwadname() {
    local d=$1 p=$2
    [ -f "$WADDIR/DOOM.WAD" ] || { FAILS="$FAILS
      no $WADDIR/DOOM.WAD to test with"; return; }
    mkcab "$d/master"; mkcab "$d/member"
    rm -f "$d/member/DOOM.WAD" "$d/member/doom.wad"
    ln -s "$WADDIR/DOOM.WAD" "$d/member/doomu.wad"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    GAME=doomu run "$d/master" 45 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    GAME=doomu run "$d/member" 42 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    expect "the joiner really loaded it as doomu.wad" "$d/member" "Added file .*/doomu\.wad"
    expect "the host is in a two player game" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the joiner is in it" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
}

# Doom 2 v1.666 against v1.9: the same name, a different file.  They would play
# two different games, so the joiner must be refused and the host play alone.
case_iwadversion() {
    local d=$1 p=$2
    local old="$HOME/games/doom-backup/DOOM2.WAD.v1.666"
    [ -f "$old" ] || { FAILS="$FAILS
      no $old to test with"; return; }
    mkcab "$d/master"; mkcab "$d/member"
    rm -f "$d/member/DOOM2.WAD"
    ln -s "$old" "$d/member/DOOM2.WAD"
    # -iwad needs a name ending in .wad: "DOOM2.WAD.v1.666" is "File not found".
    mkdir -p "$d/member/oldiwad"
    ln -s "$old" "$d/member/oldiwad/DOOM2.WAD"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 50 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    # -iwad: without it the engine prefers ~/games/doom/DOOM2.WAD over the link
    # in the cabinet's own directory, and the first run of this case quietly
    # tested two identical files.
    run "$d/member" 46 0 -linktest -linkautojoin -clientport $((p+101)) -iwad "$d/member/oldiwad/DOOM2.WAD"
    wait
    expect "the joiner really loaded the old version" "$d/member" "Added file .*/oldiwad/DOOM2\.WAD"
    expect "the joiner tried to join" "$d/member" "^LINKLOG .*joining HOSTCAB"
    expect_not "the joiner was not let into the game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0"
    expect "the host plays alone after the wait" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=1 "
}


# The clock ticks over between the start of the link's tick and the messages it
# handles -- what a Pi does about one tick in five, forced here every tick.  A
# START stamped 1 ms after "now" read as 49 days old: the joining cabinet called
# the game over the moment it began, dropped its keys and could never get in,
# and a host forgot every remote that had pressed in.  Mark saw it as "only
# works when initiated on the server": the Pi was always the one joining late.
case_slowclock() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 55 0 -linktest -linkautojoin -linkpollsleep 3 -udpport $((p+102)) -clientport $((p+101))
    sleep 2
    run "$d/member" 52 0 -linktest -linkautohost deathmatch -linkpollsleep 3 -udpport $((p+100)) -clientport $((p+103))
    wait
    expect "the member started a linked game" "$d/member" "^LINKLOG .*starting a linked game with 1 player"
    expect "the master tried to join it" "$d/master" "^LINKLOG .*joining LAPCAB"
    expect_not "the master did not give up at once" "$d/master" "^LINKLOG .*linked game over"
    expect "the member hosts a two player game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the master is in it" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
}

# The invited cabinet has sat on its attract screen longer than idletimeout --
# which a real cabinet nearly always has.  The idle check closed any menu open
# over the attract screen, and the join screen an invite opens is one, so it
# vanished the tic it appeared and a person pressing fire pressed it on the
# attract screen.  Mark: "I start a deathmatch on the laptop, and it goes back
# to attract on the Pi."  The press here is a real key event, 6 s in.
case_idlejoin() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    sed -i -e 's/^idletimeout .*/idletimeout "15"/' "$d/master/legacyhome/config.cfg"
    run "$d/master" 80 0 -linktest -linkpressafter 6 -udpport $((p+102)) -clientport $((p+101))
    sleep 20
    run "$d/member" 58 0 -linktest -linkautohost deathmatch -udpport $((p+100)) -clientport $((p+103))
    wait
    expect "the master was invited" "$d/master" "^LINKLOG .*LAPCAB invited this cabinet"
    expect "the master pressed fire" "$d/master" "^LINKLOG .*test: pressing fire"
    expect "the member started with the master's player" "$d/member" "^LINKLOG .*starting a linked game with 1 player"
    expect "the member hosts a two player game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the master is in it" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
}

# mkwad <file> <lump name>... : a tiny PWAD whose lumps have those names.
mkwad() {
    python3 - "$@" <<'PYEOF'
import struct, sys
out, names = sys.argv[1], sys.argv[2:]
data = b''.join(b'x' * 16 for _ in names)
dirofs = 12 + len(data)
d = b''.join(struct.pack('<ii8s', 12 + 16 * i, 16, n.encode()) for i, n in enumerate(names))
open(out, 'wb').write(struct.pack('<4sii', b'PWAD', len(names), dirofs) + data + d)
PYEOF
}

# The host plays with a soundtrack pack loaded and the joiner has never heard of
# it.  Music cannot change the game, so they must still play together -- Mark's
# laptop autoloads IDKFAv2.wad and Doom2OST.wad, the Pi has neither, and the Pi
# was refused ("IDKFAv2.wad not found") and went back to attract.
case_musicwad() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    mkwad "$d/master/soundtrk.wad" D_RUNNIN D_STALKS DSPISTOL DPPISTOL
    run "$d/master" 45 0 -linktest -linkautohost deathmatch -udpport $((p+100)) -file soundtrk.wad
    sleep 2
    run "$d/member" 42 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    expect "the host really loaded the soundtrack" "$d/master" "Added file .*soundtrk\.wad"
    expect "the joiner tried to join" "$d/member" "^LINKLOG .*joining HOSTCAB"
    expect_not "the joiner was not asked for it" "$d/member" "soundtrk\.wad.* not found"
    expect "the host is in a two player game" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the joiner is in it" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
}

# The same, except one lump in the pack is not audio.  That could be a map or a
# DEHACKED patch, so the joiner without it must still be refused.
case_gamewad() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    mkwad "$d/master/notmusic.wad" D_RUNNIN DSPISTOL GAMEDATA
    run "$d/master" 50 0 -linktest -linkautohost deathmatch -udpport $((p+100)) -file notmusic.wad
    sleep 2
    run "$d/member" 46 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    expect "the host really loaded it" "$d/master" "Added file .*notmusic\.wad"
    expect "the joiner was told it is missing" "$d/member" "notmusic\.wad.* not found"
    expect_not "the joiner was not let into the game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0"
    expect "the host plays alone after the wait" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=1 "
}

# The host ends the linked game.  The joining cabinet gets "Server has
# Shutdown", and a press of fire -- a real key event -- must take it straight
# back to its attract screen.  It used to step "back" onto the main menu the
# message had opened underneath, so only Escape got a cabinet out.
case_msgfire() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 55 0 -linktest -linkautohost deathmatch -linkendgame 12 -udpport $((p+100))
    sleep 2
    run "$d/member" 52 0 -linktest -linkautojoin -linkmsgpress -clientport $((p+101))
    wait
    expect "the joiner was in the game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
    expect "the host ended it" "$d/master" "^LINKLOG .*test: ending the linked game"
    expect "the joiner was told" "$d/member" "^LINKLOG .*test: message .Server has Shutdown., pressing fire"
    out "$d/member" | sed -n '/pressing fire/,$p' | grep -a "^LINKGAME " | tail -1 | grep -aq " menu=0 " \
        || FAILS="$FAILS
      after fire the joiner was still in a menu: $(out "$d/member" | grep -a '^LINKGAME ' | tail -1)"
}

# kbd <kind> <text> : the presses that type text on the Cabinet Link page's
# on-screen keyboard (tools/linktest_kbd.py reads the layout out of m_menu.c).
kbd() { python3 "$REPO/tools/linktest_kbd.py" "$1" "$2"; }

# Two cabinets that have never been linked, set up entirely from the Cabinet
# Link page with nothing but a stick and two buttons: no link.cfg, no console.
# The master allows the member from the list of cabinets it turned away, and
# the passcode is typed in mixed case, so the lowercase set is used too.
case_menusetup() {
    local d=$1 p=$2
    local pass="Link test 12345" clear20="b b b b b b b b b b b b b b b b b b b b"
    mkcab "$d/master"; mkcab "$d/member"
    # Master: role (off -> master), name, passcode, port; then, once the member
    # has been turned away, allow it from the list.
    local mkeys="open f d f $clear20 $(kbd name PICAB) d f $(kbd passcode "$pass") d d f $clear20 $(kbd port $p) w20000 u f d f esc esc"
    # Member: role (off -> master -> member), name, passcode, master, port.
    local ukeys="open f f d f $clear20 $(kbd name LAPCAB) d f $(kbd passcode "$pass") d f $(kbd master 127.0.0.1) d f $clear20 $(kbd port $p) esc esc"
    run "$d/master" 95 0 -devmode -linktest -linkkeys "$mkeys"
    run "$d/member" 95 0 -devmode -linktest -linkkeys "$ukeys"
    wait
    expect "the master's script ran" "$d/master" "^LINKLOG .*test: keys done"
    expect "the member's script ran" "$d/member" "^LINKLOG .*test: keys done"
    expect_not "no key token was misread" "$d/master" "unknown key token"
    local mc="$d/master/legacyhome/link/link.cfg" uc="$d/member/legacyhome/link/link.cfg"
    for want in "role master" "name PICAB" "port $p" "passcode $pass" "allow 127.0.0.1"; do
        grep -qx "$want" "$mc" 2>/dev/null || FAILS="$FAILS
      master link.cfg lacks \"$want\": $(tr '\n' '|' < "$mc" 2>/dev/null | sed 's/passcode [^|]*/passcode .../')"
    done
    for want in "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "passcode $pass"; do
        grep -qx "$want" "$uc" 2>/dev/null || FAILS="$FAILS
      member link.cfg lacks \"$want\": $(tr '\n' '|' < "$uc" 2>/dev/null | sed 's/passcode [^|]*/passcode .../')"
    done
    expect "the master turned the member away before it was allowed" "$d/master" "^LINKPEER 127\.0\.0\.1 - refused .*allow list"
    expect "the master sees the member online" "$d/master" "^LINKPEER 127\.0\.0\.1 [0-9A-F]{4}-[0-9A-F]{4} online [a-z]+ LAPCAB\|"
    expect "the member sees the master online" "$d/member" "^LINKPEER 127\.0\.0\.1 [0-9A-F]{4}-[0-9A-F]{4} online [a-z]+ PICAB\|"
}

# A linked game where only one cabinet has a player at it.  Mark: "it times out
# when doing a network game and one cabinet is unattended... it should stay
# running as long as one person in the game is moving the controls".  Both
# cabinets at the shortest idle timeout (15 s); the host's player turns every
# 2 s, the joining cabinet is never touched; the game must still be on, with
# both cabinets in it, long after 15 s.
case_idleshared() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=2 gamecfg "$d/master" 20; gamecfg "$d/member" 20
    for c in master member; do
        sed -i -e 's/^idletimeout .*/idletimeout "15"/' -e 's/^idlewarntime .*/idlewarntime "5"/' "$d/$c/legacyhome/config.cfg"
    done
    # The host: two panels pressed in, and then nobody there.  The joining
    # cabinet's player turns every 2 s.
    run "$d/master" 85 0 -linktest -linkautohost deathmatch -linkjoinpanels 2 -udpport $((p+100))
    sleep 2
    run "$d/member" 82 0 -linktest -linkautojoin -linkmoveevery 2000 -clientport $((p+101))
    wait
    expect "the three played together" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=3 "
    expect_not "the host did not time out" "$d/master" "^LINKLOG .*linked game over"
    expect_not "the joining cabinet did not time out" "$d/member" "^LINKLOG .*linked game over"
    # Still all in the level at the very end, 45+ s after the game began.
    lastline "$d/member" LINKGAME | grep -aq "^LINKGAME gamestate=1 netgame=1 server=0 players=3 " \
        || FAILS="$FAILS
      at the end the joining cabinet was not in the game: $(lastline "$d/member" LINKGAME)"
}

# The other half of the same rule: a linked game nobody is playing ends, on
# every cabinet.  One panel each, which never timed out at all before.
case_idleall() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    for c in master member; do
        sed -i -e 's/^idletimeout .*/idletimeout "15"/' -e 's/^idlewarntime .*/idlewarntime "5"/' "$d/$c/legacyhome/config.cfg"
    done
    run "$d/master" 75 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 72 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    expect "the two played together" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
    expect "the host's game ended" "$d/master" "^LINKLOG .*linked game over"
    expect "the joining cabinet's game ended" "$d/member" "^LINKLOG .*linked game over"
    lastline "$d/master" LINKGAME | grep -aq "^LINKGAME gamestate=[0-9] netgame=0 .* none$" \
        || FAILS="$FAILS
      at the end the host was still in a game: $(lastline "$d/master" LINKGAME)"
}

# Mark: "idle timeout, idle warning, and join screen timeout should all be shared
# on a cabinet link, otherwise we might get some weird behavior".  The host's
# idle settings govern the game.  The host at 900 s, the joining cabinet at the
# shortest, 15 s, and nobody playing: the joining cabinet must still be in the
# game at the end, well past its own 15 s.  Fails on the build before -- each
# cabinet used its own setting, so the joiner left the host's game.
case_idlehost() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    sed -i -e 's/^idletimeout .*/idletimeout "900"/' -e 's/^idlewarntime .*/idlewarntime "5"/' "$d/master/legacyhome/config.cfg"
    sed -i -e 's/^idletimeout .*/idletimeout "15"/' -e 's/^idlewarntime .*/idlewarntime "5"/' "$d/member/legacyhome/config.cfg"
    run "$d/master" 75 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 72 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    expect "the two played together" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
    expect "the joining cabinet took the host's idle settings" "$d/member" "^LINKLOG .*joining HOSTCAB .*idle timeout 900 s, warning 5 s"
    expect_not "the joining cabinet did not time out on its own 15 s" "$d/member" "^LINKLOG .*linked game over"
    lastline "$d/member" LINKGAME | grep -aq "^LINKGAME gamestate=1 netgame=1 server=0 players=2 " \
        || FAILS="$FAILS
      at the end the joining cabinet was not in the game: $(lastline "$d/member" LINKGAME)"
}

# Mark: "the first game after booting both cabinet binaries, the joining party
# shows in a smaller size window, like the 1/4 screen; exit and try again, it
# looks normal".  The joining cabinet has four panels and one person pressing
# in, it is freshly started, and it draws (the view size is only worked out
# while drawing, which -nodraw skips).  Its one player must get the whole screen.
case_joinview() {
    local d=$1 p=$2
    mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 50 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    NODRAW= run "$d/member" 47 0 -linktest -linkautojoin -clientport $((p+101))
    wait
    expect "the joining cabinet is in the game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
    local last
    last=$(out "$d/member" | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 players=2 " | tail -1)
    echo "$last" | grep -aq " views=1 " || FAILS="$FAILS
      the joining cabinet drew more than one view: $last"
    echo "$last" | grep -aEq " viewport=([0-9]+)x([0-9]+) screen=\1x\2 " || FAILS="$FAILS
      the joining cabinet's view is not the whole screen: $last"
}

# memberhost the way a person plays it: on a four panel cabinet, fire joins and
# the player is still picking a colour when the host's countdown runs out.
case_memberpress() {
    local d=$1 p=$2
    KEEPDEMOS=1 mkcab "$d/master"; KEEPDEMOS=1 mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 70 0 -linktest -linkautopress -udpport $((p+102)) -clientport $((p+101))
    sleep 2
    run "$d/member" 67 0 -linktest -linkautohost deathmatch -udpport $((p+100)) -clientport $((p+103))
    wait
    expect "the member started a linked game" "$d/member" "^LINKLOG .*starting a linked game with 1 player"
    expect "the master tried to join it" "$d/master" "^LINKLOG .*joining LAPCAB at 127\.0\.0\.1 port $((p+100))"
    expect "the member hosts a two player game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the master is in it" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
}

# The arrangement Mark's cabinets use and linkgame does not: a *member* starts
# the game and the *master* joins it, with the attract demos in place on both.
# Reported as "only works when initiated on the server, otherwise it times out".
case_memberhost() {
    local d=$1 p=$2
    KEEPDEMOS=1 mkcab "$d/master"; KEEPDEMOS=1 mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    run "$d/master" 60 0 -linktest -linkautojoin -udpport $((p+102)) -clientport $((p+101))
    sleep 2
    run "$d/member" 57 0 -linktest -linkautohost deathmatch -udpport $((p+100)) -clientport $((p+103))
    wait
    expect "the member started a linked game" "$d/member" "^LINKLOG .*starting a linked game with 1 player"
    expect "the master tried to join it" "$d/master" "^LINKLOG .*joining LAPCAB at 127\.0\.0\.1 port $((p+100))"
    expect "the member hosts a two player game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=1 players=2 "
    expect "the master is in it" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=0 players=2 "
}


# What Mark actually did: a game started on one cabinet, then -- in the same
# running programs -- a game started on the other.  Every other case starts
# fresh processes, which is why none of them saw "the join screen comes up,
# the game never starts, and the joining cabinet goes back to attract".
case_rehost() {
    local d=$1 p=$2
    KEEPDEMOS=1 mkcab "$d/master"; KEEPDEMOS=1 mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    # Game 1: the master hosts, the member joins, the master ends it after 15 s.
    # Game 2: the member hosts, the master joins.  Real game ports on both
    # (the defaults), because the two cabinets serve and join on the same one.
    run "$d/master" 110 0 -linktest -linkautohost deathmatch -linkautojoin -linkendgame 15 \
        -udpport $((p+100)) -clientport $((p+101))
    sleep 2
    run "$d/member" 106 0 -linktest -linkautojoin -linkhostafter 1 \
        -udpport $((p+102)) -clientport $((p+103))
    wait
    expect "game 1: the master hosted" "$d/master" "^LINKLOG .*starting a linked game with 1 player"
    expect "game 1: the master ended it" "$d/master" "^LINKLOG .*test: ending the linked game"
    expect "game 1 was over on the member" "$d/member" "^LINKLOG .*linked game over \(1 so far\)"
    expect "game 2: the member hosted" "$d/member" "^LINKLOG .*starting a linked game with 1 player"
    expect "game 2: the master joined it" "$d/master" "^LINKLOG .*joining LAPCAB"
    # After game 2 started, both are in it together.
    out "$d/member" | sed -n '/starting a linked game/,$p' | grep -aq "^LINKGAME gamestate=1 netgame=1 server=1 players=2 " \
        || FAILS="$FAILS
      game 2: the member never had a two player game"
    out "$d/master" | sed -n '/joining LAPCAB/,$p' | grep -aq "^LINKGAME gamestate=1 netgame=1 server=0 players=2 " \
        || FAILS="$FAILS
      game 2: the master never got into the member's game"
}

# rehost the way Mark's cabinets are: four panels each and four people at each
# -- an eight player game -- both drawing.  Game 1 the master hosts and the
# member joins; game 2, in the same running programs, the member hosts and the
# master joins.  Mark, with the laptop hosting after it had joined the Pi's game:
# the Pi drew in one corner with a slice of another view and could turn but not
# move; the next try crashed the laptop drawing a view of a player that was not
# its own (HWR_RenderPlayerView pind=1 -> players[1]).  In each game both
# cabinets must have all eight players and draw their own four.
#
# PLAYERS_EACH=1 runs the same thing one player a side.
case_rehostview() {
    local d=$1 p=$2 each=${PLAYERS_EACH:-4}
    local total=$((each * 2)) views=$each
    [ "$each" -ge 3 ] && views=4
    LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name PICAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name LAPCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    NODRAW= run "$d/master" 110 0 -linktest -linkautohost deathmatch -linkautojoin -linkjoinpanels $each \
        -linkendgame 15 -udpport $((p+100)) -clientport $((p+101))
    sleep 2
    NODRAW= run "$d/member" 106 0 -linktest -linkautojoin -linkjoinpanels $each -linkhostafter 1 \
        -udpport $((p+102)) -clientport $((p+103))
    wait
    expect "game 1 was over on the member" "$d/member" "^LINKLOG .*linked game over \(1 so far\)"
    expect "game 2: the member hosted" "$d/member" "^LINKLOG .*starting a linked game with $each player"
    expect "game 2: the master joined it" "$d/master" "^LINKLOG .*joining LAPCAB"
    local g1host g1join host join
    g1host=$(out "$d/master" | grep -a "^LINKGAME gamestate=1 netgame=1 server=1 " | tail -1)
    g1join=$(out "$d/member" | sed -n '1,/linked game over/p' | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 " | tail -1)
    host=$(out "$d/member" | sed -n '/starting a linked game/,$p' | grep -a "^LINKGAME gamestate=1 netgame=1 server=1 " | tail -1)
    join=$(out "$d/master" | sed -n '/joining LAPCAB/,$p' | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 " | tail -1)
    for pair in "game 1 host:$g1host" "game 1 joiner:$g1join" "game 2 host:$host" "game 2 joiner:$join"; do
        local who=${pair%%:*} line=${pair#*:}
        echo "$line" | grep -aq " players=$total " || FAILS="$FAILS
      $who: not a $total player game: $line"
        echo "$line" | grep -aq " views=$views " || FAILS="$FAILS
      $who: did not draw $views view(s): $line"
    done
}

# The host starts its game while an attract demo is playing -- the normal case
# on a cabinet with record demos, and one no other case covers.  A join that the
# host took for a game already in progress would leave the joiner waiting for
# the next game with no players of its own (suspected, then ruled out, for the
# "every view someone else's" report -- that was slowjoin8's).  Four a side.
case_demojoin() {
    local d=$1 p=$2
    KEEPDEMOS=1 LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 70 0 -linktest -linkautohost deathmatch -linkhostindemo -linkjoinpanels 4 -udpport $((p+100))
    sleep 2
    run "$d/member" 67 0 -linktest -linkautojoin -linkjoinpanels 4 -clientport $((p+101))
    wait
    expect "the host started from an attract demo" "$d/master" "^LINKLOG .*starting a linked game with 4 player"
    expect "the host has all eight" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=8 "
    expect "the joining cabinet has all eight" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 "
    expect "the joining cabinet's players are its own" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 .* locals=4,5,6,7 "
}

# An eight player game -- four at each cabinet -- on a slow host.  A host that
# falls behind sends several tics per packet, and with eight
# players a tic's ticcmds are big: the joiner's unpacking checked the add-player
# textcmds against the size of a fixed 45 ticcmd array rather than the packet it
# received, refused them ("Nettics: textcmd exceed buffer"), and so never learned
# its own four players -- every view someone else's, turning but not moving.
# Seen once on the laptop joining the Pi over Wi-Fi.  -netbatchtics 4 makes the
# host send exactly four tics a packet: 32 ticcmds (256 bytes) and the 143 byte
# add-player textcmd come to 399, past the old 360 byte limit, every time.
# (Slowing the host's frames only made it likely: five tics fail too, six split
# into a 45 ticcmd section and a small one, and fit.)
case_slowjoin8() {
    local d=$1 p=$2
    LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 70 0 -linktest -linkautohost deathmatch -linkjoinpanels 4 -netbatchtics 4 -udpport $((p+100))
    sleep 2
    run "$d/member" 67 0 -linktest -linkautojoin -linkjoinpanels 4 -clientport $((p+101))
    wait
    expect "the host has all eight" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=8 "
    expect_not "the joining cabinet refused no textcmd" "$d/member" "textcmd exceed buffer"
    expect "the joining cabinet has all eight, four of them its own" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 .* locals=4,5,6,7 "
}

# Eight players, four at each cabinet, and everybody walking and turning.  Mark:
# on the joining cabinet player 1 "could only turn left and right", and it was
# "booted out of the game in a matter of seconds" -- the host discards a joiner's
# ticcmds when its consistency check fails and kicks it after a few, and turning
# still shows because the view angle comes from the panel.  No case before moved
# anybody.  The joiner must stay in, with no consistency failure on the host.
case_move8() {
    local d=$1 p=$2
    LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 75 0 -linktest -linkautohost deathmatch -linkjoinpanels 4 -linkmoveevery 1000 ${NETLOSS:+-linknetloss $NETLOSS} -udpport $((p+100))
    sleep 2
    run "$d/member" 72 0 -linktest -linkautojoin -linkjoinpanels 4 -linkmoveevery 1000 ${NETLOSS:+-linknetloss $NETLOSS} -clientport $((p+101))
    wait
    expect "the joining cabinet got its four" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 .* locals=4,5,6,7 "
    expect_not "no consistency failure on the host" "$d/master" "Consistency failure|Kick player"
    expect_not "the joining cabinet was not kicked" "$d/member" "kicked|Kicked"
    lastline "$d/member" LINKGAME | grep -aq "^LINKGAME gamestate=1 netgame=1 server=0 players=8 " \
        || FAILS="$FAILS
      at the end the joining cabinet was not in the eight player game: $(lastline "$d/member" LINKGAME)"
    # Player 1 on the joining cabinet really walked.
    local first last
    first=$(out "$d/member" | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 players=8 " | head -1 | grep -o "p1=[-0-9,]*")
    last=$(out "$d/member" | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 players=8 " | tail -1 | grep -o "p1=[-0-9,]*")
    [ -n "$first" ] && [ "$first" != "$last" ] || FAILS="$FAILS
      the joining cabinet's player 1 never moved ($first -> $last)"
}

# setnames <dir> <prefix> <color1> .. <color4> : panel N is named <prefix>N
setnames() {
    local d=$1 pre=$2
    sed -i -e '/^name[234]\? /d' -e '/^color[234]\? /d' "$d/legacyhome/config.cfg"
    printf 'name "%s1"\nname2 "%s2"\nname3 "%s3"\nname4 "%s4"\ncolor "%s"\ncolor2 "%s"\ncolor3 "%s"\ncolor4 "%s"\n' \
        "$pre" "$pre" "$pre" "$pre" "$3" "$4" "$5" "$6" >> "$d/legacyhome/config.cfg"
}

# Four a side, every panel with its own name and colour, and the joining
# cabinet renames panel 4 and recolours panel 3 part way through.  Mark: a
# joining cabinet's players 3 and 4 were "player 7" and "player 8" on the
# intermission -- Send_localtextcmd sent the first two players' text commands
# only.  Both cabinets must end with the same eight names and colours.
case_names8() {
    local d=$1 p=$2
    LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    setnames "$d/master" HA 1 2 3 4; setnames "$d/member" JB 5 6 7 8
    run "$d/master" 70 0 -linktest -linkautohost deathmatch -linkjoinpanels 4 -udpport $((p+100))
    sleep 2
    run "$d/member" 67 0 -linktest -linkautojoin -linkjoinpanels 4 -linkcmdafter 15 "name4 JB4NEW; color3 9" -clientport $((p+101))
    wait
    expect "the joining cabinet got its four" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 .* locals=4,5,6,7 "
    expect "the joining cabinet renamed panel 4" "$d/member" "^LINKTEST console: name4 JB4NEW"
    local h j n
    h=$(lastline "$d/master" LINKNAMES); j=$(lastline "$d/member" LINKNAMES)
    for n in HA1/1 HA2/2 HA3/3 HA4/4 JB1/5 JB2/6 JB3/9 JB4NEW/8; do
        case "$h" in *"=$n"*) ;; *) FAILS="$FAILS
      the host does not have $n: $h" ;; esac
        case "$j" in *"=$n"*) ;; *) FAILS="$FAILS
      the joining cabinet does not have $n: $j" ;; esac
    done
    [ -n "$h" ] && [ "$h" = "$j" ] || FAILS="$FAILS
      the two cabinets list different players:
        host:   $h
        joiner: $j"
}

# move8 on a bad link: 30% of the game's packets thrown away on both cabinets
# (-linknetloss), as a poor Wi-Fi link does.  Mark's joining Pi could only turn,
# then was kicked within seconds; the tic logs showed it running tics whose
# ticcmds had never arrived -- section bits from two different server packets
# counted as one packet (d_clisrv.c, start_tic_hash).  What a player sees is
# what is checked: the joiner stays in the game and its player keeps walking.
# (A rare single repair is allowed: the host's player repair heals it.)  The
# build before was kicked in 4 runs of 4.
case_lossy8() {
    local d=$1 p=$2
    LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 75 0 -linktest -linkautohost deathmatch -linkjoinpanels 4 -linkmoveevery 1000 -linknetloss ${LOSS8:-30} -udpport $((p+100))
    sleep 2
    run "$d/member" 72 0 -linktest -linkautojoin -linkjoinpanels 4 -linkmoveevery 1000 -linknetloss ${LOSS8:-30} -clientport $((p+101))
    wait
    expect "the joining cabinet got its four" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 .* locals=4,5,6,7 "
    expect_not "the joining cabinet was not kicked" "$d/master" "Kick player"
    lastline "$d/member" LINKGAME | grep -aq "^LINKGAME gamestate=1 netgame=1 server=0 players=8 " \
        || FAILS="$FAILS
      at the end the joining cabinet was not in the eight player game: $(lastline "$d/member" LINKGAME)"
    local tail5
    tail5=$(out "$d/member" | grep -a "^LINKGAME gamestate=1 netgame=1 server=0 players=8 " | tail -5 | grep -o "p1=[-0-9,]*" | sort -u | wc -l)
    [ "$tail5" -ge 2 ] || FAILS="$FAILS
      the joining cabinet's player 1 stopped moving at the end"
}

# Eight players, four at each cabinet, every one of them doing something new
# nearly every tic (-linkchaos), on a lossy link (${LOSS8:-10}% of packets
# thrown away).  Mark's cabinets with real players still desynced after
# lossy8 passed: -linkmoveevery holds the same buttons for 400 ms, so a tic run
# with its neighbour's ticcmds could not be told from the right one.  No
# consistency failure, no repair, no kick.  CAT8=campaign plays it as a
# campaign instead of a deathmatch: Mark's first campaign after a clean
# deathmatch desynced on the smoke trail phase (see same_trail).
case_chaos8() {
    local d=$1 p=$2
    LOCALPLAYERS=4 mkcab "$d/master"; LOCALPLAYERS=4 mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    LOCALPLAYERS=4 gamecfg "$d/master" 20; LOCALPLAYERS=4 gamecfg "$d/member" 20
    run "$d/master" 75 0 -linktest -linkautohost ${CAT8:-deathmatch} -linkjoinpanels 4 -linkchaos -linknetloss ${LOSS8:-10} -udpport $((p+100))
    sleep 2
    run "$d/member" 72 0 -linktest -linkautojoin -linkjoinpanels 4 -linkchaos -linknetloss ${LOSS8:-10} -clientport $((p+101))
    wait
    expect "the joining cabinet got its four" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=0 players=8 .* locals=4,5,6,7 "
    expect_not "no consistency failure on the host" "$d/master" "Consistency failure|Kick player"
    expect_not "no repair on the joining cabinet" "$d/member" "Client repair|player_repair"
    same_trail "$d/master" "$d/member"
    lastline "$d/member" LINKGAME | grep -aq "^LINKGAME gamestate=1 netgame=1 server=0 players=8 " \
        || FAILS="$FAILS
      at the end the joining cabinet was not in the eight player game: $(lastline "$d/member" LINKGAME)"
}

case_convert() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name CABA" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name CABB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 20; gamecfg "$d/member" 20
    # Both open a Deathmatch as soon as they see each other.
    run "$d/master" 40 0 -linktest -linkautohost deathmatch -linkautojoin -udpport $((p+100)) -clientport $((p+102))
    sleep 1
    run "$d/member" 39 0 -linktest -linkautohost deathmatch -linkautojoin -udpport $((p+101)) -clientport $((p+103))
    wait
    local servers
    servers=0
    lastline "$d/master" LINKGAME | grep -q "gamestate=1 netgame=1 server=1 players=2 " && servers=$((servers+1))
    lastline "$d/member" LINKGAME | grep -q "gamestate=1 netgame=1 server=1 players=2 " && servers=$((servers+1))
    [ "$servers" = 1 ] || FAILS="$FAILS
      expected one host between them, found $servers"
    expect "cabinet A is in a two player game" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=[01] players=2 "
    expect "cabinet B is in a two player game" "$d/member" "^LINKGAME gamestate=1 netgame=1 server=[01] players=2 "
}

case_unbound() {
    local d=$1 p=$2
    mkcab "$d/master"
    cfg "$d/master" "role master" "name MASTER" "port $p" "$PASS" "allow 127.0.0.1"
    run "$d/master" 22 560
    sleep 4
    python3 "$PEER" unbound 127.0.0.1 "$p" "$d/peer" "${PASS#passcode }" \
        "$d/master/legacyhome/link/cabinet.crt" > "$d/peer.txt" 2>&1
    wait
    expect_file "a correct passcode proof from another TLS session is refused" "$d/peer.txt" "RESULT unbound closed"
}

# --------------------------------------------------------------------------
#  Select Game Sync (d_linksel.c)
# --------------------------------------------------------------------------

# gamesynccabs <dir> <port> : a master with Select Game Sync on (unless
# SYNC=0), and members CHOOSER and FOLLOWER.
gamesynccabs() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/chooser"; mkcab "$d/follower"
    cfg "$d/master" "role master" "name MASTERCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/chooser" "role member" "name CHOOSER" "master 127.0.0.1" "port $p" "$PASS"
    cfg "$d/follower" "role member" "name FOLLOWER" "master 127.0.0.1" "port $p" "$PASS"
    echo "link_gamesync \"${SYNC:-1}\"" >> "$d/master/legacyhome/config.cfg"
}

# The last LINKSEL line's game.
selgame() { lastline "$1" LINKSEL | sed -n 's/.* game=\([^ ]*\) .*/\1/p'; }
expect_game() {   # <dir> <game>
    local g
    g=$(selgame "$1")
    [ "$g" = "$2" ] || FAILS="$FAILS
      $(basename "$1") ended on ${g:-no game} (expected $2)"
}
# How many times an engine started: every start loads legacy.wad once.
starts() { out "$1" | grep -ac "Added file .*legacy\.wad"; }
# The wads= of the first LINKSEL line.
firstwads() { out "$1" | grep -a "^LINKSEL " | head -1 | sed -n 's/.* wads=\([0-9]*\) .*/\1/p'; }

# A player on one member chooses TNT.  The master and the other member follow;
# the one who chose restarts once, for its own choice, and nobody twice.
case_gamesync() {
    local d=$1 p=$2
    [ -f "$WADDIR/TNT.WAD" ] || { FAILS="$FAILS
      no $WADDIR/TNT.WAD to test with"; return; }
    gamesynccabs "$d" "$p"
    run "$d/master" 80 0
    sleep 2
    run "$d/chooser" 77 0 -linktest -linkselectat 12 tnt
    run "$d/follower" 77 0
    wait
    expect "the chooser told its master" "$d/chooser" "^LINKLOG .*told MASTERCAB this cabinet selected tnt"
    # ...before its restart, so the others do not wait for it to reconnect.
    expect "and did so before restarting" "$d/chooser" "^LINKLOG .*told MASTERCAB this cabinet selected tnt, before restarting"
    expect "the master took the choice" "$d/master" "^LINKLOG .*CHOOSER selected tnt; the other cabinets follow"
    expect "the master switched itself" "$d/master" "^LINKLOG .*switching to tnt, selected on the link"
    expect "the other member was told" "$d/master" "^LINKLOG .*telling FOLLOWER to switch to tnt"
    expect "the other member switched" "$d/follower" "^LINKLOG .*switching to tnt, selected on the link"
    expect_not "the chooser was not told to switch to its own choice" "$d/chooser" "^LINKLOG .*switching to"
    # -linkselected: the master restarted to follow, and still holds the choice
    # for any cabinet that has not followed yet.
    expect "the master still holds the choice after its restart" "$d/master" "^LINKSEL sync=1 game=tnt wads=[0-9]+ target=tnt "
    expect_game "$d/master" tnt; expect_game "$d/chooser" tnt; expect_game "$d/follower" tnt
    local c
    for c in master chooser follower; do
        [ "$(starts "$d/$c")" = 2 ] || FAILS="$FAILS
      $c started $(starts "$d/$c") times (expected 2: once, then one switch)"
    done
}

# The same choice with Select Game Sync off on the master: nothing follows.
case_gamesyncoff() {
    local d=$1 p=$2
    [ -f "$WADDIR/TNT.WAD" ] || { FAILS="$FAILS
      no $WADDIR/TNT.WAD to test with"; return; }
    SYNC=0 gamesynccabs "$d" "$p"
    run "$d/master" 50 0
    sleep 2
    run "$d/chooser" 47 0 -linktest -linkselectat 12 tnt
    run "$d/follower" 47 0
    wait
    expect "the master heard the choice and ignored it" "$d/master" "^LINKLOG .*CHOOSER selected tnt \(Select Game Sync is off\)"
    expect_not "the master did not switch" "$d/master" "^LINKLOG .*switching to"
    expect_not "the other member was not told" "$d/follower" "^LINKLOG .*switching to"
    expect_game "$d/chooser" tnt; expect_game "$d/master" doom2; expect_game "$d/follower" doom2
}

# The master chooses TNT; one member has no TNT.WAD.  That member stays where it
# is and says why, the master's page shows it under that cabinet, and the other
# member follows as usual.
case_gamesyncmissing() {
    local d=$1 p=$2
    [ -f "$WADDIR/TNT.WAD" ] || { FAILS="$FAILS
      no $WADDIR/TNT.WAD to test with"; return; }
    gamesynccabs "$d" "$p"
    # The IWAD search also looks in ~/games/doom: give the member a HOME with
    # nothing in it, or it finds TNT there and the case tests nothing.
    rm -f "$d/follower/TNT.WAD" "$d/follower/tnt.wad"
    mkdir -p "$d/follower/home"
    run "$d/master" 70 0 -linktest -linkselectat 12 tnt
    sleep 2
    run "$d/chooser" 67 0
    HOME="$d/follower/home" run "$d/follower" 67 0
    wait
    expect "the member without it really had Doom 2 from its own directory" "$d/follower" "Added file .*/follower/DOOM2\.WAD"
    expect "the master announced its choice" "$d/master" "^LINKLOG .*tnt selected here; the other cabinets follow"
    expect "the member with TNT followed" "$d/chooser" "^LINKLOG .*switching to tnt, selected on the link"
    expect "the member without TNT said why" "$d/follower" "^LINKLOG .*cannot switch to tnt: TNT NOT INSTALLED"
    expect "the master shows it under that cabinet" "$d/master" "^LINKSELPEER FOLLOWER GAME SYNC: TNT NOT INSTALLED$"
    expect_not "and did not say it was switching" "$d/follower" "^LINKLOG .*switching to"
    expect "and copying was not offered, since it is off" "$d/follower" "^LINKLOG .*copy of tnt failed: COPY MISSING WADS IS OFF"
    expect_game "$d/chooser" tnt; expect_game "$d/follower" doom2
    [ "$(starts "$d/follower")" = 1 ] || FAILS="$FAILS
      the member without TNT restarted"
    # Told once, not over and over.
    local told
    told=$(out "$d/master" | grep -ac "^LINKLOG .*telling FOLLOWER to switch to tnt")
    [ "$told" -le 2 ] || FAILS="$FAILS
      the master told FOLLOWER $told times"
}

# The master chooses TNT while a member is in a game.  The member is not
# interrupted; it switches once its game is over and it is back on attract.
case_gamesyncbusy() {
    local d=$1 p=$2
    [ -f "$WADDIR/TNT.WAD" ] || { FAILS="$FAILS
      no $WADDIR/TNT.WAD to test with"; return; }
    gamesynccabs "$d" "$p"
    run "$d/master" 75 0 -linktest -linkselectat 12 tnt
    sleep 2
    run "$d/follower" 72 0 -warp 1 -skill 3 -linktest -linkcmdat 40 "exitgame"
    wait
    expect "the member was in a level" "$d/follower" "^LINKGAME gamestate=1 "
    expect "the master waited for it" "$d/master" "^LINKLOG .*FOLLOWER is playing; it switches to tnt when it is free"
    expect "then told it" "$d/master" "^LINKLOG .*telling FOLLOWER to switch to tnt"
    expect "and it switched" "$d/follower" "^LINKLOG .*switching to tnt, selected on the link"
    expect_game "$d/follower" tnt
    # Not during the level: it switches only after its game was ended.  (The
    # restart keeps -warp, so the switched cabinet is in a level again after.)
    local ended sw inlevel
    ended=$(out "$d/follower" | grep -an "^LINKTEST console: exitgame" | head -1 | cut -d: -f1)
    sw=$(out "$d/follower" | grep -an "^LINKLOG .*switching to tnt" | head -1 | cut -d: -f1)
    [ -n "$ended" ] && [ -n "$sw" ] && [ "$ended" -lt "$sw" ] || FAILS="$FAILS
      the member switched during its game (game ended line ${ended:-?}, switch line ${sw:-?})"
    inlevel=$(out "$d/follower" | head -n "${ended:-0}" | grep -ac "^LINKGAME gamestate=1 ")
    [ "$inlevel" -ge 8 ] || FAILS="$FAILS
      the member was only in its level for $inlevel status lines -- too short to prove it waited"
}

# mkpack <out.wad> : a level pack holding Doom 2's own MAP01, so the attract
# demos still play on it.
mkpack() {
    python3 - "$WADDIR/DOOM2.WAD" "$1" <<'PYEOF'
import struct, sys
src, out = sys.argv[1], sys.argv[2]
b = open(src, 'rb').read()
n, ofs = struct.unpack_from('<ii', b, 4)
ents = [struct.unpack_from('<ii8s', b, ofs + 16*i) for i in range(n)]
i = [e[2].rstrip(b'\0') for e in ents].index(b'MAP01')
lumps = ents[i:i+11]
data = b''; d = b''
for pos, size, name in lumps:
    d += struct.pack('<ii8s', 12 + len(data), size, name)
    data += b[pos:pos+size]
open(out, 'wb').write(struct.pack('<4sii', b'PWAD', len(lumps), 12 + len(data)) + data + d)
PYEOF
}

# The master loads a level pack (no restart).  A member with the same pack under
# another case follows the same way; one without it stays and says why.
case_gamesyncpack() {
    local d=$1 p=$2
    gamesynccabs "$d" "$p"
    mkdir -p "$d/master/legacyhome/levels" "$d/chooser/legacyhome/levels" "$d/follower/legacyhome/levels"
    mkpack "$d/master/legacyhome/levels/syncpack.wad"
    cp "$d/master/legacyhome/levels/syncpack.wad" "$d/chooser/legacyhome/levels/SyncPack.wad"
    run "$d/master" 50 0 -linktest -linkselectat 12 syncpack
    sleep 2
    run "$d/chooser" 47 0
    run "$d/follower" 47 0
    wait
    expect "the master loaded the pack: one more wad" "$d/master" "^LINKSEL sync=1 game=doom2\+syncpack wads=$(( $(firstwads "$d/master") + 1 )) "
    expect "the master announced it" "$d/master" "^LINKLOG .*doom2\+syncpack selected here; the other cabinets follow"
    expect "the member with the pack followed" "$d/chooser" "^LINKLOG .*switching to doom2\+syncpack, selected on the link"
    expect "and loaded its own copy: one more wad" "$d/chooser" "^LINKSEL sync=0 game=doom2\+SyncPack wads=$(( $(firstwads "$d/chooser") + 1 )) "
    expect "the member without it said why" "$d/follower" "^LINKLOG .*cannot switch to doom2\+syncpack: NO LEVEL PACK SYNCPACK"
    expect "the master shows it" "$d/master" "^LINKSELPEER FOLLOWER GAME SYNC: NO LEVEL PACK SYNCPACK$"
    expect_game "$d/master" doom2+syncpack; expect_game "$d/chooser" doom2+SyncPack; expect_game "$d/follower" doom2
    local c
    for c in master chooser follower; do
        [ "$(starts "$d/$c")" = 1 ] || FAILS="$FAILS
      $c restarted (a pack into an empty slot needs no restart)"
    done
}

# Copy Missing Wads: the master picks TNT; a member without it gets the
# master's copy into wads/ beside its program, and switches.
case_gamesynccopy() {
    local d=$1 p=$2
    [ -f "$WADDIR/TNT.WAD" ] || { FAILS="$FAILS
      no $WADDIR/TNT.WAD to test with"; return; }
    gamesynccabs "$d" "$p"
    echo 'link_copywads "1"' >> "$d/master/legacyhome/config.cfg"
    rm -f "$d/follower/TNT.WAD" "$d/follower/tnt.wad"
    mkdir -p "$d/follower/home"
    run "$d/master" 90 0 -linktest -linkselectat 12 tnt
    sleep 2
    HOME="$d/follower/home" run "$d/follower" 87 0
    wait
    expect "the master sent it" "$d/master" "^LINKLOG .*copying TNT\.WAD \([0-9]+ bytes\) to FOLLOWER"
    expect "the member copied it beside its program" "$d/follower" "^LINKLOG .*copied TNT\.WAD from the master to .*/follower/wads/TNT\.WAD"
    expect "and switched to it, from there" "$d/follower" "Added file .*/follower/wads/TNT\.WAD"
    expect_game "$d/follower" tnt
    cmp -s "$d/follower/wads/TNT.WAD" "$WADDIR/TNT.WAD" || FAILS="$FAILS
      the copied TNT.WAD differs from the master's"
    [ -z "$(ls "$d/follower/wads/"*.part 2>/dev/null)" ] || FAILS="$FAILS
      a .part file was left behind"
    expect "the master's page said it was copied" "$d/master" "^LINKSELPEER .* GAME SYNC: COPIED TNT\.WAD$"
}

# The same for a level pack, into legacyhome/levels/.
case_gamesynccopypack() {
    local d=$1 p=$2
    gamesynccabs "$d" "$p"
    echo 'link_copywads "1"' >> "$d/master/legacyhome/config.cfg"
    mkdir -p "$d/master/legacyhome/levels"
    mkpack "$d/master/legacyhome/levels/syncpack.wad"
    run "$d/master" 70 0 -linktest -linkselectat 12 syncpack
    sleep 2
    run "$d/follower" 67 0
    wait
    expect "the member copied the pack into levels" "$d/follower" "^LINKLOG .*copied syncpack\.wad from the master to .*/follower/legacyhome/levels/syncpack\.wad"
    expect_game "$d/follower" doom2+syncpack
    cmp -s "$d/follower/legacyhome/levels/syncpack.wad" "$d/master/legacyhome/levels/syncpack.wad" || FAILS="$FAILS
      the copied pack differs from the master's"
}

# One byte damaged in transit (-linkcorruptwad on the master): the member must
# refuse the file, leave nothing behind, and stay on its own game.
case_gamesynccopybad() {
    local d=$1 p=$2
    [ -f "$WADDIR/TNT.WAD" ] || { FAILS="$FAILS
      no $WADDIR/TNT.WAD to test with"; return; }
    gamesynccabs "$d" "$p"
    echo 'link_copywads "1"' >> "$d/master/legacyhome/config.cfg"
    rm -f "$d/follower/TNT.WAD" "$d/follower/tnt.wad"
    mkdir -p "$d/follower/home"
    run "$d/master" 80 0 -linktest -linkselectat 12 tnt -linkcorruptwad
    sleep 2
    HOME="$d/follower/home" run "$d/follower" 77 0
    wait
    expect "the member refused it" "$d/follower" "^LINKLOG .*copy of TNT\.WAD failed: IT ARRIVED DAMAGED"
    [ ! -e "$d/follower/wads/TNT.WAD" ] || FAILS="$FAILS
      the damaged TNT.WAD was installed"
    [ -z "$(ls "$d/follower/wads/"* 2>/dev/null)" ] || FAILS="$FAILS
      something was left in wads/: $(ls "$d/follower/wads/")"
    expect_game "$d/follower" doom2
}

# --------------------------------------------------------------------------

total=0; passed=0; failed=0; skipped=0
port=5230
pids=()
names=()
for c in "${CASES[@]}"; do
    case " $ALL_CASES " in *" $c "*) ;; *) echo "linktest: no case '$c' (-l lists them)"; exit 2 ;; esac
    sc=$(selfcheck_of "$c")
    if [ "$SELFCHECK" = 1 ] && [ "$sc" = "-" ]; then
        printf '  SKIP  %-11s (nothing to switch off)\n' "$c"
        skipped=$((skipped+1))
        continue
    fi
    # A few cases at a time: each is up to three engines, and a machine run
    # flat out makes every timing in them a guess.  Four at once was killed by
    # the OOM guard on the laptop while a browser was open; two is the default.
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n; done
    mkdir -p "$WORK/$c"
    (
        cname=$c     # case functions are free to use $c themselves
        FAILS=""
        LKSC=""
        [ "$SELFCHECK" = 1 ] && LKSC=$sc
        export LKSC
        "case_$cname" "$WORK/$cname" "$port"
        wait
        printf '%s' "$FAILS" > "$WORK/$cname/fails.txt"
        touch "$WORK/$cname/finished"
    ) &
    pids+=($!)
    names+=("$c")
    port=$((port+2))
done

wait
for i in "${!pids[@]}"; do
    c=${names[$i]}
    total=$((total+1))
    if [ ! -f "$WORK/$c/finished" ]; then
        # A case that did not record a result has not passed.
        f="
      the case did not finish (no result recorded)"
    else
        f=$(cat "$WORK/$c/fails.txt")
    fi
    if [ "$SELFCHECK" = 1 ]; then
        if [ -n "$f" ]; then
            printf '  PASS  %-11s goes red with "%s" switched off\n' "$c" "$(selfcheck_of "$c")"
            passed=$((passed+1))
        else
            printf '  FAIL  %-11s stays green with "%s" switched off -- it cannot see that bug\n' \
                "$c" "$(selfcheck_of "$c")"
            failed=$((failed+1))
        fi
    else
        if [ -f "$WORK/$c/skip" ] && [ -z "$f" ]; then
            printf '  SKIP  %s (needs SCOREHOME)\n' "$c"
            skipped=$((skipped+1))
        elif [ -z "$f" ]; then
            printf '  PASS  %s\n' "$c"
            [ -f "$WORK/$c/note" ] && printf '        %s\n' "$(cat "$WORK/$c/note")"
            passed=$((passed+1))
        else
            printf '  FAIL  %s%s\n' "$c" "$f"
            failed=$((failed+1))
        fi
    fi
done

echo ""
echo "passed $passed, failed $failed, skipped $skipped"
[ "$failed" = 0 ]
