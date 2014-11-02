#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cinttypes>
#include <cerrno>
#include <vector>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "socket.h"
#include "io_wrapper.h"
#include "parser.h"
#include "pipe_manager.h"
#include "cstring_more.h"

using namespace std;

const char RAS_IP[] = "0.0.0.0";
const int RAS_DEFAULT_PORT = 52000;
const int MAX_CMD_SIZE = 65536;

void ras_service(socketfd_t client_socket);
int execute_cmd(socketfd_t client_socket, pipe_manager& cmd_pipe_manager, const char* origin_command);
    const int CMD_NORMAL = 0, CMD_EXIT = 1;

/* ras_service sub functions */
void ras_shell_init();
void print_welcome_msg(socketfd_t client_socket);
int read_cmd_from_socket_and_check_overflow(char* cmd_buf, int& cmd_size, socketfd_t client_socket);
/* execute_cmd sub functions */
bool is_internal_command_and_run(one_cmd& cmd, socketfd_t client_socket);
void processing_child_output_data(anony_pipe& child_output_pipe, socketfd_t client_socket);

int main(int argc, char** argv){
    /* listening RAS_PORT first */
    int ras_port = RAS_DEFAULT_PORT;
    socketfd_t ras_listen_socket;
    if(argc == 2){
        ras_port = strtol(argv[1], NULL, 0);
    }
    
    ras_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( ras_listen_socket < 0 )
        perror_and_exit("can't create socket");
    if( socket_bind(ras_listen_socket, RAS_IP, ras_port) < 0 )
        perror_and_exit("can't bind");
    if( listen(ras_listen_socket, 1) < 0)
        perror_and_exit("can't listen");

    while(1){
        socketfd_t connection_socket;
        char client_ip[IP_MAX_LEN] = {'\0'};
        int client_port;

        connection_socket = socket_accept(ras_listen_socket, client_ip, &client_port);
        if( connection_socket < 0 )
            perror_and_exit("can't accept");

        ras_service(connection_socket);
        close(connection_socket);
    }
    return 0;
}

void ras_service(socketfd_t client_socket){
    /* client is connect to server, this function do ras service to client */
    char cmd_buf[MAX_CMD_SIZE+1] = {0};
    int cmd_size = 0;
    pipe_manager cmd_pipe_manager;

    ras_shell_init();
    print_welcome_msg(client_socket);

    while(1){
        write_all(client_socket, "% ", 2);

        int recv_size = read_cmd_from_socket_and_check_overflow(cmd_buf, cmd_size, client_socket);
        if(recv_size == 0)
            break;

        char* cur_cmd_head = cmd_buf;
        char* newline_char;
        while( (newline_char = strchr(cur_cmd_head, '\n')) != NULL ){
            /* split command and execute it. */
            newline_char[0] = '\0';
            if( execute_cmd(client_socket, cmd_pipe_manager, cur_cmd_head) == CMD_EXIT )
                return;
            cur_cmd_head = newline_char+1;
        }

        if(cur_cmd_head != cmd_buf){
            /* move the un-executed command to start position of cmd_buf. */
            int used_byte = cur_cmd_head - cmd_buf;
            cmd_size -= used_byte;
            memmove(cmd_buf, cur_cmd_head, cmd_size);
        }
    }
}


