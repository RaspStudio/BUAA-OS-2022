// #include "lib.h"
// #include <mmu.h>
// #include <env.h>
// #include <kerelf.h>

// #define TMPPAGE		(USTACKTOP)
// #define TMPPAGETOP	(TMPPAGE+BY2PG)

// // HELPER FUNCTION ///////////////////////////////////////////////////////////////

// /** MyOverView:
//  * 		Set Child's USTK, Initialize argc/argv/sp(at start).	
//  */
// int usr_init_stack(u_int child, const char **argv, u_int *init_esp) {
	
// 	/*=== Count Arg's num and size ===*/
// 	int arg_cnt;
// 	int arg_size = 0;
// 	for (arg_cnt=0; argv[arg_cnt] && argv[arg_cnt][0]; arg_cnt++) {
// 		arg_size += strlen(argv[arg_cnt]) + 1;
// 		//writef("Arg[%d]:\'%s\'->\'%x\'\n", arg_cnt, argv[arg_cnt], argv[arg_cnt]);
// 	}
// 	/*--- Check Data Size: arg_size +  ---*/
// 	if (ROUND(arg_size, 4) + sizeof(u_int) * (arg_cnt + 1 + 2) > BY2PG)
// 		return -E_NO_MEM;


// 	/*=== Get Args & Strings 's Start Address ===*/
// 	char *strings = (char*)TMPPAGETOP - arg_size;
// 	u_int *args = (u_int*)(TMPPAGETOP - ROUND(arg_size, 4) - sizeof(u_int) * (arg_cnt + 1));
// 	u_int *args_special = args - 2;

// 	/*--- Alloc TEMP Page for data ---*/
// 	int r;
// 	if ((r = syscall_mem_alloc(0, TMPPAGE, PTE_V | PTE_R)) < 0)
// 		return r;

// 	/*--- Copy Args & Strings To Stack ---*/
// 	u_int i, j, k;
// 	int offset = USTACKTOP - TMPPAGETOP;
// 	for (i = 0, k = 0; i < arg_cnt; i++) {
// 		/* Calc String's VA when in USTK */
// 		args[i] = (u_int)(&strings[k]) + offset;

// 		/* Copy String Data */
// 		for(j = 0; j < strlen(argv[i]); j++)
// 		{
// 			strings[k++] = argv[i][j];
// 		}
// 		strings[k++] = '\0';
// 	}

// 	args[arg_cnt] = 0;

// 	/*--- Prepare For 'argc' For Umain ---*/
// 	args_special[0] = arg_cnt;

// 	/*--- Prepare For 'argv' For Umain () ---*/
// 	args_special[1] = (u_int)args + offset;
	
// 	/*--- Set Initialize USTK Pointer ---*/
// 	*init_esp = (u_int)args_special + offset;

// 	/*--- Map Temp Page to child's USTK ---*/
// 	if ((r = syscall_mem_map(0, TMPPAGE, child, USTACKTOP-BY2PG, PTE_V | PTE_R)) < 0)
// 		goto error;

// 	/*--- Unmap Temp Page ---*/
// 	if ((r = syscall_mem_unmap(0, TMPPAGE)) < 0)
// 		goto error;
// 	return 0;

// error:
// 	syscall_mem_unmap(0, TMPPAGE);
// 	return r;
// }

// int usr_is_elf_format(u_char *binary){
// 	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
// 	if (ehdr->e_ident[0] == ELFMAG0 &&
//         ehdr->e_ident[1] == ELFMAG1 &&
//         ehdr->e_ident[2] == ELFMAG2 &&
//         ehdr->e_ident[3] == ELFMAG3) {
//         return 1;
//     }   
//     return 0;
// }

// // MY ICODE LOADER ///////////////////////////////////////////////////////////////
// u_char page_buf[BY2PG + 1];

// // MY ICODE LOADER MAIN CODE /////////////////////////////////////////////////////
// void __dump_page(u_long dstva, u_long addr) {
// 	int r;
// 	if (addr <= dstva && dstva < addr + BY2PG)
// 		for (r = 0; r < BY2PG; r++) {
// 			if (r % 4 == 0)
// 			writef(" ");
// 			if (r % 64 == 0)
// 			writef("\n0x%08x\t", dstva+r);
// 			writef("%02x", page_buf[r]);
// 		}
// 	writef("\n");
// }

