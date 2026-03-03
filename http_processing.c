#include "http_processing.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define HTML_404 "404.html"
#define STATIC_FOLDER "./static"
#define BUFFER_SIZE 1024
#define METHOD_IDX 0
#define ROUTE_IDX 1
#define FILENAME_IDX 2

char *ROUTES[][3] = {
    {"GET", "/", "index.html"},
    {"GET", "/about", "about.html"},
};

bool file_exists(char *filename) {
  FILE *fp = fopen(filename, "r");
  bool exists = true;
  if (fp == NULL) {
    exists = false;
  }
  fclose(fp);
  return exists;
}

void serveHTML(int socketfd, char *filename) {
  FILE *fp = fopen(filename, "r");

  if (fp == NULL) {
    fprintf(stderr, "Could not open the file %s\n", filename);
    return;
  }

  char buffer[BUFFER_SIZE] = {0};

  char *http_header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
  send(socketfd, http_header, strlen(http_header), 0);

  size_t read = 0;
  while ((read = fread(buffer, sizeof(buffer[0]), BUFFER_SIZE, fp)) > 0) {
    send(socketfd, buffer, read, 0);
  }

  fclose(fp);
}

int find_matching_route(char *method, char *route) {
  size_t routes_size = sizeof(ROUTES) / sizeof(ROUTES[0]);
  for (int i = 0; i < routes_size; i++) {
    if (strcmp(method, ROUTES[i][0]) == 0 && strcmp(route, ROUTES[i][1]) == 0) {
      return i;
    }
  }
  return -1;
}

void process_routes(int socketfd, char *method, char *route) {

  int matching_route = find_matching_route(method, route);
  char *filename = HTML_404;

  if (matching_route != -1) {
    filename = ROUTES[matching_route][FILENAME_IDX];
  }

  size_t max_size = strlen(STATIC_FOLDER) + 2 + strlen(filename);
  char fullpath[max_size];

  snprintf(fullpath, sizeof(fullpath), "%s/%s", STATIC_FOLDER, filename);

  serveHTML(socketfd, fullpath);
}

void process_http(int socketfd, char *buffer) {
  char *http_method, *route, *first_line;

  first_line = strtok(buffer, "\n");
  http_method = strtok(first_line, " ");
  route = strtok(NULL, " ");
  // http_version = strtok(NULL, " ");
  process_routes(socketfd, http_method, route);
}

void handle_http_request(int socketfd) {
  char buffer[BUFFER_SIZE];
  int received_bytes;
  received_bytes = recv(socketfd, buffer, BUFFER_SIZE - 1, 0);

  if (received_bytes < 1) {
    printf("error while reading from client");
    close(socketfd);
    return;
  }
  buffer[received_bytes] = '\0';
  process_http(socketfd, buffer);
  close(socketfd);
}
