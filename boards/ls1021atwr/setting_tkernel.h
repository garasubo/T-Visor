#ifndef __INCLUDED_SETTING_TKERNEL_H__
#define __INCLUDED_SETTING_TKERNEL_H__

#include "page_table.h"

#define TK_START_ADDR 0x82000000

extern memory_area_t TK_MEMORY_AREAS_1[];
extern memory_area_t TK_MEMORY_AREAS_2[];
extern UW TK_MEMORY_AREA_NUM_1;
extern UW TK_MEMORY_AREA_NUM_2;
#endif
