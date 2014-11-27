#include <cstdio>
#include <cstdlib>

#include "cstring_more.h"
#include "io_wrapper.h"
#include "parser.h"

using namespace std;

/* struct Redirection */
Redirection::Redirection(){
    kind = REDIR_NONE; 
    filename = string();
    data.pipe_index_in_manager = -1;
}

void Redirection::set_file_redirect(string filename){
    kind = REDIR_FILE;
    data.filename = filename;
}

void Redirection::set_file_redirect(int person_id){
    kind = REDIR_FILE;
    data.person_id = person_id;
}

void Redirection::set_pipe_redirect(int pipe_index_in_manager){
    kind = REDIR_PIPE;
    data.pipe_index_in_manager = pipe_index_in_manager;
}

/* struct SingleCommand */
SingleCommand::SingleCommand(){
    executable = string();
    arguments = vector();
    args_count = 0;
}   

/*
void SingleCommand::argv_array_alloc(int size){
    argv = new char*[size]; 
    args_count = 0;
    this->size = size;
}*/

void SingleCommand::add_executable(string executable_name){
    executable = executable_name;
    add_argv(executable_name);
}

void SingleCommand::add_argv(string argument){
    argv.push_back(argument);
    args_count += 1;
}

char** SingleCommand::gen_argv(){
    char** argv;
    argv = new char* [args_count+1];
    for( int i=0; i<args_count; i++ ){
        argv[i] = new char [argument[i].length()+1];
        strncpy(argv[i], argument[i].c_str(), argument.length()+1);
    }
    argv[args_count] = NULL;
    return argv;
}

void SingleCommand::free_argv(char** argv){
    int index = 0;
    while( argv[index] != NULL ){
        delete [] argv[index];
        index += 1;
    }
    delete [] argv;
}

SingleCommand::~SingleCommand(){
    if( argv ){
        for( int i=0; i<args_count; i++ ){
            delete [] argv[i];
        }
        delete [] argv;
    }
}

/* struct OneLineCommand */
OneLineCommand::OneLineCommand(){
    cmds = vector();
    cmd_count = 0;
}

SingleCommand& OneLineCommand::current_cmd(){
    return cmds[cmd_count];
}

void OneLineCommand::next_cmd(){
    cmd_count++;
}

void OneLineCommand::add_executable(string executable_name){
    cmds[cmd_count].add_executable(executable_name);
}

void OneLineCommand::add_argv(string argument){
    cmds[cmd_count].add_argv(argument);
}

void OneLineCommand::print(){
    printf("input_redirect: ");
    if( input_redirect.kind == REDIR_NONE ){
        printf("None\n");
    }
    else{
        printf("%s\n", input_redirect.data.filename);
    }
    printf("last_output_redirect: ");
    if( last_output_redirect.kind == REDIR_NONE ){
        printf("None\n");
    }
    else{
        printf("%s\n", last_output_redirect.data.filename);
    }

    printf("command count: %d\n", cmd_count);
    for( int i=0; i<cmd_count; i++ ){
        printf("exe: %s\n", cmds[i].executable);
        for( int j=0; j<cmds[i].args_count; j++ ){
            printf("args: %s\n", cmds[i].argv[j]);
        }
        printf("pipe_index_in_manager: %d\n", output_redirect[i].data.pipe_index_in_manager);
    }
}

int OneLineCommand::parse_single_command(string command_str){
    /* parsing single command, stop at pipe or file redirection or end-of-string(line).
     * single command: executable and arguments.
     *
     * return value: number of arguments, 0 for error, 1 for only executable.
     */

    /* strip left whitespaces */
    std::size_t found = command_str.find_first_not_of(WHITESPACE) 
    if( found == string::npos )
        return 0; // No Command
    command_str = command_str.substr(found, string::npos);

    /* parse executable name */
    found = command_str.find_first_of(WHITESPACE);
    if( found == string::npos )
        return 0; // No Command
    string exe_name = command_str.substr(0, found);
    command_str = command_str.substr(found, string::npos);
    this->add_executable(exe_name);
    
    /* parse arguments */
    while( 1 ){
        std::size_t arg_start = command_str.find_first_not_of(WHITESPACE)
        std::size_t arg_end = command_str.find_first_of(WHITESPACE, arg_start);
        if( arg_start == string::npos ){
            /* stop at end of line */
            command_str = "";
            return cmd[cmd_count].args_count;
        }

        string argument = command_str.substr(arg_start, arg_end - arg_start);
        std::size_t redir_char_found = argument.find_first_of(REDIRECTION_CHARS);

        if( found != string::npos ){
            /* stop at REDIRECTION_CHARS like pipe */
            if( redir_char_found == 0 ){
                /* first char is REDIRECTION_CHARS */
                command_str = command_str.substr(arg_start, string::npos);
                return this->current_cmd().args_count;
            }
            argument = argument.substr(0, found);
            this->add_argv(argument);
            command_str = command_str.substr(arg_start + redir_char_found, string::npos);
            return this->current_cmd().args_count;
        }
        else{
            /* found one argument */
            this->add_argv(argument);
            command_str = command_str.substr(arg_end, string::npos);
            continue;
        }
    }
}

