# The build scripts

*Part of the DoomLegacy arcade cabinet build. Read before changing `tools/build.sh`,
`tools/build.ps1` or `build.bat`.*

See `CLAUDE.md` for the manual build, and `README.md` for the operator-facing version.

---

The point is to ship **build scripts rather than executables**: one command that works out what
this machine is, tells the operator what to install if anything is missing, and builds.

| script | covers | tested |
| --- | --- | --- |
| `tools/build.sh` | Linux (Debian/Fedora/Arch/SUSE families), macOS, FreeBSD | **yes**, on Fedora 42 x86_64 |
| `tools/build.ps1` | Windows via MSYS2/MinGW-w64 | **no** — see below |
| `build.bat` | double-click wrapper for the above | no |

OS/2 and DOS are deliberately out of scope. The Makefile still supports them
(`make_options_os2`, `make_options_dos`) and they are built by hand.

## Probe for capabilities, never for package names

Every dependency is checked by **compiling and linking a three-line program**, not by asking the
package manager. Package names drift and distributions substitute, and this is not hypothetical —
on the Fedora 42 machine this was written on, the build is satisfied by:

| what the build needs | what Fedora actually installed |
| --- | --- |
| `SDL2-devel` | **sdl2-compat-devel** |
| `zlib-devel` | **zlib-ng-compat-devel** |
| `mesa-libGL-devel` | **libglvnd-devel** |

On the Debian family `--install-deps` runs `apt-get update` first, and prints it in the hint. That
is not politeness: apt resolves versions from its stored index, Debian and Ubuntu delete superseded
`.deb`s from the pool immediately, and a stale index therefore fails with a 404 on packages nobody
asked for. `ci-releases.md` has the case that found it.

A script that checked for those three package names would have declared a working machine broken.
Asking the compiler *"can you build and link this?"* is the only question that stays true. The
package lists in the script are therefore **hints printed when a probe fails**, not the test.

- **Link, do not merely compile.** A header present with the library absent compiles fine and fails
  at the very end of a long build, which is the worst possible moment to find out.
- Fedora's hint also offers the file-based form (`dnf install /usr/bin/sdl2-config`), which resolves
  whatever currently provides it and so survives the next rename.

## What the script does that a person would forget

These are the same traps documented in `CLAUDE.md`, encoded so nobody has to know them:

- **`make depend` must run serially before any parallel build.** Every `../dep/*.dep` rule pipes
  through the *same* intermediate `../dep/sed.dep` and then moves it, so two parallel dep rules
  clobber each other and the build dies with `mv: cannot stat '../dep/sed.dep'` — an error pointing
  nowhere near the cause. The compile phase parallelises fine.
- **The output directories must exist first**, or make fails on a `.dep` file with "No such file or
  directory". `make dirs` only works from the build root; in `src/` it is an empty placeholder.
