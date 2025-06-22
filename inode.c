#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "bitmap.h"
#include "simplefs.h"

static const struct inode_operations simplefs_inode_ops;

/* Either return the inode that corresponds to a given inode number (ino), if
 * it is already in the cache, or create a new inode object if it is not in the
 * cache.
 *
 * Note that this function is very similar to simplefs_new_inode, except that
 * the requested inode is supposed to be allocated on-disk already. So do not
 * use this to create a completely new inode that has not been allocated on
 * disk.
 */
struct inode *simplefs_iget(struct super_block *sb, unsigned long ino)
{
    struct inode *inode = NULL;
    struct simplefs_inode *cinode = NULL;
    struct simplefs_inode_info *ci = NULL;
    struct simplefs_sb_info *sbi = SIMPLEFS_SB(sb);
    struct buffer_head *bh = NULL;
    uint32_t inode_block = (ino / SIMPLEFS_INODES_PER_BLOCK) + 1;
    uint32_t inode_shift = ino % SIMPLEFS_INODES_PER_BLOCK;
    int ret = 0;

    /* Fail if ino is out of range */
    if (ino >= sbi->nr_inodes) 
        return ERR_PTR(-EINVAL);
    
    /* Get a locked inode from Linux */
    inode = iget_locked(sb, ino);
    if (!inode)
        return ERR_PTR(-ENOMEM);

    /* If inode is in cache, return it */
    if (!(inode->i_state & I_NEW))
        return inode;

    ci = SIMPLEFS_INODE(inode);
    /* Read inode from disk and initialize */
    bh = sb_bread(sb, inode_block);
    if (!bh) {
        ret = -EIO;
        goto failed;
    }

    cinode = (struct simplefs_inode *) bh->b_data;
    cinode += inode_shift;

    inode->i_ino = ino;
    inode->i_sb = sb;
    inode->i_op = &simplefs_inode_ops;

    inode->i_mode = le32_to_cpu(cinode->i_mode);
    i_uid_write(inode, le32_to_cpu(cinode->i_uid));
    i_gid_write(inode, le32_to_cpu(cinode->i_gid));
    inode->i_size = le32_to_cpu(cinode->i_size);

#if SIMPLEFS_AT_LEAST(6, 6, 0)
    inode_set_ctime(inode, (time64_t) le32_to_cpu(cinode->i_ctime), 0);
#else
    inode->i_ctime.tv_sec = (time64_t) le32_to_cpu(cinode->i_ctime);
    inode->i_ctime.tv_nsec = 0;
#endif

#if SIMPLEFS_AT_LEAST(6, 7, 0)
    inode_set_atime(inode, (time64_t) le32_to_cpu(cinode->i_atime), 0);
    inode_set_mtime(inode, (time64_t) le32_to_cpu(cinode->i_mtime), 0);
#else
    inode->i_atime.tv_sec = (time64_t) le32_to_cpu(cinode->i_atime);
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_sec = (time64_t) le32_to_cpu(cinode->i_mtime);
    inode->i_mtime.tv_nsec = 0;
#endif

    inode->i_blocks = le32_to_cpu(cinode->i_blocks);
    set_nlink(inode, le32_to_cpu(cinode->i_nlink));

    if (S_ISDIR(inode->i_mode)) {
        ci->ei_block = le32_to_cpu(cinode->ei_block);
        // inode->i_fop = &simplefs_dir_ops;
    } else if (S_ISREG(inode->i_mode)) {
        ci->ei_block = le32_to_cpu(cinode->ei_block);
        inode->i_fop = &simplefs_file_ops;
        inode->i_mapping->a_ops = &simplefs_aops;
    } else if (S_ISLNK(inode->i_mode)) {
        // strncpy(ci->i_data, cinode->i_data, sizeof(ci->i_data));
        // inode->i_link = ci->i_data;
        // inode->i_op = &symlink_inode_ops;
    }

    brelse(bh);

    /* Unlock the inode to mark it usable */
    unlock_new_inode(inode);

    return inode;

failed:
    brelse(bh);
    iget_failed(inode);
    return ERR_PTR(ret);
}

static const struct inode_operations simplefs_inode_ops = {

};