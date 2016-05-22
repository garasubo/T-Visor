
#define __in_asm__

#include "cpsr.h"
#include "sctlr.h"
#include "scr.h"
#include "hcr.h"
#include "gic.h"
#include "hcptr.h"

#define MAIR0 0x04040404;
#define MAIR1 0xffffffff;



	.text
	.globl	sleep_vector

	.section .eitbra , "ax"
	// non-secure vector table
    .org	0x00000000
	b	startup_entry		// 00 : reset
	b	default_entry		// 04 : undefined instruction exception
	b	default_entry		// 08 : supervisor call (SVC)
	b	default_entry		// 0C : prefetch abort
	b	default_entry		// 10 : data abort
	nop				        // 14 : (reserved)
	b	default_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000020
	// secure vector table
secure_vector:
	b	startup_entry		// 00 : reset
	b	abort_entry		// 04 : undefined instruction exception
	b	default_entry		// 08 : supurvisor call (SVC)
	b	abort_entry		// 0C : prefetch abort
	b	abort_entry		// 10 : data abort
	nop				        // 14 : (reserved)
	b	default_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000040
hyper_vector:
	// hyper vector table
	nop						// 00 : reset
	b	hyp_undef_entry		// 04 : undefined instruction exception
	b	default_entry		// 08 : hyper call from hyper mode
	b	hyp_undef_entry		// 0C : prefetch abort
	b	hyp_undef_entry		// 10 : data abort
	b	hyper_entry			// 14 : hyper call from non-secure world (HVC)
	b	hyp_irq_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000060
monitor_vector:
	// monitor vector table
	nop						// 00 : (reserved)
	nop						// 04 : (reserved)
	b	hyper_init			// 08 : monitor call
	b	abort_entry		// 0C : prefetch abort
	b	abort_entry		// 10 : data abort
	nop						// 14 : (reserved)
	b	default_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000080
sleep_vector:
	// hyper vector table
	nop						// 00 : reset
	b	hyp_undef_entry		// 04 : undefined instruction exception
	b	default_entry		// 08 : hyper call from hyper mode
	b	hyp_undef_entry		// 0C : prefetch abort
	b	hyp_undef_entry		// 10 : data abort
	b	hyp_undef_entry			// 14 : hyper call from non-secure world (HVC)
	b	hyp_wakeup_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x000000a0

