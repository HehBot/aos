#ifndef HASH_H
#define HASH_H

#include <stddef.h>

// hash table

typedef struct hash_table hash_table_t;

hash_table_t* init_hash_table(size_t nr_buckets);
void hash_table_insert(hash_table_t* h, uint32_t key, size_t value);
void hash_table_delete(hash_table_t* h, uint32_t key);
int hash_table_get(hash_table_t* h, uint32_t key, size_t* value_ptr);

#endif // HASH_H
