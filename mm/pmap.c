#include "mmu.h"
#include "pmap.h"
#include "printf.h"
#include "env.h"
#include <color.h>
#include "error.h"
#define MAXMEM 64

/* These variables are set by mips_detect_memory() */
u_long maxpa;            /* Maximum physical address */
u_long npage;            /* Amount of memory(in pages) */
u_long basemem;          /* Amount of base memory(in bytes) */
u_long extmem;           /* Amount of extended memory(in bytes) */

Pde *boot_pgdir;

struct Page *pages;
static u_long freemem;

static struct Page_list page_free_list;	/* Free list of physical pages */


/* Exercise 2.1 */
/* Overview:
   Initialize basemem and npage.
   Set basemem to be 64MB, and calculate corresponding npage value.*/
void mips_detect_memory()
{
	/* Step 1: Initialize basemem.
	 * (When use real computer, CMOS tells us how many kilobytes there are). */
	
	/*========== MY CODES START ==========*/
	maxpa = (MAXMEM * 1024 * 1024);
	npage = maxpa / BY2PG;
	basemem = maxpa;
	extmem = 0;
	/*========== MY CODES END ==========*/

	// Step 2: Calculate corresponding npage value.

	//printf("\033[34m[MEMORY SYSTEM]\tPhysical Memory %dK Available.\033[0m\n", (int)(maxpa / 1024));
}

/* Overview:
   Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
   allocated memory.
   This allocator is used only while setting up virtual memory system.

   Post-Condition:
   If we're out of memory, should panic, else return this address of memory we have allocated.*/
static void *alloc(u_int n, u_int align, int clear)
{
	extern char end[];
	u_long alloced_mem;

	/* Initialize `freemem` if this is the first time. The first virtual address that the
	 * linker did *not* assign to any kernel code or global variables. */
	if (freemem == 0) {
		freemem = (u_long)end;
	}

	/* Step 1: Round up `freemem` up to be aligned properly */
	freemem = ROUND(freemem, align);

	/* Step 2: Save current value of `freemem` as allocated chunk. */
	alloced_mem = freemem;

	/* Step 3: Increase `freemem` to record allocation. */
	freemem = freemem + n;

	/* Check if we're out of memory. If we are, PANIC !! */
	if (PADDR(freemem) >= maxpa) {
		panic("out of memory\n");
		return (void *)-E_NO_MEM;
	}

	/* Step 4: Clear allocated chunk if parameter `clear` is set. */
	if (clear) {
		bzero((void *)alloced_mem, n);
	}

	/* Step 5: return allocated chunk. */
	return (void *)alloced_mem;
}

/* Exercise 2.6 */
/* Overview:
   Get the page table entry for virtual address `va` in the given
   page directory `pgdir`.
   If the page table is not exist and the parameter `create` is set to 1,
   then create it.*/

static Pte *boot_pgdir_walk(Pde *pgdir, u_long va, int create)
{

	Pde *pgdir_entryp;
	Pte *pgtable, *pgtable_entry;

	/* Step 1: Get the corresponding page directory entry and page table. */
	/* Hint: Use KADDR and PTE_ADDR to get the page table from page directory
	 * entry value. */

	pgdir_entryp = &pgdir[PDX(va)];

	/* Step 2: If the corresponding page table is not exist and parameter `create`
	 * is set, create one. And set the correct permission bits for this new page
	 * table. */

	if (((*pgdir_entryp) & PTE_V) == 0) {
		if (create) {
			*pgdir_entryp = (Pde)PADDR(alloc(BY2PG, BY2PG, 1)) | (PTE_V | PTE_R);
		} else return NULL;
	}

	/* Step 3: Get the page table entry for `va`, and return it. */

	pgtable = (Pte*)KADDR(PTE_ADDR(*pgdir_entryp));
	
	pgtable_entry = &pgtable[PTX(va)];
	return pgtable_entry;
}


/* Exercise 2.7 */
/*Overview:
  Map [va, va+size) of virtual address space to physical [pa, pa+size) in the page
  table rooted at pgdir.
  Use permission bits `perm | PTE_V` for the entries.

  Pre-Condition:
  Size is a multiple of BY2PG.*/

