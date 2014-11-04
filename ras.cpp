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
#include <signal.h>

#include "socket.h"
#include "io_wrapper.h"
#include "parser.h"
#include "pipe_manager.h"
#include "cstring_more.h"

using namespace std;

const char RAS_IP[] = "0.0.0.0";
const int RAS_DEFAULT_PORT = 52000;
const int MAX_ONELINE_CMD_SIZE = 65536;
const int MAX_CMD_SIZE = 256;

typedef void (*OneConnectionService)(socketfd_t connection_socket); 
/* for example, telnet_service, http_service, or ras_service in our hw1 */

void start_multiprocess_server(socketfd_t listen_socket, OneConnectionService service_function);
void ras_service(socketfd_t client_socket);
int execute_cmd(socketfd_t client_socket, PipeManager& cmd_pipe_manager, const char* origin_command);
    const int CMD_NORMAL = 0, CMD_EXIT = 1;

/* start_multiprocess_server sub functions */
void sigchid_waitfor_child(int sig);
/* ras_service sub functions */
void ras_shell_init();
void print_welcome_msg(socketfd_t client_socket);
int read_cmd_from_socket_and_check_overflow(char* cmd_buf, int& cmd_size, socketfd_t client_socket);
/* execute_cmd sub functions */
bool is_internal_command_and_run(bool& is_exit, SingleCommand& cmd, socketfd_t client_socket);
void processing_child_output_data(AnonyPipe& child_output_pipe, socketfd_t client_socket);

int main(int argc, char** argv){
    int ras_port = RAS_DEFAULT_PORT;
    if(argc == 2){
        ras_port = strtol(argv[1], NULL, 0);
    }
    
    /* listening ras first */
    socketfd_t ras_listen_socket;
    ras_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( ras_listen_socket < 0 )
        perror_and_exit("create socket error");

    // int on = 1;
    // setsockopt(ras_listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

    if( socket_bind(ras_listen_socket, RAS_IP, ras_port) < 0 )
        perror_and_exit("bind error");
    if( listen(ras_listen_socket, 1) < 0)
        perror_and_exit("listen error");

    start_multiprocess_server(ras_listen_socket, ras_service);
    return 0;
}

void start_multiprocess_server(socketfd_t listen_socket, OneConnectionService service_function){
    /* wait at receive SIGCHLD, release child resource for multiprocess && concurrent server */
    signal(SIGCHLD, sigchid_waitfor_child);

    while(1){
        socketfd_t connection_socket;
        char client_ip[IP_MAX_LEN] = {'\0'};
        int client_port;

        connection_socket = socket_accept(listen_socket, client_ip, &client_port);
        if( connection_socket < 0 ){
            perror("accept error");
            continue;
        }

        int child_pid = fork();
        if( child_pid == 0 ){
            int ret = close(listen_socket);
            if( ret < 0 ) perror("close listen_socket error");

            service_function(connection_socket);

            ret = close(connection_socket);
            if( ret < 0 ) perror("close connection_socket error");
            exit(EXIT_SUCCESS);
        }
        else if( child_pid > 0 ){
            close(connection_socket);
        }
        else {
            perror("fork error");
        }
    }
}

