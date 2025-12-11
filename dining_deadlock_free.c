#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

semaphore_t *chopsticks;  // all the chopsticks on the table

// how many philosophers we got
int num_philosophers;
// each philosopher eats this many times (100)
int num_meals = 100;
// lock for clean printing
pthread_mutex_t print_mutex;

// random sleep helper
void nap(int max_time_us) { 
  // sleep for random time
  usleep(rand() % max_time_us); 
}

// what each philosopher does (the deadlock-free version)
void *philosopher(void *arg) {
  // figure out who we are
  int id = *(int *)arg;
  // chopstick to the right
  int right = (id + 1) % num_philosophers;
  // chopstick to the left
  int left = id;

  // eat 100 times then we're done
  for (int meal = 0; meal < num_meals; meal++) {
    // the odd-even trick is what prevents deadlocks
    // even philosophers pick right first, odd philosophers pick left first
    if (id % 2 == 0) {
      // EVEN philosophers grab RIGHT first
      
      // try to get right chopstick
      semaphore_wait(&chopsticks[right]);
      // let everyone know we got it
      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, right);
      pthread_mutex_unlock(&print_mutex);

      // wait a tiny bit
      nap(10000);

      // now get left chopstick
      semaphore_wait(&chopsticks[left]);
      // announce it
      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, left);
      pthread_mutex_unlock(&print_mutex);
    } else {
      // ODD philosophers grab LEFT first
      
      // get left chopstick
      semaphore_wait(&chopsticks[left]);
      // say we got it
      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, left);
      pthread_mutex_unlock(&print_mutex);

      // little delay
      nap(10000);

      // now get right chopstick
      semaphore_wait(&chopsticks[right]);
      // announce it
      pthread_mutex_lock(&print_mutex);
      printf("Meal %d: Phil %d acquired chopstick %d\n", meal + 1, id, right);
      pthread_mutex_unlock(&print_mutex);
    }

    // got both chopsticks! time to eat
    pthread_mutex_lock(&print_mutex);
    printf("Meal %d: Phil %d eating\n", meal + 1, id);
    pthread_mutex_unlock(&print_mutex);

    // eating...
    nap(5000);

    // done eating
    pthread_mutex_lock(&print_mutex);
    printf("Meal %d: Phil %d finished eating\n", meal + 1, id);
    pthread_mutex_unlock(&print_mutex);

    // put down left chopstick
    semaphore_signal(&chopsticks[left]);
    // put down right chopstick
    semaphore_signal(&chopsticks[right]);

    // think for a bit before eating again
    nap(5000);
  }

  // clean up our ID memory
  free(arg);
  // thread's done
  return NULL;
}

// main function where it all begins
int main(int argc, char *argv[]) {
  // check if they used it right
  if (argc != 2) {
    // nope, show them how
    fprintf(stderr, "Usage: %s <NPROCS>\n", argv[0]);
    // exit
    return 1;
  }

  // get number of philosophers from command line
  num_philosophers = atoi(argv[1]);
  // assignment says minimum 5
  if (num_philosophers < 5) {
    // not enough philosophers
    fprintf(stderr, "Number of philosophers must be at least 5\n");
    // bail out
    return 1;
  }

  // make space for all the chopsticks
  chopsticks = (semaphore_t *)malloc(num_philosophers * sizeof(semaphore_t));
  // set up the print lock
  pthread_mutex_init(&print_mutex, NULL);

  // make each chopstick (binary semaphore starting at 1)
  for (int i = 0; i < num_philosophers; i++) {
    // only one person can hold a chopstick at a time
    semaphore_init(&chopsticks[i], 1);
  }

  // make space for all the threads
  pthread_t *threads = (pthread_t *)malloc(num_philosophers * sizeof(pthread_t));

  // create all the philosopher threads
  for (int i = 0; i < num_philosophers; i++) {
    // allocate memory for the ID
    int *id = (int *)malloc(sizeof(int));
    // set the ID
    *id = i;
    // start the philosopher thread
    pthread_create(&threads[i], NULL, philosopher, id);
  }

  // wait for everyone to finish their meals
  for (int i = 0; i < num_philosophers; i++) {
    // wait for this philosopher
    pthread_join(threads[i], NULL);
  }

  // cleanup time - destroy all chopsticks
  for (int i = 0; i < num_philosophers; i++) {
    semaphore_destroy(&chopsticks[i]);
  }

  // clean up the print lock
  pthread_mutex_destroy(&print_mutex);
  // free chopsticks array
  free(chopsticks);
  // free threads array
  free(threads);

  // success message
  printf("\nAll philosophers completed %d meals successfully!\n", num_meals);

  // end
  return 0;
}