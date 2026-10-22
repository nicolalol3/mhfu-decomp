"""Differential test: runs the original function and the C version on the same inputs
in an emulated MIPS CPU (unicorn) and compares return values and written memory.

Usage: tools/.venv/Scripts/python.exe -I tools/difftest.py [func_XXXXXXXX ...] [--runs N]
Without names it tests all the functions present in build/newcode.nm.
Result per function: OK (n inputs compared), DIFF (difference found), NOT TESTABLE.
"""
import os, random, re, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
from config import ORIG_ELF, BUILD, NEWCODE_ADDR
from difftest_specs import SPECS
from unicorn import Uc, UcError, UC_ARCH_MIPS, UC_MODE_MIPS32, UC_MODE_LITTLE_ENDIAN, \
    UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_INTR, UC_PROT_READ, UC_PROT_WRITE
from unicorn.mips_const import *

MEM_BASE, MEM_SIZE = 0x08800000, 0x01800000      # PSP user memory 0x08800000-0x0A000000
RET_MAGIC = 0x08800010                            # fake return address: we stop there
STACK_TOP = 0x09FF0000
SCRATCH = 0x09E00000                              # buffer pointed to by the "pointer" arguments
SCRATCH_SIZE = 0x8000
STUB_LO, STUB_HI = 0x0890DE38, 0x0890DE38 + 0x880  # calls to the operating system (sceXxx)
MAX_INSNS = 2_000_000
STUB_RET_CELL = 0x20
STUB_CODE = struct.pack("<II", 0x03E00008, 0x8C020000 | STUB_RET_CELL)  # jr ra; lw v0, 0x20(zero)

ARG_REGS = [UC_MIPS_REG_A0, UC_MIPS_REG_A1, UC_MIPS_REG_A2, UC_MIPS_REG_A3,
            UC_MIPS_REG_T0, UC_MIPS_REG_T1, UC_MIPS_REG_T2, UC_MIPS_REG_T3]
FARG_REGS = [UC_MIPS_REG_F12 + i for i in range(8)]
REG_BY_NAME = dict(zip(["a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3"], range(8)))

# --- "dummy functions" mode for the game code ---------------------------------------
# The game functions call hundreds of other functions: in the test these are replaced
# by dummy functions that record their arguments and return pseudo-random values
# (the same for the original and the C). This checks the logic of the single function tested:
# same calls, with the same arguments, in the same order, and the same memory written.
GAME_START = 0x08811C84                           # before this: C library (really executed)
FSTUB_CODE_ADDR = 0x08800100
FN_RET_CELL = 0x08800200                          # v0, v1, f0 returned by the dummy functions (far from page 0)
# lui at,0x0880; lw v0,0x200(at); lw v1,0x204(at); lwc1 f0,0x208(at); jr ra; nop
FSTUB_CODE = struct.pack("<6I", 0x3C010880, 0x8C220200, 0x8C230204, 0xC4200208, 0x03E00008, 0)
# functions with variable arguments: position of the "format" argument
VARIADIC = {0x08891474: 3, 0x088915B0: 4, 0x088920A8: 3, 0x0889134C: 1, 0x08892004: 1, 0x0889183C: 4}
# global variables (pointers) read by the game code -> large offset used with them (0 = none)
GAME_GLOBALS = {0x089CC438: 0x6AF14, 0x08A631A8: 0, 0x08A631A0: 0xF7A574, 0x08A62C50: 0, 0x08A62C54: 0,
                0x08A62C7C: 0, 0x08A62D8C: 0, 0x09A44C5C: 0, 0x09B22D70: 0, 0x09A4F0C4: 0, 0x089CBBE8: 0,
                0x08A62C40: 0, 0x089CBBEC: 0}


def load_signatures():
    """build/signatures.tsv (from Ghidra): {address: [(position, byte), ...]}"""
    sigs = {}
    p = os.path.join(BUILD, "signatures.tsv")
    if os.path.exists(p):
        for line in open(p):
            f = line.rstrip("\n").split("\t")
            if len(f) < 3 or f[2] == "?":
                continue
            sigs[int(f[0], 16)] = [(x.split(":")[0], int(x.split(":")[1])) for x in f[2].split(",") if x]
    sigs[0x0890BFA4] = [("a0", 4)]  # operator delete (Ghidra does not see the argument)
    return sigs


