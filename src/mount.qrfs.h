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

#endif //QRFS_MOUNT_QRFS_H
