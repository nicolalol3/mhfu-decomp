"""Input di prova su misura per le funzioni che accettano solo argomenti con vincoli precisi.
Ogni funzione riceve un generatore casuale e gli input generici (args, fargs, scratch)
e li adatta; i nomi sono quelli delle funzioni in C.
Una spec può ritornare anche una funzione setup(m): viene eseguita prima della prova (sia
per l'originale sia per il C) e prepara lo stato chiamando funzioni ORIGINALI con
m.call(indirizzo, argomenti...); ritorna gli argomenti definitivi."""
import os, random, struct

SCRATCH = 0x09E00000
TWO_OVER_PI = 0x0890FBB4  # tabella originale delle cifre di 2/pi
RET_MAGIC = 0x08800010    # indirizzo dove il banco di prova si ferma

# funzioni originali usate nelle preparazioni
O_MALLOC_R, O_FREE_R, O_REALLOC_R = 0x0880B70C, 0x0880AA40, 0x0880DB74
O_ATEXIT, O_EXIT_SYS = 0x08808ACC, 0x088041C8
O_ABS, O_ISATTY, O_LOCALECONV_R = 0x08808AC0, 0x0880455C, 0x0880B42C
O_SSEEK, O_SWRITE = 0x0880EA8C, 0x0880EA08
G_SBRK_CUR, G_SBRK_START, G_SBRK_END = 0x089CBA90, 0x089CBA94, 0x089CBA98
IMPURE = 0x089332C8       # struct _reent del modulo principale
HEAP_LO, HEAP_HI = 0x09F00000, 0x09F80000
REENT = SCRATCH + 0x7000  # struct _reent finta per i parametri "r"


def newcode_sym(name):
    """Indirizzo di un simbolo del C ricompilato (da build/newcode.nm)."""
    p = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "newcode.nm")
    for line in open(p):
        f = line.split()
        if len(f) == 3 and f[2] == name:
            return int(f[0], 16)
    return None


def w32(m, addr, *vals):
    m.uc.mem_write(addr, struct.pack("<%dI" % len(vals), *vals))


def stop_at(m, addr):
    """Sostituisce la funzione all'indirizzo con un salto al punto di arresto.
    (run() svuota poi la cache del codice tradotto di unicorn, altrimenti potrebbe
    continuare a eseguire la versione vecchia; fermarsi con emu_stop da un hook invece
    manda unicorn 2.1.4 in crash alla prova successiva.)"""
    w32(m, addr, 0x08000000 | ((RET_MAGIC >> 2) & 0x03FFFFFF), 0)
    m.code_patched = True


def kernel_rem_pio2f(rng, args, fargs, scratch):
    # x[]: nx float interi in [0, 255] (il primo non nullo), come li prepara __ieee754_rem_pio2f
    tx = [float(rng.randrange(1, 256))] + [float(rng.randrange(256)) for _ in range(2)]
    scratch = bytearray(scratch)
    scratch[0:12] = struct.pack("<3f", *tx)
    args[0] = SCRATCH          # x
    args[1] = SCRATCH + 0x100  # y
    args[2] = rng.randrange(0, 121)  # e0
    args[3] = rng.randrange(1, 4)    # nx
    args[4] = rng.randrange(0, 4)    # prec
    args[5] = TWO_OVER_PI
    return args, fargs, bytes(scratch)


# --- heap ---------------------------------------------------------------------------

def rand_size(r):
    k = r.random()
    if k < 0.5:
        return r.randrange(0, 600)
    if k < 0.85:
        return r.randrange(0, 6000)
    if k < 0.97:
        return r.randrange(0, 0x30000)
    return r.choice([0xFFFFFFFF, 0xFFFFFFF0, 0x7FFFFFF0, 0x80000000, r.getrandbits(32)])


