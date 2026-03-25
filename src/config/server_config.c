#include "config/server_config.h"
#include "core/log.h"
#include "misc/util.h"
#include <bits/getopt_core.h>
#include <errno.h>
#include <getopt.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ServerConfig g_config;
static char config_path[PATH_MAX] = "./conf.ini";

ServerConfig get_default_config() {
  ServerConfig config = {.tls_cert = "./server.crt",
                         .tls_key = "./server.key",
                         .doc_root = "./",
                         .host = "0.0.0.0",
                         .http_port = "8080",
                         .https_port = "8443",
                         .log_level = LOG_INFO,
                         .n_threads = 8,
                         .keepalive_timeout = 15,
                         .max_connections = 1024,
                         .index_file = "index.html",
                         .tls_enabled = false};
  return config;
}

const ServerConfig *get_config() { return &g_config; }

static struct option long_options[] = {
    {"host", required_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"tls-enabled", no_argument, NULL, 's'},
    {"tls-port", required_argument, NULL, 't'},
    {"tls-key", required_argument, NULL, 'k'},
    {"tls-certificate", required_argument, NULL, 'r'},
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"config", required_argument, NULL, 'c'},
    {NULL, 0, NULL, 0}};

void process_args(int argc, char **argv) {
  int opt;
  int option_idx;
  while ((opt = getopt_long(argc, argv, "h:p:st:k:r:vqc:", long_options,
                            &option_idx)) != -1) {
    switch (opt) {
    case 'h':
      snprintf(g_config.host, sizeof(g_config.host), "%s", optarg);
      break;
    case 'p':
      snprintf(g_config.http_port, sizeof(g_config.http_port), "%s", optarg);
      break;
    case 's':
      g_config.tls_enabled = true;
      break;
    case 't':
      snprintf(g_config.https_port, sizeof(g_config.https_port), "%s", optarg);
      break;
    case 'k':
      snprintf(g_config.tls_key, sizeof(g_config.tls_key), "%s", optarg);
      break;
    case 'r':
      snprintf(g_config.tls_cert, sizeof(g_config.tls_cert), "%s", optarg);
      break;
    case 'v':
      g_config.log_level = LOG_DEBUG;
      break;
    case 'q':
      log_set_quiet(true);
      break;
    case 'c':
      snprintf(config_path, sizeof(config_path), "%s", optarg);
      break;
    }
  }
}

void process_config_line(char *key, char *value, size_t key_len,
                         size_t value_len) {
  if (strncmp(key, "host", key_len) == 0) {
    snprintf(g_config.host, value_len + 1, "%s", value);
  }
  if (strncmp(key, "http_port", key_len) == 0) {
    snprintf(g_config.http_port, value_len + 1, "%s", value);
  }
  if (strncmp(key, "https_port", key_len) == 0) {
    snprintf(g_config.https_port, value_len + 1, "%s", value);
  }
  if (strncmp(key, "doc_root", key_len) == 0) {
    snprintf(g_config.doc_root, value_len + 1, "%s", value);
  }
  if (strncmp(key, "index_file", key_len) == 0) {
    snprintf(g_config.index_file, value_len + 1, "%s", value);
  }
  if (strncmp(key, "keepalive_timeout", key_len) == 0) {
    int int_val;
    if (safe_str_to_int(value, &int_val)) {
      g_config.keepalive_timeout = int_val;
    }
  }
  if (strncmp(key, "log_level", key_len) == 0) {
    if (strncmp(value, "verbose", value_len))
      g_config.log_level = LOG_DEBUG;
    if (strncmp(value, "warn", value_len))
      g_config.log_level = LOG_WARN;
    if (strncmp(value, "informational", value_len))
      g_config.log_level = LOG_INFO;
    if (strncmp(value, "quiet", value_len))
      log_set_quiet(true);
  }
  if (strncmp(key, "max_connections", key_len) == 0) {
    int int_val;
    if (safe_str_to_int(value, &int_val)) {
      g_config.max_connections = int_val;
    }
  }
  if (strncmp(key, "workers_number", key_len) == 0) {
    int int_val;
    if (safe_str_to_int(value, &int_val)) {
      g_config.n_threads = int_val;
    }
  }
  if (strncmp(key, "tls_certificate", key_len) == 0) {
    snprintf(g_config.tls_cert, value_len + 1, "%s", value);
  }
  if (strncmp(key, "tls_key", key_len) == 0) {
    snprintf(g_config.tls_key, value_len + 1, "%s", value);
  }
  if (strncmp(key, "tls_enabled", key_len) == 0) {
    if (strncmp(value, "true", value_len) == 0) {
      g_config.tls_enabled = true;
    } else {
      g_config.tls_enabled = false;
    }
  }
}

void read_config_file(char *config_path) {
  FILE *fp;
  fp = fopen(config_path, "r");
  if (!fp) {
    log_error("Could not open config file at %s: %s", config_path,
              strerror(errno));
    return;
  }

  char *line = NULL;
  ssize_t nread;
  size_t len = 0;
  size_t line_number = 0;

  while ((nread = getline(&line, &len, fp)) != -1) {

    if (line[0] == '#' || line[0] == '\n')
      continue;

    line_number++;
    char *key = line;
    while (*key == ' ' || *key == '\t')
      key++;
    char *eq_sign = strchr(key, '=');
    char *value = eq_sign + 1;
    if (!eq_sign) {
      log_warn("Malformed line in %s:%d", config_path, line_number);
      continue;
    }
    size_t key_len = eq_sign - key;
    while (key_len > 0 &&
           (key[key_len - 1] == ' ' || key[key_len - 1] == '\t')) {
      key_len--;
    }

    while (*value == ' ' || *value == '\t') {
      value++;
    }

    char *value_end = value;
    while (*value_end && *value_end != '#' && *value_end != '\n')
      value_end++;
    while (value_end > value && (value_end[-1] == ' ' || value_end[-1] == '\t'))
      value_end--;

    size_t value_len = value_end - value;

    process_config_line(key, value, key_len, value_len);
  }

  free(line);
  fclose(fp);
}

void init_server_config(int argc, char **argv) {
  g_config = get_default_config();
  process_args(argc, argv);
  read_config_file(config_path);
}