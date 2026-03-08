#include "http/parser.h"
#include "core/log.h"
#include "http/response.h"
#include "misc/utils.h"
#include "net/server.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void init_http_request(HttpRequest *req) { req->header_list.header_count = 0; }

void init_parser(HttpParser *status, char *buffer, size_t buffer_capacity) {
  status->buffer_capacity = buffer_capacity;
  status->buffer = buffer;
  status->buffer_len = 0;
  status->offset = 0;
  status->parsing_state = PARSING_HEADER_END;
  init_http_request(&status->req);
  init_http_body(&status->body);
  status->err = SRV_OK;
}

void detect_header_end(HttpParser *status) {
  char *start = status->buffer;

  if (status->offset + 3 >= status->buffer_len) {
    log_debug("Need more than 4 bytes. Current length: %zu",
              status->buffer_len);
    status->parsing_state = PARSING_HEADER_END;
    status->err = SRV_AGAIN;
    return;
  } else {
    status->err = SRV_OK;
  }

  while (status->offset + 3 < status->buffer_len) {
    size_t i = status->offset;
    if (start[i] == '\r' && start[i + 1] == '\n' && start[i + 2] == '\r' &&
        start[i + 3] == '\n') {
      log_debug(
          "Parser: found end of header section. Moving to parse metadata");
      status->parsing_state = PARSING_METADATA;
      status->offset = 0;
      return;
    }
    status->offset++;
  }

  log_debug("Parser: have not found end of header section");
}

void parse_first_line(HttpParser *status) {
  log_debug("Parser: parsing the first line");
  char *line_start = status->buffer;
  char *line_end = strchr(status->buffer, '\n');
  size_t line_len = line_end - line_start;
  *line_end = '\0';

  if (*(line_end - 1) == '\r') {
    *(line_end - 1) = '\0';
  }
  char *first_space = strchr(status->buffer, ' ');
  if (!first_space) {
    log_error("Parser: badly formatted HTTP status line. Bad request");
    status->parsing_state = PARSING_ERROR;
    status->err = SRV_ERR_BAD_REQUEST;
    return;
  }

  *first_space = '\0';
  status->req.method = line_start;

  char *second_space = strchr(first_space + 1, ' ');
  if (!second_space) {
    log_error("Parser: badly formatted HTTP status line. Bad request");
    status->parsing_state = PARSING_ERROR;
    status->err = SRV_ERR_BAD_REQUEST;
    return;
  }

  *second_space = '\0';
  status->req.path = first_space + 1;
  status->req.http_version = second_space + 1;
  status->offset += line_len + 1; // Skip LF

  // TODO: Check if method is allowed
  // TODO: Check if HTTP version is supported
}

void parse_headers(HttpParser *status) {
  char *line_start = status->buffer + status->offset;

  log_debug("Parser: start parsing headers");

  while (!(line_start[0] == '\r' && line_start[1] == '\n')) {
    char *line_end = strchr(line_start, '\n');
    if (!line_end) {
      log_error("Badly formatted headers. Bad request");
      status->parsing_state = PARSING_ERROR;
      status->err = SRV_ERR_BAD_REQUEST;
      return;
    }

    *line_end = '\0';

    if (*(line_end - 1) == '\r')
      *(line_end - 1) = '\0';

    char *colon = strchr(line_start, ':');

    if (colon == NULL) {
      log_error("Header with no colon. Bad request");
      status->parsing_state = PARSING_ERROR;
      status->err = SRV_ERR_BAD_REQUEST;
      return;
    }

    *colon = '\0';
    char *name = line_start;
    char *value = colon + 1;

    while (*value == ' ') {
      value++;
    }
    if (*value == '\0') {
      log_error("Parser: empty header. Bad request.");
      status->parsing_state = PARSING_ERROR;
      status->err = SRV_ERR_BAD_REQUEST;
      return;
    }
    add_header(&status->req.header_list, name, value);
    line_start = line_end + 1;
  }
  status->parsing_state = PARSING_BODY;
  log_debug("parser: finished parsing headers. Going to parse body");
}

void parse_metadata(HttpParser *status) {
  init_http_request(&status->req);
  parse_first_line(status);
  if (status->parsing_state == PARSING_ERROR)
    return;
  parse_headers(status);
}

void parse_body(HttpParser *status) {
  status->parsing_state = PARSING_COMPLETE;
}

void make_response(HttpParser *status) {}

void parse_http(HttpParser *status) {
  log_debug("Parser: parsing from %zu to %zu", status->offset,
            status->buffer_len);
  while (1) {

    if (status->offset >= status->buffer_len) {
      log_debug("Parser: consumed all bytes, waiting for more...");
      return;
    }

    switch (status->parsing_state) {
    case PARSING_HEADER_END:
      log_debug("Parser: looking for end of headers section");
      detect_header_end(status);
      break;
    case PARSING_METADATA: // We know that we have received all the metadata
      log_debug("Parser: parsing metadata");
      parse_metadata(status);
      break;
    case PARSING_BODY: // We should know Content-Length
      log_debug("Parser: parsing body");
      parse_body(status);
      break;
    case PARSING_ERROR:
      log_debug("Parser: there was an error");
    case PARSING_COMPLETE:
      log_debug("Parser: finished parsing");
      return;
    }

    if (status->err == SRV_AGAIN) {
      log_debug("Parser: need more bytes to continue parsing");
      return;
    }
  }
}