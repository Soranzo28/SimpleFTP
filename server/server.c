#include "../helpers/helpers.h"
#include "../net/net.h"
#include "../protocol/protocol.h"
#include "../types.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

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
  DEBUG_PRINT("[main] Called\n");

  get_args(argc, argv);

  DEBUG_PRINT("[main] Args:\nDebug: %d\nIp: %s\nPort(bin): %u\nSave path: %s\n",
              args.debug, args.ip, args.port, args.save_path);

  DEBUG_PRINT("[main] Calls get_tcp_sock_fd\n");
  int server_fd = get_tcp_sock_fd(args.port, "127.0.0.1", 1);

  if (server_fd < 0) {
    fprintf(stderr, "Unable to create server!\n");
    return 1;
  }

  printf("Listening at %s:%d\n", args.ip, args.port);

  while (1) {
    DEBUG_PRINT("[main] Waits for connection\n");

    // Inicialize connectionContext struct with connetion fd and ip
    ConnectionContext connectionContext = get_new_connection(server_fd);

    if (connectionContext.fd < 0) {
      DEBUG_PRINT("[main] Error recieving new connection, skiping this one\n");
      continue;
    }
    DEBUG_PRINT("[main] New connection recevied\n");

    DEBUG_PRINT("[main] Calls read_and_parse_header\n");
    read_and_parse_header(&connectionContext);



    if (connectionContext.actual_header.type != MSG_SEND) {
      DEBUG_PRINT("[main] Error: Msg type is not SEND and there is no file "
                  "transfering going on\n");
      send_error_header(connectionContext.fd);
      continue;
    }

    DEBUG_PRINT("[main] Calls handle_msg_send\n");

    if (handle_msg_send(&connectionContext, args.save_path) == 1) {
      DEBUG_PRINT("[main] Calls send_error_header\n");
      send_error_header(connectionContext.fd);
      continue;
    }

    DEBUG_PRINT("[main] Calls write_new_file\n");
    if (write_new_file(&connectionContext) == 1) {
      DEBUG_PRINT("[main] Calls send_error_header\n");
      send_error_header(connectionContext.fd);
      fclose(connectionContext.file);
      close(connectionContext.fd);
      fprintf(stderr, "Error recieving file!\n");
      continue;
    }
    fclose(connectionContext.file);
    close(connectionContext.fd);
    printf("File recieved succesfully!\n");
  }

  close(server_fd);
  DEBUG_PRINT("[main] Returns succesfully\n");
  return 0;
}