int execute_cmd(socketfd_t client_socket, pipe_manager& cmd_pipe_manager, const char* origin_command){
    /* parsing and execute shell command */
    int cmd_len = strlen(origin_command);
    if( cmd_len == 0 ) 
        return CMD_NORMAL;
    char* command = new char [cmd_len+1];
    strncpy_add_null(command, origin_command, strlen(origin_command));

    /* parsing */
    struct one_line_cmd parsed_cmds;
    parsing_command(&parsed_cmds, command);
    parsed_cmds.print();

    /* processing command */
    bool is_internal = is_internal_command_and_run(parsed_cmds.cmds[0], client_socket);
    if(is_internal)
        return CMD_NORMAL;
    // cmd_pipe_manager, parsed_cmds
    anony_pipe child_output_pipe;
    child_output_pipe.create_pipe();
    int legal_cmd = 0;
    for(int i=0; i<parsed_cmds.cmd_count; i++){
        /* fd creation (input file) */
        bool is_file_input = false;
        int file_input_fd = -1;
        if( i == 0 ){
            /* 1st command of line, check input file redirection */
            if( parsed_cmds.input_redirect.kind == REDIR_FILE ){
                is_file_input = true;
                file_input_fd = open(parsed_cmds.input_redirect.data.filename, O_RDONLY);
                if(file_input_fd == -1)
                    perror_and_exit("open error");
            }
        }

        if( is_file_input && cmd_pipe_manager.cmd_has_pipe(0) ){
            error_print_and_exit("ambiguous input redirection");
        }

        /* fd creation (pipe) */
        if( parsed_cmds.output_redirect[i].kind == REDIR_PIPE ){
            int pipe_index_in_manager = parsed_cmds.output_redirect[i].data.pipe_index_in_manager;
            if( !cmd_pipe_manager.cmd_has_pipe(pipe_index_in_manager) ){
                cmd_pipe_manager.get_pipe(pipe_index_in_manager).create_pipe();
            }
        }

        int pid = fork();
        if( pid == 0 ){
            /* stdin redirection */
            if( i == 0 && is_file_input ){
                dup2(file_input_fd, STDIN_FILENO);
                close(file_input_fd);
            }
            else if( cmd_pipe_manager.cmd_has_pipe(0) ){
                anony_pipe& input_pipe = cmd_pipe_manager.get_pipe(0);
                int input_fd = input_pipe.read_fd();
                dup2(input_fd, STDIN_FILENO);
                input_pipe.close_pipe();
            }

            /* stdout redirection */
            if( parsed_cmds.output_redirect[i].kind == REDIR_PIPE ){
                int pipe_index_in_manager = parsed_cmds.output_redirect[i].data.pipe_index_in_manager;
                anony_pipe& output_pipe = cmd_pipe_manager.get_pipe(pipe_index_in_manager);
                int output_fd = output_pipe.write_fd();
                dup2(output_fd, STDOUT_FILENO);
                output_pipe.close_write();
            }
            else{
                int output_fd = child_output_pipe.write_fd();
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
            
            /* stderr redirection */
            dup2(child_output_pipe.write_fd(), STDERR_FILENO);
            execvp(parsed_cmds.cmds[i].executable, parsed_cmds.cmds[i].argv);
        }
        else if(pid > 0){
            if( cmd_pipe_manager.cmd_has_pipe(0) ){
            /* if child stdin use pipe, close it(especially write end) in parent. */
                cmd_pipe_manager.get_pipe(0).close_pipe();
            }
        }   
        else{
            perror_and_exit("fork error");
        }

        /* run the next command in one-line-command */
        cmd_pipe_manager.next_pipe();
    }
    for(int i=0; i<parsed_cmds.cmd_count; i++){
        wait(NULL);
    }
    processing_child_output_data(child_output_pipe, client_socket);

    return CMD_NORMAL;
}

bool is_internal_command_and_run(one_cmd& cmd, socketfd_t client_socket){
    if( strncmp(cmd.executable, "exit", 4) == 0 ){
        exit(EXIT_SUCCESS);
    }
    else if( strncmp(cmd.executable, "printenv", 8) == 0 ){
        char tmp[1024];
        int size = sprintf(tmp, "%s\n", getenv(cmd.argv[1]));
        write_all(client_socket, tmp, size);
    }
    else if( strncmp(cmd.executable, "setenv", 6) == 0 ){
        setenv(cmd.argv[1], cmd.argv[2], 1);
    }
    else{
        return false;
    }
    return true;
}

void processing_child_output_data(anony_pipe& child_output_pipe, socketfd_t client_socket){
    child_output_pipe.close_write();
    while(1){
        char* read_buf[1024+1];
        int read_size = read(child_output_pipe.read_fd(), read_buf, 1024);
        if(read_size == 0){
            break; 
        }
        else if(read_size < 0){
            perror_and_exit("read child pipe error");
        }
        else{
            int write_size = write_all(client_socket, read_buf, read_size);
        }
    }
}

void ras_shell_init(){
    char ras_dir[1024+1];
    char* home_dir = getenv("HOME");
    if(!home_dir)
        error_print_and_exit("Error: No HOME enviroment variable\n");

    sprintf(ras_dir, "%s/ras/", home_dir);
    int ret = chdir(ras_dir);
    if(ret == -1)
        perror_and_exit("chdir error");
}

void print_welcome_msg(socketfd_t client_socket){
    char msg1[] = "****************************************\n";
    char msg2[] = "** Welcome to the information server. **\n";
    char msg3[] = "****************************************\n";

    write_all(client_socket, msg1, strlen(msg1));
    write_all(client_socket, msg2, strlen(msg2));
    write_all(client_socket, msg3, strlen(msg3));
}

int read_cmd_from_socket_and_check_overflow(char* cmd_buf, int& cmd_size, socketfd_t client_socket){
    /* return read size */
    if(cmd_size == MAX_CMD_SIZE){
        /* command too long */
        const char err_msg[] = "command too long.\n";
        write_all(client_socket, err_msg, strlen(err_msg));
        error_print(err_msg);
        cmd_size = 0;
    }
        
    int recv_size = 0;
    recv_size = read(client_socket, cmd_buf+cmd_size, MAX_CMD_SIZE-cmd_size);
    if( recv_size == 0 ){
        /* client is closing connection */
        return recv_size;
    }
    else if( recv_size == -1 ){
        perror_and_exit("read error");
    }
    cmd_size += recv_size; 
    cmd_buf[cmd_size] = '\0';
    return recv_size;
}
