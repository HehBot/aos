#include <memory/kalloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct node {
    uint32_t key;
    size_t value;
    struct node* next;
    struct node* prev;
} node_t;

typedef struct hash_table {
    node_t** table;
    size_t sz;
} hash_table_t;

hash_table_t* init_hash_table(size_t nr_buckets)
{
    hash_table_t* h = kmalloc(sizeof(*h));
    h->sz = nr_buckets;
    h->table = kmalloc(nr_buckets * (sizeof(h->table[0])));
    memset(h->table, 0, nr_buckets * (sizeof(h->table[0])));
    return h;
}

// see https://gist.github.com/badboy/6267743#32-bit-mix-functions
static size_t hash(uint32_t key)
{
    key = (~key) + (key << 15);
    key ^= (key >> 12);
    key += (key << 2);
    key ^= (key >> 4);
    key += ((key << 3) + (key << 11));
    key ^= (key >> 16);
    return key;
}

static node_t* find(hash_table_t* h, uint32_t key, int make)
{
    size_t i = hash(key) % h->sz;
    node_t* node = h->table[i];
    if (node == NULL) {
        if (make) {
            node = kmalloc(sizeof(*h->table[i]));
            node->key = key;
            node->next = node->prev = NULL;
            h->table[i] = node;
        }
        return node;
    } else {
        node_t* n = node;
        while (n != NULL && n->key != key)
            n = n->next;
        if (n == NULL)
            return node;
        return n;
    }
}

void hash_table_insert(hash_table_t* h, uint32_t key, size_t value)
{
    node_t* n = find(h, key, 1);
    if (n->key == key)
        n->value = value;
    else {
        node_t* new = kmalloc(sizeof(*new));
        if (n->next != NULL) {
            n->next->prev = new;
        }
        n->next = new;
        new->prev = n;
        new->next = n->next;
    }
}

void hash_table_delete(hash_table_t* h, uint32_t key)
{
    node_t* n = find(h, key, 0);
    if (n != NULL) {
        if (n->prev != NULL)
            n->prev->next = n->next;
        else
            h->table[hash(key) % h->sz] = n->next;
        if (n->next != NULL)
            n->next->prev = n->prev;
        kfree(n);
    }
}

int hash_table_get(hash_table_t* h, uint32_t key, size_t* value_ptr)
{
    node_t* n = find(h, key, 0);
    if (n != NULL) {
        *value_ptr = n->value;
        return 1;
    } else
        return 0;
}
