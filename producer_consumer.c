
#include "semaphore.h"
#include <pthread.h> // need threads
#include <stdio.h> // printing
#include <stdlib.h> // need malloc
#include <string.h> // need strcmp
#include <sys/time.h> // need timing functions
#include <time.h> // need time for random seed


#define BUFFER_SIZE 100    // buffer holds 100 items max
#define ITEMS_PER_PRODUCER 100    // each producer makes 100 items

// the circular buffer that producers and consumers share
typedef struct {
  int *buffer;           // where we actually store the items
  int capacity;          // how big the buffer is
  int in;                // where we put the next item
  int out;               // where we take the next item from
  semaphore_t empty;     // counts how many empty spots we have
  semaphore_t full;      // counts how many full spots we have
  pthread_mutex_t mutex; // lock for protecting the buffer
} circular_buffer_t;

// what we pass to producer threads
typedef struct {
  circular_buffer_t *buffer;  // pointer to the shared buffer
  int items_to_produce;       // how many items this producer should make
} producer_arg_t;

// what we pass to consumer threads
typedef struct {
  circular_buffer_t *buffer;  // pointer to the shared buffer
  int items_to_consume;       // how many items this consumer should eat
} consumer_arg_t;

// setting up the circular buffer
void buffer_init(circular_buffer_t *buf, int capacity) {
  // allocate space for the actual buffer
  buf->buffer = malloc(capacity * sizeof(int));
  // remember how big it is
  buf->capacity = capacity;
  // start inserting at position 0
  buf->in = 0;
  // start removing at position 0
  buf->out = 0;
  // all slots start empty
  semaphore_init(&buf->empty, capacity);
  // no slots start full
  semaphore_init(&buf->full, 0);
  // make the lock
  pthread_mutex_init(&buf->mutex, NULL);
}

// cleaning up the buffer when done
void buffer_destroy(circular_buffer_t *buf) {
  // free the buffer array
  free(buf->buffer);
  // destroy the empty semaphore
  semaphore_destroy(&buf->empty);
  // destroy the full semaphore
  semaphore_destroy(&buf->full);
  // destroy the lock
  pthread_mutex_destroy(&buf->mutex);
}

// producer putting an item in the buffer
void buffer_produce(circular_buffer_t *buf, int item) {
  // wait for an empty slot (blocks if buffer is full)
  semaphore_wait(&buf->empty);
  // lock the buffer so only we can modify it
  pthread_mutex_lock(&buf->mutex);
  // put the item in
  buf->buffer[buf->in] = item;
  // move to next position (wraps around)
  buf->in = (buf->in + 1) % buf->capacity;
  // unlock the buffer
  pthread_mutex_unlock(&buf->mutex);
  // signal that there's one more full slot now
  semaphore_signal(&buf->full);
}

// consumer taking an item from the buffer
int buffer_consume(circular_buffer_t *buf) {
  // wait for a full slot (blocks if buffer is empty)
  semaphore_wait(&buf->full);
  // lock the buffer
  pthread_mutex_lock(&buf->mutex);
  // grab the item
  int item = buf->buffer[buf->out];
  // move to next position (wraps around)
  buf->out = (buf->out + 1) % buf->capacity;
  // unlock the buffer
  pthread_mutex_unlock(&buf->mutex);
  // signal that there's one more empty slot now
  semaphore_signal(&buf->empty);
  // return what we got
  return item;
}

// what each producer thread does
void *producer_thread(void *arg) {
  // get our arguments
  producer_arg_t *p = arg;

  // produce the specified number of items
  for (int i = 0; i < p->items_to_produce; i++) {
    // put item in buffer (just using i as the item value)
    buffer_produce(p->buffer, i);
  }

  // producer's done
  return NULL;
}

// what each consumer thread does
void *consumer_thread(void *arg) {
  // get our arguments
  consumer_arg_t *c = arg;

  // consume the specified number of items
  for (int i = 0; i < c->items_to_consume; i++) {
    // take item from buffer
    int item = buffer_consume(c->buffer);
    // we don't actually use it, just throw it away
    (void)item;
  }

  // consumer's done
  return NULL;
}

