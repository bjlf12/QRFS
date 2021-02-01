/*
 *
 */

#include <string.h>
#include <math.h>
#include <errno.h>

#include "fsck.qrfs.h"
#include "my_storage.h"


int mkfs_file_size;

int blocks_consistency_check(int **blocks_in_use, my_inode *inode, int is_dir) {

    int *temp = *blocks_in_use;
    my_dirent *temp_dirent;
    int index;
    for(index=0; index<NUM_DIRECT_ENT; ++index) {

        if(!inode->direct[index]) {
            break; // Creo que puede haber un 0 de por medio
        }
        if(is_dir) {
            temp_dirent = read_data(inode->direct[index]);
            if(temp_dirent == NULL) {
                perror("Error al leer datos con la función read_data.");
                return  -EIO;

            }
            blocks_consistency_check_aux(&temp, temp_dirent); //FREE
            free(temp_dirent); //TODO le puse esto
        }
        temp[inode->direct[index]] += 1;
    }

    if(index == NUM_DIRECT_ENT) { // TODO fijarse si inicia en 0 la ostia

        uint32_t *indir1 = read_data(inode->indir_1);
        if(indir1 == NULL) {
            perror("Error al leer datos con la función read_data.");
            return -EXIT_FAILURE;
        }

        int index2;
        for(index2=0; index2< PTRS_PER_BLK; ++index2) {

            if(!indir1[index2]) {
                break;
            }
            if(is_dir) {
                temp_dirent = read_data(indir1[index2]);
                if(temp_dirent == NULL) {
                    perror("Error al leer datos con la función read_data.");
                    return -EXIT_FAILURE;
                }
                blocks_consistency_check_aux(&temp, temp_dirent); //FREE
                free(temp_dirent);
            }
            temp[indir1[index2]] += 1;
        }
        free(indir1);

        if(index2 == PTRS_PER_BLK) {

            uint32_t *indir2 = read_data(inode->indir_2);
            if(indir2 == NULL) {
                perror("Error al leer datos con la función read_data.");
                return -EXIT_FAILURE;
            }

            for(int i=0; i< PTRS_PER_BLK; ++i) {

                if(!indir2[i]) {
                    break;
                }
                if(is_dir) {
                    temp_dirent = read_data(indir2[i]);
                    blocks_consistency_check_aux(&temp, temp_dirent); //FREE
                    //free(temp_dirent);
                }
                temp[indir2[i]] += 1;
            }
            free(indir2);
        }
    }
}

int blocks_consistency_check_aux(int **blocks_in_use, my_dirent *dirent) {

    int *temp = *blocks_in_use;
    my_inode *temp_inode;
    for(int i=0; i<DIR_ENTS_PER_BLK; ++i) {

        if(!dirent[i].valid) {
            break;
        }
        temp_inode = get_inode(dirent[i].inode);
        blocks_consistency_check(&temp, temp_inode, dirent[i].isDir);
        free(temp_inode);
    }
}

int inodes_consistency_check(int **inodes_in_use, my_inode *inode, int is_dir) {

    int *temp = *inodes_in_use;
    my_dirent *temp_dirent;
    int index;
    for(index=0; index<NUM_DIRECT_ENT; ++index) {

        if(!inode->direct[index]) {
            break; // Creo que puede haber un 0 de por medio
        }
        temp_dirent = read_data(inode->direct[index]);
        if(temp_dirent == NULL) {
            //TODO hacer
            perror("Error al leer datos con la función read_data.");
            return -EIO;
        }
        inodes_consistency_check_aux(&temp, temp_dirent); //FREE
        free(temp_dirent);

    }

    if(is_dir && index == NUM_DIRECT_ENT) { // TODO fijarse si inicia en 0 la ostia

        uint32_t *indir1 = read_data(inode->indir_1);
        if(indir1 == NULL) {

            perror("Error al leer datos con la función read_data.");
            return -EXIT_FAILURE;
        }

        int index2;
        for(index2=0; index2< PTRS_PER_BLK; ++index2) {

            if(!indir1[index2]) {
                break;
            }
            temp_dirent = read_data(indir1[index2]);
            inodes_consistency_check_aux(&temp, temp_dirent); //FREE
            free(temp_dirent);
        }
        free(indir1);

        if(index2 == PTRS_PER_BLK) {

            uint32_t *indir2 = read_data(inode->indir_2);
            if(indir2 == NULL) {

                perror("Error al leer datos con la función read_data.");
                return -EXIT_FAILURE;
            }

            for(int i=0; i< PTRS_PER_BLK; ++i) {

                if(!indir2[i]) {
                    break;
                }
                temp_dirent = read_data(indir2[i]);
                inodes_consistency_check_aux(&temp, temp_dirent); //FREE
                free(temp_dirent);
            }
            free(indir2);
        }
    }
}

