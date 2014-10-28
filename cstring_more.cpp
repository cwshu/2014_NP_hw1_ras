#include <cstdlib>

#include "cstring_more.h"

char* strncpy_add_null(char* destination, const char* source, size_t num){
    strncpy(destination, source, num);
    destination[num] = '\0';
    return destination;
}
