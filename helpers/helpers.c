#include "helpers.h"
#include "../types.h"
#include "../protocol/protocol.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

extern Args args;


//Minimal of 16 bytes ("xxx.xxx.xxx.xxx" == 15 + \0 )
int ip_to_string(in_addr_t in, char buffer[16], size_t buffer_len) {
  //Converts big_endian to little_endian if needed
  in_addr_t host_order_ip = ntohl(in);
  
  if (buffer_len < 16)
  {
    DEBUG_PRINT("[ip_to_string] Error: buffer size is too small");
    return 1;
  }

  struct ip_octetes *ip = (struct ip_octetes *)&host_order_ip;
  snprintf(buffer, buffer_len, "%d.%d.%d.%d", ip->first_octect, ip->second_octect, ip->third_octect, ip->fourth_octect);

  return 0;
}

char* get_path(ConnectionContext *cx, char* save_path)
{
  if (!is_save_path_safe(save_path)) 
  {
    fprintf(stderr, "[get_path] Invalid save path");
    return NULL;
  }

  sanitize_filename(cx->actual_header.filename); 

  uint8_t add_bar_char = 0;
  size_t save_path_size = strlen(save_path);
  size_t filename_size = strlen(cx->actual_header.filename);
  size_t filename_path_size = save_path_size + filename_size;
  DEBUG_PRINT("[get_path] Save path size: %lu\nFilename size: "
              "%lu\nTotal size: %lu\n",
              save_path_size, filename_size, filename_path_size);
  // The function accepts path with / or without, here we check and add 1 to the
  // size if to hold the new '/' if needed
  if (save_path[save_path_size - 1] != '/') {
    add_bar_char = 1;
    filename_path_size++;
    DEBUG_PRINT("[get_path] Added bar char, total size now: %lu\n",
                filename_path_size);
  }

  // Create buffer large enough to hold the filename and the path, +1 for the
  // null terminator \0
  char *path = calloc(filename_path_size + 1, 1);
  if (add_bar_char) {
    snprintf(path, filename_path_size + 1, "%s/%s", save_path, cx->actual_header.filename);
  } else {
    snprintf(path, filename_path_size, "%s%s", save_path, cx->actual_header.filename);
  }
  
  cx->path = path;

  return path;
}

FILE* get_file_pointer(char *path)
{
  FILE* file = fopen(path, "wb");
  if (!file) {
    fprintf(stderr, "Unable to create new file at: %s\n", path);
    return NULL;
  }
  return file;
}

// ================== CREATES ==================
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
// ================== END CREATES ==================


// ================== SENDERS ==================


void send_error_header(sock_fd connection_fd) {
  DEBUG_PRINT("[send_error_header] Called!\n");
  Header error_header = create_error_header();
  write(connection_fd, &error_header, sizeof(Header));
  DEBUG_PRINT("[send_error_header] Header sent\n");
}

void send_ack_header(sock_fd connection_fd, char* filename) {  
  DEBUG_PRINT("[send_ack_header] Called!\n");
  Header error_header = create_ack_header(filename);
  write(connection_fd, &error_header, sizeof(Header));
  DEBUG_PRINT("[send_ack_header] Header sent\n");
}

// ================== END SENDERS ==================

// ================== HELPERS TO HELPERS ================

int is_save_path_safe(const char *path) {
    //Checks if the path starts with root '/' and checks if it is a directory 
    struct stat s;
    if (strcmp(path, "/") == 0 || strlen(path) == 0) return 0;

    if (stat(path, &s) == 0 && S_ISDIR(s.st_mode)) {
        return 1;
    }
    return 0;
}

void sanitize_filename(char *filename) {
    //Removes any '/' or '\\'
    for (int i = 0; filename[i]; i++) {
        if (filename[i] == '/' || filename[i] == '\\') {
            filename[i] = '_'; 
        }
    }
    //Removes hidden file indicator 
    if (filename[0] == '.') filename[0] = '_';
}

// ================== END HELPERS TO HELPERS ================

uLong calculate_crc(ConnectionContext* cx, void *buf, size_t buf_len) 
{

  cx->crc = crc32(cx->crc, buf, buf_len);
  DEBUG_PRINT("[crc] Chunk %lu - CRC: %lu\n", cx->chunk_count, cx->crc);
  return cx->crc;
}

int compare_crc(ConnectionContext* cx)
{
  if (cx->crc == cx->actual_header.checksum) return 0 ;
  DEBUG_PRINT("[write_new_file] Error: CRC mismatch at chunk %lu, expected: %lu | got: %u\n", cx->chunk_count, cx->crc, cx->actual_header.checksum);
  return 1;
}

int read_data(ConnectionContext* cx, uint8_t* buffer)
{
  uint16_t length = cx->actual_header.payload_size;   

  if (length > CHUNK_SIZE)
  {
      DEBUG_PRINT("[write_new_file] Error: bytes recevied (%u) are bigger than max chunk size (%u)\n", length, CHUNK_SIZE);
      return 1;
  }

  cx->chunk_count++;
  uint16_t bytes_read = 0;  

  do{

    ssize_t temp = read(cx->fd, buffer+bytes_read, length-bytes_read);
    if (temp <= 0) return 1;
    bytes_read += (uint16_t)temp;

  }while(bytes_read < length);

  DEBUG_PRINT("[write_new_file] Read chunk %lu successfully\n", cx->chunk_count);
  return 0; 
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







