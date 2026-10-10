.section .text.entry
.global _start
.global kernel_stack_top
.type _start,@function
.extern kernel_main
_start:
    cli
    movq %rdi, %rax
    lea kernel_stack_top(%rip), %rsp
    andq $-16, %rsp
    pushq $0
    popfq
    movq %rax, %rdi
    call kernel_main
1:
    cli
    hlt
    jmp 1b

.section .bss
.align 16
kernel_stack:
    .skip 16384
kernel_stack_top:
