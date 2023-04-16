#include "lib.h"
#include "fd.h"
#include <mmu.h>
#include <env.h>


static struct Dev *devtab[] = {
	&devfile,
	&devcons,
	&devpipe,
	NULL
};

// DEVICE LOOK-UP /////////////////////////////////////////////////////////////////

/** MyOverView:
 * 		Look up 'dev_id' in devtab, and let '*dev = devtab[i]'.
 * 
 *  Post-Condition:
 * 		Return 0 : success, and (*dev)->dev_id == dev_id.   
 * 	    -E_INVAL : No Such Device.
 */
int dev_lookup(int dev_id, struct Dev **dev) {
	int i;

	for (i = 0; devtab[i]; i++)
		if (devtab[i]->dev_id == dev_id) {
			*dev = devtab[i];
			return 0;
		}

	writef("[%08x] unknown device type %d\n", env->env_id, dev_id);
	return -E_INVAL;
}

// FILE DESCRIPTOR OPERATION /////////////////////////////////////////////////////

/** MyOverView:
 * 		Get First Unmapped FD Page, set *fd = INDEX2FD(freeFd)
 * 		(Let *fd be a free FD Mapping (VA in kuseg))
 *  
 *  Post-Condition:
 * 		Return 0 on success.
 * 
 *  Note:
 * 		assignable: nothing except *dev.
 */
int fd_alloc(struct Fd **fd) {
	u_int va;
	u_int fdno;

	/*----- Walk Through All FD Space -----*/
	for (fdno = 0; fdno < MAXFD - 1; fdno++) {
		/* Get FD's Va Map */
		va = INDEX2FD(fdno);

		/* Check if VPD is valid */
		if (((*vpd)[PDX(va)] & PTE_V) == 0) {
			/* No Valid VPD -> fdno is free */
			*fd = (struct Fd *)va;
			//writef(B_MAGENTA"Device"C_RESET": Alloced %d.\n", fdno);
			return 0;
		}
		
		/* Check if VPT is valid */
		if (((*vpt)[PPN(va)] & PTE_V) == 0) {
			/* No Valid VPE -> fdno is free */
			*fd = (struct Fd *)va;
			//writef(B_MAGENTA"Device"C_RESET": Alloced %d.\n", fdno);
			return 0;
		}
	}

	/*----- Every FD is used, Return error -----*/
	return -E_MAX_OPEN;
}

/** MyOverView:
 * 		Unmap fd's VA, thus we consider the fd
 * 		is freed and closed.
 */ 
void fd_close(struct Fd *fd) {
	if (syscall_mem_unmap(0, (u_int)fd)) {
		user_panic("[FD] File Descriptor Close Failed: Unable to unmap page!");
	}
	//writef("COLOS CLOSE CLOSE %d\n", fd2num(fd));
}

/** MyOverView:
 * 		If fdnum is valid, and its fd is in use (mapped),
 * 		then set *fd = INDEX2FD(fdnum).
 * 
 *  Post-Condition:
 * 		Return -E_INVAL: fdnum Out of range.
 * 			             or fd is not in use (unmapped)
 */
int fd_lookup(int fdnum, struct Fd **fd) {
	// Check that fdnum is in range and mapped.  If not, return -E_INVAL.
	// Set *fd to the fd page virtual address.  Return 0.
	u_int va;

	if (fdnum >= MAXFD) {
		return -E_INVAL;
	}

	va = INDEX2FD(fdnum);

	if (((* vpt)[va / BY2PG] & PTE_V) != 0) {	//the fd is used
		*fd = (struct Fd *)va;
		return 0;
	}

	return -E_INVAL;
}

// FILE DESCRIPTOR ADDRESS/INDEX OPERATION ///////////////////////////////////////

u_int fd2data(struct Fd *fd) {
	return INDEX2DATA(fd2num(fd));
}

int fd2num(struct Fd *fd) {
	return ((u_int)fd - FDTABLE) / BY2PG;
}

int num2fd(int fd) {
	return fd * BY2PG + FDTABLE;
}

// XXXXXXXXXXXXX //////////////////////////////////////////////////////////////////

int close(int fdnum) {
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
		||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
		return r;
	}

	r = (*dev->dev_close)(fd);
	fd_close(fd);
	return r;
}

void
close_all(void)
{
	int i;

	for (i = 0; i < MAXFD; i++) {
		close(i);
	}
}

