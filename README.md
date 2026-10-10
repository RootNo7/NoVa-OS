# NOVA OS — First Build

A small, genuinely custom x86_64 operating system designed around a simple hybrid architecture and a QEMU-friendly BIOS target.

This is the **complete first-build repository**, not a Linux distribution and not a UI theme. The implementation intentionally favors a small, inspectable codebase over a dependency-heavy stack.

## Current first-build contract

- Custom 16-bit BIOS bootloader that loads a fixed kernel slot and enters long mode.
- 64-bit kernel with GDT/IDT/TSS, paging, ring-3 execution, validated syscalls, timer, keyboard and PS/2 mouse interrupts.
- 320×200 VGA Mode 13h framebuffer desktop/compositor API.
- Native user programs: desktop shell and a small paint application.
- Round-robin scheduler infrastructure for the current runnable process plus an idle path.
- Read-only NOVA-FS on the attached disk image.
- Permission metadata, UID boundary, syscall pointer validation and kernel/user memory separation.
- Serial logging for diagnostics.
- WSL build scripts and a QEMU launch script.

## Deliberate first-build limits

This build targets **legacy BIOS + PS/2 + IDE/PATA + VGA Mode 13h** as its validated hardware profile. It does not pretend to support arbitrary modern hardware, UEFI graphics, NVMe, USB HID, Wi-Fi, or Ethernet until drivers exist.

Networking therefore reports its unavailable state honestly instead of returning fake connectivity.

## Build under WSL

From WSL:

```bash
cd /path/to/nova-os-first-build
./scripts/install-deps.sh
./build.sh
./run.sh
```

The dependency installer is intentionally small. A Debian/Ubuntu WSL instance needs a freestanding-capable GCC/binutils toolchain and QEMU.

## Runtime

The desktop boots to a simple terminal-style window. Keyboard input is interrupt-driven. Mouse movement is supported through the PS/2 controller. The shell provides:

```text
help
about
clear
mem
uptime
whoami
ls
cat <file>
files
settings
net
ipc
run paint
quit
```

`quit` leaves the application and returns to the kernel idle screen; it does not power off the machine.

## Engineering notes

The bootloader intentionally loads a fixed 512 KiB kernel slot. The image builder pads this slot, so the boot protocol is deterministic and easy to inspect. The kernel itself is linked at physical address `0x100000`.

The first-build architecture is:

```text
BIOS bootloader
    ↓
x86_64 kernel
    ├─ memory / paging / traps
    ├─ timer / PIC / keyboard / mouse
    ├─ block / NOVA-FS
    ├─ syscall + IPC boundary
    └─ process/scheduler control
          ↓
      ring-3 user space
          ├─ shell + desktop compositor client
          └─ paint application
```

The compositor is exercised as a user-space client of a deliberately small graphics syscall surface rather than being hardwired into application code.

## Verification

Run `make test` after building. It validates the boot signature, fixed-slot size, NOVA-FS header, absence of link relocations, and reproducible build artifacts. Runtime verification is performed with QEMU through `make run`.