def heap_ops(m, r, nops):
    """Prepara un heap realistico: sbrk su una zona libera e una sequenza casuale di
    malloc/free/realloc originali. Ritorna i puntatori ancora allocati."""
    w32(m, G_SBRK_CUR, HEAP_LO, HEAP_LO, HEAP_HI)
    live = []
    for _ in range(nops):
        k = r.random()
        if k < 0.55 or not live:
            p = m.call(O_MALLOC_R, REENT, rand_size(r))
            if p:
                live.append(p)
        elif k < 0.85:
            m.call(O_FREE_R, REENT, live.pop(r.randrange(len(live))))
        else:
            i = r.randrange(len(live))
            p = m.call(O_REALLOC_R, REENT, live[i], rand_size(r))
            if p:
                live[i] = p
    return live


def heap_spec(make_args):
    def spec(rng, args, fargs, scratch):
        seed, nops = rng.getrandbits(32), rng.choice([0, 1, 3, 10, 30, 80])

        def setup(m):
            r = random.Random(seed)
            live = heap_ops(m, r, nops)
            return make_args(r, live)
        return args, fargs, scratch, setup
    return spec


def pick(r, live):
    return r.choice(live) if live and r.random() < 0.95 else 0


SPECS_HEAP = {
    "_malloc_r": heap_spec(lambda r, live: [REENT, rand_size(r)]),
    "_free_r": heap_spec(lambda r, live: [REENT, pick(r, live)]),
    "free": heap_spec(lambda r, live: [pick(r, live)]),
    "_realloc_r": heap_spec(lambda r, live: [REENT, pick(r, live), rand_size(r)]),
    "_calloc_r": heap_spec(lambda r, live: [REENT, r.randrange(0, 40), r.randrange(0, 200)]),
    "_malloc_trim_r": heap_spec(lambda r, live: [REENT, r.choice([0, 16, 4096, r.randrange(0, 0x40000)])]),
    "__sfmoreglue": heap_spec(lambda r, live: [REENT, r.randrange(0, 6)]),
}


# --- memoria e stringhe -------------------------------------------------------------

def mem_spec(nptr, with_c):
    def spec(rng, args, fargs, scratch):
        base = SCRATCH + 0x1000
        a = base + rng.randrange(0, 0x800)
        b = a + rng.randrange(-40, 40) if rng.random() < 0.4 else base + rng.randrange(0, 0x800)
        n = rng.choice([0, 1, 3, 4, 7, 8, 15, 16, 17, 31, 32, 33]) if rng.random() < 0.4 else rng.randrange(0, 1500)
        c = rng.choice([0, 0xFF, rng.getrandbits(8), rng.getrandbits(32)])
        scratch = bytearray(scratch)
        if rng.random() < 0.5:  # zone uguali, per memcmp
            scratch[b - SCRATCH:b - SCRATCH + n] = scratch[a - SCRATCH:a - SCRATCH + n]
        if with_c and rng.random() < 0.5 and n:  # il byte cercato da memchr
            scratch[a - SCRATCH + rng.randrange(n)] = c & 0xFF
        if nptr == 2:
            args[:3] = [a, b, n]
        else:
            args[:3] = [a, c, n]
        return args, fargs, bytes(scratch)
    return spec


# --- stdio ---------------------------------------------------------------------------

FP = SCRATCH + 0x2000


def make_file(rng, scratch, addr, write_fn=O_SWRITE):
    """FILE plausibile: flag, descrittore e buffer dentro la zona scratch."""
    base = SCRATCH + 0x3000 + rng.randrange(0, 0x400)
    flags = rng.choice([0, 4, 8, 9, 10, 0x18, 0x88, 0x808, rng.getrandbits(16)])
    f = struct.pack("<IiihhIiiIIIII", base + rng.randrange(0, 300), 0, 0, flags - (flags >> 15 << 16),
                    rng.choice([0, 1, 2, 5, -1]), base if rng.random() < 0.9 else 0, rng.randrange(0, 1024), 0,
                    addr, 0x0880E978, write_fn, O_SSEEK if rng.random() < 0.7 else 0, 0x0880EB20)
    scratch[addr - SCRATCH:addr - SCRATCH + len(f)] = f


