//

//

#ifndef QRFS_MY_SUPER_H
#define QRFS_MY_SUPER_H

// QR v23
//#define FS_BLOCK_SIZE 256

#include <inttypes.h>
#include <stdlib.h>

// QR v10
#define MY_BLOCK_SIZE 1024
#define MY_MAGIC 0x53465251
#define SUPER_BLOCK_NUM 0
//0

#define NUMBER_OF_INODES 64 // Se puede calcular get free inode
#define NUMBER_OF_DATABLOCKS 20
//1024 200 20

#define SUPER_SIZE 24

//20 + 251*4 + 512*4

typedef struct my_super {

    uint32_t magic;
    uint32_t inode_map_sz;       /* in blocks */
    uint32_t block_map_sz;       /* in blocks */
    uint32_t inode_region_sz;    /* in blocks */
    uint32_t num_blocks;         /* total, including SB, bitmaps, inodes */
    uint32_t root_inode;        /* always inode 1 */

    //char pad[MY_BLOCK_SIZE - 6 * sizeof(int)];

} my_super; // 24

uint32_t jenkins_one_at_a_time_hash(char *key, size_t len);

void block_cipher(void **data, uint32_t shift);

void block_decipher(void **data, uint32_t shift);

#endif //QRFS_MY_SUPER_H
