#include "string_more.h"

std::string fetch_word(std::string& parsed_str, const char* split_chars){
    /* fetch the next "word" in parsed_str, and cut this word part in parsed_str.
     * the "word" means char sequence before split_chars;
     *
     * ex. 
     * split_chars: rn;
     * parsed_str   => return_string, parsed_str_after
     * "rrrnn"      => ""           , ""
     * "rraarnbbbr" => "aa"         , "nbbbr"
     * "rraaabbbrn" => "aaabbb"     , ""
     */
    std::size_t start = parsed_str.find_first_not_of(split_chars);
    std::size_t end = parsed_str.find_first_of(split_chars, start);
    // std::size_t next_start = parsed_str.find_first_not_of(split_chars, end);
    if( start == std::string::npos ){
        /* no word, parsed_str only consist of split_chars  */
        parsed_str.clear();
        return std::string();
    }

    std::string word;
    if( end == std::string::npos ){
        word = parsed_str.substr(start, std::string::npos);
        parsed_str.clear();
    }
    else{
        word = parsed_str.substr(start, end - start);
        parsed_str.erase(0, end + 1);
    }

    return word;
}

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