void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm)
{
	int i;
	Pte *pgtable_entry;

	/* Step 1: Check if `size` is a multiple of BY2PG. */
	size = ROUND(size, BY2PG);

	/* Step 2: Map virtual address space to physical address. */
	/* Hint: Use `boot_pgdir_walk` to get the page table entry of virtual address `va`. */
	for (i = 0; i < size; i += BY2PG) {
		pgtable_entry = boot_pgdir_walk(pgdir, va + i, 1);
		*pgtable_entry = PTE_ADDR(pa + i) | (perm | PTE_V);
	}
}


/* Overview:
   Set up two-level page table.

Hint:
You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void mips_vm_init()
{
	extern char end[];
	extern int mCONTEXT;
	extern struct Env *envs;

	Pde *pgdir;
	u_int n;

	/* Step 1: Allocate a page for page directory(first level page table). */
	pgdir = alloc(BY2PG, BY2PG, 1);
	//printf("to memory %x for struct page directory.\n", freemem);
	mCONTEXT = (int)pgdir;

	boot_pgdir = pgdir;

	/* Step 2: Allocate proper size of physical memory for global array `pages`,
	 * for physical memory management. Then, map virtual address `UPAGES` to
	 * physical address `pages` allocated before. In consideration of alignment,
	 * you should round up the memory size before map. */
	pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
	//printf("to memory %x for struct Pages.\n", freemem);
	n = ROUND(npage * sizeof(struct Page), BY2PG);
	boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_R);

	/* Step 3, Allocate proper size of physical memory for global array `envs`,
	 * for process management. Then map the physical address to `UENVS`. */
	envs = (struct Env *)alloc(NENV * sizeof(struct Env), BY2PG, 1);
	n = ROUND(NENV * sizeof(struct Env), BY2PG);
	boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_R);

	//printf("\033[34m[MEMORY SYSTEM]\tVirtual Memory Initialized!\033[0m\n");
}

/* Exercise 2.3 */
/*Overview:
  Initialize page structure and memory free list.
  The `pages` array has one `struct Page` entry per physical page. Pages
  are reference counted, and free pages are kept on a linked list.
Hint:
Use `LIST_INSERT_HEAD` to insert something to list.*/

