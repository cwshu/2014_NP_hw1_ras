#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "server_arch.h"

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

/* start_multiprocess_server sub functions */
void sigchid_waitfor_child(int sig){ 
    int status;
    pid_t child;
    while( (child = waitpid(-1, &status, WNOHANG)) > 0 );
}

