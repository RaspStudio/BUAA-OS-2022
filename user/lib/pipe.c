#include "lib.h"
#include <mmu.h>
#include <env.h>
#define debug 0

static int pipeclose(struct Fd*);
static int piperead(struct Fd *fd, void *buf, u_int n, u_int offset);
static int pipestat(struct Fd*, struct Stat*);
static int pipewrite(struct Fd *fd, const void *buf, u_int n, u_int offset);

struct Dev devpipe = {
.dev_id=	'p',
.dev_name=	"pipe",
.dev_read=	piperead,
.dev_write=	pipewrite,
.dev_close=	pipeclose,
.dev_stat=	pipestat,
};

#define BY2PIPE 32		// small to provoke races

struct Pipe {
	u_int p_rpos;		// read position
	u_int p_wpos;		// write position
	u_int p_id;
	u_char p_buf[BY2PIPE];	// data buffer
};

/** MyOverView:
 * 		Allocate {ReaderFd, WriterFd} to 'pfd' array.
 */
int pipe(int pfd[2])
{
	int r, va;
	struct Fd *fd0, *fd1;

	// allocate the file descriptor table entries
	if ((r = fd_alloc(&fd0)) < 0
	||  (r = syscall_mem_alloc(0, (u_int)fd0, PTE_V|PTE_R|PTE_LIBRARY)) < 0)
		goto err;

	if ((r = fd_alloc(&fd1)) < 0
	||  (r = syscall_mem_alloc(0, (u_int)fd1, PTE_V|PTE_R|PTE_LIBRARY)) < 0)
		goto err1;

	// allocate the pipe structure as first data page in both
	va = fd2data(fd0);
	if ((r = syscall_mem_alloc(0, va, PTE_V|PTE_R|PTE_LIBRARY)) < 0)
		goto err2;
	if ((r = syscall_mem_map(0, va, 0, fd2data(fd1), PTE_V|PTE_R|PTE_LIBRARY)) < 0)
		goto err3;

	// set up pipe
	struct Pipe *p = (struct Pipe*)fd2data(fd0);
	p->p_rpos = 0;
	p->p_wpos = 0;
	p->p_id = env->env_id;

	// set up fd structures
	fd0->fd_dev_id = devpipe.dev_id;
	fd0->fd_omode = O_RDONLY;

	fd1->fd_dev_id = devpipe.dev_id;
	fd1->fd_omode = O_WRONLY;

	if(debug)writef("[%08x] pipecreate \n", env->env_id, (* vpt)[VPN(va)]);

	pfd[0] = fd2num(fd0);
	pfd[1] = fd2num(fd1);
	return 0;

err3:	syscall_mem_unmap(0, va);
err2:	syscall_mem_unmap(0, (u_int)fd1);
err1:	syscall_mem_unmap(0, (u_int)fd0);
err:	return r;
}

static int _pipeisclosed(struct Fd *fd, struct Pipe *p) {
	int fd_ref, pipe_ref, runs;
	
	/*--- Get Page Ref (Loop to ensure NOT_INTERRUPTED) ---*/
	do {
		runs = env->env_runs;
		fd_ref = pageref(fd);
		pipe_ref = pageref(p);
	} while (env->env_runs == runs);

	/*--- Pipe_Ref = WFd_ref + RFd_Ref (When Open)---*/
	if (fd_ref != pipe_ref) if(debug)writef("[PIPE NOT CLOSED]=[%d]fd<%d>pipe[%d]\n", fd_ref, p->p_id, pipe_ref);
	return (fd_ref == pipe_ref) ? 1 : 0;
}


/** MyOverView:
 * 		Assert 'fdnum' is a Pipe's End:
 * 			Look fd's shared page's ref, check its pp_ref.
 */
int pipeisclosed(int fdnum)
{
	struct Fd *fd;
	struct Pipe *p;
	int r;

	if ((r = fd_lookup(fdnum, &fd)) < 0)
		return r;

	p = (struct Pipe*)fd2data(fd);
	return _pipeisclosed(fd, p);
}

/** MyOverView:
 * 		Read As much as we can, max to 'n'.
 * 		When reached max or no more to read, 
 *     	return the bytes we have read.
 */
static int piperead(struct Fd *fd, void *vbuf, u_int n, u_int offset)
{
	if(debug)writef("PIPE READ START[%d]\n", env->env_id);
	struct Pipe *p = (struct Pipe*)fd2data(fd);
	char *rbuf = (char*)vbuf;

	/*--- Wait Till Write Success ---*/
	while(p->p_rpos >= p->p_wpos) 
		if (_pipeisclosed(fd, p))
			break; 
		else syscall_yield();

	while (p->p_rpos < p->p_wpos && (rbuf - (char*)vbuf) < n)
		*(rbuf++) = p->p_buf[(p->p_rpos++) % BY2PIPE];
	
	if(debug)writef("PIPE READ END[%d]\n", p->p_id);
	return rbuf - (char*)vbuf;
}

static int pipewrite(struct Fd *fd, const void *vbuf, u_int n, u_int offset)
{
	// Your code here.  See the lab text for a description of what 
	// pipewrite needs to do.  Write a loop that transfers one byte
	// at a time.  Unlike in read, it is not okay to write only some
	// of the data.  If the pipe fills and you've only copied some of
	// the data, wait for the pipe to empty and then keep copying.
	// If the pipe is full and closed, return 0.
	// Use _pipeisclosed to check whether the pipe is closed.
	int dbg = 0;
	struct Pipe *p = (struct Pipe*)fd2data(fd);
	char *wbuf = (char*)vbuf;
	
	if (dbg) writef("[PIPE-W]\tCalled Write:[%d->%d]", p->p_rpos, p->p_wpos);

	while (wbuf - (char*)vbuf < n) {
		/*--- Wait Till Can Write ---*/
		while (p->p_wpos - p->p_rpos >= BY2PIPE)
			if (_pipeisclosed(fd, p))
				/*--- Failed To Write ---*/
				return 0;
			else syscall_yield();
		
		/*--- Copy Content ---*/
		p->p_buf[(p->p_wpos++) % BY2PIPE] = *(wbuf++);
	}
	if(debug)writef("PIPE WRITE[%d]\n", p->p_id);
	return n;
}

static int
pipestat(struct Fd *fd, struct Stat *stat)
{
	//struct Pipe *p;

	user_panic("UNIMELEMENTED: pipestat");
	return 0;
}

static int
pipeclose(struct Fd *fd)
{
	syscall_mem_unmap(0, (u_int)fd);
	syscall_mem_unmap(0, fd2data(fd));
	if(debug)writef("[PIPE ONE CLOSED]=<%d>\n", ((struct Pipe*)fd2data(fd))->p_id);
	return 0;
}

