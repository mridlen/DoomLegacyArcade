#!/usr/bin/env bash
# [Arcade] Rebuild the BSP nodes of the cabinet's IWADs with ZDBSP.
#
# Why: the vanilla NODES lump stores each partition as a rounded copy of the
# seg the builder split on, and the rounding is enough to put a viewpoint that
# is a unit or two from a partition on the wrong side.  The BSP is then walked
# out of order and a far wall occludes a near one, which is the classic slime
# trail -- the green stripe down the nukage pool on E1M1.  ZDBSP rebuilds the
# tree with fixed-point vertices, so the precision is never lost.
# See docs/arcade/gotchas.md.
#
# Output is a small PWAD per IWAD, carrying only the maps, loaded with -file.
#
#   ./tools/rebuild-nodes.sh                    rebuild every IWAD found
#   ./tools/rebuild-nodes.sh DOOM.WAD           just this one
#   ./tools/rebuild-nodes.sh --dir ~/games/doom where the IWADs live
#   ./tools/rebuild-nodes.sh --zdbsp /path/zdbsp   use an existing binary
#
# LICENSING -- read before shipping anything this produces.
#   ZDBSP is GPLv2-or-later (Randy Heit).  It is used here as an external
#   tool, so it places no licence obligation on DoomLegacy or on its output.
#   The *output is another matter*: a rebuilt map carries id Software's
#   LINEDEFS/SIDEDEFS/VERTEXES/SECTORS verbatim and its nodes are a derivative
#   of them.  Generating it locally from an IWAD you own is ordinary personal
#   use.  Committing it, or putting it in a GitHub release, is redistributing
#   id's map data and must not happen.  The *-nodes.wad names are gitignored
#   for that reason -- do not force-add them.

set -euo pipefail

here=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" && pwd )
waddir="$HOME/games/doom"
zdbsp=""
wads=()

while [ $# -gt 0 ]; do
    case "$1" in
        --dir)    waddir="$2"; shift 2 ;;
        --zdbsp)  zdbsp="$2";  shift 2 ;;
        -h|--help)
            awk 'NR>1 { if (!/^#/) exit; sub(/^# ?/, ""); print }' "$0"; exit 0 ;;
        -*) echo "error: unknown option '$1' (try --help)" >&2; exit 1 ;;
        *)  wads+=("$1"); shift ;;
    esac
done

say () { printf '%s\n' "$*"; }

# ---- find or build zdbsp -------------------------------------------------
if [ -z "$zdbsp" ]; then
    if command -v zdbsp >/dev/null 2>&1; then
        zdbsp=$( command -v zdbsp )
    else
        # gitignored working copy, kept out of the tree proper
        cache="$here/.zdbsp"
        if [ ! -x "$cache/build/zdbsp" ]; then
            say "zdbsp not found; building it into tools/.zdbsp (needs git, cmake, a C++ compiler)"
            if [ ! -d "$cache" ]; then
                git clone --depth 1 https://github.com/ZDoom/zdbsp.git "$cache"
            fi
            mkdir -p "$cache/build"
            ( cd "$cache/build" && cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null && make -j"$(nproc)" >/dev/null )
        fi
        zdbsp="$cache/build/zdbsp"
    fi
fi
[ -x "$zdbsp" ] || { echo "error: no usable zdbsp at '$zdbsp'" >&2; exit 1; }
say "using zdbsp: $zdbsp"

# ---- pick the IWADs ------------------------------------------------------
if [ ${#wads[@]} -eq 0 ]; then
    for f in DOOM.WAD DOOM2.WAD PLUTONIA.WAD TNT.WAD DOOMU.WAD HERETIC.WAD; do
        [ -f "$waddir/$f" ] && wads+=("$waddir/$f")
    done
fi
[ ${#wads[@]} -gt 0 ] || { echo "error: no IWADs found in $waddir" >&2; exit 1; }

tmp=$( mktemp -d )
trap 'rm -rf "$tmp"' EXIT

say ""
for w in "${wads[@]}"; do
    [ -f "$w" ] || w="$waddir/$w"
    [ -f "$w" ] || { echo "error: no such wad: $w" >&2; exit 1; }
    base=$( basename "$w" )
    stem=${base%.*}
    out="$waddir/${stem}-nodes.wad"

    say "$base"
    # -X extended (XNOD) nodes, which p_extnodes.c reads; -E leave REJECT alone
    # zdbsp draws a progress bar; keep it quiet but show it if it fails
    if ! "$zdbsp" -X -E -t -o "$tmp/full.wad" "$w" > "$tmp/zdbsp.log" 2>&1; then
        cat "$tmp/zdbsp.log" >&2
        echo "error: zdbsp failed on $base" >&2
        exit 1
    fi
    python3 "$here/wad_maps.py" "$tmp/full.wad" "$out"
done

say ""
say "Done.  Load a rebuilt set alongside its IWAD, e.g.:"
say "    ./doomlegacy -file ${waddir}/DOOM-nodes.wad"
say ""
say "These files contain id Software map data.  Keep them local:"
say "do not commit them and do not put them in a release."
