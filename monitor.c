#include <stdio.h>
#include <stdlib.h>
#include "codes.h"
#include "shm.h"


/* No. of simulation ranks whose status we need to monitor*/
static int num_ranks;


/* Relaxed policy metric.
 * No. of processes that permitted to be in communication for the policy
 * to return GREEN
 */
static int _relaxed_n = 0;


/* Policy implementations for the nw monitoring.
 * Strict  == NO ranks must be involved in communication.
 * Relaxed == n ranks allowed to be involved in communication.
 */
static int _strict_check();
static int _relaxed_check();


static int (*_traffic_status) () = NULL;
static int (*mon_policy[2]) () = { _strict_check, _relaxed_check };


/*
 * Implements a strict check - all ranks must be involved in computation
 * and not communication to return green.
 */
static int _strict_check() {
    //log.debug("strict check\n");

    if (shm_get_green() == num_ranks)
        return GREEN;

    return RED;
}


/*
 * Implements a relaxed check - _relaxed_n ranks may be involved in 
 * communication to return green.
 */
static int _relaxed_check() {
    //log.debug("relaxed check\n");

    if (shm_get_red() <= _relaxed_n)
        return GREEN;

    return RED;
}


/* ------------------------------------------------------------------------ */

/*
 * Monitors if MPI ranks are communicating or computing.
 * According to selected policy, returns green if no network traffic seen, 
 * else returns red.
 */
int nw_traffic_status() {
    return _traffic_status();
}


/*
 * Set which policy must be used to implement network traffic monitoring
 */
int mon_init(int num_ranks, int policyid, int relaxed_n) {
    num_ranks = num_ranks;

    _traffic_status = mon_policy[policyid];
    _relaxed_n = relaxed_n;

    shm_init(num_ranks);
}

int mon_finalize() {
    // Cleanup shared memory segment
    shm_finalize();
}

