#include "helpers.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

extern Args args;

// WARNING:
//  char** buffer MUST be at least 18 bytes long
int ip_to_string(in_addr_t in, char *buffer, size_t buffer_len) {
  if (buffer_len < 18) {
    DEBUG_PRINT("[ip_to_string] Error: buffer size is too small");
    memset(buffer, '!', buffer_len);
    return 1;
  }

  struct ip_octetes *ip = (struct ip_octetes *)&in;
  sprintf(buffer, "%d.%d.%d.%d", ip->first_octect, ip->second_octect,
          ip->third_octect, ip->fourth_octect);

  return 0;
}

void print_new_connection_ip(in_addr_t in) {
  char buffer[18];
  if (ip_to_string(in, buffer, sizeof(buffer)) == 1) {
    printf("Unable to print IP\n");
    return;
  }

  printf("Connected with: %s\n", buffer);
}
