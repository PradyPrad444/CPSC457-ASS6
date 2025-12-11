#include "semaphore.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define ITEMS_PER_PRODUCER 10

// Type definition of the circular buffer structure
typedef struct {
  int *buffer;
  int capacity;
  int count;
  int in;
  int out;
  semaphore_t empty;
  semaphore_t full;
  pthread_mutex_t mutex;
} circular_buffer_t;

typedef struct {
  int producer_id;
  circular_buffer_t *buffer;
  int *next_item;
  pthread_mutex_t *item_mutex;
} producer_arg_t;

typedef struct {
  int consumer_id;
  circular_buffer_t *buffer;
  int *consumed_count;
  pthread_mutex_t *count_mutex;
  int total_items;
} consumer_arg_t;

void buffer_init(circular_buffer_t *buf, int capacity) {
  buf->buffer = (int *)malloc(capacity * sizeof(int));
  buf->capacity = capacity;
  buf->count = 0;
  buf->in = 0;
  buf->out = 0;
  semaphore_init(&buf->empty, capacity);
  semaphore_init(&buf->full, 0);
  pthread_mutex_init(&buf->mutex, NULL);
}

void buffer_destroy(circular_buffer_t *buf) {
  free(buf->buffer);
  semaphore_destroy(&buf->empty);
  semaphore_destroy(&buf->full);
  pthread_mutex_destroy(&buf->mutex);
}

void buffer_produce(circular_buffer_t *buf, int item) {
  semaphore_wait(&buf->empty);
  pthread_mutex_lock(&buf->mutex);

  buf->buffer[buf->in] = item;
  printf("  [Buffer] Produced item %d at index %d (count: %d)\n", item, buf->in,
         buf->count + 1);
  buf->in = (buf->in + 1) % buf->capacity;
  buf->count++;

  pthread_mutex_unlock(&buf->mutex);
  semaphore_signal(&buf->full);
}

int buffer_consume(circular_buffer_t *buf) {
  semaphore_wait(&buf->full);
  pthread_mutex_lock(&buf->mutex);

  int item = buf->buffer[buf->out];
  printf("  [Buffer] Consumed item %d from index %d (count: %d)\n", item,
         buf->out, buf->count - 1);
  buf->out = (buf->out + 1) % buf->capacity;
  buf->count--;

  pthread_mutex_unlock(&buf->mutex);
  semaphore_signal(&buf->empty);

  return item;
}

void *producer_thread(void *arg) {
  producer_arg_t *parg = (producer_arg_t *)arg;

  for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
    pthread_mutex_lock(parg->item_mutex);
    int item = (*parg->next_item)++;
    pthread_mutex_unlock(parg->item_mutex);

    printf("Producer %d: producing item %d\n", parg->producer_id, item);
    buffer_produce(parg->buffer, item);
    usleep(50000); // 50ms delay to see the flow
  }

  printf("Producer %d: FINISHED (produced %d items)\n", parg->producer_id,
         ITEMS_PER_PRODUCER);
  return NULL;
}

void *consumer_thread(void *arg) {
  consumer_arg_t *carg = (consumer_arg_t *)arg;

  while (1) {
    // Check BEFORE trying to consume
    pthread_mutex_lock(carg->count_mutex);
    int current_count = *carg->consumed_count;
    pthread_mutex_unlock(carg->count_mutex);

    if (current_count >= carg->total_items) {
      printf("Consumer %d: FINISHED (saw total consumed: %d)\n",
             carg->consumer_id, current_count);
      break;
    }

    int item = buffer_consume(carg->buffer);
    printf("Consumer %d: consumed item %d\n", carg->consumer_id, item);

    pthread_mutex_lock(carg->count_mutex);
    (*carg->consumed_count)++;
    pthread_mutex_unlock(carg->count_mutex);

    usleep(60000); // 60ms delay to see the flow
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <num_producers> <num_consumers>\n", argv[0]);
    fprintf(stderr, "Example: %s 2 3\n", argv[0]);
    return 1;
  }

  int num_producers = atoi(argv[1]);
  int num_consumers = atoi(argv[2]);

  if (num_producers < 1 || num_consumers < 1) {
    fprintf(stderr, "Number of producers and consumers must be at least 1\n");
    return 1;
  }

  srand(time(NULL));

  circular_buffer_t buffer;
  buffer_init(&buffer, BUFFER_SIZE);

  int next_item = 0;
  int consumed_count = 0;
  pthread_mutex_t item_mutex, count_mutex;
  pthread_mutex_init(&item_mutex, NULL);
  pthread_mutex_init(&count_mutex, NULL);

  int total_items = num_producers * ITEMS_PER_PRODUCER;

  printf("PRODUCER-CONSUMER Test\n");
  printf("Buffer size: %d\n", BUFFER_SIZE);
  printf("Producers: %d (each produces %d items)\n", num_producers,
         ITEMS_PER_PRODUCER);
  printf("Consumers: %d\n", num_consumers);
  printf("Total items to produce: %d\n", total_items);
  printf("\n\n");

  pthread_t *producers = (pthread_t *)malloc(num_producers * sizeof(pthread_t));
  pthread_t *consumers = (pthread_t *)malloc(num_consumers * sizeof(pthread_t));
  producer_arg_t *prod_args =
      (producer_arg_t *)malloc(num_producers * sizeof(producer_arg_t));
  consumer_arg_t *cons_args =
      (consumer_arg_t *)malloc(num_consumers * sizeof(consumer_arg_t));

  // Create producers
  for (int i = 0; i < num_producers; i++) {
    prod_args[i].producer_id = i;
    prod_args[i].buffer = &buffer;
    prod_args[i].next_item = &next_item;
    prod_args[i].item_mutex = &item_mutex;
    pthread_create(&producers[i], NULL, producer_thread, &prod_args[i]);
  }

  // Create consumers
  for (int i = 0; i < num_consumers; i++) {
    cons_args[i].consumer_id = i;
    cons_args[i].buffer = &buffer;
    cons_args[i].consumed_count = &consumed_count;
    cons_args[i].count_mutex = &count_mutex;
    cons_args[i].total_items = total_items;
    pthread_create(&consumers[i], NULL, consumer_thread, &cons_args[i]);
  }

  // Wait for producers
  for (int i = 0; i < num_producers; i++) {
    pthread_join(producers[i], NULL);
  }
  printf("\nAll Producers Finished\n\n");

  // Wait for consumers
  for (int i = 0; i < num_consumers; i++) {
    pthread_join(consumers[i], NULL);
  }

  printf("\nTest Complete\n");
  printf("Total items produced: %d\n", next_item);
  printf("Total items consumed: %d\n", consumed_count);

  if (next_item == consumed_count && consumed_count == total_items) {
    printf("All items produced and consumed correctly!\n");
  } else {
    printf("Error: Mismatch in counts!\n");
  }

  // Cleanup
  buffer_destroy(&buffer);
  pthread_mutex_destroy(&item_mutex);
  pthread_mutex_destroy(&count_mutex);
  free(producers);
  free(consumers);
  free(prod_args);
  free(cons_args);

  return 0;
}