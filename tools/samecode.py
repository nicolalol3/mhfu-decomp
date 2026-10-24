"""Checks that a cleanup (renames, macros, structures) did not change the compiled code.
Each C file is compiled twice, as it is in a git revision (default HEAD) and as it is now, and
the functions are compared one by one; the symbols of the calls are compared by address
(func_XXXXXXXX, D_XXXXXXXX or the address in symbols.txt), so renames do not count.
Usage:  tools/.venv/Scripts/python.exe -I tools/samecode.py [--rev REV] [--diff] [src/x/y.c ...]
        (no files: all the C files changed since REV; --diff prints the changed instructions)"""
import difflib, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_SH = os.path.join(ROOT, "tools", "build.sh")


def git(*args):
    return subprocess.run(["git", "-C", ROOT] + list(args), capture_output=True, text=True,
                          encoding="utf-8").stdout


def wsl_path(p):
    p = os.path.abspath(p).replace("\\", "/")
    return "/mnt/" + p[0].lower() + p[2:]


def cflags():
    m = re.search(r'^CFLAGS="([^"]*)"', open(BUILD_SH).read(), re.M)
    return m.group(1).replace("-Wall", "-w").replace("-Iinclude", "")


def symbol_addresses(rev):
    addr = {}
    for text in (open(os.path.join(ROOT, "symbols.txt"), encoding="utf-8").read(),
                 git("show", rev + ":symbols.txt")):
        for l in text.splitlines():
            l = l.split("#")[0].split()
            if len(l) == 2:
                addr.setdefault(l[1], int(l[0], 16))
    return addr


def parse(dump, norm):
    funcs, cur = {}, None
    for l in dump.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+)>:$", l)
        if m:
            cur = norm(m.group(1))
            funcs[cur] = []
            continue
        if cur is None:
            continue
        m = re.match(r"^\s+[0-9a-f]+:\s+([0-9a-f]{8})\s+(.*)$", l)
        if m:
            funcs[cur].append([m.group(1), m.group(2).split("<")[0].strip(), ""])
            continue
        m = re.match(r"^\s+[0-9a-f]+: (R_MIPS_\w+)\s+([^+\s]+)(\+0x[0-9a-f]+)?", l)
        if m and funcs[cur]:
            funcs[cur][-1][2] = m.group(1) + " " + norm(m.group(2)) + (m.group(3) or "")
    return funcs


def key(ins):
    # with a relocation the encoded field is filled by the linker: compare the opcode only
    return (ins[1].split()[0] if ins[2] else ins[0]) + " " + ins[2]


def main():
    argv = sys.argv[1:]
    rev, show = "HEAD", "--diff" in argv
    if "--rev" in argv:
        i = argv.index("--rev"); rev = argv[i + 1]; del argv[i:i + 2]
    files = [a for a in argv if a.endswith(".c")]
    if not files:
        files = [f for f in git("diff", "--name-only", rev, "--", "src").split() if f.endswith(".c")]
    addr = symbol_addresses(rev)
    names = {}
    for l in open(os.path.join(ROOT, "symbols.txt"), encoding="utf-8"):
        l = l.split("#")[0].split()
        if len(l) == 2:
            names["@%08X" % int(l[0], 16)] = l[1]

    def norm(sym):
        m = re.fullmatch(r"(?:func|D)_([0-9A-F]{8})", sym)
        if m:
            return "@" + m.group(1)
        return "@%08X" % addr[sym] if sym in addr else sym

    tmp = tempfile.mkdtemp(dir=os.path.join(ROOT, "build"))
    try:
        inc = os.path.join(tmp, "include")
        os.makedirs(inc)
        for h in git("ls-tree", "--name-only", rev, "include/").split():
            open(os.path.join(inc, os.path.basename(h)), "w", encoding="utf-8").write(git("show", rev + ":" + h))
        total = 0
        for f in files:
            old = os.path.join(tmp, "old.c")
            text = git("show", rev + ":" + f)
            if not text:
                print("%s: new file" % f)
                continue
            open(old, "w", encoding="utf-8").write(text)
            cmd = ("export PATH=/usr/bin:/bin:/opt/pspdev/bin; cd %s; CF='%s'; "
                   "psp-gcc $CF -I%s -c %s -o %s/old.o && psp-gcc $CF -Iinclude -c %s -o %s/new.o && "
                   "psp-objdump -dr %s/old.o > %s/old.txt && psp-objdump -dr %s/new.o > %s/new.txt"
                   % (wsl_path(ROOT), cflags(), wsl_path(inc), wsl_path(old), wsl_path(tmp), f,
                      wsl_path(tmp), wsl_path(tmp), wsl_path(tmp), wsl_path(tmp), wsl_path(tmp)))
            sh = os.path.join(tmp, "cc.sh")
            open(sh, "w", newline="\n").write(cmd + "\n")
            r = subprocess.run(["wsl.exe", "-d", "Ubuntu", "--", "bash", wsl_path(sh)], capture_output=True,
                               text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
            if r.returncode != 0:
                print("%s: compile error\n%s" % (f, r.stderr[-2000:]))
                total += 1
                continue
            a = parse(open(os.path.join(tmp, "old.txt")).read(), norm)
            b = parse(open(os.path.join(tmp, "new.txt")).read(), norm)
            bad = []
            for name in sorted(set(a) | set(b)):
                if name not in a or name not in b:
                    bad.append((name, "only in the %s version" % ("old" if name in a else "new")))
                elif [key(i) for i in a[name]] != [key(i) for i in b[name]]:
                    bad.append((name, "%d -> %d instructions" % (len(a[name]), len(b[name]))))
                    if show:
                        for d in difflib.unified_diff(["%s %s" % (i[1], i[2]) for i in a[name]],
                                                      ["%s %s" % (i[1], i[2]) for i in b[name]],
                                                      lineterm="", n=2):
                            print("    " + d)
            print("%s: %s" % (f, "same code" if not bad else "%d functions changed" % len(bad)))
            for name, why in bad:
                print("    %-32s %s" % (names.get(name, name), why))
            total += len(bad)
        sys.exit(1 if total else 0)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


main()
