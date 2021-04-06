#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include "codes.h"


/* Total no. of simulation ranks that you must track */
static int num_ranks;

/* Line of the L1, L2 (and presumably L3) caches on Summit */
int cache_line_size = 128;

/* Size of the shared memory segment that will be allocated.
 * shmsize = num_ranks * cache_line_size
 * to avoid false sharing between threads. */
static int shmsize;

/* A unique ID to represent the shm key. */
static int shmkey = 915;

static int  _shmid;
static int* _shm_ptr;


/*
 * Returns the no. of ranks that have reported their status as GREEN -
 * not currently involved in MPI communication
 */
int shm_get_green() {
    int i, ngreen = 0;
    for(i=0; i<num_ranks; i++) {
        if(_shm_ptr[i*cache_line_size] == GREEN)
            ngreen++;
    }

    return ngreen;
}

/*
 * Returns the no. of ranks that have reported their status as RED -
 * currently involved in MPI communication
 */
int shm_get_red() {
    int i, nred = 0;
    for(i=0; i<num_ranks; i++) {
        if(_shm_ptr[i*cache_line_size] == RED)
            nred++;
    }

    return nred;
}

/*
 * Set the no. of simulation ranks that need to be monitored.
 * Create and attach the shared memory segment.
 */
int shm_init(int n) {
    num_ranks = n;
    shmsize = num_ranks * cache_line_size;

    // Create the shm segment
    _shmid = shmget(shmkey, shmsize, IPC_CREAT|0666);
    if (_shmid < 0) {
        perror("shmget");
        return SHMGET_ERROR;
    }

    // Attach the shm segment to my address space
    _shm_ptr = shmat(_shmid, NULL, 0);
    if (_shm_ptr == (void *) -1) {
        perror("shmat");
        return SHMAT_ERROR;
    }

    return 0;
}

/*
 * Detach the shm segment
 */
int shm_finalize() {
    shmdt(_shm_ptr);
    return 0;
}

