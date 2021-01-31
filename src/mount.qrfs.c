/*
 *
 */

// TODO Parse QRs
// TODO Create


#include "mount.qrfs.h"
#include "my_super.h"
#include "my_inode.h"
#include "my_storage.h"

#include <dirent.h> // DIR
#include <errno.h> //perror and ENOMEM
#include <fcntl.h> //mode_t
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> //stat
#include <unistd.h> // operaciones con dir
#include <math.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/xattr.h>
#include <sys/types.h>

/** total size of direct blocks */
static int DIR_SIZE = MY_BLOCK_SIZE * NUM_DIRECT_ENT;
static int INDIR1_SIZE = (MY_BLOCK_SIZE / sizeof(uint32_t)) * MY_BLOCK_SIZE;
static int INDIR2_SIZE = (MY_BLOCK_SIZE / sizeof(uint32_t)) * (MY_BLOCK_SIZE / sizeof(uint32_t)) * MY_BLOCK_SIZE;

void *my_init(struct fuse_conn_info *conn) {
    /*log_msg("\nmy_init()\n");

    log_conn(conn);
    log_fuse_context(fuse_get_context());
    */
    return MY_DATA;
}

/*static void get_fullpath(char fpath[PATH_MAX], const char *path) { //Ya no??
    strcpy(fpath, MY_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // Direcciones absolutas largas son cortadas aquí.

    //log_msg("    get_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
    //        MY_DATA->rootdir, path, fpath);
}*/

static char *strmode(char *buf, int mode) {

    int mask = 0400;
    char *str = "rwxrwxrwx", *retval = buf;
    *buf++ = S_ISDIR(mode) ? 'd' : '-';
    for (mask = 0400; mask != 0; str++, mask = mask >> 1)
        *buf++ = (mask & mode) ? *str : '-';
    *buf++ = 0;
    return retval;
}

int my_getattr(const char *path, struct stat *stbuf){ //ia
    //fs_init(NULL); Creo que si lo hacemos en el main no es necesario.

    printf("Obteniendo los atributos del archivo %s con my_getattr.\n", path);

    if(strcmp(path, "/dir") == 0) {
        printf("Asiesmdkaslmdlskads\n");
    }

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    my_inode *inode = get_inode(inode_id);
    cpy_stat(inode, stbuf);
    free(inode); // TODO revisar

    char mode[16], time[26], *lasts;
    printf("%5lld %s %2d %4d %4d %8lld %s %s\n",
           stbuf->st_blocks, strmode(mode, stbuf->st_mode),
           stbuf->st_nlink, stbuf->st_uid, stbuf->st_gid, stbuf->st_size,
               strtok_r(ctime_r(&stbuf->st_mtime,time),"\n",&lasts), path); //TODO revisar que debería imprimir cada una

    printf("Finalizando de obtener los atributos del archivo.\n");
    return EXIT_SUCCESS;
}

int my_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

    printf("Creando el archivo %s con la función my_create.\n", path);

    //get current and parent inodes
    mode |= S_IFREG;
    if (!S_ISREG(mode) || strcmp(path, "/") == 0) return -EINVAL; // EXIT_FAILURE
    char *dup_path = strdup(path);
    char name[FILENAME_MAX];
    int inode_id = get_inode_id_from_path(dup_path);
    int parent_inode_id = get_inode_id_and_leaf_from_path(dup_path, name);
    if (inode_id >= 0) return -EEXIST; // EXIT_FAILURE si queremos que se sobrescriba TODO printf("El archivo ya existe, sobreescribiendo\n");
    if (parent_inode_id < 0) return parent_inode_id;
    //read parent info
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR; // EXIT_FAILURE

    my_dirent *entries;
    //memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));

    entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.\n");
        return -EIO;
    }

    //assign inode and directory and update
    int res = set_attributes_and_update(entries, name, mode, false);
    if (res < 0) return res;

    if(write_data(entries, parent_inode->direct[0]) < 0) {
        perror("Error al escribir una entrada de directorio en write_data.\n");
        return -EIO;
    } // TODO if

    printf("Terminando de crear un directorio.\n");
    return EXIT_SUCCESS;
}
    /*
    int ret_stat = EXIT_SUCCESS;
    int fd;
    char fpath[PATH_MAX];

    //log_msg("\nmy_open(path\"%s\", fi=0x%08x)\n",
    //        path, fi);
    get_fullpath(fpath, path);

    fd = open(fpath, fi->flags, mode);
    if(fd < 0) {
        perror("Error al utilizar mkfile.\n");
        ret_stat = -errno;
    }

    fi->fh = fd;

    return ret_stat;
}*/
/*
int my_mknod(const char *path, mode_t mode, dev_t dev) {
    int ret_stat;
    char fpath[PATH_MAX];

    get_fullpath(fpath, path);

    if (S_ISREG(mode)) {
        ret_stat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (ret_stat >= 0)
            ret_stat = close(ret_stat);
    } else if (S_ISFIFO(mode))
        ret_stat = mkfifo(fpath, mode);
    else
        ret_stat = mknod(fpath, mode, dev);

    if (ret_stat == -1)
        return -errno;

    return ret_stat;
}*/

