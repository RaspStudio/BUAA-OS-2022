// implement fork from user space

#include "lib.h"
#include <mmu.h>
#include <env.h>


/* ----------------- help functions ---------------- */

/* Overview:
 * 	Copy `len` bytes from `src` to `dst`.
 *
 * Pre-Condition:
 * 	`src` and `dst` can't be NULL. Also, the `src` area
 * 	 shouldn't overlap the `dest`, otherwise the behavior of this
 * 	 function is undefined.
 */
void user_bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	//	writef("~~~~~~~~~~~~~~~~ src:%x dst:%x len:%x\n",(int)src,(int)dst,len);
	max = dst + len;

	// copy machine words while possible
	if (((int)src % 4 == 0) && ((int)dst % 4 == 0)) {
		while (dst + 3 < max) {
			*(int *)dst = *(int *)src;
			dst += 4;
			src += 4;
		}
	}

	// finish remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst = *(char *)src;
		dst += 1;
		src += 1;
	}

	//for(;;);
}

/* Overview:
 * 	Sets the first n bytes of the block of memory
 * pointed by `v` to zero.
 *
 * Pre-Condition:
 * 	`v` must be valid.
 *
 * Post-Condition:
 * 	the content of the space(from `v` to `v`+ n)
 * will be set to zero.
 */
void user_bzero(void *v, u_int n)
{
	char *p;
	int m;

	p = v;
	m = n;

	while (--m >= 0) {
		*p++ = 0;
	}
}
/*--------------------------------------------------------------*/

/* Overview:
 * 	Custom page fault handler - if faulting page is copy-on-write,
 * map in our own private writable copy.
 *
 * Pre-Condition:
 * 	`va` is the address which leads to a TLBS exception.
 *
 * Post-Condition:
 *  Launch a user_panic if `va` is not a copy-on-write page.
 * Otherwise, this handler should map a private writable copy of
 * the faulting page at correct address.
 */
/*** exercise 4.13 ***/
void
pgfault(u_int va)
{
	int r;
	int dbg = 0;
	
	u_int temp = USTACKTOP;

	/*--- Check Page: CopyOnWrite ---*/
	Pte page_entry = ((Pte*)(*vpt))[VPN(va)];
	Pte perm = page_entry & 0xfff;

	if (dbg) writef("[COW_HANDLER]\t Called By Env[%d]:0x%08x {%08x}\n", env->env_id, va, page_entry);

	if (!(perm & PTE_COW) || !(perm & PTE_V)) {
		user_panic("User Pgfault called by NON-COW Page!");
	}

	Pte newperm = ((perm & ~(PTE_COW)) | PTE_R);

	/*--- Temporary Mapping ---*/
	u_int varound = ROUNDDOWN(va, BY2PG);

	if ((r = syscall_mem_alloc(0, temp, newperm)) != 0) {
		user_panic("User Pgfault Can't Alloc New Space!'");
	}
	
	/*--- Copy New Page ---*/
	user_bcopy((void*)varound, (void*)(temp), BY2PG);

	/*--- Fix Back Mapping To va ---*/
	if ((r = syscall_mem_map(0, temp, 0, varound, newperm)) != 0) {
		user_panic("User Pgfault Can't Map OriginalPage -> TempPage!'");
	}

	/*--- Delete Temporary Mapping ---*/
	if ((r = syscall_mem_unmap(0, temp)) != 0) {
		user_panic("User Pgfault Can't UnMap USTACK Page!");
	}
	if (dbg) writef("[COW_HANDLER]\t Finished Env[%d]:0x%08x {%08x}\n", env->env_id, va, ((Pte*)(*vpt))[VPN(va)]);
}

/* Overview:
 * 	Map our virtual page `pn` (address pn*BY2PG) into the target `envid`
 * at the same virtual address.
 *
 * Post-Condition:
 *  if the page is writable or copy-on-write, the new mapping must be
 * created copy on write and then our mapping must be marked
 * copy on write as well. In another word, both of the new mapping and
 * our mapping should be copy-on-write if the page is writable or
 * copy-on-write.
 *
 * Hint:
 * 	PTE_LIBRARY indicates that the page is shared between processes.
 * A page with PTE_LIBRARY may have PTE_R at the same time. You
 * should process it correctly.
 */
