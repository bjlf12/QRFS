/*
 *
 */

// TODO Chequeo de consistencia

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "my_inode.h"
#include "my_storage.h"

int blocks_consistency_check_aux(int **blocks_in_use, my_dirent *dirent);// blocks_consistency_check

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
                //TODO hacer
                perror("Error al leer datos con la función read_data.\n");
                return EXIT_FAILURE;
            }
            blocks_consistency_check_aux(&temp, temp_dirent); //FREE
            //free(temp_dirent);
        }
        temp[inode->direct[index]] += 1;
    }

    if(index == NUM_DIRECT_ENT) { // TODO fijarse si inicia en 0 la ostia
        //TODO indir
        uint32_t *indir1 = read_data(inode->indir_1);
        //if(NULL)
        int index2;
        for(index2=0; index2< PTRS_PER_BLK; ++index2) {

            if(!indir1[index2]) {
                break;
            }
            if(is_dir) {
                temp_dirent = read_data(indir1[index2]);
                blocks_consistency_check_aux(&temp, temp_dirent); //FREE
                //free(temp_dirent);
            }
            temp[indir1[index2]] += 1;
        }

        if(index2 == PTRS_PER_BLK) {
            //TODO indir
            uint32_t *indir2 = read_data(inode->indir_2);
            //if(NULL)
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
        //free(temp_inode);
    }
}

int inodes_consistency_check_aux(int **inodes_in_use, my_dirent *dirent);

int inodes_consistency_check(int **inodes_in_use, my_inode *inode, int is_dir) {

    int *temp = *inodes_in_use;
    my_dirent *temp_dirent;
    int index;
    for(index=0; index<NUM_DIRECT_ENT; ++index) {

        if(!inode->direct[index]) {
            break; // Creo que puede haber un 0 de por medio
        }
        //if(is_dir) {
            temp_dirent = read_data(inode->direct[index]);
            if(temp_dirent == NULL) {
                //TODO hacer
                perror("Error al leer datos con la función read_data.\n");
                return EXIT_FAILURE;
            }
            inodes_consistency_check_aux(&temp, temp_dirent); //FREE
            //free(temp_dirent);
        //}
    }
    if(is_dir && index == NUM_DIRECT_ENT) { // TODO fijarse si inicia en 0 la ostia
        //TODO indir
        uint32_t *indir1 = read_data(inode->indir_1);
        //if(NULL)
        int index2;
        for(index2=0; index2< PTRS_PER_BLK; ++index2) {

            if(!indir1[index2]) {
                break;
            }
            temp_dirent = read_data(indir1[index2]);
            inodes_consistency_check_aux(&temp, temp_dirent); //FREE
            //free(temp_dirent); tODO
        }

        if(index2 == PTRS_PER_BLK) {
            //TODO indir
            uint32_t *indir2 = read_data(inode->indir_2);
            //if(NULL)
            for(int i=0; i< PTRS_PER_BLK; ++i) {

                if(!indir2[i]) {
                    break;
                }
                temp_dirent = read_data(indir2[i]);
                inodes_consistency_check_aux(&temp, temp_dirent); //FREE
                // free(temp_dirent)
            }
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
        }
        temp[dirent[i].inode] += 1;
        //free(temp_inode);
    }
}

