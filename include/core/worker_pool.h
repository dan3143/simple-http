#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "core/job_queue.h"
#include <pthread.h>
#include <stddef.h>

#define REQ_BUF_SIZE 8192
#define RES_BODY_SIZE 8192
#define RES_HEADER_SIZE 2048

typedef struct {
  pthread_t *threads;
  int n;
  JobQueue *queue;
} WorkerPool;

typedef struct {
  JobQueue *queue;
  int thread_id;
} WorkerArgs;

typedef struct {
  char *req_buffer;
  char *res_body_buffer;
  char *res_header_buffer;
} WorkerContext;

WorkerPool *init_worker_pool(size_t);
void free_worker_pool(WorkerPool *);

#endif