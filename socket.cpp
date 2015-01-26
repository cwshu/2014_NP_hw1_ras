// #include <iostream>
#include <cstring>
#include <cstdint>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"

using namespace std;

/* SocketAddr */
SocketAddr::SocketAddr(){
    port_hbytes = 0;
}

SocketAddr::SocketAddr(const char *ipv4_addr_str, uint16_t port_hbytes){ 
    this->ipv4_addr_str = std::string(ipv4_addr_str);
    this->port_hbytes = port_hbytes;
}

SocketAddr::SocketAddr(std::string ipv4_addr_str, uint16_t port_hbytes){
    this->ipv4_addr_str = std::string(ipv4_addr_str);
    this->port_hbytes = port_hbytes;
}

void SocketAddr::set_sockaddr(uint32_t ipv4_nbytes, uint16_t port_nbytes){
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = ipv4_nbytes;
    sock_addr.sin_port = port_nbytes;

    from_sockaddr_in(sock_addr);
}

void SocketAddr::get_sockaddr(uint32_t* ipv4_nbytes, uint16_t* port_nbytes){
    struct sockaddr_in sock_addr;
    to_sockaddr_in(sock_addr);
    
    *ipv4_nbytes = sock_addr.sin_addr.s_addr;
    *port_nbytes = sock_addr.sin_port;
}

std::string SocketAddr::to_str(){
    return ipv4_addr_str + string(":") + std::to_string(port_hbytes);
}

void SocketAddr::to_sockaddr_in(struct sockaddr_in& ret_addr){
    memset(&ret_addr, 0, sizeof(struct sockaddr_in));
    ret_addr.sin_family = AF_INET;
    inet_aton(ipv4_addr_str.c_str(), &(ret_addr.sin_addr));
    ret_addr.sin_port = htons((uint16_t)port_hbytes);
}

void SocketAddr::from_sockaddr_in(struct sockaddr_in& input_addr){
    ipv4_addr_str = std::string(inet_ntoa(input_addr.sin_addr));
    port_hbytes = ntohs(input_addr.sin_port);
}

int socket_bind(socketfd_t socketfd, SocketAddr& bind_addr){
    struct sockaddr_in addr;
    bind_addr.to_sockaddr_in(addr);

    return bind(socketfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
}

int socket_connect(socketfd_t socketfd, SocketAddr& connect_addr){
    struct sockaddr_in addr;
    connect_addr.to_sockaddr_in(addr);

    return connect(socketfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
}

socketfd_t socket_accept(socketfd_t socketfd, SocketAddr& accept_addr){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int connection_fd = accept(socketfd, (struct sockaddr*)&client_addr, &client_addr_len);

    if( connection_fd < 0 )
        /* accept error */
        return connection_fd;

    accept_addr.from_sockaddr_in(client_addr);
    return connection_fd;
}

/* IPv4AddressSet */
IPv4AddressSet::IPv4AddressSet(){
    // all IPv4 address
    start_ip_nbyte = 0;
    netmask = 0;
}

uint32_t IPv4AddressSet::get_netmask_nbyte(){
    /* netmask to netmask_nbyte
     *  0 => 0x000 ... 00 (32 bits uint)
     *  1 => 0x100 ... 00 (32 bits uint)
     *  2 => 0x110 ... 00 (32 bits uint)
     *             ...
     * 32 => 0x111 ... 11 (32 bits uint)
     *
     * method:
     *  netmask_hbyte: 1 => 31 => 0x011 ... 11 (2^31 - 1) => 0x100 ... 00 (bitwise not)
     */
    if( netmask == 0 ){
        return 0;
    }

    uint32_t reverse_netmask_hbyte = (1u << (32u - netmask)) - 1u;
    uint32_t netmask_hbyte = ~reverse_netmask_hbyte; // (bitwise not)
    uint32_t netmask_nbyte = htonl(netmask_hbyte);
    return netmask_nbyte;
}

bool IPv4AddressSet::is_belong_to_set(uint32_t ip_nbyte){
    uint32_t netmask_nbyte = get_netmask_nbyte();
    // std::cout << std::hex << start_ip_nbyte << " " << ip_nbyte << " " << netmask_nbyte << "\n";
    return (start_ip_nbyte & netmask_nbyte) == (ip_nbyte & netmask_nbyte);
}

std::string IPv4AddressSet::to_str(){
    // CIDR format
    std::string ip_str = ip_nbyte_to_str(start_ip_nbyte);
    // "ip_str/netmask"
    ip_str = ip_str + "/" + std::to_string(netmask);
    return ip_str;
}

/* IP type translation */
std::string ip_nbyte_to_str(uint32_t ip_nbyte){
    struct in_addr ip_in_addr;
    ip_in_addr.s_addr = ip_nbyte;
    std::string ip_str = string(inet_ntoa(ip_in_addr));
    return ip_str;
}

uint32_t ip_string_to_nbyte(std::string ip_str){
    struct in_addr ip_in_addr;
    inet_aton(ip_str.c_str(), &ip_in_addr);
    return ip_in_addr.s_addr;
}
