#!/bin/sh
#
# DoomLegacy arcade cabinet -- one-command build for Unix-like systems.
#
# Detects the OS, the distribution family and the CPU, checks that everything
# needed to compile is present, writes a make_options for this machine, and
# builds.  If something is missing it says which package to install and stops
# rather than failing halfway through a compile with a header error.
#
#   ./tools/build.sh                 detect, check, build
#   ./tools/build.sh --deps          only say what is needed and how to get it
#   ./tools/build.sh --install-deps  run the install command (asks for sudo)
#   ./tools/build.sh --reconfigure   rewrite make_options even if one exists
#   ./tools/build.sh --clean         clean first
#   ./tools/build.sh --debug         debug build, into svn1749/debug/bin
#   ./tools/build.sh --jobs N        parallel compile jobs (default: all cores)
#   ./tools/build.sh --arch FLAG     override the -march flag ('none' for no flag)
#
# Written for /bin/sh: it has to run on a machine that may not have bash.
#
# See docs/arcade/building.md for what this does and why, and README.md for
# the operator-facing version.

set -eu

# --------------------------------------------------------------------------
# Where we are.  The script may be invoked from anywhere.
# --------------------------------------------------------------------------
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
top_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
build_root="$top_dir/svn1749"
src_dir="$build_root/src"

[ -f "$src_dir/Makefile" ] || {
    echo "error: $src_dir/Makefile not found." >&2
    echo "       Run this from a DoomLegacy checkout (tools/build.sh)." >&2
    exit 1
}

# --------------------------------------------------------------------------
# Options
# --------------------------------------------------------------------------
do_deps_only=0
do_install_deps=0
do_reconfigure=0
do_clean=0
do_debug=0
jobs=""
arch_override=""      # set by --arch; empty means "detect from this CPU"

while [ $# -gt 0 ]; do
    case "$1" in
      --deps)          do_deps_only=1 ;;
      --install-deps)  do_install_deps=1 ;;
      --reconfigure)   do_reconfigure=1 ;;
      --clean)         do_clean=1 ;;
      --debug)         do_debug=1 ;;
      --jobs)          shift; jobs="${1:-}" ;;
      --jobs=*)        jobs="${1#--jobs=}" ;;
      --arch)          shift; arch_override="${1:-}"; do_reconfigure=1 ;;
      --arch=*)        arch_override="${1#--arch=}"; do_reconfigure=1 ;;
      # Print the comment block at the top of this file, stopping at the first
      # line that is not a comment -- a fixed line range goes stale the moment
      # an option is added, and used to spill "set -eu" into the help text.
      -h|--help)       awk 'NR>1 { if (!/^#/) exit; sub(/^# ?/, ""); print }' "$0"; exit 0 ;;
      *) echo "error: unknown option '$1' (try --help)" >&2; exit 1 ;;
    esac
    shift
done

say()  { printf '%s\n' "$*"; }
step() { printf '\n== %s\n' "$*"; }
warn() { printf 'warning: %s\n' "$*" >&2; }
die()  { printf 'error: %s\n' "$*" >&2; exit 1; }

# --------------------------------------------------------------------------
# Detect the machine
# --------------------------------------------------------------------------
step "Detecting this machine"

uname_s=$(uname -s)
uname_m=$(uname -m)

os_family=""       # linux | macos | freebsd
distro_family=""   # debian | fedora | arch | suse | unknown  (linux only)
distro_pretty=""

case "$uname_s" in
  Linux)   os_family=linux ;;
  Darwin)  os_family=macos ;;
  FreeBSD) os_family=freebsd ;;
  *) die "unsupported system '$uname_s'.
       This script covers Linux, macOS and FreeBSD.  The Makefile can also
       target OS/2 and DOS -- see svn1749/make_options_os2 and _dos and build
       those by hand." ;;
esac

if [ "$os_family" = linux ]; then
    if [ -r /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        distro_pretty="${PRETTY_NAME:-${NAME:-Linux}}"
        # ID_LIKE is what makes derivatives work: Mint says debian, Manjaro
        # says arch, Rocky says rhel/fedora.  Check ID first, then ID_LIKE.
        for id in ${ID:-} ${ID_LIKE:-}; do
            case "$id" in
              debian|ubuntu)                distro_family=debian; break ;;
              fedora|rhel|centos)           distro_family=fedora; break ;;
              arch|archlinux)               distro_family=arch;   break ;;
              opensuse*|suse|sles)          distro_family=suse;   break ;;
            esac
        done
        [ -n "$distro_family" ] || distro_family=unknown
    else
        distro_pretty="Linux (no /etc/os-release)"
        distro_family=unknown
    fi
