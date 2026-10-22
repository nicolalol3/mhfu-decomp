"""Generates build/syms.ld: original addresses for func_XXXXXXXX and D_XXXXXXXX used by the C,
for the names in symbols.txt and for the imported system functions (sceXxx)."""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from config import BUILD, SRC, ROOT, load_functions, load_symbols

os.makedirs(BUILD, exist_ok=True)
names = set()
for d in (SRC, os.path.join(ROOT, "include")):
    for dp, _, files in os.walk(d):
        for fn in files:
            if fn.endswith((".c", ".h")):
                names |= set(re.findall(r"\b((?:func|D)_[0-9A-F]{8})\b", open(os.path.join(dp, fn)).read()))
names |= {"func_%08X" % a for a in load_functions()}

# PROVIDE: the symbol is used only if it is not defined by our C
with open(os.path.join(BUILD, "syms.ld"), "w") as f:
    for line in open(os.path.join(BUILD, "imports.tsv")):
        addr, _, _, name = line.rstrip("\n").split("\t")
        f.write("PROVIDE(%s = 0x%s);\n" % (name, addr))
    for n, a in sorted(load_symbols().items()):
        f.write("PROVIDE(%s = 0x%08X);\n" % (n, a))
    for n in sorted(names):
        f.write("PROVIDE(%s = 0x%s);\n" % (n, n[-8:]))
