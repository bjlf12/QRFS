/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de montar el sistema de archivos en el equipo, se encuentran las funciones del sistema de
 * archivos las cuales funcionan bajo las interfaces que provee FUSE.
 */

#include "mount.qrfs.h"
#include "my_super.h"
#include "my_inode.h"
#include "my_storage.h"

#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>

/** Tamaño maximo de los bloques directos */
static int DIR_SIZE = MY_BLOCK_SIZE * NUM_DIRECT_ENT;
/** Cantidad de bloques directos en el primer enlace directo*/
static int INDIR1_SIZE = (MY_BLOCK_SIZE / sizeof(uint32_t)) * MY_BLOCK_SIZE;
/** Cantidad total de bloques directos en el segundo enlace */
static int INDIR2_SIZE = (MY_BLOCK_SIZE / sizeof(uint32_t)) * (MY_BLOCK_SIZE / sizeof(uint32_t)) * MY_BLOCK_SIZE;

/**
 * my_init: llamada por FUSE a la hora de iniciar
 *
 * @param conn información para la conexión de FUSE - no implementado
 * @return puntero al inicio del buffer
 */
void *my_init(struct fuse_conn_info *conn) {
    return NULL;
}

/**
 * strmode - Función utilizada para transformar una mascara de cuyo valor es un entero a string
 *
 * @param buf resultado de la operación
 * @param mode mascara de entrada para convertir
 * @return puntero al buffer del string de entrada
 */
char *strmode(char *buf, int mode) {

    int mask = 0400;
    char *str = "rwxrwxrwx", *retval = buf; // Se crea el puntero al inicio
    *buf++ = S_ISDIR(mode) ? 'd' : '-'; // compara si es directorio
    for (mask = 0400; mask != 0; str++, mask = mask >> 1) // se produce un desplazamiento de bits por iteración hasta 0
        *buf++ = (mask & mode) ? *str : '-'; // Se realiza una operacion AND para verificar igualdad
    *buf++ = 0;

    return retval; // puntero al inicio del buffer
}

/**
 * my_getattr - obtiene los atributos correspodientes de los archivos y directorios del sitema de archivos
 *
 * @param path ruta del archivo que se desea saber sus atributos
 * @param stbuf puntero a la estructura stat del sistema que almacena los datos de los archivos
 * @return 0 si termina correctamente, un entero negativo en caso que no exista la ruta
 */
int my_getattr(const char *path, struct stat *stbuf){

    printf("Obteniendo los atributos del archivo %s con my_getattr.\n", path);

    char *dup_path = strdup(path); // Se duplica la dirección para evitar modificaciones
    int inode_id = get_inode_id_from_path(dup_path); // Se obtiene el id del nodo apartir de la ruta
    if (inode_id < 0) return inode_id; // negativo si no se encontro un inodo con dicha ruta
    my_inode *inode = get_inode(inode_id); // se leen los datos del inodo
    cpy_stat(inode, stbuf); // se copia en la estructura del sistema los valores del inode obtenido

    free(inode);
    free(dup_path);

    printf("Imprimiendo el estado de un archivo/directorio.\n");
    char mode[16], time[26], *lasts;
    /** Se imprime los datos en la terminal*/
    printf("%5lld %s %2d %4d %4d %8lld %s %s\n",
           stbuf->st_blocks, strmode(mode, stbuf->st_mode),
           stbuf->st_nlink, stbuf->st_uid, stbuf->st_gid, stbuf->st_size,
               strtok_r(ctime_r(&stbuf->st_mtime,time),"\n",&lasts), path);

    printf("Finalizando de obtener los atributos del archivo.\n");
    return EXIT_SUCCESS; /** Se retorna 0, finaliza correctamente */
}

/**
 * my_create - crea un nuevo archivo en el sistema de archivos con permisos de escritura,
 * lectura y execución segun el modo.
 *
 * Errores:
 *  EEXIST - ya existe un archivo con dicho nombre en el mismo directorio
 *  ENOTDIR - el archivo no se encuentra en el directorio
 *  EIO - error al leer o escribir datos del disco
 *
 * @param path ruta en la que se desea crear el nuevo archivo
 * @param mode mascara con los permisos del archivo a crear
 * @param fi información de archivo de FUSE
 * @return 0 en caso correcto, un código error en otro caso
 */
