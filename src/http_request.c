#include "http_request.h"
#include "http_response.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

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

HttpCode parse_headers(char *buffer, size_t nbytes, HttpRequest *req,
                       char **end) {
  char *line_start = buffer;

  while (!(line_start[0] == '\r' && line_start[1] == '\n')) {
    char *line_end = strchr(line_start, '\n');
    if (!line_end)
      return HTTP_BAD_REQUEST;

    *line_end = '\0';

    if (*(line_end - 1) == '\r')
      *(line_end - 1) = '\0';

    char *colon = strchr(line_start, ':');

    if (colon == NULL) {
      return HTTP_BAD_REQUEST;
    }

    *colon = '\0';
    char *name = line_start;
    char *value = colon + 1;

    while (*value == ' ') {
      value++;
    }
    add_header(&req->header_list, name, value);
    line_start = line_end + 1;
  }

  *(end) = line_start;

  return HTTP_OK;
}

HttpCode parse_request(char *buffer, size_t nbytes, HttpRequest *req) {
  char *headers_start_ptr, *body_start_ptr;

  HttpCode status = HTTP_OK;

  status = parse_first_line(buffer, nbytes, req, &headers_start_ptr);
  headers_start_ptr++;

  if (status != HTTP_OK) {
    return status;
  }

  if (!headers_start_ptr)
    return HTTP_BAD_REQUEST;

  if (strcmp(req->http_version, "HTTP/1.1") != 0) {
    return HTTP_VERSION_NOT_SUPPORTED;
  }

  status =
      parse_headers(headers_start_ptr, (nbytes - (headers_start_ptr - buffer)),
                    req, &body_start_ptr);

  if (status != HTTP_OK)
    return status;

  return status;
}