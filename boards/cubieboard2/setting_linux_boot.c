#include "memory_define.h"
#include "page_table.h"
#include "board.h"
#include "setting_linux_boot.h"

memory_area_t MemArea[] = {
    {0x00000000, 0x01c01000, 0x00000000, MEM_AREA_VDEV | MEM_AREA_VRW | (1 << 10), 0 },
    {0x01c01000, 0x1c020000, 0x01c01000, MEM_AREA_VDEV | MEM_AREA_VNO | (1 << 10), MEM_NO_EXEC}, // DDR
    {0x01c02000, 0x40000000, 0x01c02000, MEM_AREA_VDEV | MEM_AREA_VRW | (1 << 10), MEM_NO_EXEC},
    {GICD_BASE_ADDR, GICD_BASE_ADDR+0x1000, GICD_BASE_ADDR, MEM_AREA_VDEV | MEM_AREA_VNO | (1 << 10), MEM_NO_EXEC},
    {GICV_BASE_ADDR, GICV_BASE_ADDR+0x2000, GICC_BASE_ADDR, MEM_AREA_VDEV | MEM_AREA_VRW | (1 << 10), MEM_NO_EXEC},
    {0x40000000, 0x60000000, 0x40000000, MEM_AREA_VNORMAL | MEM_AREA_VRW | (1 << 10), 0 },
    //{0x01c28000, 0x01c29000, 0x01c28000, MEM_AREA_VDEV | MEM_AREA_VNO | (1 << 10), 0},
};

UW MemAreaNum = sizeof(MemArea)/sizeof(MemArea[0]);