int my_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

    printf("Creando el archivo %s con la función my_create.\n", path);

    mode |= S_IFREG; /** Se realiza un OR para los permisos del usuario*/
    if (!S_ISREG(mode) || strcmp(path, "/") == 0) return -EINVAL; /** Si de modifica la raiz o no hay permisos*/
    char *dup_path = strdup(path); /** Duplica la ruta*/
    char name[FILENAME_MAX];
    int inode_id = get_inode_id_from_path(dup_path); /** Se busca el i-nodo apartir de la ruta de origen */
    int parent_inode_id = get_inode_id_and_leaf_from_path(dup_path, name); /** Se obtiene el i-nodo padre, el directorio*/

    if (inode_id >= 0) return -EEXIST; /** Ya existe un inodo con el mismo nombre */

    if (parent_inode_id < 0) return parent_inode_id; /** No se encontro el i-nodo del padre - directorio*/
    //read parent info
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR; /** En caso que el i-nodo padre no se un directorio */

    my_dirent *entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO; /** Si ocurre un error al leer de disco */
    }

    int res = set_attributes_and_update(entries, name, mode, false); /** Se asignan los atributos del archivo */
    if (res < 0) return res; /** En caso que no se haya podido asiganar los atributos del archivo */

    if(write_data(entries, parent_inode->direct[0]) < 0) {
        perror("Error al escribir una entrada de directorio en write_data.");
        return -EIO; /** Error al intentar escribir los datos de entries en disco */
    }

    free(parent_inode);
    free(entries);
    free(dup_path);

    printf("Terminando de crear un directorio.\n");
    return EXIT_SUCCESS; /**Se termino correctamente la creación del archivo*/
}

/**
 * my_open - función utilizada para abrir un archivo del sistema de archivos
 *
 * Errores:
 *  EISDIR - No es un directorio
 *
 * @param path - Ruta del archivo
 * @param fi - información de archivo de fuse
 * @return 0 en caso correcto, entero negativo o un error en caso contrario
 */
int my_open(const char *path, struct fuse_file_info *fi) {

    printf("Abriendo el archivo %s con la función my_open.\n", path);

    char *dup_path = strdup(path); /**Se duplica la ruta*/
    int inode_id = get_inode_id_from_path(dup_path); /**Se obtiene el id del nodo segun la ruta pasada por parametros*/
    if (inode_id < 0) return inode_id; /**No existe la ruta en el sistema de archivos*/
    my_inode *inode = get_inode(inode_id); /**Se lee la info del i-nodo*/
    if (S_ISDIR(inode->mode)) return -EISDIR; /**Error en caso que sea un directorio*/
    fi->fh = (uint64_t) inode_id; /**Se actualiza el manejador de archivo de la estructura de info de FUSE*/

    free(inode);
    free(dup_path);

    printf("Terminando de abrir el archivo.\n");
    return EXIT_SUCCESS; /**Se abrio correctamente el archivo*/
};

/**
 * my_read_dir - Función utilizada para leer los directorios del sistema de archivos
 *
 * @param inode_id - identificador del nodo que se desea leer
 * @param buf
 * @param length
 * @param offset
 * @return
 */
size_t my_read_dir(int inode_id, char *buf, size_t length, size_t offset) {

    printf("Leyendo el directorio con el identificador %d con la función my_read_dir.\n", inode_id);

    my_inode *inode = get_inode(inode_id); /** Obtiene la información del i-nodo*/
    size_t block_num = offset / MY_BLOCK_SIZE; /**Se calcula la cantidad de bloques que se deebn leer*/
    size_t blk_offset = offset % MY_BLOCK_SIZE; /**Complemento del valor bloque a leer*/
    size_t len_to_read = length; /** el largo de lo que se lee*/
    /**Se itera mientras no se hayan leido el total de bloques o aun hay datos a leer*/
    while (block_num < NUM_DIRECT_ENT && len_to_read > 0) {
        /**Se calcula el largo de la cantidad de datos actual a leer*/
        size_t cur_len_to_read = len_to_read > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_read;
        size_t temp = blk_offset + cur_len_to_read; /**Se aumenta el bloque*/

        if (!inode->direct[block_num]) { /**si no esta asignado en la posicion del numero de los bloques directos*/
            return length - len_to_read; /** se retorna el largo del directorio*/
        }
        /** Se lee la información del disco y se guarda en el buffer*/
        read_file_data(inode->direct[block_num], buf, temp, blk_offset); // TODO free buf o if

        buf += temp; /** Se avanza en el buffer*/
        len_to_read -= temp; /** se disminuye el largo a leer*/
        block_num++; /**se incrementa el número de bloque*/
        blk_offset = 0;
    }

    free(inode);

    printf("Finalizando de leer los datos de un directorio.\n");
    return length - len_to_read; /**Se retorna el largo del buffer*/
}

