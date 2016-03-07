#ifndef SBCP_LOGGER_H
#define SBCP_LOGGER_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// FIXME: autoamtic strerror(errno) if errno is set (requires an audit of all func calls that can set errno to check we do handle them properly!)

#define FOREACH_LOGLEVEL(LEVEL) \
        LEVEL(DEBUG, debug) \
        LEVEL(INFO, info) \
        LEVEL(WARNING, warning) \
        LEVEL(ERROR, error) \
        LEVEL(FATAL, fatal)

#define GENERATE_LOGLEVEL_ENUM(LEVEL, FUN_NAME) LEVEL,
typedef enum {
  FOREACH_LOGLEVEL(GENERATE_LOGLEVEL_ENUM)
} log_level;

typedef struct logger_t {
  FILE* file;
  log_level level;
  int stdout_is_a_tty;
  char* buffer;
} logger_t;

logger_t* init_logger(const char* log_file, const log_level level);
void free_logger(logger_t* logger); // TODO wkpo make sure it's called

#define GENERATE_LOGLEVEL_FUNCTION_DECLARATION(LEVEL, FUN_NAME) void FUN_NAME(logger_t* logger, const char* format, ...);
FOREACH_LOGLEVEL(GENERATE_LOGLEVEL_FUNCTION_DECLARATION)

#endif /* SBCP_LOGGER_H */
