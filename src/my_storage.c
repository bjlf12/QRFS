/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de las funciones de leer de disco y de escribir de disco.
 */

#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <zbar.h>
#include <math.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "my_inode.h"
#include "my_super.h"
#include "my_storage.h"

#define BG_COLOR {255,255,255}
#define FG_COLOR {0,0,0}
#define QR_SIZE 1000
#define QR_MARGIN 5
#define QR_VERSION 23

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
void init_storage(char *new_qrfolder_path, char *new_password, int new_file_size) {

    printf("Iniciando los datos de las funciones de almacenamiento del sistema.\n");
    qrfolder_path = new_qrfolder_path;
    password = new_password;
    file_size = new_file_size;
    printf("Terminando de inicializar los datos del almacenamiento del programa.\n");
}

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
int write_total_data(char *total_file_data) {

    printf("Escribiendo todos los datos del sistema de archivos.\n");
    FILE *file_d;
    QRcode *qrcode;
    int block_counter = SUPER_BLOCK_NUM;

    int int_bg_color[3] = BG_COLOR;
    int int_fg_color[3] = FG_COLOR;

    void *file_ptr = total_file_data;

    int size_remaining = file_size;
    char temp[MY_BLOCK_SIZE];

    char file_name[MY_FILENAME_SIZE];

    // Se escribe información de bloque en bloque
    while(size_remaining) {

        memcpy(temp, file_ptr, MY_BLOCK_SIZE);
        qrcode = QRcode_encodeData(MY_BLOCK_SIZE, temp, QR_VERSION, QR_ECLEVEL_L);
        if (qrcode == NULL) {
            perror("Error al crear el código QR en QRcode_encodeData.");
            return -ENOMEM;
        }

        sprintf(file_name, "%sQRFS-%d", qrfolder_path, block_counter++);
        file_d = fopen(file_name, "w+");
        if (file_d == NULL) {
            perror("No se pudo crear el archivo del QR en fopen.");
            return -EIO;
        }

        gdImagePtr image = qrcode_png(qrcode, int_fg_color, int_bg_color, QR_SIZE, QR_MARGIN) ;
        gdImagePng(image, file_d);
        QRcode_free (qrcode);
        gdImageDestroy(image);
        fclose(file_d);

        file_ptr += MY_BLOCK_SIZE;
        size_remaining -= MY_BLOCK_SIZE;
        if(size_remaining < 0) size_remaining = 0;
    }

    printf("Terminando de escribir los datos del sistema de archivos.\n");
    return EXIT_SUCCESS;
}

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
int write_data(void *block_data, int position) {

    printf("Comenzando a escribir información en el bloque: %d.\n", position);
    FILE *file_d;
    QRcode *qrcode;

    int int_bg_color[3] = BG_COLOR;
    int int_fg_color[3] = FG_COLOR;

    char file_name[MY_FILENAME_SIZE];

    qrcode = QRcode_encodeData(MY_BLOCK_SIZE, block_data, QR_VERSION, QR_ECLEVEL_L);
    if (qrcode == NULL) {
        perror("Error al crear el código QR.");
        return -EXIT_FAILURE;
    }

    sprintf(file_name, "%sQRFS-%d", qrfolder_path, position);
    file_d = fopen(file_name, "w+");
    if (file_d == NULL) {
        perror("No se pudo crear el archivo del QR en fopen.");
        return -EIO;
    }

    gdImagePtr image = qrcode_png(qrcode, int_fg_color, int_bg_color, QR_SIZE, QR_MARGIN) ;
    gdImagePng(image, file_d);
    QRcode_free(qrcode);
    gdImageDestroy(image);
    fclose(file_d);

    printf("Terminando de escribir la información de un bloque.\n");
    return EXIT_SUCCESS;
}

/**
 * read_data - lee un bloque completo de disco
 *   y decodifica su información
 *
 * @param block_num número de bloque a ser
 *   leído
 * @return 0 si se completa satisfactoriamente, o
 *   el valor del error
 */
