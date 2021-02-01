/**
 * Encargado de almacenar los datos de los inodos del sistema de archivos y sus funciones.
 */

#ifndef QRFS_MY_INODE_H
#define QRFS_MY_INODE_H

#include <inttypes.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "my_super.h"

// Número de entradas directas para un inodo
#define NUM_DIRECT_ENT 8
// Máximo tamaño para el nombre de los archivos del sistema
#define MY_FILENAME_SIZE 28

/**
 * Esta estructura es utilizada para almacenar información relevante de los distintos ficheros o archivos
 * presentes en el sistema de archivos.
 */
typedef struct my_inode {

    uint16_t uid;				// Identificador del usuario del dueño del archivo
    uint16_t gid;				// Identificador del grupo del dueño del archivo
    uint32_t mode;				// Permisos e indica si se trata de un archivo o directorio
    uint32_t ctime;				// Tiempo de creación
    uint32_t mtime;				// Momento de última modificación
    int32_t size;				// Tamaño en bytes
    uint32_t direct[NUM_DIRECT_ENT];	// Número de punteros de bloques directos
    uint32_t indir_1;			// Puntero al bloque indirecto 1, este bloque actua como un arreglo grande
                                // de bloques directos
    uint32_t indir_2;			// Puntero al bloque indirecto 2, este bloque actua como un arreglo de puntero
                                // a bloques que actuan como el indir_1
    uint32_t pad;               // 4 bytes
}my_inode;						// Total de 64 bytes

/**
 * Estructura para almacenar información relevante sobre los elementos que se encuentran en un directorio
 * en el sistema de archivos.
 */
typedef struct my_dirent {
    uint32_t valid;			// Bandera que indica si la entrada es válido o no (por defecto en 0)
    uint32_t isDir;			// Bandera que indica si una entrada es un directorio o no
    uint32_t inode;		    // Índice como inodo
    char filename[MY_FILENAME_SIZE];  // Nombre del archivo (tomar en cuenta el valor nulo)
}my_dirent;					// Total 32 bytes

/**
 *  Constantes para los bloques
 *   DIRENTS_PER_BLK   Número de entradas de directorio máximas por bloque
 *   INODES_PER_BLOCK  Máximo número de inodos en un bloque
 *   PTRS_PER_BLOCK    Número de punteros en un bloque
 */
enum {
    DIR_ENTS_PER_BLK = MY_BLOCK_SIZE / sizeof(struct my_dirent),
    INODES_PER_BLK = MY_BLOCK_SIZE / sizeof(struct my_inode),
    PTRS_PER_BLK = MY_BLOCK_SIZE / sizeof(uint32_t)
};

/**
 * cpy_stat - copia el estado de un inodo en una
 *   estructura stat
 *
 * @param inode del que se obtendrán los datos
 * @param st donde se copiarán los datos
 */
void cpy_stat(my_inode *inode, struct stat *sb);

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
my_inode *create_inode(int mode, int size, int which_iNode, int direct_array[NUM_DIRECT_ENT], int indir_1, int indir_2);

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
my_dirent *create_dirent(int valid, int isDir, int inode_id, char *filename);

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
int set_attributes_and_update(my_dirent *de, char *name, mode_t mode, bool isDir);

#endif //QRFS_MY_INODE_H