/*** exercise 4.10 ***/
static void
duppage(u_int envid, u_int pn)
{
	u_int addr = pn << PGSHIFT;
	u_int perm = ((Pte*)(*vpt))[pn] & 0xfff;
	int r;
	int flag = 0;// For Father Page Perm Update Flag

	if ((perm & PTE_R) == 0) {
		/* Read Only */
	} else if (perm & PTE_COW) {
		/* Writeable & CopyOnWrite */
	} else if (perm & PTE_LIBRARY) {
		/* Writeable & Library & Not COW */
	} else {
		/* Writeable & NotLib & Not COW */
		perm |= PTE_COW;
		flag = 1;
	}
	
	if ((r = syscall_mem_map(0, addr, envid, addr, perm)) != 0) {
		user_panic("[ERROR] Duppage Called MemMap Failed!");
	}

	if (flag) {
		/* Update Father Page Perm */
		if ((r = syscall_mem_map(0, addr, 0, addr, perm)) != 0) {
			user_panic("[ERROR] Duppage Called MemMap Failed!");
		}
	}
}

/* Overview:
 * 	User-level fork. Create a child and then copy our address space
 * and page fault handler setup to the child.
 *
 * Hint: use vpd, vpt, and duppage.
 * Hint: remember to fix "env" in the child process!
 * Note: `set_pgfault_handler`(user/pgfault.c) is different from
 *       `syscall_set_pgfault_handler`.
 */
/*** exercise 4.9 4.15***/
extern void __asm_pgfault_handler(void);

int fork() {
	int dbg = 0;
	// Your code here.
	u_int newenvid;
	extern struct Env *envs;
	extern struct Env *env;

	if (dbg) writef("[FORK] Called By Env[%d]\n", syscall_getenvid());

	/*--- Install Pgfault ---*/	
	set_pgfault_handler(pgfault);

	/*--- Try To Create New Thread ---*/
	if ((newenvid = syscall_env_alloc()) == 0) {
		if (dbg) writef("[FORK]\tSon[%d] Returned\n", syscall_getenvid());
		/*--- Child Thread Return (ENV_ALLOC) ---*/
		env = &envs[ENVX(syscall_getenvid())];
		OUT writef(B_BLUE"New"C_RESET"[%d]: Shell\n", env->env_id);
		return 0;
	} else if (newenvid < 0) {
		/* Something wrong happened */
		return newenvid;
	}
	if (dbg) writef("[FORK]\tFather[%d] Returned, Alloced Son[%d]\n", syscall_getenvid(), newenvid);
	/*--- Father Thread Return (ENV_ALLOC) ---*/	
	/*--- Map All Father VA to Child VA ---*/
	Pde *ppde = (Pde*)(*vpd);
	Pte *ppte = (Pte*)(*vpt);

	int i;
	for (i = 0; i < USTACKTOP; i += BY2PG) {
		if ((ppde[PDX(i)] & PTE_V) && (ppte[VPN(i)] & PTE_V)) {
			duppage(newenvid, VPN(i));
		} 
	}

	if (dbg) writef("[FORK]\tUSTACKTOP Under Mapped!\n");

	/*--- Alloc UXSTACK Space ---*/
	int r;
	if ((r = syscall_mem_alloc(newenvid, UXSTACKTOP - BY2PG, PTE_V | PTE_R)) != 0) {
		user_panic("User Fork Alloc UXSTACKTOP for son failed!");
	}

	if (dbg) writef("[FORK]\tUXSTACKTOP Under Mapped!\n");

	/*--- Set Handler ---*/	
	if ((r = syscall_set_pgfault_handler(newenvid, __asm_pgfault_handler, UXSTACKTOP)) != 0) {
		user_panic("User Fork Set Handler Failed!");
	}

	if (dbg) writef("[FORK]\tSon PGFAULT Under Mapped!\n");

	/*--- Enable Son Env ---*/
	if ((r = syscall_set_env_status(newenvid, ENV_RUNNABLE)) != 0) {
		user_panic("User Fork Enable Son: RUNNABLE Failed!");
	}

	if (dbg) writef("[FORK]\tSon is Ready!\n");

	return newenvid;
}

// Challenge!
int
sfork(void)
{
	user_panic("sfork not implemented");
	return -E_INVAL;
}
