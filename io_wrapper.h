#ifndef __IO_WRAPPER_H__
#define __IO_WRAPPER_H__

#include <string>

void perror_and_exit(const char* str);
void perror_and_exit(const std::string& str);

void error_print(const char* format, ... );
void error_print_and_exit(const char* format, ... );

int write_all(int fd, const void* buf, size_t count);

namespace str{
    std::string read(int fd, int count, bool is_nonblocking);
}

#endif
