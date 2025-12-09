#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Semaphore *chopsticks;
int num_philosophers;
int num_meals = 100;
pthread_mutex_t print_mutex;

void nap(int max_time_us) { usleep(rand() % max_time_us); }

void *philosopher(void *arg) {
  int id = *(int *)arg;
  int right = (id + 1) % num_philosophers;
  int left = id;

  for (int meal = 0; meal < num_meals; meal++) {
    // Odd-even solution: odd philosophers pick left first, even pick right
    // first
    if (id % 2 == 0) {
      // Even: pick right then left
      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, right);
      pthread_mutex_unlock(&print_mutex);
      semaphore_wait(&chopsticks[right]);

      nap(10000);

      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, left);
      pthread_mutex_unlock(&print_mutex);
      semaphore_wait(&chopsticks[left]);
    } else {
      // Odd: pick left then right
      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, left);
      pthread_mutex_unlock(&print_mutex);
      semaphore_wait(&chopsticks[left]);

      nap(10000);

      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, right);
      pthread_mutex_unlock(&print_mutex);
      semaphore_wait(&chopsticks[right]);
    }

    // Eating
    pthread_mutex_lock(&print_mutex);
    printf("Meal %d: Phil %d eating\n", meal + 1, id);
    pthread_mutex_unlock(&print_mutex);

    nap(5000);

    pthread_mutex_lock(&print_mutex);
    printf("Meal %d: Phil %d finished eating\n", meal + 1, id);
    pthread_mutex_unlock(&print_mutex);

    // Put down chopsticks
    semaphore_signal(&chopsticks[left]);
    semaphore_signal(&chopsticks[right]);

    nap(5000); // Thinking time
  }

  free(arg);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <NPROCS>\n", argv[0]);
    return 1;
  }

  num_philosophers = atoi(argv[1]);
  if (num_philosophers < 5) {
    fprintf(stderr, "Number of philosophers must be at least 5\n");
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

  printf("\nAll philosophers completed %d meals successfully!\n", num_meals);

  return 0;
}