void *read_data(int block_num) {

    printf("Leyendo la información del bloque: %d.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return NULL;
    }

    // Se crean las estructuras necesarias para obtener información a partir de un QR
    zbar_image_scanner_t *scanner = zbar_image_scanner_create();
    zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_BINARY, 1);

    int width = 0, height = 0;
    void *raw = NULL;

    char abs_file_name[MY_FILENAME_SIZE];
    sprintf(abs_file_name, "%sQRFS-%d", qrfolder_path, block_num);

    get_image_data(abs_file_name, &width, &height, &raw);

    // Se especifica la información de la imagen
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y','8','0','0'));
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

    // Se escanea la imagen y se extraen los resultados
    int n = zbar_scan_image(scanner, image);
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
    const char *data = zbar_symbol_get_data(symbol);

    char *to_return_data = malloc(MY_BLOCK_SIZE);
    if(to_return_data == NULL) {
        perror("Error al asignar memoria en read_data");
        return NULL; 
    }
    memcpy(to_return_data, data, 1024);

    // Se libera la información que ya no se utilizará
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    printf("Finalizando de leer la información de un bloque.\n");
    return (void *)to_return_data;
}

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
void read_file_data(int block_num, char *buf, size_t len, size_t offset) {

    printf("Leyendo la información de un archivo en el bloque: %d.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return;
    }

    my_dirent *entries;
    entries = read_data(block_num);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return;
    }

    memcpy(buf, entries + offset, len);
    free(entries);

    printf("Terminando de leer la información de un archivo.");
}

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
void write_file_data(int block_num, const char *buf, size_t len, size_t offset) {

    printf("Comenzando a escribir la información de un archivo en el bloque: %d.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return;
    }

    char *entries;

    entries = read_data(block_num);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return;
    }

    // Se utiliza memcpy para cumplir con las especificaciones recibidas por parámetro
    memcpy(entries + offset, buf, len);
    if(write_data(entries, block_num) < 0) {
        perror("Error al escribit las entradas de directorio en write_data.");
        return;
    };

    printf("Terminando de escribir la información de un archivo.");
}

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
int set_inode_bitmap(int inode_num) {

    printf("Marcando el bit %d del bitmap de inodos.\n", inode_num);

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al obtener memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;
    void *temp_inode_map;

    // Se lee información según la cantidad de bloques que especifica el superbloque
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        temp_inode_map = read_data(inode_map_position+i);
        memcpy(ptr, temp_inode_map, MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);

        free(temp_inode_map);
        ptr += MY_BLOCK_SIZE;
    }

    FD_SET(inode_num, inode_map);

    block_cipher((void **)&inode_map_data, key);

    // Se escribe información según la información dada por el superbloque
    ptr = (void *)inode_map_data;
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        if(write_data(ptr, inode_map_position+i) < 0) {
            perror("Error al escribir el bitmap de inodos en write_data.");
            return -EIO;
        }

        ptr += MY_BLOCK_SIZE;
    }

    free(super_block);
    free(inode_map_data);

    printf("Terminando de marcar un bit en el bitmap de inodos.");
    return EXIT_SUCCESS;
}

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
int set_block_bitmap(int block_num) {

    printf("Marcando el bit %d del bitmap de bloques.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return -EXIT_FAILURE;
    }

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el super bloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;
    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    // Se lee información hasta cumplir con la cantidad de bloques especificada en el superbloque
    for(int i=0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    FD_SET(block_num, block_map);
    block_cipher((void **)&block_map_data, key);

    // Se escribe información según la cantidad expuesta en el superbloque
    ptr = (void *)block_map_data;
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        if(write_data(ptr, block_map_position+i) < 0) {
            perror("Error al escribir el bitmap de inodos en write_data.");
            return -EIO;
        }

        ptr += MY_BLOCK_SIZE;
    }

    free(super_block);
    free(block_map_data);

    printf("Terminando de marcar un bit del bitmap de bloques.");
    return EXIT_SUCCESS;
}

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
int is_set_inode_bitmap(int inode_num) {

    printf("Comprobando si el bit %d del bitmap de inodos se encuentra activado.\n", inode_num);

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;

    // Se lee información según la cantidad que indica el superbloque que requiere la región bitmap de inodos
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        memcpy(ptr, read_data(inode_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    int result = FD_ISSET(inode_num, inode_map);

    free(super_block);
    free(inode_map_data);

    printf("Terminando de comprobar si un bit del bitmap de inodos se encuentra activado.\n");
    return result;
}

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
int is_set_block_bitmap(int block_num) {

    printf("Comprobando si el bit %d del bitmap de bloques se encuentra activado.\n", block_num);

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;
    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }
    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    // Se lee información hasta cumplir con la cantidad de bloques que indica el superbloque que ocupa el
    // bitmap de bloques
    for(int i=0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&block_map, key);
        ptr += MY_BLOCK_SIZE;
    }

    int result = FD_ISSET(block_num, block_map);

    free(super_block);
    free(block_map_data);

    printf("Terminando de comprobar si un bit del bitmap de bloques se encuentra activado.\n");
    return result;
}

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
int clear_inode_bitmap(int inode_num) {

    printf("Limpiando el bit %d del bitmap de inodos.\n", inode_num);

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;

    // Se lee toda la información que se deba según el superbloque
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        memcpy(ptr, read_data(inode_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    FD_CLR(inode_num, inode_map);

    block_cipher((void **)&inode_map_data, key);

    // Se escribe la información según indica el superbloque que ocupa el bitmap de inodos
    ptr = (void *)inode_map_data;
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        if(write_data(ptr, inode_map_position+i) < 0) {
            perror("Error al escribir el bitmap de inodos en write_data.");
            return -EIO;
        }

        ptr += MY_BLOCK_SIZE;
    }

    free(super_block);
    free(inode_map_data);

    printf("Terminando de limpiar un bit del bitmap de inodos.\n");
    return EXIT_SUCCESS;
}

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
int clear_block_bitmap(int block_num) {

    printf("Limpiando el bit %d del bitmap de bloques.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return -EXIT_FAILURE;
    }

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;
    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if (block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    // Se lee la información que indica el superbloque que se debe leer
    for (int i = 0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position + i), MY_BLOCK_SIZE);
        block_decipher((void **)&block_map, key);
        ptr += MY_BLOCK_SIZE;
    }

    FD_CLR(block_num, block_map);
    block_cipher((void **)&block_map, key);

    // Se escribe la información que indica el superbloque que se debe escribir
    if (write_data(block_map_data, block_map_position) < 0) {
        perror("Error al escribir la información del bitmap de bloques en write_data.");
        return -EIO;
    }

    free(super_block);
    free(block_map_data);

    printf("Terminando de limpiar un bit del bitmap de bloques.\n");
    return EXIT_SUCCESS;
}

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
int get_free_block() {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;
    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }
    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    // Se lee la información según indica el superbloque que se debe leer
    for(int i=0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {
        if(!FD_ISSET(i, block_map)) {

            FD_SET(i, block_map);

            block_cipher((void **)&block_map_data, key);
            write_data(block_map_data, block_map_position);

            free(super_block);
            free(block_map_data);

            return i;
        }
    }

    free(super_block);
    free(block_map_data);

    return -ENOSPC;
}

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
int get_num_free_block() {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;
    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }
    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    // Se lee la información del bitmap de bloques según indica el superbloque que se debe leer
    for(int i=0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    int count=0;
    for(int i=0; i<NUMBER_OF_DATABLOCKS; ++i) {
        if(!FD_ISSET(i, block_map)) {
            ++count;
        }
    }

    free(super_block);
    free(block_map_data);

    return count;
}

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
int get_free_inode() {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición del bloque y se obtiene su información
    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;

    // Se lee la información del bitmap de inodos según indica el superbloque que se debe leer
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        memcpy(ptr, read_data(inode_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    // Se busca cuál es el primer inodo libre
    for(int i=super_block->root_inode+1; i < NUMBER_OF_INODES; ++i) {
        if(!FD_ISSET(i, inode_map)) {
            FD_SET(i, inode_map);

            block_cipher((void **)&inode_map_data, key);
            write_data(inode_map_data, inode_map_position);

            free(super_block);
            free(inode_map_data);

            return i;
        }
    }

    free(super_block);
    free(inode_map_data);

    return -ENOSPC;
}

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
size_t read_indir1(int block_num, char *buf, size_t length, size_t offset) {

    uint32_t *blk_indices  = read_data(block_num);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / MY_BLOCK_SIZE;
    size_t blk_offset = offset % MY_BLOCK_SIZE;
    size_t len_to_read = length;

    // Por cada uno de los punteros del bloque indir1 se lee información hasta que ya no queda
    // información por leer
    while (blk_num < PTRS_PER_BLK && len_to_read > 0) {
        // Se calcula la cantidad de información que se leerá en esta iteración
        size_t cur_len_to_read = len_to_read > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_read;
        size_t temp = blk_offset + cur_len_to_read;

        // Si no hay un puntero en esta posición del bloque se termina la función
        if (!blk_indices[blk_num]) {
            return length - len_to_read;
        }

        read_file_data(blk_indices[blk_num], buf, temp, blk_offset);

        buf += temp;
        len_to_read -= temp;
        blk_num++;
        blk_offset = 0;
    }
    return length - len_to_read;
}

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
size_t read_indir2(int block_num, char *buf, size_t length, size_t offset, int indir1_size) {

    uint32_t *blk_indices = read_data(block_num);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / indir1_size;
    size_t blk_offset = offset % indir1_size;
    size_t len_to_read = length;

    // Por cada uno de los punteros del bloque indir2 se lee información hasta que ya no queda
    // información por leer
    while (blk_num < PTRS_PER_BLK && len_to_read > 0) {
        // Se calcula la cantidad de información que se leerá en esta iteración
        size_t cur_len_to_read = len_to_read > indir1_size ? (size_t) indir1_size - blk_offset : len_to_read;
        size_t temp = blk_offset + cur_len_to_read;

        // Si no hay un puntero en esta posición del bloque se termina la función
        if (!blk_indices[blk_num]) {
            return length - len_to_read;
        }

        // Se realiza un llamado a la función para leer los indirs1
        temp = read_indir1(blk_indices[blk_num], buf, temp, blk_offset);

        buf += temp;
        len_to_read -= temp;
        blk_num++;
        blk_offset = 0;
    }
    return length - len_to_read;
}

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
size_t write_indir1(int blk, const char *buf, size_t len, size_t offset) {

    uint32_t *blk_indices  = read_data(blk);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / MY_BLOCK_SIZE;
    size_t blk_offset = offset % MY_BLOCK_SIZE;
    size_t len_to_write = len;

    // Por cada uno de los punteros del bloque indir1 se lee información hasta que ya no queda
    // información por leer
    while (blk_num < PTRS_PER_BLK && len_to_write > 0) {
        // Se calcula la cantidad de información que se escribirá en esta iteración
        size_t cur_len_to_write = len_to_write > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_write;
        size_t temp = blk_offset + cur_len_to_write;

        // Si no hay un puntero en esta posición del bloque se escribe ahí
        if (!blk_indices[blk_num]) {
            int freeb = get_free_block();
            if (freeb < 0) return len - len_to_write;
            blk_indices[blk_num] = freeb;

            if(write_data(blk_indices, blk) < 0) {
                perror("Error al escribir un bloque de índices en write_data.");
                return -EIO;
            }
        }

        write_file_data(blk_indices[blk_num], buf, temp, blk_offset);

        buf += temp;
        len_to_write -= temp;
        blk_num++;
        blk_offset = 0;
    }
    return len - len_to_write;
}

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
size_t write_indir2(size_t blk, const char *buf, size_t len, size_t offset, int indir1_size) {

    uint32_t *blk_indices = read_data(blk);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / indir1_size;
    size_t blk_offset = offset % indir1_size;
    size_t len_to_write = len;

    // Por cada uno de los punteros del bloque indir2 se lee información hasta que ya no queda
    // información por leer
    while (blk_num < PTRS_PER_BLK && len_to_write > 0) {
        // Se calcula la cantidad de información que se escribirá en esta iteración
        size_t cur_len_to_write = len_to_write > indir1_size ? (size_t) indir1_size - blk_offset : len_to_write;
        size_t temp = blk_offset + cur_len_to_write;
        len_to_write -= temp;

        // Si no hay un puntero en esta posición del bloque se escribe ahí
        if (!blk_indices[blk_num]) {
            int freeb = get_free_block();
            if (freeb < 0) return len - len_to_write;
            blk_indices[blk_num] = freeb;
            //write back

            if(write_data(blk_indices, blk) < 0) {
                perror("Error al escribir un bloque de índices en write_data.");
                return -EIO;
            }
        }

        // Se realiza un llamado a la función para escribir en los indirs1
        temp = write_indir1(blk_indices[blk_num], buf, temp, blk_offset);
        if (temp == 0) return len - len_to_write;
        buf += temp;
        blk_num++;
        blk_offset = 0;
    }
    return len - len_to_write;
}

/**
 * get_inode - obtiene el inodo correspondiente
 *   al índice recibido como parámetro
 *
 * @param inode_id número índice de inodo que se
 *   desea obtener
 * @return el inodo buscado, o un valor nulo en
 *   caso de que no se encontrara
 */
my_inode *get_inode(int inode_id) {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return NULL;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición que corresponde al bloque a leer
    int block = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz +
            super_block->block_map_sz + (inode_id / INODES_PER_BLK);
    my_inode *inode_block = read_data(block);

    my_inode *to_return = malloc(sizeof(struct my_inode));

    void *ptr = inode_block + (inode_id - ((inode_id/INODES_PER_BLK)*INODES_PER_BLK));
    memcpy(to_return, ptr, sizeof(struct my_inode));

    free(super_block);
    free(inode_block);
    return to_return;
}

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
int update_inode(int inode_id, my_inode *to_update_inode) {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    // Se calcula la posición que corresponde al bloque a leer
    int block = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz +
            super_block->block_map_sz + (inode_id / INODES_PER_BLK);
    my_inode *inode_block = read_data(block);

    void *ptr = inode_block + (inode_id - ((inode_id/INODES_PER_BLK)*INODES_PER_BLK));
    memcpy(ptr, to_update_inode, sizeof(struct my_inode));

    if(write_data(inode_block, block) <0) {
        perror("Error al escribir el bloque de inodos en write_data");
        return -EIO;
    }

    free(super_block);
    free(inode_block);

    return EXIT_SUCCESS;
}

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
int find_in_dir(my_dirent *dir_entry, char *filename) {

    // Se recorre toda el directorio en busqueda de una entrada válida con un nombre de archivo que coincida
    for(int i=0; i< DIR_ENTS_PER_BLK; ++i) {
        if(dir_entry[i].valid && strcmp(dir_entry[i].filename, filename) == 0) {
            return dir_entry[i].inode;
        }
    }
    return false;
}

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
int find_free_dir(my_dirent *de) {

    // Se recorre toda el directorio en busqueda de una entrada vacía.
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (!de[i].valid) {
            return i;
        }
    }
    return -ENOSPC;
}

/**
 * is_empty_dir - determina si un directorio se
 *   encuentra vacío
 *
 * @param de puntero a la primera entrada de
 *   directorio
 * @return 1 si se encuentra vacío, 0 si es el
 *   caso contrario
 */
int is_empty_dir(my_dirent *de) {

    // Se recorre toda el directorio y si se encuentra una entrada válida se retorna 0
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (de[i].valid) {
            return false;
        }
    }
    return true;
}

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
int lookup_for_filename(int dir_inode_id, char *filename) {

    // Se obtiene el directorio actual y sus entradas de directorio
    my_inode *cur_dir = get_inode(dir_inode_id);
    my_dirent *entries = read_data(cur_dir->direct[0]);

    if (entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO;
    }
    int inode = find_in_dir(entries, filename);

    free(cur_dir);
    free(entries);

    if(!inode) {
        return -ENOENT;
    }
    return inode;
}

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
int parse(char *path, char *names[], int nnames) {

    char *dup_path = strdup(path);
    int count = 0;
    char *token = strtok(dup_path, "/");

    // Mientras que strtok brinde un elemento que puede ser comprobado...
    while (token != NULL) {
        if (strlen(token) > FILENAME_MAX - 1) return -EINVAL;
        // Se ignoran los elementos "." y ".."
        if (strcmp(token, "..") == 0 && count > 0) count--;
        else if (strcmp(token, ".") != 0) {
            // Nombre válido
            if (names != NULL && count < nnames) {
                names[count] = (char *)malloc(sizeof(char *));
                memset(names[count], 0, sizeof(char *));
                strcpy(names[count], token);
            }
            count++;
        }
        token = strtok(NULL, "/");
    }

    free(dup_path);

    // Si el número de nombres en el arreglo names excede el máximo...
    if (nnames != 0 && count > nnames) return -EXIT_FAILURE;
    return count;
}

