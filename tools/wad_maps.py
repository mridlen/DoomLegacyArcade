#!/usr/bin/env python3
# [Arcade] Copy every map's lumps out of a wad into a PWAD.
#
# Used by tools/rebuild-nodes.sh to turn ZDBSP's rebuilt copy of an IWAD into
# a small PWAD that carries only the maps.  Nothing here is Doom data: it is
# a lump copier, and the wad it reads is the operator's own.
import re
import struct
import sys

MAPLUMPS = ("THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS",
            "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP")
MAPNAME = re.compile(r'^(E\dM\d|MAP\d\d)$')


def read_directory(data):
    sig, count, diroff = struct.unpack_from('<4sii', data, 0)
    if sig not in (b'IWAD', b'PWAD'):
        raise SystemExit("not a wad: %r" % sig)
    out = []
    for i in range(count):
        lo, ll, nm = struct.unpack_from('<ii8s', data, diroff + 16 * i)
        out.append((nm.rstrip(b'\0').decode('latin1'), lo, ll))
    return out


def main(src, out):
    data = open(src, 'rb').read()
    lumps = read_directory(data)

    take = []
    nmaps = 0
    i = 0
    while i < len(lumps):
        name = lumps[i][0]
        if MAPNAME.match(name):
            nmaps += 1
            take.append((name, b""))
            j = i + 1
            while j < len(lumps) and lumps[j][0] in MAPLUMPS:
                n2, lo2, ll2 = lumps[j]
                take.append((n2, data[lo2:lo2 + ll2]))
                j += 1
            i = j
            continue
        i += 1

    if not nmaps:
        raise SystemExit("no maps found in %s" % src)

    body = b""
    entries = []
    for name, blob in take:
        entries.append((name, 12 + len(body), len(blob)))
        body += blob
    diroff = 12 + len(body)
    directory = b"".join(
        struct.pack('<ii8s', lo, ll, nm.encode('latin1').ljust(8, b'\0'))
        for nm, lo, ll in entries)
    with open(out, 'wb') as f:
        f.write(struct.pack('<4sii', b'PWAD', len(entries), diroff))
        f.write(body)
        f.write(directory)
    print("  %s: %d maps, %d lumps, %.1f MB"
          % (out, nmaps, len(entries), (diroff + len(directory)) / 1048576.0))


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit("usage: wad_maps.py <source.wad> <out.wad>")
    main(sys.argv[1], sys.argv[2])
