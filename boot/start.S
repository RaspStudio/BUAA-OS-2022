/*
 * Copyright (C) 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

/*========== For Exception ==========*/

.section    .text.exc_vec3
NESTED(except_vec3, 0, sp)
    .set    noat 
    .set    noreorder
1:
    /* Get Exception Cause To $k1 (CAUSE[6:2]) */
    mfc0    k1, CP0_CAUSE
    andi    k1, 0x7c

    /* Get Exception Handler Code Address */
    la      k0, exception_handlers
    addu    k0, k1
    lw      k0, (k0)
    nop

    /* Go To Exception Handler */
    jr      k0
    nop
END(except_vec3)
.set at


/*========== For Kernel Data ==========*/
.data
/* Kernel Page Directory Address Storage */
    .globl mCONTEXT
        mCONTEXT:
            .word 0

/* Return Address Storage For TLB Handler */
    .globl tlbra
        tlbra:
            .word 0

/*========== For Boot Code ==========*/
.text
LEAF(_start)

    .set    mips2
    .set    reorder

    /* Disable interrupts */
    mtc0    zero, CP0_STATUS

    /* Disable watch exception. */
    mtc0    zero, CP0_WATCHLO
    mtc0    zero, CP0_WATCHHI

    /* Disable kernel mode cache */
    mfc0    t0, CP0_CONFIG
    and     t0, ~0x7
    ori     t0, 0x2
    mtc0    t0, CP0_CONFIG

    /* Set up Stack */
    li      sp, 0x80400000

    /* Set up KPD */
    li      t0, 0x80400000
    sw      t0, mCONTEXT

    /* Jump to main */
    jal     main

loop:
    j       loop
    nop
END(_start)
