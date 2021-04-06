#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


int read_config(int *num_sim_ranks, int *transfersize, int *monpolicy, int *monarg, char **adfname, int *ad_nw) {
    char *configfile = "./config.txt";
    char *token;
    char *delim = "=";
    char line[128];
    char *size_mg;

    FILE *file = fopen(configfile, "r");
    if (NULL == file) {
        fprintf(stderr, "Could not open config file.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    while(fgets(line, sizeof(line), file) != NULL) {
        token = strtok(line, delim);
        token = strtok(NULL, delim);
    }

    fclose(file);
    return 0;
}

