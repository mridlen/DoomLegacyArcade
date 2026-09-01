<#
.SYNOPSIS
    DoomLegacy arcade cabinet -- one-command build for Windows.

.DESCRIPTION
    Detects the CPU, finds an MSYS2/MinGW toolchain, checks that everything
    needed to compile is present, and builds.  If something is missing it says
    exactly what to install and how, rather than failing halfway through a
    compile with a header error.

    DoomLegacy is a GNU Make project written for a Unix-like toolchain, so on
    Windows it is built with MinGW-w64 under MSYS2.  That is not a limitation
    of this script -- Visual Studio cannot build this tree as it stands.

.PARAMETER Deps
    Only report what is needed and how to get it.  Do not build.

.PARAMETER InstallDeps
    Install the MSYS2 packages needed (MSYS2 itself must already be present;
    the script prints how to get it if not).

.PARAMETER Reconfigure
    Rewrite make_options even if one already exists.

.PARAMETER Clean
    Clean before building.

.PARAMETER Jobs
    Parallel compile jobs.  Defaults to the number of processors.

.PARAMETER Arch
    Override the -march flag written into make_options.  Use 'none' for no
    flag at all.  The default is -march=native, which is right for a machine
    building for itself and wrong for one building for somebody else: it bakes
    in whatever the *builder's* CPU supports, and the binary then dies with an
    illegal-instruction fault on an older cabinet PC.  A build that will be
    distributed (a GitHub Actions release, or a binary copied elsewhere) should
    name a baseline instead:  -Arch '-march=x86-64 -mtune=generic'.
    Passing -Arch implies -Reconfigure.

.EXAMPLE
    .\tools\build.ps1
    .\tools\build.ps1 -Deps
    .\tools\build.ps1 -InstallDeps

