#ifndef __ASPRINTF_BUFFER_H__
#define __ASPRINTF_BUFFER_H__

typedef asprintf_buffer asprintf_buffer;
struct asprintf_buffer{
    char *buffer_str;
    int size;
};

int asprintf_buffer_init(asprint_buffer* buffer);
int asprintf_to_buf(asprint_buffer* buffer, const char* format, ... );
int asprintf_buffer_free(asprint_buffer* buffer);

#endif
