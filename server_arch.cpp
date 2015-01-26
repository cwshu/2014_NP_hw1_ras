#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "server_arch.h"

socketfd_t bind_and_listen_tcp_socket(SocketAddr& listen_addr){
    /* bind and listen tcp socket with listen_addr */
    socketfd_t listen_socket;
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( listen_socket < 0 )
        perror_and_exit("create socket error");

    // int on = 1;
    // setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

    if( socket_bind(listen_socket, listen_addr) < 0 )
        perror_and_exit("bind error");
    if( listen(listen_socket, 1) < 0)
        perror_and_exit("listen error");

    return listen_socket;
}

void start_multiprocess_server(socketfd_t listen_socket, OneConnectionService service_function){
    /* wait at receive SIGCHLD, release child resource for multiprocess && concurrent server */
    signal(SIGCHLD, sigchid_waitfor_child);

    while(1){
        socketfd_t client_socket;
        SocketAddr client_addr;

        client_socket = socket_accept(listen_socket, client_addr);
        if( client_socket < 0 ){
            perror("accept error");
            continue;
        }

        int child_pid = fork();
        if( child_pid == 0 ){
            int ret = close(listen_socket);
            if( ret < 0 ) perror("close listen_socket error");

            service_function(client_socket, client_addr);

            ret = close(client_socket);
            if( ret < 0 ) perror("close client_socket error");
            exit(EXIT_SUCCESS);
        }
        else if( child_pid > 0 ){
            close(client_socket);
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

