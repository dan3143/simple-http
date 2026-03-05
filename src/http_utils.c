#include "http_utils.h"
#include "utils.h"
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

bool normalize_path(const char *path, const char *root_path, char *output) {
  char canonical_root[PATH_MAX];
  char temp[PATH_MAX];

  if (!realpath(root_path, canonical_root)) {
    perror("realpath root");
    exit(1);
  }

  if (strlen(path) == 0) {
    strcpy(canonical_root, "index.html");
  }

  snprintf(temp, sizeof(temp), "%s/%s", canonical_root, path);

  if (!realpath(temp, output))
    return false;

  return strncmp(output, temp, strlen(canonical_root)) != 0;
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

bool has_header(HttpHeaderList list, const char *name) {
  for (size_t i = 0; i < list.header_count; i++) {
    if (strcasecmp(list.headers[i].name, name) == 0)
      return true;
  }
  return false;
}
