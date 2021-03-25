/*
 * This file makes a single transfer request to copy data from the src file
 * (on the local SSD) to the destination file (on the PFS).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "codes.h"

static int _readin(int srcfd, int transfersize, off_t offset, void *buf) {
    if (transfersize != pread(srcfd, buf, transfersize, offset)) {
        perror("pread error");
        return PREAD_ERROR;
    }
    return 0;
}

static int _writeout(int destfd, int transfersize, off_t offset, void *buf) {
    if (transfersize != pwrite(destfd, buf, transfersize, offset)) {
        perror("pread error");
        return PREAD_ERROR;
    }
    return 0;
}

int copy(int srcfd, int destfd, int transfersize, off_t offset) {
    void *buf = NULL;
    if (NULL == (buf = malloc(transfersize))) {
        perror("buf malloc");
        return MALLOC_ERROR;
    }

    _readin   (srcfd,  transfersize, offset, buf);
    _writeout (destfd, transfersize, offset, buf);

    free(buf);
    return 0;
}

