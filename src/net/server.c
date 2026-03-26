#include "net/server.h"
#include "config/server_config.h"
#include "core/job_queue.h"
#include "core/log.h"
#include "core/worker_pool.h"
#include "http/parser.h"
#include "http/response.h"
#include "misc/util.h"
#include "net/connection.h"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int receive_request(Connection *c, HttpParser *parser) {
  while (1) {
    int received_bytes;
    log_debug("Receiving data...");

    received_bytes = conn_read(c, parser->buffer + parser->buffer_len,
                               REQ_BUF_SIZE - parser->buffer_len);

    if (received_bytes <= 0) {
      log_debug("Could not send. Closing connection");
      return SRV_ERR_CONN_CLOSED;
    }

    if (received_bytes + parser->buffer_len >= REQ_BUF_SIZE - 1) {
      log_error("Not enough space in buffer");
      parser->err = SRV_ERR_OVERFLOW;
      return -1;
    }

    parser->buffer_len += received_bytes;

    parse_http(parser);

    if (parser->parsing_state == PARSING_ERROR)
      return -1;
    if (parser->parsing_state == PARSING_COMPLETE)
      return 0;
  }
  return -1;
}

ServerError send_response(Connection *c, HttpResponse *res,
                          WorkerContext *ctx) {
  serialize_response_metadata(res, ctx->res_header_buffer);

  size_t metadata_len = strlen(ctx->res_header_buffer);

  conn_write_all(c, ctx->res_header_buffer, metadata_len);

  if (res->headers_only)
    return SRV_OK;

  if (res->body.type == BODY_BUFFER) {
    if (conn_write_all(c, res->body.buffer_data, res->body.length) < 0) {
      return SRV_ERR_CONN_CLOSED;
    }
  }
  if (res->body.type == BODY_FILE) {
    if (conn_send_file(c, res->body.fd, res->body.length) < 0) {
      return SRV_ERR_CONN_CLOSED;
    }
  }
  return SRV_OK;
}

void handle_incoming_connection(Connection *c, WorkerContext *ctx) {
  struct sockaddr_storage addr;
  char ipstr[INET6_ADDRSTRLEN];
  int port;
  socklen_t len;

  len = sizeof(addr);
  getpeername(c->socket, (struct sockaddr *)&addr, &len);
  get_addr_str((struct sockaddr *)&addr, ipstr);
  port = get_port((struct sockaddr *)&addr);

  log_debug("Handling connection from %s:%d", ipstr, port);

  HttpParser parser;
  HttpResponse res;

  init_parser(&parser, ctx->req_buffer);

  for (;;) {

    if (c->socket < 0) {
      return;
    }

    ServerError err;

    size_t consumed = parser.offset;
    size_t leftover = parser.buffer_len - parser.offset;
    if (leftover > 0) {
      memmove(ctx->req_buffer, ctx->req_buffer + consumed, leftover);
    }

    init_parser(&parser, ctx->req_buffer);
    parser.buffer_len = leftover;

    err = receive_request(c, &parser);
    if (err == SRV_ERR_CONN_CLOSED) {
      break;
    }

    init_http_response(&res, ctx->res_body_buffer);
    make_response(&parser, &res);

    err = send_response(c, &res, ctx);
    if (err == SRV_ERR_CONN_CLOSED) {
      break;
    }

    log_info("%s -- \"%s %s %s\" - %d %s", ipstr, parser.req.method_name,
             parser.req.path, parser.req.http_version, res.status_code,
             res.status_text);

    if (res.should_close)
      break;

    cleanup_response(&res);
  }

  free_connection(c);
}

void start_http_server() {
  int server_sock;

  if (get_config()->tls_enabled) {
    server_sock = get_server_sock(get_config()->host, get_config()->https_port);
    int status =
        init_ssl_context(get_config()->tls_cert, get_config()->tls_key);
    if (status == -1) {
      log_error("Failed creating SSL context");
      return;
    }
  } else {
    server_sock = get_server_sock(get_config()->host, get_config()->http_port);
  }

  WorkerPool *pool = init_worker_pool(get_config()->n_threads);
  log_debug("Created pool of %d workers", get_config()->n_threads);

  while (1) {
    Connection *conn = conn_accept(server_sock, get_config()->tls_enabled);
    if (!conn) {
      log_error("Could not accept incoming connection: %s", strerror(errno));
      continue;
    }
    enqueue_job(conn, pool->queue);
  }
}
