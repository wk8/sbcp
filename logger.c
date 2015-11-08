#include "logger.h"

#define LINE_MAX_LENGTH 5000

logger_t* init_logger(const char* log_file, const log_level level)
{
  logger_t* logger = (logger_t*) calloc(1, sizeof(logger_t));
  // FIXME: check the file is open
  // FIXME: what if the file gets destroyed (eg logrotate?)
  logger->file = fopen(log_file, "a+");
  logger->level = level;
  logger->stdout_is_a_tty = isatty(fileno(stdout));
  // +1 to be able to append a new line char
  logger->buffer = (char*) malloc((LINE_MAX_LENGTH + 2) * sizeof(char));
  return logger;
}

void free_logger(logger_t* logger)
{
  fclose(logger->file);
  if (logger->buffer != NULL) {
    free(logger->buffer);
  }
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
  size_t current_length, remaining_space;
  int max_written_chars_count;
  char* buffer = logger->buffer;

  if (buffer == NULL) {
    perror("logger: no buffer to work with");
    return;
  }

  // build the line
	gettimeofday(&tv, NULL);
  max_written_chars_count = snprintf(buffer,
                                     LINE_MAX_LENGTH,
                                     "[ %lu-%lu (%d) ] %s: ",
                                     (unsigned long)tv.tv_sec,
                                     (unsigned long)tv.tv_usec,
                                     (int) getpid(),
                                     level_name(level));
  if (max_written_chars_count < 0) {
    perror("logger: encoding error");
    return;
  } else if (max_written_chars_count >= LINE_MAX_LENGTH) {
    perror("logger: encoding error");
    return;
  }
  current_length = max_written_chars_count;
  remaining_space = LINE_MAX_LENGTH - current_length,
	max_written_chars_count = vsnprintf(buffer + current_length,
                                      remaining_space,
                                      format,
                                      *args);
  if (max_written_chars_count < 0) {
    perror("logger: encoding error");
    return;
  }
  if (max_written_chars_count < remaining_space) {
    current_length += max_written_chars_count;
  } else {
    current_length += remaining_space;
  }
  // apppend a new line char
  buffer[current_length] = '\n';
  buffer[current_length + 1] = 0;

  // write to sdtout, if applicable
  if (logger->stdout_is_a_tty) {
    printf(buffer);
  }

  // and write to file
	fprintf(logger->file, buffer);
  fflush(logger->file);
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