int my_open(const char *path, struct fuse_file_info *fi) {

    printf("Abriendo el archivo %s con la función my_open.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    my_inode *inode = get_inode(inode_id);
    if (S_ISDIR(inode->mode)) return -EISDIR;
    fi->fh = (uint64_t) inode_id;
    free(inode); //TODO revisar
    free(dup_path);

    printf("Terminando de abrir el archivo.\n");
    return EXIT_SUCCESS;
};

size_t my_read_dir(int inode_id, char *buf, size_t length, size_t offset) {

    printf("Leyendo el directorio con el identificador %d con la función my_read_dir.\n", inode_id);

    my_inode *inode = get_inode(inode_id);
    size_t block_num = offset / MY_BLOCK_SIZE;
    size_t blk_offset = offset % MY_BLOCK_SIZE;
    size_t len_to_read = length;
    while (block_num < NUM_DIRECT_ENT && len_to_read > 0) {
        size_t cur_len_to_read = len_to_read > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_read;
        size_t temp = blk_offset + cur_len_to_read;

        if (!inode->direct[block_num]) {
            return length - len_to_read;
        }

        read_file_data(inode->direct[block_num], buf, temp, blk_offset);

        buf += temp;
        len_to_read -= temp;
        block_num++;
        blk_offset = 0;
    }

    printf("Finalizando de leer los datos de un directorio.\n");
    return length - len_to_read;
}

/**
 * read - read data from an open file.
 *
 * Should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 *
 * Errors:
 *   -ENOENT  - file does not exist
 *   -ENOTDIR - component of path not a directory
 *   -EIO     - error reading block
 *
 * @param path the path to the file
 * @param buf the read buffer
 * @param len the number of bytes to read
 * @param offset to start reading at
 * @param fi fuse file info
 */
int my_read(const char *path, char *buf, size_t length, off_t offset, struct fuse_file_info *fi) {

    printf("Leyendo el archivo %s con la función my_read.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    my_inode *inode = get_inode(inode_id);
    if (S_ISDIR(inode->mode)) return -EISDIR; // EXIT_FAILURE
    if (offset >= inode->size) return 0;

    //if len go beyond inode size, read to EOF
    if (offset + length > inode->size) {
        length = (size_t) inode->size - offset;
    }

    //len need to read
    size_t len_to_read = length;

    //read direct blocks
    if (len_to_read > 0 && offset < DIR_SIZE) {
        //len finished read
        size_t temp = my_read_dir(inode_id, buf, len_to_read, (size_t) offset);
        len_to_read -= temp;
        offset += temp;
        buf += temp;
    }

    //read indirect 1 blocks
    if (len_to_read > 0 && offset < DIR_SIZE + INDIR1_SIZE) {
        //len finished read
        size_t temp = read_indir1(inode->indir_1, buf, len_to_read, (size_t) offset - DIR_SIZE);
        len_to_read -= temp;
        offset += temp;
        buf += temp;
    }

    //read indirect 2 blocks
    if (len_to_read > 0 && offset < DIR_SIZE + INDIR1_SIZE + INDIR2_SIZE) {
        //len finshed read
        size_t temp = read_indir2(inode->indir_2, buf, len_to_read, (size_t) offset - DIR_SIZE - INDIR1_SIZE, INDIR1_SIZE);
        len_to_read -= temp;
        offset += temp;
        buf += temp;
    }

    free(inode);
    free(dup_path); //TODO revisar.

    printf("Finalizando de leer los datos de un archivo.\n");

    return (int) (length - len_to_read);
};

size_t my_write_dir(my_inode **to_write_inode, const char *buf, size_t len, size_t offset) {

    printf("Escribiendo información de un archivo con la función my_write_dir.\n");

    my_inode *inode = *to_write_inode;

    size_t blk_num = offset / MY_BLOCK_SIZE;
    size_t blk_offset = offset % MY_BLOCK_SIZE;
    size_t len_to_write = len;
    while (blk_num < NUM_DIRECT_ENT && len_to_write > 0) {
        size_t cur_len_to_write = len_to_write > MY_BLOCK_SIZE ? (size_t) MY_BLOCK_SIZE - blk_offset : len_to_write;
        size_t temp = blk_offset + cur_len_to_write;

        if (!inode->direct[blk_num]) {
            int freeb = get_free_block();
            if (freeb < 0) return len - len_to_write;
            inode->direct[blk_num] = freeb; // No hace esto?
            //add_inode(inode_id, inode);
        }

        write_file_data(inode->direct[blk_num], buf, temp, blk_offset);

        buf += temp;
        len_to_write -= temp;
        blk_num++;
        blk_offset = 0;
    }

    //free(inode);

    printf("Terminando de escribir la información de un directorio.\n");
    return len - len_to_write;
}

/**
 *  write - write data to a file
 *
 * It should return exactly the number of bytes requested, except on
 * error.
 *
 * Errors:
 *   -ENOENT  - file does not exist
 *   -ENOTDIR - component of path not a directory
 *   -EINVAL  - if 'offset' is greater than current file length.
 *  			(POSIX semantics support the creation of files with
 *  			"holes" in them, but we don't)
 *
 * @param path the file path
 * @param buf the buffer to write
 * @param len the number of bytes to write
 * @param offset the offset to starting writing at
 * @param fi the Fuse file info for writing
 */
int my_write(const char *path, const char *buf, size_t length, off_t offset,
             struct fuse_file_info *fi) {

    printf("Escribiendo información del archivo %s con la función my_write.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    my_inode *inode = get_inode(inode_id);
    if (S_ISDIR(inode->mode)) return -EISDIR;
    if (offset > inode->size) return 0;

    //len need to write
    size_t len_to_write = length;

    //write direct blocks
    if (len_to_write > 0 && offset < DIR_SIZE) {
        //len finished write
        size_t temp = my_write_dir(&inode, buf, len_to_write, (size_t) offset);
        len_to_write -= temp;
        offset += temp;
        buf += temp;
    }

    //write indirect 1 blocks
    if (len_to_write > 0 && offset < DIR_SIZE + INDIR1_SIZE) {
        //need to allocate indir_1
        if (!inode->indir_1) {
            int freeb = get_free_block();
            if (freeb < 0) return length - len_to_write;
            inode->indir_1 = freeb;
            add_inode(inode_id, inode);
        }
        size_t temp = write_indir1(inode->indir_1, buf, len_to_write, (size_t) offset - DIR_SIZE);
        len_to_write -= temp;
        offset += temp;
        buf += temp;
    }

    //write indirect 2 blocks
    if (len_to_write > 0 && offset < DIR_SIZE + INDIR1_SIZE + INDIR2_SIZE) {
        //need to allocate indir_2
        if (!inode->indir_2) {
            int freeb = get_free_block();
            if (freeb < 0) return length - len_to_write;
            inode->indir_2 = freeb;
            add_inode(inode_id, inode);
        }
        //len finshed write
        size_t temp = write_indir2(inode->indir_2, buf, len_to_write, (size_t) offset - DIR_SIZE - INDIR1_SIZE, INDIR1_SIZE);
        len_to_write -= temp;
        offset += len_to_write;
    }

    if (offset > inode->size) inode->size = offset;

    //update inode and blk
    add_inode(inode_id, inode); //////////// Estooo
    //update_blk(); //TODO revisar si es necesario

    free(inode);
    free(dup_path);

    printf("Finalizando de escribir la información de un archivo.\n");
    return (int) (length - len_to_write);
};

/**
 * rename - rename a file or directory.
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 *
 * Errors:
 *   -ENOENT   - source file or directory does not exist
 *   -ENOTDIR  - component of source or target path not a directory
 *   -EEXIST   - destination already exists
 *   -EINVAL   - source and destination not in the same directory
 *
 * @param src_path the source path
 * @param dst_path the destination path.
 * @return 0 if successful, or error value
 */
int my_rename(const char *src_path, const char *dst_path) {

    printf("Renombrando el archivo %s a %s con la función my_rename.\n", src_path, dst_path);

    //deep copy both path
    char *dup_src_path = strdup(src_path);
    char *dup_dst_path = strdup(dst_path);
    //get inodes
    int src_inode_id = get_inode_id_from_path(dup_src_path);
    int dst_inode_id = get_inode_id_from_path(dup_dst_path);
    //if src inode does not exist return error
    if (src_inode_id < 0) return src_inode_id;
    //if dst already exist return error
    if (dst_inode_id >= 0) return -EEXIST;

    //get parent directory inode
    char src_name[FILENAME_MAX];
    char dst_name[FILENAME_MAX];
    int src_parent_inode_id = get_inode_id_and_leaf_from_path(dup_src_path, src_name);
    int dst_parent_inode_id = get_inode_id_and_leaf_from_path(dup_dst_path, dst_name);
    //src and dst should be in the same directory (same parent)
    if (src_parent_inode_id != dst_parent_inode_id) return -EINVAL;
    int parent_inode_id = src_parent_inode_id;
    if (parent_inode_id < 0) return parent_inode_id;

    //read parent dir inode
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR;

    my_dirent *entries;
    //memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));

    entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.\n");
        return -EIO;
    }

    //make change to buff
    for (int i = 0; i < DIR_ENTS_PER_BLK; ++i) {
        if (entries[i].valid && strcmp(entries[i].filename, src_name) == 0) {
            memset(entries[i].filename, 0, sizeof(entries[i].filename));
            strcpy(entries[i].filename, dst_name);
        }
    }

    //write buff to inode
    if(write_data(entries, parent_inode->direct[0]) < 0) {
        perror("Error al escribir una entrada de directorio en write_data.\n");
        return -EIO;
    } //TODO if

    printf("Terminando de cambiar el nombre de un archivo.\n");
    return EXIT_SUCCESS;
}

/**
 *  mkdir - create a directory with the given mode. Behavior
 *  undefined when mode bits other than the low 9 bits are used.
 *
 * Errors
 *   -ENOTDIR  - component of path not a directory
 *   -EEXIST   - directory already exists
 *   -ENOSPC   - free inode not available
 *   -ENOSPC   - results in >32 entries in directory
 *
 * @param path path to file
 * @param mode the mode for the new directory
 * @return 0 if successful, or error value
 */
int my_mkdir(const char *path, mode_t mode) {

    printf("Creando el directorio %s con la función my_mkdir.\n", path);

    //get current and parent inodes
    mode |= S_IFDIR;
    if (!S_ISDIR(mode) || strcmp(path, "/") == 0) return -EINVAL;
    char *dup_path = strdup(path);
    char name[FILENAME_MAX];
    int inode_id = get_inode_id_from_path(dup_path);
    int parent_inode_id = get_inode_id_and_leaf_from_path(dup_path, name);
    if (inode_id >= 0) return -EEXIST;
    if (parent_inode_id < 0) return parent_inode_id;
    //read parent info
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR;

    my_dirent *entries;
    //memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));

    entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.\n");
        return -EIO; //No estaba lo agregue
    }

    //assign inode and directory and update
    int res = set_attributes_and_update(entries, name, mode, true);
    if (res < 0) return res;

    if(write_data(entries, parent_inode->direct[0]) < 0) {
        perror("Error al escribir una entrada de directorio en write_data.\n");
        return -EIO;
    } // TODO if

    printf("Terminando de crear un directorio.\n");
    return EXIT_SUCCESS;
    /*
    int ret_stat;
    char fpath[PATH_MAX];

    printf("mkdir(%s) with mode <%d>\n", path, mode);

    get_fullpath(fpath, path);

    ret_stat = mkdir(fpath, 0777);
    if(ret_stat < 0) {
        perror("Error al utilizar mkdir.\n");
    }
    return ret_stat;*/
};

/**
 * readdir - get directory contents.
 *
 * For each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the directory path
 * @param ptr  filler buf pointer
 * @param filler filler function to call for each entry
 * @param offset the file offset -- unused
 * @param fi the fuse file information
 */
int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi) {

    printf("Leyendo la información del directorio %s con la función my_readdir.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    my_inode *inode = get_inode(inode_id);
    if (!S_ISDIR(inode->mode)) return -ENOTDIR; // EXIT_FAILURE
    my_dirent *entries;
    //memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));
    struct stat sb;

    entries = read_data(inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.\n");
        return -EIO; //todo lo agregue
    }

    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (entries[i].valid) {
            cpy_stat(get_inode(entries[i].inode), &sb);
            filler(buf, entries[i].filename, &sb, 0);
        }
    }

    free(inode);
    free(entries); //TODO free a los entries

    printf("Terminando de leer la informacion de un directorio.\n");
    return EXIT_SUCCESS;

    /**int ret_stat = EXIT_SUCCESS;
    DIR *dp;
    struct dirent *de;

    dp = (DIR *) (uintptr_t) fi->fh;
    de = readdir(dp);

    if (!de) {
        perror("Error al utilizar readdir.\n"); // Arreglar a perror
        ret_stat = -errno;
        return ret_stat;
    }

    do {
        //log_msg("calling filler with name %s\n", de->d_name);
        if (filler(buf, de->d_name, NULL, 0) != 0) {
            perror("Error al utilizar filler: buffer lleno.\n");
            return -ENOMEM;
        }
    } while ((de = readdir(dp)) != NULL);

    return ret_stat;*/
};

