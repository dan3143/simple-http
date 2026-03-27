#include "http/parser.h"
#include "core/log.h"
#include "misc/util.h"
#include "net/server.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void init_http_request(HttpRequest *req) { req->header_list.header_count = 0; }

void init_parser(HttpParser *parser, char *parsing_buffer) {
  parser->buffer_len = 0;
  parser->offset = 0;
  parser->parsing_state = PARSING_HEADER_END;
  init_http_request(&parser->req);
  init_http_body(&parser->req.body);
  parser->err = SRV_OK;
  parser->buffer = parsing_buffer;
}

void detect_header_end(HttpParser *status) {
  log_debug("Looking for end of headers");
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
      log_debug("Found end of header section. Moving to parse metadata");
      status->parsing_state = PARSING_METADATA;
      status->offset = i + 4;
      status->header_end_pos = status->buffer + status->offset;
      return;
    }
    status->offset++;
  }

  log_debug("Have not found end of header section");
}

void parse_first_line(HttpParser *status) {
  log_debug("Parsing the first line");
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
  status->req.method_name = line_start;
  status->req.method = str_to_http_method(line_start);

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
  status->status_line_end_pos = line_len + 1;
}

void parse_headers(HttpParser *status) {
  char *line_start = status->buffer + status->status_line_end_pos;

  log_debug("Start parsing headers");

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
      log_error("Empty header. Bad request.");
      status->parsing_state = PARSING_ERROR;
      status->err = SRV_ERR_BAD_REQUEST;
      return;
    }
    *line_end = '\0';
    add_header(&status->req.header_list, name, value);
    line_start = line_end + 1;
  }
  status->parsing_state = PARSING_CHECK_BODY;
  log_debug("Finished parsing headers.");
}

void parse_metadata(HttpParser *status) {
  init_http_request(&status->req);
  parse_first_line(status);
  if (status->parsing_state == PARSING_ERROR)
    return;
  parse_headers(status);
}

void check_body(HttpParser *status) {
  log_debug("Checking if there is a body");
  HttpHeader *header = get_header(&status->req.header_list, "Content-Length");
  if (!header) {
    log_debug("No content-length header. Assuming there is no body");
    status->parsing_state = PARSING_COMPLETE;
    return;
  }
  log_debug("Content header found");
  char *endptr;
  status->req.body.type = BODY_BUFFER;
  status->req.body.buffer_data = status->header_end_pos;
  status->req.body.length = strtol(header->value, &endptr, 10);

  if (*endptr != '\0' || errno == ERANGE) {
    log_debug("Bad body length. Bad request.");
    status->parsing_state = PARSING_ERROR;
    status->err = SRV_ERR_BAD_REQUEST;
    return;
  }
  status->parsing_state = PARSING_BODY;
}

void parse_body(HttpParser *status) {
  log_debug("Parsing body");
  size_t metadata_size = status->header_end_pos - status->buffer;
  if (status->buffer_len < metadata_size + status->req.body.length) {
    log_debug("Body still not full. Waiting for more bytes");
    status->offset = status->buffer_len;
    status->err = SRV_AGAIN;
    return;
  }
  status->offset = metadata_size + status->req.body.length;
  status->err = SRV_OK;
  status->parsing_state = PARSING_COMPLETE;
}

void parse_http(HttpParser *status) {
  log_debug("Parsing from %zu to %zu", status->offset, status->buffer_len);
  while (1) {

    switch (status->parsing_state) {
    case PARSING_HEADER_END:
      detect_header_end(status);
      break;
    case PARSING_METADATA:
      parse_metadata(status);
      break;
    case PARSING_CHECK_BODY:
      check_body(status);
      break;
    case PARSING_BODY:
      log_debug("Start parsing body");
      parse_body(status);
      break;
    case PARSING_ERROR:
      log_debug("There was an error while parsing");
    case PARSING_COMPLETE:
      log_debug("Finished parsing");
      return;
    }

    if (status->err == SRV_AGAIN) {
      log_debug("Need more bytes to continue parsing");
      return;
    }
  }
}

bool should_keepalive(HttpRequest *req) {
  HttpHeader *h = get_header(&req->header_list, "Connection");
  if (!h) {
    return strcmp(req->http_version, "HTTP/1.1") == 0;
  }
  return strcasecmp(h->value, "close") != 0;
}