/**
 * my_read - Función encargada de leer los datos de una archivo que se encuentre abierto,
 * retorna el número de bytes que se le solicita.
 *
 * Errores:
 *  ENOENT - cuando el archivo no existe
 *  ENOTDIR - cuando no es un directorio
 *  EIO - al leer o escribir datos del disco
 *
 * @param path - la ruta del archivo
 * @param buf - el buffer de los datos a leer
 * @param length - la cantidad de bytes a leer
 * @param offset - la posición donde se lee
 * @param fi - estructura de fuse para la info de un archivo
 * @return el número de bits cuando es correcto, o un error en otro caso
 */
int my_read(const char *path, char *buf, size_t length, off_t offset, struct fuse_file_info *fi) {

    printf("Leyendo el archivo %s con la función my_read.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path); /**Se busca el inodo*/
    if (inode_id < 0) return inode_id; /**negativo si no se encuentra*/
    my_inode *inode = get_inode(inode_id); /**se obtiene la info del archivo*/
    if (S_ISDIR(inode->mode)) return -EISDIR; // EXIT_FAILURE
    if (offset >= inode->size) return 0;

    if (offset + length > inode->size) {
        length = (size_t) inode->size - offset; /** si se lee más del largo del archivo*/
    }

    size_t len_to_read = length; /** el largo a leer*/

    /**se lee la información de los bloques directos*/
    if (len_to_read > 0 && offset < DIR_SIZE) {
        //len finished read
        size_t temp = my_read_dir(inode_id, buf, len_to_read, (size_t) offset); /** lee la información de los bloques directos */
        len_to_read -= temp;
        offset += temp;
        buf += temp;
    }
    /**se leen los valores de los primeros bloques en directo en caso que aun falte de leer*/
    if (len_to_read > 0 && offset < DIR_SIZE + INDIR1_SIZE) {
        size_t temp = read_indir1(inode->indir_1, buf, len_to_read, (size_t) offset - DIR_SIZE);
        len_to_read -= temp;
        offset += temp;
        buf += temp;
    }
    /**se leen los valores de los segundos bloques en directo en caso que aun falte de leer*/
    if (len_to_read > 0 && offset < DIR_SIZE + INDIR1_SIZE + INDIR2_SIZE) {
        size_t temp = read_indir2(inode->indir_2, buf, len_to_read, (size_t) offset - DIR_SIZE - INDIR1_SIZE, INDIR1_SIZE);
        len_to_read -= temp;
        offset += temp;
        buf += temp;
    }

    free(inode);
    free(dup_path);

    printf("Finalizando de leer los datos de un archivo.\n");

    return (int) (length - len_to_read); /** se retorna el número de bytes leidos*/
};

/**
 * my_write_dir - Función utilizada para escribir los datos de un archivo en los bloques directos
 *
 * @param to_write_inode - información del inodo que se desea escribir los datos
 * @param buf - se almacenan los datos del archivo
 * @param len - largo de los bytes a leer
 * @param offset - posición donde se va a leer
 * @return el largo de los datos escritos
 */
size_t my_write_dir(my_inode **to_write_inode, const char *buf, size_t len, size_t offset) {

    printf("Escribiendo información de un archivo con la función my_write_dir.\n");

    my_inode *inode = *to_write_inode;

    size_t blk_num = offset / MY_BLOCK_SIZE; /** se calcula la cantidad de bloques*/
    size_t blk_offset = offset % MY_BLOCK_SIZE; /** el bloque donde se va a escribir*/
    size_t len_to_write = len;
    /**Se escribe mientras hayan bloques directos disponibles p se termino de escribir*/
    while (blk_num < NUM_DIRECT_ENT && len_to_write > 0) {
        size_t cur_len_to_write = len_to_write > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_write;
        size_t temp = blk_offset + cur_len_to_write;

        if (!inode->direct[blk_num]) {/**si hay un bloque directo disponible*/
            int freeb = get_free_block(); /**se obtiene un número de bloque libre*/
            if (freeb < 0) return len - len_to_write; /**si no hay libres*/
            inode->direct[blk_num] = freeb; /**se asigna el número de bloque*/
        }
        /** se esbriben los datos en el bloque obtenidos del buffer*/
        write_file_data(inode->direct[blk_num], buf, temp, blk_offset);

        buf += temp;
        len_to_write -= temp;
        blk_num++;
        blk_offset = 0;
    }

    printf("Terminando de escribir la información de un directorio.\n");
    return len - len_to_write; /**cantidad de bytes escritos*/
}


 /**
  *  my_write - Función para escribir datos en un archivo
  * @param path - ruta del archivo a escribir
  * @param buf - datos que se van a escribir
  * @param length - cantidad de datos a escribir
  * @param offset - donde inicia a escribir
  * @param fi  - información del archivo de FUSE
  * @return la cantidad de bytes escritos si termina correctamente, o bien, 0 si se escribe más haya del bloque, un error
  */
int my_write(const char *path, const char *buf, size_t length, off_t offset,
             struct fuse_file_info *fi) {

    printf("Escribiendo información del archivo %s con la función my_write.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path); /**se obtiene el id*/
    if (inode_id < 0) return inode_id; /** no se encintro el inodo*/
    my_inode *inode = get_inode(inode_id);
    if (S_ISDIR(inode->mode)) return -EISDIR; /**si es un directorio*/
    if (offset > inode->size) return 0; /**si se escribe más haya del bloque*/

    /**el largo que falta por escribir*/
    size_t len_to_write = length;

    /**se escribe en los bloques directos*/
    if (len_to_write > 0 && offset < DIR_SIZE) {
        size_t temp = my_write_dir(&inode, buf, len_to_write, (size_t) offset);/**se llenan los bloques directos*/
        len_to_write -= temp;
        offset += temp;
        buf += temp;
    }

    /**si falta por escribir se inicia en los primeros bloques indirectos*/
    if (len_to_write > 0 && offset < DIR_SIZE + INDIR1_SIZE) {
        if (!inode->indir_1) { /**si aun entran bloques indirectos*/
            int freeb = get_free_block(); /**obtiene un bloque libre*/
            if (freeb < 0) return length - len_to_write; /**no hay bloques libres*/
            inode->indir_1 = freeb; /**apunta al bloque libre*/
            update_inode(inode_id, inode); /**actualiza*/
        }
        /**se escribe en el disco*/
        size_t temp = write_indir1(inode->indir_1, buf, len_to_write, (size_t) offset - DIR_SIZE);
        len_to_write -= temp;
        offset += temp;
        buf += temp;
    }

     /**si falta por escribir se continua en los segundos bloques indirectos*/
    if (len_to_write > 0 && offset < DIR_SIZE + INDIR1_SIZE + INDIR2_SIZE) {
        if (!inode->indir_2) { /**si aun entran bloques indirectos*/
            int freeb = get_free_block(); /**obtiene un bloque libre*/
            if (freeb < 0) return length - len_to_write; /**no hay bloques libres*/
            inode->indir_1 = freeb; /**apunta al bloque libre*/
            update_inode(inode_id, inode); /**actualiza*/
        }
        /**se escribe en el disco*/
        size_t temp = write_indir2(inode->indir_2, buf, len_to_write, (size_t) offset - DIR_SIZE - INDIR1_SIZE, INDIR1_SIZE);
        len_to_write -= temp;
        offset += len_to_write;
    }

    /** se actualiza el tamaño*/
    if (offset > inode->size) inode->size = offset;

    update_inode(inode_id, inode);

    free(inode);
    free(dup_path);

    printf("Finalizando de escribir la información de un archivo.\n");
    return (int) (length - len_to_write); /**La cantidad de bytes de los datos escritos*/
};

 /**
  * my_rename - función para cambiar el nombre de un archivo o un directorio del sisema de archivos.
  *
  * Errores:
  *  ENOENT - no existe el archivo o directorio
  *  ENOTDIR - el archivo no se encuentra en el directorio
  *  EEXIST - ya existe el archivo
  *
  *
  * @param src_path - nombre actual
  * @param dst_path - nombre a cambiar
  * @return 0 si es correcto, un negativo o error el caso contrario
  */
int my_rename(const char *src_path, const char *dst_path) {

    printf("Renombrando el archivo %s a %s con la función my_rename.\n", src_path, dst_path);

    /**se duplican las rutas*/
    char *dup_src_path = strdup(src_path);
    char *dup_dst_path = strdup(dst_path);
    /**se obtienen los inodos*/
    int src_inode_id = get_inode_id_from_path(dup_src_path);
    int dst_inode_id = get_inode_id_from_path(dup_dst_path);
    /**error si no existe el nombre actual*/
    if (src_inode_id < 0) return src_inode_id;
     /**error si no existe el nombre final*/
    if (dst_inode_id >= 0) return -EEXIST;

    /**se obtiene el inodo del directorio*/
    char src_name[FILENAME_MAX];
    char dst_name[FILENAME_MAX];
    int src_parent_inode_id = get_inode_id_and_leaf_from_path(dup_src_path, src_name);
    int dst_parent_inode_id = get_inode_id_and_leaf_from_path(dup_dst_path, dst_name);
    /** se verifica que sean del mismo directorio*/
    if (src_parent_inode_id != dst_parent_inode_id) return -EINVAL; /**son directorios distintos*/
    int parent_inode_id = src_parent_inode_id;
    if (parent_inode_id < 0) return parent_inode_id; /**no existe el directorio*/

    /**se lee el inodo directo*/
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR; /**no se encuentra el directorio*/

    my_dirent *entries = read_data(parent_inode->direct[0]); /** se lee la información del disco*/
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO; /**error al leer*/
    }

    /**cambia los valores de los bloques directos*/
    for (int i = 0; i < DIR_ENTS_PER_BLK; ++i) {
        if (entries[i].valid && strcmp(entries[i].filename, src_name) == 0) {
            memset(entries[i].filename, 0, sizeof(entries[i].filename));
            strcpy(entries[i].filename, dst_name);
        }
    }

    /**guarda los cambios en disco*/
    if(write_data(entries, parent_inode->direct[0]) < 0) {
        perror("Error al escribir una entrada de directorio en write_data.");
        return -EIO; /**no se puede guardar los cambios*/
    }
    /**se libera memoria*/
    free(parent_inode);
    free(dup_src_path);
    free(dup_dst_path);
    free(entries);

    printf("Terminando de cambiar el nombre de un archivo.\n");
    return EXIT_SUCCESS;/**finaliza correctamente*/
}

 /**
  * my_mkdir - permite crear directorios en el sistema de archivos.
  *
  * Errores:
  *  ENOTDIR - No es un directorio
  *  EEXIST - Ya existe el directorio
  *  EIO - Error de escritura o lectura
  *
  * @param path - ruta donde se crea el directorio
  * @param mode - mascara del directorio a crear
  * @return 0 si es correcto, negativo o un error en otro caso.
  */
int my_mkdir(const char *path, mode_t mode) {

    printf("Creando el directorio %s con la función my_mkdir.\n", path);

    /**se obtiene la información del directorio*/
    mode |= S_IFDIR;
    if (!S_ISDIR(mode) || strcmp(path, "/") == 0) return -EINVAL; /** no hay permiso o es la raíz*/
    char *dup_path = strdup(path);
    char name[FILENAME_MAX];
    int inode_id = get_inode_id_from_path(dup_path); /**se obtien la información del inodo*/
    int parent_inode_id = get_inode_id_and_leaf_from_path(dup_path, name); /**inodo padre*/
    if (inode_id >= 0) return -EEXIST; /**ya existe un directorio*/
    if (parent_inode_id < 0) return parent_inode_id; /**no existe el directorio padre*/
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR;/**no es un directorio*/

    my_dirent *entries;
    entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) { /**se lee información del disco*/
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO; /**error al leer de disco*/
    }

    /** se asigna el inodo y directorio*/
    int res = set_attributes_and_update(entries, name, mode, true);
    if (res < 0) return res; /**no se puede crear*/

    if(write_data(entries, parent_inode->direct[0]) < 0) { /**se guardan los cambios*/
        perror("Error al escribir una entrada de directorio en write_data.");
        return -EIO; /**error al guardarlos*/
    }

    free(parent_inode);
    free(entries);
    free(dup_path);

    printf("Terminando de crear un directorio.\n");
    return EXIT_SUCCESS;/**finaliza correctamente*/
};

