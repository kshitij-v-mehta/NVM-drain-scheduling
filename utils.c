#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

void print_usage() {
    fprintf(stderr, "Run with args: -n num_sim_ranks_on_node -t ssd->pfs data transfer size -p monitoring policy id 0/1 -s arg for relaxed monitor policy\n");
    fflush(stderr);
}


int read_input_args(int argc, char **argv, int rank, int *num_sim_ranks, int *transfersize, int *monpolicy, int *monparg) {
    int c, n=0, t=0, p=0, s=0;
    char *size_mg;

    while ((c=getopt(argc, argv, "n:t:p:s:")) != -1)
        switch(c) {
            case 'n':
                n=1;
                *num_sim_ranks = strtol(optarg, &size_mg, 10);
                if (*num_sim_ranks < 1 || *num_sim_ranks > 40) {
                    fprintf(stderr, "Input arg num_sim_ranks must be 1 < n > 40. Found %d\n", *num_sim_ranks);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 't':
                t=1;
                *transfersize = strtol(optarg, &size_mg, 10);
                if (*transfersize <= 0 || *transfersize > 2147483647) {
                    fprintf(stderr, "Invalid input arg transfer size %d\n", *transfersize);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 'p':
                p=1;
                *monpolicy = strtol(optarg, &size_mg, 10);
                if (*monpolicy != 0 && *monpolicy != 1) {
                    fprintf(stderr, "Monitor policy id must be 0/1. Found %d\n", *monpolicy);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 's':
                p=1;
                *monparg = strtol(optarg, &size_mg, 10);
                break;
            default:
                print_usage();
                MPI_Abort(MPI_COMM_WORLD, 1);
                exit(1);
        }

    if (!n || !t || !p) {
        print_usage();
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }
    
    if (*monparg > *num_sim_ranks) {
        fprintf(stderr, "Monitor policy arg %d cannot be > num_sim_ranks %d\n", *monparg, *num_sim_ranks);
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }

    if (rank == 0) {
        fprintf(stdout, "INPUT ARGS: num_sim_ranks_on_node: %d, transfersize: %d, monpolicy: %d, monpolicyarg: %d\n", 
                *num_sim_ranks, *transfersize, *monpolicy, *monparg);
        fflush(stdout);
    }

    return 0;
}

