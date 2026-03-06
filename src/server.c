#include "server.h"
#include "http_processing.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 10
#define BUFFER_SIZE 16384

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int init_server_sock(char *ipstr, char *port) {
  int server_sockfd, status;
  struct addrinfo *server_info, *p, hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int yes = 1;

  if ((status = getaddrinfo(ipstr, port, &hints, &server_info)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(1);
  }

  for (p = server_info; p != NULL; p = p->ai_next) {
    if ((server_sockfd =
             socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }
    if (bind(server_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }

  freeaddrinfo(server_info);

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  printf("Initialized server on %s:%s\n", ipstr, port);

  return server_sockfd;
}

void listen_on_server_sock(int server_sockfd) {

  if (listen(server_sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  printf("Listening...\n");

  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  int client_sockfd;
  char s[INET6_ADDRSTRLEN];

  while (1) {
    sin_size = sizeof client_addr;
    client_sockfd =
        accept(server_sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (client_sockfd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
    printf("\nServer: got connection from %s\n", s);

    char *buffer = malloc(BUFFER_SIZE);
    int received_bytes;
    received_bytes = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0);

    if (received_bytes < 1) {
      printf("error while reading from client");
      close(client_sockfd);
      return;
    }

    buffer[received_bytes] = '\0';

    handle_http_request(client_sockfd, buffer, received_bytes);

    free(buffer);
    close(client_sockfd);
  }
}

void listen_on(char *ipstr, char *port) {
  int server_sockfd = init_server_sock(ipstr, port);
  listen_on_server_sock(server_sockfd);
}
