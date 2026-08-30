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

**The Windows build has still not run to completion** — the first run that gets past dependency
installation is the real test. The probe strings themselves were validated under `/bin/sh` (silent
and correctly non-zero when a library is absent, silent and zero when present, including the
`sdl2-config` command substitution and the embedded quotes).

## Verified

On Fedora 42 x86_64, from a checkout with no `make_options`:

- `--deps` reported all seven dependencies present.
- A full build from scratch took **39 seconds** and produced a binary that passes `make smoke` 5/5.
- Re-running left an existing `make_options` untouched (checked by appending a marker line and
  confirming it survived).
- With a deliberately broken compiler (`CC=false`), every library probe reported `MISS`, the correct
  `dnf` command was printed, and the script exited 1 **without** starting a build.
- `--debug` produced `svn1749/debug/bin/doomlegacy`.
