#ifndef __LOGGER__
#define __LOGGER__

void log_init(char*);
void log_info(char *, ...);
void log_debug(char *, ...);
void log_error(char *, ...);

#endif

