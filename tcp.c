#include "tcp.h"

// defines the codes for the supported operations
typedef enum {FILES_LIST, SEND_FILE_REQUEST} sbcp_operation;

// see `send_tcp_line` below
typedef int (*handle_tcp_resp)();

// sends a `line` over to a remote TCP server
// if `line_length` is >= 0, will be assumed to be the length
// of the line; otherwise, the line should be null-terminated
// if a `recv_callback` is given, then it will be used to handle
// the server's reply
// returns 0 if success, anything else means error
// TODO wkpo encrypter - support more than one secret?
int send_tcp_line(const char* ip, const uint16_t port, const sbcp_operation operation,
                  const char* line, const size_t line_length,
                  handle_tcp_resp recv_callback, void* context)
{
  
}


