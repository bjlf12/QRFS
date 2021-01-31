//
// Created by estudiante on 19/1/21.
//

#ifndef QRFS_MY_STORAGE_H
#define QRFS_MY_STORAGE_H

#include <qrencode.h>
#include <gd.h>

static char *qrfolder_path;
static char *password;
static int file_size;

void init_storage(char *new_qrfolder_path, char *new_password, int new_file_size);

int write_total_data(char *file_data);

void *read_super_data();

int write_data(void *block_data, int position);

void *read_data(int num_block); // Creo que solo podríamos utilizar este

void read_file_data(int block_num, char *buf, size_t len, size_t offset);

void write_file_data(int block_num, const char *buf, size_t len, size_t offset);

int set_inode_bitmap(int inode_num);

int set_block_bitmap(int block_num);

int is_set_inode_bitmap(int inode_num);

int is_set_block_bitmap(int block_num);

int clear_inode_bitmap(int inode_num);

int clear_block_bitmap(int block_num);

int get_free_block();

int get_num_free_block();

int get_free_inode();

size_t read_indir1(int block_num, char *buf, size_t length, size_t offset);

size_t read_indir2(int block_num, char *buf, size_t length, size_t offset, int indir1_size);

size_t write_indir1(int blk, const char *buf, size_t len, size_t offset);

size_t write_indir2(size_t blk, const char *buf, size_t len, size_t offset, int indir1_size);

my_inode *get_inode(int inode_id);

int add_inode(int inode_id, my_inode *to_update_inode);

int find_in_dir(my_dirent *dir_entry, char *filename);

int is_empty_dir(my_dirent *de);

int find_free_dir(my_dirent *de);

//int lookup(int inode_id, char *filename);

int lookup_for_filename(int dir_inode_id, char *filename);

int parse(char *path, char *names[], int nnames);

void free_char_array(char *array[], int len);

int get_inode_id_from_path(char *path);

int get_inode_id_and_leaf_from_path(char *path, char *leaf);

void get_image_data(const char *name, int *width, int *height, void **raw);

gdImagePtr qrcode_png(QRcode *code, int fg_color[3], int bg_color[3], int size, int margin);

#endif //QRFS_MY_STORAGE_H