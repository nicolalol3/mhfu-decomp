"""Helper to understand a difference found by difftest.py: repeats test number N of a
function and prints the recorded calls and the memory bytes that differ.

Usage: tools/.venv/Scripts/python.exe -I tools/difftest_debug.py NAME [N]
(N = number of the test; without N it uses the first one that shows a difference)
"""
import os, random, sys
sys.path.insert(0, os.path.dirname(__file__))
import difftest as D



def main():
    name = sys.argv[1]
    want = int(sys.argv[2]) if len(sys.argv) > 2 else None
    syms, origs = {}, {}
    for line in open(os.path.join(D.BUILD, "replaced.tsv")):
        o, n, a = line.split()
        syms[n], origs[n] = int(a, 16), int(o, 16)
    image, newcode = D.load_image(), open(os.path.join(D.BUILD, "newcode.bin"), "rb").read()
    sigs = D.load_signatures()
    orig, new = origs[name], syms[name]
    game = orig >= D.GAME_START
    m = D.Machine(image, newcode, D.make_fstubs(name, syms, origs, sigs) if game else None)
    kinds = D.load_return_kinds()
    rng = random.Random(orig)
    for run in range(1000):
        if game:
            args, fargs, scratch, m.globals = D.make_game_inputs(rng)
        else:
            args, fargs, scratch = D.make_inputs(rng)
        if game:
            D.narrow_args(args, sigs.get(orig, []))
        setup = None
        if name in D.SPECS:
            spec = D.SPECS[name](rng, args, fargs, scratch)
            args, fargs, scratch = spec[:3]
            setup = spec[3] if len(spec) > 3 else None
        a = m.run(orig, args, fargs, scratch, run, setup)
        if a["error"] or a["untestable"]:
            continue
        b = m.run(new, args, fargs, scratch, run, setup)
        d = b["error"] or D.compare(a, b, kinds.get(name, ("v0",)))
        if (want is None and d) or want == run:
            print("prova", run, "args", " ".join("%08X" % x for x in args), "->", d)
            print("v0 orig %08X  C %08X" % (a["v0"], b["v0"]))
            for i in range(max(len(a["calls"]), len(b["calls"]))):
                x = a["calls"][i] if i < len(a["calls"]) else None
                y = b["calls"][i] if i < len(b["calls"]) else None
                print("  %s %s\n     %s" % ("  " if x == y else "!!", x, y))
            for k, base in ((0, D.MEM_BASE), (1, D.STACK_TOP)):
                x, y = a["mem"][k], b["mem"][k]
                lo, hi = D.NEWCODE_ADDR - D.MEM_BASE, D.NEWCODE_ADDR - D.MEM_BASE + 0x100000
                shown = 0
                for i in range(len(x)):
                    if x[i] != y[i] and not (k == 0 and lo <= i < hi):
                        print("  mem %08X: orig %02X  C %02X" % (base + i, x[i], y[i]))
                        shown += 1
                        if shown > 40:
                            break
            return
    print("no difference found")


if __name__ == "__main__":
    main()
