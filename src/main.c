#include "config/server_config.h"
#include "core/log.h"
#include "misc/util.h"
#include "net/server.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <signal.h>

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  init_server_config(argc, argv);
  log_set_level(get_config()->log_level);
  start_http_server();
  return 0;
}