startup_entry:
	// set cpsr
	mov r0, #(PSM_SVC | CPSR_I | CPSR_F)
	msr cpsr, r0

	// move to non-secure state
	mrc p15, 0, r0, c1, c1, 0	//read scr
	and r1, r0, #1
	cmp r1, #0
	bne default_entry

	// disable timer
	ldr r0, =0xffffffff
	mcrr p15, 2, r0, r1, c14
	mov r0, #0x2
	mcr p15, 0, r0, c14, c2, 1

	// set vector base address
	ldr	r1, =secure_vector
	mcr p15, 0, r1, c12, c0, 0	// VBAR
	ldr	r1, =monitor_vector
	mcr p15, 0, r1, c12, c0, 1	// MVBAR

	// set actlr
	mrc p15, 0, r0, c1, c0, 1
	orr r0, r0, #(1 <<  6)
	mcr p15, 0, r0, c1, c0, 1


	ldr sp, =_hyp_stack_end
	
	// serial init
	bl serial_init

	mov r0, #0
	bl tv_display_boot_process
	
	// setup gic
	//bl gic_test
	ldr r0, =GICC_PMR
	mov r1, #(0xff-7)
	str r1, [r0]
	mov r2, #0
	ldr r3, =SPI_ID_MAX
	ldr r0, =GICD_IGROUPR(0)
	ldr r1, =0xffffffff
  gic_loop:
	str r1, [r0, r2, lsr #3]
	add r2, r2, #32
	cmp r2, r3
	blo gic_loop

	ldr r0, =GICD_ITARGETSR(0)
	mov r2, #32
  gic_loop_2:
	str r1, [r0, r2]
	add r2, r2, #4
	cmp r2, r3
	blo gic_loop_2

	mov r0, #1
	bl tv_display_boot_process


	// set nsacr
	mrc p15, 0, r0, c1, c1, 2
	orr r0, r0, #(HCPTR_TCP(10)|HCPTR_TCP(11))
	mcr p15, 0, r0, c1, c1, 2

	mov r0, #2
	bl tv_display_boot_process

	// move to monitor mode and restart as non-secure world
	smc #0

  nonsecure_init:
	// set cpsr
	mrs r0, cpsr
	mov r0, #(PSM_SVC | CPSR_I | CPSR_F)
	msr cpsr, r0


	
	// set sp and lr
	ldr sp, =_stack_end
	mov lr, #0

	mov r0, #5
	bl tv_display_boot_process

	bl flush_cache

	// initialize bss section
	ldr r0, =_bss_start
	ldr r1, =_page_table_end
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
  bss_loop:
	stmia r0!, {r2-r5}
	cmp r0, r1
	blo bss_loop

	mcr p15, 0, r0, c8, c7, 0	// TLB invalidate all

	// VFP and NEON initialization
	// set cpacr
	mrc p15, 0, r0, c1, c0, 2
	orr r0, r0, #(0x3 << 20)
	orr r0, r0, #(0x3 << 22)
	mcr p15, 0, r0, c1, c0, 2
	mov r0, #0x40000000
	vmsr fpexc, r0
	mov r1, #0x0
	vmsr fpscr, r0

	// enable d-cache and branch predictor
	//bl EnableCache

	// disable timer
	ldr r0, =0xffffffff
	mcrr p15, 2, r0, r1, c14
	mov r0, #0x2
	mcr p15, 0, r0, c14, c2, 1
	
	
	// hyper call test
	hvc #0

	b default_entry


hyper_init:
	ldr sp, =_hyp_stack_end
	push {lr}
	mov r0, lr
	bl tv_display_boot_process
	mrc p15, 0, r0, c1, c1, 0	//read scr
	orr r0, r0, #SCR_NS	// set NS bit
	orr r0, r0, #SCR_HCE	// set Hyper call enable bit
	orr r0, r0, #SCR_AW
	orr r0, r0, #SCR_FW
	bic r0, r0, #SCR_SIF
	mcr p15, 0, r0, c1, c1, 0	//set scr
	mov r0, #4
	bl tv_display_boot_process

	mrc p15, 4, r0, c2, c0, 2	// read HTCR
	ldr r1, =0x00003f07
	bic r0, r0, r1
	ldr r1, =0x00000500		// No-share, Inner/Outer Cachable WA/WB, T0SZ=0
	orr r0, r0, r1
	mcr p15, 4, r0, c2, c0, 2	// write HTCR
	mov r0, #5
	bl tv_display_boot_process
	
	ldr r0, =_page_table_start
	mov r1, #0
	mcrr p15, 4, r0, r1, c2	// write HTTBR
	
	// disable hyper MMU
	mrc p15, 4, r0, c1, c0, 0	// read HSCTLR
	bic r0, r0, #SCTLR_M
	bic r0, r0, #SCTLR_WXN
	mcr p15, 4, r0, c1, c0, 0	// write HSCTLR

	// cache disable
	DSB
	ldr r0, =(SCTLR_DEFALT | SCTLR_BEN | SCTLR_AFE | SCTLR_TEX)
	mcr p15, 0, r0, c1, c0, 0	// Write to SCTLR

	mov r0, #6
	bl tv_display_boot_process

	// take exception in hyp mode
	mrc p15, 4, r0, c1, c1, 0
	bic r0, r0, #HCR_AMO
	orr r0, r0, #HCR_IMO
	orr r0, r0, #HCR_FMO
	bic r0, r0, #HCR_TGE
	bic r0, r0, #HCR_PTW
	orr r0, r0, #HCR_TSC
	orr r0, r0, #HCR_TW1
	orr r0, r0, #HCR_TWE
	mcr p15, 4, r0, c1, c1, 0

	mov r0, #7
	bl tv_display_boot_process

	// HDCR
	mrc p15, 4, r0, c1, c1, 1
	bic r0, r0, #(1 << 8)
	mcr p15, 4, r0, c1, c1, 1
	mov r0, #8
	bl tv_display_boot_process

	// permit access to timer
	mrc p15, 4, r0, c14, c1, 0	// write CNTHCTL
	orr r0, r0, #0x3
	mcr p15, 4, r0, c14, c1, 0
	
	mov r0, #9
	bl tv_display_boot_process

	// permit c10, c11 access
	mrc p15, 4, r0, c1, c1, 2
	bic r0, r0, #(1 << 10)
	bic r0, r0, #(1 << 11)
	mcr p15, 4, r0, c1, c1, 2
	
	mov r0, #0xa
	bl tv_display_boot_process

	ldr r0, =MAIR0
	mcr p15, 4, r0, c10, c2, 0
	ldr r0, =MAIR1
	mcr p15, 4, r0, c10, c2, 1

	ldr r0, =hyper_vector
	mcr p15, 4, r0, c12, c0, 0	// HVBAR

	mrs r0, spsr
	bl tv_display_boot_process

	pop {lr}
	movs pc, lr

hyper_entry:
	cpsid aif
	ldr sp, =_hyp_stack_end
	push {r0-r12,r14}
	vmrs r0, fpexc
	mov r2, #0x40000000
	vmsr fpexc, r2
	vmrs r1, fpscr
	vpush {d16-d31}
	vpush {d0-d15}
	push {r0,r1}
	mov r0, sp
	bl vcpu_set_current_sp
	mrc p15, 4, r0, c5, c2, 0	//read HSR
	mov r1, sp
	bl hyp_call
	pop {r0, r1}
	vpop {d0-d15}
	vpop {d16-d31}
	vmsr fpscr, r1
	vmsr fpexc, r0
	pop {r0-r12,r14}
	eret


hyp_irq_entry:
	ldr sp, =_hyp_stack_end
	push {r0-r12,r14}
	vmrs r0, fpexc
	mov r2, #0x40000000
	vmsr fpexc, r2
	vmrs r1, fpscr
	vpush {d16-d31}
	vpush {d0-d15}
	push {r0,r1}
	mov r0, sp
	bl vcpu_set_current_sp
	ldr r1, =GICC_IAR
	ldr r0, [r1]
	bl irq_handler
hyp_irq_entry_end:
	pop {r0, r1}
	vpop {d0-d15}
	vpop {d16-d31}
	vmsr fpscr, r1
	vmsr fpexc, r0
	pop {r0-r12,r14}
	eret

hyp_wakeup_entry:
	cpsid aif
	push {r0-r4, ip, lr}
	vmrs r0, fpexc
	mov r2, #0x40000000
	vmrs r1, fpscr
	vpush {d16-d31}
	vpush {d0-d7}
	push {r0,r1}
	ldr r1, =GICC_IAR
	ldr r0, [r1]
	bl irq_handler
	ldr r0, =hyper_vector
	mcr p15, 4, r0, c12, c0, 0	// HVBAR
	pop {r0, r1}
	vpop {d0-d7}
	vpop {d16-d31}
	vmsr fpscr, r1
	vmsr fpexc, r0
	pop {r0-r4, ip, lr}
	bx lr

abort_entry:
	mov r0, lr
	ldr lr, =_irq_stack_end
	bl tv_message
	b default_entry


default_entry:
	nop
	mrs r0, cpsr
	nop
	wfi
	b default_entry

hyp_undef_entry:
	ldr sp, =_irq_stack_end
	mov r5, lr
	bl tv_enable_print
	mov r0, r5
	bl tv_message
	ldr r0, =0x1234
	bl tv_message
	b default_entry
