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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "socket.h"
#include "io_wrapper.h"
#include "parser.h"
#include "pipe_manager.h"

using namespace std;

const char RAS_IP[] = "0.0.0.0";
const int RAS_PORT = 52000;

struct special_char {
    char c;
    int num;
    special_char(){
        c = 0;
        num = 0;
    }
};

void ras_service(socketfd_t client_socket);
int execute_cmd(socketfd_t client_socket, pipe_manager& cmd_pipe_manager, const char* origin_command);
const int CMD_NORMAL = 0, CMD_EXIT = 1;

int main(){
    /* listening RAS_PORT first */
    socketfd_t ras_listen_socket;
    
    ras_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( ras_listen_socket < 0 )
        perr_and_exit("can't create socket: %s", strerror(errno));
    if( socket_bind(ras_listen_socket, RAS_IP, RAS_PORT) < 0 )
        perr_and_exit("can't bind: %s", strerror(errno));
    if( listen(ras_listen_socket, 1) < 0)
        perr_and_exit("can't listen: %s", strerror(errno));

    while(1){
        socketfd_t connection_socket;
        char client_ip[IP_MAX_LEN] = {'\0'};
        int client_port;

        connection_socket = socket_accept(ras_listen_socket, client_ip, &client_port);
        if( connection_socket < 0 )
            perr_and_exit("can't accept: %s", strerror(errno));

        ras_service(connection_socket);
        close(connection_socket);
    }
    return 0;
}

const int MAX_CMD_SIZE = 65536;
void ras_service(socketfd_t client_socket){
    /* client is connect to server, this function do ras service to client */
    char cmd_buf[MAX_CMD_SIZE+1] = {0};
    int cmd_size = 0;
    pipe_manager cmd_pipe_manager;

    while(1){

        if(cmd_size == MAX_CMD_SIZE){
            /* command too long */
            const char err_msg[] = "command too long.\n";
            send(client_socket, err_msg, strlen(err_msg), 0);
            perr(err_msg);
            cmd_size = 0;
        }
            
        int recv_size = recv(client_socket, cmd_buf+cmd_size, MAX_CMD_SIZE-recv_size, 0);
        if( recv_size == 0 )
            /* client is closing connection */
            break;
        cmd_size += recv_size; 
        cmd_buf[cmd_size] = '\0';

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
    char* command = (char*) malloc(cmd_len);
    strncpy(command, origin_command, strlen(origin_command));

    /* parsing */
    struct one_line_cmd parsed_cmds;
    parsing_command(&parsed_cmds, command);
    // parsed_cmds.print();

    /* processing command */
    // cmd_pipe_manager, parsed_cmds
    int pipe_num[2];
    anony_pipe client_output_pipe;
    pipe(pipe_num);
    client_output_pipe.set_pipe(pipe_num[0], pipe_num[1]);

    for(int i=0; i<parsed_cmds.cmd_count; i++){
        /* fd creation (input file) */
        bool is_file_input = false;
        int file_input_fd = -1;
        if( i == 0 ){
            if( parsed_cmds.input_redirect.kind == REDIR_FILE ){
                is_file_input = true;
                file_input_fd = open(parsed_cmds.input_redirect.data.filename, O_RDONLY);
                if(file_input_fd == -1)
                    perr_and_exit("open error: %s\n", strerror(errno));
            }
        }

        if( is_file_input && cmd_pipe_manager.cmd_has_pipe(0) ){
            perr_and_exit("ambiguous input redirection");
        }

        /* fd creation (pipe) */
        if( parsed_cmds.output_redirect[i].kind == REDIR_PIPE ){
            int pipe_index_in_manager = parsed_cmds.output_redirect[i].data.pipe_num;
            if( !cmd_pipe_manager.cmd_has_pipe(pipe_index_in_manager) ){
                cmd_pipe_manager.create_pipe(pipe_index_in_manager);
            }
        }

        int pid = fork();
        if( pid == 0 ){
            /* stdin redirection */
            if( i == 0 && is_file_input ){
                dup2(file_input_fd, STDIN_FILENO);
            }
            else if( cmd_pipe_manager.cmd_has_pipe(0) ){
                int input_fd = cmd_pipe_manager.pipe_of_unexecuted_cmds[0].read_fd;
                dup2(input_fd, STDIN_FILENO);
            }

            /* stdout/stderr redirection */
            int output_fd;
            if( parsed_cmds.output_redirect[i].kind == REDIR_PIPE ){
                int pipe_index_in_manager = parsed_cmds.output_redirect[i].data.pipe_num;
                output_fd = cmd_pipe_manager.pipe_of_unexecuted_cmds[pipe_index_in_manager].write_fd;
            }
            else{
                output_fd = client_output_pipe.write_fd;
            }
            dup2(output_fd, STDOUT_FILENO);
            dup2(output_fd, STDERR_FILENO);

            execvp(parsed_cmds.cmds[i].executable, parsed_cmds.cmds[i].argv);
        }
        else if( pid > 0 ){
            while(1){
                char* read_buf[1024+1];
                int read_size = read(client_output_pipe.read_fd, read_buf, 1024);
                if(read_size == 0){
                    break; 
                }
                int write_size = write_all(client_socket, read_buf, 1024);
            }
        }
        else{
            perr_and_exit("fork error: %s\n", strerror(errno));
        }
    }
    return CMD_NORMAL;
}
