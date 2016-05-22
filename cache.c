#include "basic.h"
#include "type.h"

void flush_cache(void){
    UW r0,clidr;
    W level, way, set;


    Asm("mrc p15, 1, %0, c0, c0, 1":"=r"(clidr));

    for(level = 0; level < 7; level++) {
        UW cache_type = (clidr >> (level*3))&7;
        if(cache_type < 2) continue;

        r0 = level << 1;
        Asm("mcr p15, 2, %0, c0, c0, 0"::"r"(r0));

        Asm("mrc p15, 1, %0, c0, c0, 0":"=r"(r0));

        UW log_line = (r0&7) +4;
        UW ass_num = ((r0 >> 3)&0x3ff) + 1;
        UW set_num = ((r0 >> 13)&0x7fff) + 1;

        UW log_ass = -1;
        UW tmp = ass_num;
        while(tmp) {
            tmp >>= 1;
            log_ass++;
        }
        if(ass_num & (ass_num-1)) log_ass++;

        for(way = ass_num-1; way >= 0; way--) for(set = set_num-1; set >= 0; set--){
            r0 = (way << (32-log_ass))|(set << log_line)|(level << 1);
            Asm("mcr p15, 0, %0, c7, c14, 2"::"r"(r0));
            Asm("DSB");
        }
        Asm("DSB");
    }
    Asm("DSB");

    Asm("mcr p15, 0, %0, c7, c5, 0"::"r"(0));
    Asm("mcr p15, 0, %0, c7, c5, 6"::"r"(0));
    Asm("DSB");
    Asm("ISB");

}

void flush_cache_by_va(void *addr, UW len){
    Asm("mcr p15, 0, %0, c7, c5, 0"::"r"(0));
    Asm("mcr p15, 0, %0, c7, c5, 6"::"r"(0));
    Asm("DSB");
    Asm("ISB");

    UW ctr;
    Asm("mrc p15, 0, %0, c0, c0, 1":"=r"(ctr));
    UW line_size = 1 << (ctr&0xf);
    void *end = addr+len;
    void *tmp = (void *)((UW)addr & ~(line_size - 1));
    while(tmp < end){
        Asm("mcr p15, 0, %0, c7, c14, 1":: "r"(tmp));
        Asm("mcr p15, 0, %0, c7, c11, 1":: "r"(tmp));
        Asm("mcr p15, 0, %0, c7, c5,  1":: "r"(tmp));
        tmp += line_size;
    }
    Asm("mcr p15, 0, %0, c7, c5, 0"::"r"(0));
    Asm("mcr p15, 0, %0, c7, c5, 6":: "r"(0));
    Asm("DSB");
    Asm("ISB");
}
