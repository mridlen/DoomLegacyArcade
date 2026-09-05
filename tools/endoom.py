#!/usr/bin/env python3
# [Arcade] Round-trip editor for the ENDOOM lump -- the 80x25 colour text
# screen printed on a clean exit (see I_Show_EndText, svn1749/src/sdl/endtxt.c).
#
# The lump is 4000 bytes: 2000 little-endian uint16 cells, low byte the CP437
# character code, high byte the colour attribute (low nibble foreground, high
# nibble background).  That is byte-for-byte the ANSI art scene's "Binary Text"
# (.bin) format at 80 columns -- but .bin carries no header, so editors cannot
# tell how wide it is and many will not open it.  Use `xbin` for those: same
# cell data, 11-byte header, opens straight in Moebius, PabloDraw or icy_draw.
#
# Usage:
#   endoom.py show    <wad|lmp|xb>           preview in the terminal, in colour
#   endoom.py dump    <wad|lmp|xb> [-o f.txt]  write the editable text form
#   endoom.py build   <f.txt|f.xb> [-o f.lmp]  -> 4000-byte lump
#   endoom.py xbin    <wad|lmp|txt> [-o f.xb]  -> .xb for a graphical editor
#   endoom.py extract <wad>       [-o f.lmp]   raw headerless lump out
#   endoom.py replace <wad> <f.txt|f.lmp|f.xb> patch the lump into the wad
#
# The lump is a fixed 4000 bytes, so `replace` rewrites it without moving any
# directory entry -- no other lump in the wad is touched.

import argparse, os, shutil, struct, sys

# CP437 -> Unicode, matching the engine's own cp437_to_utf[] table in
# svn1749/src/sdl/endtxt.c so the two cannot disagree.  Note it differs from
# Python's 'cp437' codec at 0x7C (broken bar, not '|'), 0x7F and 0xFF.
CP437 = (
    " ☺☻♥♦♣♠•◘○◙♂♀♪♫☼"
    "►◄↕‼\xb6\xa7▬↨↑↓→←∟↔▲▼"
    " !\"#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
    "`abcdefghijklmnopqrstuvwxyz{\xa6}~⌂"
    "\xc7\xfc\xe9\xe2\xe4\xe0\xe5\xe7\xea\xeb\xe8\xef\xee\xec\xc4\xc5"
    "\xc9\xe6\xc6\xf4\xf6\xf2\xfb\xf9\xff\xd6\xdc\xa2\xa3\xa5₧ƒ"
    "\xe1\xed\xf3\xfa\xf1\xd1\xaa\xba\xbf⌐\xac\xbd\xbc\xa1\xab\xbb"
    "░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
    "└┴┬├─┼╞╟╚╔╩╦╠═╬╧"
    "╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
    "α\xdfΓπΣσ\xb5τΦΘΩδ∞φε∩"
    "≡\xb1≥≤⌠⌡\xf7≈\xb0∙\xb7√ⁿ\xb2■ "
)
assert len(CP437) == 256, "CP437 table is %d entries" % len(CP437)

# char -> byte.  Printable ASCII first so ' ' resolves to 0x20 rather than 0x00
# or 0xFF, which are the same glyph.  '|' is not a CP437 glyph at all; accept it
# as an alias for the broken bar, which is what byte 0x7C actually draws.
ENC = {}
for _i in range(0x20, 0x7F):
    ENC.setdefault(CP437[_i], _i)
for _i in range(256):
    ENC.setdefault(CP437[_i], _i)
ENC['|'] = 0x7C

# attribute nibble -> ANSI SGR colour, matching fg_att_str/bg_att_str in endtxt.c
ANSI_FG = (30, 34, 32, 36, 31, 35, 33, 37, 90, 94, 92, 96, 91, 95, 93, 97)
ANSI_BG = (40, 44, 42, 46, 41, 45, 43, 47, 100, 104, 102, 106, 101, 105, 103, 107)

COLS, ROWS = 80, 25
NCELL = COLS * ROWS
LUMPSZ = NCELL * 2


# ---------------------------------------------------------------- XBIN -----
# A raw .bin has no header, so an ANSI editor has to be told it is 80 columns
# wide -- most guess 160, or refuse the file outright.  XBIN is the same cell
# data behind an 11-byte header that states the width, height and font size, so
# editors open it without being asked.  Header:
#   char ID[4] = "XBIN"; byte EOF = 0x1A; uint16 width; uint16 height;
#   byte fontsize; byte flags
XBIN_MAGIC = b'XBIN\x1a'
XB_PALETTE, XB_FONT, XB_COMPRESS, XB_NONBLINK, XB_512CHARS = 1, 2, 4, 8, 16


