.code16
.section .text
.global start
.type start, @function

.equ KERNEL_LOAD_SEG, 0x2000       # physical 0x20000
.equ KERNEL_START_LBA, 1
.equ KERNEL_TOTAL_SECTORS, 1024    # fixed 512 KiB slot
.equ CHUNK_SECTORS, 64             # 32 KiB per BIOS transfer
.equ CHUNKS, 16
.equ KERNEL_PHYS_SRC, 0x20000
.equ KERNEL_PHYS_DST, 0x100000

start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7c00, %sp
    movb %dl, boot_drive

    # Fast A20 gate. QEMU and normal PCs expose port 0x92.
    inb $0x92, %al
    orb $0x02, %al
    outb %al, $0x92

    # 320x200x256 VGA mode.
    movw $0x0013, %ax
    int $0x10

    # Read the complete fixed kernel slot into 0x20000..0xA0000.
    movw $KERNEL_START_LBA, current_lba
    movw $KERNEL_LOAD_SEG, current_seg
    movb $CHUNKS, chunks_left

read_next:
    movw %cs:current_seg, %ax
    movw %ax, %cs:dap+6
    movw %cs:current_lba, %ax
    movw %ax, %cs:dap+8
    movw $dap, %si
    movb %cs:boot_drive, %dl
    movb $0x42, %ah
    int $0x13
    jc disk_error

    # Advance destination by 32 KiB and LBA by 64 sectors.
    addw $0x0800, current_seg
    addw $CHUNK_SECTORS, current_lba
    decb chunks_left
    jnz read_next

    # Protected mode.
    cli
    lgdt gdt32_ptr

    movl %cr0, %eax
    orl $0x00000001, %eax
    movl %eax, %cr0
    ljmp $0x08, $protected_mode

.code32
protected_mode:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs
    movl $0x90000, %esp

    # Copy 512 KiB from low-memory staging area to the kernel's linked address.
    movl $KERNEL_PHYS_SRC, %esi
    movl $KERNEL_PHYS_DST, %edi
    movl $((512*1024)/4), %ecx
    rep movsl

    # Long-mode page tables: 0..16 MiB identity mapped with 2 MiB pages.
    # PML4 @ 0x9000, PDP @ 0xA000, PD @ 0xB000.
    movl $0x9000, %edi
    xorl %eax, %eax
    movl $3072, %ecx
    rep stosl

    # PML4[0] -> PDP.
    movl $0xA003, 0x9000
    # PDP[0] -> PD.
    movl $0xB003, 0xA000

    # Eight 2 MiB identity-mapped PDEs = 16 MiB.
    movl $0x00000083, %eax
    movl $0xB000, %edi
    movl $8, %ecx
map_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop map_pd

    # Enter long mode: CR4.PAE, EFER.LME, CR0.PG.
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    movl $0x9000, %eax
    movl %eax, %cr3

    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    lgdt gdt64_ptr
    ljmp $0x18, $long_mode

.code64
long_mode:
    movw $0x20, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs
    movq $0xA0000, %rsp

    movq $1, %rdi
    jmp 0x100000

.disk_error:
disk_error:
    movw $0x0003, %ax
    int $0x10
    movw $disk_error_msg, %si
.print:
    lodsb
    testb %al, %al
    jz .halt
    movb $0x0e, %ah
    movb $0x00, %bh
    int $0x10
    jmp .print
.halt:
    cli
    hlt
    jmp .halt

.align 8
current_lba: .long 0
current_seg: .word 0
chunks_left: .byte 0
boot_drive:  .byte 0

# Disk Address Packet.
.align 4
dap:
    .byte 0x10, 0x00
    .word CHUNK_SECTORS
    .word 0x0000
    .word KERNEL_LOAD_SEG
    .long KERNEL_START_LBA
    .long 0x00000000

.align 8
gdt32:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF     # 32-bit code
    .quad 0x00CF92000000FFFF     # 32-bit data
gdt32_end:
gdt32_ptr:
    .word gdt32_end - gdt32 - 1
    .long gdt32

.align 8
gdt64:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF     # 32-bit code (selector 0x08)
    .quad 0x00CF92000000FFFF     # 32-bit data (selector 0x10)
    .quad 0x00AF9A000000FFFF     # 64-bit code (selector 0x18)
    .quad 0x00AF92000000FFFF     # 64-bit data (selector 0x20)
gdt64_end:
gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .long gdt64

disk_error_msg:
    .asciz "NOVA BOOT DISK ERROR"

.word 0xAA55
