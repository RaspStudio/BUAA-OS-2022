#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <stackframe.h>


.macro	BUILD_HANDLER exception handler
	.align	5
	NESTED(handle_\exception, TF_SIZE, sp)  
		.set	noat
		nop
		SAVE_ALL				

		.set	at
		move	a0, sp
		jal		\handler
		nop
		j		ret_from_exception
		nop
	END(handle_\exception)
.endm

/*---------- RFE Function Export ----------*/
FEXPORT(ret_from_exception)
	.set 	noat
	.set 	noreorder
	RESTORE_SOME
	.set 	at
	/* Restore PC for Return */
	lw		k0, TF_EPC(sp)		
	/* Restore stack */	 
	lw		sp, TF_REG29(sp) 
	nop
	jr		k0								 
	rfe								 

/*========== Other Handlers Below ==========*/

/*---------- Interrupt Handler ----------*/	
	.align	5
	.set 	noreorder
NESTED(handle_int, TF_SIZE, sp)
	.set	noat

	/* Save User Frame at Int */
	SAVE_ALL

	.set	at
	/* Check Perm of Timer Int */
	mfc0	t0, CP0_CAUSE
	mfc0	t2, CP0_STATUS
	and		t0, t2
	andi	t1, t0, STATUSF_IP4

	/* Dispatch: Timer Int? */
	bnez	t1, handle_timer_int
	nop

	/* Dispatch: (...more) */

END(handle_int)

handle_timer_int:
	sb 		zero, 0xb5000110
	j		sched_yield
	nop


/*---------- Reserved Handler ----------*/
LEAF(do_reserved)
	/* My Handler Switch */
	j		1f
	nop

	mfc0    k1, CP0_CAUSE
    andi    k1, 0x7c
	srl		k1, 2
	move 	a0, k1
	j 		do_reserved_out
1:	nop
END(do_reserved)


/*---------- TLB Handler ----------*/
			
NESTED(do_refill, 0, sp)
	.set 	noreorder
	.extern tlbra
	.extern	mCONTEXT

	/*--- Load KPD, Find KPT ---*/			
1:	lw		k1, mCONTEXT
	and		k1, 0xfffff000

		/* Get BadVaddr */
		mfc0	k0, CP0_BADVADDR

		/* Get PDoffset(BadVaddr) */
		srl		k0, 20
		and		k0, 0xfffffffc
		addu	k0, k1

		/* Get PT Entry -> k1 */		
		lw		k1, 0(k0)
		nop

	/*--- Check PT Entry Validity ---*/
	move	t0, k1
	and		t0, 0x0200
					
	/* If No Valid PT -> NOPAGE */
	beqz	t0, NOPAGE
	nop

	/* Else Got Valid PT: Find PT Elem */
	and		k1, 0xfffff000
	mfc0	k0, CP0_BADVADDR

		/* Get PT Elem offset */
		srl		k0, 10
		and		k0, 0xfffffffc
		and		k0, 0x00000fff
		
		/* Load PT Elem Data */
		addu	k0, k1
		or		k0, 0x80000000
		lw		k1, 0(k0)
		nop

		/* Check PT Elem Validity */
		move	t0, k1
		and		t0, 0x0200

		/* If No Valid PT Elem -> NOPAGE */
		beqz	t0, NOPAGE
		nop

			/* Check COW Perm */
			move	k0, k1
			and		k0, 0x1

			/* If No COW -> NoCOW */
			beqz	k0, NoCOW
			nop

			/* If COW -> Delete PTE_R */
			and		k1, 0xfffffbff
NoCOW:
			/* Load PT Elem To Random TLB Line */
			/* CP0_ENTRYHI already assigned when try read */
			mtc0	k1, CP0_ENTRYLO0
			nop
			tlbwr

			/* Jump To Return */
			j		2f
			nop
NOPAGE:
	/* No Valid PT, Create One */
	mfc0	a0, CP0_BADVADDR
	lw		a1, mCONTEXT
	nop
	
	/* Save Current PC, Call Pageout */
	sw	 	ra, tlbra
	jal		pageout
	nop
	nop

	/* Pageout has alloc a page for new PT, rerun handler */
	lw		ra, tlbra
	nop
	j		1b
2:	nop

	/* Handler Finish Return */
	jr		ra
	nop
END(do_refill)

/*---------- Other Handler ---------- */

BUILD_HANDLER reserved do_reserved
BUILD_HANDLER tlb	do_refill
BUILD_HANDLER mod	page_fault_handler