/**
 * open - open file directory.
 *
 * You can save information about the open directory in
 * fi->fh. If you allocate memory, free it in fs_releasedir.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param fi fuse file system information
 */
int my_opendir(const char *path, struct fuse_file_info *fi) {

    printf("Abriendo el directorio %s con la función my_opendir.\n", path);

    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    if (!S_ISDIR(get_inode(inode_id)->mode)) return -ENOTDIR; // EXIT_FAILURE
    fi->fh = (uint64_t) inode_id;

    printf("Terminando de abrir un directorio.\n");
    return EXIT_SUCCESS;

    /*DIR *dp;
    int ret_stat = EXIT_SUCCESS;
    char fpath[PATH_MAX];

    get_fullpath(fpath, path);

    dp = opendir(fpath);    // Retorna un puntero.
    if(dp == NULL) {
        perror("Error al utilizar opendir.");
        ret_stat = -errno;
    }

    fi->fh = (intptr_t) dp;

    return ret_stat;*/
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
int my_rmdir(const char *path) {

    printf("Eliminando el directorio %s con la función my_rmdir.\n", path);

    //can not remove root
    if (strcmp(path, "/") == 0) return -EINVAL;

    //get inodes and check
    char *dup_path = strdup(path);
    char name[FILENAME_MAX]; //TODO revisar
    int inode_id = get_inode_id_from_path(dup_path);
    int parent_inode_id = get_inode_id_and_leaf_from_path(dup_path, name);
    my_inode *inode = get_inode(inode_id);
    my_inode *parent_inode = get_inode(parent_inode_id);
    if (inode_id < 0 || parent_inode_id < 0) return -ENOENT;
    if (!S_ISDIR(inode->mode)) return -ENOTDIR;
    if (!S_ISDIR(parent_inode->mode)) return -ENOTDIR;

    //check if dir if empty
    my_dirent *entries;
    //memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));

    entries = read_data(inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.\n");
        return -EIO; // lo agregue
    }

    int res = is_empty_dir(entries);
    if (res == 0) return -ENOTEMPTY;

    //remove entry from parent dir
    memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));
    // write_data(entries, inode->direct[0]); TODO revisar si es necesario

    entries = read_data(parent_inode->direct[0]);
    if(entries == NULL) {
        perror("Error al leer las entradas de directorio en read_data.\n");
        return -EIO; // lo agregue
    }

    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (entries[i].valid && strcmp(entries[i].filename, name) == 0) {
            memset(&entries[i], 0, sizeof(my_dirent)); //TODO revisar el ampersand
        }
    }
    if(write_data(entries, parent_inode->direct[0]) < 0) {
        perror("Error al escribir una entrada de directorio en write_data.\n");
        return -EIO; // lo agregue
    } //TODO if

    //return blk and clear inode
    clear_block_bitmap(inode->direct[0]);
    clear_inode_bitmap(inode_id);
    memset(inode, 0, sizeof(my_inode));

    //update
    add_inode(inode_id, inode); //cambiar de add a update
    //update_blk(); TODO revisar

    free(dup_path); //TODO free dup
    free(inode);
    free(parent_inode);
    free(entries);

    printf("Terminando de eliminar un directorio.\n");
    return EXIT_SUCCESS;

    /*int ret_stat;
    char fpath[PATH_MAX];

    get_fullpath(fpath, path);
    ret_stat = rmdir(fpath);
    if(ret_stat < 0) {
        perror("Error al utilizar rmdir.\n");
    }
    return ret_stat;*/
}

