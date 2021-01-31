//
// Created by estudiante on 18/1/21.
//

#ifndef QRFS_MY_INODE_H
#define QRFS_MY_INODE_H

#include <inttypes.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "my_super.h"

#define NUM_DIRECT_ENT 8
#define MY_FILENAME_SIZE 28

typedef struct my_inode {

    uint16_t uid;				/* user ID of file owner */
    uint16_t gid;				/* group ID of file owner */
    uint32_t mode;				/* permissions | type: file, directory, ... */
    uint32_t ctime;				/* creation time */
    uint32_t mtime;				/* last modification time */
    //time_t timeStamp; //Timestamp
    int32_t size;				/* size in bytes */
    uint32_t direct[NUM_DIRECT_ENT];	/* direct block pointers */
    uint32_t indir_1;			/* single indirect block pointer */
    uint32_t indir_2;			/* double indirect block pointer */
    uint32_t pad;            /* 64 bytes per inode */
}my_inode;								/* total 64 bytes */

typedef struct my_dirent { //Qué eran los valores por defecto? TODO borramos los valores por defecto
    uint32_t valid;			/* entry valid flag */  // Tenía un 1
    uint32_t isDir;			/* entry is directory flag */  //Tenía un 1
    uint32_t inode;		/* entry inode */ //TODO tenía un 30
    char filename[MY_FILENAME_SIZE];/* with trailing NUL */
}my_dirent;								/* total 32 bytes */

/**
 * Constants for blocks
 *   DIRENTS_PER_BLK   - number of directory entries per block
 *   INODES_PER_BLOCK  - number of inodes per block
 *   PTRS_PER_BLOCK    - number of inode pointers per block
 *   BITS_PER_BLOCK    - number of bits per block
 */
enum {
    DIR_ENTS_PER_BLK = MY_BLOCK_SIZE / sizeof(struct my_dirent),
    INODES_PER_BLK = MY_BLOCK_SIZE / sizeof(struct my_inode),
    PTRS_PER_BLK = MY_BLOCK_SIZE / sizeof(uint32_t),
    BITS_PER_BLK = MY_BLOCK_SIZE * 8
};

void cpy_stat(my_inode *inode, struct stat *sb);

my_inode *create_inode(int mode, int size, int which_iNode, int direct_array[NUM_DIRECT_ENT], int indir_1, int indir_2);

my_dirent *create_dirent(int valid, int isDir, int inode_id, char *filename);

int set_attributes_and_update(my_dirent *de, char *name, mode_t mode, bool isDir);

#endif //QRFS_MY_INODE_H
