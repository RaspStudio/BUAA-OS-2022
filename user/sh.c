#include "lib.h"
#include <args.h>

int debug_ = 0;


/////////////////////////////////
//
// get the next token from string s
// set *p1 to the beginning of the token and
// *p2 just past the token.
// return:
//	0 for end-of-string
//	> for >
//	| for |
//	w for a word
//
// eventually (once we parse the space where the nul will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2) {
	int t;

	/*--- Check Buffer Validity ---*/
	if (s == 0)
		return 0;

	/*--- Buffer Valid, Clean Dest ---*/
	*p1 = 0;
	*p2 = 0;

	/*--- Skip Whitespace ---*/
	while(strchr(WHITESPACE, *s))
		*s++ = 0;

	/*--- Check End of line ---*/
	if(*s == 0) 
		return 0;
	
	/*--- Check Symbols ---*/
	if(strchr(SYMBOLS, *s)){
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}


	*p1 = s;

	/*--- Skip Whitespace, No Whitespace and symbols ---*/
	while(*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	

	*p2 = s;

	if (debug_ > 1) {
		t = **p2;
		**p2 = 0;
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 16
void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, i, r, p[2], rightpipe;
	rightpipe = 0;
	gettoken(s, 0);
again:
	argc = 0;
	for(;;){
		c = gettoken(0, &t);
		switch(c){
		case 0:
			goto runit;
		case 'w':
			if(argc == MAXARGS){
				writef("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w'){
				writef("syntax error: < not followed by word\n");
				exit(-1);
			}
			// Your code here -- open t for reading,
			if ((r = redirect_stdin(t)) < 0)
				exit(r);
			// dup it onto fd 0, and then close the fd you got.
			break;
		case '>':
			if (gettoken(0, &t) != 'w'){
				writef("syntax error: > not followed by word\n");
				exit(-1);
			}
			// Your code here -- open t for writing,
			if ((r = redirect_stdout(t)) < 0)
				exit(r);
			// dup it onto fd 1, and then close the fd you got.
			break;
		case '|':
			// Your code here.
			// 	First, allocate a pipe.
			pipe(p);
			//	Then fork.
			if ((rightpipe = fork()) != 0) {
				/* Father Thread */
			//	the parent runs the left side of the pipe:
			//		dup the write end of the pipe onto 1
			//		close the write end of the pipe
			//		close the read end of the pipe
			//		set "rightpipe" to the child envid
			//		goto runit, to execute this piece of the pipeline
			//			and then wait for the right side to finish	
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				goto runit;
			} else {
				/* Son Thread */
			//	the child runs the right side of the pipe:
			//		dup the read end of the pipe onto 0
			//		close the read end of the pipe
			//		close the write end of the pipe
			//		goto again, to parse the rest of the command line	
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				goto again;
			}
			break;
		}
	}

runit:
	if(argc == 0) {
		if (debug_) writef("EMPTY COMMAND\n");
		return;
	}
	argv[argc] = 0;
	if (1) {
		writef("[%08x] SPAWN:", env->env_id);
		for (i=0; argv[i]; i++)
			writef(" %s", argv[i]);
		writef("\n");
	}

	if ((r = spawn(argv[0], argv)) < 0)
		writef("spawn %s: %e\n", argv[0], r);
	close_all();
	if (r >= 0) {
		if (debug_) writef("[%08x] WAIT %s %08x\n", env->env_id, argv[0], r);
		wait(r);
	}
	if (rightpipe) {
		if (debug_) writef("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
		wait(rightpipe);
	}

	exit();
}

void
readline(char *buf, u_int n)
{
	int i, r;

	r = 0;
	for(i=0; i<n; i++){
		if((r = read(0, buf+i, 1)) != 1){
			if(r < 0)
				writef("read error: %e", r);
			exit();
		}
		if(buf[i] == '\b'){
			if(i > 0)
				i -= 2;
			else
				i = 0;
		}
		if(buf[i] == '\r' || buf[i] == '\n'){
			buf[i] = 0;
			return;
		}
	}
	writef("line too long\n");
	while((r = read(0, buf, 1)) == 1 && buf[0] != '\n')
		;
	buf[0] = 0;
}	

char buf[1024];

void
usage(void)
{
	writef("usage: sh [-dix] [command-file]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int r, interactive, echocmds;
	interactive = '?';
	echocmds = 0;
	
	usr_title("OLD MOS SHELL");
	// writef("WJDWIHDIWHI:::%d\n",argc);
	// for (r = 0; r < argc; r++) {
	// 	writef("%s\n", argv[r]);
	// }
	
	// ARGBEGIN{
	// case 'd':
	// 	debug_++;
	// 	break;
	// case 'i':
	// 	interactive = 1;
	// 	break;
	// case 'x':
	// 	echocmds = 1;
	// 	break;
	// default:
	// 	usage();
	// }ARGEND


	// writef("WJDWIHDIWHI:::%d\n",argc);

	// if(argc > 1)
	// 	usage();
	// if(argc == 1){
	// 	close(0);
	// 	if ((r = open(argv[1], O_RDONLY)) < 0)
	// 		user_panic("open %s: %e", r);
	// 	user_assert(r==0);
	// }
	// if(interactive == '?')
	// 	interactive = iscons(0);
	
	for(;;){
		if (interactive)
			mys_startln(NULL);
		readline(buf, sizeof buf);
		
		if (buf[0] == '#')
			continue;
		if (echocmds)
			fwritef(1, "# %s\n", buf);
		if ((r = fork()) < 0)
			user_panic("fork: %e", r);
		if (r == 0) {
			runcmd(buf);
			exit();
			return;
		} else
			wait(r);
	}
}

