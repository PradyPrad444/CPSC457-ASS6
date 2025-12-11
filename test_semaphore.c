#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

semaphore_t sem;
int counter = 0;

void *thread_func(void *arg) {
  int id = *(int *)arg;

  printf("Thread %d: waiting on semaphore...\n", id);
  semaphore_wait(&sem);

  printf("Thread %d: got semaphore! counter=%d\n", id, counter);
  counter++;
  sleep(1);

  printf("Thread %d: signaling semaphore\n", id);
  semaphore_signal(&sem);

  return NULL;
}

int main() {
  semaphore_init(&sem, 1); // Binary semaphore

  pthread_t threads[5];
  int ids[5];

  for (int i = 0; i < 5; i++) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, thread_func, &ids[i]);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Final counter: %d (should be 5)\n", counter);
  semaphore_destroy(&sem);

  return 0;
}