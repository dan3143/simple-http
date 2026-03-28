#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <linux/limits.h>
#include <netinet/in.h>
#include <stdbool.h>

typedef struct {
  char host[INET6_ADDRSTRLEN];
  char port[6];
  int n_threads;
  char doc_root[PATH_MAX];
  char tls_cert[PATH_MAX];
  char tls_key[PATH_MAX];
  char index_file[PATH_MAX];
  int keepalive_timeout;
  int max_connections;
  int log_level;
  bool tls_enabled;
} ServerConfig;

const ServerConfig *get_config();
void init_server_config(int, char **);

#endif