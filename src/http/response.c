#include "http/response.h"
#include "core/log.h"
#include "core/strings.h"
#include "http/parser.h"
#include "misc/string_builder.h"
#include "misc/util.h"
#include "net/server.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern ServerConfig config;

void init_http_response(HttpResponse *res, HttpCode code, const char *status) {
  res->header_list.header_count = 0;
  res->status_code = code;
  res->status_text = status;
}

void serialize_response_metadata(HttpResponse *res, char *out) {

  StringBuilder sb;
  sb_init(&sb);
  sb_appendf(&sb, "HTTP/1.1 %d %s\r\n", res->status_code, res->status_text);

  for (size_t i = 0; i < res->header_list.header_count; i++) {
    sb_appendf(&sb, "%s: %s\r\n", res->header_list.headers[i].name,
               res->header_list.headers[i].value);
  }

  if (!get_header(&res->header_list, "Content-Length")) {
    sb_appendf(&sb, "Content-Length: %d\r\n", res->body.length);
  }

  if (!get_header(&res->header_list, "Connection")) {
    sb_append(&sb, "Connection: close\r\n");
  }

  sb_append(&sb, "\r\n");
  strncpy(out, sb.data, sb.length);
  out[sb.length] = '\0';
  sb_free(&sb);
}

void make_error_response(HttpCode code, HttpResponse *res) {

  char body_data[4096];
  HttpBody body;

  const char *status_text = http_code_to_text(code);
  const char *status_desc = http_code_to_description(code);

  snprintf(body_data, sizeof(body_data), ERROR_PAGE_TEMPLATE, code, status_text,
           code, status_text, status_desc);

  body.type = BODY_BUFFER;
  body.buffer_data = body_data;
  body.length = strlen(body.buffer_data);
  init_http_response(res, code, status_text);
  res->body = body;
}

void make_file_response(char *path, HttpResponse *out_res) {
  int filefd = open(path, O_RDONLY);
  struct stat stat_buf;

  if (filefd == -1) {
    log_error("Error opening file %s: %s", path, strerror(errno));
    if (errno == ENOENT) {
      make_error_response(HTTP_NOT_FOUND, out_res);
      return;
    }
    make_error_response(HTTP_INTERNAL_SERVER_ERROR, out_res);
    return;
  }

  if (fstat(filefd, &stat_buf) < 0) {
    log_error("Error getting file stats for %s: %s", path, strerror(errno));
    close(filefd);
    make_error_response(HTTP_INTERNAL_SERVER_ERROR, out_res);
    return;
  }

  HttpBody body;
  init_http_body(&body);
  init_http_response(out_res, HTTP_OK, "OK");

  char content_length_str[MAX_HEADER_VALUE];
  snprintf(content_length_str, MAX_HEADER_VALUE, "%zu", stat_buf.st_size);
  const char *mime_type = lookup_mime_type(path);

  add_header(&out_res->header_list, "Content-Type", mime_type);
  add_header(&out_res->header_list, "Content-Length", content_length_str);

  body.type = BODY_FILE;
  body.fd = filefd;
  body.length = stat_buf.st_size;
  out_res->body = body;
}

void make_response(HttpHandler *handler, HttpResponse *out_res) {
  HttpRequest req = handler->req;

  if (handler->err != SRV_OK) {
    make_error_response(srv_err_to_http_err(handler->err), out_res);
    return;
  }

  if (!is_method_supported(req.method)) {
    make_error_response(HTTP_NOT_IMPLEMENTED, out_res);
    return;
  }

  char normalized_path[PATH_MAX];
  ServerError err = normalize_path(req.path, config.root_dir, normalized_path);
  if (err != SRV_OK) {
    make_error_response(srv_err_to_http_err(err), out_res);
    return;
  }

  make_file_response(normalized_path, out_res);
}