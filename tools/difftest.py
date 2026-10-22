"""Test differenziale: esegue la funzione originale e la versione in C sugli stessi input
in una CPU MIPS emulata (unicorn) e confronta valori di ritorno e memoria scritta.

Uso: tools/.venv/Scripts/python.exe -I tools/difftest.py [func_XXXXXXXX ...] [--runs N]
Senza nomi testa tutte le funzioni presenti in build/newcode.nm.
Esito per funzione: OK (n input confrontati), DIFF (differenza trovata), NON TESTABILE.
"""
import os, random, re, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
from config import ORIG_ELF, BUILD, NEWCODE_ADDR
from difftest_specs import SPECS
from unicorn import Uc, UcError, UC_ARCH_MIPS, UC_MODE_MIPS32, UC_MODE_LITTLE_ENDIAN, \
    UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_INTR
from unicorn.mips_const import *

MEM_BASE, MEM_SIZE = 0x08800000, 0x01800000      # memoria utente PSP 0x08800000-0x0A000000
RET_MAGIC = 0x08800010                            # indirizzo di ritorno finto: lì ci fermiamo
STACK_TOP = 0x09FF0000
SCRATCH = 0x09E00000                              # buffer a cui puntano gli argomenti "puntatore"
SCRATCH_SIZE = 0x8000
STUB_LO, STUB_HI = 0x0890DE38, 0x0890DE38 + 0x880  # chiamate al sistema operativo (sceXxx)
MAX_INSNS = 2_000_000
STUB_RET_CELL = 0x20
STUB_CODE = struct.pack("<II", 0x03E00008, 0x8C020000 | STUB_RET_CELL)  # jr ra; lw v0, 0x20(zero)

ARG_REGS = [UC_MIPS_REG_A0, UC_MIPS_REG_A1, UC_MIPS_REG_A2, UC_MIPS_REG_A3,
            UC_MIPS_REG_T0, UC_MIPS_REG_T1, UC_MIPS_REG_T2, UC_MIPS_REG_T3]
FARG_REGS = [UC_MIPS_REG_F12 + i for i in range(8)]


def load_stub_argc():
    """Per ogni stub di sistema: quanti argomenti confrontare (dagli header PSPSDK)."""
    argc = {}
    for fn in ("sce_argc.tsv", "sce_argc_extra.tsv"):
        p = os.path.join(os.path.dirname(__file__), fn)
        if os.path.exists(p):
            for l in open(p):
                k, _, v = l.rstrip("\n").partition("\t")
                argc[k] = [int(x) for x in v.split(",") if x]
    out = {}
    for line in open(os.path.join(BUILD, "imports.tsv")):
        addr, _, _, name = line.rstrip("\n").split("\t")
        out[int(addr, 16)] = (name, argc.get(name, [0, 1, 2, 3]))
    return out


STUBS = load_stub_argc()


def load_image():
    d = open(ORIG_ELF, "rb").read()
    phoff, = struct.unpack_from("<I", d, 0x1C)
    _, off, vaddr, _, filesz, memsz, _, _ = struct.unpack_from("<8I", d, phoff)
    return vaddr, d[off:off + filesz]


def sx(v):
    return v - (1 << 32) if v & 0x80000000 else v


