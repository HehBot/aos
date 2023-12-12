#ifndef BUF_H
#define BUF_H

#include <fs/fs.h>

typedef enum __attribute__((packed)) diskstate {
    SECTOR_INVALID = 0,
    SECTOR_DIRTY,
    SECTOR_VALID
} diskstate_t;

typedef struct block {
    diskstate_t* state; // length (block_size / SECTOR_SIZE)
    size_t dev;
    size_t block;
    struct block* next;
    struct block* prev;
    uint8_t* data; // block_size bytes
} block_t;

#endif // BUF_H
