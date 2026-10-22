"""Runs difftest.py on all functions (or the listed ones), one per process, so that a crash
of the emulator on one function does not stop the whole run. Writes the summary to build/difftest_all.txt.

Usage: tools/.venv/Scripts/python.exe -I tools/difftest_all.py [names...] [--runs N] [--jobs J]
"""
import os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(os.path.dirname(HERE), "build")


def run_one(name, runs):
    try:
        p = subprocess.run([sys.executable, "-I", os.path.join(HERE, "difftest.py"), name, "--runs", str(runs)],
                           capture_output=True, text=True, timeout=900)
        out = [l for l in p.stdout.splitlines() if l.startswith(name + " ") or l.startswith(name + "\t")]
        if out:
            return out[-1][len(name):].strip()
        tail = (p.stderr.strip().splitlines() or ["?"])[-1]
        return "CRASH DELL'EMULATORE (%s)" % tail[:120]
    except subprocess.TimeoutExpired:
        return "TEMPO SCADUTO"


def main():
    argv = sys.argv[1:]
    runs, jobs = 100, 4
    if "--runs" in argv:
        i = argv.index("--runs"); runs = int(argv[i + 1]); del argv[i:i + 2]
    if "--jobs" in argv:
        i = argv.index("--jobs"); jobs = int(argv[i + 1]); del argv[i:i + 2]
    names = argv or sorted(l.split()[1] for l in open(os.path.join(BUILD, "replaced.tsv")))
    with ThreadPoolExecutor(jobs) as ex:
        results = list(ex.map(lambda n: (n, run_one(n, runs)), names))
    lines = ["%-28s %s" % r for r in results]
    if not argv:
        open(os.path.join(BUILD, "difftest_all.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
    for l in lines:
        print(l)
    cnt = {}
    for _, s in results:
        k = s.split()[0]
        cnt[k] = cnt.get(k, 0) + 1
    print("Summary:", ", ".join("%s %d" % kv for kv in sorted(cnt.items())))


if __name__ == "__main__":
    main()
