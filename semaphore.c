#include "semaphore.h"
#include <stdlib.h>

// setting up a new semaphore
void semaphore_init(semaphore_t *sem, int value) {
  // start with however many resources we got
  sem->count = value;
  // make a new lock for protecting our stuff
  pthread_mutex_init(&sem->mutex, NULL);
  // make a condition variable for blocking threads
  pthread_cond_init(&sem->cond, NULL);
  // no one's waiting yet so queue is empty
  sem->head = NULL;
  sem->tail = NULL;
}

// trying to get a resource (the wait operation)
void semaphore_wait(semaphore_t *sem) {
  // lock things down so only we can touch the semaphore
  pthread_mutex_lock(&sem->mutex);

  // take one resource (even if there isn't one available yet)
  sem->count--;

  // if count went negative, we gotta wait our turn
  if (sem->count < 0) {
    // make a spot in line for ourselves
    qnode_t *node = (qnode_t *)malloc(sizeof(qnode_t));
    // remember who we are
    node->tid = pthread_self();
    // we're last in line so no one after us
    node->next = NULL;

    // adding ourselves to the waiting queue
    if (sem->tail == NULL) {
      // queue's empty, we're first in line
      sem->head = node;
      sem->tail = node;
    } else {
      // there's already people waiting, get in back
      sem->tail->next = node;
      sem->tail = node;
    }

    // go to sleep until someone wakes us up
    // this automatically unlocks the mutex while we sleep
    pthread_cond_wait(&sem->cond, &sem->mutex);

    // we woke up! time to leave the queue
    if (sem->head != NULL) {
      // grab the front person (should be us)
      qnode_t *to_free = sem->head;
      // move the line forward
      sem->head = sem->head->next;
      // if queue's empty now, update tail too
      if (sem->head == NULL) {
        sem->tail = NULL;
      }
      // free up our spot in line
      free(to_free);
    }
  }

  // all done, unlock so others can use the semaphore
  pthread_mutex_unlock(&sem->mutex);
}

// giving back a resource (the signal operation)
void semaphore_signal(semaphore_t *sem) {
  // lock it up while we work
  pthread_mutex_lock(&sem->mutex);

  // add one resource back
  sem->count++;

  // if count is still not positive, someone's waiting
  if (sem->count <= 0) {
    // wake up the next person in line
    pthread_cond_signal(&sem->cond);
  }

  // unlock and let others do their thing
  pthread_mutex_unlock(&sem->mutex);
}

// cleaning up when we're done with the semaphore
void semaphore_destroy(semaphore_t *sem) {
  // destroy the lock
  pthread_mutex_destroy(&sem->mutex);
  // destroy the condition variable
  pthread_cond_destroy(&sem->cond);

  // clean up any leftover queue nodes (just in case)
  while (sem->head != NULL) {
    // save the current front
    qnode_t *temp = sem->head;
    // move to next
    sem->head = sem->head->next;
    // free the old front
    free(temp);
  }
}