/**
 * statfs - get file system statistics.
 * See 'man 2 statfs' for description of 'struct statvfs'.
 *
 * Errors
 *   none -  Needs to work
 *
 * @param path the path to the file
 * @param st the statvfs struct
 */
int my_statfs(const char *path, struct statvfs *statv) {

    /* needs to return the following fields (set others to zero):
     *   f_bsize = BLOCK_SIZE
     *   f_blocks = total image - metadata
     *   f_bfree = f_blocks - blocks used
     *   f_bavail = f_bfree
     *   f_namelen = <whatever your max namelength is>
     *
     * this should work fine, but you may want to add code to
     * calculate the correct values later.
     */

    printf("Obteniendo el estado del archivo %s con la función my_statfs.\n", path);

    my_super *super_block = read_data(SUPER_BLOCK_NUM);
    if (super_block == NULL) {
        perror("Error al leer el superbloque en read_data.\n");
        return -EIO; // TODO o exit
    }
    int root_inode_id = super_block->root_inode;
    int inode_data_base = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz + super_block->block_map_sz;

    //clear original stats
    memset(statv, 0, sizeof(*statv));
    statv->f_bsize = MY_BLOCK_SIZE;
    statv->f_blocks = (fsblkcnt_t) (NUMBER_OF_DATABLOCKS - root_inode_id - inode_data_base);
    statv->f_bfree = (fsblkcnt_t) get_num_free_block();
    statv->f_bavail = statv->f_bfree;
    statv->f_namemax = FILENAME_MAX - 1;

    printf("Terminando de obtener la informacion de un archivo.\n");
    return EXIT_SUCCESS;

    /*int ret_stat = EXIT_SUCCESS;
    char fpath[PATH_MAX];

    get_fullpath(fpath, path);
    ret_stat = statvfs(fpath, statv);
    if(ret_stat < 0) {
        perror("Error al utilizar rmdir.\n");
    }

    return ret_stat;*/
}

