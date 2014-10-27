#include <cstring>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"

using namespace std;

int socket_bind(socketfd_t socketfd, const char* ip_str, int port_hbytes){
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    inet_aton(ip_str, &(bind_addr.sin_addr));
    bind_addr.sin_port = htons((uint16_t)port_hbytes);

    return bind(socketfd, (struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in));
}

socketfd_t socket_accept(socketfd_t socketfd, char* client_ip_str, int* client_port){
    struct sockaddr_in client_addr;
    size_t client_addr_len = sizeof(struct sockaddr_in);
    int connection_fd = accept(socketfd, (struct sockaddr*)&client_addr, &client_addr_len);

    if( connection_fd < 0 )
        /* accept error */
        return connection_fd;

    strncpy(client_ip_str, inet_ntoa(client_addr.sin_addr), IP_MAX_LEN);
    *client_port = ntohs(client_addr.sin_port);
    return connection_fd;
}
