#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <mpi.h>


static char* ssd_prefix="./mnt/bb/kmehta/";


static int _dirlist(char* dirpath, char **filelist, int* num_entries) {
    DIR *d;
    struct dirent *d_ent;
    int listindex=0;
    *num_entries = 0;

    d = opendir(dirpath);
    if (NULL == d) {
        perror("opendir");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while((d_ent=readdir(d)) != NULL)
        if(d_ent->d_type == DT_REG)
            if (strncmp(d_ent->d_name, "data", 4) == 0) {
                strcpy(filelist[listindex], d_ent->d_name);
                listindex++;
            }

    closedir(d);
    *num_entries = listindex;

    return 0;
}


static char** _get_data_files(char *adios_fname, int *num_files) {
    int i;
    char** subfiles = NULL;
    
    // malloc subfiles array
    subfiles = (char**) malloc (40*sizeof(char*));
    if (NULL == subfiles) {
        perror("subfiles malloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for(i=0;i<40;i++) {
        subfiles[i] = (char*) malloc (128);
        if (NULL == subfiles[i]) {
            perror("subfiles malloc");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        memset(subfiles[i],0,128);
    }

    // Get list of data files on the node.
    _dirlist(adios_fname, subfiles, num_files);

    if (*num_files == 0) {
        fprintf(stderr, "Could not find any subfiles on node. ABORTING.\n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return NULL;
    }
    return subfiles;
}


static int _openfile(char* adios_dir, char **datafiles, int flags, int *fd, int n) {
    int i;
    
    char**filenames = NULL;
    filenames = (char**) malloc (n*sizeof(char*));
    if (NULL == filenames) {
        perror("Malloc error");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    for(i=0;i<n;i++) {
        filenames[i] = (char*) malloc (128);
        if (NULL == filenames[i]) {
            perror("malloc error");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        memset(filenames[i], 0, 128);
        strcpy(filenames[i], adios_dir);
        strcat(filenames[i], "/");
        strcat(filenames[i], datafiles[i]);
    }

    // Now open the input files for reading/writing
    for(i=0;i<n;i++) {
        fd[i] = open(filenames[i], flags, 0644);
        if(fd[i] < 0) {
            perror("File open error");
            fprintf(stderr, "Could not open %s. ABORTING. \n", filenames[i]);
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    for(i=0;i<n;i++)
        free(filenames[i]);
    free(filenames);

    return 0;
}


/* Open input and output adios subfiles */
int open_subfiles(char *ad_fname, int* fd_in, int* fd_out) {
    int i, ad_nw;
    char** data_files = NULL;

    // Set path to adios file on the local SSD
    char adios_in[128] = {0};
    strcpy(adios_in, ssd_prefix);
    strcat(adios_in, ad_fname);
    
    // Set path to adios file on the pfs
    char adios_out[128] = {0};
    strcpy(adios_out, "./");
    strcat(adios_out, ad_fname);

    // Get list of input adios subfiles
    data_files = _get_data_files(adios_in, &ad_nw);

    // Open subfiles for reading and writing
    _openfile(adios_in,  data_files, O_RDONLY,         fd_in,  ad_nw);
    _openfile(adios_out, data_files, O_CREAT|O_WRONLY, fd_out, ad_nw);

    // Free data_files
    for(i=0;i<ad_nw;i++)
        free(data_files[i]);
    free(data_files);

    return ad_nw;
}


/*
int main() {
    int i, num_files = 0;
    char **subfiles = NULL;

    subfiles = get_subfiles_list("./equilibrium.bp", &num_files);

    for(i=0; i<num_files; i++)
        fprintf(stdout, "%s\n", subfiles[i]);
    return 0;
}
*/

