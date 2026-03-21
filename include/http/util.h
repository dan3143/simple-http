#include "net/server.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef HTTP_H
#define HTTP_H

#define MAX_HEADER_NAME 64
#define MAX_HEADER_VALUE 256
#define MAX_HEADERS 32

typedef struct {
  char *ext;
  char *type;
} MimeEntry;

typedef enum {
  HTTP_CONTINUE = 100,
  HTTP_OK = 200,
  HTTP_MOVED_PERMANENTLY = 301,
  HTTP_FOUND = 302,
  HTTP_BAD_REQUEST = 400,
  HTTP_FORBIDDEN = 403,
  HTTP_NOT_FOUND = 404,
  HTTP_METHOD_NOT_ALLOWED = 405,
  HTTP_CONTENT_TOO_LARGE = 413,
  HTTP_URI_TOO_LONG = 414,
  HTTP_INTERNAL_SERVER_ERROR = 500,
  HTTP_NOT_IMPLEMENTED = 501,
  HTTP_VERSION_NOT_SUPPORTED = 505,
} HttpCode;

typedef enum {
  HTTP_UNKNOWN_METHOD = 0,
  HTTP_GET = 1 << 0,
  HTTP_HEAD = 1 << 1,
  HTTP_POST = 1 << 2,
  HTTP_PUT = 1 << 3,
  HTTP_DELETE = 1 << 4,
  HTTP_CONNECT = 1 << 5,
  HTTP_OPTIONS = 1 << 6,
  HTTP_TRACE = 1 << 7,
  HTTP_PATCH = 1 << 8,
} HttpMethod;

typedef uint32_t HttpMethodSet;

typedef struct {
  HttpCode code;
  const char *message;
} HttpStatus;

typedef struct {
  char name[MAX_HEADER_NAME];
  char value[MAX_HEADER_VALUE];
} HttpHeader;

typedef struct {
  HttpHeader headers[MAX_HEADERS];
  size_t header_count;
} HttpHeaderList;

typedef enum {
  BODY_BUFFER,
  BODY_FILE,
  BODY_NONE,
} HttpBodyType;

typedef struct {
  HttpBodyType type;
  size_t length;
  char *buffer_data;
  int fd;
} HttpBody;

const char *lookup_mime_type(const char *path);
const char *http_code_to_text(HttpCode);
const char *http_code_to_description(HttpCode);
ServerError normalize_path(const char *, const char *, char *);
bool add_header(HttpHeaderList *, const char *, const char *);
HttpHeader *get_header(HttpHeaderList *, const char *);
void init_http_body(HttpBody *);
bool is_http_error(HttpCode);
bool is_method_supported(HttpMethod);
HttpMethod str_to_http_method(char *);

static const char *ERROR_PAGE_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"utf-8\">\n"
    "  <title>%d %s</title>\n"
    "  <style>\n"
    "    body { font-family: sans-serif; background:#f5f5f5; }\n"
    "    .box { max-width:600px; margin:80px auto; padding:30px; "
    "background:white; border-radius:8px; }\n"
    "    h1 { margin-top:0; }\n"
    "  </style>\n"
    "</head>\n"
    "<body>\n"
    "  <div class=\"box\">\n"
    "    <h1>%d %s</h1>\n"
    "    <p>%s</p>\n"
    "    <hr>\n"
    "    <small>SimpleHTTP</small>\n"
    "  </div>\n"
    "</body>\n"
    "</html>\n";

#endif