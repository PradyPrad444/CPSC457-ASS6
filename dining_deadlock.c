#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Semaphore *chopsticks;
int num_philosophers;
pthread_mutex_t print_mutex;

void nap(int max_time_us) { usleep(rand() % max_time_us); }

void *philosopher(void *arg) {
  int id = *(int *)arg;
  int right = (id + 1) % num_philosophers;
  int left = id;

  // Pick up RIGHT chopstick first
  pthread_mutex_lock(&print_mutex);
  printf("Philosopher %d wants RIGHT chopstick %d\n", id, right);
  pthread_mutex_unlock(&print_mutex);

  semaphore_wait(&chopsticks[right]);

  pthread_mutex_lock(&print_mutex);
  printf("Philosopher %d picked RIGHT chopstick %d\n", id, right);
  pthread_mutex_unlock(&print_mutex);

  nap(100000); // Delay to increase deadlock chance

  // Pick up LEFT chopstick
  pthread_mutex_lock(&print_mutex);
  printf("Philosopher %d wants LEFT chopstick %d\n", id, left);
  pthread_mutex_unlock(&print_mutex);

  semaphore_wait(&chopsticks[left]);

  pthread_mutex_lock(&print_mutex);
  printf("Philosopher %d picked LEFT chopstick %d\n", id, left);
  printf("Philosopher %d eating\n", id);
  pthread_mutex_unlock(&print_mutex);

  nap(50000); // Eating time

  pthread_mutex_lock(&print_mutex);
  printf("Philosopher %d finished eating\n", id);
  pthread_mutex_unlock(&print_mutex);

  // Put down chopsticks
  semaphore_signal(&chopsticks[left]);
  semaphore_signal(&chopsticks[right]);

  free(arg);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <NPROCS>\n", argv[0]);
    return 1;
  }

  num_philosophers = atoi(argv[1]);
  if (num_philosophers < 2) {
    fprintf(stderr, "Number of philosophers must be at least 2\n");
    return 1;
  }

  chopsticks = (Semaphore *)malloc(num_philosophers * sizeof(Semaphore));
  pthread_mutex_init(&print_mutex, NULL);

  for (int i = 0; i < num_philosophers; i++) {
    semaphore_init(&chopsticks[i], 1);
  }

  pthread_t *threads =
      (pthread_t *)malloc(num_philosophers * sizeof(pthread_t));

  for (int i = 0; i < num_philosophers; i++) {
    int *id = (int *)malloc(sizeof(int));
    *id = i;
    pthread_create(&threads[i], NULL, philosopher, id);
  }

  for (int i = 0; i < num_philosophers; i++) {
    pthread_join(threads[i], NULL);
  }

  for (int i = 0; i < num_philosophers; i++) {
    semaphore_destroy(&chopsticks[i]);
  }

  pthread_mutex_destroy(&print_mutex);
  free(chopsticks);
  free(threads);

  return 0;
}