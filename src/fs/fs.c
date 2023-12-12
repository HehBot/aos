#include <fs/fs.h>
#include <stddef.h>

fs_node_t* fs_root = NULL;

size_t read_fs(fs_node_t* node, size_t offset, size_t size, void* buf)
{
    if (FS_TYPE(node->flags) != FS_DIR && node->read)
        return node->read(node, offset, size, buf);
    return 0;
}
size_t write_fs(fs_node_t* node, size_t offset, size_t size, void* buf)
{
    if (FS_TYPE(node->flags) != FS_DIR && node->write)
        return node->write(node, offset, size, buf);
    return 0;
}
void open_fs(fs_node_t* node, size_t, size_t)
{
    if (FS_TYPE(node->flags) != FS_DIR && node->open)
        node->open(node);
}
void close_fs(fs_node_t* node)
{
    if (FS_TYPE(node->flags) != FS_DIR && node->close)
        node->close(node);
}

struct dirent* read_dir_fs(fs_node_t* node, size_t index)
{
    if (FS_TYPE(node->flags) == FS_DIR && node->read_dir)
        return node->read_dir(node, index);
    return NULL;
}
fs_node_t* find_dir_fs(fs_node_t* node, char const* name)
{
    if (FS_TYPE(node->flags) == FS_DIR && node->find_dir)
        return node->find_dir(node, name);
    return NULL;
}
