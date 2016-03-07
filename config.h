#ifndef SBCP_CONFIG_H
#define SBCP_CONFIG_H

#include "files.h"

// SBCP server config
typedef struct config_t {
  char* config_path;

  char** shared_paths;
  int shared_paths_len;
  int recursive;

  char* aes_key;
} config_t;

config_t* new_config(int argc, char* argv[]);
void free_config(config_t* config);

// for a file-based config, re-parses the config file
void update_config(config_t* config);

void print_usage();

// enumerates all the files in the shared paths
files_t* shared_files(config_t* config);

#endif /* SBCP_CONFIG_H */