void page_init(void)
{
	/* Step 1: Initialize page_free_list. */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	LIST_INIT(&page_free_list);	

	/* Step 2: Align `freemem` up to multiple of BY2PG. */
	freemem = ROUND(freemem, BY2PG);

	/* Step 3: Mark all memory blow `freemem` as used(set `pp_ref`
	 * filed to 1) */

	int i;
	for (i = 0; page2kva(&pages[i]) < freemem; i++) {
		pages[i].pp_ref = 1;
	}
	//printf(B_MAGENTA"MemSystem"C_RESET": %d Bytes Available.\n", (npage - i) * BY2PG);
	/* Step 4: Mark the other memory as free. */
	for (; i < npage; i++) {
		pages[i].pp_ref = 0;
		if (i != page2ppn(pa2page(PADDR(TIMESTACK - BY2PG))))
			LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
}


/* Exercise 2.4 */
/*Overview:
  Allocates a physical page from free memory, and clear this page.

  Post-Condition:
  If failing to allocate a new page(out of memory(there's no free page)),
  return -E_NO_MEM.
  Else, set the address of allocated page to *pp, and return 0.

Note:
DO NOT increment the reference count of the page - the caller must do
these if necessary (either explicitly or via page_insert).

Hint:
Use LIST_FIRST and LIST_REMOVE defined in include/queue.h .*/

int page_alloc(struct Page **pp)
{
	struct Page *ppage_temp;
	
	/* Step 1: Get a page from free memory. If fail, return the error code.*/
	if (LIST_EMPTY(&page_free_list))
		return -E_NO_MEM;
	
	/* Step 2: Initialize this page.
	 * Hint: use `bzero`. */
	ppage_temp = LIST_FIRST(&page_free_list);
	//printf(B_MAGENTA"MemSystem"C_RESET": Allocated <0x%06x-0x%06x>.\n", page2pa(ppage_temp), page2pa(ppage_temp)+BY2PG);
	LIST_REMOVE(ppage_temp, pp_link);

	bzero((void*)page2kva(ppage_temp), BY2PG);

	*pp = ppage_temp;
	//printf("[PAGE_ALLOC] Allocated Page[%d]\n", page2ppn(*pp));
	return 0;
}

/* Exercise 2.5 */
/*Overview:
  Release a page, mark it as free if it's `pp_ref` reaches 0.
Hint:
When you free a page, just insert it to the page_free_list.*/


void page_free(struct Page *pp)
{
	/* Step 1: If there's still virtual address referring to this page, do nothing. */
	if (pp->pp_ref > 0) {
		return;
	}

	/* Step 2: If the `pp_ref` reaches 0, mark this page as free and return. */
	else if (pp->pp_ref == 0){
		LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
		return;
	}

	/* If the value of `pp_ref` is less than 0, some error must occurr before,
	 * so PANIC !!! */
	panic("cgh:pp->pp_ref is less than zero\n");
}


/* Exercise 2.8 */
/*Overview:
  Given `pgdir`, a pointer to a page directory, pgdir_walk returns a pointer
  to the page table entry (with permission PTE_R|PTE_V) for virtual address 'va'.

  Pre-Condition:
  The `pgdir` should be two-level page table structure.

  Post-Condition:
  If we're out of memory, return -E_NO_MEM.
  Else, we get the page table entry successfully, store the value of page table
  entry to *ppte, and return 0, indicating success.

Hint:
We use a two-level pointer to store page table entry and return a state code to indicate
whether this function execute successfully or not.
This function has something in common with function `boot_pgdir_walk`.*/

int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte)
{
	Pde *pgdir_entryp;
	Pte *pgtable = 0;
	struct Page *ppage = NULL;

	/* Step 1: Get the corresponding page directory entry and page table. */
	pgdir_entryp = &pgdir[PDX(va)];

	/* Step 2: If the corresponding page table is not exist(valid) and parameter `create`
	 * is set, create one. And set the correct permission bits for this new page table.
	 * When creating new page table, maybe out of memory. */
	if (((*pgdir_entryp) & PTE_V) == 0) {
		if (create) {
			int ret = page_alloc(&ppage);
			//printf("[PGDIR_WALK] Called Allocated Page[%d]\n", page2ppn(ppage));
			if (ret == 0) {
				*pgdir_entryp = (Pde)page2pa(ppage) | PTE_V | PTE_R;
				ppage->pp_ref++;
			} else {
				return ret;
			}
		} else {
			*ppte = NULL;
			return 0;
		}
	}

	/* Step 3: Set the page table entry to `*ppte` as return value. */
	pgtable = (Pte*)KADDR(PTE_ADDR(*pgdir_entryp));
	*ppte = &pgtable[PTX(va)];
	//printf("[PGDIR_WALK] Finally Allocated Page[%d]\n", page2ppn((struct Page*)pa2page(**ppte)));
	return 0;
}



/* Exercise 2.9 */
/*Overview:
  Map the physical page 'pp' at virtual address 'va'.
  The permissions (the low 12 bits) of the page table entry should be set to 'perm|PTE_V'.

  Post-Condition:
  Return 0 on success
  Return -E_NO_MEM, if page table couldn't be allocated

Hint:
If there is already a page mapped at `va`, call page_remove() to release this mapping.
The `pp_ref` should be incremented if the insertion succeeds.*/

