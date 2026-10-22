#!/bin/bash
# Disassembla una funzione del C ricompilato. Uso (WSL): tools/cdisasm.sh NOME
export PATH=/usr/bin:/bin:/opt/pspdev/bin
cd "$(dirname "$0")/.."
a=$(awk -v n="$1" '$3==n {print $1}' build/newcode.nm)
next=$(awk '{print $1}' build/newcode.nm | sort | awk -v a="$a" '$1>a {print; exit}')
psp-objdump -d -z build/newcode.elf --start-address=0x$a --stop-address=0x$next | tail -n +7
