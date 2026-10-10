#include "kernel.h"

static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdpt[512] __attribute__((aligned(4096)));
static uint64_t pd[512] __attribute__((aligned(4096)));
static uint64_t user_pt[512] __attribute__((aligned(4096)));
static uint64_t user_pool_cursor;

static void mem_set(void *dst, uint8_t v, size_t n) {
    uint8_t *d = dst; while (n--) *d++ = v;
}
static void mem_copy(void *dst, const void *src, size_t n) {
    uint8_t *d=dst; const uint8_t *s=src; while (n--) *d++=*s++;
}
static size_t str_len(const char *s, size_t max) { size_t n=0; while(n<max && s[n]) n++; return n; }

void memory_init(void) {
    user_pool_cursor = USER_POOL_PHYS;
}

static uint64_t align_up(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }

void paging_init(void) {
    mem_set(pml4, 0, sizeof(pml4));
    mem_set(pdpt, 0, sizeof(pdpt));
    mem_set(pd, 0, sizeof(pd));
    mem_set(user_pt, 0, sizeof(user_pt));

    pml4[0] = ((uint64_t)pdpt) | 0x007;
    pdpt[0] = ((uint64_t)pd) | 0x007;
    for (int i=0;i<8;i++) pd[i] = ((uint64_t)i * 0x200000ULL) | 0x083; // 2 MiB pages, supervisor
    pd[2] = ((uint64_t)user_pt) | 0x007; // 4..6 MiB: fine-grained user mapping

    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4) : "memory");
}

int user_ptr_ok(uint64_t ptr, size_t len) {
    if (ptr < USER_BASE || ptr >= USER_LIMIT) return 0;
    if (len > (size_t)(USER_LIMIT - ptr)) return 0;
    return 1;
}

void load_user_image(const uint8_t *image, size_t size) {
    if (!image || !size || size > (USER_LIMIT-USER_BASE-0x3000)) panic("invalid user image");
    for (size_t i=0;i<512;i++) user_pt[i]=0;
    user_pool_cursor = USER_POOL_PHYS;
    uint64_t phys = user_pool_cursor;
    size_t pages = (size + 4095) / 4096;
    if (phys + (pages+1)*4096 > 0x01000000ULL) panic("user memory exhausted");

    mem_set((void *)phys, 0, pages*4096);
    mem_copy((void *)phys, image, size);
    for (size_t i=0;i<pages;i++) user_pt[i] = (phys + i*4096) | 0x007;

    uint64_t stack_phys = phys + pages*4096;
    mem_set((void *)stack_phys, 0, 4096);
    user_pt[511] = stack_phys | 0x007;
    user_pool_cursor = align_up(stack_phys + 4096, 4096);

    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4) : "memory");
}

void ipc_init(void) {}
int ipc_echo(const char *msg, char *out, size_t out_size) {
    size_t n = str_len(msg, MAX_MSG-1);
    if (!out_size) return -1;
    if (n >= out_size) n = out_size-1;
    mem_copy(out, msg, n);
    out[n] = 0;
    return (int)n;
}

void kernel_main(uint64_t boot_drive) {
    (void)boot_drive;
    serial_puts("\r\nNOVA OS first build\r\n");
    klog("bootloader handoff complete");

    gdt_init();
    idt_init();
    memory_init();
    paging_init();
    pic_init();
    pit_init(100);
    keyboard_init();
    mouse_init();
    graphics_init();
    fs_init();
    ipc_init();

    klog("kernel services initialized");
    serial_puts("[NOVA] shell image bytes: "); serial_dec((uint64_t)(_binary_shell_bin_end - _binary_shell_bin_start)); serial_puts("\r\n");
    serial_puts("[NOVA] paint image bytes: "); serial_dec((uint64_t)(_binary_paint_bin_end - _binary_paint_bin_start)); serial_puts("\r\n");

    load_user_image(_binary_shell_bin_start,
                    (size_t)(_binary_shell_bin_end - _binary_shell_bin_start));

    __asm__ volatile("sti");
    enter_user(USER_BASE, USER_STACK + 4096);
    panic("returned from user mode");
}
