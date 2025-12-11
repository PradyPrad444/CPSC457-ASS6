#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

semaphore_t *chopsticks;        // array holding all the chopsticks (each chopstick is a semaphore)

int num_philosophers;           // how many philosophers are sitting at the table

// lock for making sure print statements don't get all jumbled up
pthread_mutex_t print_mutex;

// helper for sleep for a random amount of time
void nap(int max_time_us) { 
  // sleep for some random time up to max_time_us microseconds
  usleep(rand() % max_time_us); 
}

// what each philosopher does 
void *philosopher(void *arg) {
  // figure out who we are
  int id = *(int *)arg;
  // the chopstick to our right (wraps around)
  int right = (id + 1) % num_philosophers;
  // the chopstick to our left (same as our ID)
  int left = id;

  // philosophers just keep trying to eat forever
  while (1) {
    // lock before printing so messages don't overlap
    pthread_mutex_lock(&print_mutex);
    // say we want the right chopstick
    printf("Philosopher %d wants RIGHT chopstick %d\n", id, right);
    // unlock so others can print
    pthread_mutex_unlock(&print_mutex);

    // try to grab RIGHT chopstick first (THIS IS THE PROBLEM - everyone does this)
    semaphore_wait(&chopsticks[right]);

    // we got it! let everyone know
    pthread_mutex_lock(&print_mutex);
    printf("Philosopher %d picked RIGHT chopstick %d\n", id, right);
    pthread_mutex_unlock(&print_mutex);

    // wait a bit (this makes deadlock more likely to happen)
    nap(100000);

    // now try to get the LEFT chopstick
    pthread_mutex_lock(&print_mutex);
    printf("Philosopher %d wants LEFT chopstick %d\n", id, left);
    pthread_mutex_unlock(&print_mutex);

    // THIS IS WHERE DEADLOCK HAPPENS
    // everyone's holding their right chopstick and waiting for their left
    // but their left is someone else's right, so everyone just waits forever
    semaphore_wait(&chopsticks[left]);

    // if we get here, we got both chopsticks (won't happen in deadlock)
    pthread_mutex_lock(&print_mutex);
    printf("Philosopher %d picked LEFT chopstick %d\n", id, left);
    printf("Philosopher %d eating\n", id);
    pthread_mutex_unlock(&print_mutex);

    // nom nom nom
    nap(50000);

    // done eating, let everyone know
    pthread_mutex_lock(&print_mutex);
    printf("Philosopher %d finished eating\n", id);
    pthread_mutex_unlock(&print_mutex);

    // put down left chopstick
    semaphore_signal(&chopsticks[left]);
    // put down right chopstick
    semaphore_signal(&chopsticks[right]);
  }

  // never gets here because of infinite loop
  return NULL;
}

// main function where everything starts
int main(int argc, char *argv[]) {
  // make sure they gave us the right number of arguments
  if (argc != 2) {
    // nope, tell them how to use it
    fprintf(stderr, "Usage: %s <NPROCS>\n", argv[0]);
    // bail out
    return 1;
  }

  // get the number of philosophers from command line
  num_philosophers = atoi(argv[1]);
  // gotta have at least 2
  if (num_philosophers < 2) {
    // oops, not enough
    fprintf(stderr, "Number of philosophers must be at least 2\n");
    // bail
    return 1;
  }

  // make space for all the chopsticks
  chopsticks = (semaphore_t *)malloc(num_philosophers * sizeof(semaphore_t));
  // set up the print lock
  pthread_mutex_init(&print_mutex, NULL);

  // initialize each chopstick as a binary semaphore (starts at 1)
  for (int i = 0; i < num_philosophers; i++) {
    // each chopstick can only be held by one person at a time
    semaphore_init(&chopsticks[i], 1);
  }

  // make space for all the philosopher threads
  pthread_t *threads = (pthread_t *)malloc(num_philosophers * sizeof(pthread_t));
  // make space to store all their IDs
  int *ids = (int *)malloc(num_philosophers * sizeof(int));

  // create all the philosopher threads
  for (int i = 0; i < num_philosophers; i++) {
    // save their ID
    ids[i] = i;
    // start the thread running the philosopher function
    pthread_create(&threads[i], NULL, philosopher, &ids[i]);
  }

  // wait for all threads to finish (they won't - deadlock happens)
  for (int i = 0; i < num_philosophers; i++) {
    // this blocks forever when deadlock occurs
    pthread_join(threads[i], NULL);
  }

  // cleanup code (never reached because of deadlock)
  // destroy all the chopstick semaphores
  for (int i = 0; i < num_philosophers; i++) {
    semaphore_destroy(&chopsticks[i]);
  }

  // clean up the print lock
  pthread_mutex_destroy(&print_mutex);
  // free the chopsticks array
  free(chopsticks);
  // free the threads array
  free(threads);
  // free the IDs array
  free(ids);

  // if we somehow got here, we're done
  return 0;
}