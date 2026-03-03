#include "http_processing.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define HTML_404 "404.html"
#define ROOT_DIR "./static"
#define METHOD_IDX 0
#define ROUTE_IDX 1
#define FILENAME_IDX 2

char *ROUTES[][3] = {
    {"GET", "/", "index.html"},
    {"GET", "/about", "about.html"},
};

char *EXT_TO_MIME_TYPE[][2] = {{"css", "text/css"},
                               {"js", "text/js"},
                               {"gif", "image/gif"},
                               {"html", "text/html"},
                               {"ico", "image/vnd.microsoft.icon"},
                               {"jpg", "image/jpeg"},
                               {"jpeg", "image/jpeg"},
                               {"json", "application/json"},
                               {"md", "text/markdown"},
                               {"png", "image/png"}};

bool is_extension_supported(char *extension) {
  int n = sizeof(EXT_TO_MIME_TYPE) / sizeof(EXT_TO_MIME_TYPE[0]);
  return false;
}

void serveHTML(int socketfd, char *filename) {
  int filefd = open(filename, O_RDONLY);
  struct stat stat_bf;
  off_t offset = 0;

  if (filefd == -1) {
    fprintf(stderr, "Could not open the file %s\n", filename);
    return;
  }

  char *http_header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
  send(socketfd, http_header, strlen(http_header), 0);
  printf("Serving %s\n", filename);

  if (fstat(filefd, &stat_bf) < 0) {
    perror("Error getting file stats");
    close(filefd);
    return;
  }

  ssize_t bytes_sent = sendfile(socketfd, filefd, &offset, stat_bf.st_size);
  if (bytes_sent < 0) {
    perror("sendfile error");
  }

  close(filefd);
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

  size_t max_size = strlen(ROOT_DIR) + 2 + strlen(filename);
  char fullpath[max_size];

  snprintf(fullpath, sizeof(fullpath), "%s/%s", ROOT_DIR, filename);

  serveHTML(socketfd, fullpath);
}

void handle_http_request(int socketfd, char *buffer) {

  char *http_method, *route, *first_line, *http_version;
  first_line = strtok(buffer, "\r\n");

  printf("%s\n", first_line);

  http_method = strtok(first_line, " ");
  route = strtok(NULL, " ");
  http_version = strtok(NULL, " ");

  if (strcmp(http_version, "HTTP/1.1") != 0) {
    fprintf(stderr, "Only HTTP/1.1 is supported\n");
    return;
  }

  process_routes(socketfd, http_method, route);
}
