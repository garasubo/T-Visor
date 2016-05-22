
	.text
	.balign	4
	.globl	memset
	.type	memset, %function
memset:
	push {r4-r8}
	mov r3, r1
	add r3, r3, r1, lsr #4
	add r4, r3, r3, lsr #8
	mov r5, r4
	mov r6, r4
	mov r7, r4
	mov r8, r0
	mov r3, #0

  memset_1:
	add r3, r3, #32
	cmp r2, r3
	blo memset_2
	stmia r0!, {r4-r7}
	b memset_1

  memset_2:
	sub r3, r0, r8
	cmp r3, r2
	beq memset_4
	add r8, r8, r2

  memset_3:
	strb r1, [r0]
	add r0, r0, #1
	cmp r0, r8
	bne memset_3

  memset_4:
	pop {r4-r8}
