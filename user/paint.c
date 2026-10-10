#include "user.h"

int main(void){
    uint8_t color=10; uint64_t last_buttons=0;
    for(;;){
        sys_clear(2);
        sys_rect(8,8,304,24,3);
        sys_text(16,16,15,"NOVA PAINT",32);
        sys_text(120,16,15,"R RED  G GREEN  B BLUE  Q QUIT",40);
        sys_rect(16,42,288,142,0);
        uint64_t m=sys_mouse();int x=(int)(m&0xffff), y=(int)((m>>16)&0xffff);uint64_t buttons=(m>>32)&7;
        if ((buttons&1) && !(last_buttons&1)) sys_rect(x-3,y-3,7,7,color);
        last_buttons=buttons;
        sys_rect(x,y,2,2,15);
        int k=sys_getkey();
        if(k=='r'||k=='R')color=4;
        else if(k=='g'||k=='G')color=12;
        else if(k=='b'||k=='B')color=10;
        else if(k=='q'||k=='Q')sys_exec(0);
        sys_sleep(20);
    }
}
