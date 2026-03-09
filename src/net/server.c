#include "net/server.h"
#include "core/log.h"
#include "http/parser.h"
#include "misc/util.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 10
#define BUFFER_CAPACITY 16384

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

void handle_incoming_connection(int client_sock) {

  struct sockaddr_storage addr;
  char ipstr[INET6_ADDRSTRLEN];
  int port;
  socklen_t len;

  len = sizeof(addr);

  char *buffer = malloc(BUFFER_CAPACITY);
  getpeername(client_sock, (struct sockaddr *)&addr, &len);

  get_addr_str((struct sockaddr *)&addr, ipstr);
  port = get_port((struct sockaddr *)&addr);

  log_debug("Accepted connection from %s:%d", ipstr, port);

  HttpHandler handler;
  init_parser(&handler, buffer);

  if (!buffer) {
    log_error("Failed allocating %d bytes to receive data from %s",
              BUFFER_CAPACITY, ipstr);
    handler.err = SRV_ERR_IO;
    goto socket_cleanup;
  }

  while (1) {
    int received_bytes;
    log_debug("Receiving data...");
    received_bytes = recv(client_sock, handler.buffer + handler.buffer_len,
                          BUFFER_CAPACITY - handler.buffer_len, 0);
    log_debug("Received %d bytes from %s", received_bytes, ipstr);

    if (received_bytes < 0) {
      log_error("Could not receive data from %s", ipstr);
      goto finish;
    }

    if (received_bytes == 0) {
      if (handler.parsing_state != PARSING_COMPLETE) {
        log_error("Server: client closed the connection prematurely");
        break;
      }
    }

    if (received_bytes + BUFFER_CAPACITY) {
      log_error("Not enough space in buffer");
      handler.err = SRV_ERR_OVERFLOW;
      goto finish;
    }

    handler.buffer_len += received_bytes;
    handler.buffer[handler.buffer_len] = '\0';

    parse_http(&handler);

    if (handler.parsing_state == PARSING_ERROR ||
        handler.parsing_state == PARSING_COMPLETE) {
      break;
    }
  }

  log_debug("Parsing complete");

finish:
  free(buffer);

socket_cleanup:
  close(client_sock);
  log_debug("Connection closed.");
}

void listen_on_server_sock(int server_sock) {

  if (listen(server_sock, BACKLOG) == -1) {
    log_fatal("Could not listen on socket: %s", strerror(errno));
    exit(1);
  }

  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int client_sock;

  while (1) {

    sin_size = sizeof client_addr;
    client_sock =
        accept(server_sock, ((struct sockaddr *)&client_addr), &sin_size);

    if (client_sock == -1) {
      log_error("Could not accept incoming connection: %s", strerror(errno));
      continue;
    }

    handle_incoming_connection(client_sock);
  }
}

void listen_on(char *ipstr, char *port) {
  log_info("Initializing server on %s:%s", ipstr, port);
  int server_sockfd = init_server_sock(ipstr, port);
  listen_on_server_sock(server_sockfd);
}
