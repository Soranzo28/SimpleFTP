#ifndef HELPERS_C 
#define HELPERS_C

#define HELPERS_C
#include <arpa/inet.h>
#include <stdint.h>

struct ip_octetes {
  uint8_t first_octect;
  uint8_t second_octect;
  uint8_t third_octect;
  uint8_t fourth_octect;
};

static int ip_to_string(in_addr_t, char *, size_t);
void print_new_connection_ip(in_addr_t);

#endif
