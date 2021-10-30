#ifndef __MPI_UTILS__
#define __MPI_UTILS__


int mpi_init(int argc, char** argv);
int get_grank();
int get_lrank();
int get_gsize();
int get_lsize();
char* get_nodename();
MPI_Comm get_lcomm();

#endif

