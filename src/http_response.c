#include "http_response.h"
#include "log.h"
#include "string_builder.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int body_get_length(HttpBody body) {
  if (body.type == BODY_BUFFER)
    return body.buffer.length;
  if (body.type == BODY_FILE)
    return body.buffer.length;
  return 0;
}

void res_init(HttpResponse *res, HttpCode code, const char *status) {
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
    sb_append(&sb, "Connection: Close\r\n");
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

int send_error_response(int socketfd, HttpCode code) {

  if (!(code >= 400 && code < 600))
    return -1;

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
  res_init(&res, code, status_text);

  send_http_response(socketfd, res, body);

  return 0;
}