def fflush_spec(rng, args, fargs, scratch):
    scratch = bytearray(scratch)
    make_file(rng, scratch, FP)
    args[0] = FP if rng.random() < 0.9 else 0
    return args, fargs, bytes(scratch)


def smakebuf_spec(rng, args, fargs, scratch):
    scratch = bytearray(scratch)
    make_file(rng, scratch, FP)
    seed = rng.getrandbits(32)

    def setup(m):
        heap_ops(m, random.Random(seed), 5)
        return [FP]
    return args, fargs, bytes(scratch), setup


def sinit_spec(rng, args, fargs, scratch):
    s = SCRATCH + 0x1000
    scratch = bytearray(scratch)
    o = s - SCRATCH
    scratch[o + 4:o + 16] = struct.pack("<3I", s + 0x268, s + 0x268 + 0x5C, s + 0x268 + 0xB8)
    args[0] = s
    return args, fargs, bytes(scratch)


def fwalk_spec(rng, args, fargs, scratch):
    s = SCRATCH + 0x1000
    scratch = bytearray(scratch)
    o = s - SCRATCH
    # due blocchi di FILE concatenati
    g2 = SCRATCH + 0x1800
    scratch[o + 0x25C:o + 0x268] = struct.pack("<IiI", g2 if rng.random() < 0.5 else 0, rng.randrange(0, 4), s + 0x268)
    scratch[g2 - SCRATCH:g2 - SCRATCH + 12] = struct.pack("<IiI", 0, rng.randrange(0, 4), SCRATCH + 0x4000)
    for i in range(8):  # metà dei FILE in uso
        for fbase in (s + 0x268, SCRATCH + 0x4000):
            fo = fbase - SCRATCH + i * 0x5C + 12
            scratch[fo:fo + 2] = struct.pack("<h", rng.choice([0, 0, 1, -1, rng.getrandbits(15)]))
    args[0] = s if rng.random() < 0.9 else IMPURE
    args[1] = rng.choice([O_ABS, O_ISATTY, O_LOCALECONV_R])
    return args, fargs, bytes(scratch)


# --- stdlib e locale -----------------------------------------------------------------

def atexit_spec(rng, args, fargs, scratch):
    seed, n = rng.getrandbits(32), rng.choice([0, 1, 5, 31, 32, 33, 40])

    def setup(m):
        r = random.Random(seed)
        heap_ops(m, r, 3)
        for _ in range(n):
            m.call(O_ATEXIT, r.choice([O_ABS, O_ISATTY]))
        return [r.choice([O_ABS, O_ISATTY, r.getrandbits(32)])]
    return args, fargs, scratch, setup


def exit_spec(rng, args, fargs, scratch):
    """exit/abort: le funzioni registrate vengono chiamate, poi ci si ferma su _exit."""
    seed, n = rng.getrandbits(32), rng.choice([0, 1, 3, 31, 32, 34])
    code = args[0]

    def setup(m):
        r = random.Random(seed)
        heap_ops(m, r, 3)
        for _ in range(n):
            # (non abs: la sua istruzione max emulata dentro una funzione chiamata da exit
            # fa sbagliare unicorn)
            m.call(O_ATEXIT, r.choice([O_ISATTY, O_LOCALECONV_R]))
        if n:  # alcune come on_exit (con argomento)
            p = struct.unpack("<I", m.uc.mem_read(IMPURE + 0x148, 4))[0]
            w32(m, p + 0x108, r.getrandbits(32))
        stop_at(m, O_EXIT_SYS)
        stop_at(m, newcode_sym("_exit"))
        return [code]
    return args, fargs, scratch, setup


