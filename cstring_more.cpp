#include <cstdlib>

#include "cstring_more.h"

int char_belong_to(const char c, const char* char_set){
    int i = 0;
    while(char_set[i]){
        if(c == char_set[i])
            return true;
        i++;
    }
    return false;
}

char* strncpy_add_null(char* destination, const char* source, size_t num){
    strncpy(destination, source, num);
    destination[num] = '\0';
    return destination;
}
