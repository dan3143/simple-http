#include "http_processing.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include "utils.h"
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/types.h>

extern ServerConfig config;

void handle_http_request(int socketfd, char *buffer, size_t n_bytes) {
  HttpRequest req;
  HttpBody body;
  char normalized_path[PATH_MAX];

  init_http_request(&req);
  init_http_body(&body);

  HttpCode status = parse_request(buffer, n_bytes, &req, &body);
  if (is_http_error(status)) {
    send_error_response(socketfd, status);
    return;
  }

  // TODO: Handle requests to custom paths

  HttpCode path_status =
      normalize_path(req.path, config.root_dir, normalized_path);

  if (is_http_error(status)) {
    log_error("%s is not a valid path", req.path);
    send_error_response(socketfd, path_status);
    return;
  }

  log_debug("Serving file from %s", normalized_path);
  send_file_http(socketfd, normalized_path);
}
