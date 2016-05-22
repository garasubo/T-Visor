
#include "sctlr.h"
#include "hcr.h"

	.text
	.balign	4
	.globl	FlushTLB
	.type	FlushTLB, %function
FlushTLB:
	ISB
	DSB
	mcr p15, 4, r0, c8, c7, 0	// invalid all hyp tlb entries
	mcr p15, 4, r0, c8, c7, 4	// invalid all hyp tlb entries
	mcr p15, 0, r0, c7, c5, 6	// invalid branch predictor
	DSB
	ISB
	bx lr

	.text
	.balign	4
	.globl	EnableMMU
	.type	EnableMMU, %function
EnableMMU:
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #SCTLR_M
	mcr p15, 0, r0, c1, c0, 0
	DSB
	ISB
	bx lr


	.text
	.balign	4
	.globl	DisableMMU
	.type	DisableMMU, %function
DisableMMU:
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #SCTLR_M
	mcr p15, 0, r0, c1, c0, 0
	DSB
	ISB
	bx lr

	.text
	.balign	4
	.globl	EnableHMMU
	.type	EnableHMMU, %function
EnableHMMU:
	mrc p15, 4, r0, c1, c0, 0
	orr r0, r0, #SCTLR_M
	mcr p15, 4, r0, c1, c0, 0
	DSB
	ISB
	bx lr

	.text
	.balign	4
	.globl	SetVTTBR
	.type	SetVTTBR, %function
SetVTTBR:
	mcrr p15, 6, r0, r1, c2		// write VTTBR
	bx lr

	.text
	.balign	4
	.globl	SetTTBCR
	.type	SetTTBCR, %function
SetTTBCR:
	mcr p15, 0, r0, c2, c0, 2
	bx lr

	.text
	.balign	4
	.globl	GetTTBCR
	.type	GetTTBCR, %function
GetTTBCR:
	mrc p15, 0, r0, c2, c0, 2
	bx lr

	.text
	.balign	4
	.globl	SetTTBR
	.type	SetTTBR, %function
SetTTBR:
	mcrr p15, 0, r0, r1, c2
	mcrr p15, 1, r2, r3, c2
	bx lr

	.text
	.balign	4
	.globl	GetTTBR0
	.type	GetTTBR0, %function
GetTTBR0:
	mrrc p15, 0, r0, r1, c2
	bx lr

	.text
	.balign	4
	.globl	GetTTBR1
	.type	GetTTBR1, %function
GetTTBR1:
	mrrc p15, 1, r0, r1, c2
	bx lr

	.text
	.balign	4
	.globl	EnableVMMU
	.type	EnableVMMU, %function
EnableVMMU:
	DSB
	push {lr}

	mcr p15, 4, r0, c8, c7, 0	// invalid all hyp tlb entries
	mcr p15, 0, r0, c8, c7, 0	// invalid all tlb entries
	mcr p15, 0, r0, c7, c5, 6	// invalid branch predictor
	DSB
	ISB

	bl flush_cache
	bl FlushTLB

	mrc p15, 4, r0, c1, c1, 0
	orr r0, r0, #HCR_VM
	//orr r0, r0, #HCR_DC
	mcr p15, 4, r0, c1, c1, 0
	DSB
	ISB

	bl FlushTLB

	pop {lr}
	bx lr


	.text
	.balign	4
	.globl	EnableCache
	.type	EnableCache, %function
EnableCache:
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #(SCTLR_C)
	orr r0, r0, #(SCTLR_I)
	mcr p15, 0, r0, c1, c0, 0
	bx lr


	.text
	.balign	4
	.globl	DisableCache
	.type	DisableCache, %function
DisableCache:
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(SCTLR_C)
	bic r0, r0, #(SCTLR_I)
	mcr p15, 0, r0, c1, c0, 0
	bx lr