/** MyOverView:
 * 		Make 'newfdnum' points to 'oldfdnum''s file.
 */
int dup(int oldfdnum, int newfdnum)
{
	int r;
	struct Fd *oldfd, *newfd;

	/*--- Find Old File Descriptor ---*/
	if ((r = fd_lookup(oldfdnum, &oldfd)) < 0) {
		return r;
	}

	/*--- If Newfdnum in use, close it ---*/
	close(newfdnum);

	/*--- Get Newfdnum's FD struct ---*/
	newfd = (struct Fd *)INDEX2FD(newfdnum);

	/*--- Get Both FD's data space ---*/
	u_int ova = fd2data(oldfd);
	u_int nva = fd2data(newfd);

	/*--- If ova's pagetable is not null: ---*/
	int i;
	if ((* vpd)[PDX(ova)]) {
		
		/* Walk through oldFD's data space */
		for (i = 0; i < PDMAP; i += BY2PG) {
			/* Get current page's pte */
			u_int pte = (* vpt)[VPN(ova + i)];

			/* If current ova's page is valid: */
			if (pte & PTE_V) {
				/* Map this old data space to NewFD's data space */
				if ((r = syscall_mem_map(0, ova + i, 0, nva + i,
										 pte & (PTE_V | PTE_R | PTE_LIBRARY))) < 0) {
					goto err;
				}
			}
		}
	}

	/*--- Let New FD struct use Old FD's page ---*/
	if ((r = syscall_mem_map(0, (u_int)oldfd, 0, (u_int)newfd,
							 ((*vpt)[VPN(oldfd)]) & (PTE_V | PTE_R | PTE_LIBRARY))) < 0) {
		goto err;
	}

	return newfdnum;

err:
	syscall_mem_unmap(0, (u_int)newfd);

	for (i = 0; i < PDMAP; i += BY2PG) {
		syscall_mem_unmap(0, nva + i);
	}

	return r;
}

// Overview:
//	Read 'n' bytes from 'fd' at the current seek position into 'buf'.
//
// Post-Condition:
//	Update seek position.
//	Return the number of bytes read successfully.
//		< 0 on error
/*** exercise 5.9 ***/
int
read(int fdnum, void *buf, u_int n)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	// Similar to 'write' function.
	// Step 1: Get fd and dev.
	if ((r = fd_lookup(fdnum, &fd)) < 0
		||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
		return r;
	}

	// Step 2: Check open mode.
	if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
		writef("[%08x] read %d -- bad mode\n", env->env_id, fdnum);
		return -E_INVAL;
	}

	if (debug) writef("read %d %p %d via dev %s\n",
						  fdnum, buf, n, dev->dev_name);

	// Step 3: Read starting from seek position.
	r = (*dev->dev_read)(fd, buf, n, fd->fd_offset);

	// Step 4: Update seek position and set '\0' at the end of buf.
	if (r >= 0) {
		fd->fd_offset += r;
		((char*)buf)[r] = '\0';
	}

	return r;
}

int
readn(int fdnum, void *buf, u_int n)
{
	int m, tot;

	for (tot = 0; tot < n; tot += m) {
		m = read(fdnum, (char *)buf + tot, n - tot);

		if (m < 0) {
			return m;
		}

		if (m == 0) {
			break;
		}
	}

	return tot;
}

int
write(int fdnum, const void *buf, u_int n)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
		||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
		return r;
	}

	if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
		writef("[%08x] write %d -- bad mode\n", env->env_id, fdnum);
		return -E_INVAL;
	}

	if (debug) writef("write %d %p %d via dev %s\n",
						  fdnum, buf, n, dev->dev_name);

	r = (*dev->dev_write)(fd, buf, n, fd->fd_offset);

	if (r > 0) {
		fd->fd_offset += r;
	}

	return r;
}

int
seek(int fdnum, u_int offset)
{
	int r;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	fd->fd_offset = offset;
	return 0;
}


int fstat(int fdnum, struct Stat *stat)
{
	int r;
	struct Dev *dev;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0
		||  (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
		return r;
	}

	stat->st_name[0] = 0;
	stat->st_size = 0;
	stat->st_isdir = 0;
	stat->st_dev = dev;
	return (*dev->dev_stat)(fd, stat);
}

int
stat(const char *path, struct Stat *stat)
{
	int fd, r;

	if ((fd = open(path, O_RDONLY)) < 0) {
		return fd;
	}

	r = fstat(fd, stat);
	close(fd);
	return r;
}
