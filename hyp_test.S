

#include "cpsr.h"
#include "sctlr.h"
#include "hcr.h"
#include "scr.h"

	.text

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
	b	default_entry		// 04 : undefined instruction exception
	b	default_entry		// 08 : supurvisor call (SVC)
	b	default_entry		// 0C : prefetch abort
	b	default_entry		// 10 : data abort
	nop				        // 14 : (reserved)
	b	default_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000040
hyper_vector:
	// hyper vector table
	nop						// 00 : reset
	b	default_entry		// 04 : undefined instruction exception
	b	default_entry		// 08 : hyper call from hyper mode
	b	default_entry		// 0C : prefetch abort
	b	default_entry		// 10 : data abort
	b	hyper_entry			// 14 : hyper call from non-secure world (HVC)
	b	default_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000060
monitor_vector:
	// monitor vector table
	nop						// 00 : (reserved)
	nop						// 04 : (reserved)
	b	hyper_init			// 08 : monitor call
	b	default_entry		// 0C : prefetch abort
	b	default_entry		// 10 : data abort
	nop						// 14 : (reserved)
	b	default_entry		// 18 : interrupt
	b	default_entry		// 1C : fast interrupt
	.org	0x00000080

startup_entry:
	// set cpsr
	mov r0, #(PSM_SVC | CPSR_I | CPSR_F)
	msr cpsr, r0

	// set vector base address
	ldr	r1, =secure_vector
	mcr p15, 0, r1, c12, c0, 0	// VBAR
	ldr	r1, =monitor_vector
	mcr p15, 0, r1, c12, c0, 1	// MVBAR

	// set actlr
	mrc p15, 0, r0, c1, c0, 1
	orr r0, r0, #(1 <<  6)
	mcr p15, 0, r0, c1, c0, 1

	// move to monitor mode and restart as non-secure world
	smc #0

  nonsecure_init:
	ldr	r1, =_start
	mcr p15, 0, r1, c12, c0, 0	// VBAR
	// set cpsr
	mrs r0, cpsr
	mov r0, #(PSM_SVC | CPSR_I | CPSR_F)
	msr cpsr, r0

	// hyper call test
	hvc #0

	b default_entry


hyper_init:
	mrc p15, 0, r0, c1, c1, 0	//read scr
	orr r0, r0, #1	// set NS bit
	orr r0, r0, #SCR_HCE	// set Hyper call enable bit
	mcr p15, 0, r0, c1, c1, 0	//set scr
	mrs r0, cpsr



	// set VTCR
	mrc p15, 4, r0, c2, c1, 2
	ldr r1, =0x00003fdf
	bic r0, r0, r1
	ldr r1, =0x00000500
	orr r0, r0, r1
	mcr p15, 4, r0, c2, c1, 2

	ldr r0, =0x80000000
	mov r1, #0
	mcrr p15, 6, r0, r1, c2

	mov r2, #0
	orr r2, r2, #0x400
	orr r2, r2, #0xc0
	ldr r3, =0x40000000
  page_table_loop:
	str r2, [r0]
	str r1, [r0, #4]
	adds r2, r2, r3
	bcc page_table_loop
	
	// flush cache and tlb
	DSB
	mcr p15, 4, r0, c8, c7, 0	// invalid all hyp tlb entries
	mcr p15, 0, r0, c8, c7, 0	// invalid all tlb entries
	mcr p15, 0, r0, c7, c5, 6	// invalid branch predictor
	DSB
	ISB
	
	ldr r0, =_stack_start+0x100
	DSB
	ISB
	nop
	mov sp, r0
	DSB
	ISB
	nop

	mov r1, sp


	// FlushCache
	push {r4,r5}

	mov r0, lr
	nop
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0

	mrc	p15, 1, r0, c0, c0, 0
	ldr	r3, =0x3ff
	and	r1, r3, r0, LSR #3		//r1 = num of ways
	clz	r5, r1
	add	r1, r1, #1

	ldr	r3, =0x7fff
	and	r0, r3, r0, LSR #13		//r0 = num of sets

	mov	r4, #0
  l_flush_dcache1:
	mov	r3, #0
  l_flush_dcache2:
	mov	r2, r4
	lsl	r2, r5
	orr	r2, r3, LSL #5
	mcr	p15, 0, r2, cr7, c14, 2		// data cache is written back,
	add	r3, r3, #1
	cmp	r0, r3
	bne	l_flush_dcache2
	add	r4, r4, #1
	cmp	r1, r4
	bne	l_flush_dcache1
	DSB		// Drain Write Buffer

	pop {r4, r5}

	// FlushCache End

	mrc p15, 4, r0, c1, c1, 0
	orr r0, r0, #HCR_VM
	mcr p15, 4, r0, c1, c1, 0
	DSB
	ISB

	ldr r0, =hyper_vector
	mcr p15, 4, r0, c12, c0, 0	// HVBAR
	movs pc, lr

hyper_entry:
	ldr sp, =_stack_start+0x50
	eret
default_entry:
	nop
	mrs r0, cpsr
	nop
	wfi
	b default_entry
