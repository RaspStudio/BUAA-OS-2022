// User-level IPC library routines

#include "lib.h"
#include <mmu.h>
#include <env.h>

extern struct Env *env;


/** MyOverview:
 * 		Current env sent 'val' to 'whom'.
 * 		At the same time share page of 'srcva' to 'whom', 
 * 		with perm of 'perm'.
 * 
 *  Note:
 * 		Always returns with success. (Fail -> self panic)
 */
void ipc_send(u_int whom, u_int val, u_int srcva, u_int perm) {
	int r;

	while ((r = syscall_ipc_can_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
		//writef("QQ");
	}

	if (r == 0) {
		return;
	}

	user_panic("error in ipc_send: %d", r);
}

/** MyOverView:
 * 		'whom' send current env a value.
 * 
 *  Note:
 * 		Value to be sent: On Return
 * 		Sender's envid	: -> *whom
 * 		Page perm recvd : -> *perm
 * 
 *  Original Hint:
 * 		use env to discover the value and who sent it.
 */
u_int ipc_recv(u_int *whom, u_int dstva, u_int *perm) {
	
	syscall_ipc_recv(dstva);

	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	return env->env_ipc_value;
}