void ras_service(socketfd_t client_socket){
    /* client is connect to server, this function do ras service to client */
    char cmd_buf[MAX_ONELINE_CMD_SIZE+1] = {0};
    int cmd_size = 0;
    PipeManager cmd_pipe_manager;

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

int execute_cmd(socketfd_t client_socket, PipeManager& cmd_pipe_manager, const char* origin_command){
    /* parsing and execute shell command */
    int cmd_len = strlen(origin_command);
    if( cmd_len == 0 ) 
        return CMD_NORMAL;
    char* command = new char [cmd_len+1];
    strncpy_add_null(command, origin_command, strlen(origin_command));

    /* parsing */
    OneLineCommand parsed_cmds;
    parsed_cmds.parse_one_line_cmd(command);
    parsed_cmds.print();

    /* processing command */
    bool is_exit = false;
    bool is_internal = is_internal_command_and_run(is_exit, parsed_cmds.cmds[0], client_socket);
    if( is_exit ) return CMD_EXIT;
    if( is_internal ) return CMD_NORMAL;

    // cmd_pipe_manager, parsed_cmds
    AnonyPipe child_output_pipe;
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

            if( is_file_input && cmd_pipe_manager.cmd_has_pipe(0) ){
                error_print_and_exit("ambiguous input redirection");
            }
        }

        /* fd creation (output to pipe or output to file) */
        int is_file_output = false;
        int file_output_fd = -1;
        if( i == parsed_cmds.cmd_count-1 ){
            /* last command of line, check output file redirection */
            if( parsed_cmds.last_output_redirect.kind == REDIR_FILE ){
                is_file_output = true;
                file_output_fd = open(parsed_cmds.last_output_redirect.data.filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if(file_output_fd == -1)
                    perror_and_exit("open error");
            }

            if( is_file_output && cmd_pipe_manager.cmd_has_pipe(parsed_cmds.cmd_count-1) ){
                error_print_and_exit("ambiguous output redirection");
            }
        }

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
            }
            else if( cmd_pipe_manager.cmd_has_pipe(0) ){
                AnonyPipe& input_pipe = cmd_pipe_manager.get_pipe(0);
                int input_fd = input_pipe.read_fd();
                dup2(input_fd, STDIN_FILENO);
                input_pipe.close_pipe();
            }
            /*
            else{
                dup2(client_socket, STDIN_FILENO);
            }
            */

            /* stdout redirection */
            if( i == parsed_cmds.cmd_count-1 && is_file_output ){
                dup2(file_output_fd, STDOUT_FILENO);
            }
            else if( parsed_cmds.output_redirect[i].kind == REDIR_PIPE ){
                int pipe_index_in_manager = parsed_cmds.output_redirect[i].data.pipe_index_in_manager;
                AnonyPipe& output_pipe = cmd_pipe_manager.get_pipe(pipe_index_in_manager);
                int output_fd = output_pipe.write_fd();
                dup2(output_fd, STDOUT_FILENO);
            }
            else{
                int output_fd = child_output_pipe.write_fd();
                dup2(output_fd, STDOUT_FILENO);
            }
            
            /* stderr redirection */
            dup2(child_output_pipe.write_fd(), STDERR_FILENO);
            execvp(parsed_cmds.cmds[i].executable, parsed_cmds.cmds[i].argv);

            /* exec error: print "Unknown command [command_name]" */
            char unknown_cmd[MAX_CMD_SIZE+128] = "";
            int unknown_cmd_size = snprintf(unknown_cmd, MAX_CMD_SIZE+128, "Unknown command: [%s].\n", parsed_cmds.cmds[i].executable);
            write(child_output_pipe.write_fd(), unknown_cmd, unknown_cmd_size);
            exit(EXIT_FAILURE);
        }
        else if(pid > 0){
            if( cmd_pipe_manager.cmd_has_pipe(0) ){
            /* if child stdin use pipe, close write end in parent. */
                cmd_pipe_manager.get_pipe(0).close_write();
            }
            int child_status;
            wait(&child_status);
            /* child status */
            if( WIFEXITED(child_status) ){
                int exit_status = WEXITSTATUS(child_status); 
                // printf("exit_status: %d\n", exit_status);
                if( exit_status == 0 ){
                    /* correct exit */
                    cmd_pipe_manager.get_pipe(0).close_pipe();
                    /* run the next command in one-line-command */
                    cmd_pipe_manager.next_pipe();
                }
                else{
                    /* error cmd, finish this one-line-command */
                    break;
                }
            }
        }   
        else{
            perror_and_exit("fork error");
        }
    }
    processing_child_output_data(child_output_pipe, client_socket);

    return CMD_NORMAL;
}

/* main_service sub functions */
void sigchid_waitfor_child(int sig){ 
    int status;
    pid_t child;
    while( (child = waitpid(-1, &status, WNOHANG)) > 0 );
}