def xbin_wrap(lump):
    """4000-byte cell data -> a .xb file an ANSI editor will open."""
    # NonBlink: the top attribute bit is a bright background here, not blink,
    # which is how endtxt.c reads it (its MSB_BLINK is left undefined).
    return XBIN_MAGIC + struct.pack('<HHBB', COLS, ROWS, 16, XB_NONBLINK) + lump


def xbin_unrle(data, ncell):
    """Decompress XBIN RLE.  Each block: top 2 bits type, low 6 bits count-1."""
    out = bytearray()
    i = 0
    while len(out) < ncell * 2 and i < len(data):
        b = data[i]; i += 1
        kind, count = b >> 6, (b & 0x3F) + 1
        if kind == 0:                      # no compression: count char/attr pairs
            out += data[i:i + count * 2]; i += count * 2
        elif kind == 1:                    # same character, count attributes
            ch = data[i]; i += 1
            for _ in range(count):
                out += bytes((ch, data[i])); i += 1
        elif kind == 2:                    # same attribute, count characters
            at = data[i]; i += 1
            for _ in range(count):
                out += bytes((data[i], at)); i += 1
        else:                              # one char/attr pair, repeated
            pair = data[i:i + 2]; i += 2
            out += pair * count
    return bytes(out)


def xbin_read(data, path):
    """Strip an XBIN header (and any palette/font/RLE) -> 4000 bytes of cells."""
    if len(data) < 11:
        sys.exit("%s: truncated XBIN header (%d bytes)" % (path, len(data)))
    w, h, fontsize, flags = struct.unpack('<HHBB', data[5:11])
    if w != COLS:
        sys.exit("%s: XBIN is %d columns wide, ENDOOM must be %d.\n"
                 "  Set the canvas width to %d in your editor and save again."
                 % (path, w, COLS, COLS))
    if h > ROWS:
        sys.exit("%s: XBIN is %d rows tall, ENDOOM must be at most %d.\n"
                 "  Set the canvas height to %d in your editor and save again."
                 % (path, h, ROWS, ROWS))
    p = 11
    if flags & XB_PALETTE:
        p += 48
    if flags & XB_FONT:
        p += fontsize * (512 if flags & XB_512CHARS else 256)
    body = data[p:]
    if flags & XB_COMPRESS:
        body = xbin_unrle(body, w * h)
    body = body[:w * h * 2]
    if len(body) < w * h * 2:
        sys.exit("%s: XBIN data is short (%d bytes for %dx%d)" % (path, len(body), w, h))
    # a shorter canvas is padded out to 25 rows with blank cells
    return body + struct.pack('<H', 0x0020) * ((ROWS - h) * COLS)


def read_file(path, mode='rb'):
    """open().read(), but a missing or unreadable file is a message, not a trace."""
    try:
        if mode == 'rb':
            return open(path, 'rb').read()
        return open(path, encoding='utf-8').read()
    except FileNotFoundError:
        sys.exit("%s: no such file.\n"
                 "  Paths are relative to where you are now (%s)."
                 % (path, os.getcwd()))
    except IsADirectoryError:
        sys.exit("%s: that is a directory, not a file" % path)
    except PermissionError:
        sys.exit("%s: permission denied" % path)
    except UnicodeDecodeError:
        sys.exit("%s: not UTF-8 text.  If this is a lump or an XBIN, name it "
                 ".lmp/.bin/.xb so it is read as binary." % path)


def write_file(path, data):
    """Likewise for output, so a bad -o does not traceback."""
    try:
        with open(path, 'wb' if isinstance(data, bytes) else 'w',
                  **({} if isinstance(data, bytes) else {'encoding': 'utf-8'})) as f:
            f.write(data)
    except (IsADirectoryError, FileNotFoundError):
        sys.exit("%s: cannot write there -- check the directory exists" % path)
    except PermissionError:
        sys.exit("%s: permission denied" % path)


def read_lump(path):
    """Return the 4000-byte ENDOOM from a wad, an .xb, or a raw lump file."""
    data = read_file(path)
    if data[:5] == XBIN_MAGIC:
        return xbin_read(data, path)
    if data[:4] in (b'PWAD', b'IWAD'):
        if len(data) < 12:
            sys.exit("%s: truncated wad header (%d bytes)" % (path, len(data)))
        _, n, off = struct.unpack('<4sii', data[:12])
        if n < 0 or off < 12 or off + n * 16 > len(data):
            sys.exit("%s: truncated or corrupt wad -- directory says %d lumps at "
                     "offset %d, file is %d bytes" % (path, n, off, len(data)))
        for i in range(n):
            fo, sz, nm = struct.unpack('<ii8s', data[off + i * 16: off + i * 16 + 16])
            if nm.rstrip(b'\0') == b'ENDOOM':
                if sz != LUMPSZ:
                    sys.exit("%s: ENDOOM is %d bytes, expected %d" % (path, sz, LUMPSZ))
                return data[fo:fo + sz]
        sys.exit("%s: no ENDOOM lump in this wad" % path)
    if len(data) != LUMPSZ:
        sys.exit("%s: %d bytes, expected a %d-byte lump (or a wad)"
                 % (path, len(data), LUMPSZ))
    return data


