#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include "codes.h"
#include "monitor.h"
#include "utils.h"
#include "file_utils.h"
#include "copier.h"
#include "mpi_utils.h"
#include "logger.h"

static int      num_sim_ranks;      // no. of simulation ranks to monitor
static int      transfersize;       // bytes to copy from ssd->pfs in each call
static int      monpolicy;          // monitoring policy id to implement
static int      monpolicyarg=0;     // input arg to monitoring policy relaxed
static char*    ad_fname = NULL;    // adios output file name
static int      ad_nw = 0;          // no. of adios writers on the node
static subf_t*  mysubfiles;         // list of subfiles assigned to this rank
static int      num_myfiles;        // no. of subfiles assigned to me
static int      num_threads;        // no. of threads in each drainer process


int _mainloop() {

#pragma omp parallel
    {
        if( (get_grank() == 0) && (omp_get_thread_num() == 0) )
            log_info("Num threads set to %d\n", omp_get_num_threads());
    }

    while(nw_traffic_status() == GREEN) {
#pragma omp parallel
        {
            copy_step(mysubfiles, num_myfiles, transfersize);
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
 
    // Init logging information
    log_init();

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
 * --- DONE --- Distribute data files on the node amongst ranks on the node
 * --- DONE --- Local root distributes local data filenames
 * --- DONE --- Maintain a struct for the offset of each local subfile
 * --- DONE --- Start mainloop that copies data.
 * --- DONE --- Distributes files of a rank amongst its threads
 * Threads copy data
 * Write trigger code
 * Set the num_threads somewhere.
 * Copy md.idx
 * Put all input args in a config file
 * Add debugging information
 */
