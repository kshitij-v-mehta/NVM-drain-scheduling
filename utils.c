#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

void print_usage() {
    fprintf(stderr, "Run with args: -n num_sim_ranks_on_node -t ssd->pfs data transfer size -p monitoring policy id 0/1 -s arg for relaxed monitor policy -f adios filename -w num adios subsfiles per node\n");
    fflush(stderr);
}


int read_input_args(int argc, char **argv, int rank, int *num_sim_ranks, int *transfersize, int *monpolicy, int *monparg, char** adfname, int *ad_n) {
    int c, n=0, t=0, p=0, s=0, f=0, w=0;
    char *size_mg;

    while ((c=getopt(argc, argv, "n:t:p:s:f:w:")) != -1)
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
            case 'f':
                f=1;
                *adfname = optarg;
                break;
            case 'w':
                w=1;
                *ad_n = strtol(optarg, &size_mg, 10);
                if (*ad_n < 1 && *ad_n > 40) {
                    fprintf(stderr, "Number of local adios subfiles must be 1<n<40. Found %d\n", *ad_n);
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

    if (!n || !t || !p || !f || !w) {
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
        fprintf(stdout, "INPUT ARGS: num_sim_ranks_on_node: %d, transfersize: %d, monpolicy: %d, monpolicyarg: %d, adios_file: %s, n_subfiles_per_node: %d\n", 
                *num_sim_ranks, *transfersize, *monpolicy, *monparg, *adfname, *ad_n);
        fflush(stdout);
    }

    return 0;
}
