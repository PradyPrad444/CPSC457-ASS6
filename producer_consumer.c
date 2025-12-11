#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define BUFFER_SIZE 100
#define ITEMS_PER_PRODUCER 100

// circular buffer structure
typedef struct {
  int *buffer;
  int in;  // next insert position
  int out; // next remove position
  int size;
  Semaphore empty; // tracks empty slots
  Semaphore full;  // tracks filled slots
  pthread_mutex_t mutex;
} CircularBuffer;

CircularBuffer shared_buffer;
int num_producers;
int num_consumers;
int total_items;

void buffer_init(CircularBuffer *buf, int size) {
  buf->buffer = (int *)malloc(size * sizeof(int));
  buf->in = 0;
  buf->out = 0;
  buf->size = size;
  semaphore_init(&buf->empty, size); // initially all empty
  semaphore_init(&buf->full, 0);     // initially none full
  pthread_mutex_init(&buf->mutex, NULL);
}

void buffer_destroy(CircularBuffer *buf) {
  free(buf->buffer);
  semaphore_destroy(&buf->empty);
  semaphore_destroy(&buf->full);
  pthread_mutex_destroy(&buf->mutex);
}

void buffer_insert(CircularBuffer *buf, int item) {
  // wait for empty slot
  semaphore_wait(&buf->empty);

  // critical section - update buffer
  pthread_mutex_lock(&buf->mutex);
  buf->buffer[buf->in] = item;
  buf->in = (buf->in + 1) % buf->size; // wrap around
  pthread_mutex_unlock(&buf->mutex);

  // signal that one more slot is full
  semaphore_signal(&buf->full);
}

int buffer_remove(CircularBuffer *buf) {
  // wait for full slot
  semaphore_wait(&buf->full);

  // critical section - remove from buffer
  pthread_mutex_lock(&buf->mutex);
  int item = buf->buffer[buf->out];
  buf->out = (buf->out + 1) % buf->size; // wrap around
  pthread_mutex_unlock(&buf->mutex);

  // signal that one more slot is empty
  semaphore_signal(&buf->empty);

  return item;
}

void *producer(void *arg) {
  int id = *(int *)arg;
  int base = id * ITEMS_PER_PRODUCER;

  // produce ITEMS_PER_PRODUCER items
  for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
    int item = base + i; // unique item id
    buffer_insert(&shared_buffer, item);
  }

  free(arg);
  return NULL;
}

void *consumer(void *arg) {
  int *items_to_consume = (int *)arg;
  int consumed = 0;

  // consume assigned number of items
  while (consumed < *items_to_consume) {
    buffer_remove(&shared_buffer);
    consumed++;
  }

  free(arg);
  return NULL;
}

double get_time_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void run_experiment(int n_prod, int n_cons, FILE *output) {
  num_producers = n_prod;
  num_consumers = n_cons;
  total_items = num_producers * ITEMS_PER_PRODUCER;

  buffer_init(&shared_buffer, BUFFER_SIZE);

  pthread_t *prod_threads =
      (pthread_t *)malloc(num_producers * sizeof(pthread_t));
  pthread_t *cons_threads =
      (pthread_t *)malloc(num_consumers * sizeof(pthread_t));

  double start_time = get_time_ms();

  // spawn producer threads
  for (int i = 0; i < num_producers; i++) {
    int *id = (int *)malloc(sizeof(int));
    *id = i;
    pthread_create(&prod_threads[i], NULL, producer, id);
  }

  // spawn consumer threads
  // distribute items evenly, handle remainder
  int items_per_consumer = total_items / num_consumers;
  int leftover = total_items % num_consumers;

  for (int i = 0; i < num_consumers; i++) {
    int *count = (int *)malloc(sizeof(int));
    // first 'leftover' consumers get one extra item
    *count = items_per_consumer;
    if (i < leftover) {
      *count = *count + 1;
    }
    pthread_create(&cons_threads[i], NULL, consumer, count);
  }

  // wait for producers to finish
  for (int i = 0; i < num_producers; i++) {
    pthread_join(prod_threads[i], NULL);
  }

  // wait for consumers to finish
  for (int i = 0; i < num_consumers; i++) {
    pthread_join(cons_threads[i], NULL);
  }

  double end_time = get_time_ms();
  double elapsed = end_time - start_time;
  double throughput = elapsed / total_items;

  fprintf(output, "%d,%d,%f\n", num_producers, num_consumers, throughput);
  printf("Producers: %d, Consumers: %d, Throughput: %f ms/item\n",
         num_producers, num_consumers, throughput);

  buffer_destroy(&shared_buffer);
  free(prod_threads);
  free(cons_threads);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <fixed_producers|fixed_consumers>\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "fixed_producers") == 0) {
    // experiment 1: fixed 10 producers, varying consumers
    FILE *fp = fopen("output_exp1.csv", "w");
    fprintf(fp, "producers,consumers,throughput\n");

    for (int cons = 10; cons <= 300; cons += 10) {
      run_experiment(10, cons, fp);
    }

    fclose(fp);
    printf("Experiment 1 completed. Results saved to output_exp1.csv\n");
  } else if (strcmp(argv[1], "fixed_consumers") == 0) {
    // experiment 2: varying producers, fixed 10 consumers
    FILE *fp = fopen("output_exp2.csv", "w");
    fprintf(fp, "producers,consumers,throughput\n");

    for (int prod = 10; prod <= 300; prod += 10) {
      run_experiment(prod, 10, fp);
    }

    fclose(fp);
    printf("Experiment 2 completed. Results saved to output_exp2.csv\n");
  } else {
    fprintf(stderr,
            "Invalid argument. Use 'fixed_producers' or 'fixed_consumers'\n");
    return 1;
  }

  return 0;
}