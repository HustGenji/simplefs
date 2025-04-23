#include <stdint.h>

#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#define SIMPLEFS_MAGIC 0xdeadcell

#define SIMPLEFS_SB_BLOCK_NR 0

#define SIMPLEFS_BLOCK_SIZE (1 << 12) /* 4KiB */

#define SIMPLEFS_FILENAME_LEN 255

/* simplefs partition layout
 * +---------------+
 * |  superblock   |  1 block
 * +---------------+
 * |  inode store  |  sb->nr_istore_blocks blocks
 * +---------------+
 * | ifree bitmap  |  sb->nr_ifree_blocks blocks
 * +---------------+
 * | bfree bitmap  |  sb->nr_bfree_blocks blocks
 * +---------------+
 * |    data       |
 * |      blocks   |  rest of the blocks
 * +---------------+
 */

struct simplefs_inode {
    uint32_t i_mode;   /* File mode */
    uint32_t i_uid;    /* Owner id */
    uint32_t i_gid;    /* Group id */
    uint32_t i_size;   /* Size in bytes */
    uint32_t i_ctime;  /* Inode change time */
    uint32_t i_atime;  /* Access time */
    uint32_t i_mtime;  /* Modification time */
    uint32_t i_blocks; /* Block count */
    uint32_t i_nlink;  /* Hard links count */
    uint32_t ei_block; /* Block with list of extents for this file */
    char i_data[32];   /* store symlink content */
};

#define SIMPLEFS_INODES_PER_BLOCK \
    (SIMPLEFS_BLOCK_SIZE / sizeof(struct simplefs_inode))

struct simplefs_sb_info {
    uint32_t magic; /* Magic Number */
    
    uint32_t nr_blocks; /* Total number of blocks (include sb & inodes) */
    uint32_t nr_inodes; /* Total number of inodes */

    uint32_t nr_istore_blocks; /* Number of inode store blocks */
    uint32_t nr_ifree_blocks;  /* Number of inode free bitmap blocks */
    uint32_t nr_bfree_blocks;  /* Number of block free bitmap blocks */

    uint32_t nr_free_inodes; /* Number of free inodes */
    uint32_t nr_free_blocks; /* Number of free blocks */

    unsigned long *ifree_bitmap; /* In-memory free inodes bitmap */
    unsigned long *bfree_bitmap; /* In-memory free blocks bitmap */
};

struct simplefs_extent {
    uint32_t ee_block; /* first logical block extent covers */
    uint32_t ee_len;   /* number of blocks covered by extent */
    uint32_t ee_start; /* first physical block extent covers */
};

struct simplefe_file {
    uint32_t inode;
    char filename[SIMPLEFS_FILENAME_LEN];
};

#endif /* SIMPLEFS_H */