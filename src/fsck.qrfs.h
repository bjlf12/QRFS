//
// Created by estudiante on 31/1/21.
//

#ifndef QRFS_FSCK_QRFS_H
#define QRFS_FSCK_QRFS_H

#include "my_inode.h"

int blocks_consistency_check_aux(int **blocks_in_use, my_dirent *dirent);// blocks_consistency_check

int blocks_consistency_check(int **blocks_in_use, my_inode *inode, int is_dir);

int inodes_consistency_check_aux(int **inodes_in_use, my_dirent *dirent);

int inodes_consistency_check(int **inodes_in_use, my_inode *inode, int is_dir);

int check_file_system(char *user_password);


#endif //QRFS_FSCK_QRFS_H