/**
 * my_readdir - Obtiene el contenido presente de un directorio del sistema de archivos
 *
 * Errores:
 *  EIO - error de escritura o lectura
 *  ENOTDIR - no es un directorio
 *
 * @param path - ruta del directorio
 * @param buf - se almacena la información leida
 * @param filler - función de relleno para llamar para cada entrada
 * @param offset - donde se inicia leer
 * @param fi - estructura de info de archivo de FUSE
 * @return 0 si es correcto, negativo o error en otro caso
 */
int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi) {

    printf("Leyendo la información del directorio %s con la función my_readdir.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path); /**se obtiene un inodo de la ruta*/
    if (inode_id < 0) return inode_id; /**si no se encuentra*/
    my_inode *inode = get_inode(inode_id); /**información del inodo*/
    if (!S_ISDIR(inode->mode)) return -ENOTDIR;  /**si no es un directorio*/
    my_dirent *entries;
    struct stat sb;

    entries = read_data(inode->direct[0]); /**se leer los datos del bloque en disco*/
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO; /**eror al leer*/
    }
    /**se leen los bloques directos */
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (entries[i].valid) { /**si es validad*/
            cpy_stat(get_inode(entries[i].inode), &sb);
            filler(buf, entries[i].filename, &sb, 0); /**se llenan los datos en el buffer*/
        }
    }

    free(inode);
    free(entries);
    free(dup_path);

    printf("Terminando de leer la informacion de un directorio.\n");
    return EXIT_SUCCESS; /**finaliza correctamente*/
};

