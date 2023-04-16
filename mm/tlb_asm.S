#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

/* Exercise 2.10 */
LEAF(tlb_out)
//1: j 1b
nop
	mfc0	k1,CP0_ENTRYHI		// 
	mtc0	a0,CP0_ENTRYHI		// CP0_ENTRYHI = arg[0]
	nop
	// insert tlbp or tlbwi
	nop							//	if tlb.has(CP0_ENTRYHI):
	tlbp						// 		CP0_INDEX = tlb.indexOf(CP0_ENTRYHI)
	nop							//	else CP0_INDEX = -1
	// my insert end
	nop
	nop
	nop
	nop
	mfc0	k0,CP0_INDEX		//	if (CP0_INDEX >= 0) 
	bltz	k0,NOFOUND			//		
	nop							//		
	mtc0	zero,CP0_ENTRYHI	// 	
	mtc0	zero,CP0_ENTRYLO0	//		CP0_ENTRYHI = CP0_ENTRYLO0 = 0
	nop
	// insert tlbp or tlbwi

	tlbwi						//		tlb.indexOf(CP0_INDEX) = {CP0_ENTRYHI, CP0_ENTRYLO0}

	// my insert end
NOFOUND:

	mtc0	k1,CP0_ENTRYHI		//	
	
	j	ra						//	return
	nop
END(tlb_out)

