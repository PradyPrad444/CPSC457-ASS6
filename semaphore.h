#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pthread.h>

// queue node struction used for tracking who's waiting in line
typedef struct qnode {
  pthread_t tid;         // thread's ID 
  struct qnode *next;    // pointer to the next waiting in line
} qnode_t;

// the actual semaphore structure
typedef struct {
  int count;             // how many resources are available
  pthread_mutex_t mutex; // lock to make sure only one thread messes with the semaphore at a time
  pthread_cond_t cond;   // for putting threads to sleep and waking them up
  qnode_t *head;         // front of the waiting line
  qnode_t *tail;         // back of line
} semaphore_t;

// semaphore with some starting value
void semaphore_init(semaphore_t *sem, int value);

// wait operation to try to grab a resource but block if none available
void semaphore_wait(semaphore_t *sem);

// signal operation to relese a resource and wake up someone waiting
void semaphore_signal(semaphore_t *sem);

// clean up the semaphore when done
void semaphore_destroy(semaphore_t *sem);

// end of the header guard
#endif