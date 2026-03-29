#ifndef NET_C
#define NET_C

#include <netinet/in.h> 
#include "../types.h"

sock_fd get_tcp_sock_fd(unsigned int port, char* ip, int reuse_addr);
ConnectionContext get_new_connection(sock_fd server_fd);
#endif
