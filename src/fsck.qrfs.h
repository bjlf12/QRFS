/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de realizar el chequeo de consistencia
 */

#ifndef QRFS_FSCK_QRFS_H
#define QRFS_FSCK_QRFS_H

#include "my_inode.h"

int mkfs_file_size;

/**
 * blocks_consistency_check_aux - auxiliar que ayuda a recorrer
 *   el sistema de archivos
 *
 * @param blocks_in_use arreglo en el que la cada posición indica cúantas veces
 *   se encuentra utilizado un bloque
 * @param dirent entrada de directorio que será recorrida en busqueda de inodos
 * @return 0 en caso de que termine correctamente, 0 en otro caso
 */
int blocks_consistency_check_aux(int **blocks_in_use, my_dirent *dirent);

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
int blocks_consistency_check(int **blocks_in_use, my_inode *inode, int is_dir);

/**
 * inodes_consistency_check_aux - auxiliar que ayuda a recorrer
 *   el sistema de archivos en busqueda de inodos
 *
 * @param inodes_in_use arreglo en el que la cada posición indica cúantas veces
 *   se encuentra un inodo
 * @param dirent entrada de directorio que será recorrida en busqueda de inodos
 * @return 0 en caso de que termine correctamente, 1 en otro caso
 */
int inodes_consistency_check_aux(int **inodes_in_use, my_dirent *dirent);

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
int inodes_consistency_check(int **inodes_in_use, my_inode *inode, int is_dir);

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
int check_file_system(char *user_password);

#endif //QRFS_FSCK_QRFS_H
