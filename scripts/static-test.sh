#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
[[ -f "$BUILD/nova.img" ]] || { echo "missing build/nova.img" >&2; exit 1; }
[[ "$(stat -c%s "$BUILD/boot.bin")" -eq 512 ]]
SIG=$(od -An -tx1 -j510 -N2 "$BUILD/nova.img" | tr -d ' \n')
[[ "$SIG" == "55aa" ]]
KERNEL_SIZE=$(stat -c%s "$BUILD/kernel.bin")
(( KERNEL_SIZE <= 512*1024 ))
python3 - "$BUILD/nova.img" <<'PY'
import struct,sys
p=sys.argv[1]
with open(p,'rb') as f:
    f.seek(1025*512)
    s=f.read(512)
magic,version,count=struct.unpack_from('<III',s,0)
assert magic==0x3153564e, hex(magic)
assert version==1
assert count<=11
print(f'NOVAFS: version={version} files={count}')
PY
if readelf -r "$BUILD/kernel.elf" | grep -q 'R_X86_64_32\|R_X86_64_PC32'; then
  echo 'unexpected unresolved kernel relocations' >&2; exit 1
fi
printf 'static tests: PASS\n'
