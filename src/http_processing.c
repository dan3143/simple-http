#include "http_processing.h"
#include "utils.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
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

char *get_mime_type(char *filename) {
  int n = sizeof(EXT_TO_MIME_TYPE) / sizeof(EXT_TO_MIME_TYPE[0]);
  char *file_ext = get_file_extension(filename);
  char *ext;
  for (int i = 0; i < n; i++) {
    ext = EXT_TO_MIME_TYPE[i][0];
    if (strcmp(ext, file_ext) == 0)
      return EXT_TO_MIME_TYPE[i][1];
  }
  return NULL;
}

bool is_extension_supported(char *extension) {
  int n = sizeof(EXT_TO_MIME_TYPE) / sizeof(EXT_TO_MIME_TYPE[0]);
  for (int i = 0; i < n; i++) {
    if (strcmp(extension, EXT_TO_MIME_TYPE[i][0]) == 0)
      return true;
  }
  return false;
}

void send_http_response_header(int socketfd, int status_code, char *status,
                               char *headers[][2], int n_headers) {
  char http_header[2048] = "";
  snprintf(http_header, sizeof(http_header), "HTTP/1.1 %d %s\r\n", status_code,
           status);
  for (int i = 0; i < n_headers; i++) {
    char complete_header[1024] = "";
    snprintf(complete_header, sizeof(complete_header), "%s: %s\r\n",
             headers[i][0], headers[i][1]);
    strncat(http_header, complete_header, strlen(complete_header));
  }
  int sent_bytes = send(socketfd, http_header, strlen(http_header), 0);
  if (sent_bytes < 0) {
    fprintf(stderr, "Error while sending response\n");
  }
}

void send_http_response_with_body(int socketfd, int status_code, char *status,
                                  char *headers[][2], int n_headers,
                                  char *buf) {
  send_http_response_header(socketfd, status_code, status, headers, n_headers);
  snprintf(buf, strlen(buf) + 2, "%s\r\n", buf);
  int sent_bytes = send(socketfd, buf, strlen(buf), 0);
  if (sent_bytes < 0) {
    fprintf(stderr, "Error while sending response\n");
  }
}

void send_http_file(int socketfd, char *filename) {

  int filefd = open(filename, O_RDONLY);
  struct stat stat_bf;
  off_t offset = 0;

  if (filefd == -1) {
    fprintf(stderr, "Could not open the file: %s\n", filename);
    return;
  }

  if (fstat(filefd, &stat_bf) < 0) {
    perror("Error getting file stats");
    close(filefd);
    return;
  }

  ssize_t bytes_sent = sendfile(socketfd, filefd, &offset, stat_bf.st_size);

  if (bytes_sent <= 0) {
    perror("sendfile error");
  }

  close(filefd);
}

void send_http_response_with_file(int socketfd, int status_code, char *status,
                                  char *headers[][2], int n_headers,
                                  char *filename) {

  size_t max_size = strlen(ROOT_DIR) + 2 + strlen(filename);
  char fullpath[max_size];
  snprintf(fullpath, max_size, "%s/%s", ROOT_DIR, filename);

  if (!file_exists(fullpath)) {
    send_http_response_header(socketfd, 404, "NOT FOUND", headers, 0);
    return;
  }

  send_http_response_header(socketfd, status_code, status, headers, n_headers);
  char *mime_type = get_mime_type(fullpath);
  short mime_type_header_length = 14 + 64 + 1 + 64 + 4;
  char mime_type_header[mime_type_header_length];

  snprintf(mime_type_header, mime_type_header_length,
           "Content-Type: %s\r\n\r\n", mime_type);

  send(socketfd, mime_type_header, strlen(mime_type_header), 0);
  send_http_file(socketfd, fullpath);
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

void process_custom_routes(int socketfd, char *method, char *route) {

  int matching_route = find_matching_route(method, route);
  int status_code = 404;
  char *filename = HTML_404;
  char status[10] = "NOT FOUND";

  if (matching_route != -1) {
    filename = ROUTES[matching_route][FILENAME_IDX];
    status_code = 200;
    strcpy(status, "OK");
  }

  char *headers[][2] = {};

  send_http_response_with_file(socketfd, status_code, status, headers, 0,
                               filename);
}

void handle_http_request(int socketfd, char *buffer, size_t n_bytes) {

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

  char *headers[][2] = {{"Server", "Simple-HTTP"}};
  size_t n_headers = sizeof(headers) / sizeof(headers[0]);
  char *filename = route + 1; // Remove leading slash
  if (is_extension_supported(get_file_extension(filename))) {
    send_http_response_with_file(socketfd, 200, "OK", headers, n_headers,
                                 filename);
  } else {
    process_custom_routes(socketfd, http_method, route);
  }
}
