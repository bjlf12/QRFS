/*
 *
 */

// Crear el FS a partir de un directorio y una contraseña.

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <unistd.h>
#include <time.h>

#include "my_super.h"
#include "my_inode.h"
#include "my_storage.h"

int mkfs_file_size;
char *mkfs_qrfolder_path;
char *mkfs_password;

int init_file_system() {

    printf("Inicializando el sistema de archivos.\n");

    char *file_data = malloc(mkfs_file_size);
    memset(file_data, 0, mkfs_file_size);

    if(file_data == NULL) {
        perror("Error al solicitar memoria con malloc.");
        return -ENOMEM;
    }

    memset(file_data, 0, mkfs_file_size);
    my_super *my_super = (void*)file_data;

    *my_super = (struct my_super) {.magic = MY_MAGIC,
            .inode_map_sz = (int)ceil((double)NUMBER_OF_INODES / MY_BLOCK_SIZE),
            .block_map_sz = (int)ceil((double)NUMBER_OF_DATABLOCKS / MY_BLOCK_SIZE),
            .inode_region_sz = (int)ceil((double)NUMBER_OF_INODES * sizeof(my_inode) / MY_BLOCK_SIZE),
            .root_inode = 0,
            .num_blocks = NUMBER_OF_DATABLOCKS}; //TODO cambiar todos los number of datablocks por esto??

    printf("inode_map_sz: %d, block_map_sz: %d, inode_region_sz: %d, num_blocks: %d\n",
           my_super->inode_map_sz, my_super->block_map_sz, my_super->inode_region_sz, my_super->num_blocks);

    int num_block = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + my_super->inode_map_sz +
            my_super->block_map_sz + my_super->inode_region_sz;

    int root_direct[NUM_DIRECT_ENT]; //int *root_direct = (int *)malloc(sizeof(int)* NUM_DIRECT_ENT);
    root_direct[0] = num_block;
    for(int i=1; i< NUM_DIRECT_ENT; ++i) root_direct[i] = 0;

    int root_mode = 0040755; // 0040777 // mode_t S_IRWXU | S_IRWXG | S_IRWXO // Creo que para el root es 0040755
    int root_inode_id = 0; // TODO revisar si no hay problema si es 0
    int root_indir_1 = 0;
    int root_indir_2 = 0;

    int actual_block_num = ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    fd_set *inode_bitmap_ptr = (void *)(file_data + (actual_block_num*MY_BLOCK_SIZE));
    FD_SET(root_inode_id, inode_bitmap_ptr);

    actual_block_num += my_super->inode_map_sz;

    fd_set *block_bitmap_ptr = (void *)(file_data + (actual_block_num*MY_BLOCK_SIZE));
    for(int i=0; i< num_block+1; ++i) FD_SET(i, block_bitmap_ptr); // block_num

    actual_block_num += my_super->block_map_sz;

    my_inode *inodes = (void *)(file_data + (actual_block_num*MY_BLOCK_SIZE));
    my_inode *temp = create_inode(root_mode, 1024, root_inode_id, root_direct, root_indir_1, root_indir_2);
    void *inode_ptr = (void *)(inodes);
    inode_ptr += root_inode_id*sizeof(my_inode);
    memcpy(inode_ptr, temp, sizeof(struct my_inode));

    // FILE.A dksmlamds
    /*int f1_inode = 1;
    void *ptr_nu = (void *)(file_data + ((num_block)*MY_BLOCK_SIZE));
    my_dirent *root_de = ptr_nu;
    root_de[0] = (my_dirent){.valid = 1, .isDir = 0,
            .inode = f1_inode, .filename = "file.A"};
    int f1_blk = num_block+1;
    printf("Num f1blk: %d\n", f1_blk);
    void *f1_ptr = ptr_nu + MY_BLOCK_SIZE;

    memset(f1_ptr, 'A', 1000);
    my_inode *temp2 = malloc(sizeof(my_inode));
    *temp2 = (my_inode){.uid = getuid(), .gid = getgid(), .mode = 0100777,
            .ctime = time(NULL), .mtime = time(NULL),
            .size = 1000,
            .direct = {f1_blk, 0, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};
    inode_ptr += sizeof(my_inode);
    memcpy(inode_ptr, temp2, sizeof(my_inode));

    FD_SET(f1_blk, block_bitmap_ptr);
    FD_SET(1, inode_bitmap_ptr);*/
    // FILE.A dksmlamds

    printf("actual_block_num3: %d\n", actual_block_num);

    printf("%d\n", inodes[0].uid);
    printf("%d\n", inodes[1].uid);
    printf("%d\n", inodes[1].mode);

    uint32_t key = jenkins_one_at_a_time_hash(mkfs_password, strlen(mkfs_password));

    block_cipher((void **) &my_super, key);
    block_cipher((void **) &block_bitmap_ptr, key);
    block_cipher((void **) &inode_bitmap_ptr, key);

    printf("cifrado: inode_map_sz: %d, block_map_sz: %d, inode_region_sz: %d, num_blocks: %d\n",
           my_super->inode_map_sz, my_super->block_map_sz, my_super->inode_region_sz, my_super->num_blocks);

    //block_decipher((void **)&my_super, key);

    if(write_total_data(file_data) != 0) {
        perror("Error al escribir los datos del sistema de archivos.");//return
    }

    free(file_data);

    printf("Sistema de archivos inicializado.\n");
    return EXIT_SUCCESS;
}

void usage(char *arg0) {

    printf("Uso: %s directorio_qr/ constraseña\n", arg0);
}

int main(int argc, char* argv[]) {

    if(argc != 3) {
        usage(argv[0]);
        perror("Error en los argumentos.");
        return(-EINVAL);
    }

    mkfs_file_size = NUMBER_OF_DATABLOCKS * MY_BLOCK_SIZE;
    mkfs_qrfolder_path = argv[1];
    mkfs_password = argv[2];
    init_storage(mkfs_qrfolder_path, mkfs_password, mkfs_file_size);

    init_file_system();
    return EXIT_SUCCESS;
}