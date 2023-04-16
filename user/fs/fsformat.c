/*
 * BUAA MIPS OS Kernel file system format
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

/* Prevent inc/types.h, included from inc/fs.h,
 * From attempting to redefine types defined in the host's inttypes.h. */
#define _INC_TYPES_H_
#define BY2PG       4096        // bytes to a page
#include "../include/fs.h"

#define nelem(x)    (sizeof(x) / sizeof((x)[0]))
typedef struct Super Super;
typedef struct File File;

#define NBLOCK 1024 // Block Nums of this disk.
uint32_t nbitblock; // BitMap Block Nums of this disk.
uint32_t nextbno;   // Next Block Available: disk[nextbno] (next alloc).

struct Super super; // Super Block (disk[1]).

enum {
    BLOCK_FREE  = 0,
    BLOCK_BOOT  = 1,
    BLOCK_BMAP  = 2,
    BLOCK_SUPER = 3,
    BLOCK_DATA  = 4,
    BLOCK_FILE  = 5,
    BLOCK_INDEX = 6,
};

struct Block {
    uint8_t data[BY2BLK];
    uint32_t type;
} disk[NBLOCK];

//@ reverse: mutually transform between little endian and big endian.
void reverse(uint32_t *p) {
    uint8_t *x = (uint8_t *) p;
    uint32_t y = *(uint32_t *) x;
    x[3] = y & 0xFF;
    x[2] = (y >> 8) & 0xFF;
    x[1] = (y >> 16) & 0xFF;
    x[0] = (y >> 24) & 0xFF;
}

//@ reverse_block: reverse proper filed in a block.
void reverse_block(struct Block *b) {
    int i, j;
    struct Super *s;
    struct File *f, *ff;
    uint32_t *u;

    switch (b->type) {
    case BLOCK_FREE:
    case BLOCK_BOOT:
        break; // do nothing.
    case BLOCK_SUPER:
        s = (struct Super *)b->data;
        reverse(&s->s_magic);
        reverse(&s->s_nblocks);

        ff = &s->s_root;
        reverse(&ff->f_size);
        reverse(&ff->f_type);
        for(i = 0; i < NDIRECT; ++i) {
            reverse(&ff->f_direct[i]);
        }
        reverse(&ff->f_indirect);
        break;
    case BLOCK_FILE:
        f = (struct File *)b->data;
        for(i = 0; i < FILE2BLK; ++i) {
            ff = f + i;
            if(ff->f_name[0] == 0) {
                break;
            }
            else {
                reverse(&ff->f_size);
                reverse(&ff->f_type);
                for(j = 0; j < NDIRECT; ++j) {
                    reverse(&ff->f_direct[j]);
                }
                reverse(&ff->f_indirect);
            }
        }
        break;
    case BLOCK_INDEX:
    case BLOCK_BMAP:
        u = (uint32_t *)b->data;
        for(i = 0; i < BY2BLK/4; ++i) {
            reverse(u+i);
        }
        break;
    }
}

/** MyOverview: 
 *      Initialize the disk structure.
 *      Formatlize them:
 *          Root-Block/ BitMap-Block/ Super-Block.
 */
void init_disk() {
    int i, r, diff;

    /*---=== Part I : Init Root Block ===---*/
    /* Init Block Type */
    disk[0].type = BLOCK_BOOT;


    /*---=== Part II : Init BitMap Block ===---*/

    /* RoundUp NBLOCK's Bit Size, calc blocks num */
    nbitblock = (NBLOCK + BIT2BLK - 1) / BIT2BLK;
    nextbno = 2 + nbitblock;

    /* Init Block Type */
    for(i = 0; i < nbitblock; ++i) {
        disk[2+i].type = BLOCK_BMAP;
    }

    /* Init Block Free Bit */
    for(i = 0; i < nbitblock; ++i) {
        memset(disk[2+i].data, 0xff, BY2BLK);
    }

    /* Init RoundUp bits (Shouldn't be free) */
    if(NBLOCK != nbitblock * BIT2BLK) {
        diff = NBLOCK % BIT2BLK / 8;
        memset(disk[2+(nbitblock-1)].data+diff, 0x00, BY2BLK - diff);
    }

    /*---=== Part III : Init Super Block ===---*/
    disk[1].type = BLOCK_SUPER;
    super.s_magic = FS_MAGIC;
    super.s_nblocks = NBLOCK;
    super.s_root.f_type = FTYPE_DIR;
    strcpy(super.s_root.f_name, "/");
}

/** MyOverview:
 *      Alloc a block from 'disk' array,
 *      and set its block-type.
 * 
 *  Return:
 *      The index of the block alloced in 'disk' array.
 */
int next_block(int type) {
    disk[nextbno].type = type;
    return nextbno++;
}

/** MyOverView:
 *      Update all alloced block (< nextbno)
 *      to used status.
 *  
 *  Note:
 *      '2 + i/BIT2BLK'    : index of corresponding BitMap-Block.
 *      '(i%BIT2BLK) / 32' : offset (Word) in BitMap-Block.
 */ 
void flush_bitmap() {
    int i;
    for(i = 0; i < nextbno; ++i) {
        ((uint32_t *)disk[2+i/BIT2BLK].data)[(i%BIT2BLK)/32] &= ~(1<<(i%32));
    }
}

/** MyOverView:
 *      Dump Data(C Runtime/Static Data) to file,
 *      (Create Disk Image File).
 *  
 *  Note:
 *      First Copy Super Block Data to 'disk' array.
 *      Then Dump all 'disk[].data' to file.
 */
