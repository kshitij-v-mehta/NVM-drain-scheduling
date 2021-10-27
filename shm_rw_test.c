/*
 * This is an independent program to test the reading and writing of 
 * shared memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <mpi.h>
#include <unistd.h>


/* Total no. of simulation ranks that you must track */
int num_ranks;

/* Line of the L1, L2 (and presumably L3) caches on Summit */
int cache_line_size = 128;
//int cache_line_size = 1;

/* Size of the shared memory segment that will be allocated.
 * shmsize = num_ranks * cache_line_size
 * to avoid false sharing between threads. */
size_t shmsize;

/* A unique ID to represent the shm key. */
int shmkey = 925;

int  _shmid;
int* _shm_ptr;


/*
 * Set the no. of simulation ranks that need to be monitored.
 * Create and attach the shared memory segment.
 */
int shm_init(int rank, int n) {
    num_ranks = n;
    int i;
    shmsize = num_ranks * cache_line_size;
    printf("shmsize: %zu\n", shmsize);

    // Create the shm segment
    _shmid = shmget(shmkey, shmsize, IPC_CREAT|0666);
    if (_shmid < 0) {
        perror("shmget");
        return -1;
    }

    // Attach the shm segment to my address space
    _shm_ptr = shmat(_shmid, NULL, 0);
    if (_shm_ptr == (void *) -1) {
        perror("shmat");
        return -1;
    }

    // Set all values to some default
    for(i=rank*cache_line_size;i<cache_line_size/sizeof(int);i++)
        _shm_ptr[i] = -9;

    return 0;
}

/*
 * Detach the shm segment
 */
int shm_finalize() {
    shmdt(_shm_ptr);
    return 0;
}


int shm_write(int rank, int size) {
    int index = rank*cache_line_size;
    printf("Rank %d writing at loc %d\n", rank, index);
    _shm_ptr[index] = 9078;
}


int shm_read(int rank, int size) {
    sleep(5);
    int shmindex = (rank - size/2) * cache_line_size;
    fprintf(stdout, "Rank %d, loc: %d, val: %d\n", rank, shmindex, _shm_ptr[shmindex]);
    fflush(stdout);
}

int main(int argc, char **argv) {
    int i, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    //printf("Rank %d of %d checking in\n", rank, num_ranks);
    if ( (num_ranks%2) !=0) {
        fprintf(stderr, "Even no. of ranks required.\n");
        return -1;
    }

    shm_init(rank, num_ranks);

    // Half of the ranks are writers, remaining are readers
    if (rank < num_ranks/2) shm_write(rank, num_ranks);
    else                    shm_read (rank, num_ranks);

    // Cleanup
    shm_finalize();
    MPI_Finalize();
    return 0;
}

