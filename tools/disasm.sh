#!/bin/bash
# Disassembla funzioni originali. Uso (WSL): tools/disasm.sh ADDR [ADDR...]  (indirizzi hex, da functions.tsv)
export PATH=$PATH:/opt/pspdev/bin
cd "$(dirname "$0")/.."
for a in "$@"; do
  a=${a^^}
  size=$(awk -F'\t' -v A="$a" 'toupper($1)==A {print $2}' ../work/functions.tsv)
  start=$((16#$a)); stop=$((start + size))
  echo "=== func_$a ($size byte)"
  psp-objdump -d -z --start-address=$start --stop-address=$stop ../work/original/EBOOT_decrypted.elf | tail -n +7 | sed 's/^ *//' | cut -f1,3-
done
