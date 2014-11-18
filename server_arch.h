#ifndef __SERVER_ARCH_H__
#define __SERVER_ARCH_H__

#include "socket.h"
typedef void (*OneConnectionService)(socketfd_t connection_socket); 
    /* for example, telnet_service, http_service */

void start_multiprocess_server(socketfd_t listen_socket, OneConnectionService service_function);

/* start_multiprocess_server sub functions */
void sigchid_waitfor_child(int sig);

#endif
