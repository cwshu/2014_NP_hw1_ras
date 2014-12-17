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
#include "server_arch.h"

using namespace std;

const char RAS_IP[] = "0.0.0.0";
const int RAS_DEFAULT_PORT = 52000;
const int MAX_ONELINE_CMD_SIZE = 65536;
const int MAX_CMD_SIZE = 256;

void ras_service(socketfd_t client_socket);
int execute_cmd(socketfd_t client_socket, PipeManager& cmd_pipe_manager, const char* origin_command);
    const int CMD_NORMAL = 0, CMD_EXIT = 1;

/* ras_service sub functions */
void ras_shell_init();
void print_welcome_msg(socketfd_t client_socket);
int read_cmd_from_socket_and_check_overflow(char* cmd_buf, int& cmd_size, socketfd_t client_socket);

/* execute_cmd sub functions */
void pre_fd_redirection(PipeManager& cmd_pipe_manager, int origin_fd, Redirection& redirect_obj);
void fd_redirection(PipeManager& cmd_pipe_manager, int origin_fd, Redirection& redirect_obj,
  AnonyPipe& child_output_pipe);
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

    string command(origin_command);

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

    for( auto& current_cmd : parsed_cmds.cmds ){
        /* 
         * exec(current_cmd.executable, current_cmd.gen_argv())
         * stdin: current_cmd.std_input.(kind, data), cmd_pipe_manager.cmd_has_pipe(0)
         * stdout: current_cmd.std_output.(kind, data), 
         * stderr: current_cmd.std_error.(kind, data), 
         * (any redirect to pipe) cmd_pipe_manager
         */

        /* pre-processing redirection in parent process */
        if( current_cmd.std_input.kind != REDIR_NONE && cmd_pipe_manager.cmd_has_pipe(0) ){
            error_print_and_exit("ambiguous input redirection");
        }
        if( cmd_pipe_manager.cmd_has_pipe(0) ){
            current_cmd.std_input.set_pipe_redirect(0);
        }
        pre_fd_redirection(cmd_pipe_manager, STDIN_FILENO, current_cmd.std_input);
        pre_fd_redirection(cmd_pipe_manager, STDOUT_FILENO, current_cmd.std_output);
        pre_fd_redirection(cmd_pipe_manager, STDERR_FILENO, current_cmd.std_error);
        
        int pid = fork();
        if( pid == 0 ){
            /* stdin redirection */
            fd_redirection(cmd_pipe_manager, STDIN_FILENO, current_cmd.std_input, child_output_pipe);
            /* stdout redirection */
            fd_redirection(cmd_pipe_manager, STDOUT_FILENO, current_cmd.std_output, child_output_pipe);
            /* stderr redirection */
            fd_redirection(cmd_pipe_manager, STDERR_FILENO, current_cmd.std_error, child_output_pipe);

            char** argv = current_cmd.gen_argv();
            execvp(current_cmd.executable.c_str(), argv);
            /* exec error: print "Unknown command [command_name]" */
            char unknown_cmd[MAX_CMD_SIZE+128] = "";
            int u_cmd_size = snprintf(unknown_cmd, MAX_CMD_SIZE+128, "Unknown command: [%s].\n", current_cmd.executable.c_str());
            write(child_output_pipe.write_fd(), unknown_cmd, u_cmd_size);
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
    child_output_pipe.close_pipe();

    return CMD_NORMAL;
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
void pre_fd_redirection(PipeManager& cmd_pipe_manager, int origin_fd, Redirection& redirect_obj){
    /* create pipe */
    if( redirect_obj.kind == REDIR_PIPE ){
        if( origin_fd == STDOUT_FILENO || origin_fd == STDERR_FILENO ){
            int pipe_index = redirect_obj.data.pipe_index_in_manager;
            AnonyPipe& redirect_pipe = cmd_pipe_manager.get_pipe(pipe_index);
            if( !redirect_pipe.enable )
                redirect_pipe.create_pipe();
        }
    }
}

void fd_redirection(PipeManager& cmd_pipe_manager, int origin_fd, Redirection& redirect_obj,
  AnonyPipe& child_output_pipe){
    /* do the STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO redirection */
    if( redirect_obj.kind == REDIR_NONE ){
        if( origin_fd == STDOUT_FILENO || origin_fd == STDERR_FILENO ){
            int output_fd = child_output_pipe.write_fd();
            dup2(output_fd, origin_fd);
        }
    }
    else if( redirect_obj.kind == REDIR_FILE ){
        int file_fd;
        if( origin_fd == STDIN_FILENO )
            file_fd = open(redirect_obj.data.filename.c_str(), O_RDONLY);
        else
            file_fd = open(redirect_obj.data.filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);

        if(file_fd == -1)
            perror_and_exit("open error");

        dup2(file_fd, origin_fd);
    }
    else if( redirect_obj.kind == REDIR_PIPE ){
        int pipe_index = redirect_obj.data.pipe_index_in_manager;
        AnonyPipe& redirect_pipe = cmd_pipe_manager.get_pipe(pipe_index);

        if( origin_fd == STDIN_FILENO ){
            int redirect_fd = redirect_pipe.read_fd();
            dup2(redirect_fd, origin_fd);
            redirect_pipe.close_pipe();
        }
        else if( origin_fd == STDOUT_FILENO || origin_fd == STDERR_FILENO ){
            int redirect_fd = redirect_pipe.write_fd();
            dup2(redirect_fd, origin_fd);
        }
    }
}

bool is_internal_command_and_run(bool& is_exit, SingleCommand& cmd, socketfd_t client_socket){
    if( cmd.executable == "exit" ){
        is_exit = true;
    }
    else if( cmd.executable == "printenv" ){
        char tmp[1024+1];
        const char* argv1 = cmd.arguments[1].c_str();
        int size = snprintf(tmp, 1024, "%s=%s\n", argv1, getenv(argv1));
        write_all(client_socket, tmp, size);
    }
    else if( cmd.executable == "setenv" ){
        const char* argv1 = cmd.arguments[1].c_str();
        const char* argv2 = cmd.arguments[2].c_str();
        setenv(argv1, argv2, 1);
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
