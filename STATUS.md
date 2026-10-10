# First-build verification status

## Verified in the current build environment

| Check | Result |
|---|---|
| 512-byte boot sector | PASS |
| Boot signature `55 AA` | PASS |
| Kernel ELF is x86_64 executable | PASS |
| Kernel fits fixed 512 KiB boot slot | PASS |
| Shell/paint link at `0x400000` | PASS |
| Kernel/user ELF relocation-free | PASS |
| NOVA-FS header parses | PASS |
| Static validation script | PASS |
| Reproducible rebuild hashes | PASS |

## Not executed here

QEMU execution is **not validated in this container** because `qemu-system-x86_64` is absent and this environment cannot currently reach Debian package repositories. The repository therefore does not claim a successful runtime boot from this particular container.

The intended WSL validation command is:

```bash
./scripts/install-deps.sh
./build.sh
./run.sh
```

After QEMU is available, the required runtime checks are: boot to the desktop, keyboard input, mouse movement, shell commands, filesystem reads, `run paint`, returning to shell, timer/uptime, syscall boundary behavior, and serial logs without critical faults.
