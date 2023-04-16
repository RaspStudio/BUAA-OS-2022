#include "../lib/lib.h"

VarMap *map = (VarMap*)VARSTART;

void print_var(int i) {
	int isRDOLY = map->values[i].mode & VAR_NOWRITE;
	int isGLOBL = map->values[i].mode & VAR_GLOBAL;
	int isSYS = map->values[i].mode & VAR_SYS;
	int j;
	writef("$%s", map->keys[i]);
	for (j = strlen(map->keys[i]); j < MAXVARKEY; j++) writef(" ");
	writef("\t%s%s%s", isRDOLY ? VAR_STR_NOW : "", isGLOBL ? VAR_STR_GLO : "", isSYS ? VAR_STR_SYS : "");
	writef("\t%s\n", map->values[i].val);
}

void print_vars() {
	int i;
	int flag = 0;
	writef(B_MAGENTA "VarSystem" C_RESET); 
	writef(": All Variable are below.(For Curenv)\n");
	for (i = 0; i < MAXVAR; i++) {
		if (map->keys[i][0] != '\0') {
			if (map->values[i].mode & VAR_GLOBAL || map->values[i].envid == env->env_id) {
				print_var(i);
				flag = 1;
			}
		}
	}
	if (flag == 0) {
		writef("No Valid Variable For Current Env Yet!\n");
	}
}

int getVar(const char *key) {
	int i;
	for (i = 0; i < MAXVAR; i++) {
		if (strcmp(key, map->keys[i]) == 0) {
			if (map->values[i].mode & VAR_GLOBAL) {
				return i;
			}
			if (map->values[i].envid == env->env_id) {
				return i;
			}
		}
	}
	return -1;
}

int getNew() {
	int i;
	for (i = 0; i < MAXVAR; i++) {
		if (map->keys[i][0] == 0) {
			return i;
		}
	}
	return -1;
}

void var_dup(int envid, int isShell) {
	if (!isShell) {
		syscall_mem_map(0, VARSTART, envid, VARSTART, PTE_V);
	} else {
		syscall_mem_map(0, VARSTART, envid, VARSTART, PTE_V | PTE_R | PTE_COW);
		syscall_mem_map(0, VARSTART, 0, VARSTART, PTE_V | PTE_R | PTE_COW);
	}
}

void var_init() {
	assert(VARMAP2PG == 1);
	if (syscall_mem_alloc(0, VARSTART, PTE_V | PTE_R)) {
		writef(B_RED "VarSystem" C_RESET );
    	writef(": Initialization Failed.\n", VARMAP2PG);
	}
	
	// writef(B_MAGENTA "VarSystem" C_RESET );
    // writef(": Initialization Finished, Using %d Pages.\n", VARMAP2PG);
}

int var_add(char mode, const char *key, const char *val) {
	//writef("CALL ADD:(%s|%s)\n", key?key:"NULL", val?val:"NULL");
	if ((key != NULL && strlen(key) >= MAXVARKEY) 
	|| (val != NULL && strlen(val) >= MAXVARVALUE)) {
		writef(B_RED "VarSystem" C_RESET );
		writef(": Var Length Limit Exceed!\n");
		return -E_INVAL;;
	}

	if (mode == 0 && key == 0 && val == 0) {
		print_vars();
	} else if (key != 0) {
		int i = getVar(key);
		//writef("Debug:%d",i);
		if (i < 0) {
			/* Create New Var */
			if ((i = getNew()) < 0) {
				writef(B_RED "VarSystem" C_RESET );
				writef(": Cannot Get New Space!\n");
				return -E_NO_MEM;
			}
			//writef("~~~~~~~~~~~~~~~~~~%d", env->env_id);
			int id = env->env_id;
			map->values[i].envid = id;
			//writef("~~~~~~~~~~~~~~~~~~");
			map->values[i].mode = mode;
			
			strcpy(map->keys[i], key);
			if (val)
				strcpy(map->values[i].val, val);
			writef(B_MAGENTA "VarSystem" C_RESET );
			writef(": Added \'$%s\' %s%s%s\n", key, val ? "as \'" : "", val ? val : "", val ? "\'." : ".");
		} else {
			/* Update Old Var */
			if (map->values[i].mode & VAR_NOWRITE) {
				writef(B_RED "VarSystem" C_RESET );
				writef(": Cannot Modify READONLY Variable\n");
				return -E_INVAL;
			} else {
				map->values[i].mode |= mode;
				if (val)
					strcpy(map->values[i].val, val);
				writef(B_MAGENTA "VarSystem" C_RESET);
				writef(": Changed \'$%s\' %s%s%s\n", key, val ? "to \'" : "", val ? val : "", val ? "\'." : ".");
			}
		}
	} else {
		writef(B_RED "VarSystem" C_RESET );
		writef(": Wrong Usage (declare [-xr] [NAME[=VAL]])\n");
		return -E_INVAL;
	}
	
	return 0;
}

int var_del(const char *key) {
	
	int i = getVar(key);
	if (i >= 0) {
		if ((map->values[i].mode & VAR_NOWRITE) || (map->values[i].mode & VAR_SYS)) {
			writef(B_RED "VarSystem" C_RESET );
			writef(": Access Denied, Read Only!\n", key);
			return -E_INVAL;
		} else {
			map->keys[i][0] = 0;
			map->values[i].envid = 0;
			map->values[i].mode = 0;
			map->values[i].val[0] = 0;
			writef(B_MAGENTA "VarSystem" C_RESET );
			writef(": Successfully Deleted $%s!\n", key);
			return -E_INVAL;
		}
	} else {
		writef(B_RED "VarSystem" C_RESET );
		writef(": No Such Variable: %s!\n", key);
		return -E_INVAL;
	}
}

int var_get(const char *key, char *dest) {

	int i = getVar(key);
	if (i >= 0) {
		strcpy(dest, map->values[i].val);
		return 0;
	} else {
		writef(B_RED "VarSystem" C_RESET);
		writef(": No Such Variable: %s!\n", key);
		return -E_INVAL;
	}
}