/**
 * my_opendir - Función para abrir directorios del sistema de archivos
 *
 * Errores:
 *  ENOTDIR - no es directorio
 * @param path - ruta directorio
 * @param fi - informació de fuse
 * @return 0 en caso correcto, sino código de error o negativo
 */
int my_opendir(const char *path, struct fuse_file_info *fi) {

    printf("Abriendo el directorio %s con la función my_opendir.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path); /**obtiene el inodo*/
    if (inode_id < 0) return inode_id; /**no existe*/
    if (!S_ISDIR(get_inode(inode_id)->mode)) return -ENOTDIR; /**no es un directorio*/
    fi->fh = (uint64_t) inode_id;/**se actualiza el manejador de archivos*/

    free(dup_path);

    printf("Terminando de abrir un directorio.\n");
    return EXIT_SUCCESS; /**exito*/
}

/**
 * rmdir - remove a directory.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -ENOTDIR  - path not a directory
 *   -ENOEMPTY - directory not empty
 *
 * @param path the path of the directory
 * @return 0 if successful, or error value
 */
 /**
  * my_rmdir - Función para eliminar directorios del sistema de archivos
  *
  * Errores;
  *  EINVAL - no es valido
  *  ENOENT - no existe
  *  ENOTDIR - no es un directorio
  *
  * @param path - ruta del directorio
  * @return
  */
int my_rmdir(const char *path) {

    printf("Eliminando el directorio %s con la función my_rmdir.\n", path);

    /**no se borra el directorio raíz*/
    if (strcmp(path, "/") == 0) return -EINVAL;

    char *dup_path = strdup(path);
    char name[FILENAME_MAX];
    int inode_id = get_inode_id_from_path(dup_path); /**obtiene inodo*/
    int parent_inode_id = get_inode_id_and_leaf_from_path(dup_path, name);/**obtiene el padre*/
    my_inode *inode = get_inode(inode_id);
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (inode_id < 0 || parent_inode_id < 0) return -ENOENT; /**No se encuentra el directorio*/
    if (!S_ISDIR(inode->mode)) return -ENOTDIR; /** No es un directorio*/
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR; /** el padre no es un directorio*/

    my_dirent *entries;
    entries = read_data(inode->direct[0]);
    if(entries == NULL) { /**se lee de disco*/
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO; 
    }
    int res = is_empty_dir(entries); /**verificar si esta vacio*/
    if (res == 0) return -ENOTEMPTY; /**no se puede borrar*/

    memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));
    // write_data(entries, inode->direct[0]); TODO revisar si es necesario

    /**se leer la información de disco del padre*/
    entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.");
        return -EIO;
    }
    /**Se actualizan las entradas de directorio*/
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (entries[i].valid && strcmp(entries[i].filename, name) == 0) {
            memset(&entries[i], 0, sizeof(my_dirent));
        }
    }
    if(write_data(entries, parent_inode->direct[0]) < 0) { /**se guardan los cambios*/
        perror("Error al escribir una entrada de directorio en write_data.");
        return -EIO;
    }

    /**se libera el bloque y el inodo de los respectivos mapas*/
    clear_block_bitmap(inode->direct[0]);
    clear_inode_bitmap(inode_id);
    memset(inode, 0, sizeof(my_inode));

    /**actualiza el inodo*/
    update_inode(inode_id, inode);

    free(dup_path);
    free(inode);
    free(parent_inode);
    free(entries);

    printf("Terminando de eliminar un directorio.\n");
    return EXIT_SUCCESS;
}

 /**
  * my_statfs - Función para obtener las estadisticas del sistema de archivos, da tamaño de bloque, bloques libres,
  * largo de nombre,
  *
  * Errores:
  *  EIO - error al leer
  *
  * @param path - ruta del archivo
  * @param statv - estructura del sistma statvfs
  * @return 0 exitosamente, error en otro caso
  */
