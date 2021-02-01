/**
 * Autores:
 *   Brandon Ledezma Fernández
 *   Walter Morales Vásquez
 * Módulo encargado de montar el sistema de archivos en el equipo, se encuentran las funciones del sistema de
 * archivos las cuales funcionan bajo las interfaces que provee FUSE.
 */


#ifndef QRFS_MOUNT_QRFS_H
#define QRFS_MOUNT_QRFS_H

//#undef HAVE_MKDIR

#define FUSE_USE_VERSION 26 //Versión de FUSE

// This will give you some extra functionality that exists on most recent UNIX/BSD/Linux systems, but probably doesn't
// exist on other systems such as Windows.
// https://stackoverflow.com/questions/5378778/what-does-d-xopen-source-do-mean
#define _XOPEN_SOURCE 500

#define INIT_QR_ARG_POSITION 3 /**Posicion del directorio con los QR*/
#define PASSWD_ARG_POSITION 2 /**Posición de la contraseña en los argumentos*/
#define MOUNT_ARG_POSITION 1 /**Posición del punto de montaje*/


#endif //QRFS_MOUNT_QRFS_H
