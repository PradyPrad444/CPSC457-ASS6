#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pthread.h>

// FIFO Queue Node
typedef struct QueueNode {
  pthread_t id;
  struct QueueNode *next;
} QueueNode;

// FIFO Queue
typedef struct {
  QueueNode *front;
  QueueNode *rear;
} Queue;

// Monitor-Style Semaphore
typedef struct {
  int count;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  Queue waiting_queue;
} Semaphore;

// Queue operations
void queue_init(Queue *q);
void queue_enqueue(Queue *q, pthread_t id);
pthread_t queue_dequeue(Queue *q);
pthread_t queue_front(Queue *q);
int queue_is_empty(Queue *q);
void queue_destroy(Queue *q);

// Semaphore operations
void semaphore_init(Semaphore *sem, int value);
void semaphore_wait(Semaphore *sem);
void semaphore_signal(Semaphore *sem);
void semaphore_destroy(Semaphore *sem);

#endif
