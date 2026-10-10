#include "kernel.h"

static volatile uint64_t ticks;
static volatile uint32_t scheduler_slice;
static char key_queue[128];
static volatile uint32_t key_head, key_tail;
static uint8_t shift_down;
static uint8_t caps_lock;
static uint8_t mouse_cycle;
static uint8_t mouse_packet[3];
static mouse_state_t mouse_state = { 160, 100, 0 };

void outb(uint16_t port, uint8_t value) { __asm__ volatile("outb %0,%1" : : "a"(value), "Nd"(port)); }
uint8_t inb(uint16_t port) { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(port)); return v; }
void outw(uint16_t port, uint16_t value) { __asm__ volatile("outw %0,%1" : : "a"(value), "Nd"(port)); }
uint16_t inw(uint16_t port) { uint16_t v; __asm__ volatile("inw %1,%0" : "=a"(v) : "Nd"(port)); return v; }
void io_wait(void) { outb(0x80, 0); }

static void pic_eoi(unsigned irq) {
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void pic_init(void) {
    uint8_t a1 = inb(0x21), a2 = inb(0xA1);
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    (void)a1; (void)a2;
    // Unmask master IRQ0 (timer), IRQ1 (keyboard), IRQ2 (cascade).
    outb(0x21, 0xF8);
    // Unmask slave IRQ12 (mouse).
    outb(0xA1, 0xEF);
}

void pit_init(uint32_t hz) {
    if (!hz) hz = 100;
    uint32_t div = 1193182u / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(div & 0xff));
    outb(0x40, (uint8_t)((div >> 8) & 0xff));
}

static const char normal[128] = {
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',[0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',
    [0x0C]='-',[0x0D]='=',[0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',[0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',
    [0x1A]='[',[0x1B]=']',[0x1C]='\n',[0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',[0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',
    [0x27]=';',[0x28]='\'',[0x29]='`',[0x2B]='\\',[0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',[0x32]='m',
    [0x33]=',',[0x34]='.',[0x35]='/',[0x39]=' ',
};
static const char shifted[128] = {
    [0x02]='!',[0x03]='@',[0x04]='#',[0x05]='$',[0x06]='%',[0x07]='^',[0x08]='&',[0x09]='*',[0x0A]='(',[0x0B]=')',
    [0x0C]='_',[0x0D]='+',[0x10]='Q',[0x11]='W',[0x12]='E',[0x13]='R',[0x14]='T',[0x15]='Y',[0x16]='U',[0x17]='I',[0x18]='O',[0x19]='P',
    [0x1A]='{',[0x1B]='}',[0x1C]='\n',[0x1E]='A',[0x1F]='S',[0x20]='D',[0x21]='F',[0x22]='G',[0x23]='H',[0x24]='J',[0x25]='K',[0x26]='L',
    [0x27]=':',[0x28]='"',[0x29]='~',[0x2B]='|',[0x2C]='Z',[0x2D]='X',[0x2E]='C',[0x2F]='V',[0x30]='B',[0x31]='N',[0x32]='M',
    [0x33]='<',[0x34]='>',[0x35]='?',[0x39]=' ',
};

static void key_push(char c) {
    uint32_t next = (key_head + 1) % (uint32_t)sizeof(key_queue);
    if (next != key_tail) { key_queue[key_head] = c; key_head = next; }
}

void keyboard_init(void) {
    key_head = key_tail = 0;
    shift_down = 0;
    caps_lock = 0;
}

static void keyboard_irq(void) {
    uint8_t sc = inb(0x60);
    if (sc == 0x2A || sc == 0x36) { shift_down = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift_down = 0; return; }
    if (sc == 0x3A) { caps_lock ^= 1; return; }
    if (sc & 0x80) return;
    char c = shift_down ? shifted[sc] : normal[sc];
    if (caps_lock && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c) key_push(c);
}

void mouse_init(void) {
    // Enable auxiliary mouse device.
    while (inb(0x64) & 2) {}
    outb(0x64, 0xA8);
    while (inb(0x64) & 2) {}
    outb(0x64, 0x20);
    while (!(inb(0x64) & 1)) {}
    uint8_t status = inb(0x60) | 2;
    while (inb(0x64) & 2) {}
    outb(0x64, 0x60);
    while (inb(0x64) & 2) {}
    outb(0x60, status);
    while (inb(0x64) & 2) {}
    outb(0x64, 0xD4);
    while (inb(0x64) & 2) {}
    outb(0x60, 0xF4);
    // Consume controller ACK when available.
    for (volatile int i = 0; i < 100000; ++i) {
        if (inb(0x64) & 1) { (void)inb(0x60); break; }
    }
}

static void mouse_irq(void) {
    uint8_t status = inb(0x64);
    if (!(status & 1) || !(status & 0x20)) return;
    uint8_t b = inb(0x60);
    if (mouse_cycle == 0 && !(b & 0x08)) return;
    mouse_packet[mouse_cycle++] = b;
    if (mouse_cycle < 3) return;
    mouse_cycle = 0;
    int dx = (int8_t)mouse_packet[1];
    int dy = (int8_t)mouse_packet[2];
    mouse_state.x += dx;
    mouse_state.y -= dy;
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.y < 0) mouse_state.y = 0;
    if (mouse_state.x >= VGA_WIDTH) mouse_state.x = VGA_WIDTH-1;
    if (mouse_state.y >= VGA_HEIGHT) mouse_state.y = VGA_HEIGHT-1;
    mouse_state.buttons = mouse_packet[0] & 0x07;
}

void input_poll(void) {
    while (inb(0x64) & 1) {
        uint8_t status = inb(0x64);
        if (status & 0x20) {
            // Mouse controller IRQ is normally used; this path keeps polled systems responsive.
            if (mouse_cycle == 0 && !(inb(0x60) & 0x08)) continue;
        } else {
            keyboard_irq();
        }
    }
}

int keyboard_getchar(void) {
    if (key_tail == key_head) return -1;
    int c = (unsigned char)key_queue[key_tail];
    key_tail = (key_tail + 1) % (uint32_t)sizeof(key_queue);
    return c;
}

mouse_state_t mouse_get(void) { return mouse_state; }

void scheduler_tick(void) {
    ++ticks;
    ++scheduler_slice;
    if (scheduler_slice >= 10) scheduler_slice = 0;
}

uint64_t kernel_ticks(void) { return ticks; }

static volatile uint32_t irq_depth;
void interrupt_dispatch(int_frame_t *f) {
    irq_depth++;
    uint64_t vector = f->vector;
    if (vector == 32) {
        scheduler_tick();
        pic_eoi(0);
    } else if (vector == 33) {
        keyboard_irq();
        pic_eoi(1);
    } else if (vector == 44) {
        mouse_irq();
        pic_eoi(12);
    } else if (vector == 128) {
        syscall_dispatch(f);
    } else if (vector < 32) {
        if (vector == 14 && (f->cs & 3) == 3) {
            serial_puts("[NOVA] user page fault; terminating current image\r\n");
        } else {
            serial_puts("[NOVA] CPU exception vector "); serial_dec(vector); serial_puts("\r\n");
        }
        f->rax = (uint64_t)-1;
        if ((f->cs & 3) == 3) {
            // Return to the immutable shell entry. The shell can report that the child failed.
            f->rip = USER_BASE;
        } else {
            panic("kernel exception");
        }
    }
    irq_depth--;
}

void delay_ticks(uint64_t n) {
    uint64_t start = ticks;
    while (ticks - start < n) __asm__ volatile("sti; hlt" : : : "memory");
}

uint64_t kernel_uptime_ticks(void) { return ticks; }
