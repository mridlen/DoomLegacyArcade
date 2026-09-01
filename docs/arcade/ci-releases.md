# Continuous builds and releases

*Part of the DoomLegacy arcade cabinet build. Read before changing
`.github/workflows/build.yml`, or the `--arch` / `-Arch` options in `tools/build.sh` and
`tools/build.ps1`.*

See `building.md` for the build scripts themselves.

---

One workflow, `.github/workflows/build.yml`, does both jobs:

| when | what happens |
| --- | --- |
| a push to `main` that touches the build | Linux and Windows are compiled; the packages are attached to the run as artifacts, kept 90 days |
| a pull request against `main` | the same, so a branch is known to compile before it is merged |
| **Actions → Build → Run workflow** | the same, and — if *Publish a GitHub Release* is ticked and a tag given — a release is published |

There is no automatic release. Every release is a deliberate button press with a tag typed into it.

## Why not the "CMake multi-platform" starter

GitHub offers one, and it is the wrong one for this tree. `CMakeLists.txt` here is secondary,
SDL-only, and is not kept in step with the Makefile's options (`CLAUDE.md` says so at the top). The
GNU Make build is the maintained one, and `tools/build.sh` / `tools/build.ps1` already encode every
trap it has: `make depend` serially before `make -j`, the three edits `make_options` needs on a
modern toolchain, the foreign-`make_options` check, the twelve-DLL closure on Windows. A CMake
workflow would be a *second* build definition to keep correct, and the one that breaks is always the
one nobody runs locally.

So the workflow is a thin driver: install dependencies, run the build script, package, upload. Each
platform is two lines of real work. Anything learned about building this tree belongs in the build
scripts, where a developer's machine benefits from it too — not in the workflow.

Both scripts are invoked twice, which looks redundant and is not: `--install-deps` installs and then
exits with "run the script again", so the second call is the one that builds. If nothing were
missing the first call would build and the second is a no-op `make`.

## The runner's package index is always stale

The first CI run failed before it compiled a line, in `apt install`, on three packages nobody had
asked for:

```
E: Failed to fetch .../uuid-dev_2.39.3-9ubuntu6.5_amd64.deb          404  Not Found
E: Failed to fetch .../libblkid-dev_2.39.3-9ubuntu6.5_amd64.deb      404  Not Found
E: Failed to fetch .../libmount-dev_2.39.3-9ubuntu6.5_amd64.deb      404  Not Found
```

Nothing was wrong with the mirror or with the packages the build actually wants — those three are
transitive dependencies of `libsdl2-dev`. `apt` resolves versions from the index it last downloaded,
and Debian and Ubuntu **delete superseded `.deb` files from the pool** the moment a replacement
lands. A GitHub runner image ships an index frozen at image-build time, so within days it is asking
the mirror for files that no longer exist. The error names the version it wanted (`…6.5`) but never
says that `…6.6` is what is there now, which is what makes it read as a broken mirror.

The fix is `apt-get update` first, and it went into `build.sh` rather than the workflow: `build.sh`
is the one place that knows package management per distribution family, and `build.ps1` already runs
`pacman -Sy` before installing for exactly this reason — the Linux side was simply inconsistent with
its own Windows sibling. It is printed in the "Install with:" hint too, so someone copying that
command by hand off a stale index does not hit the same 404.

Only the Debian family gets it. `dnf` and `zypper` expire their own metadata and need no help, and a
bare `pacman -Sy` is left to the Arch user, for whom refreshing the database without upgrading
invites a partial upgrade.

## `-march=native` must never reach a release

This is the one thing CI can get wrong in a way that no build error reveals.

Both build scripts default to `-march=native`, which is right for a machine building for itself and
wrong for a machine building for somebody else. It bakes in whatever the *builder's* CPU happens to
support. A GitHub runner is a recent Xeon or EPYC with AVX-512; the cabinet PC is not. The binary
compiles, links, packages and uploads without a murmur, and then dies on the cabinet with

```
Illegal instruction (core dumped)
```

— no message, no log line, nothing pointing at the compiler flag that caused it.

`--arch` (`-Arch` on Windows) overrides the detected flag for exactly this case, and the workflow
passes `-march=x86-64 -mtune=generic`: the baseline every 64-bit x86 machine implements, with
`-mtune` still favouring modern CPUs in ways an old one can still execute. Passing it implies
`--reconfigure`, because otherwise an existing `make_options` would be reused and the flag silently
ignored — which is the same failure again, wearing a different hat.

Both build jobs then **grep `make_options` for the baseline flag** before packaging. That check
exists because the failure is invisible otherwise: a wrongly-tuned binary is a perfectly good
binary until it reaches the wrong CPU. Verified locally by building with the CI flags and
disassembling the result — `objdump -d` finds zero AVX or SSE4 instructions, where the default
`-march=native` build on the development machine is full of them.