.NOTES
    Detection and MSYS2 discovery are confirmed working on Windows 11 x86_64
    (reported: "Microsoft Windows 11 Pro / AMD64 / MSYS2 ucrt64 / found
    C:\msys64").  The build itself has still not been run to completion on
    Windows -- the first run that gets past dependency installation is the
    test.  tools/build.sh is the Linux/macOS equivalent and is fully tested.
#>

[CmdletBinding()]
param(
    [switch]$Deps,
    [switch]$InstallDeps,
    [switch]$Reconfigure,
    [switch]$Clean,
    [int]$Jobs = 0,
    [string]$Arch = ''
)

$ErrorActionPreference = 'Stop'

function Say  { param($m) Write-Host $m }
function Step { param($m) Write-Host ""; Write-Host "== $m" -ForegroundColor Cyan }
function Warn { param($m) Write-Host "warning: $m" -ForegroundColor Yellow }
function Die  { param($m) Write-Host "error: $m" -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------------
# Where we are
# ---------------------------------------------------------------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TopDir    = Split-Path -Parent $ScriptDir
$BuildRoot = Join-Path $TopDir 'svn1749'
$SrcDir    = Join-Path $BuildRoot 'src'

if (-not (Test-Path (Join-Path $SrcDir 'Makefile'))) {
    Die "$SrcDir\Makefile not found. Run this from a DoomLegacy checkout: .\tools\build.ps1"
}

# ---------------------------------------------------------------------------
# Detect the machine
# ---------------------------------------------------------------------------
Step "Detecting this machine"

$osCaption = (Get-CimInstance Win32_OperatingSystem).Caption
$archRaw   = $env:PROCESSOR_ARCHITECTURE
if ($env:PROCESSOR_ARCHITEW6432) { $archRaw = $env:PROCESSOR_ARCHITEW6432 }

# MSYS2 subsystem to build under, and the package prefix that goes with it.
# UCRT64 is the current default for 64-bit x86; MINGW32 is the 32-bit one that
# an old cabinet PC may need; CLANGARM64 covers Windows on ARM.
switch ($archRaw) {
    'AMD64' { $archDesc='64-bit x86'; $msysEnv='ucrt64';     $pkgPrefix='mingw-w64-ucrt-x86_64';  $archFlag='-march=native' }
    'x86'   { $archDesc='32-bit x86'; $msysEnv='mingw32';    $pkgPrefix='mingw-w64-i686';         $archFlag='-march=i686' }
    'ARM64' { $archDesc='64-bit ARM'; $msysEnv='clangarm64'; $pkgPrefix='mingw-w64-clang-aarch64';$archFlag='' }
    default { $archDesc=$archRaw;     $msysEnv='ucrt64';     $pkgPrefix='mingw-w64-ucrt-x86_64';  $archFlag=''
              Warn "unrecognised architecture '$archRaw'; assuming 64-bit x86" }
}

# -Arch overrides the detected flag.  See the .PARAMETER note above for why a
# distributable build must not use -march=native.  It implies -Reconfigure, or
# an existing make_options would be reused and the flag silently ignored.
if ($Arch) {
    if ($Arch -ieq 'none') { $archFlag = '' } else { $archFlag = $Arch }
    $Reconfigure = $true
    $archSrc = ' (from -Arch)'
}
if (-not $archSrc) { $archSrc = '' }

Say "  system    : $osCaption"
Say "  cpu       : $archRaw ($archDesc)"
Say "  arch flag : $(if($archFlag){$archFlag}else{'none'})$archSrc"
Say "  toolchain : MSYS2 $msysEnv"

# ---------------------------------------------------------------------------
# Find MSYS2
# ---------------------------------------------------------------------------
Step "Looking for MSYS2"

$msysRoot = $null
foreach ($c in @($env:MSYS2_ROOT, 'C:\msys64', 'C:\msys32', "$env:USERPROFILE\msys64")) {
    if ($c -and (Test-Path (Join-Path $c 'usr\bin\bash.exe'))) { $msysRoot = $c; break }
}

if (-not $msysRoot) {
    Say "  MSYS2 not found."
    Say ""
    Say "DoomLegacy is a GNU Make project and needs a Unix-like toolchain."
    Say "Install MSYS2 -- it is a single installer and the only prerequisite:"
    Say ""
    Say "    winget install --id MSYS2.MSYS2"
    Say ""
    Say "or download it from https://www.msys2.org/ ."
    Say "Then run this script again; it will install the compiler and libraries."
    exit 1
}
Say "  found: $msysRoot"

$bash = Join-Path $msysRoot 'usr\bin\bash.exe'

# Run a command inside the chosen MinGW environment.  MSYSTEM selects which
# compiler and libraries are on PATH; without it the MSYS shell has its own
# gcc that produces binaries depending on the MSYS runtime, which is not what
# a distributable Windows build wants.
# $ErrorActionPreference is 'Stop' for this script, and under that setting
# *anything* a native program writes to stderr becomes a terminating
# PowerShell error.  A probe for a missing tool writes to stderr by
# definition, so the first missing package killed the script before it could
# print the list of what to install -- the one message the operator actually
# needed.  Probes are expected to fail; only the exit code is the answer.
function Invoke-Msys {
    param([string]$Command, [switch]$Quiet)
    $env:MSYSTEM = $msysEnv.ToUpper()
    $env:CHERE_INVOKING = '1'
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        if ($Quiet) {
            & $bash -lc $Command 2>&1 | Out-Null
        } else {
            & $bash -lc $Command 2>&1 | ForEach-Object { Write-Host $_ }
        }
        return $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $prevEAP }
}

# Run a shell command and return its trimmed output (empty on failure).
function Get-Msys {
    param([string]$Command)
    $env:MSYSTEM = $msysEnv.ToUpper()
    $env:CHERE_INVOKING = '1'
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & $bash -lc ("{ " + $Command + " ; } 2>/dev/null") 2>$null
        if ($null -eq $out) { return '' }
        return ($out | Out-String).Trim()
    }
    finally { $ErrorActionPreference = $prevEAP }
}

# Run a shell test and report only true/false.  The command is wrapped in
# braces before the redirection is applied, because in `a || b >/dev/null`
# the redirection binds to *b alone* -- so a missing `a` still printed
# "command not found" to the console.  That is what leaked pkg-config errors
# out of the probes.
function Test-Msys {
    param([string]$Command)
    return (Invoke-Msys -Command ("{ " + $Command + " ; } >/dev/null 2>&1") -Quiet) -eq 0
}