/**
 * free_char_array - libera la memoria solicitada
 *   para un puntero de strings
 *
 * @param arr arreglo al que se le realizará la
 *   función "free"
 * @param len largo de arr
 */
void free_char_array(char *array[], int len) {

    // Se recorre al arreglo liberando a cada uno de sus elementos
    for (int i = 0; i < len; i++) {
        free(array[i]);
    }
}

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
int get_inode_id_from_path(char *path) {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);
    int root_inode_id = super_block->root_inode;

    if (strcmp(path, "/") == 0 || strlen(path) == 0) return root_inode_id;
    int inode_id = root_inode_id;
    // Se obtiene el número de nombres
    int num_names = parse(path, NULL, 0);
    // Si el número de errores excede el máximo se da un error
    if (num_names < 0) return -ENOTDIR;
    if (num_names == 0) return root_inode_id;
    // Se crea un arreglo para los nombres del tamaño antes obtenido y se reciben
    char *names[num_names];
    parse(path, names, num_names);
    // Se busca al nodo raíz
    my_inode *temp_inode = get_inode(inode_id);

    for (int i = 0; i < num_names; i++) {
        // Si el token no es un directorio se da un error
        if (!S_ISDIR(temp_inode->mode)) {
            free_char_array(names, num_names);
            return -ENOTDIR;
        }
        // Se busca al inodo en el arreglo de nombres en la posición i
        inode_id = lookup_for_filename(inode_id, names[i]);
        if (inode_id < 0) {
            free_char_array(names, num_names);
            return -ENOENT;
        }

        free(temp_inode);
        temp_inode = get_inode(inode_id);
    }

    free_char_array(names, num_names);
    free(temp_inode);
    free(super_block);

    return inode_id;
}

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
int get_inode_id_and_leaf_from_path(char *path, char *leaf) {

    // Se obtiene la información del superbloque y esta se descifra
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);
    int root_inode_id = super_block->root_inode;

    if (strcmp(path, "/") == 0 || strlen(path) == 0) return root_inode_id;
    int inode_id = root_inode_id;
    // Se obtiene el número de nombres
    int num_names = parse(path, NULL, 0);
    // Si el número de errores excede el máximo se da un error
    if (num_names < 0) return -ENOTDIR; // EXIT_FAILURE
    if (num_names == 0) return root_inode_id;
    // Se crea un arreglo para los nombres del tamaño antes obtenido y se reciben
    char *names[num_names];
    parse(path, names, num_names);
    // Se busca al nodo raíz
    my_inode *temp_inode = get_inode(inode_id);

    for (int i = 0; i < num_names -1; i++) {
        // Si el token no es un directorio se da un error
        if (!S_ISDIR(temp_inode->mode)) {
            free_char_array(names, num_names);
            return -ENOTDIR;
        }
        // Se busca al inodo en el arreglo de nombres en la posición i
        inode_id = lookup_for_filename(inode_id, names[i]);
        if (inode_id < 0) {
            free_char_array(names, num_names);
            return -ENOENT;
        }

        free(temp_inode);
        temp_inode = get_inode(inode_id);
    }
    // Se copia el último elemento del arreglo
    strcpy(leaf, names[num_names - 1]);

    if (!S_ISDIR(temp_inode->mode)) {
        return -ENOTDIR;
    }

    free(temp_inode);
    free_char_array(names, num_names);
    free(super_block);

    return inode_id;
}

