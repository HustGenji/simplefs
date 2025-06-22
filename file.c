#define pr_fmt(fmt) "simplefs: " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpage.h>

#include "bitmap.h"
#include "simplefs.h"

/*
 * Called when a file is opened in the simplefs.
 * It checks the flags associated with the file opening mode (O_WRONLY, O_RDWR,
 * O_TRUNC) and performs truncation if the file is being opened for write or
 * read/write and the O_TRUNC flag is set.
 *
 * Truncation is achieved by reading the file's index block from disk, iterating
 * over the data block pointers, releasing the associated data blocks, and
 * updating the inode metadata (size and block count).
 */
static int simplefs_open(struct inode *inode, struct file *filp)
{
    bool wronly = (filp->f_flags & O_WRONLY);
    bool rdwr = (filp->f_flags & O_RDWR);
    bool trunc = (filp->f_flags & O_TRUNC);

    if ((wronly || rdwr) && trunc && inode->i_size) {
        struct buffer_head *bh_index;
        struct simplefs_file_ei_block *ei_block;
        sector_t iblock;

        /* Fetch the file's extent block from disk */
        bh_index = sb_bread(inode->i_sb, SIMPLEFS_INODE(inode)->ei_block);
        if (!bh_index)
            return -EIO;
        
        ei_block = (struct simplefs_file_ei_block*) bh_index->b_data;

        for (iblock = 0; (iblock <= SIMPLEFS_MAX_EXTENTS) && (ei_block->extents[iblock].ee_start); iblock++) {
            put_blocks(SIMPLEFS_SB(inode->i_sb),
                       ei_block->extents[iblock].ee_start,
                       ei_block->extents[iblock].ee_len);
            memset(&ei_block->extents[iblock], 0, sizeof(struct simplefs_extent));
        }
        /* Update inode metadata */
        inode->i_size = 0;
        inode->i_blocks = 1;

        mark_buffer_dirty(bh_index);
        brelse(bh_index);
        mark_inode_dirty(inode);
    }
    return 0;
}

const struct address_space_operations simplefs_aops = {

};

const struct file_operations simplefs_file_ops = {
    .owner = THIS_MODULE,
    .open = simplefs_open,
};