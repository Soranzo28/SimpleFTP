#include "net.h"
#include <asm-generic/socket.h>
#include <iso646.h>
#include <stdio.h>

sock_fd get_tcp_sock_fd(unsigned int port, unsigned int ip, int reuse_addr) {
  sock_fd sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = ip};

  if (reuse_addr > 0) {
    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
      perror("setsockopt");
    }
  }

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return -1;
  }

  if (listen(sock, 5) < 0) {
    perror("listen");
    return -1;
  }

  return sock;
}

New_connection_info get_new_connection(sock_fd server_fd) {
  struct sockaddr_in new_connection_addr;
  New_connection_info new_connection_info = {0};

  socklen_t new_connection_len = sizeof(new_connection_addr);
  sock_fd new_connection = accept(
      server_fd, (struct sockaddr *)&new_connection_addr, &new_connection_len);
  if (new_connection == -1) {
    perror("accept");
    new_connection_info.fd = -1;
    return new_connection_info;
  }

  new_connection_info.fd = new_connection;
  new_connection_info.addr = new_connection_addr;

  return new_connection_info;
}
