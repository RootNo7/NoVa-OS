#include "kernel.h"

static uint64_t gdt[9] __attribute__((aligned(16)));
static uint64_t idt[256][2] __attribute__((aligned(16)));
tss64_t tss64 __attribute__((aligned(16)));
extern uint8_t kernel_stack_top;

static void lgdt(void *base, uint16_t size_minus_one) {
    struct __attribute__((packed)) { uint16_t limit; uint64_t base; } p = { size_minus_one, (uint64_t)base };
    __asm__ volatile("lgdt %0" : : "m"(p));
}

static void lidt(void *base, uint16_t size_minus_one) {
    struct __attribute__((packed)) { uint16_t limit; uint64_t base; } p = { size_minus_one, (uint64_t)base };
    __asm__ volatile("lidt %0" : : "m"(p));
}

static void set_seg(uint64_t *slot, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    uint64_t d = 0;
    d |= (limit & 0xffffULL);
    d |= ((uint64_t)(base & 0xffffffULL)) << 16;
    d |= ((uint64_t)access) << 40;
    d |= ((uint64_t)(limit >> 16) & 0x0fULL) << 48;
    d |= ((uint64_t)(flags & 0x0fULL)) << 52;
    d |= ((uint64_t)(base >> 24)) << 56;
    *slot = d;
}

static void set_idt_gate(int n, void (*handler)(void), uint8_t type_attr, uint16_t selector) {
    uint64_t addr = (uint64_t)handler;
    uint64_t lo = 0, hi = 0;
    lo |= (addr & 0xffffULL);
    lo |= ((uint64_t)selector) << 16;
    lo |= ((uint64_t)(type_attr)) << 40;
    lo |= ((addr >> 16) & 0xffffULL) << 48;
    hi |= addr >> 32;
    idt[n][0] = lo;
    idt[n][1] = hi;
}

void gdt_init(void) {
    for (size_t i = 0; i < 9; ++i) gdt[i] = 0;
    set_seg(&gdt[1], 0, 0xFFFFF, 0x9A, 0xC); // 32-bit code
    set_seg(&gdt[2], 0, 0xFFFFF, 0x92, 0xC); // 32-bit data
    set_seg(&gdt[3], 0, 0xFFFFF, 0x9A, 0xA); // 64-bit kernel code
    set_seg(&gdt[4], 0, 0xFFFFF, 0x92, 0xC); // kernel data
    set_seg(&gdt[5], 0, 0xFFFFF, 0xFA, 0xA); // 64-bit user code, DPL3
    set_seg(&gdt[6], 0, 0xFFFFF, 0xF2, 0xC); // user data, DPL3

    for (size_t i = 0; i < sizeof(tss64); ++i) ((uint8_t *)&tss64)[i] = 0;
    tss64.rsp0 = (uint64_t)&kernel_stack_top;
    tss64.iomap_base = sizeof(tss64);

    uint64_t base = (uint64_t)&tss64;
    uint64_t limit = sizeof(tss64) - 1;
    uint64_t low = 0, high = 0;
    low |= (limit & 0xffffULL);
    low |= (base & 0xffffffULL) << 16;
    low |= 0x89ULL << 40; // available 64-bit TSS, present
    low |= ((limit >> 16) & 0x0fULL) << 48;
    low |= ((base >> 24) & 0xffULL) << 56;
    high = base >> 32;
    gdt[7] = low;
    gdt[8] = high;

    lgdt(gdt, sizeof(gdt) - 1);
    __asm__ volatile("ltr %%ax" : : "a"((uint16_t)0x38));
}

void idt_init(void) {
    for (size_t i = 0; i < 256; ++i) { idt[i][0] = 0; idt[i][1] = 0; }

    void (*handlers[32])(void) = {
        isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,isr12,isr13,isr14,isr15,
        isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31
    };
    for (int i = 0; i < 32; ++i) set_idt_gate(i, handlers[i], 0x8E, 0x18);
    set_idt_gate(32, isr32, 0x8E, 0x18);
    set_idt_gate(33, isr33, 0x8E, 0x18);
    set_idt_gate(44, isr44, 0x8E, 0x18);
    set_idt_gate(128, isr128, 0xEE, 0x18);

    lidt(idt, sizeof(idt) - 1);
}

void enter_user(uint64_t rip, uint64_t rsp) {
    __asm__ volatile(
        "cli\n"
        "movw $0x33, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "pushq $0x33\n"
        "pushq %[user_rsp]\n"
        "pushq $0x202\n"
        "pushq $0x2B\n"
        "pushq %[user_rip]\n"
        "iretq\n"
        : : [user_rsp] "r"(rsp), [user_rip] "r"(rip) : "rax", "memory"
    );
    __builtin_unreachable();
}

void cpu_halt(void) {
    for (;;) __asm__ volatile("cli; hlt");
}
