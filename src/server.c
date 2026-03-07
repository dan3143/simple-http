#include "server.h"
#include "http_processing.h"
#include "log.h"
#include "utils.h"
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
#define BUFFER_SIZE 16384

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

void listen_on_server_sock(int server_sockfd) {

  if (listen(server_sockfd, BACKLOG) == -1) {
    log_fatal("Could not listen on socket: %s", strerror(errno));
    exit(1);
  }

  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int client_sockfd;
  char s[INET6_ADDRSTRLEN];
  int port;

  while (1) {

    sin_size = sizeof client_addr;
    client_sockfd =
        accept(server_sockfd, ((struct sockaddr *)&client_addr), &sin_size);

    if (client_sockfd == -1) {
      log_error("Could not accept incoming connection: %s", strerror(errno));
      continue;
    }

    get_addr_str((struct sockaddr *)&client_addr, s);
    port = get_port((struct sockaddr *)&client_addr);

    log_debug("Connection from %s:%d accepted", s, port);

    char *buffer = malloc(BUFFER_SIZE);

    if (!buffer) {
      log_error("Failed allocating %d bytes to receive data from %s",
                BUFFER_SIZE, s);
      continue;
    }

    int received_bytes;
    log_debug("Receiving data...");
    received_bytes = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0);
    log_debug("Received %d bytes from %s", received_bytes, s);

    if (received_bytes < 1) {
      log_error("Could not receive data from %s", s);
      close(client_sockfd);
      free(buffer);
      continue;
    }

    buffer[received_bytes] = '\0';

    log_debug("Handling data from %s as an HTTP request", s);
    handle_http_request(client_sockfd, buffer, received_bytes, s);

    free(buffer);
    close(client_sockfd);
    log_debug("Connection closed.");
  }
}

void listen_on(char *ipstr, char *port) {
  log_info("Initializing server on %s:%s", ipstr, port);
  int server_sockfd = init_server_sock(ipstr, port);
  listen_on_server_sock(server_sockfd);
}
