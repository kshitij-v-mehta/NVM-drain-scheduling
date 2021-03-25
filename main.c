#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include "codes.h"
#include "monitor.h"
#include "utils.h"


int main(int argc, char **argv) {
    int g_num_ranks, grank;    // comm world rank and size
    int num_sim_ranks;
    int transfersize;
    int monpolicy, monpolicyarg=0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &grank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_num_ranks);

    read_input_args(argc, argv, grank, &num_sim_ranks, &transfersize, &monpolicy, &monpolicyarg);

    mon_init(num_sim_ranks, monpolicy, monpolicyarg);
    nw_traffic_status();

    mon_finalize();
    MPI_Finalize();
    return 0;
}

