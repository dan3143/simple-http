#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include "http/parser.h"
#include "http/util.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_STATUS_TEXT 64

typedef struct {
  int status_code;
  const char *status_text;
  HttpHeaderList header_list;
  HttpBody body;
  bool headers_only;
} HttpResponse;

void make_response(HttpParser *, HttpResponse *);
void serialize_response_metadata(HttpResponse *, char *);
void init_http_response(HttpResponse *, char *);
void free_http_response(HttpResponse *);

#endif