int inodes_consistency_check_aux(int **inodes_in_use, my_dirent *dirent) {

    int *temp = *inodes_in_use;
    my_inode *temp_inode;
    for(int i=0; i<DIR_ENTS_PER_BLK; ++i) {

        if(!dirent[i].valid) {
            break;
        }
        if(dirent[i].isDir) {
            temp_inode = get_inode(dirent[i].inode);
            inodes_consistency_check(&temp, temp_inode, dirent[i].isDir);
            free(temp_inode);
        }
        temp[dirent[i].inode] += 1;
    }
}

int check_file_system(char *user_password) {

    char *data = malloc(mkfs_file_size); // TODO
    memset(data, 0, mkfs_file_size);
    void *ptr = (void *)data;

    for(int i=SUPER_BLOCK_NUM; i<NUMBER_OF_DATABLOCKS; ++i) {

        memcpy(ptr, read_data(i), 1024);
        ptr += 1024;
    }

    uint32_t key = jenkins_one_at_a_time_hash(user_password, strlen(user_password));

    my_super *super = (my_super *)data;

    block_decipher((void **)&super, key);

    if(MY_MAGIC != super->magic) {
        perror("La contraseña para utilizar los datos del sistema de archivos es incorrecta.");
        return -EXIT_FAILURE;
    }

    printf("magic: %d, inode_map_sz: %d, block_map_sz: %d, inode_region_sz: %d, num_blocks: %d, inode: %d\n",
           super->magic, super->inode_map_sz, super->block_map_sz, super->inode_region_sz, super->num_blocks, super->root_inode);

    void *ptr2 = (void *)data + 1024;

    fd_set *inode_b = ptr2;
    block_decipher((void **)&inode_b, key);

    printf("Is set inode (0): %d\n", FD_ISSET(0, inode_b));
    printf("Is set inode (1): %d\n", FD_ISSET(1, inode_b));
    printf("Is set inode (2): %d\n", FD_ISSET(2, inode_b));
    printf("Is set inode (3): %d\n", FD_ISSET(3, inode_b));

    ptr2 += 1024;

    fd_set *block_b = ptr2;
    block_decipher((void **)&block_b, key);

    printf("Is set block 0: %d\n", FD_ISSET(0, block_b));
    printf("Is set block 1: %d\n", FD_ISSET(1, block_b));
    printf("Is set block 2: %d\n", FD_ISSET(2, block_b));
    printf("Is set block 3: %d\n", FD_ISSET(3, block_b));
    printf("Is set block 4: %d\n", FD_ISSET(4, block_b));
    printf("Is set block 5: %d\n", FD_ISSET(5, block_b));
    printf("Is set block 6: %d\n", FD_ISSET(6, block_b));
    printf("Is set block 7: %d\n", FD_ISSET(7, block_b));
    printf("Is set block 8: %d\n", FD_ISSET(8, block_b));
    printf("Is set block 9: %d\n", FD_ISSET(9, block_b));
    printf("Is set block 9: %d\n", FD_ISSET(10, block_b));
    printf("Is set block 9: %d\n", FD_ISSET(11, block_b));
    printf("Is set block 9: %d\n", FD_ISSET(12, block_b));
    printf("Is set block 9: %d\n", FD_ISSET(13, block_b));

    ptr2 += 1024;

    my_inode *inodes = ptr2;

    printf("1: %d\n", inodes[0].uid); //TODO quitar printfs
    printf("2: %d\n", inodes[1].mode);
    printf("3: %d\n", inodes[2].uid);
    printf("4: %d\n", inodes[3].uid);
    printf("5: %d\n", inodes[4].mode);
    printf("6: %d\n", inodes[5].uid);
    printf("7: %d\n", inodes[6].uid);
    printf("8: %d\n", inodes[7].mode);
    printf("9: %d\n", inodes[8].uid);
    printf("10: %d\n", inodes[9].uid);
    printf("11: %d\n", inodes[10].uid);
    printf("12: %d\n", inodes[11].uid);
    printf("13: %d\n", inodes[12].uid);

    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {
        if(FD_ISSET(i, inode_b)) {
            printf("bitmap: %d\n", i);
        }
    }

    ptr2 += 1024*5;

    int *blocks_in_use = calloc(sizeof(int), NUMBER_OF_DATABLOCKS);
    int *free_blocks = calloc(sizeof(int), NUMBER_OF_DATABLOCKS);

    int first_usable_block = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super->inode_map_sz + super->block_map_sz + super->inode_region_sz;

    my_inode *root_inode = get_inode(super->root_inode);
    printf("Block root: %d\n", root_inode->direct[0]);

    if(blocks_consistency_check(&blocks_in_use, root_inode, true) < 0) {
        perror("Error al realizar el chequeo de consistencia de bloques.");
    }

    for(int i=0; i<first_usable_block; ++i) blocks_in_use[i] = 1;

    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {

        if(!FD_ISSET(i, block_b)) {
            free_blocks[i] += 1;
        }
    }

    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {
        if(blocks_in_use[i] > 1 || free_blocks[i] > 1) {
            perror("Se ha encontrado un duplicado de bloques en el sistema de archivos.");
            printf("Bloque encontrado: %d\n", i);
            return -ENOTRECOVERABLE;

        }
        if(blocks_in_use[i] && free_blocks[i]) {
            perror("Se ha encontrado un bloque marcado como libre, siendo parte de un inodo.");
            printf("Bloque encontrado: %d\n", i);
            return -ENOTRECOVERABLE;
        }
        if(!blocks_in_use[i] && !free_blocks[i]) {
            perror("Se ha encontrado un bloque marcado como usado, sin ser parte de ningún inodo.");
            printf("Bloque encontrado: %d\n", i);
            return -ENOTRECOVERABLE;

        }
    }
    printf("Los bloques en el sistema de archivos se encuentran en un estado consistente.\n");

    int *inodes_in_use = calloc(sizeof(int), NUMBER_OF_INODES);
    int *free_inodes = calloc(sizeof(int), NUMBER_OF_INODES);

    int first_inodes = super->root_inode+1;

    if(inodes_consistency_check(&inodes_in_use, root_inode, true) <0) {

        perror("Error al realizar el chequeo de consistencia de inodos.");
    }

    for(int i=0; i<first_inodes; ++i) inodes_in_use[i] = 1;

    for(int i=0; i<NUMBER_OF_INODES; ++i) {

        if(!FD_ISSET(i, inode_b)) {
            free_inodes[i] += 1;
        }
    }

    for(int i=0; i<NUMBER_OF_INODES; ++i) {
        if(inodes_in_use[i] > 1 || free_inodes[i] > 1) {
            perror("Se ha encontrado un duplicado de inodos en el sistema de archivos.");
            printf("Inodo encontrado: %d\n", i);
            return -ENOTRECOVERABLE;

        }
        if(inodes_in_use[i] && free_inodes[i]) {
            perror("Se ha encontrado un inodo marcado como libre, siendo un archivo en el sistema.");
            printf("Inodo encontrado: %d\n", i);
            return -ENOTRECOVERABLE;
        }
        if(!inodes_in_use[i] && !free_inodes[i]) {
            perror("Se ha encontrado un inodo marcado como utilizado, sin ser encontrado en ningún directorio.");
            printf("Inodo encontrado: %d\n", i);
            return -ENOTRECOVERABLE;
        }
    }
    printf("El sistema de archivos posee todos sus inodos en un estado consistente.\n");

    printf("Se ha terminado el chequeo de consistencia, el sistema de archivos se encuentra correctamente.\n");

    free(data); //TODO frees

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

    char *mkfs_qrfolder_path = argv[1];
    char *mkfs_password = argv[2];
    mkfs_file_size = NUMBER_OF_DATABLOCKS * MY_BLOCK_SIZE;


    init_storage(mkfs_qrfolder_path, mkfs_password, mkfs_file_size);
    if(check_file_system(mkfs_password) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}