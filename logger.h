#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

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
void free_logger(logger_t* logger);

#define GENERATE_LOGLEVEL_FUNCTION_DECLARATION(LEVEL, FUN_NAME) void FUN_NAME(logger_t* logger, const char* format, ...);
FOREACH_LOGLEVEL(GENERATE_LOGLEVEL_FUNCTION_DECLARATION)

#endif /* LOGGER_H */
