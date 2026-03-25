#include "net/connection.h"
#include "config/server_config.h"
#include "core/log.h"
#include <errno.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

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
    break;
  }

  freeaddrinfo(server_info);

  if (p == NULL) {
    log_fatal("Could not listen in specified host\n");
    exit(1);
  }
  return server_sockfd;
}

int listen_on_socket(Connection *c) {
  if (listen(c->server_sock, get_config()->max_connections) < 0) {
    return -1;
  }
  return 0;
}

int init_http_connection(Connection *conn, const char *ipstr,
                         const char *port) {
  conn->is_tls = false;
  conn->ssl = NULL;
  int server_sock = get_server_sock(ipstr, port);
  if (server_sock <= 0) {
    return -1;
  }
  conn->server_sock = server_sock;
  int status = listen_on_socket(conn);
  if (status < 0) {
    return -1;
  }
  return 0;
}

int conn_accept(Connection *conn) {
  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int client_sock =
      accept(conn->server_sock, ((struct sockaddr *)&client_addr), &sin_size);

  if (client_sock <= 0) {
    return -1;
  }
  conn->client_sock = client_sock;
  struct timeval tv = {.tv_sec = get_config()->keepalive_timeout, .tv_usec = 0};

  if (setsockopt(conn->client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
      0) {
    log_error("Failed to set timeout for client socket");
  }

  if (conn->is_tls && conn->ssl) {
    SSL_set_fd(conn->ssl, conn->client_sock);
    if (SSL_accept(conn->ssl) == -1) {
      log_error("TLS accept failed");
      SSL_free(conn->ssl);
      close(conn->client_sock);
      return -1;
    }
  }
  return 0;
}

int conn_read(Connection *conn, char *buf, size_t n) {
  size_t received_bytes = 0;

  if (conn->is_tls && conn->ssl) {
    received_bytes = SSL_read(conn->ssl, buf, n);
  } else {
    received_bytes = recv(conn->client_sock, buf, n, 0);
  }

  return received_bytes;
}

int conn_write(Connection *conn, char *buf, size_t n) {
  size_t sent_bytes = 0;
  if (conn->is_tls && conn->ssl) {
    sent_bytes = SSL_write(conn->ssl, buf, n);
  } else {
    sent_bytes = send(conn->client_sock, buf, n, 0);
  }
  return sent_bytes;
}

int conn_write_all(Connection *conn, char *buf, size_t len) {
  size_t total = 0;
  int bytes_left = len;
  int n;
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
  }

  while (remaining > 0) {
    ssize_t sent;
    if (conn->is_tls) { // TODO: support kernel TLS if available
      sent = pread(fd, buf, remaining, offset);
      conn_write_all(conn, buf, remaining);
      offset += sent;
    } else {
      sent = sendfile(conn->client_sock, fd, &offset, remaining);
    }

    if (sent <= 0)
      return sent;
    remaining -= sent;
    total_sent += sent;
  }

  if (buf) {
    free(buf);
  }

  return total_sent;
}