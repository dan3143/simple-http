#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_utils.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_STATUS_TEXT 64

typedef struct {
  int status_code;
  char status_text[MAX_STATUS_TEXT];
  HttpHeaderList header_list;
} HttpResponse;

typedef enum {
  BODY_BUFFER,
  BODY_FILE,
  BODY_NONE,
} HttpBodyType;

typedef struct {
  HttpBodyType type;

  union {
    struct {
      char *data;
      size_t length;
    } buffer;

    struct {
      int fd;
      size_t length;
    } file;
  };

} HttpBody;

#endif