// helper to get current time in milliseconds
double get_time_ms() {
  // struct for storing time
  struct timeval tv;
  // get the time
  gettimeofday(&tv, NULL);
  // convert to milliseconds
  return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// run one experiment with specific numbers of producers and consumers
void run_experiment(int num_producers, int num_consumers, FILE *output_file) {
  // make a new buffer
  circular_buffer_t buffer;
  // initialize it
  buffer_init(&buffer, BUFFER_SIZE);

  // figure out how many items total
  int total_items = num_producers * ITEMS_PER_PRODUCER;
  // split work evenly among consumers
  int items_per_consumer = total_items / num_consumers;
  // some consumers might need to do one extra
  int extra_items = total_items % num_consumers;

  // make space for producer threads
  pthread_t *producers = malloc(num_producers * sizeof(pthread_t));
  // make space for consumer threads
  pthread_t *consumers = malloc(num_consumers * sizeof(pthread_t));
  // make space for producer arguments
  producer_arg_t *prod_args = malloc(num_producers * sizeof(producer_arg_t));
  // make space for consumer arguments
  consumer_arg_t *cons_args = malloc(num_consumers * sizeof(consumer_arg_t));

  // start the timer
  double start_time = get_time_ms();

  // create all the producer threads
  for (int i = 0; i < num_producers; i++) {
    // give them the buffer
    prod_args[i].buffer = &buffer;
    // tell them how much to produce
    prod_args[i].items_to_produce = ITEMS_PER_PRODUCER;
    // start the thread
    pthread_create(&producers[i], NULL, producer_thread, &prod_args[i]);
  }

  // create all the consumer threads
  for (int i = 0; i < num_consumers; i++) {
    // give them the buffer
    cons_args[i].buffer = &buffer;
    // tell them how much to consume
    cons_args[i].items_to_consume = items_per_consumer;
    // first few get one extra item if there's a remainder
    if (i < extra_items) {
      cons_args[i].items_to_consume++;
    }
    // start the thread
    pthread_create(&consumers[i], NULL, consumer_thread, &cons_args[i]);
  }

  // wait for all producers to finish
  for (int i = 0; i < num_producers; i++) {
    // block until this producer is done
    pthread_join(producers[i], NULL);
  }

  // wait for all consumers to finish
  for (int i = 0; i < num_consumers; i++) {
    // block until this consumer is done
    pthread_join(consumers[i], NULL);
  }

  // stop the timer
  double end_time = get_time_ms();
  // calculate how long it took
  double elapsed = end_time - start_time;
  // calculate throughput (time per item)
  double throughput = elapsed / total_items;

  // write results to CSV file
  fprintf(output_file, "%d,%d,%f\n", num_producers, num_consumers, throughput);

  // cleanup time
  buffer_destroy(&buffer);
  free(producers);
  free(consumers);
  free(prod_args);
  free(cons_args);
}

// main function where everything starts
int main(int argc, char *argv[]) {
  // check if they used it right
  if (argc != 2) {
    // if wrong, show corect usage format
    fprintf(stderr, "Usage: %s <fixed_producers|fixed_consumers>\n", argv[0]);
    // bail
    return 1;
  }

  // seed the random number generator
  srand(time(NULL));

  // check which experiment they want
  if (strcmp(argv[1], "fixed_producers") == 0) {
    // experiment 1: 10 producers, varying consumers
    
    // open the output file
    FILE *output = fopen("output_exp1.csv", "w");
    // make sure it opened
    if (!output) {
      // nope, couldn't open it
      perror("Failed to open output_exp1.csv");
      // bail
      return 1;
    }

    // write the CSV header
    fprintf(output, "producers,consumers,throughput\n");

    // run experiments with 10 to 300 consumers (by 10s)
    for (int c = 10; c <= 300; c += 10) {
      // run with 10 producers and c consumers
      run_experiment(10, c, output);
    }

    // close the file
    fclose(output);

  } else if (strcmp(argv[1], "fixed_consumers") == 0) {
    // experiment 2: varying producers, 10 consumers
    
    // open the output file
    FILE *output = fopen("output_exp2.csv", "w");
    // make sure it opened
    if (!output) {
      // couldn't open it
      perror("Failed to open output_exp2.csv");
      // bail
      return 1;
    }

    // write the CSV header
    fprintf(output, "producers,consumers,throughput\n");

    // run experiments with 10 to 300 producers (by 10s)
    for (int p = 10; p <= 300; p += 10) {
      // run with p producers and 10 consumers
      run_experiment(p, 10, output);
    }

    // close the file
    fclose(output);

  } else {
    // they didn't give a valid argument
    fprintf(stderr, "Invalid argument.\n");
    // bail
    return 1;
  }

  // all done
  return 0;
}