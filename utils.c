#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

void print_usage() {
    fprintf(stderr, "Run with args:\n"
            "\t\t-n num_sim_ranks_on_node\n"
            "\t\t-t ssd->pfs data transfer size\n"
            "\t\t-p monitoring policy id 0/1\n"
            "\t\t-s arg for relaxed monitor policy\n"
            "\t\t-f adios filename\n"
            "\t\t-w num adios subsfiles per node\n"
            "\t\t-d draining method "
            "(1-coordinate flushing with computation, "
            "2-independent draining, "
            "3-posthoc draining)\n"
            "\t\t-m prefix to the nvm (e.g. /mnt/bb/<userid> on Summit)\n, "
            "\t\t-i microseconds between flush epochs, for independent draining\n, "
            "\t\t-a 0/1. 1- flush all available data everytime (independent draining)\n");
    fflush(stderr);
}


int read_input_args(int argc, char **argv, int rank, int *num_sim_ranks, 
        int *transfersize, int *monpolicy, int *monparg, char** adfname,
        int *ad_n, char* nvm_prefix, int* drain_type,
        unsigned long int* sleep_interval, int *copyall) {
    
    int c, n=0, t=0, p=0, s=0, f=0, w=0, m=0, d=0, i=0, a=0;
    char *size_mg;
    char* drain_type_str = NULL;

    while ((c=getopt(argc, argv, "n:t:p:s:f:w:m:d:i:a:")) != -1)
        switch(c) {
            case 'n':
                n=1;
                *num_sim_ranks = strtol(optarg, &size_mg, 10);
                if (*num_sim_ranks < 1 || *num_sim_ranks > 40) {
                    fprintf(stderr,
                            "Input arg num_sim_ranks must be 1 < n > 40. "
                            "Found %d\n", *num_sim_ranks);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 't':
                t=1;
                *transfersize = strtol(optarg, &size_mg, 10);
                if (*transfersize <= 0 || *transfersize > 2147483647) {
                    fprintf(stderr,
                            "Invalid input arg transfer size %d\n",
                            *transfersize);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 'p':
                p=1;
                *monpolicy = strtol(optarg, &size_mg, 10);
                if (*monpolicy != 0 && *monpolicy != 1) {
                    fprintf(stderr, "Monitor policy id must be 0/1. "
                            "Found %d\n", *monpolicy);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 's':
                s=1;
                *monparg = strtol(optarg, &size_mg, 10);
                break;
            case 'f':
                f=1;
                *adfname = optarg;
                break;
            case 'w':
                w=1;
                *ad_n = strtol(optarg, &size_mg, 10);
                if (*ad_n < 1 && *ad_n > 40) {
                    fprintf(stderr,
                            "Number of local adios subfiles must be 1<n<40. "
                            "Found %d\n", *ad_n);
                    fflush(stderr);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                    exit(1);
                }
                break;
            case 'm':
                m=1;
                if (strlen(optarg) > 127) {
                    if(rank == 0)
                        fprintf(stderr,
                                "cmd line arg for nvm prefix too long. "
                                "Maxlen = 127 chars. ABORTING.\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                memset(nvm_prefix, 0, strlen(nvm_prefix));
                strcpy(nvm_prefix, optarg);
                break;
            case 'd':
                d=1;
                *drain_type = strtol(optarg, &size_mg, 10);
                if( (*drain_type < 1) || (*drain_type > 3) ) {
                    if (rank == 0)
                        fprintf(stderr, "draining method must be 1, 2, or 3.\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                break;
            case 'i':
                i=1;
                *sleep_interval = strtol(optarg, &size_mg, 10);
                if(*sleep_interval == 0) {
                    if(rank==0)
                        fprintf(stderr, "Sleep interval between flush epochs"
                                "cannot be 0: %uld\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                break;
            case 'a':
                a=1;
                *copyall = strtol(optarg, &size_mg, 10);
                break;
            default:
                print_usage();
                MPI_Abort(MPI_COMM_WORLD, 1);
                exit(1);
        }

    if (!n || !t || !p || !s || !f || !w || !m || !d || !i || !a) {
        print_usage();
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }
    
    if (*monparg > *num_sim_ranks) {
        fprintf(stderr,
                "Monitor policy arg %d cannot be > num_sim_ranks %d\n",
                *monparg, *num_sim_ranks);
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }

    if (rank == 0) {
        drain_type_str = "coordinated";
        if (*drain_type == 2) drain_type_str = "independent";
        if (*drain_type == 3) drain_type_str = "posthoc";

        fprintf(stdout, "INPUT ARGS: ");
        fprintf(stdout, "num_sim_ranks_on_node: %d, ", *num_sim_ranks);
        fprintf(stdout, "transfersize: %d, ", *transfersize);
        fprintf(stdout, "monpolicy: %d, ", *monpolicy);
        fprintf(stdout, "monpolicyarg: %d, ", *monparg);
        fprintf(stdout, "adios_file: %s, ", *adfname);
        fprintf(stdout, "n_subfiles_per_node: %d, ", *ad_n);
        fprintf(stdout, "nvm prefix: '%s', ", nvm_prefix);
        fprintf(stdout, "draining method: %s, ", drain_type_str);
        fprintf(stdout, "useconds between flushes for independent drain: %lu, ", *sleep_interval);
        fprintf(stdout, "copy all new data everytime (1 for yes, no otherwise): %d\n", *copyall);
        fflush(stdout);
    }

    return 0;
}

