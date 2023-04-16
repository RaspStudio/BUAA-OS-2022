#include "lib.h"
#include <env.h>
void
wait(u_int envid)
{
	OUT writef(B_GREEN"Wait"C_RESET"[%d]: I'am Waiting [%d]\n", env->env_id, envid);
	struct Env *e = &envs[ENVX(envid)];
	while(e->env_id == envid && e->env_status != ENV_FREE)
		syscall_yield();
	/* When Target Env hasn't been freed or blocked, yield */
	OUT writef(B_GREEN"Wait"C_RESET"[%d]: Waiting [%d] Finished.\n", env->env_id, envid);
}


