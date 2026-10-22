#!/bin/bash
# Estrae da Ghidra il C grezzo delle funzioni indicate in build/ghidra_c/<indirizzo>.c
# Uso: tools/getc.sh 0880A000+0880A100+...
cd "$(dirname "$0")/.."
./tools/ghidra.sh -process EBOOT_decrypted.elf -noanalysis -readOnly \
  -postScript DecompileList.java "C:/Users/main/Desktop/FUPC/decomp/build/ghidra_c" "$1" 2>&1 | grep -E "ERROR|Nessuna" 
ls build/ghidra_c | wc -l
