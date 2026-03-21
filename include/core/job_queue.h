#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include <pthread.h>
#include <stddef.h>

typedef struct job {
  int client_sock;
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
int dequeue_job(JobQueue *);
void enqueue_job(int, JobQueue *);
void free_job_queue(JobQueue *);

#endif
