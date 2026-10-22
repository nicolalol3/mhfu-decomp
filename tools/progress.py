"""Decompilation percentage: bytes of the original functions replaced by C / bytes of .text.
Based on the last build (build/replaced.tsv written by inject.py)."""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from config import BUILD, TEXT_SIZE, load_functions

funcs = load_functions()
done = [int(l.split("\t")[0], 16) for l in open(os.path.join(BUILD, "replaced.tsv"))]
nbytes = sum(funcs[a] for a in done)
print("Functions replaced by C: %d / %d" % (len(done), len(funcs)))
print("Byte: %d / %d  ->  %.3f%%" % (nbytes, TEXT_SIZE, 100.0 * nbytes / TEXT_SIZE))
