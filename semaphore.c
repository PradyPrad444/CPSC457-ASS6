#include "semaphore.h"
#include <pthread.h>
#include <stdlib.h>

// Queue operations (FIFO)

void queue_init(Queue *q) {
  q->front = NULL;
  q->rear = NULL;
}

void queue_enqueue(Queue *q, pthread_t thread_id) {
  QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
  node->id = thread_id;
  node->next = NULL;

  if (q->rear == NULL) {
    // empty queue
    q->front = q->rear = node;
  } else {
    q->rear->next = node;
    q->rear = node;
  }
}

pthread_t queue_dequeue(Queue *q) {
  if (q->front == NULL) {
    return (pthread_t)0; // empty queue
  }

  QueueNode *temp = q->front;
  pthread_t id = temp->id;

  q->front = q->front->next;
  if (q->front == NULL) {
    q->rear = NULL;
  }

  free(temp);
  return id;
}

int queue_is_empty(Queue *q) { return (q->front == NULL); }

void queue_destroy(Queue *q) {
  while (!queue_is_empty(q)) {
    queue_dequeue(q);
  }
}

// Semaphore operations

void semaphore_init(Semaphore *sem, int initial_value) {
  sem->count = initial_value;
  pthread_mutex_init(&sem->mutex, NULL);
  pthread_cond_init(&sem->cond, NULL);
  queue_init(&sem->waiting_queue);
}

// monitor-style wait with FIFO fairness
void semaphore_wait(Semaphore *sem) {
  pthread_mutex_lock(&sem->mutex);

  // resource available and nobody is queued
  if (sem->count > 0 && queue_is_empty(&sem->waiting_queue)) {
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);
    return;
  }

  // else, join the waiting queue
  pthread_t self = pthread_self();
  queue_enqueue(&sem->waiting_queue, self);

  // Wait until:
  //   there is at least one resource (count > 0), AND
  //   this thread is at the front of the FIFO queue
  while (!(sem->count > 0 && sem->waiting_queue.front != NULL &&
           pthread_equal(sem->waiting_queue.front->id, self))) {
    pthread_cond_wait(&sem->cond, &sem->mutex);
  }

  // We are now the front and a resource is available
  sem->count--;
  queue_dequeue(&sem->waiting_queue);

  pthread_mutex_unlock(&sem->mutex);
}

void semaphore_signal(Semaphore *sem) {
  pthread_mutex_lock(&sem->mutex);

  // One more unit available
  sem->count++;

  // If there are any waiters, wake them up so the front can proceed
  if (!queue_is_empty(&sem->waiting_queue)) {
    // broadcast so the front thread can recheck the condition and proceed
    pthread_cond_broadcast(&sem->cond);
  }

  pthread_mutex_unlock(&sem->mutex);
}

void semaphore_destroy(Semaphore *sem) {
  queue_destroy(&sem->waiting_queue);
  pthread_mutex_destroy(&sem->mutex);
  pthread_cond_destroy(&sem->cond);
}
