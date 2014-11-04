#ifndef __PARSER_H__
#define __PARSER_H__

enum RedirectionType{
    REDIR_NONE,
    REDIR_FILE,
    REDIR_PIPE,
};

const char WHITESPACE[] = " \t\r\n\v\f";
const char SHELL_SPECIAL_CHARS[] = "|><";
const char FILE_REDIR_CHARS[] = "><";

struct Redirection{
    RedirectionType kind;
    union {
        char filename[256];
        int pipe_index_in_manager;
    } data;

    Redirection();
    void set_file_redirect(const char* filename);
    void set_pipe_redirect(int pipe_index_in_manager);
};

struct SingleCommand{
    char executable[256];
    char** argv;
    int argv_count;
    int size;

    SingleCommand();
    void argv_array_alloc(int size = 256);
    void add_executable(const char* executable_name);
    void add_argv(const char* argument);
    ~SingleCommand();
};

struct OneLineCommand{
    SingleCommand cmds[4096];
    Redirection output_redirect[4096]; // must be pipe
    int cmd_count;
    Redirection input_redirect;
    Redirection last_output_redirect;

    OneLineCommand();
    int parse_one_line_cmd(char* command);
    void set_fileio_redirect(const char* str, int len);
    void next_cmd();
    void add_executable(const char* executable_name);
    void add_argv(const char* argument);
    void add_pipe_redirect(int pipe_index_in_manager);
    void print();
};

#endif