### The check earned its keep on the first Windows run

That `make_options` grep is not ceremony. The first Windows CI run built `doomlegacy.exe` and staged
all twelve DLLs — the first end-to-end Windows build this tree has ever completed — and then failed
on the arch check, correctly.

`make_options_win` has **every** `ARCH=` line commented out. `make_options_nix` carries one live at
line 59. Both build scripts write the flag by *replacing* the line matching `^ARCH=`, so on Windows
the replacement never fired, no `ARCH` line reached the file, and the Makefile's `ifdef ARCH` left
`CFLAGS` empty and compiled with no `-march` switch at all — while the script printed
`ARCH=-march=native` as though it had set it. Both the detected CPU and an explicit `-Arch` were
being discarded in silence.

The exe was harmless, as it happens: no `-march` is a generic build, which is what a release wants.
It was right by accident, and it would have gone on being right by accident until somebody wanted a
tuned build and got a generic one, or the template changed.

Both scripts now append an `ARCH=` line when the template supplied none, mirroring the `SDL2=1`
safety net that was already there for the same reason. `build.sh` had the identical latent hole —
`make_options_nix` happens to have a live line today, and a template edit would have removed it just
as quietly.

Verified by making `make_options_nix` mimic the Windows template (every `ARCH=` commented out) and
running the real script: the flag lands. Then the net was removed again and the same run produced no
`ARCH` line at all, which is what proves the net is the thing doing the work rather than something
else in the file. With the real template restored the file has exactly one `ARCH=` line — the net
does not double up on a template that already has one.

## What CI can and cannot check

**It proves the tree compiles and links on both platforms.** With no test suite that is most of the
value, and it is the thing that actually breaks.

**It does not prove the game runs.** `tools/smoke.sh` needs a commercial IWAD — `DOOM2.WAD` or
`DOOM.WAD` — which cannot live in this repository, so the smoke checks cannot run on a runner.
Play-testing stays a human job. (A Freedoom IWAD would be redistributable and might satisfy
`smoke.sh`'s detection, since it names games by IWAD filename; nobody has tried, and a smoke run
that half-works would be worse than none.)

The two `Check the binary` steps therefore verify what *can* be verified without a screen: the
executable exists, it is the right kind of file, the arch baseline landed, and on Windows that the
runtime DLLs were actually staged. That last one matters because `build.ps1` derives the DLL closure
by walking import tables; if it silently found nothing, the zip would look complete and fail before
`main()` with a bare *"SDL2.dll was not found"* dialog.

## What is in a package

`make` already stages `legacyhome/` into `svn1749/bin/` (see `cabinet/README.md`), so `bin/` is the
runnable directory. The package is that, plus the wads beside the binary:

```
doomlegacy-arcade-<version>-linux-x86_64/
    doomlegacy               (or doomlegacy.exe + 12 DLLs on Windows)
    legacyhome/config.cfg
    legacy.wad  dogs.wad
    LICENSE  README.md  README_install.txt
    docs-arcade/*.md
```

Everything needed to play except an IWAD, which the operator supplies. The version is the tag on a
release and the first eight characters of the commit hash otherwise, so a package downloaded from an
ordinary run can still be traced to its source.

## Permissions and the release step

The workflow is `contents: read` at the top level and grants `contents: write` **only** to the
release job. A build step cannot push a tag or an asset even if something it compiled tried to.

`gh release create` makes the tag at the built commit. The step **refuses an existing tag** rather
than reusing it: moving a tag leaves every download link already handed out pointing at a release
whose contents have quietly changed underneath it.

Two shell details in there are not stylistic. The prerelease flag is set with an `if`, not
`[ … ] && flags=…`, because under `set -e` a trailing `&&` list that tests false *is* a failing
command and the step would exit 1 every time the box was left unchecked. And the release inputs are
validated in their own tiny first job, so a missing tag costs a few seconds instead of two full
builds.

## Cost

Actions is free for public repositories. A private repository on the Free plan gets 2,000
minutes/month, and the runners are billed at different rates — **Linux 1x, Windows 2x**, macOS 10x.
A full build here is roughly 3–5 minutes on Linux and rather more on Windows, so the Windows job is
the expensive half. If minutes ever get tight, the path is to drop `windows` from the push trigger
and leave it in the manual/release run, not to cut the Linux one.

The `paths:` filter on the push trigger is there for the same reason: a commit that only touches
`docs/` recompiles nothing, so it should not spend anything.

## Adding a tag trigger, if it is ever wanted

Releases are button-driven on purpose — it is what was asked for, and it keeps the tag under human
control. A `push: tags: ['v*']` trigger cannot simply be added to the existing `push:` block,
because GitHub applies the `paths:` filter to tag pushes too: tagging a commit whose changes were
doc-only would silently skip the release. It needs its own workflow, or the `paths:` filter dropped.
