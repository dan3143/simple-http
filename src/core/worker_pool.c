#include "core/worker_pool.h"
#include "core/job_queue.h"
#include "core/log.h"
#include "net/connection.h"
#include "net/server.h"
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

void *worker(void *arg) {
  WorkerArgs *args = (WorkerArgs *)arg;
  JobQueue *queue = args->queue;
  free(args);

  WorkerContext ctx;
  ctx.req_buffer = malloc(REQ_BUF_SIZE);
  ctx.res_body_buffer = malloc(RES_BODY_SIZE);
  ctx.res_header_buffer = malloc(RES_HEADER_SIZE);

  for (;;) {

    pthread_mutex_lock(&queue->mutex);
    while (queue_empty(queue)) {
      pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    Connection *c = dequeue_job(queue);
    pthread_mutex_unlock(&queue->mutex);

    handle_incoming_connection(c, &ctx);
  }
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