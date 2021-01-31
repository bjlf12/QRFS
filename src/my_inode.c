//
// Created by estudiante on 18/1/21.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "my_inode.h"
#include "my_storage.h"

/**
 * Copy stat from inode to st
 * @param inode inode to be copied from
 * @param st holder to hold copied stat
 * @param inode_idx inode_idx
 */
void cpy_stat(my_inode *inode, struct stat *st) {

    memset(st, 0, sizeof(*st));
    st->st_uid = inode->uid;
    st->st_gid = inode->gid;
    st->st_mode = (mode_t) inode->mode;
    st->st_atime = inode->mtime;
    st->st_ctime = inode->ctime;
    st->st_mtime = inode->mtime;
    st->st_size = inode->size;
    st->st_blksize = MY_BLOCK_SIZE;
    st->st_nlink = 1;
    st->st_blocks = (inode->size + MY_BLOCK_SIZE - 1) / MY_BLOCK_SIZE; //TODO revisar
}

my_inode *create_inode(int mode, int size, int which_iNode, int direct_array[NUM_DIRECT_ENT], int indir_1, int indir_2) {//recibir arreglo de direct

    my_inode *inode_to_return = (my_inode *)malloc(sizeof(my_inode));

    inode_to_return->uid = getuid();
    inode_to_return->gid = getgid();
    inode_to_return->mode = mode;
    inode_to_return->ctime = time(NULL);
    inode_to_return->mtime = time(NULL);
    inode_to_return->size = 0;
    for(int i=0; i<NUM_DIRECT_ENT; ++i) inode_to_return->direct[i] = direct_array[i];
    inode_to_return->indir_1 = indir_1;
    inode_to_return->indir_2 = indir_2;

    return inode_to_return;
}

my_dirent *create_dirent(int valid, int isDir, int inode_id, char *filename) {

    my_dirent *dirent_to_return = (my_dirent *)malloc(sizeof(my_dirent));

    dirent_to_return->valid = valid;
    dirent_to_return->isDir = isDir;
    dirent_to_return->inode = inode_id;
    strncpy(dirent_to_return->filename, filename, MY_FILENAME_SIZE);

    return dirent_to_return;
};

int set_attributes_and_update(my_dirent *de, char *name, mode_t mode, bool isDir) {

    //get free directory and inode
    int freed = find_free_dir(de);
    int freei = get_free_inode();
    int freeb = isDir ? get_free_block() : 0;
    if (freed < 0 || freei < 0 || freeb < 0) return -ENOSPC; // EXIT_FAILURE
    my_dirent *dir = &de[freed];
    my_inode *inode = get_inode(freei);
    strcpy(dir->filename, name);
    dir->inode = freei;
    dir->isDir = isDir; //TODO le cambiamos esto XD antes era true
    dir->valid = true;
    inode->uid = getuid();
    inode->gid = getgid();
    inode->mode = mode;
    inode->ctime = inode->mtime = time(NULL);
    inode->size = 0;
    inode->direct[0] = freeb;
    //update map and inode
    add_inode(freei, inode);

    free(inode);
    //update_blk(); TODO revisar si es necesario
    return EXIT_SUCCESS;
}