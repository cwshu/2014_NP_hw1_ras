#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "io_wrapper.h"

/* error output */
void perr(const char* format, ...){
    /* print format string to stderr */
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

void perr_and_exit(const char* format, ...){
    /* print format string to stderr */
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    /* exit */
    exit(EXIT_FAILURE);
}
