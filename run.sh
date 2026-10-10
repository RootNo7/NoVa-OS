#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
IMG="$ROOT/build/nova.img"
[[ -f "$IMG" ]] || "$ROOT/build.sh"
QEMU=${QEMU:-qemu-system-x86_64}
exec "$QEMU" \
  -drive "format=raw,file=$IMG,if=ide" \
  -serial stdio \
  -m 128M \
  -no-reboot \
  -no-shutdown
