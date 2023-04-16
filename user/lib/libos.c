#include "lib.h"
#include <mmu.h>
#include <env.h>

void exit() {
	//writef("EXIT [%d]\n", env->env_id);
	syscall_env_destroy(0);
}

/*--- User Space 'curenv' ---*/
struct Env *env;

void libmain(int argc, char **argv)
{
	// Set User Space 'curenv'
	env = &envs[ENVX(syscall_getenvid())];
	OUT writef(B_BLUE"New(S)"C_RESET"[%d]: %s\n", env->env_id, argv ? (argv[0] ? argv[0] : "No Arg0") : "No Arg");
	// Call User Main
	umain(argc, argv);

	// Exit and Destroy curenv
	exit();
}
