#!/bin/bash
# Builds tools/nids.tsv (NID -> system function name) from the PSPSDK stub libraries. Runs in WSL.
export PATH=$PATH:/opt/pspdev/bin
OUT="$(dirname "$0")/nids.tsv"
T=$(mktemp -d); cd $T
for a in /opt/pspdev/psp/sdk/lib/*.a; do
  mkdir -p x && cd x && psp-ar x "$a" 2>/dev/null
  for o in *.o; do
    [ -f "$o" ] || continue
    nid=$(psp-objdump -s -j .rodata.sceNid "$o" 2>/dev/null | awk '/^ 0000/{print $2}')
    [ -n "$nid" ] || continue
    name=$(psp-objdump -t "$o" | awk '$4==".sceStub.text" && $2=="g" {print $NF}' | head -1)
    [ -n "$name" ] || continue
    # the NID is stored little endian
    n=${nid:6:2}${nid:4:2}${nid:2:2}${nid:0:2}
    echo -e "${n^^}\t$name"
  done
  cd .. && rm -rf x
done | sort -u > "$OUT"
wc -l "$OUT"