# A Windows path as MSYS sees it: C:\a\b -> /c/a/b
function ConvertTo-MsysPath {
    param([string]$P)
    $full = (Resolve-Path $P).Path
    $drive = $full.Substring(0,1).ToLower()
    return '/' + $drive + ($full.Substring(2) -replace '\\','/')
}

# ---------------------------------------------------------------------------
# Check the toolchain and libraries
# ---------------------------------------------------------------------------
Step "Checking what is installed"

# pkg-config is deliberately not used.  A freshly installed MSYS2 does not
# have it -- it arrives with the toolchain -- so probing through it asks a
# question that cannot be answered on exactly the machine that needs the most
# help.  The toolchain is checked first, and the libraries are then checked the
# way build.sh does it: by compiling *and linking* a three-line program, which
# is the only test that stays true when package names change.

$missingPkgs = @()
$wrongGcc = $false

# -- phase 1: the toolchain.  Nothing else can be probed without it. --
# [Arcade] Finding "a gcc" is not enough -- it has to be the gcc for the
# subsystem whose libraries we are about to link against.
#
# MSYS2 ships its own compiler at /usr/bin/gcc, targeting the MSYS runtime.
# If the mingw toolchain is not installed, that one is found instead, and it
# searches /usr/include and /usr/lib -- it cannot see /ucrt64 at all.  The
# result is a maddening half-success: zlib and the OpenGL import libraries
# exist in the MSYS tree too, so those probes pass, while SDL2, SDL2_mixer and
# libzip are reported missing no matter how many times pacman says they are
# installed, because they live only under /ucrt64.  That is exactly what one
# machine reported.
#
# gcc -dumpmachine separates them cleanly:
#     x86_64-w64-mingw32   the toolchain we want
#     x86_64-pc-msys       MSYS2's own, which cannot link mingw packages
$haveGcc  = Test-Msys 'command -v gcc'
$gccTriple = ''
$gccPath   = ''
if ($haveGcc) {
    $gccTriple = Get-Msys 'gcc -dumpmachine'
    $gccPath   = Get-Msys 'command -v gcc'
    if ($gccTriple -notmatch 'mingw') {
        $haveGcc = $false
        $wrongGcc = $true
    }
}
# [Arcade] make goes by more than one name here, and getting this wrong
# reports an installed package as missing.
#
#   make           the MSYS package `make`.  This is the one to want: the
#                  tree is a Unix Makefile that shells out to sh, sed, awk,
#                  mv and cp, and the MSYS make understands the paths those
#                  produce.
#   mingw32-make   what the *mingw* package `mingw-w64-<arch>-make` installs.
#                  Same GNU make, different name -- the MinGW convention, so
#                  that it cannot collide with a native `make` on PATH.  A
#                  machine with this installed had "MISS : make" reported at
#                  it while pacman insisted the package was up to date.
#   gmake          some setups provide this alias.
$makeCmd = ''
foreach ($m in @('make','mingw32-make','gmake')) {
    if (Test-Msys "command -v $m") { $makeCmd = $m; break }
}
$haveMake = ($makeCmd -ne '')
if ($haveGcc) {
    Say "  ok   : C compiler (gcc, $gccTriple)"
} elseif ($wrongGcc) {
    Say "  MISS : C compiler for $msysEnv"
    Say "         found $gccPath, which targets '$gccTriple' -- that is MSYS2's"
    Say "         own compiler, not the $msysEnv toolchain. It cannot see the"
    Say "         mingw-w64 packages, so the libraries below will be reported"
    Say "         missing however many times pacman says they are installed."
    $missingPkgs += "$pkgPrefix-gcc"
} else {
    Say "  MISS : C compiler"
    $missingPkgs += "$pkgPrefix-gcc"
}
# The MSYS package is named plainly `make`, with no mingw-w64 prefix -- it is
# not part of the toolchain, it is a shell tool like sed and awk.
if ($haveMake) {
    Say "  ok   : make ($makeCmd)"
    if ($makeCmd -ne 'make') {
        # mingw32-make is a *native* Windows GNU make: it is not MSYS-aware,
        # so POSIX paths and the sh/sed/awk recipes this Makefile uses can
        # trip it.  It is accepted because it is what is installed, but the
        # MSYS package is the one that matches this tree.
        Warn "using $makeCmd (a native Windows make). If the build fails on paths or on
         sh/sed/awk, install the MSYS make instead:  pacman -S make"
    }
} else { Say "  MISS : make"; $missingPkgs += 'make' }