def load_return_sizes():
    """Size (bytes) of the value returned by each function, according to Ghidra (4 if unknown)."""
    out = {}
    p = os.path.join(BUILD, "signatures.tsv")
    if os.path.exists(p):
        for line in open(p):
            f = line.rstrip("\n").split("\t")
            if len(f) >= 2 and ":" in f[1]:
                out[int(f[0], 16)] = int(f[1].split(":")[1]) or 4
    return out


RET_SIZES = load_return_sizes()


def make_fstubs(name, syms, origs, sigs):
    """Dummy functions to test the game function "name": all the other game functions
    (original and already in C) and those of the overlays called by the C."""
    from config import load_functions
    orig = origs[name]
    fstubs = {a: (a, sigs.get(a, [])) for a, sz in load_functions().items()
              if a >= GAME_START and a != orig and sz >= 8}
    for dp, _, files in os.walk(os.path.join(os.path.dirname(BUILD), "src")):
        for fn in files:
            if fn.endswith(".c"):
                src = open(os.path.join(dp, fn), encoding="utf-8").read()
                for x in re.findall(r"func_(09[0-9A-F]{6})", src):
                    fstubs[int(x, 16)] = (int(x, 16), sigs.get(int(x, 16), []))
    for n2, a2 in syms.items():  # the other functions already in C, called directly by the C
        if n2 != name and origs[n2] >= GAME_START:
            fstubs[a2] = (origs[n2], sigs.get(origs[n2], []))
    return fstubs


def narrow_args(args, sig):
    """Arguments of 1 or 2 bytes arrive from the game already extended to 32 bits: in the test we use
    small positive values, valid both as signed and unsigned numbers."""
    for loc, size in sig:
        if loc in REG_BY_NAME and size < 4:
            i = REG_BY_NAME[loc]
            args[i] &= 0x7F if size == 1 else 0x7FFF
    return args


