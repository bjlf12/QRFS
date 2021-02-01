/**
 * Encargado de almacenar los datos de los inodos del sistema de archivos y sus funciones.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "my_inode.h"
#include "my_storage.h"

/**
 * cpy_stat - copia el estado de un inodo en una
 *   estructura stat
 *
 * @param inode del que se obtendrán los datos
 * @param st donde se copiarán los datos
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

/**
 * create_inode - crea un nuevo nodo según los
 *   parámetros que recibe
 *
 * @param mode indica los permisos del nuevo archivo
 *   y su tipo
 * @param size tamaño del archivo relacionado con el
 *   inodo
 * @param which_iNode identificador del inodo
 * @param direct_array arreglo de punteros a los bloques
 *   directos
 * @param indir_1 puntero al bloque indirecto 1 del inodo
 * @param indir_2 puntero al bloque indirecto 2 del inodo
 * @return puntero al inodo creado
 */
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

/**
 * create_dirent - crea una entrada de directorio
 *
 * @param valid indica si la entrada es válida
 * @param isDir dice si la nueva entrada apunta a un
 *   directorio o no
 * @param inode_id indica el identificador del inodo
 * @param filename nombre del archivo o directorio
 * @return puntero a la entrada de directorio recién
 *   creada
 */
my_dirent *create_dirent(int valid, int isDir, int inode_id, char *filename) {

    my_dirent *dirent_to_return = (my_dirent *)malloc(sizeof(my_dirent));

    dirent_to_return->valid = valid;
    dirent_to_return->isDir = isDir;
    dirent_to_return->inode = inode_id;
    strncpy(dirent_to_return->filename, filename, MY_FILENAME_SIZE);

    return dirent_to_return;
};

/**
 * set_attributes_and_update - recibe una entrada de
 *   directorio, actualiza sus valores y lo reescribe
 *   en disco
 *
 * Errors:
 *   -EIO si ocurre un error al escribir en disco
 *
 * @param de puntero a la entrada de directorio a actualizar
 * @param name nombre del archivo o directorio que apuntará
 * @param mode permisos y tipo del archivo o directorio
 * @param isDir indica si el nuevo inodo es directorio o no
 * @return 0 si termina con éxito, valor de error en otro
 *   caso
 */
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

    if(update_inode(freei, inode) < 0) {
        return EIO;
    }

    free(inode);
    return EXIT_SUCCESS;
}