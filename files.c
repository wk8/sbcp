#include "files.h"

files_t* empty_files()
{
  files_t* files = (files_t*) calloc(1, sizeof(files_t));
  files->next = NULL;
  files->path = NULL;
  return files;
}

files_t* from_path(logger_t* logger, const char* path, char* absolute_path_buffer)
{
  struct stat stats;
  files_t* files = NULL;

  if (stat(path, &stats) == 0) {
    if (access(path, R_OK) == 0) {
      if (S_ISREG(stats.st_mode)) {
        if (realpath(path, absolute_path_buffer) != NULL) {
          files = empty_files();
          files->path = (char*) malloc((strlen(absolute_path_buffer) + 1) * sizeof(char));
          memcpy(files->path, absolute_path_buffer, strlen(absolute_path_buffer) + 1);
          files->modified_at = stats.st_mtime;
          files->size = stats.st_size;
        } else {
          warning(logger, "files: could not get the real path for %s: %s", path, strerror(errno));
        }
      }
    } else {
      warning(logger, "files: access denied to %s: %s", path, strerror(errno));
    }
  } else {
    warning(logger, "files: could not stat %s: %s", path, strerror(errno));
  }
  
  return files;
}

files_t* find_tail(files_t* files)
{
  if (files->next == NULL) {
    return files;
  }
  return find_tail(files->next);
}

files_t* append_matching_files(logger_t* logger, files_t* files, const char* glob_pattern)
{
  glob_t glob_results;
  files_t *current, *tail = find_tail(files);
  char absolute_path_buffer[PATH_MAX + 1];
  int i;

  if (glob(glob_pattern, GLOB_MARK | GLOB_TILDE, NULL, &glob_results) == 0) {
    for (i = 0; i < glob_results.gl_pathc; i++) {
      current = from_path(logger, glob_results.gl_pathv[i], absolute_path_buffer);
      if (current != NULL) {
        tail->next = current;
        tail = current;
      }
    }
  }

  // cleanup
  globfree(&glob_results);

  return tail;
}

long nb_digits(const size_t n) {
    if (n < 10) { return 1; }
    return 1 + nb_digits(n / 10);
}

#define SBCP_FILES_COUNT_SEPARATOR "#"

char* serialize(const files_t* files)
{
  int current_path_length, num_digits;
  long serialized_length = 0, nb_files = 0, current_offset;
  // a files_t struct's root is always empty
  files_t* current = files->next;
  char* data;
  char str_len_buffer[PATH_MAX + 1];

  while (current != NULL) {
    current_path_length = strlen(current->path);
    serialized_length += nb_digits(current_path_length) + current_path_length + sizeof(time_t) + sizeof(off_t);
    nb_files++;

    current = current->next;
  }

  num_digits = nb_digits(nb_files);
  // +2 for the count separator and the null byte
  serialized_length += num_digits + 2;
  data = (char*) malloc(serialized_length * sizeof(char));
  snprintf(str_len_buffer, PATH_MAX, "%ld", nb_files);
  memcpy(data, str_len_buffer, num_digits);
  memcpy(data + num_digits, SBCP_FILES_COUNT_SEPARATOR, sizeof(char));
  current_offset = num_digits + 1;

  current = files->next;
  while (current != NULL) {
    current_path_length = strlen(current->path);
    snprintf(str_len_buffer, PATH_MAX, "%d", current_path_length);

    num_digits = nb_digits(current_path_length);
    memcpy(data + current_offset, str_len_buffer, num_digits);
    current_offset += num_digits;
    memcpy(data + current_offset, current->path, current_path_length);
    current_offset += current_path_length;
    memcpy(data + current_offset, &current->modified_at, sizeof(time_t));
    current_offset += sizeof(time_t);
    memcpy(data + current_offset, &current->size, sizeof(off_t));
    current_offset += sizeof(off_t);

    current = current->next;
  }

  data[current_offset] = NULL;

  return data;
}

files_t* deserialize_one_file(char** data)
{
  files_t *files;
  long path_len;

  files = empty_files();

  path_len = strtol(*data, data, 10);
  files->path = (char*) malloc((path_len + 1) * sizeof(char));
  memcpy(files->path, *data, path_len);
  files->path[path_len] = NULL;

  *data = *data + path_len;
  memcpy(&files->modified_at, *data, sizeof(time_t));
  *data = *data + sizeof(time_t);
  memcpy(&files->size, *data, sizeof(off_t));
  *data = *data + sizeof(off_t);

  return files;
}

files_t* deserialize(const char* string)
{
  files_t *files, *tail;
  long nb_files, i;
  char* data;

  // a files_t struct's root is always empty
  files = empty_files();
  tail = files;

  nb_files = strtol(string, &data, 10);

  for (i = 0; i < nb_files; i++) {
    tail->next = deserialize_one_file(&data);
  }

  return files;
}

void free_files(files_t* files)
{
  free_files(files->next);
  free(files->path);
  free(files);
}
