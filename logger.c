#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <string.h>
#include <stdarg.h>
#include "mpi_utils.h"


#define LOG_MSG \
    do {                                                \
        char str[64] = {0};                             \
        sprintf(str, "Rank %d writing ", get_lrank());  \
        strcat(str,s);                                  \
        va_list args;                                   \
        va_start(args, s);                              \
        vprintf(str, args);                             \
        va_end(args);                                   \
    } while(0);


#define LEVEL_NONE  0
#define LEVEL_DEBUG 1
#define LEVEL_INFO  2

static int log_level;
char* log_str[] = {"NONE", "DEBUG", "INFO"};

void log_init() {
    log_level = LEVEL_INFO;
    char *s = getenv("DRAINER_LOG_LEVEL");
    if(strlen(s) > 0) {
        if (strcmp(s, "INFO")  == 0) log_level = LEVEL_INFO;
        if (strcmp(s, "DEBUG") == 0) log_level = LEVEL_DEBUG;
        if (strcmp(s, "NONE")  == 0) log_level = LEVEL_NONE;
    }
    fprintf(stdout, "Log level set to %s\n", log_str[log_level]);
}

void log_info(char *s, ...) {
    if (log_level < LEVEL_INFO) return;
    LOG_MSG
}

void log_debug(char *s, ...) {
    if (log_level < LEVEL_DEBUG) return;
    LOG_MSG
}

