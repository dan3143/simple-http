#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "net/connection.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct job {
  Connection *conn;
  struct job *next;
} Job;

typedef struct {
  Job *head;
  Job *tail;

  pthread_mutex_t mutex;
  pthread_cond_t not_empty;

  size_t count;
  int stop;

} JobQueue;

JobQueue *init_job_queue();
Connection *dequeue_job(JobQueue *);
void enqueue_job(Connection *, JobQueue *);
void free_job_queue(JobQueue *);
bool queue_empty(JobQueue *);

#endif
