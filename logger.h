#ifndef LOGGER_H
#define LOGGER_H

// FIXME: stderr for >= WARNING, stdout for rest, as an option

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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
} logger_t;

logger_t* init_logger(const char* log_file, const log_level level);
void free_logger(logger_t* logger);

#define GENERATE_LOGLEVEL_FUNCTION_DECLARATION(LEVEL, FUN_NAME) void FUN_NAME(logger_t* logger, const char* format, ...);
FOREACH_LOGLEVEL(GENERATE_LOGLEVEL_FUNCTION_DECLARATION)

#endif /* LOGGER_H */
