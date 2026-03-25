#ifndef CONNECTION_H
#define CONNECTION_H

#include <openssl/ssl.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int client_sock;
  int server_sock;
  SSL *ssl;
  bool is_tls;
} Connection;

int init_http_connection(Connection *, const char *, const char *);
int init_https_connection(Connection *, char *, char *, char *, char *);
int conn_accept(Connection *);
int conn_read(Connection *, char *, size_t);
int conn_write(Connection *, char *, size_t);
int conn_write_all(Connection *, char *, size_t);
int conn_send_file(Connection *, int, size_t);

#endif