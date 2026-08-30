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
    UNTESTED ON WINDOWS.  Written from the Makefile and the Unix build, but
    the author had no Windows machine to run it on -- treat the first run as
    the test.  tools/build.sh is the Linux/macOS equivalent and *is* tested.
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
function Invoke-Msys {
    param([string]$Command, [switch]$Quiet)
    $env:MSYSTEM = $msysEnv.ToUpper()
    $env:CHERE_INVOKING = '1'
    if ($Quiet) {
        & $bash -lc $Command 2>&1 | Out-Null
        return $LASTEXITCODE
    }
    & $bash -lc $Command
    return $LASTEXITCODE
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

$packages = @(
    @{ Name='C compiler'; Probe='command -v gcc';        Pkg="$pkgPrefix-gcc" },
    @{ Name='make';       Probe='command -v make';       Pkg="$pkgPrefix-make" },
    @{ Name='SDL2';       Probe='command -v sdl2-config || pkg-config --exists SDL2'; Pkg="$pkgPrefix-SDL2" },
    @{ Name='SDL2_mixer'; Probe='pkg-config --exists SDL2_mixer || test -f /$env/include/SDL2/SDL_mixer.h'; Pkg="$pkgPrefix-SDL2_mixer" },
    @{ Name='libzip';     Probe='pkg-config --exists libzip'; Pkg="$pkgPrefix-libzip" },
    @{ Name='zlib';       Probe='pkg-config --exists zlib';   Pkg="$pkgPrefix-zlib" }
)

$missingPkgs = @()
foreach ($p in $packages) {
    $probe = $p.Probe -replace '/\$env/', "/$msysEnv/"
    $rc = Invoke-Msys -Command "$probe >/dev/null 2>&1" -Quiet
    if ($rc -eq 0) { Say ("  ok   : " + $p.Name) }
    else           { Say ("  MISS : " + $p.Name); $missingPkgs += $p.Pkg }
}

# OpenGL comes with the MinGW toolchain itself (opengl32/glu32 are Windows
# system libraries), so it is checked by linking rather than by package.
$glProbe = 'printf "#include <GL/gl.h>\n#include <GL/glu.h>\nint main(void){glFlush();gluErrorString(0);return 0;}\n" > /tmp/dlgl.c && gcc /tmp/dlgl.c -o /tmp/dlgl.exe -lopengl32 -lglu32'
$rc = Invoke-Msys -Command "$glProbe >/dev/null 2>&1" -Quiet
if ($rc -eq 0) { Say "  ok   : OpenGL (GL and GLU)" }
else { Say "  MISS : OpenGL (GL and GLU)"; $missingPkgs += "$pkgPrefix-gcc" }

if ($missingPkgs.Count -gt 0) {
    $list = ($missingPkgs | Select-Object -Unique) -join ' '
    Say ""
    Say "Missing packages: $list"
    Say ""
    Say "Install them with:"
    Say "    pacman -S --needed $list"
    Say ""
    if ($InstallDeps) {
        Step "Installing"
        $rc = Invoke-Msys -Command "pacman -S --needed --noconfirm $list"
        if ($rc -ne 0) { Die "pacman failed (exit $rc)" }
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
Invoke-Msys -Command "cd '$msysBuildRoot' && make dirs" -Quiet | Out-Null

if ($Clean) {
    Say "  cleaning"
    Invoke-Msys -Command "cd '$msysSrc' && make clean" -Quiet | Out-Null
}

# `make depend` must run serially before any parallel build: every ../dep/*.dep
# rule pipes through the same intermediate ../dep/sed.dep and then moves it, so
# parallel dep rules clobber each other and the build dies with
# "mv: cannot stat '../dep/sed.dep'" -- an error that points nowhere near the
# cause.  The compile phase parallelises fine.
Say "  resolving dependencies (serial -- this phase cannot be parallelised)"
Invoke-Msys -Command "cd '$msysSrc' && make depend" -Quiet | Out-Null

Say "  compiling"
$rc = Invoke-Msys -Command "cd '$msysSrc' && make -j$Jobs"
if ($rc -ne 0) {
    Say ""
    Die @"
the build failed. The compiler output above says why.
       If it mentions ../dep/sed.dep, run in an MSYS2 $msysEnv shell:
           cd $msysSrc && make depend && make
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
