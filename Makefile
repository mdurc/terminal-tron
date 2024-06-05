CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lncurses

all: server.out client.out

server.out: server.c
	$(CC) $(CFLAGS) -o server.out server.c

client.out: client.c
	$(CC) $(CFLAGS) -o client.out client.c $(LIBS)

