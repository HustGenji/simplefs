#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "simplefs.h"

struct superblock {
    union {
        struct simplefs_sb_info info;
        char padding[SIMPLEFS_BLOCK_SIZE]; /* Padding to match block size */
    };
    
};

_Static_assert(sizeof(struct superblock) == SIMPLEFS_BLOCK_SIZE);


int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s diskName\n",argv[0]);
    }

    /* Open disk image */
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("open():");
        return EXIT_FAILURE;
    }

    /* Get image size */
    struct stat stat_buf;
    int ret = fstat(fd, &stat_buf);
    if (ret) {
        perror("fstat():");
        ret = EXIT_FAILURE;
        goto fclose;
    }

fclose:
    close(fd);
}