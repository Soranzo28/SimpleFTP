#ifndef HELPERS_C 
#define HELPERS_C

#define HELPERS_C
#include <arpa/inet.h>
#include "../types.h"

#define DEBUG_PRINT(fmt, ...) \
    if (args.debug) printf(fmt, ##__VA_ARGS__)

int ip_to_string(in_addr_t in, char buffer[16], size_t buffer_len);
char* get_path(ConnectionContext* cx, char* save_path);
FILE* get_file_pointer(char *path);
void print_header(Header header);
Header create_ack_header(char *filename);
void send_error_header(sock_fd connection_fd);
uLong calculate_crc(ConnectionContext* cx, void *buf, size_t buf_len);
void send_ack_header(sock_fd connection_fd, char* filename);  
int is_save_path_safe(const char *path);
void sanitize_filename(char *filename);
int read_data(ConnectionContext* cx, uint8_t* buffer);
int compare_crc(ConnectionContext* cx);



#endif
