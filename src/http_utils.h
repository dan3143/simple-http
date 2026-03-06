#include <stdbool.h>
#include <stddef.h>

#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#define MAX_HEADER_NAME 64
#define MAX_HEADER_VALUE 256
#define MAX_HEADERS 32

typedef struct {
  char *ext;
  char *type;
} MimeEntry;

typedef enum {
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

const char *lookup_mime_type(const char *path);
const char *http_code_to_text(HttpCode);
const char *http_code_to_description(HttpCode);
bool normalize_path(const char *, const char *, char *);
bool add_header(HttpHeaderList *, const char *, const char *);
HttpHeader *get_header(HttpHeaderList *, const char *);

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