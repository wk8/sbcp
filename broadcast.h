#ifndef BROADCAST_H
#define BROADCAST_H

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "logger.h"

// the 2 options below can be overriden at compile time
// the default broadcast port
#ifndef BROADCAST_PORT
#define BROADCAST_PORT 28287
#endif
// a unique code to verify sbcp pings
// this is not a safety feature, mind you.
// must be a uint32_t
// in particular, must stay below UINT32_MAX (i.e 4294967295)
#ifndef PING_CODE
#define PING_CODE 2093727718
#endif

// listens for pings - this is blocking
void broadcast_listen(logger_t* logger);
// broadcasts a ping
void broadcast_emit(logger_t* logger);

#endif /* BROADCAST_H */
