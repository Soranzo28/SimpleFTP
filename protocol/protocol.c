#include "protocol.h"
#include <iso646.h>
#include "../helpers/helpers.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern Args args;

void read_and_parse_header(int sockfd, Header *header) {
  DEBUG_PRINT("[read_and_parse_header] Called\n");
  Header temp_buffer;
  size_t bytes_read = read(sockfd, &temp_buffer, sizeof(Header));
  DEBUG_PRINT("[read_and_parse_header] Header recivied:\n[Header] Type: %u\n[Header] Payload Size: %u\n[Header] Checksum: %u\n[Header] Filename: %s\n", temp_buffer.type, temp_buffer.payload_size, temp_buffer.checksum, temp_buffer.filename);
  if (bytes_read == sizeof(Header)) {
    memcpy(header, &temp_buffer, sizeof(Header));
    return;
  }

  memset(header, 0, sizeof(Header));
  header->type = MSG_ERROR;
  DEBUG_PRINT("[read_and_parse_header] Error on recevied header\n");
  return;
}

#define X(type, string)                                                        \
  case type: {                                                                 \
    printf("Type: %s\n", string);                                              \
  } break;

void print_header(Header header) {
  DEBUG_PRINT("[print_header] Called\n");
  switch (header.type) { MSG_TYPE }
  printf("Payload: %u\n", header.payload_size);
  printf("Checksum: %u\n", header.checksum);
  printf("File name: %s\n", header.filename);
}
#undef X

Header create_ack_header(char *filename) {
  DEBUG_PRINT("[create_ack_header] Called\n");
  Header header = {
      .type = MSG_ACK,
      .payload_size = 0,
      .checksum = 0,
  };
  strncpy(header.filename, filename, sizeof(header.filename) - 1);
  DEBUG_PRINT("[create_ack_header] Ack header created\n");
  return header;
}

Header create_error_header() {
  DEBUG_PRINT("[create_error_header] Called\n");
  Header header;

  memset(&header, 0, sizeof(Header));
  header.type = MSG_ERROR;
  DEBUG_PRINT("[create_error_header] Error header created\n");
  return header;
}

int handle_msg_send(Header header, sock_fd connection_fd, FILE **file,
                    const char *save_path) {
  DEBUG_PRINT("[handle_msg_send] Called!\n");
  FILE *new_file;

  uint8_t add_bar_char = 0;
  size_t save_path_size = strlen(save_path);
  size_t filename_size = strlen(header.filename);
  size_t filename_path_size = save_path_size + filename_size;
  DEBUG_PRINT("[handle_msg_send] Save path size: %lu\nFilename size: %lu\nTotal size: %lu\n", save_path_size, filename_size, filename_path_size);
  // The function accepts path with / or without, here we check and add 1 to the
  // size if to hold the new '/' if needed
  if (save_path[save_path_size - 1] != '/') {
    add_bar_char = 1;
    filename_path_size++;
    DEBUG_PRINT("[handle_msg_send] Added bar char, total size now: %lu\n", filename_path_size);
  }

  // Create buffer large enough to hold the filename and the path, +1 for the
  // null terminator \0
  char path[filename_path_size + 1];
  if (add_bar_char) {
    snprintf(path, sizeof(path), "%s/%s", save_path, header.filename);
  } else {
    snprintf(path, sizeof(path), "%s%s", save_path, header.filename);
  }
  DEBUG_PRINT("[handle_msg_send] Final save path: %s\n", path);

  new_file = fopen(path, "wb");
  if (!new_file) {
    fprintf(stderr, "Unable to create new file at: %s\n", path);
    return 1;
  }
  DEBUG_PRINT("[handle_msg_send] File at %s opened\n", path);
  Header ack_header = create_ack_header(header.filename);
  write(connection_fd, &ack_header, sizeof(Header));
  DEBUG_PRINT("[handle_msg_send] Ack header sent\n");
  *file = new_file;
  DEBUG_PRINT("[handle_msg_send] Returned successfully\n");
  return 0;
}

int write_new_file(Header header, sock_fd connection_fd, FILE *file) {
  DEBUG_PRINT("[write_new_file] Called\n");
  Header ack_header = create_ack_header(header.filename);
  Header now_header = {0};
  int resend_tries = 0;
  size_t chunks = 0;
  do {
    chunks++;
    // Read next header
    DEBUG_PRINT("[write_new_file] Reading header %lu (calls read_and_parse_header)\n", chunks);
    read_and_parse_header(connection_fd, &now_header);

    switch (now_header.type) {
    case MSG_DATA:
      break;
    case MSG_DONE:
      return 0;
      break;
    case MSG_ERROR:
      return 1;
    }

    //Calculate, check and read chunk
    uint16_t bytes_to_be_handled = now_header.payload_size;
    if (bytes_to_be_handled > CHUNK_SIZE)
    {
      DEBUG_PRINT("[write_new_file] Error: bytes recevied (%u) are bigger than max chunk size (%u)\n", bytes_to_be_handled, CHUNK_SIZE);
      return 1;
    }
    // Since the header has been read, now we read data
    uint8_t buffer[CHUNK_SIZE] = {0};
    uint16_t bytes_read = read(connection_fd, buffer, bytes_to_be_handled);
    DEBUG_PRINT("[write_new_file] Read chunk %lu successfully\n", chunks);


    // CRC32 calculation and verify
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)buffer, bytes_read);
    if (crc != now_header.checksum) {
      DEBUG_PRINT("[write_new_file] Error: CRC mismatch at chunk %lu: Expected: %lu | Got: %u\nTries left: %d\n",chunks, crc, now_header.checksum, resend_tries);
      if (resend_tries > MAX_RESEND_TRIES)
      {
        DEBUG_PRINT("[write_new_file] Error: Max chunk retries\n");
        return 1;
      }
        resend_tries++;
      send_error_header(connection_fd);
      DEBUG_PRINT("[write_new_file] Trying to recieve chunk again\n");
      continue;
    }
    DEBUG_PRINT("[write_new_file] CRC match successfully E: %lu | G: %u\n", crc, now_header.checksum);


    // Writes to the file

    DEBUG_PRINT("[write_new_file] Writing chunk %lu to file\n", chunks);
    uint16_t bytes_written =
        fwrite(buffer, sizeof(buffer[0]), bytes_read, file);
    resend_tries = 0;
    DEBUG_PRINT("[write_new_file] Chunk %lu wrote\n", chunks);

    // Sends ack
    write(connection_fd, &ack_header, sizeof(Header));
    DEBUG_PRINT("[write_new_file] Send ack to client\n");
    memset(&now_header, 0, sizeof(Header));
    
    DEBUG_PRINT("=============================\n");
  } while (1);
  DEBUG_PRINT("[write_new_file] File wrote\n");
}

void send_error_header(sock_fd connection_fd) {
  DEBUG_PRINT("[send_error_header] Called!\n"); 
  Header error_header = create_error_header();
  uint16_t bytes_sent = write(connection_fd, &error_header, sizeof(Header));
  DEBUG_PRINT("[send_error_header] Header sent\n");
}
