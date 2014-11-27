#include "string_more.h"

std::string lstrip(const std::string& str){
    /* strip left whitespaces */
    std::size_t found = str.find_first_not_of(WHITESPACE);
    if( found == std::string::npos ) 
        // nothing more than WHITESPACE
        return std::string();
    return str.substr(found, std::string::npos);
}

std::string rstrip(const std::string& str){
    /* strip right whitespaces */
    std::size_t found = str.find_last_not_of(WHITESPACE);
    if( found == std::string::npos ) 
        // nothing more than WHITESPACE
        return std::string();
    return str.substr(0, found+1);
}

std::string strip(const std::string& str){
    /* strip left and right whitespaces */
    std::size_t start = str.find_first_not_of(WHITESPACE);
    std::size_t end = str.find_last_not_of(WHITESPACE);
    if( start == std::string::npos )
        // nothing more than WHITESPACE
        return std::string();
    return str.substr(start, end + 1 - start);
}
