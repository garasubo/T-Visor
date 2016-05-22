#include "basic.h"
#include "type.h"
#include "port.h"
#include "debug.h"
#include "gic.h"
#include "hcr.h"

void gic_init( void ){
	// 全優先度の割込みを許す
	OUTW(GICC_PMR, 0xff-7);

	// 割込みを許可
	OUTW(GICC_CTRL, 0x0201);
	OUTW(GICD_CTRL, 0x03);
	Asm("cpsie aif");
}

void gic_virtual_init( void ){
	OUTW(GICC_PMR, 0xff-7);

	// 割込みを許可
	OUTW(GICC_CTRL, 0x0201);
	OUTW(GICD_CTRL, 0x03);
    OUTW(GICH_HCR, 0x01);

    UW i;
    for(i=0;i<PPI_ID_MAX;i++) gic_enable_int(i,0);
}

void gic_force_int( UW intvec )
{
	UW tmp;
	if(intvec >= 32) return;
	OUTB(GICD_SPENDSGIR(intvec >> 2)+(intvec & 0x3), 1);
	Asm("mrs %0, cpsr":"=r"(tmp));
}



void gic_enable_int( UW intvec, UB pri)
{
	UW n = intvec >> 5;
	UW m = intvec & ((1 << 5)-1);
	OUTW(GICD_ISENABLER(n),1 << m);

	n = intvec >> 2;
	m = intvec & ((1 << 2)-1);
	OUTB(GICD_IPRIORITYR(n)+m, (pri << 3)| (1 << 7));
}

void gic_set_isenabler( UW n, UW value ){
    OUTW(GICD_ISENABLER(n), value);
}

void gic_set_ipriority( UW n, UW value ){
    OUTW(GICD_IPRIORITYR(n), value);
}

void gic_set_icenabler( UW n, UW value ){
    OUTW(GICD_ICENABLER(n), value);
}

/*
 * Disable interrupt
 *	disable interrupt specified by `intvec'
 */
void gic_disable_int( UW intvec )
{
	UW n = intvec >> 5;
	UW m = intvec & ((1 << 5)-1);
	OUTW(GICD_ICENABLER(n),1 << m);
}

UW gic_make_virtual_hardware_interrupt( UW vintvec, UW pintvec, UB pri, bool grp1 ){
    UW mask = 0x90000000;
    mask |= ((UW)(pri&0xf8) << 20)| ((pintvec&0x1ff) << 10) |(vintvec&(0x1ff)) | ((UW) grp1 << 30);
    return mask;
}

UW gic_make_virtual_software_interrupt( UW vintvec, UB pri, bool grp1 ){
    UW mask = 0x10000000;
    mask |= ((UW)(pri&0xf8) << 20)|(vintvec&(0x1ff)) | ((UW) grp1 << 30);
    return mask;
}

UW gic_make_virtual_software_sgi( UW vintvec, UB cpu_id, UB pri, bool grp1 ){
    UW mask = 0x10000000;
    mask |= ((UW)(pri&0xf8) << 20)|(vintvec&(0x1ff)) | ((UW) grp1 << 30) | ((UW)cpu_id << 10);
    return mask;
}

UW gic_read_lr( UB n ){
    return INW(GICH_LR(n));
}

UB gic_lr_read_pri( UW lr_value ){
    return (lr_value&(0xf8 << 20)) >> 20;
}

UHW gic_lr_read_vid( UW lr_value ){
    return lr_value&0x1ff;
}

void gic_write_lr( UB n, UW mask ){
    OUTW(GICH_LR(n),mask);
}

void gic_set_np_int( void ){
    OUTW(GICH_HCR, INW(GICH_HCR)|(1 << 3));
}

void gic_clear_np_int( void ){
    OUTW(GICH_HCR, INW(GICH_HCR)&~(1 << 3));
}

