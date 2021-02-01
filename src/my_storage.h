/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de las funciones de leer de disco y de escribir de disco.
 */

#ifndef QRFS_MY_STORAGE_H
#define QRFS_MY_STORAGE_H

#include <qrencode.h>
#include <gd.h>

static char *qrfolder_path;
static char *password;
static int file_size;

/**
 * init_storage - inicializa los valores del módulo storage
 *
 * @param new_qrfolder_path el directorio donde se encuentran
 *   las imágenes con los códigos QR
 * @param new_password contraseña brindada por el usuario para
 *   decodificar la información del superbloque y de los bitmaps
 * @param new_file_size tamaño de los binarios del sistema de
 *   archivos en total.
 */
void init_storage(char *new_qrfolder_path, char *new_password, int new_file_size);

/**
 * write_total_data - escribe la información del
 *   sistema de archivos completa en imágenes con
 *   códigos QR
 *
 * Errors:
 *  -ENOMEM no hay suficiente memoria para realizar
 *    algún malloc
 *  -EIO errores de lectura o escritura en el
 *    disco
 *
 * @param total_file_data información total del
 *   sistema de archivos
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int write_total_data(char *file_data);

/**
 * write_data - escribe un bloque del sistema de
 *   archivos en disco en forma de QR
 *
 * Errors:
 *   -EXIT_FAILURE si la información codificada en
 *     QR resulta ser nula
 *   -EIO errores de lectura o escritura en disco
 *
 * @param block_data información del bloque que
 *   será almacenada
 * @param position número de bloque en el sistema
 *   de archivos total
 * @return 0 si se completa satisfactoriamente, o
 *   el valor del error
 */
int write_data(void *block_data, int position);

/**
 * read_data - lee un bloque completo de disco
 *   y decodifica su información
 *
 * @param block_num número de bloque a ser
 *   leído
 * @return 0 si se completa satisfactoriamente, o
 *   el valor del error
 */
void *read_data(int num_block);

/**
 * read_file_data - lee información de un
 *   bloque dadas ciertas especificaciones
 *   como la posición y el largo a ser
 *   leído
 *
 * @param block_num número de bloque a ser
 *   leído
 * @param buf buffer donde se almacenará
 *   la información a ser recuperada
 * @param len largo de la información que
 *   se leerá
 * @param offset posición de inicio desde
 *   donde se leerá la información
 */
void read_file_data(int block_num, char *buf, size_t len, size_t offset);

/**
 * write_file_data - se escribe en un bloque
 *   siguiendo ciertas especificaciones como
 *   la cantidad de información a ser escrita
 *   y desde dónde se comenzará a escribir.
 *
 * @param block_num número de bloque en el que
 *   se escribirá la información
 * @param buf buffer desde el que se obtendrá
 *   la información que será escrita
 * @param len largo total de la información a
 *   ser escrita
 * @param offset posición de inicio desde donde
 *   se comenzará a escribir
 */
void write_file_data(int block_num, const char *buf, size_t len, size_t offset);

/**
 * set_inode_bitmap - marca como utilizado un
 *   inode del bitmap encargado de llevar el
 *   registro de estos
 *
 * Errors:
 *   -EIO error al leer o escribir la información
 *     del disco
 *   -ENOMEM no existe suficiente memoria para
 *     realizar un malloc.
 *
 * @param inode_num número de inodo a ser marcado
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int set_inode_bitmap(int inode_num);

/**
 * set_block_bitmap - marca como utilizado un
 *   bloque del sistema de archivos en el
 *   bitmap encargado de llevar el registro
 *
 * Errors:
 *   -EXIT_FAILURE el número de bloque sobrepasa
 *     el límite actual del sistema de archivos
 *   -EIO error al escribir o leer información
 *     del disco
 *   -ENOMEM no hay suficiente memoria para un
 *     malloc
 *
 * @param block_num número de bloque que será
 *   marcado como utilizado
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int set_block_bitmap(int block_num);

/**
 * is_set_inode_bitmap - pregunta si cierto
 *   inodo se encuentra marcado como utilizado
 *   en el registro de estos
 *
 * Errors:
 *   -EIO error al leer o escribir información
 *     del disco
 *   -ENOMEM error al solicitar memoria en un
 *     malloc
 *
 * @param inode_num número de inodo del que se
 *   desea saber si está siendo utilizado
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int is_set_inode_bitmap(int inode_num);

/**
 * is_set_block_bitmap - pregunta si cierto
 *   bloque se encuentra ocupado en el registro
 *   que lleva estos datos
 *
 * Errors:
 *   -EIO error al hacer una lectura o escritura
 *     de disco
 *   -ENOMEM error al solicitar memoria para
 *     realizar un malloc
 *
 * @param block_num número de bloque del que se
 *   desea saber si está siendo ocupado
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int is_set_block_bitmap(int block_num);

/**
 * clear_inode_bitmap - marca como liberado un
 *   inodo en el registro de inodos
 *
 * Errors:
 *   -EIO error al intentar escribir o leer del
 *     disco
 *   -ENOMEM error al solicitar memoria para un
 *     malloc
 *
 * @param inode_num número de inodo que será
 *   marcado como libre
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int clear_inode_bitmap(int inode_num);

/**
 * clear_block_bitmap - marca como liberado un
 *   bloque en el bitmap encargado de llevar el
 *   registro de estos
 *
 * Errors:
 *   -EXIT_FAILURE el número de bloque sobrepasa
 *     el límite actual del sistema de archivos
 *   -EIO error al leer o escribir información
 *     del disco
 *   -ENOMEM no existe suficiente memoria para
 *     realizar un malloc.
 *
 * @param block_num número de bloque que será
 *   marcado como libre
 * @return 0 si se completa satisfactoriamente,
 *   o el valor del error
 */
