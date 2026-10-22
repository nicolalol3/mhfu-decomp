#!/bin/bash
# Compila tutto il C in src/ e produce l'EBOOT modificato. Gira dentro Ubuntu (WSL).
# Uso (da Windows): wsl.exe -d Ubuntu -- bash /mnt/c/Users/main/Desktop/FUPC/decomp/tools/build.sh
set -e
export PATH=$PATH:/opt/pspdev/bin
cd "$(dirname "$0")/.."
mkdir -p build/obj
python3 tools/symbols.py
# niente inlining/fusione fra funzioni: ogni funzione del gioco resta una funzione a sé (serve ai test e alle mod)
CFLAGS="-I/opt/pspdev/psp/sdk/include -O2 -G0 -march=allegrex -mno-abicalls -fno-pic -mno-branch-likely -fno-builtin -fno-strict-aliasing -fno-tree-loop-distribute-patterns -fno-inline-small-functions -fno-inline-functions-called-once -fno-inline-functions -fno-ipa-icf -fno-ipa-cp -fno-ipa-sra -fno-ipa-vrp -fno-ipa-bit-cp -fno-ipa-pure-const -fno-ipa-reference -fno-ipa-ra -fno-ipa-modref -ffreestanding -Wall -Wno-unused -Iinclude"
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
