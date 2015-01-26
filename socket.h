#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <iostream>
#include <cstdint>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int socketfd_t;

const int IP_MAX_LEN = 32;

/* SocketAddr */
struct SocketAddr{
    /* only support ipv4 now */
    std::string ipv4_addr_str;
    uint16_t port_hbytes;

    SocketAddr();
    SocketAddr(const char *ipv4_addr_str, uint16_t port_hbytes);
    SocketAddr(std::string ipv4_addr_str, uint16_t port_hbytes);
    void set_sockaddr(uint32_t ipv4_nbytes, uint16_t port_nbytes);
    void get_sockaddr(uint32_t* ipv4_nbytes, uint16_t* port_nbytes);
    std::string to_str();

    void to_sockaddr_in(struct sockaddr_in& ret_addr);
    void from_sockaddr_in(struct sockaddr_in& input_addr);
};

int socket_bind(socketfd_t socketfd, SocketAddr& bind_addr);
int socket_connect(socketfd_t socketfd, SocketAddr& connect_addr);
socketfd_t socket_accept(socketfd_t socketfd, SocketAddr& accept_addr);

/* IPv4AddressSet */
struct IPv4AddressSet{
    uint32_t start_ip_nbyte;
    uint32_t netmask; // 0 ~ 32

    IPv4AddressSet();
    uint32_t get_netmask_nbyte();
    bool is_belong_to_set(uint32_t ip_nbyte);
    std::string to_str();
};

/* IP type translation */
/* system API
 * 
 * type translation:
 *  - int inet_aton(const char* cp, struct in_addr* inp) : string => nbytes binary
 *  - char* inet_ntoa(struct in_addr in)                 : nbytes binary => string
 *
 * network byte order <=> host byte order
 *  - uint32_t ntohl(uint32_t) : nbyte => hbyte
 *  - uint32_t htonl(uint32_t) : hbyte => nbyte
 */
std::string ip_nbyte_to_str(uint32_t ip_nbyte);
uint32_t ip_string_to_nbyte(std::string ip_str);
#endif
