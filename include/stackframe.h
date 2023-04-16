#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <trap.h>

/*---------- Enable Int/Exc (IE -> 1) - Macro ----------*/
// .macro STI
// 	mfc0	t0,	CP0_STATUS
// 	li		t1, (STATUS_CU0 | 0x1)
// 	or		t0, t1
// 	mtc0	t0, CP0_STATUS
// .endm

/*---------- Disable Int/Exc (IE -> 0) - Macro ----------*/
// .macro CLI
// 	mfc0	t0, CP0_STATUS
// 	li		t1, (STATUS_CU0 | 0x1)
// 	or		t0, t1
// 	xor		t0, 0x1
// 	mtc0	t0, CP0_STATUS
// .endm

/*---------- Save Current Status - Macro ----------*/
.macro SAVE_ALL

	/* Get CP0_STATUS, Check CPU0 Validity */
	mfc0	k0, CP0_STATUS
	sll		k0, 3      

	/* When CP0 is Able, Go Label-1 */
	bltz	k0, 1f
	nop

	/*
	 * [TODO] CP0 is disabled. (No Perm to use CP0 instr.)
	 * (Original Hint: 'Called From User')
	 */

1:	/* Case 1: CP0 is Enabled */
	
	/* Store Old Sp in $k0; $k1 = get_sp() */
	move	k0, sp
	get_sp
	move	k1, sp

	/* Get TrapFrame Start (Push Stack) */
	subu	sp, k1, TF_SIZE

	/* Save Old Stack First */
	sw		k0, TF_REG29(sp)
	
	/* Save $v0 for save CP0 use */
	sw		$2, TF_REG2(sp)

	/* Save Current CP0 Regs */
	mfc0	v0, CP0_STATUS
	sw		v0, TF_STATUS(sp)
	mfc0	v0, CP0_CAUSE
	sw		v0, TF_CAUSE(sp)
	mfc0	v0, CP0_EPC
	sw		v0, TF_EPC(sp)
	mfc0	v0, CP0_BADVADDR
	sw		v0, TF_BADVADDR(sp)

	/* Save Current MDU Regs */
	mfhi	v0
	sw		v0, TF_HI(sp)
	mflo	v0
	sw		v0, TF_LO(sp)

	/* Save Other Regs (except $2, $29) */
	sw		$0, TF_REG0(sp)
	sw		$1, TF_REG1(sp)
	sw		$3, TF_REG3(sp)
	sw		$4, TF_REG4(sp)
	sw		$5, TF_REG5(sp)
	sw		$6, TF_REG6(sp)
	sw		$7, TF_REG7(sp)
	sw		$8, TF_REG8(sp)
	sw		$9, TF_REG9(sp)
	sw		$10, TF_REG10(sp)
	sw		$11, TF_REG11(sp)
	sw		$12, TF_REG12(sp)
	sw		$13, TF_REG13(sp)
	sw		$14, TF_REG14(sp)
	sw		$15, TF_REG15(sp)
	sw		$16, TF_REG16(sp)
	sw		$17, TF_REG17(sp)
	sw		$18, TF_REG18(sp)
	sw		$19, TF_REG19(sp)
	sw		$20, TF_REG20(sp)
	sw		$21, TF_REG21(sp)
	sw		$22, TF_REG22(sp)
	sw		$23, TF_REG23(sp)
	sw		$24, TF_REG24(sp)
	sw		$25, TF_REG25(sp)
	sw		$26, TF_REG26(sp) 
	sw		$27, TF_REG27(sp) 
	sw		$28, TF_REG28(sp)
	sw		$30, TF_REG30(sp)
	sw		$31, TF_REG31(sp)
.endm

/*
 * Note that we restore the IE flags from stack. This means
 * that a modified IE mask will be nullified.
 */
.macro RESTORE_SOME
		.set	mips1

		lw		v0, TF_STATUS(sp)
		mtc0	v0, CP0_STATUS
		lw		v0, TF_LO(sp)
		mtlo	v0
		lw		v0, TF_HI(sp)
		mthi	v0
		lw		v0, TF_EPC(sp)
		mtc0	v0, CP0_EPC

		/* Load All Regs Status (except $26, $27, $29) */
		lw		$31, TF_REG31(sp)
		lw		$30, TF_REG30(sp)
		lw		$28, TF_REG28(sp)
		lw		$25, TF_REG25(sp)
		lw		$24, TF_REG24(sp)
		lw		$23, TF_REG23(sp)
		lw		$22, TF_REG22(sp)
		lw		$21, TF_REG21(sp)
		lw		$20, TF_REG20(sp)
		lw		$19, TF_REG19(sp)
		lw		$18, TF_REG18(sp)
		lw		$17, TF_REG17(sp)
		lw		$16, TF_REG16(sp)
		lw		$15, TF_REG15(sp)
		lw		$14, TF_REG14(sp)
		lw		$13, TF_REG13(sp)
		lw		$12, TF_REG12(sp)
		lw		$11, TF_REG11(sp)
		lw		$10, TF_REG10(sp)
		lw		$9, TF_REG9(sp)
		lw		$8, TF_REG8(sp)
		lw		$7, TF_REG7(sp)
		lw		$6, TF_REG6(sp)
		lw		$5, TF_REG5(sp)
		lw		$4, TF_REG4(sp)
		lw		$3, TF_REG3(sp)
		lw		$2, TF_REG2(sp)
		lw		$1, TF_REG1(sp)
.endm


/* Assign $sp According to Exc Source - Macro For "SAVE_ALL" */
/* TimerInt: sp = TIMESTACK; else if (sp in kuseg) sp = KERNEL_SP */
.set	noreorder
.macro get_sp
	mfc0	k1, CP0_CAUSE
	andi	k1, 0x107C
	xori	k1, 0x1000
	
	/* Branch By Exc Source */
	bnez	k1, 1f
	nop

	/* If Answer Timer Int : sp = TIMESTACK */
	li		sp, 0x82000000
	j		2f
	nop
1:
	/* If Already Set Stack, Branch */
	bltz	sp, 2f
	nop
	lw		sp, KERNEL_SP
	nop

	/* Get Stack Function End */
2:	nop

.endm




/*---------- Unused Macro ----------*/
// .macro RESTORE_ALL
// 		RESTORE_SOME
// 		lw	sp,TF_REG29(sp)  /* Deallocate stack */
// .endm

// .set	noreorder
// .macro RESTORE_ALL_AND_RET
// 		RESTORE_SOME
// 		lw	k0,TF_EPC(sp) 
// 		lw	sp,TF_REG29(sp)  /* Deallocate stack */
// 		jr	k0 
// 		rfe 
// .endm



