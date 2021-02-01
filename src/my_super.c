//
// Created by estudiante on 18/1/21.
//

#include <stdlib.h>
#include "my_super.h"

// https://en.wikipedia.org/wiki/Jenkins_hash_function
uint32_t jenkins_one_at_a_time_hash(char *key, size_t len) {

    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i) {

        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return abs(hash);
}

void block_cipher(void **data, uint32_t key) {

    char *data_to_cipher = *data;

    for(int i=0; i< MY_BLOCK_SIZE; ++i) {
        data_to_cipher[i] = data_to_cipher[i] ^ key;
    }
}

void block_decipher(void **data, uint32_t key) {

    char *data_to_decipher = *data;

    for(int i=0; i< MY_BLOCK_SIZE; ++i) {
        data_to_decipher[i] = data_to_decipher[i] ^ key;
    }
}