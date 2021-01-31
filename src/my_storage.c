//
// Created by estudiante on 19/1/21.
//

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

void init_storage(char *new_qrfolder_path, char *new_password, int new_file_size) {

    printf("Iniciando los datos de las funciones de almacenamiento del sistema.\n");
    qrfolder_path = new_qrfolder_path;
    password = new_password;
    file_size = new_file_size;
    printf("Terminando de inicializar los datos del almacenamiento del programa.\n");
}

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

void *read_data(int block_num) {

    printf("Leyendo la información del bloque: %d.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return NULL;
    }

    /* create a reader */
    zbar_image_scanner_t *scanner = zbar_image_scanner_create();

    /* configure the reader */
    zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_BINARY, 1);

    /* obtain image data */
    int width = 0, height = 0;
    void *raw = NULL;

    char abs_file_name[MY_FILENAME_SIZE];
    sprintf(abs_file_name, "%sQRFS-%d", qrfolder_path, block_num);

    get_image_data(abs_file_name, &width, &height, &raw);

    /* wrap image data */
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, zbar_fourcc('Y','8','0','0'));
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

    /* scan the image for barcodes */
    int n = zbar_scan_image(scanner, image);

    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    //for(; symbol; symbol = zbar_symbol_next(symbol)) {
    /* do something useful with results */
    zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
    const char *data = zbar_symbol_get_data(symbol);

    //TODO cambiar los enteros
    char *to_return_data = malloc(1024);
    if(to_return_data == NULL) {
        perror("Error al asiganar memoria en read_data\n");
        return NULL; 
    }
    memcpy(to_return_data, data, 1024);

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    printf("Finalizando de leer la información de un bloque.\n");
    return (void *)to_return_data;
}

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

    memcpy(entries + offset, buf, len);

    if(write_data(entries, block_num) < 0) {
        perror("Error al escribit las entradas de directorio en write_data.");
        return;
    };

    printf("Terminando de escribir la información de un archivo.");
}

int set_inode_bitmap(int inode_num) {

    printf("Marcando el bit %d del bitmap de inodos.\n", inode_num);

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);

    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al obtener memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;
    void *temp_inode_map;

    for(int i=0; i < super_block->inode_map_sz; ++i) {
        temp_inode_map = read_data(inode_map_position+i);
        memcpy(ptr, temp_inode_map, MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);

        free(temp_inode_map);
        ptr += MY_BLOCK_SIZE;
    }

    FD_SET(inode_num, inode_map);

    block_cipher((void **)&inode_map_data, key);

    ptr = (void *)inode_map_data;
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        if(write_data(ptr, inode_map_position+i) < 0) {
            perror("Error al escribir el bitmap de inodos en write_data.");
            return -EIO;
        }

        ptr += MY_BLOCK_SIZE;
    }

    /*if(write_data(inode_map_data, inode_map_position) < 0) { // TODO hacer que si son más de un bloque hacerlo en un for
        perror("Error al escribir el bitmap de inodos en write_data.");
        return -EIO;
    }*/

    free(super_block);
    free(inode_map_data);

    printf("Terminando de marcar un bit en el bitmap de inodos.");
    return EXIT_SUCCESS;
}

int set_block_bitmap(int block_num) {

    printf("Marcando el bit %d del bitmap de bloques.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return -EXIT_FAILURE;
    }

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el super bloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    for(int i=0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    FD_SET(block_num, block_map);

    block_cipher((void **)&block_map_data, key);

    ptr = (void *)block_map_data;
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        if(write_data(ptr, block_map_position+i) < 0) {
            perror("Error al escribir el bitmap de inodos en write_data.");
            return -EIO;
        }

        ptr += MY_BLOCK_SIZE;
    }

    /*if(write_data(block_map_data, block_map_position) < 0) {
        perror("Error al escribir el bitmap de bloques en write_data.");
        return -EIO;
    }*/

    free(super_block);
    free(block_map_data);

    printf("Terminando de marcar un bit del bitmap de bloques.");
    return EXIT_SUCCESS;
}

