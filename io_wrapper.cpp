#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include <unistd.h>

#include "io_wrapper.h"

/* error output */
void perror_and_exit(const char* str){
    perror(str);
    exit(EXIT_FAILURE);
}

void error_print(const char* format, ... ){
    /* print format string to stderr */
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

void error_print_and_exit(const char* format, ... ){
    /* print format string to stderr */
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    /* exit */
    exit(EXIT_FAILURE);
}

/* write system call */
int write_all(int fd, const void* buf, size_t count){
    size_t writen_size = 0;
    const void* cur_buf = buf;
    while(writen_size < count){
        int size = write(fd, buf, count - writen_size);
        if(size < 0)
            return size;

        writen_size += size;
        cur_buf = (const char*)cur_buf + size;
    }
    return writen_size;
}
