#include "broadcast.h"
#include "logger.h"

void print_usage()
{
  // TODO: a nice helpful message
  exit(1);
}

int main(int argc, char* argv[])
{
  char* action;

  // FIXME: replace with args from options
  logger_t* logger = init_logger("/tmp/wk.log", DEBUG);
  debug(logger, "main: logger initialized");

  // parse arguments
  // the 1st should be an action
  // currently supported:
  //  - server: starts a new server
  //  - ping: discovers running sbcp servers in the local subnet,
  //          and prints their IP addresses and TCP ports
  // TODO print_usage
  if (argc == 1) {
    print_usage();
  }

  action = argv[1];

  if (strcmp(action, "server") == 0) {
    // FIXME: there's more than that to a server
    debug(logger, "main: starting sbcp server");
    broadcast_listen(logger);
  } else if (strcmp(action, "ping") == 0) {
    debug(logger, "main: pinging current subnet");
    // FIXME: needs to be replaced too
    broadcast_emit(logger, 28282);
  } else {
    print_usage();
  }

  return 0;
}
