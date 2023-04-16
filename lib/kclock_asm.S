#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <kclock.h>

.macro	setup_c0_status set clr
	.set	push
	mfc0	t0, CP0_STATUS
	or		t0, \set|\clr
	xor		t0, \clr
	mtc0	t0, CP0_STATUS			
	.set	pop
.endm

	.text
LEAF(set_timer)

	/* Set Timer Int Frequency */
	li 		t0, 0xc8
	sb 		t0, 0xb5000100

	/* Sava Kernel Stack Address (Going to User) */
	sw		sp, KERNEL_SP

	/* Enable Int/Exc, Enable Timer Int */
	setup_c0_status STATUS_CU0|0x1001 0
	
	/* Return */
	jr 		ra
	nop

END(set_timer)
