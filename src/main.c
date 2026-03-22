#include "core/log.h"
#include "misc/util.h"
#include "net/server.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

ServerConfig config = {.host = "0.0.0.0",
                       .port = "8080",
                       .root_dir = "./",
                       .log_level = LOG_INFO,
                       .workers = 8};

void processArgs(int argc, char **argv) {
  char c;
  while ((c = getopt(argc, argv, "h:p:d:vw:q")) != -1) {
    switch (c) {
    case 'h':
      strncpy(config.host, optarg, INET6_ADDRSTRLEN);
      break;
    case 'p':
      snprintf(config.port, sizeof(config.port), "%s", optarg);
      break;
    case 'd':
      strncpy(config.root_dir, optarg, PATH_MAX);
      break;
    case 'v':
      config.log_level = LOG_DEBUG;
      break;
    case 'w':
      config.workers = atoi(optarg);
      break;
    case 'q':
      log_set_quiet(true);
      break;
    }
  }
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  processArgs(argc, argv);
  log_set_level(config.log_level);
  listen_on(config.host, config.port);
  return 0;
}
