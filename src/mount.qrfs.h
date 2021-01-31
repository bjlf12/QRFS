//
// Created by estudiante on 13/1/21.
//

#ifndef QRFS_MOUNT_QRFS_H
#define QRFS_MOUNT_QRFS_H

//#undef HAVE_MKDIR

#define FUSE_USE_VERSION 26
#define _XOPEN_SOURCE 500
// This will give you some extra functionality that exists on most recent UNIX/BSD/Linux systems, but probably doesn't
// exist on other systems such as Windows.
// https://stackoverflow.com/questions/5378778/what-does-d-xopen-source-do-mean


#define PATH_MAX 4096

#define INIT_QR_ARG_POSITION 4
#define ROOT_ARG_POSITION 3
#define MOUNT_ARG_POSITION 2

typedef struct my_state {
    //FILE *logfile;
    char *rootdir;
}my_state;

#define MY_DATA ((struct my_state *) fuse_get_context()->private_data)
/*
void *my_init(struct fuse_conn_info *conn);

static char *strmode(char *buf, int mode);

int my_getattr(const char *path, struct stat *stbuf);

int my_create(const char *path, mode_t mode, struct fuse_file_info *fi);

int my_open(const char *path, struct fuse_file_info *fi);

size_t my_read_dir(int inode_id, char *buf, size_t length, size_t offset);

int my_read(const char *path, char *buf, size_t length, off_t offset, struct fuse_file_info *fi);

size_t my_write_dir(my_inode **to_write_inode, const char *buf, size_t len, size_t offset);

int my_write(const char *path, const char *buf, size_t length, off_t offset,
             struct fuse_file_info *fi);

int my_rename(const char *src_path, const char *dst_path);

int my_mkdir(const char *path, mode_t mode);

int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi);

int my_opendir(const char *path, struct fuse_file_info *fi);

int my_rmdir(const char *path);

int my_statfs(const char *path, struct statvfs *statv);

int my_fsync(const char *path, int datasync, struct fuse_file_info *fi);

int my_access(const char *path, int mask);

int my_chmod(const char *path, mode_t mode);


*/

#endif //QRFS_MOUNT_QRFS_H
