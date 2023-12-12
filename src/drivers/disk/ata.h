#ifndef ATA_H
#define ATA_H

#include "block.h"

size_t ata_init(void);
block_t* alloc_block(void);
void free_block(block_t* b);
void ata_sync(block_t* b);

#endif // ATA_H
