/*
 * This file makes a single transfer request to copy data from the src file
 * (on the local SSD) to the destination file (on the PFS).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <mpi.h>
#include "codes.h"
#include "file_utils.h"
#include "mpi_utils.h"
#include "logger.h"


static int _readin(int srcfd, int transfersize, off_t offset, void *buf) {
    return pread(srcfd, buf, transfersize, offset);
}

static int _writeout(int destfd, int transfersize, off_t offset, void *buf) {
    return pwrite(destfd, buf, transfersize, offset);
}


/* Copy transfersize bytes from position offset from srcfd to destfd */
int _copy(int srcfd, int destfd, int transfersize, off_t offset) {
    int rstat, wstat;
    void *buf = NULL;
    if (NULL == (buf = malloc(transfersize))) {
        perror("buf malloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return MALLOC_ERROR;
    }

    rstat = _readin(srcfd,  transfersize, offset, buf);
    if (rstat == -1) {
        perror("pread");
        fprintf(stderr, "%d/%d encountered read error at offset %jd\n",
                get_lrank(), get_grank(), (intmax_t) offset); 
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    log_debug("read %d bytes from offset %lu from fd %d\n",
            rstat, offset, srcfd);

    // write rstat bytes
    wstat = _writeout(destfd, rstat, offset, buf);
    if (wstat == -1) {
        perror("pwrite");
        fprintf(stderr, "%d/%d encountered write error at offset %jd\n",
                get_lrank(), get_grank(), (intmax_t) offset); 
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    free(buf);
    return rstat;
}


/* 
 * A thread copies one transfersize amount of data from all of its subfiles
 * This is called from an OpenMP region
 * nfp: total no. of subfiles for this mpi rank
 */
int copy_step(subf_t* subf, int nfp, int transfersize) {
    int i, n, index, stat=0;
    int num_t= omp_get_num_threads();
    int t_id = omp_get_thread_num();

    // n == how many files initially per thread
    n = nfp/num_t;
    index = n*t_id;

    // Copy the first set of files
    for (i=index; i<(index+n); i++) {
        stat = _copy(subf[i].fd_in, subf[i].fd_out, transfersize, subf[i].offset);
        subf[i].offset += stat;
    }

    // Copy leftover files if num files not exactly divisible by num threads
    i = t_id < (nfp%num_t);
    if(i) {
        index = n*num_t + i-1;
        stat = _copy(subf[index].fd_in, subf[index].fd_out, transfersize, 
                subf[index].offset);
        subf[i].offset += stat;
    }

    return stat;
}

