#include "core/job_queue.h"
#include "net/connection.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

JobQueue *init_job_queue() {
  JobQueue *queue = malloc(sizeof(JobQueue));
  queue->head = NULL;
  queue->tail = NULL;
  queue->count = 0;
  queue->stop = 0;
  pthread_cond_init(&queue->not_empty, NULL);
  pthread_mutex_init(&queue->mutex, NULL);
  return queue;
}

void free_job_queue(JobQueue *queue) {
  Job *current = queue->head;
  Job *next_node;

  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->not_empty);

  while (current != NULL) {
    next_node = current->next;
    free(current);
    current = next_node;
  }

  free(queue);
}

void enqueue_job(Connection *c, JobQueue *queue) {
  Job *job = malloc(sizeof(Job));
  job->conn = c;
  job->next = NULL;

  pthread_mutex_lock(&queue->mutex);

  if (queue->head == NULL) {
    queue->head = job;
  } else {
    queue->tail->next = job;
  }
  queue->tail = job;
  queue->count++;

  pthread_cond_signal(&queue->not_empty);
  pthread_mutex_unlock(&queue->mutex);
}

Connection *dequeue_job(JobQueue *queue) {
  Job *job = queue->head;
  if (job == NULL)
    return 0;
  Connection *sockfd = job->conn;
  queue->head = job->next;
  if (queue->head == NULL) {
    queue->tail = NULL;
  }
  job->next = NULL;
  free(job);
  queue->count--;
  return sockfd;
}

bool queue_empty(JobQueue *queue) { return queue->count == 0; }