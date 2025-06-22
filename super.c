#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/buffer_head.h>
#include <linux/fs.h>
// #include <linux/kernel.h>
// #include <linux/module.h>
#include <linux/slab.h>
#include "simplefs.h"

static struct kmem_cache *simplefs_inode_cache;

/* Needed to initiate the inode cache, to allow us to attach
 * filesystem-specific inode information.
 */
int simplefs_init_inode_cache(void)
{
    simplefs_inode_cache = kmem_cache_create_usercopy(
        "simplefs_cache", sizeof(struct simplefs_inode_info), 0, 0, 0,
        sizeof(struct simplefs_inode_info), NULL);
    if (!simplefs_inode_cache)
        return -ENOMEM;
    return 0;
}

/* De-allocate the inode cache */
void simplefs_destroy_inode_cache(void)
{
    /* wait for call_rcu() and prevent the free cache be used */
    rcu_barrier();

    kmem_cache_destroy(simplefs_inode_cache);
}

static struct inode *simplefs_alloc_inode(struct super_block *sb)
{
    struct simplefs_inode_info *ci = kmem_cache_alloc(simplefs_inode_cache, GFP_KERNEL);
    if (!ci) 
        return NULL;
    
    inode_init_once(&ci->vfs_inode);
    return &ci->vfs_inode;
}

static void simplefs_destory_inode(struct inode *inode)
{
    struct simplefs_inode_info *ci = SIMPLEFS_INODE(inode);
    kmem_cache_free(simplefs_inode_cache, ci);
}

static struct super_operations simplefs_super_ops = {
    .alloc_inode = simplefs_alloc_inode,
};

int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head *bh = NULL;
    struct simplefs_sb_info *csb = NULL;
    struct simplefs_sb_info *sbi = NULL;
    struct inode *root_inode = NULL;
    int ret = 0, i;

    /* Initialize the superblock */
    sb->s_magic = SIMPLEFS_MAGIC;
    sb_set_blocksize(sb, SIMPLEFS_BLOCK_SIZE);
    sb->s_maxbytes = SIMPLEFS_MAX_FILESIZE;
    sb->s_op = &simplefs_super_ops;

    /* Read the superblock from disk */
    bh = sb_bread(sb, SIMPLEFS_SB_BLOCK_NR);
    if (!bh)
        return -EIO;

    csb = (struct simplefs_sb_info *)bh->b_data;

    /* Check magic number */
    if (csb->magic != sb->s_magic) {
        pr_err("Wrong magic number\n");
        ret = -EINVAL;
        goto free_bh;
    }

    /* Allocate sbi */
    sbi = kzalloc(sizeof(struct simplefs_sb_info), GFP_KERNEL);
    if (!sbi) {
        ret = -ENOMEM;
        goto free_bh;
    }

    sbi->nr_blocks = csb->nr_blocks;
    sbi->nr_inodes = csb->nr_inodes;
    sbi->nr_istore_blocks = csb->nr_istore_blocks;
    sbi->nr_ifree_blocks = csb->nr_ifree_blocks;
    sbi->nr_bfree_blocks = csb->nr_bfree_blocks;
    sbi->nr_free_inodes = csb->nr_free_inodes;
    sbi->nr_free_blocks = csb->nr_free_blocks;
    sb->s_fs_info = sbi;
    brelse(bh);

    /* Allocate and copy ifree_bitmap */
    sbi->ifree_bitmap = kzalloc(sbi->nr_ifree_blocks * SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
    if (!sbi->ifree_bitmap) {
        ret = -ENOMEM;
        goto free_sbi;
    }

    for (i = 0; i < sbi->nr_ifree_blocks; i++) {
        int idx = 1 + sbi->nr_istore_blocks + i;

        bh = sb_bread(sb, idx);
        if (!bh) {
            ret = -EIO;
            goto free_ifree;
        }

        memcpy((void *)sbi->ifree_bitmap + i * SIMPLEFS_BLOCK_SIZE, bh->b_data, SIMPLEFS_BLOCK_SIZE);

        brelse(bh);
    }

    /* Allocate and copy bfree_bitmap */
    sbi->bfree_bitmap = kzalloc(sbi->nr_bfree_blocks * SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
    if (!sbi->bfree_bitmap) {
        ret = -ENOMEM;
        goto free_ifree;
    }

    for (i = 0; i < sbi->nr_bfree_blocks; i++) {
        int idx = 1 + sbi->nr_istore_blocks + sbi->nr_ifree_blocks + i;

        bh = sb_bread(sb, idx);
        if (!bh) {
            ret = -EIO;
            goto free_bfree;
        }

        memcpy((void *) sbi->bfree_bitmap + i * SIMPLEFS_BLOCK_SIZE, bh->b_data, SIMPLEFS_BLOCK_SIZE);
        brelse(bh);
    }

    /* Create root inode */
    root_inode = simplefs_iget(sb, 1);
    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        goto free_bfree;
    }

#if SIMPLEFS_AT_LEAST(6, 3, 0)
    inode_init_owner(&nop_mnt_idmap, root_inode, NULL, root_inode->i_mode);
#elif SIMPLEFS_AT_LEAST(5, 12, 0)
    inode_init_owner(&init_user_ns, root_inode, NULL, root_inode->i_mode);
#else
    inode_init_owner(root_inode, NULL, root_inode->i_mode);
#endif

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto iput;
    }

    /* TODO: support journal */
    return 0;

iput:
    iput(root_inode);
free_bfree:
    kfree(sbi->bfree_bitmap);
free_ifree:
    kfree(sbi->ifree_bitmap);
free_sbi:
    kfree(sbi);
free_bh:
    brelse(bh);
    
    return ret;
}