int my_fsync(const char *path, int datasync, struct fuse_file_info *fi) {

    printf("Sincronizando los datos del archivo %s.\n", path);

    char *dup_path = strdup(path);
    char name[FILENAME_MAX]; //TODO revisar
    int inode_id = get_inode_id_from_path(dup_path);
    if(inode_id<0) {
        perror("Error al buscar un archivo en el sistema de archivos.\n");
        return -ENOENT;
    }

    int ret_stat;
    fi->fh = inode_id;
    if(datasync) {
        ret_stat = fdatasync(fi->fh);
        if(ret_stat < 0) {
            perror("Error al utilizar la función fdatasync.\n");
            return ret_stat;
        }
    }
    else {
        ret_stat = fsync(fi->fh);
        if(ret_stat < 0) {
            perror("Error al utilizar la función fsync.\n");
            return ret_stat;
        }
    }

    printf("Terminando de sincronizar los datos de un archivo.\n");
    return EXIT_SUCCESS;
    /*
     * int ret_stat = EXIT_SUCCESS;
     * //log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	  //  path, datasync, fi);
    //log_fi(fi);

    if(datasync) { // Me parece que es lo mismo
                // https://www.informit.com/articles/article.aspx?p=23618&seqNum=5#:~:text=c)%20Write%20and%20Sync%20a%20Journal%20Entry&text=Another%20system%20call%2C%20fdatasync%20does,file's%20data%20will%20be%20written.
        ret_stat = fdatasync(fi->fh);
        if (ret_stat < 0) {
            perror("Error al utilizar fdatasync.\n");
        }
    }
    else {
        ret_stat = fsync(fi->fh);
        if (ret_stat < 0) {
            perror("Error al utilizar fsync.\n");
        }
    }

    return ret_stat;*/
}

