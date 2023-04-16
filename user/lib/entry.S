#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <mmu.h>
#include <trap.h>

/*===== For User Data =====*/
.data
	.p2align 12

.globl fsipcbuf
	fsipcbuf:
		.space BY2PG

.globl fdtab
	fdtab:
		.space BY2PG

/* User VA for Envs */
.globl envs
	envs:
		.word UENVS

/* User VA for Pages */
.globl pages
	pages:
		.word UPAGES

/* User VA for UVPT */
.globl vpt
	vpt:
		.word UVPT

/* User VA for UVPD */
.globl vpd
	vpd:
		.word (UVPT+(UVPT>>12)*4)

/* User VA for Mod(COW) Handler (Modifiable) */
.globl __pgfault_handler
	__pgfault_handler:
		.word 0

/* User VA for User Main Entry */
.extern libmain

/*===== User Space Codes =====*/
.text

/* For User Entry */
.globl _start
_start:
	lw	a0, 0(sp)
	lw	a1, 4(sp)
nop
	jal	libmain
	nop

/* User Mod(COW) Wrapper */
.set noreorder
.text
.globl __asm_pgfault_handler
__asm_pgfault_handler:

	/* call handler(BADVA) */
	lw	a0, TF_BADVADDR(sp)
	lw	t1, __pgfault_handler
	jalr	t1
	nop

	/* return from handler, restore */
	lw		v0,TF_LO(sp)
	mtlo	v0
	lw		v0,TF_HI(sp)
	mthi	v0
	lw		v0,TF_EPC(sp)
	mtc0	v0,CP0_EPC
	
	/* Load All Regs Status (except $26, $27, $29) */
	.set noat
	lw	$31,TF_REG31(sp)
	lw	$30,TF_REG30(sp)
	lw	$28,TF_REG28(sp)
	lw	$25,TF_REG25(sp)
	lw	$24,TF_REG24(sp)
	lw	$23,TF_REG23(sp)
	lw	$22,TF_REG22(sp)
	lw	$21,TF_REG21(sp)
	lw	$20,TF_REG20(sp)
	lw	$19,TF_REG19(sp)
	lw	$18,TF_REG18(sp)
	lw	$17,TF_REG17(sp)
	lw	$16,TF_REG16(sp)
	lw	$15,TF_REG15(sp)
	lw	$14,TF_REG14(sp)
	lw	$13,TF_REG13(sp)
	lw	$12,TF_REG12(sp)
	lw	$11,TF_REG11(sp)
	lw	$10,TF_REG10(sp)
	lw	$9,TF_REG9(sp)
	lw	$8,TF_REG8(sp)
	lw	$7,TF_REG7(sp)
	lw	$6,TF_REG6(sp)
	lw	$5,TF_REG5(sp)
	lw	$4,TF_REG4(sp)
	lw	$3,TF_REG3(sp)
	lw	$2,TF_REG2(sp)
	lw	$1,TF_REG1(sp)

	/* Jump to epc, restore stack from frame */
	lw	k0,TF_EPC(sp) 	//atomic operation needed
	jr	k0
	lw	sp,TF_REG29(sp)  /* Deallocate stack */
	.set at
