#include "logger.h"

int main(int argc, char *argv[])
{
  // FIXME: replace with args from options
  logger_t* logger = init_logger("/tmp/wk.log", INFO);

  return 0;
}
