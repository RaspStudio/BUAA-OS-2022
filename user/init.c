#include "lib.h"

struct {
	char msg1[5000];
	char msg2[1000];
} data = {
	"this is initialized data",
	"so is this"
};

char bss[6000];

static char stdarg[3][100] = {"init", "initarg1", "initarg2"};

inline int MY_MUL(int a,int b)
{
	int i;
	int sum;
	for(i=0,sum=0;i<b;sum+=a,i++); 
	return sum;
}

static int
sum(char *s, int n)
{
	int i, tot;

	tot = 0;
	for(i=0; i<n; i++)
		tot ^= MY_MUL(i,s[i]);
	return tot;
}
		
void
umain(int argc, char **argv) {
	//usr_title("-= Init =-");

	// Check Data Loader
	if (sum((char*)&data, sizeof data) != 0xf989e) user_panic("Loader's Data Fault.");

	// Check Bss Loader
	if (sum(bss, sizeof bss) != 0) user_panic("Loader's Bss Fault.");

	// Check Arg Loader (Stack)
	if (argc != 3) user_panic("Loader's Stack Fault.");
	int i;
	for (i=0; i<argc; i++) if (strcmp(stdarg[i], argv[i]) != 0) user_panic("Loader's Stack Fault.");

	// Check Cleared, Run Shell:
	// Prepare Stdin / Stdout for Shell.
	int r;
	if ((r = opencons()) < 0)
		user_panic("Cannot Open Console. Error Code: %d", r);
	if (r != 0)
		user_panic("Console Used Wrong FD number: %d", r);
	if ((r = dup(0, 1)) < 0)
		user_panic("Failed To Dump Console. Error Code: %d", r);

	//wait(spawnl("ls.b","ls",NULL));

	var_init();
	history_init();
	var_add(VAR_GLOBAL | VAR_SYS, VAR_WORKDIR, "/");
	
	// Try Start Shell
	for (;;) {
		//r = spawnl("sh.b", "sh", (char*)0);
		r = spawnl("mysh.b", "sup", NULL);
		if (r < 0) {
			writef("[SPAWN]\tRestart Spawn Shell...\n", r);
			continue;
		}
		wait(r);
	}
}
