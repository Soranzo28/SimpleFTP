#include "protocol.h"
#include <iso646.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void read_and_parse_header(int sockfd, Header *header) {

  Header temp_buffer;
  size_t bytes_read = read(sockfd, &temp_buffer, sizeof(Header));

  if (bytes_read == sizeof(Header)) {
    memcpy(header, &temp_buffer, sizeof(Header));
    return;
  }

  memset(header, 0, sizeof(Header));
  header->type = MSG_ERROR;
  return;
}

#define X(type, string)                                                        \
  case type: {                                                                 \
    printf("Type: %s\n", string);                                              \
  } break;
void print_header(Header header) {
  switch (header.type) { MSG_TYPE }
  printf("Payload: %u\n", header.payload_size);
  printf("Checksum: %u\n", header.checksum);
  printf("File name: %s\n", header.filename);
}
#undef X

Header create_ack_header(char *filename) {
  Header header = {
      .type = MSG_ACK,
      .payload_size = 0,
      .checksum = 0,
  };
  strncpy(header.filename, filename, sizeof(header.filename) - 1);
  return header;
}

Header create_error_header() {
  Header header;

  memset(&header, 0, sizeof(Header));
  header.type = MSG_ERROR;

  return header;
}

int handle_msg_send(Header header, sock_fd connection_fd, FILE **file) {
  FILE *new_file;
  new_file = fopen(header.filename, "wb");
  if (!new_file)
    return 1;
  Header ack_header = create_ack_header(header.filename);
  write(connection_fd, &ack_header, sizeof(Header));
  *file = new_file;
  return 0;
}

int write_new_file(Header header, sock_fd connection_fd, FILE *file) {
  if (header.type == MSG_DONE) {
    return 0;
  }
  uint16_t bytes_to_be_handled = header.payload_size;

  if (bytes_to_be_handled > CHUNK_SIZE) {
    return 1;
  }
  uint8_t buffer[CHUNK_SIZE];
  uint16_t bytes_read = read(connection_fd, buffer, bytes_to_be_handled);

  uLong crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, (const Bytef *)buffer, bytes_read);
  printf("S: %lu | C: %u\n", crc, header.checksum);
  if (crc != header.checksum) return 1;

  uint16_t bytes_written = fwrite(buffer, sizeof(buffer[0]), bytes_to_be_handled, file);

  Header ack_header = create_ack_header(header.filename);
  write(connection_fd, &ack_header, sizeof(Header));

  Header next_header = {0};
  read_and_parse_header(connection_fd, &next_header);
  switch (next_header.type) {
  case MSG_DATA:
    if (write_new_file(next_header, connection_fd, file) == 1)
      return 1;
    break;
  }
  return 0;
}

void send_error_header(sock_fd connection_fd) {
  Header error_header = create_error_header();
  uint16_t bytes_sent = write(connection_fd, &error_header, sizeof(Header));
}
