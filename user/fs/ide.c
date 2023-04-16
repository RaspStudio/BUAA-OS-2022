/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

#define IDE_SET_OFFSET 	0x13000000u
#define IDE_SET_DISKID 	0x13000010u
#define IDE_START_OP 	0x13000020u
#define IDE_OP_RESULT 	0x13000030u
#define IDE_BUFFER  	0x13004000u

int ide_op(u_int diskno, u_int offset, u_char opcode) {

	u_int word;
	u_char byte;

	/* Choose IDE disk (diskno) */
	word = diskno;
	if (syscall_write_dev((u_int)&word, IDE_SET_DISKID, sizeof(word)))
		user_panic("[IDE-OP] Failed: Choose IDE Disk (%s)", opcode == 0 ? "READ" : "WRITE");

	/* Set Offset for IDE operation */
	word = offset;
	if (syscall_write_dev((u_int)&word, IDE_SET_OFFSET, sizeof(word)))
		user_panic("[IDE-OP] Failed: Set OP Offset (%s)", opcode == 0 ? "READ" : "WRITE");

	/* Start IDE 'OP' operation */
	byte = opcode;
	if (syscall_write_dev((u_int)&byte, IDE_START_OP, sizeof(byte)))
		user_panic("[IDE-OP] Failed: Start OP (%s)", opcode == 0 ? "READ" : "WRITE");

	/* Check 'OP' Result */
	if (syscall_read_dev((u_int)&word, IDE_OP_RESULT, sizeof(word)))
		user_panic("[IDE-OP] Failed: Get OP Result (%s)", opcode == 0 ? "READ" : "WRITE");

	if (word == 0) {
		user_panic("[IDE-OP] Failed: IDE Returns Error Code %d (%s)", word, opcode == 0 ? "READ" : "WRITE");
	}

	return 0;
}

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.
// 	secno: start sector number.
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read.
//
// Post-Condition:
// 	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	int offset_begin = secno * BY2SECT;
	int offset_end = offset_begin + nsecs * BY2SECT;
	int offset;
	
	for (offset = 0; offset_begin + offset < offset_end; offset += BY2SECT) {
		ide_op(diskno, offset_begin + offset, 0);

		if (syscall_read_dev((u_long)dst + offset, IDE_BUFFER, BY2SECT))
			user_panic("[FS] Failed: Get Data From Disk Buffer");
	}
}

// Overview:
// 	write data to IDE disk.
//
// Parameters:
//	diskno: disk number.
//	secno: start sector number.
// 	src: the source data to write into IDE disk.
//	nsecs: the number of sectors to write.
//
// Post-Condition:
//	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	int offset_begin = secno * BY2SECT;
	int offset_end = offset_begin + nsecs * BY2SECT;

	int offset;

	/* For Auto-Tester */
	//writef("diskno: %d\n", diskno);

	for (offset = 0; offset_begin + offset < offset_end; offset += BY2SECT) {
		/* Fill Data To Disk Buffer */
		if (syscall_write_dev((u_long)src + offset, IDE_BUFFER, BY2SECT))
			user_panic("[FS] Failed: Fill Disk Buffer (WRITE)");

		ide_op(diskno, offset_begin + offset, 1);
	}
}
