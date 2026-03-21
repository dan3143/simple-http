#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "core/job_queue.h"
#include <pthread.h>
#include <stddef.h>

typedef struct {
  pthread_t *threads;
  int n;
  JobQueue *queue;
} WorkerPool;

typedef struct {
  JobQueue *queue;
  int thread_id;
} WorkerArgs;

WorkerPool *init_worker_pool(size_t);

#endif