def cells_of(lump):
    return list(struct.unpack('<%dH' % NCELL, lump))


def cmd_show(args):
    cells = cells_of(read_lump(args.source))
    for r in range(ROWS):
        out = []
        for c in cells[r * COLS:(r + 1) * COLS]:
            att, ch = c >> 8, c & 0xFF
            out.append("\033[0m\033[%d;%dm%s"
                       % (ANSI_FG[att & 0x0F], ANSI_BG[(att >> 4) & 0x0F], CP437[ch]))
        sys.stdout.write(''.join(out) + "\033[0m\n")


HEADER = """\
# ENDOOM source for DoomLegacy -- 80 columns x 25 rows.
# Rebuild with:  tools/endoom.py build <this file> -o ENDOOM.lmp
# Or patch a wad directly:  tools/endoom.py replace common/legacy.wad <this file>
#
# Rows come in threes.  T is the text; F and B are the foreground and background
# colour of each character, one hex digit aligned under it:
#   0 black   1 blue    2 green   3 cyan    4 red   5 magenta   6 brown   7 lgray
#   8 dgray   9 lblue   A lgreen  B lcyan   C lred  D lmagenta  E yellow  F white
# Short lines are padded on the right (T with spaces, F/B with their last digit),
# so trailing whitespace being stripped by your editor is harmless.
# Lines starting with # are ignored.  Text is CP437, so box-drawing characters
# and the like can be pasted in directly.
#
#           1         2         3         4         5         6         7         8
#  12345678901234567890123456789012345678901234567890123456789012345678901234567890
"""


def cmd_dump(args):
    cells = cells_of(read_lump(args.source))

    # 0x00, 0x20 and 0xFF are all a blank in the engine's table, so the text
    # form cannot tell them apart and `build` will emit 0x20 for all three.
    # The screen is unchanged -- and in the non-UTF8 path, where endtxt.c
    # putchar()s the raw byte, a NUL or a stray 0xFF is worse than a space.
    odd = sum(1 for c in cells if (c & 0xFF) in (0x00, 0xFF))
    if odd:
        sys.stderr.write("note: %d cell(s) use 0x00/0xFF as a blank; rebuilding "
                         "will normalise them to 0x20 (space).\n"
                         "      The screen renders identically.  Use 'extract' "
                         "instead if you need the bytes preserved.\n" % odd)

    lines = []
    if args.plain:
        for r in range(ROWS):
            lines.append(''.join(CP437[c & 0xFF]
                                 for c in cells[r * COLS:(r + 1) * COLS]).rstrip())
    else:
        lines.append(HEADER.rstrip('\n'))
        for r in range(ROWS):
            row = cells[r * COLS:(r + 1) * COLS]
            lines.append("T " + ''.join(CP437[c & 0xFF] for c in row))
            lines.append("F " + ''.join("%X" % ((c >> 8) & 0x0F) for c in row))
            lines.append("B " + ''.join("%X" % ((c >> 12) & 0x0F) for c in row))
    text = '\n'.join(lines) + '\n'
    if args.output:
        write_file(args.output, text)
        print("wrote %s" % args.output)
    else:
        sys.stdout.write(text)


