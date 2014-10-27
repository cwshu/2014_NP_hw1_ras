typedef asprintf_buffer asprintf_buffer;
struct asprintf_buffer{
    char *buffer_str;
    int size;
};

int asprintf_buffer_init(asprint_buffer* buffer){
    buffer->buffer_str = NULL;
    buffer->size = 0;
}

int asprintf_to_buf(asprint_buffer* buffer, const char* format, ... ){
    va_list argptr;
    va_start(argptr, format);

    int size = vsnprintf(buffer->buffer_str, format, buffer->size, argptr);
    if( size > buffer->size ){
        realloc(asprint_buffer->buffer_str, size);
        size = buffer->size;
        vsnprintf(buffer->buffer_str, format, buffer->size, argptr);
    }

    va_end(argptr);
    return size;
}

int asprintf_buffer_free(asprint_buffer* buffer){
    if( buffer->buffer_str )
        free(buffer->buffer_str);
}