int page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm)
{
	u_int PERM;
	Pte *pgtable_entry;
	PERM = perm | PTE_V;
	int dbg = 0;

	/* Step 1: Get corresponding page table entry. */
	pgdir_walk(pgdir, va, 0, &pgtable_entry);

	/* When Successfully Find Pte*, and existing Pte is Valid: */
	if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) {
		if (dbg) printf("Valid Page Found!\n");
		if (pa2page(*pgtable_entry) != pp) {
			/* When already another va->pp(Page), remove it */
			page_remove(pgdir, va);
			if (dbg) printf("Removed Old Map, Add new map!\n");
		} else {
			/* When already va->pp(Page), update permission and return */
			tlb_invalidate(pgdir, va);
			*pgtable_entry = (page2pa(pp) | PERM);
			if (dbg) printf("Updated Perm And Finished Insert!\n");
			return 0;
		}
	}
	if (dbg) printf("Re Get Page And Insert!\n");
	/* Step 2: Update TLB. */
	/* Now, we know that: Pte is not valid or has just be removed */
	/* As we has to insert (need modify existing tlb content), we should update tlb */
	tlb_invalidate(pgdir, va);
	/* hint: use tlb_invalidate function */

	/* Step 3: Do check, re-get page table entry to validate the insertion. */
	int ret = pgdir_walk(pgdir, va, 1, &pgtable_entry);
	if (dbg) printf("New Page Entry:0x%08x", (*pgtable_entry));	
	/* Step 3.1 Check if the page can be insert, if can’t return -E_NO_MEM */
	if (ret != 0) {
		return ret;
	}
	/* Step 3.2 Insert page and increment the pp_ref */
	*pgtable_entry = page2pa(pp) | PERM;
	pp->pp_ref++;
	return 0;
}




/*Overview:
  Look up the Page that virtual address `va` map to.

  Post-Condition:
  Return a pointer to corresponding Page, and store it's page table entry to *ppte.
  If `va` doesn't mapped to any Page, return NULL.*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte)
{
	struct Page *ppage;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	if (pte == 0) {
		return 0;
	}
	if ((*pte & PTE_V) == 0) {
		return 0;    //the page is not in memory.
	}

	/* Step 2: Get the corresponding Page struct. */

	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	ppage = pa2page(*pte);
	if (ppte) {
		*ppte = pte;
	}

	return ppage;
}

// Overview:
// 	Decrease the `pp_ref` value of Page `*pp`, if `pp_ref` reaches to 0, free this page.
void page_decref(struct Page *pp) {
	if(--pp->pp_ref == 0) {
		page_free(pp);
	}
}

// Overview:
// 	Unmaps the physical page at virtual address `va`.
void page_remove(Pde *pgdir, u_long va)
{
	Pte *pagetable_entry;
	struct Page *ppage;

	/* Step 1: Get the page table entry, and check if the page table entry is valid. */

	ppage = page_lookup(pgdir, va, &pagetable_entry);

	if (ppage == 0) {
		return;
	}

	/* Step 2: Decrease `pp_ref` and decide if it's necessary to free this page. */

	/* Hint: When there's no virtual address mapped to this page, release it. */
	ppage->pp_ref--;
	if (ppage->pp_ref == 0) {
		page_free(ppage);
	}

	/* Step 3: Update TLB. */
	*pagetable_entry = 0;
	tlb_invalidate(pgdir, va);
	return;
}

// Overview:
// 	Update TLB.
void tlb_invalidate(Pde *pgdir, u_long va)
{
	if (curenv) {
		tlb_out(PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id));
	} else {
		tlb_out(PTE_ADDR(va));
	}
}


void pageout(int va, int context)
{
	struct Page *p = NULL;
	int stdPanic = 0;

	if (context < 0x80000000) {
		if (stdPanic) panic("tlb refill and alloc error!");
		panic("[PAGEOUT] Invalid Kernel Address For Env.PgDir!");
	}

	if ((0x7f400000 < va) && (va < 0x7f800000)) {
		if (stdPanic) panic(">>>>>>>>>>>>>>>>>>>>>>it's env's zone");
		panic("[PAGEOUT] Invalid PageOut! Env Space Already Mapped!");
	}

	if (va < 0x10000) {
		if (stdPanic) panic("^^^^^^TOO LOW^^^^^^^^^");
		panic("[PAGEOUT][%d] Access Denied! <0x%08x>", curenv->env_id, va);
	}

	if ((page_alloc(&p)) < 0) {
		if (stdPanic) panic ("page alloc error!");
		panic ("[PAGEOUT] Memory Out! Cannot Alloc New Page For PageTable!");
	}

	if (stdPanic) p->pp_ref++;

	page_insert((Pde *)context, p, VA2PFN(va), PTE_R);
	//printf(B_MAGENTA"[PAGEOUT]"C_RESET":Inserted Page At [0x%x].\n", va);
}

