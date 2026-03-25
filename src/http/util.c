#include "http/util.h"
#include "config/server_config.h"
#include "core/log.h"
#include "misc/util.h"
#include "net/server.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const MimeEntry mime_table[] = {{"html", "text/html; charset=utf-8"},
                                {"htm", "text/html; charset=utf-8"},
                                {"css", "text/css; charset=utf-8"},
                                {"js", "application/javascript; charset=utf-8"},
                                {"json", "application/json; charset=utf-8"},
                                {"png", "image/png"},
                                {"jpg", "image/jpeg"},
                                {"jpeg", "image/jpeg"},
                                {"gif", "image/gif"},
                                {"ico", "image/vnd.microsoft.icon"},
                                {"svg", "image/svg+xml"},
                                {"txt", "text/plain; charset=utf-8"}};

const HttpMethodSet SUPPORTED_METHODS = HTTP_GET | HTTP_HEAD;

void init_http_body(HttpBody *body) {
  body->type = BODY_NONE;
  body->length = 0;
}

const char *lookup_mime_type(const char *path) {
  const char *ext = get_file_extension(path);
  if (!ext)
    return "application/octet-stream";

  size_t count = sizeof(mime_table) / sizeof(mime_table[0]);

  for (int i = 0; i < count; i++) {
    if (strcmp(ext, mime_table[i].ext) == 0)
      return mime_table[i].type;
  }

  return "application/octet-stream";
}

ServerError normalize_path(const char *path, const char *root_path,
                           char *output) {
  char canonical_root[PATH_MAX];
  char temp[PATH_MAX];
  char actual_path[PATH_MAX];

  if (*path == '/') // Remove leading slash
    path++;

  size_t path_len = strlen(path);

  strncpy(actual_path, path, strlen(path));
  actual_path[path_len] = '\0';

  if (!realpath(root_path, canonical_root)) {
    log_fatal("Issue with root dir: %s", strerror(errno));
    exit(1);
  }

  if (strlen(path) == 0) {
    const char *default_index = get_config()->index_file;
    strncpy(actual_path, default_index, strlen(default_index));
    actual_path[10] = '\0';
  }

  snprintf(temp, sizeof(temp), "%s/%s", canonical_root, actual_path);

  if (!realpath(temp, output)) {
    if (errno == ENOENT) {
      log_debug("File does not exist: %s", temp);
      return SRV_ERR_NOT_FOUND;
    }
    log_debug("Could not obtain realpath: %s", strerror(errno));
    return SRV_ERR_BAD_REQUEST;
  }

  if (strncmp(output, canonical_root, strlen(canonical_root)) == 0)
    return SRV_OK;
  return SRV_ERR_BAD_REQUEST;
}

bool add_header(HttpHeaderList *list, const char *name, const char *value) {
  size_t count = list->header_count;
  if (count >= MAX_HEADERS)
    return false;
  strncpy(list->headers[count].name, name, MAX_HEADER_NAME);
  strncpy(list->headers[count].value, value, MAX_HEADER_VALUE);
  list->header_count++;
  return true;
}

HttpHeader *get_header(HttpHeaderList *list, const char *name) {
  for (size_t i = 0; i < list->header_count; i++) {
    if (strcasecmp(list->headers[i].name, name) == 0)
      return &list->headers[i];
  }
  return NULL;
}

bool is_http_error(HttpCode code) { return code >= 400 && code < 600; }

bool is_method_supported(HttpMethod method) {
  return method & SUPPORTED_METHODS;
}

HttpMethod str_to_http_method(char *str) {
  switch (str[0]) {
  case 'G':
    return !strcmp(str, "GET") ? HTTP_GET : HTTP_UNKNOWN_METHOD;
  case 'H':
    return !strcmp(str, "HEAD") ? HTTP_HEAD : HTTP_UNKNOWN_METHOD;
  case 'P':
    switch (str[1]) {
    case 'O':
      return !strcmp(str, "POST") ? HTTP_POST : HTTP_UNKNOWN_METHOD;
    case 'U':
      return !strcmp(str, "PUT") ? HTTP_PUT : HTTP_UNKNOWN_METHOD;
    case 'A':
      return !strcmp(str, "PATCH") ? HTTP_PATCH : HTTP_UNKNOWN_METHOD;
    }
    return HTTP_UNKNOWN_METHOD;
  case 'D':
    return !strcmp(str, "DELETE") ? HTTP_DELETE : HTTP_UNKNOWN_METHOD;
  case 'C':
    return !strcmp(str, "CONNECT") ? HTTP_CONNECT : HTTP_UNKNOWN_METHOD;
  case 'O':
    return !strcmp(str, "OPTIONS") ? HTTP_OPTIONS : HTTP_UNKNOWN_METHOD;
  case 'T':
    return !strcmp(str, "TRACE") ? HTTP_TRACE : HTTP_UNKNOWN_METHOD;
  }
  return HTTP_UNKNOWN_METHOD;
}