int is_set_inode_bitmap(int inode_num) {

    printf("Comprobando si el bit %d del bitmap de inodos se encuentra activado.\n", inode_num);
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {

        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);

    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de inodos en malloc.");

        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;

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

int is_set_block_bitmap(int block_num) {

    printf("Comprobando si el bit %d del bitmap de bloques se encuentra activado.\n", block_num);
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }
    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

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

int clear_inode_bitmap(int inode_num) {

    printf("Limpiando el bit %d del bitmap de inodos.\n", inode_num);
    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);

    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;

    for(int i=0; i < super_block->inode_map_sz; ++i) {
        memcpy(ptr, read_data(inode_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

    FD_CLR(inode_num, inode_map);

    block_cipher((void **)&inode_map_data, key);

    ptr = (void *)inode_map_data;
    for(int i=0; i < super_block->inode_map_sz; ++i) {
        if(write_data(ptr, inode_map_position+i) < 0) {
            perror("Error al escribir el bitmap de inodos en write_data.");
            return -EIO;
        }

        ptr += MY_BLOCK_SIZE;
    }

    /*if(write_data(inode_map_data, inode_map_position) < 0) { //TODO hacer en un for por super_block->inode_map_sz
        perror("Error al escribir los datos del bitmap de inodos en write_data.");
        return -EIO;
    }*/

    free(super_block);
    free(inode_map_data);

    printf("Terminando de limpiar un bit del bitmap de inodos.\n");
    return EXIT_SUCCESS;
}

int clear_block_bitmap(int block_num) {

    printf("Limpiando el bit %d del bitmap de bloques.\n", block_num);

    if(block_num > NUMBER_OF_DATABLOCKS) {
        perror("El número de bloque sobrepasa el límite actual.");
        return -EXIT_FAILURE;
    }

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if (block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

    for (int i = 0; i < super_block->block_map_sz; ++i) {
        memcpy(ptr, read_data(block_map_position + i), MY_BLOCK_SIZE);
        block_decipher((void **)&block_map, key);
        ptr += MY_BLOCK_SIZE;
    }

    FD_CLR(block_num, block_map);
    block_cipher((void **)&block_map, key);

    if (write_data(block_map_data, block_map_position) < 0) {
        perror("Error al escribir la información del bitmap de bloques en write_data.");
        return -EIO;
    }

    free(super_block);
    free(block_map_data);

    printf("Terminando de limpiar un bit del bitmap de bloques.\n");
    return EXIT_SUCCESS;
}

int get_free_block() {

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }
    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

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
 * Count number of free blocks
 * @return number of free blocks
 */
int get_num_free_block() {

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    char *block_map_data = malloc(super_block->block_map_sz * MY_BLOCK_SIZE);
    if(block_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de bloques en malloc.");
        return -ENOMEM;
    }
    void *ptr = (void *)block_map_data;
    fd_set *block_map = ptr;

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
 * Returns a free inode number
 *
 * @return a free inode number or -ENOSPC if none available
 */
int get_free_inode() {

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if(super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }

    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);

    char *inode_map_data = malloc(super_block->inode_map_sz * MY_BLOCK_SIZE);
    if(inode_map_data == NULL) {
        perror("Error al asignar memoria para el bitmap de inodos en malloc.");
        return -ENOMEM;
    }

    void *ptr = (void *)inode_map_data;
    fd_set *inode_map = ptr;

    for(int i=0; i < super_block->inode_map_sz; ++i) {
        memcpy(ptr, read_data(inode_map_position+i), MY_BLOCK_SIZE);
        block_decipher((void **)&ptr, key);
        ptr += MY_BLOCK_SIZE;
    }

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

size_t read_indir1(int block_num, char *buf, size_t length, size_t offset) {

    uint32_t *blk_indices  = read_data(block_num);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / MY_BLOCK_SIZE;
    size_t blk_offset = offset % MY_BLOCK_SIZE;
    size_t len_to_read = length;
    while (blk_num < PTRS_PER_BLK && len_to_read > 0) {
        size_t cur_len_to_read = len_to_read > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_read;
        size_t temp = blk_offset + cur_len_to_read;

        if (!blk_indices[blk_num]) { // TODO revisar si el ser cero nos jode
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

size_t read_indir2(int block_num, char *buf, size_t length, size_t offset, int indir1_size) {

    uint32_t *blk_indices = read_data(block_num);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / indir1_size;
    size_t blk_offset = offset % indir1_size;
    size_t len_to_read = length;
    while (blk_num < PTRS_PER_BLK && len_to_read > 0) {
        size_t cur_len_to_read = len_to_read > indir1_size ? (size_t) indir1_size - blk_offset : len_to_read;
        size_t temp = blk_offset + cur_len_to_read;

        if (!blk_indices[blk_num]) {
            return length - len_to_read;
        }

        temp = read_indir1(blk_indices[blk_num], buf, temp, blk_offset);

        buf += temp;
        len_to_read -= temp;
        blk_num++;
        blk_offset = 0;
    }
    return length - len_to_read;
}

size_t write_indir1(int blk, const char *buf, size_t len, size_t offset) {

    uint32_t *blk_indices  = read_data(blk);
    if (blk_indices == NULL) 
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / MY_BLOCK_SIZE;
    size_t blk_offset = offset % MY_BLOCK_SIZE;
    size_t len_to_write = len;
    while (blk_num < PTRS_PER_BLK && len_to_write > 0) {
        size_t cur_len_to_write = len_to_write > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_write;
        size_t temp = blk_offset + cur_len_to_write;

        if (!blk_indices[blk_num]) { // Revisar si que sea 0 nos jode
            int freeb = get_free_block();
            if (freeb < 0) return len - len_to_write;
            blk_indices[blk_num] = freeb;
            //write back

            if(write_data(blk_indices, blk) < 0) { // TODO retstat
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

size_t write_indir2(size_t blk, const char *buf, size_t len, size_t offset, int indir1_size) {

    uint32_t *blk_indices = read_data(blk);
    if (blk_indices == NULL) {
        perror("Error al leer un bloque de índices en read_data.");
        return -EIO;
    }

    size_t blk_num = offset / indir1_size;
    size_t blk_offset = offset % indir1_size;
    size_t len_to_write = len;
    while (blk_num < PTRS_PER_BLK && len_to_write > 0) {
        size_t cur_len_to_write = len_to_write > indir1_size ? (size_t) indir1_size - blk_offset : len_to_write;
        size_t temp = blk_offset + cur_len_to_write;
        len_to_write -= temp;
        if (!blk_indices[blk_num]) { // TODO revisar si nos jode
            int freeb = get_free_block();
            if (freeb < 0) return len - len_to_write;
            blk_indices[blk_num] = freeb;
            //write back

            if(write_data(blk_indices, blk) < 0) {
                perror("Error al escribir un bloque de índices en write_data.");
                return -EIO;
            }
        }

        temp = write_indir1(blk_indices[blk_num], buf, temp, blk_offset);
        if (temp == 0) return len - len_to_write;
        buf += temp;
        blk_num++;
        blk_offset = 0;
    }
    return len - len_to_write;
}

my_inode *get_inode(int inode_id) {

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return NULL;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

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

int update_inode(int inode_id, my_inode *to_update_inode) {

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO;
    }
    uint32_t key = jenkins_one_at_a_time_hash(password, strlen(password));
    block_decipher((void **)&super_block, key);

    int block = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz +
            super_block->block_map_sz + (inode_id / INODES_PER_BLK);
    my_inode *inode_block = read_data(block);

    void *ptr = inode_block + (inode_id - ((inode_id/INODES_PER_BLK)*INODES_PER_BLK)); //Guardar en variable?

    memcpy(ptr, to_update_inode, sizeof(struct my_inode));

    if(write_data(inode_block, block) <0) {
        perror("Error al escribir el bloque de inodos en write_data");
        return -EIO;
    }

    free(super_block);
    free(inode_block);

    return EXIT_SUCCESS;
}

int find_in_dir(my_dirent *dir_entry, char *filename) {

    for(int i=0; i< DIR_ENTS_PER_BLK; ++i) {
        if(dir_entry[i].valid && strcmp(dir_entry[i].filename, filename) == 0) {
            return dir_entry[i].inode;
        }
    }
    return false;
}

/**
 * Find free directory entry.
 *
 * @return index of directory free entry or -ENOSPC
 *   if no space for new entry in directory
 */
int find_free_dir(my_dirent *de) {
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (!de[i].valid) {
            return i;
        }
    }
    return -ENOSPC;
}

/**
 * Determines whether directory is empty.
 *
 * @param de ptr to first entry in directory
 * @return 1 if empty 0 if has entries
 */
int is_empty_dir(my_dirent *de) {
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (de[i].valid) {
            return false;
        }
    }
    return true;
}

/**
 * Look up a single directory entry in a directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *
 */
int lookup_for_filename(int dir_inode_id, char *filename) {

    //get corresponding directory
    my_inode *cur_dir = get_inode(dir_inode_id);
    //init buff entries
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
 * Parse path name into tokens at most nnames tokens after
 * normalizing paths by removing '.' and '..' elements.
 *
 * If names is NULL,path is not altered and function  returns
 * the path count. Otherwise, path is altered by strtok() and
 * function returns names in the names array, which point to
 * elements of path string.
 *
 * @param path the directory path
 * @param names the argument token array or NULL
 * @param nnames the maximum number of names, 0 = unlimited
 * @return the number of path name tokens
 */
int parse(char *path, char *names[], int nnames) {

    char *dup_path = strdup(path);
    int count = 0;
    char *token = strtok(dup_path, "/");

    while (token != NULL) {
        if (strlen(token) > FILENAME_MAX - 1) return -EINVAL; // EXIT_FAILURE
        if (strcmp(token, "..") == 0 && count > 0) count--;
        else if (strcmp(token, ".") != 0) {
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

    //if the number of names in the path exceed the maximum
    if (nnames != 0 && count > nnames) return -EXIT_FAILURE; //TODO free names?? era un -1
    return count;
}

/**
 * free allocated char ptr array
 * @param arr arr to be freed
 */
void free_char_array(char *array[], int len) {

    for (int i = 0; i < len; i++) {
        free(array[i]);
    }
}

/**
 * Return inode number for specified file or
 * directory.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @return inode of path node or error
 */
int get_inode_id_from_path(char *path) {

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
    //get number of names
    int num_names = parse(path, NULL, 0);
    //if the number of names in the path exceed the maximum, return an error, error type to be fixed if necessary
    if (num_names < 0) return -ENOTDIR; //ENOTDIR
    if (num_names == 0) return root_inode_id;
    //copy all the names
    char *names[num_names];
    parse(path, names, num_names);
    //lookup inode
    my_inode *temp_inode = get_inode(inode_id);

    for (int i = 0; i < num_names; i++) {
        //if token is not a directory return error
        if (!S_ISDIR(temp_inode->mode)) {
            free_char_array(names, num_names);
            return -ENOTDIR;
        }
        //lookup and record inode
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
 *  Return inode number for path to specified file
 *  or directory, and a leaf name that may not yet
 *  exist.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param leaf pointer to space for FS_FILENAME_SIZE leaf name
 * @return inode of path node or error
 */
int get_inode_id_and_leaf_from_path(char *path, char *leaf) {

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
    //get number of names
    int num_names = parse(path, NULL, 0);
    //if the number of names in the path exceed the maximum, return an error, error type to be fixed if necessary
    if (num_names < 0) return -ENOTDIR; // EXIT_FAILURE
    if (num_names == 0) return root_inode_id;
    //copy all the names
    char *names[num_names];
    parse(path, names, num_names);
    //lookup inode
    my_inode *temp_inode = get_inode(inode_id);

    for (int i = 0; i < num_names -1; i++) {
        //if token is not a directory return error
        if (!S_ISDIR(temp_inode->mode)) {    //sys/stat.h
            free_char_array(names, num_names);
            return -ENOTDIR; // EXIT_FAILURE
        }
        //lookup and record inode
        inode_id = lookup_for_filename(inode_id, names[i]);
        if (inode_id < 0) {
            free_char_array(names, num_names);
            return -ENOENT; // EXIT_FAILURE
        }
        free(temp_inode);
        temp_inode = get_inode(inode_id);
    }
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
void get_image_data(const char *name, int *width, int *height, void **raw) {

    FILE *file = fopen(name, "rb");
    if(!file) {
        perror("Error al abrir un archivo QR.");
        return;
    }
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                   NULL, NULL, NULL);
    if(!png) exit(3);
    if(setjmp(png_jmpbuf(png))) exit(4);
    png_infop info = png_create_info_struct(png);
    if(!info) exit(5);
    png_init_io(png, file);
    png_read_info(png, info);
    /* configure for 8bpp grayscale input */
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
    /* allocate image */
    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    *raw = malloc(*width * *height);
    png_bytep rows[*height];
    int i;
    for(i = 0; i < *height; i++)
        rows[i] = *raw + (*width * i);
    png_read_image(png, rows);
}

gdImagePtr qrcode_png(QRcode *code, int fg_color[3], int bg_color[3], int size, int margin) {

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