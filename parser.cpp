#include <cstdio>
#include <cstdlib>

#include "cstring_more.h"
#include "io_wrapper.h"
#include "parser.h"

using namespace std;

/* struct Redirection */
Redirection::Redirection(){
    kind = REDIR_NONE; 
}

void Redirection::set_file_redirect(string filename){
    kind = REDIR_FILE;
    data.filename = filename;
}

void Redirection::set_to_person_redirect(int person_id){
    kind = REDIR_TO_PERSON;
    data.person_id = person_id;
}

void Redirection::set_pipe_redirect(int pipe_index_in_manager){
    kind = REDIR_PIPE;
    data.pipe_index_in_manager = pipe_index_in_manager;
}

void Redirection::print() const{
    if( kind == REDIR_NONE ){
        printf("no redirection\n");
    }
    else if( kind == REDIR_FILE ){
        printf("redirect to file: %s\n", data.filename.c_str());
    }
    else if( kind == REDIR_PIPE ){
        printf("redirect to pipe, pipe index = %d\n", data.pipe_index_in_manager);
    }
    else if( kind == REDIR_TO_PERSON ){
        printf("redirect to person, id = %d\n", data.person_id);
    }
}

/* struct SingleCommand */
SingleCommand::SingleCommand(){
    executable = string();
    arguments = vector<string>();
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
    arguments.push_back(argument);
    args_count += 1;
}

char** SingleCommand::gen_argv(){
    char** argv;
    argv = new char* [args_count+1];
    for( int i=0; i<args_count; i++ ){
        argv[i] = new char [arguments[i].length()+1];
        strncpy(argv[i], arguments[i].c_str(), arguments[i].length()+1);
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

/* struct OneLineCommand */
OneLineCommand::OneLineCommand(){
    cmds = vector<SingleCommand>();
    cmd_count = 0;
}

SingleCommand& OneLineCommand::current_cmd(){
    return cmds[cmd_count];
}

void OneLineCommand::next_cmd(){
    cmd_count++;
}

void OneLineCommand::add_executable(string executable_name){
    this->current_cmd().add_executable(executable_name);
}

void OneLineCommand::add_argv(string argument){
    this->current_cmd().add_argv(argument);
}

void OneLineCommand::print() const{
    printf("command count: %d\n", cmd_count);
    for( const auto& cmd : cmds ){
        printf("exe: %s", cmd.executable.c_str());
        for( const auto& argument : cmd.arguments ){
            printf("args: %s\n", argument.c_str());
        }
        cmd.std_input.print();
        cmd.std_output.print();
        cmd.std_error.print();
    }
}

string OneLineCommand::fetch_word(string& command_str){
    /* fetch the next "word" in command_str, and cut this word part in command_str.
     * the "word" means non-whitespace char sequence.
     */
    std::size_t start = command_str.find_first_not_of(WHITESPACE);
    std::size_t end = command_str.find_first_of(WHITESPACE, start);
    if( start == string::npos ){
        /* no word */
        command_str.clear();
        return string();
    }

    string word;
    if( end == string::npos ){
        word = command_str.substr(start, string::npos);
        command_str.clear();
    }
    else{
        word = command_str.substr(start, end - start);
        command_str.erase(end, string::npos);
    }

    return word;
}

int OneLineCommand::parse_single_command(string& command_str){
    /* parse single command, stop at pipe or file redirection or end-of-string(line).
     * single command: executable and arguments.
     *
     * return value: number of arguments. 
     * ex. 0 for error, 1 for only executable, 2 for executable + 1 argument.
     */

    /* parse executable name */
    string exe_name = fetch_word(command_str);
    if( exe_name.empty() )
        return 0;
    this->add_executable(exe_name);

    /* parse arguments */
    while( 1 ){
        string argument = fetch_word(command_str);
        if( argument.empty() )
            return this->current_cmd().args_count;

        std::size_t redir_char_found = argument.find_first_of(REDIRECTION_CHARS);
        if( redir_char_found != string::npos ){
            /* stop at REDIRECTION_CHARS like pipe */
            if( redir_char_found == 0 ){
                /* first char is REDIRECTION_CHARS */
                command_str = argument + string(" ") + command_str;
                return this->current_cmd().args_count;
            }
            argument = argument.substr(0, redir_char_found);
            this->add_argv(argument);

            command_str = argument.substr(redir_char_found, string::npos) + string(" ") + command_str;
            return this->current_cmd().args_count;
        }
        else{
            /* found one argument */
            this->add_argv(argument);
        }
    }
}

int OneLineCommand::parse_redirection(string& command_str){
    /* parsing REDIRECTION_CHARS 
     * format: FORMAT1 , FORMAT2 ... , (RETURN_STATUS)
     *       : <number , <filename   , (NEXT_IS_REDIR_CHARS)
     *       : >number , >filename   , (NEXT_IS_REDIR_CHARS) 
     *       : |number command       , (NEXT_IS_CMD) 
     *                 ^ command_str will be here after this function.
     *
     * return: NEXT_IS_CMD, NEXT_IS_REDIR_CHARS, NO_NEXT, CMD_ERROR
     */

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
                this->current_cmd().std_input.set_to_person_redirect(redir_num);
            }
            else if( redir_char == '>' ){
                this->current_cmd().std_output.set_to_person_redirect(redir_num);
            }
        }
        else{
            /* redirect to/from file */
            string filename = fetch_word(command_str);
            if( filename.empty() )
                return CMD_ERROR;

            if( redir_char == '<' )
                this->current_cmd().std_input.set_file_redirect(filename);
            else if( redir_char == '>' )
                this->current_cmd().std_output.set_file_redirect(filename);
        } 

        std::size_t found = command_str.find_first_not_of(WHITESPACE);
        if( found == string::npos )
            return NO_NEXT;
        if( char_belong_to(command_str[found], REDIRECTION_CHARS) )
            return NEXT_IS_REDIR_CHARS;
        return CMD_ERROR;
    }
    return CMD_ERROR;
}

int OneLineCommand::parse_one_line_cmd(string& command_str){
    /* parse one-line command, and store into OneLineCommand class
     * store array of commands
     * single command: (executable, arguments, stdin/stdout/stderr redirection)
     */
    string backup_command = command_str;
    while( 1 ){
        /* strip left whitespaces */
        std::size_t found = command_str.find_first_not_of(WHITESPACE);
        if( found == string::npos )
            return 0;
        command_str = command_str.substr(found, string::npos);
        /* parse command executable and arguments */
        parse_single_command(command_str);
        while( 1 ){
            /* parse IO redirection, and decide if there is more commands in this line. */
            int status = parse_redirection(command_str);

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
