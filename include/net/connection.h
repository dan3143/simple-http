#ifndef CONNECTION_H
#define CONNECTION_H

#include <openssl/ssl.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int socket;
  SSL *ssl;
  bool is_tls;
} Connection;

int get_server_sock(const char *ipstr, const char *port);
Connection *conn_accept(int fd, bool);
int conn_read(Connection *, char *, size_t);
int conn_write(Connection *, char *, size_t);
int conn_write_all(Connection *, char *, size_t);
int conn_send_file(Connection *, int, size_t);
int init_ssl_context(const char *cert_path, const char *key_path);
void free_ssl_context(void);
void free_connection(Connection *);

#endif