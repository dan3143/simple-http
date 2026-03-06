#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

HttpCode parse_first_line(char *buffer, size_t nbytes, HttpRequest *req,
                          char **line_end) {
  char *line_start = buffer;
  *line_end = strchr(buffer, '\n');
  **line_end = '\0';
  if (*(*line_end - 1) == '\r') {
    *(*line_end - 1) = '\0';
  }
  char *first_space = strchr(buffer, ' ');
  if (!first_space) {
    log_error("Badly formatted HTTP status line. Bad request");
    return HTTP_BAD_REQUEST;
  }

  *first_space = '\0';
  req->method = line_start;

  char *second_space = strchr(first_space + 1, ' ');
  if (!second_space) {
    log_error("Badly formatted HTTP status line. Bad request");
    return HTTP_BAD_REQUEST;
  }

  *second_space = '\0';
  req->path = first_space + 1;
  req->http_version = second_space + 1;

  return HTTP_OK;
}

HttpCode parse_headers(char *buffer, size_t nbytes, HttpRequest *req,
                       char **end) {
  char *line_start = buffer;

  req->header_list.header_count = 0;

  while (!(line_start[0] == '\r' && line_start[1] == '\n')) {
    char *line_end = strchr(line_start, '\n');
    if (!line_end) {
      log_error("Badly formatted headers. Bad request");
      return HTTP_BAD_REQUEST;
    }

    *line_end = '\0';

    if (*(line_end - 1) == '\r')
      *(line_end - 1) = '\0';

    char *colon = strchr(line_start, ':');

    if (colon == NULL) {
      log_error("Header with no colon. Bad request");
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

HttpCode parse_request(char *buffer, size_t nbytes, HttpRequest *req,
                       HttpBody *body) {
  log_debug("Parsing request");
  char *headers_start_ptr, *body_start_ptr;

  buffer[nbytes] = '\0';

  HttpCode status = HTTP_OK;

  status = parse_first_line(buffer, nbytes, req, &headers_start_ptr);
  headers_start_ptr++;

  if (status != HTTP_OK) {
    return status;
  }

  if (!headers_start_ptr) {
    log_error("Pointer to headers section's start is NULL. Bad request");
    return HTTP_BAD_REQUEST;
  }

  if (strcmp(req->http_version, "HTTP/1.1") != 0) {
    log_error("HTTP Version not supported");
    return HTTP_VERSION_NOT_SUPPORTED;
  }

  status =
      parse_headers(headers_start_ptr, (nbytes - (headers_start_ptr - buffer)),
                    req, &body_start_ptr);

  if (status != HTTP_OK)
    return status;

  if (*body_start_ptr == '\r')
    body_start_ptr++;
  if (*body_start_ptr != '\n') {
    log_error("No LF at the end of headers section. Bad request");
    return HTTP_BAD_REQUEST;
  }

  *body_start_ptr = '\0';
  body_start_ptr++;

  size_t body_len = strlen(body_start_ptr);

  if (body_len == 0) {
    body->type = BODY_NONE;
  } else {
    body->type = BODY_BUFFER;
    body->buffer.data = body_start_ptr;
    body->buffer.length = body_len;
  }

  return status;
}