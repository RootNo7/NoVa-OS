#include "kernel.h"

enum {
    SYS_CLEAR=1, SYS_RECT=2, SYS_TEXT=3, SYS_GETKEY=4, SYS_MOUSE=5,
    SYS_FS_LIST=6, SYS_FS_READ=7, SYS_EXEC=8, SYS_SLEEP=9, SYS_UPTIME=10,
    SYS_IPC=11, SYS_SYSINFO=12, SYS_REBOOT=13, SYS_EXIT=14
};

extern void delay_ticks(uint64_t n);
extern uint64_t kernel_uptime_ticks(void);

static size_t bounded_user_string(uint64_t ptr, char *out, size_t max) {
    if (!max || !user_ptr_ok(ptr,1)) return 0;
    for (size_t i=0;i+1<max;i++) {
        if (!user_ptr_ok(ptr+i,1)) return 0;
        char c=((const char *)ptr)[i]; out[i]=c;
        if (!c) return i;
    }
    out[max-1]=0;
    return max-1;
}

static int copy_user_string(uint64_t ptr, char *out, size_t max) {
    if (!user_ptr_ok(ptr,1)) return 0;
    for (size_t i=0;i<max;i++) {
        if (!user_ptr_ok(ptr+i,1)) return 0;
        out[i]=((const char *)ptr)[i];
        if (!out[i]) return 1;
    }
    out[max-1]=0;
    return 1;
}

static int app_exec(int id, int_frame_t *f) {
    const uint8_t *img=0; size_t size=0;
    if (id==0) { img=_binary_shell_bin_start; size=(size_t)(_binary_shell_bin_end-img); }
    else if (id==1) { img=_binary_paint_bin_start; size=(size_t)(_binary_paint_bin_end-img); }
    else return -1;
    load_user_image(img,size);
    f->rip=USER_BASE;
    f->rsp=USER_STACK+4096;
    f->rax=0;
    return 0;
}

void syscall_dispatch(int_frame_t *f) {
    if ((f->cs & 3) != 3) { f->rax=(uint64_t)-1; return; }
    switch (f->rax) {
    case SYS_CLEAR:
        gfx_clear((uint8_t)f->rdi); f->rax=0; break;

    case SYS_RECT:
        if ((int64_t)f->rdx < 0 || (int64_t)f->rcx < 0 ||
            f->rdi >= VGA_WIDTH || f->rsi >= VGA_HEIGHT) { f->rax=(uint64_t)-1; break; }
        gfx_rect((int)f->rdi,(int)f->rsi,(int)f->rdx,(int)f->rcx,(uint8_t)f->r8);
        f->rax=0; break;

    case SYS_TEXT: {
        char s[256];
        size_t max=f->r8; if(max==0) max=255; if(max>255) max=255;
        if (!bounded_user_string(f->rcx,s,max)) { f->rax=(uint64_t)-1; break; }
        gfx_text((int)f->rdi,(int)f->rsi,(uint8_t)f->rdx,s,255);
        f->rax=0; break;
    }

    case SYS_GETKEY:
        f->rax=(uint64_t)(int64_t)keyboard_getchar();
        break;

    case SYS_MOUSE: {
        mouse_state_t m=mouse_get();
        f->rax=((uint64_t)m.buttons<<32)|((uint64_t)(uint16_t)m.y<<16)|(uint16_t)m.x;
        break;
    }

    case SYS_FS_LIST: {
        size_t max=f->rsi;
        if (max>MAX_FS_FILES) max=MAX_FS_FILES;
        if (!user_ptr_ok(f->rdi, max*sizeof(user_fs_entry_t))) { f->rax=(uint64_t)-1; break; }
        f->rax=(uint64_t)(int64_t)fs_list((user_fs_entry_t *)f->rdi,max);
        break;
    }

    case SYS_FS_READ: {
        char name[33]; size_t max=f->rdx;
        if (max>8192) max=8192;
        if (!copy_user_string(f->rdi,name,sizeof(name)) || !user_ptr_ok(f->rsi,max)) { f->rax=(uint64_t)-1; break; }
        f->rax=(uint64_t)(int64_t)fs_read(name,(uint8_t *)f->rsi,max);
        break;
    }

    case SYS_EXEC:
        f->rax=(uint64_t)(int64_t)app_exec((int)f->rdi,f);
        break;

    case SYS_SLEEP: {
        uint64_t ms=f->rdi;
        if (ms>60000) ms=60000;
        uint64_t t=(ms+9)/10;
        delay_ticks(t);
        f->rax=0;
        break;
    }

    case SYS_UPTIME:
        f->rax=kernel_uptime_ticks();
        break;

    case SYS_IPC: {
        size_t out_size=f->rdx;
        if (out_size>128) out_size=128;
        if (!user_ptr_ok(f->rsi,out_size)) { f->rax=(uint64_t)-1; break; }
        char msg[65];
        if (!copy_user_string(f->rdi,msg,sizeof(msg))) { f->rax=(uint64_t)-1; break; }
        f->rax=(uint64_t)(int64_t)ipc_echo(msg,(char *)f->rsi,out_size);
        break;
    }

    case SYS_SYSINFO: {
        // 5 x u64: memory bytes, framebuffer address, uid, pid, uptime ticks.
        if (!user_ptr_ok(f->rdi,5*sizeof(uint64_t))) { f->rax=(uint64_t)-1; break; }
        uint64_t *p=(uint64_t *)f->rdi;
        p[0]=16ULL*1024*1024; p[1]=0xA0000; p[2]=1000; p[3]=1; p[4]=kernel_uptime_ticks();
        f->rax=0; break;
    }

    case SYS_REBOOT:
        // The only user process is uid 1000; reboot stays a controlled privileged operation.
        f->rax=(uint64_t)-13; break; // EPERM

    case SYS_EXIT:
        load_user_image(_binary_shell_bin_start,
                        (size_t)(_binary_shell_bin_end-_binary_shell_bin_start));
        f->rip=USER_BASE;
        f->rsp=USER_STACK+4096;
        f->rax=0;
        break;

    default:
        f->rax=(uint64_t)-38; // ENOSYS
        break;
    }
}
