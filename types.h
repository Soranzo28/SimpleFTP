#ifndef TYPES_H
#define TYPES_H

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <zlib.h>

typedef int sock_fd;

typedef struct {
  uint8_t type;          // tipo da mensagem
  uint16_t payload_size; // tamanho do chunk (max 65535 bytes)
  uint32_t checksum;     // CRC32 do arquivo inteiro
  char filename[255];    // nome do arquivo
} __attribute__((packed)) Header;

typedef struct {
  unsigned int debug;
  char *ip;
  unsigned int port;
  char *save_path;
} Args;

typedef struct {
  Header actual_header;
  sock_fd fd;
  uLong crc;
  FILE *file;
  char *path;
  char addr[18];
  uint64_t chunk_count;
} ConnectionContext;

struct ip_octetes {
  uint8_t first_octect;
  uint8_t second_octect;
  uint8_t third_octect;
  uint8_t fourth_octect;
};

#endif
