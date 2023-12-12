#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>

typedef struct superblock {
    uint32_t nr_inodes;
    uint32_t nr_blocks;
    uint32_t su_res_blocks;
    uint32_t nr_free_blocks;
    uint32_t nr_free_inodes;
    uint32_t superblock_block;
    uint32_t log_block_sz;
    uint32_t log_fgmt_sz;
    uint32_t blocks_per_block_grp;
    uint32_t fgmts_per_block_grp;
    uint32_t inodes_per_block_grp;
    uint32_t last_mnt_time;
    uint32_t last_write_time;
    uint16_t last_chk_mnt;
    uint16_t max_chk_mnt;
    uint16_t ext2_sign;
    uint16_t fs_state_error[2];
    uint16_t minor;
    uint32_t last_chk_time;
    uint32_t max_chk_time;
    uint32_t osid;
    uint32_t major;
    uint16_t res_uid;
    uint16_t res_gid;
    uint32_t first_nonres_inode;
    uint16_t inode_sz;
    uint16_t superblock_block_grp;
    uint32_t optional_features;
    uint32_t required_features;
    uint32_t readonly_features;
    uint32_t fsid[4];
    uint8_t volume_name[16];
    uint8_t last_mount_path[64];
    uint32_t compession_algo;
    uint8_t nr_blocks_prealloc_files;
    uint8_t nr_blocks_prealloc_dirs;
    uint16_t : 16;
    uint32_t jid[4];
    uint32_t journal_inode;
    uint32_t journal_dev;
    uint32_t head_orphan_inode;
} __attribute__((packed)) superblock_t;

typedef struct block_group_desc {
    uint32_t block_usage_bitmap_block;
    uint32_t inode_usage_bitmap_block;
    uint32_t inode_table_block;
    uint16_t nr_free_blocks;
    uint16_t nr_free_inodes;
    uint16_t nr_dir;
    uint32_t : 32;
    uint32_t : 32;
    uint32_t : 32;
    uint16_t : 16;
} block_group_desc_t;

typedef struct dir_ent {
    uint32_t inode;
    uint16_t entry_sz;
    uint8_t name_length_low;
    union {
        uint8_t name_length_high;
        uint8_t type_indicator;
    };
    uint8_t name[];
} __attribute__((packed)) dir_ent_t;

typedef struct inode {
    uint16_t type_perm;
    uint16_t uid;
    uint32_t file_size_low;
    uint32_t last_acc_time;
    uint32_t creat_time;
    uint32_t last_mod_time;
    uint32_t del_time;
    uint16_t gid;
    uint16_t hard_link_ref_count;
    uint32_t nr_sectors;
    uint32_t flags;
    uint32_t os1;
    uint32_t dbp[12];
    uint32_t sibp;
    uint32_t dibp;
    uint32_t tibp;
    uint32_t gen;
    uint32_t eab;
    union {
        uint32_t file_size_high;
        uint32_t dir_acl;
    };
    uint32_t fgmt_block;
    uint32_t os2[3];
} __attribute__((packed)) inode_t;

#endif // EXT2_H
