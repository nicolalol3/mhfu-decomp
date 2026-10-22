#!/bin/bash
# Compila tutto il C in src/ e produce l'EBOOT modificato. Gira dentro Ubuntu (WSL).
# Uso (da Windows): wsl.exe -d Ubuntu -- bash /mnt/c/Users/main/Desktop/FUPC/decomp/tools/build.sh
set -e
export PATH=$PATH:/opt/pspdev/bin
cd "$(dirname "$0")/.."
mkdir -p build/obj
python3 tools/symbols.py
CFLAGS="-I/opt/pspdev/psp/sdk/include -O2 -G0 -march=allegrex -mno-abicalls -fno-pic -mno-branch-likely -fno-builtin -fno-strict-aliasing -ffreestanding -Wall -Wno-unused -Iinclude"
OBJS=()
for c in $(find src -name '*.c' | sort); do
  o=build/obj/$(echo "${c#src/}" | tr / _ | sed 's/\.c$/.o/')
  psp-gcc $CFLAGS -c "$c" -o "$o"
  OBJS+=("$o")
done
if [ ${#OBJS[@]} -eq 0 ]; then echo "Nessun file C"; exit 0; fi
psp-ld -T tools/newcode.ld -o build/newcode.elf "${OBJS[@]}"
psp-objcopy -O binary build/newcode.elf build/newcode.bin
psp-nm build/newcode.elf > build/newcode.nm
python3 tools/inject.py build/newcode.bin build/newcode.nm build/EBOOT.BIN