int clear_block_bitmap(int block_num);

/**
 * get_free_block - obtiene el índice de el
 *   primer bloque que se encuentra libre
 *   según el bitmap encargado de llevar el
 *   registro de estos
 *
 * Errors:
 *   -EIO error al leer o escribir información
 *     del disco
 *   -ENOMEM no existe suficiente memoria para
 *     realizar un malloc.
 *   -ENOSPC
 *
 * @return el indice del bloque libre, o el
 *   valor del error
 */
int get_free_block();

/**
 * get_num_free_block - itera sobre el
 *   bitmap que lleva el registro de
 *   bloques para contar cúantos se
 *   encuentran libres y devolver este
 *   número
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *   -ENOMEM no existe suficiente memoria para
 *     realizar un malloc.
 *
 * @return el número de bloques libres, o
 *   el valor de un error
 */
int get_num_free_block();

/**
 * get_free_inode - retorna el número de inodos
 *   libres en el sistema de archivos
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *   -ENOMEM no existe suficiente memoria para
 *     realizar un malloc.
 *   -ENOSPC si no se encuentra ningún inodo libre
 *
 * @return el número de inodos libres, o un
 *   código de error
 */
int get_free_inode();

/**
 * read_indir1 - se lee la información de el
 *   bloque indirecto 1 de un inodo, tomando
 *   en cuenta ciertas especificaciones
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *
 * @param block_num número de bloque del que se
 *   obtendrá la información
 * @param buf buffer de donde se almacenará la
 *   información leída
 * @param length largo de la información a leer
 * @param offset posición desde donde se comenzará
 *   a leer información
 * @return cantidad de información que quedó como
 *   no leída, o un código de error
 */
size_t read_indir1(int block_num, char *buf, size_t length, size_t offset);

/**
 * read_indir2 - se lee la información de el
 *   bloque indirecto 2 de un inodo, tomando
 *   en cuenta ciertas especificaciones
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *
 * @param block_num número de bloque indir2 del
 *   que se obtendrá la información
 * @param buf buffer de donde se almacenará la
 *   información leída
 * @param length largo de la información a leer
 * @param offset posición desde donde se comenzará
 *   a leer información
 * @param indir1_size número de bloque indirecto 1
 *   del inodo dueño de del indirecto 2 recibido
 * @return cantidad de información que quedó como
 *   no leída, o un código de error
 */
size_t read_indir2(int block_num, char *buf, size_t length, size_t offset, int indir1_size);

/**
 * write_indir1 - escribe información en el
 *   bloque indirecto 1 de un inodo
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *
 * @param blk número de bloque del que se leerá
 * @param buf buffer de donde se obtendrá la
 *   información a leer
 * @param len largo máximo a escribir
 * @param offset posición de donde se comenzará a
 *   escribir
 * @return cantidad de información que quedó como
 *   no escrita, o un código de error
 */
size_t write_indir1(int blk, const char *buf, size_t len, size_t offset);

/**
 * write_indir2 - escribe información en el
 *   bloque indirecto 2 de un inodo
 *
 * Errors:
 *   -EIO error al leer o escribir en disco
 *
 * @param blk número de bloque del que se leerá
 * @param buf buffer de donde se obtendrá la
 *   información a leer
 * @param len largo máximo a escribir
 * @param offset posición de donde se comenzará a
 *   escribir
 * @param indir1_size largo del bloque indirecto
 *   1 del inodo dueño del indirecto 2 recibido
 * @return cantidad de información que quedó como
 *   no escrita, o un código de error
 */
size_t write_indir2(size_t blk, const char *buf, size_t len, size_t offset, int indir1_size);

/**
 * get_inode - obtiene el inodo correspondiente
 *   al índice recibido como parámetro
 *
 * @param inode_id número índice de inodo que se
 *   desea obtener
 * @return el inodo buscado, o un valor nulo en
 *   caso de que no se encontrara
 */
my_inode *get_inode(int inode_id);

