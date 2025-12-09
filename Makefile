CC = gcc
CFLAGS = -Wall -pthread -g
TARGETS = dining_deadlock dining_deadlock_free producer_consumer

all: $(TARGETS)

dining_deadlock: dining_deadlock.o semaphore.o
	$(CC) $(CFLAGS) -o dining_deadlock dining_deadlock.o semaphore.o

dining_deadlock_free: dining_deadlock_free.o semaphore.o
	$(CC) $(CFLAGS) -o dining_deadlock_free dining_deadlock_free.o semaphore.o

producer_consumer: producer_consumer.o semaphore.o
	$(CC) $(CFLAGS) -o producer_consumer producer_consumer.o semaphore.o

dining_deadlock.o: dining_deadlock.c semaphore.h
	$(CC) $(CFLAGS) -c dining_deadlock.c

dining_deadlock_free.o: dining_deadlock_free.c semaphore.h
	$(CC) $(CFLAGS) -c dining_deadlock_free.c

producer_consumer.o: producer_consumer.c semaphore.h
	$(CC) $(CFLAGS) -c producer_consumer.c

semaphore.o: semaphore.c semaphore.h
	$(CC) $(CFLAGS) -c semaphore.c

clean:
	rm -f *.o $(TARGETS) output_exp1.csv output_exp2.csv

.PHONY: all clean