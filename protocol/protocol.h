#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../net/net.h"
#include <stdint.h>
#include <stdio.h>
#include <zlib.h>

#define MSG_TYPE                                                               \
  X(0x01, "MSG_SEND")                                                          \
  X(0x02, "MSG_ACK")                                                           \
  X(0x03, "MSG_DATA")                                                          \
  X(0x04, "MSG_DONE")                                                          \
  X(0x05, "MSG_ERROR")

#define CHUNK_SIZE 8192 // 8KB
#define MAX_RESEND_TRIES 3

#define MSG_SEND 0x01
#define MSG_ACK 0x02
#define MSG_DATA 0x03
#define MSG_DONE 0x04
#define MSG_ERROR 0x05

typedef struct {
  uint8_t type;          // tipo da mensagem
  uint16_t payload_size; // tamanho do chunk (max 65535 bytes)
  uint32_t checksum;     // CRC32 do arquivo inteiro
  char filename[255];    // nome do arquivo
} __attribute__((packed)) Header;

void read_and_parse_header(int sockfd, Header *header);
void print_header(Header header);
Header create_ack_header(char *filename);
void send_error_header(sock_fd connection_fd);
int handle_msg_send(Header header, sock_fd connection_fd, FILE **file);
int write_new_file(Header header, sock_fd connection_fd, FILE *file);

#endif
