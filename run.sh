
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
IMG="$ROOT/build/nova.img"

[[ -f "$IMG" ]] || "$ROOT/build.sh"

QEMU="${QEMU:-qemu-system-x86_64}"
MODE="${1:-auto}"

case "$MODE" in
  auto)
    if [[ -n "${DISPLAY:-}" || -n "${WAYLAND_DISPLAY:-}" ]]; then
      DISPLAY_MODE="gtk"
    else
      DISPLAY_MODE="none"
    fi
    ;;
  gui)
    DISPLAY_MODE="gtk"
    ;;
  headless)
    DISPLAY_MODE="none"
    ;;
  *)
    echo "Usage: $0 [auto|gui|headless]" >&2
    exit 2
    ;;
esac

exec "$QEMU" \
  -drive "format=raw,file=$IMG,if=ide" \
  -serial stdio \
  -m 128M \
  -display "$DISPLAY_MODE" \
  -no-reboot \
  -no-shutdown