- **Three edits to the stock `make_options`**, each a hard build failure rather than a warning:
  `SDL2=1` (the stock file targets SDL 1.2), a working `ARCH=` (the stock `-march=i686` fails on
  x86-64 with "CPU you selected does not support x86-64 instruction set"), and
  `ENV_CFLAGS=-std=gnu17` (GCC 15 defaults to `-std=gnu23`, where `true`/`false` are keywords, which
  breaks this codebase's own `typedef enum {false,true} boolean`). `-g` is added with it so a crash
  on an unattended cabinet gives a backtrace with file and line.

## It never overwrites an existing make_options

`make_options` is machine-local and gitignored, and an operator may have tuned it. If one exists the
script uses it and says so; `--reconfigure` is the only way to have it rewritten. Getting this wrong
would silently discard somebody's settings on what looks like a routine rebuild.

Both scripts write `SDL2=1` and `ARCH=` by **replacing** the template's line, and then append the
setting if no such line was found. The append is not belt-and-braces: `make_options_win` has every
`ARCH=` line commented out, so on Windows the replacement never fired and the flag was silently
dropped for the life of the script — see `ci-releases.md`. Any setting written this way needs the
fallback, because a template that lacks the line fails without a word.

`--arch` (`-Arch` on Windows) is the one option that has to reach into an existing file, since it
changes what `make_options` says — so it implies `--reconfigure`. Without that it would appear to
work and change nothing, which is worse than refusing. It exists for builds that will be *run
somewhere else*, where the default `-march=native` is a shipping hazard rather than an
optimisation; `ci-releases.md` covers why.

## `BUILD=<dir>` needs its own make_options *inside* that directory

`--debug` builds into `svn1749/debug/`, and this cost a debugging session to find: the Makefile sets
`MAKE_OPTIONS = $(BUILD_DIR)make_options`, so with `BUILD=debug` it looks for
`svn1749/debug/make_options` and **not** the one in the build root. Without a copy there the build
stops with

```
Makefile:598: *** "Unknown OS: " .  Stop.
```

which reads as a failed OS detection. It has not failed to detect anything — it never read a
`make_options` at all. The script copies it in, and passes the same `BUILD=` to `make dirs` and
`make depend` so they act on the right tree. The binary then lands in `svn1749/debug/bin/`, not
`bin/debug/`.

## The closing "how to install it" message must not say `cp -a bin/*`

It used to, with a hardcoded `~/games/doom/` destination, and that was wrong twice over.

`~/games/doom/` is the **wad** directory — nine wads, no binary. The cabinet actually runs from
`svn1749/bin/`, whose `legacyhome` holds the live `config.cfg`, `highscores.dat`, `runs.dat`,
`audit.dat` and `demos/`. So a build already updates the install in place and there is nothing to
copy; the advice sent the operator to the wrong directory and contradicted the headless-testing
note in CLAUDE.md, which says in as many words that `svn1749/bin` *is* the live cabinet.

Worse, `cp -a bin/*` copies `bin/legacyhome` too. Onto a populated install that overwrites the
config, the high scores and the record demos. Any install advice must name the binary —
`cp -a bin/doomlegacy <install-dir>/` — and never glob `bin/*`.

The script now tells the two cases apart by looking for live data beside the binary
(`highscores.dat`, `runs.dat` or `demos/`, none of which a fresh build stages — the Makefile only
`cp -n`s a `config.cfg` from the tracked `cabinet/legacyhome`). With live data it says the install
was updated in place; without, it offers to run from `bin/` or to copy the binary out, with the
warning about the glob attached.

## Windows

DoomLegacy is a GNU Make project written for a Unix-like toolchain; **Visual Studio cannot build
this tree as it stands**, so Windows means MinGW-w64 under MSYS2. `build.ps1` finds MSYS2, picks the
subsystem from the CPU (`ucrt64` for x86-64, `mingw32` for 32-bit, `clangarm64` for ARM), checks the
packages, and drives the same make sequence through the MSYS bash.

- If MSYS2 is absent it prints the `winget install --id MSYS2.MSYS2` line rather than failing.
- `MSYSTEM` is set for every invocation. Without it the plain MSYS shell has its own gcc, which
  produces binaries that depend on the MSYS runtime — not a distributable Windows build.
- `build.bat` exists so nobody has to know about PowerShell execution policy; `-ExecutionPolicy
  Bypass` applies to that one invocation and changes nothing on the machine. It keeps the window
  open when double-clicked.
### What the first Windows run found

Detection and MSYS2 discovery worked first time on Windows 11 x86_64 — the CPU, the `ucrt64` choice
and `C:\msys64` were all correct. Two bugs sat immediately behind that, and both are the same class:
**a probe that fails is normal, and the script has to survive it.**

- **`a || b >/dev/null 2>&1` redirects only `b`.** The `||` binds tighter than the redirection, so a
  missing first command still printed `command not found` to the console. Every probe is now
  `{ ... ; } >/dev/null 2>&1`, wrapping the whole chain. Reproduced and fixed against real
  `/bin/sh` before being written back into the PowerShell.
- **`$ErrorActionPreference = 'Stop'` makes any native stderr a *terminating* error.** So that
  leaked line did not merely look untidy — it killed the script at the first missing package, before
  it could print the list of what to install. That list is the entire reason the script exists on a
  machine that has nothing installed yet. Probes now run with the preference restored to `Continue`
  and are judged on their exit code alone.
- **`pkg-config` was the wrong tool to probe with.** A freshly installed MSYS2 does not have it — it
  arrives with the toolchain — so the probe asked a question that cannot be answered on exactly the
  machine needing the most help. The Windows script now checks the toolchain first and, only if a
  compiler exists, probes the libraries by compiling and linking, the same way `build.sh` does.
  Without a compiler it reports one problem rather than six identical ones.
- A fresh MSYS2 having none of the packages is the **normal first-run state**, and the script now
  says so, along with the `pacman -Syu` that a new install needs before it can resolve them.

- **`make` goes by more than one name, and the wrong probe calls an installed package missing.**
  A machine reported `MISS : make` while pacman insisted
  `mingw-w64-ucrt-x86_64-make-4.4.1-4 is up to date`. Both were telling the truth: the *mingw*
  make package installs its binary as **`mingw32-make.exe`**, the MinGW convention that keeps it
  from colliding with a native `make` on PATH. The script now looks for `make`, `mingw32-make` and
  `gmake`, remembers which it found, and drives the whole build with that name.
  - The one to *want* is the plain MSYS package `make` — no `mingw-w64-` prefix, because it is a
    shell tool like sed and awk rather than part of the toolchain. This tree is a Unix Makefile
    whose recipes shell out to `sh`, `sed`, `awk`, `mv` and `cp`, and MSYS make understands the
    paths those produce. `mingw32-make` is a *native* Windows make and is not MSYS-aware, so it is
    accepted with a warning rather than trusted silently.
- **Finding "a gcc" is not enough — it must be the gcc for the chosen subsystem.** MSYS2 ships its
  own compiler at `/usr/bin/gcc`, targeting the MSYS runtime. If the mingw toolchain is not
  installed, *that* one is found instead, and it searches `/usr/include` and `/usr/lib` — it cannot
  see `/ucrt64` at all. The symptom is a maddening half-success, reported from a real machine:

  ```
    ok   : C compiler (gcc)
    MISS : SDL2
    MISS : SDL2_mixer
    MISS : libzip
    ok   : zlib
    ok   : OpenGL (GL and GLU)
  ```

  with pacman insisting all three were up to date. The split is the clue: **zlib and the OpenGL
  import libraries exist in the MSYS tree too**, so they linked; SDL2, SDL2_mixer and libzip live
  only under `/ucrt64`, so they did not. Nothing was missing except the right compiler.
  - `gcc -dumpmachine` separates them: `x86_64-w64-mingw32` is the toolchain, `x86_64-pc-msys` is
    MSYS2's own. Matching the substring `mingw` covers all three subsystems this script targets
    (`x86_64-`, `i686-` and `aarch64-w64-mingw32`) and excludes msys.
  - The triple is **printed either way**, so if that convention is ever wrong the output says so
    rather than misdiagnosing silently — it was inferred from the MSYS2 packaging convention and
    could not be confirmed on the Linux machine this was written on.
- **When gcc is absent the library list is not evidence.** The libraries are probed by compiling, so
  with no compiler they are all reported missing for one shared reason. On a machine that already
  had SDL2, SDL2_mixer and libzip installed this read as five faults instead of one, so the script
  now says explicitly that they are listed because they *cannot be checked yet*, not because they
  are known to be absent.

### The first Windows run to completion

The run above stopped at the dependency check. Getting past it exposed five more faults, in the
order the build hits them. Each looked like a missing package or a broken tool and was neither.

- **PowerShell eats double quotes on their way to a native program, so a probe must not contain
  one.** `Invoke-Msys` passes the probe to `bash.exe` as an argument; PowerShell rebuilds that
  argument string and drops unescaped `"`. `zip_open("x",0,0)` therefore reached gcc as
  `zip_open(x,0,0)`, failed on an undeclared `x`, and reported **libzip missing on a machine where
  pacman said it was installed** — with nothing in the compiler output pointing at PowerShell. The
  fix is to keep every probe source quote-free (`zip_open(0,0,0)` is as good a link test).
  Verifying this needs the *real* path: `printf ... | cat` through `& $bash -lc` shows the quotes
  gone. Note the earlier round's claim that the probes had been validated "including the embedded
  quotes" — they were, under `/bin/sh` directly, which is exactly the layer that does not have this
  bug.
- **`int main(void)` is wrong for an SDL probe on Windows.** Here `sdl2-config --cflags` emits
  `-Dmain=SDL_main`, which renames the probe's `main` and then collides with SDL_main.h's own
  declaration:
  ```
  error: conflicting types for 'SDL_main'; have 'int(void)'
  ```
  `SDL_MAIN_HANDLED` does not prevent it — the declaration is there either way. Declaring the probe
  `int main(int argc, char **argv)` matches. This is why `build.sh` can use `main(void)` and
  `build.ps1` cannot: Linux's `sdl2-config` emits no such define. Two of the three "missing"
  packages were this.
- **A `make_options` from another platform gets reused, and the failure names none of it.** The tree
  lives in a synced folder (Dropbox) shared with the Linux cabinet. `make_options` is *gitignored*,
  which stops git from carrying it between machines and does nothing whatever about the sync — so
  the Linux file, `OS=LINUX`, was sitting here and the script's "never overwrite an existing
  make_options" rule dutifully honoured it. Every one of the ~120 source files then compiles (the
  options are wrong only in what they link against) and the build dies at the very end with
  ```
  ld.exe: cannot find -lGL / -lGLU / -ldl
  ```
  which reads as three more missing packages. They are not installed on Windows and never will be:
  the OpenGL import libraries here are `-lopengl32 -lglu32`, and there is no libdl at all. The tell
  is `-DLINUX` in the compile lines. Both scripts now compare the existing `make_options`'s `OS=`
  line against their template's, regenerate when they differ, and keep the old file as
  `make_options.foreign`.
  - **And the synced `objs/` are just as stale.** make compares timestamps, not target
    architecture, so it links the Linux objects happily: `relocation truncated to fit`, then
    undefined references to `__isoc23_sscanf` and `__ctype_b_loc` — *glibc* symbols — with source
    paths under `/home/mridlen`. A platform switch invalidates every object in the tree, so
    detecting a foreign `make_options` now forces `--clean` as well. Fixing only the options leaves
    a second, less legible failure behind it.
