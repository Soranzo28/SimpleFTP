#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../types.h"

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

void read_and_parse_header(ConnectionContext*);
int handle_msg_send(ConnectionContext *cx, char* save_path);
int write_new_file(ConnectionContext* cx);
void check_sucessfull_file_recv(ConnectionContext* cx);

#endif
