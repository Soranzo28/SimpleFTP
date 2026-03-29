#include "../types.h"
#include "../helpers/helpers.h"
#include "../protocol/protocol.h"
#include <iso646.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

extern Args args;

void read_and_parse_header(ConnectionContext* connectionContext) {
  DEBUG_PRINT("[read_and_parse_header] Called\n");
  Header temp_buffer = {0};
  ssize_t bytes_read = 0, total_bytes_read = 0;

  while (total_bytes_read < (ssize_t)sizeof(Header))
  {
    bytes_read = read(connectionContext->fd, (char*)&temp_buffer+total_bytes_read, sizeof(Header)-total_bytes_read);
    if (bytes_read <= 0)
    {
      temp_buffer.type = MSG_ERROR; 
      DEBUG_PRINT("[read_and_parse_header] Error: %s (ret: %ld)\n", strerror(errno), bytes_read);
      return;
    }
    total_bytes_read += bytes_read;
  }
  DEBUG_PRINT("[read_and_parse_header] Total bytes read: %lu / %ld\n", total_bytes_read, sizeof(Header));

  DEBUG_PRINT(
      "[read_and_parse_header] Header recivied:\n[Header] Type: %u\n[Header] "
      "Payload Size: %u\n[Header] Checksum: %u\n[Header] Filename: %s\n",
      temp_buffer.type, temp_buffer.payload_size, temp_buffer.checksum,
      temp_buffer.filename);

  connectionContext->actual_header = temp_buffer;

  return;
}

int handle_msg_send(ConnectionContext* cx, char* save_path)
{
  DEBUG_PRINT("[handle_msg_send] Called!\n");

  char *path = get_path(cx, save_path);
  if (!path) return 1;

  FILE* file = get_file_pointer(path);  
  if (!file) return 1;
  cx->file = file; 
  DEBUG_PRINT("[handle_msg_send] File at %s opened\n", path);
  send_ack_header(cx->fd, cx->actual_header.filename);

  DEBUG_PRINT("[handle_msg_send] Returned successfully\n");
  return 0;
}

int write_new_file(ConnectionContext* cx)
{
  DEBUG_PRINT("[write_new_file] Called\n");
  int resend_tries = 0;
  cx->crc = crc32(0L, Z_NULL, 0);
  uint8_t* buffer = calloc(CHUNK_SIZE, 1);
  do {
    // Read next header
    DEBUG_PRINT( "[write_new_file] Reading header %lu (calls read_and_parse_header)\n", cx->chunk_count);
    read_and_parse_header(cx);

    switch (cx->actual_header.type)
    {
      case MSG_DATA:
        break;
      case MSG_DONE:
        free(buffer);
        return 0;
        break;
      case MSG_ERROR:
        free(buffer);
        return 1;
    }

    // read_data function returns 1 in case of error or connection closed 
    if (read_data(cx, buffer)) {free(buffer); return 1;}

    uLong old_crc = cx->crc;
    // calculate crc from buffer and stores at field CRC on struct 
    calculate_crc(cx, buffer, cx->actual_header.payload_size);

    // only takes one arg because we compare the calculated crc with the header, which is already on the struct
    if (compare_crc(cx))
    {
      if (resend_tries > MAX_RESEND_TRIES) 
      {
        DEBUG_PRINT("[write_new_file] Error: Max chunk retries\n");
        cx->crc = old_crc;
        free(buffer);
        return 1;
      }
      resend_tries++;
      send_error_header(cx->fd);
      DEBUG_PRINT("[write_new_file] Trying to recieve chunk again\n");
      continue;
    }

  
    DEBUG_PRINT("[write_new_file] CRC match successfully E: %lu | G: %u\n", cx->crc, cx->actual_header.checksum);

    // Writes to the file

    DEBUG_PRINT("[write_new_file] Writing chunk %lu to file\n", cx->chunk_count);
    fwrite(buffer, sizeof(buffer[0]), cx->actual_header.payload_size, cx->file);
    resend_tries = 0;
    DEBUG_PRINT("[write_new_file] Chunk %lu wrote\n", cx->chunk_count);

    // Sends ack
    send_ack_header(cx->fd, cx->actual_header.filename);
    DEBUG_PRINT("[write_new_file] Send ack to client\n");
    memset(&cx->actual_header, 0, sizeof(Header));

    DEBUG_PRINT("=============================\n");
  } while (1);
  free(buffer);
  DEBUG_PRINT("[write_new_file] File wrote\n");
}

void check_sucessfull_file_recv(ConnectionContext* cx) {
  DEBUG_PRINT("[check_sucessfull_file_recv] Checking hole file CRC\n");
  if (cx->crc != cx->actual_header.checksum) {
    DEBUG_PRINT("[check_sucessfull_file_recv] CRC mismatch! E: %lu | G: %u , removing file\n", cx->crc, cx->actual_header.checksum);
    remove(cx->path);
    return;
  }
  DEBUG_PRINT("[check_sucessfull_file_recv] CRC match E: %lu | G: %u , everything ok\n",
              cx->crc, cx->actual_header.checksum);
}

