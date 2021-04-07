#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include "codes.h"
#include "monitor.h"
#include "utils.h"
#include "file_utils.h"
#include "mpi_utils.h"

static int      num_sim_ranks;      // no. of simulation ranks to monitor
static int      transfersize;       // bytes to copy from ssd->pfs in each call
static int      monpolicy;          // monitoring policy id to implement
static int      monpolicyarg=0;     // input arg to monitoring policy relaxed
static char*    ad_fname = NULL;    // adios output file name
static int      ad_nw = 0;          // no. of adios writers on the node
static subf_t*  mysubfiles;         // list of subfiles assigned to this rank
static int      num_myfiles;        // no. of subfiles assigned to me


int _mainloop() {
    int traf_stat;
    traf_stat = nw_traffic_status();

    while(nw_traffic_status() == GREEN) {
#pragma omp parallel
        {
        }
    }
}


/* ------------------------------------------------------------------------ */
int main(int argc, char **argv) {
    // MPI initializations
    mpi_init(argc, argv);

    // Read input args
    read_input_args(argc, argv, get_grank(), &num_sim_ranks, &transfersize, 
                    &monpolicy, &monpolicyarg, &ad_fname, &ad_nw);

    // Initialize traffic monitor
    mon_init(num_sim_ranks, monpolicy, monpolicyarg);

    // Open input and output ADIOS subfiles
    assign_and_open_local_subfiles(ad_fname, &mysubfiles, &num_myfiles);

    // Start the main loop
    _mainloop();

    // Cleanup
    mon_finalize();
    MPI_Finalize();
    return 0;
}


/*
 * NEXT STEPS:
 * --- DONE --- Distribute data files on the node amongst ranks on the node.
 * --- DONE ---  Need to sort the data files first, or have the local root distribute them
 * --- DONE --- Maintain a struct for the offset of each local subfile
 * Start mainloop that copies data.
 * Put all input args in a config file
 * Set the num_threads somewhere.
 * Implement trigger mechanism.
 */