def setlocale_spec(rng, args, fargs, scratch, reent=True):
    scratch = bytearray(scratch)
    loc = SCRATCH + 0x100
    scratch[0x100:0x103] = rng.choice([b"C\0\0", b"\0\0\0", b"CC\0", b"c\0\0", bytes(scratch[0x100:0x103])])
    i = 2 if reent else 1
    args[i] = loc if rng.random() < 0.8 else 0
    return args, fargs, bytes(scratch)


MODREENT_TABLE = 0x089CBADC


def reent_spec(rng, args, fargs, scratch):
    """k0 -> dati del thread -> (byte 4) thread -> (0x37C) id modulo; tabella modulo->reent."""
    seed = rng.getrandbits(32)

    def setup(m):
        r = random.Random(seed)
        td, thr, t2 = SCRATCH + 0x5000, SCRATCH + 0x5100, SCRATCH + 0x5800
        if r.random() < 0.9:
            m.k0 = td
            w32(m, td + 4, thr if r.random() < 0.9 else 0)
        modid = r.choice([r.randrange(0, 200), -r.randrange(1, 200), r.getrandbits(31)])
        w32(m, thr + 0x37C, modid & 0xFFFFFFFF)
        h = modid - int(modid / 32) * 32  # resto col segno, come il % del C
        tables = [MODREENT_TABLE] + ([t2] if r.random() < 0.5 else [])
        w32(m, MODREENT_TABLE, tables[1] if len(tables) > 1 else 0)
        if len(tables) > 1:
            w32(m, t2, 0)
        where = r.choice(tables + [None])
        if where is not None and -32 < h < 32:
            w32(m, where + 4 + 4 * h, r.getrandbits(32))
            w32(m, where + 4 + 128 + 4 * h, modid & 0xFFFFFFFF)
        return args
    return args, fargs, scratch, setup


def raise_spec(with_reent):
    """struct _reent con la tabella dei gestori assente (va creata con malloc) o presente,
    con gestori predefinito/ignora/errore o una funzione innocua."""
    def spec(rng, args, fargs, scratch):
        seed = rng.getrandbits(32)
        sig = rng.choice([0, 6, 31, 32, -1, rng.randrange(0, 32)])

        def setup(m):
            r = random.Random(seed)
            heap_ops(m, r, 3)
            reent = REENT if with_reent else IMPURE
            table = SCRATCH + 0x6000
            w32(m, reent + 0x258, table if r.random() < 0.6 else 0)
            for i in range(32):
                w32(m, table + 4 * i, r.choice([0, 1, 0xFFFFFFFF, O_ABS, O_ISATTY]))
            return [reent, sig & 0xFFFFFFFF] if with_reent else [sig & 0xFFFFFFFF]
        return args, fargs, scratch, setup
    return spec


SPECS = {
    "__kernel_rem_pio2f": kernel_rem_pio2f,
    "_raise_r": raise_spec(True),
    "raise": raise_spec(False),
    "_init_signal_r": raise_spec(True),
    "__getreent": reent_spec,
    "_getmodreent": reent_spec,
    "__errno": reent_spec,
    "memchr": mem_spec(1, True),
    "memcmp": mem_spec(2, False),
    "memcpy": mem_spec(2, False),
    "memmove": mem_spec(2, False),
    "memset": mem_spec(1, False),
    "fflush": fflush_spec,
    "__smakebuf": smakebuf_spec,
    "__sinit": sinit_spec,
    "_fwalk": fwalk_spec,
    "atexit": atexit_spec,
    "exit": exit_spec,
    "abort": exit_spec,
    "_setlocale_r": setlocale_spec,
    "setlocale": lambda rng, a, f, s: setlocale_spec(rng, a, f, s, reent=False),
}
SPECS.update(SPECS_HEAP)


# --- numeri grandi (Bigint, usati da printf per i numeri con la virgola) ----------------

O_BALLOC = 0x0880D8C4


def reent_zero(m):
    """struct _reent finta azzerata (liste dei Bigint liberi, risultati di dtoa)."""
    m.uc.mem_write(REENT, b"\0" * 0x300)


