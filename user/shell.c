#include "user.h"

#define ROWS 14
#define COLS 47
static char lines[ROWS][COLS+1];
static char input[41];
static size_t input_len;
static fs_entry_t files[12];
static char file_buf[2049];

static void clear_lines(void){for(int r=0;r<ROWS;r++){for(int c=0;c<COLS;c++)lines[r][c]=' ';lines[r][COLS]=0;}}
static void copy_line(int row,const char *s){for(int c=0;c<COLS;c++)lines[row][c]=s[c]?s[c]:' ';lines[row][COLS]=0;}
static void add_line(const char *s){for(int r=1;r<ROWS;r++)copy_line(r-1,lines[r]);copy_line(ROWS-2,s);}
static void add_u64(const char *prefix,uint64_t v){char b[21];char out[COLS+1];size_t p=0;while(prefix[p]&&p<COLS)out[p]=prefix[p],p++;utoa10(v,b,sizeof(b));for(size_t i=0;b[i]&&p<COLS;i++)out[p++]=b[i];out[p]=0;add_line(out);}
static int starts_arg(const char *cmd,const char *prefix,const char **arg){size_t n=ulen(prefix);if(!ustarts(cmd,prefix))return 0;if(cmd[n]==' ') { *arg=&cmd[n+1]; return 1;}return cmd[n]==0;}

static void render(void){
    sys_clear(1);
    sys_rect(7,20,306,159,8);
    sys_rect(10,23,300,153,0);
    sys_rect(10,23,300,14,3);
    sys_text(16,27,15,"NOVA TERMINAL",32);
    sys_rect(14,41,292,128,0);
    for(int r=0;r<ROWS-1;r++)sys_text(16,44+r*9,15,lines[r],COLS);
    sys_text(16,44+(ROWS-1)*9,14,"NOVA> ",7);
    sys_text(58,44+(ROWS-1)*9,15,input,sizeof(input));
    char clk[32],sec[21];
    utoa10(sys_uptime()/100,sec,sizeof(sec));
    size_t p=0;clk[p++]='U';clk[p++]='P';clk[p++]=' ';for(size_t i=0;sec[i]&&p<30;i++)clk[p++]=sec[i];clk[p]=0;
    sys_rect(244,25,60,10,3);sys_text(248,27,15,clk,30);
    uint64_t m=sys_mouse();int x=(int)(m&0xffff),y=(int)((m>>16)&0xffff);
    sys_rect(x,y,2,2,15);
}

static void cmd_help(void){add_line("COMMANDS:");add_line("HELP ABOUT CLEAR MEM UPTIME WHOAMI");add_line("LS CAT <FILE> FILES SETTINGS NET IPC");add_line("RUN PAINT  QUIT");}
static void cmd_about(void){add_line("NOVA OS FIRST BUILD");add_line("CUSTOM X86_64 / HYBRID CORE");add_line("RING3 APPS + SYSCALL ABI");add_line("BIOS / VGA13 / PS2 / IDE");}
static void cmd_files(void){int n=sys_fs_list(files,12);if(n<0){add_line("FS OFFLINE OR UNAVAILABLE");return;}add_line("FILE MANAGER");for(int i=0;i<n;i++){char line[COLS+1];size_t p=0;for(int j=0;j<28&&files[i].name[j]&&p<COLS;j++)line[p++]=files[i].name[j];if(p<COLS)line[p++]=' ';if(p<COLS)line[p++]=' ';char b[21];utoa10(files[i].size,b,sizeof(b));for(size_t j=0;b[j]&&p<COLS;j++)line[p++]=b[j];line[p]=0;add_line(line);}}
static void cmd_ls(void){int n=sys_fs_list(files,12);if(n<0){add_line("NO NOVA-FS");return;}for(int i=0;i<n;i++)add_line(files[i].name);}
static void cmd_cat(const char *name){if(!name||!name[0]){add_line("USAGE: CAT <FILE>");return;}int n=sys_fs_read(name,(uint8_t*)file_buf,sizeof(file_buf)-1);if(n<0){add_line("FILE NOT FOUND");return;}file_buf[n]=0;size_t start=0;for(int guard=0;guard<12&&start<(size_t)n;guard++){size_t end=start;while(end<(size_t)n&&file_buf[end]!='\n'&&end-start<COLS-1)end++;char line[COLS+1];size_t k=0;while(start<end)line[k++]=file_buf[start++];line[k]=0;add_line(line);while(start<(size_t)n&&(file_buf[start]=='\n'||file_buf[start]=='\r'))start++;}}
static void cmd_settings(void){uint64_t s[5];if(sys_sysinfo(s)<0){add_line("SYSINFO DENIED");return;}add_u64("MEM BYTES ",s[0]);add_line("DISPLAY 320X200 VGA");add_line("INPUT PS2 KBD + MOUSE");add_u64("UID ",s[2]);add_u64("PID ",s[3]);}
static void cmd_mem(void){uint64_t s[5];if(sys_sysinfo(s)==0)add_u64("MEMORY ",s[0]);else add_line("MEMORY ERROR");}
static void cmd_uptime(void){add_u64("TICKS ",sys_uptime());}
static void cmd_ipc(void){char out[64];int n=sys_ipc("hello from shell",out,sizeof(out));if(n<0){add_line("IPC ERROR");return;}add_line("IPC ECHO:");add_line(out);}

static void run_command(void){
    input[input_len]=0;
    if(!input_len)return;
    if(ueq(input,"help")){add_line("NOVA> HELP");cmd_help();}
    else if(ueq(input,"about")){add_line("NOVA> ABOUT");cmd_about();}
    else if(ueq(input,"clear")){clear_lines();}
    else if(ueq(input,"mem")){cmd_mem();}
    else if(ueq(input,"uptime")){cmd_uptime();}
    else if(ueq(input,"whoami")){add_line("NOVA USER 1000");}
    else if(ueq(input,"ls")){cmd_ls();}
    else if(ueq(input,"files")){cmd_files();}
    else if(ueq(input,"settings")){cmd_settings();}
    else if(ueq(input,"net")){add_line("NETWORK: NO NIC DRIVER ATTACHED");add_line("STATE: SAFE / UNSUPPORTED");}
    else if(ueq(input,"ipc")){cmd_ipc();}
    else if(ueq(input,"quit")){sys_exit();}
    else {const char *arg=0;if(starts_arg(input,"cat",&arg))cmd_cat(arg);else if(ueq(input,"run paint")){sys_exec(1);}}
    input_len=0;input[0]=0;
}

int main(void){
    clear_lines();input_len=0;input[0]=0;
    add_line("NOVA OS READY");add_line("RING3 SHELL / UID 1000");add_line("TYPE HELP");
    for(;;){
        int k=sys_getkey();
        if(k>=32&&k<127){if(input_len<sizeof(input)-1)input[input_len++]=(char)k;}
        else if(k=='\n'){run_command();}
        else if(k==8&&input_len){input[--input_len]=0;}
        render();sys_sleep(20);
    }
}
