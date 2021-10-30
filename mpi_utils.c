#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


static int grank, lrank;
static int gsize, lsize;
static MPI_Comm localcomm;
static char nodename[MPI_MAX_PROCESSOR_NAME];


int mpi_init(int argc, char** argv) {
    int resultlen;
    MPI_Init(&argc, &argv);

    // Global rank and size
    MPI_Comm_rank(MPI_COMM_WORLD, &grank);
    MPI_Comm_size(MPI_COMM_WORLD, &gsize);

    // Local rank and size
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0,
            MPI_INFO_NULL, &localcomm);
    MPI_Comm_rank(localcomm, &lrank);
    MPI_Comm_size(localcomm, &lsize);

    memset(nodename, 0, MPI_MAX_PROCESSOR_NAME);
    MPI_Get_processor_name(nodename, &resultlen);

    return 0;
}

int get_grank() {
    return grank;
}

int get_lrank() {
    return lrank;
}

int get_gsize() {
    return gsize;
}

int get_lsize() {
    return lsize;
}

char* get_nodename() {
    return nodename;
}

MPI_Comm get_lcomm() {
    return localcomm;
}