/* to complete a runnable example, this abbreviated implementation of
 * get_data() will use libpng to read an image file. refer to libpng
 * documentation for details
 */
/**
 * get_image_data - se obtiene la información de una imagen
 *
 * @param name nombre del archivo del que se obtendrá la información
 * @param width ancho del archivo
 * @param height altura del archivo
 * @param raw puntero a datos vacíos que terminará conteniendo la
 *   información binaria del archivo buscado
 */
void get_image_data(const char *name, int *width, int *height, void **raw) {

    // Se abre el archivo con el código QR pasado por parámetro
    FILE *file = fopen(name, "rb");
    if(!file) {
        perror("Error al abrir un archivo QR.");
        return;
    }
    // Se crea la estructura con la información para generar un png
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                   NULL, NULL, NULL);
    if(!png) exit(EXIT_FAILURE);
    if(setjmp(png_jmpbuf(png))) exit(EXIT_FAILURE);
    png_infop info = png_create_info_struct(png);
    if(!info) exit(EXIT_FAILURE);
    png_init_io(png, file);
    png_read_info(png, info);

    // Se configura para la escala de grises 8bpp
    int color = png_get_color_type(png, info);
    int bits = png_get_bit_depth(png, info);
    if(color & PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if(color == PNG_COLOR_TYPE_GRAY && bits < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if(bits == 16)
        png_set_strip_16(png);
    if(color & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png);
    if(color & PNG_COLOR_MASK_COLOR)
        png_set_rgb_to_gray_fixed(png, 1, -1, -1);

    // Se reserva espacio para la imagen
    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    *raw = malloc(*width * *height);
    png_bytep rows[*height];
    int i;
    for(i = 0; i < *height; i++)
        rows[i] = *raw + (*width * i);
    png_read_image(png, rows);
}

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
gdImagePtr qrcode_png(QRcode *code, int fg_color[3], int bg_color[3], int size, int margin) {

    // Se rellena la información para generar un png a partir de una estructura QRcode de qrencode
    int code_size = size / code->width;
    code_size = (code_size == 0)  ? 1 : code_size;
    int img_width = code->width * code_size + 2 * margin;
    gdImagePtr img = gdImageCreate (img_width,img_width);
    int img_fgcolor =  gdImageColorAllocate(img,fg_color[0],fg_color[1],fg_color[2]);
    int img_bgcolor = gdImageColorAllocate(img,bg_color[0],bg_color[1],bg_color[2]);
    gdImageFill(img,0,0,img_bgcolor);
    u_char *p = code->data;
    int x, y, posx, posy;
    for (y = 0 ; y < code->width ; y++) {
        for (x = 0 ; x < code->width ; x++) {
            if (*p & 1) {
                posx = x * code_size + margin;
                posy = y * code_size + margin;
                gdImageFilledRectangle(img,posx,posy,posx + code_size,posy + code_size,img_fgcolor);
            }
            p++;
        }
    }
    return img;
}