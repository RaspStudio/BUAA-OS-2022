#include <lib.h>
#include <mmu.h>
#include <env.h>
#include <kerelf.h>

#define debug 0
#define TMPPAGE		(BY2PG)
#define TMPPAGETOP	(TMPPAGE+BY2PG)

void strcat(char *a, const char *b) {
    strlcat(a,b,MAXPATHLEN);
}

int init_stack(u_int child, char **argv, u_int *init_esp)
{
	int argc, i, r, tot;
	char *strings;
	u_int *args;

	// count the number of arguments (argc)
	// and the total amount of space needed for strings (tot)
	tot = 0;
	for (argc = 0; argv[argc]; argc++)
		tot += strlen(argv[argc]) + 1;

	// make sure everything will fit in the initial stack page
	if (ROUND(tot, 4) + 4 * (argc + 3) > BY2PG)
		return -E_NO_MEM;

	// determine where to place the strings and the args array
	strings = (char *)TMPPAGETOP - tot;
	args = (u_int *)(TMPPAGETOP - ROUND(tot, 4) - 4 * (argc + 1));

	if ((r = syscall_mem_alloc(0, TMPPAGE, PTE_V|PTE_R)) < 0)
		return r;
	
	// copy the argument strings into the stack page at 'strings'
	char *ctemp, *argv_temp;
	u_int j;
	ctemp = strings;
	for(i = 0; i < argc; i++)
	{
		argv_temp = argv[i];
		for(j = 0; j < strlen(argv[i]); j++)
			*ctemp++ = *argv_temp++;
		*ctemp++ = 0;
	}

	// initialize args[0..argc-1] to be pointers to these strings
	// that will be valid addresses for the child environment
	// (for whom this page will be at USTACKTOP-BY2PG!).
	ctemp = (char *)(USTACKTOP - TMPPAGETOP + (u_int)strings);
	for(i = 0; i < argc; i++)
	{
		args[i] = (u_int)ctemp;
		ctemp += strlen(argv[i]) + 1;
	}

	// set args[argc] to null-terminate the args array.
	ctemp--;
	args[argc] = ctemp;
	
	// push two more words onto the child's stack below 'args',
	// containing the argc and argv parameters to be passed
	// to the child's umain() function.
	u_int *pargv_ptr;
	pargv_ptr = args - 1;
	*pargv_ptr = USTACKTOP - TMPPAGETOP + (u_int)args;
	pargv_ptr--;
	*pargv_ptr = argc;
	
	// set *init_esp to the initial stack pointer for the child
	*init_esp = USTACKTOP - TMPPAGETOP + (u_int)pargv_ptr;

	if ((r = syscall_mem_map(0, TMPPAGE, child, USTACKTOP-BY2PG, PTE_V|PTE_R)) < 0)
		goto error;
	if ((r = syscall_mem_unmap(0, TMPPAGE)) < 0)
		goto error;

	return 0;

error:
	syscall_mem_unmap(0, TMPPAGE);
	return r;
}

int usr_is_elf_format(u_char *binary){
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
        ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr->e_ident[EI_MAG3] == ELFMAG3) {
        return 1;
    }   

    return 0;
}

int usr_load_elf(int fd, Elf32_Phdr *ph, int child_envid){
	//Hint: maybe this function is useful 
	//      If you want to use this func, you should fill it ,it's not hard
	u_long va = ph->p_vaddr;
	u_int32_t sgsize = ph->p_memsz;
	u_int32_t bin_size = ph->p_filesz;
	u_char *bin;
    u_long i;
    int r;
    u_long offset = va - ROUNDDOWN(va, BY2PG);
	u_long len = 0;

	if ((r = read_map(fd, ph->p_offset, &bin)) < 0)
		return r;

	if (offset != 0) {
		if ((r = syscall_mem_alloc(child_envid, va, PTE_V | PTE_R)) < 0)
			return r;
		if ((r = syscall_mem_map(child_envid, va, 0, USTACKTOP, PTE_V | PTE_R)) < 0)
			return r;
		len = MIN(BY2PG - offset, bin_size);
		user_bcopy(bin, USTACKTOP + offset, len);
		if ((r = syscall_mem_unmap(0, USTACKTOP)) < 0)
			return r;
	}
	// delta can't be "len"(different from lib/env.c/load_icode_mapper), because
	// user can't check whether a page is alloced, so when bin_size is not aligned,
	// the page of bin_size will be alloced twice, the first physial page will be lost
	for (i = len; i < bin_size; i += BY2PG) {
		if ((r = syscall_mem_alloc(child_envid, va + i, PTE_V | PTE_R)) < 0)
			return r;
		if ((r = syscall_mem_map(child_envid, va + i, 0, USTACKTOP, PTE_V | PTE_R)) < 0)
			return r;
		user_bcopy(bin + i, USTACKTOP, MIN(BY2PG, bin_size - i));
		if ((r = syscall_mem_unmap(0, USTACKTOP)) < 0)
			return r;
    }
    
	while (i < sgsize) {
		if ((r = syscall_mem_alloc(child_envid, va + i, PTE_V | PTE_R)) < 0)
			return r;
		i += BY2PG;
	}
    return 0;
}

