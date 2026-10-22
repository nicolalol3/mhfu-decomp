"""Crea un EBOOT modificato: aggiunge il nuovo codice C e devia le funzioni originali.

Uso: python inject.py <newcode.bin> <newcode.nm> <EBOOT_out>
- Il nuovo codice viene caricato a NEWCODE_ADDR in un segmento ELF in più
  (riutilizziamo l'header di programma vuoto del segmento "debug.elf").
- All'inizio di ogni funzione originale riscritta in C mettiamo "j nuova; nop".
"""
import os, re, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
from config import ORIG_ELF, NEWCODE_ADDR, BUILD, load_functions, load_symbols

binf, nmf, out = sys.argv[1:4]
elf = bytearray(open(ORIG_ELF, "rb").read())
code = open(binf, "rb").read()

syms = {}
for line in open(nmf):
    p = line.split()
    if len(p) == 3:
        syms[p[2]] = int(p[0], 16)
memsz = syms["_newcode_end"] - NEWCODE_ADDR

phoff, = struct.unpack_from("<I", elf, 0x1C)
phnum, = struct.unpack_from("<H", elf, 0x2C)
main = struct.unpack_from("<8I", elf, phoff)
main_end = main[2] + main[5]
assert NEWCODE_ADDR >= main_end, hex(main_end)

# header di programma da riutilizzare: l'ultimo (debug.elf, tipo PT_NULL)
slot = phoff + (phnum - 1) * 32
assert struct.unpack_from("<I", elf, slot)[0] == 0
while len(elf) % 16:
    elf.append(0)
fileoff = len(elf)
elf += code
struct.pack_into("<8I", elf, slot, 1, fileoff, NEWCODE_ADDR, NEWCODE_ADDR, len(code), memsz, 7, 16)


def va2off(va):
    vaddr, filesz = main[2], main[4]
    assert vaddr <= va < vaddr + filesz
    return main[1] + (va - vaddr)


funcs = load_functions()
named = load_symbols()
replaced = []
for name, addr in sorted(syms.items()):
    if addr < NEWCODE_ADDR:
        continue
    m = re.fullmatch(r"func_([0-9A-F]{8})", name)
    if m:
        orig = int(m.group(1), 16)
    elif named.get(name) in funcs:
        orig = named[name]
    else:
        continue  # funzione nuova o di supporto, non sostituisce nulla
    assert funcs[orig] >= 8, name
    o = va2off(orig)
    struct.pack_into("<II", elf, o, 0x08000000 | ((addr >> 2) & 0x03FFFFFF), 0)
    replaced.append((orig, name, addr))

open(out, "wb").write(elf)
with open(os.path.join(BUILD, "replaced.tsv"), "w") as f:
    for orig, name, addr in sorted(replaced):
        f.write("%08X\t%s\t%08X\n" % (orig, name, addr))
print("Funzioni deviate al codice C: %d, nuovo codice: %d byte" % (len(replaced), memsz))