if (-not $haveGcc) {
    # Without a compiler the library probes below would all report MISS for
    # the same single reason, which reads as six problems instead of one.
    # This is worth being loud about: a machine with SDL2, SDL2_mixer and
    # libzip already installed still sees them listed as missing here, purely
    # because there is nothing to compile a test with yet.
    Say "         (the libraries cannot be checked until the compiler is"
    Say "          installed -- they are listed below for that reason alone,"
    Say "          not because they are known to be absent)"
    $missingPkgs += @("$pkgPrefix-SDL2", "$pkgPrefix-SDL2_mixer", "$pkgPrefix-libzip", "$pkgPrefix-zlib")
} else {
    # -- phase 2: the libraries, by compile-and-link. --
    #
    # [Arcade] Two Windows-only traps live in these five lines, and both of
    # them report an installed package as missing:
    #
    # 1. The probe source must not contain a double quote.  PowerShell rebuilds
    #    the argument string when it launches a native program (bash.exe), and
    #    it eats unescaped `"` on the way -- so `zip_open("x",0,0)` reached the
    #    compiler as `zip_open(x,0,0)`, which fails on an undeclared `x` and
    #    reported libzip missing on a machine where pacman said it was up to
    #    date.  Nothing in the compiler output would have hinted at PowerShell.
    #    Keep every Src below quote-free; a null pointer argument works as well
    #    as a string literal for a link test.
    #
    # 2. `int main(void)` is wrong for the SDL probes on Windows.  Here
    #    `sdl2-config --cflags` adds `-Dmain=SDL_main`, which renames the
    #    probe's main and then collides with SDL_main.h's own
    #    `int SDL_main(int, char **)` declaration:
    #        error: conflicting types for 'SDL_main'; have 'int(void)'
    #    SDL_MAIN_HANDLED does not prevent that -- the declaration is there
    #    either way.  Declaring the probe with argc/argv matches it.  This is
    #    why build.sh can use `main(void)` and this script cannot: on Linux
    #    sdl2-config emits no such define.
    $probes = @(
        @{ Name='SDL2';       Pkg="$pkgPrefix-SDL2"
           Src='#define SDL_MAIN_HANDLED\n#include <SDL.h>\nint main(int argc, char **argv){(void)argc;(void)argv;SDL_Init(0);return 0;}\n'
           Flags='$(sdl2-config --cflags --libs 2>/dev/null || echo -lSDL2)' },
        @{ Name='SDL2_mixer'; Pkg="$pkgPrefix-SDL2_mixer"
           Src='#define SDL_MAIN_HANDLED\n#include <SDL.h>\n#include <SDL_mixer.h>\nint main(int argc, char **argv){(void)argc;(void)argv;Mix_Init(0);return 0;}\n'
           Flags='$(sdl2-config --cflags --libs 2>/dev/null || echo -lSDL2) -lSDL2_mixer' },
        @{ Name='libzip';     Pkg="$pkgPrefix-libzip"
           Src='#include <zip.h>\nint main(void){zip_open(0,0,0);return 0;}\n'
           Flags='-lzip' },
        @{ Name='zlib';       Pkg="$pkgPrefix-zlib"
           Src='#include <zlib.h>\nint main(void){return (int)zlibVersion()[0];}\n'
           Flags='-lz' },
        @{ Name='OpenGL (GL and GLU)'; Pkg="$pkgPrefix-gcc"
           Src='#include <GL/gl.h>\n#include <GL/glu.h>\nint main(void){glFlush();gluErrorString(0);return 0;}\n'
           Flags='-lopengl32 -lglu32' }
    )
    foreach ($p in $probes) {
        $cmd = "printf '" + $p.Src + "' > /tmp/dlprobe.c && gcc /tmp/dlprobe.c -o /tmp/dlprobe.exe " + $p.Flags
        if (Test-Msys $cmd) { Say ("  ok   : " + $p.Name) }
        else { Say ("  MISS : " + $p.Name); $missingPkgs += $p.Pkg }
    }
}

