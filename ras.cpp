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

#include "socket.h"
#include "io_wrapper.h"
#include "parser.h"

using namespace std;

const char RAS_IP[] = "0.0.0.0";
const int RAS_PORT = 52000;

struct anony_pipe {
    int enable;
    int read_fd;
    int write_fd;
    anony_pipe(){
        enable = 0;
        read_fd = -1;
        write_fd = -1;
    }
}; 

struct pipe_manager {
    int cur_pipe;
    vector<anony_pipe> pipe_array;
    pipe_manager(){
        cur_pipe = 0;
        pipe_array = vector<anony_pipe>(16, anony_pipe());
    }
};

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
    parsed_cmds.print();

#if 0
    for(int i=0; cmds[cmd_num] != NULL; i++){
        /* execute command.
         * manage pipe and io redirection.
         * if command error, print command and stop this input.
         */
        int read_fd = -1, write_fd = -1;
        if( s_char[cmd_num].c == '|' ){
            int index = cmd_pipe_manager.cur_pipe + s_char[cmd_num].num;
            if( index > cmd_pipe_manager.pipe_array.size() ){
                cmd_pipe_manager.pipe_array.resize(index, anony_pipe());
            }
            if( !cmd_pipe_manager.pipe_array[index].enable ){
                int pipefd[2];
                pipe(pipefd);
                cmd_pipe_manager.pipe_array[index].read_fd = pipefd[0];
                cmd_pipe_manager.pipe_array[index].write_fd = pipefd[1];
            }
            write_fd = cmd_pipe_manager.pipe_array[index].write_fd;

            if( cmd_pipe_manager.pipe_array[cmd_pipe_manager.cur_pipe].enable )
                read_fd = cmd_pipe_manager.pipe_array[cmd_pipe_manager.cur_pipe].read_fd;
        }

        int pid = fork();
        if( pid == 0 ){
            if( read_fd != -1 ) dup2(read_fd, 0);
            if( write_fd != -1 ) dup2(write_fd, 1);
            // execvp();
        }
        /*
        else if( pid > 0 ){
            
        }
        */
        cmd_pipe_manager.cur_pipe += 1;
    }
#endif
    return CMD_NORMAL;
}