int OneLineCommand::parse_redirection(string command_str){
    /* parsing REDIRECTION_CHARS 
     * format: FORMAT1 , FORMAT2 ... , (RETURN_STATUS)
     *       : <number , <filename   , (NEXT_IS_REDIR_CHARS)
     *       : >number , >filename   , (NEXT_IS_REDIR_CHARS) 
     *       : |number command       , (NEXT_IS_CMD) 
     *                 ^ command_str will be here after this function.
     *
     * return: NEXT_IS_CMD, NEXT_IS_REDIR_CHARS, NO_NEXT, CMD_ERROR
     */

    /* strip left whitespaces */
    std::size_t found = command_str.find_first_not_of(WHITESPACE) 
    if( found == string::npos )
        return NO_NEXT; // No Command
    command_str = command_str.substr(found, string::npos);

    char redir_char = command_str[0];
    if( !char_belong_to(redir_char, REDIRECTION_CHARS) ){
        return CMD_ERROR;
    }
    
    /* [|><][0-9]+ => redir_char + redir_num 
     * [|><]       => redir_char (redir_num = -1)
     */
    int redir_num = -1;
    if( command_str[1] >= '0' && command_str[1] <= '9' ){
        std::size_t index_after_num = 0;
        redir_num = stoi(command_str, &index_after_num);
        command_str = command_str.substr(index_after_num, string::npos);
    }
    else
        command_str = command_str.substr(1, string::npos);

    /* store (redir_char, redir_num) */
    if( redir_char == '|' ){
        if( redir_num == -1 ) 
            redir_num = 1;

        if( redir_num == 0 )
            error_print_and_exit("pipe error: %s\n", command_str.substr(0, 2).c_str());

        this->current_cmd().std_output.set_pipe_redirect(redir_num);
        this->cmd_count += 1;
        return NEXT_IS_CMD;
    }
    else if( char_belong_to(redir_char, FILE_REDIR_CHARS) ){
        if( redir_num != -1 ){
            /* redirect to/from number(person in chat room) */
            if( redir_char == '<' ){
                this->current_cmd().std_input.set_file_redirect(redir_num);
            }
            else if( redir_char == '>' ){
                this->current_cmd().std_output.set_file_redirect(redir_num);
            }
        }
        else{
            /* redirect to/from file */
            std::size_t start = command_str.find_first_not_of(WHITESPACE)
            std::size_t end = command_str.find_first_of(WHITESPACE, start);
            if( start == string::npos ){
                /* end of line */
                command_str = "";
                return CMD_ERROR; // no filename
            }
            string filename = command_str.substr(start, end - start);

            if( redir_char == '<' ){
                this->current_cmd().std_input.set_file_redirect(filename);
            }
            else if( redir_char == '>' ){
                this->current_cmd().std_output.set_file_redirect(filename);
            }
            command_str = command_str.substr(end, string::npos);
        } 
        return NEXT_IS_REDIR_CHARS;
    }
}

int OneLineCommand::parse_one_line_cmd(string command_str){
    string backup_command = command_str;
    while( 1 ){
        std::size_t found = command_str.find_first_not_of(WHITESPACE) /* strip left whitespaces */
        if( found == string::npos )
            return 0;
        current_cmd = command_str.substr(pos = found);

        parse_single_command(current_cmd);
        while( 1 ){
            int status = parse_redirection(current_cmd);

            if( status == NEXT_IS_CMD )
                break;
            else if( status == NEXT_IS_REDIR_CHARS )
                continue;
            else if( status == NO_NEXT )
                return 0;
            else if( status == CMD_ERROR )
                error_print_and_exit("parsing command error %s\n", backup_command.c_str());
        }
    }
    return 0;
}
