/**
* Autores:
*   Brandon Ledezma Fernández
*   Walter Morales Vásquez
* Módulo encargado de manejar funciones relevantes al sistema de archivos.
*/

#include <stdlib.h>
#include "my_super.h"

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
uint32_t jenkins_one_at_a_time_hash(char *key, size_t len) {

    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i) {/**Se realizan iteraciones con el fin de generar el valor hash*/
        hash += key[i];
        hash += (hash << 10); /**se realiza un corrimiento de bits*/
        hash ^= (hash >> 6); /**y una comparación AND por caracter en la clave*/
    }
    /**Se realizan operaciones de corrimiento y comparación finales*/
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return abs(hash); /**Se aplica un absoluto para obtener solo popsitivos*/
}

/**
 * block_cipher - Se encarga de cifrar los datos que pertenecen a un bloque,
 * obtiene un puntero a la información que sera cifrada y la clave con que
 * se cifrara dichos datos a los cuales les realizara una operación AND a cada byte.
 *
 * @param data - puntero a la información que se cifra.
 * @param key - clave utilizada para cifrar los datos
 */
void block_cipher(void **data, uint32_t key) {

    char *data_to_cipher = *data;

    for(int i=0; i< MY_BLOCK_SIZE; ++i) { /**Itera todos el bloque de datos*/
        data_to_cipher[i] = data_to_cipher[i] ^ key; /**raliza la operacion AND*/
    }
}

/**
 * block_cipher - Se encarga de decifrar los datos que pertenecen a un bloque,
 * obtiene un puntero a la información que sera decifrada y la clave con que
 * se cifro anteriormente dichos datos a los cuales les realizara una operación
 * AND a cada byte para obtener el valor original.
 *
 * @param data - puntero a la información que se cifra.
 * @param key - clave utilizada para cifrar los datos
 */
void block_decipher(void **data, uint32_t key) {

    char *data_to_decipher = *data;

    for(int i=0; i< MY_BLOCK_SIZE; ++i) { /**Itera todos el bloque de datos*/
        data_to_decipher[i] = data_to_decipher[i] ^ key; /**raliza la operacion AND para obtener el estado original*/
    }
}