# Decompilation of Monster Hunter Freedom Unite (PSP, EU, ULES01213)

Goal: rewrite the game's code in C, function by function, so that it is easy to modify.
The compiled code does not need to be byte-for-byte identical to the original: it only has
to **behave** the same way.

## How it works

1. The original game (decrypted EBOOT) remains the base.
2. Each function rewritten in C (`src/`) is compiled and loaded into memory right after the
   game; at the start of the original function, a jump to the C version is placed.
3. So the game runs with a mix of original code and C code, and the C part grows little by
   little.

## Folders

| Path | Contents |
|---|---|
| `src/` | the decompiled C code (`libc/` = C library, `libm/` = math) |
| `include/` | shared headers (`quest.h`: the quest object and the format of the quest files; `game.h`: fields of the general game state) |
| `symbols.txt` | readable names of game functions and variables (address → name) |
| `tools/` | tools: build, injection, tests |
| `build/` | generated files (modified EBOOT, test screenshots, ...) |
| `../work/` | Ghidra analysis, decrypted original EBOOT, extracted disc for the tests |

## Checking each function

- **Test bench** (`tools/difftest.py`): runs the original function and the C version on an
  emulated MIPS CPU with hundreds of random inputs (including extreme values), and compares
  the result, the memory written and the calls to the operating system.
- **Boot test** (`tools/boottest.sh`): starts the game in PPSSPP with the modified EBOOT and
  saves a screenshot to `build/screenshot.png`.
- **Same code** (`tools/samecode.py`): after a cleanup that should not change anything
  (renames, macros), checks that the compiled code is identical to a saved reference.

## Readability

The code is written first with raw offsets (`U32(self, 0x28)`), then, once a part of the game
is complete and tested, it is cleaned up: structures with named fields, names instead of
`func_XXXXXXXX` / `D_XXXXXXXX`, named constants. Done so far: the quest system
(`src/game/quest*.c`, `include/quest.h`). With structures GCC can keep fields in registers, so
the compiled code changes a little: the cleaned functions are checked again with the test bench.

## Commands (run by Claude)

```
wsl.exe -d Ubuntu -- bash tools/build.sh                 # compiles and creates build/EBOOT.BIN
tools/.venv/Scripts/python.exe -I tools/difftest.py      # test bench
tools/boottest.sh 25                                     # boot test in PPSSPP
python tools/progress.py                                 # percentage completed
```

## Status

See `python tools/progress.py`. The percentage refers only to the EBOOT (1,089,080 bytes of
code); the game loads other code (overlays) from `DATA.BIN`, which is not counted yet.
