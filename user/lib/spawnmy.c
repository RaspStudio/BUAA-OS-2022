#include "lib.h"
#include <mmu.h>
#include <env.h>
#include <kerelf.h>

#define debug 0

// CORE SPAWN CODE ///////////////////////////////////////////////////////////////

int spawn(char *path, char **argv)
{
	char pathb[MAXPATHLEN] = {0};
	strcpy(pathb, path);
	int len = strlen(pathb);
	if (len <= 2 || !(pathb[len-2] == '.' && pathb[len-1] == 'b')) {
		pathb[len] = '.', pathb[len+1] = 'b', pathb[len+2] = '\0';
	}

	/*--- First, Open the executable file ---*/
	int fdnum;
	if ((fdnum = open(pathb, O_RDONLY)) < 0) {
		writef("[SPAWN] Cannot Open User Executable File!\n");
		return -E_NOT_FOUND;
	}

	/*--- Load Some Useful Headers ---*/
	Elf32_Ehdr ehdr;
	if (readn(fdnum, &ehdr, sizeof(ehdr)) < 0) {
		writef("[SPAWN] Read File Header Failed!\n");
		return -E_INVAL;
	}

	/*--- And Check executable file format ---*/
	if (ehdr.e_type != 2 || !usr_is_elf_format((u_char*)&ehdr)) {
		writef("[SPAWN] Invalid ELF File Format!\n");
		return -E_INVAL;
	}

	/*--- Second, Alloc Env Block For Exec File ---*/
	int envid;
	if ((envid = syscall_env_alloc()) < 0) {
		user_panic("[SPAWN] Unable To Alloc New Env Block!\n");
	}

	/*--- Third, Initialize Env's Stack ---*/
	u_int ustkp;
	if (usr_init_stack(envid, (const char**)argv, &ustkp) != 0) {
		user_panic("[SPAWN] Unable To Init User STACK!\n");
	}
	
	/*--- Finally, Load ELF File Data	 ---*/
	usr_load_elf(envid, fdnum);
	close(fdnum);
	/*-------------------------------------------------------------------*/

	/*--- First Prepare Initframe ---*/
	//writef("\n\033[32m:::::::::::::::[ SPAWNING : \"%13s\" ]:::::::::::::::\n\033[0m", my_mid_ltext(path,13));
	struct Trapframe *tf = &(envs[ENVX(envid)].env_tf);
	tf->pc = UTEXT;
	tf->regs[29] = ustkp;


	// Share memory (FD/FDDATA)
	u_int pdeno = 0;
	u_int pteno = 0;
	u_int pn = 0;
	u_int va = 0;
	for(pdeno = 0; pdeno < PDX(UTOP); pdeno++)
	{
		if(!((* vpd)[pdeno] & PTE_V))
			continue;
		for(pteno = 0; pteno <= PTX(~0); pteno++)
		{
			pn = (pdeno<<10) + pteno;
			if(((* vpt)[pn] & PTE_V) && ((* vpt)[pn] & PTE_LIBRARY))
			{
				va = pn * BY2PG;
				//writef("SHAREING : 0x%8x\n",va);
				if (syscall_mem_map(0, va, envid, va, PTE_V | PTE_R | PTE_LIBRARY) < 0) {
					user_panic("[SPAWN] Unable To Map: va[%x] -> env[%x]\n",va,envid);
				}
			}
		}
	}

	

	var_dup(envid, !strcmp("my_shell.b", path));
	

	if (syscall_set_env_status(envid, ENV_RUNNABLE) < 0)
		user_panic("[SPAWN] Unable To Set Spawned Env RUNNABLE!\n");
	
	return envid;		
}

int spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}

