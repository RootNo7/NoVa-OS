#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"
mkdir -p "$BUILD"

CC=${CC:-gcc}
LD=${LD:-ld}
OBJCOPY=${OBJCOPY:-objcopy}
AS=${AS:-as}

CFLAGS=(-std=gnu11 -ffreestanding -fno-stack-protector -fno-pie -no-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel -O2 -Wall -Wextra -Werror=implicit-function-declaration -I"$ROOT/include")
USER_CFLAGS=(-std=gnu11 -ffreestanding -fno-stack-protector -fno-pie -no-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=small -O2 -Wall -Wextra -Werror=implicit-function-declaration -Wno-unused-function -Wno-unused-variable -fno-asynchronous-unwind-tables -fno-unwind-tables -nostdinc -I"$ROOT/include")

rm -f "$BUILD"/*.o "$BUILD"/*.elf "$BUILD"/*.bin "$BUILD"/*.img

printf '[1/8] bootloader\n'
$AS --32 -o "$BUILD/boot.o" "$ROOT/boot/boot.S"
$LD -m elf_i386 -T "$ROOT/boot/boot.ld" -o "$BUILD/boot.bin" "$BUILD/boot.o"
BOOT_SIZE=$(stat -c%s "$BUILD/boot.bin")
[[ "$BOOT_SIZE" -eq 512 ]] || { echo "boot sector must be 512 bytes, got $BOOT_SIZE" >&2; exit 1; }

printf '[2/8] kernel objects\n'
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/kernel.c" -o "$BUILD/kernel.o"
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/drivers.c" -o "$BUILD/drivers.o"
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/graphics.c" -o "$BUILD/graphics.o"
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/fs.c" -o "$BUILD/fs.o"
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/syscall.c" -o "$BUILD/syscall.o"
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/gdt.c" -o "$BUILD/gdt.o"
$CC "${CFLAGS[@]}" -c "$ROOT/kernel/serial.c" -o "$BUILD/serial.o"
$AS --64 -o "$BUILD/entry64.o" "$ROOT/kernel/entry64.S"
$AS --64 -o "$BUILD/interrupts.o" "$ROOT/kernel/interrupts.S"

printf '[3/8] user programs\n'
$CC "${USER_CFLAGS[@]}" -c "$ROOT/user/sys.c" -o "$BUILD/user_sys.o"
$CC "${USER_CFLAGS[@]}" -c "$ROOT/user/shell.c" -o "$BUILD/user_shell.o"
$CC "${USER_CFLAGS[@]}" -c "$ROOT/user/paint.c" -o "$BUILD/user_paint.o"
$LD -T "$ROOT/user/user.ld" -o "$BUILD/shell.elf" "$BUILD/user_sys.o" "$BUILD/user_shell.o"
$LD -T "$ROOT/user/user.ld" -o "$BUILD/paint.elf" "$BUILD/user_sys.o" "$BUILD/user_paint.o"
$OBJCOPY -O binary "$BUILD/shell.elf" "$BUILD/shell.bin"
$OBJCOPY -O binary "$BUILD/paint.elf" "$BUILD/paint.bin"
(cd "$BUILD" && $OBJCOPY -I binary -O elf64-x86-64 -B i386:x86-64 shell.bin shell_blob.o)
(cd "$BUILD" && $OBJCOPY -I binary -O elf64-x86-64 -B i386:x86-64 paint.bin paint_blob.o)

printf '[4/8] kernel link\n'
$CC -nostdlib -no-pie -Wl,-T,"$ROOT/kernel/kernel.ld",-z,max-page-size=0x1000,--build-id=none -o "$BUILD/kernel.elf" \
  "$BUILD/entry64.o" "$BUILD/interrupts.o" "$BUILD/kernel.o" "$BUILD/drivers.o" "$BUILD/graphics.o" "$BUILD/fs.o" "$BUILD/syscall.o" "$BUILD/gdt.o" "$BUILD/serial.o" \
  "$BUILD/shell_blob.o" "$BUILD/paint_blob.o" -lgcc
$OBJCOPY -O binary "$BUILD/kernel.elf" "$BUILD/kernel.bin"
KERNEL_SIZE=$(stat -c%s "$BUILD/kernel.bin")
[[ "$KERNEL_SIZE" -le $((512*1024)) ]] || { echo "kernel exceeds 512 KiB boot slot: $KERNEL_SIZE" >&2; exit 1; }

printf '[5/8] NOVA-FS image\n'
python3 "$ROOT/tools/mkfs.py" "$ROOT/fs" "$BUILD/fs.bin"

printf '[6/8] disk image\n'
# Sector 0: boot sector. Sectors 1..1024: fixed kernel slot. Sector 1025+: NOVA-FS.
KERNEL_SECTORS=1024
FS_LBA=1025
FS_SIZE=$(stat -c%s "$BUILD/fs.bin")
FS_SECTORS=$(( (FS_SIZE + 511) / 512 ))
TOTAL_SECTORS=$((FS_LBA + FS_SECTORS))
truncate -s $((TOTAL_SECTORS * 512)) "$BUILD/nova.img"
dd if="$BUILD/boot.bin" of="$BUILD/nova.img" bs=512 seek=0 conv=notrunc status=none
dd if="$BUILD/kernel.bin" of="$BUILD/nova.img" bs=512 seek=1 conv=notrunc status=none
# pad the fixed kernel slot with zeros by leaving the pre-sized image untouched
# (filesystem begins at the fixed FS_LBA below).
dd if="$BUILD/fs.bin" of="$BUILD/nova.img" bs=512 seek=$FS_LBA conv=notrunc status=none

printf '[7/8] static validation\n'
file "$BUILD/kernel.elf" "$BUILD/nova.img" || true
objdump -f "$BUILD/kernel.elf" | sed -n '1,4p'
objdump -h "$BUILD/kernel.elf" | sed -n '1,18p'

printf '[8/8] complete\n'
printf 'image: %s\n' "$BUILD/nova.img"
printf 'kernel bytes: %s\n' "$KERNEL_SIZE"
printf 'filesystem sectors: %s\n' "$FS_SECTORS"