int my_statfs(const char *path, struct statvfs *statv) {

    printf("Obteniendo el estado del archivo %s con la función my_statfs.\n", path);

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.");
        return -EIO; /**no se lee el superbloque*/

    }
    int root_inode_id = super_block->root_inode;
    int inode_data_base = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz + super_block->block_map_sz;

    /**se limpia la estructura y se añaden los datos correspondientse del sistema de archivos*/
    memset(statv, 0, sizeof(*statv));
    statv->f_bsize = MY_BLOCK_SIZE;
    statv->f_blocks = (fsblkcnt_t) (NUMBER_OF_DATABLOCKS - root_inode_id - inode_data_base);
    statv->f_bfree = (fsblkcnt_t) get_num_free_block();
    statv->f_bavail = statv->f_bfree;
    statv->f_namemax = FILENAME_MAX - 1;

    printf("Terminando de obtener la informacion de un archivo.\n");
    return EXIT_SUCCESS;

}
/**
 * my_fsync - Copia los datos de los archivos que se enuentran en memoria sicronizarlos en disco.
 *
 * Errores:
 *  ENOENT - no existe
 *
 * @param path - ruta del archivo
 * @param datasync - valor del estado de la información
 * @param fi - estructura de archivo de FUSE.
 * @return o si termina correctamente, negativo o error en otro caso
 */
