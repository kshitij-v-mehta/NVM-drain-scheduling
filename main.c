#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <omp.h>
#include "codes.h"
#include "monitor.h"
#include "utils.h"
#include "file_utils.h"
#include "copier.h"
#include "mpi_utils.h"
#include "logger.h"
#include "draining.h"

static int      num_sim_ranks;      // no. of simulation ranks to monitor
static int      transfersize;       // bytes to copy from ssd->pfs in each call
static int      monpolicy;          // monitoring policy id to implement
static int      monpolicyarg=0;     // input arg to monitoring policy relaxed
static char*    ad_fname = NULL;    // adios output file name
static int      ad_nw = 0;          // no. of adios writers on the node
static subf_t*  mysubfiles;         // list of subfiles assigned to this rank
static int      num_myfiles;        // no. of subfiles assigned to me
static int      num_threads;        // no. of threads in each drainer process
static char     nvm_prefix[128];    // Path to the NVM. e.g. /mnt/bb/kmehta
static int      drain_type;         // Coordinated or independent draining

#define TWOGB  2147483647


int _mainloop() {
    //trigger_check();
    //execute_

    int *copy_status = NULL;
    int curState = RED;
    int allcopied = 0;
    char *sizemg;
    int i;
    int nt;
    
    nt = (int)strtol(getenv("OMP_NUM_THREADS"), &sizemg, 10);

    // Allocate array to hold the copy status for each thread
    copy_status = (int*) malloc (nt*sizeof(int));
    if (NULL == copy_status) {
        perror("Could not alloc internal int array for threads. ABORTING.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for(i=0; i<nt; i++)
        copy_status[i] = -1;


#pragma omp parallel
    {
        if( (get_grank() == 0) && (omp_get_thread_num() == 0) )
            log_info("Num threads set to %d\n", omp_get_num_threads());
    }

    // Start flushing
    while( (nw_traffic_status() != EXIT_DONE) )
        if(drain_type != POSTHOC_DRAIN)
            drain(mysubfiles, num_myfiles, transfersize);

    // Main app has exited. Flush remaining data.
    log_info("EXIT_DONE received\n");
    while(!allcopied) {
#pragma omp parallel
        {
            transfersize = TWOGB;
            copy_status[omp_get_thread_num()] = 
                copy_step(mysubfiles, num_myfiles, transfersize);
            log_info("Copied %d bytes\n", copy_status[omp_get_thread_num()]);
        }

        allcopied = 1;
        for(i=0; i<nt; i++)
            if (copy_status[i] != 0) allcopied = 0;
    }

    // Node-local roots look for and copy the adios metadata file
    // mdx and profiling.json
    //if(get_lrank() == 0) copy_adios_md();

    free(copy_status);
}


/* ------------------------------------------------------------------------ */
int main(int argc, char **argv) {
    // MPI initializations
    mpi_init(argc, argv);

    // Read input args
    read_input_args(argc, argv, get_grank(), &num_sim_ranks, &transfersize, 
                    &monpolicy, &monpolicyarg, &ad_fname, &ad_nw, nvm_prefix,
                    &drain_type);
 
    // Init logging information
    log_init(argv[0]);

    // Set the draining method
    set_drain_type(drain_type);

    // Initialize traffic monitor
    mon_init(num_sim_ranks, monpolicy, monpolicyarg);

    // Set the prefix to the NVM. e.g. /mnt/bb/kmehta
    file_utils_init(nvm_prefix);

    // Open input and output ADIOS subfiles
    assign_and_open_local_subfiles(ad_fname, &mysubfiles, &num_myfiles, ad_nw);

    // Start the main loop
    _mainloop();

cleanup:
    log_info("All done. Goodbye.\n");
    // Cleanup
    mon_finalize();
    MPI_Finalize();
    return 0;
}


/*
 *  NEXT STEPS:
 *  --- DONE --- Distribute data files on the node amongst ranks on the node
 *  --- DONE --- Local root distributes local data filenames
 *  --- DONE --- Maintain a struct for the offset of each local subfile
 *  --- DONE --- Start mainloop that copies data.
 *  --- DONE --- Distributes files of a rank amongst its threads
 *  --- DONE --- Threads copy data
 *  --- DONE --- Set the num_threads somewhere
 *  --- DONE --- Add logger
 *  --- DONE --- Test concurrent writing to ssd and flushing
 *  --- DONE --- Fix subfiles not being found if drainer launched too early
 *  --- DONE --- Remove hard-coded nvm prefix and take it as cmd line arg
 *  --- DONE --- Implement and select different draining methods
 *  Add options to setup continuous and periodic draining
 *  Add profile timers and option to log into separate files for ranks
 *  Clearly state the max. no. of subfiles allowed on a node
 *  Write code to flush multiple bp files from a node
 *  Create public traffic status library
 *  Copy md.idx
 *  Develop trigger mechanism
 *  Maintain a table of computational kernels with index=filename:linenumber
 *   and value=runtime for ml
 */

