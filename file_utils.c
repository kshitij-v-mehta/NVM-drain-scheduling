#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <mpi.h>
#include "file_utils.h"
#include "mpi_utils.h"
#include "logger.h"

//TODO
static char _nvm_prefix[128];

/*
 * Return a list of data subfiles from the adios container represented
 * by dirpath
 */
static int _dirlist(char* dirpath, char **filelist, int* num_entries) {
    DIR *d;
    struct dirent *d_ent;
    int listindex=0;
    *num_entries = 0;

    d = opendir(dirpath);
    if (NULL == d) {
        perror("opendir");
        log_error("Could not open %s\n", dirpath);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while((d_ent=readdir(d)) != NULL)
        if(d_ent->d_type == DT_REG)
            if (strncmp(d_ent->d_name, "data", 4) == 0) {
                strcpy(filelist[listindex], d_ent->d_name);
                log_debug("found %s\n", d_ent->d_name);
                listindex++;
            }

    closedir(d);
    *num_entries = listindex;

    return 0;
}


/*
 * Allocate a 2D array for holding list of n data subfiles
 */
static char** _allocate_subfiles(int n) {
    int i;
    char **subfiles = NULL;

    subfiles = (char**) malloc (n*sizeof(char*));
    if (NULL == subfiles) {
        perror("malloc");
        log_error("Could not allocate subfiles. ABORTING.\n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return NULL;
    }

    for(i=0; i<n; i++) {
        subfiles[i] = (char*) malloc(32);
        if (NULL == subfiles[i]) {
            perror("malloc");
            log_error("Could not allocate subfiles. ABORTING.\n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return NULL;
        }
        memset(subfiles[i],0,32);
    }

    return subfiles;
}


/*
 * Get a list of adios data files under the input adios container
 */
static char** _get_data_files(char *adios_fname, int *num_files, int fpn) {
    int i;
    char** subfiles = NULL;
    char fullpath[128] = {0};

    strcpy(fullpath, _nvm_prefix);
    strcat(fullpath, "/");
    strcat(fullpath, adios_fname);

    // malloc subfiles array
    subfiles = _allocate_subfiles(40);  //TODO

    // Get list of data files on the node.
    log_debug("Looking for subfiles in %s\n", fullpath);
    while(*num_files < fpn)
        _dirlist(fullpath, subfiles, num_files);
    if(get_lrank() == 0) log_info("found %d files on node\n", *num_files);

    if (*num_files == 0) {
        log_error("Could not find any subfiles on node. ABORTING.\n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return NULL;
    }
    return subfiles;
}


/*
 * Node-local root gets list of data subfiles from the SSD and distributes
 * them evenly amongst all ranks.
 * fpn - num subfiles expected per node
 */
char** _root_distribute_subfiles(char* adios_fname, int *num_myfiles, int fpn) {
    char** allsubfiles = NULL;
    char** subfiles = NULL;
    int i,j, num_files_on_node, n;
    int startindex=0;

    allsubfiles =  _get_data_files(adios_fname, &num_files_on_node, fpn);
    
    // Distribute subfiles to other ranks on node
    for(i=0; i<get_lsize(); i++) {
    
        // Distribute files equally. Remainder files also get distributed
        // equally.
        n = num_files_on_node/get_lsize() + (i<(num_files_on_node%get_lsize()));
    
        // Rank 0 does not send to itself
        if (i==0) {
            *num_myfiles = n;
            subfiles = _allocate_subfiles(*num_myfiles);
            for (j=0; j<*num_myfiles; j++) {
                strcpy(subfiles[j], allsubfiles[j]);
                log_debug("assigned subfile %s to self\n", allsubfiles[j]);
            }
            startindex += n;

            log_info("%d num subfiles assigned\n", *num_myfiles);
            continue;
        }
    
        // Send how many files you are going to send
        MPI_Send(&n, 1, MPI_INT, i, 0, get_lcomm());
    
        // Send the filenames
        for(j=0; j<n; j++) {
            MPI_Send(allsubfiles[startindex++], 32, MPI_CHAR, i, 0, get_lcomm());
            log_debug("sent subfile %s to %d\n", allsubfiles[startindex-1], i);
        }
    }

    // Free allsubfiles
    for(i=0;i<num_files_on_node;i++)
        free(allsubfiles[i]);
    free(allsubfiles);

    return subfiles;
}

/*
 * Non-root ranks on the node receive their assigned subfile names from
 * the root.
 */
char** _recv_subfiles(int *num_myfiles) {
    int i;
    char** subfiles = NULL;
    MPI_Status status;

    MPI_Recv(num_myfiles, 1, MPI_INT, 0, 0, get_lcomm(), &status);

    subfiles = _allocate_subfiles(*num_myfiles);
    for(i=0; i<*num_myfiles; i++)
        MPI_Recv(subfiles[i], 32, MPI_CHAR, 0, 0, get_lcomm(), &status);

    log_info("%d num files assigned\n", *num_myfiles);
    for(i=0; i<*num_myfiles; i++)
        log_debug("received subfile %s\n", subfiles[i]);

    return subfiles;
}


/*
 * Initialize the subf_t entry for a data subfile.
 * Sets the input (ssd) and output filenames (pfs)
 * Opens the ssd file for reading, and the pfs file for writing
 */
int _create_subft_entry(subf_t* t, char* datafilename, char* adiosfname) {
    char infname[128] = {0}, outfname[128] = {0};
    
    // Set the full input ssd fname
    strcpy(infname, _nvm_prefix);
    strcat(infname, "/");
    strcat(infname, adiosfname);
    strcat(infname, "/");
    strcat(infname, datafilename);
    memset((char*)t->fname_ssd, 0, 128);
    strcpy(t->fname_ssd, infname);

    // Set the output pfs fname
    strcpy(outfname, "./");
    strcat(outfname, adiosfname);
    strcat(outfname, "/");
    strcat(outfname, datafilename);
    memset(t->fname_pfs, 0, 128);
    strcpy(t->fname_pfs, outfname);

    log_info("opening '%s' for reading\n", infname);

    // Open file on ssd for reading
    if(-1 == (t->fd_in = open(infname, O_RDONLY, 0644))) {
        perror("open failed");
        log_error("Could not open %s for reading. ABORTING.\n", infname);
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    log_info("opening '%s' for writing\n", outfname);
    // Open file on pfs for writing
    if(-1 == (t->fd_out = open(outfname, O_CREAT|O_WRONLY, 0644))) {
        perror("open");
        log_error("Could not open %s for writing. ABORTING.\n", infname);
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    // Set the offset to start of file
    t->offset = 0;

    return 0;
}


/*
 * Create an array of subf_t structs for your assigned subfiles and open the
 * subfiles for reading the corresponding output subfiles for writing.
 */
int _form_subfile_t(char* adiosfname, subf_t** mysubfiles,
        char** subfilenames, int num_myfiles) {
    int i;
    char infname[128], outfname[128];
    *mysubfiles = (subf_t*) malloc (num_myfiles * sizeof(subf_t));
    if(NULL == *mysubfiles) {
        perror("malloc");
        log_error("Could not allocate subf_t array. ABORTING.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    for(i=0; i<num_myfiles; i++)
        _create_subft_entry(&(*mysubfiles)[i], subfilenames[i], adiosfname);

    // free subfilenames
    for(i=0; i<num_myfiles; i++)
        free(subfilenames[i]);
    free(subfilenames);
}


/*---------------------------------------------------------------------------*/
/* Set the path to the NVM location. e.g. /mnt/bb/kmehta
 */
void file_utils_init(char *nvm_prefix) {
    memset((void*)_nvm_prefix, 0, strlen(_nvm_prefix));
    strcpy(_nvm_prefix,nvm_prefix);
}

/*
 * Local root sees what subfiles are available on the local ssd (data*).
 * It distributes them evenly amongst ranks.
 * Everyone then creates an array of subfile_t structs for their assigned
 * files and opens them for reading.
 * fpn = subfiles per node
 */
int assign_and_open_local_subfiles(char* adiosfname, subf_t** mysubfiles, 
        int* num_myfiles, int fpn) {
    int i, j, start_index=0, n_assigned;
    int num_files_on_node = 0;
    char** subfilenames= NULL;
    MPI_Status status;

    // Distributes data files on the node amongst all ranks
    *num_myfiles = 0;
    if(get_lrank() == 0)
        subfilenames = _root_distribute_subfiles(adiosfname, num_myfiles, fpn);
    else
        subfilenames = _recv_subfiles(num_myfiles);

    // Create the subfile_t array of structs
    _form_subfile_t(adiosfname, mysubfiles, subfilenames, *num_myfiles);

    return 0;
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

