#ifndef HTTP_PROCESSING_H
#define HTTP_PROCESSING_H

#include <sys/socket.h>
#include <sys/types.h>

void handle_http_request(int, char *, size_t, char *);

#endif
