#include "net/server.h"
#include "core/job_queue.h"
#include "core/log.h"
#include "core/worker_pool.h"
#include "http/parser.h"
#include "http/response.h"
#include "misc/util.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 1024
#define BUFFER_CAPACITY 16384

extern ServerConfig config;

int init_server_sock(char *ipstr, char *port) {
  int server_sockfd, status;
  struct addrinfo *server_info, *p, hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
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

char *allocate_buffer(HttpParser *handler) {
  char *buffer = malloc(BUFFER_CAPACITY);

  if (!buffer) {
    log_error("Failed allocating %d bytes");
    handler->err = SRV_ERR_IO;
    return NULL;
  }

  init_parser(handler, buffer);
  return buffer;
}

int parse_request(int client_sock, HttpParser *parser) {
  log_debug("Parsing request");
  while (1) {
    int received_bytes;
    log_debug("Receiving data...");
    received_bytes = recv(client_sock, parser->buffer + parser->buffer_len,
                          BUFFER_CAPACITY - parser->buffer_len, 0);

    if (received_bytes < 0) {
      log_error("Error while receiving: %s", strerror(errno));
      parser->err = SRV_ERR_INTERNAL;
      return -1;
    }

    if (received_bytes == 0) {
      log_error("Client closed the connection");
      return SRV_ERR_CONN_RST;
    }

    if (received_bytes + parser->buffer_len >= BUFFER_CAPACITY - 1) {
      log_error("Not enough space in buffer");
      parser->err = SRV_ERR_OVERFLOW;
      return -1;
    }

    parser->buffer_len += received_bytes;

    parse_http(parser);

    if (parser->parsing_state == PARSING_ERROR)
      return -1;
    if (parser->parsing_state == PARSING_COMPLETE)
      return 0;
  }
  return -1;
}

void send_response(int client_sock, HttpResponse *res) {
  char *metadata_str = malloc(BUFFER_CAPACITY);
  serialize_response_metadata(res, metadata_str);

  size_t metadata_len = strlen(metadata_str);

  send_all(client_sock, metadata_str, &metadata_len);
  free(metadata_str);

  if (res->body.type == BODY_BUFFER) {
    if (res->headers_only)
      return;

    size_t body_len = res->body.length;
    send_all(client_sock, res->body.buffer_data, &body_len);
    return;
  }

  if (res->body.type == BODY_FILE) {

    off_t offset = 0;
    size_t remaining = res->body.length;

    if (res->headers_only) {
      return;
    }

    while (remaining > 0) {
      ssize_t sent = sendfile(client_sock, res->body.fd, &offset, remaining);
      if (sent <= 0)
        break;
      remaining -= sent;
    }
  }
}

void handle_incoming_connection(int client_sock) {

  struct sockaddr_storage addr;
  char ipstr[INET6_ADDRSTRLEN];
  int port;
  socklen_t len;
  HttpParser handler;

  len = sizeof(addr);
  getpeername(client_sock, (struct sockaddr *)&addr, &len);
  get_addr_str((struct sockaddr *)&addr, ipstr);
  port = get_port((struct sockaddr *)&addr);
  log_debug("Accepted connection from %s:%d", ipstr, port);

  if (!allocate_buffer(&handler)) {
    goto cleanup_socket;
  }

  ServerError err = parse_request(client_sock, &handler);

  if (err == SRV_ERR_CONN_RST) {
    goto cleanup_parser;
  }
  HttpResponse *res = init_http_response();
  make_response(&handler, res);
  send_response(client_sock, res);

  log_info("%s -- \"%s %s %s\" - %d %s", ipstr, handler.req.method_name,
           handler.req.path, handler.req.http_version, res->status_code,
           res->status_text);

  free_http_response(res);
cleanup_parser:
  free(handler.buffer);
cleanup_socket:
  close(client_sock);
}

void listen_on_server_sock(int server_sock) {

  if (listen(server_sock, BACKLOG) == -1) {
    log_fatal("Could not listen on socket: %s", strerror(errno));
    exit(1);
  }

  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int client_sock;

  WorkerPool *pool = init_worker_pool(config.workers);
  log_debug("Created pool of %d workers", config.workers);
  while (1) {

    sin_size = sizeof client_addr;
    client_sock =
        accept(server_sock, ((struct sockaddr *)&client_addr), &sin_size);

    if (client_sock == -1) {
      log_error("Could not accept incoming connection: %s", strerror(errno));
      continue;
    }

    log_debug("Adding to queue...");
    enqueue_job(client_sock, pool->queue);
  }
}

void listen_on(char *ipstr, char *port) {
  log_info("Initializing server on %s:%s", ipstr, port);
  int server_sockfd = init_server_sock(ipstr, port);
  listen_on_server_sock(server_sockfd);
}