def parse_text(path):
    """Editable text form -> list of 2000 cells.  Exits with a located message."""
    rows = {'T': [], 'F': [], 'B': []}
    for lineno, raw in enumerate(read_file(path, 'r').splitlines(), 1):
        line = raw.rstrip('\n').rstrip('\r')
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        tag = line[0].upper()
        if tag not in rows or (len(line) > 1 and line[1] != ' '):
            sys.exit("%s:%d: expected a line starting 'T ', 'F ' or 'B ', got %r"
                     % (path, lineno, line[:12]))
        body = line[2:]
        if len(body) > COLS:
            sys.exit("%s:%d: %s row is %d columns, maximum is %d"
                     % (path, lineno, tag, len(body), COLS))
        rows[tag].append((lineno, body))

    for tag in 'TFB':
        if len(rows[tag]) != ROWS:
            sys.exit("%s: found %d '%s' rows, need exactly %d"
                     % (path, len(rows[tag]), tag, ROWS))

    cells = []
    for r in range(ROWS):
        (tln, text), (fln, fg), (bln, bg) = rows['T'][r], rows['F'][r], rows['B'][r]
        text = text.ljust(COLS)
        fg = fg.ljust(COLS, fg[-1] if fg else '7')
        bg = bg.ljust(COLS, bg[-1] if bg else '0')
        for c in range(COLS):
            ch = text[c]
            if ch not in ENC:
                sys.exit("%s:%d: column %d: %r (U+%04X) is not a CP437 character"
                         % (path, tln, c + 1, ch, ord(ch)))
            for digit, ln, what in ((fg[c], fln, 'foreground'), (bg[c], bln, 'background')):
                if digit not in '0123456789abcdefABCDEF':
                    sys.exit("%s:%d: column %d: %r is not a hex %s colour digit 0-F"
                             % (path, ln, c + 1, digit, what))
            cells.append((int(bg[c], 16) << 12) | (int(fg[c], 16) << 8) | ENC[ch])
    return cells


def build_lump(path):
    if path.lower().endswith(('.lmp', '.bin', '.xb')):
        return read_lump(path)
    return struct.pack('<%dH' % NCELL, *parse_text(path))


def cmd_build(args):
    lump = build_lump(args.source)
    out = args.output or 'ENDOOM.lmp'
    write_file(out, lump)
    print("wrote %s (%d bytes)" % (out, len(lump)))


def cmd_extract(args):
    lump = read_lump(args.source)
    out = args.output or 'ENDOOM.lmp'
    write_file(out, lump)
    print("wrote %s (%d bytes) -- raw cell data, no header.  If an ANSI editor "
          "will not\nopen it, use 'xbin' instead." % (out, len(lump)))


def cmd_xbin(args):
    lump = build_lump(args.source) if args.source.lower().endswith('.txt') \
        else read_lump(args.source)
    out = args.output or 'ENDOOM.xb'
    write_file(out, xbin_wrap(lump))
    print("wrote %s (%d bytes) -- open this in Moebius, PabloDraw or icy_draw.\n"
          "Save it back as XBin and feed it straight to 'build' or 'replace'."
          % (out, len(lump) + 11))


def cmd_replace(args):
    lump = build_lump(args.source)
    data = bytearray(read_file(args.wad))
    if bytes(data[:4]) not in (b'PWAD', b'IWAD'):
        sys.exit("%s: not a wad" % args.wad)
    _, n, off = struct.unpack('<4sii', bytes(data[:12]))
    for i in range(n):
        fo, sz, nm = struct.unpack('<ii8s', bytes(data[off + i * 16: off + i * 16 + 16]))
        if nm.rstrip(b'\0') == b'ENDOOM':
            if sz != len(lump):
                sys.exit("%s: ENDOOM is %d bytes, refusing to write %d in place"
                         % (args.wad, sz, len(lump)))
            if not args.no_backup:
                bak = args.wad + '.bak'
                shutil.copy2(args.wad, bak)
                print("backed up to %s" % bak)
            data[fo:fo + sz] = lump
            write_file(args.wad, bytes(data))
            print("replaced ENDOOM in %s (%d bytes at offset %d)" % (args.wad, sz, fo))
            return
    sys.exit("%s: no ENDOOM lump to replace" % args.wad)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)

    p = sub.add_parser('show', help='preview the screen in the terminal, in colour')
    p.add_argument('source')
    p.set_defaults(func=cmd_show)

    p = sub.add_parser('dump', help='write the editable text form')
    p.add_argument('source')
    p.add_argument('-o', '--output')
    p.add_argument('--plain', action='store_true', help='just the text, no colour rows')
    p.set_defaults(func=cmd_dump)

    p = sub.add_parser('build', help='editable text form -> 4000-byte lump')
    p.add_argument('source')
    p.add_argument('-o', '--output')
    p.set_defaults(func=cmd_build)

    p = sub.add_parser('extract', help='raw headerless lump out of a wad')
    p.add_argument('source')
    p.add_argument('-o', '--output')
    p.set_defaults(func=cmd_extract)

    p = sub.add_parser('xbin', help='write .xb for a graphical ANSI editor')
    p.add_argument('source')
    p.add_argument('-o', '--output')
    p.set_defaults(func=cmd_xbin)

    p = sub.add_parser('replace', help='patch a lump or text form into a wad in place')
    p.add_argument('wad')
    p.add_argument('source')
    p.add_argument('--no-backup', action='store_true')
    p.set_defaults(func=cmd_replace)

    args = ap.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
