#include "http_response.h"
#include "log.h"
#include "string_builder.h"
#include "utils.h"
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

int body_get_length(HttpBody body) {
  if (body.type == BODY_BUFFER)
    return body.buffer.length;
  if (body.type == BODY_FILE)
    return body.buffer.length;
  return 0;
}

void init_http_response(HttpResponse *res, HttpCode code, const char *status) {
  res->header_list.header_count = 0;
  res->status_code = code;
  res->status_text = status;
}

void send_http_response(int socketfd, HttpResponse res, HttpBody body) {
  log_info("Response: %d - %s", res.status_code, res.status_text);

  StringBuilder sb;
  sb_init(&sb);
  sb_appendf(&sb, "HTTP/1.1 %d %s\r\n", res.status_code, res.status_text);

  for (size_t i = 0; i < res.header_list.header_count; i++) {
    sb_appendf(&sb, "%s: %s\r\n", res.header_list.headers[i].name,
               res.header_list.headers[i].value);
  }

  if (!get_header(&res.header_list, "Content-Length")) {
    size_t content_length = body_get_length(body);
    sb_appendf(&sb, "Content-Length: %d\r\n", content_length);
  }

  if (!get_header(&res.header_list, "Connection")) {
    sb_append(&sb, "Connection: close\r\n");
  }

  sb_append(&sb, "\r\n");

  log_debug("Sending HTTP metadata (status line and headers)");
  send(socketfd, sb.data, sb.length, 0);

  sb_free(&sb);

  if (body.type == BODY_BUFFER) {
    log_debug("Sending HTTP body");
    send(socketfd, body.buffer.data, body.buffer.length, 0);
  } else if (body.type == BODY_FILE) {
    log_debug("Sending file as HTTP body");
    off_t offset = 0;
    size_t remaining = body.file.length;
    while (remaining > 0) {
      ssize_t sent = sendfile(socketfd, body.file.fd, &offset, remaining);
      if (sent <= 0)
        break;
      remaining -= sent;
    }
    close(body.file.fd);
  }
  log_debug("Successfully sent HTTP response");
}

void send_file_http(int socketfd, char *path) {
  int filefd = open(path, O_RDONLY);
  struct stat stat_buf;

  if (filefd == -1) {
    log_error("Error opening file %s: %s", path, strerror(errno));
    if (errno == ENOENT) {
      send_error_response(socketfd, HTTP_NOT_FOUND);
      return;
    }
    send_error_response(socketfd, HTTP_INTERNAL_SERVER_ERROR);
    return;
  }

  if (fstat(filefd, &stat_buf) < 0) {
    log_error("Error getting file stats for %s: %s", path, strerror(errno));
    send_error_response(socketfd, HTTP_INTERNAL_SERVER_ERROR);
    close(filefd);
    return;
  }

  HttpBody body;
  HttpResponse res;
  init_http_body(&body);
  init_http_response(&res, HTTP_OK, "OK");
  body.type = BODY_FILE;
  body.file.fd = filefd;
  body.file.length = stat_buf.st_size;
  send_http_response(socketfd, res, body);
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
  body.buffer.data = body_data;
  const char *status_text = http_code_to_text(code);
  const char *status_desc = http_code_to_description(code);
  snprintf(body_data, sizeof(body_data), ERROR_PAGE_TEMPLATE, code, status_text,
           code, status_text, status_desc);
  body.buffer.data = body_data;
  body.buffer.length = strlen(body_data);

  init_http_response(&res, code, status_text);

  send_http_response(socketfd, res, body);

  return;
}