def make_bigint(m, r, k=None):
    """Bigint vero creato con il Balloc originale e riempito a caso."""
    k = r.randrange(0, 4) if k is None else k
    b = m.call(O_BALLOC, REENT, k)
    maxwds = 1 << k
    wds = r.randrange(1, maxwds + 1)
    words = [r.choice([0, 1, 0xFFFFFFFF, r.getrandbits(32)]) for _ in range(wds)]
    if words[-1] == 0:
        words[-1] = r.getrandbits(32) | 1  # la cifra più alta non è zero
    w32(m, b + 12, r.choice([0, 0, 1]), wds, *words)
    return b


def bigint_spec(make_args):
    def spec(rng, args, fargs, scratch):
        seed = rng.getrandbits(32)

        def setup(m):
            r = random.Random(seed)
            heap_ops(m, r, 2)
            reent_zero(m)
            return make_args(m, r)
        return args, fargs, scratch, setup
    return spec


def rand_double_words(r):
    """double come coppia di parole (bassa, alta): valori comuni, estremi e speciali."""
    k = r.random()
    if k < 0.6:
        v = r.choice([r.uniform(-1e6, 1e6), r.uniform(-1, 1), float(r.randrange(-1000, 1000)),
                      r.uniform(-1e300, 1e300), 10.0 ** r.randrange(-300, 300)])
        hi_lo = struct.unpack("<II", struct.pack("<d", v))
        return hi_lo[0], hi_lo[1]
    return r.getrandbits(32), r.choice([0, 0x80000000, 0x7FF00000, 0x00100000, r.getrandbits(32)])


OUT_A, OUT_B = SCRATCH + 0x7800, SCRATCH + 0x7810


def _d2b_args(m, r):
    lo, hi = rand_double_words(r)
    return [REENT, 0, lo, hi, OUT_A, OUT_B]


def _dtoa_args(m, r):
    lo, hi = rand_double_words(r)
    return [REENT, 0, lo, hi, r.choice([0, 1, 2, 3, 4, 5]), r.choice([0, 1, 3, 6, 17, 20, -1 & 0xFFFFFFFF]),
            OUT_A, OUT_B]


SPECS.update({
    "mult": bigint_spec(lambda m, r: [REENT, make_bigint(m, r), make_bigint(m, r)]),
    "lshift": bigint_spec(lambda m, r: [REENT, make_bigint(m, r), r.randrange(0, 100)]),
    "b2d": bigint_spec(lambda m, r: [make_bigint(m, r), OUT_A]),
    "d2b": bigint_spec(_d2b_args),
    "_dtoa_r": bigint_spec(_dtoa_args),
})


# --- printf e simili -------------------------------------------------------------------

FORMATS = [b"%d", b"%5d|%-5d|%05d", b"%x %X %o", b"%s", b"%10.3s|", b"%c%c", b"%%", b"%u", b"%+d % d",
           b"%ld %hd", b"%f", b"%.2f", b"%e", b"%g", b"%10.4e|%-12g|", b"%#x %#o", b"%p",
           b"Vita: %d/%d", b"%3d%%", b"testo senza formato", b"%*d", b"%.*f", b"%s e %s", b"%08.3f"]
FMT, VA, STR, BUF = SCRATCH + 0x100, SCRATCH + 0x200, SCRATCH + 0x300, SCRATCH + 0x1000
O_SWRITE_FN = 0x0880EA08


