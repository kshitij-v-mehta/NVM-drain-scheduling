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
                get_lrank(), get_grank(), offset); 
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
                get_lrank(), get_grank(), offset); 
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    free(buf);
    return rstat;
}


/* 
 * A thread copies one transfersize amount of data from all of its subfiles
 * if copyall==1, copy all data available in the file
 * This is called from an OpenMP region
 * nfp: total no. of subfiles for this mpi rank
 * Returns the number of bytes copied
 */
int copy_step(subf_t* subf, int nfp, int transfersize, int copyall) {
    // ret == no. of bytes copied in a single copy call
    // stat == total no. of bytes copied in this function call
    int i, n, index, stat=0, ret=1;
    int num_t= omp_get_num_threads();
    int t_id = omp_get_thread_num();

    // n == how many files initially per thread
    n = nfp/num_t;
    index = n*t_id;

    // Copy the first set of files
    for (i=index; i<(index+n); i++) {
        log_debug("copying %s, fd: %d, transfersize: %d, copyall: %d\n",
                subf[i].fname_ssd, subf[i].fd_in, transfersize, copyall);
        if (copyall == 1) {
            while(ret > 0) {
                ret = _copy(subf[i].fd_in, subf[i].fd_out, transfersize, subf[i].offset);
                subf[i].offset += ret;
                stat += ret;
            }
        }
        else {
            ret = _copy(subf[i].fd_in, subf[i].fd_out, transfersize, subf[i].offset);
            subf[i].offset += ret;
            stat += ret;
        }
    }

    // Copy leftover files if num files not exactly divisible by num threads
    i = t_id < (nfp%num_t);
    if(i) {
        index = n*num_t + t_id;
        log_debug("copying %s, fd: %d\n", subf[index].fname_ssd, subf[index].fd_in); 
        ret = _copy(subf[index].fd_in, subf[index].fd_out, transfersize, subf[index].offset);
        subf[index].offset += ret;
        stat += ret;
    }

    // Return the total no. of bytes copied in this function
    return stat;
}