def load_stub_argc():
    """For each system stub: how many arguments to compare (from the PSPSDK headers)."""
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
    def __init__(self, image, newcode, fstubs=None):
        # initial memory: original EBOOT + new code + emulated stubs, the rest zeroed
        va, img = image
        pristine = bytearray(MEM_SIZE)
        pristine[va - MEM_BASE:va - MEM_BASE + len(img)] = img
        pristine[NEWCODE_ADDR - MEM_BASE:NEWCODE_ADDR - MEM_BASE + len(newcode)] = newcode
        pristine[STUB_LO - MEM_BASE:STUB_HI - MEM_BASE] = STUB_CODE * ((STUB_HI - STUB_LO) // 8)
        # dummy functions: at the start of each one "j FSTUB_CODE; nop"
        self.fstubs = fstubs or {}
        if self.fstubs:
            pristine[FSTUB_CODE_ADDR - MEM_BASE:FSTUB_CODE_ADDR - MEM_BASE + len(FSTUB_CODE)] = FSTUB_CODE
            jump = struct.pack("<II", 0x08000000 | ((FSTUB_CODE_ADDR >> 2) & 0x03FFFFFF), 0)
            for a in self.fstubs:
                pristine[a - MEM_BASE:a - MEM_BASE + 8] = jump
        self.globals = {}
        self.pristine = bytes(pristine)
        self._new_uc()

    def _new_uc(self):
        """Creates (or recreates) the emulated CPU. After a stop on an Allegrex instruction, unicorn 2.1.4
        sometimes crashes on the next test: in that case it is recreated from scratch."""
        self.uc = uc = Uc(UC_ARCH_MIPS, UC_MODE_MIPS32 + UC_MODE_LITTLE_ENDIAN)
        uc.mem_map(MEM_BASE, MEM_SIZE)
        # page for STUB_RET_CELL, not executable: a jump to 0 must fail at once (running
        # code here while the stub writes to it crashes unicorn)
        uc.mem_map(0, 0x1000, UC_PROT_READ | UC_PROT_WRITE)
        # No hooks on writes: in unicorn 2.1.4 a write hook run in the delay slot
        # of a jump breaks the jump. The memory is instead restored and compared
        # in full at every run.
        uc.hook_add(UC_HOOK_CODE, self._on_code)
        uc.hook_add(UC_HOOK_INTR, self._on_intr)
        self.clean_ctx = uc.context_save()  # clean CPU state, restored at every run
        self.code_patched = False             # see stop_at in difftest_specs.py
        self.had_trap = False

    # --- hook -------------------------------------------------------------
    def _on_code(self, uc, addr, size, _):
        self.count += 1
        self.last_pc = addr
        if self.count > MAX_INSNS:
            self.error = "too many instructions (infinite loop?)"
            uc.emu_stop()
        elif addr == RET_MAGIC:
            uc.emu_stop()
        elif addr in self.fstubs:
            self._fake_call(uc, addr)
        elif STUB_LO <= addr < STUB_HI and (addr - STUB_LO) % 8 == 0:
            # system function: we record the call and return 0
            name, regs = STUBS.get(addr, ("?", [0, 1, 2, 3]))
            self.calls.append((name, tuple(self._arg_key(uc.reg_read(ARG_REGS[r])) for r in regs)))
            # pseudo-random return value, but identical in the two runs
            r = random.Random(self.seed * 1000 + len(self.calls))
            # writing registers inside a hook makes unicorn fail: the emulated stub
            # ("jr ra; lw v0, 0x20(zero)") reads the return value from this memory cell
            uc.mem_write(STUB_RET_CELL, struct.pack("<I", r.choice([0, 0, 1, 2, 0x40, 0x80020001, 0xFFFFFFFF])))

    def _fake_call(self, uc, addr):
        """Dummy function: records the name and the arguments (per the Ghidra signature, reduced to the
        their size) and prepares the return value."""
        orig, sig = self.fstubs[addr]
        sp = uc.reg_read(UC_MIPS_REG_SP)

        def arg(loc, size):
            if loc in REG_BY_NAME:
                v = uc.reg_read(ARG_REGS[REG_BY_NAME[loc]])
            elif loc.startswith("f") and loc[1:].isdigit():
                v = uc.reg_read(UC_MIPS_REG_F0 + int(loc[1:])) & 0xFFFFFFFF
            elif loc.startswith("S"):
                v = struct.unpack("<I", uc.mem_read(sp + int(loc[1:]), 4))[0]
            else:
                return "?"
            if size < 4:
                return v & ((1 << (8 * size)) - 1)
            return self._arg_key(v & 0xFFFFFFFF)
        if orig in VARIADIC:
            k = VARIADIC[orig]
            regs = ["a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3"]
            vals = [arg(r, 4) for r in regs[:k + 1]]
            fmt = uc.reg_read(ARG_REGS[k])
            n = 0
            if MEM_BASE <= fmt < 0x09000000:  # only real game formats (not random test text)
                t = bytes(uc.mem_read(fmt, 256)).split(b"\0")[0]
                n = t.replace(b"%%", b"").count(b"%")
            extra = regs[k + 1:] + ["S0", "S4", "S8", "S12"]
            vals += [arg(r, 4) for r in extra[:n]]
        else:
            vals = [arg(loc, size) for loc, size in sig]
        self.calls.append(("%08X" % orig, tuple(vals)))
        r = random.Random(self.seed * 1000 + len(self.calls) + 7)
        # values valid whatever the real type (Ghidra often says 4 bytes even for a char):
        # small positive numbers or pointers to the test area
        v0 = r.choice([0, 0, 1, 1, 2, 3, 5, 0x7F, r.randrange(0, 0x80),
                       SCRATCH + r.randrange(0, 0x7000, 4), SCRATCH + r.randrange(0, 0x7000, 4)])
        rsize = RET_SIZES.get(orig, 4)
        if rsize < 4:  # 1 or 2 byte value: small and positive (as the game returns it)
            v0 &= 0x7F if rsize == 1 else 0x7FFF
        f0 = struct.unpack("<I", struct.pack("<f", r.uniform(-100, 100)))[0]
        uc.mem_write(FN_RET_CELL, struct.pack("<III", v0, r.getrandbits(32), f0))

    def _arg_key(self, v):
        """A pointer argument stands for its contents: strings and buffers can be
        at different addresses in the recompiled C; all stack pointers are equivalent."""
        if STACK_TOP - 0x20000 <= v < STACK_TOP:
            return "STACK"
        if NEWCODE_ADDR <= v < NEWCODE_ADDR + 0x100000 or 0x0890F8F8 <= v < 0x089CBA90:
            data = bytes(self.uc.mem_read(v, 32))
            end = data.find(b"\0")
            if end > 0 and all(32 <= c < 127 or c in (9, 10, 13) for c in data[:end]):
                data = data[:end]  # C string: counts only up to the terminator
            return ("PTR", data)
        return v

    def _on_intr(self, uc, intno, _):
        # Changing registers inside a hook makes unicorn fail: we stop and
        # the instruction is emulated outside the hook (see _resolve_trap).
        # (inside this hook the PC read is 0: we use the last instruction executed)
        self.trap_pc = self.last_pc
        self.intno = intno
        self.had_trap = True      # on the next run the emulated CPU is recreated
        uc.emu_stop()

    def _insn(self, addr):
        return struct.unpack("<I", self.uc.mem_read(addr, 4))[0]

    def _branch_target(self, bpc, insn):
        """If insn is a jump, returns (taken?, target, link register, likely); otherwise None."""
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
        """Emulates the Allegrex instruction that stopped the CPU; returns the PC to restart from, or None."""
        if not MEM_BASE + 4 <= pc < MEM_BASE + MEM_SIZE:
            self.error = "jump to an invalid address (%08X)" % pc
            return None
        insn = self._insn(pc)
        bpc = None
        if self._branch_target(pc, insn) is not None:
            bpc = pc  # stop on the jump: the unsupported instruction is in the delay slot
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
            if not self._emulate_allegrex(slot) and not self._exec_single(slot):
                return self._unsupported(slot, bpc + 4)
        return target if taken else bpc + 8

    SINGLE_ADDR = 0x08800300  # area where a delay-slot instruction is run alone

    def _exec_single(self, insn):
        """Runs with unicorn a single normal instruction (not a jump) copied separately: needed
        when unicorn stops on a jump whose delay slot contains a common instruction."""
        if insn == 0 or self._branch_target(self.SINGLE_ADDR, insn) is not None or (insn >> 26) in (
                0x18, 0x19, 0x1B, 0x34, 0x36, 0x37, 0x3C, 0x3D, 0x3E, 0x3F, 0x32, 0x35, 0x3A, 0x1C, 0x1F):
            return insn == 0
        uc = self.uc
        uc.mem_write(self.SINGLE_ADDR, struct.pack("<II", insn, 0))
        uc.ctl_flush_tb()
        self.trap_pc = None
        try:
            uc.emu_start(self.SINGLE_ADDR, self.SINGLE_ADDR + 4, count=1)
        except UcError as e:
            self.error = "CPU error: %s (delay slot)" % e
            return False
        return self.trap_pc is None

    def _unsupported(self, insn, pc):
        self.error = "unsupported instruction %08X at %08X" % (insn, pc)
        self.untestable = (insn >> 26) in (0x18, 0x19, 0x1B, 0x34, 0x36, 0x37, 0x3C, 0x3D, 0x3E, 0x3F, 0x32, 0x35, 0x3A)
        return None

    def _emulate_allegrex(self, insn):
        """Allegrex instructions that do not exist in standard MIPS."""
        uc = self.uc
        op, rs, rt, rd, sa, fn = insn >> 26, (insn >> 21) & 31, (insn >> 16) & 31, (insn >> 11) & 31, (insn >> 6) & 31, insn & 63
        R = lambda n: uc.reg_read(UC_MIPS_REG_0 + n)
        W = lambda n, v: n and uc.reg_write(UC_MIPS_REG_0 + n, v & 0xFFFFFFFF)
        if op == 0:
            if fn in (0x16, 0x17):  # clz / clo
                v = R(rs) if fn == 0x16 else (~R(rs)) & 0xFFFFFFFF
                W(rd, 32 - v.bit_length())
                return True
            if fn in (0x2C, 0x2D):  # max (0x2C) / min (0x2D)
                a, b = sx(R(rs)), sx(R(rt))
                W(rd, max(a, b) if fn == 0x2C else min(a, b))
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
    def run(self, func, args, fargs, scratch, seed=0, setup=None):
        """setup(m), if present, prepares the state by calling ORIGINAL functions with m.call and
        returns the arguments (possibly new) for the function to test."""
        if self.had_trap:
            self._new_uc()
        uc = self.uc
        uc.context_restore(self.clean_ctx)
        uc.mem_write(MEM_BASE, self.pristine)  # identical initial memory for each run
        uc.mem_write(0, b"\0" * 0x1000)        # also page 0 (null pointers write there)
        uc.mem_write(SCRATCH, scratch)
        for a, v in self.globals.items():
            uc.mem_write(a, struct.pack("<I", v))
        uc.mem_write(RET_MAGIC, b"\0" * 8)
        if self.code_patched:  # the previous test had modified the code: out with the cache
            uc.ctl_flush_tb()
            self.code_patched = False
        self.seed = seed
        self.calls = []
        self.k0 = 0
        if setup is not None:
            try:
                args = setup(self) or args
            except UcError:
                return dict(error="preparazione fallita: %s" % self.error, untestable=False)
            self.calls = []  # the system calls of the preparation do not count
            if self.code_patched:  # the preparation modified the code (stop_at)
                uc.ctl_flush_tb()
        self._start(func, args, fargs)
        return dict(
            error=self.error, untestable=self.untestable,
            v0=uc.reg_read(UC_MIPS_REG_V0), v1=uc.reg_read(UC_MIPS_REG_V1),
            f0=uc.reg_read(UC_MIPS_REG_F0), calls=self.calls,
            # final memory, excluding the stack area (local variables may differ)
            mem=(bytes(uc.mem_read(MEM_BASE, STACK_TOP - 0x20000 - MEM_BASE)),
                 bytes(uc.mem_read(STACK_TOP, MEM_BASE + MEM_SIZE - STACK_TOP))))

    def call(self, func, *args):
        """Runs a function (for the preparations) and returns v0; UcError if it fails."""
        self._start(func, list(args), [])
        if self.error:
            raise UcError(0)
        return self.uc.reg_read(UC_MIPS_REG_V0)

    def _start(self, func, args, fargs):
        uc = self.uc
        for i in range(1, 32):
            uc.reg_write(UC_MIPS_REG_0 + i, 0)
        for r, v in zip(ARG_REGS, args):
            uc.reg_write(r, v)
        for r, v in zip(FARG_REGS, fargs):
            uc.reg_write(r, v)
        uc.reg_write(UC_MIPS_REG_SP, STACK_TOP - 0x100)
        uc.reg_write(UC_MIPS_REG_RA, RET_MAGIC)
        uc.reg_write(UC_MIPS_REG_K0, self.k0)  # data of the current thread (0 = none)
        self.count, self.error, self.untestable = 0, None, False
        pc = func
        while pc is not None:
            self.trap_pc = None
            try:
                uc.emu_start(pc, RET_MAGIC)
            except UcError as e:
                self.error = "CPU error: %s at %08X" % (e, uc.reg_read(UC_MIPS_REG_PC))
            if self.trap_pc is None or self.error:
                break
            if os.environ.get("DIFFTEST_TRACE"):
                print("trap: last_pc %08X sp %08X intno %s count %d" % (self.trap_pc, uc.reg_read(UC_MIPS_REG_SP), getattr(self, "intno", "?"), self.count))
            pc = self._resolve_trap(self.trap_pc)
            if pc is not None:
                self._clear_branch_state()

    REGS_TO_KEEP = ([UC_MIPS_REG_0 + i for i in range(1, 32)] + [UC_MIPS_REG_HI, UC_MIPS_REG_LO] +
                    [UC_MIPS_REG_F0 + i for i in range(32)] + [UC_MIPS_REG_FCSR])

    def _clear_branch_state(self):
        """After a stop in the delay slot of a jump, unicorn remembers the "pending" jump and,
        on restart, would run the first instruction twice. We restart from a clean CPU state
        with the same registers."""
        uc = self.uc
        vals = [uc.reg_read(r) for r in self.REGS_TO_KEEP]
        uc.context_restore(self.clean_ctx)
        for r, v in zip(self.REGS_TO_KEEP, vals):
            uc.reg_write(r, v)


SPECIAL_FLOATS = [0x00000000, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC00000, 0x3F800000, 0xBF800000,
                  0x3F000000, 0x40000000, 0x00000001, 0x007FFFFF, 0x00800000, 0x7F7FFFFF, 0x3FC90FDB]


def random_float_bits(rng):
    """Test floats: common values, special values (0, inf, NaN, subnormals) and random exponents."""
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


def make_game_inputs(rng):
    """Inputs for the game functions: the first argument ("self") almost always points to a
    valid area; the test memory contains valid pointers, small numbers and floats;
    the global pointer variables point into the test area."""
    args, fargs, _ = make_inputs(rng)
    if rng.random() < 0.95:
        args[0] = SCRATCH + rng.randrange(0, 0x40, 4)
    words = []
    for _ in range(SCRATCH_SIZE // 4):
        k = rng.random()
        if k < 0.3:
            words.append(0)
        elif k < 0.55:
            words.append(rng.randrange(0, 16))
        elif k < 0.7:
            words.append(SCRATCH + rng.randrange(0, SCRATCH_SIZE - 0x100, 4))
        elif k < 0.85:
            words.append(struct.unpack("<I", struct.pack("<f", rng.uniform(-500, 500)))[0])
        else:
            words.append(rng.getrandbits(32))
    scratch = struct.pack("<%dI" % len(words), *words)
    glob = {}
    for a, big in GAME_GLOBALS.items():
        if big >= 0x100000 or (big and rng.random() < 0.5):
            glob[a] = SCRATCH + rng.randrange(0x6000, 0x7000, 4) - big  # far fields fall into the test area
        else:
            glob[a] = SCRATCH + rng.randrange(0, 0x6000, 4)
    # the game's language (D_08A631A0 + 0xF7A574) is 0..6
    lang = glob[0x08A631A0] + 0xF7A574 - SCRATCH
    scratch = scratch[:lang] + struct.pack("<I", rng.randrange(0, 7)) + scratch[lang + 4:]
    return args, fargs, scratch, glob


def load_return_kinds():
    """From the C, finds what to compare on return: void (nothing), float (f0), 64 bit (v0+v1), other (v0)."""
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
        # not compared: the new code area (private data of the C) and the test bench
        # (0x08800000-0x08804000: cells of the dummy functions, instructions run apart)
        lo, hi = NEWCODE_ADDR - MEM_BASE, NEWCODE_ADDR - MEM_BASE + 0x100000
        ma, mb = a["mem"][0], b["mem"][0]
        if ma[0x4000:lo] != mb[0x4000:lo] or ma[hi:] != mb[hi:] or a["mem"][1] != b["mem"][1]:
            base = [MEM_BASE, STACK_TOP]
            for k in (0, 1):
                x, y = a["mem"][k], b["mem"][k]
                for i in range(0, len(x), 0x1000):
                    if x[i:i + 0x1000] != y[i:i + 0x1000] and not (k == 0 and (lo <= i < hi or i < 0x4000)):
                        j = next(j for j in range(i, i + 0x1000) if x[j] != y[j])
                        return "different memory written (e.g. %08X)" % (base[k] + j)
    if a["calls"] != b["calls"]:
        for x, y in zip(a["calls"] + [None] * 99, b["calls"]):
            if x != y:
                return "different system calls: original %s, C %s" % (x, y)
        return "different system calls (count)"
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
    image, newcode = load_image(), open(os.path.join(BUILD, "newcode.bin"), "rb").read()
    sigs = load_signatures()
    results = {}
    for name in names:
        # new machine for each function: unicorn 2.1.4 sometimes crashes reusing
        # the same one after runs interrupted halfway (stop_at)
        orig, new = origs[name], syms[name]
        game = orig >= GAME_START
        fstubs = make_fstubs(name, syms, origs, sigs) if game else None
        m = Machine(image, newcode, fstubs)
        rng = random.Random(orig)
        ok, skipped, status = 0, 0, None
        for run in range(runs):
            if os.environ.get("DIFFTEST_TRACE"):
                print("prova", run, flush=True)
            if game:
                args, fargs, scratch, m.globals = make_game_inputs(rng)
            else:
                args, fargs, scratch = make_inputs(rng)
            setup = None
            if game:
                narrow_args(args, sigs.get(orig, []))
            if name in SPECS:
                spec = SPECS[name](rng, args, fargs, scratch)
                args, fargs, scratch = spec[:3]
                setup = spec[3] if len(spec) > 3 else None
            a = m.run(orig, args, fargs, scratch, run, setup)
            if a["untestable"]:
                status = "NOT TESTABLE (%s)" % a["error"]; break
            if a["error"]:
                skipped += 1; continue
            b = m.run(new, args, fargs, scratch, run, setup)
            d = b["error"] or compare(a, b, kinds.get(name, ("v0",)))
            if d:
                status = "DIFF: %s  [args %s]" % (d, " ".join("%08X" % x for x in args[:4])); break
            ok += 1
        if status is None:
            status = "OK (%d inputs compared)" % ok if ok else "NOT TESTABLE (all inputs fail)"
        results[name] = status
        print("%-14s %s" % (name, status))
    return results


if __name__ == "__main__":
    main()
