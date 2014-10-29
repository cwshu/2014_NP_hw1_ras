#ifndef __IO_WRAPPER_H__
#define __IO_WRAPPER_H__

void perr(const char* format, ...);
void perr_and_exit(const char* format, ...);

int write_all(int fd, const void* buf, size_t count);

#endif
