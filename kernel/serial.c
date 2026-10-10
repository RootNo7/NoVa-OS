#include "kernel.h"

static const uint16_t COM1 = 0x3F8;

static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static int ready(void) { return inb(COM1 + 5) & 0x20; }

void serial_puts(const char *s) {
    static int init;
    if (!init) { serial_init(); init = 1; }
    while (*s) {
        while (!ready()) {}
        outb(COM1, (uint8_t)*s++);
    }
}

void serial_hex(uint64_t v) {
    static const char h[] = "0123456789ABCDEF";
    char b[17];
    for (int i = 15; i >= 0; --i) b[15-i] = h[(v >> (i*4)) & 0xF];
    b[16] = 0;
    serial_puts(b);
}

void serial_dec(uint64_t v) {
    char b[21]; int i = 20; b[i] = 0;
    if (!v) { serial_puts("0"); return; }
    while (v && i) { b[--i] = (char)('0' + (v % 10)); v /= 10; }
    serial_puts(&b[i]);
}

void klog(const char *msg) { serial_puts("[NOVA] "); serial_puts(msg); serial_puts("\r\n"); }
void panic(const char *msg) { serial_puts("\r\n[NOVA PANIC] "); serial_puts(msg); serial_puts("\r\n"); cpu_halt(); }
