"""Gira in WSL: dai prototipi degli header PSPSDK ricava quanti registri intero (a0-a3, t0-t3)
usano i parametri di ogni funzione sce* (convenzione EABI: i valori a 64 bit occupano una coppia
di registri allineata). I float vanno nei registri FPU e non sono contati.
Scrive tools/sce_argc.tsv (nome<TAB>indici dei registri, es. "0,2,3,4")."""
import os, re

INC = "/opt/pspdev/psp/sdk/include"
W64 = re.compile(r"\b(SceOff|SceInt64|SceUInt64|SceLong64|SceULong64|s64|u64|long\s+long|int64_t|uint64_t)\b")
FLT = re.compile(r"^(const\s+)?float\b")


def split_params(params):
    out, depth, cur = [], 0, ""
    for ch in params:
        depth += ch == "("
        depth -= ch == ")"
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    out.append(cur.strip())
    return out


def int_regs(params):
    """Indici dei registri intero usati (0=a0 ... 7=t3); i registri di allineamento sono esclusi."""
    regs, n = [], 0
    for p in split_params(params):
        if p in ("", "void", "..."):
            continue
        if "*" not in p and FLT.match(p):
            continue
        if "*" not in p and W64.search(p):
            n += n & 1
            regs += [n, n + 1]
            n += 2
        else:
            regs.append(n)
            n += 1
    return ",".join(str(r) for r in regs if r < 8)


out = {}
for dp, _, files in os.walk(INC):
    for fn in files:
        if fn.endswith(".h"):
            src = open(os.path.join(dp, fn), errors="ignore").read()
            src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
            for m in re.finditer(r"\b(sce\w+)\s*\(([^;{)]*(?:\([^)]*\)[^;{)]*)*)\)\s*;", src):
                out.setdefault(m.group(1), int_regs(m.group(2)))
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "sce_argc.tsv"), "w") as f:
    for k in sorted(out):
        f.write("%s\t%s\n" % (k, out[k]))
print(len(out))
