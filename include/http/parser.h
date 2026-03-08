#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http/response.h"
#include "http/util.h"
#include "net/server.h"
#include <stddef.h>

#define MAX_METHOD_LEN 16
#define MAX_PATH_LEN 256
#define MAX_HTTP_VERSION_LEN 8

typedef struct {
  char *method;
  char *path;
  char *http_version;
  HttpHeaderList header_list;
} HttpRequest;

typedef enum {
  PARSING_HEADER_END,
  PARSING_METADATA,
  PARSING_BODY,
  PARSING_COMPLETE,
  PARSING_ERROR,
} ParsingState;

typedef struct {
  ParsingState parsing_state;
  ServerError err;
  HttpRequest req;
  HttpBody body;

  char *buffer;
  size_t buffer_capacity;
  size_t buffer_len;
  size_t offset;
} ParserStatus;

HttpCode parse_request(char *, size_t, HttpRequest *, HttpBody *);
void init_http_request(HttpRequest *);

#endif