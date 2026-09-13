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

ALL_CASES="pair passcode allow emptyallow lockout identity fakemaster pinnedfake garbage bigframe unbound linkgame campaign nojoin noshow stranger convert"

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
        linkgame|campaign|nojoin|noshow|convert) echo "-" ;;
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
    rm -rf "$d/legacyhome/demos" "$d/legacyhome/link"
    mkdir -p "$d/legacyhome/demos" "$d/legacyhome/link"
    rm -f "$d/legacyhome"/config8p.cfg* "$d/legacyhome"/configgl.cfg* \
          "$d/legacyhome"/confign.cfg* "$d/legacyhome/autoexec.cfg"
    sed -i -e 's/^drawmode .*/drawmode "Software 8bit"/' \
           -e 's/^localplayers .*/localplayers "1"/' "$d/legacyhome/config.cfg"
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
    ( cd "$d" && env LK_SELFCHECK="${LKSC:-}" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        SDL_NO_SIGNAL_HANDLERS=1 timeout "$secs" ./doomlegacyarcade -game doom2 -nodraw -nosound -nomusic -linkstatus "$@" \
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

# gamecfg <dir> <jointime> : a short countdown, one panel
gamecfg() {
    sed -i -e "s/^jointime .*/jointime \"$2\"/" -e 's/^localplayers .*/localplayers "1"/' \
        "$1/legacyhome/config.cfg"
    grep -q '^jointime ' "$1/legacyhome/config.cfg" || echo "jointime \"$2\"" >> "$1/legacyhome/config.cfg"
}

# The last LINKGAME / LINKNET line an engine printed.
lastline() { out "$1" | grep -a "^$2 " | tail -1; }

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
}

case_nojoin() {
    local d=$1 p=$2
    mkcab "$d/master"; mkcab "$d/member"
    cfg "$d/master" "role master" "name HOSTCAB" "port $p" "$PASS" "allow 127.0.0.1"
    cfg "$d/member" "role member" "name JOINCAB" "master 127.0.0.1" "port $p" "$PASS"
    gamecfg "$d/master" 6; gamecfg "$d/member" 6
    run "$d/master" 34 0 -linktest -linkautohost deathmatch -udpport $((p+100))
    sleep 2
    run "$d/member" 32 0 -linktest -clientport $((p+101))
    wait
    expect "the other cabinet was invited" "$d/member" "^LINKLOG .*HOSTCAB invited this cabinet"
    expect "nobody joined, so the invite ended there" "$d/member" "^LINKLOG .*HOSTCAB's invite is over"
    expect_not "the host did not start a linked game" "$d/master" "starting a linked game"
    expect "the host plays alone" "$d/master" "^LINKGAME gamestate=1 netgame=1 server=1 players=1 "
    expect_not "the other cabinet never joined" "$d/member" "^LINKGAME gamestate=1 netgame=1"
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
        if [ -z "$f" ]; then
            printf '  PASS  %s\n' "$c"
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