fi

# CPU.  -march=native is right almost everywhere and lets the compiler decide,
# but it does not exist on every target, so it is chosen per architecture
# rather than assumed.
arch_flag="-march=native"
arch_note=""
arch_src=""        # note appended to the summary line when --arch overrode it
case "$uname_m" in
  x86_64|amd64)          arch_desc="64-bit x86" ;;
  i386|i486|i586|i686)   arch_desc="32-bit x86" ;;
  aarch64|arm64)         arch_desc="64-bit ARM" ;;
  armv7*|armv6*)         arch_desc="32-bit ARM" ;;
  ppc64le|ppc64|ppc)     arch_desc="PowerPC"
                         # GCC uses -mcpu for PowerPC, not -march.
                         arch_flag="-mcpu=native" ;;
  riscv64)               arch_desc="64-bit RISC-V" ;;
  *)                     arch_desc="$uname_m"
                         arch_flag=""
                         arch_note="unrecognised CPU: building without a -march flag" ;;
esac

# Apple silicon clang does not accept -march=native.
if [ "$os_family" = macos ]; then
    case "$uname_m" in
      arm64) arch_flag=""; arch_note="Apple silicon: building without a -march flag" ;;
    esac
fi

# --arch overrides all of the above.  -march=native is right for a machine
# building for itself and wrong for a machine building for somebody else: it
# bakes in whatever the *builder's* CPU happens to support, and the binary then
# dies with "Illegal instruction" on a cabinet PC that is a few generations
# older.  A build that will be distributed -- a GitHub Actions release, or a
# binary copied to another machine -- must name a baseline instead, e.g.
#   --arch '-march=x86-64 -mtune=generic'
# Passing --arch implies --reconfigure, or an existing make_options would be
# reused and the flag silently ignored.
if [ -n "$arch_override" ]; then
    case "$arch_override" in
      none|NONE) arch_flag=""; arch_note="" ;;
      *)         arch_flag="$arch_override"; arch_note="" ;;
    esac
    arch_src=" (from --arch)"
fi

say "  system      : ${distro_pretty:-$uname_s}"
say "  cpu         : $uname_m ($arch_desc)"
say "  arch flag   : ${arch_flag:-none}${arch_src}"
[ -z "$arch_note" ] || warn "$arch_note"

# --------------------------------------------------------------------------
# What the build needs, and how to ask for it
# --------------------------------------------------------------------------
#
# Everything below is *probed*, not looked up in the package database.  That is
# deliberate: package names drift and distributions substitute.  On Fedora 42
# this very build is satisfied by sdl2-compat-devel, zlib-ng-compat-devel and
# libglvnd-devel -- none of which are named SDL2-devel, zlib-devel or
# mesa-libGL-devel.  Asking the compiler whether it can build something is the
# only question that stays true.
#
# The package lists are therefore *hints* printed when a probe fails.

# pkg_refresh runs before pkg_install where a stale package index is a real
# failure rather than a slow one.  apt resolves package versions from the index
# it last downloaded, and Debian/Ubuntu delete superseded .debs from the pool as
# soon as a new one lands -- so an index even a few days old asks the mirror for
# files that are no longer there and the install dies on a 404, naming packages
# nobody asked for:
#
#   E: Failed to fetch .../uuid-dev_2.39.3-9ubuntu6.5_amd64.deb  404  Not Found
#
# This is not hypothetical and it is not a mirror outage: a GitHub Actions
# ubuntu-24.04 runner ships an index frozen at image-build time and hits it
# within days.  (tools/build.ps1 already runs `pacman -Sy` first for the same
# reason.)  dnf and zypper expire their own metadata and need no help; plain
# `pacman -Sy` is left to the Arch user, for whom refreshing the database
# without upgrading invites a partial upgrade.
pkg_refresh=""

case "$distro_family" in
  debian) pkg_refresh="sudo apt-get update"
          pkg_install="sudo apt install -y"
          pkg_list="build-essential libsdl2-dev libsdl2-mixer-dev libzip-dev zlib1g-dev libgl1-mesa-dev libglu1-mesa-dev" ;;
  fedora) pkg_install="sudo dnf install -y"
          pkg_list="gcc make SDL2-devel SDL2_mixer-devel libzip-devel zlib-devel mesa-libGL-devel mesa-libGLU-devel" ;;
  arch)   pkg_install="sudo pacman -S --needed"
          pkg_list="base-devel sdl2 sdl2_mixer libzip zlib mesa glu" ;;
  suse)   pkg_install="sudo zypper install -y"
          pkg_list="gcc make SDL2-devel SDL2_mixer-devel libzip-devel zlib-devel Mesa-libGL-devel glu-devel" ;;
  *)      pkg_install=""
          pkg_list="" ;;
