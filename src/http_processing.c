#include "http_processing.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include "utils.h"
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/types.h>

#define TRY_HTTP(x)                                                            \
  do {                                                                         \
    status = (x);                                                              \
    if (is_http_error(status))                                                 \
      goto end;                                                                \
  } while (0)

extern ServerConfig config;

void handle_http_request(int socketfd, char *buffer, size_t n_bytes,
                         char *ipstr) {
  HttpRequest req;
  HttpBody body;
  char normalized_path[PATH_MAX];

  init_http_request(&req);
  init_http_body(&body);

  HttpCode status = HTTP_OK;

  TRY_HTTP(parse_request(buffer, n_bytes, &req, &body));
  TRY_HTTP(normalize_path(req.path, config.root_dir, normalized_path));
  TRY_HTTP(send_file_http(socketfd, normalized_path));

end:
  if (is_http_error(status)) {
    send_error_response(socketfd, status);
  }
  log_info("%s -- \"%s %s %s\" %d", ipstr, req.method, req.path,
           req.http_version, status);
}
