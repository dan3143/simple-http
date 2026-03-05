
#include "http_response.h"
#include "string_builder.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool add_header(HttpResponse *res, const char *name, const char *value) {
  size_t count = res->header_count;
  if (count >= MAX_HEADERS)
    return false;
  strncpy(res->headers[count].name, name, MAX_HEADER_NAME);
  strncpy(res->headers[count].value, value, MAX_HEADER_VALUE);
  res->header_count++;
  return true;
}

bool has_header(HttpResponse *res, const char *name) {
  for (size_t i = 0; i < res->header_count; i++) {
    if (strcmp(res->headers[i].name, name) == 0)
      return true;
  }
  return false;
}

int body_get_length(HttpBody *body) {
  if (body->type == BODY_BUFFER)
    return body->buffer.length;
  if (body->type == BODY_FILE)
    return body->buffer.length;
  return 0;
}

void send_http_response(int socketfd, HttpResponse *res, HttpBody *body) {
  StringBuilder *sb;
  sb_init(sb);
  sb_appendf(sb, "HTTP/1.1 %d %s\r\n", res->status_code, res->status_text);

  for (size_t i = 0; i < res->header_count; i++) {
    sb_appendf(sb, "%s: %s\r\n", res->headers[i].name, res->headers[i].value);
  }

  if (!has_header(res, "Content-Length")) {
    size_t content_length = body_get_length(body);
    sb_appendf(sb, "Content-Length: %d\r\n", content_length);
  }

  // No keep-alive for now
  if (!has_header(res, "Connection")) {
    sb_append(sb, "Connection: Close");
  }

  sb_append(sb, "\r\n");

  send(socketfd, sb->data, sb->length, 0);

  sb_free(sb);

  if (body->type == BODY_BUFFER) {
    send(socketfd, body->buffer.data, body->buffer.length, 0);
  } else if (body->type == BODY_FILE) {
    off_t offset = 0;
    size_t remaining = body->file.length;
    while (remaining > 0) {
      ssize_t sent = sendfile(socketfd, body->file.fd, &offset, remaining);
      if (sent <= 0)
        break;
      remaining -= sent;
    }
    close(body->file.fd);
  }
}