int check_file_system(char *user_password) {

    char *data = malloc(1024*10);
    memset(data, 0, 1024*10);
    void *ptr = (void *)data;
    for(int i=SUPER_BLOCK_NUM; i<10; ++i) {

        memcpy(ptr, read_data(i), 1024);
        ptr += 1024;
    }

    uint32_t key = jenkins_one_at_a_time_hash(user_password, strlen(user_password));

    my_super *super = (my_super *)data;

    block_decipher((void **)&super, key);

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

    printf("1: %d\n", inodes[0].uid);
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

    for(int i=0; i<10; ++i) {
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

    //my_dirent *dirent_root;

    blocks_consistency_check(&blocks_in_use, root_inode, true);
    for(int i=0; i<first_usable_block; ++i) {
        blocks_in_use[i] = 1;
    }

    //dirent_root = read_data(dirent_inode->direct[0]);

    //blocks_consistency_check_aux(&blocks_in_use, dirent_root);

    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {

        if(!FD_ISSET(i, block_b)) {
            free_blocks[i] += 1;
        }
    }

    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {
        if(blocks_in_use[i] > 1 || free_blocks[i] > 1) {
            perror("Se ha encontrado un duplicado de bloques en el sistema de archivos. ");
            printf("Bloque encontrado: %d\n", i);
            return EXIT_FAILURE;
        }
        if(blocks_in_use[i] && free_blocks[i]) {
            perror("Se ha encontrado un bloque marcado como libre, pero siendo parte de un inodo. ");
            printf("Bloque encontrado: %d\n", i);
            return EXIT_FAILURE;
        }
        if(!blocks_in_use[i] && !free_blocks[i]) {
            perror("Se ha encontrado un bloque marcado como no libre, pero sin ser parte de ningún inodo. ");
            printf("Bloque encontrado: %d\n", i);
            return EXIT_FAILURE;
        }
    }
    printf("Los bloques en el sistema de archivos se encuentran en un estado consistente.\n");

    int *inodes_in_use = calloc(sizeof(int), NUMBER_OF_INODES);
    int *free_inodes = calloc(sizeof(int), NUMBER_OF_INODES);

    int first_inodes = super->root_inode+1;

    inodes_consistency_check(&inodes_in_use, root_inode, true); //Manejar el valor de retorno TODO
    for(int i=0; i<first_inodes; ++i) {
        inodes_in_use[i] = 1;
    }

    for(int i=0; i<NUMBER_OF_INODES; ++i) {

        if(!FD_ISSET(i, inode_b)) {
            free_inodes[i] += 1;
        }
    }

    for(int i=0; i<NUMBER_OF_INODES; ++i) {
        if(inodes_in_use[i] > 1 || free_inodes[i] > 1) {
            perror("Se ha encontrado un duplicado de inodos en el sistema de archivos.");
            printf("Inodo encontrado: %d\n", i);
            return EXIT_FAILURE;
        }
        if(inodes_in_use[i] && free_inodes[i]) {
            perror("Se ha encontrado un inodo marcado como libre, pero siendo parte de un directorio.");
            printf("Inodo encontrado: %d\n", i);
            return EXIT_FAILURE;
        }
        if(!inodes_in_use[i] && !free_inodes[i]) {
            perror("Se ha encontrado un inodo marcado como no libre, pero sin ser encontrado en ningún directorio.");
            printf("Inodo encontrado: %d\n", i);
            return EXIT_FAILURE;
        }
    }
    printf("El sistema de archivos posee todos sus inodos en un estado consistente.\n");


    /*my_dirent *dirent_root = (void *)(data + ((inodes[0].direct[0])*1024));

    printf("%d\n", dirent_root->isDir);

    my_dirent *dirent_dir = (void *)(data + ((inodes[2].direct[0])*1024));

    bloque_prueba *b_prueb = (void *)data;

    ptr2 += 1024;

    char *dataf1 = (void *)(data + ((inodes[1].direct[0])*1024));*/

    printf("Se ha terminado el chequeo de consistencia, el sistema de archivos se encuentra correctamente.\n");

    free(data);
}

void usage() {

}

int main(int argc, char* argv[]) {

    if(argc != 4) {
        usage();
        perror("Error en los argumentos.\n");
        return(-1);
    }
    /*FILE *data = fopen("algonuevo", "w+");
    fprintf(data, "%c", 1);
    //memcpy(data, '\0'+1, 1024);

    fclose(data);*/

    /*int file = open("nombrealgo", O_WRONLY|O_CREAT|O_TRUNC, 0777);
    char *data = malloc(100);
    void *ptr = (void *)data;*/

    /*struct nom *n= ptr;
    n->x = 1;
    n->y = 20;*/

    //memcpy(ptr, 'm', 4);
    //write(file, data, 100);
    //close(file);

    init_storage(argv[1], argv[3], 9821);
    check_file_system(argv[3]);
}