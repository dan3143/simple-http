#include "log.h"
#include "server.h"
#include "utils.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <string.h>

ServerConfig config = {
    .host = "0.0.0.0", .port = "8080", .root_dir = "./", .log_level = LOG_INFO};

void processArgs(int argc, char **argv) {
  char c;
  while ((c = getopt(argc, argv, "h:p:d:v")) != -1) {
    switch (c) {
    case 'h':
      strncpy(config.host, optarg, INET6_ADDRSTRLEN);
      break;
    case 'p':
      strncpy(config.port, optarg, 5);
      break;
    case 'd':
      strncpy(config.root_dir, optarg, PATH_MAX);
      break;
    case 'v':
      config.log_level = LOG_DEBUG;
      break;
    }
  }
}

int main(int argc, char **argv) {
  processArgs(argc, argv);
  log_set_level(config.log_level);
  listen_on(config.host, config.port);
  return 0;
}
