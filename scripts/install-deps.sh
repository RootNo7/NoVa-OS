#!/usr/bin/env bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
if [ "${EUID:-$(id -u)}" -eq 0 ]; then
  APT=(apt-get)
elif command -v sudo >/dev/null 2>&1; then
  APT=(sudo apt-get)
else
  echo "Need root or sudo to install WSL dependencies." >&2
  exit 1
fi
"${APT[@]}" update
"${APT[@]}" install -y build-essential binutils qemu-system-x86 python3
