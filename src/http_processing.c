#include "http_processing.h"
#include "http_response.h"
#include "utils.h"

void handle_http_request(int socketfd, char *buffer, size_t n_bytes) {
  send_error_response(socketfd, 400);
}