// void __usr_load_binary(u_long dstOffset, u_long srcoffset, int fdnum, u_int32_t size) {
//     if (fdnum == -1) {
//         user_bzero((void*)(TMPPAGE + dstOffset), size);
//     } else {
// 		user_bzero(page_buf, sizeof(page_buf));
// 		seek(fdnum, srcoffset);
//         readn(fdnum, (void*)page_buf, size);
// 		user_bcopy(page_buf, (void*)(TMPPAGE + dstOffset), size);
//     }
// }

// void __usr_get_page(int envid, u_long dstva) {
// 	/* Try Get Existing Page */
// 	if (syscall_mem_map(envid, dstva, 0, TMPPAGE, PTE_V | PTE_R) < 0) {
// 		/* If No Exist, Alloc New And Map*/
// 		if (syscall_mem_alloc(envid, dstva, PTE_V | PTE_R) < 0)
// 			user_panic("[SPAWN] Failed To Alloc Page!");
// 		if (syscall_mem_map(envid, dstva, 0, TMPPAGE, PTE_V | PTE_R) < 0)
// 			user_panic("[SPAWN] Unable To Map Data To Target Env!");
// 	}
// 	//writef("GET EXIST:");
// }

// int __usr_load_data(int envid, u_long dstva, u_long srcoffset, int fdnum, u_int32_t datasize) {
//     //writef("[%s] 0x%08x ~ 0x%08x\t", fdnum == -1 ? "ZERO" : "LOAD", dstva, dstva + datasize);
	
// 	u_long i;
//     u_long offset = dstva - ROUNDDOWN(dstva, BY2PG);
//     u_long size;

//     for (i = 0; i < datasize; i += size) {
//         /* Get Temp Page */
//         __usr_get_page(envid, dstva+i);

//         /* Write Data */
//         size = MIN((datasize - i), (BY2PG - offset));
//         __usr_load_binary(offset, srcoffset + i, fdnum, size);
//         offset = 0;

// 		//writef("[%s] -> [%08x->0x1000]\n", fdnum > 0 ? "DATA" : "ZERO", ROUNDDOWN(dstva + i, BY2PG));
// 		//__dump_page(dstva + i, 0x400000);

// 		/* Unget Temp Page */
// 		if (syscall_mem_unmap(0, TMPPAGE) < 0)
// 			user_panic("[SPAWN] Unable To Unmap Temp Page!");
//     }
//     return 0;
// }

// void __usr_load_mapper(int envid, u_long dstva, u_long srcoffset, int fdnum, u_int sgsize, u_int bin_size) {
//     __usr_load_data(envid, dstva, srcoffset, fdnum, bin_size);
// 	//__usr_load_data(envid, dstva + bin_size, 0, -1, sgsize - bin_size);// TODO WHY BUG ?
// }


// /** MyOverView:
//  * 		My ELF Loader in USER MODE.
//  */
// int usr_load_elf(int envid, int fdnum) {
// 	seek(fdnum, 0);
// 	readn(fdnum, page_buf, sizeof(Elf32_Ehdr));
// 	Elf32_Ehdr ehdr = *((Elf32_Ehdr*)page_buf);

// 	u_int phdr_off = ehdr.e_phoff;
// 	u_int size = 0;

// 	if (UTEXT != ehdr.e_entry) 
// 		user_panic("[SPAWN] Invalid Entry: User Executable File!\n");

// 	int i;
// 	for (i = 0; i < ehdr.e_phnum; i++) {
// 		if (seek(fdnum, phdr_off + i * ehdr.e_phentsize) < 0) syscall_panic("1");
// 		if (readn(fdnum, page_buf, sizeof(Elf32_Phdr)) < 0) syscall_panic("2");
// 		Elf32_Phdr phdr = *((Elf32_Phdr*)page_buf);
// 		if (phdr.p_type == PT_LOAD) {
// 			u_long va = phdr.p_vaddr;
// 			u_int segSize = phdr.p_memsz;
// 			u_int codeSize = phdr.p_filesz;
// 			u_long srcoffset = phdr.p_offset;
// 			__usr_load_mapper(envid, va, srcoffset, fdnum, segSize, codeSize);
// 			//writef(B_MAGENTA "Loader" C_RESET ": Loaded %d Bytes of Data, %d Bytes of Bss.\n", codeSize, segSize - codeSize);
// 			size += segSize;
// 			// htr_load_elf(fdnum, &phdr, envid);
// 		}
// 	}

// 	return size;
// }
