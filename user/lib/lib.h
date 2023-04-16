#ifndef LIB_H
#define LIB_H
#include "fd.h"
#include "pmap.h"
#include <mmu.h>
#include <trap.h>
#include <env.h>
#include <args.h>
#include <unistd.h>
#include <color.h>
/////////////////////////////////////////////////////head
extern void umain();
extern void libmain();
extern void exit();

extern struct Env *env;


#define USED(x) (void)(x)
//////////////////////////////////////////////////////printf
#include <stdarg.h>
//#define		LP_MAX_BUF	80

void user_lp_Print(void (*output)(void *, const char *, int),
				   void *arg,
				   const char *fmt,
				   va_list ap);

void writef(char *fmt, ...);
void writef_line(char *fmt, ...);
void writef_base(const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);

void _user_panic(const char *, int, const char *, ...)
__attribute__((noreturn));

#define user_panic(...) _user_panic(__FILE__, __LINE__, __VA_ARGS__)


/////////////////////////////////////////////////////fork spawn
int spawn(char *prog, char **argv);
int spawnl(char *prot, char *args, ...);
int fork(void);
void pgfault(u_int va);

void user_bcopy(const void *src, void *dst, size_t len);
void user_bzero(void *v, u_int n);
//////////////////////////////////////////////////syscall_lib
extern int msyscall(int, int, int, int, int, int);

void syscall_putchar(char ch);
u_int syscall_getenvid(void);
void syscall_yield(void);
int syscall_env_destroy(u_int envid);
int syscall_set_pgfault_handler(u_int envid, void (*func)(void),
								u_int xstacktop);
int syscall_mem_alloc(u_int envid, u_int va, u_int perm);
int syscall_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva,
					u_int perm);
int syscall_mem_unmap(u_int envid, u_int va);

inline static int syscall_env_alloc(void)
{
    return msyscall(SYS_env_alloc, 0, 0, 0, 0, 0);
}

int syscall_set_env_status(u_int envid, u_int status);
int syscall_set_trapframe(u_int envid, struct Trapframe *tf);
void syscall_panic(char *msg);
int syscall_ipc_can_send(u_int envid, u_int value, u_int srcva, u_int perm);
void syscall_ipc_recv(u_int dstva);
int syscall_cgetc();

int syscall_write_dev(u_int va, u_int dev, u_int len);
int syscall_read_dev(u_int va, u_int dev, u_int len);

// USER IPC OPERATIONS //////////////////////////////////////////////////////////////////////////

/** MyOverview:
 * 		Current env sent 'val' to 'whom'.
 * 		At the same time share page of 'srcva' to 'whom', 
 * 		with perm of 'perm'.
 * 
 *  Note:
 * 		Always returns with success. (Fail -> self panic)
 */
void	ipc_send(u_int whom, u_int val, u_int srcva, u_int perm);

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
u_int	ipc_recv(u_int *whom, u_int dstva, u_int *perm);

// USER CODE LOADER /////////////////////////////////////////////////////////////////////////////
// int usr_init_stack(u_int child, const char **argv, u_int *init_esp);
// int usr_is_elf_format(u_char *binary);
// int usr_load_elf(int envid, int fdnum);

// MYS CFILE VAR ///////////////
void var_init();
void var_dup(int envid, int isShell);
int var_add(char mode, const char *key, const char *val);
int var_del(const char *key);
int var_get(const char *key, char *dest);

// MYS CFILE HISTORY ////////////
int history_open();
void history_init();
void history_add(int fd, const char *op);
int history_up(int fd, char *dst);
int history_down(int fd, char *dst);
void history_print();

// MYS CFILE UI ////////////////////////////////////////////////////////////////////////////////////
void mys_title();
void usr_title(const char *name);
void mys_startln(const char *path, int flag);

#define SHELL_LOG 0
#define OUT \
	if(SHELL_LOG)

// MYS CFILE IO /////////////////////////////////////////////////////////////////////////////////

int redirect_stdin(const char *path);
int redirect_stdout(const char *path);
//int redirect(const char *srcPath, const char *dstPath);

void mys_nextln(char *buf, u_long size, int history_fd);

void mys_cd(int argc, char **argv);
void mys_declare(int argc, char **argv);
void mys_unset(int argc, char **argv);

// MYS STRING ///////////////////////////////////////////////////////////////////////////////////

int strlen(const char *s);
char *strcpy(char *dst, const char *src);
const char *strchr(const char *s, char c);
void *memcpy(void *destaddr, void const *srcaddr, u_int len);
int strcmp(const char *p, const char *q);
char *strlcat(char *front, const char *back, unsigned long max_len);

char *my_mid_text(const char *text);
char *my_mid_ltext(const char *text, u_long length);
char *my_console_line(const char *text, const char *line_color, const char *text_color);
/////////////////////////////////////////////////////////////////////////////////////////////////


// wait.c
void wait(u_int envid);

// console.c
int opencons(void);
int iscons(int fdnum);

// pipe.c
int pipe(int pfd[2]);
int pipeisclosed(int fdnum);

// pageref.c
int	pageref(void *);

// pgfault.c
void set_pgfault_handler(void (*fn)(u_int va));

// fprintf.c
int fwritef(int fd, const char *fmt, ...);

// fsipc.c
int	fsipc_open(const char *, u_int, struct Fd *);
int	fsipc_map(u_int, u_int, u_int);
int	fsipc_set_size(u_int, u_int);
int	fsipc_close(u_int);
int	fsipc_dirty(u_int, u_int);
int	fsipc_remove(const char *);
int	fsipc_sync(void);
int	fsipc_incref(u_int);

// fd.c
int	close(int fd);
int	read(int fd, void *buf, u_int nbytes);
int	write(int fd, const void *buf, u_int nbytes);
int	seek(int fd, u_int offset);
void	close_all(void);
int	readn(int fd, void *buf, u_int nbytes);
int	dup(int oldfd, int newfd);
int fstat(int fdnum, struct Stat *stat);
int	stat(const char *path, struct Stat *);

// file.c
int	open(const char *path, int mode);
int	read_map(int fd, u_int offset, void **blk);
int	remove(const char *path);
int	ftruncate(int fd, u_int size);
int	sync(void);



#define user_assert(x)	\
	do {	if (!(x)) user_panic("assertion failed: %s", #x); } while (0)

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

#endif