int my_access(const char *path, int mask) { //TODO arreglar

    printf("Utilizando la función my_access con el archivo %s, con la máscara: %d.\n", path, mask);

    char *dup_path = strdup(path);
    char name[FILENAME_MAX]; //TODO revisar
    int inode_id = get_inode_id_from_path(dup_path);

    if(inode_id < 0) {
        return -EACCES;
    }
    if(mask == F_OK) {
        return EXIT_SUCCESS;
    }
    my_inode *inode = get_inode(inode_id);

    if(inode->mode & (mask*0100)) { // TODO nos deja leer aunque no hayna permisos
        printf("Terminando la función my_access.\n"); //TODO cambiar
        return EXIT_SUCCESS;
    } // and o or

    printf("Terminando la función my_access.\n"); //TODO cambiar
    return -EACCES;

    /*int ret_stat = EXIT_SUCCESS;
    char fpath[PATH_MAX];

    //log_msg("\nmy_access(path=\"%s\", mask=0%o)\n",
    //        path, mask);
    get_fullpath(fpath, path);
    ret_stat = access(fpath, mask);

    if (ret_stat < 0) {
        perror("Error al utilizar access.\n");
        ret_stat = -errno;
    }

    return ret_stat;*/
}

//todo F, borrarla
int fs_mknod(const char *path, mode_t mode, dev_t dev) {

    printf("Creando un nodo para el path %s con la funcion fs_mknod.\n", path);

    //get current and parent inodes
    mode |= S_IFREG;
    if (!S_ISREG(mode) || strcmp(path, "/") == 0) return -EINVAL;
    char *_path = strdup(path);
    char name[FILENAME_MAX];
    int inode_idx = get_inode_id_from_path(_path);
    int parent_inode_idx = get_inode_id_and_leaf_from_path(_path, name);
    if (inode_idx >= 0) return -EEXIST;
    if (parent_inode_idx < 0) return parent_inode_idx;
    //read parent info
    my_inode *parent_inode = get_inode(parent_inode_idx);
    if (!(S_ISDIR(parent_inode->mode))) return -ENOTDIR;

    my_dirent *entries;//[DIR_ENTS_PER_BLK];
    //memset(entries, 0, DIR_ENTS_PER_BLK * sizeof(my_dirent));
    //if (disk->ops->read(disk, parent_inode->direct[0], 1, entries) < 0)
    //    exit(1);

    entries = read_data(parent_inode->direct[0]);

    //assign inode and directory and update
    int res = set_attributes_and_update(entries, name, mode, false);
    if (res < 0) return res;

    //write entries buffer into disk
    //if (disk->ops->write(disk, parent_inode->direct[0], 1, entries) < 0)
    //    exit(1);

    write_data(entries, parent_inode->direct[0]);

    printf("Terminando de crear un nodo.\n");
    return EXIT_SUCCESS;
}

