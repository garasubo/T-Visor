#include "basic.h"
#include "type.h"
#include "debug.h"
#include "memory_define.h"
#include "memory_manage.h"
#include "page_table.h"

#define TABLE_SIZE 14

extern volatile void *_page_table_start;
extern volatile void *_page_table_end;

static UW table_max_num;
static volatile UW page_table;
static UW table_count;

extern void EnableHMMU(void);
extern void EnableVMMU(void);

static UW page_table_get_new_table(void){
    if(table_count>=table_max_num){
        tv_abort("!!! cannot get page !!!\n");
        return 0;
    }
    UW ret = page_table+(1 << TABLE_SIZE)*table_count;
    table_count++;
    return ret;
}


void page_table_init(void){
    page_table = (volatile UW)&_page_table_start;
    table_max_num = (UW)(&_page_table_end-&_page_table_start) >> TABLE_SIZE;
    table_count = 0;
    //tv_puts("Hyper\n");
    UW reg1 = 0, reg2 = 0;
    Asm("mrc p15, 4, %0, c2, c1, 2":"=r"(reg1));  // read VTCR
    UW mask = ~0x00003fdf;
    reg1 &= mask;
    reg1 |= 0x00000540;     // No-share, Inner/Outer Cachable WA/WB, T0SZ=0, SL0=1(first level)
    Asm("mcr p15, 4, %0, c2, c1, 2"::"r"(reg1));

    reg1 = page_table;
    Asm("mcrr p15, 6, %0, %1, c2"::"r"(reg1),"r"(reg2)); // write VTTBR


    // enable MMU
    EnableVMMU();



}

static void page_table_add_third_level(UD *second_table, int jdx){
    int k;
    if(second_table[jdx]==0){
        second_table[jdx] = page_table_get_new_table()|0x3;
    } else if((second_table[jdx]&0x3)==0x1){
        // block to table
        UD st = second_table[jdx]|0x2;
        second_table[jdx] = page_table_get_new_table()|0x3;
        UD *third_table = (UD *)(UW)(second_table[jdx]&0xfffffffc);
        for(k=0;k<0x200;k++){
            third_table[k] = st + 0x1000*k;
        }
    }
}

void page_table_add_pages(void *table, memory_area_t *mem_area, int num){
    UD *initial_table = (UD *)table;
    UW i;
    for(i=0; i<num; i++){
        UW start = mem_area[i].virt;
        UW start_p = mem_area[i].start;
        UW end = mem_area[i].end-mem_area[i].start+mem_area[i].virt;
        UD attr = ((UD)mem_area[i].hi_attr << 32)|mem_area[i].lo_attr;

        UW idx = start >> 30;
        UW idx_end = end >> 30;
        UW jdx = (start&0x3fffffff) >> 21;
        UW jdx_end = (end&0x3fffffff) >> 21;
        UW kdx = (start&0x001fffff) >> 12;
        UW kdx_end = (end&0x001fffff) >> 12;

        for(;idx<idx_end;idx++){
            UD* second_table = (UD *)(UW)(initial_table[idx]&0xfffffffc);
            if(kdx!=0){
                page_table_add_third_level(second_table, jdx);
                UD *third_table = (UD *)(UW)(second_table[jdx]&0xfffffffc);
                for(;kdx<0x200;kdx++){
                    third_table[kdx] = attr | start_p | 0x3;
                    start_p += 0x1000;
                }
                jdx++;
                kdx = 0;
            }
            for(;jdx<0x200;jdx++){
                second_table[jdx] = attr | start_p | 0x1;
                start_p += 0x200000;

            }
            jdx = 0;
        }
        UD* second_table = (UD *)(UW)(initial_table[idx]&0xfffffffc);
        if(kdx!=0&&jdx<jdx_end){
            page_table_add_third_level(second_table, jdx);
            UD *third_table = (UD *)(UW)(second_table[jdx]&0xfffffffc);
            for(;kdx<0x200;kdx++){
                third_table[kdx] = attr | start_p | 0x3;
                start_p += 0x1000;
            }
            jdx++;
            kdx = 0;
        }
        for(;jdx<jdx_end;jdx++){
            second_table[jdx] = attr | start_p | 0x1;
            start_p += 0x200000;
        }
        if(kdx<kdx_end){
            page_table_add_third_level(second_table, jdx);
            UD *third_table = (UD *)(UW)(second_table[jdx]&0xfffffffc);
            for(;kdx<kdx_end;kdx++){
                third_table[kdx] = attr | start_p | 0x3;
                start_p += 0x1000;
            }
        }
    }

}

UD* page_table_setup(memory_area_t *mem_area, int num){
    UW i;
    if(table_count%2==1) table_count++;
    UD *initial_table = (UD *)page_table_get_new_table();
    table_count++;
    for(i=0;i<4;i++){
        initial_table[i] = page_table_get_new_table()|0x3;
    }

    page_table_add_pages((void *)initial_table, mem_area, num);

    return initial_table;
}


