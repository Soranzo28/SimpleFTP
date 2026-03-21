#ifndef NET_C
#define NET_C

#include <netinet/in.h>

typedef int sock_fd;

typedef struct 
{
  sock_fd fd;
  struct sockaddr_in addr;
} New_connection_info;

sock_fd get_tcp_sock_fd(unsigned int port, char* ip, int reuse_addr);
New_connection_info get_new_connection(sock_fd server_fd);
#endif
