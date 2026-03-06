#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_STATUS_TEXT 64

typedef struct {
  int status_code;
  const char *status_text;
  HttpHeaderList header_list;
} HttpResponse;

void send_error_response(int, HttpCode);
void send_file_http(int, char *);

#endif