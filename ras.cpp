#include <cstdio>
#include <cstdlib>
#include <cstdarg>
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

using namespace std;

const char RAS_IP[] = "0.0.0.0";
const int RAS_PORT = 52000;
const char WHITESPACE[] = " \t\r\n\v\f";

/*
void test_pwd(const char* tmp_file_name){
    FILE* fptr = fopen(tmp_file_name, "w");
    fprintf(fptr, "test\n");
    fclose(fptr);
}
*/
typedef struct anony_pipe {
    int read_fd;
    int write_fd;
} anony_pipe;

typedef struct special_char {
    char c;
    int num;
} special_char;

void ras_service(socketfd_t client_socket);
int execute_cmd(socketfd_t client_socket, vector<anony_pipe> pipes_fd, const char* origin_command);
const int CMD_NORMAL = 0, CMD_EXIT = 1;

void perr(const char* format, ...);
void perr_and_exit(const char* format, ...);

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
    vector<anony_pipe> pipes_fd;

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
            if( execute_cmd(client_socket, pipes_fd, cur_cmd_head) == CMD_EXIT )
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

const char SHELL_SPECIAL_CHARS[] = "|><";
int execute_cmd(socketfd_t client_socket, vector<anony_pipe> pipes_fd, const char* origin_command){
    /* parsing and execute shell command */
    int cmd_len = strlen(origin_command);
    if( cmd_len == 0 ) 
        return CMD_NORMAL;
    char* command = (char*) malloc(cmd_len);
    strncpy(command, origin_command, strlen(origin_command));

    char* current_cmd = command;
    char* cmds[10001] = {NULL}; /* point to NULL after last cmd */
    special_char s_char[10001] = {0}; /* .c member is NULL after last s_char */
    int cmd_num = 0;

    while(1){
        current_cmd += strspn(current_cmd, WHITESPACE); /* strip left whitespaces */
        if(current_cmd[0] == '\0'){
            /* NULL-terminator of command */
            s_char[cmd_num].c = 0;
            cmds[cmd_num] = NULL;

            break;
        }

        char* special_char_place = strpbrk(current_cmd, SHELL_SPECIAL_CHARS);
        if(special_char_place != NULL){
            /* record command */
            int cmd_len = special_char_place - current_cmd;
            cmds[cmd_num] = (char*) malloc(cmd_len+1*sizeof(char));
            strncpy(cmds[cmd_num], current_cmd, cmd_len);
            
            /* record special_char and update current_cmd */
            s_char[cmd_num].c = special_char_place[0];
            s_char[cmd_num].num = 0;
            if(special_char_place[0] == '|'){
                /* cmd1 |<num> cmd2 */
                if(special_char_place[1] >= '0' && special_char_place[1] <= '9'){
                    /* |[0-9] => pipe + number */
                    char *end_of_num = NULL;
                    int pipe_num = strtol(special_char_place+1, &end_of_num, 10);
                    if(pipe_num == 0){
                        end_of_num[0] = '\0';
                        perr_and_exit("pipe error: %s\n", special_char_place);
                    }
                    
                    s_char[cmd_num].num = pipe_num;
                    current_cmd = end_of_num;
                }
                else{
                    /* only pipe without number */
                    s_char[cmd_num].num = 1;
                    current_cmd = special_char_place+1;
                }
            }
            else{
                current_cmd = special_char_place+1;
            }

            cmd_num++;
        }
        else{
            int cmd_len = strlen(current_cmd);
            cmds[cmd_num] = (char*) malloc(cmd_len+1*sizeof(char));
            strncpy(cmds[cmd_num], current_cmd, cmd_len);
            s_char[cmd_num].c = 0; /* end of s_char */
            cmds[cmd_num+1] = NULL;
            cmd_num++;

            break;
        }
    }

#if 0
    printf("cmd_num: %d\n", cmd_num);
    for(int i=0; cmds[i] != NULL; i++){
        printf("cmd: %s\n", cmds[i]);
        if(s_char[i].c == '|'){
            printf("s_char: (%c, %d)\n", s_char[i].c, s_char[i].num);
        }
        else if(s_char[i].c != 0){
            printf("s_char: %c\n", s_char[i].c);
        }
    }
#endif

#if 0
    strtok(cmd_start, WHITESPACE);

    printf("command: %s\n", cmd_start);

    /* build-in command */
    if( strncmp(cmd_start, "exit", 4) == 0 ){
        return CMD_EXIT;
    }
    else if( strncmp(cmd_start, "printenv", 4) == 0 ){
        char* arg1 = strtok(NULL, WHITESPACE);
        // printf("arg1 = %s, len = %d\n", arg1, strlen(arg1));
        const char* env_value = getenv(arg1);
        if( env_value == NULL ){
            printf("No environment variable %s\n", arg1);
        }
        else{
            printf("%s=%s\n", arg1, env_value);
        }
    }
    else if( strncmp(cmd_start, "setenv", 4) == 0 ){
        char* arg1 = strtok(NULL, WHITESPACE);
        char* arg2 = strtok(NULL, WHITESPACE);
        // printf("arg1 = %s, len = %d\n", arg1, strlen(arg1));
        // printf("arg2 = %s, len = %d\n", arg2, strlen(arg2));
        setenv(arg1, arg2, 1);
    }
#endif

    return CMD_NORMAL;
}

/* error output */
void perr(const char* format, ...){
    /* print format string to stderr */
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

void perr_and_exit(const char* format, ...){
    /* print format string to stderr */
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    /* exit */
    exit(EXIT_FAILURE);
}
