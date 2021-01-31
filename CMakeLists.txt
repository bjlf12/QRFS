cmake_minimum_required(VERSION 3.17)
project(QRFS C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(FUSE REQUIRED)

include_directories(src)
include_directories(/usr/include)

add_executable(mount.qrfs
        src/mount.qrfs.c
        src/my_storage.c
        src/my_inode.c
        src/my_super.c
        src/my_inode.h
        src/my_super.h)

target_link_libraries(mount.qrfs gd)
target_link_libraries(mount.qrfs qrencode)
target_link_libraries(mount.qrfs zbar)
target_link_libraries(mount.qrfs png)
target_link_libraries(mount.qrfs pthread)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

#target_link_libraries(mount.qrfs "/usr/include/fuse.h")
include_directories(${FUSE_INCLUDE_DIR})
target_link_libraries(mount.qrfs ${FUSE_LIBRARIES})

add_executable(fsck.qrfs
        src/fsck.qrfs.c
        src/my_storage.c
        src/my_inode.c
        src/my_super.c
        src/my_inode.h
        src/my_super.h)

target_link_libraries(fsck.qrfs gd)
target_link_libraries(fsck.qrfs qrencode)
target_link_libraries(fsck.qrfs zbar)
target_link_libraries(fsck.qrfs png)
target_link_libraries(fsck.qrfs pthread)

add_executable(mkfs.qrfs
        src/mkfs.qrfs.c
        src/my_storage.c
        src/my_inode.c
        src/my_super.c
        src/my_inode.h
        src/my_super.h)

target_link_libraries(mkfs.qrfs gd)
target_link_libraries(mkfs.qrfs qrencode)
target_link_libraries(mkfs.qrfs zbar)
target_link_libraries(mkfs.qrfs png)
target_link_libraries(mkfs.qrfs pthread)