- **Under MSYS the Makefile's Windows dep rule writes its dep files into the wrong place, silently.**
  The `%.dep` rule used DOS paths (`> ..\dep\main1.dep`), but MSYS recipes run in `sh`, where a
  backslash is an escape and not a separator. The redirect created a file in `src/` literally named
  `..depmain1.dep`; `fixdep` was then handed `..\dep\main1.dep` and answered
  ```
  Dep file does not exist: ..\dep\main1.dep
  ```
  followed by a usage message — which reads as a broken or mis-called `fixdep`. It is neither: the
  dep file was written, just somewhere nobody looked. `ls src/..dep*` is the check that settles it.
  `svn1749/src/Makefile` now has an `ifdef HAVE_MSYS` branch on `%.dep` using POSIX paths
  throughout. (The pre-existing `CC_SELECT=MINGW` branch is half-converted the same way — it
  redirects to `$(DD)/` but still passes `$(DD_WIN)\` to fixdep — so it has the same fault.)
- **Windows ships an OpenGL 1.1 `<GL/gl.h>` and nothing newer.** The header is Microsoft's, frozen
  in 1996, however modern the driver underneath is. The arcade texture-clamp fix uses
  `GL_CLAMP_TO_EDGE`, core since OpenGL **1.2**, so it exists in every implementation this will run
  on but is absent from the declarations mingw compiles against:
  `error: 'GL_CLAMP_TO_EDGE' undeclared`, on a line that compiles without comment on Linux.
  `r_opengl.c` now defines the enum value (`0x812F`) under `#ifndef`. It is a constant handed to the
  driver, not a function to resolve, so there is nothing to load at runtime. Expect this class of
  break for **any** GL constant newer than 1.1 added on the Linux side.

### The runtime DLLs are staged, not documented

The linked exe would not start: **`SDL2_mixer.dll was not found`**, then `SDL2.dll was not found`.
Windows has no rpath and no `ldconfig` — a MinGW binary finds its libraries by bare filename, in the
launch directory and then on `PATH`, and outside an MSYS2 shell neither contains `/ucrt64/bin`. The
dialog appears before `main()`, so there is no log and nothing that names the build.

The script used to *print* an instruction to copy "SDL2.dll and friends". That understates it by an
order of magnitude and is why the instruction failed in practice:

- **The closure is twelve DLLs, not two.** SDL2_mixer links a codec for every music format it
  supports, and those pull their own: `SDL2`, `SDL2_mixer`, `libFLAC`, `libmpg123-0`, `libogg-0`,
  `libopus-0`, `libopusfile-0`, `libvorbis-0`, `libvorbisfile-3`, `libwavpack-1`, `libwinpthread-1`,
  `libxmp`. Copying the one the dialog names gets you the next dialog.
- **A hardcoded list would rot into that same dialog.** The set tracks whatever SDL2_mixer was
  compiled against, which is a packaging decision upstream of this tree. So `build.ps1` derives it:
  breadth-first over the PE import tables with `objdump -p`, keeping only names that exist under the
  mingw prefix. Anything *not* there is a Windows system DLL — `KERNEL32`, `OPENGL32`, `GLU32`, the
  `api-ms-win-crt-*` stubs — already on the machine and not ours to ship.
- Two PowerShell traps showed up again on the way, both worth recognising by their error text:
  - The walker is written to a **file** and run as `sh script.sh`, not passed as an argument, because
    PowerShell strips double quotes out of native-program arguments — the same fault as the libzip
    probe above.
  - `Get-Msys "..." -split "\`n"` **without parentheses** parses `-split` as an argument to
    `Get-Msys` rather than an operator on its result. The listing stays a single string with newlines
    inside it, reaches `Copy-Item` as one filename, and fails with `Illegal characters in path`.
    Write `(Get-Msys "...") -split ...`.

The check that settles whether the set is complete is `LoadLibraryW` on the exe **with MSYS2 removed
from `PATH`** — it resolves the entire import graph without running the program, so it cannot be
fooled by a DLL that happens to be on PATH, and it returns an error instead of a modal dialog.

## Verified

On Fedora 42 x86_64, from a checkout with no `make_options`:

- `--deps` reported all seven dependencies present.
- A full build from scratch took **39 seconds** and produced a binary that passes `make smoke` 5/5.
- Re-running left an existing `make_options` untouched (checked by appending a marker line and
  confirming it survived).
- With a deliberately broken compiler (`CC=false`), every library probe reported `MISS`, the correct
  `dnf` command was printed, and the script exited 1 **without** starting a build.
- `--debug` produced `svn1749/debug/bin/doomlegacy`.

On Windows 11 Pro x86_64, MSYS2 `ucrt64`, gcc 15.2.0, from a tree whose `make_options` and `objs/`
had been synced in from the Linux cabinet:

- All seven dependencies report `ok` (they did not before the probe fixes, while pacman reported
  every package up to date).
- `build.bat` from that Linux-contaminated state runs the whole recovery unattended — detects
  `OS=LINUX`, keeps it as `make_options.foreign`, regenerates for Windows, forces the clean, and
  links `svn1749\bin\doomlegacy.exe` (11.5 MB).
- The recovery path was re-tested by restoring the Linux `make_options` and running again, so it is
  confirmed from the actual failing state rather than inferred.
- The 12 runtime DLLs are staged into `svn1749\bin` automatically, and a copy of that directory
  passes `LoadLibraryW` with **MSYS2 scrubbed off `PATH`** — every import in the graph resolves from
  the staged files alone.
- **It starts.** Reported by the operator from a run directory holding `svn1749\bin` plus
  `legacy.wad` and an IWAD. Nothing beyond "it loaded" has been checked — no rendering, input,
  sound, scoring or attract-cycle behaviour on Windows has been observed yet.

On a GitHub Actions `windows-latest` runner, from a clean checkout — which is the first time the
script has run on a machine nobody prepared for it:

- `-InstallDeps` brought up the whole ucrt64 toolchain from the runner's stock MSYS2 unattended, and
  the build then linked `doomlegacy.exe` (11.5 MB) and staged all 12 DLLs. Every earlier Windows run
  was on a machine that already had MSYS2 set up by hand.
- That run is also what exposed the `ARCH=` hole above: it had been silently dropping the flag on
  Windows since the script was written, and only a check that read `make_options` back could see it.

### Launching it from `svn1749\bin` crashes silently — it is missing its data, not broken

Double-clicking `doomlegacy.exe` in the build tree "does nothing at all": no window, no dialog, no
log file. It is not doing nothing — it exits with **`0xC0000374`, `STATUS_HEAP_CORRUPTION`**, and
because the exe is linked `-mwindows` there is no console for the message to reach.

Redirecting output is what makes it visible (`./doomlegacy.exe > out.txt 2>&1` from an MSYS2 shell —
stdout redirection still works with no console attached). The log stops here:

```
Initializing SDL...
 0 joystick(s) found.
StartupGraphics...
VID_SetMode(window,0)
```

The cause is the ordinary one — `svn1749\bin` has **no `legacy.wad` and no IWAD**, and this is the
documented "do not run it from the build tree" case. What is *not* ordinary is the failure mode: it
should be a legible "IWAD not found", not heap corruption inside video startup. Whether that is
Windows-specific or a latent bug the Linux side has been getting away with is **unknown and
uninvestigated**. Do not read a heap-corruption exit here as a fault in the build.
- `HAVE_LIBZIP` and `HAVE_ZLIB` are **off** in this build: `make_options_win` does not set them, so
  the Windows binary has no PK3 support. The libraries are probed for and present; only the option
  is missing. Nothing has been decided about that yet.
