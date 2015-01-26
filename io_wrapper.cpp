#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>

#include <errno.h>
#include <unistd.h>

#include "io_wrapper.h"

/* error output */
void perror_and_exit(const char* str){
    perror(str);
    exit(EXIT_FAILURE);
}

void perror_and_exit(const std::string& str){
    perror(str.c_str());
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
        if( size < 0 )
            return size;
        if( size == 0 )
            return writen_size;

        writen_size += size;
        cur_buf = (const char*)cur_buf + size;
    }
    return writen_size;
}

int write_all_char_array(int fd, const char* str){
    /* str must be zero terminate string */
    size_t count = strlen(str);
    return write_all(fd, str, count);
}

int write_all_str(int fd, const std::string& str){
    return write_all(fd, str.c_str(), str.length());
}

namespace str{
    /* str */ 
    std::string read(int fd, int count, bool is_nonblocking){

        char* buf = new char [count+1];
        int r_size;
        while( 1 ){
            r_size = ::read(fd, buf, count);
            if( r_size < 0 ){
                if( is_nonblocking )
                    if( errno == EAGAIN || errno == EWOULDBLOCK )
                        continue;

                if( errno == ECONNRESET ){
                    perror("read error");
                    return "";
                }

                perror_and_exit("read error");
            }
            break;
        }

        std::string read_str;
        if( r_size ){
            read_str = std::string(buf, r_size);
        }

        delete [] buf;
        return read_str;
    }
}
    
