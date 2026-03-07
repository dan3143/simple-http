#include "http_processing.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include "utils.h"
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/types.h>

extern ServerConfig config;

void handle_http_request(int socketfd, char *buffer, size_t n_bytes,
                         char *ipstr) {
  HttpRequest req;
  HttpBody body;
  char normalized_path[PATH_MAX];

  init_http_request(&req);
  init_http_body(&body);

  HttpCode status = parse_request(buffer, n_bytes, &req, &body);

  if (is_http_error(status)) {
    goto end;
  }

  status = normalize_path(req.path, config.root_dir, normalized_path);

  if (is_http_error(status)) {
    log_debug("%s is not a valid path", req.path);
    goto end;
  }

  status = send_file_http(socketfd, normalized_path);
  if (is_http_error(status)) {
    goto end;
  }

end:
  if (is_http_error(status)) {
    send_error_response(socketfd, status);
  }
  log_info("%s -- \"%s %s %s\" %d", ipstr, req.method, req.path,
           req.http_version, status);
}