def printf_spec(kind):
    """kind: 'v' = vfprintf(fp, fmt, ap), 'r' = _vfprintf_r(reent, fp, fmt, ap), 'sb' = __sbprintf."""
    def spec(rng, args, fargs, scratch):
        scratch = bytearray(scratch)
        fmt = rng.choice(FORMATS) + rng.choice([b"", b" fine", b"\n"])
        scratch[0x100:0x100 + len(fmt) + 1] = fmt + b"\0"
        scratch[0x300:0x300 + 9] = b"Rathalos\0"
        # lista degli argomenti: interi piccoli, la stringa, double allineati a 8
        words = []
        for _ in range(8):
            c = rng.random()
            if c < 0.4:
                words += [rng.choice([0, 1, 7, 42, 255, 1000, 0xFFFFFFFF, rng.getrandbits(32)]), STR]
            else:
                v = rng.choice([0.0, 1.5, -2.25, 3.14159, 1e10, -1e-5, rng.uniform(-1e5, 1e5)])
                words += list(struct.unpack("<II", struct.pack("<d", v)))
        scratch[0x200:0x200 + 4 * len(words)] = struct.pack("<%dI" % len(words), *words)
        unbuf = kind == "sb" or rng.random() < 0.3
        if unbuf:  # file senza buffer: passa per __sbprintf e __swrite
            f = struct.pack("<IiihhIiiIIIII", 0, 0, 0, 0x000A, 1, 0, 0, 0, FP, 0x0880E978, O_SWRITE_FN, O_SSEEK, 0x0880EB20)
        else:      # file su stringa, come sprintf
            f = struct.pack("<IiihhIiiIIIII", BUF, 0, 0x7FFFFFFF, 0x0208, -1, BUF, 0x7FFFFFFF, 0, FP, 0, 0, 0, 0)
        scratch[FP - SCRATCH:FP - SCRATCH + len(f)] = f
        seed = rng.getrandbits(32)

        def setup(m):
            heap_ops(m, random.Random(seed), 2)
            if kind == "r":
                return [IMPURE, FP, FMT, VA]
            return [FP, FMT, VA]
        return args, fargs, bytes(scratch), setup
    return spec


SPECS.update({
    "vfprintf": printf_spec("v"),
    "_vfprintf_r": printf_spec("r"),
    "__sbprintf": printf_spec("sb"),
})


# --- funzioni del gioco con argomenti da limitare --------------------------------------

def stars_spec(rng, args, fargs, scratch):
    args[1] = rng.randrange(0, 17)   # numero di stelle (il buffer del gioco ne contiene ~20)
    return args, fargs, scratch


def resist_spec(rng, args, fargs, scratch):
    args[3] = rng.randrange(0, 4)     # pagina
    args[5] = rng.randrange(0, 10)    # righe per pagina
    args[6] = rng.randrange(7, 0x80)  # nessuna voce evidenziata (l'evidenziazione usa la VFPU)
    return args, fargs, scratch


SPECS.update({
    "menu_draw_stars": stars_spec,
    "menu_draw_resist_table": resist_spec,
})


def _quorem_args(m, r):
    """quorem(b, S): S con la cifra alta non nulla e b con al massimo le stesse parole di S."""
    S = make_bigint(m, r)
    sw = struct.unpack("<I", bytes(m.uc.mem_read(S + 16, 4)))[0]
    b = make_bigint(m, r, 0 if sw == 1 else None)
    bw = struct.unpack("<I", bytes(m.uc.mem_read(b + 16, 4)))[0]
    if bw > sw:
        w32(m, b + 16, sw)
    return [b, S]


SPECS.update({
    "multadd": bigint_spec(lambda m, r: [REENT, make_bigint(m, r), r.choice([2, 5, 10, r.randrange(2, 100)]),
                                         r.randrange(0, 10)]),
    "pow5mult": bigint_spec(lambda m, r: [REENT, make_bigint(m, r), r.randrange(0, 60)]),
    "i2b": bigint_spec(lambda m, r: [REENT, r.choice([0, 1, 625, r.getrandbits(32)])]),
    "cmp": bigint_spec(lambda m, r: [make_bigint(m, r), make_bigint(m, r)]),
    "diff": bigint_spec(lambda m, r: [REENT, make_bigint(m, r), make_bigint(m, r)]),
    "quorem": bigint_spec(_quorem_args),
})