//todo cambiar nombre
int fs_chmod(const char *path, mode_t mode) {

    printf("Haciendo un chmod.\n");
    //printf("resultado mkdir: %d\n", my_mkdir("/dir1", 0777));
    char* _path = strdup(path);
    int inode_idx = get_inode_id_from_path(_path);
    if (inode_idx < 0) return inode_idx;
    my_inode *inode = get_inode(inode_idx);
    //protect system from other modes
    mode |= S_ISDIR(inode->mode) ? S_IFDIR : S_IFREG;
    //change through reference
    inode->mode = mode;
    add_inode(inode_idx, inode);

    printf("Terminando con el chmod\n");
    return EXIT_SUCCESS;
}
/*
void fs_truncate_dir(uint32_t *de) {
    for (int i = 0; i < DIR_ENTS_PER_BLK; i++) {
        if (de[i]) return_blk(de[i]);
        de[i] = 0;
    }
}

void fs_truncate_indir1(int blk_num) {
    my_dirent *entries;//[PTRS_PER_BLK];
    //memset(entries, 0, PTRS_PER_BLK * sizeof(uint32_t));
    if (disk->ops->read(disk, blk_num, 1, entries) < 0)
        exit(1);
    //clear each blk and wipe from blk_map
    for (int i = 0; i < PTRS_PER_BLK; i++) {
        if (entries[i]) return_blk(entries[i]);
        entries[i] = 0;
    }
}

void fs_truncate_indir2(int blk_num) {
    uint32_t entries[PTRS_PER_BLK];
    memset(entries, 0, PTRS_PER_BLK * sizeof(uint32_t));
    if (disk->ops->read(disk, blk_num, 1, entries) < 0)
        exit(1);
    //clear each double link
    for (int i = 0; i < PTRS_PER_BLK; i++) {
        if (entries[i]) fs_truncate_indir1(entries[i]);
        entries[i] = 0;
    }
}

int fs_truncate(const char *path, off_t len)
{
    //cheat
    if (len != 0) return -EINVAL;

    //get inode
    char *dup_path = strdup(path);
    int inode_id = get_inode_id_from_path(dup_path);
    if (inode_id < 0) return inode_id;
    my_inode *inode = get_inode(inode_id);
    //if (S_ISDIR(inode->mode)) return -EISDIR;

    //clear direct
    fs_truncate_dir(inode->direct);

    //clear indirect1
    if (inode->indir_1) {
        fs_truncate_indir1(inode->indir_1);
        return_blk(inode->indir_1);
    }
    inode->indir_1 = 0;

    //clear indirect2
    if (inode->indir_2) {
        fs_truncate_indir2(inode->indir_2);
        return_blk(inode->indir_2);
    }
    inode->indir_2 = 0;

    inode->size = 0;

    //update at the end for efficiency
    add_inode(inode_id, inode);
    //update_blk();

    return EXIT_SUCCESS;
}*/



