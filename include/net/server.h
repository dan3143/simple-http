#ifndef SERVER_H
#define SERVER_H

#include "core/worker_pool.h"
typedef enum {
  SRV_OK = 0,
  SRV_AGAIN,
  SRV_ERR_PARSE,
  SRV_ERR_NOT_FOUND,
  SRV_ERR_BAD_REQUEST,
  SRV_ERR_INTERNAL,
  SRV_ERR_IO,
  SRV_ERR_OVERFLOW,
  SRV_ERR_CONN_CLOSED,
} ServerError;

void start_http_server(const char *, const char *);
void handle_incoming_connection(int, WorkerContext *);

#endif