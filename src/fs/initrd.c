// initrd.c -- Defines the interface for and structures relating to the initial ramdisk.
// Written for JamesM's kernel development tutorials.

#include <fs/initrd.h>
#include <memory/kalloc.h>
#include <string.h>

initrd_header_t* initrd_header; // The header.
initrd_file_header_t* file_headers; // The list of file headers.
fs_node_t* initrd_root; // Our root directory node.
fs_node_t* initrd_dev; // We also add a directory node for /dev, so we can mount devfs later on.
fs_node_t* root_nodes; // List of file nodes.
size_t nroot_nodes; // Number of file nodes.

struct dirent dirent;

static size_t initrd_read(fs_node_t* node, size_t offset, size_t size, void* buffer)
{
    initrd_file_header_t header = file_headers[node->inode];
    if (offset >= header.length)
        return 0;
    if (offset + size > header.length)
        size = header.length - offset;
    memcpy(buffer, (void*)(initrd_header) + header.offset + offset, size);
    return size;
}

static struct dirent* initrd_read_dir(fs_node_t* node, size_t index)
{
    if (node == initrd_root && index == 0) {
        strcpy(dirent.name, "dev");
        dirent.name[3] = 0; // Make sure the string is NULL-terminated.
        dirent.inode = 0;
        return &dirent;
    }
    if (index - 1 >= nroot_nodes)
        return NULL;
    strcpy(dirent.name, (char const*)root_nodes[index - 1].name);
    dirent.inode = root_nodes[index - 1].inode;
    return &dirent;
}

static fs_node_t* initrd_find_dir(fs_node_t* node, char const* name)
{
    if (node == initrd_root
        && !strcmp(name, "dev"))
        return initrd_dev;

    for (size_t i = 0; i < nroot_nodes; i++)
        if (!strcmp(name, (char const*)root_nodes[i].name))
            return &root_nodes[i];
    return NULL;
}

fs_node_t* read_initrd(void* location)
{
    // Initialise the main and file header pointers and populate the root directory.
    initrd_header = (initrd_header_t*)location;
    file_headers = (initrd_file_header_t*)(location + sizeof(initrd_header_t));

    // Initialise the root directory.
    initrd_root = kmalloc(sizeof(fs_node_t));
    strcpy((char*)initrd_root->name, "initrd");
    initrd_root->mask = initrd_root->uid = initrd_root->gid = initrd_root->inode = initrd_root->length = 0;
    initrd_root->flags = FS_DIR;
    initrd_root->read_dir = &initrd_read_dir;
    initrd_root->find_dir = &initrd_find_dir;
    initrd_root->impl = 0;

    // Initialise the /dev directory (required!)
    initrd_dev = kmalloc(sizeof(fs_node_t));
    strcpy((char*)initrd_dev->name, "dev");
    initrd_dev->mask = initrd_dev->uid = initrd_dev->gid = initrd_dev->inode = initrd_dev->length = 0;
    initrd_dev->flags = FS_DIR;
    initrd_dev->read_dir = &initrd_read_dir;
    initrd_dev->find_dir = &initrd_find_dir;
    initrd_dev->impl = 0;

    root_nodes = (fs_node_t*)kmalloc(sizeof(fs_node_t) * initrd_header->nfiles);
    nroot_nodes = initrd_header->nfiles;

    // For every file...
    for (size_t i = 0; i < initrd_header->nfiles; i++) {
        // Create a new file node.
        strcpy((char*)root_nodes[i].name, (char const*)file_headers[i].name);
        root_nodes[i].mask = root_nodes[i].uid = root_nodes[i].gid = 0;
        root_nodes[i].length = file_headers[i].length;
        root_nodes[i].flags = FS_FILE;
        root_nodes[i].read = &initrd_read;
        root_nodes[i].write = NULL;
        root_nodes[i].open = NULL;
        root_nodes[i].close = NULL;
        root_nodes[i].impl = 0;
        root_nodes[i].inode = i;
    }

    return initrd_root;
}
