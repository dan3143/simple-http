#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_response.h"
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

HttpCode parse_request(char *, size_t, HttpRequest *);

#endif