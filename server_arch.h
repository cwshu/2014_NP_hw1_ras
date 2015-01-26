#ifndef __SERVER_ARCH_H__
#define __SERVER_ARCH_H__

#include "socket.h"
#include "io_wrapper.h"

typedef void (*OneConnectionService)(socketfd_t connection_socket, SocketAddr& connection_addr);
    /* for example, telnet_service, http_service */

socketfd_t bind_and_listen_tcp_socket(SocketAddr& listen_addr);

void start_multiprocess_server(socketfd_t listen_socket, OneConnectionService service_function);
    /* helping closing client_socket, so service_function doesn't need close it.
     * wait at receive SIGCHLD, release child resource for multiprocess && concurrent server */

/* start_multiprocess_server sub functions */
void sigchid_waitfor_child(int sig);

#endif
