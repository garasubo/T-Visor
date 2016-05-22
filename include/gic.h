#ifndef __INCLUDED_GIC_H__
#define __INCLUDED_GIC_H__

#include "board.h"

#define GICD_CTRL (GICD_BASE_ADDR+0x000)
#define GICD_TYPER (GICD_BASE_ADDR+0x004)
#define GICD_IIDR (GICD_BASE_ADDR+0x008)
#define GICD_IGROUPR(x) (GICD_BASE_ADDR+(0x080+0x004*(x)))
#define GICD_ISENABLER(x) (GICD_BASE_ADDR+(0x100+0x004*(x)))
#define GICD_ICENABLER(x) (GICD_BASE_ADDR+(0x180+0x004*(x)))
#define GICD_ISPENDER(x) (GICD_BASE_ADDR+(0x200+0x004*(x)))
#define GICD_ICPENDER(x) (GICD_BASE_ADDR+(0x280+0x004*(x)))
#define GICD_ISACTIVER(x) (GICD_BASE_ADDR+(0x300+0x004*(x)))
#define GICD_ICACTIVER(x) (GICD_BASE_ADDR+(0x380+0x004*(x)))
#define GICD_IPRIORITYR(x) (GICD_BASE_ADDR+(0x400+0x004*(x)))
#define GICD_ITARGETSR(x) (GICD_BASE_ADDR+(0x800+0x004*(x)))
#define GICD_ICFGR(x) (GICD_BASE_ADDR+(0xc00+0x004*(x)))
#define GICD_PPISR (GICD_BASE_ADDR+0xd00)
#define GICD_SPISR(x) (GICD_BASE_ADDR+(0xd04+0x004*(x)))
#define GICD_NSACR(x) (GICD_BASE_ADDR+(0xe00+0x004*(x)))
#define GICD_SGIR (GICD_BASE_ADDR+0xf00)
#define GICD_CPENDSGIR(x) (GICD_BASE_ADDR+(0xf10+0x004*(x)))
#define GICD_SPENDSGIR(x) (GICD_BASE_ADDR+(0xf20+0x004*(x)))

#define GICC_CTRL (GICC_BASE_ADDR+0x0000)
#define GICC_PMR (GICC_BASE_ADDR+0x0004)
#define GICC_BPR (GICC_BASE_ADDR+0x0008)
#define GICC_IAR (GICC_BASE_ADDR+0x000c)
#define GICC_EOIR (GICC_BASE_ADDR+0x0010)
#define GICC_RPR (GICC_BASE_ADDR+0x0014)
#define GICC_HPPIR (GICC_BASE_ADDR+0x0018)
#define GICC_ABPR (GICC_BASE_ADDR+0x001c)
#define GICC_AIAR (GICC_BASE_ADDR+0x0020)
#define GICC_AEOIR (GICC_BASE_ADDR+0x0024)
#define GICC_AHPPIR (GICC_BASE_ADDR+0x0028)
#define GICC_APR(x) (GICC_BASE_ADDR+(0x00d0+0x0004*(x)))
#define GICC_NSAPR(x) (GICC_BASE_ADDR+(0x00e0+0x0004*(x)))
#define GICC_IIDR (GICC_BASE_ADDR+0x00fc)
#define GICC_DIR (GICC_BASE_ADDR+0x1000)

#define GICH_HCR (GICH_BASE_ADDR+0x0000)
#define GICH_VTR (GICH_BASE_ADDR+0x0004)
#define GICH_VMCR (GICH_BASE_ADDR+0x0008)
#define GICH_MISR (GICH_BASE_ADDR+0x0010)
#define GICH_EISR0 (GICH_BASE_ADDR+0x0020)
#define GICH_EISR1 (GICH_BASE_ADDR+0x0024)
#define GICH_ELSR0 (GICH_BASE_ADDR+0x0030)
#define GICH_ELSR1 (GICH_BASE_ADDR+0x0034)
#define GICH_APR (GICH_BASE_ADDR+0x00f0)
#define GICH_LR(x) (GICH_BASE_ADDR+0x0100+0x4*(x))
#define GICH_LR_NUM 4

#define SGI_ID_MAX 16
#define PPI_ID_MAX 32
#define SPI_ID_MAX 512

#ifndef __in_asm__

void gic_init( void );
void gic_virtual_init( void );
void gic_force_int( UW intvec );
void gic_enable_int( UW intvec, UB pri);
void gic_disable_int( UW intvec );
UW gic_make_virtual_hardware_interrupt( UW vintvec, UW pintvec, UB pri, bool grp1 );
UW gic_make_virtual_software_interrupt( UW vintvec, UB pri, bool grp1 );
UW gic_make_virtual_software_sgi( UW vintvec, UB cpu_id, UB pri, bool grp1 );
UW gic_read_lr( UB n );
void gic_write_lr( UB n, UW mask );
UB gic_lr_read_pri( UW lr_value );
UHW gic_lr_read_vid( UW lr_value );
void gic_set_np_int( void );
void gic_clear_np_int( void );
void gic_set_isenabler( UW n, UW value );
void gic_set_icenabler( UW n, UW value );
void gic_set_ipriority( UW n, UW value );

#endif // __in_asm__

#endif