int spawn(char *prog, char **argv)
{
	u_char elfbuf[512];
	int r;
	int fd;
	u_int child_envid;
	int size, text_start;
	u_int i, *blk;
	u_int esp;
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph;
	// Note 0: some variable may be not used,you can cancel them as you like
	// Step 1: Open the file specified by `prog` (prog is the path of the program)
	char path[MAXPATHLEN];
	int len = strlen(prog);
	strcpy(path, prog);
	if (len <= 2 || path[len - 2] != '.' || path[len - 1] != 'b')
		strcat(path, ".b");
	if ((r = open(path, O_RDONLY)) < 0)
		return r;
	
	// Your code begins here
	// Before Step 2 , You had better check the "target" spawned is a execute bin
	fd = r;
	if ((r = readn(fd, elfbuf, sizeof(Elf32_Ehdr))) < 0)
		return r;
	elf = (Elf32_Ehdr *)elfbuf;
	if (!usr_is_elf_format(elfbuf) || elf->e_type != 2)
		return -E_INVAL;

	// Step 2: Allocate an env (Hint: using syscall_env_alloc())
	if ((r = syscall_env_alloc()) < 0)
		return r;

	// Step 3: Using init_stack(...) to initialize the stack of the allocated env
	child_envid = r;
	if ((r = init_stack(child_envid, argv, &esp)) < 0)
		return r;

	// Step 3: Map file's content to new env's text segment
	//        Hint 1: what is the offset of the text segment in file? try to use objdump to find out.
	//        Hint 2: using read_map(...)
	//		  Hint 3: Important!!! sometimes ,its not safe to use read_map ,guess why 
	//				  If you understand, you can achieve the "load APP" with any method
	size = elf->e_phentsize;
	text_start = elf->e_phoff;
	for (i = 0; i < elf->e_phnum; i++) {
		if ((r = seek(fd, text_start)) < 0)
			return r;
		if ((r = readn(fd, elfbuf, size)) < 0)
			return r;
		ph = (Elf32_Phdr *)elfbuf;
		if (ph->p_type == PT_LOAD)
			if ((r = usr_load_elf(fd, ph, child_envid)) < 0)
				return r;
		text_start += size;
	}
    close(fd);
	// Note1: Step 1 and 2 need sanity check. In other words, you should check whether
	//       the file is opened successfully, and env is allocated successfully.
	// Note2: You can achieve this func in any way ，remember to ensure the correctness
	//        Maybe you can review lab3 
	// Your code ends here

	struct Trapframe *tf;
	// writef("\n::::::::::spawn size : %x  sp : %x::::::::\n", size, esp);
	tf = &envs[ENVX(child_envid)].env_tf;
	tf->pc = UTEXT;
	tf->regs[29] = esp;

	// Share memory
	u_int pdeno = 0;
	u_int pteno = 0;
	u_int pn = 0;
	u_int va = 0;
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		if(!((*vpd)[pdeno] & PTE_V))
			continue;
		for (pteno = 0; pteno <= PTX(~0); pteno++) {
			pn = (pdeno << 10) + pteno;
			if ( ((*vpt)[pn] & PTE_V) && ((*vpt)[pn] & PTE_LIBRARY) ) {
				va = pn * BY2PG;
				if ((r = syscall_mem_map(0, va, child_envid, va, PTE_V|PTE_R|PTE_LIBRARY)) < 0) {
					writef("va: %x   child_envid: %x   \n", va, child_envid);
					return r;
				}
			}
		}
	}

	var_dup(child_envid, !strcmp("mysh.b", path));

	if ((r = syscall_set_env_status(child_envid, ENV_RUNNABLE)) < 0) {
		writef("set child runnable is wrong\n");
		return r;
	}
	return child_envid;
}

int spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}