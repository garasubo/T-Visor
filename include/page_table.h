#ifndef __INCLUDED_PAGE_TABLE_H__
#define __INCLUDED_PAGE_TABLE_H__

#include "basic.h"
#include "type.h"

typedef struct {
    UW start;
    UW end;
    UW virt;
    UW lo_attr;
    UW hi_attr;
} memory_area_t;

typedef struct {
    memory_area_t *mem_area;
    UW area_num;
} T_CPT;

void page_table_init(void);
void page_table_add_pages(void *table, memory_area_t *mem_area, int num);
UD* page_table_setup(memory_area_t *mem_area, int num);

#endif
