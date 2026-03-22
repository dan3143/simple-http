#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http/util.h"
#include "net/server.h"
#include <stddef.h>

#define MAX_METHOD_LEN 16
#define MAX_PATH_LEN 256
#define MAX_HTTP_VERSION_LEN 8

typedef struct {
  HttpMethod method;
  char *method_name;
  char *path;
  char *http_version;
  HttpHeaderList header_list;
  HttpBody body;
} HttpRequest;

typedef enum {
  PARSING_HEADER_END,
  PARSING_METADATA,
  PARSING_CHECK_BODY,
  PARSING_BODY,
  PARSING_COMPLETE,
  PARSING_ERROR,
} ParsingState;

typedef struct {
  ParsingState parsing_state;
  ServerError err;
  HttpRequest req;

  char *buffer;
  size_t buffer_len;
  size_t offset;
  size_t status_line_end_pos;
  char *header_end_pos;
} HttpParser;

void init_http_request(HttpRequest *);
void init_parser(HttpParser *, char *);
void parse_http(HttpParser *);
bool should_keepalive(HttpRequest *);

#endif