# Sistemas Operativos - Segundo Proyecto: QRFS

Segundo Proyecto del curso de Sistemas Operativos (código 6600) en la carrera de Ingeniería en Computación.

## Objetivo

Realizar una re-implementación de algunas de las funciones de un filesystem en el espacio de usuario del Sistema Operativo GNU/Linux. El proyecto debe ser realizado utilizando el lenguaje de programación C y las bibliotecas FUSE. Además, el filesystem debe poder imprimirse en hojas de papel.

## Requerimientos

### QRFS

Se tratará de una implementación de las siguientes funciones POSIX:

* getattr
* create
* open
* read
* write
* rename
* mkdir
* readdir
* opendir
* rmdir
* statfs
* fsync
* access

### mkfs.qrfs

* Se encarga de crear un nuevo sistema de archivos QRFS.
* El usuario puede definir el nombre del archivo QR que será el inicio de su partición física.
* Se le solicitará al usuario introducir un nuevo “passphrase”. Este se almacenará, y se utilizará como firma para encontrar el inicio del FS.
* Toda la información relevante a la organización del FS (ej. Superblock, o el manejo de espacio libre) deberá ser cifrada a partir de la contraseña del usuario. El resto del FS permanecerá sin cifrar.
* El sistema de archivos deberá de utilizar i-nodos como estructura de indexación de bloques.

### fsck.qrfs

Realiza un chequeo completo de consistencia del QRFS. Con el fin de comprobar la funcionalidad de la contraseña del usuario, la integridad de los archivos almacenados y el correcto funcionamiento de la información de organización del FS.

### mount.qrfs

Realiza el trabajo de montar el FS en el sistema operativo. El QRFS será encargado de encontrar la firma en los archivos QR para definir cuál corresponde a los bloques iniciales del FS.

## Funcionamiento del programa

### Compilación

Para la compilación del proyecto se hace uso del archivo "CMakeList.txt", mediante los comandos:

```console
mkdir QRFS_build
cd QRFS_build
cmake ../
cmake --build .
```

### Ejecución del código mkfs.qrfs

El primer argumento es el directorio donde se desean crear los distintos archivos QR que servirán de almacenamiento de los datos, mientras que el segundo es la contraseña para cifrar la información sobre la organización del FS.

```console
./mkfs.qrfs directorio_qr/ constraseña
```

Una vez ejecutado este comando se encontrará un nuevo directorio con el nombre especificado, el cual contendrá los archivos QR.

![qrs](https://user-images.githubusercontent.com/56206208/129429353-f3c58e39-e6fe-47a9-b419-c78fb4e58046.png)

### Ejecución del código fsck.qrfs

Se recibe el directorio que almacena a los archivos QR y la contraseña utilizada para la creación del FS. 

```console
./fsck.qrfs directorio_qr/ constraseña
```

Este mostrará mensajes de error si se encuentra alguna inconsistencia en el QRFS. En caso contrario se indicará el correcto funcionamiento del mismo.

![fsck](https://user-images.githubusercontent.com/56206208/129429317-a6a8b181-b019-412a-86d6-71b282e4dab8.png)

### Ejecución del código mount.qrfs

El parámetro opcional es utilizado para indicar que se desea recibir mensajes sobre el funcionamiento del programa. Por su parte, se debe indicar el nombre del directorio que contiene a los archivos QR, la contraseña utilizada en la creación del FS y la carpeta que será utilizada como punto de montaje.

```console
./mount.qrfs [-f] directorio_qr/ constraseña punto_montaje/
```

Se deberá encontrar el nuevo sistema de archivos montado en el sistema operativo. Se podrán realizar las funciones solicitadas tal cual como funcionan en los sistemas Linux.

![qrfs](https://user-images.githubusercontent.com/56206208/129429308-1cef74d3-7e6d-43e2-bd50-1b34cd092025.png) 

## Estado

El programa funciona correctamente, y fueron implementadas todas las funcionalidades indicadas.

## Realizado por:

* Brandon Ledezma Fernández

* Walter Morales Vásquez