/**
 * update_inode - actualiza un inodo presente en
 *   memoria principal al disco
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *
 * @param inode_id identificador que sirve como
 *   índice del inodo a actualizar
 * @param to_update_inode puntero al inodo que será
 *   actualizado
 * @return 0 en caso de que resulte éxitoso, o un
 *   código de error en otro caso
 */
int update_inode(int inode_id, my_inode *to_update_inode);

/**
 * find_in_dir - encuentra el índice de un
 *   inodo al buscar entre las entradas de un
 *   directorio
 *
 * @param dir_entry entradas de directorio de
 *   donde se buscará un inodo de un archivo
 * @param filename nombre del archivo por el
 *   se buscará el inodo
 * @return número de inodo en caso de que se
 *   encuentre, o 0 en caso contrario
 */
int find_in_dir(my_dirent *dir_entry, char *filename);

/**
 * is_empty_dir - determina si un directorio se
 *   encuentra vacío
 *
 * @param de puntero a la primera entrada de
 *   directorio
 * @return 1 si se encuentra vacío, 0 si es el
 *   caso contrario
 */
int is_empty_dir(my_dirent *de);

/**
 * find_free_dir - se busca una entrada de directorio
 *   libre
 *
 * Errors:
 *   -ENOSPC no existe espacio para un nuevo inodo
 *
 * @return indice de una entrada de directorio vacía o
 *   un valor de error
 */
int find_free_dir(my_dirent *de);

/**
 * lookup_for_filename - busca una entrada de directorio en
 *   un directorio
 *
 * Errors:
 *   -EIO errores de lectura o escritura en disco
 *   -ENOENT un elemento del path no se encuentra
 *
 * @return el índice de un inodo en caso de encontrar el
 *   buscado, código de error en caso contrario
 */
int lookup_for_filename(int dir_inode_id, char *filename);

/**
 * parse - divide una dirección "path" en tokens hasta el
 * valor nnames, remueve cualquier elemento "." o ".."
 *
 * Si names es NULL, el path no es alterado y se retorna el
 * contador de paths. En otro caso, el path es alterado por
 * strtok() y la función retorna los nombres en el arreglo
 * names, que apunta a los elementos del string del path
 *
 * @param path el path del directorio
 * @param names arreglo a los nombres presentes en el path o
 *   NULL
 * @param nnames el número máximo de nombres, si este número
 *   es 0 el valor es ilimitado
 * @return el número de elementos en el path recibido
 */
int parse(char *path, char *names[], int nnames);

/**
 * free_char_array - libera la memoria solicitada
 *   para un puntero de strings
 *
 * @param arr arreglo al que se le realizará la
 *   función "free"
 * @param len largo de arr
 */
void free_char_array(char *array[], int len);

/**
 * get_inode_id_from_path - retorna el número identificado de un
 *   inodo dado el path del archivo relacionado a este
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *   -ENOENT un elemento del path no se encuentra
 *   -ENOTDIR un elemento intermedio del path no es un directorio
 *
 * @param path la dirección del archivo
 * @return identificador del inodo si es encontrado o un
 *   código de error
 */
int get_inode_id_from_path(char *path);

/**
 *  get_inode_id_and_leaf_from_path - retorna el número de inodo
 *    para un archivo o directorio según un path, además, indica
 *    el nombre de una hoja que puede que aún no exista
 *
 * Errors:
 *   -EIO error al leer o escribir de disco
 *   -ENOENT un elemento del path no se encuentra
 *   -ENOTDIR un elemento intermedio del path no es un directorio
 *
 * @param path el path del archivo o directorio
 * @param leaf puntero al espacio para el nombre de la hoja
 * @return el identificador del inodo buscado o un código de
 *   error
 */
int get_inode_id_and_leaf_from_path(char *path, char *leaf);

/**
 * get_image_data - se obtiene la información de una imagen
 *
 * @param name nombre del archivo del que se obtendrá la información
 * @param width ancho del archivo
 * @param height altura del archivo
 * @param raw puntero a datos vacíos que terminará conteniendo la
 *   información binaria del archivo buscado
 */
void get_image_data(const char *name, int *width, int *height, void **raw);

/**
 * qrcode_png - genera una imágen con un código QR a partir
 *   de un elemento QRcode
 *
 * @param code estructura que almacena toda la información para
 *   formar un código QR
 * @param fg_color arreglo que define el color de primer plano
 *   que tendrá la imagen generada
 * @param bg_color arreglo que define el color del fondo de la
 *   imagen que será generada
 * @param size tamaño que tendrá la imagen a generar
 * @param margin margen que tendrá la imagen a generar
 * @return estructura que almacena la información para generar
 *   un archivo png
 */
gdImagePtr qrcode_png(QRcode *code, int fg_color[3], int bg_color[3], int size, int margin);

#endif //QRFS_MY_STORAGE_H