#ifndef __FILEUTILS__
#define __FILEUTILS__


typedef struct _subfile_t {
    char fname_ssd[128];
    char fname_pfs[128];
    int fd_in;
    int fd_out;
    off_t offset;
} subf_t;

int assign_and_open_local_subfiles(char*, subf_t**, int*, int); 

#endif

