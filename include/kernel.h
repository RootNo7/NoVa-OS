#ifndef NOVA_KERNEL_H
#define NOVA_KERNEL_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define USER_BASE 0x400000ULL
#define USER_LIMIT 0x600000ULL
#define USER_STACK 0x5FF000ULL
#define USER_POOL_PHYS 0x00600000ULL
#define FS_LBA 1025ULL
#define FS_MAGIC 0x3153564eU /* NV S1 little-endian */
#define MAX_FS_FILES 11
#define MAX_MSG 64

typedef struct __attribute__((packed)) int_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t rip, cs, rflags, rsp, ss;
} int_frame_t;

typedef struct __attribute__((packed)) tss64 {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} tss64_t;

typedef struct __attribute__((packed)) fs_entry_disk {
    char name[32];
    uint32_t size;
    uint32_t lba;
    uint32_t mode;
    uint32_t uid;
} fs_entry_disk_t;

typedef struct user_fs_entry {
    char name[32];
    uint32_t size;
    uint32_t mode;
    uint32_t uid;
} user_fs_entry_t;

typedef struct mouse_state {
    int x, y;
    uint8_t buttons;
} mouse_state_t;

typedef struct process {
    uint32_t pid;
    uint32_t uid;
    uint32_t state;
    uint64_t ticks;
    uint64_t entry;
} process_t;

void panic(const char *msg);
void klog(const char *msg);
void serial_puts(const char *s);
void serial_hex(uint64_t v);
void serial_dec(uint64_t v);

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);
void io_wait(void);

void pic_init(void);
void pit_init(uint32_t hz);
void keyboard_init(void);
void mouse_init(void);
void input_poll(void);
int keyboard_getchar(void);
mouse_state_t mouse_get(void);

void graphics_init(void);
void gfx_clear(uint8_t c);
void gfx_rect(int x, int y, int w, int h, uint8_t c);
void gfx_text(int x, int y, uint8_t c, const char *s, size_t max_len);
void gfx_palette_init(void);

void memory_init(void);
void paging_init(void);
void load_user_image(const uint8_t *image, size_t size);
int user_ptr_ok(uint64_t ptr, size_t len);

void gdt_init(void);
void enter_user(uint64_t rip, uint64_t rsp);
extern tss64_t tss64;

void fs_init(void);
int fs_list(user_fs_entry_t *out, size_t max_entries);
int fs_read(const char *name, uint8_t *out, size_t max_size);

void syscall_dispatch(int_frame_t *f);
void interrupt_dispatch(int_frame_t *f);
void scheduler_tick(void);
uint64_t kernel_uptime_ticks(void);
void delay_ticks(uint64_t n);

void ipc_init(void);
int ipc_echo(const char *msg, char *out, size_t out_size);

extern const uint8_t _binary_shell_bin_start[];
extern const uint8_t _binary_shell_bin_end[];
extern const uint8_t _binary_paint_bin_start[];
extern const uint8_t _binary_paint_bin_end[];

extern void isr0(void); extern void isr1(void); extern void isr2(void); extern void isr3(void);
extern void isr4(void); extern void isr5(void); extern void isr6(void); extern void isr7(void);
extern void isr8(void); extern void isr9(void); extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
extern void isr32(void); extern void isr33(void); extern void isr44(void); extern void isr128(void);

void idt_init(void);
void cpu_halt(void);

#endif
