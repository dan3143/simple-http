#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_response.h"
#include <stddef.h>

#define MAX_METHOD_LEN 16
#define MAX_PATH_LEN 256
#define MAX_HTTP_VERSION_LEN 8

typedef struct {
  char method[MAX_METHOD_LEN];
  char path[MAX_PATH_LEN];
  char http_version[MAX_HTTP_VERSION_LEN];
  HttpHeader headers[MAX_HEADERS];
  size_t header_count;
} HttpRequest;

HttpCode parse_request(char *, size_t, HttpRequest *);

#endif