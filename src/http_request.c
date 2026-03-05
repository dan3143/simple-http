#include "http_request.h"
#include "http_response.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

HttpCode parse_first_line(char *buffer, size_t nbytes, HttpRequest *req,
                          char **line_end) {

  char *first_space = memchr(buffer, ' ', nbytes);
  if (!first_space)
    return HTTP_BAD_REQUEST;

  size_t method_len = first_space - buffer;
  if (method_len - 1 > MAX_METHOD_LEN)
    return HTTP_BAD_REQUEST;

  memcpy(req->method, buffer, method_len);
  req->method[method_len] = '\0';

  char *second_space = memchr(first_space + 1, ' ', nbytes - method_len - 1);
  if (!second_space)
    return HTTP_BAD_REQUEST;

  size_t path_len = second_space - (first_space + 1);
  if (path_len - 1 > MAX_PATH_LEN)
    return HTTP_URI_TOO_LONG;

  memcpy(req->path, first_space + 1, path_len);
  req->path[path_len] = '\0';

  *line_end =
      memchr(second_space + 1, '\n', nbytes - (method_len + 1 + path_len + 1));
  if (!line_end)
    return HTTP_BAD_REQUEST;

  char *actual_line_end = *line_end;

  // Don't count the CR if there is a full CRLF line terminator
  if (*(actual_line_end - 1) == '\r') {
    actual_line_end--;
  }

  size_t version_len = actual_line_end - (second_space + 1);
  if (version_len - 1 > MAX_HTTP_VERSION_LEN)
    return HTTP_BAD_REQUEST;
  memcpy(req->http_version, second_space + 1, version_len);
  req->http_version[version_len] = '\0';
  return HTTP_OK;
}

// TODO
HttpCode parse_headers(char *buffer, size_t nbytes, HttpRequest *req,
                       char **end) {
  printf("%s", buffer);
  printf("Len: %zu\n", nbytes);
  return HTTP_OK;
}

HttpCode parse_request(char *buffer, size_t nbytes, HttpRequest *req) {
  char *first_line_end, *headers_end;
  int first_line_len = 0;
  HttpCode status;

  status = parse_first_line(buffer, nbytes, req, &first_line_end);

  if (status != HTTP_OK) {
    return status;
  }

  if (!first_line_end)
    return HTTP_BAD_REQUEST;

  if (strcmp(req->http_version, "HTTP/1.1") != 0) {
    return HTTP_VERSION_NOT_SUPPORTED;
  }

  status = parse_headers(first_line_end + 1,
                         (nbytes - (first_line_end + 1 - buffer)), req,
                         &headers_end);

  return HTTP_OK;
}