class Machine:
    def __init__(self, image, newcode):
        self.uc = uc = Uc(UC_ARCH_MIPS, UC_MODE_MIPS32 + UC_MODE_LITTLE_ENDIAN)
        uc.mem_map(MEM_BASE, MEM_SIZE)
        uc.mem_map(0, 0x1000)  # pagina per STUB_RET_CELL
        # memoria iniziale: EBOOT originale + nuovo codice + stub emulati, il resto a zero
        va, img = image
        pristine = bytearray(MEM_SIZE)
        pristine[va - MEM_BASE:va - MEM_BASE + len(img)] = img
        pristine[NEWCODE_ADDR - MEM_BASE:NEWCODE_ADDR - MEM_BASE + len(newcode)] = newcode
        pristine[STUB_LO - MEM_BASE:STUB_HI - MEM_BASE] = STUB_CODE * ((STUB_HI - STUB_LO) // 8)
        self.pristine = bytes(pristine)
        # Niente hook sulle scritture: in unicorn 2.1.4 un hook sulle scritture eseguito nello slot
        # di ritardo di un salto rompe il salto. La memoria viene invece ripristinata e confrontata
        # per intero a ogni esecuzione.
        uc.hook_add(UC_HOOK_CODE, self._on_code)
        uc.hook_add(UC_HOOK_INTR, self._on_intr)
        self.clean_ctx = uc.context_save()  # stato CPU pulito, ripristinato a ogni esecuzione

    # --- hook -------------------------------------------------------------
    def _on_code(self, uc, addr, size, _):
        self.count += 1
        self.last_pc = addr
        if self.count > MAX_INSNS:
            self.error = "troppe istruzioni (loop infinito?)"
            uc.emu_stop()
        elif addr == RET_MAGIC:
            uc.emu_stop()
        elif STUB_LO <= addr < STUB_HI and (addr - STUB_LO) % 8 == 0:
            # funzione di sistema: registriamo la chiamata e ritorniamo 0
            name, regs = STUBS.get(addr, ("?", [0, 1, 2, 3]))
            self.calls.append((name, tuple(self._arg_key(uc.reg_read(ARG_REGS[r])) for r in regs)))
            # valore di ritorno pseudo-casuale ma identico nelle due esecuzioni
            r = random.Random(self.seed * 1000 + len(self.calls))
            # scrivere registri dentro un hook manda in errore unicorn: lo stub emulato
            # ("jr ra; lw v0, 0x20(zero)") legge il valore di ritorno da questa cella di memoria
            uc.mem_write(STUB_RET_CELL, struct.pack("<I", r.choice([0, 0, 1, 2, 0x40, 0x80020001, 0xFFFFFFFF])))

    def _arg_key(self, v):
        """Un argomento puntatore vale per il suo contenuto: stringhe e buffer possono stare
        a indirizzi diversi nel C ricompilato; i puntatori allo stack sono tutti equivalenti."""
        if STACK_TOP - 0x20000 <= v < STACK_TOP:
            return "STACK"
        if NEWCODE_ADDR <= v < NEWCODE_ADDR + 0x100000 or 0x0890F8F8 <= v < 0x089CBA90:
            data = bytes(self.uc.mem_read(v, 32))
            end = data.find(b"\0")
            if end > 0 and all(32 <= c < 127 or c in (9, 10, 13) for c in data[:end]):
                data = data[:end]  # stringa C: conta solo fino al terminatore
            return ("PTR", data)
        return v

    def _on_intr(self, uc, intno, _):
        # Modificare registri dentro un hook manda in errore unicorn: ci fermiamo e
        # l'istruzione viene emulata fuori dall'hook (vedi _resolve_trap).
        # (dentro questo hook il PC letto vale 0: usiamo l'ultima istruzione eseguita)
        self.trap_pc = self.last_pc
        uc.emu_stop()

    def _insn(self, addr):
        return struct.unpack("<I", self.uc.mem_read(addr, 4))[0]

    def _branch_target(self, bpc, insn):
        """Se insn è un salto, ritorna (preso?, destinazione, registro link, likely); altrimenti None."""
        uc = self.uc
        R = lambda n: uc.reg_read(UC_MIPS_REG_0 + n)
        op, rs, rt, rd, fn = insn >> 26, (insn >> 21) & 31, (insn >> 16) & 31, (insn >> 11) & 31, insn & 63
        off = bpc + 4 + (((insn & 0xFFFF) ^ 0x8000) - 0x8000) * 4
        if op in (2, 3):
            return True, ((bpc + 4) & 0xF0000000) | ((insn & 0x3FFFFFF) << 2), (31 if op == 3 else 0), False
        if op == 0 and fn in (8, 9):
            return True, R(rs), (rd if fn == 9 else 0), False
        conds = {4: lambda: R(rs) == R(rt), 5: lambda: R(rs) != R(rt),
                 6: lambda: sx(R(rs)) <= 0, 7: lambda: sx(R(rs)) > 0}
        if op in conds:
            return conds[op](), off, 0, False
        if op - 0x10 in conds and op >= 0x14:
            return conds[op - 0x10](), off, 0, True
        if op == 1 and rt in (0, 1, 2, 3, 0x10, 0x11):
            taken = sx(R(rs)) < 0 if rt in (0, 2, 0x10) else sx(R(rs)) >= 0
            return taken, off, (31 if rt >= 0x10 else 0), rt in (2, 3)
        return None

    def _resolve_trap(self, pc):
        """Emula l'istruzione Allegrex che ha fermato la CPU; ritorna il PC da cui ripartire o None."""
        if not MEM_BASE + 4 <= pc < MEM_BASE + MEM_SIZE:
            self.error = "salto a un indirizzo non valido (%08X)" % pc
            return None
        insn = self._insn(pc)
        bpc = None
        if self._branch_target(pc, insn) is not None:
            bpc = pc  # fermati sul salto: l'istruzione non supportata è nello slot di ritardo
        elif self._branch_target(pc - 4, self._insn(pc - 4)) is not None:
            bpc = pc - 4
        if bpc is None:
            if not self._emulate_allegrex(insn):
                return self._unsupported(insn, pc)
            return pc + 4
        taken, target, link, likely = self._branch_target(bpc, self._insn(bpc))
        if link:
            self.uc.reg_write(UC_MIPS_REG_0 + link, bpc + 8)
        if taken or not likely:
            slot = self._insn(bpc + 4)
            if not self._emulate_allegrex(slot):
                return self._unsupported(slot, bpc + 4)
        return target if taken else bpc + 8

    def _unsupported(self, insn, pc):
        self.error = "istruzione non supportata %08X a %08X" % (insn, pc)
        self.untestable = (insn >> 26) in (0x18, 0x19, 0x1B, 0x34, 0x36, 0x37, 0x3C, 0x3D, 0x3E, 0x3F, 0x32, 0x35, 0x3A)
        return None

    def _emulate_allegrex(self, insn):
        """Istruzioni Allegrex che non esistono nel MIPS standard."""
        uc = self.uc
        op, rs, rt, rd, sa, fn = insn >> 26, (insn >> 21) & 31, (insn >> 16) & 31, (insn >> 11) & 31, (insn >> 6) & 31, insn & 63
        R = lambda n: uc.reg_read(UC_MIPS_REG_0 + n)
        W = lambda n, v: n and uc.reg_write(UC_MIPS_REG_0 + n, v & 0xFFFFFFFF)
        if op == 0:
            if fn in (0x2C, 0x2D):  # min / max
                a, b = sx(R(rs)), sx(R(rt))
                W(rd, min(a, b) if fn == 0x2C else max(a, b))
                return True
            if fn in (0x1C, 0x1D, 0x2E, 0x2F):  # madd/maddu/msub/msubu
                hilo = (uc.reg_read(UC_MIPS_REG_HI) << 32) | uc.reg_read(UC_MIPS_REG_LO)
                if fn in (0x1C, 0x2E):
                    p = sx(R(rs)) * sx(R(rt))
                else:
                    p = R(rs) * R(rt)
                hilo = (hilo + p if fn in (0x1C, 0x1D) else hilo - p) & 0xFFFFFFFFFFFFFFFF
                uc.reg_write(UC_MIPS_REG_HI, hilo >> 32)
                uc.reg_write(UC_MIPS_REG_LO, hilo & 0xFFFFFFFF)
                return True
        if op == 0x1F and fn == 0x20 and sa == 0x14:  # bitrev
            W(rd, int("{:032b}".format(R(rt))[::-1], 2))
            return True
        return False

    # --- esecuzione -------------------------------------------------------
    def run(self, func, args, fargs, scratch, seed=0):
        uc = self.uc
        uc.context_restore(self.clean_ctx)
        uc.mem_write(MEM_BASE, self.pristine)  # memoria iniziale identica per ogni esecuzione
        uc.mem_write(SCRATCH, scratch)
        uc.mem_write(RET_MAGIC, b"\0" * 8)
        for i in range(1, 32):
            uc.reg_write(UC_MIPS_REG_0 + i, 0)
        for r, v in zip(ARG_REGS, args):
            uc.reg_write(r, v)
        for r, v in zip(FARG_REGS, fargs):
            uc.reg_write(r, v)
        uc.reg_write(UC_MIPS_REG_SP, STACK_TOP - 0x100)
        uc.reg_write(UC_MIPS_REG_RA, RET_MAGIC)
        self.count, self.calls, self.error, self.untestable = 0, [], None, False
        self.seed = seed
        pc = func
        while pc is not None:
            self.trap_pc = None
            try:
                uc.emu_start(pc, RET_MAGIC)
            except UcError as e:
                self.error = "errore CPU: %s a %08X" % (e, uc.reg_read(UC_MIPS_REG_PC))
            if self.trap_pc is None or self.error:
                break
            pc = self._resolve_trap(self.trap_pc)
        return dict(
            error=self.error, untestable=self.untestable,
            v0=uc.reg_read(UC_MIPS_REG_V0), v1=uc.reg_read(UC_MIPS_REG_V1),
            f0=uc.reg_read(UC_MIPS_REG_F0), calls=self.calls,
            # memoria finale, esclusa la zona dello stack (le variabili locali possono differire)
            mem=(bytes(uc.mem_read(MEM_BASE, STACK_TOP - 0x20000 - MEM_BASE)),
                 bytes(uc.mem_read(STACK_TOP, MEM_BASE + MEM_SIZE - STACK_TOP))))


SPECIAL_FLOATS = [0x00000000, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC00000, 0x3F800000, 0xBF800000,
                  0x3F000000, 0x40000000, 0x00000001, 0x007FFFFF, 0x00800000, 0x7F7FFFFF, 0x3FC90FDB]


def random_float_bits(rng):
    """Float di prova: valori comuni, valori speciali (0, inf, NaN, subnormali) ed esponenti a caso."""
    k = rng.random()
    if k < 0.5:
        return struct.unpack("<I", struct.pack("<f", rng.uniform(-1000, 1000)))[0]
    if k < 0.65:
        return rng.choice(SPECIAL_FLOATS) ^ (rng.random() < 0.2 and 0x80000000 or 0)
    return rng.getrandbits(32)


def make_inputs(rng):
    args = []
    for _ in ARG_REGS:
        k = rng.random()
        if k < 0.45:
            args.append(SCRATCH + rng.randrange(0, SCRATCH_SIZE // 2, 4))
        elif k < 0.75:
            args.append(rng.randrange(0, 64))
        else:
            args.append(rng.getrandbits(32))
    fargs = [random_float_bits(rng) for _ in FARG_REGS]
    scratch = bytes(rng.getrandbits(8) for _ in range(SCRATCH_SIZE))
    return args, fargs, scratch


def load_return_kinds():
    """Dal C ricava cosa confrontare al ritorno: void (niente), float (f0), 64 bit (v0+v1), altro (v0)."""
    kinds = {}
    for dp, _, files in os.walk(os.path.join(os.path.dirname(BUILD), "src")):
        for fn in files:
            if fn.endswith(".c"):
                src = open(os.path.join(dp, fn)).read()
                for m in re.finditer(r"^((?:static\s+)?[A-Za-z_][\w \*]*?)\s*\b(\w+)\s*\([^;{]*\)\s*\{", src, re.M):
                    t = m.group(1).replace("static", "").strip()
                    if t == "void":
                        kinds[m.group(2)] = ()
                    elif t in ("f32", "float"):
                        kinds[m.group(2)] = ("f0",)
                    elif t in ("s64", "u64", "SceOff", "long long", "f64", "double"):
                        kinds[m.group(2)] = ("v0", "v1")
                    else:
                        kinds[m.group(2)] = ("v0",)
    return kinds


def compare(a, b, regs=("v0",)):
    for k in regs:
        if a[k] != b[k]:
            return "%s: originale %08X, C %08X" % (k, a[k], b[k])
    if a["mem"] != b["mem"]:
        # la zona del nuovo codice (dati privati del C) non va confrontata
        lo, hi = NEWCODE_ADDR - MEM_BASE, NEWCODE_ADDR - MEM_BASE + 0x100000
        ma, mb = a["mem"][0], b["mem"][0]
        if ma[:lo] != mb[:lo] or ma[hi:] != mb[hi:] or a["mem"][1] != b["mem"][1]:
            base = [MEM_BASE, STACK_TOP]
            for k in (0, 1):
                x, y = a["mem"][k], b["mem"][k]
                for i in range(0, len(x), 0x1000):
                    if x[i:i + 0x1000] != y[i:i + 0x1000] and not (k == 0 and lo <= i < hi):
                        j = next(j for j in range(i, i + 0x1000) if x[j] != y[j])
                        return "memoria scritta diversa (es. %08X)" % (base[k] + j)
    if a["calls"] != b["calls"]:
        for x, y in zip(a["calls"] + [None] * 99, b["calls"]):
            if x != y:
                return "chiamate di sistema diverse: originale %s, C %s" % (x, y)
        return "chiamate di sistema diverse (numero)"
    return None


def main():
    argv = sys.argv[1:]
    runs = 300
    if "--runs" in argv:
        i = argv.index("--runs"); runs = int(argv[i + 1]); del argv[i:i + 2]
    syms, origs = {}, {}
    for line in open(os.path.join(BUILD, "replaced.tsv")):
        orig, name, addr = line.split()
        syms[name], origs[name] = int(addr, 16), int(orig, 16)
    names = argv or sorted(syms)
    kinds = load_return_kinds()
    m = Machine(load_image(), open(os.path.join(BUILD, "newcode.bin"), "rb").read())
    results = {}
    for name in names:
        orig, new = origs[name], syms[name]
        rng = random.Random(orig)
        ok, skipped, status = 0, 0, None
        for run in range(runs):
            args, fargs, scratch = make_inputs(rng)
            if name in SPECS:
                args, fargs, scratch = SPECS[name](rng, args, fargs, scratch)
            a = m.run(orig, args, fargs, scratch, run)
            if a["untestable"]:
                status = "NON TESTABILE (%s)" % a["error"]; break
            if a["error"]:
                skipped += 1; continue
            b = m.run(new, args, fargs, scratch, run)
            d = b["error"] or compare(a, b, kinds.get(name, ("v0",)))
            if d:
                status = "DIFF: %s  [args %s]" % (d, " ".join("%08X" % x for x in args[:4])); break
            ok += 1
        if status is None:
            status = "OK (%d input confrontati)" % ok if ok else "NON TESTABILE (tutti gli input vanno in errore)"
        results[name] = status
        print("%-14s %s" % (name, status))
    return results


if __name__ == "__main__":
    main()
