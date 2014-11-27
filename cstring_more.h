#ifndef __C_STRING_MORE_H__
#define __C_STRING_MORE_H__

#include <cstring>
using namespace std;

int char_belong_to(char c, char* char_set);
char* strncpy_add_null(char* destination, const char* source, size_t num);

#endif
