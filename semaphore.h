#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pthread.h>

// Node in the FIFO queue of waiting threads
typedef struct QueueNode {
  pthread_t id;
  struct QueueNode *next;
} QueueNode;

// Simple FIFO queue
typedef struct {
  QueueNode *front;
  QueueNode *rear;
} Queue;

// Monitor-style counting semaphore
typedef struct {
  int count;
  pthread_mutex_t mutex; // protects count + queue
  pthread_cond_t cond;   // condition variable for waiting threads
  Queue waiting_queue;
} Semaphore;

void semaphore_init(Semaphore *sem, int initial_value);
void semaphore_wait(Semaphore *sem);
void semaphore_signal(Semaphore *sem);
void semaphore_destroy(Semaphore *sem);

// Queue helpers
void queue_init(Queue *q);
void queue_enqueue(Queue *q, pthread_t thread_id);
pthread_t queue_dequeue(Queue *q);
int queue_is_empty(Queue *q);
void queue_destroy(Queue *q);

#endif
