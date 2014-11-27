#ifndef __PARSER_H__
#define __PARSER_H__

#include <vector>
#include <string>
using namespace std;

enum RedirectionType{
    REDIR_NONE,
    REDIR_FILE,
    REDIR_PIPE,
};

const char WHITESPACE[] = " \t\r\n\v\f";
const char REDIRECTION_CHARS[] = "|><";
const char FILE_REDIR_CHARS[] = "><";

struct Redirection{
    RedirectionType kind;
    union {
        string filename;
        int person_id;
        int pipe_index_in_manager;
    } data;

    Redirection();
    void set_file_redirect(string filename);
    void set_file_redirect(int person_id);
    void set_pipe_redirect(int pipe_index_in_manager);
};

struct SingleCommand{
    string executable;
    vector<string> arguments;
    int args_count;
    Redirection std_input; // input redirection are both depend on here and PipeManager.
    Redirection std_output;
    Redirection std_error;

    SingleCommand();
    // void argv_array_alloc(int size = 256);
    void add_executable(string executable_name);
    void add_argv(string argument);
    char** gen_argv();
    ~SingleCommand();
};

struct OneLineCommand{
    vector<SingleCommand> cmds;
    int cmd_count;

    OneLineCommand();

    SingleCommand& current_cmd();
    void next_cmd();
    void add_executable(string executable_name);
    void add_argv(string argument);
    void print();

    int parse_one_line_cmd(string command_str);
    int parse_single_command(string command_str);
    int parse_redirection(string command_str);
    /* return: NEXT_IS_CMD, NEXT_IS_REDIR_CHARS, NO_NEXT, CMD_ERROR */
};
const int CMD_ERROR = -1;
const int NO_NEXT = 0;
const int NEXT_IS_CMD = 1;
const int NEXT_IS_REDIR_CHARS = 2;

#endif