esac

if [ "$os_family" = macos ]; then
    pkg_install="brew install"
    pkg_list="sdl2 sdl2_mixer libzip"
elif [ "$os_family" = freebsd ]; then
    pkg_install="sudo pkg install"
    pkg_list="gmake sdl2 sdl2_mixer libzip mesa-libs"
fi

# --------------------------------------------------------------------------
# Probe
# --------------------------------------------------------------------------
step "Checking what is installed"

CC="${CC:-cc}"
command -v "$CC" >/dev/null 2>&1 || CC=gcc
missing=""

have_cmd() { command -v "$1" >/dev/null 2>&1; }

# Try to compile *and link* a tiny program.  Compiling alone would pass with a
# header present and the library absent, which is exactly the case that fails
# at the very end of a long build.
probe_link() {
    _name=$1; _src=$2; _flags=$3
    _tmp=$(mktemp -d 2>/dev/null || echo /tmp/dlbuild.$$)
    mkdir -p "$_tmp"
    printf '%s\n' "$_src" > "$_tmp/probe.c"
    if $CC "$_tmp/probe.c" -o "$_tmp/probe" $_flags >"$_tmp/err" 2>&1; then
        say "  ok   : $_name"
        rm -rf "$_tmp"
        return 0
    fi
    say "  MISS : $_name"
    rm -rf "$_tmp"
    return 1
}

need() { missing="$missing $1"; }

have_cmd "$CC"   || { say "  MISS : a C compiler"; need "a C compiler"; }
have_cmd make    || { say "  MISS : make"; need "make"; }
have_cmd "$CC" && say "  ok   : C compiler ($CC)"
have_cmd make    && say "  ok   : make"

# SDL2.  sdl2-config is what the Makefile itself uses to find flags.
sdl_cflags=""; sdl_libs=""
if have_cmd sdl2-config; then
    sdl_cflags=$(sdl2-config --cflags 2>/dev/null || true)
    sdl_libs=$(sdl2-config --libs 2>/dev/null || true)
    say "  ok   : sdl2-config"
elif have_cmd pkg-config && pkg-config --exists sdl2 2>/dev/null; then
    sdl_cflags=$(pkg-config --cflags sdl2)
    sdl_libs=$(pkg-config --libs sdl2)
    say "  ok   : SDL2 (via pkg-config; sdl2-config absent)"
    warn "the Makefile prefers sdl2-config -- install the SDL2 development package if the build fails"
else
    say "  MISS : SDL2 development files (no sdl2-config, no pkg-config sdl2)"
    need "SDL2"
fi

if [ -n "$sdl_libs" ]; then
    probe_link "SDL2_mixer" \
      '#include <SDL_mixer.h>
       int main(void){ Mix_Init(0); return 0; }' \
      "$sdl_cflags $sdl_libs -lSDL2_mixer" || need "SDL2_mixer"
fi

probe_link "libzip" \
  '#include <zip.h>
   int main(void){ zip_open("x",0,0); return 0; }' \
  "-lzip" || need "libzip"

probe_link "zlib" \
  '#include <zlib.h>
   int main(void){ return (int)zlibVersion()[0]; }' \
  "-lz" || need "zlib"

if [ "$os_family" = macos ]; then
    probe_link "OpenGL" \
      '#include <OpenGL/gl.h>
       int main(void){ glFlush(); return 0; }' \
      "-framework OpenGL" || need "OpenGL"
else
    probe_link "OpenGL (GL and GLU)" \
      '#include <GL/gl.h>
       #include <GL/glu.h>
       int main(void){ glFlush(); gluErrorString(0); return 0; }' \
      "-lGL -lGLU" || need "OpenGL/GLU"
fi

