/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de realizar el chequeo de consistencia
 */

#include <string.h>
#include <math.h>
#include <errno.h>

#include "fsck.qrfs.h"
#include "my_storage.h"

/**
 * blocks_consistency_check - encargado de comenzar el chequeo de consistencia
 *   para los bloques en el sistema de archivos
 *
 * Rellena un arreglo indicando si encuentra a los bloques en las posiciones del
 *   arreglo como utilizados
 *
 * Errors:
 *   -EIO error al leer de disco
 *
 * @param blocks_in_use arreglo en el que la cada posición indica cúantas veces
 *   se encuentra utilizado un bloque
 * @param inode inodo por el que se recorrerá el sistema de archivos
 * @param is_dir bandera que indica si el inode se trata o no de un directorio
 * @return 0 en caso de que termine con éxito, un código de error en otro caso
 */
int blocks_consistency_check(int **blocks_in_use, my_inode *inode, int is_dir) {

    int *temp = *blocks_in_use;
    my_dirent *temp_dirent;
    int index;
    for(index=0; index<NUM_DIRECT_ENT; ++index) {

        if(!inode->direct[index]) {
            break;
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
            return -EIO;
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
                    return -EIO;
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
                return -EIO;
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
    return EXIT_SUCCESS;
}

/**
 * blocks_consistency_check_aux - auxiliar que ayuda a recorrer
 *   el sistema de archivos
 *
 * @param blocks_in_use arreglo en el que la cada posición indica cúantas veces
 *   se encuentra utilizado un bloque
 * @param dirent entrada de directorio que será recorrida en busqueda de inodos
 * @return 0 en caso de que termine correctamente, 0 en otro caso
 */
int blocks_consistency_check_aux(int **blocks_in_use, my_dirent *dirent) {

    int *temp = *blocks_in_use;
    my_inode *temp_inode;
    for(int i=0; i<DIR_ENTS_PER_BLK; ++i) {

        if(!dirent[i].valid) {
            break;
        }
        temp_inode = get_inode(dirent[i].inode);
        if(blocks_consistency_check(&temp, temp_inode, dirent[i].isDir) < 0) {
            return EXIT_FAILURE;
        }
        free(temp_inode);
    }
    return EXIT_SUCCESS;
}

/**
 * blocks_consistency_check - encargado de comenzar el chequeo de consistencia
 *   para los inodos en el sistema de archivos
 *
 * Rellena un arreglo indicando si encuentra un inodo, esto dependiendo del
 *   número indice del inodo
 *
 * Errors:
 *   -EIO error al leer de disco
 *
 * @param inodes_in_use arreglo en el que la cada posición indica cúantas veces
 *   se encuentra un inodo
 * @param inode inodo por el que se recorrerá el sistema de archivos
 * @param is_dir bandera que indica si el inode se trata o no de un directorio
 * @return 0 en caso de que termine con éxito, un código de error en otro caso
 */
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
            return -EIO;
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
                return -EIO;
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

/**
 * inodes_consistency_check_aux - auxiliar que ayuda a recorrer
 *   el sistema de archivos en busqueda de inodos
 *
 * @param inodes_in_use arreglo en el que la cada posición indica cúantas veces
 *   se encuentra un inodo
 * @param dirent entrada de directorio que será recorrida en busqueda de inodos
 * @return 0 en caso de que termine correctamente, 1 en otro caso
 */
int inodes_consistency_check_aux(int **inodes_in_use, my_dirent *dirent) {

    int *temp = *inodes_in_use;
    my_inode *temp_inode;
    for(int i=0; i<DIR_ENTS_PER_BLK; ++i) {

        if(!dirent[i].valid) {
            break;
        }
        if(dirent[i].isDir) {
            temp_inode = get_inode(dirent[i].inode);
            if(inodes_consistency_check(&temp, temp_inode, dirent[i].isDir)<0) {
                return EXIT_FAILURE;
            }
            free(temp_inode);
        }
        temp[dirent[i].inode] += 1;
    }
    return EXIT_SUCCESS;
}

/**
 * check_file_system - realiza un chequeo completo al sistema de
 *   archivos
 *
 * Errors:
 *   -ENOTRECOVERABLE un estado no recuperable
 *
 * @param user_password contraseña con la que se decifrarán los
 *   datos para leer la información de organización del sistema de
 *   archivos
 * @return 0 en caso de éxito, 1 en otro caso
 */
int check_file_system(char *user_password) {

    char *data = malloc(mkfs_file_size);
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

    //printf("magic: %d, inode_map_sz: %d, block_map_sz: %d, inode_region_sz: %d, num_blocks: %d, inode: %d\n",
    //       super->magic, super->inode_map_sz, super->block_map_sz, super->inode_region_sz, super->num_blocks, super->root_inode);

    void *ptr2 = (void *)data + 1024;

    fd_set *inode_b = ptr2;
    block_decipher((void **)&inode_b, key);

    ptr2 += 1024;

    fd_set *block_b = ptr2;
    block_decipher((void **)&block_b, key);

    ptr2 += 1024;

    my_inode *inodes = ptr2;

    int *blocks_in_use = calloc(sizeof(int), NUMBER_OF_DATABLOCKS);
    int *free_blocks = calloc(sizeof(int), NUMBER_OF_DATABLOCKS);

    int first_usable_block = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super->inode_map_sz + super->block_map_sz + super->inode_region_sz;

    my_inode *root_inode = get_inode(super->root_inode);

    if(blocks_consistency_check(&blocks_in_use, root_inode, true) < 0) {
        perror("Error al realizar el chequeo de consistencia de bloques.");
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
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

    free(data);

    return EXIT_SUCCESS;
}

/**
 * usage - muestra la información sobre el uso del programa
 */
void usage() {
    printf("Uso: ./fsck.qrfs directorio_qr/ constraseña\n");
}

/**
 * main - función principal del programa
 *
 * Errors:
 *   -EINVAL argumentos inválidos
 *
 * @param argc contador de argumentos
 * @param argv arreglo de argumentos
 * @return 0 en caso de éxito, 1 en otro
 *   caso
 */
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