if ($missingPkgs.Count -gt 0) {
    $list = ($missingPkgs | Select-Object -Unique) -join ' '
    Say ""
    Say "Missing packages: $list"
    Say ""
    Say "Install them from an MSYS2 shell with:"
    Say "    pacman -S --needed $list"
    Say ""
    if ($wrongGcc) {
        Say "The libraries above are almost certainly installed already: they are"
        Say "reported missing because the compiler that was found cannot see them."
        Say "Installing $pkgPrefix-gcc should resolve all of it at once."
        Say ""
    }
    Say "A freshly installed MSYS2 has none of these -- it ships only its own"
    Say "base environment, so this is the normal first-run state, not a fault."
    Say "If pacman reports nothing to do or cannot find a package, update it"
    Say "first (this may ask you to close and reopen the shell):"
    Say "    pacman -Syu"
    Say ""
    if ($InstallDeps) {
        Step "Installing"
        # -Syu first: on a fresh MSYS2 the package databases are older than the
        # packages being asked for, and pacman will otherwise say it cannot
        # find them.
        Invoke-Msys -Command "pacman -Sy --noconfirm" | Out-Null
        $rc = Invoke-Msys -Command "pacman -S --needed --noconfirm $list"
        if ($rc -ne 0) { Die "pacman failed (exit $rc).
       Try it by hand in an MSYS2 shell:  pacman -Syu  then  pacman -S --needed $list" }
        Say ""
        Say "Installed. Run the script again to build."
        exit 0
    }
    Say "Re-run with -InstallDeps to install them, then build."
    exit 1
}

if ($Deps) { Say ""; Say "All dependencies present."; exit 0 }

# ---------------------------------------------------------------------------
# make_options
# ---------------------------------------------------------------------------
Step "Configuring"

$template = Join-Path $BuildRoot 'make_options_win'
$opts     = Join-Path $BuildRoot 'make_options'
if (-not (Test-Path $template)) { Die "template $template not found" }

# [Arcade] An existing make_options is only reusable if it was written for
# *this* platform.  The tree is kept in a synced folder (Dropbox) shared with
# the Linux cabinet, and make_options is gitignored -- which stops git from
# carrying it between machines but does nothing about the sync, so the Linux
# file lands here and this script happily reused it.
#
# Nothing about that failure names the cause.  Every one of the ~120 source
# files compiles, because the Linux options are wrong only in what they link
# against; the build then dies at the very end with
#     ld.exe: cannot find -lGL / -lGLU / -ldl
# which reads as three missing packages.  They are not installed on Windows
# and never will be: the OpenGL import libraries here are -lopengl32 -lglu32,
# and there is no libdl at all.  The tell is -DLINUX in the compile lines.
#
# The templates each carry exactly one uncommented OS= line, so comparing the
# existing file's against the template's identifies a foreign make_options
# without needing a marker of our own (and works on files written before this
# check existed).
$templateOs = ((Get-Content $template) -match '^OS=' | Select-Object -First 1)
$existingOs = ''
if (Test-Path $opts) {
    $existingOs = ((Get-Content $opts) -match '^OS=' | Select-Object -First 1)
}
$foreignOpts = ($existingOs -ne '') -and ($templateOs -ne '') -and ($existingOs -ne $templateOs)

if ((Test-Path $opts) -and -not $Reconfigure -and -not $foreignOpts) {
    # make_options is machine-local and gitignored, and may have been tuned.
    Say "  using the existing make_options"
    Say "  (delete it or pass -Reconfigure to regenerate)"
} else {
    if ($foreignOpts) {
        Say "  the existing make_options says '$existingOs', not '$templateOs' --"
        Say "  it was written for another platform (a synced folder will do this)."
        Say "  Regenerating it for Windows; the old one is kept as make_options.foreign."
        Copy-Item -Path $opts -Destination "$opts.foreign" -Force
        # Whatever synced make_options here synced ../objs with it, and those
        # objects are for the other platform.  make compares timestamps, not
        # targets, so it considers them up to date and links them -- and the
        # error names neither the sync nor the objects:
        #     relocation truncated to fit: R_X86_64_32 against `joystick_path'
        #     undefined reference to `__isoc23_sscanf'  (that is glibc)
        # with source paths under /home/mridlen. A platform switch invalidates
        # every object in the tree, so force the clean rather than leaving the
        # operator to work that out from a linker error.
        Say "  objects from the other platform are unusable -- forcing a clean."
        $Clean = $true
    }
    Say "  writing make_options from make_options_win"
    $lines = Get-Content $template
    $out = foreach ($line in $lines) {
        if     ($line -match '^ARCH=')      { if ($archFlag) { "ARCH=$archFlag" } else { "# ARCH= (none for this cpu)" } }
        elseif ($line -match '^# *SDL2=1')  { 'SDL2=1' }
        else                                { $line }
    }
    $out += ''
    $out += '# Added by tools/build.ps1'
    # -std=gnu17: GCC 15 defaults to gnu23, where true/false are keywords,
    # which breaks this codebase's own typedef enum {false,true} boolean.
    # -g so a crash gives a backtrace with file and line.
    $out += 'ENV_CFLAGS=-std=gnu17 -g'
    if (-not ($out -match '^SDL2=1')) { $out += 'SDL2=1' }
    # The same safety net for ARCH, and on Windows it is not a net but the only
    # thing that works: make_options_win has *every* ARCH= line commented out
    # (make_options_nix carries one live), so the '^ARCH=' replacement above
    # never fires and no ARCH line reaches the file at all.  The Makefile's
    # `ifdef ARCH` then leaves CFLAGS empty and compiles with no -march switch,
    # while this script cheerfully reports the flag it thought it had set.
    # Both the detected CPU and an explicit -Arch were being discarded in
    # silence -- which is exactly the failure -Arch exists to prevent, so the
    # CI check on make_options is what found it.
    if ($archFlag -and -not ($out -match '^ARCH=')) { $out += "ARCH=$archFlag" }
    Set-Content -Path $opts -Value $out -Encoding ASCII
    Say "  SDL2=1, ARCH=$(if($archFlag){$archFlag}else{'none'}), ENV_CFLAGS=-std=gnu17 -g"
}

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
if ($Jobs -le 0) { $Jobs = [int]$env:NUMBER_OF_PROCESSORS; if ($Jobs -le 0) { $Jobs = 2 } }

$msysBuildRoot = ConvertTo-MsysPath $BuildRoot
$msysSrc       = ConvertTo-MsysPath $SrcDir

Step "Building ($Jobs jobs)"

# Output directories must exist first or make fails on a .dep file.  `dirs`
# only works from the build root; in src/ it is an empty placeholder target.
Invoke-Msys -Command "cd '$msysBuildRoot' && $makeCmd dirs" -Quiet | Out-Null

if ($Clean) {
    Say "  cleaning"
    Invoke-Msys -Command "cd '$msysSrc' && $makeCmd clean" -Quiet | Out-Null
}

# `make depend` must run serially before any parallel build: every ../dep/*.dep
# rule pipes through the same intermediate ../dep/sed.dep and then moves it, so
# parallel dep rules clobber each other and the build dies with
# "mv: cannot stat '../dep/sed.dep'" -- an error that points nowhere near the
# cause.  The compile phase parallelises fine.
Say "  resolving dependencies (serial -- this phase cannot be parallelised)"
Invoke-Msys -Command "cd '$msysSrc' && $makeCmd depend" -Quiet | Out-Null

Say "  compiling"
$rc = Invoke-Msys -Command "cd '$msysSrc' && $makeCmd -j$Jobs"
if ($rc -ne 0) {
    Say ""
    Die @"
the build failed. The compiler output above says why.
       If it mentions ../dep/sed.dep, run in an MSYS2 $msysEnv shell:
           cd $msysSrc && $makeCmd depend && $makeCmd
       If it mentions a missing header, re-run this script with -Deps.
"@
}

# ---------------------------------------------------------------------------
# Runtime DLLs
# ---------------------------------------------------------------------------
# [Arcade] Windows has no rpath and no ldconfig: a MinGW binary finds its
# libraries by filename, in the directory it was launched from and then on
# PATH.  Outside an MSYS2 shell neither contains /ucrt64/bin, so the exe dies
# on a bare "SDL2.dll was not found" dialog before main() -- no log, no
# console output, nothing that names the build.
#
# This used to be a printed instruction to "copy SDL2.dll and friends", which
# understates it: the closure is *twelve* DLLs, because SDL2_mixer pulls in a
# codec for every music format (FLAC, mpg123, ogg/vorbis, opus/opusfile,
# wavpack, xmp) and each of those pulls its own. Copying the two named in the
# error message gets a second dialog, then a third. Worse, the list is not
# stable -- it tracks whatever SDL2_mixer was compiled against, so a hardcoded
# list in this script would rot silently into exactly the same dialog.
#
# So derive it: walk the PE import tables with objdump, breadth-first, and keep
# only names that exist under the mingw prefix.  Anything not there is a
# Windows system DLL (KERNEL32, OPENGL32, GLU32, the api-ms-win-crt-* stubs)
# which is already present on the machine and must NOT be shipped.
$binary = Join-Path $BuildRoot 'bin\doomlegacy.exe'
if (Test-Path $binary) {
    Step "Staging runtime DLLs"
    $msysBin = ConvertTo-MsysPath $binary
    # Single-quoted here-string: the shell's own $ and backslashes must survive
    # PowerShell.  Written to a file rather than passed as an argument, because
    # PowerShell strips double quotes out of native-program arguments.
    $walker = @'
PREFIX=/@ENV@/bin
work=$(mktemp); found=$(mktemp)
echo "$1" > "$work"
while [ -s "$work" ]; do
    cur=$(head -n1 "$work")
    tail -n +2 "$work" > "$work.tmp" && mv "$work.tmp" "$work"
    objdump -p "$cur" 2>/dev/null | sed -n 's/^[[:space:]]*DLL Name: //p' | while read -r d; do
        if [ -f "$PREFIX/$d" ] && ! grep -qxF "$d" "$found"; then
            echo "$d" >> "$found"
            echo "$PREFIX/$d" >> "$work"
        fi
    done
done
sort -u "$found"
rm -f "$work" "$found"
'@ -replace '@ENV@', $msysEnv
    $walkerPath = Join-Path $env:TEMP 'dl_dllwalk.sh'
    Set-Content -Path $walkerPath -Value $walker -Encoding ASCII
    $msysWalker = ConvertTo-MsysPath $walkerPath

    # The parentheses matter: without them PowerShell reads -split as an
    # argument to Get-Msys rather than as an operator on its result, and the
    # whole listing stays one string with newlines embedded in it.  That then
    # reaches Copy-Item as a single filename and fails with the thoroughly
    # unhelpful "Illegal characters in path".
    $dlls = @((Get-Msys "sh '$msysWalker' '$msysBin'") -split "`r?`n" |
              ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' })
    Remove-Item $walkerPath -ErrorAction SilentlyContinue

    if ($dlls.Count -eq 0) {
        # Not fatal -- the binary still runs from an MSYS2 shell -- but say so
        # rather than leaving the operator to meet the dialog.
        Warn "could not determine the runtime DLLs (is objdump installed?).
         The exe will only run from an MSYS2 $msysEnv shell until they are
         copied from $msysRoot\$msysEnv\bin beside it."
    } else {
        $srcDll = Join-Path $msysRoot "$msysEnv\bin"
        $dstDll = Join-Path $BuildRoot 'bin'
        foreach ($d in $dlls) {
            Copy-Item -Path (Join-Path $srcDll $d) -Destination $dstDll -Force
        }
        Say ("  copied " + $dlls.Count + " DLLs from $srcDll")
        Say ("  " + ($dlls -join ', '))
    }
}

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
Step "Done"
if (Test-Path $binary) {
    Say "  built: $binary"
    Say ""
    Say "  Do not run it from the build tree -- it looks for its data next to"
    Say "  the binary. Copy everything from svn1749\bin (the DLLs included)"
    Say "  into a run directory alongside legacy.wad and an IWAD."
    Say ""
    Say "  An operator session that can change settings is:  doomlegacy.exe -devmode"
} else {
    Die "the build reported success but $binary is missing."
}
