"""Legge la tabella delle importazioni dell'EBOOT e scrive build/imports.tsv:
indirizzo stub, libreria, NID, nome (dal database tools/nids.tsv)."""
import os, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
from config import ORIG_ELF, BUILD, ROOT

d = open(ORIG_ELF, "rb").read()
phoff, = struct.unpack_from("<I", d, 0x1C)
_, off, vaddr, _, filesz, _, _, _ = struct.unpack_from("<8I", d, phoff)
rd = lambda va, fmt: struct.unpack_from(fmt, d, off + va - vaddr)
cstr = lambda va: d[off + va - vaddr:d.index(b"\0", off + va - vaddr)].decode()
nids = {}
for fn in ("nids.tsv", "nids_extra.tsv"):  # nids_extra: NID noti assenti dal PSPSDK
    nids.update(l.rstrip("\n").split("\t") for l in open(os.path.join(ROOT, "tools", fn)))

# .lib.stub: 0x0890E6D4 - 0x0890E97C
va, end = 0x0890E6D4, 0x0890E97C
rows = []
while va < end:
    name_p, ver, attr, ln, vcount, count, nid_p, stub_p = rd(va, "<IHHBBHII")
    lib = cstr(name_p)
    for i in range(count):
        nid = "%08X" % rd(nid_p + 4 * i, "<I")[0]
        rows.append((stub_p + 8 * i, lib, nid, nids.get(nid, "%s_%s" % (lib, nid))))
    va += ln * 4
os.makedirs(BUILD, exist_ok=True)
with open(os.path.join(BUILD, "imports.tsv"), "w") as f:
    for r in rows:
        f.write("%08X\t%s\t%s\t%s\n" % r)
known = sum(1 for r in rows if not r[3].endswith(r[2]))
print("Import: %d, con nome: %d" % (len(rows), known))
