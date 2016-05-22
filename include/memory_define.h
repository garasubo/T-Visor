#ifndef __INCLUDED_MEMORY_DEFINE_H__
#define __INCLUDED_MEMORY_DEFINE_H__

#include "type.h"
#include "page_table.h"



#define MEM_NO_EXEC (1 << 22)
#define MEM_AREA_VSTOR (0x0)
#define MEM_AREA_VDEV (0x4)
#define MEM_AREA_VNORMAL (0x3c)
#define MEM_AREA_HDEV (0x0)
#define MEM_AREA_HNORMAL (0x1c)
#define MEM_AREA_VNO (0x00)
#define MEM_AREA_VRO (0x40)
#define MEM_AREA_VWO (0x80)
#define MEM_AREA_VRW (0xc0)

#define MAIR0 0x04040404;
#define MAIR1 0xffffffff;

extern memory_area_t MemArea[];
extern memory_area_t *VMemArea[];
extern UW MemAreaNum;

#endif
