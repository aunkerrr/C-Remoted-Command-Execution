CC = gcc
CFLAGS = -g -Wall -Wextra -pthread
TARGETS = server client

.PHONY: all clean
.DEFAULT_GOAL := all

all: $(TARGETS)

server: server.c common.h
	$(CC) $(CFLAGS) server.c -o server

client: client.c common.h
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f $(TARGETS) *.o
