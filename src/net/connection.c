#include "net/connection.h"
#include "config/server_config.h"
#include "core/log.h"
#include <errno.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

static SSL_CTX *g_ssl_ctx = NULL;

int get_server_sock(const char *ipstr, const char *port) {
  int server_sockfd, status;
  struct addrinfo *server_info, *p, hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int yes = 1;

  if ((status = getaddrinfo(ipstr, port, &hints, &server_info)) != 0) {
    log_error("getaddrinfo: %s", gai_strerror(status));
    exit(1);
  }

  for (p = server_info; p != NULL; p = p->ai_next) {
    if ((server_sockfd =
             socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      log_error("Error while initializing socket: %s", strerror(errno));
      continue;
    }
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) {
      log_fatal("Error when setting socket options: %s", strerror(errno));
      exit(1);
    }
    if (bind(server_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_sockfd);
      log_error("Error during bind: %s", strerror(errno));
      continue;
    }
    if (listen(server_sockfd, get_config()->max_connections) < 0) {
      close(server_sockfd);
      log_error("Error when listening: %s", strerror(errno));
      continue;
    }
    break;
  }

  freeaddrinfo(server_info);

  if (p == NULL) {
    log_fatal("Could not listen in specified host\n");
    exit(1);
  }
  return server_sockfd;
}

Connection *get_new_connection(bool is_tls) {
  Connection *conn = malloc(sizeof(Connection));
  conn->socket = -1;
  conn->is_tls = is_tls;
  conn->ssl = NULL;
  return conn;
}

void free_connection(Connection *conn) {
  if (conn->socket > 0) {
    close(conn->socket);
  }

  if (conn->ssl) {
    SSL_free(conn->ssl);
  }

  free(conn);
}

int load_certificates(SSL_CTX *ctx, const char *cert_path,
                      const char *key_path) {
  if (SSL_CTX_load_verify_locations(ctx, cert_path, key_path) != 1) {
    log_error("Error verifying certificate and key files");
    return -1;
  }
  if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
    log_error("Error verifying certificate and key files");
    return -1;
  }

  if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
    log_error("Error setting %s as a certificate: %s", cert_path,
              strerror(errno));
    return -1;
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
    log_error("Error setting %s as the key: %s", key_path, strerror(errno));
    return -1;
  }

  return 0;
}

int init_ssl_context(const char *cert_path, const char *key_path) {
  const SSL_METHOD *method;
  SSL_CTX *ctx;

  OpenSSL_add_ssl_algorithms();
  SSL_load_error_strings();

  method = TLS_server_method();
  ctx = SSL_CTX_new(method);
  if (ctx == NULL) {
    log_error("Error creating TLS context");
    return -1;
  }

  SSL_CTX_set_cipher_list(ctx, "ALL:eNULL");
  load_certificates(ctx, cert_path, key_path);

  g_ssl_ctx = ctx;

  return 0;
}

SSL_CTX *get_ssl_context() { return g_ssl_ctx; }

Connection *conn_accept(int fd, bool is_tls) {

  struct sockaddr_storage client_addr;
  socklen_t sin_size = sizeof(client_addr);
  int client_sock = accept(fd, ((struct sockaddr *)&client_addr), &sin_size);

  if (client_sock <= 0) {
    return NULL;
  }

  Connection *conn = get_new_connection(is_tls);

  conn->socket = client_sock;
  struct timeval tv = {.tv_sec = get_config()->keepalive_timeout, .tv_usec = 0};

  if (setsockopt(conn->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    log_error("Failed to set timeout for client socket");
  }

  if (conn->is_tls) {
    conn->ssl = SSL_new(get_ssl_context());
    SSL_set_fd(conn->ssl, conn->socket);
    if (SSL_accept(conn->ssl) <= -1) {
      free_connection(conn);
      return NULL;
    }
  }
  return conn;
}

int conn_read(Connection *conn, char *buf, size_t n) {
  ssize_t received_bytes = 0;

  if (conn->is_tls && conn->ssl) {
    received_bytes = SSL_read(conn->ssl, buf, n);
  } else {
    received_bytes = recv(conn->socket, buf, n, 0);
  }

  return received_bytes;
}

int conn_write(Connection *conn, char *buf, size_t n) {
  ssize_t sent_bytes = 0;
  if (conn->is_tls && conn->ssl) {
    sent_bytes = SSL_write(conn->ssl, buf, n);
  } else {
    sent_bytes = send(conn->socket, buf, n, 0);
  }
  return sent_bytes;
}

int conn_write_all(Connection *conn, char *buf, size_t len) {
  size_t total = 0;
  int bytes_left = len;
  ssize_t n;
  while (total < len) {
    n = conn_write(conn, buf + total, bytes_left);
    if (n <= 0) {
      break;
    }
    total += n;
    bytes_left -= n;
  }
  return total;
}

int conn_send_file(Connection *conn, int fd, size_t len) {
  off_t offset = 0;
  size_t remaining = len;
  size_t total_sent = 0;
  char *buf = NULL;

  if (conn->is_tls) {
    buf = malloc(len);
    if (!buf)
      return -1;
  }

  while (remaining > 0) {
    ssize_t sent;
    if (conn->is_tls) {
      sent = pread(fd, buf, remaining, offset);
      if (sent <= 0)
        break;
      conn_write_all(conn, buf, sent);
      offset += sent;
    } else {
      sent = sendfile(conn->socket, fd, &offset, remaining);
      if (sent <= 0) {
        break;
      }
    }

    remaining -= sent;
    total_sent += sent;
  }
  free(buf);
  return total_sent;
}