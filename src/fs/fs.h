#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#define FS_TYPE(x) ((x)&0x07)
#define FS_FILE 0x01
#define FS_DIR 0x02
#define FS_CHARDEV 0x03
#define FS_BLKDEV 0x04
#define FS_PIPE 0x05
#define FS_SYMLINK 0x06

#define FS_MOUNTPOINT 0x08

struct fs_node;
typedef size_t (*read_file_t)(struct fs_node*, size_t, size_t, void*);
typedef size_t (*write_file_t)(struct fs_node*, size_t, size_t, void*);
typedef void (*open_file_t)(struct fs_node*);
typedef void (*close_file_t)(struct fs_node*);

struct dirent {
    char name[128];
    uint32_t ino;
};
typedef struct dirent* (*read_dir_t)(struct fs_node*, size_t);
typedef struct fs_node* (*find_dir_t)(struct fs_node*, char const* name);

typedef struct fs_node {
    uint8_t name[128];
    uint32_t inode;

    uint32_t mask;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t length;
    uint32_t impl;

    union {
        struct {
            read_file_t read;
            write_file_t write;
            open_file_t open;
            close_file_t close;
        };
        struct {
            read_dir_t read_dir;
            find_dir_t find_dir;
        };
    };

    struct fs_node* ptr;
} fs_node_t;

extern fs_node_t* fs_root;

size_t read_fs(fs_node_t* node, size_t offset, size_t size, void* buf);
size_t write_fs(fs_node_t* node, size_t offset, size_t size, void* buf);
void open_fs(fs_node_t* node, size_t read, size_t write);
void close_fs(fs_node_t* node);
struct dirent* read_dir_fs(fs_node_t* node, size_t index);
fs_node_t* find_dir_fs(fs_node_t* node, char const* name);

#endif // FS_H