int my_fsync(const char *path, int datasync, struct fuse_file_info *fi) {

    printf("Sincronizando los datos del archivo %s.\n", path);

    /**se obtiene el inodo*/
    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if(inode_id<0) {
        perror("Error al buscar un archivo en el sistema de archivos.");
        return -ENOENT;
    }

    int ret_stat;
    fi->fh = inode_id; /**se actualiza el manejador de archivo*/
    if(datasync) { /**vacia solo los datos del usuario*/
        ret_stat = fdatasync(fi->fh);
        if(ret_stat < 0) {
            perror("Error al utilizar la función fdatasync.");
            return ret_stat;
        }
    }
    else {/**vacia los datos del usuario y los metadatos*/
        ret_stat = fsync(fi->fh);
        if(ret_stat < 0) {
            perror("Error al utilizar la función fsync.");
            return ret_stat;
        }
    }

    free(dup_path);

    printf("Terminando de sincronizar los datos de un archivo.\n");
    return EXIT_SUCCESS;
}

/**
 * my_access - Se encarga de comprobar sobre el acceso a los datos del sistema de archivos
 *
 * Errores:
 *
 *
 * @param path - ruta del archivo o directorio
 * @param mask - mascara o modo de acceso
 * @return 0 correctamente, -1 en caso contrario
 */
