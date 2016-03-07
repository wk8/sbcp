#ifndef SBCP_FILES_H
#define SBCP_FILES_H

#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logger.h"

typedef struct files_t {
  char* path;
  time_t modified_at;
  off_t size;

  struct files_t* next;
} files_t;

files_t* empty_files();
void free_files(files_t* files);

// appends to `files` all the files matched by the `glob_pattern`
// returns a pointer to the tail of the current list
files_t* append_matching_files(logger_t* logger, files_t* files, const char* glob_pattern);

// the caller is responsible for freeing the returned null-terminated string
char* serialize(const files_t* files);
// the caller is responsible for freeing the returned files
files_t* deserialize(const char* string);

#endif /* SBCP_FILES_H */
