#ifndef SIMPLEFS_BITMAP_H
#define SIMPLEFS_BITMAP_H

#include <linux/bitmap.h>

#include "simplefs.h"

/* Mark the 'len' bit(s) from i-th bit in freemap as free (i.e. 1) */
static inline int put_free_bits(unsigned long *freemap,
                                unsigned long size,
                                uint32_t i,
                                uint32_t len)
{
    /* i is greater than freemap size */
    if (i + len - 1 > size)
        return -1;

    bitmap_set(freemap, i, len);

    return 0;
}

static inline void put_blocks(struct simplefs_sb_info *sbi,
                              uint32_t bno,
                              uint32_t len)
{
    if (put_free_bits(sbi->bfree_bitmap, sbi->nr_blocks, bno, len))
        return;
    
        sbi->nr_free_blocks += len;
}

#endif /* SIMPLEFS_BITMAP_H */