/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de manejar funciones relevantes al sistema de archivos.
 */

#ifndef QRFS_MY_SUPER_H
#define QRFS_MY_SUPER_H



#include <inttypes.h>
#include <stdlib.h>

/**Tamaño de cada bloque dentro del sistema de archivos*/
#define MY_BLOCK_SIZE 1024

/**Número magico utilizado como la firma de los QR,
 * en los cuales tiene como valor QRFS
 * */
#define MY_MAGIC 0x53465251

/**Número de bloque que contiene al superbloque*/
#define SUPER_BLOCK_NUM 0

/**Cantidad de inodos por bloque*/
#define NUMBER_OF_INODES 64

/**Cantidad total de bloques presentes en el sistema*/
#define NUMBER_OF_DATABLOCKS 1024

/**Cantidad de bytes correspondientes al superbloque*/
#define SUPER_SIZE 24

/**
 * Estructura utilizada
 * para almacenar datos pertenecientes al super bloque
 * del sistema de archivos
 */
typedef struct my_super {

    uint32_t magic;  /**Número magico que es utilizado como firma de los archivos de los QR creados.*/
    uint32_t inode_map_sz;       /**Cantidad de bloques que ocupa el mapa de inodos*/
    uint32_t block_map_sz;       /**Cantidad de bloques que ocupa el mapa de bloques*/
    uint32_t inode_region_sz;    /**Cantidad de bloques que ocupa el mapa de bloques*/
    uint32_t num_blocks;         /**Cantidad total de bloques disponile en el sistema de archivos*/
    uint32_t root_inode;        /**Número del bloque que contiene al directorio raíz*/


} my_super; /** Esta estrctura tiene un tamaño de 24 bytes*/

/**
 * jenkins_one_at_a_time_hash - Esta función permite obtener un valor
 * hash de una clave multibyte, en nuestro caso de la contraseña
 * utilizada a la hora de crear el sistema de archivos.
 *
 * Esta función fue diseñada por  Bob Jenkins.
 *
 * Información obtenido de: https://en.wikipedia.org/wiki/Jenkins_hash_function
 *
 * @param key - clave utilizada en el sistema de archivos
 * @param len - largo de la clave
 * @return Un valor entero positivo perteneciente al hash de la clave
 */
uint32_t jenkins_one_at_a_time_hash(char *key, size_t len);

/**
 * block_cipher - Se encarga de cifrar los datos que pertenecen a un bloque,
 * obtiene un puntero a la información que sera cifrada y la clave con que
 * se cifrara dichos datos a los cuales les realizara una operación AND a cada byte.
 *
 * @param data - puntero a la información que se cifra.
 * @param key - clave utilizada para cifrar los datos
 */
void block_cipher(void **data, uint32_t key);

/**
 * block_cipher - Se encarga de decifrar los datos que pertenecen a un bloque,
 * obtiene un puntero a la información que sera decifrada y la clave con que
 * se cifro anteriormente dichos datos a los cuales les realizara una operación
 * AND a cada byte para obtener el valor original.
 *
 * @param data - puntero a la información que se cifra.
 * @param key - clave utilizada para cifrar los datos
 */
void block_decipher(void **data, uint32_t key);

#endif //QRFS_MY_SUPER_H
