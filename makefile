CC = gcc
CFLAGS = -std=c11 -O3 -Wall
CPPFLAGS = -I.
LDFLAGS = -pthread

.PHONY: clean test

all: server client supervisor

linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c linkedlist.c -o $@

supervisor: supervisor.c linkedlist.o
	$(CC) $(CFLAGS) $(CPPFLAGS) linkedlist.o supervisor.c -o $@

clean:
	rm OOB-server-* server client supervisor *.log *.o

continuous:
	bash continuous_test.sh

test:
	bash test.sh
