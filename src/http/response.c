#include "http/response.h"
#include "core/log.h"
#include "misc/string_builder.h"
#include "misc/util.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
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

void init_http_response(HttpResponse *res, HttpCode code, const char *status) {
  res->header_list.header_count = 0;
  res->status_code = code;
  res->status_text = status;
}

void send_http_response(int socketfd, HttpResponse res, HttpBody body) {
  StringBuilder sb;
  sb_init(&sb);
  sb_appendf(&sb, "HTTP/1.1 %d %s\r\n", res.status_code, res.status_text);

  for (size_t i = 0; i < res.header_list.header_count; i++) {
    sb_appendf(&sb, "%s: %s\r\n", res.header_list.headers[i].name,
               res.header_list.headers[i].value);
  }

  if (!get_header(&res.header_list, "Content-Length")) {
    sb_appendf(&sb, "Content-Length: %d\r\n", body.length);
  }

  if (!get_header(&res.header_list, "Connection")) {
    sb_append(&sb, "Connection: close\r\n");
  }

  sb_append(&sb, "\r\n");

  size_t sent_len = sb.length;

  send_all(socketfd, sb.data, &sent_len);

  sb_free(&sb);

  if (body.type == BODY_BUFFER) {
    sent_len = body.length;
    send_all(socketfd, body.buffer_data, &sent_len);
  } else if (body.type == BODY_FILE) {
    off_t offset = 0;
    size_t remaining = body.length;
    while (remaining > 0) {
      ssize_t sent = sendfile(socketfd, body.fd, &offset, remaining);
      if (sent <= 0)
        break;
      remaining -= sent;
    }
    close(body.fd);
  }
  log_debug("Successfully sent HTTP response");
}

HttpCode send_file_http(int socketfd, char *path) {
  int filefd = open(path, O_RDONLY);
  struct stat stat_buf;

  if (filefd == -1) {
    log_error("Error opening file %s: %s", path, strerror(errno));
    if (errno == ENOENT) {
      return HTTP_NOT_FOUND;
    }
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  if (fstat(filefd, &stat_buf) < 0) {
    log_error("Error getting file stats for %s: %s", path, strerror(errno));
    close(filefd);
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  HttpBody body;
  HttpResponse res;
  init_http_body(&body);
  init_http_response(&res, HTTP_OK, "OK");
  body.type = BODY_FILE;
  body.fd = filefd;
  body.length = stat_buf.st_size;
  send_http_response(socketfd, res, body);
  return HTTP_OK;
}

void send_error_response(int socketfd, HttpCode code) {

  if (!(is_http_error(code))) {
    log_error("Trying to send an error message with a non-error HTTP code");
    return;
  }

  char body_data[4096];
  HttpResponse res;
  HttpBody body;

  body.type = BODY_BUFFER;
  body.buffer_data = body_data;
  const char *status_text = http_code_to_text(code);
  const char *status_desc = http_code_to_description(code);
  snprintf(body_data, sizeof(body_data), ERROR_PAGE_TEMPLATE, code, status_text,
           code, status_text, status_desc);
  body.buffer_data = body_data;
  body.length = strlen(body_data);

  init_http_response(&res, code, status_text);

  send_http_response(socketfd, res, body);

  return;
}
