#include "helpers/helpers.h"
#include "net/net.h"
#include "protocol/protocol.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>


#define PORT 9000

int main() {
  int server_fd = get_tcp_sock_fd(PORT, INADDR_ANY, 1);

  if (server_fd < 0) {
    fprintf(stderr, "Unable to create server!\n");
    return 1;
  }

  printf("Listening at port %d\n", PORT);

  while (1) {
    New_connection_info new_connection_info = get_new_connection(server_fd);
    FILE *file = NULL;
    in_addr_t new_connection_raw_addr =
        new_connection_info.addr.sin_addr.s_addr;
    sock_fd new_connection_fd = new_connection_info.fd;

    if (new_connection_fd < 0) {
      continue;
    }

    print_new_connection_ip(new_connection_raw_addr);

    Header header;
    read_and_parse_header(new_connection_fd, &header);

    if (header.type != MSG_SEND && file == NULL) {
      send_error_header(new_connection_fd);
      continue;
    }

    if (handle_msg_send(header, new_connection_fd, &file) == 1) {
      send_error_header(new_connection_fd);
      continue;
    }

    if (write_new_file(header, new_connection_fd, file) == 1) {
      send_error_header(new_connection_fd);
      fclose(file);
      close(new_connection_fd);
      fprintf(stderr, "Error recieving file!\n");
      continue;
    }
    fclose(file);
    close(new_connection_fd);
    printf("File recieved succesfully!\n");
  }

  close(server_fd);

  return 0;
}
