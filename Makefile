CC = gcc
CFLAGS = -pthread -Wall -Wextra -g
TARGETS = dining_deadlock dining_deadlock_free producer_consumer

all: $(TARGETS)

dining_deadlock: dining_deadlock.c semaphore.c semaphore.h
	$(CC) $(CFLAGS) -o dining_deadlock dining_deadlock.c semaphore.c

dining_deadlock_free: dining_deadlock_free.c semaphore.c semaphore.h
	$(CC) $(CFLAGS) -o dining_deadlock_free dining_deadlock_free.c semaphore.c

producer_consumer: producer_consumer.c semaphore.c semaphore.h
	$(CC) $(CFLAGS) -o producer_consumer producer_consumer.c semaphore.c

clean:
	rm -f $(TARGETS) *.o output_exp1.csv output_exp2.csv

.PHONY: all clean