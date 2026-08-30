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
    [int]$Jobs = 0
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

Say "  system    : $osCaption"
Say "  cpu       : $archRaw ($archDesc)"
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

# -- phase 1: the toolchain.  Nothing else can be probed without it. --
$haveGcc  = Test-Msys 'command -v gcc'
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
if ($haveGcc)  { Say "  ok   : C compiler (gcc)" } else { Say "  MISS : C compiler"; $missingPkgs += "$pkgPrefix-gcc" }
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
    # SDL_MAIN_HANDLED keeps SDL from redefining main() in the probe.
    $probes = @(
        @{ Name='SDL2';       Pkg="$pkgPrefix-SDL2"
           Src='#define SDL_MAIN_HANDLED\n#include <SDL.h>\nint main(void){SDL_Init(0);return 0;}\n'
           Flags='$(sdl2-config --cflags --libs 2>/dev/null || echo -lSDL2)' },
        @{ Name='SDL2_mixer'; Pkg="$pkgPrefix-SDL2_mixer"
           Src='#define SDL_MAIN_HANDLED\n#include <SDL.h>\n#include <SDL_mixer.h>\nint main(void){Mix_Init(0);return 0;}\n'
           Flags='$(sdl2-config --cflags --libs 2>/dev/null || echo -lSDL2) -lSDL2_mixer' },
        @{ Name='libzip';     Pkg="$pkgPrefix-libzip"
           Src='#include <zip.h>\nint main(void){zip_open("x",0,0);return 0;}\n'
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

if ((Test-Path $opts) -and -not $Reconfigure) {
    # make_options is machine-local and gitignored, and may have been tuned.
    Say "  using the existing make_options"
    Say "  (delete it or pass -Reconfigure to regenerate)"
} else {
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
# Done
# ---------------------------------------------------------------------------
$binary = Join-Path $BuildRoot 'bin\doomlegacy.exe'
Step "Done"
if (Test-Path $binary) {
    Say "  built: $binary"
    Say ""
    Say "  Do not run it from the build tree -- it looks for its data next to"
    Say "  the binary. Copy everything from svn1749\bin into a run directory"
    Say "  alongside legacy.wad and an IWAD (DOOM.WAD, DOOM2.WAD ...)."
    Say ""
    Say "  It also needs the MinGW runtime DLLs (SDL2.dll and friends). Copy"
    Say "  them from $msysRoot\$msysEnv\bin, or run from an MSYS2 $msysEnv"
    Say "  shell where they are already on PATH."
    Say ""
    Say "  An operator session that can change settings is:  doomlegacy.exe -devmode"
} else {
    Die "the build reported success but $binary is missing."
}