void finish_fs(char *name) {
    int fd, i, k, n, r;
    uint32_t *p;

    // Prepare super block.
    memcpy(disk[1].data, &super, sizeof(super));

    // Dump data in `disk` to target image file.
    fd = open(name, O_RDWR|O_CREAT, 0666);
    for(i = 0; i < 1024; ++i) {
        reverse_block(disk+i);
        write(fd, disk[i].data, BY2BLK);
    }

    // Finish.
    close(fd);
}

/** MyOverView:
 *      Save bno as f's data pointer.
 *      (f->son[index] = bno)
 * 
 *  Note:
 *      f       : Father File Block.
 *      index   : bno's index in Father Block.
 *      bno     : son block's index in 'disk' array.
 */
void save_block_link(struct File *f, int index, int bno)
{
    /* Our Indirect Pointer List Only Take One Block,
     * And We Ignore Indirect Block's front pointers,
     * So Our File Can Contains Max 'NINDIRECT' pointers.
     */
    assert(index < NINDIRECT);

    if(index < NDIRECT) {
        f->f_direct[index] = bno;
    }
    else {
        if(f->f_indirect == 0) {
            // create new indirect block.
            f->f_indirect = next_block(BLOCK_INDEX);
        }
        ((uint32_t *)(disk[f->f_indirect].data))[index] = bno;
    }
}

/** MyOverView:
 *      Alloc a new FILE block, put it under 'dirf->son[index]',
 *      then increase 'dirf->f_size' by BY2BLK (Dir now owns a new block).
 * 
 *  Return:
 *      Alloced FILE block's index.
 */
int make_link_block(struct File *dirf, int index) {
    int bno = next_block(BLOCK_FILE);
    save_block_link(dirf, index, bno);
    dirf->f_size += BY2BLK;
    return bno;
}

//@ Overview:
//      Create new block pointer for a file under specified directory.
//      Notice that when we delete a file, we do not re-arrenge all
//      other file pointers, so we should be careful of existing empty
//      file pointers.
//
// Post-Condition:
//      We ASSUME that this function will never fail.
//
// Return:
//      Return a unused struct File pointer
// Hint:
//      use make_link_block function

/** MyOverView:
 *      Get A Unused FILE CONTROL struct under 'dirf'.
 *  
 *  Note:
 *      Exercise 5.4
 */
struct File *create_file(struct File *dirf) {
    struct File *dirblk;
    int i, bno, j;
    int maxIndex = dirf->f_size / BY2BLK;

    /* Check if there's space left in alloced space */
    for (i = 0; i < maxIndex; i++) {
        /*--- Get Dir Block ---*/
        if (i < NDIRECT) {
            bno = dirf->f_direct[i];
        } else {
            bno = ((int*)(disk[dirf->f_indirect].data))[i];
        }
        dirblk = (struct File*)(disk[bno].data);

        /*--- Find An Empty Place ---*/
        for (j = 0; j < FILE2BLK; j++) {
            if (dirblk[j].f_name[0] == '\0') {
                /* File Name is NULL, EMPTY */
                return &dirblk[j];
            }
        }
    }

    /* No Space Left, We Need Alloc New Space */
    bno = make_link_block(dirf, maxIndex);

    /* Return the first empty block */
    return (struct File*)(&disk[bno].data);
}

//@ Write file to disk under specified dir.
void write_file(struct File *dirf, const char *path) {
    int iblk = 0, r = 0, n = sizeof(disk[0].data);
    uint8_t buffer[BY2BLK + 1], *dist;
    struct File *target = create_file(dirf);

    /* in case `create_file` is't filled */
    if (target == NULL) return;

    int fd = open(path, O_RDONLY);
 
    // Get file name with no path prefix.
    const char *fname = strrchr(path, '/');
    if(fname)
        fname++;
    else
        fname = path;
    strcpy(target->f_name, fname);
 
    target->f_size = lseek(fd, 0, SEEK_END);
    target->f_type = FTYPE_REG;
 
    // Start reading file.
    lseek(fd, 0, SEEK_SET);
    while((r = read(fd, disk[nextbno].data, n)) > 0) {
        save_block_link(target, iblk++, next_block(BLOCK_DATA));
    }
    close(fd); // Close file descriptor.
}

//@ Overview:
//      Write directory to disk under specified dir.
//      Notice that we may use standard library functions to operate on
//      directory to get file infomation.
//
// Post-Condition:
//      We ASSUME that this funcion will never fail
void write_directory(struct File *dirf, char *name) {
    int iblk = 0, r = 0;
    uint8_t buffer[BY2BLK + 1], *dist;
    struct File *target = create_file(dirf);

    /* in case `create_file` is't filled */
    if (target == NULL) return;

 
    // Get file name with no path prefix.
    const char *fname = strrchr(name, '/');
    if(fname)
        fname++;
    else
        fname = name;
    strcpy(target->f_name, fname);
 
    target->f_size = 0;
    target->f_type = FTYPE_DIR;
}

/** MyOverView:
 *      Enter Point Of Our ImageDumper.
 * 
 *  Usage:
 *      Img-Path    Files...
 *      Img-Path -r Dirs...
 */
int main(int argc, char **argv) {
    int i;

    init_disk();

    if(argc < 3 || (strcmp(argv[2], "-r") == 0 && argc != 4)) {
        fprintf(stderr, "\
Usage: fsformat gxemul/fs.img files...\n\
       fsformat gxemul/fs.img -r DIR\n");
        exit(0);
    }

    if(strcmp(argv[2], "-r") == 0) {
        for (i = 3; i < argc; ++i) {
            write_directory(&super.s_root, argv[i]);
        }
    }
    else {
        write_directory(&super.s_root, "bin");
        for(i = 2; i < argc; ++i) {
            write_file(&super.s_root, argv[i]);
        }
    }

    flush_bitmap();
    finish_fs(argv[1]);

    return 0;
}
