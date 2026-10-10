#include "kernel.h"

static int fs_ready;
static fs_entry_disk_t entries[MAX_FS_FILES];
static uint32_t entry_count;
static uint8_t sector_buf[512] __attribute__((aligned(2)));

static int ata_wait(uint8_t mask, uint8_t expected) {
    for (uint32_t i=0;i<1000000;i++) {
        uint8_t s=inb(0x1F7);
        if (s & 0x01) return 0;
        if ((s & mask) == expected) return 1;
    }
    return 0;
}

static int ata_read28(uint32_t lba, uint8_t *buf) {
    if (!ata_wait(0x80, 0)) return 0;
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F4, (uint8_t)(lba>>8));
    outb(0x1F5, (uint8_t)(lba>>16));
    outb(0x1F6, 0xE0 | ((lba>>24)&0x0F));
    outb(0x1F7, 0x20);
    if (!ata_wait(0x88, 0x08)) return 0;
    for (int i=0;i<256;i++) ((uint16_t *)buf)[i]=inw(0x1F0);
    return 1;
}

static size_t cstr_len(const char *s, size_t max) { size_t n=0; while(n<max && s[n]) n++; return n; }
static int cstr_eq(const char *a, const char *b, size_t max) {
    for(size_t i=0;i<max;i++){ if(a[i]!=b[i]) return 0; if(!a[i] || !b[i]) return 1; }
    return 1;
}

void fs_init(void) {
    fs_ready=0; entry_count=0;
    if (!ata_read28((uint32_t)FS_LBA, sector_buf)) { klog("filesystem: block device unavailable"); return; }
    uint32_t magic = *(uint32_t *)&sector_buf[0];
    uint32_t count = *(uint32_t *)&sector_buf[8];
    if (magic != FS_MAGIC || count > MAX_FS_FILES) { klog("filesystem: invalid NOVA-FS header"); return; }
    for (uint32_t i=0;i<count;i++) {
        const fs_entry_disk_t *e = (const fs_entry_disk_t *)&sector_buf[16 + i*sizeof(fs_entry_disk_t)];
        entries[i] = *e;
    }
    entry_count=count; fs_ready=1;
    klog("filesystem: NOVA-FS mounted read-only");
}

int fs_list(user_fs_entry_t *out, size_t max_entries) {
    if (!fs_ready) return -2;
    size_t n = entry_count < max_entries ? entry_count : max_entries;
    for(size_t i=0;i<n;i++) {
        for(size_t j=0;j<32;j++) out[i].name[j]=entries[i].name[j];
        out[i].size=entries[i].size; out[i].mode=entries[i].mode; out[i].uid=entries[i].uid;
    }
    return (int)n;
}

int fs_read(const char *name, uint8_t *out, size_t max_size) {
    if (!fs_ready || !name) return -2;
    size_t name_len=cstr_len(name,31);
    if (!name_len) return -3;
    for(uint32_t i=0;i<entry_count;i++) {
        if (!cstr_eq(entries[i].name,name,32)) continue;
        size_t total=entries[i].size < max_size ? entries[i].size : max_size;
        size_t copied=0;
        uint32_t lba=entries[i].lba;
        while(copied<total) {
            if(!ata_read28(lba++, sector_buf)) return -4;
            size_t take=total-copied; if(take>512) take=512;
            for(size_t j=0;j<take;j++) out[copied+j]=sector_buf[j];
            copied+=take;
        }
        return (int)copied;
    }
    return -5;
}
