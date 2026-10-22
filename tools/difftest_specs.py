"""Input di prova su misura per le funzioni che accettano solo argomenti con vincoli precisi.
Ogni funzione riceve un generatore casuale e gli input generici (args, fargs, scratch)
e li adatta; i nomi sono quelli delle funzioni in C."""
import struct

SCRATCH = 0x09E00000
TWO_OVER_PI = 0x0890FBB4  # tabella originale delle cifre di 2/pi


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


SPECS = {
    "__kernel_rem_pio2f": kernel_rem_pio2f,
}
