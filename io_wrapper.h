#ifndef __IO_WRAPPER_H__
#define __IO_WRAPPER_H__

void perror_and_exit(const char* str);
void error_print(const char* format, ... );
void error_print_and_exit(const char* format, ... );

int write_all(int fd, const void* buf, size_t count);

#endif
