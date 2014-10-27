#ifndef __SOCKET_H__
#define __SOCKET_H__

typedef int socketfd_t;

const int IP_MAX_LEN = 32;

int socket_bind(socketfd_t socketfd, const char* ip_str, int port_hbytes);
socketfd_t socket_accept(socketfd_t socketfd, char* client_ip_str, int* client_port);

#endif