if [ -n "$missing" ]; then
    say ""
    say "Missing:$missing"
    say ""
    if [ -n "$pkg_list" ]; then
        say "Install with:"
        # Print the refresh too -- someone copying this by hand off a stale
        # index hits the same 404 the script would have.
        [ -z "$pkg_refresh" ] || say "    $pkg_refresh"
        say "    $pkg_install $pkg_list"
        if [ "$distro_family" = fedora ]; then
            say ""
            say "If a package name is not found (Fedora renames these -- SDL2-devel is"
            say "provided by sdl2-compat-devel, zlib-devel by zlib-ng-compat-devel,"
            say "mesa-libGL-devel by libglvnd-devel), dnf will also resolve by file:"
            say "    sudo dnf install /usr/bin/sdl2-config /usr/include/zlib.h /usr/include/GL/gl.h"
        fi
    else
        say "This script does not know the package names for this system."
        say "You need: a C compiler, make, and the *development* packages for"
        say "SDL2, SDL2_mixer, libzip, zlib and OpenGL (GL and GLU)."
    fi
    say ""
    if [ "$do_install_deps" = 1 ] && [ -n "$pkg_list" ]; then
        step "Installing dependencies"
        if [ -n "$pkg_refresh" ]; then
            say "  refreshing the package index"
            # shellcheck disable=SC2086
            $pkg_refresh || warn "could not refresh the package index; the install may 404"
        fi
        # Deliberately not quoted: the list is several words.
        # shellcheck disable=SC2086
        $pkg_install $pkg_list
        say ""
        say "Dependencies installed.  Run the script again to build."
        exit 0
    fi
    say "Re-run with --install-deps to install them, then build."
    exit 1
fi

[ "$do_deps_only" = 0 ] || { say ""; say "All dependencies present."; exit 0; }

# --------------------------------------------------------------------------
# make_options
# --------------------------------------------------------------------------
step "Configuring"

template=""
case "$os_family" in
  linux|freebsd) template="$build_root/make_options_nix" ;;
  macos)         template="$build_root/make_options_mac" ;;
esac
[ -f "$template" ] || die "template $template not found"

opts="$build_root/make_options"

# [Arcade] An existing make_options is only reusable if it was written for
# *this* platform.  The tree lives in a synced folder (Dropbox) shared with a
# Windows machine, and make_options is gitignored -- which stops git from
# carrying it between machines but does nothing about the sync, so the other
# platform's file arrives and gets reused.  Every source file still compiles
# (the options are wrong only in what they link against) and the build dies at
# the link with a list of libraries that look missing but simply do not exist
# on this platform.  Each template carries exactly one uncommented OS= line, so
# comparing them identifies a foreign file with no marker of our own.
template_os=$(grep -m1 '^OS=' "$template" 2>/dev/null || true)
existing_os=""
[ -f "$opts" ] && existing_os=$(grep -m1 '^OS=' "$opts" 2>/dev/null || true)
foreign_opts=0
if [ -n "$existing_os" ] && [ -n "$template_os" ] && [ "$existing_os" != "$template_os" ]; then
    foreign_opts=1
fi

if [ -f "$opts" ] && [ "$do_reconfigure" = 0 ] && [ "$foreign_opts" = 0 ]; then
    # make_options is machine-local and gitignored, and an operator may have
    # tuned it.  Never silently overwrite somebody's settings.
    say "  using the existing $opts"
    say "  (delete it or pass --reconfigure to regenerate)"
else
    if [ "$foreign_opts" = 1 ]; then
        say "  the existing make_options says '$existing_os', not '$template_os' --"
        say "  it was written for another platform (a synced folder will do this)."
        say "  Regenerating it; the old one is kept as make_options.foreign."
        cp "$opts" "$opts.foreign"
        # Whatever synced make_options here synced ../objs with it, and those
        # objects are for the other platform.  make compares timestamps, not
        # targets, so it considers them up to date and links them -- and the
        # error names neither the sync nor the objects (mingw objects in a
        # glibc link give "relocation truncated to fit" and undefined
        # references to __isoc23_* and __ctype_*).  A platform switch
        # invalidates every object in the tree.
        say "  objects from the other platform are unusable -- forcing a clean."
        do_clean=1
    fi
    say "  writing $opts from $(basename "$template")"

    # The three edits the stock template needs on a modern toolchain, each of
    # which is otherwise a hard build failure rather than a warning:
    #
    #   SDL2=1        the stock file targets SDL 1.2, which no current distro
    #                 ships; with this the Makefile finds sdl2-config.
    #   ARCH=         the stock -march=i686 is 32-bit only and fails on x86-64
    #                 with "CPU you selected does not support x86-64".
    #   ENV_CFLAGS=   GCC 15 defaults to -std=gnu23, where true/false are
    #                 keywords, which breaks this codebase's own
    #                 typedef enum {false,true} boolean.  -g is added so that
    #                 a crash on an unattended cabinet gives a backtrace with
    #                 file and line rather than bare function names.
    awk -v archflag="$arch_flag" '
        /^ARCH=/            { if (archflag != "") print "ARCH=" archflag;
                              else print "# ARCH= (none for this cpu)";
                              next }
        /^# *SDL2=1/        { print "SDL2=1"; next }
        { print }
        END { print "";
              print "# Added by tools/build.sh";
              print "ENV_CFLAGS=-std=gnu17 -g" }
    ' "$template" > "$opts"

    grep -q '^SDL2=1' "$opts" || echo "SDL2=1" >> "$opts"
    # The same net for ARCH.  make_options_nix carries one live ARCH= line so
    # the awk above finds it, but make_options_win has every one of them
    # commented out -- there the replacement never fires, no ARCH line is
    # written, and the Makefile's `ifdef ARCH` compiles with no -march switch
    # while the script reports the flag it thought it set.  A template edit
    # would do the same here, and it would be just as quiet.
    if [ -n "$arch_flag" ]; then
        grep -q '^ARCH=' "$opts" || echo "ARCH=$arch_flag" >> "$opts"
    fi
    say "  SDL2=1, ARCH=${arch_flag:-none}, ENV_CFLAGS=-std=gnu17 -g"
