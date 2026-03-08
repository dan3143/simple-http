#include "http/parser.h"
#include "core/log.h"
#include "http/response.h"
#include "net/server.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

void init_http_request(HttpRequest *req) { req->header_list.header_count = 0; }

void detect_header_end(ParserStatus *status) {

  char *start = status->buffer + status->offset;

  if (status->buffer_len + status->offset < 4) {
    status->parsing_state = PARSING_HEADER_END;
    return;
  }

  for (size_t i = status->offset; i < status->buffer_len; i++) {
    if (start[i] == '\r' && start[i + 1] == '\n' && start[i + 2] == '\r' &&
        start[i + 3]) {
      status->parsing_state = PARSING_METADATA;
      status->offset += i + 4;
      return;
    }
    status->offset++;
  }
}

void parse_first_line(ParserStatus *status) {
  char *line_start = status->buffer;
  char *line_end = strchr(status->buffer, '\n');
  size_t line_len = line_end - line_start;
  *line_end = '\0';

  if (*(line_end - 1) == '\r') {
    *(line_end - 1) = '\0';
  }
  char *first_space = strchr(status->buffer, ' ');
  if (!first_space) {
    log_error("Badly formatted HTTP status line. Bad request");
    status->parsing_state = PARSING_ERROR;
    status->err = SRV_ERR_BAD_REQUEST;
    return;
  }

  *first_space = '\0';
  status->req.method = line_start;

  char *second_space = strchr(first_space + 1, ' ');
  if (!second_space) {
    log_error("Badly formatted HTTP status line. Bad request");
    status->parsing_state = PARSING_ERROR;
    status->err = SRV_ERR_BAD_REQUEST;
    return;
  }

  *second_space = '\0';
  status->req.path = first_space + 1;
  status->req.http_version = second_space + 1;
  status->offset += line_len;
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

void parse_metadata(ParserStatus *status) {
  parse_first_line(status);
  if (status->parsing_state == PARSING_ERROR)
    return;
  // parse_headers(status);
}

void parse_body(ParserStatus *status) {}

void make_response(ParserStatus *status) {}

void parse(ParserStatus *status) {

  bool finished = false;

  while (!finished) {

    if (status->offset >= status->buffer_len) {
      return; // We need more bytes
    }

    switch (status->parsing_state) {
    case PARSING_HEADER_END:
      detect_header_end(status);
      break;
    case PARSING_METADATA: // We know that we have received all the metadata
      parse_metadata(status);
      break;
    case PARSING_BODY: // We should know Content-Length
      parse_body(status);
      break;
    case PARSING_ERROR:
    case PARSING_COMPLETE:
      return;
    }
  }
}