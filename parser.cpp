#include <cstdio>
#include <cstdlib>
#include "cstring_more.h"

#include "io_wrapper.h"
#include "parser.h"

using namespace std;

/* struct redirect */
redirect::redirect(){
    kind = REDIR_NONE; 
    memset(data.filename, 0, 256*sizeof(char));
    data.pipe_index_in_manager = -1;
}

void redirect::set_file_redirect(const char* filename){
    kind = REDIR_FILE;
    int len = strlen(filename);
    strncpy_add_null(data.filename, filename, len);
}

void redirect::set_pipe_redirect(int pipe_index_in_manager){
    kind = REDIR_PIPE;
    data.pipe_index_in_manager = pipe_index_in_manager;
}

/* struct one_cmd */
one_cmd::one_cmd(){
    memset(executable, 0, 256*sizeof(char));
    argv = NULL;
    argv_count = -1;
}

void one_cmd::argv_array_alloc(int size){
    argv = new char*[size]; 
    argv_count = 0;
    this->size = size;
}

void one_cmd::add_executable(const char* executable_name){
    int len = strlen(executable_name);
    strncpy_add_null(executable, executable_name, len);
    argv_array_alloc();
    add_argv(executable_name);
}

void one_cmd::add_argv(const char* argument){
    int len = strlen(argument);
    argv[argv_count] = new char[len];
    strncpy_add_null(argv[argv_count], argument, len);
    argv_count += 1;
    argv[argv_count] = NULL; /* NULL-terminate for exec */
}

one_cmd::~one_cmd(){
    if( argv ){
        for(int i=0; i<argv_count; i++){
            delete [] argv[i];
        }
        delete [] argv;
    }
}

/* struct one_line_cmd */
one_line_cmd::one_line_cmd(){
    cmd_count = 0;
    input_redirect = redirect();
    last_output_redirect = redirect();
}

void one_line_cmd::set_fileio_redirect(const char* str, int len){
    char op = str[0];
    str += 1;
    const char* filename = str + strspn(str, WHITESPACE);
    const char* filename_end = filename + strcspn(filename, WHITESPACE);

    if( filename >= str+len-1 )
        perr_and_exit("fileio redirect error:%c%s\n", op, str);
    if( filename_end == NULL || filename_end > str+len-1 )
        filename_end = str+len-1;

    int filename_len = filename_end - filename;
    if( filename_len == 0 )
        perr_and_exit("fileio redirect error:%c%s\n", op, str);

    char filename_cp[256] = {0};
    strncpy_add_null(filename_cp, filename, filename_len);
    if( op == '<' ){
        if(input_redirect.kind != REDIR_NONE)
            perr_and_exit("more than one input redirection error\n");
        input_redirect.set_file_redirect(filename_cp);
    }
    else if( op == '>' ){
        if( last_output_redirect.kind != REDIR_NONE )
            perr_and_exit("more than one output redirection error\n");
        last_output_redirect.set_file_redirect(filename_cp);
    }
    else{
        perr_and_exit("io_redirection mistake op %c\n", op);
    }
}

void one_line_cmd::next_cmd(){
    cmd_count++;
}

void one_line_cmd::add_executable(const char* executable_name){
    cmds[cmd_count].add_executable(executable_name);
}

void one_line_cmd::add_argv(const char* argument){
    cmds[cmd_count].add_argv(argument);
}

void one_line_cmd::add_pipe_redirect(int pipe_index_in_manager){
    output_redirect[cmd_count].set_pipe_redirect(pipe_index_in_manager);
}

void one_line_cmd::print(){
    printf("input_redirect: ");
    if(input_redirect.kind == REDIR_NONE){
        printf("None\n");
    }
    else{
        printf("%s\n", input_redirect.data.filename);
    }
    printf("last_output_redirect: ");
    if(last_output_redirect.kind == REDIR_NONE){
        printf("None\n");
    }
    else{
        printf("%s\n", last_output_redirect.data.filename);
    }

    printf("command count: %d\n", cmd_count);
    for(int i=0; i<cmd_count; i++){
        printf("exe: %s\n", cmds[i].executable);
        for(int j=0; j<cmds[i].argv_count; j++){
            printf("args: %s\n", cmds[i].argv[j]);
        }
        printf("pipe_index_in_manager: %d\n", output_redirect[i].data.pipe_index_in_manager);
    }
}

int parsing_command(struct one_line_cmd* parsed_cmd, char* command){
    char* current_cmd = command;

    current_cmd += strspn(current_cmd, WHITESPACE); /* strip left whitespaces */

    char* first_io_idx = strpbrk(current_cmd, FILE_REDIR_CHARS);
    char* io_idx = first_io_idx;
    while( io_idx ){
        char* next_io_idx = strpbrk(io_idx+1, FILE_REDIR_CHARS);
        int len;
        if( next_io_idx )
            len = next_io_idx - io_idx;
        else
            len = strlen(io_idx);

        parsed_cmd->set_fileio_redirect(io_idx, len);
        io_idx = next_io_idx;
    }

    if( first_io_idx )
        first_io_idx[0] = '\0';

    char* split_str = current_cmd;
    while(1){
        int end_of_cmd = false;

        if( parsed_cmd->cmds[parsed_cmd->cmd_count].argv_count <= 0 ){
            char* exe_name = strtok(split_str, WHITESPACE);
            split_str = NULL;
            if( !exe_name )
                break;
            parsed_cmd->add_executable(exe_name);
        }

        while(1){
            char* argument = strtok(NULL, WHITESPACE);
            if( !argument ){
                parsed_cmd->cmd_count += 1;
                end_of_cmd = true;
                break;
            }

            char* pipe_str = strpbrk(argument, "|");
            if( !pipe_str ){
                parsed_cmd->add_argv(argument);
                continue;
            }

            char arg_bef_pipe[256];
            strncpy_add_null(arg_bef_pipe, argument, pipe_str - argument);
            parsed_cmd->add_argv(arg_bef_pipe);
            char* cmd_after_pipe = NULL;
            if( pipe_str[1] >= '0' && pipe_str[1] <= '9' ){
                /* |[0-9] => pipe + number */
                char *end_of_num = NULL;
                int pipe_index_in_manager = strtol(pipe_str+1, &end_of_num, 10);
                if(pipe_index_in_manager == 0){
                    end_of_num[0] = '\0';
                    perr_and_exit("pipe error: %s\n", pipe_str);
                }
                parsed_cmd->add_pipe_redirect(pipe_index_in_manager);
                cmd_after_pipe = end_of_num;
            }
            else{
                /* only pipe without number */
                parsed_cmd->add_pipe_redirect(1);
                cmd_after_pipe = pipe_str+1;
            }
            parsed_cmd->cmd_count += 1;

            cmd_after_pipe = cmd_after_pipe + strspn(cmd_after_pipe, WHITESPACE);
            if( cmd_after_pipe[0] != '\0' ){
                parsed_cmd->add_executable(cmd_after_pipe);
            }
            break;
        }

        if( end_of_cmd )
            break;
    }
    return 0;
}