/* ras_service sub functions */
void ras_shell_init(){
    char ras_dir[1024+1];
    char* home_dir = getenv("HOME");
    if(!home_dir)
        error_print_and_exit("Error: No HOME enviroment variable\n");

    snprintf(ras_dir, 1024, "%s/ras/", home_dir);
    int ret = chdir(ras_dir);
    if(ret == -1)
        perror_and_exit("chdir error");

    ret = setenv("PATH", "bin:.", 1);
    if(ret == -1)
        perror_and_exit("setenv error");
}

void print_welcome_msg(socketfd_t client_socket){
    char msg1[] = "****************************************\n";
    char msg2[] = "** Welcome to the information server. **\n";
    char msg3[] = "****************************************\n";
    /*
    char* msg4 = new char [65536];
    snprintf(msg4, 65535, "** You are in the directory, %s/ras/.\n", getenv("HOME"));
    char msg5[] = "** This directory will be under \"/\", in this system.\n";
    char msg6[] = "** This directory includes the following executable programs.\n";
    char msg7[] = "**\n";
    char msg8[] = "**    bin/\n";
    char msg9[] = "**    test.html    (test file)\n";
    char msg10[] = "**\n";
    char msg11[] = "** The directory bin/ includes:\n";
    char msg12[] = "**    cat\n";
    char msg13[] = "**    ls\n";
    char msg14[] = "**    removetag         (Remove HTML tags.)\n";
    char msg15[] = "**    removetag0        (Remove HTML tags with error message.)\n";
    char msg16[] = "**    number            (Add a number in each line.)\n";
    char msg17[] = "**\n";
    char msg18[] = "** In addition, the following two commands are supported by ras.\n";
    char msg19[] = "**    setenv\n";
    char msg20[] = "**    printenv\n";
    char msg21[] = "**\n";
    */

    write_all(client_socket, msg1, strlen(msg1));
    write_all(client_socket, msg2, strlen(msg2));
    write_all(client_socket, msg3, strlen(msg3));
    /*
    write_all(client_socket, msg4, strlen(msg4));
    write_all(client_socket, msg5, strlen(msg5));
    write_all(client_socket, msg6, strlen(msg6));
    write_all(client_socket, msg7, strlen(msg7));
    write_all(client_socket, msg8, strlen(msg8));
    write_all(client_socket, msg9, strlen(msg9));
    write_all(client_socket, msg10, strlen(msg10));
    write_all(client_socket, msg11, strlen(msg11));
    write_all(client_socket, msg12, strlen(msg12));
    write_all(client_socket, msg13, strlen(msg13));
    write_all(client_socket, msg14, strlen(msg14));
    write_all(client_socket, msg15, strlen(msg15));
    write_all(client_socket, msg16, strlen(msg16));
    write_all(client_socket, msg17, strlen(msg17));
    write_all(client_socket, msg18, strlen(msg18));
    write_all(client_socket, msg19, strlen(msg19));
    write_all(client_socket, msg20, strlen(msg20));
    write_all(client_socket, msg21, strlen(msg21));
    delete [] msg4;
    */
}

int read_cmd_from_socket_and_check_overflow(char* cmd_buf, int& cmd_size, socketfd_t client_socket){
    /* return read size */
    if(cmd_size == MAX_ONELINE_CMD_SIZE){
        /* command too long */
        const char err_msg[] = "command too long.\n";
        write_all(client_socket, err_msg, strlen(err_msg));
        error_print(err_msg);
        cmd_size = 0;
    }
        
    int recv_size = 0;
    recv_size = read(client_socket, cmd_buf+cmd_size, MAX_ONELINE_CMD_SIZE-cmd_size);
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

/* execute_cmd sub functions */
bool is_internal_command_and_run(bool& is_exit, SingleCommand& cmd, socketfd_t client_socket){
    if( strncmp(cmd.executable, "exit", 4) == 0 ){
        is_exit = true;
    }
    else if( strncmp(cmd.executable, "printenv", 8) == 0 ){
        char tmp[1024+1];
        int size = snprintf(tmp, 1024, "%s=%s\n", cmd.argv[1], getenv(cmd.argv[1]));
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

void processing_child_output_data(AnonyPipe& child_output_pipe, socketfd_t client_socket){
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
