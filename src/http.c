#include "http.h"
#include "log.h"
#include "utils.h"
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

void init_http_body(HttpBody *body) { body->type = BODY_NONE; }

const char *lookup_mime_type(const char *path) {
  const char *ext = get_file_extension(path);
  if (!ext)
    return "application/octect-stream";

  size_t count = sizeof(mime_table) / sizeof(mime_table[0]);

  for (int i = 0; i < count; i++) {
    if (strcmp(ext, mime_table[i].ext) == 0)
      return mime_table[i].type;
  }

  return "application/octect-stream";
}

HttpCode normalize_path(const char *path, const char *root_path, char *output) {
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
    strncpy(actual_path, "index.html", 10);
    actual_path[10] = '\0';
  }

  snprintf(temp, sizeof(temp), "%s/%s", canonical_root, actual_path);

  if (!realpath(temp, output)) {
    if (errno == ENOENT) {
      log_debug("File does not exist: %s", temp);
      return HTTP_NOT_FOUND;
    }
    log_debug("Could not obtain realpath: %s", strerror(errno));
    return HTTP_BAD_REQUEST;
  }

  if (strncmp(output, temp, strlen(canonical_root)) == 0)
    return HTTP_OK;
  return HTTP_BAD_REQUEST;
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

const char *http_code_to_text(HttpCode code) {
  switch (code) {
  case HTTP_OK:
    return "OK";
  case HTTP_BAD_REQUEST:
    return "Bad Request";
  case HTTP_VERSION_NOT_SUPPORTED:
    return "HTTP Version Not Supported";
  case HTTP_CONTENT_TOO_LARGE:
    return "Content Too Large";
  case HTTP_FORBIDDEN:
    return "Forbidden";
  case HTTP_FOUND:
    return "Found";
  case HTTP_INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case HTTP_NOT_FOUND:
    return "Not Found";
  case HTTP_MOVED_PERMANENTLY:
    return "Moved Permanently";
  default:
    return "";
  }
}

const char *http_code_to_description(HttpCode code) {
  switch (code) {
  case HTTP_OK:
    return "Request completed successfully";
  case HTTP_BAD_REQUEST:
    return "Malformed syntax, invalid framing, or deceptive request routing.";
  case HTTP_VERSION_NOT_SUPPORTED:
    return "Server does not support the HTTP version used in the request.";
  case HTTP_CONTENT_TOO_LARGE:
    return "Request body exceeds server or endpoint limits.";
  case HTTP_FORBIDDEN:
    return "Server understood the request but refuses to authorize it.";
  case HTTP_FOUND:
    return "Temporary redirect.";
  case HTTP_INTERNAL_SERVER_ERROR:
    return "Generic catch-all for unhandled server-side exceptions.";
  case HTTP_NOT_FOUND:
    return "Resource not found at this URI.";
  case HTTP_MOVED_PERMANENTLY:
    return "Resource permanently relocated. Clients should update stored URLs.";
  default:
    return "";
  }
}