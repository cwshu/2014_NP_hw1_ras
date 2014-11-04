#ifndef __PARSER_H__
#define __PARSER_H__

enum redirect_type{
    REDIR_NONE,
    REDIR_FILE,
    REDIR_PIPE,
};

const char WHITESPACE[] = " \t\r\n\v\f";
const char SHELL_SPECIAL_CHARS[] = "|><";
const char FILE_REDIR_CHARS[] = "><";
const char PIPE_CHARS[] = "|!";

struct redirect{
    redirect_type kind;
    union {
        char filename[256];
        int pipe_index_in_manager;
    } data;

    redirect();
    void set_file_redirect(const char* filename);
    void set_pipe_redirect(int pipe_index_in_manager);
};

struct one_cmd{
    char executable[256];
    char** argv;
    int argv_count;
    int size;

    one_cmd();
    void argv_array_alloc(int size = 256);
    void add_executable(const char* executable_name);
    void add_argv(const char* argument);
    ~one_cmd();
};

struct one_line_cmd{
    one_cmd cmds[4096];
    redirect output_redirect[4096]; // must be pipe
    redirect stderr_redirect[4096]; // must be pipe
    int cmd_count;
    redirect input_redirect;
    redirect last_output_redirect;

    one_line_cmd();
    int parse_one_line_cmd(char* command);
    void set_fileio_redirect(const char* str, int len);
    void next_cmd();
    void add_executable(const char* executable_name);
    void add_argv(const char* argument);
    void add_stdout_pipe_redirect(int pipe_index_in_manager);
    void add_stderr_pipe_redirect(int pipe_index_in_manager);
    void print();
};

#endif