fi

# --------------------------------------------------------------------------
# Build
# --------------------------------------------------------------------------
if [ -z "$jobs" ]; then
    jobs=$( (nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) )
fi

MAKE=make
have_cmd gmake && [ "$os_family" = freebsd ] && MAKE=gmake

build_args=""
if [ "$do_debug" = 1 ]; then
    build_args="DEBUG=1 BUILD=debug"

    # BUILD=<dir> looks for make_options *inside that directory*
    # (MAKE_OPTIONS = $(BUILD_DIR)make_options in the Makefile), not in the
    # build root.  Without a copy there the build stops with the entirely
    # unhelpful `*** "Unknown OS: " .  Stop.` -- it has not failed to detect
    # anything, it simply never read a make_options at all.
    mkdir -p "$build_root/debug"
    cp "$build_root/make_options" "$build_root/debug/make_options"
fi

step "Building (${jobs} jobs)"

# The output directories must exist before anything compiles, or make fails on
# a .dep file with "No such file or directory".  `dirs` only works from the
# build root -- in src/ it is an empty placeholder target.  It needs the same
# BUILD= as the compile, or it makes the directories for the wrong tree.
# shellcheck disable=SC2086
( cd "$build_root" && $MAKE dirs $build_args ) >/dev/null 2>&1 || true

if [ "$do_clean" = 1 ]; then
    say "  cleaning"
    ( cd "$src_dir" && $MAKE clean ) >/dev/null 2>&1 || true
fi

# `make depend` must run serially before any parallel build.  Every ../dep/*.dep
# rule pipes through the *same* intermediate ../dep/sed.dep and then moves it,
# so two parallel dep rules clobber each other and the build dies with
# "mv: cannot stat '../dep/sed.dep'" -- an error that points nowhere near the
# cause.  The compile phase parallelises fine.
say "  resolving dependencies (serial -- this phase cannot be parallelised)"
# shellcheck disable=SC2086
( cd "$src_dir" && $MAKE depend $build_args ) >/dev/null 2>&1 || true

say "  compiling"
# shellcheck disable=SC2086
if ! ( cd "$src_dir" && $MAKE -j"$jobs" $build_args ); then
    say ""
    die "the build failed.  The compiler output above says why.
       If it mentions ../dep/sed.dep, run:  cd svn1749/src && make depend && make
       If it mentions a missing header, re-run this script with --deps."
fi

# --------------------------------------------------------------------------
# Done
# --------------------------------------------------------------------------
binary="$build_root/bin/doomlegacy"
[ "$do_debug" = 0 ] || binary="$build_root/debug/bin/doomlegacy"

step "Done"
if [ -x "$binary" ]; then
    say "  built: $binary"
    say ""
    say "  Do not run it from the build tree -- it looks for its data next to"
    say "  the binary.  Copy everything from $build_root/bin into a run"
    say "  directory alongside legacy.wad and an IWAD (DOOM.WAD, DOOM2.WAD...):"
    say ""
    say "      cp -a $build_root/bin/* ~/games/doom/"
    say "      cd ~/games/doom && ./doomlegacy"
    say ""
    say "  An operator session that can change settings is:  ./doomlegacy -devmode"
else
    die "the build reported success but $binary is missing."
fi
