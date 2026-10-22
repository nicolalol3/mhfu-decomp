"""Percorsi e costanti condivise dagli strumenti del progetto."""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORK = os.path.join(os.path.dirname(ROOT), "work")
ORIG_ELF = os.path.join(WORK, "original", "EBOOT_decrypted.elf")
FUNCTIONS_TSV = os.path.join(WORK, "functions.tsv")
BUILD = os.path.join(ROOT, "build")
SRC = os.path.join(ROOT, "src")
TEST_DISC_EBOOT = os.path.join(WORK, "disc", "PSP_GAME", "SYSDIR", "EBOOT.BIN")

TEXT_START = 0x08804000
TEXT_SIZE = 1089080           # byte di .text: denominatore della percentuale
NEWCODE_ADDR = 0x09D65500     # subito dopo la memoria del modulo originale (0x09D65480)


def load_functions():
    """Ritorna {indirizzo: dimensione} delle funzioni trovate da Ghidra."""
    funcs = {}
    with open(FUNCTIONS_TSV) as f:
        next(f)
        for line in f:
            addr, size = line.split("\t")[:2]
            funcs[int(addr, 16)] = int(size)
    return funcs


def load_symbols():
    """Ritorna {nome: indirizzo} da symbols.txt."""
    out = {}
    for line in open(os.path.join(ROOT, "symbols.txt")):
        line = line.split("#")[0].split()
        if len(line) == 2:
            out[line[1]] = int(line[0], 16)
    return out
