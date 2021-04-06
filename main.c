#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include "codes.h"
#include "monitor.h"
#include "utils.h"
#include "file_utils.h"

static int   g_num_ranks, grank;    // comm world rank and size
static int   num_sim_ranks;         // no. of simulation ranks to monitor
static int   transfersize;          // bytes to copy from ssd->pfs in each call
static int   monpolicy;             // monitoring policy id to implement
static int   monpolicyarg=0;        // input arg to monitoring policy relaxed
static char* ad_fname = NULL;       // adios output file name
static int   ad_nw = 0;             // no. of adios writers per node
static int   fd_in[40];             // fds for adios input subfiles
static int   fd_out[40];            // fds for adios output subfiles


int _mainloop() {
    nw_traffic_status();
}


/* ------------------------------------------------------------------------ */
int main(int argc, char **argv) {
    // MPI initializations
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &grank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_num_ranks);

    // Read input args
    read_input_args(argc, argv, grank, &num_sim_ranks, &transfersize, 
                    &monpolicy, &monpolicyarg, &ad_fname, &ad_nw);

    // Initialize traffic monitor
    mon_init(num_sim_ranks, monpolicy, monpolicyarg);

    // Open input and output ADIOS subfiles
    ad_nw = open_subfiles(ad_fname, fd_in, fd_out);

    // Start the main loop
    _mainloop();

    // Cleanup
    mon_finalize();
    MPI_Finalize();
    return 0;
}

