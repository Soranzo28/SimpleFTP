#include "server.h"
#include "../helpers/helpers.h"
#include "../net/net.h"
#include "../protocol/protocol.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Args args;

void get_args(int argc, char **argv) {
  args.debug = 0;
  args.ip = "0.0.0.0";
  args.port = 9000;
  args.save_path = ".";

  char *help_message =
      "Usage: ./server [--debug] [--ip IP] [--port PORT] [--sp SAVE PATH]\n";

  static struct option long_options[] = {
      {"debug", no_argument, 0, 'd'},      {"ip", required_argument, 0, 'i'},
      {"port", required_argument, 0, 'p'}, {"sp", required_argument, 0, 's'},
      {"help", no_argument, 0, 'h'},       {0, 0, 0, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "di:p:s:h", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'd':
      args.debug = 1;
      break;
    case 'i':
      args.ip = optarg;
      if (strcmp(args.ip, "localhost") == 0)
        args.ip = "127.0.0.1";
      break;
    case 'p':
      args.port = atoi(optarg);
      break;
    case 's':
      args.save_path = optarg;
      break;
    case '?':
      fprintf(stderr, "%s", help_message);
      exit(1);
    case 'h':
      printf("%s", help_message);
      exit(0);
    }
  }
  DEBUG_PRINT("[get_args] Returned succesfully\n");
}

int main(int argc, char **argv) {
  get_args(argc, argv);
  DEBUG_PRINT("[main] Called\n");

  DEBUG_PRINT("[main] Args:\nDebug: %d\nIp: %s\nPort(bin): %u\nSave path: %s\n",
              args.debug, args.ip, args.port, args.save_path);
  const unsigned int PORT = args.port;
  const char *SAVE_PATH = args.save_path;

  DEBUG_PRINT("[main] Calls get_tcp_sock_fd\n");
  int server_fd = get_tcp_sock_fd(PORT, "127.0.0.1", 1);

  if (server_fd < 0) {
    fprintf(stderr, "Unable to create server!\n");
    return 1;
  }

  printf("Listening at %s:%d\n", args.ip, args.port);

  DEBUG_PRINT("[main] Waits for connection\n");
  while (1) {
    DEBUG_PRINT("[main] Calls get_new_connection\n");
    New_connection_info new_connection_info = get_new_connection(server_fd);
    DEBUG_PRINT("[main] Connection recevied\n");
    FILE *file = NULL;
    in_addr_t new_connection_raw_addr =
        new_connection_info.addr.sin_addr.s_addr;
    sock_fd new_connection_fd = new_connection_info.fd;

    if (new_connection_fd < 0) {
      continue;
    }

    DEBUG_PRINT("[main] Calls print_new_connection_ip\n");
    print_new_connection_ip(new_connection_raw_addr);

    Header header;
    DEBUG_PRINT("[main] Calls read_and_parse_header\n");
    read_and_parse_header(new_connection_fd, &header);

    if (header.type != MSG_SEND && file == NULL) {
      DEBUG_PRINT("[main] Calls send_error_header\n");
      send_error_header(new_connection_fd);
      continue;
    }

    DEBUG_PRINT("[main] Calls handle_msg_send\n");
    char *total_path;
    if (handle_msg_send(header, new_connection_fd, &file, SAVE_PATH,
                        &total_path) == 1) {
      DEBUG_PRINT("[main] Calls send_error_header\n");
      send_error_header(new_connection_fd);
      continue;
    }

    DEBUG_PRINT("[main] Calls write_new_file\n");
    if (write_new_file(header, new_connection_fd, file, total_path) == 1) {
      DEBUG_PRINT("[main] Calls send_error_header\n");
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
  DEBUG_PRINT("[main] Returns succesfully\n");
  return 0;
}
