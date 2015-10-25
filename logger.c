#include "logger.h"

logger_t* init_logger(const char* log_file, const log_level level)
{
  logger_t* logger = (logger_t*) calloc(1, sizeof(logger_t));
  // FIXME: check the file is open
  // FIXME: what if the file gets destroyed (eg logrotate?)
  logger->file = fopen(log_file, "a+");
  logger->level = level;
  return logger;
}

void free_logger(logger_t* logger)
{
  fclose(logger->file);
  free(logger);
}

#define GENERATE_LOGLEVEL_NAME(LEVEL, FUN_NAME) case LEVEL: return #LEVEL;
const char* level_name(const log_level level)
{
  switch(level) {
    FOREACH_LOGLEVEL(GENERATE_LOGLEVEL_NAME)
  }
}

void log_line(logger_t* logger, const log_level level, const char* format, va_list* args)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	fprintf(logger->file, "[ %lu-%lu (%d) ] %s: ", (unsigned long)tv.tv_sec, (unsigned long)tv.tv_usec, (int) getpid(), level_name(level));
	vfprintf(logger->file, format, *args);
	fprintf(logger->file, "\n");
}

#define GENERATE_LOGLEVEL_FUNCTION_IMPL(LEVEL, FUN_NAME) \
void FUN_NAME(logger_t* logger, const char* format, ...) \
{ \
  va_list args; \
  if (LEVEL < logger->level) { \
    return; \
  } \
  va_start(args, format); \
  log_line(logger, LEVEL, format, &args); \
  va_end(args); \
}
FOREACH_LOGLEVEL(GENERATE_LOGLEVEL_FUNCTION_IMPL)