struct fuse_operations my_oper = {

        .getattr = my_getattr,
        .mkdir = my_mkdir,
        .rmdir = my_rmdir,
        .rename = my_rename,
        .create = my_create,
        //.mknod = fs_mknod,//my_mknod,
        .open = my_open,
        .read = my_read,
        .write = my_write,
        .statfs = my_statfs,
        .fsync = my_fsync,
        .opendir = my_opendir,
        .readdir = my_readdir,
        .init = my_init,
        .access = my_access,
        .chmod = fs_chmod
};

void usage() {
    printf("Uso: ./mount.qrfs directorio_qr/ punto_montaje/");
}

int main(int argc, char *argv[]) {

    //Sacar esto
    //root_inode_id = 1;

    int fuse_stat;
    my_state *my_currentstate;
    printf("Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);

    if(argc < 4) {
        perror("Usage: mount.qrfs [opciones] qrfolder/ qrinitial mountpoint/ password\n");
        return -EINVAL;
    }

    int mount_file_size = NUMBER_OF_DATABLOCKS * MY_BLOCK_SIZE;
    char *mount_qrfolder_path = argv[2];
    char *mount_password = argv[5];
    init_storage(mount_qrfolder_path, mount_password, mount_file_size);

    /*uint32_t key = jenkins_one_at_a_time_hash(mount_password, strlen(mount_password));
    my_super *super_block = read_data(SUPER_BLOCK_NUM);

    int inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    int block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    void *inode_map = read_data(inode_map_position);
    void *block_map = read_data(block_map_position);

    block_decipher((void **)&super_block, key);
    block_decipher((void **)&inode_map, key);
    block_decipher((void **)&block_map, key);

    write_data(super_block, SUPER_BLOCK_NUM);
    write_data(inode_map, inode_map_position);
    write_data(block_map, block_map_position);

    free(super_block);
    free(inode_map);
    free(block_map);*/

    /*int file = open("nombrealgo", O_WRONLY|O_CREAT|O_TRUNC, 0777);
    char *data=malloc(1024*10);
    void *ptr = (void *)data;
    for(int i=1; i<10; ++i) {

        memcpy(ptr, read_data(i), 1024);
        ptr += 1024;
    }
    write(file, data, 1024*10);
    close(file);*/

    /*char *data = malloc(1024*1024);
    memset(data, 0, 1024*1024);
    open("prueba.img", data, 1024*1024);
    void *ptr = (void *)data;

    for(int i=1; i< 200; ++i) {
        write_data(ptr, i);
        ptr += 1024;
    }*/

    if ((getuid() == 0) || (geteuid() == 0)) {
        fprintf(stderr, "Running mount.qrfs as root opens unnacceptable security holes.\n");
        return -EACCES;
    }

    // TODO Parse QRs INIT QR

    my_currentstate = malloc(sizeof(my_state));
    if(my_currentstate == NULL) {
        perror("Error al utilizar malloc.\n");
        return -ENOMEM;
    }
    my_currentstate->rootdir = realpath(argv[argc - ROOT_ARG_POSITION], NULL);
    //my_data->logfile = log_open();

    argv[argc - INIT_QR_ARG_POSITION] = argv[argc - MOUNT_ARG_POSITION];
    argv[argc - ROOT_ARG_POSITION] = argv[argc - MOUNT_ARG_POSITION] = NULL;
    argv[5] = NULL;
    argc -= 3;

    printf("Llamando a fuse_main.\n");
    fuse_stat = fuse_main(argc, argv, &my_oper, NULL);

    printf("Saliendo de fuse_main");

    /*super_block = read_data(SUPER_BLOCK_NUM);

    inode_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE);
    block_map_position = SUPER_BLOCK_NUM + ceil((double)SUPER_SIZE / MY_BLOCK_SIZE) + super_block->inode_map_sz;

    inode_map = read_data(inode_map_position);
    block_map = read_data(block_map_position);

    block_decipher((void **)&super_block, key);
    block_decipher((void **)&inode_map, key);
    block_decipher((void **)&block_map, key);

    write_data(super_block, SUPER_BLOCK_NUM);
    write_data(inode_map, inode_map_position);
    write_data(block_map, block_map_position);

    free(super_block);
    free(inode_map);
    free(block_map);*/

    return EXIT_SUCCESS;
}