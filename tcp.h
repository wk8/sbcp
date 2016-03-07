#ifndef SBCP_TCP_H
#define SBCP_TCP_H

#include <inttypes.h>

#include "logger.h"

// forks a child process that listens for incoming TCP connections
// returns 0 if success, anything else means error
// copies the port it binds to to `tcp_port`
// TODO wkpo encrypter!
int fork_tcp_server(uint16_t* tcp_port);

// sends the list of `files` to the specified `host` and `port`
// returns 0 if success, anything else means error
int send_files_list(/*TODO wkpo*/);

// requests the file with the given `md5_sha` to be sent to the
// local client from the specified `host` and `port`
// returns 0 if success, anything else means error
int request_file(/*TODO wkpo*/);

#endif /* SBCP_TCP_H */
