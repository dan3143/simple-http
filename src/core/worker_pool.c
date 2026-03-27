#include "core/worker_pool.h"
#include "core/job_queue.h"
#include "core/log.h"
#include "misc/util.h"
#include "net/connection.h"
#include "net/server.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

void *worker(void *arg) {
  WorkerArgs *args = (WorkerArgs *)arg;
  JobQueue *queue = args->queue;
  int id = args->thread_id;
  free(args);

  WorkerContext ctx;
  ctx.req_buffer = malloc(REQ_BUF_SIZE);
  ctx.res_body_buffer = malloc(RES_BODY_SIZE);
  ctx.res_header_buffer = malloc(RES_HEADER_SIZE);

  for (;;) {

    pthread_mutex_lock(&queue->mutex);
    while (queue_empty(queue) && !queue->stop) {
      pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    if (queue->stop) {
      pthread_mutex_unlock(&queue->mutex);
      break;
    }

    Connection *c = dequeue_job(queue);
    char ipstr[INET6_ADDRSTRLEN];
    ip_str_from_socket(c->socket, ipstr);
    int port = port_from_socket(c->socket);

    pthread_mutex_unlock(&queue->mutex);
    log_info("Thread %d is managing connection: %s:%d", id, ipstr, port);

    handle_incoming_connection(c, &ctx);
  }

  free(ctx.req_buffer);
  free(ctx.res_body_buffer);
  free(ctx.res_header_buffer);
  return NULL;
}

WorkerPool *init_worker_pool(size_t n_workers) {
  JobQueue *queue = init_job_queue();
  WorkerPool *pool = malloc(sizeof(WorkerPool));
  pool->threads = malloc(n_workers * sizeof(pthread_t));
  pool->n = n_workers;
  pool->queue = queue;
  for (int i = 0; i < n_workers; i++) {
    WorkerArgs *args = malloc(sizeof(WorkerArgs));
    args->queue = pool->queue;
    args->thread_id = i;
    pthread_create(&pool->threads[i], NULL, worker, (void *)args);
    log_info("Created thread %d", i);
  }
  return pool;
}

void free_worker_pool(WorkerPool *pool) {
  stop_job_queue(pool->queue);
  for (int i = 0; i < pool->n; i++) {
    pthread_join(pool->threads[i], NULL);
  }
  free(pool->threads);
  free_job_queue(pool->queue);
  free(pool);
}