#include "net/server.h"
#include "core/log.h"
#include "http/parser.h"
#include "misc/utils.h"
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

void listen_on_server_sock(int server_sock) {

  if (listen(server_sock, BACKLOG) == -1) {
    log_fatal("Could not listen on socket: %s", strerror(errno));
    exit(1);
  }

  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int client_sock;
  char s[INET6_ADDRSTRLEN];
  int port;

  while (1) {

    sin_size = sizeof client_addr;
    client_sock =
        accept(server_sock, ((struct sockaddr *)&client_addr), &sin_size);

    if (client_sock == -1) {
      log_error("Could not accept incoming connection: %s", strerror(errno));
      continue;
    }

    get_addr_str((struct sockaddr *)&client_addr, s);
    port = get_port((struct sockaddr *)&client_addr);

    log_debug("Connection from %s:%d accepted", s, port);

    char *buffer = malloc(BUFFER_CAPACITY);

    HttpParser parser;
    init_parser(&parser, buffer, BUFFER_CAPACITY);

    if (!buffer) {
      log_error("Failed allocating %d bytes to receive data from %s",
                BUFFER_CAPACITY, s);
      close(client_sock); // TODO: send 500 error instead
      continue;
    }

    while (1) {
      int received_bytes;
      log_debug("Receiving data...");
      received_bytes = recv(client_sock, parser.buffer + parser.buffer_len,
                            BUFFER_CAPACITY - parser.buffer_len, 0);
      log_debug("Received %d bytes from %s", received_bytes, s);

      // TODO: handle this
      if (received_bytes < 0) {
        log_error("Could not receive data from %s", s);
        break;
      }

      if (received_bytes == 0) {
        if (parser.parsing_state != PARSING_COMPLETE) {
          log_error("Server: client closed the connection prematurely");
          break;
        }
      }

      // TODO: check if received bytes go out of bounds

      parser.buffer_len += received_bytes;
      parser.buffer[parser.buffer_len] = '\0';

      parse_http(&parser);

      if (parser.parsing_state == PARSING_ERROR ||
          parser.parsing_state == PARSING_COMPLETE) {
        break;
      }
    }

    log_debug("Parsing complete");

    close(client_sock);
    free(buffer);
    log_debug("Connection closed.");
  }
}

void listen_on(char *ipstr, char *port) {
  log_info("Initializing server on %s:%s", ipstr, port);
  int server_sockfd = init_server_sock(ipstr, port);
  listen_on_server_sock(server_sockfd);
}
