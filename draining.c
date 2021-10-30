#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include "logger.h"
#include "mpi_utils.h"
#include "file_utils.h"
#include "copier.h"
#include "monitor.h"
#include "codes.h"


static int curState = RED;
static int (*_drain) (subf_t*, int, int) = NULL;


/*
 * Drain the NVM by coordinating draining with computation in the 
 * simulation.
 */
int _coordinated_drain(subf_t* mysubfiles, int num_myfiles,
        int transfersize) {

    while( (nw_traffic_status() == GREEN) ) {

        // To minimize logging outputs
        if(curState != GREEN) {
            log_info("Traffic Green\n");
            curState = GREEN;
        }

#pragma omp parallel
        {
            copy_step(mysubfiles, num_myfiles, transfersize);
        }
    }
    // To minimize logging outputs
    if(curState != RED) {
        log_info("Traffic Red\n");
        curState = RED;
    }

    return 0;
}


/*
 * Independent draining drains the NVM periodically, without
 * any coordination of computation in the main simulation
 */

int _independent_drain(subf_t* mysubfiles, int num_myfiles,
        int transfersize) {
    
    // Sleep for 10 milliseconds
    usleep(10000);
    copy_step(mysubfiles, num_myfiles, transfersize);

    return 0;
}


/*
 * Function pointer that points to the actual draining function
 */
int drain(subf_t* mysubfiles, int num_myfiles, int transfersize) {
    _drain(mysubfiles, num_myfiles, transfersize);
}


/*
 * Set the function pointer to point to the right draining method
 */
int set_drain_type(int drain_type) {
    if(drain_type == 1)
        _drain = _coordinated_drain;
    else
        _drain = _independent_drain;
}

