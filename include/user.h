#ifndef NOVA_USER_H
#define NOVA_USER_H
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT64_TYPE__ int64_t;
typedef __SIZE_TYPE__ size_t;

typedef struct {
    char name[32];
    uint32_t size;
    uint32_t mode;
    uint32_t uid;
} fs_entry_t;

typedef struct {
    uint64_t x,y,buttons;
} mouse_raw_t;

static inline int64_t syscall0(uint64_t n) {
    int64_t r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n) : "memory"); return r;
}
static inline int64_t syscall1(uint64_t n,uint64_t a1) {
    int64_t r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n),"D"(a1) : "memory"); return r;
}
static inline int64_t syscall2(uint64_t n,uint64_t a1,uint64_t a2) {
    int64_t r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n),"D"(a1),"S"(a2) : "memory"); return r;
}
static inline int64_t syscall3(uint64_t n,uint64_t a1,uint64_t a2,uint64_t a3) {
    int64_t r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n),"D"(a1),"S"(a2),"d"(a3) : "memory"); return r;
}
static inline int64_t syscall4(uint64_t n,uint64_t a1,uint64_t a2,uint64_t a3,uint64_t a4) {
    int64_t r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n),"D"(a1),"S"(a2),"d"(a3),"c"(a4) : "memory"); return r;
}
static inline int64_t syscall5(uint64_t n,uint64_t a1,uint64_t a2,uint64_t a3,uint64_t a4,uint64_t a5) {
    int64_t r; register uint64_t rr8 __asm__("r8") = a5;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n),"D"(a1),"S"(a2),"d"(a3),"c"(a4),"r"(rr8) : "memory");
    return r;
}

static inline int sys_clear(uint8_t c){return (int)syscall1(1,c);}
static inline int sys_rect(int x,int y,int w,int h,uint8_t c){return (int)syscall5(2,(uint64_t)x,(uint64_t)y,(uint64_t)w,(uint64_t)h,c);}
static inline int sys_text(int x,int y,uint8_t c,const char *s,size_t max){return (int)syscall5(3,(uint64_t)x,(uint64_t)y,c,(uint64_t)s,max);}
static inline int sys_getkey(void){return (int)syscall0(4);}
static inline uint64_t sys_mouse(void){return (uint64_t)syscall0(5);}
static inline int sys_fs_list(fs_entry_t *out,size_t max){return (int)syscall2(6,(uint64_t)out,max);}
static inline int sys_fs_read(const char *name,uint8_t *out,size_t max){return (int)syscall3(7,(uint64_t)name,(uint64_t)out,max);}
static inline int sys_exec(int app){return (int)syscall1(8,(uint64_t)app);}
static inline int sys_sleep(uint64_t ms){return (int)syscall1(9,ms);}
static inline uint64_t sys_uptime(void){return (uint64_t)syscall0(10);}
static inline int sys_ipc(const char *msg,char *out,size_t size){return (int)syscall3(11,(uint64_t)msg,(uint64_t)out,size);}
static inline int sys_sysinfo(uint64_t *out){return (int)syscall1(12,(uint64_t)out);}
static inline int sys_reboot(void){return (int)syscall0(13);}
static inline void sys_exit(void){(void)syscall0(14);}

static size_t ulen(const char *s){size_t n=0;while(s[n])n++;return n;}
static int ueq(const char *a,const char *b){size_t i=0;while(a[i]&&b[i]&&a[i]==b[i])i++;return a[i]==0&&b[i]==0;}
static int ustarts(const char *s,const char *p){size_t i=0;while(p[i]){if(s[i]!=p[i])return 0;i++;}return 1;}
static void ucopy(char *d,const char *s,size_t n){for(size_t i=0;i<n;i++)d[i]=s[i];}

static void utoa10(uint64_t v,char *out,size_t cap){
    static const uint64_t pows[] = {10000000000000000000ULL,1000000000000000000ULL,100000000000000000ULL,10000000000000000ULL,1000000000000000ULL,100000000000000ULL,10000000000000ULL,1000000000000ULL,100000000000ULL,10000000000ULL,1000000000ULL,100000000ULL,10000000ULL,1000000ULL,100000ULL,10000ULL,1000ULL,100ULL,10ULL,1ULL};
    size_t at=0;int started=0;
    for(size_t i=0;i<20 && at+1<cap;i++){
        uint64_t p=pows[i]; uint8_t d=0; while(v>=p){v-=p;d++;}
        if(d||started||p==1){out[at++]=(char)('0'+d);started=1;}
    }
    out[at]=0;
}

void desktop_draw(const char *title);
void desktop_cursor(void);
int main(void);

#endif
