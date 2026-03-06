#include "http_processing.h"
#include "http_response.h"
#include "utils.h"

#define HTML_404 "404.html"
#define ROOT_DIR "./static"
#define METHOD_IDX 0
#define ROUTE_IDX 1
#define FILENAME_IDX 2

void handle_http_request(int socketfd, char *buffer, size_t n_bytes) {
  send_error_response(socketfd, 400);
}