int my_access(const char *path, int mask) { //TODO arreglar

    printf("Utilizando la función my_access con el archivo %s, con la máscara: %d.\n", path, mask);


    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path); /**obtiene el id del inodo*/

    if(inode_id < 0) {
        printf("Terminando de verificar el acceso con my_access.\n");
        return -EXIT_FAILURE; /**No existe*/
    }
    if(mask == F_OK) {
        printf("Terminando de verificar el acceso con my_access.\n");
        return EXIT_SUCCESS; /**Existe*/
    }
    my_inode *inode = get_inode(inode_id);

    /**se compara el modo del nodo con el recibido con una operación AND*/
    if(inode->mode & (mask*0100)) { // TODO nos deja leer aunque no hayna permisos
        printf("Terminando la función my_access.\n");
        return EXIT_SUCCESS; /**tiene acceso*/
    }
    free(dup_path);
    printf("Terminando de verificar el acceso con my_access.\n");
    return -EXIT_FAILURE; /** no tiene acceso*/
}

/**
 * my_chmod - Función para cambiar los permisos de escritura, lectura y ejecucion de los directorios o archivos
 * @param path - ruta del fichero o archivo
 * @param mode - marcara con los permisos
 * @return 0 si es correcto, otro valor en otro caso
 */
int my_chmod(const char *path, mode_t mode) {

    printf("Haciendo un chmod.\n");
    //printf("resultado mkdir: %d\n", my_mkdir("/dir1", 0777));
    char* dup_path = strdup(path);
    int inode_idx = get_inode_id_from_path(dup_path); /**se obtiene el inodo*/
    if (inode_idx < 0) return inode_idx; /**no existe*/
    my_inode *inode = get_inode(inode_idx);
    /**Verifica si tiene otro modo*/
    mode |= S_ISDIR(inode->mode) ? S_IFDIR : S_IFREG;
    /**cambia el modo*/
    inode->mode = mode;
    update_inode(inode_idx, inode);

    free(inode);

    printf("Terminando con el chmod\n");
    return EXIT_SUCCESS;
}

/**
 * Estructura utilizada por fuse para tener acceso a nuestra implementación de las funciones,
 * contiene el nombre de las funciones que se utilizaran en el sistema de archivos
 */
struct fuse_operations my_oper = {
        .getattr = my_getattr,
        .mkdir = my_mkdir,
        .rmdir = my_rmdir,
        .rename = my_rename,
        .create = my_create,
        .open = my_open,
        .read = my_read,
        .write = my_write,
        .statfs = my_statfs,
        .fsync = my_fsync,
        .opendir = my_opendir,
        .readdir = my_readdir,
        .init = my_init,
        .access = my_access,
        .chmod = my_chmod
};

/**
 * Muestra una guia de uso del programa
 * */
void usage() {
    printf("Uso: ./mount.qrfs [-f] directorio_qr/ contraseña punto_montaje/"); //TODO con el ./ ??
}

/**
 * main-Función principal del programa, en esta se obtienen los argumentos y se llama
 * a la funcion de incio de FUSE
 *
 * Errores:
 *  EINVAL - argumentos invalidos
 *
 * @param argc cantidad de argumentos
 * @param argv arreglo de string con la información de los argumentos
 * @return 0 si es correcto, error en otro caso
 */
int main(int argc, char *argv[]) {

    printf("Versión de FUSE: %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);

    if(argc < 4) { /**Verifica la cantidad de argumentos*/
        usage(); //ayuda de uso
        perror("Error en los argumentos.");
        return -EINVAL;
    }

    /**Se establecen los datos del sistma de archivos. tamaño, contraseña y folder con los qr*/
    int mount_file_size = NUMBER_OF_DATABLOCKS * MY_BLOCK_SIZE;
    char *mount_qrfolder_path = argv[argc - INIT_QR_ARG_POSITION];
    char *mount_password = argv[argc - PASSWD_ARG_POSITION];
    init_storage(mount_qrfolder_path, mount_password, mount_file_size);

    /**Se acomodan los argumentos para sacarlos a fuse, asi se pueden agregar parametros opcionales*/
    argv[argc - INIT_QR_ARG_POSITION] = argv[argc - MOUNT_ARG_POSITION]; //TODO revisar
    argv[argc - PASSWD_ARG_POSITION] = argv[argc - MOUNT_ARG_POSITION] = NULL;
    argc -= 2;

    printf("Llamando a fuse_main.\n");

    fuse_main(argc, argv, &my_oper, NULL); /**Se llama a la función principal de fuse*/

    printf("Saliendo de fuse_main");

    return EXIT_SUCCESS;
}