# Editing the ENDOOM screen

Applies when changing the 80x25 colour text screen printed on a clean exit — the
"Thanks for playing Doom Legacy" banner. Read this before touching the `ENDOOM`
lump in `common/legacy.wad`, or `svn1749/src/sdl/endtxt.c`.

## Where it lives in the code

- `d_main.c:4288` — `D_Quit_Save()` caches the lump (`W_CheckNumForName("ENDOOM")`)
  at shutdown step `quitseq < 10`, **before** `W_Shutdown()` closes the wads.
  Skipped unless the quit is `QUIT_normal`, and suppressed by `-noendtext` /
  `-noendtxt` or `cv_textout` being 0.
- `d_main.c:4334` — step `quitseq < 29`, after graphics and system teardown,
  calls `I_Show_EndText( endtext )`.
- `svn1749/src/sdl/endtxt.c:168` — `I_Show_EndText()`, the SDL build's copy.
  Converts CP437 to UTF-8 via its `cp437_to_utf[]` table and the DOS colour
  attributes to ANSI SGR via `fg_att_str[]` / `bg_att_str[]`, then prints.
  Parallel copies: `linux_x/endtxt.c:168`, `macos/endtxt.c:19`,
  `win32/win_sys.c:2349`, `djgppdos/I_system.c:341`.

`M_Restart_Program` (`m_menu.c`) deliberately suppresses ENDOOM — the cabinet is
re-execing, not returning to a terminal.

## The lump format

4000 bytes: 2000 little-endian `uint16` cells, row-major, 80 columns by 25 rows.

| bits | meaning |
| --- | --- |
| 0-7 | CP437 character code |
| 8-11 | foreground colour, 0-15 |
| 12-15 | background colour, 0-15 |

Colour order is the DOS one: 0 black, 1 blue, 2 green, 3 cyan, 4 red, 5 magenta,
6 brown, 7 light grey, then 8-15 the bright versions of the same.

**The top attribute bit is a bright background, not blink.** `endtxt.c` leaves
`MSB_BLINK` undefined, so it masks the background as a full four bits
(`bg = (att >> 4) & 0x0F`). Anything that treats bit 15 as blink will get the
backgrounds wrong on the top half of the palette.

Three character codes — `0x00`, `0x20` and `0xFF` — are all a blank space in the
engine's own table.

## Why SLADE will not edit it

SLADE recognises `ENDOOM` and hands it to its **ANSI viewer, which is read-only**.
There is no text-edit or hex-edit path to it in SLADE. Export and re-import is
the only route through SLADE, and that still needs something else to do the edit.

## Editing it

`tools/endoom.py` does the whole round trip. Two workflows:

### Text form — no GUI needed

```
tools/endoom.py dump common/legacy.wad -o endoom.txt   # edit in any editor
tools/endoom.py replace common/legacy.wad endoom.txt   # patch it back
tools/endoom.py show common/legacy.wad                 # preview, in colour
```

The text form is three lines per row: `T` the text, `F` and `B` one hex colour
digit per character aligned underneath. Short lines are padded on the right, so
an editor stripping trailing whitespace is harmless. Errors are reported with a
file, line and column.

### Graphical — Moebius, PabloDraw, icy_draw

```
tools/endoom.py xbin common/legacy.wad -o ENDOOM.xb    # edit in the ANSI editor
tools/endoom.py replace common/legacy.wad ENDOOM.xb    # patch it back
```

**Use `xbin`, not `extract`.** The lump is byte-for-byte the ANSI scene's
"Binary Text" (`.bin`) format, so renaming it `.bin` is *technically* correct —
but `.bin` has **no header**, so the editor cannot know the file is 80 columns
wide. Editors respond by guessing 160, refusing the file, or opening it as
garbage, and none of them says why. This is the single most likely reason an
ENDOOM will not load.

XBIN (`.xb`) is the same cell data behind an 11-byte header that states width,
height and font size, so editors open it without being asked. `endoom.py` writes
it with the NonBlink flag set, which is what makes the editor treat the top
attribute bit as a bright background — matching `endtxt.c`.

Reading `.xb` back handles what editors actually save: RLE compression, an
embedded palette, an embedded font, and a canvas shorter than 25 rows (padded).
A canvas that is not 80 columns is refused with a message saying so.

## Gotchas

- **ANSI editors append a SAUCE record, and some append a second without
  removing the first.** SAUCE is 128 bytes of metadata (title, author, canvas
  size) at the end of the file, after an EOF `0x1A` byte and an optional COMNT
  block. It made the first real edited file 8413 bytes where the header
  arithmetic said 8155. `strip_sauce()` removes any number of them, from `.xb`
  and from raw `.bin` alike. Do not "simplify" it away on the grounds that
  truncating to 4000 bytes already skips it — that only works because the
  metadata trails the cell data, and it would stop working the moment anything
  needed the real file length.
- **A custom palette or font in the `.xb` is discarded, and that is correct.**
  ENDOOM is printed to a terminal as ANSI colour escapes; there is nowhere for a
  palette or a font to go. Draw with the standard 16 DOS colours and expect the
  terminal's own font. Characters outside plain ASCII will depend on the
  terminal's encoding — `endtxt.c` converts to UTF-8 only when `cv_textout` is 2.
- **`replace` writes in place and the size never changes.** The lump is a fixed
  4000 bytes, so no directory entry moves and no other lump is touched. It backs
  up to `<wad>.bak` unless `--no-backup`. A rebuilt lump that is not exactly 4000
  bytes is refused rather than written.
- **`0x00` and `0xFF` normalise to `0x20` through the text form**, because all
  three are the same blank glyph and the text form cannot tell them apart. The
  screen renders identically — and in the non-UTF8 path, where `endtxt.c`
  `putchar()`s the raw byte, a NUL or stray `0xFF` is worse than a space. `dump`
  warns when the source contains them. Use `extract`/`xbin` if the exact bytes
  matter.
- **The CP437 table in `endoom.py` is the engine's own**, transcribed from
  `cp437_to_utf[]` in `endtxt.c` and verified equal at all 256 entries. It
  differs from Python's `cp437` codec at `0x7C` (broken bar, not `|`), `0x7F` and
  `0xFF`, so do not "fix" it to match the codec. `|` is accepted on input as an
  alias for `0x7C`, which is what that byte actually draws.
- **`legacy.wad` is tracked in git and the user edits their live copy**, so follow
  the CLAUDE.md rule: verify only the expected lumps differ before committing.

## How this was verified

- CP437 table compared against `endtxt.c` — 0 mismatches in 256 entries.
- `dump` → `build` on the real lump: byte identical.
- Stress lump exercising all 256 character codes against all 256 attributes:
  byte identical apart from the 9 documented `0x00`/`0xFF` blank cells.
- XBIN: plain, RLE-compressed, palette+font, and a 3-row canvas all read back
  byte identical; a 160-column canvas is refused.
- **End to end against the real engine.** A copy of `legacy.wad` was patched with
  a marker row painted bright yellow on red, then run headless
  (`SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`, `SDL_NO_SIGNAL_HANDLERS=1`)
  with `printf 'wait 35\nquit\n' > legacyhome/autoexec.cfg` to force a clean quit.
  The engine printed the marker text, and 80 cells of `\033[1;33m\033[41m` —
  the patched row, in the right colours.

  Note that the marker text is **not contiguous in the raw output**: `endtxt.c`
  emits colour escapes before every single character. Strip them first
  (`sed 's/\x1b\[[0-9;]*m//g'`) or a grep for your own edit finds nothing and
  looks like the patch failed. This is the same trap as the `READ THE DOCS`
  check in `tools/smoke.sh`.
